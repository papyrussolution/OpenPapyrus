/*
 * $Id: boundary.h,v 1.2 2014/03/19 17:30:33 sfeam Exp $
 */

#ifndef GNUPLOT_BOUNDARY_H
# define GNUPLOT_BOUNDARY_H
//#include "syscfg.h"

//void boundary(curve_points *plots, int count);
//void do_key_bounds(legend_key *key);
//void do_key_layout(legend_key *key);
int find_maxl_keys(const curve_points *plots, int count, int *kcnt);
//void do_key_sample(curve_points *this_plot, legend_key *key, char *title,  int xl, int yl);
//void do_key_sample_point(curve_points *this_plot, legend_key *key, int xl, int yl);
//void draw_titles();
//void draw_key(legend_key *key, bool key_pass, int *xl, int *yl);
//
// Probably some of these could be made static 
//
class GpBoundary {
public:
	GpBoundary()
	{
		KeyEntryHeight = 0;
		key_point_offset = 0;
		key_col_wth = 0;
		yl_ref = 0;
		ylabel_x = 0;
		y2label_x = 0;
		xlabel_y = 0;
		x2label_y = 0;
		ylabel_y = 0;
		y2label_y = 0;
		xtic_y = 0;
		x2tic_y = 0;
		ytic_x = 0;
		y2tic_x = 0;
		key_cols = 0;
		key_rows = 0;
		//
		key_sample_width = 0;    // width of line sample 
		key_sample_left = 0;     // offset from x for left of line sample 
		key_sample_right = 0;    // offset from x for right of line sample 
		key_text_left = 0;       // offset from x for left-justified text 
		key_text_right = 0;      // offset from x for right-justified text 
		key_size_left = 0;       // size of left bit of key (text or sample, depends on key->reverse) 
		key_size_right = 0;      // size of right part of key (including padding) 
		key_xleft = 0;           // Amount of space on the left required by the key 
		max_ptitl_len = 0;       // max length of plot-titles (keys) 
		ptitl_cnt = 0;           // count keys with len > 0  
		key_width = 0;           // calculate once, then everyone uses it 
		key_height = 0;          // ditto 
		key_title_height = 0;    // nominal number of lines * character height 
		key_title_extra = 0;     // allow room for subscript/superscript 
		time_y = 0;
		time_x = 0;
		title_y = 0;
		//
		xlablin = 0;
		x2lablin = 0;
		ylablin = 0;
		y2lablin = 0;
		titlelin = 0;
		xticlin = 0;
		x2ticlin = 0;
	}
	void   Boundary(const curve_points * pPlots, int count);
	void   DoKeyBounds(legend_key * key);
	void   DoKeyLayout(legend_key * key);
	void   DoKeySample(curve_points * pPlot, legend_key * key, char * pTitle, int xl, int yl);
	void   DoKeySamplePoint(curve_points * this_plot, legend_key * key, int xl, int yl);
	void   DrawTitles();
	void   DrawKey(legend_key * key, bool key_pass, int * xinkey, int * yinkey);

	int    KeyEntryHeight; // bigger of t->v_char, t->v_tic 
	int    key_col_wth;
	int    key_rows;
	int    yl_ref;
	int    xtic_y;
	int    ytic_x;
	int    x2tic_y;
	int    y2tic_x;
	int    xlabel_y;
private:
	int    key_point_offset; // offset from x for point sample 
	int    ylabel_x;
	int    y2label_x;
	int    x2label_y;
	int    ylabel_y;
	int    y2label_y;
	int    key_cols;
	//
	// privates for Boundary
	//
	int    key_sample_width;    // width of line sample 
	int    key_sample_left;     // offset from x for left of line sample 
	int    key_sample_right;    // offset from x for right of line sample 
	int    key_text_left;       // offset from x for left-justified text 
	int    key_text_right;      // offset from x for right-justified text 
	int    key_size_left;       // size of left bit of key (text or sample, depends on key->reverse) 
	int    key_size_right;      // size of right part of key (including padding) 
	int    key_xleft;           // Amount of space on the left required by the key 
	int    max_ptitl_len;       // max length of plot-titles (keys) 
	int    ptitl_cnt;           // count keys with len > 0  
	int    key_width;           // calculate once, then everyone uses it 
	int    key_height;          // ditto 
	int    key_title_height;    // nominal number of lines * character height 
	int    key_title_extra;     // allow room for subscript/superscript 
	int    time_y;
	int    time_x;
	int    title_y;
	//
	int    xlablin;
	int    x2lablin;
	int    ylablin;
	int    y2lablin;
	int    titlelin;
	int    xticlin;
	int    x2ticlin;
};

extern GpBoundary GpB;

#endif /* GNUPLOT_BOUNDARY_H */
