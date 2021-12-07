// GNUPLOT - gadgets.c 
// Copyright 2000, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
// 
// This file contains mainly a collection of global variables that
// describe the status of several parts of the gnuplot plotting engine
// that are used by both 2D and 3D plots, and thus belong neither to
// graphics.c nor graph3d.c, alone. This is not a very clean solution,
// but better than mixing internal status and the user interface as we
// used to have it, in set.c and setshow.h 
// 
// Description of the color box associated with AxS.__CB() 
//
const  color_box_struct default_color_box = {SMCOLOR_BOX_DEFAULT, 'v', 1, -1, 0, LAYER_FRONT, 0,
	{screen, screen, screen, 0.90, 0.2, 0.0}, {screen, screen, screen, 0.05, 0.6, 0.0}, FALSE, {0, 0, 0, 0} };
const  t_colorspec background_fill(TC_LT, LT_BACKGROUND, 0.0); // = BACKGROUND_COLORSPEC; /* used for filled points */
const  lp_style_type default_border_lp(lp_style_type::defBorder); // = DEFAULT_BORDER_LP;
const  lp_style_type background_lp(lp_style_type::defBkg); //= {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN};
//#define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }

lp_style_type::lp_style_type(int def) : flags(0), PtType(0), p_interval(0), p_number(0), P_Colormap(0)
{
	memzero(p_char, sizeof(p_char));
	CustomDashPattern.SetDefault();
	if(def == defBkg)
		SetDefault();
	else if(def == defZeroAxis) {
		// #define DEFAULT_AXIS_ZEROAXIS {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
		l_type = LT_AXIS;
		d_type = DASHTYPE_AXIS;
		l_width = 1.0;
		PtSize = PTSZ_DEFAULT;
		pm3d_color.SetBlack();
	}
	else if(def == defParallelAxis) {
		//#define DEFAULT_PARALLEL_AXIS_STYLE {{0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 2.0, 0.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}, LAYER_FRONT }
		l_type = LT_BLACK;
		d_type = DASHTYPE_SOLID;
		l_width = 2.0;
		PtSize = 0.0;
		pm3d_color.SetBlack();
	}
	else if(def == defGrid) {
		//#define DEFAULT_GRID_LP {0, LT_AXIS, 0, DASHTYPE_AXIS, 0, 0.5, 0.0, DEFAULT_P_CHAR, {TC_LT, LT_AXIS, 0.0}, DEFAULT_DASHPATTERN}
		l_type = LT_AXIS;
		d_type = DASHTYPE_AXIS;
		l_width = 0.5;
		PtSize = 0.0;
		pm3d_color.Set(TC_LT, LT_AXIS, 0.0);
	}
	else if(def == defBorder) {
		//#define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }
		l_type = LT_BLACK;
		d_type = DASHTYPE_SOLID;
		l_width = 1.0;
		PtSize = 1.0;
		pm3d_color.SetBlack();
	}
	else if(def == defArrow) {
		//{0, LT_DEFAULT, 0, DASHTYPE_SOLID, 0, 0, 1.0, 0.0, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN};
		l_type = LT_DEFAULT;
		d_type = DASHTYPE_SOLID;
		l_width = 1.0;
		PtSize = 0.0;
		pm3d_color.SetDefault();
	}
	else if(def == defHypertextPoint) {
		// {1, LT_BLACK, 4, DASHTYPE_SOLID, 0, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, {TC_RGB, 0x000000, 0.0}, DEFAULT_DASHPATTERN};
		flags = 1;
		l_type = LT_BLACK;
		PtType = 4;
		d_type = DASHTYPE_SOLID;
		l_width = 1.0;
		PtSize = PTSZ_DEFAULT;
		pm3d_color.Set(TC_RGB, 0, 0.0);
	}
	else
		SetDefault2();
}

void lp_style_type::SetDefault()
{
	// {0, LT_BACKGROUND, 0, DASHTYPE_SOLID, 0, 1.0, 0.0, DEFAULT_P_CHAR, BACKGROUND_COLORSPEC, DEFAULT_DASHPATTERN},
	flags = 0;
	l_type = LT_BACKGROUND;
	PtType = 0;
	d_type = DASHTYPE_SOLID;
	p_interval = 0;
	p_number = 0;
	l_width = 1.0;
	PtSize = 0.0;
	memzero(p_char, sizeof(p_char));
	pm3d_color.SetBackground();
	CustomDashPattern.SetDefault();
}

void lp_style_type::SetDefault2() // DEFAULT_LP_STYLE_TYPE
{
	// #define DEFAULT_LP_STYLE_TYPE {0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, DEFAULT_COLORSPEC, DEFAULT_DASHPATTERN}
	flags = 0;
	l_type = LT_BLACK;
	PtType = 0;
	d_type = DASHTYPE_SOLID;
	p_interval = 0;
	p_number = 0;
	l_width = 1.0;
	PtSize = PTSZ_DEFAULT;
	memzero(p_char, sizeof(p_char));
	pm3d_color.SetDefault();
	CustomDashPattern.SetDefault();
}

void lp_style_type::SetDefaultKeybox()
{
	// #define DEFAULT_KEYBOX_LP {0, LT_NODRAW, 0, DASHTYPE_SOLID, 0, 1.0, PTSZ_DEFAULT, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN}
	flags = 0;
	l_type = LT_NODRAW;
	PtType = 0;
	d_type = DASHTYPE_SOLID;
	p_interval = 0;
	p_number = 0;
	l_width = 1.0;
	PtSize = PTSZ_DEFAULT;
	memzero(p_char, sizeof(p_char));
	pm3d_color.SetBlack();
	CustomDashPattern.SetDefault();
}
// 
// Test a single point to be within the BoundingBox.
// Sets the returned integers 4 l.s.b. as follows:
// bit 0 if to the left of xleft.
// bit 1 if to the right of xright.
// bit 2 if below of ybot.
// bit 3 if above of ytop.
// 0 is returned if inside.
// 
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
	const int state = V.ClipLine(&x1, &y1, &x2, &y2);
	if(state != 0) {
		pTerm->move(pTerm, x1, y1);
		pTerm->vector(pTerm, x2, y2);
	}
	return state;
}
// 
// Draw a contiguous line path which may be clipped. Compared to
// draw_clip_line(), this routine moves to a coordinate only when necessary.
// 
//void draw_clip_polygon(GpTermEntry * pTerm, int points, gpiPoint * p)
void GnuPlot::DrawClipPolygon(GpTermEntry * pTerm, int points, gpiPoint * p)
{
	if(points > 1) {
		bool continuous = true;
		if(p[0].x != p[points-1].x || p[0].y != p[points-1].y)
			continuous = false;
		int x1 = p[0].x;
		int y1 = p[0].y;
		int pos1 = V.ClipPoint(x1, y1);
		if(!pos1) // move to first point if it is inside 
			pTerm->move(pTerm, x1, y1);
		newpath(pTerm);
		for(int i = 1; i < points; i++) {
			int x2 = p[i].x;
			int y2 = p[i].y;
			const int pos2 = V.ClipPoint(x2, y2);
			const int clip_ret = V.ClipLine(&x1, &y1, &x2, &y2);
			if(clip_ret) {
				// there is a line to draw 
				if(pos1) // first vertex was recalculated, move to new start point 
					pTerm->move(pTerm, x1, y1);
				pTerm->vector(pTerm, x2, y2);
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
				pTerm->arrow(pTerm, sx, sy, ex, ey, SHAFT_ONLY | head);
		// if we're not supposed to be drawing any heads, we're done 
		if((head & BOTH_HEADS) == NOHEAD)
			return;
		// If this is truly a 0-vector, then we CAN'T draw the head 
		// because the orientation of the head is indeterminate     
		if(dsx == dex && dsy == dey)
			return;
		// If the head size is fixed we are free to change the length of 
		// very short vectors so that the orientation is accurate.	 
		if(GPT.CArw.HeadFixedSize) {
			// Direction vector in (dex,dey). I need to convert this to integers
			// with a scale that's large-enough to give me good angular resolution,
			// but small-enough to not overflow the data type.
			double rescale = 1000.0 / MAX(fabs(dex-dsx), fabs(dey-dsy) );
			int newlenx = static_cast<int>((dex - dsx) * rescale);
			int newleny = static_cast<int>((dey - dsy) * rescale);
			if(head & END_HEAD)
				pTerm->arrow(pTerm, ex - newlenx, ey - newleny, ex, ey, END_HEAD|HEADS_ONLY);
			if(head & BACKHEAD)
				pTerm->arrow(pTerm, sx, sy, sx + newlenx, sy + newleny, BACKHEAD|HEADS_ONLY);
		}
		else
			pTerm->arrow(pTerm, sx, sy, ex, ey, head|HEADS_ONLY);
	}
	else
		pTerm->arrow(pTerm, sx, sy, ex, ey, head); // The normal case, draw the whole thing at once */
}

/* Clip the given line to drawing coords defined by BoundingBox.
 *   This routine uses the cohen & sutherland bit mapping for fast clipping -
 * see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
 * Return 0: entire line segment is outside bounding box
 *  1: entire line segment is inside bounding box
 * -1: line segment has been clipped to bounding box
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
		return 1; /* segment is totally in */
	if(pos1 & pos2)
		return 0; /* segment is totally out. */
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
		prev = in[in_length-1]; /* start with the last vertex */
	for(j = 0; j < in_length; j++) {
		curr = in[j];
		if(vertex_is_inside(curr, clip_boundary)) {
			if(vertex_is_inside(prev, clip_boundary)) {
				// both are inside, add current vertex 
				out[*out_length] = in[j];
				(*out_length)++;
			}
			else {
				// changed from outside to inside, add intersection point and current point 
				intersect_polyedge_with_boundary(prev, curr, out+(*out_length), clip_boundary);
				out[*out_length+1] = curr;
				*out_length += 2;
			}
		}
		else {
			if(vertex_is_inside(prev, clip_boundary)) {
				// changed from inside to outside, add intersection point 
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
		tmp_corners = (gpiPoint *)SAlloc::R(tmp_corners, 4 * in_length * sizeof(gpiPoint));
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
//static int move_pos_x; // @global
//static int move_pos_y; // @global

//void clip_move(int x, int y)
void GnuPlot::ClipMove(int x, int y)
{
	//move_pos_x = x;
	//move_pos_y = y;
	Gg.MovPos.Set(x, y);
}

//void clip_vector(GpTermEntry * pTerm, int x, int y)
void GnuPlot::ClipVector(GpTermEntry * pTerm, int x, int y)
{
	DrawClipLine(pTerm, /*move_pos_x*/Gg.MovPos.x, /*move_pos_y*/Gg.MovPos.y, x, y);
	Gg.MovPos.Set(x, y);
	//move_pos_x = x;
	//move_pos_y = y;
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
		DrawClipLine(pTerm, MapiX(xbeg), MapiY(ybeg), MapiX(xend), MapiY(yend));
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
		pTerm->move(pTerm, MapiX(x1), MapiY(y1));
		pTerm->vector(pTerm, MapiX(x2), MapiY(y2));
		// @fallthrough 
outside:
		// Leave current position at unclipped endpoint 
		pTerm->move(pTerm, MapiX(xend), MapiY(yend));
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
		LpUseProperties(pTerm, &style, tc->lt);
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
	else if(!IsPlotWithPalette()) {
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

//void apply_head_properties(const arrow_style_type * pArrowProperties)
void GnuPlot::ApplyHeadProperties(GpTermEntry * pTerm, const arrow_style_type * pArrowProperties)
{
	GPT.CArw.HeadFilled = pArrowProperties->headfill;
	GPT.CArw.HeadFixedSize = pArrowProperties->head_fixedsize;
	GPT.CArw.HeadLength = 0;
	if(pArrowProperties->head_length > 0) {
		// set head length+angle for term->arrow 
		double xtmp, ytmp;
		GpPosition headsize = {first_axes, graph, graph, 0., 0., 0.};
		headsize.x = pArrowProperties->head_length;
		headsize.scalex = (position_type)pArrowProperties->head_lengthunit;
		MapPositionR(pTerm, &headsize, &xtmp, &ytmp, "arrow");
		GPT.CArw.HeadAngle = pArrowProperties->head_angle;
		GPT.CArw.HeadBackAngle = pArrowProperties->head_backangle;
		GPT.CArw.HeadLength = static_cast<int>(xtmp);
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
			FREEANDASSIGN(pLabel, temp);
		}
	}
}

//void get_offsets(text_label * pLabel, int * pHTic, int * pVTic)
void GnuPlot::GetOffsets(GpTermEntry * pTerm, text_label * pLabel, int * pHTic, int * pVTic)
{
	if(pLabel->lp_properties.flags & LP_SHOW_POINTS) {
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
	IgnoreEnhanced(pLabel->noenhanced);
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
	else {
		// A normal label (always print text) 
		GetOffsets(pTerm, pLabel, &htic, &vtic);
		if(pLabel->boxed < 0)
			textbox = &Gg.textbox_opts[0];
		else if(pLabel->boxed > 0)
			textbox = &Gg.textbox_opts[pLabel->boxed];
		// Initialize the bounding box accounting 
		if(textbox && pTerm->boxed_text && (textbox->opaque || !textbox->noborder))
			pTerm->boxed_text(pTerm, x + htic, y + vtic, TEXTBOX_INIT);
		if(pLabel->rotate && (*pTerm->text_angle)(pTerm, pLabel->rotate)) {
			WriteMultiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, pLabel->rotate, pLabel->font);
			pTerm->text_angle(pTerm, 0);
		}
		else {
			WriteMultiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, 0, pLabel->font);
		}
	}
	if(textbox && pTerm->boxed_text && (textbox->opaque || !textbox->noborder)) {
		// Adjust the bounding box margins 
		pTerm->boxed_text(pTerm, (int)(textbox->Margin.x * 100.0), (int)(textbox->Margin.y * 100.0), TEXTBOX_MARGINS);
		// Blank out the box and reprint the label 
		if(textbox->opaque) {
			ApplyPm3DColor(pTerm, &textbox->fillcolor);
			pTerm->boxed_text(pTerm, 0, 0, TEXTBOX_BACKGROUNDFILL);
			ApplyPm3DColor(pTerm, &(pLabel->textcolor));
			// Init for each of fill and border 
			if(!textbox->noborder)
				pTerm->boxed_text(pTerm, x + htic, y + vtic, TEXTBOX_INIT);
			if(pLabel->rotate && pTerm->text_angle(pTerm, pLabel->rotate)) {
				WriteMultiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, pLabel->rotate, pLabel->font);
				pTerm->text_angle(pTerm, 0);
			}
			else
				WriteMultiline(pTerm, x + htic, y + vtic, pLabel->text, pLabel->pos, (VERT_JUSTIFY)justify, 0, pLabel->font);
		}
		// Draw the bounding box 
		if(!textbox->noborder) {
			pTerm->linewidth(pTerm, textbox->linewidth);
			ApplyPm3DColor(pTerm, &textbox->border_color);
			pTerm->boxed_text(pTerm, 0, 0, TEXTBOX_OUTLINE);
		}
		pTerm->boxed_text(pTerm, 0, 0, TEXTBOX_FINISH);
	}
	// The associated point, if any 
	// WriteMultiline() clips text to on_page; do the same for any point 
	if((pLabel->lp_properties.flags & LP_SHOW_POINTS) && on_page(pTerm, x, y)) {
		TermApplyLpProperties(pTerm, &pLabel->lp_properties);
		pTerm->point(pTerm, x, y, pLabel->lp_properties.PtType);
		// the default label color is that of border 
		TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
	IgnoreEnhanced(false);
}
// 
// STR points to a label string, possibly with several lines separated
// by \n.  Return the number of characters in the longest line.  If
// LINES is not NULL, set *LINES to the number of lines in the label. 
//
//int label_width(const char * pStr, int * pLines)
int GnuPlot::LabelWidth(const char * pStr, int * pLines)
{
	int mlen = 0;
	char * s, * e;
	if(isempty(pStr)) {
		ASSIGN_PTR(pLines, 0);
	}
	else {
		int len = 0;
		int l = 0;
		char * lab = (char *)SAlloc::M(strlen(pStr) + 2);
		strcpy(lab, pStr);
		strcat(lab, "\n");
		s = lab;
		while((e = (char *)strchr(s, '\n')) != NULL) {
			*e = '\0';
			len = EstimateStrlen(s, NULL); /* = e-s ? */
			SETMAX(mlen, len);
			if(len || l || *pStr == '\n')
				l++;
			s = ++e;
		}
		// lines = NULL => not interested - div 
		ASSIGN_PTR(pLines, l);
		SAlloc::F(lab);
	}
	return mlen;
}
//
// Here so that it can be shared by the 2D and 3D code
//
//void do_timelabel(int x, int y)
void GnuPlot::DoTimeLabel(GpTermEntry * pTerm, int x, int y)
{
	text_label temp = Gg.LblTime;
	char str[MAX_LINE_LEN+1];
	time_t now;
	if(Gg.LblTime.rotate == 0 && !Gg.TimeLabelBottom)
		y -= pTerm->ChrV;
	time(&now);
	strftime(str, MAX_LINE_LEN, Gg.LblTime.text, localtime(&now));
	temp.text = str;
	WriteLabel(pTerm, x, y, &temp);
}

//void init_gadgets()
void GnuPlot::InitGadgets()
{
	//GpObject    grid_wall[5];// = {WALL_Y0, WALL_X0, WALL_Y1, WALL_X1, WALL_Z0}; /* Pointer to array of grid walls */
//#define WALL_Y0 { NULL, WALL_Y0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_Y1 { NULL, WALL_Y1_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_X0 { NULL, WALL_X0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_X1 { NULL, WALL_X1_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
//#define WALL_Z0 { NULL, WALL_Z0_TAG, LAYER_FRONTBACK, OBJ_POLYGON, OBJ_CLIP, {FS_TRANSPARENT_SOLID, 50, 0, BLACK_COLORSPEC}, DEFAULT_LP_STYLE_TYPE, {.polygon = {5, NULL} } }
	static GpPosition y0_wall_corners[5] = WALL_Y0_CORNERS;
	static GpPosition x0_wall_corners[5] = WALL_X0_CORNERS;
	static GpPosition y1_wall_corners[5] = WALL_Y1_CORNERS;
	static GpPosition x1_wall_corners[5] = WALL_X1_CORNERS;
	static GpPosition z0_wall_corners[5] = WALL_Z0_CORNERS;
	for(size_t i = 0; i < SIZEOFARRAY(Gg.GridWall); i++) {
		Gg.GridWall[i].SetDefaultGridWall();
		Gg.GridWall[i].tag = i;
		if((i+1) < SIZEOFARRAY(Gg.GridWall))
			Gg.GridWall[i].next = &Gg.GridWall[i+1];
		//grid_wall[i].next = &grid_wall[i+1];
	}
	Gg.GridWall[WALL_Y0_TAG].o.polygon.vertex = y0_wall_corners;
	Gg.GridWall[WALL_X0_TAG].o.polygon.vertex = x0_wall_corners;
	Gg.GridWall[WALL_Y1_TAG].o.polygon.vertex = y1_wall_corners;
	Gg.GridWall[WALL_X1_TAG].o.polygon.vertex = x1_wall_corners;
	Gg.GridWall[WALL_Z0_TAG].o.polygon.vertex = z0_wall_corners;
	Gg.GridWall[WALL_Y0_TAG].lp_properties.pm3d_color.type = TC_RGB;
	Gg.GridWall[WALL_X0_TAG].lp_properties.pm3d_color.type = TC_RGB;
	Gg.GridWall[WALL_Y1_TAG].lp_properties.pm3d_color.type = TC_RGB;
	Gg.GridWall[WALL_X1_TAG].lp_properties.pm3d_color.type = TC_RGB;
	Gg.GridWall[WALL_Z0_TAG].lp_properties.pm3d_color.type = TC_RGB;
	Gg.GridWall[WALL_Y0_TAG].lp_properties.pm3d_color.lt = WALL_Y_COLOR;
	Gg.GridWall[WALL_X0_TAG].lp_properties.pm3d_color.lt = WALL_X_COLOR;
	Gg.GridWall[WALL_Y1_TAG].lp_properties.pm3d_color.lt = WALL_Y_COLOR;
	Gg.GridWall[WALL_X1_TAG].lp_properties.pm3d_color.lt = WALL_X_COLOR;
	Gg.GridWall[WALL_Z0_TAG].lp_properties.pm3d_color.lt = WALL_Z_COLOR;
}
//
// walk through the list of objects to see if any require pm3d processing
//
//bool pm3d_objects()
bool GnuPlot::Pm3DObjects()
{
	GpObject * obj = Gg.P_FirstObject;
	while(obj) {
		if(obj->layer == LAYER_DEPTHORDER)
			return true;
		obj = obj->next;
	}
	return false;
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
