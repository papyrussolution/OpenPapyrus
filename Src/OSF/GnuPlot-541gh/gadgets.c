// GNUPLOT - gadgets.c 
// Copyright 2000, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

/* This file contains mainly a collection of global variables that
 * describe the status of several parts of the gnuplot plotting engine
 * that are used by both 2D and 3D plots, and thus belong neither to
 * graphics.c nor graph3d.c, alone. This is not a very clean solution,
 * but better than mixing internal status and the user interface as we
 * used to have it, in set.c and setshow.h */

legend_key keyT;// = DEFAULT_KEY_PROPS;
//
// Description of the color box associated with GPO.AxS.__CB() 
//
//color_box_struct color_box; // initialized in init_color() 
const color_box_struct default_color_box = {SMCOLOR_BOX_DEFAULT, 'v', 1, -1, 0, LAYER_FRONT, 0,
	{screen, screen, screen, 0.90, 0.2, 0.0}, {screen, screen, screen, 0.05, 0.6, 0.0}, FALSE, {0, 0, 0, 0} };

//BoundingBox plot_bounds_Removed; // The graph box (terminal coordinates) calculated by boundary() or boundary3d() 
//BoundingBox page_bounds_Removed; // The bounding box for 3D plots prior to applying view transformations 
//BoundingBox canvas_Removed; // The bounding box for the entire drawable area  of current terminal 
//BoundingBox * clip_area_Removed = &GPO.V.BbPlot; // The bounding box against which clipping is to be done 

// 'set size', 'set origin' settings 
//float  xsize_Removed = 1.0f;        // scale factor for size
//float  ysize_Removed = 1.0f;        // scale factor for size 
//float  zsize_Removed = 1.0f;        // scale factor for size 
//float  xoffset_Removed = 0.0f;      // x origin
//float  yoffset_Removed = 0.0f;      // y origin
//float  aspect_ratio = 0.0f; // don't attempt to force it 
//int    aspect_ratio_3D = 0; // 2 will put x and y on same scale, 3 for z also 

// EAM Augest 2006 - redefine margin as GpPosition so that absolute placement is possible 
//GpPosition lmargin_Removed = DEFAULT_MARGIN_POSITION; /* space between left edge and GPO.V.BbPlot.xleft in chars (-1: computed) */
//GpPosition bmargin_Removed = DEFAULT_MARGIN_POSITION; /* space between bottom and GPO.V.BbPlot.ybot in chars (-1: computed) */
//GpPosition rmargin_Removed = DEFAULT_MARGIN_POSITION; /* space between right edge and GPO.V.BbPlot.xright in chars (-1: computed) */
//GpPosition tmargin_Removed = DEFAULT_MARGIN_POSITION; /* space between top edge and GPO.V.BbPlot.ytop in chars (-1: computed) */
//custom_dashtype_def * first_custom_dashtype = NULL; /* Pointer to first 'set dashtype' definition in linked list */
//text_label * first_label = NULL; /* Pointer to the start of the linked list of 'set label' definitions */
// Pointer to first 'set linestyle' definition in linked list 
//linestyle_def * first_linestyle = NULL;
//linestyle_def * first_perm_linestyle = NULL;
//linestyle_def * first_mono_linestyle = NULL;
//arrowstyle_def * first_arrowstyle = NULL; /* Pointer to first 'set style arrow' definition in linked list */
//t_pixmap * pixmap_listhead = NULL; /* Listhead for pixmaps */
//arrow_def * first_arrow = NULL; /* set arrow */
//GpObject  * first_object = NULL; /* Pointer to first object instance in linked list */
//pa_style   parallel_axis_style; // = DEFAULT_PARALLEL_AXIS_STYLE; // Holds the properties from 'set style parallelaxis' 
//spider_web spiderplot_style; // = DEFAULT_SPIDERPLOT_STYLE; // Holds properties for 'set style spiderplot' 
GpObject    grid_wall[5];// = {WALL_Y0, WALL_X0, WALL_Y1, WALL_X1, WALL_Z0}; /* Pointer to array of grid walls */
//text_label title; // = EMPTY_LABELSTRUCT; /* 'set title' status */
// 'set timelabel' status 
//text_label timelabel; // = EMPTY_LABELSTRUCT;
//int    timelabel_bottom = TRUE;
//double zero = ZERO; // zero threshold, may _not_ be 0! 
// Status of 'set pointsize' and 'set pointintervalbox' commands 
//double pointsize = 1.0;
//double pointintervalbox = 1.0;
//double boxwidth              = -1.0; /* box width (automatic) for plot style "with boxes" */
//bool   boxwidth_is_absolute  = true; /* whether box width is absolute (default) or relative */
// set border 
int    draw_border = 31; // The current settings 
int    user_border = 31; // What the user last set explicitly 
int    border_layer = LAYER_FRONT;
#define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }
lp_style_type border_lp(lp_style_type::defBorder); // = DEFAULT_BORDER_LP;
const  t_colorspec background_fill(TC_LT, LT_BACKGROUND, 0.0); // = BACKGROUND_COLORSPEC; /* used for filled points */
const  lp_style_type default_border_lp(lp_style_type::defBorder); // = DEFAULT_BORDER_LP;
const  lp_style_type background_lp(lp_style_type::defBkg); //= {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN};
//bool   polar = false;
//bool   inverted_raxis = false;
//bool   spiderplot = false; // toggle spiderplot mode on/off 
//bool   parametric = false;
//bool   in_parametric = false;
//bool   is_3d_plot = false; // If last plot was a 3d one. 
//bool   volatile_data = false; // Flag to show that volatile input data is present 
//bool   cornerpoles = true;
//bool   prefer_line_styles = false; // Prefer line styles over plain line types 
// set clip 
//bool   clip_lines1 = true;
//bool   clip_lines2 = false;
//bool   clip_points = false;
//bool   clip_radial = false;

//static int clip_line(int *, int *, int *, int *);

// set samples 
//int    samples_1 = SAMPLES;
//int    samples_2 = SAMPLES;
// set angles 
double ang2rad = 1.0;           /* 1 or pi/180, tracking angles_format */
enum PLOT_STYLE data_style = POINTSTYLE;
enum PLOT_STYLE func_style = LINES;
TRefresh_Allowed refresh_ok = E_REFRESH_NOT_OK; /* Flag to signal that the existing data is valid for a quick refresh */
int    refresh_nplots = 0; /* FIXME: do_plot should be able to figure this out on its own! */
fill_style_type default_fillstyle(FS_EMPTY, 100, 0); // = { FS_EMPTY, 100, 0, DEFAULT_COLORSPEC };
// Default rectangle style - background fill, black border 
GpObject default_rectangle(t_object::defRectangle);//= DEFAULT_RECTANGLE_STYLE;
GpObject default_circle(t_object::defCircle);// = DEFAULT_CIRCLE_STYLE;
GpObject default_ellipse(t_object::defEllipse);// = DEFAULT_ELLIPSE_STYLE;
// filledcurves style options 
filledcurves_opts filledcurves_opts_data = EMPTY_FILLEDCURVES_OPTS;
filledcurves_opts filledcurves_opts_func = EMPTY_FILLEDCURVES_OPTS;
histogram_style histogram_opts; // = DEFAULT_HISTOGRAM_STYLE;
boxplot_style boxplot_opts = DEFAULT_BOXPLOT_STYLE;
int current_x11_windowid = 0; /* WINDOWID to be filled by terminals running on X11 (x11, wxt, qt, ...) */
textbox_style textbox_opts[NUM_TEXTBOX_STYLES];

/*****************************************************************/
/* Routines that deal with global objects defined in this module */
/*****************************************************************/

/* Clipping to the bounding box: */

/* Test a single point to be within the BoundingBox.
 * Sets the returned integers 4 l.s.b. as follows:
 * bit 0 if to the left of xleft.
 * bit 1 if to the right of xright.
 * bit 2 if below of ybot.
 * bit 3 if above of ytop.
 * 0 is returned if inside.
 */
//int FASTCALL clip_point(int x, int y)
int FASTCALL GpView::ClipPoint(int x, int y) const
{
	int ret_val = 0;
	if(P_ClipArea) {
		if(x < P_ClipArea->xleft)
			ret_val |= 0x01;
		if(x > P_ClipArea->xright)
			ret_val |= 0x02;
		if(y < P_ClipArea->ybot)
			ret_val |= 0x04;
		if(y > P_ClipArea->ytop)
			ret_val |= 0x08;
	}
	return ret_val;
}
// 
// Clip the given line to drawing coords defined by BoundingBox.
//   This routine uses the cohen & sutherland bit mapping for fast clipping -
// see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
// 
//int draw_clip_line(GpTermEntry * pTerm, int x1, int y1, int x2, int y2)
int GnuPlot::DrawClipLine(GpTermEntry * pTerm, int x1, int y1, int x2, int y2)
{
	int state = V.ClipLine(&x1, &y1, &x2, &y2);
	if(state != 0) {
		(pTerm->move)(pTerm, x1, y1);
		(pTerm->vector)(pTerm, x2, y2);
	}
	return state;
}
// 
// Draw a contiguous line path which may be clipped. Compared to
// draw_clip_line(), this routine moves to a coordinate only when necessary.
// 
void draw_clip_polygon(GpTermEntry * pTerm, int points, gpiPoint * p)
{
	int i;
	int x1, y1, x2, y2;
	int pos1, pos2, clip_ret;
	bool continuous = true;
	if(points <= 1)
		return;
	if(p[0].x != p[points-1].x || p[0].y != p[points-1].y)
		continuous = false;
	x1 = p[0].x;
	y1 = p[0].y;
	pos1 = GPO.V.ClipPoint(x1, y1);
	if(!pos1) // move to first point if it is inside 
		(pTerm->move)(pTerm, x1, y1);
	newpath(pTerm);
	for(i = 1; i < points; i++) {
		x2 = p[i].x;
		y2 = p[i].y;
		pos2 = GPO.V.ClipPoint(x2, y2);
		clip_ret = GPO.V.ClipLine(&x1, &y1, &x2, &y2);
		if(clip_ret) {
			// there is a line to draw 
			if(pos1) // first vertex was recalculated, move to new start point 
				(pTerm->move)(pTerm, x1, y1);
			(pTerm->vector)(pTerm, x2, y2);
		}
		else {
			continuous = false; // Path is not continuous; make sure closepath is not called 
		}
		x1 = p[i].x;
		y1 = p[i].y;
		// The end point and the line do not necessarily have the same
		// status. The end point can be 'inside', but the whole line is
		// 'outside'. Do not update pos1 in this case.  Bug #1268.
		if(!(clip_ret == 0 && pos2 == 0))
			pos1 = pos2;
	}
	// Only call closepath if the polygon is truly closed; otherwise 
	// a spurious line connecting the start and end is generated.    
	if(continuous)
		closepath(pTerm);
}
// 
// arrow is specified in terminal coordinates
// but we use double rather than int so that the precision is sufficient
// to orient and draw the arrow head correctly even for very short vectors.
// 
//void draw_clip_arrow(GpTermEntry * pTerm, double dsx, double dsy, double dex, double dey, t_arrow_head head)
void GnuPlot::DrawClipArrow(GpTermEntry * pTerm, double dsx, double dsy, double dex, double dey, t_arrow_head head)
{
	int sx = GpAxis::MapRealToInt(dsx);
	int sy = GpAxis::MapRealToInt(dsy);
	int ex = GpAxis::MapRealToInt(dex);
	int ey = GpAxis::MapRealToInt(dey);
	int dx, dy;
	// Don't draw head if the arrow itself is clipped 
	if(V.ClipPoint(sx, sy))
		head = (t_arrow_head)(((int)head) & ~BACKHEAD);
	if(V.ClipPoint(ex, ey))
		head = (t_arrow_head)(((int)head) & ~END_HEAD);
	// clip_line returns 0 if the whole thing is out of range 
	if(!V.ClipLine(&sx, &sy, &ex, &ey))
		return;
	// Special case code for short vectors */
	// Most terminals are OK with using this code for long vectors also.	
	// However some terminals (e.g. tikz) look terrible because the shaft of a	
	// long vector overruns the head when the head is drawn with HEADS_ONLY.	
	// FIXME:  this is a very ad hoc definition of "short".			
	dx = abs(ex-sx);
	dy = abs(ey-sy);
	if(dx < 25 && dy < 25) {
		// draw the body of the vector (rounding errors are a problem) 
		if(dx > 1 || dy > 1)
			if(!((pTerm->flags & TERM_IS_LATEX)))
				(pTerm->arrow)(pTerm, sx, sy, ex, ey, SHAFT_ONLY | head);
		// if we're not supposed to be drawing any heads, we're done 
		if((head & BOTH_HEADS) == NOHEAD)
			return;
		// If this is truly a 0-vector, then we CAN'T draw the head 
		// because the orientation of the head is indeterminate     
		if(dsx == dex && dsy == dey)
			return;
		// If the head size is fixed we are free to change the length of 
		// very short vectors so that the orientation is accurate.	 
		if(curr_arrow_headfixedsize) {
			// Direction vector in (dex,dey). I need to convert this to integers
			// with a scale that's large-enough to give me good angular resolution,
			// but small-enough to not overflow the data type.
			double rescale = 1000.0 / MAX(fabs(dex-dsx), fabs(dey-dsy) );
			int newlenx = static_cast<int>((dex - dsx) * rescale);
			int newleny = static_cast<int>((dey - dsy) * rescale);
			if(head & END_HEAD)
				(pTerm->arrow)(pTerm, ex - newlenx, ey - newleny, ex, ey, END_HEAD|HEADS_ONLY);
			if(head & BACKHEAD)
				(pTerm->arrow)(pTerm, sx, sy, sx + newlenx, sy + newleny, BACKHEAD|HEADS_ONLY);
		}
		else
			(pTerm->arrow)(pTerm, sx, sy, ex, ey, head|HEADS_ONLY);
	}
	else
		(pTerm->arrow)(pTerm, sx, sy, ex, ey, head); // The normal case, draw the whole thing at once */
}

/* Clip the given line to drawing coords defined by BoundingBox.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 * Return 0: entire line segment is outside bounding box
 *        1: entire line segment is inside bounding box
 *       -1: line segment has been clipped to bounding box
 */
//int clip_line(int * x1, int * y1, int * x2, int * y2)
int GpView::ClipLine(int * x1, int * y1, int * x2, int * y2) const
{
	/* Apr 2014: This algorithm apparently assumed infinite precision
	 * integer arithmetic. It was failing when passed coordinates that
	 * were hugely out of bounds because tests for signedness of the
	 * form (dx * dy > 0) would overflow rather than correctly evaluating
	 * to (sign(dx) == sign(dy)).  Worse yet, the numerical values are
	 * used to determine which end of the segment to update.
	 * This is now addressed by making dx and dy (double) rather than (int)
	 * but it might be better to hard-code the sign tests.
	 */
	double dx, dy, x, y;
	int x_intr[4], y_intr[4], count;
	int x_max, x_min, y_max, y_min;
	int pos1 = ClipPoint(*x1, *y1);
	int pos2 = ClipPoint(*x2, *y2);
	if(!pos1 && !pos2)
		return 1;       /* segment is totally in */
	if(pos1 & pos2)
		return 0;       /* segment is totally out. */
	/* Here part of the segment MAY be inside. test the intersection
	 * of this segment with the 4 boundaries for hopefully 2 intersections
	 * in. If none are found segment is totaly out.
	 * Under rare circumstances there may be up to 4 intersections (e.g.
	 * when the line passes directly through at least one corner).
	 */
	count = 0;
	dx = *x2 - *x1;
	dy = *y2 - *y1;
	// Find intersections with the x parallel bbox lines: 
	if(dy != 0) {
		x = (P_ClipArea->ybot - *y2) * dx / dy + *x2; // Test for P_ClipArea->ybot boundary. 
		if(x >= P_ClipArea->xleft && x <= P_ClipArea->xright) {
			x_intr[count] = static_cast<int>(x);
			y_intr[count++] = P_ClipArea->ybot;
		}
		x = (P_ClipArea->ytop - *y2) * dx / dy + *x2; // Test for P_ClipArea->ytop boundary. 
		if(x >= P_ClipArea->xleft && x <= P_ClipArea->xright) {
			x_intr[count] = static_cast<int>(x);
			y_intr[count++] = P_ClipArea->ytop;
		}
	}
	// Find intersections with the y parallel bbox lines: 
	if(dx != 0) {
		y = (P_ClipArea->xleft - *x2) * dy / dx + *y2; // Test for P_ClipArea->xleft boundary. 
		if(y >= P_ClipArea->ybot && y <= P_ClipArea->ytop) {
			x_intr[count] = P_ClipArea->xleft;
			y_intr[count++] = static_cast<int>(y);
		}
		y = (P_ClipArea->xright - *x2) * dy / dx + *y2; // Test for P_ClipArea->xright boundary. 
		if(y >= P_ClipArea->ybot && y <= P_ClipArea->ytop) {
			x_intr[count] = P_ClipArea->xright;
			y_intr[count++] = static_cast<int>(y);
		}
	}
	if(count < 2)
		return 0;
	// check which intersections to use, for more than two intersections the first two may be identical 
	if((count > 2) && (x_intr[0] == x_intr[1]) && (y_intr[0] == y_intr[1])) {
		x_intr[1] = x_intr[2];
		y_intr[1] = y_intr[2];
	}
	if(*x1 < *x2) {
		x_min = *x1;
		x_max = *x2;
	}
	else {
		x_min = *x2;
		x_max = *x1;
	}
	if(*y1 < *y2) {
		y_min = *y1;
		y_max = *y2;
	}
	else {
		y_min = *y2;
		y_max = *y1;
	}
	if(pos1 && pos2) {      /* Both were out - update both */
		/* EAM Sep 2008 - preserve direction of line segment */
		if((dx*(x_intr[1]-x_intr[0]) < 0) ||  (dy*(y_intr[1]-y_intr[0]) < 0)) {
			*x1 = x_intr[1];
			*y1 = y_intr[1];
			*x2 = x_intr[0];
			*y2 = y_intr[0];
		}
		else {
			*x1 = x_intr[0];
			*y1 = y_intr[0];
			*x2 = x_intr[1];
			*y2 = y_intr[1];
		}
	}
	else if(pos1) { // Only x1/y1 was out - update only it 
		if(dx * (*x2 - x_intr[0]) + dy * (*y2 - y_intr[0]) > 0) {
			*x1 = x_intr[0];
			*y1 = y_intr[0];
		}
		else {
			*x1 = x_intr[1];
			*y1 = y_intr[1];
		}
	}
	else { // Only x2/y2 was out - update only it 
		// Same difference here, again 
		if(dx * (x_intr[0] - *x1) + dy * (y_intr[0] - *y1) > 0) {
			*x2 = x_intr[0];
			*y2 = y_intr[0];
		}
		else {
			*x2 = x_intr[1];
			*y2 = y_intr[1];
		}
	}
	if(*x1 < x_min || *x1 > x_max || *x2 < x_min || *x2 > x_max || *y1 < y_min || *y1 > y_max || *y2 < y_min || *y2 > y_max)
		return 0;
	return -1;
}
// 
// test if coordinates of a vertex are inside boundary box. The start
// and end points for the clip_boundary must be in correct order for
// this to work properly (see respective definitions in clip_polygon()). 
// 
static bool vertex_is_inside(gpiPoint test_vertex, const gpiPoint * clip_boundary)
{
	if(clip_boundary[1].x > clip_boundary[0].x)           /*bottom edge*/
		if(test_vertex.y >= clip_boundary[0].y) 
			return TRUE;
	if(clip_boundary[1].x < clip_boundary[0].x)           /*top edge*/
		if(test_vertex.y <= clip_boundary[0].y) 
			return TRUE;
	if(clip_boundary[1].y > clip_boundary[0].y)           /*right edge*/
		if(test_vertex.x <= clip_boundary[1].x) 
			return TRUE;
	if(clip_boundary[1].y < clip_boundary[0].y)           /*left edge*/
		if(test_vertex.x >= clip_boundary[1].x) 
			return TRUE;
	return FALSE;
}

static void intersect_polyedge_with_boundary(gpiPoint first, gpiPoint second, gpiPoint * intersect, const gpiPoint * clip_boundary)
{
	/* this routine is called only if one point is outside and the other
	   is inside, which implies that clipping is needed at a horizontal
	   boundary, that second.y is different from first.y and no division
	   by zero occurs. Same for vertical boundary and x coordinates. */
	if(clip_boundary[0].y == clip_boundary[1].y) { /* horizontal */
		(*intersect).y = clip_boundary[0].y;
		(*intersect).x = first.x + (clip_boundary[0].y - first.y) * (second.x - first.x)/(second.y - first.y);
	}
	else { /* vertical */
		(*intersect).x = clip_boundary[0].x;
		(*intersect).y = first.y + (clip_boundary[0].x - first.x) * (second.y - first.y)/(second.x - first.x);
	}
}
//
// Clip the given polygon to a single edge of the bounding box. 
//
static void clip_polygon_to_boundary(gpiPoint * in, gpiPoint * out, int in_length, int * out_length, const gpiPoint * clip_boundary)
{
	gpiPoint prev, curr; /* start and end point of current polygon edge. */
	int j;
	*out_length = 0;
	if(in_length <= 0)
		return;
	else
		prev = in[in_length - 1]; /* start with the last vertex */
	for(j = 0; j < in_length; j++) {
		curr = in[j];
		if(vertex_is_inside(curr, clip_boundary)) {
			if(vertex_is_inside(prev, clip_boundary)) {
				/* both are inside, add current vertex */
				out[*out_length] = in[j];
				(*out_length)++;
			}
			else {
				/* changed from outside to inside, add intersection point and current point */
				intersect_polyedge_with_boundary(prev, curr, out+(*out_length), clip_boundary);
				out[*out_length+1] = curr;
				*out_length += 2;
			}
		}
		else {
			if(vertex_is_inside(prev, clip_boundary)) {
				/* changed from inside to outside, add intersection point */
				intersect_polyedge_with_boundary(prev, curr, out+(*out_length), clip_boundary);
				(*out_length)++;
			}
		}
		prev = curr;
	}
}
// 
// Clip the given polygon to drawing coords defined by BoundingBox.
// This routine uses the Sutherland-Hodgman algorithm.  When calling
// this function, you must make sure that you reserved enough
// memory for the output polygon. out_length can be as big as 2*(in_length - 1)
// 
//void clip_polygon(const gpiPoint * pIn, gpiPoint * pOut, int in_length, int * out_length)
void GpView::ClipPolygon(const gpiPoint * pIn, gpiPoint * pOut, int in_length, int * out_length) const
{
	static gpiPoint * tmp_corners = NULL;
	if(!P_ClipArea) {
		memcpy(pOut, pIn, in_length * sizeof(gpiPoint));
		*out_length = in_length;
	}
	else {
		gpiPoint clip_boundary[5];
		tmp_corners = (gpiPoint *)gp_realloc(tmp_corners, 4 * in_length * sizeof(gpiPoint), "clip_polygon");
		// vertices of the rectangular clipping window starting from top-left in counterclockwise direction 
		clip_boundary[0].x = P_ClipArea->xleft; // top left 
		clip_boundary[0].y = P_ClipArea->ytop;
		clip_boundary[1].x = P_ClipArea->xleft; // bottom left 
		clip_boundary[1].y = P_ClipArea->ybot;
		clip_boundary[2].x = P_ClipArea->xright; // bottom right 
		clip_boundary[2].y = P_ClipArea->ybot;
		clip_boundary[3].x = P_ClipArea->xright; // top right 
		clip_boundary[3].y = P_ClipArea->ytop;
		clip_boundary[4] = clip_boundary[0];
		memcpy(tmp_corners, pIn, in_length * sizeof(gpiPoint));
		for(int i = 0; i < 4; i++) {
			clip_polygon_to_boundary(tmp_corners, pOut, in_length, out_length, clip_boundary+i);
			memcpy(tmp_corners, pOut, *out_length * sizeof(gpiPoint));
			in_length = *out_length;
		}
	}
}

// Two routines to emulate move/vector sequence using line drawing routine. 
static int move_pos_x; // @global
static int move_pos_y; // @global

void clip_move(int x, int y)
{
	move_pos_x = x;
	move_pos_y = y;
}

//void clip_vector(GpTermEntry * pTerm, int x, int y)
void GnuPlot::ClipVector(GpTermEntry * pTerm, int x, int y)
{
	DrawClipLine(pTerm, move_pos_x, move_pos_y, x, y);
	move_pos_x = x;
	move_pos_y = y;
}
// 
// draw_polar_clip_line() assumes that the endpoints have already
// been categorized as INRANGE/OUTRANGE, and that "set clip radial" is in effect.
// 
//void draw_polar_clip_line(GpTermEntry * pTerm, double xbeg, double ybeg, double xend, double yend)
void GnuPlot::DrawPolarClipLine(GpTermEntry * pTerm, double xbeg, double ybeg, double xend, double yend)
{
	double R; // radius of limiting circle 
	double a, b; // line expressed as y = a*x + b 
	double x1, y1, x2, y2; // Intersections of line and circle 
	double Q, Q2; // sqrt term of quadratic equation 
	bool vertical = FALSE; // flag for degenerate case 
	bool beg_inrange, end_inrange;
	if(AxS.__R().set_max == -VERYLARGE)
		goto outside;
	R = AxS.__R().set_max - AxS.__R().set_min;
	// If both endpoints are inside the limiting circle, draw_clip_line suffices 
	beg_inrange = (xbeg*xbeg + ybeg*ybeg) <= R*R;
	end_inrange = (xend*xend + yend*yend) <= R*R;
	if(beg_inrange && end_inrange) {
		DrawClipLine(pTerm, AxS.MapiX(xbeg), AxS.MapiY(ybeg), AxS.MapiX(xend), AxS.MapiY(yend));
	}
	else {
		// FIXME:  logscale and other odd cases are not covered by this equation 
		if(fabs(xbeg - xend) > ZERO) {
			// Recast line in the form y = a*x + b 
			a = (yend - ybeg) / (xend - xbeg);
			b = ybeg - xbeg * a;
			// the line may intersect a circle of radius R in two places 
			Q2 = 4*a*a*b*b - 4 * (1 + a*a) * (b*b - R*R);
			if(Q2 < 0)
				goto outside;
			Q = sqrt(Q2);
			x1 = (-2*a*b + Q) / ( 2*(1+a*a));
			x2 = (-2*a*b - Q) / ( 2*(1+a*a));
			y1 = a * x1 + b;
			y2 = a * x2 + b;
		}
		else {
			// degenerate case (vertical line) 
			x1 = x2 = xbeg;
			if(fabs(x1) > R)
				goto outside;
			vertical = TRUE;
			y1 = sqrt(R*R - x1*x1);
			y2 = -y1;
			if(!inrange(y1, ybeg, yend) && !inrange(y2, ybeg, yend))
				goto outside;
		}
		// 
		// If one of the original endpoints was INRANGE then use it
		// rather than the second intersection point.
		// 
		if(vertical) {
			y1 = MIN(y1, MAX(ybeg, yend));
			y2 = MAX(y2, MIN(ybeg, yend));
		}
		else if(beg_inrange) {
			if(!inrange(x1, xbeg, xend)) {
				x1 = xbeg;
				y1 = ybeg;
			}
			else {
				x2 = xbeg;
				y2 = ybeg;
			}
		}
		else if(end_inrange) {
			if(!inrange(x1, xbeg, xend)) {
				x1 = xend;
				y1 = yend;
			}
			else {
				x2 = xend;
				y2 = yend;
			}
		}
		else {
			// Both OUTRANGE. Are they on the same side of the circle? 
			if(!inrange(x1, xbeg, xend))
				goto outside;
		}
		// Draw the part of the line inside the bounding circle 
		(pTerm->move)(pTerm, AxS.MapiX(x1), AxS.MapiY(y1));
		(pTerm->vector)(pTerm, AxS.MapiX(x2), AxS.MapiY(y2));
		// fall through 
outside:
		// Leave current position at unclipped endpoint 
		(pTerm->move)(pTerm, AxS.MapiX(xend), AxS.MapiY(yend));
	}
}
//
// Common routines for setting text or line color from t_colorspec 
//
//void apply_pm3dcolor(GpTermEntry * pTerm, t_colorspec * tc)
void GnuPlot::ApplyPm3DColor(GpTermEntry * pTerm, const t_colorspec * tc)
{
	double cbval;
	// V5 - term->linetype(LT_BLACK) would clobber the current	
	// dashtype so instead we use term->set_color(black).	
	static t_colorspec black = BLACK_COLORSPEC;
	// Replace colorspec with that of the requested line style 
	lp_style_type style;
	if(tc->type == TC_LINESTYLE) {
		lp_use_properties(pTerm, &style, tc->lt);
		tc = &style.pm3d_color;
	}
	if(tc->type == TC_DEFAULT) {
		pTerm->set_color(pTerm, &black);
		return;
	}
	else if(tc->type == TC_LT) {
		pTerm->set_color(pTerm, tc);
		return;
	}
	else if(tc->type == TC_RGB) {
		// FIXME: several plausible ways for monochrome terminals to handle color request
		// (1) Allow all color requests despite the label "monochrome"
		// (2) Choose any color you want so long as it is black
		// (3) Convert colors to gray scale (NTSC?)
		// Monochrome terminals are still allowed to display rgb variable colors 
		if(pTerm->flags & TERM_MONOCHROME && tc->value >= 0)
			pTerm->set_color(pTerm, &black);
		else
			pTerm->set_color(pTerm, tc);
		return;
	}
	else if(tc->type == TC_VARIABLE) // Leave unchanged. (used only by "set errorbars"??) 
		return;
	else if(!is_plot_with_palette()) {
		pTerm->set_color(pTerm, &black);
		return;
	}
	else {
		switch(tc->type) {
			case TC_Z:
				set_color(pTerm, Cb2Gray(tc->value));
				break;
			case TC_CB:
				if(AxS.__CB().log)
					cbval = (tc->value <= 0) ? AxS.__CB().min : tc->value;
				else
					cbval = tc->value;
				set_color(pTerm, Cb2Gray(cbval));
				break;
			case TC_FRAC:
				set_color(pTerm, SmPltt.Positive == SMPAL_POSITIVE ?  tc->value : 1-tc->value);
				break;
			default:
				break; // cannot happen 
		}
	}
}

//void reset_textcolor(const t_colorspec * tc)
void GnuPlot::ResetTextColor(GpTermEntry * pTerm, const t_colorspec * tc)
{
	if(tc->type != TC_DEFAULT)
		pTerm->linetype(pTerm, LT_BLACK);
}

void default_arrow_style(struct arrow_style_type * arrow)
{
	static const lp_style_type tmp_lp_style(lp_style_type::defArrow); // = {0, LT_DEFAULT, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN};
	arrow->tag = -1;
	arrow->layer = LAYER_BACK;
	arrow->lp_properties = tmp_lp_style;
	arrow->head = (t_arrow_head)1;
	arrow->head_length = 0.0;
	arrow->head_lengthunit = first_axes;
	arrow->head_angle = 15.0;
	arrow->head_backangle = 90.0;
	arrow->headfill = AS_NOFILL;
	arrow->head_fixedsize = FALSE;
}

void apply_head_properties(const arrow_style_type * pArrowProperties)
{
	curr_arrow_headfilled = pArrowProperties->headfill;
	curr_arrow_headfixedsize = pArrowProperties->head_fixedsize;
	curr_arrow_headlength = 0;
	if(pArrowProperties->head_length > 0) {
		// set head length+angle for term->arrow 
		double xtmp, ytmp;
		GpPosition headsize = {first_axes, graph, graph, 0., 0., 0.};
		headsize.x = pArrowProperties->head_length;
		headsize.scalex = (position_type)pArrowProperties->head_lengthunit;
		GPO.MapPositionR(term, &headsize, &xtmp, &ytmp, "arrow");
		curr_arrow_headangle = pArrowProperties->head_angle;
		curr_arrow_headbackangle = pArrowProperties->head_backangle;
		curr_arrow_headlength = static_cast<int>(xtmp);
	}
}

void free_labels(text_label * pLabel)
{
	if(pLabel) {
		char * master_font = pLabel->font;
		// Labels generated by 'plot with labels' all use the same font 
		SAlloc::F(master_font);
		while(pLabel) {
			SAlloc::F(pLabel->text);
			if(pLabel->font && pLabel->font != master_font)
				SAlloc::F(pLabel->font);
			text_label * temp = pLabel->next;
			SAlloc::F(pLabel);
			pLabel = temp;
		}
	}
}

//void get_offsets(text_label * pLabel, int * pHTic, int * pVTic)
void GnuPlot::GetOffsets(GpTermEntry * pTerm, text_label * pLabel, int * pHTic, int * pVTic)
{
	if((pLabel->lp_properties.flags & LP_SHOW_POINTS)) {
		*pHTic = static_cast<int>(Gg.PointSize * pTerm->TicH * 0.5);
		*pVTic = static_cast<int>(Gg.PointSize * pTerm->TicV * 0.5);
	}
	else {
		*pHTic = 0;
		*pVTic = 0;
	}
	if(Gg.Is3DPlot) {
		int htic2, vtic2;
		Map3DPositionR(pTerm, &pLabel->offset, &htic2, &vtic2, "get_offsets");
		*pHTic += htic2;
		*pVTic += vtic2;
	}
	else {
		double htic2, vtic2;
		MapPositionR(pTerm, &(pLabel->offset), &htic2, &vtic2, "get_offsets");
		*pHTic += (int)htic2;
		*pVTic += (int)vtic2;
	}
}
// 
// Write one label, with all the trimmings.
// This routine is used for both 2D and 3D plots.
// 
//void write_label(GpTermEntry * pTerm, int x, int y, struct text_label * this_label)
void GnuPlot::WriteLabel(GpTermEntry * pTerm, int x, int y, text_label * pLabel)
{
	int htic, vtic;
	int justify = JUST_TOP; /* This was the 2D default; 3D had CENTRE */
	textbox_style * textbox = NULL;
	ApplyPm3DColor(pTerm, &(pLabel->textcolor));
	ignore_enhanced(pLabel->noenhanced);
	// The text itself 
	if(pLabel->hypertext) {
		if(pLabel->text && *pLabel->text) {
			// Treat text as hypertext 
			char * font = pLabel->font;
			if(font)
				pTerm->set_font(pTerm, font);
			if(pTerm->hypertext)
				pTerm->hypertext(pTerm, TERM_HYPERTEXT_TOOLTIP, pLabel->text);
			if(font)
				pTerm->set_font(pTerm, "");
		}
	}
	else{
		// A normal label (always print text) 
		GetOffsets(pTerm, pLabel, &htic, &vtic);
		if(pLabel->boxed < 0)
			textbox = &textbox_opts[0];
		else if(pLabel->boxed > 0)
			textbox = &textbox_opts[pLabel->boxed];
		// Initialize the bounding box accounting 
		if(textbox && pTerm->boxed_text && (textbox->opaque || !textbox->noborder))
			(pTerm->boxed_text)(x + htic, y + vtic, TEXTBOX_INIT);
		if(pLabel->rotate && (*pTerm->text_angle)(pLabel->rotate)) {
			write_multiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, pLabel->rotate, pLabel->font);
			(pTerm->text_angle)(0);
		}
		else {
			write_multiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, 0, pLabel->font);
		}
	}
	if(textbox && pTerm->boxed_text && (textbox->opaque || !textbox->noborder)) {
		// Adjust the bounding box margins 
		(pTerm->boxed_text)((int)(textbox->xmargin * 100.0), (int)(textbox->ymargin * 100.0), TEXTBOX_MARGINS);
		// Blank out the box and reprint the label 
		if(textbox->opaque) {
			ApplyPm3DColor(pTerm, &textbox->fillcolor);
			(pTerm->boxed_text)(0, 0, TEXTBOX_BACKGROUNDFILL);
			ApplyPm3DColor(pTerm, &(pLabel->textcolor));
			// Init for each of fill and border 
			if(!textbox->noborder)
				(pTerm->boxed_text)(x + htic, y + vtic, TEXTBOX_INIT);
			if(pLabel->rotate && (*pTerm->text_angle)(pLabel->rotate)) {
				write_multiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, pLabel->rotate, pLabel->font);
				(pTerm->text_angle)(0);
			}
			else
				write_multiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, 0, pLabel->font);
		}
		// Draw the bounding box 
		if(!textbox->noborder) {
			(pTerm->linewidth)(pTerm, textbox->linewidth);
			ApplyPm3DColor(pTerm, &textbox->border_color);
			(pTerm->boxed_text)(0, 0, TEXTBOX_OUTLINE);
		}
		(pTerm->boxed_text)(0, 0, TEXTBOX_FINISH);
	}
	// The associated point, if any 
	// write_multiline() clips text to on_page; do the same for any point 
	if((pLabel->lp_properties.flags & LP_SHOW_POINTS) && on_page(pTerm, x, y)) {
		TermApplyLpProperties(pTerm, &pLabel->lp_properties);
		(pTerm->point)(pTerm, x, y, pLabel->lp_properties.PtType);
		// the default label color is that of border 
		TermApplyLpProperties(pTerm, &border_lp);
	}
	ignore_enhanced(FALSE);
}
// 
// STR points to a label string, possibly with several lines separated
// by \n.  Return the number of characters in the longest line.  If
// LINES is not NULL, set *LINES to the number of lines in the label. 
//
int label_width(const char * str, int * lines)
{
	int mlen = 0;
	char * s, * e;
	if(isempty(str)) {
		ASSIGN_PTR(lines, 0);
	}
	else {
		int len = 0;
		int l = 0;
		char * lab = (char *)gp_alloc(strlen(str) + 2, "in label_width");
		strcpy(lab, str);
		strcat(lab, "\n");
		s = lab;
		while((e = (char*)strchr(s, '\n')) != NULL) {
			*e = '\0';
			len = estimate_strlen(s, NULL); /* = e-s ? */
			if(len > mlen)
				mlen = len;
			if(len || l || *str == '\n')
				l++;
			s = ++e;
		}
		// lines = NULL => not interested - div 
		ASSIGN_PTR(lines, l);
		SAlloc::F(lab);
	}
	return mlen;
}
//
// Here so that it can be shared by the 2D and 3D code
//
void do_timelabel(int x, int y)
{
	text_label temp = GPO.Gg.LblTime;
	char str[MAX_LINE_LEN+1];
	time_t now;
	if(GPO.Gg.LblTime.rotate == 0 && !GPO.Gg.TimeLabelBottom)
		y -= term->ChrV;
	time(&now);
	strftime(str, MAX_LINE_LEN, GPO.Gg.LblTime.text, localtime(&now));
	temp.text = str;
	GPO.WriteLabel(term, x, y, &temp);
}

void init_gadgets()
{
	//GpObject    grid_wall[5];// = {WALL_Y0, WALL_X0, WALL_Y1, WALL_X1, WALL_Z0}; /* Pointer to array of grid walls */
//#define WALL_Y0 { NULL, WALL_Y0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_Y1 { NULL, WALL_Y1_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_X0 { NULL, WALL_X0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_X1 { NULL, WALL_X1_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_Z0 { NULL, WALL_Z0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
	int i;
	static GpPosition y0_wall_corners[5] = WALL_Y0_CORNERS;
	static GpPosition x0_wall_corners[5] = WALL_X0_CORNERS;
	static GpPosition y1_wall_corners[5] = WALL_Y1_CORNERS;
	static GpPosition x1_wall_corners[5] = WALL_X1_CORNERS;
	static GpPosition z0_wall_corners[5] = WALL_Z0_CORNERS;
	for(i = 0; i < SIZEOFARRAY(grid_wall); i++) {
		grid_wall[i].SetDefaultGridWall();
		grid_wall[i].tag = i;
		if((i+1) < SIZEOFARRAY(grid_wall))
			grid_wall[i].next = &grid_wall[i+1];
		//grid_wall[i].next = &grid_wall[i+1];
	}
	grid_wall[WALL_Y0_TAG].o.polygon.vertex = y0_wall_corners;
	grid_wall[WALL_X0_TAG].o.polygon.vertex = x0_wall_corners;
	grid_wall[WALL_Y1_TAG].o.polygon.vertex = y1_wall_corners;
	grid_wall[WALL_X1_TAG].o.polygon.vertex = x1_wall_corners;
	grid_wall[WALL_Z0_TAG].o.polygon.vertex = z0_wall_corners;
	grid_wall[WALL_Y0_TAG].lp_properties.pm3d_color.type = TC_RGB;
	grid_wall[WALL_X0_TAG].lp_properties.pm3d_color.type = TC_RGB;
	grid_wall[WALL_Y1_TAG].lp_properties.pm3d_color.type = TC_RGB;
	grid_wall[WALL_X1_TAG].lp_properties.pm3d_color.type = TC_RGB;
	grid_wall[WALL_Z0_TAG].lp_properties.pm3d_color.type = TC_RGB;
	grid_wall[WALL_Y0_TAG].lp_properties.pm3d_color.lt = WALL_Y_COLOR;
	grid_wall[WALL_X0_TAG].lp_properties.pm3d_color.lt = WALL_X_COLOR;
	grid_wall[WALL_Y1_TAG].lp_properties.pm3d_color.lt = WALL_Y_COLOR;
	grid_wall[WALL_X1_TAG].lp_properties.pm3d_color.lt = WALL_X_COLOR;
	grid_wall[WALL_Z0_TAG].lp_properties.pm3d_color.lt = WALL_Z_COLOR;
}
//
// walk through the list of objects to see if any require pm3d processing
//
bool pm3d_objects(void)
{
	GpObject * obj = GPO.Gg.P_FirstObject;
	while(obj) {
		if(obj->layer == LAYER_DEPTHORDER)
			return TRUE;
		obj = obj->next;
	}
	return FALSE;
}
// 
// Place overall title on the canvas (shared by plot and splot).
// 
//void place_title(int title_x, int title_y)
void GnuPlot::PlaceTitle(GpTermEntry * pTerm, int titleX, int titleY)
{
	if(Gg.LblTitle.text) {
		// NB: write_label applies text color but does not reset it 
		WriteLabel(pTerm, titleX, titleY, &Gg.LblTitle);
		ResetTextColor(pTerm, &Gg.LblTitle.textcolor);
	}
}
