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

GpGraph3DBlock::GpGraph3DBlock() : KeyEntryHeight(0), KeyTitleHeight(0), KeyTitleExtra(0), KeyTitleWidth(0),
	KeySampleLeft(0), KeySampleRight(0), KeyPointOffset(0), KeyTextLeft(0), KeyTextRight(0), KeySizeLeft(0), KeySizeRight(0),
	SPlotMapActive(0), SPlotMapSurfaceRotX(0.0f), SPlotMapSurfaceRotZ(0.0f), SPlotMapSurfaceScale(0.0f),
	XAxisY(0.0), YAxisX(0.0), ZAxisX(0.0), ZAxisY(0.0), XzPlane(false), YzPlane(false), CanPm3D(false),
	PTitlCnt(0), MaxPTitlLen(0), TitleLin(0), KeySampleWidth(0), KeyRows(0), KeyCols(0), KeyColWth(0), YlRef(0), KTitleLines(0.0),
	CeilingZ1(0.0), BaseZ1(0.0), SurfaceRotZ(30.0f), SurfaceRotX(60.0f), SurfaceScale(1.0f), SurfaceZScale(1.0f), SurfaceLScale(0.0f),
	MapviewScale(1.0f), Azimuth(0.0f), /*xmiddle(0), ymiddle(0), xscaler(0), yscaler(0),*/ xyscaler(0.0), radius_scaler(0.0),
	draw_contour(CONTOUR_NONE), clabel_interval(20), clabel_start(5), clabel_font(0), clabel_onecolor(false), draw_surface(true), implicit_surface(true),
	hidden3d(false), hidden3d_layer(LAYER_BACK), splot_map(false), xz_projection(false), yz_projection(false), in_3d_polygon(false), xyplane(0.5, false),
	floor_z(0.0), ceiling_z(0.0), base_z(0.0), floor_z1(0.0)
#ifdef USE_MOUSE
	,axis3d_o_x(0), axis3d_o_y(0), axis3d_x_dx(0), axis3d_x_dy(0), axis3d_y_dx(0), axis3d_y_dy(0)
#endif
{
	Back.Set(0.0);
	Right.Set(0.0);
	Front.Set(0.0);
	MEMSZERO(trans_mat);
}

#define i_inrange(z, a, b) inrange((z), (a), (b))

#define apx_eq(x, y) (fabs(x-y) < 0.001)
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define SQR(x) ((x) * (x))

int GnuPlot::FindMaxlCntr(const gnuplot_contours * pContours, int * pCount)
{
	int cnt = 0;
	int mlen = 0;
	for(const gnuplot_contours * p_cntrs = pContours; p_cntrs; p_cntrs = p_cntrs->next) {
		if(p_cntrs->isNewLevel) {
			const int len = EstimateStrlen(p_cntrs->label, NULL) - strspn(p_cntrs->label, " ");
			if(len)
				cnt++;
			if(len > mlen)
				mlen = len;
		}
	}
	*pCount += cnt;
	return (mlen);
}
//
// calculate the number and max-width of the keys for an splot.
// Note that a blank line is issued after each set of contours
//
int GnuPlot::FindMaxlKeys3D(const GpSurfacePoints * pPlots, int count, int * pKCnt)
{
	const GpSurfacePoints * this_plot = pPlots;
	int mlen = 0;
	int cnt = 0;
	for(int surf = 0; surf < count; this_plot = this_plot->next_sp, surf++) {
		// we draw a main entry if there is one, and we are
		// drawing either surface, or unlabeled contours
		if(this_plot->title && *this_plot->title && !this_plot->title_is_suppressed && !this_plot->title_position) {
			++cnt;
			const int len = EstimateStrlen(this_plot->title, NULL);
			SETMAX(mlen, len);
		}
		if(_3DBlk.draw_contour && !_3DBlk.clabel_onecolor && this_plot->contours && this_plot->plot_style != LABELPOINTS) {
			const int len = FindMaxlCntr(this_plot->contours, &cnt);
			SETMAX(mlen, len);
		}
	}
	ASSIGN_PTR(pKCnt, cnt);
	return mlen;
}
//
// borders of plotting area 
// computed once on every call to do_plot 
//
void GnuPlot::Boundary3D(GpTermEntry * pTerm, const GpSurfacePoints * plots, int count)
{
	legend_key * key = &Gg.KeyT;
	int i;
	_3DBlk.TitleLin = 0;
	_3DBlk.KeySampleWidth = (key->swidth >= 0.0) ? static_cast<int>(key->swidth * pTerm->CH() + pTerm->TicH) : 0;
	_3DBlk.KeyEntryHeight = static_cast<int>(pTerm->MulTicV(1.25 * key->vert_factor));
	if(_3DBlk.KeyEntryHeight < static_cast<int>(pTerm->CV())) {
		// is this reasonable ? 
		_3DBlk.KeyEntryHeight = static_cast<int>(pTerm->CV() * key->vert_factor);
	}
	// Approximate width of titles is used to determine number of rows, cols
	// The actual widths will be recalculated later
	_3DBlk.MaxPTitlLen = FindMaxlKeys3D(plots, count, &_3DBlk.PTitlCnt);
	_3DBlk.KeyTitleWidth = LabelWidth(key->title.text, &i) * pTerm->CH();
	_3DBlk.KTitleLines = i;
	_3DBlk.KeyColWth = (_3DBlk.MaxPTitlLen + 4) * pTerm->CH() + _3DBlk.KeySampleWidth;
	if(V.MarginL.scalex == screen)
		V.BbPlot.xleft = static_cast<int>(pTerm->MulMaxX(V.MarginL.x) + 0.5);
	else if(V.MarginL.x >= 0)
		V.BbPlot.xleft = static_cast<int>(V.MarginL.x * (double)pTerm->CH() + 0.5);
	else
		V.BbPlot.xleft = pTerm->CH() * 2 + pTerm->TicH;
	if(V.MarginR.scalex == screen)
		V.BbPlot.xright = static_cast<int>(pTerm->MulMaxX(V.MarginR.x) + 0.5);
	else // No tic label on the right side, so ignore rmargin 
		V.BbPlot.xright = static_cast<int>(pTerm->MulMaxX(V.Size.x) - pTerm->CH() * 2 - pTerm->TicH);
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
				_3DBlk.KeyCols = (int)(V.BbPlot.xright - V.BbPlot.xleft) / ((_3DBlk.MaxPTitlLen + 4) * pTerm->CH() + _3DBlk.KeySampleWidth);
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
		ExchangeForOrder(&V.MarginB.x, &V.MarginT.x);
	}
	// this should also consider the view and number of lines in xformat || yformat || xlabel || ylabel 
	if(V.MarginB.scalex == screen)
		V.BbPlot.ybot = static_cast<int>(pTerm->MulMaxY(V.MarginB.x) + 0.5);
	else if(_3DBlk.splot_map && V.MarginB.x >= 0)
		V.BbPlot.ybot = static_cast<int>((double)pTerm->CV() * V.MarginB.x);
	else
		V.BbPlot.ybot = static_cast<int>(pTerm->CV() * 2.5 + 1);
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
		V.BbPlot.ytop = static_cast<int>(pTerm->MulMaxY(V.MarginT.x) + 0.5);
	else // FIXME: Why no provision for tmargin in terms of character height? 
		V.BbPlot.ytop = static_cast<int>(V.Size.y * pTerm->MaxY - pTerm->CV() * (_3DBlk.TitleLin + 1.5) - 1);
	if(key->visible)
		if(key->region == GPKEY_AUTO_INTERIOR_LRTBC || (oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_RMARGIN)) {
			// calculate max no rows, limited by V.BbPlot.ytop-V.BbPlot.ybot 
			i = static_cast<int>((int)(V.BbPlot.ytop - V.BbPlot.ybot) / pTerm->CV() - 1 - _3DBlk.KTitleLines);
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
			int key_width = _3DBlk.KeyColWth * _3DBlk.KeyCols - 2 * pTerm->CH();
			if(V.MarginR.scalex != screen)
				V.BbPlot.xright -= key_width;
		}
	}
	if(key->visible)
		if(oneof2(key->region, GPKEY_AUTO_EXTERIOR_LRTBC, GPKEY_AUTO_EXTERIOR_MARGIN) && key->margin == GPKEY_LMARGIN) {
			int key_width = _3DBlk.KeyColWth * _3DBlk.KeyCols - 2 * pTerm->CH();
			if(V.MarginL.scalex != screen)
				V.BbPlot.xleft += key_width;
		}
	if(!_3DBlk.splot_map && V.AspectRatio3D > 0) {
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
		V.BbPlot.xleft += pTerm->MulMaxX(V.Offset.x);
	if(V.MarginR.scalex != screen)
		V.BbPlot.xright += pTerm->MulMaxX(V.Offset.x);
	if(V.MarginT.scalex != screen)
		V.BbPlot.ytop += pTerm->MulMaxY(V.Offset.y);
	if(V.MarginB.scalex != screen)
		V.BbPlot.ybot += pTerm->MulMaxY(V.Offset.y);
	_3DBlk.Middle.x = (V.BbPlot.xright + V.BbPlot.xleft) / 2;
	_3DBlk.Middle.y = (V.BbPlot.ytop + V.BbPlot.ybot) / 2;
	// HBB: Magic number alert! 
	_3DBlk.Scaler.x = ((V.BbPlot.xright - V.BbPlot.xleft) * 4L) / 7L;
	_3DBlk.Scaler.y = ((V.BbPlot.ytop - V.BbPlot.ybot) * 4L) / 7L;
	// Allow explicit control via set {}margin screen 
	if(V.MarginT.scalex == screen || V.MarginB.scalex == screen)
		_3DBlk.Scaler.y = static_cast<int>((V.BbPlot.ytop - V.BbPlot.ybot) / _3DBlk.SurfaceScale);
	if(V.MarginR.scalex == screen || V.MarginL.scalex == screen)
		_3DBlk.Scaler.x = static_cast<int>((V.BbPlot.xright - V.BbPlot.xleft) / _3DBlk.SurfaceScale);
	// prevent infinite loop or divide-by-zero if scaling is bad 
	SETIFZ(_3DBlk.Scaler.y, 1);
	SETIFZ(_3DBlk.Scaler.x, 1);
	// 'set size {square|ratio}' for splots 
	if(_3DBlk.splot_map && V.AspectRatio != 0.0f) {
		double current_aspect_ratio;
		if(V.AspectRatio < 0.0f && AxS.__X().GetRange() != 0.0) {
			current_aspect_ratio = -V.AspectRatio * fabs(AxS.__Y().GetRange() / AxS.__X().GetRange());
		}
		else
			current_aspect_ratio = V.AspectRatio;
		// {{{  set aspect ratio if valid and sensible 
		if(current_aspect_ratio >= 0.01 && current_aspect_ratio <= 100.0) {
			double current = (double)_3DBlk.Scaler.y / _3DBlk.Scaler.x;
			double required = current_aspect_ratio * pTerm->TicV / pTerm->TicH;
			if(current > required) // too tall 
				_3DBlk.Scaler.y = static_cast<int>(_3DBlk.Scaler.x * required);
			else // too wide 
				_3DBlk.Scaler.x = static_cast<int>(_3DBlk.Scaler.y / required);
		}
	}
	_3DBlk.xyscaler = sqrt(_3DBlk.Scaler.x * _3DBlk.Scaler.y); // For anything that really wants to be the same on x and y 
	_3DBlk.radius_scaler = _3DBlk.Scaler.x * _3DBlk.SurfaceScale / (AxS.__X().max - AxS.__X().min); // This one is used to scale circles in 3D plots 
	// Set default clipping 
	if(_3DBlk.splot_map)
		V.P_ClipArea = &V.BbPlot;
	else if(pTerm->CheckFlag(TERM_CAN_CLIP))
		V.P_ClipArea = NULL;
	else
		V.P_ClipArea = &V.BbCanvas;
}

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
		if(sstreq(pTerm->GetName(), "windows"))
			aspect = 1.0;
#endif
		if(pArrow->end.scalex != screen && pArrow->end.scalex != character && !_3DBlk.splot_map)
			return FALSE;
		Map3DPositionRDouble(pTerm, &pArrow->end, &junkw, &junkh, "arrow");
		radius = junkw;
		*pDex = *pDsx + cos(SMathConst::PiDiv180 * pArrow->angle) * radius;
		*pDey = *pDsy + sin(SMathConst::PiDiv180 * pArrow->angle) * radius * aspect;
	}
	else {
		Map3DPositionDouble(pTerm, &pArrow->end, pDex, pDey, "arrow");
	}
	return TRUE;
}

void GnuPlot::PlaceLabels3D(GpTermEntry * pTerm, text_label * pListHead, int layer)
{
	int x, y;
	pTerm->pointsize(pTerm, Gg.PointSize);
	for(text_label * p_label = pListHead; p_label; p_label = p_label->next) {
		if(p_label->layer != layer)
			continue;
		if(layer == LAYER_PLOTLABELS) {
			double xx, yy;
			Map3D_XY_double(p_label->place.x, p_label->place.y, p_label->place.z, &xx, &yy);
			x = static_cast<int>(xx);
			y = static_cast<int>(yy);
			// Only clip in 2D 
			if(_3DBlk.splot_map && V.ClipPoint(x, y))
				continue;
		}
		else
			Map3DPosition(pTerm, &p_label->place, &x, &y, "label");
		WriteLabel(pTerm, x, y, p_label);
	}
}

void GnuPlot::PlaceArrows3D(GpTermEntry * pTerm, int layer)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Allow arrows to run off the plot, so long as they are still on the canvas 
	V.P_ClipArea = pTerm->CheckFlag(TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	for(arrow_def * this_arrow = Gg.P_FirstArrow; this_arrow; this_arrow = this_arrow->next) {
		double dsx, dsy, dex, dey;
		if(this_arrow->arrow_properties.layer != layer)
			continue;
		if(GetArrow3D(pTerm, this_arrow, &dsx, &dsy, &dex, &dey)) {
			TermApplyLpProperties(pTerm, &(this_arrow->arrow_properties.lp_properties));
			ApplyHeadProperties(pTerm, &this_arrow->arrow_properties);
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
	int    surface;
	GpSurfacePoints * this_plot = NULL;
	int    xl = 0;
	int    yl = 0;
	int    xl_save;
	int    yl_save;
	int    xl_prev = 0;
	int    yl_prev = 0;
	//int    title_x = 0;
	//int    title_y = 0;
	SPoint2I title_pos;
	transform_matrix mat;
	int    key_count;
	bool   key_pass = FALSE;
	legend_key * key = &Gg.KeyT;
	bool   pm3d_order_depth = FALSE;
	GpAxis * primary_z;
	// Initiate transformation matrix using the global view variables. 
	if(_3DBlk.splot_map) {
		SPlotMapActivate();
	}
	else if(_3DBlk.xz_projection) {
		_3DBlk.SurfaceRotX = 270.0f;
		_3DBlk.SurfaceRotZ = 0.0f;
		_3DBlk.SurfaceScale = 1.425f * _3DBlk.MapviewScale;
	}
	else if(_3DBlk.yz_projection) {
		_3DBlk.SurfaceRotX = 90.0f;
		_3DBlk.SurfaceRotZ = 90.0f;
		_3DBlk.SurfaceScale = 1.425f * _3DBlk.MapviewScale;
		AxS[FIRST_Z_AXIS].FlipProjection();
	}
	_3DBlk.in_3d_polygon = FALSE; // protects polygons from xz, yz projections 
	mat_rot_z(_3DBlk.SurfaceRotZ, _3DBlk.trans_mat);
	mat_rot_x(_3DBlk.SurfaceRotX, mat);
	mat_mult(_3DBlk.trans_mat, _3DBlk.trans_mat, mat);
	mat_scale(_3DBlk.SurfaceScale / 2.0f, _3DBlk.SurfaceScale / 2.0f, _3DBlk.SurfaceScale / 2.0f, mat);
	mat_mult(_3DBlk.trans_mat, _3DBlk.trans_mat, mat);
	// The azimuth is applied as a rotation about the line of sight 
	if(_3DBlk.Azimuth != 0.0f && !_3DBlk.splot_map) {
		mat_rot_z(_3DBlk.Azimuth, mat);
		mat_mult(_3DBlk.trans_mat, _3DBlk.trans_mat, mat);
	}
	if(Gg.Polar)
		IntError(NO_CARET, "Cannot splot in polar coordinate system.");
	// In the case of a nonlinear z axis this points to the linear version 
	// that shadows it.  Otherwise it just points to FIRST_Z_AXIS.         
	primary_z = AxS.__Z().IsNonLinear() ? AxS.__Z().linked_to_primary : &AxS.__Z();
	// absolute or relative placement of xyplane along z 
	if(AxS.__Z().IsNonLinear()) {
		if(_3DBlk.xyplane.absolute) {
			if(primary_z->log && _3DBlk.xyplane.z <= 0)
				_3DBlk.BaseZ1 = EvalLinkFunction(primary_z, AxS.__Z().min);
			else
				_3DBlk.BaseZ1 = EvalLinkFunction(primary_z, _3DBlk.xyplane.z);
		}
		else
			_3DBlk.BaseZ1 = primary_z->min - (primary_z->GetRange() * _3DBlk.xyplane.z);
		_3DBlk.base_z = EvalLinkFunction(&AxS.__Z(), _3DBlk.BaseZ1);
	}
	else {
		_3DBlk.BaseZ1 = _3DBlk.xyplane.absolute ? _3DBlk.xyplane.z : (primary_z->min - (primary_z->GetRange() * _3DBlk.xyplane.z));
		_3DBlk.base_z = _3DBlk.BaseZ1;
	}
	// If we are to draw some portion of the xyplane make sure zmin is updated properly. 
	if(AxS.__X().ticmode || AxS.__Y().ticmode || Gg.draw_border & 0x00F) {
		if(primary_z->min > primary_z->max) {
			_3DBlk.floor_z1 = MAX(primary_z->min, _3DBlk.BaseZ1);
			_3DBlk.CeilingZ1 = MIN(primary_z->max, _3DBlk.BaseZ1);
		}
		else {
			_3DBlk.floor_z1 = MIN(primary_z->min, _3DBlk.BaseZ1);
			_3DBlk.CeilingZ1 = MAX(primary_z->max, _3DBlk.BaseZ1);
		}
	}
	else {
		_3DBlk.floor_z1 = primary_z->min;
		_3DBlk.CeilingZ1 = primary_z->max;
	}
	if(AxS.__Z().IsNonLinear()) {
		_3DBlk.floor_z = EvalLinkFunction(&AxS.__Z(), _3DBlk.floor_z1);
		_3DBlk.ceiling_z = EvalLinkFunction(&AxS.__Z(), _3DBlk.CeilingZ1);
	}
	else {
		_3DBlk.floor_z = _3DBlk.floor_z1;
		_3DBlk.ceiling_z = _3DBlk.CeilingZ1;
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
	if(!_3DBlk.splot_map && (_3DBlk.SurfaceRotX == 90.0f || _3DBlk.SurfaceRotX == 270.0f)) {
		if(_3DBlk.SurfaceRotZ == 0.0f || _3DBlk.SurfaceRotZ == 180.0f) {
			_3DBlk.XzPlane = true;
			_3DBlk.base_z = _3DBlk.floor_z;
		}
		if(_3DBlk.SurfaceRotZ == 90.0f || _3DBlk.SurfaceRotZ == 270.0f) {
			_3DBlk.YzPlane = true;
			if(_3DBlk.SurfaceRotX == 270.0f || _3DBlk.yz_projection)
				_3DBlk.base_z = _3DBlk.ceiling_z;
		}
	}
	TermStartPlot(pTerm);
	pTerm->Layer_(TERM_LAYER_3DPLOT);
	GpU.screen_ok = FALSE;
	pTerm->Layer_(TERM_LAYER_BACKTEXT); // Sync point for epslatex text positioning 
	Boundary3D(pTerm, plots, pcount); // now compute boundary for plot 
	axis_set_scale_and_range(&AxS[FIRST_X_AXIS], V.BbPlot.xleft, V.BbPlot.xright);
	axis_set_scale_and_range(&AxS[FIRST_Y_AXIS], V.BbPlot.ybot, V.BbPlot.ytop);
	axis_set_scale_and_range(&AxS[FIRST_Z_AXIS], static_cast<int>(_3DBlk.floor_z), static_cast<int>(_3DBlk.ceiling_z));
	// SCALE FACTORS 
	_3DBlk.Scale3D.z = 2.0 / (_3DBlk.ceiling_z - _3DBlk.floor_z) * _3DBlk.SurfaceZScale;
	_3DBlk.Scale3D.y = 2.0 / AxS.__Y().GetRange();
	_3DBlk.Scale3D.x = 2.0 / AxS.__X().GetRange();
	if(AxS.__X().IsNonLinear())
		_3DBlk.Scale3D.x = 2.0 / AxS.__X().linked_to_primary->GetRange();
	if(AxS.__Y().IsNonLinear())
		_3DBlk.Scale3D.y = 2.0 / AxS.__Y().linked_to_primary->GetRange();
	if(AxS.__Z().IsNonLinear())
		_3DBlk.Scale3D.z = 2.0 / (_3DBlk.CeilingZ1 - _3DBlk.floor_z1) * _3DBlk.SurfaceZScale;
	// Allow 'set view equal xy' to adjust rendered length of the X and/or Y axes.
	// NB: only works correctly for terminals whose coordinate system is isotropic. 
	//xcenter3d = ycenter3d = zcenter3d = 0.0;
	_3DBlk.Center3D.Set(0.0, 0.0, 0.0);
	if(V.AspectRatio3D >= 2) {
		if(_3DBlk.Scale3D.y > _3DBlk.Scale3D.x) {
			_3DBlk.Center3D.y = 1.0 - _3DBlk.Scale3D.x/_3DBlk.Scale3D.y;
			_3DBlk.Scale3D.y = _3DBlk.Scale3D.x;
		}
		else if(_3DBlk.Scale3D.x > _3DBlk.Scale3D.y) {
			_3DBlk.Center3D.x = 1.0 - _3DBlk.Scale3D.y/_3DBlk.Scale3D.x;
			_3DBlk.Scale3D.x = _3DBlk.Scale3D.y;
		}
		if(V.AspectRatio3D >= 3)
			_3DBlk.Scale3D.z = _3DBlk.Scale3D.x;
	}
	// FIXME: I do not understand why this is correct 
	if(AxS.__Z().IsNonLinear())
		_3DBlk.Center3D.z = 0.0;
	// Without this the rotation center would be located at 
	// the bottom of the plot. This places it in the middle.
	else
		_3DBlk.Center3D.z =  -(_3DBlk.ceiling_z - _3DBlk.floor_z) / 2.0 * _3DBlk.Scale3D.z + 1.0;
	// Needed for mousing by outboard terminal drivers 
	if(_3DBlk.splot_map) {
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
		_3DBlk.CanPm3D = (IsPlotWithPalette() && !MakePalette(pTerm) && !pTerm->CheckFlag(TERM_NULL_SET_COLOR));
	}
	// Give a chance for rectangles to be behind everything else 
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_BEHIND, 3);
	if(replot_mode != AXIS_ONLY_ROTATE)
		PlacePixmaps(pTerm, LAYER_BEHIND, 3);
	TermApplyLpProperties(pTerm, &Gg.border_lp); /* border linetype */
	// must come before using Draw3DGraphBox() the first time 
	Setup3DBoxCorners();
	// DRAW GRID AND BORDER 
	// Original behaviour: draw entire grid in back, if 'set grid back': 
	// HBB 20040331: but not if in hidden3d mode 
	if(_3DBlk.splot_map && Gg.border_layer != LAYER_FRONT)
		Draw3DGraphBox(pTerm, plots, pcount, BORDERONLY, LAYER_BACK);
	else if(!_3DBlk.hidden3d && (AxS.grid_layer == LAYER_BACK))
		Draw3DGraphBox(pTerm, plots, pcount, ALLGRID, LAYER_BACK);
	else if(!_3DBlk.hidden3d && (AxS.grid_layer == LAYER_BEHIND))
		// Default layering mode.  Draw the back part now, but not if
		// hidden3d is in use, because that relies on all isolated
		// lines being output after all surfaces have been defined. 
		Draw3DGraphBox(pTerm, plots, pcount, BACKGRID, LAYER_BACK);
	else if(_3DBlk.hidden3d && Gg.border_layer == LAYER_BEHIND)
		Draw3DGraphBox(pTerm, plots, pcount, ALLGRID, LAYER_BACK);
	// Save state of V.BbPlot before applying rotations, etc 
	memcpy(&V.BbPage, &V.BbPlot, sizeof(V.BbPage));
	// Clipping in 'set view map' mode should be like 2D clipping 
	// FIXME:  Wasn't this already done in boundary3d?            
	if(_3DBlk.splot_map) {
		int map_x1, map_y1, map_x2, map_y2;
		Map3D_XY(AxS.__X().min, AxS.__Y().min, _3DBlk.base_z, &map_x1, &map_y1);
		Map3D_XY(AxS.__X().max, AxS.__Y().max, _3DBlk.base_z, &map_x2, &map_y2);
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
	else if(_3DBlk.Azimuth == 0.0f) {
		int xl, xb, xr, xf, yl, yb, yr, yf;
		Map3D_XY(_3DBlk.ZAxisX, _3DBlk.ZAxisY,   _3DBlk.base_z, &xl, &yl);
		Map3D_XY(_3DBlk.Back.x,  _3DBlk.Back.y,  _3DBlk.base_z, &xb, &yb);
		Map3D_XY(_3DBlk.Right.x, _3DBlk.Right.y, _3DBlk.base_z, &xr, &yr);
		Map3D_XY(_3DBlk.Front.x, _3DBlk.Front.y, _3DBlk.base_z, &xf, &yf);
		V.BbPlot.xleft = MIN(xl, xb); /* Always xl? */
		V.BbPlot.xright = MAX(xb, xr); /* Always xr? */
	}
	// PLACE TITLE 
	if(Gg.LblTitle.text != 0) {
		int x, y;
		if(_3DBlk.splot_map) { /* case 'set view map' */
			int map_x1, map_y1, map_x2, map_y2;
			int tics_len = 0;
			if(AxS.__X().ticmode & TICS_MIRROR) {
				tics_len = (int)pTerm->MulTicV(AxS.__X().ticscale * (AxS.__X().TicIn ? -1 : 1));
				if(tics_len < 0) tics_len = 0; /* take care only about upward tics */
			}
			Map3D_XY(AxS.__X().min, AxS.__Y().min, _3DBlk.base_z, &map_x1, &map_y1);
			Map3D_XY(AxS.__X().max, AxS.__Y().max, _3DBlk.base_z, &map_x2, &map_y2);
			// Distance between the title base line and graph top line or the upper part of
			// tics is as given by character height: 
			x = ((map_x1 + map_x2) / 2);
			y = static_cast<int>(map_y1 + tics_len + (_3DBlk.TitleLin + 0.5) * (pTerm->CV()));
		}
		else { // usual 3d set view ... 
			x = (V.BbPlot.xleft + V.BbPlot.xright) / 2;
			y = (V.BbPlot.ytop + _3DBlk.TitleLin * (pTerm->CH()));
		}
		// Save title position for later 
		title_pos.x = x;
		title_pos.y = y;
	}
	// PLACE TIMELABEL 
	if(Gg.LblTime.text) {
		int x = pTerm->CV();
		int y = Gg.TimeLabelBottom ? static_cast<int>(V.Offset.y * AxS.__Y().max + pTerm->CV()) : (V.BbPlot.ytop - pTerm->CV());
		DoTimeLabel(pTerm, x, y);
	}
	// Add 'back' color box 
	if((replot_mode != AXIS_ONLY_ROTATE) && _3DBlk.CanPm3D && IsPlotWithColorbox() && Gg.ColorBox.layer == LAYER_BACK)
		DrawColorSmoothBox(pTerm, MODE_SPLOT);
	PlaceObjects(pTerm, Gg.GridWall, LAYER_BACK, 3); /* Grid walls */
	PlacePixmaps(pTerm, LAYER_BACK, 3); /* pixmaps before objects so that a rectangle can be used as a border */
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_BACK, 3); /* Add 'back' rectangles */
	PlaceLabels3D(pTerm, Gg.P_FirstLabel, LAYER_BACK); /* PLACE LABELS */
	PlaceArrows3D(pTerm, LAYER_BACK); // PLACE ARROWS 
	pTerm->Layer_(TERM_LAYER_FRONTTEXT); /* Sync point for epslatex text positioning */
	if(_3DBlk.hidden3d && _3DBlk.draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
		InitHiddenLineRemoval();
		ResetHiddenLineRemoval();
	}
	// WORK OUT KEY POSITION AND SIZE 
	Do3DKeyLayout(pTerm, key, &xl, &yl);
	// "set key opaque" requires two passes, with the key drawn in the second pass 
	xl_save = xl; 
	yl_save = yl;
SECOND_KEY_PASS:
	// This tells the canvas, qt, and svg terminals to restart the plot   
	// count so that key titles are in sync with the plots they describe. 
	pTerm->Layer_(TERM_LAYER_RESET_PLOTNO);
	// Key box 
	if(key->visible) {
		pTerm->Layer_(TERM_LAYER_KEYBOX);
		/* In two-pass mode, we blank out the key area after the graph	*/
		/* is drawn and then redo the key in the blank area.		*/
		if(key_pass && pTerm->fillbox && !pTerm->CheckFlag(TERM_NULL_SET_COLOR)) {
			pTerm->set_color(pTerm, &key->fillcolor);
			pTerm->FillBox_(FS_OPAQUE, key->bounds.xleft, key->bounds.ybot, key->bounds.xright - key->bounds.xleft, key->bounds.ytop - key->bounds.ybot);
		}
		if(key->box.l_type > LT_NODRAW && key->bounds.ytop != key->bounds.ybot) {
			TermApplyLpProperties(pTerm, &key->box);
			newpath(pTerm);
			ClipMove(key->bounds.xleft, key->bounds.ybot);
			ClipVector(pTerm, key->bounds.xleft, key->bounds.ytop);
			ClipVector(pTerm, key->bounds.xright, key->bounds.ytop);
			ClipVector(pTerm, key->bounds.xright, key->bounds.ybot);
			ClipVector(pTerm, key->bounds.xleft, key->bounds.ybot);
			closepath(pTerm);
			// draw a horizontal line between key title and first entry  JFi 
			ClipMove(key->bounds.xleft, key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra);
			ClipVector(pTerm, key->bounds.xright, key->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra);
		}
		if(key->title.text) {
			int center = (key->bounds.xright + key->bounds.xleft) / 2;
			int titley = key->bounds.ytop - _3DBlk.KeyTitleHeight/2;
			// FIXME: empirical tweak. I don't know why this is needed 
			titley += (_3DBlk.KTitleLines-1) * pTerm->CV()/2;
			WriteLabel(pTerm, center, titley, &key->title);
			pTerm->SetLineType_(LT_BLACK);
		}
	}
	// DRAW SURFACES AND CONTOURS 
	if(!key_pass)
		if(_3DBlk.hidden3d && (_3DBlk.hidden3d_layer == LAYER_BACK) && _3DBlk.draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
			pTerm->Layer_(TERM_LAYER_BEFORE_PLOT);
			Plot3DHidden(pTerm, plots, pcount);
			pTerm->Layer_(TERM_LAYER_AFTER_PLOT);
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
	_3DBlk.YlRef = yl -= _3DBlk.KeyEntryHeight / 2; /* centralise the keys */
	/* PM January 2005: The mistake of missing blank lines in the data file is
	 * so frequently made (see questions at comp.graphics.apps.gnuplot) that it
	 * really deserves this warning. But don't show it too often --- only if it
	 * is a single surface in the plot.
	 */
	if(plots->plot_style != BOXES)
		if(pcount == 1 && plots->num_iso_read == 1 && _3DBlk.CanPm3D && (plots->plot_style == PM3DSURFACE || PM3D_IMPLICIT == _Pm3D.pm3d.implicit))
			fprintf(stderr, "  Warning: Single isoline (scan) is not enough for a pm3d plot.\n\t   Hint: Missing blank lines in the data file? See 'help pm3d' and FAQ.\n");
	pm3d_order_depth = (_3DBlk.CanPm3D && !_3DBlk.draw_contour && _Pm3D.pm3d.direction == PM3D_DEPTH);
	// TODO:
	//  During "refresh" from rotation it would be better to re-use previously
	//  built quadrangle list rather than clearing and rebuilding it.
	if(pm3d_order_depth || _Pm3D.track_pm3d_quadrangles) {
		Pm3DDepthQueueClear();
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
			pTerm->Layer_(TERM_LAYER_BEFORE_PLOT);
			if(!key_pass && this_plot->plot_type != KEYENTRY)
				if(_3DBlk.CanPm3D && PM3D_IMPLICIT == _Pm3D.pm3d.implicit)
					Pm3DDrawOne(pTerm, this_plot);
			lkey = (key->visible && this_plot->title && this_plot->title[0] && !this_plot->title_is_suppressed);
			draw_this_surface = (_3DBlk.draw_surface && !this_plot->opt_out_of_surface);
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
				if(this_plot->title_is_automated && pTerm->CheckFlag(TERM_IS_LATEX))
					title = texify_title(title, this_plot->plot_type);
				if(key->textcolor.type != TC_DEFAULT)
					ApplyPm3DColor(pTerm, &key->textcolor); /* Draw key text in same color as key title */
				else
					pTerm->SetLineType_(LT_BLACK); /* Draw key text in black */
				IgnoreEnhanced(this_plot->title_no_enhanced);
				KeyText(pTerm, xl, yl, title);
				IgnoreEnhanced(false);
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
					    if(!_3DBlk.hidden3d)
						    Plot3DImpulses(pTerm, this_plot);
					    break;
					case STEPS: /* HBB: I think these should be here */
					case FILLSTEPS:
					case FSTEPS:
					case HISTEPS:
					case SURFACEGRID:
					case LINES:
					    if(draw_this_surface) {
						    if(!_3DBlk.hidden3d || this_plot->opt_out_of_hidden3d)
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
						    if(!_3DBlk.hidden3d || this_plot->opt_out_of_hidden3d)
							    Plot3DPoints(pTerm, this_plot);
					    }
					    break;
					case LINESPOINTS:
					    if(draw_this_surface) {
						    if(!_3DBlk.hidden3d || this_plot->opt_out_of_hidden3d) {
							    Plot3DLinesPm3D(pTerm, this_plot);
							    Plot3DPoints(pTerm, this_plot);
						    }
					    }
					    break;
					case VECTOR:
					    if(!_3DBlk.hidden3d || this_plot->opt_out_of_hidden3d)
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
						    if(_3DBlk.CanPm3D && PM3D_IMPLICIT != _Pm3D.pm3d.implicit) {
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
						    if(_3DBlk.hidden3d && !(this_plot->opt_out_of_hidden3d))
							    break;
						    if(_3DBlk.draw_contour && !(this_plot->opt_out_of_contours))
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
					    if(!(_3DBlk.hidden3d && draw_this_surface))
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
					    else if(this_plot->contours && _3DBlk.clabel_onecolor) {
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
			if(_3DBlk.draw_contour && this_plot->contours) {
				gnuplot_contours * cntrs = this_plot->contours;
				lp_style_type thiscontour_lp_properties;
				static char * thiscontour_label = NULL;
				bool save_hidden3d;
				int ic = 1; /* ic will index the contour linetypes */
				thiscontour_lp_properties = this_plot->lp_properties;
				TermApplyLpProperties(pTerm, &(thiscontour_lp_properties));
				while(cntrs) {
					if(!_3DBlk.clabel_onecolor && cntrs->isNewLevel) {
						if(key->visible && !this_plot->title_is_suppressed && this_plot->plot_style != LABELPOINTS) {
							pTerm->SetLineType_(LT_BLACK);
							KeyText(pTerm, xl, yl, cntrs->label);
						}
						if(thiscontour_lp_properties.pm3d_color.type == TC_Z)
							set_color(pTerm, Cb2Gray(cntrs->z) );
						else {
							lp_style_type ls = thiscontour_lp_properties;
							int contour_linetype;
							ic++; /* Increment linetype used for contour */
							// First contour line type defaults to surface linetype + 1  
							// but can be changed using 'set cntrparams firstlinetype N' 
							if(_Cntr.ContourFirstLineType > 0)
								contour_linetype = _Cntr.ContourFirstLineType + ic - 2;
							else
								contour_linetype = this_plot->hidden3d_top_linetype + ic;
							// hidden3d processing looks directly at l_type 
							// for other purposes the line color is set by load_linetype 
							if(_3DBlk.hidden3d)
								thiscontour_lp_properties.l_type = contour_linetype - 1;
							LoadLineType(pTerm, &ls, contour_linetype);
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
							    Cntr3DImpulses(pTerm, cntrs, &thiscontour_lp_properties);
							    break;
							case STEPS:
							case FSTEPS:
							case HISTEPS:
							// treat all the above like 'lines' 
							case LINES:
							case PM3DSURFACE:
							    save_hidden3d = _3DBlk.hidden3d;
							    if(this_plot->opt_out_of_hidden3d)
								    _3DBlk.hidden3d = false;
							    Cntr3DLines(pTerm, cntrs, &thiscontour_lp_properties);
							    _3DBlk.hidden3d = save_hidden3d;
							    break;
							case LINESPOINTS:
							    Cntr3DLines(pTerm, cntrs, &thiscontour_lp_properties);
							// @fallthrough to draw the points 
							case DOTS:
							case POINTSTYLE:
							    Cntr3DPoints(pTerm, cntrs, &thiscontour_lp_properties);
							    break;
							case LABELPOINTS:
							    if(cntrs->isNewLevel) {
								    const char * c = &cntrs->label[strspn(cntrs->label, " ")];
								    SAlloc::F(thiscontour_label);
								    thiscontour_label = sstrdup(c);
							    }
							    Cntr3DLabels(pTerm, cntrs, thiscontour_label, this_plot->labels);
							    break;
							default:
							    break;
						}
					cntrs = cntrs->next;
				}
			}
			// Sync point for end of this curve (used by svg, post, ...) 
			pTerm->Layer_(TERM_LAYER_AFTER_PLOT);
		}
	if(!key_pass)
		if(pm3d_order_depth || _Pm3D.track_pm3d_quadrangles) {
			Pm3DDepthQueueFlush(pTerm); // draw pending plots 
		}
	if(!key_pass)
		if(_3DBlk.hidden3d && (_3DBlk.hidden3d_layer == LAYER_FRONT) && _3DBlk.draw_surface && (replot_mode != AXIS_ONLY_ROTATE)) {
			pTerm->Layer_(TERM_LAYER_BEFORE_PLOT);
			Plot3DHidden(pTerm, plots, pcount);
			pTerm->Layer_(TERM_LAYER_AFTER_PLOT);
		}
	// Draw grid and border.
	// The 1st case allows "set border behind" to override hidden3d processing.
	// The 2nd case either leaves everything to hidden3d or forces it to the front.
	// The 3rd case is the non-hidden3d default - draw back pieces (done earlier),
	// then the graph, and now the front pieces.
	if(_3DBlk.hidden3d && Gg.border_layer == LAYER_BEHIND)
		// the important thing is _not_ to draw the back grid 
		// Draw3DGraphBox(plots, pcount, FRONTGRID, LAYER_FRONT) 
		;
	else if(_3DBlk.hidden3d || AxS.grid_layer == LAYER_FRONT)
		Draw3DGraphBox(pTerm, plots, pcount, ALLGRID, LAYER_FRONT);
	else if(AxS.grid_layer == LAYER_BEHIND)
		Draw3DGraphBox(pTerm, plots, pcount, FRONTGRID, LAYER_FRONT);
	// Go back and draw the legend in a separate pass if "key opaque" 
	if(key->visible && key->front && !key_pass) {
		key_pass = TRUE;
		xl = xl_save; yl = yl_save;
		goto SECOND_KEY_PASS;
	}
	// Add 'front' color box 
	if((replot_mode != AXIS_ONLY_ROTATE) && _3DBlk.CanPm3D && IsPlotWithColorbox() && Gg.ColorBox.layer == LAYER_FRONT)
		DrawColorSmoothBox(pTerm, MODE_SPLOT);
	// Add 'front' rectangles 
	PlacePixmaps(pTerm, LAYER_FRONT, 3);
	PlaceObjects(pTerm, Gg.P_FirstObject, LAYER_FRONT, 3);
	PlaceObjects(pTerm, Gg.GridWall, LAYER_FRONT, 3); /* Grid walls */
	PlaceLabels3D(pTerm, Gg.P_FirstLabel, LAYER_FRONT); /* PLACE LABELS */
	PlaceArrows3D(pTerm, LAYER_FRONT); // PLACE ARROWS 
	// PLACE TITLE LAST 
	if(Gg.LblTitle.text)
		PlaceTitle(pTerm, title_pos.x, title_pos.y);
#ifdef USE_MOUSE
	// finally, store the 2d projection of the x and y axis, to enable zooming by mouse 
	{
		int x, y;
		Map3D_XY(AxS.__X().min, AxS.__Y().min, _3DBlk.base_z, &_3DBlk.axis3d_o_x, &_3DBlk.axis3d_o_y);
		Map3D_XY(AxS.__X().max, AxS.__Y().min, _3DBlk.base_z, &x, &y);
		_3DBlk.axis3d_x_dx = x - _3DBlk.axis3d_o_x;
		_3DBlk.axis3d_x_dy = y - _3DBlk.axis3d_o_y;
		Map3D_XY(AxS.__X().min, AxS.__Y().max, _3DBlk.base_z, &x, &y);
		_3DBlk.axis3d_y_dx = x - _3DBlk.axis3d_o_x;
		_3DBlk.axis3d_y_dy = y - _3DBlk.axis3d_o_y;
	}
#endif
	// Release the palette if we have used one (PostScript only?) 
	if(IsPlotWithPalette() && pTerm->previous_palette)
		pTerm->previous_palette(pTerm);
	TermEndPlot(pTerm);
	if(_3DBlk.hidden3d && _3DBlk.draw_surface) {
		TermHiddenLineRemoval();
	}
	if(_3DBlk.splot_map)
		SPlotMapDeactivate();
	else if(_3DBlk.xz_projection || _3DBlk.yz_projection)
		_3DBlk.SurfaceScale = 1.0f;
	else if(_3DBlk.yz_projection)
		AxS[FIRST_Z_AXIS].FlipProjection();
}
// 
// plot3d_impulses:
// Plot the surfaces in IMPULSES style
// 
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
				    Map3D_XY(points[i].Pt, &x, &y);
				    double z = AxS.__Z().ClipToRange(0.0);
				    Map3D_XY(points[i].Pt.GetXY(), z, &xx0, &yy0);
				    ClipMove(xx0, yy0);
				    ClipVector(pTerm, x, y);
				    break;
			    }
				case OUTRANGE:
			    {
				    if(!AxS.__X().InRange(points[i].Pt.x) || !AxS.__Y().InRange(points[i].Pt.y))
					    break;
				    if(AxS.__Z().InRange(0.0)) {
					    // zero point is INRANGE 
					    Map3D_XY(points[i].Pt.GetXY(), 0.0, &xx0, &yy0);
					    // must cross z = AxS.__Z().min or AxS.__Z().max limits 
					    if(inrange(AxS.__Z().min, 0.0, points[i].Pt.z) && AxS.__Z().min != 0.0 && AxS.__Z().min != points[i].Pt.z) {
						    Map3D_XY(points[i].Pt.GetXY(), AxS.__Z().min, &x, &y);
					    }
					    else {
						    Map3D_XY(points[i].Pt.GetXY(), AxS.__Z().max, &x, &y);
					    }
				    }
				    else {
					    // zero point is also OUTRANGE 
					    if(inrange(AxS.__Z().min, 0.0, points[i].Pt.z) && inrange(AxS.__Z().max, 0.0, points[i].Pt.z)) {
						    // crosses z = AxS.__Z().min or AxS.__Z().max limits 
						    Map3D_XY(points[i].Pt.GetXY(), AxS.__Z().max, &x, &y);
						    Map3D_XY(points[i].Pt.GetXY(), AxS.__Z().min, &xx0, &yy0);
					    }
					    else {
						    // doesn't cross z = AxS.__Z().min or AxS.__Z().max limits 
						    break;
					    }
				    }
				    ClipMove(xx0, yy0);
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

void GnuPlot::Plot3DLines(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int x, y, xx0, yy0; // point in terminal coordinates 
	iso_curve * icrvs = pPlot->iso_crvs;
	bool rgb_from_column;
	// These are handled elsewhere.  
	if(pPlot->has_grid_topology && _3DBlk.hidden3d)
		return;
	// These don't need to be drawn at all 
	if(pPlot->lp_properties.l_type != LT_NODRAW) {
		rgb_from_column = pPlot->pm3d_color_from_column && pPlot->lp_properties.pm3d_color.type == TC_RGB && pPlot->lp_properties.pm3d_color.value < 0.0;
		while(icrvs) {
			enum coord_type prev = UNDEFINED; /* type of previous plot */
			const GpCoordinate * points = icrvs->points;
			for(int i = 0; i < icrvs->p_count; i++) {
				if(rgb_from_column)
					SetRgbColorVar(pTerm, (uint)points[i].CRD_COLOR);
				else if(pPlot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
					pPlot->lp_properties.pm3d_color.lt = (int)(points[i].CRD_COLOR);
					ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
				}
				switch(points[i].type) {
					case INRANGE: {
						Map3D_XY(points[i].Pt, &x, &y);
						if(prev == INRANGE) {
							ClipVector(pTerm, x, y);
						}
						else {
							if(prev == OUTRANGE) {
								// from outrange to inrange 
								if(!Gg.ClipLines1)
									ClipMove(x, y);
								else {
									//
									// Calculate intersection point and draw vector from there
									//
									//double clip_x, clip_y, clip_z;
									SPoint3R _clip;
									Edge3DIntersect(&points[i-1], &points[i], _clip);
									Map3D_XY(_clip, &xx0, &yy0);
									ClipMove(xx0, yy0);
									ClipVector(pTerm, x, y);
								}
							}
							else {
								ClipMove(x, y);
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
								//double clip_x, clip_y, clip_z;
								SPoint3R _clip;
								Edge3DIntersect(&points[i-1], &points[i], _clip);
								Map3D_XY(_clip, &xx0, &yy0);
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
									ClipMove(x, y);
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
}
// 
// this is basically the same function as above, but:
//   - it splits the bunch of scans in two sets corresponding to
//   the two scan directions.
//   - reorders the two sets -- from behind to front
//   - checks if inside on scan of a set the order should be inverted
// 
void GnuPlot::Plot3DLinesPm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int invert[2] = {0, 0};
	int n[2] = {0, 0};
	int x, y, xx0, yy0; // point in terminal coordinates 
	enum coord_type prev = UNDEFINED;
	//double z;
	// just a shortcut 
	bool color_from_column = pPlot->pm3d_color_from_column;
	// If plot really uses RGB rather than pm3d colors, let plot3d_lines take over 
	if(pPlot->lp_properties.pm3d_color.type == TC_RGB) {
		ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
		Plot3DLines(pTerm, pPlot);
	}
	else if(pPlot->lp_properties.pm3d_color.type == TC_LT) {
		Plot3DLines(pTerm, pPlot);
	}
	else if(pPlot->lp_properties.pm3d_color.type == TC_LINESTYLE) {
		Plot3DLines(pTerm, pPlot);
	}
	else if(!pPlot->has_grid_topology || !_3DBlk.hidden3d) { // These are handled elsewhere.  
		iso_curve ** icrvs_pair[2];
		// split the bunch of scans in two sets in
		// which the scans are already depth ordered 
		Pm3DRearrangeScanArray(pPlot, icrvs_pair, &n[0], &invert[0], icrvs_pair + 1, &n[1], &invert[1]);
		for(int set = 0; set < 2; set++) {
			int begin = 0;
			const int step = invert[set] ? -1 /* begin is set below to the length of the scan-1 */ : 1;
			for(int scan = 0; scan < n[set] && icrvs_pair[set]; scan++) {
				const iso_curve * icrvs = icrvs_pair[set][scan];
				if(invert[set]) {
					begin = icrvs->p_count - 1;
				}
				prev = UNDEFINED; // type of previous plot 
				const GpCoordinate * points = icrvs->points;
				for(int cnt = 0, i = begin; cnt < icrvs->p_count; cnt++, i += step) {
					switch(points[i].type) {
						case INRANGE:
							Map3D_XY(points[i].Pt, &x, &y);
							if(prev == INRANGE) {
								const double z = color_from_column ? ((points[i-step].CRD_COLOR + points[i].CRD_COLOR) * 0.5) : ((points[i-step].Pt.z + points[i].Pt.z) * 0.5);
								set_color(pTerm, Cb2Gray(z));
								ClipVector(pTerm, x, y);
							}
							else {
								if(prev == OUTRANGE) {
									// from outrange to inrange 
									if(!Gg.ClipLines1) {
										ClipMove(x, y);
									}
									else {
										// Calculate intersection point and draw vector from there
										//double clip_x, clip_y, clip_z;
										SPoint3R _clip;
										Edge3DIntersect(&points[i-step], &points[i], _clip);
										Map3D_XY(_clip, &xx0, &yy0);
										ClipMove(xx0, yy0);
										const double z = color_from_column ? ((points[i-step].CRD_COLOR + points[i].CRD_COLOR) * 0.5) : ((points[i-step].Pt.z + points[i].Pt.z) * 0.5);
										set_color(pTerm, Cb2Gray(z));
										ClipVector(pTerm, x, y);
									}
								}
								else {
									ClipMove(x, y);
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
									//double clip_x, clip_y, clip_z;
									SPoint3R _clip;
									Edge3DIntersect(&points[i-step], &points[i], _clip);
									Map3D_XY(_clip, &xx0, &yy0);
									const double z = color_from_column ? ((points[i-step].CRD_COLOR + points[i].CRD_COLOR) * 0.5) : ((points[i-step].Pt.z + points[i].Pt.z) * 0.5);
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
										ClipMove(x, y);
										const double z = color_from_column ? ((points[i-step].CRD_COLOR + points[i].CRD_COLOR) * 0.5) : ((points[i-step].Pt.z + points[i].Pt.z) * 0.5);
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
				}
			}
		}
		SAlloc::F(icrvs_pair[0]);
		SAlloc::F(icrvs_pair[1]);
	}
}
// 
// Plot the surfaces in POINTSTYLE style
// 
void GnuPlot::Plot3DPoints(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	int i;
	int x, y;
	iso_curve * icrvs = pPlot->iso_crvs;
	int interval = pPlot->lp_properties.p_interval;
	// Set whatever we can that applies to every point in the loop 
	if(pPlot->lp_properties.PtType == PT_CHARACTER) {
		IgnoreEnhanced(true);
		if(pPlot->labels->font && pPlot->labels->font[0])
			(pTerm->set_font)(pTerm, pPlot->labels->font);
		pTerm->justify_text(pTerm, CENTRE);
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
				Map3D_XY(point->Pt, &x, &y);
				if(!V.ClipPoint(x, y)) {
					// A negative interval indicates we should blank 
					// out the area behind the point symbol          
					if(pPlot->plot_style == LINESPOINTS && interval < 0) {
						pTerm->set_color(pTerm, &background_fill);
						(pTerm->pointsize)(pTerm, Gg.PointSize * Gg.PointIntervalBox);
						pTerm->Pnt_(x, y, 6);
						TermApplyLpProperties(pTerm, &(pPlot->lp_properties));
					}
					Check3DForVariableColor(pTerm, pPlot, point);
					if((pPlot->plot_style == POINTSTYLE || pPlot->plot_style == LINESPOINTS) && pPlot->lp_properties.PtSize == PTSZ_VARIABLE)
						(pTerm->pointsize)(pTerm, Gg.PointSize * point->CRD_PTSIZE);
					// We could dummy up circles as a point of type 7, but this way 
					// the radius can use x-axis coordinates rather than pointsize. 
					// FIXME: track per-plot fillstyle 
					if(pPlot->plot_style == CIRCLES) {
						const double radius = point->CRD_PTSIZE * _3DBlk.radius_scaler;
						DoArc(pTerm, x, y, radius, 0.0, 360.0, style_from_fill(&Gg.default_fillstyle), FALSE);
						// Retrace the border if the style requests it 
						if(NeedFillBorder(pTerm, &Gg.default_fillstyle))
							DoArc(pTerm, x, y, radius, 0., 360., 0, FALSE);
						continue;
					}
					// This code is also used for "splot ... with dots" 
					if(pPlot->plot_style == DOTS) {
						pTerm->Pnt_(x, y, -1);
						continue;
					}
					// variable point type 
					if((pPlot->lp_properties.PtType == PT_VARIABLE) && !(isnan(point->CRD_PTTYPE))) {
						pTerm->Pnt_(x, y, (int)(point->CRD_PTTYPE) - 1);
					}
					// Print special character rather than drawn symbol 
					if(pPlot->lp_properties.PtType == PT_CHARACTER)
						ptchar = pPlot->lp_properties.p_char;
					else if(pPlot->lp_properties.PtType == PT_VARIABLE && isnan(point->CRD_PTTYPE))
						ptchar = (char *)(&point->CRD_PTCHAR);
					else
						ptchar = NULL;
					if(ptchar) {
						if(pPlot->labels)
							ApplyPm3DColor(pTerm, &(pPlot->labels->textcolor));
						pTerm->put_text(pTerm, x, y, ptchar);
					}
					// The normal case 
					else if(pPlot->lp_properties.PtType >= -1)
						pTerm->Pnt_(x, y, pPlot->lp_properties.PtType);
				}
			}
		}
		icrvs = icrvs->next;
	}
	// Return to initial state 
	if(pPlot->lp_properties.PtType == PT_CHARACTER) {
		if(pPlot->labels->font && pPlot->labels->font[0])
			(pTerm->set_font)(pTerm, "");
		IgnoreEnhanced(false);
	}
}
// 
// cntr3d_impulses:
// Plot a surface contour in IMPULSES style
// 
void GnuPlot::Cntr3DImpulses(GpTermEntry * pTerm, gnuplot_contours * cntr, lp_style_type * lp)
{
	GpVertex vertex_on_surface;
	GpVertex vertex_on_base;
	if(_3DBlk.draw_contour & CONTOUR_SRF) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].Pt, &vertex_on_surface);
			Map3D_XYZ(cntr->coords[i].Pt.GetXY(), 0.0, &vertex_on_base);
			// HBB 20010822: Provide correct color-coding for "linetype palette" PM3D mode 
			vertex_on_base.real_z = cntr->coords[i].Pt.z;
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
void GnuPlot::Cntr3DLines(GpTermEntry * pTerm, gnuplot_contours * cntr, lp_style_type * lp)
{
	int i; // point index 
	GpVertex this_vertex;
	// In the case of "set view map" (only) clip the contour lines to the graph 
	BoundingBox * clip_save = V.P_ClipArea;
	if(_3DBlk.splot_map)
		V.P_ClipArea = &V.BbPlot;
	if(_3DBlk.draw_contour & CONTOUR_SRF) {
		Map3D_XYZ(cntr->coords[0].Pt, &this_vertex);
		// move slightly frontward, to make sure the contours are
		// visible in front of the the triangles they're in, if this is a hidden3d plot 
		if(_3DBlk.hidden3d && !VERTEX_IS_UNDEFINED(this_vertex))
			this_vertex.z += 1e-2;
		Polyline3DStart(pTerm, &this_vertex);
		for(i = 1; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].Pt, &this_vertex);
			// move slightly frontward, to make sure the contours are
			// visible in front of the the triangles they're in, if this
			// is a hidden3d plot */
			if(_3DBlk.hidden3d && !VERTEX_IS_UNDEFINED(this_vertex))
				this_vertex.z += 1e-2;
			Polyline3DNext(pTerm, &this_vertex, lp);
		}
	}
	if(_3DBlk.draw_contour & CONTOUR_BASE) {
		Map3D_XYZ(cntr->coords[0].Pt.GetXY(), _3DBlk.base_z, &this_vertex);
		this_vertex.real_z = cntr->coords[0].Pt.z;
		Polyline3DStart(pTerm, &this_vertex);
		for(i = 1; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].Pt.GetXY(), _3DBlk.base_z, &this_vertex);
			this_vertex.real_z = cntr->coords[i].Pt.z;
			Polyline3DNext(pTerm, &this_vertex, lp);
		}
	}
	if(_3DBlk.splot_map)
		V.P_ClipArea = clip_save;
}
//
// cntr3d_points:
// Plot a surface contour in POINTSTYLE style
//
void GnuPlot::Cntr3DPoints(GpTermEntry * pTerm, gnuplot_contours * cntr, lp_style_type * lp)
{
	GpVertex v;
	if(_3DBlk.draw_contour & CONTOUR_SRF) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].Pt, &v);
			// move slightly frontward, to make sure the contours and
			// points are visible in front of the triangles they're
			// in, if this is a hidden3d plot 
			if(_3DBlk.hidden3d && !VERTEX_IS_UNDEFINED(v))
				v.z += 1e-2;
			Draw3DPoint(pTerm, &v, lp);
		}
	}
	if(_3DBlk.draw_contour & CONTOUR_BASE) {
		for(int i = 0; i < cntr->num_pts; i++) {
			Map3D_XYZ(cntr->coords[i].Pt.GetXY(), _3DBlk.base_z, &v);
			// HBB 20010822: see above 
			v.real_z = cntr->coords[i].Pt.z;
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
void GnuPlot::Cntr3DLabels(GpTermEntry * pTerm, gnuplot_contours * cntr, char * pLevelText, text_label * pLabel)
{
	int i;
	int x, y;
	GpVertex v;
	lp_style_type * lp = &(pLabel->lp_properties);
	// Drawing a label at every point would be too crowded 
	int interval = lp->p_interval;
	if(interval <= 0) 
		interval = 999; // Place label only at start point 
	if(_3DBlk.draw_contour & CONTOUR_BASE) {
		for(i = 0; i < cntr->num_pts; i++) {
			if((i-_3DBlk.clabel_start) % interval) // Offset to avoid sitting on the border 
				continue;
			Map3D_XY(cntr->coords[i].Pt.GetXY(), _3DBlk.base_z, &x, &y);
			pLabel->text = pLevelText;
			pLabel->font = _3DBlk.clabel_font;
			if(_3DBlk.hidden3d) {
				Map3D_XYZ(cntr->coords[i].Pt.GetXY(), _3DBlk.base_z, &v);
				v.real_z = cntr->coords[i].Pt.z;
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
#define MAP_HEIGHT_X(x) (((x)-AxS.__X().min)/(AxS.__X().GetRange()) > 0.9 ? 1 : 0)
#define MAP_HEIGHT_Y(y) (((y)-AxS.__Y().min)/(AxS.__Y().GetRange()) > 0.9 ? 1 : 0)
// 
// if point is at corner, update height[][] and depth[][]
// we are still assuming that extremes of surfaces are at corners,
// but we are not assuming order of corners
// 
void GnuPlot::CheckCornerHeight(GpCoordinate * p, double height[2][2], double depth[2][2])
{
	if(p->type == INRANGE) {
		// FIXME HBB 20010121: don't compare 'zero' to data values in absolute terms. 
		if((fabs(p->Pt.x - AxS.__X().min) < Gg.Zero || fabs(p->Pt.x - AxS.__X().max) < Gg.Zero) && (fabs(p->Pt.y - AxS.__Y().min) < Gg.Zero || fabs(p->Pt.y - AxS.__Y().max) < Gg.Zero)) {
			int x = MAP_HEIGHT_X(p->Pt.x);
			int y = MAP_HEIGHT_Y(p->Pt.y);
			SETMAX(height[x][y], p->Pt.z);
			SETMIN(depth[x][y], p->Pt.z);
		}
	}
}
//
// work out where the axes and tics are drawn 
//
void GnuPlot::Setup3DBoxCorners()
{
	int quadrant = static_cast<int>(_3DBlk.SurfaceRotZ / 90.0f);
	if((quadrant + 1) & 2) {
		_3DBlk.ZAxisX   = AxS.__X().max;
		_3DBlk.Right.x  = AxS.__X().min;
		_3DBlk.Back.y   = AxS.__Y().min;
		_3DBlk.Front.y  = AxS.__Y().max;
	}
	else {
		_3DBlk.ZAxisX   = AxS.__X().min;
		_3DBlk.Right.x  = AxS.__X().max;
		_3DBlk.Back.y   = AxS.__Y().max;
		_3DBlk.Front.y  = AxS.__Y().min;
	}
	if(quadrant & 2) {
		_3DBlk.ZAxisY   = AxS.__Y().max;
		_3DBlk.Right.y  = AxS.__Y().min;
		_3DBlk.Back.x   = AxS.__X().max;
		_3DBlk.Front.x  = AxS.__X().min;
	}
	else {
		_3DBlk.ZAxisY   = AxS.__Y().min;
		_3DBlk.Right.y  = AxS.__Y().max;
		_3DBlk.Back.x   = AxS.__X().min;
		_3DBlk.Front.x  = AxS.__X().max;
	}
	quadrant = static_cast<int>(_3DBlk.SurfaceRotX / 90.0f);
	if((quadrant & 2) && !_3DBlk.splot_map) {
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
void GnuPlot::Draw3DGraphBox(GpTermEntry * pTerm, const GpSurfacePoints * pPlot, int plotNum, WHICHGRID whichgrid, int currentLayer)
{
	int    x, y; // point in terminal coordinates 
	BoundingBox * clip_save = V.P_ClipArea;
	FPRINTF((stderr, "draw_3d_graphbox: whichgrid = %d current_layer = %d border_layer = %d\n", whichgrid, currentLayer, Gg.border_layer));
	V.P_ClipArea = &V.BbCanvas;
	if(Gg.draw_border && _3DBlk.splot_map) {
		if(Gg.border_layer == currentLayer) {
			TermApplyLpProperties(pTerm, &Gg.border_lp);
			if((Gg.draw_border & 15) == 15)
				newpath(pTerm);
			Map3D_XY(_3DBlk.ZAxisX, _3DBlk.ZAxisY, _3DBlk.base_z, &x, &y);
			ClipMove(x, y);
			Map3D_XY(_3DBlk.Back.x, _3DBlk.Back.y, _3DBlk.base_z, &x, &y);
			if(Gg.draw_border & 2)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(_3DBlk.Right.x, _3DBlk.Right.y, _3DBlk.base_z, &x, &y);
			if(Gg.draw_border & 8)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(_3DBlk.Front.x, _3DBlk.Front.y, _3DBlk.base_z, &x, &y);
			if(Gg.draw_border & 4)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(_3DBlk.ZAxisX, _3DBlk.ZAxisY, _3DBlk.base_z, &x, &y);
			if(Gg.draw_border & 1)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			if((Gg.draw_border & 15) == 15)
				closepath(pTerm);
		}
	}
	else if(Gg.draw_border && _3DBlk.yz_projection) {
		if(Gg.border_layer == currentLayer) {
			GpAxis * yaxis = &AxS[FIRST_Y_AXIS];
			GpAxis * zaxis = &AxS[FIRST_Z_AXIS];
			TermApplyLpProperties(pTerm, &Gg.border_lp);
			if((Gg.draw_border & 15) == 15)
				newpath(pTerm);
			Map3D_XY(0.0, yaxis->min, zaxis->min, &x, &y);
			ClipMove(x, y);
			Map3D_XY(0.0, yaxis->max, zaxis->min, &x, &y);
			if(Gg.draw_border & 8)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(0.0, yaxis->max, zaxis->max, &x, &y);
			if(Gg.draw_border & 4)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(0.0, yaxis->min, zaxis->max, &x, &y);
			if(Gg.draw_border & 2)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(0.0, yaxis->min, zaxis->min, &x, &y);
			if(Gg.draw_border & 1)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			if((Gg.draw_border & 15) == 15)
				closepath(pTerm);
		}
	}
	else if(Gg.draw_border && _3DBlk.xz_projection) {
		if(Gg.border_layer == currentLayer) {
			GpAxis * xaxis = &AxS[FIRST_X_AXIS];
			GpAxis * zaxis = &AxS[FIRST_Z_AXIS];
			TermApplyLpProperties(pTerm, &Gg.border_lp);
			if((Gg.draw_border & 15) == 15)
				newpath(pTerm);
			Map3D_XY(xaxis->min, 0.0, zaxis->min, &x, &y);
			ClipMove(x, y);
			Map3D_XY(xaxis->max, 0.0, zaxis->min, &x, &y);
			if(Gg.draw_border & 2)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(xaxis->max, 0.0, zaxis->max, &x, &y);
			if(Gg.draw_border & 4)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(xaxis->min, 0.0, zaxis->max, &x, &y);
			if(Gg.draw_border & 8)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			Map3D_XY(xaxis->min, 0.0, zaxis->min, &x, &y);
			if(Gg.draw_border & 1)
				ClipVector(pTerm, x, y);
			else
				ClipMove(x, y);
			if((Gg.draw_border & 15) == 15)
				closepath(pTerm);
		}
	}
	else if(Gg.draw_border) {
		// the four corners of the base plane, in normalized view coordinates (-1..1) on all three axes. 
		GpVertex bl;
		GpVertex bb;
		GpVertex br;
		GpVertex bf;
		// map to normalized view coordinates the corners of the baseplane: left, back, right and front, in that order: 
		Map3D_XYZ(_3DBlk.ZAxisX,  _3DBlk.ZAxisY,  _3DBlk.base_z, &bl);
		Map3D_XYZ(_3DBlk.Back.x,  _3DBlk.Back.y,  _3DBlk.base_z, &bb);
		Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, _3DBlk.base_z, &br);
		Map3D_XYZ(_3DBlk.Front.x, _3DBlk.Front.y, _3DBlk.base_z, &bf);
		if(BACKGRID != whichgrid) {
			// Draw front part of base grid, right to front corner: 
			if(Gg.draw_border & 4)
				Draw3DLine(pTerm, &br, &bf, &Gg.border_lp);
			// ... and left to front: 
			if(Gg.draw_border & 1)
				Draw3DLine(pTerm, &bl, &bf, &Gg.border_lp);
		}
		if(FRONTGRID != whichgrid) {
			// Draw back part of base grid: left to back corner: 
			if(Gg.draw_border & 2)
				Draw3DLine(pTerm, &bl, &bb, &Gg.border_lp);
			// ... and right to back: 
			if(Gg.draw_border & 8)
				Draw3DLine(pTerm, &br, &bb, &Gg.border_lp);
		}
		// if surface is drawn, draw the rest of the graph box, too: 
		if(_3DBlk.draw_surface || (_3DBlk.draw_contour & CONTOUR_SRF) || (_Pm3D.pm3d.implicit == PM3D_IMPLICIT && strpbrk(_Pm3D.pm3d.where, "st"))) {
			GpVertex fl, fb, fr, ff; // floor left/back/right/front corners 
			GpVertex tl, tb, tr, tf; // top left/back/right/front corners 
			Map3D_XYZ(_3DBlk.ZAxisX,  _3DBlk.ZAxisY,  _3DBlk.floor_z, &fl);
			Map3D_XYZ(_3DBlk.Back.x,  _3DBlk.Back.y,  _3DBlk.floor_z, &fb);
			Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, _3DBlk.floor_z, &fr);
			Map3D_XYZ(_3DBlk.Front.x, _3DBlk.Front.y, _3DBlk.floor_z, &ff);
			Map3D_XYZ(_3DBlk.ZAxisX,  _3DBlk.ZAxisY,  _3DBlk.ceiling_z, &tl);
			Map3D_XYZ(_3DBlk.Back.x,  _3DBlk.Back.y,  _3DBlk.ceiling_z, &tb);
			Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, _3DBlk.ceiling_z, &tr);
			Map3D_XYZ(_3DBlk.Front.x, _3DBlk.Front.y, _3DBlk.ceiling_z, &tf);
			if((Gg.draw_border & 0xf0) == 0xf0) {
				// all four verticals are drawn - save some time by
				// drawing them to the full height, regardless of
				// where the surface lies 
				if(FRONTGRID != whichgrid) {
					// Draw the back verticals floor-to-ceiling, left: 
					Draw3DLine(pTerm, &fl, &tl, &Gg.border_lp);
					// ... back: 
					Draw3DLine(pTerm, &fb, &tb, &Gg.border_lp);
					// ... and right 
					Draw3DLine(pTerm, &fr, &tr, &Gg.border_lp);
				}
				if(BACKGRID != whichgrid) {
					// Draw the front vertical: floor-to-ceiling, front: 
					Draw3DLine(pTerm, &ff, &tf, &Gg.border_lp);
				}
			}
			else {
				// find heights of surfaces at the corners of the xy rectangle 
				double height[2][2];
				double depth[2][2];
				const int zaxis_i = MAP_HEIGHT_X(_3DBlk.ZAxisX);
				const int zaxis_j = MAP_HEIGHT_Y(_3DBlk.ZAxisY);
				const int back_i = MAP_HEIGHT_X(_3DBlk.Back.x);
				const int back_j = MAP_HEIGHT_Y(_3DBlk.Back.y);
				height[0][0] = height[0][1] = height[1][0] = height[1][1] = _3DBlk.base_z;
				depth[0][0] = depth[0][1] = depth[1][0] = depth[1][1] = _3DBlk.base_z;
				//
				// FIXME HBB 20000617: this method contains the
				// assumption that the topological corners of the
				// surface mesh(es) are also the geometrical ones of
				// their xy projections. This is only true for
				// 'explicit' surface datasets, i.e. z(x,y) 
				//
				if(Gg.CornerPoles) {
					for(; --plotNum >= 0; pPlot = pPlot->next_sp) {
						const iso_curve * curve = pPlot->iso_crvs;
						if(!oneof3(pPlot->plot_type, NODATA, KEYENTRY, VOXELDATA)) {
							int iso;
							if(pPlot->plot_type == DATA3D) {
								if(!pPlot->has_grid_topology)
									continue;
								iso = pPlot->num_iso_read;
							}
							else
								iso = Gg.IsoSamples2;
							const int count = curve->p_count;
							if(count) {
								CheckCornerHeight(curve->points, height, depth);
								CheckCornerHeight(curve->points + count - 1, height, depth);
								while(--iso)
									curve = curve->next;
								CheckCornerHeight(curve->points, height, depth);
								CheckCornerHeight(curve->points + count - 1, height, depth);
							}
						}
					}
				}

#define VERTICAL(mask, x, y, i, j, bottom, top)                       \
	if(Gg.draw_border&mask) {                         \
		Draw3DLine(pTerm, bottom, top, &Gg.border_lp);        \
	} \
	else if(height[i][j] != depth[i][j] && (AxS.__X().ticmode || AxS.__Y().ticmode || Gg.draw_border & 0x00F)) { \
		GpVertex a, b;                                \
		Map3D_XYZ(x, y, depth[i][j], &a);              \
		Map3D_XYZ(x, y, height[i][j], &b);             \
		Draw3DLine(pTerm, &a, &b, &Gg.border_lp);            \
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
				if(Gg.draw_border & 0x100)
					Draw3DLine(pTerm, &tl, &tb, &Gg.border_lp);
				/* ... and top right to back: */
				if(Gg.draw_border & 0x200)
					Draw3DLine(pTerm, &tr, &tb, &Gg.border_lp);
			}
			if(BACKGRID != whichgrid) {
				// Draw front part of top of box: top left to front corner: 
				if(Gg.draw_border & 0x400)
					Draw3DLine(pTerm, &tl, &tf, &Gg.border_lp);
				// ... and top right to front: 
				if(Gg.draw_border & 0x800)
					Draw3DLine(pTerm, &tr, &tf, &Gg.border_lp);
			}
		}
	}
	// In 'set view map' mode, treat grid as in 2D plots 
	if(_3DBlk.splot_map && currentLayer != abs(AxS.grid_layer)) {
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
			const GpAxis * primary = AxS.__X().linked_to_primary;
			mid_x = (primary->max + primary->min) / 2.;
			mid_x = EvalLinkFunction(&AxS.__X(), mid_x);
		}
		else {
			mid_x = (AxS.__X().max + AxS.__X().min) / 2.;
		}
		Map3D_XYZ(mid_x, _3DBlk.XAxisY, _3DBlk.base_z, &v0);
		Map3D_XYZ(mid_x, other_end,     _3DBlk.base_z, &v1);
		// Unusual case: 2D projection of the xz plane 
		if(!_3DBlk.splot_map && _3DBlk.XzPlane)
			Map3D_XYZ(mid_x, _3DBlk.XAxisY, AxS.__Z().max+AxS.__Z().min-_3DBlk.base_z, &v1);
		//tic_unitx = (v1.x - v0.x) / xyscaler;
		//tic_unity = (v1.y - v0.y) / xyscaler;
		//tic_unitz = (v1.z - v0.z) / xyscaler;
		_3DBlk.TicUnit.Set((v1.x - v0.x) / _3DBlk.xyscaler, (v1.y - v0.y) / _3DBlk.xyscaler, (v1.z - v0.z) / _3DBlk.xyscaler);
		// Don't output tics and grids if this is the front part of a
		// two-part grid drawing process: 
		if((_3DBlk.SurfaceRotX <= 90.0f && FRONTGRID != whichgrid) || (_3DBlk.SurfaceRotX > 90.0f && BACKGRID != whichgrid))
			if(AxS.__X().ticmode)
				GenTics(pTerm, &AxS[FIRST_X_AXIS], &GnuPlot::XTickCallback);
		if(AxS.__X().label.text) {
			if((_3DBlk.SurfaceRotX <= 90.0f && BACKGRID != whichgrid) || (_3DBlk.SurfaceRotX > 90.0f && FRONTGRID != whichgrid) || _3DBlk.splot_map) {
				int x1, y1;
				if(_3DBlk.splot_map) { /* case 'set view map' */
					// copied from xtick_callback(): baseline of tics labels 
					GpVertex v1, v2;
					Map3D_XYZ(mid_x, _3DBlk.XAxisY, _3DBlk.base_z, &v1);
					v2.x = v1.x;
					v2.y = v1.y - _3DBlk.TicUnit.y * pTerm->CV();
					if(!AxS.__X().TicIn)
						v2.y -= pTerm->MulTicV(_3DBlk.TicUnit.y * AxS.__X().ticscale);
					TERMCOORD(&v2, x1, y1);
					// Default displacement with respect to baseline of tics labels 
					y1 -= (1.5 * pTerm->CV());
				}
				else { // usual 3d set view ... 
					if(AxS.__X().label.tag == ROTATE_IN_3D_LABEL_TAG) {
						double ang, angx0, angx1, angy0, angy1;
						Map3D_XY_double(AxS.__X().min, _3DBlk.XAxisY, _3DBlk.base_z, &angx0, &angy0);
						Map3D_XY_double(AxS.__X().max, _3DBlk.XAxisY, _3DBlk.base_z, &angx1, &angy1);
						ang = atan2(angy1-angy0, angx1-angx0) / SMathConst::PiDiv180;
						if(ang < -90) 
							ang += 180;
						if(ang > 90) 
							ang -= 180;
						AxS.__X().label.rotate = (ang > 0) ? ffloori(ang + 0.5) : ffloori(ang - 0.5);
					}
					if(AxS.__X().ticmode & TICS_ON_AXIS) {
						Map3D_XYZ(mid_x, 0.0, _3DBlk.base_z, &v1);
					}
					else {
						Map3D_XYZ(mid_x, _3DBlk.XAxisY, _3DBlk.base_z, &v1);
					}
					if(_3DBlk.xz_projection) {
						v1.x -= pTerm->MulTicH(3.0 * _3DBlk.TicUnit.x);
						v1.y -= pTerm->MulTicH(3.0 * _3DBlk.TicUnit.y);
					}
					else if(AxS.__X().ticmode & TICS_ON_AXIS) {
						v1.x += pTerm->MulTicH(2.0 * ((AxS.__X().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.x);
						v1.y += pTerm->MulTicH(2.0 * ((AxS.__X().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.y);
					}
					else {
						v1.x -= pTerm->MulTicH(10.0 * _3DBlk.TicUnit.x);
						v1.y -= pTerm->MulTicH(10.0 * _3DBlk.TicUnit.y);
					}
					if(!AxS.__X().TicIn) {
						v1.x -= pTerm->MulTicH(_3DBlk.TicUnit.x * AxS.__X().ticscale);
						v1.y -= pTerm->MulTicH(_3DBlk.TicUnit.y * AxS.__X().ticscale);
					}
					TERMCOORD(&v1, x1, y1);
				}
				WriteLabel(pTerm, x1, y1, &AxS.__X().label);
			}
		}
		if(_3DBlk.splot_map && AxS[SECOND_X_AXIS].ticmode)
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
		Map3D_XYZ(_3DBlk.YAxisX, mid_y, _3DBlk.base_z, &v0);
		Map3D_XYZ(other_end,     mid_y, _3DBlk.base_z, &v1);
		// Unusual case: 2D projection of the yz plane 
		if(!_3DBlk.splot_map && _3DBlk.YzPlane)
			Map3D_XYZ(_3DBlk.YAxisX, mid_y, AxS.__Z().max+AxS.__Z().min-_3DBlk.base_z, &v1);
		//_3DBlk.TicUnit.x = (v1.x - v0.x) / xyscaler;
		//_3DBlk.TicUnit.y = (v1.y - v0.y) / xyscaler;
		//_3DBlk.TicUnit.z = (v1.z - v0.z) / xyscaler;
		_3DBlk.TicUnit.Set((v1.x - v0.x) / _3DBlk.xyscaler, (v1.y - v0.y) / _3DBlk.xyscaler, (v1.z - v0.z) / _3DBlk.xyscaler);
		// Don't output tics and grids if this is the front part of a two-part grid drawing process: 
		if((_3DBlk.SurfaceRotX <= 90.0f && FRONTGRID != whichgrid) || (_3DBlk.SurfaceRotX > 90.0f && BACKGRID != whichgrid))
			if(AxS.__Y().ticmode)
				GenTics(pTerm, &AxS[FIRST_Y_AXIS], &GnuPlot::YTickCallback);
		if(AxS.__Y().label.text) {
			if((_3DBlk.SurfaceRotX <= 90.0f && BACKGRID != whichgrid) || (_3DBlk.SurfaceRotX > 90.0f && FRONTGRID != whichgrid) || _3DBlk.splot_map) {
				int x1, y1;
				int save_rotate = AxS.__Y().label.rotate;
				if(_3DBlk.splot_map) { /* case 'set view map' */
					// copied from ytick_callback(): baseline of tics labels 
					GpVertex v1, v2;
					Map3D_XYZ(_3DBlk.YAxisX, mid_y, _3DBlk.base_z, &v1);
					if(AxS.__Y().ticmode & TICS_ON_AXIS && !AxS.__X().log && AxS.__X().InRange(0.0)) {
						Map3D_XYZ(0.0, _3DBlk.YAxisX, _3DBlk.base_z, &v1);
					}
					v2.x = v1.x - _3DBlk.TicUnit.x * pTerm->CH() * 1;
					v2.y = v1.y;
					if(!AxS.__X().TicIn)
						v2.x -= pTerm->MulTicH(_3DBlk.TicUnit.x * AxS.__X().ticscale);
					TERMCOORD(&v2, x1, y1);
					// calculate max length of y-tics labels 
					AxS.WidestTicLen = 0;
					if(AxS.__Y().ticmode & TICS_ON_BORDER) {
						AxS.WidestTicLen = 0; // reset the global variable 
						GenTics(pTerm, &AxS[FIRST_Y_AXIS], &GnuPlot::WidestTicCallback);
					}
					// Default displacement with respect to baseline of tics labels 
					x1 -= (0.5 + AxS.WidestTicLen) * pTerm->CH();
				}
				else { // usual 3d set view ...
					if(AxS.__Y().label.tag == ROTATE_IN_3D_LABEL_TAG) {
						double ang, angx0, angx1, angy0, angy1;
						Map3D_XY_double(_3DBlk.YAxisX, AxS.__Y().min, _3DBlk.base_z, &angx0, &angy0);
						Map3D_XY_double(_3DBlk.YAxisX, AxS.__Y().max, _3DBlk.base_z, &angx1, &angy1);
						ang = atan2(angy1-angy0, angx1-angx0) / SMathConst::PiDiv180;
						if(ang < -90) 
							ang += 180;
						if(ang > 90) 
							ang -= 180;
						AxS.__Y().label.rotate = (ang > 0.0) ? ffloori(ang + 0.5) : ffloori(ang - 0.5);
					}
					else if(!_3DBlk.yz_projection) {
						// The 2D default state (ylabel rotate) is not wanted in 3D 
						AxS.__Y().label.rotate = 0;
					}
					if(AxS.__Y().ticmode & TICS_ON_AXIS) {
						Map3D_XYZ(0.0, mid_y, _3DBlk.base_z, &v1);
					}
					else {
						Map3D_XYZ(_3DBlk.YAxisX, mid_y, _3DBlk.base_z, &v1);
					}
					if(_3DBlk.yz_projection) {
						v1.x -= pTerm->MulTicH(3.0 * _3DBlk.TicUnit.x);
						v1.y -= pTerm->MulTicH(3.0 * _3DBlk.TicUnit.y);
					}
					else if(AxS.__Y().ticmode & TICS_ON_AXIS) {
						v1.x += pTerm->MulTicH(2.0 * ((AxS.__Y().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.x);
						v1.y += pTerm->MulTicH(2.0 * ((AxS.__Y().TicIn) ? 1.0 : -1.0) * _3DBlk.TicUnit.y);
					}
					else {
						v1.x -= pTerm->MulTicH(10.0 * _3DBlk.TicUnit.x);
						v1.y -= pTerm->MulTicH(10.0 * _3DBlk.TicUnit.y);
					}
					if(!AxS.__Y().TicIn) {
						v1.x -= pTerm->MulTicV(_3DBlk.TicUnit.x * AxS.__Y().ticscale);
						v1.y -= pTerm->MulTicV(_3DBlk.TicUnit.y * AxS.__Y().ticscale);
					}
					TERMCOORD(&v1, x1, y1);
				}
				WriteLabel(pTerm, x1, y1, &AxS.__Y().label);
				AxS.__Y().label.rotate = save_rotate;
			}
		}
		if(_3DBlk.splot_map && AxS[SECOND_Y_AXIS].ticmode)
			GenTics(pTerm, &AxS[SECOND_Y_AXIS], &GnuPlot::YTickCallback);
	}
	// do z tics 
	if(AxS.__Z().ticmode &&
	    // Don't output tics and grids if this is the front part of a two-part grid drawing process: 
	    (FRONTGRID != whichgrid) && !_3DBlk.splot_map && (_3DBlk.SurfaceRotX != 0.0f) && (_3DBlk.draw_surface || (_3DBlk.draw_contour & CONTOUR_SRF) || strchr(_Pm3D.pm3d.where, 's'))) {
		GenTics(pTerm, &AxS[FIRST_Z_AXIS], &GnuPlot::ZTickCallback);
	}
	if(AxS.__Y().zeroaxis && !AxS.__X().log && AxS.__X().InRange(0.0)) {
		GpVertex v1, v2;
		// line through x=0 
		Map3D_XYZ(0.0, AxS.__Y().min, _3DBlk.base_z, &v1);
		Map3D_XYZ(0.0, AxS.__Y().max, _3DBlk.base_z, &v2);
		Draw3DLine(pTerm, &v1, &v2, AxS.__Y().zeroaxis);
	}
	if(AxS.__Z().zeroaxis && !AxS.__X().log && AxS.__X().InRange(0.0)) {
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
		Map3D_XYZ(AxS.__X().min, 0.0, _3DBlk.base_z, &v1);
		Map3D_XYZ(AxS.__X().max, 0.0, _3DBlk.base_z, &v2);
		Draw3DLine(pTerm, &v1, &v2, AxS.__X().zeroaxis);
	}
	// PLACE ZLABEL - along the middle grid Z axis - eh ? 
	if(AxS.__Z().label.text && !_3DBlk.splot_map && (currentLayer == LAYER_FRONT || whichgrid == ALLGRID) && (_3DBlk.draw_surface || (_3DBlk.draw_contour & CONTOUR_SRF) || strpbrk(_Pm3D.pm3d.where, "st"))) {
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
			x -= 5 * pTerm->CH();
		}
		else {
			Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, mid_z, &v1);
			TERMCOORD(&v1, x, y);
			if(fabs(_3DBlk.Azimuth) > 80.0f)
				y += 2 * sgn(_3DBlk.Azimuth) * pTerm->CV();
			else
				x -= 7 * pTerm->CH();
		}
		if(AxS.__Z().label.tag == ROTATE_IN_3D_LABEL_TAG) {
			double ang, angx0, angx1, angy0, angy1;
			Map3D_XY_double(_3DBlk.ZAxisX, _3DBlk.ZAxisY, AxS.__Z().min, &angx0, &angy0);
			Map3D_XY_double(_3DBlk.ZAxisX, _3DBlk.ZAxisY, AxS.__Z().max, &angx1, &angy1);
			ang = atan2(angy1-angy0, angx1-angx0) / SMathConst::PiDiv180;
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

void GnuPlot::XTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid/* linetype or -2 for none */, ticmark * userlabels)
{
	double scale = tic_scale(ticlevel, pAx) * (pAx->TicIn ? 1 : -1);
	double other_end = AxS.__Y().min + AxS.__Y().max - _3DBlk.XAxisY;
	GpVertex v1, v2, v3, v4;
	// Draw full-length grid line 
	Map3D_XYZ(place, _3DBlk.XAxisY, _3DBlk.base_z, &v1);
	if(rGrid.l_type > LT_NODRAW) {
		pTerm->Layer_(TERM_LAYER_BEGIN_GRID);
		// to save mapping twice, map non-axis y 
		Map3D_XYZ(place, other_end, _3DBlk.base_z, &v3);
		Draw3DLine(pTerm, &v1, &v3, const_cast<lp_style_type *>(&rGrid)); // @badcast
		pTerm->Layer_(TERM_LAYER_END_GRID);
	}
	// Vertical grid lines (in yz plane) 
	if(AxS.grid_vertical_lines && rGrid.l_type > LT_NODRAW) {
		GpVertex v4, v5;
		double which_face = (_3DBlk.SurfaceRotX > 90.0f && _3DBlk.SurfaceRotX < 270.0f) ? _3DBlk.XAxisY : other_end;
		pTerm->Layer_(TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(place, which_face, AxS.__Z().min, &v4);
		Map3D_XYZ(place, which_face, _3DBlk.ceiling_z, &v5);
		Draw3DLine(pTerm, &v4, &v5, const_cast<lp_style_type *>(&rGrid)); // @badcast
		pTerm->Layer_(TERM_LAYER_END_GRID);
	}
	if((AxS.__X().ticmode & TICS_ON_AXIS) && !AxS.__Y().log && AxS.__Y().InRange(0.0)) {
		Map3D_XYZ(place, 0.0, _3DBlk.base_z, &v1);
	}
	// NB: secondary axis must be linked to primary 
	if(pAx->index == SECOND_X_AXIS && pAx->linked_to_primary && pAx->link_udf->at) {
		place = EvalLinkFunction(&AxS[FIRST_X_AXIS], place);
	}
	// Draw bottom tic mark 
	if((pAx->index == FIRST_X_AXIS) || (pAx->index == SECOND_X_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		v2.x = v1.x + pTerm->MulTicV(_3DBlk.TicUnit.x * scale);
		v2.y = v1.y + pTerm->MulTicV(_3DBlk.TicUnit.y * scale);
		v2.z = v1.z + pTerm->MulTicV(_3DBlk.TicUnit.z * scale);
		v2.real_z = v1.real_z;
		Draw3DLine(pTerm, &v1, &v2, &Gg.border_lp);
	}
	// Draw top tic mark 
	if((pAx->index == SECOND_X_AXIS) || (pAx->index == FIRST_X_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		if(_3DBlk.xz_projection)
			Map3D_XYZ(place, other_end, AxS.__Z().max, &v3);
		else
			Map3D_XYZ(place, other_end, _3DBlk.base_z, &v3);
		v4.x = v3.x - pTerm->MulTicV(_3DBlk.TicUnit.x * scale);
		v4.y = v3.y - pTerm->MulTicV(_3DBlk.TicUnit.y * scale);
		v4.z = v3.z - pTerm->MulTicV(_3DBlk.TicUnit.z * scale);
		v4.real_z = v3.real_z;
		Draw3DLine(pTerm, &v3, &v4, &Gg.border_lp);
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
			if(fabs((place - userlabels->position) / (AxS.__X().max - AxS.__X().min)) <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "xtics");
		// allow manual justification of tick labels, but only for projections 
		if((_3DBlk.splot_map || _3DBlk.xz_projection) && pAx->manual_justify)
			just = pAx->tic_pos;
		else if(_3DBlk.TicUnit.x * _3DBlk.Scaler.x < -0.9)
			just = LEFT;
		else if(_3DBlk.TicUnit.x * _3DBlk.Scaler.x < 0.9)
			just = CENTRE;
		else
			just = RIGHT;
		if(pAx->index == SECOND_X_AXIS) {
			v4.x = v3.x + _3DBlk.TicUnit.x * pTerm->CH() * 1;
			v4.y = v3.y + _3DBlk.TicUnit.y * pTerm->CV() * 1;
			if(!pAx->TicIn) {
				v4.x += pTerm->MulTicV(_3DBlk.TicUnit.x * pAx->ticscale);
				v4.y += pTerm->MulTicV(_3DBlk.TicUnit.y * pAx->ticscale);
			}
			TERMCOORD(&v4, x2, y2);
		}
		else {
			v2.x = v1.x - _3DBlk.TicUnit.x * pTerm->CH() * 1;
			v2.y = v1.y - _3DBlk.TicUnit.y * pTerm->CV() * 1;
			if(!pAx->TicIn) {
				v2.x -= pTerm->MulTicV(_3DBlk.TicUnit.x * pAx->ticscale);
				v2.y -= pTerm->MulTicV(_3DBlk.TicUnit.y * pAx->ticscale);
			}
			TERMCOORD(&v2, x2, y2);
		}
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		angle = pAx->tic_rotate;
		if(!(_3DBlk.splot_map && angle && pTerm->SetTextAngle_(angle)))
			angle = 0;
		IgnoreEnhanced(!pAx->ticdef.enhanced);
		WriteMultiline(pTerm, x2+offsetx, y2+offsety, text, (JUSTIFY)just, JUST_TOP, angle, pAx->ticdef.font);
		IgnoreEnhanced(false);
		pTerm->SetTextAngle_(0);
		TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
}

void GnuPlot::YTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels)
{
	double scale = tic_scale(ticlevel, pAx) * (pAx->TicIn ? 1 : -1);
	double other_end = AxS.__X().min + AxS.__X().max - _3DBlk.YAxisX;
	GpVertex v1, v2, v3, v4;
	// Draw full-length grid line 
	Map3D_XYZ(_3DBlk.YAxisX, place, _3DBlk.base_z, &v1);
	if(rGrid.l_type > LT_NODRAW) {
		pTerm->Layer_(TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(other_end, place, _3DBlk.base_z, &v3);
		Draw3DLine(pTerm, &v1, &v3, const_cast<lp_style_type *>(&rGrid)); // @badcast
		pTerm->Layer_(TERM_LAYER_END_GRID);
	}
	// Vertical grid lines (in xz plane) 
	if(AxS.grid_vertical_lines && rGrid.l_type > LT_NODRAW) {
		GpVertex v4, v5;
		double which_face = (_3DBlk.SurfaceRotX > 90.0f && _3DBlk.SurfaceRotX < 270.0f) ? _3DBlk.YAxisX : other_end;
		pTerm->Layer_(TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(which_face, place, AxS.__Z().min, &v4);
		Map3D_XYZ(which_face, place, _3DBlk.ceiling_z, &v5);
		Draw3DLine(pTerm, &v4, &v5, const_cast<lp_style_type *>(&rGrid)); // @badcast
		pTerm->Layer_(TERM_LAYER_END_GRID);
	}
	if(AxS.__Y().ticmode & TICS_ON_AXIS && !AxS.__X().log && AxS.__X().InRange(0.0)) {
		Map3D_XYZ(0.0, place, _3DBlk.base_z, &v1);
	}
	// NB: secondary axis must be linked to primary 
	if(pAx->index == SECOND_Y_AXIS && pAx->linked_to_primary && pAx->link_udf->at) {
		place = EvalLinkFunction(&AxS[FIRST_Y_AXIS], place);
	}
	// Draw left tic mark 
	if((pAx->index == FIRST_Y_AXIS) || (pAx->index == SECOND_Y_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		v2.x = v1.x + pTerm->MulTicH(_3DBlk.TicUnit.x * scale);
		v2.y = v1.y + pTerm->MulTicH(_3DBlk.TicUnit.y * scale);
		v2.z = v1.z + pTerm->MulTicH(_3DBlk.TicUnit.z * scale);
		v2.real_z = v1.real_z;
		Draw3DLine(pTerm, &v1, &v2, &Gg.border_lp);
	}
	// Draw right tic mark 
	if((pAx->index == SECOND_Y_AXIS) || (pAx->index == FIRST_Y_AXIS && (pAx->ticmode & TICS_MIRROR))) {
		if(_3DBlk.yz_projection)
			Map3D_XYZ(other_end, place, AxS.__Z().min, &v3);
		else
			Map3D_XYZ(other_end, place, _3DBlk.base_z, &v3);
		v4.x = v3.x - pTerm->MulTicH(_3DBlk.TicUnit.x * scale);
		v4.y = v3.y - pTerm->MulTicH(_3DBlk.TicUnit.y * scale);
		v4.z = v3.z - pTerm->MulTicH(_3DBlk.TicUnit.z * scale);
		v4.real_z = v3.real_z;
		Draw3DLine(pTerm, &v3, &v4, &Gg.border_lp);
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
			if(fabs((place - userlabels->position) / AxS.__Y().GetRange()) <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "ytics");
		// allow manual justification of tick labels, but only for projections 
		if((_3DBlk.splot_map || _3DBlk.yz_projection) && pAx->manual_justify)
			just = pAx->tic_pos;
		else if(_3DBlk.TicUnit.x * _3DBlk.Scaler.x < -0.9)
			just = (pAx->index == FIRST_Y_AXIS) ? LEFT : RIGHT;
		else if(_3DBlk.TicUnit.x * _3DBlk.Scaler.x < 0.9)
			just = CENTRE;
		else
			just = (pAx->index == FIRST_Y_AXIS) ? RIGHT : LEFT;
		if(pAx->index == SECOND_Y_AXIS) {
			v4.x = v3.x + _3DBlk.TicUnit.x * pTerm->CH() * 1;
			v4.y = v3.y + _3DBlk.TicUnit.y * pTerm->CV() * 1;
			if(!pAx->TicIn) {
				v4.x += pTerm->MulTicH(_3DBlk.TicUnit.x * pAx->ticscale);
				v4.y += pTerm->MulTicV(_3DBlk.TicUnit.y * pAx->ticscale);
			}
			TERMCOORD(&v4, x2, y2);
		}
		else {
			v2.x = v1.x - _3DBlk.TicUnit.x * pTerm->CH() * 1;
			v2.y = v1.y - _3DBlk.TicUnit.y * pTerm->CV() * 1;
			if(!pAx->TicIn) {
				v2.x -= pTerm->MulTicH(_3DBlk.TicUnit.x * pAx->ticscale);
				v2.y -= pTerm->MulTicV(_3DBlk.TicUnit.y * pAx->ticscale);
			}
			TERMCOORD(&v2, x2, y2);
		}
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		angle = pAx->tic_rotate;
		if(!(_3DBlk.splot_map && angle && pTerm->SetTextAngle_(angle)))
			angle = 0;
		IgnoreEnhanced(!pAx->ticdef.enhanced);
		WriteMultiline(pTerm, x2+offsetx, y2+offsety, text, (JUSTIFY)just, JUST_TOP, angle, pAx->ticdef.font);
		IgnoreEnhanced(false);
		pTerm->SetTextAngle_(0);
		TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
}

void GnuPlot::ZTickCallback(GpTermEntry * pTerm, GpAxis * pAx, double place, char * text, int ticlevel, const lp_style_type & rGrid, ticmark * userlabels)
{
	int len = static_cast<int>(pTerm->MulTicH(tic_scale(ticlevel, pAx) * (pAx->TicIn ? 1 : -1)));
	GpVertex v1, v2, v3;
	if(pAx->ticmode & TICS_ON_AXIS)
		Map3D_XYZ(0.0, 0.0, place, &v1);
	else
		Map3D_XYZ(_3DBlk.ZAxisX, _3DBlk.ZAxisY, place, &v1);
	// Needed both for grid and for azimuth ztics 
	Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, place, &v3);
	if(rGrid.l_type > LT_NODRAW) {
		pTerm->Layer_(TERM_LAYER_BEGIN_GRID);
		Map3D_XYZ(_3DBlk.Back.x, _3DBlk.Back.y, place, &v2);
		Draw3DLine(pTerm, &v1, &v2, const_cast<lp_style_type *>(&rGrid)); // @badcast
		Draw3DLine(pTerm, &v2, &v3, const_cast<lp_style_type *>(&rGrid)); // @badcast
		pTerm->Layer_(TERM_LAYER_END_GRID);
	}
	if(_3DBlk.Azimuth != 0.0f) {
		v2.x = v1.x + (v3.x - v1.x) * len / _3DBlk.xyscaler;
		v2.y = v1.y + (v3.y - v1.y) * len / _3DBlk.xyscaler;
		v2.z = v1.z + (v3.z - v1.z) * len / _3DBlk.xyscaler;
	}
	else {
		v2.x = v1.x + len / (double)_3DBlk.Scaler.x;
		v2.y = v1.y;
		v2.z = v1.z;
	}
	v2.real_z = v1.real_z;
	Draw3DLine(pTerm, &v1, &v2, &Gg.border_lp);
	if(text) {
		int x1, y1;
		int just;
		int offsetx, offsety;
		/* Skip label if we've already written a user-specified one here */
#define MINIMUM_SEPARATION 0.001
		while(userlabels) {
			if(fabs((place - userlabels->position) / AxS.__Z().GetRange()) <= MINIMUM_SEPARATION) {
				text = NULL;
				break;
			}
			userlabels = userlabels->next;
		}
#undef MINIMUM_SEPARATION
		// get offset 
		Map3DPositionR(pTerm, &pAx->ticdef.offset, &offsetx, &offsety, "ztics");
		TERMCOORD(&v1, x1, y1);
		if(fabs(_3DBlk.Azimuth) > 80.0f) {
			// Z axis is (nearly) horizontal 
			y1 += pTerm->MulTicV(sgn(_3DBlk.Azimuth) * 2);
		}
		else {
			// the normal case 
			x1 -= pTerm->MulTicH(2);
			if(!pAx->TicIn)
				x1 -= pTerm->MulTicH(pAx->ticscale);
		}
		// allow manual justification of tick labels, but only for projections 
		if((_3DBlk.xz_projection || _3DBlk.yz_projection) && pAx->manual_justify)
			just = pAx->tic_pos;
		else
			just = RIGHT;
		// User-specified different color for the tics text 
		if(pAx->ticdef.textcolor.type == TC_Z)
			pAx->ticdef.textcolor.value = place;
		if(pAx->ticdef.textcolor.type != TC_DEFAULT)
			ApplyPm3DColor(pTerm, &(pAx->ticdef.textcolor));
		IgnoreEnhanced(!pAx->ticdef.enhanced);
		WriteMultiline(pTerm, x1+offsetx, y1+offsety, text, (JUSTIFY)just, JUST_CENTRE, 0, pAx->ticdef.font);
		IgnoreEnhanced(false);
		TermApplyLpProperties(pTerm, &Gg.border_lp);
	}
	if(AxS.__Z().ticmode & TICS_MIRROR) {
		if(_3DBlk.Azimuth != 0.0f) {
			v2.x = v3.x + (v1.x - v3.x) * len / _3DBlk.xyscaler;
			v2.y = v3.y + (v1.y - v3.y) * len / _3DBlk.xyscaler;
			v2.z = v3.z + (v1.z - v3.z) * len / _3DBlk.xyscaler;
			Draw3DLine(pTerm, &v3, &v2, &Gg.border_lp);
		}
		else {
			Map3D_XYZ(_3DBlk.Right.x, _3DBlk.Right.y, place, &v1);
			v2.x = v1.x - len / (double)_3DBlk.Scaler.x;
			v2.y = v1.y;
			v2.z = v1.z;
			v2.real_z = v1.real_z;
			Draw3DLine(pTerm, &v1, &v2, &Gg.border_lp);
		}
	}
}

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
		    if(_3DBlk.xz_projection && flat && !_3DBlk.in_3d_polygon)
			    *zpos = AxS.__Z().min + *xpos * AxS.__Z().GetRange();
		    else if(_3DBlk.yz_projection && flat && !_3DBlk.in_3d_polygon)
			    // Why is the direction inverted? 
			    *zpos = AxS.__Z().max + *xpos * (AxS.__Z().min - AxS.__Z().max);
		    else
			    *xpos = AxS.__X().min + *xpos * AxS.__X().GetRange();
		    plot_coords = TRUE;
		    break;
		case screen:
		    *xpos = *xpos * (pTerm->MaxX -1) + 0.5;
		    screen_coords = TRUE;
		    break;
		case character:
		    *xpos = *xpos * pTerm->CH_C() + 0.5;
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
		    if(_3DBlk.xz_projection && flat && !_3DBlk.in_3d_polygon)
			    *xpos = AxS.__X().min + *ypos * AxS.__X().GetRange();
		    else if(_3DBlk.splot_map)
			    *ypos = AxS.__Y().max - *ypos * AxS.__Y().GetRange();
		    else
			    *ypos = AxS.__Y().min + *ypos * AxS.__Y().GetRange();
		    plot_coords = TRUE;
		    break;
		case screen:
		    *ypos = *ypos * (pTerm->MaxY -1) + 0.5;
		    screen_coords = TRUE;
		    break;
		case character:
		    *ypos = *ypos * pTerm->CV_C() + 0.5;
		    char_coords = TRUE;
		    break;
		case polar_axes:
		    break;
	}
	switch(pPos->scalez) {
		case first_axes:
		case second_axes:
		case polar_axes:
		    if(_3DBlk.splot_map)
			    *zpos = 1.0; // Avoid failure if z=0 with logscale z 
		    else
			    *zpos = AxisLogValueChecked(FIRST_Z_AXIS, *zpos, what);
		    plot_coords = TRUE;
		    break;
		case graph:
		    if((_3DBlk.xz_projection || _3DBlk.yz_projection) && flat && !_3DBlk.in_3d_polygon)
			    ; // already received "x" fraction 
		    else
			    *zpos = AxS.__Z().min + *zpos * AxS.__Z().GetRange();
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
	double zpos = _3DBlk.splot_map ? AxS.__Z().min : pPos->z;
	// startpoint in graph coordinates 
	if(Map3DGetPosition(pTerm, pPos, what, &xpos, &ypos, &zpos) == 0) {
		int xoriginlocal;
		int yoriginlocal;
		Map3D_XY_double(xpos, ypos, zpos, xx, yy);
		xpos = (pPos->scalex == graph) ? AxS.__X().min : 0.0;
		ypos = (pPos->scaley == graph) ? (_3DBlk.splot_map ? AxS.__Y().max : AxS.__Y().min) : 0.0;
		zpos = (pPos->scalez == graph) ? AxS.__Z().min : (_3DBlk.splot_map ? AxS.__Z().min : 0.0);
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
void GnuPlot::KeyText(GpTermEntry * pTerm, int xl, int yl, char * pText)
{
	legend_key * key = &Gg.KeyT;
	pTerm->Layer_(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(key->just == GPKEY_LEFT) {
		WriteMultiline(pTerm, xl + _3DBlk.KeyTextLeft, yl, pText, LEFT, JUST_TOP, 0, key->font);
	}
	else {
		if(pTerm->justify_text(pTerm, RIGHT)) {
			WriteMultiline(pTerm, xl + _3DBlk.KeyTextRight, yl, pText, RIGHT, JUST_TOP, 0, key->font);
		}
		else {
			int x = xl + _3DBlk.KeyTextRight - (pTerm->CH()) * EstimateStrlen(pText, NULL);
			WriteMultiline(pTerm, x, yl, pText, LEFT, JUST_TOP, 0, key->font);
		}
	}
	pTerm->Layer_(TERM_LAYER_END_KEYSAMPLE);
}

void GnuPlot::KeySampleLine(GpTermEntry * pTerm, int xl, int yl)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Clip against canvas 
	V.P_ClipArea = pTerm->CheckFlag(TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	pTerm->Layer_(TERM_LAYER_BEGIN_KEYSAMPLE);
	DrawClipLine(pTerm, xl + _3DBlk.KeySampleLeft, yl, xl + _3DBlk.KeySampleRight, yl);
	pTerm->Layer_(TERM_LAYER_END_KEYSAMPLE);
	V.P_ClipArea = clip_save;
}

void GnuPlot::KeySamplePoint(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype)
{
	BoundingBox * clip_save = V.P_ClipArea;
	// Clip against canvas 
	V.P_ClipArea = pTerm->CheckFlag(TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
	pTerm->Layer_(TERM_LAYER_BEGIN_KEYSAMPLE);
	if(!V.ClipPoint(xl + _3DBlk.KeyPointOffset, yl)) {
		if(pointtype == PT_CHARACTER && pPlot) {
			ApplyPm3DColor(pTerm, &(pPlot->labels->textcolor));
			pTerm->put_text(pTerm, xl + _3DBlk.KeyPointOffset, yl, pPlot->lp_properties.p_char);
			ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
		}
		else {
			pTerm->Pnt_(xl + _3DBlk.KeyPointOffset, yl, pointtype);
		}
	}
	pTerm->Layer_(TERM_LAYER_END_KEYSAMPLE);
	V.P_ClipArea = clip_save;
}

void GnuPlot::KeySampleFill(GpTermEntry * pTerm, int xl, int yl, GpSurfacePoints * pPlot)
{
	const fill_style_type * fs = &pPlot->fill_properties;
	int style = style_from_fill(fs);
	int x = xl + _3DBlk.KeySampleLeft;
	int y = yl - _3DBlk.KeyEntryHeight/4;
	int w = _3DBlk.KeySampleRight - _3DBlk.KeySampleLeft;
	int h = _3DBlk.KeyEntryHeight/2;
	if(pTerm->fillbox) {
		pTerm->Layer_(TERM_LAYER_BEGIN_KEYSAMPLE);
		if(pPlot->plot_style == CIRCLES) {
			DoArc(pTerm, x+w/2, yl, _3DBlk.KeyEntryHeight/4, 0.0, 360.0, style, FALSE);
			// Retrace the border if the style requests it 
			if(NeedFillBorder(pTerm, fs))
				DoArc(pTerm, x+w/2, yl, _3DBlk.KeyEntryHeight/4, 0.0, 360.0, 0, FALSE);
		}
		else if(w > 0) {
			pTerm->FillBox_(style, x, y, w, h);
			// FIXME:  what other plot styles want a border on the key sample? 
			if((pPlot->plot_style & (PLOT_STYLE_HAS_PM3DBORDER | PLOT_STYLE_HAS_FILL))) {
				if((pPlot->plot_style & PLOT_STYLE_HAS_PM3DBORDER))
					if(_Pm3D.pm3d.border.l_type != LT_NODRAW && _Pm3D.pm3d.border.l_type != LT_DEFAULT)
						TermApplyLpProperties(pTerm, &_Pm3D.pm3d.border);
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
		pTerm->Layer_(TERM_LAYER_END_KEYSAMPLE);
	}
}
//
// returns minimal and maximal values of the cb-range (or z-range if taking the
// color from the z value) of the given surface
//
static void get_surface_cbminmax(const GpSurfacePoints * pPlot, double * cbmin, double * cbmax)
{
	const bool color_from_column = pPlot->pm3d_color_from_column; // just a shortcut 
	const iso_curve * icrvs = pPlot->iso_crvs;
	*cbmin = VERYLARGE;
	*cbmax = -VERYLARGE;
	for(int curve = 0; icrvs && curve < pPlot->num_iso_read;) {
		// fprintf(stderr, "**** NEW ISOCURVE - nb of pts: %i ****\n", icrvs->p_count); 
		GpCoordinate * points = icrvs->points;
		for(int i = 0; i < icrvs->p_count; i++) {
			// fprintf(stderr, "  point i=%i => x=%4g y=%4g z=%4lg cb=%4lg\n",i, points[i].x,points[i].y,points[i].z,points[i].CRD_COLOR); 
			if(points[i].type == INRANGE) {
				// ?? if (!clip_point(x, y)) ... 
				const coordval cb = color_from_column ? points[i].CRD_COLOR : points[i].Pt.z;
				SETMIN(*cbmin, cb);
				SETMAX(*cbmax, cb);
			}
		}
		icrvs = icrvs->next;
		curve++;
	}
}
//
// Draw a gradient color line for a key (legend).
//
void GnuPlot::KeySampleLinePm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl)
{
	int steps = MIN(24, abs(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft));
	// don't multiply by key->swidth --- could be >> palette.maxcolors 
	int x_to = xl + _3DBlk.KeySampleRight;
	double step = ((double)(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft)) / steps;
	int i = 1, x1 = xl + _3DBlk.KeySampleLeft, x2;
	double gray, gray_from, gray_to, gray_step;
	int colortype = pPlot->lp_properties.pm3d_color.type;
	// If pPlot uses a constant color, set it here and then let simpler routine take over 
	if((colortype == TC_RGB && pPlot->lp_properties.pm3d_color.value >= 0.0) || (colortype == TC_LT) || (colortype == TC_LINESTYLE)) {
		lp_style_type lptmp = pPlot->lp_properties;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			LpUseProperties(pTerm, &lptmp, (int)(pPlot->iso_crvs->points[0].CRD_COLOR));
		ApplyPm3DColor(pTerm, &lptmp.pm3d_color);
		KeySampleLine(pTerm, xl, yl);
	}
	else {
		// color gradient only over the cb-values of the surface, if smaller than the cb-axis range (the latter are gray values [0:1]) 
		double cbmin, cbmax;
		get_surface_cbminmax(pPlot, &cbmin, &cbmax);
		if(cbmin <= cbmax) { // splot 1/0, for example 
			cbmin = MAX(cbmin, AxS.__CB().min);
			cbmax = MIN(cbmax, AxS.__CB().max);
			gray_from = Cb2Gray(cbmin);
			gray_to = Cb2Gray(cbmax);
			gray_step = (gray_to - gray_from)/steps;
			ClipMove(x1, yl);
			x2 = x1;
			while(i <= steps) {
				// if (i>1) set_color( i==steps ? 1 : (i-0.5)/steps ); ... range [0:1] 
				gray = (i==steps) ? gray_to : gray_from+i*gray_step;
				set_color(pTerm, gray);
				ClipMove(x2, yl);
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
void GnuPlot::KeySamplePointPm3D(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int xl, int yl, int pointtype)
{
	BoundingBox * clip_save = V.P_ClipArea;
	int x_to = xl + _3DBlk.KeySampleRight;
	int i = 0, x1 = xl + _3DBlk.KeySampleLeft, x2;
	double gray, gray_from, gray_to, gray_step;
	int colortype = pPlot->lp_properties.pm3d_color.type;
	// rule for number of steps: 3*char_width*pointsize or char_width for dots, but at least 3 points 
	double step = pTerm->CH() * (pointtype == -1 ? 1 : 3*(1+(Gg.PointSize-1)/2));
	int steps = (int)(((double)(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft)) / step + 0.5);
	SETMAX(steps, 2);
	step = ((double)(_3DBlk.KeySampleRight - _3DBlk.KeySampleLeft)) / steps;
	// If pPlot uses a constant color, set it here and then let simpler routine take over 
	if((colortype == TC_RGB && pPlot->lp_properties.pm3d_color.value >= 0.0) || oneof2(colortype, TC_LT, TC_LINESTYLE)) {
		lp_style_type lptmp = pPlot->lp_properties;
		if(pPlot->lp_properties.l_type == LT_COLORFROMCOLUMN)
			LpUseProperties(pTerm, &lptmp, (int)(pPlot->iso_crvs->points[0].CRD_COLOR));
		ApplyPm3DColor(pTerm, &lptmp.pm3d_color);
		KeySamplePoint(pTerm, pPlot, xl, yl, pointtype);
	}
	else {
		// color gradient only over the cb-values of the surface, if smaller than the
		// cb-axis range (the latter are gray values [0:1]) 
		double cbmin, cbmax;
		get_surface_cbminmax(pPlot, &cbmin, &cbmax);
		if(cbmin <= cbmax) { // splot 1/0, for example 
			cbmin = MAX(cbmin, AxS.__CB().min);
			cbmax = MIN(cbmax, AxS.__CB().max);
			gray_from = Cb2Gray(cbmin);
			gray_to = Cb2Gray(cbmax);
			gray_step = (gray_to - gray_from)/steps;
			// Clip to canvas 
			V.P_ClipArea = pTerm->CheckFlag(TERM_CAN_CLIP) ? NULL : &V.BbCanvas;
			while(i <= steps) {
				// if (i>0) set_color( i==steps ? gray_to : (i-0.5)/steps ); ... range [0:1] 
				gray = (i==steps) ? gray_to : gray_from+i*gray_step;
				set_color(pTerm, gray);
				x2 = i==0 ? x1 : (i==steps ? x_to : x1 + (int)(i*step+0.5));
				// x2 += _3DBlk.KeyPointOffset; ... that's if there is only 1 point 
				if(!V.ClipPoint(x2, yl))
					pTerm->Pnt_(x2, yl, pointtype);
				i++;
			}
			V.P_ClipArea = clip_save;
		}
	}
}
// 
// plot_vectors:
// Plot the curves in VECTORS style
// 
void GnuPlot::Plot3DVectors(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	double x1, y1, x2, y2;
	GpCoordinate * tails = pPlot->iso_crvs->points;
	GpCoordinate * heads = pPlot->iso_crvs->next->points;
	// Only necessary once, unless variable arrow style 
	arrow_style_type ap = pPlot->arrow_properties;
	TermApplyLpProperties(pTerm, &ap.lp_properties);
	ApplyHeadProperties(pTerm, &ap);
	for(int i = 0; i < pPlot->iso_crvs->p_count; i++) {
		if(heads[i].type != UNDEFINED && tails[i].type != UNDEFINED) {
			// variable arrow style read from extra data column 
			if(pPlot->arrow_properties.tag == AS_VARIABLE) {
				int as = static_cast<int>(heads[i].CRD_COLOR);
				ArrowUseProperties(&ap, as);
				TermApplyLpProperties(pTerm, &ap.lp_properties);
				ApplyHeadProperties(pTerm, &ap);
			}
			else {
				Check3DForVariableColor(pTerm, pPlot, &heads[i]);
			}
			// The normal case: both ends in range 
			if(heads[i].type == INRANGE && tails[i].type == INRANGE) {
				Map3D_XY_double(tails[i].Pt, &x1, &y1);
				Map3D_XY_double(heads[i].Pt, &x2, &y2);
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
				//double clip_x, clip_y, clip_z;
				SPoint3R _clip;
				Edge3DIntersect(&heads[i], &tails[i], _clip);
				if(tails[i].type == INRANGE) {
					Map3D_XY_double(tails[i].Pt, &x1, &y1);
					Map3D_XY_double(_clip, &x2, &y2);
				}
				else {
					Map3D_XY_double(_clip, &x1, &y1);
					Map3D_XY_double(heads[i].Pt, &x2, &y2);
				}
				DrawClipArrow(pTerm, x1, y1, x2, y2, ap.head);
			}
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
void GnuPlot::Plot3DZErrorFill(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	iso_curve * curve = pPlot->iso_crvs;
	int i1, i2; // index leading and trailing coord of current quadrangle 
	int count = 0;
	gpdPoint corner[4];
	// Find leading edge of first quadrangle 
	for(i1 = 0; i1 < curve->p_count; i1++) {
		if(curve->points[i1].type == INRANGE)
			break;
	}
	for(i2 = i1+1; i2 < curve->p_count; i2++) {
		if(curve->points[i2].type == INRANGE) {
			count++; // Found one 
			corner[0] = corner[1] = curve->points[i1].Pt.GetXY();
			corner[0].z = curve->points[i1].CRD_ZLOW;
			corner[1].z = curve->points[i1].CRD_ZHIGH;
			corner[2] = corner[3] = curve->points[i2].Pt.GetXY();
			corner[3].z = curve->points[i2].CRD_ZLOW;
			corner[2].z = curve->points[i2].CRD_ZHIGH;
			Pm3DAddQuadrangle(pTerm, pPlot, corner);
			i1 = i2;
		}
	}
	if(count == 0)
		IntError(NO_CARET, "all points out of range");
	// Default is to write out each zerror plot as we come to it     
	// (most recent plot occludes all previous plots). To get proper 
	// sorting, use "set pm3d depthorder".                           
	if(_Pm3D.pm3d.direction != PM3D_DEPTH)
		Pm3DDepthQueueFlush(pTerm);
}
// 
// 3D version of plot with boxes.
// By default only a flat rectangle is drawn.  "set boxdepth <depth>"
// changes this to draw real boxes (4 sides + top).
// The boxes are drawn as pm3d rectangles. This means that depth-cueing
// must be done with "set pm3d depth base" rather than with "set hidden3d".
// 
void GnuPlot::Plot3DBoxes(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	fill_style_type save_fillstyle;
	const iso_curve * icrvs = pPlot->iso_crvs;
	gpdPoint corner[4];
	// This initialization is normally done via pm3d_plot()
	// but 3D boxes are drawn in a parallel code path.
	if(_Pm3D.pm3d_shade.strength > 0)
		Pm3DInitLightingModel();
	// FIXME: fillstyle and border color always come from "set style fill" 
	_Pm3D.pm3d.border = pPlot->lp_properties;
	_Pm3D.pm3d.border.pm3d_color = Gg.default_fillstyle.border_color;
	while(icrvs) {
		const GpCoordinate * points = icrvs->points;
		for(int i = 0; i < icrvs->p_count; i++) {
			if(points[i].type != UNDEFINED) {
				double dxh = points[i].xhigh; // rectangle extent along X axis 
				double dxl = points[i].xlow;
				double dyl = points[i].Pt.y; // rectangle extent along Y axis 
				double dyh = points[i].Pt.y;
				double dz  = points[i].Pt.z; // box height
				const GpAxis & r_ax_x = AxS.__X();
				const GpAxis & r_ax_y = AxS.__Y();
				// Box is out of range on y 
				if((dyl > r_ax_y.min && dyl > r_ax_y.max) || (dyl < r_ax_y.min && dyl < r_ax_y.max))
					continue;
				if(_Plt.boxdepth != 0.0) {
					if(r_ax_y.log) {
						const double depth = (_Plt.boxdepth < 0.0) ? (V.BoxWidth * _3DBlk.Scaler.y/_3DBlk.Scaler.x) : _Plt.boxdepth;
						dyl *= pow(r_ax_y.base, -depth/2.0);
						dyh *= pow(r_ax_y.base, depth/2.0);
					}
					else {
						const double depth = (_Plt.boxdepth < 0.0) ? (V.BoxWidth * r_ax_y.GetRange() / r_ax_x.GetRange()) : _Plt.boxdepth;
						dyl -= depth / 2.0;
						dyh += depth / 2.0;
					}
					dyl = r_ax_y.ClipToRange(dyl);
					dyh = r_ax_y.ClipToRange(dyh);
				}
				// clip to border 
				dxl = r_ax_x.ClipToRange(dxl);
				dxh = r_ax_x.ClipToRange(dxh);
				// Entire box is out of range on x 
				if(dxl == dxh && (dxl == r_ax_x.min || dxl == r_ax_x.max))
					continue;
				double zbase = AxS.__Z().ClipToRange(0.0); // box base 
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
				if(_Plt.boxdepth == 0.0)
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
			}
		}
		icrvs = icrvs->next;
	}
	// FIXME The only way to get the pm3d flush code to see our fill 
	// style is to temporarily copy it to the global fillstyle.      
	save_fillstyle = Gg.default_fillstyle;
	Gg.default_fillstyle = pPlot->fill_properties;
	// By default we write out each set of boxes as it is seen.  
	// The other option is to let them accummulate and then sort 
	// them together with all other pm3d elements to draw later. 
	if(_Pm3D.pm3d.direction != PM3D_DEPTH) {
		_Pm3D.pm3d.base_sort = true;
		Pm3DDepthQueueFlush(pTerm);
		_Pm3D.pm3d.base_sort = false;
	}
	Gg.default_fillstyle = save_fillstyle; // Restore global fillstyle 
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
	if(_Pm3D.pm3d_shade.strength > 0)
		Pm3DInitLightingModel();
	style = style_from_fill(&pPlot->fill_properties);
	// Most polygons are small 
	quadmax = 8;
	quad = (gpdPoint *)SAlloc::R(quad, quadmax * sizeof(gpdPoint));
	for(icrvs = pPlot->iso_crvs; icrvs; icrvs = icrvs->next) {
		// Allow for very large polygons (e.g. cartographic outlines) 
		const int npoints = icrvs->p_count;
		if(npoints > quadmax) {
			quadmax = npoints;
			quad = (gpdPoint *)SAlloc::R(quad, quadmax * sizeof(gpdPoint));
		}
		// Copy the vertex coordinates into a pm3d quadrangle 
		for(nv = 0, points = icrvs->points; nv < npoints; nv++) {
			quad[nv] = points[nv].Pt;
		}
		// Treat triangle as a degenerate quadrangle 
		if(nv == 3) {
			quad[3] = points[0].Pt;
		}
		// Ignore lines and points 
		if(nv < 3)
			continue;
		// Coloring piggybacks on options for isosurface 
		quad[0].c = (pPlot->pm3d_color_from_column && !isnan(points[0].CRD_COLOR)) ? points[0].CRD_COLOR : pPlot->fill_properties.border_color.lt;
		quad[1].c = style;
		Pm3DAddPolygon(pTerm, pPlot, quad, nv);
	}
	// Default is to write out each polygon as we come to it. 
	// To get proper sorting, use "set pm3d depthorder".      
	if(_Pm3D.pm3d.direction != PM3D_DEPTH)
		Pm3DDepthQueueFlush(pTerm);
	// Clean up 
	ZFREE(quad);
	quadmax = 0;
}

void GnuPlot::Check3DForVariableColor(GpTermEntry * pTerm, GpSurfacePoints * pPlot, const GpCoordinate * pPoint)
{
	int colortype = pPlot->lp_properties.pm3d_color.type;
	switch(colortype) {
		case TC_RGB:
		    if(pPlot->pm3d_color_from_column && pPlot->lp_properties.pm3d_color.value < 0.0)
			    SetRgbColorVar(pTerm, (uint)pPoint->CRD_COLOR);
		    break;
		case TC_Z:
		case TC_DEFAULT: // pm3d mode assumes this is default 
		    set_color(pTerm, Cb2Gray(pPlot->pm3d_color_from_column ? pPoint->CRD_COLOR : pPoint->Pt.z));
		    break;
		case TC_LINESTYLE: // color from linestyle in data column 
		    pPlot->lp_properties.pm3d_color.lt = (int)(pPoint->CRD_COLOR);
		    ApplyPm3DColor(pTerm, &(pPlot->lp_properties.pm3d_color));
		    break;
		case TC_COLORMAP:
		    if(pPlot->lp_properties.P_Colormap) {
			    double gray = Map2Gray(pPoint->CRD_COLOR, pPlot->lp_properties.P_Colormap);
			    SetRgbColorVar(pTerm, rgb_from_colormap(gray, pPlot->lp_properties.P_Colormap) );
		    }
		    break;
		default:
		    // The other cases were taken care of already 
		    break;
	}
}

//void do_3dkey_layout(GpTermEntry * pTerm, legend_key * key, int * xinkey, int * yinkey)
void GnuPlot::Do3DKeyLayout(GpTermEntry * pTerm, legend_key * pKey, int * xinkey, int * yinkey)
{
	int key_height, key_width;
	// NOTE: All of these had better not change after being calculated here! 
	if(pKey->reverse) {
		_3DBlk.KeySampleLeft = -_3DBlk.KeySampleWidth;
		_3DBlk.KeySampleRight = 0;
		_3DBlk.KeyTextLeft  = pTerm->CH();
		_3DBlk.KeyTextRight = pTerm->CH() * (_3DBlk.MaxPTitlLen + 1);
		_3DBlk.KeySizeRight = static_cast<int>(pTerm->CH() * (_3DBlk.MaxPTitlLen + 2 + pKey->width_fix));
		_3DBlk.KeySizeLeft  = pTerm->CH() + _3DBlk.KeySampleWidth;
	}
	else {
		_3DBlk.KeySampleLeft = 0;
		_3DBlk.KeySampleRight = _3DBlk.KeySampleWidth;
		_3DBlk.KeyTextLeft = -(int)(pTerm->CH() * (_3DBlk.MaxPTitlLen + 1));
		_3DBlk.KeyTextRight = -(int)pTerm->CH();
		_3DBlk.KeySizeLeft  = static_cast<int>(pTerm->CH() * (_3DBlk.MaxPTitlLen + 2 + pKey->width_fix));
		_3DBlk.KeySizeRight = pTerm->CH() + _3DBlk.KeySampleWidth;
	}
	_3DBlk.KeyPointOffset = (_3DBlk.KeySampleLeft + _3DBlk.KeySampleRight) / 2;
	// Key title width and height, adjusted for font size and markup 
	_3DBlk.KeyTitleExtra = 0;
	_3DBlk.KeyTitleHeight = 0;
	if(pKey->title.text) {
		double est_height;
		if(pKey->title.font)
			pTerm->set_font(pTerm, pKey->title.font);
		EstimateStrlen(pKey->title.text, &est_height);
		_3DBlk.KeyTitleHeight = static_cast<int>(est_height * pTerm->CV());
		if(pKey->title.font)
			pTerm->set_font(pTerm, "");
		// Allow a little extra clearance for markup 
		if(pTerm->CheckFlag(TERM_ENHANCED_TEXT) && (strchr(pKey->title.text, '^') || strchr(pKey->title.text, '_')))
			_3DBlk.KeyTitleExtra = pTerm->CV()/2;
	}
	key_width = _3DBlk.KeyColWth * (_3DBlk.KeyCols - 1) + _3DBlk.KeySizeRight + _3DBlk.KeySizeLeft;
	key_height = static_cast<int>(_3DBlk.KeyTitleHeight + _3DBlk.KeyTitleExtra + _3DBlk.KeyEntryHeight * _3DBlk.KeyRows + pKey->height_fix * pTerm->CV());
	// Make room for extra long title 
	SETMAX(key_width, _3DBlk.KeyTitleWidth);
	// Now that we know the size of the key, we can position it as requested 
	if(pKey->region == GPKEY_USER_PLACEMENT) {
		int corner_x, corner_y;
		Map3DPosition(pTerm, &pKey->user_pos, &corner_x, &corner_y, "key");
		if(pKey->hpos == CENTRE)
			pKey->bounds.xleft = corner_x - key_width / 2;
		else if(pKey->hpos == RIGHT)
			pKey->bounds.xleft = corner_x - key_width;
		else
			pKey->bounds.xleft = corner_x;
		pKey->bounds.xright = pKey->bounds.xleft + key_width;
		pKey->bounds.ytop = corner_y;
		pKey->bounds.ybot = corner_y - key_height;
		*xinkey = pKey->bounds.xleft + _3DBlk.KeySizeLeft;
		*yinkey = pKey->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
	}
	else {
		const BoundingBox * p_bounds = (pKey->fixed && !_3DBlk.splot_map) ? &V.BbPage : &V.BbPlot;
		if(pKey->region != GPKEY_AUTO_INTERIOR_LRTBC && pKey->margin == GPKEY_BMARGIN) {
			if(_3DBlk.PTitlCnt > 0) {
				// we divide into columns, then centre in column by considering
				// ratio of key_left_size to key_right_size
				// key_size_left / (key_size_left+_3DBlk.KeySizeRight) * (bounds->xright-bounds->xleft)/_3DBlk.KeyCols
				// do one integer division to maximise accuracy (hope we dont overflow!)
				*xinkey = p_bounds->xleft + ((p_bounds->xright - p_bounds->xleft) * _3DBlk.KeySizeLeft) / (_3DBlk.KeyCols * (_3DBlk.KeySizeLeft + _3DBlk.KeySizeRight));
				pKey->bounds.xleft = *xinkey - _3DBlk.KeySizeLeft;
				pKey->bounds.xright = pKey->bounds.xleft + key_width;
				pKey->bounds.ytop = p_bounds->ybot;
				pKey->bounds.ybot = p_bounds->ybot - key_height;
				*yinkey = pKey->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
			}
		}
		else {
			if(pKey->vpos == JUST_TOP) {
				pKey->bounds.ytop = p_bounds->ytop - pTerm->TicV;
				pKey->bounds.ybot = pKey->bounds.ytop - key_height;
				*yinkey = pKey->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
			}
			else {
				pKey->bounds.ybot = p_bounds->ybot + pTerm->TicV;
				pKey->bounds.ytop = pKey->bounds.ybot + key_height;
				*yinkey = pKey->bounds.ytop - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
			}
			if(pKey->region != GPKEY_AUTO_INTERIOR_LRTBC && pKey->margin == GPKEY_RMARGIN) {
				// keys outside plot border (right) 
				pKey->bounds.xleft  = p_bounds->xright + pTerm->TicH;
				pKey->bounds.xright = pKey->bounds.xleft + key_width;
				*xinkey = pKey->bounds.xleft + _3DBlk.KeySizeLeft;
			}
			else if(pKey->region != GPKEY_AUTO_INTERIOR_LRTBC && pKey->margin == GPKEY_LMARGIN) {
				// keys outside plot border (left) 
				pKey->bounds.xright = p_bounds->xleft - pTerm->TicH;
				pKey->bounds.xleft = pKey->bounds.xright - key_width;
				*xinkey = pKey->bounds.xleft + _3DBlk.KeySizeLeft;
			}
			else if(pKey->hpos == LEFT) {
				pKey->bounds.xleft = p_bounds->xleft + pTerm->TicH;
				pKey->bounds.xright = pKey->bounds.xleft + key_width;
				*xinkey = pKey->bounds.xleft + _3DBlk.KeySizeLeft;
			}
			else {
				pKey->bounds.xright = p_bounds->xright - pTerm->TicH;
				pKey->bounds.xleft = pKey->bounds.xright - key_width;
				*xinkey = pKey->bounds.xleft + _3DBlk.KeySizeLeft;
			}
		}
		_3DBlk.YlRef = *yinkey - _3DBlk.KeyTitleHeight - _3DBlk.KeyTitleExtra;
	}
	*yinkey -= (pKey->height_fix * pTerm->CV()) / 2; // Center the key entries vertically, allowing for requested extra space 
}

//void splot_map_activate()
void GnuPlot::SPlotMapActivate()
{
	if(!_3DBlk.SPlotMapActive) {
		_3DBlk.SPlotMapActive = 1;
		// save current values 
		_3DBlk.SPlotMapSurfaceRotX = _3DBlk.SurfaceRotX;
		_3DBlk.SPlotMapSurfaceRotZ = _3DBlk.SurfaceRotZ;
		_3DBlk.SPlotMapSurfaceScale = _3DBlk.SurfaceScale;
		// set new values 
		_3DBlk.SurfaceRotX = 180.0f;
		_3DBlk.SurfaceRotZ = 0.0f;
		// version 4 had constant value _3DBlk.SurfaceScale = 1.3 
		_3DBlk.SurfaceScale = 1.425f * _3DBlk.MapviewScale;
		// The Y axis runs backwards from a normal 2D plot 
		AxS[FIRST_Y_AXIS].FlipProjection();
	}
}

//void splot_map_deactivate()
void GnuPlot::SPlotMapDeactivate()
{
	if(_3DBlk.SPlotMapActive) {
		_3DBlk.SPlotMapActive = 0;
		// restore the original values 
		_3DBlk.SurfaceRotX  = _3DBlk.SPlotMapSurfaceRotX;
		_3DBlk.SurfaceRotZ  = _3DBlk.SPlotMapSurfaceRotZ;
		_3DBlk.SurfaceScale = _3DBlk.SPlotMapSurfaceScale;
		// The Y axis runs backwards from a normal 2D plot 
		AxS[FIRST_Y_AXIS].FlipProjection();
	}
}

#ifdef BOXERROR_3D
// 
// 3D version of plot with boxerrorbars
// The only intended use for this is in xz projection, where it generates a
// box + errorbar plot oriented horizontally rather than vertically.
// 
void GnuPlot::Plot3DBoxErrorBars(GpTermEntry * pTerm, GpSurfacePoints * pPlot)
{
	double dx, dxl, dxh; /* rectangle extent along X axis (vertical) */
	double dz, dzl, dzh; /* rectangle extent along Z axis (horizontal) */
	double dy; /* always 0 */
	int x0, y0, x1, y1; /* terminal coordinates */
	const int style = style_from_fill(&pPlot->fill_properties);
	const int colortype = pPlot->fill_properties.border_color.type;
	if(!_3DBlk.xz_projection) {
		IntWarn(NO_CARET, "splot 'with boxerrorbars' only works in xz projection");
	}
	else {
		// We make two passes through the data
		// 1st pass: draw the boxes
		// 2nd pass: draw the errorbars
		for(int pass = 1; pass <= 2; pass++) {
			iso_curve * icrvs = pPlot->iso_crvs;
			if(pass == 1) {
				if(colortype == TC_RGB)
					SetRgbColorConst(pTerm, pPlot->lp_properties.pm3d_color.lt);
			}
			if(pass == 2) {
				// Errorbar line style from "set bars" 
				if(Gr.BarLp.flags & LP_ERRORBAR_SET)
					TermApplyLpProperties(pTerm, &Gr.BarLp);
				else {
					TermApplyLpProperties(pTerm, &pPlot->lp_properties);
					NeedFillBorder(pTerm, &pPlot->fill_properties);
				}
			}
			while(icrvs) {
				const GpCoordinate * points = icrvs->points;
				for(int i = 0; i < icrvs->p_count; i++) {
					if(points[i].type != UNDEFINED) {
						dx  = points[i].Pt.x;
						dxh = dx + V.BoxWidth/2.0;
						dxl = dx - V.BoxWidth/2.0;
						dz  = points[i].Pt.z;
						dzl = points[i].CRD_ZLOW;
						dzh = points[i].CRD_ZHIGH;
						dy  = 0.0;
						// clip to border 
						dxl = AxS.__X().ClipToRange(dxl);
						dxh = AxS.__X().ClipToRange(dxh);
						dzl = AxS.__Z().ClipToRange(dzl);
						dzh = AxS.__Z().ClipToRange(dzh);
						dz  = AxS.__Z().ClipToRange(dz);
						// Entire box is out of range 
						if(dxl == dxh && (dxl == AxS.__X().min || dxl == AxS.__X().max))
							continue;
						if(pass == 1) {
							// Variable color 
							Check3DForVariableColor(pTerm, pPlot, &points[i]);
							// Draw box 
							Map3D_XY(dxl, dy, AxS.__Z().min, &x0, &y0);
							Map3D_XY(dxh, dy, dz, &x1, &y1);
							pTerm->FillBox_(style, x0, MIN(y0, y1), (x1-x0), abs(y1-y0));
							// Draw border 
							if(NeedFillBorder(pTerm, &pPlot->fill_properties)) {
								newpath(pTerm);
								pTerm->Mov_(x0, y0);
								pTerm->Vec_(x1, y0);
								pTerm->Vec_(x1, y1);
								pTerm->Vec_(x0, y1);
								pTerm->Vec_(x0, y0);
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
							Map3D_XY(dxl, dy,  dz, &x0, &vl);
							Map3D_XY(dxh, dy,  dz, &x0, &vh);
							Map3D_XY(dx,  dy, dzl, &x0, &y0);
							Map3D_XY(dx,  dy, dzh, &x1, &y1);
							// Draw main error bar 
							pTerm->Mov_(x0, y0);
							pTerm->Vec_(x1, y1);
							// Draw the whiskers perpendicular to the main bar 
							if(Gr.BarSize >= 0.0) {
								vl = static_cast<int>(y0 + Gr.BarSize * (y0 - vl));
								vh = static_cast<int>(y0 + Gr.BarSize * (y0 - vh));
							}
							DrawClipLine(pTerm, x0, vl, x0, vh);
							DrawClipLine(pTerm, x1, vl, x1, vh);
						}
					}
				}
				icrvs = icrvs->next;
			}
		} /* Passes 1 and 2 */
		// Restore base properties before key sample is drawn 
		TermApplyLpProperties(pTerm, &pPlot->lp_properties);
	}
}

#else
void GnuPlot::Plot3DBoxErrorBars(GpTermEntry * pTerm, GpSurfacePoints * plot)
{
	IntError(NO_CARET, "this copy of gnuplot does not support 3D boxerrorbars");
}
#endif /* BOXERROR_3D */
