// UEDP.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>

int ProcessUed(const char * pSrcFileName, bool forceUpdatePlDecl, PPLogger * pLogger);

int main(int argc, const char * argv[])
{
	int    result = 0;
	SString src_file_name("\\Papyrus\\Src\\Rsrc\\Data\\Sartre\\UED.txt");
	bool   force_update_pldecl = false;
	int    r = ProcessUed(src_file_name, force_update_pldecl, 0);
	return result;
}