/* GNUPLOT - boundary.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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

#define ERRORBARTIC(terminalPtr) MAX(((terminalPtr)->h_tic/2), 1)

/*{{{  local variables */
static int xlablin, x2lablin, ylablin, y2lablin, titlelin, xticlin, x2ticlin;

/*{{{  local and global variables */
static int key_sample_width;    /* width of line sample */
static int key_sample_height;   /* sample itself; does not scale with "set key spacing" */
static int key_sample_left;     /* offset from x for left of line sample */
static int key_sample_right;    /* offset from x for right of line sample */
static int key_text_left;       /* offset from x for left-justified text */
static int key_text_right;      /* offset from x for right-justified text */
static int key_size_left;       /* size of left bit of key (text or sample, depends on key->reverse) */
static int key_size_right;      /* size of right part of key (including padding) */
static int key_xleft;           /* Amount of space on the left required by the key */
static int max_ptitl_len = 0;   /* max length of plot-titles (keys) */
static int ptitl_cnt;           /* count keys with len > 0  */

static int key_width;           /* calculate once, then everyone uses it */
static int key_height;          /* ditto */
static int key_title_height;    /* nominal number of lines * character height */
static int key_title_extra;     /* allow room for subscript/superscript */
static int key_title_ypos;      /* offset from key->bounds.ytop */
static int time_y, time_x;

int title_x, title_y;           /* Used by boundary and by 2D graphics */

/*
 * These quantities are needed in do_plot() e.g. for histogtram title layout
 */
int key_entry_height;           /* bigger of t->v_char, t->v_tic */
int key_point_offset;           /* offset from x for point sample */
int ylabel_x, y2label_x, xlabel_y, x2label_y;
int x2label_yoffset;
int ylabel_y, y2label_y, xtic_y, x2tic_y, ytic_x, y2tic_x;
int key_rows;
int key_cols;
int key_count;

static int key_col_wth, yl_ref;
static int xl, yl;

/*{{{  boundary() */
/* borders of plotting area
 * computed once on every call to do_plot
 *
 * The order in which things are done has become critical:
 *  GPO.V.BbPlot.ytop depends on title, x2label
 *  GPO.V.BbPlot.ybot depends on key, if "under"
 *  once we have these, we can setup the y1 and y2 tics and the
 *  only then can we calculate GPO.V.BbPlot.xleft and GPO.V.BbPlot.xright
 *  GPO.V.BbPlot.xright depends also on key RIGHT
 *  then we can do x and x2 tics
 *
 * For set size ratio ..., everything depends on everything else...
 * not really a lot we can do about that, so we lose if the plot has to
 * be reduced vertically. But the chances are the
 * change will not be very big, so the number of tics will not
 * change dramatically.
 *
 * Margin computation redone by Dick Crawford (rccrawford@lanl.gov) 4/98
 */
//void boundary(termentry * pTerm, curve_points * plots, int count)
void GnuPlot::Boundary(termentry * pTerm, curve_points * plots, int count)
{
	int    yticlin = 0;
	int    y2ticlin = 0;
	legend_key * key = &keyT;
	//struct termentry * t = term;
	const  int can_rotate = (pTerm->text_angle)(TEXT_VERTICAL);
	int    xtic_textheight = 0; /* height of xtic labels */
	int    x2tic_textheight = 0; /* height of x2tic labels */
	int    title_textheight = 0; /* height of title */
	int    xlabel_textheight = 0; /* height of xlabel */
	int    x2label_textheight = 0; /* height of x2label */
	int    ylabel_textwidth = 0; /* width of (rotated) ylabel */
	int    y2label_textwidth = 0; /* width of (rotated) y2label */
	int    timelabel_textwidth = 0; /* width of timestamp */
	int    timelabel_textheight = 0; /* height of timestamp */
	int    ytic_textwidth = 0; /* width of ytic labels */
	int    y2tic_textwidth = 0; /* width of y2tic labels */
	int    x2tic_height = 0;   /* 0 for tic_in or no x2tics, ticscale*v_tic otherwise */
	int    xtic_textwidth = 0; /* amount by which the xtic label protrude to the right */
	int    xtic_height = 0;
	int    ytic_width = 0;
	int    y2tic_width = 0;
	int    ttic_textheight = 0; // vertical clearance for ttics 
	// figure out which rotatable items are to be rotated
	// (ylabel and y2label are rotated if possible) 
	const int vertical_timelabel = can_rotate ? timelabel.rotate : 0;
	const int vertical_xtics  = can_rotate ? AxS[FIRST_X_AXIS].tic_rotate : 0;
	const int vertical_x2tics = can_rotate ? AxS[SECOND_X_AXIS].tic_rotate : 0;
	const int vertical_ytics  = can_rotate ? AxS[FIRST_Y_AXIS].tic_rotate : 0;
	const int vertical_y2tics = can_rotate ? AxS[SECOND_Y_AXIS].tic_rotate : 0;
	bool shift_labels_to_border = FALSE;
	xticlin = ylablin = y2lablin = xlablin = x2lablin = titlelin = 0;
	/*{{{  count lines in labels and tics */
	if(title.text)
		label_width(title.text, &titlelin);
	if(AxS[FIRST_X_AXIS].label.text)
		label_width(AxS[FIRST_X_AXIS].label.text, &xlablin);
	// This should go *inside* label_width(), but it messes up the key title 
	// Imperfect check for subscripts or superscripts 
	if((pTerm->flags & TERM_ENHANCED_TEXT) && AxS[FIRST_X_AXIS].label.text && strpbrk(AxS[FIRST_X_AXIS].label.text, "_^"))
		xlablin++;
	if(AxS[SECOND_X_AXIS].label.text)
		label_width(AxS[SECOND_X_AXIS].label.text, &x2lablin);
	if(AxS[FIRST_Y_AXIS].label.text)
		label_width(AxS[FIRST_Y_AXIS].label.text, &ylablin);
	if(AxS[SECOND_Y_AXIS].label.text)
		label_width(AxS[SECOND_Y_AXIS].label.text, &y2lablin);
	if(AxS[FIRST_X_AXIS].ticmode) {
		label_width(AxS[FIRST_X_AXIS].formatstring, &xticlin);
		// Reserve room for user tic labels even if format of autoticks is "" 
		if(xticlin == 0 && AxS[FIRST_X_AXIS].ticdef.def.user)
			xticlin = 1;
	}
	if(AxS[SECOND_X_AXIS].ticmode)
		label_width(AxS[SECOND_X_AXIS].formatstring, &x2ticlin);
	if(AxS[FIRST_Y_AXIS].ticmode)
		label_width(AxS[FIRST_Y_AXIS].formatstring, &yticlin);
	if(AxS[SECOND_Y_AXIS].ticmode)
		label_width(AxS[SECOND_Y_AXIS].formatstring, &y2ticlin);
	/*}}} */

	/*{{{  preliminary V.BbPlot.ytop  calculation */

	/*     first compute heights of things to be written in the margin */

	/* Title placement has been reworked for 5.4
	 * NOTE: title_textheight is _not_ the height of the title!
	 * It is the amount of space reserved for the title above the plot.
	 * A negative offset greater than the number of title lines means
	 * that the title will appear inside the boundary and no extra space
	 * needs to be reserved for it above the plot.
	 */
	title_textheight = 0;
	if(titlelin) {
		title_textheight = pTerm->v_char; /* Gap of one normal line height */
		if(title.font)
			pTerm->set_font(title.font);
		title_y = titlelin * pTerm->v_char;
		if((titlelin + title.offset.y) > 0)
			title_textheight += titlelin * pTerm->v_char;
		if(title.font)
			pTerm->set_font("");
		title_y += 0.5 * pTerm->v_char; /* Approximate same placement as version 5.2 */
	}
	// Extra space at the top for spiderplot axis label 
	if(spiderplot)
		title_textheight += 1.5 * pTerm->v_char;
	// x2label 
	if(x2lablin) {
		double tmpx, tmpy;
		MapPositionR(pTerm, &(AxS[SECOND_X_AXIS].label.offset), &tmpx, &tmpy, "x2label");
		if(AxS[SECOND_X_AXIS].label.font)
			pTerm->set_font(AxS[SECOND_X_AXIS].label.font);
		x2label_textheight = (int)(x2lablin * pTerm->v_char);
		x2label_yoffset = tmpy;
		if(AxS[SECOND_X_AXIS].label.font)
			pTerm->set_font("");
	}
	else
		x2label_textheight = 0;
	// tic labels 
	if(AxS[SECOND_X_AXIS].ticmode & TICS_ON_BORDER) {
		// ought to consider tics on axes if axis near border 
		x2tic_textheight = (int)(x2ticlin * pTerm->v_char);
	}
	else
		x2tic_textheight = 0;
	// tics 
	if(AxS[SECOND_X_AXIS].ticmode & TICS_ON_BORDER) {
		x2tic_height = pTerm->v_tic * AxS[SECOND_X_AXIS].ticscale;
		if(AxS[SECOND_X_AXIS].tic_in)
			x2tic_height = -x2tic_height;
	}
	else
		x2tic_height = 0;
	// Polar (theta) tic labels need space at top and bottom of plot 
	if(AxS.Theta().ticmode) {
		// FIXME:  Really 5% of polar grid radius, but we don't know that yet 
		ttic_textheight = 2.0 * pTerm->v_char;
	}
	// timestamp 
	if(timelabel.text) {
		int timelin;
		timelabel_textwidth = label_width(timelabel.text, &timelin);
		if(vertical_timelabel) {
			timelabel_textheight = timelabel_textwidth * pTerm->v_char;
			timelabel_textwidth = (timelin + 1.5) * pTerm->h_char;
			timelabel.place.y = 0;
		}
		else {
			timelabel_textheight = timelin * pTerm->v_char;
			timelabel_textwidth = timelabel_textwidth * pTerm->h_char;
			// save textheight for use in do_key_bounds() 
			timelabel.place.y = timelabel_textheight;
		}
	}
	// ylabel placement 
	if(AxS[FIRST_Y_AXIS].label.text) {
		if(can_rotate && AxS[FIRST_Y_AXIS].label.rotate != 0) {
			ylabel_textwidth = ylablin * pTerm->v_char;
		}
		else {
			/* Trying to estimate this length caused more problems than it solved.
			 * For one thing it comes out wrong for text passed to TeX terminals.
			 * Assume the common case is roughly 3 character widths and let the
			 * user adjust lmargin and offset for longer non-rotated ylabels.
			 */
			ylabel_textwidth = 3 * pTerm->h_char;
		}
	}
	// y2label placement 
	if(AxS[SECOND_Y_AXIS].label.text) {
		if(can_rotate && AxS[SECOND_Y_AXIS].label.rotate != 0) {
			y2label_textwidth = y2lablin * pTerm->v_char;
			if(!AxS[SECOND_Y_AXIS].ticmode)
				y2label_textwidth += 0.5 * pTerm->v_char;
		}
		else {
			// See above. Estimating true text length causes problems 
			y2label_textwidth = 3 * pTerm->h_char;
		}
	}
	// compute V.BbPlot.ytop from the various components unless tmargin is explicitly specified
	V.BbPlot.ytop = (int)(0.5 + (V.YSize + V.YOffset) * (pTerm->ymax-1));
	// Sanity check top and bottom margins, in case the user got confused 
	if(V.MarginB.scalex == screen && V.MarginT.scalex == screen) {
		if(V.MarginB.x > V.MarginT.x) {
			double tmp = V.MarginB.x;
			V.MarginB.x = V.MarginT.x;
			V.MarginT.x = tmp;
		}
	}
	if(V.MarginT.scalex == screen) {
		V.BbPlot.ytop = (V.MarginT.x) * (float)(pTerm->ymax-1); // Specified as absolute position on the canvas 
	}
	else if(V.MarginT.x >=0) {
		V.BbPlot.ytop -= (int)(V.MarginT.x * (float)pTerm->v_char + 0.5); // Specified in terms of character height 
	}
	else {
		// Auto-calculation of space required 
		int top_margin = title_textheight;
		if(x2label_textheight + x2label_yoffset > 0)
			top_margin += x2label_textheight;
		if(timelabel_textheight > top_margin && !timelabel_bottom && !vertical_timelabel)
			top_margin = timelabel_textheight;
		top_margin += x2tic_textheight;
		top_margin += pTerm->v_char;
		if(x2tic_height > 0)
			top_margin += x2tic_height;
		top_margin += ttic_textheight;
		V.BbPlot.ytop -= top_margin;
		if(V.BbPlot.ytop == (int)(0.5 + (V.YSize + V.YOffset) * (pTerm->ymax-1))) {
			V.BbPlot.ytop -= (int)(pTerm->h_char * 2); // make room for the end of rotated ytics or y2tics 
		}
	}
	// end of preliminary V.BbPlot.ytop calculation }}} 
	// {{{  preliminary V.BbPlot.xleft, needed for "under" 
	if(V.MarginL.scalex == screen)
		V.BbPlot.xleft = V.MarginL.x * (float)pTerm->xmax;
	else
		V.BbPlot.xleft = V.XOffset * pTerm->xmax + pTerm->h_char * (V.MarginL.x >= 0 ? V.MarginL.x : 1);
	// }}} 
	// {{{  tentative V.BbPlot.xright, needed for "under" 
	if(V.MarginR.scalex == screen)
		V.BbPlot.xright = V.MarginR.x * (float)(pTerm->xmax - 1);
	else
		V.BbPlot.xright = (V.XSize + V.XOffset) * (pTerm->xmax - 1) - pTerm->h_char * (V.MarginR.x >= 0 ? V.MarginR.x : 2);
	// }}} 
	// {{{  preliminary V.BbPlot.ybot calculation first compute heights of labels and tics 
	// tic labels 
	shift_labels_to_border = FALSE;
	if(AxS[FIRST_X_AXIS].ticmode & TICS_ON_AXIS) {
		/* FIXME: This test for how close the axis is to the border does not match */
		/*        the tests in axis_output_tics(), and assumes FIRST_Y_AXIS.       */
		if(!inrange(0.0, AxS[FIRST_Y_AXIS].min, AxS[FIRST_Y_AXIS].max))
			shift_labels_to_border = TRUE;
		if(0.05 > fabs(AxS[FIRST_Y_AXIS].min / (AxS[FIRST_Y_AXIS].max - AxS[FIRST_Y_AXIS].min)))
			shift_labels_to_border = TRUE;
	}
	if((AxS[FIRST_X_AXIS].ticmode & TICS_ON_BORDER) || shift_labels_to_border) {
		xtic_textheight = (int)(pTerm->v_char * (xticlin + 1));
	}
	else
		xtic_textheight =  0;
	// tics 
	if(!AxS[FIRST_X_AXIS].tic_in && ((AxS[FIRST_X_AXIS].ticmode & TICS_ON_BORDER) || 
		((AxS[SECOND_X_AXIS].ticmode & TICS_MIRROR) && (AxS[SECOND_X_AXIS].ticmode & TICS_ON_BORDER))))
		xtic_height = (int)(pTerm->v_tic * AxS[FIRST_X_AXIS].ticscale);
	else
		xtic_height = 0;
	// xlabel 
	if(xlablin) {
		double tmpx, tmpy;
		MapPositionR(pTerm, &(AxS[FIRST_X_AXIS].label.offset), &tmpx, &tmpy, "boundary");
		// offset is subtracted because if > 0, the margin is smaller 
		// textheight is inflated by 0.2 to allow descenders to clear bottom of canvas 
		xlabel_textheight = (((float)xlablin + 0.2) * pTerm->v_char - tmpy);
		if(!AxS[FIRST_X_AXIS].ticmode)
			xlabel_textheight += 0.5 * pTerm->v_char;
	}
	else
		xlabel_textheight = 0;
	// compute V.BbPlot.ybot from the various components unless bmargin is explicitly specified  
	V.BbPlot.ybot = V.YOffset * (float)pTerm->ymax;
	if(V.MarginB.scalex == screen) {
		// Absolute position for bottom of plot 
		V.BbPlot.ybot = V.MarginB.x * (float)pTerm->ymax;
	}
	else if(V.MarginB.x >= 0) {
		// Position based on specified character height 
		V.BbPlot.ybot += V.MarginB.x * (float)pTerm->v_char + 0.5;
	}
	else {
		V.BbPlot.ybot += xtic_height + xtic_textheight;
		if(xlabel_textheight > 0)
			V.BbPlot.ybot += xlabel_textheight;
		if(!vertical_timelabel && timelabel_bottom && timelabel_textheight > 0)
			V.BbPlot.ybot += timelabel_textheight;
		if(V.BbPlot.ybot == (int)(pTerm->ymax * V.YOffset)) {
			// make room for the end of rotated ytics or y2tics 
			V.BbPlot.ybot += (int)(pTerm->h_char * 2);
		}
		if(spiderplot) // Extra space at the bottom for spiderplot axis label 
			V.BbPlot.ybot += 2 * pTerm->h_char;
		/* Last chance for better estimate of space required for ttic labels */
		/* It is too late to go back and adjust positions relative to ytop */
		if(ttic_textheight > 0) {
			ttic_textheight = 0.05 * (V.BbPlot.ytop - V.BbPlot.ybot);
			V.BbPlot.ybot += ttic_textheight;
		}
	}
	/*  end of preliminary V.BbPlot.ybot calculation }}} */
	// Determine the size and position of the key box 
	if(key->visible) {
		// Count max_len key and number keys with len > 0 
		max_ptitl_len = find_maxl_keys(plots, count, &ptitl_cnt);
		DoKeyLayout(pTerm, key);
	}
	// Adjust range of dependent axes y and y2 
	if(nonlinear(&AxS[FIRST_Y_AXIS]))
		extend_primary_ticrange(&AxS[FIRST_Y_AXIS]);
	if(nonlinear(&AxS[SECOND_Y_AXIS]))
		extend_primary_ticrange(&AxS[SECOND_Y_AXIS]);
	setup_tics(&AxS[FIRST_Y_AXIS], 20);
	setup_tics(&AxS[SECOND_Y_AXIS], 20);
	// Adjust color axis limits if necessary. 
	if(is_plot_with_palette()) {
		AxisCheckedExtendEmptyRange(COLOR_AXIS, "All points of color axis undefined.");
		if(color_box.where != SMCOLOR_BOX_NO)
			setup_tics(&AxS[COLOR_AXIS], 20);
	}
	/*{{{  recompute V.BbPlot.xleft based on widths of ytics, ylabel etc
	   unless it has been explicitly set by lmargin */

	/* tic labels */
	shift_labels_to_border = FALSE;
	if(AxS[FIRST_Y_AXIS].ticmode & TICS_ON_AXIS) {
		/* FIXME: This test for how close the axis is to the border does not match */
		/*        the tests in axis_output_tics(), and assumes FIRST_X_AXIS.       */
		if(!inrange(0.0, AxS[FIRST_X_AXIS].min, AxS[FIRST_X_AXIS].max))
			shift_labels_to_border = TRUE;
		if(0.1 > fabs(AxS[FIRST_X_AXIS].min / (AxS[FIRST_X_AXIS].max - AxS[FIRST_X_AXIS].min)))
			shift_labels_to_border = TRUE;
	}
	if((AxS[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER) || shift_labels_to_border) {
		if(vertical_ytics)
			/* HBB: we will later add some white space as part of this, so
			 * reserve two more rows (one above, one below the text ...).
			 * Same will be done to similar calc.'s elsewhere */
			ytic_textwidth = (int)(pTerm->v_char * (yticlin + 2));
		else {
			widest_tic_strlen = 0; /* reset the global variable ... */
			/* get gen_tics to call widest_tic_callback with all labels
			 * the latter sets widest_tic_strlen to the length of the widest
			 * one ought to consider tics on axis if axis near border...
			 */
			gen_tics(&AxS[FIRST_Y_AXIS], widest_tic_callback);
			ytic_textwidth = (int)(pTerm->h_char * (widest_tic_strlen + 2));
		}
	}
	else if(AxS[FIRST_Y_AXIS].label.text) {
		// substitutes for extra space added to left of ytix labels 
		ytic_textwidth = 2 * (pTerm->h_char);
	}
	else {
		ytic_textwidth = 0;
	}
	/* tics */
	if(!AxS[FIRST_Y_AXIS].tic_in && ((AxS[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER) || 
		((AxS[SECOND_Y_AXIS].ticmode & TICS_MIRROR) && (AxS[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER))))
		ytic_width = (int)(pTerm->h_tic * AxS[FIRST_Y_AXIS].ticscale);
	else
		ytic_width = 0;
	if(V.MarginL.x < 0) {
		// Auto-calculation 
		int space_to_left = key_xleft;
		if(space_to_left < timelabel_textwidth && vertical_timelabel)
			space_to_left = timelabel_textwidth;
		if(space_to_left < ylabel_textwidth)
			space_to_left = ylabel_textwidth;
		V.BbPlot.xleft = V.XOffset * pTerm->xmax;
		V.BbPlot.xleft += space_to_left;
		V.BbPlot.xleft += ytic_width + ytic_textwidth;
		if(V.BbPlot.xleft - ytic_width - ytic_textwidth < 0)
			V.BbPlot.xleft = ytic_width + ytic_textwidth;
		if(V.BbPlot.xleft == pTerm->xmax * V.XOffset)
			V.BbPlot.xleft += pTerm->h_char * 2;
		// DBT 12-3-98  extra margin just in case 
		V.BbPlot.xleft += 0.5 * pTerm->h_char;
	}
	/* Note: we took care of explicit 'set lmargin foo' at line 492 */
	/*  end of V.BbPlot.xleft calculation }}} */

	/*{{{  recompute V.BbPlot.xright based on widest y2tic. y2labels, key "outside"
	   unless it has been explicitly set by rmargin */
	// tic labels 
	if(AxS[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER) {
		if(vertical_y2tics)
			y2tic_textwidth = (int)(pTerm->v_char * (y2ticlin + 2));
		else {
			widest_tic_strlen = 0; /* reset the global variable ... */
			/* get gen_tics to call widest_tic_callback with all labels
			 * the latter sets widest_tic_strlen to the length of the widest
			 * one ought to consider tics on axis if axis near border...
			 */
			gen_tics(&AxS[SECOND_Y_AXIS], widest_tic_callback);
			y2tic_textwidth = (int)(pTerm->h_char * (widest_tic_strlen + 2));
		}
	}
	else {
		y2tic_textwidth = 0;
	}
	// EAM May 2009
	// Check to see if any xtic labels are so long that they extend beyond
	// the right boundary of the plot. If so, allow extra room in the margin.
	// If the labels are too long to fit even with a big margin, too bad.
	if(AxS[FIRST_X_AXIS].ticdef.def.user) {
		ticmark * tic = AxS[FIRST_X_AXIS].ticdef.def.user;
		int maxrightlabel = V.BbPlot.xright;
		// We don't really know the plot layout yet, but try for an estimate 
		axis_set_scale_and_range(&AxS[FIRST_X_AXIS], V.BbPlot.xleft, V.BbPlot.xright);
		while(tic) {
			if(tic->label) {
				double xx;
				int length = static_cast<int>(estimate_strlen(tic->label, NULL) * cos(DEG2RAD * (double)(AxS[FIRST_X_AXIS].tic_rotate)) * pTerm->h_char);
				if(inrange(tic->position, AxS[FIRST_X_AXIS].set_min, AxS[FIRST_X_AXIS].set_max)) {
					xx = AxisLogValueChecked(FIRST_X_AXIS, tic->position, "xtic");
					xx = AxS.MapiX(xx);
					xx += (AxS[FIRST_X_AXIS].tic_rotate) ? length : length /2;
					if(maxrightlabel < xx)
						maxrightlabel = xx;
				}
			}
			tic = tic->next;
		}
		xtic_textwidth = maxrightlabel - V.BbPlot.xright;
		if(xtic_textwidth > pTerm->xmax/4) {
			xtic_textwidth = pTerm->xmax/4;
			IntWarn(NO_CARET, "difficulty making room for xtic labels");
		}
	}
	// tics 
	if(!AxS[SECOND_Y_AXIS].tic_in && ((AxS[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER) || 
		((AxS[FIRST_Y_AXIS].ticmode & TICS_MIRROR) && (AxS[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER))))
		y2tic_width = (int)(pTerm->h_tic * AxS[SECOND_Y_AXIS].ticscale);
	else
		y2tic_width = 0;
	// Make room for the color box if needed. 
	if(V.MarginR.scalex != screen) {
		if(is_plot_with_colorbox()) {
#define COLORBOX_SCALE 0.100
#define WIDEST_COLORBOX_TICTEXT 3
			if((color_box.where != SMCOLOR_BOX_NO) && (color_box.where != SMCOLOR_BOX_USER)) {
				V.BbPlot.xright -= (int)(V.BbPlot.xright-V.BbPlot.xleft)*COLORBOX_SCALE;
				V.BbPlot.xright -= (int)((pTerm->h_char) * WIDEST_COLORBOX_TICTEXT);
			}
			color_box.xoffset = 0;
		}
		if(V.MarginR.x < 0) {
			color_box.xoffset = V.BbPlot.xright;
			V.BbPlot.xright -= y2tic_width + y2tic_textwidth;
			if(y2label_textwidth > 0)
				V.BbPlot.xright -= y2label_textwidth;
			if(V.BbPlot.xright > (V.XSize+V.XOffset)*(pTerm->xmax-1) - (pTerm->h_char * 2))
				V.BbPlot.xright = (V.XSize+V.XOffset)*(pTerm->xmax-1) - (pTerm->h_char * 2);
			color_box.xoffset -= V.BbPlot.xright;
			// EAM 2009 - protruding xtic labels 
			if((static_cast<int>(pTerm->xmax) - V.BbPlot.xright) < xtic_textwidth)
				V.BbPlot.xright = pTerm->xmax - xtic_textwidth;
			// DBT 12-3-98  extra margin just in case 
			V.BbPlot.xright -= 1.0 * pTerm->h_char;
		}
		/* Note: we took care of explicit 'set rmargin foo' at line 502 */
	}
	/*  end of V.BbPlot.xright calculation }}} */

	/* Set up x and x2 tics */
	/* we should base the guide on the width of the xtics, but we cannot
	 * use widest_tics until tics are set up. Bit of a downer - let us
	 * assume tics are 5 characters wide
	 */
	setup_tics(&AxS[FIRST_X_AXIS], 20);
	setup_tics(&AxS[SECOND_X_AXIS], 20);
	// Make sure that if polar grid is shown on a cartesian axis plot
	// the rtics match up with the primary x tics.                    
	if(AxS.__R().ticmode && (polar || raxis)) {
		if(AxS.__R().BadRange() || (!polar && AxS.__R().min != 0)) {
			set_explicit_range(&AxS.__R(), 0.0, AxS.__X().max);
			AxS.__R().min = 0;
			AxS.__R().max = AxS[FIRST_X_AXIS].max;
			IntWarn(NO_CARET, "resetting rrange");
		}
		setup_tics(&AxS[POLAR_AXIS], 10);
	}
	// Modify the bounding box to fit the aspect ratio, if any was given 
	if(aspect_ratio != 0.0) {
		double current_aspect_ratio;
		double current, required;
		if(aspect_ratio < 0 && (AxS.__X().max - AxS.__X().min) != 0.0) {
			current_aspect_ratio = -aspect_ratio * fabs((AxS.__Y().max - AxS.__Y().min) / (AxS.__X().max - AxS.__X().min));
		}
		else
			current_aspect_ratio = aspect_ratio;
		if(current_aspect_ratio < 0.005 || current_aspect_ratio > 2000.0)
			IntWarn(NO_CARET, "extreme aspect ratio");
		current = ((double)(V.BbPlot.ytop - V.BbPlot.ybot)) / ((double)(V.BbPlot.xright - V.BbPlot.xleft));
		required = (current_aspect_ratio * pTerm->v_tic) / pTerm->h_tic;
		// Fixed borders take precedence over centering 
		if(current > required) {
			// too tall 
			int old_height = V.BbPlot.ytop - V.BbPlot.ybot;
			int new_height = static_cast<int>(required * (V.BbPlot.xright - V.BbPlot.xleft));
			if(V.MarginB.scalex == screen)
				V.BbPlot.ytop = V.BbPlot.ybot + new_height;
			else if(V.MarginT.scalex == screen)
				V.BbPlot.ybot = V.BbPlot.ytop - new_height;
			else {
				V.BbPlot.ybot += (old_height - new_height) / 2;
				V.BbPlot.ytop -= (old_height - new_height) / 2;
			}
		}
		else {
			// too wide 
			int old_width = V.BbPlot.xright - V.BbPlot.xleft;
			int new_width = static_cast<int>((V.BbPlot.ytop - V.BbPlot.ybot) / required);
			if(V.MarginL.scalex == screen)
				V.BbPlot.xright = V.BbPlot.xleft + new_width;
			else if(V.MarginR.scalex == screen)
				V.BbPlot.xleft = V.BbPlot.xright - new_width;
			else {
				V.BbPlot.xleft += (old_width - new_width) / 2;
				V.BbPlot.xright -= (old_width - new_width) / 2;
			}
		}
	}
	// 
	// Calculate space needed for tic label rotation.
	// If [tb]margin is auto, move the plot boundary.
	// Otherwise use textheight to adjust placement of various titles.
	// 
	if(AxS[SECOND_X_AXIS].ticmode & TICS_ON_BORDER && vertical_x2tics) {
		/* Assuming left justified tic labels. Correction below if they aren't */
		double projection = sin((double)AxS[SECOND_X_AXIS].tic_rotate*DEG2RAD);
		if(AxS[SECOND_X_AXIS].tic_pos == RIGHT)
			projection *= -1;
		else if(AxS[SECOND_X_AXIS].tic_pos == CENTRE)
			projection = 0.5*fabs(projection);
		widest_tic_strlen = 0;  /* reset the global variable ... */
		gen_tics(&AxS[SECOND_X_AXIS], widest_tic_callback);
		if(V.MarginT.x < 0) /* Undo original estimate */
			V.BbPlot.ytop += x2tic_textheight;
		// Adjust spacing for rotation 
		if(projection > 0.0)
			x2tic_textheight += (int)(pTerm->h_char * (widest_tic_strlen)) * projection;
		if(V.MarginT.x < 0)
			V.BbPlot.ytop -= x2tic_textheight;
	}
	if(AxS[FIRST_X_AXIS].ticmode & TICS_ON_BORDER && vertical_xtics) {
		double projection;
		// This adjustment will happen again in axis_output_tics but we need it now 
		if(AxS[FIRST_X_AXIS].tic_rotate == TEXT_VERTICAL && !AxS[FIRST_X_AXIS].manual_justify)
			AxS[FIRST_X_AXIS].tic_pos = RIGHT;
		if(AxS[FIRST_X_AXIS].tic_rotate == 90)
			projection = -1.0;
		else if(AxS[FIRST_X_AXIS].tic_rotate == TEXT_VERTICAL)
			projection = -1.0;
		else
			projection = -sin((double)AxS[FIRST_X_AXIS].tic_rotate*DEG2RAD);
		if(AxS[FIRST_X_AXIS].tic_pos == RIGHT)
			projection *= -1;
		widest_tic_strlen = 0;  /* reset the global variable ... */
		gen_tics(&AxS[FIRST_X_AXIS], widest_tic_callback);
		if(V.MarginB.x < 0)
			V.BbPlot.ybot -= xtic_textheight;
		if(projection > 0.0)
			xtic_textheight = (int)(pTerm->h_char * widest_tic_strlen) * projection + pTerm->v_char;
		if(V.MarginB.x < 0)
			V.BbPlot.ybot += xtic_textheight;
	}
	// 
	// Notwithstanding all these fancy calculations,
	// V.BbPlot.ytop must always be above V.BbPlot.ybot
	// 
	if(V.BbPlot.ytop < V.BbPlot.ybot) {
		int i = V.BbPlot.ytop;
		V.BbPlot.ytop = V.BbPlot.ybot;
		V.BbPlot.ybot = i;
		FPRINTF((stderr, "boundary: Big problems! V.BbPlot.ybot > V.BbPlot.ytop\n"));
	}
	// compute coordinates for axis labels, title etc 
	x2label_y = V.BbPlot.ytop + x2label_textheight;
	x2label_y += 0.5 * pTerm->v_char;
	if(x2label_textheight + x2label_yoffset >= 0) {
		x2label_y += 1.5 * x2tic_textheight;
		// Adjust for the tics themselves 
		if(x2tic_height > 0)
			x2label_y += x2tic_height;
	}
	title_x = (V.BbPlot.xleft + V.BbPlot.xright) / 2;
	// title_y was previously set to the actual title height.
	// Further corrections to this placement only if it is above the plot
	title_y += V.BbPlot.ytop;
	if(titlelin + title.offset.y > 0) {
		title_y += x2tic_textheight;
		title_y += ttic_textheight;
		if(x2label_y + x2label_yoffset > V.BbPlot.ytop)
			title_y += x2label_textheight;
		if(x2tic_height > 0)
			title_y += x2tic_height;
	}
	// Shift upward by 0.2 line to allow for descenders in xlabel text 
	xlabel_y = V.BbPlot.ybot - xtic_height - xtic_textheight - xlabel_textheight + ((float)xlablin+0.2) * pTerm->v_char;
	xlabel_y -= ttic_textheight;
	ylabel_x = V.BbPlot.xleft - ytic_width - ytic_textwidth;
	ylabel_x -= ylabel_textwidth/2;
	y2label_x = V.BbPlot.xright + y2tic_width + y2tic_textwidth;
	y2label_x += y2label_textwidth/2;
	/* Nov 2016  - simplify placement of timestamp
	 * Stamp the same place on the page regardless of plot margins
	 */
	if(vertical_timelabel) {
		time_x = 1.5 * pTerm->h_char;
		if(timelabel_bottom)
			time_y = pTerm->v_char;
		else
			time_y = pTerm->ymax - pTerm->v_char;
	}
	else {
		time_x = 1.0 * pTerm->h_char;
		if(timelabel_bottom)
			time_y = timelabel_textheight - 0.5 * pTerm->v_char;
		else
			time_y = pTerm->ymax;
	}
	xtic_y = V.BbPlot.ybot - xtic_height - (int)(vertical_xtics ? pTerm->h_char : pTerm->v_char);
	x2tic_y = V.BbPlot.ytop + (x2tic_height > 0 ? x2tic_height : 0) + (vertical_x2tics ? (int)pTerm->h_char : pTerm->v_char);
	ytic_x = V.BbPlot.xleft - ytic_width - (vertical_ytics ? (ytic_textwidth - (int)pTerm->v_char) : (int)pTerm->h_char);
	y2tic_x = V.BbPlot.xright + y2tic_width + (int)(vertical_y2tics ? pTerm->v_char : pTerm->h_char);
	// restore text to horizontal [we tested rotation above] 
	(pTerm->text_angle)(0);
	// needed for map_position() below 
	axis_set_scale_and_range(&AxS[FIRST_X_AXIS], V.BbPlot.xleft, V.BbPlot.xright);
	axis_set_scale_and_range(&AxS[SECOND_X_AXIS], V.BbPlot.xleft, V.BbPlot.xright);
	axis_set_scale_and_range(&AxS[FIRST_Y_AXIS], V.BbPlot.ybot, V.BbPlot.ytop);
	axis_set_scale_and_range(&AxS[SECOND_Y_AXIS], V.BbPlot.ybot, V.BbPlot.ytop);
	DoKeyBounds(pTerm, key); // Calculate limiting bounds of the key 
	V.P_ClipArea = &V.BbPlot; // Set default clipping to the plot boundary 
	// Sanity checks 
	if(V.BbPlot.xright < V.BbPlot.xleft || V.BbPlot.ytop < V.BbPlot.ybot)
		IntWarn(NO_CARET, "Terminal canvas area too small to hold plot.\n\t    Check plot boundary and font sizes.");
}

/*}}} */

//void do_key_bounds(termentry * pTerm, legend_key * key)
void GnuPlot::DoKeyBounds(termentry * pTerm, legend_key * pKey)
{
	//struct termentry * t = term;
	key_height = key_title_height + key_title_extra + key_rows * key_entry_height + pKey->height_fix * key_entry_height;
	key_width = key_col_wth * key_cols;
	// Key inside plot boundaries 
	if(pKey->region == GPKEY_AUTO_INTERIOR_LRTBC || (pKey->region == GPKEY_AUTO_EXTERIOR_LRTBC && pKey->vpos == JUST_CENTRE && pKey->hpos == CENTRE)) {
		if(pKey->vpos == JUST_TOP) {
			pKey->bounds.ytop = V.BbPlot.ytop - pTerm->v_tic;
			pKey->bounds.ybot = pKey->bounds.ytop - key_height;
		}
		else if(pKey->vpos == JUST_BOT) {
			pKey->bounds.ybot = V.BbPlot.ybot + pTerm->v_tic;
			pKey->bounds.ytop = pKey->bounds.ybot + key_height;
		}
		else { // (key->vpos == JUST_CENTRE) 
			pKey->bounds.ybot = ((V.BbPlot.ybot + V.BbPlot.ytop) - key_height) / 2;
			pKey->bounds.ytop = ((V.BbPlot.ybot + V.BbPlot.ytop) + key_height) / 2;
		}
		if(pKey->hpos == LEFT) {
			pKey->bounds.xleft = V.BbPlot.xleft + pTerm->h_char;
			pKey->bounds.xright = pKey->bounds.xleft + key_width;
		}
		else if(pKey->hpos == RIGHT) {
			pKey->bounds.xright = V.BbPlot.xright - pTerm->h_char;
			pKey->bounds.xleft = pKey->bounds.xright - key_width;
		}
		else { // (key->hpos == CENTER) 
			pKey->bounds.xleft = ((V.BbPlot.xright + V.BbPlot.xleft) - key_width) / 2;
			pKey->bounds.xright = ((V.BbPlot.xright + V.BbPlot.xleft) + key_width) / 2;
		}
		// Key outside plot boundaries 
	}
	else if(pKey->region == GPKEY_AUTO_EXTERIOR_LRTBC || pKey->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
		// Vertical alignment 
		if(pKey->margin == GPKEY_TMARGIN) {
			// align top first since tmargin may be manual 
			pKey->bounds.ytop = (V.YSize + V.YOffset) * pTerm->ymax - pTerm->v_tic;
			pKey->bounds.ybot = pKey->bounds.ytop - key_height;
		}
		else if(pKey->margin == GPKEY_BMARGIN) {
			// align bottom first since bmargin may be manual 
			pKey->bounds.ybot = V.YOffset * pTerm->ymax + pTerm->v_tic;
			if(timelabel.rotate == 0 && timelabel_bottom && timelabel.place.y > 0)
				pKey->bounds.ybot += (int)(timelabel.place.y);
			pKey->bounds.ytop = pKey->bounds.ybot + key_height;
		}
		else {
			if(pKey->vpos == JUST_TOP) {
				// align top first since tmargin may be manual 
				pKey->bounds.ytop = V.BbPlot.ytop;
				pKey->bounds.ybot = pKey->bounds.ytop - key_height;
			}
			else if(pKey->vpos == JUST_CENTRE) {
				pKey->bounds.ybot = ((V.BbPlot.ybot + V.BbPlot.ytop) - key_height) / 2;
				pKey->bounds.ytop = ((V.BbPlot.ybot + V.BbPlot.ytop) + key_height) / 2;
			}
			else {
				// align bottom first since bmargin may be manual 
				pKey->bounds.ybot = V.BbPlot.ybot;
				pKey->bounds.ytop = pKey->bounds.ybot + key_height;
			}
		}
		// Horizontal alignment 
		if(pKey->margin == GPKEY_LMARGIN) {
			// align left first since lmargin may be manual 
			pKey->bounds.xleft = V.XOffset * pTerm->xmax + pTerm->h_char;
			pKey->bounds.xright = pKey->bounds.xleft + key_width;
		}
		else if(pKey->margin == GPKEY_RMARGIN) {
			// align right first since rmargin may be manual 
			pKey->bounds.xright = (V.XSize + V.XOffset) * (pTerm->xmax-1) - pTerm->h_char;
			pKey->bounds.xleft = pKey->bounds.xright - key_width;
		}
		else {
			if(pKey->hpos == LEFT) {
				// align left first since lmargin may be manual 
				pKey->bounds.xleft = V.BbPlot.xleft;
				pKey->bounds.xright = pKey->bounds.xleft + key_width;
			}
			else if(pKey->hpos == CENTRE) {
				pKey->bounds.xleft  = ((V.BbPlot.xright + V.BbPlot.xleft) - key_width) / 2;
				pKey->bounds.xright = ((V.BbPlot.xright + V.BbPlot.xleft) + key_width) / 2;
			}
			else {
				// align right first since rmargin may be manual 
				pKey->bounds.xright = V.BbPlot.xright;
				pKey->bounds.xleft = pKey->bounds.xright - key_width;
			}
		}
		// Key at explicit position specified by user 
	}
	else {
		int x, y;
		// FIXME!!!
		// pm 22.1.2002: if key->user_pos.scalex or scaley == first_axes or second_axes,
		// then the graph scaling is not yet known and the box is positioned incorrectly;
		// you must do "replot" to avoid the wrong plot ... bad luck if output does not
		// go to screen
		MapPosition(pTerm, &pKey->user_pos, &x, &y, "key");
		// Here top, bottom, left, right refer to the alignment with respect to point. 
		pKey->bounds.xleft = x;
		if(pKey->hpos == CENTRE)
			pKey->bounds.xleft -= key_width/2;
		else if(pKey->hpos == RIGHT)
			pKey->bounds.xleft -= key_width;
		pKey->bounds.xright = pKey->bounds.xleft + key_width;
		pKey->bounds.ytop = y;
		if(pKey->vpos == JUST_CENTRE)
			pKey->bounds.ytop += key_height/2;
		else if(pKey->vpos == JUST_BOT)
			pKey->bounds.ytop += key_height;
		pKey->bounds.ybot = pKey->bounds.ytop - key_height;
	}
}
//
// Calculate positioning of components that make up the key box 
//
//void do_key_layout(termentry * pTerm, legend_key * key)
void GnuPlot::DoKeyLayout(termentry * pTerm, legend_key * pKey)
{
	//struct termentry * t = term;
	bool key_panic = FALSE;
	// If there is a separate font for the key, use it for space calculations.	
	if(pKey->font)
		pTerm->set_font(pKey->font);
	// Is it OK to initialize these here rather than in do_plot? 
	key_count = 0;
	key_xleft = 0;
	xl = yl = 0;
	key_sample_width = (pKey->swidth >= 0) ? (pKey->swidth * pTerm->h_char + pTerm->h_tic) : 0;
	key_sample_height = MAX(1.25 * pTerm->v_tic, pTerm->v_char);
	key_entry_height = key_sample_height * pKey->vert_factor;
	// HBB 20020122: safeguard to prevent division by zero later 
	SETIFZ(key_entry_height, 1);
	// Key title length and height, adjusted for font size and markup 
	key_title_height = 0;
	key_title_extra = 0;
	key_title_ypos = 0;
	if(pKey->title.text) {
		double est_height;
		int est_lines;
		if(pKey->title.font)
			pTerm->set_font(pKey->title.font);
		label_width(pKey->title.text, &est_lines);
		estimate_strlen(pKey->title.text, &est_height);
		key_title_height = est_height * pTerm->v_char;
		key_title_ypos = (key_title_height/2);
		if(pKey->title.font)
			pTerm->set_font("");
		// FIXME: empirical tweak. I don't know why this is needed 
		key_title_ypos -= (est_lines-1) * pTerm->v_char/2;
	}
	if(pKey->reverse) {
		key_sample_left = -key_sample_width;
		key_sample_right = 0;
		// if key width is being used, adjust right-justified text 
		key_text_left  = pTerm->h_char;
		key_text_right = pTerm->h_char * (max_ptitl_len + 1 + pKey->width_fix);
		key_size_left  = pTerm->h_char - key_sample_left; // sample left is -ve 
		key_size_right = key_text_right;
	}
	else {
		key_sample_left = 0;
		key_sample_right = key_sample_width;
		// if key width is being used, adjust left-justified text 
		key_text_left = -(int)(pTerm->h_char * (max_ptitl_len + 1 + pKey->width_fix));
		key_text_right = -(int)pTerm->h_char;
		key_size_left = -key_text_left;
		key_size_right = key_sample_right + pTerm->h_char;
	}
	key_point_offset = (key_sample_left + key_sample_right) / 2;
	// advance width for cols 
	key_col_wth = key_size_left + key_size_right;
	key_rows = ptitl_cnt;
	key_cols = 1;
	// calculate rows and cols for key 
	if(pKey->stack_dir == GPKEY_HORIZONTAL) {
		// maximise no cols, limited by label-length 
		key_cols = (int)(V.BbPlot.xright - V.BbPlot.xleft) / key_col_wth;
		if(pKey->maxcols > 0 && key_cols > pKey->maxcols)
			key_cols = pKey->maxcols;
		// EAM Dec 2004 - Rather than turn off the key, try to squeeze 
		if(key_cols == 0) {
			key_cols = 1;
			key_panic = TRUE;
			key_col_wth = (V.BbPlot.xright - V.BbPlot.xleft);
		}
		key_rows = (ptitl_cnt + key_cols - 1) / key_cols;
		// now calculate actual no cols depending on no rows 
		key_cols = (key_rows == 0) ? 1 : (ptitl_cnt + key_rows - 1) / key_rows;
		SETIFZ(key_cols, 1);
	}
	else {
		// maximise no rows, limited by V.BbPlot.ytop-V.BbPlot.ybot 
		int i = (V.BbPlot.ytop - V.BbPlot.ybot - pKey->height_fix * key_entry_height - key_title_height - key_title_extra) / key_entry_height;
		if(pKey->maxrows > 0 && i > pKey->maxrows)
			i = pKey->maxrows;
		if(i == 0) {
			i = 1;
			key_panic = TRUE;
		}
		if(ptitl_cnt > i) {
			key_cols = (ptitl_cnt + i - 1) / i;
			// now calculate actual no rows depending on no cols 
			if(key_cols == 0) {
				key_cols = 1;
				key_panic = TRUE;
			}
			key_rows = (ptitl_cnt + key_cols - 1) / key_cols;
		}
	}
	// If the key title is wider than the contents, try to make room for it 
	if(pKey->title.text) {
		int ytlen = label_width(pKey->title.text, NULL) - pKey->swidth + 2;
		if(pKey->title.font)
			pTerm->set_font(pKey->title.font);
		ytlen *= pTerm->h_char;
		if(ytlen > key_cols * key_col_wth)
			key_col_wth = ytlen / key_cols;
		if(pKey->title.font)
			pTerm->set_font("");
	}
	// Adjust for outside key, leave manually set margins alone 
	if((pKey->region == GPKEY_AUTO_EXTERIOR_LRTBC && (pKey->vpos != JUST_CENTRE || pKey->hpos != CENTRE)) || pKey->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
		int more = 0;
		if(pKey->margin == GPKEY_BMARGIN && V.MarginB.x < 0) {
			more = key_rows * key_entry_height + key_title_height + key_title_extra + pKey->height_fix * key_entry_height;
			if(V.BbPlot.ybot + more > V.BbPlot.ytop)
				key_panic = TRUE;
			else
				V.BbPlot.ybot += more;
		}
		else if(pKey->margin == GPKEY_TMARGIN && V.MarginT.x < 0) {
			more = key_rows * key_entry_height + key_title_height + key_title_extra + pKey->height_fix * key_entry_height;
			if(V.BbPlot.ytop - more < V.BbPlot.ybot)
				key_panic = TRUE;
			else
				V.BbPlot.ytop -= more;
		}
		else if(pKey->margin == GPKEY_LMARGIN && V.MarginL.x < 0) {
			more = key_col_wth * key_cols;
			if(V.BbPlot.xleft + more > V.BbPlot.xright)
				key_panic = TRUE;
			else
				key_xleft = more;
			V.BbPlot.xleft += key_xleft;
		}
		else if(pKey->margin == GPKEY_RMARGIN && V.MarginR.x < 0) {
			more = key_col_wth * key_cols;
			if(V.BbPlot.xright - more < V.BbPlot.xleft)
				key_panic = TRUE;
			else
				V.BbPlot.xright -= more;
		}
	}
	// Restore default font 
	if(pKey->font)
		pTerm->set_font("");
	// warn if we had to punt on key size calculations 
	if(key_panic)
		IntWarn(NO_CARET, "Warning - difficulty fitting plot titles into key");
}

int find_maxl_keys(curve_points * plots, int count, int * kcnt)
{
	int mlen = 0;
	int len;
	int curve;
	int cnt = 0;
	int previous_plot_style = 0;
	curve_points * this_plot = plots;
	for(curve = 0; curve < count; this_plot = this_plot->next, curve++) {
		if(this_plot->plot_style == PARALLELPLOT)
			continue;
		if(this_plot->title && !this_plot->title_is_suppressed &&  !this_plot->title_position) {
			if(this_plot->plot_style == SPIDERPLOT && this_plot->plot_type != KEYENTRY)
				; /* Nothing */
			else {
				ignore_enhanced(this_plot->title_no_enhanced);
				len = estimate_strlen(this_plot->title, NULL);
				if(len != 0) {
					cnt++;
					if(len > mlen)
						mlen = len;
				}
				ignore_enhanced(FALSE);
			}
		}
		/* Check for new histogram here and save space for divider */
		if(this_plot->plot_style == HISTOGRAMS &&  previous_plot_style == HISTOGRAMS && this_plot->histogram_sequence == 0 && cnt > 1)
			cnt++;
		/* Check for column-stacked histogram with key entries.
		 * Same thing for spiderplots.
		 * This is needed for 'plot ... using col:key(1)'
		 */
		if(this_plot->labels &&
		    (this_plot->plot_style == HISTOGRAMS || this_plot->plot_style == SPIDERPLOT)) {
			text_label * key_entry = this_plot->labels->next;
			for(; key_entry; key_entry = key_entry->next) {
				cnt++;
				len = key_entry->text ? estimate_strlen(key_entry->text, NULL) : 0;
				if(len > mlen)
					mlen = len;
			}
		}
		previous_plot_style = this_plot->plot_style;
	}
	ASSIGN_PTR(kcnt, cnt);
	return (mlen);
}
// 
// Make the key sample code a subroutine so that it can eventually be
// shared by the 3d code also. As of now the two code sections are not very parallel.  EAM Nov 2003
//
void do_key_sample(termentry * pTerm, const curve_points * this_plot, legend_key * key, char * title, coordval var_color)
{
	//struct termentry * t = term;
	int xl_save = xl;
	int yl_save = yl;
	// Clip key box against canvas 
	BoundingBox * clip_save = GPO.V.P_ClipArea;
	GPO.V.P_ClipArea = (term->flags & TERM_CAN_CLIP) ? NULL : &GPO.V.BbCanvas;
	// If the plot this title belongs to specified a non-standard place 
	// for the key sample to appear, use that to override xl, yl.       
	if(this_plot->title_position && this_plot->title_position->scalex != character) {
		GPO.MapPosition(pTerm, this_plot->title_position, &xl, &yl, "key sample");
		xl -=  (key->just == GPKEY_LEFT) ? key_text_left : key_text_right;
	}
	(pTerm->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(key->textcolor.type == TC_VARIABLE)
		; /* Draw key text in same color as plot */
	else if(key->textcolor.type != TC_DEFAULT)
		apply_pm3dcolor(pTerm, &key->textcolor); /* Draw key text in same color as key title */
	else
		(pTerm->linetype)(LT_BLACK); /* Draw key text in black */
	if(this_plot->title_is_automated && (pTerm->flags & TERM_IS_LATEX)) {
		title = texify_title(title, this_plot->plot_type);
	}
	if(key->just == GPKEY_LEFT) {
		write_multiline(xl + key_text_left, yl, title, LEFT, JUST_CENTRE, 0, key->font);
	}
	else {
		if((pTerm->justify_text)(RIGHT)) {
			write_multiline(xl + key_text_right, yl, title, RIGHT, JUST_CENTRE, 0, key->font);
		}
		else {
			int x = xl + key_text_right - pTerm->h_char * estimate_strlen(title, NULL);
			if(oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) || inrange((x), (GPO.V.BbPlot.xleft), (GPO.V.BbPlot.xright)))
				write_multiline(x, yl, title, LEFT, JUST_CENTRE, 0, key->font);
		}
	}
	// Draw sample in same style and color as the corresponding plot  
	// The variable color case uses the color of the first data point 
	if(!check_for_variable_color(this_plot, &var_color))
		term_apply_lp_properties(pTerm, &this_plot->lp_properties);
	// draw sample depending on bits set in plot_style 
	if(this_plot->plot_style & PLOT_STYLE_HAS_FILL && pTerm->fillbox) {
		const fill_style_type * fs = &this_plot->fill_properties;
		int style = style_from_fill(fs);
		int x = xl + key_sample_left;
		int y = yl - key_sample_height/4;
		int w = key_sample_right - key_sample_left;
		int h = key_sample_height/2;
		if(this_plot->plot_style == CIRCLES && w > 0) {
			do_arc(xl + key_point_offset, yl, key_sample_height/4, 0., 360., style, FALSE);
			// Retrace the border if the style requests it 
			if(need_fill_border(fs))
				do_arc(xl + key_point_offset, yl, key_sample_height/4, 0., 360., 0, FALSE);
		}
		else if(this_plot->plot_style == ELLIPSES && w > 0) {
			t_ellipse * key_ellipse = (t_ellipse*)gp_alloc(sizeof(t_ellipse), "cute little ellipse for the key sample");
			key_ellipse->center.x = xl + key_point_offset;
			key_ellipse->center.y = yl;
			key_ellipse->extent.x = w * 2/3;
			key_ellipse->extent.y = h;
			key_ellipse->orientation = 0.0;
			// already in term coords, no need to map 
			do_ellipse(2, key_ellipse, style, FALSE);
			// Retrace the border if the style requests it 
			if(need_fill_border(fs)) {
				do_ellipse(2, key_ellipse, 0, FALSE);
			}
			SAlloc::F(key_ellipse);
		}
		else if(w > 0) { /* All other plot types with fill */
			if(style != FS_EMPTY)
				(pTerm->fillbox)(style, x, y, w, h);
			// need_fill_border will set the border linetype, but candlesticks don't want it 
			if((this_plot->plot_style == CANDLESTICKS && fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW) || style == FS_EMPTY || need_fill_border(fs)) {
				newpath(pTerm);
				draw_clip_line(pTerm, xl + key_sample_left,  yl - key_sample_height/4, xl + key_sample_right, yl - key_sample_height/4);
				draw_clip_line(pTerm, xl + key_sample_right, yl - key_sample_height/4, xl + key_sample_right, yl + key_sample_height/4);
				draw_clip_line(pTerm, xl + key_sample_right, yl + key_sample_height/4, xl + key_sample_left,  yl + key_sample_height/4);
				draw_clip_line(pTerm, xl + key_sample_left,  yl + key_sample_height/4, xl + key_sample_left,  yl - key_sample_height/4);
				closepath(pTerm);
			}
			if(fs->fillstyle != FS_EMPTY && fs->fillstyle != FS_DEFAULT && !(fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW)) {
				// need_fill_border() might have changed our original linetype 
				term_apply_lp_properties(pTerm, &this_plot->lp_properties);
			}
		}
	}
	else if((this_plot->plot_style & PLOT_STYLE_HAS_VECTOR) && pTerm->arrow) {
		double x1 = xl + key_sample_left;
		double y1 = yl;
		double x2 = xl + key_sample_right;
		double y2 = yl;
		apply_head_properties(&(this_plot->arrow_properties));
		draw_clip_arrow(pTerm, x1, y1, x2, y2, this_plot->arrow_properties.head);
	}
	else if(this_plot->lp_properties.l_type == LT_NODRAW) {
		;
	}
	else if((this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR) && this_plot->plot_type != FUNC) {
		// errors for data plots only 
		if((bar_lp.flags & LP_ERRORBAR_SET) != 0)
			term_apply_lp_properties(pTerm, &bar_lp);
		draw_clip_line(pTerm, xl + key_sample_left, yl, xl + key_sample_right, yl);
		// Even if error bars are dotted, the end lines are always solid 
		if((bar_lp.flags & LP_ERRORBAR_SET) != 0)
			pTerm->dashtype(DASHTYPE_SOLID, NULL);
	}
	else if((this_plot->plot_style & PLOT_STYLE_HAS_LINE)) {
		draw_clip_line(pTerm, xl + key_sample_left, yl, xl + key_sample_right, yl);
	}
	if((this_plot->plot_type == DATA || this_plot->plot_type == KEYENTRY) && (this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR) && (this_plot->plot_style != CANDLESTICKS) && (bar_size > 0.0)) {
		const uint error_bar_tic = ERRORBARTIC(pTerm);
		draw_clip_line(pTerm, xl + key_sample_left, yl + error_bar_tic, xl + key_sample_left, yl - error_bar_tic);
		draw_clip_line(pTerm, xl + key_sample_right, yl + error_bar_tic, xl + key_sample_right, yl - error_bar_tic);
	}
	/* oops - doing the point sample now would break the postscript
	 * terminal for example, which changes current line style
	 * when drawing a point, but does not restore it. We must wait to
	 * draw the point sample at the end of do_plot (comment KEY SAMPLES).
	 */
	(pTerm->layer)(TERM_LAYER_END_KEYSAMPLE);
	// Restore original linetype for the main plot if we changed it 
	if(this_plot->plot_type != FUNC && (this_plot->plot_style & PLOT_STYLE_HAS_ERRORBAR) && (bar_lp.flags & LP_ERRORBAR_SET) != 0) {
		term_apply_lp_properties(pTerm, &this_plot->lp_properties);
	}
	// Restore previous clipping area 
	GPO.V.P_ClipArea = clip_save;
	xl = xl_save;
	yl = yl_save;
}

void do_key_sample_point(curve_points * this_plot, legend_key * key)
{
	struct termentry * t = term;
	int xl_save = xl;
	int yl_save = yl;
	/* If the plot this title belongs to specified a non-standard place
	 * for the key sample to appear, use that to override xl, yl.
	 * For "at end|beg" do nothing at all.
	 */
	if(this_plot->title_position) {
		if(this_plot->title_position->scalex == character)
			return;
		GPO.MapPosition(t, this_plot->title_position, &xl, &yl, "key sample");
		xl -=  (key->just == GPKEY_LEFT) ? key_text_left : key_text_right;
	}
	(t->layer)(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(this_plot->plot_style == LINESPOINTS &&  this_plot->lp_properties.p_interval < 0) {
		t_colorspec background_fill = BACKGROUND_COLORSPEC;
		(*t->set_color)(&background_fill);
		(*t->pointsize)(pointsize * pointintervalbox);
		(*t->point)(xl + key_point_offset, yl, 6);
		term_apply_lp_properties(t, &this_plot->lp_properties);
	}
	if(this_plot->plot_style == BOXPLOT) {
		; /* Don't draw a sample point in the key */
	}
	else if(this_plot->plot_style == DOTS) {
		if(on_page(xl + key_point_offset, yl))
			(*t->point)(xl + key_point_offset, yl, -1);
	}
	else if(this_plot->plot_style & PLOT_STYLE_HAS_POINT) {
		if(this_plot->lp_properties.p_size == PTSZ_VARIABLE)
			(*t->pointsize)(pointsize);
		if(on_page(xl + key_point_offset, yl)) {
			if(this_plot->lp_properties.p_type == PT_CHARACTER) {
				if(this_plot->labels->textcolor.type != TC_DEFAULT)
					apply_pm3dcolor(t, &(this_plot->labels->textcolor));
				(*t->put_text)(xl + key_point_offset, yl, this_plot->lp_properties.p_char);
				apply_pm3dcolor(t, &(this_plot->lp_properties.pm3d_color));
			}
			else {
				(*t->point)(xl + key_point_offset, yl, this_plot->lp_properties.p_type);
			}
		}
	}
	else if(this_plot->plot_style == LABELPOINTS) {
		text_label * label = this_plot->labels;
		if(label->lp_properties.flags & LP_SHOW_POINTS) {
			term_apply_lp_properties(t, &label->lp_properties);
			(*t->point)(xl + key_point_offset, yl, label->lp_properties.p_type);
		}
	}
	xl = xl_save;
	yl = yl_save;
	(t->layer)(TERM_LAYER_END_KEYSAMPLE);
}

/* Graph legend is now optionally done in two passes. The first pass calculates	*/
/* and reserves the necessary space.  Next the individual plots in the graph    */
/* are drawn. Then the reserved space for the legend is blanked out, and        */
/* finally the second pass through this code draws the legend.			*/
void draw_key(legend_key * key, bool key_pass)
{
	struct termentry * t = term;
	(t->layer)(TERM_LAYER_KEYBOX);
	/* In two-pass mode (set key opaque) we blank out the key box after	*/
	/* the graph is drawn and then redo the key in the blank area.	*/
	if(key_pass && t->fillbox && !(t->flags & TERM_NULL_SET_COLOR)) {
		(*t->set_color)(&key->fillcolor);
		(*t->fillbox)(FS_OPAQUE, key->bounds.xleft, key->bounds.ybot, key_width, key_height);
	}
	if(key->title.text) {
		int title_anchor;
		if(key->title.pos == CENTRE)
			title_anchor = (key->bounds.xleft + key->bounds.xright) / 2;
		else if(key->title.pos == RIGHT)
			title_anchor = key->bounds.xright - t->h_char;
		else
			title_anchor = key->bounds.xleft + t->h_char;
		// Only draw the title once 
		if(key_pass || !key->front) {
			write_label(t, title_anchor, key->bounds.ytop - key_title_ypos, &key->title);
			(*t->linetype)(LT_BLACK);
		}
	}
	if(key->box.l_type > LT_NODRAW) {
		BoundingBox * clip_save = GPO.V.P_ClipArea;
		GPO.V.P_ClipArea = (t->flags & TERM_CAN_CLIP) ? NULL : &GPO.V.BbCanvas;
		term_apply_lp_properties(t, &key->box);
		newpath(t);
		draw_clip_line(t, key->bounds.xleft, key->bounds.ybot, key->bounds.xleft, key->bounds.ytop);
		draw_clip_line(t, key->bounds.xleft, key->bounds.ytop, key->bounds.xright, key->bounds.ytop);
		draw_clip_line(t, key->bounds.xright, key->bounds.ytop, key->bounds.xright, key->bounds.ybot);
		draw_clip_line(t, key->bounds.xright, key->bounds.ybot, key->bounds.xleft, key->bounds.ybot);
		closepath(t);
		// draw a horizontal line between key title and first entry 
		if(key->title.text)
			draw_clip_line(t, key->bounds.xleft, key->bounds.ytop - (key_title_height + key_title_extra), key->bounds.xright, key->bounds.ytop - (key_title_height + key_title_extra));
		GPO.V.P_ClipArea = clip_save;
	}
	yl_ref = key->bounds.ytop - (key_title_height + key_title_extra);
	yl_ref -= ((key->height_fix + 1) * key_entry_height) / 2;
	xl = key->bounds.xleft + key_size_left;
	yl = yl_ref;
}
// 
// This routine draws the plot title, the axis labels, and an optional time stamp.
// 
//void draw_titles()
void GnuPlot::DrawTitles(termentry * pTerm)
{
	//struct termentry * t = term;
	// YLABEL 
	if(AxS[FIRST_Y_AXIS].label.text) {
		int x = ylabel_x;
		int y = (V.BbPlot.ytop + V.BbPlot.ybot) / 2;
		// There has been much argument about the optimal ylabel position 
		x += pTerm->h_char / 4.0;
		write_label(pTerm, x, y, &(AxS[FIRST_Y_AXIS].label));
		reset_textcolor(&(AxS[FIRST_Y_AXIS].label.textcolor));
	}
	// Y2LABEL 
	if(AxS[SECOND_Y_AXIS].label.text) {
		int x = y2label_x;
		int y = (V.BbPlot.ytop + V.BbPlot.ybot) / 2;
		write_label(pTerm, x, y, &(AxS[SECOND_Y_AXIS].label));
		reset_textcolor(&(AxS[SECOND_Y_AXIS].label.textcolor));
	}
	// XLABEL 
	if(AxS[FIRST_X_AXIS].label.text) {
		text_label * label = &AxS[FIRST_X_AXIS].label;
		double tmpx, tmpy;
		MapPositionR(pTerm, &(label->offset), &tmpx, &tmpy, "xlabel");
		int x = (V.BbPlot.xright + V.BbPlot.xleft) / 2;
		int y = xlabel_y - pTerm->v_char / 2;
		y -= tmpy; // xlabel_y already contained tmpy 
		write_label(pTerm, x, y, label);
		reset_textcolor(&(label->textcolor));
	}
	// X2LABEL 
	if(AxS[SECOND_X_AXIS].label.text) {
		// we worked out y-coordinate in boundary() 
		int x = (V.BbPlot.xright + V.BbPlot.xleft) / 2;
		int y = x2label_y - pTerm->v_char / 2;
		write_label(pTerm, x, y, &(AxS[SECOND_X_AXIS].label));
		reset_textcolor(&(AxS[SECOND_X_AXIS].label.textcolor));
	}
	// RLABEL 
	if(AxS[POLAR_AXIS].label.text) {
		// This assumes we always have a horizontal R axis 
		int x = AxS.MapiX(PolarRadius(AxS.__R().max) / 2.0);
		int y = AxS.MapiY(0.0) + pTerm->v_char;
		write_label(pTerm, x, y, &(AxS[POLAR_AXIS].label));
		reset_textcolor(&(AxS[POLAR_AXIS].label.textcolor));
	}
	// PLACE TIMELABEL 
	if(timelabel.text)
		do_timelabel(time_x, time_y);
}
//
// advance current position in the key in preparation for next key entry 
//
void advance_key(bool only_invert)
{
	legend_key * key = &keyT;
	if(key->invert)
		yl = key->bounds.ybot + yl_ref + key_entry_height/2 - yl;
	if(only_invert)
		return;
	if(key_count >= key_rows) {
		yl = yl_ref;
		xl += key_col_wth;
		key_count = 0;
	}
	else
		yl = yl - key_entry_height;
}

/* stupid test used in only one place but it refers to our local variables */
bool at_left_of_key()
{
	return (yl == yl_ref);
}
