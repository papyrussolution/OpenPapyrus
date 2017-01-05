/*
 * $Id: jitter.h,v 1.1 2015/09/14 03:28:47 sfeam Exp $
 */
#ifndef GNUPLOT_JITTER_H
# define GNUPLOT_JITTER_H

//#include "syscfg.h"
//#include "axis.h"
//#include "command.h"
//#include "gp_types.h"
//#include "gadgets.h"
//#include "graphics.h"
//#include "save.h"
//#include <math.h>

enum jitterstyle {
    JITTER_DEFAULT = 0,
    JITTER_SWARM,
    JITTER_SQUARE
};

struct t_jitter {
	t_jitter()
	{
		overlap.Set(first_axes, first_axes, first_axes, 0.0, 0.0, 0.0);
		spread = 0.0;
		limit = 0.0;
		style = JITTER_DEFAULT;
	}
    t_position overlap;
    double spread;
    double limit;
    enum jitterstyle style;
};

extern t_jitter jitter;

extern void jitter_points(curve_points *plot);
extern void set_jitter();
extern void show_jitter();
extern void unset_jitter();
extern void save_jitter(FILE *);

#endif
