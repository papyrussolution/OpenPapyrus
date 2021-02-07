// GNUPLOT - mouse.c 
//
// driver independent mouse part. 
//
/*
 * AUTHORS
 *   Original Software (October 1999 - January 2000):
 *     Pieter-Tjerk de Boer <ptdeboer@cs.utwente.nl>
 *     Petr Mikulik <mikulik@physics.muni.cz>
 *     Johannes Zellner <johannes@zellner.org>
 */
#include <gnuplot.h>
#pragma hdrstop

#ifdef USE_MOUSE /* comment out whole file, otherwise... */
#ifdef _WIN32
	#include "win/winmain.h"
#endif

/********************** variables ***********************************************************/
char mouse_fmt_default[] = "% #g";
udft_entry mouse_readout_function = {NULL, "mouse_readout_function", NULL, NULL, 2 /*dummy_values[]*/};

long mouse_mode = MOUSE_COORDINATES_REAL;
char* mouse_alt_string = (char*)0;
mouse_setting_t default_mouse_setting = DEFAULT_MOUSE_SETTING;
mouse_setting_t mouse_setting = DEFAULT_MOUSE_SETTING;

/* the following table must match exactly the
 * enum's of GP_ in mousecmn.h and end with a NULL pointer!
 */
static char* special_keys[] = {
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
	"Button2",
	"Button3",
	"GP_LAST_KEY",
	(char*)0 /* must be the last line */
};

/* "usual well-known" keycodes, i.e. those not listed in special_keys in mouse.h
 */
static const struct gen_table usual_special_keys[] =
{
	{ "BackSpace", GP_BackSpace},
	{ "Tab", GP_Tab},
	{ "KP_Enter", GP_KP_Enter},
	{ "Return", GP_Return},
	{ "Escape", GP_Escape},
	{ "Delete", GP_Delete},
	{ NULL, 0}
};

/* the status of the shift, ctrl and alt keys
 */
static int modifier_mask = 0;

/* Structure for the ruler: on/off, position,...
 */
static struct {
	bool on;
	double x, y, x2, y2;    /* ruler position in real units of the graph */
	long px, py;            /* ruler position in the viewport units */
} ruler = { false, 0.0, 0.0, 0.0, 0.0, 0, 0 };

/* the coordinates of the mouse cursor in gnuplot's internal coordinate system
 */
static int mouse_x = -1, mouse_y = -1;

/* the "real" coordinates of the mouse cursor, i.e., in the user's coordinate
 * system(s)
 */
static double real_x, real_y, real_x2, real_y2;
static int button = 0; // status of buttons; button i corresponds to bit (1<<i) of this variable
/* variables for setting the zoom region:
 */
static bool setting_zoom_region = false; // flag, TRUE while user is outlining the zoom region 
static int setting_zoom_x, setting_zoom_y; // coordinates of the first corner of the zoom region, in the internal coordinate system 

/* variables for changing the 3D view:
 */
/* do we allow motion to result in a replot right now? */
bool allowmotion = TRUE;        /* used by pm.trm, too */
static bool needreplot = FALSE; // did we already postpone a replot because allowmotion was FALSE ? 
static int start_x, start_y; // mouse position when dragging started 
static int motion = 0; // ButtonPress sets this to 0, ButtonMotion to 1 
static float zero_rot_x, zero_rot_z; // values for rot_x and rot_z corresponding to zero position of mouse 

/* bind related stuff */

struct bind_t {
	bind_t * prev;
	int key;
	char modifier;
	char * command;
	char *(*builtin)(struct gp_event_t * ge);
	bool allwindows;
	bind_t * next;
};

static bind_t * bindings = (bind_t*)0;
static const int NO_KEY = -1;
static bool trap_release = FALSE;

/*
 * event -> name translation for debugging
 */
const char* GE_evt_name(int type)
{
#define GE_EVT_NAME(name) case name: return #name;
	switch(type) {
		GE_EVT_LIST(GE_EVT_NAME);
		default:;
	}
	return "GE_UNKNOWN";
}

// forward declarations 
static void alert();
static char * xy_format();
static char * zoombox_format();
//static char * GetAnnotateString(char * s, double x, double y, int mode, char * fmt);
static char * xDateTimeFormat(double x, char * b, int mode);
static void GetRulerString(char * p, double x, double y);
//static void apply_zoom(struct t_zoom * z);
//static void do_zoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max);
static void ZoomNext();
static void ZoomPrevious();
static void ZoomUnzoom();
static void incr_mousemode(const int amount);
//static void UpdateStatuslineWithMouseSetting(mouse_setting_t * ms);
static bind_t * get_binding(struct gp_event_t * ge, bool current);
static void event_keypress(struct gp_event_t * ge, bool current);
static void ChangeView(int x, int z);
static void ChangeAzimuth(int x);
static void event_buttonpress(struct gp_event_t * ge);
static void event_buttonrelease(struct gp_event_t * ge);
static void event_motion(gp_event_t * ge);
static void event_modifier(gp_event_t * ge);
//static void do_save_3dplot(surface_points *, int, REPLOT_TYPE);
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
static char * builtin_autoscale(struct gp_event_t * ge);
static char * builtin_toggle_border(struct gp_event_t * ge);
static char * builtin_replot(struct gp_event_t * ge);
static char * builtin_toggle_grid(struct gp_event_t * ge);
static char * builtin_help(struct gp_event_t * ge);
static char * builtin_set_plots_visible(struct gp_event_t * ge);
static char * builtin_set_plots_invisible(struct gp_event_t * ge);
static char * builtin_invert_plot_visibilities(struct gp_event_t * ge);
static char * builtin_toggle_log(struct gp_event_t * ge);
static char * builtin_nearest_log(struct gp_event_t * ge);
static char * builtin_toggle_mouse(struct gp_event_t * ge);
static char * builtin_toggle_ruler(struct gp_event_t * ge);
static char * builtin_decrement_mousemode(struct gp_event_t * ge);
static char * builtin_increment_mousemode(struct gp_event_t * ge);
static char * builtin_toggle_polardistance(struct gp_event_t * ge);
static char * builtin_toggle_verbose(struct gp_event_t * ge);
static char * builtin_toggle_ratio(struct gp_event_t * ge);
static char * builtin_zoom_next(struct gp_event_t * ge);
static char * builtin_zoom_previous(struct gp_event_t * ge);
static char * builtin_unzoom(struct gp_event_t * ge);
static char * builtin_rotate_right(struct gp_event_t * ge);
static char * builtin_rotate_up(struct gp_event_t * ge);
static char * builtin_rotate_left(struct gp_event_t * ge);
static char * builtin_rotate_down(struct gp_event_t * ge);
static char * builtin_azimuth_left(struct gp_event_t * ge);
static char * builtin_azimuth_right(struct gp_event_t * ge);
static char * builtin_cancel_zoom(struct gp_event_t * ge);
static char * builtin_zoom_in_around_mouse(struct gp_event_t * ge);
static char * builtin_zoom_out_around_mouse(struct gp_event_t * ge);
#if (0) /* Not currently used */
static char * builtin_zoom_scroll_left(struct gp_event_t * ge);
static char * builtin_zoom_scroll_right(struct gp_event_t * ge);
static char * builtin_zoom_scroll_up(struct gp_event_t * ge);
static char * builtin_zoom_scroll_down(struct gp_event_t * ge);
static char * builtin_zoom_in_X(struct gp_event_t * ge);
static char * builtin_zoom_out_X(struct gp_event_t * ge);
#endif

/* prototypes for bind stuff
 * which are used only here. */
static void bind_install_default_bindings();
static void bind_clear(bind_t * b);
static int lookup_key(char * ptr, int * len);
static int bind_scan_lhs(bind_t * out, const char * in);
static char * bind_fmt_lhs(const bind_t * in);
static int bind_matches(const bind_t * a, const bind_t * b);
static void bind_display_one(bind_t * ptr);
static void bind_display(char * lhs);
static void bind_all(char * lhs);
static void bind_remove(bind_t * b);
static void bind_append(char * lhs, char * rhs, char *(*builtin)(struct gp_event_t * ge));
//static void recalc_ruler_pos();
static void turn_ruler_off();
static int nearest_label_tag(int x, int y);
static void remove_label(int x, int y);
static void put_label(char * label, double x, double y);

/********* functions ********************************************/
//
// produce a beep 
//
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

#ifndef HAVE_STPCPY
	// handy function for composing strings; note: some platforms may
	// already provide it, how should we handle that? autoconf? -- ptdb 
	char * stpcpy(char * s, const char * p)
	{
		strcpy(s, p);
		return s + strlen(p);
	}
#endif
//
// Transform mouse coordinates to graph coordinates
//
//static void MousePosToGraphPosReal(int xx, int yy, double * x, double * y, double * x2, double * y2)
void GnuPlot::MousePosToGraphPosReal(int xx, int yy, double * x, double * y, double * x2, double * y2)
{
	GpAxis * secondary;
	if(is_3d_plot) {
		// for 3D plots, we treat the mouse position as if it is
		// in the bottom plane, i.e., the plane of the x and y axis 
		// note: at present, this projection is only correct if
		// surface_rot_z is a multiple of 90 degrees! 
		// HBB 20010522: added protection against division by zero
		// for cases like 'set view 90,0' 
		xx -= axis3d_o_x;
		yy -= axis3d_o_y;
		if(abs(axis3d_x_dx) > abs(axis3d_x_dy)) {
			*x = AxS[FIRST_X_AXIS].min + ((double)xx) / axis3d_x_dx * (AxS[FIRST_X_AXIS].max - AxS[FIRST_X_AXIS].min);
		}
		else if(axis3d_x_dy != 0) {
			*x = AxS[FIRST_X_AXIS].min + ((double)yy) / axis3d_x_dy * (AxS[FIRST_X_AXIS].max - AxS[FIRST_X_AXIS].min);
		}
		else {
			/* both diffs are zero (x axis points into the screen */
			*x = VERYLARGE;
		}

		if(abs(axis3d_y_dx) > abs(axis3d_y_dy)) {
			*y = AxS[FIRST_Y_AXIS].min + ((double)xx) / axis3d_y_dx * (AxS[FIRST_Y_AXIS].max - AxS[FIRST_Y_AXIS].min);
		}
		else if(axis3d_y_dy != 0) {
			if(splot_map)
				*y = AxS[FIRST_Y_AXIS].max + ((double)yy) / axis3d_y_dy * (AxS[FIRST_Y_AXIS].min - AxS[FIRST_Y_AXIS].max);
			else
				*y = AxS[FIRST_Y_AXIS].min + ((double)yy) / axis3d_y_dy * (AxS[FIRST_Y_AXIS].max - AxS[FIRST_Y_AXIS].min);
		}
		else {
			/* both diffs are zero (y axis points into the screen */
			*y = VERYLARGE;
		}
		*x2 = *y2 = VERYLARGE; /* protection */
		return;
	}
	/* 2D plot */
	if(V.BbPlot.xright == V.BbPlot.xleft)
		*x = *x2 = VERYLARGE; /* protection */
	else {
		*x  = AxS[FIRST_X_AXIS].MapBack(xx);
		*x2 = AxS[SECOND_X_AXIS].MapBack(xx);
	}
	if(V.BbPlot.ytop == V.BbPlot.ybot)
		*y = *y2 = VERYLARGE; /* protection */
	else {
		*y  = AxS[FIRST_Y_AXIS].MapBack(yy);
		*y2 = AxS[SECOND_Y_AXIS].MapBack(yy);
	}
	FPRINTF((stderr, "POS: xx=%i, yy=%i  =>  x=%g  y=%g\n", xx, yy, *x, *y));

	/* If x2 or y2 is linked to a primary axis via mapping function, apply it now */
	/* FIXME:  this triggers on both linked x1/x2 and on nonlinear x2 */
	secondary = &AxS[SECOND_X_AXIS];
	if(secondary->IsNonLinear())
		*x2 = EvalLinkFunction(secondary, *x);
	secondary = &AxS[SECOND_Y_AXIS];
	if(secondary->IsNonLinear())
		*y2 = EvalLinkFunction(secondary, *y);
	// If x or y is linked to a (hidden) primary axis, it's a bit more complicated 
	secondary = &AxS[FIRST_X_AXIS];
	if(secondary->linked_to_primary &&  secondary->linked_to_primary->index == -FIRST_X_AXIS) {
		*x = secondary->linked_to_primary->MapBack(xx);
		*x = EvalLinkFunction(secondary, *x);
	}
	secondary = &AxS[FIRST_Y_AXIS];
	if(secondary->linked_to_primary &&  secondary->linked_to_primary->index == -FIRST_Y_AXIS) {
		*y = secondary->linked_to_primary->MapBack(yy);
		*y = EvalLinkFunction(secondary, *y);
	}
	secondary = &AxS[SECOND_X_AXIS];
	if(secondary->linked_to_primary &&  secondary->linked_to_primary->index == -SECOND_X_AXIS) {
		*x2 = secondary->linked_to_primary->MapBack(xx);
		*x2 = EvalLinkFunction(secondary, *x2);
	}
	secondary = &AxS[SECOND_Y_AXIS];
	if(secondary->linked_to_primary &&  secondary->linked_to_primary->index == -SECOND_Y_AXIS) {
		*y2 = secondary->linked_to_primary->MapBack(yy);
		*y2 = EvalLinkFunction(secondary, *y2);
	}
}

static char * xy_format()
{
	static char format[64];
	format[0] = NUL;
	strncat(format, mouse_setting.fmt, 30);
	strncat(format, ", ", 3);
	strncat(format, mouse_setting.fmt, 30);
	return format;
}

static char * zoombox_format()
{
	static char format[64];
	format[0] = NUL;
	strncat(format, mouse_setting.fmt, 30);
	strncat(format, "\r", 2);
	strncat(format, mouse_setting.fmt, 30);
	return format;
}
//
// formats the information for an annotation (middle mouse button clicked)
//
char * GnuPlot::GetAnnotateString(char * s, double x, double y, int mode, char * fmt)
{
	if(AxS[FIRST_X_AXIS].datatype == DT_DMS || AxS[FIRST_Y_AXIS].datatype == DT_DMS) {
		static char dms_format[16];
		sprintf(dms_format, "%%D%s%%.2m'", degree_sign);
		if(AxS[FIRST_X_AXIS].datatype == DT_DMS)
			gstrdms(s, fmt ? fmt : dms_format, x);
		else
			sprintf(s, mouse_setting.fmt, x);
		strcat(s, ", ");
		s += strlen(s);
		if(AxS[FIRST_Y_AXIS].datatype == DT_DMS)
			gstrdms(s, fmt ? fmt : dms_format, y);
		else
			sprintf(s, mouse_setting.fmt, y);
		s += strlen(s);
	}
	else if(oneof4(mode, MOUSE_COORDINATES_XDATE, MOUSE_COORDINATES_XTIME, MOUSE_COORDINATES_XDATETIME, MOUSE_COORDINATES_TIMEFMT)) { // time is on the x axis
		char buf[0xff];
		char format[0xff] = "[%s, ";
		strcat(format, mouse_setting.fmt);
		strcat(format, "]");
		sprintf(s, format, xDateTimeFormat(x, buf, mode), y);
	}
	else if(mode == MOUSE_COORDINATES_FRACTIONAL) {
		double xrange = AxS[FIRST_X_AXIS].max - AxS[FIRST_X_AXIS].min;
		double yrange = AxS[FIRST_Y_AXIS].max - AxS[FIRST_Y_AXIS].min;
		/* calculate fractional coordinates.
		 * prevent division by zero */
		if(xrange) {
			char format[0xff] = "/";
			strcat(format, mouse_setting.fmt);
			sprintf(s, format, (x - AxS[FIRST_X_AXIS].min) / xrange);
		}
		else {
			sprintf(s, "/(undefined)");
		}
		s += strlen(s);
		if(yrange) {
			char format[0xff] = ", ";
			strcat(format, mouse_setting.fmt);
			strcat(format, "/");
			sprintf(s, format, (y - AxS[FIRST_Y_AXIS].min) / yrange);
		}
		else {
			sprintf(s, ", (undefined)/");
		}
	}
	else if(mode == MOUSE_COORDINATES_REAL1) {
		sprintf(s, xy_format(), x, y); /* w/o brackets */
	}
	else if((mode == MOUSE_COORDINATES_ALT) && polar) {
		double r;
		double phi = atan2(y, x);
		double rmin = (AxS.__R().autoscale & AUTOSCALE_MIN) ? 0.0 : AxS.__R().set_min;
		double theta = phi / DEG2RAD;
		/* Undo "set theta" */
		theta = (theta - theta_origin) * theta_direction;
		if(theta > 180.0)
			theta = theta - 360.0;
		if(AxS.__R().IsNonLinear())
			r = EvalLinkFunction(&AxS.__R(), x/cos(phi) + AxS.__R().linked_to_primary->min);
		else if(AxS.__R().log)
			r = rmin + x/cos(phi);
		else if(inverted_raxis)
			r = rmin - x/cos(phi);
		else
			r = rmin + x/cos(phi);
		if(fmt)
			sprintf(s, fmt, theta, r);
		else
			sprintf(s, "theta: %.1f%s  r: %g", theta, degree_sign, r);
	}
	else if((mode == MOUSE_COORDINATES_ALT) && fmt) {
		sprintf(s, fmt, x, y); /* user defined format */
	}
	else if(mode == MOUSE_COORDINATES_FUNCTION) {
		/* EXPERIMENTAL !!! */
		GpValue original_x, original_y;
		GpValue readout;
		udvt_entry * plot_x = Ev.AddUdvByName("x");
		udvt_entry * plot_y = Ev.AddUdvByName("y");
		original_x = plot_x->udv_value;
		original_y = plot_y->udv_value;
		Gcomplex(&(plot_x->udv_value), x, 0);
		Gcomplex(&(plot_y->udv_value), y, 0);
		readout.SetNotDefined();
		EvaluateAt(mouse_readout_function.at, &readout);
		plot_x->udv_value = original_x;
		plot_y->udv_value = original_y;
		if(readout.type != STRING) {
			IntWarn(NO_CARET, "mouseformat function did not return a string");
		}
		else {
			sprintf(s, "%s", readout.v.string_val);
		}
		gpfree_string(&readout);
	}
	else {
		/* Default format ("set mouse mouseformat" is not active) */
		sprintf(s, xy_format(), x, y); /* usual x,y values */
	}
	return s + strlen(s);
}
// 
// Format x according to the date/time mouse mode. Uses and returns b as a buffer
// 
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
		    gstrftime(b, 0xff, P_TimeFormat, x);
		    break;
		default:
		    sprintf(b, mouse_setting.fmt, x);
	}
	return b;
}
// 
// Format one axis coordinate for output to mouse status or button 2 label text
// 
static char * mkstr(char * sp, double x, AXIS_INDEX axis)
{
	if(x >= VERYLARGE)
		return sp;
	if(axis == FIRST_X_AXIS && oneof4(mouse_mode, MOUSE_COORDINATES_XDATE, MOUSE_COORDINATES_XTIME, MOUSE_COORDINATES_XDATETIME, MOUSE_COORDINATES_TIMEFMT)) {
		// mouseformats 3 4 5 6 use specific time format for x coord
		xDateTimeFormat(x, sp, mouse_mode);
	}
	else if(GPO.AxS[axis].datatype == DT_TIMEDATE) {
		char * format = copy_or_invent_formatstring(&GPO.AxS[axis]);
		while(strchr(format, '\n'))
			*(strchr(format, '\n')) = ' ';
		gstrftime(sp, 40, format, x);
	}
	else
		sprintf(sp, mouse_setting.fmt, x);
	return (sp + strlen(sp));
}

/* ratio for log, distance for linear */
#define DIST(x, rx, axis) (GPO.AxS[axis].log) ? ((rx==0) ? 99999 : x / rx) : (x - rx)

/* formats the ruler information (position, distance,...) into string p
        (it must be sufficiently long)
   x, y is the current mouse position in real coords (for the calculation
        of distance)
 */
static void GetRulerString(char * p, double x, double y)
{
	double dx, dy;
	char format[0xff] = "  ruler: [";
	strcat(format, mouse_setting.fmt);
	strcat(format, ", ");
	strcat(format, mouse_setting.fmt);
	strcat(format, "]  distance: ");
	strcat(format, mouse_setting.fmt);
	strcat(format, ", ");
	strcat(format, mouse_setting.fmt);
	dx = DIST(x, ruler.x, FIRST_X_AXIS);
	dy = DIST(y, ruler.y, FIRST_Y_AXIS);
	sprintf(p, format, ruler.x, ruler.y, dx, dy);
	/* Previously, the following "if" let the polar coordinates to be shown only
	   for lin-lin plots:
	        if (mouse_setting.polardistance && !GPO.AxS[FIRST_X_AXIS].log && !GPO.AxS[FIRST_Y_AXIS].log) ...
	   Now, let us support also semilog and log-log plots.
	   Values of mouse_setting.polardistance are:
	        0 (no polar coordinates), 1 (polar coordinates), 2 (tangent instead of angle).
	 */
	if(mouse_setting.polardistance) {
		double rho, phi, rx, ry;
		char ptmp[69];
		rx = ruler.x;
		ry = ruler.y;
		format[0] = '\0';
		strcat(format, " (");
		strcat(format, mouse_setting.fmt);
		rho = sqrt((x - rx) * (x - rx) + (y - ry) * (y - ry)); /* distance */
		if(mouse_setting.polardistance == 1) { /* (distance, angle) */
			phi = (180 / M_PI) * atan2(y - ry, x - rx);
			strcat(format, ", % #.4gdeg)");
		}
		else { // mouse_setting.polardistance==2: (distance, tangent) 
			phi = x - rx;
			phi = (phi == 0) ? ((y-ry>0) ? VERYLARGE : -VERYLARGE) : (y - ry)/phi;
			sprintf(format+strlen(format), ", tangent=%s)", mouse_setting.fmt);
		}
		sprintf(ptmp, format, rho, phi);
		strcat(p, ptmp);
	}
}

static struct t_zoom * zoom_head = NULL, * zoom_now = NULL;
static GpAxis * axis_array_copy = NULL;
//
// Applies the zoom rectangle of  z  by sending the appropriate command to gnuplot
//
//static void apply_zoom(struct t_zoom * z)
void GnuPlot::ApplyZoom(t_zoom * z)
{
	int is_splot_map = (is_3d_plot && (splot_map == TRUE));
	if(zoom_now) { // remember the current zoom 
		zoom_now->_Min.x = AxS[FIRST_X_AXIS].set_min;
		zoom_now->_Max.x = AxS[FIRST_X_AXIS].set_max;
		zoom_now->_2Min.x = AxS[SECOND_X_AXIS].set_min;
		zoom_now->_2Max.x = AxS[SECOND_X_AXIS].set_max;
		zoom_now->_Min.y = AxS[FIRST_Y_AXIS].set_min;
		zoom_now->_Max.y = AxS[FIRST_Y_AXIS].set_max;
		zoom_now->_2Min.y = AxS[SECOND_Y_AXIS].set_min;
		zoom_now->_2Max.y = AxS[SECOND_Y_AXIS].set_max;
	}
	// EAM DEBUG - The autoscale save/restore was too complicated, and
	// broke refresh. Just save the complete axis state and have done with it.
	if(zoom_now == zoom_head && z != zoom_head) {
		axis_array_copy = (GpAxis *)gp_realloc(axis_array_copy, sizeof(GpAxis) * AXIS_ARRAY_SIZE, "axis_array copy");
		for(uint ci = 0; ci < AXIS_ARRAY_SIZE; ci++) {
			axis_array_copy[ci] = AxS[ci];
		}
		//memcpy(axis_array_copy, axis_array, sizeof(axis_array));
	}
	// If we are zooming, we don't want to autoscale the range.
	// This wasn't necessary before we introduced "refresh".  Why?
	if(zoom_now == zoom_head && z != zoom_head) {
		AxS[FIRST_X_AXIS].autoscale = AUTOSCALE_NONE;
		AxS[FIRST_Y_AXIS].autoscale = AUTOSCALE_NONE;
		AxS[SECOND_X_AXIS].autoscale = AUTOSCALE_NONE;
		AxS[SECOND_Y_AXIS].autoscale = AUTOSCALE_NONE;
	}
	zoom_now = z;
	if(zoom_now == NULL) {
		alert();
		return;
	}
	// Now we're committed. Notify the terminal the the next replot is a zoom 
	(*term->layer)(TERM_LAYER_BEFORE_ZOOM);
	// New range on primary axes 
	SetExplicitRange(&AxS[FIRST_X_AXIS], zoom_now->_Min.x, zoom_now->_Max.x);
	SetExplicitRange(&AxS[FIRST_Y_AXIS], zoom_now->_Min.y, zoom_now->_Max.y);
	// EAM Apr 2013 - The tests on VERYLARGE protect against trying to
	// interpret the autoscaling initial state as an actual limit value. 
	if(!is_3d_plot && (zoom_now->_2Min.x < VERYLARGE && zoom_now->_2Max.x > -VERYLARGE)) {
		SetExplicitRange(&AxS[SECOND_X_AXIS], zoom_now->_2Min.x, zoom_now->_2Max.x);
	}
	if(!is_3d_plot && (zoom_now->_2Min.y < VERYLARGE && zoom_now->_2Max.y > -VERYLARGE)) {
		SetExplicitRange(&AxS[SECOND_Y_AXIS], zoom_now->_2Min.y, zoom_now->_2Max.y);
	}
	/* EAM Jun 2007 - The autoscale save/restore was too complicated, and broke
	 * refresh. Just save/restore the complete axis state and have done with it.
	 * Well, not _quite_ the complete state.  The labels are maintained dynamically.
	 * Apr 2015 - The same is now true (dynamic storage) for ticfmt, formatstring.
	 */
	if(zoom_now == zoom_head) {
		for(int i = 0; i < AXIS_ARRAY_SIZE; i++) {
			axis_array_copy[i].label = AxS[i].label;
			axis_array_copy[i].ticdef.def.user = AxS[i].ticdef.def.user;
			axis_array_copy[i].ticdef.font = AxS[i].ticdef.font;
			axis_array_copy[i].ticfmt = AxS[i].ticfmt;
			axis_array_copy[i].formatstring = AxS[i].formatstring;
			AxS[i] = axis_array_copy[i]; // @sobolev
		}
		// @sobolev memcpy(axis_array, axis_array_copy, sizeof(axis_array));
		//
		// The shadowed primary axis, if any, is not restored by the memcpy.
		// We choose to recalculate the limits, but alternatively we could find
		// some place to save/restore the unzoomed limits.
		if(AxS[FIRST_X_AXIS].IsNonLinear())
			CloneLinkedAxes(&AxS[FIRST_X_AXIS], AxS[FIRST_X_AXIS].linked_to_primary);
		if(AxS[FIRST_Y_AXIS].IsNonLinear())
			CloneLinkedAxes(&AxS[FIRST_Y_AXIS], AxS[FIRST_Y_AXIS].linked_to_primary);
		// Falling through to GPO.DoStringReplot() does not work! 
		if(volatile_data) {
			if(refresh_ok == E_REFRESH_OK_2D) {
				RefreshRequest();
				return;
			}
			if(is_splot_map && (refresh_ok == E_REFRESH_OK_3D)) {
				RefreshRequest();
				return;
			}
		}
	}
	else
		inside_zoom = TRUE;
	DoStringReplot("");
	inside_zoom = FALSE;
}
// 
// makes a zoom: update zoom history, call gnuplot to set ranges + replot
// 
//static void do_zoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max)
void GnuPlot::DoZoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max)
{
	struct t_zoom * z;
	if(zoom_head == NULL) { // queue not yet created, thus make its head 
		zoom_head = (struct t_zoom *)gp_alloc(sizeof(struct t_zoom), "mouse zoom history head");
		zoom_head->prev = NULL;
		zoom_head->next = NULL;
	}
	if(zoom_now == NULL)
		zoom_now = zoom_head;
	if(zoom_now->next == NULL) {    /* allocate new item */
		z = (struct t_zoom *)gp_alloc(sizeof(struct t_zoom), "mouse zoom history element");
		z->prev = zoom_now;
		z->next = NULL;
		zoom_now->next = z;
		z->prev = zoom_now;
	}
	else                    /* overwrite next item */
		z = zoom_now->next;

//#define SET_AXIS(axis, name, minmax, condition) z->name ## minmax = (AxS[axis].minmax condition) ? name ## minmax : AxS[axis].minmax
	//SET_AXIS(FIRST_X_AXIS,  x,  min, < VERYLARGE);
	//SET_AXIS(FIRST_Y_AXIS,  y,  min, < VERYLARGE);
	//SET_AXIS(SECOND_X_AXIS, x2, min, < VERYLARGE);
	//SET_AXIS(SECOND_Y_AXIS, y2, min, < VERYLARGE);
	//
	//SET_AXIS(FIRST_X_AXIS,  x,  max, > -VERYLARGE);
	//SET_AXIS(FIRST_Y_AXIS,  y,  max, > -VERYLARGE);
	//SET_AXIS(SECOND_X_AXIS, x2, max, > -VERYLARGE);
	//SET_AXIS(SECOND_Y_AXIS, y2, max, > -VERYLARGE);
//#undef SET_AXIS
	z->_Min.x  = (AxS[FIRST_X_AXIS].min  < VERYLARGE)  ? xmin  : AxS[FIRST_X_AXIS].min;
	z->_Min.y  = (AxS[FIRST_Y_AXIS].min  < VERYLARGE)  ? ymin  : AxS[FIRST_Y_AXIS].min;
	z->_2Min.x = (AxS[SECOND_X_AXIS].min < VERYLARGE)  ? x2min : AxS[SECOND_X_AXIS].min;
	z->_2Min.y = (AxS[SECOND_Y_AXIS].min < VERYLARGE)  ? y2min : AxS[SECOND_Y_AXIS].min;
	z->_Max.x  = (AxS[FIRST_X_AXIS].max  > -VERYLARGE) ? xmax  : AxS[FIRST_X_AXIS].max;
	z->_Max.y  = (AxS[FIRST_Y_AXIS].max  > -VERYLARGE) ? ymax  : AxS[FIRST_Y_AXIS].max;
	z->_2Max.x = (AxS[SECOND_X_AXIS].max > -VERYLARGE) ? x2max : AxS[SECOND_X_AXIS].max;
	z->_2Max.y = (AxS[SECOND_Y_AXIS].max > -VERYLARGE) ? y2max : AxS[SECOND_Y_AXIS].max;
	ApplyZoom(z);
}

static void ZoomNext()
{
	if(zoom_now == NULL || zoom_now->next == NULL)
		alert();
	else
		GPO.ApplyZoom(zoom_now->next);
	if(display_ipc_commands()) {
		fprintf(stderr, "next zoom.\n");
	}
}

static void ZoomPrevious()
{
	if(zoom_now == NULL || zoom_now->prev == NULL)
		alert();
	else
		GPO.ApplyZoom(zoom_now->prev);
	if(display_ipc_commands()) {
		fprintf(stderr, "previous zoom.\n");
	}
}

static void ZoomUnzoom()
{
	if(zoom_head == NULL || zoom_now == zoom_head)
		alert();
	else
		GPO.ApplyZoom(zoom_head);
	if(display_ipc_commands()) {
		fprintf(stderr, "unzoom.\n");
	}
}

static void incr_mousemode(const int amount)
{
	long int old = mouse_mode;
	bool found_a_new_one = FALSE;
	mouse_mode += amount;
	while(!found_a_new_one) {
		if(MOUSE_COORDINATES_ALT == mouse_mode && !(mouse_alt_string || polar))
			mouse_mode += amount; /* stepping over */
		else if(MOUSE_COORDINATES_FUNCTION == mouse_mode && mouse_readout_function.at == NULL)
			mouse_mode += amount; /* stepping over */
		else if(mouse_mode > MOUSE_COORDINATES_FUNCTION)
			mouse_mode = MOUSE_COORDINATES_REAL1;
		else if(mouse_mode <= MOUSE_COORDINATES_REAL)
			mouse_mode = MOUSE_COORDINATES_FUNCTION;
		else
			found_a_new_one = TRUE;
	}
	GPO.UpdateStatusLine();
	if(display_ipc_commands())
		fprintf(stderr, "switched mouse format from %ld to %ld\n", old, mouse_mode);
}

#define TICS_ON(ti) (((ti)&TICS_MASK)!=NO_TICS)

//void UpdateStatusline()
void GnuPlot::UpdateStatusLine()
{
	UpdateStatusLineWithMouseSetting(term, &mouse_setting);
}

//static void UpdateStatuslineWithMouseSetting(mouse_setting_t * ms)
void GnuPlot::UpdateStatusLineWithMouseSetting(termentry * pTerm, mouse_setting_t * ms)
{
	char s0[256], * sp;
	s0[0] = 0;
	// This suppresses mouse coordinate update after a ^C 
	if(term_initialised && ms->on) {
		if(!ALMOST2D) {
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
		else if(!TICS_ON(AxS[SECOND_X_AXIS].ticmode) && !TICS_ON(AxS[SECOND_Y_AXIS].ticmode)) {
			// only first X and Y axis are in use 
			sp = GetAnnotateString(s0, real_x, real_y, mouse_mode, mouse_alt_string);
			if(ruler.on)
				GetRulerString(sp, real_x, real_y);
		}
		else {
			// X2 and/or Y2 are in use: use more verbose format 
			sp = s0;
			if(TICS_ON(AxS[FIRST_X_AXIS].ticmode)) {
				sp = stpcpy(sp, "x=");
				sp = mkstr(sp, real_x, FIRST_X_AXIS);
				*sp++ = ' ';
			}
			if(TICS_ON(AxS[FIRST_Y_AXIS].ticmode)) {
				sp = stpcpy(sp, "y=");
				sp = mkstr(sp, real_y, FIRST_Y_AXIS);
				*sp++ = ' ';
			}
			if(TICS_ON(AxS[SECOND_X_AXIS].ticmode)) {
				sp = stpcpy(sp, "x2=");
				sp = mkstr(sp, real_x2, SECOND_X_AXIS);
				*sp++ = ' ';
			}
			if(TICS_ON(AxS[SECOND_Y_AXIS].ticmode)) {
				sp = stpcpy(sp, "y2=");
				sp = mkstr(sp, real_y2, SECOND_Y_AXIS);
				*sp++ = ' ';
			}
			if(ruler.on) {
				// ruler on? then also print distances to ruler 
				if(TICS_ON(AxS[FIRST_X_AXIS].ticmode)) {
					stpcpy(sp, "dx=");
					sprintf(sp+3, mouse_setting.fmt, DIST(real_x, ruler.x, FIRST_X_AXIS));
					sp += strlen(sp);
				}
				if(TICS_ON(AxS[FIRST_Y_AXIS].ticmode)) {
					stpcpy(sp, "dy=");
					sprintf(sp+3, mouse_setting.fmt, DIST(real_y, ruler.y, FIRST_Y_AXIS));
					sp += strlen(sp);
				}
				if(TICS_ON(AxS[SECOND_X_AXIS].ticmode)) {
					stpcpy(sp, "dx2=");
					sprintf(sp+4, mouse_setting.fmt, DIST(real_x2, ruler.x2, SECOND_X_AXIS));
					sp += strlen(sp);
				}
				if(TICS_ON(AxS[SECOND_Y_AXIS].ticmode)) {
					stpcpy(sp, "dy2=");
					sprintf(sp+4, mouse_setting.fmt, DIST(real_y2, ruler.y2, SECOND_Y_AXIS));
					sp += strlen(sp);
				}
			}
			*--sp = 0;      /* delete trailing space */
		}
		if(pTerm->put_tmptext && *s0)
			(pTerm->put_tmptext)(0, s0);
	}
}

void recalc_statusline()
{
	GPO.MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
	GPO.UpdateStatusLine();
}

/****************** handlers for user's actions ******************/

static char * builtin_autoscale(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-autoscale` (set autoscale keepfix; replot)";
	}
	GPO.DoStringReplot("set autoscale keepfix");
	return (char*)0;
}

static char * builtin_toggle_border(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-toggle-border`";

	/* EAM July 2009  Cycle through border settings
	 * - no border
	 * - last border requested by the user
	 * - default border
	 * - (3D only) full border
	 */
	if(draw_border == 0 && draw_border != user_border)
		draw_border = user_border;
	else if(draw_border == user_border && draw_border != 31)
		draw_border = 31;
	else if(is_3d_plot && draw_border == 31)
		draw_border = 4095;
	else
		draw_border = 0;
	GPO.DoStringReplot("");
	return (char*)0;
}

static char * builtin_replot(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-replot`";
	}
	GPO.DoStringReplot("");
	return (char*)0;
}

static char * builtin_toggle_grid(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-toggle-grid`";
	}
	if(!some_grid_selected())
		GPO.DoStringReplot("set grid");
	else
		GPO.DoStringReplot("unset grid");
	return (char*)0;
}

static char * builtin_help(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-help`";
	}
	fprintf(stderr, "\n");
	bind_display((char*)0); /* display all bindings */
	restore_prompt();
	return (char*)0;
}

static char * builtin_set_plots_visible(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-set-plots-visible`";
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_SET_VISIBLE, -1);
	return (char*)0;
}

static char * builtin_set_plots_invisible(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-set-plots-invisible`";
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_SET_INVISIBLE, -1);
	return (char*)0;
}

static char * builtin_invert_plot_visibilities(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-invert-plot-visibilities`";
	}
	if(term->modify_plots)
		term->modify_plots(MODPLOTS_INVERT_VISIBILITIES, -1);
	return (char*)0;
}

static char * builtin_toggle_log(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-toggle-log` y logscale for plots, z and cb for splots";
	if(volatile_data)
		GPO.IntWarn(NO_CARET, "Cannot toggle log scale for volatile data");
	else if((color_box.bounds.xleft < mouse_x && mouse_x < color_box.bounds.xright) && (color_box.bounds.ybot  < mouse_y && mouse_y < color_box.bounds.ytop))
		GPO.DoStringReplot(GPO.AxS.__CB().log ? "unset log cb" : "set log cb");
	else if(is_3d_plot && !splot_map)
		GPO.DoStringReplot(GPO.AxS.__Z().log ? "unset log z" : "set log z");
	else
		GPO.DoStringReplot(GPO.AxS[FIRST_Y_AXIS].log ? "unset log y" : "set log y");
	return (char*)0;
}

static char * builtin_nearest_log(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-nearest-log` toggle logscale of axis nearest cursor";
	if((color_box.bounds.xleft < mouse_x && mouse_x < color_box.bounds.xright) && (color_box.bounds.ybot  < mouse_y && mouse_y < color_box.bounds.ytop)) {
		GPO.DoStringReplot(GPO.AxS.__CB().log ? "unset log cb" : "set log cb");
	}
	else if(is_3d_plot && !splot_map) {
		GPO.DoStringReplot(GPO.AxS.__Z().log ? "unset log z" : "set log z");
	}
	else {
		/* 2D-plot: figure out which axis/axes is/are
		 * close to the mouse cursor, and toggle those lin/log */
		/* note: here it is assumed that the x axis is at
		 * the bottom, x2 at top, y left and y2 right; it
		 * would be better to derive that from the ..tics settings */
		bool change_x1 = FALSE;
		bool change_y1 = FALSE;
		bool change_x2 = FALSE;
		bool change_y2 = FALSE;
		if(mouse_y < GPO.V.BbPlot.ybot + (GPO.V.BbPlot.ytop - GPO.V.BbPlot.ybot) / 4 &&  mouse_x > GPO.V.BbPlot.xleft && mouse_x < GPO.V.BbPlot.xright)
			change_x1 = TRUE;
		if(mouse_x < GPO.V.BbPlot.xleft + (GPO.V.BbPlot.xright - GPO.V.BbPlot.xleft) / 4 &&  mouse_y > GPO.V.BbPlot.ybot && mouse_y < GPO.V.BbPlot.ytop)
			change_y1 = TRUE;
		if(mouse_y > GPO.V.BbPlot.ytop - (GPO.V.BbPlot.ytop - GPO.V.BbPlot.ybot) / 4 &&  mouse_x > GPO.V.BbPlot.xleft && mouse_x < GPO.V.BbPlot.xright)
			change_x2 = TRUE;
		if(mouse_x > GPO.V.BbPlot.xright - (GPO.V.BbPlot.xright - GPO.V.BbPlot.xleft) / 4 &&  mouse_y > GPO.V.BbPlot.ybot && mouse_y < GPO.V.BbPlot.ytop)
			change_y2 = TRUE;
		if(change_x1)
			do_string(GPO.AxS[FIRST_X_AXIS].log ? "unset log x" : "set log x");
		if(change_y1)
			do_string(GPO.AxS[FIRST_Y_AXIS].log ? "unset log y" : "set log y");
		if(change_x2 && !splot_map)
			do_string(GPO.AxS[SECOND_X_AXIS].log ? "unset log x2" : "set log x2");
		if(change_y2 && !splot_map)
			do_string(GPO.AxS[SECOND_Y_AXIS].log ? "unset log y2" : "set log y2");
		if(!change_x1 && !change_y1 && splot_map)
			GPO.DoStringReplot(GPO.AxS.__Z().log ? "unset log z" : "set log z");
		if(change_x1 || change_y1 || change_x2 || change_y2)
			GPO.DoStringReplot("");
	}
	return (char*)0;
}

static char * builtin_toggle_mouse(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-toggle-mouse`";
	}
	if(!mouse_setting.on) {
		mouse_setting.on = 1;
		if(display_ipc_commands()) {
			fprintf(stderr, "turning mouse on.\n");
		}
	}
	else {
		mouse_setting.on = 0;
		if(display_ipc_commands()) {
			fprintf(stderr, "turning mouse off.\n");
		}
	}
	if(term->set_cursor)
		term->set_cursor(0, 0, 0);
	GPO.UpdateStatusLine();
	return (char*)0;
}

static char * builtin_toggle_ruler(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-toggle-ruler`";
	}
	if(!term->set_ruler)
		return (char*)0;
	if(ruler.on) {
		turn_ruler_off();
		if(display_ipc_commands())
			fprintf(stderr, "turning ruler off.\n");
	}
	else if(ALMOST2D) {
		// only allow ruler, if the plot is 2d or a 3d `map' 
		udvt_entry * u;
		ruler.on = TRUE;
		ruler.px = ge->mx;
		ruler.py = ge->my;
		GPO.MousePosToGraphPosReal(ruler.px, ruler.py, &ruler.x, &ruler.y, &ruler.x2, &ruler.y2);
		(*term->set_ruler)(ruler.px, ruler.py);
		if((u = GPO.Ev.AddUdvByName("MOUSE_RULER_X"))) {
			Gcomplex(&u->udv_value, ruler.x, 0);
		}
		if((u = GPO.Ev.AddUdvByName("MOUSE_RULER_Y"))) {
			Gcomplex(&u->udv_value, ruler.y, 0);
		}
		if(display_ipc_commands()) {
			fprintf(stderr, "turning ruler on.\n");
		}
	}
	GPO.UpdateStatusLine();
	return (char*)0;
}

static char * builtin_decrement_mousemode(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-previous-mouse-format`";
	}
	incr_mousemode(-1);
	return (char*)0;
}

static char * builtin_increment_mousemode(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-next-mouse-format`";
	}
	incr_mousemode(1);
	return (char*)0;
}

static char * builtin_toggle_polardistance(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-toggle-polardistance`";
	}
	if(++mouse_setting.polardistance > 2) 
		mouse_setting.polardistance = 0;
	// values: 0 (no polar coordinates), 1 (polar coordinates), 2 (tangent instead of angle) 
	term->set_cursor((mouse_setting.polardistance ? -3 : -4), ge->mx, ge->my); /* change cursor type */
	GPO.UpdateStatusLine();
	if(display_ipc_commands()) {
		fprintf(stderr, "distance to ruler will %s be shown in polar coordinates.\n", mouse_setting.polardistance ? "" : "not");
	}
	return (char*)0;
}

static char * builtin_toggle_verbose(struct gp_event_t * ge)
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

static char * builtin_toggle_ratio(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-toggle-ratio`";
	else {
		if(GPO.V.AspectRatio == 0.0f)
			GPO.DoStringReplot("set size ratio -1");
		else if(GPO.V.AspectRatio == 1.0f)
			GPO.DoStringReplot("set size nosquare");
		else
			GPO.DoStringReplot("set size square");
		return (char*)0;
	}
}

static char * builtin_zoom_next(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-next` go to next zoom in the zoom stack";
	else {
		ZoomNext();
		return (char*)0;
	}
}

static char * builtin_zoom_previous(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-previous` go to previous zoom in the zoom stack";
	else {
		ZoomPrevious();
		return (char*)0;
	}
}

static char * builtin_unzoom(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-unzoom`";
	}
	ZoomUnzoom();
	return (char*)0;
}

static char * builtin_rotate_right(struct gp_event_t * ge)
{
	if(!ge)
		return "`scroll right in 2d, rotate right in 3d`; <Shift> faster";
	if(is_3d_plot)
		ChangeView(0, -1);
	else {
		int k = (modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_right();
	}
	return (char*)0;
}

static char * builtin_rotate_left(struct gp_event_t * ge)
{
	if(!ge)
		return "`scroll left in 2d, rotate left in 3d`; <Shift> faster";
	if(is_3d_plot)
		ChangeView(0, 1);
	else {
		int k = (modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_left();
	}
	return (char*)0;
}

static char * builtin_rotate_up(struct gp_event_t * ge)
{
	if(!ge)
		return "`scroll up in 2d, rotate up in 3d`; <Shift> faster";
	if(is_3d_plot)
		ChangeView(1, 0);
	else {
		int k = (modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_up();
	}
	return (char*)0;
}

static char * builtin_rotate_down(struct gp_event_t * ge)
{
	if(!ge)
		return "`scroll down in 2d, rotate down in 3d`; <Shift> faster";
	if(is_3d_plot)
		ChangeView(-1, 0);
	else {
		int k = (modifier_mask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			do_zoom_scroll_down();
	}
	return (char*)0;
}

static char * builtin_azimuth_left(struct gp_event_t * ge)
{
	if(!ge)
		return "`rotate azimuth left in 3d`; <ctrl> faster";
	if(is_3d_plot)
		ChangeAzimuth(-1);
	return (char*)0;
}

static char * builtin_azimuth_right(struct gp_event_t * ge)
{
	if(!ge)
		return "`rotate azimuth right in 3d`; <ctrl> faster";
	if(is_3d_plot)
		ChangeAzimuth(1);
	return (char*)0;
}

static char * builtin_cancel_zoom(struct gp_event_t * ge)
{
	if(!ge) {
		return "`builtin-cancel-zoom` cancel zoom region";
	}
	if(!setting_zoom_region)
		return (char*)0;
	if(term->set_cursor)
		term->set_cursor(0, 0, 0);
	setting_zoom_region = FALSE;
	if(display_ipc_commands()) {
		fprintf(stderr, "zooming cancelled.\n");
	}
	return (char*)0;
}

/* Check whether this event is bound to a command.
 * If so return a pointer to the binding, otherwise return NULL.
 */
static bind_t * get_binding(struct gp_event_t * ge, bool current)
{
	int c, par2;
	bind_t * ptr;
	bind_t keypress;
	if(ge->type == GE_buttonpress || ge->type == GE_buttonrelease) {
		int b = ge->par1;
		c = (b == 3) ? GP_Button3 : (b == 2) ? GP_Button2 : GP_Button1;
		par2 = 0;
	}
	else {
		c = ge->par1;
		if((modifier_mask & Mod_Shift) && ((c & 0xff) == 0))
			c = toupper(c);
		par2 = ge->par2;
	}
	if(!bindings)
		bind_install_default_bindings();
	bind_clear(&keypress);
	keypress.key = c;
	keypress.modifier = modifier_mask;
	for(ptr = bindings; ptr; ptr = ptr->next) {
		if(bind_matches(&keypress, ptr)) {
			/* Always honor keys set with "bind all" */
			if(ptr->allwindows && ptr->command)
				return ptr;
			/* But otherwise ignore inactive windows */
			else if(!current)
				break;
			/* Let user defined bindings overwrite the builtin bindings */
			else if((par2 & 1) == 0 && ptr->command)
				return ptr;
			else if(ptr->builtin)
				return ptr;
			else
				FPRINTF((stderr, "%s:%d protocol error\n", __FILE__, __LINE__));
		}
	}
	return NULL;
}

static void event_keypress(struct gp_event_t * ge, bool current)
{
	int x, y;
	int par2;
	bind_t * ptr;
	bind_t keypress;
	udvt_entry * keywin;
	int c = ge->par1;
	if((modifier_mask & Mod_Shift) && ((c & 0xff) == 0))
		c = toupper(c);
	par2 = ge->par2;
	x = ge->mx;
	y = ge->my;
	bind_clear(&keypress);
	keypress.key = c;
	keypress.modifier = modifier_mask;
	/*
	 * On 'pause mouse keypress' in active window export current keypress
	 * and mouse coords to user variables. A key with 'bind all' terminates
	 * a pause even from non-active windows.
	 * Ignore NULL keypress.
	 *
	 * If we are paused for a keystroke, this takes precendence over normal
	 * key bindings. Otherwise, for example typing 'm' would turn off mousing,
	 * which is a bad thing if you are in the  middle of a mousing operation.
	 */
	if((paused_for_mouse & PAUSE_KEYSTROKE) && (c > '\0') && current) {
		GPO.LoadMouseVariables(x, y, FALSE, c);
		return;
	}
	if(!(ptr = get_binding(ge, current)))
		return;
	if((keywin = GPO.Ev.AddUdvByName("MOUSE_KEY_WINDOW")))
		Ginteger(&keywin->udv_value, ge->winid);
	if(current)
		GPO.LoadMouseVariables(x, y, FALSE, c);
	else
		GPO.LoadMouseVariables(0, 0, FALSE, c);
	if(ptr->allwindows && ptr->command)
		do_string(ptr->command);
	else if((par2 & 1) == 0 && ptr->command)
		do_string(ptr->command);
	else if(ptr->builtin)
		ptr->builtin(ge);
}

static void ChangeView(int x, int z)
{
	if(modifier_mask & Mod_Shift) {
		x *= 10;
		z *= 10;
	}
	if(x) {
		surface_rot_x += x;
		if(surface_rot_x < 0.0f)
			surface_rot_x += 360.0f;
		if(surface_rot_x > 360.0f)
			surface_rot_x -= 360.0f;
	}
	if(z) {
		surface_rot_z += z;
		if(surface_rot_z < 0.0f)
			surface_rot_z += 360.0f;
		if(surface_rot_z > 360.0f)
			surface_rot_z -= 360.0f;
	}
	if(x || z) {
		GPO.Ev.FillGpValFoat("GPVAL_VIEW_ROT_X", surface_rot_x);
		GPO.Ev.FillGpValFoat("GPVAL_VIEW_ROT_Z", surface_rot_z);
	}
	if(display_ipc_commands()) {
		fprintf(stderr, "changing view to %f, %f.\n", surface_rot_x, surface_rot_z);
	}
	GPO.DoSave3DPlot(term, first_3dplot, plot3d_num, NORMAL_REPLOT);
	if(ALMOST2D) {
		// 2D plot, or suitably aligned 3D plot: update statusline 
		if(!term->put_tmptext)
			return;
		recalc_statusline();
	}
}

static void ChangeAzimuth(int x)
{
	/* Disable for 2D projections */
	if(xz_projection || yz_projection)
		return;
	/* Can't use Mod_Shift because keyboards differ on the */
	/* shift status of the < and > keys. */
	if(modifier_mask & Mod_Ctrl)
		x *= 10;
	if(x) {
		azimuth += x;
		if(azimuth < 0)
			azimuth += 360;
		if(azimuth > 360)
			azimuth -= 360;
		GPO.Ev.FillGpValFoat("GPVAL_VIEW_AZIMUTH", azimuth);
	}
	if(display_ipc_commands())
		fprintf(stderr, "changing azimuth to %f.\n", azimuth);
	GPO.DoSave3DPlot(term, first_3dplot, plot3d_num, NORMAL_REPLOT);
}

int is_mouse_outside_plot(void)
{
#define CHECK_AXIS_OUTSIDE(real, axis) (GPO.AxS[axis].min <  VERYLARGE && GPO.AxS[axis].max > -VERYLARGE && \
	((real < GPO.AxS[axis].min && real < GPO.AxS[axis].max) || (real > GPO.AxS[axis].min && real > GPO.AxS[axis].max)))

	return CHECK_AXIS_OUTSIDE(real_x,  FIRST_X_AXIS) || CHECK_AXIS_OUTSIDE(real_y,  FIRST_Y_AXIS) ||
		CHECK_AXIS_OUTSIDE(real_x2, SECOND_X_AXIS) || CHECK_AXIS_OUTSIDE(real_y2, SECOND_Y_AXIS);
#undef CHECK_AXIS_OUTSIDE
}
//
// Return a new upper or lower axis limit that is a linear
// combination of the current limits.
//
//static double rescale(int axIdx, double w1, double w2)
double GnuPlot::Rescale(int axIdx, double w1, double w2)
{
	double newlimit;
	GpAxis * p_ax = &AxS[axIdx];
	double axmin = p_ax->min;
	double axmax = p_ax->max;
	if(p_ax->IsNonLinear()) {
		axmin = EvalLinkFunction(p_ax->linked_to_primary, axmin);
		axmax = EvalLinkFunction(p_ax->linked_to_primary, axmax);
	}
	newlimit = w1*axmin + w2*axmax;
	if(p_ax->IsNonLinear())
		newlimit = EvalLinkFunction(p_ax->linked_to_primary->linked_to_secondary, newlimit);
	return newlimit;
}

// Rescale axes and do zoom. 
//static void zoom_rescale_xyx2y2(double a0, double a1, double a2, double a3, double a4, double a5, double a6,
    //double a7, double a8, double a9, double a10, double a11, double a12, double a13, double a14, double a15, char msg[])
void GnuPlot::ZoomRescale_XYX2Y2(double a0, double a1, double a2, double a3, double a4, double a5, double a6,
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
	retain_offsets = TRUE;
	DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	if(msg[0] && display_ipc_commands()) {
		fputs(msg, stderr); fputs("\n", stderr);
	}
}

/* Scroll left. */
static void do_zoom_scroll_left()
{
	GPO.ZoomRescale_XYX2Y2(1.1, -0.1,
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
	GPO.ZoomRescale_XYX2Y2(0.9,  0.1,
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
	GPO.ZoomRescale_XYX2Y2(1,    0,
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
	GPO.ZoomRescale_XYX2Y2(1,   0,
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
// Return new lower and upper axis limits from expanding current limits
// relative to current mouse position.
//
//static void rescale_around_mouse(double * pNewMin, double * pNewMax, int axIdx, double mouse_pos, double scale)
void GnuPlot::RescaleAroundMouse(double * pNewMin, double * pNewMax, int axIdx, double mouse_pos, double scale)
{
	GpAxis * p_ax = &AxS[axIdx];
	GpAxis * p_ax_primary = p_ax->linked_to_primary;
	double axmin = p_ax->min;
	double axmax = p_ax->max;
	if(p_ax->IsNonLinear()) {
		axmin = EvalLinkFunction(p_ax_primary, axmin);
		axmax = EvalLinkFunction(p_ax_primary, axmax);
		mouse_pos = EvalLinkFunction(p_ax_primary, mouse_pos);
	}
	*pNewMin = mouse_pos + (axmin - mouse_pos) * scale;
	*pNewMax = mouse_pos + (axmax - mouse_pos) * scale;
	if(p_ax->IsNonLinear()) {
		*pNewMin = EvalLinkFunction(p_ax_primary->linked_to_secondary, *pNewMin);
		*pNewMax = EvalLinkFunction(p_ax_primary->linked_to_secondary, *pNewMax);
	}
}
//
// Zoom in/out within x-axis. 
//
//static void zoom_in_X(int zoom_key)
void GnuPlot::ZoomInX(int zoomKey)
{
	retain_offsets = TRUE;
	if(is_mouse_outside_plot()) {
		/* zoom in (X axis only) */
		double w1 = (zoomKey == '+') ? 23./25. : 23./21.;
		double w2 = (zoomKey == '+') ?  2./25. : -2./21.;
		ZoomRescale_XYX2Y2(w1, w2, 1, 0, w1, w2, 1, 0,  w2, w1, 0, 1, w2, w1, 0, 1, ((zoomKey == '+') ? "zoom in X" : "zoom out X"));
	}
	else {
		double xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max;
		double scale = (zoomKey == '+') ? 0.75 : 1.25;
		RescaleAroundMouse(&xmin,  &xmax,  FIRST_X_AXIS,  real_x,  scale);
		RescaleAroundMouse(&x2min, &x2max, SECOND_X_AXIS, real_x2, scale);
		ymin  = Rescale(FIRST_Y_AXIS,  1, 0);
		y2min = Rescale(SECOND_Y_AXIS, 1, 0);
		ymax  = Rescale(FIRST_Y_AXIS,  0, 1);
		y2max = Rescale(SECOND_Y_AXIS, 0, 1);
		DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	}
}

static void do_zoom_in_X() { GPO.ZoomInX('+'); }
static void do_zoom_out_X() { GPO.ZoomInX('-'); }

/* Zoom around mouse cursor unless the cursor is outside the graph boundary,
   when it scales around the graph center.
   Syntax: zoom_key == '+' ... zoom in, zoom_key == '-' ... zoom out
 */
static void zoom_around_mouse(int zoom_key)
{
	double xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max;
	if(is_mouse_outside_plot()) {
		// zoom in (factor of approximately 2^(.25), so four steps gives 2x larger) 
		double w1 = (zoom_key=='+') ? 23./25. : 23./21.;
		double w2 = (zoom_key=='+') ?  2./25. : -2./21.;
		xmin  = GPO.Rescale(FIRST_X_AXIS,  w1, w2);
		ymin  = GPO.Rescale(FIRST_Y_AXIS,  w1, w2);
		x2min = GPO.Rescale(SECOND_X_AXIS, w1, w2);
		y2min = GPO.Rescale(SECOND_Y_AXIS, w1, w2);
		xmax  = GPO.Rescale(FIRST_X_AXIS,  w2, w1);
		ymax  = GPO.Rescale(FIRST_Y_AXIS,  w2, w1);
		x2max = GPO.Rescale(SECOND_X_AXIS, w2, w1);
		y2max = GPO.Rescale(SECOND_Y_AXIS, w2, w1);
	}
	else {
		int zsign = (zoom_key=='+') ? -1 : 1;
		double xscale = pow(1.25, zsign * mouse_setting.xmzoom_factor);
		double yscale = pow(1.25, zsign * mouse_setting.ymzoom_factor);
		// {x,y}zoom_factor = 0: not zoom, = 1: 0.8/1.25 zoom 
		GPO.RescaleAroundMouse(&xmin,  &xmax,  FIRST_X_AXIS,  real_x,  xscale);
		GPO.RescaleAroundMouse(&ymin,  &ymax,  FIRST_Y_AXIS,  real_y,  yscale);
		GPO.RescaleAroundMouse(&x2min, &x2max, SECOND_X_AXIS, real_x2, xscale);
		GPO.RescaleAroundMouse(&y2min, &y2max, SECOND_Y_AXIS, real_y2, yscale);
	}
	retain_offsets = TRUE;
	GPO.DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	if(display_ipc_commands())
		fprintf(stderr, "zoom %s.\n", (zoom_key=='+' ? "in" : "out"));
}

static void do_zoom_in_around_mouse() { zoom_around_mouse('+'); }
static void do_zoom_out_around_mouse() { zoom_around_mouse('-'); }

static char * builtin_zoom_in_around_mouse(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-in` zoom in";
	do_zoom_in_around_mouse();
	return (char*)0;
}

static char * builtin_zoom_out_around_mouse(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-out` zoom out";
	do_zoom_out_around_mouse();
	return (char*)0;
}

#if (0) /* Not currently used */
static char * builtin_zoom_scroll_left(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-left` scroll left";
	do_zoom_scroll_left();
	return (char*)0;
}

static char * builtin_zoom_scroll_right(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-right` scroll right";
	do_zoom_scroll_right();
	return (char*)0;
}

static char * builtin_zoom_scroll_up(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-up` scroll up";
	do_zoom_scroll_up();
	return (char*)0;
}

static char * builtin_zoom_scroll_down(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-scroll-down` scroll down";
	do_zoom_scroll_down();
	return (char*)0;
}

static char * builtin_zoom_in_X(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-in-X` zoom in X axis";
	do_zoom_in_X();
	return (char*)0;
}

static char * builtin_zoom_out_X(struct gp_event_t * ge)
{
	if(!ge)
		return "`builtin-zoom-out-X` zoom out X axis";
	do_zoom_out_X();
	return (char*)0;
}

#endif /* Not currently used */

static void event_buttonpress(gp_event_t * ge)
{
	int b;
	motion = 0;
	b = ge->par1;
	mouse_x = ge->mx;
	mouse_y = ge->my;
	button |= (1 << b);
	FPRINTF((stderr, "(event_buttonpress) mouse_x = %d\tmouse_y = %d\n", mouse_x, mouse_y));
	GPO.MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
	if((b == 4 || b == 6) && /* 4 - wheel up, 6 - wheel left */ (!replot_disabled || (E_REFRESH_NOT_OK != refresh_ok)) /* Use refresh if available */
	    && !(paused_for_mouse & PAUSE_BUTTON3)) {
		/* Ctrl+Shift+wheel up or Squeeze (not implemented) */
		if((modifier_mask & Mod_Ctrl) && (modifier_mask & Mod_Shift))
			do_zoom_in_X();
		/* Ctrl+wheel up or Ctrl+stroke */
		else if((modifier_mask & Mod_Ctrl))
			do_zoom_in_around_mouse();
		/* Horizontal stroke (button 6) or Shift+wheel up */
		else if(b == 6 || (modifier_mask & Mod_Shift))
			do_zoom_scroll_left();
		/* Wheel up (no modifier keys) */
		else
			do_zoom_scroll_up();
	}
	else if(((b == 5) || (b == 7)) && /* 5 - wheel down, 7 - wheel right */
	    (!replot_disabled || (E_REFRESH_NOT_OK != refresh_ok))      /* Use refresh if available */
	    && !(paused_for_mouse & PAUSE_BUTTON3)) {
		/* Ctrl+Shift+wheel down or Unsqueeze (not implemented) */
		if((modifier_mask & Mod_Ctrl) && (modifier_mask & Mod_Shift))
			do_zoom_out_X();

		/* Ctrl+wheel down or Ctrl+stroke */
		else if((modifier_mask & Mod_Ctrl))
			do_zoom_out_around_mouse();
		/* Horizontal stroke (button 7) or Shift+wheel down */
		else if(b == 7 || (modifier_mask & Mod_Shift))
			do_zoom_scroll_right();
		/* Wheel down (no modifier keys) */
		else
			do_zoom_scroll_down();
	}
	else if(ALMOST2D) {
		// "pause button1" or "pause any" takes precedence over key bindings 
		if(1 == b) {
			if(paused_for_mouse & PAUSE_BUTTON1) {
				GPO.LoadMouseVariables(mouse_x, mouse_y, TRUE, b);
				trap_release = TRUE; /* Don't trigger on release also */
				return;
			}
		}
		// In 2D mouse buttons 1-3 are available for "bind" commands 
		if(oneof3(b, 1, 2, 3)) {
			if(get_binding(ge, TRUE)) {
				event_keypress(ge, TRUE);
				return;
			}
		}
		if(!setting_zoom_region) {
			if(3 == b && (!replot_disabled || (E_REFRESH_NOT_OK != refresh_ok)) /* Use refresh if available */ && !(paused_for_mouse & PAUSE_BUTTON3)) {
				/* start zoom; but ignore it when
				 *   - replot is disabled, e.g. with inline data, or
				 *   - during 'pause mouse'
				 * allow zooming during 'pause mouse key' */
				setting_zoom_x = mouse_x;
				setting_zoom_y = mouse_y;
				setting_zoom_region = TRUE;
				if(term->set_cursor) {
					int mv_mouse_x, mv_mouse_y;
					if(mouse_setting.annotate_zoom_box && term->put_tmptext) {
						double real_x, real_y, real_x2, real_y2;
						char s[64];
						// tell driver annotations 
						GPO.MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
						sprintf(s, zoombox_format(), real_x, real_y);
						term->put_tmptext(1, s);
						term->put_tmptext(2, s);
					}
					/* displace mouse in order not to start with an empty zoom box */
					mv_mouse_x = term->MaxX / 20;
					mv_mouse_y = (term->MaxX == term->MaxY) ? mv_mouse_x : (int)((mv_mouse_x * (double)term->MaxY) / term->MaxX);
					mv_mouse_x += mouse_x;
					mv_mouse_y += mouse_y;
					/* change cursor type */
					term->set_cursor(3, 0, 0);
					/* warp pointer */
					if(mouse_setting.warp_pointer)
						term->set_cursor(-2, mv_mouse_x, mv_mouse_y);
					/* turn on the zoom box */
					term->set_cursor(-1, setting_zoom_x, setting_zoom_y);
				}
				if(display_ipc_commands()) {
					fprintf(stderr, "starting zoom region.\n");
				}
			}
		}
		else {
			/* complete zoom (any button finishes zooming) */

			/* the following variables are used to check if the box
			 * is big enough to be considered as zoom box.
			 */
			int dist_x = setting_zoom_x - mouse_x;
			int dist_y = setting_zoom_y - mouse_y;
			int dist = static_cast<int>(sqrt((double)(dist_x * dist_x + dist_y * dist_y)));
			if(1 == b || 2 == b) {
				/* zoom region is finished by the `wrong' button.
				 * `trap' the next button-release event so that
				 * it won't trigger the actions which are bound
				 * to these events.
				 */
				trap_release = TRUE;
			}
			if(term->set_cursor) {
				term->set_cursor(0, 0, 0);
				if(mouse_setting.annotate_zoom_box && term->put_tmptext) {
					term->put_tmptext(1, "");
					term->put_tmptext(2, "");
				}
			}
			if(dist > 10 /* more ore less arbitrary */) {
				double xmin, ymin, x2min, y2min;
				double xmax, ymax, x2max, y2max;
				GPO.MousePosToGraphPosReal(setting_zoom_x, setting_zoom_y, &xmin, &ymin, &x2min, &y2min);
				xmax = real_x;
				x2max = real_x2;
				ymax = real_y;
				y2max = real_y2;
				// keep the axes (no)reversed as they are now 
#define rev(a1, a2, A) if(sgn(a2-a1) != sgn(GPO.AxS[A].max-GPO.AxS[A].min)) { double tmp = a1; a1 = a2; a2 = tmp; }
				rev(xmin,  xmax,  FIRST_X_AXIS);
				rev(ymin,  ymax,  FIRST_Y_AXIS);
				rev(x2min, x2max, SECOND_X_AXIS);
				rev(y2min, y2max, SECOND_Y_AXIS);
#undef rev
				GPO.DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
				if(display_ipc_commands()) {
					fprintf(stderr, "zoom region finished.\n");
				}
			}
			else {
				/* silently ignore a tiny zoom box. This might
				 * happen, if the user starts and finishes the
				 * zoom box at the same position. */
			}
			setting_zoom_region = FALSE;
		}
	}
	else {
		if(term->set_cursor) {
			if(button & (1 << 1) || button & (1 << 3))
				term->set_cursor(1, 0, 0);
			else if(button & (1 << 2))
				term->set_cursor(2, 0, 0);
		}
	}
	start_x = mouse_x;
	start_y = mouse_y;
	zero_rot_z = surface_rot_z + (360.0f * mouse_x) / term->MaxX;
	// zero_rot_x = surface_rot_x - 180.0 * mouse_y / term->MaxY; 
	zero_rot_x = surface_rot_x - (360.0f * mouse_y) / term->MaxY;
}

static void event_buttonrelease(gp_event_t * ge)
{
	int doubleclick;
	int b = ge->par1;
	mouse_x = ge->mx;
	mouse_y = ge->my;
	doubleclick = ge->par2;
	button &= ~(1 << b);    /* remove button */
	if(setting_zoom_region)
		return;
	// FIXME:  This mechanism may no longer be needed 
	if(TRUE == trap_release) {
		trap_release = FALSE;
		return;
	}
	// binding takes precedence over default action 
	if(b == 1 || b == 2 || b == 3) {
		if(get_binding(ge, TRUE))
			return;
	}
	GPO.MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
	FPRINTF((stderr, "MOUSE.C: doublclick=%i, set=%i, motion=%i, ALMOST2D=%i\n", (int)doubleclick, (int)mouse_setting.doubleclick, (int)motion, (int)ALMOST2D));
	if(ALMOST2D) {
		char s0[256];
		if(b == 1 && term->set_clipboard && ((doubleclick <= mouse_setting.doubleclick) || !mouse_setting.doubleclick)) {
			// put coordinates to clipboard. For 3d plots this takes
			// only place, if the user didn't drag (rotate) the plot 
			if(!is_3d_plot || !motion) {
				GPO.GetAnnotateString(s0, real_x, real_y, mouse_mode, mouse_alt_string);
				term->set_clipboard(s0);
				if(display_ipc_commands()) {
					fprintf(stderr, "put `%s' to clipboard.\n", s0);
				}
			}
		}
		if(b == 2) {
			// draw temporary annotation or label. For 3d plots this is
			// only done if the user didn't drag (scale) the plot 
			if(!is_3d_plot || !motion) {
				GPO.GetAnnotateString(s0, real_x, real_y, mouse_mode, mouse_alt_string);
				if(mouse_setting.label) {
					if(modifier_mask & Mod_Ctrl) {
						remove_label(mouse_x, mouse_y);
					}
					else {
						put_label(s0, real_x, real_y);
					}
				}
				else {
					int x = mouse_x;
					int y = mouse_y;
					int dx = term->TicH;
					int dy = term->TicV;
					(term->linewidth)(border_lp.l_width);
					(term->linetype)(border_lp.l_type);
					(term->move)(x - dx, y);
					(term->vector)(x + dx, y);
					(term->move)(x, y - dy);
					(term->vector)(x, y + dy);
					(term->justify_text)(LEFT);
					(term->put_text)(x + dx / 2, y + dy / 2 + term->ChrV / 3, s0);
					(term->text)();
				}
			}
		}
	}
	if(is_3d_plot && (b == 1 || b == 2 || b == 3)) {
		if(!!(modifier_mask & Mod_Ctrl) && !needreplot) {
			// redraw the 3d plot if its last redraw was 'quick' (only axes) because modifier key was pressed 
			GPO.DoSave3DPlot(term, first_3dplot, plot3d_num, NORMAL_REPLOT);
		}
		else if(b==1) {
			// Needed if the previous plot was QUICK_REFRESH 
			GPO.DoSave3DPlot(term, first_3dplot, plot3d_num, NORMAL_REPLOT);
		}
		if(term->set_cursor)
			term->set_cursor((button & (1 << 1)) ? 1 : (button & (1 << 2)) ? 2 : 0, 0, 0);
	}
	// Export current mouse coords to user-accessible variables also 
	GPO.LoadMouseVariables(mouse_x, mouse_y, TRUE, b);
	GPO.UpdateStatusLine();
}

static void event_motion(gp_event_t * ge)
{
	motion = 1;
	mouse_x = ge->mx;
	mouse_y = ge->my;
	if(is_3d_plot && (splot_map == FALSE)) { /* Rotate the surface if it is 3D graph but not "set view map". */
		bool redraw = FALSE;
		if(button & (1 << 1)) {
			// dragging with button 1 -> rotate 
			//surface_rot_x = floor(0.5 + zero_rot_x + 180.0 * mouse_y / term->MaxY);
			surface_rot_x = floor(0.5 + fmod(zero_rot_x + 360.0 * mouse_y / term->MaxY, 360));
			if(surface_rot_x < 0.0f)
				surface_rot_x += 360.0f;
			if(surface_rot_x > 360.0f)
				surface_rot_x -= 360.0f;
			surface_rot_z = floor(0.5 + fmod(zero_rot_z - 360.0 * mouse_x / term->MaxX, 360));
			if(surface_rot_z < 0)
				surface_rot_z += 360;
			redraw = TRUE;
		}
		else if(button & (1 << 2)) {
			// dragging with button 2 -> scale or changing ticslevel.
			// we compare the movement in x and y direction, and
			// change either scale or zscale 
			double relx = (double)abs(mouse_x - start_x) / (double)term->TicH;
			double rely = (double)abs(mouse_y - start_y) / (double)term->TicV;
			if(modifier_mask & Mod_Shift) {
				xyplane.z += (1 + fabs(xyplane.z)) * (mouse_y - start_y) * 2.0 / term->MaxY;
			}
			else {
				if(relx > rely) {
					surface_lscale += (mouse_x - start_x) * 2.0 / term->MaxX;
					surface_scale = exp(surface_lscale);
					if(surface_scale < 0)
						surface_scale = 0;
				}
				else {
					if(disable_mouse_z && (mouse_y-start_y > 0))
						;
					else {
						surface_zscale += (mouse_y - start_y) * 2.0 / term->MaxY;
						disable_mouse_z = FALSE;
					}
					if(surface_zscale < 0)
						surface_zscale = 0;
				}
			}
			// reset the start values 
			start_x = mouse_x;
			start_y = mouse_y;
			redraw = TRUE;
		}
		else if(button & (1 << 3)) {
			// dragging with button 3 -> change azimuth 
			ChangeAzimuth( (mouse_x - start_x) * 90.0 / term->MaxX);
			start_x = mouse_x;
			redraw = TRUE;
		}
		if(!ALMOST2D) {
			turn_ruler_off();
		}
		if(redraw) {
			if(allowmotion) {
				// is processing of motions allowed right now?
				// then replot while
				// disabling further replots until it completes 
				allowmotion = FALSE;
				GPO.DoSave3DPlot(term, first_3dplot, plot3d_num, ((modifier_mask & Mod_Ctrl) != 0) ? AXIS_ONLY_ROTATE : QUICK_REFRESH);
				GPO.Ev.FillGpValFoat("GPVAL_VIEW_ROT_X", surface_rot_x);
				GPO.Ev.FillGpValFoat("GPVAL_VIEW_ROT_Z", surface_rot_z);
				GPO.Ev.FillGpValFoat("GPVAL_VIEW_SCALE", surface_scale);
				GPO.Ev.FillGpValFoat("GPVAL_VIEW_ZSCALE", surface_zscale);
				GPO.Ev.FillGpValFoat("GPVAL_VIEW_AZIMUTH", azimuth);
			}
			else {
				needreplot = TRUE; // postpone the replotting 
			}
		}
	} /* if (3D plot) */
	if(ALMOST2D) {
		// 2D plot, or suitably aligned 3D plot: update
		// statusline and possibly the zoombox annotation 
		if(!term->put_tmptext)
			return;
		GPO.MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
		GPO.UpdateStatusLine();
		if(setting_zoom_region && mouse_setting.annotate_zoom_box) {
			double real_x, real_y, real_x2, real_y2;
			char s[64];
			GPO.MousePosToGraphPosReal(mouse_x, mouse_y, &real_x, &real_y, &real_x2, &real_y2);
			sprintf(s, zoombox_format(), real_x, real_y);
			term->put_tmptext(2, s);
		}
	}
}

static void event_modifier(struct gp_event_t * ge)
{
	modifier_mask = ge->par1;
	if(modifier_mask == 0 && is_3d_plot && (button & ((1 << 1) | (1 << 2))) && !needreplot) {
		// redraw the 3d plot if modifier key released 
		GPO.DoSave3DPlot(term, first_3dplot, plot3d_num, NORMAL_REPLOT);
	}
}

//void event_plotdone()
void GnuPlot::EventPlotDone(termentry * pTerm)
{
	if(needreplot) {
		needreplot = FALSE;
		DoSave3DPlot(pTerm, first_3dplot, plot3d_num, ((modifier_mask & Mod_Ctrl) != 0) ? AXIS_ONLY_ROTATE : NORMAL_REPLOT);
	}
	else
		allowmotion = TRUE;
}

void event_reset(struct gp_event_t * ge)
{
	modifier_mask = 0;
	button = 0;
	builtin_cancel_zoom(ge);
	if(term && term_initialised && term->set_cursor) {
		term->set_cursor(0, 0, 0);
		if(mouse_setting.annotate_zoom_box && term->put_tmptext) {
			term->put_tmptext(1, "");
			term->put_tmptext(2, "");
		}
	}
	/* This hack is necessary on some systems in order to prevent one
	 * character of input from being swallowed when the plot window is
	 * closed. But which systems, exactly, and in what circumstances?
	 */
	if(paused_for_mouse || !interactive) {
		if(term && term_initialised && (!strncmp("x11", term->name, 3) || !strncmp("wxt", term->name, 3) || !strncmp("qt", term->name, 2)))
			ungetc('\n', stdin);
	}
	if(paused_for_mouse) {
		paused_for_mouse = 0;
#ifdef _WIN32
		/* close pause message box */
		kill_pending_Pause_dialog();
#endif
	}
	// Dummy up a keystroke event so that we can conveniently check for a  
	// binding to "Close". We only get these for the current window. 
	if(ge != (void*)1) {
		ge->par1 = GP_Cancel; /* Dummy keystroke */
		ge->par2 = 0;   /* Not used; could pass window id here? */
		event_keypress(ge, TRUE);
	}
}

//void do_event(gp_event_t * pGe)
void GnuPlot::DoEvent(termentry * pTerm, gp_event_t * pGe)
{
	if(pTerm) {
		// disable `replot` when some data were sent through stdin 
		replot_disabled = plotted_data_from_stdin;
		if(pGe->type) {
			FPRINTF((stderr, "(do_event) type       = %s\n", GE_evt_name(pGe->type)));
			FPRINTF((stderr, "           mx, my     = %d, %d\n", pGe->mx, pGe->my));
			FPRINTF((stderr, "           par1, par2 = %d, %d\n", pGe->par1, pGe->par2));
		}
		switch(pGe->type) {
			case GE_plotdone:
				EventPlotDone(pTerm);
				if(pGe->winid) {
					current_x11_windowid = pGe->winid;
					UpdateGpvalVariables(6); // fill GPVAL_TERM_WINDOWID 
				}
				break;
			case GE_keypress:
				event_keypress(pGe, TRUE);
				break;
			case GE_keypress_old:
				event_keypress(pGe, FALSE);
				break;
			case GE_modifier:
				event_modifier(pGe);
				break;
			case GE_motion:
				if(!mouse_setting.on)
					break;
				event_motion(pGe);
				break;
			case GE_buttonpress:
				if(!mouse_setting.on)
					break;
				event_buttonpress(pGe);
				break;
			case GE_buttonrelease:
				if(!mouse_setting.on)
					break;
				event_buttonrelease(pGe);
				break;
			case GE_replot:
				// auto-generated replot (e.g. from replot-on-resize) 
				// FIXME: more terminals should use this! 
				if(replot_line == NULL || replot_line[0] == '\0')
					break;
				if(!strncmp(replot_line, "test", 4))
					break;
				if(multiplot)
					break;
				DoStringReplot("");
				break;
			case GE_reset:
				event_reset(pGe);
				break;
			case GE_fontprops:
	#ifdef X11
				// EAM FIXME:  Despite the name, only X11 uses this to pass font info.	
				// Everyone else passes just the plot height and width.			
				if(!strcmp(pTerm->name, "x11")) {
					/* These are declared in ../term/x11.trm */
					extern int X11_hchar_saved, X11_vchar_saved;
					extern double X11_ymax_saved;
					/* Cached sizing values for the x11 terminal. Each time an X11 window is
					   resized, these are updated with the new sizes. When a replot happens some
					   time later, these saved values are used. The normal mechanism for doing this
					   is sending a QG from inboard to outboard driver, then the outboard driver
					   responds with the sizing info in a GE_fontprops event. The problem is that
					   other than during plot initialization the communication is asynchronous.
					 */
					X11_hchar_saved = pGe->par1;
					X11_vchar_saved = pGe->par2;
					X11_ymax_saved = (double)pTerm->MaxX * (double)pGe->my / fabs((double)pGe->mx);
					// If mx < 0, we simply save the values for future use, and move on 
					if(pGe->mx < 0) {
						break;
					}
					else {
						// Otherwise we apply the changes right now 
						pTerm->ChrH = X11_hchar_saved;
						pTerm->ChrV = X11_vchar_saved;
						// factor of 2.5 must match the use in x11.trm 
						pTerm->TicH = pTerm->TicV = X11_vchar_saved / 2.5;
						pTerm->MaxY  = X11_ymax_saved;
					}
				}
				else
				// Fall through to cover non-x11 case 
	#endif
				// Other terminals update aspect ratio based on current window size 
				pTerm->TicV = pTerm->TicH * (double)pGe->mx / (double)pGe->my;
				FPRINTF((stderr, "mouse do_event: window size %d X %d, font hchar %d vchar %d\n", pGe->mx, pGe->my, pGe->par1, pGe->par2));
				break;
			case GE_buttonpress_old:
			case GE_buttonrelease_old:
				// ignore 
				break;
			case GE_raise:
				// FIXME: No generic routine implemented! 
				// Individual terminal types must handle it themselves if they care 
				break;
			default:
				fprintf(stderr, "%s:%d unrecognized event type %d\n", __FILE__, __LINE__, pGe->type);
				break;
		}
		replot_disabled = FALSE; // enable replot again 
	}
}
// 
// convenience wrapper for do_event();
// returns TRUE if it ends pause mouse;
// currently used by caca.trm, djsvga.trm, and pc.trm 
// 
bool exec_event(char type, int mx, int my, int par1, int par2, int winid)
{
	gp_event_t ge;
	ge.type = type;
	ge.mx = mx;
	ge.my = my;
	ge.par1 = par1;
	ge.par2 = par2;
	ge.winid = winid;
	GPO.DoEvent(term, &ge);
	// end pause mouse? 
	if((type == GE_buttonrelease) && (paused_for_mouse & PAUSE_CLICK) && (((par1 == 1) && 
		(paused_for_mouse & PAUSE_BUTTON1)) || ((par1 == 2) && (paused_for_mouse & PAUSE_BUTTON2)) || ((par1 == 3) && (paused_for_mouse & PAUSE_BUTTON3)))) {
		paused_for_mouse = 0;
		return true;
	}
	else if(type == GE_keypress && (paused_for_mouse & PAUSE_KEYSTROKE) && (par1 != NUL)) {
		paused_for_mouse = 0;
		return true;
	}
	else
		return false;
}

//static void do_save_3dplot(surface_points * plots, int pcount, REPLOT_TYPE quick)
void GnuPlot::DoSave3DPlot(termentry * pTerm, surface_points * pPlots, int pcount, REPLOT_TYPE quick)
{
	if(!pPlots || E_REFRESH_NOT_OK == refresh_ok) {
		// !plots might happen after the `reset' command for example
		// (reported by Franz Bakan).
		// !refresh_ok can happen for example if log scaling is reset (EAM).
		// replotrequest() should set up everything again in either case.
		Pgm.ReplotRequest();
	}
	else
		Do3DPlot(pTerm, pPlots, pcount, quick);
}
/*
 * bind related functions
 */
static void bind_install_default_bindings()
{
	bind_remove_all();
	bind_append("a", (char*)0, builtin_autoscale);
	bind_append("b", (char*)0, builtin_toggle_border);
	bind_append("e", (char*)0, builtin_replot);
	bind_append("g", (char*)0, builtin_toggle_grid);
	bind_append("h", (char*)0, builtin_help);
	bind_append("i", (char*)0, builtin_invert_plot_visibilities);
	bind_append("l", (char*)0, builtin_toggle_log);
	bind_append("L", (char*)0, builtin_nearest_log);
	bind_append("m", (char*)0, builtin_toggle_mouse);
	bind_append("r", (char*)0, builtin_toggle_ruler);
	bind_append("V", (char*)0, builtin_set_plots_invisible);
	bind_append("v", (char*)0, builtin_set_plots_visible);
	bind_append("1", (char*)0, builtin_decrement_mousemode);
	bind_append("2", (char*)0, builtin_increment_mousemode);
	bind_append("5", (char*)0, builtin_toggle_polardistance);
	bind_append("6", (char*)0, builtin_toggle_verbose);
	bind_append("7", (char*)0, builtin_toggle_ratio);
	bind_append("n", (char*)0, builtin_zoom_next);
	bind_append("p", (char*)0, builtin_zoom_previous);
	bind_append("u", (char*)0, builtin_unzoom);
	bind_append("+", (char*)0, builtin_zoom_in_around_mouse);
	bind_append("=", (char*)0, builtin_zoom_in_around_mouse); /* same key as + but no need for Shift */
	bind_append("-", (char*)0, builtin_zoom_out_around_mouse);
	bind_append("Right", (char*)0, builtin_rotate_right);
	bind_append("Up", (char*)0, builtin_rotate_up);
	bind_append("Left", (char*)0, builtin_rotate_left);
	bind_append("Down", (char*)0, builtin_rotate_down);
	bind_append("Opt-<", (char*)0, builtin_azimuth_left);
	bind_append("Opt->", (char*)0, builtin_azimuth_right);
	bind_append("Escape", (char*)0, builtin_cancel_zoom);
}

static void bind_clear(bind_t * b)
{
	b->key = NO_KEY;
	b->modifier = 0;
	b->command = (char*)0;
	b->builtin = 0;
	b->prev = (struct bind_t *)0;
	b->next = (struct bind_t *)0;
}

/* returns the enum which corresponds to the
 * string (ptr) or NO_KEY if ptr matches not
 * any of special_keys. */
static int lookup_key(char * ptr, int * len)
{
	char ** keyptr;
	/* first, search in the table of "usual well-known" keys */
	int what = lookup_table_nth(usual_special_keys, ptr);
	if(what >= 0) {
		*len = strlen(usual_special_keys[what].key);
		return usual_special_keys[what].value;
	}
	/* second, search in the table of other keys */
	for(keyptr = special_keys; *keyptr; ++keyptr) {
		if(!strcmp(ptr, *keyptr)) {
			*len = strlen(ptr);
			return keyptr - special_keys + GP_FIRST_KEY;
		}
	}
	return NO_KEY;
}

/* returns 1 on success, else 0. */
static int bind_scan_lhs(bind_t * out, const char * in)
{
	static const char DELIM = '-';
	int itmp = NO_KEY;
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
		else if(!strncasecmp(ptr, "shift-", 6)) {
			out->modifier |= Mod_Shift;
			ptr += 6;
		}
		else if(!strncasecmp(ptr, "opt-", 4)) {
			out->modifier |= Mod_Opt;
			ptr += 4;
		}
		else if(NO_KEY != (itmp = lookup_key(ptr, &len))) {
			out->key = itmp;
			ptr += len;
		}
		else if((out->key = *ptr++) && *ptr && *ptr != DELIM) {
			fprintf(stderr, "bind: cannot parse %s\n", in);
			return 0;
		}
	}
	if(NO_KEY == out->key)
		return 0;       /* failed */
	else
		return 1;       /* success */
}

/* note, that this returns a pointer
 * to the static char* `out' which is
 * modified on subsequent calls.
 */
static char * bind_fmt_lhs(const bind_t * in)
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
	if(in->modifier & Mod_Shift) {
		strcat(out, "Shift-");
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

static int bind_matches(const bind_t * a, const bind_t * b)
{
	int a_mod = a->modifier;
	int b_mod = b->modifier;

	/* discard Shift modifier (except for mouse buttons) */
	if(a->key < GP_Button1) {
		a_mod &= (Mod_Ctrl | Mod_Alt);
		b_mod &= (Mod_Ctrl | Mod_Alt);
	}

	if(a->key == b->key && a_mod == b_mod)
		return 1;
	else if(a->key == b->key && (b->modifier & Mod_Opt))
		/* Mod_Opt means both Alt and Ctrl are optional */
		return 2;
	else
		return 0;
}

static void bind_display_one(bind_t * ptr)
{
	fprintf(stderr, " %-13s ", bind_fmt_lhs(ptr));
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
	bind_t * ptr;
	bind_t lhs_scanned;
	if(!bindings) {
		bind_install_default_bindings();
	}
	if(!lhs) {
		// display all bindings 
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
		fprintf(stderr, fmt, "<B3-Motion>", "change view (azimuth)");
		// mouse wheel 
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
#if (0) /* Not implemented in the core code! */
#ifndef DISABLE_SPACE_RAISES_CONSOLE
		fprintf(stderr, " %-12s   %s\n", "Space", "raise gnuplot console window");
#endif
#endif
		fprintf(stderr, " %-12s * %s\n", "q", "close this plot window");
		fprintf(stderr, "\n");
		for(ptr = bindings; ptr; ptr = ptr->next) {
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
	for(ptr = bindings; ptr; ptr = ptr->next) {
		if(bind_matches(&lhs_scanned, ptr)) {
			bind_display_one(ptr);
			break;  /* only one match */
		}
	}
}

static void bind_remove(bind_t * b)
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
				bindings->prev = b->prev;
			ZFREE(b->command);
			if(b == bindings) {
				bindings = b->next;
				if(bindings && bindings->prev)
					bindings->prev->next = (bind_t*)0;
			}
			SAlloc::F(b);
		}
	}
}

static void bind_append(char * lhs, char * rhs, char *(*builtin)(struct gp_event_t * ge))
{
	bind_t * p_new = (bind_t*)gp_alloc(sizeof(bind_t), "bind_append->p_new");
	if(!bind_scan_lhs(p_new, lhs)) {
		SAlloc::F(p_new);
		return;
	}
	if(!bindings) {
		bindings = p_new; // first binding 
	}
	else {
		for(bind_t * ptr = bindings; ptr; ptr = ptr->next) {
			if(bind_matches(p_new, ptr)) {
				/* overwriting existing binding */
				if(!rhs) {
					ptr->builtin = builtin;
				}
				else if(*rhs) {
					ZFREE(ptr->command);
					ptr->command = rhs;
				}
				else { /* rhs is an empty string, so remove the binding */
					bind_remove(ptr);
				}
				SAlloc::F(p_new); /* don't need it any more */
				return;
			}
		}
		/* if we're here, the binding does not exist yet */
		/* append binding ... */
		bindings->prev->next = p_new;
		p_new->prev = bindings->prev;
	}
	bindings->prev = p_new;
	p_new->next = (struct bind_t *)0;
	p_new->allwindows = FALSE; /* Can be explicitly set later */
	if(!rhs) {
		p_new->builtin = builtin;
	}
	else if(*rhs) {
		p_new->command = rhs; /* was allocated in command.c */
	}
	else {
		bind_remove(p_new);
	}
}

void bind_process(char * lhs, char * rhs, bool allwindows)
{
	if(!bindings) {
		bind_install_default_bindings();
	}
	if(!rhs) {
		bind_display(lhs);
	}
	else {
		bind_append(lhs, rhs, 0);
		if(allwindows)
			bind_all(lhs);
	}
	SAlloc::F(lhs);
}

void bind_all(char * lhs)
{
	bind_t * ptr;
	bind_t keypress;
	if(!bind_scan_lhs(&keypress, lhs))
		return;
	for(ptr = bindings; ptr; ptr = ptr->next) {
		if(bind_matches(&keypress, ptr))
			ptr->allwindows = TRUE;
	}
}

void bind_remove_all()
{
	bind_t * safe;
	for(bind_t * ptr = bindings; ptr; safe = ptr, ptr = ptr->next, SAlloc::F(safe)) {
		if(ptr->command) {
			ZFREE(ptr->command);
		}
	}
	bindings = (bind_t*)0;
}
//
// Ruler is on, thus recalc its (px,py) from (x,y) for the current zoom and log axes.
//
//static void recalc_ruler_pos()
void GnuPlot::RecalcRulerPos()
{
	double P, dummy;
	if(is_3d_plot) {
		// To be exact, it is 'set view map' splot. 
		int ppx, ppy;
		dummy = 1.0; // dummy value, but not 0.0 for the fear of log z-axis 
		Map3D_XY(ruler.x, ruler.y, dummy, &ppx, &ppy);
		ruler.px = ppx;
		ruler.py = ppy;
	}
	else {
		// It is 2D plot. 
		if(AxS[FIRST_X_AXIS].log && ruler.x < 0)
			ruler.px = -1;
		else {
			P = ruler.x;
			ruler.px = AxS[FIRST_X_AXIS].MapI(P);
		}
		if(AxS[FIRST_Y_AXIS].log && ruler.y < 0)
			ruler.py = -1;
		else {
			P = ruler.y;
			ruler.py = AxS[FIRST_Y_AXIS].MapI(P);
		}
		MousePosToGraphPosReal(ruler.px, ruler.py, &dummy, &dummy, &ruler.x2, &ruler.y2);
	}
}
//
// Recalculate and replot the ruler after a '(re)plot'. Called from term.c.
//
//void update_ruler()
void GnuPlot::UpdateRuler(termentry * pTerm)
{
	if(pTerm->set_ruler && ruler.on) {
		(pTerm->set_ruler)(-1, -1);
		RecalcRulerPos();
		(pTerm->set_ruler)(ruler.px, ruler.py);
	}
}

/* Set ruler on/off, and set its position.
   Called from set.c for 'set mouse ruler ...' command.
 */
void set_ruler(bool on, int mx, int my)
{
	struct gp_event_t ge;
	if(ruler.on == FALSE && on == FALSE)
		return;
	if(ruler.on == TRUE && on == TRUE && (mx < 0 || my < 0))
		return;
	if(ruler.on == TRUE) /* ruler is on => switch it off */
		builtin_toggle_ruler(&ge);
	/* now the ruler is off */
	if(on == FALSE) /* want ruler off */
		return;
	if(mx>=0 && my>=0) { /* change ruler position */
		ge.mx = mx;
		ge.my = my;
	}
	else { /* don't change ruler position */
		ge.mx = ruler.px;
		ge.my = ruler.py;
	}
	builtin_toggle_ruler(&ge);
}

/* for checking if we change from plot to splot (or vice versa) */
int plot_mode(int set)
{
	static int mode = MODE_PLOT;
	if(oneof2(set, MODE_PLOT, MODE_SPLOT)) {
		if(mode != set) {
			turn_ruler_off();
		}
		mode = set;
	}
	return mode;
}

static void turn_ruler_off()
{
	if(ruler.on) {
		udvt_entry * u;
		ruler.on = FALSE;
		if(term && term->set_ruler) {
			(*term->set_ruler)(-1, -1);
		}
		if((u = GPO.Ev.AddUdvByName("MOUSE_RULER_X")))
			u->udv_value.SetNotDefined();
		if((u = GPO.Ev.AddUdvByName("MOUSE_RULER_Y")))
			u->udv_value.SetNotDefined();
		if(display_ipc_commands()) {
			fprintf(stderr, "turning ruler off.\n");
		}
	}
}

static int nearest_label_tag(int xref, int yref)
{
	double min = -1;
	int min_tag = -1;
	double diff_squared;
	int x, y;
	int xd;
	int yd;
	for(text_label * this_label = first_label; this_label != NULL; this_label = this_label->next) {
		if(is_3d_plot) {
			GPO.Map3DPosition(&this_label->place, &xd, &yd, "label");
			xd -= xref;
			yd -= yref;
		}
		else {
			GPO.MapPosition(term, &this_label->place, &x, &y, "label");
			xd = x - xref;
			yd = y - yref;
		}
		diff_squared = xd * xd + yd * yd;
		if(-1 == min || min > diff_squared) {
			// now we check if we're within a certain threshold around the label 
			double tic_diff_squared;
			int htic, vtic;
			get_offsets(this_label, &htic, &vtic);
			tic_diff_squared = htic * htic + vtic * vtic;
			if(diff_squared < tic_diff_squared) {
				min = diff_squared;
				min_tag = this_label->tag;
			}
		}
	}
	return min_tag;
}

static void remove_label(int x, int y)
{
	int tag = nearest_label_tag(x, y);
	if(-1 != tag) {
		char cmd[0x40];
		sprintf(cmd, "unset label %d", tag);
		GPO.DoStringReplot(cmd);
	}
}

static void put_label(char * label, double x, double y)
{
	char cmd[512];
	sprintf(cmd, "set label \"%s\" at %g,%g %s", label, x, y, mouse_setting.labelopts ? mouse_setting.labelopts : "point pt 1");
	GPO.DoStringReplot(cmd);
}
//
// Save current mouse position to user-accessible variables.
// Save the keypress or mouse button that triggered this in MOUSE_KEY,
// and define MOUSE_BUTTON if it was a button click.
//
//static void load_mouse_variables(double x, double y, bool button, int c)
void GnuPlot::LoadMouseVariables(double x, double y, bool button, int c)
{
	udvt_entry * current;
	MousePosToGraphPosReal(x, y, &real_x, &real_y, &real_x2, &real_y2);
	if((current = Ev.AddUdvByName("MOUSE_BUTTON"))) {
		Ginteger(&current->udv_value, button ? c : -1);
		if(!button)
			current->udv_value.SetNotDefined();
	}
	if((current = Ev.AddUdvByName("MOUSE_KEY"))) {
		Ginteger(&current->udv_value, c);
	}
	if((current = Ev.AddUdvByName("MOUSE_CHAR"))) {
		char * keychar = (char *)gp_alloc(2, "key_char");
		keychar[0] = c;
		keychar[1] = '\0';
		gpfree_string(&current->udv_value);
		Gstring(&current->udv_value, keychar);
	}
	if((current = Ev.AddUdvByName("MOUSE_X"))) {
		Gcomplex(&current->udv_value, real_x, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_Y"))) {
		Gcomplex(&current->udv_value, real_y, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_X2"))) {
		Gcomplex(&current->udv_value, real_x2, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_Y2"))) {
		Gcomplex(&current->udv_value, real_y2, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_SHIFT"))) {
		Ginteger(&current->udv_value, modifier_mask & Mod_Shift);
	}
	if((current = Ev.AddUdvByName("MOUSE_ALT"))) {
		Ginteger(&current->udv_value, modifier_mask & Mod_Alt);
	}
	if((current = Ev.AddUdvByName("MOUSE_CTRL"))) {
		Ginteger(&current->udv_value, modifier_mask & Mod_Ctrl);
	}
}

#endif /* USE_MOUSE */
