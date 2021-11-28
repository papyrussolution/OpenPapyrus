// GNUPLOT - term.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * Bookkeeping and support routines for 'set multiplot layout ...'
 * Jul 2004 Volker Dobler     layout rows, columns
 * Feb 2013 Christoph Bersch  layout margins spacing
 * Mar 2014 Ethan A Merritt   refactor into separate file (used to be in term.c)
 */
#include <gnuplot.h>
#pragma hdrstop

enum set_multiplot_id {
	S_MULTIPLOT_LAYOUT,
	S_MULTIPLOT_COLUMNSFIRST, S_MULTIPLOT_ROWSFIRST, S_MULTIPLOT_SCALE,
	S_MULTIPLOT_DOWNWARDS, S_MULTIPLOT_UPWARDS,
	S_MULTIPLOT_OFFSET, S_MULTIPLOT_TITLE,
	S_MULTIPLOT_MARGINS, S_MULTIPLOT_SPACING,
	S_MULTIPLOT_INVALID
};

static const struct gen_table set_multiplot_tbl[] =
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

//#define MP_LAYOUT_DEFAULT {          \
		//FALSE, /* auto_layout */         \
		//0, /* current_panel */       \
		//0, 0, /* num_rows, num_cols */  \
		//FALSE, /* row_major */           \
		//TRUE, /* downwards */           \
		//0, 0, /* act_row, act_col */    \
		//1, 1, /* xscale, yscale */      \
		//0, 0, /* xoffset, yoffset */    \
		//FALSE, /* auto_layout_margins */ \
		//{screen, screen, screen, 0.1, -1, -1}, /* lmargin */ \
		//{screen, screen, screen, 0.9, -1, -1}, /* rmargin */ \
		//{screen, screen, screen, 0.1, -1, -1}, /* bmargin */ \
		//{screen, screen, screen, 0.9, -1, -1}, /* tmargin */ \
		//{screen, screen, screen, 0.05, -1, -1}, /* xspacing */ \
		//{screen, screen, screen, 0.05, -1, -1}, /* yspacing */ \
		//0, 0, 0, 0, /* prev_ sizes and offsets */ \
		//DEFAULT_MARGIN_POSITION, DEFAULT_MARGIN_POSITION, DEFAULT_MARGIN_POSITION, \
		//DEFAULT_MARGIN_POSITION, /* prev_ margins */ EMPTY_LABELSTRUCT, 0.0 }
//
// Helper routines 
//
//void multiplot_next()
void GnuPlot::MultiplotNext()
{
	MpLo.current_panel++;
	if(MpLo.auto_layout) {
		if(MpLo.row_major) {
			MpLo.act_row++;
			if(MpLo.act_row == MpLo.num_rows) {
				MpLo.act_row = 0;
				MpLo.act_col++;
				if(MpLo.act_col == MpLo.num_cols) {
					// IntWarn(NO_CARET,"will overplot first plot"); 
					MpLo.act_col = 0;
				}
			}
		}
		else { // column-major 
			MpLo.act_col++;
			if(MpLo.act_col == MpLo.num_cols) {
				MpLo.act_col = 0;
				MpLo.act_row++;
				if(MpLo.act_row == MpLo.num_rows) {
					// IntWarn(NO_CARET,"will overplot first plot"); 
					MpLo.act_row = 0;
				}
			}
		}
		MultiplotReset();
	}
}

//void multiplot_previous()
void GnuPlot::MultiplotPrevious()
{
	MpLo.current_panel--;
	if(MpLo.auto_layout) {
		if(MpLo.row_major) {
			MpLo.act_row--;
			if(MpLo.act_row < 0) {
				MpLo.act_row = MpLo.num_rows-1;
				MpLo.act_col--;
				if(MpLo.act_col < 0) {
					// IntWarn(NO_CARET,"will overplot first plot"); 
					MpLo.act_col = MpLo.num_cols-1;
				}
			}
		}
		else { // column-major 
			MpLo.act_col--;
			if(MpLo.act_col < 0) {
				MpLo.act_col = MpLo.num_cols-1;
				MpLo.act_row--;
				if(MpLo.act_row < 0) {
					// IntWarn(NO_CARET,"will overplot first plot"); 
					MpLo.act_row = MpLo.num_rows-1;
				}
			}
		}
		MultiplotReset();
	}
}

//int multiplot_current_panel()
int GnuPlot::GetMultiplotCurrentPanel() const
{
	return MpLo.current_panel;
}

//void multiplot_start()
void GnuPlot::MultiplotStart(GpTermEntry * pTerm)
{
	bool set_spacing = FALSE;
	bool set_margins = FALSE;
	Pgm.Shift();
	// Only a few options are possible if we are already in multiplot mode 
	// So far we have "next".  Maybe also "previous", "clear"? 
	if(GPT.Flags & GpTerminalBlock::fMultiplot) {
		if(Pgm.EqualsCur("next")) {
			Pgm.Shift();
			if(!MpLo.auto_layout)
				IntErrorCurToken("only valid inside an auto-layout multiplot");
			MultiplotNext();
			return;
		}
		else if(Pgm.AlmostEqualsCur("prev$ious")) {
			Pgm.Shift();
			if(!MpLo.auto_layout)
				IntErrorCurToken("only valid inside an auto-layout multiplot");
			MultiplotPrevious();
			return;
		}
		else {
			TermEndMultiplot(pTerm);
		}
	}
	// FIXME: more options should be reset/initialized each time 
	MpLo.auto_layout = FALSE;
	MpLo.auto_layout_margins = FALSE;
	MpLo.current_panel = 0;
	MpLo.title.noenhanced = FALSE;
	ZFREE(MpLo.title.text);
	ZFREE(MpLo.title.font);
	MpLo.title.boxed = 0;
	// Parse options 
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("ti$tle")) {
			Pgm.Shift();
			ParseLabelOptions(&MpLo.title, 2);
			if(!Pgm.EndOfCommand())
				MpLo.title.text = TryToGetString();
			ParseLabelOptions(&MpLo.title, 2);
			continue;
		}
		if(Pgm.AlmostEqualsCur("lay$out")) {
			if(MpLo.auto_layout)
				IntErrorCurToken("too many layout commands");
			else
				MpLo.auto_layout = TRUE;
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("expecting '<num_cols>,<num_rows>'");
			// read row,col 
			MpLo.num_rows = IntExpression();
			if(Pgm.EndOfCommand() || !Pgm.EqualsCur(",") )
				IntErrorCurToken("expecting ', <num_cols>'");
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				IntErrorCurToken("expecting <num_cols>");
			MpLo.num_cols = IntExpression();
			// remember current values of the plot size and the margins 
			MpLo.PrevSize.x = V.Size.x;
			MpLo.PrevSize.y = V.Size.y;
			MpLo.PrevOffset = V.Offset;
			MpLo.prev_lmargin = V.MarginL;
			MpLo.prev_rmargin = V.MarginR;
			MpLo.prev_bmargin = V.MarginB;
			MpLo.prev_tmargin = V.MarginT;
			MpLo.act_row = 0;
			MpLo.act_col = 0;
			continue;
		}
		// The remaining options are only valid for auto-layout mode 
		if(!MpLo.auto_layout)
			IntErrorCurToken("only valid in the context of an auto-layout command");
		switch(Pgm.LookupTableForCurrentToken(&set_multiplot_tbl[0])) {
			case S_MULTIPLOT_COLUMNSFIRST:
			    MpLo.row_major = TRUE;
			    Pgm.Shift();
			    break;
			case S_MULTIPLOT_ROWSFIRST:
			    MpLo.row_major = FALSE;
			    Pgm.Shift();
			    break;
			case S_MULTIPLOT_DOWNWARDS:
			    MpLo.downwards = TRUE;
			    Pgm.Shift();
			    break;
			case S_MULTIPLOT_UPWARDS:
			    MpLo.downwards = FALSE;
			    Pgm.Shift();
			    break;
			case S_MULTIPLOT_SCALE:
			    Pgm.Shift();
			    MpLo.xscale = RealExpression();
			    MpLo.yscale = MpLo.xscale;
			    if(!Pgm.EndOfCommand() && Pgm.EqualsCur(",") ) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand()) {
					    IntErrorCurToken("expecting <yscale>");
				    }
				    MpLo.yscale = RealExpression();
			    }
			    break;
			case S_MULTIPLOT_OFFSET:
			    Pgm.Shift();
				MpLo.Offset.Set(static_cast<float>(RealExpression()));
			    if(!Pgm.EndOfCommand() && Pgm.EqualsCur(",") ) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand()) {
					    IntErrorCurToken("expecting <yoffset>");
				    }
				    MpLo.Offset.y = static_cast<float>(RealExpression());
			    }
			    break;
			case S_MULTIPLOT_MARGINS:
			    Pgm.Shift();
			    if(Pgm.EndOfCommand())
				    IntErrorCurToken("expecting '<left>,<right>,<bottom>,<top>'");
			    MpLo.lmargin.scalex = screen;
			    MpLayoutSetMarginOrSpacing(&MpLo.lmargin);
			    if(!Pgm.EndOfCommand() && Pgm.EqualsCur(",") ) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand())
					    IntErrorCurToken("expecting <right>");
				    MpLo.rmargin.scalex = MpLo.lmargin.scalex;
				    MpLayoutSetMarginOrSpacing(&MpLo.rmargin);
			    }
			    else {
				    IntErrorCurToken("expecting <right>");
			    }
			    if(!Pgm.EndOfCommand() && Pgm.EqualsCur(",") ) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand())
					    IntErrorCurToken("expecting <top>");
				    MpLo.bmargin.scalex = MpLo.rmargin.scalex;
				    MpLayoutSetMarginOrSpacing(&MpLo.bmargin);
			    }
			    else {
				    IntErrorCurToken("expecting <bottom>");
			    }
			    if(!Pgm.EndOfCommand() && Pgm.EqualsCur(",") ) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand())
					    IntErrorCurToken("expecting <bottom>");
				    MpLo.tmargin.scalex = MpLo.bmargin.scalex;
				    MpLayoutSetMarginOrSpacing(&MpLo.tmargin);
			    }
			    else {
				    IntErrorCurToken("expecting <top>");
			    }
			    set_margins = TRUE;
			    break;
			case S_MULTIPLOT_SPACING:
			    Pgm.Shift();
			    if(Pgm.EndOfCommand())
				    IntErrorCurToken("expecting '<xspacing>,<yspacing>'");
			    MpLo.xspacing.scalex = screen;
			    MpLayoutSetMarginOrSpacing(&MpLo.xspacing);
			    MpLo.yspacing = MpLo.xspacing;
			    if(!Pgm.EndOfCommand() && Pgm.EqualsCur(",")) {
				    Pgm.Shift();
				    if(Pgm.EndOfCommand())
					    IntErrorCurToken("expecting <yspacing>");
				    MpLayoutSetMarginOrSpacing(&MpLo.yspacing);
			    }
			    set_spacing = TRUE;
			    break;
			default:
			    IntErrorCurToken("invalid or duplicate option");
			    break;
		}
	}
	if(set_spacing || set_margins) {
		if(set_spacing && set_margins) {
			if(MpLo.lmargin.x >= 0 && MpLo.rmargin.x >= 0 && MpLo.tmargin.x >= 0 && MpLo.bmargin.x >= 0 && MpLo.xspacing.x >= 0 && MpLo.yspacing.x >= 0)
				MpLo.auto_layout_margins = TRUE;
			else
				IntError(NO_CARET, "must give positive margin and spacing values");
		}
		else if(set_margins) {
			MpLo.auto_layout_margins = true;
			MpLo.xspacing.SetX(screen, 0.05);
			MpLo.yspacing.SetX(screen, 0.05);
		}
		// Sanity check that screen tmargin is > screen bmargin 
		if(MpLo.bmargin.scalex == screen && MpLo.tmargin.scalex == screen)
			if(MpLo.bmargin.x > MpLo.tmargin.x) {
				double tmp = MpLo.bmargin.x;
				MpLo.bmargin.x = MpLo.tmargin.x;
				MpLo.tmargin.x = tmp;
			}
	}
	// If we reach here, then the command has been successfully parsed.
	// Aug 2013: call term_start_plot() before setting multiplot so that
	// the wxt and qt terminals will reset the plot count to 0 before
	// ignoring subsequent TERM_LAYER_RESET requests.
	TermStartPlot(pTerm);
	GPT.Flags |= GpTerminalBlock::fMultiplot;
	GPT.MultiplotCount = 0;
	Ev.FillGpValInteger("GPVAL_MULTIPLOT", 1);
	// Place overall title before doing anything else 
	if(MpLo.title.text) {
		char * p = MpLo.title.text;
		uint x = pTerm->MaxX  / 2;
		uint y = pTerm->MaxY - pTerm->ChrV;
		WriteLabel(pTerm, x, y, &(MpLo.title));
		ResetTextColor(pTerm, &(MpLo.title.textcolor));
		// Calculate fractional height of title compared to entire page 
		// If it would fill the whole page, forget it! 
		for(y = 1; *p; p++)
			if(*p == '\n')
				y++;
		// Oct 2012 - ChrV depends on the font used 
		if(MpLo.title.font && *MpLo.title.font)
			pTerm->set_font(pTerm, MpLo.title.font);
		MpLo.title_height = (double)(y * pTerm->ChrV) / (double)pTerm->MaxY;
		if(MpLo.title.font && *MpLo.title.font)
			pTerm->set_font(pTerm, "");
		if(MpLo.title_height > 0.9)
			MpLo.title_height = 0.05;
	}
	else
		MpLo.title_height = 0.0;
	MultiplotReset();
}

//void multiplot_end()
void GnuPlot::MultiplotEnd()
{
	GPT.Flags &= ~GpTerminalBlock::fMultiplot;
	GPT.MultiplotCount = 0;
	Ev.FillGpValInteger("GPVAL_MULTIPLOT", 0);
	// reset plot size, origin and margins to values before 'set multiplot layout' 
	if(MpLo.auto_layout) {
		V.Size.x  = MpLo.PrevSize.x;
		V.Size.y  = MpLo.PrevSize.y;
		V.Offset  = MpLo.PrevOffset;
		V.MarginL = MpLo.prev_lmargin;
		V.MarginR = MpLo.prev_rmargin;
		V.MarginB = MpLo.prev_bmargin;
		V.MarginT = MpLo.prev_tmargin;
	}
	// reset automatic multiplot layout 
	MpLo.auto_layout = FALSE;
	MpLo.auto_layout_margins = FALSE;
	MpLo.xscale = MpLo.yscale = 1.0;
	MpLo.Offset.SetZero();
	MpLo.lmargin.scalex = MpLo.rmargin.scalex = screen;
	MpLo.bmargin.scalex = MpLo.tmargin.scalex = screen;
	MpLo.lmargin.x = MpLo.rmargin.x = MpLo.bmargin.x = MpLo.tmargin.x = -1;
	MpLo.xspacing.scalex = MpLo.yspacing.scalex = screen;
	MpLo.xspacing.x = MpLo.yspacing.x = -1;
	ZFREE(MpLo.title.text);
}
//
// Helper function for multiplot auto layout to issue size and offset cmds 
//
//void multiplot_reset()
void GnuPlot::MultiplotReset()
{
	if(MpLo.auto_layout_margins)
		MpLayoutMarginsAndSpacing(GPT.P_Term);
	else
		MpLayoutSizeAndOffset();
}

//static void mp_layout_size_and_offset()
void GnuPlot::MpLayoutSizeAndOffset()
{
	if(MpLo.auto_layout) {
		// fprintf(stderr, "col==%d row==%d\n",MpLo.act_col,MpLo.act_row); 
		// the 'set size' command 
		V.Size.x = static_cast<float>(MpLo.xscale / MpLo.num_cols);
		V.Size.y = static_cast<float>(MpLo.yscale / MpLo.num_rows);
		// the 'set origin' command 
		V.Offset.x = static_cast<float>(static_cast<double>(MpLo.act_col) / MpLo.num_cols);
		if(MpLo.downwards)
			V.Offset.y = static_cast<float>(1.0 - static_cast<double>(MpLo.act_row+1) / MpLo.num_rows);
		else
			V.Offset.y = static_cast<float>(static_cast<double>(MpLo.act_row) / MpLo.num_rows);
		// fprintf(stderr, "xoffset==%g  yoffset==%g\n", xoffset,yoffset); 
		// Allow a little space at the top for a title 
		if(MpLo.title.text) {
			V.Size.y   *= (1.0 - MpLo.title_height);
			V.Offset.y *= (1.0 - MpLo.title_height);
		}
		// corrected for x/y-scaling factors and user defined offsets 
		V.Offset.x -= (MpLo.xscale-1)/(2*MpLo.num_cols);
		V.Offset.y -= (MpLo.yscale-1)/(2*MpLo.num_rows);
		// fprintf(stderr, "  xoffset==%g  yoffset==%g\n", xoffset,yoffset); 
		V.Offset.x += MpLo.Offset.x;
		V.Offset.y += MpLo.Offset.y;
		// fprintf(stderr, "  xoffset==%g  yoffset==%g\n", xoffset,yoffset); 
	}
}
// 
// Helper function for multiplot auto layout to set the explicit plot margins,
// if requested with 'margins' and 'spacing' options. 
// 
//static void mp_layout_margins_and_spacing()
void GnuPlot::MpLayoutMarginsAndSpacing(GpTermEntry * pTerm)
{
	// width and height of a single sub plot. 
	double tmp_width, tmp_height;
	double leftmargin, rightmargin, topmargin, bottommargin, xspacing, yspacing;
	if(MpLo.auto_layout_margins) {
		leftmargin   = (MpLo.lmargin.scalex == screen) ? MpLo.lmargin.x : ((MpLo.lmargin.x * pTerm->ChrH) / pTerm->MaxX);
		rightmargin  = (MpLo.rmargin.scalex == screen) ? MpLo.rmargin.x : (1 - (MpLo.rmargin.x * pTerm->ChrH) / pTerm->MaxX);
		topmargin    = (MpLo.tmargin.scalex == screen) ? MpLo.tmargin.x : (1 - (MpLo.tmargin.x * pTerm->ChrV) / pTerm->MaxY);
		bottommargin = (MpLo.bmargin.scalex == screen) ? MpLo.bmargin.x : ((MpLo.bmargin.x * pTerm->ChrV) / pTerm->MaxY);
		xspacing     = (MpLo.xspacing.scalex == screen) ? MpLo.xspacing.x : ((MpLo.xspacing.x * pTerm->ChrH) / pTerm->MaxX);
		yspacing     = (MpLo.yspacing.scalex == screen) ? MpLo.yspacing.x : (MpLo.yspacing.x * pTerm->ChrV) / pTerm->MaxY;
		tmp_width  = (rightmargin - leftmargin - (MpLo.num_cols - 1) * xspacing) / MpLo.num_cols;
		tmp_height = (topmargin - bottommargin - (MpLo.num_rows - 1) * yspacing) / MpLo.num_rows;
		V.MarginL.x = leftmargin + MpLo.act_col * (tmp_width + xspacing);
		V.MarginL.scalex = screen;
		V.MarginR.x = V.MarginL.x + tmp_width;
		V.MarginR.scalex = screen;
		if(MpLo.downwards) {
			V.MarginB.x = bottommargin + (MpLo.num_rows - MpLo.act_row - 1) * (tmp_height + yspacing);
		}
		else {
			V.MarginB.x = bottommargin + MpLo.act_row * (tmp_height + yspacing);
		}
		V.MarginB.scalex = screen;
		V.MarginT.x = V.MarginB.x + tmp_height;
		V.MarginT.scalex = screen;
	}
}

//static void mp_layout_set_margin_or_spacing(GpPosition * pMargin)
void GnuPlot::MpLayoutSetMarginOrSpacing(GpPosition * pMargin)
{
	pMargin->x = -1;
	if(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("sc$reen")) {
			pMargin->scalex = screen;
			Pgm.Shift();
		}
		else if(Pgm.AlmostEqualsCur("char$acter")) {
			pMargin->scalex = character;
			Pgm.Shift();
		}
		pMargin->x = RealExpression();
		if(pMargin->x < 0)
			pMargin->x = -1;
		if(pMargin->scalex == screen) {
			if(pMargin->x < 0)
				pMargin->x = 0;
			if(pMargin->x > 1)
				pMargin->x = 1;
		}
	}
}
