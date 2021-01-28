// JITTER.C
// Copyright Ethan A Merritt 2015
// 
// This file contains routines used to support the "set jitter" option.
// This plot mode was inspired by the appearance of "beeswarm" plots produced
// by R, but I do not know to what extent the algorithms used are the same.
// 
#include <gnuplot.h>
#pragma hdrstop

t_jitter jitter = {{first_axes, first_axes, first_axes, 0.0, 0.0, 0.0}, 0.0, 0.0, JITTER_DEFAULT};

static int compare_xypoints(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	struct coordinate const * p1 = (struct coordinate const*)arg1;
	struct coordinate const * p2 = (struct coordinate const*)arg2;
	/* Primary sort is on x */
	/* FIXME: I'd like to treat x coords within jitter.x as equal, */
	/*        but the coordinate system mismatch makes this hard.  */
	if(p1->x > p2->x)
		return (1);
	if(p1->x < p2->x)
		return (-1);
	if(p1->y > p2->y)
		return (1);
	if(p1->y < p2->y)
		return (-1);
	return (0);
}
// 
// "set jitter overlap <ydelta> spread <factor>"
// displaces overlapping points in a point plot.
// The jittering algorithm is inspired by the beeswarm plot variant in R.
// 
static double jdist(coordinate * pi, coordinate * pj)
{
	int delx = GPO.AxS.MapiX(pi->x) - GPO.AxS.MapiX(pj->x);
	int dely = GPO.AxS.MapiY(pi->y) - GPO.AxS.MapiY(pj->y);
	return sqrt(delx*delx + dely*dely);
}

void jitter_points(curve_points * plot)
{
	int i, j;
	// The "x" and "xscale" stored in jitter are really along y 
	double xjit, ygap;
	GpPosition yoverlap;
	yoverlap.x = 0;
	yoverlap.y = jitter.overlap.x;
	yoverlap.scaley = jitter.overlap.scalex;
	GPO.MapPositionR(term, &yoverlap, &xjit, &ygap, "jitter");
	// Clear data slots where we will later store the jitter offsets.
	// Store variable color temporarily in z so it is not lost by sorting.
	for(i = 0; i < plot->p_count; i++) {
		if(plot->varcolor)
			plot->points[i].z = plot->varcolor[i];
		plot->points[i].CRD_XJITTER = 0.0;
		plot->points[i].CRD_YJITTER = 0.0;
	}
	// Sort points 
	qsort(plot->points, plot->p_count, sizeof(struct coordinate), compare_xypoints);
	// For each point, check whether subsequent points would overlap it. 
	// If so, displace them in a fixed pattern 
	i = 0;
	while(i < plot->p_count - 1) {
		for(j = 1; i+j < plot->p_count; j++) {
			if(jdist(&plot->points[i], &plot->points[i+j]) >= ygap)
				break;
			// Displace point purely on x 
			xjit  = (j+1)/2 * jitter.spread * plot->lp_properties.p_size;
			if(jitter.limit > 0)
				while(xjit > jitter.limit)
					xjit -= jitter.limit;
			if((j & 01) != 0)
				xjit = -xjit;
			plot->points[i+j].CRD_XJITTER = xjit;
			if(jitter.style == JITTER_SQUARE)
				plot->points[i+j].CRD_YJITTER = plot->points[i].y - plot->points[i+j].y;
			// Displace points on y instead of x 
			if(jitter.style == JITTER_ON_Y) {
				plot->points[i+j].CRD_YJITTER = xjit;
				plot->points[i+j].CRD_XJITTER = 0;
			}
		}
		i += j;
	}
	// Copy variable colors back to where the plotting code expects to find them 
	if(plot->varcolor) {
		for(i = 0; i < plot->p_count; i++)
			plot->varcolor[i] = plot->points[i].z;
	}
}
//
// process 'set jitter' command 
//
void set_jitter()
{
	GPO.Pgm.Shift();
	// Default overlap criterion 1 character (usually on y) 
	jitter.overlap.scalex = character;
	jitter.overlap.x = 1;
	jitter.spread = 1.0;
	jitter.limit = 0.0;
	jitter.style = JITTER_DEFAULT;
	if(GPO.Pgm.EndOfCommand())
		return;
	while(!GPO.Pgm.EndOfCommand()) {
		if(GPO.Pgm.AlmostEqualsCur("over$lap")) {
			GPO.Pgm.Shift();
			GPO.GetPositionDefault(&jitter.overlap, character, 2);
		}
		else if(GPO.Pgm.EqualsCur("spread")) {
			GPO.Pgm.Shift();
			jitter.spread = GPO.RealExpression();
			if(jitter.spread <= 0)
				jitter.spread = 1.0;
		}
		else if(GPO.Pgm.EqualsCur("swarm")) {
			GPO.Pgm.Shift();
			jitter.style = JITTER_SWARM;
		}
		else if(GPO.Pgm.EqualsCur("square")) {
			GPO.Pgm.Shift();
			jitter.style = JITTER_SQUARE;
		}
		else if(GPO.Pgm.EqualsCur("wrap")) {
			GPO.Pgm.Shift();
			jitter.limit = GPO.RealExpression();
		}
		else if(GPO.Pgm.AlmostEqualsCur("vert$ical")) {
			GPO.Pgm.Shift();
			jitter.style = JITTER_ON_Y;
		}
		else
			GPO.IntErrorCurToken("unrecognized keyword");
	}
}

/* process 'show jitter' command */
void show_jitter()
{
	if(jitter.spread <= 0) {
		fprintf(stderr, "\tno jitter\n");
		return;
	}
	fprintf(stderr, "\toverlap criterion  %g %s coords\n", jitter.overlap.x, coord_msg[jitter.overlap.scalex]);
	fprintf(stderr, "\tspread multiplier on x (or y): %g\n", jitter.spread);
	if(jitter.limit > 0)
		fprintf(stderr, "\twrap at %g character widths\n", jitter.limit);
	fprintf(stderr, "\tstyle: %s\n", jitter.style == JITTER_SQUARE ? "square" : jitter.style == JITTER_ON_Y ? "vertical" : "swarm");
}

/* process 'unset jitter' command */
void unset_jitter()
{
	jitter.spread = 0;
}

/* called by the save command */
void save_jitter(FILE * fp)
{
	if(jitter.spread <= 0)
		fprintf(fp, "unset jitter\n");
	else {
		fprintf(fp, "set jitter overlap %s%g", jitter.overlap.scalex == character ? "" : coord_msg[jitter.overlap.scalex], jitter.overlap.x);
		fprintf(fp, "  spread %g  wrap %g", jitter.spread, jitter.limit);
		fprintf(fp, jitter.style == JITTER_SQUARE ? " square\n" : jitter.style == JITTER_ON_Y ? " vertical\n" : "\n");
	}
}
