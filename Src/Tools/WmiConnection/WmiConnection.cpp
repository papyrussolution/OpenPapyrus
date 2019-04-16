// WmiConnection.cpp
// Copyright (c) A. Kurilov 2008
//
#include "WmiConnection.h"
#include "resource.h"
//
// КОНСТАНТЫ {
//
const int minParamLen = 14;
const bstr_t bsReturnValue			= bstr_t(L"ReturnValue");
const bstr_t bsWQL					= bstr_t(L"WQL");
const char *const sTHIS_DLLFILE		= "wmi_connection.dll";
// @cstr {
// общие сообщения об ошибках
const char *const sCOM_ERROR		= "ошибка COM";
const char *const sWMI_ERROR		= "ошибка WMI";
const char *const sERR_UNKNOWN		= "неизвестная ошибка, код: ";
const char *const sACCESS_DENIED	= "доступ запрещен";
// сообщения об ошибках WBEM
const char *const sINV_NS			= "неправильное пространство имен";
const char *const sOUT_OF_MEM		= "нехватает свободной памяти";
const char *const sINV_CLASS		= "неправильно указан класс";
const char *const sINCOMPL_CLASS	= "некорректный или незарегистрированный класс, нельзя получить экземпляр";
const char *const sINV_PAR			= "неправильно указан параметр";
const char *const sINV_PROPTYPE		= "неправильно сформирован тип значения параметра";
const char *const sINV_OBJPATH		= "неправильно сформирован путь к объекту";
const char *const sINV_METHOD		= "неправильный метод";
const char *const sMETHOD_DISBLD	= "нельзя выполнить метод, отмеченный как [disabled]";
const char *const sMETHOD_NOTIMPL	= "нельзя выполнить метод, не отмеченный как [implemented]";
const char *const sINV_METHODPAR	= "неправильные параметры метода";
const char *const sCLSOBJ_NOTFND	= "объект не найден, вероятно его нет";
const char *const sNEED_RECONNECT	= "требуется новое соединение с сервисом WMI";
const char *const sNET_ERR			= "ошибка сети";
const char *const sINV_QUERY		= "сформирован неправильный запрос WQL";
const char *const sINV_QUERYTYPE	= "сформирован запрос неправильного типа, возможно используется не WQL";
const char *const sTYPE_MISMATCH	= "указанный тип параметра не подходит";
// сообщения об ошибках WMI
const char *const sNOT_SUPPORTED	= "метод не поддерживается";
const char *const sDEP_SVCRUNNING	= "сервисы, зависящие от этого выполняются (попытка остановить сервис?)";
const char *const sINV_SVCCONTROL	= "неправильное управление сервисом";
const char *const sSVC_CANNTACCPT	= "сервис не поддается управлению";
const char *const sSVC_NOTACTIVE	= "сервис не активен";
const char *const sSVC_REQUEST		= "истекло время ожидания результата операции с сервисом";
const char *const sUNKNOWNFAIL		= "неизвестная ошибка";
const char *const sPATH_NOTFOUND	= "путь не найден";
const char *const sSVC_RUNNING		= "сервис уже запущен";
const char *const sSVC_DBLOCKED		= "база данных сервисов заблокирована";
const char *const sSVC_DEPDEL		= "удален сервис, от которого зависит текущий сервис";
const char *const sSVC_DEPFAIL		= "ошибка зависимости сервиса";
const char *const sSVC_DISABLED		= "сервис отключен";
const char *const sSVC_LOGONFAIL	= "аутентификация сервися закончилась неудачно";
const char *const sSVC_MARKEDDEL	= "сервис отмечен для удаления";
const char *const sSVC_NOTHREAD		= "нет потока выполнения (service no thread)";
const char *const sSVC_CIRCDEP		= "обнаружена цикличиская зависимость сервисов";
const char *const sSVC_DUPNAME		= "дублирование имен сервисов";
const char *const sSVC_INVNAME		= "некорректное имя сервиса";
const char *const sSVC_INVPARAM		= "некорректный параметр для сервиса";
const char *const sSVC_INVACCOUNT	= "некорректная учетная запись сервиса";
const char *const sSVC_EXISTS		= "сервис уже существует";
const char *const sSVC_PAUSED		= "сервис уже приостановлен";
const char *const sCOM_LIBFAIL		= "Не удалось инициализировать библиотеку COM";
const char *const sCOM_SECFAIL		= "Не удалось инициализировать настройки безопасности COM";
const char *const sWBEM_INSTFAIL	= "Не удалось получить новый экземпляр класса IWbemLocator (ф-ция CoCreateInstance)";
const char *const sPROXY_FAIL		= "Ф-ция CoSetProxyBlanket закончилась неудачно";
const char *const sCONNECT_FAIL		= "Не удалось установить соединение с сервисом WMI";
const char *const sRETURNED_ERR		= "Закончилось неудачно";
const char *const sNEW_REGKEY		= "Создание нового ключа реестра";
const char *const sDEL_REGKEY		= "Удаление ключа реестра";
const char *const sGET_REGKEY		= "Получение значения ключа реестра";
const char *const sSET_KEYVALUE		= "Установка значения ключа реестра";
const char *const sNOT_IMPL			= "Нет реализации";
const char *const sRPC_FAIL			= "Возможно недоступна удаленная служба RPC или неправильный адрес сервера";
// прочие сообщения
const char *const sGETSVCOBJ		= "Получение объекта сервиса ";
const char *const sGETMETHOD		= "Получение метода ";
const char *const sSPAWNINST		= "Создание нового экземпляра из описания класса WMI (ф-ция SpawnInstance) ";
const char *const sPUTPARAM			= "Добавление парамаетра ";
const char *const sEXECMETHOD		= "Выполнение метода ";
const char *const sATPATH			= " для пути ";
const char *const sCONNECTING		= "Соединение с сервисом WMI";
const char *const sREAD_RETURN		= "Получение возвращаемого методом значения";
const char *const sNEW_SVC			= "Создание нового сервиса windows: ";
const char *const sDEL_SVC			= "Удаление сервиса windows: ";
const char *const sSVC_START		= "Запуск сервиса windows: ";
const char *const sSVC_STOP			= "Остановка сервиса windows: ";
const char *const sGET_PROPVALUE	= "Получение значения свойства объекта";
const char *const sREAD_KEYVAL		= "Чтение значения ключа реестра";
const char *const sNEW_PROCESS		= "Создание и запуск нового процесса: ";
const char *const sSUCCEED			= "OK";
const char *const sNOTCONNECTED		= "Нет соединения";
// прочие строковые константы
const char *const sEMPTY			= "";
const char *const sHEXPREFIX		= "0x";
const char *const sCHANGE			= "Change";
const char *const sCREATE			= "Create";
const char *const sDELETE			= "Delete";
const char *const sWIN32_PROCESS	= "Win32_Process";
const char *const sWIN32_SERVICE	= "Win32_Service";
const char *const sWIN32_SHARE		= "Win32_Share";
const char *const sSTARTSERVICE		= "StartService";
const char *const sSTOPSERVICE		= "StopService";
const char *const sRESUMESERVICE	= "ResumeService";
const char *const sNAME				= "Name";
const char *const sDISPLAYNAME		= "DisplayName";
const char *const sPATH				= "Path";
const char *const sPATHNAME			= "PathName";
const char *const sSTARTMODE		= "StartMode";
const char *const sSTARTNAME		= "StartName";
const char *const sSTARTPASSWORD	= "StartPassword";
const char *const sSERVICETYPE		= "ServiceType";
const char *const sERRORCONTROL		= "ErrorControl";
const char *const sDESKTOPINTERACT	= "DesktopInteract";
const char *const sLOADORDER		= "LoadOrderGroup";
const char *const sLOADORDERDEPS	= "LoadOrderGroupDependencies";
const char *const sSERVICEDEPS		= "ServiceDependencies";
const char *const sSTDREGPROV		= "StdRegProv";
const char *const sCREATEKEY		= "CreateKey";
const char *const sDELETEKEY		= "DeleteKey";
const char *const sSUBKEYNAME		= "SubkeyName";
const char *const sHDEFKEY			= "hDefKey";
const char *const sGETSTRINGVALUE	= "GetStringValue";
const char *const sSETSTRINGVALUE	= "SetStringValue";
const char *const sENUMVALUES		= "EnumValues";
const char *const sVALUE			= "sValue";
const char *const sVALUENAME		= "sValueName";
const char *const sCOMMANDLINE		= "CommandLine";
const char *const sCURR_DIR			= "CurrentDirectory";
const char *const sNS_DEFAULT		= "root\\default";
const char *const sNS_CIMV2			= "root\\cimv2";
const char *const sSELECTALLFROM	= "SELECT * FROM ";
const char *const sWHERE_NAME		= " WHERE Name=\"";
const char *const sDOUBLE_QUOTE		= "\"";
// } @cstr
// } КОНСТАНТЫ
//
// структура соединеня с сервисом WMI
struct WmiConnection
{
	WmiConnection()
	{
		status = 0;
		P_loc = NULL; P_svc = NULL;
		currentNS = ""; lastServer = ""; lastUser = ""; lastPassword = ""; lastMsg = "";
	}
	~WmiConnection()
	{
		if(P_loc) {
			P_loc->Release();
			P_loc = NULL;
		}
		if(P_svc) {
			P_svc->Release();
			P_svc = NULL;
		}
	}
	int status;	// connected & OK?
	IWbemLocator* P_loc;
	IWbemServices* P_svc;
	SString
		currentNS,		// текущее подключенное пространство имен
		lastServer,
		lastUser,
		lastPassword,
		lastMsg;
} *P_conn; // возникает ограничение - возможно только одно соединение в каждом адресном пространстве
// используется в ф-ции "AuthBox"
struct Account {
	char *pUserName;
	char *pPassWord;
};
// entry point
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{ return TRUE; }
//
// account dialog procedure / message handler
//
BOOL CALLBACK AuthDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{	//
		case WM_INITDIALOG:
		{	// погдотовить результат
			Account *pAccount = (Account *) lParam;
			if(pAccount)
				SetWindowLong(hwndDlg, GWL_USERDATA, (long)pAccount);
			return TRUE;
		}
		case WM_COMMAND:
		{	// пользователь что-то сделал
			switch(LOWORD(wParam))
			{	// пользователь нажал "OK" или "Cancel"?
				case IDOK:
				{	// получить логин и пароль
					Account * pAccount = (Account *)GetWindowLong(hwndDlg, GWL_USERDATA);
					if(pAccount) {
						pAccount->pUserName = new char[256];
						GetDlgItemText(hwndDlg, IDC_USERNAME, pAccount->pUserName, 0x100);
						pAccount->pPassWord = new char[256];
						GetDlgItemText(hwndDlg, IDC_PASSWORD, pAccount->pPassWord, 0x100);
						EndDialog(hwndDlg, wParam);
					}
					return TRUE;
				}
				case IDCANCEL:
				{	// не получить логин и пароль
					Account *pAccount = (Account*) GetWindowLong(hwndDlg, GWL_USERDATA);
					pAccount->pUserName = NULL;
					pAccount->pPassWord = NULL;
					EndDialog(hwndDlg, wParam);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}
//
// проверить возвращаемое значение, установить соответствие с ошибкой WBEM и приготовить сообщение
//
int CheckWbemFailure(HRESULT hr, const SString& cause)
{
	if(FAILED(hr)) {
		SString msg = SString(cause).Cat(": ");
		switch(hr) {
			case WBEM_E_ACCESS_DENIED:				msg.Cat(sACCESS_DENIED);	break;
			case WBEM_E_INVALID_NAMESPACE:			msg.Cat(sINV_NS);			break;
			case WBEM_E_OUT_OF_MEMORY:				msg.Cat(sOUT_OF_MEM);		break;
			case WBEM_E_INVALID_CLASS:				msg.Cat(sINV_CLASS);		break;
			case WBEM_E_INCOMPLETE_CLASS:			msg.Cat(sINCOMPL_CLASS);	break;
			case WBEM_E_INVALID_PARAMETER:			msg.Cat(sINV_PAR);			break;
			case WBEM_E_INVALID_PROPERTY_TYPE:		msg.Cat(sINV_PROPTYPE);		break;
			case WBEM_E_INVALID_OBJECT_PATH:		msg.Cat(sINV_OBJPATH);		break;
			case WBEM_E_INVALID_METHOD:				msg.Cat(sINV_METHOD);		break;
			case WBEM_E_METHOD_DISABLED:			msg.Cat(sMETHOD_DISBLD);	break;
			case WBEM_E_METHOD_NOT_IMPLEMENTED:		msg.Cat(sMETHOD_NOTIMPL);	break;
			case WBEM_E_INVALID_METHOD_PARAMETERS:	msg.Cat(sINV_METHODPAR);	break;
			case WBEM_E_NOT_FOUND:					msg.Cat(sCLSOBJ_NOTFND);	break;
			case WBEM_E_SHUTTING_DOWN:				msg.Cat(sNEED_RECONNECT);	break;
			case WBEM_E_TRANSPORT_FAILURE:			msg.Cat(sNET_ERR);			break;
			case WBEM_E_INVALID_QUERY:				msg.Cat(sINV_QUERY);		break;
			case WBEM_E_INVALID_QUERY_TYPE:			msg.Cat(sINV_QUERYTYPE);	break;
			case WBEM_E_TYPE_MISMATCH:				msg.Cat(sTYPE_MISMATCH);	break;
			case 0x800706ba:						msg.Cat(sRPC_FAIL);			break;
			case E_NOTIMPL:							msg.Cat(sNOT_IMPL);			break;
			case E_ACCESSDENIED:					msg.Cat(sACCESS_DENIED);	break;
			default:
				char cshr[16];
				ltoa(hr, cshr, 16);
				msg.Cat(sERR_UNKNOWN);
				msg.Cat(sHEXPREFIX);
				msg.Cat(cshr);
		}
		P_conn->lastMsg = msg;
	}
	else {
		P_conn->lastMsg = sSUCCEED;
		hr = 0;
	}
	return hr;
}
//
// проверить возвращаемое значение, установить соответствие с ошибкой WMI и приготовить сообщение
//
int CheckWmiFailure(short code, const SString& cause)
{
	if(code!=0) {
		SString msg;
		(msg = cause).CatDiv(':', 2);
		switch(code) {
			case 1:	msg.Cat( sNOT_SUPPORTED		); break;
			case 2:	msg.Cat( sACCESS_DENIED		); break;
			case 3:	msg.Cat( sDEP_SVCRUNNING	); break;
			case 4:	msg.Cat( sINV_SVCCONTROL	); break;
			case 5:	msg.Cat( sSVC_CANNTACCPT	); break;
			case 6:	msg.Cat( sSVC_NOTACTIVE		); break;
			case 7:	msg.Cat( sSVC_REQUEST		); break;
			case 8: msg.Cat( sUNKNOWNFAIL		); break;
			case 9:	msg.Cat( sPATH_NOTFOUND		); break;
			case 10:msg.Cat( sSVC_RUNNING		); break;
			case 11:msg.Cat( sSVC_DBLOCKED		); break;
			case 12:msg.Cat( sSVC_DEPDEL		); break;
			case 13:msg.Cat( sSVC_DEPFAIL		); break;
			case 14:msg.Cat( sSVC_DISABLED		); break;
			case 15:msg.Cat( sSVC_LOGONFAIL		); break;
			case 16:msg.Cat( sSVC_MARKEDDEL		); break;
			case 17:msg.Cat( sSVC_NOTHREAD		); break;
			case 18:msg.Cat( sSVC_CIRCDEP		); break;
			case 19:msg.Cat( sSVC_DUPNAME		); break;
			case 20:msg.Cat( sSVC_INVNAME		); break;
			case 21:msg.Cat( sSVC_INVPARAM		); break;
			case 22:msg.Cat( sSVC_INVACCOUNT	); break;
			case 23:msg.Cat( sSVC_EXISTS		); break;
			case 24:msg.Cat( sSVC_PAUSED		); break;
			default:
				msg.Cat(sERR_UNKNOWN).Cat(code);
		}
		P_conn->lastMsg = msg;
	}
	else
		P_conn->lastMsg = sSUCCEED;
	return code;
}
//
// переприсоедениться к указанному namespace если оно не совпадает с текущим
//
int EnsureCurrentNS(const SString& rNameSpace)
{
	int ok;
	char last_server[32], last_user[64], last_password[64], name_space[32];
	if(P_conn && (P_conn->currentNS.Cmp(rNameSpace, 0)!=0)) {
		P_conn->lastServer.CopyTo(last_server, 32);
		P_conn->lastUser.CopyTo(last_user, 64);
		P_conn->lastPassword.CopyTo(last_password, 64);
		rNameSpace.CopyTo(name_space, 32);
		ok = Connect(last_server, last_user, last_password, name_space);
	}
	else
		ok = 0;
	if(P_conn==NULL || !P_conn->status || P_conn->currentNS.Cmp(rNameSpace, 0))
		ok = 0;
	return ok;
}
//
// UI-диалог для ввода логина и парол
//
EXPORT int LoginPasswordBox(char *pUserNameBuff, char *pPassWordBuff, size_t maxLength)
{
	Account account; account.pUserName = NULL; account.pPassWord = NULL;
	size_t size = 0;
	if(DialogBoxParam (
			GetModuleHandle(sTHIS_DLLFILE), MAKEINTRESOURCE(authDlg),
			0, (DLGPROC) AuthDialogProc, (LPARAM) &account
		)
	){
		if(account.pUserName) {
			size = strlen(account.pUserName);
			size = MIN(size, maxLength);
			strnzcpy(pUserNameBuff, account.pUserName, size+1);
			delete account.pUserName;
		}
		else
			pUserNameBuff[0] = 0;
		if(account.pPassWord) {
			size = strlen(account.pPassWord);
			size = MIN(size, maxLength);
			strnzcpy(pPassWordBuff, account.pPassWord, size+1);
			delete account.pPassWord;
		}
	}
	return BIN(strlen(pUserNameBuff)); // returns 0 if user name wasn't specified by user
}
//
// получить последнее сохранившееся диагностическое сообщение
//
EXPORT void GetLastMsg(char* pBuff, size_t msgMaxLength)
{
	if(!pBuff)
		pBuff = new char[msgMaxLength];
	if(P_conn)
		P_conn->lastMsg.CopyTo(pBuff, msgMaxLength);
	else
		STRNSCPY(pBuff, sNOTCONNECTED);
}
//
// вернуть 1 если указанный путь является UNC, 0 - иначе
//
EXPORT int IsUncPath(const char* pPath)
{
	int unc = 0;
	// минимально возможный путь UNC может выглядеть так: "\\.\z"
	if(pPath && strlen(pPath)>5 && pPath[0]=='\\' && pPath[1]=='\\')
		unc = 1;
	return unc;
}
//
// получить имя сервера из исходного пути UNC
//
EXPORT size_t GetServerName(const char* pUncPath, char* pBuff, size_t length)
{
	if(IsUncPath(pUncPath)) {
		size_t i = 2;
		while(pUncPath[i]!='\\' && i<length)
			i ++;
		length = i - 2;
		if(!pBuff)
			pBuff = new char[length+1];
		for(i=0; i<length; i++)
			pBuff[i] = pUncPath[i+2];
		pBuff[i] = 0;
	}
	return length;
}
//
// execute the WQL query & extract specified property value from result
// you must be connected to correct namespace already
//
EXPORT int GetPropertyValue(const char *pQuery, const char *pProperty, char* pBuff, size_t maxLen)
{
	int ok = 1;
	IEnumWbemClassObject *p_enum = NULL;
	BSTR bs_query = _bstr_t(pQuery);
	BSTR bs_property = _bstr_t(pProperty);
	//
	if( pQuery && strlen(pQuery)>1 && pProperty && strlen(pProperty)>1
		&& !CheckWbemFailure (
			P_conn->P_svc->ExecQuery( bsWQL, bs_query,
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL, &p_enum ), sGET_PROPVALUE ) && p_enum ) {
		IWbemClassObject *p_clsobj;
		ULONG ul_ret = 0;
		if(!CheckWbemFailure(p_enum->Next(WBEM_INFINITE, 1, &p_clsobj, &ul_ret), sGET_PROPVALUE) && ul_ret > 0) {
			VARIANT v_prop;
			if(!CheckWbemFailure(p_clsobj->Get( bs_property, 0, &v_prop, 0, 0 ), sGET_PROPVALUE)) {
				SString sstmp;
				sstmp.CopyFromOleStr(v_prop.bstrVal).CopyTo(pBuff, maxLen);
			}
			p_clsobj->Release();
			VariantClear(&v_prop);
		}
		p_enum->Release();
	}
	SysFreeString(bs_query);
	SysFreeString(bs_property);
	return ok;
}
//
// get the input-argument class object
//
IWbemClassObject* GetServicesObject(const SString& rClassPath)
{
	IWbemClassObject* p_class = NULL;
	SString message = SString(sGETSVCOBJ).Cat(rClassPath);
	BSTR class_path = NULL;
	CheckWbemFailure(P_conn->P_svc->GetObject(rClassPath.CopyToOleStr(&class_path),0, NULL, &p_class, NULL), message);
	SysFreeString(class_path);
	return p_class;
}
//
// get specified WMI method
//
IWbemClassObject* GetMethod(IWbemClassObject* p_class, const SString& rName)
{
	IWbemClassObject* p_inclass = NULL;
	SString message = SString(sGETMETHOD).Cat(rName);
	BSTR name = NULL;
	if(p_class)
		CheckWbemFailure(p_class->GetMethod(rName.CopyToOleStr(&name), 0, &p_inclass, NULL), message);
	SysFreeString(name);
	return p_inclass;
}
//
// create an instance of the input-argument class
//
IWbemClassObject* SpawnInstance(IWbemClassObject* p_inclass)
{
	IWbemClassObject* p_ininst = NULL;
	if(p_inclass)
		CheckWbemFailure(p_inclass->SpawnInstance(0, &p_ininst), SString(sSPAWNINST));
	return p_ininst;
}
//
// create and store the value for the input parameter
// string value version
void PutParamStr(IWbemClassObject *p_ininst, const SString& rPropertyName, const SString& rPropertyValue, int expandIfTooShort = 0)
{
	VARIANT v_param;
    v_param.vt = VT_BSTR;
	v_param.bstrVal = NULL;
	SString property_value = rPropertyValue;
	if(expandIfTooShort)
		property_value.Align(minParamLen, ADJ_LEFT);
	v_param.bstrVal = property_value.CopyToOleStr(&(v_param.bstrVal));
	if(p_ininst)
		CheckWbemFailure(p_ininst->Put(bstr_t(rPropertyName), 0, &v_param, 0), SString(sPUTPARAM).Cat(property_value));
	VariantClear(&v_param);
}
// boolean value version
void PutParamBool(IWbemClassObject* p_ininst, const SString& rName, int value)
{
    VARIANT v_param;
    v_param.vt = VT_BOOL;
	v_param.boolVal = (value? 0xffff : 0);
	BSTR bs_name = NULL;
	if(p_ininst)
		CheckWbemFailure(p_ininst->Put(rName.CopyToOleStr(&bs_name), 0, &v_param, 0), SString(sPUTPARAM).Cat(rName));
	SysFreeString(bs_name);
	VariantClear(&v_param);
}
// 8-bit unsigned integer value version
void PutParamByte(IWbemClassObject* p_ininst, const SString& rName, uchar value)
{
	VARIANT v_param;
    v_param.vt = VT_UI1;
	v_param.bVal = value;
	BSTR bs_name = NULL;
    if(p_ininst)
		CheckWbemFailure(p_ininst->Put(rName.CopyToOleStr(&bs_name), 0, &v_param, 0), SString(sPUTPARAM).Cat(rName));
	SysFreeString(bs_name);
	VariantClear(&v_param);
}
// 32-bit unsigned integer value version
void PutParamLong(IWbemClassObject* p_ininst, const SString& rName, ulong value)
{
    VARIANT v_param;
    v_param.vt = VT_I4;
	v_param.lVal = value;
	BSTR bs_name = NULL;
    if(p_ininst)
		CheckWbemFailure(p_ininst->Put(rName.CopyToOleStr(&bs_name), 0, &v_param, 0), SString(sPUTPARAM).Cat(rName));
	SysFreeString(bs_name);
	VariantClear(&v_param);
}
//
// execute the method
//
IWbemClassObject* Exec(IWbemClassObject* p_ininst, const SString& rClassPath, const SString& rMethodName)
{
	IWbemClassObject* p_outinst = NULL;
	BSTR class_path = NULL, method_name = NULL;
	class_path	= rClassPath.CopyToOleStr(&class_path);
	method_name	= rMethodName.CopyToOleStr(&method_name);
	SString message = SString(sEXECMETHOD).Cat(rMethodName).Cat(sATPATH).Cat(rClassPath);
	if(p_ininst)
		CheckWbemFailure(P_conn->P_svc->ExecMethod(class_path, method_name, 0, NULL, p_ininst, &p_outinst, NULL), message);
	SysFreeString(class_path);
	SysFreeString(method_name);
	return p_outinst;
}
//
// convert UNC path to appropriate local path on remote windows host
//
EXPORT int GetLocalFromUnc(const char* pUncPath, char* pBuff, size_t maxLen)
{
	int ok = 1;
	SString	unc_path = pUncPath, local_path = sEMPTY, share_name = sEMPTY, share_path = sEMPTY, tail = sEMPTY;
	// STEP #1: determine server
	char server[MAXLEN];
	ok = BIN(GetServerName(pUncPath, server, MAXLEN));
	// STEP #2: determine share name
	size_t i, j, len = unc_path.Len();
	if(ok && server && strlen(server)>0) {
		i = strlen(server) + 3;
		j = 0;
		while(unc_path[i+j]!='\\' && unc_path[i+j]!='/' && i+j<len)
			j ++;
		share_name = unc_path.Sub(i, j, share_name);
	}
	// STEP #3: determine input path tail
	i = strlen(server) + share_name.Len() + 4;
	if(ok && share_name.NotEmpty() && i<len)
		tail = unc_path.Sub(i, len-i, tail);
	else
		ok = 0;
	// STEP #4: get that share path
	// * share name must be valid
	// * the server that you're connected to and the host that has shared directory must be the same
	// * you must be connected to WMI into the namespace of "root\cimv2"
	if(ok && share_name.NotEmpty() && P_conn->lastServer.Cmp(server, 0)==0 && EnsureCurrentNS(sNS_CIMV2)) {
		char buff_share_path[MAXLEN];
		ok = GetPropertyValue (
				SString(sSELECTALLFROM).Cat(sWIN32_SHARE).Cat(sWHERE_NAME).Cat(share_name).Cat(sDOUBLE_QUOTE),
				sPATH, buff_share_path, MAXLEN
			);
		if(ok)
			share_path = buff_share_path;
	}
	// STEP #5: concatenate that share path and input path tail
	if(ok && share_path.NotEmpty()) {
		local_path = share_path;
		if(tail.NotEmpty())
			local_path.CatChar('\\').Cat(tail);
		if(!pBuff)
			pBuff = new char[maxLen];
		local_path.CopyTo(pBuff, maxLen);
	}
	else
		ok = 0;
	return BIN(ok);
}
//
// connects to specified WMI namespace with specified user name and password
//
EXPORT int Connect(const char *pServer, const char *pUserName, const char *pUserPassword, const char *pNameSpace)
{
	const SString
		server		= (pServer==NULL || strlen(pServer)<1)?				sEMPTY		: SString(pServer),
		user_name	= (pUserName==NULL || strlen(pUserName)<1)?			sEMPTY		: SString(pUserName),
		user_password=(pUserPassword==NULL || strlen(pUserPassword)<1)?	sEMPTY		: SString(pUserPassword),
		name_space	= (pNameSpace==NULL || strlen(pNameSpace)<1)?		sNS_DEFAULT	: SString(pNameSpace);
	if(P_conn)
		Release();
	P_conn = new WmiConnection();
	// STEP#1: initialize COM library
	P_conn->status = !BIN(CoInitializeEx(0, COINIT_APARTMENTTHREADED));
	if(!P_conn->status)
		MessageBox(NULL, sCOM_LIBFAIL, sCOM_ERROR, MB_OK);
	// STEP#2: initialize COM security
	if(P_conn->status) {
		HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		if(FAILED(hr) && hr!=RPC_E_TOO_LATE) { // "RPC_E_TOO_LATE" happens sometimes but isn't fatal
			MessageBox(NULL, sCOM_SECFAIL, sCOM_ERROR, MB_OK);
			P_conn->status = 0;
		}
	}
	// STEP#3: obtain the initial locator to WMI on a particular host computer
	if(P_conn->status && FAILED(CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &(P_conn->P_loc)))) {
		P_conn->status = 0;
		MessageBox(NULL, sWBEM_INSTFAIL, sCOM_ERROR, MB_OK);
	}
	// STEP#4: connect to the default namespace with the
	// current user and obtain pointer P_svc to make IWbemServices calls
	if( P_conn->status && P_conn->P_loc) {
		BSTR full_name_space = NULL;
		(server.Empty()? name_space : SString("\\\\").Cat(server).Cat("\\").Cat(name_space))
			.CopyToOleStr(&full_name_space);
		// не стоит подключаться локально с логином/паролем не по умолчанию, т.к. WMI при этом выдает ошибку
		// поэтому проверить не только имя пользователя но еще и сервер
		if(user_name.Empty() || server.Empty() || server.Cmp(".", 0)==0 || server.Cmp("localhost", 0)==0) {
			P_conn->status = !CheckWbemFailure ( P_conn->P_loc->ConnectServer (
					full_name_space, NULL, NULL, 0, NULL, 0, 0, &(P_conn->P_svc) ),
				sCONNECTING );
		}
		else { // подключение к удаленному сервису WMI с явно указанным логином/паролем
			BSTR bs_user = NULL, bs_password = NULL;
			user_name.CopyToOleStr(&bs_user);
			user_password.CopyToOleStr(&bs_password);
			P_conn->status = !CheckWbemFailure ( P_conn->P_loc->ConnectServer (
					full_name_space, bs_user, bs_password, 0, NULL, 0, 0, &(P_conn->P_svc)),
				sCONNECTING );
			SysFreeString(bs_user);
			SysFreeString(bs_password);
		}
		SysFreeString(full_name_space);
	}
	// STEP#5: set the IWbemServices proxy so that impersonation of the user (client) occurs
	if(P_conn->status && P_conn->P_svc!=NULL && FAILED(CoSetProxyBlanket( P_conn->P_svc, RPC_C_AUTHN_WINNT,
			RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE) )) {
		P_conn->status = 0;
		MessageBox( NULL, sPROXY_FAIL, sCOM_ERROR, MB_OK );
	}
	// done, checking
	if(P_conn->status) {
		P_conn->currentNS	= name_space;
		P_conn->lastServer	= server;
		P_conn->lastUser	= user_name;
		P_conn->lastPassword= user_password;
		P_conn->lastMsg		= sSUCCEED;
	}
	else { // copying local connection parameters for possible future re-connecting
		char * p_msg = new char[0x100];
		::MessageBox(NULL, P_conn->lastMsg.CopyTo(p_msg, 0x100), sCONNECT_FAIL, MB_OK);
		delete p_msg;
		Release();
	}
	return P_conn->status;
}
//
// free any used resources
//
EXPORT void Release()
{
	if(P_conn) {
		delete P_conn;
		P_conn = NULL;
		CoUninitialize();
		OleUninitialize();
	}
}
//
// create and run a new windows service
//
EXPORT int WinSvcCreate (
	const char *pName,
	const char *pDisplayName,
	const char *pPathName,
	ServiceType serviceType,
	ErrorControl errorControl,
	int desktopInteract,
	const char *pStartName,
	const char *pStartPassword,
	const char *pOrder,
	const char *pOrderDeps,
	const char *pSvcDeps,
	int expandIfTooShort
)
{
	int ok = 0;
	if(EnsureCurrentNS(sNS_CIMV2) && pName && pPathName) {
		char buff_path_name[MAXLEN];
		if(IsUncPath(pPathName))
			ok = GetLocalFromUnc(pPathName, buff_path_name, MAXLEN);
		else {
			strnzcpy(buff_path_name, pPathName, MAXLEN);
			ok = 1;
		}
		const SString
			name			= pName,
			display_name	= pDisplayName,
			path_name		= buff_path_name,
			start_name		= (pStartName && strlen(pStartName)>0)?			pStartName : sEMPTY,
			start_password	= (pStartPassword && strlen(pStartPassword)>0)?	pStartPassword : sEMPTY,
			order			= (pOrder && strlen(pOrder)>0)?					pOrder : sEMPTY,
			order_deps		= (pOrderDeps && strlen(pOrderDeps)>0)?			pOrderDeps : sEMPTY,
			svc_deps		= (pSvcDeps && strlen(pSvcDeps)>0)?				pSvcDeps : sEMPTY;
		IWbemClassObject
			*p_class = GetServicesObject(sWIN32_SERVICE),
			*p_inclass,
			*p_ininst,
			*p_outinst;
		if(ok && p_class) {
			p_inclass = GetMethod(p_class, sCREATE);
			if(p_inclass) {
				p_ininst = SpawnInstance(p_inclass);
				if(p_ininst) {
					PutParamStr ( p_ininst, sNAME, name, expandIfTooShort );
					PutParamStr ( p_ininst, sDISPLAYNAME, display_name, expandIfTooShort );
					PutParamStr ( p_ininst, sPATHNAME, path_name, 0 );
					PutParamByte( p_ininst, sSERVICETYPE, serviceType );
					PutParamByte( p_ininst, sERRORCONTROL, errorControl );
					PutParamBool( p_ininst, sDESKTOPINTERACT, desktopInteract );
					if(start_name.NotEmpty()) {
						PutParamStr ( p_ininst, sSTARTNAME, start_name, 0 );
						PutParamStr ( p_ininst, sSTARTPASSWORD, start_password, 0 );
					}
					if(order.NotEmpty())
						PutParamStr ( p_ininst, sLOADORDER, order );
					if(order_deps.NotEmpty())
						PutParamStr ( p_ininst, sLOADORDERDEPS, order_deps );
					if(svc_deps.NotEmpty())
						PutParamStr ( p_ininst, sSERVICEDEPS, svc_deps );
					// call the method
					p_outinst = Exec(p_ininst, sWIN32_SERVICE, sCREATE);
					if(p_outinst) {
						VARIANT v_retval;
						if(!CheckWbemFailure( p_outinst->Get (bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN))
							ok = !CheckWmiFailure( short(v_retval.lVal), SString(sNEW_SVC).Cat(name) );
						VariantClear(&v_retval);
						p_outinst->Release();
					}
					else ok = 0;
					p_ininst->Release();
				}
				else ok = 0;
				p_inclass->Release();
			}
			else ok = 0;
		}
	}
	return ok;
}
//
// uninstall windows service
//
EXPORT int WinSvcDelete(const char *pSvcName, int expandIfTooShort)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_CIMV2) && pSvcName) {
		SString	svc_name = expandIfTooShort? SString(pSvcName).Align(minParamLen, ADJ_LEFT) : SString(pSvcName),
				path = SString(sWIN32_SERVICE).Cat(".Name=\"").Cat(svc_name).Cat(sDOUBLE_QUOTE);
		BSTR bs_path = NULL;
		if(CheckWbemFailure(
				P_conn->P_svc->DeleteInstance(path.CopyToOleStr(&bs_path), 0, NULL, NULL),
				SString(sDEL_SVC).Cat(svc_name))) {
			IWbemClassObject *p_winsvc = GetServicesObject(sWIN32_SERVICE), *p_outinst = NULL;
			if(p_winsvc) {
				p_outinst = Exec(p_winsvc, path, sDELETE);
				if(p_outinst) {
					VARIANT v_retval;
					if(!CheckWbemFailure( p_outinst->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN ))
						CheckWmiFailure( short(v_retval.lVal), SString(sDEL_SVC).Cat(svc_name) );
					VariantClear(&v_retval);
					p_outinst->Release();
				}
				else ok = 0;
				p_winsvc->Release();
			}
			else ok = 0;
		}
		else ok = 0;
		SysFreeString(bs_path);
	}
	else ok = 0;
	return ok;
}
//
// start windows service with specified name
//
EXPORT int WinSvcStart(const char *pSvcName, int expandIfTooShort)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_CIMV2) && pSvcName) {
		SString	svc_name = expandIfTooShort? SString(pSvcName).Align(minParamLen, ADJ_LEFT) : SString(pSvcName),
				path = SString(sWIN32_SERVICE).Cat(".Name=\"").Cat(svc_name).Cat(sDOUBLE_QUOTE);
		IWbemClassObject *p_winsvc = GetServicesObject(sWIN32_SERVICE), *p_outinst = NULL;
		if(p_winsvc) {
			p_outinst = Exec(p_winsvc, path, sSTARTSERVICE);
			if(p_outinst) {
				VARIANT v_retval;
				if(!CheckWbemFailure( p_outinst->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN ))
					CheckWmiFailure( short(v_retval.lVal), SString(sSVC_START).Cat(svc_name) );
				VariantClear(&v_retval);
				p_outinst->Release();
			}
			else ok = 0;
			p_winsvc->Release();
		}
		else ok = 0;
	}
	else ok = 0;
	return ok;
}
//
// stop windows service with specified name
//
EXPORT int WinSvcStop(const char *pSvcName, int expandIfTooShort)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_CIMV2) && pSvcName) {
		SString svc_name = expandIfTooShort? SString(pSvcName).Align(minParamLen, ADJ_LEFT) : SString(pSvcName),
				path = SString(sWIN32_SERVICE).Cat(".Name=\"").Cat(svc_name).Cat(sDOUBLE_QUOTE);
		IWbemClassObject *p_winsvc = GetServicesObject(sWIN32_SERVICE), *p_outinst = NULL;
		if(p_winsvc) {
			p_outinst = Exec(p_winsvc, path, sSTOPSERVICE);
			if(p_outinst) {
				VARIANT v_retval;
				if(!CheckWbemFailure( p_outinst->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN ))
					CheckWmiFailure( short(v_retval.lVal), SString(sSVC_STOP).Cat(svc_name) );
				VariantClear(&v_retval);
				p_outinst->Release();
			}
			else ok = 0;
			p_winsvc->Release();
		}
		else ok = 0;
	}
	else ok = 0;
	return ok;
}
//
// check if specified windows service exists
//
EXPORT int WinSvcExists(const char *pSvcName, int expandIfTooShort)
{
	int exists = 1;
	if(EnsureCurrentNS(sNS_CIMV2) && pSvcName) {
		SString	svc_name = expandIfTooShort? SString(pSvcName).Align(minParamLen, ADJ_LEFT) : SString(pSvcName),
				path = SString(sWIN32_SERVICE).Cat(".Name=\"").Cat(svc_name).Cat(sDOUBLE_QUOTE);
		IWbemClassObject *p_winsvc = GetServicesObject(sWIN32_SERVICE), *p_outinst = NULL;
		if(p_winsvc) {
			BSTR bs_path = NULL, bs_resume_service = _bstr_t(sRESUMESERVICE);
			if( P_conn->P_svc->ExecMethod (
						path.CopyToOleStr(&bs_path), bs_resume_service,
						0, NULL, p_winsvc, &p_outinst, NULL
					) == WBEM_E_NOT_FOUND )
				exists = 0;
			else if(p_outinst) {
				VARIANT v_retval;
				p_outinst->Get(bsReturnValue, 0, &v_retval, NULL, 0);
				if(v_retval.lVal == 9)
					exists = 0;
				VariantClear(&v_retval);
				p_outinst->Release();
			}
			SysFreeString(bs_path);
			SysFreeString(bs_resume_service);
			p_winsvc->Release();
		}
	}
	else
		exists = -1;
	return exists;
}
//
// get specified by it's name property of specified by it's name windows service
//
EXPORT int WinSvcGetProperty(const char *pSvcName, const char *pProperty, int expandIfTooShort, char* pBuff, size_t maxLen)
{
	int ok = 0;
	if(EnsureCurrentNS(sNS_CIMV2) && pSvcName && pProperty) {
		IEnumWbemClassObject* pEnumerator = NULL;
		SString	svc_name = expandIfTooShort? SString(pSvcName).Align(minParamLen, ADJ_LEFT) : SString(pSvcName),
				query = SString(sSELECTALLFROM).Cat(sWIN32_SERVICE).Cat(sWHERE_NAME).Cat(svc_name).Cat(sDOUBLE_QUOTE),
				property = SString(pProperty);
		ok = GetPropertyValue(query, property, pBuff, maxLen);
	}
	return ok;
}
//
// read windows registry key value
//
EXPORT int WinRegGetStr(long targetBranch, const char *pkey_name, const char *pvalue_name, int expandIfTooShort, char* pBuff, size_t maxLen)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_DEFAULT) && pkey_name && pvalue_name) {
		const SString key_name = pkey_name, value_name = pvalue_name;
		IWbemClassObject
			*p_params = NULL, *p_ininst = NULL, *p_outs = NULL,
			*p_winreg = GetServicesObject(sSTDREGPROV);
		if(p_winreg) {
			p_params = GetMethod(p_winreg, sGETSTRINGVALUE);
			if(p_params) {
				p_ininst = SpawnInstance(p_params);
				if(p_ininst) {
					PutParamLong( p_ininst, sHDEFKEY, targetBranch );
					PutParamStr ( p_ininst, sSUBKEYNAME, key_name, expandIfTooShort );
					PutParamStr ( p_ininst, sVALUENAME, value_name, expandIfTooShort );
					p_outs = Exec(p_ininst, sSTDREGPROV, sGETSTRINGVALUE);
					if(p_outs) {
						VARIANT var_retval, var_keyval;
						BSTR bs_value = _bstr_t(sVALUE);
						if( CheckWbemFailure (
								p_outs->Get(bsReturnValue, 0, &var_retval, NULL, 0),
								sREAD_RETURN ) ||
							var_retval.lVal ||
							CheckWbemFailure(p_outs->Get(bs_value, 0, &var_keyval, 0, 0), sREAD_KEYVAL)
						) { // что-то не так
							P_conn->lastMsg = SString(sGET_REGKEY).Cat(": ").Cat(sRETURNED_ERR);
							ok = 0;
						}
						else { // скопировать значение в буфер
							SString ss_tmp;
							ss_tmp.CopyFromOleStr(var_keyval.bstrVal);
							if(!pBuff)
								pBuff = new char[maxLen];
							ss_tmp.CopyTo(pBuff, maxLen);
						}
						SysFreeString(bs_value);
						VariantClear(&var_retval);
						p_outs->Release();
					}
					p_ininst->Release();
				}
				p_params->Release();
			}
			p_winreg->Release();
		}
	}
	return ok;
}
//
// add windows registry key/value pair
//
EXPORT int WinRegAddStr(long targetBranch, const char *pkey_name, const char *pvalue_name, const char *pkey_value, int expandIfTooShort)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_DEFAULT) && pkey_name && pvalue_name && pkey_value) {
		const SString key_name = SString(pkey_name), value_name = SString(pvalue_name), key_value = SString(pkey_value);
		IWbemClassObject
			*p_params = NULL, *p_ininst = NULL, *p_outs = NULL,
			*p_winreg	= GetServicesObject(sSTDREGPROV);
		if(p_winreg) {
			// STEP #1: create key
			p_params = GetMethod(p_winreg, sCREATEKEY);
			if(p_params) {
				p_ininst = SpawnInstance(p_params);
				if(p_ininst) {
					PutParamLong( p_ininst, sHDEFKEY, targetBranch );
					PutParamStr ( p_ininst, sSUBKEYNAME, key_name, expandIfTooShort );
					p_outs = Exec(p_ininst, sSTDREGPROV, sCREATEKEY);
					if(p_outs) {
						VARIANT v_retval;
						if(CheckWbemFailure(p_outs->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN) || v_retval.lVal)
							P_conn->lastMsg = SString(sNEW_REGKEY).Cat(": ").Cat(sRETURNED_ERR);
						p_outs->Release();
						p_outs = NULL;
					}
					p_ininst->Release();
					p_ininst = NULL;
				}
				p_params->Release();
				p_params = NULL;
			}
			// STEP #2: set key value
			p_params = GetMethod(p_winreg, sSETSTRINGVALUE);
			if(p_params) {
				p_ininst = SpawnInstance(p_params);
				if(p_ininst) {
					PutParamLong( p_ininst, sHDEFKEY, targetBranch );
					PutParamStr ( p_ininst, sSUBKEYNAME, key_name, expandIfTooShort );
					PutParamStr ( p_ininst, sVALUENAME, value_name, expandIfTooShort );
					PutParamStr ( p_ininst, sVALUE, key_value, expandIfTooShort );
					p_outs = Exec(p_ininst, sSTDREGPROV, sSETSTRINGVALUE);
					if(p_outs) {
						VARIANT v_retval;
						if(CheckWbemFailure(p_outs->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN) || v_retval.lVal )
							P_conn->lastMsg = SString(sSET_KEYVALUE).Cat(": ").Cat(sRETURNED_ERR);
						p_outs->Release();
					}
					else ok = 0;
					p_ininst->Release();
				}
				else ok = 0;
				p_params->Release();
			}
			else ok = 0;
			p_winreg->Release();
		}
		else ok = 0;
	}
	return ok;
}
//
// delete windows registry key
//
EXPORT int WinRegDel(long targetBranch, const char *pkey_name, int expandIfTooShort)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_DEFAULT) && pkey_name) {
		const SString key_name = SString(pkey_name);
		IWbemClassObject *p_params = NULL, *p_ininst = NULL, *p_outs = NULL,
			*p_winreg = GetServicesObject(sSTDREGPROV);
		if(p_winreg) {
			p_params = GetMethod(p_winreg, sDELETEKEY);
			if(p_params) {
				p_ininst = SpawnInstance(p_params);
				if(p_ininst) {
					PutParamLong( p_ininst, sHDEFKEY, targetBranch );
					PutParamStr ( p_ininst, sSUBKEYNAME, key_name, expandIfTooShort );
					p_outs = Exec(p_ininst, sSTDREGPROV, sDELETEKEY);
					if(p_outs) {
						VARIANT v_retval;
						if(CheckWbemFailure(p_outs->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN) || v_retval.lVal)
							P_conn->lastMsg = SString(sDEL_REGKEY).Cat(": ").Cat(sRETURNED_ERR);
						p_outs->Release();
						p_outs = NULL;
					}
					else ok = 0;
					p_ininst->Release();
				}
				else ok = 0;
				p_params->Release();
			}
			else ok = 0;
			p_winreg->Release();
		}
		else ok = 0;
	}
	return ok;
}
//
// create and run a new windos process
//
EXPORT int WinProcessCreate(const char *pCmdLine, const char *pCurrDir)
{
	int ok = 1;
	if(EnsureCurrentNS(sNS_CIMV2) && pCmdLine && strlen(pCmdLine)>0) {
		IWbemClassObject
			*p_params = NULL, *p_inclass = NULL, *p_outinst = NULL,
			*p_proc_cls	= GetServicesObject(sWIN32_PROCESS);
		if(p_proc_cls) {
			p_inclass = GetMethod(p_proc_cls, sCREATE);
			if(p_inclass) {
				p_params = SpawnInstance(p_inclass);
				if(p_params) {
					PutParamStr(p_params, sCOMMANDLINE, pCmdLine);
					if(pCurrDir && strlen(pCurrDir)>0)
						PutParamStr(p_params, sCURR_DIR, pCurrDir);
					p_outinst = Exec(p_params, sWIN32_PROCESS, sCREATE);
					if(p_outinst) {
						VARIANT v_retval;
						if(!CheckWbemFailure(p_outinst->Get(bsReturnValue, 0, &v_retval, NULL, 0), sREAD_RETURN))
							CheckWmiFailure( short(v_retval.lVal), SString(sNEW_PROCESS).Cat(pCmdLine) );
						VariantClear(&v_retval);
						p_outinst->Release();
					}
					else ok = 0;
					p_params->Release();
				}
				else ok = 0;
				p_inclass->Release();
			}
			else ok = 0;
			p_proc_cls->Release();
		}
		else ok = 0;
	}
	else ok = 0;
	return ok;
}
//
