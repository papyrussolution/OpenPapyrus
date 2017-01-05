/*
 * $Id: setshow.h,v 1.55 2015/12/19 21:45:35 sfeam Exp $
 */

/* GNUPLOT - setshow.h */

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


#ifndef GNUPLOT_SETSHOW_H
# define GNUPLOT_SETSHOW_H

#include "stdfn.h"
//#include "axis.h"
//#include "gadgets.h"
//#include "term_api.h"

/* The set and show commands, in setshow.c */
void set_command();
void unset_command();
void reset_command();
void show_command();
/* and some accessible support functions */
void show_version(FILE *fp);
void set_format();
void set_colorsequence(int option);
char *conv_text(const char *s);
void delete_linestyle(linestyle_def **, linestyle_def *, linestyle_def *);
void delete_dashtype(custom_dashtype_def *, custom_dashtype_def *);
/* void delete_arrowstyle(arrowstyle_def *, arrowstyle_def *); */
void reset_key();
//void free_marklist(ticmark * list);
extern int enable_reset_palette;
void reset_palette();
void reset_bars();
//void rrange_to_xy();
void unset_monochrome();
// Called from set_label(), plot2d.c and plot3d.c 
extern void parse_label_options(GpTextLabel *, int ndim);
extern GpTextLabel * new_text_label(int tag);
extern void disp_value(FILE *, t_value *, bool);
extern ticmark * prune_dataticks(ticmark *list);


#endif /* GNUPLOT_SETSHOW_H */
