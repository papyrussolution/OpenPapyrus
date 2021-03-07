/*
 * This program creates some binary data files used by the demo
 * binary.dem to exercise gnuplot's binary input routines.
 * This code is not used by gnuplot itself.
 *
 * Copyright (c) 1992 Robert K. Cunningham, MIT Lincoln Laboratory
 *
 */
/*
 * Ethan A Merritt July 2014
 * Remove dependence on any gnuplot source files
 */
#include <gnuplot.h>
#pragma hdrstop

static float function(int p, double x, double y);

struct range {
	float xmin;
	float xmax;
	float ymin;
	float ymax;
};

#define NUM_PLOTS 2
static range TheRange[] = {{-3, 3, -2, 2},
			   {-3, 3, -3, 3},
			   {-3, 3, -3, 3}}; /* Sampling rate causes this to go from -3:6*/

static float function(int p, double x, double y)
{
	float t = 0;                    /* HBB 990828: initialize */
	switch(p) {
		case 0:
		    t = static_cast<float>(1.0 / (x * x + y * y + 1.0));
		    break;
		case 1:
			t = static_cast<float>(sin(x * x + y * y) / (x * x + y * y));
			SETMIN(t, 1.0);
		    break;
		case 2:
		    t = static_cast<float>(sin(x * x + y * y) / (x * x + y * y));
		    // sinc modulated sinc 
		    t *= sin(4.0 * (x * x + y * y)) / (4.0 * (x * x + y * y));
		    if(t > 1.0)
			    t = 1.0;
		    break;
		default:
		    fprintf(stderr, "Unknown function\n");
		    break;
	}
	return t;
}

int fwrite_matrix(FILE * fout, float ** m, int xsize, int ysize, float * rt, float * ct)
{
	int j;
	int status;
	float length = (float)ysize;
	if((status = fwrite((char *)&length, sizeof(float), 1, fout)) != 1) {
		fprintf(stderr, "fwrite 1 returned %d\n", status);
		return 0;
	}
	fwrite((char *)ct, sizeof(float), ysize, fout);
	for(j = 0; j < xsize; j++) {
		fwrite((char *)&rt[j], sizeof(float), 1, fout);
		fwrite((char *)(m[j]), sizeof(float), ysize, fout);
	}

	return 1;
}

#define ISOSAMPLES 5.0f

int main(void)
{
	int plot;
	int i, j;
	int im;
	float x, y;
	float * rt, * ct;
	float ** m;
	int xsize, ysize;
	char buf[256];
	FILE * fout;
	//  Create a few standard test interfaces 
	for(plot = 0; plot < NUM_PLOTS; plot++) {
		xsize = static_cast<int>((TheRange[plot].xmax - TheRange[plot].xmin) * ISOSAMPLES + 1);
		ysize = static_cast<int>((TheRange[plot].ymax - TheRange[plot].ymin) * ISOSAMPLES + 1);
		sprintf(buf, "binary%d", plot + 1);
		if(!(fout = fopen(buf, "wb"))) {
			fprintf(stderr, "Could not open output file\n");
			return EXIT_FAILURE;
		}
		rt = (float *)SAlloc::C(xsize, sizeof(float));
		ct = (float *)SAlloc::C(ysize, sizeof(float));
		m = (float **)SAlloc::C(xsize, sizeof(m[0]));
		for(im = 0; im < xsize; im++) {
			m[im] = (float *)SAlloc::C(ysize, sizeof(m[0][0]));
		}
		for(y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0f / ISOSAMPLES) {
			ct[j] = y;
		}
		for(x = TheRange[plot].xmin, i = 0; i < xsize; i++, x += 1.0f / ISOSAMPLES) {
			rt[i] = x;
			for(y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0f / ISOSAMPLES) {
				m[i][j] = function(plot, x, y);
			}
		}
		fwrite_matrix(fout, m, xsize, ysize, rt, ct);
		SAlloc::F(rt);
		SAlloc::F(ct);
		for(im = 0; im < xsize; im++)
			SAlloc::F(m[im]);
		SAlloc::F(m);
	}
	// Show that it's ok to vary sampling rate, as long as x1<x2, y1<y2... 
	sprintf(buf, "binary%d", plot + 1);
	if(!(fout = fopen(buf, "wb"))) {
		fprintf(stderr, "Could not open output file\n");
		return EXIT_FAILURE;
	}
	xsize = static_cast<int>((TheRange[plot].xmax - TheRange[plot].xmin) * ISOSAMPLES + 1);
	ysize = static_cast<int>((TheRange[plot].ymax - TheRange[plot].ymin) * ISOSAMPLES + 1);
	rt = (float *)SAlloc::C(xsize, sizeof(float));
	ct = (float *)SAlloc::C(ysize, sizeof(float));
	m = (float **)SAlloc::C(xsize, sizeof(m[0]));
	for(im = 0; im < xsize; im++) {
		m[im] = (float *)SAlloc::C(ysize, sizeof(m[0][0]));
	}
	for(y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0f / ISOSAMPLES) {
		ct[j] = y > 0 ? 2 * y : y;
	}
	for(x = TheRange[plot].xmin, i = 0; i < xsize; i++, x += 1.0f / ISOSAMPLES) {
		rt[i] = x > 0 ? 2 * x : x;
		for(y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0f / ISOSAMPLES) {
			m[i][j] = function(plot, x, y);
		}
	}
	fwrite_matrix(fout, m, xsize, ysize, rt, ct);
	SAlloc::F(rt);
	SAlloc::F(ct);
	for(im = 0; im < xsize; im++)
		SAlloc::F(m[im]);
	SAlloc::F(m);
	return 0;
}
