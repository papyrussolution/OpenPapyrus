// SLTESTAPP.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестовое приложение для отработки функций запуска и управления системными процессами
//
#include <pp.h>
#include <wsctl.h>

int main(int argc, char * argv[])
{
	int    result = 0;
	SString temp_buf;
	SStringU temp_buf_u;
	SString out_buf;
	SString policypath;
	WsCtl_ClientPolicy policy;
	(out_buf = "SlTestApp: тестовое приложение").Transf(CTRANSF_UTF8_TO_INNER).CR();
	slfprintf_stderr(out_buf);
	if(argc == 1) {
		slfprintf_stderr("There aren't cmdline args\n");
	}
	else {
		out_buf.Z().Cat("Cmdline args").Space().CatParStr(argc).Colon().CR();
		slfprintf_stderr(out_buf);
		for(int i = 1; i < argc; i++) {
			//temp_buf.CopyUtf8FromUnicode(argv[i], sstrlen(argv[i]), 0);
			temp_buf = argv[i];
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			out_buf.Z().Tab().Cat(temp_buf).CR();
			slfprintf_stderr(out_buf);
			if(temp_buf.IsEqiAscii("policypath") && (i+1) < argc) {
				policypath = argv[++i];
			}
		}
	}
	{
		wchar_t curdir_u[1024];
		::GetCurrentDirectoryW(sizeof(curdir_u), curdir_u);
		temp_buf.CopyUtf8FromUnicode(curdir_u, sstrlen(curdir_u), 0);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		out_buf.Z().Cat("Current Directory").CatDiv(':', 2).Cat(temp_buf).CR();
		slfprintf_stderr(out_buf);
	}
	if(policypath.NotEmpty()) {
		temp_buf.Z().Cat("policy path").CatDiv(':', 2).Cat(policypath).CR();
		slfprintf_stderr(out_buf);
		//
	}
	//
	slfprintf_stderr("Press [Enter] to finish...\n");
	getchar();
	return result;
}
