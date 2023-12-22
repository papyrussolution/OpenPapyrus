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
#include "testcms2.h"
//
// Auxiliary, duplicate a context and mark the block as non-debug because in this case the allocator
// and deallocator have different context owners
//
static cmsContext DupContext(cmsContext src, void * Data)
{
	cmsContext cpy = cmsDupContext(src, Data);
	DebugMemDontCheckThis(cpy);
	return cpy;
}
//
// Simple context functions
//
// Allocation order
int32 CheckAllocContext()
{
	cmsContext c1, c2, c3, c4;
	c1 = cmsCreateContext(NULL, NULL);              // This creates a context by using the normal malloc
	DebugMemDontCheckThis(c1);
	cmsDeleteContext(c1);
	c2 = cmsCreateContext(PluginMemHandler(), NULL); // This creates a context by using the debug malloc
	DebugMemDontCheckThis(c2);
	cmsDeleteContext(c2);

	c1 = cmsCreateContext(NULL, NULL);
	DebugMemDontCheckThis(c1);

	c2 = cmsCreateContext(PluginMemHandler(), NULL);
	DebugMemDontCheckThis(c2);

	cmsPluginTHR(c1, PluginMemHandler()); // Now the context have custom allocators

	c3 = DupContext(c1, NULL);
	c4 = DupContext(c2, NULL);

	cmsDeleteContext(c1); // Should be deleted by using nomal malloc
	cmsDeleteContext(c2); // Should be deleted by using debug malloc
	cmsDeleteContext(c3); // Should be deleted by using nomal malloc
	cmsDeleteContext(c4); // Should be deleted by using debug malloc

	return 1;
}

// Test the very basic context capabilities
int32 CheckSimpleContext(FILE * fOut)
{
	int a = 1;
	int b = 32;
	int32 rc = 0;
	cmsContext c1, c2, c3;
	// This function creates a context with a special
	// memory manager that check allocation
	c1 = WatchDogContext(fOut, &a);
	cmsDeleteContext(c1);
	c1 = WatchDogContext(fOut, &a);
	// Let's check duplication
	c2 = DupContext(c1, NULL);
	c3 = DupContext(c2, NULL);
	// User data should have been propagated
	rc = (*(int*)cmsGetContextUserData(c3)) == 1;
	// Free resources
	cmsDeleteContext(c1);
	cmsDeleteContext(c2);
	cmsDeleteContext(c3);
	if(!rc) {
		Fail("Creation of user data failed");
		return 0;
	}
	// Back to create 3 levels of inherance
	c1 = cmsCreateContext(NULL, &a);
	DebugMemDontCheckThis(c1);
	c2 = DupContext(c1, NULL);
	c3 = DupContext(c2, &b);
	rc = (*(int*)cmsGetContextUserData(c3)) == 32;
	cmsDeleteContext(c1);
	cmsDeleteContext(c2);
	cmsDeleteContext(c3);
	if(!rc) {
		Fail("Modification of user data failed");
		return 0;
	}
	// All seems ok
	return rc;
}

// --------------------------------------------------------------------------------------------------
//Alarm color functions
// --------------------------------------------------------------------------------------------------

// This function tests the alarm codes across contexts
int32 CheckAlarmColorsContext(FILE * fOut)
{
	int32 rc = 0;
	const uint16 codes[] =
	{0x0000, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xaaaa, 0xbbbb, 0xcccc, 0xdddd, 0xeeee,
	 0xffff};
	uint16 out[16];
	cmsContext c1, c2, c3;
	int i;
	c1 = WatchDogContext(fOut, NULL);
	cmsSetAlarmCodesTHR(c1, codes);
	c2 = DupContext(c1, NULL);
	c3 = DupContext(c2, NULL);
	cmsGetAlarmCodesTHR(c3, out);
	rc = 1;
	for(i = 0; i < 16; i++) {
		if(out[i] != codes[i]) {
			Fail("Bad alarm code %x != %x", out[i], codes[i]);
			rc = 0;
			break;
		}
	}
	cmsDeleteContext(c1);
	cmsDeleteContext(c2);
	cmsDeleteContext(c3);
	return rc;
}

// --------------------------------------------------------------------------------------------------
//Adaptation state functions
// --------------------------------------------------------------------------------------------------

// Similar to the previous, but for adaptation state
int32 CheckAdaptationStateContext(FILE * fOut)
{
	int32 rc = 0;
	cmsContext c1, c2, c3;
	double old1, old2;
	old1 =  cmsSetAdaptationStateTHR(NULL, -1);
	c1 = WatchDogContext(fOut, NULL);
	cmsSetAdaptationStateTHR(c1, 0.7);
	c2 = DupContext(c1, NULL);
	c3 = DupContext(c2, NULL);
	rc = IsGoodVal("Adaptation state", cmsSetAdaptationStateTHR(c3, -1), 0.7, 0.001);
	cmsDeleteContext(c1);
	cmsDeleteContext(c2);
	cmsDeleteContext(c3);
	old2 =  cmsSetAdaptationStateTHR(NULL, -1);
	if(old1 != old2) {
		Fail("Adaptation state has changed");
		return 0;
	}
	return rc;
}

// --------------------------------------------------------------------------------------------------
// Interpolation plugin check: A fake 1D and 3D interpolation will be used to test the functionality.
// --------------------------------------------------------------------------------------------------

// This fake interpolation takes always the closest lower node in the interpolation table for 1D
static void Fake1Dfloat(const float Value[], float Output[], const cmsInterpParams* p)
{
	float val2;
	int cell;
	const float* LutTable = (const float*)p->Table;
	// Clip upper values
	if(Value[0] >= 1.0) {
		Output[0] = LutTable[p->Domain[0]];
		return;
	}
	val2 = p->Domain[0] * Value[0];
	cell = (int)floor(val2);
	Output[0] =  LutTable[cell];
}

// This fake interpolation just uses scrambled negated indexes for output
static void Fake3D16(const uint16 Input[], uint16 Output[], const struct _cms_interp_struc * /*p*/)
{
	Output[0] =  0xFFFF - Input[2];
	Output[1] =  0xFFFF - Input[1];
	Output[2] =  0xFFFF - Input[0];
}

// The factory chooses interpolation routines on depending on certain conditions.
cmsInterpFunction my_Interpolators_Factory(uint32 nInputChannels, uint32 nOutputChannels, uint32 dwFlags)
{
	cmsInterpFunction Interpolation;
	boolint IsFloat = (dwFlags & CMS_LERP_FLAGS_FLOAT);
	// Initialize the return to zero as a non-supported mark
	memzero(&Interpolation, sizeof(Interpolation));
	// For 1D to 1D and floating point
	if(nInputChannels == 1 && nOutputChannels == 1 && IsFloat) {
		Interpolation.LerpFloat = Fake1Dfloat;
	}
	else if(nInputChannels == 3 && nOutputChannels == 3 && !IsFloat) {
		// For 3D to 3D and 16 bits
		Interpolation.Lerp16 = Fake3D16;
	}
	// Here is the interpolation
	return Interpolation;
}

// Interpolation plug-in
static cmsPluginInterpolation InterpPluginSample = {
	{ cmsPluginMagicNumber, 2060, cmsPluginInterpolationSig, NULL },
	my_Interpolators_Factory
};

// This is the check code for 1D interpolation plug-in
int32 CheckInterp1DPlugin(FILE * fOut)
{
	cmsToneCurve * Sampled1D = NULL;
	cmsContext cpy = NULL;
	const float tab[] = { 0.0f, 0.10f, 0.20f, 0.30f, 0.40f, 0.50f, 0.60f, 0.70f, 0.80f, 0.90f, 1.00f }; // A straight line
	// 1st level context
	cmsContext ctx = WatchDogContext(fOut, NULL);
	if(!ctx) {
		Fail("Cannot create context");
		goto Error;
	}
	cmsPluginTHR(ctx, &InterpPluginSample);
	cpy = DupContext(ctx, NULL);
	if(cpy == NULL) {
		Fail("Cannot create context (2)");
		goto Error;
	}
	Sampled1D = cmsBuildTabulatedToneCurveFloat(cpy, 11, tab);
	if(Sampled1D == NULL) {
		Fail("Cannot create tone curve (1)");
		goto Error;
	}
	// Do some interpolations with the plugin
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(Sampled1D, 0.10f), 0.10, 0.01)) goto Error;
	if(!IsGoodVal("0.13", cmsEvalToneCurveFloat(Sampled1D, 0.13f), 0.10, 0.01)) goto Error;
	if(!IsGoodVal("0.55", cmsEvalToneCurveFloat(Sampled1D, 0.55f), 0.50, 0.01)) goto Error;
	if(!IsGoodVal("0.9999", cmsEvalToneCurveFloat(Sampled1D, 0.9999f), 0.90, 0.01)) goto Error;
	cmsFreeToneCurve(Sampled1D);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	// Now in global context
	Sampled1D = cmsBuildTabulatedToneCurveFloat(NULL, 11, tab);
	if(Sampled1D == NULL) {
		Fail("Cannot create tone curve (2)");
		goto Error;
	}
	// Now without the plug-in
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(Sampled1D, 0.10f), 0.10, 0.001)) goto Error;
	if(!IsGoodVal("0.13", cmsEvalToneCurveFloat(Sampled1D, 0.13f), 0.13, 0.001)) goto Error;
	if(!IsGoodVal("0.55", cmsEvalToneCurveFloat(Sampled1D, 0.55f), 0.55, 0.001)) goto Error;
	if(!IsGoodVal("0.9999", cmsEvalToneCurveFloat(Sampled1D, 0.9999f), 0.9999, 0.001)) goto Error;
	cmsFreeToneCurve(Sampled1D);
	return 1;
Error:
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy); // @fix ctx-->cpy
	cmsFreeToneCurve(Sampled1D);
	return 0;
}

// Checks the 3D interpolation
int32 CheckInterp3DPlugin(FILE * fOut)
{
	cmsPipeline * p;
	cmsStage * clut;
	cmsContext ctx;
	uint16 In[3], Out[3];
	uint16 identity[] = {
		0,       0,       0,
		0,       0,       0xffff,
		0,       0xffff,  0,
		0,       0xffff,  0xffff,
		0xffff,  0,       0,
		0xffff,  0,       0xffff,
		0xffff,  0xffff,  0,
		0xffff,  0xffff,  0xffff
	};
	ctx = WatchDogContext(fOut, NULL);
	if(!ctx) {
		Fail("Cannot create context");
		return 0;
	}
	cmsPluginTHR(ctx, &InterpPluginSample);
	p =  cmsPipelineAlloc(ctx, 3, 3);
	clut = cmsStageAllocCLut16bit(ctx, 2, 3, 3, identity);
	cmsPipelineInsertStage(p, cmsAT_BEGIN, clut);
	// Do some interpolations with the plugin
	In[0] = 0; In[1] = 0; In[2] = 0;
	cmsPipelineEval16(In, Out, p);
	if(!IsGoodWord("0", Out[0], 0xFFFF - 0)) goto Error;
	if(!IsGoodWord("1", Out[1], 0xFFFF - 0)) goto Error;
	if(!IsGoodWord("2", Out[2], 0xFFFF - 0)) goto Error;
	In[0] = 0x1234; In[1] = 0x5678; In[2] = 0x9ABC;
	cmsPipelineEval16(In, Out, p);

	if(!IsGoodWord("0", 0xFFFF - 0x9ABC, Out[0])) goto Error;
	if(!IsGoodWord("1", 0xFFFF - 0x5678, Out[1])) goto Error;
	if(!IsGoodWord("2", 0xFFFF - 0x1234, Out[2])) goto Error;

	cmsPipelineFree(p);
	cmsDeleteContext(ctx);

	// Now without the plug-in

	p =  cmsPipelineAlloc(NULL, 3, 3);
	clut = cmsStageAllocCLut16bit(NULL, 2, 3, 3, identity);
	cmsPipelineInsertStage(p, cmsAT_BEGIN, clut);

	In[0] = 0; In[1] = 0; In[2] = 0;
	cmsPipelineEval16(In, Out, p);

	if(!IsGoodWord("0", 0, Out[0])) goto Error;
	if(!IsGoodWord("1", 0, Out[1])) goto Error;
	if(!IsGoodWord("2", 0, Out[2])) goto Error;

	In[0] = 0x1234; In[1] = 0x5678; In[2] = 0x9ABC;
	cmsPipelineEval16(In, Out, p);

	if(!IsGoodWord("0", 0x1234, Out[0])) goto Error;
	if(!IsGoodWord("1", 0x5678, Out[1])) goto Error;
	if(!IsGoodWord("2", 0x9ABC, Out[2])) goto Error;

	cmsPipelineFree(p);
	return 1;

Error:
	cmsPipelineFree(p);
	return 0;
}
//
// Parametric curve plugin check: sin(x)/cos(x) function will be used to test the functionality.
//
#define TYPE_SIN  1000
#define TYPE_COS  1010
#define TYPE_TAN  1020
#define TYPE_709  709

static double my_fns(int Type, const double Params[], double R)
{
	double Val;
	switch(Type) {
		case TYPE_SIN:
		    Val = Params[0]* sin(R * SMathConst::Pi);
		    break;
		case -TYPE_SIN:
		    Val = asin(R) / (SMathConst::Pi * Params[0]);
		    break;
		case TYPE_COS:
		    Val = Params[0]* cos(R * SMathConst::Pi);
		    break;
		case -TYPE_COS:
		    Val = acos(R) / (SMathConst::Pi * Params[0]);
		    break;
		default: return -1.0;
	}
	return Val;
}

static double my_fns2(int Type, const double Params[], double R)
{
	double Val;
	switch(Type) {
		case TYPE_TAN:
		    Val = Params[0]* tan(R * SMathConst::Pi);
		    break;
		case -TYPE_TAN:
		    Val = atan(R) / (SMathConst::Pi * Params[0]);
		    break;
		default: return -1.0;
	}
	return Val;
}

static double Rec709Math(int Type, const double Params[], double R)
{
	double Fun = 0;
	switch(Type) {
		case 709:
		    if(R <= (Params[3]*Params[4])) Fun = R / Params[3];
		    else Fun = pow(((R - Params[2])/Params[1]), Params[0]);
		    break;
		case -709:
		    if(R <= Params[4]) Fun = R * Params[3];
		    else Fun = Params[1] * pow(R, (1/Params[0])) + Params[2];
		    break;
	}
	return Fun;
}

// Add nonstandard TRC curves -> Rec709

cmsPluginParametricCurves Rec709Plugin = {
	{ cmsPluginMagicNumber, 2060, cmsPluginParametricCurveSig, NULL }, 1, {TYPE_709}, {5}, Rec709Math
};

static cmsPluginParametricCurves CurvePluginSample = {
	{ cmsPluginMagicNumber, 2060, cmsPluginParametricCurveSig, NULL }, 2, // nFunctions
	{ TYPE_SIN, TYPE_COS }, // Function Types
	{ 1, 1 },            // ParameterCount
	my_fns               // Evaluator
};

static cmsPluginParametricCurves CurvePluginSample2 = {
	{ cmsPluginMagicNumber, 2060, cmsPluginParametricCurveSig, NULL }, 1, // nFunctions
	{ TYPE_TAN},         // Function Types
	{ 1 },               // ParameterCount
	my_fns2              // Evaluator
};

// --------------------------------------------------------------------------------------------------
// In this test, the DupContext function will be checked as well
// --------------------------------------------------------------------------------------------------
int32 CheckParametricCurvePlugin(FILE * fOut)
{
	cmsContext ctx = NULL;
	cmsContext cpy = NULL;
	cmsContext cpy2 = NULL;
	cmsToneCurve * sinus;
	cmsToneCurve * cosinus;
	cmsToneCurve * tangent;
	cmsToneCurve * reverse_sinus;
	cmsToneCurve * reverse_cosinus;
	double scale = 1.0;
	ctx = WatchDogContext(fOut, NULL);
	cmsPluginTHR(ctx, &CurvePluginSample);
	cpy = DupContext(ctx, NULL);
	cmsPluginTHR(cpy, &CurvePluginSample2);
	cpy2 =  DupContext(cpy, NULL);
	cmsPluginTHR(cpy2, &Rec709Plugin);
	sinus = cmsBuildParametricToneCurve(cpy, TYPE_SIN, &scale);
	cosinus = cmsBuildParametricToneCurve(cpy, TYPE_COS, &scale);
	tangent = cmsBuildParametricToneCurve(cpy, TYPE_TAN, &scale);
	reverse_sinus = cmsReverseToneCurve(sinus);
	reverse_cosinus = cmsReverseToneCurve(cosinus);
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(sinus, 0.10f), sin(0.10 * SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.60", cmsEvalToneCurveFloat(sinus, 0.60f), sin(0.60* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.90", cmsEvalToneCurveFloat(sinus, 0.90f), sin(0.90* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(cosinus, 0.10f), cos(0.10* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.60", cmsEvalToneCurveFloat(cosinus, 0.60f), cos(0.60* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.90", cmsEvalToneCurveFloat(cosinus, 0.90f), cos(0.90* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(tangent, 0.10f), tan(0.10* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.60", cmsEvalToneCurveFloat(tangent, 0.60f), tan(0.60* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.90", cmsEvalToneCurveFloat(tangent, 0.90f), tan(0.90* SMathConst::Pi), 0.001)) goto Error;
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(reverse_sinus, 0.10f), asin(0.10)/SMathConst::Pi, 0.001)) goto Error;
	if(!IsGoodVal("0.60", cmsEvalToneCurveFloat(reverse_sinus, 0.60f), asin(0.60)/SMathConst::Pi, 0.001)) goto Error;
	if(!IsGoodVal("0.90", cmsEvalToneCurveFloat(reverse_sinus, 0.90f), asin(0.90)/SMathConst::Pi, 0.001)) goto Error;
	if(!IsGoodVal("0.10", cmsEvalToneCurveFloat(reverse_cosinus, 0.10f), acos(0.10)/SMathConst::Pi, 0.001)) goto Error;
	if(!IsGoodVal("0.60", cmsEvalToneCurveFloat(reverse_cosinus, 0.60f), acos(0.60)/SMathConst::Pi, 0.001)) goto Error;
	if(!IsGoodVal("0.90", cmsEvalToneCurveFloat(reverse_cosinus, 0.90f), acos(0.90)/SMathConst::Pi, 0.001)) goto Error;
	cmsFreeToneCurve(sinus);
	cmsFreeToneCurve(cosinus);
	cmsFreeToneCurve(tangent);
	cmsFreeToneCurve(reverse_sinus);
	cmsFreeToneCurve(reverse_cosinus);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	return 1;
Error:
	cmsFreeToneCurve(sinus);
	cmsFreeToneCurve(reverse_sinus);
	cmsFreeToneCurve(cosinus);
	cmsFreeToneCurve(reverse_cosinus);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	return 0;
}
//
// formatters plugin check: 5-6-5 RGB format
//
// We define this special type as 0 bytes not float, and set the upper bit
#define TYPE_RGB_565  (COLORSPACE_SH(PT_RGB)|CHANNELS_SH(3)|BYTES_SH(0) | (1 << 23))

uint8 * my_Unroll565(struct _cmstransform_struct * /*nfo*/, uint16 wIn[], uint8 * accum, uint32 /*_stride*/)
{
	uint16 pixel = *(uint16*)accum; // Take whole pixel
	double r = floor(((double)(pixel & 31) * 65535.0) / 31.0 + 0.5);
	double g = floor((((pixel >> 5) & 63) * 65535.0) / 63.0 + 0.5);
	double b = floor((((pixel >> 11) & 31) * 65535.0) / 31.0 + 0.5);
	wIn[2] = (uint16)r;
	wIn[1] = (uint16)g;
	wIn[0] = (uint16)b;
	return accum + 2;
}

uint8 * my_Pack565(_cmsTRANSFORM * /*info*/, uint16 wOut[], uint8 * output, uint32 /*_stride*/)
{
	const int r = (int)floor(( wOut[2] * 31) / 65535.0 + 0.5);
	const int g = (int)floor(( wOut[1] * 63) / 65535.0 + 0.5);
	const int b = (int)floor(( wOut[0] * 31) / 65535.0 + 0.5);
	uint16 pixel = (r & 31)  | (( g & 63) << 5) | ((b & 31) << 11);
	*(uint16*)output = pixel;
	return output + 2;
}

cmsFormatter my_FormatterFactory(uint32 Type, cmsFormatterDirection Dir, uint32 dwFlags)
{
	cmsFormatter Result = { NULL };
	if((Type == TYPE_RGB_565) && !(dwFlags & CMS_PACK_FLAGS_FLOAT) && (Dir == cmsFormatterInput)) {
		Result.Fmt16 = my_Unroll565;
	}
	return Result;
}

cmsFormatter my_FormatterFactory2(uint32 Type, cmsFormatterDirection Dir, uint32 dwFlags)
{
	cmsFormatter Result = { NULL };
	if((Type == TYPE_RGB_565) && !(dwFlags & CMS_PACK_FLAGS_FLOAT) && (Dir == cmsFormatterOutput)) {
		Result.Fmt16 = my_Pack565;
	}
	return Result;
}

static cmsPluginFormatters FormattersPluginSample = { {cmsPluginMagicNumber, 2060, cmsPluginFormattersSig, NULL}, my_FormatterFactory };
static cmsPluginFormatters FormattersPluginSample2 = { {cmsPluginMagicNumber, 2060, cmsPluginFormattersSig, NULL}, my_FormatterFactory2 };

int32 CheckFormattersPlugin(FILE * fOut)
{
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsContext cpy;
	cmsContext cpy2;
	cmsHTRANSFORM xform;
	uint16 stream[] = { 0xffffU, 0x1234U, 0x0000U, 0x33ddU };
	uint16 result[4];
	int i;
	cmsPluginTHR(ctx, &FormattersPluginSample);
	cpy = DupContext(ctx, NULL);
	cmsPluginTHR(cpy, &FormattersPluginSample2);
	cpy2 = DupContext(cpy, NULL);
	xform = cmsCreateTransformTHR(cpy2, NULL, TYPE_RGB_565, NULL, TYPE_RGB_565, INTENT_PERCEPTUAL, cmsFLAGS_NULLTRANSFORM);
	cmsDoTransform(xform, stream, result, 4);
	cmsDeleteTransform(xform);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	for(i = 0; i < 4; i++)
		if(stream[i] != result[i]) 
			return 0;
	return 1;
}
//
// TagTypePlugin plugin check
//
#define SigIntType      ((cmsTagTypeSignature)0x74747448)     //   'tttH'
#define SigInt          ((cmsTagSignature)0x74747448)         //   'tttH'

static void * Type_int_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 /*SizeOfTag*/)
{
	uint32 * Ptr = (uint32 *)_cmsMalloc(self->ContextID, sizeof(uint32));
	if(!Ptr || !_cmsReadUInt32Number(io, Ptr)) 
		return NULL;
	*nItems = 1;
	return Ptr;
}

static boolint Type_int_Write(struct _cms_typehandler_struct * /*pSelf*/, cmsIOHANDLER* io, void * Ptr, uint32 /*nItems*/)
{
	return _cmsWriteUInt32Number(io, *(uint32 *)Ptr);
}

static void * Type_int_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, n * sizeof(uint32));
}

void Type_int_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}

static cmsPluginTag HiddenTagPluginSample = {
	{ cmsPluginMagicNumber, 2060, cmsPluginTagSig, NULL},
	SigInt,  {  1, 1, { SigIntType }, NULL }
};

static cmsPluginTagType TagTypePluginSample = {
	{ cmsPluginMagicNumber, 2060, cmsPluginTagTypeSig,  (cmsPluginBase*)&HiddenTagPluginSample},
	{ SigIntType, Type_int_Read, Type_int_Write, Type_int_Dup, Type_int_Free, NULL }
};

int32 CheckTagTypePlugin(FILE * fOut)
{
	cmsContext cpy = NULL;
	cmsContext cpy2 = NULL;
	cmsHPROFILE h = NULL;
	uint32 myTag = 1234;
	uint32 rc = 0;
	char * data = NULL;
	uint32 * ptr = NULL;
	uint32 clen = 0;
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsPluginTHR(ctx, &TagTypePluginSample);
	cpy = DupContext(ctx, NULL);
	cpy2 = DupContext(cpy, NULL);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	h = cmsCreateProfilePlaceholder(cpy2);
	if(!h) {
		Fail("Create placeholder failed");
		goto Error;
	}
	if(!cmsWriteTag(h, SigInt, &myTag)) {
		Fail("Plug-in failed");
		goto Error;
	}
	rc = cmsSaveProfileToMem(h, NULL, &clen);
	if(!rc) {
		Fail("Fetch mem size failed");
		goto Error;
	}
	data = static_cast<char *>(SAlloc::M(clen));
	if(!data) {
		Fail("malloc failed ?!?");
		goto Error;
	}
	rc = cmsSaveProfileToMem(h, data, &clen);
	if(!rc) {
		Fail("Save to mem failed");
		goto Error;
	}
	cmsCloseProfile(h);
	cmsSetLogErrorHandler(NULL);
	h = cmsOpenProfileFromMem(data, clen);
	if(!h) {
		Fail("Open profile failed");
		goto Error;
	}
	ptr = (uint32 *)cmsReadTag(h, SigInt);
	if(ptr) {
		Fail("read tag/context switching failed");
		goto Error;
	}
	cmsCloseProfile(h);
	ResetFatalError();
	h = cmsOpenProfileFromMemTHR(cpy2, data, clen);
	if(!h) {
		Fail("Open profile from mem failed");
		goto Error;
	}
	// Get rid of data
	SAlloc::F(data);
	data = NULL;
	ptr = (uint32 *)cmsReadTag(h, SigInt);
	if(!ptr) {
		Fail("Read tag/conext switching failed (2)");
		return 0;
	}
	rc = (*ptr == 1234);
	cmsCloseProfile(h);
	cmsDeleteContext(cpy2);
	return rc;
Error:
	cmsCloseProfile(h);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	SAlloc::F(data);
	return 0;
}
//
// MPE plugin check:
//
#define SigNegateType ((cmsStageSignature)0x6E202020)

static void EvaluateNegate(const float In[], float Out[], const cmsStage * /*pMpe*/)
{
	Out[0] = 1.0f - In[0];
	Out[1] = 1.0f - In[1];
	Out[2] = 1.0f - In[2];
}

static cmsStage * StageAllocNegate(cmsContext ContextID)
{
	return _cmsStageAllocPlaceholder(ContextID, SigNegateType, 3, 3, EvaluateNegate, NULL, NULL, NULL);
}

static void * Type_negate_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 /*SizeOfTag*/)
{
	uint16 Chans;
	if(!_cmsReadUInt16Number(io, &Chans)) 
		return NULL;
	if(Chans != 3) 
		return NULL;
	*nItems = 1;
	return StageAllocNegate(self->ContextID);
}

static boolint Type_negate_Write(struct _cms_typehandler_struct * /*pSelf*/, cmsIOHANDLER* io, void * /*Ptr*/, uint32 /*nItems*/)
{
	if(!_cmsWriteUInt16Number(io, 3)) return FALSE;
	return TRUE;
}

static cmsPluginMultiProcessElement MPEPluginSample = {
	{cmsPluginMagicNumber, 2060, cmsPluginMultiProcessElementSig, NULL},
	{ (cmsTagTypeSignature)SigNegateType, Type_negate_Read, Type_negate_Write, NULL, NULL, NULL }
};

int32 CheckMPEPlugin(FILE * fOut)
{
	cmsContext cpy = NULL;
	cmsContext cpy2 = NULL;
	cmsHPROFILE h = NULL;
	//uint32 myTag = 1234;
	uint32 rc = 0;
	char * data = NULL;
	uint32 clen = 0;
	float In[3], Out[3];
	cmsPipeline * pipe;
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsPluginTHR(ctx, &MPEPluginSample);
	cpy =  DupContext(ctx, NULL);
	cpy2 = DupContext(cpy, NULL);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	h = cmsCreateProfilePlaceholder(cpy2);
	if(!h) {
		Fail("Create placeholder failed");
		goto Error;
	}
	pipe = cmsPipelineAlloc(cpy2, 3, 3);
	cmsPipelineInsertStage(pipe, cmsAT_BEGIN, StageAllocNegate(cpy2));
	In[0] = 0.3f; In[1] = 0.2f; In[2] = 0.9f;
	cmsPipelineEvalFloat(In, Out, pipe);
	rc = (IsGoodVal("0", Out[0], 1.0-In[0], 0.001) && IsGoodVal("1", Out[1], 1.0-In[1], 0.001) && IsGoodVal("2", Out[2], 1.0-In[2], 0.001));
	if(!rc) {
		Fail("Pipeline failed");
		goto Error;
	}
	if(!cmsWriteTag(h, cmsSigDToB3Tag, pipe)) {
		Fail("Plug-in failed");
		goto Error;
	}
	// This cleans the stage as well
	cmsPipelineFree(pipe);
	rc = cmsSaveProfileToMem(h, NULL, &clen);
	if(!rc) {
		Fail("Fetch mem size failed");
		goto Error;
	}
	data = static_cast<char *>(SAlloc::M(clen));
	if(!data) {
		Fail("malloc failed ?!?");
		goto Error;
	}
	rc = cmsSaveProfileToMem(h, data, &clen);
	if(!rc) {
		Fail("Save to mem failed");
		goto Error;
	}
	cmsCloseProfile(h);
	cmsSetLogErrorHandler(NULL);
	h = cmsOpenProfileFromMem(data, clen);
	if(!h) {
		Fail("Open profile failed");
		goto Error;
	}
	pipe = (cmsPipeline *)cmsReadTag(h, cmsSigDToB3Tag);
	if(pipe != NULL) {
		// Unsupported stage, should fail
		Fail("read tag/context switching failed");
		goto Error;
	}
	cmsCloseProfile(h);
	ResetFatalError();
	h = cmsOpenProfileFromMemTHR(cpy2, data, clen);
	if(!h) {
		Fail("Open profile from mem failed");
		goto Error;
	}
	// Get rid of data
	ZFREE(data);
	pipe = (cmsPipeline *)cmsReadTag(h, cmsSigDToB3Tag);
	if(pipe == NULL) {
		Fail("Read tag/conext switching failed (2)");
		return 0;
	}
	// Evaluate for negation
	In[0] = 0.3f; In[1] = 0.2f; In[2] = 0.9f;
	cmsPipelineEvalFloat(In, Out, pipe);
	rc = (IsGoodVal("0", Out[0], 1.0-In[0], 0.001) && IsGoodVal("1", Out[1], 1.0-In[1], 0.001) && IsGoodVal("2", Out[2], 1.0-In[2], 0.001));
	cmsCloseProfile(h);
	cmsDeleteContext(cpy2);
	return rc;
Error:
	cmsCloseProfile(h);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	SAlloc::F(data);
	return 0;
}
//
// Optimization plugin check:
//
static void FastEvaluateCurves(const uint16 In[], uint16 Out[], const void * /*pData*/)
{
	Out[0] = In[0];
}

static boolint MyOptimize(cmsPipeline ** Lut, uint32 /*Intent*/, uint32 * /*InputFormat*/, uint32 * /*OutputFormat*/, uint32 * dwFlags)
{
	cmsStage * mpe;
	_cmsStageToneCurvesData* Data;
	//  Only curves in this LUT? All are identities?
	for(mpe = cmsPipelineGetPtrToFirstStage(*Lut); mpe != NULL; mpe = cmsStageNext(mpe)) {
		if(cmsStageType(mpe) != cmsSigCurveSetElemType) 
			return FALSE;
		// Check for identity
		Data = (_cmsStageToneCurvesData*)cmsStageData(mpe);
		if(Data->nCurves != 1) return FALSE;
		if(cmsEstimateGamma(Data->TheCurves[0], 0.1) > 1.0) return FALSE;
	}
	*dwFlags |= cmsFLAGS_NOCACHE;
	_cmsPipelineSetOptimizationParameters(*Lut, FastEvaluateCurves, NULL, NULL, NULL);
	return TRUE;
}

cmsPluginOptimization OptimizationPluginSample = {
	{cmsPluginMagicNumber, 2060, cmsPluginOptimizationSig, NULL},
	MyOptimize
};

int32 CheckOptimizationPlugin(FILE * fOut)
{
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsContext cpy;
	cmsContext cpy2;
	cmsHTRANSFORM xform;
	uint8 In[] = { 10, 20, 30, 40 };
	uint8 Out[4];
	cmsToneCurve * Linear[1];
	cmsHPROFILE h;
	int i;
	cmsPluginTHR(ctx, &OptimizationPluginSample);
	cpy = DupContext(ctx, NULL);
	cpy2 = DupContext(cpy, NULL);
	Linear[0] = cmsBuildGamma(cpy2, 1.0);
	h = cmsCreateLinearizationDeviceLinkTHR(cpy2, cmsSigGrayData, Linear);
	cmsFreeToneCurve(Linear[0]);

	xform = cmsCreateTransformTHR(cpy2, h, TYPE_GRAY_8, h, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);
	cmsCloseProfile(h);

	cmsDoTransform(xform, In, Out, 4);

	cmsDeleteTransform(xform);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);

	for(i = 0; i < 4; i++)
		if(In[i] != Out[i]) return 0;

	return 1;
}

// --------------------------------------------------------------------------------------------------
// Check the intent plug-in
// --------------------------------------------------------------------------------------------------

/*
   This example creates a new rendering intent, at intent number 300, that is identical to perceptual
   intent for all color spaces but gray to gray transforms, in this case it bypasses the data.
   Note that it has to clear all occurrences of intent 300 in the intents array to avoid
   infinite recursion.
 */

#define INTENT_DECEPTIVE   300

static cmsPipeline *  MyNewIntent(cmsContext ContextID, uint32 nProfiles, uint32 TheIntents[],
    cmsHPROFILE hProfiles[], boolint BPC[], double AdaptationStates[], uint32 dwFlags)
{
	cmsPipeline *    Result;
	uint32 ICCIntents[256];
	for(uint32 i = 0; i < nProfiles; i++)
		ICCIntents[i] = (TheIntents[i] == INTENT_DECEPTIVE) ? INTENT_PERCEPTUAL : TheIntents[i];
	if(cmsGetColorSpace(hProfiles[0]) != cmsSigGrayData ||
	    cmsGetColorSpace(hProfiles[nProfiles-1]) != cmsSigGrayData)
		return _cmsDefaultICCintents(ContextID, nProfiles, ICCIntents, hProfiles, BPC, AdaptationStates, dwFlags);
	Result = cmsPipelineAlloc(ContextID, 1, 1);
	if(Result == NULL) 
		return NULL;
	cmsPipelineInsertStage(Result, cmsAT_BEGIN, cmsStageAllocIdentity(ContextID, 1));
	return Result;
}

static cmsPluginRenderingIntent IntentPluginSample = {
	{cmsPluginMagicNumber, 2060, cmsPluginRenderingIntentSig, NULL},

	INTENT_DECEPTIVE, MyNewIntent,  "bypass gray to gray rendering intent"
};

int32 CheckIntentPlugin(FILE * fOut)
{
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsContext cpy;
	cmsContext cpy2;
	cmsHTRANSFORM xform;
	cmsHPROFILE h1, h2;
	cmsToneCurve * Linear1;
	cmsToneCurve * Linear2;
	uint8 In[] = { 10, 20, 30, 40 };
	uint8 Out[4];
	int i;
	cmsPluginTHR(ctx, &IntentPluginSample);
	cpy  = DupContext(ctx, NULL);
	cpy2 = DupContext(cpy, NULL);
	Linear1 = cmsBuildGamma(cpy2, 3.0);
	Linear2 = cmsBuildGamma(cpy2, 0.1);
	h1 = cmsCreateLinearizationDeviceLinkTHR(cpy2, cmsSigGrayData, &Linear1);
	h2 = cmsCreateLinearizationDeviceLinkTHR(cpy2, cmsSigGrayData, &Linear2);

	cmsFreeToneCurve(Linear1);
	cmsFreeToneCurve(Linear2);
	xform = cmsCreateTransformTHR(cpy2, h1, TYPE_GRAY_8, h2, TYPE_GRAY_8, INTENT_DECEPTIVE, 0);
	cmsCloseProfile(h1); 
	cmsCloseProfile(h2);
	cmsDoTransform(xform, In, Out, 4);
	cmsDeleteTransform(xform);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	for(i = 0; i < 4; i++)
		if(Out[i] != In[i]) 
			return 0;
	return 1;
}
//
// Check the full transform plug-in
//
// This is a sample intent that only works for gray8 as output, and always returns '42'
static void TrancendentalTransform(struct _cmstransform_struct * /*CMM*/, const void * /*pInputBuffer*/, void * OutputBuffer, uint32 Size, uint32 /*_stride*/)
{
	for(uint32 i = 0; i < Size; i++) {
		((uint8 *)OutputBuffer)[i] = 0x42;
	}
}

boolint TransformFactory(_cmsTransformFn* xformPtr, void ** /*ppUserData*/, _cmsFreeUserDataFn * /*pFreePrivateDataFn*/,
    cmsPipeline ** /*ppLut*/, uint32 * /*pInputFormat*/, uint32 * OutputFormat, uint32 * /*dwFlags*/)

{
	if(*OutputFormat == TYPE_GRAY_8) {
		// *Lut holds the pipeline to be applied
		*xformPtr = TrancendentalTransform;
		return TRUE;
	}
	return FALSE;
}

// The Plug-in entry point
static cmsPluginTransform FullTransformPluginSample = {
	{ cmsPluginMagicNumber, 2060, cmsPluginTransformSig, NULL},

	TransformFactory
};

int32 CheckTransformPlugin(FILE * fOut)
{
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsContext cpy;
	cmsContext cpy2;
	cmsHTRANSFORM xform;
	uint8 In[] = { 10, 20, 30, 40 };
	uint8 Out[4];
	cmsToneCurve * Linear;
	cmsHPROFILE h;
	int i;
	cmsPluginTHR(ctx, &FullTransformPluginSample);
	cpy  = DupContext(ctx, NULL);
	cpy2 = DupContext(cpy, NULL);
	Linear = cmsBuildGamma(cpy2, 1.0);
	h = cmsCreateLinearizationDeviceLinkTHR(cpy2, cmsSigGrayData, &Linear);
	cmsFreeToneCurve(Linear);
	xform = cmsCreateTransformTHR(cpy2, h, TYPE_GRAY_8, h, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);
	cmsCloseProfile(h);
	cmsDoTransform(xform, In, Out, 4);
	cmsDeleteTransform(xform);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	for(i = 0; i < 4; i++)
		if(Out[i] != 0x42)
			return 0;
	return 1;
}

// --------------------------------------------------------------------------------------------------
// Check the mutex plug-in
// --------------------------------------------------------------------------------------------------

typedef struct {
	int nlocks;
} MyMtx;

static void * MyMtxCreate(cmsContext id)
{
	MyMtx* mtx = (MyMtx*)_cmsMalloc(id, sizeof(MyMtx));
	mtx->nlocks = 0;
	return mtx;
}

static void MyMtxDestroy(cmsContext id, void * mtx)
{
	MyMtx* mtx_ = (MyMtx*)mtx;
	if(mtx_->nlocks != 0)
		Die(stderr, "Locks != 0 when setting free a mutex");
	_cmsFree(id, mtx);
}

static boolint MyMtxLock(cmsContext /*id*/, void * mtx)
{
	MyMtx* mtx_ = (MyMtx*)mtx;
	mtx_->nlocks++;
	return TRUE;
}

static void MyMtxUnlock(cmsContext /*id*/, void * mtx)
{
	MyMtx * mtx_ = (MyMtx*)mtx;
	mtx_->nlocks--;
}

static cmsPluginMutex MutexPluginSample = { { cmsPluginMagicNumber, 2060, cmsPluginMutexSig, NULL}, MyMtxCreate,  MyMtxDestroy,  MyMtxLock,  MyMtxUnlock };

int32 CheckMutexPlugin(FILE * fOut)
{
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsContext cpy;
	cmsContext cpy2;
	cmsHTRANSFORM xform;
	uint8 In[] = { 10, 20, 30, 40 };
	uint8 Out[4];
	cmsToneCurve * Linear;
	cmsHPROFILE h;
	int i;
	cmsPluginTHR(ctx, &MutexPluginSample);
	cpy  = DupContext(ctx, NULL);
	cpy2 = DupContext(cpy, NULL);
	Linear = cmsBuildGamma(cpy2, 1.0);
	h = cmsCreateLinearizationDeviceLinkTHR(cpy2, cmsSigGrayData, &Linear);
	cmsFreeToneCurve(Linear);
	xform = cmsCreateTransformTHR(cpy2, h, TYPE_GRAY_8, h, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);
	cmsCloseProfile(h);
	cmsDoTransform(xform, In, Out, 4);
	cmsDeleteTransform(xform);
	cmsDeleteContext(ctx);
	cmsDeleteContext(cpy);
	cmsDeleteContext(cpy2);
	for(i = 0; i < 4; i++)
		if(Out[i] != In[i]) return 0;
	return 1;
}

int32 CheckMethodPackDoublesFromFloat(FILE * fOut)
{
	cmsContext ctx = WatchDogContext(fOut, NULL);
	cmsHTRANSFORM xform;
	cmsHTRANSFORM l_pFakeProfileLAB;
	double l_D_OutputColorArrayBlack[8];
	double l_D_OutputColorArrayBlue[8];
	cmsCIELab LabInBlack;
	cmsCIELab LabInBlue;
	uint16 Lab_UI16_Black[3];
	uint16 Lab_UI16_Blue[3];
	cmsHPROFILE OutputCMYKProfile;
	uint32 l_UI32_OutputFormat;
	cmsPluginTHR(ctx, &FullTransformPluginSample);
	l_pFakeProfileLAB = cmsCreateLab2ProfileTHR(ctx, NULL);
	if(l_pFakeProfileLAB == NULL)
		return 0;
	OutputCMYKProfile = cmsOpenProfileFromFileTHR(ctx, "TestCLT.icc", "r");
	if(OutputCMYKProfile == NULL)
		return 0;
	l_UI32_OutputFormat = 0;
	l_UI32_OutputFormat |= COLORSPACE_SH(PT_CMYK);
	l_UI32_OutputFormat |= PLANAR_SH(1);
	l_UI32_OutputFormat |= CHANNELS_SH(4);
	l_UI32_OutputFormat |= BYTES_SH(0);
	l_UI32_OutputFormat |= FLOAT_SH(1);
	xform = cmsCreateTransformTHR(ctx, l_pFakeProfileLAB, TYPE_Lab_DBL, OutputCMYKProfile, l_UI32_OutputFormat, INTENT_PERCEPTUAL, 0);
	cmsCloseProfile(OutputCMYKProfile);
	cmsCloseProfile(l_pFakeProfileLAB);
	Lab_UI16_Black[0] = 0;
	Lab_UI16_Black[1] = 32768;
	Lab_UI16_Black[2] = 32768;
	Lab_UI16_Blue[0] = 0;
	Lab_UI16_Blue[1] = 8192;
	Lab_UI16_Blue[2] = 8192;
	cmsLabEncoded2Float(&LabInBlack, Lab_UI16_Black);
	cmsLabEncoded2Float(&LabInBlue, Lab_UI16_Blue);
	memzero(l_D_OutputColorArrayBlack, sizeof(l_D_OutputColorArrayBlack));
	memzero(l_D_OutputColorArrayBlue, sizeof(l_D_OutputColorArrayBlue));
	cmsDoTransform(xform, &LabInBlack, l_D_OutputColorArrayBlack, 1);
	cmsDoTransform(xform, &LabInBlue, l_D_OutputColorArrayBlue, 1);
	cmsDeleteTransform(xform);
	cmsDeleteContext(ctx);
	return 1;
}
