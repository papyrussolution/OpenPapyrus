// GNUPLOT - vplot.c 
// Copyright Ethan A Merritt 2019
//
/*
 * This file implements 3D plotting directly from a voxel grid
 *      splot $voxelgrid with {dots|points} {above <iso_level>}
 *	splot $voxelgrid with isosurface level <iso_level>
 * For point or dot plots
 *      points are only drawn if voxel value > iso_level (defaults to 0)
 * For isosurface plots
 *	the isosurface is generated for value == iso_level
 */
#include <gnuplot.h>
#pragma hdrstop

#ifdef VOXEL_GRID_SUPPORT

/*            Data structures for tessellation
 *            ===============================
 * We offer a choice of two tessellation tables.
 * One is Heller's table originally derived for use with marching cubes.
 * It contains triangles only.
 * The other contains a mix of quadrangles and triangles.
 * Near a complicated fold in the surface the quadrangles are an imperfect
 * approximation of the triangular tessellation.  However for most smooth
 * surfaces the reduced number of facets from using quadrangles makes the
 * pm3d rendering look cleaner.
 */
#include "marching_cubes.h"
#include "qt_table.h"

//static int    scaled_offset[8][3]; // local copy of vertex offsets from voxel corner, scaled by downsampling 
//static double intersection[12][3]; // the fractional index intersection along each of the cubes's 12 edges 
//static t_voxel cornervalue[8]; // working copy of the corner values for the current cube 
//
// local prototypes 
//
//static void vertex_interp(int edge_no, int start, int end, t_voxel isolevel);
//static void tessellate_one_cube(GpSurfacePoints * plot, int ix, int iy, int iz);
//
// splot $vgrid with {dots|points} {above <value>}
//
//void vplot_points(GpSurfacePoints * pPlot, double level)
void GnuPlot::VPlotPoints(GpTermEntry * pTerm, GpSurfacePoints * pPlot, double level)
{
	int ix, iy, iz;
	double vx, vy, vz;
	const vgrid * vgrid = pPlot->vgrid;
	//GpTermEntry * t = term;
	const int N = vgrid->size;
	int x, y;
	const int downsample = pPlot->lp_properties.p_interval;
	// dots or points only 
	if(pPlot->lp_properties.PtType == PT_CHARACTER)
		pPlot->lp_properties.PtType = -1;
	if(pPlot->lp_properties.PtType == PT_VARIABLE)
		pPlot->lp_properties.PtType = -1;
	// Set whatever we can that applies to every point in the loop 
	if(pPlot->lp_properties.pm3d_color.type == TC_RGB)
		SetRgbColorConst(term, pPlot->lp_properties.pm3d_color.lt);
	for(ix = 0; ix < N; ix++) {
		for(iy = 0; iy < N; iy++) {
			for(iz = 0; iz < N; iz++) {
				// The pointinterval property can be used to downsample 
				if((downsample > 0) && (ix % downsample || iy % downsample || iz % downsample))
					continue;
				const int index = ix + iy * N + iz * N*N;
				const t_voxel * voxel = &vgrid->vdata[index];
				if(*voxel <= level)
					continue;
				// vx, vy, vz are the true coordinates of this voxel 
				vx = vgrid->vxmin + ix * vgrid->vxdelta;
				vy = vgrid->vymin + iy * vgrid->vydelta;
				vz = vgrid->vzmin + iz * vgrid->vzdelta;

				if(jitter.spread > 0) {
					vx += jitter.spread * vgrid->vxdelta * ((double)(rand()/(double)RAND_MAX ) - 0.5);
					vy += jitter.spread * vgrid->vydelta * ((double)(rand()/(double)RAND_MAX ) - 0.5);
					vz += jitter.spread * vgrid->vzdelta * ((double)(rand()/(double)RAND_MAX ) - 0.5);
				}
				Map3D_XY(vx, vy, vz, &x, &y);
				// the usual variable color array cannot be used for voxel data 
				// but we can use the voxel value itself as a palette index     
				if(pPlot->lp_properties.pm3d_color.type == TC_Z)
					set_color(pTerm, Cb2Gray(*voxel));
				// This code is also used for "splot ... with dots" 
				if(pPlot->plot_style == DOTS)
					(pTerm->point)(pTerm, x, y, -1);
				// The normal case 
				else if(pPlot->lp_properties.PtType >= 0)
					(pTerm->point)(pTerm, x, y, pPlot->lp_properties.PtType);
			}
		}
	}
}
// 
// splot $vgrid with isosurface level <value>
//
//void vplot_isosurface(GpSurfacePoints * pPlot, int downsample)
void GnuPlot::VPlotIsoSurface(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int downsample)
{
	int i, j, k;
	int N = pPlot->vgrid->size;
	// Apply down-sampling, if any, to the vertex offsets 
	if(downsample > 1)
		downsample = fceili((double)N / 76.0);
	SETMAX(downsample, 1);
	for(i = 0; i < 8; i++) {
		for(j = 0; j < 3; j++)
			_VP.ScaledOffset[i][j] = downsample * vertex_offset[i][j];
	}
	// These initializations are normally done in pm3d_plot()
	// isosurfaces do not use that code path.
	if(pm3d_shade.strength > 0)
		pm3d_init_lighting_model();
	for(i = 0; i < N - downsample; i += downsample) {
		for(j = 0; j < N - downsample; j += downsample) {
			for(k = 0; k < N - downsample; k += downsample) {
				TessellateOneCube(pTerm, pPlot, i, j, k);
			}
		}
	}
}
// 
// tessellation algorithm applied to a single voxel.
// ix, iy, iz are the indices of the corner nearest [xmin, ymin, zmin].
// We will work in index space and convert back to actual graph coordinates
// when we have found the triangles (if any) that result from intersections
// of the isosurface with this voxel.
// 
//static void tessellate_one_cube(GpSurfacePoints * pPlot, int ix, int iy, int iz)
void GnuPlot::TessellateOneCube(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int ix, int iy, int iz)
{
	struct vgrid * vgrid = pPlot->vgrid;
	t_voxel isolevel = static_cast<t_voxel>(pPlot->iso_level);
	int N = vgrid->size;
	int ivertex, iedge, it;
	int corner_flags;               /* bit field */
	int edge_flags;                 /* bit field */
	// Make a local copy of the values at the cube corners 
	for(ivertex = 0; ivertex < 8; ivertex++) {
		int cx = ix + _VP.ScaledOffset[ivertex][0];
		int cy = iy + _VP.ScaledOffset[ivertex][1];
		int cz = iz + _VP.ScaledOffset[ivertex][2];
		_VP.CornerValue[ivertex] = vgrid->vdata[cx + cy*N + cz*N*N];
	}
	// Flag which vertices are inside the surface and which are outside 
	corner_flags = 0;
	if(_VP.CornerValue[0] < isolevel) corner_flags |= 1;
	if(_VP.CornerValue[1] < isolevel) corner_flags |= 2;
	if(_VP.CornerValue[2] < isolevel) corner_flags |= 4;
	if(_VP.CornerValue[3] < isolevel) corner_flags |= 8;
	if(_VP.CornerValue[4] < isolevel) corner_flags |= 16;
	if(_VP.CornerValue[5] < isolevel) corner_flags |= 32;
	if(_VP.CornerValue[6] < isolevel) corner_flags |= 64;
	if(_VP.CornerValue[7] < isolevel) corner_flags |= 128;
	// Look up which edges are affected by this corner pattern 
	edge_flags = cube_edge_flags[corner_flags];
	// If no edges are affected (surface does not intersect voxel) we're done 
	if(edge_flags == 0)
		return;
	/*
	 * Find the intersection point on each affected edge.
	 * Store in intersection[edge_no][i] as fractional (non-integral) indices.
	 * vertex_interp( edge_no, start_corner, end_corner, isolevel )
	 */
	if(edge_flags &    1) VertexInterp( 0, 0, 1, isolevel);
	if(edge_flags &    2) VertexInterp( 1, 1, 2, isolevel);
	if(edge_flags &    4) VertexInterp( 2, 2, 3, isolevel);
	if(edge_flags &    8) VertexInterp( 3, 3, 0, isolevel);
	if(edge_flags &   16) VertexInterp( 4, 4, 5, isolevel);
	if(edge_flags &   32) VertexInterp( 5, 5, 6, isolevel);
	if(edge_flags &   64) VertexInterp( 6, 6, 7, isolevel);
	if(edge_flags &  128) VertexInterp( 7, 7, 4, isolevel);
	if(edge_flags &  256) VertexInterp( 8, 0, 4, isolevel);
	if(edge_flags &  512) VertexInterp( 9, 1, 5, isolevel);
	if(edge_flags & 1024) VertexInterp(10, 2, 6, isolevel);
	if(edge_flags & 2048) VertexInterp(11, 3, 7, isolevel);
	//
	// Convert the content of intersection[][] from fractional indices to plot coordinates
	//
	for(iedge = 0; iedge < 12; iedge++) {
		_VP.Intersection[iedge][0] = vgrid->vxmin + ((double)ix + _VP.Intersection[iedge][0]) * vgrid->vxdelta;
		_VP.Intersection[iedge][1] = vgrid->vymin + ((double)iy + _VP.Intersection[iedge][1]) * vgrid->vydelta;
		_VP.Intersection[iedge][2] = vgrid->vzmin + ((double)iz + _VP.Intersection[iedge][2]) * vgrid->vzdelta;
	}
	if(isosurface_options.tessellation == 0) {
		//
		// Draw a mixture of quadrangles and triangles
		//
		for(it = 0; it < 3; it++) {
			gpdPoint quad[4]; /* The structure expected by gnuplot's pm3d */
			if(qt_table[corner_flags][4*it] < 0)
				break;
			ivertex = qt_table[corner_flags][4*it]; /* first vertex */
			quad[0].x = _VP.Intersection[ivertex][0];
			quad[0].y = _VP.Intersection[ivertex][1];
			quad[0].z = _VP.Intersection[ivertex][2];
			ivertex = qt_table[corner_flags][4*it+1]; /* second */
			quad[1].x = _VP.Intersection[ivertex][0];
			quad[1].y = _VP.Intersection[ivertex][1];
			quad[1].z = _VP.Intersection[ivertex][2];
			ivertex = qt_table[corner_flags][4*it+2]; /* third */
			quad[2].x = _VP.Intersection[ivertex][0];
			quad[2].y = _VP.Intersection[ivertex][1];
			quad[2].z = _VP.Intersection[ivertex][2];
			// 4th vertex == -1 indicates a triangle 
			// repeat the 3rd vertex to treat it as a degenerate quadrangle 
			if(qt_table[corner_flags][4*it+3] >= 0)
				ivertex = qt_table[corner_flags][4*it+3]; /* fourth */
			quad[3].x = _VP.Intersection[ivertex][0];
			quad[3].y = _VP.Intersection[ivertex][1];
			quad[3].z = _VP.Intersection[ivertex][2];
			// Color choice 
			quad[0].c = pPlot->lp_properties.pm3d_color.lt;
			// Debugging aid: light up all facets of the same class 
			if(debug > 0 && debug == corner_flags)
				quad[0].c = 6+it;
			// Hand off this facet to the pm3d code 
			Pm3DAddQuadrangle(pTerm, pPlot, quad);
		}
	}
	else {
		/*
		 * Draw the triangles from a purely triangular tessellation.
		 * There can be up to four per voxel cube.
		 */
		for(it = 0; it < 4; it++) {
			gpdPoint quad[4]; /* The structure expected by gnuplot's pm3d */
			if(triangle_table[corner_flags][3*it] < 0)
				break;
			ivertex = triangle_table[corner_flags][3*it]; /* first vertex */
			quad[0].x = _VP.Intersection[ivertex][0];
			quad[0].y = _VP.Intersection[ivertex][1];
			quad[0].z = _VP.Intersection[ivertex][2];
			ivertex = triangle_table[corner_flags][3*it+1]; /* second */
			quad[1].x = _VP.Intersection[ivertex][0];
			quad[1].y = _VP.Intersection[ivertex][1];
			quad[1].z = _VP.Intersection[ivertex][2];
			ivertex = triangle_table[corner_flags][3*it+2]; /* third */
			quad[2].x = _VP.Intersection[ivertex][0];
			quad[2].y = _VP.Intersection[ivertex][1];
			quad[2].z = _VP.Intersection[ivertex][2];
			// pm3d always wants a quadrangle, so repeat the 3rd vertex 
			quad[3] = quad[2];
			// Color choice 
			quad[0].c = pPlot->lp_properties.pm3d_color.lt;
			// Hand off this triangle to the pm3d code 
			Pm3DAddQuadrangle(pTerm, pPlot, quad);
		}
	}
}

//static void vertex_interp(int edge_no, int start, int end, t_voxel isolevel)
void GnuPlot::VertexInterp(int edgeNo, int start, int end, t_voxel isolevel)
{
	double fracindex;
	for(int i = 0; i < 3; i++) {
		if(vertex_offset[end][i] == vertex_offset[start][i])
			fracindex = 0.0;
		else
			fracindex = (_VP.ScaledOffset[end][i] - _VP.ScaledOffset[start][i]) * (isolevel - _VP.CornerValue[start]) / (_VP.CornerValue[end] - _VP.CornerValue[start]);
		_VP.Intersection[edgeNo][i] = _VP.ScaledOffset[start][i] + fracindex;
	}
}

#endif /* VOXEL_GRID_SUPPORT */

#ifndef VOXEL_GRID_SUPPORT
//void vplot_points(GpSurfacePoints * plot, double level) 
void GnuPlot::VPlotPoints(GpTermEntry * pTerm, GpSurfacePoints * pPlot, double level)
{
}
//void vplot_isosurface(GpSurfacePoints * plot, int downsample) 
void GnuPlot::VPlotIsoSurface(GpTermEntry * pTerm, GpSurfacePoints * pPlot, int downsample)
{
}
#endif
