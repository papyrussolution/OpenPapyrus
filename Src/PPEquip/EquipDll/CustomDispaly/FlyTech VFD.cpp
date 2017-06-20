// FLYTECHVFD.CPP
// Библиотека для работы с дисплеем покупателя FlyTech VFD в режиме Epson
//

#include <slib.h>
#include <CustomDisplay.h>

#define STRLEN		20 // Число символов в строке

class FlyTechfdEquip : public CustomDisplayEquip {
public:
	int Connect();
	int GetConfig(char * pBuf, size_t bufSize);
	int PrintLine();
};

static FlyTechfdEquip * P_FlyTechVfdDisp = 0;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return true; 
}

EXPORT int RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int ok = 0;
	THROW(pCmd && pOutputData && outSize);
	if(sstreqi_ascii(pCmd, "INIT"))
		THROWERR(Init(), CUSTDISP_NOTINITED)
	else if(sstreqi_ascii(pCmd, "RELEASE"))
		THROW(Release())
	else if(P_FlyTechVfdDisp) {
		ok = P_FlyTechVfdDisp->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
		if(ok == 0) {
			if(sstreqi_ascii(pCmd, "GETCONFIG"))
				ok = P_FlyTechVfdDisp->GetConfig(pOutputData, outSize);
			else if(sstreqi_ascii(pCmd, "PUTLINE"))
				THROW(P_FlyTechVfdDisp->PrintLine() == 0)
			else if(sstreqi_ascii(pCmd, "CONNECT")) {
				THROWERR(P_FlyTechVfdDisp->Connect(), CUSTDISP_NOTCONNECTED);
			}
		}
	}
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		P_FlyTechVfdDisp->LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}

int Init() 
{
	if(!P_FlyTechVfdDisp)
		P_FlyTechVfdDisp = new FlyTechfdEquip;
	return 1;
}

int Release() 
{	
	if(P_FlyTechVfdDisp)
		ZDELETE(P_FlyTechVfdDisp);
	return 1;
}

int FlyTechfdEquip::GetConfig(char * pBuf, size_t bufSize)
{
	int ok = 0;
	SString str;
	str.CatEq("STRLEN", (long)STRLEN);
	if(str.Len() + 1 > bufSize)
		ok = 2;
	else
		memcpy(pBuf, str, str.Len() + 1);
	return ok;
}

int FlyTechfdEquip::PrintLine() 
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
	Text.ToUpper();
	for(uint i = 0; i < Text.Len(); i++)
		THROWERR(CommPort.PutChr(Text.C(i)), CUSTDISP_PRINTTOPORT);
	CATCH
		ok = 1;
	ENDCATCH;
	ClearParams();
	return ok;
}

int FlyTechfdEquip::Connect() {
	return CommPort.InitPort(Port);
}
