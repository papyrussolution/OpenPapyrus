// WmiConnection.h
// Copyright (c) A.Kurilov 2008
//
#ifndef WMI_CONNECTION_H
#define WMI_CONNECTION_H
#ifndef _WIN32_DCOM
	#define _WIN32_DCOM
#endif
#define EXPORT extern "C" __declspec(dllexport)
// SString
#include <slib.h>
// включить средства Win API для работы с WMI
#include <wbemidl.h>
#include <comdef.h>
const size_t MAXLEN = 0x100;
//
// ТИПЫ {
//
// определение веток реестра windows
enum RegHKey
{
	CLASSES_ROOT	= 0x80000000,
	CURRENT_USER	= 0x80000001,
	LOCAL_MACHINE	= 0x80000002,
	USERS			= 0x80000003,
	CURRENT_CONFIG	= 0x80000005,
};
// типы сервисов windows
enum ServiceType
{
	SVCTYPE_KERNEL		= 0x1,	// kernel driver
	SVCTYPE_FILESYS		= 0x2,	// file system driver
	SVCTYPE_ADAPTER		= 0x4,	// adapter
	SVCTYPE_RECOGNIZER	= 0x8,	// recognizer driver
	SVCTYPE_OWN			= 0x10,	// own process
	SVCTYPE_SHARE		= 0x20	// share process
};
// тип флага, определяющего  поведение в случае ошибки работы сервиса windows
enum ErrorControl
{
	ERRCTL_SILENT		= 0,	// silence
	ERRCTL_NOTIFY		= 1,	// notify user
	ERRCTL_LASTGOOD		= 2,	// restart system with last-known-good configuration
	ERRCTL_ATTEMPTGOOD	= 3		// attempt to start system with a good configuration
};
//
// } ТИПЫ
//
// ФУНКЦИИ {
//
BOOL APIENTRY DllMain(HANDLE, DWORD, LPVOID);
// диалог для ввода имени пользователя и парол
EXPORT int LoginPasswordBox(char *pUserNameBuff, char *pPassWordBuff, size_t maxLen = MAXLEN);
// возвращает указатель на последнее диагностическое сообщение модул
EXPORT void GetLastMsg(char *buff, size_t maxLen = MAXLEN);
// если входной параметр является UNC-путем возвращает 1, иначе 0
EXPORT int IsUncPath(const char *pPath);
// извлекает имя сервера из UNC-строки вида "\\xxxx\yyyyy\zzzzzz"
EXPORT size_t GetServerName(const char *pUncPath, char *pBuff, size_t maxLen = MAXLEN);
// исполняет WQL запрос для получения значения свойства
EXPORT int GetPropertyValue(const char *pQuery, const char *pProperty, char *pBuff, size_t maxLen = MAXLEN);
// рассчитывает локальный путь на удаленном сервере, соответствующий указанному на входе UNC-пути
// если исходный UNC-путь не-UNC или возникнет ошибка вернет 0, а результат в буфере не определен
EXPORT int GetLocalFromUnc(const char *pUncPath, char *pBuff, size_t maxLen = MAXLEN);
// соединение с сервисом WMI для выполнения чего-нибудь
// возвращает 1 если успешно, 0 - иначе
// требует "Release()" если соединение было успешно установлено
EXPORT int Connect (
	// соединяет с localhost если сервер не указан или сервер="."
	const char *pServer		= NULL,
	// возможные значения: "domain\\user", "user@domain", ".\\user" или "user" или вообще ничего
	// в последнем случае пытается соединиться, используя текущий локальный аккаунт
	const char *pDomUserName= NULL,
	const char *pUserPasswd	= NULL,
	const char *pNameSpace	= NULL
);
// создать новый сервис windos
EXPORT int WinSvcCreate (
	const char *pName,						// max 256 chars, case-sensetive and no [back]slashes
	const char *pDisplayName,				// max 256 chars, case-sensetive and no [back]slashes
	const char *pPathName,					// путь к исполняемому файлу (нужно для запуска, min 14 символов)
	ServiceType serviceType		= SVCTYPE_OWN,
	ErrorControl errorControl	= ERRCTL_NOTIFY,
	int desktopInteract			= 1,		// does service can interact with desktop?
	const char *pStartName		= NULL,		// windows-аккаунт для запуска (min 14 символов)
	const char *pStartPassword	= NULL,		// пароль для входа для запуска (min 14 символов)
	const char *pLoadOrderGroup	= NULL,		// групповое имя для этого сервиса
	const char *pOrderGrpDeps	= NULL,		// список групп сервисов, от которых будет зависеть этот
	const char *pServiceDeps	= NULL,		// список сервисов, от которых будет зависеть этот
	int expandIfTooShort		= 1
);
// удалить сервис по его имени
EXPORT int WinSvcDelete(const char *pName, int expandIfTooShort = 1);
// запустить сервис по его имени
EXPORT int WinSvcStart(const char *pName, int expandIfTooShort = 1);
// остановить сервис по его имени
EXPORT int WinSvcStop(const char *pName, int expandIfTooShort = 1);
// проверить существует ли уже сервис с таким же именем, возваращает 1 если да, 0 - иначе
EXPORT int WinSvcExists(const char *pName, int expandIfTooShort = 1);
// получить указываемое значение свойства указываемого сервиса
EXPORT int WinSvcGetProperty(const char *pName, const char *pProperty, int expandIfTooShort, char* pBuff, size_t maxLen = MAXLEN);
// получить значение строкового ключа реестра
EXPORT int WinRegGetStr(long targetBranch, const char *pKeyName, const char *pValueName, int expandIfTooShort, char* pBuff, size_t maxLen = MAXLEN);
// создать значение строкового ключа реестра
EXPORT int WinRegAddStr(long targetBranch, const char *pKeyName, const char *pValueName, const char *pKeyValue, int expandIfTooShort = 1);
// удалить ключ реестра
EXPORT int WinRegDel(long targetBranch, const char *pKeyName, int expandIfTooShort = 1);
// создание и запуск нового процесса windos по указываемой командной строке
EXPORT int WinProcessCreate(const char *pCmdLine, const char *pCurrDir = NULL);
// не забываем освободить ресурсы
EXPORT void Release();
//
// } ФУНЦИИ
//
#endif // WMI_CONNECTION_H
