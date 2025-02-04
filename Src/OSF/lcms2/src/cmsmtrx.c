//
//  Little Color Management System
//  Copyright (c) 1998-2020 Marti Maria Saguer
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
#include "lcms2_internal.h"
#pragma hdrstop

// @sobolev (unused) #define DSWAP(x, y)     {double tmp = (x); (x) = (y); (y) = tmp;}

// Initiate a vector
void CMSEXPORT _cmsVEC3init(cmsVEC3* r, double x, double y, double z)
{
	r->n[VX] = x;
	r->n[VY] = y;
	r->n[VZ] = z;
}

// Vector subtraction
void CMSEXPORT _cmsVEC3minus(cmsVEC3* r, const cmsVEC3* a, const cmsVEC3* b)
{
	r->n[VX] = a->n[VX] - b->n[VX];
	r->n[VY] = a->n[VY] - b->n[VY];
	r->n[VZ] = a->n[VZ] - b->n[VZ];
}

// Vector cross product
void CMSEXPORT _cmsVEC3cross(cmsVEC3* r, const cmsVEC3* u, const cmsVEC3* v)
{
	r->n[VX] = u->n[VY] * v->n[VZ] - v->n[VY] * u->n[VZ];
	r->n[VY] = u->n[VZ] * v->n[VX] - v->n[VZ] * u->n[VX];
	r->n[VZ] = u->n[VX] * v->n[VY] - v->n[VX] * u->n[VY];
}

// Vector dot product
double CMSEXPORT _cmsVEC3dot(const cmsVEC3* u, const cmsVEC3* v)
{
	return u->n[VX] * v->n[VX] + u->n[VY] * v->n[VY] + u->n[VZ] * v->n[VZ];
}

// Euclidean length
double CMSEXPORT _cmsVEC3length(const cmsVEC3* a)
{
	return sqrt(a->n[VX] * a->n[VX] + a->n[VY] * a->n[VY] + a->n[VZ] * a->n[VZ]);
}

// Euclidean distance
double CMSEXPORT _cmsVEC3distance(const cmsVEC3* a, const cmsVEC3* b)
{
	const double d1 = a->n[VX] - b->n[VX];
	const double d2 = a->n[VY] - b->n[VY];
	const double d3 = a->n[VZ] - b->n[VZ];
	return sqrt(d1*d1 + d2*d2 + d3*d3);
}

// 3x3 Identity
void CMSEXPORT _cmsMAT3identity(cmsMAT3* a)
{
	_cmsVEC3init(&a->v[0], 1.0, 0.0, 0.0);
	_cmsVEC3init(&a->v[1], 0.0, 1.0, 0.0);
	_cmsVEC3init(&a->v[2], 0.0, 0.0, 1.0);
}

static boolint CloseEnough(double a, double b)
{
	return fabs(b - a) < (1.0 / 65535.0);
}

boolint CMSEXPORT _cmsMAT3isIdentity(const cmsMAT3* a)
{
	cmsMAT3 Identity;
	_cmsMAT3identity(&Identity);
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			if(!CloseEnough(a->v[i].n[j], Identity.v[i].n[j])) 
				return FALSE;
	return TRUE;
}

// Multiply two matrices
void CMSEXPORT _cmsMAT3per(cmsMAT3* r, const cmsMAT3* a, const cmsMAT3* b)
{
#define ROWCOL(i, j) a->v[i].n[0]*b->v[0].n[j] + a->v[i].n[1]*b->v[1].n[j] + a->v[i].n[2]*b->v[2].n[j]
	_cmsVEC3init(&r->v[0], ROWCOL(0, 0), ROWCOL(0, 1), ROWCOL(0, 2));
	_cmsVEC3init(&r->v[1], ROWCOL(1, 0), ROWCOL(1, 1), ROWCOL(1, 2));
	_cmsVEC3init(&r->v[2], ROWCOL(2, 0), ROWCOL(2, 1), ROWCOL(2, 2));
#undef ROWCOL //(i, j)
}

// Inverse of a matrix b = a^(-1)
boolint CMSEXPORT _cmsMAT3inverse(const cmsMAT3* a, cmsMAT3* b)
{
	double c0 =  a->v[1].n[1]*a->v[2].n[2] - a->v[1].n[2]*a->v[2].n[1];
	double c1 = -a->v[1].n[0]*a->v[2].n[2] + a->v[1].n[2]*a->v[2].n[0];
	double c2 =  a->v[1].n[0]*a->v[2].n[1] - a->v[1].n[1]*a->v[2].n[0];
	double det = a->v[0].n[0]*c0 + a->v[0].n[1]*c1 + a->v[0].n[2]*c2;
	if(fabs(det) < MATRIX_DET_TOLERANCE) 
		return FALSE; // singular matrix; can't invert
	b->v[0].n[0] = c0/det;
	b->v[0].n[1] = (a->v[0].n[2]*a->v[2].n[1] - a->v[0].n[1]*a->v[2].n[2])/det;
	b->v[0].n[2] = (a->v[0].n[1]*a->v[1].n[2] - a->v[0].n[2]*a->v[1].n[1])/det;
	b->v[1].n[0] = c1/det;
	b->v[1].n[1] = (a->v[0].n[0]*a->v[2].n[2] - a->v[0].n[2]*a->v[2].n[0])/det;
	b->v[1].n[2] = (a->v[0].n[2]*a->v[1].n[0] - a->v[0].n[0]*a->v[1].n[2])/det;
	b->v[2].n[0] = c2/det;
	b->v[2].n[1] = (a->v[0].n[1]*a->v[2].n[0] - a->v[0].n[0]*a->v[2].n[1])/det;
	b->v[2].n[2] = (a->v[0].n[0]*a->v[1].n[1] - a->v[0].n[1]*a->v[1].n[0])/det;
	return TRUE;
}

// Solve a system in the form Ax = b
boolint CMSEXPORT _cmsMAT3solve(cmsVEC3* x, const cmsMAT3 * a, const cmsVEC3 * b)
{
	cmsMAT3 m, a_1;
	memmove(&m, a, sizeof(cmsMAT3));
	if(!_cmsMAT3inverse(&m, &a_1)) 
		return FALSE; // Singular matrix
	_cmsMAT3eval(x, &a_1, b);
	return TRUE;
}

// Evaluate a vector across a matrix
void CMSEXPORT _cmsMAT3eval(cmsVEC3* r, const cmsMAT3* a, const cmsVEC3* v)
{
	r->n[VX] = a->v[0].n[VX]*v->n[VX] + a->v[0].n[VY]*v->n[VY] + a->v[0].n[VZ]*v->n[VZ];
	r->n[VY] = a->v[1].n[VX]*v->n[VX] + a->v[1].n[VY]*v->n[VY] + a->v[1].n[VZ]*v->n[VZ];
	r->n[VZ] = a->v[2].n[VX]*v->n[VX] + a->v[2].n[VY]*v->n[VY] + a->v[2].n[VZ]*v->n[VZ];
}
