/* GNUPLOT - gadgets.c */

/*[
 * Copyright 2000, 2004   Thomas Williams, Colin Kelley
 *
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
#include <gnuplot.h>
#pragma hdrstop

GpGadgets GpGg; // @global
//
// This file contains mainly a collection of global variables that
// used to be in 'set.c', where they didn't really belong. They
// describe the status of several parts of the gnuplot plotting engine
// that are used by both 2D and 3D plots, and thus belong neither to
// graphics.c nor graph3d.c, alone. This is not a very clean solution,
// but better than mixing internal status and the user interface as we
// used to have it, in set.c and setshow.h
// 

#define DEFAULT_BORDER_LP { 0, LT_BLACK, 0, DASHTYPE_SOLID, 0, 1.0, 1.0, DEFAULT_P_CHAR, BLACK_COLORSPEC, DEFAULT_DASHPATTERN }

pa_style parallel_axis_style; // = DEFAULT_PARALLEL_AXIS_STYLE; // Holds the properties from 'set style parallelaxis' 
//
// Clipping to the bounding box: 
//
// Test a single point to be within the BoundingBox.
// Sets the returned integers 4 l.s.b. as follows:
// bit 0 if to the left of xleft.
// bit 1 if to the right of xright.
// bit 2 if below of ybot.
// bit 3 if above of ytop.
// 0 is returned if inside.
//
//int clip_point(uint x, uint y)
int GpGadgets::ClipPoint(uint x, uint y) const
{
	int    ret_val = 0;
	if(P_Clip) {
		if((int)x < P_Clip->xleft)
			ret_val |= 0x01;
		if((int)x > P_Clip->xright)
			ret_val |= 0x02;
		if((int)y < P_Clip->ybot)
			ret_val |= 0x04;
		if((int)y > P_Clip->ytop)
			ret_val |= 0x08;
	}
	return ret_val;
}
//
// Clip the given line to drawing coords defined by BoundingBox.
// This routine uses the cohen & sutherland bit mapping for fast clipping -
// see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
//
//void draw_clip_line(int x1, int y1, int x2, int y2)
void GpGadgets::DrawClipLine(GpTermEntry * pT, int x1, int y1, int x2, int y2)
{
	if(ClipLine(&x1, &y1, &x2, &y2)) { // clip_line() returns zero --> segment completely outside bounding box 
		pT->_Move(x1, y1);
		pT->_Vector(x2, y2);
	}
}
//
// Draw a contiguous line path which may be clipped. Compared to
// draw_clip_line(), this routine moves to a GpCoordinate only when necessary.
//
//void draw_clip_polygon(int points, gpiPoint * p)
void GpGadgets::DrawClipPolygon(GpTermEntry * pT, int points, gpiPoint * p)
{
	if(points > 1) {
		int x1 = p[0].x;
		int y1 = p[0].y;
		int pos1 = ClipPoint(x1, y1);
		if(!pos1) // move to first point if it is inside 
			pT->_Move(x1, y1);
		for(int i = 1; i < points; i++) {
			int x2 = p[i].x;
			int y2 = p[i].y;
			int pos2 = ClipPoint(x2, y2);
			int clip_ret = ClipLine(&x1, &y1, &x2, &y2);
			if(clip_ret) {
				// there is a line to draw 
				if(pos1) // first vertex was recalculated, move to new start point 
					pT->_Move(x1, y1);
				pT->_Vector(x2, y2);
			}
			x1 = p[i].x;
			y1 = p[i].y;
			// The end point and the line do not necessarily have the same
			// status. The end point can be 'inside', but the whole line is
			// 'outside'. Do not update pos1 in this case.  Bug #1268.
			// FIXME: This is papering over an inconsistency in GpCoordinate
			// calculation somewhere else!
			//
			if(!(clip_ret == 0 && pos2 == 0))
				pos1 = pos2;
		}
	}
}

//void draw_clip_arrow(int sx, int sy, int ex, int ey, int head)
void GpGadgets::DrawClipArrow(GpTermEntry * pT, int sx, int sy, int ex, int ey, int head)
{
	// Don't draw head if the arrow itself is clipped 
	if(ClipPoint(sx, sy))
		head &= ~BACKHEAD;
	if(ClipPoint(ex, ey))
		head &= ~END_HEAD;
	ClipLine(&sx, &sy, &ex, &ey);
	// Call terminal routine to draw the clipped arrow 
	pT->arrow((uint)sx, (uint)sy, (uint)ex, (uint)ey, head);
}
//
// Clip the given line to drawing coords defined by BoundingBox.
//   This routine uses the cohen & sutherland bit mapping for fast clipping -
// see "Principles of Interactive Computer Graphics" Newman & Sproull page 65.
// Return 0: entire line segment is outside bounding box
//        1: entire line segment is inside bounding box
//       -1: line segment has been clipped to bounding box
//
//int clip_line(int * x1, int * y1, int * x2, int * y2)
int GpGadgets::ClipLine(int * x1, int * y1, int * x2, int * y2) const
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
	double x, y;
	int    x_intr[4], y_intr[4];
	int    x_max, x_min, y_max, y_min;
	const int pos1 = ClipPoint(*x1, *y1);
	const int pos2 = ClipPoint(*x2, *y2);
	if(!pos1 && !pos2) // segment is totally in 
		return 1;       
	else if(pos1 & pos2)
		return 0;       // segment is totally out
	else {
		//
		// Here part of the segment MAY be inside. test the intersection
		// of this segment with the 4 boundaries for hopefully 2 intersections
		// in. If none are found segment is totaly out.
		// Under rare circumstances there may be up to 4 intersections (e.g.
		// when the line passes directly through at least one corner).
		//
		int    count = 0;
		const double dx = *x2 - *x1;
		const double dy = *y2 - *y1;
		// Find intersections with the x parallel bbox lines: 
		if(dy != 0) {
			x = (P_Clip->ybot - *y2) * dx / dy + *x2; // Test for P_Clip->ybot boundary
			if(x >= P_Clip->xleft && x <= P_Clip->xright) {
				x_intr[count] = (int)x;
				y_intr[count++] = P_Clip->ybot;
			}
			x = (P_Clip->ytop - *y2) * dx / dy + *x2; // Test for P_Clip->ytop boundary
			if(x >= P_Clip->xleft && x <= P_Clip->xright) {
				x_intr[count] = (int)x;
				y_intr[count++] = P_Clip->ytop;
			}
		}
		// Find intersections with the y parallel bbox lines:
		if(dx != 0) {
			y = (P_Clip->xleft - *x2) * dy / dx + *y2; // Test for P_Clip->xleft boundary
			if(y >= P_Clip->ybot && y <= P_Clip->ytop) {
				x_intr[count] = P_Clip->xleft;
				y_intr[count++] = (int)y;
			}
			y = (P_Clip->xright - *x2) * dy / dx + *y2; // Test for P_Clip->xright boundary
			if(y >= P_Clip->ybot && y <= P_Clip->ytop) {
				x_intr[count] = P_Clip->xright;
				y_intr[count++] = (int)y;
			}
		}
		if(count < 2)
			return 0;
		else {
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
			if(pos1 && pos2) { // Both were out - update both 
				// EAM Sep 2008 - preserve direction of line segment 
				if((dx*(x_intr[1]-x_intr[0]) < 0) || (dy*(y_intr[1]-y_intr[0]) < 0)) {
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
				/* Nov 2010: When clip_line() and draw_clip_line() were consolidated in */
				/* 2000, the test below was the only point of difference between them.  */
				/* Unfortunately, the wrong version was kept. Now I change it back.     */
				/* The effect of the wrong version (>= rather than >) was that a line   */
				/* from ymin to ymax+eps was clipped to ymin,ymin rather than ymin,ymax */
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
			return (*x1 < x_min || *x1 > x_max || *x2 < x_min || *x2 > x_max || *y1 < y_min || *y1 > y_max || *y2 < y_min || *y2 > y_max) ? 0 : -1;
		}
	}
}

/* test if coordinates of a vertex are inside boundary box. The start
   and end points for the clip_boundary must be in correct order for
   this to work properly (see respective definitions in clip_polygon()). */
bool vertex_is_inside(gpiPoint test_vertex, gpiPoint * clip_boundary)
{
	if(clip_boundary[1].x > clip_boundary[0].x)           /*bottom edge*/
		if(test_vertex.y >= clip_boundary[0].y) return true;
	if(clip_boundary[1].x < clip_boundary[0].x)           /*top edge*/
		if(test_vertex.y <= clip_boundary[0].y) return true;
	if(clip_boundary[1].y > clip_boundary[0].y)           /*right edge*/
		if(test_vertex.x <= clip_boundary[1].x) return true;
	if(clip_boundary[1].y < clip_boundary[0].y)           /*left edge*/
		if(test_vertex.x >= clip_boundary[1].x) return true;
	return false;
}

void intersect_polyedge_with_boundary(gpiPoint first, gpiPoint second, gpiPoint * intersect, gpiPoint * clip_boundary)
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
// Clip the given polygon to a single edge of the bounding box
//
void clip_polygon_to_boundary(gpiPoint * in, gpiPoint * out, int in_length, int * out_length, gpiPoint * clip_boundary)
{
	*out_length = 0;
	if(in_length > 0) {
		gpiPoint prev = in[in_length - 1]; // start with the last vertex 
		for(int j = 0; j < in_length; j++) {
			gpiPoint curr = in[j];
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
}

/* Clip the given polygon to drawing coords defined by BoundingBox.
 * This routine uses the Sutherland-Hodgman algorithm.  When calling
 * this function, you must make sure that you reserved enough
 * memory for the output polygon. out_length can be as big as
 * 2*(in_length - 1)
 */
//void clip_polygon(gpiPoint * in, gpiPoint * out, int in_length, int * out_length)
void GpGadgets::ClipPolygon(gpiPoint * pIn, gpiPoint * pOut, int inLength, int * pOutLength)
{
	gpiPoint clip_boundary[5];
	static gpiPoint * tmp_corners = NULL; // @global
	if(!P_Clip) {
		memcpy(pOut, pIn, inLength * sizeof(gpiPoint));
		*pOutLength = inLength;
	}
	else {
		tmp_corners = (gpiPoint *)gp_realloc(tmp_corners, 2 * inLength * sizeof(gpiPoint), "clip_polygon");
		// vertices of the rectangular clipping window starting from
		// top-left in counterclockwise direction 
		clip_boundary[0].x = P_Clip->xleft; /* top left */
		clip_boundary[0].y = P_Clip->ytop;
		clip_boundary[1].x = P_Clip->xleft; /* bottom left */
		clip_boundary[1].y = P_Clip->ybot;
		clip_boundary[2].x = P_Clip->xright; /* bottom right */
		clip_boundary[2].y = P_Clip->ybot;
		clip_boundary[3].x = P_Clip->xright; /* top right */
		clip_boundary[3].y = P_Clip->ytop;
		clip_boundary[4] = clip_boundary[0];

		memcpy(tmp_corners, pIn, inLength * sizeof(gpiPoint));
		for(size_t i = 0; i < 4; i++) {
			clip_polygon_to_boundary(tmp_corners, pOut, inLength, pOutLength, clip_boundary+i);
			memcpy(tmp_corners, pOut, *pOutLength * sizeof(gpiPoint));
			inLength = *pOutLength;
		}
	}
}

// Two routines to emulate move/vector sequence using line drawing routine
static uint move_pos_x, move_pos_y;

void clip_move(uint x, uint y)
{
	move_pos_x = x;
	move_pos_y = y;
}

//void clip_vector(uint x, uint y)
void GpGadgets::ClipVector(GpTermEntry * pT, uint x, uint y)
{
	DrawClipLine(pT, move_pos_x, move_pos_y, x, y);
	move_pos_x = x;
	move_pos_y = y;
}
//
// Common routines for setting text or line color from t_colorspec 
//
//void apply_pm3dcolor(t_colorspec * tc)
void GpGadgets::ApplyPm3DColor(GpTermEntry * pT, t_colorspec * pCs)
{
	double cbval;
	// V5 - term->_LineType(LT_BLACK) would clobber the current
	// dashtype so instead we use term->set_color(black).	
	static t_colorspec black(TC_LT, LT_BLACK, 0.0); // = BLACK_COLORSPEC;
	// Replace colorspec with that of the requested line style 
	lp_style_type style;
	if(pCs->type == TC_LINESTYLE) {
		lp_use_properties(&style, pCs->lt);
		pCs = &style.pm3d_color;
	}
	if(pCs->type == TC_DEFAULT) {
		pT->set_color(&black);
	}
	else if(pCs->type == TC_LT) {
		// Removed Jan 2015 if(!monochrome_terminal)
		pT->set_color(pCs);
	}
	else if(pCs->type == TC_RGB) {
		// FIXME: several plausible ways for monochrome terminals to handle color request
		// (1) Allow all color requests despite the label "monochrome"
		// (2) Choose any color you want so long as it is black
		// (3) Convert colors to gray scale (NTSC?)
		//
		// Monochrome terminals are still allowed to display rgb variable colors 
		pT->set_color(((pT->flags & TERM_MONOCHROME) && pCs->value >= 0) ? &black : pCs);
	}
	else if(pCs->type == TC_VARIABLE) { // Leave unchanged. (used only by "set errorbars"??) 
		;
	}
	else if(!is_plot_with_palette()) {
		pT->set_color(&black);
	}
	else {
		switch(pCs->type) {
			case TC_Z:
				pT->SetColor(CB2Gray(Z2CB(pCs->value)));
				break;
			case TC_CB:
				{
					const GpAxis & r_ax = GetCB();
					cbval = (r_ax.Flags & GpAxis::fLog) ? ((pCs->value <= 0) ? r_ax.Range.low : (log(pCs->value) / r_ax.log_base)) : pCs->value;
				}
				pT->SetColor(CB2Gray(cbval));
				break;
			case TC_FRAC:
				pT->SetColor(SmPalette.positive == SMPAL_POSITIVE ?  pCs->value : 1-pCs->value);
				break;
		}
	}
}

void reset_textcolor(const t_colorspec * tc)
{
	if(tc->type != TC_DEFAULT)
		term->_LineType(LT_BLACK);
}

void default_arrow_style(arrow_style_type * arrow)
{
	static const lp_style_type tmp_lp_style(lp_style_type::defCommon); // = DEFAULT_LP_STYLE_TYPE;

	arrow->tag = -1;
	arrow->layer = LAYER_BACK;
	arrow->lp_properties = tmp_lp_style;
	arrow->head = (t_arrow_head)1;
	arrow->head_length = 0.0;
	arrow->head_lengthunit = first_axes;
	arrow->head_angle = 15.0;
	arrow->head_backangle = 90.0;
	arrow->headfill = AS_NOFILL;
	arrow->head_fixedsize = false;
}

//void apply_head_properties(arrow_style_type * arrow_properties)
void GpGadgets::ApplyHeadProperties(const arrow_style_type & rArrowProperties)
{
	curr_arrow_headfilled = rArrowProperties.headfill;
	curr_arrow_headfixedsize = rArrowProperties.head_fixedsize;
	curr_arrow_headlength = 0;
	if(rArrowProperties.head_length > 0) {
		// set head length+angle for term->arrow 
		double xtmp, ytmp;
		GpPosition headsize;
		headsize.Set(first_axes, graph, graph, 0., 0., 0.);
		headsize.x = rArrowProperties.head_length;
		headsize.scalex = (position_type)rArrowProperties.head_lengthunit;
		MapPositionR(headsize, &xtmp, &ytmp, "arrow");
		curr_arrow_headangle = rArrowProperties.head_angle;
		curr_arrow_headbackangle = rArrowProperties.head_backangle;
		curr_arrow_headlength = (int)xtmp;
	}
}

//void get_offsets(GpTextLabel * pLabel, int * pHTic, int * pVTic)
void GpGadgets::GetOffsets(GpTextLabel * pLabel, int * pHTic, int * pVTic)
{
	if((pLabel->lp_properties.flags & LP_SHOW_POINTS)) {
		*pHTic = (int)(PtSz * term->HTic * 0.5);
		*pVTic = (int)(PtSz * term->VTic * 0.5);
	}
	else {
		*pHTic = 0;
		*pVTic = 0;
	}
	if(Is3DPlot) {
		int htic2, vtic2;
		Map3DPositionR(pLabel->offset, &htic2, &vtic2, "get_offsets");
		*pHTic += htic2;
		*pVTic += vtic2;
	}
	else {
		double htic2, vtic2;
		MapPositionR(pLabel->offset, &htic2, &vtic2, "get_offsets");
		*pHTic += (int)htic2;
		*pVTic += (int)vtic2;
	}
}
//
// Write one label, with all the trimmings.
// This routine is used for both 2D and 3D plots.
//
//void write_label(GpTermEntry * pT, uint x, uint y, GpTextLabel * pLabel)
void GpGadgets::WriteLabel(GpTermEntry * pT, uint x, uint y, GpTextLabel * pLabel)
{
	int    htic, vtic;
	VERT_JUSTIFY justify = JUST_TOP; // This was the 2D default; 3D had CENTRE 
	ApplyPm3DColor(pT, &(pLabel->textcolor));
	ignore_enhanced(pLabel->noenhanced);
	// The text itself 
	if(pLabel->hypertext) {
		// Treat text as hypertext 
		const char * p_font = pLabel->font;
		if(p_font)
			pT->set_font(p_font);
		if(pT->hypertext)
			pT->hypertext(TERM_HYPERTEXT_TOOLTIP, pLabel->text);
		if(p_font)
			pT->set_font("");
	}
	else {
		// A normal label (always print text) 
		GetOffsets(pLabel, &htic, &vtic);
#ifdef EAM_BOXED_TEXT
		// Initialize the bounding box accounting 
		if(pLabel->boxed && pT->boxed_text)
			(*pT->boxed_text)(x + htic, y + vtic, TEXTBOX_INIT);
#endif
		if(pLabel->rotate && (*pT->text_angle)(pLabel->rotate)) {
			pT->DrawMultiline(x + htic, y + vtic, pLabel->text, pLabel->pos, justify, pLabel->rotate, pLabel->font);
			(*pT->text_angle)(0);
		}
		else {
			pT->DrawMultiline(x + htic, y + vtic, pLabel->text, pLabel->pos, justify, 0, pLabel->font);
		}
	}
#ifdef EAM_BOXED_TEXT
	// Adjust the bounding box margins 
	if(pLabel->boxed && pT->boxed_text)
		(*pT->boxed_text)((int)(textbox_opts.xmargin * 100.0), (int)(textbox_opts.ymargin * 100.0), TEXTBOX_MARGINS);
	if(pLabel->boxed && pT->boxed_text && textbox_opts.opaque) {
		// Blank out the box and reprint the label 
		(*pT->boxed_text)(0, 0, TEXTBOX_BACKGROUNDFILL);
		if(pLabel->rotate && (*pT->text_angle)(pLabel->rotate)) {
			pT->DrawMultiline(x + htic, y + vtic, pLabel->text, pLabel->pos, justify, pLabel->rotate, pLabel->font);
			(*pT->text_angle)(0);
		}
		else {
			pT->DrawMultiline(x + htic, y + vtic, pLabel->text, pLabel->pos, justify, 0, pLabel->font);
		}
	}
	// Draw the bounding box - FIXME should set line properties first 
	if(pLabel->boxed && pT->boxed_text) {
		(*pT->boxed_text)(0, 0, textbox_opts.noborder ? TEXTBOX_FINISH : TEXTBOX_OUTLINE);
	}
#endif
	// The associated point, if any 
	// pT->DrawMultiline() clips text to on_page; do the same for any point 
	if((pLabel->lp_properties.flags & LP_SHOW_POINTS) && on_page(x, y)) {
		ApplyLpProperties(pT, &pLabel->lp_properties);
		(*pT->point)(x, y, pLabel->lp_properties.p_type);
		// the default label color is that of border 
		ApplyLpProperties(pT, &BorderLp);
	}
	ignore_enhanced(false);
}
//
// STR points to a label string, possibly with several lines separated
// by \n.  Return the number of characters in the longest line.  If
// LINES is not NULL, set *LINES to the number of lines in the label.
//
int label_width(const char * str, int * pLines)
{
	int    mlen = 0;
	if(isempty(str)) {
		ASSIGN_PTR(pLines, 0);
	}
	else {
		char * s, * e;
		int    len = 0;
		int    lnc = 0;
		char * lab = (char *)malloc(strlen(str) + 2);
		strcpy(lab, str);
		strcat(lab, "\n");
		s = lab;
		while((e = (char*)strchr(s, '\n')) != NULL) {
			*e = '\0';
			len = estimate_strlen(s); /* = e-s ? */
			if(len > mlen)
				mlen = len;
			if(len || lnc || *str == '\n')
				lnc++;
			s = ++e;
		}
		// lines = NULL => not interested - div
		ASSIGN_PTR(pLines, lnc);
		free(lab);
	}
	return mlen;
}

