// ATOL.CPP
// Copyright (c) V.Nasonov 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
// Интерфейс (асинхронный) к драйверу "Атол"
//
#include <pp.h>
#pragma hdrstop
#include <comdisp.h>
//
//   Интерфейс для работы с ATOL_CARD
//
#define AC_MAX_OPER_COUNT   250

class AtolCardIntrf {
public:
	SLAPI  AtolCardIntrf();
	SLAPI ~AtolCardIntrf();
	int    SLAPI InitIntrf();
	int    SLAPI ACStartTransaction();
	int    SLAPI ACEndTransaction();
	int    SLAPI ACCancelTransaction();
	int    SLAPI ACOpenTable(const char * pTableName);
	int    SLAPI ACCloseTable();
	int    SLAPI ACClearTable(const char * pTableName);
	int    SLAPI ACNewRecord();
	int    SLAPI ACPut(int fieldNo, long value);
	int    SLAPI ACPut(int fieldNo, const char * pStr);
	int    SLAPI ACInsert(PPID * pID = 0);
	int    SLAPI ACGetIDsAry(const char * pTableName, int fieldNo, LAssocArray * pAry);
private:
	int    SLAPI CheckForOperCount();
	int    SLAPI ExecAtolOper(PPID id);
	int    SLAPI SetErrorMessage();
	enum {
		ResultCode,
		ResultDescription,
		Active,
		TableName,
		OpenTable,
		RefreshTable,
		CloseTable,
		FieldIndex,
		FieldValue,
		FieldValueAsString,
		RecordCount,
		RecordNumber,
		BeginTransaction,
		ApplyTransaction,
		CancelTransaction,
		AddRecord,
		EditRecord,
		PostRecord,
		DeleteRecord,
	};
	ComDispInterface * P_DispIntrf;
	int    ResCode;
	int    IsStartedTransaction;
	int    IsOpenedTable;
	int    OperCount;
};

SLAPI AtolCardIntrf::AtolCardIntrf()
{
	P_DispIntrf   = 0;
	ResCode       = 0;
	IsStartedTransaction = 0;
	IsOpenedTable = 0;
	OperCount     = 0;
}

SLAPI AtolCardIntrf::~AtolCardIntrf()
{
	if(P_DispIntrf) {
		P_DispIntrf->SetProperty(Active, FALSE);
		ZDELETE(P_DispIntrf);
	}
}

int SLAPI AtolCardIntrf::InitIntrf()
{
	int    ok = 1;
	THROW_MEM(P_DispIntrf = new ComDispInterface);
	if(P_DispIntrf->Init("AddIn.acExch2")) {
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, ResultCode) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, ResultDescription) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, Active) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, TableName) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, OpenTable) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, RefreshTable) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, CloseTable) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, FieldIndex) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, FieldValue) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, FieldValueAsString) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, RecordCount) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, RecordNumber) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, BeginTransaction) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, ApplyTransaction) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, CancelTransaction) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, AddRecord) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, EditRecord) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, PostRecord) > 0);
		THROW(ASSIGN_ID_BY_NAME(P_DispIntrf, DeleteRecord) > 0);
		THROW(P_DispIntrf->SetProperty(Active, TRUE));
	}
	else {
		ok = -1;
	}
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACStartTransaction()
{
	int    ok = -1;
	if(!IsStartedTransaction) {
		THROW(ExecAtolOper(BeginTransaction));
		IsStartedTransaction = 1;
		ok = 1;
	}
	OperCount = 0;
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACEndTransaction()
{
	int    ok = -1;
	if(IsStartedTransaction) {
		THROW(ExecAtolOper(ApplyTransaction));
		IsStartedTransaction = 0;
		ok = 1;
	}
	OperCount = 0;
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACCancelTransaction()
{
	int    ok = -1;
	THROW(ACCloseTable());
	if(IsStartedTransaction) {
		THROW(ExecAtolOper(CancelTransaction));
		IsStartedTransaction = 0;
		ok = 1;
	}
	OperCount = 0;
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACOpenTable(const char * pTableName)
{
	int    ok = 1;
	THROW(P_DispIntrf->SetProperty(TableName, pTableName));
	THROW(ExecAtolOper(OpenTable));
	IsOpenedTable = 1;
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACCloseTable()
{
	int    ok = -1;
	if(IsOpenedTable) {
		THROW(ExecAtolOper(RefreshTable));
		THROW(ExecAtolOper(CloseTable));
		IsOpenedTable = 0;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACClearTable(const char * pTableName)
{
	int    ok = 1;
	long   numrec = 0;
	THROW(ACOpenTable(pTableName));
	THROW(P_DispIntrf->GetProperty(RecordCount, &numrec));
	for(; numrec > 0 ; numrec--) {
		THROW(P_DispIntrf->SetProperty(RecordNumber, numrec));
		THROW(ExecAtolOper(DeleteRecord));
	}
	THROW(ACCloseTable());
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACNewRecord()
{
	int    ok = 1;
	THROW(ExecAtolOper(AddRecord));
	THROW(ExecAtolOper(EditRecord));
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACPut(int fieldNo, long value)
{
	int    ok = 1;
	THROW(P_DispIntrf->SetProperty(FieldIndex, fieldNo));
	THROW(P_DispIntrf->SetProperty(FieldValue, value));
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACPut(int fieldNo, const char * pStr)
{
	int    ok = 1;
	THROW(P_DispIntrf->SetProperty(FieldIndex, fieldNo));
	THROW(P_DispIntrf->SetProperty(FieldValueAsString, pStr));
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ACInsert(PPID * pID)
{
	int    ok = 1;
	long   id = 0;
	THROW(ExecAtolOper(PostRecord));
	THROW(P_DispIntrf->SetProperty(FieldIndex, 0L));
	THROW(P_DispIntrf->GetProperty(FieldValue, &id));
	THROW(CheckForOperCount());
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int SLAPI AtolCardIntrf::ACGetIDsAry(const char * pTableName, int fieldNo, LAssocArray * pAry)
{
	int    ok = 1;
	if(pAry) {
		long   i, numrec = 0, id1, id2;
		THROW(ACOpenTable(pTableName));
		THROW(P_DispIntrf->GetProperty(RecordCount, &numrec));
		for(i = 1; i <= numrec; i++) {
			THROW(P_DispIntrf->SetProperty(RecordNumber, i));
			THROW(P_DispIntrf->SetProperty(FieldIndex, fieldNo));
			THROW(P_DispIntrf->GetProperty(FieldValue, &id1));
			THROW(P_DispIntrf->SetProperty(FieldIndex, 0L));
			THROW(P_DispIntrf->GetProperty(FieldValue, &id2));
			THROW(pAry->Add(id1, id2, 0));
		}
		THROW(ACCloseTable());
	}
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::CheckForOperCount()
{
	int    ok = 1;
	char   table_name[32];
	if(++OperCount == AC_MAX_OPER_COUNT && IsStartedTransaction) {
		THROW(P_DispIntrf->GetProperty(TableName, table_name, sizeof(table_name)));
		THROW(ACCloseTable());
		THROW(ACEndTransaction());
		THROW(ACStartTransaction());
		THROW(ACOpenTable(table_name));
	}
	CATCHZOK
	return ok;
}

int SLAPI AtolCardIntrf::ExecAtolOper(PPID id)
{
	return P_DispIntrf->CallMethod(id) ? 1 : (SetErrorMessage(), 0);
}

int SLAPI AtolCardIntrf::SetErrorMessage()
{
	int    ok = -1;
	char   err_buf[MAXPATH];
	memzero(err_buf, sizeof(err_buf));
	THROW(P_DispIntrf->GetProperty(ResultCode, &ResCode) > 0);
	if(ResCode != 0) {
		THROW(P_DispIntrf->GetProperty(ResultDescription, err_buf, sizeof(err_buf) - 1) > 0);
		SCharToOem(err_buf);
		PPSetAddedMsgString(err_buf);
		PPSetError(PPERR_ATOL_CARD);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//   Интерфейс к драйверу "Атол"
//

//   Схемы в ATOL для PAPYRUS
#define ATOL_INNER_SCHEME           1
#define ATOL_OUTER_SCHEME        1001
//   BIAS для кода категории карт в ATOL-CARD
#define AC_BIAS_CARD_CAT_CODE   10000
//   Длина поля кода карты
#define AC_DEF_CARD_CODE_LEN       40
//   Тип чека
#define CHK_SALE            0   // Продажа
#define CHK_RETURN          1   // Возврат
//   Тип оплаты
#define PAYMENT_CASH        1   // Наличными
//   Тип операции (транзакции) с чеком
#define OPTYPE_CHKLINE     11   // Строка чека
#define OPTYPE_STORNO      12   // Сторно строки
#define OPTYPE_PAYM1       40   // Оплата с вводом суммы клиента
#define OPTYPE_PAYM2       41   // Оплата без ввода суммы клиента
#define OPTYPE_CHKCLOSED   55   // Закрытие чека
#define OPTYPE_CANCEL      56   // Отмена чека
#define OPTYPE_ZREPORT     61   // Z-отчет

class ACS_ATOL : public PPAsyncCashSession {
public:
	SLAPI  ACS_ATOL(PPID id);
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
	virtual int SLAPI FinishImportSession(PPIDArray *);  // @v6.0.5 VADIM
	virtual int SLAPI SetGoodsRestLoadFlag(int updOnly); // @v5.3.6 VADIM
protected:
	virtual int SLAPI ExportSCard(FILE * pFile, int updOnly);
	int    UseInnerAutoDscnt; // @v5.4.12 VADIM
private:
	int    SLAPI ConvertWareList(const char * pImpPath, const char * pExpPath);
	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	PPIDArray SessAry; // @v6.0.5 VADIM
	SString PathRpt;
	SString PathFlag;
	// @v5.9.9 VADIM {
	SString PathGoods;
	SString PathGoodsFlag;
	int     UseAltImport;
	// } @v5.9.9 VADIM
	int     ImpExpTimeout;
	StringSet ImpPaths;
	StringSet ExpPaths;
};

class CM_ATOL : public PPCashMachine {
public:
	SLAPI CM_ATOL(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_ATOL(NodeID); }
};

REGISTER_CMT(ATOL,0,1);

class ACS_ATOLWOATOLCARD : public ACS_ATOL {
public:
	SLAPI  ACS_ATOLWOATOLCARD(PPID id);
private:
	virtual int SLAPI ExportSCard(FILE * pFile, int updOnly);
};

class CM_ATOLWOATOLCARD : public PPCashMachine {
public:
	SLAPI CM_ATOLWOATOLCARD(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPAsyncCashSession * SLAPI AsyncInterface()
	{
		return new ACS_ATOLWOATOLCARD(NodeID);
	}
};

REGISTER_CMT(ATOLWOATOLCARD,0,1);

SLAPI ACS_ATOLWOATOLCARD::ACS_ATOLWOATOLCARD(PPID id) : ACS_ATOL(id)
{
	UseInnerAutoDscnt = 1;
}

int SLAPI ACS_ATOLWOATOLCARD::ExportSCard(FILE * pFile, int updOnly)
{
	int    ok = -1;
	if(pFile) {
		long   dscnt_code = 0;
		PPID   ser_id;
		const  char * p_format = "%s\n";
		SString f_str, temp;
		PPObjSCardSeries scs_obj;
		PPSCardSeries    ser_rec;
		PPObjSCard       s_crd_obj;
		THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_DELETEALLDSCNTS, f_str));
		THROW_PP(fprintf(pFile, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
		if(!updOnly) {
			THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_DELETEALLSCHEMES, f_str));
			THROW_PP(fprintf(pFile, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
			THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_ADDINNERSCHEMES, f_str));
			THROW_PP(fprintf(pFile, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
			(f_str = 0).Cat(ATOL_INNER_SCHEME).Semicol();             // #1 - код схемы внутренней авт.скидки
			// @v9.0.2 PPGetWord(PPWORD_CARD, 0, temp);
			PPLoadString("card", temp); // @9.0.2
			f_str.Cat(temp.Transf(CTRANSF_INNER_TO_OUTER)).CatCharN(';', 2); // #2 - наименование схемы, #3 - не используется //
			f_str.CatChar('0').Semicol(); // #4 - тип операции объединения (0 - не объединять)
			THROW_PP(fprintf(pFile, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
		}
		THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_ADDSCARDDSCNTS, f_str));
		THROW_PP(fprintf(pFile, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
		for(ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;)
			if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
				PPSCardSerPacket scs_pack;
				SCardTbl::Key2 k;
				MEMSZERO(k);
				k.SeriesID = ser_id;
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				while(s_crd_obj.P_Tbl->search(2, &k, spGt) && k.SeriesID == ser_id) {
					SCardTbl::Rec card_rec;
					s_crd_obj.P_Tbl->copyBufTo(&card_rec);
					s_crd_obj.SetInheritance(&scs_pack, &card_rec);
					if(card_rec.PDis && !(card_rec.Flags & SCRDF_CLOSED) && !(card_rec.Expiry && card_rec.Expiry < LConfig.OperDate)) {
						int  i;
						(f_str = 0).Cat(ATOL_INNER_SCHEME).Semicol(); // #1 - код схемы внутренней авт.скидки
						f_str.Cat(++dscnt_code).Semicol();      // #2 - код скидки
						f_str.Cat(card_rec.Code).Semicol();     // #3 - наименование скидки (код карты)
						f_str.Cat(PPGetWord(PPWORD_DISCOUNT, 0, temp)).CatChar(' ');
						(temp = 0).Cat(fdiv100i(card_rec.PDis), MKSFMTD(0, 2, NMBF_NOTRAILZ));
						f_str.Cat(temp).CatChar('%').Semicol(); // #4 - текст для чека
						f_str.CatChar('0').Semicol();           // #5 - тип скидки (0 - процентная)
						f_str.Cat(temp).Semicol();              // #6 - значение скидки
						(temp = 0).CatCharN(';', 2).CatChar('0').Semicol();
						// #7 - #24 - не используем
						for(i = 0; i < 6; i++)
							f_str.Cat(temp);
						f_str.Cat(card_rec.Code).Semicol().Cat(card_rec.Code).Semicol().
							CatChar('0').Semicol().Transf(CTRANSF_INNER_TO_OUTER);    // #25 - #27 - описание фактора скидки (код карты)
						f_str.Cat(temp).Cat(temp).Semicol().Cat(temp).Cat(temp); // #28 - #40 - не используем
						THROW_PP(fprintf(pFile, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
					}
				}
			}
	}
	CATCHZOK
	return ok;
}

SLAPI ACS_ATOL::ACS_ATOL(PPID id) : PPAsyncCashSession(id)
{
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ATOL_TIMEOUT, &ImpExpTimeout);
	UseAltImport = 0;
	UseInnerAutoDscnt = 0;
	ChkRepPeriod.SetZero();
}

int SLAPI ACS_ATOL::ExportSCard(FILE *, int)
{
	int   ok = 1, r;
	AtolCardIntrf ac_intrf;
	PPWait(1);
	THROW(r = ac_intrf.InitIntrf());
	if(r > 0) {
		uint     c;
		long     owner_code = 0;
		PPID     ser_id = 0, last_cat_id = 0, last_cat_code = 0;
		SString  buf, card_tbl_name, cat_tbl_name, card_n_cat_tbl_name;
		PPObjSCardSeries scs_obj;
		PPSCardSeries ser_rec;
		PPObjSCard   s_crd_obj;
		PPObjPerson  psn_obj;
		LAssocArray  cat_code_ary;
		LAssocArray  card_n_cat_ary;
		SString scardcat_fmt_buf;
		PPLoadText(PPTXT_ATOL_SCARDCAT, scardcat_fmt_buf);
		scardcat_fmt_buf.Transf(CTRANSF_INNER_TO_OUTER);
		THROW(PPGetSubStr(PPTXT_ATOLCARD_TBL_NAMES, PPACTN_CARDS,        card_tbl_name));
		THROW(PPGetSubStr(PPTXT_ATOLCARD_TBL_NAMES, PPACTN_CATEGORIES,   cat_tbl_name));
		THROW(PPGetSubStr(PPTXT_ATOLCARD_TBL_NAMES, PPACTN_CARDS_N_CATS, card_n_cat_tbl_name));
		THROW(ac_intrf.ACStartTransaction());
		THROW(ac_intrf.ACClearTable(card_n_cat_tbl_name));
		THROW(ac_intrf.ACClearTable(card_tbl_name));
		card_n_cat_ary.freeAll();
		cat_code_ary.freeAll();
		THROW(ac_intrf.ACGetIDsAry(cat_tbl_name, 2, &cat_code_ary));
		THROW(ac_intrf.ACOpenTable(card_tbl_name));
		MEMSZERO(ser_rec);
		for(ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
			if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
				PPSCardSerPacket scs_pack;
				SCardTbl::Key2 k;
				MEMSZERO(k);
				k.SeriesID = ser_id;
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				while(s_crd_obj.P_Tbl->search(2, &k, spGt) && k.SeriesID == ser_id) {
					SCardTbl::Rec card_rec;
					s_crd_obj.P_Tbl->copyBufTo(&card_rec);
					s_crd_obj.SetInheritance(&scs_pack, &card_rec);
					if(!(card_rec.Flags & SCRDF_CLOSED) && !(card_rec.Expiry && card_rec.Expiry < LConfig.OperDate)) {
						uint  pos = 0;
						PPID  card_id, cat_code = card_rec.PDis + AC_BIAS_CARD_CAT_CODE;
						PersonTbl::Rec psn_rec;
						THROW(ac_intrf.ACNewRecord());
						THROW(ac_intrf.ACPut(1, ++owner_code));     // Код владельца карты
						if(psn_obj.Search(card_rec.PersonID, &psn_rec) > 0)
							(buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
						else
							buf = 0;
						THROW(ac_intrf.ACPut(2, buf));              // Владелец карты
						(buf = 0).CatCharN(' ', AC_DEF_CARD_CODE_LEN - strlen(card_rec.Code)).Cat(card_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
						THROW(ac_intrf.ACPut(5, buf));              // Код дисконтной карты
						(buf = 0).Cat(card_rec.Dt, DATF_GERMAN | DATF_CENTURY);
						THROW(ac_intrf.ACPut(6, buf));              // Дата выпуска карты
						THROW(ac_intrf.ACInsert(&card_id));
						if(last_cat_code != cat_code) {
							if(!cat_code_ary.Search(cat_code, &last_cat_id, 0)) {
								THROW(ac_intrf.ACCloseTable());
								THROW(ac_intrf.ACOpenTable(cat_tbl_name));
								THROW(ac_intrf.ACNewRecord());
								(buf = 0).Printf(scardcat_fmt_buf, fdiv100i(card_rec.PDis));
								THROW(ac_intrf.ACPut(1, buf));      // Наименование категории карты
								THROW(ac_intrf.ACPut(2, cat_code)); // Код категории карты
								THROW(ac_intrf.ACInsert(&last_cat_id));
								THROW(ac_intrf.ACCloseTable());
								THROW(cat_code_ary.Add(cat_code, last_cat_id, 0));
								THROW(ac_intrf.ACOpenTable(card_tbl_name));
							}
							last_cat_code = cat_code;
						}
						THROW(card_n_cat_ary.Add(last_cat_id, card_id, 0));
					}
				}
			}
		}
		THROW(ac_intrf.ACCloseTable());
		THROW(ac_intrf.ACOpenTable(card_n_cat_tbl_name));
		for(c = 0; c < card_n_cat_ary.getCount(); c++) {
			LAssoc  card_n_cat = card_n_cat_ary.at(c);
			THROW(ac_intrf.ACNewRecord());
			THROW(ac_intrf.ACPut(0, card_n_cat.Key)); // ID категории карты
			THROW(ac_intrf.ACPut(1, card_n_cat.Val)); // ID карты
			THROW(ac_intrf.ACInsert());
		}
		THROW(ac_intrf.ACCloseTable());
		THROW(ac_intrf.ACEndTransaction());
	}
	else
		ok = -1;
	CATCH
		ac_intrf.ACCancelTransaction();
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI ACS_ATOL::SetGoodsRestLoadFlag(int updOnly)
{
	int    ok = -1, use_replace_qtty_wosale = 0;
	PPIniFile  ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ATOL_QTTYWOSALE, &use_replace_qtty_wosale);
	if(use_replace_qtty_wosale && (updOnly || PPMessage(mfConf|mfYesNo, PPCFM_LOADGDSRESTWOSALES) == cmYes)) {
		Flags |= PPACSF_LOADRESTWOSALES;
		ok = 1;
	}
	else
		Flags &= ~PPACSF_LOADRESTWOSALES;
	return ok;
}

int SLAPI ACS_ATOL::ExportData(int updOnly)
{
	int    ok = 1, next_barcode = 0;
	//char   load_symb = '$';
	const  char * p_load_symb = "$";
	const  char * p_format = "%s\n";
	PPID   prev_goods_id = 0, stat_id = 0;
	LAssocArray  grp_n_level_ary;
	SString   f_str, tail;
	SString   path_goods, path_flag;
	PPUnit    unit_rec;
	PPObjUnit unit_obj;
	PPAsyncCashNode cn_data;
	AsyncCashGoodsInfo gds_info;
	AsyncCashGoodsIterator * p_gds_iter = 0;
	AsyncCashGoodsGroupInfo grp_info;
	AsyncCashGoodsGroupIterator * p_grp_iter = 0;
	FILE * p_file = 0;
	const  int check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);

	THROW(GetNodeData(&cn_data) > 0);
	if(cn_data.DrvVerMajor > 3 || (cn_data.DrvVerMajor == 3 && cn_data.DrvVerMinor >= 4))
		p_load_symb = "#";
	if(!P_Dls)
		THROW_MEM(P_Dls = new DeviceLoadingStat);
	P_Dls->StartLoading(&stat_id, dvctCashs, NodeID, 1);
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_ATOL_IMP_TXT,  path_goods));
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_ATOL_IMP_FLAG, path_flag));
	THROW_PP_S(p_file = fopen(path_flag, "w"), PPERR_CANTOPENFILE, path_flag);
	fclose(p_file);
	THROW_PP_S(p_file = fopen(path_goods, "w"), PPERR_CANTOPENFILE, path_goods);
	THROW_MEM(p_gds_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, P_Dls));
	THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_INITFILE, f_str));
	THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	THROW_PP(fprintf(p_file, p_format, p_load_symb) > 0, PPERR_EXPFILEWRITEFAULT);
	THROW(ExportSCard(p_file, updOnly));
	if(updOnly || (Flags & PPACSF_LOADRESTWOSALES)) {
		THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS,
			(Flags & PPACSF_LOADRESTWOSALES) ? PPATOLCS_REPLACEQTTYWOSALE : PPATOLCS_REPLACEQTTY, f_str));
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	else {
		THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_DELETEALL, f_str));
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
		THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_REPLACEQTTY, f_str));
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
		THROW_MEM(p_grp_iter = new AsyncCashGoodsGroupIterator(NodeID, 0, P_Dls));
		while(p_grp_iter->Next(&grp_info) > 0) {
			f_str = 0;
			f_str.Cat(grp_info.ID).CatCharN(';', 2);      // #1 - ИД группы товаров, #2 - не используется //
			tail = grp_info.Name;
			tail.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4).Semicol();
			f_str.Cat(tail);                              // #3 - Наименование группы товаров
			f_str.Cat(tail);                              // #4 - Текст для чека (?)
			f_str.CatCharN(';', 8);                       // #5-#12 - Не используем
			f_str.Cat(ATOL_OUTER_SCHEME).Semicol();    // #13 - Код схемы внешней автоматической скидки
			f_str.CatCharN(';', 2);                       // #14-#15 - Не используем
			f_str.Cat(grp_info.ParentID).Semicol();    // #16 - ИД родительской группы
			f_str.CatChar('0').Semicol();              // #17 - Признак товарной группы (0)
			f_str.Cat((long)grp_info.Level).Semicol(); // #18 - Номер уровня иерархического списка
			f_str.CatCharN(';', 4);                       // #19-#22 - Не используем
			f_str.Cat((long)(grp_info.VatRate * 100.0)).Semicol(); // #23 - Налоговая ставка (в 0.01%)
			THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
			grp_n_level_ary.Add(grp_info.ID, grp_info.Level, 0);
		}
	}
	while(p_gds_iter->Next(&gds_info) > 0) {
		size_t bclen;
	   	if(gds_info.ID != prev_goods_id) {
			int    i;
			long   level = 0;
			if(prev_goods_id) {
				f_str.Transf(CTRANSF_INNER_TO_OUTER).Semicol().Cat(tail);
				THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
			}
			next_barcode = 0;
			f_str = 0;
			f_str.Cat(gds_info.ID).Semicol();            // #1 - ИД товара
			tail = gds_info.Name;
			tail.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4).Semicol(); // #3 - Наименование товара
			tail.Cat(tail);                                 // #4 - Текст для чека
			tail.Cat(gds_info.Price, SFMT_MONEY).Semicol(); // #5 - Цена товара
			if(CheckCnFlag(CASHF_EXPGOODSREST))
				tail.Cat(gds_info.Rest, SFMT_QTTY);  // #6 - Остаток
			tail.Semicol();
			if(UseInnerAutoDscnt && gds_info.NoDis <= 0)    // && gds_info.NoDis <= 0 - @v6.0.1 VADIM
				tail.Cat(ATOL_INNER_SCHEME);                // #7 - Код схемы внутренней автоматической скидки
			tail.Semicol();
			// #8 - Флаги {
		   	if(unit_obj.Fetch(gds_info.UnitID, &unit_rec) > 0)
				tail.CatChar((unit_rec.Flags & PPUnit::IntVal) ? '0' : '1'); // Разрешить дробное кол-во
			else {
				//
				// Если для товара не определена единица измерения (правда, так быть не должно),
				// то НЕ разрешаем дробное количество
				//
				tail.CatChar('0');
			}
			for(i = 0; i < 4; i++)
				tail.Comma().CatChar('1'); // Разрешить продажи, возврат, отриц.остатки, регистрацию без указания кол-ва
			tail.Semicol();
			// }
			if(gds_info.NoDis > 0)
				tail.Cat(gds_info.Price, SFMT_MONEY);       // #9 - Min цена товара
			tail.Semicol();
			tail.CatCharN(';', 3);                                // #10-#12 - Не используем
			if(gds_info.NoDis <= 0)                               // @v6.0.1 VADIM
				tail.Cat(ATOL_OUTER_SCHEME);                      // #13 - Код схемы внешней автоматической скидки
			tail.Semicol();
			tail.CatCharN(';', 2);                                // #14-#15 - Не используем
			if(cn_data.Flags & CASHF_EXPGOODSGROUPS && gds_info.ParentID && grp_n_level_ary.Search(gds_info.ParentID, &level, 0) > 0)
				tail.Cat(gds_info.ParentID);                      // #16 - ИД группы товаров
			else
				tail.CatChar('0');
			tail.Semicol();
			tail.CatChar('1').Semicol();                          // #17 - Признак товара (1)
			tail.Cat(level + 1).Semicol();                        // #18 - Номер уровня иерархического списка
			tail.CatCharN(';', 4);                                // #19-#22 - Не используем
			tail.Cat((long)(gds_info.VatRate * 100.0)).Semicol(); // #23 - Налоговая ставка (в 0.01%)
		}
		if((bclen = strlen(gds_info.BarCode)) != 0) {
			gds_info.AdjustBarcode(check_dig);
			int    wp = GetGoodsCfg().IsWghtPrefix(gds_info.BarCode);
			if(wp == 1)
				STRNSCPY(gds_info.BarCode, gds_info.BarCode+strlen(GetGoodsCfg().WghtPrefix));
			else if(wp == 2)
				STRNSCPY(gds_info.BarCode, gds_info.BarCode+strlen(GetGoodsCfg().WghtCntPrefix));
			else
				AddCheckDigToBarcode(gds_info.BarCode);
			if(next_barcode)
				f_str.Comma();
			next_barcode = 1;
			f_str.Cat(gds_info.BarCode); // #2 - Список штрихкодов через запятую
		}
	   	prev_goods_id = gds_info.ID;
		PPWaitPercent(p_gds_iter->GetIterCounter());
	}
	if(prev_goods_id) {
		f_str.Transf(CTRANSF_INNER_TO_OUTER).Semicol().Cat(tail);
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	fclose(p_file);
	PPWait(0);
	PPWait(1);
	THROW(DistributeFile(path_goods, 0));
	THROW(DistributeFile(path_flag,  0));
	if(stat_id)
		P_Dls->FinishLoading(stat_id, 1, 1);
	CATCH
		SFile::ZClose(&p_file);
		ok = 0;
	ENDCATCH
	delete p_gds_iter;
	delete p_grp_iter;
	PPWait(0);
	return ok;
}

int SLAPI ACS_ATOL::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtrErr(&dlg)) {
			SString dt_buf;
			ChkRepPeriod.low = LConfig.OperDate;
			ChkRepPeriod.upp = LConfig.OperDate;
			SetupCalCtrl(CTLCAL_DATERNG_PERIOD, dlg, CTL_DATERNG_PERIOD, 1);
			SetPeriodInput(dlg, CTL_DATERNG_PERIOD, &ChkRepPeriod);
			PPWait(0);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(dlg->getCtrlString(CTL_DATERNG_PERIOD, dt_buf) && strtoperiod(dt_buf, &ChkRepPeriod, 0) && !ChkRepPeriod.IsZero()) {
					SETIFZ(ChkRepPeriod.upp, plusdate(LConfig.OperDate, 2));
					if(diffdate(ChkRepPeriod.upp, ChkRepPeriod.low) >= 0)
						ok = valid_data = 1;
				}
				if(ok < 0)
					PPErrorByDialog(dlg, CTL_DATERNG_PERIOD, PPERR_INVPERIODINPUT);
			}
			PPWait(1);
		}
	}
	else {
		ChkRepPeriod = *pPrd;
		ok = 1;
	}
	if(ok > 0) {
		PPAsyncCashNode acn;
		THROW(GetNodeData(&acn) > 0);
		acn.GetLogNumList(LogNumList);
		{
			SString  alt_imp_params;
			PPIniFile  ini_file;
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_ACSCLOSE_USEALTIMPORT, alt_imp_params) > 0) {
				uint    pos = 0;
				SString param;
				StringSet params(';', alt_imp_params);
				params.get(&pos, param);
				UseAltImport = BIN(param.ToLong());
				if(UseAltImport) {
					params.get(&pos, param);
					if(param.NotEmptyS()) {
						uint  pos = 0;
						StringSet  file_names(',', param);
						file_names.get(&pos, PathRpt);
						file_names.get(&pos, PathFlag);
						file_names.get(&pos, PathGoods);
						file_names.get(&pos, PathGoodsFlag);
					}
				}
			}
			if(!PathRpt.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_EXP_TXT,   PathRpt));
			if(!PathFlag.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_EXP_FLAG,  PathFlag));
			if(!PathGoods.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_GOODS_TXT, PathGoods));
			if(!PathGoodsFlag.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_GOODS_FLG, PathGoodsFlag));
		}
		THROW_PP(acn.ExpPaths.NotEmptyS() || acn.ImpFiles.NotEmptyS(), PPERR_INVFILESET);
		ImpPaths.clear();
		ImpPaths.setDelim(";");
		{
			SString & r_list = acn.ImpFiles.NotEmpty() ? acn.ImpFiles : acn.ExpPaths;
			ImpPaths.setBuf(r_list, r_list.Len()+1);
		}
		ExpPaths.clear();
		ExpPaths.setDelim(";");
		{
			SString & r_list = acn.ExpPaths.NotEmpty() ? acn.ExpPaths : acn.ImpFiles;
			ExpPaths.setBuf(r_list, r_list.Len()+1);
		}
	}
	CATCHZOK
	*pSessCount = BIN(ok > 0);
	*pIsForwardSess = 0; // 1; @v6.0.7 1-->0
	delete dlg;
	return ok;
}

int SLAPI ACS_ATOL::ConvertWareList(const char * pImpPath, const char * pExpPath)
{
	int    ok = 1, ta = 0, field_no;
	uint   pos;
	long   op_type, nsmena, cash_no, chk_no;
	long   count = 0;
	PPID   grp_id = 0, goods_id = 0;
	PPIDArray   new_goods;
	LAssocArray zrep_ary; // Пара {номер_кассы; номер_смены}
	LDATETIME dtm;
	SString   buf, card_code, wait_msg;
	StringSet ss(';', 0);
	IterCounter   cntr;
	SCardCore     sc_core;
	SCardTbl::Rec sc_rec;
	PPGoodsPacket gds_pack;
	PPObjGoods    goods_obj;

	SString   imp_file_name = PathRpt;
	SPathStruc::ReplacePath(imp_file_name, pImpPath, 1);
	SFile     imp_file(imp_file_name, SFile::mRead); // PathRpt-->imp_file_name
	THROW(imp_file.IsValid());
	for(pos = 0; pos < 3; pos++)
		imp_file.ReadLine(buf);
	THROW(PPStartTransaction(&ta, 1));
	// @v6.0.5 VADIM {
	//
	// Собираем список Z-отчетов в массив zrep_ary.
	// Для каждого найденного Z-отчета создаем запись в таблице кассовых сессий (CS)
	// и заносим идентификатор сессии в массив ACS_ATOS::SessAry.
	//
	while(imp_file.ReadLine(buf) > 0) {
		ss.clear();
		ss.add(buf);
		ss.get(&(pos = 0), buf);     // Код транзакции (не используем)
		ss.get(&pos, buf);           // Дата транзакции
		strtodate(buf.Strip(), DATF_DMY, &dtm.d);
		ss.get(&pos, buf);           // Время транзакции
		strtotime(buf.Strip(), TIMF_HMS, &dtm.t);
		ss.get(&pos, buf);           // Тип транзакции (операции)
		op_type = buf.ToLong();
		if(op_type == OPTYPE_ZREPORT) {
			ss.get(&pos, buf);       // Номер ККМ
			cash_no = buf.ToLong();
			if(LogNumList.lsearch(cash_no)) {
				PPID   sess_id;
				for(field_no = 5; field_no < 9 && ss.get(&pos, buf) > 0; field_no++); // Поля 6-8 пропускаем
				nsmena = buf.ToLong(); // Номер смены
				if(CS.SearchByNumber(&sess_id, NodeID, cash_no, nsmena, dtm) > 0) {
					if(CS.data.Temporary)
						THROW(CS.ResetTempSessTag(sess_id, 0));
				}
				else
					THROW(CS.CreateSess(&sess_id, NodeID, cash_no, nsmena, dtm, 0));
				SessAry.addUnique(sess_id);
				zrep_ary.Add(cash_no, nsmena, &(pos = 0));
			}
		}
		count++;
	}
	//
	//
	//
	imp_file.Seek(0);
	for(pos = 0; pos < 3; pos++)
		imp_file.ReadLine(buf);
	// } @v6.0.5 VADIM
	while(imp_file.ReadLine(buf) > 0) {
		ss.clear();
		ss.add(buf);
		ss.get(&(pos = 0), buf);     // Код транзакции (не используем)
		ss.get(&pos, buf);           // Дата транзакции
		strtodate(buf.Strip(), DATF_DMY, &dtm.d);
		ss.get(&pos, buf);           // Время транзакции
		strtotime(buf.Strip(), TIMF_HMS, &dtm.t);
		ss.get(&pos, buf);           // Тип транзакции (операции)
		op_type = buf.ToLong();
		if(op_type == OPTYPE_CHKCLOSED) {
			ss.get(&pos, buf);       // Номер ККМ
			cash_no = buf.ToLong();
			if(LogNumList.lsearch(cash_no)) {
				int    hour, min;
				long   chk_type;
				PPID   id = 0;
				LDATETIME dttm2;
				ss.get(&pos, buf);
				chk_no = buf.ToLong();      // Номер чека
				decodetime(&hour, &min, 0, 0, &dtm.t);
				dtm.t = encodetime(hour, min, (int)(chk_no % 60), 0);
				dttm2 = dtm;
				ss.get(&pos, buf);       // Поле 7 пропускаем
				ss.get(&pos, card_code); // Код дисконтной карты
				for(field_no = 8; field_no < 13 && ss.get(&pos, buf) > 0; field_no++); // Поля 9-12 пропускаем
				chk_type = buf.ToLong(); // Тип чека
				ss.get(&pos, buf);       // Номер смены
				nsmena = buf.ToLong();
				if(oneof2(chk_type, CHK_SALE, CHK_RETURN)) {
					long   fl  = (chk_type == CHK_RETURN) ? CCHKF_RETURN : 0;
					if(!zrep_ary.SearchPair(cash_no, nsmena, &(pos = 0)))
						fl |= CCHKF_TEMPSESS;
					THROW(AddTempCheck(&id, nsmena, fl, cash_no, chk_no, 0, 0, &dttm2, 0, 0));
				}
				if(card_code.NotEmptyS() && sc_core.SearchCode(0, card_code, &sc_rec) > 0)
					THROW(AddTempCheckSCardID(id, sc_rec.ID));
			}
		}
	}
	imp_file.Seek(0);
	for(pos = 0; pos < 3; pos++)
		imp_file.ReadLine(buf);
	cntr.Init(count);
	PPLoadText(PPTXT_ACSCLS_IMPORT, wait_msg);
	while(imp_file.ReadLine(buf) > 0) {
		int    r;
		long   op_type, cash_no, chk_no;
		ss.clear();
		ss.add(buf);
		pos = 0;
		//   № транзакции, дата транзакции, время транзакции - пропускаем, выбираем тип транзакции (операции)
		for(field_no = 0; field_no < 4 && ss.get(&pos, buf) > 0; field_no++);
		op_type = buf.ToLong();      // 4. Тип операции
		ss.get(&pos, buf);
		cash_no = buf.ToLong();      // 5. Номер ККМ
		ss.get(&pos, buf);
		chk_no = buf.ToLong();       // 6. Номер чека
		ss.get(&pos, buf);           // 7. Код кассира - пропускаем
		if(oneof2(op_type, OPTYPE_CHKLINE, OPTYPE_STORNO)) { // Строка чека или сторно строки
			double qtty, price, dscnt_price, dscnt;
			ss.get(&pos, buf);
			goods_id = buf.ToLong(); // 8. Код товара
			// @v5.9.9 VADIM {
			if(UseAltImport) {
				BarcodeTbl::Rec  bc_rec;
				if(goods_obj.SearchByArticle(goods_id, &bc_rec) > 0)
					goods_id = bc_rec.GoodsID;
				else {
					buf = "ATOL";
					if(!grp_id && goods_obj.P_Tbl->SearchByName(PPGDSK_GROUP, buf, &grp_id, &gds_pack.Rec) <= 0) {
						gds_pack.destroy();
						gds_pack.Rec.Kind = PPGDSK_GROUP;
						buf.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						THROW(goods_obj.PutPacket(&grp_id, &gds_pack, 0));
					}
					gds_pack.destroy();
					gds_pack.Rec.Kind = PPGDSK_GOODS;
					gds_pack.Rec.ParentID = grp_id;
					(buf = 0).CatEq("ID", goods_id);
					buf.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
					STRNSCPY(gds_pack.Rec.Abbr, gds_pack.Rec.Name);
					MEMSZERO(bc_rec);
					(buf = 0).CatChar('$').Cat(goods_id);
					buf.CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
					bc_rec.Qtty = 1.0;
					bc_rec.BarcodeType = -1;
					gds_pack.Codes.insert(&bc_rec);
					THROW(new_goods.add(goods_id));
					THROW(goods_obj.PutPacket(&(goods_id = 0), &gds_pack, 0));
				}
			}
			// } @v5.9.9 VADIM
			ss.get(&pos, buf);       // 9. Поле 9 - пропускаем
			ss.get(&pos, buf);
			price = buf.ToReal();    // 10. Цена
			ss.get(&pos, buf);
			qtty = buf.ToReal();     // 11. Количество
			for(field_no = 11; field_no < 15 && ss.get(&pos, buf) > 0; field_no++); // Поля 12-14 пропускаем
			dscnt_price = buf.ToReal(); // Цена  со скидками
			ss.get(&pos, buf);
			dscnt = buf.ToReal();    // Сумма со скидками
			THROW(r = SearchTempCheckByCode(cash_no, chk_no));
			if(r > 0) {
				double line_amount;
				PPID   chk_id = P_TmpCcTbl->data.ID;
				qtty = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -fabs(qtty) : fabs(qtty);
				if(op_type == OPTYPE_STORNO) {
					TempCCheckLineTbl::Key2 k2;
					k2.CheckID = chk_id;
					BExtQuery cclq(P_TmpCclTbl, 2);
					cclq.selectAll().where(P_TmpCclTbl->CheckID == chk_id);
					for(cclq.initIteration(0, &k2, spGe); cclq.nextIteration() > 0;)
						if(P_TmpCclTbl->data.GoodsID == goods_id && P_TmpCclTbl->data.Quantity == qtty) {
							THROW_DB(P_TmpCclTbl->deleteRec());
							break;
						}
					qtty = -qtty;
				}
				else {
					SetupTempCcLineRec(0, chk_id, chk_no, P_TmpCcTbl->data.Dt, 0, goods_id);
					SetTempCcLineValues(0, qtty, price, price - dscnt_price);
					THROW_DB(P_TmpCclTbl->insertRec());
				}
				line_amount = R2(qtty * dscnt_price);
				dscnt       = R2(qtty * price - dscnt);
				THROW(AddTempCheckAmounts(chk_id, line_amount, dscnt));
			}
		}
		else if(oneof2(op_type, OPTYPE_PAYM1, OPTYPE_PAYM2)) { // Оплата
			long   paym_type;
			ss.get(&pos, buf);        // Поле 8 - пропускаем
			ss.get(&pos, buf);
			paym_type = buf.ToLong(); // Тип оплаты (1 - наличные)
			if(paym_type != PAYMENT_CASH) {
				THROW(r = SearchTempCheckByCode(cash_no, chk_no));
				if(r > 0 && !(P_TmpCcTbl->data.Flags & CCHKF_BANKING)) {
					P_TmpCcTbl->data.Flags |= CCHKF_BANKING;
					THROW_DB(P_TmpCcTbl->updateRec());
				}
			}
		}
		else if(op_type == OPTYPE_CANCEL) { // Отмена чека
			THROW(r = SearchTempCheckByCode(cash_no, chk_no));
			if(r > 0) {
				TempCCheckLineTbl::Key2 ccl_k2;
				ccl_k2.CheckID = P_TmpCcTbl->data.ID;
				P_TmpCcTbl->data.Flags &= ~CCHKF_BANKING;
				LDBLTOMONEY(0.0, P_TmpCcTbl->data.Amount);
				LDBLTOMONEY(0.0, P_TmpCcTbl->data.Discount);
				THROW_DB(P_TmpCcTbl->updateRec());
				while(P_TmpCclTbl->searchForUpdate(2, &ccl_k2, spEq))
					THROW_DB(P_TmpCclTbl->deleteRec()); // @sfu
			}
		}
		PPWaitPercent(cntr.Increment(), wait_msg);
	}
	if(UseAltImport && new_goods.getCount()) {
		PPWait(0);
		PPLoadText(PPTXT_IMPGOODS, wait_msg);
		PPWaitMsg(wait_msg);
		SString path_goodsflag = PathGoodsFlag;
		SString path_goods = PathGoods;
		SPathStruc::ReplacePath(path_goodsflag, pExpPath, 1);
		SPathStruc::ReplacePath(path_goods, pImpPath, 1);
		THROW_PP(ok = WaitForExists(path_goodsflag, 1, NZOR(ImpExpTimeout, 5000)), PPERR_ATOL_IMPCHECKS);
		if(ok > 0) {
			SFile  gfile;
			StringSet ss_comma(',', 0);
			SFile::Remove(path_goods);
			buf = path_goodsflag;
			SPathStruc::ReplaceExt(buf, "tmp", 1);
			gfile.Open(buf, SFile::mWrite);
			THROW(gfile.IsValid());
			for(pos = 0; pos < new_goods.getCount(); pos++) {
				(buf = 0).Cat(new_goods.at(pos)).CR();
				gfile.WriteLine(buf);
			}
			buf = gfile.GetName();
			gfile.Close();
			SDelay(300);
			SFile::Rename(buf, path_goodsflag);
			THROW_PP(ok = WaitForExists(path_goodsflag, 1, NZOR(ImpExpTimeout, 5000)), PPERR_ATOL_IMPCHECKS);
			gfile.Open(path_goods, SFile::mRead);
			THROW(gfile.IsValid());
			cntr.Init(new_goods.getCount());
			while(gfile.ReadLine(buf) > 0) {
				ss.clear();
				ss.add(buf);
				ss.get(&(pos = 0), buf); // Артикул
				goods_id = buf.ToLong();
				ss.get(&pos, buf);       // Список штрихкодов
				ss_comma.clear();
				ss_comma.add(buf);
				ss.get(&pos, buf);       // 1 - товар найден в Атоле
				if(buf.ToLong()) {
					ss.get(&pos, buf);   // Наименование товара
					BarcodeTbl::Rec  bc_rec;
					if(goods_obj.SearchByArticle(goods_id, &bc_rec) > 0 && goods_obj.GetPacket(bc_rec.GoodsID, &gds_pack, 0) > 0) {
						goods_id = bc_rec.GoodsID;
						buf.Strip().ToOem().CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						STRNSCPY(gds_pack.Rec.Abbr, gds_pack.Rec.Name);
						gds_pack.Codes.insert(&bc_rec); // Добавляем артикул в список штрихкодов, т.к. GetPacket его пропускает
						for(pos = 0; ss_comma.get(&pos, buf);)
							if(buf.NotEmptyS()) {
								MEMSZERO(bc_rec);
								bc_rec.GoodsID = goods_id;
								buf.CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
								bc_rec.Qtty = 1.0;
								gds_pack.Codes.insert(&bc_rec);
							}
						THROW(goods_obj.PutPacket(&goods_id, &gds_pack, 0));
					}
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			}
			gfile.Close();
		}
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI ACS_ATOL::ImportSession(int)
{
	int    ok = 1, notify_timeout = ImpExpTimeout ? ImpExpTimeout : 5000;
	uint   set_no = 0;
	SString imp_path, exp_path;
	SString path_rpt;
	SString path_flag;
	LDATE  first_date = ChkRepPeriod.low, last_date = ChkRepPeriod.upp;
	SETIFZ(last_date, plusdate(LConfig.OperDate, 2));
	first_date = plusdate(first_date, -1);
	last_date  = plusdate(last_date, 1);
	THROW(CreateTables());
	for(uint i = 0; ImpPaths.get(&i, imp_path);) {
		set_no++;
		{
			int exp_path_found = 0;
			for(uint j = 0, n = 0; !exp_path_found && ExpPaths.get(&j, exp_path);)
				if(++n == set_no)
					exp_path_found = 1;
			if(!exp_path_found)
				exp_path = imp_path;
		}
		if(::access(imp_path, 0) == 0) {
			path_rpt = PathRpt;
			path_flag = PathFlag;
			SPathStruc::ReplacePath(path_rpt,  imp_path, 1);
			SPathStruc::ReplacePath(path_flag,  exp_path, 1);
			THROW_PP(ok = WaitForExists(path_flag, 1, notify_timeout), PPERR_ATOL_IMPCHECKS);
			if(ok > 0) {
				int     y, m, d;
				SString buf, tmp_buf, tmp_name, date_mask = "%02d.%02d.%04d";
				SFile::Remove(path_rpt);
				tmp_name = path_flag;
				SPathStruc::ReplaceExt(tmp_name, "tmp", 1);
				SFile  query_file(tmp_name, SFile::mWrite);
				THROW(PPGetSubStr(PPTXT_ATOL_CMDSTRINGS, PPATOLCS_TRANSBYDTTM, buf));
				query_file.WriteLine(buf.CR());
				decodedate(&d, &m, &y, &first_date);
				(buf = 0).Printf(date_mask, d, m, y).Semicol();
				decodedate(&d, &m, &y, &last_date);
				buf.Cat(tmp_buf.Printf(date_mask, d, m, y)).CR();
				query_file.WriteLine(buf);
				query_file.Close();
				SDelay(300);
				SFile::Rename(tmp_name, path_flag);
				THROW_PP(ok = WaitForExists(path_flag, 1, notify_timeout), PPERR_ATOL_IMPCHECKS);
				if(ok > 0)
					THROW(ConvertWareList(imp_path, exp_path));
			}
		}
		else {
			// log message
		}
	}
	CATCHZOK
	return ok;
}
// @v6.0.5 VADIM {
int SLAPI ACS_ATOL::FinishImportSession(PPIDArray * pSessList)
{
	return pSessList->addUnique(&SessAry);
}
// } @v6.0.5 VADIM
