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

// Tone curves are powerful constructs that can contain curves specified in diverse ways.
// The curve is stored in segments, where each segment can be sampled or specified by parameters.
// a 16.bit simplification of the *whole* curve is kept for optimization purposes. For float operation,
// each segment is evaluated separately. Plug-ins may be used to define new parametric schemes,
// each plug-in may define up to MAX_TYPES_IN_LCMS_PLUGIN functions types. For defining a function,
// the plug-in should provide the type id, how many parameters each type has, and a pointer to
// a procedure that evaluates the function. In the case of reverse evaluation, the evaluator will
// be called with the type id as a negative value, and a sampled version of the reversed curve
// will be built.

// ----------------------------------------------------------------- Implementation
// Maxim number of nodes
#define MAX_NODES_IN_CURVE   4097
#define MINUS_INF            (-1E22F)
#define PLUS_INF             (+1E22F)

// The list of supported parametric curves
typedef struct _cmsParametricCurvesCollection_st {
	uint32 nFunctions;                                 // Number of supported functions in this chunk
	int32 FunctionTypes[MAX_TYPES_IN_LCMS_PLUGIN];     // The identification types
	uint32 ParameterCount[MAX_TYPES_IN_LCMS_PLUGIN];   // Number of parameters for each function
	cmsParametricCurveEvaluator Evaluator;                      // The evaluator
	struct _cmsParametricCurvesCollection_st* Next; // Next in list
} _cmsParametricCurvesCollection;

// This is the default (built-in) evaluator
static double DefaultEvalParametricFn(int Type, const double Params[], double R);

// The built-in list
static _cmsParametricCurvesCollection DefaultCurves = {
	9,                              // # of curve types
	{ 1, 2, 3, 4, 5, 6, 7, 8, 108 }, // Parametric curve ID
	{ 1, 3, 4, 5, 7, 4, 5, 5, 1 },  // Parameters by type
	DefaultEvalParametricFn,        // Evaluator
	NULL                            // Next in chain
};

// Duplicates the zone of memory used by the plug-in in the new context
static void DupPluginCurvesList(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	_cmsCurvesPluginChunkType newHead = { NULL };
	_cmsParametricCurvesCollection*  entry;
	_cmsParametricCurvesCollection*  Anterior = NULL;
	_cmsCurvesPluginChunkType* head = (_cmsCurvesPluginChunkType*)src->chunks[CurvesPlugin];
	assert(head != NULL);
	// Walk the list copying all nodes
	for(entry = head->ParametricCurves; entry != NULL; entry = entry->Next) {
		_cmsParametricCurvesCollection * newEntry = (_cmsParametricCurvesCollection*)_cmsSubAllocDup(ctx->MemPool, entry, sizeof(_cmsParametricCurvesCollection));
		if(!newEntry)
			return;
		// We want to keep the linked list order, so this is a little bit tricky
		newEntry->Next = NULL;
		if(Anterior)
			Anterior->Next = newEntry;
		Anterior = newEntry;
		SETIFZQ(newHead.ParametricCurves, newEntry);
	}
	ctx->chunks[CurvesPlugin] = _cmsSubAllocDup(ctx->MemPool, &newHead, sizeof(_cmsCurvesPluginChunkType));
}

// The allocator have to follow the chain
void _cmsAllocCurvesPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	assert(ctx);
	if(src) {
		DupPluginCurvesList(ctx, src); // Copy all linked list
	}
	else {
		static _cmsCurvesPluginChunkType CurvesPluginChunk = { NULL };
		ctx->chunks[CurvesPlugin] = _cmsSubAllocDup(ctx->MemPool, &CurvesPluginChunk, sizeof(_cmsCurvesPluginChunkType));
	}
}

// The linked list head
_cmsCurvesPluginChunkType _cmsCurvesPluginChunk = { NULL };

// As a way to install new parametric curves
boolint _cmsRegisterParametricCurvesPlugin(cmsContext ContextID, cmsPluginBase* Data)
{
	_cmsCurvesPluginChunkType* ctx = (_cmsCurvesPluginChunkType*)_cmsContextGetClientChunk(ContextID, CurvesPlugin);
	cmsPluginParametricCurves* Plugin = (cmsPluginParametricCurves*)Data;
	_cmsParametricCurvesCollection* fl;
	if(!Data) {
		ctx->ParametricCurves =  NULL;
		return TRUE;
	}
	fl = (_cmsParametricCurvesCollection*)_cmsPluginMalloc(ContextID, sizeof(_cmsParametricCurvesCollection));
	if(fl == NULL) return FALSE;

	// Copy the parameters
	fl->Evaluator  = Plugin->Evaluator;
	fl->nFunctions = Plugin->nFunctions;

	// Make sure no mem overwrites
	if(fl->nFunctions > MAX_TYPES_IN_LCMS_PLUGIN)
		fl->nFunctions = MAX_TYPES_IN_LCMS_PLUGIN;

	// Copy the data
	memmove(fl->FunctionTypes,  Plugin->FunctionTypes,   fl->nFunctions * sizeof(uint32));
	memmove(fl->ParameterCount, Plugin->ParameterCount,  fl->nFunctions * sizeof(uint32));

	// Keep linked list
	fl->Next = ctx->ParametricCurves;
	ctx->ParametricCurves = fl;

	// All is ok
	return TRUE;
}

// Search in type list, return position or -1 if not found
static int IsInSet(int Type, _cmsParametricCurvesCollection* c)
{
	for(int i = 0; i < (int)c->nFunctions; i++)
		if(abs(Type) == c->FunctionTypes[i]) 
			return i;
	return -1;
}

// Search for the collection which contains a specific type
static _cmsParametricCurvesCollection * GetParametricCurveByType(cmsContext ContextID, int Type, int* index)
{
	_cmsParametricCurvesCollection* c;
	int Position;
	_cmsCurvesPluginChunkType* ctx = (_cmsCurvesPluginChunkType*)_cmsContextGetClientChunk(ContextID, CurvesPlugin);
	for(c = ctx->ParametricCurves; c != NULL; c = c->Next) {
		Position = IsInSet(Type, c);
		if(Position != -1) {
			ASSIGN_PTR(index, Position);
			return c;
		}
	}
	// If none found, revert for defaults
	for(c = &DefaultCurves; c != NULL; c = c->Next) {
		Position = IsInSet(Type, c);
		if(Position != -1) {
			ASSIGN_PTR(index, Position);
			return c;
		}
	}
	return NULL;
}

// Low level allocate, which takes care of memory details. nEntries may be zero, and in this case
// no optimation curve is computed. nSegments may also be zero in the inverse case, where only the
// optimization curve is given. Both features simultaneously is an error
static cmsToneCurve * AllocateToneCurveStruct(cmsContext ContextID, uint32 nEntries,
    uint32 nSegments, const cmsCurveSegment* Segments, const uint16* Values)
{
	cmsToneCurve * p = 0;
	uint32 i;
	// We allow huge tables, which are then restricted for smoothing operations
	if(nEntries > 65530) {
		cmsSignalError(ContextID, cmsERROR_RANGE, "Couldn't create tone curve of more than 65530 entries");
		return NULL;
	}
	if(nEntries == 0 && nSegments == 0) {
		cmsSignalError(ContextID, cmsERROR_RANGE, "Couldn't create tone curve with zero segments and no table");
		return NULL;
	}
	// Allocate all required pointers, etc.
	p = (cmsToneCurve *)_cmsMallocZero(ContextID, sizeof(cmsToneCurve));
	if(!p) return NULL;
	// In this case, there are no segments
	if(nSegments == 0) {
		p->Segments = NULL;
		p->Evals = NULL;
	}
	else {
		p->Segments = (cmsCurveSegment*)_cmsCalloc(ContextID, nSegments, sizeof(cmsCurveSegment));
		if(p->Segments == NULL) 
			goto Error;
		p->Evals    = (cmsParametricCurveEvaluator*)_cmsCalloc(ContextID, nSegments, sizeof(cmsParametricCurveEvaluator));
		if(p->Evals == NULL) 
			goto Error;
	}
	p->nSegments = nSegments;
	// This 16-bit table contains a limited precision representation of the whole curve and is kept for
	// increasing xput on certain operations.
	if(nEntries == 0) {
		p->Table16 = NULL;
	}
	else {
		p->Table16 = (uint16*)_cmsCalloc(ContextID, nEntries, sizeof(uint16));
		if(p->Table16 == NULL) 
			goto Error;
	}
	p->nEntries  = nEntries;
	// Initialize members if requested
	if(Values != NULL && (nEntries > 0)) {
		for(i = 0; i < nEntries; i++)
			p->Table16[i] = Values[i];
	}

	// Initialize the segments stuff. The evaluator for each segment is located and a pointer to it
	// is placed in advance to maximize performance.
	if(Segments != NULL && (nSegments > 0)) {
		_cmsParametricCurvesCollection * c;
		p->SegInterp = (cmsInterpParams**)_cmsCalloc(ContextID, nSegments, sizeof(cmsInterpParams*));
		if(p->SegInterp == NULL) goto Error;
		for(i = 0; i < nSegments; i++) {
			// Type 0 is a special marker for table-based curves
			if(Segments[i].Type == 0)
				p->SegInterp[i] = _cmsComputeInterpParams(ContextID, Segments[i].nGridPoints, 1, 1, NULL, CMS_LERP_FLAGS_FLOAT);
			memmove(&p->Segments[i], &Segments[i], sizeof(cmsCurveSegment));
			if(Segments[i].Type == 0 && Segments[i].SampledPoints != NULL)
				p->Segments[i].SampledPoints = (float *)_cmsDupMem(ContextID, Segments[i].SampledPoints, sizeof(float) * Segments[i].nGridPoints);
			else
				p->Segments[i].SampledPoints = NULL;
			c = GetParametricCurveByType(ContextID, Segments[i].Type, NULL);
			if(c)
				p->Evals[i] = c->Evaluator;
		}
	}
	p->InterpParams = _cmsComputeInterpParams(ContextID, p->nEntries, 1, 1, p->Table16, CMS_LERP_FLAGS_16BITS);
	if(p->InterpParams != NULL)
		return p;
Error:
	if(p) {
		_cmsFree(ContextID, p->SegInterp);
		_cmsFree(ContextID, p->Segments);
		_cmsFree(ContextID, p->Evals);
		_cmsFree(ContextID, p->Table16);
		_cmsFree(ContextID, p);
	}
	return NULL;
}

// Parametric Fn using floating point
static double DefaultEvalParametricFn(int Type, const double Params[], double R)
{
	double e, Val, disc;
	switch(Type) {
		// X = Y ^ Gamma
		case 1:
		    if(R < 0) {
			    if(fabs(Params[0] - 1.0) < MATRIX_DET_TOLERANCE)
				    Val = R;
			    else
				    Val = 0;
		    }
		    else
			    Val = pow(R, Params[0]);
		    break;

		// Type 1 Reversed: X = Y ^1/gamma
		case -1:
		    if(R < 0) {
			    if(fabs(Params[0] - 1.0) < MATRIX_DET_TOLERANCE)
				    Val = R;
			    else
				    Val = 0;
		    }
		    else {
			    if(fabs(Params[0]) < MATRIX_DET_TOLERANCE)
				    Val = PLUS_INF;
			    else
				    Val = pow(R, 1 / Params[0]);
		    }
		    break;

		// CIE 122-1966
		// Y = (aX + b)^Gamma  | X >= -b/a
		// Y = 0               | else
		case 2:
	    {
		    if(fabs(Params[1]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    disc = -Params[2] / Params[1];
			    if(R >= disc) {
				    e = Params[1] * R + Params[2];
				    if(e > 0)
					    Val = pow(e, Params[0]);
				    else
					    Val = 0;
			    }
			    else
				    Val = 0;
		    }
	    }
	    break;
		// Type 2 Reversed
		// X = (Y ^1/g  - b) / a
		case -2:
	    {
		    if(fabs(Params[0]) < MATRIX_DET_TOLERANCE || fabs(Params[1]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    if(R < 0)
				    Val = 0;
			    else
				    Val = (pow(R, 1.0 / Params[0]) - Params[2]) / Params[1];

			    if(Val < 0)
				    Val = 0;
		    }
	    }
	    break;

		// IEC 61966-3
		// Y = (aX + b)^Gamma | X <= -b/a
		// Y = c              | else
		case 3:
	    {
		    if(fabs(Params[1]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    disc = -Params[2] / Params[1];
			    if(disc < 0)
				    disc = 0;

			    if(R >= disc) {
				    e = Params[1] * R + Params[2];

				    if(e > 0)
					    Val = pow(e, Params[0]) + Params[3];
				    else
					    Val = 0;
			    }
			    else
				    Val = Params[3];
		    }
	    }
	    break;

		// Type 3 reversed
		// X=((Y-c)^1/g - b)/a      | (Y>=c)
		// X=-b/a                   | (Y<c)
		case -3:
	    {
		    if(fabs(Params[1]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    if(R >= Params[3]) {
				    e = R - Params[3];

				    if(e > 0)
					    Val = (pow(e, 1 / Params[0]) - Params[2]) / Params[1];
				    else
					    Val = 0;
			    }
			    else {
				    Val = -Params[2] / Params[1];
			    }
		    }
	    }
	    break;

		// IEC 61966-2.1 (sRGB)
		// Y = (aX + b)^Gamma | X >= d
		// Y = cX             | X < d
		case 4:
		    if(R >= Params[4]) {
			    e = Params[1]*R + Params[2];

			    if(e > 0)
				    Val = pow(e, Params[0]);
			    else
				    Val = 0;
		    }
		    else
			    Val = R * Params[3];
		    break;

		// Type 4 reversed
		// X=((Y^1/g-b)/a)    | Y >= (ad+b)^g
		// X=Y/c              | Y< (ad+b)^g
		case -4:
	    {
		    if(fabs(Params[0]) < MATRIX_DET_TOLERANCE || fabs(Params[1]) < MATRIX_DET_TOLERANCE || fabs(Params[3]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    e = Params[1] * Params[4] + Params[2];
			    if(e < 0)
				    disc = 0;
			    else
				    disc = pow(e, Params[0]);

			    if(R >= disc) {
				    Val = (pow(R, 1.0 / Params[0]) - Params[2]) / Params[1];
			    }
			    else {
				    Val = R / Params[3];
			    }
		    }
	    }
	    break;

		// Y = (aX + b)^Gamma + e | X >= d
		// Y = cX + f             | X < d
		case 5:
		    if(R >= Params[4]) {
			    e = Params[1]*R + Params[2];
			    if(e > 0)
				    Val = pow(e, Params[0]) + Params[5];
			    else
				    Val = Params[5];
		    }
		    else
			    Val = R*Params[3] + Params[6];
		    break;

		// Reversed type 5
		// X=((Y-e)1/g-b)/a   | Y >=(ad+b)^g+e), cd+f
		// X=(Y-f)/c          | else
		case -5:
	    {
		    if(fabs(Params[1]) < MATRIX_DET_TOLERANCE || fabs(Params[3]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    disc = Params[3] * Params[4] + Params[6];
			    if(R >= disc) {
				    e = R - Params[5];
				    if(e < 0)
					    Val = 0;
				    else
					    Val = (pow(e, 1.0 / Params[0]) - Params[2]) / Params[1];
			    }
			    else {
				    Val = (R - Params[6]) / Params[3];
			    }
		    }
	    }
	    break;

		// Types 6,7,8 comes from segmented curves as described in ICCSpecRevision_02_11_06_Float.pdf
		// Type 6 is basically identical to type 5 without d

		// Y = (a * X + b) ^ Gamma + c
		case 6:
		    e = Params[1]*R + Params[2];

		    if(e < 0)
			    Val = Params[3];
		    else
			    Val = pow(e, Params[0]) + Params[3];
		    break;

		// ((Y - c) ^1/Gamma - b) / a
		case -6:
	    {
		    if(fabs(Params[1]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    e = R - Params[3];
			    if(e < 0)
				    Val = 0;
			    else
				    Val = (pow(e, 1.0 / Params[0]) - Params[2]) / Params[1];
		    }
	    }
	    break;

		// Y = a * log (b * X^Gamma + c) + d
		case 7:

		    e = Params[2] * pow(R, Params[0]) + Params[3];
		    if(e <= 0)
			    Val = Params[4];
		    else
			    Val = Params[1]*log10(e) + Params[4];
		    break;

		// (Y - d) / a = log(b * X ^Gamma + c)
		// pow(10, (Y-d) / a) = b * X ^Gamma + c
		// pow((pow(10, (Y-d) / a) - c) / b, 1/g) = X
		case -7:
	    {
		    if(fabs(Params[0]) < MATRIX_DET_TOLERANCE || fabs(Params[1]) < MATRIX_DET_TOLERANCE || fabs(Params[2]) < MATRIX_DET_TOLERANCE) {
			    Val = 0;
		    }
		    else {
			    Val = pow((pow(10.0, (R - Params[4]) / Params[1]) - Params[3]) / Params[2], 1.0 / Params[0]);
		    }
	    }
	    break;

		//Y = a * b^(c*X+d) + e
		case 8:
		    Val = (Params[0] * pow(Params[1], Params[2] * R + Params[3]) + Params[4]);
		    break;

		// Y = (log((y-e) / a) / log(b) - d ) / c
		// a=0, b=1, c=2, d=3, e=4,
		case -8:

		    disc = R - Params[4];
		    if(disc < 0) Val = 0;
		    else {
			    if(fabs(Params[0]) < MATRIX_DET_TOLERANCE ||
				fabs(Params[2]) < MATRIX_DET_TOLERANCE) {
				    Val = 0;
			    }
			    else {
				    Val = (log(disc / Params[0]) / log(Params[1]) - Params[3]) / Params[2];
			    }
		    }
		    break;

		// S-Shaped: (1 - (1-x)^1/g)^1/g
		case 108:
		    if(fabs(Params[0]) < MATRIX_DET_TOLERANCE)
			    Val = 0;
		    else
			    Val = pow(1.0 - pow(1 - R, 1/Params[0]), 1/Params[0]);
		    break;

		// y = (1 - (1-x)^1/g)^1/g
		// y^g = (1 - (1-x)^1/g)
		// 1 - y^g = (1-x)^1/g
		// (1 - y^g)^g = 1 - x
		// 1 - (1 - y^g)^g
		case -108:
		    Val = 1 - pow(1 - pow(R, Params[0]), Params[0]);
		    break;
		default:
		    // Unsupported parametric curve. Should never reach here
		    return 0;
	}
	return Val;
}

// Evaluate a segmented function for a single value. Return -Inf if no valid segment found .
// If fn type is 0, perform an interpolation on the table
static double EvalSegmentedFn(const cmsToneCurve * g, double R)
{
	float Out32;
	double Out;
	for(int i = (int)g->nSegments - 1; i >= 0; --i) {
		// Check for domain
		if((R > g->Segments[i].x0) && (R <= g->Segments[i].x1)) {
			// Type == 0 means segment is sampled
			if(g->Segments[i].Type == 0) {
				float R1 = (float)(R - g->Segments[i].x0) / (g->Segments[i].x1 - g->Segments[i].x0);
				// Setup the table (TODO: clean that)
				g->SegInterp[i]->Table = g->Segments[i].SampledPoints;
				g->SegInterp[i]->Interpolation.LerpFloat(&R1, &Out32, g->SegInterp[i]);
				Out = (double)Out32;
			}
			else {
				Out = g->Evals[i](g->Segments[i].Type, g->Segments[i].Params, R);
			}
			if(isinf(Out))
				return PLUS_INF;
			else if(isinf(-Out))
				return MINUS_INF;
			else
				return Out;
		}
	}
	return MINUS_INF;
}

// Access to estimated low-res table
uint32 CMSEXPORT cmsGetToneCurveEstimatedTableEntries(const cmsToneCurve * t)
{
	assert(t);
	return t->nEntries;
}

const uint16* CMSEXPORT cmsGetToneCurveEstimatedTable(const cmsToneCurve * t)
{
	assert(t);
	return t->Table16;
}

// Create an empty gamma curve, by using tables. This specifies only the limited-precision part, and leaves the
// floating point description empty.
cmsToneCurve * CMSEXPORT cmsBuildTabulatedToneCurve16(cmsContext ContextID, uint32 nEntries, const uint16 Values[])
{
	return AllocateToneCurveStruct(ContextID, nEntries, 0, NULL, Values);
}

static uint32 EntriesByGamma(double Gamma)
{
	if(fabs(Gamma - 1.0) < 0.001) 
		return 2;
	return 4096;
}

// Create a segmented gamma, fill the table
cmsToneCurve * CMSEXPORT cmsBuildSegmentedToneCurve(cmsContext ContextID, uint32 nSegments, const cmsCurveSegment Segments[])
{
	uint32 i;
	double R, Val;
	cmsToneCurve * g;
	uint32 nGridPoints = 4096;
	assert(Segments != NULL);
	// Optimizatin for identity curves.
	if(nSegments == 1 && Segments[0].Type == 1) {
		nGridPoints = EntriesByGamma(Segments[0].Params[0]);
	}
	g = AllocateToneCurveStruct(ContextID, nGridPoints, nSegments, Segments, NULL);
	if(g == NULL) 
		return NULL;
	// Once we have the floating point version, we can approximate a 16 bit table of 4096 entries
	// for performance reasons. This table would normally not be used except on 8/16 bits transforms.
	for(i = 0; i < nGridPoints; i++) {
		R   = (double)i / (nGridPoints-1);
		Val = EvalSegmentedFn(g, R);
		// Round and saturate
		g->Table16[i] = _cmsQuickSaturateWord(Val * 65535.0);
	}
	return g;
}

// Use a segmented curve to store the floating point table
cmsToneCurve * CMSEXPORT cmsBuildTabulatedToneCurveFloat(cmsContext ContextID, uint32 nEntries, const float values[])
{
	cmsCurveSegment Seg[3];
	// A segmented tone curve should have function segments in the first and last positions
	// Initialize segmented curve part up to 0 to constant value = samples[0]
	Seg[0].x0 = MINUS_INF;
	Seg[0].x1 = 0;
	Seg[0].Type = 6;

	Seg[0].Params[0] = 1;
	Seg[0].Params[1] = 0;
	Seg[0].Params[2] = 0;
	Seg[0].Params[3] = values[0];
	Seg[0].Params[4] = 0;

	// From zero to 1
	Seg[1].x0 = 0;
	Seg[1].x1 = 1.0;
	Seg[1].Type = 0;

	Seg[1].nGridPoints = nEntries;
	Seg[1].SampledPoints = (float *)values;

	// Final segment is constant = lastsample
	Seg[2].x0 = 1.0;
	Seg[2].x1 = PLUS_INF;
	Seg[2].Type = 6;

	Seg[2].Params[0] = 1;
	Seg[2].Params[1] = 0;
	Seg[2].Params[2] = 0;
	Seg[2].Params[3] = values[nEntries-1];
	Seg[2].Params[4] = 0;

	return cmsBuildSegmentedToneCurve(ContextID, 3, Seg);
}

// Parametric curves
//
// Parameters goes as: Curve, a, b, c, d, e, f
// Type is the ICC type +1
// if type is negative, then the curve is analytically inverted
cmsToneCurve * CMSEXPORT cmsBuildParametricToneCurve(cmsContext ContextID, int32 Type, const double Params[])
{
	cmsCurveSegment Seg0;
	int Pos = 0;
	uint32 size;
	_cmsParametricCurvesCollection* c = GetParametricCurveByType(ContextID, Type, &Pos);
	assert(Params != NULL);
	if(!c) {
		cmsSignalError(ContextID, cmsERROR_UNKNOWN_EXTENSION, "Invalid parametric curve type %d", Type);
		return NULL;
	}
	memzero(&Seg0, sizeof(Seg0));
	Seg0.x0   = MINUS_INF;
	Seg0.x1   = PLUS_INF;
	Seg0.Type = Type;
	size = c->ParameterCount[Pos] * sizeof(double);
	memmove(Seg0.Params, Params, size);
	return cmsBuildSegmentedToneCurve(ContextID, 1, &Seg0);
}

// Build a gamma table based on gamma constant
cmsToneCurve * CMSEXPORT cmsBuildGamma(cmsContext ContextID, double Gamma)
{
	return cmsBuildParametricToneCurve(ContextID, 1, &Gamma);
}

// Free all memory taken by the gamma curve
void CMSEXPORT cmsFreeToneCurve(cmsToneCurve * Curve)
{
	if(Curve) {
		cmsContext ContextID = Curve->InterpParams->ContextID;
		_cmsFreeInterpParams(Curve->InterpParams);
		_cmsFree(ContextID, Curve->Table16);
		if(Curve->Segments) {
			for(uint32 i = 0; i < Curve->nSegments; i++) {
				_cmsFree(ContextID, Curve->Segments[i].SampledPoints);
				_cmsFreeInterpParams(Curve->SegInterp[i]);
			}
			_cmsFree(ContextID, Curve->Segments);
			_cmsFree(ContextID, Curve->SegInterp);
		}
		_cmsFree(ContextID, Curve->Evals);
		_cmsFree(ContextID, Curve);
	}
}

// Utility function, free 3 gamma tables
void CMSEXPORT cmsFreeToneCurveTriple(cmsToneCurve * Curve[3])
{
	assert(Curve != NULL);
	cmsFreeToneCurve(Curve[0]);
	cmsFreeToneCurve(Curve[1]);
	cmsFreeToneCurve(Curve[2]);
	Curve[0] = Curve[1] = Curve[2] = NULL;
}

// Duplicate a gamma table
cmsToneCurve * CMSEXPORT cmsDupToneCurve(const cmsToneCurve * In)
{
	if(In == NULL) return NULL;
	return AllocateToneCurveStruct(In->InterpParams->ContextID, In->nEntries, In->nSegments, In->Segments, In->Table16);
}

// Joins two curves for X and Y. Curves should be monotonic.
// We want to get
//
//      y = Y^-1(X(t))
//
cmsToneCurve * CMSEXPORT cmsJoinToneCurve(cmsContext ContextID, const cmsToneCurve * X, const cmsToneCurve * Y, uint32 nResultingPoints)
{
	cmsToneCurve * out = NULL;
	cmsToneCurve * Yreversed = NULL;
	float t, x;
	float* Res = NULL;
	uint32 i;
	assert(X != NULL);
	assert(Y != NULL);
	Yreversed = cmsReverseToneCurveEx(nResultingPoints, Y);
	if(Yreversed == NULL) goto Error;

	Res = (float *)_cmsCalloc(ContextID, nResultingPoints, sizeof(float));
	if(Res == NULL) goto Error;

	//Iterate
	for(i = 0; i <  nResultingPoints; i++) {
		t = (float)i / (nResultingPoints-1);
		x = cmsEvalToneCurveFloat(X,  t);
		Res[i] = cmsEvalToneCurveFloat(Yreversed, x);
	}
	// Allocate space for output
	out = cmsBuildTabulatedToneCurveFloat(ContextID, nResultingPoints, Res);
Error:
	_cmsFree(ContextID, Res);
	cmsFreeToneCurve(Yreversed);
	return out;
}

// Get the surrounding nodes. This is tricky on non-monotonic tables
static int GetInterval(double In, const uint16 LutTable[], const struct _cms_interp_struc* p)
{
	int i;
	int y0, y1;
	// A 1 point table is not allowed
	if(p->Domain[0] < 1) return -1;
	// Let's see if ascending or descending.
	if(LutTable[0] < LutTable[p->Domain[0]]) {
		// Table is overall ascending
		for(i = (int)p->Domain[0] - 1; i >= 0; --i) {
			y0 = LutTable[i];
			y1 = LutTable[i+1];
			if(y0 <= y1) { // Increasing
				if(In >= y0 && In <= y1) return i;
			}
			else if(y1 < y0) { // Decreasing
				if(In >= y1 && In <= y0) return i;
			}
		}
	}
	else {
		// Table is overall descending
		for(i = 0; i < (int)p->Domain[0]; i++) {
			y0 = LutTable[i];
			y1 = LutTable[i+1];

			if(y0 <= y1) { // Increasing
				if(In >= y0 && In <= y1) return i;
			}
			else if(y1 < y0) { // Decreasing
				if(In >= y1 && In <= y0) return i;
			}
		}
	}

	return -1;
}

// Reverse a gamma table
cmsToneCurve * CMSEXPORT cmsReverseToneCurveEx(uint32 nResultSamples, const cmsToneCurve * InCurve)
{
	cmsToneCurve * out;
	double a = 0, b = 0, y, x1, y1, x2, y2;
	int i, j;
	int Ascending;

	assert(InCurve != NULL);

	// Try to reverse it analytically whatever possible

	if(InCurve->nSegments == 1 && InCurve->Segments[0].Type > 0 &&
	    /* InCurve -> Segments[0].Type <= 5 */
	    GetParametricCurveByType(InCurve->InterpParams->ContextID, InCurve->Segments[0].Type, NULL) != NULL) {
		return cmsBuildParametricToneCurve(InCurve->InterpParams->ContextID,
			   -(InCurve->Segments[0].Type),
			   InCurve->Segments[0].Params);
	}

	// Nope, reverse the table.
	out = cmsBuildTabulatedToneCurve16(InCurve->InterpParams->ContextID, nResultSamples, NULL);
	if(!out)
		return NULL;

	// We want to know if this is an ascending or descending table
	Ascending = !cmsIsToneCurveDescending(InCurve);

	// Iterate across Y axis
	for(i = 0; i < (int)nResultSamples; i++) {
		y = (double)i * 65535.0 / (nResultSamples - 1);

		// Find interval in which y is within.
		j = GetInterval(y, InCurve->Table16, InCurve->InterpParams);
		if(j >= 0) {
			// Get limits of interval
			x1 = InCurve->Table16[j];
			x2 = InCurve->Table16[j+1];

			y1 = (double)(j * 65535.0) / (InCurve->nEntries - 1);
			y2 = (double)((j+1) * 65535.0) / (InCurve->nEntries - 1);

			// If collapsed, then use any
			if(x1 == x2) {
				out->Table16[i] = _cmsQuickSaturateWord(Ascending ? y2 : y1);
				continue;
			}
			else {
				// Interpolate
				a = (y2 - y1) / (x2 - x1);
				b = y2 - a * x2;
			}
		}

		out->Table16[i] = _cmsQuickSaturateWord(a* y + b);
	}

	return out;
}

// Reverse a gamma table
cmsToneCurve * CMSEXPORT cmsReverseToneCurve(const cmsToneCurve * InGamma)
{
	assert(InGamma != NULL);

	return cmsReverseToneCurveEx(4096, InGamma);
}

// From: Eilers, P.H.C. (1994) Smoothing and interpolation with finite
// differences. in: Graphic Gems IV, Heckbert, P.S. (ed.), Academic press.
//
// Smoothing and interpolation with second differences.
//
//   Input:  weights (w), data (y): vector from 1 to m.
//   Input:  smoothing parameter (lambda), length (m).
//   Output: smoothed vector (z): vector from 1 to m.

static boolint smooth2(cmsContext ContextID, float w[], float y[], float z[], float lambda, int m)
{
	int i, i1, i2;
	boolint st;
	float * c = (float *)_cmsCalloc(ContextID, MAX_NODES_IN_CURVE, sizeof(float));
	float * d = (float *)_cmsCalloc(ContextID, MAX_NODES_IN_CURVE, sizeof(float));
	float * e = (float *)_cmsCalloc(ContextID, MAX_NODES_IN_CURVE, sizeof(float));
	if(c != NULL && d != NULL && e != NULL) {
		d[1] = w[1] + lambda;
		c[1] = -2 * lambda / d[1];
		e[1] = lambda /d[1];
		z[1] = w[1] * y[1];
		d[2] = w[2] + 5 * lambda - d[1] * c[1] *  c[1];
		c[2] = (-4 * lambda - d[1] * c[1] * e[1]) / d[2];
		e[2] = lambda / d[2];
		z[2] = w[2] * y[2] - c[1] * z[1];

		for(i = 3; i < m - 1; i++) {
			i1 = i - 1; i2 = i - 2;
			d[i] = w[i] + 6 * lambda - c[i1] * c[i1] * d[i1] - e[i2] * e[i2] * d[i2];
			c[i] = (-4 * lambda -d[i1] * c[i1] * e[i1])/ d[i];
			e[i] = lambda / d[i];
			z[i] = w[i] * y[i] - c[i1] * z[i1] - e[i2] * z[i2];
		}

		i1 = m - 2; i2 = m - 3;

		d[m - 1] = w[m - 1] + 5 * lambda -c[i1] * c[i1] * d[i1] - e[i2] * e[i2] * d[i2];
		c[m - 1] = (-2 * lambda - d[i1] * c[i1] * e[i1]) / d[m - 1];
		z[m - 1] = w[m - 1] * y[m - 1] - c[i1] * z[i1] - e[i2] * z[i2];
		i1 = m - 1; i2 = m - 2;

		d[m] = w[m] + lambda - c[i1] * c[i1] * d[i1] - e[i2] * e[i2] * d[i2];
		z[m] = (w[m] * y[m] - c[i1] * z[i1] - e[i2] * z[i2]) / d[m];
		z[m - 1] = z[m - 1] / d[m - 1] - c[m - 1] * z[m];

		for(i = m - 2; 1<= i; i--)
			z[i] = z[i] / d[i] - c[i] * z[i+1] - e[i] * z[i+2];
		st = TRUE;
	}
	else 
		st = FALSE;
	_cmsFree(ContextID, c);
	_cmsFree(ContextID, d);
	_cmsFree(ContextID, e);
	return st;
}

// Smooths a curve sampled at regular intervals.
boolint CMSEXPORT cmsSmoothToneCurve(cmsToneCurve * Tab, double lambda)
{
	boolint SuccessStatus = TRUE;
	float * w, * y, * z;
	uint32 i, nItems, Zeros, Poles;
	if(Tab != NULL && Tab->InterpParams != NULL) {
		cmsContext ContextID = Tab->InterpParams->ContextID;
		if(!cmsIsToneCurveLinear(Tab)) { // Only non-linear curves need smoothing
			nItems = Tab->nEntries;
			if(nItems < MAX_NODES_IN_CURVE) {
				// Allocate one more item than needed
				w = (float *)_cmsCalloc(ContextID, nItems + 1, sizeof(float));
				y = (float *)_cmsCalloc(ContextID, nItems + 1, sizeof(float));
				z = (float *)_cmsCalloc(ContextID, nItems + 1, sizeof(float));
				if(w != NULL && y != NULL && z != NULL) { // Ensure no memory allocation failure
					memzero(w, (nItems + 1) * sizeof(float));
					memzero(y, (nItems + 1) * sizeof(float));
					memzero(z, (nItems + 1) * sizeof(float));
					for(i = 0; i < nItems; i++) {
						y[i+1] = (float)Tab->Table16[i];
						w[i+1] = 1.0;
					}
					if(smooth2(ContextID, w, y, z, (float)lambda, (int)nItems)) {
						// Do some reality - checking...

						Zeros = Poles = 0;
						for(i = nItems; i > 1; --i) {
							if(z[i] == 0.) Zeros++;
							if(z[i] >= 65535.) Poles++;
							if(z[i] < z[i - 1]) {
								cmsSignalError(ContextID, cmsERROR_RANGE, "cmsSmoothToneCurve: Non-Monotonic.");
								SuccessStatus = FALSE;
								break;
							}
						}
						if(SuccessStatus && Zeros > (nItems / 3)) {
							cmsSignalError(ContextID, cmsERROR_RANGE, "cmsSmoothToneCurve: Degenerated, mostly zeros.");
							SuccessStatus = FALSE;
						}
						if(SuccessStatus && Poles > (nItems / 3)) {
							cmsSignalError(ContextID, cmsERROR_RANGE, "cmsSmoothToneCurve: Degenerated, mostly poles.");
							SuccessStatus = FALSE;
						}
						if(SuccessStatus) { // Seems ok
							for(i = 0; i < nItems; i++) {
								// Clamp to uint16
								Tab->Table16[i] = _cmsQuickSaturateWord(z[i+1]);
							}
						}
					}
					else { // Could not smooth
						cmsSignalError(ContextID, cmsERROR_RANGE, "cmsSmoothToneCurve: Function smooth2 failed.");
						SuccessStatus = FALSE;
					}
				}
				else { // One or more buffers could not be allocated
					cmsSignalError(ContextID, cmsERROR_RANGE, "cmsSmoothToneCurve: Could not allocate memory.");
					SuccessStatus = FALSE;
				}
				_cmsFree(ContextID, z);
				_cmsFree(ContextID, y);
				_cmsFree(ContextID, w);
			}
			else { // too many items in the table
				cmsSignalError(ContextID, cmsERROR_RANGE, "cmsSmoothToneCurve: Too many points.");
				SuccessStatus = FALSE;
			}
		}
	}
	else { // Tab parameter or Tab->InterpParams is NULL
		// Can't signal an error here since the ContextID is not known at this point
		SuccessStatus = FALSE;
	}
	return SuccessStatus;
}

// Is a table linear? Do not use parametric since we cannot guarantee some weird parameters resulting
// in a linear table. This way assures it is linear in 12 bits, which should be enough in most cases.
boolint CMSEXPORT cmsIsToneCurveLinear(const cmsToneCurve * Curve)
{
	int i;
	int diff;

	assert(Curve != NULL);

	for(i = 0; i < (int)Curve->nEntries; i++) {
		diff = abs((int)Curve->Table16[i] - (int)_cmsQuantizeVal(i, Curve->nEntries));
		if(diff > 0x0f)
			return FALSE;
	}

	return TRUE;
}

// Same, but for monotonicity
boolint CMSEXPORT cmsIsToneCurveMonotonic(const cmsToneCurve * t)
{
	uint32 n;
	int i, last;
	boolint lDescending;

	assert(t);

	// Degenerated curves are monotonic? Ok, let's pass them
	n = t->nEntries;
	if(n < 2) return TRUE;

	// Curve direction
	lDescending = cmsIsToneCurveDescending(t);

	if(lDescending) {
		last = t->Table16[0];

		for(i = 1; i < (int)n; i++) {
			if(t->Table16[i] - last > 2) // We allow some ripple
				return FALSE;
			else
				last = t->Table16[i];
		}
	}
	else {
		last = t->Table16[n-1];

		for(i = (int)n - 2; i >= 0; --i) {
			if(t->Table16[i] - last > 2)
				return FALSE;
			else
				last = t->Table16[i];
		}
	}

	return TRUE;
}

// Same, but for descending tables
boolint CMSEXPORT cmsIsToneCurveDescending(const cmsToneCurve * t)
{
	assert(t);

	return t->Table16[0] > t->Table16[t->nEntries-1];
}

// Another info fn: is out gamma table multisegment?
boolint CMSEXPORT cmsIsToneCurveMultisegment(const cmsToneCurve * t)
{
	assert(t);

	return t->nSegments > 1;
}

int32 CMSEXPORT cmsGetToneCurveParametricType(const cmsToneCurve * t)
{
	assert(t);

	if(t->nSegments != 1) return 0;
	return t->Segments[0].Type;
}

// We need accuracy this time
float CMSEXPORT cmsEvalToneCurveFloat(const cmsToneCurve * Curve, float v)
{
	assert(Curve != NULL);
	// Check for 16 bits table. If so, this is a limited-precision tone curve
	if(Curve->nSegments == 0) {
		uint16 In = (uint16)_cmsQuickSaturateWord(v * 65535.0);
		uint16 Out = cmsEvalToneCurve16(Curve, In);
		return (float)(Out / 65535.0);
	}
	return (float)EvalSegmentedFn(Curve, v);
}

// We need xput over here
uint16 CMSEXPORT cmsEvalToneCurve16(const cmsToneCurve * Curve, uint16 v)
{
	uint16 out;
	assert(Curve != NULL);
	Curve->InterpParams->Interpolation.Lerp16(&v, &out, Curve->InterpParams);
	return out;
}

// Least squares fitting.
// A mathematical procedure for finding the best-fitting curve to a given set of points by
// minimizing the sum of the squares of the offsets ("the residuals") of the points from the curve.
// The sum of the squares of the offsets is used instead of the offset absolute values because
// this allows the residuals to be treated as a continuous differentiable quantity.
//
// y = f(x) = x ^ g
//
// R  = (yi - (xi^g))
// R2 = (yi - (xi^g))2
// SUM R2 = SUM (yi - (xi^g))2
//
// dR2/dg = -2 SUM x^g log(x)(y - x^g)
// solving for dR2/dg = 0
//
// g = 1/n * SUM(log(y) / log(x))

double CMSEXPORT cmsEstimateGamma(const cmsToneCurve * t, double Precision)
{
	double gamma, sum, sum2;
	double n, x, y, Std;
	uint32 i;
	assert(t);
	sum = sum2 = n = 0;
	// Excluding endpoints
	for(i = 1; i < (MAX_NODES_IN_CURVE-1); i++) {
		x = (double)i / (MAX_NODES_IN_CURVE-1);
		y = (double)cmsEvalToneCurveFloat(t, (float)x);

		// Avoid 7% on lower part to prevent
		// artifacts due to linear ramps

		if(y > 0. && y < 1. && x > 0.07) {
			gamma = log(y) / log(x);
			sum  += gamma;
			sum2 += gamma * gamma;
			n++;
		}
	}

	// Take a look on SD to see if gamma isn't exponential at all
	Std = sqrt((n * sum2 - sum * sum) / (n*(n-1)));

	if(Std > Precision)
		return -1.0;

	return (sum / n); // The mean
}

// Retrieve parameters on one-segment tone curves

double * CMSEXPORT cmsGetToneCurveParams(const cmsToneCurve * t)
{
	assert(t);

	if(t->nSegments != 1) return NULL;
	return t->Segments[0].Params;
}
