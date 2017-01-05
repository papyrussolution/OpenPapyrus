// PPVER.CPP
// Copyright (c) A.Sobolev, A.Starodub 2001, 2002, 2003, 2005
// @Kernel
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
#include <idea.h>

SLAPI PPVerT::PPVerT(int j, int n, int r)
{
	Set(j, n, r);
}

int SLAPI PPVerT::Get(int * pJ, int * pN, int * pR) const
{
	int j = (int)(V >> 8);
	int n = (int)(V & 0x00ff);
	int r = (int)R;
	ASSIGN_PTR(pJ, j);
	ASSIGN_PTR(pN, n);
	ASSIGN_PTR(pR, r);
	return 1;
}

int SLAPI PPVerT::Set(int j, int n, int r)
{
	V = (((uint16)j) << 8) | ((uint16)n);
	R = (uint16)r;
	return 1;
}

int SLAPI PPVerT::IsLt(int j, int n, int r) const
{
	PPVerT v2(j, n, r);
	if(V < v2.V)
		return 1;
	if(V == v2.V && R < v2.R)
		return 1;
	return 0;
}

int SLAPI PPVerT::IsGt(int j, int n, int r) const
{
	PPVerT v2(j, n, r);
	if(V > v2.V)
		return 1;
	if(V == v2.V && R > v2.R)
		return 1;
	return 0;
}

int SLAPI PPVerT::IsEq(int j, int n, int r) const
{
	PPVerT v2(j, n, r);
	return (V == v2.V && R == v2.R) ? 1 : 0;
}
//
//
//
struct __VerInfo {
	char   ProductName[64];
	char   ProductTeam[128];
	char   Demo[8];
	char   Secret[32];
	uint16 StartDevYear;
	uint16 LastDevYear;
	uint16 MajorVer;
	uint16 MinorVer;
	uint16 Revision;
	char   RegNumber[64];
	int16  MaxUsers; // -1 - unlimited
};

SLAPI PPVersionInfo::PPVersionInfo(char * pOuterFileName)
{
	if(pOuterFileName)
		P_OuterFileName = newStr(pOuterFileName);
	else
		P_OuterFileName = 0;
	P_Info = 0;
}

SLAPI PPVersionInfo::~PPVersionInfo()
{
	Clear();
	delete P_OuterFileName;
}

int SLAPI PPVersionInfo::Clear()
{
	if(P_Info) {
		IdeaRandMem(P_Info, sizeof(__VerInfo));
		ZFREE(P_Info);
	}
	return 1;
}

static const unsigned char VERDATA[] = {
	#include <private\verdata.inc>
};

int SLAPI PPVersionInfo::Decrypt()
{
	if(P_Info)
		return 1;
	int    ok = 1;
	__VerInfo * p_info = 0;
	size_t dl = 0;
	uint16 struc_size = 0;
	unsigned char * p_data = 0;
	unsigned char   b[256];
	FILE * p_file = 0;
	long   offs = 0L;
	if(P_OuterFileName) {
		p_file = fopen(P_OuterFileName, "rb");
		if(p_file) {
			THROW_PP((offs = SearchStrInFile(p_file, 0L, (char*)VERDATA, 0)) >= 0, PPERR_VERINFONFOUND);
			fseek(p_file, offs, SEEK_SET);
			SLibError = SLERR_READFAULT;
			THROW_PP(fread(b, 64, 1, p_file) == 1, PPERR_SLIB);
			struc_size = *(uint16*)(b+strlen((char*)b)+1);
			THROW_MEM(p_data = (unsigned char*)malloc(struc_size));
			SLibError = SLERR_READFAULT;
			fseek(p_file, offs, SEEK_SET);
			THROW_PP(fread(p_data, struc_size, 1, p_file) == 1, PPERR_SLIB);
		}
	}
	else {
		p_data = (unsigned char *)VERDATA;
		struc_size = sizeof(VERDATA);
	}

	THROW_MEM(P_Info = malloc(sizeof(__VerInfo)));
	p_info = (__VerInfo*)P_Info;

	memcpy(b, p_data, struc_size);
	dl = strlen((char*)b)+1+sizeof(struc_size);
	IdeaDecrypt((char*)b, (char*)b+dl, struc_size-dl);

	STRNSCPY(p_info->ProductName, (char*)b+dl); dl += strlen((char*)b+dl)+1;
	STRNSCPY(p_info->ProductTeam, (char*)b+dl); dl += strlen((char*)b+dl)+1;
	STRNSCPY(p_info->Demo, (char*)b+dl);        dl += strlen((char*)b+dl)+1;
	STRNSCPY(p_info->Secret, (char*)b+dl);      dl += strlen((char*)b+dl)+1;
	p_info->StartDevYear = *(uint16*)(b+dl); dl += sizeof(uint16);
	p_info->LastDevYear = *(uint16*)(b+dl);  dl += sizeof(uint16);
	p_info->MajorVer = *(uint16*)(b+dl);     dl += sizeof(uint16);
	p_info->MinorVer = *(uint16*)(b+dl);     dl += sizeof(uint16);
	p_info->Revision = *(uint16*)(b+dl);     dl += sizeof(uint16);
	p_info->MaxUsers = -1;
	IdeaRandMem(b, sizeof(b));
	CATCH
		ok = 0;
	ENDCATCH
	if(P_OuterFileName)
		free(p_data);
	if(p_file)
		fclose(p_file);
	return ok;
}

int SLAPI PPVersionInfo::GetProductName(char * pBuf, size_t bufSize)
{
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		strnzcpy(pBuf, p_info->ProductName, bufSize);
		Clear();
		return 1;
	}
	else {
		pBuf[0] = 0;
		return 0;
	}
}

int SLAPI PPVersionInfo::GetTeam(char * pBuf, size_t bufSize)
{
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		strnzcpy(pBuf, p_info->ProductTeam, bufSize);
		Clear();
		return 1;
	}
	else {
		pBuf[0] = 0;
		return 0;
	}
}

int SLAPI PPVersionInfo::GetSecret(char * pBuf, size_t bufSize)
{
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		strnzcpy(pBuf, p_info->Secret, bufSize);
		Clear();
		return 1;
	}
	else {
		pBuf[0] = 0;
		return 0;
	}
}

int SLAPI PPVersionInfo::GetDevYears(uint * pStart, uint * pLast)
{
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		ASSIGN_PTR(pStart, p_info->StartDevYear);
		ASSIGN_PTR(pLast, p_info->LastDevYear);
		Clear();
		return 1;
	}
	else {
		ASSIGN_PTR(pStart, 0);
		ASSIGN_PTR(pLast, 0);
		return 0;
	}
}

PPVerT SLAPI PPVersionInfo::GetVersion()
{
	uint j, n, r;
	PPVerT v;
	if(GetVersion(&j, &n, &r))
		v.Set((int)j, (int)n, (int)r);
	return v;
}

int SLAPI PPVersionInfo::GetVersion(uint * pMajor, uint * pMinor, uint * pRevision, char * pDemo)
{
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		ASSIGN_PTR(pMajor, p_info->MajorVer);
		ASSIGN_PTR(pMinor, p_info->MinorVer);
		ASSIGN_PTR(pRevision, p_info->Revision);
		if(pDemo)
			strcpy(pDemo, p_info->Demo);
		Clear();
		return 1;
	}
	else {
		ASSIGN_PTR(pMajor, 0);
		ASSIGN_PTR(pMinor, 0);
		ASSIGN_PTR(pRevision, 0);
		if(pDemo)
			pDemo[0] = 0;
		return 0;
	}
}

int SLAPI PPVersionInfo::GetMaxUserNumber(int * pMaxUsers)
{
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		ASSIGN_PTR(pMaxUsers, p_info->MaxUsers);
		Clear();
		return 1;
	}
	else {
		ASSIGN_PTR(pMaxUsers, 0);
		return 0;
	}
}

int SLAPI PPVersionInfo::GetVersionText(char * pBuf, size_t bufSize)
{
	char b[64];
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		sprintf(b, "%d.%d.%d", p_info->MajorVer, p_info->MinorVer, p_info->Revision);
		strcat(b, p_info->Demo);
		strnzcpy(pBuf, b, bufSize);
		Clear();
		return 1;
	}
	else {
		pBuf[0] = 0;
		return 0;
	}
}

int SLAPI PPVersionInfo::GetCopyrightText(char * pBuf, size_t bufSize)
{
	char b[256];
	if(Decrypt()) {
		__VerInfo * p_info = (__VerInfo*)P_Info;
		sprintf(b, "%s Version %d.%d.%d%s  (c) %s %d-%d",
			p_info->ProductName, (int)p_info->MajorVer, (int)p_info->MinorVer,
			(int)p_info->Revision, p_info->Demo, p_info->ProductTeam,
			(int)p_info->StartDevYear, (int)p_info->LastDevYear);
		strnzcpy(pBuf, b, bufSize);
		Clear();
		return 1;
	}
	else {
		pBuf[0] = 0;
		return 0;
	}
}
