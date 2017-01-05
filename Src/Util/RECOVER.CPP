// RECOVER.CPP
//
#include <slib.h>
#pragma hdrstop
#include <string.h>

int main(int argc, char **argv)
{
	int64  size = 0;
	char in_fname[MAXPATH], out_fname[MAXPATH];
	if(argc < 3) {
		printf("RECOVER Copyright (c) Petroglif 2002\n");
		printf("Usage: RECOVER infile outfile\n");
		return -1;
	}
	STRNSCPY(in_fname, argv[1]);
	if(strip(in_fname)[0] == 0) {
		printf("Error: Undefined input file name\n");
		return -1;
	}
	STRNSCPY(out_fname, argv[2]);
	if(strip(out_fname)[0] == 0) {
		printf("Error: Undefined output file name\n");
		return -1;
	}
	if(DoCompress(in_fname, out_fname, &size, 0, 0) <= 0) {
		printf("Error: can't recover (backup) file %s\n", out_fname);
		return -1;
	}
	return 0;
}
