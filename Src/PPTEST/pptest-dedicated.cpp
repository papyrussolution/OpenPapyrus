// PPTEST-DEDICATED.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Отдельный модуль тестирования.
//
#include <pp.h>
#pragma hdrstop
//
//
//
int DummyProc_dirent(); // @prototype @forcelink
int DummyProc_bzip3(); // @prototype @forcelink
int DummyProc_sfxtree(); // @forcelink
int DummyProc_TulipIndicators(); // @prototype @forcelink
// @v11.2.0 {
// Для сборки _MSC_VER менее чем 2015 мы не будем поддерживать LMDB. Здесь включена пустышка для пропуска соответствующего теста
#if _MSC_VER >= 1900
	int DummyProc_LMDB();   // @prototype @forcelink
	int DummyProc_SFileStorage(); // @prototype @forcelink
#else
	#if SLTEST_RUNNING
		SLTEST_R(LMDB) { return 1; }
	#endif
#endif
// } @v11.2.0

int main(int argc, char ** argv)
{
	if(DS.Init(PPSession::fInitPaths, 0, /*pUiDescriptionFileName*/0)) {
		STestSuite s(STestSuite::fConsole);
		SRng::CreateInstance((SRng::Algorithm)0, 0); // @forcelink RandomNumberGeneragtor
		DummyProc_dirent(); // @v10.9.12 @forcelink
		DummyProc_bzip3(); // @v11.7.4 @forcelink
		DummyProc_sfxtree(); // @v11.7.4 @forcelink
		DummyProc_TulipIndicators(); // @v11.4.4 @forcelink
	#if _MSC_VER >= 1900
		DummyProc_LMDB(); // @v11.2.0 @forcelink
		DummyProc_SFileStorage(); // @v11.3.3 @forcelink
	#endif
		s.Run("\\papyrus\\src\\pptest\\testdef-dedicated.ini");
		//Test_Alg_SS_Z("c:\\papyrus\\src\\pptest\\words.");
		//Test_InterfaceCall();
	}
	PPReleaseStrings();
	return 0;
}