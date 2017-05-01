/*
 * $Id: boundary.c,v 1.31 2016/03/09 04:40:01 sfeam Exp $
 */

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

#define ERRORBARTIC MAX((t->HTic/2), 1)
//
// These quantities are needed in do_plot() e.g. for histogtram GpGg.title layout
//
//GpBoundary GpB; // @global

/*{{{  boundary() */
/* borders of plotting area
 * computed once on every call to do_plot
 *
 * The order in which things is done is getting pretty critical:
 *  GpGg.PlotBounds.ytop depends on GpGg.title, x2label, ylabels (if no rotated text)
 *  GpGg.PlotBounds.ybot depends on key, if "under"
 *  once we have these, we can setup the y1 and y2 tics and the
 *  only then can we calculate GpGg.PlotBounds.xleft and GpGg.PlotBounds.xright
 *  GpGg.PlotBounds.xright depends also on key RIGHT
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

void GpBoundary::Boundary(GpTermEntry * pT, GpGadgets & rGg, const CurvePoints * pPlots, int count)
{
	int    yticlin = 0, y2ticlin = 0, timelin = 0;
	legend_key * key = &rGg.keyT;
	/* FIXME HBB 20000506: this line is the reason for the 'D0,1;D1,0'
	 * bug in the HPGL terminal: we actually carry out the switch of
	 * text orientation, just for finding out if the terminal can do
	 * that. *But* we're not in graphical mode, yet, so this call
	 * yields undesirable results */
	int can_rotate = (*pT->text_angle)(TEXT_VERTICAL);
	int xtic_textheight;    // height of xtic labels
	int x2tic_textheight;   // height of x2tic labels
	int title_textheight;   // height of rGg.title
	int xlabel_textheight;  // height of xlabel
	int x2label_textheight; // height of x2label
	int timetop_textheight; // height of timestamp (if at top)
	int timebot_textheight; // height of timestamp (if at bottom)
	int ylabel_textheight;  // height of (unrotated) ylabel
	int y2label_textheight; // height of (unrotated) y2label
	int ylabel_textwidth;   // width of (rotated) ylabel
	int y2label_textwidth;  // width of (rotated) y2label
	int timelabel_textwidth; // width of timestamp
	int ytic_textwidth;     // width of ytic labels
	int y2tic_textwidth;    // width of y2tic labels
	int x2tic_height;       // 0 for tic_in or no x2tics, ticscale*VTic otherwise
	int xtic_textwidth = 0; // amount by which the xtic label protrude to the right
	int xtic_height;
	int ytic_width;
	int y2tic_width;
	//
	// figure out which rotatable items are to be rotated (ylabel and y2label are rotated if possible)
	//
	int vertical_timelabel = can_rotate ? rGg.timelabel_rotate : 0;
	int vertical_xtics  = can_rotate ? rGg[FIRST_X_AXIS].tic_rotate : 0;
	int vertical_x2tics = can_rotate ? rGg[SECOND_X_AXIS].tic_rotate : 0;
	int vertical_ytics  = can_rotate ? rGg[FIRST_Y_AXIS].tic_rotate : 0;
	int vertical_y2tics = can_rotate ? rGg[SECOND_Y_AXIS].tic_rotate : 0;
	bool shift_labels_to_border = false;
	xticlin = 0;
	ylablin = 0;
	y2lablin = 0;
	xlablin = 0;
	x2lablin = 0;
	titlelin = 0;
	/*{{{  count lines in labels and tics */
	label_width(rGg.title.text, &titlelin);
	label_width(rGg[FIRST_X_AXIS].label.text, &xlablin);
	//
	// This should go *inside* label_width(), but it messes up the key rGg.title
	// Imperfect check for subscripts or superscripts
	if((term->flags & TERM_ENHANCED_TEXT) && rGg[FIRST_X_AXIS].label.text && strpbrk(rGg[FIRST_X_AXIS].label.text, "_^"))
		xlablin++;
	label_width(rGg[SECOND_X_AXIS].label.text, &x2lablin);
	label_width(rGg[FIRST_Y_AXIS].label.text, &ylablin);
	label_width(rGg[SECOND_Y_AXIS].label.text, &y2lablin);
	if(rGg[FIRST_X_AXIS].ticmode) {
		label_width(rGg[FIRST_X_AXIS].formatstring, &xticlin);
		// Reserve room for user tic labels even if format of autoticks is ""
		if(xticlin == 0 && rGg[FIRST_X_AXIS].ticdef.def.user)
			xticlin = 1;
	}
	if(rGg[SECOND_X_AXIS].ticmode)
		label_width(rGg[SECOND_X_AXIS].formatstring, &x2ticlin);
	if(rGg[FIRST_Y_AXIS].ticmode)
		label_width(rGg[FIRST_Y_AXIS].formatstring, &yticlin);
	if(rGg[SECOND_Y_AXIS].ticmode)
		label_width(rGg[SECOND_Y_AXIS].formatstring, &y2ticlin);
	if(rGg.timelabel.text)
		label_width(rGg.timelabel.text, &timelin);
	/*}}} */
	/*{{{  preliminary rGg.PlotBounds.ytop  calculation */
	/*     first compute heights of things to be written in the margin */
	/* rGg.title */
	if(titlelin) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg.title.offset, &tmpx, &tmpy, "boundary");
		if(rGg.title.font)
			pT->set_font(rGg.title.font);
		title_textheight = (int)((titlelin) * (pT->VChr) + tmpy);
		if(rGg.title.font)
			pT->set_font("");
		title_textheight += (int)(pT->VChr); /* Gap of one normal line height */
	}
	else
		title_textheight = 0;
	// x2label 
	if(x2lablin) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[SECOND_X_AXIS].label.offset, &tmpx, &tmpy, "boundary");
		if(rGg[SECOND_X_AXIS].label.font)
			pT->set_font(rGg[SECOND_X_AXIS].label.font);
		x2label_textheight = (int)(x2lablin * pT->VChr + tmpy);
		if(!rGg[SECOND_X_AXIS].ticmode)
			x2label_textheight += (int)(0.5 * pT->VChr);
		if(rGg[SECOND_X_AXIS].label.font)
			pT->set_font("");
	}
	else
		x2label_textheight = 0;
	//
	// tic labels
	//
	// ought to consider tics on axes if axis near border
	x2tic_textheight = (rGg[SECOND_X_AXIS].ticmode & TICS_ON_BORDER) ? (int)(x2ticlin * pT->VChr) : 0;
	//
	// tics
	//
	if(!(rGg[SECOND_X_AXIS].Flags & GpAxis::fTicIn) && ((rGg[SECOND_X_AXIS].ticmode & TICS_ON_BORDER) || ((rGg[FIRST_X_AXIS].ticmode & TICS_MIRROR) && (rGg[FIRST_X_AXIS].ticmode & TICS_ON_BORDER))))
		x2tic_height = (int)(pT->VTic * rGg[SECOND_X_AXIS].ticscale);
	else
		x2tic_height = 0;
	// timestamp
	if(rGg.timelabel.text && !rGg.timelabel_bottom) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg.timelabel.offset, &tmpx, &tmpy, "boundary");
		timetop_textheight = (int)((timelin + 2) * pT->VChr + tmpy);
	}
	else
		timetop_textheight = 0;
	// horizontal ylabel 
	if(rGg[FIRST_Y_AXIS].label.text && !can_rotate) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[FIRST_Y_AXIS].label.offset, &tmpx, &tmpy, "boundary");
		if(rGg[FIRST_Y_AXIS].label.font)
			pT->set_font(rGg[FIRST_Y_AXIS].label.font);
		ylabel_textheight = (int)(ylablin * pT->VChr + tmpy);
		if(rGg[FIRST_Y_AXIS].label.font)
			pT->set_font("");
	}
	else
		ylabel_textheight = 0;
	// horizontal y2label 
	if(rGg[SECOND_Y_AXIS].label.text && !can_rotate) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[SECOND_Y_AXIS].label.offset, &tmpx, &tmpy, "boundary");
		if(rGg[SECOND_Y_AXIS].label.font)
			pT->set_font(rGg[FIRST_Y_AXIS].label.font);
		y2label_textheight = (int)(y2lablin * pT->VChr + tmpy);
		if(rGg[SECOND_Y_AXIS].label.font)
			pT->set_font("");
	}
	else
		y2label_textheight = 0;
	// compute rGg.PlotBounds.ytop from the various components unless rGg.TMrg is explicitly specified
	rGg.PlotBounds.ytop = (int)(0.5 + (rGg.YSz + rGg.YOffs) * (pT->ymax-1));
	/* Sanity check top and bottom margins, in case the user got confused */
	if(rGg.BMrg.scalex == screen && rGg.TMrg.scalex == screen)
		if(rGg.BMrg.x > rGg.TMrg.x) {
			Exchange(&rGg.BMrg.x, &rGg.TMrg.x);
		}
	if(rGg.TMrg.scalex == screen) {
		rGg.PlotBounds.ytop = (int)(rGg.TMrg.x * (float)(pT->ymax-1)); /* Specified as absolute position on the rGg.Canvas */
	}
	else if(rGg.TMrg.x >=0) {
		rGg.PlotBounds.ytop -= (int)(rGg.TMrg.x * (float)pT->VChr + 0.5); /* Specified in terms of character height */
	}
	else {
		/* Auto-calculation of space required */
		int top_margin = x2label_textheight + title_textheight;
		if(timetop_textheight + ylabel_textheight > top_margin)
			top_margin = timetop_textheight + ylabel_textheight;
		if(y2label_textheight > top_margin)
			top_margin = y2label_textheight;
		top_margin += x2tic_height + x2tic_textheight;
		/* x2tic_height and x2tic_textheight are computed as only the
		 *     relevant heights, but they nonetheless need a blank
		 *     space above them  */
		if(top_margin > x2tic_height)
			top_margin += (int)pT->VChr;
		rGg.PlotBounds.ytop -= top_margin;
		if(rGg.PlotBounds.ytop == (int)(0.5 + (rGg.YSz + rGg.YOffs) * (pT->ymax-1))) {
			rGg.PlotBounds.ytop -= (int)(pT->HChr * 2); // make room for the end of rotated ytics or y2tics
		}
	}

	/*  end of preliminary rGg.PlotBounds.ytop calculation }}} */

	/*{{{  preliminary rGg.PlotBounds.xleft, needed for "under" */
	rGg.PlotBounds.xleft = (int)((rGg.LMrg.scalex == screen) ? (rGg.LMrg.x * (float)pT->xmax) : (rGg.XOffs * pT->xmax + pT->HChr * (rGg.LMrg.x >= 0 ? rGg.LMrg.x : 1)));
	/*}}} */
	/*{{{  tentative rGg.PlotBounds.xright, needed for "under" */
	if(rGg.RMrg.scalex == screen)
		rGg.PlotBounds.xright = (int)(rGg.RMrg.x * (float)(pT->xmax - 1));
	else
		rGg.PlotBounds.xright = (int)((rGg.XSz + rGg.XOffs) * (pT->xmax - 1) - pT->HChr * (rGg.RMrg.x >= 0 ? rGg.RMrg.x : 2));
	/*}}} */

	//{{{  preliminary rGg.PlotBounds.ybot calculation first compute heights of labels and tics
	//
	// tic labels
	//
	shift_labels_to_border = false;
	if(rGg[FIRST_X_AXIS].ticmode & TICS_ON_AXIS) {
		// FIXME: This test for how close the axis is to the border does not match
		//        the tests in rGg.AxisOutputTics(), and assumes FIRST_Y_AXIS.
		if(!rGg[FIRST_Y_AXIS].InRange(0.0))
			shift_labels_to_border = true;
		if(0.05 > fabs(rGg[FIRST_Y_AXIS].Range.low / rGg[FIRST_Y_AXIS].GetRange()))
			shift_labels_to_border = true;
	}
	xtic_textheight = ((rGg[FIRST_X_AXIS].ticmode & TICS_ON_BORDER) || shift_labels_to_border) ? (int)(pT->VChr * (xticlin + 1)) : 0;
	//
	// tics
	//
	if(!(rGg[FIRST_X_AXIS].Flags & GpAxis::fTicIn) && ((rGg[FIRST_X_AXIS].ticmode & TICS_ON_BORDER)
		|| ((rGg[SECOND_X_AXIS].ticmode & TICS_MIRROR) && (rGg[SECOND_X_AXIS].ticmode & TICS_ON_BORDER))))
		xtic_height = (int)(pT->VTic * rGg[FIRST_X_AXIS].ticscale);
	else
		xtic_height = 0;
	//
	// xlabel
	//
	if(xlablin) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[FIRST_X_AXIS].label.offset, &tmpx, &tmpy, "boundary");
		// offset is subtracted because if > 0, the margin is smaller
		// textheight is inflated by 0.2 to allow descenders to clear bottom of rGg.Canvas
		xlabel_textheight = (int)((((float)xlablin + 0.2) * pT->VChr - tmpy));
		if(!rGg[FIRST_X_AXIS].ticmode)
			xlabel_textheight += (int)(0.5 * pT->VChr);
	}
	else
		xlabel_textheight = 0;
	// timestamp
	if(rGg.timelabel.text && rGg.timelabel_bottom) {
		/* && !vertical_timelabel)
		 * DBT 11-18-98 resize plot for vertical timelabels too !
		 */
		double tmpx, tmpy;
		rGg.MapPositionR(rGg.timelabel.offset, &tmpx, &tmpy, "boundary");
		/* offset is subtracted because if . 0, the margin is smaller */
		timebot_textheight = (int)(timelin * pT->VChr - tmpy);
	}
	else
		timebot_textheight = 0;
	// compute rGg.PlotBounds.ybot from the various components unless rGg.BMrg is explicitly specified
	rGg.PlotBounds.ybot = (int)(rGg.YOffs * (float)pT->ymax);
	if(rGg.BMrg.scalex == screen) {
		rGg.PlotBounds.ybot = (int)(rGg.BMrg.x * (float)pT->ymax); // Absolute position for bottom of plot
	}
	else if(rGg.BMrg.x >= 0) {
		rGg.PlotBounds.ybot += (int)(rGg.BMrg.x * (float)pT->VChr + 0.5); // Position based on specified character height
	}
	else {
		rGg.PlotBounds.ybot += xtic_height + xtic_textheight;
		if(xlabel_textheight > 0)
			rGg.PlotBounds.ybot += xlabel_textheight;
		if(timebot_textheight > 0)
			rGg.PlotBounds.ybot += timebot_textheight;
		// HBB 19990616: round to nearest integer, required to escape floating point inaccuracies
		if(rGg.PlotBounds.ybot == (int)(pT->ymax * rGg.YOffs)) {
			// make room for the end of rotated ytics or y2tics
			rGg.PlotBounds.ybot += (int)(pT->HChr * 2);
		}
	}
	/*  end of preliminary rGg.PlotBounds.ybot calculation }}} */
	// Determine the size and position of the key box
	if(key->visible) {
		// Count max_len key and number keys with len > 0
		max_ptitl_len = find_maxl_keys(pPlots, count, &ptitl_cnt);
		DoKeyLayout(pT, rGg, key);
	}
	/*{{{  set up y and y2 tics */
	rGg[FIRST_Y_AXIS].SetupTics(20);
	rGg[SECOND_Y_AXIS].SetupTics(20);
	/*}}} */
	// Adjust color axis limits if necessary.
	if(is_plot_with_palette()) {
		// June 2014 - moved outside do_plot so that it is not called during a refresh set_cbminmax();
		rGg.AxisCheckedExtendEmptyRange(COLOR_AXIS, "All points of color axis undefined.");
		if(rGg.ColorBox.where != SMCOLOR_BOX_NO)
			rGg[COLOR_AXIS].SetupTics(20);
	}

	/*{{{  recompute rGg.PlotBounds.xleft based on widths of ytics, ylabel etc
	   unless it has been explicitly set by rGg.LMrg */

	/* tic labels */
	shift_labels_to_border = false;
	if(rGg[FIRST_Y_AXIS].ticmode & TICS_ON_AXIS) {
		// FIXME: This test for how close the axis is to the border does not match 
		//        the tests in rGg.AxisOutputTics(), and assumes FIRST_X_AXIS.       
		if(!rGg[FIRST_X_AXIS].InRange(0.0))
			shift_labels_to_border = true;
		if(0.1 > fabs(rGg[FIRST_X_AXIS].Range.low / rGg[FIRST_X_AXIS].GetRange()))
			shift_labels_to_border = true;
	}
	if((rGg[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER) || shift_labels_to_border) {
		if(vertical_ytics)
			/* HBB: we will later add some white space as part of this, so
			 * reserve two more rows (one above, one below the text ...).
			 * Same will be done to similar calc.'s elsewhere */
			ytic_textwidth = (int)(pT->VChr * (yticlin + 2));
		else {
			rGg.widest_tic_strlen = 0; /* reset the global variable ... */
			//
			// get gen_tics to call widest_tic_callback with all labels
			// the latter sets widest_tic_strlen to the length of the widest
			// one ought to consider tics on axis if axis near border...
			//
			rGg.GenTics(pT, rGg[FIRST_Y_AXIS], &GpGadgets::WidestTicCallback);
			ytic_textwidth = (int)(pT->HChr * (rGg.widest_tic_strlen + 2));
		}
	}
	else {
		ytic_textwidth = 0;
	}
	/* tics */
	if(!(rGg[FIRST_Y_AXIS].Flags & GpAxis::fTicIn) && ((rGg[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER) || ((rGg[SECOND_Y_AXIS].ticmode & TICS_MIRROR) && (rGg[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER))))
		ytic_width = (int)(pT->HTic * rGg[FIRST_Y_AXIS].ticscale);
	else
		ytic_width = 0;
	/* ylabel */
	if(rGg[FIRST_Y_AXIS].label.text && can_rotate) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[FIRST_Y_AXIS].label.offset, &tmpx, &tmpy, "boundary");
		ylabel_textwidth = (int)(ylablin * (pT->VChr) - tmpx);
		if(!rGg[FIRST_Y_AXIS].ticmode)
			ylabel_textwidth += (int)(0.5 * pT->VChr);
	}
	else
		ylabel_textwidth = 0; // this should get large for NEGATIVE ylabel.xoffsets  DBT 11-5-98
	/* timestamp */
	if(rGg.timelabel.text && vertical_timelabel) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg.timelabel.offset, &tmpx, &tmpy, "boundary");
		timelabel_textwidth = (int)((timelin + 1.5) * pT->VChr - tmpx);
	}
	else
		timelabel_textwidth = 0;
	if(rGg.LMrg.x < 0) {
		/* Auto-calculation */
		double tmpx, tmpy;
		int space_to_left = key_xleft;
		SETMAX(space_to_left, timelabel_textwidth);
		SETMAX(space_to_left, ylabel_textwidth);
		rGg.PlotBounds.xleft = (int)(rGg.XOffs * pT->xmax) + space_to_left + ytic_width + ytic_textwidth;
		// make sure rGg.PlotBounds.xleft is wide enough for a negatively
		// x-offset horizontal timestamp
		rGg.MapPositionR(rGg.timelabel.offset, &tmpx, &tmpy, "boundary");
		if(!vertical_timelabel && rGg.PlotBounds.xleft - ytic_width - ytic_textwidth < -(int)(tmpx))
			rGg.PlotBounds.xleft = ytic_width + ytic_textwidth - (int)(tmpx);
		if(rGg.PlotBounds.xleft == (int)(pT->xmax * rGg.XOffs)) {
			/* make room for end of xtic or x2tic label */
			rGg.PlotBounds.xleft += (int)(pT->HChr * 2);
		}
		/* DBT 12-3-98  extra margin just in case */
		rGg.PlotBounds.xleft += (int)(0.5 * pT->HChr);
	}
	/* Note: we took care of explicit 'set rGg.LMrg foo' at line 492 */

	/*  end of rGg.PlotBounds.xleft calculation }}} */

	/*{{{  recompute rGg.PlotBounds.xright based on widest y2tic. y2labels, key "outside"
	   unless it has been explicitly set by rGg.RMrg */

	/* tic labels */
	if(rGg[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER) {
		if(vertical_y2tics)
			y2tic_textwidth = (int)(pT->VChr * (y2ticlin + 2));
		else {
			rGg.widest_tic_strlen = 0; // reset the global variable ...
			//
			// get gen_tics to call widest_tic_callback with all labels
			// the latter sets widest_tic_strlen to the length of the widest
			// one ought to consider tics on axis if axis near border...
			//
			rGg.GenTics(pT, rGg[SECOND_Y_AXIS], &GpGadgets::WidestTicCallback);
			y2tic_textwidth = (int)(pT->HChr * (rGg.widest_tic_strlen + 2));
		}
	}
	else {
		y2tic_textwidth = 0;
	}
	/* EAM May 2009
	 * Check to see if any xtic labels are so long that they extend beyond
	 * the right boundary of the plot. If so, allow extra room in the margin.
	 * If the labels are too long to fit even with a big margin, too bad.
	 */
	if(rGg[FIRST_X_AXIS].ticdef.def.user) {
		ticmark * tic = rGg[FIRST_X_AXIS].ticdef.def.user;
		int maxrightlabel = rGg.PlotBounds.xright;
		// We don'pT really know the plot layout yet, but try for an estimate
		rGg[FIRST_X_AXIS].SetScaleAndRange(rGg.PlotBounds.xleft, rGg.PlotBounds.xright);
		while(tic) {
			if(tic->label) {
				const int length = (int)(estimate_strlen(tic->label) * cos(DEG2RAD * (double)(rGg[FIRST_X_AXIS].tic_rotate)) * term->HChr);
				if(inrange(tic->position, rGg[FIRST_X_AXIS].SetRange.low, rGg[FIRST_X_AXIS].SetRange.upp)) {
					double xx = rGg.LogValueChecked(FIRST_X_AXIS, tic->position, "xtic");
					xx = rGg.Map(FIRST_X_AXIS, xx);
					xx += (rGg[FIRST_X_AXIS].tic_rotate) ? length : length /2;
					SETMAX(maxrightlabel, (int)xx);
				}
			}
			tic = tic->next;
		}
		xtic_textwidth = maxrightlabel - rGg.PlotBounds.xright;
		if(xtic_textwidth > (int)(term->xmax/4)) {
			xtic_textwidth = term->xmax/4;
			GpGg.IntWarn(NO_CARET, "difficulty making room for xtic labels");
		}
	}
	/* tics */
	if(!(rGg[SECOND_Y_AXIS].Flags & GpAxis::fTicIn) && ((rGg[SECOND_Y_AXIS].ticmode & TICS_ON_BORDER)
		|| ((rGg[FIRST_Y_AXIS].ticmode & TICS_MIRROR) && (rGg[FIRST_Y_AXIS].ticmode & TICS_ON_BORDER))))
		y2tic_width = (int)(pT->HTic * rGg[SECOND_Y_AXIS].ticscale);
	else
		y2tic_width = 0;
	/* y2label */
	if(can_rotate && rGg[SECOND_Y_AXIS].label.text) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[SECOND_Y_AXIS].label.offset, &tmpx, &tmpy, "boundary");
		y2label_textwidth = (int)(y2lablin * pT->VChr + tmpx);
		if(!rGg[SECOND_Y_AXIS].ticmode)
			y2label_textwidth += (int)(0.5 * pT->VChr);
	}
	else
		y2label_textwidth = 0;
	// Make room for the color box if needed
	if(rGg.RMrg.scalex != screen) {
		if(is_plot_with_colorbox()) {
#define COLORBOX_SCALE 0.100
#define WIDEST_COLORBOX_TICTEXT 3
			if((rGg.ColorBox.where != SMCOLOR_BOX_NO) && (rGg.ColorBox.where != SMCOLOR_BOX_USER)) {
				rGg.PlotBounds.xright -= (int)((int)(rGg.PlotBounds.xright-rGg.PlotBounds.xleft)*COLORBOX_SCALE);
				rGg.PlotBounds.xright -= (int)((pT->HChr) * WIDEST_COLORBOX_TICTEXT);
			}
			rGg.ColorBox.xoffset = 0;
		}
		if(rGg.RMrg.x < 0) {
			rGg.ColorBox.xoffset = rGg.PlotBounds.xright;
			rGg.PlotBounds.xright -= y2tic_width + y2tic_textwidth;
			if(y2label_textwidth > 0)
				rGg.PlotBounds.xright -= y2label_textwidth;
			{
				const double _rgt = ((rGg.XSz+rGg.XOffs)*(pT->xmax-1) - (pT->HChr * 2));
				if(rGg.PlotBounds.xright > _rgt)
					rGg.PlotBounds.xright = (int)_rgt;
			}
			rGg.ColorBox.xoffset -= rGg.PlotBounds.xright;
			/* EAM 2009 - protruding xtic labels */
			if((int)(term->xmax - rGg.PlotBounds.xright) < xtic_textwidth)
				rGg.PlotBounds.xright = term->xmax - xtic_textwidth;
			/* DBT 12-3-98  extra margin just in case */
			rGg.PlotBounds.xright -= (int)(1.0 * pT->HChr);
		}
		/* Note: we took care of explicit 'set rGg.RMrg foo' at line 502 */
	}
	// end of rGg.PlotBounds.xright calculation }}} 
	/* Set up x and x2 tics */
	/* we should base the guide on the width of the xtics, but we cannot
	 * use widest_tics until tics are set up. Bit of a downer - let us
	 * assume tics are 5 characters wide
	 */
	// HBB 20001205: moved this block to before rGg.AspectRatio is
	// applied: setup_tics may extend the ranges, which would distort the aspect ratio 
	rGg[FIRST_X_AXIS].SetupTics(20);
	rGg[SECOND_X_AXIS].SetupTics(20);
	if(rGg.IsPolar)
		rGg[POLAR_AXIS].SetupTics(10);
	// Modify the bounding box to fit the aspect ratio, if any was given
	if(rGg.AspectRatio != 0.0) {
		double current_aspect_ratio = (rGg.AspectRatio < 0 && (rGg.GetX().GetRange()) != 0.0) ? -rGg.AspectRatio * fabs((rGg.GetY().GetRange()) / (rGg.GetX().GetRange())) : rGg.AspectRatio;
		/* Set aspect ratio if valid and sensible */
		/* EAM Mar 2008 - fixed borders take precedence over centering */
		if(current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
			double current = ((double)(rGg.PlotBounds.ytop - rGg.PlotBounds.ybot)) / (rGg.PlotBounds.xright - rGg.PlotBounds.xleft);
			double required = (current_aspect_ratio * pT->VTic) / pT->HTic;
			if(current > required) {
				/* too tall */
				const int old_height = rGg.PlotBounds.ytop - rGg.PlotBounds.ybot;
				const int new_height = (int)(required * (rGg.PlotBounds.xright - rGg.PlotBounds.xleft));
				if(rGg.BMrg.scalex == screen)
					rGg.PlotBounds.ytop = rGg.PlotBounds.ybot + new_height;
				else if(rGg.TMrg.scalex == screen)
					rGg.PlotBounds.ybot = rGg.PlotBounds.ytop - new_height;
				else {
					rGg.PlotBounds.ybot += (old_height - new_height) / 2;
					rGg.PlotBounds.ytop -= (old_height - new_height) / 2;
				}
			}
			else {
				const int old_width = rGg.PlotBounds.xright - rGg.PlotBounds.xleft;
				const int new_width = (int)((rGg.PlotBounds.ytop - rGg.PlotBounds.ybot) / required);
				if(rGg.LMrg.scalex == screen)
					rGg.PlotBounds.xright = rGg.PlotBounds.xleft + new_width;
				else if(rGg.RMrg.scalex == screen)
					rGg.PlotBounds.xleft = rGg.PlotBounds.xright - new_width;
				else {
					rGg.PlotBounds.xleft += (old_width - new_width) / 2;
					rGg.PlotBounds.xright -= (old_width - new_width) / 2;
				}
			}
		}
	}
	//
	// Calculate space needed for tic label rotation.
	// If [tb]margin is auto, move the plot boundary.
	// Otherwise use textheight to adjust placement of various titles.
	//
	if(rGg[SECOND_X_AXIS].ticmode & TICS_ON_BORDER && vertical_x2tics) {
		// Assuming left justified tic labels. Correction below if they aren'pT 
		double projection = sin((double)rGg[SECOND_X_AXIS].tic_rotate*DEG2RAD);
		if(rGg[SECOND_X_AXIS].label.pos == RIGHT)
			projection *= -1;
		else if(rGg[SECOND_X_AXIS].label.pos == CENTRE)
			projection = 0.5*fabs(projection);
		rGg.widest_tic_strlen = 0;  // reset the global variable ... 
		rGg.GenTics(pT, rGg[SECOND_X_AXIS], &GpGadgets::WidestTicCallback);
		if(rGg.TMrg.x < 0) // Undo original estimate 
			rGg.PlotBounds.ytop += x2tic_textheight;
		// Adjust spacing for rotation 
		if(projection > 0.0)
			x2tic_textheight += (int)((int)(pT->HChr * (rGg.widest_tic_strlen)) * projection);
		if(rGg.TMrg.x < 0)
			rGg.PlotBounds.ytop -= x2tic_textheight;
	}
	if(rGg[FIRST_X_AXIS].ticmode & TICS_ON_BORDER && vertical_xtics) {
		double projection;
		// This adjustment will happen again in rGg.AxisOutputTics but we need it now 
		if(rGg[FIRST_X_AXIS].tic_rotate == TEXT_VERTICAL && !(rGg[FIRST_X_AXIS].Flags & GpAxis::fManualJustify))
			rGg[FIRST_X_AXIS].label.pos = RIGHT;
		if(rGg[FIRST_X_AXIS].tic_rotate == 90)
			projection = -1.0;
		else if(rGg[FIRST_X_AXIS].tic_rotate == TEXT_VERTICAL)
			projection = -1.0;
		else
			projection = -sin((double)rGg[FIRST_X_AXIS].tic_rotate*DEG2RAD);
		if(rGg[FIRST_X_AXIS].label.pos == RIGHT)
			projection *= -1;
		rGg.widest_tic_strlen = 0; // reset the global variable ... 
		rGg.GenTics(pT, rGg[FIRST_X_AXIS], &GpGadgets::WidestTicCallback);
		if(rGg.BMrg.x < 0)
			rGg.PlotBounds.ybot -= xtic_textheight;
		if(projection > 0.0)
			xtic_textheight = (int)((int)(pT->HChr * rGg.widest_tic_strlen) * projection + pT->VChr);
		if(rGg.BMrg.x < 0)
			rGg.PlotBounds.ybot += xtic_textheight;
	}
	// EAM - FIXME
	// Notwithstanding all these fancy calculations, rGg.PlotBounds.ytop must always be above rGg.PlotBounds.ybot
	//
	if(rGg.PlotBounds.ytop < rGg.PlotBounds.ybot) {
		Exchange(&rGg.PlotBounds.ytop, &rGg.PlotBounds.ybot);
		FPRINTF((stderr, "boundary: Big problems! rGg.PlotBounds.ybot > rGg.PlotBounds.ytop\n"));
	}
	//
	// compute coordinates for axis labels, rGg.title et al (some of these may not be used)
	//
	X2LabelY = rGg.PlotBounds.ytop + x2tic_height + x2tic_textheight + x2label_textheight;
	if(x2tic_textheight && (title_textheight || x2label_textheight))
		X2LabelY += pT->VChr;
	title_y = X2LabelY + title_textheight;
	YLabelY = rGg.PlotBounds.ytop + x2tic_height + x2tic_textheight + ylabel_textheight;
	Y2LabelY = rGg.PlotBounds.ytop + x2tic_height + x2tic_textheight + y2label_textheight;
	/* Shift upward by 0.2 line to allow for descenders in xlabel text */
	XLabelY = (int)(rGg.PlotBounds.ybot - xtic_height - xtic_textheight - xlabel_textheight + ((float)xlablin+0.2) * pT->VChr);
	YLabelX = rGg.PlotBounds.xleft - ytic_width - ytic_textwidth;
	if(rGg[FIRST_Y_AXIS].label.text && can_rotate)
		YLabelX -= ylabel_textwidth;
	Y2LabelX = rGg.PlotBounds.xright + y2tic_width + y2tic_textwidth;
	if(rGg[SECOND_Y_AXIS].label.text && can_rotate)
		Y2LabelX += y2label_textwidth - y2lablin * pT->VChr;

	if(vertical_timelabel) {
		if(rGg.timelabel_bottom)
			time_y = XLabelY - timebot_textheight + xlabel_textheight;
		else {
			time_y = title_y + timetop_textheight - title_textheight - x2label_textheight;
		}
	}
	else {
		if(rGg.timelabel_bottom)
			time_y = rGg.PlotBounds.ybot - xtic_height - xtic_textheight - xlabel_textheight - timebot_textheight + pT->VChr;
		else if(ylabel_textheight > 0)
			time_y = YLabelY + timetop_textheight;
		else
			time_y = rGg.PlotBounds.ytop + x2tic_height + x2tic_textheight + timetop_textheight + (int)pT->HChr;
	}
	if(vertical_timelabel)
		time_x = rGg.PlotBounds.xleft - ytic_width - ytic_textwidth - timelabel_textwidth;
	else {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg.timelabel.offset, &tmpx, &tmpy, "boundary");
		time_x = rGg.PlotBounds.xleft - ytic_width - ytic_textwidth + (int)(tmpx);
	}
	xtic_y  = rGg.PlotBounds.ybot   - xtic_height  - (int)(vertical_xtics ? pT->HChr : pT->VChr);
	x2tic_y = rGg.PlotBounds.ytop   + x2tic_height + (vertical_x2tics ? (int)pT->HChr : x2tic_textheight);
	ytic_x  = rGg.PlotBounds.xleft  - ytic_width   - (vertical_ytics ? (ytic_textwidth - (int)pT->VChr) : (int)pT->HChr);
	y2tic_x = rGg.PlotBounds.xright + y2tic_width  + (int)(vertical_y2tics ? pT->VChr : pT->HChr);
	/* restore text to horizontal [we tested rotation above] */
	(void)(*pT->text_angle)(0);
	/* needed for map_position() below */
	rGg[FIRST_X_AXIS].SetScaleAndRange(rGg.PlotBounds.xleft, rGg.PlotBounds.xright);
	rGg[SECOND_X_AXIS].SetScaleAndRange(rGg.PlotBounds.xleft, rGg.PlotBounds.xright);
	rGg[FIRST_Y_AXIS].SetScaleAndRange(rGg.PlotBounds.ybot,  rGg.PlotBounds.ytop);
	rGg[SECOND_Y_AXIS].SetScaleAndRange(rGg.PlotBounds.ybot,  rGg.PlotBounds.ytop);
	DoKeyBounds(pT, rGg, key); /* Calculate limiting bounds of the key */
	rGg.P_Clip = &rGg.PlotBounds; /* Set default clipping to the plot boundary */
	/* Sanity check. FIXME:  Stricter test? Fatal error? */
	if(rGg.PlotBounds.xright < rGg.PlotBounds.xleft || rGg.PlotBounds.ytop < rGg.PlotBounds.ybot)
		GpGg.IntWarn(NO_CARET, "Terminal rGg.Canvas area too small to hold plot.\n\t    Check plot boundary and font sizes.");
}

/*}}} */

void GpBoundary::DoKeyBounds(GpTermEntry * pT, GpGadgets & rGg, legend_key * key)
{
	key_height = (int)(key_title_height + key_title_extra + key_rows * KeyEntryHeight + key->height_fix * KeyEntryHeight);
	key_width = key_col_wth * KeyCols;
	/* Key inside plot boundaries */
	if(key->region == GPKEY_AUTO_INTERIOR_LRTBC || (key->region == GPKEY_AUTO_EXTERIOR_LRTBC && key->vpos == JUST_CENTRE && key->hpos == CENTRE)) {
		if(key->vpos == JUST_TOP) {
			key->bounds.ytop = rGg.PlotBounds.ytop - pT->VTic;
			key->bounds.ybot = key->bounds.ytop - key_height;
		}
		else if(key->vpos == JUST_BOT) {
			key->bounds.ybot = rGg.PlotBounds.ybot + pT->VTic;
			key->bounds.ytop = key->bounds.ybot + key_height;
		}
		else { /* (key->vpos == JUST_CENTRE) */
			key->bounds.ybot = ((rGg.PlotBounds.ybot + rGg.PlotBounds.ytop) - key_height) / 2;
			key->bounds.ytop = ((rGg.PlotBounds.ybot + rGg.PlotBounds.ytop) + key_height) / 2;
		}
		if(key->hpos == LEFT) {
			key->bounds.xleft = rGg.PlotBounds.xleft + pT->HChr;
			key->bounds.xright = key->bounds.xleft + key_width;
		}
		else if(key->hpos == RIGHT) {
			key->bounds.xright = rGg.PlotBounds.xright - pT->HChr;
			key->bounds.xleft = key->bounds.xright - key_width;
		}
		else { /* (key->hpos == CENTER) */
			key->bounds.xleft = ((rGg.PlotBounds.xright + rGg.PlotBounds.xleft) - key_width) / 2;
			key->bounds.xright = ((rGg.PlotBounds.xright + rGg.PlotBounds.xleft) + key_width) / 2;
		}
		// Key outside plot boundaries
	}
	else if(oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN)) {
		//
		// Vertical alignment
		//
		if(key->margin == GPKEY_TMARGIN) {
			// align top first since rGg.TMrg may be manual
			key->bounds.ytop = (int)((rGg.YSz + rGg.YOffs) * pT->ymax - pT->VTic);
			key->bounds.ybot = key->bounds.ytop - key_height;
		}
		else if(key->margin == GPKEY_BMARGIN) {
			// align bottom first since rGg.BMrg may be manual
			key->bounds.ybot = (int)(rGg.YOffs * pT->ymax + pT->VTic);
			key->bounds.ytop = key->bounds.ybot + key_height;
		}
		else {
			if(key->vpos == JUST_TOP) {
				// align top first since rGg.TMrg may be manual
				key->bounds.ytop = rGg.PlotBounds.ytop;
				key->bounds.ybot = key->bounds.ytop - key_height;
			}
			else if(key->vpos == JUST_CENTRE) {
				key->bounds.ybot = ((rGg.PlotBounds.ybot + rGg.PlotBounds.ytop) - key_height) / 2;
				key->bounds.ytop = ((rGg.PlotBounds.ybot + rGg.PlotBounds.ytop) + key_height) / 2;
			}
			else {
				// align bottom first since rGg.BMrg may be manual
				key->bounds.ybot = rGg.PlotBounds.ybot;
				key->bounds.ytop = key->bounds.ybot + key_height;
			}
		}
		//
		// Horizontal alignment
		//
		if(key->margin == GPKEY_LMARGIN) {
			// align left first since rGg.LMrg may be manual
			key->bounds.xleft = (int)(rGg.XOffs * pT->xmax + pT->HChr);
			key->bounds.xright = key->bounds.xleft + key_width;
		}
		else if(key->margin == GPKEY_RMARGIN) {
			// align right first since rGg.RMrg may be manual
			key->bounds.xright = (int)((rGg.XSz + rGg.XOffs) * (pT->xmax-1) - pT->HChr);
			key->bounds.xleft = key->bounds.xright - key_width;
		}
		else {
			if(key->hpos == LEFT) {
				// align left first since rGg.LMrg may be manual
				key->bounds.xleft = rGg.PlotBounds.xleft;
				key->bounds.xright = key->bounds.xleft + key_width;
			}
			else if(key->hpos == CENTRE) {
				key->bounds.xleft  = ((rGg.PlotBounds.xright + rGg.PlotBounds.xleft) - key_width) / 2;
				key->bounds.xright = ((rGg.PlotBounds.xright + rGg.PlotBounds.xleft) + key_width) / 2;
			}
			else {
				// align right first since rGg.RMrg may be manual
				key->bounds.xright = rGg.PlotBounds.xright;
				key->bounds.xleft = key->bounds.xright - key_width;
			}
		}
		// Key at explicit position specified by user
	}
	else {
		int x, y;
		/* FIXME!!!
		 * pm 22.1.2002: if key->user_pos.scalex or scaley == first_axes or second_axes,
		 * then the graph scaling is not yet known and the box is positioned incorrectly;
		 * you must do "replot" to avoid the wrong plot ... bad luck if output does not
		 * go to screen
		 */
		rGg.MapPosition(pT, &key->user_pos, &x, &y, "key");
		// Here top, bottom, left, right refer to the alignment with respect to point.
		key->bounds.xleft = x;
		if(key->hpos == CENTRE)
			key->bounds.xleft -= key_width/2;
		else if(key->hpos == RIGHT)
			key->bounds.xleft -= key_width;
		key->bounds.xright = key->bounds.xleft + key_width;
		key->bounds.ytop = y;
		if(key->vpos == JUST_CENTRE)
			key->bounds.ytop += key_height/2;
		else if(key->vpos == JUST_BOT)
			key->bounds.ytop += key_height;
		key->bounds.ybot = key->bounds.ytop - key_height;
	}
}
//
// Calculate positioning of components that make up the key box
//
void GpBoundary::DoKeyLayout(GpTermEntry * pT, GpGadgets & rGg, legend_key * pKey)
{
	bool key_panic = false;
	/* If there is a separate font for the pKey, use it for space calculations.	*/
	if(pKey->font)
		pT->set_font(pKey->font);
	key_xleft = 0;
	key_sample_width = (pKey->swidth >= 0) ? (int)(pKey->swidth * pT->HChr + pT->HTic) : 0;
	KeyEntryHeight = (int)(pT->VTic * 1.25 * pKey->vert_factor);
	if(KeyEntryHeight < (int)pT->VChr)
		KeyEntryHeight = (int)(pT->VChr * pKey->vert_factor);
	/* HBB 20020122: safeguard to prevent division by zero later */
	SETIFZ(KeyEntryHeight, 1);
	/* Key rGg.title length and height */
	key_title_height = 0;
	key_title_extra = 0;
	if(pKey->title.text) {
		int ytheight;
		label_width(pKey->title.text, &ytheight);
		key_title_height = ytheight * pT->VChr;
		if((*pKey->title.text) && (pT->flags & TERM_ENHANCED_TEXT) && (strchr(pKey->title.text, '^') || strchr(pKey->title.text, '_')))
			key_title_extra = pT->VChr;
	}
	if(pKey->reverse) {
		key_sample_left = -key_sample_width;
		key_sample_right = 0;
		/* if pKey width is being used, adjust right-justified text */
		key_text_left = pT->HChr;
		key_text_right = (int)(pT->HChr * (max_ptitl_len + 1 + pKey->width_fix));
		key_size_left = pT->HChr - key_sample_left; /* sample left is -ve */
		key_size_right = key_text_right;
	}
	else {
		key_sample_left = 0;
		key_sample_right = key_sample_width;
		/* if pKey width is being used, adjust left-justified text */
		key_text_left = -(int)(pT->HChr * (max_ptitl_len + 1 + pKey->width_fix));
		key_text_right = -(int)pT->HChr;
		key_size_left = -key_text_left;
		key_size_right = key_sample_right + pT->HChr;
	}
	key_point_offset = (key_sample_left + key_sample_right) / 2;
	/* advance width for cols */
	key_col_wth = key_size_left + key_size_right;
	key_rows = ptitl_cnt;
	KeyCols = 1;
	/* calculate rows and cols for pKey */
	if(pKey->stack_dir == GPKEY_HORIZONTAL) {
		// maximise no cols, limited by label-length 
		KeyCols = (int)(rGg.PlotBounds.xright - rGg.PlotBounds.xleft) / key_col_wth;
		if(pKey->maxcols > 0 && KeyCols > pKey->maxcols)
			KeyCols = pKey->maxcols;
		// EAM Dec 2004 - Rather than turn off the pKey, try to squeeze 
		if(KeyCols == 0) {
			KeyCols = 1;
			key_panic = true;
			key_col_wth = (rGg.PlotBounds.xright - rGg.PlotBounds.xleft);
		}
		key_rows = (ptitl_cnt + KeyCols - 1) / KeyCols;
		/* now calculate actual no cols depending on no rows */
		KeyCols = (key_rows == 0) ? 1 : (ptitl_cnt + key_rows - 1) / key_rows;
		SETIFZ(KeyCols, 1);
	}
	else {
		/* maximise no rows, limited by rGg.PlotBounds.ytop-rGg.PlotBounds.ybot */
		int    i = (int)((rGg.PlotBounds.ytop - rGg.PlotBounds.ybot - pKey->height_fix * KeyEntryHeight - key_title_height - key_title_extra) / KeyEntryHeight);
		if(pKey->maxrows > 0 && i > pKey->maxrows)
			i = pKey->maxrows;
		if(i == 0) {
			i = 1;
			key_panic = true;
		}
		if(ptitl_cnt > i) {
			KeyCols = (ptitl_cnt + i - 1) / i;
			// now calculate actual no rows depending on no cols 
			if(KeyCols == 0) {
				KeyCols = 1;
				key_panic = true;
			}
			key_rows = (ptitl_cnt + KeyCols - 1) / KeyCols;
		}
	}
	/* If the pKey rGg.title is wider than the contents, try to make room for it */
	if(pKey->title.text) {
		int    ytlen = (int)(label_width(pKey->title.text, NULL) - pKey->swidth + 2);
		ytlen *= pT->HChr;
		if(ytlen > KeyCols * key_col_wth)
			key_col_wth = ytlen / KeyCols;
	}
	/* Adjust for outside pKey, leave manually set margins alone */
	if((pKey->region == GPKEY_AUTO_EXTERIOR_LRTBC && (pKey->vpos != JUST_CENTRE || pKey->hpos != CENTRE)) || pKey->region == GPKEY_AUTO_EXTERIOR_MARGIN) {
		int more = 0;
		if(pKey->margin == GPKEY_BMARGIN && rGg.BMrg.x < 0) {
			more = (int)(key_rows * KeyEntryHeight + key_title_height + key_title_extra + pKey->height_fix * KeyEntryHeight);
			if(rGg.PlotBounds.ybot + more > rGg.PlotBounds.ytop)
				key_panic = true;
			else
				rGg.PlotBounds.ybot += more;
		}
		else if(pKey->margin == GPKEY_TMARGIN && rGg.TMrg.x < 0) {
			more = (int)(key_rows * KeyEntryHeight + key_title_height + key_title_extra + pKey->height_fix * KeyEntryHeight);
			if(rGg.PlotBounds.ytop - more < rGg.PlotBounds.ybot)
				key_panic = true;
			else
				rGg.PlotBounds.ytop -= more;
		}
		else if(pKey->margin == GPKEY_LMARGIN && rGg.LMrg.x < 0) {
			more = key_col_wth * KeyCols;
			if(rGg.PlotBounds.xleft + more > rGg.PlotBounds.xright)
				key_panic = true;
			else
				key_xleft = more;
			rGg.PlotBounds.xleft += key_xleft;
		}
		else if(pKey->margin == GPKEY_RMARGIN && rGg.RMrg.x < 0) {
			more = key_col_wth * KeyCols;
			if(rGg.PlotBounds.xright - more < rGg.PlotBounds.xleft)
				key_panic = true;
			else
				rGg.PlotBounds.xright -= more;
		}
	}
	// Restore default font
	if(pKey->font)
		pT->set_font("");
	// warn if we had to punt on pKey size calculations
	if(key_panic)
		GpGg.IntWarn(NO_CARET, "Warning - difficulty fitting plot titles into pKey");
}

int find_maxl_keys(const CurvePoints * pPlots, int count, int * kcnt)
{
	int    len;
	int    previous_plot_style = 0;
	int    mlen = 0;
	int    cnt = 0;
	const  CurvePoints * this_plot = pPlots;
	for(int curve = 0; curve < count; this_plot = this_plot->P_Next, curve++) {
		if(this_plot->title && !this_plot->title_is_suppressed && !this_plot->title_position) {
			ignore_enhanced(this_plot->title_no_enhanced);
			len = estimate_strlen(this_plot->title);
			if(len != 0) {
				cnt++;
				SETMAX(mlen, len);
			}
			ignore_enhanced(false);
		}
		// Check for new histogram here and save space for divider
		if(this_plot->plot_style == HISTOGRAMS &&  previous_plot_style == HISTOGRAMS &&  this_plot->histogram_sequence == 0 && cnt > 1)
			cnt++;
		// Check for column-stacked histogram with key entries
		if(this_plot->plot_style == HISTOGRAMS && this_plot->labels) {
			for(GpTextLabel * key_entry = this_plot->labels->next; key_entry; key_entry = key_entry->next) {
				cnt++;
				len = key_entry->text ? estimate_strlen(key_entry->text) : 0;
				SETMAX(mlen, len);
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
void GpBoundary::DoKeySample(GpTermEntry * pT, GpGadgets & rGg, CurvePoints * pPlot, 
	legend_key * pKey, char * pTitle, int ptXl, int ptYl)
{
	/* Clip pKey box against rGg.Canvas */
	BoundingBox * clip_save = rGg.P_Clip;
	rGg.P_Clip = (pT->flags & TERM_CAN_CLIP) ? 0 : &rGg.Canvas;
	/* If the plot this rGg.title belongs to specified a non-standard place */
	/* for the pKey sample to appear, use that to override ptXl, ptYl.       */
	if(pPlot->title_position && pPlot->title_position->scalex != character) {
		rGg.MapPosition(pT, pPlot->title_position, &ptXl, &ptYl, "pKey sample");
		ptXl -=  (pKey->just == GPKEY_LEFT) ? key_text_left : key_text_right;
	}
	pT->_Layer(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(pKey->textcolor.type == TC_VARIABLE)
		; /* Draw pKey text in same color as plot */
	else if(pKey->textcolor.type != TC_DEFAULT)
		rGg.ApplyPm3DColor(pT, &pKey->textcolor); /* Draw pKey text in same color as pKey rGg.title */
	else
		pT->_LineType(LT_BLACK); /* Draw pKey text in black */
	if(pKey->just == GPKEY_LEFT) {
		pT->DrawMultiline(ptXl + key_text_left, ptYl, pTitle, LEFT, JUST_CENTRE, 0, pKey->font);
	}
	else {
		if((*pT->justify_text)(RIGHT)) {
			pT->DrawMultiline(ptXl + key_text_right, ptYl, pTitle, RIGHT, JUST_CENTRE, 0, pKey->font);
		}
		else {
			int x = ptXl + key_text_right - pT->HChr * estimate_strlen(pTitle);
			if(oneof2(pKey->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) || inrange((x), (rGg.PlotBounds.xleft), (rGg.PlotBounds.xright)))
				pT->DrawMultiline(x, ptYl, pTitle, LEFT, JUST_CENTRE, 0, pKey->font);
		}
	}
	/* Draw sample in same style and color as the corresponding plot  */
	/* The variable color case uses the color of the first data point */
	if(!check_for_variable_color(pPlot, &pPlot->varcolor[0]))
		rGg.ApplyLpProperties(pT, &pPlot->lp_properties);
	/* draw sample depending on bits set in plot_style */
	if(pPlot->plot_style & PLOT_STYLE_HAS_FILL && pT->fillbox) {
		fill_style_type * fs = &pPlot->fill_properties;
		int style = style_from_fill(fs);
		uint x = ptXl + key_sample_left;
		uint y = ptYl - KeyEntryHeight/4;
		uint w = key_sample_right - key_sample_left;
		uint h = KeyEntryHeight/2;
#ifdef EAM_OBJECTS
		if(pPlot->plot_style == CIRCLES && w > 0) {
			do_arc(ptXl + key_point_offset, ptYl, KeyEntryHeight/4, 0., 360., style, false);
			// Retrace the border if the style requests it
			if(rGg.NeedFillBorder(pT, fs)) {
				do_arc(ptXl + key_point_offset, ptYl, KeyEntryHeight/4, 0., 360., 0, false);
			}
		}
		else if(pPlot->plot_style == ELLIPSES && w > 0) {
			t_ellipse * key_ellipse = (t_ellipse*)malloc(sizeof(t_ellipse));
			key_ellipse->center.x = ptXl + key_point_offset;
			key_ellipse->center.y = ptYl;
			key_ellipse->extent.x = w * 2/3;
			key_ellipse->extent.y = h;
			key_ellipse->orientation = 0.0;
			// already in pT coords, no need to map 
			rGg.DoEllipse(pT, 2, key_ellipse, style, false);
			// Retrace the border if the style requests it 
			if(rGg.NeedFillBorder(pT, fs)) {
				rGg.DoEllipse(pT, 2, key_ellipse, 0, false);
			}
			free(key_ellipse);
		}
		else
#endif
		if(w > 0) { /* All other plot types with fill */
			if(style != FS_EMPTY)
				(*pT->fillbox)(style, x, y, w, h);
			// need_fill_border will set the border linetype, but candlesticks don'pT want it
			if((pPlot->plot_style == CANDLESTICKS && fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW) || style == FS_EMPTY || rGg.NeedFillBorder(pT, fs)) {
				newpath(pT);
				const int q_keh = KeyEntryHeight/4;
				rGg.DrawClipLine(pT, ptXl + key_sample_left,  ptYl - q_keh, ptXl + key_sample_right, ptYl - q_keh);
				rGg.DrawClipLine(pT, ptXl + key_sample_right, ptYl - q_keh, ptXl + key_sample_right, ptYl + q_keh);
				rGg.DrawClipLine(pT, ptXl + key_sample_right, ptYl + q_keh, ptXl + key_sample_left,  ptYl + q_keh);
				rGg.DrawClipLine(pT, ptXl + key_sample_left,  ptYl + q_keh, ptXl + key_sample_left,  ptYl - q_keh);
				closepath(pT);
			}
			if(fs->fillstyle != FS_EMPTY && fs->fillstyle != FS_DEFAULT && !(fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW)) {
				// need_fill_border() might have changed our original linetype
				rGg.ApplyLpProperties(pT, &pPlot->lp_properties);
			}
		}
	}
	else if(pPlot->plot_style == VECTOR && pT->arrow) {
		rGg.ApplyHeadProperties(pPlot->arrow_properties);
		rGg.DrawClipArrow(pT, ptXl + key_sample_left, ptYl, ptXl + key_sample_right, ptYl, pPlot->arrow_properties.head);
	}
	else if(pPlot->lp_properties.l_type == LT_NODRAW) {
		;
	}
	else if((pPlot->plot_style & PLOT_STYLE_HAS_ERRORBAR) && pPlot->plot_type == DATA) {
		// errors for data plots only 
		if(rGg.BarLp.flags & LP_ERRORBAR_SET)
			rGg.ApplyLpProperties(pT, &rGg.BarLp);
		rGg.DrawClipLine(pT, ptXl + key_sample_left, ptYl, ptXl + key_sample_right, ptYl);
		// Even if error bars are dotted, the end lines are always solid 
		if((rGg.BarLp.flags & LP_ERRORBAR_SET) != 0)
			pT->dashtype(DASHTYPE_SOLID, NULL);
	}
	else if(pPlot->plot_style & PLOT_STYLE_HAS_LINE) {
		rGg.DrawClipLine(pT, ptXl + key_sample_left, ptYl, ptXl + key_sample_right, ptYl);
	}
	if(pPlot->plot_type == DATA && (pPlot->plot_style & PLOT_STYLE_HAS_ERRORBAR) && pPlot->plot_style != CANDLESTICKS && 
		rGg.BarSize > 0.0) {
		const int errbartic = MAX((pT->HTic/2), 1);
		rGg.DrawClipLine(pT, ptXl + key_sample_left,  ptYl + errbartic, ptXl + key_sample_left,  ptYl - errbartic);
		rGg.DrawClipLine(pT, ptXl + key_sample_right, ptYl + errbartic, ptXl + key_sample_right, ptYl - errbartic);
	}
	/* oops - doing the point sample now would break the postscript
	 * terminal for example, which changes current line style
	 * when drawing a point, but does not restore it. We must wait to
	 * draw the point sample at the end of do_plot (comment KEY SAMPLES).
	 */
	pT->_Layer(TERM_LAYER_END_KEYSAMPLE);
	/* Restore previous clipping area */
	rGg.P_Clip = clip_save;
}

void GpBoundary::DoKeySamplePoint(GpTermEntry * pT, GpGadgets & rGg, CurvePoints * pPlot, legend_key * key, int xl, int yl)
{
	// If the plot this rGg.title belongs to specified a non-standard place 
	// for the key sample to appear, use that to override xl, yl.       
	if(pPlot->title_position && pPlot->title_position->scalex != character) {
		rGg.MapPosition(pT, pPlot->title_position, &xl, &yl, "key sample");
		xl -=  (key->just == GPKEY_LEFT) ? key_text_left : key_text_right;
	}
	pT->_Layer(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(pPlot->plot_style == LINESPOINTS && pPlot->lp_properties.p_interval < 0) {
		t_colorspec background_fill(TC_LT, LT_BACKGROUND, 0.0);
		(*pT->set_color)(&background_fill);
		(*pT->pointsize)(rGg.PtSz * rGg.PtIntervalBox);
		(*pT->point)(xl + key_point_offset, yl, 6);
		rGg.ApplyLpProperties(pT, &pPlot->lp_properties);
	}
	if(pPlot->plot_style == BOXPLOT) {
		; /* Don'pT draw a sample point in the key */
	}
	else if(pPlot->plot_style == DOTS) {
		if(on_page(xl + key_point_offset, yl))
			(*pT->point)(xl + key_point_offset, yl, -1);
	}
	else if(pPlot->plot_style & PLOT_STYLE_HAS_POINT) {
		if(pPlot->lp_properties.p_size == PTSZ_VARIABLE)
			(*pT->pointsize)(rGg.PtSz);
		if(on_page(xl + key_point_offset, yl)) {
			if(pPlot->lp_properties.p_type == PT_CHARACTER) {
				rGg.ApplyPm3DColor(pT, &(pPlot->labels->textcolor));
				pT->_PutText(xl + key_point_offset, yl, pPlot->lp_properties.p_char);
				rGg.ApplyPm3DColor(pT, &(pPlot->lp_properties.pm3d_color));
			}
			else {
				(*pT->point)(xl + key_point_offset, yl, pPlot->lp_properties.p_type);
			}
		}
	}
	else if(pPlot->plot_style == LABELPOINTS) {
		GpTextLabel * label = pPlot->labels;
		if(label->lp_properties.flags & LP_SHOW_POINTS) {
			rGg.ApplyLpProperties(pT, &label->lp_properties);
			(*pT->point)(xl + key_point_offset, yl, label->lp_properties.p_type);
		}
	}
	pT->_Layer(TERM_LAYER_END_KEYSAMPLE);
}
//
// Graph legend is now optionally done in two passes. The first pass calculates
// and reserves the necessary space.  Next the individual plots in the graph
// are drawn. Then the reserved space for the legend is blanked out, and
// finally the second pass through this code draws the legend.
//
void GpBoundary::DrawKey(GpTermEntry * pT, GpGadgets & rGg, legend_key * pKey, bool keyPass, int * pXInKey, int * pYInKey)
{
	// In two-pass mode (set pKey opaque) we blank out the pKey box after
	// the graph is drawn and then redo the pKey in the blank area.
	if(keyPass && pT->fillbox && !(pT->flags & TERM_NULL_SET_COLOR)) {
		t_colorspec background_fill(TC_LT, LT_BACKGROUND, 0.0);
		(*pT->set_color)(&background_fill);
		(*pT->fillbox)(FS_OPAQUE, pKey->bounds.xleft, pKey->bounds.ybot, key_width, key_height);
	}
	if(pKey->title.text) {
		int title_anchor;
		if(pKey->title.pos == CENTRE)
			title_anchor = (pKey->bounds.xleft + pKey->bounds.xright) / 2;
		else if(pKey->title.pos == RIGHT)
			title_anchor = pKey->bounds.xright - pT->HChr;
		else
			title_anchor = pKey->bounds.xleft + pT->HChr;
		/* Only draw the rGg.title once */
		if(keyPass || !pKey->front) {
			// FIXME: Now that there is a full GpTextLabel structure for the pKey rGg.title
			//        maybe we should call write_label() to get the full processing?
			rGg.ApplyPm3DColor(pT, (pKey->textcolor.type == TC_RGB && pKey->textcolor.value < 0) ? &(pKey->box.pm3d_color) : &(pKey->textcolor));
			ignore_enhanced(pKey->title.noenhanced);
			pT->DrawMultiline(title_anchor, pKey->bounds.ytop - (key_title_extra + KeyEntryHeight)/2, pKey->title.text, pKey->title.pos, JUST_TOP, 0, pKey->title.font ? pKey->title.font : pKey->font);
			ignore_enhanced(false);
			pT->_LineType(LT_BLACK);
		}
	}
	if(pKey->box.l_type > LT_NODRAW) {
		BoundingBox * clip_save = rGg.P_Clip;
		rGg.P_Clip = (pT->flags & TERM_CAN_CLIP) ? 0 : &rGg.Canvas;
		rGg.ApplyLpProperties(pT, &pKey->box);
		{
			newpath(pT);
			rGg.DrawClipLine(pT, pKey->bounds.xleft,  pKey->bounds.ybot, pKey->bounds.xleft,  pKey->bounds.ytop);
			rGg.DrawClipLine(pT, pKey->bounds.xleft,  pKey->bounds.ytop, pKey->bounds.xright, pKey->bounds.ytop);
			rGg.DrawClipLine(pT, pKey->bounds.xright, pKey->bounds.ytop, pKey->bounds.xright, pKey->bounds.ybot);
			rGg.DrawClipLine(pT, pKey->bounds.xright, pKey->bounds.ybot, pKey->bounds.xleft,  pKey->bounds.ybot);
			closepath(pT);
		}
		// draw a horizontal line between pKey rGg.title and first entry
		if(pKey->title.text)
			rGg.DrawClipLine(pT, pKey->bounds.xleft, pKey->bounds.ytop - (key_title_height + key_title_extra), pKey->bounds.xright, pKey->bounds.ytop - (key_title_height + key_title_extra));
		rGg.P_Clip = clip_save;
	}
	yl_ref = pKey->bounds.ytop - (key_title_height + key_title_extra);
	yl_ref -= (int)(((pKey->height_fix + 1) * KeyEntryHeight) / 2);
	*pXInKey = pKey->bounds.xleft + key_size_left;
	*pYInKey = yl_ref;
}
//
// This routine draws the plot GpGg.title, the axis labels, and an optional time stamp.
//
void GpBoundary::DrawTitles(GpTermEntry * pT, GpGadgets & rGg)
{
	// YLABEL 
	if(rGg[FIRST_Y_AXIS].label.text) {
		ignore_enhanced(rGg[FIRST_Y_AXIS].label.noenhanced);
		rGg.ApplyPm3DColor(pT, &(rGg[FIRST_Y_AXIS].label.textcolor));
		// we worked out x-posn in boundary() 
		if(pT->text_angle(rGg[FIRST_Y_AXIS].label.rotate)) {
			double tmpx, tmpy;
			rGg.MapPositionR(rGg[FIRST_Y_AXIS].label.offset, &tmpx, &tmpy, "doplot");
			uint x = YLabelX + (pT->VChr / 2);
			uint y = (uint)((rGg.PlotBounds.ytop + rGg.PlotBounds.ybot) / 2 + tmpy);
			//pT->DrawMultiline(x, y, rGg[FIRST_Y_AXIS].label.text, CENTRE, JUST_TOP, rGg[FIRST_Y_AXIS].label.rotate, rGg[FIRST_Y_AXIS].label.font);
			DrawAxisLabel(x, y, rGg[FIRST_Y_AXIS], CENTRE, JUST_TOP, false);
			pT->text_angle(0);
		}
		else {
			// really bottom just, but we know number of lines so we need to adjust x-posn by one line
			uint x = YLabelX;
			uint y = YLabelY;
			//pT->DrawMultiline(x, y, rGg[FIRST_Y_AXIS].label.text, LEFT, JUST_TOP, 0, rGg[FIRST_Y_AXIS].label.font);
			DrawAxisLabel(x, y, rGg[FIRST_Y_AXIS], LEFT, JUST_TOP, true);
		}
		reset_textcolor(&(rGg[FIRST_Y_AXIS].label.textcolor));
		ignore_enhanced(false);
	}
	// Y2LABEL 
	if(rGg[SECOND_Y_AXIS].label.text) {
		ignore_enhanced(rGg[SECOND_Y_AXIS].label.noenhanced);
		rGg.ApplyPm3DColor(pT, &(rGg[SECOND_Y_AXIS].label.textcolor));
		// we worked out coordinates in boundary() 
		if(pT->text_angle(rGg[SECOND_Y_AXIS].label.rotate)) {
			double tmpx, tmpy;
			rGg.MapPositionR(rGg[SECOND_Y_AXIS].label.offset, &tmpx, &tmpy, "doplot");
			uint   x = Y2LabelX + (pT->VChr / 2) - 1;
			uint   y = (uint)((rGg.PlotBounds.ytop + rGg.PlotBounds.ybot) / 2 + tmpy);
			//pT->DrawMultiline(x, y, rGg[SECOND_Y_AXIS].label.text, CENTRE, JUST_TOP, rGg[SECOND_Y_AXIS].label.rotate, rGg[SECOND_Y_AXIS].label.font);
			DrawAxisLabel(x, y, rGg[SECOND_Y_AXIS], CENTRE, JUST_TOP, false);
			pT->text_angle(0);
		}
		else {
			// really bottom just, but we know number of lines
			uint x = Y2LabelX;
			uint y = Y2LabelY;
			//pT->DrawMultiline(x, y, rGg[SECOND_Y_AXIS].label.text, RIGHT, JUST_TOP, 0, rGg[SECOND_Y_AXIS].label.font);
			DrawAxisLabel(x, y, rGg[SECOND_Y_AXIS], RIGHT, JUST_TOP, true);
		}
		reset_textcolor(&(rGg[SECOND_Y_AXIS].label.textcolor));
		ignore_enhanced(false);
	}
	// XLABEL 
	if(rGg[FIRST_X_AXIS].label.text) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[FIRST_X_AXIS].label.offset, &tmpx, &tmpy, "doplot");
		uint   x = (uint)((rGg.PlotBounds.xright + rGg.PlotBounds.xleft) / 2 +  tmpx);
		uint   y = XLabelY - pT->VChr / 2; /* HBB */
		ignore_enhanced(rGg[FIRST_X_AXIS].label.noenhanced);
		rGg.ApplyPm3DColor(pT, &(rGg[FIRST_X_AXIS].label.textcolor));
		//pT->DrawMultiline(x, y, rGg[FIRST_X_AXIS].label.text, CENTRE, JUST_TOP, 0, rGg[FIRST_X_AXIS].label.font);
		DrawAxisLabel(x, y, rGg[FIRST_X_AXIS], CENTRE, JUST_TOP, true);
		reset_textcolor(&(rGg[FIRST_X_AXIS].label.textcolor));
		ignore_enhanced(false);
	}
	// PLACE TITLE 
	if(rGg.title.text) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg.title.offset, &tmpx, &tmpy, "doplot");
		/* we worked out y-GpCoordinate in boundary() */
		uint x = (uint)((rGg.PlotBounds.xleft + rGg.PlotBounds.xright) / 2 + tmpx);
		uint y = title_y - pT->VChr / 2;
		ignore_enhanced(rGg.title.noenhanced);
		rGg.ApplyPm3DColor(pT, &(rGg.title.textcolor));
		pT->DrawMultiline(x, y, rGg.title.text, CENTRE, JUST_TOP, 0, rGg.title.font);
		reset_textcolor(&(rGg.title.textcolor));
		ignore_enhanced(false);
	}
	// X2LABEL 
	if(rGg[SECOND_X_AXIS].label.text) {
		double tmpx, tmpy;
		rGg.MapPositionR(rGg[SECOND_X_AXIS].label.offset, &tmpx, &tmpy, "doplot");
		// we worked out y-GpCoordinate in boundary()
		uint x = (uint)((rGg.PlotBounds.xright + rGg.PlotBounds.xleft) / 2 + tmpx);
		uint y = X2LabelY - pT->VChr / 2 - 1;
		ignore_enhanced(rGg[SECOND_X_AXIS].label.noenhanced);
		rGg.ApplyPm3DColor(pT, &(rGg[SECOND_X_AXIS].label.textcolor));
		//pT->DrawMultiline(x, y, rGg[SECOND_X_AXIS].label.text, CENTRE, JUST_TOP, 0, rGg[SECOND_X_AXIS].label.font);
		DrawAxisLabel(x, y, rGg[SECOND_X_AXIS], CENTRE, JUST_TOP, true);
		reset_textcolor(&(rGg[SECOND_X_AXIS].label.textcolor));
		ignore_enhanced(false);
	}
	// PLACE TIMEDATE 
	if(rGg.timelabel.text) {
		// we worked out coordinates in boundary() 
		time_t now;
		uint x = time_x;
		uint y = time_y;
		time(&now);
		// there is probably no way to find out in advance how many chars strftime() writes
		char * str = (char *)malloc(MAX_LINE_LEN + 1);
		strftime(str, MAX_LINE_LEN, rGg.timelabel.text, localtime(&now));
		rGg.ApplyPm3DColor(pT, &(rGg.timelabel.textcolor));
		if(rGg.timelabel_rotate && pT->text_angle(TEXT_VERTICAL)) {
			x += pT->VChr / 2; /* HBB */
			if(rGg.timelabel_bottom)
				pT->DrawMultiline(x, y, str, LEFT, JUST_TOP, TEXT_VERTICAL, rGg.timelabel.font);
			else
				pT->DrawMultiline(x, y, str, RIGHT, JUST_TOP, TEXT_VERTICAL, rGg.timelabel.font);
			pT->text_angle(0);
		}
		else {
			y -= pT->VChr / 2; /* HBB */
			if(rGg.timelabel_bottom)
				pT->DrawMultiline(x, y, str, LEFT, JUST_BOT, 0, rGg.timelabel.font);
			else
				pT->DrawMultiline(x, y, str, LEFT, JUST_TOP, 0, rGg.timelabel.font);
		}
		free(str);
	}
}
