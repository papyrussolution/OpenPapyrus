/* GNUPLOT - mouse.c */

/* driver independent mouse part. */

/*[
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
 *   Original Software (October 1999 - January 2000):
 *     Pieter-Tjerk de Boer <ptdeboer@cs.utwente.nl>
 *     Petr Mikulik <mikulik@physics.muni.cz>
 *     Johannes Zellner <johannes@zellner.org>
 */
#include <gnuplot.h>
#pragma hdrstop
//#define _MOUSE_C                /* FIXME HBB 20010207: violates Codestyle */
#ifdef USE_MOUSE                /* comment out whole file, otherwise... */
	//#include "mouse.h"
	#ifdef _Windows
		#include "win/winmain.h"
	#endif
/********************** variables ***********************************************************/
char mouse_fmt_default[] = "% #g";

//GpMouse::Settings default_mouse_setting = DEFAULT_MOUSE_SETTING;

long   mouse_mode = MOUSE_COORDINATES_REAL;
char * mouse_alt_string = (char*) 0;
//
// variables for changing the 3D view:
//
bool   allowmotion = true; // do we allow motion to result in a replot right now? (used by pm.trm, too)

//
// enum of GP_ -keycodes has moved to mousecmn.h so that it can be
// accessed by standalone terminals too 
//
// FIXME HBB 20010207: Codestyle violation, again
// the following table must match exactly the enum's of GP_ and end with a NULL pointer!
//
const char * special_keys[] = {
    "GP_FIRST_KEY", /* keep this dummy there */
    "Linefeed",
    "Clear",
    "Pause",
    "Scroll_Lock",
    "Sys_Req",
    "Insert",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "PageUp",
    "PageDown",
    "End",
    "Begin",
    "KP_Space",
    "KP_Tab",
    "KP_F1",
    "KP_F2",
    "KP_F3",
    "KP_F4",

    /* see KP_0 - KP_9 */
    "KP_Insert",
    "KP_End",
    "KP_Down",
    "KP_PageDown",
    "KP_Left",
    "KP_Begin",
    "KP_Right",
    "KP_Home",
    "KP_Up",
    "KP_PageUp",

    "KP_Delete",
    "KP_Equal",
    "KP_Multiply",
    "KP_Add",
    "KP_Separator",
    "KP_Subtract",
    "KP_Decimal",
    "KP_Divide",
    "KP_0",
    "KP_1",
    "KP_2",
    "KP_3",
    "KP_4",
    "KP_5",
    "KP_6",
    "KP_7",
    "KP_8",
    "KP_9",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "Close",
    "Button1",
    "GP_LAST_KEY",
    (const char*)0 // must be the last line 
};
//
// "usual well-known" keycodes, i.e. those not listed in special_keys in mouse.h
//
static const GenTable usual_special_keys[] =
{
	{ "BackSpace", GP_BackSpace},
	{ "Tab", GP_Tab},
	{ "KP_Enter", GP_KP_Enter},
	{ "Return", GP_Return},
	{ "Escape", GP_Escape},
	{ "Delete", GP_Delete},
	{ NULL, 0}
};

// forward declarations 
static void alert();
static char * xy_format();
static char * zoombox_format();
//static char * GetAnnotateString(char * s, double x, double y, int mode, char * fmt);
static char * xDateTimeFormat(double x, char * b, int mode);
//static void GetRulerString(char * p, double x, double y);
//static void ZoomNext();
//static void ZoomPrevious();
//static void ZoomUnzoom();
static void incr_mousemode(const int amount);
//static void UpdateStatuslineWithMouseSetting(GpMouse::Settings * ms);
//static void ChangeView(int x, int z);
static void event_modifier(GpEvent* ge);
//static void load_mouse_variables(double, double, bool, int);

static void do_zoom_in_around_mouse();
static void do_zoom_out_around_mouse();
static void do_zoom_in_X();
static void do_zoom_out_X();
static void do_zoom_scroll_up();
static void do_zoom_scroll_down();
static void do_zoom_scroll_left();
static void do_zoom_scroll_right();

/* builtins */
static char * builtin_autoscale(GpEvent* ge);
static char * builtin_toggle_border(GpEvent* ge);
static char * builtin_replot(GpEvent* ge);
static char * builtin_toggle_grid(GpEvent* ge);
static char * builtin_help(GpEvent* ge);
static char * builtin_set_plots_visible(GpEvent* ge);
static char * builtin_set_plots_invisible(GpEvent* ge);
static char * builtin_invert_plot_visibilities(GpEvent* ge);
static char * builtin_toggle_log(GpEvent* ge);
static char * builtin_nearest_log(GpEvent* ge);
static char * builtin_toggle_mouse(GpEvent* ge);
static char * builtin_toggle_ruler(GpEvent* ge);
static char * builtin_decrement_mousemode(GpEvent* ge);
static char * builtin_increment_mousemode(GpEvent* ge);
static char * builtin_toggle_polardistance(GpEvent* ge);
static char * builtin_toggle_verbose(GpEvent* ge);
static char * builtin_toggle_ratio(GpEvent* ge);
static char * builtin_zoom_next(GpEvent* ge);
static char * builtin_zoom_previous(GpEvent* ge);
static char * builtin_unzoom(GpEvent* ge);
static char * builtin_rotate_right(GpEvent* ge);
static char * builtin_rotate_up(GpEvent* ge);
static char * builtin_rotate_left(GpEvent* ge);
static char * builtin_rotate_down(GpEvent* ge);
static char * builtin_cancel_zoom(GpEvent* ge);
static char * builtin_zoom_in_around_mouse(GpEvent* ge);
static char * builtin_zoom_out_around_mouse(GpEvent* ge);
#if(0) /* Not currently used */
static char * builtin_zoom_scroll_left(GpEvent* ge);
static char * builtin_zoom_scroll_right(GpEvent* ge);
static char * builtin_zoom_scroll_up(GpEvent* ge);
static char * builtin_zoom_scroll_down(GpEvent* ge);
static char * builtin_zoom_in_X(GpEvent* ge);
static char * builtin_zoom_out_X(GpEvent* ge);
#endif

/* prototypes for bind stuff
 * which are used only here. */
//static void bind_install_default_bindings();
static void bind_clear(GpMouse::Bind * b);
static int lookup_key(char * ptr, int * len);
static int bind_scan_lhs(GpMouse::Bind * out, const char * in);
static char * bind_fmt_lhs(const GpMouse::Bind * in);
static int bind_matches(const GpMouse::Bind * a, const GpMouse::Bind * b);
static void bind_display_one(GpMouse::Bind * ptr);
static void bind_display(char* lhs);
//static void bind_all(char* lhs);
//static void bind_remove(GpMouse::Bind * b);
//static void bind_append(char * lhs, char * rhs, char *(*builtin)(GpEvent* ge));
//static void recalc_ruler_pos();
static void turn_ruler_off();
static void put_label(char * label, double x, double y);

/********* functions ********************************************/

/* produce a beep */
static void alert()
{
#ifdef HAVE_LIBREADLINE
	#if !defined(MISSING_RL_DING)
		rl_ding();
	#endif
	fflush(rl_outstream);
#else
	fprintf(stderr, "\a");
#endif
}
//
// main job of transformation, which is not device dependent
//
void GpGadgets::MousePosToGraphPosReal(int xx, int yy, double * x, double * y, double * x2, double * y2)
{
	if(!Is3DPlot) {
		if(PlotBounds.xright == PlotBounds.xleft)
			*x = *x2 = GPVL;  /* protection */
		else {
			*x  = MapBack(FIRST_X_AXIS, xx);
			*x2 = MapBack(SECOND_X_AXIS, xx);
		}
		if(PlotBounds.ytop == PlotBounds.ybot)
			*y = *y2 = GPVL;  /* protection */
		else {
			*y  = MapBack(FIRST_Y_AXIS, yy);
			*y2 = MapBack(SECOND_Y_AXIS, yy);
		}
		FPRINTF((stderr, "POS: xx=%i, yy=%i  =>  x=%g  y=%g\n", xx, yy, *x, *y));
	}
	else {
		/* for 3D plots, we treat the mouse position as if it is
		 * in the bottom plane, i.e., the plane of the x and y axis */
		/* note: at present, this projection is only correct if
		 * surface_rot_z is a multiple of 90 degrees! */
		/* HBB 20010522: added protection against division by zero
		 * for cases like 'set view 90,0' */
		xx -= axis3d_o_x;
		yy -= axis3d_o_y;
		if(abs(axis3d_x_dx) > abs(axis3d_x_dy)) {
			*x = AxA[FIRST_X_AXIS].Range.low + ((double)xx) / axis3d_x_dx * (AxA[FIRST_X_AXIS].GetRange());
		}
		else if(axis3d_x_dy != 0) {
			*x = AxA[FIRST_X_AXIS].Range.low + ((double)yy) / axis3d_x_dy * (AxA[FIRST_X_AXIS].GetRange());
		}
		else {
			// both diffs are zero (x axis points into the screen 
			*x = GPVL;
		}
		if(abs(axis3d_y_dx) > abs(axis3d_y_dy)) {
			*y = AxA[FIRST_Y_AXIS].Range.low + ((double)xx) / axis3d_y_dx * (AxA[FIRST_Y_AXIS].GetRange());
		}
		else if(axis3d_y_dy != 0) {
			if(splot_map)
				*y = AxA[FIRST_Y_AXIS].Range.upp + ((double)yy) / axis3d_y_dy * -AxA[FIRST_Y_AXIS].GetRange();
			else
				*y = AxA[FIRST_Y_AXIS].Range.low + ((double)yy) / axis3d_y_dy * AxA[FIRST_Y_AXIS].GetRange();
		}
		else {
			// both diffs are zero (y axis points into the screen 
			*y = GPVL;
		}
		*x2 = *y2 = GPVL; // protection 
	}
	/*
	   Note: there is PlotBounds.xleft+0.5 in "#define map_x" in graphics.c, which
	   makes no major impact here. It seems that the mistake of the real
	   GpCoordinate is at about 0.5%, which corresponds to the screen resolution.
	   It would be better to round the distance to this resolution, and thus
	   *x = xmin + rounded-to-screen-resolution (xdistance)
	 */

	// Now take into account possible log scales of x and y axes
	*x = DelogValue(FIRST_X_AXIS, *x);
	*y = DelogValue(FIRST_Y_AXIS, *y);
	if(!Is3DPlot) {
		*x2 = DelogValue(SECOND_X_AXIS, *x2);
		*y2 = DelogValue(SECOND_Y_AXIS, *y2);
	}
	// If x2 or y2 is linked to a primary axis via mapping function, apply it now
	if(!Is3DPlot) {
		GpAxis * secondary = &AxA[SECOND_X_AXIS];
		if(secondary->P_LinkToPrmr && secondary->link_udf->at)
			*x2 = secondary->EvalLinkFunction(*x);
		secondary = &AxA[SECOND_Y_AXIS];
		if(secondary->P_LinkToPrmr && secondary->link_udf->at)
			*y2 = secondary->EvalLinkFunction(*y);
	}
#ifdef NONLINEAR_AXES
	// If x or y is linked to a (hidden) primary axis, it's a bit more complicated
	if(!Is3DPlot) {
		GpAxis * secondary = &AxA[FIRST_X_AXIS];
		if(secondary->P_LinkToPrmr && secondary->P_LinkToPrmr->Index == -FIRST_X_AXIS) {
			*x = secondary->P_LinkToPrmr->MapBack(xx);
			*x = secondary->EvalLinkFunction(*x);
		}
		secondary = &AxA[FIRST_Y_AXIS];
		if(secondary->P_LinkToPrmr && secondary->P_LinkToPrmr->Index == -FIRST_Y_AXIS) {
			*y = secondary->P_LinkToPrmr->MapBack(yy);
			*y = secondary->EvalLinkFunction(*y);
		}
		secondary = &AxA[SECOND_X_AXIS];
		if(secondary->P_LinkToPrmr && secondary->P_LinkToPrmr->Index == -SECOND_X_AXIS) {
			*x2 = secondary->P_LinkToPrmr->MapBack(xx);
			*x2 = secondary->EvalLinkFunction(*x2);
		}
		secondary = &AxA[SECOND_Y_AXIS];
		if(secondary->P_LinkToPrmr && secondary->P_LinkToPrmr->Index == -SECOND_Y_AXIS) {
			*y2 = secondary->P_LinkToPrmr->MapBack(yy);
			*y2 = secondary->EvalLinkFunction(*y2);
		}
	}
#endif
}

static char * xy_format()
{
	static char format[64];
	format[0] = NUL;
	strncat(format, GpGg.Mse.Cfg.fmt, 30);
	strncat(format, ", ", 2);
	strncat(format, GpGg.Mse.Cfg.fmt, 30);
	return format;
}

static char * zoombox_format()
{
	static char format[64];
	format[0] = NUL;
	strncat(format, GpGg.Mse.Cfg.fmt, 30);
	strncat(format, "\r", 2);
	strncat(format, GpGg.Mse.Cfg.fmt, 30);
	return format;
}
//
// formats the information for an annotation (middle mouse button clicked)
//
//static char * GetAnnotateString(char * s, double x, double y, int mode, char * fmt)
char * GpGadgets::GetAnnotateString(char * s, double x, double y, int mode, char * fmt)
{
	if(AxA[FIRST_X_AXIS].datatype == DT_DMS || AxA[FIRST_Y_AXIS].datatype == DT_DMS) {
		static char dms_format[16];
		sprintf(dms_format, "%%D%s%%.2m'", degree_sign);
		if(AxA[FIRST_X_AXIS].datatype == DT_DMS)
			gstrdms(s, fmt ? fmt : dms_format, x);
		else
			sprintf(s, Mse.Cfg.fmt, x);
		strcat(s, ", ");
		s += strlen(s);
		if(AxA[FIRST_Y_AXIS].datatype == DT_DMS)
			gstrdms(s, fmt ? fmt : dms_format, y);
		else
			sprintf(s, Mse.Cfg.fmt, y);
		s += strlen(s);
	}
	else if(oneof4(mode, MOUSE_COORDINATES_XDATE, MOUSE_COORDINATES_XTIME, MOUSE_COORDINATES_XDATETIME, MOUSE_COORDINATES_TIMEFMT)) {
		/* time is on the x axis */
		char buf[0xff];
		char format[0xff] = "[%s, ";
		strcat(format, Mse.Cfg.fmt);
		strcat(format, "]");
		sprintf(s, format, xDateTimeFormat(x, buf, mode), y);
	}
	else if(mode == MOUSE_COORDINATES_FRACTIONAL) {
		double xrange = AxA[FIRST_X_AXIS].GetRange();
		double yrange = AxA[FIRST_Y_AXIS].GetRange();
		// calculate fractional coordinates. prevent division by zero
		if(xrange) {
			char format[0xff] = "/";
			strcat(format, Mse.Cfg.fmt);
			sprintf(s, format, (x - AxA[FIRST_X_AXIS].Range.low) / xrange);
		}
		else {
			sprintf(s, "/(undefined)");
		}
		s += strlen(s);
		if(yrange) {
			char format[0xff] = ", ";
			strcat(format, Mse.Cfg.fmt);
			strcat(format, "/");
			sprintf(s, format, (y - AxA[FIRST_Y_AXIS].Range.low) / yrange);
		}
		else {
			sprintf(s, ", (undefined)/");
		}
	}
	else if(mode == MOUSE_COORDINATES_REAL1) {
		sprintf(s, xy_format(), x, y); /* w/o brackets */
	}
	else if(mode == MOUSE_COORDINATES_ALT && (fmt || IsPolar)) {
		if(IsPolar) {
			double r;
			const double rmin = (GetR().AutoScale & AUTOSCALE_MIN) ? 0.0 : GetR().SetRange.low;
			const double phi = atan2(y, x);
			if(GetR().Flags & GpAxis::fLog)
				r = UndoLog(POLAR_AXIS, x/cos(phi) + DoLog(POLAR_AXIS, rmin));
			else
				r = x/cos(phi) + rmin;
			if(fmt)
				sprintf(s, fmt, phi/Ang2Rad, r);
			else {
				sprintf(s, "polar: ");
				s += strlen(s);
				sprintf(s, xy_format(), phi/Ang2Rad, r);
			}
		}
		else {
			sprintf(s, fmt, x, y); /* user defined format */
		}
	}
	else {
		sprintf(s, xy_format(), x, y); /* usual x,y values */
	}
	return s + strlen(s);
}

/* Format x according to the date/time mouse mode. Uses and returns b as
   a buffer
 */
static char * xDateTimeFormat(double x, char * b, int mode)
{
	struct tm tm;
	switch(mode) {
		case MOUSE_COORDINATES_XDATE:
		    ggmtime(&tm, x);
		    sprintf(b, "%d. %d. %04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year);
		    break;
		case MOUSE_COORDINATES_XTIME:
		    ggmtime(&tm, x);
		    sprintf(b, "%d:%02d", tm.tm_hour, tm.tm_min);
		    break;
		case MOUSE_COORDINATES_XDATETIME:
		    ggmtime(&tm, x);
		    sprintf(b, "%d. %d. %04d %d:%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year,
		    tm.tm_hour, tm.tm_min);
		    break;
		case MOUSE_COORDINATES_TIMEFMT:
		    // FIXME HBB 20000507: P_TimeFormat is for *reading* timedata, not for writing them!
		    gstrftime(b, 0xff, GpGg.P_TimeFormat, x);
		    break;
		default:
		    sprintf(b, GpGg.Mse.Cfg.fmt, x);
	}
	return b;
}

/* HBB 20000507: fixed a construction error. Was using the 'P_TimeFormat'
 * string (which is for reading, not writing time data) to output the
 * value. Code is now closer to what setup_tics does. */
#define MKSTR(sp, x, axis)					  \
	do {								\
		if(x >= GPVL) break;				   \
		if(GpGg[axis].datatype == DT_TIMEDATE) {		   \
			char * format = copy_or_invent_formatstring(&GpGg[axis]);	 \
			while(strchr(format, '\n'))				\
				*(strchr(format, '\n')) = ' ';			    \
			gstrftime(sp, 40, format, x);				\
		} else {						    \
			sprintf(sp, GpGg.Mse.Cfg.fmt, x);			\
		}							    \
		sp += strlen(sp);					    \
	} while(0)

// ratio for log, distance for linear
#define DIST(x, rx, axis) (GpGg[axis].Flags & GpAxis::fLog) ? ((rx==0) ? 99999 : x / rx) : (x - rx)

/* formats the GpGg.Mse.Ruler information (position, distance,...) into string p
        (it must be sufficiently long)
   x, y is the current mouse position in real coords (for the calculation
        of distance)
 */
//static void GetRulerString(char * p, double x, double y)
void GpGadgets::GetRulerString(char * p, double x, double y)
{
	double dx, dy;
	char format[0xff] = "  Mse.Ruler: [";
	strcat(format, Mse.Cfg.fmt);
	strcat(format, ", ");
	strcat(format, Mse.Cfg.fmt);
	strcat(format, "]  distance: ");
	strcat(format, Mse.Cfg.fmt);
	strcat(format, ", ");
	strcat(format, Mse.Cfg.fmt);

	dx = DIST(x, Mse.Ruler.P.x, FIRST_X_AXIS);
	dy = DIST(y, Mse.Ruler.P.y, FIRST_Y_AXIS);
	sprintf(p, format, Mse.Ruler.P.x, Mse.Ruler.P.y, dx, dy);

	/* Previously, the following "if" let the polar coordinates to be shown only
	   for lin-lin plots:
	        if(Mse.Cfg.polardistance && !AxA[FIRST_X_AXIS].log && !AxA[FIRST_Y_AXIS].log) ...
	   Now, let us support also semilog and log-log plots.
	   Values of Mse.Cfg.polardistance are:
	        0 (no polar coordinates), 1 (polar coordinates), 2 (tangent instead of angle).
	 */
	if(Mse.Cfg.polardistance) {
		double rho, phi, rx, ry;
		char ptmp[69];
		x  = LogValue(FIRST_X_AXIS, x);
		y  = LogValue(FIRST_Y_AXIS, y);
		rx = LogValue(FIRST_X_AXIS, Mse.Ruler.P.x);
		ry = LogValue(FIRST_Y_AXIS, Mse.Ruler.P.y);
		format[0] = '\0';
		strcat(format, " (");
		strcat(format, Mse.Cfg.fmt);
		rho = sqrt((x - rx) * (x - rx) + (y - ry) * (y - ry)); /* distance */
		if(Mse.Cfg.polardistance == 1) { /* (distance, angle) */
			phi = (180 / M_PI) * atan2(y - ry, x - rx);
			strcat(format, ", % #.4gdeg)");
		}
		else { /* Mse.Cfg.polardistance==2: (distance, tangent) */
			phi = x - rx;
			phi = (phi == 0) ? ((y-ry>0) ? GPVL : -GPVL) : (y - ry)/phi;
			sprintf(format+strlen(format), ", tangent=%s)", Mse.Cfg.fmt);
		}
		sprintf(ptmp, format, rho, phi);
		strcat(p, ptmp);
	}
}

static GpZoom * zoom_head = NULL;
static GpZoom * zoom_now = NULL;
static GpAxis * axis_array_copy = NULL;
//
// Applies the zoom rectangle of  z  by sending the appropriate command to gnuplot
//
//static void apply_zoom(GpZoom * z)
void GpGadgets::ApplyZoom(GpTermEntry * pT, GpZoom * pZ)
{
	char   s[1024];           // HBB 20011005: made larger 
	int    is_splot_map = (Is3DPlot && splot_map);
	if(zoom_now != NULL) {  // remember the current zoom 
		zoom_now->xRange  = AxA[FIRST_X_AXIS].SetRange;
		zoom_now->x2Range = AxA[SECOND_X_AXIS].SetRange;
		zoom_now->yRange  = AxA[FIRST_Y_AXIS].SetRange;
		zoom_now->y2Range = AxA[SECOND_Y_AXIS].SetRange;
	}
	// EAM DEBUG - The autoscale save/restore was too complicated, and
	// broke refresh. Just save the complete axis state and have done with it.
	if(zoom_now == zoom_head && pZ != zoom_head) {
		//axis_array_copy = (GpAxis *)gp_realloc(axis_array_copy, sizeof(AxArry), "AxArry copy");
		//memcpy(axis_array_copy, AxArry, sizeof(AxArry));
		DestroyAxesCopy(axis_array_copy);
		axis_array_copy = (GpAxis *)CreateAxesCopy();
	}
	//
	// If we are zooming, we don't want to autoscale the range.
	// This wasn't necessary before we introduced "refresh".  Why?
	//
	if(zoom_now == zoom_head && pZ != zoom_head) {
		AxA[FIRST_X_AXIS].AutoScale = AUTOSCALE_NONE;
		AxA[FIRST_Y_AXIS].AutoScale = AUTOSCALE_NONE;
		AxA[SECOND_X_AXIS].AutoScale = AUTOSCALE_NONE;
		AxA[SECOND_Y_AXIS].AutoScale = AUTOSCALE_NONE;
	}
	zoom_now = pZ;
	if(zoom_now == NULL) {
		alert();
	}
	else {
		// Now we're committed. Notify the terminal the the next replot is a zoom
		pT->_Layer(TERM_LAYER_BEFORE_ZOOM);
		sprintf(s, "set xr[%.12g:%.12g]; set yr[%.12g:%.12g]", zoom_now->xRange.low, zoom_now->xRange.upp, 
			zoom_now->yRange.low, zoom_now->yRange.upp);
		// EAM Apr 2013 - The tests on GPVL protect against trying to   
		// interpret the autoscaling initial state as an actual limit value. 
		if(!Is3DPlot && (zoom_now->x2Range.low < GPVL && zoom_now->x2Range.upp > -GPVL)) {
			sprintf(s + strlen(s), "; set x2r[% #g:% #g]", zoom_now->x2Range.low, zoom_now->x2Range.upp);
		}
		if(!Is3DPlot && (zoom_now->y2Range.low < GPVL && zoom_now->y2Range.upp > -GPVL)) {
			sprintf(s + strlen(s), "; set y2r[% #g:% #g]", zoom_now->y2Range.low, zoom_now->y2Range.upp);
		}
		//
		// EAM Jun 2007 - The autoscale save/restore was too complicated, and broke
		// refresh. Just save/restore the complete axis state and have done with it.
		// Well, not _quite_ the complete state.  The labels are maintained dynamically.
		// Apr 2015 - The same is now true (dynamic storage) for ticfmt, formatstring.
		//
		if(zoom_now == zoom_head) {
			for(int i = 0; i < AXIS_ARRAY_SIZE; i++) {
				const GpAxis & r_ax = AxA[i];
				axis_array_copy[i].label = r_ax.label;
				axis_array_copy[i].ticdef.def.user = r_ax.ticdef.def.user;
				axis_array_copy[i].ticdef.font = r_ax.ticdef.font;
				axis_array_copy[i].ticfmt = r_ax.ticfmt;
				axis_array_copy[i].formatstring = r_ax.formatstring;
			}
			//memcpy(AxArry, axis_array_copy, sizeof(AxArry));
			RestoreAxesCopy(axis_array_copy);
			s[0] = '\0'; // FIXME:  Is this better than calling ReplotRequest()? 
	#ifdef NONLINEAR_AXES
			// The shadowed primary axis, if any, is not restored by the memcpy.	
			// We choose to recalculate the limits, but alternatively we could find	
			// some place to save/restore the unzoomed limits.			
			if(AxA[FIRST_X_AXIS].P_LinkToPrmr)
				clone_linked_axes(&AxA[FIRST_X_AXIS], AxA[FIRST_X_AXIS].P_LinkToPrmr);
			if(AxA[FIRST_Y_AXIS].P_LinkToPrmr)
				clone_linked_axes(&AxA[FIRST_Y_AXIS], AxA[FIRST_Y_AXIS].P_LinkToPrmr);
	#endif
			// Falling through to do_string_replot() does not work! 
			if(IsVolatileData) {
				if(RefreshOk == E_REFRESH_OK_2D) {
					RefreshRequest();
					return;
				}
				if(is_splot_map && (RefreshOk == E_REFRESH_OK_3D)) {
					RefreshRequest();
					return;
				}
			}
		}
		else
			inside_zoom = true;
		DoStringReplot(GpGg.Gp__C, s);
		inside_zoom = false;
	}
}
//
// makes a zoom: update zoom history, call gnuplot to set ranges + replot
//
//static void do_zoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max)
void GpGadgets::DoZoom(GpTermEntry * pT, double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max)
{
	GpZoom * p_z;
	if(zoom_head == NULL) { /* queue not yet created, thus make its head */
		zoom_head = (GpZoom *)malloc(sizeof(GpZoom));
		zoom_head->prev = NULL;
		zoom_head->next = NULL;
	}
	SETIFZ(zoom_now, zoom_head);
	if(zoom_now->next == NULL) {    /* allocate new item */
		p_z = (GpZoom *)malloc(sizeof(GpZoom));
		p_z->prev = zoom_now;
		p_z->next = NULL;
		zoom_now->next = p_z;
		p_z->prev = zoom_now;
	}
	else                    /* overwrite next item */
		p_z = zoom_now->next;
	p_z->xRange.low = (AxA[FIRST_X_AXIS].Range.low  < GPVL) ? xmin  : AxA[FIRST_X_AXIS].Range.low;
	p_z->xRange.low = (AxA[FIRST_Y_AXIS].Range.low  < GPVL) ? ymin  : AxA[FIRST_Y_AXIS].Range.low;
	p_z->xRange.low = (AxA[SECOND_X_AXIS].Range.low < GPVL) ? x2min : AxA[SECOND_X_AXIS].Range.low;
	p_z->xRange.low = (AxA[SECOND_Y_AXIS].Range.low < GPVL) ? y2min : AxA[SECOND_Y_AXIS].Range.low;

	p_z->xRange.upp = (AxA[FIRST_X_AXIS].Range.upp  > -GPVL) ? xmax  : AxA[FIRST_X_AXIS].Range.upp;
	p_z->xRange.upp = (AxA[FIRST_Y_AXIS].Range.upp  > -GPVL) ? ymax  : AxA[FIRST_Y_AXIS].Range.upp;
	p_z->xRange.upp = (AxA[SECOND_X_AXIS].Range.upp > -GPVL) ? x2max : AxA[SECOND_X_AXIS].Range.upp;
	p_z->xRange.upp = (AxA[SECOND_Y_AXIS].Range.upp > -GPVL) ? y2max : AxA[SECOND_Y_AXIS].Range.upp;

	ApplyZoom(pT, p_z);
}

static void incr_mousemode(const int amount)
{
	long old = mouse_mode;
	mouse_mode += amount;
	if(MOUSE_COORDINATES_ALT == mouse_mode && !(mouse_alt_string || GpGg.IsPolar))
		mouse_mode += amount;  /* stepping over */
	if(mouse_mode > MOUSE_COORDINATES_ALT) {
		mouse_mode = MOUSE_COORDINATES_REAL1;
	}
	else if(mouse_mode <= MOUSE_COORDINATES_REAL) {
		mouse_mode = MOUSE_COORDINATES_ALT;
		if(!(mouse_alt_string || GpGg.IsPolar))
			mouse_mode--;  /* stepping over */
	}
	GpGg.UpdateStatusline(term);
	if(display_ipc_commands())
		fprintf(stderr, "switched mouse format from %ld to %ld\n", old, mouse_mode);
}

//#define TICS_ON(ti) (((ti)&TICS_MASK)!=NO_TICS)

//void UpdateStatusline()
void GpGadgets::UpdateStatusline(GpTermEntry * pT)
{
	UpdateStatuslineWithMouseSetting(pT, &Mse.Cfg);
}

//static void UpdateStatuslineWithMouseSetting(GpMouse::Settings * ms)
void GpGadgets::UpdateStatuslineWithMouseSetting(GpTermEntry * pT, GpMouse::Settings * ms)
{
	char s0[256], * sp;
/* This suppresses mouse GpCoordinate update after a ^C, but I think
 * that the relevant terminals do their own checks anyhow so we
 * we can just let the ones that care silently skip the update
 * while the ones that don't care keep on updating as usual.
 */
#if(0)
	if(!term_initialised)
		return;
#endif
	if(!ms->on) {
		s0[0] = 0;
	}
	else if(!ALMOST2D) {
		char format[0xff];
		format[0] = '\0';
		strcat(format, "view: ");
		strcat(format, ms->fmt);
		strcat(format, ", ");
		strcat(format, ms->fmt);
		strcat(format, "   scale: ");
		strcat(format, ms->fmt);
		strcat(format, ", ");
		strcat(format, ms->fmt);
		sprintf(s0, format, surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
	}
	else if(!AxA[SECOND_X_AXIS].TicsOn() && !AxA[SECOND_Y_AXIS].TicsOn()) {
		// only first X and Y axis are in use
		sp = GetAnnotateString(s0, Mse.RealP.x, Mse.RealP.y, mouse_mode, mouse_alt_string);
		if(Mse.Ruler.on) {
			GetRulerString(sp, Mse.RealP.x, Mse.RealP.y);
		}
	}
	else {
		// X2 and/or Y2 are in use: use more verbose format 
		sp = s0;
		if(AxA[FIRST_X_AXIS].TicsOn()) {
			sp = stpcpy(sp, "x=");
			MKSTR(sp, Mse.RealP.x, FIRST_X_AXIS);
			*sp++ = ' ';
		}
		if(AxA[FIRST_Y_AXIS].TicsOn()) {
			sp = stpcpy(sp, "y=");
			MKSTR(sp, Mse.RealP.y, FIRST_Y_AXIS);
			*sp++ = ' ';
		}
		if(AxA[SECOND_X_AXIS].TicsOn()) {
			sp = stpcpy(sp, "x2=");
			MKSTR(sp, Mse.RealP2.x, SECOND_X_AXIS);
			*sp++ = ' ';
		}
		if(AxA[SECOND_Y_AXIS].TicsOn()) {
			sp = stpcpy(sp, "y2=");
			MKSTR(sp, Mse.RealP2.y, SECOND_Y_AXIS);
			*sp++ = ' ';
		}
		if(Mse.Ruler.on) {
			// Mse.Ruler on? then also print distances to Mse.Ruler 
			if(AxA[FIRST_X_AXIS].TicsOn()) {
				stpcpy(sp, "dx=");
				sprintf(sp+3, Mse.Cfg.fmt, DIST(Mse.RealP.x, Mse.Ruler.P.x, FIRST_X_AXIS));
				sp += strlen(sp);
			}
			if(AxA[FIRST_Y_AXIS].TicsOn()) {
				stpcpy(sp, "dy=");
				sprintf(sp+3, Mse.Cfg.fmt, DIST(Mse.RealP.y, Mse.Ruler.P.y, FIRST_Y_AXIS));
				sp += strlen(sp);
			}
			if(AxA[SECOND_X_AXIS].TicsOn()) {
				stpcpy(sp, "dx2=");
				sprintf(sp+4, Mse.Cfg.fmt, DIST(Mse.RealP2.x, Mse.Ruler.P2.x, SECOND_X_AXIS));
				sp += strlen(sp);
			}
			if(AxA[SECOND_Y_AXIS].TicsOn()) {
				stpcpy(sp, "dy2=");
				sprintf(sp+4, Mse.Cfg.fmt, DIST(Mse.RealP2.y, Mse.Ruler.P2.y, SECOND_Y_AXIS));
				sp += strlen(sp);
			}
		}
		*--sp = 0;      /* delete trailing space */
	}
	if(pT->put_tmptext) {
		(pT->put_tmptext)(0, s0);
	}
}

#undef MKSTR

//void recalc_statusline()
void GpGadgets::RecalcStatusLine()
{
	MousePosToGraphPosReal(Mse.MP.x, Mse.MP.y, &Mse.RealP.x, &Mse.RealP.y, &Mse.RealP2.x, &Mse.RealP2.y);
	UpdateStatusline(term);
}

/****************** handlers for user's actions ******************/

static char * builtin_autoscale(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-autoscale` (set autoscale keepfix; replot)";
	}
	else {
		GpGg.DoStringReplot(GpGg.Gp__C, "set autoscale keepfix");
		return (char*)0;
	}
}

static char * builtin_toggle_border(GpEvent * ge)
{
	if(!ge)
		return "`builtin-toggle-border`";

	/* EAM July 2009  Cycle through border settings
	 * - no border
	 * - last border requested by the user
	 * - default border
	 * - (3D only) full border
	 */
	if(GpGg.DrawBorder == 0 && GpGg.DrawBorder != GpGg.UserBorder)
		GpGg.DrawBorder = GpGg.UserBorder;
	else if(GpGg.DrawBorder == GpGg.UserBorder && GpGg.DrawBorder != 31)
		GpGg.DrawBorder = 31;
	else if(GpGg.Is3DPlot && GpGg.DrawBorder == 31)
		GpGg.DrawBorder = 4095;
	else
		GpGg.DrawBorder = 0;
	GpGg.DoStringReplot(GpGg.Gp__C, "");
	return (char*)0;
}

static char * builtin_replot(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-replot`";
	}
	else {
		GpGg.DoStringReplot(GpGg.Gp__C, "");
		return (char*)0;
	}
}

static char * builtin_toggle_grid(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-toggle-grid`";
	}
	else {
		if(!GpGg.SomeGridSelected())
			GpGg.DoStringReplot(GpGg.Gp__C, "set grid");
		else
			GpGg.DoStringReplot(GpGg.Gp__C, "unset grid");
		return (char*)0;
	}
}

static char * builtin_help(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-help`";
	}
	else {
		fprintf(stderr, "\n");
		bind_display((char*)0); /* display all GpGg.Mse.P_Bindings */
		restore_prompt();
		return (char*)0;
	}
}

static char * builtin_set_plots_visible(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-set-plots-visible`";
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_SET_VISIBLE, -1);
	return (char*)0;
}

static char * builtin_set_plots_invisible(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-set-plots-invisible`";
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_SET_INVISIBLE, -1);
	return (char*)0;
}

static char * builtin_invert_plot_visibilities(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-invert-plot-visibilities`";
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_INVERT_VISIBILITIES, -1);
	return (char*)0;
}

static char * builtin_toggle_log(GpEvent * ge)
{
	return GpGg.BuiltInToggleLog(GpGg.Gp__C, ge);
}

char * GpGadgets::BuiltInToggleLog(GpCommand & rC, GpEvent * ge)
{
	if(!ge)
		return "`builtin-toggle-log` y logscale for plots, z and cb for splots";
	else {
		if(IsVolatileData)
			IntWarn(NO_CARET, "Cannot toggle log scale for volatile data");
		else if((ColorBox.bounds.xleft < GpGg.Mse.MP.x && GpGg.Mse.MP.x < ColorBox.bounds.xright) && 
			(ColorBox.bounds.ybot < GpGg.Mse.MP.y && GpGg.Mse.MP.y < ColorBox.bounds.ytop))
			DoStringReplot(rC, (GetCB().Flags & GpAxis::fLog) ? "unset log cb" : "set log cb");
		else if(Is3DPlot && !splot_map)
			DoStringReplot(rC, (GetZ().Flags & GpAxis::fLog) ? "unset log z" : "set log z");
		else
			DoStringReplot(rC, (AxA[FIRST_Y_AXIS].Flags & GpAxis::fLog) ? "unset log y" : "set log y");
		return 0;
	}
}

static char * builtin_nearest_log(GpEvent * ge)
{
	return GpGg.BuiltInNearestLog(GpGg.Gp__C, ge);
}

char * GpGadgets::BuiltInNearestLog(GpCommand & rC, GpEvent * ge)
{
	if(!ge)
		return "`builtin-nearest-log` toggle logscale of axis nearest cursor";
	if((ColorBox.bounds.xleft < Mse.MP.x && Mse.MP.x < ColorBox.bounds.xright) && 
		(ColorBox.bounds.ybot < Mse.MP.y && Mse.MP.y < ColorBox.bounds.ytop)) {
		DoStringReplot(rC, (GetCB().Flags & GpAxis::fLog) ? "unset log cb" : "set log cb");
	}
	else if(Is3DPlot && !splot_map) {
		DoStringReplot(rC, (GetZ().Flags & GpAxis::fLog) ? "unset log z" : "set log z");
	}
	else {
		/* 2D-plot: figure out which axis/axes is/are
		 * close to the mouse cursor, and toggle those lin/log */
		/* note: here it is assumed that the x axis is at
		 * the bottom, x2 at top, y left and y2 right; it
		 * would be better to derive that from the ..tics settings */
		bool change_x1 = false;
		bool change_y1 = false;
		bool change_x2 = false;
		bool change_y2 = false;
		if(Mse.MP.y < PlotBounds.ybot + (PlotBounds.ytop - PlotBounds.ybot) / 4 && Mse.MP.x > PlotBounds.xleft && Mse.MP.x < PlotBounds.xright)
			change_x1 = true;
		if(Mse.MP.x < PlotBounds.xleft + (PlotBounds.xright - PlotBounds.xleft) / 4 && Mse.MP.y > PlotBounds.ybot && Mse.MP.y < PlotBounds.ytop)
			change_y1 = true;
		if(Mse.MP.y > PlotBounds.ytop - (PlotBounds.ytop - PlotBounds.ybot) / 4 && Mse.MP.x > PlotBounds.xleft && Mse.MP.x < PlotBounds.xright)
			change_x2 = true;
		if(Mse.MP.x > PlotBounds.xright - (PlotBounds.xright - PlotBounds.xleft) / 4 && Mse.MP.y > PlotBounds.ybot && Mse.MP.y < PlotBounds.ytop)
			change_y2 = true;
		if(change_x1)
			rC.DoString((AxA[FIRST_X_AXIS].Flags & GpAxis::fLog) ? "unset log x" : "set log x");
		if(change_y1)
			rC.DoString((AxA[FIRST_Y_AXIS].Flags & GpAxis::fLog) ? "unset log y" : "set log y");
		if(change_x2 && !splot_map)
			rC.DoString((AxA[SECOND_X_AXIS].Flags & GpAxis::fLog) ? "unset log x2" : "set log x2");
		if(change_y2 && !splot_map)
			rC.DoString((AxA[SECOND_Y_AXIS].Flags & GpAxis::fLog) ? "unset log y2" : "set log y2");
		if(!change_x1 && !change_y1 && splot_map)
			DoStringReplot(rC, (GetZ().Flags & GpAxis::fLog) ? "unset log z" : "set log z");
		if(change_x1 || change_y1 || change_x2 || change_y2)
			DoStringReplot(rC, "");
	}
	return (char*)0;
}

static char * builtin_toggle_mouse(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-toggle-mouse`";
	}
	if(!GpGg.Mse.Cfg.on) {
		GpGg.Mse.Cfg.on = 1;
		if(display_ipc_commands()) {
			fprintf(stderr, "turning mouse on.\n");
		}
	}
	else {
		GpGg.Mse.Cfg.on = 0;
		if(display_ipc_commands()) {
			fprintf(stderr, "turning mouse off.\n");
		}
	}
	GpGg.UpdateStatusline(term);
	return (char*)0;
}

static char * builtin_toggle_ruler(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-toggle-ruler`";
	}
	if(!term->set_ruler)
		return (char*)0;
	if(GpGg.Mse.Ruler.on) {
		turn_ruler_off();
		if(display_ipc_commands())
			fprintf(stderr, "turning ruler off.\n");
	}
	else if(ALMOST2D) {
		// only allow GpGg.Mse.Ruler, if the plot is 2d or a 3d `map'
		UdvtEntry * u;
		GpGg.Mse.Ruler.on = true;
		GpGg.Mse.Ruler.px = ge->mx;
		GpGg.Mse.Ruler.py = ge->my;
		GpGg.MousePosToGraphPosReal(GpGg.Mse.Ruler.px, GpGg.Mse.Ruler.py, &GpGg.Mse.Ruler.P.x, &GpGg.Mse.Ruler.P.y, &GpGg.Mse.Ruler.P2.x, &GpGg.Mse.Ruler.P2.y);
		(*term->set_ruler)(GpGg.Mse.Ruler.px, GpGg.Mse.Ruler.py);
		if((u = GpGg.Ev.AddUdvByName("MOUSE_RULER_X"))) {
			u->udv_value.SetComplex(GpGg.Mse.Ruler.P.x, 0);
		}
		if((u = GpGg.Ev.AddUdvByName("MOUSE_RULER_Y"))) {
			u->udv_value.SetComplex(GpGg.Mse.Ruler.P.y, 0);
		}
		if(display_ipc_commands()) {
			fprintf(stderr, "turning ruler on.\n");
		}
	}
	GpGg.UpdateStatusline(term);
	return (char*)0;
}

static char * builtin_decrement_mousemode(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-previous-mouse-format`";
	}
	incr_mousemode(-1);
	return (char*)0;
}

static char * builtin_increment_mousemode(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-next-mouse-format`";
	}
	incr_mousemode(1);
	return (char*)0;
}

static char * builtin_toggle_polardistance(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-toggle-polardistance`";
	}
	if(++GpGg.Mse.Cfg.polardistance > 2) GpGg.Mse.Cfg.polardistance = 0;
	/* values: 0 (no polar coordinates), 1 (polar coordinates), 2 (tangent instead of angle) */
	term->set_cursor((GpGg.Mse.Cfg.polardistance ? -3 : -4), ge->mx, ge->my); /* change cursor type */
	GpGg.UpdateStatusline(term);
	if(display_ipc_commands()) {
		fprintf(stderr, "distance to GpGg.Mse.Ruler will %s be shown in polar coordinates.\n", GpGg.Mse.Cfg.polardistance ? "" : "not");
	}
	return (char*)0;
}

static char * builtin_toggle_verbose(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-toggle-verbose`";
	}
	/* this is tricky as the command itself modifies
	 * the state of display_ipc_commands() */
	if(display_ipc_commands()) {
		fprintf(stderr, "echoing of communication commands is turned off.\n");
	}
	toggle_display_of_ipc_commands();
	if(display_ipc_commands()) {
		fprintf(stderr, "communication commands will be echoed.\n");
	}
	return (char*)0;
}

static char * builtin_toggle_ratio(GpEvent * ge)
{
	if(!ge) {
		return "`builtin-toggle-ratio`";
	}
	if(GpGg.AspectRatio == 0)
		GpGg.DoStringReplot(GpGg.Gp__C, "set size ratio -1");
	else if(GpGg.AspectRatio == 1)
		GpGg.DoStringReplot(GpGg.Gp__C, "set size nosquare");
	else
		GpGg.DoStringReplot(GpGg.Gp__C, "set size square");
	return (char*)0;
}

static char * builtin_zoom_next(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-next` go to next zoom in the zoom stack";
	else {
		//ZoomNext();
		//static void ZoomNext()
		{
			if(zoom_now == NULL || zoom_now->next == NULL)
				alert();
			else
				GpGg.ApplyZoom(term, zoom_now->next);
			if(display_ipc_commands()) {
				fprintf(stderr, "next zoom.\n");
			}
		}
		return (char*)0;
	}
}

static char * builtin_zoom_previous(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-previous` go to previous zoom in the zoom stack";
	else {
		//ZoomPrevious();
		//static void ZoomPrevious()
		{
			if(zoom_now == NULL || zoom_now->prev == NULL)
				alert();
			else
				GpGg.ApplyZoom(term, zoom_now->prev);
			if(display_ipc_commands()) {
				fprintf(stderr, "previous zoom.\n");
			}
		}
		return (char*)0;
	}
}

static char * builtin_unzoom(GpEvent * ge)
{
	if(!ge)
		return "`builtin-unzoom`";
	else {
		//ZoomUnzoom();
		//static void ZoomUnzoom()
		{
			if(zoom_head == NULL || zoom_now == zoom_head)
				alert();
			else
				GpGg.ApplyZoom(term, zoom_head);
			if(display_ipc_commands()) {
				fprintf(stderr, "unzoom.\n");
			}
		}
		return (char*)0;
	}
}

static char * builtin_rotate_right(GpEvent * ge)
{
	if(!ge)
		return "`scroll right in 2d, rotate right in 3d`; <Shift> faster";
	if(GpGg.Is3DPlot)
		GpGg.ChangeView(term, GpGg.Gp__C, 0, -1);
	else {
		int k = (GpGg.Mse.modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_right();
	}
	return (char*)0;
}

static char * builtin_rotate_left(GpEvent * ge)
{
	if(!ge)
		return "`scroll left in 2d, rotate left in 3d`; <Shift> faster";
	if(GpGg.Is3DPlot)
		GpGg.ChangeView(term, GpGg.Gp__C, 0, 1);
	else {
		int k = (GpGg.Mse.modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_left();
	}
	return (char*)0;
}

static char * builtin_rotate_up(GpEvent * ge)
{
	if(!ge)
		return "`scroll up in 2d, rotate up in 3d`; <Shift> faster";
	if(GpGg.Is3DPlot)
		GpGg.ChangeView(term, GpGg.Gp__C, 1, 0);
	else {
		int k = (GpGg.Mse.modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_up();
	}
	return (char*)0;
}

static char * builtin_rotate_down(GpEvent * ge)
{
	if(!ge)
		return "`scroll down in 2d, rotate down in 3d`; <Shift> faster";
	if(GpGg.Is3DPlot)
		GpGg.ChangeView(term, GpGg.Gp__C, -1, 0);
	else {
		int k = (GpGg.Mse.modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_down();
	}
	return (char*)0;
}

static char * builtin_cancel_zoom(GpEvent * ge)
{
	if(!ge)
		return "`builtin-cancel-zoom` cancel zoom region";
	else {
		if(GpGg.Mse.setting_zoom_region) {
			if(term->set_cursor)
				term->set_cursor(0, 0, 0);
			GpGg.Mse.setting_zoom_region = false;
			if(display_ipc_commands())
				fprintf(stderr, "zooming cancelled.\n");
		}
		return (char*)0;
	}
}

//static void event_keypress(GpEvent * ge, bool current)
void GpGadgets::EventKeyPress(GpTermEntry * pT, GpCommand & rC, GpEvent * pGe, bool current)
{
	GpMouse::Bind * ptr;
	GpMouse::Bind keypress;
	int    c = pGe->par1;
	int    par2 = pGe->par2;
	int    x = pGe->mx;
	int    y = pGe->my;
	if(!Mse.P_Bindings) {
		Mse.BindInstallDefaultBindings();
	}
	if((Mse.modifier_mask & Mod_Shift) && ((c & 0xff) == 0)) {
		c = toupper(c);
	}
	bind_clear(&keypress);
	keypress.key = c;
	keypress.modifier = Mse.modifier_mask;
	/*
	 * On 'pause mouse keypress' in active window export current keypress
	 * and mouse coords to user variables. A key with 'bind all' terminates
	 * a pause even from non-active windows.
	 * Ignore NULL keypress.
	 *
	 * If we are paused for a keystroke, this takes precendence over normal
	 * key Mse.P_Bindings. Otherwise, for example typing 'm' would turn off mousing,
	 * which is a bad thing if you are in the  middle of a mousing operation.
	 */
	if((paused_for_mouse & PAUSE_KEYSTROKE) && (c > '\0') && current) {
		LoadMouseVariables(x, y, false, c);
	}
	else {
		for(ptr = Mse.P_Bindings; ptr; ptr = ptr->next) {
			if(bind_matches(&keypress, ptr)) {
				UdvtEntry * keywin;
				if((keywin = Ev.AddUdvByName("MOUSE_KEY_WINDOW"))) {
					keywin->udv_value.SetInt(pGe->winid);
				}
				/* Always honor keys set with "bind all" */
				if(ptr->allwindows && ptr->command) {
					if(current)
						LoadMouseVariables(x, y, false, c);
					else
						/* FIXME - Better to clear MOUSE_[XY] than to set it wrongly. */
						/*         This may be worth a separate subroutine.           */
						LoadMouseVariables(0, 0, false, c);
					rC.DoString(ptr->command);
					// Treat as a current event after we return to x11.trm 
					pGe->type = GE_keypress;
					break;
					// But otherwise ignore inactive windows 
				}
				else if(!current) {
					break;
					/* Let user defined Mse.P_Bindings overwrite the builtin Mse.P_Bindings */
				}
				else if((par2 & 1) == 0 && ptr->command) {
					rC.DoString(ptr->command);
					break;
				}
				else if(ptr->builtin) {
					ptr->builtin(pGe);
				}
				else {
					fprintf(stderr, "%s:%d protocol error\n", __FILE__, __LINE__);
				}
			}
		}
	}
}

//static void ChangeView(int x, int z)
void GpGadgets::ChangeView(GpTermEntry * pT, GpCommand & rC, int x, int z)
{
	if(Mse.modifier_mask & Mod_Shift) {
		x *= 10;
		z *= 10;
	}
	if(x) {
		surface_rot_x += x;
		if(surface_rot_x < 0)
			surface_rot_x += 360;
		if(surface_rot_x > 360)
			surface_rot_x -= 360;
	}
	if(z) {
		surface_rot_z += z;
		if(surface_rot_z < 0)
			surface_rot_z += 360;
		if(surface_rot_z > 360)
			surface_rot_z -= 360;
	}
	if(x || z) {
		Ev.FillGpValFloat("GPVAL_VIEW_ROT_X", surface_rot_x);
		Ev.FillGpValFloat("GPVAL_VIEW_ROT_Z", surface_rot_z);
	}
	if(display_ipc_commands()) {
		fprintf(stderr, "changing view to %f, %f.\n", surface_rot_x, surface_rot_z);
	}
	DoSave3DPlot(rC, P_First3DPlot, plot3d_num, 0 /* not quick */);
	if(ALMOST2D) {
		// 2D plot, or suitably aligned 3D plot: update statusline
		if(!pT->put_tmptext)
			return;
		RecalcStatusLine();
	}
}

int is_mouse_outside_plot(void)
{
	// Here I look at both min/max each time because reversed ranges can make
	// min > max
#define CHECK_AXIS_OUTSIDE(real, axis)					\
	(GpGg[axis].Range.low < GPVL && GpGg[axis].Range.upp > -GPVL && \
	    ((real < GpGg.DelogValue(axis, GpGg[axis].Range.low) && real < GpGg.DelogValue(axis, GpGg[axis].Range.upp)) || \
		    (real > GpGg.DelogValue(axis, GpGg[axis].Range.low) && real > GpGg.DelogValue(axis, GpGg[axis].Range.upp))))

	return
		CHECK_AXIS_OUTSIDE(GpGg.Mse.RealP.x,  FIRST_X_AXIS)  ||
		CHECK_AXIS_OUTSIDE(GpGg.Mse.RealP.y,  FIRST_Y_AXIS)  ||
		CHECK_AXIS_OUTSIDE(GpGg.Mse.RealP2.x, SECOND_X_AXIS) ||
		CHECK_AXIS_OUTSIDE(GpGg.Mse.RealP2.y, SECOND_Y_AXIS);

#undef CHECK_AXIS_OUTSIDE
}
//
// Return a new (upper or lower) axis limit that is a linear
// combination of the current limits
//
//static double rescale(int axIdx, double w1, double w2)
double GpGadgets::Rescale(int axIdx, double w1, double w2) const
{
	const GpAxis & r_ax = AxA[axIdx];
	const double logval = w1 * r_ax.Range.low + w2 * r_ax.Range.upp;
	return DelogValue((AXIS_INDEX)axIdx, logval);
}
//
// Rescale axes and do zoom
//
/*static void zoom_rescale_xyx2y2(double a0, double a1, double a2, double a3, double a4, double a5, double a6,
    double a7, double a8, double a9, double a10, double a11, double a12, double a13, double a14, double a15,
    char msg[])*/
void GpGadgets::ZoomRescaleXYX2Y2(GpTermEntry * pT, double a0, double a1, double a2, double a3, double a4, double a5, double a6,
    double a7, double a8, double a9, double a10, double a11, double a12, double a13, double a14, double a15, char msg[])
{
	double xmin  = Rescale(FIRST_X_AXIS,   a0, a1);
	double ymin  = Rescale(FIRST_Y_AXIS,   a2, a3);
	double x2min = Rescale(SECOND_X_AXIS,  a4, a5);
	double y2min = Rescale(SECOND_Y_AXIS,  a6, a7);
	double xmax  = Rescale(FIRST_X_AXIS,   a8, a9);
	double ymax  = Rescale(FIRST_Y_AXIS,  a10, a11);
	double x2max = Rescale(SECOND_X_AXIS, a12, a13);
	double y2max = Rescale(SECOND_Y_AXIS, a14, a15);
	DoZoom(pT, xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	if(msg[0] && display_ipc_commands()) {
		fputs(msg, stderr); fputs("\n", stderr);
	}
}

/* Scroll left. */
static void do_zoom_scroll_left()
{
	GpGg.ZoomRescaleXYX2Y2(term, 
		1.1, -0.1,
	    1,   0,
	    1.1, -0.1,
	    1,   0,
	    0.1, 0.9,
	    0,   1,
	    0.1, 0.9,
	    0,   1,
	    "scroll left.\n");
}

/* Scroll right. */
static void do_zoom_scroll_right()
{
	GpGg.ZoomRescaleXYX2Y2(term, 
		0.9,  0.1,
	    1,    0,
	    0.9,  0.1,
	    1,    0,
	    -0.1, 1.1,
	    0,    1,
	    -0.1, 1.1,
	    0,    1,
	    "scroll right");
}

/* Scroll up. */
static void do_zoom_scroll_up()
{
	GpGg.ZoomRescaleXYX2Y2(term, 
		1,    0,
	    0.9,  0.1,
	    1,    0,
	    0.9,  0.1,
	    0,    1,
	    -0.1, 1.1,
	    0,    1,
	    -0.1, 1.1,
	    "scroll up");
}

/* Scroll down. */
static void do_zoom_scroll_down()
{
	GpGg.ZoomRescaleXYX2Y2(term,
		1,   0,
	    1.1, -0.1,
	    1,   0,
	    1.1, -0.1,
	    0,   1,
	    0.1, 0.9,
	    0,   1,
	    0.1, 0.9,
	    "scroll down");
}
//
// Return new lower and upper axis limits as current limits resized
// around current mouse position
//
//static void rescale_around_mouse(double * newmin, double * newmax, int axIdx, double mouse_pos, double scale)
void GpGadgets::RescaleAroundMouse(double * newmin, double * newmax, int axIdx, double mouse_pos, double scale) const
{
	const double unlog_pos = LogValue((AXIS_INDEX)axIdx, mouse_pos);
	*newmin = unlog_pos + (AxA[axIdx].Range.low - unlog_pos) * scale;
	*newmin = DelogValue((AXIS_INDEX)axIdx, *newmin);
	*newmax = unlog_pos + (AxA[axIdx].Range.upp - unlog_pos) * scale;
	*newmax = DelogValue((AXIS_INDEX)axIdx, *newmax);
}
//
// Zoom in/out within x-axis.
//
//static void zoom_in_X(int zoom_key)
void GpGadgets::ZoomInX(GpTermEntry * pT, int zoom_key)
{
	// I don't check for "outside" here. will do that later
	if(is_mouse_outside_plot()) {
		// zoom in (X axis only) 
		double w1 = (zoom_key=='+') ? 23./25. : 23./21.;
		double w2 = (zoom_key=='+') ?  2./25. : -2./21.;
		ZoomRescaleXYX2Y2(pT, w1, w2, 1, 0, w1, w2, 1, 0,  w2, w1, 0, 1, w2, w1, 0, 1, (zoom_key=='+' ? "zoom in X" : "zoom out X"));
	}
	else {
		double xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max;
		double scale = (zoom_key=='+') ? 0.75 : 1.25;
		RescaleAroundMouse(&xmin,  &xmax,  FIRST_X_AXIS,  GpGg.Mse.RealP.x,  scale);
		RescaleAroundMouse(&x2min, &x2max, SECOND_X_AXIS, GpGg.Mse.RealP2.x, scale);
		ymin  = Rescale(FIRST_Y_AXIS,  1, 0);
		y2min = Rescale(SECOND_Y_AXIS, 1, 0);
		ymax  = Rescale(FIRST_Y_AXIS,  0, 1);
		y2max = Rescale(SECOND_Y_AXIS, 0, 1);
		DoZoom(pT, xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	}
}
//
// Zoom around mouse cursor unless the cursor is outside the graph boundary,
// when it scales around the graph center.
// Syntax: zoom_key == '+' ... zoom in, zoom_key == '-' ... zoom out
//
//static void zoom_around_mouse(int zoom_key)
void GpGadgets::ZoomAroundMouse(GpTermEntry * pT, int zoom_key)
{
	double xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max;
	if(is_mouse_outside_plot()) {
		// zoom in (factor of approximately 2^(.25), so four steps gives 2x larger) 
		double w1 = (zoom_key=='+') ? 23./25. : 23./21.;
		double w2 = (zoom_key=='+') ?  2./25. : -2./21.;
		xmin  = Rescale(FIRST_X_AXIS,  w1, w2);
		ymin  = Rescale(FIRST_Y_AXIS,  w1, w2);
		x2min = Rescale(SECOND_X_AXIS, w1, w2);
		y2min = Rescale(SECOND_Y_AXIS, w1, w2);

		xmax  = Rescale(FIRST_X_AXIS,  w2, w1);
		ymax  = Rescale(FIRST_Y_AXIS,  w2, w1);
		x2max = Rescale(SECOND_X_AXIS, w2, w1);
		y2max = Rescale(SECOND_Y_AXIS, w2, w1);
	}
	else {
		int zsign = (zoom_key=='+') ? -1 : 1;
		double xscale = pow(1.25, zsign * Mse.Cfg.xmzoom_factor);
		double yscale = pow(1.25, zsign * Mse.Cfg.ymzoom_factor);
		// {x,y}zoom_factor = 0: not zoom, = 1: 0.8/1.25 zoom 
		RescaleAroundMouse(&xmin,  &xmax,  FIRST_X_AXIS,  Mse.RealP.x,  xscale);
		RescaleAroundMouse(&ymin,  &ymax,  FIRST_Y_AXIS,  Mse.RealP.y,  yscale);
		RescaleAroundMouse(&x2min, &x2max, SECOND_X_AXIS, Mse.RealP2.x, xscale);
		RescaleAroundMouse(&y2min, &y2max, SECOND_Y_AXIS, Mse.RealP2.y, yscale);
	}
	DoZoom(pT, xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	if(display_ipc_commands())
		fprintf(stderr, "zoom %s.\n", (zoom_key=='+' ? "in" : "out"));
}

static void do_zoom_in_X()
{
	GpGg.ZoomInX(term, '+');
}

static void do_zoom_out_X()
{
	GpGg.ZoomInX(term, '-');
}

static void do_zoom_in_around_mouse()
{
	GpGg.ZoomAroundMouse(term, '+');
}

static void do_zoom_out_around_mouse()
{
	GpGg.ZoomAroundMouse(term, '-');
}

static char * builtin_zoom_in_around_mouse(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-in` zoom in";
	do_zoom_in_around_mouse();
	return (char*)0;
}

static char * builtin_zoom_out_around_mouse(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-out` zoom out";
	do_zoom_out_around_mouse();
	return (char*)0;
}

#if(0) /* Not currently used */
static char * builtin_zoom_scroll_left(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-left` scroll left";
	do_zoom_scroll_left();
	return (char*)0;
}

static char * builtin_zoom_scroll_right(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-right` scroll right";
	do_zoom_scroll_right();
	return (char*)0;
}

static char * builtin_zoom_scroll_up(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-up` scroll up";
	do_zoom_scroll_up();
	return (char*)0;
}

static char * builtin_zoom_scroll_down(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-down` scroll down";
	do_zoom_scroll_down();
	return (char*)0;
}

static char * builtin_zoom_in_X(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-in-X` zoom in X axis";
	do_zoom_in_X();
	return (char*)0;
}

static char * builtin_zoom_out_X(GpEvent * ge)
{
	if(!ge)
		return "`builtin-zoom-out-X` zoom out X axis";
	do_zoom_out_X();
	return (char*)0;
}

#endif /* Not currently used */

//static void event_buttonpress(GpEvent * ge)
void GpGadgets::EventButtonPress(GpTermEntry * pT, GpCommand & rC, GpEvent * pGe)
{
	int    b;
	Mse.motion = 0;
	b = pGe->par1;
	Mse.MP.x = pGe->mx;
	Mse.MP.y = pGe->my;
	Mse.button |= (1 << b);
	FPRINTF((stderr, "(event_buttonpress) mouse_x = %d\tmouse_y = %d\n", mouse_x, mouse_y));
	MousePosToGraphPosReal(Mse.MP.x, Mse.MP.y, &Mse.RealP.x, &Mse.RealP.y, &Mse.RealP2.x, &Mse.RealP2.y);
	if((b == 4 || b == 6) && /* 4 - wheel up, 6 - wheel left */ (!rC.IsReplotDisabled || (E_REFRESH_NOT_OK != RefreshOk)) && !(paused_for_mouse & PAUSE_BUTTON3)) {
		// Ctrl+Shift+wheel up or Squeeze (not implemented)
		if((Mse.modifier_mask & Mod_Ctrl) && (Mse.modifier_mask & Mod_Shift))
			do_zoom_in_X();
		// Ctrl+wheel up or Ctrl+stroke
		else if((Mse.modifier_mask & Mod_Ctrl))
			do_zoom_in_around_mouse();
		// Horizontal stroke (button 6) or Shift+wheel up
		else if(b == 6 || (Mse.modifier_mask & Mod_Shift))
			do_zoom_scroll_left();
		// Wheel up (no modifier keys)
		else
			do_zoom_scroll_up();
	}
	else if(((b == 5) || (b == 7)) && /* 5 - wheel down, 7 - wheel right */ (!rC.IsReplotDisabled || (E_REFRESH_NOT_OK != RefreshOk)) && !(paused_for_mouse & PAUSE_BUTTON3)) {
		// Ctrl+Shift+wheel down or Unsqueeze (not implemented)
		if((Mse.modifier_mask & Mod_Ctrl) && (Mse.modifier_mask & Mod_Shift))
			do_zoom_out_X();
		/* Ctrl+wheel down or Ctrl+stroke */
		else if((Mse.modifier_mask & Mod_Ctrl))
			do_zoom_out_around_mouse();
		/* Horizontal stroke (button 7) or Shift+wheel down */
		else if(b == 7 || (Mse.modifier_mask & Mod_Shift))
			do_zoom_scroll_right();
		/* Wheel down (no modifier keys) */
		else
			do_zoom_scroll_down();
	}
	else if(ALMOST2D) {
		if(!Mse.setting_zoom_region) {
			if(1 == b) {
				/* "pause button1" or "pause any" takes precedence over key Mse.P_Bindings */
				if(paused_for_mouse & PAUSE_BUTTON1) {
					LoadMouseVariables(Mse.MP.x, Mse.MP.y, true, b);
					Mse.TrapRelease = true; /* Don't trigger on release also */
					return;
				}
			}
			else if(2 == b) {
				/* not bound in 2d graphs */
			}
			else if(3 == b && (!rC.IsReplotDisabled || (E_REFRESH_NOT_OK != RefreshOk)) && !(paused_for_mouse & PAUSE_BUTTON3)) {
				/* start zoom; but ignore it when
				 *   - replot is disabled, e.g. with inline data, or
				 *   - during 'pause mouse'
				 * allow zooming during 'pause mouse key' */
				Mse.SettingZoomP = Mse.MP;
				Mse.setting_zoom_region = true;
				if(pT->set_cursor) {
					int mv_mouse_x, mv_mouse_y;
					if(Mse.Cfg.annotate_zoom_box && pT->put_tmptext) {
						double real_x, real_y, real_x2, real_y2;
						char s[64];
						/* tell driver annotations */
						MousePosToGraphPosReal(Mse.MP.x, Mse.MP.y, &real_x, &real_y, &real_x2, &real_y2);
						sprintf(s, zoombox_format(), real_x, real_y);
						pT->put_tmptext(1, s);
						pT->put_tmptext(2, s);
					}
					/* displace mouse in order not to start with an empty zoom box */
					mv_mouse_x = pT->xmax / 20;
					mv_mouse_y = (pT->xmax == pT->ymax) ? mv_mouse_x : (int)((mv_mouse_x * (double)pT->ymax) / pT->xmax);
					mv_mouse_x += Mse.MP.x;
					mv_mouse_y += Mse.MP.y;
					/* change cursor type */
					pT->set_cursor(3, 0, 0);
					/* warp pointer */
					if(Mse.Cfg.warp_pointer)
						pT->set_cursor(-2, mv_mouse_x, mv_mouse_y);
					/* turn on the zoom box */
					pT->set_cursor(-1, Mse.SettingZoomP.x, Mse.SettingZoomP.y);
				}
				if(display_ipc_commands()) {
					fprintf(stderr, "starting zoom region.\n");
				}
			}
		}
		else {
			/* complete zoom (any button finishes zooming.) */

			/* the following variables are used to check,
			 * if the box is big enough to be considered
			 * as zoom box. */
			int dist_x = Mse.SettingZoomP.x - Mse.MP.x;
			int dist_y = Mse.SettingZoomP.y - Mse.MP.y;
			int dist = (int)sqrt((double)(dist_x * dist_x + dist_y * dist_y));

			if(1 == b || 2 == b) {
				/* zoom region is finished by the `wrong' button.
				 * `trap' the next button-release event so that
				 * it won't trigger the actions which are bound
				 * to these events.
				 */
				Mse.TrapRelease = true;
			}

			if(pT->set_cursor) {
				pT->set_cursor(0, 0, 0);
				if(Mse.Cfg.annotate_zoom_box && pT->put_tmptext) {
					pT->put_tmptext(1, "");
					pT->put_tmptext(2, "");
				}
			}
			if(dist > 10) { // more ore less arbitrary 
				double xmin, ymin, x2min, y2min;
				double xmax, ymax, x2max, y2max;
				MousePosToGraphPosReal(Mse.SettingZoomP.x, Mse.SettingZoomP.y, &xmin, &ymin, &x2min, &y2min);
				xmax = Mse.RealP.x;
				x2max = Mse.RealP2.x;
				ymax = Mse.RealP.y;
				y2max = Mse.RealP2.y;
				// keep the axes (no)reversed as they are now 
#define rev(a1, a2, A) if(sgn(a2-a1) != sgn(AxA[A].Range.GetDistance())) { Exchange(&a1, &a2); }
				rev(xmin,  xmax,  FIRST_X_AXIS);
				rev(ymin,  ymax,  FIRST_Y_AXIS);
				rev(x2min, x2max, SECOND_X_AXIS);
				rev(y2min, y2max, SECOND_Y_AXIS);
#undef rev
				DoZoom(pT, xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
				if(display_ipc_commands()) {
					fprintf(stderr, "zoom region finished.\n");
				}
			}
			else {
				/* silently ignore a tiny zoom box. This might
				 * happen, if the user starts and finishes the
				 * zoom box at the same position. */
			}
			Mse.setting_zoom_region = false;
		}
	}
	else {
		if(pT->set_cursor) {
			if(Mse.button & (1 << 1))
				pT->set_cursor(1, 0, 0);
			else if(Mse.button & (1 << 2))
				pT->set_cursor(2, 0, 0);
		}
	}
	Mse.SP = Mse.MP;
	Mse.zero_rot_z = surface_rot_z + 360.0f * Mse.MP.x / pT->xmax;
	// zero_rot_x = surface_rot_x - 180.0 * mouse_y / pT->ymax;
	Mse.zero_rot_x = surface_rot_x - 360.0f * Mse.MP.y / pT->ymax;
}

//static void event_buttonrelease(GpEvent * ge)
void GpGadgets::EventButtonRelease(GpTermEntry * pT, GpEvent * pGe)
{
	Mse.MP.x = pGe->mx;
	Mse.MP.y = pGe->my;
	int    b = pGe->par1;
	int    doubleclick = pGe->par2;
	Mse.button &= ~(1 << b);    /* remove button */
	if(!Mse.setting_zoom_region) {
		if(Mse.TrapRelease = true) {
			Mse.TrapRelease = false;
		}
		else {
			MousePosToGraphPosReal(Mse.MP.x, Mse.MP.y, &Mse.RealP.x, &Mse.RealP.y, &Mse.RealP2.x, &Mse.RealP2.y);
			FPRINTF((stderr, "MOUSE.C: doublclick=%i, set=%i, motion=%i, ALMOST2D=%i\n", (int)doubleclick, (int)Mse.Cfg.doubleclick,
					(int)motion, (int)ALMOST2D));
			if(ALMOST2D) {
				char s0[256];
				if(b == 1 && pT->set_clipboard && ((doubleclick <= Mse.Cfg.doubleclick) || !Mse.Cfg.doubleclick)) {
					// put coordinates to clipboard. For 3d plots this takes
					// only place, if the user didn't drag (rotate) the plot 
					if(!Is3DPlot || !Mse.motion) {
						GetAnnotateString(s0, Mse.RealP.x, Mse.RealP.y, mouse_mode, mouse_alt_string);
						pT->set_clipboard(s0);
						if(display_ipc_commands()) {
							fprintf(stderr, "put `%s' to clipboard.\n", s0);
						}
					}
				}
				if(b == 2) {
					// draw temporary annotation or label. For 3d plots this is
					// only done if the user didn't drag (scale) the plot 
					if(!Is3DPlot || !Mse.motion) {
						GetAnnotateString(s0, Mse.RealP.x, Mse.RealP.y, mouse_mode, mouse_alt_string);
						if(Mse.Cfg.label) {
							if(Mse.modifier_mask & Mod_Ctrl) {
								RemoveLabel(pT, GpGg.Gp__C, Mse.MP.x, Mse.MP.y);
							}
							else {
								put_label(s0, Mse.RealP.x, Mse.RealP.y);
							}
						}
						else {
							int x = Mse.MP.x;
							int y = Mse.MP.y;
							int dx = pT->HTic;
							int dy = pT->VTic;
							(pT->linewidth)(BorderLp.l_width);
							(pT->linetype)(BorderLp.l_type);
							pT->_Move(x - dx, y);
							pT->_Vector(x + dx, y);
							pT->_Move(x, y - dy);
							pT->_Vector(x, y + dy);
							(pT->justify_text)(LEFT);
							(pT->put_text)(x + dx / 2, y + dy / 2 + pT->VChr / 3, s0);
							(pT->text)();
						}
					}
				}
			}
			if(Is3DPlot && (b == 1 || b == 2)) {
				if(!!(Mse.modifier_mask & Mod_Ctrl) && !Mse.needreplot) {
					// redraw the 3d plot if its last redraw was 'quick' (only axes) because modifier key was pressed 
					DoSave3DPlot(GpGg.Gp__C, P_First3DPlot, plot3d_num, 0);
				}
				if(pT->set_cursor)
					pT->set_cursor((Mse.button & (1 << 1)) ? 1 : (Mse.button & (1 << 2)) ? 2 : 0, 0, 0);
			}
			// Export current mouse coords to user-accessible variables also 
			LoadMouseVariables(Mse.MP.x, Mse.MP.y, true, b);
			UpdateStatusline(pT);
			// In 2D mouse button 1 is available for "bind" commands 
			if(!Is3DPlot && (b == 1)) {
				int save = pGe->par1;
				pGe->par1 = GP_Button1;
				pGe->par2 = 0;
				EventKeyPress(pT, GpGg.Gp__C, pGe, true);
				pGe->par1 = save; // needed for "pause mouse" 
			}
		}
	}
}

//static void event_motion(GpEvent * ge)
void GpGadgets::EventMotion(GpCommand & rC, GpEvent * pGe)
{
	Mse.motion = 1;
	Mse.MP.x = pGe->mx;
	Mse.MP.y = pGe->my;
	if(Is3DPlot && (splot_map == false)) { // Rotate the surface if it is 3D graph but not "set view map"
		bool redraw = false;
		if(Mse.button & (1 << 1)) {
			/* dragging with button 1 -> rotate */
			/*surface_rot_x = floor(0.5 + zero_rot_x + 180.0 * mouse_y / term->ymax);*/
			surface_rot_x = floorf(0.5f + fmodf(Mse.zero_rot_x + 360.0f * Mse.MP.y / term->ymax, 360));
			if(surface_rot_x < 0)
				surface_rot_x += 360;
			if(surface_rot_x > 360)
				surface_rot_x -= 360;
			surface_rot_z = floorf(0.5f + fmodf(Mse.zero_rot_z - 360.0f * Mse.MP.x / term->xmax, 360));
			if(surface_rot_z < 0)
				surface_rot_z += 360;
			redraw = true;
		}
		else if(Mse.button & (1 << 2)) {
			// dragging with button 2 -> scale or changing ticslevel.
			// we compare the movement in x and y direction, and
			// change either scale or zscale 
			double relx = (double)abs(Mse.MP.x - Mse.SP.x) / (double)term->HTic;
			double rely = (double)abs(Mse.MP.y - Mse.SP.y) / (double)term->VTic;
			if(Mse.modifier_mask & Mod_Shift) {
				xyplane.Z += (1 + fabs(xyplane.Z)) * (Mse.MP.y - Mse.SP.y) * 2.0 / term->ymax;
			}
			else {
				if(relx > rely) {
					surface_lscale += (Mse.MP.x - Mse.SP.x) * 2.0f / term->xmax;
					surface_scale = expf(surface_lscale);
					SETMAX(surface_scale, 0);
				}
				else {
					if(disable_mouse_z && ((Mse.MP.y-Mse.SP.y) > 0))
						;
					else {
						surface_zscale += (Mse.MP.y - Mse.SP.y) * 2.0f / term->ymax;
						disable_mouse_z = false;
					}
					SETMAX(surface_zscale, 0);
				}
			}
			// reset the start values 
			Mse.SP.x = Mse.MP.x;
			Mse.SP.y = Mse.MP.y;
			redraw = true;
		}
		if(!ALMOST2D) {
			turn_ruler_off();
		}

		if(redraw) {
			if(allowmotion) {
				/* is processing of motions allowed right now?
				 * then replot while
				 * disabling further replots until it completes */
				allowmotion = false;
				DoSave3DPlot(rC, P_First3DPlot, plot3d_num, !!(Mse.modifier_mask & Mod_Ctrl));
				Ev.FillGpValFloat("GPVAL_VIEW_ROT_X", surface_rot_x);
				Ev.FillGpValFloat("GPVAL_VIEW_ROT_Z", surface_rot_z);
				Ev.FillGpValFloat("GPVAL_VIEW_SCALE", surface_scale);
				Ev.FillGpValFloat("GPVAL_VIEW_ZSCALE", surface_zscale);
			}
			else {
				/* postpone the replotting */
				Mse.needreplot = true;
			}
		}
	} /* if(3D plot) */

	if(ALMOST2D) {
		/* 2D plot, or suitably aligned 3D plot: update
		 * statusline and possibly the zoombox annotation */
		if(!term->put_tmptext)
			return;
		MousePosToGraphPosReal(Mse.MP.x, Mse.MP.y, &Mse.RealP.x, &Mse.RealP.y, &Mse.RealP2.x, &Mse.RealP2.y);
		UpdateStatusline(term);
		if(Mse.setting_zoom_region && Mse.Cfg.annotate_zoom_box) {
			double real_x, real_y, real_x2, real_y2;
			char s[64];
			MousePosToGraphPosReal(Mse.MP.x, Mse.MP.y, &real_x, &real_y, &real_x2, &real_y2);
			sprintf(s, zoombox_format(), real_x, real_y);
			term->put_tmptext(2, s);
		}
	}
}

static void event_modifier(GpEvent * ge)
{
	GpGg.Mse.modifier_mask = ge->par1;
	if(GpGg.Mse.modifier_mask == 0 && GpGg.Is3DPlot && (GpGg.Mse.button & ((1 << 1) | (1 << 2))) && !GpGg.Mse.needreplot) {
		// redraw the 3d plot if modifier key released 
		GpGg.DoSave3DPlot(GpGg.Gp__C, GpGg.P_First3DPlot, GpGg.plot3d_num, 0);
	}
}

void event_plotdone()
{
	if(GpGg.Mse.needreplot) {
		GpGg.Mse.needreplot = false;
		GpGg.DoSave3DPlot(GpGg.Gp__C, GpGg.P_First3DPlot, GpGg.plot3d_num, !!(GpGg.Mse.modifier_mask & Mod_Ctrl));
	}
	else {
		allowmotion = true;
	}
}

//void event_reset(GpEvent * ge)
void GpGadgets::EventReset(GpTermEntry * pT, GpEvent * pEv)
{
	Mse.modifier_mask = 0;
	Mse.button = 0;
	builtin_cancel_zoom(pEv);
	if(pT && term_initialised && pT->set_cursor) {
		pT->set_cursor(0, 0, 0);
		if(Mse.Cfg.annotate_zoom_box && pT->put_tmptext) {
			pT->put_tmptext(1, "");
			pT->put_tmptext(2, "");
		}
	}
	if(paused_for_mouse) {
		paused_for_mouse = 0;
#ifdef WIN32
		kill_pending_Pause_dialog(); // close pause message box 
#endif
		/* This hack is necessary on some systems in order to prevent one  */
		/* character of input from being swallowed when the plot window is */
		/* closed. But which systems, exactly?                             */
		if(pT && (!strncmp("x11", pT->name, 3) || !strncmp("wxt", pT->name, 3) || !strncmp("qt", pT->name, 2)))
			ungetc('\n', stdin);
	}
	/* Dummy up a keystroke event so that we can conveniently check for a  */
	/* binding to "Close". We only get these for the current window. */
	if(pEv != (void*)1) {
		pEv->par1 = GP_Cancel; /* Dummy keystroke */
		pEv->par2 = 0;   /* Not used; could pass window id here? */
		EventKeyPress(pT, GpGg.Gp__C, pEv, true);
	}
}

//void do_event(GpEvent * ge)
void GpGadgets::DoEvent(GpTermEntry * pT, GpEvent * pGe)
{
	if(pT) {
		// disable `replot` when some data were sent through stdin
		Gp__C.IsReplotDisabled = GpDf.plotted_data_from_stdin;
		if(pGe->type) {
			FPRINTF((stderr, "(do_event) type       = %d\n", pGe->type));
			FPRINTF((stderr, "           mx, my     = %d, %d\n", pGe->mx, pGe->my));
			FPRINTF((stderr, "           par1, par2 = %d, %d\n", pGe->par1, pGe->par2));
		}
		switch(pGe->type) {
			case GE_plotdone:
				event_plotdone();
				if(pGe->winid) {
					CurrentX11WindowId = pGe->winid;
					Ev.UpdateGpValVariables(6); /* fill GPVAL_TERM_WINDOWID */
				}
				break;
			case GE_keypress:
				EventKeyPress(pT, Gp__C, pGe, true);
				break;
			case GE_keypress_old:
				EventKeyPress(pT, Gp__C, pGe, false);
				break;
			case GE_modifier:
				event_modifier(pGe);
				break;
			case GE_motion:
				if(!Mse.Cfg.on)
					break;
				EventMotion(Gp__C, pGe);
				break;
			case GE_buttonpress:
				if(!Mse.Cfg.on)
					break;
				EventButtonPress(pT, Gp__C, pGe);
				break;
			case GE_buttonrelease:
				if(!Mse.Cfg.on)
					break;
				EventButtonRelease(pT, pGe);
				break;
			case GE_replot:
				// auto-generated replot (e.g. from replot-on-resize) 
				// FIXME: more terminals should use this! 
				if(Gp__C.P_ReplotLine == NULL || Gp__C.P_ReplotLine[0] == '\0')
					break;
				if(!strncmp(Gp__C.P_ReplotLine, "test", 4))
					break;
				if(IsMultiPlot)
					break;
				DoStringReplot(Gp__C, "");
				break;
			case GE_reset:
				EventReset(pT, pGe);
				break;
			case GE_fontprops:
	#ifdef X11
				/* EAM FIXME:  Despite the name, only X11 uses this to pass font info.	*/
				/* Everyone else passes just the plot height and width.			*/
				if(!strcmp(pT->name, "x11")) {
					/* These are declared in ../pT/x11.trm */
					extern int X11_hchar_saved, X11_vchar_saved;
					extern double X11_ymax_saved;
					// Cached sizing values for the x11 terminal. Each time an X11 window is
					// resized, these are updated with the new sizes. When a replot happens some
					// time later, these saved values are used. The normal mechanism for doing this
					// is sending a QG from inboard to outboard driver, then the outboard driver
					// responds with the sizing info in a GE_fontprops event. The problem is that
					// other than during plot initialization the communication is asynchronous.
					X11_hchar_saved = pGe->par1;
					X11_vchar_saved = pGe->par2;
					X11_ymax_saved = (double)pT->xmax * (double)pGe->my / fabs((double)pGe->mx);

					/* If mx < 0, we simply save the values for future use, and move on */
					if(pGe->mx < 0) {
						break;
					}
					else {
						/* Otherwise we apply the changes right now */
						pT->HChr = X11_hchar_saved;
						pT->VChr = X11_vchar_saved;
						/* factor of 2.5 must match the use in x11.trm */
						pT->HTic = pT->VTic = X11_vchar_saved / 2.5;
						pT->ymax  = X11_ymax_saved;
					}
				}
				else
				/* Fall through to cover non-x11 case */
	#endif
				/* Other terminals update aspect ratio based on current window size */
				pT->VTic = (uint)(pT->HTic * (double)pGe->mx / (double)pGe->my);
				FPRINTF((stderr, "mouse do_event: window size %d X %d, font hchar %d vchar %d\n", pGe->mx, pGe->my, pGe->par1, pGe->par2));
				break;
			case GE_buttonpress_old:
			case GE_buttonrelease_old:
				/* ignore */
				break;
			default:
				fprintf(stderr, "%s:%d protocol error\n", __FILE__, __LINE__);
				break;
		}
		Gp__C.IsReplotDisabled = false; // enable replot again
	}
}

//static void do_save_3dplot(SurfacePoints * plots, int pcount, int quick)
void GpGadgets::DoSave3DPlot(GpCommand & rC, SurfacePoints * plots, int pcount, int quick)
{
#if(0)
#define M_TEST_AXIS(A) (A.log && ((!(A.set_autoscale & AUTOSCALE_MIN) && A.set_min <= 0) || (!(A.set_autoscale & AUTOSCALE_MAX) && A.set_max <= 0)))
#endif
	if(!plots || (E_REFRESH_NOT_OK == RefreshOk)) {
		/* !plots might happen after the `reset' command for example
		 * (reported by Franz Bakan).
		 * !RefreshOk can happen for example if log scaling is reset (EAM).
		 * ReplotRequest should set up everything again in either case.
		 */
		ReplotRequest(rC);
	}
	else {
#if(0) /* Dead code.  This error is now trapped elsewhere */
		if(M_TEST_AXIS(GetX()) || M_TEST_AXIS(GetY()) || M_TEST_AXIS(GetZ()) || M_TEST_AXIS(GetCB())) {
			GpGg.IntErrorNoCaret("axis ranges must be above 0 for log scale!");
			return;
		}
#endif
		Do3DPlot(term, plots, pcount, quick);
	}

#undef M_TEST_AXIS
}

/*
 * bind related functions
 */

//static void bind_install_default_bindings()
void GpMouse::BindInstallDefaultBindings()
{
	BindRemoveAll();
	BindAppend("a", (char*)0, builtin_autoscale);
	BindAppend("b", (char*)0, builtin_toggle_border);
	BindAppend("e", (char*)0, builtin_replot);
	BindAppend("g", (char*)0, builtin_toggle_grid);
	BindAppend("h", (char*)0, builtin_help);
	BindAppend("i", (char*)0, builtin_invert_plot_visibilities);
	BindAppend("l", (char*)0, builtin_toggle_log);
	BindAppend("L", (char*)0, builtin_nearest_log);
	BindAppend("m", (char*)0, builtin_toggle_mouse);
	BindAppend("r", (char*)0, builtin_toggle_ruler);
	BindAppend("V", (char*)0, builtin_set_plots_invisible);
	BindAppend("v", (char*)0, builtin_set_plots_visible);
	BindAppend("1", (char*)0, builtin_decrement_mousemode);
	BindAppend("2", (char*)0, builtin_increment_mousemode);
	BindAppend("5", (char*)0, builtin_toggle_polardistance);
	BindAppend("6", (char*)0, builtin_toggle_verbose);
	BindAppend("7", (char*)0, builtin_toggle_ratio);
	BindAppend("n", (char*)0, builtin_zoom_next);
	BindAppend("p", (char*)0, builtin_zoom_previous);
	BindAppend("u", (char*)0, builtin_unzoom);
	BindAppend("+", (char*)0, builtin_zoom_in_around_mouse);
	BindAppend("=", (char*)0, builtin_zoom_in_around_mouse); /* same key as + but no need for Shift */
	BindAppend("-", (char*)0, builtin_zoom_out_around_mouse);
	BindAppend("Right", (char*)0, builtin_rotate_right);
	BindAppend("Up", (char*)0, builtin_rotate_up);
	BindAppend("Left", (char*)0, builtin_rotate_left);
	BindAppend("Down", (char*)0, builtin_rotate_down);
	BindAppend("Escape", (char*)0, builtin_cancel_zoom);
}

static void bind_clear(GpMouse::Bind * b)
{
	b->key = GpGg.Mse.NO_KEY;
	b->modifier = 0;
	b->command = (char*)0;
	b->builtin = 0;
	b->prev = (GpMouse::Bind*)0;
	b->next = (GpMouse::Bind*)0;
}
//
// returns the enum which corresponds to the
// string (ptr) or GpGg.Mse.NO_KEY if ptr matches not
// any of special_keys
//
static int lookup_key(char * ptr, int * len)
{
	// first, search in the table of "usual well-known" keys 
	int what = lookup_table_nth(usual_special_keys, ptr);
	if(what >= 0) {
		*len = strlen(usual_special_keys[what].key);
		return usual_special_keys[what].value;
	}
	else {
		// second, search in the table of other keys
		for(const char ** keyptr = special_keys; *keyptr; ++keyptr) {
			if(!strcmp(ptr, *keyptr)) {
				*len = strlen(ptr);
				return keyptr - special_keys + GP_FIRST_KEY;
			}
		}
		return GpGg.Mse.NO_KEY;
	}
}

/* returns 1 on success, else 0. */
static int bind_scan_lhs(GpMouse::Bind * out, const char * in)
{
	static const char DELIM = '-';
	int itmp = GpGg.Mse.NO_KEY;
	char * ptr;
	int len;
	bind_clear(out);
	if(!in) {
		return 0;
	}
	for(ptr = (char*)in; ptr && *ptr; /* EMPTY */) {
		if(!strncasecmp(ptr, "alt-", 4)) {
			out->modifier |= Mod_Alt;
			ptr += 4;
		}
		else if(!strncasecmp(ptr, "ctrl-", 5)) {
			out->modifier |= Mod_Ctrl;
			ptr += 5;
		}
		else if(GpGg.Mse.NO_KEY != (itmp = lookup_key(ptr, &len))) {
			out->key = itmp;
			ptr += len;
		}
		else if((out->key = *ptr++) && *ptr && *ptr != DELIM) {
			fprintf(stderr, "bind: cannot parse %s\n", in);
			return 0;
		}
	}
	if(GpGg.Mse.NO_KEY == out->key)
		return 0;       /* failed */
	else
		return 1;       /* success */
}

/* note, that this returns a pointer
 * to the static char* `out' which is
 * modified on subsequent calls.
 */
static char * bind_fmt_lhs(const GpMouse::Bind * in)
{
	static char out[0x40];
	out[0] = '\0';          /* empty string */
	if(!in)
		return out;
	if(in->modifier & Mod_Ctrl) {
		sprintf(out, "Ctrl-");
	}
	if(in->modifier & Mod_Alt) {
		strcat(out, "Alt-");
	}
	if(in->key > GP_FIRST_KEY && in->key < GP_LAST_KEY) {
		strcat(out, special_keys[in->key - GP_FIRST_KEY]);
	}
	else {
		int k = 0;
		for(; usual_special_keys[k].value > 0; k++) {
			if(usual_special_keys[k].value == in->key) {
				strcat(out, usual_special_keys[k].key);
				k = -1;
				break;
			}
		}
		if(k >= 0) {
			char foo[2] = {'\0', '\0'};
			foo[0] = in->key;
			strcat(out, foo);
		}
	}
	return out;
}

static int bind_matches(const GpMouse::Bind * a, const GpMouse::Bind * b)
{
	// discard Shift modifier 
	int a_mod = a->modifier & (Mod_Ctrl | Mod_Alt);
	int b_mod = b->modifier & (Mod_Ctrl | Mod_Alt);
	if(a->key == b->key && a_mod == b_mod)
		return 1;
	else
		return 0;
}

static void bind_display_one(GpMouse::Bind * ptr)
{
	fprintf(stderr, " %-12s ", bind_fmt_lhs(ptr));
	fprintf(stderr, "%c ", ptr->allwindows ? '*' : ' ');
	if(ptr->command) {
		fprintf(stderr, "`%s`\n", ptr->command);
	}
	else if(ptr->builtin) {
		fprintf(stderr, "%s\n", ptr->builtin(0));
	}
	else {
		fprintf(stderr, "`%s:%d oops.'\n", __FILE__, __LINE__);
	}
}

static void bind_display(char * lhs)
{
	GpMouse::Bind * ptr;
	GpMouse::Bind lhs_scanned;
	if(!GpGg.Mse.P_Bindings) {
		GpGg.Mse.BindInstallDefaultBindings();
	}
	if(!lhs) {
		// display all GpGg.Mse.P_Bindings 
		char fmt[] = " %-17s  %s\n";
		fprintf(stderr, "\n");
		// mouse buttons 
		fprintf(stderr, fmt, "<B1> doubleclick", "send mouse coordinates to clipboard (pm win wxt x11)");
		fprintf(stderr, fmt, "<B2>", "annotate the graph using `mouseformat` (see keys '1', '2')");
		fprintf(stderr, fmt, "", "or draw labels if `set mouse labels is on`");
		fprintf(stderr, fmt, "<Ctrl-B2>", "remove label close to pointer if `set mouse labels` is on");
		fprintf(stderr, fmt, "<B3>", "mark zoom region (only for 2d-plots and maps)");
		fprintf(stderr, fmt, "<B1-Motion>", "change view (rotation); use <Ctrl> to rotate the axes only");
		fprintf(stderr, fmt, "<B2-Motion>", "change view (scaling); use <Ctrl> to scale the axes only");
		fprintf(stderr, fmt, "<Shift-B2-Motion>", "vertical motion -- change xyplane");

		/* mouse wheel */
		fprintf(stderr, fmt, "<wheel-up>", "  scroll up (in +Y direction)");
		fprintf(stderr, fmt, "<wheel-down>", "  scroll down");
		fprintf(stderr, fmt, "<shift-wheel-up>", "  scroll left (in -X direction)");
		fprintf(stderr, fmt, "<shift-wheel-down>", " scroll right");
		fprintf(stderr, fmt, "<Control-WheelUp>", "  zoom in on mouse position");
		fprintf(stderr, fmt, "<Control-WheelDown>", "zoom out on mouse position");
		fprintf(stderr, fmt, "<Shift-Control-WheelUp>", "  pinch on x");
		fprintf(stderr, fmt, "<Shift-Control-WheelDown>", "expand on x");

		fprintf(stderr, "\n");
		/* keystrokes */
#ifndef DISABLE_SPACE_RAISES_CONSOLE
		fprintf(stderr, " %-12s   %s\n", "Space", "raise gnuplot console window");
#endif
		fprintf(stderr, " %-12s * %s\n", "q", "close this plot window");
		fprintf(stderr, "\n");
		for(ptr = GpGg.Mse.P_Bindings; ptr; ptr = ptr->next) {
			bind_display_one(ptr);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "              * indicates this key is active from all plot windows\n");
		fprintf(stderr, "\n");
		return;
	}
	if(!bind_scan_lhs(&lhs_scanned, lhs)) {
		return;
	}
	for(ptr = GpGg.Mse.P_Bindings; ptr; ptr = ptr->next) {
		if(bind_matches(&lhs_scanned, ptr)) {
			bind_display_one(ptr);
			break;  /* only one match */
		}
	}
}

//static void bind_remove(GpMouse::Bind * b)
void GpMouse::BindRemove(GpMouse::Bind * b)
{
	if(b) {
		if(b->builtin) {
			// don't remove builtins, just remove the overriding command 
			ZFREE(b->command);
		}
		else {
			if(b->prev)
				b->prev->next = b->next;
			if(b->next)
				b->next->prev = b->prev;
			else
				P_Bindings->prev = b->prev;
			if(b->command) {
				ZFREE(b->command);
			}
			if(b == P_Bindings) {
				P_Bindings = b->next;
				if(P_Bindings && P_Bindings->prev) {
					P_Bindings->prev->next = (GpMouse::Bind*)0;
				}
			}
			free(b);
		}
	}
}

//static void bind_append(char * lhs, char * rhs, char *(*builtin)(GpEvent* ge))
void GpMouse::BindAppend(char * lhs, char * rhs, char *(*builtin)(GpEvent* ge))
{
	GpMouse::Bind * p_new = (GpMouse::Bind *)malloc(sizeof(GpMouse::Bind));
	if(!bind_scan_lhs(p_new, lhs)) {
		free(p_new);
	}
	else {
		if(!P_Bindings) {
			P_Bindings = p_new; // first binding 
		}
		else {
			GpMouse::Bind * ptr;
			for(ptr = P_Bindings; ptr; ptr = ptr->next) {
				if(bind_matches(p_new, ptr)) {
					/* overwriting existing binding */
					if(!rhs) {
						ptr->builtin = builtin;
					}
					else if(*rhs) {
						if(ptr->command) {
							ZFREE(ptr->command);
						}
						ptr->command = rhs;
					}
					else { /* rhs is an empty string, so remove the binding */
						BindRemove(ptr);
					}
					free(p_new); /* don't need it any more */
					return;
				}
			}
			/* if we're here, the binding does not exist yet */
			/* append binding ... */
			P_Bindings->prev->next = p_new;
			p_new->prev = P_Bindings->prev;
		}
		P_Bindings->prev = p_new;
		p_new->next = (GpMouse::Bind*)0;
		p_new->allwindows = false; /* Can be explicitly set later */
		if(!rhs) {
			p_new->builtin = builtin;
		}
		else if(*rhs) {
			p_new->command = rhs; /* was allocated in command.c */
		}
		else {
			BindRemove(p_new);
		}
	}
}

//void bind_process(char * lhs, char * rhs, bool allwindows)
void GpMouse::BindProcess(char * lhs, char * rhs, bool allwindows)
{
	if(!P_Bindings) {
		BindInstallDefaultBindings();
	}
	if(!rhs) {
		bind_display(lhs);
	}
	else {
		BindAppend(lhs, rhs, 0);
		if(allwindows)
			BindAll(lhs);
	}
	free(lhs);
}

void GpMouse::BindAll(char * lhs)
{
	GpMouse::Bind keypress;
	if(bind_scan_lhs(&keypress, lhs)) {
		for(GpMouse::Bind * ptr = P_Bindings; ptr; ptr = ptr->next) {
			if(bind_matches(&keypress, ptr))
				ptr->allwindows = true;
		}
	}
}

//void bind_remove_all()
void GpMouse::BindRemoveAll()
{
	GpMouse::Bind * safe;
	for(GpMouse::Bind * ptr = P_Bindings; ptr; safe = ptr, ptr = ptr->next, free(safe)) {
		ZFREE(ptr->command);
	}
	P_Bindings = (GpMouse::Bind*)0;
}
//
// Ruler is on, thus recalc its (px,py) from (x,y) for the current zoom and log axes
//
//static void recalc_ruler_pos()
void GpGadgets::RecalcRulerPos()
{
	double P, dummy;
	if(Is3DPlot) {
		// To be exact, it is 'set view map' splot. 
		int ppx, ppy;
		dummy = 1.0; // dummy value, but not 0.0 for the fear of log z-axis 
		Map3DXY(Mse.Ruler.P.x, Mse.Ruler.P.y, dummy, &ppx, &ppy);
		Mse.Ruler.px = ppx;
		Mse.Ruler.py = ppy;
	}
	else {
		// It is 2D plot
		if(AxA[FIRST_X_AXIS].Flags & GpAxis::fLog && Mse.Ruler.P.x < 0)
			Mse.Ruler.px = -1;
		else {
			P = LogValue(FIRST_X_AXIS, Mse.Ruler.P.x);
			Mse.Ruler.px = Map(FIRST_X_AXIS, P);
		}
		if(AxA[FIRST_Y_AXIS].Flags & GpAxis::fLog && Mse.Ruler.P.y < 0)
			Mse.Ruler.py = -1;
		else {
			P = LogValue(FIRST_Y_AXIS, Mse.Ruler.P.y);
			Mse.Ruler.py = Map(FIRST_Y_AXIS, P);
		}
		MousePosToGraphPosReal(Mse.Ruler.px, Mse.Ruler.py, &dummy, &dummy, &Mse.Ruler.P2.x, &Mse.Ruler.P2.y);
	}
}
//
// Recalculate and replot the GpGg.Mse.Ruler after a '(re)plot'. Called from term.c.
//
//void update_ruler()
void GpGadgets::UpdateRuler(GpTermEntry * pT)
{
	if(pT->set_ruler && Mse.Ruler.on) {
		(*pT->set_ruler)(-1, -1);
		RecalcRulerPos();
		(*pT->set_ruler)(Mse.Ruler.px, Mse.Ruler.py);
	}
}
//
// Set GpGg.Mse.Ruler on/off, and set its position.
// Called from set.c for 'set mouse GpGg.Mse.Ruler ...' command.
//
//void set_ruler(bool on, int mx, int my)
void GpGadgets::SetMouseRuler(bool on, int mx, int my)
{
	GpEvent ge;
	if(Mse.Ruler.on == false && on == false)
		return;
	if(Mse.Ruler.on == true && on == true && (mx < 0 || my < 0))
		return;
	if(Mse.Ruler.on == true) /* Mse.Ruler is on => switch it off */
		builtin_toggle_ruler(&ge);
	// now the Mse.Ruler is off 
	if(on == false) // want Mse.Ruler off 
		return;
	if(mx>=0 && my>=0) { /* change Mse.Ruler position */
		ge.mx = mx;
		ge.my = my;
	}
	else { /* don't change Mse.Ruler position */
		ge.mx = Mse.Ruler.px;
		ge.my = (int)Mse.Ruler.py;
	}
	builtin_toggle_ruler(&ge);
}

/* for checking if we change from plot to splot (or vice versa) */
int plot_mode(int set)
{
	static int mode = MODE_PLOT;
	if(MODE_PLOT == set || MODE_SPLOT == set) {
		if(mode != set) {
			turn_ruler_off();
		}
		mode = set;
	}
	return mode;
}

static void turn_ruler_off()
{
	if(GpGg.Mse.Ruler.on) {
		UdvtEntry * u;
		GpGg.Mse.Ruler.on = false;
		if(term && term->set_ruler) {
			(*term->set_ruler)(-1, -1);
		}
		if((u = GpGg.Ev.AddUdvByName("MOUSE_RULER_X")))
			u->udv_value.type = NOTDEFINED;
		if((u = GpGg.Ev.AddUdvByName("MOUSE_RULER_Y")))
			u->udv_value.type = NOTDEFINED;
		if(display_ipc_commands()) {
			fprintf(stderr, "turning GpGg.Mse.Ruler off.\n");
		}
	}
}

//static int nearest_label_tag(int xref, int yref)
int GpGadgets::NearestLabelTag(GpTermEntry * pT, int xref, int yref)
{
	double min = -1;
	int    min_tag = -1;
	double diff_squared;
	int    x, y;
	int xd;
	int yd;
	for(GpTextLabel * p_label = first_label; p_label != NULL; p_label = p_label->next) {
		if(Is3DPlot) {
			Map3DPosition(p_label->place, &xd, &yd, "label");
			xd -= xref;
			yd -= yref;
		}
		else {
			MapPosition(pT, &p_label->place, &x, &y, "label");
			xd = x - xref;
			yd = y - yref;
		}
		diff_squared = xd * xd + yd * yd;
		if(-1 == min || min > diff_squared) {
			// now we check if we're within a certain threshold around the label
			double tic_diff_squared;
			int htic, vtic;
			GetOffsets(p_label, &htic, &vtic);
			tic_diff_squared = htic * htic + vtic * vtic;
			if(diff_squared < tic_diff_squared) {
				min = diff_squared;
				min_tag = p_label->tag;
			}
		}
	}
	return min_tag;
}

//static void remove_label(int x, int y)
void GpGadgets::RemoveLabel(GpTermEntry * pT, GpCommand & rC, int x, int y)
{
	const int tag = NearestLabelTag(pT, x, y);
	if(-1 != tag) {
		char cmd[0x40];
		sprintf(cmd, "unset label %d", tag);
		DoStringReplot(rC, cmd);
	}
}

static void put_label(char * label, double x, double y)
{
	char cmd[256];
	sprintf(cmd, "set label \"%s\" at %g,%g %s", label, x, y, GpGg.Mse.Cfg.labelopts ? GpGg.Mse.Cfg.labelopts : "point pt 1");
	GpGg.DoStringReplot(GpGg.Gp__C, cmd);
}

/* Save current mouse position to user-accessible variables.
 * Save the keypress or mouse button that triggered this in MOUSE_KEY,
 * and define MOUSE_BUTTON if it was a button click.
 */
//static void load_mouse_variables(double x, double y, bool button, int c)
void GpGadgets::LoadMouseVariables(double x, double y, bool button, int c)
{
	UdvtEntry * p_current;
	MousePosToGraphPosReal((int)x, (int)y, &Mse.RealP.x, &Mse.RealP.y, &Mse.RealP2.x, &Mse.RealP2.y);
	if((p_current = Ev.AddUdvByName("MOUSE_BUTTON"))) {
		p_current->udv_value.SetInt(button ? c : -1);
		if(!button)
			p_current->udv_value.type = NOTDEFINED;
	}
	if((p_current = Ev.AddUdvByName("MOUSE_KEY"))) {
		p_current->udv_value.SetInt(c);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_CHAR"))) {
		char * keychar = (char *)malloc(2);
		keychar[0] = c;
		keychar[1] = '\0';
		gpfree_string(&p_current->udv_value);
		Gstring(&p_current->udv_value, keychar);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_X"))) {
		p_current->udv_value.SetComplex(Mse.RealP.x, 0);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_Y"))) {
		p_current->udv_value.SetComplex(Mse.RealP.y, 0);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_X2"))) {
		p_current->udv_value.SetComplex(Mse.RealP2.x, 0);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_Y2"))) {
		p_current->udv_value.SetComplex(Mse.RealP2.y, 0);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_SHIFT"))) {
		p_current->udv_value.SetInt(Mse.modifier_mask & Mod_Shift);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_ALT"))) {
		p_current->udv_value.SetInt(Mse.modifier_mask & Mod_Alt);
	}
	if((p_current = Ev.AddUdvByName("MOUSE_CTRL"))) {
		p_current->udv_value.SetInt(Mse.modifier_mask & Mod_Ctrl);
	}
}

#endif /* USE_MOUSE */
