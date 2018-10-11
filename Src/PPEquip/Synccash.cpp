// SYNCCASH.CPP
// @codepage windows-1251
// Интерфейс (синхронный) для ККМ
//
#include <pp.h>
#pragma hdrstop
#include <comdisp.h>

#define DEF_STRLEN             36   // Длина строки
#define DEF_DRAWER_NUMBER		0	// Номер денежного ящика
#define DEF_FONTSIZE			3	// Средний размер шрифта

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
	SLAPI  Sync_BillTaxEntry() : VAT(0), SalesTax(0), Amount(0.0)
	{
	}
	long   VAT;      // prec 0.01
	long   SalesTax; // prec 0.01
	double Amount;
};

class Sync_BillTaxArray : public SVector { // @v9.8.4 SArray-->SVector
public:
	SLAPI  Sync_BillTaxArray() : SVector(sizeof(Sync_BillTaxEntry))
	{
	}
	int    SLAPI Search(long VAT, long salesTax, uint * p = 0);
	int    SLAPI Insert(Sync_BillTaxEntry * e, uint * p = 0);
	int    SLAPI Add(Sync_BillTaxEntry * e);
	Sync_BillTaxEntry & SLAPI  at(uint p);
};

IMPL_CMPFUNC(Sync_BillTaxEnKey, i1, i2) { RET_CMPCASCADE2((const Sync_BillTaxEntry*)i1, (const Sync_BillTaxEntry*)i2, VAT, SalesTax); }

int SLAPI Sync_BillTaxArray::Search(long VAT, long salesTax, uint * p)
{
	Sync_BillTaxEntry bte;
	bte.VAT      = VAT;
	bte.SalesTax = salesTax;
	return bsearch(&bte, p, PTR_CMPFUNC(Sync_BillTaxEnKey));
}

int SLAPI Sync_BillTaxArray::Insert(Sync_BillTaxEntry * e, uint * p)
	{ return ordInsert(e, p, PTR_CMPFUNC(Sync_BillTaxEnKey)) ? 1 : PPSetErrorSLib(); }
Sync_BillTaxEntry & SLAPI Sync_BillTaxArray::at(uint p)
	{ return *(Sync_BillTaxEntry*)SVector::at(p); } // @v9.8.4 SArray-->SVector

int SLAPI Sync_BillTaxArray::Add(Sync_BillTaxEntry * e)
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
	SLAPI  SCS_SYNCCASH(PPID n, char * name, char * port);
	SLAPI ~SCS_SYNCCASH();
	virtual int SLAPI PrintCheck(CCheckPacket *, uint flags);
	virtual int SLAPI PrintFiscalCorrection(const PPCashMachine::FiscalCorrection * pFc);
	// @v10.0.0 virtual int SLAPI PrintCheckByBill(const PPBillPacket * pPack, double multiplier, int departN);
	virtual int SLAPI PrintCheckCopy(CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int SLAPI PrintSlipDoc(CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int SLAPI GetSummator(double * val);
	virtual int SLAPI AddSummator(double add) { return 1; }
	virtual int SLAPI CloseSession(PPID sessID);
	virtual int SLAPI PrintXReport(const CSessInfo *);
	virtual int SLAPI PrintZReportCopy(const CSessInfo *);
	virtual int SLAPI PrintIncasso(double sum, int isIncome);
	virtual int SLAPI GetPrintErrCode();
	virtual int SLAPI OpenBox();
	virtual int SLAPI CheckForSessionOver();
	virtual int SLAPI PrintBnkTermReport(const char * pZCheck);

	// PPCashNode NodeRec;
	PPAbstractDevice * P_AbstrDvc;
	StrAssocArray Arr_In;
	StrAssocArray Arr_Out;
private:
	virtual int SLAPI InitChannel() { return 1; }
	int    SLAPI Connect(int forceKeepAlive = 0);
	int    SLAPI AnnulateCheck();
	int    SLAPI CheckForCash(double sum);
	//int  SLAPI CheckForEKLZOrFMOverflow() { return 1; }
	int    SLAPI PrintReport(int withCleaning);
	int	   SLAPI PrintDiscountInfo(CCheckPacket * pPack, uint flags);
	int    SLAPI GetCheckInfo(const PPBillPacket * pPack, Sync_BillTaxArray * pAry, long * pFlags, SString & rName);
	int    SLAPI AllowPrintOper();
	void   SLAPI SetErrorMessage();
	//void SLAPI WriteLogFile(PPID id);
	void   SLAPI CutLongTail(char * pBuf);
	void   SLAPI CutLongTail(SString & rBuf);
	int    SLAPI LineFeed(int lineCount, int useReceiptRibbon, int useJournalRibbon);
	int    SLAPI CheckForRibbonUsing(uint ribbonParam, StrAssocArray & rOut); // ribbonParam == SlipLineParam::RegTo
	int    SLAPI ExecPrintOper(int cmd, StrAssocArray & rIn, StrAssocArray & rOut);
	int    SLAPI ExecOper(int cmd, StrAssocArray & rIn, StrAssocArray & rOut);
	int    GetStatus(int & rStatus);
	int    SetLogotype();
	int    GetPort(const char * pPortName, int * pPortNo);

	enum DevFlags {
		sfConnected     = 0x0001, // установлена связь с ККМ, COM-порт занят
		sfOpenCheck     = 0x0002, // чек открыт
		sfCancelled     = 0x0004, // операция печати чека прервана пользователем
		sfPrintSlip     = 0x0010, // печать подкладного документа
		sfNotUseCutter  = 0x0020, // не использовать отрезчик чеков
		sfUseWghtSensor = 0x0040, // использовать весовой датчик
		sfKeepAlive     = 0x0080  // @v10.0.12 Держать установленное соединение с аппаратом
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
	uint   Inited;          // Если равен 0, то устройство не инициализировано, 1 - инициализировано
	int	   PrintLogo;       // Печатать логотип
	int	   IsSetLogo;       // 0 - логотип не загружен, 1 - логотип загружен
	SString AdmName;        // Имя сист.администратора
};

int  SCS_SYNCCASH::RefToIntrf = 0;

class CM_SYNCCASH : public PPCashMachine {
public:
	SLAPI CM_SYNCCASH(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPSyncCashSession * SLAPI SyncInterface();
};

PPSyncCashSession * SLAPI CM_SYNCCASH::SyncInterface()
{
	PPSyncCashSession * cs = 0;
	if(IsValid()) {
		cs = (PPSyncCashSession *)new SCS_SYNCCASH(NodeID, NodeRec.Name, NodeRec.Port);
		CALLPTRMEMB(cs, Init(NodeRec.Name, NodeRec.Port));
	}
	return cs;
}

REGISTER_CMT(SYNCCASH,1,0);

static void SLAPI WriteLogFile_PageWidthOver(const char * pFormatName)
{
	SString msg_fmt, msg;
	msg.Printf(PPLoadTextS(PPTXT_SLIPFMT_WIDTHOVER, msg_fmt), pFormatName);
	PPLogMessage(PPFILNAM_SHTRIH_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
}

int SCS_SYNCCASH::GetPort(const char * pPortName, int * pPortNo)
{
	int    ok = 0, port = 0;
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

SLAPI SCS_SYNCCASH::SCS_SYNCCASH(PPID n, char * name, char * port) : PPSyncCashSession(n, name, port), Port(0), CashierPassword(0),
	AdmPassword(0), ResCode(RESCODE_NO_ERROR), ErrCode(SYNCPRN_NO_ERROR), CheckStrLen(DEF_STRLEN), Flags(/*sfKeepAlive*/0),
	RibbonParam(0), Inited(0), IsSetLogo(0), PrintLogo(0)
{
	if(SCn.Flags & CASHF_NOTUSECHECKCUTTER)
		Flags |= sfNotUseCutter;
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

SLAPI SCS_SYNCCASH::~SCS_SYNCCASH()
{
	if(Flags & sfConnected) {
		ExecOper(DVCCMD_DISCONNECT, Arr_In, Arr_Out);
		ExecOper(DVCCMD_RELEASE, Arr_In, Arr_Out);
		Inited = 0;
	}
	ZDELETE(P_AbstrDvc);
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, int val)
{
	SString & r_str = SLS.AcquireRvlStr(); // @v9.9.4 SLS.AcquireRvlStr
	return rArr.Add(pos, r_str.Z().Cat(val), 1) ? 1 : PPSetErrorSLib();
}

static int FASTCALL ArrAdd(StrAssocArray & rArr, int pos, double val)
{
	SString & r_str = SLS.AcquireRvlStr(); // @v9.9.4 SLS.AcquireRvlStr
	return rArr.Add(pos, r_str.Z().Cat(val), 1) ? 1 : PPSetErrorSLib();
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

int SLAPI SCS_SYNCCASH::Connect(int forceKeepAlive/*= 0*/)
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
				// Определяем имя кассира
				if(PPObjPerson::GetCurUserPerson(0, &temp_buf) < 0) {
					PPObjSecur sec_obj(PPOBJ_USR, 0);
					PPSecur sec_rec;
					if(sec_obj.Fetch(LConfig.User, &sec_rec) > 0)
						temp_buf = sec_rec.Name;
				}
				THROW(ArrAdd(Arr_In, DVCPARAM_CSHRNAME, temp_buf));
				THROW(ArrAdd(Arr_In, DVCPARAM_ADMINNAME, AdmName.NotEmpty() ? AdmName : temp_buf)); // Имя администратора
				THROW(ArrAdd(Arr_In, DVCPARAM_SESSIONID, /*pIn->*/SCn.CurSessID)); // Текущая кассовая сесси
			}
			THROW(ExecOper(DVCCMD_SETCFG, Arr_In, Arr_Out));
			// Загружаем логотип
			Arr_In.Z();
			if(!IsSetLogo)
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

int  SLAPI SCS_SYNCCASH::AnnulateCheck()
{
	int    ok = -1;
	int    status = 0;
	GetStatus(status);
	// Проверка на незавершенную печать
	if(status & PRNMODE_AFTER_NO_PAPER) {
		Flags |= sfOpenCheck;
		THROW(ExecPrintOper(DVCCMD_CONTINUEPRINT, Arr_In.Z(), Arr_Out));
		do {
			GetStatus(status);
		} while(status & PRNMODE_PRINT);
		Flags &= ~sfOpenCheck;
	}
	// Проверка на наличие открытого чека, который надо аннулировать
	// @v10.1.0 GetStatus(status);
	if(status & FRMODE_OPEN_CHECK) {
		Flags |= (sfOpenCheck | sfCancelled);
		PPMessage(mfInfo|mfOK, PPINF_SHTRIHFR_CHK_ANNUL, 0);
		THROW(ExecPrintOper(DVCCMD_ANNULATE, Arr_In.Z(), Arr_Out));
		Flags &= ~(sfOpenCheck | sfCancelled);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI SCS_SYNCCASH::PrintFiscalCorrection(const PPCashMachine::FiscalCorrection * pFc)
{
	int    ok = 1;
	SString temp_buf;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	Flags |= sfOpenCheck;
	Arr_In.Z();
	/*
	struct FiscalCorrection {
		SLAPI  FiscalCorrection();
		enum {
			fIncome    = 0x0001, // Приход денег (отрицательная коррекция). Если не стоит, то - расход.
			fByPrecept = 0x0002  // Коррекция по предписанию
		};
		double AmtCash;    // @#{>=0} Сумма наличного платежа
		double AmtBank;    // @#{>=0} Сумма электронного платежа
		double AmtPrepay;  // @#{>=0} Сумма предоплатой
		double AmtPostpay; // @#{>=0} Сумма постоплатой
		double AmtVat18;   // Сумма налога по ставке 18%
		double AmtVat10;   // Сумма налога по ставке 10%
		double AmtVat00;   // Сумма расчета по ставке 0%
		double AmtNoVat;   // Сумма расчета без налога
		LDATE  Dt;         // Дата документа основания коррекции
		long   Flags;      // @flags
		SString Code;      // Номер документа основания коррекции
		SString Reason;    // Основание коррекции
		SString Operator;  // Имя оператора
	};
	*/
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
	THROW(ArrAdd(Arr_In, DVCPARAM_DATE, temp_buf.Z().Cat(pFc->Dt, DATF_ISO8601|DATF_CENTURY)));
	THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, pFc->Reason));
	if(pFc->Flags & pFc->fVatFree) {
		THROW(ArrAdd(Arr_In, DVCPARAM_VATFREE, 1));
		THROW(ArrAdd(Arr_In, DVCPARAM_VATFREEAMOUNT, (pFc->AmtCash+pFc->AmtBank+pFc->AmtPrepay+pFc->AmtPostpay)));
	}
	else {
		if(pFc->VatRate > 0) {
			THROW(ArrAdd(Arr_In, DVCPARAM_VATRATE, pFc->Reason));
		}
		else {
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT18, pFc->AmtVat18));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT10, pFc->AmtVat10));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATAMOUNT00, pFc->AmtVat00));
			THROW(ArrAdd(Arr_In, DVCPARAM_VATFREEAMOUNT, pFc->AmtNoVat));
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
	Flags &= ~sfOpenCheck;
	return ok;
}

int SLAPI SCS_SYNCCASH::PrintCheck(CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	int    chk_no = 0;
	int    is_format = 0;
	SString buf;
	SString input;
	SString param_name;
	SString param_val;
	SString temp_buf;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	THROW_INVARG(pPack);
	if(pPack->GetCount() == 0)
		ok = -1;
	else {
		SlipDocCommonParam sdc_param;
		const  int is_vat_free = BIN(CnObj.IsVatFree(NodeID) > 0);
		double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
		double sum = fabs(pPack->_Cash) + 0.001;
		double running_total = 0.0;
		double fiscal = 0.0;
		double nonfiscal = 0.0;
		pPack->HasNonFiscalAmount(&fiscal, &nonfiscal);
		THROW(Connect());
		// @v10.1.0 if(flags & PRNCHK_LASTCHKANNUL) {
			THROW(AnnulateCheck());
		// @v10.1.0 }
		if(flags & PRNCHK_RETURN && !(flags & PRNCHK_BANKING)) {
			int    is_cash;
			THROW(is_cash = CheckForCash(amt));
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
				if(sdc_param.PageWidth > (uint)CheckStrLen)
					WriteLogFile_PageWidthOver(p_format_name);
				RibbonParam = 0;
				Arr_In.Z();
				CheckForRibbonUsing(sdc_param.RegTo, Arr_In);
				if(fiscal != 0.0) {
					PROFILE_START_S("DVCCMD_OPENCHECK")
					THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, (flags & PRNCHK_RETURN) ? RETURNCHECK : SALECHECK));
					THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, pPack->Rec.Code));
					THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
					PROFILE_END
				}
				else {
					PROFILE_START_S("DVCCMD_OPENCHECK")
					//THROW(SetFR(DocumentName, "" /*sdc_param.Title*/));
					//THROW(ExecFRPrintOper(PrintDocumentTitle));
					THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
					THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, pPack->Rec.Code));
					THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
					PROFILE_END
					PROFILE_START_S("DVCCMD_PRINTTEXT")
					Arr_In.Z();
					THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, sdc_param.Title));
					THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
					PROFILE_END
				}
				Flags |= sfOpenCheck;
				for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
					Arr_In.Z();
					if(sl_param.Flags & SlipLineParam::fRegFiscal) {
						CheckForRibbonUsing(SlipLineParam::fRegRegular | SlipLineParam::fRegJournal, Arr_In);
						double _q = sl_param.Qtty;
						double _p = sl_param.Price;
						running_total += (_q * _p);
						PROFILE_START_S("DVCCMD_PRINTFISCAL")
						THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, sl_param.Text)); // @v9.5.7
						THROW(ArrAdd(Arr_In, DVCPARAM_CODE, sl_param.Code)); // @v9.5.7
						THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, _q));
						THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, fabs(_p)));
						THROW(ArrAdd(Arr_In, DVCPARAM_DEPARTMENT, (sl_param.DivID > 16 || sl_param.DivID < 0) ? 0 :  sl_param.DivID));
						if(is_vat_free) {
							THROW(ArrAdd(Arr_In, DVCPARAM_VATFREE, 1)); // @v9.7.1
						}
						else {
							THROW(ArrAdd(Arr_In, DVCPARAM_VATRATE, fabs(sl_param.VatRate))); // @v9.7.1
						}
						THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
						PROFILE_END
						Flags |= sfOpenCheck;
						prn_total_sale = 0;
					}
					else if(sl_param.Kind == sl_param.lkBarcode) {
						;
					}
					else if(sl_param.Kind == sl_param.lkSignBarcode) {
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
							if(sl_param.Flags & sl_param.fBcTextBelow) {
								ArrAdd(Arr_In, DVCPARAM_LABEL, "below");
							}
							else if(sl_param.Flags & sl_param.fBcTextAbove) {
								ArrAdd(Arr_In, DVCPARAM_LABEL, "above");
							}
							THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf));
							THROW(ExecPrintOper(DVCCMD_PRINTBARCODE, Arr_In, Arr_Out));
							PROFILE_END
						}
					}
					else {
						PROFILE(CheckForRibbonUsing(sl_param.Flags, Arr_In));
						PROFILE_START_S("DVCCMD_PRINTTEXT")
						THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, line_buf.Trim((sl_param.Font > 1) ? CheckStrLen / 2 : CheckStrLen)));
						THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, (sl_param.Font == 1) ? DEF_FONTSIZE : sl_param.Font));
						THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
						PROFILE_END
					}
				}
				Arr_In.Z();
				running_total = fabs(running_total); // @v7.7.2
				CheckForRibbonUsing(SlipLineParam::fRegRegular|SlipLineParam::fRegJournal, Arr_In);
				if(prn_total_sale) {
					if(fiscal != 0.0) {
						PROFILE_START_S("DVCCMD_PRINTFISCAL")
						if(!pPack->GetCount()) {
							THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, 1L));
							THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, amt));
							THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
							Flags |= sfOpenCheck;
							running_total += amt;
						}
						else /*if(fiscal != 0.0)*/ {
							THROW(ArrAdd(Arr_In, DVCPARAM_QUANTITY, 1L));
							THROW(ArrAdd(Arr_In, DVCPARAM_PRICE, fiscal));
							THROW(ExecPrintOper(DVCCMD_PRINTFISCAL, Arr_In, Arr_Out));
							Flags |= sfOpenCheck;
							running_total += fiscal;
						}
						PROFILE_END
					}
				}
				else if(running_total != amt) {
					SString fmt_buf, msg_buf, added_buf;
					PPLoadText(PPTXT_SHTRIH_RUNNGTOTALGTAMT, fmt_buf);
					const char * p_sign = (running_total > amt) ? " > " : ((running_total < amt) ? " < " : " ?==? ");
					added_buf.Z().Cat(running_total, MKSFMTD(0, 20, NMBF_NOTRAILZ)).Cat(p_sign).Cat(amt, MKSFMTD(0, 20, NMBF_NOTRAILZ));
					msg_buf.Printf(fmt_buf, added_buf.cptr());
					PPLogMessage(PPFILNAM_SHTRIH_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				}
			}
		}
		if(!is_format) {
			CCheckLineTbl::Rec ccl;
			for(uint pos = 0; pPack->EnumLines(&pos, &ccl) > 0;) {
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
				Flags |= sfOpenCheck;
			}
			// Информация о скидке
			THROW(PrintDiscountInfo(pPack, flags));
			buf.Z().CatCharN('=', CheckStrLen);
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, buf));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		}
		Arr_In.Z();
		{
			const CcAmountList & r_al = pPack->AL_Const();
			const int is_al = BIN(r_al.getCount());
			const double amt_bnk = is_al ? r_al.Get(CCAMTTYP_BANK) : ((pPack->Rec.Flags & CCHKF_BANKING) ? fiscal : 0.0);
			const double amt_cash = (fiscal - amt_bnk);
			if(amt_bnk > 0.0) {
				THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCARD, amt_bnk))
			}
			if(amt_cash > 0.0) {
				THROW(ArrAdd(Arr_In, DVCPARAM_PAYMCASH, amt_cash));
			}
		}
		// Всегда закрываем чек
		//if(fiscal != 0.0) {
			THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out));
		//}
		Flags &= ~sfOpenCheck;
		ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, 0));
		THROW(ExecPrintOper(DVCCMD_GETCHECKPARAM, Arr_In, Arr_Out));
		if(Arr_Out.getCount()) {
			for(uint i = 0; Arr_Out.GetText(i, buf) > 0; i++) {
				DestrStr(buf, param_name, param_val);
				if(param_name.IsEqiAscii("CHECKNUM"))
					pPack->Rec.Code = (int32)param_val.ToInt64();
			}
		}
		ErrCode = SYNCPRN_NO_ERROR;
	}
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			if(ErrCode != SYNCPRN_ERROR_AFTER_PRINT) {
				SString no_print_txt;
				PPLoadText(PPTXT_CHECK_NOT_PRINTED, no_print_txt);
				ErrCode = (Flags & sfOpenCheck) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				PPLogMessage(PPFILNAM_SHTRIH_LOG, CCheckCore::MakeCodeString(&pPack->Rec, no_print_txt), LOGMSGF_TIME|LOGMSGF_USER);
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			if(Flags & sfOpenCheck)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int SLAPI SCS_SYNCCASH::CheckForCash(double sum)
{
	double cash_sum = 0.0;
	return GetSummator(&cash_sum) ? ((cash_sum < sum) ? -1 : 1) : 0;
}

int SLAPI SCS_SYNCCASH::GetSummator(double * val)
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
			if(param_name.CmpNC("CASHAMOUNT") == 0)
				cash_amt = param_val.ToReal();
		}
	}
	CATCH
		ok = (SetErrorMessage(), 0);
	ENDCATCH
	ASSIGN_PTR(val, cash_amt);
	return ok;
}

int	SLAPI SCS_SYNCCASH::PrintDiscountInfo(CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	SString input;
	double amt = R2(fabs(MONEYTOLDBL(pPack->Rec.Amount)));
	double dscnt = R2(MONEYTOLDBL(pPack->Rec.Discount));
	if(flags & PRNCHK_RETURN)
		dscnt = -dscnt;
	if(dscnt > 0.0) {
		double  pcnt = round(dscnt * 100.0 / (amt + dscnt), 1);
		SString prn_str, temp_str;
		SCardCore scc;
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN('-', CheckStrLen)));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		temp_str.Z().Cat(amt + dscnt, SFMT_MONEY);
		prn_str = "СУММА БЕЗ СКИДКИ"; // @cstr #0
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_str.Len()).Cat(temp_str);
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		if(scc.Search(pPack->Rec.SCardID, 0) > 0) {
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, (prn_str = "КАРТА").Space().Cat(scc.data.Code)));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			if(scc.data.PersonID && GetPersonName(scc.data.PersonID, temp_str) > 0) { // @v6.0.9 GetObjectName-->GetPersonName
				(prn_str = "ВЛАДЕЛЕЦ").Space().Cat(temp_str.Transf(CTRANSF_INNER_TO_OUTER)); // @cstr #2
				CutLongTail(prn_str);
				Arr_In.Z();
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
		}
		temp_str.Z().Cat(dscnt, SFMT_MONEY);
		(prn_str = "СКИДКА").Space().Cat(pcnt, MKSFMTD(0, (flags & PRNCHK_ROUNDINT) ? 0 : 1, NMBF_NOTRAILZ)).CatChar('%'); // @cstr #3
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_str.Len()).Cat(temp_str);
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
	}
	CATCHZOK
	return ok;
}

void SLAPI SCS_SYNCCASH::CutLongTail(char * pBuf)
{
	char * p = 0;
	if(pBuf && sstrlen(pBuf) > (uint)CheckStrLen) {
		pBuf[CheckStrLen + 1] = 0;
		if((p = strrchr(pBuf, ' ')) != 0)
			*p = 0;
		else
			pBuf[CheckStrLen] = 0;
	}
}

void SLAPI SCS_SYNCCASH::CutLongTail(SString & rBuf)
{
	char  buf[256];
	rBuf.CopyTo(buf, sizeof(buf));
	CutLongTail(buf);
	rBuf = buf;
}

int SLAPI SCS_SYNCCASH::CloseSession(PPID sessID) { return PrintReport(1); }
int SLAPI SCS_SYNCCASH::PrintXReport(const CSessInfo *) { return PrintReport(0); }

int SLAPI SCS_SYNCCASH::PrintReport(int withCleaning)
{
	int    ok = 1;
	ResCode = RESCODE_NO_ERROR;
	THROW(Connect());
	Flags |= sfOpenCheck;
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
	Flags &= ~sfOpenCheck;
	return ok;
}

void SLAPI SCS_SYNCCASH::SetErrorMessage()
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

int  SLAPI SCS_SYNCCASH::CheckForRibbonUsing(uint ribbonParam, StrAssocArray & rOut)
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

int SLAPI SCS_SYNCCASH::CheckForSessionOver()
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

int SLAPI SCS_SYNCCASH::PrintCheckCopy(CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	int    ok = 1;
	int    is_format = 0;
	SlipDocCommonParam  sdc_param;
	THROW_INVARG(pPack);
	THROW(Connect());
	Arr_In.Z();
	THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
	THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, pPack->Rec.Code));
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
		SString prn_str, temp_str;
		CCheckLineTbl::Rec ccl;
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, "КОПИЯ ЧЕКА"));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, (flags & PRNCHK_RETURN) ? "ВОЗВРАТ ПРОДАЖИ" : "ПРОДАЖА"));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		for(pos = 0; pPack->EnumLines(&pos, &ccl) > 0;) {
			double  price = intmnytodbl(ccl.Price) - ccl.Dscnt;
			double  qtty  = R3(fabs(ccl.Quantity));
			GetGoodsName(ccl.GoodsID, prn_str);
			CutLongTail(prn_str.Transf(CTRANSF_INNER_TO_OUTER));
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			if(qtty != 1.0) {
				temp_str.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatDiv('X', 1).Cat(price, SFMT_MONEY);
				Arr_In.Z();
				THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN(' ', CheckStrLen - temp_str.Len()).Cat(temp_str)));
				THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
			}
			temp_str.Z().CatEq(0, qtty * price, SFMT_MONEY);
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN(' ', CheckStrLen - temp_str.Len()).Cat(temp_str)));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		}
		THROW(PrintDiscountInfo(pPack, flags));
		Arr_In.Z();
		THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, prn_str.Z().CatCharN('=', CheckStrLen)));
		THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out));
		temp_str.Z().CatEq(0, fabs(MONEYTOLDBL(pPack->Rec.Amount)), SFMT_MONEY);
		prn_str = "ИТОГ"; // @cstr #12
		prn_str.CatCharN(' ', CheckStrLen / 2 - prn_str.Len() - temp_str.Len()).Cat(temp_str);
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

int SLAPI SCS_SYNCCASH::LineFeed(int lineCount, int useReceiptRibbon, int useJournalRibbon)
{
	int    ok = 1;
	int    cur_receipt;
	int    cur_journal;
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

int SLAPI SCS_SYNCCASH::GetCheckInfo(const PPBillPacket * pPack, Sync_BillTaxArray * pAry, long * pFlags, SString & rName)
{
	int    ok = 1, wovatax = 0;
	long   flags = 0;
	Sync_BillTaxEntry bt_entry;
	PPID   main_org_id;
	if(GetMainOrgID(&main_org_id)) {
		PersonTbl::Rec prec;
		if(SearchObject(PPOBJ_PERSON, main_org_id, &prec) > 0 && prec.Flags & PSNF_NOVATAX)
			wovatax = 1;
	}
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
		amtt_obj.GetTaxAmountIDs(&tais, 1);
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
			PPGoodsTaxEntry  gt_entry;
			THROW(goods_obj.FetchTax(ti->GoodsID, pPack->Rec.Dt, pPack->Rec.OpID, &gt_entry));
			bt_entry.VAT = wovatax ? 0 : gt_entry.VAT;
			re = (ti->Flags & PPTFR_RMVEXCISE) ? 1 : 0;
			bt_entry.SalesTax = ((CConfig.Flags & CCFLG_PRICEWOEXCISE) ? !re : re) ? 0 : gt_entry.SalesTax;
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

int SLAPI SCS_SYNCCASH::PrintSlipDoc(CCheckPacket * pPack, const char * pFormatName, uint flags)
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
		StringSet head_lines((const char *)&r);
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
				else if(fill_head_lines)
					if(str_num < (int)sdc_param.HeadLines)
						head_lines.add(line_buf);
					else
						fill_head_lines = 0;
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

int SLAPI SCS_SYNCCASH::PrintZReportCopy(const CSessInfo * pInfo)
{
	int  ok = -1;
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
		SString  line_buf, format_name = "ZReportCopy";
		SlipDocCommonParam  sdc_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			SlipLineParam  sl_param;
			if(sdc_param.PageWidth > (uint)CheckStrLen)
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

int SLAPI SCS_SYNCCASH::PrintIncasso(double sum, int isIncome)
{
	ResCode = RESCODE_NO_ERROR;
	int    ok = 1;
	THROW(Connect());
	Flags |= sfOpenCheck;
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
	Flags &= ~sfOpenCheck;
	return ok;
}

int SLAPI SCS_SYNCCASH::OpenBox()
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
				ErrCode = (Flags & sfOpenCheck) ? SYNCPRN_CANCEL_WHILE_PRINT : SYNCPRN_CANCEL;
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			if(Flags & sfOpenCheck)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			ok = PPErrorZ();
		}
	ENDCATCH
	return ok;
}

int SLAPI SCS_SYNCCASH::GetPrintErrCode()
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

int SLAPI SCS_SYNCCASH::AllowPrintOper()
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
			Flags |= sfOpenCheck;
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

int SLAPI SCS_SYNCCASH::ExecPrintOper(int cmd, StrAssocArray & rIn, StrAssocArray & rOut)
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
		// @v10.1.2 r = AllowPrintOper();
		r = oneof2(cmd, DVCCMD_CLOSECHECK, DVCCMD_GETCHECKPARAM) ? 1 : AllowPrintOper(); // @v10.1.2
		//
		// Если выдана ошибка, не описанная в простоколе, то выходим для получения текста ошибки
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

int SLAPI SCS_SYNCCASH::ExecOper(int cmd, StrAssocArray & rIn, StrAssocArray & rOut)
{
	int    ok = 1;
	THROW(ok = P_AbstrDvc->RunCmd__(cmd, rIn, rOut));
	if(ok == -1) {
		SString & r_buf = SLS.AcquireRvlStr();
		THROW(rOut.GetText(0, r_buf));
		ResCode = r_buf.ToLong();
		ok = 0;
	}
	else if((ok == 1) && (ResCode == RESCODE_NO_CONNECTION)) { // При подборе скорости обмена порта в прошлый раз мог вернуться этот код ошибки и
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
	if(sl_param.PictPath.Empty()) {
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
		IsSetLogo = 1;
	}
	CATCHZOK;
    return ok;
}

int SLAPI SCS_SYNCCASH::PrintBnkTermReport(const char * pZCheck)
{
	int    ok = 1;
	SString str;
	StringSet str_set('\n', pZCheck);
	Arr_In.Z();
	Arr_Out.Z();
	THROW(Connect());
	{
		//SlipDocCommonParam sdc_param;
		//THROW(P_SlipFmt->Init("CCheck", &sdc_param));
		THROW(ArrAdd(Arr_In, DVCPARAM_CHECKTYPE, SERVICEDOC));
		THROW(ArrAdd(Arr_In, DVCPARAM_CHECKNUM, 0));
		THROW(ExecPrintOper(DVCCMD_OPENCHECK, Arr_In, Arr_Out));
		for(uint pos = 0; str_set.get(&pos, str) > 0;) {
			Arr_In.Z();
			THROW(ArrAdd(Arr_In, DVCPARAM_RIBBONPARAM, CHECKRIBBON));
			THROW(ArrAdd(Arr_In, DVCPARAM_FONTSIZE, DEF_FONTSIZE));
			THROW(ArrAdd(Arr_In, DVCPARAM_TEXT, str));
			THROW(ExecPrintOper(DVCCMD_PRINTTEXT, Arr_In, Arr_Out.Z()));
		}
		Arr_In.Z();
		THROW(ExecPrintOper(DVCCMD_CLOSECHECK, Arr_In, Arr_Out.Z()));
		// @v10.2.2 {
		{
			Arr_In.Z();
			THROW(ExecPrintOper(DVCCMD_CUT, Arr_In, Arr_Out.Z()));
		}
		// } @v10.2.2
	}
	CATCHZOK;
	return ok;
}
