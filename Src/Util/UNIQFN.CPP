// UNIQFN.CPP
// Copyright (c) A.Sobolev 2000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slib.h>

int main(int argc, char * argv[])
{
	char path[MAXPATH], fname[MAXPATH];
	char prefix[64], ext[32];
	char cmd_line[256];
	char * p;
	if(argc < 2) {
		printf("Usage: uniqfn path file_prefix[.ext]\n");
		return 1;
	}
	strcpy(path, argv[1]);
	prefix[0] = 0;
	ext[0] = 0;
	if(argc >= 3) {
		strcpy(prefix, argv[2]);
		p = strchr(prefix, '.');
		if(p) {
			*p = 0;
			strcpy(ext, p+1);
		}
	}
	//if(*strip(prefix) == 0)
	//	strcpy(prefix, "UFN");
	//if(argc

	long last_count = 0;
	SDirEntry sdir_entry;
	SDirec    sdir;
	setLastSlash(path);
	sprintf(fname, "%s%s*.%s", path, prefix, ext);
	for(sdir.Init(fname); sdir.Next(&sdir_entry) > 0;) {
		p = strchr(sdir_entry.FileName, '.');
		if(strnicmp(sdir_entry.FileName, prefix, strlen(prefix)) == 0 &&
			(!p || stricmp(p+1, ext) == 0)) {
			if(p)
				*p = 0;
			long n = atol(sdir_entry.FileName);
			if(n > last_count)
				last_count = n;
		}
	}
	setLastSlash(path);

	sprintf(fname, " %s%s%0*ld.%s ", path, prefix, 8-strlen(prefix), last_count+1, ext);
	printf(fname);
	return 0;
}

