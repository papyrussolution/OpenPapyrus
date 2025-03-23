// WSCTL.H
// Copyright (c) A.Sobolev 2023, 2024, 2025
//
//
// Descr: Блок само-идентификации. 
//
class WsCtl_SelfIdentityBlock {
public:
	WsCtl_SelfIdentityBlock();
	int    GetOwnIdentifiers();

	S_GUID Uuid;
	MACAddrArray MacAdrList; // @v12.0.1
	S_IPAddr IpAdr; // @v12.0.1
	PPID   PrcID; // Идентификатор процессора на сервере. Инициируется ответом от сервера.
	PPID   ComputerID; // @v12.0.3 Идентификатор компьютера на сервере.
	PPID   CompCatID; // @v12.0.3 Идентификатор категории компьютера.
	SString PrcName; // Наименование процессора на сервере. Инициируется ответом от сервера.
	SString CompCatName; // @v12.0.3 Наименование категории компьютера
};
//
// 
// 
class WsCtl_Config {
public:
	WsCtl_Config();
	WsCtl_Config & Z();
	bool   IsEq(const WsCtl_Config & rS) const
	{
		return (Server == rS.Server && Port == rS.Port && Timeout == rS.Timeout && 
			DbSymb == rS.DbSymb && User == rS.User && Password == rS.Password);
	}
	//
	// Descr: Считывает конфигурацию из win-реестра
	//
	int    Read();
	//
	// Descr: Записывает конфигурацию в win-реестр
	//
	int    Write();
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJsObj);

	SString Server;
	int    Port;
	int    Timeout;
	//
	SString DbSymb;
	SString User;
	SString Password;
};

class WsCtl_LoginBlock {
public:
	WsCtl_LoginBlock();
	~WsCtl_LoginBlock();
	WsCtl_LoginBlock & Z();

	char   LoginText[256];
	char   PwText[128];	
};

class WsCtl_RegistrationBlock {
public:
	WsCtl_RegistrationBlock();
	~WsCtl_RegistrationBlock();
	WsCtl_RegistrationBlock & Z();

	char   Name[256];
	char   Phone[32];
	char   PwText[128];
	char   PwRepeatText[128];
};
/*
{
	"account": {
		"login": "abc",
		"password": "",
	},
	"app": {
		"enable": [
			"abc",
			"123"
		],
		"disable" : [
			"def",
			"456"
		]
	}
}
*/
class WsCtl_ClientPolicy {
public:
	WsCtl_ClientPolicy();
	WsCtl_ClientPolicy(const WsCtl_ClientPolicy & rS);
	WsCtl_ClientPolicy & FASTCALL Copy(const WsCtl_ClientPolicy & rS);
	WsCtl_ClientPolicy & FASTCALL operator = (const WsCtl_ClientPolicy & rS) { return Copy(rS); }
	bool FASTCALL operator == (const WsCtl_ClientPolicy & rS) { return IsEq(rS); }
	bool FASTCALL operator != (const WsCtl_ClientPolicy & rS) { return !IsEq(rS); }
	WsCtl_ClientPolicy & Z();
	bool   FASTCALL IsEq(const WsCtl_ClientPolicy & rS) const;
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJsObj);
	int    Resolve();
	int    Apply();
	//
	// Descr: Формирует строку пути к базовому каталогу системного образа для копирования и восстановления.
	//
	SString & MakeBaseSystemImagePath(SString & rBuf) const;
	//
	// Descr: Функция выясняет существует ли системный образ.
	//
	bool   IsThereSystemImage() const;
	int    CreateSystemImage();
	int    RestoreSystemImage();

	SString SysUser;
	SString SysPassword;
	StringSet SsAppEnabled;
	StringSet SsAppDisabled;
	StringSet SsAppPaths; // @v11.8.1

	struct AllowedPath {
		AllowedPath() : Flags(0)
		{
		}
		bool   FASTCALL operator == (const AllowedPath & rS) const { return (Flags == rS.Flags && Path == rS.Path); }
		uint   Flags;   // SFile::accsfXXX
		SString Path;
		SString ResolvedPath; // Так как path может быть задан в шаблонизированном виде, могут понадобиться дополнительные действия //
			// по разрешению шаблонов. Окончательный результат заносится в ResolvedPath.
	};
	struct AllowedRegistryEntry {
		AllowedRegistryEntry() : RegKeyType(0), Flags(0)
		{
		}
		bool   FASTCALL operator == (const AllowedRegistryEntry & rS) const { return (RegKeyType == rS.RegKeyType && Flags == rS.Flags && Branch == rS.Branch); }
		int    RegKeyType; // WinRegKey::regkeytypXXX general || wow64_32 || wow64_64
		uint   Flags;       
		SString Branch;
	};
	TSCollection <AllowedPath> AllowedPathList;
	TSCollection <AllowedRegistryEntry> AllowedRegList;
};
//
// Descr: Дескриптор исполняемой программы, отображаемый в клиентском окне.
//
class WsCtl_ProgramEntry {
public:
	WsCtl_ProgramEntry();
	WsCtl_ProgramEntry & Z();
	//
	// Descr: see descr of WsCtl_ProgramEntry::IsEq
	//
	bool   FASTCALL operator == (const WsCtl_ProgramEntry & rS) const { return IsEq(rS); }
	//
	// Descr: see descr of WsCtl_ProgramEntry::IsEq
	//
	bool   FASTCALL operator != (const WsCtl_ProgramEntry & rS) const { return !IsEq(rS); }
	//
	// Descr: Выясняет эквивалентность экземпляра this с экземпляром rS.
	//   Эквивалентность определяется без учета члена FullResolvedPath поскольку
	//   он вычисляется клиентом на основании контекстной информации.
	// Returns:
	//   true - *this и rS эквивалентны.
	//   false - *this и rS различаются.
	//
	bool   FASTCALL IsEq(const WsCtl_ProgramEntry & rS) const;
	SJson * ToJsonObj(bool withResolvance) const;
	int    FromJsonObj(const SJson * pJsObj);

	enum {
		fResolving_FileNFound = 0x0001, // Исполняемый файл программы не найден
		fResolving_ByCache    = 0x0002  // Полный путь к программе разрешен посредством кэша 
	};

	PPID   ID;                 // @v12.0.6 Идентификатор объекта в базе данных сервера
	PPID   CategoryID;         // @v12.0.6 Идентификатор категории в базе данных сервера
	SString Category;          // utf8 Категория программы
	SString Title;             // utf8 Отображаемый на экране заголовок программы
	SString ExeFileName;       // utf8 Имя исполняемого файла (с расширением) 
	SString FullResolvedPath;  // @transient utf8 Полный путь к исполняемому файлу.
	SString PicSymb;           // utf8 Символ изображения иконки //
	int   PicHashAlg;          // @v12.0.6 Алгоритм хэша содержимого изображения // 
	SBinaryChunk PicHash;      // @v12.0.6 Хэш содержимого изображения //
	uint64 UedTime_Resolution; // @v12.1.9 (cache) Время вычисления полного пути программы
	uint  Flags;               // @v12.1.9 @flags (в том числе флаги кэша, информирующие о результате вычисления полного пути) 
};

class WsCtl_ProgramCollection : public TSCollection <WsCtl_ProgramEntry> {
public:
	WsCtl_ProgramCollection();
	WsCtl_ProgramCollection(const WsCtl_ProgramCollection & rS);
	WsCtl_ProgramCollection & FASTCALL Copy(const WsCtl_ProgramCollection & rS);
	WsCtl_ProgramCollection & FASTCALL operator = (const WsCtl_ProgramCollection & rS) { return Copy(rS); }
	bool   FASTCALL operator == (const WsCtl_ProgramCollection & rS) const { return IsEq(rS); }
	bool   FASTCALL operator != (const WsCtl_ProgramCollection & rS) const { return !IsEq(rS); }
	//
	// Descr: Функция определяет эквивалентность экземпляра this с экземпляром rS.
	//   Эквивалентность определяется без учета членов CatList и SelectedCatSurrogateId.
	//   Кроме того, see desr of WsCtl_ProgramEntry::IsEq.
	// Returns:
	//   true - *this и rS эквивалентны.
	//   false - *this и rS различаются.// 
	//
	bool   FASTCALL IsEq(const WsCtl_ProgramCollection & rS) const;
	bool   IsResolved() const { return Resolved; }
	WsCtl_ProgramEntry * SearchByID(PPID id) const;

	enum {
		catsurrogateidAll = 10000
	};
	long   GetSelectedCatSurrogateId() const { return SelectedCatSurrogateId; }
	void   SetSelectedCatSurrogateId(long id) { SelectedCatSurrogateId = id; }
	const StrAssocArray & GetCatList() const { return CatList; }
	SJson * ToJsonObj(bool withResolvance) const;
	int    FromJsonObj(const SJson * pJsObj);
	int    MakeCatList();
	int    Resolve(const WsCtl_ClientPolicy & rPolicy);

	mutable SMtLock Lck; // Блокировка, используемая для асинхронного запуска процедура разрешения путей к программам.
private:
	StrAssocArray CatList; // @transient
	long   SelectedCatSurrogateId; // @transient
	bool   Resolved; // @transient
};
//
// Descr: Класс, отвечающий за системное сопровождение сессий:
//   -- авторизация в операционной системе
//   -- запуск и завершение процессов
//   -- управление ограничениями
//   -- отслеживание изменений в системе
//
class WsCtl_SessionFrame {
public:
	WsCtl_SessionFrame() : State(0)
	{
	}
	~WsCtl_SessionFrame()
	{
	}
	void   SetClientPolicy(const WsCtl_ClientPolicy & rP)
	{
		Policy = rP;
	}
	int    Start();
	int    Finish();
	int    LaunchProcess(const WsCtl_ProgramEntry * pPgmEntry);
private:
	struct Process : public SlProcess::Result {
		SString Name;
		SString Path;
	};
	enum {
		stRunning = 0x0001
	};
	WsCtl_ClientPolicy Policy;
	TSCollection <Process> RunningProcessList; // Список хандлеров запущенных процессов
	SPtrHandle H_UserToken;
	uint   State;
};
//
// Descr: Блок, отвечающий за взаимодействие серверной сессии с модулем WsCtl
//
class WsCtlSrvBlock {
public:
	static int Test();

	struct RegistrationBlock { // @v11.9.10
		RegistrationBlock();
		RegistrationBlock & Z();
		bool   IsValid() const;
		bool   FromJsonObj(const SJson * pJs);

		S_GUID  WsCtlUuid;
		SString Name;
		SString Phone;
		SString PwText;
		// Results:
		int    Status;  // 0 - error, 1 - success
		PPID   SCardID;
		PPID   PsnID;
	};
	struct ComputerRegistrationBlock { // @v12.0.1
		ComputerRegistrationBlock();
		ComputerRegistrationBlock & Z();
		SJson * ToJsonObj(bool asServerReply) const;
		bool   FromJsonObj(const SJson * pJs);

		S_GUID WsCtlUuid; // IN/OUT
		MACAddrArray MacAdrList;
		SString Name;
		SString CompCatName; // @v12.0.4 Наименование категории компьютера
		// Results:
		int    Status;  // 0 - error, 1 - success
		PPID   PrcID;
		PPID   ComputerID;
		PPID   CompCatID; // @v12.0.4 Идентификатор категории компьютера
	};
	struct AuthBlock {
		AuthBlock();
		AuthBlock & Z();
		//
		// Descr: Возвращает true если входящие параметры инициализированы в достаточной степени для запуска исполнения команды
		//
		bool   IsEmpty() const;
		bool   FromJsonObj(const SJson * pJs);

		S_GUID  WsCtlUuid;
		SString LoginText;
		SString Pw;
		// Results:
		PPID   SCardID;
		PPID   PsnID;
	};
	struct StartSessBlock {
		StartSessBlock();
		//
		// Descr: Возвращает true если входящие параметры инициализированы в достаточной степени для запуска исполнения команды
		//
		bool   IsEmpty() const { return (!SCardID || !GoodsID); }
		StartSessBlock & Z();
		bool   FromJsonObj(const SJson * pJs);

		S_GUID WsCtlUuid;
		PPID   SCardID;
		PPID   GoodsID;
		PPID   TechID;
		double Amount;
		//
		// Results:
		//
		PPID   TSessID;
		PPID   RetTechID;
		PPID   RetSCardID;
		PPID   RetGoodsID;
		LDATETIME ScOpDtm; // Время операции списания денег //
		double WrOffAmount; // Списанная сумма //
		STimeChunk SessTimeRange;
	};

	WsCtlSrvBlock();
	int    SearchPrcByWsCtlUuid(const S_GUID & rWsCtlUuid, PPID * pPrcID, SString * pPrcNameUtf8, ProcessorTbl::Rec * pRec); // @v12.2.4 (ProcessorTbl::Rec * pRec)
	int    Init(const S_GUID & rUuid);
	//
	// Descr: Возвращает ранжированный список видов котировок, которые могут быть применены для установки цен для технологических сессий.
	//
	int    GetRawQuotKindList(PPIDArray & rList);
	int    GetQuotList(PPID goodsID, PPID locID, const PPIDArray & rRawQkList, PPQuotArray & rQList);
	int    StartSess(StartSessBlock & rBlk);
	int    Registration(RegistrationBlock & rBlk);
	int    RegisterComputer(ComputerRegistrationBlock & rBlk);
	int    Auth(AuthBlock & rBlk);
	int    SendClientPolicy(const S_GUID & rComputerUuid, SString & rResult);
	//
	// Descr: Отправляет клиенту список програм для запуска в json-формате
	// ARG(mock IN): если true, то отправляется тестовая "болванка" workspace/wsctl/wsctl-program.json. В противном случае список 
	//   програм формируется из объекта данных PPObjSwProgram
	// ARG(rResult OUT): буфер для результата
	//
	int    SendProgramList(bool mock, const S_GUID & rComputerUuid, SString & rResult);

	PPObjTSession TSesObj;
	PPObjQuotKind QkObj;
	PPObjGoods GObj;
	PPObjPerson PsnObj;
	PPObjSCard ScObj;
	PPObjArticle ArObj;
	PPObjComputer CompObj;
	S_GUID WsUUID; // UUID управляемой рабочей станции
	PPID   PrcID;  // Ид процессора, соответствующего рабочей станции
	PPID   ComputerID;  // @v12.2.4 Идентификатор компьютера (в общем, должно выполняться правило: 
		// ProcessorRec(PrcID).LinkObjType == PPOBJ_COMPUTER && ProcessorRec(PrcID).LinkObjID == ComputerID)
	PPID   CompCatID;   // @v12.2.4 Идентификатор категории компьютера
	SString PrcNameUtf8;
private:
	PPSCardConfig ScCfg;
	PPID   ScSerID; // Серия карт (из конфигурации)
	PPID   PsnKindID; // Вид персоналий, связанный с серией карт ScSerID
};
//
// Descr: Заголовочный класс приложения, куда я сваливаю всякие общеупотребимые утилиты и данные
//
class WsCtlApp {
public:
	WsCtlApp();
	static bool GetLocalCachePath(SString & rPath);
	static int  GetProgramListFromCache(WsCtl_ProgramCollection * pPgmL, bool resolvedPrmListCache, WsCtl_ClientPolicy * pPolicyL);
	static int  StoreProgramListResolvedCache(const WsCtl_ProgramCollection & rPgmL);
private:
};