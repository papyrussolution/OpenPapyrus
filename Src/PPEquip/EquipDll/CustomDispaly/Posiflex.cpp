// POSIFLEX.CPP
// Библиотека для работы с дисплеем покупателя Posiflex
//
#include <slib.h>
#include <CustomDisplay.h>

#define STRLEN		20 // Число символов в строке

static const uchar CPTysso[32] =
	{0x41, 0xA0, 0x42, 0xA1, 0xE0, 0x45, 0xA3, 0xA4, 0xA5, 0xA6, 0x4B, 0xA7, 0x4D, 0x48, 0x4F, 0xA8,
	 0x50, 0x43, 0x54, 0xA9, 0xAA, 0x58, 0xE1, 0xAB, 0xAC, 0xE2, 0xAD, 0xAE, 0x62, 0xAF, 0xB0, 0xB1};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return true; 
}

class PosiflexEquip : public CustomDisplayEquip {
public:
	SString UsbDllPath;
	SDynLibrary * P_Lib;

	int GetConfig(char * pBuf, size_t bufSize);
	int PrintLine();
	int Connect(); // new
	int UsbConnect();
	int UsbDisconnect();
	int UsbPrintLine();
	int UsbClearDisplay();
	long (*Openu)();
	long (*Closeu)();
	long (*Writeu)(const char *, long);
	long (*Stateu)();
private:
	int SLAPI ConvertStrToTyssoCodePage(SString & rStr);
};

static PosiflexEquip * P_PosiflexDisp = 0;

EXPORT int /*STDAPICALLTYPE*/ RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int    ok = 0;
	THROW(pCmd != NULL && pOutputData != NULL && outSize != NULL);
	if(sstreqi_ascii(pCmd, "INIT")) {
		StringSet set_pairs(';', pInputData);
		SString params, s_param, param_val;
		THROWERR(Init(), CUSTDISP_NOTINITED);
		THROW(P_PosiflexDisp);
		for(uint i = 0; set_pairs.get(&i, params) > 0;) {
			params.Divide('=', s_param, param_val);
			if(s_param.CmpNC("DLLPATH") == 0)
				P_PosiflexDisp->UsbDllPath.Z().Cat(param_val).Cat("USBPD.DLL");
		}
	}
	else if(sstreqi_ascii(pCmd, "RELEASE"))
		THROW(Release())
	else if(P_PosiflexDisp){
		ok = P_PosiflexDisp->RunOneCommand(pCmd, pInputData, pOutputData, outSize);
		if(ok == 0) {
			if(sstreqi_ascii(pCmd, "GETCONFIG"))
				ok = P_PosiflexDisp->GetConfig(pOutputData, outSize);
			else if(sstreqi_ascii(pCmd, "PUTLINE")) {
				if(P_PosiflexDisp->Flags & F_USB)
					THROW(P_PosiflexDisp->UsbPrintLine() == 0)
				else
					THROW(P_PosiflexDisp->PrintLine() == 0)
			}
			else if(sstreqi_ascii(pCmd, "CONNECT")) {
				if(P_PosiflexDisp->Flags & F_USB) {
					THROWERR(P_PosiflexDisp->UsbConnect(), CUSTDISP_NOTCONNECTED)
				}
				else {
					THROWERR(P_PosiflexDisp->Connect(), CUSTDISP_NOTCONNECTED);
				}
			}
			else if(sstreqi_ascii(pCmd, "DISCONNECT")) {
				if(P_PosiflexDisp->Flags & F_USB)
					THROW(P_PosiflexDisp->UsbDisconnect());
			}
			else if(sstreqi_ascii(pCmd, "CLEARDISPLAY")) {
				if(P_PosiflexDisp->Flags & F_USB)
					THROW(P_PosiflexDisp->UsbClearDisplay());
			}
		}
	}
	CATCH
		ok = 1;
		_itoa(ErrorCode, pOutputData, 10);
		P_PosiflexDisp->LastError = ErrorCode;
		ErrorCode = 0;
	ENDCATCH;
	return ok;
}

int Init() 
{
	SETIFZ(P_PosiflexDisp, new PosiflexEquip);
	if(P_PosiflexDisp)
		P_PosiflexDisp->P_Lib = 0;
	return 1;
}

int Release() 
{	
	if(P_PosiflexDisp) {
		ZDELETE(P_PosiflexDisp->P_Lib);
		P_PosiflexDisp->Openu = 0;
		P_PosiflexDisp->Closeu = 0;
		P_PosiflexDisp->Stateu = 0;
		P_PosiflexDisp->Writeu = 0;
		ZDELETE(P_PosiflexDisp);
	}
	return 1;
}

int PosiflexEquip::GetConfig(char * pBuf, size_t bufSize)
{
	int    ok = 0;
	SString str;
	str.CatEq("STRLEN", (long)STRLEN);
	if(str.Len() + 1 > bufSize)
		ok = 2;
	else
		memcpy(pBuf, str, str.Len() + 1);
	return ok;
}

int PosiflexEquip::PrintLine() 
{
	int    ok = 0;
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
	ConvertStrToTyssoCodePage(Text);
	for(uint i = 0; i < Text.Len(); i++)
		THROWERR(CommPort.PutChr(Text.C(i)), CUSTDISP_PRINTTOPORT);
	CATCH
		ok = 1;
	ENDCATCH;
	ClearParams();
	return ok;
}

int SLAPI PosiflexEquip::ConvertStrToTyssoCodePage(SString & rStr)
{
	int  ok = -1;
	SString str;
	if(rStr) {
		rStr.ToUpper();
		for(uint i = 0; i < rStr.Len(); i++) {
			uchar c = rStr.C(i);
			if(c >= 0x80 && c < 0xA0)
				str.CatChar(CPTysso[c-0x80]);
			else
				str.CatChar(c);
		}
		rStr.Z().Cat(str);
		ok = 1;
	}
	return ok;
}

// new {
int PosiflexEquip::Connect() 
{
	int    ok = 1;
	SString control_symbol;
	THROW(CommPort.InitPort(Port, 0, 0));
	control_symbol.Z().CatChar(0x1B).CatChar(0x74).CatChar(6); // Установим кодовую таблицу CP866
	for(size_t index = 0; index < control_symbol.Len(); index++)
		THROW(CommPort.PutChr(control_symbol.C(index)));
	CATCH
		ok = 0;
	ENDCATCH;
	return ok;
}
// } new

int PosiflexEquip::UsbConnect()
{
	int    ok = 1;
	SString control_symbol;
	P_Lib = new SDynLibrary(UsbDllPath);
	THROWERR(P_Lib && P_Lib->IsValid(), CUSTDISP_USBLIB);
	THROWERR(Openu = (long(*)())P_Lib->GetProcAddr("OpenUSBpd"), CUSTDISP_USBLIB);
	THROWERR(Closeu =(long(*)())P_Lib->GetProcAddr("CloseUSBpd"), CUSTDISP_USBLIB);
	THROWERR(Writeu = (long(*)(const char *, long))P_Lib->GetProcAddr("WritePD"), CUSTDISP_USBLIB);
	THROWERR(Stateu = (long(*)())P_Lib->GetProcAddr("PdState"), CUSTDISP_USBLIB);
	THROW(Openu() == 0);
	control_symbol.Z().CatChar(0x1B).CatChar(0x74).CatChar(6); // Установим кодовую таблицу CP866
	THROWERR(Writeu(control_symbol, (long)control_symbol.Len()) == 0, CUSTDISP_PRINTTOPORT);
	CATCH
		Openu = 0;
		Closeu = 0;
		Writeu = 0;
		Stateu = 0;
		ok = 0;
	ENDCATCH;
	return ok;
}

int PosiflexEquip::UsbDisconnect()
{
	int ok = 1;
	if(Closeu)
		ok = Closeu();
	return ok;
}

int PosiflexEquip::UsbPrintLine()
{
	int ok = 0;
	SString control_symbol;
	if(VerTab == VERT_UPP || VerTab == VERT_DOWN) {
		/*control_symbol.Z().CatChar(0x0b);
		THROWERR(Writeu(control_symbol, control_symbol.Len()), CUSTDISP_PRINTTOPORT);*/
		if(VerTab == VERT_DOWN) {
			control_symbol.Z().CatChar(0x0a);
			THROWERR(Writeu(control_symbol, (long)control_symbol.Len()) == 0, CUSTDISP_PRINTTOPORT);
		}
	}
	if(Align == LEFT)
		Text.Align(STRLEN, ADJ_LEFT);
	else if(Align == CENTER)
		Text.Align(STRLEN, ADJ_CENTER);
	else if(Align == RIGHT)
		Text.Align(STRLEN, ADJ_RIGHT);
	THROWERR(Writeu(Text, (long)Text.Len()) == 0, CUSTDISP_PRINTTOPORT);
	CATCH
		ok = 1;
	ENDCATCH;
	ClearParams();
	return ok;
}

int PosiflexEquip::UsbClearDisplay()
{
	SString control_symbol;
	control_symbol.Z().CatChar(0x0c);
	return (Writeu(control_symbol, (long)control_symbol.Len()) == 0) ? 1 : 0;
}