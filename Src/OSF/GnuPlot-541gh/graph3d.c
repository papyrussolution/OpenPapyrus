// GNUPLOT - graph3d.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * AUTHORS
 *   Original Software: Gershon Elber and many others.
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * 3.6 - split graph3d.c into graph3d.c (graph), util3d.c (intersections, etc) hidden3d.c (hidden-line removal code)
 */
#include <gnuplot.h>
#pragma hdrstop

struct GpGraph3DBlock {
	GpGraph3DBlock();
	int    KeyEntryHeight; // bigger of t->ChrV, pointsize*t->v_tick 
	int    KeyTitleHeight;
	int    KeyTitleExtra;  // allow room for subscript/superscript 
	int    KeyTitleWidth;
	// 
	// we precalculate features of the key, to save lots of nested
	// ifs in code - x,y = user supplied or computed position of key
	// taken to be inner edge of a line sample
	// 
	int    KeySampleLeft;  // offset from x for left of line sample 
	int    KeySampleRight; // offset from x for right of line sample 
	int    KeyPointOffset; // offset from x for point sample 
	int    KeyTextLeft;    // offset from x for left-justified text 
	int    KeyTextRight;   // offset from x for right-justified text 
	int    KeySizeLeft;    // distance from x to left edge of box 
	int    KeySizeRight;   // distance from x to right edge of box 
	int    SPlotMapActive;
	float  SPlotMapSurfaceRotX;
	float  SPlotMapSurfaceRotZ;
	float  SPlotMapSurfaceScale;
	//
	// x and y input range endpoints where the three axes are to be
	// displayed (left, front-left, and front-right edges of the cube) 
	//
	double XAxisY;
	double YAxisX;
	double ZAxisX;
	double ZAxisY;
	// 
	// ... and the same for the back, right, and front corners 
	// 
	SPoint2R Back;
	SPoint2R Right;
	SPoint2R Front;
	SPoint3R TicUnit; // unit vector (terminal coords) 
	// 
	// The global flags splot_map, xz_projection, and yz_projection are specific views.
	// These flag the more general case of looking down the x or y axis
	// 
	bool   XzPlane;
	bool   YzPlane;
	bool   CanPm3D;
	//
	int    PTitlCnt;
	int    MaxPTitlLen;
	int    TitleLin;
	int    KeySampleWidth;
	int    KeyRows;
	int    KeyCols;
	int    KeyColWth;
	int    YlRef;
	double KTitleLines;
	double CeilingZ1;
	double BaseZ1;
};

GpGraph3DBlock::GpGraph3DBlock() : KeyEntryHeight(0), KeyTitleHeight(0), KeyTitleExtra(0), KeyTitleWidth(0),
	KeySampleLeft(0), KeySampleRight(0), KeyPointOffset(0), KeyTextLeft(0), KeyTextRight(0), KeySizeLeft(0), KeySizeRight(0),
	SPlotMapActive(0), SPlotMapSurfaceRotX(0.0f), SPlotMapSurfaceRotZ(0.0f), SPlotMapSurfaceScale(0.0f),
	XAxisY(0.0), YAxisX(0.0), ZAxisX(0.0), ZAxisY(0.0), XzPlane(false), YzPlane(false), CanPm3D(false),
	PTitlCnt(0), MaxPTitlLen(0), TitleLin(0), KeySampleWidth(0), KeyRows(0), KeyCols(0), KeyColWth(0), YlRef(0), KTitleLines(0.0),
	CeilingZ1(0.0), BaseZ1(0.0)
{
	Back.Set(0.0);
	Right.Set(0.0);
	Front.Set(0.0);
}

GpGraph3DBlock _3DBlk;

//static double ceiling_z1;
//static double base_z1;
//static int    ptitl_cnt;
//static int    max_ptitl_len;
//static int    titlelin;
//static int    key_sample_width;
//static int    key_rows;
//static int    key_cols;
//static int    key_col_wth;
//static int    yl_ref;
//static double ktitle_lines = 0.0;
//
// Support routines for "set view map"
//
//static int   splot_map_active = 0;
//static float splot_map_surface_rot_x;
//static float splot_map_surface_rot_z;
//static float splot_map_surface_scale;

//static int key_entry_height;    /* bigger of t->ChrV, pointsize*t->v_tick */
//static int key_title_height;
//static int key_title_extra;     /* allow room for subscript/superscript */
//static int key_title_width;
// 
// we precalculate features of the key, to save lots of nested
// ifs in code - x,y = user supplied or computed position of key
// taken to be inner edge of a line sample
// 
//static int key_sample_left;  // offset from x for left of line sample 
//static int key_sample_right; // offset from x for right of line sample 
//static int key_point_offset; // offset from x for point sample 
//static int key_text_left;    // offset from x for left-justified text 
//static int key_text_right;   // offset from x for right-justified text 
//static int key_size_left;    // distance from x to left edge of box 
//static int key_size_right;   // distance from x to right edge of box 

t_contour_placement draw_contour = CONTOUR_NONE; // is contouring wanted?
int    clabel_interval = 20; // label every 20th contour segment 
int    clabel_start = 5; // starting with the 5th 
char * clabel_font = NULL; // default to current font 
bool   clabel_onecolor = FALSE; // use same linetype for all contours 
bool   draw_surface = TRUE; // Draw the surface at all? (FALSE if only contours are wanted) 
bool   implicit_surface = TRUE; // Always create a gridded surface when lines are read from a data file 
bool   hidden3d = FALSE; // Was hidden3d display selected by user? 
int    hidden3d_layer = LAYER_BACK;
// 
// Rotation and scale of the 3d view, as controlled by 'set view': 
// 
float  surface_rot_z = 30.0f;
float  surface_rot_x = 60.0f;
float  surface_scale = 1.0f;
float  surface_zscale = 1.0f;
float  surface_lscale = 0.0f;
float  mapview_scale = 1.0f;
float  azimuth = 0.0f;
// 
// These flags indicate projection onto the xy, xz or yz plane
// as requested by 'set view map' or 'set view projection'.
// in_3d_polygon disables conversion of graph coordinates from x/y/z to
// hor/ver in projection; i.e. polygon vertices are always orthogonal x/y/z.
// 
bool   splot_map = FALSE;
bool   xz_projection = FALSE;
bool   yz_projection = FALSE;
bool   in_3d_polygon = FALSE;
t_xyplane xyplane = { 0.5, FALSE }; // position of the base plane, as given by 'set ticslevel' or 'set xyplane' 
// 'set isosamples' settings 
//int    iso_samples_1 = ISO_SAMPLES;
//int    iso_samples_2 = ISO_SAMPLES;
//double xscale3d, yscale3d, zscale3d;
//double xcenter3d = 0.0;
//double ycenter3d = 0.0;
//double zcenter3d = 0.0;
SPoint3R Scale3D;
SPoint3R Center3D;

static void do_3dkey_layout(GpTermEntry * pTerm, legend_key * key, int * xinkey, int * yinkey);
static void get_surface_cbminmax(const GpSurfacePoints * plot, double * cbmin, double * cbmax);
static int  find_maxl_cntr(gnuplot_contours * contours, int * count);
static int  find_maxl_keys3d(const GpSurfacePoints * plots, int count, int * kcnt);
static void key_text(GpTermEntry * pTerm, int xl, int yl, char * text);
//static bool get_arrow3d(arrow_def*, double*, double*, double*, double*);
//static void place_arrows3d(int);
//static void place_labels3d(text_label * listhead, int layer);
//static void flip_projection_axis(GpAxis * axis);
static void splot_map_activate();
static void splot_map_deactivate();

#define i_inrange(z, a, b) inrange((z), (a), (b))

#define apx_eq(x, y) (fabs(x-y) < 0.001)
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define SQR(x) ((x) * (x))
// 
// Define the boundary of the plot
// These are computed at each call to do_plot, and are constant over
// the period of one do_plot. They actually only change when the term
// type changes and when the 'set size' factors change.
// 
int    xmiddle;
int    ymiddle;
int    xscaler;
int    yscaler;
double xyscaler;
double radius_scaler;
//
// Boundary and scale factors, in user coordinates 
//
// These positions assume a single linear scale encompassing the
// zrange plus extra space below for the baseplane.  This was messy but
// correct before the introduction of nonlinear axes. Now - not so much.
//
// ceiling_z is the highest z in use
// floor_z   is the lowest z in use
// base_z is the z of the base
// min3d_z is the lowest z of the graph area
// max3d_z is the highest z of the graph area
//
// ceiling_z is either max3d_z or base_z, and similarly for floor_z
// There should be no part of graph drawn outside
// min3d_z:max3d_z  - apart from arrows, perhaps
//
double floor_z;
double ceiling_z; // made exportable for PM3D 
double base_z; // made exportable for PM3D 
// 
// To handle a non-linear z axis we need to calculate these values on
// the other end of the linked linear:nonlinear axis pair.
// 
double floor_z1; // Used also by map_z3d() 
transform_matrix trans_mat;

#ifdef USE_MOUSE
	int axis3d_o_x;
	int axis3d_o_y;
	int axis3d_x_dx;
	int axis3d_x_dy;
	int axis3d_y_dx;
	int axis3d_y_dy;
#endif
//
// calculate the number and max-width of the keys for an splot.
// Note that a blank line is issued after each set of contours
//
static int find_maxl_keys3d(const GpSurfacePoints * plots, int count, int * kcnt)
{
	int len;
	const GpSurfacePoints * this_plot = plots;
	int mlen = 0;
	int cnt = 0;
	for(int surf = 0; surf < count; this_plot = this_plot->next_sp, surf++) {
		// we draw a main entry if there is one, and we are
		// drawing either surface, or unlabeled contours
		if(this_plot->title && *this_plot->title && !this_plot->title_is_suppressed && !this_plot->title_position) {
			++cnt;
			len = estimate_strlen(this_plot->title, NULL);
			if(len > mlen)
				mlen = len;
		}
		if(draw_contour && !clabel_onecolor && this_plot->contours && this_plot->plot_style != LABELPOINTS) {
			len = find_maxl_cntr(this_plot->contours, &cnt);
			if(len > mlen)
				mlen = len;
		}
	}
	ASSIGN_PTR(kcnt, cnt);
	return (mlen);
}

static int find_maxl_cntr(struct gnuplot_contours * contours, int * count)
{
	int cnt = 0;
	int mlen = 0;
	int len;
	for(gnuplot_contours * cntrs = contours; cntrs;) {
		if(cntrs->isNewLevel) {
			len = estimate_strlen(cntrs->label, NULL) - strspn(cntrs->label, " ");
			if(len)
				cnt++;
			if(len > mlen)
				mlen = len;
		}
		cntrs = cntrs->next;
	}
	*count += cnt;
	return (mlen);
}
//
// borders of plotting area 
// computed once on every call to do_plot 
//
//static void boundary3d(const GpSurfacePoints * plots, int count)
void GnuPlot::Boundary3D(GpTermEntry * pTerm, const GpSurfacePoints * plots, int count)
{
	legend_key * key = &keyT;
	int i;
	_3DBlk.TitleLin = 0;
	_3DBlk.KeySampleWidth = (key->swidth >= 0.0) ? static_cast<int>(key->swidth * pTerm->ChrH + pTerm->TicH) : 0;
	_3DBlk.KeyEntryHeight = static_cast<int>(pTerm->TicV * 1.25 * key->vert_factor);
	if(_3DBlk.KeyEntryHeight < static_cast<int>(pTerm->ChrV)) {
		// is this reasonable ? 
		_3DBlk.KeyEntryHeight = static_cast<int>(pTerm->ChrV * key->vert_factor);
	}
	// Approximate width of titles is used to determine number of rows, cols
	// The actual widths will be recalculated later
	_3DBlk.MaxPTitlLen = find_maxl_keys3d(plots, count, &_3DBlk.PTitlCnt);
	_3DBlk.KeyTitleWidth = label_width(key->title.text, &i) * pTerm->ChrH;
	_3DBlk.KTitleLines = i;
	_3DBlk.KeyColWth = (_3DBlk.MaxPTitlLen + 4) * pTerm->ChrH + _3DBlk.KeySampleWidth;
	if(V.MarginL.scalex == screen)
		V.BbPlot.xleft = static_cast<int>(V.MarginL.x * (double)pTerm->MaxX + 0.5);
	else if(V.MarginL.x >= 0)
		V.BbPlot.xleft = static_cast<int>(V.MarginL.x * (double)pTerm->ChrH + 0.5);
	else
		V.BbPlot.xleft = pTerm->ChrH * 2 + pTerm->TicH;
	if(V.MarginR.scalex == screen)
		V.BbPlot.xright = static_cast<int>(V.MarginR.x * (double)pTerm->MaxX + 0.5);
	else // No tic label on the right side, so ignore rmargin 
		V.BbPlot.xright = static_cast<int>(V.Size.x * pTerm->MaxX - pTerm->ChrH * 2 - pTerm->TicH);
	_3DBlk.KeyRows = _3DBlk.PTitlCnt;
	_3DBlk.KeyCols = 1;
	if(_3DBlk.KeyRows > key->maxrows && key->maxrows > 0) {
		_3DBlk.KeyRows = key->maxrows;
		_3DBlk.KeyCols = (_3DBlk.PTitlCnt - 1)/_3DBlk.KeyRows + 1;
	}
	if(key->visible)
		if((key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC) && key->margin == GPKEY_BMARGIN) {
			if(_3DBlk.PTitlCnt > 0) {
				// calculate max no cols, limited by label-length 
				_3DBlk.KeyCols = (int)(V.BbPlot.xright - V.BbPlot.xleft) / ((_3DBlk.MaxPTitlLen + 4) * pTerm->ChrH + _3DBlk.KeySampleWidth);
				SETIFZ(_3DBlk.KeyCols, 1);
				_3DBlk.KeyRows = (int)((_3DBlk.PTitlCnt - 1)/ _3DBlk.KeyCols) + 1;
				// Limit the number of rows if requested by user 
				if(_3DBlk.KeyRows > key->maxrows && key->maxrows > 0)
					_3DBlk.KeyRows = key->maxrows;
				// now calculate actual no cols depending on no rows 
				_3DBlk.KeyCols = (int)((_3DBlk.PTitlCnt - 1)/ _3DBlk.KeyRows) + 1;
				_3DBlk.KeyColWth = (int)(V.BbPlot.xright - V.BbPlot.xleft) / _3DBlk.KeyCols;
			}
			else {
				_3DBlk.KeyRows = _3DBlk.KeyCols = _3DBlk.KeyColWth = 0;
			}
		}
	// Sanity check top and bottom margins, in case the user got confused 
	if(V.MarginB.scalex == screen && V.MarginT.scalex == screen) {
		ExchangeToOrder(&V.MarginB.x, &V.MarginT.x);
	}
	// this should also consider the view and number of lines in xformat || yformat || xlabel || ylabel 
	if(V.MarginB.scalex == screen)
		V.BbPlot.ybot = static_cast<int>(V.MarginB.x * (double)pTerm->MaxY + 0.5);
	else if(splot_map && V.MarginB.x >= 0)
		V.BbPlot.ybot = static_cast<int>((double)pTerm->ChrV * V.MarginB.x);
	else
		V.BbPlot.ybot = static_cast<int>(pTerm->ChrV * 2.5 + 1);
	if(key->visible)
		if(_3DBlk.KeyRows && (key->region == GPKEY_AUTO_EXTERIOR_MARGIN || key->region == GPKEY_AUTO_EXTERIOR_LRTBC) && key->margin == GPKEY_BMARGIN)
			V.BbPlot.ybot += _3DBlk.KeyRows * _3DBlk.KeyEntryHeight + _3DBlk.KeyTitleHeight;
	if(Gg.LblTitle.text) {
		_3DBlk.TitleLin++;
		for(uint lblidx = 0; lblidx < strlen(Gg.LblTitle.text); lblidx++) {
			if(Gg.LblTitle.text[lblidx] == '\\')
				_3DBlk.TitleLin++;
		}
	}
	if(V.MarginT.scalex == screen)
		V.BbPlot.ytop = static_cast<int>(V.MarginT.x * (double)pTerm->MaxY + 0.5);
	else // FIXME: Why no provision for tmargin in terms of character height? 
		V.BbPlot.ytop = static_cast<int>(V.Size.y * pTerm->MaxY - pTerm->ChrV * (_3DBlk.TitleLin + 1.5) - 1);
	if(key->visible)
		if(key->region == GPKEY_AUTO_INTERIOR_LRTBC || (oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_RMARGIN)) {
			// calculate max no rows, limited by V.BbPlot.ytop-V.BbPlot.ybot 
			i = static_cast<int>((int)(V.BbPlot.ytop - V.BbPlot.ybot) / pTerm->ChrV - 1 - _3DBlk.KTitleLines);
			if(i > key->maxrows && key->maxrows > 0)
				i = key->maxrows;
			if(i <= 0)
				i = 1;
			if(_3DBlk.PTitlCnt > i) {
				_3DBlk.KeyCols = (int)((_3DBlk.PTitlCnt - 1)/ i) + 1;
				// now calculate actual no rows depending on no cols 
				_3DBlk.KeyRows = (int)((_3DBlk.PTitlCnt - 1) / _3DBlk.KeyCols) + 1;
			}
		}
	if(key->visible) {
		if(oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_RMARGIN) {
			int key_width = _3DBlk.KeyColWth * _3DBlk.KeyCols - 2 * pTerm->ChrH;
			if(V.MarginR.scalex != screen)
				V.BbPlot.xright -= key_width;
		}
	}
	if(key->visible)
		if(oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_LMARGIN) {
			int key_width = _3DBlk.KeyColWth * _3DBlk.KeyCols - 2 * pTerm->ChrH;
			if(V.MarginL.scalex != screen)
				V.BbPlot.xleft += key_width;
		}
	if(!splot_map && V.AspectRatio3D > 0) {
		const int height = (V.BbPlot.ytop - V.BbPlot.ybot);
		const int width  = (V.BbPlot.xright - V.BbPlot.xleft);
		if(height > width) {
			V.BbPlot.ybot += (height-width)/2;
			V.BbPlot.ytop -= (height-width)/2;
		}
		else {
			V.BbPlot.xleft += (width-height)/2;
			V.BbPlot.xright -= (width-height)/2;
		}
	}
	if(V.MarginL.scalex != screen)
		V.BbPlot.xleft += pTerm->MaxX * V.Offset.X;
	if(V.MarginR.scalex != screen)
		V.BbPlot.xright += pTerm->MaxX * V.Offset.X;
	if(V.MarginT.scalex != screen)
		V.BbPlot.ytop += pTerm->MaxY * V.Offset.Y;
	if(V.MarginB.scalex != screen)
		V.BbPlot.ybot += pTerm->MaxY * V.Offset.Y;
	xmiddle = (V.BbPlot.xright + V.BbPlot.xleft) / 2;
	ymiddle = (V.BbPlot.ytop + V.BbPlot.ybot) / 2;
	// HBB: Magic number alert! 
	xscaler = ((V.BbPlot.xright - V.BbPlot.xleft) * 4L) / 7L;
	yscaler = ((V.BbPlot.ytop - V.BbPlot.ybot) * 4L) / 7L;
	// Allow explicit control via set {}margin screen 
	if(V.MarginT.scalex == screen || V.MarginB.scalex == screen)
		yscaler = static_cast<int>((V.BbPlot.ytop - V.BbPlot.ybot) / surface_scale);
	if(V.MarginR.scalex == screen || V.MarginL.scalex == screen)
		xscaler = static_cast<int>((V.BbPlot.xright - V.BbPlot.xleft) / surface_scale);
	// prevent infinite loop or divide-by-zero if scaling is bad 
	if(yscaler == 0) yscaler = 1;
	if(xscaler == 0) xscaler = 1;
	// 'set size {square|ratio}' for splots 
	if(splot_map && V.AspectRatio != 0.0f) {
		double current_aspect_ratio;
		if(V.AspectRatio < 0.0f && (AxS.__X().max - AxS.__X().min) != 0.0) {
			current_aspect_ratio = -V.AspectRatio * fabs((AxS.__Y().max - AxS.__Y().min) / (AxS.__X().max - AxS.__X().min));
		}
		else
			current_aspect_ratio = V.AspectRatio;
		// {{{  set aspect ratio if valid and sensible 
		if(current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
			double current = (double)yscaler / xscaler;
			double required = current_aspect_ratio * pTerm->TicV / pTerm->TicH;
			if(current > required) // too tall 
				yscaler = static_cast<int>(xscaler * required);
			else // too wide 
				xscaler = static_cast<int>(yscaler / required);
		}
	}
	xyscaler = sqrt(xscaler*yscaler); // For anything that really wants to be the same on x and y 
	radius_scaler = xscaler * surface_scale / (AxS.__X().max - AxS.__X().min); // This one is used to scale circles in 3D plots 
	// Set default clipping 
	if(splot_map)
		V.P_ClipArea = &V.BbPlot;
	else if(pTerm->flags & TERM_CAN_CLIP)
		V.P_ClipArea = NULL;
	else
		V.P_ClipArea = &V.BbCanvas;
}

//static bool get_arrow3d(arrow_def * pArrow, double * pDsx, double * pDsy, double * pDex, double * pDey)
bool GnuPlot::GetArrow3D(GpTermEntry * pTerm, arrow_def * pArrow, double * pDsx, double * pDsy, double * pDex, double * pDey)
{
	Map3DPositionDouble(pTerm, &pArrow->start, pDsx, pDsy, "arrow");
	if(pArrow->type == arrow_end_relative) {
		Map3DPositionRDouble(pTerm, &pArrow->end, pDex, pDey, "arrow");
		*pDex += *pDsx;
		*pDey += *pDsy;
	}
	else if(pArrow->type == arrow_end_oriented) {
		double aspect = (double)pTerm->TicV / (double)pTerm->TicH;
		double radius;
		double junkw, junkh;
#ifdef _WIN32
		if(strcmp(pTerm->name, "windows") == 0)
			aspect = 1.0;
#endif
		if(pArrow->end.scalex != screen && pArrow->end.scalex != character && !splot_map)
			return FALSE;
		Map3DPositionRDouble(pTerm, &pArrow->end, &junkw, &junkh, "arrow");
		radius = junkw;
		*pDex = *pDsx + cos(DEG2RAD * pArrow->angle) * radius;
		*pDey = *pDsy + sin(DEG2RAD * pArrow->angle) * radius * aspect;
	}
	else {
		Map3DPositionDouble(pTerm, &pArrow->end, pDex, pDey, "arrow");
	}
	return TRUE;
}

//static void place_labels3d(text_label * pListHead, int layer)
void GnuPlot::PlaceLabels3D(GpTermEntry * pTerm, text_label * pListHead, int layer)
{
	int x, y;
	pTerm->pointsize(Gg.PointSize);
	for(text_label * p_label = pListHead; p_label; p_label = p_label->next) {
		if(p_label->layer != layer)
			continue;
		if(layer == LAYER_PLOTLABELS) {
			double xx, yy;
			Map3D_XY_double(p_label->place.x, p_label->place.y, p_label->place.z, &xx, &yy);
			x = static_cast<int>(xx);
			y = static_cast<int>(yy);
			// Only clip in 2D 
			if(splot_map && V.ClipPoint(x, y))
				continue;
		}
		else
			Map3DPosition(pTerm, &p_label->place, &x, &y, "label");
		WriteLabel(pTerm, x, y, p_label);
	}
}

//static void place_arrows3d(int layer)
void GnuPlot::PlaceArrows3D(GpTermEntry * pTerm, int layer)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Allow arrows to run off the plot, so long as they are still on the canvas 
	V.P_ClipArea = (pTerm->flags & TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	for(arrow_def * this_arrow = Gg.P_FirstArrow; this_arrow; this_arrow = this_arrow->next) {
		double dsx, dsy, dex, dey;
		if(this_arrow->arrow_properties.layer != layer)
			continue;
		if(GetArrow3D(pTerm, this_arrow, &dsx, &dsy, &dex, &dey)) {
			TermApplyLpProperties(pTerm, &(this_arrow->arrow_properties.lp_properties));
			apply_head_properties(&(this_arrow->arrow_properties));
			DrawClipArrow(pTerm, dsx, dsy, dex, dey, this_arrow->arrow_properties.head);
		}
		else {
			FPRINTF((stderr, "place_arrows3d: skipping out-of-bounds arrow\n"));
		}
	}
	V.P_ClipArea = clip_save;
}

//void do_3dplot(GpTermEntry * pTerm, GpSurfacePoints * plots, int pcount/* count of plots in linked list */, REPLOT_TYPE replot_mode/* replot/refresh/axes-only/quick-refresh */)
void GnuPlot::Do3DPlot(GpTermEntry * pTerm, GpSurfacePoints * plots, int pcount/* count of plots in linked list */, REPLOT_TYPE replot_mode/* replot/refresh/axes-only/quick-refresh */)
{
	int surface;
	GpSurfacePoints * this_plot = NULL;
	int xl = 0, yl = 0;
	int xl_save, yl_save;
	int xl_prev = 0, yl_prev = 0;
	int title_x = 0;
	int title_y = 0;
	transform_matrix mat;
	int key_count;
	bool key_pass = FALSE;
	legend_key * key = &keyT;
	bool pm3d_order_depth = FALSE;
	GpAxis * primary_z;
	// Initiate transformation matrix using the global view variables. 
	if(splot_map) {
		splot_map_activate();
	}
	else if(xz_projection) {
		surface_rot_x = 270.0f;
		surface_rot_z = 0.0f;
		surface_scale = 1.425f * mapview_scale;
	}
	else if(yz_projection) {
		surface_rot_x = 90.0f;
		surface_rot_z = 90.0f;
		surface_scale = 1.425f * mapview_scale;
		AxS[FIRST_Z_AXIS].FlipProjection();
	}
	in_3d_polygon = FALSE;  /* protects polygons from xz, yz projections */
	mat_rot_z(surface_rot_z, trans_mat);
	mat_rot_x(surface_rot_x, mat);
	mat_mult(trans_mat, trans_mat, mat);
	mat_scale(surface_scale / 2.0, surface_scale / 2.0, surface_scale / 2.0, mat);
	mat_mult(trans_mat, trans_mat, mat);
	// The azimuth is applied as a rotation about the line of sight 
	if(azimuth != 0.0f && !splot_map) {
		mat_rot_z(azimuth, mat);
		mat_mult(trans_mat, trans_mat, mat);
	}
	if(Gg.Polar)
		IntError(NO_CARET, "Cannot splot in polar coordinate system.");
	// In the case of a nonlinear z axis this points to the linear version 
	// that shadows it.  Otherwise it just points to FIRST_Z_AXIS.         
	primary_z = AxS.__Z().IsNonLinear() ? AxS.__Z().linked_to_primary : &AxS.__Z();
	// absolute or relative placement of xyplane along z 
	if(AxS.__Z().IsNonLinear()) {
		if(xyplane.absolute) {
			if(primary_z->log && xyplane.z <= 0)
				_3DBlk.BaseZ1 = EvalLinkFunction(primary_z, AxS.__Z().min);
			else
				_3DBlk.BaseZ1 = EvalLinkFunction(primary_z, xyplane.z);
		}
		else
			_3DBlk.BaseZ1 = primary_z->min - (primary_z->max - primary_z->min) * xyplane.z;
		base_z = EvalLinkFunction(&AxS.__Z(), _3DBlk.BaseZ1);
	}
	else {
		if(xyplane.absolute)
			_3DBlk.BaseZ1 = xyplane.z;
		else
			_3DBlk.BaseZ1 = primary_z->min - (primary_z->max - primary_z->min) * xyplane.z;
		base_z = _3DBlk.BaseZ1;
	}
	// If we are to draw some portion of the xyplane make sure zmin is updated properly. 
	if(AxS.__X().ticmode || AxS.__Y().ticmode || draw_border & 0x00F) {
		if(primary_z->min > primary_z->max) {
			floor_z1 = MAX(primary_z->min, _3DBlk.BaseZ1);
			_3DBlk.CeilingZ1 = MIN(primary_z->max, _3DBlk.BaseZ1);
		}
		else {
			floor_z1 = MIN(primary_z->min, _3DBlk.BaseZ1);
			_3DBlk.CeilingZ1 = MAX(primary_z->max, _3DBlk.BaseZ1);
		}
	}
	else {
		floor_z1 = primary_z->min;
		_3DBlk.CeilingZ1 = primary_z->max;
	}
	if(AxS.__Z().IsNonLinear()) {
		floor_z = EvalLinkFunction(&AxS.__Z(), floor_z1);
		ceiling_z = EvalLinkFunction(&AxS.__Z(), _3DBlk.CeilingZ1);
	}
	else {
		floor_z = floor_z1;
		ceiling_z = _3DBlk.CeilingZ1;
	}
	if(AxS.__X().min == AxS.__X().max)
		IntError(NO_CARET, "x_min3d should not equal x_max3d!");
	if(AxS.__Y().min == AxS.__Y().max)
		IntError(NO_CARET, "y_min3d should not equal y_max3d!");
	if(AxS.__Z().min == AxS.__Z().max)
		IntError(NO_CARET, "z_min3d should not equal z_max3d!");
	// Special case projections of the xz or yz plane 
	// Place x or y axis to the left of the plot 
	_3DBlk.XzPlane = _3DBlk.YzPlane = false;
	if(!splot_map && (surface_rot_x == 90 || surface_rot_x == 270)) {
		if(surface_rot_z ==  0 || surface_rot_z == 180) {
			_3DBlk.XzPlane = true;
			base_z = floor_z;
		}
		if(surface_rot_z == 90 || surface_rot_z == 270) {
			_3DBlk.YzPlane = true;
			if(surface_rot_x == 270 || yz_projection)
				base_z = ceiling_z;
		}
	}
	TermStartPlot(pTerm);
	(pTerm->layer)(pTerm, TERM_LAYER_3DPLOT);
	screen_ok = FALSE;
	(pTerm->layer)(pTerm, TERM_LAYER_BACKTEXT); // Sync point for epslatex text positioning 
	Boundary3D(pTerm, plots, pcount); // now compute boundary for plot 
	axis_set_scale_and_range(&AxS[FIRST_X_AXIS], V.BbPlot.xleft, V.BbPlot.xright);
	axis_set_scale_and_range(&AxS[FIRST_Y_AXIS], V.BbPlot.ybot, V.BbPlot.ytop);
	axis_set_scale_and_range(&AxS[FIRST_Z_AXIS], static_cast<int>(floor_z), static_cast<int>(ceiling_z));
	// SCALE FACTORS 
	Scale3D.z = 2.0 / (ceiling_z - floor_z) * surface_zscale;
	Scale3D.y = 2.0 / (AxS.__Y().max - AxS.__Y().min);
	Scale3D.x = 2.0 / (AxS.__X().max - AxS.__X().min);
	if(AxS.__X().IsNonLinear())
		Scale3D.x = 2.0 / (AxS.__X().linked_to_primary->max - AxS.__X().linked_to_primary->min);
	if(AxS.__Y().IsNonLinear())
		Scale3D.y = 2.0 / (AxS.__Y().linked_to_primary->max - AxS.__Y().linked_to_primary->min);
	if(AxS.__Z().IsNonLinear())
		Scale3D.z = 2.0 / (_3DBlk.CeilingZ1 - floor_z1) * surface_zscale;
	// Allow 'set view equal xy' to adjust rendered length of the X and/or Y axes.
	// NB: only works correctly for terminals whose coordinate system is isotropic. 
	//xcenter3d = ycenter3d = zcenter3d = 0.0;
	Center3D.Set(0.0, 0.0, 0.0);
	if(V.AspectRatio3D >= 2) {
		if(Scale3D.y > Scale3D.x) {
			Center3D.y = 1.0 - Scale3D.x/Scale3D.y;
			Scale3D.y = Scale3D.x;
		}
		else if(Scale3D.x > Scale3D.y) {
			Center3D.x = 1.0 - Scale3D.y/Scale3D.x;
			Scale3D.x = Scale3D.y;
		}
		if(V.AspectRatio3D >= 3)
			Scale3D.z = Scale3D.x;
	}
	// FIXME: I do not understand why this is correct 
	if(AxS.__Z().IsNonLinear())
		Center3D.z = 0.0;
	// Without this the rotation center would be located at 
	// the bottom of the plot. This places it in the middle.
	else
		Center3D.z =  -(ceiling_z - floor_z) / 2.0 * Scale3D.z + 1.0;
	// Needed for mousing by outboard terminal drivers 
	if(splot_map) {
		GpAxis * p_ax_x = &AxS[FIRST_X_AXIS];
		GpAxis * p_ax_y = &AxS[FIRST_Y_AXIS];
		int xl, xr, yb, yt;
		Map3D_XY(p_ax_x->min, p_ax_y->min, 0.0, &xl, &yb);
		Map3D_XY(p_ax_x->max, p_ax_y->max, 0.0, &xr, &yt);
		axis_set_scale_and_range(p_ax_x, xl, xr);
		axis_set_scale_and_range(p_ax_y, yb, yt);
	}
	// Initialize palette 
	if(replot_mode != AXIS_ONLY_ROTATE) {
		_3DBlk.CanPm3D = is_plot_with_palette() && !MakePalette(pTerm) && ((pTerm->flags & TERM_NULL_SET_COLOR) == 0);
	}
	// Give a chance for rectangles to be behind everything else 
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_BEHIND, 3);
	if(replot_mode != AXIS_ONLY_ROTATE)
		PlacePixmaps(pTerm, LAYER_BEHIND, 3);
	TermApplyLpProperties(pTerm, &border_lp);   /* border linetype */
	// must come before using Draw3DGraphBox() the first time 
	Setup3DBoxCorners();
	// DRAW GRID AND BORDER 
	// Original behaviour: draw entire grid in back, if 'set grid back': 
	// HBB 20040331: but not if in hidden3d mode 
	if(splot_map && border_layer != LAYER_FRONT)
		Draw3DGraphBox(pTerm, plots, pcount, BORDERONLY, LAYER_BACK);
	else if(!hidden3d && (grid_layer == LAYER_BACK))
		Draw3DGraphBox(pTerm, plots, pcount, ALLGRID, LAYER_BACK);
	else if(!hidden3d && (grid_layer == LAYER_BEHIND))
		// Default layering mode.  Draw the back part now, but not if
		// hidden3d is in use, because that relies on all isolated
		// lines being output after all surfaces have been defined. 
		Draw3DGraphBox(pTerm, plots, pcount, BACKGRID, LAYER_BACK);
	else if(hidden3d && border_layer == LAYER_BEHIND)
		Draw3DGraphBox(pTerm, plots, pcount, ALLGRID, LAYER_BACK);
	// Save state of V.BbPlot before applying rotations, etc 
	memcpy(&V.BbPage, &V.BbPlot, sizeof(V.BbPage));
	// Clipping in 'set view map' mode should be like 2D clipping 
	// FIXME:  Wasn't this already done in boundary3d?            
	if(splot_map) {
		int map_x1, map_y1, map_x2, map_y2;
		Map3D_XY(AxS.__X().min, AxS.__Y().min, base_z, &map_x1, &map_y1);
		Map3D_XY(AxS.__X().max, AxS.__Y().max, base_z, &map_x2, &map_y2);
		V.BbPlot.xleft = map_x1;
		V.BbPlot.xright = map_x2;
		V.BbPlot.ybot = map_y2;
		V.BbPlot.ytop = map_y1;
	}
	// Define the clipping area in 3D to lie between the left-most and
	// right-most graph box edges.  This is introduced for the benefit of
	// zooming in the canvas terminal.  It may or may not make any practical
	// difference for other terminals.  If it causes problems, then we will need
	// a separate BoundingBox structure to track the actual 3D graph box.
	else if(azimuth == 0) {
		int xl, xb, xr, xf, yl, yb, yr, yf;
		Map3D_XY(_3DBlk.ZAxisX, _3DBlk.ZAxisY, base_z, &xl, &yl);
		Map3D_XY(_3DBlk.Back.x,  _3DBlk.Back.y,  base_z, &xb, &yb);
		Map3D_XY(_3DBlk.Right.x, _3DBlk.Right.y, base_z, &xr, &yr);
		Map3D_XY(_3DBlk.Front.x, _3DBlk.Front.y, base_z, &xf, &yf);
		V.BbPlot.xleft = MIN(xl, xb); /* Always xl? */
		V.BbPlot.xright = MAX(xb, xr); /* Always xr? */
	}
	// PLACE TITLE 
	if(Gg.LblTitle.text != 0) {
		int x, y;
		if(splot_map) { /* case 'set view map' */
			int map_x1, map_y1, map_x2, map_y2;
			int tics_len = 0;
			if(AxS.__X().ticmode & TICS_MIRROR) {
				tics_len = (int)(AxS.__X().ticscale * (AxS.__X().TicIn ? -1 : 1) * (pTerm->TicV));
				if(tics_len < 0) tics_len = 0; /* take care only about upward tics */
			}
			Map3D_XY(AxS.__X().min, AxS.__Y().min, base_z, &map_x1, &map_y1);
			Map3D_XY(AxS.__X().max, AxS.__Y().max, base_z, &map_x2, &map_y2);
			// Distance between the title base line and graph top line or the upper part of
			// tics is as given by character height: 
			x = ((map_x1 + map_x2) / 2);
			y = static_cast<int>(map_y1 + tics_len + (_3DBlk.TitleLin + 0.5) * (pTerm->ChrV));
		}
		else { // usual 3d set view ... 
			x = (V.BbPlot.xleft + V.BbPlot.xright) / 2;
			y = (V.BbPlot.ytop + _3DBlk.TitleLin * (pTerm->ChrH));
		}
		// Save title position for later 
		title_x = x;
		title_y = y;
	}
	// PLACE TIMELABEL 
	if(Gg.LblTime.text) {
		int x = pTerm->ChrV;
		int y = Gg.TimeLabelBottom ? static_cast<int>(V.Offset.Y * AxS.__Y().max + pTerm->ChrV) : (V.BbPlot.ytop - pTerm->ChrV);
		do_timelabel(x, y);
	}
	// Add 'back' color box 
	if((replot_mode != AXIS_ONLY_ROTATE) && _3DBlk.CanPm3D && is_plot_with_colorbox() && Gg.ColorBox.layer == LAYER_BACK)
		DrawColorSmoothBox(pTerm, MODE_SPLOT);
	PlaceObjects(pTerm, grid_wall, LAYER_BACK, 3); /* Grid walls */
	PlacePixmaps(pTerm, LAYER_BACK, 3); /* pixmaps before objects so that a rectangle can be used as a border */
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_BACK, 3); /* Add 'back' rectangles */
	PlaceLabels3D(pTerm, Gg.P_FirstLabel, LAYER_BACK); /* PLACE LABELS */
	PlaceArrows3D(pTerm, LAYER_BACK); // PLACE ARROWS 
	(pTerm->layer)(pTerm, TERM_LAYER_FRONTTEXT); /* Sync point for epslatex text positioning */
	if(hidden3d && draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
		init_hidden_line_removal();
		reset_hidden_line_removal();
	}
	// WORK OUT KEY POSITION AND SIZE 
	do_3dkey_layout(pTerm, key, &xl, &yl);
	// "set key opaque" requires two passes, with the key drawn in the second pass 
	xl_save = xl; yl_save = yl;
SECOND_KEY_PASS:
	// This tells the canvas, qt, and svg terminals to restart the plot   
	// count so that key titles are in sync with the plots they describe. 
	(pTerm->layer)(pTerm, TERM_LAYER_RESET_PLOTNO);
	// Key box 
	if(key->visible) {
		(pTerm->layer)(pTerm, TERM_LAYER_KEYBOX);
		/* In two-pass mode, we blank out the key area after the graph	*/
		/* is drawn and then redo the key in the blank area.		*/
		if(key_pass && pTerm->fillbox && !(pTerm->flags & TERM_NULL_SET_COLOR)) {
			(pTerm->set_color)(pTerm, &key->fillcolor);
			(pTerm->fillbox)(pTerm, FS_OPAQUE, key->bounds.xleft, key->bounds.ybot, key->bounds.xright - key->bounds.xleft, key->bounds.ytop - key->bounds.ybot);
		}
		if(key->box.l_type > LT_NODRAW &&  key->bounds.ytop != key->bounds.ybot) {
			TermApplyLpProperties(pTerm, &key->box);
			newpath(pTerm);
			clip_move(key->bounds.xleft, key->bounds.ybot);
			ClipVector(pTerm, key->bounds.xleft, key->bounds.ytop);
			ClipVector(pTerm, key->bounds.xright, key->bounds.ytop);
			ClipVector(pTerm, key->bounds.xright, key->bounds.ybot);
			ClipVector(pTerm, key->bounds.xleft, key->bounds.ybot);
			closepath(pTerm);
			// draw a horizontal line between key title and first entry  JFi 
			clip_move(key->bounds.xleft, key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra);
			ClipVector(pTerm, key->bounds.xright, key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra);
		}
		if(key->title.text) {
			int center = (key->bounds.xright + key->bounds.xleft) / 2;
			int titley = key->bounds.ytop - _3DBlk.KeyTitleHeight/2;
			// FIXME: empirical tweak. I don't know why this is needed 
			titley += (_3DBlk.KTitleLines-1) * pTerm->ChrV/2;
			WriteLabel(pTerm, center, titley, &key->title);
			(pTerm->linetype)(pTerm, LT_BLACK);
		}
	}
	// DRAW SURFACES AND CONTOURS 
	if(!key_pass)
		if(hidden3d && (hidden3d_layer == LAYER_BACK) && draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
			(pTerm->layer)(pTerm, TERM_LAYER_BEFORE_PLOT);
			Plot3DHidden(pTerm, plots, pcount);
			(pTerm->layer)(pTerm, TERM_LAYER_AFTER_PLOT);
		}

	// Set up bookkeeping for the individual key titles 
#define NEXT_KEY_LINE()                                 \
	do {                                                \
		if(++key_count >= _3DBlk.KeyRows) {                    \
			yl = _3DBlk.YlRef; xl += _3DBlk.KeyColWth; key_count = 0;  \
		} else                                              \
			yl -= _3DBlk.KeyEntryHeight;                         \
	} while(0)
	key_count = 0;
	_3DBlk.YlRef = yl -= _3DBlk.KeyEntryHeight / 2;    /* centralise the keys */
	/* PM January 2005: The mistake of missing blank lines in the data file is
	 * so frequently made (see questions at comp.graphics.apps.gnuplot) that it
	 * really deserves this warning. But don't show it too often --- only if it
	 * is a single surface in the plot.
	 */
	if(plots->plot_style != BOXES)
		if(pcount == 1 && plots->num_iso_read == 1 && _3DBlk.CanPm3D && (plots->plot_style == PM3DSURFACE || PM3D_IMPLICIT == pm3d.implicit))
			fprintf(stderr, "  Warning: Single isoline (scan) is not enough for a pm3d plot.\n\t   Hint: Missing blank lines in the data file? See 'help pm3d' and FAQ.\n");
	pm3d_order_depth = (_3DBlk.CanPm3D && !draw_contour && pm3d.direction == PM3D_DEPTH);
	/* TODO:
	 *   During "refresh" from rotation it would be better to re-use previously
	 *   built quadrangle list rather than clearing and rebuilding it.
	 */
	if(pm3d_order_depth || track_pm3d_quadrangles) {
		pm3d_depth_queue_clear();
		PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_DEPTHORDER, 3);
	}
	this_plot = plots;
	if(replot_mode != AXIS_ONLY_ROTATE)
		for(surface = 0; surface < pcount; this_plot = this_plot->next_sp, surface++) {
			// just an abbreviation 
			bool lkey, draw_this_surface;
			// Skip over abortive data structures 
			if(this_plot->plot_type == NODATA)
				continue;
			// Sync point for start of new curve (used by svg, post, ...) 
			(pTerm->layer)(pTerm, TERM_LAYER_BEFORE_PLOT);
			if(!key_pass && this_plot->plot_type != KEYENTRY)
				if(_3DBlk.CanPm3D && PM3D_IMPLICIT == pm3d.implicit)
					Pm3DDrawOne(pTerm, this_plot);
			lkey = (key->visible && this_plot->title && this_plot->title[0] && !this_plot->title_is_suppressed);
			draw_this_surface = (draw_surface && !this_plot->opt_out_of_surface);
			if(this_plot->plot_type == KEYENTRY)
				draw_this_surface = TRUE;
			// User-specified key locations can use the 2D code 
			if(this_plot->title_position) {
				xl_prev = xl;
				yl_prev = yl;
				if(this_plot->title_position->scalex != character) {
					Map3DPosition(pTerm, this_plot->title_position, &xl, &yl, "key sample");
					xl -=  (key->just == GPKEY_LEFT) ? _3DBlk.KeyTextLeft : _3DBlk.KeyTextRight;
				}
				else {
					// Option to label the end of the curve on the plot itself 
					AttachTitleToPlot(pTerm, (curve_points *)this_plot, key);
				}
			}
			if(lkey && (!this_plot->title_position || this_plot->title_position->scalex != character)) {
				char * title = this_plot->title;
				if(this_plot->title_is_automated && (pTerm->flags & TERM_IS_LATEX))
					title = texify_title(title, this_plot->plot_type);
				if(key->textcolor.type != TC_DEFAULT)
					ApplyPm3DColor(pTerm, &key->textcolor); /* Draw key text in same color as key title */
				else
					(pTerm->linetype)(pTerm, LT_BLACK); /* Draw key text in black */
				ignore_enhanced(this_plot->title_no_enhanced);
				key_text(pTerm, xl, yl, title);
				ignore_enhanced(FALSE);
			}
			TermApplyLpProperties(pTerm, &(this_plot->lp_properties));
			// Voxel data is a special case. what about hidden3d mode? pm3d?
			if(!key_pass && this_plot->plot_type == VOXELDATA) {
				switch(this_plot->plot_style) {
					default:
					    // style should default to DOTS 
					    this_plot->plot_style = DOTS;
					case DOTS:
					case POINTSTYLE:
					    VPlotPoints(pTerm, this_plot, this_plot->iso_level);
					    break;
					case ISOSURFACE:
					    if(replot_mode == QUICK_REFRESH)
						    VPlotIsoSurface(pTerm, this_plot, 4);
					    else
						    VPlotIsoSurface(pTerm, this_plot, 1);
					    break;
				}
			}
			// First draw the graph plot itself 
			if(!key_pass && this_plot->plot_type != KEYENTRY && this_plot->plot_type != VOXELDATA)
				switch(this_plot->plot_style) {
					case FILLEDCURVES: /* same, but maybe we could dummy up ZERRORFILL? */
					case IMPULSES:
					    if(!hidden3d)
						    Plot3DImpulses(pTerm, this_plot);
					    break;
					case STEPS: /* HBB: I think these should be here */
					case FILLSTEPS:
					case FSTEPS:
					case HISTEPS:
					case SURFACEGRID:
					case LINES:
					    if(draw_this_surface) {
						    if(!hidden3d || this_plot->opt_out_of_hidden3d)
							    Plot3DLinesPm3D(pTerm, this_plot);
					    }
					    break;
					case YERRORLINES: /* ignored; treat like points */
					case XERRORLINES: /* ignored; treat like points */
					case XYERRORLINES: /* ignored; treat like points */
					case YERRORBARS: /* ignored; treat like points */
					case XERRORBARS: /* ignored; treat like points */
					case XYERRORBARS: /* ignored; treat like points */
					case BOXXYERROR: /* HBB: ignore these as well */
					case ARROWS:
					case CANDLESTICKS: /* HBB: ditto */
					case BOXPLOT:
					case FINANCEBARS:
					case CIRCLES:
					case ELLIPSES:
					case POINTSTYLE:
					case DOTS:
					    if(draw_this_surface) {
						    if(!hidden3d || this_plot->opt_out_of_hidden3d)
							    Plot3DPoints(pTerm, this_plot);
					    }
					    break;
					case LINESPOINTS:
					    if(draw_this_surface) {
						    if(!hidden3d || this_plot->opt_out_of_hidden3d) {
							    Plot3DLinesPm3D(pTerm, this_plot);
							    Plot3DPoints(pTerm, this_plot);
						    }
					    }
					    break;
					case VECTOR:
					    if(!hidden3d || this_plot->opt_out_of_hidden3d)
						    Plot3DVectors(pTerm, this_plot);
					    break;
					case ZERRORFILL:
					    // Always draw filled areas even if we _also_ do hidden3d processing 
					    if(pTerm->filled_polygon)
						    Plot3DZErrorFill(pTerm, this_plot);
					    TermApplyLpProperties(pTerm, &(this_plot->lp_properties));
					    Plot3DLines(pTerm, this_plot);
					    break;
					case BOXES:
					    if(pTerm->filled_polygon)
						    Plot3DBoxes(pTerm, this_plot);
					    else
						    Plot3DImpulses(pTerm, this_plot);
					    break;
					case BOXERROR:
					    Plot3DBoxErrorBars(pTerm, this_plot);
					    break;
					case PM3DSURFACE:
					    if(draw_this_surface) {
						    if(_3DBlk.CanPm3D && PM3D_IMPLICIT != pm3d.implicit) {
							    Pm3DDrawOne(pTerm, this_plot);
							    if(!pm3d_order_depth)
								    Pm3DDepthQueueFlush(pTerm); // draw plot immediately 
						    }
					    }
					    break;
					case POLYGONS:
					    if(pTerm->filled_polygon)
						    Plot3DPolygons(pTerm, this_plot);
					    else
						    Plot3DLines(pTerm, this_plot);
					    break;
					case LABELPOINTS:
					    if(draw_this_surface) {
						    if(hidden3d && !(this_plot->opt_out_of_hidden3d))
							    break;
						    if(draw_contour && !(this_plot->opt_out_of_contours))
							    break;
						    PlaceLabels3D(pTerm, this_plot->labels->next, LAYER_PLOTLABELS);
					    }
					    break;
					case HISTOGRAMS: /* Cannot happen */
					    break;
					case IMAGE:
					    // Plot image using projection of 3D plot coordinates to 2D viewing coordinates. 
					    this_plot->image_properties.type = IC_PALETTE;
					    ProcessImage(pTerm, this_plot, IMG_PLOT);
					    break;
					case RGBIMAGE:
					    // Plot image using projection of 3D plot coordinates to 2D viewing coordinates. 
					    this_plot->image_properties.type = IC_RGB;
					    ProcessImage(pTerm, this_plot, IMG_PLOT);
					    break;
					case RGBA_IMAGE:
					    this_plot->image_properties.type = IC_RGBA;
					    ProcessImage(pTerm, this_plot, IMG_PLOT);
					    break;
					case PARALLELPLOT:
					case SPIDERPLOT:
					    IntError(NO_CARET, "plot style not supported in 3D");
					    break;
					case ISOSURFACE:
					case PLOT_STYLE_NONE:
					case TABLESTYLE:
					    /* cannot happen */
					    break;
				}/* switch(plot-style) plot proper */
			// Next draw the key sample 
			if(lkey && (!this_plot->title_position || this_plot->title_position->scalex != character))
				switch(this_plot->plot_style) {
					case FILLEDCURVES:
					case IMPULSES:
					    if(!(hidden3d && draw_this_surface))
						    KeySampleLine(pTerm, xl, yl);
					    break;
					case STEPS: /* HBB: I think these should be here */
					case FILLSTEPS:
					case FSTEPS:
					case HISTEPS:
					case SURFACEGRID:
					case LINES:
					    // Normal case (surface) 
					    if(draw_this_surface)
						    KeySampleLinePm3D(pTerm, this_plot, xl, yl);
					    // Contour plot with no surface, all contours use the same linetype 
					    else if(this_plot->contours != NULL && clabel_onecolor) {
						    KeySampleLine(pTerm, xl, yl);
					    }
					    break;
					case YERRORLINES: /* ignored; treat like points */
					case XERRORLINES: /* ignored; treat like points */
					case XYERRORLINES: /* ignored; treat like points */
					case YERRORBARS: /* ignored; treat like points */
					case XERRORBARS: /* ignored; treat like points */
					case XYERRORBARS: /* ignored; treat like points */
					case BOXXYERROR: /* HBB: ignore these as well */
					case CANDLESTICKS: /* HBB: ditto */
					case BOXPLOT:
					case FINANCEBARS:
					case ELLIPSES:
					case POINTSTYLE:
					    if(this_plot->plot_type == VOXELDATA) {
						    if(this_plot->lp_properties.pm3d_color.type == TC_Z)
							    set_color(pTerm, 0.5);
						    KeySamplePoint(pTerm, this_plot, xl, yl, this_plot->lp_properties.PtType);
					    }
					    else if(draw_this_surface)
						    KeySamplePointPm3D(pTerm, this_plot, xl, yl, this_plot->lp_properties.PtType);
					    break;

					case LABELPOINTS:
					    if((this_plot->labels->lp_properties.flags & LP_SHOW_POINTS)) {
						    TermApplyLpProperties(pTerm, &this_plot->labels->lp_properties);
						    KeySamplePoint(pTerm, this_plot, xl, yl, this_plot->labels->lp_properties.PtType);
					    }
					    break;

					case LINESPOINTS:
					    if(draw_this_surface) {
						    if(this_plot->lp_properties.l_type != LT_NODRAW)
							    KeySampleLinePm3D(pTerm, this_plot, xl, yl);
						    KeySamplePointPm3D(pTerm, this_plot, xl, yl, this_plot->lp_properties.PtType);
					    }
					    break;
					case DOTS:
					    if(draw_this_surface)
						    KeySamplePointPm3D(pTerm, this_plot, xl, yl, -1);
					    break;
					case VECTOR:
					    KeySampleLinePm3D(pTerm, this_plot, xl, yl);
					    break;
					case ZERRORFILL:
					    ApplyPm3DColor(pTerm, &this_plot->fill_properties.border_color);
					    KeySampleFill(pTerm, xl, yl, this_plot);
					    TermApplyLpProperties(pTerm, &this_plot->lp_properties);
					    KeySampleLine(pTerm, xl, yl);
					    break;
					case BOXES:
					case CIRCLES:
					case BOXERROR:
					    ApplyPm3DColor(pTerm, &this_plot->lp_properties.pm3d_color);
					    if(this_plot->iso_crvs)
						    Check3DForVariableColor(pTerm, this_plot, this_plot->iso_crvs->points);
					    KeySampleFill(pTerm, xl, yl, this_plot);
					    break;
					case ISOSURFACE:
					    ApplyPm3DColor(pTerm, &this_plot->fill_properties.border_color);
					    KeySampleFill(pTerm, xl, yl, this_plot);
					    break;
					case PLOT_STYLE_NONE:
					// cannot happen 
					default:
					    break;
				}/* switch(plot-style) key sample */
			// If the title went somewhere other than the key,
			// restore the previous key position.
			// Else move down one line in the key.
			if(this_plot->title_position) {
				xl = xl_prev;
				yl = yl_prev;
			}
			else if(lkey)
				NEXT_KEY_LINE();
			// Draw contours for previous surface 
			if(draw_contour && this_plot->contours != NULL) {
				gnuplot_contours * cntrs = this_plot->contours;
				lp_style_type thiscontour_lp_properties;
				static char * thiscontour_label = NULL;
				bool save_hidden3d;
				int ic = 1; /* ic will index the contour linetypes */
				thiscontour_lp_properties = this_plot->lp_properties;
				TermApplyLpProperties(pTerm, &(thiscontour_lp_properties));
				while(cntrs) {
					if(!clabel_onecolor && cntrs->isNewLevel) {
						if(key->visible && !this_plot->title_is_suppressed && this_plot->plot_style != LABELPOINTS) {
							(pTerm->linetype)(pTerm, LT_BLACK);
							key_text(pTerm, xl, yl, cntrs->label);
						}
						if(thiscontour_lp_properties.pm3d_color.type == TC_Z)
							set_color(pTerm, Cb2Gray(cntrs->z) );
						else {
							lp_style_type ls = thiscontour_lp_properties;
							int contour_linetype;
							ic++; /* Increment linetype used for contour */
							// First contour line type defaults to surface linetype + 1  
							// but can be changed using 'set cntrparams firstlinetype N' 
							if(contour_firstlinetype > 0)
								contour_linetype = contour_firstlinetype + ic - 2;
							else
								contour_linetype = this_plot->hidden3d_top_linetype + ic;
							// hidden3d processing looks directly at l_type 
							// for other purposes the line color is set by load_linetype 
							if(hidden3d)
								thiscontour_lp_properties.l_type = contour_linetype - 1;
							load_linetype(pTerm, &ls, contour_linetype);
							thiscontour_lp_properties.pm3d_color = ls.pm3d_color;
							thiscontour_lp_properties.l_width = ls.l_width * this_plot->lp_properties.l_width;
							thiscontour_lp_properties.d_type = ls.d_type;
							thiscontour_lp_properties.CustomDashPattern = ls.CustomDashPattern;
							TermApplyLpProperties(pTerm, &thiscontour_lp_properties);
						}
						if(key->visible && !this_plot->title_is_suppressed && !(this_plot->plot_style == LABELPOINTS)) {
							switch(this_plot->plot_style) {
								case IMPULSES:
								case LINES:
								case LINESPOINTS:
								case FILLEDCURVES:
								case VECTOR:
								case STEPS:
								case FSTEPS:
								case HISTEPS:
								case PM3DSURFACE: KeySampleLine(pTerm, xl, yl); break;
								case POINTSTYLE: KeySamplePoint(pTerm, this_plot, xl, yl, this_plot->lp_properties.PtType); break;
								case DOTS: KeySamplePoint(pTerm, this_plot, xl, yl, -1); break;
								default:
								    break;
							} /* switch */
							NEXT_KEY_LINE();
						} /* key */
					} /* clabel_onecolor */
					// now draw the contour 
					if(!key_pass)
						switch(this_plot->plot_style) {
							// treat boxes like impulses: 
							case BOXES:
							case FILLEDCURVES:
							case VECTOR:
							case IMPULSES:
							    Ñntr3DImpulses(pTerm, cntrs, &thiscontour_lp_properties);
							    break;
							case STEPS:
							case FSTEPS:
							case HISTEPS:
							// treat all the above like 'lines' 
							case LINES:
							case PM3DSURFACE:
							    save_hidden3d = hidden3d;
							    if(this_plot->opt_out_of_hidden3d)
								    hidden3d = FALSE;
							    Ñntr3DLines(pTerm, cntrs, &thiscontour_lp_properties);
							    hidden3d = save_hidden3d;
							    break;
							case LINESPOINTS:
							    Ñntr3DLines(pTerm, cntrs, &thiscontour_lp_properties);
							// Fall through to draw the points 
							case DOTS:
							case POINTSTYLE:
							    Cntr3DPoints(pTerm, cntrs, &thiscontour_lp_properties);
							    break;
							case LABELPOINTS:
							    if(cntrs->isNewLevel) {
								    const char * c = &cntrs->label[strspn(cntrs->label, " ")];
								    SAlloc::F(thiscontour_label);
								    thiscontour_label = gp_strdup(c);
							    }
							    Cntr3DLabels(pTerm, cntrs, thiscontour_label, this_plot->labels);
							    break;
							default:
							    break;
						}/*switch */
					cntrs = cntrs->next;
				} /* loop over contours */
			} /* draw contours */
			/* Sync point for end of this curve (used by svg, post, ...) */
			(pTerm->layer)(pTerm, TERM_LAYER_AFTER_PLOT);
		}/* loop over surfaces */
	if(!key_pass)
		if(pm3d_order_depth || track_pm3d_quadrangles) {
			Pm3DDepthQueueFlush(pTerm); // draw pending plots 
		}
	if(!key_pass)
		if(hidden3d && (hidden3d_layer == LAYER_FRONT) && draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
			(pTerm->layer)(pTerm, TERM_LAYER_BEFORE_PLOT);
			Plot3DHidden(pTerm, plots, pcount);
			(pTerm->layer)(pTerm, TERM_LAYER_AFTER_PLOT);
		}
	/* Draw grid and border.
	 * The 1st case allows "set border behind" to override hidden3d processing.
	 * The 2nd case either leaves everything to hidden3d or forces it to the front.
	 * The 3rd case is the non-hidden3d default - draw back pieces (done earlier),
	 * then the graph, and now the front pieces.
	 */
	if(hidden3d && border_layer == LAYER_BEHIND)
		// the important thing is _not_ to draw the back grid 
		// Draw3DGraphBox(plots, pcount, FRONTGRID, LAYER_FRONT) 
		;
	else if(hidden3d || grid_layer == LAYER_FRONT)
		Draw3DGraphBox(pTerm, plots, pcount, ALLGRID, LAYER_FRONT);
	else if(grid_layer == LAYER_BEHIND)
		Draw3DGraphBox(pTerm, plots, pcount, FRONTGRID, LAYER_FRONT);
	// Go back and draw the legend in a separate pass if "key opaque" 
	if(key->visible && key->front && !key_pass) {
		key_pass = TRUE;
		xl = xl_save; yl = yl_save;
		goto SECOND_KEY_PASS;
	}
	// Add 'front' color box 
	if((replot_mode != AXIS_ONLY_ROTATE) && _3DBlk.CanPm3D && is_plot_with_colorbox() && Gg.ColorBox.layer == LAYER_FRONT)
		DrawColorSmoothBox(pTerm, MODE_SPLOT);
	// Add 'front' rectangles 
	PlacePixmaps(pTerm, LAYER_FRONT, 3);
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_FRONT, 3);
	PlaceObjects(pTerm, grid_wall, LAYER_FRONT, 3); /* Grid walls */
	PlaceLabels3D(pTerm, Gg.P_FirstLabel, LAYER_FRONT); /* PLACE LABELS */
	PlaceArrows3D(pTerm, LAYER_FRONT); // PLACE ARROWS 
	// PLACE TITLE LAST 
	if(Gg.LblTitle.text)
		PlaceTitle(pTerm, title_x, title_y);
#ifdef USE_MOUSE
	// finally, store the 2d projection of the x and y axis, to enable zooming by mouse 
	{
		int x, y;
		Map3D_XY(AxS.__X().min, AxS.__Y().min, base_z, &axis3d_o_x, &axis3d_o_y);
		Map3D_XY(AxS.__X().max, AxS.__Y().min, base_z, &x, &y);
		axis3d_x_dx = x - axis3d_o_x;
		axis3d_x_dy = y - axis3d_o_y;
		Map3D_XY(AxS.__X().min, AxS.__Y().max, base_z, &x, &y);
		axis3d_y_dx = x - axis3d_o_x;
		axis3d_y_dy = y - axis3d_o_y;
	}
#endif
	// Release the palette if we have used one (PostScript only?) 
	if(is_plot_with_palette() && pTerm->previous_palette)
		pTerm->previous_palette();
	TermEndPlot(pTerm);
	if(hidden3d && draw_surface) {
		term_hidden_line_removal();
	}
	if(splot_map)
		splot_map_deactivate();
	else if(xz_projection || yz_projection)
		surface_scale = 1.0;
	else if(yz_projection)
		AxS[FIRST_Z_AXIS].FlipProjection();
}
// 
// plot3d_impulses:
// Plot the surfaces in IMPULSES style
// 
//static void plot3d_impulses(GpSurfacePoints * pPlot)
void GnuPlot::Plot3DImpulses(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int i; // point index 
	int x, y, xx0, yy0; // point in terminal coordinates 
	iso_curve * icrvs = pPlot->iso_crvs;
	int colortype = pPlot->lp_properties.pm3d_color.type;
	if(colortype == TC_RGB)
		SetRgbColorConst(pTerm, pPlot->lp_properties.pm3d_color.lt);
	while(icrvs) {
		GpCoordinate * points = icrvs->points;
		for(i = 0; i < icrvs->p_count; i++) {
			Check3DForVariableColor(pTerm, pPlot, &points[i]);
			switch(points[i].type) {
				case INRANGE:
			    {
				    double z = 0.0;
				    Map3D_XY(points[i].x, points[i].y, points[i].z, &x, &y);
				    cliptorange(z, AxS.__Z().min, AxS.__Z().max);
				    Map3D_XY(points[i].x, points[i].y, z, &xx0, &yy0);
				    clip_move(xx0, yy0);
				    ClipVector(pTerm, x, y);
				    break;
			    }
				case OUTRANGE:
			    {
				    if(!AxS.__X().InRange(points[i].x) || !AxS.__Y().InRange(points[i].y))
					    break;
				    if(AxS.__Z().InRange(0.0)) {
					    // zero point is INRANGE 
					    Map3D_XY(points[i].x, points[i].y, 0.0, &xx0, &yy0);
					    // must cross z = AxS.__Z().min or AxS.__Z().max limits 
					    if(inrange(AxS.__Z().min, 0.0, points[i].z) && AxS.__Z().min != 0.0 && AxS.__Z().min != points[i].z) {
						    Map3D_XY(points[i].x, points[i].y, AxS.__Z().min, &x, &y);
					    }
					    else {
						    Map3D_XY(points[i].x, points[i].y, AxS.__Z().max, &x, &y);
					    }
				    }
				    else {
					    // zero point is also OUTRANGE 
					    if(inrange(AxS.__Z().min, 0.0, points[i].z) && inrange(AxS.__Z().max, 0.0, points[i].z)) {
						    // crosses z = AxS.__Z().min or AxS.__Z().max limits 
						    Map3D_XY(points[i].x, points[i].y, AxS.__Z().max, &x, &y);
						    Map3D_XY(points[i].x, points[i].y, AxS.__Z().min, &xx0, &yy0);
					    }
					    else {
						    // doesn't cross z = AxS.__Z().min or AxS.__Z().max limits 
						    break;
					    }
				    }
				    clip_move(xx0, yy0);
				    ClipVector(pTerm, x, y);
				    break;
			    }
				default: /* just a safety */
				case UNDEFINED: {
				    break;
			    }
			}
		}
		icrvs = icrvs->next;
	}
}
/* plot3d_lines:
 * Plot the surfaces in LINES style
 */
/* We want to always draw the lines in the same direction, otherwise when
   we draw an adjacent box we might get the line drawn a little differently
   and we get splotches.  */

//static void plot3d_lines(GpSurfacePoints * plot)
void GnuPlot::Plot3DLines(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int i;
	int x, y, xx0, yy0; /* point in terminal coordinates */
	double clip_x, clip_y, clip_z;
	iso_curve * icrvs = pPlot->iso_crvs;
	GpCoordinate * points;
	bool rgb_from_column;
	// These are handled elsewhere.  
	if(pPlot->has_grid_topology && hidden3d)
		return;
	// These don't need to be drawn at all 
	if(pPlot->lp_properties.l_type == LT_NODRAW)
		return;
	rgb_from_column = pPlot->pm3d_color_from_column && pPlot->lp_properties.pm3d_color.type == TC_RGB && pPlot->lp_properties.pm3d_color.value < 0.0;
	while(icrvs) {
		enum coord_type prev = UNDEFINED; /* type of previous plot */
		for(i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
			if(rgb_from_column)
				SetRgbColorVar(pTerm, (uint)points[i].CRD_COLOR);
			else if(pPlot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
				pPlot->lp_properties.pm3d_color.lt = (int)(points[i].CRD_COLOR);
				ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
			}
			switch(points[i].type) {
				case INRANGE: {
				    Map3D_XY(points[i].x, points[i].y, points[i].z, &x, &y);
				    if(prev == INRANGE) {
					    ClipVector(pTerm, x, y);
				    }
				    else {
					    if(prev == OUTRANGE) {
						    // from outrange to inrange 
						    if(!Gg.ClipLines1)
							    clip_move(x, y);
						    else {
							    //
							    // Calculate intersection point and draw vector from there
							    //
							    Edge3DIntersect(&points[i-1], &points[i], &clip_x, &clip_y, &clip_z);
							    Map3D_XY(clip_x, clip_y, clip_z, &xx0, &yy0);
							    clip_move(xx0, yy0);
							    ClipVector(pTerm, x, y);
						    }
					    }
					    else {
						    clip_move(x, y);
					    }
				    }
				    break;
			    }
				case OUTRANGE: {
				    if(prev == INRANGE) {
					    // from inrange to outrange 
					    if(Gg.ClipLines1) {
						    //
							// Calculate intersection point and draw vector to it
							//
						    Edge3DIntersect(&points[i-1], &points[i], &clip_x, &clip_y, &clip_z);
						    Map3D_XY(clip_x, clip_y, clip_z, &xx0, &yy0);
						    ClipVector(pTerm, xx0, yy0);
					    }
				    }
				    else if(prev == OUTRANGE) {
					    // from outrange to outrange 
					    if(Gg.ClipLines2) {
						    double lx[2], ly[2], lz[2]; /* two edge points */
						    //
						    // Calculate the two 3D intersection points if present
						    //
						    if(TwoEdge3DIntersect(&points[i-1], &points[i], lx, ly, lz)) {
							    Map3D_XY(lx[0], ly[0], lz[0], &x, &y);
							    Map3D_XY(lx[1], ly[1], lz[1], &xx0, &yy0);
							    clip_move(x, y);
							    ClipVector(pTerm, xx0, yy0);
						    }
					    }
				    }
				    break;
			    }
				case UNDEFINED: {
				    break;
			    }
				default:
				    IntWarn(NO_CARET, "Unknown point type in plot3d_lines");
			}
			prev = points[i].type;
		}
		icrvs = icrvs->next;
	}
}
// 
// this is basically the same function as above, but:
//   - it splits the bunch of scans in two sets corresponding to
//   the two scan directions.
//   - reorders the two sets -- from behind to front
//   - checks if inside on scan of a set the order should be inverted
// 
//static void plot3d_lines_pm3d(GpTermEntry * pTerm, GpSurfacePoints * plot)
void GnuPlot::Plot3DLinesPm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	iso_curve ** icrvs_pair[2];
	int invert[2] = {0, 0};
	int n[2] = {0, 0};
	int i, set, scan;
	int x, y, xx0, yy0; /* point in terminal coordinates */
	double clip_x, clip_y, clip_z;
	GpCoordinate * points;
	enum coord_type prev = UNDEFINED;
	double z;
	// just a shortcut 
	bool color_from_column = pPlot->pm3d_color_from_column;
	// If plot really uses RGB rather than pm3d colors, let plot3d_lines take over 
	if(pPlot->lp_properties.pm3d_color.type == TC_RGB) {
		ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
		Plot3DLines(pTerm, pPlot);
		return;
	}
	else if(pPlot->lp_properties.pm3d_color.type == TC_LT) {
		Plot3DLines(pTerm, pPlot);
		return;
	}
	else if(pPlot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
		Plot3DLines(pTerm, pPlot);
		return;
	}
	// These are handled elsewhere.  
	if(pPlot->has_grid_topology && hidden3d)
		return;
	// split the bunch of scans in two sets in
	// which the scans are already depth ordered 
	pm3d_rearrange_scan_array(pPlot, icrvs_pair, &n[0], &invert[0], icrvs_pair + 1, &n[1], &invert[1]);
	for(set = 0; set < 2; set++) {
		int begin = 0;
		int step;
		if(invert[set]) {
			// begin is set below to the length of the scan - 1 
			step = -1;
		}
		else {
			step = 1;
		}
		for(scan = 0; scan < n[set] && icrvs_pair[set]; scan++) {
			int cnt;
			iso_curve * icrvs = icrvs_pair[set][scan];
			if(invert[set]) {
				begin = icrvs->p_count - 1;
			}
			prev = UNDEFINED; /* type of previous plot */
			for(cnt = 0, i = begin, points = icrvs->points; cnt < icrvs->p_count; cnt++, i += step) {
				switch(points[i].type) {
					case INRANGE:
					    Map3D_XY(points[i].x, points[i].y, points[i].z, &x, &y);
					    if(prev == INRANGE) {
						    if(color_from_column)
							    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
						    else
							    z =  (points[i - step].z + points[i].z) * 0.5;
						    set_color(pTerm, Cb2Gray(z));
						    ClipVector(pTerm, x, y);
					    }
					    else {
						    if(prev == OUTRANGE) {
							    // from outrange to inrange 
							    if(!Gg.ClipLines1) {
								    clip_move(x, y);
							    }
							    else {
								    /*
								     * Calculate intersection point and draw
								     * vector from there
								     */
								    Edge3DIntersect(&points[i-step], &points[i], &clip_x, &clip_y, &clip_z);
								    Map3D_XY(clip_x, clip_y, clip_z, &xx0, &yy0);
								    clip_move(xx0, yy0);
								    if(color_from_column)
									    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
								    else
									    z =  (points[i - step].z + points[i].z) * 0.5;
								    set_color(pTerm, Cb2Gray(z));
								    ClipVector(pTerm, x, y);
							    }
						    }
						    else {
							    clip_move(x, y);
						    }
					    }
					    break;
					case OUTRANGE:
					    if(prev == INRANGE) {
						    // from inrange to outrange 
						    if(Gg.ClipLines1) {
							    //
							    // Calculate intersection point and draw vector to it
							    //
							    Edge3DIntersect(&points[i-step], &points[i], &clip_x, &clip_y, &clip_z);
							    Map3D_XY(clip_x, clip_y, clip_z, &xx0, &yy0);
							    if(color_from_column)
								    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
							    else
								    z =  (points[i - step].z + points[i].z) * 0.5;
							    set_color(pTerm, Cb2Gray(z));
							    ClipVector(pTerm, xx0, yy0);
						    }
					    }
					    else if(prev == OUTRANGE) {
						    // from outrange to outrange 
						    if(Gg.ClipLines2) {
							    //
							    // Calculate the two 3D intersection points if present
							    //
							    double lx[2], ly[2], lz[2];
							    if(TwoEdge3DIntersect(&points[i-step], &points[i], lx, ly, lz)) {
								    Map3D_XY(lx[0], ly[0], lz[0], &x, &y);
								    Map3D_XY(lx[1], ly[1], lz[1], &xx0, &yy0);
								    clip_move(x, y);
								    if(color_from_column)
									    z =  (points[i - step].CRD_COLOR + points[i].CRD_COLOR) * 0.5;
								    else
									    z =  (points[i - step].z + points[i].z) * 0.5;
								    set_color(pTerm, Cb2Gray(z) );
								    ClipVector(pTerm, xx0, yy0);
							    }
						    }
					    }
					    break;
					case UNDEFINED:
					    break;
					default:
					    IntWarn(NO_CARET, "Unknown point type in plot3d_lines");
				}
				prev = points[i].type;
			} /* one scan */
		} /* while (icrvs)  */
	} /* for (scan = 0; scan < 2; scan++) */
	SAlloc::F(icrvs_pair[0]);
	SAlloc::F(icrvs_pair[1]);
}
// 
// Plot the surfaces in POINTSTYLE style
// 
//static void plot3d_points(GpTermEntry * pTerm, GpSurfacePoints * plot)
void GnuPlot::Plot3DPoints(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int i;
	int x, y;
	iso_curve * icrvs = pPlot->iso_crvs;
	int interval = pPlot->lp_properties.p_interval;
	// Set whatever we can that applies to every point in the loop 
	if(pPlot->lp_properties.PtType == PT_CHARACTER) {
		ignore_enhanced(TRUE);
		if(pPlot->labels->font && pPlot->labels->font[0])
			(pTerm->set_font)(pTerm, pPlot->labels->font);
		(pTerm->justify_text)(CENTRE);
	}
	while(icrvs) {
		GpCoordinate * point;
		int colortype = pPlot->lp_properties.pm3d_color.type;
		const char * ptchar;
		// Apply constant color outside of the loop 
		if(pPlot->plot_style == CIRCLES)
			SetRgbColorConst(pTerm, pPlot->fill_properties.border_color.lt);
		else if(colortype == TC_RGB)
			SetRgbColorConst(pTerm, pPlot->lp_properties.pm3d_color.lt);
		for(i = 0; i < icrvs->p_count; i++) {
			// Only print 1 point per interval 
			if((pPlot->plot_style == LINESPOINTS) && (interval) && (i % interval))
				continue;
			point = &(icrvs->points[i]);
			if(point->type == INRANGE) {
				Map3D_XY(point->x, point->y, point->z, &x, &y);
				if(!V.ClipPoint(x, y)) {
					// A negative interval indicates we should blank 
					// out the area behind the point symbol          
					if(pPlot->plot_style == LINESPOINTS && interval < 0) {
						(pTerm->set_color)(pTerm, &background_fill);
						(pTerm->pointsize)(Gg.PointSize * Gg.PointIntervalBox);
						(pTerm->point)(pTerm, x, y, 6);
						TermApplyLpProperties(pTerm, &(pPlot->lp_properties));
					}
					Check3DForVariableColor(pTerm, pPlot, point);
					if((pPlot->plot_style == POINTSTYLE || pPlot->plot_style == LINESPOINTS) && pPlot->lp_properties.PtSize == PTSZ_VARIABLE)
						(pTerm->pointsize)(Gg.PointSize * point->CRD_PTSIZE);
					// We could dummy up circles as a point of type 7, but this way 
					// the radius can use x-axis coordinates rather than pointsize. 
					// FIXME: track per-plot fillstyle 
					if(pPlot->plot_style == CIRCLES) {
						double radius = point->CRD_PTSIZE * radius_scaler;
						DoArc(pTerm, x, y, radius, 0.0, 360.0, style_from_fill(&default_fillstyle), FALSE);
						// Retrace the border if the style requests it 
						if(NeedFillBorder(pTerm, &default_fillstyle))
							DoArc(pTerm, x, y, radius, 0., 360., 0, FALSE);
						continue;
					}
					// This code is also used for "splot ... with dots" 
					if(pPlot->plot_style == DOTS) {
						(pTerm->point)(pTerm, x, y, -1);
						continue;
					}
					// variable point type 
					if((pPlot->lp_properties.PtType == PT_VARIABLE) && !(isnan(point->CRD_PTTYPE))) {
						(pTerm->point)(pTerm, x, y, (int)(point->CRD_PTTYPE) - 1);
					}
					// Print special character rather than drawn symbol 
					if(pPlot->lp_properties.PtType == PT_CHARACTER)
						ptchar = pPlot->lp_properties.p_char;
					else if(pPlot->lp_properties.PtType == PT_VARIABLE && isnan(point->CRD_PTTYPE))
						ptchar = (char*)(&point->CRD_PTCHAR);
					else
						ptchar = NULL;
					if(ptchar) {
						if(pPlot->labels)
							ApplyPm3DColor(pTerm, &(pPlot->labels->textcolor));
						(pTerm->put_text)(pTerm, x, y, ptchar);
					}
					// The normal case 
					else if(pPlot->lp_properties.PtType >= -1)
						(pTerm->point)(pTerm, x, y, pPlot->lp_properties.PtType);
				}
			}
		}
		icrvs = icrvs->next;
	}
	// Return to initial state 
	if(pPlot->lp_properties.PtType == PT_CHARACTER) {
		if(pPlot->labels->font && pPlot->labels->font[0])
			(pTerm->set_font)(pTerm, "");
		ignore_enhanced(FALSE);
	}
}
// 
// cntr3d_impulses:
// Plot a surface contour in IMPULSES style
// 
//static void cntr3d_impulses(gnuplot_contours * cntr, lp_style_type * lp)
void GnuPlot::Ñntr3DImpulses(GpTermEntry * pTerm, gnuplot_contours * cntr, lp_style_type * lp)
{
	GpVertex vertex_on_surface;
	GpVertex vertex_on_base;
	if(draw_contour & CONTOUR_SRF) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &vertex_on_surface);
			Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, 0.0, &vertex_on_base);
			// HBB 20010822: Provide correct color-coding for "linetype palette" PM3D mode 
			vertex_on_base.real_z = cntr->coords[i].z;
			Draw3DLine(pTerm, &vertex_on_surface, &vertex_on_base, lp);
		}
	}
	else
		Cntr3DPoints(pTerm, cntr, lp); // Must be on base grid, so do points. 
}
//
// cntr3d_lines:
// Plot a surface contour in LINES style
//
//static void cntr3d_lines(gnuplot_contours * cntr, struct lp_style_type * lp)
void GnuPlot::Ñntr3DLines(GpTermEntry * pTerm, gnuplot_contours * cntr, struct lp_style_type * lp)
{
	int i; // point index 
	GpVertex this_vertex;
	// In the case of "set view map" (only) clip the contour lines to the graph 
	BoundingBox * clip_save = V.P_ClipArea;
	if(splot_map)
		V.P_ClipArea = &V.BbPlot;
	if(draw_contour & CONTOUR_SRF) {
		Map3D_XYZ(cntr->coords[0].x, cntr->coords[0].y, cntr->coords[0].z, &this_vertex);
		// move slightly frontward, to make sure the contours are
		// visible in front of the the triangles they're in, if this is a hidden3d plot 
		if(hidden3d && !VERTEX_IS_UNDEFINED(this_vertex))
			this_vertex.z += 1e-2;
		Polyline3DStart(pTerm, &this_vertex);
		for(i = 1; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &this_vertex);
			// move slightly frontward, to make sure the contours are
			// visible in front of the the triangles they're in, if this
			// is a hidden3d plot */
			if(hidden3d && !VERTEX_IS_UNDEFINED(this_vertex))
				this_vertex.z += 1e-2;
			Polyline3DNext(pTerm, &this_vertex, lp);
		}
	}
	if(draw_contour & CONTOUR_BASE) {
		Map3D_XYZ(cntr->coords[0].x, cntr->coords[0].y, base_z, &this_vertex);
		this_vertex.real_z = cntr->coords[0].z;
		Polyline3DStart(pTerm, &this_vertex);
		for(i = 1; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, base_z, &this_vertex);
			this_vertex.real_z = cntr->coords[i].z;
			Polyline3DNext(pTerm, &this_vertex, lp);
		}
	}
	if(splot_map)
		V.P_ClipArea = clip_save;
}
//
// cntr3d_points:
// Plot a surface contour in POINTSTYLE style
//
//static void cntr3d_points(gnuplot_contours * cntr, struct lp_style_type * lp)
void GnuPlot::Cntr3DPoints(GpTermEntry * pTerm, gnuplot_contours * cntr, lp_style_type * lp)
{
	GpVertex v;
	if(draw_contour & CONTOUR_SRF) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, cntr->coords[i].z, &v);
			// move slightly frontward, to make sure the contours and
			// points are visible in front of the triangles they're
			// in, if this is a hidden3d plot 
			if(hidden3d && !VERTEX_IS_UNDEFINED(v))
				v.z += 1e-2;
			Draw3DPoint(pTerm, &v, lp);
		}
	}
	if(draw_contour & CONTOUR_BASE) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, base_z, &v);
			// HBB 20010822: see above 
			v.real_z = cntr->coords[i].z;
			Draw3DPoint(pTerm, &v, lp);
		}
	}
}
// 
// cntr3d_labels:
// Place contour labels on a contour line at the base.
// These are the same labels that would be used in the key.
// The label density is controlled by the point interval property
//   splot FOO with labels point pi 20 nosurface
// 
//static void cntr3d_labels(gnuplot_contours * cntr, char * level_text, struct text_label * label)
void GnuPlot::Cntr3DLabels(GpTermEntry * pTerm, gnuplot_contours * cntr, char * pLevelText, text_label * pLabel)
{
	int i;
	int x, y;
	GpVertex v;
	lp_style_type * lp = &(pLabel->lp_properties);
	// Drawing a label at every point would be too crowded 
	int interval = lp->p_interval;
	if(interval <= 0) 
		interval = 999; /* Place label only at start point */
	if(draw_contour & CONTOUR_BASE) {
		for(i = 0; i < cntr->num_pts; i++) {
			if((i-clabel_start) % interval) /* Offset to avoid sitting on the border */
				continue;
			Map3D_XY(cntr->coords[i].x, cntr->coords[i].y, base_z, &x, &y);
			pLabel->text = pLevelText;
			pLabel->font = clabel_font;
			if(hidden3d) {
				Map3D_XYZ(cntr->coords[i].x, cntr->coords[i].y, base_z, &v);
				v.real_z = cntr->coords[i].z;
				v.label = pLabel;
				DrawLabelHidden(pTerm, &v, lp, x, y);
			}
			else
				WriteLabel(pTerm, x, y, pLabel);
			pLabel->text = NULL; // Otherwise someone will try to free it 
			pLabel->font = NULL;
		}
	}
}
// 
// map xmin | xmax to 0 | 1 and same for y
// 0.1 avoids any rounding errors
// 
#define MAP_HEIGHT_X(x) (((x)-AxS.__X().min)/(AxS.__X().max-AxS.__X().min) > 0.9 ? 1 : 0)
#define MAP_HEIGHT_Y(y) (((y)-AxS.__Y().min)/(AxS.__Y().max-AxS.__Y().min) > 0.9 ? 1 : 0)
// 
// if point is at corner, update height[][] and depth[][]
// we are still assuming that extremes of surfaces are at corners,
// but we are not assuming order of corners
// 
//static void check_corner_height(GpCoordinate * p, double height[2][2], double depth[2][2])
void GnuPlot::CheckCornerHeight(GpCoordinate * p, double height[2][2], double depth[2][2])
{
	if(p->type == INRANGE) {
		// FIXME HBB 20010121: don't compare 'zero' to data values in absolute terms. 
		if((fabs(p->x - AxS.__X().min) < Gg.Zero || fabs(p->x - AxS.__X().max) < Gg.Zero) && (fabs(p->y - AxS.__Y().min) < Gg.Zero || fabs(p->y - AxS.__Y().max) < Gg.Zero)) {
			int x = MAP_HEIGHT_X(p->x);
			int y = MAP_HEIGHT_Y(p->y);
			if(height[x][y] < p->z)
				height[x][y] = p->z;
			if(depth[x][y] > p->z)
				depth[x][y] = p->z;
		}
	}
}
//
// work out where the axes and tics are drawn 
//
//static void setup_3d_box_corners()
void GnuPlot::Setup3DBoxCorners()
{
	int quadrant = static_cast<int>(surface_rot_z / 90.0f);
	if((quadrant + 1) & 2) {
		_3DBlk.ZAxisX = AxS.__X().max;
		_3DBlk.Right.x = AxS.__X().min;
		_3DBlk.Back.y  = AxS.__Y().min;
		_3DBlk.Front.y  = AxS.__Y().max;
	}
	else {
		_3DBlk.ZAxisX = AxS.__X().min;
		_3DBlk.Right.x = AxS.__X().max;
		_3DBlk.Back.y  = AxS.__Y().max;
		_3DBlk.Front.y  = AxS.__Y().min;
	}
	if(quadrant & 2) {
		_3DBlk.ZAxisY = AxS.__Y().max;
		_3DBlk.Right.y = AxS.__Y().min;
		_3DBlk.Back.x  = AxS.__X().max;
		_3DBlk.Front.x  = AxS.__X().min;
	}
	else {
		_3DBlk.ZAxisY = AxS.__Y().min;
		_3DBlk.Right.y = AxS.__Y().max;
		_3DBlk.Back.x  = AxS.__X().min;
		_3DBlk.Front.x  = AxS.__X().max;
	}
	quadrant = static_cast<int>(surface_rot_x / 90.0f);
	if((quadrant & 2) && !splot_map) {
		Exchange(&_3DBlk.Front.y, &_3DBlk.Back.y);
		Exchange(&_3DBlk.Front.x, &_3DBlk.Back.x);
	}
	if((quadrant + 1) & 2) {
		// labels on the back axes 
		_3DBlk.YAxisX = _3DBlk.Back.x;
		_3DBlk.XAxisY = _3DBlk.Back.y;
	}
	else {
		_3DBlk.YAxisX = _3DBlk.Front.x;
		_3DBlk.XAxisY = _3DBlk.Front.y;
	}
}
//
// Draw all elements of the 3d graph box, including borders, zeroaxes,
// tics, gridlines, ticmarks, axis labels and the base plane. 
//
//static void draw_3d_graphbox(GpTermEntry * pTerm, const GpSurfacePoints * plot, int plot_num, WHICHGRID whichgrid, int current_layer)
void GnuPlot::Draw3DGraphBox(GpTermEntry * pTerm, const GpSurfacePoints * pPlot, int plotNum, WHICHGRID whichgrid, int currentLayer)
{
	int    x, y; // point in terminal coordinates 
	BoundingBox * clip_save = V.P_ClipArea;
	FPRINTF((stderr, "draw_3d_graphbox: whichgrid = %d current_layer = %d border_layer = %d\n", whichgrid, currentLayer, border_layer));
	V.P_ClipArea = &V.BbCanvas;
	if(draw_border && splot_map) {
		if(border_layer == currentLayer) {
			TermApplyLpProperties(pTerm, &border_lp);
			if((draw_border & 15) == 15)
				newpath(pTerm);
			Map3D_XY(_3DBlk.ZAxisX, _3DBlk.ZAxisY, base_z, &x, &y);
			clip_move(x, y);
			Map3D_XY(_3DBlk.Back.x, _3DBlk.Back.y, base_z, &x, &y);
			if(draw_border & 2)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(_3DBlk.Right.x, _3DBlk.Right.y, base_z, &x, &y);
			if(draw_border & 8)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(_3DBlk.Front.x, _3DBlk.Front.y, base_z, &x, &y);
			if(draw_border & 4)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(_3DBlk.ZAxisX, _3DBlk.ZAxisY, base_z, &x, &y);
			if(draw_border & 1)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			if((draw_border & 15) == 15)
				closepath(pTerm);
		}
	}
	else if(draw_border && yz_projection) {
		if(border_layer == currentLayer) {
			GpAxis * yaxis = &AxS[FIRST_Y_AXIS];
			GpAxis * zaxis = &AxS[FIRST_Z_AXIS];
			TermApplyLpProperties(pTerm, &border_lp);
			if((draw_border & 15) == 15)
				newpath(pTerm);
			Map3D_XY(0.0, yaxis->min, zaxis->min, &x, &y);
			clip_move(x, y);
			Map3D_XY(0.0, yaxis->max, zaxis->min, &x, &y);
			if(draw_border & 8)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(0.0, yaxis->max, zaxis->max, &x, &y);
			if(draw_border & 4)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(0.0, yaxis->min, zaxis->max, &x, &y);
			if(draw_border & 2)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(0.0, yaxis->min, zaxis->min, &x, &y);
			if(draw_border & 1)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			if((draw_border & 15) == 15)
				closepath(pTerm);
		}
	}
	else if(draw_border && xz_projection) {
		if(border_layer == currentLayer) {
			GpAxis * xaxis = &AxS[FIRST_X_AXIS];
			GpAxis * zaxis = &AxS[FIRST_Z_AXIS];
			TermApplyLpProperties(pTerm, &border_lp);
			if((draw_border & 15) == 15)
				newpath(pTerm);
			Map3D_XY(xaxis->min, 0.0, zaxis->min, &x, &y);
			clip_move(x, y);
			Map3D_XY(xaxis->max, 0.0, zaxis->min, &x, &y);
			if(draw_border & 2)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(xaxis->max, 0.0, zaxis->max, &x, &y);
			if(draw_border & 4)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(xaxis->min, 0.0, zaxis->max, &x, &y);
			if(draw_border & 8)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			Map3D_XY(xaxis->min, 0.0, zaxis->min, &x, &y);
			if(draw_border & 1)
				ClipVector(pTerm, x, y);
			else
				clip_move(x, y);
			if((draw_border & 15) == 15)
				closepath(pTerm);
		}
	}
	else if(draw_border) {
		// the four corners of the base plane, in normalized view coordinates (-1..1) on all three axes. 
		GpVertex bl;
		GpVertex bb;
		GpVertex br;
		GpVertex bf;
		// map to normalized view coordinates the corners of the baseplane: left, back, right and front, in that order: 
		Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, base_z, &bl);
		Map3D_XYZ(_3DBlk.Back.x, _3DBlk.Back.y, base_z, &bb);
		Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, base_z, &br);
		Map3D_XYZ(_3DBlk.Front.x, _3DBlk.Front.y, base_z, &bf);
		if(BACKGRID != whichgrid) {
			// Draw front part of base grid, right to front corner: 
			if(draw_border & 4)
				Draw3DLine(pTerm, &br, &bf, &border_lp);
			// ... and left to front: 
			if(draw_border & 1)
				Draw3DLine(pTerm, &bl, &bf, &border_lp);
		}
		if(FRONTGRID != whichgrid) {
			// Draw back part of base grid: left to back corner: 
			if(draw_border & 2)
				Draw3DLine(pTerm, &bl, &bb, &border_lp);
			// ... and right to back: 
			if(draw_border & 8)
				Draw3DLine(pTerm, &br, &bb, &border_lp);
		}
		/* if surface is drawn, draw the rest of the graph box, too: */
		if(draw_surface || (draw_contour & CONTOUR_SRF) || (pm3d.implicit == PM3D_IMPLICIT && strpbrk(pm3d.where, "st") != NULL)) {
			GpVertex fl, fb, fr, ff; /* floor left/back/right/front corners */
			GpVertex tl, tb, tr, tf; /* top left/back/right/front corners */
			Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, floor_z, &fl);
			Map3D_XYZ(_3DBlk.Back.x, _3DBlk.Back.y, floor_z, &fb);
			Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, floor_z, &fr);
			Map3D_XYZ(_3DBlk.Front.x, _3DBlk.Front.y, floor_z, &ff);
			Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, ceiling_z, &tl);
			Map3D_XYZ(_3DBlk.Back.x, _3DBlk.Back.y, ceiling_z, &tb);
			Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, ceiling_z, &tr);
			Map3D_XYZ(_3DBlk.Front.x, _3DBlk.Front.y, ceiling_z, &tf);
			if((draw_border & 0xf0) == 0xf0) {
				// all four verticals are drawn - save some time by
				// drawing them to the full height, regardless of
				// where the surface lies 
				if(FRONTGRID != whichgrid) {
					// Draw the back verticals floor-to-ceiling, left: 
					Draw3DLine(pTerm, &fl, &tl, &border_lp);
					// ... back: 
					Draw3DLine(pTerm, &fb, &tb, &border_lp);
					// ... and right 
					Draw3DLine(pTerm, &fr, &tr, &border_lp);
				}
				if(BACKGRID != whichgrid) {
					// Draw the front vertical: floor-to-ceiling, front: 
					Draw3DLine(pTerm, &ff, &tf, &border_lp);
				}
			}
			else {
				// find heights of surfaces at the corners of the xy rectangle 
				double height[2][2];
				double depth[2][2];
				int zaxis_i = MAP_HEIGHT_X(_3DBlk.ZAxisX);
				int zaxis_j = MAP_HEIGHT_Y(_3DBlk.ZAxisY);
				int back_i = MAP_HEIGHT_X(_3DBlk.Back.x);
				int back_j = MAP_HEIGHT_Y(_3DBlk.Back.y);
				height[0][0] = height[0][1] = height[1][0] = height[1][1] = base_z;
				depth[0][0] = depth[0][1] = depth[1][0] = depth[1][1] = base_z;
				//
				// FIXME HBB 20000617: this method contains the
				// assumption that the topological corners of the
				// surface mesh(es) are also the geometrical ones of
				// their xy projections. This is only true for
				// 'explicit' surface datasets, i.e. z(x,y) 
				//
				if(Gg.CornerPoles) {
					for(; --plotNum >= 0; pPlot = pPlot->next_sp) {
						iso_curve * curve = pPlot->iso_crvs;
						int count;
						int iso;
						if(pPlot->plot_type == NODATA || pPlot->plot_type == KEYENTRY)
							continue;
						if(pPlot->plot_type == VOXELDATA)
							continue;
						if(pPlot->plot_type == DATA3D) {
							if(!pPlot->has_grid_topology)
								continue;
							iso = pPlot->num_iso_read;
						}
						else
							iso = Gg.IsoSamples2;
						count = curve->p_count;
						if(count == 0)
							continue;
						CheckCornerHeight(curve->points, height, depth);
						CheckCornerHeight(curve->points + count - 1, height, depth);
						while(--iso)
							curve = curve->next;
						CheckCornerHeight(curve->points, height, depth);
						CheckCornerHeight(curve->points + count - 1, height, depth);
					}
				}

#define VERTICAL(mask, x, y, i, j, bottom, top)                       \
	if(draw_border&mask) {                         \
		Draw3DLine(pTerm, bottom, top, &border_lp);        \
	} \
	else if(height[i][j] != depth[i][j] && (AxS.__X().ticmode || AxS.__Y().ticmode || draw_border & 0x00F)) { \
		GpVertex a, b;                                \
		Map3D_XYZ(x, y, depth[i][j], &a);              \
		Map3D_XYZ(x, y, height[i][j], &b);             \
		Draw3DLine(pTerm, &a, &b, &border_lp);            \
	}
				if(FRONTGRID != whichgrid) {
					VERTICAL(0x10, _3DBlk.ZAxisX, _3DBlk.ZAxisY, zaxis_i, zaxis_j, &fl, &tl); /* Draw back verticals: floor-to-ceiling left: */
					VERTICAL(0x20, _3DBlk.Back.x, _3DBlk.Back.y, back_i, back_j, &fb, &tb); /* ... back: */
					VERTICAL(0x40, _3DBlk.Right.x, _3DBlk.Right.y, 1 - zaxis_i, 1 - zaxis_j, &fr, &tr); /* ... and right: */
				}
				if(BACKGRID != whichgrid) {
					VERTICAL(0x80, _3DBlk.Front.x, _3DBlk.Front.y, 1 - back_i, 1 - back_j, &ff, &tf); /* Draw front verticals: floor-to-ceiling front */
				}
#undef VERTICAL
			} /* else (all 4 verticals drawn?) */
			/* now border lines on top */
			if(FRONTGRID != whichgrid) {
				/* Draw back part of top of box: top left to back corner: */
				if(draw_border & 0x100)
					Draw3DLine(pTerm, &tl, &tb, &border_lp);
				/* ... and top right to back: */
				if(draw_border & 0x200)
					Draw3DLine(pTerm, &tr, &tb, &border_lp);
			}
			if(BACKGRID != whichgrid) {
				/* Draw front part of top of box: top left to front corner: */
				if(draw_border & 0x400)
					Draw3DLine(pTerm, &tl, &tf, &border_lp);
				/* ... and top right to front: */
				if(draw_border & 0x800)
					Draw3DLine(pTerm, &tr, &tf, &border_lp);
			}
		} /* else (surface is drawn) */
	} /* if (draw_border) */
	// In 'set view map' mode, treat grid as in 2D plots 
	if(splot_map && currentLayer != abs(grid_layer)) {
		V.P_ClipArea = clip_save;
		return;
	}
	if(whichgrid == BORDERONLY) {
		V.P_ClipArea = clip_save;
		return;
	}
	// Draw ticlabels and axis labels 
	// x axis 
	if((AxS.__X().ticmode || AxS.__X().label.text) && !_3DBlk.YzPlane) {
		GpVertex v0, v1;
		double other_end = AxS.__Y().min + AxS.__Y().max - _3DBlk.XAxisY;
		double mid_x;
		if(AxS.__X().IsNonLinear()) {
			GpAxis * primary = AxS.__X().linked_to_primary;
			mid_x = (primary->max + primary->min) / 2.;
			mid_x = EvalLinkFunction(&AxS.__X(), mid_x);
		}
		else {
			mid_x = (AxS.__X().max + AxS.__X().min) / 2.;
		}
		Map3D_XYZ(mid_x, _3DBlk.XAxisY, base_z, &v0);
		Map3D_XYZ(mid_x, other_end, base_z, &v1);
		// Unusual case: 2D projection of the xz plane 
		if(!splot_map && _3DBlk.XzPlane)
			Map3D_XYZ(mid_x, _3DBlk.XAxisY, AxS.__Z().max+AxS.__Z().min-base_z, &v1);
		//tic_unitx = (v1.x - v0.x) / xyscaler;
		//tic_unity = (v1.y - v0.y) / xyscaler;
		//tic_unitz = (v1.z - v0.z) / xyscaler;
		_3DBlk.TicUnit.Set((v1.x - v0.x) / xyscaler, (v1.y - v0.y) / xyscaler, (v1.z - v0.z) / xyscaler);
		// Don't output tics and grids if this is the front part of a
		// two-part grid drawing process: 
		if((surface_rot_x <= 90 && FRONTGRID != whichgrid) || (surface_rot_x > 90 && BACKGRID != whichgrid))
			if(AxS.__X().ticmode)
				GenTics(pTerm, &AxS[FIRST_X_AXIS], &GnuPlot::XTickCallback);
		if(AxS.__X().label.text) {
			if((surface_rot_x <= 90 && BACKGRID != whichgrid) || (surface_rot_x > 90 && FRONTGRID != whichgrid) || splot_map) {
				int x1, y1;
				if(splot_map) { /* case 'set view map' */
					// copied from xtick_callback(): baseline of tics labels 
					GpVertex v1, v2;
					Map3D_XYZ(mid_x, _3DBlk.XAxisY, base_z, &v1);
					v2.x = v1.x;
					v2.y = v1.y - _3DBlk.TicUnit.y * pTerm->ChrV;
					if(!AxS.__X().TicIn)
						v2.y -= _3DBlk.TicUnit.y * pTerm->TicV * AxS.__X().ticscale;
					TERMCOORD(&v2, x1, y1);
					// Default displacement with respect to baseline of tics labels 
					y1 -= (1.5 * pTerm->ChrV);
				}
				else { // usual 3d set view ... 
					if(AxS.__X().label.tag == ROTATE_IN_3D_LABEL_TAG) {
						double ang, angx0, angx1, angy0, angy1;
						Map3D_XY_double(AxS.__X().min, _3DBlk.XAxisY, base_z, &angx0, &angy0);
						Map3D_XY_double(AxS.__X().max, _3DBlk.XAxisY, base_z, &angx1, &angy1);
						ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
						if(ang < -90) 
							ang += 180;
						if(ang > 90) 
							ang -= 180;
						AxS.__X().label.rotate = (ang > 0) ? ffloori(ang + 0.5) : ffloori(ang - 0.5);
					}
					if(AxS.__X().ticmode & TICS_ON_AXIS) {
						Map3D_XYZ(mid_x, 0.0, base_z, &v1);
					}
					else {
						Map3D_XYZ(mid_x, _3DBlk.XAxisY, base_z, &v1);
					}
					if(xz_projection) {
						v1.x -= 3.0 * pTerm->TicH * _3DBlk.TicUnit.x;
						v1.y -= 3.0 * pTerm->TicH * _3DBlk.TicUnit.y;
					}
					else if(AxS.__X().ticmode & TICS_ON_AXIS) {
						v1.x += 2.0 * pTerm->TicH * ((AxS.__X().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.x;
						v1.y += 2.0 * pTerm->TicH * ((AxS.__X().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.y;
					}
					else {
						v1.x -= 10.0 * pTerm->TicH * _3DBlk.TicUnit.x;
						v1.y -= 10.0 * pTerm->TicH * _3DBlk.TicUnit.y;
					}
					if(!AxS.__X().TicIn) {
						v1.x -= _3DBlk.TicUnit.x * AxS.__X().ticscale * pTerm->TicH;
						v1.y -= _3DBlk.TicUnit.y * AxS.__X().ticscale * pTerm->TicH;
					}
					TERMCOORD(&v1, x1, y1);
				}
				WriteLabel(pTerm, x1, y1, &AxS.__X().label);
			}
		}
		if(splot_map && AxS[SECOND_X_AXIS].ticmode)
			GenTics(pTerm, &AxS[SECOND_X_AXIS], &GnuPlot::XTickCallback);
	}
	// y axis 
	if((AxS.__Y().ticmode || AxS.__Y().label.text) && !_3DBlk.XzPlane) {
		GpVertex v0, v1;
		double other_end = AxS.__X().min + AxS.__X().max - _3DBlk.YAxisX;
		double mid_y;
		if(AxS.__Y().IsNonLinear()) {
			GpAxis * primary = AxS.__Y().linked_to_primary;
			mid_y = (primary->max + primary->min) / 2.;
			mid_y = EvalLinkFunction(&AxS.__Y(), mid_y);
		}
		else {
			mid_y = (AxS.__Y().max + AxS.__Y().min) / 2.;
		}
		Map3D_XYZ(_3DBlk.YAxisX, mid_y, base_z, &v0);
		Map3D_XYZ(other_end, mid_y, base_z, &v1);
		// Unusual case: 2D projection of the yz plane 
		if(!splot_map && _3DBlk.YzPlane)
			Map3D_XYZ(_3DBlk.YAxisX, mid_y, AxS.__Z().max+AxS.__Z().min-base_z, &v1);
		//_3DBlk.TicUnit.x = (v1.x - v0.x) / xyscaler;
		//_3DBlk.TicUnit.y = (v1.y - v0.y) / xyscaler;
		//_3DBlk.TicUnit.z = (v1.z - v0.z) / xyscaler;
		_3DBlk.TicUnit.Set((v1.x - v0.x) / xyscaler, (v1.y - v0.y) / xyscaler, (v1.z - v0.z) / xyscaler);
		// Don't output tics and grids if this is the front part of a two-part grid drawing process: 
		if((surface_rot_x <= 90 && FRONTGRID != whichgrid) || (surface_rot_x > 90 && BACKGRID != whichgrid))
			if(AxS.__Y().ticmode)
				GenTics(pTerm, &AxS[FIRST_Y_AXIS], &GnuPlot::YTickCallback);
		if(AxS.__Y().label.text) {
			if((surface_rot_x <= 90 && BACKGRID != whichgrid) || (surface_rot_x > 90 && FRONTGRID != whichgrid) || splot_map) {
				int x1, y1;
				int save_rotate = AxS.__Y().label.rotate;
				if(splot_map) { /* case 'set view map' */
					// copied from ytick_callback(): baseline of tics labels 
					GpVertex v1, v2;
					Map3D_XYZ(_3DBlk.YAxisX, mid_y, base_z, &v1);
					if(AxS.__Y().ticmode & TICS_ON_AXIS && !AxS.__X().log && AxS.__X().InRange(0.0)) {
						Map3D_XYZ(0.0, _3DBlk.YAxisX, base_z, &v1);
					}
					v2.x = v1.x - _3DBlk.TicUnit.x * pTerm->ChrH * 1;
					v2.y = v1.y;
					if(!AxS.__X().TicIn)
						v2.x -= _3DBlk.TicUnit.x * pTerm->TicH * AxS.__X().ticscale;
					TERMCOORD(&v2, x1, y1);
					// calculate max length of y-tics labels 
					widest_tic_strlen = 0;
					if(AxS.__Y().ticmode & TICS_ON_BORDER) {
						widest_tic_strlen = 0; // reset the global variable 
						GenTics(pTerm, &AxS[FIRST_Y_AXIS], &GnuPlot::WidestTicCallback);
					}
					// Default displacement with respect to baseline of tics labels 
					x1 -= (0.5 + widest_tic_strlen) * pTerm->ChrH;
				}
				else { // usual 3d set view ...
					if(AxS.__Y().label.tag == ROTATE_IN_3D_LABEL_TAG) {
						double ang, angx0, angx1, angy0, angy1;
						Map3D_XY_double(_3DBlk.YAxisX, AxS.__Y().min, base_z, &angx0, &angy0);
						Map3D_XY_double(_3DBlk.YAxisX, AxS.__Y().max, base_z, &angx1, &angy1);
						ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
						if(ang < -90) 
							ang += 180;
						if(ang > 90) 
							ang -= 180;
						AxS.__Y().label.rotate = (ang > 0.0) ? ffloori(ang + 0.5) : ffloori(ang - 0.5);
					}
					else if(!yz_projection) {
						// The 2D default state (ylabel rotate) is not wanted in 3D 
						AxS.__Y().label.rotate = 0;
					}
					if(AxS.__Y().ticmode & TICS_ON_AXIS) {
						Map3D_XYZ(0.0, mid_y, base_z, &v1);
					}
					else {
						Map3D_XYZ(_3DBlk.YAxisX, mid_y, base_z, &v1);
					}
					if(yz_projection) {
						v1.x -= 3.0 * pTerm->TicH * _3DBlk.TicUnit.x;
						v1.y -= 3.0 * pTerm->TicH * _3DBlk.TicUnit.y;
					}
					else if(AxS.__Y().ticmode & TICS_ON_AXIS) {
						v1.x += 2.0 * pTerm->TicH * ((AxS.__Y().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.x;
						v1.y += 2.0 * pTerm->TicH * ((AxS.__Y().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.y;
					}
					else {
						v1.x -= 10.0 * pTerm->TicH * _3DBlk.TicUnit.x;
						v1.y -= 10.0 * pTerm->TicH * _3DBlk.TicUnit.y;
					}

					if(!AxS.__Y().TicIn) {
						v1.x -= _3DBlk.TicUnit.x * AxS.__Y().ticscale * pTerm->TicV;
						v1.y -= _3DBlk.TicUnit.y * AxS.__Y().ticscale * pTerm->TicV;
					}
					TERMCOORD(&v1, x1, y1);
				}
				WriteLabel(pTerm, x1, y1, &AxS.__Y().label);
				AxS.__Y().label.rotate = save_rotate;
			}
		}
		if(splot_map && AxS[SECOND_Y_AXIS].ticmode)
			GenTics(pTerm, &AxS[SECOND_Y_AXIS], &GnuPlot::YTickCallback);
	}
	// do z tics 
	if(AxS.__Z().ticmode &&
	    // Don't output tics and grids if this is the front part of a two-part grid drawing process: 
	    (FRONTGRID != whichgrid) && (splot_map == FALSE) && (surface_rot_x != 0) && (draw_surface || (draw_contour & CONTOUR_SRF) || strchr(pm3d.where, 's'))) {
		GenTics(pTerm, &AxS[FIRST_Z_AXIS], &GnuPlot::ZTickCallback);
	}
	if((AxS.__Y().zeroaxis) && !AxS.__X().log && AxS.__X().InRange(0.0)) {
		GpVertex v1, v2;
		// line through x=0 
		Map3D_XYZ(0.0, AxS.__Y().min, base_z, &v1);
		Map3D_XYZ(0.0, AxS.__Y().max, base_z, &v2);
		Draw3DLine(pTerm, &v1, &v2, AxS.__Y().zeroaxis);
	}
	if((AxS.__Z().zeroaxis) && !AxS.__X().log && AxS.__X().InRange(0.0)) {
		GpVertex v1, v2;
		// line through x=0 y=0 
		Map3D_XYZ(0.0, 0.0, AxS.__Z().min, &v1);
		Map3D_XYZ(0.0, 0.0, AxS.__Z().max, &v2);
		Draw3DLine(pTerm, &v1, &v2, AxS.__Z().zeroaxis);
	}
	if((AxS.__X().zeroaxis) && !AxS.__Y().log && AxS.__Y().InRange(0.0)) {
		GpVertex v1, v2;
		TermApplyLpProperties(pTerm, AxS.__X().zeroaxis);
		// line through y=0 
		Map3D_XYZ(AxS.__X().min, 0.0, base_z, &v1);
		Map3D_XYZ(AxS.__X().max, 0.0, base_z, &v2);
		Draw3DLine(pTerm, &v1, &v2, AxS.__X().zeroaxis);
	}
	// PLACE ZLABEL - along the middle grid Z axis - eh ? 
	if(AxS.__Z().label.text && !splot_map && (currentLayer == LAYER_FRONT || whichgrid == ALLGRID) && (draw_surface || (draw_contour & CONTOUR_SRF) || strpbrk(pm3d.where, "st") != NULL)) {
		GpVertex v1;
		double mid_z;
		if(AxS.__Z().IsNonLinear()) {
			mid_z = (AxS.__Z().linked_to_primary->max + AxS.__Z().linked_to_primary->min) / 2.;
			mid_z = EvalLinkFunction(&AxS.__Z(), mid_z);
		}
		else
			mid_z = (AxS.__Z().max + AxS.__Z().min) / 2.0;
		if(AxS.__Z().ticmode & TICS_ON_AXIS) {
			Map3D_XYZ(0, 0, mid_z, &v1);
			TERMCOORD(&v1, x, y);
			x -= 5 * pTerm->ChrH;
		}
		else {
			Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, mid_z, &v1);
			TERMCOORD(&v1, x, y);
			if(fabs(azimuth) > 80)
				y += 2 * sgn(azimuth) * pTerm->ChrV;
			else
				x -= 7 * pTerm->ChrH;
		}
		if(AxS.__Z().label.tag == ROTATE_IN_3D_LABEL_TAG) {
			double ang, angx0, angx1, angy0, angy1;
			Map3D_XY_double(_3DBlk.ZAxisX, _3DBlk.ZAxisY, AxS.__Z().min, &angx0, &angy0);
			Map3D_XY_double(_3DBlk.ZAxisX, _3DBlk.ZAxisY, AxS.__Z().max, &angx1, &angy1);
			ang = atan2(angy1-angy0, angx1-angx0) / DEG2RAD;
			if(ang < -90) 
				ang += 180;
			if(ang > 90) 
				ang -= 180;
			AxS.__Z().label.rotate = (ang > 0) ? ffloori(ang + 0.5) : ffloori(ang - 0.5);
		}
		WriteLabel(pTerm, x, y, &AxS.__Z().label);
	}
	V.P_ClipArea = clip_save;
}

//static void xtick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid/* linetype or -2 for none */, ticmark * userlabels)
void GnuPlot::XTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid/* linetype or -2 for none */, ticmark * userlabels)
{
	double scale = tic_scale(ticlevel, pAx) * (pAx->TicIn ? 1 : -1);
	double other_end = AxS.__Y().min + AxS.__Y().max - _3DBlk.XAxisY;
	GpVertex v1, v2, v3, v4;
	// Draw full-length grid line 
	Map3D_XYZ(place, _3DBlk.XAxisY, base_z, &v1);
	if(rGrid.l_type > LT_NODRAW) {
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		// to save mapping twice, map non-axis y 
		Map3D_XYZ(place, other_end, base_z, &v3);
		Draw3DLine(pTerm, &v1, &v3, const_cast<lp_style_type *>(&rGrid)); // @badcast
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	}
	// Vertical grid lines (in yz plane) 
	if(grid_vertical_lines && rGrid.l_type > LT_NODRAW) {
		GpVertex v4, v5;
		double which_face = (surface_rot_x > 90 && surface_rot_x < 270) ? _3DBlk.XAxisY : other_end;
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(place, which_face, AxS.__Z().min, &v4);
		Map3D_XYZ(place, which_face, ceiling_z, &v5);
		Draw3DLine(pTerm, &v4, &v5, const_cast<lp_style_type *>(&rGrid)); // @badcast
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	}
	if((AxS.__X().ticmode & TICS_ON_AXIS) && !AxS.__Y().log && AxS.__Y().InRange(0.0)) {
		Map3D_XYZ(place, 0.0, base_z, &v1);
	}
	// NB: secondary axis must be linked to primary 
	if(pAx->index == SECOND_X_AXIS &&  pAx->linked_to_primary && pAx->link_udf->at != NULL) {
		place = EvalLinkFunction(&AxS[FIRST_X_AXIS], place);
	}
	// Draw bottom tic mark 
	if((pAx->index == FIRST_X_AXIS) || (pAx->index == SECOND_X_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		v2.x = v1.x + _3DBlk.TicUnit.x * scale * pTerm->TicV;
		v2.y = v1.y + _3DBlk.TicUnit.y * scale * pTerm->TicV;
		v2.z = v1.z + _3DBlk.TicUnit.z * scale * pTerm->TicV;
		v2.real_z = v1.real_z;
		Draw3DLine(pTerm, &v1, &v2, &border_lp);
	}
	// Draw top tic mark 
	if((pAx->index == SECOND_X_AXIS) || (pAx->index == FIRST_X_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		if(xz_projection)
			Map3D_XYZ(place, other_end, AxS.__Z().max, &v3);
		else
			Map3D_XYZ(place, other_end, base_z, &v3);
		v4.x = v3.x - _3DBlk.TicUnit.x * scale * pTerm->TicV;
		v4.y = v3.y - _3DBlk.TicUnit.y * scale * pTerm->TicV;
		v4.z = v3.z - _3DBlk.TicUnit.z * scale * pTerm->TicV;
		v4.real_z = v3.real_z;
		Draw3DLine(pTerm, &v3, &v4, &border_lp);
	}
	// Draw tic label 
	if(text) {
		int just;
		int x2, y2;
		int angle;
		int offsetx, offsety;
		// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 0.001
		while(userlabels) {
			if(fabs((place - userlabels->position) / (AxS.__X().max - AxS.__X().min))
			    <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "xtics");
		// allow manual justification of tick labels, but only for projections 
		if((splot_map || xz_projection) && pAx->manual_justify)
			just = pAx->tic_pos;
		else if(_3DBlk.TicUnit.x * xscaler < -0.9)
			just = LEFT;
		else if(_3DBlk.TicUnit.x * xscaler < 0.9)
			just = CENTRE;
		else
			just = RIGHT;
		if(pAx->index == SECOND_X_AXIS) {
			v4.x = v3.x + _3DBlk.TicUnit.x * pTerm->ChrH * 1;
			v4.y = v3.y + _3DBlk.TicUnit.y * pTerm->ChrV * 1;
			if(!pAx->TicIn) {
				v4.x += _3DBlk.TicUnit.x * pTerm->TicV * pAx->ticscale;
				v4.y += _3DBlk.TicUnit.y * pTerm->TicV * pAx->ticscale;
			}
			TERMCOORD(&v4, x2, y2);
		}
		else {
			v2.x = v1.x - _3DBlk.TicUnit.x * pTerm->ChrH * 1;
			v2.y = v1.y - _3DBlk.TicUnit.y * pTerm->ChrV * 1;
			if(!pAx->TicIn) {
				v2.x -= _3DBlk.TicUnit.x * pTerm->TicV * pAx->ticscale;
				v2.y -= _3DBlk.TicUnit.y * pTerm->TicV * pAx->ticscale;
			}
			TERMCOORD(&v2, x2, y2);
		}
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		angle = pAx->tic_rotate;
		if(!(splot_map && angle && pTerm->text_angle(angle)))
			angle = 0;
		ignore_enhanced(!pAx->ticdef.enhanced);
		write_multiline(pTerm, x2+offsetx, y2+offsety, text, (JUSTIFY)just, JUST_TOP, angle, pAx->ticdef.font);
		ignore_enhanced(FALSE);
		pTerm->text_angle(0);
		TermApplyLpProperties(pTerm, &border_lp);
	}
}

//static void ytick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels)
void GnuPlot::YTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels)
{
	double scale = tic_scale(ticlevel, pAx) * (pAx->TicIn ? 1 : -1);
	double other_end = AxS.__X().min + AxS.__X().max - _3DBlk.YAxisX;
	GpVertex v1, v2, v3, v4;
	// Draw full-length grid line 
	Map3D_XYZ(_3DBlk.YAxisX, place, base_z, &v1);
	if(rGrid.l_type > LT_NODRAW) {
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(other_end, place, base_z, &v3);
		Draw3DLine(pTerm, &v1, &v3, const_cast<lp_style_type *>(&rGrid)); // @badcast
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	}
	// Vertical grid lines (in xz plane) 
	if(grid_vertical_lines && rGrid.l_type > LT_NODRAW) {
		GpVertex v4, v5;
		double which_face = (surface_rot_x > 90 && surface_rot_x < 270) ? _3DBlk.YAxisX : other_end;
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(which_face, place, AxS.__Z().min, &v4);
		Map3D_XYZ(which_face, place, ceiling_z, &v5);
		Draw3DLine(pTerm, &v4, &v5, const_cast<lp_style_type *>(&rGrid)); // @badcast
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	}
	if(AxS.__Y().ticmode & TICS_ON_AXIS && !AxS.__X().log && AxS.__X().InRange(0.0)) {
		Map3D_XYZ(0.0, place, base_z, &v1);
	}
	// NB: secondary axis must be linked to primary 
	if(pAx->index == SECOND_Y_AXIS && pAx->linked_to_primary && pAx->link_udf->at) {
		place = EvalLinkFunction(&AxS[FIRST_Y_AXIS], place);
	}
	// Draw left tic mark 
	if((pAx->index == FIRST_Y_AXIS) || (pAx->index == SECOND_Y_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		v2.x = v1.x + _3DBlk.TicUnit.x * scale * pTerm->TicH;
		v2.y = v1.y + _3DBlk.TicUnit.y * scale * pTerm->TicH;
		v2.z = v1.z + _3DBlk.TicUnit.z * scale * pTerm->TicH;
		v2.real_z = v1.real_z;
		Draw3DLine(pTerm, &v1, &v2, &border_lp);
	}
	// Draw right tic mark 
	if((pAx->index == SECOND_Y_AXIS) || (pAx->index == FIRST_Y_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		if(yz_projection)
			Map3D_XYZ(other_end, place, AxS.__Z().min, &v3);
		else
			Map3D_XYZ(other_end, place, base_z, &v3);
		v4.x = v3.x - _3DBlk.TicUnit.x * scale * pTerm->TicH;
		v4.y = v3.y - _3DBlk.TicUnit.y * scale * pTerm->TicH;
		v4.z = v3.z - _3DBlk.TicUnit.z * scale * pTerm->TicH;
		v4.real_z = v3.real_z;
		Draw3DLine(pTerm, &v3, &v4, &border_lp);
	}
	// Draw tic label 
	if(text) {
		int just;
		int x2, y2;
		int angle;
		int offsetx, offsety;
		// Skip label if we've already written a user-specified one here 
#define MINIMUM_SEPARATION 0.001
		while(userlabels) {
			if(fabs((place - userlabels->position) / (AxS.__Y().max - AxS.__Y().min))
			    <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "ytics");
		// allow manual justification of tick labels, but only for projections 
		if((splot_map || yz_projection) && pAx->manual_justify)
			just = pAx->tic_pos;
		else if(_3DBlk.TicUnit.x * xscaler < -0.9)
			just = (pAx->index == FIRST_Y_AXIS) ? LEFT : RIGHT;
		else if(_3DBlk.TicUnit.x * xscaler < 0.9)
			just = CENTRE;
		else
			just = (pAx->index == FIRST_Y_AXIS) ? RIGHT : LEFT;
		if(pAx->index == SECOND_Y_AXIS) {
			v4.x = v3.x + _3DBlk.TicUnit.x * pTerm->ChrH * 1;
			v4.y = v3.y + _3DBlk.TicUnit.y * pTerm->ChrV * 1;
			if(!pAx->TicIn) {
				v4.x += _3DBlk.TicUnit.x * pTerm->TicH * pAx->ticscale;
				v4.y += _3DBlk.TicUnit.y * pTerm->TicV * pAx->ticscale;
			}
			TERMCOORD(&v4, x2, y2);
		}
		else {
			v2.x = v1.x - _3DBlk.TicUnit.x * pTerm->ChrH * 1;
			v2.y = v1.y - _3DBlk.TicUnit.y * pTerm->ChrV * 1;
			if(!pAx->TicIn) {
				v2.x -= _3DBlk.TicUnit.x * pTerm->TicH * pAx->ticscale;
				v2.y -= _3DBlk.TicUnit.y * pTerm->TicV * pAx->ticscale;
			}
			TERMCOORD(&v2, x2, y2);
		}
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		angle = pAx->tic_rotate;
		if(!(splot_map && angle && pTerm->text_angle(angle)))
			angle = 0;
		ignore_enhanced(!pAx->ticdef.enhanced);
		write_multiline(pTerm, x2+offsetx, y2+offsety, text, (JUSTIFY)just, JUST_TOP, angle, pAx->ticdef.font);
		ignore_enhanced(FALSE);
		pTerm->text_angle(0);
		TermApplyLpProperties(pTerm, &border_lp);
	}
}

//static void ztick_callback(GpAxis * pAx, double place, char * text, int ticlevel, lp_style_type grid, ticmark * userlabels)
void GnuPlot::ZTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels)
{
	int len = static_cast<int>(tic_scale(ticlevel, pAx) * (pAx->TicIn ? 1 : -1) * (pTerm->TicH));
	GpVertex v1, v2, v3;
	if(pAx->ticmode & TICS_ON_AXIS)
		Map3D_XYZ(0., 0., place, &v1);
	else
		Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, place, &v1);
	// Needed both for grid and for azimuth ztics 
	Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, place, &v3);
	if(rGrid.l_type > LT_NODRAW) {
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(_3DBlk.Back.x, _3DBlk.Back.y, place, &v2);
		Draw3DLine(pTerm, &v1, &v2, const_cast<lp_style_type *>(&rGrid)); // @badcast
		Draw3DLine(pTerm, &v2, &v3, const_cast<lp_style_type *>(&rGrid)); // @badcast
		(pTerm->layer)(pTerm, TERM_LAYER_END_GRID);
	}
	if(azimuth != 0) {
		v2.x = v1.x + (v3.x - v1.x) * len / xyscaler;
		v2.y = v1.y + (v3.y - v1.y) * len / xyscaler;
		v2.z = v1.z + (v3.z - v1.z) * len / xyscaler;
	}
	else {
		v2.x = v1.x + len / (double)xscaler;
		v2.y = v1.y;
		v2.z = v1.z;
	}
	v2.real_z = v1.real_z;
	Draw3DLine(pTerm, &v1, &v2, &border_lp);
	if(text) {
		int x1, y1;
		int just;
		int offsetx, offsety;
		/* Skip label if we've already written a user-specified one here */
#define MINIMUM_SEPARATION 0.001
		while(userlabels) {
			if(fabs((place - userlabels->position) / (AxS.__Z().max - AxS.__Z().min))
			    <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "ztics");
		TERMCOORD(&v1, x1, y1);
		if(fabs(azimuth) > 80) {
			/* Z axis is (nearly) horizontal */
			y1 += sgn(azimuth) * (pTerm->TicV) * 2;
		}
		else {
			// the normal case 
			x1 -= (pTerm->TicH) * 2;
			if(!pAx->TicIn)
				x1 -= (pTerm->TicH) * pAx->ticscale;
		}
		// allow manual justification of tick labels, but only for projections 
		if((xz_projection || yz_projection) && pAx->manual_justify)
			just = pAx->tic_pos;
		else
			just = RIGHT;
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type == TC_Z)
			pAx->ticdef.textcolor.value = place;
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		ignore_enhanced(!pAx->ticdef.enhanced);
		write_multiline(pTerm, x1+offsetx, y1+offsety, text, (JUSTIFY)just, JUST_CENTRE, 0, pAx->ticdef.font);
		ignore_enhanced(FALSE);
		TermApplyLpProperties(pTerm, &border_lp);
	}
	if(AxS.__Z().ticmode & TICS_MIRROR) {
		if(azimuth != 0) {
			v2.x = v3.x + (v1.x - v3.x) * len / xyscaler;
			v2.y = v3.y + (v1.y - v3.y) * len / xyscaler;
			v2.z = v3.z + (v1.z - v3.z) * len / xyscaler;
			Draw3DLine(pTerm, &v3, &v2, &border_lp);
		}
		else {
			Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, place, &v1);
			v2.x = v1.x - len / (double)xscaler;
			v2.y = v1.y;
			v2.z = v1.z;
			v2.real_z = v1.real_z;
			Draw3DLine(pTerm, &v1, &v2, &border_lp);
		}
	}
}

//static int map3d_getposition(GpPosition * pos, const char * what, double * xpos, double * ypos, double * zpos)
int GnuPlot::Map3DGetPosition(const GpTermEntry * pTerm, GpPosition * pPos, const char * what, double * xpos, double * ypos, double * zpos)
{
	bool screen_coords = FALSE;
	bool char_coords = FALSE;
	bool plot_coords = FALSE;
	double xx, yy;
#define flat ((pPos->scalex == graph) && (pPos->scaley == graph) && (pPos->z == 0))
	switch(pPos->scalex) {
		case first_axes:
		case second_axes:
		    *xpos = AxisLogValueChecked(FIRST_X_AXIS, *xpos, what);
		    plot_coords = TRUE;
		    break;
		case graph:
		    if(xz_projection && flat && !in_3d_polygon)
			    *zpos = AxS.__Z().min + *xpos * (AxS.__Z().max - AxS.__Z().min);
		    else if(yz_projection && flat && !in_3d_polygon)
			    // Why is the direction inverted? 
			    *zpos = AxS.__Z().max + *xpos * (AxS.__Z().min - AxS.__Z().max);
		    else
			    *xpos = AxS.__X().min + *xpos * (AxS.__X().max - AxS.__X().min);
		    plot_coords = TRUE;
		    break;
		case screen:
		    *xpos = *xpos * (pTerm->MaxX -1) + 0.5;
		    screen_coords = TRUE;
		    break;
		case character:
		    *xpos = *xpos * pTerm->ChrH + 0.5;
		    char_coords = TRUE;
		    break;
		case polar_axes:
		    PolarToXY(*xpos, *ypos, &xx, &yy, FALSE);
		    *xpos = AxisLogValueChecked(FIRST_X_AXIS, xx, what);
		    *ypos = AxisLogValueChecked(FIRST_Y_AXIS, yy, what);
		    plot_coords = TRUE;
		    pPos->scaley = polar_axes; // Just to make sure 
		    break;
	}
	switch(pPos->scaley) {
		case first_axes:
		case second_axes:
		    *ypos = AxisLogValueChecked(FIRST_Y_AXIS, *ypos, what);
		    plot_coords = TRUE;
		    break;
		case graph:
		    if(xz_projection && flat && !in_3d_polygon)
			    *xpos = AxS.__X().min + *ypos * (AxS.__X().max - AxS.__X().min);
		    else if(splot_map)
			    *ypos = AxS.__Y().max - *ypos * (AxS.__Y().max - AxS.__Y().min);
		    else
			    *ypos = AxS.__Y().min + *ypos * (AxS.__Y().max - AxS.__Y().min);
		    plot_coords = TRUE;
		    break;
		case screen:
		    *ypos = *ypos * (pTerm->MaxY -1) + 0.5;
		    screen_coords = TRUE;
		    break;
		case character:
		    *ypos = *ypos * pTerm->ChrV + 0.5;
		    char_coords = TRUE;
		    break;
		case polar_axes:
		    break;
	}
	switch(pPos->scalez) {
		case first_axes:
		case second_axes:
		case polar_axes:
		    if(splot_map)
			    *zpos = 1.0; // Avoid failure if z=0 with logscale z 
		    else
			    *zpos = AxisLogValueChecked(FIRST_Z_AXIS, *zpos, what);
		    plot_coords = TRUE;
		    break;
		case graph:
		    if((xz_projection || yz_projection) && flat && !in_3d_polygon)
			    ; /* already received "x" fraction */
		    else
			    *zpos = AxS.__Z().min + *zpos * (AxS.__Z().max - AxS.__Z().min);
		    plot_coords = TRUE;
		    break;
		case screen:
		    screen_coords = TRUE;
		    break;
		case character:
		    char_coords = TRUE;
		    break;
	}
	if(plot_coords && (screen_coords || char_coords))
		IntError(NO_CARET, "Cannot mix screen or character coords with plot coords");
	return (screen_coords || char_coords);
#undef flat
}
/*
 * map3d_position()  wrapper for map3d_position_double
 */
//void map3d_position(GpPosition * pos, int * x, int * y, const char * what)
void GnuPlot::Map3DPosition(const GpTermEntry * pTerm, GpPosition * pos, int * x, int * y, const char * what)
{
	double xx, yy;
	Map3DPositionDouble(pTerm, pos, &xx, &yy, what);
	*x = static_cast<int>(xx);
	*y = static_cast<int>(yy);
}

//void map3d_position_double(struct GpPosition * pos, double * x, double * y, const char * what)
void GnuPlot::Map3DPositionDouble(const GpTermEntry * pTerm, GpPosition * pos, double * x, double * y, const char * what)
{
	double xpos = pos->x;
	double ypos = pos->y;
	double zpos = pos->z;
	if(Map3DGetPosition(pTerm, pos, what, &xpos, &ypos, &zpos) == 0) {
		Map3D_XY_double(xpos, ypos, zpos, x, y);
	}
	else {
		// Screen or character coordinates 
		*x = xpos;
		*y = ypos;
	}
}

//void map3d_position_r(GpPosition * pPos, int * x, int * y, const char * what)
void GnuPlot::Map3DPositionR(const GpTermEntry * pTerm, GpPosition * pPos, int * x, int * y, const char * what)
{
	double xx, yy;
	Map3DPositionRDouble(pTerm, pPos, &xx, &yy, what);
	*x = static_cast<int>(xx);
	*y = static_cast<int>(yy);
}

//void map3d_position_r_double(GpPosition * pos, double * xx, double * yy, const char * what)
void GnuPlot::Map3DPositionRDouble(const GpTermEntry * pTerm, GpPosition * pPos, double * xx, double * yy, const char * what)
{
	double xpos = pPos->x;
	double ypos = pPos->y;
	double zpos = splot_map ? AxS.__Z().min : pPos->z;
	// startpoint in graph coordinates 
	if(Map3DGetPosition(pTerm, pPos, what, &xpos, &ypos, &zpos) == 0) {
		int xoriginlocal;
		int yoriginlocal;
		Map3D_XY_double(xpos, ypos, zpos, xx, yy);
		if(pPos->scalex == graph)
			xpos = AxS.__X().min;
		else
			xpos = 0;
		if(pPos->scaley == graph)
			ypos = splot_map ? AxS.__Y().max : AxS.__Y().min;
		else
			ypos = 0;
		if(pPos->scalez == graph)
			zpos = AxS.__Z().min;
		else if(splot_map)
			zpos = AxS.__Z().min;
		else
			zpos = 0;
		Map3D_XY(xpos, ypos, zpos, &xoriginlocal, &yoriginlocal);
		*xx -= xoriginlocal;
		*yy -= yoriginlocal;
	}
	else {
		// endpoint `screen' or 'character' coordinates 
		*xx = xpos;
		*yy = ypos;
	}
}
// 
// these code blocks were moved to functions, to make the code simpler
// 
static void key_text(GpTermEntry * pTerm, int xl, int yl, char * pText)
{
	legend_key * key = &keyT;
	(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_KEYSAMPLE);
	if(key->just == GPKEY_LEFT) {
		write_multiline(pTerm, xl + _3DBlk.KeyTextLeft, yl, pText, LEFT, JUST_TOP, 0, key->font);
	}
	else {
		if((pTerm->justify_text)(RIGHT)) {
			write_multiline(pTerm, xl + _3DBlk.KeyTextRight, yl, pText, RIGHT, JUST_TOP, 0, key->font);
		}
		else {
			int x = xl + _3DBlk.KeyTextRight - (pTerm->ChrH) * estimate_strlen(pText, NULL);
			write_multiline(pTerm, x, yl, pText, LEFT, JUST_TOP, 0, key->font);
		}
	}
	(pTerm->layer)(pTerm, TERM_LAYER_END_KEYSAMPLE);
}

//static void key_sample_line(GpTermEntry * pTerm, int xl, int yl)
void GnuPlot::KeySampleLine(GpTermEntry * pTerm, int xl, int yl)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Clip against canvas 
	V.P_ClipArea = (pTerm->flags & TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_KEYSAMPLE);
	DrawClipLine(pTerm, xl + _3DBlk.KeySampleLeft, yl, xl + _3DBlk.KeySampleRight, yl);
	(pTerm->layer)(pTerm, TERM_LAYER_END_KEYSAMPLE);
	V.P_ClipArea = clip_save;
}

//static void key_sample_point(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype)
void GnuPlot::KeySamplePoint(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Clip against canvas 
	V.P_ClipArea = (pTerm->flags & TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_KEYSAMPLE);
	if(!V.ClipPoint(xl + _3DBlk.KeyPointOffset, yl)) {
		if(pointtype == PT_CHARACTER && pPlot) {
			ApplyPm3DColor(pTerm, &(pPlot->labels->textcolor));
			(pTerm->put_text)(pTerm, xl + _3DBlk.KeyPointOffset, yl, pPlot->lp_properties.p_char);
			ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
		}
		else {
			(pTerm->point)(pTerm, xl + _3DBlk.KeyPointOffset, yl, pointtype);
		}
	}
	(pTerm->layer)(pTerm, TERM_LAYER_END_KEYSAMPLE);
	V.P_ClipArea = clip_save;
}

//static void key_sample_fill(GpTermEntry * pTerm, int xl, int yl, GpSurfacePoints * pPlot)
void GnuPlot::KeySampleFill(GpTermEntry * pTerm, int xl, int yl, GpSurfacePoints * pPlot)
{
	const fill_style_type * fs = &pPlot->fill_properties;
	int style = style_from_fill(fs);
	int x = xl + _3DBlk.KeySampleLeft;
	int y = yl - _3DBlk.KeyEntryHeight/4;
	int w = _3DBlk.KeySampleRight - _3DBlk.KeySampleLeft;
	int h = _3DBlk.KeyEntryHeight/2;
	if(pTerm->fillbox) {
		(pTerm->layer)(pTerm, TERM_LAYER_BEGIN_KEYSAMPLE);
		if(pPlot->plot_style == CIRCLES) {
			DoArc(pTerm, x+w/2, yl, _3DBlk.KeyEntryHeight/4, 0.0, 360.0, style, FALSE);
			// Retrace the border if the style requests it 
			if(NeedFillBorder(pTerm, fs))
				DoArc(pTerm, x+w/2, yl, _3DBlk.KeyEntryHeight/4, 0.0, 360.0, 0, FALSE);
		}
		else if(w > 0) {
			(pTerm->fillbox)(pTerm, style, x, y, w, h);
			// FIXME:  what other plot styles want a border on the key sample? 
			if((pPlot->plot_style & (PLOT_STYLE_HAS_PM3DBORDER | PLOT_STYLE_HAS_FILL))) {
				if((pPlot->plot_style & PLOT_STYLE_HAS_PM3DBORDER))
					if(pm3d.border.l_type != LT_NODRAW && pm3d.border.l_type != LT_DEFAULT)
						TermApplyLpProperties(pTerm, &pm3d.border);
	#ifdef BOXERROR_3D
				if(pPlot->plot_style == BOXERROR)
					NeedFillBorder(pTerm, &pPlot->fill_properties);
	#endif
				newpath(pTerm);
				DrawClipLine(pTerm, x, y, x+w, y);
				DrawClipLine(pTerm, x+w, y, x+w, y+h);
				DrawClipLine(pTerm, x+w, y+h, x, y+h);
				DrawClipLine(pTerm, x, y+h, x, y);
				closepath(pTerm);
			}
		}
		(pTerm->layer)(pTerm, TERM_LAYER_END_KEYSAMPLE);
	}
}
//
// returns minimal and maximal values of the cb-range (or z-range if taking the
// color from the z value) of the given surface
//
static void get_surface_cbminmax(const GpSurfacePoints * pPlot, double * cbmin, double * cbmax)
{
	int i, curve = 0;
	const bool color_from_column = pPlot->pm3d_color_from_column; /* just a shortcut */
	coordval cb;
	const iso_curve * icrvs = pPlot->iso_crvs;
	GpCoordinate * points;
	*cbmin = VERYLARGE;
	*cbmax = -VERYLARGE;
	while(icrvs && curve < pPlot->num_iso_read) {
		// fprintf(stderr,"**** NEW ISOCURVE - nb of pts: %i ****\n", icrvs->p_count); 
		for(i = 0, points = icrvs->points; i < icrvs->p_count; i++) {
			// fprintf(stderr,"  point i=%i => x=%4g y=%4g z=%4lg cb=%4lg\n",i, points[i].x,points[i].y,points[i].z,points[i].CRD_COLOR); 
			if(points[i].type == INRANGE) {
				// ?? if (!clip_point(x, y)) ... 
				cb = color_from_column ? points[i].CRD_COLOR : points[i].z;
				if(cb < *cbmin) *cbmin = cb;
				if(cb > *cbmax) *cbmax = cb;
			}
		} /* points on one scan */
		icrvs = icrvs->next;
		curve++;
	} /* surface */
}
//
// Draw a gradient color line for a key (legend).
//
//static void key_sample_line_pm3d(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl)
void GnuPlot::KeySampleLinePm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl)
{
	int steps = MIN(24, abs(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft));
	// don't multiply by key->swidth --- could be >> palette.maxcolors 
	int x_to = xl + _3DBlk.KeySampleRight;
	double step = ((double)(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft)) / steps;
	int i = 1, x1 = xl + _3DBlk.KeySampleLeft, x2;
	double cbmin, cbmax;
	double gray, gray_from, gray_to, gray_step;
	int colortype = pPlot->lp_properties.pm3d_color.type;
	// If pPlot uses a constant color, set it here and then let simpler routine take over 
	if((colortype == TC_RGB && pPlot->lp_properties.pm3d_color.value >= 0.0) || (colortype == TC_LT) || (colortype == TC_LINESTYLE)) {
		lp_style_type lptmp = pPlot->lp_properties;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			lp_use_properties(pTerm, &lptmp, (int)(pPlot->iso_crvs->points[0].CRD_COLOR));
		ApplyPm3DColor(pTerm, &lptmp.pm3d_color);
		KeySampleLine(pTerm, xl, yl);
	}
	else {
		// color gradient only over the cb-values of the surface, if smaller than the cb-axis range (the latter are gray values [0:1]) 
		get_surface_cbminmax(pPlot, &cbmin, &cbmax);
		if(cbmin <= cbmax) { // splot 1/0, for example 
			cbmin = MAX(cbmin, AxS.__CB().min);
			cbmax = MIN(cbmax, AxS.__CB().max);
			gray_from = Cb2Gray(cbmin);
			gray_to = Cb2Gray(cbmax);
			gray_step = (gray_to - gray_from)/steps;
			clip_move(x1, yl);
			x2 = x1;
			while(i <= steps) {
				// if (i>1) set_color( i==steps ? 1 : (i-0.5)/steps ); ... range [0:1] 
				gray = (i==steps) ? gray_to : gray_from+i*gray_step;
				set_color(pTerm, gray);
				clip_move(x2, yl);
				x2 = (i==steps) ? x_to : x1 + (int)(i*step+0.5);
				ClipVector(pTerm, x2, yl);
				i++;
			}
		}
	}
}
//
// Draw a sequence of points with gradient color a key (legend).
//
//static void key_sample_point_pm3d(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype)
void GnuPlot::KeySamplePointPm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype)
{
	BoundingBox * clip_save = V.P_ClipArea;
	int x_to = xl + _3DBlk.KeySampleRight;
	int i = 0, x1 = xl + _3DBlk.KeySampleLeft, x2;
	double cbmin, cbmax;
	double gray, gray_from, gray_to, gray_step;
	int colortype = pPlot->lp_properties.pm3d_color.type;
	// rule for number of steps: 3*char_width*pointsize or char_width for dots, but at least 3 points 
	double step = pTerm->ChrH * (pointtype == -1 ? 1 : 3*(1+(Gg.PointSize-1)/2));
	int steps = (int)(((double)(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft)) / step + 0.5);
	SETMAX(steps, 2);
	step = ((double)(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft)) / steps;
	/* If pPlot uses a constant color, set it here and then let simpler routine take over */
	if((colortype == TC_RGB && pPlot->lp_properties.pm3d_color.value >= 0.0) || (colortype == TC_LT) || (colortype == TC_LINESTYLE)) {
		lp_style_type lptmp = pPlot->lp_properties;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			lp_use_properties(pTerm, &lptmp, (int)(pPlot->iso_crvs->points[0].CRD_COLOR));
		ApplyPm3DColor(pTerm, &lptmp.pm3d_color);
		KeySamplePoint(pTerm, pPlot, xl, yl, pointtype);
		return;
	}
	// color gradient only over the cb-values of the surface, if smaller than the
	// cb-axis range (the latter are gray values [0:1]) 
	get_surface_cbminmax(pPlot, &cbmin, &cbmax);
	if(cbmin > cbmax) 
		return; /* splot 1/0, for example */
	cbmin = MAX(cbmin, AxS.__CB().min);
	cbmax = MIN(cbmax, AxS.__CB().max);
	gray_from = Cb2Gray(cbmin);
	gray_to = Cb2Gray(cbmax);
	gray_step = (gray_to - gray_from)/steps;
	// Clip to canvas 
	V.P_ClipArea = (pTerm->flags & TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	while(i <= steps) {
		// if (i>0) set_color( i==steps ? gray_to : (i-0.5)/steps ); ... range [0:1] 
		gray = (i==steps) ? gray_to : gray_from+i*gray_step;
		set_color(pTerm, gray);
		x2 = i==0 ? x1 : (i==steps ? x_to : x1 + (int)(i*step+0.5));
		// x2 += _3DBlk.KeyPointOffset; ... that's if there is only 1 point 
		if(!V.ClipPoint(x2, yl))
			(pTerm->point)(pTerm, x2, yl, pointtype);
		i++;
	}
	V.P_ClipArea = clip_save;
}
// 
// plot_vectors:
// Plot the curves in VECTORS style
// 
//static void plot3d_vectors(GpTermEntry * pTerm, GpSurfacePoints * plot)
void GnuPlot::Plot3DVectors(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	double x1, y1, x2, y2;
	GpCoordinate * tails = pPlot->iso_crvs->points;
	GpCoordinate * heads = pPlot->iso_crvs->next->points;
	// Only necessary once, unless variable arrow style 
	arrow_style_type ap = pPlot->arrow_properties;
	TermApplyLpProperties(pTerm, &ap.lp_properties);
	apply_head_properties(&ap);
	for(int i = 0; i < pPlot->iso_crvs->p_count; i++) {
		if(heads[i].type == UNDEFINED || tails[i].type == UNDEFINED)
			continue;
		// variable arrow style read from extra data column 
		if(pPlot->arrow_properties.tag == AS_VARIABLE) {
			int as = static_cast<int>(heads[i].CRD_COLOR);
			arrow_use_properties(&ap, as);
			TermApplyLpProperties(pTerm, &ap.lp_properties);
			apply_head_properties(&ap);
		}
		else {
			Check3DForVariableColor(pTerm, pPlot, &heads[i]);
		}
		// The normal case: both ends in range 
		if(heads[i].type == INRANGE && tails[i].type == INRANGE) {
			Map3D_XY_double(tails[i].x, tails[i].y, tails[i].z, &x1, &y1);
			Map3D_XY_double(heads[i].x, heads[i].y, heads[i].z, &x2, &y2);
			DrawClipArrow(pTerm, x1, y1, x2, y2, ap.head);
			// "set clip two" - both ends out of range 
		}
		else if(heads[i].type != INRANGE && tails[i].type != INRANGE) {
			double lx[2], ly[2], lz[2];
			if(!Gg.ClipLines2)
				continue;
			TwoEdge3DIntersect(&tails[i], &heads[i], lx, ly, lz);
			Map3D_XY_double(lx[0], ly[0], lz[0], &x1, &y1);
			Map3D_XY_double(lx[1], ly[1], lz[1], &x2, &y2);
			DrawClipArrow(pTerm, x1, y1, x2, y2, ap.head);
			// "set clip one" - one end out of range 
		}
		else if(Gg.ClipLines1) {
			double clip_x, clip_y, clip_z;
			Edge3DIntersect(&heads[i], &tails[i], &clip_x, &clip_y, &clip_z);
			if(tails[i].type == INRANGE) {
				Map3D_XY_double(tails[i].x, tails[i].y, tails[i].z, &x1, &y1);
				Map3D_XY_double(clip_x, clip_y, clip_z, &x2, &y2);
			}
			else {
				Map3D_XY_double(clip_x, clip_y, clip_z, &x1, &y1);
				Map3D_XY_double(heads[i].x, heads[i].y, heads[i].z, &x2, &y2);
			}
			DrawClipArrow(pTerm, x1, y1, x2, y2, ap.head);
		}
	}
}
// 
// splot with zerrorfill
// This 3D style is similar to a 2D filledcurves plot between two lines.
// Put together a list of the component quadrangles using the data structures
// normally used by pm3d routines pm3d_plot(), pm3d_depth_queue_flush().
// The component quadrangles from all plots are sorted and flushed together.
// 
//static void plot3d_zerrorfill(GpSurfacePoints * plot)
void GnuPlot::Plot3DZErrorFill(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	iso_curve * curve = pPlot->iso_crvs;
	int i1, i2;     /* index leading and trailing coord of current quadrangle */
	int count = 0;
	gpdPoint corner[4];
	// Find leading edge of first quadrangle 
	for(i1 = 0; i1 < curve->p_count; i1++) {
		if(curve->points[i1].type == INRANGE)
			break;
	}
	for(i2 = i1+1; i2 < curve->p_count; i2++) {
		if(curve->points[i2].type != INRANGE)
			continue;
		count++; /* Found one */
		corner[0].x = corner[1].x = curve->points[i1].x;
		corner[0].y = corner[1].y = curve->points[i1].y;
		corner[0].z = curve->points[i1].CRD_ZLOW;
		corner[1].z = curve->points[i1].CRD_ZHIGH;
		corner[2].x = corner[3].x = curve->points[i2].x;
		corner[2].y = corner[3].y = curve->points[i2].y;
		corner[3].z = curve->points[i2].CRD_ZLOW;
		corner[2].z = curve->points[i2].CRD_ZHIGH;
		Pm3DAddQuadrangle(pTerm, pPlot, corner);
		i1 = i2;
	}
	if(count == 0)
		IntError(NO_CARET, "all points out of range");
	// Default is to write out each zerror plot as we come to it     
	// (most recent plot occludes all previous plots). To get proper 
	// sorting, use "set pm3d depthorder".                           
	if(pm3d.direction != PM3D_DEPTH)
		Pm3DDepthQueueFlush(pTerm);
}
// 
// 3D version of plot with boxes.
// By default only a flat rectangle is drawn.  "set boxdepth <depth>"
// changes this to draw real boxes (4 sides + top).
// The boxes are drawn as pm3d rectangles. This means that depth-cueing
// must be done with "set pm3d depth base" rather than with "set hidden3d".
// 
//static void plot3d_boxes(GpSurfacePoints * plot)
void GnuPlot::Plot3DBoxes(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int i;                  /* point index */
	double dxl, dxh;        /* rectangle extent along X axis */
	double dyl, dyh;        /* rectangle extent along Y axis */
	double zbase, dz;       /* box base and height */
	fill_style_type save_fillstyle;
	iso_curve * icrvs = pPlot->iso_crvs;
	gpdPoint corner[4];
	// This initialization is normally done via pm3d_plot()
	// but 3D boxes are drawn in a parallel code path.
	if(pm3d_shade.strength > 0)
		pm3d_init_lighting_model();
	// FIXME: fillstyle and border color always come from "set style fill" 
	pm3d.border = pPlot->lp_properties;
	pm3d.border.pm3d_color = default_fillstyle.border_color;
	while(icrvs) {
		GpCoordinate * points = icrvs->points;
		for(i = 0; i < icrvs->p_count; i++) {
			if(points[i].type == UNDEFINED)
				continue;
			dxh = points[i].xhigh;
			dxl = points[i].xlow;
			dyl = points[i].y;
			dyh = points[i].y;
			dz = points[i].z;
			// Box is out of range on y 
			if((dyl > AxS.__Y().min && dyl > AxS.__Y().max) || (dyl < AxS.__Y().min && dyl < AxS.__Y().max))
				continue;
			if(boxdepth != 0) {
				double depth = boxdepth;
				if(AxS.__Y().log) {
					if(boxdepth < 0)
						depth = V.BoxWidth * yscaler/xscaler;
					dyl *= pow(AxS.__Y().base, -depth/2.);
					dyh *= pow(AxS.__Y().base, depth/2.);
				}
				else {
					if(boxdepth < 0)
						depth = V.BoxWidth * (AxS.__Y().max-AxS.__Y().min)/(AxS.__X().max-AxS.__X().min);
					dyl -= depth / 2.;
					dyh += depth / 2.;
				}
				cliptorange(dyl, AxS.__Y().min, AxS.__Y().max);
				cliptorange(dyh, AxS.__Y().min, AxS.__Y().max);
			}
			// clip to border 
			cliptorange(dxl, AxS.__X().min, AxS.__X().max);
			cliptorange(dxh, AxS.__X().min, AxS.__X().max);
			// Entire box is out of range on x 
			if(dxl == dxh && (dxl == AxS.__X().min || dxl == AxS.__X().max))
				continue;
			zbase = 0;
			cliptorange(zbase, AxS.__Z().min, AxS.__Z().max);
			// Copy variable color value into pPlot header for pm3d_add_quadrangle 
			if(pPlot->pm3d_color_from_column)
				pPlot->lp_properties.pm3d_color.lt = static_cast<int>(points[i].CRD_COLOR);
			// Construct and store single pm3d rectangle (front of box) 
			// Z	corner1	corner2	
			// 0	corner0 corner3 
			corner[0].x = corner[1].x = dxl;
			corner[2].x = corner[3].x = dxh;
			corner[0].y = corner[1].y = corner[2].y = corner[3].y = dyl;
			corner[0].z = corner[3].z = zbase;
			corner[1].z = corner[2].z = dz;
			Pm3DAddQuadrangle(pTerm, pPlot, corner);
			// The normal case is to draw the front only (boxdepth = 0) 
			if(boxdepth == 0)
				continue;
			// Back side of the box 
			corner[0].y = corner[1].y = corner[2].y = corner[3].y = dyh;
			Pm3DAddQuadrangle(pTerm, pPlot, corner);
			// Left side of box 
			corner[2].x = corner[3].x = dxl;
			corner[0].y = corner[1].y = dyl;
			corner[0].z = corner[3].z = zbase;
			corner[1].z = corner[2].z = dz;
			Pm3DAddQuadrangle(pTerm, pPlot, corner);
			// Right side of box 
			corner[0].x = corner[1].x = corner[2].x = corner[3].x = dxh;
			Pm3DAddQuadrangle(pTerm, pPlot, corner);
			// Top of box 
			corner[0].x = corner[1].x = dxl;
			corner[0].y = corner[3].y = dyl;
			corner[1].y = corner[2].y = dyh;
			corner[0].z = corner[3].z = dz;
			Pm3DAddQuadrangle(pTerm, pPlot, corner);
		} /* loop over points */
		icrvs = icrvs->next;
	}
	// FIXME The only way to get the pm3d flush code to see our fill 
	// style is to temporarily copy it to the global fillstyle.      
	save_fillstyle = default_fillstyle;
	default_fillstyle = pPlot->fill_properties;
	// By default we write out each set of boxes as it is seen.  
	// The other option is to let them accummulate and then sort 
	// them together with all other pm3d elements to draw later. 
	if(pm3d.direction != PM3D_DEPTH) {
		pm3d.base_sort = TRUE;
		Pm3DDepthQueueFlush(pTerm);
		pm3d.base_sort = FALSE;
	}
	default_fillstyle = save_fillstyle; // Restore global fillstyle 
}
/*
 * Plot the data as a set of polygons.
 * Successive lines of input data provide vertex coordinates.
 * A blank line separates polygons.
 * E.g. two triangles:
 *	x1 y1 z1
 *	x2 y2 z2
 *	x3 y3 z3
 *
 *	x1 y1 z1
 *	x2 y2 z2
 *	x3 y3 z3
 */
//static void plot3d_polygons(GpSurfacePoints * plot)
void GnuPlot::Plot3DPolygons(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int nv;
	iso_curve * icrvs;
	GpCoordinate * points;
	int style;
	static gpdPoint * quad = NULL;
	static int quadmax = 0;
	// These don't need to be drawn at all 
	if(pPlot->lp_properties.l_type == LT_NODRAW)
		return;
	// This initialization is normally done in pm3d_plot()
	// but polygons do not necessarily use that code path.
	if(pm3d_shade.strength > 0)
		pm3d_init_lighting_model();
	style = style_from_fill(&pPlot->fill_properties);
	// Most polygons are small 
	quadmax = 8;
	quad = (gpdPoint *)gp_realloc(quad, quadmax * sizeof(gpdPoint), NULL);
	for(icrvs = pPlot->iso_crvs; icrvs; icrvs = icrvs->next) {
		// Allow for very large polygons (e.g. cartographic outlines) 
		const int npoints = icrvs->p_count;
		if(npoints > quadmax) {
			quadmax = npoints;
			quad = (gpdPoint *)gp_realloc(quad, quadmax * sizeof(gpdPoint), NULL);
		}
		// Copy the vertex coordinates into a pm3d quadrangle 
		for(nv = 0, points = icrvs->points; nv < npoints; nv++) {
			quad[nv].x = points[nv].x;
			quad[nv].y = points[nv].y;
			quad[nv].z = points[nv].z;
		}
		// Treat triangle as a degenerate quadrangle 
		if(nv == 3) {
			quad[3].x = points[0].x;
			quad[3].y = points[0].y;
			quad[3].z = points[0].z;
		}
		// Ignore lines and points 
		if(nv < 3)
			continue;
		// Coloring piggybacks on options for isosurface 
		if(pPlot->pm3d_color_from_column && !isnan(points[0].CRD_COLOR))
			quad[0].c = points[0].CRD_COLOR;
		else
			quad[0].c = pPlot->fill_properties.border_color.lt;
		quad[1].c = style;
		Pm3DAddPolygon(pTerm, pPlot, quad, nv);
	}
	// Default is to write out each polygon as we come to it. 
	// To get proper sorting, use "set pm3d depthorder".      
	if(pm3d.direction != PM3D_DEPTH)
		Pm3DDepthQueueFlush(pTerm);
	// Clean up 
	ZFREE(quad);
	quadmax = 0;
}

//static void check3d_for_variable_color(GpTermEntry * pTerm, GpSurfacePoints * pPlot, GpCoordinate * pPoint)
void GnuPlot::Check3DForVariableColor(GpTermEntry * pTerm, GpSurfacePoints * pPlot, GpCoordinate * pPoint)
{
	int colortype = pPlot->lp_properties.pm3d_color.type;
	switch(colortype) {
		case TC_RGB:
		    if(pPlot->pm3d_color_from_column && pPlot->lp_properties.pm3d_color.value < 0.0)
			    SetRgbColorVar(pTerm, (uint)pPoint->CRD_COLOR);
		    break;
		case TC_Z:
		case TC_DEFAULT: /* pm3d mode assumes this is default */
		    if(pPlot->pm3d_color_from_column)
			    set_color(pTerm, Cb2Gray(pPoint->CRD_COLOR));
		    else
			    set_color(pTerm, Cb2Gray(pPoint->z));
		    break;
		case TC_LINESTYLE: /* color from linestyle in data column */
		    pPlot->lp_properties.pm3d_color.lt = (int)(pPoint->CRD_COLOR);
		    ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
		    break;
		case TC_COLORMAP:
		    if(pPlot->lp_properties.P_Colormap) {
			    double gray = map2gray(pPoint->CRD_COLOR, pPlot->lp_properties.P_Colormap);
			    SetRgbColorVar(pTerm, rgb_from_colormap(gray, pPlot->lp_properties.P_Colormap) );
		    }
		    break;
		default:
		    // The other cases were taken care of already 
		    break;
	}
}

void do_3dkey_layout(GpTermEntry * pTerm, legend_key * key, int * xinkey, int * yinkey)
{
	int key_height, key_width;
	// NOTE: All of these had better not change after being calculated here! 
	if(key->reverse) {
		_3DBlk.KeySampleLeft = -_3DBlk.KeySampleWidth;
		_3DBlk.KeySampleRight = 0;
		_3DBlk.KeyTextLeft  = pTerm->ChrH;
		_3DBlk.KeyTextRight = pTerm->ChrH * (_3DBlk.MaxPTitlLen + 1);
		_3DBlk.KeySizeRight = static_cast<int>(pTerm->ChrH * (_3DBlk.MaxPTitlLen + 2 + key->width_fix));
		_3DBlk.KeySizeLeft  = pTerm->ChrH + _3DBlk.KeySampleWidth;
	}
	else {
		_3DBlk.KeySampleLeft = 0;
		_3DBlk.KeySampleRight = _3DBlk.KeySampleWidth;
		_3DBlk.KeyTextLeft = -(int)(pTerm->ChrH * (_3DBlk.MaxPTitlLen + 1));
		_3DBlk.KeyTextRight = -(int)pTerm->ChrH;
		_3DBlk.KeySizeLeft  = static_cast<int>(pTerm->ChrH * (_3DBlk.MaxPTitlLen + 2 + key->width_fix));
		_3DBlk.KeySizeRight = pTerm->ChrH + _3DBlk.KeySampleWidth;
	}
	_3DBlk.KeyPointOffset = (_3DBlk.KeySampleLeft + _3DBlk.KeySampleRight) / 2;
	// Key title width and height, adjusted for font size and markup 
	_3DBlk.KeyTitleExtra = 0;
	_3DBlk.KeyTitleHeight = 0;
	if(key->title.text) {
		double est_height;
		if(key->title.font)
			pTerm->set_font(pTerm, key->title.font);
		estimate_strlen(key->title.text, &est_height);
		_3DBlk.KeyTitleHeight = static_cast<int>(est_height * pTerm->ChrV);
		if(key->title.font)
			pTerm->set_font(pTerm, "");
		// Allow a little extra clearance for markup 
		if((pTerm->flags & TERM_ENHANCED_TEXT) && (strchr(key->title.text, '^') || strchr(key->title.text, '_')))
			_3DBlk.KeyTitleExtra = pTerm->ChrV/2;
	}
	key_width = _3DBlk.KeyColWth * (_3DBlk.KeyCols - 1) + _3DBlk.KeySizeRight + _3DBlk.KeySizeLeft;
	key_height = static_cast<int>(_3DBlk.KeyTitleHeight + _3DBlk.KeyTitleExtra + _3DBlk.KeyEntryHeight * _3DBlk.KeyRows + key->height_fix * pTerm->ChrV);
	// Make room for extra long title 
	SETMAX(key_width, _3DBlk.KeyTitleWidth);
	// Now that we know the size of the key, we can position it as requested 
	if(key->region == GPKEY_USER_PLACEMENT) {
		int corner_x, corner_y;
		GPO.Map3DPosition(pTerm, &key->user_pos, &corner_x, &corner_y, "key");
		if(key->hpos == CENTRE)
			key->bounds.xleft = corner_x - key_width / 2;
		else if(key->hpos == RIGHT)
			key->bounds.xleft = corner_x - key_width;
		else
			key->bounds.xleft = corner_x;
		key->bounds.xright = key->bounds.xleft + key_width;
		key->bounds.ytop = corner_y;
		key->bounds.ybot = corner_y - key_height;
		*xinkey = key->bounds.xleft + _3DBlk.KeySizeLeft;
		*yinkey = key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
	}
	else {
		const BoundingBox * p_bounds = (key->fixed && !splot_map) ? &GPO.V.BbPage : &GPO.V.BbPlot;
		if(key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_BMARGIN) {
			if(_3DBlk.PTitlCnt > 0) {
				// we divide into columns, then centre in column by considering
				// ratio of key_left_size to key_right_size
				// key_size_left / (key_size_left+_3DBlk.KeySizeRight) * (bounds->xright-bounds->xleft)/_3DBlk.KeyCols
				// do one integer division to maximise accuracy (hope we dont overflow!)
				*xinkey = p_bounds->xleft + ((p_bounds->xright - p_bounds->xleft) * _3DBlk.KeySizeLeft) / (_3DBlk.KeyCols * (_3DBlk.KeySizeLeft + _3DBlk.KeySizeRight));
				key->bounds.xleft = *xinkey - _3DBlk.KeySizeLeft;
				key->bounds.xright = key->bounds.xleft + key_width;
				key->bounds.ytop = p_bounds->ybot;
				key->bounds.ybot = p_bounds->ybot - key_height;
				*yinkey = key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
			}
		}
		else {
			if(key->vpos == JUST_TOP) {
				key->bounds.ytop = p_bounds->ytop - pTerm->TicV;
				key->bounds.ybot = key->bounds.ytop - key_height;
				*yinkey = key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
			}
			else {
				key->bounds.ybot = p_bounds->ybot + pTerm->TicV;
				key->bounds.ytop = key->bounds.ybot + key_height;
				*yinkey = key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
			}
			if(key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_RMARGIN) {
				// keys outside plot border (right) 
				key->bounds.xleft = p_bounds->xright + pTerm->TicH;
				key->bounds.xright = key->bounds.xleft + key_width;
				*xinkey = key->bounds.xleft + _3DBlk.KeySizeLeft;
			}
			else if(key->region != GPKEY_AUTO_INTERIOR_LRTBC && key->margin == GPKEY_LMARGIN) {
				// keys outside plot border (left) 
				key->bounds.xright = p_bounds->xleft - pTerm->TicH;
				key->bounds.xleft = key->bounds.xright - key_width;
				*xinkey = key->bounds.xleft + _3DBlk.KeySizeLeft;
			}
			else if(key->hpos == LEFT) {
				key->bounds.xleft = p_bounds->xleft + pTerm->TicH;
				key->bounds.xright = key->bounds.xleft + key_width;
				*xinkey = key->bounds.xleft + _3DBlk.KeySizeLeft;
			}
			else {
				key->bounds.xright = p_bounds->xright - pTerm->TicH;
				key->bounds.xleft = key->bounds.xright - key_width;
				*xinkey = key->bounds.xleft + _3DBlk.KeySizeLeft;
			}
		}
		_3DBlk.YlRef = *yinkey - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
	}
	*yinkey -= (key->height_fix * pTerm->ChrV) / 2; // Center the key entries vertically, allowing for requested extra space 
}

void splot_map_activate()
{
	if(!_3DBlk.SPlotMapActive) {
		_3DBlk.SPlotMapActive = 1;
		// save current values 
		_3DBlk.SPlotMapSurfaceRotX = surface_rot_x;
		_3DBlk.SPlotMapSurfaceRotZ = surface_rot_z;
		_3DBlk.SPlotMapSurfaceScale = surface_scale;
		// set new values 
		surface_rot_x = 180.0f;
		surface_rot_z = 0.0f;
		// version 4 had constant value surface_scale = 1.3 
		surface_scale = 1.425f * mapview_scale;
		// The Y axis runs backwards from a normal 2D plot 
		GPO.AxS[FIRST_Y_AXIS].FlipProjection();
	}
}

void splot_map_deactivate()
{
	if(_3DBlk.SPlotMapActive) {
		_3DBlk.SPlotMapActive = 0;
		// restore the original values 
		surface_rot_x = _3DBlk.SPlotMapSurfaceRotX;
		surface_rot_z = _3DBlk.SPlotMapSurfaceRotZ;
		surface_scale = _3DBlk.SPlotMapSurfaceScale;
		// The Y axis runs backwards from a normal 2D plot 
		GPO.AxS[FIRST_Y_AXIS].FlipProjection();
	}
}

#ifdef BOXERROR_3D
// 
// 3D version of plot with boxerrorbars
// The only intended use for this is in xz projection, where it generates a
// box + errorbar plot oriented horizontally rather than vertically.
// 
//static void plot3d_boxerrorbars(GpTermEntry * pTerm, GpSurfacePoints * plot)
void GnuPlot::Plot3DBoxErrorBars(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int i;                  /* point index */
	double dx, dxl, dxh;    /* rectangle extent along X axis (vertical) */
	double dz, dzl, dzh;    /* rectangle extent along Z axis (horizontal) */
	double dy;              /* always 0 */
	int x0, y0, x1, y1;     /* terminal coordinates */
	int pass;
	int style = style_from_fill(&pPlot->fill_properties);
	int colortype = pPlot->fill_properties.border_color.type;
	if(!xz_projection) {
		IntWarn(NO_CARET, "splot 'with boxerrorbars' only works in xz projection");
		return;
	}
	// We make two passes through the data
	// 1st pass: draw the boxes
	// 2nd pass: draw the errorbars
	for(pass = 1; pass<=2; pass++) {
		iso_curve * icrvs = pPlot->iso_crvs;
		if(pass == 1) {
			if(colortype == TC_RGB)
				SetRgbColorConst(pTerm, pPlot->lp_properties.pm3d_color.lt);
		}
		if(pass == 2) {
			// Errorbar line style from "set bars" 
			if((bar_lp.flags & LP_ERRORBAR_SET) != 0)
				TermApplyLpProperties(pTerm, &bar_lp);
			else {
				TermApplyLpProperties(pTerm, &pPlot->lp_properties);
				NeedFillBorder(pTerm, &pPlot->fill_properties);
			}
		}
		while(icrvs) {
			GpCoordinate * points = icrvs->points;
			for(i = 0; i < icrvs->p_count; i++) {
				if(points[i].type == UNDEFINED)
					continue;
				dx  = points[i].x;
				dxh = dx + V.BoxWidth/2.0;
				dxl = dx - V.BoxWidth/2.0;
				dz = points[i].z;
				dzl = points[i].CRD_ZLOW;
				dzh = points[i].CRD_ZHIGH;
				dy = 0;
				// clip to border 
				cliptorange(dxl, AxS.__X().min, AxS.__X().max);
				cliptorange(dxh, AxS.__X().min, AxS.__X().max);
				cliptorange(dzl, AxS.__Z().min, AxS.__Z().max);
				cliptorange(dzh, AxS.__Z().min, AxS.__Z().max);
				cliptorange(dz, AxS.__Z().min, AxS.__Z().max);
				// Entire box is out of range 
				if(dxl == dxh && (dxl == AxS.__X().min || dxl == AxS.__X().max))
					continue;
				if(pass == 1) {
					// Variable color 
					Check3DForVariableColor(pTerm, pPlot, &points[i]);
					// Draw box 
					Map3D_XY(dxl, dy, AxS.__Z().min, &x0, &y0);
					Map3D_XY(dxh, dy, dz, &x1, &y1);
					(pTerm->fillbox)(pTerm, style, x0, MIN(y0, y1), (x1-x0), abs(y1-y0));
					// Draw border 
					if(NeedFillBorder(pTerm, &pPlot->fill_properties)) {
						newpath(pTerm);
						(pTerm->move)(pTerm, x0, y0);
						(pTerm->vector)(pTerm, x1, y0);
						(pTerm->vector)(pTerm, x1, y1);
						(pTerm->vector)(pTerm, x0, y1);
						(pTerm->vector)(pTerm, x0, y0);
						closepath(pTerm);
						if(pPlot->fill_properties.border_color.type != TC_DEFAULT)
							TermApplyLpProperties(pTerm, &pPlot->lp_properties);
					}
				}
				if(pass == 2) {
					int vl, vh;
					// conservative clipping 
					if((AxS.__X().min < AxS.__X().max) && (dx <= AxS.__X().min || dx >= AxS.__X().max))
						continue;
					if((AxS.__X().min > AxS.__X().max) && (dx <= AxS.__X().max || dx >= AxS.__X().min))
						continue;
					// Draw error bars 
					Map3D_XY(dxl, dy, dz, &x0, &vl);
					Map3D_XY(dxh, dy, dz, &x0, &vh);
					Map3D_XY(dx, dy, dzl, &x0, &y0);
					Map3D_XY(dx, dy, dzh, &x1, &y1);
					// Draw main error bar 
					(pTerm->move)(pTerm, x0, y0);
					(pTerm->vector)(pTerm, x1, y1);
					// Draw the whiskers perpendicular to the main bar 
					if(bar_size >= 0.0) {
						vl = static_cast<int>(y0 + bar_size * (y0 - vl));
						vh = static_cast<int>(y0 + bar_size * (y0 - vh));
					}
					DrawClipLine(pTerm, x0, vl, x0, vh);
					DrawClipLine(pTerm, x1, vl, x1, vh);
				}
			} /* loop over points */
			icrvs = icrvs->next;
		}
	} /* Passes 1 and 2 */
	// Restore base properties before key sample is drawn 
	TermApplyLpProperties(pTerm, &pPlot->lp_properties);
}

#else
//static void plot3d_boxerrorbars(GpTermEntry * pTerm, GpSurfacePoints * plot)
void GnuPlot::Plot3DBoxErrorBars(GpTermEntry * pTerm, GpSurfacePoints * plot)
{
	IntError(NO_CARET, "this copy of gnuplot does not support 3D boxerrorbars");
}
#endif /* BOXERROR_3D */
