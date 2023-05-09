// PPDRVAPI.H
// Copyright (c) A.Sobolev 2013, 2019, 2020, 2021, 2023
// @codepage UTF-8
//
#ifndef __PPDRVAPI_H
#define __PPDRVAPI_H

#include <slib.h>

#define EXPORTPROC extern "C" __declspec(dllexport)

class PPBaseDriver;

typedef PPBaseDriver * (*PPDrv_CreateInstanceProc)();

class PPDrvThreadLocalArea {
public:
	friend class PPDrvSession;
	PPDrvThreadLocalArea();
	~PPDrvThreadLocalArea();
	bool   IsConsistent() const;
	long   GetId() const;
	ThreadID GetThreadID() const;
private:
	int    ResetFinishEvent();
	int    SignalFinishEvent();

	int    Sign;           // Если Sign == SIGN_PPDRVTLA, то данный объект является валидным (в частности, не разрушен деструктором)
	long   Id;             // @id
	ThreadID TId;          // Идентификатор потока
	Evnt * P_FinishEvnt;
public:
	PPBaseDriver * I;
	int    LastErr;        // Last error code //
	int    State;          //
	SString AddedMsgString;
};
//
// Descr: Утилитный класс, реализующий разбор строки входных параметров.
//   Формат строки: KEY1=VAL1; KEY2=VAL2
//   Пробелы не значимы.
//
class PPDrvInputParamBlock {
public:
	static SString & EncodeText(SString & rBuf);
	static SString & DecodeText(SString & rBuf);

	explicit PPDrvInputParamBlock(const char * pInputText);
	int    Add(const char * pParam, const char * pValue);
	SString & GetRawBuf(SString & rBuf) const;
	int    Get(const char * pParam, SString & rValue);
private:
	StringSet Ss;
	SString TempBuf;
	SString KeyBuf;
	SString ValBuf;
};

class PPDrvSession {
public:
	/* @v10.4.5 The SIntToSymbTabEntry should be used from now on.
	struct TextTableEntry {
		int    Code;
		const char * P_Text;
	};*/
	int    ImplementDllMain(HANDLE hModule, DWORD dwReason);

	PPDrvSession(const char * pName, PPDrv_CreateInstanceProc proc, uint verMajor, uint verMinor, uint errMsgCount, const SIntToSymbTabEntry * pErrMsgTab);
	~PPDrvSession();
	int    Init(HINSTANCE hInst);
	int    InitThread();
	int    ReleaseThread();
	PPDrvThreadLocalArea & GetTLA();
	const PPDrvThreadLocalArea & GetConstTLA() const;
	PPBaseDriver * GetI();
	//
	// Descr: Задает таблицу с соответствием текст сообщения об ошибке с кодами.
	//   Элементы таблицы должны соответствовать структуре TextTableEntry.
	//
	void   DefineErrTextTable(uint numEntries, const SIntToSymbTabEntry * pTab);
	int    GetLastErr();
	int    GetErrText(int errCode, SString & rBuf);
	void   SetErrCode(int errCode);
	void   SetLogFileName(const char * pFileName);

	enum {
		logfTime     = 0x0001,
		logfName     = 0x0002,
		logfThreadId = 0x0004
	};
	int    Log(const char * pMsg, long flags);
	int    Helper_ProcessCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize);
private:
	void   DefineErrText(int errCode, const char * pText);
	int    CreateI();

	const  SString Name;
	const  uint VerMajor;
	const  uint VerMinor;
	const  PPDrv_CreateInstanceProc Proc;
	long   TlsIdx;         // Ид локальной области потока    //
	long   Id;
	ACount LastThread;
	ACount DllRef;         // Счетчик активных клиентов для DLL-сервера
	SCriticalSection::Data Cs_Log; // Критическая секция защиты записи в Log и собственно переменной LogFileName
	SString LogFileName;
	StrAssocArray ErrText;
};
//
//
//
class PPBaseDriver {
public:
	enum {
		serrNotInited  = 1000,
		serrInvCommand,
		serrNoMem,
		serrNotEnoughRetBufLen
	};
	PPBaseDriver();
	virtual ~PPBaseDriver();
	//
	// Descr: Обрабатывает все команды, кроме INIT и RELEASE. Текст команды поступает в ProcessCommand
	//   в верхнем регистре.
	//
	virtual  int  ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput);
private:
	ACount RefCount;
	long   Capability;
	SString LogFileName;
};
//
// Descr: Макрос, который должен использоваться для инициализации драйвера устройства.
// Sample:
//   static PPDrvSession::TextTableEntry _SampleDrvErrTab[] = { { 1, "error text 1" }, { 2, "error text 2" } };
//   class PPDrvSample : public PPBaseDriver { };
//   PPDRV_INSTANCE_ERRTAB(Sample, 1, 0, PPDrvSample, _SampleDrvErrTab);
//
#define PPDRV_INSTANCE_ERRTAB(name, major_ver, minor_ver, cls, errtab) \
	static PPBaseDriver * CreateInstance_##name() { return new cls; } \
	PPDrvSession DRVS(#name, CreateInstance_##name, major_ver, minor_ver, SIZEOFARRAY(errtab), errtab); \
	BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) { return DRVS.ImplementDllMain(hModule, dwReason); } \
	extern "C" __declspec(dllexport) int RunCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize) \
		{ return DRVS.Helper_ProcessCommand(pCmd, pInputData, pOutputData, outSize); }

#endif // __PPDRVAPI_H
