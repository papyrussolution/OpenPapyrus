#ifndef FPTR_INTERFACE_C_H
#define FPTR_INTERFACE_C_H

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

typedef void (*ScanerEventHandlerFunc)(const unsigned char*, int);

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
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSettings(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSettings(void *ptr, const wchar_t *deviceSettings);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingAsBuff(void *ptr, const wchar_t *name, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSingleSettingAsBuff(void *ptr, const wchar_t *name, const wchar_t *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingAsInt(void *ptr, const wchar_t *name, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSingleSettingAsInt(void *ptr, const wchar_t *name, const int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingAsDouble(void *ptr, const wchar_t *name, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DeviceSingleSettingAsDouble(void *ptr, const wchar_t *name, const double value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceSingleSettingMapping(void *ptr, const wchar_t *name, wchar_t *bfr, int bfrSize);

DTOSHARED_EXPORT int DTOSHARED_CCA ShowProperties(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ApplySingleSettings(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetSingleSettings(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_Caption(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Caption(void *ptr, const wchar_t *caption);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CaptionPurpose(void *ptr, int *captionPurpose);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CaptionPurpose(void *ptr, int captionPurpose);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CaptionIsSupported(void *ptr, int *isSupported);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CaptionName(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Value(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Value(void *ptr, double value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ValuePurpose(void *ptr, int *valuePurpose);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ValuePurpose(void *ptr, int valuePurpose);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ValueIsSupported(void *ptr, int *isSupported);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ValueName(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ValueMapping(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CharLineLength(void *ptr, int *charLineLength);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SerialNumber(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SerialNumber(void *ptr, const wchar_t *serialNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Time(void *ptr, int *hours, int *minutes, int *seconds);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Time(void *ptr, int hours, int minutes, int seconds);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Date(void *ptr, int *day, int *month, int *year);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Date(void *ptr, int day, int month, int year);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Fiscal(void *ptr, int *fiscal);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TestMode(void *ptr, int *testMode);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TestMode(void *ptr, int testMode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_EnableCheckSumm(void *ptr, int *enableCheckSumm);
DTOSHARED_EXPORT int DTOSHARED_CCA put_EnableCheckSumm(void *ptr, int enableCheckSumm);
DTOSHARED_EXPORT int DTOSHARED_CCA get_UserPassword(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_UserPassword(void *ptr, const wchar_t *password);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Mode(void *ptr, int *mode);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Mode(void *ptr, int mode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Alignment(void *ptr, int *alignment);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Alignment(void *ptr, int alignment);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TextWrap(void *ptr, int *textWrap);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TextWrap(void *ptr, int textWrap);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Barcode(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Barcode(void *ptr, const wchar_t *barcode);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeType(void *ptr, int *barcodeType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeType(void *ptr, int barcodeType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrintBarcodeText(void *ptr, int *printBarcodeText);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PrintBarcodeText(void *ptr, int printBarcodeText);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SlipDocOrientation(void *ptr, int *slipDocOrientation);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SlipDocOrientation(void *ptr, int slipDocOrientation);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Scale(void *ptr, double *scale);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Scale(void *ptr, double scale);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Height(void *ptr, int *height);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Height(void *ptr, int height);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TypeClose(void *ptr, int *typeClose);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TypeClose(void *ptr, int typeClose);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Summ(void *ptr, double *summ);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Summ(void *ptr, double summ);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CheckType(void *ptr, int *checkType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CheckState(void *ptr, int *checkState);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CheckType(void *ptr, int chequeType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CheckNumber(void *ptr, int *chequeNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CheckNumber(void *ptr, int chequeNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_RegisterNumber(void *ptr, int *registerNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_RegisterNumber(void *ptr, int registerNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DocNumber(void *ptr, int *docNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DocNumber(void *ptr, int docNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SessionOpened(void *ptr, int *sessionOpened);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Session(void *ptr, int *session);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Session(void *ptr, int session);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CheckPaperPresent(void *ptr, int *checkPaperPresent);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ControlPaperPresent(void *ptr, int *controlPaperPresent);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PLUNumber(void *ptr, int *pluNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PLUNumber(void *ptr, int pluNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Name(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Name(void *ptr, const wchar_t *name);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Price(void *ptr, double *price);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Price(void *ptr, double price);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Quantity(void *ptr, double *quantity);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Quantity(void *ptr, double quantity);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Department(void *ptr, int *department);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Department(void *ptr, int department);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DiscountType(void *ptr, int *discountType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DiscountType(void *ptr, int discountType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReportType(void *ptr, int *reportType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReportType(void *ptr, int reportType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BufferedPrint(void *ptr, int *bufferedPrint);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BufferedPrint(void *ptr, int bufferedPrint);
DTOSHARED_EXPORT int DTOSHARED_CCA get_InfoLine(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Model(void *ptr, int *model);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ClearFlag(void *ptr, int *flag);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ClearFlag(void *ptr, int flag);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FileName(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FileName(void *ptr, const wchar_t *name);
DTOSHARED_EXPORT int DTOSHARED_CCA get_INN(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_INN(void *ptr, const wchar_t *inn);
DTOSHARED_EXPORT int DTOSHARED_CCA get_MachineNumber(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_MachineNumber(void *ptr, const wchar_t *machineNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_License(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_License(void *ptr, const wchar_t *license);
DTOSHARED_EXPORT int DTOSHARED_CCA get_LicenseNumber(void *ptr, int *licenseNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA put_LicenseNumber(void *ptr, int licenseNumber);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Table(void *ptr, int *table);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Table(void *ptr, int table);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Row(void *ptr, int *row);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Row(void *ptr, int row);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Field(void *ptr, int *field);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Field(void *ptr, int field);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FieldType(void *ptr, int *fieldType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FieldType(void *ptr, int fieldType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CommandBuffer(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CommandBuffer(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_AnswerBuffer(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DateEnd(void *ptr, int *day, int *month, int *year);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DateEnd(void *ptr, int day, int month, int year);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SessionEnd(void *ptr, int *session);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SessionEnd(void *ptr, int session);
DTOSHARED_EXPORT int DTOSHARED_CCA get_EKLZFlags(void *ptr, int *flags);
DTOSHARED_EXPORT int DTOSHARED_CCA get_EKLZKPKNumber(void *ptr, int *num);
DTOSHARED_EXPORT int DTOSHARED_CCA put_EKLZKPKNumber(void *ptr, int num);
DTOSHARED_EXPORT int DTOSHARED_CCA get_UnitType(void *ptr, int *unitType);
DTOSHARED_EXPORT int DTOSHARED_CCA put_UnitType(void *ptr, int unitType);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PictureNumber(void *ptr, int *num);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PictureNumber(void *ptr, int num);
DTOSHARED_EXPORT int DTOSHARED_CCA get_LeftMargin(void *ptr, int *margin);
DTOSHARED_EXPORT int DTOSHARED_CCA put_LeftMargin(void *ptr, int margin);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Memory(void *ptr, int *memory);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PictureState(void *ptr, int *state);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Width(void *ptr, int *width);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Width(void *ptr, int width);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Operator(void *ptr, int *op);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Operator(void *ptr, int op);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FontBold(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FontBold(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FontItalic(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FontItalic(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FontNegative(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FontNegative(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FontUnderline(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FontUnderline(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FontDblHeight(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FontDblHeight(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FontDblWidth(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FontDblWidth(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PrintPurpose(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrintPurpose(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReceiptFont(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReceiptFont(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReceiptFontHeight(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReceiptFontHeight(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReceiptBrightness(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReceiptBrightness(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ReceiptLinespacing(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReceiptLinespacing(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_JournalFont(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalFont(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_JournalFontHeight(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalFontHeight(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_JournalBrightness(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalBrightness(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_JournalLinespacing(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalLinespacing(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SummPointPosition(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SummPointPosition(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Destination(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Destination(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TaxNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TaxNumber(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodePrintType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodePrintType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeControlCode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeControlCode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeCorrection(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeCorrection(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeEncoding(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeEncoding(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeEncodingMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeEncodingMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FeedValue(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FeedValue(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ClsPtr(void *ptr, void **cls);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PixelLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_RcpPixelLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JrnPixelLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SlipPixelLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_RcpCharLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JrnCharLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SlipCharLineLength(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Count(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Count(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SlotNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SlotNumber(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DrawerOpened(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CoverOpened(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BatteryLow(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_VerHi(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_VerLo(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Build(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Codepage(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Remainder(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Change(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_LogicalNumber(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_LogicalNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_OperationType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_OperationType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DiscountNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DiscountNumber(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CounterType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CounterType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PowerSupplyValue(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PowerSupplyState(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PowerSupplyType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PowerSupplyType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_StepCounterType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_StepCounterType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodePixelProportions(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodePixelProportions(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeProportions(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeProportions(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeColumns(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeColumns(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeRows(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeRows(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodePackingMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodePackingMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeUseProportions(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeUseProportions(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeUseRows(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeUseRows(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeUseColumns(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeUseColumns(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeUseCorrection(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeUseCorrection(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeUseCodeWords(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeUseCodeWords(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeInvert(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeInvert(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeDeferredPrint(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeDeferredPrint(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeNumber(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DrawerOnTimeout(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DrawerOnTimeout(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DrawerOffTimeout(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DrawerOffTimeout(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_DrawerOnQuantity(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DrawerOnQuantity(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Frequency(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Frequency(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Duration(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Duration(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_Directory(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Directory(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FileSize(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FileSize(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FileOpenType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FileOpenType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FileOpenMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FileOpenMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FileOffset(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FileOffset(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FileReadSize(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FileReadSize(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_NeedResultFlag(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_NeedResultFlag(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA SetMode(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetMode(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA Beep(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Sound(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA OpenDrawer(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA AdvancedOpenDrawer(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA FullCut(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PartialCut(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Feed(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA OpenDirectory(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReadDirectory(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CloseDirectory(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA OpenFile(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CloseFile(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA DeleteFileFromSD(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA WriteFileToSD(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReadFile_(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA GetStatus(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetRegister(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetRange(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetSumm(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA OpenSession(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CashIncome(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CashOutcome(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Report(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA NewDocument(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA OpenCheck(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Registration(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Annulate(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Return(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Buy(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA BuyReturn(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA BuyAnnulate(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Storno(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Discount(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Charge(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetChargeDiscount(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA Payment(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA StornoPayment(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CancelCheck(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CloseCheck(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SummTax(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA StornoTax(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA PrintString(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA AddTextField(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintFormattedText(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintHeader(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintFooter(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA BeginDocument(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EndDocument(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintLastCheckCopy(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA PrintBarcode(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintPicture(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA GetPictureArrayStatus(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetPictureStatus(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintPictureByNumber(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA AddPicture(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA AddPictureFromFile(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA DeleteLastPicture(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ClearPictureArray(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetPicture(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PrintBarcodeByNumber(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ClearBarcodeArray(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA DeleteLastBarcode(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetBarcode(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA BeginReport(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetRecord(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EndReport(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA BeginAdd(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetRecord(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EndAdd(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA SetCaption(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetCaption(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetValue(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetValue(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetTableField(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetTableField(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA Fiscalization(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetSummary(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetDate(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetTime(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetLicense(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetLicense(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetSerialNumber(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA InitTables(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA TechZero(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA RunCommand(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA FlushBuffer(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ClearOutput(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA TestConnector(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA DemoPrint(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOff(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA WriteData(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA EKLZActivate(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EKLZCloseArchive(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EKLZGetStatus(void *ptr);



DTOSHARED_EXPORT int DTOSHARED_CCA put_ScannerEventHandlerFunc(void *ptr, ScanerEventHandlerFunc func);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ScannerEventHandler(void *ptr, void *handler);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ScannerPortHandler(void *ptr, void **handler);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ScannerMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ScannerMode(void *ptr, int *value);



DTOSHARED_EXPORT int DTOSHARED_CCA put_PinPadMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PinPadMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOnPinPad(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOffPinPad(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA WritePinPad(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReadPinPad(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PinPadDevice(void *ptr, void **cls);

DTOSHARED_EXPORT int DTOSHARED_CCA put_ModemMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOnModem(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOffModem(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA WriteModem(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReadModem(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemDevice(void *ptr, void **cls);

DTOSHARED_EXPORT int DTOSHARED_CCA put_ReadSize(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ReadSize(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA OpenPinPad(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ClosePinPad(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA OpenModem(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CloseModem(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA put_ModemConnectionType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemConnectionType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ModemAddress(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemAddress(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_ModemPort(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemPort(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA GetModemStatus(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetPinPadStatus(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WriteSize(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemStatus(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemSignal(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemOperator(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ModemError(void *ptr, wchar_t *bfr, int bfrSize);

DTOSHARED_EXPORT int DTOSHARED_CCA GetDeviceMetrics(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceDescription(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA GetCurrentMode(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_OutOfPaper(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrinterConnectionFailed(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrinterMechanismError(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrinterCutMechanismError(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrinterOverheatError(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA GetCurrentStatus(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA GetLastSummary(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_AdvancedMode(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_BottomMargin(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BottomMargin(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA get_EKLZKPK(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA EKLZGetKPK(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeVersion(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeVersion(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_TaxPassword(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TaxPassword(void *ptr, wchar_t *bfr, int bfrSize);

DTOSHARED_EXPORT int DTOSHARED_CCA put_Classifier(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_Classifier(void *ptr, wchar_t *bfr, int bfrSize);

DTOSHARED_EXPORT int DTOSHARED_CCA put_FiscalPropertyNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FiscalPropertyNumber(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FiscalPropertyValue(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FiscalPropertyValue(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FiscalPropertyType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FiscalPropertyType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_FiscalPropertyPrint(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FiscalPropertyPrint(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA WriteFiscalProperty(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReadFiscalProperty(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_HasNotSendedDocs(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA RunFNCommand(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_CounterDimension(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CounterDimension(void *ptr, int value);

DTOSHARED_EXPORT int DTOSHARED_CCA get_DiscountInSession(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ChargeInSession(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_NetworkError(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_OFDError(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FNError(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_TimeoutACK(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TimeoutACK(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TimeoutENQ(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TimeoutENQ(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA AddBarcode(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA GetBarcodeArrayStatus(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA Correction(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReturnCorrection(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA BuyCorrection(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA BuyReturnCorrection(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA put_PrintCheck(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PrintCheck(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA get_FNState(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA GetUnitVersion(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_TaxSum(void *ptr, double *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TaxSum(void *ptr, double value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TaxMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TaxMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PositionType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PositionType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PositionPaymentType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_PositionPaymentType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA AddFiscalProperty(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetFiscalProperties(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FfdVersion(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DeviceFfdVersion(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FNFfdVersion(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CommandCode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ErrorCode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ErrorData(void *ptr, wchar_t *bfr, int bfrSize);

DTOSHARED_EXPORT int DTOSHARED_CCA put_PositionSum(void *ptr, double value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PositionSum(void *ptr, double *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_FiscalPropertyUser(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FiscalPropertyUser(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_WiFiMode(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WiFiMode(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA WriteWiFi(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ReadWiFi(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WiFiDevice(void *ptr, void **cls);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOnWiFi(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA PowerOffWiFi(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA put_WiFiConnectionType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WiFiConnectionType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_WiFiAddress(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WiFiAddress(void *ptr, wchar_t *bfr, int bfrSize);
DTOSHARED_EXPORT int DTOSHARED_CCA put_WiFiPort(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WiFiPort(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA GetWiFiStatus(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_WiFiStatus(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA OpenWiFi(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA CloseWiFi(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_FNFiscal(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_ENVDMode(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA BeginFormFiscalProperty(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA EndFormFiscalProperty(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA put_LogLvl(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_LogLvl(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA SetLogLvl(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA ResetLogLvl(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA put_LogMessage(void *ptr, const wchar_t *value);
DTOSHARED_EXPORT int DTOSHARED_CCA WriteLog(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_ENVDEnabled(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_TaxNumeration(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_DocNumberEnd(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_DocNumberEnd(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_BarcodeOverlay(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_BarcodeOverlay(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_PositionQuantityType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_PositionQuantityType(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA ContinuePrint(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA SetDateTime(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA get_BatteryCharge(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA put_UseOnlyTaxNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_UseOnlyTaxNumber(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA AddTextAttribute(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA AddFormattedTextAttribute(void *ptr);
DTOSHARED_EXPORT int DTOSHARED_CCA put_CheckAttributeNumber(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_CheckAttributeNumber(void *ptr, int *value);

DTOSHARED_EXPORT int DTOSHARED_CCA get_TimeEnd(void *ptr, int *hours, int *minutes, int *seconds);
DTOSHARED_EXPORT int DTOSHARED_CCA put_TimeEnd(void *ptr, int hours, int minutes, int seconds);

DTOSHARED_EXPORT int DTOSHARED_CCA put_SystemOperationType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_SystemOperationData(void *ptr, const wchar_t *bfr);
DTOSHARED_EXPORT int DTOSHARED_CCA get_SystemOperationResult(void *ptr, wchar_t *bfr, int size);
DTOSHARED_EXPORT int DTOSHARED_CCA ExecSystemOperation(void *ptr);

DTOSHARED_EXPORT int DTOSHARED_CCA put_JournalDataType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalDataType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA put_JournalAttributesType(void *ptr, int value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalAttributesType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA get_JournalDocumentType(void *ptr, int *value);
DTOSHARED_EXPORT int DTOSHARED_CCA GetJournalStatus(void *ptr);

}

#endif // FPTR_INTERFACE_C_H
