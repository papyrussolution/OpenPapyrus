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

long   mouse_mode = MOUSE_COORDINATES_REAL;
char * mouse_alt_string = (char *)0;
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
	(char *)0 /* must be the last line */
};

/* "usual well-known" keycodes, i.e. those not listed in special_keys in mouse.h
 */
static const struct gen_table usual_special_keys[] = {
	{ "BackSpace", GP_BackSpace},
	{ "Tab", GP_Tab},
	{ "KP_Enter", GP_KP_Enter},
	{ "Return", GP_Return},
	{ "Escape", GP_Escape},
	{ "Delete", GP_Delete},
	{ NULL, 0}
};
//
// do we allow motion to result in a replot right now? 
bool allowmotion = TRUE;        /* used by pm.trm, too */
//
// bind related stuff 
//
static const int NO_KEY = -1;
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
void GnuPlot::MousePosToGraphPosReal(SPoint2I pt, double * x, double * y, double * x2, double * y2)
{
	GpAxis * secondary;
	if(Gg.Is3DPlot) {
		// for 3D plots, we treat the mouse position as if it is
		// in the bottom plane, i.e., the plane of the x and y axis 
		// note: at present, this projection is only correct if
		// _3DBlk.SurfaceRotZ is a multiple of 90 degrees! 
		// HBB 20010522: added protection against division by zero
		// for cases like 'set view 90,0' 
		pt.x -= _3DBlk.axis3d_o_x;
		pt.y -= _3DBlk.axis3d_o_y;
		if(abs(_3DBlk.axis3d_x_dx) > abs(_3DBlk.axis3d_x_dy)) {
			*x = AxS[FIRST_X_AXIS].min + ((double)pt.x) / _3DBlk.axis3d_x_dx * AxS[FIRST_X_AXIS].GetRange();
		}
		else if(_3DBlk.axis3d_x_dy != 0) {
			*x = AxS[FIRST_X_AXIS].min + ((double)pt.y) / _3DBlk.axis3d_x_dy * AxS[FIRST_X_AXIS].GetRange();
		}
		else {
			// both diffs are zero (x axis points into the screen 
			*x = VERYLARGE;
		}
		if(abs(_3DBlk.axis3d_y_dx) > abs(_3DBlk.axis3d_y_dy)) {
			*y = AxS[FIRST_Y_AXIS].min + ((double)pt.x) / _3DBlk.axis3d_y_dx * AxS[FIRST_Y_AXIS].GetRange();
		}
		else if(_3DBlk.axis3d_y_dy != 0) {
			if(_3DBlk.splot_map)
				*y = AxS[FIRST_Y_AXIS].max + ((double)pt.y) / _3DBlk.axis3d_y_dy * (AxS[FIRST_Y_AXIS].min - AxS[FIRST_Y_AXIS].max);
			else
				*y = AxS[FIRST_Y_AXIS].min + ((double)pt.y) / _3DBlk.axis3d_y_dy * (AxS[FIRST_Y_AXIS].GetRange());
		}
		else {
			// both diffs are zero (y axis points into the screen 
			*y = VERYLARGE;
		}
		*x2 = *y2 = VERYLARGE; // protection 
		return;
	}
	// 2D plot 
	if(V.BbPlot.xright == V.BbPlot.xleft)
		*x = *x2 = VERYLARGE; // protection 
	else {
		*x  = AxS[FIRST_X_AXIS].MapBack(pt.x);
		*x2 = AxS[SECOND_X_AXIS].MapBack(pt.x);
	}
	if(V.BbPlot.ytop == V.BbPlot.ybot)
		*y = *y2 = VERYLARGE; /* protection */
	else {
		*y  = AxS[FIRST_Y_AXIS].MapBack(pt.y);
		*y2 = AxS[SECOND_Y_AXIS].MapBack(pt.y);
	}
	FPRINTF((stderr, "POS: xx=%i, yy=%i  =>  x=%g  y=%g\n", pt.x, pt.y, *x, *y));

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
	if(secondary->linked_to_primary && secondary->linked_to_primary->index == -FIRST_X_AXIS) {
		*x = secondary->linked_to_primary->MapBack(pt.x);
		*x = EvalLinkFunction(secondary, *x);
	}
	secondary = &AxS[FIRST_Y_AXIS];
	if(secondary->linked_to_primary && secondary->linked_to_primary->index == -FIRST_Y_AXIS) {
		*y = secondary->linked_to_primary->MapBack(pt.y);
		*y = EvalLinkFunction(secondary, *y);
	}
	secondary = &AxS[SECOND_X_AXIS];
	if(secondary->linked_to_primary && secondary->linked_to_primary->index == -SECOND_X_AXIS) {
		*x2 = secondary->linked_to_primary->MapBack(pt.x);
		*x2 = EvalLinkFunction(secondary, *x2);
	}
	secondary = &AxS[SECOND_Y_AXIS];
	if(secondary->linked_to_primary && secondary->linked_to_primary->index == -SECOND_Y_AXIS) {
		*y2 = secondary->linked_to_primary->MapBack(pt.y);
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
		sprintf(dms_format, "%%D%s%%.2m'", GpU.degree_sign);
		if(AxS[FIRST_X_AXIS].datatype == DT_DMS)
			GStrDMS(s, fmt ? fmt : dms_format, x);
		else
			sprintf(s, mouse_setting.fmt, x);
		strcat(s, ", ");
		s += strlen(s);
		if(AxS[FIRST_Y_AXIS].datatype == DT_DMS)
			GStrDMS(s, fmt ? fmt : dms_format, y);
		else
			sprintf(s, mouse_setting.fmt, y);
		s += strlen(s);
	}
	else if(oneof4(mode, MOUSE_COORDINATES_XDATE, MOUSE_COORDINATES_XTIME, MOUSE_COORDINATES_XDATETIME, MOUSE_COORDINATES_TIMEFMT)) { // time is on the x axis
		char buf[0xff];
		char format[0xff] = "[%s, ";
		strcat(format, mouse_setting.fmt);
		strcat(format, "]");
		sprintf(s, format, XDateTimeFormat(x, buf, mode), y);
	}
	else if(mode == MOUSE_COORDINATES_FRACTIONAL) {
		double xrange = AxS[FIRST_X_AXIS].GetRange();
		double yrange = AxS[FIRST_Y_AXIS].GetRange();
		// calculate fractional coordinates. prevent division by zero 
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
		sprintf(s, xy_format(), x, y); // w/o brackets 
	}
	else if((mode == MOUSE_COORDINATES_ALT) && Gg.Polar) {
		double r;
		double phi = atan2(y, x);
		double rmin = (AxS.__R().autoscale & AUTOSCALE_MIN) ? 0.0 : AxS.__R().set_min;
		double theta = phi / SMathConst::PiDiv180;
		// Undo "set theta" 
		theta = (theta - AxS.ThetaOrigin) * AxS.ThetaDirection;
		if(theta > 180.0)
			theta = theta - 360.0;
		if(AxS.__R().IsNonLinear())
			r = EvalLinkFunction(&AxS.__R(), x/cos(phi) + AxS.__R().linked_to_primary->min);
		else if(AxS.__R().log)
			r = rmin + x/cos(phi);
		else if(Gg.InvertedRaxis)
			r = rmin - x/cos(phi);
		else
			r = rmin + x/cos(phi);
		if(fmt)
			sprintf(s, fmt, theta, r);
		else
			sprintf(s, "theta: %.1f%s  r: %g", theta, GpU.degree_sign, r);
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
		if(readout.Type != STRING) {
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
char * GnuPlot::XDateTimeFormat(double x, char * pB, int mode)
{
	struct tm tm;
	switch(mode) {
		case MOUSE_COORDINATES_XDATE:
		    GGmTime(&tm, x);
		    sprintf(pB, "%d. %d. %04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year);
		    break;
		case MOUSE_COORDINATES_XTIME:
		    GGmTime(&tm, x);
		    sprintf(pB, "%d:%02d", tm.tm_hour, tm.tm_min);
		    break;
		case MOUSE_COORDINATES_XDATETIME:
		    GGmTime(&tm, x);
		    sprintf(pB, "%d. %d. %04d %d:%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year,
			tm.tm_hour, tm.tm_min);
		    break;
		case MOUSE_COORDINATES_TIMEFMT:
		    GStrFTime(pB, 0xff, AxS.P_TimeFormat, x);
		    break;
		default:
		    sprintf(pB, mouse_setting.fmt, x);
	}
	return pB;
}
// 
// Format one axis coordinate for output to mouse status or button 2 label text
// 
char * GnuPlot::MkStr(char * sp, double x, AXIS_INDEX axis)
{
	if(x >= VERYLARGE)
		return sp;
	if(axis == FIRST_X_AXIS && oneof4(mouse_mode, MOUSE_COORDINATES_XDATE, MOUSE_COORDINATES_XTIME, MOUSE_COORDINATES_XDATETIME, MOUSE_COORDINATES_TIMEFMT)) {
		// mouseformats 3 4 5 6 use specific time format for x coord
		XDateTimeFormat(x, sp, mouse_mode);
	}
	else if(AxS[axis].datatype == DT_TIMEDATE) {
		char * format = CopyOrInventFormatString(&AxS[axis]);
		while(strchr(format, '\n'))
			*(strchr(format, '\n')) = ' ';
		GStrFTime(sp, 40, format, x);
	}
	else
		sprintf(sp, mouse_setting.fmt, x);
	return (sp + strlen(sp));
}

// ratio for log, distance for linear 
#define DIST(x, rx, axis) (AxS[axis].log) ? ((rx==0) ? 99999 : x / rx) : (x - rx)

/* formats the ruler information (position, distance,...) into string p
        (it must be sufficiently long)
   x, y is the current mouse position in real coords (for the calculation
        of distance)
 */
void GnuPlot::GetRulerString(char * p, double x, double y)
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
	dx = DIST(x, _Mse.Ruler.RealPos.x, FIRST_X_AXIS);
	dy = DIST(y, _Mse.Ruler.RealPos.y, FIRST_Y_AXIS);
	sprintf(p, format, _Mse.Ruler.RealPos.x, _Mse.Ruler.RealPos.y, dx, dy);
	/* Previously, the following "if" let the polar coordinates to be shown only
	   for lin-lin plots:
	        if (mouse_setting.polardistance && !AxS[FIRST_X_AXIS].log && !AxS[FIRST_Y_AXIS].log) ...
	   Now, let us support also semilog and log-log plots.
	   Values of mouse_setting.polardistance are:
	        0 (no polar coordinates), 1 (polar coordinates), 2 (tangent instead of angle).
	 */
	if(mouse_setting.polardistance) {
		double rho, phi, rx, ry;
		char ptmp[69];
		rx = _Mse.Ruler.RealPos.x;
		ry = _Mse.Ruler.RealPos.y;
		format[0] = '\0';
		strcat(format, " (");
		strcat(format, mouse_setting.fmt);
		rho = sqrt((x - rx) * (x - rx) + (y - ry) * (y - ry)); /* distance */
		if(mouse_setting.polardistance == 1) { /* (distance, angle) */
			phi = (180 / SMathConst::Pi) * atan2(y - ry, x - rx);
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
void GnuPlot::ApplyZoom(GpTermEntry * pTerm, t_zoom * z)
{
	int is_splot_map = (Gg.Is3DPlot && _3DBlk.splot_map);
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
		axis_array_copy = (GpAxis *)SAlloc::R(axis_array_copy, sizeof(GpAxis) * AXIS_ARRAY_SIZE);
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
	(pTerm->layer)(pTerm, TERM_LAYER_BEFORE_ZOOM);
	// New range on primary axes 
	SetExplicitRange(&AxS[FIRST_X_AXIS], zoom_now->_Min.x, zoom_now->_Max.x);
	SetExplicitRange(&AxS[FIRST_Y_AXIS], zoom_now->_Min.y, zoom_now->_Max.y);
	// EAM Apr 2013 - The tests on VERYLARGE protect against trying to
	// interpret the autoscaling initial state as an actual limit value. 
	if(!Gg.Is3DPlot && (zoom_now->_2Min.x < VERYLARGE && zoom_now->_2Max.x > -VERYLARGE)) {
		SetExplicitRange(&AxS[SECOND_X_AXIS], zoom_now->_2Min.x, zoom_now->_2Max.x);
	}
	if(!Gg.Is3DPlot && (zoom_now->_2Min.y < VERYLARGE && zoom_now->_2Max.y > -VERYLARGE)) {
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
		// Falling through to DoStringReplot() does not work! 
		if(Gg.VolatileData) {
			if(Gg.refresh_ok == E_REFRESH_OK_2D) {
				RefreshRequest(pTerm);
				return;
			}
			if(is_splot_map && (Gg.refresh_ok == E_REFRESH_OK_3D)) {
				RefreshRequest(pTerm);
				return;
			}
		}
	}
	else
		AxS.inside_zoom = true;
	DoStringReplot(pTerm, "");
	AxS.inside_zoom = false;
}
// 
// makes a zoom: update zoom history, call gnuplot to set ranges + replot
// 
void GnuPlot::DoZoom(double xmin, double ymin, double x2min, double y2min, double xmax, double ymax, double x2max, double y2max)
{
	struct t_zoom * z;
	if(zoom_head == NULL) { // queue not yet created, thus make its head 
		zoom_head = (struct t_zoom *)SAlloc::M(sizeof(struct t_zoom));
		zoom_head->prev = NULL;
		zoom_head->next = NULL;
	}
	if(zoom_now == NULL)
		zoom_now = zoom_head;
	if(zoom_now->next == NULL) {    /* allocate new item */
		z = (struct t_zoom *)SAlloc::M(sizeof(struct t_zoom));
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
	ApplyZoom(term, z);
}

void GnuPlot::ZoomNext(GpTermEntry * pTerm)
{
	if(zoom_now == NULL || zoom_now->next == NULL)
		alert();
	else
		ApplyZoom(pTerm, zoom_now->next);
	if(display_ipc_commands()) {
		fprintf(stderr, "next zoom.\n");
	}
}

void GnuPlot::ZoomPrevious(GpTermEntry * pTerm)
{
	if(zoom_now == NULL || zoom_now->prev == NULL)
		alert();
	else
		ApplyZoom(pTerm, zoom_now->prev);
	if(display_ipc_commands()) {
		fprintf(stderr, "previous zoom.\n");
	}
}

void GnuPlot::ZoomUnzoom(GpTermEntry * pTerm)
{
	if(zoom_head == NULL || zoom_now == zoom_head)
		alert();
	else
		ApplyZoom(pTerm, zoom_head);
	if(display_ipc_commands()) {
		fprintf(stderr, "unzoom.\n");
	}
}

void GnuPlot::IncrMouseMode(const int amount)
{
	long int old = mouse_mode;
	bool found_a_new_one = FALSE;
	mouse_mode += amount;
	while(!found_a_new_one) {
		if(MOUSE_COORDINATES_ALT == mouse_mode && !(mouse_alt_string || Gg.Polar))
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
	UpdateStatusLine();
	if(display_ipc_commands())
		fprintf(stderr, "switched mouse format from %ld to %ld\n", old, mouse_mode);
}

#define TICS_ON(ti) (((ti)&TICS_MASK)!=NO_TICS)

void GnuPlot::UpdateStatusLine()
{
	UpdateStatusLineWithMouseSetting(term, &mouse_setting);
}

void GnuPlot::UpdateStatusLineWithMouseSetting(GpTermEntry * pTerm, mouse_setting_t * ms)
{
	char s0[256], * sp;
	s0[0] = 0;
	// This suppresses mouse coordinate update after a ^C 
	if(TermInitialised && ms->on) {
		if(!IsAlmost2D()) {
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
			sprintf(s0, format, _3DBlk.SurfaceRotX, _3DBlk.SurfaceRotZ, _3DBlk.SurfaceScale, _3DBlk.SurfaceZScale);
		}
		else if(!TICS_ON(AxS[SECOND_X_AXIS].ticmode) && !TICS_ON(AxS[SECOND_Y_AXIS].ticmode)) {
			// only first X and Y axis are in use 
			sp = GetAnnotateString(s0, _Mse.RealPos.x, _Mse.RealPos.y, mouse_mode, mouse_alt_string);
			if(_Mse.Ruler.on)
				GetRulerString(sp, _Mse.RealPos.x, _Mse.RealPos.y);
		}
		else {
			// X2 and/or Y2 are in use: use more verbose format 
			sp = s0;
			if(TICS_ON(AxS[FIRST_X_AXIS].ticmode)) {
				sp = stpcpy(sp, "x=");
				sp = MkStr(sp, _Mse.RealPos.x, FIRST_X_AXIS);
				*sp++ = ' ';
			}
			if(TICS_ON(AxS[FIRST_Y_AXIS].ticmode)) {
				sp = stpcpy(sp, "y=");
				sp = MkStr(sp, _Mse.RealPos.y, FIRST_Y_AXIS);
				*sp++ = ' ';
			}
			if(TICS_ON(AxS[SECOND_X_AXIS].ticmode)) {
				sp = stpcpy(sp, "x2=");
				sp = MkStr(sp, _Mse.RealPos2.x, SECOND_X_AXIS);
				*sp++ = ' ';
			}
			if(TICS_ON(AxS[SECOND_Y_AXIS].ticmode)) {
				sp = stpcpy(sp, "y2=");
				sp = MkStr(sp, _Mse.RealPos2.y, SECOND_Y_AXIS);
				*sp++ = ' ';
			}
			if(_Mse.Ruler.on) {
				// ruler on? then also print distances to ruler 
				if(TICS_ON(AxS[FIRST_X_AXIS].ticmode)) {
					stpcpy(sp, "dx=");
					sprintf(sp+3, mouse_setting.fmt, DIST(_Mse.RealPos.x, _Mse.Ruler.RealPos.x, FIRST_X_AXIS));
					sp += strlen(sp);
				}
				if(TICS_ON(AxS[FIRST_Y_AXIS].ticmode)) {
					stpcpy(sp, "dy=");
					sprintf(sp+3, mouse_setting.fmt, DIST(_Mse.RealPos.y, _Mse.Ruler.RealPos.y, FIRST_Y_AXIS));
					sp += strlen(sp);
				}
				if(TICS_ON(AxS[SECOND_X_AXIS].ticmode)) {
					stpcpy(sp, "dx2=");
					sprintf(sp+4, mouse_setting.fmt, DIST(_Mse.RealPos2.x, _Mse.Ruler.RealPos2.x, SECOND_X_AXIS));
					sp += strlen(sp);
				}
				if(TICS_ON(AxS[SECOND_Y_AXIS].ticmode)) {
					stpcpy(sp, "dy2=");
					sprintf(sp+4, mouse_setting.fmt, DIST(_Mse.RealPos2.y, _Mse.Ruler.RealPos2.y, SECOND_Y_AXIS));
					sp += strlen(sp);
				}
			}
			*--sp = 0;      /* delete trailing space */
		}
		if(pTerm->put_tmptext && *s0)
			(pTerm->put_tmptext)(pTerm, 0, s0);
	}
}

//void recalc_statusline()
void GnuPlot::RecalcStatusLine()
{
	MousePosToGraphPosReal(_Mse.Pos, &_Mse.RealPos.x, &_Mse.RealPos.y, &_Mse.RealPos2.x, &_Mse.RealPos2.y);
	UpdateStatusLine();
}
//
// handlers for user's actions
//
char * GnuPlot::BuiltinAutoscale(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-autoscale` (set autoscale keepfix; replot)";
	}
	else {
		DoStringReplot(term, "set autoscale keepfix");
		return (char *)0;
	}
}

char * GnuPlot::BuiltinToggleBorder(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-toggle-border`";
	else {
		// EAM July 2009  Cycle through border settings
		// - no border
		// - last border requested by the user
		// - default border
		// - (3D only) full border
		if(Gg.draw_border == 0 && Gg.draw_border != Gg.user_border)
			Gg.draw_border = Gg.user_border;
		else if(Gg.draw_border == Gg.user_border && Gg.draw_border != 31)
			Gg.draw_border = 31;
		else if(Gg.Is3DPlot && Gg.draw_border == 31)
			Gg.draw_border = 4095;
		else
			Gg.draw_border = 0;
		DoStringReplot(term, "");
		return (char *)0;
	}
}

char * GnuPlot::BuiltinReplot(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-replot`";
	else {
		DoStringReplot(term, "");
		return (char *)0;
	}
}

char * GnuPlot::BuiltinToggleGrid(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-toggle-grid`";
	}
	else {
		if(!SomeGridSelected())
			DoStringReplot(term, "set grid");
		else
			DoStringReplot(term, "unset grid");
		return (char *)0;
	}
}

char * GnuPlot::BuiltinHelp(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-help`";
	}
	else {
		fprintf(stderr, "\n");
		BindDisplay((char *)0); // display all bindings 
		RestorePrompt();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinSetPlotsVisible(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-set-plots-visible`";
	}
	else {
		if(pTerm->modify_plots)
			pTerm->modify_plots(MODPLOTS_SET_VISIBLE, -1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinSetPlotsInvisible(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-set-plots-invisible`";
	}
	else {
		if(pTerm->modify_plots)
			pTerm->modify_plots(MODPLOTS_SET_INVISIBLE, -1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinInvertPlotVisibilities(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-invert-plot-visibilities`";
	}
	else {
		if(pTerm->modify_plots)
			pTerm->modify_plots(MODPLOTS_INVERT_VISIBILITIES, -1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinToggleLog(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-toggle-log` y logscale for plots, z and cb for splots";
	if(Gg.VolatileData)
		IntWarn(NO_CARET, "Cannot toggle log scale for volatile data");
	else if((Gg.ColorBox.bounds.xleft < _Mse.Pos.x && _Mse.Pos.x < Gg.ColorBox.bounds.xright) && (Gg.ColorBox.bounds.ybot  < _Mse.Pos.y && _Mse.Pos.y < Gg.ColorBox.bounds.ytop))
		DoStringReplot(pTerm, AxS.__CB().log ? "unset log cb" : "set log cb");
	else if(Gg.Is3DPlot && !_3DBlk.splot_map)
		DoStringReplot(pTerm, AxS.__Z().log ? "unset log z" : "set log z");
	else
		DoStringReplot(pTerm, AxS[FIRST_Y_AXIS].log ? "unset log y" : "set log y");
	return (char *)0;
}

char * GnuPlot::BuiltinNearestLog(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-nearest-log` toggle logscale of axis nearest cursor";
	if((Gg.ColorBox.bounds.xleft < _Mse.Pos.x && _Mse.Pos.x < Gg.ColorBox.bounds.xright) && (Gg.ColorBox.bounds.ybot < _Mse.Pos.y && _Mse.Pos.y < Gg.ColorBox.bounds.ytop)) {
		DoStringReplot(pTerm, AxS.__CB().log ? "unset log cb" : "set log cb");
	}
	else if(Gg.Is3DPlot && !_3DBlk.splot_map) {
		DoStringReplot(pTerm, AxS.__Z().log ? "unset log z" : "set log z");
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
		if(_Mse.Pos.y < V.BbPlot.ybot + (V.BbPlot.ytop - V.BbPlot.ybot) / 4 && _Mse.Pos.x > V.BbPlot.xleft && _Mse.Pos.x < V.BbPlot.xright)
			change_x1 = TRUE;
		if(_Mse.Pos.x < V.BbPlot.xleft + (V.BbPlot.xright - V.BbPlot.xleft) / 4 && _Mse.Pos.y > V.BbPlot.ybot && _Mse.Pos.y < V.BbPlot.ytop)
			change_y1 = TRUE;
		if(_Mse.Pos.y > V.BbPlot.ytop - (V.BbPlot.ytop - V.BbPlot.ybot) / 4 && _Mse.Pos.x > V.BbPlot.xleft && _Mse.Pos.x < V.BbPlot.xright)
			change_x2 = TRUE;
		if(_Mse.Pos.x > V.BbPlot.xright - (V.BbPlot.xright - V.BbPlot.xleft) / 4 && _Mse.Pos.y > V.BbPlot.ybot && _Mse.Pos.y < V.BbPlot.ytop)
			change_y2 = TRUE;
		if(change_x1)
			DoString(AxS[FIRST_X_AXIS].log ? "unset log x" : "set log x");
		if(change_y1)
			DoString(AxS[FIRST_Y_AXIS].log ? "unset log y" : "set log y");
		if(change_x2 && !_3DBlk.splot_map)
			DoString(AxS[SECOND_X_AXIS].log ? "unset log x2" : "set log x2");
		if(change_y2 && !_3DBlk.splot_map)
			DoString(AxS[SECOND_Y_AXIS].log ? "unset log y2" : "set log y2");
		if(!change_x1 && !change_y1 && _3DBlk.splot_map)
			DoStringReplot(pTerm, AxS.__Z().log ? "unset log z" : "set log z");
		if(change_x1 || change_y1 || change_x2 || change_y2)
			DoStringReplot(pTerm, "");
	}
	return (char *)0;
}

char * GnuPlot::BuiltinToggleMouse(GpEvent * ge, GpTermEntry * pTerm)
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
	if(pTerm->set_cursor)
		pTerm->set_cursor(pTerm, 0, 0, 0);
	UpdateStatusLine();
	return (char *)0;
}

char * GnuPlot::BuiltinToggleRuler(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-toggle-ruler`";
	}
	else if(!pTerm->set_ruler)
		return (char *)0;
	else {
		if(_Mse.Ruler.on) {
			TurnRulerOff(pTerm);
			if(display_ipc_commands())
				fprintf(stderr, "turning ruler off.\n");
		}
		else if(IsAlmost2D()) {
			// only allow ruler, if the plot is 2d or a 3d `map' 
			udvt_entry * u;
			_Mse.Ruler.on = true;
			_Mse.Ruler.Pos.x = ge->mx;
			_Mse.Ruler.Pos.y = ge->my;
			MousePosToGraphPosReal(_Mse.Ruler.Pos, &_Mse.Ruler.RealPos.x, &_Mse.Ruler.RealPos.y, &_Mse.Ruler.RealPos2.x, &_Mse.Ruler.RealPos2.y);
			pTerm->set_ruler(pTerm, _Mse.Ruler.Pos.x, _Mse.Ruler.Pos.y);
			if((u = Ev.AddUdvByName("MOUSE_RULER_X"))) {
				Gcomplex(&u->udv_value, _Mse.Ruler.RealPos.x, 0);
			}
			if((u = Ev.AddUdvByName("MOUSE_RULER_Y"))) {
				Gcomplex(&u->udv_value, _Mse.Ruler.RealPos.y, 0);
			}
			if(display_ipc_commands()) {
				fprintf(stderr, "turning ruler on.\n");
			}
		}
		UpdateStatusLine();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinDecrementMouseMode(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-previous-mouse-format`";
	}
	else {
		IncrMouseMode(-1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinIncrementMouseMode(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-next-mouse-format`";
	}
	else {
		IncrMouseMode(1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinTogglePolarDistance(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-toggle-polardistance`";
	}
	else {
		if(++mouse_setting.polardistance > 2) 
			mouse_setting.polardistance = 0;
		// values: 0 (no polar coordinates), 1 (polar coordinates), 2 (tangent instead of angle) 
		pTerm->set_cursor(pTerm, (mouse_setting.polardistance ? -3 : -4), ge->mx, ge->my); /* change cursor type */
		UpdateStatusLine();
		if(display_ipc_commands()) {
			fprintf(stderr, "distance to ruler will %s be shown in polar coordinates.\n", mouse_setting.polardistance ? "" : "not");
		}
		return (char *)0;
	}
}

char * GnuPlot::BuiltinToggleVerbose(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-toggle-verbose`";
	}
	else {
		// this is tricky as the command itself modifies the state of display_ipc_commands() 
		if(display_ipc_commands()) {
			fprintf(stderr, "echoing of communication commands is turned off.\n");
		}
		toggle_display_of_ipc_commands();
		if(display_ipc_commands()) {
			fprintf(stderr, "communication commands will be echoed.\n");
		}
		return (char *)0;
	}
}

char * GnuPlot::BuiltinToggleRatio(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-toggle-ratio`";
	else {
		if(V.AspectRatio == 0.0f)
			DoStringReplot(pTerm, "set size ratio -1");
		else if(V.AspectRatio == 1.0f)
			DoStringReplot(pTerm, "set size nosquare");
		else
			DoStringReplot(pTerm, "set size square");
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomNext(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-next` go to next zoom in the zoom stack";
	else {
		ZoomNext(pTerm);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomPrevious(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-previous` go to previous zoom in the zoom stack";
	else {
		ZoomPrevious(pTerm);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinUnzoom(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-unzoom`";
	}
	else {
		ZoomUnzoom(pTerm);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinRotateRight(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`scroll right in 2d, rotate right in 3d`; <Shift> faster";
	else {
		if(Gg.Is3DPlot)
			ChangeView(pTerm, 0, -1);
		else {
			int k = (_Mse.ModifierMask & Mod_Shift) ? 3 : 1;
			while(k-- > 0)
				DoZoomScrollRight();
		}
		return (char *)0;
	}
}

char * GnuPlot::BuiltinRotateLeft(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`scroll left in 2d, rotate left in 3d`; <Shift> faster";
	else {
		if(Gg.Is3DPlot)
			ChangeView(pTerm, 0, 1);
		else {
			int k = (_Mse.ModifierMask & Mod_Shift) ? 3 : 1;
			while(k-- > 0)
				DoZoomScrollLeft();
		}
		return (char *)0;
	}
}

char * GnuPlot::BuiltinRotateUp(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`scroll up in 2d, rotate up in 3d`; <Shift> faster";
	if(Gg.Is3DPlot)
		ChangeView(pTerm, 1, 0);
	else {
		int k = (_Mse.ModifierMask & Mod_Shift) ? 3 : 1;
		while(k-- > 0)
			DoZoomScrollUp();
	}
	return (char *)0;
}

char * GnuPlot::BuiltinRotateDown(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`scroll down in 2d, rotate down in 3d`; <Shift> faster";
	else {
		if(Gg.Is3DPlot)
			ChangeView(pTerm, -1, 0);
		else {
			int k = (_Mse.ModifierMask & Mod_Shift) ? 3 : 1;
			while(k-- > 0)
				DoZoomScrollDown();
		}
		return (char *)0;
	}
}

char * GnuPlot::BuiltinAzimuthLeft(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`rotate azimuth left in 3d`; <ctrl> faster";
	else {
		if(Gg.Is3DPlot)
			ChangeAzimuth(pTerm, -1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinAzimuthRight(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`rotate azimuth right in 3d`; <ctrl> faster";
	else {
		if(Gg.Is3DPlot)
			ChangeAzimuth(pTerm, 1);
		return (char *)0;
	}
}

char * GnuPlot::BuiltinCancelZoom(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge) {
		return "`builtin-cancel-zoom` cancel zoom region";
	}
	else if(!_Mse.SettingZoomRegion)
		return (char *)0;
	else {
		if(pTerm->set_cursor)
			pTerm->set_cursor(pTerm, 0, 0, 0);
		_Mse.SettingZoomRegion = false;
		if(display_ipc_commands()) {
			fprintf(stderr, "zooming cancelled.\n");
		}
		return (char *)0;
	}
}
//
// Check whether this event is bound to a command.
// If so return a pointer to the binding, otherwise return NULL.
//
GnuPlot::bind_t * GnuPlot::GetBinding(GpEvent * ge, bool current)
{
	int c, par2;
	bind_t * ptr;
	bind_t keypress;
	if(oneof2(ge->type, GE_buttonpress, GE_buttonrelease)) {
		int b = ge->par1;
		c = (b == 3) ? GP_Button3 : (b == 2) ? GP_Button2 : GP_Button1;
		par2 = 0;
	}
	else {
		c = ge->par1;
		if((_Mse.ModifierMask & Mod_Shift) && ((c & 0xff) == 0))
			c = toupper(c);
		par2 = ge->par2;
	}
	if(!_Mse.P_Bindings)
		BindInstallDefaultBindings();
	BindClear(&keypress);
	keypress.key = c;
	keypress.modifier = _Mse.ModifierMask;
	for(ptr = _Mse.P_Bindings; ptr; ptr = ptr->next) {
		if(BindMatches(&keypress, ptr)) {
			// Always honor keys set with "bind all" 
			if(ptr->allwindows && ptr->command)
				return ptr;
			// But otherwise ignore inactive windows 
			else if(!current)
				break;
			// Let user defined bindings overwrite the builtin bindings 
			else if((par2 & 1) == 0 && ptr->command)
				return ptr;
			else if(ptr->HandlerFunc) //else if(ptr->builtin)
				return ptr;
			else
				FPRINTF((stderr, "%s:%d protocol error\n", __FILE__, __LINE__));
		}
	}
	return NULL;
}

void GnuPlot::EventKeyPress(GpEvent * ge, GpTermEntry * pTerm, bool current)
{
	int x, y;
	int par2;
	bind_t * ptr;
	bind_t keypress;
	udvt_entry * keywin;
	int c = ge->par1;
	if((_Mse.ModifierMask & Mod_Shift) && ((c & 0xff) == 0))
		c = toupper(c);
	par2 = ge->par2;
	x = ge->mx;
	y = ge->my;
	BindClear(&keypress);
	keypress.key = c;
	keypress.modifier = _Mse.ModifierMask;
	// 
	// On 'pause mouse keypress' in active window export current keypress
	// and mouse coords to user variables. A key with 'bind all' terminates
	// a pause even from non-active windows.
	// Ignore NULL keypress.
	// 
	// If we are paused for a keystroke, this takes precendence over normal
	// key bindings. Otherwise, for example typing 'm' would turn off mousing,
	// which is a bad thing if you are in the  middle of a mousing operation.
	// 
	if((paused_for_mouse & PAUSE_KEYSTROKE) && (c > '\0') && current) {
		LoadMouseVariables(x, y, FALSE, c);
		return;
	}
	if(!(ptr = GetBinding(ge, current)))
		return;
	if((keywin = Ev.AddUdvByName("MOUSE_KEY_WINDOW")))
		Ginteger(&keywin->udv_value, ge->winid);
	if(current)
		LoadMouseVariables(x, y, FALSE, c);
	else
		LoadMouseVariables(0, 0, FALSE, c);
	if(ptr->allwindows && ptr->command)
		DoString(ptr->command);
	else if((par2 & 1) == 0 && ptr->command)
		DoString(ptr->command);
	//else if(ptr->builtin)
		//ptr->builtin(ge);
	else if(ptr->HandlerFunc) {
		(this->*ptr->HandlerFunc)(ge, pTerm);
	}
}

void GnuPlot::ChangeView(GpTermEntry * pTerm, int x, int z)
{
	if(_Mse.ModifierMask & Mod_Shift) {
		x *= 10;
		z *= 10;
	}
	if(x) {
		_3DBlk.SurfaceRotX += x;
		if(_3DBlk.SurfaceRotX < 0.0f)
			_3DBlk.SurfaceRotX += 360.0f;
		if(_3DBlk.SurfaceRotX > 360.0f)
			_3DBlk.SurfaceRotX -= 360.0f;
	}
	if(z) {
		_3DBlk.SurfaceRotZ += z;
		if(_3DBlk.SurfaceRotZ < 0.0f)
			_3DBlk.SurfaceRotZ += 360.0f;
		if(_3DBlk.SurfaceRotZ > 360.0f)
			_3DBlk.SurfaceRotZ -= 360.0f;
	}
	if(x || z) {
		Ev.FillGpValFoat("GPVAL_VIEW_ROT_X", _3DBlk.SurfaceRotX);
		Ev.FillGpValFoat("GPVAL_VIEW_ROT_Z", _3DBlk.SurfaceRotZ);
	}
	if(display_ipc_commands()) {
		fprintf(stderr, "changing view to %f, %f.\n", _3DBlk.SurfaceRotX, _3DBlk.SurfaceRotZ);
	}
	DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, NORMAL_REPLOT);
	if(IsAlmost2D()) {
		// 2D plot, or suitably aligned 3D plot: update statusline 
		if(!pTerm->put_tmptext)
			return;
		RecalcStatusLine();
	}
}

void GnuPlot::ChangeAzimuth(GpTermEntry * pTerm, int x)
{
	// Disable for 2D projections 
	if(_3DBlk.xz_projection || _3DBlk.yz_projection)
		return;
	// Can't use Mod_Shift because keyboards differ on the  shift status of the < and > keys. 
	if(_Mse.ModifierMask & Mod_Ctrl)
		x *= 10;
	if(x) {
		_3DBlk.Azimuth += x;
		if(_3DBlk.Azimuth < 0.0f)
			_3DBlk.Azimuth += 360.0f;
		if(_3DBlk.Azimuth > 360.0f)
			_3DBlk.Azimuth -= 360.0f;
		Ev.FillGpValFoat("GPVAL_VIEW_AZIMUTH", _3DBlk.Azimuth);
	}
	if(display_ipc_commands())
		fprintf(stderr, "changing azimuth to %f.\n", _3DBlk.Azimuth);
	DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, NORMAL_REPLOT);
}

//int is_mouse_outside_plot(void)
int GnuPlot::IsMouseOutsidePlot()
{
#define CHECK_AXIS_OUTSIDE(v, idx) (AxS[idx].min < VERYLARGE && AxS[idx].max > -VERYLARGE && (((v) < AxS[idx].min && (v) < AxS[idx].max) || ((v) > AxS[idx].min && (v) > AxS[idx].max)))
	return CHECK_AXIS_OUTSIDE(_Mse.RealPos.x, FIRST_X_AXIS) || CHECK_AXIS_OUTSIDE(_Mse.RealPos.y, FIRST_Y_AXIS) || CHECK_AXIS_OUTSIDE(_Mse.RealPos2.x, SECOND_X_AXIS) || CHECK_AXIS_OUTSIDE(_Mse.RealPos2.y, SECOND_Y_AXIS);
#undef CHECK_AXIS_OUTSIDE
}
//
// Return a new upper or lower axis limit that is a linear
// combination of the current limits.
//
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
	Gr.RetainOffsets = true;
	DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	if(msg[0] && display_ipc_commands()) {
		fputs(msg, stderr); fputs("\n", stderr);
	}
}
//
// Scroll left. 
//
void GnuPlot::DoZoomScrollLeft()
{
	ZoomRescale_XYX2Y2(1.1, -0.1, 1,   0, 1.1, -0.1, 1,   0, 0.1, 0.9, 0,   1, 0.1, 0.9, 0,   1, "scroll left.\n");
}
//
// Scroll right. 
//
void GnuPlot::DoZoomScrollRight()
{
	ZoomRescale_XYX2Y2(0.9,  0.1, 1,    0, 0.9,  0.1, 1,    0, -0.1, 1.1, 0,    1, -0.1, 1.1, 0,    1, "scroll right");
}
//
// Scroll up. 
//
void GnuPlot::DoZoomScrollUp()
{
	ZoomRescale_XYX2Y2(1,    0, 0.9,  0.1, 1,    0, 0.9,  0.1, 0,    1, -0.1, 1.1, 0,    1, -0.1, 1.1, "scroll up");
}
//
// Scroll down. 
//
void GnuPlot::DoZoomScrollDown()
{
	ZoomRescale_XYX2Y2(1,   0, 1.1, -0.1, 1,   0, 1.1, -0.1, 0,   1, 0.1, 0.9, 0,   1, 0.1, 0.9, "scroll down");
}
//
// Return new lower and upper axis limits from expanding current limits
// relative to current mouse position.
//
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
void GnuPlot::ZoomInX(int zoomKey)
{
	Gr.RetainOffsets = true;
	if(IsMouseOutsidePlot()) {
		// zoom in (X axis only) 
		double w1 = (zoomKey == '+') ? 23.0/25.0 : 23.0/21.0;
		double w2 = (zoomKey == '+') ?  2.0/25.0 : -2.0/21.0;
		ZoomRescale_XYX2Y2(w1, w2, 1, 0, w1, w2, 1, 0,  w2, w1, 0, 1, w2, w1, 0, 1, ((zoomKey == '+') ? "zoom in X" : "zoom out X"));
	}
	else {
		double xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max;
		double scale = (zoomKey == '+') ? 0.75 : 1.25;
		RescaleAroundMouse(&xmin,  &xmax,  FIRST_X_AXIS,  _Mse.RealPos.x,  scale);
		RescaleAroundMouse(&x2min, &x2max, SECOND_X_AXIS, _Mse.RealPos2.x, scale);
		ymin  = Rescale(FIRST_Y_AXIS,  1, 0);
		y2min = Rescale(SECOND_Y_AXIS, 1, 0);
		ymax  = Rescale(FIRST_Y_AXIS,  0, 1);
		y2max = Rescale(SECOND_Y_AXIS, 0, 1);
		DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	}
}

void GnuPlot::DoZoomInX() { ZoomInX('+'); }
void GnuPlot::DoZoomOutX() { ZoomInX('-'); }
//
// Zoom around mouse cursor unless the cursor is outside the graph boundary,
// when it scales around the graph center.
// Syntax: zoom_key == '+' ... zoom in, zoom_key == '-' ... zoom out
//
void GnuPlot::ZoomAroundMouse(int zoom_key)
{
	double xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max;
	if(IsMouseOutsidePlot()) {
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
		double xscale = pow(1.25, zsign * mouse_setting.xmzoom_factor);
		double yscale = pow(1.25, zsign * mouse_setting.ymzoom_factor);
		// {x,y}zoom_factor = 0: not zoom, = 1: 0.8/1.25 zoom 
		RescaleAroundMouse(&xmin,  &xmax,  FIRST_X_AXIS,  _Mse.RealPos.x,  xscale);
		RescaleAroundMouse(&ymin,  &ymax,  FIRST_Y_AXIS,  _Mse.RealPos.y,  yscale);
		RescaleAroundMouse(&x2min, &x2max, SECOND_X_AXIS, _Mse.RealPos2.x, xscale);
		RescaleAroundMouse(&y2min, &y2max, SECOND_Y_AXIS, _Mse.RealPos2.y, yscale);
	}
	Gr.RetainOffsets = true;
	DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
	if(display_ipc_commands())
		fprintf(stderr, "zoom %s.\n", (zoom_key=='+' ? "in" : "out"));
}

void GnuPlot::DoZoomInAroundBouse() { ZoomAroundMouse('+'); }
void GnuPlot::DoZoomOutAroundMouse() { ZoomAroundMouse('-'); }

char * GnuPlot::BuiltinZoomInAroundMouse(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-in` zoom in";
	else {
		DoZoomInAroundBouse();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomOutAroundMouse(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-out` zoom out";
	else {
		DoZoomOutAroundMouse();
		return (char *)0;
	}
}

#if (0) // Not currently used 

char * GnuPlot::BuiltinZoomScrollLeft(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-scroll-left` scroll left";
	else {
		DoZoomScrollLeft();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomScrollRight(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-scroll-right` scroll right";
	else {
		DoZoomScrollRight();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomScrollUp(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-scroll-up` scroll up";
	else {
		DoZoomScrollUp();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomScrollDown(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-scroll-down` scroll down";
	else {
		DoZoomScrollDown();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomInX(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-in-X` zoom in X axis";
	else {
		DoZoomInX();
		return (char *)0;
	}
}

char * GnuPlot::BuiltinZoomOutX(GpEvent * ge, GpTermEntry * pTerm)
{
	if(!ge)
		return "`builtin-zoom-out-X` zoom out X axis";
	else {
		DoZoomOutX();
		return (char *)0;
	}
}
#endif // Not currently used 

void GnuPlot::EventButtonPress(GpEvent * pGe, GpTermEntry * pTerm)
{
	_Mse.Motion = 0;
	int b = pGe->par1;
	_Mse.Pos.x = pGe->mx;
	_Mse.Pos.y = pGe->my;
	_Mse.Button |= (1 << b);
	FPRINTF((stderr, "(event_buttonpress) mouse_x = %d\tmouse_y = %d\n", _Mse.Pos.x, _Mse.Pos.y));
	MousePosToGraphPosReal(_Mse.Pos, &_Mse.RealPos.x, &_Mse.RealPos.y, &_Mse.RealPos2.x, &_Mse.RealPos2.y);
	if(oneof2(b, 4, 6) && /* 4 - wheel up, 6 - wheel left */ (!Pgm.replot_disabled || (E_REFRESH_NOT_OK != Gg.refresh_ok)) && // Use refresh if available 
	    !(paused_for_mouse & PAUSE_BUTTON3)) {
		// Ctrl+Shift+wheel up or Squeeze (not implemented) 
		if((_Mse.ModifierMask & Mod_Ctrl) && (_Mse.ModifierMask & Mod_Shift))
			DoZoomInX();
		// Ctrl+wheel up or Ctrl+stroke 
		else if((_Mse.ModifierMask & Mod_Ctrl))
			DoZoomInAroundBouse();
		// Horizontal stroke (button 6) or Shift+wheel up 
		else if(b == 6 || (_Mse.ModifierMask & Mod_Shift))
			DoZoomScrollLeft();
		// Wheel up (no modifier keys) 
		else
			DoZoomScrollUp();
	}
	else if(oneof2(b, 5, 7) && /* 5 - wheel down, 7 - wheel right */
	    (!Pgm.replot_disabled || (E_REFRESH_NOT_OK != Gg.refresh_ok)) /* Use refresh if available */ && !(paused_for_mouse & PAUSE_BUTTON3)) {
		// Ctrl+Shift+wheel down or Unsqueeze (not implemented) 
		if((_Mse.ModifierMask & Mod_Ctrl) && (_Mse.ModifierMask & Mod_Shift))
			DoZoomOutX();
		// Ctrl+wheel down or Ctrl+stroke 
		else if((_Mse.ModifierMask & Mod_Ctrl))
			DoZoomOutAroundMouse();
		// Horizontal stroke (button 7) or Shift+wheel down 
		else if(b == 7 || (_Mse.ModifierMask & Mod_Shift))
			DoZoomScrollRight();
		// Wheel down (no modifier keys) 
		else
			DoZoomScrollDown();
	}
	else if(IsAlmost2D()) {
		// "pause button1" or "pause any" takes precedence over key bindings 
		if(1 == b) {
			if(paused_for_mouse & PAUSE_BUTTON1) {
				LoadMouseVariables(_Mse.Pos.x, _Mse.Pos.y, TRUE, b);
				_Mse.TrapRelease = true; // Don't trigger on release also 
				return;
			}
		}
		// In 2D mouse buttons 1-3 are available for "bind" commands 
		if(oneof3(b, 1, 2, 3)) {
			if(GetBinding(pGe, TRUE)) {
				EventKeyPress(pGe, pTerm, TRUE);
				return;
			}
		}
		if(!_Mse.SettingZoomRegion) {
			if(3 == b && (!Pgm.replot_disabled || (E_REFRESH_NOT_OK != Gg.refresh_ok)) /* Use refresh if available */ && !(paused_for_mouse & PAUSE_BUTTON3)) {
				/* start zoom; but ignore it when
				 *   - replot is disabled, e.g. with inline data, or
				 *   - during 'pause mouse'
				 * allow zooming during 'pause mouse key' */
				_Mse.SettingZoom = _Mse.Pos;
				_Mse.SettingZoomRegion = true;
				if(pTerm->set_cursor) {
					int mv_mouse_x, mv_mouse_y;
					if(mouse_setting.annotate_zoom_box && pTerm->put_tmptext) {
						double _real_x, _real_y, _real_x2, _real_y2;
						char s[64];
						// tell driver annotations 
						MousePosToGraphPosReal(_Mse.Pos, &_real_x, &_real_y, &_real_x2, &_real_y2);
						sprintf(s, zoombox_format(), _real_x, _real_y);
						pTerm->put_tmptext(pTerm, 1, s);
						pTerm->put_tmptext(pTerm, 2, s);
					}
					/* displace mouse in order not to start with an empty zoom box */
					mv_mouse_x = pTerm->MaxX / 20;
					mv_mouse_y = (pTerm->MaxX == pTerm->MaxY) ? mv_mouse_x : (int)((mv_mouse_x * (double)pTerm->MaxY) / pTerm->MaxX);
					mv_mouse_x += _Mse.Pos.x;
					mv_mouse_y += _Mse.Pos.y;
					// change cursor type 
					pTerm->set_cursor(pTerm, 3, 0, 0);
					// warp pointer 
					if(mouse_setting.warp_pointer)
						pTerm->set_cursor(pTerm, -2, mv_mouse_x, mv_mouse_y);
					// turn on the zoom box 
					pTerm->set_cursor(pTerm, -1, _Mse.SettingZoom.x, _Mse.SettingZoom.y);
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
			int dist_x = _Mse.SettingZoom.x - _Mse.Pos.x;
			int dist_y = _Mse.SettingZoom.y - _Mse.Pos.y;
			int dist = static_cast<int>(sqrt((double)(dist_x * dist_x + dist_y * dist_y)));
			if(1 == b || 2 == b) {
				/* zoom region is finished by the `wrong' button.
				 * `trap' the next button-release event so that
				 * it won't trigger the actions which are bound
				 * to these events.
				 */
				_Mse.TrapRelease = true;
			}
			if(pTerm->set_cursor) {
				pTerm->set_cursor(pTerm, 0, 0, 0);
				if(mouse_setting.annotate_zoom_box && pTerm->put_tmptext) {
					pTerm->put_tmptext(pTerm, 1, "");
					pTerm->put_tmptext(pTerm, 2, "");
				}
			}
			if(dist > 10 /* more ore less arbitrary */) {
				double xmin, ymin, x2min, y2min;
				double xmax, ymax, x2max, y2max;
				MousePosToGraphPosReal(_Mse.SettingZoom, &xmin, &ymin, &x2min, &y2min);
				xmax = _Mse.RealPos.x;
				x2max = _Mse.RealPos2.x;
				ymax = _Mse.RealPos.y;
				y2max = _Mse.RealPos2.y;
				// keep the axes (no)reversed as they are now 
#define rev(a1, a2, A) if(sgn(a2-a1) != sgn(AxS[A].GetRange())) { Exchange(&a1, &a2); }
				rev(xmin,  xmax,  FIRST_X_AXIS);
				rev(ymin,  ymax,  FIRST_Y_AXIS);
				rev(x2min, x2max, SECOND_X_AXIS);
				rev(y2min, y2max, SECOND_Y_AXIS);
#undef rev
				DoZoom(xmin, ymin, x2min, y2min, xmax, ymax, x2max, y2max);
				if(display_ipc_commands()) {
					fprintf(stderr, "zoom region finished.\n");
				}
			}
			else {
				/* silently ignore a tiny zoom box. This might
				 * happen, if the user starts and finishes the
				 * zoom box at the same position. */
			}
			_Mse.SettingZoomRegion = false;
		}
	}
	else {
		if(pTerm->set_cursor) {
			if(_Mse.Button & (1 << 1) || _Mse.Button & (1 << 3))
				pTerm->set_cursor(pTerm, 1, 0, 0);
			else if(_Mse.Button & (1 << 2))
				pTerm->set_cursor(pTerm, 2, 0, 0);
		}
	}
	_Mse.Start = _Mse.Pos;
	_Mse.ZeroRotZ = _3DBlk.SurfaceRotZ + (360.0f * _Mse.Pos.x) / pTerm->MaxX;
	// zero_rot_x = _3DBlk.SurfaceRotX - 180.0 * _Mse.Pos.y / pTerm->MaxY; 
	_Mse.ZeroRotX = _3DBlk.SurfaceRotX - (360.0f * _Mse.Pos.y) / pTerm->MaxY;
}

void GnuPlot::EventButtonRelease(GpEvent * pGe, GpTermEntry * pTerm)
{
	int doubleclick;
	int b = pGe->par1;
	_Mse.Pos.x = pGe->mx;
	_Mse.Pos.y = pGe->my;
	doubleclick = pGe->par2;
	_Mse.Button &= ~(1 << b);    /* remove button */
	if(_Mse.SettingZoomRegion)
		return;
	else if(_Mse.TrapRelease) { // FIXME:  This mechanism may no longer be needed 
		_Mse.TrapRelease = false;
		return;
	}
	else {
		// binding takes precedence over default action 
		if(oneof3(b, 1, 2, 3)) {
			if(GetBinding(pGe, TRUE))
				return;
		}
		MousePosToGraphPosReal(_Mse.Pos, &_Mse.RealPos.x, &_Mse.RealPos.y, &_Mse.RealPos2.x, &_Mse.RealPos2.y);
		FPRINTF((stderr, "MOUSE.C: doublclick=%i, set=%i, motion=%i, ALMOST2D=%i\n", (int)doubleclick, (int)mouse_setting.doubleclick, (int)_Mse.Motion, (int)IsAlmost2D()));
		if(IsAlmost2D()) {
			char s0[256];
			if(b == 1 && pTerm->set_clipboard && ((doubleclick <= mouse_setting.doubleclick) || !mouse_setting.doubleclick)) {
				// put coordinates to clipboard. For 3d plots this takes
				// only place, if the user didn't drag (rotate) the plot 
				if(!Gg.Is3DPlot || !_Mse.Motion) {
					GetAnnotateString(s0, _Mse.RealPos.x, _Mse.RealPos.y, mouse_mode, mouse_alt_string);
					pTerm->set_clipboard(pTerm, s0);
					if(display_ipc_commands())
						fprintf(stderr, "put `%s' to clipboard.\n", s0);
				}
			}
			if(b == 2) {
				// draw temporary annotation or label. For 3d plots this is
				// only done if the user didn't drag (scale) the plot 
				if(!Gg.Is3DPlot || !_Mse.Motion) {
					GetAnnotateString(s0, _Mse.RealPos.x, _Mse.RealPos.y, mouse_mode, mouse_alt_string);
					if(mouse_setting.label) {
						if(_Mse.ModifierMask & Mod_Ctrl) {
							RemoveLabel(pTerm, _Mse.Pos.x, _Mse.Pos.y);
						}
						else {
							PutLabel(pTerm, s0, _Mse.RealPos.x, _Mse.RealPos.y);
						}
					}
					else {
						int x = _Mse.Pos.x;
						int y = _Mse.Pos.y;
						int dx = pTerm->TicH;
						int dy = pTerm->TicV;
						pTerm->linewidth(pTerm, Gg.border_lp.l_width);
						pTerm->linetype(pTerm, Gg.border_lp.l_type);
						pTerm->move(pTerm, x - dx, y);
						pTerm->vector(pTerm, x + dx, y);
						pTerm->move(pTerm, x, y - dy);
						pTerm->vector(pTerm, x, y + dy);
						pTerm->justify_text(pTerm, LEFT);
						pTerm->put_text(pTerm, x + dx / 2, y + dy / 2 + pTerm->ChrV / 3, s0);
						(pTerm->text)(pTerm);
					}
				}
			}
		}
		if(Gg.Is3DPlot && (b == 1 || b == 2 || b == 3)) {
			if(!!(_Mse.ModifierMask & Mod_Ctrl) && !_Mse.NeedReplot) {
				// redraw the 3d plot if its last redraw was 'quick' (only axes) because modifier key was pressed 
				DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, NORMAL_REPLOT);
			}
			else if(b==1) {
				// Needed if the previous plot was QUICK_REFRESH 
				DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, NORMAL_REPLOT);
			}
			if(pTerm->set_cursor)
				pTerm->set_cursor(pTerm, (_Mse.Button & (1 << 1)) ? 1 : (_Mse.Button & (1 << 2)) ? 2 : 0, 0, 0);
		}
		// Export current mouse coords to user-accessible variables also 
		LoadMouseVariables(_Mse.Pos.x, _Mse.Pos.y, TRUE, b);
		UpdateStatusLine();
	}
}

void GnuPlot::EventMotion(GpEvent * pGe, GpTermEntry * pTerm)
{
	_Mse.Motion = 1;
	_Mse.Pos.x = pGe->mx;
	_Mse.Pos.y = pGe->my;
	if(Gg.Is3DPlot && !_3DBlk.splot_map) { // Rotate the surface if it is 3D graph but not "set view map". 
		bool redraw = FALSE;
		if(_Mse.Button & (1 << 1)) {
			// dragging with button 1 -> rotate 
			//_3DBlk.SurfaceRotX = floor(0.5 + zero_rot_x + 180.0 * _Mse.Pos.y / pTerm->MaxY);
			_3DBlk.SurfaceRotX = floorf(0.5f + fmodf(_Mse.ZeroRotX + 360.0f * _Mse.Pos.y / pTerm->MaxY, 360));
			if(_3DBlk.SurfaceRotX < 0.0f)
				_3DBlk.SurfaceRotX += 360.0f;
			if(_3DBlk.SurfaceRotX > 360.0f)
				_3DBlk.SurfaceRotX -= 360.0f;
			_3DBlk.SurfaceRotZ = floorf(0.5f + fmodf(_Mse.ZeroRotZ - 360.0f * _Mse.Pos.x / pTerm->MaxX, 360));
			if(_3DBlk.SurfaceRotZ < 0.0f)
				_3DBlk.SurfaceRotZ += 360.0f;
			redraw = TRUE;
		}
		else if(_Mse.Button & (1 << 2)) {
			// dragging with button 2 -> scale or changing ticslevel.
			// we compare the movement in x and y direction, and
			// change either scale or zscale 
			double relx = (double)abs(_Mse.Pos.x - _Mse.Start.x) / (double)pTerm->TicH;
			double rely = (double)abs(_Mse.Pos.y - _Mse.Start.y) / (double)pTerm->TicV;
			if(_Mse.ModifierMask & Mod_Shift) {
				_3DBlk.xyplane.z += (1 + fabs(_3DBlk.xyplane.z)) * (_Mse.Pos.y - _Mse.Start.y) * 2.0 / pTerm->MaxY;
			}
			else {
				if(relx > rely) {
					_3DBlk.SurfaceLScale += (_Mse.Pos.x - _Mse.Start.x) * 2.0f / pTerm->MaxX;
					_3DBlk.SurfaceScale = static_cast<float>(exp(_3DBlk.SurfaceLScale));
					SETMAX(_3DBlk.SurfaceScale, 0.0f);
				}
				else {
					if(disable_mouse_z && (_Mse.Pos.y-_Mse.Start.y > 0))
						;
					else {
						_3DBlk.SurfaceZScale += (_Mse.Pos.y - _Mse.Start.y) * 2.0f / pTerm->MaxY;
						disable_mouse_z = FALSE;
					}
					SETMAX(_3DBlk.SurfaceZScale, 0.0f);
				}
			}
			// reset the start values 
			_Mse.Start = _Mse.Pos;
			redraw = TRUE;
		}
		else if(_Mse.Button & (1 << 3)) {
			// dragging with button 3 -> change azimuth 
			ChangeAzimuth(pTerm, static_cast<int>((_Mse.Pos.x - _Mse.Start.x) * 90.0 / pTerm->MaxX));
			_Mse.Start.x = _Mse.Pos.x;
			redraw = TRUE;
		}
		if(!IsAlmost2D()) {
			TurnRulerOff(pTerm);
		}
		if(redraw) {
			if(allowmotion) {
				// is processing of motions allowed right now?
				// then replot while
				// disabling further replots until it completes 
				allowmotion = FALSE;
				DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, (_Mse.ModifierMask & Mod_Ctrl) ? AXIS_ONLY_ROTATE : QUICK_REFRESH);
				Ev.FillGpValFoat("GPVAL_VIEW_ROT_X", _3DBlk.SurfaceRotX);
				Ev.FillGpValFoat("GPVAL_VIEW_ROT_Z", _3DBlk.SurfaceRotZ);
				Ev.FillGpValFoat("GPVAL_VIEW_SCALE", _3DBlk.SurfaceScale);
				Ev.FillGpValFoat("GPVAL_VIEW_ZSCALE", _3DBlk.SurfaceZScale);
				Ev.FillGpValFoat("GPVAL_VIEW_AZIMUTH", _3DBlk.Azimuth);
			}
			else {
				_Mse.NeedReplot = true; // postpone the replotting 
			}
		}
	}
	if(IsAlmost2D()) {
		// 2D plot, or suitably aligned 3D plot: update
		// statusline and possibly the zoombox annotation 
		if(pTerm->put_tmptext) {
			MousePosToGraphPosReal(_Mse.Pos, &_Mse.RealPos.x, &_Mse.RealPos.y, &_Mse.RealPos2.x, &_Mse.RealPos2.y);
			UpdateStatusLine();
			if(_Mse.SettingZoomRegion && mouse_setting.annotate_zoom_box) {
				double _real_x, _real_y, _real_x2, _real_y2;
				char s[64];
				MousePosToGraphPosReal(_Mse.Pos, &_real_x, &_real_y, &_real_x2, &_real_y2);
				sprintf(s, zoombox_format(), _real_x, _real_y);
				pTerm->put_tmptext(pTerm, 2, s);
			}
		}
	}
}

void GnuPlot::EventModifier(GpEvent * ge, GpTermEntry * pTerm)
{
	_Mse.ModifierMask = ge->par1;
	if(!_Mse.ModifierMask && Gg.Is3DPlot && (_Mse.Button & ((1 << 1) | (1 << 2))) && !_Mse.NeedReplot) {
		// redraw the 3d plot if modifier key released 
		DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, NORMAL_REPLOT);
	}
}

//void event_plotdone()
void GnuPlot::EventPlotDone(GpTermEntry * pTerm)
{
	if(_Mse.NeedReplot) {
		_Mse.NeedReplot = false;
		DoSave3DPlot(pTerm, _Plt.first_3dplot, _Plt.plot3d_num, (_Mse.ModifierMask & Mod_Ctrl) ? AXIS_ONLY_ROTATE : NORMAL_REPLOT);
	}
	else
		allowmotion = TRUE;
}

//void event_reset(GpEvent * ge)
void GnuPlot::EventReset(GpEvent * ge, GpTermEntry * pTerm)
{
	_Mse.ModifierMask = 0;
	_Mse.Button = 0;
	BuiltinCancelZoom(ge, pTerm);
	if(pTerm && TermInitialised && pTerm->set_cursor) {
		pTerm->set_cursor(pTerm, 0, 0, 0);
		if(mouse_setting.annotate_zoom_box && pTerm->put_tmptext) {
			pTerm->put_tmptext(pTerm, 1, "");
			pTerm->put_tmptext(pTerm, 2, "");
		}
	}
	// This hack is necessary on some systems in order to prevent one
	// character of input from being swallowed when the plot window is
	// closed. But which systems, exactly, and in what circumstances?
	if(paused_for_mouse || !_Plt.interactive) {
		if(pTerm && TermInitialised && (!strncmp("x11", pTerm->name, 3) || !strncmp("wxt", pTerm->name, 3) || !strncmp("qt", pTerm->name, 2)))
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
		ge->par1 = GP_Cancel; // Dummy keystroke 
		ge->par2 = 0; // Not used; could pass window id here? 
		EventKeyPress(ge, pTerm, true);
	}
}

//void do_event(GpEvent * pGe)
void GnuPlot::DoEvent(GpTermEntry * pTerm, GpEvent * pGe)
{
	if(pTerm) {
		// disable `replot` when some data were sent through stdin 
		Pgm.replot_disabled = _Df.plotted_data_from_stdin;
		if(pGe->type) {
			FPRINTF((stderr, "(do_event) type       = %s\n", GE_evt_name(pGe->type)));
			FPRINTF((stderr, "           mx, my     = %d, %d\n", pGe->mx, pGe->my));
			FPRINTF((stderr, "           par1, par2 = %d, %d\n", pGe->par1, pGe->par2));
		}
		switch(pGe->type) {
			case GE_plotdone:
				EventPlotDone(pTerm);
				if(pGe->winid) {
					Gg.current_x11_windowid = pGe->winid;
					UpdateGpvalVariables(pTerm, 6); // fill GPVAL_TERM_WINDOWID 
				}
				break;
			case GE_keypress: EventKeyPress(pGe, pTerm, true); break;
			case GE_keypress_old: EventKeyPress(pGe, pTerm, false); break;
			case GE_modifier: EventModifier(pGe, pTerm); break;
			case GE_motion:
				if(!mouse_setting.on)
					break;
				EventMotion(pGe, pTerm);
				break;
			case GE_buttonpress:
				if(!mouse_setting.on)
					break;
				EventButtonPress(pGe, pTerm);
				break;
			case GE_buttonrelease:
				if(!mouse_setting.on)
					break;
				EventButtonRelease(pGe, pTerm);
				break;
			case GE_replot:
				// auto-generated replot (e.g. from replot-on-resize) 
				// FIXME: more terminals should use this! 
				if(isempty(Pgm.replot_line))
					break;
				if(!strncmp(Pgm.replot_line, "test", 4))
					break;
				if(GPT.Flags & GpTerminalBlock::fMultiplot)
					break;
				DoStringReplot(pTerm, "");
				break;
			case GE_reset:
				EventReset(pGe, pTerm);
				break;
			case GE_fontprops:
	#ifdef X11
				// EAM FIXME:  Despite the name, only X11 uses this to pass font info.	
				// Everyone else passes just the plot height and width.			
				if(sstreq(pTerm->name, "x11")) {
					// These are declared in ../term/x11.trm 
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
				// @fallthrough to cover non-x11 case 
	#endif
				// Other terminals update aspect ratio based on current window size 
				pTerm->TicV = static_cast<uint>(pTerm->TicH * (double)pGe->mx / (double)pGe->my);
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
		Pgm.replot_disabled = false; // enable replot again 
	}
}
// 
// convenience wrapper for do_event();
// returns TRUE if it ends pause mouse;
// currently used by caca.trm, djsvga.trm, and pc.trm 
// 
bool exec_event(char type, int mx, int my, int par1, int par2, int winid)
{
	GpEvent ge;
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

void GnuPlot::DoSave3DPlot(GpTermEntry * pTerm, GpSurfacePoints * pPlots, int pcount, REPLOT_TYPE quick)
{
	if(!pPlots || E_REFRESH_NOT_OK == Gg.refresh_ok) {
		// !plots might happen after the `reset' command for example
		// (reported by Franz Bakan).
		// !refresh_ok can happen for example if log scaling is reset (EAM).
		// replotrequest() should set up everything again in either case.
		ReplotRequest(pTerm);
	}
	else
		Do3DPlot(pTerm, pPlots, pcount, quick);
}
//
// bind related functions
//
void GnuPlot::BindInstallDefaultBindings()
{
	BindRemoveAll();
	BindAppend("a",      0, &GnuPlot::BuiltinAutoscale);
	BindAppend("b", 	 0, &GnuPlot::BuiltinToggleBorder);
	BindAppend("e", 	 0, &GnuPlot::BuiltinReplot);
	BindAppend("g", 	 0, &GnuPlot::BuiltinToggleGrid);
	BindAppend("h", 	 0, &GnuPlot::BuiltinHelp);
	BindAppend("i", 	 0, &GnuPlot::BuiltinInvertPlotVisibilities);
	BindAppend("l", 	 0, &GnuPlot::BuiltinToggleLog);
	BindAppend("L", 	 0, &GnuPlot::BuiltinNearestLog);
	BindAppend("m", 	 0, &GnuPlot::BuiltinToggleMouse);
	BindAppend("r", 	 0, &GnuPlot::BuiltinToggleRuler);
	BindAppend("V", 	 0, &GnuPlot::BuiltinSetPlotsInvisible);
	BindAppend("v", 	 0, &GnuPlot::BuiltinSetPlotsVisible);
	BindAppend("1", 	 0, &GnuPlot::BuiltinDecrementMouseMode);
	BindAppend("2", 	 0, &GnuPlot::BuiltinIncrementMouseMode);
	BindAppend("5", 	 0, &GnuPlot::BuiltinTogglePolarDistance);
	BindAppend("6", 	 0, &GnuPlot::BuiltinToggleVerbose);
	BindAppend("7", 	 0, &GnuPlot::BuiltinToggleRatio);
	BindAppend("n", 	 0, &GnuPlot::BuiltinZoomNext);
	BindAppend("p", 	 0, &GnuPlot::BuiltinZoomPrevious);
	BindAppend("u", 	 0, &GnuPlot::BuiltinUnzoom);
	BindAppend("+", 	 0, &GnuPlot::BuiltinZoomInAroundMouse);
	BindAppend("=", 	 0, &GnuPlot::BuiltinZoomInAroundMouse); // same key as + but no need for Shift 
	BindAppend("-", 	 0, &GnuPlot::BuiltinZoomOutAroundMouse);
	BindAppend("Right",  0, &GnuPlot::BuiltinRotateRight);
	BindAppend("Up",     0, &GnuPlot::BuiltinRotateUp);
	BindAppend("Left",   0, &GnuPlot::BuiltinRotateLeft);
	BindAppend("Down",   0, &GnuPlot::BuiltinRotateDown);
	BindAppend("Opt-<",  0, &GnuPlot::BuiltinAzimuthLeft);
	BindAppend("Opt->",  0, &GnuPlot::BuiltinAzimuthRight);
	BindAppend("Escape", 0, &GnuPlot::BuiltinCancelZoom);
}

void GnuPlot::BindClear(bind_t * b)
{
	b->key = NO_KEY;
	b->modifier = 0;
	b->command = (char *)0;
	//b->builtin = 0;
	b->HandlerFunc = 0;
	b->prev = (struct bind_t *)0;
	b->next = (struct bind_t *)0;
}
//
// returns the enum which corresponds to the
// string (ptr) or NO_KEY if ptr matches not
// any of special_keys.
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
		for(char ** keyptr = special_keys; *keyptr; ++keyptr) {
			if(sstreq(ptr, *keyptr)) {
				*len = strlen(ptr);
				return keyptr - special_keys + GP_FIRST_KEY;
			}
		}
		return NO_KEY;
	}
}
//
// returns 1 on success, else 0. 
//
int GnuPlot::BindScanLhs(bind_t * out, const char * in)
{
	static const char DELIM = '-';
	int itmp = NO_KEY;
	char * ptr;
	int len;
	BindClear(out);
	if(!in) {
		return 0;
	}
	for(ptr = (char *)in; ptr && *ptr; /* EMPTY */) {
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
// 
// note, that this returns a pointer
// to the static char* `out' which is
// modified on subsequent calls.
// 
char * GnuPlot::BindFmtLhs(const bind_t * in)
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

int GnuPlot::BindMatches(const bind_t * a, const bind_t * b)
{
	int a_mod = a->modifier;
	int b_mod = b->modifier;
	// discard Shift modifier (except for mouse buttons) 
	if(a->key < GP_Button1) {
		a_mod &= (Mod_Ctrl | Mod_Alt);
		b_mod &= (Mod_Ctrl | Mod_Alt);
	}
	if(a->key == b->key && a_mod == b_mod)
		return 1;
	else if(a->key == b->key && (b->modifier & Mod_Opt))
		// Mod_Opt means both Alt and Ctrl are optional 
		return 2;
	else
		return 0;
}

void GnuPlot::BindDisplayOne(bind_t * ptr)
{
	fprintf(stderr, " %-13s ", BindFmtLhs(ptr));
	fprintf(stderr, "%c ", ptr->allwindows ? '*' : ' ');
	if(ptr->command) {
		fprintf(stderr, "`%s`\n", ptr->command);
	}
	/*else if(ptr->builtin) {
		fprintf(stderr, "%s\n", ptr->builtin(0));
	}*/
	else if(ptr->HandlerFunc) {
		fprintf(stderr, "%s\n", (this->*ptr->HandlerFunc)(0, term));
	}
	else {
		fprintf(stderr, "`%s:%d oops.'\n", __FILE__, __LINE__);
	}
}

void GnuPlot::BindDisplay(char * lhs)
{
	bind_t * ptr;
	bind_t lhs_scanned;
	if(!_Mse.P_Bindings) {
		BindInstallDefaultBindings();
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
		for(ptr = _Mse.P_Bindings; ptr; ptr = ptr->next) {
			BindDisplayOne(ptr);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "              * indicates this key is active from all plot windows\n");
		fprintf(stderr, "\n");
		return;
	}
	if(!BindScanLhs(&lhs_scanned, lhs)) {
		return;
	}
	for(ptr = _Mse.P_Bindings; ptr; ptr = ptr->next) {
		if(BindMatches(&lhs_scanned, ptr)) {
			BindDisplayOne(ptr);
			break;  /* only one match */
		}
	}
}

void GnuPlot::BindRemove(bind_t * b)
{
	if(b) {
		//if(b->builtin) {
		if(b->HandlerFunc) {
			// don't remove builtins, just remove the overriding command 
			ZFREE(b->command); 
		}
		else {
			if(b->prev)
				b->prev->next = b->next;
			if(b->next)
				b->next->prev = b->prev;
			else
				_Mse.P_Bindings->prev = b->prev;
			ZFREE(b->command);
			if(b == _Mse.P_Bindings) {
				_Mse.P_Bindings = b->next;
				if(_Mse.P_Bindings && _Mse.P_Bindings->prev)
					_Mse.P_Bindings->prev->next = (bind_t*)0;
			}
			SAlloc::F(b);
		}
	}
}

void GnuPlot::BindAppend(char * lhs, char * rhs, BuiltinEventHandler handlerFunc)
{
	bind_t * p_new = (bind_t *)SAlloc::M(sizeof(bind_t));
	if(!BindScanLhs(p_new, lhs)) {
		SAlloc::F(p_new);
	}
	else {
		if(!_Mse.P_Bindings)
			_Mse.P_Bindings = p_new; // first binding 
		else {
			for(bind_t * ptr = _Mse.P_Bindings; ptr; ptr = ptr->next) {
				if(BindMatches(p_new, ptr)) {
					// overwriting existing binding 
					if(!rhs) {
						//ptr->builtin = builtin;
						ptr->HandlerFunc = handlerFunc;
					}
					else if(*rhs) {
						ZFREE(ptr->command);
						ptr->command = rhs;
					}
					else // rhs is an empty string, so remove the binding 
						BindRemove(ptr);
					SAlloc::F(p_new); // don't need it any more 
					return;
				}
			}
			// if we're here, the binding does not exist yet 
			// append binding ... 
			_Mse.P_Bindings->prev->next = p_new;
			p_new->prev = _Mse.P_Bindings->prev;
		}
		_Mse.P_Bindings->prev = p_new;
		p_new->next = (struct bind_t *)0;
		p_new->allwindows = FALSE; // Can be explicitly set later 
		if(!rhs) {
			//p_new->builtin = builtin;
			p_new->HandlerFunc = handlerFunc;
		}
		else if(*rhs)
			p_new->command = rhs; // was allocated in command.c 
		else
			BindRemove(p_new);
	}
}

//void bind_process(char * lhs, char * rhs, bool allwindows)
void GnuPlot::BindProcess(char * lhs, char * rhs, bool allwindows)
{
	if(!_Mse.P_Bindings) {
		BindInstallDefaultBindings();
	}
	if(!rhs) {
		BindDisplay(lhs);
	}
	else {
		BindAppend(lhs, rhs, 0);
		if(allwindows)
			BindAll(lhs);
	}
	SAlloc::F(lhs);
}

//void bind_all(char * lhs)
void GnuPlot::BindAll(char * lhs)
{
	bind_t keypress;
	if(BindScanLhs(&keypress, lhs)) {
		for(bind_t * ptr = _Mse.P_Bindings; ptr; ptr = ptr->next) {
			if(BindMatches(&keypress, ptr))
				ptr->allwindows = TRUE;
		}
	}
}

void GnuPlot::BindRemoveAll()
{
	bind_t * safe;
	for(bind_t * ptr = _Mse.P_Bindings; ptr; safe = ptr, ptr = ptr->next, SAlloc::F(safe)) {
		ZFREE(ptr->command);
	}
	_Mse.P_Bindings = (bind_t*)0;
}
//
// Ruler is on, thus recalc its (px,py) from (x,y) for the current zoom and log axes.
//
void GnuPlot::RecalcRulerPos()
{
	double P, dummy;
	if(Gg.Is3DPlot) {
		// To be exact, it is 'set view map' splot. 
		int ppx, ppy;
		dummy = 1.0; // dummy value, but not 0.0 for the fear of log z-axis 
		Map3D_XY(_Mse.Ruler.RealPos.x, _Mse.Ruler.RealPos.y, dummy, &ppx, &ppy);
		_Mse.Ruler.Pos.x = ppx;
		_Mse.Ruler.Pos.y = ppy;
	}
	else {
		// It is 2D plot. 
		if(AxS[FIRST_X_AXIS].log && _Mse.Ruler.RealPos.x < 0)
			_Mse.Ruler.Pos.x = -1;
		else {
			P = _Mse.Ruler.RealPos.x;
			_Mse.Ruler.Pos.x = AxS[FIRST_X_AXIS].MapI(P);
		}
		if(AxS[FIRST_Y_AXIS].log && _Mse.Ruler.RealPos.y < 0)
			_Mse.Ruler.Pos.y = -1;
		else {
			P = _Mse.Ruler.RealPos.y;
			_Mse.Ruler.Pos.y = AxS[FIRST_Y_AXIS].MapI(P);
		}
		MousePosToGraphPosReal(_Mse.Ruler.Pos, &dummy, &dummy, &_Mse.Ruler.RealPos2.x, &_Mse.Ruler.RealPos2.y);
	}
}
//
// Recalculate and replot the ruler after a '(re)plot'. Called from term.c.
//
//void update_ruler()
void GnuPlot::UpdateRuler(GpTermEntry * pTerm)
{
	if(pTerm->set_ruler && _Mse.Ruler.on) {
		pTerm->set_ruler(pTerm, -1, -1);
		RecalcRulerPos();
		pTerm->set_ruler(pTerm, _Mse.Ruler.Pos.x, _Mse.Ruler.Pos.y);
	}
}
// 
// Set ruler on/off, and set its position.
// Called from set.c for 'set mouse ruler ...' command.
//
//void set_ruler(bool on, int mx, int my)
void GnuPlot::SetRuler(GpTermEntry * pTerm, bool on, int mx, int my)
{
	GpEvent ge;
	if(!_Mse.Ruler.on && !on)
		return;
	else if(_Mse.Ruler.on && on && (mx < 0 || my < 0))
		return;
	else {
		if(_Mse.Ruler.on) // ruler is on => switch it off 
			BuiltinToggleRuler(&ge, pTerm);
		// now the ruler is off 
		if(on) { // want ruler off 
			if(mx>=0 && my>=0) { // change ruler position 
				ge.mx = mx;
				ge.my = my;
			}
			else { // don't change ruler position 
				ge.mx = _Mse.Ruler.Pos.x;
				ge.my = _Mse.Ruler.Pos.y;
			}
			BuiltinToggleRuler(&ge, pTerm);
		}
	}
}
//
// for checking if we change from plot to splot (or vice versa) 
//
int GnuPlot::PlotMode(GpTermEntry * pTerm, int set)
{
	static int mode = MODE_PLOT;
	if(oneof2(set, MODE_PLOT, MODE_SPLOT)) {
		if(mode != set) {
			TurnRulerOff(pTerm);
		}
		mode = set;
	}
	return mode;
}

void GnuPlot::TurnRulerOff(GpTermEntry * pTerm)
{
	if(_Mse.Ruler.on) {
		udvt_entry * u;
		_Mse.Ruler.on = FALSE;
		if(pTerm && pTerm->set_ruler) {
			pTerm->set_ruler(pTerm, -1, -1);
		}
		if((u = Ev.AddUdvByName("MOUSE_RULER_X")))
			u->udv_value.SetNotDefined();
		if((u = Ev.AddUdvByName("MOUSE_RULER_Y")))
			u->udv_value.SetNotDefined();
		if(display_ipc_commands()) {
			fprintf(stderr, "turning ruler off.\n");
		}
	}
}

int GnuPlot::NearestLabelTag(GpTermEntry * pTerm, int xref, int yref)
{
	double min = -1;
	int min_tag = -1;
	double diff_squared;
	int x, y;
	int xd;
	int yd;
	for(text_label * this_label = Gg.P_FirstLabel; this_label; this_label = this_label->next) {
		if(Gg.Is3DPlot) {
			Map3DPosition(pTerm, &this_label->place, &xd, &yd, "label");
			xd -= xref;
			yd -= yref;
		}
		else {
			MapPosition(pTerm, &this_label->place, &x, &y, "label");
			xd = x - xref;
			yd = y - yref;
		}
		diff_squared = xd * xd + yd * yd;
		if(-1 == min || min > diff_squared) {
			// now we check if we're within a certain threshold around the label 
			int htic, vtic;
			GetOffsets(pTerm, this_label, &htic, &vtic);
			const double tic_diff_squared = htic * htic + vtic * vtic;
			if(diff_squared < tic_diff_squared) {
				min = diff_squared;
				min_tag = this_label->tag;
			}
		}
	}
	return min_tag;
}

void GnuPlot::RemoveLabel(GpTermEntry * pTerm, int x, int y)
{
	int tag = NearestLabelTag(pTerm, x, y);
	if(-1 != tag) {
		char cmd[0x40];
		sprintf(cmd, "unset label %d", tag);
		DoStringReplot(pTerm, cmd);
	}
}

void GnuPlot::PutLabel(GpTermEntry * pTerm, const char * pLabel, double x, double y)
{
	char cmd[512];
	sprintf(cmd, "set label \"%s\" at %g,%g %s", pLabel, x, y, mouse_setting.labelopts ? mouse_setting.labelopts : "point pt 1");
	DoStringReplot(pTerm, cmd);
}
//
// Save current mouse position to user-accessible variables.
// Save the keypress or mouse button that triggered this in MOUSE_KEY,
// and define MOUSE_BUTTON if it was a button click.
//
void GnuPlot::LoadMouseVariables(double x, double y, bool button, int c)
{
	MousePosToGraphPosReal(SPoint2I(static_cast<int>(x), static_cast<int>(y)), &_Mse.RealPos.x, &_Mse.RealPos.y, &_Mse.RealPos2.x, &_Mse.RealPos2.y);
	udvt_entry * current = Ev.AddUdvByName("MOUSE_BUTTON");
	if(current) {
		Ginteger(&current->udv_value, button ? c : -1);
		if(!button)
			current->udv_value.SetNotDefined();
	}
	current = Ev.AddUdvByName("MOUSE_KEY");
	if(current) {
		Ginteger(&current->udv_value, c);
	}
	current = Ev.AddUdvByName("MOUSE_CHAR");
	if(current) {
		char * keychar = (char *)SAlloc::M(2);
		keychar[0] = c;
		keychar[1] = '\0';
		gpfree_string(&current->udv_value);
		Gstring(&current->udv_value, keychar);
	}
	if((current = Ev.AddUdvByName("MOUSE_X"))) {
		Gcomplex(&current->udv_value, _Mse.RealPos.x, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_Y"))) {
		Gcomplex(&current->udv_value, _Mse.RealPos.y, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_X2"))) {
		Gcomplex(&current->udv_value, _Mse.RealPos2.x, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_Y2"))) {
		Gcomplex(&current->udv_value, _Mse.RealPos2.y, 0);
	}
	if((current = Ev.AddUdvByName("MOUSE_SHIFT"))) {
		Ginteger(&current->udv_value, _Mse.ModifierMask & Mod_Shift);
	}
	if((current = Ev.AddUdvByName("MOUSE_ALT"))) {
		Ginteger(&current->udv_value, _Mse.ModifierMask & Mod_Alt);
	}
	if((current = Ev.AddUdvByName("MOUSE_CTRL"))) {
		Ginteger(&current->udv_value, _Mse.ModifierMask & Mod_Ctrl);
	}
}

#endif /* USE_MOUSE */
