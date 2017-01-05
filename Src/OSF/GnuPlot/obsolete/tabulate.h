/*
 * $Id: tabulate.h,v 1.2 2014/04/05 06:17:09 markisch Exp $
 */

/* GNUPLOT - tabulate.h */

#ifndef GNUPLOT_TABULATE_H
# define GNUPLOT_TABULATE_H

//#include "syscfg.h"
//
// Routines in tabulate.c needed by other modules:
//
void print_table(curve_points * first_plot, int plot_num);
void print_3dtable(int pcount);

extern FILE * table_outfile;
extern UdvtEntry * table_var;
extern bool table_mode;

#endif /* GNUPLOT_TABULATE_H */
