// SHTRIH_DPD201.CPP
// Библиотека для работы с дисплеем покупателя Shtrih-DPD201
//

#include <slib.h>
#include <CustomDisplay.h>

#define STRLEN		20 // Число символов в строке

class ShtrhDpdEquip : public CustomDisplayEquip {
public:
	int GetConfig(char * pBuf, size_t bufSize);
	int PrintLine();
	int Connect(); // new
private:
	int ConvertStrTo850CodePage(SString & rStr);
};

static ShtrhDpdEquip * P_ShtrhDpdDisp = 0;

static const uchar RusChar850[32] =
	{0x41, 0xB0, 0x42, 0xB1, 0xB2, 0x45, 0xB3, 0xB4, 0xB8, 0xB9, 0x4B, 0xBA, 0x4D, 0x48, 0x4F, 0xBB,
	 0x50, 0x43, 0x54, 0xBC, 0xBF, 0x58, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0x62, 0xC8, 0xC9, 0xCA};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return true; 
}

EXPORT int /*STDAPICALLTYPE*/ RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	THROW((pCmd != NULL) && (pOutputData != NULL) && (outSize != NULL));
	if(sstreqi_ascii(pCmd, "INIT"))
		THROWERR(Init(), CUSTDISP_NOTINITED)
	else if(sstreqi_ascii(pCmd, "RELEASE"))
		THROW(Release())
	else if(P_ShtrhDpdDisp) {
		ok = P_ShtrhDpdDisp->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
		if(ok == 0) {
			if(sstreqi_ascii(pCmd, "GETCONFIG"))
				ok = P_ShtrhDpdDisp->GetConfig(pOutputData, outSize);
			else if(sstreqi_ascii(pCmd, "PUTLINE"))
				THROW(P_ShtrhDpdDisp->PrintLine() == 0)
			// new {
			else if(sstreqi_ascii(pCmd, "CONNECT")) {
				THROWERR(P_ShtrhDpdDisp->Connect(), CUSTDISP_NOTCONNECTED);
			}
			// } new
		}
	}
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		P_ShtrhDpdDisp->LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}

int Init() 
{
	SETIFZ(P_ShtrhDpdDisp, new ShtrhDpdEquip);
	return 1;
}

int Release() 
{	
	ZDELETE(P_ShtrhDpdDisp);
	return 1;
}

int ShtrhDpdEquip::GetConfig(char * pBuf, size_t bufSize)
{
	int ok = 0;
	SString str;
	str.CatEq("STRLEN", STRLEN);
	if(str.Len() + 1 > bufSize)
		ok = 2;
	else
		memcpy(pBuf, str, str.Len() + 1);
	return ok;
}

int ShtrhDpdEquip::PrintLine() 
{
	int ok = 0;
	if(VerTab == VERT_UPP || VerTab == VERT_DOWN) {
		THROWERR(CommPort.PutChr(0x0b), CUSTDISP_PRINTTOPORT);
		if(VerTab == VERT_DOWN)
			THROWERR(CommPort.PutChr(0x0a), CUSTDISP_PRINTTOPORT);
	}
	if(Align == LEFT)
		Text.Align(STRLEN, ADJ_LEFT);
	else if(Align == CENTER)
		Text.Align(STRLEN, ADJ_CENTER);
	else if(Align == RIGHT)
		Text.Align(STRLEN, ADJ_RIGHT);
	ConvertStrTo850CodePage(Text);
	for(uint i = 0; i < Text.Len(); i++)
		THROWERR(CommPort.PutChr(Text.C(i)), CUSTDISP_PRINTTOPORT);
	CATCH
		ok = 1;
	ENDCATCH;
	ClearParams();
	return ok;
}

int ShtrhDpdEquip::ConvertStrTo850CodePage(SString & rStr)
{
	int  ok = -1;
	SString str;
	if(rStr.NotEmpty()) {
		rStr.ToUpper();
		for(uint i = 0; i < rStr.Len(); i++) {
			uchar c = rStr.C(i);
			if(c >= 0x80 && c < 0xA0)
				str.CatChar(RusChar850[c - 0x80]);
			else
				str.CatChar(c);
		}
		rStr.Z().Cat(str);
		ok = 1;
	}
	return ok;
}

// new {
int ShtrhDpdEquip::Connect() { return CommPort.InitPort(Port, 0, 0); }
// } new