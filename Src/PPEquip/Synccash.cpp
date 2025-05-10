// SYNCCASH.CPP
// Copyright (c) A.Sobolev 1997-2021, 2022, 2024, 2025
// @codepage UTF-8
// Интерфейс (синхронный) для ККМ
//
#include <pp.h>
#pragma hdrstop

#define DEF_STRLEN             36   // Длина строки
#define DEF_DRAWER_NUMBER		0	// Номер денежного ящика
#define DEF_FONTSIZE			3	// Средний размер шрифта
#define MIN_FONTSIZE			1   // Минимальный размер шрифта

#define SERVICEDOC				0	// Тип документа - сервисный
#define	SALECHECK				1	// Тип документа - чек на продажу
#define RETURNCHECK				2	// Тип документа - чек на возврат
#define DEPOSITCHECK			3	// Тип документа - чек внесени
#define INCASSCHECK				4	// Тип документа - чек изъяти
#define CHECKRIBBON				0	// Печать на носитель - чековая лента
#define JOURNALRIBBON			1	// Печать на носитель - контрольная лента
#define FISCALMEM				2	// Печать на носитель - фискальная память
#define SLIPDOCUMENT			3	// Печать на носитель - подкладной документ
//
//   Режимы печати чеков и состояние чеков
//
#define PRNMODE_NO_PRINT			0x00	// Нет печати
#define NO_PAPER					0x01	// Нет бумаги
#define PRNMODE_AFTER_NO_PAPER		0x02	// Ожидание команды печати после режима 2
#define PRNMODE_PRINT				0x04	// Режим печати
#define FRMODE_OPEN_CHECK			0x08	// Чек открыт
//
//   Коды возврата при операциях печати
//
#define RESCODE_NO_ERROR			  0
#define RESCODE_UNKNOWNCOMMAND		301		// Неизвестная команда
#define RESCODE_NO_CONNECTION		303		// Соединение не установлено
#define RESCODE_SLIP_IS_EMPTY		400		// Буфер подкладного документа пуст
#define RESCODE_INVEKLZSTATE		401		// Некорректное состояние ЭКЛЗ
#define RESCODE_MEMOVERFLOW					402		// ЭКЛЗ или ФП переполнена
#define RESCODE_GOTOCTO				403		// Ошибка ЭКЛЗ. Просьба обратиться в ЦТО

//
//   Sync_BillTaxArray
//
struct Sync_BillTaxEntry { // @flat
	Sync_BillTaxEntry() : VAT(0), SalesTax(0), Amount(0.0)
	{
	}
	long   VAT;      // prec 0.01
	long   SalesTax; // prec 0.01
	double Amount;
};

class Sync_BillTaxArray : public SVector {
public:
	Sync_BillTaxArray() : SVector(sizeof(Sync_BillTaxEntry))
	{
	}
	int    Search(long VAT, long salesTax, uint * p = 0);
	int    Insert(const Sync_BillTaxEntry * pEntry, uint * p = 0);
	int    Add(Sync_BillTaxEntry * e);
	Sync_BillTaxEntry & at(uint p);
};

IMPL_CMPFUNC(Sync_BillTaxEnKey, i1, i2)
	{ RET_CMPCASCADE2(static_cast<const Sync_BillTaxEntry *>(i1), static_cast<const Sync_BillTaxEntry *>(i2), VAT, SalesTax); }

int Sync_BillTaxArray::Search(long VAT, long salesTax, uint * p)
{
	Sync_BillTaxEntry bte;
	bte.VAT      = VAT;
	bte.SalesTax = salesTax;
	return bsearch(&bte, p, PTR_CMPFUNC(Sync_BillTaxEnKey));
}

int Sync_BillTaxArray::Insert(const Sync_BillTaxEntry * pEntry, uint * p)
	{ return ordInsert(pEntry, p, PTR_CMPFUNC(Sync_BillTaxEnKey)) ? 1 : PPSetErrorSLib(); }
Sync_BillTaxEntry & Sync_BillTaxArray::at(uint p)
	{ return *static_cast<Sync_BillTaxEntry *>(SVector::at(p)); }

int Sync_BillTaxArray::Add(Sync_BillTaxEntry * e)
{
	int    ok = 1;
	uint   p;
	if(Search(e->VAT, e->SalesTax, &p)) {
		Sync_BillTaxEntry & bte = at(p);
		bte.Amount += e->Amount;
	}
	else {
		THROW(Insert(e));
	}
	CATCHZOK
	return ok;
}

class SCS_SYNCCASH : public PPSyncCashSession {
public:
	SCS_SYNCCASH(PPID n, char * pName, char * pPort);
	~SCS_SYNCCASH();
	virtual int PreprocessChZnCode(int op, const char * pCode, double qtty, int uomId, uint uomFragm, CCheckPacket::PreprocessChZnCodeResult & rResult);
	virtual int PrintCheck(CCheckPacket * pPack, uint flags);
	virtual int PrintFiscalCorrection(const PPCashMachine::FiscalCorrection * pFc);
	virtual int PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int PrintSlipDoc(const CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int GetSummator(double * val);
	virtual int GetDeviceTime(LDATETIME * pDtm);
	virtual int OpenSession_(PPID sessID); // @v11.2.12
	virtual int CloseSession(PPID sessID);
	virtual int PrintXReport(const CSessInfo *);
	virtual int PrintZReportCopy(const CSessInfo *);
	virtual int PrintIncasso(double sum, int isIncome);
	virtual int GetPrintErrCode();
	virtual int OpenBox();
	virtual int CheckForSessionOver();
	virtual int PrintBnkTermReport(const char * pZCheck);
	virtual int Diagnostics(StringSet * pSs); // @v11.1.9

	PPAbstractDevice * P_AbstrDvc;
	StrAssocArray Arr_In;
	StrAssocArray Arr_Out;
private:
	int    Connect(int forceKeepAlive = 0);
	int    AnnulateCheck();
	int    CheckForCash(double sum);
	int    PrintReport(int withCleaning);
	int	   PrintDiscountInfo(const CCheckPacket * pPack, uint flags);
	int    GetCheckInfo(const PPBillPacket * pPack, Sync_BillTaxArray * pAry, long * pFlags, SString & rName);
	int    AllowPrintOper();
	void   SetErrorMessage();
	void   CutLongTail(char * pBuf);
	void   CutLongTail(SString & rBuf);
	int    LineFeed(int lineCount, int useReceiptRibbon, int useJournalRibbon);
	int    CheckForRibbonUsing(uint ribbonParam, StrAssocArray & rOut); // ribbonParam == SlipLineParam::RegTo
	int    ExecPrintOper(int cmd, const StrAssocArray & rIn, StrAssocArray & rOut);
	int    ExecOper(int cmd, const StrAssocArray & rIn, StrAssocArray & rOut);
	int    GetStatus(int & rStatus);
	int    SetLogotype();
	int    GetPort(const char * pPortName, int * pPortNo);
	int    PutPrescription(const CCheckPacket::Prescription & rPrescr);

	enum DevFlags {
		sfConnected     = 0x0001, // установлена связь с ККМ, COM-порт занят
		sfCheckOpened     = 0x0002, // чек открыт
		sfCancelled     = 0x0004, // операция печати чека прервана пользователем
		sfPrintSlip     = 0x0010, // печать подкладного документа
		sfDontUseCutter = 0x0020, // не использовать отрезчик чеков
		sfUseWghtSensor = 0x0040, // использовать весовой датчик
		sfKeepAlive     = 0x0080, // @v10.0.12 Держать установленное соединение с аппаратом
		sfSkipAfVerif   = 0x0100, // @v10.8.0 (skip after func verification) Пропускать проверку аппарата после исполнения функций при печати чеков
		sfLogging       = 0x0200  // @v11.5.0 Вести системный файл журнала операций с кассовым регистратором
	};
	static int RefToIntrf;
	int	   Port;            // Номер порта
	long   CashierPassword; // Пароль кассира
	long   AdmPassword;     // Пароль сист.администратора
	int    ResCode;         // Код выполнения команды
	int    ErrCode;         // Код ошибки, возвращаемый командой
	int    DeviceType;
	long   CheckStrLen;     // Длина строки чека в символах
	long   Flags;           // Флаги настройки ККМ
	uint   RibbonParam;     // Носитель для печати
	int	   PrintLogo;       // Печатать логотип
	bool   IsLogoSet;       // false - логотип не загружен, true - логотип загружен
	bool   Inited;          // Если равен false, то устройство не инициализировано, true - инициализировано
	uint8  Reserve[2];      // @alignment
	SString AdmName;        // Имя сист.администратора
};

int  SCS_SYNCCASH::RefToIntrf = 0;

class CM_SYNCCASH : public PPCashMachine {
public:
	CM_SYNCCASH(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPSyncCashSession * SyncInterface();
};

PPSyncCashSession * CM_SYNCCASH::SyncInterface()
{
	PPSyncCashSession * p_cs = 0;
	if(IsValid()) {
		p_cs = new SCS_SYNCCASH(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(p_cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return p_cs;
}

REGISTER_CMT(SYNCCASH, true, false);

static void WriteLogFile_PageWidthOver(const char * pFormatName)
{
	SString msg_fmt;
	SString msg;
	PPLogMessage(PPFILNAM_SHTRIH_LOG, msg.Printf(PPLoadTextS(PPTXT_SLIPFMT_WIDTHOVER, msg_fmt), pFormatName), LOGMSGF_TIME|LOGMSGF_USER);
}

int SCS_SYNCCASH::GetPort(const char * pPortName, int * pPortNo)
{
	int    ok = 0;
	int    port = 0;
	*pPortNo = 0;
	if(pPortName) {
		int  comdvcs = IsComDvcSymb(pPortName, &port);
		if(comdvcs == comdvcsCom && port > 0 && port < 32) {
			ASSIGN_PTR(pPortNo, port-1);
			ok = 1;
		}
	}
	if(!ok)
		PPSetError(PPERR_SYNCCASH_INVPORT);
	return ok;
}

SCS_SYNCCASH::SCS_SYNCCASH(PPID n, char * name, char * port) : PPSyncCashSession(n, name, port), Port(0), CashierPassword(0),
	AdmPassword(0), ResCode(RESCODE_NO_ERROR), ErrCode(SYNCPRN_NO_ERROR), CheckStrLen(DEF_STRLEN), Flags(/*sfKeepAlive*/0),
	RibbonParam(0), Inited(false), IsLogoSet(false), PrintLogo(0), DeviceType(0)
{
	if(SCn.Flags & CASHF_NOTUSECHECKCUTTER)
		Flags |= sfDontUseCutter;
	RefToIntrf++;
	P_AbstrDvc = new PPAbstractDevice(0);
	P_AbstrDvc->PCpb.Cls = DVCCLS_SYNCPOS;
	P_AbstrDvc->GetDllName(DVCCLS_SYNCPOS, SCn.CashType, P_AbstrDvc->PCpb.DllName);
	THROW(P_AbstrDvc->IdentifyDevice(P_AbstrDvc->PCpb.DllName));
	THROW(GetPort(SCn.Port, &Port));
	CATCH
		State |= stError;
	ENDCATCH
}

SCS_SYNCCASH::~SCS_SYNCCASH()
{
	if(Flags & sfConnected) {
		ExecOper(DVCCMD_DISCONNECT, Arr_In, Arr_Out);
		ExecOper(DVCCMD_RELEASE, Arr_In, Arr_Out);
		Inited = false;
	}
	ZDELETE(P_AbstrDvc);
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, int val)
{
	SString & r_str = SLS.AcquireRvlStr();
	return rArr.Add(pos, r_str.Z().Cat(val), 1) ? 1 : PPSetErrorSLib();
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, double val)
{
	SString & r_str = SLS.AcquireRvlStr();
	return rArr.Add(pos, r_str.Z().Cat(val), 1) ? 1 : PPSetErrorSLib();
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, int64 val) // @v12.1.1
{
	SString & r_str = SLS.AcquireRvlStr();
	return rArr.Add(pos, r_str.Z().Cat(val), 1) ? 1 : PPSetErrorSLib();
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, const S_GUID & rVal) // @v12.1.1
{
	SString & r_str = SLS.AcquireRvlStr();
	return rArr.Add(pos, r_str.Z().Cat(rVal, S_GUID::fmtIDL), 1) ? 1 : PPSetErrorSLib();
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, const char * str)
	{ return rArr.Add(pos, str, 1) ? 1 : PPSetErrorSLib(); }

static void FASTCALL DestrStr(const SString & rStr, SString & rParamName, SString & rParamVal)
{
	if(rStr.NotEmpty())
		rStr.Divide('=', rParamName, rParamVal);
	else {
		rParamName.Z();
		rParamVal.Z();
	}
}

int SCS_SYNCCASH::Connect(int forceKeepAlive/*= 0*/)
{
	int    ok = 1;
	if((forceKeepAlive || Flags & sfKeepAlive) && Flags & sfConnected) {
		;
	}
	else {
		int    not_use_wght_sensor = 0;
		SString temp_buf;
		SString left;
		SString right;
		PPIniFile ini_file;
		// @v10.0.12 {
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_POSREGISTERKEEPALIVE, temp_buf.Z()) > 0) {
			if(temp_buf == "0" || temp_buf.IsEqiAscii("false") || temp_buf.IsEqiAscii("no"))
				Flags &= ~sfKeepAlive;
			else if(temp_buf == "1" || temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes"))
				Flags |= sfKeepAlive;
		}
		// } @v10.0.12
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_POSREGISTERSKIPAFVERIF, temp_buf.Z()) > 0) {
			if(temp_buf == "0" || temp_buf.IsEqiAscii("false") || temp_buf.IsEqiAscii("no"))
				Flags &= ~sfSkipAfVerif;
			else if(temp_buf == "1" || temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes"))
				Flags |= sfSkipAfVerif;
		}
		// @v11.5.0 {
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_POSREGISTERLOGGING, temp_buf.Z()) > 0) {
			if(temp_buf == "0" || temp_buf.IsEqiAscii("false") || temp_buf.IsEqiAscii("no"))
				Flags &= ~sfLogging;
			else if(temp_buf == "1" || temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes"))
				Flags |= sfLogging;
		}
		// } @v11.5.0
		if(Flags & sfConnected) {
			THROW(ExecOper(DVCCMD_DISCONNECT, Arr_In.Z(), Arr_Out));
			Flags &= ~sfConnected;
		}
		ok = 0;
		if(!Inited) {
			THROW(ExecOper(DVCCMD_INIT, Arr_In.Z(), Arr_Out));
		}
		THROW_PP(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_SHTRIHFRPASSWORD, temp_buf) > 0, PPERR_SHTRIHFRADMPASSW);
		temp_buf.Divide(',', left, AdmName);
		CashierPassword = AdmPassword = left.ToLong();
		AdmName.Strip().Transf(CTRANSF_INNER_TO_OUTER);
		{
			const  int __def_baud_rate =  7; // Скорость обмена по умолчанию 57600 бод // @v10.0.02 10-->7
			const  int __max_baud_rate = 10; // Max скорость обмена 256000 бод
			// @v10.1.2 static const int __baud_rate_list[] = { -1, 7, 8, 2, 3, 4, 5, 6, 9, 10, 1, 0 }; // the first entry is for ordered rate
			static const int __baud_rate_list[] = { -1, 8, 7, 2, 3, 4, 5, 6, 9, 10, 1, 0 }; // the first entry is for ordered rate // @v10.1.2
			/*
				0: cbr2400  1: cbr4800   2: cbr9600    3: cbr14400    4: cbr19200  5: cbr38400
				6: cbr56000 7: cbr57600  8: cbr115200  9: cbr128000  10: cbr256000
			*/
			//int    def_baud_rate = __def_baud_rate;
			int    settled_baud_rate = __def_baud_rate;
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRCONNECTPARAM, temp_buf) > 0) {
				if(temp_buf.Divide(',', left, right) > 0)
					settled_baud_rate = left.ToLong();
				if(settled_baud_rate < 0 || settled_baud_rate > __max_baud_rate)
					settled_baud_rate = __def_baud_rate;
			}
			for(uint baud_rate_idx = 0; baud_rate_idx < SIZEOFARRAY(__baud_rate_list); baud_rate_idx++) {
				int try_baud_rate = __baud_rate_list[baud_rate_idx];
				if(try_baud_rate != settled_baud_rate) {
					if(try_baud_rate == -1)
						try_baud_rate = settled_baud_rate;
					Arr_In.Z();
					THROW(ArrAdd(Arr_In, DVCPARAM_PORT, Port));
					THROW(ArrAdd(Arr_In, DVCPARAM_BAUDRATE, try_baud_rate));
					ok = ExecOper(DVCCMD_CONNECT, Arr_In, Arr_Out);
					if(ok == 1) {
						settled_baud_rate = try_baud_rate;
						break;
					}
					else {
						THROW(ResCode == RESCODE_NO_CONNECTION);
					}
				}
			}
			THROW(ok);
			Flags |= sfConnected;
		}
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHFRNOTUSEWEIGHTSENSOR, &not_use_wght_sensor);
		SETFLAG(Flags, sfUseWghtSensor, !not_use_wght_sensor);
		{
			long    cshr_pssw = 0L;
			int     logical_number = 1; // Логический номер кассы
			SString param_name;
			SString param_val;
			SString operator_name; // @v10.8.5 Name of cashier
			//
			// Получаем пароль кассира
			//
			Arr_In.Z();
			THROW(ExecPrintOper(DVCCMD_GETCONFIG, Arr_In, Arr_Out));
			if(Arr_Out.getCount()) {
				for(uint i = 0; Arr_Out.GetText(i, temp_buf) > 0; i++) {
					DestrStr(temp_buf, param_name, param_val);
					if(sstreqi_ascii(param_name, "CASHPASS"))
						CashierPassword = param_val.ToLong();
					else if(sstreqi_ascii(param_name, "CHECKSTRLEN"))
						CheckStrLen = param_val.ToLong();
					else if(sstreqi_ascii(param_name, "LOGNUM"))
						logical_number = param_val.ToLong();
				}
			}
			// Проверяем на наличие незакрытого чека
			// @v10.1.0 (будет сделана гарантированная проверка при вызове PrintCheck()) THROW(AnnulateCheck());
			// Получаем остальные параметры
			Arr_In.Z();
			{
				int    val = 0;
				THROW(ArrAdd(Arr_In, DVCPARAM_AUTOCASHNULL, 1)); // Установить автоматическое обнуление наличности
				THROW(ArrAdd(Arr_In, DVCPARAM_ID, /*pIn->*/SCn.ID)); // Опредеяем ID ККМ
				GetPort(/*pIn->*/SCn.Port, &val); // Определяем имя порта и переводим его в число
				THROW(ArrAdd(Arr_In, DVCPARAM_PORT, val));
				THROW(ArrAdd(Arr_In, DVCPARAM_LOGNUM, logical_number)); // Логический номер ККМ
				THROW(ArrAdd(Arr_In, DVCPARAM_FLAGS, /*pIn->*/SCn.Flags)); // Флаги
				THROW(ArrAdd(Arr_In, DVCPARAM_PRINTLOGO, PrintLogo)) // Логотип (пока что путь жестко определен здесь)
				{
					// Пароль сисадмина для Штрих-ФР-Ф
					//PPIniFile ini_file;
					ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_SHTRIHFRPASSWORD, temp_buf);
					temp_buf.Divide(',', left, right);
					THROW(ArrAdd(Arr_In, DVCPARAM_ADMINPASSWORD, left));
				}
				{
					// Определяем имя кассира
					PPSyncCashSession::GetCurrentUserName(operator_name);
					THROW(ArrAdd(Arr_In, DVCPARAM_CSHRNAME, operator_name));
				}
				THROW(ArrAdd(Arr_In, DVCPARAM_ADMINNAME, AdmName.NotEmpty() ? AdmName : operator_name)); // Имя администратора
				THROW(ArrAdd(Arr_In, DVCPARAM_SESSIONID, /*pIn->*/SCn.CurSessID)); // Текущая кассовая сесси
				THROW(ArrAdd(Arr_In, DVCPARAM_LOGGING, BIN(Flags & sfLogging))); // @v11.5.0
			}
			THROW(ExecOper(DVCCMD_SETCFG, Arr_In, Arr_Out));
			// Загружаем логотип
			Arr_In.Z();
			if(!IsLogoSet)
				THROW(SetLogotype());
			CashierPassword = cshr_pssw;
		}
	}
	CATCH
		if(Flags & sfConnected) {
			SetErrorMessage();
			THROW(ExecOper(DVCCMD_DISCONNECT, Arr_In.Z(), Arr_Out));
		}
		else {
			Flags |= sfConnected;
			SetErrorMessage();
		}
		Flags &= ~sfConnected;
		ok = 0;
	ENDCATCH
	return ok;
}

/*virtual*/int SCS_SYNCCASH::Diagnostics(StringSet * pSs) // @v11.1.9
{
	int    ok = 1;
	CALLPTRMEMB(pSs, Z());
	THROW(Connect());
	THROW(ExecOper(DVCCMD_DIAGNOSTICS, Arr_In.Z(), Arr_Out));
	if(pSs) {
		for(uint i = 0; i < Arr_Out.getCount(); i++) {
			StrAssocArray::Item item = Arr_Out.at_WithoutParent(i);
			if(!isempty(item.Txt))
				pSs->add(item.Txt);
		}
	}
	CATCHZOK
	return ok;
}

/*static*/void PPSyncCashSession::LogPreprocessChZnCodeResult(int ret, int op, const char * pCode, double qtty, const CCheckPacket::PreprocessChZnCodeResult & rResult) // @v11.2.3
{
	SString msg;
	msg.Cat("PreprocessChZnCode").CatDiv(':', 2).CatChar('(').Cat(ret).CatChar(')').Space();
	{
		const char * p_op_text = 0;
		switch(op) {
			case 0: p_op_text = "check"; break;
			case 1: p_op_text = "accept"; break;
			case 2: p_op_text = "reject"; break;
		}
		if(p_op_text)
			msg.Cat(p_op_text).Space();
	}
	msg.Cat(pCode).Space().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ)).Space();
	if(op == 0) {
		msg.Cat("result").CatChar('[').CatHex(static_cast<long>(rResult.CheckResult)).Space().Cat(rResult.Reason).Space().
			CatHex(static_cast<long>(rResult.ProcessingResult)).Space().
			Cat(rResult.ProcessingCode).Space().Cat(rResult.Status).CatChar(']');
	}
	PPLogMessage(PPFILNAM_CCHECK_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);	
}

int SCS_SYNCCASH::PreprocessChZnCode(int op, const char * pCode, double qtty, int uomId, uint uomFragm, CCheckPacket::PreprocessChZnCodeResult & rResult)
{
	int    ok = -1;
	if(op == ppchzcopInit) {
		;
	}
	// @v11.9.4 {
	else if(op == ppchzcopSurrogateCheck) {
		rResult.CheckResult = 0;
		rResult.Reason = 0;
		rResult.ProcessingResult = 0;
		rResult.ProcessingCode = 0;
		rResult.Status = 0;
		if(!isempty(pCode)) {
			THROW(Connect());
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_CHZNCODE, pCode));
			THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, qtty));
			if(uomId)
				THROW(ArrAdd(Arr_In, DVCPARAM_UOMID, uomId));
			THROW(ArrAdd(Arr_In, DVCPARAM_DRAFTBEERSIMPLIFIED, pCode));
			THROW(ExecOper(DVCCMD_PREPROCESSCHZNCODE, Arr_In, Arr_Out));
		}
	}
	// } @v11.9.4
	else if(op == ppchzcopCheck) {
		rResult.CheckResult = 0;
		rResult.Reason = 0;
		rResult.ProcessingResult = 0;
		rResult.ProcessingCode = 0;
		rResult.Status = 0;
		if(!isempty(pCode)) {
			GtinStruc gts;
			if(PPChZnPrcssr::InterpretChZnCodeResult(PPChZnPrcssr::ParseChZnCode(pCode, gts, 0)) == PPChZnPrcssr::chznciReal) {
				SString temp_buf;
				if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
					SString serial;
					SString partn;
					SString result_chzn_code;
					SString left, right;
					SString gtin(temp_buf);
					result_chzn_code.Cat(temp_buf);
					if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
						result_chzn_code.Cat(temp_buf);
						serial = temp_buf;
					}
					if(gts.GetToken(GtinStruc::fldPart, &temp_buf)) {
						if(serial.IsEmpty())
							result_chzn_code.Cat(temp_buf);
						partn = temp_buf;
					}
					THROW(Connect());
					Arr_In.Z();
					THROW(ArrAdd(Arr_In, DVCPARAM_CHZNCODE, pCode));
					THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, qtty));
					// @v11.2.6 {
					if(ffrac(qtty) != 0.0 && uomFragm > 0) {
						THROW(ArrAdd(Arr_In, DVCPARAM_UOMFRAGM, static_cast<int>(uomFragm)));
					}
					// } @v11.2.6 
					THROW(ExecOper(DVCCMD_PREPROCESSCHZNCODE, Arr_In, Arr_Out));
					for(uint i = 0; i < Arr_Out.getCount(); i++) {
						StrAssocArray::Item item = Arr_Out.at_WithoutParent(i);
						if(!isempty(item.Txt)) {
							if((temp_buf = item.Txt).Divide('=', left, right) > 0) {
								left.Strip();
								if(left.IsEqiAscii("CheckResult"))
									rResult.CheckResult = right.ToLong();
								else if(left.IsEqiAscii("Reason"))
									rResult.Reason = right.ToLong();
								else if(left.IsEqiAscii("ProcessingResult"))
									rResult.ProcessingResult = right.ToLong();
								else if(left.IsEqiAscii("ProcessingCode"))
									rResult.ProcessingCode = right.ToLong();
								else if(left.IsEqiAscii("Status"))
									rResult.Status = right.ToLong();
							}
						}
					}
					ok = 1;
				}
			}
		}
	}
	else if(oneof2(op, ppchzcopAccept, ppchzcopReject)) {
		THROW(Connect());
		Arr_In.Z();
		THROW(ExecOper((op == 2) ? DVCCMD_REJECTCHZNCODE : DVCCMD_ACCEPTCHZNCODE, Arr_In, Arr_Out));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int  SCS_SYNCCASH::AnnulateCheck()
{
	int    ok = -1;
	int    status = 0;
	GetStatus(status);
	// Проверка на незавершенную печать
	if(status & PRNMODE_AFTER_NO_PAPER) {
		Flags |= sfCheckOpened;
		THROW(ExecPrintOper(DVCCMD_CONTINUEPRINT, Arr_In.Z(), Arr_Out));
		do {
			GetStatus(status);
		} while(status & PRNMODE_PRINT);
		Flags &= ~sfCheckOpened;
	}
	// Проверка на наличие открытого чека, который надо аннулировать
	// @v10.1.0 GetStatus(status);
	if(status & FRMODE_OPEN_CHECK) {
		Flags |= (sfCheckOpened | sfCancelled);
		PPMessage(mfInfo|mfOK, PPINF_SHTRIHFR_CHK_ANNUL, 0);
		THROW(ExecPrintOper(DVCCMD_ANNULATE, Arr_In.Z(), Arr_Out));
		Flags &= ~(sfCheckOpened | sfCancelled);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::PrintFiscalCorrection(const PPCashMachine::FiscalCorrection * pFc)
{
	int    ok = 1;
	SString temp_buf;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	Flags |= sfCheckOpened;
	Arr_In.Z();
	/*
		"PAYMCASH"
		"PAYMCARD"
		"PREPAY"
		"POSTPAY"
		"RECKONPAY"
		"CODE"
		"DATE"
		"TEXT"
		"VATRATE"
		"VATFREE"
		"VATAMOUNT18"
		"VATAMOUNT10"
		"VATAMOUNT00"
		"VATFREEAMOUNT"
	*/
	THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCASH, pFc->AmtCash));
	THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCARD, pFc->AmtBank));
	THROW(ArrAdd(Arr_In, DVCPARAM_CODE, pFc->Code));
	THROW(ArrAdd(Arr_In, DVCPARAM_DATE, temp_buf.Z().Cat(pFc->Dt, DATF_ISO8601CENT)));
	THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, (temp_buf = pFc->Reason).Transf(CTRANSF_INNER_TO_OUTER)));
	// @v12.3.3 {
	if(pFc->FiscalSign.NotEmpty()) {
		THROW(ArrAdd(Arr_In, DVCCMD_FISCALSIGN, pFc->FiscalSign)); 
	}
	// } @v12.3.3 
	if(pFc->Flags & pFc->fVatFree) {
		THROW(ArrAdd(Arr_In, DVCPARAM_VATFREE, 1));
		THROW(ArrAdd(Arr_In, DVCPARAM_VATFREEAMOUNT, (pFc->AmtCash+pFc->AmtBank+pFc->AmtPrepay+pFc->AmtPostpay)));
	}
	else {
		if(pFc->VatRate > 0.0) {
			THROW(ArrAdd(Arr_In, DVCPARAM_VATRATE, pFc->VatRate));
		}
		else {
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT20, fabs(pFc->AmtVat20)));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT18, fabs(pFc->AmtVat18)));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT10, fabs(pFc->AmtVat10)));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT00, fabs(pFc->AmtVat00)));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT05, fabs(pFc->AmtVat05))); // @v12.2.10
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT07, fabs(pFc->AmtVat07))); // @v12.2.10
			THROW(ArrAdd(Arr_In, DVCPARAM_VATFREEAMOUNT, fabs(pFc->AmtNoVat)));
		}
	}
	THROW(ExecPrintOper(DVCCMD_CHECKCORRECTION, Arr_In, Arr_Out));
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = 0;
		}
	ENDCATCH
	Flags &= ~sfCheckOpened;
	return ok;
}

int SCS_SYNCCASH::PutPrescription(const CCheckPacket::Prescription & rPrescr)
{
	bool    ok = true;
	if(rPrescr.Number.NotEmpty()) {
		THROW(ArrAdd(Arr_In, DVCPARAM_PRESCRNUMB, rPrescr.Number)); 
		if(rPrescr.Serial.NotEmpty()) {
			THROW(ArrAdd(Arr_In, DVCPARAM_PRESCRSERIAL, rPrescr.Serial)); 
		}
		if(checkdate(rPrescr.Dt)) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			r_temp_buf.Z().Cat(rPrescr.Dt, DATF_ISO8601CENT);
			THROW(ArrAdd(Arr_In, DVCPARAM_PRESCRDATE, r_temp_buf)); 
		}
	}
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::PrintCheck(CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	int    chk_no = 0;
	int    is_format = 0;
	SString buf;
	SString input;
	SString param_name;
	SString param_val;
	SString temp_buf;
	PPObjGoods goods_obj;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	THROW_INVARG(pPack);
	if(pPack->GetCount() == 0)
		ok = -1;
	else {
		SlipDocCommonParam sdc_param;
		PPID   tax_sys_id = 0;
		OfdFactors ofdf; // @v11.3.12
		const  int   ccop = pPack->GetCcOp(); // @v12.3.3
		const  bool  is_vat_free = (CnObj.IsVatFree(NodeID) > 0);
		double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
		double sum = fabs(pPack->_Cash) + 0.001;
		double running_total = 0.0;
		double real_fiscal = 0.0;
		double real_nonfiscal = 0.0;
		CnObj.GetTaxSystem(NodeID, pPack->Rec.Dt, &tax_sys_id);
		pPack->HasNonFiscalAmount(&real_fiscal, &real_nonfiscal);
		const double _fiscal = (PPConst::Flags & PPConst::fDoSeparateNonFiscalCcItems) ? real_fiscal : (real_fiscal + real_nonfiscal);
		const CcAmountList & r_al = pPack->AL_Const();
		const int is_al = BIN(r_al.getCount());
		const double amt_bnk = is_al ? r_al.Get(CCAMTTYP_BANK) : ((pPack->Rec.Flags & CCHKF_BANKING) ? _fiscal : 0.0);
		const double amt_cash = (PPConst::Flags & PPConst::fDoSeparateNonFiscalCcItems) ? (_fiscal - amt_bnk) : (is_al ? r_al.Get(CCAMTTYP_CASH) : (_fiscal - amt_bnk));
		const double amt_ccrd = is_al ? r_al.Get(CCAMTTYP_CRDCARD) : (real_fiscal + real_nonfiscal - _fiscal);
		SString buyers_email;
		SString buyers_phone;
		bool  paperless = false; 
		// @v12.3.3 @construction {
		if(oneof4(ccop, CCOP_CORRECTION_SELL, CCOP_CORRECTION_SELLSTORNO, CCOP_CORRECTION_RET, CCOP_CORRECTION_RETSTORNO)) {
			PPCashMachine::FiscalCorrection fc;
			/*
				struct FiscalCorrection {
					FiscalCorrection();
					enum {
						fIncome    = 0x0001, // Приход денег (отрицательная коррекция). Если не стоит, то - расход.
						fByPrecept = 0x0002, // Коррекция по предписанию
						fVatFree   = 0x0004  // Продавец освобожден от НДС
					};
					double AmtCash;    // @#{>=0} Сумма наличного платежа
					double AmtBank;    // @#{>=0} Сумма электронного платежа
					double AmtPrepay;  // @#{>=0} Сумма предоплатой
					double AmtPostpay; // @#{>=0} Сумма постоплатой
					double AmtVat20;   // Сумма налога по ставке 20%
					double AmtVat18;   // Сумма налога по ставке 18%
					double AmtVat10;   // Сумма налога по ставке 10%
					double AmtVat07;   // @v12.2.5 Сумма налога по ставке 7%
					double AmtVat05;   // @v12.2.5 Сумма налога по ставке 5%
					double AmtVat00;   // Сумма расчета по ставке 0%
					double AmtNoVat;   // Сумма расчета без налога
					double VatRate;    // Единственная ставка НДС. Если VatRate != 0, тогда AmtVat18, AmtVat10, AmtVat00 и AmtNoVat игнорируются
					LDATE  Dt;         // Дата документа основания коррекции
					long   Flags;      // @flags
					SString Code;      // Номер документа основания коррекции
					SString Reason;    // Основание коррекции
					SString Operator;  // Имя оператора
					SString FiscalSign; // @v12.3.3 Фискальный признак чека
				};
			*/ 
			//PrintFiscalCorrection(&fc);
		}
		// } @v12.3.3 
		else {
			CCheckPacket::Prescription prescr; // @v11.8.0
			pPack->GetPrescription(prescr); // @v11.8.0
			GetOfdFactors(ofdf); // @v11.3.12
			// @v11.3.6 {
			{
				pPack->GetExtStrData(CCheckPacket::extssBuyerPhone, temp_buf);
				if(temp_buf.NotEmpty())
					PPEAddr::Phone::NormalizeStr(temp_buf, PPEAddr::Phone::nsfPlus, buyers_phone);
				else
					buyers_phone.Z();
				pPack->GetExtStrData(CCheckPacket::extssBuyerEMail, buyers_email);
				paperless = (buyers_email.NotEmpty() || buyers_phone.NotEmpty()) ? LOGIC(pPack->Rec.Flags & CCHKF_PAPERLESS) : false;
			}
			// } @v11.3.6 
			THROW(Connect());
			THROW(AnnulateCheck());
			PreprocessCCheckForOfd12(ofdf, pPack); // @v11.3.12 Блок, созданный в v11.1.11 замещен общей функцией базового класса
			if(flags & PRNCHK_RETURN && amt_cash != 0.0) {
				const int is_cash = CheckForCash(amt); // @v12.0.5 @fix amt-->amt_cash
				THROW(is_cash);
				THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
			}
			if(P_SlipFmt) {
				int    prn_total_sale = 1;
				int    r = 0;
				SString line_buf;
				SlipLineParam sl_param;
				const char * p_format_name = "CCheck";
				THROW(r = P_SlipFmt->Init(p_format_name, &sdc_param));
				if(r > 0) {
					P_SlipFmt->InitIteration(pPack);
					P_SlipFmt->NextIteration(line_buf, &sl_param);
					is_format = 1;
					if(sdc_param.PageWidth > static_cast<uint>(CheckStrLen))
						WriteLogFile_PageWidthOver(p_format_name);
					RibbonParam = 0;
					Arr_In.Z();
					CheckForRibbonUsing(sdc_param.RegTo, Arr_In);
					{
						PROFILE_START_S("DVCCMD_OPENCHECK");
						THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, (_fiscal != 0.0) ? ((flags & PRNCHK_RETURN) ? RETURNCHECK : SALECHECK) : SERVICEDOC));
						THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, pPack->Rec.Code));
						// @v11.2.3 {
						{
							LDATETIME ccts;
							temp_buf.Z().Cat(ccts.Set(pPack->Rec.Dt, pPack->Rec.Tm), DATF_ISO8601CENT, 0);
							THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTIMESTAMP, temp_buf));
						}
						// } @v11.2.3 
						THROW(ArrAdd(Arr_In, DVCPARAM_TAXSYSTEM, tax_sys_id));
						temp_buf.Z();
						if(!ofdf.OfdVer_.IsEmpty())
							ofdf.OfdVer_.ToStr(temp_buf);
						THROW(ArrAdd(Arr_In, DVCPARAM_OFDVER, temp_buf)); // @v11.1.9
						// @v11.3.6 {
						if(paperless) {
							THROW(ArrAdd(Arr_In, DVCPARAM_PAPERLESS, 1)); 
						}
						// } @v11.3.6 
						THROW(PutPrescription(prescr)); // @v11.8.0 // @v11.9.3 (one call)
						THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
						PROFILE_END
						if(_fiscal == 0.0 && !paperless) { // @v11.3.6
							PROFILE_START_S("DVCCMD_PRINTTEXT")
							Arr_In.Z();
							THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, sdc_param.Title));
							THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
							PROFILE_END
						}
					}
					Flags |= sfCheckOpened;
					for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
						Arr_In.Z();
						if(sl_param.Flags & SlipLineParam::fRegFiscal) {
							CheckForRibbonUsing(SlipLineParam::fRegRegular | SlipLineParam::fRegJournal, Arr_In);
							const double _q = sl_param.Qtty;
							const double _p = sl_param.Price;
							running_total += (_q * _p);
							PROFILE_START_S("DVCCMD_PRINTFISCAL")
							THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, sl_param.Text));
							THROW(ArrAdd(Arr_In, DVCPARAM_CODE, sl_param.Code));
							THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, _q));
							// @v11.9.4 {
							if(sl_param.PhQtty > 0.0) {
								THROW(ArrAdd(Arr_In, DVCPARAM_PHQTTY, sl_param.PhQtty));
							}
							if(sl_param.Flags & SlipLineParam::fDraftBeerSimplified && sl_param.Code.NotEmpty()) {
								THROW(ArrAdd(Arr_In, DVCPARAM_DRAFTBEERSIMPLIFIED, sl_param.Code));
								if(sl_param.ChZnGTIN.NotEmpty())
									THROW(ArrAdd(Arr_In, DVCPARAM_CHZNGTIN, sl_param.ChZnGTIN));
							}
							// } @v11.9.4 
							// @v11.9.5 {
							if(sl_param.UomId) {
								THROW(ArrAdd(Arr_In, DVCPARAM_UOMID, sl_param.UomId));
							}
							// } @v11.9.5 
							// @v11.2.6 {
							if(sl_param.UomFragm > 0) {
								THROW(ArrAdd(Arr_In, DVCPARAM_UOMFRAGM, static_cast<int>(sl_param.UomFragm)));
							}
							// } @v11.2.6 
							THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, fabs(_p)));
							THROW(ArrAdd(Arr_In, DVCPARAM_DEPARTMENT, (sl_param.DivID > 16 || sl_param.DivID < 0) ? 0 :  sl_param.DivID));
							if(is_vat_free) {
								THROW(ArrAdd(Arr_In, DVCPARAM_VATFREE, 1));
							}
							else {
								THROW(ArrAdd(Arr_In, DVCPARAM_VATRATE, fabs(sl_param.VatRate)));
							}
							if(sl_param.ChZnCode.NotEmptyS()) {
								THROW(ArrAdd(Arr_In, DVCPARAM_CHZNCODE, sl_param.ChZnCode));
								THROW(ArrAdd(Arr_In, DVCPARAM_CHZNGTIN, sl_param.ChZnGTIN));
								THROW(ArrAdd(Arr_In, DVCPARAM_CHZNSERIAL, sl_param.ChZnSerial));
								THROW(ArrAdd(Arr_In, DVCPARAM_CHZNPARTN, sl_param.ChZnPartN));
								THROW(ArrAdd(Arr_In, DVCPARAM_CHZNPRODTYPE, sl_param.ChZnProductType));
								// @v11.1.11 {
								if(sl_param.PpChZnR.LineIdx > 0) {
									THROW(ArrAdd(Arr_In, DVCPARAM_CHZNPPRESULT, sl_param.PpChZnR.CheckResult)); // @v11.1.11 Результат проверки марки честный знак на фазе препроцессинга
									THROW(ArrAdd(Arr_In, DVCPARAM_CHZNPPSTATUS, sl_param.PpChZnR.Status)); // @v11.1.11 Статус, присвоенный марке честный знак на фазе препроцессинга
								}
								// } @v11.1.11
								// @v11.2.4 {
								if(ofdf.Sid.NotEmpty())
									THROW(ArrAdd(Arr_In, DVCPARAM_CHZNSID, ofdf.Sid));
								// } @v11.2.4
								// @v12.1.1 {
								if(!sl_param.ChZnPm_ReqId.IsZero() && sl_param.ChZnPm_ReqTimestamp != 0) {
									THROW(ArrAdd(Arr_In, DVCPARAM_CHZNPMREQID, sl_param.ChZnPm_ReqId));
									THROW(ArrAdd(Arr_In, DVCPARAM_CHZNPMREQTIMESTAMP, sl_param.ChZnPm_ReqTimestamp));
								}
								// } @v12.1.1
							}
							if(sl_param.PaymTermTag != CCheckPacket::pttUndef) {
								uint   str_id = 0;
								switch(sl_param.PaymTermTag) {
									case CCheckPacket::pttFullPrepay: str_id = DVCPARAM_PTT_FULL_PREPAY; break;
									case CCheckPacket::pttPrepay: str_id = DVCPARAM_PTT_PREPAY; break;
									case CCheckPacket::pttAdvance: str_id = DVCPARAM_PTT_ADVANCE; break;
									case CCheckPacket::pttFullPayment: str_id = DVCPARAM_PTT_FULLPAYMENT; break;
									case CCheckPacket::pttPartial: str_id = DVCPARAM_PTT_PARTIAL; break;
									case CCheckPacket::pttCreditHandOver: str_id = DVCPARAM_PTT_CREDITHANDOVER; break;
									case CCheckPacket::pttCredit: str_id = DVCPARAM_PTT_CREDIT; break;
								}
								if(str_id) {
									PPLoadString(PPSTR_ABDVCCMD, str_id, temp_buf);
									THROW(ArrAdd(Arr_In, DVCPARAM_PAYMENTTERMTAG, temp_buf));
								}
							}
							// @erikJ v10.4.12 {
							if(sl_param.SbjTermTag != CCheckPacket::sttUndef) {
								uint   str_id = 0;
								switch(sl_param.SbjTermTag) {
									case CCheckPacket::sttGood: str_id = DVCPARAM_STT_GOOD; break;
									case CCheckPacket::sttExcisableGood: str_id = DVCPARAM_STT_EXCISABLEGOOD; break;
									case CCheckPacket::sttExecutableWork: str_id = DVCPARAM_STT_EXECUTABLEWORK; break;
									case CCheckPacket::sttService: str_id = DVCPARAM_STT_SERVICE; break;
									case CCheckPacket::sttBetting: str_id = DVCPARAM_STT_BETTING; break;
									case CCheckPacket::sttPaymentGambling: str_id = DVCPARAM_STT_PAYMENTGAMBLING; break;
									case CCheckPacket::sttBettingLottery: str_id = DVCPARAM_STT_BETTINGLOTTERY; break;
									case CCheckPacket::sttPaymentLottery: str_id = DVCPARAM_STT_PAYMENTLOTTERY; break;
									case CCheckPacket::sttGrantRightsUseIntellectualActivity: str_id = DVCPARAM_STT_GRANTSRIGHTSUSEINTELLECTUALACTIVITY; break;
									case CCheckPacket::sttAdvance: str_id = DVCPARAM_STT_ADVANCE; break;
									case CCheckPacket::sttPaymentsPayingAgent: str_id = DVCPARAM_STT_PAYMENTSPAYINGAGENT; break;
									case CCheckPacket::sttSubjTerm: str_id = DVCPARAM_STT_SUBJTERM; break;
									case CCheckPacket::sttNotSubjTerm: str_id = DVCPARAM_STT_NOTSUBJTERM; break;
									case CCheckPacket::sttTransferPropertyRights: str_id = DVCPARAM_STT_TRANSFERPROPERTYRIGHTS; break;
									case CCheckPacket::sttNonOperatingIncome: str_id = DVCPARAM_STT_NONOPERATINGINCOME; break;
									case CCheckPacket::sttExpensesReduceTax: str_id = DVCPARAM_STT_EXPENSESREDUCETAX; break;
									case CCheckPacket::sttAmountMerchantFee: str_id = DVCPARAM_STT_AMOUNTMERCHANTFEE; break;
									case CCheckPacket::sttResortFee: str_id = DVCPARAM_STT_RESORTFEE; break;
									case CCheckPacket::sttDeposit: str_id = DVCPARAM_SUBJTERMTAG; break;
								}
								if(str_id) {
									PPLoadString(PPSTR_ABDVCCMD, str_id, temp_buf);
									THROW(ArrAdd(Arr_In, DVCPARAM_SUBJTERMTAG, temp_buf));
								}
							}
							// } @erik v10.4.12
							THROW(PutPrescription(prescr)); // @v11.8.0 // @v11.9.3 (one call)
							THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
							PROFILE_END
							Flags |= sfCheckOpened;
							prn_total_sale = 0;
						}
						else if(sl_param.Kind == sl_param.lkBarcode) {
							;
						}
						else if(sl_param.Kind == sl_param.lkSignBarcode) {
							if(!paperless) { // @v11.3.6
								if(line_buf.NotEmptyS()) {
									PROFILE(CheckForRibbonUsing(SlipLineParam::fRegRegular, Arr_In));
									// type: EAN8 EAN13 UPCA UPCE CODE39 IL2OF5 CODABAR PDF417 QRCODE
									// width (points)
									// height (points)
									// label : none below above
									// text: code
									PPBarcode::GetStdName(sl_param.BarcodeStd, temp_buf);
									temp_buf.SetIfEmpty("qr"); // "pdf417";
									PROFILE_START_S("DVCCMD_PRINTBARCODE")
									ArrAdd(Arr_In, DVCPARAM_TYPE, temp_buf);
									ArrAdd(Arr_In, DVCPARAM_WIDTH,  sl_param.BarcodeWd);
									ArrAdd(Arr_In, DVCPARAM_HEIGHT, sl_param.BarcodeHt);
									const char * p_label_place = (sl_param.Flags & sl_param.fBcTextBelow) ? "below" : ((sl_param.Flags & sl_param.fBcTextAbove) ? "above" : 0);
									if(p_label_place)
										ArrAdd(Arr_In, DVCPARAM_LABEL, p_label_place);
									THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf));
									THROW(ExecPrintOper(DVCCMD_PRINTBARCODE, Arr_In, Arr_Out));
									PROFILE_END
								}
							}
						}
						else {
							if(!paperless) { // @v11.3.6
								PROFILE(CheckForRibbonUsing(sl_param.Flags, Arr_In));
								PROFILE_START_S("DVCCMD_PRINTTEXT")
								THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf.Trim((sl_param.Font > 1) ? (CheckStrLen / 2) : CheckStrLen)));
								THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, (sl_param.Font == 1) ? DEF_FONTSIZE : sl_param.Font));
								THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
								PROFILE_END
							}
						}
					}
					Arr_In.Z();
					running_total = fabs(running_total);
					CheckForRibbonUsing(SlipLineParam::fRegRegular|SlipLineParam::fRegJournal, Arr_In);
					if(prn_total_sale) {
						if(_fiscal != 0.0) {
							PROFILE_START_S("DVCCMD_PRINTFISCAL")
							if(!pPack->GetCount()) {
								THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, 1L));
								THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, amt));
								THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
								Flags |= sfCheckOpened;
								running_total += amt;
							}
							else /*if(fiscal != 0.0)*/ {
								THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, 1L));
								THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, _fiscal));
								THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
								Flags |= sfCheckOpened;
								running_total += _fiscal;
							}
							PROFILE_END
						}
					}
					else if(running_total != amt) {
						SString fmt_buf, msg_buf, added_buf;
						PPLoadText(PPTXT_SHTRIH_RUNNGTOTALGTAMT, fmt_buf);
						const char * p_sign = (running_total > amt) ? " > " : ((running_total < amt) ? " < " : " ?==? ");
						added_buf.Z().Cat(running_total, MKSFMTD(0, 12, NMBF_NOTRAILZ)).Cat(p_sign).Cat(amt, MKSFMTD(0, 12, NMBF_NOTRAILZ));
						msg_buf.Printf(fmt_buf, added_buf.cptr());
						PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
				}
			}
			if(!is_format) {
				CCheckLineTbl::Rec ccl;
				for(uint pos = 0; pPack->EnumLines(&pos, &ccl);) {
					int  division = (ccl.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? ccl.DivID - CHECK_LINE_IS_PRINTED_BIAS : ccl.DivID;
					// Наименование товара
					GetGoodsName(ccl.GoodsID, buf);
					buf.Strip().Transf(CTRANSF_INNER_TO_OUTER).Trim(CheckStrLen);
					Arr_In.Z();
					THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, buf));
					THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
					// Цена
					Arr_In.Z();
					THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, R2(intmnytodbl(ccl.Price) - ccl.Dscnt)));
					// Количество
					THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, R3(fabs(ccl.Quantity))));
					// Отдел
					THROW(ArrAdd(Arr_In, DVCPARAM_DEPARTMENT, (division > 16 || division < 0) ? 0 : division));
					THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
					Flags |= sfCheckOpened;
				}
				// Информация о скидке
				THROW(PrintDiscountInfo(pPack, flags));
				buf.Z().CatCharN('=', CheckStrLen);
				Arr_In.Z();
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, buf));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
			Arr_In.Z();
			if(flags & PRNCHK_RETURN) {
				if(amt_bnk != 0.0) { THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCARD, fabs(amt_bnk))) }
				if(amt_cash != 0.0) { THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCASH, fabs(amt_cash))); }
				if(amt_ccrd != 0.0) { THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCCRD, fabs(amt_ccrd))); }
			}
			else {
				if(amt_bnk > 0.0) { THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCARD, amt_bnk)) }
				if(amt_cash > 0.0) { THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCASH, amt_cash)); }
				if(amt_ccrd > 0.0) { THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCCRD, amt_ccrd)); }
			}
			{
				if(ofdf.Sid.NotEmpty())
					THROW(ArrAdd(Arr_In, DVCPARAM_CHZNSID, ofdf.Sid));
			}
			// @v11.3.6 {
			if(buyers_email.NotEmptyS()) {
				THROW(ArrAdd(Arr_In, DVCPARAM_BUYERSEMAIL, buyers_email));
			}
			if(buyers_phone.NotEmptyS()) {
				THROW(ArrAdd(Arr_In, DVCPARAM_BUYERSPHONE, buyers_phone));
			}
			// } @v11.3.6 
			THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out)); // Всегда закрываем чек
			Flags &= ~sfCheckOpened;
			ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, 0));
			THROW(ExecPrintOper(DVCCMD_GETCHECKPARAM, Arr_In, Arr_Out));
			if(Arr_Out.getCount()) {
				for(uint i = 0; Arr_Out.GetText(i, buf) > 0; i++) {
					DestrStr(buf, param_name, param_val);
					if(param_name.IsEqiAscii("CHECKNUM"))
						pPack->Rec.Code = static_cast<int32>(param_val.ToInt64());
				}
			}
			ErrCode = SYNCPRN_NO_ERROR;
		}
	}
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				SString no_print_txt;
				PPLoadText(PPTXT_CHECK_NOT_PRINTED, no_print_txt);
				ErrCode = (Flags & sfCheckOpened) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				PPLogMessage(PPFILNAM_SHTRIH_LOG, CCheckCore::MakeCodeString(&pPack->Rec, 0, no_print_txt), LOGMSGF_TIME|LOGMSGF_USER);
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			if(Flags & sfCheckOpened)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int SCS_SYNCCASH::CheckForCash(double sum)
{
	double cash_sum = 0.0;
	return GetSummator(&cash_sum) ? ((cash_sum < sum) ? -1 : 1) : 0;
}

int SCS_SYNCCASH::GetSummator(double * val)
{
	int    ok = 1;
	double cash_amt = 0.0;
	ResCode = RESCODE_NO_ERROR;
	SString input, buf, param_name, param_val;
	THROW(Connect(1)); // @v10.1.0 ()-->(1)
	Arr_In.Z();
	THROW(ArrAdd(Arr_In, DVCPARAM_CASHAMOUNT, 0));
	THROW(ExecPrintOper(DVCCMD_GETCHECKPARAM, Arr_In, Arr_Out));
	if(Arr_Out.getCount()) {
		for(uint i = 0; Arr_Out.GetText(i, buf) > 0; i++) {
			DestrStr(buf, param_name, param_val);
			if(param_name.IsEqiAscii("CASHAMOUNT"))
				cash_amt = param_val.ToReal();
		}
	}
	CATCH
		ok = (SetErrorMessage(), 0);
	ENDCATCH
	ASSIGN_PTR(val, cash_amt);
	return ok;
}

/*virtual*/int SCS_SYNCCASH::GetDeviceTime(LDATETIME * pDtm)
{
	int    ok = -1;
	LDATETIME dtm = ZERODATETIME;
	ResCode = RESCODE_NO_ERROR;
	SString input, buf, param_name, param_val;
	THROW(Connect(1));
	Arr_In.Z();
	THROW(ExecPrintOper(DVCCMD_GETDEVICETIME, Arr_In, Arr_Out));
	if(Arr_Out.getCount()) {
		for(uint i = 0; Arr_Out.GetText(i, buf) > 0; i++) {
			strtodatetime(buf, &dtm, DATF_ISO8601, 0);
			if(checkdate(dtm.d)) {
				ok = 1;
				break;
			}
			else {
				DestrStr(buf, param_name, param_val);
				if(param_name.IsEqiAscii("DEVICETIME")) {
					strtodatetime(param_val, &dtm, DATF_ISO8601, TIMF_HMS);
					if(checkdate(dtm.d)) {
						ok = 1;
						break;
					}
				}
			}
		}
	}
	CATCH
		ok = (SetErrorMessage(), 0);
	ENDCATCH
	ASSIGN_PTR(pDtm, dtm);
	return ok;
}

int	SCS_SYNCCASH::PrintDiscountInfo(const CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	SString input;
	double amt = R2(fabs(MONEYTOLDBL(pPack->Rec.Amount)));
	double dscnt = R2(MONEYTOLDBL(pPack->Rec.Discount));
	if(flags & PRNCHK_RETURN)
		dscnt = -dscnt;
	if(dscnt > 0.0) {
		double  pcnt = round(dscnt * 100.0 / (amt + dscnt), 1);
		SString prn_str;
		SString temp_buf;
		SString word_buf;
		SCardCore scc;
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN('-', CheckStrLen)));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		temp_buf.Z().Cat(amt + dscnt, SFMT_MONEY);
		// @v10.4.9 prn_str = "СУММА БЕЗ СКИДКИ"; // @cstr #0
		// @v10.4.9 {
		PPLoadText(PPTXT_CCFMT_AMTWODISCOUNT, word_buf); // СУММА БЕЗ СКИДКИ
		prn_str = word_buf.Transf(CTRANSF_INNER_TO_OUTER);
		// } @v10.4.9
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_buf.Len()).Cat(temp_buf);
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		if(scc.Search(pPack->Rec.SCardID, 0) > 0) {
			Arr_In.Z();
			// @v10.4.9 {
			PPLoadText(PPTXT_CCFMT_CARD, word_buf); // КАРТА
			word_buf.Transf(CTRANSF_INNER_TO_OUTER);
			// } @v10.4.9
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, (prn_str = word_buf).Space().Cat(scc.data.Code)));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			if(scc.data.PersonID && GetPersonName(scc.data.PersonID, temp_buf) > 0) { // @v6.0.9 GetObjectName-->GetPersonName
				// @v10.4.9 {
				PPLoadText(PPTXT_CCFMT_CARDOWNER, word_buf); // ВЛАДЕЛЕЦ
				word_buf.Transf(CTRANSF_INNER_TO_OUTER);
				// } @v10.4.9
				(prn_str = word_buf).Space().Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)); // @cstr #2
				CutLongTail(prn_str);
				Arr_In.Z();
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
		}
		temp_buf.Z().Cat(dscnt, SFMT_MONEY);
		// @v10.4.9 {
		PPLoadText(PPTXT_CCFMT_DISCOUNT, word_buf); // СКИДКА
		word_buf.Transf(CTRANSF_INNER_TO_OUTER);
		// } @v10.4.9
		(prn_str = word_buf).Space().Cat(pcnt, MKSFMTD(0, (flags & PRNCHK_ROUNDINT) ? 0 : 1, NMBF_NOTRAILZ)).CatChar('%'); // @cstr #3
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_buf.Len()).Cat(temp_buf);
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
	}
	CATCHZOK
	return ok;
}

void SCS_SYNCCASH::CutLongTail(char * pBuf)
{
	if(pBuf && sstrlen(pBuf) > (uint)CheckStrLen) {
		pBuf[CheckStrLen + 1] = 0;
		char * p = sstrrchr(pBuf, ' ');
		if(p)
			*p = 0;
		else
			pBuf[CheckStrLen] = 0;
	}
}

void SCS_SYNCCASH::CutLongTail(SString & rBuf)
{
	char  buf[256];
	rBuf.CopyTo(buf, sizeof(buf));
	CutLongTail(buf);
	rBuf = buf;
}

int SCS_SYNCCASH::CloseSession(PPID sessID) { return PrintReport(1); }

int SCS_SYNCCASH::OpenSession_(PPID sessID) // @v11.2.12
{
	int    ok = 1;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	Arr_In.Z();
	THROW(ExecPrintOper(DVCCMD_OPENSESSION, Arr_In, Arr_Out));
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::PrintXReport(const CSessInfo *) { return PrintReport(0); }

int SCS_SYNCCASH::PrintReport(int withCleaning)
{
	int    ok = 1;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	Flags |= sfCheckOpened;
	Arr_In.Z();
	if(withCleaning) {
		THROW(ExecPrintOper(DVCCMD_ZREPORT, Arr_In, Arr_Out));
	}
	else {
		THROW(ExecPrintOper(DVCCMD_XREPORT, Arr_In, Arr_Out));
	}
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = 0;
		}
	ENDCATCH
	Flags &= ~sfCheckOpened;
	return ok;
}

void SCS_SYNCCASH::SetErrorMessage()
{
	Arr_In.Z();
	if((Flags & sfConnected) && ResCode != RESCODE_NO_ERROR && ExecOper(DVCCMD_GETLASTERRORTEXT, Arr_In, Arr_Out) > 0) {
		SString err_msg, err_buf;
		for(uint i = 0; Arr_Out.GetText(i, err_buf) > 0; i++)
			err_msg.Cat(err_buf);
		PPSetError(PPERR_SYNCCASH, err_msg.Transf(CTRANSF_OUTER_TO_INNER));
		ResCode = RESCODE_NO_ERROR;
	}
}

int  SCS_SYNCCASH::CheckForRibbonUsing(uint ribbonParam, StrAssocArray & rOut)
{
	int    ok = 1;
	if(ribbonParam) {
		//if((RibbonParam & SlipLineParam::fRegRegular) != (ribbonParam & SlipLineParam::fRegRegular)) {
		if(!TESTFLAG(RibbonParam, ribbonParam, SlipLineParam::fRegRegular)) {
			if(ribbonParam & SlipLineParam::fRegRegular) {
                THROW(ArrAdd(rOut, DVCPARAM_RIBBONPARAM, CHECKRIBBON));
				SETFLAG(RibbonParam, SlipLineParam::fRegRegular, ribbonParam & SlipLineParam::fRegRegular);
			}
		}
		//if((RibbonParam & SlipLineParam::fRegJournal) != (ribbonParam & SlipLineParam::fRegJournal)) {
		if(!TESTFLAG(RibbonParam, ribbonParam, SlipLineParam::fRegJournal)) {
			if(ribbonParam & SlipLineParam::fRegJournal) {
				THROW(ArrAdd(rOut, DVCPARAM_RIBBONPARAM, JOURNALRIBBON));
				SETFLAG(RibbonParam, SlipLineParam::fRegJournal, ribbonParam & SlipLineParam::fRegJournal);
			}
		}
	}
	CATCHZOK;
	return ok;
}

int SCS_SYNCCASH::CheckForSessionOver()
{
	int    ok = -1;
	THROW(Connect());
	THROW(ExecPrintOper(DVCCMD_CHECKSESSOVER, Arr_In.Z(), Arr_Out));
	if(Arr_Out.getCount()) {
		SString buf;
		Arr_Out.GetText(0, buf);
		if(buf == "1") // Сессия больше 24 часов
			ok = 1;
	}
	CATCHZOK;
	return ok;
}

int SCS_SYNCCASH::PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	int    ok = 1;
	int    is_format = 0;
	SString temp_buf;
	SlipDocCommonParam  sdc_param;
	THROW_INVARG(pPack);
	THROW(Connect());
	Arr_In.Z();
	THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
	THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, pPack->Rec.Code));
	// @v11.2.3 {
	{
		LDATETIME ccts;
		temp_buf.Z().Cat(ccts.Set(pPack->Rec.Dt, pPack->Rec.Tm), DATF_ISO8601CENT, 0);
		THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTIMESTAMP, temp_buf));
	}
	// } @v11.2.3 
	THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
	if(P_SlipFmt) {
		int   r = 0;
		SString line_buf;
		const char * p_format_name = isempty(pFormatName) ? ((flags & PRNCHK_RETURN) ? "CCheckRetCopy" : "CCheckCopy") : pFormatName;
		SlipLineParam  sl_param;
		THROW(r = P_SlipFmt->Init(p_format_name, &sdc_param));
		if(r > 0) {
			is_format = 1;
			if(sdc_param.PageWidth > (uint)CheckStrLen)
				WriteLogFile_PageWidthOver(p_format_name);
			RibbonParam = 0;
			Arr_In.Z();
			CheckForRibbonUsing(sdc_param.RegTo, Arr_In);
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, sdc_param.Title));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				Arr_In.Z();
				CheckForRibbonUsing(sl_param.Flags, Arr_In);
				THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, (sl_param.Font == 1) ? DEF_FONTSIZE : sl_param.Font));
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf.Trim((sl_param.Font > 1) ? CheckStrLen / 2 : CheckStrLen)));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
		}
	}
	if(!is_format) {
		uint    pos;
		SString prn_str;
		SString word_buf;
		CCheckLineTbl::Rec ccl;
		Arr_In.Z();
		PPLoadText(PPTXT_CCFMT_CHKCOPY, word_buf); // КОПИЯ ЧЕКА
		word_buf.Transf(CTRANSF_INNER_TO_OUTER);
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, word_buf));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		Arr_In.Z();
		PPLoadText((flags & PRNCHK_RETURN) ? PPTXT_CCFMT_RETURN : PPTXT_CCFMT_SALE, word_buf); // "ВОЗВРАТ ПРОДАЖИ" : "ПРОДАЖА"
		word_buf.Transf(CTRANSF_INNER_TO_OUTER);
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, word_buf));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		for(pos = 0; pPack->EnumLines(&pos, &ccl);) {
			const double price = intmnytodbl(ccl.Price) - ccl.Dscnt;
			const double qtty  = R3(fabs(ccl.Quantity));
			GetGoodsName(ccl.GoodsID, prn_str);
			CutLongTail(prn_str.Transf(CTRANSF_INNER_TO_OUTER));
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			if(qtty != 1.0) {
				temp_buf.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatDiv('X', 1).Cat(price, SFMT_MONEY);
				Arr_In.Z();
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN(' ', CheckStrLen - temp_buf.Len()).Cat(temp_buf)));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
			temp_buf.Z().CatEq(0, qtty * price, SFMT_MONEY);
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN(' ', CheckStrLen - temp_buf.Len()).Cat(temp_buf)));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		}
		THROW(PrintDiscountInfo(pPack, flags));
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN('=', CheckStrLen)));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		temp_buf.Z().CatEq(0, fabs(MONEYTOLDBL(pPack->Rec.Amount)), SFMT_MONEY);
		PPLoadText(PPTXT_CCFMT_TOTAL, word_buf); // ИТОГ
		word_buf.Transf(CTRANSF_INNER_TO_OUTER);
		prn_str = word_buf; // @cstr #12
		prn_str.CatCharN(' ', CheckStrLen / 2 - prn_str.Len() - temp_buf.Len()).Cat(temp_buf);
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
	}
	THROW(LineFeed(6, TRUE, FALSE));
	Arr_In.Z();
	THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out));
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
		}
	ENDCATCH
	return ok;
}

int SCS_SYNCCASH::LineFeed(int lineCount, int useReceiptRibbon, int useJournalRibbon)
{
	int    ok = 1;
	int    cur_receipt = 0;
	int    cur_journal = 0;
	SString temp_buf;
	Arr_In.Z();
	THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, 0));
	THROW(ExecPrintOper(DVCCMD_GETCHECKPARAM, Arr_In, Arr_Out));
	if(Arr_Out.getCount()) {
		SString param_name;
		SString param_val;
		for(uint i = 0; Arr_Out.GetText(i, temp_buf) > 0; i++) {
			DestrStr(temp_buf, param_name, param_val);
			if(param_name.IsEqiAscii("RIBBONPARAM") && param_val.ToLong() == 0)
				cur_receipt = 1;
			else if(param_name.IsEqiAscii("RIBBONPARAM") && param_val.ToLong() == 1)
				cur_journal = 1;
		}
	}
	for(uint i = 0; i < (uint)lineCount; i++) {
		Arr_In.Z();
		if(cur_receipt != useReceiptRibbon)
			THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, 0));
		if(cur_journal != useJournalRibbon)
			THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, 1));
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, temp_buf.Z().CatCharN(' ', CheckStrLen)));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
	}
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::GetCheckInfo(const PPBillPacket * pPack, Sync_BillTaxArray * pAry, long * pFlags, SString & rName)
{
	int    ok = 1;
	long   flags = 0;
	Sync_BillTaxEntry bt_entry;
	const  PPID main_org_id = GetMainOrgID();
	/* @v12.2.4 
	int    wovatax = 0;
	if(GetMainOrgID(&main_org_id)) {
		PersonTbl::Rec prec;
		if(SearchObject(PPOBJ_PERSON, main_org_id, &prec) > 0 && prec.Flags & PSNF_NOVATAX)
			wovatax = 1;
	}*/
	THROW_INVARG(pPack && pAry);
	RVALUEPTR(flags, pFlags);
	if(pPack->OpTypeID == PPOPT_ACCTURN) {
		long   s_tax = 0;
		double amt1 = 0.0;
		double amt2 = 0.0;
		PPObjAmountType amtt_obj;
		TaxAmountIDs    tais;
		double sum = BR2(pPack->Rec.Amount);
		if(sum < 0.0)
			flags |= PRNCHK_RETURN;
		sum = fabs(sum);
		amtt_obj.GetTaxAmountIDs(tais, 1);
		if(tais.STaxAmtID)
			s_tax = tais.STaxRate;
		if(tais.VatAmtID[0])
			amt1 = fabs(pPack->Amounts.Get(tais.VatAmtID[0], 0L));
		if(tais.VatAmtID[1])
			amt2 = fabs(pPack->Amounts.Get(tais.VatAmtID[1], 0L));
		bt_entry.VAT = (amt1 || amt2) ? ((amt1 > amt2) ? tais.VatRate[0] : tais.VatRate[1]) : 0;
		bt_entry.SalesTax = s_tax;
		bt_entry.Amount   = sum;
		THROW(pAry->Add(&bt_entry));
	}
	else {
		PPTransferItem * ti;
		PPObjGoods  goods_obj;
		if(pPack->OpTypeID == PPOPT_GOODSRETURN) {
			PPOprKind op_rec;
			if(GetOpData(pPack->Rec.OpID, &op_rec) > 0 && IsExpendOp(op_rec.LinkOpID) > 0)
				flags |= PRNCHK_RETURN;
		}
		else if(pPack->OpTypeID == PPOPT_GOODSRECEIPT)
			flags |= PRNCHK_RETURN;
		for(uint i = 0; pPack->EnumTItems(&i, &ti);) {
			int re;
			PPGoodsTaxEntry gtx;
			THROW(goods_obj.FetchTaxEntry2(ti->GoodsID, 0/*lotID*/, main_org_id/*taxPayerID*/, pPack->Rec.Dt, pPack->Rec.OpID, &gtx));
			// @v12.2.4 bt_entry.VAT = wovatax ? 0 : gtx.VAT;
			bt_entry.VAT = gtx.VAT; // @v12.2.4
			re = (ti->Flags & PPTFR_RMVEXCISE) ? 1 : 0;
			bt_entry.SalesTax = ((CConfig.Flags & CCFLG_PRICEWOEXCISE) ? !re : re) ? 0 : gtx.SalesTax;
			bt_entry.Amount   = ti->CalcAmount();
			THROW(pAry->Add(&bt_entry));
		}
	}
	if(pPack->Rec.Object)
		GetArticleName(pPack->Rec.Object, rName);
	CATCHZOK
	ASSIGN_PTR(pFlags, flags);
	return ok;
}

int SCS_SYNCCASH::PrintSlipDoc(const CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	int    ok = -1;
	SString temp_buf;
	THROW_INVARG(pPack);
	THROW(Connect());
	if(P_SlipFmt) {
		int   r = 1;
		SString line_buf;
		const char * p_format_name = isempty(pFormatName) ? "SlipDocument" : pFormatName;
		StringSet head_lines(reinterpret_cast<const char *>(&r));
		SlipDocCommonParam  sdc_param;
		THROW(r = P_SlipFmt->Init(p_format_name, &sdc_param));
		if(r > 0) {
			int   str_num, print_head_lines = 0, fill_head_lines = 1;
			SlipLineParam  sl_param;
			Flags |= sfPrintSlip;
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
			THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
			Arr_In.Z();
			THROW(ExecPrintOper(DVCCMD_CLEARSLIPBUF, Arr_In, Arr_Out));
			for(str_num = 0, P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				if(print_head_lines) {
					for(uint i = 0; head_lines.get(&i, temp_buf);) {
						Arr_In.Z();
						THROW(ArrAdd(Arr_In, DVCPARAM_STRNUM, ++str_num));
						THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, temp_buf));
						THROW(ExecPrintOper(DVCCMD_FILLSLIPBUF, Arr_In, Arr_Out));
					}
					print_head_lines = 0;
				}
				else if(fill_head_lines) {
					if(str_num < (int)sdc_param.HeadLines)
						head_lines.add(line_buf);
					else
						fill_head_lines = 0;
				}
				Arr_In.Z();
				THROW(ArrAdd(Arr_In, DVCPARAM_STRNUM, ++str_num));
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf));
				THROW(ExecPrintOper(DVCCMD_FILLSLIPBUF, Arr_In, Arr_Out));
				str_num++;
				if(str_num == sdc_param.PageLength) {
					Arr_In.Z();
					THROW(ExecPrintOper(DVCCMD_PRINTSLIPDOC, Arr_In, Arr_Out));
					print_head_lines = 1;
					str_num = 0;
				}
			}
			if(str_num) {
				Arr_In.Z();
				THROW(ExecPrintOper(DVCCMD_PRINTSLIPDOC, Arr_In, Arr_Out));
			}
			THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out));
			ok = 1;
		}
	}
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
		}
	ENDCATCH
	Flags &= ~sfPrintSlip;
	return ok;
}

int SCS_SYNCCASH::PrintZReportCopy(const CSessInfo * pInfo)
{
	int    ok = -1;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	THROW_INVARG(pInfo);
	THROW(Connect());
	Arr_In.Z();
	THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
	THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, pInfo->Rec.SessNumber));
	THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
	if(P_SlipFmt) {
		int   r = 0;
		SString  line_buf;
		const SString format_name("ZReportCopy");
		SlipDocCommonParam  sdc_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			SlipLineParam  sl_param;
			if(sdc_param.PageWidth > static_cast<uint>(CheckStrLen))
				WriteLogFile_PageWidthOver(format_name);
			RibbonParam = 0;
			Arr_In.Z();
			CheckForRibbonUsing(sdc_param.RegTo, Arr_In);
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, sdc_param.Title));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			for(P_SlipFmt->InitIteration(pInfo); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				Arr_In.Z();
				CheckForRibbonUsing(sl_param.Flags, Arr_In);
				THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, (sl_param.Font == 1) ? DEF_FONTSIZE : sl_param.Font));
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf.Trim((sl_param.Font > 1) ? CheckStrLen / 2 : CheckStrLen)));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
			THROW(LineFeed(6, TRUE, FALSE));
		}
	}
	Arr_In.Z();
	THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out));
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
		}
	ENDCATCH
	return ok;
}

int SCS_SYNCCASH::PrintIncasso(double sum, int isIncome)
{
	ResCode = RESCODE_NO_ERROR;
	int    ok = 1;
	THROW(Connect());
	Flags |= sfCheckOpened;
	if(isIncome) {
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, DEPOSITCHECK));
		THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
		//THROW(LineFeed(6, TRUE, FALSE)); // @vmiller Ибо это не сервисный документ, поэтому нельзя тут печатать текст
	}
	else {
		int    is_cash = 0;
		THROW(is_cash = CheckForCash(sum));
		THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, INCASSCHECK));
		THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
		//THROW(LineFeed(6, TRUE, FALSE)); // @vmiller Ибо это не сервисный документ, поэтому нельзя тут печатать текст
	}
	Arr_In.Z();
	THROW(ArrAdd(Arr_In, DVCPARAM_AMOUNT, sum));
	THROW(ExecPrintOper(DVCCMD_INCASHMENT, Arr_In, Arr_Out));
	Arr_In.Z();
	THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out));
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			ok = 0;
		}
	ENDCATCH
	Flags &= ~sfCheckOpened;
	return ok;
}

int SCS_SYNCCASH::OpenBox()
{
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	int    ok = -1;
	THROW(Connect());
	THROW(ArrAdd(Arr_In.Z(), DVCPARAM_DRAWERNUM, DEF_DRAWER_NUMBER));
	THROW(ExecPrintOper(DVCCMD_OPENBOX, Arr_In, Arr_Out));
	ok = 1;
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				ErrCode = (Flags & sfCheckOpened) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			if(Flags & sfCheckOpened)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = PPErrorZ();
		}
	ENDCATCH
	return ok;
}

int SCS_SYNCCASH::GetPrintErrCode()
{
	return ErrCode;
}

int SCS_SYNCCASH::GetStatus(int & rStatus)
{
	rStatus = 0;
	int    ok = 1;
	StrAssocArray arr_in, arr_out;
	THROW(ExecOper(DVCCMD_GETECRSTATUS, arr_in, arr_out));
	//THROW(ResCode == RESCODE_NO_ERROR);
	if(arr_out.getCount()) {
		SString param_name, param_val, buf;
		for(uint i = 0; arr_out.GetText(i, buf) > 0; i++) {
			DestrStr(buf, param_name, param_val);
			if(param_name.IsEqiAscii("STATUS"))
				rStatus = param_val.ToLong();
		}
	}
	CATCHZOK;
	return ok;
}

int SCS_SYNCCASH::AllowPrintOper()
{
	int    ok = 1;
	int    wait_prn_err = 0;
	int    status = 0;
	StrAssocArray arr_in, arr_out;
	// SetErrorMessage();
	// Ожидание окончания операции печати
	uint   wait_cycle_count = 0;
	do {
		GetStatus(status);
		wait_prn_err = 1;
		wait_cycle_count++;
	} while(status & PRNMODE_PRINT);
	//
	// Если нет чековой ленты
	//
	// @v10.1.0 (избыточная команда - выше была уже вызвана) GetStatus(status);
	if(status & NO_PAPER) {
		if(status & FRMODE_OPEN_CHECK)
			Flags |= sfCheckOpened;
		while(status & NO_PAPER) {
			int  send_msg = 0, r;
			PPSetError(PPERR_SYNCCASH_NO_CHK_RBN);
			send_msg = 1;
			r = PPError();
			if((!send_msg && r != cmOK) || (send_msg &&	PPMessage(mfConf|mfYesNo, PPCFM_SETPAPERTOPRINT, 0) != cmYes)) {
				Flags |= sfCancelled;
				ok = 0;
			}
			GetStatus(status);
		}
		wait_prn_err = 1;
		ResCode = RESCODE_NO_ERROR;
	}
	//
	// Проверяем, надо ли завершить печать после заправки ленты
	//
	if(status & PRNMODE_AFTER_NO_PAPER) {
		THROW(ExecPrintOper(DVCCMD_CONTINUEPRINT, arr_in.Z(), arr_out));
		// @v10.1.0 (избыточная команда - ниже будет вызвана с гарантией) GetStatus(status);
		wait_prn_err = 1;
	}
	//
	// Дополнительный запрос, так как не всем ККМ требуется команда на продолжение печати, а ждать завершения печати надо
	//
	do {
		GetStatus(status);
	} while(status & PRNMODE_PRINT);
	//
	// Ошибки ЭКЛЗ
	//
	// @v9.6.9 (избыточная команда - выше как минимум один вызов был) GetStatus(status);
	if(ResCode == RESCODE_GOTOCTO) {
		SetErrorMessage();
		SString  err_msg(DS.GetConstTLA().AddedMsgString), added_msg;
		if(PPLoadText(PPTXT_APPEAL_CTO, added_msg))
			err_msg.CR().Cat("\003").Cat(added_msg);
		PPSetAddedMsgString(err_msg);
		PPError();
		ok = -1;
	}
	THROW_PP(ResCode != RESCODE_MEMOVERFLOW, PPERR_SYNCCASH_OVERFLOW);
	//
	// Если ситуация не связана непосредственно с процессом печати, выдаем сообщение об ошибке
	// @v5.9.2 При закрытии чека - сумма оплаты меньше суммы чека - не связано с процессом печати, но wait_prn_err == 1
	//
	if(!wait_prn_err || (ResCode == RESCODE_INVEKLZSTATE)) {
		SetErrorMessage();
		PPError();
		Flags |= sfCancelled;
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::ExecPrintOper(int cmd, const StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	int    r = 0;
	SString temp_buf;
	do {
		THROW(ok = P_AbstrDvc->RunCmd__(cmd, rIn, rOut));
		if(ok == -1) {
			THROW(rOut.GetText(0, temp_buf));
			ResCode = temp_buf.ToLong();
			ok = 0;
		}
		if(ResCode == RESCODE_UNKNOWNCOMMAND || (Flags & sfPrintSlip && ResCode == RESCODE_SLIP_IS_EMPTY)) {
			ok = 0;
			break;
		}
		if(Flags & sfSkipAfVerif)
			r = 1;
		else
			r = oneof2(cmd, DVCCMD_CLOSECHECK, DVCCMD_GETCHECKPARAM) ? 1 : 1/*AllowPrintOper()*/; // @v10.1.2
		//
		// Если выдана ошибка, не описанная в протоколе, то выходим для получения текста ошибки
		//
		/*
		if((ResCode != RESCODE_NO_ERROR) && (ResCode != RESCODE_UNKNOWNCOMMAND) && (ResCode != RESCODE_NO_CONNECTION) && (ResCode != RESCODE_SLIP_IS_EMPTY) &&
			(ResCode != RESCODE_INVEKLZSTATE) && (ResCode != RESCODE_MEMOVERFLOW) && (ResCode != RESCODE_GOTOCTO)) {
		*/
		if(!oneof7(ResCode, RESCODE_NO_ERROR, RESCODE_UNKNOWNCOMMAND, RESCODE_NO_CONNECTION, RESCODE_SLIP_IS_EMPTY, RESCODE_INVEKLZSTATE, RESCODE_MEMOVERFLOW, RESCODE_GOTOCTO)) {
			ok = 0;
			break;
		}
	} while((ok != 1) && (r > 0));
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::ExecOper(int cmd, const StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	THROW(ok = P_AbstrDvc->RunCmd__(cmd, rIn, rOut));
	if(ok == -1) {
		SString & r_buf = SLS.AcquireRvlStr();
		THROW(rOut.GetText(0, r_buf));
		ResCode = r_buf.ToLong();
		ok = 0;
	}
	else if(ok == 1 && ResCode == RESCODE_NO_CONNECTION) { // При подборе скорости обмена порта в прошлый раз мог вернуться этот код ошибки и
		// он сохранялся, даже когда соединения успешно устанавливалось
		// Теперь же сокрость подобрана и соединение установлено, а значит и нет теперь ошибки
		ResCode = RESCODE_NO_ERROR;
	}
	CATCHZOK
	return ok;
}

int SCS_SYNCCASH::SetLogotype()
{
	int    ok = 1;
	SString str;
	SFile  file;
	int64  fsize = 0;
	SImageBuffer img_buf;
	SlipDocCommonParam sdc_param;
	SlipLineParam sl_param;
	CCheckPacket p_pack;
	THROW(P_SlipFmt->Init("CCheck", &sdc_param));
	P_SlipFmt->InitIteration(&p_pack);
	P_SlipFmt->NextIteration(str, &sl_param);
	if(sl_param.PictPath.IsEmpty()) {
		PrintLogo = 0;
	}
	else {
		PrintLogo = 1;
		THROW(img_buf.Load(sl_param.PictPath));
		uint height = img_buf.GetHeight();
		uint width = img_buf.GetWidth();
		THROW(file.Open(sl_param.PictPath, mRead|mBinary));
		if(file.IsValid()) {
			file.CalcSize(&fsize);
			file.Close();
		}
		THROW(ArrAdd(Arr_In, DVCPARAM_LOGOSIZE, (int)fsize));
		THROW(ArrAdd(Arr_In, DVCPARAM_LOGOTYPE, sl_param.PictPath));
		THROW(ArrAdd(Arr_In, DVCPARAM_LOGOWIDTH, (int)width));
		THROW(ArrAdd(Arr_In, DVCPARAM_LOGOHEIGHT, (int)height));
		THROW(ExecOper(DVCCMD_SETLOGOTYPE, Arr_In, Arr_Out));
		IsLogoSet = true;
	}
	CATCHZOK;
    return ok;
}

int SCS_SYNCCASH::PrintBnkTermReport(const char * pZCheck)
{
	int    ok = 1;
	size_t zc_len = sstrlen(pZCheck);
	if(zc_len) {
		const char * p_delim = 0;
		switch(SDetermineEOLFormat(pZCheck, zc_len)) {
			case eolWindows: p_delim = "\xD\xA"; break;
			case eolUnix: p_delim = "\xA"; break;
			case eolMac: p_delim = "\xD"; break;
			default: p_delim = "\n"; break;
		}
		SString str;
		//StringSet str_set('\n', pZCheck);
		StringSet str_set(p_delim);
		str_set.setBuf(pZCheck, zc_len+1);
		Arr_In.Z();
		Arr_Out.Z();
		THROW(Connect());
		{
			//SlipDocCommonParam sdc_param;
			//THROW(P_SlipFmt->Init("CCheck", &sdc_param));
			THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
			THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, 0));
			THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
			for(uint pos = 0; str_set.get(&pos, str);) {
				str.Chomp();
				Arr_In.Z();
				// 0xDF^^
				// ~0xDA^^
				// ~0xDE^^
				if(str.HasPrefixIAscii("0xDF^^")) {
					if(!(Flags & sfDontUseCutter)) {
						THROW(ExecPrintOper(DVCCMD_CUT, Arr_In, Arr_Out.Z()));
					}
					else
						SDelay(1000);
					//str.Z().CatCharN('-', 8);
					//THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, CHECKRIBBON));
					//THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, DEF_FONTSIZE));
					//THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, str));
					//THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out.Z()));
				}
				else if(str.HasPrefixIAscii("~0xDA^^")) {
					if(!(Flags & sfDontUseCutter)) {
						THROW(ExecPrintOper(DVCCMD_CUT, Arr_In, Arr_Out.Z()));
					}
					else
						SDelay(1000);
					//str.Z().CatCharN('-', 8);
					//THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, CHECKRIBBON));
					//THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, DEF_FONTSIZE));
					//THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, str));
					//THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out.Z()));
				}
				else if(str.HasPrefixIAscii("~0xDE^^")) {
					if(!(Flags & sfDontUseCutter)) {
						THROW(ExecPrintOper(DVCCMD_CUT, Arr_In, Arr_Out.Z()));
					}
					else
						SDelay(1000);
					//str.Z().CatCharN('-', 8);
					//THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, CHECKRIBBON));
					//THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, DEF_FONTSIZE));
					//THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, str));
					//THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out.Z()));
				}
				else {
					// @v11.0.9 {
					if(str.IsEmpty()) {
						str.Space();
					}
					// } @v11.0.9
					THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, CHECKRIBBON));
					THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, MIN_FONTSIZE)); //@SevaSob @v11.4.1
					THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, str));
					THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out.Z()));
				}
			}
			Arr_In.Z();
			THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out.Z()));
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_DBINFO|LOGMSGF_COMP);
		ok = 0;
	ENDCATCH
	return ok;
}
