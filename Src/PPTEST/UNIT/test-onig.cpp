// TEST-ONIG.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop

extern int OnigTestSyntax_main(FILE * fOut);
extern int OnigTestOptions_main(FILE * fOut);
extern int OnigTestRegSet_main(FILE * fOut);
extern int OnigTestBack_main(FILE * fOut);
extern int OnigTestUtf8_main(FILE * fOut);
extern int OnigTestU_main(FILE * fOut);
extern int OnigTestP_main(FILE * fOut);
extern int OnigTestC_main(FILE * fOut);
extern int OnigTestC_Windows_main(FILE * fOut);

SLTEST_R(Onig)
{
	SString out_file_name;
	SLS.GetLogPath(out_file_name);
	out_file_name.SetLastSlash().Cat("onig-test.log");
	FILE * f_onig_out = fopen(out_file_name, "w");
	SLTEST_CHECK_Z(OnigTestSyntax_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestOptions_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestRegSet_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestU_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestP_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestC_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestC_Windows_main(f_onig_out));
	SLTEST_CHECK_Z(OnigTestUtf8_main(f_onig_out)); // !
	SLTEST_CHECK_Z(OnigTestBack_main(f_onig_out)); // !
	SFile::ZClose(&f_onig_out);
	return CurrentStatus;
}
