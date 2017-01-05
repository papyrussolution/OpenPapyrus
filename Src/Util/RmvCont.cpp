// RMVCONT.cpp
// Starodub A. 2005, 2008, 2011
//
#include <slib.h>
#include <db.h>

// @stub
int CallbackCompress(long a, long b, const char * c, int stop)
{
	return 1;
}

int main(int argc, char * argv[])
{
	int    ok = -1;
	if(argc < 2) {
		printf("RMVCONT Copyright (c) Petroglif 2005\n");
		printf("Usage: RMVCONT fulldbpath\n");
	}
	else {
		int    found_delta_files = 0;
		char   data_path[MAXPATH], wildcard[MAXPATH];
		SDirec * p_direc = 0;
		SDirEntry dir_entry;
		memzero(data_path, sizeof(data_path));
		memzero(wildcard, sizeof(wildcard));
		STRNSCPY(data_path, argv[1]);
		strcat(setLastSlash(STRNSCPY(wildcard, data_path)), "*.^^^");
		if((p_direc = new SDirec(wildcard))) {
			for(; p_direc->Next(&dir_entry) > 0;)
				if(!(dir_entry.Attr & 0x10)) {
					char   file_name[MAXPATH];
					DBTable tbl;
					found_delta_files = 1;
					setLastSlash(STRNSCPY(file_name, data_path));
					strcat(file_name, dir_entry.FileName);
					replaceExt(file_name, "btr", 1);
					tbl.open(file_name);
					Btrieve::RemoveContinuous(file_name);
				}
			if(found_delta_files)
				ok = 0;
			else
				printf("Not found *.^^^ files in path=[%s]", data_path);
			delete p_direc;
		}
		else
			printf("Not enought memory");
	}
	return ok;
}

