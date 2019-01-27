// PPVER.CPP
// Copyright (c) A.Sobolev, A.Starodub 2001, 2002, 2003, 2005, 2007, 2008, 2011, 2013, 2014, 2015, 2016, 2017, 2019
// @Kernel
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>
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
	uint16 MinMajorVer;
	uint16 MinMinorVer;
	uint16 MinRevision;
	char   OpenSource[8];
};

SLAPI PPVersionInfo::PPVersionInfo(const char * pOuterFileName) : P_Info_(0), P_Info_Pre9303(0), P_OuterFileName(newStr(pOuterFileName))
{
}

SLAPI PPVersionInfo::PPVersionInfo(const PPVersionInfo & s) : P_Info_(0), P_Info_Pre9303(0), P_OuterFileName(0)
{
	if(s.P_OuterFileName)
		P_OuterFileName = newStr(s.P_OuterFileName);
	else
		ZDELETE(P_OuterFileName);
}

SLAPI PPVersionInfo::~PPVersionInfo()
{
	Clear();
	delete P_OuterFileName;
}

PPVersionInfo & FASTCALL PPVersionInfo::operator = (const PPVersionInfo & s)
{
	Clear();
	if(s.P_OuterFileName)
		P_OuterFileName = newStr(s.P_OuterFileName);
	else
		ZDELETE(P_OuterFileName);
	return *this;
}

void SLAPI PPVersionInfo::Clear()
{
	if(P_Info_Pre9303) {
		IdeaRandMem(P_Info_Pre9303, sizeof(__VerInfo));
		ZFREE(P_Info_Pre9303);
	}
	ZDELETE(P_Info_);
}

static const unsigned char VERDATA[] = {
	#include <private\verdata.inc>
};

int SLAPI PPVersionInfo::Decrypt()
{
	int    ok = 1;
	uchar * p_data_pre9303 = 0;
	__VerInfo * p_info_pre9303 = 0;
	FILE * p_file = 0;
	if(!P_Info_ && !P_Info_Pre9303) {
		//PapyrusPrivateBlock ppb;
		//ppb.RestoreBuffer()
		size_t dl = 0;
		uint16 struc_size = 0;
		uchar  b[256];
		long   offs = 0L;
		if(P_OuterFileName) {
			p_file = fopen(P_OuterFileName, "rb");
			if(p_file) {
				offs = SearchStrInFile(p_file, 0L, (const char *)(PTR32C(VERDATA)+1), 0);
				if(offs > (long)sizeof(uint32)) {
					STempBuffer tbuf(sizeof(VERDATA));
					THROW_SL(tbuf.IsValid());
					fseek(p_file, offs, SEEK_SET);
					SLibError = SLERR_READFAULT;
					THROW_SL(fread(tbuf, sizeof(PapyrusPrivateBlock::Header), 1, p_file) == 1);
					{
						const size_t _buf_size = ((const PapyrusPrivateBlock *)tbuf.ucptr())->H.RawBufferSize;
						THROW_SL(tbuf.Alloc(_buf_size));
						fseek(p_file, offs-sizeof(uint32), SEEK_SET);
						SLibError = SLERR_READFAULT;
						THROW_SL(fread(tbuf, _buf_size, 1, p_file) == 1);
						THROW_MEM(P_Info_ = new PapyrusPrivateBlock);
						THROW(P_Info_->RestoreBuffer(tbuf, _buf_size));
					}
				}
				else {
					SString old_sign;
					// Что бы полная строка не могла быть найдена в результирующем файле делим на 2 части.
					old_sign.Cat("65426832").Cat("10565340");
					offs = SearchStrInFile(p_file, 0L, old_sign, 0);
					if(offs >= 0) {
						fseek(p_file, offs, SEEK_SET);
						SLibError = SLERR_READFAULT;
						THROW_SL(fread(b, 64, 1, p_file) == 1);
						struc_size = *(uint16 *)(b+sstrlen((char*)b)+1);
						THROW_MEM(p_data_pre9303 = (uchar *)SAlloc::M(struc_size));
						SLibError = SLERR_READFAULT;
						fseek(p_file, offs, SEEK_SET);
						THROW_SL(fread(p_data_pre9303, struc_size, 1, p_file) == 1);
						//
						THROW_MEM(P_Info_Pre9303 = SAlloc::M(sizeof(__VerInfo)));
						p_info_pre9303 = (__VerInfo*)P_Info_Pre9303;

						memcpy(b, p_data_pre9303, struc_size);
						dl = sstrlen((char*)b)+1+sizeof(struc_size);
						IdeaDecrypt((char*)b, (char*)b+dl, struc_size-dl);

						STRNSCPY(p_info_pre9303->ProductName, (char*)b+dl); dl += sstrlen((char*)b+dl)+1;
						STRNSCPY(p_info_pre9303->ProductTeam, (char*)b+dl); dl += sstrlen((char*)b+dl)+1;
						STRNSCPY(p_info_pre9303->Demo, (char*)b+dl);        dl += sstrlen((char*)b+dl)+1;
						STRNSCPY(p_info_pre9303->Secret, (char*)b+dl);      dl += sstrlen((char*)b+dl)+1;
						p_info_pre9303->StartDevYear = *(uint16*)(b+dl); dl += sizeof(uint16);
						p_info_pre9303->LastDevYear = *(uint16*)(b+dl);  dl += sizeof(uint16);
						p_info_pre9303->MajorVer = *(uint16*)(b+dl);     dl += sizeof(uint16);
						p_info_pre9303->MinorVer = *(uint16*)(b+dl);     dl += sizeof(uint16);
						p_info_pre9303->Revision = *(uint16*)(b+dl);     dl += sizeof(uint16);
						p_info_pre9303->MaxUsers = -1;
						//
						p_info_pre9303->MinMajorVer = *(uint16*)(b+dl);     dl += sizeof(uint16);
						p_info_pre9303->MinMinorVer = *(uint16*)(b+dl);     dl += sizeof(uint16);
						p_info_pre9303->MinRevision = *(uint16*)(b+dl);     dl += sizeof(uint16);
						IdeaRandMem(b, sizeof(b));
					}
					else
						CALLEXCEPT_PP_S(PPERR_VERINFONFOUND, P_OuterFileName);
				}
			}
		}
		else {
			THROW_MEM(P_Info_ = new PapyrusPrivateBlock);
			THROW(P_Info_->RestoreBuffer(VERDATA, sizeof(VERDATA)));
			THROW(P_Info_->H.RawBufferSize == sizeof(VERDATA));
		}
	}
	CATCH
		ZDELETE(P_Info_);
		ZDELETE(P_Info_Pre9303);
		ok = 0;
	ENDCATCH
	if(P_OuterFileName)
		SAlloc::F(p_data_pre9303);
	SFile::ZClose(&p_file);
	return ok;
}

int SLAPI PPVersionInfo::GetProductName(SString & rBuf)
{
	if(Decrypt()) {
		if(P_Info_Pre9303) {
			__VerInfo * p_info = (__VerInfo*)P_Info_Pre9303;
			rBuf = p_info->ProductName;
		}
		else if(P_Info_) {
			rBuf = P_Info_->ProductName;
		}
		Clear();
		return 1;
	}
	else {
		rBuf.Z();
		return 0;
	}
}

int SLAPI PPVersionInfo::GetTeam(char * pBuf, size_t bufSize)
{
	if(Decrypt()) {
		if(P_Info_Pre9303) {
			__VerInfo * p_info = (__VerInfo*)P_Info_Pre9303;
			strnzcpy(pBuf, p_info->ProductTeam, bufSize);
		}
		else if(P_Info_) {
			strnzcpy(pBuf, P_Info_->Team, bufSize);
		}
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
		if(P_Info_Pre9303) {
			__VerInfo * p_info = (__VerInfo*)P_Info_Pre9303;
			strnzcpy(pBuf, p_info->Secret, bufSize);
		}
		else if(P_Info_) {
			strnzcpy(pBuf, P_Info_->Secret, bufSize);
		}
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
		if(P_Info_Pre9303) {
			__VerInfo * p_info = (__VerInfo*)P_Info_Pre9303;
			ASSIGN_PTR(pStart, p_info->StartDevYear);
			ASSIGN_PTR(pLast, p_info->LastDevYear);
		}
		else if(P_Info_) {
			ASSIGN_PTR(pStart, P_Info_->StartDevYear);
			ASSIGN_PTR(pLast, P_Info_->LastDevYear);
		}
		Clear();
		return 1;
	}
	else {
		ASSIGN_PTR(pStart, 0);
		ASSIGN_PTR(pLast, 0);
		return 0;
	}
}

SVerT SLAPI PPVersionInfo::GetVersion(int minVersion /* = 0 */)
{
	uint j, n, r;
	SVerT v;
	if(GetVersion(&j, &n, &r, 0, minVersion))
		v.Set((int)j, (int)n, (int)r);
	return v;
}

int SLAPI PPVersionInfo::GetAssemblyN()
{
	int    result = 0;
	if(Decrypt()) {
		if(P_Info_Pre9303) {
		}
		else if(P_Info_) {
			result = P_Info_->AssemblyN;
		}
	}
	return result;
}

int SLAPI PPVersionInfo::GetVersion(uint * pMajor, uint * pMinor, uint * pRevision, char * pDemo, int minVersion /* = 0 */)
{
	if(Decrypt()) {
		if(P_Info_Pre9303) {
			const __VerInfo * p_info = (const __VerInfo *)P_Info_Pre9303;
			ASSIGN_PTR(pMajor,    (minVersion) ? p_info->MinMajorVer : p_info->MajorVer);
			ASSIGN_PTR(pMinor,    (minVersion) ? p_info->MinMinorVer : p_info->MinorVer);
			ASSIGN_PTR(pRevision, (minVersion) ? p_info->MinRevision : p_info->Revision);
			strnzcpy(pDemo, p_info->Demo, 8);
		}
		else if(P_Info_) {
			int    mj = 0, mn = 0, rev = 0;
			if(minVersion) {
				P_Info_->MinVer.Get(&mj, &mn, &rev);
			}
			else {
				P_Info_->Ver.Get(&mj, &mn, &rev);
			}
			ASSIGN_PTR(pMajor,    (uint)mj);
			ASSIGN_PTR(pMinor,    (uint)mn);
			ASSIGN_PTR(pRevision, (uint)rev);
			strnzcpy(pDemo, (P_Info_->Flags & P_Info_->fDemo) ? "demo" : 0, 8);
		}
		Clear();
		return 1;
	}
	else {
		ASSIGN_PTR(pMajor, 0);
		ASSIGN_PTR(pMinor, 0);
		ASSIGN_PTR(pRevision, 0);
		ASSIGN_PTR(pDemo, 0);
		return 0;
	}
}

/* @v9.3.3 int SLAPI PPVersionInfo::GetMaxUserNumber(int * pMaxUsers)
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
}*/

int SLAPI PPVersionInfo::GetVersionText(char * pBuf, size_t bufSize)
{
	if(Decrypt()) {
		SString temp_buf;
		if(P_Info_Pre9303) {
			const __VerInfo * p_info = (const __VerInfo*)P_Info_Pre9303;
			temp_buf.CatDotTriplet(p_info->MajorVer, p_info->MinorVer, p_info->Revision).Cat(p_info->Demo);
			temp_buf.CopyTo(pBuf, bufSize);
		}
		else if(P_Info_) {
			P_Info_->Ver.ToStr(temp_buf);
			if(P_Info_->Flags & P_Info_->fDemo) {
				temp_buf.Space().Cat("demo");
			}
			temp_buf.CopyTo(pBuf, bufSize);
		}
		Clear();
		return 1;
	}
	else {
		ASSIGN_PTR(pBuf, 0);
		return 0;
	}
}

int SLAPI PPVersionInfo::GetDefaultEncrKey(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	if(Decrypt()) {
		if(P_Info_)
			rBuf = P_Info_->DefPassword;
		else
			ok = 0;
		Clear();
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPVersionInfo::GetMsftTranslAcc(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	if(Decrypt()) {
		if(P_Info_)
			rBuf = P_Info_->MsftTranslAcc;
		else
			ok = 0;
		Clear();
	}
	else
		ok = 0;
	return ok;
}

long SLAPI PPVersionInfo::GetFlags()
{
	long   result = 0;
	if(Decrypt()) {
		if(P_Info_)
			result = P_Info_->Flags;
		Clear();
	}
	return result;
}

int SLAPI PPVersionInfo::GetCopyrightText(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	if(Decrypt()) {
		if(P_Info_Pre9303) {
			const __VerInfo * p_info = (const __VerInfo *)P_Info_Pre9303;
			rBuf.Cat(p_info->ProductName).Space().Cat("Version").Space().
				CatDotTriplet((int)p_info->MajorVer, (int)p_info->MinorVer, (int)p_info->Revision).
				Cat(p_info->Demo).Space().Space().CatParStr("c").Space().
				Cat(p_info->ProductTeam).Space().
				Cat((int)p_info->StartDevYear).CatChar('-').Cat((int)p_info->LastDevYear);
		}
		else if(P_Info_) {
			SString temp_buf;
			P_Info_->Ver.ToStr(temp_buf);
			rBuf.Cat(P_Info_->ProductName).Space().Cat("Version").Space().Cat(temp_buf);
			if(P_Info_->Flags & P_Info_->fDemo)
				rBuf.Space().Cat("demo");
			rBuf.Space().Space().CatParStr("c").Space().Cat(P_Info_->Team).Space().
				Cat((int)P_Info_->StartDevYear).CatChar('-').Cat((int)P_Info_->LastDevYear);
		}
		Clear();
	}
	else
		ok = 0;
	return ok;
}
