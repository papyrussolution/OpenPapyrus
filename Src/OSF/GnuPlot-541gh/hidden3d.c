// GNUPLOT - hidden3d.c 
// Copyright 1986 - 1993, 1998, 1999, 2004   Thomas Williams, Colin Kelley
//
/*
 * 1999 Hans-Bernhard Broeker (Broeker@physik.rwth-aachen.de)
 * Major rewrite, affecting just about everything
 */
#include <gnuplot.h>
#pragma hdrstop

/*************************/
/* Configuration section */
/*************************/

// Original HIDDEN3D_QUADTREE comment (prior "gridbox" method removed 20 years later Jan 2019)
// 
// HBB 19991204: new code started to finally implement a spatially
// ordered data structure to store the polygons in. This is meant to
// speed up the HLR process. Before, the hot spot of hidden3d was the
// loop in in_front, where by far most of the polygons are rejected by
// the first test, already. The idea is to _not_ to loop over all
// those polygons far away from the edge under consideration, in the
// first place. Instead, store the polygons in an xy grid of lists,
// so we can select a sample of these lists to test a given edge against. 
// 
// The actual configuration is stored in these variables, modifiable at runtime through 'set hidden3d' options 
//int hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
//static long hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
//static int hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
//static int hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
//static int hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;
// 
// The 'real' code begins, here.
// first: types and global variables
// 
// precision of calculations in normalized space. Coordinates closer to
// each other than an absolute difference of EPSILON are considered
// equal, by some of the routines in this module. */
#define EPSILON 1e-5

// The code used to die messily if the scale parameters got over-large.
// Prevent this from happening due to mousing by locking out the mouse response. 
bool disable_mouse_z = false;

// Some inexact operations: == , > , >=, sign() 
#define EQ(X, Y)  (fabs( (X)-(Y) ) < EPSILON)   /* X == Y */
#define GR(X, Y)  ((X) > (Y) + EPSILON)         /* X >  Y */
#define GE(X, Y)  ((X) >= (Y)-EPSILON)          /* X >= Y */
#define SIGN(X)  ( ((X)<-EPSILON) ? -1 : ((X)>EPSILON) )

//typedef edge * p_edge;
//
// One triangle of the surface mesh(es). 
//
#define POLY_NVERT 3
struct GpMeshTriangle {
	long vertex[POLY_NVERT]; /* The vertices (indices on vlist) */
	// min/max in all three directions 
	coordval xmin;
	coordval xmax;
	coordval ymin;
	coordval ymax;
	coordval zmin;
	coordval zmax;
	t_plane plane; /* the plane coefficients */
	bool frontfacing; /* is polygon facing front- or backwards? */
};

//typedef GpMeshTriangle * p_polygon;
//
// convenience #defines to make the generic vector usable as typed arrays 
//
#define vlist ((GpVertex *)_Plt.HiddenVertices.v)
#define plist ((GpMeshTriangle *)_Plt.HiddenPolygons.v)
#define elist ((GpEdge *)_Plt.HiddenEdges.v)
//
// HBB 20000716: spatially oriented hierarchical data structure to
// store polygons in. For now, it's a simple xy grid of z-sorted
// lists. A single polygon can appear in several lists, if it spans
// cell borders 
//
struct qtreelist {
	long p;    // the polygon 
	long next; // next element in this chain 
};
//typedef qtreelist * p_qtreelist;
// 
// The quadtree algorithm sorts the objects into lists indexed by x/y.
// The number of cells in x and y direction has a huge effect on run time. 
// If the granularity is 10, 24% of the CPU time for all.dem is spent in   
// the routine in_front().  If granularity is bumped to 40 this goes down  
// to 12%.  The tradeoff is increased size of the quadtree array.	   
// 
#ifndef QUADTREE_GRANULARITY
	#define QUADTREE_GRANULARITY 30
#endif
//static long quadtree[QUADTREE_GRANULARITY][QUADTREE_GRANULARITY];
//static long pfirst; // first polygon in zsorted chain
//static long efirst; // first edges in zsorted chain 
//static dynarray qtree; // the dynarray to actually store all that stuff in
//#define qlist ((qtreelist *)qtree.v)
//
// and a routine to calculate the cells' position in that array: 
//
int GnuPlot::CoordToTreeCell(coordval x) const
{
	int index = static_cast<int>(((((x) / _3DBlk.SurfaceScale) + 1.0) / 2.0) * QUADTREE_GRANULARITY);
	if(index >= QUADTREE_GRANULARITY)
		index = QUADTREE_GRANULARITY - 1;
	else if(index < 0)
		index = 0;
	return index;
}
//
// Prototypes for internal functions of this module. 
//
static GP_INLINE double eval_plane_equation(t_plane p, GpVertex * v);
static GP_INLINE double intersect_line_plane(GpVertex * v1, GpVertex * v2, t_plane p);
static double intersect_line_line(const GpVertex * v1, const GpVertex * v2, const GpVertex * w1, const GpVertex * w2);
static GP_INLINE double area2D(GpVertex * v1, GpVertex * v2, GpVertex * v3);
// 
// Set the options for hidden3d. To be called from set.c, when the
// user has begun a command with 'set hidden3d', to parse the rest of that command 
// 
void GnuPlot::SetHidden3DOptions()
{
	int tmp;
	while(!Pgm.EndOfCommand()) {
		switch(Pgm.LookupTableForCurrentToken(&set_hidden3d_tbl[0])) {
			case S_HI_DEFAULTS:
			    // reset all parameters to defaults 
			    ResetHidden3DOptions();
			    Pgm.Shift();
			    if(!Pgm.EndOfCommand())
				    IntErrorCurToken("No further options allowed after 'defaults'");
			    return;
			    break;
			case S_HI_OFFSET:
			    Pgm.Shift();
			    _Plt.hiddenBacksideLinetypeOffset = IntExpression();
			    Pgm.Rollback();
			    break;
			case S_HI_NOOFFSET: _Plt.hiddenBacksideLinetypeOffset = 0; break;
			case S_HI_TRIANGLEPATTERN:
			    Pgm.Shift();
			    _Plt.hiddenTriangleLinesdrawnPattern = IntExpression();
			    Pgm.Rollback();
			    break;
			case S_HI_UNDEFINED:
			    Pgm.Shift();
			    tmp = IntExpression();
			    if(tmp <= 0 || tmp > UNHANDLED)
				    tmp = UNHANDLED;
			    _Plt.hiddenHandleUndefinedPoints = tmp;
			    Pgm.Rollback();
			    break;
			case S_HI_NOUNDEFINED: _Plt.hiddenHandleUndefinedPoints = UNHANDLED; break;
			case S_HI_ALTDIAGONAL: _Plt.hiddenShowAlternativeDiagonal = 1; break;
			case S_HI_NOALTDIAGONAL: _Plt.hiddenShowAlternativeDiagonal = 0; break;
			case S_HI_BENTOVER: _Plt.hiddenHandleBentoverQuadrangles = 1; break;
			case S_HI_NOBENTOVER: _Plt.hiddenHandleBentoverQuadrangles = 0; break;
			case S_HI_BACK: _3DBlk.hidden3d_layer = LAYER_BACK; break;
			case S_HI_FRONT: _3DBlk.hidden3d_layer = LAYER_FRONT; break;
			case S_HI_INVALID: IntErrorCurToken("No such option to hidden3d (or wrong order)");
			default: break;
		}
		Pgm.Shift();
	}
}

//void show_hidden3doptions()
void GnuPlot::ShowHidden3DOptions()
{
	fprintf(stderr, "\t  Hidden3d elements will be drawn in %s of non-hidden3d elements\n", (_3DBlk.hidden3d_layer == LAYER_BACK) ? "back" : "front");
	fprintf(stderr, "\t  Back side of surfaces has linestyle offset of %d\n\t  Bit-Mask of Lines to draw in each triangle is %ld\n\t  %d: ",
	    _Plt.hiddenBacksideLinetypeOffset, _Plt.hiddenTriangleLinesdrawnPattern, _Plt.hiddenHandleUndefinedPoints);
	switch(_Plt.hiddenHandleUndefinedPoints) {
		case OUTRANGE: fputs("Outranged and undefined datapoints are omitted from the surface.\n", stderr); break;
		case UNDEFINED: fputs("Only undefined datapoints are omitted from the surface.\n", stderr); break;
		case UNHANDLED: fputs("Will not check for undefined datapoints (may cause crashes).\n", stderr); break;
		default: fputs("Value stored for undefined datapoint handling is illegal!!!\n", stderr); break;
	}
	fprintf(stderr, "\t  Will %suse other diagonal if it gives a less jaggy outline\n\t  Will %sdraw diagonal visibly if quadrangle is 'bent over'\n",
	    _Plt.hiddenShowAlternativeDiagonal ? "" : "not ", _Plt.hiddenHandleBentoverQuadrangles ? "" : "not ");
}
//
// Implements proper 'save'ing of the new hidden3d options... 
//
//void save_hidden3doptions(FILE * fp)
void GnuPlot::SaveHidden3DOptions(FILE * fp)
{
	if(!_3DBlk.hidden3d)
		fputs("unset hidden3d\n", fp);
	else {
		fprintf(fp, "set hidden3d %s offset %d trianglepattern %ld undefined %d %saltdiagonal %sbentover\n",
			_3DBlk.hidden3d_layer == LAYER_BACK ? "back" : "front", _Plt.hiddenBacksideLinetypeOffset, _Plt.hiddenTriangleLinesdrawnPattern,
			_Plt.hiddenHandleUndefinedPoints, _Plt.hiddenShowAlternativeDiagonal ? "" : "no", _Plt.hiddenHandleBentoverQuadrangles ? "" : "no");
	}
}
//
// Initialize the necessary steps for hidden line removal and
// initialize global variables. 
//
//void init_hidden_line_removal()
void GnuPlot::InitHiddenLineRemoval()
{
	// Check for some necessary conditions to be set elsewhere: 
	// HandleUndefinedPoints mechanism depends on these: 
	assert(OUTRANGE == 1);
	assert(UNDEFINED == 2);
	// Re-mapping of this value makes the test easier in the critical section 
	if(_Plt.hiddenHandleUndefinedPoints < OUTRANGE)
		_Plt.hiddenHandleUndefinedPoints = UNHANDLED;
	init_dynarray(&_Plt.HiddenVertices, sizeof(GpVertex ), 100, 100);
	init_dynarray(&_Plt.HiddenEdges, sizeof(GpEdge), 100, 100);
	init_dynarray(&_Plt.HiddenPolygons, sizeof(GpMeshTriangle), 100, 100);
	init_dynarray(&Hid3D.QTree, sizeof(qtreelist), 100, 100);
}
//
// Reset the hidden line data to a fresh start. 
//
//void reset_hidden_line_removal()
void GnuPlot::ResetHiddenLineRemoval()
{
	_Plt.HiddenVertices.end = 0;
	_Plt.HiddenEdges.end = 0;
	_Plt.HiddenPolygons.end = 0;
	Hid3D.QTree.end = 0;
}
//
// Terminates the hidden line removal process.
// Free any memory allocated by init_hidden_line_removal above. 
//
//void term_hidden_line_removal()
void GnuPlot::TermHiddenLineRemoval()
{
	free_dynarray(&_Plt.HiddenPolygons);
	free_dynarray(&_Plt.HiddenEdges);
	free_dynarray(&_Plt.HiddenVertices);
	free_dynarray(&Hid3D.QTree);
}

#if 0 /* UNUSED ! */
/* Do we see the top or bottom of the polygon, or is it 'on edge'? */
#define GET_SIDE(vlst, csign)                                            \
	do {                                                                    \
		double ctmp =                                                       \
		    vlist[vlst[0]].x * (vlist[vlst[1]].y - vlist[vlst[2]].y) +      \
		    vlist[vlst[1]].x * (vlist[vlst[2]].y - vlist[vlst[0]].y) +      \
		    vlist[vlst[2]].x * (vlist[vlst[0]].y - vlist[vlst[1]].y);       \
		csign = SIGN(ctmp);                                                \
	} while(0)
#endif /*unused*/

long GnuPlot::StoreVertex(GpCoordinate * pPoint, lp_style_type * pLpStyle, bool colorFromColumn)
{
	GpVertex * thisvert = (GpVertex *)NextFromDynArray(&_Plt.HiddenVertices);
	thisvert->lp_style = pLpStyle;
	if((int)pPoint->type >= _Plt.hiddenHandleUndefinedPoints) {
		FLAG_VERTEX_AS_UNDEFINED(*thisvert);
		return -1;
	}
	else {
		Map3D_XYZ(pPoint->Pt, thisvert);
		if(colorFromColumn) {
			thisvert->real_z = pPoint->CRD_COLOR;
			thisvert->lp_style->pm3d_color.lt = LT_COLORFROMCOLUMN;
		}
		else
			thisvert->real_z = pPoint->Pt.z;
		// Store pointer back to original point 
		// Needed to support variable pointsize or pointtype 
		thisvert->original = pPoint;
		return (thisvert - vlist);
	}
}
// 
// A part of store_edge that does the actual storing. Used by
// in_front(), as well, so I separated it out. 
// 
long GnuPlot::MakeEdge(long vnum1, long vnum2, lp_style_type * lp, int style, int next)
{
	GpEdge * thisedge = (GpEdge *)NextFromDynArray(&_Plt.HiddenEdges);
	GpVertex * v1 = vlist + vnum1;
	GpVertex * v2 = vlist + vnum2;
	thisedge->style = style;
	thisedge->lp = lp;
	thisedge->next = next;
	// ensure z ordering inside each edge 
	if(v1->z >= v2->z) {
		thisedge->v1 = vnum1;
		thisedge->v2 = vnum2;
		if(lp->PtType == PT_ARROWHEAD) 
			thisedge->style = PT_ARROWHEAD;
		if(lp->PtType == PT_BACKARROW) 
			thisedge->style = PT_BACKARROW;
	}
	else {
		thisedge->v1 = vnum2;
		thisedge->v2 = vnum1;
		if(lp->PtType == PT_ARROWHEAD) 
			thisedge->style = PT_BACKARROW;
		if(lp->PtType == PT_BACKARROW) 
			thisedge->style = PT_ARROWHEAD;
	}
	return thisedge - elist;
}
//
// store the edge from vnum1 to vnum2 into the edge list. Ensure that
// the vertex with higher z is stored in v1, to ease sorting by zmax 
//
long GnuPlot::StoreEdge(long vnum1, edge_direction direction, long crvlen, lp_style_type * lp, int style)
{
	GpVertex * v1 = vlist + vnum1;
	GpVertex * v2 = NULL; /* just in case: initialize... */
	long vnum2;
	uint drawbits = (0x1 << direction);
	switch(direction) {
		case edir_vector:
		    v2 = v1 + 1;
		    drawbits = 0;
		    break;
		case edir_west:
		    v2 = v1 - 1;
		    break;
		case edir_north:
		    v2 = v1 - crvlen;
		    break;
		case edir_NW:
		    v2 = v1 - crvlen - 1;
		    break;
		case edir_NE:
		    v2 = v1 - crvlen;
		    v1 -= 1;
		    drawbits >>= 1; /* altDiag is handled like normal NW one */
		    break;
		case edir_impulse:
		    v2 = v1 - 1;
		    drawbits = 0; /* don't care about the triangle pattern */
		    break;
		case edir_point:
		    v2 = v1;
		    drawbits = 0; /* nothing to draw, but disable check */
		    break;
	}
	vnum2 = v2 - vlist;
	if(VERTEX_IS_UNDEFINED(*v1) || VERTEX_IS_UNDEFINED(*v2)) {
		return -2;
	}
	if(drawbits && /* no bits set: 'blind' edge --> no test! */ !(_Plt.hiddenTriangleLinesdrawnPattern & drawbits))
		style = LT_NODRAW;
	return MakeEdge(vnum1, vnum2, lp, style, -1);
}
// 
// Calculate the normal equation coefficients of the plane of polygon
// 'p'. Uses is the 'signed projected area' method. Its benefit is
// that it doesn't rely on only three of the vertices of 'p', as the
// naive cross product method does. 
//
bool GnuPlot::GetPlane(GpMeshTriangle * poly, t_plane plane)
{
	int i;
	double x, y, z, s;
	bool frontfacing = true;
	// calculate the signed areas of the polygon projected onto the
	// planes x=0, y=0 and z=0, respectively. The three areas form
	// the components of the plane's normal vector: 
	GpVertex * v1 = vlist + poly->vertex[POLY_NVERT-1];
	GpVertex * v2 = vlist + poly->vertex[0];
	plane[0] = (v1->y - v2->y) * (v1->z + v2->z);
	plane[1] = (v1->z - v2->z) * (v1->x + v2->x);
	plane[2] = (v1->x - v2->x) * (v1->y + v2->y);
	for(i = 1; i < POLY_NVERT; i++) {
		v1 = v2;
		v2 = vlist + poly->vertex[i];
		plane[0] += (v1->y - v2->y) * (v1->z + v2->z);
		plane[1] += (v1->z - v2->z) * (v1->x + v2->x);
		plane[2] += (v1->x - v2->x) * (v1->y + v2->y);
	}
	// Normalize the resulting normal vector 
	s = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
	if(GE(0.0, s)) {
		// The normal vanishes, i.e. the polygon is degenerate. We build
		// another vector that is orthogonal to the line of the polygon 
		v1 = vlist + poly->vertex[0];
		for(i = 1; i < POLY_NVERT; i++) {
			v2 = vlist + poly->vertex[i];
			if(!V_EQUAL(v1, v2))
				break;
		}
		// build (x,y,z) that should be linear-independant from <v1, v2> 
		x = v1->x;
		y = v1->y;
		z = v1->z;
		if(EQ(y, v2->y))
			y += 1.0;
		else
			x += 1.0;
		// Re-do the signed area computations 
		plane[0] = v1->y * (v2->z - z) + v2->y * (z - v1->z) + y * (v1->z - v2->z);
		plane[1] = v1->z * (v2->x - x) + v2->z * (x - v1->x) + z * (v1->x - v2->x);
		plane[2] = v1->x * (v2->y - y) + v2->x * (y - v1->y) + x * (v1->y - v2->y);
		s = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
	}
	// ensure that normalized c is > 0 
	if(plane[2] < 0.0) {
		s *= -1.0;
		frontfacing = FALSE;
	}
	plane[0] /= s;
	plane[1] /= s;
	plane[2] /= s;
	// Now we have the normalized normal vector, insert one of the
	// vertices into the equation to get 'd'. For an even better result,
	// an average over all the vertices might be used 
	plane[3] = -plane[0] * v1->x - plane[1] * v1->y - plane[2] * v1->z;
	return frontfacing;
}
//
// Evaluate the plane equation represented a four-vector for the given
// vector. For points in the plane, this should result in values ==0.
// < 0 is 'away' from the polygon, > 0 is infront of it 
//
static GP_INLINE double eval_plane_equation(t_plane p, GpVertex * v)
{
	return (p[0]*v->x + p[1]*v->y + p[2]*v->z + p[3]);
}
//
// Find the intersection of a line and plane in 3d space in
// terms of parameterization u where v = v1 + u * (v2 - v1) 
//
static GP_INLINE double intersect_line_plane(GpVertex * v1, GpVertex * v2, t_plane p)
{
	double numerator = eval_plane_equation(p, v1);
	if(numerator == 0)
		return 0;
	else {
		double denominator = p[0] * (v1->x - v2->x) + p[1] * (v1->y - v2->y) + p[2] * (v1->z - v2->z);
		return (denominator==0 ? (numerator>0 ? VERYLARGE : -VERYLARGE) : numerator/denominator);
	}
}
//
// Find the intersection of two lines in 2d space in terms
// of parameterization u where v = v1 + u * (v2 - v1) 
//
static double intersect_line_line(const GpVertex * v1, const GpVertex * v2, const GpVertex * w1, const GpVertex * w2)
{
	double numerator = (w2->x - w1->x)*(v1->y - w1->y) - (w2->y - w1->y)*(v1->x - w1->x);
	if(numerator == 0.0)
		return 0;
	else {
		double denominator = (w2->y - w1->y)*(v2->x - v1->x) - (w2->x - w1->x)*(v2->y - v1->y);
		return (denominator==0.0 ? ((numerator > 0.0) ? VERYLARGE : -VERYLARGE) : numerator/denominator);
	}
}
//
// Check whether the point is covered by the plane in 3d space
//
// 0 - point not covered
// 1 - point covered and does not lie in plane
// 2 - point covered and lies in plane
//
int GnuPlot::CoverPointPoly(const GpVertex * v1, const GpVertex * v2, double u, GpMeshTriangle * poly)
{
	// Using EQ() test seemed to have no effect on results 
	if(poly->plane[2] == 0) {
		// The element is "vertical" so treat as infitesimally small for now.
		// An alternative would be to interpolate the edge closest to the
		// viewer plane.  However, there may be tests previous to this that
		// rule out this case. 
		return 0;
	}
	else {
		GpVertex * w1 = vlist + poly->vertex[0];
		GpVertex * w2 = vlist + poly->vertex[1];
		GpVertex * w3 = vlist + poly->vertex[2];
		double p_side[3]; /* Signed areas */
		GpVertex p;
		p.x = v1->x + u * (v2->x - v1->x);
		p.y = v1->y + u * (v2->y - v1->y);
		p.z = v1->z + u * (v2->z - v1->z);
		// Check if point is inside triangular element 
		p_side[0] = area2D(w1, w2, &p);
		p_side[1] = area2D(w2, w3, &p);
		p_side[2] = area2D(w3, w1, &p);
		if(0 || (GE(p_side[0], 0) && GE(p_side[1], 0) && GE(p_side[2], 0)) || (GE(0, p_side[0]) && GE(0, p_side[1]) && GE(0, p_side[2]))) {
			// Point inside closed triangle, now check z value 
			double z_plane = -(poly->plane[0]*p.x + poly->plane[1]*p.y + poly->plane[3]) / poly->plane[2];
			if(GE(z_plane, p.z)) {
				// Covered, but is it on the plane? 
				if(GE(p.z, z_plane))
					return 2;
				else
					return 1;
			}
			else
				return 0;
		}
		else
			return 0;
	}
}
// 
// Build the data structure for this polygon. The return value is the
// index of the newly generated polygon. This is memorized for access
// to polygons in the previous isoline, from the next-following one. 
// 
long GnuPlot::StorePolygon(long vnum1, polygon_direction direction, long crvlen)
{
	long int v[POLY_NVERT] = {0};
	GpVertex * v1;
	GpVertex * v2;
	GpVertex * v3;
	GpMeshTriangle * p;
	switch(direction) {
		case pdir_NE:
		    v[0] = vnum1;
		    v[2] = vnum1 - crvlen;
		    v[1] = v[2] - 1;
		    break;
		case pdir_SW:
		    /* triangle points southwest, here */
		    v[0] = vnum1;
		    v[1] = vnum1 - 1;
		    v[2] = v[1] - crvlen;
		    break;
		case pdir_SE:
		    /* alt-diagonal, case 1: southeast triangle: */
		    v[0] = vnum1;
		    v[2] = vnum1 - crvlen;
		    v[1] = vnum1 - 1;
		    break;
		case pdir_NW:
		    v[2] = vnum1 - crvlen;
		    v[0] = vnum1 - 1;
		    v[1] = v[0] - crvlen;
		    break;
	}
	v1 = vlist + v[0];
	v2 = vlist + v[1];
	v3 = vlist + v[2];
	if(VERTEX_IS_UNDEFINED(*v1) || VERTEX_IS_UNDEFINED(*v2) || VERTEX_IS_UNDEFINED(*v3))
		return (-2);
	// Check if polygon is degenerate 
	if(V_EQUAL(v1, v2) || V_EQUAL(v2, v3) || V_EQUAL(v3, v1))
		return (-2);
	// All else OK, fill in the polygon: 
	p = (GpMeshTriangle *)NextFromDynArray(&_Plt.HiddenPolygons);
	memcpy(p->vertex, v, sizeof(v));
	// Some helper macros for repeated code blocks: 
	// Gets Minimum 'var' value of polygon 'poly' into variable 'min. C is one of x, y, or z: 
#define GET_MIN(poly, var, min)                 \
	do {                                        \
		const long * v = poly->vertex;                 \
		min = vlist[*v++].var;                  \
		for(int i = 1; i< POLY_NVERT; i++, v++)    \
			SETMIN(min, vlist[*v].var);            \
		if(min < -_3DBlk.SurfaceScale) disable_mouse_z = true; \
	} while(0)
	// Gets Maximum 'var' value of polygon 'poly', as with GET_MIN 
#define GET_MAX(poly, var, max)                 \
	do {                                        \
		const long * v = poly->vertex;                 \
		max = vlist[*v++].var;                  \
		for(int i = 1; i< POLY_NVERT; i++, v++)    \
			SETMAX(max, vlist[*v].var);            \
		if(max > _3DBlk.SurfaceScale) disable_mouse_z = true; \
	} while(0)
	GET_MIN(p, x, p->xmin);
	GET_MIN(p, y, p->ymin);
	GET_MIN(p, z, p->zmin);
	GET_MAX(p, x, p->xmax);
	GET_MAX(p, y, p->ymax);
	GET_MAX(p, z, p->zmax);
#undef GET_MIN
#undef GET_MAX
	p->frontfacing = GetPlane(p, p->plane);
	return (p - plist);
}
//
// color edges, based on the orientation of polygon(s). One of the two
// edges passed in is a new one, meaning there is no other polygon
// sharing it, yet. The other, 'old' edge is common to the new polygon
// and another one, which was created earlier on. If these two polygon
// differ in their orientation (one front-, the other backsided to the
// viewer), this routine has to resolve that conflict.  Edge colours
// are changed only if the edge wasn't invisible, before 
// 
void GnuPlot::ColorEdges(long new_edge/* index of 'new', conflictless edge */,
	long old_edge/* index of 'old' edge, may conflict */, long new_poly/* index of current polygon */,
	long old_poly/* index of poly sharing old_edge */, int above/* style number for front of polygons */, int below/* style number for backside of polys */)
{
	if(new_poly > -2) {
		// new polygon was built successfully 
		if(old_poly <= -2)
			old_poly = new_poly; // old polygon doesn't exist. Use new_polygon for both: 
		int casenumber = (plist[new_poly].frontfacing ? 1 : 0) + 2 * (plist[old_poly].frontfacing ? 1 : 0);
		switch(casenumber) {
			case 0:
			    // both backfacing 
			    if(elist[new_edge].style != LT_NODRAW)
				    elist[new_edge].style = below;
			    if(elist[old_edge].style != LT_NODRAW)
				    elist[old_edge].style = below;
			    break;
			case 2:
			    if(elist[new_edge].style != LT_NODRAW)
				    elist[new_edge].style = below;
			// FALLTHROUGH 
			case 1:
			    // new front-, old one backfacing, or 
			    // new back-, old one frontfacing 
			    if(((new_edge == old_edge) && _Plt.hiddenHandleBentoverQuadrangles) /* a diagonal edge! */ || (elist[old_edge].style != LT_NODRAW)) {
				    /* conflict has occurred: two polygons meet here, with opposige
				     * sides being shown. What's to do?
				     * 1) find a vertex of one polygon outside this common
				     * edge
				     * 2) check whether it's in front of or behind the
				     * other polygon's plane
				     * 3) if in front, color the edge according to the
				     * vertex' polygon, otherwise, color like the other
				     * polygon */
				    long vnum1 = elist[old_edge].v1;
				    long vnum2 = elist[old_edge].v2;
				    GpMeshTriangle * p = plist + new_poly;
				    long pvert = -1;
				    double point_to_plane;
				    if(p->vertex[0] == vnum1) {
					    if(p->vertex[1] == vnum2) {
						    pvert = p->vertex[2];
					    }
					    else if(p->vertex[2] == vnum2) {
						    pvert = p->vertex[1];
					    }
				    }
				    else if(p->vertex[1] == vnum1) {
					    if(p->vertex[0] == vnum2) {
						    pvert = p->vertex[2];
					    }
					    else if(p->vertex[2] == vnum2) {
						    pvert = p->vertex[0];
					    }
				    }
				    else if(p->vertex[2] == vnum1) {
					    if(p->vertex[0] == vnum2) {
						    pvert = p->vertex[1];
					    }
					    else if(p->vertex[1] == vnum2) {
						    pvert = p->vertex[0];
					    }
				    }
				    assert(pvert >= 0);
				    point_to_plane = eval_plane_equation(plist[old_poly].plane, vlist + pvert);
				    if(point_to_plane > 0) {
					    // point in new_poly is in front of old_poly plane 
					    elist[old_edge].style = p->frontfacing ? above : below;
				    }
				    else {
					    elist[old_edge].style = plist[old_poly].frontfacing ? above : below;
				    }
			    }
			    break;
			case 3:
			    break; // both frontfacing: nothing to do 
		}
	}
	else {
		return; // Ooops? build_networks() must have guessed incorrectly that this polygon should exist. 
	}
}
// 
// This somewhat monstrous routine fills the vlist, elist and plist
// dynamic arrays with values from all those plots. It strives to
// respect all the topological linkage between vertices, edges and
// polygons. E.g., it has to find the correct color for each edge,
// based on the orientation of the two polygons sharing it, WRT both
// the observer and each other. */
// NEW FEATURE HBB 20000715: allow non-grid datasets too, by storing
// only vertices and 'direct' edges, but no polygons or 'cross' edges
// 
void GnuPlot::BuildNetworks(GpSurfacePoints * pPlots, int pcount)
{
	long i;
	GpSurfacePoints * this_plot;
	int  surface; /* count the surfaces (i.e. sub-plots) */
	long crv, ncrvs; /* count isolines */
	long nverts; /* count vertices */
	long max_crvlen; /* maximal length of isoline in any plot */
	long nv, ne, np; /* local poly/edge/vertex counts */
	long * north_polygons; /* stores polygons of isoline above */
	long * these_polygons; /* same, being built for use by next turn */
	long * north_edges; /* stores edges of polyline above */
	long * these_edges; /* same, being built for use by next turn */
	iso_curve * icrvs;
	int above = LT_NODRAW; // linetype for edges of front side
	int below = LT_NODRAW; // linetype for edges of back side
	lp_style_type * lp; /* pointer to line and point properties */
	// Count out the initial sizes needed for the polygon and vertex lists. 
	nv = ne = np = 0;
	max_crvlen = -1;
	for(this_plot = pPlots, surface = 0; surface < pcount; this_plot = this_plot->next_sp, surface++) {
		long crvlen;
		// Quietly skip empty plots 
		if(oneof2(this_plot->plot_type, NODATA, KEYENTRY))
			continue;
		// Allow individual plots to opt out of hidden3d calculations 
		if(this_plot->opt_out_of_hidden3d)
			continue;
		crvlen = this_plot->iso_crvs->p_count;
		// register maximal isocurve length. Only necessary for
		// grid-topology plots that will create polygons, so I can do
		// it here, already. 
		if(crvlen > max_crvlen)
			max_crvlen = crvlen;
		// count 'curves' (i.e. isolines) and vertices in this plot
		nverts = 0;
		if(this_plot->plot_type == FUNC3D) {
			ncrvs = 0;
			for(icrvs = this_plot->iso_crvs; icrvs; icrvs = icrvs->next) {
				ncrvs++;
			}
			nverts += ncrvs * crvlen;
		}
		else if(this_plot->plot_type == DATA3D) {
			ncrvs = this_plot->num_iso_read;
			if(this_plot->has_grid_topology)
				nverts += ncrvs * crvlen;
			else if(this_plot->plot_style == VECTOR)
				nverts += this_plot->iso_crvs->p_count;
			else {
				// have to check each isoline separately: 
				for(icrvs = this_plot->iso_crvs; icrvs; icrvs = icrvs->next)
					nverts += icrvs->p_count;
			}
		}
		else {
			return; // Cannot happen 
		}
		// To avoid possibly surprising error messages, several 2d-only
		// plot styles are mapped to others, that are genuinely available in 3d. 
		switch(this_plot->plot_style) {
			case PM3DSURFACE:
			case LINESPOINTS:
			case STEPS:
			case FSTEPS:
			case HISTEPS:
			case LINES:
			case SURFACEGRID:
			    nv += nverts;
			    ne += nverts - ncrvs;
			    if(this_plot->has_grid_topology) {
				    ne += 2 * nverts - ncrvs - 2 * crvlen + 1;
				    np += 2 * (ncrvs - 1) * (crvlen - 1);
			    }
			    break;
			case BOXES:
			case FILLEDCURVES:
			case IMPULSES:
			case VECTOR:
			    nv += 2 * nverts;
			    ne += nverts;
			    break;
			case IMAGE:
			case RGBIMAGE:
			case RGBA_IMAGE:
			    // Ignore these 
			    break;
			case CIRCLES:
			    this_plot->lp_properties.flags |= LP_SHOW_POINTS;
			    this_plot->lp_properties.PtType = PT_CIRCLE;
			    this_plot->lp_properties.PtSize = PTSZ_VARIABLE;
			    nv += nverts;
			    ne += nverts; // a 'phantom edge' per isolated point 
			    break;
			case DOTS:
			    this_plot->lp_properties.flags |= LP_SHOW_POINTS;
			    this_plot->lp_properties.PtType = -1;
				// @fallthrough 
			case POINTSTYLE:
			default:
			    /* treat all remaining ones like 'points' */
			    nv += nverts;
			    ne += nverts; /* a 'phantom edge' per isolated point */
			    break;
		}
	}
	// Check for no usable data at all */
	// June 2017 - increase minimum length from 1 to 2 
	if(max_crvlen <= 1)
		return;
	// allocate all the lists to the size we need: 
	ResizeDynArray(&_Plt.HiddenVertices, nv);
	ResizeDynArray(&_Plt.HiddenEdges, ne);
	ResizeDynArray(&_Plt.HiddenPolygons, np);
	// allocate the storage for polygons and edges of the isoline just
	// above the current one, to allow easy access to them from the
	// current isoline 
	north_polygons = (long *)SAlloc::M(2 * max_crvlen * sizeof(long));
	these_polygons = (long *)SAlloc::M(2 * max_crvlen * sizeof(long));
	north_edges = (long *)SAlloc::M(3 * max_crvlen * sizeof(long));
	these_edges = (long *)SAlloc::M(3 * max_crvlen * sizeof(long));
	// initialize the lists, all in one large loop. This is different
	// from the previous approach, which went over the vertices,
	// first, and only then, in new loop, built polygons 
	for(this_plot = pPlots, surface = 0; surface < pcount; this_plot = this_plot->next_sp, surface++) {
		bool color_from_column = this_plot->pm3d_color_from_column;
		long crvlen;
		lp = &(this_plot->lp_properties);
		// Quietly skip empty plots 
		if(oneof2(this_plot->plot_type, NODATA, KEYENTRY))
			continue;
		// Allow individual plots to opt out of hidden3d calculations 
		if(this_plot->opt_out_of_hidden3d)
			continue;
		crvlen = this_plot->iso_crvs->p_count;
		// We can't use the linetype passed to us, because it has been through 
		// load_linetype(), which replaced the nominal linetype with the one   
		// assigned by "set linetype ..."                                      
		above = this_plot->hidden3d_top_linetype;
		below = above + _Plt.hiddenBacksideLinetypeOffset;
		// The "nosurface" flag is interpreted by hidden3d mode to mean 
		// "don't draw this surface".  I.e. draw only the contours.	
		if(this_plot->opt_out_of_surface)
			above = below = LT_NODRAW;
		// This is a special flag indicating that the user specified an	
		// explicit surface color in the splot command.			
		if(lp->flags & LP_EXPLICIT_COLOR)
			below = above;
		// We will not actually draw PM3D surfaces here, but their      
		// edges can be used to calculate occlusion of lines, including 
		// the plot borders. (NB: the PM3D surface will _not_ be hidden 
		// by other non-PM3D surfaces.					
		if(this_plot->plot_style == PM3DSURFACE)
			above = below = LT_NODRAW;
		// calculate the point symbol type: 
		// Assumes that upstream functions have made sure this is initialized sensibly --- thou hast been warned 
		if(this_plot->plot_style == VECTOR) {
			lp->PtType = PT_ARROWHEAD;
			if(this_plot->arrow_properties.head == BACKHEAD)
				lp->PtType = PT_BACKARROW;
			if(this_plot->arrow_properties.head == NOHEAD) {
				this_plot->arrow_properties.head_length = 1;
				this_plot->arrow_properties.head_angle = 0;
			}
			// NB: It would not work to apply arrowhead properties now 
			// because hidden3d code mixes arrows from multiple plots. 
		}
		// HBB 20000715: new initialization code block for non-grid
		// structured datasets. Sufficiently different from the rest
		// to warrant separate code, I think. 
		if(!this_plot->has_grid_topology) {
			for(crv = 0, icrvs = this_plot->iso_crvs; icrvs; crv++, icrvs = icrvs->next) {
				GpCoordinate * points = icrvs->points;
				long previousvertex = -1;
				// To handle labels we must look inside a separate list 
				// rather than just walking through the points arrays.  
				if(this_plot->plot_style == LABELPOINTS) {
					long thisvertex;
					GpCoordinate labelpoint;
					lp->flags |= LP_SHOW_POINTS; // Labels can use the code for hidden points 
					labelpoint.type = INRANGE;
					for(text_label * label = this_plot->labels->next; label != NULL; label = label->next) {
						labelpoint.Pt.x = label->place.x;
						labelpoint.Pt.y = label->place.y;
						labelpoint.Pt.z = label->place.z;
						if(label->textcolor.type == TC_Z)
							labelpoint.CRD_COLOR = label->textcolor.value;
						else
							labelpoint.CRD_COLOR = label->textcolor.lt;
						thisvertex = StoreVertex(&labelpoint, lp, color_from_column);
						if(thisvertex < 0)
							continue;
						(vlist+thisvertex)->label = label;
						StoreEdge(thisvertex, edir_point, crvlen, lp, above);
					}
				}
				else {
					for(i = 0; i < icrvs->p_count; i++) {
						long thisvertex;
						long basevertex;
						int interval = this_plot->lp_properties.p_interval;
						// NULL lp means don't draw a point at this vertex 
						if(this_plot->plot_style == LINESPOINTS && interval && (i % interval))
							thisvertex = StoreVertex(points + i, NULL, color_from_column);
						else
							thisvertex = StoreVertex(points + i, lp, color_from_column);
						if(this_plot->plot_style == VECTOR) {
							StoreVertex(icrvs->next->points+i, 0, 0);
						}
						if(thisvertex < 0) {
							previousvertex = thisvertex;
							continue;
						}
						switch(this_plot->plot_style) {
							case PM3DSURFACE:
							case LINESPOINTS:
							case STEPS:
							case FSTEPS:
							case HISTEPS:
							case LINES:
							case SURFACEGRID:
							    if(previousvertex >= 0)
								    StoreEdge(thisvertex, edir_west, 0, lp, above);
							    break;
							case VECTOR:
							    StoreEdge(thisvertex, edir_vector, 0, lp, above);
							    break;
							case BOXES:
							case FILLEDCURVES:
							    /* set second vertex to the low end of zrange */
								{
									coordval remember_z = points[i].Pt.z;
									points[i].Pt.z = AxS[FIRST_Z_AXIS].Range.low;
									basevertex = StoreVertex(points + i, lp, color_from_column);
									points[i].Pt.z = remember_z;
								}
							    if(basevertex > 0)
								    StoreEdge(basevertex, edir_impulse, 0, lp, above);
							    break;
							case IMPULSES:
							    /* set second vertex to z=0 */
								{
									coordval remember_z = points[i].Pt.z;
									points[i].Pt.z = 0.0;
									basevertex = StoreVertex(points + i, lp, color_from_column);
									points[i].Pt.z = remember_z;
								}
							    if(basevertex > 0)
								    StoreEdge(basevertex, edir_impulse, 0, lp, above);
							    break;
							case IMAGE:
							case RGBIMAGE:
							case RGBA_IMAGE:
							    /* Ignore these */
							    break;
							case POINTSTYLE:
							default: /* treat all the others like 'points' */
							    StoreEdge(thisvertex, edir_point, crvlen, lp, above);
							    break;
						}
						previousvertex = thisvertex;
					}
				}
			}
			continue; // done with this plot! 
		}
		// initialize stored indices of north-of-this-isoline polygons and edges properly 
		for(i = 0; i < this_plot->iso_crvs->p_count; i++) {
			north_polygons[2 * i] = north_polygons[2 * i + 1] = north_edges[3 * i] = north_edges[3 * i + 1] = north_edges[3 * i + 2] = -3;
		}
		for(crv = 0, icrvs = this_plot->iso_crvs; icrvs; crv++, icrvs = icrvs->next) {
			GpCoordinate * points = icrvs->points;
			for(i = 0; i < icrvs->p_count; i++) {
				long basevertex;
				long e1, e2, e3;
				long pnum;
				long thisvertex = StoreVertex(points + i, lp, color_from_column);
				// Preset the pointers to the polygons and edges belonging to this isoline 
				these_polygons[2 * i] = these_polygons[2 * i + 1] = these_edges[3 * i] = these_edges[3 * i + 1] = these_edges[3 * i + 2] = -3;
				switch(this_plot->plot_style) {
					case PM3DSURFACE:
					case LINESPOINTS:
					case STEPS:
					case FSTEPS:
					case HISTEPS:
					case LINES:
					case SURFACEGRID:
					    if(i > 0) {
						    // not first point, so we might want to set up the edge(s) to the left of this vertex 
						    if(thisvertex < 0) {
							    if((crv > 0) && _Plt.hiddenShowAlternativeDiagonal) {
								    /* this vertex is invalid, but the
								     * other three might still form a
								     * valid triangle, facing northwest to
								     * do that, we'll need the 'wrong'
								     * diagonal, which goes from SW to NE:
								     * */
								    these_edges[i*3+2] = e3 = StoreEdge(_Plt.HiddenVertices.end - 1, edir_NE, crvlen, lp, above);
								    if(e3 > -2) {
									    /* don't store this polygon for
									     * later: it doesn't share edges
									     * with any others to the south or
									     * east, so there's need to */
									    pnum = StorePolygon(_Plt.HiddenVertices.end - 1, pdir_NW, crvlen);
									    /* The other two edges of this
									     * polygon need to be checked
									     * against the neighboring
									     * polygons' orientations, before
									     * being coloured */
									    ColorEdges(e3, these_edges[3*(i-1) +1], pnum, these_polygons[2*(i-1) + 1], above, below);
									    ColorEdges(e3, north_edges[3*i], pnum, north_polygons[2*i], above, below);
								    }
							    }
							    break; /* nothing else to do for invalid vertex */
						    }

						    /* Coming here means that the current vertex
						     * is valid: check the other three of this
						     * cell, by trying to set up the edges from
						     * this one to there */
						    these_edges[i*3] = e1 = StoreEdge(thisvertex, edir_west, crvlen, lp, above);
						    if(crv > 0) { /* vertices to the north exist */
							    these_edges[i*3 + 1] = e2 = StoreEdge(thisvertex, edir_north, crvlen, lp, above);
							    these_edges[i*3 + 2] = e3 = StoreEdge(thisvertex, edir_NW, crvlen, lp, above);
							    if(e3 > -2) {
								    /* diagonal edge of this cell is OK,
								     * so try to build both the polygons:
								     * */
								    if(e1 > -2) {
									    /* one pair of edges is valid: put
									     * first polygon, which points
									     * towards the southwest */
									    these_polygons[2*i] = pnum = StorePolygon(thisvertex, pdir_SW, crvlen);
									    ColorEdges(e1, these_edges[3*(i-1)+1], pnum, these_polygons[2*(i-1)+ 1], above, below);
								    }
								    if(e2 > -2) {
									    // other pair of two is fine, put the northeast polygon: 
									    these_polygons[2*i + 1] = pnum = StorePolygon(thisvertex, pdir_NE, crvlen);
									    ColorEdges(e2, north_edges[3*i], pnum, north_polygons[2*i], above, below);
								    }
								    /* In case these two new polygons
								     * differ in orientation, find good
								     * coloring of the diagonal */
								    ColorEdges(e3, e3, these_polygons[2*i], these_polygons[2*i+1], above, below);
							    } /* if e3 valid */
							    else if((e1 > -2) && (e2 > -2) && _Plt.hiddenShowAlternativeDiagonal) {
								    /* looks like all but the north-west
								     * vertex are usable, so we set up the
								     * southeast-pointing triangle, using
								     * the 'wrong' diagonal: */
								    these_edges[3*i + 2] = e3 = StoreEdge(thisvertex, edir_NE, crvlen, lp, above);
								    if(e3 > -2) {
									    // fill this polygon into *both* polygon places for this
									    // quadrangle, as this triangle coincides with both edges that
									    // will be used by later polygons
									    these_polygons[2*i] = these_polygons[2*i+1] = pnum = StorePolygon(thisvertex, pdir_SE, crvlen);
									    // This case is somewhat special: all edges are new, so there is
										// no other polygon orientation to consider 
									    if(pnum > -2) {
										    if(!plist[pnum].frontfacing)
											    elist[e1].style = elist[e2].style = elist[e3].style = below;
									    }
								    }
							    }
						    }
					    }
					    else if((crv > 0) && (thisvertex >= 0)) {
						    // We're at the west border of the grid, but
							// not on the north one: put vertical end-wall edge:
						    these_edges[3*i + 1] = StoreEdge(thisvertex, edir_north, crvlen, lp, above);
					    }
					    break;
					case BOXES:
					case FILLEDCURVES:
					case IMPULSES:
					    if(thisvertex < 0)
						    break;
					    // set second vertex to the low end of zrange 
					    {
						    coordval remember_z = points[i].Pt.z;
						    points[i].Pt.z = (this_plot->plot_style == IMPULSES) ? 0.0 : AxS[FIRST_Z_AXIS].Range.low;
						    basevertex = StoreVertex(points + i, lp, color_from_column);
						    points[i].Pt.z = remember_z;
					    }
					    if(basevertex > 0)
						    StoreEdge(basevertex, edir_impulse, 0, lp, above);
					    break;
					case POINTSTYLE:
					default: /* treat all the others like 'points' */
					    if(thisvertex < 0) /* Ignore invalid vertex */
						    break;
					    StoreEdge(thisvertex, edir_point, crvlen, lp, above);
					    break;
				}
			}
			// Swap the 'north' lists of polygons and edges with
			// 'these' ones, which have been filled in the pass
			// through this isocurve 
			{
				long * temp = north_polygons;
				north_polygons = these_polygons;
				these_polygons = temp;
				temp = north_edges;
				north_edges = these_edges;
				these_edges = temp;
			}
		} /* for(isocrv) */
	} /* for(plot) */
	SAlloc::F(these_polygons);
	SAlloc::F(north_polygons);
	SAlloc::F(these_edges);
	SAlloc::F(north_edges);
}
// 
// Sort the elist in order of growing zmax. Uses qsort on an array of
// plist indices, and then fills in the 'next' fields in struct
// polygon to store the resulting order inside the plist 
// 
/*static*/int GnuPlot::CompareEdgesByZMin(void * pCtx, SORTFUNC_ARGS p1, SORTFUNC_ARGS p2)
{
	const GnuPlot * p_this = static_cast<GnuPlot *>(pCtx);
	const GpVertex * p_v_list = static_cast<const GpVertex *>(p_this->_Plt.HiddenVertices.v);
	const GpEdge * p_e_list = static_cast<const GpEdge *>(p_this->_Plt.HiddenEdges.v);
	return SIGN(p_v_list[p_e_list[*(const long*)p1].v2].z - p_v_list[p_e_list[*(const long*)p2].v2].z);
}

void GnuPlot::SortEdgesByZ()
{
	if(_Plt.HiddenEdges.end) {
		long i;
		long * sortarray = (long *)SAlloc::M(sizeof(long) * _Plt.HiddenEdges.end);
		// initialize sortarray with an identity mapping 
		for(i = 0; i < _Plt.HiddenEdges.end; i++)
			sortarray[i] = i;
		// sort it 
		qsort_s(sortarray, (size_t)_Plt.HiddenEdges.end, sizeof(long), GnuPlot::CompareEdgesByZMin, this);
		{
			// traverse plist in the order given by sortarray, and set the 'next' pointers 
			GpEdge * p_this = elist + sortarray[0];
			for(i = 1; i < _Plt.HiddenEdges.end; i++) {
				p_this->next = sortarray[i];
				p_this = elist + sortarray[i];
			}
			p_this->next = -1L;
		}
		// 'efirst' is the index of the leading element of plist 
		Hid3D.EFirst = sortarray[0];
		SAlloc::F(sortarray);
	}
}

/*static*/int GnuPlot::ComparePolysByZMax(void * pCtx, SORTFUNC_ARGS p1, SORTFUNC_ARGS p2)
{
	GnuPlot * p_this = static_cast<GnuPlot *>(pCtx);
	return (SIGN(((GpMeshTriangle *)p_this->_Plt.HiddenPolygons.v)[*(const long*)p1].zmax - ((GpMeshTriangle *)p_this->_Plt.HiddenPolygons.v)[*(const long*)p2].zmax));
}

void GnuPlot::SortPolysByZ()
{
	long i;
	if(_Plt.HiddenPolygons.end) {
		long * sortarray = (long *)SAlloc::M(sizeof(long) * _Plt.HiddenPolygons.end);
		// initialize sortarray with an identity mapping 
		for(i = 0; i < _Plt.HiddenPolygons.end; i++)
			sortarray[i] = i;
		// sort it 
		qsort_s(sortarray, (size_t)_Plt.HiddenPolygons.end, sizeof(long), GnuPlot::ComparePolysByZMax, this);
		// traverse plist in the order given by sortarray, and set the
		// 'next' pointers 
		// HBB 20000716: Loop backwards, to ease construction of
		// linked lists from the head: 
		{
			int grid_x;
			int grid_y;
			for(grid_x = 0; grid_x < QUADTREE_GRANULARITY; grid_x++)
				for(grid_y = 0; grid_y < QUADTREE_GRANULARITY; grid_y++)
					Hid3D.Quadtree[grid_x][grid_y] = -1;
			for(i = _Plt.HiddenPolygons.end - 1; i >= 0; i--) {
				GpMeshTriangle * p_this = plist + sortarray[i];
				int grid_x_low = CoordToTreeCell(p_this->xmin);
				int grid_x_high = CoordToTreeCell(p_this->xmax);
				int grid_y_low = CoordToTreeCell(p_this->ymin);
				int grid_y_high = CoordToTreeCell(p_this->ymax);
				for(grid_x = grid_x_low; grid_x <= grid_x_high; grid_x++) {
					for(grid_y = grid_y_low; grid_y <= grid_y_high; grid_y++) {
						qtreelist * newhead = (qtreelist *)NextFromDynArray(&Hid3D.QTree);
						newhead->next = Hid3D.Quadtree[grid_x][grid_y];
						newhead->p = sortarray[i];
						Hid3D.Quadtree[grid_x][grid_y] = newhead - ((qtreelist *)Hid3D.QTree.v);
					}
				}
			}
		}
		Hid3D.PFirst = sortarray[0];
		SAlloc::F(sortarray);
	}
}
//
// Drawing the polygons
//
//
// draw a single vertex as a point symbol, if requested by the chosen
// plot style (linespoints, points, or dots...) 
//
void GnuPlot::DrawVertex(GpTermEntry * pTerm, GpVertex * pV)
{
	int x, y;
	if(pV->lp_style) {
		const int p_type = pV->lp_style->PtType;
		TERMCOORD(pV, x, y);
		if((p_type >= -1 || oneof3(p_type, PT_CHARACTER, PT_VARIABLE, PT_CIRCLE)) && !V.ClipPoint(x, y)) {
			const t_colorspec * tc = &(pV->lp_style->pm3d_color);
			if(pV->label) {
				WriteLabel(pTerm, x, y, pV->label);
				pV->lp_style = NULL;
				return;
			}
			if(tc->type == TC_LINESTYLE && tc->lt == LT_COLORFROMCOLUMN) {
				lp_style_type style = *(pV->lp_style);
				LoadLineType(pTerm, &style, (int)pV->real_z);
				tc = &style.pm3d_color;
				ApplyPm3DColor(pTerm, tc);
			}
			else if(tc->type == TC_RGB && tc->lt == LT_COLORFROMCOLUMN)
				SetRgbColorVar(pTerm, (uint)pV->real_z);
			else if(tc->type == TC_RGB)
				SetRgbColorConst(pTerm, tc->lt);
			else if(tc->type == TC_CB)
				set_color(pTerm, Cb2Gray(pV->real_z));
			else if(tc->type == TC_Z)
				set_color(pTerm, Cb2Gray(pV->real_z));
			if(p_type == PT_CIRCLE) {
				const double radius = pV->original->CRD_PTSIZE * _3DBlk.radius_scaler;
				DoArc(pTerm, x, y, radius, 0.0, 360.0, style_from_fill(&Gg.default_fillstyle), FALSE);
				if(NeedFillBorder(pTerm, &Gg.default_fillstyle))
					DoArc(pTerm, x, y, radius, 0.0, 360.0, 0, FALSE);
				pV->lp_style = NULL;
				return;
			}
			if(pV->lp_style->PtSize == PTSZ_VARIABLE)
				(pTerm->pointsize)(pTerm, Gg.PointSize * pV->original->CRD_PTSIZE);
			if(p_type == PT_CHARACTER)
				pTerm->put_text(pTerm, x, y, pV->lp_style->p_char);
			else if(p_type == PT_VARIABLE)
				pTerm->Pnt_(x, y, (int)(pV->original->CRD_PTTYPE) - 1);
			else
				pTerm->Pnt_(x, y, p_type);
			// vertex has been drawn --> flag it as done 
			pV->lp_style = NULL;
		}
	}
}
//
// The function that actually draws the visible portions of lines 
//
void GnuPlot::DrawEdge(GpTermEntry * pTerm, GpEdge * e, GpVertex * v1, GpVertex * v2)
{
	// It used to be that e contained style as a integer linetype.
	// This destroyed any style attributes set in the splot command.
	// We really just want to extract a colorspec.
	t_colorspec color = e->lp->pm3d_color;
	lp_style_type lptemp = *(e->lp);
	bool recolor = FALSE;
	bool arrow = oneof2(lptemp.PtType, PT_ARROWHEAD, PT_BACKARROW);
	int varcolor;
	if(arrow && (e->style == PT_BACKARROW))
		varcolor = static_cast<int>(v2->real_z);
	else
		varcolor = static_cast<int>(v1->real_z);
	// This handles 'lc rgb variable' 
	if(color.type == TC_RGB && color.lt == LT_COLORFROMCOLUMN) {
		recolor = TRUE;
		lptemp.pm3d_color.lt = varcolor;
	}
	else if(color.type == TC_RGB && (lptemp.flags & LP_EXPLICIT_COLOR)) { // This handles explicit 'lc rgb' in the plot command 
		recolor = TRUE;
	}
	else if(color.type == TC_RGB && e->lp == &Gg.border_lp) {
		lptemp.pm3d_color.lt = varcolor;
	}
	else if(lptemp.l_type == LT_COLORFROMCOLUMN) { // This handles 'lc variable' 
		recolor = TRUE;
		LoadLineType(pTerm, &lptemp, varcolor);
	}
	else if(arrow) { // This handles style VECTORS 
		lptemp.PtType = e->style;
	}
	else if(_Plt.hiddenBacksideLinetypeOffset && e->lp->pm3d_color.type != TC_Z) { // This is the default style: color top and bottom in successive colors 
		recolor = TRUE;
		LoadLineType(pTerm, &lptemp, e->style + 1);
		color = lptemp.pm3d_color;
	}
	else // The remaining case is hiddenBacksideLinetypeOffset == 0 in which case we assume the correct color is already set 
		;
	if(recolor) {
		color = lptemp.pm3d_color;
		lptemp = *(e->lp);
		lptemp.pm3d_color = color;
		if(arrow)
			lptemp.PtType = e->style;
	}
	// Only the original tip of an arrow should show an arrowhead 
	// FIXME:  Arrowhead lines are not themselves subject to hidden line removal 
	if(arrow) {
		// FIXME: e->lp points to this_plot->lp_properties but what we need is 
		// a pointer to the immediately following field e->arrow_properties.   
		lp_style_type * lp = e->lp;
		arrow_style_type * as = (arrow_style_type*)(&lp[1]);
		ApplyHeadProperties(pTerm, as);
		if(as->head == BOTH_HEADS)
			lptemp.PtType = PT_BOTHHEADS;
		if(e->v2 != v2-vlist && e->v1 != v1-vlist)
			lptemp.PtType = 0;
		if(lptemp.PtType == PT_BACKARROW) {
			if(e->v2 == v2-vlist && e->v1 != v1-vlist)
				lptemp.PtType = 0;
		}
		if(lptemp.PtType == PT_ARROWHEAD) {
			if(e->v1 == v1-vlist && e->v2 != v2-vlist)
				lptemp.PtType = 0;
		}
		if(lptemp.PtType == PT_BOTHHEADS) {
			if(e->v1 == v1-vlist && e->v2 != v2-vlist)
				lptemp.PtType = PT_BACKARROW;
			if(e->v2 == v2-vlist && e->v1 != v1-vlist)
				lptemp.PtType = PT_ARROWHEAD;
		}
	}
	Draw3DLineUnconditional(pTerm, v1, v2, &lptemp, color);
	if(e->lp->flags & LP_SHOW_POINTS) {
		DrawVertex(pTerm, v1);
		DrawVertex(pTerm, v2);
	}
}
//
// The depth sort algorithm (in_front) and its
// whole lot of helper functions        
//
// Split a given line segment into two at an inner point. The inner
// point is specified as a fraction of the line-length (0 is V1, 1 is V2) 
// 
// HBB 20001108: changed to now take two vertex pointers as its
// arguments, rather than an edge pointer. 
// 
// HBB 20001204: changed interface again. Now use vertex indices,
// rather than pointers, to avoid problems with dangling pointers
// after GnuPlot::NextFromDynArray() call. 
// 
long GnuPlot::SplitLineAtRatio(long vnum1, long vnum2/* vertex indices of line to split */, double w/* where to split it */)
{
	// Create a new vertex 
	GpVertex * v = (GpVertex *)NextFromDynArray(&_Plt.HiddenVertices);
	v->x = (vlist[vnum2].x - vlist[vnum1].x) * w + vlist[vnum1].x;
	v->y = (vlist[vnum2].y - vlist[vnum1].y) * w + vlist[vnum1].y;
	v->z = (vlist[vnum2].z - vlist[vnum1].z) * w + vlist[vnum1].z;
	v->real_z = (vlist[vnum2].real_z - vlist[vnum1].real_z) * w + vlist[vnum1].real_z;
	// no point symbol for vertices generated by splitting an edge 
	v->lp_style = NULL;
	// additional checks to prevent adding unnecessary vertices 
	if(V_EQUAL(v, vlist + vnum1)) {
		DropLastDynArray(&_Plt.HiddenVertices);
		return vnum1;
	}
	if(V_EQUAL(v, vlist + vnum2)) {
		DropLastDynArray(&_Plt.HiddenVertices);
		return vnum2;
	}
	return (v - vlist);
}

/* Compute the 'signed area' of 3 points in their 2d projection
 * to the x-y plane. Essentially the z component of the crossproduct.
 * Should come out positive if v1, v2, v3 are ordered counter-clockwise */

static GP_INLINE double area2D(GpVertex * v1, GpVertex * v2, GpVertex * v3)
{
	double dx12 = v2->x - v1->x; /* x/y components of (v2-v1) and (v3-v1) */
	double dx13 = v3->x - v1->x;
	double dy12 = v2->y - v1->y;
	double dy13 = v3->y - v1->y;
	return (dx12 * dy13 - dy12 * dx13);
}

/*********************************************************************/
/* The actual heart of all this: determines if edge at index 'edgenum'
 * of the elist is in_front of all the polygons, or not. If necessary,
 * it will recursively call itself to isolate more than one visible
 * fragment of the input edge. Wherever possible, recursion is
 * avoided, by in-place modification of the edge.
 *
 * The visible fragments are then drawn by a call to 'draw_edge' from
 * inside this routine. */
/*********************************************************************/
/* HBB 20001108: changed to now take the vertex numbers as additional
 * arguments. The idea is to not overwrite the endpoint stored with
 * the edge, so Test 2 will catch on even after the subject edge has
 * been split up before one of its two polygons is tested against it. */
int GnuPlot::InFront(GpTermEntry * pTerm, long edgenum/* number of the edge in elist */, long vnum1, long vnum2/* numbers of its endpoints */, long * firstpoly/* first plist index to consider */)
{
	GpMeshTriangle * p; // pointer to current testing polygon 
	long polynum; // ... and its index in the plist 
	GpVertex * v1;
	GpVertex * v2; /* pointers to vertices of input edge */
	coordval xmin, xmax; /* all of these are for the edge */
	coordval ymin, ymax;
	coordval zmin;
	int grid_x, grid_y;
	int grid_x_low, grid_x_high;
	int grid_y_low, grid_y_high;
	long listhead;
	/* zmin of the edge, as it started out. This is needed separately to
	 * allow modifying '*firstpoly', without moving it too far to the
	 * front. */
	coordval first_zmin;
	/* Keep track of number of vertices before the process and compare
	 * at end of process to know how many vertices to remove. */
	long enter_vertices;
	/* macro for eliminating tail-recursion inside in_front: when the
	 * current edge is modified, recompute all function-wide status
	 * variables. Note that it guarantees that v1 is always closer to
	 * the viewer than v2 (in z direction) */
	/* HBB 20001108: slightly changed so it can be called with vnum1
	 * and vnum2 as its arguments, too */
#define setup_edge(vert1, vert2)                \
	do {                                        \
		if(vlist[vert1].z > vlist[vert2].z) {  \
			v1 = vlist + (vert1);               \
			v2 = vlist + (vert2);               \
		} else {                                \
			v1 = vlist + (vert2);               \
			v2 = vlist + (vert1);               \
		}                                       \
		vnum1 = v1 - vlist;                     \
		vnum2 = v2 - vlist;                     \
		zmin = v2->z;                           \
		if(v1->x > v2->x) { xmin = v2->x; xmax = v1->x; } else { xmin = v1->x; xmax = v2->x; } \
		if(v1->y > v2->y) { ymin = v2->y; ymax = v1->y; } else { ymin = v1->y; ymax = v2->y; } \
	} while(0) /* end macro setup_edge */

	/* use the macro for initial setup, too: */
	setup_edge(vnum1, vnum2);
	first_zmin = zmin;
	enter_vertices = _Plt.HiddenVertices.end;
	grid_x_low = CoordToTreeCell(xmin);
	grid_x_high = CoordToTreeCell(xmax);
	grid_y_low = CoordToTreeCell(ymin);
	grid_y_high = CoordToTreeCell(ymax);
	for(grid_x = grid_x_low; grid_x <= grid_x_high; grid_x++)
		for(grid_y = grid_y_low; grid_y <= grid_y_high; grid_y++)
			for(listhead = Hid3D.Quadtree[grid_x][grid_y]; listhead >= 0; listhead = ((qtreelist *)Hid3D.QTree.v)[listhead].next) {
				// shortcut variables for the three vertices of 'p':
				GpVertex * w1;
				GpVertex * w2;
				GpVertex * w3;
				polynum = ((qtreelist *)Hid3D.QTree.v)[listhead].p;
				p = plist + polynum;
				/* OK, off we go with the real work. This algorithm had its
				 * beginnings as the one of 'HLines.java', as described in
				 * the book 'Computer Graphics for Java Programmers', by
				 * Dutch professor Leen Ammeraal, published by J. Wiley &
				 * Sons, ISBN 0 471 98142 7.
				 *
				 * However, it was revamped with an approach that breaks
				 * up the edge into five possible subsegments and removes
				 * the one contiguous subsegment, if any, that is hidden.
				 */

				/* Test 1 (2D): minimax tests. Do x/y ranges of polygon
				 * and edge have any overlap? */
				if(0 || (p->xmax < xmin) || (p->xmin > xmax) || (p->ymax < ymin) || (p->ymin > ymax))
					continue;
				/* Tests 2 and 3 switched... */

				/* Test 3 (3D): Is edge completely in front of polygon? */
				if(p->zmax < zmin) {
					/* Polygon completely behind this edge. Move start of
					 * relevant plist to this point, to speed up next
					 * run. This makes use of the fact that elist is also
					 * kept in upwardly sorted order of zmin, i.e. the
					 * condition found here will also hold for all coming
					 * edges in the list */
					if(p->zmax < first_zmin)
						*firstpoly = polynum;
					continue; /* this polygon is done with */
				}

				/* Test 2 (0D): does edge belong to this very polygon? */
				/* 20001108: to make this rejector more effective, do keep
				 * the original edge vertices unchanged */
				if(1 && (0 || (p->vertex[0] == elist[edgenum].v1) || (p->vertex[1] == elist[edgenum].v1) || (p->vertex[2] == elist[edgenum].v1)) && 
					(0 || (p->vertex[0] == elist[edgenum].v2) || (p->vertex[1] == elist[edgenum].v2) || (p->vertex[2] == elist[edgenum].v2)))
					continue;
				w1 = vlist + p->vertex[0];
				w2 = vlist + p->vertex[1];
				w3 = vlist + p->vertex[2];

				/* The final 'catch-all' handler: [was Test 4-9 (3D)]
				 * Daniel Sebald 2007
				 * ---------------------------------------------------
				 * If one examines the possible scenarios for an edge (v1,v2)
				 * passing through a triangular 3D element in 2D space, it
				 * is evident that at most 4 breaks in the edge are possible,
				 * one for each infinite triangle side intersection and
				 * one for the edge possibly passing directly through the
				 * polygon.  We first compute all these intersections in terms
				 * of parameterization v = v1 + u * (v2 - v1).  That gives us
				 * four values of u.  They likely will not all be in the range
				 * (0,1), i.e., between v1 and v2.  We discard all those not
				 * in the range, and the remaining associated points along with
				 * endpoint v1 and v2 describe a series of subsegements that are
				 * considered individually.  If any contiguous subgroup is
				 * hidden (there can only be at most one for a convex polygon),
				 * it is removed.
				 *
				 * This routine is general in the sense that the earlier tests
				 * are only needed for speed.
				 *
				 * The following website illustrates geometrical concepts and
				 * formulas:  http://local.wasp.uwa.edu.au/~pbourke/geometry/
				 */

				{
					double u_int[4]; /* Intersection points along edge v1, v2 */
					double u_seg[6]; /* Sorted subsegment points */
					int segs; /* Number of segments */
					int i;
					u_int[0] = intersect_line_plane(v1, v2, p->plane);
					u_int[1] = intersect_line_line(v1, v2, w1, w2);
					u_int[2] = intersect_line_line(v1, v2, w2, w3);
					u_int[3] = intersect_line_line(v1, v2, w3, w1);
					/* Check if between v1 and v2 */
					u_seg[0] = 0;
					segs = 1;
					for(i = 0; i < 4; i++) {
						if((0 < u_int[i]) && (u_int[i] < 1)) {
							u_seg[segs] = u_int[i];
							segs++;
						}
					}
					u_seg[segs] = 1;
					// Sort the points.  First and last point already in order
					for(i = 1; i < segs; i++) {
						int j = i+1;
						for(; j < segs; j++) {
							SExchangeForOrder(&u_seg[i], &u_seg[j]);
						}
					}
					// Check if contiguous segments or segment is covered 
					for(i = 0; i < segs; i++) {
						int covA = CoverPointPoly(v1, v2, u_seg[i], p);
						if(covA) {
							// First covered point, now look for last covered point 
							int j, covB = 0;
							for(j = i; j < segs; j++) {
								int cover = CoverPointPoly(v1, v2, u_seg[j+1], p);
								if(!cover)
									break;
								covB = cover;
							}
							if(i == j)
								break; /* Only one covered point, no segment covered */
							if(covA == 2 && covB == 2)
								break; /* Points covered, but both are on the plane */
							else {
								// This is the hidden segment 
								if(i == 0) {
									// Missing segment is at start of v1, v2 
									if(j == segs) {
										// Whole edge is hidden 
										while(_Plt.HiddenVertices.end > enter_vertices)
											DropLastDynArray(&_Plt.HiddenVertices);
										return 0;
									}
									else {
										// Shrink the edge and continue 
										long newvert = SplitLineAtRatio(vnum1, vnum2, u_seg[j]);
										setup_edge(newvert, vnum2);
										break;
									}
								}
								else if(j == segs) {
									// Missing segment is at end of v1, v2.  The i = 0
									// case already tested, so shrink edge and continue 
									long newvert = SplitLineAtRatio(vnum1, vnum2, u_seg[i]);
									setup_edge(vnum1, newvert);
									break;
								}
								else {
									/* Handle new edge then shrink edge */
									long newvert[2];
									newvert[0] = SplitLineAtRatio(vnum1, vnum2, u_seg[i]);
									newvert[1] = SplitLineAtRatio(vnum1, vnum2, u_seg[j]);
									/* If the newvert[1] is vnum1 this would be an infinite
									 * loop and stack overflow if not checked since in_front()
									 * was just called with vnum1 and vnum2 and got to this
									 * point.  This is the equivalent of snipping out a tiny
									 * segment near end of an edge.  Simply ignore.
									 */
									if(newvert[1] != vnum1) {
										InFront(pTerm, edgenum, newvert[1], vnum2, &polynum); // @recursion
										setup_edge(vnum1, newvert[0]);
									}
									break;
								}
							}
						}
					}

					/* Nothing is covered */
					continue;
				} /* end of part 'T4-9' */
			}/* for (polygons in list) */
	// Came here, so there's something left of this edge, which needs
	// to be drawn.  But the vertices are different, now, so copy our
	// new vertices back into 'e' 
	DrawEdge(pTerm, elist + edgenum, vlist + vnum1, vlist + vnum2);
	while(_Plt.HiddenVertices.end > enter_vertices)
		DropLastDynArray(&_Plt.HiddenVertices);
	return 1;
}
// 
// HBB 20000617: reimplemented this routine from scratch */
// Externally callable function to draw a line, but hide it behind the
// visible surface. 
// NB: The p_vertex arguments are not allowed to be pointers into the
// hidden3d 'vlist' structure. If they are, they may become invalid
// before they're used, because of the GnuPlot::NextFromDynArray() call. 
// 
//void draw_line_hidden(GpVertex * v1, GpVertex * v2/* pointers to the end vertices */, lp_style_type * lp/* line and point style to draw in */)
void GnuPlot::DrawLineHidden(GpTermEntry * pTerm, GpVertex * v1, GpVertex * v2/* pointers to the end vertices */, lp_style_type * lp/* line and point style to draw in */)
{
	long vstore1, vstore2;
	long edgenum;
	long temp_pfirst;
	// If no polygons have been stored, nothing can be hidden, and we
	// can't use in_front() because the datastructures are partly
	// invalid. So just draw the line and be done with it 
	if(!_Plt.HiddenPolygons.end) {
		Draw3DLineUnconditional(pTerm, v1, v2, lp, lp->pm3d_color);
	}
	else {
		// Copy two vertices into hidden3d arrays: 
		NextFromDynArray(&_Plt.HiddenVertices);
		vstore1 = _Plt.HiddenVertices.end - 1;
		vlist[vstore1] = *v1;
		if(v2) {
			vlist[vstore1].lp_style = NULL;
			NextFromDynArray(&_Plt.HiddenVertices);
			vstore2 = _Plt.HiddenVertices.end - 1;
			vlist[vstore2] = *v2;
			vlist[vstore2].lp_style = NULL;
		}
		else {
			// v2 == NULL --> this is a point symbol to be drawn. Make two
			// vertex pointers the same, and set up the 'style' field 
			vstore2 = vstore1;
			vlist[vstore2].lp_style = lp;
		}
		// store the edge into the hidden3d datastructures 
		edgenum = MakeEdge(vstore1, vstore2, lp, lp->l_type, -1);
		// remove hidden portions of the line, and draw what remains 
		temp_pfirst = Hid3D.PFirst;
		InFront(pTerm, edgenum, elist[edgenum].v1, elist[edgenum].v2, &temp_pfirst);
		// release allocated storage slots: 
		DropLastDynArray(&_Plt.HiddenEdges);
		DropLastDynArray(&_Plt.HiddenVertices);
		if(v2)
			DropLastDynArray(&_Plt.HiddenVertices);
	}
}
//
// Externally callable function to draw a label, but hide it behind any
// visible occluding surfaces. 
//
//void draw_label_hidden(GpVertex * v, struct lp_style_type * lp, int x, int y)
void GnuPlot::DrawLabelHidden(GpTermEntry * pTerm, GpVertex * v, lp_style_type * lp, int x, int y)
{
	long thisvertex, edgenum, temp_pfirst;
	// If there is no surface to hide behind, just draw the label 
	if(!_Plt.HiddenPolygons.end)
		WriteLabel(pTerm, x, y, v->label);
	else {
		NextFromDynArray(&_Plt.HiddenVertices);
		thisvertex = _Plt.HiddenVertices.end - 1;
		vlist[thisvertex] = *v;
		vlist[thisvertex].lp_style = lp; /* Not sure this is necessary */
		lp->flags |= LP_SHOW_POINTS; /* Labels can use the code for hidden points */
		edgenum = MakeEdge(thisvertex, thisvertex, lp, lp->l_type, -1);
		FPRINTF((stderr, "label: \"%s\" at [%d %d]  vertex %ld edge %ld\n", v->label->text, x, y, thisvertex, edgenum));
		temp_pfirst = Hid3D.PFirst;
		InFront(pTerm, edgenum, elist[edgenum].v1, elist[edgenum].v2, &temp_pfirst);
		DropLastDynArray(&_Plt.HiddenEdges);
		DropLastDynArray(&_Plt.HiddenVertices);
	}
}
//
// and, finally, the 'mother function' that uses all these lots of tools
// 
//void plot3d_hidden(GpTermEntry * pTerm, GpSurfacePoints * plots, int pcount)
void GnuPlot::Plot3DHidden(GpTermEntry * pTerm, GpSurfacePoints * plots, int pcount)
{
	// make vertices, edges and polygons out of all the plots 
	BuildNetworks(plots, pcount);
	if(!_Plt.HiddenEdges.end) {
		// No drawable edges found. Free all storage and bail out. 
		TermHiddenLineRemoval();
		IntError(NO_CARET, "*All* edges undefined or out of range, thus no plot.");
	}
	if(!_Plt.HiddenPolygons.end) {
		// No polygons anything could be hidden behind... 
		SortEdgesByZ();
		while(Hid3D.EFirst >= 0) {
			DrawEdge(pTerm, elist+Hid3D.EFirst, vlist + elist[Hid3D.EFirst].v1, vlist + elist[Hid3D.EFirst].v2);
			Hid3D.EFirst = elist[Hid3D.EFirst].next;
		}
	}
	else {
		long int temporary_pfirst;
		SortEdgesByZ(); // Presort edges in z order 
		SortPolysByZ(); // Presort polygons in z order 
		temporary_pfirst = Hid3D.PFirst;
		while(Hid3D.EFirst >=0) {
			if(elist[Hid3D.EFirst].style != LT_NODRAW) // skip invisible edges 
				InFront(pTerm, Hid3D.EFirst, elist[Hid3D.EFirst].v1, elist[Hid3D.EFirst].v2, &temporary_pfirst);
			Hid3D.EFirst = elist[Hid3D.EFirst].next;
		}
	}
}

//void reset_hidden3doptions()
void GnuPlot::ResetHidden3DOptions()
{
	_Plt.hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
	_Plt.hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
	_Plt.hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
	_Plt.hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
	_Plt.hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;
	_3DBlk.hidden3d_layer = LAYER_BACK;
}

/* Emacs editing help for HBB:
 * Local Variables: ***
 * c-basic-offset: 4 ***
 * End: ***
 */
