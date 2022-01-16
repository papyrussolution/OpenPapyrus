// GNUPLOT - post.h 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#ifndef TERM_POST_H
#define TERM_POST_H
//
// Needed by terminals which output postscript (post.trm and pslatex.trm)
//
#ifdef PSLATEX_DRIVER
	extern void PSTEX_common_init(GpTermEntry_Static * pThis);
	extern void PSTEX_reopen_output(GpTermEntry_Static * pThis);
	extern void EPSLATEX_common_init(GpTermEntry_Static * pThis);
	extern void EPSLATEX_reopen_output(GpTermEntry_Static * pThis, const char *);
#endif

#define PS_POINT_TYPES 8

// assumes landscape 
#define PS_XMAX (10*720)
#define PS_YMAX (7*720)
#define PS_YMAX_OLDSTYLE (6*720)
//
// These seem to be unnecessary, thus commented out 
//#define PS_XLAST (PS_XMAX - 1)
//#define PS_YLAST (PS_YMAX - 1)
#define PS_VTIC (PS_YMAX/80)
#define PS_HTIC (PS_YMAX/80)
#define PS_SC 10 // scale is 1pt = 10 units 
#define PS_LW (0.5*PS_SC) // linewidth = 0.5 pts 
// character size defaults: 
// 14 pt for postscript 
#define PS_VCHAR (14*PS_SC)
#define PS_HCHAR (14*PS_SC*6/10)
// 10 pt for ps(la)tex 
#define PSTEX_VCHAR (10*PS_SC)
#define PSTEX_HCHAR (10*PS_SC*6/10)
// 11 pt for epslatex 
#define EPSLATEX_VCHAR (11*PS_SC)
#define EPSLATEX_HCHAR (11*PS_SC*6/10)
#ifdef PSLATEX_DRIVER
	/*TERM_PUBLIC*/extern char * epslatex_header; // additional LaTeX header information for epslatex terminal 
#endif

#endif /* TERM_POST_H */
