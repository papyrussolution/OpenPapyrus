/* GNUPLOT - term.c */

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

/*
 * Bookkeeping and support routines for 'set multiplot layout ...'
 * Jul 2004 Volker Dobler     layout rows, columns
 * Feb 2013 Christoph Bersch  layout margins spacing
 * Mar 2014 Ethan A Merritt   refactor into separate file (used to be in term.c)
 */
#include <gnuplot.h>
#pragma hdrstop

//static void mp_layout_size_and_offset();
//static void mp_layout_margins_and_spacing();
static void mp_layout_set_margin_or_spacing(GpCommand & rC, GpPosition *);

enum set_multiplot_id {
	S_MULTIPLOT_LAYOUT,
	S_MULTIPLOT_COLUMNSFIRST, 
	S_MULTIPLOT_ROWSFIRST, 
	S_MULTIPLOT_SCALE,
	S_MULTIPLOT_DOWNWARDS, 
	S_MULTIPLOT_UPWARDS,
	S_MULTIPLOT_OFFSET, 
	S_MULTIPLOT_TITLE,
	S_MULTIPLOT_MARGINS, 
	S_MULTIPLOT_SPACING,
	S_MULTIPLOT_INVALID
};

static const GenTable set_multiplot_tbl[] =
{
	{ "lay$out", S_MULTIPLOT_LAYOUT },
	{ "col$umnsfirst", S_MULTIPLOT_COLUMNSFIRST },
	{ "row$sfirst", S_MULTIPLOT_ROWSFIRST },
	{ "down$wards", S_MULTIPLOT_DOWNWARDS },
	{ "up$wards", S_MULTIPLOT_UPWARDS },
	{ "sca$le", S_MULTIPLOT_SCALE },
	{ "off$set", S_MULTIPLOT_OFFSET },
	{ "ti$tle", S_MULTIPLOT_TITLE },
	{ "ma$rgins", S_MULTIPLOT_MARGINS },
	{ "spa$cing", S_MULTIPLOT_SPACING },
	{ NULL, S_MULTIPLOT_INVALID }
};

//static MpLayout MpL; // = MP_LAYOUT_DEFAULT;

// Helper routines 
//void multiplot_next()
void GpGadgets::MultiplotNext(GpTermEntry * pT)
{
	MpL.current_panel++;
	if(MpL.auto_layout) {
		if(MpL.row_major) {
			MpL.act_row++;
			if(MpL.act_row == MpL.num_rows) {
				MpL.act_row = 0;
				MpL.act_col++;
				if(MpL.act_col == MpL.num_cols) {
					/* int_warn(NO_CARET,"will overplot first plot"); */
					MpL.act_col = 0;
				}
			}
		}
		else { /* column-major */
			MpL.act_col++;
			if(MpL.act_col == MpL.num_cols) {
				MpL.act_col = 0;
				MpL.act_row++;
				if(MpL.act_row == MpL.num_rows) {
					/* int_warn(NO_CARET,"will overplot first plot"); */
					MpL.act_row = 0;
				}
			}
		}
		if(MpL.auto_layout_margins)
			MpLayoutMarginsAndSpacing(pT);
		else
			MpLayoutSizeAndOffset();
	}
}

//void multiplot_previous()
void GpGadgets::MultiplotPrevious(GpTermEntry * pT)
{
	MpL.current_panel--;
	if(MpL.auto_layout) {
		if(MpL.row_major) {
			MpL.act_row--;
			if(MpL.act_row < 0) {
				MpL.act_row = MpL.num_rows-1;
				MpL.act_col--;
				if(MpL.act_col < 0) {
					/* int_warn(NO_CARET,"will overplot first plot"); */
					MpL.act_col = MpL.num_cols-1;
				}
			}
		}
		else { /* column-major */
			MpL.act_col--;
			if(MpL.act_col < 0) {
				MpL.act_col = MpL.num_cols-1;
				MpL.act_row--;
				if(MpL.act_row < 0) {
					/* int_warn(NO_CARET,"will overplot first plot"); */
					MpL.act_row = MpL.num_rows-1;
				}
			}
		}
		if(MpL.auto_layout_margins)
			MpLayoutMarginsAndSpacing(pT);
		else
			MpLayoutSizeAndOffset();
	}
}

//int multiplot_current_panel()
int GpGadgets::MultiplotCurrentPanel() const
{
	return MpL.current_panel;
}

void GpGadgets::MultiplotStart(GpTermEntry * pT, GpCommand & rC)
{
	bool set_spacing = false;
	bool set_margins = false;
	rC.CToken++;
	// Only a few options are possible if we are already in multiplot mode 
	// So far we have "next".  Maybe also "previous", "clear"? 
	if(IsMultiPlot) {
		if(rC.Eq("next")) {
			rC.CToken++;
			if(!MpL.auto_layout)
				IntErrorCurToken("only valid inside an auto-layout multiplot");
			MultiplotNext(pT);
			return;
		}
		else if(rC.AlmostEq("prev$ious")) {
			rC.CToken++;
			if(!MpL.auto_layout)
				IntErrorCurToken("only valid inside an auto-layout multiplot");
			MultiplotPrevious(pT);
			return;
		}
		else {
			TermEndMultiplot(pT);
		}
	}
	/* FIXME: more options should be reset/initialized each time */
	MpL.auto_layout = false;
	MpL.auto_layout_margins = false;
	MpL.current_panel = 0;
	MpL.title.noenhanced = false;
	ZFREE(MpL.title.text);
	ZFREE(MpL.title.font);
	/* Parse options */
	while(!rC.EndOfCommand()) {
		if(rC.AlmostEq("ti$tle")) {
			rC.CToken++;
			MpL.title.text = rC.TryToGetString();
			continue;
		}
		if(rC.Eq("font")) {
			rC.CToken++;
			MpL.title.font = rC.TryToGetString();
			continue;
		}
		if(rC.AlmostEq("enh$anced")) {
			MpL.title.noenhanced = false;
			rC.CToken++;
			continue;
		}
		if(rC.AlmostEq("noenh$anced")) {
			MpL.title.noenhanced = true;
			rC.CToken++;
			continue;
		}
		if(rC.AlmostEq("lay$out")) {
			if(MpL.auto_layout)
				IntErrorCurToken("too many layout commands");
			else
				MpL.auto_layout = true;

			rC.CToken++;
			if(rC.EndOfCommand()) {
				IntErrorCurToken("expecting '<num_cols>,<num_rows>'");
			}

			/* read row,col */
			MpL.num_rows = rC.IntExpression();
			if(rC.EndOfCommand() || !rC.Eq(",") )
				IntErrorCurToken("expecting ', <num_cols>'");

			rC.CToken++;
			if(rC.EndOfCommand())
				IntErrorCurToken("expecting <num_cols>");
			MpL.num_cols = rC.IntExpression();

			/* remember current values of the plot size and the margins */
			MpL.PrevSz.x = XSz;
			MpL.PrevSz.y = YSz;
			MpL.PrefOffs.x = XOffs;
			MpL.PrefOffs.y = YOffs;
			MpL.prev_LMrg = LMrg;
			MpL.prev_RMrg = RMrg;
			MpL.prev_BMrg = BMrg;
			MpL.prev_TMrg = TMrg;

			MpL.act_row = 0;
			MpL.act_col = 0;
			continue;
		}
		/* The remaining options are only valid for auto-layout mode */
		if(!MpL.auto_layout)
			IntErrorCurToken("only valid in the context of an auto-layout command");
		switch(rC.LookupTable(&set_multiplot_tbl[0], rC.CToken)) {
			case S_MULTIPLOT_COLUMNSFIRST:
			    MpL.row_major = true;
			    rC.CToken++;
			    break;
			case S_MULTIPLOT_ROWSFIRST:
			    MpL.row_major = false;
			    rC.CToken++;
			    break;
			case S_MULTIPLOT_DOWNWARDS:
			    MpL.downwards = true;
			    rC.CToken++;
			    break;
			case S_MULTIPLOT_UPWARDS:
			    MpL.downwards = false;
			    rC.CToken++;
			    break;
			case S_MULTIPLOT_SCALE:
			    rC.CToken++;
			    MpL.Scale.Set(rC.RealExpression());
			    if(!rC.EndOfCommand() && rC.Eq(",") ) {
				    rC.CToken++;
				    if(rC.EndOfCommand()) {
					    IntErrorCurToken("expecting <yscale>");
				    }
				    MpL.Scale.y = rC.RealExpression();
			    }
			    break;
			case S_MULTIPLOT_OFFSET:
			    rC.CToken++;
			    MpL.Offs.Set(rC.RealExpression());
			    if(!rC.EndOfCommand() && rC.Eq(",") ) {
				    rC.CToken++;
				    if(rC.EndOfCommand()) {
					    IntErrorCurToken("expecting <yoffset>");
				    }
				    MpL.Offs.y = rC.RealExpression();
			    }
			    break;
			case S_MULTIPLOT_MARGINS:
			    rC.CToken++;
			    if(rC.EndOfCommand())
				    IntErrorCurToken("expecting '<left>,<right>,<bottom>,<top>'");

			    MpL.LMrg.scalex = screen;
			    mp_layout_set_margin_or_spacing(rC, &(MpL.LMrg));
			    if(!rC.EndOfCommand() && rC.Eq(",") ) {
				    rC.CToken++;
				    if(rC.EndOfCommand())
					    IntErrorCurToken("expecting <right>");

				    MpL.RMrg.scalex = MpL.LMrg.scalex;
				    mp_layout_set_margin_or_spacing(rC, &(MpL.RMrg));
			    }
			    else {
				    IntErrorCurToken("expecting <right>");
			    }
			    if(!rC.EndOfCommand() && rC.Eq(",") ) {
				    rC.CToken++;
				    if(rC.EndOfCommand())
					    IntErrorCurToken("expecting <top>");

				    MpL.BMrg.scalex = MpL.RMrg.scalex;
				    mp_layout_set_margin_or_spacing(rC, &(MpL.BMrg));
			    }
			    else {
				    IntErrorCurToken("expecting <bottom>");
			    }
			    if(!rC.EndOfCommand() && rC.Eq(",") ) {
				    rC.CToken++;
				    if(rC.EndOfCommand())
					    IntErrorCurToken("expecting <bottom>");

				    MpL.TMrg.scalex = MpL.BMrg.scalex;
				    mp_layout_set_margin_or_spacing(rC, &(MpL.TMrg));
			    }
			    else {
				    IntErrorCurToken("expection <top>");
			    }
			    set_margins = true;
			    break;
			case S_MULTIPLOT_SPACING:
			    rC.CToken++;
			    if(rC.EndOfCommand())
				    IntErrorCurToken("expecting '<xspacing>,<yspacing>'");
			    MpL.xspacing.scalex = screen;
			    mp_layout_set_margin_or_spacing(rC, &(MpL.xspacing));
			    MpL.yspacing = MpL.xspacing;

			    if(!rC.EndOfCommand() && rC.Eq(",")) {
				    rC.CToken++;
				    if(rC.EndOfCommand())
					    IntErrorCurToken("expecting <yspacing>");
				    mp_layout_set_margin_or_spacing(rC, &(MpL.yspacing));
			    }
			    set_spacing = true;
			    break;
			default:
			    IntErrorCurToken("invalid or duplicate option");
			    break;
		}
	}
	if(set_spacing || set_margins) {
		if(set_spacing && set_margins) {
			if(MpL.LMrg.x >= 0 && MpL.RMrg.x >= 0 && MpL.TMrg.x >= 0 && MpL.BMrg.x >= 0 && MpL.xspacing.x >= 0 && MpL.yspacing.x >= 0)
				MpL.auto_layout_margins = true;
			else
				GpGg.IntErrorNoCaret("must give positive margin and spacing values");
		}
		else if(set_spacing) {
			int_warn(NO_CARET, "must give margins and spacing, continue with auto margins.");
		}
		else if(set_margins) {
			MpL.auto_layout_margins = true;
			MpL.xspacing.scalex = screen;
			MpL.xspacing.x = 0.05;
			MpL.yspacing.scalex = screen;
			MpL.yspacing.x = 0.05;
			int_warn(NO_CARET, "must give margins and spacing, continue with spacing of 0.05");
		}
		// Sanity check that screen TMrg is > screen BMrg 
		if(MpL.BMrg.scalex == screen && MpL.TMrg.scalex == screen)
			if(MpL.BMrg.x > MpL.TMrg.x) {
				Exchange(&MpL.BMrg.x, &MpL.TMrg.x);
			}
	}
	//
	// If we reach here, then the command has been successfully parsed.
	// Aug 2013: call term_start_plot() before setting multiplot so that
	// the wxt and qt terminals will reset the plot count to 0 before
	// ignoring subsequent TERM_LAYER_RESET requests.
	// 
	TermStartPlot(pT);
	IsMultiPlot = true;
	Ev.FillGpValInteger("GPVAL_MULTIPLOT", 1);
	// Place overall title before doing anything else
	if(MpL.title.text) {
		double tmpx, tmpy;
		uint x, y;
		char * p = MpL.title.text;
		MapPositionR(MpL.title.offset, &tmpx, &tmpy, "mp title");
		x = (uint)(pT->xmax / 2 + tmpx);
		y = (uint)(pT->ymax - pT->VChr + tmpy);
		ignore_enhanced(MpL.title.noenhanced);
		ApplyPm3DColor(pT, &(MpL.title.textcolor));
		pT->DrawMultiline(x, y, MpL.title.text, CENTRE, JUST_TOP, 0, MpL.title.font);
		reset_textcolor(&(MpL.title.textcolor));
		ignore_enhanced(false);
		//
		// Calculate fractional height of title compared to entire page
		// If it would fill the whole page, forget it!
		for(y = 1; *p; p++)
			if(*p == '\n')
				y++;
		// Oct 2012 - VChr depends on the font used
		if(MpL.title.font && *MpL.title.font)
			pT->set_font(MpL.title.font);
		MpL.title_height = (double)(y * pT->VChr) / (double)pT->ymax;
		if(MpL.title.font && *MpL.title.font)
			pT->set_font("");
		if(MpL.title_height > 0.9)
			MpL.title_height = 0.05;
	}
	else {
		MpL.title_height = 0.0;
	}
	if(MpL.auto_layout_margins)
		MpLayoutMarginsAndSpacing(pT);
	else
		MpLayoutSizeAndOffset();
}

void GpGadgets::MultiplotEnd()
{
	IsMultiPlot = false;
	Ev.FillGpValInteger("GPVAL_MULTIPLOT", 0);
	// reset plot size, origin and margins to values before 'set multiplot layout' 
	if(MpL.auto_layout) {
		XSz = (float)MpL.PrevSz.x;
		YSz = (float)MpL.PrevSz.y;
		XOffs = (float)MpL.PrefOffs.x;
		YOffs = (float)MpL.PrefOffs.y;
		LMrg = MpL.prev_LMrg;
		RMrg = MpL.prev_RMrg;
		BMrg = MpL.prev_BMrg;
		TMrg = MpL.prev_TMrg;
	}
	// reset automatic multiplot layout 
	MpL.auto_layout = false;
	MpL.auto_layout_margins = false;
	MpL.Scale.Set(1.0);
	MpL.Offs.Set(0.0);
	MpL.LMrg.scalex = MpL.RMrg.scalex = screen;
	MpL.BMrg.scalex = MpL.TMrg.scalex = screen;
	MpL.LMrg.x = MpL.RMrg.x = MpL.BMrg.x = MpL.TMrg.x = -1;
	MpL.xspacing.scalex = MpL.yspacing.scalex = screen;
	MpL.xspacing.x = MpL.yspacing.x = -1;
	ZFREE(MpL.title.text);
}
//
// Helper function for multiplot auto layout to issue size and offset cmds 
//
//static void mp_layout_size_and_offset()
void GpGadgets::MpLayoutSizeAndOffset()
{
	if(MpL.auto_layout) {
		// fprintf(stderr,"col==%d row==%d\n",MpL.act_col,MpL.act_row); 
		// the 'set size' command 
		XSz = (float)(MpL.Scale.x / MpL.num_cols);
		YSz = (float)(MpL.Scale.y / MpL.num_rows);
		// the 'set origin' command 
		XOffs = (float)((double)(MpL.act_col) / MpL.num_cols);
		if(MpL.downwards)
			YOffs = (float)(1.0 - (double)(MpL.act_row+1) / MpL.num_rows);
		else
			YOffs = (float)((double)(MpL.act_row) / MpL.num_rows);
		// fprintf(stderr,"xoffset==%g  yoffset==%g\n", xoffset,yoffset); 
		// Allow a little space at the top for a title 
		if(MpL.title.text) {
			YSz   *= (float)(1.0 - MpL.title_height);
			YOffs *= (float)(1.0 - MpL.title_height);
		}
		// corrected for x/y-scaling factors and user defined offsets 
		XOffs -= (float)((MpL.Scale.x-1)/(2*MpL.num_cols));
		YOffs -= (float)((MpL.Scale.y-1)/(2*MpL.num_rows));
		// fprintf(stderr,"  xoffset==%g  yoffset==%g\n", xoffset,yoffset); 
		XOffs += (float)MpL.Offs.x;
		YOffs += (float)MpL.Offs.y;
		// fprintf(stderr,"  xoffset==%g  yoffset==%g\n", xoffset,yoffset); 
	}
}
//
// Helper function for multiplot auto layout to set the explicit plot margins,
// if requested with 'margins' and 'spacing' options.
//
//static void mp_layout_margins_and_spacing(void)
void GpGadgets::MpLayoutMarginsAndSpacing(GpTermEntry * pT)
{
	// width and height of a single sub plot
	double tmp_width, tmp_height;
	double lefTMrg, righTMrg, topmargin, bottommargin, xspacing, yspacing;
	if(MpL.auto_layout_margins) {
		lefTMrg = (MpL.LMrg.scalex == screen) ? MpL.LMrg.x : (MpL.LMrg.x * pT->HChr) / pT->xmax;
		righTMrg = (MpL.RMrg.scalex == screen) ? MpL.RMrg.x : (1 - (MpL.RMrg.x * pT->HChr) / pT->xmax);
		topmargin = (MpL.TMrg.scalex == screen) ? MpL.TMrg.x : (1 - (MpL.TMrg.x * pT->VChr) / pT->ymax);
		bottommargin = (MpL.BMrg.scalex == screen) ? MpL.BMrg.x : (MpL.BMrg.x * pT->VChr) / pT->ymax;
		xspacing = (MpL.xspacing.scalex == screen) ? MpL.xspacing.x : (MpL.xspacing.x * pT->HChr) / pT->xmax;
		yspacing = (MpL.yspacing.scalex == screen) ? MpL.yspacing.x : (MpL.yspacing.x * pT->VChr) / pT->ymax;
		tmp_width = (righTMrg - lefTMrg - (MpL.num_cols - 1) * xspacing) / MpL.num_cols;
		tmp_height = (topmargin - bottommargin - (MpL.num_rows - 1) * yspacing) / MpL.num_rows;
		LMrg.x = lefTMrg + MpL.act_col * (tmp_width + xspacing);
		LMrg.scalex = screen;
		RMrg.x = LMrg.x + tmp_width;
		RMrg.scalex = screen;
		if(MpL.downwards) {
			BMrg.x = bottommargin + (MpL.num_rows - MpL.act_row - 1) * (tmp_height + yspacing);
		}
		else {
			BMrg.x = bottommargin + MpL.act_row * (tmp_height + yspacing);
		}
		BMrg.scalex = screen;
		TMrg.x = BMrg.x + tmp_height;
		TMrg.scalex = screen;
	}
}

static void mp_layout_set_margin_or_spacing(GpCommand & rC, GpPosition * margin)
{
	margin->x = -1;
	if(!rC.EndOfCommand()) {
		if(rC.AlmostEq("sc$reen")) {
			margin->scalex = screen;
			rC.CToken++;
		}
		else if(rC.AlmostEq("char$acter")) {
			margin->scalex = character;
			rC.CToken++;
		}
		margin->x = rC.RealExpression();
		if(margin->x < 0)
			margin->x = -1;
		if(margin->scalex == screen) {
			if(margin->x < 0)
				margin->x = 0;
			if(margin->x > 1)
				margin->x = 1;
		}
	}
}

