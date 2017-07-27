// SOAPDBT.CPP
// Copyright (c) A.Starodub 2007, 2008, 2010, 2015, 2017
//
#include <db.h>
#pragma hdrstop
//
// @ModuleDef(SoapDbFile)
//
SoapDbFile::Param::Param(const char * pRootTag, const char * pHeadTag, const char * pRecTag)
{
	Init(pRootTag, pHeadTag, pRecTag);
}

int SoapDbFile::Param::Init(const char * pRootTag, const char * pHeadTag, const char * pRecTag)
{
	RootTag.CopyFrom(pRootTag);
	HeadTag.CopyFrom(pHeadTag);
	RecTag.CopyFrom(pRecTag);
	return 1;
}

SoapDbFile::SoapDbFile()
{
	Pos = 0;
	NumRecs = 0;
}

SoapDbFile::~SoapDbFile()
{
	Close();
}

int SoapDbFile::Open(const char * pPath, const Param * pParam, int readOnly)
{
	int ok = 0;
	Pos = 0;
	if(!RVALUEPTR(P, pParam))
		P.Init(0, 0, 0);
	if(!readOnly) {
	}
	else {
	}
	if(ok == 0) {
		SLS.SetAddedMsgString(pPath);
		SLibError = SLERR_OPENFAULT;
	}
	return ok;
}

int SoapDbFile::Close()
{
	Pos = 0;
	return 1;
}

int SoapDbFile::GetRecord(const SdRecord & rRec, void * pDataBuf)
{
	int    ok = -1;
	// THROW(CheckParam(rRec));
	if(Pos >= 0 && Pos < NumRecs) {
		SFormatParam fp;
		SdbField fld;
		for(uint fld_pos = 0; rRec.GetFieldByPos(fld_pos, &fld) > 0; fld_pos++) {
			int r = 0;
			SString field_buf;
			if(r > 0) {
				THROW(fld.PutFieldDataToBuf(field_buf, pDataBuf, fp));
			}
		}
		ok = 1;
		Pos++;
	}
	CATCHZOK
	return ok;
}

int SoapDbFile::AppendRecord(const SdRecord & rRec, const void * pDataBuf)
{
	int    ok = -1;
	SFormatParam fp;
	fp.Flags |= SFormatParam::fFloatSize;
	// THROW(CheckParam(rRec));
	if(Pos == 0) {
	}
	for(uint i = 0; i < rRec.GetCount(); i++) {
		SString line, field_name;
		SdbField fld;
		THROW(rRec.GetFieldByPos(i, &fld));
		field_name = fld.Name;
		if(!field_name.NotEmptyS())
			field_name.Cat(fld.ID);
		fld.GetFieldDataFromBuf(line, pDataBuf, fp);
	}
	ok = 1;
	Pos++;
	CATCHZOK
	return ok;
}

int SoapDbFile::GetNumRecords(ulong * pNumRecs)
{
	int    ok = 1;
	long   num_recs = 0;
	num_recs = NumRecs;
	ASSIGN_PTR(pNumRecs, num_recs);
	return ok;
}
