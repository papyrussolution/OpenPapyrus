// GNUPLOT - contour.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/*
 * AUTHORS
 *   Original Software: Gershon Elber
 *   Improvements to the numerical algorithms: Hans-Martin Keller, 1995,1997 (hkeller@gwdg.de)
 *
 */
#include <gnuplot.h>
#pragma hdrstop

// FIXME HBB 2000052: yet another local copy of 'epsilon'. Why? 
#define EPSILON  1e-5           /* Used to decide if two float are equal. */
#define SQR(x)  ((x) * (x))

//static int    solve_cubic_1(tri_diag m[], int n);
//static void   solve_cubic_2(tri_diag m[], double x[], int n);
//static double fetch_knot(bool contr_isclosed, int num_of_points, int order, int i);

static int reverse_sort(SORTFUNC_ARGS arg1, SORTFUNC_ARGS arg2)
{
	double const * p1 = (double const *)arg2;
	double const * p2 = (double const *)arg1;
	if(*p1 > *p2)
		return 1;
	else if(*p1 < *p2)
		return -1;
	else
		return 0;
}
//
// Calculate minimum and maximum values
//
#if 0 // {
static void calc_min_max(int numIsoLines/* number of iso-lines input */, const iso_curve * pIsoLines/* iso-lines input */,
    double * xx_min, double * yy_min, double * zz_min, double * xx_max, double * yy_max, double * zz_max) /* min/max values in/out */
{
	const int grid_x_max = pIsoLines->p_count; // number of vertices per iso_line 
	(*xx_min) = (*yy_min) = (*zz_min) = VERYLARGE;  // clear min/max values 
	(*xx_max) = (*yy_max) = (*zz_max) = -VERYLARGE;
	for(int j = 0; j < numIsoLines; j++) {
		const GpCoordinate * p_vertex = pIsoLines->points;
		for(int i = 0; i < grid_x_max; i++) {
			const GpCoordinate & r_item = p_vertex[i];
			if(r_item.type != UNDEFINED) {
				SETMAX((*xx_max), r_item.x);
				SETMAX((*yy_max), r_item.y);
				SETMAX((*zz_max), r_item.z);
				SETMIN((*xx_min), r_item.x);
				SETMIN((*yy_min), r_item.y);
				SETMIN((*zz_min), r_item.z);
			}
		}
		pIsoLines = pIsoLines->next;
	}
	// fprintf(stderr, " x: %g, %g\n", (*xx_min), (*xx_max));
	// fprintf(stderr, " y: %g, %g\n", (*yy_min), (*yy_max));
	// fprintf(stderr, " z: %g, %g\n", (*zz_min), (*zz_max));
}
#endif // } 0
static void calc_min_max(int numIsoLines/* number of iso-lines input */, const iso_curve * pIsoLines/* iso-lines input */, SPoint3R & rMin, SPoint3R & rMax/* min/max values in/out */)
{
	const int grid_x_max = pIsoLines->p_count; // number of vertices per iso_line 
	rMin.Set(VERYLARGE); // clear min/max values 
	rMax.Set(-VERYLARGE);
	for(int j = 0; j < numIsoLines; j++) {
		const GpCoordinate * p_vertex = pIsoLines->points;
		for(int i = 0; i < grid_x_max; i++) {
			const GpCoordinate & r_item = p_vertex[i];
			if(r_item.type != UNDEFINED) {
				SETMAX(rMax.x, r_item.Pt.x);
				SETMAX(rMax.y, r_item.Pt.y);
				SETMAX(rMax.z, r_item.Pt.z);
				SETMIN(rMin.x, r_item.Pt.x);
				SETMIN(rMin.y, r_item.Pt.y);
				SETMIN(rMin.z, r_item.Pt.z);
			}
		}
		pIsoLines = pIsoLines->next;
	}
	// fprintf(stderr, " x: %g, %g\n", (*xx_min), (*xx_max));
	// fprintf(stderr, " y: %g, %g\n", (*yy_min), (*yy_max));
	// fprintf(stderr, " z: %g, %g\n", (*zz_min), (*zz_max));
}
// 
// Entry routine to this whole set of contouring module.
// 
//gnuplot_contours * contour(int numIsoLines, iso_curve * pIsoLines)
gnuplot_contours * GnuPlot::Contour(int numIsoLines, iso_curve * pIsoLines)
{
	int i;
	double * zlist;
	PolyNode * p_polys, * p_poly;
	EdgeNode * p_edges, * p_edge;
	double z = 0;
	double z0 = 0;
	double dz = 0;
	gnuplot_contours * save_contour_list;
	// HBB FIXME 20050804: The number of contour_levels as set by 'set
	// cnrparam lev inc a,b,c' is almost certainly wrong if z axis is
	// logarithmic 
	int num_of_z_levels = _Cntr.ContourLevels; // # Z contour levels. 
	_Cntr.InterpKind = _Cntr.ContourKind;
	_Cntr.P_ContourList = NULL;
	// 
	// Calculate min/max values :
	// 
	//calc_min_max(numIsoLines, pIsoLines, &_Cntr.Min.x, &_Cntr.Min.y, &_Cntr.Min.z, &_Cntr.Max.x, &_Cntr.Max.y, &_Cntr.Max.z);
	calc_min_max(numIsoLines, pIsoLines, _Cntr.Min, _Cntr.Max);
	// 
	// Generate list of edges (p_edges) and list of triangles (p_polys):
	// 
	GenTriangle(numIsoLines, pIsoLines, &p_polys, &p_edges);
	_Cntr.CrntCntrPtIndex = 0;
	if(_Cntr.ContourLevelsKind == LEVELS_AUTO) {
		if(AxS.__Z().IsNonLinear()) {
			_Cntr.Max.z = EvalLinkFunction(AxS.__Z().linked_to_primary, _Cntr.Max.z);
			_Cntr.Min.z = EvalLinkFunction(AxS.__Z().linked_to_primary, _Cntr.Min.z);
		}
		dz = fabs(_Cntr.Max.z - _Cntr.Min.z);
		if(dz == 0)
			return NULL; /* empty z range ? */
		/* Find a tic step that will generate approximately the
		 * desired number of contour levels. The "* 2" is historical.
		 * */
		dz = quantize_normal_tics(dz, ((int)_Cntr.ContourLevels + 1) * 2);
		z0 = floor(_Cntr.Min.z / dz) * dz;
		num_of_z_levels = ffloori((_Cntr.Max.z - z0) / dz);
		if(num_of_z_levels <= 0)
			return NULL;
	}
	// Build a list of contour levels 
	zlist = (double *)SAlloc::M(num_of_z_levels * sizeof(double));
	for(i = 0; i < num_of_z_levels; i++) {
		switch(_Cntr.ContourLevelsKind) {
			case LEVELS_AUTO:
			    z = z0 + (i+1) * dz;
			    z = CheckZero(z, dz);
			    if(AxS.__Z().IsNonLinear())
				    z = EvalLinkFunction((&AxS.__Z()), z);
			    break;
			case LEVELS_INCREMENTAL:
			    if(AxS.__Z().log)
				    z = contour_levels_list[0] * pow(contour_levels_list[1], (double)i);
			    else
				    z = contour_levels_list[0] + i * contour_levels_list[1];
			    break;
			case LEVELS_DISCRETE:
			    z = contour_levels_list[i];
			    break;
		}
		zlist[i] = z;
	}
	// Sort the list high-to-low if requested 
	if(_Cntr.ContourSortLevels)
		qsort(zlist, num_of_z_levels, sizeof(double), reverse_sort);
	// Create contour line for each z value in the list 
	for(i = 0; i < num_of_z_levels; i++) {
		z = zlist[i];
		_Cntr.ContourLevel = z;
		save_contour_list = _Cntr.P_ContourList;
		GenContours(p_edges, z, _Cntr.Min.x, _Cntr.Max.x, _Cntr.Min.y, _Cntr.Max.y);
		if(_Cntr.P_ContourList != save_contour_list) {
			_Cntr.P_ContourList->isNewLevel = 1;
			// Nov-2011 Use gprintf rather than sprintf so that LC_NUMERIC is used 
			GPrintf(_Cntr.P_ContourList->label, sizeof(_Cntr.P_ContourList->label), _Cntr.ContourFormat, 1.0, z);
			_Cntr.P_ContourList->z = z;
		}
	}
	/* Free all contouring related temporary data. */
	SAlloc::F(zlist);
	while(p_polys) {
		p_poly = p_polys->next;
		FREEANDASSIGN(p_polys, p_poly);
	}
	while(p_edges) {
		p_edge = p_edges->next;
		FREEANDASSIGN(p_edges, p_edge);
	}
	return _Cntr.P_ContourList;
}
// 
// Adds another point to the currently build contour.
// 
//static void add_cntr_point(double x, double y)
void GnuPlot::AddCntrPoint(double x, double y)
{
	int index;
	if(_Cntr.CrntCntrPtIndex >= MAX_POINTS_PER_CNTR - 1) {
		index = _Cntr.CrntCntrPtIndex - 1;
		EndCrntCntr();
		_Cntr.CrntCntr[0] = _Cntr.CrntCntr[index * 2];
		_Cntr.CrntCntr[1] = _Cntr.CrntCntr[index * 2 + 1];
		_Cntr.CrntCntrPtIndex = 1; /* Keep the last point as first of this one. */
	}
	_Cntr.CrntCntr[_Cntr.CrntCntrPtIndex * 2] = x;
	_Cntr.CrntCntr[_Cntr.CrntCntrPtIndex * 2 + 1] = y;
	_Cntr.CrntCntrPtIndex++;
}
//
// Done with current contour - create gnuplot data structure for it.
//
//static void end_crnt_cntr()
void GnuPlot::EndCrntCntr()
{
	gnuplot_contours * cntr = (gnuplot_contours *)SAlloc::M(sizeof(gnuplot_contours));
	cntr->coords = (GpCoordinate *)SAlloc::M(sizeof(GpCoordinate) * _Cntr.CrntCntrPtIndex);
	for(int i = 0; i < _Cntr.CrntCntrPtIndex; i++) {
		cntr->coords[i].Pt.Set(_Cntr.CrntCntr[i*2], _Cntr.CrntCntr[i*2+1], _Cntr.ContourLevel);
	}
	cntr->num_pts = _Cntr.CrntCntrPtIndex;
	cntr->label[0] = '\0';
	cntr->next = _Cntr.P_ContourList;
	_Cntr.P_ContourList = cntr;
	_Cntr.P_ContourList->isNewLevel = 0;
	_Cntr.CrntCntrPtIndex = 0;
}
// 
// Generates all contours by tracing the intersecting triangles.
// 
//static void gen_contours(EdgeNode * p_edges, double z_level, double xx_min, double xx_max, double yy_min, double yy_max)
void GnuPlot::GenContours(EdgeNode * p_edges, double z_level, double xx_min, double xx_max, double yy_min, double yy_max)
{
	int num_active = UpdateAllEdges(p_edges, z_level); // Do pass 1. // Get the number of edges marked ACTIVE. 
	bool contr_isclosed = false; // Start to look for contour on boundaries. 
	while(num_active > 0) { // Do Pass 2. 
		// Generate One contour (and update NumActive as needed): 
		ContourNode * p_cntr = GenOneContour(p_edges, z_level, &contr_isclosed, &num_active);
		// Emit it in requested format: 
		PutContour(p_cntr, xx_min, xx_max, yy_min, yy_max, contr_isclosed);
	}
}
//
// Does pass 1, or marks the edges which are active (crosses this z_level)
// Returns number of active edges (marked ACTIVE).
//
//static int update_all_edges(EdgeNode * p_edges, double z_level)
int GnuPlot::UpdateAllEdges(EdgeNode * p_edges, double z_level)
{
	int count = 0;
	while(p_edges) {
		// use the same test at both vertices to avoid roundoff errors 
		if((p_edges->vertex[0]->Pt.z >= z_level) != (p_edges->vertex[1]->Pt.z >= z_level)) {
			p_edges->is_active = true;
			count++;
		}
		else
			p_edges->is_active = false;
		p_edges = p_edges->next;
	}
	return count;
}
// 
// Does pass 2, or find one complete contour out of the triangulation data base:
// 
// Returns a pointer to the contour (as linked list), contr_isclosed
// tells if the contour is a closed line or not, and num_active is updated.
// 
//static ContourNode * gen_one_contour(EdgeNode * p_edges/* list of edges input */,
    //double z_level/* Z level of contour input */, bool * contr_isclosed/* open or closed contour, in/out */, int * num_active/* number of active edges in/out */)
GnuPlot::ContourNode * GnuPlot::GenOneContour(EdgeNode * pEdges/* list of edges input */,
	double z_level/* Z level of contour input */, bool * contr_isclosed/* open or closed contour, in/out */, int * num_active/* number of active edges in/out */)
{
	EdgeNode * pe_temp;
	if(!*contr_isclosed) {
		// Look for something to start with on boundary: 
		pe_temp = pEdges;
		while(pe_temp) {
			if(pe_temp->is_active && (pe_temp->position == EdgeNode::BOUNDARY))
				break;
			pe_temp = pe_temp->next;
		}
		if(!pe_temp)
			*contr_isclosed = TRUE; // No more contours on boundary. 
		else {
			return TraceContour(pe_temp, z_level, num_active, *contr_isclosed);
		}
	}
	if(*contr_isclosed) {
		// Look for something to start with inside: 
		pe_temp = pEdges;
		while(pe_temp) {
			if(pe_temp->is_active && (pe_temp->position != EdgeNode::BOUNDARY))
				break;
			pe_temp = pe_temp->next;
		}
		if(!pe_temp) {
			*num_active = 0;
			fprintf(stderr, "gen_one_contour: no contour found\n");
			return NULL;
		}
		else {
			*contr_isclosed = TRUE;
			return TraceContour(pe_temp, z_level, num_active, *contr_isclosed);
		}
	}
	return NULL; // We should never be here, but lint... 
}
// 
// Search the data base along a contour starts at the edge pe_start until
// a boundary edge is detected or until we close the loop back to pe_start.
// Returns a linked list of all the points on the contour
// Also decreases num_active by the number of points on contour.
// 
//static ContourNode * trace_contour(EdgeNode * pe_start/* edge to start contour input */,
    //double z_level/* Z level of contour input */, int * num_active/* number of active edges in/out */, bool contr_isclosed/* open or closed contour line (input) */)
GnuPlot::ContourNode * GnuPlot::TraceContour(EdgeNode * pe_start/* edge to start contour input */,
	double z_level/* Z level of contour input */, int * num_active/* number of active edges in/out */, bool contr_isclosed/* open or closed contour line (input) */)
{
	ContourNode * p_cntr, * pc_tail;
	EdgeNode * p_next_edge;
	PolyNode * p_poly, * PLastpoly = NULL;
	int i;
	EdgeNode * p_edge = pe_start; /* first edge to start contour */
	// Generate the header of the contour - the point on pe_start. 
	if(!contr_isclosed) {
		pe_start->is_active = FALSE;
		(*num_active)--;
	}
	if(p_edge->poly[0] || p_edge->poly[1]) { /* more than one point */
		p_cntr = pc_tail = UpdateCntrPt(pe_start, z_level); /* first point */
		do {
			// Find polygon to continue (Not where we came from - PLastpoly): 
			p_poly = (p_edge->poly[0] == PLastpoly) ? p_edge->poly[1] : p_edge->poly[0];
			p_next_edge = NULL; /* In case of error, remains NULL. */
			for(i = 0; i < 3; i++) /* Test the 3 edges of the polygon: */
				if(p_poly->edge[i] != p_edge)
					if(p_poly->edge[i]->is_active)
						p_next_edge = p_poly->edge[i];
			if(!p_next_edge) { /* Error exit */
				pc_tail->next = NULL;
				FreeContour(p_cntr);
				fprintf(stderr, "trace_contour: unexpected end of contour\n");
				return NULL;
			}
			p_edge = p_next_edge;
			PLastpoly = p_poly;
			p_edge->is_active = FALSE;
			(*num_active)--;
			// Do not allocate contour points on diagonal edges 
			if(p_edge->position != EdgeNode::DIAGONAL) {
				pc_tail->next = UpdateCntrPt(p_edge, z_level);
				// Remove nearby points 
				if(FuzzyEqual(pc_tail, pc_tail->next)) {
					SAlloc::F(pc_tail->next);
				}
				else
					pc_tail = pc_tail->next;
			}
		} while((p_edge != pe_start) && (p_edge->position != EdgeNode::BOUNDARY));
		pc_tail->next = NULL;
		// For closed contour the first and last point should be equal 
		if(pe_start == p_edge) {
			(p_cntr->x) = (pc_tail->x);
			(p_cntr->y) = (pc_tail->y);
		}
	}
	else {                  /* only one point, forget it */
		p_cntr = NULL;
	}
	return p_cntr;
}
// 
// Allocates one contour location and update it to to correct position
// according to z_level and edge p_edge.
// 
//static ContourNode * update_cntr_pt(EdgeNode * p_edge, double z_level)
GnuPlot::ContourNode * GnuPlot::UpdateCntrPt(EdgeNode * p_edge, double z_level)
{
	ContourNode * p_cntr;
	double t = (z_level - p_edge->vertex[0]->Pt.z) / (p_edge->vertex[1]->Pt.z - p_edge->vertex[0]->Pt.z);
	// test if t is out of interval [0:1] (should not happen but who knows ...) 
	t = (t < 0.0 ? 0.0 : t);
	t = (t > 1.0 ? 1.0 : t);
	p_cntr = (ContourNode *)SAlloc::M(sizeof(ContourNode));
	p_cntr->x = p_edge->vertex[1]->Pt.x * t + p_edge->vertex[0]->Pt.x * (1 - t);
	p_cntr->y = p_edge->vertex[1]->Pt.y * t + p_edge->vertex[0]->Pt.y * (1 - t);
	return p_cntr;
}
//
// Simple routine to decide if two contour points are equal by
// calculating the relative error (< EPSILON).  
//
//static int fuzzy_equal(const ContourNode * pCntr1, const ContourNode * pCntr2)
int GnuPlot::FuzzyEqual(const ContourNode * pCntr1, const ContourNode * pCntr2) const
{
	double unit_x = fabs(_Cntr.Max.x - _Cntr.Min.x); /* reference */
	double unit_y = fabs(_Cntr.Max.y - _Cntr.Min.y);
	return ((fabs(pCntr1->x - pCntr2->x) < unit_x * EPSILON) && (fabs(pCntr1->y - pCntr2->y) < unit_y * EPSILON));
}
// 
// Generate the triangles.
// Returns the lists (edges & polys) via pointers to their heads.
// 
//static void gen_triangle(int num_isolines/* number of iso-lines input */, iso_curve * iso_lines/* iso-lines input */, PolyNode ** p_polys/* list of polygons output */,
    //EdgeNode ** p_edges/* list of edges output */)
void GnuPlot::GenTriangle(int num_isolines/* number of iso-lines input */, iso_curve * iso_lines/* iso-lines input */, PolyNode ** p_polys/* list of polygons output */,
	EdgeNode ** p_edges/* list of edges output */)
{
	int i, j, grid_x_max = iso_lines->p_count;
	EdgeNode * p_edge1;
	EdgeNode * p_edge2;
	EdgeNode * edge0;
	EdgeNode * edge1;
	EdgeNode * edge2;
	EdgeNode * pe_tail;
	EdgeNode * pe_tail2;
	EdgeNode * pe_temp;
	PolyNode * pp_tail;
	GpCoordinate * p_vrtx1;
	GpCoordinate * p_vrtx2;
	(*p_polys) = pp_tail = NULL; /* clear lists */
	(*p_edges) = pe_tail = NULL;
	p_vrtx1 = iso_lines->points; /* first row of vertices */
	p_edge1 = pe_tail = NULL; /* clear list of edges */
	// Generate edges of first row 
	for(j = 0; j < grid_x_max - 1; j++)
		AddEdge(p_vrtx1 + j, p_vrtx1 + j + 1, &p_edge1, &pe_tail);
	(*p_edges) = p_edge1; /* update main list */
	/*
	 * Combines vertices to edges and edges to triangles:
	 * ==================================================
	 * The edges are stored in the edge list, referenced by p_edges
	 * (pe_tail points on last edge).
	 *
	 * Temporary pointers:
	 * 1. p_edge2: Top horizontal edge list:      +-----------------------+ 2
	 * 2. p_tail : end of middle edge list:       |\  |\  |\  |\  |\  |\  |
	 *                                      |  \|  \|  \|  \|  \|  \|
	 * 3. p_edge1: Bottom horizontal edge list:   +-----------------------+ 1
	 *
	 * pe_tail2  : end of list beginning at p_edge2
	 * pe_temp   : position inside list beginning at p_edge1
	 * p_edges   : head of the master edge list (part of our output)
	 * p_vrtx1   : start of lower row of input vertices
	 * p_vrtx2   : start of higher row of input vertices
	 *
	 * The routine generates two triangle            Lower      Upper 1
	 * upper one and lower one:                     | \           ----
	 * (Nums. are edges order in polys)            0|   \1       0\   |2
	 * The polygons are stored in the polygon        ----           \ |
	 * list (*p_polys) (pp_tail points on             2
	 * last polygon).
	 *                                                  1
	 *                                             -----------
	 * In addition, the edge lists are updated -        | \   0     |
	 * each edge has two pointers on the two            |   \       |
	 * (one active if boundary) polygons which         0|1   0\1   0|1
	 * uses it. These two pointer to polygons           |       \   |
	 * are named: poly[0], poly[1]. The diagram         |    1    \ |
	 * on the right show how they are used for the       -----------
	 * upper and lower polygons (INNER_MESH polygons only).  0
	 */
	for(i = 1; i < num_isolines; i++) {
		// Read next column and gen. polys. 
		iso_lines = iso_lines->next;
		p_vrtx2 = iso_lines->points; /* next row of vertices */
		p_edge2 = pe_tail2 = NULL; /* clear top horizontal list */
		pe_temp = p_edge1; /* pointer in bottom list */
		/*
		 * Generate edges and triangles for next row:
		 */
		// generate first vertical edge 
		edge2 = AddEdge(p_vrtx1, p_vrtx2, p_edges, &pe_tail);
		for(j = 0; j < grid_x_max - 1; j++) {
			edge0 = edge2; // copy vertical edge for lower triangle 
			if(pe_temp && pe_temp->vertex[0] == p_vrtx1 + j) {
				// test lower edge 
				edge2 = pe_temp;
				pe_temp = pe_temp->next;
			}
			else {
				edge2 = NULL; // edge is undefined 
			}
			edge1 = AddEdge(p_vrtx1 + j + 1, p_vrtx2 + j, p_edges, &pe_tail); // generate diagonal edge 
			if(edge1)
				edge1->position = EdgeNode::DIAGONAL;
			AddPoly(edge0, edge1, edge2, p_polys, &pp_tail); // generate lower triangle 
			edge0 = edge1; // copy diagonal edge for upper triangle 
			edge1 = AddEdge(p_vrtx2 + j, p_vrtx2 + j + 1, &p_edge2, &pe_tail2); // generate upper edge 
			edge2 = AddEdge(p_vrtx1 + j + 1, p_vrtx2 + j + 1, p_edges, &pe_tail); // generate vertical edge 
			AddPoly(edge0, edge1, edge2, p_polys, &pp_tail); // generate upper triangle 
		}
		if(p_edge2) {
			// HBB 19991130 bugfix: if p_edge2 list is empty,
			// don't change p_edges list! Crashes by access
			// to NULL pointer pe_tail, the second time through, otherwise 
			if(*p_edges) { // Chain new edges to main list. 
				pe_tail->next = p_edge2;
				pe_tail = pe_tail2;
			}
			else {
				(*p_edges) = p_edge2;
				pe_tail = pe_tail2;
			}
		}
		// this row finished, move list heads up one row: 
		p_edge1 = p_edge2;
		p_vrtx1 = p_vrtx2;
	}
	// Update the boundary flag, saved in each edge, and update indexes: 
	pe_temp = (*p_edges);
	while(pe_temp) {
		if((!(pe_temp->poly[0])) || (!(pe_temp->poly[1])))
			(pe_temp->position) = EdgeNode::BOUNDARY;
		pe_temp = pe_temp->next;
	}
}
// 
// Generate new edge and append it to list, but only if both vertices are
// defined. The list is referenced by p_edge and pe_tail (p_edge points on
// first edge and pe_tail on last one).
// Note, the list may be empty (pe_edge==pe_tail==NULL) on entry and exit.
// 
//static EdgeNode * add_edge(GpCoordinate * point0,  /* 2 vertices input */ GpCoordinate * point1, EdgeNode ** p_edge/* pointers to edge list in/out */, EdgeNode ** pe_tail)
GnuPlot::EdgeNode * GnuPlot::AddEdge(GpCoordinate * point0,  /* 2 vertices input */ GpCoordinate * point1, EdgeNode ** p_edge/* pointers to edge list in/out */, EdgeNode ** pe_tail)
{
	EdgeNode * pe_temp = NULL;
#if 1
	if(point0->type == INRANGE && point1->type == INRANGE)
#else
	if(point0->type != UNDEFINED && point1->type != UNDEFINED)
#endif
	{
		pe_temp = (EdgeNode *)SAlloc::M(sizeof(EdgeNode));
		pe_temp->poly[0] = NULL; /* clear links           */
		pe_temp->poly[1] = NULL;
		pe_temp->vertex[0] = point0; // First vertex of edge. 
		pe_temp->vertex[1] = point1; // Second vertex of edge. 
		pe_temp->next = NULL;
		pe_temp->position = EdgeNode::INNER_MESH; // default position in mesh 
		if((*pe_tail)) {
			(*pe_tail)->next = pe_temp; /* Stick new record as last one. */
		}
		else {
			(*p_edge) = pe_temp; /* start new list if empty */
		}
		(*pe_tail) = pe_temp; /* continue to last record. */
	}
	return pe_temp; /* returns NULL, if no edge allocated */
}
// 
// Generate new triangle and append it to list, but only if all edges are defined.
// The list is referenced by p_poly and pp_tail (p_poly points on first polygon
// and pp_tail on last one).
// Note, the list may be empty (pe_ploy==pp_tail==NULL) on entry and exit.
// 
//static PolyNode * add_poly(EdgeNode * edge0, EdgeNode * edge1, EdgeNode * edge2/* 3 edges input */, PolyNode ** p_poly,
    //PolyNode ** pp_tail/* pointers to polygon list in/out */)
GnuPlot::PolyNode * GnuPlot::AddPoly(EdgeNode * edge0, EdgeNode * edge1, EdgeNode * edge2/* 3 edges input */, PolyNode ** p_poly,
	PolyNode ** pp_tail/* pointers to polygon list in/out */)
{
	PolyNode * pp_temp = NULL;
	if(edge0 && edge1 && edge2) {
		pp_temp = (PolyNode *)SAlloc::M(sizeof(PolyNode));
		pp_temp->edge[0] = edge0; /* First edge of triangle */
		pp_temp->edge[1] = edge1; /* Second one             */
		pp_temp->edge[2] = edge2; /* Third one              */
		pp_temp->next = NULL;
		if(edge0->poly[0]) /* update edge0 */
			edge0->poly[1] = pp_temp;
		else
			edge0->poly[0] = pp_temp;
		if(edge1->poly[0]) /* update edge1 */
			edge1->poly[1] = pp_temp;
		else
			edge1->poly[0] = pp_temp;
		if(edge2->poly[0]) /* update edge2 */
			edge2->poly[1] = pp_temp;
		else
			edge2->poly[0] = pp_temp;
		if((*pp_tail))  /* Stick new record as last one. */
			(*pp_tail)->next = pp_temp;
		else
			(*p_poly) = pp_temp; /* start new list if empty */
		(*pp_tail) = pp_temp; /* continue to last record. */
	}
	return pp_temp; /* returns NULL, if no edge allocated */
}
//
// Calls the (hopefully) desired interpolation/approximation routine.
//
//static void put_contour(ContourNode * p_cntr/* contour structure input */, double xx_min, double xx_max, double yy_min, double yy_max/* minimum/maximum values input */,
    //bool contr_isclosed/* contour line closed? (input) */)
void GnuPlot::PutContour(ContourNode * p_cntr/* contour structure input */, double xx_min, double xx_max, double yy_min, double yy_max/* minimum/maximum values input */,
	bool contr_isclosed/* contour line closed? (input) */)
{
	if(p_cntr) { // Nothing to do if it is empty contour. 
		switch(_Cntr.InterpKind) {
			case CONTOUR_KIND_LINEAR: // No interpolation/approximation. 
				PutContourNothing(p_cntr);
				break;
			case CONTOUR_KIND_CUBIC_SPL: // Cubic spline interpolation. 
				PutContourCubic(p_cntr, xx_min, xx_max, yy_min, yy_max, ChkContourKind(p_cntr, contr_isclosed));
				break;
			case CONTOUR_KIND_BSPLINE: // Bspline approximation. 
				PutContourBSpline(p_cntr, ChkContourKind(p_cntr, contr_isclosed));
				break;
		}
		FreeContour(p_cntr);
	}
}
//
// Simply puts contour coordinates in order with no interpolation or approximation.
//
//static void put_contour_nothing(ContourNode * p_cntr)
void GnuPlot::PutContourNothing(ContourNode * pCntr)
{
	while(pCntr) {
		AddCntrPoint(pCntr->x, pCntr->y);
		pCntr = pCntr->next;
	}
	EndCrntCntr();
}
//
// for some reason contours are never flagged as 'isclosed'
// if first point == last point, set flag accordingly
//
//static int chk_contour_kind(ContourNode * p_cntr, bool contr_isclosed)
int GnuPlot::ChkContourKind(ContourNode * p_cntr, bool contr_isclosed)
{
	bool current_contr_isclosed = contr_isclosed;
	if(!contr_isclosed) {
		ContourNode * pc_tail = p_cntr;
		while(pc_tail->next)
			pc_tail = pc_tail->next; /* Find last point. */
		// test if first and last point are equal 
		if(FuzzyEqual(pc_tail, p_cntr))
			current_contr_isclosed = TRUE;
	}
	return (current_contr_isclosed);
}
// 
// Generate a cubic spline curve through the points (x_i,y_i) which are
// stored in the linked list p_cntr.
// The spline is defined as a 2d-function s(t) = (x(t),y(t)), where the
// parameter t is the length of the linear stroke.
// 
//static void put_contour_cubic(ContourNode * p_cntr, double xx_min, double xx_max, double yy_min, double yy_max, bool contr_isclosed)
void GnuPlot::PutContourCubic(ContourNode * p_cntr, double xx_min, double xx_max, double yy_min, double yy_max, bool contr_isclosed)
{
	int num_pts = CountContour(p_cntr); // Number of points in contour. 
	ContourNode * pc_tail = p_cntr; // Find last point. 
	while(pc_tail->next)
		pc_tail = pc_tail->next;
	if(contr_isclosed) {
		// Test if first and last point are equal (should be) 
		if(!FuzzyEqual(pc_tail, p_cntr)) {
			pc_tail->next = p_cntr; /* Close contour list - make it circular. */
			num_pts++;
		}
	}
	{
		double * delta_t = (double *)SAlloc::M(num_pts * sizeof(double)); // Interval length t_{i+1}-t_i 
		double * d2x = (double *)SAlloc::M(num_pts * sizeof(double)); // Second derivatives x''(t_i), y''(t_i) 
		double * d2y = (double *)SAlloc::M(num_pts * sizeof(double));
		// Width and height of the grid is used as a unit length (2d-norm) 
		double unit_x = xx_max - xx_min; // To define norm (x,y)-plane 
		double unit_y = yy_max - yy_min;
		// FIXME HBB 20010121: 'zero' should not be used as an absolute figure to compare to data 
		unit_x = (unit_x > Gg.Zero ? unit_x : Gg.Zero); // should not be zero 
		unit_y = (unit_y > Gg.Zero ? unit_y : Gg.Zero);
		if(num_pts > 2) {
			//
			// Calculate second derivatives d2x[], d2y[] and interval lengths delta_t[]:
			//
			if(!GenCubicSpline(num_pts, p_cntr, d2x, d2y, delta_t, contr_isclosed, unit_x, unit_y)) {
				SAlloc::F(delta_t);
				SAlloc::F(d2x);
				SAlloc::F(d2y);
				if(contr_isclosed)
					pc_tail->next = NULL; /* Un-circular list */
				return;
			}
		}
		// If following (num_pts > 1) is TRUE then exactly 2 points in contour.  
		else if(num_pts > 1) {
			// set all second derivatives to zero, interval length to 1 
			d2x[0] = 0.0;
			d2y[0] = 0.0;
			d2x[1] = 0.0;
			d2y[1] = 0.0;
			delta_t[0] = 1.0;
		}
		else {                  /* Only one point ( ?? ) - ignore it. */
			SAlloc::F(delta_t);
			SAlloc::F(d2x);
			SAlloc::F(d2y);
			if(contr_isclosed)
				pc_tail->next = NULL; /* Un-circular list */
			return;
		}
		{
			// Calculate "num_intpol" interpolated values 
			int num_intpol = 1 + (num_pts - 1) * _Cntr.ContourPts; /* global: ContourPts */
			IntpCubicSpline(num_pts, p_cntr, d2x, d2y, delta_t, num_intpol);
			SAlloc::F(delta_t);
			SAlloc::F(d2x);
			SAlloc::F(d2y);
			if(contr_isclosed)
				pc_tail->next = NULL; /* Un-circular list */
			EndCrntCntr();
		}
	}
}
// 
// Find Bspline approximation for this data set.
// Uses global variable ContourPts to determine number of samples per
// interval, where the knot vector intervals are assumed to be uniform, and
// global variable contour_order for the order of Bspline to use.
// 
//static void put_contour_bspline(ContourNode * p_cntr, bool contr_isclosed)
void GnuPlot::PutContourBSpline(ContourNode * p_cntr, bool contr_isclosed)
{
	int order = _Cntr.ContourOrder - 1;
	int num_pts = CountContour(p_cntr); // Number of points in contour. 
	if(num_pts >= 2) { // Can't do nothing if empty or one points! 
		// Order must be less than number of points in curve - fix it if needed. */
		if(order > num_pts - 1)
			order = num_pts - 1;
		GenBSplineApprox(p_cntr, num_pts, order, contr_isclosed);
		EndCrntCntr();
	}
}
//
// Free all elements in the contour list.
//
//static void free_contour(ContourNode * p_cntr)
void GnuPlot::FreeContour(ContourNode * pCntr)
{
	while(pCntr) {
		ContourNode * pc_temp = pCntr;
		pCntr = pCntr->next;
		SAlloc::F(pc_temp);
	}
}
//
// Counts number of points in contour.
//
//static int count_contour(ContourNode * pCntr)
int GnuPlot::CountContour(const ContourNode * pCntr) const
{
	int count = 0;
	while(pCntr) {
		count++;
		pCntr = pCntr->next;
	}
	return count;
}
// 
// The following two procedures solve the special linear system which arise
// in cubic spline interpolation. If x is assumed cyclic ( x[i]=x[n+i] ) the
// equations can be written as (i=0,1,...,n-1):
//     m[i][0] * x[i-1] + m[i][1] * x[i] + m[i][2] * x[i+1] = b[i] .
// In matrix notation one gets M * x = b, where the matrix M is tridiagonal
// with additional elements in the upper right and lower left position:
//   m[i][0] = M_{i,i-1}  for i=1,2,...,n-1    and    m[0][0] = M_{0,n-1} ,
//   m[i][1] = M_{i, i }  for i=0,1,...,n-1
//   m[i][2] = M_{i,i+1}  for i=0,1,...,n-2    and    m[n-1][2] = M_{n-1,0}.
// M should be symmetric (m[i+1][0]=m[i][2]) and positive definite.
// The size of the system is given in n (n>=1).
// 
// In the first procedure the Cholesky decomposition M = C^T * D * C
// (C is upper triangle with unit diagonal, D is diagonal) is calculated.
// Return TRUE if decomposition exist.
// 
static int solve_cubic_1(tri_diag m[], int n)
{
	int i;
	double m_ij, m_n, m_nn, d;
	if(n < 1)
		return FALSE; /* Dimension should be at least 1 */
	d = m[0][1]; /* D_{0,0} = M_{0,0} */
	if(d <= 0.)
		return FALSE; /* M (or D) should be positive definite */
	m_n = m[0][0]; /* M_{0,n-1}  */
	m_nn = m[n-1][1]; /* M_{n-1,n-1} */
	for(i = 0; i < n - 2; i++) {
		m_ij = m[i][2]; /* M_{i,1}  */
		m[i][2] = m_ij / d; /* C_{i,i+1} */
		m[i][0] = m_n / d; /* C_{i,n-1} */
		m_nn -= m[i][0] * m_n; /* to get C_{n-1,n-1} */
		m_n = -m[i][2] * m_n; /* to get C_{i+1,n-1} */
		d = m[i+1][1] - m[i][2] * m_ij; /* D_{i+1,i+1} */
		if(d <= 0.)
			return FALSE; /* Elements of D should be positive */
		m[i+1][1] = d;
	}
	if(n >= 2) {            /* Complete last column */
		m_n += m[n - 2][2]; /* add M_{n-2,n-1} */
		m[n - 2][0] = m_n / d; /* C_{n-2,n-1} */
		m[n-1][1] = d = m_nn - m[n - 2][0] * m_n; /* D_{n-1,n-1} */
		if(d <= 0.)
			return FALSE;
	}
	return TRUE;
}
// 
// The second procedure solves the linear system, with the Choleky
// decomposition calculated above (in m[][]) and the right side b given
// in x[]. The solution x overwrites the right side in x[].
// 
static void solve_cubic_2(tri_diag m[], double x[], int n)
{
	int i;
	// Division by transpose of C : b = C^{-T} * b 
	double x_n = x[n-1];
	for(i = 0; i < n - 2; i++) {
		x[i+1] -= m[i][2] * x[i]; /* C_{i,i+1} * x_{i} */
		x_n -= m[i][0] * x[i]; /* C_{i,n-1} * x_{i} */
	}
	if(n >= 2)
		x[n-1] = x_n - m[n - 2][0] * x[n - 2]; /* C_{n-2,n-1} * x_{n-1} */
	// Division by D: b = D^{-1} * b 
	for(i = 0; i < n; i++)
		x[i] /= m[i][1];
	// Division by C: b = C^{-1} * b 
	x_n = x[n-1];
	if(n >= 2)
		x[n - 2] -= m[n - 2][0] * x_n; /* C_{n-2,n-1} * x_{n-1} */
	for(i = n - 3; i >= 0; i--) {
		/* C_{i,i+1} * x_{i+1} + C_{i,n-1} * x_{n-1} */
		x[i] -= m[i][2] * x[i+1] + m[i][0] * x_n;
	}
}
// 
// Find second derivatives (x''(t_i),y''(t_i)) of cubic spline interpolation
// through list of points (x_i,y_i). The parameter t is calculated as the
// length of the linear stroke. The number of points must be at least 3.
// Note: For closed contours the first and last point must be equal.
// 
//static int gen_cubic_spline(int num_pts/* Number of points (num_pts>=3), input */, ContourNode * p_cntr/* List of points (x(t_i),y(t_i)), input */,
    //double d2x[], double d2y[]/* Second derivatives (x''(t_i),y''(t_i)), output */,
    //double delta_t[]/* List of interval lengths t_{i+1}-t_{i}, output */,
    //bool contr_isclosed/* Closed or open contour?, input  */, double unit_x, double unit_y/* Unit length in x and y (norm=1), input */)
int GnuPlot::GenCubicSpline(int num_pts/* Number of points (num_pts>=3), input */, ContourNode * p_cntr/* List of points (x(t_i),y(t_i)), input */,
	double d2x[], double d2y[]/* Second derivatives (x''(t_i),y''(t_i)), output */,
	double delta_t[]/* List of interval lengths t_{i+1}-t_{i}, output */,
	bool contr_isclosed/* Closed or open contour?, input  */, double unit_x, double unit_y/* Unit length in x and y (norm=1), input */)
{
	int n, i;
	double norm;
	tri_diag * m = (tri_diag *)SAlloc::M(num_pts * sizeof(tri_diag)); /* The tri-diagonal matrix is saved here. */
	// 
	// Calculate first differences in (d2x[i], d2y[i]) and interval lengths in delta_t[i]:
	// 
	ContourNode * pc_temp = p_cntr;
	for(i = 0; i < num_pts - 1; i++) {
		d2x[i] = pc_temp->next->x - pc_temp->x;
		d2y[i] = pc_temp->next->y - pc_temp->y;
		//
		// The norm of a linear stroke is calculated in "normal coordinates"
		// and used as interval length:
		//
		delta_t[i] = sqrt(SQR(d2x[i] / unit_x) + SQR(d2y[i] / unit_y));
		d2x[i] /= delta_t[i]; /* first difference, with unit norm: */
		d2y[i] /= delta_t[i]; /* || (d2x[i], d2y[i]) || = 1      */
		pc_temp = pc_temp->next;
	}
	// 
	// Setup linear system:  m * x = b
	// 
	n = num_pts - 2; /* Without first and last point */
	if(contr_isclosed) {
		// First and last points must be equal for closed contours 
		delta_t[num_pts-1] = delta_t[0];
		d2x[num_pts-1] = d2x[0];
		d2y[num_pts-1] = d2y[0];
		n++; // Add last point (= first point) 
	}
	for(i = 0; i < n; i++) {
		// Matrix M, mainly tridiagonal with cyclic second index ("j = j+n mod n") 
		m[i][0] = delta_t[i]; /* Off-diagonal element M_{i,i-1} */
		m[i][1] = 2. * (delta_t[i] + delta_t[i+1]); /* M_{i,i} */
		m[i][2] = delta_t[i+1]; /* Off-diagonal element M_{i,i+1} */
		// Right side b_x and b_y 
		d2x[i] = (d2x[i+1] - d2x[i]) * 6.0;
		d2y[i] = (d2y[i+1] - d2y[i]) * 6.0;
		/*
		 * If the linear stroke shows a cusps of more than 90 degree, the right
		 * side is reduced to avoid oscillations in the spline:
		 */
		norm = sqrt(SQR(d2x[i] / unit_x) + SQR(d2y[i] / unit_y)) / 8.5;
		if(norm > 1.) {
			d2x[i] /= norm;
			d2y[i] /= norm;
			// The first derivative will not be continuous 
		}
	}
	if(!contr_isclosed) {
		// Third derivative is set to zero at both ends 
		m[0][1] += m[0][0]; // M_{0,0}
		m[0][0] = 0.0; // M_{0,n-1}   
		m[n-1][1] += m[n-1][2]; // M_{n-1,n-1} 
		m[n-1][2] = 0.0; // M_{n-1,0}   
	}
	// Solve linear systems for d2x[] and d2y[] 
	if(solve_cubic_1(m, n)) { /* Calculate Cholesky decomposition */
		solve_cubic_2(m, d2x, n); /* solve M * d2x = b_x */
		solve_cubic_2(m, d2y, n); /* solve M * d2y = b_y */
	}
	else {                  /* Should not happen, but who knows ... */
		SAlloc::F(m);
		return FALSE;
	}
	// Shift all second derivatives one place right and abdate end points 
	for(i = n; i > 0; i--) {
		d2x[i] = d2x[i-1];
		d2y[i] = d2y[i-1];
	}
	if(contr_isclosed) {
		d2x[0] = d2x[n];
		d2y[0] = d2y[n];
	}
	else {
		d2x[0] = d2x[1]; /* Third derivative is zero in */
		d2y[0] = d2y[1]; /* first and last interval */
		d2x[n + 1] = d2x[n];
		d2y[n + 1] = d2y[n];
	}
	SAlloc::F(m);
	return TRUE;
}
// 
// Calculate interpolated values of the spline function (defined via p_cntr
// and the second derivatives d2x[] and d2y[]). The number of tabulated
// values is n. On an equidistant grid n_intpol values are calculated.
// 
//static void intp_cubic_spline(int n, ContourNode * p_cntr, double d2x[], double d2y[], double delta_t[], int n_intpol)
void GnuPlot::IntpCubicSpline(int n, ContourNode * p_cntr, double d2x[], double d2y[], double delta_t[], int n_intpol)
{
	double t, t_skip;
	double x0, x1, x, y0, y1, y;
	double d, hx, dx0, dx01, hy, dy0, dy01;
	int i;
	double t_max = 0.0; // The length of the total interval 
	for(i = 0; i < n - 1; i++)
		t_max += delta_t[i];
	// The distance between interpolated points 
	t_skip = (1.0 - 1e-7) * t_max / (n_intpol - 1);
	t = 0.0; // Parameter value 
	x1 = p_cntr->x;
	y1 = p_cntr->y;
	AddCntrPoint(x1, y1); /* First point. */
	t += t_skip;
	for(i = 0; i < n - 1; i++) {
		p_cntr = p_cntr->next;
		d = delta_t[i]; /* Interval length */
		x0 = x1;
		y0 = y1;
		x1 = p_cntr->x;
		y1 = p_cntr->y;
		hx = (x1 - x0) / d;
		hy = (y1 - y0) / d;
		dx0 = (d2x[i+1] + 2 * d2x[i]) / 6.0;
		dy0 = (d2y[i+1] + 2 * d2y[i]) / 6.0;
		dx01 = (d2x[i+1] - d2x[i]) / (6.0 * d);
		dy01 = (d2y[i+1] - d2y[i]) / (6.0 * d);
		while(t <= delta_t[i]) { // t in current interval ? 
			x = x0 + t * (hx + (t - d) * (dx0 + t * dx01));
			y = y0 + t * (hy + (t - d) * (dy0 + t * dy01));
			AddCntrPoint(x, y); // next point. 
			t += t_skip;
		}
		t -= delta_t[i]; // Parameter t relative to start of next interval 
	}
}
// 
// Routine to get the i knot from uniform knot vector. The knot vector
// might be float (Knot(i) = i) or open (where the first and last "order"
// knots are equal). contr_isclosed determines knot kind - open contour means
// open knot vector, and closed contour selects float knot vector.
// Note the knot vector is not exist and this routine simulates it existence
// Also note the indexes for the knot vector starts from 0.
// 
static double fetch_knot(bool contr_isclosed, int num_of_points, int order, int i)
{
	if(!contr_isclosed) {
		if(i <= order)
			return 0.0;
		else if(i <= num_of_points)
			return (double)(i - order);
		else
			return (double)(num_of_points - order);
	}
	else
		return (double)i;
}
// 
// Generate a Bspline curve defined by all the points given in linked list p:
// Algorithm: using deBoor algorithm
// Note: if Curvekind is open contour than Open end knot vector is assumed,
//   else (closed contour) Float end knot vector is assumed.
// It is assumed that num_of_points is at least 2, and order of Bspline is less
// than num_of_points!
// 
//static void gen_bspline_approx(ContourNode * p_cntr, int num_of_points, int order, bool contr_isclosed)
void GnuPlot::GenBSplineApprox(ContourNode * p_cntr, int num_of_points, int order, bool contr_isclosed)
{
	int knot_index = 0, pts_count = 1;
	double dt, t, next_t, t_min, t_max, x, y;
	ContourNode * pc_temp = p_cntr, * pc_tail = NULL;
	/* If the contour is Closed one we must update few things:
	 * 1. Make the list temporary circular, so we can close the contour.
	 * 2. Update num_of_points - increase it by "order-1" so contour will be
	 *    closed. This will evaluate order more sections to close it!
	 */
	if(contr_isclosed) {
		pc_tail = p_cntr;
		while(pc_tail->next)
			pc_tail = pc_tail->next; /* Find last point. */
		// test if first and last point are equal 
		if(FuzzyEqual(pc_tail, p_cntr)) {
			/* Close contour list - make it circular. */
			pc_tail->next = p_cntr->next;
			num_of_points += order - 1;
		}
		else {
			pc_tail->next = p_cntr;
			num_of_points += order;
		}
	}
	// Find first (t_min) and last (t_max) t value to eval: 
	t = t_min = fetch_knot(contr_isclosed, num_of_points, order, order);
	t_max = fetch_knot(contr_isclosed, num_of_points, order, num_of_points);
	next_t = t_min + 1.0;
	knot_index = order;
	dt = 1.0 / _Cntr.ContourPts; // Number of points per one section. 
	while(t < t_max) {
		if(t > next_t) {
			pc_temp = pc_temp->next; // Next order ctrl. pt. to blend. 
			knot_index++;
			next_t += 1.0;
		}
		EvalBSpline(t, pc_temp, num_of_points, order, knot_index, contr_isclosed, &x, &y); /* Next pt. */
		AddCntrPoint(x, y);
		pts_count++;
		// As we might have some real number round off problems we do      
		// the last point outside the loop                                 
		if(pts_count == _Cntr.ContourPts * (num_of_points - order) + 1)
			break;
		t += dt;
	}
	// Now do the last point 
	EvalBSpline(t_max - EPSILON, pc_temp, num_of_points, order, knot_index, contr_isclosed, &x, &y);
	AddCntrPoint(x, y); /* Complete the contour. */
	if(contr_isclosed) /* Update list - un-circular it. */
		pc_tail->next = NULL;
}
// 
// The routine to evaluate the B-spline value at point t using knot vector
// from function fetch_knot(), and the control points p_cntr.
// Returns (x, y) of approximated B-spline. Note that p_cntr points on the
// first control point to blend with. The B-spline is of order order.
// 
//static void eval_bspline(double t, ContourNode * p_cntr, int num_of_points, int order, int j, bool contr_isclosed, double * x, double * y)
void GnuPlot::EvalBSpline(double t, ContourNode * p_cntr, int num_of_points, int order, int j, bool contr_isclosed, double * x, double * y)
{
	int i, p;
	double ti, tikp; /* Copy p_cntr into it to make it faster. */
	double * dx = (double *)SAlloc::M((order + j) * sizeof(double));
	double * dy = (double *)SAlloc::M((order + j) * sizeof(double));
	// Set the dx/dy - [0] iteration step, control points (p==0 iterat.): 
	for(i = j - order; i <= j; i++) {
		dx[i] = p_cntr->x;
		dy[i] = p_cntr->y;
		p_cntr = p_cntr->next;
	}
	for(p = 1; p <= order; p++) {   /* Iteration (b-spline level) counter. */
		for(i = j; i >= j - order + p; i--) { /* Control points indexing. */
			ti = fetch_knot(contr_isclosed, num_of_points, order, i);
			tikp = fetch_knot(contr_isclosed, num_of_points, order, i + order + 1 - p);
			if(ti == tikp) { /* Should not be a problems but how knows... */
			}
			else {
				dx[i] = dx[i] * (t - ti) / (tikp - ti) + /* Calculate x. */ dx[i-1] * (tikp - t) / (tikp - ti);
				dy[i] = dy[i] * (t - ti) / (tikp - ti) + /* Calculate y. */ dy[i-1] * (tikp - t) / (tikp - ti);
			}
		}
	}
	*x = dx[j];
	*y = dy[j];
	SAlloc::F(dx);
	SAlloc::F(dy);
}
