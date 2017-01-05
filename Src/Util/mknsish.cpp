// MKNSISH.CPP
//
//#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <slib.h>

struct __PPVer {
	int Major; // MajorVer
	int Minor; // MinorVer
	int Revision; // Revision
	int Asm; // AssemblyVer
};

int GetVerInfo(const char * pVerFileName, __PPVer * pInfo)
{
	int    ok = 1;
	FILE * p_file = fopen(pVerFileName, "r");
	if(p_file) {
		const char * p_mjv = "MajorVer";
		const char * p_mnv = "MinorVer";
		const char * p_r = "Revision";
		const char * p_asm = "AssemblyVer";
		char buf[512], val_buf[128];
		while(fgets(buf, sizeof(buf), p_file) != 0) {
			if(strnicmp(buf, p_mjv, strlen(p_mjv)) == 0) {
				char * p = strchr(buf, '=');
				if(p) {
					strcpy(val_buf, p+1);
					pInfo->Major = atol(val_buf);
				}
			}
			if(strnicmp(buf, p_mnv, strlen(p_mnv)) == 0) {
				char * p = strchr(buf, '=');
				if(p) {
					strcpy(val_buf, p+1);
					pInfo->Minor = atol(val_buf);
				}
			}
			if(strnicmp(buf, p_r, strlen(p_r)) == 0) {
				char * p = strchr(buf, '=');
				if(p) {
					strcpy(val_buf, p+1);
					pInfo->Revision = atol(val_buf);
				}
			}
			if(strnicmp(buf, p_asm, strlen(p_asm)) == 0) {
				char * p = strchr(buf, '=');
				if(p) {
					strcpy(val_buf, p+1);
					pInfo->Asm = atol(val_buf);
				}
			}
		}
		fclose(p_file);
	}
	else
		ok = 0;
	return ok;
}

static void Usage()
{
	printf("Usage:\n");
	printf("Run build batch: MKNSISH.EXE genver.dat /run batchfile papyrus_root_path [/demo || /manual]\n");
}

int main(int argc, char *argv[])
{
	__PPVer ver;
	int    err = 0;
	if(argc < 4) {
		Usage();
		err = -1;
	}
	else {
		if(!GetVerInfo(argv[1], &ver)) {
			printf("Error: unable get version info from file '%s'\n", argv[1]);
			err = -1;
		}
		else if(stricmp(argv[2], "/run") == 0) {
			char root_path[MAXPATH];
			if(argc >= 5)
				STRNSCPY(root_path, argv[4]);
			else
				STRNSCPY(root_path, "c:\\papyrus");
			char temp_buf[1024];
			sprintf(temp_buf, "%s %s %d%d%02d%c %d.%d.%02d(%d)", 
				argv[3], root_path, ver.Major, ver.Minor, ver.Revision, 'a' + ver.Asm % 26,
				ver.Major, ver.Minor, ver.Revision, ver.Asm);
			if(argc >= 6 && stricmp(argv[5], "/demo") == 0)
				strcat(temp_buf, " demo");
			else if(argc >= 6 && stricmp(argv[5], "/manual") == 0)
				strcat(temp_buf, " manual");
			else if(argc >= 6 && stricmp(argv[5], "/mkdemo") == 0)
				strcat(temp_buf, " distanddemo");
			else
				strcat(temp_buf, " dist");

			system(temp_buf);
		}
		else {
			Usage();
			err = -1;
		}
	}
	return err;
}