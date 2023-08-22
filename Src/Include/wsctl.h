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
	bool FASTCALL operator == (const WsCtl_ClientPolicy & rS) { return IsEq(rS); }
	bool FASTCALL operator != (const WsCtl_ClientPolicy & rS) { return !IsEq(rS); }
	WsCtl_ClientPolicy & Z();
	bool   FASTCALL IsEq(const WsCtl_ClientPolicy & rS) const;
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJsObj);
	int    Apply();

	SString SysUser;
	SString SysPassword;
	StringSet SsAppEnabled;
	StringSet SsAppDisabled;
};
//
// Descr: Дескриптор исполняемой программы, отображаемый в клиентском окне.
//
class WsCtl_ProgramEntry {
public:
	WsCtl_ProgramEntry();
	WsCtl_ProgramEntry & Z();
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJsObj);
	SString Category;         // utf8 Категория программы
	SString Title;            // utf8 Отображаемый на экране заголовок программы
	SString ExeFileName;      // utf8 Имя исполняемого файла (с расширением) 
	SString FullResolvedPath; // utf8 Полный путь к исполняемому файлу.
	SString PicSymb;          // utf8 Символ изображения иконки //
};

class WsCtl_ProgramCollection : public TSCollection <WsCtl_ProgramEntry> {
public:
	WsCtl_ProgramCollection();
	SJson * ToJsonObj() const;
	int    FromJsonObj(const SJson * pJsObj);
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
