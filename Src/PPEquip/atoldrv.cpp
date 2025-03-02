// ATOLDRV.CPP
// Copyright (c) A.Starodub, A.Sobolev 2010, 2011, 2013, 2015, 2016, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Интерфейс с драйвером оборудования АТОЛ 
//
#include <pp.h>
#pragma hdrstop
#include <atol-dto\libfptr10.h>

//HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\ATOL\Drivers\10.0\KKT
//
//
//
#define RESCODE_NO_ERROR             0L   // Нет ошибок
#define RESCODE_DVCCMDUNSUPP       -12L   // Код возврата "Команда не поддерживается в данной конфигурации оборудования"
#define RESCODE_MODE_OFF           -16L   // Не поддерживается в данном режиме устройства
#define RESCODE_NOPAPER          -3807L   // Нет бумаги
#define RESCODE_PAYM_LESS_SUM    -3835L   // Вносимая сумма меньше суммы чека
#define RESCODE_OPEN_SESS_LONG   -3822L   // Cмена октрыта более 24 часов
#define RESCODE_PRINTCONTROLTAPE -3827L   // Идет печать контрольной ленты
#define RESCODE_PRINTREPORT      -3829L   // Идет печать отчета
#define RESCODE_PAYM_LESS_SUM    -3835L   // Вносимая сумма меньше суммы чека

#define CHKST_CLOSE            0L

#define PRNMODE_PRINT          2L

#define MODE_REGISTER          1L
#define MODE_XREPORT           2L
#define MODE_ZREPORT           3L
#define MODE_EKLZ_REPORT       6L

#define REPORT_TYPE_Z          1L
#define REPORT_TYPE_X          2L
//
//
//
static void WriteLogFile_PageWidthOver(const char * pFormatName)
{
	SString msg_fmt;
	SString msg;
	msg.Printf(PPLoadTextS(PPTXT_SLIPFMT_WIDTHOVER, msg_fmt), pFormatName);
	PPLogMessage(PPFILNAM_ATOLDRV_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
}

class SCS_ATOLDRV : public PPSyncCashSession {
public:
	SCS_ATOLDRV(PPID n, char * name, char * port);
	~SCS_ATOLDRV();
	virtual int PrintCheck(CCheckPacket * pPack, uint flags);
	virtual int PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags);
	virtual int PrintXReport(const CSessInfo *) { return PrintReport(0); }
	virtual int OpenSession_(PPID sessID) 
	{ 
		int    ok = -1;
		StateBlock stb;
		if(P_Fptr10 && P_Fptr10->OpenShiftProc) {
			SStringU temp_buf_u;
			SString operator_name;
			THROW(Connect(&stb));
			{
				void * fph = P_Fptr10->Handler;
				PPSyncCashSession::GetCurrentUserName(operator_name);
				operator_name.Transf(CTRANSF_INNER_TO_OUTER);
				temp_buf_u.Z().CopyFromMb_OUTER(operator_name, operator_name.Len());
				P_Fptr10->SetParamStrProc(fph, 1021, temp_buf_u);
				//temp_buf_u.Z().CopyFromMb_OUTER(CashierPassword, CashierPassword.Len());
				temp_buf_u.Z().CopyFromMb_OUTER(P_Fptr10->UserPassword, P_Fptr10->UserPassword.Len());
				P_Fptr10->SetParamStrProc(fph, 1203, temp_buf_u);
				THROW(P_Fptr10->OperatorLoginProc(fph) == 0);
				P_Fptr10->OpenShiftProc(fph);
				ok = 1;
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int CloseSession(PPID sessID) { return PrintReport(1); }
	virtual int PrintZReportCopy(const CSessInfo *);
	virtual int PrintIncasso(double sum, int isIncome);
	virtual int OpenBox()
	{
		int     ok = -1;
		ResCode = RESCODE_NO_ERROR;
		ErrCode = SYNCPRN_ERROR;
		StateBlock stb;
		THROW(Connect(&stb));
		if(!(stb.Flags & stb.fDrawerOpened)) {
			if(P_Fptr10) {
				P_Fptr10->OpenDrawerProc(P_Fptr10->Handler);
			}
			else if(P_Disp) {
				THROW(ExecOper(OpenDrawer));
				ok = 1;
			}
		}
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
				DoBeep();
				if(Flags & sfCheckOpened)
					ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
				ok = 0;
			}
		ENDCATCH
		return ok;
	}
	virtual int GetSummator(double * pVal)
	{
		int    ok = 1;
		StateBlock stb;
		THROW(Connect(&stb));
		CATCH
			ok = (SetErrorMessage(), 0);
		ENDCATCH
		ASSIGN_PTR(pVal, stb.Summator);
		return ok;
	}
	virtual int GetDeviceTime(LDATETIME * pDtm) 
	{ 
		int    ok = -1;
		ASSIGN_PTR(pDtm, ZERODATETIME); 
		if(P_Fptr10) {
			int   yr = 0;
			int   mn = 0;
			int   d = 0;
			int   h = 0;
			int   m = 0;
			int   s = 0;
			P_Fptr10->GetParamDateTimeProc(P_Fptr10->Handler, LIBFPTR_PARAM_DATE_TIME, &yr, &mn, &d, &h, &m, &s);
			if(checkirange(yr, 1996, 2200) && checkirange(mn, 1, 12) && checkirange(d, 1, 31) && checkirange(h, 0, 24) && checkirange(m, 0, 60) && checkirange(s, 0, 60)) {
				pDtm->d = encodedate(yr, mn, d);
				pDtm->t = encodetime(h, m, s, 0);
				ok = 1;
			}
		}
		return ok; 
	}
	virtual int EditParam(void *);
	virtual int CheckForSessionOver()
	{
		int    ok = -1;
		long   err_code = 0L;
		StateBlock stb;
		THROW(Connect(&stb));
		if(P_Fptr10) {
			if(stb.ShiftState == LIBFPTR_SS_EXPIRED)
				ok = 1;
		}
		else if(P_Disp) {
			SetProp(Mode, MODE_REGISTER);
			Exec(SetMode);
			if(ResCode == RESCODE_OPEN_SESS_LONG)
				ok = 1;
		}
		CATCHZOK
		if(P_Disp)
			Exec(ResetMode);
		return ok;
	}
	virtual int PrintBnkTermReport(const char * pZCheck);
	virtual int PreprocessChZnCode(int op, const char * pCode, double qtty, int uomId, uint uomFragm, CCheckPacket::PreprocessChZnCodeResult & rResult); // @v11.2.12
	virtual int Diagnostics(StringSet * pSs)
	{ 
		if(pSs) {
			SString temp_buf;
			if(P_Fptr10) {
				pSs->add("fptr10 mode");
				if(P_Fptr10->GetVersionString) {
					temp_buf = P_Fptr10->GetVersionString();
					if(temp_buf.NotEmptyS())
						pSs->add(temp_buf);
				}
				temp_buf.Z().Cat("libfptr_util_form_nomenclature").Space().Cat(P_Fptr10->UtilFormNomenclature ? "defined" : "undefined");
				pSs->add(temp_buf);
			}
			else if(P_Disp) {
				pSs->add("dispatch mode");
			}
		}
		return 1; 
	} 
private:
	struct StateBlock {
		StateBlock() : OperatorId(0), LogicalNumber(0), ShiftState(0), ShiftNumber(0), Model(0), Mode(0),
			SubMode(0), DocNumber(0), ReceiptNumber(0), ReceiptType(0), ReceiptSum(0.0), Summator(0.0), LineLen(0), LineLenPix(0),
			Flags(0), Dtm(ZERODATETIME)
		{
		}
		uint   OperatorId;
		uint   LogicalNumber;
		uint   ShiftState;
		uint   ShiftNumber;
		uint   Model;
		uint   Mode;
		uint   SubMode;
		uint   DocNumber;
		uint   ReceiptNumber;
		uint   ReceiptType;
		double ReceiptSum;
		double Summator; // Сумма выручки в кассе
		uint   LineLen;
		uint   LineLenPix;
		enum {
			fFiscalDevice = 0x0001,
			fFiscalFN     = 0x0002,
			fFNPresent    = 0x0004,
			fDrawerOpened = 0x0008,
			fPaperPresent = 0x0010,
			fCoverOpened  = 0x0020,
			fPrnConnLost  = 0x0040,
			fPrnError     = 0x0080,
			fCutError     = 0x0100,
			fPrnOverheat  = 0x0200,
			fDvcBlocked   = 0x0400
		};
		uint   Flags;
		LDATETIME Dtm;
		SString Serial;
		SString ModelName;
		SString UnitVersion;
	};
	class AtolFptr10 {
	public:
		enum {
			stInitialized = 0x0001
		};
		AtolFptr10() : State(0), Handler(0), 
			CreateHandleProc(0), DestroyHandleProc(0), GetVersionString(0), GetSettingsProc(0), GetErrorCodeProc(0), GetErrorDescrProc(0),
			OpenProc(0), CloseProc(0), IsOpenedProc(0), SetParamBoolProc(0), SetParamIntProc(0), SetParamDoubleProc(0), 
			SetParamStrProc(0), SetParamDatetimeProc(0), SetParamByteArrayProc(0), 
			SetNonPrintableParamByteArrayProc(0), PrintTextProc(0), PaymentProc(0), CutProc(0), BeepProc(0),
			QueryDataProc(0), GetParamIntProc(0), GetParamDoubleProc(0), GetParamBoolProc(0), GetParamStrProc(0), GetParamDateTimeProc(0),
			GetParamByteArrayProc(0), GetSingleSettingProc(0), SetSingleSettingProc(0), ApplySingleSettingsProc(0), OpenDrawerProc(0),
			OperatorLoginProc(0), OpenReceiptProc(0), CancelReceiptProc(0), RegistrationProc(0), CloseReceiptProc(0), PrintBarcodeProc(0),
			ReportProc(0), CheckDocumentClosedProc(0), CashIncomeProc(0), CashOutcomeProc(0), UtilFormNomenclature(0), UtilFormTlvProc(0),
			ProcessJsonProc(0), OpenShiftProc(0)
		{
			SString dll_path;
			PPGetFilePath(PPPATH_BIN, "fptr10.dll", dll_path);
			THROW_SL(fileExists(dll_path));
			THROW_SL(Lib.Load(dll_path));
			THROW_SL(CreateHandleProc  = reinterpret_cast<int (* cdecl)(void **)>(Lib.GetProcAddr("libfptr_create")));
			THROW_SL(DestroyHandleProc = reinterpret_cast<int (* cdecl)(void **)>(Lib.GetProcAddr("libfptr_destroy")));
			THROW_SL(GetVersionString  = reinterpret_cast<const char * (* cdecl)()>(Lib.GetProcAddr("libfptr_get_version_string")));
			THROW_SL(GetSettingsProc   = reinterpret_cast<int (* cdecl)(void *, wchar_t *, int)>(Lib.GetProcAddr("libfptr_get_settings")));
			THROW_SL(PrintTextProc   = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_print_text")));
			THROW_SL(PaymentProc     = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_payment")));
			THROW_SL(CutProc         = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_cut")));
			THROW_SL(BeepProc        = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_beep")));
			THROW_SL(OpenDrawerProc  = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_open_drawer")));
			THROW_SL(QueryDataProc   = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_query_data")));
			THROW_SL(OperatorLoginProc       = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_operator_login")));
			THROW_SL(OpenReceiptProc = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_open_receipt")));
			THROW_SL(CancelReceiptProc       = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_cancel_receipt")));
			THROW_SL(RegistrationProc        = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_registration")));
			THROW_SL(CloseReceiptProc        = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_close_receipt")));
			THROW_SL(ApplySingleSettingsProc = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_apply_single_settings")));
			THROW_SL(OpenProc        = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_open")));
			THROW_SL(CloseProc       = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_close")));
			THROW_SL(IsOpenedProc    = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_is_opened")));
			THROW_SL(PrintBarcodeProc        = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_print_barcode")));
			THROW_SL(ReportProc      = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_report")));
			THROW_SL(CheckDocumentClosedProc = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_check_document_closed")));
			THROW_SL(GetErrorCodeProc        = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_error_code")));
			THROW_SL(CashIncomeProc  = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_cash_income")));
			THROW_SL(CashOutcomeProc = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_cash_outcome")));
			THROW_SL(GetSingleSettingProc    = reinterpret_cast<int (* cdecl)(void *, const wchar_t *, wchar_t *, int)>(Lib.GetProcAddr("libfptr_get_single_setting")));
			THROW_SL(SetSingleSettingProc    = reinterpret_cast<int (* cdecl)(void *, const wchar_t *, const wchar_t *)>(Lib.GetProcAddr("libfptr_set_single_setting")));
			THROW_SL(GetErrorDescrProc     = reinterpret_cast<int (* cdecl)(void *, wchar_t *, int)>(Lib.GetProcAddr("libfptr_error_description")));
			THROW_SL(SetParamBoolProc      = reinterpret_cast<int (* cdecl)(void *, int, int)>(Lib.GetProcAddr("libfptr_set_param_bool")));
			THROW_SL(SetParamIntProc       = reinterpret_cast<int (* cdecl)(void *, int, uint)>(Lib.GetProcAddr("libfptr_set_param_int")));
			THROW_SL(SetParamDoubleProc    = reinterpret_cast<int (* cdecl)(void *, int, double)>(Lib.GetProcAddr("libfptr_set_param_double")));
			THROW_SL(SetParamStrProc       = reinterpret_cast<int (* cdecl)(void *, int, const wchar_t *)>(Lib.GetProcAddr("libfptr_set_param_str")));
			THROW_SL(SetParamDatetimeProc  = reinterpret_cast<int (* cdecl)(void *, int, int, int, int, int, int, int)>(Lib.GetProcAddr("libfptr_set_param_datetime")));
			THROW_SL(SetParamByteArrayProc = reinterpret_cast<int (* cdecl)(void *, int, const uchar *, int)>(Lib.GetProcAddr("libfptr_set_param_bytearray")));
			SetNonPrintableParamByteArrayProc = reinterpret_cast<int (* cdecl)(void *, int, const uchar *, int)>(Lib.GetProcAddr("libfptr_set_non_printable_param_bytearray"));
			THROW_SL(GetParamBoolProc      = reinterpret_cast<int (* cdecl)(void *, int)>(Lib.GetProcAddr("libfptr_get_param_bool")));
			THROW_SL(GetParamIntProc       = reinterpret_cast<uint (* cdecl)(void *, int)>(Lib.GetProcAddr("libfptr_get_param_int")));
			THROW_SL(GetParamDoubleProc    = reinterpret_cast<double (* cdecl)(void *, int)>(Lib.GetProcAddr("libfptr_get_param_double")));
			THROW_SL(GetParamStrProc       = reinterpret_cast<int (* cdecl)(void *, int, wchar_t *, int)>(Lib.GetProcAddr("libfptr_get_param_str")));
			THROW_SL(GetParamDateTimeProc  = reinterpret_cast<void (* cdecl)(void *, int, int*, int*, int*, int*, int*, int*)>(Lib.GetProcAddr("libfptr_get_param_datetime")));
			THROW_SL(GetParamByteArrayProc = reinterpret_cast<int (* cdecl)(void *, int, uchar *, int)>(Lib.GetProcAddr("libfptr_get_param_bytearray")));
			THROW_SL(ProcessJsonProc = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_process_json"))); // @v11.2.8
			THROW_SL(OpenShiftProc = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_open_shift"))); // @v11.3.12
			UtilFormNomenclature = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_util_form_nomenclature"));
			UtilFormTlvProc      = reinterpret_cast<int (* cdecl)(void *)>(Lib.GetProcAddr("libfptr_util_form_tlv")); // @v11.0.0
			THROW(CreateHandleProc(&Handler) == 0 && Handler);
			{
				SString work_path;
				SStringU temp_buf_u;
				PPGetPath(PPPATH_BIN, work_path);
				//work_path.SetLastSlash().Cat("ATOL");
				temp_buf_u.CopyFromMb_OUTER(work_path, work_path.Len());
				SetSingleSettingProc(Handler, LIBFPTR_SETTING_LIBRARY_PATH, temp_buf_u);
				ApplySingleSettingsProc(Handler);
			}
			State |= stInitialized;
			CATCH
			ENDCATCH
		}
		~AtolFptr10()
		{
			if(DestroyHandleProc && Handler)
				DestroyHandleProc(&Handler);
		}
		bool   IsValid() const { return LOGIC(State & stInitialized); }
		//
		void * Handler;
		int (* cdecl CreateHandleProc)(void **);
		int (* cdecl DestroyHandleProc)(void **);
		const char * (* cdecl GetVersionString)();
		//DTOX_SHARED_EXPORT const char * DTOX_SHARED_CCA libfptr_get_version_string();
		int (* cdecl GetSettingsProc)(void *, wchar_t *, int);
		int (* cdecl GetSingleSettingProc)(void *, const wchar_t *, wchar_t *, int);
		int (* cdecl SetSingleSettingProc)(void *, const wchar_t *, const wchar_t *);
		int (* cdecl ApplySingleSettingsProc)(void *);
		int (* cdecl GetErrorCodeProc)(void *);
		int (* cdecl GetErrorDescrProc)(void *, wchar_t *, int);
		int (* cdecl OpenProc)(void *);  // "libfptr_open")
		int (* cdecl CloseProc)(void *); // "libfptr_close"
		int (* cdecl IsOpenedProc)(void *); // "libfptr_is_opened"
		int (* cdecl SetParamBoolProc)(void *, int paramId, int value);
		int (* cdecl SetParamIntProc)(void *, int paramId, uint value);
		int (* cdecl SetParamDoubleProc)(void *, int paramId, double value);
		int (* cdecl SetParamStrProc)(void *, int paramId, const wchar_t * pValue);
		int (* cdecl SetParamDatetimeProc)(void *, int paramId, int yr, int mon, int day, int hr, int mn, int sc);
		int (* cdecl SetParamByteArrayProc)(void *, int paramId, const uchar * pValue, int size);
		int (* cdecl SetNonPrintableParamByteArrayProc)(void *, int paramId, const uchar * pValue, int size); // @v11.0.0
		int (* cdecl PrintTextProc)(void *);
		int (* cdecl PaymentProc)(void *);
		int (* cdecl CutProc)(void *);
		int (* cdecl BeepProc)(void *);
		int (* cdecl OpenDrawerProc)(void *);
		int (* cdecl QueryDataProc)(void *);
		int (* cdecl GetParamBoolProc)(void *, int);
		uint   (* cdecl GetParamIntProc)(void *, int);
		int    (* cdecl GetParamStrProc)(void *, int, wchar_t * pValue, int size);
		double (* cdecl GetParamDoubleProc)(void *, int);
		void   (* cdecl GetParamDateTimeProc)(void *, int, int * pYr, int * pMon, int * pDay, int * pHr, int * pMn, int * pSc);
		int    (* cdecl GetParamByteArrayProc)(void *, int, uchar * pValue, int size);
		int    (* cdecl UtilFormNomenclature)(void *); //DTOX_SHARED_EXPORT int DTOX_SHARED_CCA libfptr_util_form_nomenclature(libfptr_handle handle);
		int    (* cdecl UtilFormTlvProc)(void *); // @v11.0.0 //DTOX_SHARED_EXPORT int DTOX_SHARED_CCA libfptr_util_form_tlv(libfptr_handle handle);
		int    (* cdecl OperatorLoginProc)(void *);
		int    (* cdecl OpenReceiptProc)(void *);
		int    (* cdecl CancelReceiptProc)(void *);
		int    (* cdecl CloseReceiptProc)(void *);
		int    (* cdecl RegistrationProc)(void *);
		int    (* cdecl PrintBarcodeProc)(void *);
		int    (* cdecl ReportProc)(void *);
		int    (* cdecl CheckDocumentClosedProc)(void *);
		int    (* cdecl CashIncomeProc)(void *);
		int    (* cdecl CashOutcomeProc)(void *);
		int    (* cdecl ProcessJsonProc)(void *); // @v11.2.8 libfptr_process_json(libfptr_handle handle);
		int    (* cdecl OpenShiftProc)(void *); // @v11.3.12 int libfptr_open_shift(libfptr_handle handle);

		SString AccessPassword;
		SString UserPassword;
	private:
		long   State;
		SDynLibrary Lib;
	};
	int  CallJsonProc(const SJson * pJs, SString & rResultJsonBuf)
	{
		rResultJsonBuf.Z();
		int    ok = 1;
		THROW(pJs);
		THROW(P_Fptr10 && P_Fptr10->ProcessJsonProc);
		{
			void * h = P_Fptr10->Handler;
			STempBuffer ret_buf(SKILOBYTE(1));
			SStringU temp_buf_u;
			SString temp_buf;
			SString log_buf;
			THROW_SL(ret_buf.IsValid());
			THROW_SL(pJs->ToStr(temp_buf_u));
			// @v11.5.6 {
			if(Flags & sfLogging) {
				temp_buf.CopyUtf8FromUnicode(temp_buf_u, temp_buf_u.Len(), 0);
				log_buf.Z().Cat("json-req").CatDiv(':', 2).Cat(temp_buf);
				PPLogMessage(PPFILNAM_ATOLDRV_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
			// } @v11.5.6 
			P_Fptr10->SetParamStrProc(h, LIBFPTR_PARAM_JSON_DATA, temp_buf_u.ucptr());
			if(P_Fptr10->ProcessJsonProc(h) != 0) {
				SetErrorMessage();
				CALLEXCEPT();
			}
			int ret_size = P_Fptr10->GetParamStrProc(h, LIBFPTR_PARAM_JSON_DATA, static_cast<wchar_t *>(ret_buf.vptr()), ret_buf.GetSize());
			if(ret_size > static_cast<int>(ret_buf.GetSize())) {
				THROW_SL(ret_buf.Alloc(ret_size+128));
				ret_size = P_Fptr10->GetParamStrProc(h, LIBFPTR_PARAM_JSON_DATA, static_cast<wchar_t *>(ret_buf.vptr()), ret_buf.GetSize());
			}
			if(ret_size > 0) {
				temp_buf_u.CopyFromN(static_cast<wchar_t *>(ret_buf.vptr()), ret_size);
				temp_buf_u.CopyToUtf8(rResultJsonBuf, 0);
				// @v11.5.6 {
				if(Flags & sfLogging) {
					log_buf.Z().Cat("json-rep").CatDiv(':', 2).Cat(rResultJsonBuf);
					PPLogMessage(PPFILNAM_ATOLDRV_LOG, log_buf, LOGMSGF_TIME|LOGMSGF_COMP);
				}
				// } @v11.5.6 
			}
		}
		CATCHZOK
		return ok;
	}
	int  ReadSettingsBulk(SString & rJsonBuf); // handler
	int  WriteSettingsBulk(const SString & rJsonBuf); // handler
	int  Connect(StateBlock * pStB)
	{
		int    ok = 1;
		bool   enabled = false;
		SString temp_buf;
		SString param_buf;
		PPIniFile ini_file;
		THROW_PP(ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_SHTRIHFRPASSWORD, param_buf) > 0, PPERR_SHTRIHFRADMPASSW);
		param_buf.Divide(',', CashierPassword, temp_buf);
		AdmPassword = CashierPassword;
		// @v11.5.6 {
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_POSREGISTERLOGGING, temp_buf.Z()) > 0) {
			if(temp_buf == "0" || temp_buf.IsEqiAscii("false") || temp_buf.IsEqiAscii("no"))
				Flags &= ~sfLogging;
			else if(temp_buf == "1" || temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes"))
				Flags |= sfLogging;
		}
		// } @v11.5.6
		// @v11.5.7 {
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_ATOLDRIVER_JSON, temp_buf.Z()) > 0) {
			if(temp_buf == "0" || temp_buf.IsEqiAscii("false") || temp_buf.IsEqiAscii("no"))
				Flags &= ~sfUseJson;
			else if(temp_buf == "1" || temp_buf.IsEqiAscii("true") || temp_buf.IsEqiAscii("yes"))
				Flags |= sfUseJson;
		}
		// } @v11.5.7
		if(P_Fptr10) {
			void * h = P_Fptr10->Handler;
			SStringU temp_buf_u;
			//libfptr_set_single_setting(LIBFPTR_SETTING_MODEL, std::to_wstring(LIBFPTR_MODEL_ATOL_AUTO).c_str());
			//libfptr_set_single_setting(LIBFPTR_SETTING_PORT, std::to_wstring(LIBFPTR_PORT_COM).c_str());
			//libfptr_set_single_setting(LIBFPTR_SETTING_COM_FILE, L"COM5");
			//libfptr_set_single_setting(LIBFPTR_SETTING_BAUDRATE, LIBFPTR_PORT_BR_115200);
			//libfptr_apply_single_settings(fptr);
			temp_buf.Z().Cat(LIBFPTR_MODEL_ATOL_AUTO);
			temp_buf_u.CopyFromMb_OUTER(temp_buf, temp_buf.Len());
			P_Fptr10->SetSingleSettingProc(h, LIBFPTR_SETTING_MODEL, temp_buf_u);
			temp_buf.Z().Cat(LIBFPTR_PORT_COM);
			temp_buf_u.CopyFromMb_OUTER(temp_buf, temp_buf.Len());
			P_Fptr10->SetSingleSettingProc(h, LIBFPTR_SETTING_PORT, temp_buf_u);
			{
				temp_buf.Z().Cat(Port).Strip();
				if(temp_buf.IsDec())
					temp_buf.Z().Cat("com").Cat(Port);
				temp_buf_u.CopyFromMb_OUTER(temp_buf, temp_buf.Len());
				P_Fptr10->SetSingleSettingProc(h, LIBFPTR_SETTING_COM_FILE, temp_buf_u);
			}
			P_Fptr10->ApplySingleSettingsProc(h);
			THROW(P_Fptr10->OpenProc(h) == 0);
			ok = 1;
		}
		else if(P_Disp) {
			GetProp(DeviceEnabled, &enabled);
			if(enabled == false) {
				THROW(SetProp(CurrentDeviceNumber, atol(Port)) > 0);
				THROW(SetProp(DeviceEnabled, true) > 0);
				THROW(SetProp(Password, CashierPassword));
				THROW(Annulate(MODE_REGISTER));
			}
		}
		if(pStB)
			GetState(*pStB);
		CATCH
			SetErrorMessage();
			ok = 0;
		ENDCATCH
		return ok;
	}
	int  GetState(StateBlock & rBlk)
	{
		int    ok = 0;
		rBlk.Flags = 0;
		if(P_Fptr10) {
			void * h = P_Fptr10->Handler;
			P_Fptr10->SetParamIntProc(h, LIBFPTR_PARAM_DATA_TYPE, LIBFPTR_DT_STATUS);
			P_Fptr10->QueryDataProc(h);
			rBlk.OperatorId = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_OPERATOR_ID);
			rBlk.LogicalNumber = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_LOGICAL_NUMBER);
			rBlk.DocNumber = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_DOCUMENT_NUMBER);
			rBlk.ReceiptNumber = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_RECEIPT_NUMBER);
			rBlk.ReceiptType = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_RECEIPT_TYPE);
			rBlk.ReceiptSum = P_Fptr10->GetParamDoubleProc(h, LIBFPTR_PARAM_RECEIPT_SUM);
			rBlk.ShiftState = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_SHIFT_STATE);
			rBlk.ShiftNumber = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_SHIFT_NUMBER);
			rBlk.LineLen = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_RECEIPT_LINE_LENGTH);
			CheckStrLen = rBlk.LineLen;
			rBlk.LineLenPix = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_RECEIPT_LINE_LENGTH_PIX);
			rBlk.Model = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_MODEL);
			rBlk.Mode = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_MODE);
			rBlk.SubMode = P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_SUBMODE);
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_FISCAL))
				rBlk.Flags |= rBlk.fFiscalDevice;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_FN_FISCAL))
				rBlk.Flags |= rBlk.fFiscalFN;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_FN_PRESENT))
				rBlk.Flags |= rBlk.fFNPresent;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_CASHDRAWER_OPENED))
				rBlk.Flags |= rBlk.fDrawerOpened;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_RECEIPT_PAPER_PRESENT))
				rBlk.Flags |= rBlk.fPaperPresent;
			//if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_PAPER_NEAR_END)) ;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_COVER_OPENED))
				rBlk.Flags |= rBlk.fCoverOpened;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_PRINTER_CONNECTION_LOST))
				rBlk.Flags |= rBlk.fPrnConnLost;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_PRINTER_ERROR))
				rBlk.Flags |= rBlk.fPrnError;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_CUT_ERROR))
				rBlk.Flags |= rBlk.fCutError;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_PRINTER_OVERHEAT))
				rBlk.Flags |= rBlk.fPrnOverheat;
			if(P_Fptr10->GetParamIntProc(h, LIBFPTR_PARAM_BLOCKED))
				rBlk.Flags |= rBlk.fDvcBlocked;
			//
			//libfptr_set_param_int(fptr, LIBFPTR_PARAM_DATA_TYPE, LIBFPTR_DT_REVENUE);
			//libfptr_query_data(fptr);
			//double revenue = libfptr_get_param_double(fptr, LIBFPTR_PARAM_SUM);
			P_Fptr10->SetParamIntProc(h, LIBFPTR_PARAM_DATA_TYPE, LIBFPTR_DT_CASH_SUM);
			P_Fptr10->QueryDataProc(h);
			rBlk.Summator = P_Fptr10->GetParamDoubleProc(h, LIBFPTR_PARAM_SUM);
			ok = 1;
		}
		else if(P_Disp) {
			int    int_val = 0;
			long   lng_val = 0;
			double real_val = 0.0;
			THROW(Exec(GetStatus));
			THROW(GetProp(DrawerOpened, &int_val));
			if(int_val)
				rBlk.Flags |= rBlk.fDrawerOpened;
			THROW(GetProp(CharLineLength, &lng_val));
			rBlk.LineLen = lng_val;
			CheckStrLen = lng_val;
			THROW(GetProp(CheckNumber, &int_val));
			rBlk.ReceiptNumber = static_cast<uint>(int_val);
			THROW(ExecOper(GetSumm));
			THROW(GetProp(Summ, &real_val));
			rBlk.Summator = real_val;
		}
		CATCHZOK
		return ok;
	}
	enum {
		ptfWrap         = 0x0001,
		ptfDoubleWidth  = 0x0002,
		ptfDoubleHeight = 0x0004
	};
	int  PrintText(const char * pText, long flags, int alignment/*ALIGN_XXX*/)
	{
		int    ok = 0;
		if(P_Fptr10) {
			void * fph = P_Fptr10->Handler;
			THROW(AllowPrintOper_Fptr10());
			SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
			r_temp_buf_u.CopyFromMb_OUTER(pText, sstrlen(pText));
			P_Fptr10->SetParamStrProc(fph, LIBFPTR_PARAM_TEXT, r_temp_buf_u);
			if(alignment) {
				if(alignment == ALIGN_LEFT) 
					P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_ALIGNMENT, LIBFPTR_ALIGNMENT_LEFT);
				else if(alignment == ALIGN_RIGHT)
					P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_ALIGNMENT, LIBFPTR_ALIGNMENT_RIGHT);
				else if(alignment == ALIGN_CENTER)
					P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_ALIGNMENT, LIBFPTR_ALIGNMENT_CENTER);
			}
			if(flags & ptfWrap)
				P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_TEXT_WRAP, LIBFPTR_TW_WORDS);
			if(flags & ptfDoubleWidth)
				P_Fptr10->SetParamBoolProc(fph, LIBFPTR_PARAM_FONT_DOUBLE_WIDTH, 1);
			if(flags & ptfDoubleHeight)
				P_Fptr10->SetParamBoolProc(fph, LIBFPTR_PARAM_FONT_DOUBLE_HEIGHT, 1);
			THROW(P_Fptr10->PrintTextProc(fph) == 0);
			ok = 1;
		}
		else if(P_Disp) {
			SetProp(Caption, pText);
			if(flags & ptfWrap)
				SetProp(TextWrap, 1L);
			THROW(ExecOper(PrintString));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	int    RegisterPayment(double amount, int paymType)
	{
		int    ok = 0;
		if(P_Fptr10) {
			void * fph = P_Fptr10->Handler;
			P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_PAYMENT_TYPE, paymType);
			P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_PAYMENT_SUM, amount);
			THROW(P_Fptr10->PaymentProc(fph) == 0);
			ok = 1;
		}
		else if(P_Disp) {
			const int atol_paytype_cash = 0;
			const int atol_paytype_bank = (SCn.DrvVerMajor > 8 || (SCn.DrvVerMajor == 8 && SCn.DrvVerMinor >= 15)) ? 1 : 3;
			int local_paym_type = 0;
			if(paymType == LIBFPTR_PT_CASH)
				local_paym_type = atol_paytype_cash;
			else if(paymType == LIBFPTR_PT_ELECTRONICALLY)
				local_paym_type = atol_paytype_bank;
			else
				local_paym_type = atol_paytype_cash; // @default
			THROW(SetProp(Summ, amount));
			THROW(SetProp(TypeClose, paymType));
			THROW(ExecOper(Payment));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	void   CutPaper(int partial)
	{
		if(P_Fptr10) {
			P_Fptr10->SetParamIntProc(P_Fptr10->Handler, LIBFPTR_PARAM_CUT_TYPE, partial ? LIBFPTR_CT_PART : LIBFPTR_CT_FULL);
			P_Fptr10->CutProc(P_Fptr10->Handler);
		}
		else if(P_Disp) {
			ExecOper(FullCut);
		}
	}
	void   DoBeep()
	{
		if(P_Fptr10) {
			P_Fptr10->BeepProc(P_Fptr10->Handler);
		}
		else if(P_Disp) {
			Exec(Beep);
		}
	}
	int  CheckForCash(double sum)
	{
		double cash_sum = 0.0;
		return GetSummator(&cash_sum) ? ((cash_sum < sum) ? -1 : 1) : 0;
	}
	int  Annulate(long mode)
	{
		int    ok = 1;
		if(P_Fptr10) {
			P_Fptr10->CancelReceiptProc(P_Fptr10->Handler);
		}
		else if(P_Disp) {
			long   check_state = 0L;
			THROW(GetProp(CheckState, &check_state));
			if(check_state != 0) {
				if(mode)
					THROW(SetProp(Mode, mode));
				THROW(ExecOper(SetMode));
				THROW(ExecOper(CancelCheck) > 0);
				if(mode)
					THROW(ExecOper(ResetMode));
				CutPaper(0);
			}
		}
		CATCHZOK
		return ok;
	}
	void WriteLogFile(PPID id);
	void CutLongTail(SString & rBuf) const { rBuf.Trim(CheckStrLen).TrimRightChr(' '); }
	int  AllowPrintOper(uint id);
	int  AllowPrintOper_Fptr10();
	int  PrintDiscountInfo(const CCheckPacket * pPack, uint flags);
	int  PrintReport(int withCleaning);
	//int  GetCheckInfo(PPBillPacket * pPack, BillTaxArray * pAry, long * pFlags, SString &rName);
	ComDispInterface * InitDisp();
	int  SetErrorMessage();
	int  SetProp(uint propID, bool propValue);
	int  SetProp(uint propID, int  propValue);
	int  SetProp(uint propID, long propValue);
	int  SetProp(uint propID, double propValue);
	int  SetProp(uint propID, const char * pPropValue);
	int  GetProp(uint propID, bool  * pPropValue);
	int  GetProp(uint propID, int   * pPropValue);
	int  GetProp(uint propID, long  * pPropValue);
	int  GetProp(uint propID, double  * pPropValue);
	int  Exec(uint id);
	int  ExecOper(uint id);
	enum {
		ShowProperties, // Методы
		OpenCheck,
		CloseCheck,
		CancelCheck,
		SetMode,
		ResetMode,
		PrintString,
		PrintHeader,
		Registration,
		Return,
		NewDocument,
		FullCut,
		GetSumm,
		Beep,
		Payment,
		GetCurrentMode,
		GetStatus,
		CashIncome,
		CashOutcome,
		OpenDrawer,
		Report,
		BeginDocument,
		EndDocument,
		SlipDocCharLineLength,
		SlipDocTopMargin,
		SlipDocLeftMargin,
		SlipDocOrientation,
		Mode,           // Параметры
		AdvancedMode,
		CheckNumber,
		Caption,
		Quantity,
		Price,
		Department,
		Name,
		CharLineLength,
		TextWrap,
		CheckType,
		TestMode,
		ResultCode,
		ResultDescription,
		CurrentDeviceNumber,
		CheckPaperPresent,
		ControlPaperPresent,
		Password,
		DeviceEnabled,
		Summ,
		CheckState,
		TypeClose,
		OutOfPaper,
		ECRError,
		DrawerOpened,
		ReportType,
		DocNumber,
		PointPosition,
		PrintBarcode,
		Barcode,
		BarcodeType,
		Height,
		PrintBarcodeText,
		AutoSize,
		Alignment,
		Scale,
		PrintPurpose,
		BarcodeControlCode,
		TaxTypeNumber,      // 1..5
		OperatorName,       // Имя кассира 
	};

	enum AtolDrvFlags {
		sfConnected     = 0x0001,     // установлена связь с устройством, COM-порт занят
		sfCheckOpened   = 0x0002,     // чек открыт
		sfCancelled     = 0x0004,     // операция печати чека прервана пользователем
		sfPrintSlip     = 0x0008,     // печать подкладного документа
		sfDontUseCutter = 0x0010,     // не использовать отрезчик чеков
		sfUseWghtSensor = 0x0020,     // использовать весовой датчик
		sfLogging       = 0x0040,     // @v11.5.6 Вести отладочный журнал операций
		sfUseJson       = 0x0080,     // @v11.5.7 Применять команды в json-формате (устанавливается в ответ на параметр pp.ini [config] PPINIPARAM_ATOLDRIVER_JSON (AtolDriverJson)
	};

	static ComDispInterface * P_Disp;
	static AtolFptr10 * P_Fptr10;
	static int RefToIntrf;
	long   ResCode;
	long   ErrCode;
	long   CheckStrLen;
	long   Flags;
	SString CashierPassword;
	SString AdmPassword;
};

ComDispInterface * SCS_ATOLDRV::P_Disp = 0; // @global
SCS_ATOLDRV::AtolFptr10 * SCS_ATOLDRV::P_Fptr10 = 0; // @global
int  SCS_ATOLDRV::RefToIntrf = 0; // @global

class CM_ATOLDRV : public PPCashMachine {
public:
	CM_ATOLDRV(PPID cashID) : PPCashMachine(cashID) 
	{
	}
	virtual PPSyncCashSession * SyncInterface()
	{
		PPSyncCashSession * cs = IsValid() ? new SCS_ATOLDRV(NodeID, NodeRec.Name, NodeRec.Port) : 0;
		CALLPTRMEMB(cs, Init(NodeRec.Name, NodeRec.Port));
		return cs;
	}
};

REGISTER_CMT(ATOLDRV, true, false);

SCS_ATOLDRV::SCS_ATOLDRV(PPID n, char * name, char * port) : 
	PPSyncCashSession(n, name, port), Flags(0), ResCode(RESCODE_NO_ERROR), ErrCode(0), CheckStrLen(0)
{
	SString temp_buf;
	if(!P_Fptr10) {
		P_Fptr10 = new AtolFptr10();
		if(P_Fptr10 && P_Fptr10->IsValid()) {
			SString settings_buf;
			{
				temp_buf.Z().Cat("atol-driver").CatDiv(':', 2).Cat("mode").Space().Cat("fptr10");
				PPLogMessage(PPFILNAM_ATOLDRV_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
			ReadSettingsBulk(settings_buf);
			if(settings_buf.NotEmpty()) {
				temp_buf.Z().Cat("atol-driver-settings").CatDiv(':', 2).Cat(settings_buf);
				PPLogMessage(PPFILNAM_ATOLDRV_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
			{
				SJson * p_json_doc = SJson::Parse(settings_buf);
				if(p_json_doc) {
					SJson * p_next = 0;
					for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_next) {
						p_next = p_cur->P_Next;
						switch(p_cur->Type) {
							case SJson::tARRAY: p_next = p_cur->P_Child; break;
							case SJson::tOBJECT: p_next = p_cur->P_Child; break;
							case SJson::tSTRING:
								if(sstreqi_ascii(p_cur->Text, "AccessPassword"))
									SJson::GetChildTextUnescaped(p_cur, P_Fptr10->AccessPassword);
								else if(sstreqi_ascii(p_cur->Text, "UserPassword"))
									SJson::GetChildTextUnescaped(p_cur, P_Fptr10->UserPassword);
								break;
						}
					}
				}
				ZDELETE(p_json_doc);
			}
		}
		else {
			ZDELETE(P_Fptr10);
		}
	}
	if(P_Fptr10) {
		RefToIntrf++;
	}
	else {
		SETIFZ(P_Disp, InitDisp());
		if(P_Disp) {
			RefToIntrf++;
			{
				temp_buf.Z().Cat("atol-driver").CatDiv(':', 2).Cat("mode").Space().Cat("dispatch-interface");
				PPLogMessage(PPFILNAM_ATOLDRV_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_COMP);
			}
		}
	}
	if(SCn.Flags & CASHF_NOTUSECHECKCUTTER)
		Flags |= sfDontUseCutter;
}

SCS_ATOLDRV::~SCS_ATOLDRV()
{
	if(RefToIntrf > 0) {
		RefToIntrf--;
		if(RefToIntrf == 0) {
			ZDELETE(P_Disp);
			ZDELETE(P_Fptr10);
		}
	}
}

/*virtual*/int SCS_ATOLDRV::PreprocessChZnCode(int op, const char * pCode, double qtty, int uomId, uint uomFragm, CCheckPacket::PreprocessChZnCodeResult & rResult) // @v11.2.12
{
	if(op == ppchzcopCheck) { // Другие операции этот блок менять не должны!
		rResult.Z();
	}
	int    ok = 1;
	int    mcv_status = -1;
	SString temp_buf;
	StateBlock stb;
	THROW(Connect(&stb));
	if(P_Fptr10 && P_Fptr10->IsValid() && P_Fptr10->ProcessJsonProc) {
		if(op == ppchzcopInit) {
			{
				SJson  js(SJson::tOBJECT);
				js.InsertString("type", "cancelMarkingCodeValidation");
				THROW(CallJsonProc(&js, temp_buf));
			}
			{
				SJson  js(SJson::tOBJECT);
				js.InsertString("type", "clearMarkingCodeValidationResult");
				THROW(CallJsonProc(&js, temp_buf));
			}
		}
		else if(op == ppchzcopCheck) {
			{
				SJson  js(SJson::tOBJECT);
				js.InsertString("type", "beginMarkingCodeValidation");
				{
					SJson * p_js_params = SJson::CreateObj();
					p_js_params->InsertString("imcType", "auto"/*"imcShort"*/);
					temp_buf.Z().EncodeMime64(pCode, sstrlen(pCode));
					p_js_params->InsertString("imc", temp_buf);
					// itemPieceSold itemDryForSale 
					p_js_params->InsertString("itemEstimatedStatus", "itemDryForSale");
					p_js_params->InsertDouble("itemQuantity", qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
					p_js_params->InsertString("itemUnits", "piece");
					p_js_params->InsertInt("imcModeProcessing", 0);
					// @v11.8.12 {
					if(qtty > 0.0 && qtty < 1.0 && uomFragm > 0) {
						double ip = 0.0;
						double nmrtr = 0.0;
						double dnmntr = 0.0;
						if(fsplitintofractions(qtty, uomFragm, 1E-5, &ip, &nmrtr, &dnmntr)) {
							//CreateStr(SLS.AcquireRvlStr().Cat(R0i(nmrtr)).Slash().Cat(R0i(dnmntr)), in_data);
							temp_buf.Z().Cat(R0i(nmrtr)).Slash().Cat(R0i(dnmntr));
							p_js_params->InsertString("itemFractionalAmount", temp_buf);
						}
					}
					// } @v11.8.12 
					js.Insert("params", p_js_params);
				}
				THROW(CallJsonProc(&js, temp_buf));
			}
			//
			{
				const uint max_tries = 5;
				const long _timeout = 500;
				uint  try_no = 0;
				do {
					if(try_no)
						SDelay(_timeout);
					SJson js(SJson::tOBJECT);
					js.InsertString("type", "getMarkingCodeValidationStatus");
					THROW(CallJsonProc(&js, temp_buf));
					{
						int    err_code = -1;
						SString err_text;
						SString err_description;
						SString mark_operator_item_status;
						SString mark_operator_response_result;
						SString imc_type;
						SString imc_barcode;
						int    imc_mode_processing = 0;
						bool   sent_imc_request = false;
						SJson * p_js_reply = SJson::Parse(temp_buf);
						if(p_js_reply) {
							if(p_js_reply->IsObject()) {
								for(const SJson * p_itm = p_js_reply->P_Child; p_itm; p_itm = p_itm->P_Next) {
									if(p_itm->Text.IsEqiAscii("driverError") && SJson::IsObject(p_itm->P_Child)) {
										for(const SJson * p_dei = p_itm->P_Child->P_Child; p_dei; p_dei = p_dei->P_Next) {
											if(p_dei->Text.IsEqiAscii("code")) {
												err_code = p_dei->P_Child->Text.ToLong();
											}
											else if(p_dei->Text.IsEqiAscii("error"))
												SJson::GetChildTextUnescaped(p_dei, err_text);
											else if(p_dei->Text.IsEqiAscii("description"))
												SJson::GetChildTextUnescaped(p_dei, err_description);
										}
									}
									else if(p_itm->Text.IsEqiAscii("onlineValidation") && SJson::IsObject(p_itm->P_Child)) {
										CCheckPacket::PreprocessChZnCodeResult ttt; // @debug
										for(const SJson * p_olv = p_itm->P_Child->P_Child; p_olv; p_olv = p_olv->P_Next) {
											if(p_olv->Text.IsEqiAscii("itemInfoCheckResult") && SJson::IsObject(p_olv->P_Child)) { // object tag 2106
												for(const SJson * p_icr = p_olv->P_Child->P_Child; p_icr; p_icr = p_icr->P_Next) {
													if(p_icr->Text.IsEqiAscii("imcCheckFlag")) {
														if(SJson::GetBoolean(p_icr->P_Child) == 1)
															rResult.CheckResult |= (1 << 0);
													}
													else if(p_icr->Text.IsEqiAscii("imcCheckResult")) {
														if(SJson::GetBoolean(p_icr->P_Child) == 1)
															rResult.CheckResult |= (1 << 1);
													}
													else if(p_icr->Text.IsEqiAscii("imcStatusInfo")) {
														if(SJson::GetBoolean(p_icr->P_Child) == 1)
															rResult.CheckResult |= (1 << 2);
													}
													else if(p_icr->Text.IsEqiAscii("imcEstimatedStatusCorrect")) {
														if(SJson::GetBoolean(p_icr->P_Child) == 1)
															rResult.CheckResult |= (1 << 3);
													}
													else if(p_icr->Text.IsEqiAscii("ecrStandAloneFlag")) {
														if(SJson::GetBoolean(p_icr->P_Child) == 1)
															rResult.CheckResult |= (1 << 4);
													}
												}
											}
											else if(p_olv->Text.IsEqiAscii("markOperatorItemStatus")) { // tag 2109
												if(p_olv->P_Child) {
													if(p_olv->P_Child->Text.IsEqiAscii("itemEstimatedStatusCorrect"))
														rResult.Status = 1;
													if(p_olv->P_Child->Text.IsEqiAscii("itemEstimatedStatusIncorrect"))
														rResult.Status = 2;
													if(p_olv->P_Child->Text.IsEqiAscii("itemSaleStopped"))
														rResult.Status = 3;
												}
											}
											else if(p_olv->Text.IsEqiAscii("markOperatorResponse") && SJson::IsObject(p_olv->P_Child)) { // object tag 2005
												rResult.ProcessingResult |= (1 << 0); // Зарезервирован - 1
												rResult.ProcessingResult |= (1 << 2); // Зарезервирован - 1
												for(const SJson * p_mor = p_olv->P_Child->P_Child; p_mor; p_mor = p_mor->P_Next) {
													if(p_mor->Text.IsEqiAscii("responseStatus")) {
														if(SJson::GetBoolean(p_mor->P_Child) == 1)
															rResult.ProcessingResult |= (1 << 1);
													}
													else if(p_mor->Text.IsEqiAscii("itemStatusCheck")) {
														if(SJson::GetBoolean(p_mor->P_Child) == 1)
															rResult.ProcessingResult |= (1 << 3);
													}
												}
											}
											else if(p_olv->Text.IsEqiAscii("markOperatorResponseResult")) { // tag 2105
												if(p_olv->P_Child) {
													if(p_olv->P_Child->Text.IsEqiAscii("correct"))
														rResult.ProcessingCode = 0;
													else if(p_olv->P_Child->Text.IsEqiAscii("incorrect"))
														rResult.ProcessingCode = 1;
													else if(p_olv->P_Child->Text.IsEqiAscii("unrecognized"))
														rResult.ProcessingCode = 2;
												}
											}
											else if(p_olv->Text.IsEqiAscii("imcType")) { // tag 2100
												SJson::GetChildTextUnescaped(p_olv, imc_type);
											}
											else if(p_olv->Text.IsEqiAscii("imcBarcode")) { // tag 2101
												SJson::GetChildTextUnescaped(p_olv, imc_barcode);
											}
											else if(p_olv->Text.IsEqiAscii("imcModeProcessing")) { // tag 2102
												imc_mode_processing = p_olv->P_Child ? p_olv->P_Child->Text.ToLong() : 0;
											}
										}									
									}
									else if(p_itm->Text.IsEqiAscii("sentImcRequest")) {
										int b = SJson::GetBoolean(p_itm->P_Child);
										if(b > 0)
											sent_imc_request = true;
									}
									else if(p_itm->Text.IsEqiAscii("ready")) {
										int b = SJson::GetBoolean(p_itm->P_Child);
										if(b > 0)
											mcv_status = 1;					
									}
								}
							}
							if(mcv_status > 0) {
								;
							}
							ZDELETE(p_js_reply);
						}
					}
					try_no++;
				} while(mcv_status < 0 && try_no < max_tries);
			}
		}
		else if(op == ppchzcopAccept) {
			SJson  js(SJson::tOBJECT);
			js.InsertString("type", "acceptMarkingCode");
			THROW(CallJsonProc(&js, temp_buf));			
		}
		else if(op == ppchzcopReject) {
			SJson  js(SJson::tOBJECT);
			js.InsertString("type", "declineMarkingCode");
			THROW(CallJsonProc(&js, temp_buf));			
		}
	}
	CATCHZOK
	return ok;
}

int SCS_ATOLDRV::ReadSettingsBulk(SString & rJsonBuf)
{
	int    ok = 0;
	rJsonBuf.Z();
	if(P_Fptr10 && P_Fptr10->IsValid()) {
		STempBuffer temp_buf(SKILOBYTE(4));
		THROW_SL(temp_buf.IsValid());
		wchar_t * p_buf = reinterpret_cast<wchar_t *>(static_cast<char *>(temp_buf));
		int result_len = P_Fptr10->GetSettingsProc(P_Fptr10->Handler, p_buf, static_cast<int>(temp_buf.GetSize()));
		if(result_len > 0) {
			rJsonBuf.CopyUtf8FromUnicode(p_buf, sstrlen(p_buf), 1);
			ok = 1;
		}
		else {
			// @error
		}
	}
	CATCHZOK
	return ok;
}

int SCS_ATOLDRV::WriteSettingsBulk(const SString & rJsonBuf)
{
	int    ok = 0;
	if(P_Fptr10 && P_Fptr10->IsValid()) {
	}
	return ok;
}

ComDispInterface * SCS_ATOLDRV::InitDisp()
{
	int    r = 0;
	ComDispInterface * p_disp = 0;
	THROW_MEM(p_disp = new ComDispInterface);
	static const char * drv_name_list[] = { "AddIn.FprnM45", "AddIn.Fptr10" /*, "Addin.DrvFR3", "Addin.DrvFR", "Addin.DrvFR1C17", "Addin.DrvFR1C"*/ };
	int   init_drv_result = 0;
	for(uint i = 0; !init_drv_result && i < SIZEOFARRAY(drv_name_list); i++) {
		if(p_disp->Init(drv_name_list[i])) {
			init_drv_result = i+1;
		}
	}
	//THROW(p_disp->Init("AddIn.FprnM45"));
	if(init_drv_result) {
		THROW(ASSIGN_ID_BY_NAME(p_disp, ShowProperties) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, OpenCheck) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CloseCheck) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CancelCheck) > 0);
		//THROW(ASSIGN_ID_BY_NAME(p_disp, set_Mode) > 0);
		//THROW(ASSIGN_ID_BY_NAME(p_disp, get_Mode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, SetMode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, ResetMode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, PrintString) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, PrintHeader) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Registration) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Return) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, NewDocument) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, FullCut) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, GetSumm) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Beep) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Payment) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, GetCurrentMode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, GetStatus) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CashIncome) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CashOutcome) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, OpenDrawer) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Report) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, BeginDocument) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, EndDocument) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocCharLineLength) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocTopMargin) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocLeftMargin) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, SlipDocOrientation) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, PrintBarcode) > 0);
		// Параметры
		THROW(ASSIGN_ID_BY_NAME(p_disp, Mode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, AdvancedMode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CheckNumber) > 0);
		//THROW(ASSIGN_ID_BY_NAME(p_disp, CheckNumber) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Caption) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Quantity) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Price) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Department) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Name) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Caption) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CharLineLength) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, TextWrap) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CheckType) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, TestMode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CheckPaperPresent) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, ControlPaperPresent) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, ResultCode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, ResultDescription) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CurrentDeviceNumber) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Password) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, DeviceEnabled) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Summ) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, CheckState) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, TypeClose) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, OutOfPaper) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, ECRError) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, DrawerOpened) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, ReportType) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Barcode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, BarcodeType) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Height) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, PrintBarcodeText) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, AutoSize) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Alignment) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, Scale) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, PrintPurpose) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, BarcodeControlCode) > 0);
		THROW(ASSIGN_ID_BY_NAME(p_disp, TaxTypeNumber) > 0); // 1..5
		THROW(ASSIGN_ID_BY_NAME(p_disp, OperatorName) > 0);
	}
	CATCH
		ZDELETE(p_disp);
	ENDCATCH
	return p_disp;
}

/*virtual*/int SCS_ATOLDRV::EditParam(void * pDevNum)
{
	int    ok = 1;
	long   dev_num = pDevNum ? *static_cast<const long *>(pDevNum) : 1;
	THROW_INVARG(P_Disp);
	THROW(P_Disp->SetProperty(CurrentDeviceNumber, dev_num) > 0);
	THROW(P_Disp->CallMethod(ShowProperties) > 0);
	THROW(P_Disp->GetProperty(CurrentDeviceNumber, &dev_num) > 0);
	ASSIGN_PTR(static_cast<long *>(pDevNum), dev_num);
	CATCH
		SetErrorMessage();
		ok = 0;
	ENDCATCH
	return ok;
}

int SCS_ATOLDRV::SetErrorMessage()
{
	int    ok = -1;
	THROW_INVARG(P_Fptr10 || P_Disp);
	if(P_Fptr10) {
		int    err_code = P_Fptr10->GetErrorCodeProc(P_Fptr10->Handler);
		if(err_code != LIBFPTR_OK) {
			SString err_buf;
			STempBuffer temp_buf(SKILOBYTE(2));
			err_buf.Z().Cat(err_code);
			THROW_SL(temp_buf.IsValid());
			wchar_t * p_buf = reinterpret_cast<wchar_t *>(static_cast<char *>(temp_buf));
			int result_len = P_Fptr10->GetErrorDescrProc(P_Fptr10->Handler, p_buf, static_cast<int>(temp_buf.GetSize()));
			if(result_len > 0) {
				err_buf.CopyUtf8FromUnicode(p_buf, sstrlen(p_buf), 1);
				err_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			PPSetError(PPERR_ATOL_DRV, err_buf);
			ok = 1;
		}
	}
	else if(P_Disp) {
		char   err_buf[MAX_PATH];
		memzero(err_buf, sizeof(err_buf));
		THROW(P_Disp->GetProperty(ResultCode, &ResCode) > 0);
		if(ResCode != RESCODE_NO_ERROR) {
			THROW(P_Disp->GetProperty(ResultDescription, err_buf, sizeof(err_buf) - 1) > 0);
			SCharToOem(err_buf);
			PPSetError(PPERR_ATOL_DRV);
			PPSetAddedMsgString(err_buf);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SCS_ATOLDRV::SetProp(uint propID, bool propValue) { return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::SetProp(uint propID, int propValue) { return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::SetProp(uint propID, long propValue) { return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::SetProp(uint propID, double propValue) { return BIN(P_Disp && P_Disp->SetProperty(propID, propValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::SetProp(uint propID, const char * pPropValue) { return BIN(P_Disp && P_Disp->SetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::GetProp(uint propID, bool  * pPropValue) { return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::GetProp(uint propID, int * pPropValue) { return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::GetProp(uint propID, long * pPropValue) { return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1); }
int SCS_ATOLDRV::GetProp(uint propID, double * pPropValue) { return BIN(P_Disp && P_Disp->GetProperty(propID, pPropValue) > 0 && SetErrorMessage() == -1); }

int	SCS_ATOLDRV::PrintDiscountInfo(const CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	double amt = R2(fabs(MONEYTOLDBL(pPack->Rec.Amount)));
	double dscnt = R2(MONEYTOLDBL(pPack->Rec.Discount));
	if(flags & PRNCHK_RETURN)
		dscnt = -dscnt;
	if(dscnt > 0.0) {
		const double pcnt = round(dscnt * 100.0 / (amt + dscnt), 1);
		SString prn_str;
		SString temp_str;
		SCardCore scc;
		THROW(PrintText(prn_str.Z().CatCharN('-', CheckStrLen), 0, 0));
		temp_str.Z().Cat(amt + dscnt, SFMT_MONEY);
		PPLoadText(PPTXT_CCFMT_AMTWODISCOUNT, prn_str);
		prn_str.Transf(CTRANSF_INNER_TO_OUTER);
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_str.Len()).Cat(temp_str);
		THROW(PrintText(prn_str, 0, 0));
		if(scc.Search(pPack->Rec.SCardID, 0) > 0) {
			PPLoadText(PPTXT_CCFMT_CARD, prn_str);
			prn_str.Transf(CTRANSF_INNER_TO_OUTER);
			THROW(PrintText((prn_str/*= "КАРТА"*/).Space().Cat(scc.data.Code), 0, 0));
			if(scc.data.PersonID && GetPersonName(scc.data.PersonID, temp_str) > 0) {
				PPLoadText(PPTXT_CCFMT_CARDOWNER, prn_str);
				prn_str.Transf(CTRANSF_INNER_TO_OUTER);
				(prn_str/*= "ВЛАДЕЛЕЦ"*/).Space().Cat(temp_str.Transf(CTRANSF_INNER_TO_OUTER));
				CutLongTail(prn_str);
				THROW(PrintText(prn_str, 0, 0));
			}
		}
		temp_str.Z().Cat(dscnt, SFMT_MONEY);
		PPLoadText(PPTXT_CCFMT_DISCOUNT, prn_str);
		prn_str.Transf(CTRANSF_INNER_TO_OUTER);
		(prn_str/*= "СКИДКА"*/).Space().Cat(pcnt, MKSFMTD(0, (flags & PRNCHK_ROUNDINT) ? 0 : 1, NMBF_NOTRAILZ)).CatChar('%');
		prn_str.CatCharN(' ', CheckStrLen - prn_str.Len() - temp_str.Len()).Cat(temp_str);
		THROW(PrintText(prn_str, 0, 0));
	}
	CATCHZOK
	return ok;
}

void SCS_ATOLDRV::WriteLogFile(PPID id)
{
	if(P_Disp && (CConfig.Flags & CCFLG_DEBUG)) {
		long   mode = 0, adv_mode = 0;
		size_t pos = 0;
		SString msg_fmt;
		SString msg;
		SString oper_name;
		SString mode_descr;
		SString err_msg = DS.GetConstTLA().AddedMsgString;
		PPLoadText(PPTXT_LOG_SHTRIH, msg_fmt);
		P_Disp->GetNameByID(id, oper_name);
		GetProp(Mode, &mode);
		mode_descr.Cat(mode);
		mode_descr.ToOem();
		if(err_msg.SearchChar('\n', &pos))
			err_msg.Trim(pos);
		GetProp(AdvancedMode, &adv_mode);
		msg.Printf(msg_fmt, oper_name.cptr(), err_msg.cptr(), mode_descr.cptr(), adv_mode);
		PPLogMessage(PPFILNAM_ATOLDRV_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
	}
}

static int FASTCALL IsModeOffPrint(int mode)
	{ return BIN(oneof4(mode, MODE_REGISTER, MODE_XREPORT, MODE_ZREPORT, MODE_EKLZ_REPORT)); }

int SCS_ATOLDRV::AllowPrintOper_Fptr10()
{
	int    ok = 1;
	if(P_Fptr10) {
		do {
			StateBlock stb;
			GetState(stb);
			if(!(stb.Flags & stb.fPaperPresent)) {
				if(PPMessage(mfConf|mfYesNo, PPCFM_SETPAPERTOPRINT) == cmYes)
					ok = -1;
				else
					ok = 0;
			}
			else if(stb.Flags & stb.fCoverOpened) {
				if(PPMessage(mfConf|mfYesNo, PPCFM_CLOSEPRNCOVERTOPRINT) == cmYes)
					ok = -1;
				else
					ok = 0;
			}
			else
				ok = 1;
		} while(ok < 0);
	}
	return ok;
}

int SCS_ATOLDRV::AllowPrintOper(uint id)
{
	//
	// Функция AllowPrintOper разбирается со всеми ситуациями,
	//  которые могут возникнуть при печати чека.
	// Код возврата: 1 - операция печати разрешена, 0 - запрещена.
	//
	int    ok = 1;
	bool   is_chk_rbn = true;
	bool   is_jrn_rbn = true;
	bool   out_paper = false;
	int    wait_prn_err = 0;
	int    continue_print = 0;
	long   mode = 0L;
	long   adv_mode = 0L;
	long   chk_state = CHKST_CLOSE;
	long   last_res_code = ResCode;
	SetErrorMessage();
	// Ожидание окончания операции печати
	do {
		THROW(Exec(GetStatus));
		THROW(GetProp(Mode, &mode));
		if(last_res_code == RESCODE_NOPAPER)
			wait_prn_err = 1;
	} while(oneof2(last_res_code, RESCODE_PRINTCONTROLTAPE, RESCODE_PRINTREPORT));
	{
		continue_print = BIN(last_res_code == RESCODE_NOPAPER);
		// На всякий случай помечаем, что чек открыт
		// (иначе при сбое операции открытия чека неизвестно: чек уже открыт или нет)
		THROW(GetProp(CheckState, &chk_state));
		if(chk_state != CHKST_CLOSE)
			Flags |= sfCheckOpened;
		// Ожидание заправки чековой ленты или выхода из режима, когда нельзя печатать чек
		while(ok && last_res_code == RESCODE_NOPAPER || (last_res_code == RESCODE_MODE_OFF && IsModeOffPrint(mode))) {
			int  send_msg = 0, r = 0;
			THROW(Exec(GetStatus));
			THROW(GetProp(CheckPaperPresent,   &is_chk_rbn));
			THROW(GetProp(ControlPaperPresent, &is_jrn_rbn));
			if(!is_chk_rbn) {
				PPSetError(PPERR_SYNCCASH_NO_CHK_RBN);
				send_msg = 1;
			}
			else if(!is_jrn_rbn) {
				PPSetError(PPERR_SYNCCASH_NO_JRN_RBN);
				send_msg = 1;
			}
			else
				WriteLogFile(id);
			//Exec(Beep);
			DoBeep();
			wait_prn_err = 1;
			r = PPError();
			if((!send_msg && r != cmOK) || (send_msg && /*Exec(Beep) &&*/ PPMessage(mfConf|mfYesNo, PPCFM_SETPAPERTOPRINT) != cmYes)) {
				Flags |= sfCancelled;
				ok = 0;
			}
			THROW(Exec(GetStatus));
			THROW(GetProp(ECRError, &last_res_code));
		}
		// Проверяем, надо ли завершить печать после заправки ленты
		if(continue_print) {
			WriteLogFile(id);
			THROW(ExecOper(id));
			THROW(Exec(GetStatus));
			THROW(GetProp(Mode,         &mode));
			THROW(GetProp(AdvancedMode, &adv_mode));
			wait_prn_err = 1;
		}
		//
		// Это, конечно, не отрывок из "Илиады", а очередная попытка
		// справиться с идиотскими ошибками, возникающими из-за этой дерьмовой ЭКЛЗ.
		//
		if(chk_state != CHKST_CLOSE && id == FullCut) {
			WriteLogFile(id);
			SString  err_msg(DS.GetConstTLA().AddedMsgString), added_msg;
			if(PPLoadText(PPTXT_APPEAL_CTO, added_msg))
				err_msg.CR().CatChar('\003').Cat(added_msg);
			PPSetAddedMsgString(err_msg);
			DoBeep();
			PPError();
			ok = -1;
		}
	}
	//
	// Если ситуация не связана непосредственно с процессом печати, выдаем сообщение об ошибке
	// При закрытии чека - сумма оплаты меньше суммы чека - не связано с процессом печати, но wait_prn_err == 1
	//
	if(!wait_prn_err || last_res_code == RESCODE_PAYM_LESS_SUM) {
		WriteLogFile(id);
		DoBeep();
		PPError();
		Flags |= sfCancelled;
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int SCS_ATOLDRV::Exec(uint id)
{
	int    ok = 1;
	THROW_INVARG(P_Disp);
	THROW(SetProp(Password, CashierPassword));
	THROW(P_Disp->CallMethod(id) > 0);
	THROW(SetErrorMessage() == -1);
	CATCHZOK
	return ok;
}

int SCS_ATOLDRV::ExecOper(uint id)
{
	int    ok = 1;
	THROW(P_Disp/* && P_Disp->SetProperty(Password, CashierPassword) > 0 && SetErrorMessage() == -1*/); // @debug
	do {
		THROW(P_Disp->CallMethod(id) > 0);
		THROW(P_Disp->GetProperty(ResultCode, &ResCode) > 0);
		if(ResCode == RESCODE_DVCCMDUNSUPP) {
			ok = -1;
			break;
		}
	} while(ResCode != RESCODE_NO_ERROR && (ok = AllowPrintOper(id)) > 0);
	CATCHZOK
	return ok;
}

/*
Чек коррекции:
Пример:

type": "sellCorrection",
	"taxationType": "osn",
	"electronically": true,
	"ignoreNonFiscalPrintErrors": false,
	"correctionType": "self",
	"correctionBaseDate": "2017.07.25",
	"correctionBaseNumber": "1175",
	"operator": {
		"name": "Иванов",
		"vatin": "123654789507"
	},
	"clientInfo": {
		"emailOrPhone": "+79161234567"
	},
	"items": [
		{
			"type": "position",
			"name": "Бананы",
			"price": 73.15,
			"quantity": 1.0,
			"amount": 73.15,
			"infoDiscountAmount": 0.0,
			"department": 1,
			"measurementUnit": "кг",
			"paymentMethod": "advance",
			"paymentObject": "commodity",
			"nomenclatureCode": "MTIzNDEyMzQ1Njc4MTIzNDU2Nzg5MDEyMzQ1Njc4OTA=",
			"tax": { "type": "vat20" },
			"agentInfo": {
				"agents": ["payingAgent", "bankPayingAgent"],
				"payingAgent": { "operation": "Оплата", "phones": ["+79161112233"] },
				"receivePaymentsOperator": { "phones": ["+79163331122"] },
				"moneyTransferOperator": {
					"phones": ["+79162223311"],
					"name": "Оператор перевода",
					"address": "Улица Оператора Перевода, д.1",
					"vatin": "321456987121"
				}
			},
			"supplierInfo": { "phones": ["+79175555555"], "name": "Поставщик", "vatin": "956839506500" }
		},
		{
			"type": "position",
			"name": "Шуба",
			"price": 51.25,
			"quantity": 2.0,
			"amount": 102.50,
			"department": 1,
			"paymentMethod": "fullPayment",
			"paymentObject": "commodity",
			"nomenclatureCode": "MTIzNDEyMzQ1Njc4MTIzNDU2Nzg5MDEyMzQ1Njc4OTA=",
			"tax": { "type": "vat10" }
		},
		{
			"type": "position",
			"name": "Пряники",
			"price": 64.25,
			"quantity": 2.0,
			"amount": 128.50,
			"infoDiscountAmount": 6.0,
			"department": 1,
			"measurementUnit": "шт.",
			"paymentMethod": "fullPrepayment",
			"paymentObject": "commodity",
			"tax": { "type": "vat10" }
		},
		{
			"type": "position",
			"name": "Батон нарезной",
			"price": 38.15,
			"quantity": 1.0,
			"amount": 38.15,
			"department": 1,
			"measurementUnit": "шт.",
			"paymentMethod": "fullPrepayment",
			"paymentObject": "commodity",
			"tax": { "type": "10" }
		},
		{
			"type": "position",
			"name": "Кефир",
			"price": 48.45,
			"quantity": 1.0,
			"amount": 48.45,
			"department": 1,
			"measurementUnit": "шт.",
			"paymentMethod": "fullPrepayment",
			"paymentObject": "excise",
			"tax": { "type": "vat0" }
		}
	],
	"payments": [ { "type": "electronically", "sum": 390.70 } ],
	"total": 390.7
}

При работе по ФФД ≥ 1.1 чеки коррекции имеют вид, аналогичный обычным чекам, но с добавлением информации о коррекции: тип, описание, дата документа основания и номер документа основания.
Описание задания Параметр 	Описание 	Тип 	Возможные значения 	Обязательность
type 	Тип задания 	string 	sellCorrection - чек коррекции прихода
buyCorrection - чек коррекции расхода
sellReturnCorrection - чек коррекции возврата прихода (ФФД ≥ 1.1)
buyReturnCorrection - чек коррекции возврата расхода (ФФД ≥ 1.1) 	+
ignoreNonFiscalPrintErrors 	Игнорировать ошибки при печати нефискальных элементов из items 	bool 		
operator 	Оператор (кассир) 	object 		
correctionType 	Тип коррекции (тег 1173) 	string 	self - самостоятельно
instruction - по предписанию 	
correctionBaseDate 	Дата совершения корректируемого расчета (тег 1178) 	string 	Формат - yyyy.mm.dd 	
correctionBaseNumber	Номер предписания налогового органа (тег 1179) 	string 		
electronically 	Электронный чек 	bool 	true - электронный чек
false - печатный чек 	по умолчанию - false
taxationType 	Система налогообложения 	string 	osn - общая
usnIncome - упрощенная (Доход)
usnIncomeOutcome - упрощенная (Доход минус Расход)
esn - единый сельскохозяйственный налог
patent - патентная 	можно не передавать, если ККТ зарегистрирована с единственной СНО
paymentsPlace 	Место проведения расчета (тег 1187) 	string 		
paymentsAddress 	Адрес расчётов (тег 1009) 	string 		+ для ФФД ≥ 1.2
machineNumber 	Номер автомата (тег 1036) 	string 		
clientInfo 	Данные покупателя 	object 		
companyInfo 	Данные продавца 	object 		
agentInfo 	Данные агента 	object 		
supplierInfo 	Данные поставщика 	object 		
items 	Элементы документа 	object[] 		+
payments 	Оплаты 	object[] 		+
taxes 	Налоги на чек 	object[] 		
total 	Итог чека 	number 	Может отличаться от суммы позиций на значение, равное копейкам чека.
Если не задан - высчитывается автоматически из суммы всех позиций	
preItems 	Элементы для печати до документа 	object[] 	Используются только элементы с типами:
text
barcode
pictureFromMemory
pixels 	
postItems 	Элементы для печати после документа 	object[] 	Используются только элементы с типами:
text
barcode
pictureFromMemory
pixels 	
customParameters 	Пользовательские параметры 	object[] 		

    Пример ответа на чек коррекции

{
  "fiscalParams" : {
     "fiscalDocumentDateTime" : "2018-03-06T13:52:00+03:00",
     "fiscalDocumentNumber" : 49,
     "fiscalDocumentSign" : "2403680979",
     "fiscalReceiptNumber" : 12,
     "fnNumber" : "9999078900000961",
     "registrationNumber" : "0000000001002292",
     "shiftNumber" : 11,
     "total" : 2000.0,
     "fnsUrl": "www.nalog.gov.ru"
  },
  "warnings": {
     "notPrinted": true
  }
}
*/ 

int SCS_ATOLDRV::PrintCheck(CCheckPacket * pPack, uint flags)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	/* @v11.5.2 
	#ifdef  NDEBUG
		bool   use_json_cmd = false; // @v11.3.5 @debug
	#else
		bool   use_json_cmd = true; // @v11.3.5 @debug
	#endif // ! NDEBUG
	*/
	const  bool   use_json_cmd = LOGIC(Flags & sfUseJson); // @v11.5.2
	bool   is_format = false;
	bool   enabled = true;
	int    jsproc_result = 0;
	SString temp_buf;
	SStringU temp_buf_u;
	SString buf;
	SString debug_log_buf;
	OfdFactors ofdf; // @v11.3.12
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR;
	void * fph = P_Fptr10 ? P_Fptr10->Handler : 0;
	THROW_INVARG(pPack);
	GetOfdFactors(ofdf); // @v11.3.12
	debug_log_buf.CatEq("CCID", pPack->Rec.ID).CatDiv(';', 2).CatEq("CCCode", pPack->Rec.Code);
	if(pPack->GetCount() == 0)
		ok = -1;
	else {
		PreprocessCCheckForOfd12(ofdf, pPack); // @v11.3.12
		if(use_json_cmd && !P_Disp && P_SlipFmt) {
			//
			// @v12.2.9 Формирование json чека в формате atol-driver перенесено в общий модуль PPSyncCashSession с целью тестирования.
			//
			SJson * p_js_cc = /*PPSyncCashSession::*/AtolDrv_MakeJson_CCheck(ofdf, pPack, P_SlipFmt, 0);
			if(p_js_cc) {
				jsproc_result = CallJsonProc(p_js_cc, temp_buf);
			}
		}
		else {
			SlipDocCommonParam sdc_param;
			double amt = fabs(R2(MONEYTOLDBL(pPack->Rec.Amount)));
			double sum = fabs(pPack->_Cash);
			double running_total = 0.0;
			SString operator_name;
			SString goods_name;
			SString chzn_sid; // @v11.0.0
			const  bool is_vat_free = (CnObj.IsVatFree(NodeID) > 0);
			StateBlock stb;
			double real_fiscal = 0.0;
			double real_nonfiscal = 0.0;
			pPack->HasNonFiscalAmount(&real_fiscal, &real_nonfiscal);
			const double _fiscal = (PPConst::Flags & PPConst::fDoSeparateNonFiscalCcItems) ? real_fiscal : (real_fiscal + real_nonfiscal);
			const CcAmountList & r_al = pPack->AL_Const();
			const bool   is_al = (r_al.getCount() > 0);
			const double amt_bnk = is_al ? r_al.Get(CCAMTTYP_BANK) : ((pPack->Rec.Flags & CCHKF_BANKING) ? _fiscal : 0.0);
			const double amt_cash = (PPConst::Flags & PPConst::fDoSeparateNonFiscalCcItems) ? (_fiscal - amt_bnk) : (is_al ? r_al.Get(CCAMTTYP_CASH) : (_fiscal - amt_bnk));
			const double amt_ccrd = is_al ? r_al.Get(CCAMTTYP_CRDCARD) : (real_fiscal + real_nonfiscal - _fiscal);
			// @v11.0.0 {
			if(SCn.LocID)
				PPRef->Ot.GetTagStr(PPOBJ_LOCATION, SCn.LocID, PPTAG_LOC_CHZNCODE, chzn_sid);
			// } @v11.0.0 
			THROW(Connect(&stb));
			THROW(AllowPrintOper_Fptr10());
			if(flags & PRNCHK_LASTCHKANNUL) {
				THROW(Annulate(MODE_REGISTER));
			}
			if(flags & PRNCHK_RETURN && !(flags & PRNCHK_BANKING)) {
				int is_cash = 0;
				THROW(is_cash = CheckForCash(amt));
				THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
			}
			//
			// Имя кассира
			//
			PPSyncCashSession::GetCurrentUserName(operator_name);
			operator_name.Transf(CTRANSF_INNER_TO_OUTER);
			if(P_Disp) {
				THROW(SetProp(OperatorName, operator_name));
				THROW(SetProp(Mode, MODE_REGISTER));
				THROW(ExecOper(NewDocument));
				//THROW(GetProp(CharLineLength, &CheckStrLen));
			}
			pPack->Rec.Code = static_cast<long>(stb.ReceiptNumber);
			if(P_SlipFmt) {
				bool   prn_total_sale = true;
				int    r = 0;
				SString line_buf;
				const SString format_name("CCheck");
				SlipLineParam sl_param;
				THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
				if(r > 0) {
					is_format = true;
					if(sdc_param.PageWidth > static_cast<uint>(CheckStrLen))
						WriteLogFile_PageWidthOver(format_name);
					if(P_Fptr10) {
						temp_buf_u.Z().CopyFromMb_OUTER(operator_name, operator_name.Len());
						P_Fptr10->SetParamStrProc(fph, 1021, temp_buf_u);
						//temp_buf_u.Z().CopyFromMb_OUTER(CashierPassword, CashierPassword.Len());
						temp_buf_u.Z().CopyFromMb_OUTER(P_Fptr10->UserPassword, P_Fptr10->UserPassword.Len());
						P_Fptr10->SetParamStrProc(fph, 1203, temp_buf_u);
						THROW(P_Fptr10->OperatorLoginProc(fph) == 0);
						// @v11.0.0 {
						// LIBFPTR_PARAM_TAG_VALUE LIBFPTR_PARAM_RECEIPT_TYPE
						if(chzn_sid.NotEmpty() && P_Fptr10->UtilFormTlvProc) {
							uint8  fptr10_mdlp_buf[512];
							int    mdlp_buf_len = 0;
							P_Fptr10->SetParamStrProc(fph, 1085, L"mdlp");
							temp_buf.Z().Cat("sid").Cat(chzn_sid).CatChar('&');
							temp_buf_u.CopyFromMb_OUTER(temp_buf, temp_buf.Len());
							P_Fptr10->SetParamStrProc(fph, 1086, temp_buf_u); // sid
							P_Fptr10->UtilFormTlvProc(fph);
											
							//uint8 fptr10_mdlp_buf[512];
							//int   mdlp_buf_len = 0;
							mdlp_buf_len = P_Fptr10->GetParamByteArrayProc(fph, LIBFPTR_PARAM_TAG_VALUE, fptr10_mdlp_buf, sizeof(fptr10_mdlp_buf));
							//libfptr_set_param_str(fptr, 1191, 'mdlp1/10&');
							//libfptr_set_param_str(fptr, 1085, "mdlp");
							//libfptr_set_param_str(fptr, 1086, 'sid717528521946&');
							if(mdlp_buf_len > 0 && P_Fptr10->SetNonPrintableParamByteArrayProc) {
								P_Fptr10->SetNonPrintableParamByteArrayProc(fph, 1084, fptr10_mdlp_buf, mdlp_buf_len);
							}
						}
						// } @v11.0.0 
						P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_RECEIPT_TYPE, (flags & PRNCHK_RETURN) ? LIBFPTR_RT_SELL_RETURN : LIBFPTR_RT_SELL);
						// @v11.3.10 {
						{
							bool paperless = false;
							SString buyers_email;
							SString buyers_phone;
							pPack->GetExtStrData(CCheckPacket::extssBuyerEMail, buyers_email);
							pPack->GetExtStrData(CCheckPacket::extssBuyerPhone, buyers_phone);
							if(buyers_email.NotEmpty()) {
								paperless = LOGIC(pPack->Rec.Flags & CCHKF_PAPERLESS);
								temp_buf_u.Z().CopyFromMb_INNER(buyers_email, buyers_email.Len());
								P_Fptr10->SetParamStrProc(fph, 1008, temp_buf_u);
							}
							else if(buyers_phone.NotEmpty()) {
								paperless = LOGIC(pPack->Rec.Flags & CCHKF_PAPERLESS);
								temp_buf_u.Z().CopyFromMb_INNER(buyers_phone, buyers_phone.Len());
								P_Fptr10->SetParamStrProc(fph, 1008, temp_buf_u);
							}
							else
								paperless = false;
							if(paperless) {
								P_Fptr10->SetParamBoolProc(fph, LIBFPTR_PARAM_RECEIPT_ELECTRONICALLY, paperless);
							}
						}
						// } @v11.3.10 
						THROW(P_Fptr10->OpenReceiptProc(fph) == 0);
					}
					else if(P_Disp) {
						THROW(SetProp(CheckType, (flags & PRNCHK_RETURN) ? 2L : 1L));
						THROW(ExecOper(OpenCheck));
					}
					Flags |= sfCheckOpened;
					debug_log_buf.Space().CatChar('{');
					for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
						if(sl_param.Flags & SlipLineParam::fRegFiscal) {
							const  double _q = sl_param.Qtty;
							const  double _p = fabs(sl_param.Price);
							int    tax_type_number = 1;
							goods_name.Z();
							{
								(goods_name = sl_param.Text).Strip();
								if(!goods_name.NotEmptyS())
									goods_name = "WARE";
								else
									goods_name.Transf(CTRANSF_INNER_TO_OUTER);
								goods_name.Trim(CheckStrLen);
							}
							running_total += (_q * _p);
							const double pq = R3(_q);
							const double pp = R2(_p);
							debug_log_buf.CatChar('[').CatEq("QTY", pq).Space().CatEq("PRICE", pp, MKSFMTD(0, 10, 0)).CatChar(']');
							if(P_Fptr10) {
								uint8 fptr10_mark_buf[512];
								int   mark_buf_data_len = 0;
								if(sl_param.ChZnProductType && sl_param.ChZnGTIN.NotEmpty() && sl_param.ChZnSerial.NotEmpty()) {
									{
										temp_buf.Z().Cat("chzn-mark").CatDiv(':', 2).Cat(sl_param.ChZnProductType).CatChar('-').Cat(sl_param.ChZnGTIN).CatChar('-').Cat(sl_param.ChZnSerial);
										PPLogMessage(PPFILNAM_ATOLDRV_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
									}
									if(P_Fptr10->UtilFormNomenclature) {
										int marking_type = -1;
										switch(sl_param.ChZnProductType) {
											case GTCHZNPT_FUR: marking_type = LIBFPTR_NT_FURS; break;
											case GTCHZNPT_TOBACCO: 
											case GTCHZNPT_ALTTOBACCO: // @v11.9.0
												marking_type = LIBFPTR_NT_TOBACCO; break;
											case GTCHZNPT_SHOE: 
												// @v12.0.6 marking_type = LIBFPTR_NT_SHOES; break;
												marking_type = 0x444D; break; // @v12.0.6
											case GTCHZNPT_MEDICINE: marking_type = LIBFPTR_NT_MEDICINES; break;
											case GTCHZNPT_CARTIRE: marking_type = 0x444D; break;
											case GTCHZNPT_TEXTILE: marking_type = 0x444D; break; // @v11.0.0
											case GTCHZNPT_PERFUMERY: marking_type = 0x444D; break; // @v11.3.12 Парфюмерия
											case GTCHZNPT_MILK: marking_type = 0x444D; break; // @v11.3.12 Молоко
											case GTCHZNPT_WATER: marking_type = 0x444D; break; // @v11.3.12 Вода
											case GTCHZNPT_BEER: marking_type = 0x444D; break; // @v12.0.3 Пиво
											case GTCHZNPT_ANTISEPTIC: marking_type = 0x444D; break; // @v12.0.5
											case GTCHZNPT_MEDICALDEVICES: marking_type = 0x444D; break; // @v12.1.2
											case GTCHZNPT_SOFTDRINKS: marking_type = 0x444D; break; // @v12.1.10
											case GTCHZNPT_NONALCBEER: marking_type = 0x444D; break; // @v12.2.6 
										}
										if(marking_type >= 0) {
											P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_NOMENCLATURE_TYPE, marking_type);
											temp_buf_u.CopyFromMb_OUTER(sl_param.ChZnGTIN, sl_param.ChZnGTIN.Len());
											P_Fptr10->SetParamStrProc(fph, LIBFPTR_PARAM_GTIN, temp_buf_u);
											temp_buf_u.CopyFromMb_OUTER(sl_param.ChZnSerial, sl_param.ChZnSerial.Len());
											P_Fptr10->SetParamStrProc(fph, LIBFPTR_PARAM_SERIAL_NUMBER, temp_buf_u);
											P_Fptr10->UtilFormNomenclature(fph);
											mark_buf_data_len = P_Fptr10->GetParamByteArrayProc(fph, LIBFPTR_PARAM_TAG_VALUE, fptr10_mark_buf, sizeof(fptr10_mark_buf));
											temp_buf.Z().Cat("chzn-mark-composed").CatDiv(':', 2).CatEq("length", static_cast<long>(mark_buf_data_len));
											PPLogMessage(PPFILNAM_ATOLDRV_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);										
											/*
											fptr.setParam(fptr.LIBFPTR_PARAM_NOMENCLATURE_TYPE, fptr.LIBFPTR_NT_TOBACCO);
											fptr.setParam(fptr.LIBFPTR_PARAM_GTIN, '98765432101234');
											fptr.setParam(fptr.LIBFPTR_PARAM_SERIAL_NUMBER, 'ABC1234');
											fptr.utilFormNomenclature;
											nomenclatureCode := fptr.getParamByteArray(fptr.LIBFPTR_PARAM_TAG_VALUE);
											fptr.setParam(fptr.LIBFPTR_PARAM_COMMODITY_NAME, 'товар1');
											fptr.setParam(1212, 5); // признак предмета расчета
											fptr.setParam(fptr.LIBFPTR_PARAM_PRICE, pr);
											fptr.setParam(fptr.LIBFPTR_PARAM_QUANTITY, 1);
											fptr.setParam(fptr.LIBFPTR_PARAM_TAX_TYPE, fptr.LIBFPTR_TAX_VAT0);
											fptr.setParam(fptr.LIBFPTR_PARAM_USE_ONLY_TAX_TYPE, True);
											fptr.setParam(1162, nomenclatureCode);
											fptr.registration; 
											*/
											/*
												1191 - propertiesItem
												1084 - properties 
												1085 - propertyName
												1086 - propertyValue
											*/
										}
									}
								}
								temp_buf_u.CopyFromMb_OUTER(goods_name, goods_name.Len());
								P_Fptr10->SetParamStrProc(fph, LIBFPTR_PARAM_COMMODITY_NAME, temp_buf_u);
								P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_PRICE, pp);
								P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_QUANTITY, pq);
								if(is_vat_free)
									tax_type_number = LIBFPTR_TAX_NO;
								else {
									const double vatrate = fabs(sl_param.VatRate);
									if(vatrate == 18.0)
										tax_type_number = LIBFPTR_TAX_VAT18;
									else if(vatrate == 20.0)
										tax_type_number = LIBFPTR_TAX_VAT20;
									else if(vatrate == 10.0)
										tax_type_number = LIBFPTR_TAX_VAT10;
									// @v12.2.3 {
									else if(vatrate == 7.0)
										tax_type_number = LIBFPTR_TAX_VAT7;
									else if(vatrate == 5.0)
										tax_type_number = LIBFPTR_TAX_VAT5;
									// } @v12.2.3 
									else if(vatrate == 0.0)
										tax_type_number = LIBFPTR_TAX_VAT0;
									else
										tax_type_number = LIBFPTR_TAX_VAT20; // @default
								}
								P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_TAX_TYPE, tax_type_number);
								P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_DEPARTMENT, (sl_param.DivID > 16 || sl_param.DivID < 0) ? 0 : sl_param.DivID);
								if(mark_buf_data_len > 0) {
									P_Fptr10->SetParamByteArrayProc(fph, 1162, fptr10_mark_buf, mark_buf_data_len);
								}
								// @v11.1.9 {
								if(sl_param.ChZnProductType == GTCHZNPT_MEDICINE) {
									SString tag_1191;
									tag_1191.Cat("mdlp");
									P_Fptr10->SetParamStrProc(fph, 1191, SUcSwitchW(tag_1191));
								}
								// } @v11.1.9
								THROW(P_Fptr10->RegistrationProc(fph) == 0);
							}
							else if(P_Disp) {
								THROW(SetProp(Name, goods_name)); // Наименование товара
								THROW(SetProp(Quantity, pq));
								THROW(SetProp(Price, pp));
								if(SCn.DrvVerMinor == 30) { 
									// 1 - 0%
									// 2 - 10%
									// 3 - 18%
									// 4 - 18/118 ?
									// 5 - 10/110
									// 6 - без НДС
									if(is_vat_free)
										tax_type_number = 6;
									else {
										const double vatrate = fabs(sl_param.VatRate);
										if(vatrate == 18.0 || vatrate == 20.0)
											tax_type_number = 3;
										else if(vatrate == 10.0)
											tax_type_number = 2;
										else if(vatrate == 0.0)
											tax_type_number = 1;
										else
											tax_type_number = 3; // @default
									}
								}
								else if((SCn.DrvVerMinor % 2) == 0) { 
									//
									// 1 - 18
									// 2 - 10
									// 3 - 0
									// 4 - без НДС
									//
									if(is_vat_free)
										tax_type_number = 4;
									else {
										const double vatrate = fabs(sl_param.VatRate);
										if(vatrate == 18.0 || vatrate == 20.0)
											tax_type_number = 1;
										else if(vatrate == 10.0)
											tax_type_number = 2;
										else if(vatrate == 0.0)
											tax_type_number = 3;
										else
											tax_type_number = 1; // @default
									}
								}
								else {
									//
									// 1 - 18
									// 2 - 10
									// 3 - 18/118
									// 4 - 10/110
									// 5 - 0
									// 6 - без НДС
									//
									if(is_vat_free)
										tax_type_number = 6;
									else {
										const double vatrate = fabs(sl_param.VatRate);
										if(vatrate == 18.0 || vatrate == 20.0)
											tax_type_number = 1;
										else if(vatrate == 10.0)
											tax_type_number = 2;
										else if(vatrate == 0.0)
											tax_type_number = 5;
										else
											tax_type_number = 1; // @default
									}
								}
								THROW(SetProp(TaxTypeNumber, tax_type_number));
								THROW(SetProp(Department, (sl_param.DivID > 16 || sl_param.DivID < 0) ? 0 : static_cast<int32>(sl_param.DivID)));
								THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
							}
							Flags |= sfCheckOpened;
							prn_total_sale = false;
						}
						else if(sl_param.Kind == sl_param.lkBarcode) {
							;
						}
						else if(sl_param.Kind == sl_param.lkSignBarcode) {
							if(line_buf.NotEmptyS()) {
								int    atol_bctype = 0;
								const  int barcode_height = (sl_param.BarcodeHt > 0) ? sl_param.BarcodeHt : 50;
								const  int barcode_scale = 300;
								if(P_Fptr10) {
									switch(sl_param.BarcodeStd) {
										case BARCSTD_CODE39: atol_bctype = LIBFPTR_BT_CODE_39; break;
										case BARCSTD_UPCA: atol_bctype = LIBFPTR_BT_UPC_A; break;
										case BARCSTD_UPCE: atol_bctype = LIBFPTR_BT_UPC_E; break;
										case BARCSTD_EAN13: atol_bctype = LIBFPTR_BT_EAN_13; break;
										case BARCSTD_EAN8: atol_bctype = LIBFPTR_BT_EAN_8; break;
										case BARCSTD_ANSI: atol_bctype = LIBFPTR_BT_CODABAR; break;
										case BARCSTD_CODE93: atol_bctype = LIBFPTR_BT_CODE_93; break;
										case BARCSTD_CODE128: atol_bctype = LIBFPTR_BT_CODE_128; break;
										case BARCSTD_PDF417: atol_bctype = LIBFPTR_BT_PDF417; break;
										case BARCSTD_QR: atol_bctype = LIBFPTR_BT_QR; break;
										default: atol_bctype = LIBFPTR_BT_QR; break; // QR by default
									}
									temp_buf_u.Z().CopyFromMb_OUTER(line_buf, line_buf.Len());
									P_Fptr10->SetParamStrProc(fph, LIBFPTR_PARAM_BARCODE, temp_buf_u);
									P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_BARCODE_TYPE, atol_bctype);
									P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_HEIGHT, barcode_height);
									P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_SCALE, barcode_scale);
									P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_ALIGNMENT, LIBFPTR_ALIGNMENT_CENTER);
									P_Fptr10->SetParamBoolProc(fph, LIBFPTR_PARAM_BARCODE_PRINT_TEXT, BIN(sl_param.Flags & (sl_param.fBcTextBelow|sl_param.fBcTextAbove)));
									P_Fptr10->PrintBarcodeProc(fph);
								}
								else if(P_Disp) {
									switch(sl_param.BarcodeStd) {
										case BARCSTD_CODE39: atol_bctype = 1; break;
										case BARCSTD_UPCA: atol_bctype = 0; break;
										case BARCSTD_UPCE: atol_bctype = 4; break;
										case BARCSTD_EAN13: atol_bctype = 2; break;
										case BARCSTD_EAN8: atol_bctype = 3; break;
										case BARCSTD_ANSI: atol_bctype = 6; break;
										case BARCSTD_CODE93: atol_bctype = 7; break;
										case BARCSTD_CODE128: atol_bctype = 8; break;
										case BARCSTD_PDF417: atol_bctype = 10; break;
										case BARCSTD_QR: atol_bctype = 84; break;
										default: atol_bctype = 84; break; // QR by default
									}
									THROW(SetProp(BarcodeType, atol_bctype));
									THROW(SetProp(Barcode, line_buf));
									THROW(SetProp(Height, barcode_height));
									if(sl_param.Flags & sl_param.fBcTextBelow) {
										THROW(SetProp(PrintBarcodeText, 2));
									}
									else if(sl_param.Flags & sl_param.fBcTextAbove) {
										THROW(SetProp(PrintBarcodeText, 1));
									}
									else {
										THROW(SetProp(PrintBarcodeText, 0));
									}
									THROW(SetProp(AutoSize, true));
									THROW(SetProp(Alignment, 1));
									THROW(SetProp(Scale, barcode_scale));
									THROW(SetProp(PrintPurpose, 1));
									THROW(SetProp(BarcodeControlCode, false));
									THROW(ExecOper(PrintBarcode));
								}
							}
						}
						else {
							THROW(PrintText(line_buf.Trim(CheckStrLen), ptfWrap, 0));
						}
					}
					if(prn_total_sale) {
						if(!pPack->GetCount()) {
							const double pq = 1.0;
							const double pp = fabs(amt);
							debug_log_buf.CatChar('[').CatEq("QTY", pq).Space().CatEq("PRICE", pp, MKSFMTD(0, 10, 0)).CatChar(']');
							if(P_Fptr10) {
								P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_PRICE, pp);
								P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_QUANTITY, pq);
								THROW(P_Fptr10->RegistrationProc(fph) == 0);
							}
							else if(P_Disp) {
								THROW(SetProp(Quantity, pq));
								THROW(SetProp(Price, pp));
								THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
							}
							Flags |= sfCheckOpened;
							running_total += amt;
						}
					}
					else if(running_total > amt) {
						SString fmt_buf, msg_buf, added_buf;
						added_buf.Z().Cat(running_total, MKSFMTD(0, 12, NMBF_NOTRAILZ)).CatChar('>').Cat(amt, MKSFMTD(0, 12, NMBF_NOTRAILZ));
						msg_buf.Printf(PPLoadTextS(PPTXT_SHTRIH_RUNNGTOTALGTAMT, fmt_buf), added_buf.cptr());
						PPLogMessage(PPFILNAM_ATOLDRV_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					debug_log_buf.CatChar('}');
				}
			}
			if(!is_format) {
				CCheckLineTbl::Rec ccl;
				buf.Z().CatCharN('-', CheckStrLen);
				for(uint pos = 0; pPack->EnumLines(&pos, &ccl);) {
					int  division = (ccl.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? ccl.DivID - CHECK_LINE_IS_PRINTED_BIAS : ccl.DivID;
					GetGoodsName(ccl.GoodsID, buf);
					buf.Strip().Transf(CTRANSF_INNER_TO_OUTER).Trim(CheckStrLen);
					const double pq = R3(fabs(ccl.Quantity));
					const double pp = R2(intmnytodbl(ccl.Price) - ccl.Dscnt);
					if(P_Fptr10) {
						temp_buf_u.CopyFromMb_OUTER(buf, buf.Len());
						P_Fptr10->SetParamStrProc(fph, LIBFPTR_PARAM_COMMODITY_NAME, temp_buf_u);
						P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_PRICE, pp);
						P_Fptr10->SetParamDoubleProc(fph, LIBFPTR_PARAM_QUANTITY, pq);
						P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_DEPARTMENT, (division > 16 || division < 0) ? 0 : division);
						THROW(P_Fptr10->RegistrationProc(fph) == 0);
					}
					else if(P_Disp) {
						THROW(SetProp(Name, buf));    // Наименование товара
						THROW(SetProp(Price, pp));    // Цена
						THROW(SetProp(Quantity, pq)); // Количество
						THROW(SetProp(Department, (division > 16 || division < 0) ? 0 : division));
						THROW(ExecOper((flags & PRNCHK_RETURN) ? Return : Registration));
					}
					Flags |= sfCheckOpened;
				}
				// Информация о скидке
				THROW(PrintDiscountInfo(pPack, flags));
				buf.Z().CatCharN('-', CheckStrLen);
				THROW(PrintText(buf.Trim(CheckStrLen), 0, 0));
			}
			debug_log_buf.Space().CatEq("SUM", sum, MKSFMTD(0, 10, 0)).Space().CatEq("RUNNINGTOTAL", running_total, MKSFMTD(0, 10, 0));
			if(!feqeps(running_total, sum, 1E-5))
				sum = running_total;
			{
				const double __amt_bnk = R2(fabs(amt_bnk));
				const double __amt_ccrd = R2(fabs(amt_ccrd));
				const double __amt_cash = R2(fabs(sum - __amt_bnk - __amt_ccrd));
				debug_log_buf.Space().CatEq("PAYMBANK", __amt_bnk, MKSFMTD(0, 10, 0)).
					Space().CatEq("PAYMCASH", __amt_cash, MKSFMTD(0, 10, 0)).
					Space().CatEq("PAYMCRDCARD", __amt_ccrd, MKSFMTD(0, 10, 0));
				if(__amt_cash > 0.0) {
					THROW(RegisterPayment(__amt_cash, LIBFPTR_PT_CASH));
				}
				if(__amt_bnk > 0.0) {
					THROW(RegisterPayment(__amt_bnk, LIBFPTR_PT_ELECTRONICALLY));
				}
				if(__amt_ccrd > 0.0) {
					THROW(RegisterPayment(__amt_ccrd, LIBFPTR_PT_OTHER));
				}
			}
			if(P_Fptr10) {
				THROW(AllowPrintOper_Fptr10());
				THROW(P_Fptr10->CloseReceiptProc(fph) == 0);
			}
			else if(P_Disp) {
				THROW(ExecOper(CloseCheck));
				THROW(Exec(ResetMode));
			}
			Flags &= ~sfCheckOpened;
			ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
			if(!(Flags & sfDontUseCutter)) {
				CutPaper(0);
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
				PPLogMessage(PPFILNAM_ATOLDRV_LOG, pPack ? CCheckCore::MakeCodeString(&pPack->Rec, 0, no_print_txt) : "", LOGMSGF_TIME|LOGMSGF_USER);
				ok = 0;
			}
		}
		else {
			SetErrorMessage();
			DoBeep();
			if(Flags & sfCheckOpened)
				ErrCode = SYNCPRN_ERROR_WHILE_PRINT;
			PPLogMessage(PPFILNAM_ATOLDRV_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			ok = 0;
		}
		if(P_Fptr10) {
			if(Flags & sfCheckOpened)
				P_Fptr10->CancelReceiptProc(fph);
		}
		else if(P_Disp) {
			if(Flags & sfCheckOpened)
				ExecOper(CancelCheck);
			Exec(ResetMode);
		}
	ENDCATCH
	if(CConfig.Flags & CCFLG_DEBUG && debug_log_buf.NotEmptyS()) {
		PPLogMessage(PPFILNAM_DEBUG_LOG, debug_log_buf, LOGMSGF_USER|LOGMSGF_TIME);
	}
	return ok;
}

/*virtual*/int SCS_ATOLDRV::PrintCheckCopy(const CCheckPacket * pPack, const char * pFormatName, uint flags)
{
	int     ok = 1;
	SlipDocCommonParam sdc_param;
	StateBlock stb;
	THROW_INVARG(pPack);
	THROW(Connect(&stb));
	if(P_Disp) {
		THROW(SetProp(Mode, MODE_REGISTER));
	}
	if(P_SlipFmt) {
		int   r = 0;
		SString  line_buf;
		const SString format_name((!isempty(pFormatName)) ? pFormatName : ((flags & PRNCHK_RETURN) ? "CCheckRetCopy" : "CCheckCopy"));
		SlipLineParam sl_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			for(P_SlipFmt->InitIteration(pPack); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				THROW(PrintText(line_buf.Trim(CheckStrLen), ptfWrap, 0));
			}
			if(P_Disp) {
				THROW(ExecOper(PrintHeader));
			}
			if(!(Flags & sfDontUseCutter)) {
				CutPaper(0);
			}
		}
	}
	ErrCode = SYNCPRN_NO_ERROR;
	CATCH
		if(Flags & sfCancelled)
			Flags &= ~sfCancelled;
		else {
			SetErrorMessage();
			DoBeep();
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int SCS_ATOLDRV::PrintReport(int withCleaning)
{
	int     ok = 1;
	int     mode = 0;
	SString cshr_pssw;
	SString operator_name;
	SString temp_buf;
	SStringU temp_buf_u;
	ResCode = RESCODE_NO_ERROR;
	StateBlock stb;
	THROW(Connect(&stb));
	THROW(AllowPrintOper_Fptr10());
	// Закрыть сессию можно только под паролем администратора
	cshr_pssw = CashierPassword;
	CashierPassword = AdmPassword;
	PPSyncCashSession::GetCurrentUserName(operator_name);
	operator_name.Transf(CTRANSF_INNER_TO_OUTER);
	//
	Flags |= sfCheckOpened;
	if(P_Fptr10) {
		void * fph = P_Fptr10->Handler;
		//libfptr_set_param_str(fptr, 1021, L"Кассир Иванов И.");
		//libfptr_set_param_str(fptr, 1203, L"123456789047");
		//libfptr_operator_login(fptr);
		//libfptr_set_param_int(fptr, LIBFPTR_PARAM_REPORT_TYPE, LIBFPTR_RT_CLOSE_SHIFT);
		//libfptr_report(fptr);
		//libfptr_check_document_closed();
		{
			temp_buf_u.Z().CopyFromMb_OUTER(operator_name, operator_name.Len());
			P_Fptr10->SetParamStrProc(fph, 1021, temp_buf_u);
			//temp_buf_u.Z().CopyFromMb_OUTER(CashierPassword, CashierPassword.Len());
			temp_buf_u.Z().CopyFromMb_OUTER(P_Fptr10->UserPassword, P_Fptr10->UserPassword.Len());
			P_Fptr10->SetParamStrProc(fph, 1203, temp_buf_u);
			THROW(P_Fptr10->OperatorLoginProc(fph) == 0);
		}
		P_Fptr10->SetParamIntProc(fph, LIBFPTR_PARAM_REPORT_TYPE, (withCleaning) ? LIBFPTR_RT_CLOSE_SHIFT : LIBFPTR_RT_X);
		THROW(P_Fptr10->ReportProc(fph) == 0);
		P_Fptr10->CheckDocumentClosedProc(fph);
	}
	else if(P_Disp) {
		THROW(SetProp(Mode, (withCleaning) ? MODE_ZREPORT : MODE_XREPORT));
		THROW(Exec(SetMode));
		THROW(SetProp(ReportType, (withCleaning) ? REPORT_TYPE_Z : REPORT_TYPE_X));
		THROW(ExecOper(Report));
	}
	if(!(Flags & sfDontUseCutter)) {
		CutPaper(0);
	}
	CATCH
		if(Flags & sfCancelled)
			Flags &= ~sfCancelled;
		else {
			SetErrorMessage();
			DoBeep();
		}
		ok = 0;
	ENDCATCH
	if(Flags & sfCheckOpened) {
		Flags &= ~sfCheckOpened;
		CashierPassword = cshr_pssw;
	}
	return ok;
}

int SCS_ATOLDRV::PrintZReportCopy(const CSessInfo * pInfo)
{
	int  ok = -1;
	ResCode = RESCODE_NO_ERROR;
	ErrCode = SYNCPRN_ERROR_AFTER_PRINT;
	StateBlock stb;
	THROW_INVARG(pInfo);
	THROW(Connect(&stb));
	THROW(AllowPrintOper_Fptr10());
	if(P_Disp) {
		THROW(SetProp(Mode, MODE_REGISTER));
		//THROW(GetProp(CharLineLength, &CheckStrLen));
	}
	if(P_SlipFmt) {
		int   r = 0;
		SString line_buf;
		SString format_name("ZReportCopy");
		SlipDocCommonParam sdc_param;
		THROW(r = P_SlipFmt->Init(format_name, &sdc_param));
		if(r > 0) {
			SlipLineParam sl_param;
			if(sdc_param.PageWidth > (uint)CheckStrLen)
				WriteLogFile_PageWidthOver(format_name);
			for(P_SlipFmt->InitIteration(pInfo); P_SlipFmt->NextIteration(line_buf, &sl_param) > 0;) {
				THROW(PrintText(line_buf.Trim(CheckStrLen), ptfWrap, 0));
			}
			if(P_Disp) {
				THROW(ExecOper(PrintHeader));
			}
			// THROW(Exec(EndDocument)); // Закончить и напечатать документ
			if(!(Flags & sfDontUseCutter)) {
				CutPaper(0);
			}
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
			DoBeep();
			ok = 0;
		}
	ENDCATCH
	return ok;
}

int SCS_ATOLDRV::PrintIncasso(double sum, int isIncome)
{
	int    ok = 1;
	StateBlock stb;
	ResCode = RESCODE_NO_ERROR;
	if(!isIncome) {
		int is_cash = 0;
		THROW(is_cash = CheckForCash(sum));
		THROW_PP(is_cash > 0, PPERR_SYNCCASH_NO_CASH);
	}
	THROW(Connect(&stb));
	THROW(AllowPrintOper_Fptr10());
	if(P_Fptr10) {
		P_Fptr10->SetParamDoubleProc(P_Fptr10->Handler, LIBFPTR_PARAM_SUM, R2(fabs(sum)));
		if(isIncome) {
			THROW(P_Fptr10->CashIncomeProc(P_Fptr10->Handler) == 0);
		}
		else {
			THROW(P_Fptr10->CashOutcomeProc(P_Fptr10->Handler) == 0);
		}
	}
	else if(P_Disp) {
		THROW(SetProp(Mode, MODE_REGISTER));
		THROW(ExecOper(SetMode));
		Flags |= sfCheckOpened;
		THROW(SetProp(Summ, sum));
		THROW(ExecOper(isIncome ? CashIncome : CashOutcome));
	}
	if(!(Flags & sfDontUseCutter)) {
		CutPaper(0);
	}
	CATCH
		if(Flags & sfCancelled) {
			Flags &= ~sfCancelled;
			ok = -1;
		}
		else {
			SetErrorMessage();
			DoBeep();
			ok = 0;
		}
	ENDCATCH
	if(P_Disp) {
		THROW(Exec(ResetMode));
	}
	Flags &= ~sfCheckOpened;
	return ok;
}

/*virtual*/int SCS_ATOLDRV::PrintBnkTermReport(const char * pZCheck)
{
	int    ok = 1;
	size_t zc_len = sstrlen(pZCheck);
	if(zc_len) {
		StateBlock stb;
		SEOLFormat eolf = SDetermineEOLFormat(pZCheck, zc_len);
		const char * p_delim = 0;
		switch(eolf) {
			case eolWindows: p_delim = "\xD\xA"; break;
			case eolUnix: p_delim = "\xA"; break;
			case eolMac: p_delim = "\xD"; break;
			default: p_delim = "\n"; break;
		}
		SString str;
		StringSet str_set(p_delim);
		str_set.setBuf(pZCheck, zc_len+1);
		THROW(Connect(&stb));
		THROW(AllowPrintOper_Fptr10());
		if(P_Disp) {
			SetProp(Mode, MODE_REGISTER);
		}
		{
			for(uint pos = 0; str_set.get(&pos, str);) {
				str.Chomp();
				THROW(PrintText(str, ptfWrap, 0));
				SDelay(10); // Иногда не удается распечатать слип. Гипотеза: драйвер не успевает обрабатывать быструю последовательность строк.
			}
			if(!(Flags & sfDontUseCutter))
				CutPaper(0);
			else
				SDelay(1000);
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_DBINFO|LOGMSGF_COMP);
		ok = 0;
	ENDCATCH
	return ok;
}
