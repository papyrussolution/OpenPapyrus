// UEDP.CPP
// Copyright (c) A.Sobolev 2023, 2025
//
#include <pp.h>
#pragma hdrstop
#include <ued.h>
#include <sartre.h>

int main(int argc, const char * argv[])
{
	int    result = 0;
	SString src_file_name; //("\\Papyrus\\Src\\Rsrc\\Data\\Sartre\\UED.txt");
	SString out_path;
	SString c_path;
	SString java_path;
	SString rt_out_path;
	//bool   force_update_pldecl = false;
	//bool   tolerant_mode = false;
	uint   flags = 0;
	DS.Init(PPSession::fInitPaths, 0, 0);
	assert(argc >= 1);
	if(argc < 1) {
		result = -1;
	}
	else if(argc == 1) {
		SString help_buf;
		help_buf.Cat("Usage").CatDiv(':', 2).Cat(argv[0]).Space().Cat("[options]").Space().Cat("source-file-name").CR();
		help_buf.Cat("options").CatDiv(':', 2).CR();
		help_buf.Tab().Cat("-tol").Tab_(3).Cat("tolerant mode").CR();
		help_buf.Tab().Cat("-out").Tab_(3).Cat("output-path").CR();
		help_buf.Tab().Cat("-rtpath").Tab_(3).Cat("runtime output-path").CR();
		help_buf.Tab().Cat("-cpath").Tab_(3).Cat("c-definitions-path").CR();
		help_buf.Tab().Cat("-javapath").Tab_(2).Cat("java-definitions-path").CR();
		help_buf.Tab().Cat("-forceupdatepldecl").Tab_(1).Cat("force update programming language output even if source file unchanged").CR();
		fprintf(stdout, help_buf.cptr());
		result = 1;
	}
	else if(argc > 1) {
		for(int argn = 1; argn < argc; argn++) {
			if(sstreqi_ascii(argv[argn], "-out")) {
				if((argn+1) < argc) {
					out_path = argv[++argn];
				}
				else {
					; // @todo @err
				}
			}
			else if(sstreqi_ascii(argv[argn], "-rtpath")) {
				if((argn+1) < argc) {
					rt_out_path = argv[++argn];
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
				flags |= prcssuedfForceUpdatePlDecl;
			}
			else if(sstreqi_ascii(argv[argn], "-tol")) {
				flags |= prcssuedfTolerant;
			}
			else {
				if(src_file_name.IsEmpty())
					src_file_name = argv[argn];
				else {
					; // @todo @err
				}
			}
		}
		PPLogger logger(PPLogger::fStdErr);
		int    r = ProcessUed(src_file_name, out_path, rt_out_path, c_path, java_path, flags, &logger);
		if(!r)
			result = -1;
	}
	return result;
}