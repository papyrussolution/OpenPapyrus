// WSCTL.H
// Copyright (c) A.Sobolev 2023, 2024
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
		ComputerRegistrationBlock() : Status(0), PrcID(0), ComputerID(0)
		{
		}
		ComputerRegistrationBlock & Z()
		{
			WsCtlUuid.Z();
			MacAdrList.clear();
			Name.Z();
			Status = 0;
			PrcID = 0;
			ComputerID = 0;
			return *this;
		}
		SJson * ToJsonObj(bool asServerReply) const
		{
			SJson * p_result = SJson::CreateObj();
			SString temp_buf;
			if(asServerReply) {
				p_result->InsertInt("status", Status);
			}
			if(Name.NotEmpty()) {
				p_result->InsertString("nm", Name);
			}
			{
				SJson * p_js_macadr_list = 0;
				for(uint i = 0; i < MacAdrList.getCount(); i++) {
					const MACAddr & r_macadr = MacAdrList.at(i);
					if(!r_macadr.IsZero()) {
						SETIFZ(p_js_macadr_list, new SJson(SJson::tARRAY));
						r_macadr.ToStr(0, temp_buf);
						SJson * p_js_macadr = SJson::CreateString(temp_buf);
						p_js_macadr_list->InsertChild(p_js_macadr);
						p_js_macadr = 0;
					}
				}
				if(p_js_macadr_list) {
					p_result->Insert("macadr_list", p_js_macadr_list);
				}
			}
			if(!!WsCtlUuid) {
				WsCtlUuid.ToStr(S_GUID::fmtIDL, temp_buf);
				p_result->InsertString("wsctluuid", temp_buf);
			}
			return p_result;
		}
		bool   FromJsonObj(const SJson * pJs)
		{
			bool   result = false;
			Z();
			if(pJs) {
				const SJson * p_c = 0;
				p_c = pJs->FindChildByKey("status");
				if(SJson::IsNumber(p_c)) {
					Status = p_c->Text.ToLong();
				}
				p_c = pJs->FindChildByKey("nm");
				if(SJson::IsString(p_c))
					Name = p_c->Text;
				p_c = pJs->FindChildByKey("wsctluuid");
				if(SJson::IsString(p_c)) {
					WsCtlUuid.FromStr(p_c->Text);
				}
				p_c = pJs->FindChildByKey("macadr_list");
				if(SJson::IsArray(p_c)) {
					SString temp_buf;
					for(const SJson * p_js_item = p_c->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsString(p_js_item) && (temp_buf = p_js_item->Text).NotEmptyS()) {
							MACAddr macadr;
							if(macadr.FromStr(temp_buf)) {
								MacAdrList.insert(&macadr);
							}
						}
					}
				}
				result = true;
			}
			return result;
		}
		S_GUID WsCtlUuid; // IN/OUT
		MACAddrArray MacAdrList;
		SString Name;
		// Results:
		int    Status;  // 0 - error, 1 - success
		PPID   PrcID;
		PPID   ComputerID;
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
	int    SearchPrcByWsCtlUuid(const S_GUID & rWsCtlUuid, PPID * pPrcID, SString * pPrcNameUtf8);
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
	int    SendClientPolicy(SString & rResult);
	int    SendProgramList(SString & rResult);

	PPObjTSession TSesObj;
	PPObjQuotKind QkObj;
	PPObjGoods GObj;
	PPObjPerson PsnObj;
	PPObjSCard ScObj;
	PPObjArticle ArObj;
	PPObjComputer CompObj;
	S_GUID WsUUID; // UUID управляемой рабочей станции
	PPID   PrcID;  // Ид процессора, соответствующего рабочей станции
	SString PrcNameUtf8;
private:
	PPSCardConfig ScCfg;
	PPID   ScSerID; // Серия карт (из конфигурации)
	PPID   PsnKindID; // Вид персоналий, связанный с серией карт ScSerID
};
