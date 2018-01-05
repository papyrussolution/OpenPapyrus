// STYLOPALM.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2011, 2015, 2018
//
#pragma hdrstop
#include <stylopalm.h>
#include <ctype.h>

const char * P_PalmConfigFileName     = "palmcfg.bin";
const char * P_StyloPalmCreatorID     = "SPII";
const char * P_PalmArcTblName         = "SpiiUcl.pdb";
const char * P_PalmPackedDataTblName  = "SpiiPD.pdb";
const char * P_PalmProgramFileName    = "StyloWce.exe";  // обновление файлов поддерживают только устройства с ОС Windows Mobile
extern const char * P_PalmDllFileName = "TodayItem.dll"; // обновление файлов поддерживают только устройства с ОС Windows Mobile

PalmConfig::PalmConfig()
{
	Init();
}

void PalmConfig::Init()
{
	THISZERO();
	Size = sizeof(*this);
	Ver = 7;
	SendBufSize = PALMPACKRECLEN;
	RecvBufSize = PALMARCBUFSIZE;
}

int PalmConfig::Write(const char * pPath)
{
	int    ok = 1;
#if !defined(__palmos__) && !defined(_WIN32_WCE) // {
	SString fname;
	SPathStruc ps(pPath);
	if(ps.Ext.Len())
		fname = pPath;
	else {
		ps.Nam = P_PalmConfigFileName;
		ps.Merge(fname);
	}
	FILE * f = fopen(fname, "wb");
	if(f) {
		if(fwrite(this, sizeof(*this), 1, f) != 1)
			ok = 0;
		SFile::ZClose(&f);
	}
	else
		ok = 0;
#endif // } !__palmos__ && !_WIN32_WCE
	return ok;
}

int PalmConfig::Read(const char * pPath)
{
	int    ok = 1;
#if !defined(__palmos__) &&  !defined(_WIN32_WCE) // {
	SString fname;
	SPathStruc ps(pPath);
	if(ps.Ext.Len())
		fname = pPath;
	else
		(fname = pPath).SetLastSlash().Cat(P_PalmConfigFileName);
	FILE * f = fopen(fname, "rb");
	if(f) {
		if(fread(this, sizeof(*this), 1, f) != 1)
			ok = 0;
		SFile::ZClose(&f);
	}
	else
		ok = 0;
#endif // } !__palmos__ && !_WIN32_WCE
	return ok;
}
//
//
//
int PalmArcHdr::Check() const
{
	if(Ver != 1)
		return 0;
	if(NumRecs > PALM_MAXRECCOUNT)
		return 0;
	size_t i;
	for(i = 0; i < sizeof(Reserve); i++)
		if(Reserve[i] != 0)
			return 0;
#if !defined(__palmos__)  && !defined(_WIN32_WCE) // {
	const size_t len = strlen(Name);
	for(i = 0; i < len; i++)
		if(!isalpha(Name[i]) && Name[i] != '.' && Name[i] != '_')
			return 0;
#endif // } !__palmos__ && !_WIN32_WCE
	return 1;
}
