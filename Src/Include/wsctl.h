// WSCTL.H
// Copyright (c) A.Sobolev 2023
//
//
// Descr: Блок само-идентификации. 
//
class WsCtl_SelfIdentityBlock {
public:
	WsCtl_SelfIdentityBlock();
	int    GetOwnUuid();

	S_GUID Uuid;
	PPID   PrcID; // Идентификатор процессора на сервере. Инициируется ответом от сервера.
	SString PrcName; // Наименование процессора на сервере. Инициируется ответом от сервера.
};
//
// 
// 
class WsCtl_Config {
public:
	WsCtl_Config();
	WsCtl_Config & Z();
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

	SString SysUser;
	SString SysPassword;
	StringSet SsAppEnabled;
	StringSet SsAppDisabled;
	StringSet SsAppPaths; // @v11.8.1

	struct AllowedPath {
		AllowedPath() : Flags(0)
		{
		}
		bool  FASTCALL operator == (const AllowedPath & rS) const
		{
			return (Flags == rS.Flags && Path == rS.Path);
		}
		uint  Flags;   // SFile::accsfXXX
		SString Path;
		SString ResolvedPath; // Так как path может быть задан в шаблонизированном виде, могут понадобиться дополнительные действия //
			// по разрешению шаблонов. Окончательный результат заносится в ResolvedPath.
	};
	struct AllowedRegistryEntry {
		AllowedRegistryEntry() : RegKeyType(0), Flags(0)
		{
		}
		bool   FASTCALL operator == (const AllowedRegistryEntry & rS) const
		{
			return (RegKeyType == rS.RegKeyType && Flags == rS.Flags && Branch == rS.Branch);
		}
		int   RegKeyType; // WinRegKey::regkeytypXXX general || wow64_32 || wow64_64
		uint  Flags;       
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
	SString Category;         // utf8 Категория программы
	SString Title;            // utf8 Отображаемый на экране заголовок программы
	SString ExeFileName;      // utf8 Имя исполняемого файла (с расширением) 
	SString FullResolvedPath; // @transient utf8 Полный путь к исполняемому файлу.
	SString PicSymb;          // utf8 Символ изображения иконки //
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
		//
		// Results:
		//
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
	int    SearchPrcByWsCtlUuid(const S_GUID & rWsCtlUuid, PPID * pPrcID, SString * pPrcNameUtf8);
	int    Init(const S_GUID & rUuid);
	//
	// Descr: Возвращает ранжированный список видов котировок, которые могут быть применены для установки цен для технологических сессий.
	//
	int    GetRawQuotKindList(PPIDArray & rList);
	int    GetQuotList(PPID goodsID, PPID locID, const PPIDArray & rRawQkList, PPQuotArray & rQList);
	int    StartSess(StartSessBlock & rBlk);
	int    Auth(AuthBlock & rBlk);
	int    SendClientPolicy(SString & rResult);
	int    SendProgramList(SString & rResult);

	PPObjTSession TSesObj;
	PPObjQuotKind QkObj;
	PPObjGoods GObj;
	PPObjPerson PsnObj;
	PPObjSCard ScObj;
	PPObjArticle ArObj;
	S_GUID WsUUID; // UUID управляемой рабочей станции
	PPID   PrcID;  // Ид процессора, соответствующего рабочей станции
	SString PrcNameUtf8;
private:
	PPSCardConfig ScCfg;
	PPID   ScSerID; // Серия карт (из конфигурации)
	PPID   PsnKindID; // Вид персоналий, связанный с серией карт ScSerID
};
