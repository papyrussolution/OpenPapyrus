/*
        MAKEPATH.C
        Copyright (c) 1987, 1992 by Borland International
        All Rights Reserved.
 */
#include <slib.h>
#include <tv.h>
#pragma hdrstop

static void __makepath(register char * pathP, const char * driveP, const char * dirP, const char * nameP, const char * extP)
{
	if(driveP && *driveP) {
		*pathP++ = *driveP++;
		*pathP++ = ':';
	}
	if(dirP && *dirP) {
		pathP = stpcpy(pathP, dirP);
		if(*(pathP-1) != '\\' && *(pathP-1) != '/')
			*pathP++ = '\\';
	}
	if(nameP)
		pathP = stpcpy(pathP, nameP);
	if(extP && *extP) {
		if(*extP != '.')
			*pathP++ = '.';
		pathP = stpcpy(pathP, extP);
	}
	*pathP = 0;
}

void fnmerge(register char * pathP, const char * driveP, const char * dirP, const char * nameP, const char * extP)
{
	__makepath(pathP, driveP, dirP, nameP, extP);
}
//
//
//
#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE     0x10

static int DotFound(char * pB)
{
	if(*(pB-1) == '.')
		pB--;
	switch(*--pB) {
	    case ':':
		if(*(pB-2) != '\0')
			break;
	    case '/':
	    case '\\':
	    case '\0':
		return 1;
	}
	return 0;
}

int _fnsplit(const char * pathP, char * driveP, char * dirP, char * nameP, char * extP)
{
	register char * pB;
	register int Wrk;
	int    Ret;
	char   buf[MAXPATH+2];
	/*
	        Set all string to default value zero
	 */
	Ret = 0;
	ASSIGN_PTR(driveP, 0);
	ASSIGN_PTR(dirP, 0);
	ASSIGN_PTR(nameP, 0);
	ASSIGN_PTR(extP, 0);
	/*
	        Copy filename into template up to MAXPATH characters
	 */
	pB = buf;
	while(*pathP == ' ')
		pathP++;
	if((Wrk = strlen(pathP)) > MAXPATH)
		Wrk = MAXPATH;
	*pB++ = 0;
	strncpy(pB, pathP, Wrk);
	*(pB += Wrk) = 0;
	/*
	        Split the filename and fill corresponding nonzero pointers
	 */
	Wrk = 0;
	for(; ;) {
		switch(*--pB) {
		    case '.':
			if(!Wrk && (*(pB+1) == '\0'))
				Wrk = DotFound(pB);
			if((!Wrk) && ((Ret&EXTENSION) == 0)) {
				Ret |= EXTENSION;
				strnzcpy(extP, pB, MAXEXT);
				*pB = 0;
			}
			continue;
		    case ':':
			if(pB != &buf[2])
				continue;
		    case '\0':
			if(Wrk) {
				if(*++pB)
					Ret |= DIRECTORY;
				strnzcpy(dirP, pB, MAXDIR);
				*pB-- = 0;
				break;
			}
		    case '/':
		    case '\\':
			if(!Wrk) {
				Wrk++;
				if(*++pB)
					Ret |= FILENAME;
				strnzcpy(nameP, pB, MAXFILE);
				*pB-- = 0;
				if(*pB == 0 || (*pB == ':' && pB == &buf[2]))
					break;
			}
			continue;
		    case '*':
		    case '?':
			if(!Wrk)
				Ret |= WILDCARDS;
		    default:
			continue;
		}
		break;
	}
	if(*pB == ':') {
		if(buf[1])
			Ret |= DRIVE;
		strnzcpy(driveP, &buf[1], MAXDRIVE);
	}
	return Ret;
}

int fnsplit(const char * pathP, char * driveP, char * dirP, char * nameP, char * extP)
{
	return _fnsplit(pathP, driveP, dirP, nameP, extP);
}

