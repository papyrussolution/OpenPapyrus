// EXECVDOS.CPP
// Copyright (c) A.Sobolev 2016
//
#include <slib.h>

int main(int argc, char ** argv)
{
	ExecVDosParam param;
	SString temp_buf;
	if(argc > 1) {
		for(int i = 1; i < argc; i++) {
			if(stricmp(argv[i], "-exe") == 0) {
				i++;
				if(i < argc) {
					param.ExePath = argv[i];
				}
			}
			else if(stricmp(argv[i], "-start") == 0) {
				i++;
				if(i < argc) {
					param.StartUpPath = argv[i];
				}
			}
			else if(stricmp(argv[i], "-cmd") == 0) {
				i++;
				if(i < argc) {
					temp_buf = argv[i];
					temp_buf.StripQuotes();
					param.Batch.add(temp_buf);
				}
			}
			else if(stricmp(argv[i], "-exit") == 0) {
				param.Flags |= param.fExitAfter;
			}
		}
		ExecVDos(param);
	}
	else {
		(temp_buf = "execvdos.exe").Space().Cat("vdos executor for Papyrus Project").CR();
		printf(temp_buf);
		(temp_buf = "Copyright (c) A.Sobolev 2016").CR();
		printf(temp_buf);
		(temp_buf = 0).Tab().Cat("Usage").CatChar(':').CR();
		printf(temp_buf);
		(temp_buf = 0).Tab().Cat("-exe path").CatDiv(':', 2).Cat("path where vdos.exe").CR();
		printf(temp_buf);
		(temp_buf = 0).Tab().Cat("-start path").CatDiv(':', 2).Cat("path to root C:").CR();
		printf(temp_buf);
		(temp_buf = 0).Tab().Cat("-cmd \"cmdline\"").CatDiv(':', 2).Cat("optional command line").CR();
		printf(temp_buf);
		(temp_buf = 0).Tab().Cat("-exit").CatDiv(':', 2).Cat("close vdos after execution command line").CR();
	}
	//param.ExePath = "\\papyrus\\tools\\vdos";
	//param.StartUpPath = "d:\\papyrus\\";
	//temp_buf = "tools\\dlgdsn.exe src\\rsrc\\dlg\\about.dlg";
	//param.Batch.add(temp_buf);
	//param.Flags |= param.fExitAfter;
	//param.Flags |= param.fWait;
}