// SCSPCTRT.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Реализация специальных интерпретаций поведения персоанльных карт
//
#include <pp.h>
#pragma hdrstop

class SCardSpecialTreatment {
public:
	SCardSpecialTreatment()
	{
	}
	virtual ~SCardSpecialTreatment()
	{
	}
	virtual int VerifyOwner(const PPSCardPacket * pPack)
	{
		return -1;
	}
	virtual int QueryDiscount(const CCheckPacket * pCcPack, CCheckPacket * pRetCcPack, long * pRetFlags)
	{
		return -1;
	}
};

class SCardSpecialTreatment_AstraZeneca : public SCardSpecialTreatment {
public:
	SCardSpecialTreatment_AstraZeneca() : SCardSpecialTreatment()
	{
	}
	virtual ~SCardSpecialTreatment_AstraZeneca()
	{
	}
	virtual int VerifyOwner(const PPSCardPacket * pPack);
	virtual int QueryDiscount(const CCheckPacket * pCcPack, CCheckPacket * pRetCcPack, long * pRetFlags);
private:
	void MakeUrl(const char * pSuffix, SString & rBuf)
	{
		//https://astrazeneca.like-pharma.com/api/1.0/register/
		SString sfx;
		sfx = "api/1.0/";
		if(!isempty(pSuffix))
			sfx.Cat(pSuffix).SetLastDSlash();
		rBuf = InetUrl::MkHttps("astrazeneca.like-pharma.com", sfx);
	}
};

int SCardSpecialTreatment_AstraZeneca::VerifyOwner(const PPSCardPacket * pPack)
{
	return -1;
}

int SCardSpecialTreatment_AstraZeneca::QueryDiscount(const CCheckPacket * pCcPack, CCheckPacket * pRetCcPack, long * pRetFlags)
{
	return -1;
}