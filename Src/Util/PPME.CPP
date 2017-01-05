// PPME.CPP
// Copyright (c) A.Sobolev 1998, 2007, 2008
//
// PPY UUCP Mail Extractor
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <ctype.h>
#include <slib.h>

typedef unsigned int uint;

struct UUCP_MSG {
	UUCP_MSG();
	~UUCP_MSG();
	void destroy();
	char * from;
	char * to;
	char * msgid;
	char * subject;
	LDATE  date;
	LTIME  time;
	long   lines;
	long   size;

	long beg;
	long body_start;
	long end;
};

class UUCPFolder : public TSArray <UUCP_MSG> {
public:
	SLAPI UUCPFolder(char * fn);
	SLAPI ~UUCPFolder();
	int SLAPI isValid();
	int SLAPI putMessage(int idx, FILE *, int bodyOnly = 0, int binary = 1);
	int SLAPI dir();
private:
	int SLAPI parse(char * fn);
	int SLAPI getMessage(UUCP_MSG &);
	int SLAPI testNextLine();
	int SLAPI getFldName(char * buf, char * fldnam, uint * p);
	int SLAPI getField(char * fld_name, char * fld_val);
	int SLAPI skipEmptyLines();
	int  errcode;
	char infname[MAXPATH];
	FILE * in;
};

UUCP_MSG::UUCP_MSG()
{
	memset(this, 0, sizeof(UUCP_MSG));
}

UUCP_MSG::~UUCP_MSG()
{
	//destroy();
}

void UUCP_MSG::destroy()
{
	free(from);
	from = 0;
	free(to);
	to = 0;
	free(msgid);
	msgid = 0;
	free(subject);
	subject = 0;
	lines = 0;
	size = 0;
}

#define END_OF_FILE   -1
#define MSG_BODY       0
#define FLD_START      1
#define FLD_FROM       2
#define FLD_TO         3
#define FLD_DATE       4
#define FLD_MSGID      5
#define FLD_LINES      6
#define FLD_SIZE       7
#define FLD_SUBJECT    8
#define FLD_OTHER   1001

#define STR_START "From "
#define STR_FROM  "From"
#define STR_TO    "To"
#define STR_DATE  "Date"
#define STR_MSGID "Message-Id"
#define STR_LINES "Lines"
#define STR_SIZE  "Content-Length"
#define STR_SUBJ  "Subject"

SLAPI UUCPFolder::UUCPFolder(char * fn)
{
	errcode = 0;
	in = 0;
	strcpy(infname, fn);
	parse(fn);
}

SLAPI UUCPFolder::~UUCPFolder()
{
	if(in)
		fclose(in);
	for(int i = 0; i < getCount(); i++)
		at(i).destroy();
}

int SLAPI UUCPFolder::testNextLine()
{
	int c = getc(in);
	ungetc(c, in);
	if(c == EOF)
		return EOF;
	return (c == ' ' || c == '\t') ? 1 : 0;
}

int SLAPI UUCPFolder::getFldName(char * buf, char * fldnam, uint * p)
{
	if(strnicmp(buf, "From ", strlen("From ")) == 0) {
		*p = strlen("From ");
		strcpy(fldnam, "From");
		return FLD_START;
	}
	else {
		char * s = strchr(buf, ':');
		if(s) {
			memcpy(fldnam, buf, (uint)(s - buf));
			fldnam[(uint)(s - buf)] = 0;
			s++;
			while(*s == ' ' || *s == '\t')
				s++;
			*p = (uint)(s - buf);
		}
		else {
			fldnam[0] = 0;
			*p = 0;
		}
	}
	if(stricmp(fldnam, STR_FROM) == 0)
		return FLD_FROM;
	if(stricmp(fldnam, STR_TO) == 0)
		return FLD_TO;
	if(stricmp(fldnam, STR_DATE) == 0)
		return FLD_DATE;
	if(stricmp(fldnam, STR_MSGID) == 0)
		return FLD_MSGID;
	if(stricmp(fldnam, STR_LINES) == 0)
		return FLD_LINES;
	if(stricmp(fldnam, STR_SIZE) == 0)
		return FLD_SIZE;
	if(stricmp(fldnam, STR_SUBJ) == 0)
		return FLD_SUBJECT;
	return FLD_OTHER;
}

int SLAPI UUCPFolder::getField(char * fld_name, char * fld_val)
{
	int  f = END_OF_FILE;
	uint pos;
	char buf[512], * p;
	if(fgets(buf, sizeof(buf), in)) {
		if(buf[0] == '\n')
			f = MSG_BODY;
		else {
			f = getFldName(buf, fld_name, &pos);
			p = stpcpy(fld_val, buf + pos);
			if(testNextLine() > 0)
				do
					if(fgets(buf, sizeof(buf), in))
						p = stpcpy(p, buf + 1);
					else
						break;
				while(testNextLine() > 0);
			int l = strlen(fld_val);
			if(fld_val[l-1] == '\n')
				fld_val[l-1] = 0;
		}
	}
	return f;
}

int SLAPI UUCPFolder::skipEmptyLines()
{
	int c;
	while((c = getc(in)) == '\n');
	ungetc(c, in);
	return (c == EOF) ? 0 : 1;
}

int SLAPI UUCPFolder::getMessage(UUCP_MSG & msg)
{
	int f;
	char * fld_name = (char *)malloc(64);
	char * fld_val  = (char *)malloc(4096);
	skipEmptyLines();
	msg.beg = ftell(in);
	while((f = getField(fld_name, fld_val)) != MSG_BODY && f != END_OF_FILE) {
		switch(f) {
			case FLD_FROM:
				msg.from = strdup(fld_val);
				break;
			case FLD_TO:
				msg.to = strdup(fld_val);
				break;
			case FLD_DATE:
				{
					// "Thu, 22 Jan 1998 19:14:59 +0300 (MSK)"
					char * mss[12] = {
						"Jan", "Feb", "Mar", "Apr", "May", "Jun",
						"Jul", "Aug", "Seb", "Oct", "Nov", "Dec"
					};
					int day, mon = 0, year, h, m, s;
					char ms[16];
					sscanf(fld_val, "%*s %d %s %d %d:%d:%d",
						&day, ms, &year, &h, &m, &s);
					for(int i = 0; mon == 0 && i < 12; i++)
						if(stricmp(ms, mss[i]) == 0)
							mon = i+1;
					msg.date = encodedate(day, mon, year);
					msg.time = encodetime(h, m, s, 0);
				}
				break;
			case FLD_MSGID:
				msg.msgid = strdup(fld_val);
				break;
			case FLD_SUBJECT:
				msg.subject = strdup(fld_val);
				break;
			case FLD_LINES:
				msg.lines = atol(fld_val);
				break;
			case FLD_SIZE:
				msg.size = atol(fld_val);
				break;
		}
	}
	free(fld_name);
	if(f == MSG_BODY) {
		msg.body_start = ftell(in);
		for(long l = 0; l < msg.lines; l++) {
			if(!fgets(fld_val, 512, in)) {
				printf("Unexpected EOF\n");
				fclose(in);
				free(fld_val);
				return 0;
			}
		}
		msg.end = ftell(in);
		free(fld_val);
		return 1;
	}
	free(fld_val);
	return -1;
}

int SLAPI UUCPFolder::putMessage(int idx, FILE * out, int bodyOnly, int binary)
{
	FILE * f = fopen(infname, (binary ? "rb" : "rt"));
	if(f == 0)
		return 0;
	UUCP_MSG & msg = at(idx);
	long p = bodyOnly ? msg.body_start : msg.beg;
	fseek(f, p, SEEK_SET);
	while(p < msg.end) {
		int c = getc(f);
		if(c != EOF)
			putc(c, out);
		else {
			fclose(f);
			return 0;
		}
		p++;
	}
	putc(0x0D, out);
	putc(0x0A, out);
	fclose(f);
	return 1;
}

int SLAPI UUCPFolder::dir()
{
	for(int i = 0; i < getCount(); i++) {
		UUCP_MSG & msg = at(i);
		char ds[16], ts[16];
		printf("%3d %-40s %s %s %4ld %s\n",
			i, msg.from, datefmt(&msg.date, DATF_DMY, ds),
			timefmt(msg.time, 0, ts), msg.lines, msg.subject);
	}
	return 1;
}

int SLAPI UUCPFolder::parse(char * fn)
{
	in = fopen(strcpy(infname, fn), "rt");
	if(in == 0) {
		printf("Unable open file %s\n", fn);
		return 0;
	}
	UUCP_MSG msg;
	while(getMessage(msg) > 0) {
		insert(&msg);
	}
	fclose(in);
	in = 0;
	return 1;
}

static void help()
{
	char * text =
	"\nPPME Utility\n"
	"Usage: ppme -s subject [-r dest_dir] [-d] folder\n"
	"\tfolder       - UUPC folder name\n"
	"Options:\n"
	"\t-?           - this help screen\n"
	"\t-s subject   - messages subject must be converted to binary files\n"
	"\t-r dest_path - path to put result files\n"
	"\t-d           - remove extracted messages from folder\n";
	printf(text);
}

static void invalid_params()
{
	printf("Invalid parameters\n");
	help();
}

// Prototype
FILE * OpenTempFile(char * path, char * fname, char * mode);
char * BTOALastErrString(char * buf);
int SLAPI atob(char * fname, char * outpath);

int main(int argc, char * argv[])
{
	int i;
	char outpath[MAXDIR];
	char foldername[MAXPATH];
	char subject[64];
	char tmpfoldername[MAXPATH];
	char tmpfname[MAXPATH];
	char errmsg[128];
	char * p;
	int  rmv   = 0;
	FILE * temp_folder = 0;
	FILE * tmp = 0;
	outpath[0]    = 0;
	foldername[0] = 0;
	subject[0]    = 0;
	for(i = 1; i < argc; i++) {
		p = argv[i];
		if(p[0] == '-' || p[0] == '/') {
			if(p[1] == '?' || toupper(p[1]) == 'H') {
				help();
				return 0;
			}
			else if(toupper(p[1]) == 'D')
				rmv = 1;
			else if(toupper(p[1]) == 'S') {
				if(p[2])
					strcpy(subject, p+2);
				else if((i+1) < argc)
					strcpy(subject, argv[++i]);
				else
					return (invalid_params(), -1);
			}
			else if(toupper(p[1]) == 'R') {
				if(p[2])
					strcpy(outpath, p+2);
				else if((i+1) < argc)
					strcpy(outpath, argv[++i]);
				else
					return (invalid_params(), -1);
			}
			else
				return (invalid_params(), -1);
		}
		else
			strcpy(foldername, p);
	}
	if(foldername[0] == 0) {
		printf("Folder name needed\n");
		help();
		return -1;
	}
	if(subject[0] == 0) {
		printf("Subject name needed\n");
		help();
		return -1;
	}
	UUCPFolder folder(foldername);
	folder.dir();
	if(rmv) {
		char drv[MAXDRIVE], dir[MAXDIR], fpath[MAXPATH];
		fnsplit(foldername, drv, dir, 0, 0);
		strcpy(fpath, drv);
		strcat(fpath, dir);
		temp_folder = OpenTempFile(fpath, tmpfoldername, "wb");
		if(temp_folder == 0) {
			printf("Error creating temp folder\n");
			return -1;
		}
	}
	for(i = 0; i < folder.getCount(); i++) {
		if(stricmp(subject, folder.at(i).subject) == 0) {
			tmp = OpenTempFile(getenv("TMP"), tmpfname, "wb");
			if(tmp == 0) {
				printf("Error creating temp file for message\n");
				if(temp_folder) {
					fclose(temp_folder);
					unlink(tmpfoldername);
				}
				return -1;
			}
			folder.putMessage(i, tmp, 1, 1);
			fclose(tmp);
			tmp = 0;
			if(!atob(tmpfname, outpath)) {
				printf(BTOALastErrString(errmsg));
				if(rmv)
					folder.putMessage(i, temp_folder, 0, 1);
			}
			unlink(tmpfname);
		}
		else if(rmv) {
			folder.putMessage(i, temp_folder, 0, 1);
		}
	}
	if(temp_folder) {
		fclose(temp_folder);
		unlink(foldername);
		rename(tmpfoldername, foldername);
	}
	return 0;
}


