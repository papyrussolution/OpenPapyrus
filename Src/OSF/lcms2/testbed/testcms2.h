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
#ifndef TESTCMS2_H
#define TESTCMS2_H

#include "lcms2_internal.h"

// On Visual Studio, use debug CRT
#ifdef _MSC_VER
#    include "crtdbg.h"
#endif

#ifdef CMS_IS_WINDOWS_
#    include <io.h>
#endif

#define cmsmin(a, b) (((a) < (b)) ? (a) : (b))

// Used to mark special pointers
void DebugMemDontCheckThis(void * Ptr);

boolint STDCALL IsGoodVal(const char * title, double in, double out, double max);
boolint STDCALL IsGoodFixed15_16(const char * title, double in, double out);
boolint IsGoodFixed8_8(const char * title, double in, double out);
boolint STDCALL IsGoodWord(const char * title, uint16 in, uint16 out);
boolint STDCALL IsGoodWordPrec(const char * title, uint16 in, uint16 out, uint16 maxErr);

void * PluginMemHandler();
cmsContext WatchDogContext(FILE * fOut, void * usr);

void ResetFatalError();
void Die(FILE * fOut, const char * Reason, ...);
void Dot(FILE * fOut);
void Fail(const char* frm, ...);
void SubTest(FILE * fOut, const char* frm, ...);
void TestMemoryLeaks(FILE * fOut, boolint ok);
void Say(FILE * fOut, const char* str);

// Plug-in tests
int32 CheckSimpleContext(FILE * fOut);
int32 CheckAllocContext();
int32 CheckAlarmColorsContext(FILE * fOut);
int32 CheckAdaptationStateContext(FILE * fOut);
int32 CheckInterp1DPlugin(FILE * fOut);
int32 CheckInterp3DPlugin(FILE * fOut);
int32 CheckParametricCurvePlugin(FILE * fOut);
int32 CheckFormattersPlugin(FILE * fOut);
int32 CheckTagTypePlugin(FILE * fOut);
int32 CheckMPEPlugin(FILE * fOut);
int32 CheckOptimizationPlugin(FILE * fOut);
int32 CheckIntentPlugin(FILE * fOut);
int32 CheckTransformPlugin(FILE * fOut);
int32 CheckMutexPlugin(FILE * fOut);
int32 CheckMethodPackDoublesFromFloat(FILE * fOut);

// Zoo
void CheckProfileZOO(FILE * fOut);

#endif
