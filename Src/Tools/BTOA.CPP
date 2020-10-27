// BTOA.CPP
// Copyright (c) A.Sobolev 1998
//
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <slib.h>
#include <ctype.h>

//#define BTOA_MAIN

#define BTOAERR_TEMPWRITE     1
#define BTOAERR_BCOUNTSYNC    2
#define BTOAERR_ILLINCHAR     3
#define BTOAERR_INVFOOTER     4
#define BTOAERR_CHECKSUM      5
#define BTOAERR_INOPENING     6
#define BTOAERR_OUTOPENING    7
#define BTOAERR_TMPOPENING    8
#define BTOAERR_EMPTYINPUT    9

char * BTOAErrStr[] = {
	"success",
	"write error on temp file",
	"bcount synchronization error",
	"illegal character in input stream",
	"scanf failure on sumcheck line",
	"sumcheck values don't match",
	"error opening input file",
	"error opening output file",
	"error opening temp file",
	"empty input stream"
};

int BTOAErrCode = 0;

FILE * OpenTempFile(char * path, char * fname, char * mode)
{
	char p[MAXDIR], fn[MAXPATH];
	if(path && path[0]) {
		strcpy(p, path);
		int l = strlen(p);
		if(p[l-1] != '\\') {
			p[l] = '\\';
			p[l+1] = 0;
		}
	}
	else
		p[0] = 0;
	for(long i = 1; i < 99999L; i++) {
		sprintf(fn, "%sSL%05ld.TMP", p, i);
		if(access(fn, 0) != 0) {
			if(fname)
				strcpy(fname, fn);
			return fopen(fn, mode ? mode : "w+b");
		}
	}
	return (FILE*)0;
}

char * BTOALastErrString(char * buf)
{
	sprintf(buf, "BTOA error: %s\n", BTOAErrStr[BTOAErrCode]);
	return buf;
}
//
// btoa
//
#define MAXPERLINE 78

struct BTOA {
	SLAPI BTOA() { memset(this, 0, sizeof(BTOA)); bcount = 3; }
	void SLAPI encode(uchar c, FILE *);
	void SLAPI wordout(FILE *);
	ulong Ceor;
	ulong Csum;
	ulong Crot;
	int   ccount;
	int   bcount;
	union {
		ulong w;
		uchar b[4];
	} ublock;
};

void SLAPI BTOA::encode(uchar c, FILE * out)
{
	Ceor ^= c;
	Csum += c;
	Csum += 1;
	if(Crot & 0x80000000UL) {
		Crot <<= 1;
		Crot += 1;
	}
	else
		Crot <<= 1;
	Crot += c;
	ublock.b[bcount--] = c;
	if(bcount < 0) {
		wordout(out);
		bcount = 3;
	}
}

void SLAPI BTOA::wordout(FILE * out)
{
	if(ublock.w == 0L) {
		putc('z', out); /* note: 'z' is outside mod-85 char set */
		if(++ccount == MAXPERLINE) {
			putc('\n', out);
			ccount = 0;
		}
	}
	else {
		int i;
		char tmp[5];
		for(i = 0; i < 5; i++, ublock.w /= 85)
			tmp[i] = ublock.w % 85 + '!';
		for(i = 4; i >= 0; i--) {
			putc(tmp[i], out);
			if(++ccount == MAXPERLINE) {
				putc('\n', out);
				ccount = 0;
			}
		}
	}
}

int SLAPI btoa(char * infname, char * outfname)
{
	int  c;
	long n = 0;
	char header[128];
	char drv[MAXDRIVE], dir[MAXDIR], nam[MAXFILE], ext[MAXEXT];
	uint fdate, ftime;
	BTOA blk;
	FILE * out = 0;
	FILE * in  = fopen(infname, "rb");
	if(in == 0)
		return (BTOAErrCode = BTOAERR_INOPENING, 0);
	_dos_getftime(fileno(in), &fdate, &ftime);
	fnsplit(infname, drv, dir, nam, ext);
	sprintf(header, "xbtoa Begin %s%s %u %u\n", nam, ext, fdate, ftime);
	if(outfname)
		out = fopen(outfname, "wb");
	else {
		out = stdout;
		setmode(fileno(out), O_BINARY);
	}
	if(out == 0) {
		fclose(in);
		return (BTOAErrCode = BTOAERR_OUTOPENING, 0);
	}
	fprintf(out, header);
	while((c = getc(in)) != EOF) {
		blk.encode(c, out);
		n++;
	}
	while(blk.bcount != 3)
		blk.encode(0, out);
	/* n is written twice as crude cross check*/
	fprintf(out, "\nxbtoa End N %ld %lx E %lx S %lx R %lx\n", n, n,
		blk.Ceor, blk.Csum, blk.Crot);
	fclose(in);
	fclose(out);
	return 1;
}
//
// atob
//
struct ATOB {
	ATOB() { bcount = (int)(Ceor = Csum = Crot = ublock.w = 0UL); }
	int  dwordout(ulong c, FILE *);
	int  decode(uchar c, FILE *);
	ulong Ceor;
	ulong Csum;
	ulong Crot;
	int   bcount;
	union U {
		ulong w;
		uchar b[4];
	} ublock;
};

int ATOB::dwordout(ulong dw, FILE * tmp)
{
	U u;
	u.w = dw;
	for(int i = 3; i >= 0; i--) {
		int c = u.b[i];
		Ceor ^= c;
		Csum += c;
		Csum += 1;
		if(Crot & 0x80000000UL) {
			Crot <<= 1;
			Crot += 1;
		}
		else
			Crot <<= 1;
		Crot += c;
		if(putc(c, tmp) == EOF)
			return (BTOAErrCode = BTOAERR_TEMPWRITE, 0);
	}
	return 1;
}

#define DE(c) ((c) - '!')

int ATOB::decode(uchar c, FILE * tmp)
{
	if(c == 'z') {
		if(bcount != 0)
			return (BTOAErrCode = BTOAERR_BCOUNTSYNC, 0);
		else if(!dwordout(0, tmp))
			return 0;
	}
	else if(c >= '!' && c < ('!' + 85))
		if(bcount == 0) {
			ublock.w = DE(c);
			bcount++;
		}
		else if(bcount < 4) {
			ublock.w = ublock.w * 85 + DE(c);
			bcount++;
		}
		else {
			if(!dwordout(ublock.w * 85 + DE(c), tmp))
				return 0;
			ublock.w = 0;
			bcount = 0;
		}
	else
		return (BTOAErrCode = BTOAERR_ILLINCHAR, 0);
	return 1;
}

int SLAPI atob(char * fname, char * outpath)
{
	EXCEPTVAR(BTOAErrCode);
	BTOAErrCode = 0;
	int    ok = 1;
	int    c;
	char   buf[128];
	uint   fdate, ftime;
	ulong  n1, n2, oeor, osum, orot, l;
	char   infname[MAXPATH], outfname[MAXPATH], tmpfname[MAXPATH];
	FILE * in = 0, * out = 0, * tmp = 0;
	ATOB   blk;
	infname[0] = outfname[0] = tmpfname[0] = 0;
	THROW_V(in = fopen(strcpy(infname, fname), "rt"), BTOAERR_INOPENING);
	/*
		search for header line
	*/
	for(out = 0; !out && fgets(buf, sizeof(buf), in);)
		if((strncmp(buf, "xbtoa Begin", strlen("xbtoa Begin")))==0) {
			sscanf(buf, "%*s %*s %s %u %u", outfname, &fdate, &ftime);
			if(outpath && outpath[0]) {
				int l = strlen(strcpy(buf, outpath));
				if(buf[l-1] != '\\') {
					buf[l++] = '\\';
					buf[l] = 0;
				}
				strcpy(buf + l, outfname);
				strcpy(outfname, buf);
			}
			THROW_V(out = fopen(outfname, "wb"), BTOAERR_OUTOPENING);
		}
	THROW_V(out, BTOAERR_EMPTYINPUT);
	THROW_V(tmp = OpenTempFile(getenv("TMP"), tmpfname, "wb+"), BTOAERR_TMPOPENING);
	while((c = getc(in)) != EOF && c != 'x')
		if(c != '\n') {
			THROW(blk.decode(c, tmp));
		}
	THROW_V(fscanf(in, "btoa End N %ld %lx E %lx S %lx R %lx\n",
		&n1, &n2, &oeor, &osum, &orot) == 5, BTOAERR_INVFOOTER);
	THROW_V((n1 == n2 && oeor == blk.Ceor && osum == blk.Csum &&
		orot == blk.Crot), BTOAERR_CHECKSUM);
	/*
		copy OK tmp file to out
	*/
	rewind(tmp);
	for(l = 0; (c=getc(tmp)) != EOF && l < n1; l++)
		putc(c, out);
	fflush(out);
	_dos_setftime(fileno(out), fdate, ftime);
	CATCH
		ok = 0;
	ENDCATCH
	fclose(tmp);
   	fclose(out);
	fclose(in);
	if(tmpfname[0])
		unlink(tmpfname);
	if(!ok && outfname)
		unlink(outfname);
	return ok;
}

#ifdef BTOA_MAIN

static void help()
{
	printf("\nBTOA encoding/decoding utility.\nUsage: btoa [options] infile [outfile]\n");
	printf("-?      - help (this screen)\n");
	printf("-x      - decoding (atob)\n");
	printf("-r path - dest path (decoding only. Default current dir.)\n");
	printf("infile  - input file name\n");
	printf("outfile - output file name (encoding only. If empty, then stdout.)\n\n");
}

static void invalid_params()
{
	printf("Invalid parameters\n");
	help();
}

int main(int argc, char *argv[])
{
	int ok = 1;
	int decoding = 0;
	int in_param = 0;
	int out_param = 0;
	char outpath[MAXDIR];
	outpath[0] = 0;
	if(argc < 2) {
		help();
		return 2;
	}
	for(int i = 1; i < argc; i++) {
		if(argv[i][0] == '-' || argv[i][0] == '/') {
			if(toupper(argv[i][1]) == 'X')
				decoding = 1;
			else if(argv[i][1] == '?' || toupper(argv[i][1]) == 'H') {
				help();
				return 0;
			}
			else if(toupper(argv[i][1]) == 'R') {
				if(argv[i][2])
					strcpy(outpath, argv[i] + 2);
				else {
					if(i+1 < argc)
						strcpy(outpath, argv[++i]);
					else
						return (invalid_params(), 2);
				}
			}
			else
				return (invalid_params(), 2);
		}
		else {
			if(in_param == 0)
				in_param = i;
			else if(out_param == 0)
				out_param = i;
			else
				return (invalid_params(), 2);
		}
	}
	if(in_param == 0)
		return (invalid_params(), 2);
	if(decoding)
		ok = atob(argv[in_param], outpath);
	else
		ok = btoa(argv[in_param], out_param ? argv[out_param] : 0);
	if(!ok) {
		char buf[128];
		printf(BTOALastErrString(buf));
		return -1;
	}
	return 0;
}

#endif // BTOA_MAIN
