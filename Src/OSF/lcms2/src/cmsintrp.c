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

// This module incorporates several interpolation routines, for 1 to 8 channels on input and
// up to 65535 channels on output. The user may change those by using the interpolation plug-in

// Some people may want to compile as C++ with all warnings on, in this case make compiler silent
#ifdef _MSC_VER
#    if(_MSC_VER >= 1400)
#       pragma warning( disable : 4365 )
#    endif
#endif

// Interpolation routines by default
static cmsInterpFunction DefaultInterpolatorsFactory(uint32 nInputChannels, uint32 nOutputChannels, uint32 dwFlags);

// This is the default factory
_cmsInterpPluginChunkType _cmsInterpPluginChunk = { NULL };

// The interpolation plug-in memory chunk allocator/dup
void _cmsAllocInterpPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	void * from;
	_cmsAssert(ctx != NULL);
	if(src != NULL) {
		from = src->chunks[InterpPlugin];
	}
	else {
		static _cmsInterpPluginChunkType InterpPluginChunk = { NULL };
		from = &InterpPluginChunk;
	}
	_cmsAssert(from != NULL);
	ctx->chunks[InterpPlugin] = _cmsSubAllocDup(ctx->MemPool, from, sizeof(_cmsInterpPluginChunkType));
}

// Main plug-in entry
boolint _cmsRegisterInterpPlugin(cmsContext ContextID, cmsPluginBase* Data)
{
	cmsPluginInterpolation* Plugin = (cmsPluginInterpolation*)Data;
	_cmsInterpPluginChunkType* ptr = (_cmsInterpPluginChunkType*)_cmsContextGetClientChunk(ContextID, InterpPlugin);

	if(Data == NULL) {
		ptr->Interpolators = NULL;
		return TRUE;
	}

	// Set replacement functions
	ptr->Interpolators = Plugin->InterpolatorsFactory;
	return TRUE;
}

// Set the interpolation method
boolint _cmsSetInterpolationRoutine(cmsContext ContextID, cmsInterpParams* p)
{
	_cmsInterpPluginChunkType* ptr = (_cmsInterpPluginChunkType*)_cmsContextGetClientChunk(ContextID, InterpPlugin);

	p->Interpolation.Lerp16 = NULL;

	// Invoke factory, possibly in the Plug-in
	if(ptr->Interpolators != NULL)
		p->Interpolation = ptr->Interpolators(p->nInputs, p->nOutputs, p->dwFlags);

	// If unsupported by the plug-in, go for the LittleCMS default.
	// If happens only if an extern plug-in is being used
	if(p->Interpolation.Lerp16 == NULL)
		p->Interpolation = DefaultInterpolatorsFactory(p->nInputs, p->nOutputs, p->dwFlags);

	// Check for valid interpolator (we just check one member of the union)
	if(p->Interpolation.Lerp16 == NULL) {
		return FALSE;
	}

	return TRUE;
}

// This function precalculates as many parameters as possible to speed up the interpolation.
cmsInterpParams* _cmsComputeInterpParamsEx(cmsContext ContextID,
    const uint32 nSamples[],
    uint32 InputChan, uint32 OutputChan,
    const void * Table,
    uint32 dwFlags)
{
	cmsInterpParams* p;
	uint32 i;

	// Check for maximum inputs
	if(InputChan > MAX_INPUT_DIMENSIONS) {
		cmsSignalError(ContextID, cmsERROR_RANGE, "Too many input channels (%d channels, max=%d)", InputChan, MAX_INPUT_DIMENSIONS);
		return NULL;
	}

	// Creates an empty object
	p = (cmsInterpParams*)_cmsMallocZero(ContextID, sizeof(cmsInterpParams));
	if(!p) return NULL;

	// Keep original parameters
	p->dwFlags  = dwFlags;
	p->nInputs  = InputChan;
	p->nOutputs = OutputChan;
	p->Table     = Table;
	p->ContextID  = ContextID;

	// Fill samples per input direction and domain (which is number of nodes minus one)
	for(i = 0; i < InputChan; i++) {
		p->nSamples[i] = nSamples[i];
		p->Domain[i]   = nSamples[i] - 1;
	}

	// Compute factors to apply to each component to index the grid array
	p->opta[0] = p->nOutputs;
	for(i = 1; i < InputChan; i++)
		p->opta[i] = p->opta[i-1] * nSamples[InputChan-i];

	if(!_cmsSetInterpolationRoutine(ContextID, p)) {
		cmsSignalError(ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unsupported interpolation (%d->%d channels)", InputChan, OutputChan);
		_cmsFree(ContextID, p);
		return NULL;
	}

	// All seems ok
	return p;
}

// This one is a wrapper on the anterior, but assuming all directions have same number of nodes
cmsInterpParams* CMSEXPORT _cmsComputeInterpParams(cmsContext ContextID, uint32 nSamples,
    uint32 InputChan, uint32 OutputChan, const void * Table, uint32 dwFlags)
{
	int i;
	uint32 Samples[MAX_INPUT_DIMENSIONS];

	// Fill the auxiliary array
	for(i = 0; i < MAX_INPUT_DIMENSIONS; i++)
		Samples[i] = nSamples;

	// Call the extended function
	return _cmsComputeInterpParamsEx(ContextID, Samples, InputChan, OutputChan, Table, dwFlags);
}

// Free all associated memory
void CMSEXPORT _cmsFreeInterpParams(cmsInterpParams* p)
{
	if(p) _cmsFree(p->ContextID, p);
}

// Inline fixed point interpolation
cmsINLINE CMS_NO_SANITIZE uint16 LinearInterp(cmsS15Fixed16Number a, cmsS15Fixed16Number l, cmsS15Fixed16Number h)
{
	uint32 dif = (uint32)(h - l) * a + 0x8000;
	dif = (dif >> 16) + l;
	return (uint16)(dif);
}

//  Linear interpolation (Fixed-point optimized)
static void LinLerp1D(const uint16 Value[], uint16 Output[], const cmsInterpParams* p)
{
	uint16 y1, y0;
	int cell0, rest;
	int val3;
	const uint16* LutTable = (uint16*)p->Table;
	// if last value...
	if(Value[0] == 0xffff) {
		Output[0] = LutTable[p->Domain[0]];
	}
	else {
		val3 = p->Domain[0] * Value[0];
		val3 = _cmsToFixedDomain(val3); // To fixed 15.16

		cell0 = FIXED_TO_INT(val3);     // Cell is 16 MSB bits
		rest = FIXED_REST_TO_INT(val3); // Rest is 16 LSB bits

		y0 = LutTable[cell0];
		y1 = LutTable[cell0 + 1];

		Output[0] = LinearInterp(rest, y0, y1);
	}
}

// To prevent out of bounds indexing
cmsINLINE float fclamp(float v)
{
	return ((v < 1.0e-9f) || isnan(v)) ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// Floating-point version of 1D interpolation
static void LinLerp1Dfloat(const float Value[], float Output[], const cmsInterpParams* p)
{
	float y1, y0;
	float rest;
	int cell0, cell1;
	const float * LutTable = (float *)p->Table;
	float val2 = fclamp(Value[0]);
	// if last value...
	if(val2 == 1.0) {
		Output[0] = LutTable[p->Domain[0]];
	}
	else {
		val2 *= p->Domain[0];
		cell0 = (int)floor(val2);
		cell1 = (int)ceil(val2);
		// Rest is 16 LSB bits
		rest = val2 - cell0;
		y0 = LutTable[cell0];
		y1 = LutTable[cell1];
		Output[0] = y0 + (y1 - y0) * rest;
	}
}

// Eval gray LUT having only one input channel
static CMS_NO_SANITIZE void Eval1Input(const uint16 Input[], uint16 Output[], const cmsInterpParams* p16)
{
	cmsS15Fixed16Number fk;
	cmsS15Fixed16Number k0, k1, rk, K0, K1;
	int v;
	uint32 OutChan;
	const uint16* LutTable = (uint16*)p16->Table;
	v = Input[0] * p16->Domain[0];
	fk = _cmsToFixedDomain(v);
	k0 = FIXED_TO_INT(fk);
	rk = (uint16)FIXED_REST_TO_INT(fk);
	k1 = k0 + (Input[0] != 0xFFFFU ? 1 : 0);
	K0 = p16->opta[0] * k0;
	K1 = p16->opta[0] * k1;
	for(OutChan = 0; OutChan < p16->nOutputs; OutChan++) {
		Output[OutChan] = LinearInterp(rk, LutTable[K0+OutChan], LutTable[K1+OutChan]);
	}
}

// Eval gray LUT having only one input channel
static void Eval1InputFloat(const float Value[], float Output[], const cmsInterpParams* p)
{
	float y1, y0;
	float rest;
	int cell0, cell1;
	uint32 OutChan;
	const float* LutTable = (float *)p->Table;
	float val2 = fclamp(Value[0]);
	// if last value...
	if(val2 == 1.0) {
		y0 = LutTable[p->Domain[0]];

		for(OutChan = 0; OutChan < p->nOutputs; OutChan++) {
			Output[OutChan] = y0;
		}
	}
	else {
		val2 *= p->Domain[0];

		cell0 = (int)floor(val2);
		cell1 = (int)ceil(val2);

		// Rest is 16 LSB bits
		rest = val2 - cell0;

		cell0 *= p->opta[0];
		cell1 *= p->opta[0];

		for(OutChan = 0; OutChan < p->nOutputs; OutChan++) {
			y0 = LutTable[cell0 + OutChan];
			y1 = LutTable[cell1 + OutChan];

			Output[OutChan] = y0 + (y1 - y0) * rest;
		}
	}
}

// Bilinear interpolation (16 bits) - float version
static void BilinearInterpFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
#define LERP(a, l, h)    (float)((l)+(((h)-(l))*(a)))
#define DENS(i, j)      (LutTable[(i)+(j)+OutChan])
	const float* LutTable = (float *)p->Table;
	float px, py;
	int x0, y0,
	    X0, Y0, X1, Y1;
	int TotalOut, OutChan;
	float fx, fy,
	    d00, d01, d10, d11,
	    dx0, dx1,
	    dxy;
	TotalOut   = p->nOutputs;
	px = fclamp(Input[0]) * p->Domain[0];
	py = fclamp(Input[1]) * p->Domain[1];
	x0 = (int)_cmsQuickFloor(px); fx = px - (float)x0;
	y0 = (int)_cmsQuickFloor(py); fy = py - (float)y0;
	X0 = p->opta[1] * x0;
	X1 = X0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[1]);
	Y0 = p->opta[0] * y0;
	Y1 = Y0 + (fclamp(Input[1]) >= 1.0 ? 0 : p->opta[0]);
	for(OutChan = 0; OutChan < TotalOut; OutChan++) {
		d00 = DENS(X0, Y0);
		d01 = DENS(X0, Y1);
		d10 = DENS(X1, Y0);
		d11 = DENS(X1, Y1);
		dx0 = LERP(fx, d00, d10);
		dx1 = LERP(fx, d01, d11);
		dxy = LERP(fy, dx0, dx1);
		Output[OutChan] = dxy;
	}
#undef LERP
#undef DENS
}

// Bilinear interpolation (16 bits) - optimized version
static CMS_NO_SANITIZE void BilinearInterp16(const uint16 Input[], uint16 Output[], const cmsInterpParams* p)
{
#define DENS(i, j) (LutTable[(i)+(j)+OutChan])
#define LERP(a, l, h)     (uint16)(l + ROUND_FIXED_TO_INT(((h-l)*a)))

	const uint16* LutTable = (uint16*)p->Table;
	int OutChan, TotalOut;
	cmsS15Fixed16Number fx, fy;
	int rx, ry;
	int x0, y0;
	int X0, X1, Y0, Y1;
	int d00, d01, d10, d11,
	    dx0, dx1,
	    dxy;

	TotalOut   = p->nOutputs;

	fx = _cmsToFixedDomain((int)Input[0] * p->Domain[0]);
	x0  = FIXED_TO_INT(fx);
	rx  = FIXED_REST_TO_INT(fx);// Rest in 0..1.0 domain

	fy = _cmsToFixedDomain((int)Input[1] * p->Domain[1]);
	y0  = FIXED_TO_INT(fy);
	ry  = FIXED_REST_TO_INT(fy);

	X0 = p->opta[1] * x0;
	X1 = X0 + (Input[0] == 0xFFFFU ? 0 : p->opta[1]);

	Y0 = p->opta[0] * y0;
	Y1 = Y0 + (Input[1] == 0xFFFFU ? 0 : p->opta[0]);

	for(OutChan = 0; OutChan < TotalOut; OutChan++) {
		d00 = DENS(X0, Y0);
		d01 = DENS(X0, Y1);
		d10 = DENS(X1, Y0);
		d11 = DENS(X1, Y1);

		dx0 = LERP(rx, d00, d10);
		dx1 = LERP(rx, d01, d11);

		dxy = LERP(ry, dx0, dx1);

		Output[OutChan] = (uint16)dxy;
	}

#   undef LERP
#   undef DENS
}

// Trilinear interpolation (16 bits) - float version
static void TrilinearInterpFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
#define LERP(a, l, h)      (float)((l)+(((h)-(l))*(a)))
#define DENS(i, j, k)      (LutTable[(i)+(j)+(k)+OutChan])

	const float* LutTable = (float *)p->Table;
	float px, py, pz;
	int x0, y0, z0,
	    X0, Y0, Z0, X1, Y1, Z1;
	int TotalOut, OutChan;
	float fx, fy, fz,
	    d000, d001, d010, d011,
	    d100, d101, d110, d111,
	    dx00, dx01, dx10, dx11,
	    dxy0, dxy1, dxyz;

	TotalOut   = p->nOutputs;

	// We need some clipping here
	px = fclamp(Input[0]) * p->Domain[0];
	py = fclamp(Input[1]) * p->Domain[1];
	pz = fclamp(Input[2]) * p->Domain[2];

	x0 = (int)floor(px); fx = px - (float)x0; // We need full floor funcionality here
	y0 = (int)floor(py); fy = py - (float)y0;
	z0 = (int)floor(pz); fz = pz - (float)z0;

	X0 = p->opta[2] * x0;
	X1 = X0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[2]);

	Y0 = p->opta[1] * y0;
	Y1 = Y0 + (fclamp(Input[1]) >= 1.0 ? 0 : p->opta[1]);

	Z0 = p->opta[0] * z0;
	Z1 = Z0 + (fclamp(Input[2]) >= 1.0 ? 0 : p->opta[0]);

	for(OutChan = 0; OutChan < TotalOut; OutChan++) {
		d000 = DENS(X0, Y0, Z0);
		d001 = DENS(X0, Y0, Z1);
		d010 = DENS(X0, Y1, Z0);
		d011 = DENS(X0, Y1, Z1);

		d100 = DENS(X1, Y0, Z0);
		d101 = DENS(X1, Y0, Z1);
		d110 = DENS(X1, Y1, Z0);
		d111 = DENS(X1, Y1, Z1);

		dx00 = LERP(fx, d000, d100);
		dx01 = LERP(fx, d001, d101);
		dx10 = LERP(fx, d010, d110);
		dx11 = LERP(fx, d011, d111);

		dxy0 = LERP(fy, dx00, dx10);
		dxy1 = LERP(fy, dx01, dx11);

		dxyz = LERP(fz, dxy0, dxy1);

		Output[OutChan] = dxyz;
	}

#   undef LERP
#   undef DENS
}

// Trilinear interpolation (16 bits) - optimized version
static CMS_NO_SANITIZE void TrilinearInterp16(const uint16 Input[], uint16 Output[], const cmsInterpParams* p)
{
#define DENS(i, j, k) (LutTable[(i)+(j)+(k)+OutChan])
#define LERP(a, l, h)     (uint16)(l + ROUND_FIXED_TO_INT(((h-l)*a)))

	const uint16* LutTable = (uint16*)p->Table;
	int OutChan, TotalOut;
	cmsS15Fixed16Number fx, fy, fz;
	int rx, ry, rz;
	int x0, y0, z0;
	int X0, X1, Y0, Y1, Z0, Z1;
	int d000, d001, d010, d011,
	    d100, d101, d110, d111,
	    dx00, dx01, dx10, dx11,
	    dxy0, dxy1, dxyz;

	TotalOut   = p->nOutputs;

	fx = _cmsToFixedDomain((int)Input[0] * p->Domain[0]);
	x0  = FIXED_TO_INT(fx);
	rx  = FIXED_REST_TO_INT(fx);// Rest in 0..1.0 domain

	fy = _cmsToFixedDomain((int)Input[1] * p->Domain[1]);
	y0  = FIXED_TO_INT(fy);
	ry  = FIXED_REST_TO_INT(fy);

	fz = _cmsToFixedDomain((int)Input[2] * p->Domain[2]);
	z0 = FIXED_TO_INT(fz);
	rz = FIXED_REST_TO_INT(fz);

	X0 = p->opta[2] * x0;
	X1 = X0 + (Input[0] == 0xFFFFU ? 0 : p->opta[2]);

	Y0 = p->opta[1] * y0;
	Y1 = Y0 + (Input[1] == 0xFFFFU ? 0 : p->opta[1]);

	Z0 = p->opta[0] * z0;
	Z1 = Z0 + (Input[2] == 0xFFFFU ? 0 : p->opta[0]);

	for(OutChan = 0; OutChan < TotalOut; OutChan++) {
		d000 = DENS(X0, Y0, Z0);
		d001 = DENS(X0, Y0, Z1);
		d010 = DENS(X0, Y1, Z0);
		d011 = DENS(X0, Y1, Z1);

		d100 = DENS(X1, Y0, Z0);
		d101 = DENS(X1, Y0, Z1);
		d110 = DENS(X1, Y1, Z0);
		d111 = DENS(X1, Y1, Z1);

		dx00 = LERP(rx, d000, d100);
		dx01 = LERP(rx, d001, d101);
		dx10 = LERP(rx, d010, d110);
		dx11 = LERP(rx, d011, d111);

		dxy0 = LERP(ry, dx00, dx10);
		dxy1 = LERP(ry, dx01, dx11);

		dxyz = LERP(rz, dxy0, dxy1);

		Output[OutChan] = (uint16)dxyz;
	}

#   undef LERP
#   undef DENS
}

// Tetrahedral interpolation, using Sakamoto algorithm.
#define DENS(i, j, k) (LutTable[(i)+(j)+(k)+OutChan])
static void TetrahedralInterpFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
	const float* LutTable = (float *)p->Table;
	float px, py, pz;
	int x0, y0, z0,
	    X0, Y0, Z0, X1, Y1, Z1;
	float rx, ry, rz;
	float c0, c1 = 0, c2 = 0, c3 = 0;
	int OutChan, TotalOut;

	TotalOut   = p->nOutputs;

	// We need some clipping here
	px = fclamp(Input[0]) * p->Domain[0];
	py = fclamp(Input[1]) * p->Domain[1];
	pz = fclamp(Input[2]) * p->Domain[2];

	x0 = (int)floor(px); rx = (px - (float)x0); // We need full floor functionality here
	y0 = (int)floor(py); ry = (py - (float)y0);
	z0 = (int)floor(pz); rz = (pz - (float)z0);

	X0 = p->opta[2] * x0;
	X1 = X0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[2]);

	Y0 = p->opta[1] * y0;
	Y1 = Y0 + (fclamp(Input[1]) >= 1.0 ? 0 : p->opta[1]);

	Z0 = p->opta[0] * z0;
	Z1 = Z0 + (fclamp(Input[2]) >= 1.0 ? 0 : p->opta[0]);

	for(OutChan = 0; OutChan < TotalOut; OutChan++) {
		// These are the 6 Tetrahedral

		c0 = DENS(X0, Y0, Z0);

		if(rx >= ry && ry >= rz) {
			c1 = DENS(X1, Y0, Z0) - c0;
			c2 = DENS(X1, Y1, Z0) - DENS(X1, Y0, Z0);
			c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
		}
		else if(rx >= rz && rz >= ry) {
			c1 = DENS(X1, Y0, Z0) - c0;
			c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
			c3 = DENS(X1, Y0, Z1) - DENS(X1, Y0, Z0);
		}
		else if(rz >= rx && rx >= ry) {
			c1 = DENS(X1, Y0, Z1) - DENS(X0, Y0, Z1);
			c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
			c3 = DENS(X0, Y0, Z1) - c0;
		}
		else if(ry >= rx && rx >= rz) {
			c1 = DENS(X1, Y1, Z0) - DENS(X0, Y1, Z0);
			c2 = DENS(X0, Y1, Z0) - c0;
			c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
		}
		else if(ry >= rz && rz >= rx) {
			c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
			c2 = DENS(X0, Y1, Z0) - c0;
			c3 = DENS(X0, Y1, Z1) - DENS(X0, Y1, Z0);
		}
		else if(rz >= ry && ry >= rx) {
			c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
			c2 = DENS(X0, Y1, Z1) - DENS(X0, Y0, Z1);
			c3 = DENS(X0, Y0, Z1) - c0;
		}
		else {
			c1 = c2 = c3 = 0;
		}

		Output[OutChan] = c0 + c1 * rx + c2 * ry + c3 * rz;
	}
}

#undef DENS

static CMS_NO_SANITIZE void TetrahedralInterp16(const uint16 Input[], uint16 Output[], const cmsInterpParams* p)
{
	const uint16* LutTable = (uint16*)p->Table;
	cmsS15Fixed16Number fx, fy, fz;
	cmsS15Fixed16Number rx, ry, rz;
	int x0, y0, z0;
	cmsS15Fixed16Number c0, c1, c2, c3, Rest;
	cmsS15Fixed16Number X0, X1, Y0, Y1, Z0, Z1;
	uint32 TotalOut = p->nOutputs;

	fx = _cmsToFixedDomain((int)Input[0] * p->Domain[0]);
	fy = _cmsToFixedDomain((int)Input[1] * p->Domain[1]);
	fz = _cmsToFixedDomain((int)Input[2] * p->Domain[2]);

	x0 = FIXED_TO_INT(fx);
	y0 = FIXED_TO_INT(fy);
	z0 = FIXED_TO_INT(fz);

	rx = FIXED_REST_TO_INT(fx);
	ry = FIXED_REST_TO_INT(fy);
	rz = FIXED_REST_TO_INT(fz);

	X0 = p->opta[2] * x0;
	X1 = (Input[0] == 0xFFFFU ? 0 : p->opta[2]);

	Y0 = p->opta[1] * y0;
	Y1 = (Input[1] == 0xFFFFU ? 0 : p->opta[1]);

	Z0 = p->opta[0] * z0;
	Z1 = (Input[2] == 0xFFFFU ? 0 : p->opta[0]);

	LutTable = &LutTable[X0+Y0+Z0];

	// Output should be computed as x = ROUND_FIXED_TO_INT(_cmsToFixedDomain(Rest))
	// which expands as: x = (Rest + ((Rest+0x7fff)/0xFFFF) + 0x8000)>>16
	// This can be replaced by: t = Rest+0x8001, x = (t + (t>>16))>>16
	// at the cost of being off by one at 7fff and 17ffe.

	if(rx >= ry) {
		if(ry >= rz) {
			Y1 += X1;
			Z1 += Y1;
			for(; TotalOut; TotalOut--) {
				c1 = LutTable[X1];
				c2 = LutTable[Y1];
				c3 = LutTable[Z1];
				c0 = *LutTable++;
				c3 -= c2;
				c2 -= c1;
				c1 -= c0;
				Rest = c1 * rx + c2 * ry + c3 * rz + 0x8001;
				*Output++ = (uint16)c0 + ((Rest + (Rest>>16))>>16);
			}
		}
		else if(rz >= rx) {
			X1 += Z1;
			Y1 += X1;
			for(; TotalOut; TotalOut--) {
				c1 = LutTable[X1];
				c2 = LutTable[Y1];
				c3 = LutTable[Z1];
				c0 = *LutTable++;
				c2 -= c1;
				c1 -= c3;
				c3 -= c0;
				Rest = c1 * rx + c2 * ry + c3 * rz + 0x8001;
				*Output++ = (uint16)c0 + ((Rest + (Rest>>16))>>16);
			}
		}
		else {
			Z1 += X1;
			Y1 += Z1;
			for(; TotalOut; TotalOut--) {
				c1 = LutTable[X1];
				c2 = LutTable[Y1];
				c3 = LutTable[Z1];
				c0 = *LutTable++;
				c2 -= c3;
				c3 -= c1;
				c1 -= c0;
				Rest = c1 * rx + c2 * ry + c3 * rz + 0x8001;
				*Output++ = (uint16)c0 + ((Rest + (Rest>>16))>>16);
			}
		}
	}
	else {
		if(rx >= rz) {
			X1 += Y1;
			Z1 += X1;
			for(; TotalOut; TotalOut--) {
				c1 = LutTable[X1];
				c2 = LutTable[Y1];
				c3 = LutTable[Z1];
				c0 = *LutTable++;
				c3 -= c1;
				c1 -= c2;
				c2 -= c0;
				Rest = c1 * rx + c2 * ry + c3 * rz + 0x8001;
				*Output++ = (uint16)c0 + ((Rest + (Rest>>16))>>16);
			}
		}
		else if(ry >= rz) {
			Z1 += Y1;
			X1 += Z1;
			for(; TotalOut; TotalOut--) {
				c1 = LutTable[X1];
				c2 = LutTable[Y1];
				c3 = LutTable[Z1];
				c0 = *LutTable++;
				c1 -= c3;
				c3 -= c2;
				c2 -= c0;
				Rest = c1 * rx + c2 * ry + c3 * rz + 0x8001;
				*Output++ = (uint16)c0 + ((Rest + (Rest>>16))>>16);
			}
		}
		else {
			Y1 += Z1;
			X1 += Y1;
			for(; TotalOut; TotalOut--) {
				c1 = LutTable[X1];
				c2 = LutTable[Y1];
				c3 = LutTable[Z1];
				c0 = *LutTable++;
				c1 -= c2;
				c2 -= c3;
				c3 -= c0;
				Rest = c1 * rx + c2 * ry + c3 * rz + 0x8001;
				*Output++ = (uint16)c0 + ((Rest + (Rest>>16))>>16);
			}
		}
	}
}

#define DENS(i, j, k) (LutTable[(i)+(j)+(k)+OutChan])
static CMS_NO_SANITIZE void Eval4Inputs(const uint16 Input[], uint16 Output[], const cmsInterpParams* p16)
{
	const uint16* LutTable;
	cmsS15Fixed16Number fk;
	cmsS15Fixed16Number k0, rk;
	int K0, K1;
	cmsS15Fixed16Number fx, fy, fz;
	cmsS15Fixed16Number rx, ry, rz;
	int x0, y0, z0;
	cmsS15Fixed16Number X0, X1, Y0, Y1, Z0, Z1;
	uint32 i;
	cmsS15Fixed16Number c0, c1, c2, c3, Rest;
	uint32 OutChan;
	uint16 Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];

	fk  = _cmsToFixedDomain((int)Input[0] * p16->Domain[0]);
	fx  = _cmsToFixedDomain((int)Input[1] * p16->Domain[1]);
	fy  = _cmsToFixedDomain((int)Input[2] * p16->Domain[2]);
	fz  = _cmsToFixedDomain((int)Input[3] * p16->Domain[3]);

	k0  = FIXED_TO_INT(fk);
	x0  = FIXED_TO_INT(fx);
	y0  = FIXED_TO_INT(fy);
	z0  = FIXED_TO_INT(fz);

	rk  = FIXED_REST_TO_INT(fk);
	rx  = FIXED_REST_TO_INT(fx);
	ry  = FIXED_REST_TO_INT(fy);
	rz  = FIXED_REST_TO_INT(fz);

	K0 = p16->opta[3] * k0;
	K1 = K0 + (Input[0] == 0xFFFFU ? 0 : p16->opta[3]);

	X0 = p16->opta[2] * x0;
	X1 = X0 + (Input[1] == 0xFFFFU ? 0 : p16->opta[2]);

	Y0 = p16->opta[1] * y0;
	Y1 = Y0 + (Input[2] == 0xFFFFU ? 0 : p16->opta[1]);

	Z0 = p16->opta[0] * z0;
	Z1 = Z0 + (Input[3] == 0xFFFFU ? 0 : p16->opta[0]);

	LutTable = (uint16*)p16->Table;
	LutTable += K0;

	for(OutChan = 0; OutChan < p16->nOutputs; OutChan++) {
		c0 = DENS(X0, Y0, Z0);

		if(rx >= ry && ry >= rz) {
			c1 = DENS(X1, Y0, Z0) - c0;
			c2 = DENS(X1, Y1, Z0) - DENS(X1, Y0, Z0);
			c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
		}
		else if(rx >= rz && rz >= ry) {
			c1 = DENS(X1, Y0, Z0) - c0;
			c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
			c3 = DENS(X1, Y0, Z1) - DENS(X1, Y0, Z0);
		}
		else if(rz >= rx && rx >= ry) {
			c1 = DENS(X1, Y0, Z1) - DENS(X0, Y0, Z1);
			c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
			c3 = DENS(X0, Y0, Z1) - c0;
		}
		else if(ry >= rx && rx >= rz) {
			c1 = DENS(X1, Y1, Z0) - DENS(X0, Y1, Z0);
			c2 = DENS(X0, Y1, Z0) - c0;
			c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
		}
		else if(ry >= rz && rz >= rx) {
			c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
			c2 = DENS(X0, Y1, Z0) - c0;
			c3 = DENS(X0, Y1, Z1) - DENS(X0, Y1, Z0);
		}
		else if(rz >= ry && ry >= rx) {
			c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
			c2 = DENS(X0, Y1, Z1) - DENS(X0, Y0, Z1);
			c3 = DENS(X0, Y0, Z1) - c0;
		}
		else {
			c1 = c2 = c3 = 0;
		}

		Rest = c1 * rx + c2 * ry + c3 * rz;

		Tmp1[OutChan] = (uint16)(c0 + ROUND_FIXED_TO_INT(_cmsToFixedDomain(Rest)));
	}

	LutTable = (uint16*)p16->Table;
	LutTable += K1;

	for(OutChan = 0; OutChan < p16->nOutputs; OutChan++) {
		c0 = DENS(X0, Y0, Z0);

		if(rx >= ry && ry >= rz) {
			c1 = DENS(X1, Y0, Z0) - c0;
			c2 = DENS(X1, Y1, Z0) - DENS(X1, Y0, Z0);
			c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
		}
		else if(rx >= rz && rz >= ry) {
			c1 = DENS(X1, Y0, Z0) - c0;
			c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
			c3 = DENS(X1, Y0, Z1) - DENS(X1, Y0, Z0);
		}
		else if(rz >= rx && rx >= ry) {
			c1 = DENS(X1, Y0, Z1) - DENS(X0, Y0, Z1);
			c2 = DENS(X1, Y1, Z1) - DENS(X1, Y0, Z1);
			c3 = DENS(X0, Y0, Z1) - c0;
		}
		else if(ry >= rx && rx >= rz) {
			c1 = DENS(X1, Y1, Z0) - DENS(X0, Y1, Z0);
			c2 = DENS(X0, Y1, Z0) - c0;
			c3 = DENS(X1, Y1, Z1) - DENS(X1, Y1, Z0);
		}
		else if(ry >= rz && rz >= rx) {
			c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
			c2 = DENS(X0, Y1, Z0) - c0;
			c3 = DENS(X0, Y1, Z1) - DENS(X0, Y1, Z0);
		}
		else if(rz >= ry && ry >= rx) {
			c1 = DENS(X1, Y1, Z1) - DENS(X0, Y1, Z1);
			c2 = DENS(X0, Y1, Z1) - DENS(X0, Y0, Z1);
			c3 = DENS(X0, Y0, Z1) - c0;
		}
		else {
			c1 = c2 = c3 = 0;
		}

		Rest = c1 * rx + c2 * ry + c3 * rz;

		Tmp2[OutChan] = (uint16)(c0 + ROUND_FIXED_TO_INT(_cmsToFixedDomain(Rest)));
	}

	for(i = 0; i < p16->nOutputs; i++) {
		Output[i] = LinearInterp(rk, Tmp1[i], Tmp2[i]);
	}
}

#undef DENS

// For more that 3 inputs (i.e., CMYK)
// evaluate two 3-dimensional interpolations and then linearly interpolate between them.

static void Eval4InputsFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
	const float* LutTable = (float *)p->Table;
	float rest;
	float pk;
	int k0, K0, K1;
	const float* T;
	uint32 i;
	float Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	pk = fclamp(Input[0]) * p->Domain[0];
	k0 = _cmsQuickFloor(pk);
	rest = pk - (float)k0;

	K0 = p->opta[3] * k0;
	K1 = K0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[3]);

	p1 = *p;
	memmove(&p1.Domain[0], &p->Domain[1], 3*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	TetrahedralInterpFloat(Input + 1,  Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;
	TetrahedralInterpFloat(Input + 1,  Tmp2, &p1);

	for(i = 0; i < p->nOutputs; i++) {
		float y0 = Tmp1[i];
		float y1 = Tmp2[i];

		Output[i] = y0 + (y1 - y0) * rest;
	}
}

static CMS_NO_SANITIZE void Eval5Inputs(const uint16 Input[], uint16 Output[], const cmsInterpParams* p16)
{
	const uint16* LutTable = (uint16*)p16->Table;
	cmsS15Fixed16Number fk;
	cmsS15Fixed16Number k0, rk;
	int K0, K1;
	const uint16* T;
	uint32 i;
	uint16 Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	fk = _cmsToFixedDomain((cmsS15Fixed16Number)Input[0] * p16->Domain[0]);
	k0 = FIXED_TO_INT(fk);
	rk = FIXED_REST_TO_INT(fk);

	K0 = p16->opta[4] * k0;
	K1 = p16->opta[4] * (k0 + (Input[0] != 0xFFFFU ? 1 : 0));

	p1 = *p16;
	memmove(&p1.Domain[0], &p16->Domain[1], 4*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval4Inputs(Input + 1, Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval4Inputs(Input + 1, Tmp2, &p1);

	for(i = 0; i < p16->nOutputs; i++) {
		Output[i] = LinearInterp(rk, Tmp1[i], Tmp2[i]);
	}
}

static void Eval5InputsFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
	const float* LutTable = (float *)p->Table;
	float rest;
	float pk;
	int k0, K0, K1;
	const float* T;
	uint32 i;
	float Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	pk = fclamp(Input[0]) * p->Domain[0];
	k0 = _cmsQuickFloor(pk);
	rest = pk - (float)k0;

	K0 = p->opta[4] * k0;
	K1 = K0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[4]);

	p1 = *p;
	memmove(&p1.Domain[0], &p->Domain[1], 4*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval4InputsFloat(Input + 1,  Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval4InputsFloat(Input + 1,  Tmp2, &p1);

	for(i = 0; i < p->nOutputs; i++) {
		float y0 = Tmp1[i];
		float y1 = Tmp2[i];

		Output[i] = y0 + (y1 - y0) * rest;
	}
}

static CMS_NO_SANITIZE void Eval6Inputs(const uint16 Input[], uint16 Output[], const cmsInterpParams* p16)
{
	const uint16* LutTable = (uint16*)p16->Table;
	cmsS15Fixed16Number fk;
	cmsS15Fixed16Number k0, rk;
	int K0, K1;
	const uint16* T;
	uint32 i;
	uint16 Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	fk = _cmsToFixedDomain((cmsS15Fixed16Number)Input[0] * p16->Domain[0]);
	k0 = FIXED_TO_INT(fk);
	rk = FIXED_REST_TO_INT(fk);

	K0 = p16->opta[5] * k0;
	K1 = p16->opta[5] * (k0 + (Input[0] != 0xFFFFU ? 1 : 0));

	p1 = *p16;
	memmove(&p1.Domain[0], &p16->Domain[1], 5*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval5Inputs(Input + 1, Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval5Inputs(Input + 1, Tmp2, &p1);

	for(i = 0; i < p16->nOutputs; i++) {
		Output[i] = LinearInterp(rk, Tmp1[i], Tmp2[i]);
	}
}

static void Eval6InputsFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
	const float* LutTable = (float *)p->Table;
	float rest;
	float pk;
	int k0, K0, K1;
	const float* T;
	uint32 i;
	float Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	pk = fclamp(Input[0]) * p->Domain[0];
	k0 = _cmsQuickFloor(pk);
	rest = pk - (float)k0;

	K0 = p->opta[5] * k0;
	K1 = K0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[5]);

	p1 = *p;
	memmove(&p1.Domain[0], &p->Domain[1], 5*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval5InputsFloat(Input + 1,  Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval5InputsFloat(Input + 1,  Tmp2, &p1);

	for(i = 0; i < p->nOutputs; i++) {
		float y0 = Tmp1[i];
		float y1 = Tmp2[i];

		Output[i] = y0 + (y1 - y0) * rest;
	}
}

static CMS_NO_SANITIZE void Eval7Inputs(const uint16 Input[], uint16 Output[], const cmsInterpParams* p16)
{
	const uint16* LutTable = (uint16*)p16->Table;
	cmsS15Fixed16Number fk;
	cmsS15Fixed16Number k0, rk;
	int K0, K1;
	const uint16* T;
	uint32 i;
	uint16 Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	fk = _cmsToFixedDomain((cmsS15Fixed16Number)Input[0] * p16->Domain[0]);
	k0 = FIXED_TO_INT(fk);
	rk = FIXED_REST_TO_INT(fk);

	K0 = p16->opta[6] * k0;
	K1 = p16->opta[6] * (k0 + (Input[0] != 0xFFFFU ? 1 : 0));

	p1 = *p16;
	memmove(&p1.Domain[0], &p16->Domain[1], 6*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval6Inputs(Input + 1, Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval6Inputs(Input + 1, Tmp2, &p1);

	for(i = 0; i < p16->nOutputs; i++) {
		Output[i] = LinearInterp(rk, Tmp1[i], Tmp2[i]);
	}
}

static void Eval7InputsFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
	const float* LutTable = (float *)p->Table;
	float rest;
	float pk;
	int k0, K0, K1;
	const float* T;
	uint32 i;
	float Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	pk = fclamp(Input[0]) * p->Domain[0];
	k0 = _cmsQuickFloor(pk);
	rest = pk - (float)k0;

	K0 = p->opta[6] * k0;
	K1 = K0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[6]);

	p1 = *p;
	memmove(&p1.Domain[0], &p->Domain[1], 6*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval6InputsFloat(Input + 1,  Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval6InputsFloat(Input + 1,  Tmp2, &p1);

	for(i = 0; i < p->nOutputs; i++) {
		float y0 = Tmp1[i];
		float y1 = Tmp2[i];

		Output[i] = y0 + (y1 - y0) * rest;
	}
}

static CMS_NO_SANITIZE void Eval8Inputs(const uint16 Input[], uint16 Output[], const cmsInterpParams* p16)
{
	const uint16* LutTable = (uint16*)p16->Table;
	cmsS15Fixed16Number fk;
	cmsS15Fixed16Number k0, rk;
	int K0, K1;
	const uint16* T;
	uint32 i;
	uint16 Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	fk = _cmsToFixedDomain((cmsS15Fixed16Number)Input[0] * p16->Domain[0]);
	k0 = FIXED_TO_INT(fk);
	rk = FIXED_REST_TO_INT(fk);

	K0 = p16->opta[7] * k0;
	K1 = p16->opta[7] * (k0 + (Input[0] != 0xFFFFU ? 1 : 0));

	p1 = *p16;
	memmove(&p1.Domain[0], &p16->Domain[1], 7*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval7Inputs(Input + 1, Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;
	Eval7Inputs(Input + 1, Tmp2, &p1);

	for(i = 0; i < p16->nOutputs; i++) {
		Output[i] = LinearInterp(rk, Tmp1[i], Tmp2[i]);
	}
}

static void Eval8InputsFloat(const float Input[], float Output[], const cmsInterpParams* p)
{
	const float* LutTable = (float *)p->Table;
	float rest;
	float pk;
	int k0, K0, K1;
	const float* T;
	uint32 i;
	float Tmp1[MAX_STAGE_CHANNELS], Tmp2[MAX_STAGE_CHANNELS];
	cmsInterpParams p1;

	pk = fclamp(Input[0]) * p->Domain[0];
	k0 = _cmsQuickFloor(pk);
	rest = pk - (float)k0;

	K0 = p->opta[7] * k0;
	K1 = K0 + (fclamp(Input[0]) >= 1.0 ? 0 : p->opta[7]);

	p1 = *p;
	memmove(&p1.Domain[0], &p->Domain[1], 7*sizeof(uint32));

	T = LutTable + K0;
	p1.Table = T;

	Eval7InputsFloat(Input + 1,  Tmp1, &p1);

	T = LutTable + K1;
	p1.Table = T;

	Eval7InputsFloat(Input + 1,  Tmp2, &p1);

	for(i = 0; i < p->nOutputs; i++) {
		float y0 = Tmp1[i];
		float y1 = Tmp2[i];

		Output[i] = y0 + (y1 - y0) * rest;
	}
}

// The default factory
static cmsInterpFunction DefaultInterpolatorsFactory(uint32 nInputChannels, uint32 nOutputChannels, uint32 dwFlags)
{
	cmsInterpFunction Interpolation;
	boolint IsFloat     = (dwFlags & CMS_LERP_FLAGS_FLOAT);
	boolint IsTrilinear = (dwFlags & CMS_LERP_FLAGS_TRILINEAR);
	memzero(&Interpolation, sizeof(Interpolation));
	// Safety check
	if(nInputChannels >= 4 && nOutputChannels >= MAX_STAGE_CHANNELS)
		return Interpolation;
	switch(nInputChannels) {
		case 1: // Gray LUT / linear
		    if(nOutputChannels == 1) {
			    if(IsFloat)
				    Interpolation.LerpFloat = LinLerp1Dfloat;
			    else
				    Interpolation.Lerp16 = LinLerp1D;
		    }
		    else {
			    if(IsFloat)
				    Interpolation.LerpFloat = Eval1InputFloat;
			    else
				    Interpolation.Lerp16 = Eval1Input;
		    }
		    break;

		case 2: // Duotone
		    if(IsFloat)
			    Interpolation.LerpFloat =  BilinearInterpFloat;
		    else
			    Interpolation.Lerp16    =  BilinearInterp16;
		    break;

		case 3: // RGB et al

		    if(IsTrilinear) {
			    if(IsFloat)
				    Interpolation.LerpFloat = TrilinearInterpFloat;
			    else
				    Interpolation.Lerp16 = TrilinearInterp16;
		    }
		    else {
			    if(IsFloat)
				    Interpolation.LerpFloat = TetrahedralInterpFloat;
			    else {
				    Interpolation.Lerp16 = TetrahedralInterp16;
			    }
		    }
		    break;

		case 4: // CMYK lut

		    if(IsFloat)
			    Interpolation.LerpFloat =  Eval4InputsFloat;
		    else
			    Interpolation.Lerp16    =  Eval4Inputs;
		    break;

		case 5: // 5 Inks
		    if(IsFloat)
			    Interpolation.LerpFloat =  Eval5InputsFloat;
		    else
			    Interpolation.Lerp16    =  Eval5Inputs;
		    break;

		case 6: // 6 Inks
		    if(IsFloat)
			    Interpolation.LerpFloat =  Eval6InputsFloat;
		    else
			    Interpolation.Lerp16    =  Eval6Inputs;
		    break;

		case 7: // 7 inks
		    if(IsFloat)
			    Interpolation.LerpFloat =  Eval7InputsFloat;
		    else
			    Interpolation.Lerp16    =  Eval7Inputs;
		    break;
		case 8: // 8 inks
		    if(IsFloat)
			    Interpolation.LerpFloat =  Eval8InputsFloat;
		    else
			    Interpolation.Lerp16    =  Eval8Inputs;
		    break;
		    break;

		default:
		    Interpolation.Lerp16 = NULL;
	}
	return Interpolation;
}
