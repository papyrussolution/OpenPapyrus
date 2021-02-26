// GNUPLOT - util3d.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
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

#define AXIS_ACTUAL_MIN(axis) AxS[axis].GetActualMin()
#define AXIS_ACTUAL_MAX(axis) AxS[axis].GetActualMax()

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
	double cos_teta = cos(teta);
	double sin_teta = sin(teta);
	mat_unit(mat);          /* Make it unit matrix. */
	mat[0][0] = cos_teta;
	mat[0][1] = -sin_teta;
	mat[1][0] = sin_teta;
	mat[1][1] = cos_teta;
}
//
// Multiply two transform_matrix. Result can be one of two operands. 
//
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

#define IN_AXIS_RANGE(val, axis) AxS[axis].InRange(val)
// 
// single edge intersection algorithm */
// Given two points, one inside and one outside the plot, return
// the point where an edge of the plot intersects the line segment defined
// by the two points.
// 
//void edge3d_intersect(coordinate * p1, coordinate * p2, double * ex, double * ey, double * ez) /* the point where it crosses an edge */
void GnuPlot::Edge3DIntersect(const GpCoordinate * p1, const GpCoordinate * p2, double * ex, double * ey, double * ez/* the point where it crosses an edge */)
{
	int count;
	double ix = p1->x;
	double iy = p1->y;
	double iz = p1->z;
	double ox = p2->x;
	double oy = p2->y;
	double oz = p2->z;
	double x, y, z;         /* possible intersection point */
	if(p1->type == INRANGE) {
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
	// nasty degenerate cases, effectively drawing to an infinity point (?)
	// cope with them here, so don't process them as a "real" OUTRANGE point
	// 
	// If more than one coord is -VERYLARGE, then can't ratio the "infinities"
	// so drop out by returning FALSE 
	count = 0;
	if(ox == -VERYLARGE)
		count++;
	if(oy == -VERYLARGE)
		count++;
	if(oz == -VERYLARGE)
		count++;
	// either doesn't pass through 3D volume *or*
	// can't ratio infinities to get a direction to draw line, so return the INRANGE point 
	if(count > 1) {
		*ex = ix;
		*ey = iy;
		*ez = iz;
		return;
	}
	else if(count == 1) {
		*ex = ix;
		*ey = iy;
		*ez = iz;
		if(ox == -VERYLARGE)
			*ex = AXIS_ACTUAL_MIN(FIRST_X_AXIS);
		else if(oy == -VERYLARGE)
			*ey = AXIS_ACTUAL_MIN(FIRST_Y_AXIS);
		else
			*ez = AXIS_ACTUAL_MIN(FIRST_Z_AXIS); // obviously oz is -VERYLARGE and (ox != -VERYLARGE && oy != -VERYLARGE) 
		return;
	}
	else {
		// 
		// Can't have case (ix == ox && iy == oy && iz == oz) as one point
		// is INRANGE and one point is OUTRANGE.
		// 
		if(ix == ox) {
			if(iy == oy) {
				// line parallel to z axis 
				// assume iy in yrange, && ix in xrange 
				*ex = ix; /* == ox */
				*ey = iy; /* == oy */
				if(inrange(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), iz, oz))
					*ez = AXIS_ACTUAL_MAX(FIRST_Z_AXIS);
				else if(inrange(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), iz, oz))
					*ez = AXIS_ACTUAL_MIN(FIRST_Z_AXIS);
				else {
					IntError(NO_CARET, "error in edge3d_intersect");
				}
				return;
			}
			if(iz == oz) {
				// line parallel to y axis 
				// assume iz in zrange && ix in xrange 
				*ex = ix; /* == ox */
				*ez = iz; /* == oz */
				if(inrange(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), iy, oy))
					*ey = AXIS_ACTUAL_MAX(FIRST_Y_AXIS);
				else if(inrange(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), iy, oy))
					*ey = AXIS_ACTUAL_MIN(FIRST_Y_AXIS);
				else {
					IntError(NO_CARET, "error in edge3d_intersect");
				}
				return;
			}
			// nasty 2D slanted line in a yz plane 
	#define INTERSECT_PLANE(cut, axis, eff, eff_axis, res_x, res_y, res_z)  \
		do {                                                            \
			if(inrange(cut, i ## axis, o ## axis) && cut != i ## axis && cut != o ## axis) { \
				eff = (cut - i ## axis) * ((o ## eff - i ## eff) / (o ## axis - i ## axis)) + i ## eff; \
				if(IN_AXIS_RANGE(eff, eff_axis)) {                     \
					*ex = res_x;                                        \
					*ey = res_y;                                        \
					*ez = res_z;                                        \
					return;                                             \
				}                                                       \
			}                                                           \
		} while(0)
			INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), y, z, FIRST_Z_AXIS, ix, AXIS_ACTUAL_MIN(FIRST_Y_AXIS), z);
			INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), y, z, FIRST_Z_AXIS, ix, AXIS_ACTUAL_MAX(FIRST_Y_AXIS), z);
			INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), z, y, FIRST_Y_AXIS, ix, y, AXIS_ACTUAL_MIN(FIRST_Z_AXIS));
			INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), z, y, FIRST_Y_AXIS,ix, y, AXIS_ACTUAL_MAX(FIRST_Z_AXIS));
		} /* if (ix == ox) */
		if(iy == oy) {
			/* already checked case (ix == ox && iy == oy) */
			if(oz == iz) {
				/* line parallel to x axis */

				/* assume inrange(iz) && inrange(iy) */
				*ey = iy; /* == oy */
				*ez = iz; /* == oz */
				if(inrange(AXIS_ACTUAL_MAX(FIRST_X_AXIS), ix, ox))
					*ex = AXIS_ACTUAL_MAX(FIRST_X_AXIS);
				else if(inrange(AXIS_ACTUAL_MIN(FIRST_X_AXIS), ix, ox))
					*ex = AXIS_ACTUAL_MIN(FIRST_X_AXIS);
				else {
					IntError(NO_CARET, "error in edge3d_intersect");
				}
				return;
			}
			// nasty 2D slanted line in an xz plane 
			INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_X_AXIS), x, z, FIRST_Z_AXIS, AXIS_ACTUAL_MIN(FIRST_X_AXIS), iy, z);
			INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_X_AXIS), x, z, FIRST_Z_AXIS, AXIS_ACTUAL_MAX(FIRST_X_AXIS), iy, z);
			INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, x, iy, AXIS_ACTUAL_MIN(FIRST_Z_AXIS));
			INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, x, iy, AXIS_ACTUAL_MAX(FIRST_Z_AXIS));
		} /* if(iy==oy) */
		if(iz == oz) {
			// already checked cases (ix == ox && iz == oz) and (iy == oy && iz == oz) 
			// 2D slanted line in an xy plane 
			// assume inrange(oz) 
			INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, AXIS_ACTUAL_MIN(FIRST_X_AXIS), y, iz);
			INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, AXIS_ACTUAL_MAX(FIRST_X_AXIS), y, iz);
			INTERSECT_PLANE(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, x, AXIS_ACTUAL_MIN(FIRST_Y_AXIS), iz);
			INTERSECT_PLANE(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, x, AXIS_ACTUAL_MAX(FIRST_Y_AXIS), iz);
		} /* if(iz==oz) */
	#undef INTERSECT_PLANE
		// really nasty general slanted 3D case 
	#define INTERSECT_DIAG(cut, axis, eff, eff_axis, eff2, eff2_axis, res_x, res_y, res_z) \
		do {                                                            \
			if(inrange(cut, i ## axis, o ## axis) && cut != i ## axis && cut != o ## axis) { \
				eff = (cut - i ## axis) * ((o ## eff - i ## eff) / (o ## axis - i ## axis)) + i ## eff; \
				eff2 = (cut - i ## axis) * ((o ## eff2 - i ## eff2) / (o ## axis - i ## axis)) + i ## eff2; \
				if(IN_AXIS_RANGE(eff, eff_axis) && IN_AXIS_RANGE(eff2, eff2_axis)) { \
					*ex = res_x;                                        \
					*ey = res_y;                                        \
					*ez = res_z;                                        \
					return;                                             \
				}                                                       \
			}                                                           \
		} while(0)
		INTERSECT_DIAG(AXIS_ACTUAL_MIN(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, z, FIRST_Z_AXIS, AXIS_ACTUAL_MIN(FIRST_X_AXIS), y, z);
		INTERSECT_DIAG(AXIS_ACTUAL_MAX(FIRST_X_AXIS), x, y, FIRST_Y_AXIS, z, FIRST_Z_AXIS, AXIS_ACTUAL_MAX(FIRST_X_AXIS), y, z);
		INTERSECT_DIAG(AXIS_ACTUAL_MIN(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, z, FIRST_Z_AXIS, x, AXIS_ACTUAL_MIN(FIRST_Y_AXIS), z);
		INTERSECT_DIAG(AXIS_ACTUAL_MAX(FIRST_Y_AXIS), y, x, FIRST_X_AXIS, z, FIRST_Z_AXIS, x, AXIS_ACTUAL_MAX(FIRST_Y_AXIS), z);
		INTERSECT_DIAG(AXIS_ACTUAL_MIN(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, y, FIRST_Y_AXIS, x, y, AXIS_ACTUAL_MIN(FIRST_Z_AXIS));
		INTERSECT_DIAG(AXIS_ACTUAL_MAX(FIRST_Z_AXIS), z, x, FIRST_X_AXIS, y, FIRST_Y_AXIS, x, y, AXIS_ACTUAL_MAX(FIRST_Z_AXIS));
	#undef INTERSECT_DIAG
		// If we reach here, the inrange point is on the edge, and
		// the line segment from the outrange point does not cross any
		// other edges to get there. In this case, we return the inrange
		// point as the 'edge' intersection point. This will basically draw line.
		*ex = ix;
		*ey = iy;
		*ez = iz;
	}
}

/* double edge intersection algorithm */
/* Given two points, both outside the plot, return
 * the points where an edge of the plot intersects the line segment defined
 * by the two points. There may be zero, one, two, or an infinite number
 * of intersection points. (One means an intersection at a corner, infinite
 * means overlaying the edge itself). We return FALSE when there is nothing
 * to draw (zero intersections), and TRUE when there is something to
 * draw (the one-point case is a degenerate of the two-point case and we do
 * not distinguish it - we draw it anyway).
 */
//bool /* any intersection? */ two_edge3d_intersect(coordinate * p0, coordinate * p1, double * lx, double * ly, double * lz) /* lx[2], ly[2], lz[2]: points where it crosses edges */
bool GnuPlot::TwoEdge3DIntersect(const GpCoordinate * p0, const GpCoordinate * p1, double * lx, double * ly, double * lz/* lx[2], ly[2], lz[2]: points where it crosses edges */)
{
	int count;
	// global AxS[FIRST_{X,Y,Z}_AXIS].{min,max} 
	double ix = p0->x;
	double iy = p0->y;
	double iz = p0->z;
	double ox = p1->x;
	double oy = p1->y;
	double oz = p1->z;
	double t[6];
	double swap;
	double x, y, z;         /* possible intersection point */
	double t_min, t_max;
	/* nasty degenerate cases, effectively drawing to an infinity point (?)
	   cope with them here, so don't process them as a "real" OUTRANGE point

	   If more than one coord is -VERYLARGE, then can't ratio the "infinities"
	   so drop out by returning FALSE */
	count = 0;
	if(ix == -VERYLARGE)
		count++;
	if(ox == -VERYLARGE)
		count++;
	if(iy == -VERYLARGE)
		count++;
	if(oy == -VERYLARGE)
		count++;
	if(iz == -VERYLARGE)
		count++;
	if(oz == -VERYLARGE)
		count++;
	// either doesn't pass through 3D volume *or*
	// can't ratio infinities to get a direction to draw line, so simply return(FALSE) 
	if(count > 1) {
		return (FALSE);
	}
	if(ox == -VERYLARGE || ix == -VERYLARGE) {
		if(ix == -VERYLARGE) {
			/* swap points so ix/iy/iz don't have a -VERYLARGE component */
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
		/* check actually passes through the 3D graph volume */
		if(ix > AxS[FIRST_X_AXIS].max && IN_AXIS_RANGE(iy, FIRST_Y_AXIS) && IN_AXIS_RANGE(iz, FIRST_Z_AXIS)) {
			lx[0] = AxS[FIRST_X_AXIS].min;
			ly[0] = iy;
			lz[0] = iz;

			lx[1] = AxS[FIRST_X_AXIS].max;
			ly[1] = iy;
			lz[1] = iz;

			return TRUE;
		}
		else {
			return (FALSE);
		}
	}
	if(oy == -VERYLARGE || iy == -VERYLARGE) {
		if(iy == -VERYLARGE) {
			/* swap points so ix/iy/iz don't have a -VERYLARGE component */
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
		/* check actually passes through the 3D graph volume */
		if(iy > AxS[FIRST_Y_AXIS].max && IN_AXIS_RANGE(ix, FIRST_X_AXIS) && IN_AXIS_RANGE(iz, FIRST_Z_AXIS)) {
			lx[0] = ix;
			ly[0] = AxS[FIRST_Y_AXIS].min;
			lz[0] = iz;
			lx[1] = ix;
			ly[1] = AxS[FIRST_Y_AXIS].max;
			lz[1] = iz;
			return TRUE;
		}
		else {
			return (FALSE);
		}
	}
	if(oz == -VERYLARGE || iz == -VERYLARGE) {
		if(iz == -VERYLARGE) {
			/* swap points so ix/iy/iz don't have a -VERYLARGE component */
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
		/* check actually passes through the 3D graph volume */
		if(iz > AxS[FIRST_Z_AXIS].max && IN_AXIS_RANGE(ix, FIRST_X_AXIS) && IN_AXIS_RANGE(iy, FIRST_Y_AXIS)) {
			lx[0] = ix;
			ly[0] = iy;
			lz[0] = AxS[FIRST_Z_AXIS].min;

			lx[1] = ix;
			ly[1] = iy;
			lz[1] = AxS[FIRST_Z_AXIS].max;

			return TRUE;
		}
		else {
			return (FALSE);
		}
	}
	/*
	 * Quick outcode tests on the 3d graph volume
	 */

	/* test z coord first --- most surface OUTRANGE points generated
	 * between AxS[FIRST_Z_AXIS].min and baseplane (i.e. when
	 * ticslevel is non-zero)
	 */
	if(MAX(iz, oz) < AxS[FIRST_Z_AXIS].min || MIN(iz, oz) > AxS[FIRST_Z_AXIS].max)
		return (FALSE);
	if(MAX(ix, ox) < AxS[FIRST_X_AXIS].min || MIN(ix, ox) > AxS[FIRST_X_AXIS].max)
		return (FALSE);
	if(MAX(iy, oy) < AxS[FIRST_Y_AXIS].min || MIN(iy, oy) > AxS[FIRST_Y_AXIS].max)
		return (FALSE);
	// 
	// Special horizontal/vertical, etc. cases are checked and
	// remaining slant lines are checked separately.
	// 
	// The slant line intersections are solved using the parametric
	// form of the equation for a line, since if we test x/y/z min/max
	// planes explicitly then e.g. a line passing through a corner
	// point (x_min,y_min,z_min) actually intersects all 3 planes and
	// hence further tests would be required to anticipate this and
	// similar situations. 
	// 
	// Can have case (ix == ox && iy == oy && iz == oz) as both points OUTRANGE 
	if(ix == ox && iy == oy && iz == oz) {
		// but as only define single outrange point, can't intersect 3D graph volume 
		return (FALSE);
	}
	if(ix == ox) {
		if(iy == oy) {
			/* line parallel to z axis */

			/* x and y coords must be in range, and line must span
			 * both FIRST_Z_AXIS->min and ->max.
			 *
			 * note that spanning FIRST_Z_AXIS->min implies spanning
			 * ->max as both points OUTRANGE */
			if(!IN_AXIS_RANGE(ix, FIRST_X_AXIS) || !IN_AXIS_RANGE(iy, FIRST_Y_AXIS)) {
				return (FALSE);
			}
			if(inrange(AxS[FIRST_Z_AXIS].min, iz, oz)) {
				lx[0] = ix;
				ly[0] = iy;
				lz[0] = AxS[FIRST_Z_AXIS].min;

				lx[1] = ix;
				ly[1] = iy;
				lz[1] = AxS[FIRST_Z_AXIS].max;

				return TRUE;
			}
			else
				return (FALSE);
		}
		if(iz == oz) {
			/* line parallel to y axis */
			if(!IN_AXIS_RANGE(ix, FIRST_X_AXIS) || !IN_AXIS_RANGE(iz, FIRST_Z_AXIS)) {
				return (FALSE);
			}
			if(inrange(AxS[FIRST_Y_AXIS].min, iy, oy)) {
				lx[0] = ix;
				ly[0] = AxS[FIRST_Y_AXIS].min;
				lz[0] = iz;

				lx[1] = ix;
				ly[1] = AxS[FIRST_Y_AXIS].max;
				lz[1] = iz;

				return TRUE;
			}
			else
				return (FALSE);
		}

		/* nasty 2D slanted line in a yz plane */
		if(!IN_AXIS_RANGE(ox, FIRST_X_AXIS))
			return (FALSE);

		t[0] = (AxS[FIRST_Y_AXIS].min - iy) / (oy - iy);
		t[1] = (AxS[FIRST_Y_AXIS].max - iy) / (oy - iy);

		if(t[0] > t[1]) {
			swap = t[0];
			t[0] = t[1];
			t[1] = swap;
		}
		t[2] = (AxS[FIRST_Z_AXIS].min - iz) / (oz - iz);
		t[3] = (AxS[FIRST_Z_AXIS].max - iz) / (oz - iz);

		if(t[2] > t[3]) {
			swap = t[2];
			t[2] = t[3];
			t[3] = swap;
		}
		t_min = MAX(MAX(t[0], t[2]), 0.0);
		t_max = MIN(MIN(t[1], t[3]), 1.0);

		if(t_min > t_max)
			return (FALSE);

		lx[0] = ix;
		ly[0] = iy + t_min * (oy - iy);
		lz[0] = iz + t_min * (oz - iz);

		lx[1] = ix;
		ly[1] = iy + t_max * (oy - iy);
		lz[1] = iz + t_max * (oz - iz);

		/* Can only have 0 or 2 intersection points -- only need test
		 * one coord */
		if(IN_AXIS_RANGE(ly[0], FIRST_Y_AXIS) && IN_AXIS_RANGE(lz[0], FIRST_Z_AXIS)) {
			return TRUE;
		}
		return (FALSE);
	}
	if(iy == oy) {
		/* already checked case (ix == ox && iy == oy) */
		if(oz == iz) {
			/* line parallel to x axis */
			if(!IN_AXIS_RANGE(iy, FIRST_Y_AXIS) || !IN_AXIS_RANGE(iz, FIRST_Z_AXIS)) {
				return (FALSE);
			}
			if(inrange(AxS[FIRST_X_AXIS].min, ix, ox)) {
				lx[0] = AxS[FIRST_X_AXIS].min;
				ly[0] = iy;
				lz[0] = iz;

				lx[1] = AxS[FIRST_X_AXIS].max;
				ly[1] = iy;
				lz[1] = iz;

				return TRUE;
			}
			else
				return (FALSE);
		}
		/* nasty 2D slanted line in an xz plane */

		if(!IN_AXIS_RANGE(oy, FIRST_Y_AXIS))
			return (FALSE);
		t[0] = (AxS[FIRST_X_AXIS].min - ix) / (ox - ix);
		t[1] = (AxS[FIRST_X_AXIS].max - ix) / (ox - ix);
		if(t[0] > t[1]) {
			swap = t[0];
			t[0] = t[1];
			t[1] = swap;
		}
		t[2] = (AxS[FIRST_Z_AXIS].min - iz) / (oz - iz);
		t[3] = (AxS[FIRST_Z_AXIS].max - iz) / (oz - iz);
		if(t[2] > t[3]) {
			swap = t[2];
			t[2] = t[3];
			t[3] = swap;
		}
		t_min = MAX(MAX(t[0], t[2]), 0.0);
		t_max = MIN(MIN(t[1], t[3]), 1.0);
		if(t_min > t_max)
			return (FALSE);

		lx[0] = ix + t_min * (ox - ix);
		ly[0] = iy;
		lz[0] = iz + t_min * (oz - iz);

		lx[1] = ix + t_max * (ox - ix);
		ly[1] = iy;
		lz[1] = iz + t_max * (oz - iz);
		/*
		 * Can only have 0 or 2 intersection points -- only need test one coord
		 */
		if(IN_AXIS_RANGE(lx[0], FIRST_X_AXIS) && IN_AXIS_RANGE(lz[0], FIRST_Z_AXIS)) {
			return TRUE;
		}
		return (FALSE);
	}
	if(iz == oz) {
		/* already checked cases (ix == ox && iz == oz) and (iy == oy && iz == oz) */
		/* nasty 2D slanted line in an xy plane */
		if(!IN_AXIS_RANGE(oz, FIRST_Z_AXIS))
			return (FALSE);
		t[0] = (AxS[FIRST_X_AXIS].min - ix) / (ox - ix);
		t[1] = (AxS[FIRST_X_AXIS].max - ix) / (ox - ix);

		if(t[0] > t[1]) {
			swap = t[0];
			t[0] = t[1];
			t[1] = swap;
		}
		t[2] = (AxS[FIRST_Y_AXIS].min - iy) / (oy - iy);
		t[3] = (AxS[FIRST_Y_AXIS].max - iy) / (oy - iy);

		if(t[2] > t[3]) {
			swap = t[2];
			t[2] = t[3];
			t[3] = swap;
		}
		t_min = MAX(MAX(t[0], t[2]), 0.0);
		t_max = MIN(MIN(t[1], t[3]), 1.0);

		if(t_min > t_max)
			return (FALSE);

		lx[0] = ix + t_min * (ox - ix);
		ly[0] = iy + t_min * (oy - iy);
		lz[0] = iz;

		lx[1] = ix + t_max * (ox - ix);
		ly[1] = iy + t_max * (oy - iy);
		lz[1] = iz;
		/*
		 * Can only have 0 or 2 intersection points -- only need test one coord
		 */
		if(IN_AXIS_RANGE(lx[0], FIRST_X_AXIS) && IN_AXIS_RANGE(ly[0], FIRST_Y_AXIS)) {
			return TRUE;
		}
		return (FALSE);
	}
	/* really nasty general slanted 3D case */

	/*
	   Solve parametric equation

	   (ix, iy, iz) + t (diff_x, diff_y, diff_z)

	   where 0.0 <= t <= 1.0 and

	   diff_x = (ox - ix);
	   diff_y = (oy - iy);
	   diff_z = (oz - iz);
	 */

	t[0] = (AxS[FIRST_X_AXIS].min - ix) / (ox - ix);
	t[1] = (AxS[FIRST_X_AXIS].max - ix) / (ox - ix);

	if(t[0] > t[1]) {
		swap = t[0];
		t[0] = t[1];
		t[1] = swap;
	}
	t[2] = (AxS[FIRST_Y_AXIS].min - iy) / (oy - iy);
	t[3] = (AxS[FIRST_Y_AXIS].max - iy) / (oy - iy);

	if(t[2] > t[3]) {
		swap = t[2];
		t[2] = t[3];
		t[3] = swap;
	}
	t[4] = (iz == oz) ? 0.0 : (AxS[FIRST_Z_AXIS].min - iz) / (oz - iz);
	t[5] = (iz == oz) ? 1.0 : (AxS[FIRST_Z_AXIS].max - iz) / (oz - iz);

	if(t[4] > t[5]) {
		swap = t[4];
		t[4] = t[5];
		t[5] = swap;
	}
	t_min = MAX(MAX(t[0], t[2]), MAX(t[4], 0.0));
	t_max = MIN(MIN(t[1], t[3]), MIN(t[5], 1.0));
	if(t_min > t_max)
		return (FALSE);

	lx[0] = ix + t_min * (ox - ix);
	ly[0] = iy + t_min * (oy - iy);
	lz[0] = iz + t_min * (oz - iz);

	lx[1] = ix + t_max * (ox - ix);
	ly[1] = iy + t_max * (oy - iy);
	lz[1] = iz + t_max * (oz - iz);
	/*
	 * Can only have 0 or 2 intersection points -- only need test one coord
	 */
	if(IN_AXIS_RANGE(lx[0], FIRST_X_AXIS) && IN_AXIS_RANGE(ly[0], FIRST_Y_AXIS) && IN_AXIS_RANGE(lz[0], FIRST_Z_AXIS)) {
		return TRUE;
	}
	return (FALSE);
}
/*
	double GnuPlot::MapX3D(double x);
	double GnuPlot::MapY3D(double y);
	double GnuPlot::MapZ3D(double z);
*/
double GnuPlot::MapX3D(double x)
{
	const GpAxis * xaxis = &AxS[FIRST_X_AXIS];
	if(xaxis->linked_to_primary) {
		xaxis = xaxis->linked_to_primary;
		x = EvalLinkFunction(xaxis, x);
	}
	return ((x - xaxis->min) * _3DBlk.Scale3D.x + _3DBlk.Center3D.x - 1.0);
}

double GnuPlot::MapY3D(double y)
{
	const GpAxis * yaxis = &AxS[FIRST_Y_AXIS];
	if(yaxis->linked_to_primary) {
		yaxis = yaxis->linked_to_primary;
		y = EvalLinkFunction(yaxis, y);
	}
	return ((y - yaxis->min) * _3DBlk.Scale3D.y + _3DBlk.Center3D.y - 1.0);
}

double GnuPlot::MapZ3D(double z)
{
	const GpAxis * zaxis = &AxS[FIRST_Z_AXIS];
	if(zaxis->linked_to_primary) {
		zaxis = zaxis->linked_to_primary;
		z = EvalLinkFunction(zaxis, z);
	}
	return ((z - _3DBlk.floor_z1) * _3DBlk.Scale3D.z + _3DBlk.Center3D.z - 1.0);
}
// 
// Performs transformation from 'user coordinates' to a normalized
// vector in 'graph coordinates' (-1..1 in all three directions).  
// 
//void map3d_xyz(double x, double y, double z/* user coordinates */, GpVertex * out)
void GnuPlot::Map3D_XYZ(double x, double y, double z/* user coordinates */, GpVertex * pOut)
{
	double v[4], res[4]; // Homogeneous coords. vectors.
	// Normalize object space to -1..1 
	v[0] = MapX3D(x);
	v[1] = MapY3D(y);
	v[2] = MapZ3D(z);
	v[3] = 1.0;
	// Res[] = V[] * trans_mat[][] (uses row-vectors) 
	for(int i = 0; i < 4; i++) {
		res[i] = _3DBlk.trans_mat[3][i]; // V[3] is always 1. 
		res[i] += v[0] * _3DBlk.trans_mat[0][i];
		res[i] += v[1] * _3DBlk.trans_mat[1][i];
		res[i] += v[2] * _3DBlk.trans_mat[2][i];
	}
	if(res[3] == 0)
		res[3] = 1.0e-5;
	pOut->x = res[0] / res[3];
	pOut->y = res[1] / res[3];
	pOut->z = res[2] / res[3];
	// store z for later color calculation 
	pOut->real_z = z;
	pOut->label = NULL;
}
//
// Function to map from user 3D space to normalized 'camera' view
// space, and from there directly to terminal coordinates 
//
//void map3d_xy(double x, double y, double z, int * xt, int * yt)
void GnuPlot::Map3D_XY(double x, double y, double z, int * xt, int * yt)
{
	double xtd, ytd;
	Map3D_XY_double(x, y, z, &xtd, &ytd);
	*xt = static_cast<int>(xtd);
	*yt = static_cast<int>(ytd);
}

SPoint2I GnuPlot::Map3D_XY(double x, double y, double z)
{
	double xtd, ytd;
	Map3D_XY_double(x, y, z, &xtd, &ytd);
	return SPoint2I(static_cast<int>(xtd), static_cast<int>(ytd));
}

//void map3d_xy_double(double x, double y, double z, double * xt, double * yt)
void GnuPlot::Map3D_XY_double(double x, double y, double z, double * xt, double * yt)
{
	GpVertex v;
	Map3D_XYZ(x, y, z, &v);
	TERMCOORD(&v, *xt, *yt);
}
//
// HBB 20020313: New routine, broken out of draw3d_point, to be used
// to output a single point without any checks for hidden3d 
//
//void draw3d_point_unconditional(GpTermEntry * pTerm, GpVertex * v, lp_style_type * lp)
void GnuPlot::Draw3DPointUnconditional(GpTermEntry * pTerm, const GpVertex * pV, const lp_style_type * pLp)
{
	int x, y;
	TERMCOORD(pV, x, y);
	// Jul 2010 EAM - is it safe to overwrite like this? Make a copy instead? 
	// @sobolev pLp->pm3d_color.value = pV->real_z;
	// @sobolev TermApplyLpProperties(pTerm, pLp);
	// @sobolev {
	if(pLp->pm3d_color.value != pV->real_z) {
		lp_style_type temp_lp = *pLp;
		temp_lp.pm3d_color.value = pV->real_z;
		TermApplyLpProperties(pTerm, &temp_lp);
	}
	else {
		TermApplyLpProperties(pTerm, pLp);
	}
	// } @sobolev 
	if(!V.ClipPoint(x, y))
		pTerm->point(pTerm, x, y, pLp->PtType);
}
// 
// Moved this upward, to make optional inlining in draw3d_line easier for compilers 
// HBB 20021128: removed GP_INLINE qualifier to avoid MSVC++ silliness 
// 
//void draw3d_line_unconditional(GpTermEntry * pTerm, GpVertex * v1, GpVertex * v2, lp_style_type * lp, t_colorspec color)
void GnuPlot::Draw3DLineUnconditional(GpTermEntry * pTerm, const GpVertex * pV1, const GpVertex * pV2, const lp_style_type * lp, t_colorspec color)
{
	// HBB 20020312: v2 can be NULL, if this call is coming from draw_line_hidden. --> redirect to point drawing routine 
	if(!pV2) {
		Draw3DPointUnconditional(pTerm, pV1, lp);
	}
	else {
		double x1, y1, x2, y2;
		lp_style_type ls = *lp;
		TERMCOORD_DOUBLE(pV1, x1, y1);
		TERMCOORD_DOUBLE(pV2, x2, y2);
		ls.pm3d_color = color; // Replace original color with the one passed in 
		// Color by Z value 
		if(ls.pm3d_color.type == TC_Z)
			ls.pm3d_color.value = (pV1->real_z + pV2->real_z) * 0.5;
		// This interrupts the polyline, messing up dash patterns. 
		// Skip it if the caller indicates that the line properties are 
		// already set by passing in color.type = TC_DEFAULT 
		if(color.type != TC_DEFAULT)
			TermApplyLpProperties(pTerm, &ls);
		// Support for hidden3d VECTOR mode with arrowheads 
		if(lp->PtType == PT_ARROWHEAD)
			DrawClipArrow(pTerm, x1, y1, x2, y2, END_HEAD);
		else if(lp->PtType == PT_BACKARROW)
			DrawClipArrow(pTerm, x1, y1, x2, y2, BACKHEAD);
		else if(lp->PtType == PT_BOTHHEADS)
			DrawClipArrow(pTerm, x1, y1, x2, y2, BOTH_HEADS);
		else
			DrawClipLine(pTerm, static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2));
	}
}

//void draw3d_line(GpVertex * v1, GpVertex * v2, lp_style_type * lp)
void GnuPlot::Draw3DLine(GpTermEntry * pTerm, GpVertex * v1, GpVertex * v2, lp_style_type * lp)
{
	// hidden3d routine can't work if no surface was drawn at all 
	if(_3DBlk.hidden3d && _3DBlk.draw_surface)
		DrawLineHidden(pTerm, v1, v2, lp);
	else
		Draw3DLineUnconditional(pTerm, v1, v2, lp, lp->pm3d_color);
}
//
// HBB 20000621: new routine, to allow for hiding point symbols behind the surface 
//
//void draw3d_point(GpVertex * v, lp_style_type * lp)
void GnuPlot::Draw3DPoint(GpTermEntry * pTerm, GpVertex * v, lp_style_type * lp)
{
	// hidden3d routine can't work if no surface was drawn at all 
	if(_3DBlk.hidden3d && _3DBlk.draw_surface)
		DrawLineHidden(pTerm, v, NULL, lp); // Draw vertex as a zero-length edge 
	else
		Draw3DPointUnconditional(pTerm, v, lp);
}

/* HBB NEW 20031218: tools for drawing polylines in 3D with a semantic
 * like term->move() and term->vector() */

static GpVertex polyline3d_previous_vertex; // @global Previous points 3D position 

//void polyline3d_start(GpVertex * v1)
void GnuPlot::Polyline3DStart(GpTermEntry * pTerm, GpVertex * v1)
{
	int x1, y1;
	polyline3d_previous_vertex = *v1;
	if(_3DBlk.hidden3d && _3DBlk.draw_surface)
		return;
	// EAM - This may now be unneeded. But I'm not sure. */
	//       Perhaps the hidden3d code needs the move.   */
	TERMCOORD(v1, x1, y1);
	pTerm->move(pTerm, x1, y1);
}

//void polyline3d_next(GpVertex * v2, lp_style_type * lp)
void GnuPlot::Polyline3DNext(GpTermEntry * pTerm, GpVertex * v2, lp_style_type * lp)
{
	// v5: Indicate that line properties are already set so that draw3d_*
	//     routines do not mess up dash patterns by resetting them
	t_colorspec nochange = DEFAULT_COLORSPEC;
	if(_3DBlk.hidden3d && _3DBlk.draw_surface)
		DrawLineHidden(pTerm, &polyline3d_previous_vertex, v2, lp);
	else
		Draw3DLineUnconditional(pTerm, &polyline3d_previous_vertex, v2, lp, nochange);
	polyline3d_previous_vertex = *v2;
}
