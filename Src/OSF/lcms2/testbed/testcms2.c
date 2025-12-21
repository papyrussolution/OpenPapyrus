// TESTCMS2.C
// Little Color Management System
// Copyright (c) 1998-2020 Marti Maria Saguer
// @codepage UTF-8
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

#if SLTEST_RUNNING // {

#include "testcms2.h"

typedef int32 (* TestFn)(); // A single check. Returns 1 if success, 0 if failed
typedef int32 (* TestWithOutFn)(FILE * fOut); // A single check. Returns 1 if success, 0 if failed
typedef float (* dblfnptr)(float x, const double Params[]); // A parametric Tone curve test function

#define TEXT_ERROR_BUFFER_SIZE  4096 // Some globals to keep track of error

static char ReasonToFailBuffer[TEXT_ERROR_BUFFER_SIZE];
static char SubTestBuffer[TEXT_ERROR_BUFFER_SIZE];
static int32 TotalTests = 0, TotalFail = 0;
static boolint TrappedError;
static int32 SimultaneousErrors;
static SString TestbedPath; // @sobolev

//#define cmsmin(a, b) (((a) < (b)) ? (a) : (b))

// Die, a fatal unexpected error is detected!
void Die(FILE * fOut, const char * Reason, ...)
{
	va_list args;
	va_start(args, Reason);
	vsprintf(ReasonToFailBuffer, Reason, args);
	va_end(args);
	fprintf(fOut, "\n%s\n", ReasonToFailBuffer);
	fflush(fOut);
	exit(1);
}

// Memory management replacement -----------------------------------------------------------------------------

// This is just a simple plug-in for malloc, free and realloc to keep track of memory allocated,
// maximum requested as a single block and maximum allocated at a given time. Results are printed at the end
static uint32 SingleHit, MaxAllocated = 0, TotalMemory = 0;

// I'm hiding the size before the block. This is a well-known technique and probably the blocks coming from
// malloc are built in a way similar to that, but I do on my own to be portable.
typedef struct {
	uint32 KeepSize;
	cmsContext WhoAllocated;
	uint32 DontCheck;
	union {
		uint64 HiSparc;

		// '_cmsMemoryBlock' block is prepended by the
		// allocator for any requested size. Thus, union holds
		// "widest" type to guarantee proper '_cmsMemoryBlock'
		// alignment for any requested size.
	} alignment;
} _cmsMemoryBlock;

#define SIZE_OF_MEM_HEADER (sizeof(_cmsMemoryBlock))

// This is a fake thread descriptor used to check thread integrity.
// Basically it returns a different threadID each time it is called.
// Then the memory management replacement functions does check if each
// free() is being called with same ContextID used on malloc()
static cmsContext DbgThread()
{
	static uint32 n = 1;
	return (cmsContext)(void *)((uint8 *)NULL + (n++ % 0xff0));
}

// The allocate routine
static void * DebugMalloc(cmsContext ContextID, uint32 size)
{
	_cmsMemoryBlock* blk;
	if(size <= 0) {
		Die(stderr, "malloc requested with zero bytes");
	}
	TotalMemory += size;
	if(TotalMemory > MaxAllocated)
		MaxAllocated = TotalMemory;
	if(size > SingleHit)
		SingleHit = size;
	blk = (_cmsMemoryBlock*)SAlloc::M(size + SIZE_OF_MEM_HEADER);
	if(blk == NULL) 
		return NULL;
	blk->KeepSize = size;
	blk->WhoAllocated = ContextID;
	blk->DontCheck = 0;
	return (void *)((uint8 *)blk + SIZE_OF_MEM_HEADER);
}

// The free routine
static void DebugFree(cmsContext ContextID, void * Ptr)
{
	_cmsMemoryBlock* blk;
	if(Ptr == NULL) {
		Die(stderr, "NULL free (which is a no-op in C, but may be an clue of something going wrong)");
	}
	blk = (_cmsMemoryBlock*)(((uint8 *)Ptr) - SIZE_OF_MEM_HEADER);
	TotalMemory -= blk->KeepSize;
	if(blk->WhoAllocated != ContextID && !blk->DontCheck) {
		Die(stderr, "Trying to free memory allocated by a different thread");
	}
	SAlloc::F(blk);
}

// Reallocate, just a malloc, a copy and a free in this case.
static void * DebugRealloc(cmsContext ContextID, void * Ptr, uint32 NewSize)
{
	_cmsMemoryBlock* blk;
	uint32 max_sz;
	void *  NewPtr = DebugMalloc(ContextID, NewSize);
	if(Ptr == NULL) 
		return NewPtr;
	blk = (_cmsMemoryBlock*)(((uint8 *)Ptr) - SIZE_OF_MEM_HEADER);
	max_sz = blk->KeepSize > NewSize ? NewSize : blk->KeepSize;
	memmove(NewPtr, Ptr, max_sz);
	DebugFree(ContextID, Ptr);
	return NewPtr;
}

// Let's know the totals
static void DebugMemPrintTotals(FILE * fOut)
{
	fprintf(fOut, "[Memory statistics]\n");
	fprintf(fOut, "Allocated = %u MaxAlloc = %u Single block hit = %u\n", TotalMemory, MaxAllocated, SingleHit);
}

void DebugMemDontCheckThis(void * Ptr)
{
	_cmsMemoryBlock* blk = (_cmsMemoryBlock*)(((uint8 *)Ptr) - SIZE_OF_MEM_HEADER);
	blk->DontCheck = 1;
}

// Memory string
static const char * MemStr(uint32 size)
{
	static char Buffer[1024];
	if(size > 1024*1024) {
		sprintf(Buffer, "%g Mb", (double)size / (1024.0*1024.0));
	}
	else if(size > 1024) {
		sprintf(Buffer, "%g Kb", (double)size / 1024.0);
	}
	else
		sprintf(Buffer, "%g bytes", (double)size);
	return Buffer;
}

void TestMemoryLeaks(FILE * fOut, boolint ok)
{
	if(TotalMemory > 0)
		fprintf(fOut, "Ok, but %s are left!\n", MemStr(TotalMemory));
	else {
		if(ok) 
			fprintf(fOut, "Ok.\n");
	}
}

// Here we go with the plug-in declaration
static cmsPluginMemHandler DebugMemHandler = {
	{ cmsPluginMagicNumber, 2060, cmsPluginMemHandlerSig, NULL }, DebugMalloc, DebugFree, DebugRealloc, NULL, NULL, NULL };

// Returnds a pointer to the memhandler plugin
void * PluginMemHandler() { return (void *)&DebugMemHandler; }

cmsContext WatchDogContext(FILE * fOut, void * usr)
{
	cmsContext ctx = cmsCreateContext(&DebugMemHandler, usr);
	if(!ctx)
		Die(fOut, "Unable to create memory managed context");
	DebugMemDontCheckThis(ctx);
	return ctx;
}

static void FatalErrorQuit(FILE * fOut, cmsContext ContextID, uint32 ErrorCode, const char * Text)
{
	Die(fOut, Text);
	CXX_UNUSED(ContextID);
	CXX_UNUSED(ErrorCode);
}

void ResetFatalError()
{
	cmsSetLogErrorHandler(FatalErrorQuit);
}

// Print a dot for gauging
void Dot(FILE * fOut)
{
	fprintf(fOut, "."); 
	fflush(fOut);
}

void Say(FILE * fOut, const char * str)
{
	fprintf(fOut, "%s", str); 
	fflush(fOut);
}

// Keep track of the reason to fail

void Fail(const char * frm, ...)
{
	va_list args;
	va_start(args, frm);
	vsprintf(ReasonToFailBuffer, frm, args);
	va_end(args);
}

// Keep track of subtest

void SubTest(FILE * fOut, const char * frm, ...)
{
	va_list args;
	Dot(fOut);
	va_start(args, frm);
	vsprintf(SubTestBuffer, frm, args);
	va_end(args);
}

static void Check_Prelude(FILE * fOut, const char * Title)
{
	fprintf(fOut, "Checking %s ...", Title);
	fflush(fOut);
	ReasonToFailBuffer[0] = 0;
	SubTestBuffer[0] = 0;
	TrappedError = FALSE;
	SimultaneousErrors = 0;
	TotalTests++;
}

static void Check_OnFail(FILE * fOut, const char * Title)
{
	fprintf(fOut, "FAIL!\n");
	if(SubTestBuffer[0])
		fprintf(fOut, "%s: [%s]\n\t%s\n", Title, SubTestBuffer, ReasonToFailBuffer);
	else
		fprintf(fOut, "%s:\n\t%s\n", Title, ReasonToFailBuffer);
	if(SimultaneousErrors > 1)
		fprintf(fOut, "\tMore than one (%d) errors were reported\n", SimultaneousErrors);
	TotalFail++;
}

static void Check(FILE * fOut, const char * Title, TestWithOutFn Fn)
{
	Check_Prelude(fOut, Title);
	if(Fn(fOut) && !TrappedError) {
		// It is a good place to check memory
		TestMemoryLeaks(fOut, TRUE);
	}
	else {
		Check_OnFail(fOut, Title);
	}
	fflush(fOut);
}

// The check framework
static void Check(FILE * fOut, const char * Title, TestFn Fn)
{
	/*
	fprintf(fOut, "Checking %s ...", Title);
	fflush(fOut);
	ReasonToFailBuffer[0] = 0;
	SubTestBuffer[0] = 0;
	TrappedError = FALSE;
	SimultaneousErrors = 0;
	TotalTests++;
	*/
	Check_Prelude(fOut, Title);
	if(Fn() && !TrappedError) {
		// It is a good place to check memory
		TestMemoryLeaks(fOut, TRUE);
	}
	else {
		Check_OnFail(fOut, Title);
		/*
		fprintf(fOut, "FAIL!\n");
		if(SubTestBuffer[0])
			fprintf(fOut, "%s: [%s]\n\t%s\n", Title, SubTestBuffer, ReasonToFailBuffer);
		else
			fprintf(fOut, "%s:\n\t%s\n", Title, ReasonToFailBuffer);
		if(SimultaneousErrors > 1)
			fprintf(fOut, "\tMore than one (%d) errors were reported\n", SimultaneousErrors);
		TotalFail++;
		*/
	}
	fflush(fOut);
}

// Dump a tone curve, for easy diagnostic
void DumpToneCurve(cmsToneCurve * gamma, const char * FileName)
{
	cmsHANDLE hIT8 = cmsIT8Alloc(gamma->InterpParams->ContextID);
	cmsIT8SetPropertyDbl(hIT8, "NUMBER_OF_FIELDS", 2);
	cmsIT8SetPropertyDbl(hIT8, "NUMBER_OF_SETS", gamma->nEntries);
	cmsIT8SetDataFormat(hIT8, 0, "SAMPLE_ID");
	cmsIT8SetDataFormat(hIT8, 1, "VALUE");
	for(uint32 i = 0; i < gamma->nEntries; i++) {
		char Val[30];
		sprintf(Val, "%u", i);
		cmsIT8SetDataRowCol(hIT8, i, 0, Val);
		sprintf(Val, "0x%x", gamma->Table16[i]);
		cmsIT8SetDataRowCol(hIT8, i, 1, Val);
	}
	cmsIT8SaveToFile(hIT8, FileName);
	cmsIT8Free(hIT8);
}

// -------------------------------------------------------------------------------------------------

// Used to perform several checks.
// The space used is a clone of a well-known commercial
// color space which I will name "Above RGB"
static cmsHPROFILE Create_AboveRGB()
{
	cmsToneCurve * Curve[3];
	cmsHPROFILE hProfile;
	cmsCIExyY D65;
	cmsCIExyYTRIPLE Primaries = {{0.64, 0.33, 1 }, {0.21, 0.71, 1 }, {0.15, 0.06, 1 }};
	Curve[0] = Curve[1] = Curve[2] = cmsBuildGamma(DbgThread(), 2.19921875);
	cmsWhitePointFromTemp(&D65, 6504);
	hProfile = cmsCreateRGBProfileTHR(DbgThread(), &D65, &Primaries, Curve);
	cmsFreeToneCurve(Curve[0]);
	return hProfile;
}

// A gamma-2.2 gray space
static cmsHPROFILE Create_Gray22()
{
	cmsHPROFILE hProfile;
	cmsToneCurve * Curve = cmsBuildGamma(DbgThread(), 2.2);
	if(Curve == NULL) 
		return NULL;
	hProfile = cmsCreateGrayProfileTHR(DbgThread(), cmsD50_xyY(), Curve);
	cmsFreeToneCurve(Curve);
	return hProfile;
}

// A gamma-3.0 gray space
static cmsHPROFILE Create_Gray30()
{
	cmsHPROFILE hProfile;
	cmsToneCurve * Curve = cmsBuildGamma(DbgThread(), 3.0);
	if(Curve == NULL) 
		return NULL;
	hProfile = cmsCreateGrayProfileTHR(DbgThread(), cmsD50_xyY(), Curve);
	cmsFreeToneCurve(Curve);
	return hProfile;
}

static cmsHPROFILE Create_GrayLab()
{
	cmsHPROFILE hProfile;
	cmsToneCurve * Curve = cmsBuildGamma(DbgThread(), 1.0);
	if(Curve == NULL) 
		return NULL;
	hProfile = cmsCreateGrayProfileTHR(DbgThread(), cmsD50_xyY(), Curve);
	cmsFreeToneCurve(Curve);
	cmsSetPCS(hProfile, cmsSigLabData);
	return hProfile;
}

// A CMYK devicelink that adds gamma 3.0 to each channel
static cmsHPROFILE Create_CMYK_DeviceLink()
{
	cmsHPROFILE hProfile;
	cmsToneCurve * Tab[4];
	cmsToneCurve * Curve = cmsBuildGamma(DbgThread(), 3.0);
	if(Curve == NULL) 
		return NULL;
	Tab[0] = Curve;
	Tab[1] = Curve;
	Tab[2] = Curve;
	Tab[3] = Curve;
	hProfile = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigCmykData, Tab);
	if(hProfile == NULL) 
		return NULL;
	cmsFreeToneCurve(Curve);
	return hProfile;
}

// Create a fake CMYK profile, without any other requeriment that being coarse CMYK.
// DON'T USE THIS PROFILE FOR ANYTHING, IT IS USELESS BUT FOR TESTING PURPOSES.
typedef struct {
	cmsHTRANSFORM hLab2sRGB;
	cmsHTRANSFORM sRGB2Lab;
	cmsHTRANSFORM hIlimit;
} FakeCMYKParams;

static double Clip(double v)
{
	if(v < 0) return 0;
	if(v > 1) return 1;
	return v;
}

static boolint ForwardSampler(const uint16 In[], uint16 Out[], void * Cargo)
{
	FakeCMYKParams* p = (FakeCMYKParams*)Cargo;
	double rgb[3], cmyk[4];
	double c, m, y, k;
	cmsDoTransform(p->hLab2sRGB, In, rgb, 1);
	c = 1 - rgb[0];
	m = 1 - rgb[1];
	y = 1 - rgb[2];
	k = (c < m ? cmsmin(c, y) : cmsmin(m, y));
	// NONSENSE WARNING!: I'm doing this just because this is a test
	// profile that may have ink limit up to 400%. There is no UCR here
	// so the profile is basically useless for anything but testing.
	cmyk[0] = c;
	cmyk[1] = m;
	cmyk[2] = y;
	cmyk[3] = k;
	cmsDoTransform(p->hIlimit, cmyk, Out, 1);
	return 1;
}

static boolint ReverseSampler(const uint16 In[], uint16 Out[], void * Cargo)
{
	FakeCMYKParams* p = (FakeCMYKParams*)Cargo;
	double c, m, y, k, rgb[3];
	c = In[0] / 65535.0;
	m = In[1] / 65535.0;
	y = In[2] / 65535.0;
	k = In[3] / 65535.0;
	if(k == 0) {
		rgb[0] = Clip(1 - c);
		rgb[1] = Clip(1 - m);
		rgb[2] = Clip(1 - y);
	}
	else if(k == 1) {
		rgb[0] = rgb[1] = rgb[2] = 0;
	}
	else {
		rgb[0] = Clip((1 - c) * (1 - k));
		rgb[1] = Clip((1 - m) * (1 - k));
		rgb[2] = Clip((1 - y) * (1 - k));
	}

	cmsDoTransform(p->sRGB2Lab, rgb, Out, 1);
	return 1;
}

static cmsHPROFILE CreateFakeCMYK(double InkLimit, boolint lUseAboveRGB)
{
	cmsHPROFILE hICC;
	cmsPipeline * AToB0, * BToA0;
	cmsStage * CLUT;
	cmsContext ContextID;
	FakeCMYKParams p;
	cmsHPROFILE hLab, hsRGB, hLimit;
	uint32 cmykfrm;
	if(lUseAboveRGB)
		hsRGB = Create_AboveRGB();
	else
		hsRGB  = cmsCreate_sRGBProfile();
	hLab   = cmsCreateLab4Profile(NULL);
	hLimit = cmsCreateInkLimitingDeviceLink(cmsSigCmykData, InkLimit);
	cmykfrm = FLOAT_SH(1) | BYTES_SH(0)|CHANNELS_SH(4);
	p.hLab2sRGB = cmsCreateTransform(hLab,  TYPE_Lab_16,  hsRGB, TYPE_RGB_DBL, INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
	p.sRGB2Lab  = cmsCreateTransform(hsRGB, TYPE_RGB_DBL, hLab,  TYPE_Lab_16,  INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
	p.hIlimit   = cmsCreateTransform(hLimit, cmykfrm, NULL, TYPE_CMYK_16, INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
	cmsCloseProfile(hLab); 
	cmsCloseProfile(hsRGB); 
	cmsCloseProfile(hLimit);
	ContextID = DbgThread();
	hICC = cmsCreateProfilePlaceholder(ContextID);
	if(!hICC) 
		return NULL;
	cmsSetProfileVersion(hICC, 4.3);
	cmsSetDeviceClass(hICC, cmsSigOutputClass);
	cmsSetColorSpace(hICC,  cmsSigCmykData);
	cmsSetPCS(hICC,         cmsSigLabData);
	BToA0 = cmsPipelineAlloc(ContextID, 3, 4);
	if(BToA0 == NULL) 
		return 0;
	CLUT = cmsStageAllocCLut16bit(ContextID, 17, 3, 4, NULL);
	if(CLUT == NULL) 
		return 0;
	if(!cmsStageSampleCLut16bit(CLUT, ForwardSampler, &p, 0)) 
		return 0;
	cmsPipelineInsertStage(BToA0, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, 3));
	cmsPipelineInsertStage(BToA0, cmsAT_END, CLUT);
	cmsPipelineInsertStage(BToA0, cmsAT_END, _cmsStageAllocIdentityCurves(ContextID, 4));
	if(!cmsWriteTag(hICC, cmsSigBToA0Tag, (void *)BToA0)) 
		return 0;
	cmsPipelineFree(BToA0);
	AToB0 = cmsPipelineAlloc(ContextID, 4, 3);
	if(AToB0 == NULL) return 0;
	CLUT = cmsStageAllocCLut16bit(ContextID, 17, 4, 3, NULL);
	if(CLUT == NULL) return 0;
	if(!cmsStageSampleCLut16bit(CLUT, ReverseSampler, &p, 0)) return 0;

	cmsPipelineInsertStage(AToB0, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(ContextID, 4));
	cmsPipelineInsertStage(AToB0, cmsAT_END, CLUT);
	cmsPipelineInsertStage(AToB0, cmsAT_END, _cmsStageAllocIdentityCurves(ContextID, 3));

	if(!cmsWriteTag(hICC, cmsSigAToB0Tag, (void *)AToB0)) return 0;
	cmsPipelineFree(AToB0);

	cmsDeleteTransform(p.hLab2sRGB);
	cmsDeleteTransform(p.sRGB2Lab);
	cmsDeleteTransform(p.hIlimit);

	cmsLinkTag(hICC, cmsSigAToB1Tag, cmsSigAToB0Tag);
	cmsLinkTag(hICC, cmsSigAToB2Tag, cmsSigAToB0Tag);
	cmsLinkTag(hICC, cmsSigBToA1Tag, cmsSigBToA0Tag);
	cmsLinkTag(hICC, cmsSigBToA2Tag, cmsSigBToA0Tag);

	return hICC;
}

// Does create several profiles for latter
// use------------------------------------------------------------------------------------------------

static int32 OneVirtual(FILE * fOut, cmsHPROFILE h, const char * SubTestTxt, const char * FileName)
{
	SubTest(fOut, SubTestTxt);
	if(!h) 
		return 0;
	if(!cmsSaveProfileToFile(h, FileName)) 
		return 0;
	cmsCloseProfile(h);
	h = cmsOpenProfileFromFile(FileName, "r");
	if(!h) return 0;
	cmsCloseProfile(h);
	return 1;
}

// This test checks the ability of lcms2 to save its built-ins as valid profiles.
// It does not check the functionality of such profiles
static int32 CreateTestProfiles(FILE * fOut)
{
	cmsHPROFILE h = cmsCreate_sRGBProfileTHR(DbgThread());
	if(!OneVirtual(fOut, h, "sRGB profile", "sRGBlcms2.icc")) 
		return 0;
	// ----
	h = Create_AboveRGB();
	if(!OneVirtual(fOut, h, "aRGB profile", "aRGBlcms2.icc")) 
		return 0;
	// ----
	h = Create_Gray22();
	if(!OneVirtual(fOut, h, "Gray profile", "graylcms2.icc")) 
		return 0;
	// ----
	h = Create_Gray30();
	if(!OneVirtual(fOut, h, "Gray 3.0 profile", "gray3lcms2.icc")) 
		return 0;
	// ----
	h = Create_GrayLab();
	if(!OneVirtual(fOut, h, "Gray Lab profile", "glablcms2.icc")) 
		return 0;
	// ----
	h = Create_CMYK_DeviceLink();
	if(!OneVirtual(fOut, h, "Linearization profile", "linlcms2.icc")) 
		return 0;
	// -------
	h = cmsCreateInkLimitingDeviceLinkTHR(DbgThread(), cmsSigCmykData, 150);
	if(!h) 
		return 0;
	if(!OneVirtual(fOut, h, "Ink-limiting profile", "limitlcms2.icc")) 
		return 0;
	// ------
	h = cmsCreateLab2ProfileTHR(DbgThread(), NULL);
	if(!OneVirtual(fOut, h, "Lab 2 identity profile", "labv2lcms2.icc")) 
		return 0;
	// ----
	h = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	if(!OneVirtual(fOut, h, "Lab 4 identity profile", "labv4lcms2.icc")) 
		return 0;
	// ----
	h = cmsCreateXYZProfileTHR(DbgThread());
	if(!OneVirtual(fOut, h, "XYZ identity profile", "xyzlcms2.icc")) 
		return 0;
	// ----
	h = cmsCreateNULLProfileTHR(DbgThread());
	if(!OneVirtual(fOut, h, "NULL profile", "nullcms2.icc")) 
		return 0;
	// ---
	h = cmsCreateBCHSWabstractProfileTHR(DbgThread(), 17, 0, 0, 0, 0, 5000, 6000);
	if(!OneVirtual(fOut, h, "BCHS profile", "bchslcms2.icc")) 
		return 0;
	// ---
	h = CreateFakeCMYK(300, FALSE);
	if(!OneVirtual(fOut, h, "Fake CMYK profile", "lcms2cmyk.icc")) 
		return 0;
	// ---
	h = cmsCreateBCHSWabstractProfileTHR(DbgThread(), 17, 0, 1.2, 0, 3, 5000, 5000);
	if(!OneVirtual(fOut, h, "Brightness", "brightness.icc")) 
		return 0;
	return 1;
}

static void RemoveTestProfiles()
{
	remove("sRGBlcms2.icc");
	remove("aRGBlcms2.icc");
	remove("graylcms2.icc");
	remove("gray3lcms2.icc");
	remove("linlcms2.icc");
	remove("limitlcms2.icc");
	remove("labv2lcms2.icc");
	remove("labv4lcms2.icc");
	remove("xyzlcms2.icc");
	remove("nullcms2.icc");
	remove("bchslcms2.icc");
	remove("lcms2cmyk.icc");
	remove("glablcms2.icc");
	remove("lcms2link.icc");
	remove("lcms2link2.icc");
	remove("brightness.icc");
}
//
// Check the size of basic types. If this test fails, nothing is going to work anyway
static int32 CheckBaseTypes()
{
	// Ignore warnings about conditional expression
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#endif
	if(sizeof(uint8) != 1) return 0;
	if(sizeof(int8) != 1) return 0;
	if(sizeof(uint16) != 2) return 0;
	if(sizeof(int16) != 2) return 0;
	if(sizeof(uint32) != 4) return 0;
	if(sizeof(int32) != 4) return 0;
	if(sizeof(uint64) != 8) return 0;
	if(sizeof(int64) != 8) return 0;
	if(sizeof(float) != 4) return 0;
	if(sizeof(double) != 8) return 0;
	if(sizeof(cmsSignature) != 4) return 0;
	if(sizeof(cmsU8Fixed8Number) != 2) return 0;
	if(sizeof(cmsS15Fixed16Number) != 4) return 0;
	if(sizeof(cmsU16Fixed16Number) != 4) return 0;
	return 1;
}
//
// Are we little or big endian?  From Harbison&Steele.
static int32 CheckEndianness()
{
	int32 BigEndian, IsOk;
	union {
		long l;
		char c[sizeof(long)];
	} u;
	u.l = 1;
	BigEndian = (u.c[sizeof(long) - 1] == 1);
#ifdef SL_BIGENDIAN
	IsOk = BigEndian;
#else
	IsOk = !BigEndian;
#endif
	assert(IsOk); // @sobolev
	if(!IsOk) {
		Die(stderr, "\nOOOPPSS! You have CMS_USE_BIG_ENDIAN toggle misconfigured!\n\nPlease, edit lcms2.h and %s the CMS_USE_BIG_ENDIAN toggle.\n", BigEndian ? "uncomment" : "comment");
		return 0;
	}
	return 1;
}

// Check quick floor
static int32 CheckQuickFloor()
{
	if((_cmsQuickFloor(1.234) != 1) || (_cmsQuickFloor(32767.234) != 32767) || (_cmsQuickFloor(-1.234) != -2) || (_cmsQuickFloor(-32767.1) != -32768)) {
		assert(0);
		Die(stderr, "\nOOOPPSS! _cmsQuickFloor() does not work as expected in your machine!\n\nPlease, edit lcms2.h and uncomment the CMS_DONT_USE_FAST_FLOOR toggle.\n");
		return 0;
	}
	return 1;
}

// Quick floor restricted to word
static int32 CheckQuickFloorWord()
{
	for(uint32 i = 0; i < 65535; i++) {
		if(_cmsQuickFloorWord((double)i + 0.1234) != i) {
			assert(0);
			Die(stderr, "\nOOOPPSS! _cmsQuickFloorWord() does not work as expected in your machine!\n\nPlease, edit lcms2.h and uncomment the CMS_DONT_USE_FAST_FLOOR toggle.\n");
			return 0;
		}
	}
	return 1;
}

// Precision stuff.
#define FIXED_PRECISION_15_16 (1.0 / 65535.0) // On 15.16 fixed point, this is the maximum we can obtain. Remember ICC profiles have storage limits on this number
#define FIXED_PRECISION_8_8 (1.0 / 255.0) // On 8.8 fixed point, that is the max we can obtain.
#define FLOAT_PRECISSION      (0.00001) // On float type, this is the precision we expect

static double MaxErr;
static double AllowedErr = FIXED_PRECISION_15_16;

boolint STDCALL IsGoodVal(const char * title, double in, double out, double max)
{
	double Err = fabs(in - out);
	if(Err > MaxErr) 
		MaxErr = Err;
	if((Err > max )) {
		Fail("(%s): Must be %f, But is %f ", title, in, out);
		return FALSE;
	}
	return TRUE;
}

boolint STDCALL IsGoodFixed15_16(const char * title, double in, double out)
{
	return IsGoodVal(title, in, out, FIXED_PRECISION_15_16);
}

boolint IsGoodFixed8_8(const char * title, double in, double out)
{
	return IsGoodVal(title, in, out, FIXED_PRECISION_8_8);
}

boolint STDCALL IsGoodWord(const char * title, uint16 in, uint16 out)
{
	if((abs(in - out) > 0)) {
		Fail("(%s): Must be %x, But is %x ", title, in, out);
		return FALSE;
	}
	return TRUE;
}

boolint STDCALL IsGoodWordPrec(const char * title, uint16 in, uint16 out, uint16 maxErr)
{
	if((abs(in - out) > maxErr )) {
		Fail("(%s): Must be %x, But is %x ", title, in, out);
		return FALSE;
	}
	return TRUE;
}
//
// Fixed point
//
static int32 TestSingleFixed15_16(double d)
{
	cmsS15Fixed16Number f = _cmsDoubleTo15Fixed16(d);
	double RoundTrip = _cms15Fixed16toDouble(f);
	double Error     = fabs(d - RoundTrip);
	return ( Error <= FIXED_PRECISION_15_16);
}

static int32 CheckFixedPoint15_16()
{
	if(!TestSingleFixed15_16(1.0)) return 0;
	if(!TestSingleFixed15_16(2.0)) return 0;
	if(!TestSingleFixed15_16(1.23456)) return 0;
	if(!TestSingleFixed15_16(0.99999)) return 0;
	if(!TestSingleFixed15_16(0.1234567890123456789099999)) return 0;
	if(!TestSingleFixed15_16(-1.0)) return 0;
	if(!TestSingleFixed15_16(-2.0)) return 0;
	if(!TestSingleFixed15_16(-1.23456)) return 0;
	if(!TestSingleFixed15_16(-1.1234567890123456789099999)) return 0;
	if(!TestSingleFixed15_16(+32767.1234567890123456789099999)) return 0;
	if(!TestSingleFixed15_16(-32767.1234567890123456789099999)) return 0;
	return 1;
}

static int32 TestSingleFixed8_8(double d)
{
	cmsS15Fixed16Number f = _cmsDoubleTo8Fixed8(d);
	double RoundTrip = _cms8Fixed8toDouble((uint16)f);
	double Error     = fabs(d - RoundTrip);
	return ( Error <= FIXED_PRECISION_8_8);
}

static int32 CheckFixedPoint8_8()
{
	if(!TestSingleFixed8_8(1.0)) return 0;
	if(!TestSingleFixed8_8(2.0)) return 0;
	if(!TestSingleFixed8_8(1.23456)) return 0;
	if(!TestSingleFixed8_8(0.99999)) return 0;
	if(!TestSingleFixed8_8(0.1234567890123456789099999)) return 0;
	if(!TestSingleFixed8_8(+255.1234567890123456789099999)) return 0;

	return 1;
}

// D50 constant --------------------------------------------------------------------------------------------

static int32 CheckD50Roundtrip()
{
	double cmsD50X_2 =  0.96420288;
	double cmsD50Y_2 =  1.0;
	double cmsD50Z_2 = 0.82490540;
	cmsS15Fixed16Number xe = _cmsDoubleTo15Fixed16(cmsD50X);
	cmsS15Fixed16Number ye = _cmsDoubleTo15Fixed16(cmsD50Y);
	cmsS15Fixed16Number ze = _cmsDoubleTo15Fixed16(cmsD50Z);
	double x =  _cms15Fixed16toDouble(xe);
	double y =  _cms15Fixed16toDouble(ye);
	double z =  _cms15Fixed16toDouble(ze);
	double dx = fabs(cmsD50X - x);
	double dy = fabs(cmsD50Y - y);
	double dz = fabs(cmsD50Z - z);
	double euc = sqrt(dx*dx + dy*dy + dz* dz);
	if(euc > 1E-5) {
		Fail("D50 roundtrip |err| > (%f) ", euc);
		return 0;
	}

	xe = _cmsDoubleTo15Fixed16(cmsD50X_2);
	ye = _cmsDoubleTo15Fixed16(cmsD50Y_2);
	ze = _cmsDoubleTo15Fixed16(cmsD50Z_2);

	x =  _cms15Fixed16toDouble(xe);
	y =  _cms15Fixed16toDouble(ye);
	z =  _cms15Fixed16toDouble(ze);

	dx = fabs(cmsD50X_2 - x);
	dy = fabs(cmsD50Y_2 - y);
	dz = fabs(cmsD50Z_2 - z);

	euc = sqrt(dx*dx + dy*dy + dz* dz);

	if(euc > 1E-5) {
		Fail("D50 roundtrip |err| > (%f) ", euc);
		return 0;
	}

	return 1;
}

// Linear interpolation -----------------------------------------------------------------------------------------------

// Since prime factors of 65535 (FFFF) are,
//
//            0xFFFF = 3 * 5 * 17 * 257
//
// I test tables of 2, 4, 6, and 18 points, that will be exact.

static void BuildTable(int32 n, uint16 Tab[], boolint Descending)
{
	for(int32 i = 0; i < n; i++) {
		double v = (double)((double)65535.0 * i ) / (n-1);
		Tab[Descending ? (n - i - 1) : i ] = (uint16)floor(v + 0.5);
	}
}

// A single function that does check 1D interpolation
// nNodesToCheck = number on nodes to check
// Down = Create decreasing tables
// Reverse = Check reverse interpolation
// max_err = max allowed error

static int32 Check1D(int32 nNodesToCheck, boolint Down, int32 max_err)
{
	int32 ok = 1;
	uint16 in, out;
	cmsInterpParams* p;
	uint16 * Tab = (uint16 *)SAlloc::M(sizeof(uint16)* nNodesToCheck);
	if(Tab) {
		p = _cmsComputeInterpParams(DbgThread(), nNodesToCheck, 1, 1, Tab, CMS_LERP_FLAGS_16BITS);
		if(!p) 
			return 0;
		BuildTable(nNodesToCheck, Tab, Down);
		for(uint32 i = 0; i <= 0xffff; i++) {
			in = (uint16)i;
			out = 0;
			p->Interpolation.Lerp16(&in, &out, p);
			if(Down) 
				out = 0xffff - out;
			if(abs(out - in) > max_err) {
				Fail("(%dp): Must be %x, But is %x : ", nNodesToCheck, in, out);
				_cmsFreeInterpParams(p);
				SAlloc::F(Tab);
				return 0;
			}
		}
		_cmsFreeInterpParams(p);
		SAlloc::F(Tab);
	}
	else
		ok = 0;
	return ok;
}

static int32 Check1DLERP2() { return Check1D(2, FALSE, 0); }
static int32 Check1DLERP3() { return Check1D(3, FALSE, 1); }
static int32 Check1DLERP4() { return Check1D(4, FALSE, 0); }
static int32 Check1DLERP6() { return Check1D(6, FALSE, 0); }
static int32 Check1DLERP18() { return Check1D(18, FALSE, 0); }
static int32 Check1DLERP2Down() { return Check1D(2, TRUE, 0); }
static int32 Check1DLERP3Down() { return Check1D(3, TRUE, 1); }
static int32 Check1DLERP6Down() { return Check1D(6, TRUE, 0); }
static int32 Check1DLERP18Down() { return Check1D(18, TRUE, 0); }

static int32 ExhaustiveCheck1DLERP(FILE * fOut)
{
	fprintf(fOut, "\n");
	for(uint32 j = 10; j <= 4096; j++) {
		if((j % 10) == 0) 
			fprintf(fOut, "%u    \r", j);
		if(!Check1D(j, FALSE, 1)) 
			return 0;
	}
	fprintf(fOut, "\rResult is ");
	return 1;
}

static int32 ExhaustiveCheck1DLERPDown(FILE * fOut)
{
	fprintf(fOut, "\n");
	for(uint32 j = 10; j <= 4096; j++) {
		if((j % 10) == 0) 
			fprintf(fOut, "%u    \r", j);
		if(!Check1D(j, TRUE, 1)) 
			return 0;
	}
	fprintf(fOut, "\rResult is ");
	return 1;
}

// 3D interpolation -------------------------------------------------------------------------------------------------

static int32 Check3DinterpolationFloatTetrahedral(FILE * fOut)
{
	cmsInterpParams* p;
	int32 i;
	float In[3], Out[3];
	float FloatTable[] = { //R     G    B
		0,    0,   0,// B=0,G=0,R=0
		0,    0,  .25,// B=1,G=0,R=0

		0,   .5,    0,// B=0,G=1,R=0
		0,   .5,  .25,// B=1,G=1,R=0

		1,    0,    0,// B=0,G=0,R=1
		1,    0,  .25,// B=1,G=0,R=1

		1,    .5,   0,// B=0,G=1,R=1
		1,    .5,  .25// B=1,G=1,R=1
	};
	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT);
	MaxErr = 0.0;
	for(i = 0; i < 0xffff; i++) {
		In[0] = In[1] = In[2] = (float)( (float)i / 65535.0F);
		p->Interpolation.LerpFloat(In, Out, p);
		if(!IsGoodFixed15_16("Channel 1", Out[0], In[0])) 
			goto Error;
		if(!IsGoodFixed15_16("Channel 2", Out[1], (float)In[1] / 2.F)) 
			goto Error;
		if(!IsGoodFixed15_16("Channel 3", Out[2], (float)In[2] / 4.F)) 
			goto Error;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr);
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 Check3DinterpolationFloatTrilinear(FILE * fOut)
{
	cmsInterpParams* p;
	int32 i;
	float In[3], Out[3];
	float FloatTable[] = { //R     G    B
		0,    0,   0,// B=0,G=0,R=0
		0,    0,  .25,// B=1,G=0,R=0

		0,   .5,    0,// B=0,G=1,R=0
		0,   .5,  .25,// B=1,G=1,R=0

		1,    0,    0,// B=0,G=0,R=1
		1,    0,  .25,// B=1,G=0,R=1

		1,    .5,   0,// B=0,G=1,R=1
		1,    .5,  .25// B=1,G=1,R=1
	};
	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT|CMS_LERP_FLAGS_TRILINEAR);
	MaxErr = 0.0;
	for(i = 0; i < 0xffff; i++) {
		In[0] = In[1] = In[2] = (float)( (float)i / 65535.0F);
		p->Interpolation.LerpFloat(In, Out, p);
		if(!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
		if(!IsGoodFixed15_16("Channel 2", Out[1], (float)In[1] / 2.F)) goto Error;
		if(!IsGoodFixed15_16("Channel 3", Out[2], (float)In[2] / 4.F)) goto Error;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr);
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 Check3DinterpolationTetrahedral16(FILE * fOut)
{
	cmsInterpParams* p;
	int32 i;
	uint16 In[3], Out[3];
	uint16 Table[] = {
		0,    0,   0,
		0,    0,   0xffff,

		0,    0xffff,    0,
		0,    0xffff,    0xffff,

		0xffff,    0,    0,
		0xffff,    0,    0xffff,

		0xffff,    0xffff,   0,
		0xffff,    0xffff,   0xffff
	};
	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_16BITS);
	MaxErr = 0.0;
	for(i = 0; i < 0xffff; i++) {
		In[0] = In[1] = In[2] = (uint16)i;
		p->Interpolation.Lerp16(In, Out, p);
		if(!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
		if(!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
		if(!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr);
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 Check3DinterpolationTrilinear16(FILE * fOut)
{
	cmsInterpParams* p;
	int32 i;
	uint16 In[3], Out[3];
	uint16 Table[] = {
		0,    0,   0,
		0,    0,   0xffff,

		0,    0xffff,    0,
		0,    0xffff,    0xffff,

		0xffff,    0,    0,
		0xffff,    0,    0xffff,

		0xffff,    0xffff,   0,
		0xffff,    0xffff,   0xffff
	};
	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_TRILINEAR);
	MaxErr = 0.0;
	for(i = 0; i < 0xffff; i++) {
		In[0] = In[1] = In[2] = (uint16)i;
		p->Interpolation.Lerp16(In, Out, p);
		if(!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
		if(!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
		if(!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr);
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 ExaustiveCheck3DinterpolationFloatTetrahedral(FILE * fOut)
{
	cmsInterpParams* p;
	int32 r, g, b;
	float In[3], Out[3];
	float FloatTable[] = { //R     G    B
		0,    0,   0,// B=0,G=0,R=0
		0,    0,  .25,// B=1,G=0,R=0

		0,   .5,    0,// B=0,G=1,R=0
		0,   .5,  .25,// B=1,G=1,R=0

		1,    0,    0,// B=0,G=0,R=1
		1,    0,  .25,// B=1,G=0,R=1

		1,    .5,   0,// B=0,G=1,R=1
		1,    .5,  .25// B=1,G=1,R=1
	};

	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT);

	MaxErr = 0.0;
	for(r = 0; r < 0xff; r++)
		for(g = 0; g < 0xff; g++)
			for(b = 0; b < 0xff; b++) {
				In[0] = (float)r / 255.0F;
				In[1] = (float)g / 255.0F;
				In[2] = (float)b / 255.0F;

				p->Interpolation.LerpFloat(In, Out, p);

				if(!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
				if(!IsGoodFixed15_16("Channel 2", Out[1], (float)In[1] / 2.F)) goto Error;
				if(!IsGoodFixed15_16("Channel 3", Out[2], (float)In[2] / 4.F)) goto Error;
			}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr);
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 ExaustiveCheck3DinterpolationFloatTrilinear(FILE * fOut)
{
	cmsInterpParams* p;
	int32 r, g, b;
	float In[3], Out[3];
	float FloatTable[] = { //R     G    B
		0,    0,   0,// B=0,G=0,R=0
		0,    0,  .25,// B=1,G=0,R=0

		0,   .5,    0,// B=0,G=1,R=0
		0,   .5,  .25,// B=1,G=1,R=0

		1,    0,    0,// B=0,G=0,R=1
		1,    0,  .25,// B=1,G=0,R=1

		1,    .5,   0,// B=0,G=1,R=1
		1,    .5,  .25// B=1,G=1,R=1
	};
	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, FloatTable, CMS_LERP_FLAGS_FLOAT|CMS_LERP_FLAGS_TRILINEAR);
	MaxErr = 0.0;
	for(r = 0; r < 0xff; r++)
		for(g = 0; g < 0xff; g++)
			for(b = 0; b < 0xff; b++) {
				In[0] = (float)r / 255.0F;
				In[1] = (float)g / 255.0F;
				In[2] = (float)b / 255.0F;

				p->Interpolation.LerpFloat(In, Out, p);

				if(!IsGoodFixed15_16("Channel 1", Out[0], In[0])) goto Error;
				if(!IsGoodFixed15_16("Channel 2", Out[1], (float)In[1] / 2.F)) goto Error;
				if(!IsGoodFixed15_16("Channel 3", Out[2], (float)In[2] / 4.F)) goto Error;
			}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr);
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 ExhaustiveCheck3DinterpolationTetrahedral16()
{
	cmsInterpParams* p;
	int32 r, g, b;
	uint16 In[3], Out[3];
	uint16 Table[] = {
		0,    0,   0,
		0,    0,   0xffff,

		0,    0xffff,    0,
		0,    0xffff,    0xffff,

		0xffff,    0,    0,
		0xffff,    0,    0xffff,

		0xffff,    0xffff,   0,
		0xffff,    0xffff,   0xffff
	};

	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_16BITS);

	for(r = 0; r < 0xff; r++)
		for(g = 0; g < 0xff; g++)
			for(b = 0; b < 0xff; b++) {
				In[0] = (uint16)r;
				In[1] = (uint16)g;
				In[2] = (uint16)b;
				p->Interpolation.Lerp16(In, Out, p);
				if(!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
				if(!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
				if(!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
			}

	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

static int32 ExhaustiveCheck3DinterpolationTrilinear16()
{
	cmsInterpParams* p;
	int32 r, g, b;
	uint16 In[3], Out[3];
	uint16 Table[] = {
		0,    0,   0,
		0,    0,   0xffff,

		0,    0xffff,    0,
		0,    0xffff,    0xffff,

		0xffff,    0,    0,
		0xffff,    0,    0xffff,

		0xffff,    0xffff,   0,
		0xffff,    0xffff,   0xffff
	};

	p = _cmsComputeInterpParams(DbgThread(), 2, 3, 3, Table, CMS_LERP_FLAGS_TRILINEAR);

	for(r = 0; r < 0xff; r++)
		for(g = 0; g < 0xff; g++)
			for(b = 0; b < 0xff; b++) {
				In[0] = (uint16)r;
				In[1] = (uint16)g;
				In[2] = (uint16)b;

				p->Interpolation.Lerp16(In, Out, p);

				if(!IsGoodWord("Channel 1", Out[0], In[0])) goto Error;
				if(!IsGoodWord("Channel 2", Out[1], In[1])) goto Error;
				if(!IsGoodWord("Channel 3", Out[2], In[2])) goto Error;
			}
	_cmsFreeInterpParams(p);
	return 1;
Error:
	_cmsFreeInterpParams(p);
	return 0;
}

// Check reverse interpolation on LUTS. This is right now exclusively used by K preservation algorithm
static int32 CheckReverseInterpolation3x3()
{
	cmsPipeline * Lut;
	cmsStage * clut;
	float Target[4], Result[4], Hint[4];
	float err, max;
	int32 i;
	uint16 Table[] = {
		0,    0,   0,         // 0 0 0
		0,    0,   0xffff,    // 0 0 1

		0,    0xffff,    0,   // 0 1 0
		0,    0xffff,    0xffff,// 0 1 1

		0xffff,    0,    0,   // 1 0 0
		0xffff,    0,    0xffff,// 1 0 1

		0xffff,    0xffff,   0,// 1 1 0
		0xffff,    0xffff,   0xffff,// 1 1 1
	};

	Lut = cmsPipelineAlloc(DbgThread(), 3, 3);

	clut = cmsStageAllocCLut16bit(DbgThread(), 2, 3, 3, Table);
	cmsPipelineInsertStage(Lut, cmsAT_BEGIN, clut);

	Target[0] = 0; Target[1] = 0; Target[2] = 0;
	Hint[0] = 0; Hint[1] = 0; Hint[2] = 0;
	cmsPipelineEvalReverseFloat(Target, Result, NULL, Lut);
	if(Result[0] != 0 || Result[1] != 0 || Result[2] != 0) {
		Fail("Reverse interpolation didn't find zero");
		goto Error;
	}
	// Transverse identity
	max = 0;
	for(i = 0; i <= 100; i++) {
		float in = i / 100.0F;
		Target[0] = in; Target[1] = 0; Target[2] = 0;
		cmsPipelineEvalReverseFloat(Target, Result, Hint, Lut);
		err = fabsf(in - Result[0]);
		if(err > max) max = err;
		memcpy(Hint, Result, sizeof(Hint));
	}
	cmsPipelineFree(Lut);
	return (max <= FLOAT_PRECISSION);
Error:
	cmsPipelineFree(Lut);
	return 0;
}

static int32 CheckReverseInterpolation4x3(FILE * fOut)
{
	cmsPipeline * Lut;
	cmsStage * clut;
	float Target[4], Result[4], Hint[4];
	float err, max;
	int32 i;

	// 4 -> 3, output gets 3 first channels copied
	uint16 Table[] = {
		0,         0,         0,  //  0 0 0 0   = ( 0, 0, 0)
		0,         0,         0,  //  0 0 0 1   = ( 0, 0, 0)

		0,         0,         0xffff,//  0 0 1 0   = ( 0, 0, 1)
		0,         0,         0xffff,//  0 0 1 1   = ( 0, 0, 1)

		0,         0xffff,    0,  //  0 1 0 0   = ( 0, 1, 0)
		0,         0xffff,    0,  //  0 1 0 1   = ( 0, 1, 0)

		0,         0xffff,    0xffff,//  0 1 1 0    = ( 0, 1, 1)
		0,         0xffff,    0xffff,//  0 1 1 1    = ( 0, 1, 1)

		0xffff,    0,         0,  //  1 0 0 0    = ( 1, 0, 0)
		0xffff,    0,         0,  //  1 0 0 1    = ( 1, 0, 0)

		0xffff,    0,         0xffff,//  1 0 1 0    = ( 1, 0, 1)
		0xffff,    0,         0xffff,//  1 0 1 1    = ( 1, 0, 1)

		0xffff,    0xffff,    0,  //  1 1 0 0    = ( 1, 1, 0)
		0xffff,    0xffff,    0,  //  1 1 0 1    = ( 1, 1, 0)

		0xffff,    0xffff,    0xffff,//  1 1 1 0    = ( 1, 1, 1)
		0xffff,    0xffff,    0xffff,//  1 1 1 1    = ( 1, 1, 1)
	};

	Lut = cmsPipelineAlloc(DbgThread(), 4, 3);

	clut = cmsStageAllocCLut16bit(DbgThread(), 2, 4, 3, Table);
	cmsPipelineInsertStage(Lut, cmsAT_BEGIN, clut);

	// Check if the LUT is behaving as expected
	SubTest(fOut, "4->3 feasibility");
	for(i = 0; i <= 100; i++) {
		Target[0] = i / 100.0F;
		Target[1] = Target[0];
		Target[2] = 0;
		Target[3] = 12;

		cmsPipelineEvalFloat(Target, Result, Lut);

		if(!IsGoodFixed15_16("0", Target[0], Result[0])) goto Error;
		if(!IsGoodFixed15_16("1", Target[1], Result[1])) goto Error;
		if(!IsGoodFixed15_16("2", Target[2], Result[2])) goto Error;
	}
	SubTest(fOut, "4->3 zero");
	Target[0] = 0;
	Target[1] = 0;
	Target[2] = 0;
	// This one holds the fixed K
	Target[3] = 0;
	// This is our hint (which is a big lie in this case)
	Hint[0] = 0.1F; Hint[1] = 0.1F; Hint[2] = 0.1F;
	cmsPipelineEvalReverseFloat(Target, Result, Hint, Lut);
	if(Result[0] != 0 || Result[1] != 0 || Result[2] != 0 || Result[3] != 0) {
		Fail("Reverse interpolation didn't find zero");
		goto Error;
	}
	SubTest(fOut, "4->3 find CMY");
	max = 0;
	for(i = 0; i <= 100; i++) {
		float in = i / 100.0F;
		Target[0] = in; Target[1] = 0; Target[2] = 0;
		cmsPipelineEvalReverseFloat(Target, Result, Hint, Lut);
		err = fabsf(in - Result[0]);
		SETMAX(max, err);
		memcpy(Hint, Result, sizeof(Hint));
	}
	cmsPipelineFree(Lut);
	return (max <= FLOAT_PRECISSION);
Error:
	cmsPipelineFree(Lut);
	return 0;
}

// Check all interpolation.

static uint16 Fn8D1(uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7, uint16 a8, uint32 m)
{
	return (uint16)((a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8) / m);
}

static uint16 Fn8D2(uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7, uint16 a8, uint32 m)
{
	return (uint16)((a1 + 3* a2 + 3* a3 + a4 + a5 + a6 + a7 + a8 ) / (m + 4));
}

static uint16 Fn8D3(uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7, uint16 a8, uint32 m)
{
	return (uint16)((3*a1 + 2*a2 + 3*a3 + a4 + a5 + a6 + a7 + a8) / (m + 5));
}

static boolint Sampler3D(const uint16 In[], uint16 Out[], void * Cargo)
{
	Out[0] = Fn8D1(In[0], In[1], In[2], 0, 0, 0, 0, 0, 3);
	Out[1] = Fn8D2(In[0], In[1], In[2], 0, 0, 0, 0, 0, 3);
	Out[2] = Fn8D3(In[0], In[1], In[2], 0, 0, 0, 0, 0, 3);
	return 1;
	CXX_UNUSED(Cargo);
}

static boolint Sampler4D(const uint16 In[], uint16 Out[], void * Cargo)
{
	Out[0] = Fn8D1(In[0], In[1], In[2], In[3], 0, 0, 0, 0, 4);
	Out[1] = Fn8D2(In[0], In[1], In[2], In[3], 0, 0, 0, 0, 4);
	Out[2] = Fn8D3(In[0], In[1], In[2], In[3], 0, 0, 0, 0, 4);
	return 1;
	CXX_UNUSED(Cargo);
}

static boolint Sampler5D(const uint16 In[], uint16 Out[], void * Cargo)
{
	Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], 0, 0, 0, 5);
	Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], 0, 0, 0, 5);
	Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], 0, 0, 0, 5);
	return 1;
	CXX_UNUSED(Cargo);
}

static boolint Sampler6D(const uint16 In[], uint16 Out[], void * Cargo)
{
	Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], In[5], 0, 0, 6);
	Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], In[5], 0, 0, 6);
	Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], In[5], 0, 0, 6);
	return 1;
	CXX_UNUSED(Cargo);
}

static boolint Sampler7D(const uint16 In[], uint16 Out[], void * Cargo)
{
	Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], In[5], In[6], 0, 7);
	Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], In[5], In[6], 0, 7);
	Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], In[5], In[6], 0, 7);
	return 1;
	CXX_UNUSED(Cargo);
}

static boolint Sampler8D(const uint16 In[], uint16 Out[], void * Cargo)
{
	Out[0] = Fn8D1(In[0], In[1], In[2], In[3], In[4], In[5], In[6], In[7], 8);
	Out[1] = Fn8D2(In[0], In[1], In[2], In[3], In[4], In[5], In[6], In[7], 8);
	Out[2] = Fn8D3(In[0], In[1], In[2], In[3], In[4], In[5], In[6], In[7], 8);
	return 1;
	CXX_UNUSED(Cargo);
}

static boolint CheckOne3D(cmsPipeline * lut, uint16 a1, uint16 a2, uint16 a3)
{
	uint16 In[3], Out1[3], Out2[3];
	In[0] = a1; In[1] = a2; In[2] = a3;
	// This is the interpolated value
	cmsPipelineEval16(In, Out1, lut);
	// This is the real value
	Sampler3D(In, Out2, NULL);
	// Let's see the difference
	if(!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;
	return TRUE;
}

static boolint CheckOne4D(cmsPipeline * lut, uint16 a1, uint16 a2, uint16 a3, uint16 a4)
{
	uint16 In[4], Out1[3], Out2[3];
	In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4;
	// This is the interpolated value
	cmsPipelineEval16(In, Out1, lut);
	// This is the real value
	Sampler4D(In, Out2, NULL);
	// Let's see the difference
	if(!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;
	return TRUE;
}

static boolint CheckOne5D(cmsPipeline * lut, uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5)
{
	uint16 In[5], Out1[3], Out2[3];
	In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5;
	// This is the interpolated value
	cmsPipelineEval16(In, Out1, lut);
	// This is the real value
	Sampler5D(In, Out2, NULL);
	// Let's see the difference
	if(!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;
	return TRUE;
}

static boolint CheckOne6D(cmsPipeline * lut, uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5, uint16 a6)
{
	uint16 In[6], Out1[3], Out2[3];
	In[0] = a1; 
	In[1] = a2; 
	In[2] = a3; 
	In[3] = a4; 
	In[4] = a5; 
	In[5] = a6;
	// This is the interpolated value
	cmsPipelineEval16(In, Out1, lut);
	// This is the real value
	Sampler6D(In, Out2, NULL);
	// Let's see the difference
	if(!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;
	return TRUE;
}

static boolint CheckOne7D(cmsPipeline * lut, uint16 a1, uint16 a2,
    uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7)
{
	uint16 In[7], Out1[3], Out2[3];
	In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5; In[5] = a6; In[6] = a7;
	// This is the interpolated value
	cmsPipelineEval16(In, Out1, lut);
	// This is the real value
	Sampler7D(In, Out2, NULL);
	// Let's see the difference
	if(!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;
	return TRUE;
}

static boolint CheckOne8D(cmsPipeline * lut, uint16 a1, uint16 a2,
    uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7, uint16 a8)
{
	uint16 In[8], Out1[3], Out2[3];

	In[0] = a1; In[1] = a2; In[2] = a3; In[3] = a4; In[4] = a5; In[5] = a6; In[6] = a7; In[7] = a8;

	// This is the interpolated value
	cmsPipelineEval16(In, Out1, lut);

	// This is the real value
	Sampler8D(In, Out2, NULL);

	// Let's see the difference

	if(!IsGoodWordPrec("Channel 1", Out1[0], Out2[0], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 2", Out1[1], Out2[1], 2)) return FALSE;
	if(!IsGoodWordPrec("Channel 3", Out1[2], Out2[2], 2)) return FALSE;

	return TRUE;
}

static int32 Check3Dinterp()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	cmsStage * mpe = cmsStageAllocCLut16bit(DbgThread(), 9, 3, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler3D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne3D(lut, 0, 0, 0)) return 0;
	if(!CheckOne3D(lut, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne3D(lut, 0x8080, 0x8080, 0x8080)) return 0;
	if(!CheckOne3D(lut, 0x0000, 0xFE00, 0x80FF)) return 0;
	if(!CheckOne3D(lut, 0x1111, 0x2222, 0x3333)) return 0;
	if(!CheckOne3D(lut, 0x0000, 0x0012, 0x0013)) return 0;
	if(!CheckOne3D(lut, 0x3141, 0x1415, 0x1592)) return 0;
	if(!CheckOne3D(lut, 0xFF00, 0xFF01, 0xFF12)) return 0;
	cmsPipelineFree(lut);
	return 1;
}

static int32 Check3DinterpGranular()
{
	cmsPipeline * lut;
	cmsStage * mpe;
	uint32 Dimensions[] = { 7, 8, 9 };
	lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 3, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler3D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne3D(lut, 0, 0, 0)) return 0;
	if(!CheckOne3D(lut, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne3D(lut, 0x8080, 0x8080, 0x8080)) return 0;
	if(!CheckOne3D(lut, 0x0000, 0xFE00, 0x80FF)) return 0;
	if(!CheckOne3D(lut, 0x1111, 0x2222, 0x3333)) return 0;
	if(!CheckOne3D(lut, 0x0000, 0x0012, 0x0013)) return 0;
	if(!CheckOne3D(lut, 0x3141, 0x1415, 0x1592)) return 0;
	if(!CheckOne3D(lut, 0xFF00, 0xFF01, 0xFF12)) return 0;
	cmsPipelineFree(lut);
	return 1;
}

static int32 Check4Dinterp()
{
	cmsPipeline * lut;
	cmsStage * mpe;
	lut = cmsPipelineAlloc(DbgThread(), 4, 3);
	mpe = cmsStageAllocCLut16bit(DbgThread(), 9, 4, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler4D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne4D(lut, 0, 0, 0, 0)) return 0;
	if(!CheckOne4D(lut, 0xffff, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne4D(lut, 0x8080, 0x8080, 0x8080, 0x8080)) return 0;
	if(!CheckOne4D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888)) return 0;
	if(!CheckOne4D(lut, 0x1111, 0x2222, 0x3333, 0x4444)) return 0;
	if(!CheckOne4D(lut, 0x0000, 0x0012, 0x0013, 0x0014)) return 0;
	if(!CheckOne4D(lut, 0x3141, 0x1415, 0x1592, 0x9261)) return 0;
	if(!CheckOne4D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13)) return 0;
	cmsPipelineFree(lut);
	return 1;
}

static int32 Check4DinterpGranular()
{
	cmsPipeline * lut;
	cmsStage * mpe;
	uint32 Dimensions[] = { 9, 8, 7, 6 };
	lut = cmsPipelineAlloc(DbgThread(), 4, 3);
	mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 4, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler4D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne4D(lut, 0, 0, 0, 0)) return 0;
	if(!CheckOne4D(lut, 0xffff, 0xffff, 0xffff, 0xffff)) return 0;

	if(!CheckOne4D(lut, 0x8080, 0x8080, 0x8080, 0x8080)) return 0;
	if(!CheckOne4D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888)) return 0;
	if(!CheckOne4D(lut, 0x1111, 0x2222, 0x3333, 0x4444)) return 0;
	if(!CheckOne4D(lut, 0x0000, 0x0012, 0x0013, 0x0014)) return 0;
	if(!CheckOne4D(lut, 0x3141, 0x1415, 0x1592, 0x9261)) return 0;
	if(!CheckOne4D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13)) return 0;

	cmsPipelineFree(lut);

	return 1;
}

static int32 Check5DinterpGranular()
{
	uint32 Dimensions[] = { 3, 2, 2, 2, 2 };
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 5, 3);
	cmsStage * mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 5, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler5D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne5D(lut, 0, 0, 0, 0, 0)) return 0;
	if(!CheckOne5D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne5D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234)) return 0;
	if(!CheckOne5D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078)) return 0;
	if(!CheckOne5D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455)) return 0;
	if(!CheckOne5D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333)) return 0;
	if(!CheckOne5D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567)) return 0;
	if(!CheckOne5D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344)) return 0;
	cmsPipelineFree(lut);
	return 1;
}

static int32 Check6DinterpGranular()
{
	uint32 Dimensions[] = { 4, 3, 3, 2, 2, 2 };
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 6, 3);
	cmsStage * mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 6, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler6D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne6D(lut, 0, 0, 0, 0, 0, 0)) return 0;
	if(!CheckOne6D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne6D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234, 0x1122)) return 0;
	if(!CheckOne6D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078, 0x2233)) return 0;
	if(!CheckOne6D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455, 0x3344)) return 0;
	if(!CheckOne6D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333, 0x4455)) return 0;
	if(!CheckOne6D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567, 0x5566)) return 0;
	if(!CheckOne6D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344, 0x6677)) return 0;
	cmsPipelineFree(lut);
	return 1;
}

static int32 Check7DinterpGranular()
{
	cmsPipeline * lut;
	cmsStage * mpe;
	uint32 Dimensions[] = { 4, 3, 3, 2, 2, 2, 2 };
	lut = cmsPipelineAlloc(DbgThread(), 7, 3);
	mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 7, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler7D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne7D(lut, 0, 0, 0, 0, 0, 0, 0)) return 0;
	if(!CheckOne7D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne7D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234, 0x1122, 0x0056)) return 0;
	if(!CheckOne7D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078, 0x2233, 0x0088)) return 0;
	if(!CheckOne7D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455, 0x3344, 0x1987)) return 0;
	if(!CheckOne7D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333, 0x4455, 0x9988)) return 0;
	if(!CheckOne7D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567, 0x5566, 0xfe56)) return 0;
	if(!CheckOne7D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344, 0x6677, 0xbabe)) return 0;
	cmsPipelineFree(lut);
	return 1;
}

static int32 Check8DinterpGranular()
{
	cmsPipeline * lut;
	cmsStage * mpe;
	uint32 Dimensions[] = { 4, 3, 3, 2, 2, 2, 2, 2 };
	lut = cmsPipelineAlloc(DbgThread(), 8, 3);
	mpe = cmsStageAllocCLut16bitGranular(DbgThread(), Dimensions, 8, 3, NULL);
	cmsStageSampleCLut16bit(mpe, Sampler8D, NULL, 0);
	cmsPipelineInsertStage(lut, cmsAT_BEGIN, mpe);
	// Check accuracy
	if(!CheckOne8D(lut, 0, 0, 0, 0, 0, 0, 0, 0)) return 0;
	if(!CheckOne8D(lut, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff)) return 0;
	if(!CheckOne8D(lut, 0x8080, 0x8080, 0x8080, 0x8080, 0x1234, 0x1122, 0x0056, 0x0011)) return 0;
	if(!CheckOne8D(lut, 0x0000, 0xFE00, 0x80FF, 0x8888, 0x8078, 0x2233, 0x0088, 0x2020)) return 0;
	if(!CheckOne8D(lut, 0x1111, 0x2222, 0x3333, 0x4444, 0x1455, 0x3344, 0x1987, 0x4532)) return 0;
	if(!CheckOne8D(lut, 0x0000, 0x0012, 0x0013, 0x0014, 0x2333, 0x4455, 0x9988, 0x1200)) return 0;
	if(!CheckOne8D(lut, 0x3141, 0x1415, 0x1592, 0x9261, 0x4567, 0x5566, 0xfe56, 0x6666)) return 0;
	if(!CheckOne8D(lut, 0xFF00, 0xFF01, 0xFF12, 0xFF13, 0xF344, 0x6677, 0xbabe, 0xface)) return 0;
	cmsPipelineFree(lut);
	return 1;
}
//
// Colorimetric conversions
//
// Lab to LCh and back should be performed at 1E-12 accuracy at least
static int32 CheckLab2LCh()
{
	int32 l, a, b;
	double dist, Max = 0;
	cmsCIELab Lab, Lab2;
	cmsCIELCh LCh;
	for(l = 0; l <= 100; l += 10) {
		for(a = -128; a <= +128; a += 8) {
			for(b = -128; b <= 128; b += 8) {
				Lab.L = l;
				Lab.a = a;
				Lab.b = b;
				cmsLab2LCh(&LCh, &Lab);
				cmsLCh2Lab(&Lab2, &LCh);
				dist = cmsDeltaE(&Lab, &Lab2);
				SETMAX(Max, dist);
			}
		}
	}
	return (Max < 1E-12);
}

// Lab to LCh and back should be performed at 1E-12 accuracy at least
static int32 CheckLab2XYZ()
{
	int32 l, a, b;
	double dist, Max = 0;
	cmsCIELab Lab, Lab2;
	cmsCIEXYZ XYZ;
	for(l = 0; l <= 100; l += 10) {
		for(a = -128; a <= +128; a += 8) {
			for(b = -128; b <= 128; b += 8) {
				Lab.L = l;
				Lab.a = a;
				Lab.b = b;
				cmsLab2XYZ(NULL, &XYZ, &Lab);
				cmsXYZ2Lab(NULL, &Lab2, &XYZ);
				dist = cmsDeltaE(&Lab, &Lab2);
				SETMAX(Max, dist);
			}
		}
	}
	return (Max < 1E-12);
}

// Lab to xyY and back should be performed at 1E-12 accuracy at least
static int32 CheckLab2xyY()
{
	int32 l, a, b;
	double dist, Max = 0;
	cmsCIELab Lab, Lab2;
	cmsCIEXYZ XYZ;
	cmsCIExyY xyY;
	for(l = 0; l <= 100; l += 10) {
		for(a = -128; a <= +128; a += 8) {
			for(b = -128; b <= 128; b += 8) {
				Lab.L = l;
				Lab.a = a;
				Lab.b = b;
				cmsLab2XYZ(NULL, &XYZ, &Lab);
				cmsXYZ2xyY(&xyY, &XYZ);
				cmsxyY2XYZ(&XYZ, &xyY);
				cmsXYZ2Lab(NULL, &Lab2, &XYZ);
				dist = cmsDeltaE(&Lab, &Lab2);
				SETMAX(Max, dist);
			}
		}
	}
	return (Max < 1E-12);
}

static int32 CheckLabV2encoding()
{
	int32 n2, i, j;
	uint16 Inw[3], aw[3];
	cmsCIELab Lab;
	n2 = 0;
	for(j = 0; j < 65535; j++) {
		Inw[0] = Inw[1] = Inw[2] = (uint16)j;
		cmsLabEncoded2FloatV2(&Lab, Inw);
		cmsFloat2LabEncodedV2(aw, &Lab);
		for(i = 0; i < 3; i++) {
			if(aw[i] != j) {
				n2++;
			}
		}
	}
	return (n2 == 0);
}

static int32 CheckLabV4encoding()
{
	int32 n2, i, j;
	uint16 Inw[3], aw[3];
	cmsCIELab Lab;
	n2 = 0;
	for(j = 0; j < 65535; j++) {
		Inw[0] = Inw[1] = Inw[2] = (uint16)j;
		cmsLabEncoded2Float(&Lab, Inw);
		cmsFloat2LabEncoded(aw, &Lab);
		for(i = 0; i < 3; i++) {
			if(aw[i] != j) {
				n2++;
			}
		}
	}
	return (n2 == 0);
}
//
// BlackBody
//
static int32 CheckTemp2CHRM()
{
	int32 j;
	double d, v, Max = 0;
	cmsCIExyY White;
	for(j = 4000; j < 25000; j++) {
		cmsWhitePointFromTemp(&White, j);
		if(!cmsTempFromWhitePoint(&v, &White)) 
			return 0;
		d = fabs(v - j);
		SETMAX(Max, d);
	}
	// 100 degree is the actual resolution
	return (Max < 100);
}

// Tone curves -----------------------------------------------------------------------------------------------------

static int32 CheckGammaEstimation(FILE * fOut, cmsToneCurve * c, double g)
{
	double est = cmsEstimateGamma(c, 0.001);
	SubTest(fOut, "Gamma estimation");
	if(fabs(est - g) > 0.001) 
		return 0;
	return 1;
}

static int32 CheckGammaCreation16(FILE * fOut)
{
	cmsToneCurve * LinGamma = cmsBuildGamma(DbgThread(), 1.0);
	int32 i;
	uint16 in, out;
	for(i = 0; i < 0xffff; i++) {
		in = (uint16)i;
		out = cmsEvalToneCurve16(LinGamma, in);
		if(in != out) {
			Fail("(lin gamma): Must be %x, But is %x : ", in, out);
			cmsFreeToneCurve(LinGamma);
			return 0;
		}
	}
	if(!CheckGammaEstimation(fOut, LinGamma, 1.0)) 
		return 0;
	cmsFreeToneCurve(LinGamma);
	return 1;
}

static int32 CheckGammaCreationFlt(FILE * fOut)
{
	cmsToneCurve * LinGamma = cmsBuildGamma(DbgThread(), 1.0);
	int32 i;
	float in, out;
	for(i = 0; i < 0xffff; i++) {
		in = (float)(i / 65535.0);
		out = cmsEvalToneCurveFloat(LinGamma, in);
		if(fabs(in - out) > (1/65535.0)) {
			Fail("(lin gamma): Must be %f, But is %f : ", in, out);
			cmsFreeToneCurve(LinGamma);
			return 0;
		}
	}
	if(!CheckGammaEstimation(fOut, LinGamma, 1.0)) 
		return 0;
	cmsFreeToneCurve(LinGamma);
	return 1;
}

// Curve curves using a single power function
// Error is given in 0..ffff counts
static int32 CheckGammaFloat(FILE * fOut, double g)
{
	cmsToneCurve * Curve = cmsBuildGamma(DbgThread(), g);
	int32 i;
	float in, out;
	double val, Err;
	MaxErr = 0.0;
	for(i = 0; i < 0xffff; i++) {
		in = (float)(i / 65535.0);
		out = cmsEvalToneCurveFloat(Curve, in);
		val = pow((double)in, g);
		Err = fabs(val - out);
		if(Err > MaxErr) MaxErr = Err;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr * 65535.0);
	if(!CheckGammaEstimation(fOut, Curve, g)) 
		return 0;
	cmsFreeToneCurve(Curve);
	return 1;
}

static int32 CheckGamma18(FILE * fOut)
{
	return CheckGammaFloat(fOut, 1.8);
}

static int32 CheckGamma22(FILE * fOut)
{
	return CheckGammaFloat(fOut, 2.2);
}

static int32 CheckGamma30(FILE * fOut)
{
	return CheckGammaFloat(fOut, 3.0);
}

// Check table-based gamma functions
static int32 CheckGammaFloatTable(FILE * fOut, double g)
{
	float Values[1025];
	cmsToneCurve * Curve;
	int32 i;
	float in, out;
	double val, Err;
	for(i = 0; i <= 1024; i++) {
		in = (float)(i / 1024.0);
		Values[i] = powf(in, (float)g);
	}
	Curve = cmsBuildTabulatedToneCurveFloat(DbgThread(), 1025, Values);
	MaxErr = 0.0;
	for(i = 0; i <= 0xffff; i++) {
		in = (float)(i / 65535.0);
		out = cmsEvalToneCurveFloat(Curve, in);
		val = pow(in, g);
		Err = fabs(val - out);
		if(Err > MaxErr) 
			MaxErr = Err;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr * 65535.0);
	if(!CheckGammaEstimation(fOut, Curve, g)) 
		return 0;
	cmsFreeToneCurve(Curve);
	return 1;
}

static int32 CheckGamma18Table(FILE * fOut) { return CheckGammaFloatTable(fOut, 1.8); }
static int32 CheckGamma22Table(FILE * fOut) { return CheckGammaFloatTable(fOut, 2.2); }
static int32 CheckGamma30Table(FILE * fOut) { return CheckGammaFloatTable(fOut, 3.0); }

// Create a curve from a table (which is a pure gamma function) and check it against the pow function.
static int32 CheckGammaWordTable(FILE * fOut, double g)
{
	uint16 Values[1025];
	cmsToneCurve * Curve;
	int32 i;
	float in, out;
	double val, Err;
	for(i = 0; i <= 1024; i++) {
		in = (float)(i / 1024.0);
		Values[i] = (uint16)floor(pow(in, g) * 65535.0 + 0.5);
	}
	Curve = cmsBuildTabulatedToneCurve16(DbgThread(), 1025, Values);
	MaxErr = 0.0;
	for(i = 0; i <= 0xffff; i++) {
		in = (float)(i / 65535.0);
		out = cmsEvalToneCurveFloat(Curve, in);
		val = pow(in, g);
		Err = fabs(val - out);
		if(Err > MaxErr) 
			MaxErr = Err;
	}
	if(MaxErr > 0) 
		fprintf(fOut, "|Err|<%lf ", MaxErr * 65535.0);
	if(!CheckGammaEstimation(fOut, Curve, g)) 
		return 0;
	cmsFreeToneCurve(Curve);
	return 1;
}

static int32 CheckGamma18TableWord(FILE * fOut) { return CheckGammaWordTable(fOut, 1.8); }
static int32 CheckGamma22TableWord(FILE * fOut) { return CheckGammaWordTable(fOut, 2.2); }
static int32 CheckGamma30TableWord(FILE * fOut) { return CheckGammaWordTable(fOut, 3.0); }

// Curve joining test. Joining two high-gamma of 3.0 curves should
// give something like linear
static int32 CheckJointCurves()
{
	boolint rc;
	cmsToneCurve * Forward = cmsBuildGamma(DbgThread(), 3.0);
	cmsToneCurve * Reverse = cmsBuildGamma(DbgThread(), 3.0);
	cmsToneCurve * Result = cmsJoinToneCurve(DbgThread(), Forward, Reverse, 256);
	cmsFreeToneCurve(Forward); 
	cmsFreeToneCurve(Reverse);
	rc = cmsIsToneCurveLinear(Result);
	cmsFreeToneCurve(Result);
	if(!rc)
		Fail("Joining same curve twice does not result in a linear ramp");
	return rc;
}

// Create a gamma curve by cheating the table
static cmsToneCurve * GammaTableLinear(int32 nEntries, boolint Dir)
{
	cmsToneCurve * g = cmsBuildTabulatedToneCurve16(DbgThread(), nEntries, NULL);
	for(int32 i = 0; i < nEntries; i++) {
		int32 v = _cmsQuantizeVal(i, nEntries);
		if(Dir)
			g->Table16[i] = (uint16)v;
		else
			g->Table16[i] = (uint16)(0xFFFF - v);
	}
	return g;
}

static int32 CheckJointCurvesDescending()
{
	cmsToneCurve * Reverse, * Result;
	int32 i, rc;
	cmsToneCurve * Forward = cmsBuildGamma(DbgThread(), 2.2);
	// Fake the curve to be table-based
	for(i = 0; i < 4096; i++)
		Forward->Table16[i] = 0xffff - Forward->Table16[i];
	Forward->Segments[0].Type = 0;
	Reverse = cmsReverseToneCurve(Forward);
	Result = cmsJoinToneCurve(DbgThread(), Reverse, Reverse, 256);
	cmsFreeToneCurve(Forward);
	cmsFreeToneCurve(Reverse);
	rc = cmsIsToneCurveLinear(Result);
	cmsFreeToneCurve(Result);
	return rc;
}

static int32 CheckFToneCurvePoint(cmsToneCurve * c, uint16 Point, int32 Value)
{
	int32 Result = cmsEvalToneCurve16(c, Point);
	return (abs(Value - Result) < 2);
}

static int32 CheckReverseDegenerated()
{
	cmsToneCurve * p, * g;
	uint16 Tab[16];
	Tab[0] = 0;
	Tab[1] = 0;
	Tab[2] = 0;
	Tab[3] = 0;
	Tab[4] = 0;
	Tab[5] = 0x5555;
	Tab[6] = 0x6666;
	Tab[7] = 0x7777;
	Tab[8] = 0x8888;
	Tab[9] = 0x9999;
	Tab[10] = 0xffff;
	Tab[11] = 0xffff;
	Tab[12] = 0xffff;
	Tab[13] = 0xffff;
	Tab[14] = 0xffff;
	Tab[15] = 0xffff;

	p = cmsBuildTabulatedToneCurve16(DbgThread(), 16, Tab);
	g = cmsReverseToneCurve(p);

	// Now let's check some points
	if(!CheckFToneCurvePoint(g, 0x5555, 0x5555)) return 0;
	if(!CheckFToneCurvePoint(g, 0x7777, 0x7777)) return 0;

	// First point for zero
	if(!CheckFToneCurvePoint(g, 0x0000, 0x4444)) return 0;

	// Last point
	if(!CheckFToneCurvePoint(g, 0xFFFF, 0xFFFF)) return 0;

	cmsFreeToneCurve(p);
	cmsFreeToneCurve(g);

	return 1;
}

// Build a parametric sRGB-like curve
static cmsToneCurve * Build_sRGBGamma()
{
	double Parameters[5];
	Parameters[0] = 2.4;
	Parameters[1] = 1. / 1.055;
	Parameters[2] = 0.055 / 1.055;
	Parameters[3] = 1. / 12.92;
	Parameters[4] = 0.04045; // d
	return cmsBuildParametricToneCurve(DbgThread(), 4, Parameters);
}

// Join two gamma tables in floating point format. Result should be a straight line
static cmsToneCurve * CombineGammaFloat(cmsToneCurve * g1, cmsToneCurve * g2)
{
	uint16 Tab[256];
	float f;
	int32 i;
	for(i = 0; i < 256; i++) {
		f = (float)i / 255.0F;
		f = cmsEvalToneCurveFloat(g2, cmsEvalToneCurveFloat(g1, f));
		Tab[i] = (uint16)floor(f * 65535.0 + 0.5);
	}
	return cmsBuildTabulatedToneCurve16(DbgThread(), 256, Tab);
}

// Same of anterior, but using quantized tables
static cmsToneCurve * CombineGamma16(cmsToneCurve * g1, cmsToneCurve * g2)
{
	uint16 Tab[256];
	for(int32 i = 0; i < 256; i++) {
		uint16 wValIn = _cmsQuantizeVal(i, 256);
		Tab[i] = cmsEvalToneCurve16(g2, cmsEvalToneCurve16(g1, wValIn));
	}
	return cmsBuildTabulatedToneCurve16(DbgThread(), 256, Tab);
}

static int32 CheckJointFloatCurves_sRGB()
{
	boolint rc;
	cmsToneCurve * Forward = Build_sRGBGamma();
	cmsToneCurve * Reverse = cmsReverseToneCurve(Forward);
	cmsToneCurve * Result = CombineGammaFloat(Forward, Reverse);
	cmsFreeToneCurve(Forward); 
	cmsFreeToneCurve(Reverse);
	rc = cmsIsToneCurveLinear(Result);
	cmsFreeToneCurve(Result);
	return rc;
}

static int32 CheckJoint16Curves_sRGB()
{
	boolint rc;
	cmsToneCurve * Forward = Build_sRGBGamma();
	cmsToneCurve * Reverse = cmsReverseToneCurve(Forward);
	cmsToneCurve * Result = CombineGamma16(Forward, Reverse);
	cmsFreeToneCurve(Forward); 
	cmsFreeToneCurve(Reverse);
	rc = cmsIsToneCurveLinear(Result);
	cmsFreeToneCurve(Result);
	return rc;
}

// sigmoidal curve f(x) = (1-x^g) ^(1/g)

static int32 CheckJointCurvesSShaped()
{
	double p = 3.2;
	int32 rc;
	cmsToneCurve * Forward = cmsBuildParametricToneCurve(DbgThread(), 108, &p);
	cmsToneCurve * Reverse = cmsReverseToneCurve(Forward);
	cmsToneCurve * Result = cmsJoinToneCurve(DbgThread(), Forward, Forward, 4096);
	cmsFreeToneCurve(Forward);
	cmsFreeToneCurve(Reverse);
	rc = cmsIsToneCurveLinear(Result);
	cmsFreeToneCurve(Result);
	return rc;
}

// --------------------------------------------------------------------------------------------------------

// Implementation of some tone curve functions
static float Gamma(float x, const double Params[])
{
	return (float)pow(x, Params[0]);
}

static float CIE122(float x, const double Params[])

{
	double e, Val;
	if(x >= -Params[2] / Params[1]) {
		e = Params[1]*x + Params[2];
		if(e > 0)
			Val = pow(e, Params[0]);
		else
			Val = 0;
	}
	else
		Val = 0;
	return (float)Val;
}

static float IEC61966_3(float x, const double Params[])
{
	double e, Val;
	if(x >= -Params[2] / Params[1]) {
		e = Params[1]*x + Params[2];
		if(e > 0)
			Val = pow(e, Params[0]) + Params[3];
		else
			Val = 0;
	}
	else
		Val = Params[3];
	return (float)Val;
}

static float IEC61966_21(float x, const double Params[])
{
	double e, Val;
	if(x >= Params[4]) {
		e = Params[1]*x + Params[2];
		if(e > 0)
			Val = pow(e, Params[0]);
		else
			Val = 0;
	}
	else
		Val = x * Params[3];

	return (float)Val;
}

static float param_5(float x, const double Params[])
{
	double e, Val;
	// Y = (aX + b)^Gamma + e | X >= d
	// Y = cX + f             | else
	if(x >= Params[4]) {
		e = Params[1]*x + Params[2];
		if(e > 0)
			Val = pow(e, Params[0]) + Params[5];
		else
			Val = 0;
	}
	else
		Val = x*Params[3] + Params[6];

	return (float)Val;
}

static float param_6(float x, const double Params[])
{
	double Val;
	double e = Params[1]*x + Params[2];
	if(e > 0)
		Val = pow(e, Params[0]) + Params[3];
	else
		Val = 0;
	return (float)Val;
}

static float param_7(float x, const double Params[])
{
	double Val = Params[1]*log10(Params[2] * pow(x, Params[0]) + Params[3]) + Params[4];
	return (float)Val;
}

static float param_8(float x, const double Params[])
{
	double Val = (Params[0] * pow(Params[1], Params[2] * x + Params[3]) + Params[4]);
	return (float)Val;
}

static float sigmoidal(float x, const double Params[])
{
	double Val = pow(1.0 - pow(1 - x, 1/Params[0]), 1/Params[0]);
	return (float)Val;
}

static boolint CheckSingleParametric(const char * Name, dblfnptr fn, int32 Type, const double Params[])
{
	int32 i;
	char InverseText[256];
	cmsToneCurve * tc = cmsBuildParametricToneCurve(DbgThread(), Type, Params);
	cmsToneCurve * tc_1 = cmsBuildParametricToneCurve(DbgThread(), -Type, Params);
	for(i = 0; i <= 1000; i++) {
		float x = (float)i / 1000;
		float y_fn, y_param, x_param, y_param2;
		y_fn = fn(x, Params);
		y_param = cmsEvalToneCurveFloat(tc, x);
		x_param = cmsEvalToneCurveFloat(tc_1, y_param);
		y_param2 = fn(x_param, Params);
		if(!IsGoodVal(Name, y_fn, y_param, FIXED_PRECISION_15_16))
			goto Error;
		sprintf(InverseText, "Inverse %s", Name);
		if(!IsGoodVal(InverseText, y_fn, y_param2, FIXED_PRECISION_15_16))
			goto Error;
	}
	cmsFreeToneCurve(tc);
	cmsFreeToneCurve(tc_1);
	return TRUE;
Error:
	cmsFreeToneCurve(tc);
	cmsFreeToneCurve(tc_1);
	return FALSE;
}

// Check against some known values
static int32 CheckParametricToneCurves()
{
	double Params[10];
	// 1) X = Y ^ Gamma
	Params[0] = 2.2;
	if(!CheckSingleParametric("Gamma", Gamma, 1, Params)) return 0;

	// 2) CIE 122-1966
	// Y = (aX + b)^Gamma  | X >= -b/a
	// Y = 0               | else

	Params[0] = 2.2;
	Params[1] = 1.5;
	Params[2] = -0.5;

	if(!CheckSingleParametric("CIE122-1966", CIE122, 2, Params)) return 0;

	// 3) IEC 61966-3
	// Y = (aX + b)^Gamma | X <= -b/a
	// Y = c              | else

	Params[0] = 2.2;
	Params[1] = 1.5;
	Params[2] = -0.5;
	Params[3] = 0.3;

	if(!CheckSingleParametric("IEC 61966-3", IEC61966_3, 3, Params)) return 0;

	// 4) IEC 61966-2.1 (sRGB)
	// Y = (aX + b)^Gamma | X >= d
	// Y = cX             | X < d

	Params[0] = 2.4;
	Params[1] = 1. / 1.055;
	Params[2] = 0.055 / 1.055;
	Params[3] = 1. / 12.92;
	Params[4] = 0.04045;
	if(!CheckSingleParametric("IEC 61966-2.1", IEC61966_21, 4, Params)) 
		return 0;
	// 5) Y = (aX + b)^Gamma + e | X >= d
	// Y = cX + f             | else
	Params[0] = 2.2;
	Params[1] = 0.7;
	Params[2] = 0.2;
	Params[3] = 0.3;
	Params[4] = 0.1;
	Params[5] = 0.5;
	Params[6] = 0.2;
	if(!CheckSingleParametric("param_5", param_5, 5, Params)) 
		return 0;
	// 6) Y = (aX + b) ^ Gamma + c
	Params[0] = 2.2;
	Params[1] = 0.7;
	Params[2] = 0.2;
	Params[3] = 0.3;
	if(!CheckSingleParametric("param_6", param_6, 6, Params)) 
		return 0;
	// 7) Y = a * log (b * X^Gamma + c) + d
	Params[0] = 2.2;
	Params[1] = 0.9;
	Params[2] = 0.9;
	Params[3] = 0.02;
	Params[4] = 0.1;
	if(!CheckSingleParametric("param_7", param_7, 7, Params)) 
		return 0;
	// 8) Y = a * b ^ (c*X+d) + e
	Params[0] = 0.9;
	Params[1] = 0.9;
	Params[2] = 1.02;
	Params[3] = 0.1;
	Params[4] = 0.2;
	if(!CheckSingleParametric("param_8", param_8, 8, Params)) 
		return 0;
	// 108: S-Shaped: (1 - (1-x)^1/g)^1/g
	Params[0] = 1.9;
	if(!CheckSingleParametric("sigmoidal", sigmoidal, 108, Params)) 
		return 0;
	// All OK
	return 1;
}
//
// LUT checks
//
static int32 CheckLUTcreation()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 1, 1);
	int32 n1 = cmsPipelineStageCount(lut);
	cmsPipeline * lut2 = cmsPipelineDup(lut);
	int32 n2 = cmsPipelineStageCount(lut2);
	cmsPipelineFree(lut);
	cmsPipelineFree(lut2);
	return (n1 == 0) && (n2 == 0);
}

// Create a MPE for a identity matrix
static void AddIdentityMatrix(cmsPipeline * lut)
{
	const double Identity[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
	cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocMatrix(DbgThread(), 3, 3, Identity, NULL));
}

// Create a MPE for identity float CLUT
static void AddIdentityCLUTfloat(cmsPipeline * lut)
{
	const float Table[] = {
		0,    0,    0,
		0,    0,    1.0,

		0,    1.0,    0,
		0,    1.0,    1.0,

		1.0,    0,    0,
		1.0,    0,    1.0,

		1.0,    1.0,    0,
		1.0,    1.0,    1.0
	};

	cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocCLutFloat(DbgThread(), 2, 3, 3, Table));
}

// Create a MPE for identity float CLUT
static void AddIdentityCLUT16(cmsPipeline * lut)
{
	const uint16 Table[] = {
		0,    0,    0,
		0,    0,    0xffff,

		0,    0xffff,    0,
		0,    0xffff,    0xffff,

		0xffff,    0,    0,
		0xffff,    0,    0xffff,

		0xffff,    0xffff,    0,
		0xffff,    0xffff,    0xffff
	};
	cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocCLut16bit(DbgThread(), 2, 3, 3, Table));
}

// Create a 3 fn identity curves

static void Add3GammaCurves(cmsPipeline * lut, double Curve)
{
	cmsToneCurve * id = cmsBuildGamma(DbgThread(), Curve);
	cmsToneCurve * id3[3];
	id3[0] = id;
	id3[1] = id;
	id3[2] = id;
	cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocToneCurves(DbgThread(), 3, id3));
	cmsFreeToneCurve(id);
}

static int32 CheckFloatLUT(cmsPipeline * lut)
{
	int32 i, j;
	float Inf[3], Outf[3];
	int32 n1 = 0;
	for(j = 0; j < 65535; j++) {
		int32 af[3];
		Inf[0] = Inf[1] = Inf[2] = (float)j / 65535.0F;
		cmsPipelineEvalFloat(Inf, Outf, lut);
		af[0] = (int32)floor(Outf[0]*65535.0 + 0.5);
		af[1] = (int32)floor(Outf[1]*65535.0 + 0.5);
		af[2] = (int32)floor(Outf[2]*65535.0 + 0.5);
		for(i = 0; i < 3; i++) {
			if(af[i] != j) {
				n1++;
			}
		}
	}
	return (n1 == 0);
}

static int32 Check16LUT(cmsPipeline * lut)
{
	int32 i, j;
	uint16 Inw[3], Outw[3];
	int32 n2 = 0;
	for(j = 0; j < 65535; j++) {
		int32 aw[3];
		Inw[0] = Inw[1] = Inw[2] = (uint16)j;
		cmsPipelineEval16(Inw, Outw, lut);
		aw[0] = Outw[0];
		aw[1] = Outw[1];
		aw[2] = Outw[2];
		for(i = 0; i < 3; i++) {
			if(aw[i] != j) {
				n2++;
			}
		}
	}
	return (n2 == 0);
}

// Check any LUT that is linear
static int32 CheckStagesLUT(cmsPipeline * lut, int32 ExpectedStages)
{
	int32 nInpChans  = cmsPipelineInputChannels(lut);
	int32 nOutpChans = cmsPipelineOutputChannels(lut);
	int32 nStages    = cmsPipelineStageCount(lut);
	return (nInpChans == 3) && (nOutpChans == 3) && (nStages == ExpectedStages);
}

static int32 CheckFullLUT(cmsPipeline * lut, int32 ExpectedStages)
{
	int32 rc = CheckStagesLUT(lut, ExpectedStages) && Check16LUT(lut) && CheckFloatLUT(lut);
	cmsPipelineFree(lut);
	return rc;
}

static int32 Check1StageLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	return CheckFullLUT(lut, 1);
}

static int32 Check2StageLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUTfloat(lut);
	return CheckFullLUT(lut, 2);
}

static int32 Check2Stage16LUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUT16(lut);
	return CheckFullLUT(lut, 2);
}

static int32 Check3StageLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUTfloat(lut);
	Add3GammaCurves(lut, 1.0);
	return CheckFullLUT(lut, 3);
}

static int32 Check3Stage16LUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUT16(lut);
	Add3GammaCurves(lut, 1.0);
	return CheckFullLUT(lut, 3);
}

static int32 Check4StageLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUTfloat(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityMatrix(lut);
	return CheckFullLUT(lut, 4);
}

static int32 Check4Stage16LUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUT16(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityMatrix(lut);
	return CheckFullLUT(lut, 4);
}

static int32 Check5StageLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUTfloat(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityMatrix(lut);
	Add3GammaCurves(lut, 1.0);
	return CheckFullLUT(lut, 5);
}

static int32 Check5Stage16LUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	AddIdentityCLUT16(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityMatrix(lut);
	Add3GammaCurves(lut, 1.0);
	return CheckFullLUT(lut, 5);
}

static int32 Check6StageLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityCLUTfloat(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityMatrix(lut);
	Add3GammaCurves(lut, 1.0);
	return CheckFullLUT(lut, 6);
}

static int32 Check6Stage16LUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	AddIdentityMatrix(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityCLUT16(lut);
	Add3GammaCurves(lut, 1.0);
	AddIdentityMatrix(lut);
	Add3GammaCurves(lut, 1.0);
	return CheckFullLUT(lut, 6);
}

static int32 CheckLab2LabLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	int32 rc;
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocLab2XYZ(DbgThread()));
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocXYZ2Lab(DbgThread()));
	rc = CheckFloatLUT(lut) && CheckStagesLUT(lut, 2);
	cmsPipelineFree(lut);
	return rc;
}

static int32 CheckXYZ2XYZLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	int32 rc;
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocXYZ2Lab(DbgThread()));
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocLab2XYZ(DbgThread()));
	rc = CheckFloatLUT(lut) && CheckStagesLUT(lut, 2);
	cmsPipelineFree(lut);
	return rc;
}

static int32 CheckLab2LabMatLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	int32 rc;
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocLab2XYZ(DbgThread()));
	AddIdentityMatrix(lut);
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocXYZ2Lab(DbgThread()));
	rc = CheckFloatLUT(lut) && CheckStagesLUT(lut, 3);
	cmsPipelineFree(lut);
	return rc;
}

static int32 CheckNamedColorLUT()
{
	cmsPipeline * lut = cmsPipelineAlloc(DbgThread(), 3, 3);
	int32 i, j, rc = 1, n2;
	uint16 PCS[3];
	uint16 Colorant[cmsMAXCHANNELS];
	char Name[255];
	uint16 Inw[3], Outw[3];
	cmsNAMEDCOLORLIST * nc = cmsAllocNamedColorList(DbgThread(), 256, 3, "pre", "post");
	if(nc == NULL) 
		return 0;
	for(i = 0; i < 256; i++) {
		PCS[0] = PCS[1] = PCS[2] = (uint16)i;
		Colorant[0] = Colorant[1] = Colorant[2] = Colorant[3] = (uint16)i;
		sprintf(Name, "#%d", i);
		if(!cmsAppendNamedColor(nc, Name, PCS, Colorant)) {
			rc = 0; break;
		}
	}
	cmsPipelineInsertStage(lut, cmsAT_END, _cmsStageAllocNamedColor(nc, FALSE));
	cmsFreeNamedColorList(nc);
	if(rc == 0) 
		return 0;
	n2 = 0;
	for(j = 0; j < 256; j++) {
		Inw[0] = (uint16)j;
		cmsPipelineEval16(Inw, Outw, lut);
		for(i = 0; i < 3; i++) {
			if(Outw[i] != j) {
				n2++;
			}
		}
	}
	cmsPipelineFree(lut);
	return (n2 == 0);
}

// A lightweight test of multilocalized unicode structures.
static int32 CheckMLU()
{
	cmsMLU * mlu2, * mlu3;
	char Buffer[256], Buffer2[256];
	int32 rc = 1;
	int32 i;
	cmsHPROFILE h = NULL;
	// Allocate a MLU structure, no preferred size
	cmsMLU * mlu = cmsMLUalloc(DbgThread(), 0);
	// Add some localizations
	cmsMLUsetWide(mlu, "en", "US", L"Hello, world");
	cmsMLUsetWide(mlu, "es", "ES", L"Hola, mundo");
	cmsMLUsetWide(mlu, "fr", "FR", L"Bonjour, le monde");
	cmsMLUsetWide(mlu, "ca", "CA", L"Hola, mon");
	// Check the returned string for each language
	cmsMLUgetASCII(mlu, "en", "US", Buffer, 256);
	if(!sstreq(Buffer, "Hello, world")) 
		rc = 0;
	cmsMLUgetASCII(mlu, "es", "ES", Buffer, 256);
	if(!sstreq(Buffer, "Hola, mundo")) 
		rc = 0;
	cmsMLUgetASCII(mlu, "fr", "FR", Buffer, 256);
	if(!sstreq(Buffer, "Bonjour, le monde"))
		rc = 0;
	cmsMLUgetASCII(mlu, "ca", "CA", Buffer, 256);
	if(!sstreq(Buffer, "Hola, mon"))
		rc = 0;
	if(rc == 0)
		Fail("Unexpected string '%s'", Buffer);
	// So far, so good.
	cmsMLUfree(mlu);
	// Now for performance, allocate an empty struct
	mlu = cmsMLUalloc(DbgThread(), 0);
	// Fill it with several thousands of different lenguages
	for(i = 0; i < 4096; i++) {
		char Lang[3];
		Lang[0] = (char)(i % 255);
		Lang[1] = (char)(i / 255);
		Lang[2] = 0;
		sprintf(Buffer, "String #%i", i);
		cmsMLUsetASCII(mlu, Lang, Lang, Buffer);
	}
	// Duplicate it
	mlu2 = cmsMLUdup(mlu);
	// Get rid of original
	cmsMLUfree(mlu);
	// Check all is still in place
	for(i = 0; i < 4096; i++) {
		char Lang[3];
		Lang[0] = (char)(i % 255);
		Lang[1] = (char)(i / 255);
		Lang[2] = 0;
		cmsMLUgetASCII(mlu2, Lang, Lang, Buffer2, 256);
		sprintf(Buffer, "String #%i", i);
		if(!sstreq(Buffer, Buffer2)) {
			rc = 0; 
			break;
		}
	}
	if(rc == 0)
		Fail("Unexpected string '%s'", Buffer2);
	// Check profile IO
	h = cmsOpenProfileFromFileTHR(DbgThread(), "mlucheck.icc", "w");
	cmsSetProfileVersion(h, 4.3);
	cmsWriteTag(h, cmsSigProfileDescriptionTag, mlu2);
	cmsCloseProfile(h);
	cmsMLUfree(mlu2);
	h = cmsOpenProfileFromFileTHR(DbgThread(), "mlucheck.icc", "r");
	mlu3 = (cmsMLU*)cmsReadTag(h, cmsSigProfileDescriptionTag);
	if(mlu3 == NULL) {
		Fail("Profile didn't get the MLU\n"); rc = 0; goto Error;
	}
	// Check all is still in place
	for(i = 0; i < 4096; i++) {
		char Lang[3];
		Lang[0] = (char)(i % 255);
		Lang[1] = (char)(i / 255);
		Lang[2] = 0;
		cmsMLUgetASCII(mlu3, Lang, Lang, Buffer2, 256);
		sprintf(Buffer, "String #%i", i);
		if(!sstreq(Buffer, Buffer2)) {
			rc = 0; break;
		}
	}
	if(rc == 0) 
		Fail("Unexpected string '%s'", Buffer2);
Error:
	cmsCloseProfile(h);
	remove("mlucheck.icc");
	return rc;
}

// A lightweight test of named color structures.
static int32 CheckNamedColorList()
{
	cmsNAMEDCOLORLIST * nc2;
	int32 i, j, rc = 1;
	char Name[cmsMAX_PATH];
	uint16 PCS[3];
	uint16 Colorant[cmsMAXCHANNELS];
	char CheckName[cmsMAX_PATH];
	uint16 CheckPCS[3];
	uint16 CheckColorant[cmsMAXCHANNELS];
	cmsHPROFILE h;
	cmsNAMEDCOLORLIST * nc = cmsAllocNamedColorList(DbgThread(), 0, 4, "prefix", "suffix");
	if(nc == NULL) 
		return 0;
	for(i = 0; i < 4096; i++) {
		PCS[0] = PCS[1] = PCS[2] = (uint16)i;
		Colorant[0] = Colorant[1] = Colorant[2] = Colorant[3] = (uint16)(4096 - i);
		sprintf(Name, "#%d", i);
		if(!cmsAppendNamedColor(nc, Name, PCS, Colorant)) {
			rc = 0; break;
		}
	}
	for(i = 0; i < 4096; i++) {
		CheckPCS[0] = CheckPCS[1] = CheckPCS[2] = (uint16)i;
		CheckColorant[0] = CheckColorant[1] = CheckColorant[2] = CheckColorant[3] = (uint16)(4096 - i);
		sprintf(CheckName, "#%d", i);
		if(!cmsNamedColorInfo(nc, i, Name, NULL, NULL, PCS, Colorant)) {
			rc = 0; 
			goto Error;
		}
		for(j = 0; j < 3; j++) {
			if(CheckPCS[j] != PCS[j]) {
				rc = 0; 
				Fail("Invalid PCS"); 
				goto Error;
			}
		}
		for(j = 0; j < 4; j++) {
			if(CheckColorant[j] != Colorant[j]) {
				rc = 0; 
				Fail("Invalid Colorant"); 
				goto Error;
			}
		}
		if(!sstreq(Name, CheckName)) {
			rc = 0; 
			Fail("Invalid Name"); 
			goto Error;
		}
	}
	h = cmsOpenProfileFromFileTHR(DbgThread(), "namedcol.icc", "w");
	if(!h) 
		return 0;
	if(!cmsWriteTag(h, cmsSigNamedColor2Tag, nc)) 
		return 0;
	cmsCloseProfile(h);
	cmsFreeNamedColorList(nc);
	nc = NULL;
	h = cmsOpenProfileFromFileTHR(DbgThread(), "namedcol.icc", "r");
	nc2 = (cmsNAMEDCOLORLIST*)cmsReadTag(h, cmsSigNamedColor2Tag);
	if(cmsNamedColorCount(nc2) != 4096) {
		rc = 0; Fail("Invalid count"); goto Error;
	}
	i = cmsNamedColorIndex(nc2, "#123");
	if(i != 123) {
		rc = 0; Fail("Invalid index"); goto Error;
	}
	for(i = 0; i < 4096; i++) {
		CheckPCS[0] = CheckPCS[1] = CheckPCS[2] = (uint16)i;
		CheckColorant[0] = CheckColorant[1] = CheckColorant[2] = CheckColorant[3] = (uint16)(4096 - i);
		sprintf(CheckName, "#%d", i);
		if(!cmsNamedColorInfo(nc2, i, Name, NULL, NULL, PCS, Colorant)) {
			rc = 0; 
			goto Error;
		}
		for(j = 0; j < 3; j++) {
			if(CheckPCS[j] != PCS[j]) {
				rc = 0; 
				Fail("Invalid PCS"); 
				goto Error;
			}
		}
		for(j = 0; j < 4; j++) {
			if(CheckColorant[j] != Colorant[j]) {
				rc = 0; 
				Fail("Invalid Colorant"); 
				goto Error;
			}
		}
		if(!sstreq(Name, CheckName)) {
			rc = 0; 
			Fail("Invalid Name"); 
			goto Error;
		}
	}
	cmsCloseProfile(h);
	remove("namedcol.icc");
Error:
	cmsFreeNamedColorList(nc);
	return rc;
}

// Formatters

static boolint FormatterFailed;

static void CheckSingleFormatter16(cmsContext id, uint32 Type, const char * Text)
{
	uint16 Values[cmsMAXCHANNELS];
	uint8 Buffer[1024];
	cmsFormatter f, b;
	int32 i, j, nChannels, bytes;
	_cmsTRANSFORM info;
	// Already failed?
	if(FormatterFailed) return;
	memzero(&info, sizeof(info));
	info.OutputFormat = info.InputFormat = Type;
	// Go forth and back
	f = _cmsGetFormatter(id, Type,  cmsFormatterInput, CMS_PACK_FLAGS_16BITS);
	b = _cmsGetFormatter(id, Type,  cmsFormatterOutput, CMS_PACK_FLAGS_16BITS);
	if(f.Fmt16 == NULL || b.Fmt16 == NULL) {
		Fail("no formatter for %s", Text);
		FormatterFailed = TRUE;
		// Useful for debug
		f = _cmsGetFormatter(id, Type,  cmsFormatterInput, CMS_PACK_FLAGS_16BITS);
		b = _cmsGetFormatter(id, Type,  cmsFormatterOutput, CMS_PACK_FLAGS_16BITS);
		return;
	}
	nChannels = T_CHANNELS(Type);
	bytes     = T_BYTES(Type);
	for(j = 0; j < 5; j++) {
		for(i = 0; i < nChannels; i++) {
			Values[i] = (uint16)(i+j);
			// For 8-bit
			if(bytes == 1)
				Values[i] <<= 8;
		}
		b.Fmt16(&info, Values, Buffer, 2);
		memzero(Values, sizeof(Values));
		f.Fmt16(&info, Values, Buffer, 2);
		for(i = 0; i < nChannels; i++) {
			if(bytes == 1)
				Values[i] >>= 8;
			if(Values[i] != i+j) {
				Fail("%s failed", Text);
				FormatterFailed = TRUE;

				// Useful for debug
				for(i = 0; i < nChannels; i++) {
					Values[i] = (uint16)(i+j);
					// For 8-bit
					if(bytes == 1)
						Values[i] <<= 8;
				}

				b.Fmt16(&info, Values, Buffer, 1);
				f.Fmt16(&info, Values, Buffer, 1);
				return;
			}
		}
	}
}

#define C(a) CheckSingleFormatter16(0, a, #a)

// Check all formatters
static int32 CheckFormatters16()
{
	FormatterFailed = FALSE;

	C(TYPE_GRAY_8);
	C(TYPE_GRAY_8_REV);
	C(TYPE_GRAY_16);
	C(TYPE_GRAY_16_REV);
	C(TYPE_GRAY_16_SE);
	C(TYPE_GRAYA_8);
	C(TYPE_GRAYA_16);
	C(TYPE_GRAYA_16_SE);
	C(TYPE_GRAYA_8_PLANAR);
	C(TYPE_GRAYA_16_PLANAR);
	C(TYPE_RGB_8);
	C(TYPE_RGB_8_PLANAR);
	C(TYPE_BGR_8);
	C(TYPE_BGR_8_PLANAR);
	C(TYPE_RGB_16);
	C(TYPE_RGB_16_PLANAR);
	C(TYPE_RGB_16_SE);
	C(TYPE_BGR_16);
	C(TYPE_BGR_16_PLANAR);
	C(TYPE_BGR_16_SE);
	C(TYPE_RGBA_8);
	C(TYPE_RGBA_8_PLANAR);
	C(TYPE_RGBA_16);
	C(TYPE_RGBA_16_PLANAR);
	C(TYPE_RGBA_16_SE);
	C(TYPE_ARGB_8);
	C(TYPE_ARGB_8_PLANAR);
	C(TYPE_ARGB_16);
	C(TYPE_ABGR_8);
	C(TYPE_ABGR_8_PLANAR);
	C(TYPE_ABGR_16);
	C(TYPE_ABGR_16_PLANAR);
	C(TYPE_ABGR_16_SE);
	C(TYPE_BGRA_8);
	C(TYPE_BGRA_8_PLANAR);
	C(TYPE_BGRA_16);
	C(TYPE_BGRA_16_SE);
	C(TYPE_CMY_8);
	C(TYPE_CMY_8_PLANAR);
	C(TYPE_CMY_16);
	C(TYPE_CMY_16_PLANAR);
	C(TYPE_CMY_16_SE);
	C(TYPE_CMYK_8);
	C(TYPE_CMYKA_8);
	C(TYPE_CMYK_8_REV);
	C(TYPE_YUVK_8);
	C(TYPE_CMYK_8_PLANAR);
	C(TYPE_CMYK_16);
	C(TYPE_CMYK_16_REV);
	C(TYPE_YUVK_16);
	C(TYPE_CMYK_16_PLANAR);
	C(TYPE_CMYK_16_SE);
	C(TYPE_KYMC_8);
	C(TYPE_KYMC_16);
	C(TYPE_KYMC_16_SE);
	C(TYPE_KCMY_8);
	C(TYPE_KCMY_8_REV);
	C(TYPE_KCMY_16);
	C(TYPE_KCMY_16_REV);
	C(TYPE_KCMY_16_SE);
	C(TYPE_CMYK5_8);
	C(TYPE_CMYK5_16);
	C(TYPE_CMYK5_16_SE);
	C(TYPE_KYMC5_8);
	C(TYPE_KYMC5_16);
	C(TYPE_KYMC5_16_SE);
	C(TYPE_CMYK6_8);
	C(TYPE_CMYK6_8_PLANAR);
	C(TYPE_CMYK6_16);
	C(TYPE_CMYK6_16_PLANAR);
	C(TYPE_CMYK6_16_SE);
	C(TYPE_CMYK7_8);
	C(TYPE_CMYK7_16);
	C(TYPE_CMYK7_16_SE);
	C(TYPE_KYMC7_8);
	C(TYPE_KYMC7_16);
	C(TYPE_KYMC7_16_SE);
	C(TYPE_CMYK8_8);
	C(TYPE_CMYK8_16);
	C(TYPE_CMYK8_16_SE);
	C(TYPE_KYMC8_8);
	C(TYPE_KYMC8_16);
	C(TYPE_KYMC8_16_SE);
	C(TYPE_CMYK9_8);
	C(TYPE_CMYK9_16);
	C(TYPE_CMYK9_16_SE);
	C(TYPE_KYMC9_8);
	C(TYPE_KYMC9_16);
	C(TYPE_KYMC9_16_SE);
	C(TYPE_CMYK10_8);
	C(TYPE_CMYK10_16);
	C(TYPE_CMYK10_16_SE);
	C(TYPE_KYMC10_8);
	C(TYPE_KYMC10_16);
	C(TYPE_KYMC10_16_SE);
	C(TYPE_CMYK11_8);
	C(TYPE_CMYK11_16);
	C(TYPE_CMYK11_16_SE);
	C(TYPE_KYMC11_8);
	C(TYPE_KYMC11_16);
	C(TYPE_KYMC11_16_SE);
	C(TYPE_CMYK12_8);
	C(TYPE_CMYK12_16);
	C(TYPE_CMYK12_16_SE);
	C(TYPE_KYMC12_8);
	C(TYPE_KYMC12_16);
	C(TYPE_KYMC12_16_SE);
	C(TYPE_XYZ_16);
	C(TYPE_Lab_8);
	C(TYPE_ALab_8);
	C(TYPE_Lab_16);
	C(TYPE_Yxy_16);
	C(TYPE_YCbCr_8);
	C(TYPE_YCbCr_8_PLANAR);
	C(TYPE_YCbCr_16);
	C(TYPE_YCbCr_16_PLANAR);
	C(TYPE_YCbCr_16_SE);
	C(TYPE_YUV_8);
	C(TYPE_YUV_8_PLANAR);
	C(TYPE_YUV_16);
	C(TYPE_YUV_16_PLANAR);
	C(TYPE_YUV_16_SE);
	C(TYPE_HLS_8);
	C(TYPE_HLS_8_PLANAR);
	C(TYPE_HLS_16);
	C(TYPE_HLS_16_PLANAR);
	C(TYPE_HLS_16_SE);
	C(TYPE_HSV_8);
	C(TYPE_HSV_8_PLANAR);
	C(TYPE_HSV_16);
	C(TYPE_HSV_16_PLANAR);
	C(TYPE_HSV_16_SE);

	C(TYPE_XYZ_FLT);
	C(TYPE_Lab_FLT);
	C(TYPE_GRAY_FLT);
	C(TYPE_RGB_FLT);
	C(TYPE_BGR_FLT);
	C(TYPE_CMYK_FLT);
	C(TYPE_LabA_FLT);
	C(TYPE_RGBA_FLT);
	C(TYPE_ARGB_FLT);
	C(TYPE_BGRA_FLT);
	C(TYPE_ABGR_FLT);

	C(TYPE_XYZ_DBL);
	C(TYPE_Lab_DBL);
	C(TYPE_GRAY_DBL);
	C(TYPE_RGB_DBL);
	C(TYPE_BGR_DBL);
	C(TYPE_CMYK_DBL);

	C(TYPE_LabV2_8);
	C(TYPE_ALabV2_8);
	C(TYPE_LabV2_16);

#ifndef CMS_NO_HALF_SUPPORT

	C(TYPE_GRAY_HALF_FLT);
	C(TYPE_RGB_HALF_FLT);
	C(TYPE_CMYK_HALF_FLT);
	C(TYPE_RGBA_HALF_FLT);

	C(TYPE_RGBA_HALF_FLT);
	C(TYPE_ARGB_HALF_FLT);
	C(TYPE_BGR_HALF_FLT);
	C(TYPE_BGRA_HALF_FLT);
	C(TYPE_ABGR_HALF_FLT);

#endif

	return FormatterFailed == 0 ? 1 : 0;
}

#undef C

static void CheckSingleFormatterFloat(uint32 Type, const char * Text)
{
	float Values[cmsMAXCHANNELS];
	uint8 Buffer[1024];
	cmsFormatter f, b;
	int32 i, j, nChannels;
	_cmsTRANSFORM info;

	// Already failed?
	if(FormatterFailed) return;
	memzero(&info, sizeof(info));
	info.OutputFormat = info.InputFormat = Type;
	// Go forth and back
	f = _cmsGetFormatter(0, Type,  cmsFormatterInput, CMS_PACK_FLAGS_FLOAT);
	b = _cmsGetFormatter(0, Type,  cmsFormatterOutput, CMS_PACK_FLAGS_FLOAT);
	if(f.FmtFloat == NULL || b.FmtFloat == NULL) {
		Fail("no formatter for %s", Text);
		FormatterFailed = TRUE;
		// Useful for debug
		f = _cmsGetFormatter(0, Type,  cmsFormatterInput, CMS_PACK_FLAGS_FLOAT);
		b = _cmsGetFormatter(0, Type,  cmsFormatterOutput, CMS_PACK_FLAGS_FLOAT);
		return;
	}
	nChannels = T_CHANNELS(Type);

	for(j = 0; j < 5; j++) {
		for(i = 0; i < nChannels; i++) {
			Values[i] = (float)(i+j);
		}
		b.FmtFloat(&info, Values, Buffer, 1);
		memzero(Values, sizeof(Values));
		f.FmtFloat(&info, Values, Buffer, 1);
		for(i = 0; i < nChannels; i++) {
			double delta = fabs(Values[i] - ( i+j));
			if(delta > 0.000000001) {
				Fail("%s failed", Text);
				FormatterFailed = TRUE;
				// Useful for debug
				for(i = 0; i < nChannels; i++) {
					Values[i] = (float)(i+j);
				}
				b.FmtFloat(&info, Values, Buffer, 1);
				f.FmtFloat(&info, Values, Buffer, 1);
				return;
			}
		}
	}
}

#define C(a) CheckSingleFormatterFloat(a, #a)

static int32 CheckFormattersFloat()
{
	FormatterFailed = FALSE;
	C(TYPE_XYZ_FLT);
	C(TYPE_Lab_FLT);
	C(TYPE_GRAY_FLT);
	C(TYPE_RGB_FLT);
	C(TYPE_BGR_FLT);
	C(TYPE_CMYK_FLT);

	C(TYPE_LabA_FLT);
	C(TYPE_RGBA_FLT);

	C(TYPE_ARGB_FLT);
	C(TYPE_BGRA_FLT);
	C(TYPE_ABGR_FLT);

	C(TYPE_XYZ_DBL);
	C(TYPE_Lab_DBL);
	C(TYPE_GRAY_DBL);
	C(TYPE_RGB_DBL);
	C(TYPE_BGR_DBL);
	C(TYPE_CMYK_DBL);
	C(TYPE_XYZ_FLT);

#ifndef CMS_NO_HALF_SUPPORT
	C(TYPE_GRAY_HALF_FLT);
	C(TYPE_RGB_HALF_FLT);
	C(TYPE_CMYK_HALF_FLT);
	C(TYPE_RGBA_HALF_FLT);

	C(TYPE_RGBA_HALF_FLT);
	C(TYPE_ARGB_HALF_FLT);
	C(TYPE_BGR_HALF_FLT);
	C(TYPE_BGRA_HALF_FLT);
	C(TYPE_ABGR_HALF_FLT);
#endif

	return FormatterFailed == 0 ? 1 : 0;
}

#undef C

#ifndef CMS_NO_HALF_SUPPORT
// Check half float
#define my_isfinite(x) ((x) != (x))
static int32 CheckFormattersHalf()
{
	for(int i = 0; i < 0xffff; i++) {
		float f = _cmsHalf2Float((uint16)i);
		if(!my_isfinite(f)) {
			int j = _cmsFloat2Half(f);
			if(i != j) {
				Fail("%d != %d in Half float support!\n", i, j);
				return 0;
			}
		}
	}
	return 1;
}
#endif

static int32 CheckOneRGB(cmsHTRANSFORM xform, uint16 R, uint16 G, uint16 B, uint16 Ro, uint16 Go, uint16 Bo)
{
	uint16 RGB[3];
	uint16 Out[3];
	RGB[0] = R;
	RGB[1] = G;
	RGB[2] = B;
	cmsDoTransform(xform, RGB, Out, 1);
	return IsGoodWord("R", Ro, Out[0]) && IsGoodWord("G", Go, Out[1]) && IsGoodWord("B", Bo, Out[2]);
}

// Check known values going from sRGB to XYZ
static int32 CheckOneRGB_double(cmsHTRANSFORM xform, double R, double G, double B, double Ro, double Go, double Bo)
{
	double RGB[3];
	double Out[3];
	RGB[0] = R;
	RGB[1] = G;
	RGB[2] = B;
	cmsDoTransform(xform, RGB, Out, 1);
	return IsGoodVal("R", Ro, Out[0], 0.01) && IsGoodVal("G", Go, Out[1], 0.01) && IsGoodVal("B", Bo, Out[2], 0.01);
}

static int32 CheckChangeBufferFormat()
{
	cmsHPROFILE hsRGB = cmsCreate_sRGBProfile();
	cmsHTRANSFORM xform = cmsCreateTransform(hsRGB, TYPE_RGB_16, hsRGB, TYPE_RGB_16, INTENT_PERCEPTUAL, 0);
	cmsCloseProfile(hsRGB);
	if(!xform) 
		return 0;
	if(!CheckOneRGB(xform, 0, 0, 0, 0, 0, 0)) return 0;
	if(!CheckOneRGB(xform, 120, 0, 0, 120, 0, 0)) return 0;
	if(!CheckOneRGB(xform, 0, 222, 255, 0, 222, 255)) return 0;
	if(!cmsChangeBuffersFormat(xform, TYPE_BGR_16, TYPE_RGB_16)) return 0;
	if(!CheckOneRGB(xform, 0, 0, 123, 123, 0, 0)) return 0;
	if(!CheckOneRGB(xform, 154, 234, 0, 0, 234, 154)) return 0;
	if(!cmsChangeBuffersFormat(xform, TYPE_RGB_DBL, TYPE_RGB_DBL)) return 0;
	if(!CheckOneRGB_double(xform, 0.20, 0, 0, 0.20, 0, 0)) return 0;
	if(!CheckOneRGB_double(xform, 0, 0.9, 1, 0, 0.9, 1)) return 0;
	cmsDeleteTransform(xform);
	return 1;
}

// Write tag testbed ----------------------------------------------------------------------------------------

static int32 CheckXYZ(int32 Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsCIEXYZ XYZ, * Pt;
	switch(Pass) {
		case 1:
		    XYZ.X = 1.0; 
			XYZ.Y = 1.1; 
			XYZ.Z = 1.2;
		    return cmsWriteTag(hProfile, tag, &XYZ);
		case 2:
		    Pt = (cmsCIEXYZ*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    return IsGoodFixed15_16("X", 1.0, Pt->X) && IsGoodFixed15_16("Y", 1.1, Pt->Y) && IsGoodFixed15_16("Z", 1.2, Pt->Z);
		default:
		    return 0;
	}
}

static int32 CheckGamma(int32 Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsToneCurve * g, * Pt;
	int32 rc;
	switch(Pass) {
		case 1:
		    g = cmsBuildGamma(DbgThread(), 1.0);
		    rc = cmsWriteTag(hProfile, tag, g);
		    cmsFreeToneCurve(g);
		    return rc;
		case 2:
		    Pt = (cmsToneCurve *)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) return 0;
		    return cmsIsToneCurveLinear(Pt);
		default:
		    return 0;
	}
}

static int32 CheckTextSingle(int32 Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsMLU * m, * Pt;
	int32 rc;
	char Buffer[256];
	switch(Pass) {
		case 1:
		    m = cmsMLUalloc(DbgThread(), 0);
		    cmsMLUsetASCII(m, cmsNoLanguage, cmsNoCountry, "Test test");
		    rc = cmsWriteTag(hProfile, tag, m);
		    cmsMLUfree(m);
		    return rc;
		case 2:
		    Pt = (cmsMLU*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    cmsMLUgetASCII(Pt, cmsNoLanguage, cmsNoCountry, Buffer, 256);
		    if(!sstreq(Buffer, "Test test")) 
				return FALSE;
		    return TRUE;
		default:
		    return 0;
	}
}

static int32 CheckText(int32 Pass, cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsMLU * m, * Pt;
	int32 rc;
	char Buffer[256];
	switch(Pass) {
		case 1:
		    m = cmsMLUalloc(DbgThread(), 0);
		    cmsMLUsetASCII(m, cmsNoLanguage, cmsNoCountry, "Test test");
		    cmsMLUsetASCII(m, "en",  "US",  "1 1 1 1");
		    cmsMLUsetASCII(m, "es",  "ES",  "2 2 2 2");
		    cmsMLUsetASCII(m, "ct",  "ES",  "3 3 3 3");
		    cmsMLUsetASCII(m, "en",  "GB",  "444444444");
		    rc = cmsWriteTag(hProfile, tag, m);
		    cmsMLUfree(m);
		    return rc;
		case 2:
		    Pt = (cmsMLU*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    cmsMLUgetASCII(Pt, cmsNoLanguage, cmsNoCountry, Buffer, 256);
		    if(!sstreq(Buffer, "Test test")) 
				return FALSE;
		    cmsMLUgetASCII(Pt, "en", "US", Buffer, 256);
		    if(!sstreq(Buffer, "1 1 1 1")) 
				return FALSE;
		    cmsMLUgetASCII(Pt, "es", "ES", Buffer, 256);
		    if(!sstreq(Buffer, "2 2 2 2"))
				return FALSE;
		    cmsMLUgetASCII(Pt, "ct", "ES", Buffer, 256);
		    if(!sstreq(Buffer, "3 3 3 3"))
				return FALSE;
		    cmsMLUgetASCII(Pt, "en", "GB",  Buffer, 256);
		    if(!sstreq(Buffer, "444444444"))
				return FALSE;
		    return TRUE;
		default:
		    return 0;
	}
}

static int32 CheckData(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsICCData * Pt;
	cmsICCData d = { 1, 0, { '?' }};
	int32 rc;
	switch(Pass) {
		case 1:
		    rc = cmsWriteTag(hProfile, tag, &d);
		    return rc;
		case 2:
		    Pt = (cmsICCData*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) return 0;
		    return (Pt->data[0] == '?') && (Pt->flag == 0) && (Pt->len == 1);
		default:
		    return 0;
	}
}

static int32 CheckSignature(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsTagSignature * Pt, Holder;
	switch(Pass) {
		case 1:
		    Holder = (cmsTagSignature)cmsSigPerceptualReferenceMediumGamut;
		    return cmsWriteTag(hProfile, tag, &Holder);
		case 2:
		    Pt = (cmsTagSignature*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    return *Pt == cmsSigPerceptualReferenceMediumGamut;
		default:
		    return 0;
	}
}

static int32 CheckDateTime(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	struct tm * Pt, Holder;
	switch(Pass) {
		case 1:
		    Holder.tm_hour = 1;
		    Holder.tm_min = 2;
		    Holder.tm_sec = 3;
		    Holder.tm_mday = 4;
		    Holder.tm_mon = 5;
		    Holder.tm_year = 2009 - 1900;
		    return cmsWriteTag(hProfile, tag, &Holder);
		case 2:
		    Pt = (struct tm *)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    return (Pt->tm_hour == 1 && Pt->tm_min == 2 && Pt->tm_sec == 3 && Pt->tm_mday == 4 && Pt->tm_mon == 5 && Pt->tm_year == 2009 - 1900);
		default:
		    return 0;
	}
}

static int32 CheckNamedColor(int32 Pass, cmsHPROFILE hProfile, cmsTagSignature tag, int32 max_check, boolint colorant_check)
{
	cmsNAMEDCOLORLIST* nc;
	int32 i, j, rc;
	char Name[255];
	uint16 PCS[3];
	uint16 Colorant[cmsMAXCHANNELS];
	char CheckName[255];
	uint16 CheckPCS[3];
	uint16 CheckColorant[cmsMAXCHANNELS];
	switch(Pass) {
		case 1:
		    nc = cmsAllocNamedColorList(DbgThread(), 0, 4, "prefix", "suffix");
		    if(nc == NULL) 
				return 0;
		    for(i = 0; i < max_check; i++) {
			    PCS[0] = PCS[1] = PCS[2] = (uint16)i;
			    Colorant[0] = Colorant[1] = Colorant[2] = Colorant[3] = (uint16)(max_check - i);
			    sprintf(Name, "#%d", i);
			    if(!cmsAppendNamedColor(nc, Name, PCS, Colorant)) {
				    Fail("Couldn't append named color"); return 0;
			    }
		    }
		    rc = cmsWriteTag(hProfile, tag, nc);
		    cmsFreeNamedColorList(nc);
		    return rc;
		case 2:
		    nc = (cmsNAMEDCOLORLIST*)cmsReadTag(hProfile, tag);
		    if(nc == NULL) 
				return 0;
		    for(i = 0; i < max_check; i++) {
			    CheckPCS[0] = CheckPCS[1] = CheckPCS[2] = (uint16)i;
			    CheckColorant[0] = CheckColorant[1] = CheckColorant[2] = CheckColorant[3] = (uint16)(max_check - i);
			    sprintf(CheckName, "#%d", i);
			    if(!cmsNamedColorInfo(nc, i, Name, NULL, NULL, PCS, Colorant)) {
				    Fail("Invalid string"); 
					return 0;
			    }
			    for(j = 0; j < 3; j++) {
				    if(CheckPCS[j] != PCS[j]) {
					    Fail("Invalid PCS"); 
						return 0;
				    }
			    }
			    // This is only used on named color list
			    if(colorant_check) {
				    for(j = 0; j < 4; j++) {
					    if(CheckColorant[j] != Colorant[j]) {
						    Fail("Invalid Colorant"); 
							return 0;
					    }
				    }
			    }
			    if(!sstreq(Name, CheckName)) {
				    Fail("Invalid Name");  
					return 0;
			    }
		    }
		    return 1;

		default: return 0;
	}
}

static int32 CheckLUT(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsPipeline * Lut, * Pt;
	int32 rc;
	switch(Pass) {
		case 1:
		    Lut = cmsPipelineAlloc(DbgThread(), 3, 3);
		    if(!Lut) 
				return 0;
		    // Create an identity LUT
		    cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocIdentityCurves(DbgThread(), 3));
		    cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocIdentityCLut(DbgThread(), 3));
		    cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocIdentityCurves(DbgThread(), 3));
		    rc =  cmsWriteTag(hProfile, tag, Lut);
		    cmsPipelineFree(Lut);
		    return rc;
		case 2:
		    Pt = (cmsPipeline *)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    // Transform values, check for identity
		    return Check16LUT(Pt);

		default:
		    return 0;
	}
}

static int32 CheckCHAD(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	double * Pt;
	double CHAD[] = { 0, .1, .2, .3, .4, .5, .6, .7, .8 };
	int32 i;
	switch(Pass) {
		case 1:
		    return cmsWriteTag(hProfile, tag, CHAD);

		case 2:
		    Pt = (double *)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) return 0;

		    for(i = 0; i < 9; i++) {
			    if(!IsGoodFixed15_16("CHAD", Pt[i], CHAD[i])) return 0;
		    }

		    return 1;

		default:
		    return 0;
	}
}

static int32 CheckChromaticity(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsCIExyYTRIPLE * Pt, c = { {0, .1, 1 }, { .3, .4, 1 }, { .6, .7, 1 }};
	switch(Pass) {
		case 1:
		    return cmsWriteTag(hProfile, tag, &c);
		case 2:
		    Pt = (cmsCIExyYTRIPLE*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    if(!IsGoodFixed15_16("xyY", Pt->Red.x, c.Red.x)) 
				return 0;
		    if(!IsGoodFixed15_16("xyY", Pt->Red.y, c.Red.y)) 
				return 0;
		    if(!IsGoodFixed15_16("xyY", Pt->Green.x, c.Green.x)) 
				return 0;
		    if(!IsGoodFixed15_16("xyY", Pt->Green.y, c.Green.y)) 
				return 0;
		    if(!IsGoodFixed15_16("xyY", Pt->Blue.x, c.Blue.x)) 
				return 0;
		    if(!IsGoodFixed15_16("xyY", Pt->Blue.y, c.Blue.y)) 
				return 0;
		    return 1;
		default:
		    return 0;
	}
}

static int32 CheckColorantOrder(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	uint8 * Pt, c[cmsMAXCHANNELS];
	int32 i;

	switch(Pass) {
		case 1:
		    for(i = 0; i < cmsMAXCHANNELS; i++) c[i] = (uint8)(cmsMAXCHANNELS - i - 1);
		    return cmsWriteTag(hProfile, tag, c);

		case 2:
		    Pt = (uint8 *)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) return 0;

		    for(i = 0; i < cmsMAXCHANNELS; i++) {
			    if(Pt[i] != ( cmsMAXCHANNELS - i - 1 )) return 0;
		    }
		    return 1;

		default:
		    return 0;
	}
}

static int32 CheckMeasurement(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsICCMeasurementConditions * Pt, m;

	switch(Pass) {
		case 1:
		    m.Backing.X = 0.1;
		    m.Backing.Y = 0.2;
		    m.Backing.Z = 0.3;
		    m.Flare = 1.0;
		    m.Geometry = 1;
		    m.IlluminantType = cmsILLUMINANT_TYPE_D50;
		    m.Observer = 1;
		    return cmsWriteTag(hProfile, tag, &m);

		case 2:
		    Pt = (cmsICCMeasurementConditions*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) return 0;

		    if(!IsGoodFixed15_16("Backing", Pt->Backing.X, 0.1)) return 0;
		    if(!IsGoodFixed15_16("Backing", Pt->Backing.Y, 0.2)) return 0;
		    if(!IsGoodFixed15_16("Backing", Pt->Backing.Z, 0.3)) return 0;
		    if(!IsGoodFixed15_16("Flare",   Pt->Flare, 1.0)) return 0;

		    if(Pt->Geometry != 1) return 0;
		    if(Pt->IlluminantType != cmsILLUMINANT_TYPE_D50) return 0;
		    if(Pt->Observer != 1) return 0;
		    return 1;

		default:
		    return 0;
	}
}

static int32 CheckUcrBg(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsUcrBg * Pt, m;
	int32 rc;
	char Buffer[256];

	switch(Pass) {
		case 1:
		    m.Ucr = cmsBuildGamma(DbgThread(), 2.4);
		    m.Bg  = cmsBuildGamma(DbgThread(), -2.2);
		    m.Desc = cmsMLUalloc(DbgThread(), 1);
		    cmsMLUsetASCII(m.Desc,  cmsNoLanguage, cmsNoCountry, "test UCR/BG");
		    rc = cmsWriteTag(hProfile, tag, &m);
		    cmsMLUfree(m.Desc);
		    cmsFreeToneCurve(m.Bg);
		    cmsFreeToneCurve(m.Ucr);
		    return rc;
		case 2:
		    Pt = (cmsUcrBg*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    cmsMLUgetASCII(Pt->Desc, cmsNoLanguage, cmsNoCountry, Buffer, 256);
		    if(!sstreq(Buffer, "test UCR/BG")) 
				return 0;
		    return 1;
		default:
		    return 0;
	}
}

static int32 CheckCRDinfo(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsMLU * mlu;
	char Buffer[256];
	int32 rc;
	switch(Pass) {
		case 1:
		    mlu = cmsMLUalloc(DbgThread(), 5);
		    cmsMLUsetWide(mlu,  "PS", "nm", L"test postscript");
		    cmsMLUsetWide(mlu,  "PS", "#0", L"perceptual");
		    cmsMLUsetWide(mlu,  "PS", "#1", L"relative_colorimetric");
		    cmsMLUsetWide(mlu,  "PS", "#2", L"saturation");
		    cmsMLUsetWide(mlu,  "PS", "#3", L"absolute_colorimetric");
		    rc = cmsWriteTag(hProfile, tag, mlu);
		    cmsMLUfree(mlu);
		    return rc;
		case 2:
		    mlu = (cmsMLU*)cmsReadTag(hProfile, tag);
		    if(mlu == NULL) 
				return 0;
		    cmsMLUgetASCII(mlu, "PS", "nm", Buffer, 256);
		    if(!sstreq(Buffer, "test postscript")) 
				return 0;
		    cmsMLUgetASCII(mlu, "PS", "#0", Buffer, 256);
		    if(!sstreq(Buffer, "perceptual")) 
				return 0;
		    cmsMLUgetASCII(mlu, "PS", "#1", Buffer, 256);
		    if(!sstreq(Buffer, "relative_colorimetric")) 
				return 0;
		    cmsMLUgetASCII(mlu, "PS", "#2", Buffer, 256);
		    if(!sstreq(Buffer, "saturation"))
				return 0;
		    cmsMLUgetASCII(mlu, "PS", "#3", Buffer, 256);
		    if(!sstreq(Buffer, "absolute_colorimetric")) 
				return 0;
		    return 1;
		default:
		    return 0;
	}
}

static cmsToneCurve * CreateSegmentedCurve()
{
	cmsCurveSegment Seg[3];
	float Sampled[2] = { 0, 1};
	Seg[0].Type = 6;
	Seg[0].Params[0] = 1;
	Seg[0].Params[1] = 0;
	Seg[0].Params[2] = 0;
	Seg[0].Params[3] = 0;
	Seg[0].x0 = -1E22F;
	Seg[0].x1 = 0;
	Seg[1].Type = 0;
	Seg[1].nGridPoints = 2;
	Seg[1].SampledPoints = Sampled;
	Seg[1].x0 = 0;
	Seg[1].x1 = 1;
	Seg[2].Type = 6;
	Seg[2].Params[0] = 1;
	Seg[2].Params[1] = 0;
	Seg[2].Params[2] = 0;
	Seg[2].Params[3] = 0;
	Seg[2].x0 = 1;
	Seg[2].x1 = 1E22F;
	return cmsBuildSegmentedToneCurve(DbgThread(), 3, Seg);
}

static int32 CheckMPE(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsPipeline * Lut;
	cmsPipeline * Pt;
	cmsToneCurve * G[3];
	int32 rc;
	switch(Pass) {
		case 1:
		    Lut = cmsPipelineAlloc(DbgThread(), 3, 3);
		    cmsPipelineInsertStage(Lut, cmsAT_BEGIN, _cmsStageAllocLabV2ToV4(DbgThread()));
		    cmsPipelineInsertStage(Lut, cmsAT_END, _cmsStageAllocLabV4ToV2(DbgThread()));
		    AddIdentityCLUTfloat(Lut);
		    G[0] = G[1] = G[2] = CreateSegmentedCurve();
		    cmsPipelineInsertStage(Lut, cmsAT_END, cmsStageAllocToneCurves(DbgThread(), 3, G));
		    cmsFreeToneCurve(G[0]);
		    rc = cmsWriteTag(hProfile, tag, Lut);
		    cmsPipelineFree(Lut);
		    return rc;
		case 2:
		    Pt = (cmsPipeline *)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    return CheckFloatLUT(Pt);
		default:
		    return 0;
	}
}

static int32 CheckScreening(int32 Pass,  cmsHPROFILE hProfile, cmsTagSignature tag)
{
	cmsScreening * Pt, sc;
	int32 rc;
	switch(Pass) {
		case 1:
		    sc.Flag = 0;
		    sc.nChannels = 1;
		    sc.Channels[0].Frequency = 2.0;
		    sc.Channels[0].ScreenAngle = 3.0;
		    sc.Channels[0].SpotShape = cmsSPOT_ELLIPSE;
		    rc = cmsWriteTag(hProfile, tag, &sc);
		    return rc;
		case 2:
		    Pt = (cmsScreening*)cmsReadTag(hProfile, tag);
		    if(Pt == NULL) 
				return 0;
		    if(Pt->nChannels != 1) 
				return 0;
		    if(Pt->Flag      != 0) 
				return 0;
		    if(!IsGoodFixed15_16("Freq", Pt->Channels[0].Frequency, 2.0)) 
				return 0;
		    if(!IsGoodFixed15_16("Angle", Pt->Channels[0].ScreenAngle, 3.0)) 
				return 0;
		    if(Pt->Channels[0].SpotShape != cmsSPOT_ELLIPSE) 
				return 0;
		    return 1;
		default:
		    return 0;
	}
}

static boolint CheckOneStr(cmsMLU* mlu, int32 n)
{
	char Buffer[256], Buffer2[256];
	cmsMLUgetASCII(mlu, "en", "US", Buffer, 255);
	sprintf(Buffer2, "Hello, world %d", n);
	if(!sstreq(Buffer, Buffer2)) 
		return FALSE;
	cmsMLUgetASCII(mlu, "es", "ES", Buffer, 255);
	sprintf(Buffer2, "Hola, mundo %d", n);
	if(!sstreq(Buffer, Buffer2)) 
		return FALSE;
	return TRUE;
}

static void SetOneStr(cmsMLU** mlu, const wchar_t * s1, const wchar_t * s2)
{
	*mlu = cmsMLUalloc(DbgThread(), 0);
	cmsMLUsetWide(*mlu, "en", "US", s1);
	cmsMLUsetWide(*mlu, "es", "ES", s2);
}

static int32 CheckProfileSequenceTag(int32 Pass,  cmsHPROFILE hProfile)
{
	cmsSEQ* s;
	int32 i;
	switch(Pass) {
		case 1:
		    s = cmsAllocProfileSequenceDescription(DbgThread(), 3);
		    if(!s) 
				return 0;
		    SetOneStr(&s->seq[0].Manufacturer, L"Hello, world 0", L"Hola, mundo 0");
		    SetOneStr(&s->seq[0].Model, L"Hello, world 0", L"Hola, mundo 0");
		    SetOneStr(&s->seq[1].Manufacturer, L"Hello, world 1", L"Hola, mundo 1");
		    SetOneStr(&s->seq[1].Model, L"Hello, world 1", L"Hola, mundo 1");
		    SetOneStr(&s->seq[2].Manufacturer, L"Hello, world 2", L"Hola, mundo 2");
		    SetOneStr(&s->seq[2].Model, L"Hello, world 2", L"Hola, mundo 2");
#ifdef CMS_DONT_USE_INT64
		    s->seq[0].attributes[0] = cmsTransparency|cmsMatte;
		    s->seq[0].attributes[1] = 0;
#else
		    s->seq[0].attributes = cmsTransparency|cmsMatte;
#endif
#ifdef CMS_DONT_USE_INT64
		    s->seq[1].attributes[0] = cmsReflective|cmsMatte;
		    s->seq[1].attributes[1] = 0;
#else
		    s->seq[1].attributes = cmsReflective|cmsMatte;
#endif
#ifdef CMS_DONT_USE_INT64
		    s->seq[2].attributes[0] = cmsTransparency|cmsGlossy;
		    s->seq[2].attributes[1] = 0;
#else
		    s->seq[2].attributes = cmsTransparency|cmsGlossy;
#endif
		    if(!cmsWriteTag(hProfile, cmsSigProfileSequenceDescTag, s)) 
				return 0;
		    cmsFreeProfileSequenceDescription(s);
		    return 1;
		case 2:
		    s = (cmsSEQ*)cmsReadTag(hProfile, cmsSigProfileSequenceDescTag);
		    if(!s) return 0;
		    if(s->n != 3) return 0;
#ifdef CMS_DONT_USE_INT64
		    if(s->seq[0].attributes[0] != (cmsTransparency|cmsMatte)) return 0;
		    if(s->seq[0].attributes[1] != 0) return 0;
#else
		    if(s->seq[0].attributes != (cmsTransparency|cmsMatte)) return 0;
#endif
#ifdef CMS_DONT_USE_INT64
		    if(s->seq[1].attributes[0] != (cmsReflective|cmsMatte)) return 0;
		    if(s->seq[1].attributes[1] != 0) return 0;
#else
		    if(s->seq[1].attributes != (cmsReflective|cmsMatte)) return 0;
#endif
#ifdef CMS_DONT_USE_INT64
		    if(s->seq[2].attributes[0] != (cmsTransparency|cmsGlossy)) return 0;
		    if(s->seq[2].attributes[1] != 0) return 0;
#else
		    if(s->seq[2].attributes != (cmsTransparency|cmsGlossy)) return 0;
#endif
		    // Check MLU
		    for(i = 0; i < 3; i++) {
			    if(!CheckOneStr(s->seq[i].Manufacturer, i)) return 0;
			    if(!CheckOneStr(s->seq[i].Model, i)) return 0;
		    }
		    return 1;
		default:
		    return 0;
	}
}

static int32 CheckProfileSequenceIDTag(int32 Pass,  cmsHPROFILE hProfile)
{
	cmsSEQ* s;
	int32 i;
	switch(Pass) {
		case 1:
		    s = cmsAllocProfileSequenceDescription(DbgThread(), 3);
		    if(!s) 
				return 0;
		    memcpy(s->seq[0].ProfileID.ID8, "0123456789ABCDEF", 16);
		    memcpy(s->seq[1].ProfileID.ID8, "1111111111111111", 16);
		    memcpy(s->seq[2].ProfileID.ID8, "2222222222222222", 16);
		    SetOneStr(&s->seq[0].Description, L"Hello, world 0", L"Hola, mundo 0");
		    SetOneStr(&s->seq[1].Description, L"Hello, world 1", L"Hola, mundo 1");
		    SetOneStr(&s->seq[2].Description, L"Hello, world 2", L"Hola, mundo 2");
		    if(!cmsWriteTag(hProfile, cmsSigProfileSequenceIdTag, s)) 
				return 0;
		    cmsFreeProfileSequenceDescription(s);
		    return 1;
		case 2:
		    s = (cmsSEQ*)cmsReadTag(hProfile, cmsSigProfileSequenceIdTag);
		    if(!s) return 0;
		    if(s->n != 3) return 0;
		    if(memcmp(s->seq[0].ProfileID.ID8, "0123456789ABCDEF", 16) != 0) return 0;
		    if(memcmp(s->seq[1].ProfileID.ID8, "1111111111111111", 16) != 0) return 0;
		    if(memcmp(s->seq[2].ProfileID.ID8, "2222222222222222", 16) != 0) return 0;
		    for(i = 0; i < 3; i++) {
			    if(!CheckOneStr(s->seq[i].Description, i)) return 0;
		    }
		    return 1;
		default:
		    return 0;
	}
}

static int32 CheckICCViewingConditions(int32 Pass,  cmsHPROFILE hProfile)
{
	cmsICCViewingConditions* v;
	cmsICCViewingConditions s;
	switch(Pass) {
		case 1:
		    s.IlluminantType = 1;
		    s.IlluminantXYZ.X = 0.1;
		    s.IlluminantXYZ.Y = 0.2;
		    s.IlluminantXYZ.Z = 0.3;
		    s.SurroundXYZ.X = 0.4;
		    s.SurroundXYZ.Y = 0.5;
		    s.SurroundXYZ.Z = 0.6;
		    if(!cmsWriteTag(hProfile, cmsSigViewingConditionsTag, &s)) return 0;
		    return 1;
		case 2:
		    v = (cmsICCViewingConditions*)cmsReadTag(hProfile, cmsSigViewingConditionsTag);
		    if(v == NULL) return 0;
		    if(v->IlluminantType != 1) return 0;
		    if(!IsGoodVal("IlluminantXYZ.X", v->IlluminantXYZ.X, 0.1, 0.001)) return 0;
		    if(!IsGoodVal("IlluminantXYZ.Y", v->IlluminantXYZ.Y, 0.2, 0.001)) return 0;
		    if(!IsGoodVal("IlluminantXYZ.Z", v->IlluminantXYZ.Z, 0.3, 0.001)) return 0;
		    if(!IsGoodVal("SurroundXYZ.X", v->SurroundXYZ.X, 0.4, 0.001)) return 0;
		    if(!IsGoodVal("SurroundXYZ.Y", v->SurroundXYZ.Y, 0.5, 0.001)) return 0;
		    if(!IsGoodVal("SurroundXYZ.Z", v->SurroundXYZ.Z, 0.6, 0.001)) return 0;
		    return 1;
		default:
		    return 0;
	}
}

static int32 CheckVCGT(int32 Pass,  cmsHPROFILE hProfile)
{
	cmsToneCurve * Curves[3];
	cmsToneCurve ** PtrCurve;
	switch(Pass) {
		case 1:
		    Curves[0] = cmsBuildGamma(DbgThread(), 1.1);
		    Curves[1] = cmsBuildGamma(DbgThread(), 2.2);
		    Curves[2] = cmsBuildGamma(DbgThread(), 3.4);
		    if(!cmsWriteTag(hProfile, cmsSigVcgtTag, Curves)) 
				return 0;
		    cmsFreeToneCurveTriple(Curves);
		    return 1;
		case 2:
		    PtrCurve = (cmsToneCurve **)cmsReadTag(hProfile, cmsSigVcgtTag);
		    if(PtrCurve == NULL) 
				return 0;
		    if(!IsGoodVal("VCGT R", cmsEstimateGamma(PtrCurve[0], 0.01), 1.1, 0.001)) return 0;
		    if(!IsGoodVal("VCGT G", cmsEstimateGamma(PtrCurve[1], 0.01), 2.2, 0.001)) return 0;
		    if(!IsGoodVal("VCGT B", cmsEstimateGamma(PtrCurve[2], 0.01), 3.4, 0.001)) return 0;
		    return 1;
		default:;
	}
	return 0;
}

// Only one of the two following may be used, as they share the same tag
static int32 CheckDictionary16(int32 Pass, cmsHPROFILE hProfile)
{
	cmsHANDLE hDict;
	const cmsDICTentry* e;
	switch(Pass) {
		case 1:
		    hDict = cmsDictAlloc(DbgThread());
		    cmsDictAddEntry(hDict, L"Name0",  NULL, NULL, NULL);
		    cmsDictAddEntry(hDict, L"Name1",  L"", NULL, NULL);
		    cmsDictAddEntry(hDict, L"Name",  L"String", NULL, NULL);
		    cmsDictAddEntry(hDict, L"Name2", L"12",    NULL, NULL);
		    if(!cmsWriteTag(hProfile, cmsSigMetaTag, hDict)) return 0;
		    cmsDictFree(hDict);
		    return 1;

		case 2:

		    hDict = cmsReadTag(hProfile, cmsSigMetaTag);
		    if(hDict == NULL) return 0;
		    e = cmsDictGetEntryList(hDict);
		    if(memcmp(e->Name, L"Name2", sizeof(wchar_t) * 5) != 0) return 0;
		    if(memcmp(e->Value, L"12",  sizeof(wchar_t) * 2) != 0) return 0;
		    e = cmsDictNextEntry(e);
		    if(memcmp(e->Name, L"Name", sizeof(wchar_t) * 4) != 0) return 0;
		    if(memcmp(e->Value, L"String",  sizeof(wchar_t) * 5) != 0) return 0;
		    e = cmsDictNextEntry(e);
		    if(memcmp(e->Name, L"Name1", sizeof(wchar_t) *5) != 0) return 0;
		    if(e->Value == NULL) return 0;
		    if(*e->Value != 0) return 0;
		    e = cmsDictNextEntry(e);
		    if(memcmp(e->Name, L"Name0", sizeof(wchar_t) * 5) != 0) return 0;
		    if(e->Value != NULL) return 0;
		    return 1;

		default:;
	}

	return 0;
}

static int32 CheckDictionary24(int32 Pass,  cmsHPROFILE hProfile)
{
	cmsHANDLE hDict;
	const cmsDICTentry* e;
	cmsMLU* DisplayName;
	char Buffer[256];
	int32 rc = 1;

	switch(Pass) {
		case 1:
		    hDict = cmsDictAlloc(DbgThread());

		    DisplayName = cmsMLUalloc(DbgThread(), 0);

		    cmsMLUsetWide(DisplayName, "en", "US", L"Hello, world");
		    cmsMLUsetWide(DisplayName, "es", "ES", L"Hola, mundo");
		    cmsMLUsetWide(DisplayName, "fr", "FR", L"Bonjour, le monde");
		    cmsMLUsetWide(DisplayName, "ca", "CA", L"Hola, mon");

		    cmsDictAddEntry(hDict, L"Name",  L"String", DisplayName, NULL);
		    cmsMLUfree(DisplayName);

		    cmsDictAddEntry(hDict, L"Name2", L"12",    NULL, NULL);
		    if(!cmsWriteTag(hProfile, cmsSigMetaTag, hDict)) return 0;
		    cmsDictFree(hDict);
		    return 1;
		case 2:
		    hDict = cmsReadTag(hProfile, cmsSigMetaTag);
		    if(hDict == NULL) 
				return 0;
		    e = cmsDictGetEntryList(hDict);
		    if(memcmp(e->Name, L"Name2", sizeof(wchar_t) * 5) != 0) 
				return 0;
		    if(memcmp(e->Value, L"12",  sizeof(wchar_t) * 2) != 0) 
				return 0;
		    e = cmsDictNextEntry(e);
		    if(memcmp(e->Name, L"Name", sizeof(wchar_t) * 4) != 0) 
				return 0;
		    if(memcmp(e->Value, L"String",  sizeof(wchar_t) * 5) != 0) 
				return 0;
		    cmsMLUgetASCII(e->DisplayName, "en", "US", Buffer, 256);
		    if(!sstreq(Buffer, "Hello, world"))
				rc = 0;
		    cmsMLUgetASCII(e->DisplayName, "es", "ES", Buffer, 256);
		    if(!sstreq(Buffer, "Hola, mundo"))
				rc = 0;
		    cmsMLUgetASCII(e->DisplayName, "fr", "FR", Buffer, 256);
		    if(!sstreq(Buffer, "Bonjour, le monde")) 
				rc = 0;
		    cmsMLUgetASCII(e->DisplayName, "ca", "CA", Buffer, 256);
		    if(!sstreq(Buffer, "Hola, mon"))
				rc = 0;
		    if(rc == 0)
			    Fail("Unexpected string '%s'", Buffer);
		    return 1;
		default:;
	}
	return 0;
}

static int32 CheckRAWtags(int32 Pass,  cmsHPROFILE hProfile)
{
	char Buffer[7];

	switch(Pass) {
		case 1:
		    return cmsWriteRawTag(hProfile, (cmsTagSignature)0x31323334, "data123", 7);

		case 2:
		    if(!cmsReadRawTag(hProfile, (cmsTagSignature)0x31323334, Buffer, 7)) return 0;

		    if(strncmp(Buffer, "data123", 7) != 0) return 0;
		    return 1;

		default:
		    return 0;
	}
}

// This is a very big test that checks every single tag
static int32 CheckProfileCreation(FILE * fOut)
{
	cmsHPROFILE h;
	int32 Pass;
	h = cmsCreateProfilePlaceholder(DbgThread());
	if(!h) 
		return 0;
	cmsSetProfileVersion(h, 4.3);
	if(cmsGetTagCount(h) != 0) {
		Fail("Empty profile with nonzero number of tags"); goto Error;
	}
	if(cmsIsTag(h, cmsSigAToB0Tag)) {
		Fail("Found a tag in an empty profile"); goto Error;
	}
	cmsSetColorSpace(h, cmsSigRgbData);
	if(cmsGetColorSpace(h) !=  cmsSigRgbData) {
		Fail("Unable to set colorspace"); goto Error;
	}
	cmsSetPCS(h, cmsSigLabData);
	if(cmsGetPCS(h) !=  cmsSigLabData) {
		Fail("Unable to set colorspace"); goto Error;
	}
	cmsSetDeviceClass(h, cmsSigDisplayClass);
	if(cmsGetDeviceClass(h) != cmsSigDisplayClass) {
		Fail("Unable to set deviceclass"); goto Error;
	}
	cmsSetHeaderRenderingIntent(h, INTENT_SATURATION);
	if(cmsGetHeaderRenderingIntent(h) != INTENT_SATURATION) {
		Fail("Unable to set rendering intent"); goto Error;
	}
	for(Pass = 1; Pass <= 2; Pass++) {
		SubTest(fOut, "Tags holding XYZ");
		if(!CheckXYZ(Pass, h, cmsSigBlueColorantTag)) goto Error;
		if(!CheckXYZ(Pass, h, cmsSigGreenColorantTag)) goto Error;
		if(!CheckXYZ(Pass, h, cmsSigRedColorantTag)) goto Error;
		if(!CheckXYZ(Pass, h, cmsSigMediaBlackPointTag)) goto Error;
		if(!CheckXYZ(Pass, h, cmsSigMediaWhitePointTag)) goto Error;
		if(!CheckXYZ(Pass, h, cmsSigLuminanceTag)) goto Error;
		SubTest(fOut, "Tags holding curves");
		if(!CheckGamma(Pass, h, cmsSigBlueTRCTag)) goto Error;
		if(!CheckGamma(Pass, h, cmsSigGrayTRCTag)) goto Error;
		if(!CheckGamma(Pass, h, cmsSigGreenTRCTag)) goto Error;
		if(!CheckGamma(Pass, h, cmsSigRedTRCTag)) goto Error;
		SubTest(fOut, "Tags holding text");
		if(!CheckTextSingle(Pass, h, cmsSigCharTargetTag)) goto Error;
		if(!CheckTextSingle(Pass, h, cmsSigScreeningDescTag)) goto Error;
		if(!CheckText(Pass, h, cmsSigCopyrightTag)) goto Error;
		if(!CheckText(Pass, h, cmsSigProfileDescriptionTag)) goto Error;
		if(!CheckText(Pass, h, cmsSigDeviceMfgDescTag)) goto Error;
		if(!CheckText(Pass, h, cmsSigDeviceModelDescTag)) goto Error;
		if(!CheckText(Pass, h, cmsSigViewingCondDescTag)) goto Error;
		SubTest(fOut, "Tags holding cmsICCData");
		if(!CheckData(Pass, h, cmsSigPs2CRD0Tag)) goto Error;
		if(!CheckData(Pass, h, cmsSigPs2CRD1Tag)) goto Error;
		if(!CheckData(Pass, h, cmsSigPs2CRD2Tag)) goto Error;
		if(!CheckData(Pass, h, cmsSigPs2CRD3Tag)) goto Error;
		if(!CheckData(Pass, h, cmsSigPs2CSATag)) goto Error;
		if(!CheckData(Pass, h, cmsSigPs2RenderingIntentTag)) goto Error;
		SubTest(fOut, "Tags holding signatures");
		if(!CheckSignature(Pass, h, cmsSigColorimetricIntentImageStateTag)) goto Error;
		if(!CheckSignature(Pass, h, cmsSigPerceptualRenderingIntentGamutTag)) goto Error;
		if(!CheckSignature(Pass, h, cmsSigSaturationRenderingIntentGamutTag)) goto Error;
		if(!CheckSignature(Pass, h, cmsSigTechnologyTag)) goto Error;
		SubTest(fOut, "Tags holding date_time");
		if(!CheckDateTime(Pass, h, cmsSigCalibrationDateTimeTag)) goto Error;
		if(!CheckDateTime(Pass, h, cmsSigDateTimeTag)) goto Error;
		SubTest(fOut, "Tags holding named color lists");
		if(!CheckNamedColor(Pass, h, cmsSigColorantTableTag, 15, FALSE)) goto Error;
		if(!CheckNamedColor(Pass, h, cmsSigColorantTableOutTag, 15, FALSE)) goto Error;
		if(!CheckNamedColor(Pass, h, cmsSigNamedColor2Tag, 4096, TRUE)) goto Error;
		SubTest(fOut, "Tags holding LUTs");
		if(!CheckLUT(Pass, h, cmsSigAToB0Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigAToB1Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigAToB2Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigBToA0Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigBToA1Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigBToA2Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigPreview0Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigPreview1Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigPreview2Tag)) goto Error;
		if(!CheckLUT(Pass, h, cmsSigGamutTag)) goto Error;
		SubTest(fOut, "Tags holding CHAD");
		if(!CheckCHAD(Pass, h, cmsSigChromaticAdaptationTag)) goto Error;
		SubTest(fOut, "Tags holding Chromaticity");
		if(!CheckChromaticity(Pass, h, cmsSigChromaticityTag)) goto Error;
		SubTest(fOut, "Tags holding colorant order");
		if(!CheckColorantOrder(Pass, h, cmsSigColorantOrderTag)) goto Error;
		SubTest(fOut, "Tags holding measurement");
		if(!CheckMeasurement(Pass, h, cmsSigMeasurementTag)) goto Error;
		SubTest(fOut, "Tags holding CRD info");
		if(!CheckCRDinfo(Pass, h, cmsSigCrdInfoTag)) goto Error;
		SubTest(fOut, "Tags holding UCR/BG");
		if(!CheckUcrBg(Pass, h, cmsSigUcrBgTag)) goto Error;
		SubTest(fOut, "Tags holding MPE");
		if(!CheckMPE(Pass, h, cmsSigDToB0Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigDToB1Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigDToB2Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigDToB3Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigBToD0Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigBToD1Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigBToD2Tag)) goto Error;
		if(!CheckMPE(Pass, h, cmsSigBToD3Tag)) goto Error;
		SubTest(fOut, "Tags using screening");
		if(!CheckScreening(Pass, h, cmsSigScreeningTag)) goto Error;
		SubTest(fOut, "Tags holding profile sequence description");
		if(!CheckProfileSequenceTag(Pass, h)) goto Error;
		if(!CheckProfileSequenceIDTag(Pass, h)) goto Error;
		SubTest(fOut, "Tags holding ICC viewing conditions");
		if(!CheckICCViewingConditions(Pass, h)) goto Error;
		SubTest(fOut, "VCGT tags");
		if(!CheckVCGT(Pass, h)) goto Error;
		SubTest(fOut, "RAW tags");
		if(!CheckRAWtags(Pass, h)) goto Error;
		SubTest(fOut, "Dictionary meta tags");
		// if(!CheckDictionary16(Pass, h)) goto Error;
		if(!CheckDictionary24(Pass, h)) 
			goto Error;
		if(Pass == 1) {
			cmsSaveProfileToFile(h, "alltags.icc");
			cmsCloseProfile(h);
			h = cmsOpenProfileFromFileTHR(DbgThread(), "alltags.icc", "r");
		}
	}
	/*
	   Not implemented (by design):

	   cmsSigDataTag                   = 0x64617461,  // 'data'  -- Unused
	   cmsSigDeviceSettingsTag         = 0x64657673,  // 'devs'  -- Unused
	   cmsSigNamedColorTag             = 0x6E636f6C,  // 'ncol'  -- Don't use this one, deprecated by ICC
	   cmsSigOutputResponseTag         = 0x72657370,  // 'resp'  -- Possible patent on this
	 */
	cmsCloseProfile(h);
	remove("alltags.icc");
	return 1;
Error:
	cmsCloseProfile(h);
	remove("alltags.icc");
	return 0;
}

// Thanks to Christopher James Halse Rogers for the bugfixing and providing this test
static int32 CheckVersionHeaderWriting()
{
	cmsHPROFILE h;
	int index;
	float test_versions[] = { 2.3f, 4.08f, 4.09f, 4.3f };
	for(index = 0; index < sizeof(test_versions)/sizeof(test_versions[0]); index++) {
		h = cmsCreateProfilePlaceholder(DbgThread());
		if(!h) return 0;
		cmsSetProfileVersion(h, test_versions[index]);
		cmsSaveProfileToFile(h, "versions.icc");
		cmsCloseProfile(h);
		h = cmsOpenProfileFromFileTHR(DbgThread(), "versions.icc", "r");
		// Only the first 3 digits are significant
		if(fabs(cmsGetProfileVersion(h) - test_versions[index]) > 0.005) {
			Fail("Version failed to round-trip: wrote %.2f, read %.2f", test_versions[index], cmsGetProfileVersion(h));
			return 0;
		}
		cmsCloseProfile(h);
		remove("versions.icc");
	}
	return 1;
}

static void GetTestProfileName(const char * pName, char * pBuf, size_t bufLen)
{
	if(TestbedPath.IsEmpty())
		strnzcpy(pBuf, pName, bufLen);
	else {
		SString temp_buf;
		(temp_buf = TestbedPath).SetLastDSlash().Cat(pName);
		strnzcpy(pBuf, temp_buf, bufLen);
	}
}

// Test on Richard Hughes "crayons.icc"
static int32 CheckMultilocalizedProfile()
{
	char buffer[256];
	char file_name[512];
	GetTestProfileName("crayons.icc", file_name, sizeof(file_name));
	cmsHPROFILE hProfile = cmsOpenProfileFromFile(/*"crayons.icc"*/file_name, "r");
	cmsMLU * Pt = (cmsMLU*)cmsReadTag(hProfile, cmsSigProfileDescriptionTag);
	cmsMLUgetASCII(Pt, "en", "GB", buffer, 256);
	if(!sstreq(buffer, "Crayon Colours"))
		return FALSE;
	cmsMLUgetASCII(Pt, "en", "US", buffer, 256);
	if(!sstreq(buffer, "Crayon Colors")) 
		return FALSE;
	cmsCloseProfile(hProfile);
	return TRUE;
}

// Error reporting
//  -------------------------------------------------------------------------------------------------------

static void ErrorReportingFunction(FILE * /*fOut*/, cmsContext ContextID, uint32 ErrorCode, const char * Text)
{
	TrappedError = TRUE;
	SimultaneousErrors++;
	strncpy(ReasonToFailBuffer, Text, TEXT_ERROR_BUFFER_SIZE-1);
	CXX_UNUSED(ContextID);
	CXX_UNUSED(ErrorCode);
}

static int32 CheckBadProfiles()
{
	cmsHPROFILE h = cmsOpenProfileFromFileTHR(DbgThread(), "IDoNotExist.icc", "r");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	h = cmsOpenProfileFromFileTHR(DbgThread(), "IAmIllFormed*.icc", "r");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	// No profile name given
	h = cmsOpenProfileFromFileTHR(DbgThread(), "", "r");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	h = cmsOpenProfileFromFileTHR(DbgThread(), "..", "r");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	h = cmsOpenProfileFromFileTHR(DbgThread(), "IHaveBadAccessMode.icc", "@");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	h = cmsOpenProfileFromFileTHR(DbgThread(), "bad.icc", "r");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}

	h = cmsOpenProfileFromFileTHR(DbgThread(), "toosmall.icc", "r");
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	h = cmsOpenProfileFromMemTHR(DbgThread(), NULL, 3);
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	h = cmsOpenProfileFromMemTHR(DbgThread(), "123", 3);
	if(h) {
		cmsCloseProfile(h);
		return 0;
	}
	if(SimultaneousErrors != 9) 
		return 0;
	return 1;
}

static int32 CheckErrReportingOnBadProfiles()
{
	int32 rc;
	cmsSetLogErrorHandler(ErrorReportingFunction);
	rc = CheckBadProfiles();
	cmsSetLogErrorHandler(FatalErrorQuit);
	// Reset the error state
	TrappedError = FALSE;
	return rc;
}

static int32 CheckBadTransforms()
{
	cmsHPROFILE h1 = cmsCreate_sRGBProfile();
	cmsHTRANSFORM x1 = cmsCreateTransform(NULL, 0, NULL, 0, 0, 0);
	if(x1) {
		cmsDeleteTransform(x1);
		return 0;
	}
	x1 = cmsCreateTransform(h1, TYPE_RGB_8, h1, TYPE_RGB_8, 12345, 0);
	if(x1) {
		cmsDeleteTransform(x1);
		return 0;
	}
	x1 = cmsCreateTransform(h1, TYPE_CMYK_8, h1, TYPE_RGB_8, 0, 0);
	if(x1) {
		cmsDeleteTransform(x1);
		return 0;
	}
	x1 = cmsCreateTransform(h1, TYPE_RGB_8, h1, TYPE_CMYK_8, 1, 0);
	if(x1) {
		cmsDeleteTransform(x1);
		return 0;
	}
	// sRGB does its output as XYZ!
	x1 = cmsCreateTransform(h1, TYPE_RGB_8, NULL, TYPE_Lab_8, 1, 0);
	if(x1) {
		cmsDeleteTransform(x1);
		return 0;
	}
	cmsCloseProfile(h1);
	{
		char file_name[512];
		GetTestProfileName("test1.icc", file_name, sizeof(file_name));
		cmsHPROFILE hp1 = cmsOpenProfileFromFile(/*"test1.icc"*/file_name, "r");
		cmsHPROFILE hp2 = cmsCreate_sRGBProfile();
		x1 = cmsCreateTransform(hp1, TYPE_BGR_8, hp2, TYPE_BGR_8, INTENT_PERCEPTUAL, 0);
		cmsCloseProfile(hp1); 
		cmsCloseProfile(hp2);
		if(x1) {
			cmsDeleteTransform(x1);
			return 0;
		}
	}
	return 1;
}

static int32 CheckErrReportingOnBadTransforms()
{
	int32 rc;
	cmsSetLogErrorHandler(ErrorReportingFunction);
	rc = CheckBadTransforms();
	cmsSetLogErrorHandler(FatalErrorQuit);
	// Reset the error state
	TrappedError = FALSE;
	return rc;
}

// Check a linear xform
static int32 Check8linearXFORM(cmsHTRANSFORM xform, int32 nChan)
{
	int32 n2, i, j;
	uint8 Inw[cmsMAXCHANNELS], Outw[cmsMAXCHANNELS];
	n2 = 0;
	for(j = 0; j < 0xFF; j++) {
		memset(Inw, j, sizeof(Inw));
		cmsDoTransform(xform, Inw, Outw, 1);
		for(i = 0; i < nChan; i++) {
			int32 dif = abs(Outw[i] - j);
			if(dif > n2) n2 = dif;
		}
	}
	// We allow 2 contone of difference on 8 bits
	if(n2 > 2) {
		Fail("Differences too big (%x)", n2);
		return 0;
	}
	return 1;
}

static int32 Compare8bitXFORM(cmsHTRANSFORM xform1, cmsHTRANSFORM xform2, int32 nChan)
{
	int32 n2, i, j;
	uint8 Inw[cmsMAXCHANNELS], Outw1[cmsMAXCHANNELS], Outw2[cmsMAXCHANNELS];;
	n2 = 0;
	for(j = 0; j < 0xFF; j++) {
		memset(Inw, j, sizeof(Inw));
		cmsDoTransform(xform1, Inw, Outw1, 1);
		cmsDoTransform(xform2, Inw, Outw2, 1);
		for(i = 0; i < nChan; i++) {
			int32 dif = abs(Outw2[i] - Outw1[i]);
			if(dif > n2) n2 = dif;
		}
	}
	// We allow 2 contone of difference on 8 bits
	if(n2 > 2) {
		Fail("Differences too big (%x)", n2);
		return 0;
	}
	return 1;
}

// Check a linear xform
static int32 Check16linearXFORM(cmsHTRANSFORM xform, int32 nChan)
{
	int32 n2, i, j;
	uint16 Inw[cmsMAXCHANNELS], Outw[cmsMAXCHANNELS];

	n2 = 0;
	for(j = 0; j < 0xFFFF; j++) {
		for(i = 0; i < nChan; i++) Inw[i] = (uint16)j;

		cmsDoTransform(xform, Inw, Outw, 1);

		for(i = 0; i < nChan; i++) {
			int32 dif = abs(Outw[i] - j);
			if(dif > n2) n2 = dif;
		}

		// We allow 2 contone of difference on 16 bits
		if(n2 > 0x200) {
			Fail("Differences too big (%x)", n2);
			return 0;
		}
	}

	return 1;
}

static int32 Compare16bitXFORM(cmsHTRANSFORM xform1, cmsHTRANSFORM xform2, int32 nChan)
{
	int32 n2, i, j;
	uint16 Inw[cmsMAXCHANNELS], Outw1[cmsMAXCHANNELS], Outw2[cmsMAXCHANNELS];;
	n2 = 0;
	for(j = 0; j < 0xFFFF; j++) {
		for(i = 0; i < nChan; i++) Inw[i] = (uint16)j;

		cmsDoTransform(xform1, Inw, Outw1, 1);
		cmsDoTransform(xform2, Inw, Outw2, 1);

		for(i = 0; i < nChan; i++) {
			int32 dif = abs(Outw2[i] - Outw1[i]);
			if(dif > n2) n2 = dif;
		}
	}

	// We allow 2 contone of difference on 16 bits
	if(n2 > 0x200) {
		Fail("Differences too big (%x)", n2);
		return 0;
	}

	return 1;
}

// Check a linear xform
static int32 CheckFloatlinearXFORM(cmsHTRANSFORM xform, int32 nChan)
{
	int32 i, j;
	float In[cmsMAXCHANNELS], Out[cmsMAXCHANNELS];
	for(j = 0; j < 0xFFFF; j++) {
		for(i = 0; i < nChan; i++) In[i] = (float)(j / 65535.0); ;
		cmsDoTransform(xform, In, Out, 1);
		for(i = 0; i < nChan; i++) {
			// We allow no difference in floating point
			if(!IsGoodFixed15_16("linear xform float", Out[i], (float)(j / 65535.0)))
				return 0;
		}
	}

	return 1;
}

// Check a linear xform
static int32 CompareFloatXFORM(cmsHTRANSFORM xform1, cmsHTRANSFORM xform2, int32 nChan)
{
	int32 i, j;
	float In[cmsMAXCHANNELS], Out1[cmsMAXCHANNELS], Out2[cmsMAXCHANNELS];
	for(j = 0; j < 0xFFFF; j++) {
		for(i = 0; i < nChan; i++) In[i] = (float)(j / 65535.0); ;
		cmsDoTransform(xform1, In, Out1, 1);
		cmsDoTransform(xform2, In, Out2, 1);
		for(i = 0; i < nChan; i++) {
			// We allow no difference in floating point
			if(!IsGoodFixed15_16("linear xform float", Out1[i], Out2[i]))
				return 0;
		}
	}
	return 1;
}
//
// Curves only transforms
//
static int32 CheckCurvesOnlyTransforms(FILE * fOut)
{
	cmsHTRANSFORM xform1, xform2;
	cmsHPROFILE h1, h2, h3;
	cmsToneCurve * c1, * c2, * c3;
	int32 rc = 1;

	c1 = cmsBuildGamma(DbgThread(), 2.2);
	c2 = cmsBuildGamma(DbgThread(), 1/2.2);
	c3 = cmsBuildGamma(DbgThread(), 4.84);

	h1 = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigGrayData, &c1);
	h2 = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigGrayData, &c2);
	h3 = cmsCreateLinearizationDeviceLinkTHR(DbgThread(), cmsSigGrayData, &c3);

	SubTest(fOut, "Gray float optimizeable transform");
	xform1 = cmsCreateTransform(h1, TYPE_GRAY_FLT, h2, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, 0);
	rc &= CheckFloatlinearXFORM(xform1, 1);
	cmsDeleteTransform(xform1);
	if(rc == 0) goto Error;

	SubTest(fOut, "Gray 8 optimizeable transform");
	xform1 = cmsCreateTransform(h1, TYPE_GRAY_8, h2, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);
	rc &= Check8linearXFORM(xform1, 1);
	cmsDeleteTransform(xform1);
	if(rc == 0) goto Error;

	SubTest(fOut, "Gray 16 optimizeable transform");
	xform1 = cmsCreateTransform(h1, TYPE_GRAY_16, h2, TYPE_GRAY_16, INTENT_PERCEPTUAL, 0);
	rc &= Check16linearXFORM(xform1, 1);
	cmsDeleteTransform(xform1);
	if(rc == 0) goto Error;

	SubTest(fOut, "Gray float non-optimizeable transform");
	xform1 = cmsCreateTransform(h1, TYPE_GRAY_FLT, h1, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, 0);
	xform2 = cmsCreateTransform(h3, TYPE_GRAY_FLT, NULL, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, 0);

	rc &= CompareFloatXFORM(xform1, xform2, 1);
	cmsDeleteTransform(xform1);
	cmsDeleteTransform(xform2);
	if(rc == 0) goto Error;

	SubTest(fOut, "Gray 8 non-optimizeable transform");
	xform1 = cmsCreateTransform(h1, TYPE_GRAY_8, h1, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);
	xform2 = cmsCreateTransform(h3, TYPE_GRAY_8, NULL, TYPE_GRAY_8, INTENT_PERCEPTUAL, 0);

	rc &= Compare8bitXFORM(xform1, xform2, 1);
	cmsDeleteTransform(xform1);
	cmsDeleteTransform(xform2);
	if(rc == 0) 
		goto Error;
	SubTest(fOut, "Gray 16 non-optimizeable transform");
	xform1 = cmsCreateTransform(h1, TYPE_GRAY_16, h1, TYPE_GRAY_16, INTENT_PERCEPTUAL, 0);
	xform2 = cmsCreateTransform(h3, TYPE_GRAY_16, NULL, TYPE_GRAY_16, INTENT_PERCEPTUAL, 0);
	rc &= Compare16bitXFORM(xform1, xform2, 1);
	cmsDeleteTransform(xform1);
	cmsDeleteTransform(xform2);
	if(rc == 0) 
		goto Error;
Error:
	cmsCloseProfile(h1); 
	cmsCloseProfile(h2); 
	cmsCloseProfile(h3);
	cmsFreeToneCurve(c1); 
	cmsFreeToneCurve(c2); 
	cmsFreeToneCurve(c3);
	return rc;
}
//
// Lab to Lab trivial transforms
//
static double MaxDE;

static int32 CheckOneLab(cmsHTRANSFORM xform, double L, double a, double b)
{
	cmsCIELab In, Out;
	double dE;
	In.L = L; In.a = a; In.b = b;
	cmsDoTransform(xform, &In, &Out, 1);
	dE = cmsDeltaE(&In, &Out);
	if(dE > MaxDE) MaxDE = dE;
	if(MaxDE >  0.003) {
		Fail("dE=%f Lab1=(%f, %f, %f)\n\tLab2=(%f %f %f)", MaxDE, In.L, In.a, In.b, Out.L, Out.a, Out.b);
		cmsDoTransform(xform, &In, &Out, 1);
		return 0;
	}
	return 1;
}

// Check several Lab, slicing at non-exact values. Precision should be 16 bits. 50x50x50 checks aprox.
static int32 CheckSeveralLab(cmsHTRANSFORM xform)
{
	int32 L, a, b;
	MaxDE = 0;
	for(L = 0; L < 65536; L += 1311) {
		for(a = 0; a < 65536; a += 1232) {
			for(b = 0; b < 65536; b += 1111) {
				if(!CheckOneLab(xform, (L * 100.0) / 65535.0,
				    (a  / 257.0) - 128, (b / 257.0) - 128))
					return 0;
			}
		}
	}
	return 1;
}

static int32 OneTrivialLab(FILE * fOut, cmsHPROFILE hLab1, cmsHPROFILE hLab2, const char * txt)
{
	cmsHTRANSFORM xform;
	int32 rc;
	SubTest(fOut, txt);
	xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_DBL, hLab2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hLab1); 
	cmsCloseProfile(hLab2);
	rc = CheckSeveralLab(xform);
	cmsDeleteTransform(xform);
	return rc;
}

static int32 CheckFloatLabTransforms(FILE * fOut)
{
	return OneTrivialLab(fOut, cmsCreateLab4ProfileTHR(DbgThread(), NULL), cmsCreateLab4ProfileTHR(DbgThread(), NULL),  "Lab4/Lab4") &&
	       OneTrivialLab(fOut, cmsCreateLab2ProfileTHR(DbgThread(), NULL), cmsCreateLab2ProfileTHR(DbgThread(), NULL),  "Lab2/Lab2") &&
	       OneTrivialLab(fOut, cmsCreateLab4ProfileTHR(DbgThread(), NULL), cmsCreateLab2ProfileTHR(DbgThread(), NULL),  "Lab4/Lab2") &&
	       OneTrivialLab(fOut, cmsCreateLab2ProfileTHR(DbgThread(), NULL), cmsCreateLab4ProfileTHR(DbgThread(), NULL),  "Lab2/Lab4");
}

static int32 CheckEncodedLabTransforms()
{
	cmsHTRANSFORM xform;
	uint16 In[3];
	cmsCIELab Lab;
	cmsCIELab White = { 100, 0, 0 };
	cmsHPROFILE hLab1 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	cmsHPROFILE hLab2 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_16, hLab2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hLab1); 
	cmsCloseProfile(hLab2);
	In[0] = 0xFFFF;
	In[1] = 0x8080;
	In[2] = 0x8080;
	cmsDoTransform(xform, In, &Lab, 1);
	if(cmsDeltaE(&Lab, &White) > 0.0001) 
		return 0;
	cmsDeleteTransform(xform);
	hLab1 = cmsCreateLab2ProfileTHR(DbgThread(), NULL);
	hLab2 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_LabV2_16, hLab2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hLab1); 
	cmsCloseProfile(hLab2);
	In[0] = 0xFF00;
	In[1] = 0x8000;
	In[2] = 0x8000;
	cmsDoTransform(xform, In, &Lab, 1);
	if(cmsDeltaE(&Lab, &White) > 0.0001) 
		return 0;
	cmsDeleteTransform(xform);
	hLab2 = cmsCreateLab2ProfileTHR(DbgThread(), NULL);
	hLab1 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_DBL, hLab2, TYPE_LabV2_16, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hLab1); 
	cmsCloseProfile(hLab2);
	Lab.L = 100;
	Lab.a = 0;
	Lab.b = 0;
	cmsDoTransform(xform, &Lab, In, 1);
	if(In[0] != 0xFF00 || In[1] != 0x8000 || In[2] != 0x8000) 
		return 0;
	cmsDeleteTransform(xform);
	hLab1 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	hLab2 = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hLab1, TYPE_Lab_DBL, hLab2, TYPE_Lab_16, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hLab1); 
	cmsCloseProfile(hLab2);
	Lab.L = 100;
	Lab.a = 0;
	Lab.b = 0;
	cmsDoTransform(xform, &Lab, In, 1);
	if(In[0] != 0xFFFF || In[1] != 0x8080 || In[2] != 0x8080) 
		return 0;
	cmsDeleteTransform(xform);
	return 1;
}

static int32 CheckStoredIdentities(FILE * fOut)
{
	cmsHPROFILE hLab, hLink, h4, h2;
	cmsHTRANSFORM xform;
	int32 rc = 1;
	hLab  = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hLab, TYPE_Lab_8, hLab, TYPE_Lab_8, 0, 0);
	hLink = cmsTransform2DeviceLink(xform, 3.4, 0);
	cmsSaveProfileToFile(hLink, "abstractv2.icc");
	cmsCloseProfile(hLink);
	hLink = cmsTransform2DeviceLink(xform, 4.3, 0);
	cmsSaveProfileToFile(hLink, "abstractv4.icc");
	cmsCloseProfile(hLink);
	cmsDeleteTransform(xform);
	cmsCloseProfile(hLab);
	h4 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv4.icc", "r");
	xform = cmsCreateTransformTHR(DbgThread(), h4, TYPE_Lab_DBL, h4, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	SubTest(fOut, "V4");
	rc &= CheckSeveralLab(xform);
	cmsDeleteTransform(xform);
	cmsCloseProfile(h4);
	if(!rc) 
		goto Error;
	SubTest(fOut, "V2");
	h2 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv2.icc", "r");
	xform = cmsCreateTransformTHR(DbgThread(), h2, TYPE_Lab_DBL, h2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	rc &= CheckSeveralLab(xform);
	cmsDeleteTransform(xform);
	cmsCloseProfile(h2);
	if(!rc) 
		goto Error;
	SubTest(fOut, "V2 -> V4");
	h2 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv2.icc", "r");
	h4 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv4.icc", "r");
	xform = cmsCreateTransformTHR(DbgThread(), h4, TYPE_Lab_DBL, h2, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	rc &= CheckSeveralLab(xform);
	cmsDeleteTransform(xform);
	cmsCloseProfile(h2);
	cmsCloseProfile(h4);
	SubTest(fOut, "V4 -> V2");
	h2 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv2.icc", "r");
	h4 = cmsOpenProfileFromFileTHR(DbgThread(), "abstractv4.icc", "r");
	xform = cmsCreateTransformTHR(DbgThread(), h2, TYPE_Lab_DBL, h4, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	rc &= CheckSeveralLab(xform);
	cmsDeleteTransform(xform);
	cmsCloseProfile(h2);
	cmsCloseProfile(h4);
Error:
	remove("abstractv2.icc");
	remove("abstractv4.icc");
	return rc;
}

// Check a simple xform from a matrix profile to itself. Test floating point accuracy.
static int32 CheckMatrixShaperXFORMFloat()
{
	cmsHPROFILE hAbove, hSRGB;
	cmsHTRANSFORM xform;
	int32 rc1, rc2;
	hAbove = Create_AboveRGB();
	xform = cmsCreateTransformTHR(DbgThread(), hAbove, TYPE_RGB_FLT, hAbove, TYPE_RGB_FLT,  INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hAbove);
	rc1 = CheckFloatlinearXFORM(xform, 3);
	cmsDeleteTransform(xform);

	hSRGB = cmsCreate_sRGBProfileTHR(DbgThread());
	xform = cmsCreateTransformTHR(DbgThread(), hSRGB, TYPE_RGB_FLT, hSRGB, TYPE_RGB_FLT,  INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hSRGB);
	rc2 = CheckFloatlinearXFORM(xform, 3);
	cmsDeleteTransform(xform);

	return rc1 && rc2;
}

// Check a simple xform from a matrix profile to itself. Test 16 bits accuracy.
static int32 CheckMatrixShaperXFORM16()
{
	cmsHPROFILE hAbove, hSRGB;
	cmsHTRANSFORM xform;
	int32 rc1, rc2;

	hAbove = Create_AboveRGB();
	xform = cmsCreateTransformTHR(DbgThread(), hAbove, TYPE_RGB_16, hAbove, TYPE_RGB_16,  INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hAbove);

	rc1 = Check16linearXFORM(xform, 3);
	cmsDeleteTransform(xform);

	hSRGB = cmsCreate_sRGBProfileTHR(DbgThread());
	xform = cmsCreateTransformTHR(DbgThread(), hSRGB, TYPE_RGB_16, hSRGB, TYPE_RGB_16,  INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hSRGB);
	rc2 = Check16linearXFORM(xform, 3);
	cmsDeleteTransform(xform);

	return rc1 && rc2;
}

// Check a simple xform from a matrix profile to itself. Test 8 bits accuracy.
static int32 CheckMatrixShaperXFORM8()
{
	cmsHPROFILE hAbove, hSRGB;
	cmsHTRANSFORM xform;
	int32 rc1, rc2;

	hAbove = Create_AboveRGB();
	xform = cmsCreateTransformTHR(DbgThread(), hAbove, TYPE_RGB_8, hAbove, TYPE_RGB_8,  INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hAbove);
	rc1 = Check8linearXFORM(xform, 3);
	cmsDeleteTransform(xform);

	hSRGB = cmsCreate_sRGBProfileTHR(DbgThread());
	xform = cmsCreateTransformTHR(DbgThread(), hSRGB, TYPE_RGB_8, hSRGB, TYPE_RGB_8,  INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hSRGB);
	rc2 = Check8linearXFORM(xform, 3);
	cmsDeleteTransform(xform);

	return rc1 && rc2;
}

// TODO: Check LUT based to LUT based transforms for CMYK

// -----------------------------------------------------------------------------------------------------------------

// Check known values going from sRGB to XYZ
static int32 CheckOneRGB_f(cmsHTRANSFORM xform, int32 R, int32 G, int32 B,
    double X, double Y, double Z, double err)
{
	float RGB[3];
	double Out[3];
	RGB[0] = (float)(R / 255.0);
	RGB[1] = (float)(G / 255.0);
	RGB[2] = (float)(B / 255.0);
	cmsDoTransform(xform, RGB, Out, 1);
	return IsGoodVal("X", X, Out[0], err) && IsGoodVal("Y", Y, Out[1], err) && IsGoodVal("Z", Z, Out[2], err);
}

static int32 Chack_sRGB_Float()
{
	cmsHPROFILE hsRGB, hXYZ, hLab;
	cmsHTRANSFORM xform1, xform2;
	int32 rc;
	hsRGB = cmsCreate_sRGBProfileTHR(DbgThread());
	hXYZ  = cmsCreateXYZProfileTHR(DbgThread());
	hLab  = cmsCreateLab4ProfileTHR(DbgThread(), NULL);

	xform1 =  cmsCreateTransformTHR(DbgThread(), hsRGB, TYPE_RGB_FLT, hXYZ, TYPE_XYZ_DBL,
		INTENT_RELATIVE_COLORIMETRIC, 0);

	xform2 =  cmsCreateTransformTHR(DbgThread(), hsRGB, TYPE_RGB_FLT, hLab, TYPE_Lab_DBL,
		INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hsRGB);
	cmsCloseProfile(hXYZ);
	cmsCloseProfile(hLab);

	MaxErr = 0;

	// Xform 1 goes from 8 bits to XYZ,
	rc  = CheckOneRGB_f(xform1, 1, 1, 1,        0.0002927, 0.0003035,  0.000250,  0.0001);
	rc  &= CheckOneRGB_f(xform1, 127, 127, 127, 0.2046329, 0.212230,   0.175069,  0.0001);
	rc  &= CheckOneRGB_f(xform1, 12, 13, 15,    0.0038364, 0.0039928,  0.003853,  0.0001);
	rc  &= CheckOneRGB_f(xform1, 128, 0, 0,     0.0941240, 0.0480256,  0.003005,  0.0001);
	rc  &= CheckOneRGB_f(xform1, 190, 25, 210,  0.3204592, 0.1605926,  0.468213,  0.0001);

	// Xform 2 goes from 8 bits to Lab, we allow 0.01 error max
	rc  &= CheckOneRGB_f(xform2, 1, 1, 1,       0.2741748, 0, 0,                   0.01);
	rc  &= CheckOneRGB_f(xform2, 127, 127, 127, 53.192776, 0, 0,                   0.01);
	rc  &= CheckOneRGB_f(xform2, 190, 25, 210,  47.052136, 74.565610, -56.883274,  0.01);
	rc  &= CheckOneRGB_f(xform2, 128, 0, 0,     26.164701, 48.478171, 39.4384713,  0.01);

	cmsDeleteTransform(xform1);
	cmsDeleteTransform(xform2);
	return rc;
}

static boolint GetProfileRGBPrimaries(cmsHPROFILE hProfile, cmsCIEXYZTRIPLE * result, uint32 intent)
{
	cmsHPROFILE hXYZ;
	cmsHTRANSFORM hTransform;
	double rgb[3][3] = {{1., 0., 0.}, {0., 1., 0.}, {0., 0., 1.}};
	hXYZ = cmsCreateXYZProfile();
	if(hXYZ == NULL) return FALSE;
	hTransform = cmsCreateTransform(hProfile, TYPE_RGB_DBL, hXYZ, TYPE_XYZ_DBL, intent, cmsFLAGS_NOCACHE | cmsFLAGS_NOOPTIMIZE);
	cmsCloseProfile(hXYZ);
	if(hTransform == NULL) return FALSE;
	cmsDoTransform(hTransform, rgb, result, 3);
	cmsDeleteTransform(hTransform);
	return TRUE;
}

static int32 CheckRGBPrimaries()
{
	cmsHPROFILE hsRGB;
	cmsCIEXYZTRIPLE tripXYZ;
	cmsCIExyYTRIPLE tripxyY;
	boolint result;
	cmsSetAdaptationState(0);
	hsRGB = cmsCreate_sRGBProfileTHR(DbgThread());
	if(!hsRGB) 
		return 0;
	result = GetProfileRGBPrimaries(hsRGB, &tripXYZ, INTENT_ABSOLUTE_COLORIMETRIC);
	cmsCloseProfile(hsRGB);
	if(!result) 
		return 0;
	cmsXYZ2xyY(&tripxyY.Red, &tripXYZ.Red);
	cmsXYZ2xyY(&tripxyY.Green, &tripXYZ.Green);
	cmsXYZ2xyY(&tripxyY.Blue, &tripXYZ.Blue);
	// valus were taken from http://en.wikipedia.org/wiki/RGB_color_spaces#Specifications 
	if(!IsGoodFixed15_16("xRed", tripxyY.Red.x, 0.64) || !IsGoodFixed15_16("yRed", tripxyY.Red.y, 0.33) || !IsGoodFixed15_16("xGreen", tripxyY.Green.x, 0.30) ||
	    !IsGoodFixed15_16("yGreen", tripxyY.Green.y, 0.60) || !IsGoodFixed15_16("xBlue", tripxyY.Blue.x, 0.15) || !IsGoodFixed15_16("yBlue", tripxyY.Blue.y, 0.06)) {
		Fail("One or more primaries are wrong.");
		return FALSE;
	}
	return TRUE;
}
//
// This function will check CMYK -> CMYK transforms. It uses FOGRA29 and SWOP ICC profiles
//
static int32 CheckCMYK(int32 Intent, const char * pProfile1, const char * pProfile2)
{
	char file_name1[512];
	char file_name2[512];
	GetTestProfileName(pProfile1, file_name1, sizeof(file_name1));
	GetTestProfileName(pProfile2, file_name2, sizeof(file_name2));
	cmsHPROFILE hSWOP  = cmsOpenProfileFromFileTHR(DbgThread(), file_name1, "r");
	cmsHPROFILE hFOGRA = cmsOpenProfileFromFileTHR(DbgThread(), file_name2, "r");
	float CMYK1[4], CMYK2[4];
	cmsCIELab Lab1, Lab2;
	double DeltaL;
	int32 i;
	cmsHPROFILE hLab = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	cmsHTRANSFORM xform = cmsCreateTransformTHR(DbgThread(), hSWOP, TYPE_CMYK_FLT, hFOGRA, TYPE_CMYK_FLT, Intent, 0);
	cmsHTRANSFORM swop_lab = cmsCreateTransformTHR(DbgThread(), hSWOP,   TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, Intent, 0);
	cmsHTRANSFORM fogra_lab = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, Intent, 0);
	double Max = 0.0;
	for(i = 0; i <= 100; i++) {
		CMYK1[0] = 10;
		CMYK1[1] = 20;
		CMYK1[2] = 30;
		CMYK1[3] = (float)i;
		cmsDoTransform(swop_lab, CMYK1, &Lab1, 1);
		cmsDoTransform(xform, CMYK1, CMYK2, 1);
		cmsDoTransform(fogra_lab, CMYK2, &Lab2, 1);
		DeltaL = fabs(Lab1.L - Lab2.L);
		SETMAX(Max, DeltaL);
	}
	cmsDeleteTransform(xform);
	xform = cmsCreateTransformTHR(DbgThread(),  hFOGRA, TYPE_CMYK_FLT, hSWOP, TYPE_CMYK_FLT, Intent, 0);
	for(i = 0; i <= 100; i++) {
		CMYK1[0] = 10;
		CMYK1[1] = 20;
		CMYK1[2] = 30;
		CMYK1[3] = (float)i;
		cmsDoTransform(fogra_lab, CMYK1, &Lab1, 1);
		cmsDoTransform(xform, CMYK1, CMYK2, 1);
		cmsDoTransform(swop_lab, CMYK2, &Lab2, 1);
		DeltaL = fabs(Lab1.L - Lab2.L);
		SETMAX(Max, DeltaL);
	}
	cmsCloseProfile(hSWOP);
	cmsCloseProfile(hFOGRA);
	cmsCloseProfile(hLab);
	cmsDeleteTransform(xform);
	cmsDeleteTransform(swop_lab);
	cmsDeleteTransform(fogra_lab);
	return Max < 3.0;
}

static int32 CheckCMYKRoundtrip() { return CheckCMYK(INTENT_RELATIVE_COLORIMETRIC, "test1.icc", "test1.icc"); }
static int32 CheckCMYKPerceptual() { return CheckCMYK(INTENT_PERCEPTUAL, "test1.icc", "test2.icc"); }
static int32 CheckCMYKRelCol() { return CheckCMYK(INTENT_RELATIVE_COLORIMETRIC, "test1.icc", "test2.icc"); }

static int32 CheckKOnlyBlackPreserving()
{
	char file_name1[512];
	char file_name2[512];
	GetTestProfileName("test1.icc", file_name1, sizeof(file_name1));
	GetTestProfileName("test2.icc", file_name2, sizeof(file_name2));
	cmsHPROFILE hSWOP  = cmsOpenProfileFromFileTHR(DbgThread(), /*"test1.icc"*/file_name1, "r");
	cmsHPROFILE hFOGRA = cmsOpenProfileFromFileTHR(DbgThread(), /*"test2.icc"*/file_name2, "r");
	cmsHTRANSFORM xform, swop_lab, fogra_lab;
	float CMYK1[4], CMYK2[4];
	cmsCIELab Lab1, Lab2;
	cmsHPROFILE hLab;
	double DeltaL, Max;
	int32 i;
	hLab = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hSWOP, TYPE_CMYK_FLT, hFOGRA, TYPE_CMYK_FLT, INTENT_PRESERVE_K_ONLY_PERCEPTUAL, 0);
	swop_lab = cmsCreateTransformTHR(DbgThread(), hSWOP,   TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);
	fogra_lab = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);
	Max = 0;
	for(i = 0; i <= 100; i++) {
		CMYK1[0] = 0;
		CMYK1[1] = 0;
		CMYK1[2] = 0;
		CMYK1[3] = (float)i;
		cmsDoTransform(swop_lab, CMYK1, &Lab1, 1); // SWOP CMYK to Lab1
		cmsDoTransform(xform, CMYK1, CMYK2, 1); // SWOP To FOGRA using black preservation
		cmsDoTransform(fogra_lab, CMYK2, &Lab2, 1); // Obtained FOGRA CMYK to Lab2
		DeltaL = fabs(Lab1.L - Lab2.L); // We care only on L*
		SETMAX(Max, DeltaL);
	}
	cmsDeleteTransform(xform);
	// dL should be below 3.0
	// Same, but FOGRA to SWOP
	xform = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hSWOP, TYPE_CMYK_FLT, INTENT_PRESERVE_K_ONLY_PERCEPTUAL, 0);
	for(i = 0; i <= 100; i++) {
		CMYK1[0] = 0;
		CMYK1[1] = 0;
		CMYK1[2] = 0;
		CMYK1[3] = (float)i;
		cmsDoTransform(fogra_lab, CMYK1, &Lab1, 1);
		cmsDoTransform(xform, CMYK1, CMYK2, 1);
		cmsDoTransform(swop_lab, CMYK2, &Lab2, 1);
		DeltaL = fabs(Lab1.L - Lab2.L);
		SETMAX(Max, DeltaL);
	}
	cmsCloseProfile(hSWOP);
	cmsCloseProfile(hFOGRA);
	cmsCloseProfile(hLab);
	cmsDeleteTransform(xform);
	cmsDeleteTransform(swop_lab);
	cmsDeleteTransform(fogra_lab);
	return (Max < 3.0);
}

static int32 CheckKPlaneBlackPreserving()
{
	char file_name1[512];
	char file_name2[512];
	GetTestProfileName("test1.icc", file_name1, sizeof(file_name1));
	GetTestProfileName("test2.icc", file_name2, sizeof(file_name2));
	cmsHPROFILE hSWOP  = cmsOpenProfileFromFileTHR(DbgThread(), /*"test1.icc"*/file_name1, "r");
	cmsHPROFILE hFOGRA = cmsOpenProfileFromFileTHR(DbgThread(), /*"test2.icc"*/file_name2, "r");
	cmsHTRANSFORM xform, swop_lab, fogra_lab;
	float CMYK1[4], CMYK2[4];
	cmsCIELab Lab1, Lab2;
	cmsHPROFILE hLab;
	double DeltaE, Max;
	int32 i;
	hLab = cmsCreateLab4ProfileTHR(DbgThread(), NULL);
	xform = cmsCreateTransformTHR(DbgThread(), hSWOP, TYPE_CMYK_FLT, hFOGRA, TYPE_CMYK_FLT, INTENT_PERCEPTUAL, 0);
	swop_lab = cmsCreateTransformTHR(DbgThread(), hSWOP,  TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);
	fogra_lab = cmsCreateTransformTHR(DbgThread(), hFOGRA, TYPE_CMYK_FLT, hLab, TYPE_Lab_DBL, INTENT_PERCEPTUAL, 0);
	Max = 0;
	for(i = 0; i <= 100; i++) {
		CMYK1[0] = 0;
		CMYK1[1] = 0;
		CMYK1[2] = 0;
		CMYK1[3] = (float)i;
		cmsDoTransform(swop_lab, CMYK1, &Lab1, 1);
		cmsDoTransform(xform, CMYK1, CMYK2, 1);
		cmsDoTransform(fogra_lab, CMYK2, &Lab2, 1);
		DeltaE = cmsDeltaE(&Lab1, &Lab2);
		SETMAX(Max, DeltaE);
	}
	cmsDeleteTransform(xform);
	xform = cmsCreateTransformTHR(DbgThread(),  hFOGRA, TYPE_CMYK_FLT, hSWOP, TYPE_CMYK_FLT, INTENT_PRESERVE_K_PLANE_PERCEPTUAL, 0);
	for(i = 0; i <= 100; i++) {
		CMYK1[0] = 30;
		CMYK1[1] = 20;
		CMYK1[2] = 10;
		CMYK1[3] = (float)i;
		cmsDoTransform(fogra_lab, CMYK1, &Lab1, 1);
		cmsDoTransform(xform, CMYK1, CMYK2, 1);
		cmsDoTransform(swop_lab, CMYK2, &Lab2, 1);
		DeltaE = cmsDeltaE(&Lab1, &Lab2);
		SETMAX(Max, DeltaE);
	}
	cmsDeleteTransform(xform);
	cmsCloseProfile(hSWOP);
	cmsCloseProfile(hFOGRA);
	cmsCloseProfile(hLab);
	cmsDeleteTransform(swop_lab);
	cmsDeleteTransform(fogra_lab);
	return Max < 30.0;
}

static int32 CheckProofingXFORMFloat()
{
	int32 rc;
	cmsHPROFILE hAbove = Create_AboveRGB();
	cmsHTRANSFORM xform =  cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_FLT, hAbove, TYPE_RGB_FLT, hAbove, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_SOFTPROOFING);
	cmsCloseProfile(hAbove);
	rc = CheckFloatlinearXFORM(xform, 3);
	cmsDeleteTransform(xform);
	return rc;
}

static int32 CheckProofingXFORM16()
{
	int32 rc;
	cmsHPROFILE hAbove = Create_AboveRGB();
	cmsHTRANSFORM xform =  cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_16, hAbove, TYPE_RGB_16, hAbove, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_SOFTPROOFING|cmsFLAGS_NOCACHE);
	cmsCloseProfile(hAbove);
	rc = Check16linearXFORM(xform, 3);
	cmsDeleteTransform(xform);
	return rc;
}

static int32 CheckGamutCheck(FILE * fOut)
{
	cmsHPROFILE hSRGB, hAbove;
	cmsHTRANSFORM xform;
	int32 rc;
	uint16 Alarm[16] = { 0xDEAD, 0xBABE, 0xFACE };
	// Set alarm codes to fancy values so we could check the out of gamut condition
	cmsSetAlarmCodes(Alarm);
	// Create the profiles
	hSRGB  = cmsCreate_sRGBProfileTHR(DbgThread());
	hAbove = Create_AboveRGB();
	if(hSRGB == NULL || hAbove == NULL) 
		return 0;   // Failed
	SubTest(fOut, "Gamut check on floating point");
	// Create a gamut checker in the same space. No value should be out of gamut
	xform = cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_FLT, hAbove, TYPE_RGB_FLT, hAbove,
		INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_GAMUTCHECK);
	if(!CheckFloatlinearXFORM(xform, 3)) {
		cmsCloseProfile(hSRGB);
		cmsCloseProfile(hAbove);
		cmsDeleteTransform(xform);
		Fail("Gamut check on same profile failed");
		return 0;
	}
	cmsDeleteTransform(xform);
	SubTest(fOut, "Gamut check on 16 bits");
	xform = cmsCreateProofingTransformTHR(DbgThread(), hAbove, TYPE_RGB_16, hAbove, TYPE_RGB_16, hSRGB,
		INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_GAMUTCHECK);
	cmsCloseProfile(hSRGB);
	cmsCloseProfile(hAbove);
	rc = Check16linearXFORM(xform, 3);
	cmsDeleteTransform(xform);
	return rc;
}

static int32 CheckBlackPoint()
{
	cmsHPROFILE hProfile;
	cmsCIEXYZ Black;
	cmsCIELab Lab;
	char file_name[512];
	GetTestProfileName("test5.icc", file_name, sizeof(file_name));
	hProfile  = cmsOpenProfileFromFileTHR(DbgThread(), /*"test5.icc"*/file_name, "r");
	cmsDetectDestinationBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hProfile);

	GetTestProfileName("test1.icc", file_name, sizeof(file_name));
	hProfile = cmsOpenProfileFromFileTHR(DbgThread(), /*"test1.icc"*/file_name, "r");
	cmsDetectDestinationBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsXYZ2Lab(NULL, &Lab, &Black);
	cmsCloseProfile(hProfile);

	hProfile = cmsOpenProfileFromFileTHR(DbgThread(), "lcms2cmyk.icc", "r");
	cmsDetectDestinationBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsXYZ2Lab(NULL, &Lab, &Black);
	cmsCloseProfile(hProfile);

	GetTestProfileName("test2.icc", file_name, sizeof(file_name));
	hProfile = cmsOpenProfileFromFileTHR(DbgThread(), /*"test2.icc"*/file_name, "r");
	cmsDetectDestinationBlackPoint(&Black, hProfile, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsXYZ2Lab(NULL, &Lab, &Black);
	cmsCloseProfile(hProfile);

	GetTestProfileName("test1.icc", file_name, sizeof(file_name));
	hProfile = cmsOpenProfileFromFileTHR(DbgThread(), /*"test1.icc"*/file_name, "r");
	cmsDetectDestinationBlackPoint(&Black, hProfile, INTENT_PERCEPTUAL, 0);
	cmsXYZ2Lab(NULL, &Lab, &Black);
	cmsCloseProfile(hProfile);
	return 1;
}

static int32 CheckOneTAC(double InkLimit)
{
	double d;
	cmsHPROFILE h = CreateFakeCMYK(InkLimit, TRUE);
	cmsSaveProfileToFile(h, "lcmstac.icc");
	cmsCloseProfile(h);
	h = cmsOpenProfileFromFile("lcmstac.icc", "r");
	d = cmsDetectTAC(h);
	cmsCloseProfile(h);
	remove("lcmstac.icc");
	if(fabs(d - InkLimit) > 5) 
		return 0;
	return 1;
}

static int32 CheckTAC()
{
	if(!CheckOneTAC(180)) return 0;
	if(!CheckOneTAC(220)) return 0;
	if(!CheckOneTAC(286)) return 0;
	if(!CheckOneTAC(310)) return 0;
	if(!CheckOneTAC(330)) return 0;
	return 1;
}

#define NPOINTS_IT8 10  // (17*17*17*17)

static int32 CheckCGATS(FILE * fOut)
{
	cmsHANDLE it8;
	int32 i;
	SubTest(fOut, "IT8 creation");
	it8 = cmsIT8Alloc(DbgThread());
	if(it8 == NULL) 
		return 0;
	cmsIT8SetSheetType(it8, "LCMS/TESTING");
	cmsIT8SetPropertyStr(it8, "ORIGINATOR",   "1 2 3 4");
	cmsIT8SetPropertyUncooked(it8, "DESCRIPTOR",   "1234");
	cmsIT8SetPropertyStr(it8, "MANUFACTURER", "3");
	cmsIT8SetPropertyDbl(it8, "CREATED",      4);
	cmsIT8SetPropertyDbl(it8, "SERIAL",       5);
	cmsIT8SetPropertyHex(it8, "MATERIAL",     0x123);
	cmsIT8SetPropertyDbl(it8, "NUMBER_OF_SETS", NPOINTS_IT8);
	cmsIT8SetPropertyDbl(it8, "NUMBER_OF_FIELDS", 4);
	cmsIT8SetDataFormat(it8, 0, "SAMPLE_ID");
	cmsIT8SetDataFormat(it8, 1, "RGB_R");
	cmsIT8SetDataFormat(it8, 2, "RGB_G");
	cmsIT8SetDataFormat(it8, 3, "RGB_B");
	SubTest(fOut, "Table creation");
	for(i = 0; i < NPOINTS_IT8; i++) {
		char Patch[20];
		sprintf(Patch, "P%d", i);
		cmsIT8SetDataRowCol(it8, i, 0, Patch);
		cmsIT8SetDataRowColDbl(it8, i, 1, i);
		cmsIT8SetDataRowColDbl(it8, i, 2, i);
		cmsIT8SetDataRowColDbl(it8, i, 3, i);
	}
	SubTest(fOut, "Save to file");
	cmsIT8SaveToFile(it8, "TEST.IT8");
	cmsIT8Free(it8);
	SubTest(fOut, "Load from file");
	it8 = cmsIT8LoadFromFile(DbgThread(), "TEST.IT8");
	if(it8 == NULL) return 0;
	SubTest(fOut, "Save again file");
	cmsIT8SaveToFile(it8, "TEST.IT8");
	cmsIT8Free(it8);
	SubTest(fOut, "Load from file (II)");
	it8 = cmsIT8LoadFromFile(DbgThread(), "TEST.IT8");
	if(it8 == NULL) return 0;
	SubTest(fOut, "Change prop value");
	if(cmsIT8GetPropertyDbl(it8, "DESCRIPTOR") != 1234) {
		return 0;
	}
	cmsIT8SetPropertyDbl(it8, "DESCRIPTOR", 5678);
	if(cmsIT8GetPropertyDbl(it8, "DESCRIPTOR") != 5678) {
		return 0;
	}
	SubTest(fOut, "Positive numbers");
	if(cmsIT8GetDataDbl(it8, "P3", "RGB_G") != 3) {
		return 0;
	}
	SubTest(fOut, "Positive exponent numbers");
	cmsIT8SetPropertyDbl(it8, "DBL_PROP", 123E+12);
	if((cmsIT8GetPropertyDbl(it8, "DBL_PROP") - 123E+12) > 1) {
		return 0;
	}
	SubTest(fOut, "Negative exponent numbers");
	cmsIT8SetPropertyDbl(it8, "DBL_PROP_NEG", 123E-45);
	if((cmsIT8GetPropertyDbl(it8, "DBL_PROP_NEG") - 123E-45) > 1E-45) {
		return 0;
	}
	SubTest(fOut, "Negative numbers");
	cmsIT8SetPropertyDbl(it8, "DBL_NEG_VAL", -123);
	if((cmsIT8GetPropertyDbl(it8, "DBL_NEG_VAL")) != -123) {
		return 0;
	}
	cmsIT8Free(it8);
	remove("TEST.IT8");
	return 1;
}

static int32 CheckCGATS2()
{
	const uint8 junk[] = { 0x0, 0xd, 0xd, 0xa, 0x20, 0xd, 0x20, 0x20, 0x20, 0x3a, 0x31, 0x3d, 0x3d, 0x3d, 0x3d };
	cmsHANDLE handle = cmsIT8LoadFromMem(0, (const void *)junk, sizeof(junk));
	if(handle)
		cmsIT8Free(handle);
	return 1;
}

static int32 CheckCGATS_Overflow()
{
	const uint8 junk[] = { "@\nA 1.e2147483648\n" };
	cmsHANDLE handle = cmsIT8LoadFromMem(0, (const void *)junk, sizeof(junk));
	if(handle)
		cmsIT8Free(handle);
	return 1;
}

// Create CSA/CRD
static void GenerateCSA(const char * cInProf, const char * FileName)
{
	cmsHPROFILE hProfile;
	uint32 n;
	char * Buffer;
	cmsContext BuffThread = DbgThread();
	FILE * o = 0;
	if(cInProf == NULL)
		hProfile = cmsCreateLab4Profile(NULL);
	else
		hProfile = cmsOpenProfileFromFile(cInProf, "r");
	n = cmsGetPostScriptCSA(DbgThread(), hProfile, 0, 0, NULL, 0);
	if(!n) return;
	Buffer = (char *)_cmsMalloc(BuffThread, n + 1);
	cmsGetPostScriptCSA(DbgThread(), hProfile, 0, 0, Buffer, n);
	Buffer[n] = 0;
	if(FileName) {
		o = fopen(FileName, "wb");
		fwrite(Buffer, n, 1, o);
		fclose(o);
	}
	_cmsFree(BuffThread, Buffer);
	cmsCloseProfile(hProfile);
	if(FileName)
		remove(FileName);
}

static void GenerateCRD(const char * cOutProf, const char * FileName)
{
	cmsHPROFILE hProfile;
	uint32 n;
	char * Buffer;
	uint32 dwFlags = 0;
	cmsContext BuffThread = DbgThread();
	if(cOutProf == NULL)
		hProfile = cmsCreateLab4Profile(NULL);
	else
		hProfile = cmsOpenProfileFromFile(cOutProf, "r");
	n = cmsGetPostScriptCRD(DbgThread(), hProfile, 0, dwFlags, NULL, 0);
	if(!n) 
		return;
	Buffer = (char *)_cmsMalloc(BuffThread, n + 1);
	cmsGetPostScriptCRD(DbgThread(), hProfile, 0, dwFlags, Buffer, n);
	Buffer[n] = 0;
	if(FileName) {
		FILE* o = fopen(FileName, "wb");
		fwrite(Buffer, n, 1, o);
		fclose(o);
	}
	_cmsFree(BuffThread, Buffer);
	cmsCloseProfile(hProfile);
	if(FileName)
		remove(FileName);
}

static int32 CheckPostScript()
{
	char file_name[512];
	GetTestProfileName("test5.icc", file_name, sizeof(file_name));
	GenerateCSA(/*"test5.icc"*/file_name, "sRGB_CSA.ps");
	GenerateCSA("aRGBlcms2.icc", "aRGB_CSA.ps");
	GetTestProfileName("test4.icc", file_name, sizeof(file_name));
	GenerateCSA(/*"test4.icc"*/file_name, "sRGBV4_CSA.ps");
	GetTestProfileName("test1.icc", file_name, sizeof(file_name));
	GenerateCSA(/*"test1.icc"*/file_name, "SWOP_CSA.ps");
	GenerateCSA(NULL, "Lab_CSA.ps");
	GenerateCSA("graylcms2.icc", "gray_CSA.ps");
	GetTestProfileName("test5.icc", file_name, sizeof(file_name));
	GenerateCRD(/*"test5.icc"*/file_name, "sRGB_CRD.ps");
	GenerateCRD("aRGBlcms2.icc", "aRGB_CRD.ps");
	GenerateCRD(NULL, "Lab_CRD.ps");
	GetTestProfileName("test1.icc", file_name, sizeof(file_name));
	GenerateCRD(/*"test1.icc"*/file_name, "SWOP_CRD.ps");
	GetTestProfileName("test4.icc", file_name, sizeof(file_name));
	GenerateCRD(/*"test4.icc"*/file_name, "sRGBV4_CRD.ps");
	GenerateCRD("graylcms2.icc", "gray_CRD.ps");
	return 1;
}

static int32 CheckGray(cmsHTRANSFORM xform, uint8 g, double L)
{
	cmsCIELab Lab;
	cmsDoTransform(xform, &g, &Lab, 1);
	if(!IsGoodVal("a axis on gray", 0, Lab.a, 0.001)) 
		return 0;
	if(!IsGoodVal("b axis on gray", 0, Lab.b, 0.001)) 
		return 0;
	return IsGoodVal("Gray value", L, Lab.L, 0.01);
}

static int32 CheckInputGray()
{
	cmsHPROFILE hGray = Create_Gray22();
	cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
	cmsHTRANSFORM xform;
	if(hGray == NULL || hLab == NULL) 
		return 0;
	xform = cmsCreateTransform(hGray, TYPE_GRAY_8, hLab, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hGray); 
	cmsCloseProfile(hLab);
	if(!CheckGray(xform, 0, 0)) return 0;
	if(!CheckGray(xform, 125, 52.768)) return 0;
	if(!CheckGray(xform, 200, 81.069)) return 0;
	if(!CheckGray(xform, 255, 100.0)) return 0;
	cmsDeleteTransform(xform);
	return 1;
}

static int32 CheckLabInputGray()
{
	cmsHPROFILE hGray = Create_GrayLab();
	cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
	cmsHTRANSFORM xform;
	if(hGray == NULL || hLab == NULL) 
		return 0;
	xform = cmsCreateTransform(hGray, TYPE_GRAY_8, hLab, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hGray); 
	cmsCloseProfile(hLab);
	if(!CheckGray(xform, 0, 0)) return 0;
	if(!CheckGray(xform, 125, 49.019)) return 0;
	if(!CheckGray(xform, 200, 78.431)) return 0;
	if(!CheckGray(xform, 255, 100.0)) return 0;
	cmsDeleteTransform(xform);
	return 1;
}

static int32 CheckOutGray(cmsHTRANSFORM xform, double L, uint8 g)
{
	cmsCIELab Lab;
	uint8 g_out;
	Lab.L = L;
	Lab.a = 0;
	Lab.b = 0;
	cmsDoTransform(xform, &Lab, &g_out, 1);
	return IsGoodVal("Gray value", g, (double)g_out, 0.01);
}

static int32 CheckOutputGray()
{
	cmsHPROFILE hGray = Create_Gray22();
	cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
	cmsHTRANSFORM xform;
	if(hGray == NULL || hLab == NULL) return 0;
	xform = cmsCreateTransform(hLab, TYPE_Lab_DBL, hGray, TYPE_GRAY_8, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hGray); 
	cmsCloseProfile(hLab);
	if(!CheckOutGray(xform, 0, 0)) return 0;
	if(!CheckOutGray(xform, 100, 255)) return 0;
	if(!CheckOutGray(xform, 20, 52)) return 0;
	if(!CheckOutGray(xform, 50, 118)) return 0;
	cmsDeleteTransform(xform);
	return 1;
}

static int32 CheckLabOutputGray()
{
	cmsHPROFILE hGray = Create_GrayLab();
	cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
	cmsHTRANSFORM xform;
	int32 i;
	if(hGray == NULL || hLab == NULL) return 0;
	xform = cmsCreateTransform(hLab, TYPE_Lab_DBL, hGray, TYPE_GRAY_8, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(hGray); 
	cmsCloseProfile(hLab);
	if(!CheckOutGray(xform, 0, 0)) return 0;
	if(!CheckOutGray(xform, 100, 255)) return 0;
	for(i = 0; i < 100; i++) {
		uint8 g = (uint8)floor(i * 255.0 / 100.0 + 0.5);
		if(!CheckOutGray(xform, i, g)) return 0;
	}
	cmsDeleteTransform(xform);
	return 1;
}

static int32 CheckV4gamma()
{
	uint16 Lin[] = {0, 0xffff};
	cmsToneCurve * g = cmsBuildTabulatedToneCurve16(DbgThread(), 2, Lin);
	cmsHPROFILE h = cmsOpenProfileFromFileTHR(DbgThread(), "v4gamma.icc", "w");
	if(!h)
		return 0;
	cmsSetProfileVersion(h, 4.3);
	if(!cmsWriteTag(h, cmsSigGrayTRCTag, g)) 
		return 0;
	cmsCloseProfile(h);
	cmsFreeToneCurve(g);
	remove("v4gamma.icc");
	return 1;
}

// boolint cmsGBDdumpVRML(cmsHANDLE hGBD, const char * fname);

// Gamut descriptor routines
static int32 CheckGBD(FILE * fOut)
{
	cmsCIELab Lab;
	int32 L, a, b;
	uint32 r1, g1, b1;
	cmsHPROFILE hLab, hsRGB;
	cmsHTRANSFORM xform;
	cmsHANDLE h = cmsGBDAlloc(DbgThread());
	if(!h) 
		return 0;
	// Fill all Lab gamut as valid
	SubTest(fOut, "Filling RAW gamut");
	for(L = 0; L <= 100; L += 10)
		for(a = -128; a <= 128; a += 5)
			for(b = -128; b <= 128; b += 5) {
				Lab.L = L;
				Lab.a = a;
				Lab.b = b;
				if(!cmsGDBAddPoint(h, &Lab)) 
					return 0;
			}

	// Complete boundaries
	SubTest(fOut, "computing Lab gamut");
	if(!cmsGDBCompute(h, 0)) 
		return 0;
	// All points should be inside gamut
	SubTest(fOut, "checking Lab gamut");
	for(L = 10; L <= 90; L += 25)
		for(a = -120; a <= 120; a += 25)
			for(b = -120; b <= 120; b += 25) {
				Lab.L = L;
				Lab.a = a;
				Lab.b = b;
				if(!cmsGDBCheckPoint(h, &Lab)) {
					return 0;
				}
			}
	cmsGBDFree(h);
	// Now for sRGB
	SubTest(fOut, "checking sRGB gamut");
	h = cmsGBDAlloc(DbgThread());
	hsRGB = cmsCreate_sRGBProfile();
	hLab  = cmsCreateLab4Profile(NULL);
	xform = cmsCreateTransform(hsRGB, TYPE_RGB_8, hLab, TYPE_Lab_DBL, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hsRGB); 
	cmsCloseProfile(hLab);
	for(r1 = 0; r1 < 256; r1 += 5) {
		for(g1 = 0; g1 < 256; g1 += 5)
			for(b1 = 0; b1 < 256; b1 += 5) {
				uint8 rgb[3];
				rgb[0] = (uint8)r1;
				rgb[1] = (uint8)g1;
				rgb[2] = (uint8)b1;
				cmsDoTransform(xform, rgb, &Lab, 1);
				// if(fabs(Lab.b) < 20 && Lab.a > 0) continue;
				if(!cmsGDBAddPoint(h, &Lab)) {
					cmsGBDFree(h);
					return 0;
				}
			}
	}
	if(!cmsGDBCompute(h, 0)) 
		return 0;
	// cmsGBDdumpVRML(h, "c:\\colormaps\\lab.wrl");
	for(r1 = 10; r1 < 200; r1 += 10) {
		for(g1 = 10; g1 < 200; g1 += 10)
			for(b1 = 10; b1 < 200; b1 += 10) {
				uint8 rgb[3];
				rgb[0] = (uint8)r1;
				rgb[1] = (uint8)g1;
				rgb[2] = (uint8)b1;
				cmsDoTransform(xform, rgb, &Lab, 1);
				if(!cmsGDBCheckPoint(h, &Lab)) {
					cmsDeleteTransform(xform);
					cmsGBDFree(h);
					return 0;
				}
			}
	}
	cmsDeleteTransform(xform);
	cmsGBDFree(h);
	SubTest(fOut, "checking LCh chroma ring");
	h = cmsGBDAlloc(DbgThread());
	for(r1 = 0; r1 < 360; r1++) {
		cmsCIELCh LCh;
		LCh.L = 70;
		LCh.C = 60;
		LCh.h = r1;
		cmsLCh2Lab(&Lab, &LCh);
		if(!cmsGDBAddPoint(h, &Lab)) {
			cmsGBDFree(h);
			return 0;
		}
	}
	if(!cmsGDBCompute(h, 0)) 
		return 0;
	cmsGBDFree(h);
	return 1;
}

static int32 CheckMD5()
{
	cmsHPROFILE pProfile = cmsOpenProfileFromFile("sRGBlcms2.icc", "r");
	cmsProfileID ProfileID1, ProfileID2, ProfileID3, ProfileID4;
	_cmsICCPROFILE* h = (_cmsICCPROFILE*)pProfile;
	if(cmsMD5computeID(pProfile)) 
		cmsGetHeaderProfileID(pProfile, ProfileID1.ID8);
	if(cmsMD5computeID(pProfile)) 
		cmsGetHeaderProfileID(pProfile, ProfileID2.ID8);
	cmsCloseProfile(pProfile);
	pProfile = cmsOpenProfileFromFile("sRGBlcms2.icc", "r");
	h = (_cmsICCPROFILE*)pProfile;
	if(cmsMD5computeID(pProfile)) 
		cmsGetHeaderProfileID(pProfile, ProfileID3.ID8);
	if(cmsMD5computeID(pProfile)) 
		cmsGetHeaderProfileID(pProfile, ProfileID4.ID8);
	cmsCloseProfile(pProfile);
	return ((memcmp(ProfileID1.ID8, ProfileID3.ID8, sizeof(ProfileID1)) == 0) && (memcmp(ProfileID2.ID8, ProfileID4.ID8, sizeof(ProfileID2)) == 0));
}

static int32 CheckLinking()
{
	cmsPipeline * pipeline;
	cmsStage * stageBegin, * stageEnd;
	// Create a CLUT based profile
	cmsHPROFILE h = cmsCreateInkLimitingDeviceLinkTHR(DbgThread(), cmsSigCmykData, 150);
	// link a second tag
	cmsLinkTag(h, cmsSigAToB1Tag, cmsSigAToB0Tag);
	// Save the linked devicelink
	if(!cmsSaveProfileToFile(h, "lcms2link.icc")) 
		return 0;
	cmsCloseProfile(h);
	// Now open the profile and read the pipeline
	h = cmsOpenProfileFromFile("lcms2link.icc", "r");
	if(!h) 
		return 0;
	pipeline = (cmsPipeline *)cmsReadTag(h, cmsSigAToB1Tag);
	if(pipeline == NULL) {
		return 0;
	}
	pipeline = cmsPipelineDup(pipeline);
	// extract stage from pipe line
	cmsPipelineUnlinkStage(pipeline, cmsAT_BEGIN, &stageBegin);
	cmsPipelineUnlinkStage(pipeline, cmsAT_END,   &stageEnd);
	cmsPipelineInsertStage(pipeline, cmsAT_END,    stageEnd);
	cmsPipelineInsertStage(pipeline, cmsAT_BEGIN,  stageBegin);
	if(cmsTagLinkedTo(h, cmsSigAToB1Tag) != cmsSigAToB0Tag) 
		return 0;
	cmsWriteTag(h, cmsSigAToB0Tag, pipeline);
	cmsPipelineFree(pipeline);
	if(!cmsSaveProfileToFile(h, "lcms2link2.icc")) 
		return 0;
	cmsCloseProfile(h);
	return 1;
}

//  TestMPE
//
//  Created by Paul Miller on 30/08/2016.
//
static cmsHPROFILE IdentityMatrixProfile(cmsColorSpaceSignature dataSpace)
{
	cmsContext ctx = 0;
	cmsVEC3 zero = {{0, 0, 0}};
	cmsMAT3 identity;
	cmsPipeline * forward;
	cmsPipeline * reverse;
	cmsHPROFILE identityProfile = cmsCreateProfilePlaceholder(ctx);
	cmsSetProfileVersion(identityProfile, 4.3);
	cmsSetDeviceClass(identityProfile,     cmsSigColorSpaceClass);
	cmsSetColorSpace(identityProfile,       dataSpace);
	cmsSetPCS(identityProfile,              cmsSigXYZData);
	cmsSetHeaderRenderingIntent(identityProfile,  INTENT_RELATIVE_COLORIMETRIC);
	cmsWriteTag(identityProfile, cmsSigMediaWhitePointTag, cmsD50_XYZ());
	_cmsMAT3identity(&identity);
	// build forward transform.... (RGB to PCS)
	forward = cmsPipelineAlloc(0, 3, 3);
	cmsPipelineInsertStage(forward, cmsAT_END, cmsStageAllocMatrix(ctx, 3, 3, (double *)&identity, (double *)&zero));
	cmsWriteTag(identityProfile, cmsSigDToB1Tag, forward);
	cmsPipelineFree(forward);
	reverse = cmsPipelineAlloc(0, 3, 3);
	cmsPipelineInsertStage(reverse, cmsAT_END, cmsStageAllocMatrix(ctx, 3, 3, (double *)&identity, (double *)&zero));
	cmsWriteTag(identityProfile, cmsSigBToD1Tag, reverse);
	cmsPipelineFree(reverse);
	return identityProfile;
}

static int32 CheckFloatXYZ()
{
	cmsHPROFILE input;
	cmsHPROFILE xyzProfile = cmsCreateXYZProfile();
	cmsHTRANSFORM xform;
	float in[4];
	float out[4];
	in[0] = 1.0;
	in[1] = 1.0;
	in[2] = 1.0;
	in[3] = 0.5;
	// RGB to XYZ
	input = IdentityMatrixProfile(cmsSigRgbData);
	xform = cmsCreateTransform(input, TYPE_RGB_FLT, xyzProfile, TYPE_XYZ_FLT, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(input);
	cmsDoTransform(xform, in, out, 1);
	cmsDeleteTransform(xform);
	if(!IsGoodVal("Float RGB->XYZ", in[0], out[0], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float RGB->XYZ", in[1], out[1], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float RGB->XYZ", in[2], out[2], FLOAT_PRECISSION))
		return 0;

	// XYZ to XYZ
	input = IdentityMatrixProfile(cmsSigXYZData);
	xform = cmsCreateTransform(input, TYPE_XYZ_FLT, xyzProfile, TYPE_XYZ_FLT, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(input);
	cmsDoTransform(xform, in, out, 1);
	cmsDeleteTransform(xform);
	if(!IsGoodVal("Float XYZ->XYZ", in[0], out[0], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZ->XYZ", in[1], out[1], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZ->XYZ", in[2], out[2], FLOAT_PRECISSION))
		return 0;
	input = IdentityMatrixProfile(cmsSigXYZData);
#define TYPE_XYZA_FLT          (FLOAT_SH(1)|COLORSPACE_SH(PT_XYZ)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(4))
	xform = cmsCreateTransform(input, TYPE_XYZA_FLT, xyzProfile, TYPE_XYZA_FLT, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_COPY_ALPHA);
	cmsCloseProfile(input);
	cmsDoTransform(xform, in, out, 1);
	cmsDeleteTransform(xform);
	if(!IsGoodVal("Float XYZA->XYZA", in[0], out[0], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZA->XYZA", in[1], out[1], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZA->XYZA", in[2], out[2], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZA->XYZA", in[3], out[3], FLOAT_PRECISSION))
		return 0;

	// XYZ to RGB
	input = IdentityMatrixProfile(cmsSigRgbData);
	xform = cmsCreateTransform(xyzProfile, TYPE_XYZ_FLT, input, TYPE_RGB_FLT, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(input);
	cmsDoTransform(xform, in, out, 1);
	cmsDeleteTransform(xform);
	if(!IsGoodVal("Float XYZ->RGB", in[0], out[0], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZ->RGB", in[1], out[1], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float XYZ->RGB", in[2], out[2], FLOAT_PRECISSION))
		return 0;
	// Now the optimizer should remove a stage
	// XYZ to RGB
	input = IdentityMatrixProfile(cmsSigRgbData);
	xform = cmsCreateTransform(input, TYPE_RGB_FLT, input, TYPE_RGB_FLT, INTENT_RELATIVE_COLORIMETRIC, 0);
	cmsCloseProfile(input);
	cmsDoTransform(xform, in, out, 1);
	cmsDeleteTransform(xform);
	if(!IsGoodVal("Float RGB->RGB", in[0], out[0], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float RGB->RGB", in[1], out[1], FLOAT_PRECISSION) ||
	    !IsGoodVal("Float RGB->RGB", in[2], out[2], FLOAT_PRECISSION))
		return 0;
	cmsCloseProfile(xyzProfile);
	return 1;
}
/*
   Bug reported

        1)
        sRGB built-in V4.3 -> Lab identity built-in V4.3
        Flags: "cmsFLAGS_NOCACHE", "cmsFLAGS_NOOPTIMIZE"
        Input format: TYPE_RGBA_FLT
        Output format: TYPE_LabA_FLT

        2) and back
        Lab identity built-in V4.3 -> sRGB built-in V4.3
        Flags: "cmsFLAGS_NOCACHE", "cmsFLAGS_NOOPTIMIZE"
        Input format: TYPE_LabA_FLT
        Output format: TYPE_RGBA_FLT

 */
static int32 ChecksRGB2LabFLT()
{
	cmsHPROFILE hSRGB = cmsCreate_sRGBProfile();
	cmsHPROFILE hLab  = cmsCreateLab4Profile(NULL);
	cmsHTRANSFORM xform1 = cmsCreateTransform(hSRGB, TYPE_RGBA_FLT, hLab, TYPE_LabA_FLT, 0, cmsFLAGS_NOCACHE|cmsFLAGS_NOOPTIMIZE);
	cmsHTRANSFORM xform2 = cmsCreateTransform(hLab, TYPE_LabA_FLT, hSRGB, TYPE_RGBA_FLT, 0, cmsFLAGS_NOCACHE|cmsFLAGS_NOOPTIMIZE);
	float RGBA1[4], RGBA2[4], LabA[4];
	int i;
	for(i = 0; i <= 100; i++) {
		RGBA1[0] = i / 100.0F;
		RGBA1[1] = i / 100.0F;
		RGBA1[2] = i / 100.0F;
		RGBA1[3] = 0;
		cmsDoTransform(xform1, RGBA1, LabA,  1);
		cmsDoTransform(xform2, LabA, RGBA2, 1);
		if(!IsGoodVal("Float RGB->RGB", RGBA1[0], RGBA2[0], FLOAT_PRECISSION) ||
		    !IsGoodVal("Float RGB->RGB", RGBA1[1], RGBA2[1], FLOAT_PRECISSION) ||
		    !IsGoodVal("Float RGB->RGB", RGBA1[2], RGBA2[2], FLOAT_PRECISSION))
			return 0;
	}
	cmsDeleteTransform(xform1);
	cmsDeleteTransform(xform2);
	cmsCloseProfile(hSRGB);
	cmsCloseProfile(hLab);
	return 1;
}
/*
 * parametric curve for Rec709
 */
static double Rec709(double L)
{
	if(L <0.018) 
		return 4.5*L;
	else {
		double a = 1.099* pow(L, 0.45);
		a = a - 0.099;
		return a;
	}
}

static int32 CheckParametricRec709()
{
	double params[7];
	cmsToneCurve * t;
	int i;
	params[0] = 0.45; /* y */
	params[1] = pow(1.099, 1.0 / 0.45); /* a */
	params[2] = 0.0; /* b */
	params[3] = 4.5; /* c */
	params[4] = 0.018; /* d */
	params[5] = -0.099; /* e */
	params[6] = 0.0; /* f */
	t = cmsBuildParametricToneCurve(NULL, 5, params);
	for(i = 0; i < 256; i++) {
		float n = (float)i / 255.0F;
		uint16 f1 = (uint16)floor(255.0 * cmsEvalToneCurveFloat(t, n) + 0.5);
		uint16 f2 = (uint16)floor(255.0*Rec709((double)i / 255.0) + 0.5);
		if(f1 != f2) {
			cmsFreeToneCurve(t);
			return 0;
		}
	}
	cmsFreeToneCurve(t);
	return 1;
}

#define kNumPoints  10

typedef float (* Function)(float x);

static float StraightLine(float x) { return (float)(0.1 + 0.9 * x); }

static int32 TestCurve(const char * label, cmsToneCurve * curve, Function fn)
{
	int32 ok = 1;
	for(int i = 0; i < kNumPoints*3; i++) {
		float x = (float)i / (kNumPoints*3 - 1);
		float expectedY = fn(x);
		float out = cmsEvalToneCurveFloat(curve, x);
		if(!IsGoodVal(label, expectedY, out, FLOAT_PRECISSION)) {
			ok = 0;
		}
	}
	return ok;
}

static int32 CheckFloatSamples()
{
	float y[kNumPoints];
	cmsToneCurve * curve;
	int32 ok;
	for(int i = 0; i < kNumPoints; i++) {
		float x = (float)i / (kNumPoints-1);
		y[i] = StraightLine(x);
	}
	curve = cmsBuildTabulatedToneCurveFloat(NULL, kNumPoints, y);
	ok = TestCurve("Float Samples", curve, StraightLine);
	cmsFreeToneCurve(curve);
	return ok;
}

static int32 CheckFloatSegments()
{
	int32 ok = 1;
	int i;
	cmsToneCurve * curve;
	float y[ kNumPoints];
	// build a segmented curve with a sampled section...
	cmsCurveSegment Seg[3];
	// Initialize segmented curve part up to 0.1
	Seg[0].x0 = -1e22f;  // -infinity
	Seg[0].x1 = 0.1f;
	Seg[0].Type = 6;         // Y = (a * X + b) ^ Gamma + c
	Seg[0].Params[0] = 1.0f; // gamma
	Seg[0].Params[1] = 0.9f; // a
	Seg[0].Params[2] = 0.0f;    // b
	Seg[0].Params[3] = 0.1f; // c
	Seg[0].Params[4] = 0.0f;
	// From zero to 1
	Seg[1].x0 = 0.1f;
	Seg[1].x1 = 0.9f;
	Seg[1].Type = 0;
	Seg[1].nGridPoints = kNumPoints;
	Seg[1].SampledPoints = y;
	for(i = 0; i < kNumPoints; i++) {
		float x = (float)(0.1 + ((float)i / (kNumPoints-1)) * (0.9 - 0.1));
		y[i] = StraightLine(x);
	}
	// from 1 to +infinity
	Seg[2].x0 = 0.9f;
	Seg[2].x1 = 1e22f; // +infinity
	Seg[2].Type = 6;

	Seg[2].Params[0] = 1.0f;
	Seg[2].Params[1] = 0.9f;
	Seg[2].Params[2] = 0.0f;
	Seg[2].Params[3] = 0.1f;
	Seg[2].Params[4] = 0.0f;
	curve = cmsBuildSegmentedToneCurve(0, 3, Seg);
	ok = TestCurve("Float Segmented Curve", curve, StraightLine);
	cmsFreeToneCurve(curve);
	return ok;
}

static int32 CheckReadRAW(FILE * fOut)
{
	int32 tag_size, tag_size1;
	char buffer[4];
	cmsHPROFILE hProfile;
	char file_name[512];
	SubTest(fOut, "RAW read on on-disk");
	GetTestProfileName("test1.icc", file_name, sizeof(file_name));
	hProfile = cmsOpenProfileFromFile(/*"test1.icc"*/file_name, "r");
	if(hProfile == NULL)
		return 0;
	tag_size = cmsReadRawTag(hProfile, cmsSigGamutTag, buffer, 4);
	tag_size1 = cmsReadRawTag(hProfile, cmsSigGamutTag, NULL, 0);
	cmsCloseProfile(hProfile);
	if(tag_size != 4)
		return 0;
	if(tag_size1 != 37009)
		return 0;
	SubTest(fOut, "RAW read on in-memory created profiles");
	hProfile = cmsCreate_sRGBProfile();
	tag_size = cmsReadRawTag(hProfile, cmsSigGreenColorantTag, buffer, 4);
	tag_size1 = cmsReadRawTag(hProfile, cmsSigGreenColorantTag, NULL, 0);
	cmsCloseProfile(hProfile);
	if(tag_size != 4)
		return 0;
	if(tag_size1 != 20)
		return 0;
	return 1;
}

static int32 CheckMeta()
{
	char * data;
	cmsHANDLE dict;
	uint32 clen;
	FILE * fp;
	int rc;
	/* open file */
	char file_name[512];
	GetTestProfileName("ibm-t61.icc", file_name, sizeof(file_name));
	cmsHPROFILE p = cmsOpenProfileFromFile(/*"ibm-t61.icc"*/file_name, "r");
	if(!p) 
		return 0;
	/* read dictionary, but don't do anything with the value */
	//COMMENT OUT THE NEXT TWO LINES AND IT WORKS FINE!!!
	dict = cmsReadTag(p, cmsSigMetaTag);
	if(dict == NULL) 
		return 0;
	/* serialize profile to memory */
	rc = cmsSaveProfileToMem(p, NULL, &clen);
	if(!rc) 
		return 0;
	data = static_cast<char *>(SAlloc::M(clen));
	rc = cmsSaveProfileToMem(p, data, &clen);
	if(!rc) 
		return 0;
	/* write the memory blob to a file */
	//NOTE: The crash does not happen if cmsSaveProfileToFile() is used */
	fp = fopen("new.icc", "wb");
	fwrite(data, 1, clen, fp);
	fclose(fp);
	SAlloc::F(data);
	cmsCloseProfile(p);
	/* open newly created file and read metadata */
	p = cmsOpenProfileFromFile("new.icc", "r");
	//ERROR: Bad dictionary Name/Value
	//ERROR: Corrupted tag 'meta'
	//test: test.c:59: main: Assertion `dict' failed.
	dict = cmsReadTag(p, cmsSigMetaTag);
	if(dict == NULL) 
		return 0;
	cmsCloseProfile(p);
	return 1;
}

// Bug on applying null transforms on floating point buffers
static int32 CheckFloatNULLxform()
{
	int i;
	float in[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	float out[10];
	cmsHTRANSFORM xform = cmsCreateTransform(NULL, TYPE_GRAY_FLT, NULL, TYPE_GRAY_FLT, INTENT_PERCEPTUAL, cmsFLAGS_NULLTRANSFORM);
	if(!xform) {
		Fail("Unable to create float null transform");
		return 0;
	}
	cmsDoTransform(xform, in, out, 10);
	cmsDeleteTransform(xform);
	for(i = 0; i < 10; i++) {
		if(!IsGoodVal("float nullxform", in[i], out[i], 0.001)) {
			return 0;
		}
	}
	return 1;
}

static int32 CheckRemoveTag()
{
	cmsHPROFILE p = cmsCreate_sRGBProfileTHR(NULL);
	/* set value */
	cmsMLU * mlu = cmsMLUalloc(NULL, 1);
	int ret = cmsMLUsetASCII(mlu, "en", "US", "bar");
	if(!ret) 
		return 0;
	ret = cmsWriteTag(p, cmsSigDeviceMfgDescTag, mlu);
	if(!ret) 
		return 0;
	cmsMLUfree(mlu);
	/* remove the tag  */
	ret = cmsWriteTag(p, cmsSigDeviceMfgDescTag, NULL);
	if(!ret) 
		return 0;
	/* THIS EXPLODES */
	cmsCloseProfile(p);
	return 1;
}

static int32 CheckMatrixSimplify()
{
	cmsHPROFILE pIn;
	cmsHPROFILE pOut;
	cmsHTRANSFORM t;
	uchar buf[3] = { 127, 32, 64 };
	char file_name[512];
	GetTestProfileName("ibm-t61.icc", file_name, sizeof(file_name));
	pIn = cmsCreate_sRGBProfile();
	pOut = cmsOpenProfileFromFile(file_name/*"ibm-t61.icc"*/, "r");
	if(pIn == NULL || pOut == NULL)
		return 0;
	t = cmsCreateTransform(pIn, TYPE_RGB_8, pOut, TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
	cmsDoTransformStride(t, buf, buf, 1, 1);
	cmsDeleteTransform(t);
	cmsCloseProfile(pIn);
	cmsCloseProfile(pOut);
	return buf[0] == 144 && buf[1] == 0 && buf[2] == 69;
}

static int32 CheckTransformLineStride()
{
	cmsHPROFILE pIn;
	cmsHPROFILE pOut;
	cmsHTRANSFORM t;
	// Our buffer is formed by 4 RGB8 lines, each line is 2 pixels wide plus a padding of one byte
	uint8 buf1[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,
				  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,
				  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0,
				  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, };

	// Our buffer2 is formed by 4 RGBA lines, each line is 2 pixels wide plus a padding of one byte

	uint8 buf2[] = { 0xff, 0xff, 0xff, 1, 0xff, 0xff, 0xff, 1, 0,
				  0xff, 0xff, 0xff, 1, 0xff, 0xff, 0xff, 1, 0,
				  0xff, 0xff, 0xff, 1, 0xff, 0xff, 0xff, 1, 0,
				  0xff, 0xff, 0xff, 1, 0xff, 0xff, 0xff, 1, 0};

	// Our buffer3 is formed by 4 RGBA16 lines, each line is 2 pixels wide plus a padding of two bytes

	uint16 buf3[] = { 0xffff, 0xffff, 0xffff, 0x0101, 0xffff, 0xffff, 0xffff, 0x0101, 0,
				   0xffff, 0xffff, 0xffff, 0x0101, 0xffff, 0xffff, 0xffff, 0x0101, 0,
				   0xffff, 0xffff, 0xffff, 0x0101, 0xffff, 0xffff, 0xffff, 0x0101, 0,
				   0xffff, 0xffff, 0xffff, 0x0101, 0xffff, 0xffff, 0xffff, 0x0101, 0 };

	uint8 out[1024];
	char file_name[512];
	GetTestProfileName("ibm-t61.icc", file_name, sizeof(file_name));
	memzero(out, sizeof(out));
	pIn = cmsCreate_sRGBProfile();
	pOut = cmsOpenProfileFromFile(file_name/*"ibm-t61.icc"*/, "r");
	if(pIn == NULL || pOut == NULL)
		return 0;
	t = cmsCreateTransform(pIn, TYPE_RGB_8, pOut, TYPE_RGB_8, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA);
	cmsDoTransformLineStride(t, buf1, out, 2, 4, 7, 7, 0, 0);
	cmsDeleteTransform(t);
	if(memcmp(out, buf1, sizeof(buf1)) != 0) {
		Fail("Failed transform line stride on RGB8");
		cmsCloseProfile(pIn);
		cmsCloseProfile(pOut);
		return 0;
	}
	memzero(out, sizeof(out));
	t = cmsCreateTransform(pIn, TYPE_RGBA_8, pOut, TYPE_RGBA_8, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA);
	cmsDoTransformLineStride(t, buf2, out, 2, 4, 9, 9, 0, 0);
	cmsDeleteTransform(t);
	if(memcmp(out, buf2, sizeof(buf2)) != 0) {
		cmsCloseProfile(pIn);
		cmsCloseProfile(pOut);
		Fail("Failed transform line stride on RGBA8");
		return 0;
	}
	memzero(out, sizeof(out));
	t = cmsCreateTransform(pIn, TYPE_RGBA_16, pOut, TYPE_RGBA_16, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA);
	cmsDoTransformLineStride(t, buf3, out, 2, 4, 18, 18, 0, 0);
	cmsDeleteTransform(t);
	if(memcmp(out, buf3, sizeof(buf3)) != 0) {
		cmsCloseProfile(pIn);
		cmsCloseProfile(pOut);
		Fail("Failed transform line stride on RGBA16");
		return 0;
	}
	memzero(out, sizeof(out));
	// From 8 to 16
	t = cmsCreateTransform(pIn, TYPE_RGBA_8, pOut, TYPE_RGBA_16, INTENT_PERCEPTUAL, cmsFLAGS_COPY_ALPHA);
	cmsDoTransformLineStride(t, buf2, out, 2, 4, 9, 18, 0, 0);
	cmsDeleteTransform(t);
	if(memcmp(out, buf3, sizeof(buf3)) != 0) {
		cmsCloseProfile(pIn);
		cmsCloseProfile(pOut);
		Fail("Failed transform line stride on RGBA16");
		return 0;
	}
	cmsCloseProfile(pIn);
	cmsCloseProfile(pOut);
	return 1;
}

static int32 CheckPlanar8opt()
{
	cmsHPROFILE aboveRGB = Create_AboveRGB();
	cmsHPROFILE sRGB = cmsCreate_sRGBProfile();
	cmsHTRANSFORM transform = cmsCreateTransform(sRGB, TYPE_RGB_8_PLANAR, aboveRGB, TYPE_RGB_8_PLANAR, INTENT_PERCEPTUAL, 0);
	cmsDeleteTransform(transform);
	cmsCloseProfile(aboveRGB);
	cmsCloseProfile(sRGB);
	return 1;
}
/**
 * Bug reported & fixed. Thanks to Kornel Lesinski for spotting this.
 */
static int32 CheckSE()
{
	cmsHPROFILE input_profile = Create_AboveRGB();
	cmsHPROFILE output_profile = cmsCreate_sRGBProfile();
	cmsHTRANSFORM tr = cmsCreateTransform(input_profile, TYPE_RGBA_8, output_profile, TYPE_RGBA_16_SE, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_COPY_ALPHA);
	uint8 rgba[4] = { 40, 41, 41, 0xfa };
	uint16 out[4];
	cmsDoTransform(tr, rgba, out, 1);
	cmsCloseProfile(input_profile);
	cmsCloseProfile(output_profile);
	cmsDeleteTransform(tr);
	if(out[0] != 0xf622 || out[1] != 0x7f24 || out[2] != 0x7f24)
		return 0;
	return 1;
}
/**
 * Bug reported.
 */
static int32 CheckForgedMPE()
{
	uint32 i;
	cmsHPROFILE dstProfile;
	cmsColorSpaceSignature srcCS;
	uint32 nSrcComponents;
	uint32 srcFormat;
	uint32 intent = 0;
	uint32 flags = 0;
	cmsHTRANSFORM hTransform;
	uint8 output[4];
	char file_name[512];
	GetTestProfileName("bad_mpe.icc", file_name, sizeof(file_name));
	cmsHPROFILE srcProfile = cmsOpenProfileFromFile(/*"bad_mpe.icc"*/file_name, "r");
	if(!srcProfile)
		return 0;
	dstProfile = cmsCreate_sRGBProfile();
	if(!dstProfile) {
		cmsCloseProfile(srcProfile);
		return 0;
	}
	srcCS = cmsGetColorSpace(srcProfile);
	nSrcComponents = cmsChannelsOf(srcCS);
	if(srcCS == cmsSigLabData) {
		srcFormat = COLORSPACE_SH(PT_Lab) | CHANNELS_SH(nSrcComponents) | BYTES_SH(0);
	}
	else {
		srcFormat = COLORSPACE_SH(PT_ANY) | CHANNELS_SH(nSrcComponents) | BYTES_SH(1);
	}
	cmsSetLogErrorHandler(ErrorReportingFunction);
	hTransform = cmsCreateTransform(srcProfile, srcFormat, dstProfile, TYPE_BGR_8, intent, flags);
	cmsCloseProfile(srcProfile);
	cmsCloseProfile(dstProfile);
	cmsSetLogErrorHandler(FatalErrorQuit);
	// Should report error
	if(!TrappedError) return 0;
	TrappedError = FALSE;
	// Transform should NOT be created
	if(!hTransform) return 1;
	// Never should reach here
	if(T_BYTES(srcFormat) == 0) { // 0 means double
		double input[128];
		for(i = 0; i < nSrcComponents; i++)
			input[i] = 0.5f;
		cmsDoTransform(hTransform, input, output, 1);
	}
	else {
		uint8 input[128];
		for(i = 0; i < nSrcComponents; i++)
			input[i] = 128;
		cmsDoTransform(hTransform, input, output, 1);
	}
	cmsDeleteTransform(hTransform);
	return 0;
}
/**
 * What the self test is trying to do is creating a proofing transform
 * with gamut check, so we can getting the coverage of one profile of
 * another, i.e. to approximate the gamut intersection. e.g.
 * Thanks to Richard Hughes for providing the test
 */
static int32 CheckProofingIntersection()
{
	cmsHPROFILE hnd1 = cmsCreate_sRGBProfile();
	cmsHPROFILE hnd2 = Create_AboveRGB();
	cmsHPROFILE profile_null = cmsCreateNULLProfileTHR(DbgThread());
	cmsHTRANSFORM transform = cmsCreateProofingTransformTHR(DbgThread(), hnd1, TYPE_RGB_FLT, profile_null, TYPE_GRAY_FLT, hnd2,
		INTENT_ABSOLUTE_COLORIMETRIC, INTENT_ABSOLUTE_COLORIMETRIC, cmsFLAGS_GAMUTCHECK|cmsFLAGS_SOFTPROOFING);
	cmsCloseProfile(hnd1);
	cmsCloseProfile(hnd2);
	cmsCloseProfile(profile_null);
	// Failed?
	if(transform == NULL) 
		return 0;
	cmsDeleteTransform(transform);
	return 1;
}
//
// PERFORMANCE CHECKS
//
typedef struct {uint8 r, g, b, a;}    Scanline_rgba8;
typedef struct {uint16 r, g, b, a;}   Scanline_rgba16;
typedef struct {float r, g, b, a;}  Scanline_rgba32;
typedef struct {uint8 r, g, b;}       Scanline_rgb8;
typedef struct {uint16 r, g, b;}      Scanline_rgb16;
typedef struct {float r, g, b;}     Scanline_rgb32;

static void TitlePerformance(FILE * fOut, const char * Txt)
{
	fprintf(fOut, "%-45s: ", Txt); 
	fflush(fOut);
}

static void PrintPerformance(FILE * fOut, uint32 Bytes, uint32 SizeOfPixel, double diff)
{
	double seconds  = (double)diff / CLOCKS_PER_SEC;
	double mpix_sec = Bytes / (1024.0*1024.0*seconds*SizeOfPixel);
	fprintf(fOut, "%#4.3g MPixel/sec.\n", mpix_sec);
	fflush(fOut);
}

static void SpeedTest32bits(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, int32 Intent)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	Scanline_rgba32 * In;
	uint32 Mb;
	uint32 Interval = 4; // Power of 2 number to increment r,g,b values by in the loops to keep the test duration practically short
	uint32 NumPixels;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_RGBA_FLT, hlcmsProfileOut, TYPE_RGBA_FLT, Intent, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	NumPixels = 256 / Interval * 256 / Interval * 256 / Interval;
	Mb = NumPixels * sizeof(Scanline_rgba32);
	In = (Scanline_rgba32*)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r += Interval) {
			for(g = 0; g < 256; g += Interval) {
				for(b = 0; b < 256; b += Interval) {
					In[j].r = r / 256.0f;
					In[j].g = g / 256.0f;
					In[j].b = b / 256.0f;
					In[j].a = (In[j].r + In[j].g + In[j].b) / 3;
					j++;
				}
			}
		}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, NumPixels);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(Scanline_rgba32), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest16bits(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, int32 Intent)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	Scanline_rgb16 * In;
	uint32 Mb;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_RGB_16, hlcmsProfileOut, TYPE_RGB_16, Intent, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	Mb = 256*256*256 * sizeof(Scanline_rgb16);
	In = (Scanline_rgb16*)SAlloc::M(Mb);
	if(In) { // @v11.4.5
		j = 0;
		for(r = 0; r < 256; r++) {
			for(g = 0; g < 256; g++) {
				for(b = 0; b < 256; b++) {
					In[j].r = (uint16)((r << 8) | r);
					In[j].g = (uint16)((g << 8) | g);
					In[j].b = (uint16)((b << 8) | b);
					j++;
				}
			}
		}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, 256*256*256);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(Scanline_rgb16), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest32bitsCMYK(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	Scanline_rgba32 * In;
	uint32 Mb;
	uint32 Interval = 4; // Power of 2 number to increment r,g,b values by in the loops to keep the test duration practically short
	uint32 NumPixels;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_CMYK_FLT, hlcmsProfileOut, TYPE_CMYK_FLT, INTENT_PERCEPTUAL, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	NumPixels = 256 / Interval * 256 / Interval * 256 / Interval;
	Mb = NumPixels * sizeof(Scanline_rgba32);
	In = (Scanline_rgba32*)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r += Interval) {
			for(g = 0; g < 256; g += Interval) {
				for(b = 0; b < 256; b += Interval) {
					In[j].r = r / 256.0f;
					In[j].g = g / 256.0f;
					In[j].b = b / 256.0f;
					In[j].a = (In[j].r + In[j].g + In[j].b) / 3;
					j++;
				}
			}
		}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, NumPixels);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(Scanline_rgba32), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest16bitsCMYK(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	Scanline_rgba16 * In;
	uint32 Mb;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_CMYK_16, hlcmsProfileOut, TYPE_CMYK_16, INTENT_PERCEPTUAL,  cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	Mb = 256*256*256*sizeof(Scanline_rgba16);
	In = (Scanline_rgba16*)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r++) {
			for(g = 0; g < 256; g++) {
				for(b = 0; b < 256; b++) {
					In[j].r = (uint16)((r << 8) | r);
					In[j].g = (uint16)((g << 8) | g);
					In[j].b = (uint16)((b << 8) | b);
					In[j].a = 0;
					j++;
				}
			}
		}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, 256*256*256);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(Scanline_rgba16), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest8bits(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, int32 Intent)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	Scanline_rgb8 * In;
	uint32 Mb;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_RGB_8, hlcmsProfileOut, TYPE_RGB_8, Intent, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	Mb = 256*256*256*sizeof(Scanline_rgb8);
	In = (Scanline_rgb8*)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r++)
			for(g = 0; g < 256; g++)
				for(b = 0; b < 256; b++) {
					In[j].r = (uint8)r;
					In[j].g = (uint8)g;
					In[j].b = (uint8)b;
					j++;
				}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, 256*256*256);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(Scanline_rgb8), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest8bitsCMYK(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	Scanline_rgba8 * In;
	uint32 Mb;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_CMYK_8, hlcmsProfileOut, TYPE_CMYK_8, INTENT_PERCEPTUAL, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	Mb = 256*256*256*sizeof(Scanline_rgba8);
	In = (Scanline_rgba8*)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r++)
			for(g = 0; g < 256; g++)
				for(b = 0; b < 256; b++) {
					In[j].r = (uint8)r;
					In[j].g = (uint8)g;
					In[j].b = (uint8)b;
					In[j].a = (uint8)0;
					j++;
				}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, 256*256*256);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(Scanline_rgba8), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest32bitsGray(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, int32 Intent)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	float * In;
	uint32 Mb;
	uint32 Interval = 4; // Power of 2 number to increment r,g,b values by in the loops to keep the test duration practically short
	uint32 NumPixels;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_GRAY_FLT, hlcmsProfileOut, TYPE_GRAY_FLT, Intent, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	NumPixels = 256 / Interval * 256 / Interval * 256 / Interval;
	Mb = NumPixels * sizeof(float);
	In = (float *)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r += Interval)
			for(g = 0; g < 256; g += Interval)
				for(b = 0; b < 256; b += Interval) {
					In[j] = ((r + g + b) / 768.0f);
					j++;
				}

		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, NumPixels);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(float), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest16bitsGray(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, int32 Intent)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	uint16 * In;
	uint32 Mb;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_GRAY_16, hlcmsProfileOut, TYPE_GRAY_16, Intent, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	Mb = 256*256*256 * sizeof(uint16);
	In = (uint16 *)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r++) {
			for(g = 0; g < 256; g++) {
				for(b = 0; b < 256; b++) {
					In[j] = (uint16)((r + g + b) / 3);
					j++;
				}
			}
		}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, 256*256*256);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(uint16), diff);
	cmsDeleteTransform(hlcmsxform);
}

static void SpeedTest8bitsGray(FILE * fOut, const char * Title, cmsHPROFILE hlcmsProfileIn, cmsHPROFILE hlcmsProfileOut, int32 Intent)
{
	int32 r, g, b, j;
	clock_t atime;
	double diff;
	cmsHTRANSFORM hlcmsxform;
	uint8 * In;
	uint32 Mb;
	if(hlcmsProfileIn == NULL || hlcmsProfileOut == NULL)
		Die(fOut, "Unable to open profiles");
	hlcmsxform  = cmsCreateTransformTHR(DbgThread(), hlcmsProfileIn, TYPE_GRAY_8, hlcmsProfileOut, TYPE_GRAY_8, Intent, cmsFLAGS_NOCACHE);
	cmsCloseProfile(hlcmsProfileIn);
	cmsCloseProfile(hlcmsProfileOut);
	Mb = 256*256*256;
	In = (uint8 *)SAlloc::M(Mb);
	if(In) {
		j = 0;
		for(r = 0; r < 256; r++) {
			for(g = 0; g < 256; g++) {
				for(b = 0; b < 256; b++) {
					In[j] = (uint8)r;
					j++;
				}
			}
		}
		TitlePerformance(fOut, Title);
		atime = clock();
		cmsDoTransform(hlcmsxform, In, In, 256*256*256);
		diff = clock() - atime;
		SAlloc::F(In);
	}
	else {
		Die(fOut, "Not enough memory (%u)", Mb);
	}
	PrintPerformance(fOut, Mb, sizeof(uint8), diff);
	cmsDeleteTransform(hlcmsxform);
}

static cmsHPROFILE CreateCurves()
{
	cmsToneCurve * Gamma = cmsBuildGamma(DbgThread(), 1.1);
	cmsToneCurve * Transfer[3];
	Transfer[0] = Transfer[1] = Transfer[2] = Gamma;
	cmsHPROFILE h = cmsCreateLinearizationDeviceLink(cmsSigRgbData, Transfer);
	cmsFreeToneCurve(Gamma);
	return h;
}

static void SpeedTest(FILE * fOut)
{
	char file_name1[512];
	char file_name2[512];
	char file_name3[512];
	char file_name5[512];
	fprintf(fOut, "\n\nP E R F O R M A N C E   T E S T S\n");
	fprintf(fOut, "=================================\n\n");
	fflush(fOut);
	GetTestProfileName("test1.icc", file_name1, sizeof(file_name1));
	GetTestProfileName("test2.icc", file_name2, sizeof(file_name2));
	GetTestProfileName("test3.icc", file_name3, sizeof(file_name3));
	GetTestProfileName("test5.icc", file_name5, sizeof(file_name5));
	SpeedTest8bits(fOut, "8 bits on CLUT profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile(/*"test3.icc"*/file_name3, "r"), INTENT_PERCEPTUAL);
	SpeedTest16bits(fOut, "16 bits on CLUT profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile(/*"test3.icc"*/file_name3, "r"), INTENT_PERCEPTUAL);
	SpeedTest32bits(fOut, "32 bits on CLUT profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile(/*"test3.icc"*/file_name3, "r"), INTENT_PERCEPTUAL);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bits(fOut, "8 bits on Matrix-Shaper profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_PERCEPTUAL);
	SpeedTest16bits(fOut, "16 bits on Matrix-Shaper profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_PERCEPTUAL);
	SpeedTest32bits(fOut, "32 bits on Matrix-Shaper profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_PERCEPTUAL);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bits(fOut, "8 bits on SAME Matrix-Shaper profiles", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), INTENT_PERCEPTUAL);
	SpeedTest16bits(fOut, "16 bits on SAME Matrix-Shaper profiles", cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_PERCEPTUAL);
	SpeedTest32bits(fOut, "32 bits on SAME Matrix-Shaper profiles", cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_PERCEPTUAL);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bits(fOut, "8 bits on Matrix-Shaper profiles (AbsCol)", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_ABSOLUTE_COLORIMETRIC);
	SpeedTest16bits(fOut, "16 bits on Matrix-Shaper profiles (AbsCol)", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_ABSOLUTE_COLORIMETRIC);
	SpeedTest32bits(fOut, "32 bits on Matrix-Shaper profiles (AbsCol)", cmsOpenProfileFromFile(/*"test5.icc"*/file_name5, "r"), cmsOpenProfileFromFile("aRGBlcms2.icc", "r"), INTENT_ABSOLUTE_COLORIMETRIC);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bits(fOut, "8 bits on curves", CreateCurves(), CreateCurves(), INTENT_PERCEPTUAL);
	SpeedTest16bits(fOut, "16 bits on curves", CreateCurves(), CreateCurves(), INTENT_PERCEPTUAL);
	SpeedTest32bits(fOut, "32 bits on curves", CreateCurves(), CreateCurves(), INTENT_PERCEPTUAL);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bitsCMYK(fOut, "8 bits on CMYK profiles", cmsOpenProfileFromFile(/*"test1.icc"*/file_name1, "r"), cmsOpenProfileFromFile(/*"test2.icc"*/file_name2, "r"));
	SpeedTest16bitsCMYK(fOut, "16 bits on CMYK profiles", cmsOpenProfileFromFile(/*"test1.icc"*/file_name1, "r"), cmsOpenProfileFromFile(/*"test2.icc"*/file_name2, "r"));
	SpeedTest32bitsCMYK(fOut, "32 bits on CMYK profiles", cmsOpenProfileFromFile(/*"test1.icc"*/file_name1, "r"), cmsOpenProfileFromFile(/*"test2.icc"*/file_name2, "r"));
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bitsGray(fOut, "8 bits on gray-to gray", cmsOpenProfileFromFile("gray3lcms2.icc", "r"), cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);
	SpeedTest16bitsGray(fOut, "16 bits on gray-to gray", cmsOpenProfileFromFile("gray3lcms2.icc", "r"), cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);
	SpeedTest32bitsGray(fOut, "32 bits on gray-to gray", cmsOpenProfileFromFile("gray3lcms2.icc", "r"), cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bitsGray(fOut, "8 bits on gray-to-lab gray", cmsOpenProfileFromFile("graylcms2.icc", "r"), cmsOpenProfileFromFile("glablcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);
	SpeedTest16bitsGray(fOut, "16 bits on gray-to-lab gray", cmsOpenProfileFromFile("graylcms2.icc", "r"), cmsOpenProfileFromFile("glablcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);
	SpeedTest32bitsGray(fOut, "32 bits on gray-to-lab gray", cmsOpenProfileFromFile("graylcms2.icc", "r"), cmsOpenProfileFromFile("glablcms2.icc", "r"), INTENT_RELATIVE_COLORIMETRIC);
	fprintf(fOut, "\n");
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	SpeedTest8bitsGray(fOut, "8 bits on SAME gray-to-gray", cmsOpenProfileFromFile("graylcms2.icc", "r"), cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_PERCEPTUAL);
	SpeedTest16bitsGray(fOut, "16 bits on SAME gray-to-gray", cmsOpenProfileFromFile("graylcms2.icc", "r"), cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_PERCEPTUAL);
	SpeedTest32bitsGray(fOut, "32 bits on SAME gray-to-gray", cmsOpenProfileFromFile("graylcms2.icc", "r"), cmsOpenProfileFromFile("graylcms2.icc", "r"), INTENT_PERCEPTUAL);
	fprintf(fOut, "\n");
}
//
// Print the supported intents
//
static void PrintSupportedIntents(FILE * fOut)
{
	uint32 Codes[200];
	char * Descriptions[200];
	uint32 n = cmsGetSupportedIntents(200, Codes, Descriptions);
	fprintf(fOut, "Supported intents:\n");
	for(uint32 i = 0; i < n; i++) {
		fprintf(fOut, "\t%u - %s\n", Codes[i], Descriptions[i]);
	}
	fprintf(fOut, "\n");
}

#ifdef LCMS_FAST_EXTENSIONS
	void * cmsFast8Bitextensions();
#endif

// @sobolev int main(int argc, char * argv[])
int Test_LCMS2(const char * pTestbedPath, const char * pOutputFileName, bool exhaustive)
{
	int32 Exhaustive = 0;
	int32 DoSpeedTests = 1;
	int32 DoCheckTests = 1;
	int32 DoPluginTests = 1;
	int32 DoZooTests = 0; // Я не знаю где взять каталог colormaps с тестовыми файлами :(
	FILE * f_out = 0;
	if(!isempty(pTestbedPath)) {
		TestbedPath = pTestbedPath;
	}
	if(!isempty(pOutputFileName))
		f_out = fopen(pOutputFileName, "w");
	SETIFZQ(f_out, stdout);
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	// First of all, check for the right header
	if(cmsGetEncodedCMMversion() != LCMS_VERSION) {
		Die(f_out, "Oops, you are mixing header and shared lib!\nHeader version reports to be '%d' and shared lib '%d'\n", LCMS_VERSION, cmsGetEncodedCMMversion());
	}
	fprintf(f_out, "LittleCMS %2.2f test bed %s %s\n\n", LCMS_VERSION / 1000.0, __DATE__, __TIME__);
	if(exhaustive) {
		Exhaustive = 1;
		fprintf(f_out, "Running exhaustive tests (will take a while...)\n\n");
	}
	/*if((argc == 2) && strcmp(argv[1], "--exhaustive") == 0) {
		Exhaustive = 1;
		fprintf(f_out, "Running exhaustive tests (will take a while...)\n\n");
	}*/
#ifdef LCMS_FAST_EXTENSIONS
	fprintf(f_out, "Installing fast 8 bit extension ...");
	cmsPlugin(cmsFast8Bitextensions());
	fprintf(f_out, "done.\n");
#endif
	fprintf(f_out, "Installing debug memory plug-in ... ");
	cmsPlugin(&DebugMemHandler);
	fprintf(f_out, "done.\n");

	fprintf(f_out, "Installing error logger ... ");
	cmsSetLogErrorHandler(FatalErrorQuit);
	fprintf(f_out, "done.\n");

	PrintSupportedIntents(f_out);

	Check(f_out, "Base types", CheckBaseTypes);
	Check(f_out, "endianness", CheckEndianness);
	Check(f_out, "quick floor", CheckQuickFloor);
	Check(f_out, "quick floor word", CheckQuickFloorWord);
	Check(f_out, "Fixed point 15.16 representation", CheckFixedPoint15_16);
	Check(f_out, "Fixed point 8.8 representation", CheckFixedPoint8_8);
	Check(f_out, "D50 roundtrip", CheckD50Roundtrip);

	// Create utility profiles
	if(DoCheckTests || DoSpeedTests)
		Check(f_out, "Creation of test profiles", CreateTestProfiles);

	if(DoCheckTests) {
		// Forward 1D interpolation
		Check(f_out, "1D interpolation in 2pt tables", Check1DLERP2);
		Check(f_out, "1D interpolation in 3pt tables", Check1DLERP3);
		Check(f_out, "1D interpolation in 4pt tables", Check1DLERP4);
		Check(f_out, "1D interpolation in 6pt tables", Check1DLERP6);
		Check(f_out, "1D interpolation in 18pt tables", Check1DLERP18);
		Check(f_out, "1D interpolation in descending 2pt tables", Check1DLERP2Down);
		Check(f_out, "1D interpolation in descending 3pt tables", Check1DLERP3Down);
		Check(f_out, "1D interpolation in descending 6pt tables", Check1DLERP6Down);
		Check(f_out, "1D interpolation in descending 18pt tables", Check1DLERP18Down);
		if(Exhaustive) {
			Check(f_out, "1D interpolation in n tables", ExhaustiveCheck1DLERP);
			Check(f_out, "1D interpolation in descending tables", ExhaustiveCheck1DLERPDown);
		}
		// Forward 3D interpolation
		Check(f_out, "3D interpolation Tetrahedral (float) ", Check3DinterpolationFloatTetrahedral);
		Check(f_out, "3D interpolation Trilinear (float) ", Check3DinterpolationFloatTrilinear);
		Check(f_out, "3D interpolation Tetrahedral (16) ", Check3DinterpolationTetrahedral16);
		Check(f_out, "3D interpolation Trilinear (16) ", Check3DinterpolationTrilinear16);
		if(Exhaustive) {
			Check(f_out, "Exhaustive 3D interpolation Tetrahedral (float) ", ExaustiveCheck3DinterpolationFloatTetrahedral);
			Check(f_out, "Exhaustive 3D interpolation Trilinear  (float) ", ExaustiveCheck3DinterpolationFloatTrilinear);
			Check(f_out, "Exhaustive 3D interpolation Tetrahedral (16) ", ExhaustiveCheck3DinterpolationTetrahedral16);
			Check(f_out, "Exhaustive 3D interpolation Trilinear (16) ", ExhaustiveCheck3DinterpolationTrilinear16);
		}
		Check(f_out, "Reverse interpolation 3 -> 3", CheckReverseInterpolation3x3);
		Check(f_out, "Reverse interpolation 4 -> 3", CheckReverseInterpolation4x3);
		// High dimensionality interpolation
		Check(f_out, "3D interpolation", Check3Dinterp);
		Check(f_out, "3D interpolation with granularity", Check3DinterpGranular);
		Check(f_out, "4D interpolation", Check4Dinterp);
		Check(f_out, "4D interpolation with granularity", Check4DinterpGranular);
		Check(f_out, "5D interpolation with granularity", Check5DinterpGranular);
		Check(f_out, "6D interpolation with granularity", Check6DinterpGranular);
		Check(f_out, "7D interpolation with granularity", Check7DinterpGranular);
		Check(f_out, "8D interpolation with granularity", Check8DinterpGranular);
		// Encoding of colorspaces
		Check(f_out, "Lab to LCh and back (float only) ", CheckLab2LCh);
		Check(f_out, "Lab to XYZ and back (float only) ", CheckLab2XYZ);
		Check(f_out, "Lab to xyY and back (float only) ", CheckLab2xyY);
		Check(f_out, "Lab V2 encoding", CheckLabV2encoding);
		Check(f_out, "Lab V4 encoding", CheckLabV4encoding);
		// BlackBody
		Check(f_out, "Blackbody radiator", CheckTemp2CHRM);
		// Tone curves
		Check(f_out, "Linear gamma curves (16 bits)", CheckGammaCreation16);
		Check(f_out, "Linear gamma curves (float)", CheckGammaCreationFlt);

		Check(f_out, "Curve 1.8 (float)", CheckGamma18);
		Check(f_out, "Curve 2.2 (float)", CheckGamma22);
		Check(f_out, "Curve 3.0 (float)", CheckGamma30);

		Check(f_out, "Curve 1.8 (table)", CheckGamma18Table);
		Check(f_out, "Curve 2.2 (table)", CheckGamma22Table);
		Check(f_out, "Curve 3.0 (table)", CheckGamma30Table);

		Check(f_out, "Curve 1.8 (word table)", CheckGamma18TableWord);
		Check(f_out, "Curve 2.2 (word table)", CheckGamma22TableWord);
		Check(f_out, "Curve 3.0 (word table)", CheckGamma30TableWord);

		Check(f_out, "Parametric curves", CheckParametricToneCurves);

		Check(f_out, "Join curves", CheckJointCurves);
		Check(f_out, "Join curves descending", CheckJointCurvesDescending);
		Check(f_out, "Join curves degenerated", CheckReverseDegenerated);
		Check(f_out, "Join curves sRGB (Float)", CheckJointFloatCurves_sRGB);
		Check(f_out, "Join curves sRGB (16 bits)", CheckJoint16Curves_sRGB);
		Check(f_out, "Join curves sigmoidal", CheckJointCurvesSShaped);

		// LUT basics
		Check(f_out, "LUT creation & dup", CheckLUTcreation);
		Check(f_out, "1 Stage LUT ", Check1StageLUT);
		Check(f_out, "2 Stage LUT ", Check2StageLUT);
		Check(f_out, "2 Stage LUT (16 bits)", Check2Stage16LUT);
		Check(f_out, "3 Stage LUT ", Check3StageLUT);
		Check(f_out, "3 Stage LUT (16 bits)", Check3Stage16LUT);
		Check(f_out, "4 Stage LUT ", Check4StageLUT);
		Check(f_out, "4 Stage LUT (16 bits)", Check4Stage16LUT);
		Check(f_out, "5 Stage LUT ", Check5StageLUT);
		Check(f_out, "5 Stage LUT (16 bits) ", Check5Stage16LUT);
		Check(f_out, "6 Stage LUT ", Check6StageLUT);
		Check(f_out, "6 Stage LUT (16 bits) ", Check6Stage16LUT);
		// LUT operation
		Check(f_out, "Lab to Lab LUT (float only) ", CheckLab2LabLUT);
		Check(f_out, "XYZ to XYZ LUT (float only) ", CheckXYZ2XYZLUT);
		Check(f_out, "Lab to Lab MAT LUT (float only) ", CheckLab2LabMatLUT);
		Check(f_out, "Named Color LUT", CheckNamedColorLUT);
		Check(f_out, "Usual formatters", CheckFormatters16);
		Check(f_out, "Floating point formatters", CheckFormattersFloat);
#ifndef CMS_NO_HALF_SUPPORT
		Check(f_out, "HALF formatters", CheckFormattersHalf);
#endif
		// ChangeBuffersFormat
		Check(f_out, "ChangeBuffersFormat", CheckChangeBufferFormat);
		// MLU
		Check(f_out, "Multilocalized Unicode", CheckMLU);
		// Named color
		Check(f_out, "Named color lists", CheckNamedColorList);
		// Profile I/O (this one is huge!)
		Check(f_out, "Profile creation", CheckProfileCreation);
		Check(f_out, "Header version", CheckVersionHeaderWriting);
		Check(f_out, "Multilocalized profile", CheckMultilocalizedProfile);
		// Error reporting
		Check(f_out, "Error reporting on bad profiles", CheckErrReportingOnBadProfiles);
		Check(f_out, "Error reporting on bad transforms", CheckErrReportingOnBadTransforms);
		// Transforms
		Check(f_out, "Curves only transforms", CheckCurvesOnlyTransforms);
		Check(f_out, "Float Lab->Lab transforms", CheckFloatLabTransforms);
		Check(f_out, "Encoded Lab->Lab transforms", CheckEncodedLabTransforms);
		Check(f_out, "Stored identities", CheckStoredIdentities);
		Check(f_out, "Matrix-shaper transform (float)",   CheckMatrixShaperXFORMFloat);
		Check(f_out, "Matrix-shaper transform (16 bits)", CheckMatrixShaperXFORM16);
		Check(f_out, "Matrix-shaper transform (8 bits)",  CheckMatrixShaperXFORM8);
		Check(f_out, "Primaries of sRGB", CheckRGBPrimaries);
		// Known values
		Check(f_out, "Known values across matrix-shaper", Chack_sRGB_Float);
		Check(f_out, "Gray input profile", CheckInputGray);
		Check(f_out, "Gray Lab input profile", CheckLabInputGray);
		Check(f_out, "Gray output profile", CheckOutputGray);
		Check(f_out, "Gray Lab output profile", CheckLabOutputGray);
		Check(f_out, "Matrix-shaper proofing transform (float)",   CheckProofingXFORMFloat);
		Check(f_out, "Matrix-shaper proofing transform (16 bits)",  CheckProofingXFORM16);
		Check(f_out, "Gamut check", CheckGamutCheck);
		Check(f_out, "CMYK roundtrip on perceptual transform",   CheckCMYKRoundtrip);
		Check(f_out, "CMYK perceptual transform",   CheckCMYKPerceptual);
		// Check(f_out, "CMYK rel.col. transform",   CheckCMYKRelCol);
		Check(f_out, "Black ink only preservation", CheckKOnlyBlackPreserving);
		Check(f_out, "Black plane preservation", CheckKPlaneBlackPreserving);
		Check(f_out, "Deciding curve types", CheckV4gamma);
		Check(f_out, "Black point detection", CheckBlackPoint);
		Check(f_out, "TAC detection", CheckTAC);
		Check(f_out, "CGATS parser", CheckCGATS);
		Check(f_out, "CGATS parser on junk", CheckCGATS2);
		Check(f_out, "CGATS parser on overflow", CheckCGATS_Overflow);
		Check(f_out, "PostScript generator", CheckPostScript);
		Check(f_out, "Segment maxima GBD", CheckGBD);
		Check(f_out, "MD5 digest", CheckMD5);
		Check(f_out, "Linking", CheckLinking);
		Check(f_out, "floating point tags on XYZ", CheckFloatXYZ);
		Check(f_out, "RGB->Lab->RGB with alpha on FLT", ChecksRGB2LabFLT);
		Check(f_out, "Parametric curve on Rec709", CheckParametricRec709);
		Check(f_out, "Floating Point sampled curve with non-zero start", CheckFloatSamples);
		Check(f_out, "Floating Point segmented curve with short sampled segment", CheckFloatSegments);
		Check(f_out, "Read RAW portions", CheckReadRAW);
		Check(f_out, "Check MetaTag", CheckMeta);
		Check(f_out, "Null transform on floats", CheckFloatNULLxform);
		Check(f_out, "Set free a tag", CheckRemoveTag);
		Check(f_out, "Matrix simplification", CheckMatrixSimplify);
		Check(f_out, "Planar 8 optimization", CheckPlanar8opt);
		Check(f_out, "Swap endian feature", CheckSE);
		Check(f_out, "Transform line stride RGB", CheckTransformLineStride);
		Check(f_out, "Forged MPE profile", CheckForgedMPE);
		Check(f_out, "Proofing intersection", CheckProofingIntersection);
	}
	if(DoPluginTests) {
		Check(f_out, "Context memory handling", CheckAllocContext);
		Check(f_out, "Simple context functionality", CheckSimpleContext);
		Check(f_out, "Alarm codes context", CheckAlarmColorsContext);
		Check(f_out, "Adaptation state context", CheckAdaptationStateContext);
		Check(f_out, "1D interpolation plugin", CheckInterp1DPlugin);
		Check(f_out, "3D interpolation plugin", CheckInterp3DPlugin);
		Check(f_out, "Parametric curve plugin", CheckParametricCurvePlugin);
		Check(f_out, "Formatters plugin",       CheckFormattersPlugin);
		Check(f_out, "Tag type plugin",         CheckTagTypePlugin);
		Check(f_out, "MPE type plugin",         CheckMPEPlugin);
		Check(f_out, "Optimization plugin",     CheckOptimizationPlugin);
		Check(f_out, "Rendering intent plugin", CheckIntentPlugin);
		Check(f_out, "Full transform plugin",   CheckTransformPlugin);
		Check(f_out, "Mutex plugin",            CheckMutexPlugin);
	}
	if(DoSpeedTests)
		SpeedTest(f_out);
#ifdef CMS_IS_WINDOWS_
	if(DoZooTests)
		CheckProfileZOO(f_out);
#endif
	DebugMemPrintTotals(f_out);
	cmsUnregisterPlugins();
	// Cleanup
	if(DoCheckTests || DoSpeedTests)
		RemoveTestProfiles();
	TestbedPath.Z();
	return TotalFail;
}

#endif // } SLTEST_RUNNING
