/* GNUPLOT - util3d.c */

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

/*
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * 3.6 - split graph3d.c into graph3d.c (graph),
 *                            util3d.c (intersections, etc)
 *                            hidden3d.c (hidden-line removal code)
 *
 */
#include <gnuplot.h>
#pragma hdrstop

/* EAM DEBUG - moved these here from axis.h */
#define AXIS_ACTUAL_MIN(axis) MIN(GpGg[axis].Range.upp, GpGg[axis].Range.low)
#define AXIS_ACTUAL_MAX(axis) MAX(GpGg[axis].Range.upp, GpGg[axis].Range.low)

/* Prototypes for local functions */
static void mat_unit(transform_matrix mat);

static void mat_unit(transform_matrix mat)
{
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			if(i == j)
				mat[i][j] = 1.0;
			else
				mat[i][j] = 0.0;
}

void mat_scale(double sx, double sy, double sz, transform_matrix mat)
{
	mat_unit(mat);          /* Make it unit matrix. */
	mat[0][0] = sx;
	mat[1][1] = sy;
	mat[2][2] = sz;
}

void mat_rot_x(double teta, transform_matrix mat)
{
	teta *= DEG2RAD;
	double cos_teta = cos(teta);
	double sin_teta = sin(teta);
	mat_unit(mat);          /* Make it unit matrix. */
	mat[1][1] = cos_teta;
	mat[1][2] = -sin_teta;
	mat[2][1] = sin_teta;
	mat[2][2] = cos_teta;
}

void mat_rot_z(double teta, transform_matrix mat)
{
	teta *= DEG2RAD;
	const double cos_teta = cos(teta);
	const double sin_teta = sin(teta);
	mat_unit(mat);          /* Make it unit matrix. */
	mat[0][0] = cos_teta;
	mat[0][1] = -sin_teta;
	mat[1][0] = sin_teta;
	mat[1][1] = cos_teta;
}

/* Multiply two transform_matrix. Result can be one of two operands. */
void mat_mult(transform_matrix mat_res, transform_matrix mat1, transform_matrix mat2)
{
	int i, j, k;
	transform_matrix mat_res_temp;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++) {
			mat_res_temp[i][j] = 0;
			for(k = 0; k < 4; k++)
				mat_res_temp[i][j] += mat1[i][k] * mat2[k][j];
		}
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			mat_res[i][j] = mat_res_temp[i][j];
}

/* single edge intersection algorithm */
/* Given two points, one inside and one outside the plot, return
 * the point where an edge of the plot intersects the line segment defined
 * by the two points.
 */
//void edge3d_intersect(GpCoordinate * p1, GpCoordinate * p2, double * ex, double * ey, double * ez) /* the point where it crosses an edge */
void GpGadgets::Edge3DIntersect(GpCoordinate * pPt1, GpCoordinate * pPt2, double * pEx, double * pEy, double * pEz) /* the point where it crosses an edge */
{
	int count;
	double ix = pPt1->x;
	double iy = pPt1->y;
	double iz = pPt1->z;
	double ox = pPt2->x;
	double oy = pPt2->y;
	double oz = pPt2->z;
	double x, y, z;         /* possible intersection point */
	if(pPt1->type == INRANGE) {
		/* swap points around so that ix/ix/iz are INRANGE and ox/oy/oz are OUTRANGE */
		x = ix;
		ix = ox;
		ox = x;
		y = iy;
		iy = oy;
		oy = y;
		z = iz;
		iz = oz;
		oz = z;
	}
	/* nasty degenerate cases, effectively drawing to an infinity point (?)
	   cope with them here, so don't process them as a "real" OUTRANGE point

	   If more than one coord is -GPVL, then can't ratio the "infinities"
	   so drop out by returning false */

	count = 0;
	if(ox == -GPVL)
		count++;
	if(oy == -GPVL)
		count++;
	if(oz == -GPVL)
		count++;
	/* either doesn't pass through 3D volume *or*
	   can't ratio infinities to get a direction to draw line, so return the INRANGE point */
	if(count > 1) {
		*pEx = ix;
		*pEy = iy;
		*pEz = iz;
		return;
	}
	if(count == 1) {
		*pEx = ix;
		*pEy = iy;
		*pEz = iz;
		if(ox == -GPVL) {
			*pEx = AXIS_ACTUAL_MIN(FIRST_X_AXIS);
			return;
		}
		if(oy == -GPVL) {
			*pEy = AXIS_ACTUAL_MIN(FIRST_Y_AXIS);
			return;
		}
		/* obviously oz is -GPVL and (ox != -GPVL && oy != -GPVL) */
		*pEz = AXIS_ACTUAL_MIN(FIRST_Z_AXIS);
		return;
	}
	/*
	 * Can't have case (ix == ox && iy == oy && iz == oz) as one point
	 * is INRANGE and one point is OUTRANGE.
	 */
	if(ix == ox) {
		if(iy == oy) {
			/* line parallel to z axis */
			/* assume iy in yrange, && ix in xrange */
			*pEx = ix; /* == ox */
			*pEy = iy; /* == oy */
			if(inrange(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), iz, oz))
				*pEz = AXIS_ACTUAL_MAX(FIRST_Z_AXIS);
			else if(inrange(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), iz, oz))
				*pEz = AXIS_ACTUAL_MIN(FIRST_Z_AXIS);
			else {
				IntErrorNoCaret("error in edge3d_intersect");
			}
			return;
		}
		if(iz == oz) {
			/* line parallel to y axis */
			/* assume iz in zrange && ix in xrange */
			*pEx = ix; /* == ox */
			*pEz = iz; /* == oz */
			if(inrange(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), iy, oy))
				*pEy = AXIS_ACTUAL_MAX(FIRST_Y_AXIS);
			else if(inrange(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), iy, oy))
				*pEy = AXIS_ACTUAL_MIN(FIRST_Y_AXIS);
			else {
				IntErrorNoCaret("error in edge3d_intersect");
			}
			return;
		}
		/* nasty 2D slanted line in a yz plane */
#define INTERSECT_PLANE(cut, axis, eff, eff_axis, res_x, res_y, res_z)	\
	do {								\
		if(inrange(cut, i ## axis, o ## axis) && cut != i ## axis && cut != o ## axis) { \
			eff = (cut - i ## axis) * ((o ## eff - i ## eff) / (o ## axis - i ## axis)) + i ## eff; \
			if(InAxisRange(eff, eff_axis)) { \
				*pEx = res_x;					    \
				*pEy = res_y;					    \
				*pEz = res_z;					    \
				return;						    \
			}							\
		}							    \
	} while(0)
		INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), y, z, FIRST_Z_AXIS, ix, AXIS_ACTUAL_MIN(FIRST_Y_AXIS), z);
		INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), y, z, FIRST_Z_AXIS, ix, AXIS_ACTUAL_MAX(FIRST_Y_AXIS), z);
		INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), z, y, FIRST_Y_AXIS, ix, y, AXIS_ACTUAL_MIN(FIRST_Z_AXIS));
		INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), z, y, FIRST_Y_AXIS, ix, y, AXIS_ACTUAL_MAX(FIRST_Z_AXIS));
	} /* if(ix == ox) */

	if(iy == oy) {
		/* already checked case (ix == ox && iy == oy) */
		if(oz == iz) {
			/* line parallel to x axis */
			/* assume inrange(iz) && inrange(iy) */
			*pEy = iy; /* == oy */
			*pEz = iz; /* == oz */
			if(inrange(AXIS_ACTUAL_MAX(FIRST_X_AXIS), ix, ox))
				*pEx = AXIS_ACTUAL_MAX(FIRST_X_AXIS);
			else if(inrange(AXIS_ACTUAL_MIN(FIRST_X_AXIS), ix, ox))
				*pEx = AXIS_ACTUAL_MIN(FIRST_X_AXIS);
			else {
				IntErrorNoCaret("error in edge3d_intersect");
			}
			return;
		}
		/* nasty 2D slanted line in an xz plane */
		INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_X_AXIS), x, z, FIRST_Z_AXIS, AXIS_ACTUAL_MIN(FIRST_X_AXIS), iy, z);
		INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_X_AXIS), x, z, FIRST_Z_AXIS, AXIS_ACTUAL_MAX(FIRST_X_AXIS), iy, z);
		INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, x, iy, AXIS_ACTUAL_MIN(FIRST_Z_AXIS));
		INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, x, iy, AXIS_ACTUAL_MAX(FIRST_Z_AXIS));
	} /* if(iy==oy) */

	if(iz == oz) {
		/* already checked cases (ix == ox && iz == oz) and (iy == oy
		   && iz == oz) */

		/* 2D slanted line in an xy plane */

		/* assume inrange(oz) */

		INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, AXIS_ACTUAL_MIN(FIRST_X_AXIS), y, iz);
		INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, AXIS_ACTUAL_MAX(FIRST_X_AXIS), y, iz);
		INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, x, AXIS_ACTUAL_MIN(FIRST_Y_AXIS), iz);
		INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, x, AXIS_ACTUAL_MAX(FIRST_Y_AXIS), iz);
	} /* if(iz==oz) */
#undef INTERSECT_PLANE

	/* really nasty general slanted 3D case */

#define INTERSECT_DIAG(cut, axis, eff, eff_axis, eff2, eff2_axis, res_x, res_y, res_z) \
	do {								\
		if(inrange(cut, i ## axis, o ## axis) && cut != i ## axis && cut != o ## axis) { \
			eff = (cut - i ## axis) * ((o ## eff - i ## eff) / (o ## axis - i ## axis)) + i ## eff; \
			eff2 = (cut - i ## axis) * ((o ## eff2 - i ## eff2) / (o ## axis - i ## axis)) + i ## eff2; \
			if(InAxisRange(eff, eff_axis) && InAxisRange(eff2, eff2_axis)) { \
				*pEx = res_x;					    \
				*pEy = res_y;					    \
				*pEz = res_z;					    \
				return;						    \
			}							\
		}							    \
	} while(0)
	INTERSECT_DIAG(AXIS_ACTUAL_MIN(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, z, FIRST_Z_AXIS, AXIS_ACTUAL_MIN(FIRST_X_AXIS), y, z);
	INTERSECT_DIAG(AXIS_ACTUAL_MAX(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, z, FIRST_Z_AXIS, AXIS_ACTUAL_MAX(FIRST_X_AXIS), y, z);
	INTERSECT_DIAG(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, z, FIRST_Z_AXIS, x, AXIS_ACTUAL_MIN(FIRST_Y_AXIS), z);
	INTERSECT_DIAG(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, z, FIRST_Z_AXIS, x, AXIS_ACTUAL_MAX(FIRST_Y_AXIS), z);
	INTERSECT_DIAG(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, y, FIRST_Y_AXIS, x, y, AXIS_ACTUAL_MIN(FIRST_Z_AXIS));
	INTERSECT_DIAG(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, y, FIRST_Y_AXIS, x, y, AXIS_ACTUAL_MAX(FIRST_Z_AXIS));

#undef INTERSECT_DIAG

	/* If we reach here, the inrange point is on the edge, and
	 * the line segment from the outrange point does not cross any
	 * other edges to get there. In this case, we return the inrange
	 * point as the 'edge' intersection point. This will basically draw
	 * line.
	 */
	*pEx = ix;
	*pEy = iy;
	*pEz = iz;
	return;
}

/* double edge intersection algorithm */
/* Given two points, both outside the plot, return
 * the points where an edge of the plot intersects the line segment defined
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return false when there is nothing
 * to draw (zero intersections), and true when there is something to
 * draw (the one-point case is a degenerate of the two-point case and we do
 * not distinguish it - we draw it anyway).
 */
/* any intersection? */
//bool two_edge3d_intersect(GpCoordinate * p0, GpCoordinate * p1, double * lx, double * ly, double * lz) 
/* lx[2], ly[2], lz[2]: points where it crosses edges */
bool GpGadgets::TwoEdge3DIntersect(GpCoordinate * p0, GpCoordinate * p1, double * lx, double * ly, double * lz) 
{
	double ix = p0->x;
	double iy = p0->y;
	double iz = p0->z;
	double ox = p1->x;
	double oy = p1->y;
	double oz = p1->z;
	double t[6];
	double t_min, t_max;
	//
	// nasty degenerate cases, effectively drawing to an infinity point (?)
	// cope with them here, so don't process them as a "real" OUTRANGE point
	// 
	// If more than one coord is -GPVL, then can't ratio the "infinities"
	// so drop out by returning false 
	// 
	int    count = 0;
	if(ix == -GPVL)
		count++;
	if(ox == -GPVL)
		count++;
	if(iy == -GPVL)
		count++;
	if(oy == -GPVL)
		count++;
	if(iz == -GPVL)
		count++;
	if(oz == -GPVL)
		count++;
	// either doesn't pass through 3D volume *or*
	// can't ratio infinities to get a direction to draw line, so simply return(false)
	if(count > 1) {
		return false;
	}
	else if(ox == -GPVL || ix == -GPVL) {
		if(ix == -GPVL) {
			// swap points so ix/iy/iz don't have a -GPVL component 
			Exchange(&ix, &ox);
			Exchange(&iy, &oy);
			Exchange(&iz, &oz);
		}
		// check actually passes through the 3D graph volume 
		if(ix > AxA[FIRST_X_AXIS].Range.upp && InAxisRange2(iy, FIRST_Y_AXIS, iz, FIRST_Z_AXIS)) {
			lx[0] = AxA[FIRST_X_AXIS].Range.low;
			ly[0] = iy;
			lz[0] = iz;
			lx[1] = AxA[FIRST_X_AXIS].Range.upp;
			ly[1] = iy;
			lz[1] = iz;
			return (true);
		}
		else {
			return (false);
		}
	}
	else if(oy == -GPVL || iy == -GPVL) {
		if(iy == -GPVL) {
			// swap points so ix/iy/iz don't have a -GPVL component 
			Exchange(&ix, &ox);
			Exchange(&iy, &oy);
			Exchange(&iz, &oz);
		}
		// check actually passes through the 3D graph volume 
		if(iy > AxA[FIRST_Y_AXIS].Range.upp && InAxisRange2(ix, FIRST_X_AXIS, iz, FIRST_Z_AXIS)) {
			lx[0] = ix;
			ly[0] = AxA[FIRST_Y_AXIS].Range.low;
			lz[0] = iz;
			lx[1] = ix;
			ly[1] = AxA[FIRST_Y_AXIS].Range.upp;
			lz[1] = iz;
			return (true);
		}
		else {
			return (false);
		}
	}
	else if(oz == -GPVL || iz == -GPVL) {
		if(iz == -GPVL) {
			// swap points so ix/iy/iz don't have a -GPVL component 
			Exchange(&ix, &ox);
			Exchange(&iy, &oy);
			Exchange(&iz, &oz);
		}
		// check actually passes through the 3D graph volume 
		if(iz > AxA[FIRST_Z_AXIS].Range.upp && InAxisRange2(ix, FIRST_X_AXIS, iy, FIRST_Y_AXIS)) {
			lx[0] = ix;
			ly[0] = iy;
			lz[0] = AxA[FIRST_Z_AXIS].Range.low;
			lx[1] = ix;
			ly[1] = iy;
			lz[1] = AxA[FIRST_Z_AXIS].Range.upp;
			return (true);
		}
		else {
			return (false);
		}
	}
	//
	// Quick outcode tests on the 3d graph volume
	//
	// test z coord first --- most surface OUTRANGE points generated
	// between AxA[FIRST_Z_AXIS].min and baseplane (i.e. when ticslevel is non-zero)
	//
	else if(MAX(iz, oz) < AxA[FIRST_Z_AXIS].Range.low || MIN(iz, oz) > AxA[FIRST_Z_AXIS].Range.upp)
		return (false);
	else if(MAX(ix, ox) < AxA[FIRST_X_AXIS].Range.low || MIN(ix, ox) > AxA[FIRST_X_AXIS].Range.upp)
		return (false);
	else if(MAX(iy, oy) < AxA[FIRST_Y_AXIS].Range.low || MIN(iy, oy) > AxA[FIRST_Y_AXIS].Range.upp)
		return (false);
	/* Special horizontal/vertical, etc. cases are checked and
	 * remaining slant lines are checked separately.
	 *
	 * The slant line intersections are solved using the parametric
	 * form of the equation for a line, since if we test x/y/z min/max
	 * planes explicitly then e.g. a line passing through a corner
	 * point (x_min,y_min,z_min) actually intersects all 3 planes and
	 * hence further tests would be required to anticipate this and
	 * similar situations. */

	// Can have case (ix == ox && iy == oy && iz == oz) as both points OUTRANGE
	else if(ix == ox && iy == oy && iz == oz) {
		return (false); // but as only define single outrange point, can't intersect 3D graph volume
	}
	else if(ix == ox) {
		if(iy == oy) {
			/* line parallel to z axis */

			/* x and y coords must be in range, and line must span
			 * both FIRST_Z_AXIS->min and ->max.
			 *
			 * note that spanning FIRST_Z_AXIS->min implies spanning ->max as both points OUTRANGE */
			if(!InAxisRange2(ix, FIRST_X_AXIS, iy, FIRST_Y_AXIS)) {
				return (false);
			}
			else if(inrange(AxA[FIRST_Z_AXIS].Range.low, iz, oz)) {
				lx[0] = ix;
				ly[0] = iy;
				lz[0] = AxA[FIRST_Z_AXIS].Range.low;
				lx[1] = ix;
				ly[1] = iy;
				lz[1] = AxA[FIRST_Z_AXIS].Range.upp;
				return (true);
			}
			else
				return (false);
		}
		if(iz == oz) {
			// line parallel to y axis 
			if(!InAxisRange2(ix, FIRST_X_AXIS, iz, FIRST_Z_AXIS)) {
				return (false);
			}
			else if(inrange(AxA[FIRST_Y_AXIS].Range.low, iy, oy)) {
				lx[0] = ix;
				ly[0] = AxA[FIRST_Y_AXIS].Range.low;
				lz[0] = iz;
				lx[1] = ix;
				ly[1] = AxA[FIRST_Y_AXIS].Range.upp;
				lz[1] = iz;
				return (true);
			}
			else
				return (false);
		}
		// nasty 2D slanted line in a yz plane 
		if(!InAxisRange(ox, FIRST_X_AXIS))
			return (false);
		else {
			t[0] = fscale(AxA[FIRST_Y_AXIS].Range.low, iy, oy);
			t[1] = fscale(AxA[FIRST_Y_AXIS].Range.upp, iy, oy);
			if(t[0] > t[1]) {
				Exchange(t+0, t+1);
			}
			t[2] = fscale(AxA[FIRST_Z_AXIS].Range.low, iz, oz);
			t[3] = fscale(AxA[FIRST_Z_AXIS].Range.upp, iz, oz);
			if(t[2] > t[3]) {
				Exchange(t+2, t+3);
			}
			t_min = MAX(MAX(t[0], t[2]), 0.0);
			t_max = MIN(MIN(t[1], t[3]), 1.0);
			if(t_min > t_max)
				return (false);
			else {
				lx[0] = ix;
				ly[0] = iy + t_min * (oy - iy);
				lz[0] = iz + t_min * (oz - iz);
				lx[1] = ix;
				ly[1] = iy + t_max * (oy - iy);
				lz[1] = iz + t_max * (oz - iz);
				// Can only have 0 or 2 intersection points -- only need test one coord
				return (InAxisRange2(ly[0], FIRST_Y_AXIS, lz[0], FIRST_Z_AXIS)) ? true : false;
			}
		}
	}
	else if(iy == oy) {
		// already checked case (ix == ox && iy == oy) 
		if(oz == iz) {
			// line parallel to x axis 
			if(!InAxisRange2(iy, FIRST_Y_AXIS, iz, FIRST_Z_AXIS)) {
				return (false);
			}
			else if(inrange(AxA[FIRST_X_AXIS].Range.low, ix, ox)) {
				lx[0] = AxA[FIRST_X_AXIS].Range.low;
				ly[0] = iy;
				lz[0] = iz;
				lx[1] = AxA[FIRST_X_AXIS].Range.upp;
				ly[1] = iy;
				lz[1] = iz;
				return (true);
			}
			else
				return (false);
		}
		// nasty 2D slanted line in an xz plane 
		else if(!InAxisRange(oy, FIRST_Y_AXIS))
			return (false);
		else {
			t[0] = fscale(AxA[FIRST_X_AXIS].Range.low, ix, ox);
			t[1] = fscale(AxA[FIRST_X_AXIS].Range.upp, ix, ox);
			if(t[0] > t[1])
				Exchange(t+0, t+1);
			t[2] = fscale(AxA[FIRST_Z_AXIS].Range.low, iz, oz);
			t[3] = fscale(AxA[FIRST_Z_AXIS].Range.upp, iz, oz);
			if(t[2] > t[3])
				Exchange(t+2, t+3);
			t_min = MAX(MAX(t[0], t[2]), 0.0);
			t_max = MIN(MIN(t[1], t[3]), 1.0);
			if(t_min > t_max)
				return (false);
			else {
				lx[0] = ix + t_min * (ox - ix);
				ly[0] = iy;
				lz[0] = iz + t_min * (oz - iz);
				lx[1] = ix + t_max * (ox - ix);
				ly[1] = iy;
				lz[1] = iz + t_max * (oz - iz);
				// Can only have 0 or 2 intersection points -- only need test one coord
				return InAxisRange2(lx[0], FIRST_X_AXIS, lz[0], FIRST_Z_AXIS);
			}
		}
	}
	else if(iz == oz) {
		// already checked cases (ix == ox && iz == oz) and (iy == oy && iz == oz) 
		// nasty 2D slanted line in an xy plane 
		if(!InAxisRange(oz, FIRST_Z_AXIS))
			return (false);
		else {
			t[0] = fscale(AxA[FIRST_X_AXIS].Range.low, ix, ox);
			t[1] = fscale(AxA[FIRST_X_AXIS].Range.upp, ix, ox);
			if(t[0] > t[1])
				Exchange(t+0, t+1);
			t[2] = fscale(AxA[FIRST_Y_AXIS].Range.low, iy, oy);
			t[3] = fscale(AxA[FIRST_Y_AXIS].Range.upp, iy, oy);
			if(t[2] > t[3])
				Exchange(t+2, t+3);
			t_min = MAX(MAX(t[0], t[2]), 0.0);
			t_max = MIN(MIN(t[1], t[3]), 1.0);
			if(t_min > t_max)
				return (false);
			lx[0] = ix + t_min * (ox - ix);
			ly[0] = iy + t_min * (oy - iy);
			lz[0] = iz;
			lx[1] = ix + t_max * (ox - ix);
			ly[1] = iy + t_max * (oy - iy);
			lz[1] = iz;
			// Can only have 0 or 2 intersection points -- only need test one coord
			return InAxisRange2(lx[0], FIRST_X_AXIS, ly[0], FIRST_Y_AXIS);
		}
	}
	else {
		// really nasty general slanted 3D case 
		/*
		Solve parametric equation

		(ix, iy, iz) + t (diff_x, diff_y, diff_z)

		where 0.0 <= t <= 1.0 and

		diff_x = (ox - ix);
		diff_y = (oy - iy);
		diff_z = (oz - iz);
		*/
		t[0] = fscale(AxA[FIRST_X_AXIS].Range.low, ix, ox);
		t[1] = fscale(AxA[FIRST_X_AXIS].Range.upp, ix, ox);
		if(t[0] > t[1])
			Exchange(t+0, t+1);
		t[2] = fscale(AxA[FIRST_Y_AXIS].Range.low, iy, oy);
		t[3] = fscale(AxA[FIRST_Y_AXIS].Range.upp, iy, oy);
		if(t[2] > t[3])
			Exchange(t+2, t+3);
		t[4] = (iz == oz) ? 0.0 : fscale(AxA[FIRST_Z_AXIS].Range.low, iz, oz);
		t[5] = (iz == oz) ? 1.0 : fscale(AxA[FIRST_Z_AXIS].Range.upp, iz, oz);
		if(t[4] > t[5])
			Exchange(t+4, t+5);
		t_min = MAX(MAX(t[0], t[2]), MAX(t[4], 0.0));
		t_max = MIN(MIN(t[1], t[3]), MIN(t[5], 1.0));
		if(t_min > t_max)
			return (false);
		else {
			lx[0] = ix + t_min * (ox - ix);
			ly[0] = iy + t_min * (oy - iy);
			lz[0] = iz + t_min * (oz - iz);
			lx[1] = ix + t_max * (ox - ix);
			ly[1] = iy + t_max * (oy - iy);
			lz[1] = iz + t_max * (oz - iz);
			// Can only have 0 or 2 intersection points -- only need test one coord
			return InAxisRange3(lx[0], FIRST_X_AXIS, ly[0], FIRST_Y_AXIS, lz[0], FIRST_Z_AXIS);
		}
	}
}

double GpGadgets::MapX3D(double v)
{
	return ((v - GetX().Range.low) * Scale3d.x + Center3d.x - 1.0);
}
double GpGadgets::MapY3D(double v)
{
	return ((v - GetY().Range.low) * Scale3d.y + Center3d.y - 1.0);
}
double GpGadgets::MapZ3D(double v)
{
	return ((v - floor_z) * Scale3d.z + Center3d.z - 1.0);
}

//
// Performs transformation from 'user coordinates' to a normalized
//vector in 'graph coordinates' (-1..1 in all three directions).
//
//void map3d_xyz(double x, double y, double z /* user coordinates */, GpVertex * out)
void GpGadgets::Map3DXYZ(double x, double y, double z /* user coordinates */, GpVertex * out)
{
	double V[4], Res[4]; // Homogeneous coords. vectors
	// Normalize object space to -1..1 
	V[0] = MapX3D(x); // map_x3d(x);
	V[1] = MapY3D(y); // map_y3d(y);
	V[2] = MapZ3D(z); // map_z3d(z);
	V[3] = 1.0;
	// Res[] = V[] * trans_mat[][] (uses row-vectors)
	for(int i = 0; i < 4; i++) {
		Res[i] = trans_mat[3][i]; // V[3] is 1. anyway
		for(int j = 0; j < 3; j++)
			Res[i] += V[j] * trans_mat[j][i];
	}
	if(Res[3] == 0)
		Res[3] = 1.0e-5;
	out->x = Res[0] / Res[3];
	out->y = Res[1] / Res[3];
	out->z = Res[2] / Res[3];
	// store z for later color calculation 
	out->real_z = z;
	out->label = NULL;
}
//
// Function to map from user 3D space to normalized 'camera' view
// space, and from there directly to terminal coordinates 
//
//void map3d_xy(double x, double y, double z, int * xt, int * yt)
void GpGadgets::Map3DXY(double x, double y, double z, int * xt, int * yt)
{
	double xtd, ytd;
	Map3DXY(x, y, z, &xtd, &ytd);
	*xt = (int)xtd;
	*yt = (int)ytd;
}

//void map3d_xy_double(double x, double y, double z, double * xt, double * yt)
void GpGadgets::Map3DXY(double x, double y, double z, double * xt, double * yt)
{
	GpVertex v;
	Map3DXYZ(x, y, z, &v);
	uint   ux, uy;
	TermCoord(v, ux, uy);
	*xt = ux;
	*yt = uy;
}
//
// HBB 20020313: New routine, broken out of draw3d_point, to be used
// to output a single point without any checks for hidden3d 
//
void GpGadgets::Draw3DPointUnconditional(GpTermEntry * pT, const GpVertex * pV, lp_style_type * pLp)
{
	uint x, y;
	TermCoord(*pV, x, y);
	// Jul 2010 EAM - is it safe to overwrite like this? Make a copy instead? 
	pLp->pm3d_color.value = pV->real_z;
	ApplyLpProperties(pT, pLp);
	if(!ClipPoint(x, y))
		pT->point(x, y, pLp->p_type);
}
//
// Moved this upward, to make optional inlining in draw3d_line easier for compilers 
// HBB 20021128: removed GP_INLINE qualifier to avoid MSVC++ silliness 
//void draw3d_line_unconditional(GpVertex * v1, GpVertex * v2, lp_style_type * lp, t_colorspec color)
void GpGadgets::Draw3DLineUnconditional(GpTermEntry * pT, const GpVertex * pV1, const GpVertex * pV2, lp_style_type * pLp, t_colorspec cs)
{
	// HBB 20020312: pV2 can be NULL, if this call is coming from
	// draw_line_hidden. --> redirect to point drawing routine 
	if(!pV2) {
		Draw3DPointUnconditional(pT, pV1, pLp);
	}
	else {
		uint x1, y1, x2, y2;
		lp_style_type ls = *pLp;
		TermCoord(*pV1, x1, y1);
		TermCoord(*pV2, x2, y2);
		// Replace original color with the one passed in 
		ls.pm3d_color = cs;
		// Color by Z value 
		if(ls.pm3d_color.type == TC_Z)
			ls.pm3d_color.value = (pV1->real_z + pV2->real_z) * 0.5;
		//
		// This interrupts the polyline, messing up dash patterns. 
		// Skip it if the caller indicates that the line properties are 
		// already set by passing in color.type = TC_DEFAULT 
		//
		if(cs.type != TC_DEFAULT)
			ApplyLpProperties(pT, &ls);
		// Support for hidden3d VECTOR mode with arrowheads 
		if(pLp->p_type == PT_ARROWHEAD)
			DrawClipArrow(pT, x1, y1, x2, y2, END_HEAD);
		else if(pLp->p_type == PT_BACKARROW)
			DrawClipArrow(pT, x1, y1, x2, y2, BACKHEAD);
		else
			DrawClipLine(pT, x1, y1, x2, y2);
	}
}

void GpGadgets::Draw3DLine(GpTermEntry * pT, GpVertex * pV1, GpVertex * pV2, lp_style_type * pLp)
{
	// hidden3d routine can't work if no surface was drawn at all
	if(hidden3d && draw_surface)
		DrawLineHidden(pT, pV1, pV2, pLp);
	else 
		Draw3DLineUnconditional(pT, pV1, pV2, pLp, pLp->pm3d_color);
}
//
// HBB 20000621: new routine, to allow for hiding point symbols behind the surface
//
void GpGadgets::Draw3DPoint(GpTermEntry * pT, GpVertex & rV, lp_style_type * pLp)
{
	// hidden3d routine can't work if no surface was drawn at all 
	if(hidden3d && draw_surface)
		DrawLineHidden(pT, &rV, NULL, pLp); // Draw vertex as a zero-length edge 
	else
		Draw3DPointUnconditional(pT, &rV, pLp);
}
//
// HBB NEW 20031218: tools for drawing polylines in 3D with a semantic
// like term->move() and term->vector() 
//
// Previous points 3D position 
//static GpVertex polyline3d_previous_vertex; // @global

//void polyline3d_start(GpVertex * v1)
void GpGadgets::PolyLine3DStart(GpTermEntry * pT, GpVertex & rV1)
{
	uint x1, y1;
	PolyLine3DPreviousVertex = rV1;
	if(!hidden3d || !draw_surface) {
		// EAM - This may now be unneeded. But I'm not sure. 
		//       Perhaps the hidden3d code needs the move.   
		TermCoord(rV1, x1, y1);
		pT->move(x1, y1);
	}
}

//void polyline3d_next(GpVertex * v2, lp_style_type * lp)
void GpGadgets::PolyLine3DNext(GpTermEntry * pT, GpVertex & rV2, lp_style_type * pLp)
{
	// v5: Indicate that line properties are already set so that draw3d_*
	//     routines do not mess up dash patterns by resetting them       
	t_colorspec nochange; // = DEFAULT_COLORSPEC;
	nochange.SetDefault();
	if(hidden3d && draw_surface)
		DrawLineHidden(pT, &PolyLine3DPreviousVertex, &rV2, pLp);
	else
		Draw3DLineUnconditional(pT, &PolyLine3DPreviousVertex, &rV2, pLp, nochange);
	PolyLine3DPreviousVertex = rV2;
}

