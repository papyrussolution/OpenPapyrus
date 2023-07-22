// UEDP.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>

int ProcessUed(const char * pSrcFileName, const char * pOutPath, const char * pCPath, const char * pJavaPath, bool forceUpdatePlDecl, PPLogger * pLogger);

int main(int argc, const char * argv[])
{
	int    result = 0;
	SString src_file_name; //("\\Papyrus\\Src\\Rsrc\\Data\\Sartre\\UED.txt");
	SString out_path;
	SString c_path;
	SString java_path;
	bool   force_update_pldecl = false;
	assert(argc >= 1);
	if(argc < 1) {
		result = -1;
	}
	else if(argc == 1) {
		SString help_buf;
		help_buf.Cat("Usage").CatDiv(':', 2).Cat(argv[0]).Space().Cat("[options]").Space().Cat("source-file-name").CR();
		help_buf.Cat("options").CatDiv(':', 2).CR();
		help_buf.Tab().Cat("-out").Tab(3).Cat("output-path").CR();
		help_buf.Tab().Cat("-cpath").Tab(3).Cat("c-definitions-path").CR();
		help_buf.Tab().Cat("-javapath").Tab(2).Cat("java-definitions-path").CR();
		help_buf.Tab().Cat("-forceupdatepldecl").Tab(1).Cat("force update programming language output even if source file unchanged").CR();
		fprintf(stdout, help_buf.cptr());
	}
	else if(argc > 1) {
		for(int argn = 1; argn < argc; argn++) {
			if(sstreqi_ascii(argv[argn], "-out")) {
				if((argn+1) < argc) {
				}
				else {
					; // @todo @err
				}
			}
			else if(sstreqi_ascii(argv[argn], "-cpath")) {
				if((argn+1) < argc) {
					c_path = argv[++argn];
				}
				else {
					; // @todo @err
				}
			}
			else if(sstreqi_ascii(argv[argn], "-javapath")) {
				if((argn+1) < argc) {
					java_path = argv[++argn];
				}
				else {
					; // @todo @err
				}
			}
			else if(sstreqi_ascii(argv[argn], "-forceupdatepldecl")) {
				force_update_pldecl = true;
			}
			else {
				if(src_file_name.IsEmpty())
					src_file_name = argv[argn];
				else {
					; // @todo @err
				}
			}
		}
		int    r = ProcessUed(src_file_name, out_path, c_path, java_path, force_update_pldecl, 0);
	}
	return result;
}