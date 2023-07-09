// BenchCon.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

struct CPrintBenchCallback : public IBenchPrintCallback {
	void Print(const char * s);
	void NewLine();
	HRESULT CheckBreak();

	FILE * _file;
};

void CPrintBenchCallback::Print(const char * s) { fputs(s, _file); }
void CPrintBenchCallback::NewLine() { fputc('\n', _file); }
HRESULT CPrintBenchCallback::CheckBreak() { return NConsoleClose::TestBreakSignal() ? E_ABORT : S_OK; }

HRESULT BenchCon(DECL_EXTERNAL_CODECS_LOC_VARS const CObjectVector<CProperty> &props, uint32 numIterations, FILE * f)
{
	CPrintBenchCallback callback;
	callback._file = f;
	return Bench(EXTERNAL_CODECS_LOC_VARS &callback, NULL, props, numIterations, true);
}

