// JITTER.C
// Copyright Ethan A Merritt 2015
// 
// This file contains routines used to support the "set jitter" option.
// This plot mode was inspired by the appearance of "beeswarm" plots produced
// by R, but I do not know to what extent the algorithms used are the same.
// 
#include <gnuplot.h>
#pragma hdrstop

//t_jitter jitter; // = {{first_axes, first_axes, first_axes, 0.0, 0.0, 0.0}, 0.0, 0.0, JITTER_DEFAULT};

static int compare_xypoints(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	//const GpCoordinate * p1 = static_cast<const GpCoordinate *>(arg1);
	//const GpCoordinate * p2 = static_cast<const GpCoordinate *>(arg2);
	// Primary sort is on x */
	// FIXME: I'd like to treat x coords within jitter.x as equal, 
	//        but the coordinate system mismatch makes this hard.  
	RET_CMPCASCADE2(&static_cast<const GpCoordinate *>(arg1)->Pt, &static_cast<const GpCoordinate *>(arg2)->Pt, x, y);
	/*if(p1->x > p2->x)
		return 1;
	else if(p1->x < p2->x)
		return -1;
	else if(p1->y > p2->y)
		return 1;
	else if(p1->y < p2->y)
		return -1;
	else
		return 0;*/
}
// 
// "set jitter overlap <ydelta> spread <factor>"
// displaces overlapping points in a point plot.
// The jittering algorithm is inspired by the beeswarm plot variant in R.
// 
//static double jdist(const GpCoordinate * pi, const GpCoordinate * pj)
double GnuPlot::JDist(const GpCoordinate * pi, const GpCoordinate * pj)
{
	int delx = MapiX(pi->Pt.x) - MapiX(pj->Pt.x);
	int dely = MapiY(pi->Pt.y) - MapiY(pj->Pt.y);
	return sqrt(delx*delx + dely*dely);
}

//void jitter_points(const GpTermEntry * pTerm, curve_points * pPlot)
void GnuPlot::JitterPoints(const GpTermEntry * pTerm, curve_points * pPlot)
{
	int i, j;
	// The "x" and "xscale" stored in jitter are really along y 
	double xjit, ygap;
	GpPosition yoverlap;
	yoverlap.x = 0;
	yoverlap.y = Jitter.overlap.x;
	yoverlap.scaley = Jitter.overlap.scalex;
	MapPositionR(pTerm, &yoverlap, &xjit, &ygap, "jitter");
	// Clear data slots where we will later store the jitter offsets.
	// Store variable color temporarily in z so it is not lost by sorting.
	for(i = 0; i < pPlot->p_count; i++) {
		if(pPlot->varcolor)
			pPlot->points[i].Pt.z = pPlot->varcolor[i];
		pPlot->points[i].CRD_XJITTER = 0.0;
		pPlot->points[i].CRD_YJITTER = 0.0;
	}
	// Sort points 
	qsort(pPlot->points, pPlot->p_count, sizeof(GpCoordinate), compare_xypoints);
	// For each point, check whether subsequent points would overlap it. 
	// If so, displace them in a fixed pattern 
	i = 0;
	while(i < pPlot->p_count - 1) {
		for(j = 1; i+j < pPlot->p_count; j++) {
			if(JDist(&pPlot->points[i], &pPlot->points[i+j]) >= ygap)
				break;
			// Displace point purely on x 
			xjit  = (j+1)/2 * Jitter.spread * pPlot->lp_properties.PtSize;
			if(Jitter.limit > 0)
				while(xjit > Jitter.limit)
					xjit -= Jitter.limit;
			if((j & 01) != 0)
				xjit = -xjit;
			pPlot->points[i+j].CRD_XJITTER = xjit;
			if(Jitter.style == JITTER_SQUARE)
				pPlot->points[i+j].CRD_YJITTER = pPlot->points[i].Pt.y - pPlot->points[i+j].Pt.y;
			// Displace points on y instead of x 
			if(Jitter.style == JITTER_ON_Y) {
				pPlot->points[i+j].CRD_YJITTER = xjit;
				pPlot->points[i+j].CRD_XJITTER = 0;
			}
		}
		i += j;
	}
	// Copy variable colors back to where the plotting code expects to find them 
	if(pPlot->varcolor) {
		for(i = 0; i < pPlot->p_count; i++)
			pPlot->varcolor[i] = pPlot->points[i].Pt.z;
	}
}
//
// process 'set jitter' command 
//
//void set_jitter()
void GnuPlot::SetJitter()
{
	Pgm.Shift();
	// Default overlap criterion 1 character (usually on y) 
	Jitter.overlap.scalex = character;
	Jitter.overlap.x = 1;
	Jitter.spread = 1.0;
	Jitter.limit = 0.0;
	Jitter.style = JITTER_DEFAULT;
	if(Pgm.EndOfCommand())
		return;
	while(!Pgm.EndOfCommand()) {
		if(Pgm.AlmostEqualsCur("over$lap")) {
			Pgm.Shift();
			GetPositionDefault(&Jitter.overlap, character, 2);
		}
		else if(Pgm.EqualsCur("spread")) {
			Pgm.Shift();
			Jitter.spread = RealExpression();
			if(Jitter.spread <= 0)
				Jitter.spread = 1.0;
		}
		else if(Pgm.EqualsCur("swarm")) {
			Pgm.Shift();
			Jitter.style = JITTER_SWARM;
		}
		else if(Pgm.EqualsCur("square")) {
			Pgm.Shift();
			Jitter.style = JITTER_SQUARE;
		}
		else if(Pgm.EqualsCur("wrap")) {
			Pgm.Shift();
			Jitter.limit = RealExpression();
		}
		else if(Pgm.AlmostEqualsCur("vert$ical")) {
			Pgm.Shift();
			Jitter.style = JITTER_ON_Y;
		}
		else
			IntErrorCurToken("unrecognized keyword");
	}
}
//
// process 'show jitter' command 
//
//void show_jitter()
void GnuPlot::ShowJitter()
{
	if(Jitter.spread <= 0) {
		fprintf(stderr, "\tno jitter\n");
		return;
	}
	fprintf(stderr, "\toverlap criterion  %g %s coords\n", Jitter.overlap.x, coord_msg[Jitter.overlap.scalex]);
	fprintf(stderr, "\tspread multiplier on x (or y): %g\n", Jitter.spread);
	if(Jitter.limit > 0)
		fprintf(stderr, "\twrap at %g character widths\n", Jitter.limit);
	fprintf(stderr, "\tstyle: %s\n", Jitter.style == JITTER_SQUARE ? "square" : Jitter.style == JITTER_ON_Y ? "vertical" : "swarm");
}
//
// process 'unset jitter' command 
//
//void unset_jitter()
void GnuPlot::UnsetJitter()
{
	Jitter.spread = 0;
}
//
// called by the save command 
//
//void save_jitter(FILE * fp)
void GnuPlot::SaveJitter(FILE * fp)
{
	if(Jitter.spread <= 0)
		fprintf(fp, "unset jitter\n");
	else {
		fprintf(fp, "set jitter overlap %s%g", Jitter.overlap.scalex == character ? "" : coord_msg[Jitter.overlap.scalex], Jitter.overlap.x);
		fprintf(fp, "  spread %g  wrap %g", Jitter.spread, Jitter.limit);
		fprintf(fp, Jitter.style == JITTER_SQUARE ? " square\n" : Jitter.style == JITTER_ON_Y ? " vertical\n" : "\n");
	}
}
