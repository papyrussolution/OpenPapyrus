#ifndef PAYCARD_INTERFACE_C_H
#define PAYCARD_INTERFACE_C_H

#if defined(_WIN32) || defined(_WIN32_WCE)
#  if defined(DTO_LIBRARY)
#    define DTOSHARED_EXPORT __declspec(dllexport)
#  else
#    define DTOSHARED_EXPORT __declspec(dllimport)
#  endif
#  define DTOSHARED_CCA __cdecl
#elif defined(__linux__) || defined(ANDROID)
#  define DTOSHARED_EXPORT
#  define DTOSHARED_CCA
#else
#  define DTOSHARED_EXPORT
#  define DTOSHARED_CCA __attribute__ ((cdecl))
#endif

extern "C"
{

DTOSHARED_EXPORT int DTOSHARED_CCA get_LicenseValid(void *ptr, int *state);
DTOSHARED_EXPORT int DTOSHARED_CCA get_LicenseExpiredDate(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Version(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DriverName(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceEnabled(void *ptr, int *deviceEnabled);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceEnabled(void *ptr, int deviceEnabled);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ResultCode(void *ptr, int *resultCode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ResultDescription(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BadParam(void *ptr, int *badParam);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BadParamDescription(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ApplicationHandle(void *ptr, void **appHandle);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ApplicationHandle(void *ptr, void *appHandle);
DTOSHARED_EXPORT int DTOSHARED_CCA ShowProperties(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSettings(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSettings(void *ptr, const wchar_t *deviceSettings);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingAsBuff(void *ptr, const wchar_t *name, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSingleSettingAsBuff(void *ptr, const wchar_t *name, const wchar_t *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingAsInt(void *ptr, const wchar_t *name, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSingleSettingAsInt(void *ptr, const wchar_t *name, const int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingAsDouble(void *ptr, const wchar_t *name, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSingleSettingAsDouble(void *ptr, const wchar_t *name, const double value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingMapping(void *ptr, const wchar_t *name, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA ApplySingleSettings(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetSingleSettings(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA Reset(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Execute(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TerminalNumber(void *ptr, int *terminalNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TerminalNumber(void *ptr, int terminalNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_OperationType(void *ptr, int *operationType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_OperationType(void *ptr, int operationType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ServiceOperationType(void *ptr, int *serviceOperationType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ServiceOperationType(void *ptr, int serviceOperationType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_AuthorizationType(void *ptr, int *authorizationType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_AuthorizationType(void *ptr, int authorizationType);
DTOSHARED_EXPORT int DTOSHARED_CCA Authorization(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrepareAuthorization(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Sum(void *ptr, double *sum);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Sum(void *ptr, double sum);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Name(void *ptr, wchar_t *name, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Name(void *ptr, const wchar_t *name);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PhoneNumber(void *ptr, wchar_t *phoneNumber, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PhoneNumber(void *ptr, const wchar_t *phoneNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Account(void *ptr, wchar_t *account, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Account(void *ptr, const wchar_t *account);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CardNumber(void *ptr, wchar_t *cardNumber, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CardNumber(void *ptr, const wchar_t *cardNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CardType(void *ptr, wchar_t *cardType, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CardExpDate(void *ptr, wchar_t *cardExpDate, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CardExpDate(void *ptr, const wchar_t *cardExpDate);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DataTracks(void *ptr, wchar_t *dataTracks, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DataTracks(void *ptr, const wchar_t *dataTracks);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DataTrack2(void *ptr, wchar_t *dataTrack2, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DataTrack2(void *ptr, const wchar_t *dataTrack2);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CardHolderName(void *ptr, wchar_t *cardHolderName, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CardHolderName(void *ptr, const wchar_t *cardHolderName);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReferenceNumber(void *ptr, wchar_t *referenceNumber, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReferenceNumber(void *ptr, const wchar_t *referenceNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CharLineLength(void *ptr, int *charLineLength);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CharLineLength(void *ptr, int charLineLength);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ECRSessionNumber(void *ptr, int *ecrSessionNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ECRSessionNumber(void *ptr, int ecrSessionNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ECRReceiptNumber(void *ptr, int *ecrReceiptNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ECRReceiptNumber(void *ptr, int ecrReceiptNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Currency(void *ptr, wchar_t *currency, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Currency(void *ptr, const wchar_t *currency);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ResponseCode(void *ptr, int *responseCode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TransType(void *ptr, int *transType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TransDate(void *ptr, wchar_t *transDate, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TransTime(void *ptr, wchar_t *transTime, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TransID(void *ptr, wchar_t *transID, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_AuthCode(void *ptr, wchar_t *authCode, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_AuthCode(void *ptr, const wchar_t *authCode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TerminalID(void *ptr, wchar_t *terminalID, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_MsgNumber(void *ptr, int *msgNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_MessageType(void *ptr, wchar_t *messageType, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_MerchNumber(void *ptr, wchar_t *merchNumber, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_MerchCategoryCode(void *ptr, wchar_t *merchCategoryCode, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_MerchEngName(void *ptr, wchar_t *merchEngName, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Text(void *ptr, wchar_t *text, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Discount(void *ptr, double *discount);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Bonus(void *ptr, double *bonus);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Barcode(void *ptr, wchar_t *barcode, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Barcode(void *ptr, const wchar_t *barcode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Cashier(void *ptr, wchar_t *cashier, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Cashier(void *ptr, const wchar_t *cashier);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Mask(void *ptr, wchar_t *mask, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Commission(void *ptr, double *commission);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ServiceOperator(void *ptr, wchar_t *serviceOperator, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SumWareCode(void *ptr, wchar_t *sumWareCode, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CommissionWareCode(void *ptr, wchar_t *commissionWareCode, int bufferSize);

DTOSHARED_EXPORT int DTOSHARED_CCA get_SlipNumber(void *ptr, int *slipNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SlipNumber(void *ptr, int slipNumber);

DTOSHARED_EXPORT int DTOSHARED_CCA BeginReport(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetState(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA AddToReport(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EndReport(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportAuthCode(void *ptr, wchar_t *reportAuthCode, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportAuthCode(void *ptr, const wchar_t *reportAuthCode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportOperationType(void *ptr, int *reportOperationType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportOperationType(void *ptr, int reportOperationType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportTransType(void *ptr, int *reportTransType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportTransType(void *ptr, int reportTransType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportSum(void *ptr, double *reportSum);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportSum(void *ptr, double reportSum);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportCardNumber(void *ptr, wchar_t *reportCardNumber, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportCardNumber(void *ptr, const wchar_t *reportCardNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportCardExpDate(void *ptr, wchar_t *reportCardExpDate, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportCardExpDate(void *ptr, const wchar_t *reportCardExpDate);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportSlipNumber(void *ptr, int *reportSlipNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportSlipNumber(void *ptr, int reportSlipNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportTransDate(void *ptr, wchar_t *reportTransDate, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportTransDate(void *ptr, const wchar_t *reportTransDate);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportTransTime(void *ptr, wchar_t *reportTransTime, int BufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportTransTime(void *ptr, const wchar_t *reportTransTime);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportMsgNumber(void *ptr, int *reportMsgNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportMsgNumber(void *ptr, int reportMsgNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportTerminalID(void *ptr, wchar_t *reportTerminalID, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportTerminalID(void *ptr, const wchar_t *reportTerminalID);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportReferenceNumber(void *ptr, wchar_t *reportReferenceNumber, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportReferenceNumber(void *ptr, const wchar_t *reportReferenceNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportResponseCode(void *ptr, wchar_t *reportResponceCode, int bufferSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportResponseCode(void *ptr, const wchar_t *reportResponceCode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportType(void *ptr, int *reportType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportType(void *ptr, int reportType);

DTOSHARED_EXPORT int DTOSHARED_CCA put_PinPadDevice(void *ptr, void *cls);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ModemDevice(void *ptr, void *cls);

}

#endif
