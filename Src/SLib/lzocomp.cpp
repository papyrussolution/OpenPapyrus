// LZOCOMP.CPP
// LZO compress decompress function.
// Copyright Alex Osolotkin, A.Starodub 1999-2003, 2010, 2011, 2016, 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <sys\stat.h>
#include "minilzo.h"

//#define KB 1024

#ifdef __WIN32__
	#define huge
#endif

//#define HEAP_ALLOC(var,size) \
//	long __LZO_MMODEL var [ ((size) + (sizeof(long) - 1)) / sizeof(long) ]

//static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);
long huge * wrkmem = 0; // @global

int SLAPI getFileTime(int fh, LDATETIME * creation, LDATETIME * lastAccess, LDATETIME * lastModif);
int SLAPI setFileTime(int fh, LDATETIME * creation, LDATETIME * lastAccess, LDATETIME * lastModif);

int compress(char *src, char *dest, int isdecomp, PercentFunc pf, ulong *sz)
{
	struct _HEADER { // size = 128
		char   FileName[64];
		LDATETIME FileLModDateTime;
		long   FileSize;
		ulong  CRC;
		char   Reserve[48];
	} header;
	EXCEPTVAR(SLibError);
	int    r = 0;
	CRC32  _crc32;
	FILE * fin = 0, * fout = 0;
	lzo_uint in_len = 0;
	lzo_uint out_len = 0;
	lzo_uint new_len = 0;
	lzo_uint old_size = 0;
	lzo_uint new_size = 0;
	unsigned char huge * inbuf = 0;
	unsigned char huge * outbuf = 0;
	// AHTOXA {
	char   buf[512];
#ifdef __WIN32__
	HANDLE srchdl = 0;
#endif
	// } AHTOXA
	struct stat statbuf;
	uint BUFLEN = SKILOBYTE(32);
	LDATETIME creation_time, last_access_time, last_modif_time;
	if(lzo_init() != LZO_E_OK)
		return r;
	THROW(wrkmem = (long *)SAlloc::C(((LZO1X_1_MEM_COMPRESS)+sizeof(long)-1)/sizeof(long), sizeof(long)));
	do {
		ZFREE(inbuf);
		ZFREE(outbuf);
		inbuf = (uchar *)SAlloc::M(BUFLEN);
		outbuf = (uchar *)SAlloc::M(BUFLEN);
	} while((!inbuf || !outbuf) && (BUFLEN -= SKILOBYTE(1)) >= SKILOBYTE(1));
	THROW(inbuf && outbuf);
// AHTOXA {
#ifdef __WIN32__
	srchdl = CreateFile(src, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); // @unicodeproblem
	THROW_V(srchdl >= 0, SLERR_OPENFAULT);
	getFileTime((int)srchdl,  &creation_time, &last_access_time, &last_modif_time);
	if(srchdl > 0)
		CloseHandle(srchdl);
#endif
// } AHTOXA
	THROW_V(fin  = fopen(src, "rb"), SLERR_OPENFAULT);
	THROW_V(fout = fopen(dest, "w+b"), SLERR_OPENFAULT);
	fstat(fileno(fin), &statbuf);
	statbuf.st_size >>= 10;
// AHTOXA {
#ifndef __WIN32__
// } AHTOXA
	getFileTime(fileno(fin), &creation_time, &last_access_time, &last_modif_time);
// AHTOXA {
#endif
	memzero(buf, sizeof(buf));
	memzero(&header, sizeof(header));
	if(!isdecomp) {
		char   f_ext[16];
		memzero(f_ext, sizeof(f_ext));
		// calculate crc (isdecomp == 0)
		header.FileLModDateTime = last_modif_time;
		fnsplit(src, 0, 0, header.FileName, f_ext);
		strcat(header.FileName, f_ext);
		while((in_len = fread(buf, sizeof(buf), 1, fin)) > 0)
			header.CRC = _crc32.Calc(header.CRC, (uint8 *)buf, (size_t)in_len);
		header.FileSize = SKILOBYTE(statbuf.st_size);
		fseek(fin, 0L, SEEK_SET);
		fwrite(&header, 1, sizeof(header), fout);
		in_len = 0;
	}
	else {
		fread(&header, 1, sizeof(header), fin);
	}
// } AHTOXA
	while(1) {
		if(isdecomp) {
	   		fread(&new_len, 1, sizeof(new_len), fin);
			old_size += sizeof(new_len);
		}
		else
			new_len = BUFLEN;
		if((in_len = fread(inbuf, 1, (size_t) new_len, fin)) == 0) {
			r = 1;
			break;
		}
		if(isdecomp) {
			THROW(lzo1x_decompress(inbuf, in_len, outbuf, &out_len, NULL) == LZO_E_OK);
		}
		else {
			THROW(lzo1x_1_compress(inbuf, in_len, outbuf, &out_len, wrkmem) == LZO_E_OK);
		}
		old_size += in_len;
		if(pf) {
			THROW_V(pf((old_size >> 10), statbuf.st_size, src, !isdecomp), SLERR_USERBREAK);
		}
		if(!isdecomp) {
			fwrite(&out_len, 1, sizeof(out_len), fout);
			new_size += sizeof(out_len);
		}
	   	fwrite(outbuf, 1, (size_t) out_len, fout);
		new_size += out_len;
	}
	// AHTOXA {
	// check CRC
	if(isdecomp) {
		ulong crc = 0;
		fseek(fout, 0L, SEEK_SET);
		while((out_len = fread(buf, sizeof(buf), 1, fout)) > 0)
			crc = _crc32.Calc(crc, (uint8 *)buf, (size_t)out_len);
		THROW_V((long)crc == header.CRC, SLERR_INVALIDCRC);
	}
	if(fout) {
#ifndef __WIN32__
		old_size = (long)setFileTime(fileno(fout), &creation_time, &last_access_time, &last_modif_time);
#endif
		fclose(fout);
		fout = 0;
#ifdef __WIN32__
		srchdl = CreateFile(dest, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); // @unicodeproblem
		THROW_V(srchdl >= 0, SLERR_OPENFAULT);
		setFileTime((int)srchdl, &creation_time, &last_access_time, &last_modif_time);
		if(srchdl > 0)
			CloseHandle(srchdl);
#endif
	}
	// } AHTOXA
	CATCH
		if(fout) {
			fclose(fout);
			fout = 0;
			SFile::Remove(dest);
		}
		r = 0;
	ENDCATCH
	if(r && sz)
		*sz = new_size;
	if(fin)
		fclose(fin);
	if(fout)
		fclose(fout);
	ZFREE(wrkmem);
	SAlloc::F(inbuf);
	SAlloc::F(outbuf);
  	return r;
}
