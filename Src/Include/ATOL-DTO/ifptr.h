/** \file */

#ifndef IFPTR_H
#define IFPTR_H

#include "idtobase.h"
#include "abstract__event.h"

namespace TED
{

//! Драйвер контрольно-кассовых машин (дККТ)
namespace Fptr
{

#define DTO_IFPTR_VER1      15

class IFptr;

//! Протокол работы ККТ
enum Protocol
{
    ProtocolDefault = 0,        /*!< По умолчанию для выбранной модели        */
    ProtocolAtol20,             /*!< АТОЛ 2                                   */
    ProtocolAtol30,             /*!< АТОЛ 3                                   */
};

//! Модель ККТ
enum Model
{
    ModelFelicsRF = 14,         /*!< ФЕЛИКС-Р Ф                               */
    ModelFelics02K = 15,        /*!< ФЕЛИКС-02К / ЕНВД                        */
    ModelFelicsRK = 24,         /*!< ФЕЛИКС-РК / ЕНВД                         */
    ModelShtrihFRK = 25,        /*!< ШТРИХ-ФР-К                               */
    ModelFelics3SK = 27,        /*!< ФЕЛИКС-3СК                               */
    ModelFPrint02K = 30,        /*!< FPrint-02K / ЕНВД                        */
    ModelFPrint03K = 31,        /*!< FPrint-03K / ЕНВД                        */
    ModelFPrint88K = 32,        /*!< FPrint-88K / ЕНВД                        */
    ModelFPrint5200K = 35,      /*!< FPrint-5200K / ЕНВД                      */
    ModelFPrint55K = 47,        /*!< FPrint-55 ПТК / K / ЕНВД                 */
    ModelFPrint22K = 52,        /*!< FPrint-22 ПТК / K / ЕНВД                 */
    ModelFPrint11K = 51,        /*!< FPrint-11 ПТК / ЕНВД                     */
    ModelFPrint77K = 53,        /*!< FPrint-77 ПТК / ЕНВД                     */
    ModelFPrintPay01K = 54,     /*!< FPrintPay-01ПТК                          */
    ModelATOL25F = 57,          /*!< АТОЛ 25Ф                                 */
    ModelATOL30F = 61,          /*!< АТОЛ 30Ф                                 */
    ModelATOL55F = 62,          /*!< АТОЛ 55Ф                                 */
    ModelATOL22F = 63,          /*!< АТОЛ 22Ф (АТОЛ FPrint-22ПТК)             */
    ModelATOL52F = 64,          /*!< АТОЛ 52Ф                                 */
    ModelATOL03F = 65,          /*!< АТОЛ 03Ф                                 */
    ModelATOL11F = 67,          /*!< АТОЛ 11Ф                                 */
    ModelATOL02F = 68,          /*!< АТОЛ 02Ф                                 */
    ModelATOL77F = 69,          /*!< АТОЛ 77Ф                                 */
    ModelSM02 = 71,             /*!< СМ-02 ПТК / ЕНВД                         */
    ModelATOL90F = 72,          /*!< АТОЛ 90Ф                                 */
    ModelFPrint30K = 73,        /*!< Принтер документов FPrint-30 для ЕНВД    */
    ModelEVOTORST2F = 74,       /*!< ЭВОТОР СТ2Ф                              */
    ModelATOL60F = 75,          /*!< АТОЛ 60Ф                                 */
    ModelKaznacheyFA = 76,      /*!< Казначей ФА                              */
    ModelATOL42FS = 77,         /*!< АТОЛ 42ФС                                */
    ModelATOL15F = 78,          /*!< АТОЛ 15Ф                                 */
    ModelEVOTORST3F = 79,       /*!< ЭВОТОР СТ3Ф                              */
    ModelATOL50F = 80,          /*!< АТОЛ 50Ф                                 */
    ModelATOL20F = 81,          /*!< АТОЛ 20Ф                                 */
    ModelATOL91F = 82,          /*!< АТОЛ 91Ф                                 */
    ModelATOL92F = 84,          /*!< АТОЛ 92Ф                                 */
    ModelATOL150F = 86,         /*!< АТОЛ Sigma 10 (АТОЛ 150Ф)                */
    ModelShtrihLightFRK = 113,  /*!< ШТРИХ-LIGHT-ФР-К                         */
    ModelPiritFR01K = 114,      /*!< ПИРИТ ФР01К                              */
    ModelSpark801T = 123,       /*!< СПАРК-801т                               */
    ModelPiritK = 128,          /*!< Pirit K                                  */
    ModelPirit2F = 129,         /*!< Пирит 02Ф                                */
    ModelShtrihLight01F = 130,  /*!< ШТРИХ-ЛАЙТ-01Ф / ШТРИХ-ЛАЙТ-02Ф          */
    ModelShtrihM01F = 131,      /*!< ШТРИХ-М-01Ф / ШТРИХ-М-02Ф                */
    ModelShtrihFR01F = 132,     /*!< ШТРИХ-ФР-01Ф                             */
    ModelShtrihMini01F = 133,   /*!< ШТРИХ-МИНИ-01Ф                           */
    ModelShtrihMini02F = 134,   /*!< ШТРИХ-МИНИ-02Ф                           */
    ModelRetail01 = 135,        /*!< РИТЕЙЛ-01Ф                               */
    ModelRetail02 = 136,        /*!< РИТЕЙЛ-02Ф                               */
    ModelVikiPrint57F = 137,    /*!< Вики Принт 57Ф                           */
    ModelShtrihMPTK = 150,      /*!< ШТРИХ-М-ПТК                              */
};


//! Выравнивание
enum Alignment
{
    AlignmentLeft = 0,      /*!< По левому краю                               */
    AlignmentCenter,        /*!< По центру                                    */
    AlignmentRight          /*!< По правому краю                              */
};

//! Перенос текста
enum TextWrap
{
    TextWrapNone = 0,       /*!< Без переноса                                 */
    TextWrapWord,           /*!< Перенос по словам                            */
    TextWrapLine            /*!< Перенос по строкам                           */
};

//! Тип штрихкода
enum BarcodeType
{
    BarcodeEAN8 = 0,        /*!< EAN8                                         */
    BarcodeEAN13,           /*!< EAN13                                        */
    BarcodeUPCA,            /*!< UPCA                                         */
    BarcodeCode39,          /*!< Code 39                                      */
    BarcodeQR,              /*!< QR-код                                       */
    BarcodePDF417,          /*!< PDF417                                       */
    BarcodeITF14,           /*!< ITF-14                                       */
    BarcodeInterleaved2of5, /*!< Interleaved Two of Five                      */
    BarcodeUPCE,            /*!< UPCE                                         */
    BarcodeCodabar,         /*!< CODABAR                                      */
    BarcodeCode93,          /*!< Code 93                                      */
    BarcodeCode128,         /*!< Code 128                                     */
};

//! Уровень коррекции QR-кода
enum BarcodeQRCorrection
{
    BarcodeQRCorrectionDefault = 0, /*!< По умолчанию/выбору ККТ              */
    BarcodeQRCorrectionLevelL,      /*!< Low                                  */
    BarcodeQRCorrectionLevelM,      /*!< Medium                               */
    BarcodeQRCorrectionLevelQ,      /*!< Quartile                             */
    BarcodeQRCorrectionLevelH       /*!< High                                 */
};

//! Кодировка QR-кода
enum BarcodeQREncoding
{
    BarcodeQREncodingISO8859 = 0,   /*!< ISO-8859                             */
    BarcodeQREncodingUTF8,          /*!< UTF-8                                */
};

//! Режим кодировки QR-кода
enum BarcodeQREncodingMode
{
    BarcodeQREncodingModeNumbers = 0,      /*!< Числовой                      */
    BarcodeQREncodingModeNumbersLetters,   /*!< Буквенно-числовой             */
    BarcodeQREncodingMode8Bit,             /*!< 8-битный                      */
    BarcodeQREncodingModeECI               /*!< 8-битный ECI                  */
};

//! Режим упаковки PDF417-кода
enum BarcodePDF417PackingMode
{
    BarcodePDF417PackingModeDefault = 0,    /*!< Автоматический               */
    BarcodePDF417PackingModeText,           /*!< Текстовый                    */
    BarcodePDF417PackingModeBin,            /*!< Бинарный                     */
    BarcodePDF417PackingModeNumbers         /*!< Числовой                     */
};

//! Способ печати ШК
enum BarcodePrintType
{
    BarcodeSoftware = 0,    /*!< Генерация изображения штрихкода средствами
                                драйвера и печать его как картинки            */
    BarcodeHardware,        /*!< Печать ШК средствами устройства              */
    BarcodeAuto             /*!< Автоматический выбор способа печати          */
};

//! Режимы ККТ
enum Mode
{
    ModeSelect = 0,         /*!< Выбора                                       */
    ModeRegistration,       /*!< Регистрации                                  */
    ModeReportNoClear,      /*!< Отчетов без гашения                          */
    ModeReportClear,        /*!< Отчетов с гашением                           */
    ModeProgramming,        /*!< Программирования                             */
    ModeFiscalMemory,       /*!< Доступа к фискальной памяти                  */
    ModeEKLZ,               /*!< Доступа к ЭКЛЗ                               */
    ModeExtra               /*!< Дополнительный                               */
};

//! Типы регистраций
enum RegistrationType
{
    RegistrationStorno = 0,     /*!< Сторнo                                   */
    RegistrationSell,           /*!< Продажа / приход                         */
    RegistrationSellReturn,     /*!< Возврат продажи / прихода                */
    RegistrationSellAnnulate,   /*!< Аннулирование продажи                    */
    RegistrationBuy,            /*!< Покупка / расход                         */
    RegistrationBuyReturn,      /*!< Возврат покупки / расхода                */
    RegistrationBuyAnnulate,    /*!< Аннулирование покупки                    */
    RegistrationSellCorrection, /*!< Коррекция прихода                        */
    RegistrationSellReturnCorrection,   /*!< Коррекция возврата прихода       */
    RegistrationBuyCorrection,  /*!< Коррекция расхода                        */
    RegistrationBuyReturnCorrection,    /*!< Коррекция возврата расхода       */
};

//! Типы / состояния чека
enum ChequeType
{
    ChequeClosed = 0,           /*!< Чек закрыт                               */
    ChequeSell,                 /*!< Чек продажи / прихода                    */
    ChequeSellReturn,           /*!< Чек возврат продажи / прихода            */
    ChequeSellAnnulate,         /*!< Чек аннулирования продажи                */
    ChequeBuy,                  /*!< Чек покупки / расхода                    */
    ChequeBuyReturn,            /*!< Чек возврата покупки / расхода           */
    ChequeBuyAnnulate,          /*!< Чек аннулирования покупки                */
    ChequeSellCorrection,       /*!< Чек коррекции прихода                    */
    ChequeSellReturnCorrection, /*!< Чек коррекции возврата прихода           */
    ChequeBuyCorrection,        /*!< Чек коррекции расхода                    */
    ChequeBuyReturnCorrection,  /*!< Чек коррекции возврата расхода           */
};

//! Типы отчетов
enum ReportType
{
    ReportCRCLear = 0,              /*!< Гашение контрольной ленты            */
    ReportZ = 1,                    /*!< Отчет с гашением                     */
    ReportX = 2,                    /*!< Отчет без гашения                    */
    ReportFiscalDatesReduced = 3,   /*!< Сокращенный фискальный отчет
                                            по диапазону дат                  */
    ReportFiscalSessionsReduced = 4, /*!< Сокращенный фискальный отчет
                                             по диапазону смен                */
    ReportFiscalDatesFull = 5,      /*!< Полный фискальный отчет
                                            по диапазону дат                  */
    ReportFiscalSessionsFull = 6,   /*!< Полный фискальный отчет
                                            по диапазону смен                 */
    ReportDepartment = 7,           /*!< Отчет по секциям                     */
    ReportCashiers = 8,             /*!< Отчет по кассирам                    */
    ReportHours = 10,               /*!< Отчет по часам                       */
    ReportQuantity = 11,            /*!< Отчет по кол-ву                      */
    ReportRibbon = 12,              /*!< Данные КЛ                            */
    ReportRibbonCheque = 13,        /*!< Данные КЛ по чеку                    */
    ReportDumpRibbon = 14,          /*!< Дамп КЛ                              */
    ReportDumpRibbonCheque = 15,    /*!< Дамп КЛ по чеку                      */
    ReportRom = 19,                 /*!< ПО ККТ                               */
    ReportRomUnit = 20,             /*!< ПО модуля ККТ                        */
    ReportEKLZActivationTotal = 22, /*!< ЭКЛЗ итоги активации                 */
    ReportEKLZSessionTotal = 23,    /*!< ЭКЛЗ итоги смены                     */
    ReportEKLZSessionCR = 24,       /*!< ЭКЛЗ КЛ смены                        */
    ReportEKLZKPKDoc = 25,          /*!< ЭКЛЗ документ по номеру              */
    ReportEKLZDatesDepartmentsReduced = 26, /*!< ЭКЛЗ по датам краткий
                                                        по секциям            */
    ReportEKLZDatesDepartmentsFull = 27,    /*!< ЭКЛЗ по датам полный
                                                        по секциям            */
    ReportEKLZDatesSessionsTotalReduced = 28,   /*!< ЭКЛЗ по датам краткий
                                                        по итогам смен        */
    ReportEKLZDatesSessionsTotalFull = 29,  /*!< ЭКЛЗ по датам полный
                                                        по итогам смен        */
    ReportEKLZSessionsDepartmentsReduced = 30,  /*!< ЭКЛЗ по сменам краткий
                                                        по секциям            */
    ReportEKLZSessionsDepartmentsFull = 31, /*!< ЭКЛЗ по сменам полный
                                                        по секциям            */
    ReportEKLZSessionsTotalReduced = 32,    /*!< ЭКЛЗ по сменам краткий
                                                        по итогам смен        */
    ReportEKLZSessionsTotalFull = 33,   /*!< ЭКЛЗ по сменам полный по итогам
                                                                  смен        */
    ReportDocumentByNumber = 34,    /*!< Печать документа по номеру           */
    ReportCRPrintFull = 35,         /*!< Печать КЛ полностью                  */
    ReportCRPrintReduced = 36,      /*!< Печать КЛ сокращенно                 */
    ReportService = 38,             /*!< Служебный отчет                      */
    ReportSD = 39,                  /*!< Электронный отчет с SD               */
    ReportJournalData = 41,         /*!< Данные ЭЖ                            */
    ReportAccountingState = 42,     /*!< Состояние расчетов                   */
    ReportPrintInfo = 43,           /*!< Печать информации о ККТ              */
    ReportTestDevice = 44,          /*!< Тестирование ККТ                     */
    ReportOfdConnectionDiagnostic = 45, /*!< Диагностика соединения с ОФД     */
    ReportLastDocument = 46,        /*!< Получение последнего чека
                                            в электронном виде                */
    ReportSessionTotalCounters = 47, /*!< Счетчики итогов смены               */
    ReportFNTotalCounters = 48,      /*!< Счетчики итогов ФН                  */
    ReportNotSentDocumentsCounters = 49, /*!< Счетчики по непереданным ФД     */
    ReportPrintDocumentsFromJournalByNumbers = 50, /*!< ЭЖ по диапазону документов */
    ReportPrintDocumentsFromJournalBySessions = 51, /*!< ЭЖ по диапазону смен    */
    ReportJournalDataEx = 52,         /*!< Данные ЭЖ (расширенный)             */
};

//! Типы скидок
enum DiscountType
{
    DiscountSumm = 0,           /*!< Скидка суммой                            */
    DiscountPercent             /*!< Скидка в процентах                       */
};

//! Перемещение наличности
enum CashMoveDirection
{
    CashMoveIncome = 0,         /*!< Внесения                                 */
    CashMoveOutcome             /*!< Выплаты                                  */
};

//! Типы полей таблиц ККТ
enum FieldType
{
    FieldInteger = 0,           /*!< Целое число (BCD)                        */
    FieldString,                /*!< Строка (тип 1)                           */
    FieldBuffer,                /*!< Массив байтов                            */
    FieldIntegerBin,            /*!< Целое число (BIN)                        */
    FieldStringEx               /*!< Строка (тип 2)                           */
};

//! Типы источников дампов ПО
enum UnitType
{
    UnitMain = 1,               /*!< Процессор с внутренним ПО ККТ            */
    UnitFiscalMemory,           /*!< Процессор управления фискальной памятью  */
    UnitBootBlock,              /*!< Bootblock процессора в внутренним ПО     */
    UnitPrinter,                /*!< Процессор управления принтером           */
    UnitPrinterBootBlock        /*!< Bootblock процессора управления принтером*/
};

//! Состояние изображения в памяти ККТ
enum PictureState
{
    PictureClosed = 0,          /*!< Запись изображения закрыта               */
    PictureOpened               /*!< Запись изображения открыта               */
};

//! Типы транзакций в отчете КЛ
enum TransactionType
{
    TransactionSellFree = 1,            /*!< Продажа по свободной цене        */
    TransactionStornoFree,              /*!< Сторно по свободной цене         */
    TransactionAnnulateFree,            /*!< Аннулирование по свободной цене  */
    TransactionReturnFree,              /*!< Возврат по свободной цене        */
    TransactionSummDiscountPos = 5,     /*!< Абсолютная скидка на позицию     */
    TransactionSummChargePos,           /*!< Абсолютная надбавка на позицию   */
    TransactionPercentDiscountPos,      /*!< Процентная скидка на позицию     */
    TransactionPercentChargePos,        /*!< Процентная надбавка на позицию   */
    TransactionSellInCode = 11,         /*!< Продажа по внутреннему коду      */
    TransactionStornoInCode,            /*!< Сторно по внутреннему коду       */
    TransactionAnnulateInCode,          /*!< Аннулирование по внутреннему коду*/
    TransactionReturnInCode,            /*!< Возврат по внутреннему коду      */
    TransactionSellOutCode = 21,        /*!< Продажа по внешнему коду         */
    TransactionStornoOutCode,           /*!< Сторно по внешнему коду          */
    TransactionAnnulateOutCode,         /*!< Аннулирование по внешнему коду   */
    TransactionReturnOutCode,           /*!< Возврат по внешнему коду         */
    TransactionSummDiscountAll = 35,    /*!< Абсолютная скидка на чек         */
    TransactionSummChargeAll,           /*!< Абсолютная надбавка на чек       */
    TransactionPercentDiscountAll,      /*!< Процентная скидка на чек         */
    TransactionPercentChargeAll,        /*!< Процентная надбавка на чек       */
    TransactionPaymentType0 = 42,       /*!< Оплата наличными (тип 0)         */
    TransactionPaymentType1 = 44,       /*!< Оплата первым типом оплаты       */
    TransactionPaymentType2 = 47,       /*!< Оплата вторым типом оплаты       */
    TransactionPaymentType3 = 48,       /*!< Оплата третьим типом оплаты      */
    TransactionCashIncome = 50,         /*!< Внесение в кассу                 */
    TransactionCashOutcome,             /*!< Выплата из кассы                 */
    TransactionEndOfCheque = 55         /*!< Конец чека                       */
};

//! Цели печати
enum PrintPurpose
{
    PrintReceipt = 0,           /*!< Печатать на ЧЛ                           */
    PrintJournal,               /*!< Печатать на КЛ                           */
    PrintEverywhere,            /*!< Печатать на ЧЛ и КЛ                      */
};

//! Шрифты
enum Font
{
    FontBySettings = 0,         /*!< По настройке ККТ                         */
    Font12x24,                  /*!< 12x24                                    */
    Font12x20,                  /*!< 12x20                                    */
    Font12x16,                  /*!< 12x16                                    */
    Font12x10,                  /*!< 12x10                                    */
    Font12x10_Bold,             /*!< 12x10 жирный                             */
    Font10x14,                  /*!< 10x14                                    */
    Font9x10                    /*!< 9x10                                     */
};

//! Высота шрифта
enum FontHeight
{
    FontHeightBySettings = 0,   /*!< По настройке ККТ                         */
    FontHeightStretched,        /*!< Растянутый по высоте                     */
    FontHeightSingle            /*!< Одиночная высота                         */
};

//! Цель налога / скидки
enum DestinationType
{
    OnCheck = 0,                /*!< На чек                                   */
    OnPosition,                 /*!< На позицию                               */
};

//! Способ создания файла
enum FileOpenType
{
    FileOpenIfExisted = 0,      /*! Открыть существующий                      */
    FileCreateNew               /*! Создать новый                             */
};

//! Способ открытия файла
enum FileOpenMode
{
    FileReadOnly = 0,           /*!< На чтение                                */
    FileWriteOnly,              /*!< На запись                                */
    FileReadWrite               /*!< На чтение и запись                       */
};

//! Режим работы с пинпадом
enum PinPadMode
{
    PinPadModeOff = 0,
    PinPadModeSync = 1,         /*!< Данные пинпада по запросу                */
    PinPadModeAsync = 2,        /*!< Асинхронное ожидание данных от пинпада   */
};

//! Режим работы с модемом
enum ModemMode
{
    ModemModeOff = 0,
    ModemModeSync = 1,          /*!< Данные модема по запросу                 */
    ModemModeAsync = 2,         /*!< Асинхронное ожидание данных от модема    */
};

//! Режим работы с WiFi
enum WiFiMode
{
    WiFiModeOff = 0,
    WiFiModeSync = 1,          /*!< Данные Wi-Fi  по запросу                  */
    WiFiModeAsync = 2,         /*!< Асинхронное ожидание данных от Wi-Fi      */
};

//! Режим работы со сканером
enum ScannerMode
{
    ScannerModeOff = 0,          /*!< Обмен отключен                          */
    ScannerModeAsync = 1         /*!< Асинхронное ожидание данных от сканера  */
};

//! Состояние модема
enum ModemState
{
    ModemStateOff = 0,              /*!< Выключен                             */
    ModemStateInit = 1,             /*!< Инициализируется                     */
    ModemStateNotConnected = 2,     /*!< Зарегистрирован в сети, не подключен */
    ModemStateConnecting = 3,       /*!< Подключается                         */
    ModemStateConnected = 4,        /*!< Подключен                            */
    ModemStateDisconnecting = 5,    /*!< Отключается                          */
    ModemStateBusy = 6,             /*!< Занят                                */
    ModemStateGPRSEstablished = 7,  /*!< Зарегистрирован в сети, не подключен,
                                         соединение GPRS установлено          */
    ModemStateTcpDisconnecting = 8, /*!< Отключается от хоста                 */
};

//! Способ соединения модема
enum ModemConnectionType
{
    ModemConnectionTcp = 1,     /*!< TCP/IP                                   */
    ModemConnectionUdp,         /*!< UDP                                      */
};

//! Состояние WiFi
enum WiFiState
{
    WiFiStateOff = 0,               /*!< Выключен                             */
    WiFiStateConnectingToAP = 1,    /*!< Подключается к точке доступа         */
    WiFiStateConnectedToAP = 2,     /*!< Подключен к точке доступа            */
    WiFiStateConnecting = 3,        /*!< Подключается к серверу               */
    WiFiStateConnected = 4,         /*!< Подключен к серверу                  */
    WiFiStateDisconnecting = 5,     /*!< Отключается                          */
};

//! Способ соединения WiFi
enum WiFiConnectionType
{
    WiFiConnectionTcp = 1,      /*!< TCP/IP                                   */
    WiFiConnectionUdp,          /*!< UDP                                      */
};

//! Тип реквизита
enum FiscalPropertyType
{
    FiscalPropertyTypeRaw = 0,      /*!< Массив байтов                        */
    FiscalPropertyTypeByte,         /*!< 1-байтовое целое                     */
    FiscalPropertyTypeInt16,        /*!< 2-байтовое целое                     */
    FiscalPropertyTypeInt32,        /*!< 4-байтовое целое                     */
    FiscalPropertyTypeUnixTime,     /*!< Unix-время                           */
    FiscalPropertyTypeString        /*!< Строка                               */
};

//! Размерность регистра
enum CounterDimension
{
    CounterDimension7BCD = 0,       /*!< 7 BCD                                */
    CounterDimension9BCD            /*!< 9 BCD                                */
};

//! Режим работы с налогом
enum TaxMode
{
    TaxOnPosition = 0,              /*!< Налог на позицию                     */
    TaxOnUnit                       /*!< Налог на единицу                     */
};

//! Налоговые ставки
enum TaxNumber
{
    TaxVATByDepartment = 0,         /*!< Из таблицы отделов                   */
    TaxVAT0,                        /*!< НДС 0%                               */
    TaxVAT10,                       /*!< НДС 10\%                             */
    TaxVAT18,                       /*!< НДС 18\% (20\%)                      */
    TaxVATNo,                       /*!< Без НДС                              */
    TaxVAT110,                      /*!< НДС 10/110 (20/120)                  */
    TaxVAT118,                      /*!< НДС 18/118                           */
};

//! Версии ФФД
enum FFDVersion
{
    FFD_NONE = 0,                   /*! ККТ старого порядка                   */
    FFD_1_0 = 100,                  /*! ФФД 1.0                               */
    FFD_1_05 = 105,                 /*! ФФД 1.05                              */
    FFD_1_1 = 110,                  /*! ФФД 1.1                               */
};

enum JournalAttributeType
{
    JournalAttributeFiscal,
    JournalAttributeAll,
};

enum JournalDataType
{
    JournalDataReceipts = 2,
    JournalDataReports = 3,
};

}

}


//! Создает экземпляр драйвера ККТ.
/*!
   При несовпадении версии ver и актуальной будет возвращаться 0, а в лог записываться соответствующее сообщение.
   \param ver Версия драйвера (DTO_IFPTR_VER1)
   \return Указатель на экземпляр драйвера ККТ
   \retval 0 Ошибка
 */
extern "C" DTOSHARED_EXPORT TED::Fptr::IFptr * DTOSHARED_CCA CreateFptrInterface(int ver);

//! Разрушает экземпляр драйвера ККТ.
/*!
   \param iface Экземпляр драйвера ККТ
 */
extern "C" DTOSHARED_EXPORT void DTOSHARED_CCA ReleaseFptrInterface(TED::Fptr::IFptr **iface);

//! Тип функции CreateFptrInterface().
typedef TED::Fptr::IFptr*(*CreateFptrInterfacePtr)(int ver);
//! Тип функции ReleaseFptrInterface().
typedef void (*ReleaseFptrInterfacePtr)(TED::Fptr::IFptr **iface);


namespace TED
{

namespace Fptr
{

//! Интерфейс драйвера контрольно кассовых машин (дККТ)
class IFptr : public IDTOBase
{
public:
    friend DTOSHARED_EXPORT void DTOSHARED_CCA ::ReleaseFptrInterface(TED::Fptr::IFptr * *);
    typedef void (*ReleaseFunction)(IFptr**);

    //! Возвращает указатель на функцию удаления интерфейса.
    /*!
       \return Указатель на функцию удаления интерфейса
     */
    virtual ReleaseFunction DTOSHARED_CCA get_ReleaseFunction() = 0;

    //! Записывает строковый системный параметр в ККТ.
    /*!
       Записывает строку, заданную в свойстве \a Caption, в системный параметр под номером \a CaptionPurpose.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Caption         <td>Значение параметра  <td>put_Caption()
        <tr><td>CaptionPurpose  <td>Номер параметра     <td>put_CaptionPurpose()
       </table>
       Список строковых параметров представлен в разделе \ref fptrCaptions.

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \warning Разные модели ККТ поддерживают разные параметры!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa GetCaption()
     */
    virtual int DTOSHARED_CCA SetCaption() = 0;

    //! Считывает строковый системный параметр из ККТ.
    /*!
       Считывает из ККТ строковый системный параметр под номером \a CaptionPurpose и записывает значение в \a Caption.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CaptionPurpose  <td>Номер параметра     <td>put_CaptionPurpose()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Caption         <td>Значение параметра  <td>get_Caption()
       </table>
       Список строковых параметров представлен в разделе \ref fptrCaptions.

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \warning Разные модели ККТ поддерживают разные параметры!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa SetCaption()
     */
    virtual int DTOSHARED_CCA GetCaption() = 0;

    //! Возвращает строку.
    /*!
       \param bfr Буфер для строки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_Caption()
     */
    virtual int DTOSHARED_CCA get_Caption(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает строку.
    /*!
       \param value Строка
       \retval -1 Ошибка
       \retval 0 Успех
       \return Код результата
       \sa get_Caption()
     */
    virtual int DTOSHARED_CCA put_Caption(const wchar_t *value) = 0;

    //! Возвращает номер строкового системного параметра.
    /*!
       \param value Номер параметра
       \retval 0 Успех
       \sa put_CaptionPurpose()
     */
    virtual int DTOSHARED_CCA get_CaptionPurpose(int *value) = 0;

    //! Устанавливает номер строкового системного параметра.
    /*!
       \param value Номер параметра
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CaptionPurpose()
     */
    virtual int DTOSHARED_CCA put_CaptionPurpose(int value) = 0;

    //! Проверяет наличие строкового системного параметра.
    /*!
       Проверяет наличие строкового системного параметра под номером \a CaptionPurpose для настроенной модели ККТ.
       \param value Флаг наличия параметра (1 (true) - параметр есть, 0 (false) - параметра нет)
       \retval 0 Успех
       \sa put_CaptionPurpose()
     */
    virtual int DTOSHARED_CCA get_CaptionIsSupported(int *value) = 0;

    //! Возвращает название строкового системного параметра.
    /*!
       Возвращает название строкового системного параметра под номером \a CaptionPurpose.
       \param bfr Буфер для названия
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CaptionPurpose()
     */
    virtual int DTOSHARED_CCA get_CaptionName(wchar_t *bfr, int bfrSize) = 0;

    //! Записывает числовой системный параметр в ККТ.
    /*!
       Записывает число, заданное в свойстве \a Value, в системный параметр под номером \a ValuePurpose.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Value           <td>Значение параметра  <td>put_Value()
        <tr><td>ValuePurpose    <td>Номер параметра     <td>put_Value()
       </table>
       Список числовых параметров представлен в разделе \ref fptrValues.

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa GetValue()
     */
    virtual int DTOSHARED_CCA SetValue() = 0;

    //! Считывает числовой системный параметр из ККТ.
    /*!
       Считывает из ККТ числовой системный параметр под номером \a ValuePurpose и записывает значение в \a Value.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ValuePurpose    <td>Номер параметра     <td>put_ValuePurpose()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Value           <td>Значение параметра  <td>get_Value()
       </table>
       Список числовых параметров представлен в разделе \ref fptrValues.

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa SetValue()
     */
    virtual int DTOSHARED_CCA GetValue() = 0;

    //! Возвращает числовой системный параметр.
    /*!
       \param value Число
       \retval 0 Успех
       \sa put_Value()
     */
    virtual int DTOSHARED_CCA get_Value(double *value) = 0;

    //! Устанавливает числовой системный параметр.
    /*!
       \param value Число
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Value()
     */
    virtual int DTOSHARED_CCA put_Value(double value) = 0;

    //! Возвращает номер числового системного параметра.
    /*!
       \param value Номер параметра
       \retval 0 Успех
       \sa put_ValuePurpose()
     */
    virtual int DTOSHARED_CCA get_ValuePurpose(int *value) = 0;

    //! Устанавливает номер числового системного параметра.
    /*!
       \param value Номер параметра
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ValuePurpose()
     */
    virtual int DTOSHARED_CCA put_ValuePurpose(int value) = 0;

    //! Проверяет наличие числового системного параметра.
    /*!
       Проверяет наличие числового системного параметра под номером \a ValuePurpose для настроенной модели ККТ.
       \param value Флаг наличия параметра (1 (true) - параметр есть, 0 (false) - параметра нет)
       \retval 0 Успех
       \sa put_ValuePurpose()
     */
    virtual int DTOSHARED_CCA get_ValueIsSupported(int *value) = 0;

    //! Возвращает название числового системного параметра
    /*!
       Возвращает название числового системного параметра под номером \a ValuePurpose.
       \param bfr Буфер для названия
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ValuePurpose()
     */
    virtual int DTOSHARED_CCA get_ValueName(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает список значений числового системного параметра.
    /*!
       Возвращает список значений числового системного параметра под номером \a ValuePurpose.
       \param bfr Буфер для списка
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ValuePurpose()
     */
    virtual int DTOSHARED_CCA get_ValueMapping(wchar_t *bfr, int bfrSize) = 0;

    //! Выдает гудок средствами ККТ.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Beep() = 0;

    //! Выдает звуковой сигнал средствами ККТ.
    /*!
       Выдает звуковой сигнал частотой \a Frequency (в гц) и длительностью \a Duration (в мс) средствами ККТ.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Frequency   <td>Частота (в Гц)      <td>put_Frequency()
        <tr><td>Duration    <td>Длительность (в мс) <td>put_Duration()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Sound() = 0;

    //! Устанавливает частоту.
    /*!
       \param value Частота
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Frequency()
     */
    virtual int DTOSHARED_CCA put_Frequency(int value) = 0;

    //! Возвращает частоту.
    /*!
       \param value Частота
       \retval 0 Успех
       \sa put_Frequency()
     */
    virtual int DTOSHARED_CCA get_Frequency(int *value) = 0;

    //! Устанавливает длительность.
    /*!
       \param value Длительность
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Duration()
     */
    virtual int DTOSHARED_CCA put_Duration(int value) = 0;

    //! Возвращает длительность.
    /*!
       \param value Длительность
       \retval 0 Успех
       \sa put_Duration()
     */
    virtual int DTOSHARED_CCA get_Duration(int *value) = 0;

    //! Открывает подключенный к ККТ денежный ящик.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \warning По результатам выполнения данного метода нельзя судить о:
       - наличии связи с денежным ящиком;
       - состоянии ящика (открылся ящик или нет). Для определения состояния денежного ящика
       надо анализировать содержимое свойства \a DrawerOpened после успешного выполнения
       метода GetStatus().
     */
    virtual int DTOSHARED_CCA OpenDrawer() = 0;

    //! Производит импульсное открытие денежного ящика.
    /*!
       Метод подает команду ККТ открыть денежный ящик, подключенный к ККТ, используя заданные
       время включения (\a DrawerOnTimeout, в 10 мс), время выключения (\a DrawerOffTimeout, в 10 мс) и количество импульсов (\a DrawerOnQuantity)
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>DrawerOnTimeout     <td>Время включения (в 10 мс)   <td>put_DrawerOnTimeout()
        <tr><td>DrawerOffTimeout    <td>Время выключения (в 10 мс)  <td>put_DrawerOffTimeout()
        <tr><td>DrawerOnQuantity    <td>Количество импульсов        <td>put_DrawerOnQuantity()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \warning По результатам выполнения данного метода нельзя судить о:
       - наличии связи с денежным ящиком;
       - состоянии ящика (открылся ящик или нет). Для определения состояния денежного ящика
       надо анализировать содержимое свойства DrawerOpened после успешного выполнения
       метода GetStatus().
     */
    virtual int DTOSHARED_CCA AdvancedOpenDrawer() = 0;

    //! Устанавливает время включения денежного ящика.
    /*!
       \param value Время включения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DrawerOnTimeout()
     */
    virtual int DTOSHARED_CCA put_DrawerOnTimeout(int value) = 0;

    //! Возвращает время включения денежного ящика.
    /*!
       \param value Время включения
       \retval 0 Успех
       \sa put_DrawerOnTimeout()
     */
    virtual int DTOSHARED_CCA get_DrawerOnTimeout(int *value) = 0;

    //! Устанавливает время выключения денежного ящика.
    /*!
       \param value Время выключения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DrawerOffTimeout()
     */
    virtual int DTOSHARED_CCA put_DrawerOffTimeout(int value) = 0;

    //! Возвращает время выключения денежного ящика.
    /*!
       \param value Время выключения
       \retval 0 Успех
       \sa put_DrawerOffTimeout()
     */
    virtual int DTOSHARED_CCA get_DrawerOffTimeout(int *value) = 0;

    //! Устанавливает количество импульсов для открытия денежного ящика.
    /*!
       \param value Количество импульсов
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DrawerOnQuantity()
     */
    virtual int DTOSHARED_CCA put_DrawerOnQuantity(int value) = 0;

    //! Возвращает количество импульсов для открытия денежного ящика.
    /*!
       \param value Количество импульсов
       \retval 0 Успех
       \sa put_DrawerOnQuantity()
     */
    virtual int DTOSHARED_CCA get_DrawerOnQuantity(int *value) = 0;

    //! Заполняет свойства драйвера текущим состоянием ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>SummPointPosition       <td>Позиция десятичной точки в суммах       <td>get_SummPointPosition()
        <tr><td>CheckState              <td>Состояние чека (TED::Fptr::ChequeType)  <td>get_CheckState()
        <tr><td>CheckNumber             <td>Номер чека                              <td>get_CheckNumber()
        <tr><td>DocNumber               <td>Номер документа                         <td>get_DocNumber()
        <tr><td>CharLineLength          <td>Ширина активной станции в символах      <td>get_CharLineLength()
        <tr><td>PixelLineLength         <td>Ширина активной станции в точках        <td>get_PixelLineLength()
        <tr><td>RcpCharLineLength       <td>Ширина ЧЛ в символах                    <td>get_RcpCharLineLength()
        <tr><td>RcpPixelLineLength      <td>Ширина ЧЛ в точках                      <td>get_RcpPixelLineLength()
        <tr><td>JrnCharLineLength       <td>Ширина КЛ в символах                    <td>get_JrnCharLineLength()
        <tr><td>JrnPixelLineLength      <td>Ширина КЛ в точках                      <td>get_JrnPixelLineLength()
        <tr><td>SlipCharLineLength      <td>Ширина ПД в символах                    <td>get_SlipCharLineLength()
        <tr><td>SlipPixelLineLength     <td>Ширина ПД в точках                      <td>get_SlipPixelLineLength()
        <tr><td>SerialNumber            <td>Заводской номер                         <td>get_SerialNumber()
        <tr><td>Session                 <td>Смена                                   <td>get_Session()
        <tr><td>Date                    <td>Текущая дата                            <td>get_Date()
        <tr><td>Time                    <td>Текущее время                           <td>get_Time()
        <tr><td>Operator                <td>Номер оператора                         <td>get_Operator()
        <tr><td>LogicalNumber           <td>Номер ККТ                               <td>get_LogicalNumber()
        <tr><td>SessionOpened           <td>Флаг состояния смены                    <td>get_SessionOpened()
        <tr><td>Fiscal                  <td>Флаг фискальности                       <td>get_Fiscal()
        <tr><td>DrawerOpened            <td>Флаг состояния денежного ящика          <td>get_DrawerOpened()
        <tr><td>CoverOpened             <td>Флаг состояния крышки                   <td>get_CoverOpened()
        <tr><td>CheckPaperPresent       <td>Флаг наличия ЧЛ                         <td>get_CheckPaperPresent()
        <tr><td>ControlPaperPresent     <td>Флаг наличия КЛ                         <td>get_ControlPaperPresent()
        <tr><td>Model                   <td>Модель                                  <td>get_Model()
        <tr><td>Mode                    <td>Режим (TED::Fptr::Mode)                 <td>get_Mode()
        <tr><td>AdvancedMode            <td>Подрежим                                <td>get_AdvancedMode()
        <tr><td>SlotNumber              <td>Номер/тип порта                         <td>get_SlotNumber()
        <tr><td>Summ                    <td>Сумма чека                              <td>get_Summ()
        <tr><td>FNFiscal                <td>Флаг фискализации ФН                    <td>get_FNFiscal()
        <tr><td>OutOfPaper              <td>Флаг отсутствия бумаги                  <td>get_OutOfPaper()
        <tr><td>PrinterConnectionFailed <td>Флаг отсутствия связи с принтером       <td>get_PrinterConnectionFailed()
        <tr><td>PrinterMechanismError   <td>Флаг ошибки печатающего устройства      <td>get_PrinterMechanismError()
        <tr><td>PrinterCutMechanismError<td>Флаг ошибки отрезчика                   <td>get_PrinterCutMechanismError()
        <tr><td>PrinterOverheatError    <td>Флаг перегрева печатающей головки       <td>get_PrinterOverheatError()
        <tr><td>VerHi                   <td>Версия ПО                               <td>get_VerHi()
        <tr><td>VerLo                   <td>Подверсия ПО                            <td>get_VerLo()
        <tr><td>Build                   <td>Версия сборки ПО                        <td>get_Build()
        <tr><td>DeviceDescription       <td>Описание устройства                     <td>get_DeviceDescription()
       </table>

       \warning Следует как можно реже пользоваться данным методом, так как для его выполнения
       требуется время.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetStatus() = 0;

    //! Возвращает максимальную длину строки символов.
    /*!
       \param value Длина строки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_CharLineLength(int *value) = 0;

    //! Возвращает серийный номер устройства.
    /*!
       \param bfr Буфер для номера
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_SerialNumber()
     */
    virtual int DTOSHARED_CCA get_SerialNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает серийный номер устройства.
    /*!
       \param value Серийный номер
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_SerialNumber()
     */
    virtual int DTOSHARED_CCA put_SerialNumber(const wchar_t *value) = 0;

    //! Возвращает признак фискализированности ККТ.
    /*!
       \param value Признак фискализированности ККТ (1 (true) - фискализирована, 0 (false) - не фискализирована)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Fiscal(int *value) = 0;

    //! Устанавливает время в ККТ.
    /*!
       Устанавливает время \a Time в ККТ.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Time        <td>Время           <td>put_Time()
       </table>

       \deprecated Устаревшая команда. В новых интеграциях рекомендуется использовать \ref SetDateTime()

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetTime() = 0;

    //! Устанавливает дату в ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Date        <td>Дата            <td>put_Date()
       </table>

       \deprecated Устаревшая команда. В новых интеграциях рекомендуется использовать \ref SetDateTime()

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetDate() = 0;

    //! Возвращает время.
    /*!
       \param hours Часы
       \param minutes Минуты
       \param seconds Секунды
       \retval 0 Успех
       \sa put_Time()
     */
    virtual int DTOSHARED_CCA get_Time(int *hours, int *minutes, int *seconds) = 0;

    //! Устанавливает время.
    /*!
       \param hours Часы
       \param minutes Минуты
       \param seconds Секунды
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Time()
     */
    virtual int DTOSHARED_CCA put_Time(int hours, int minutes, int seconds) = 0;

    //! Возвращает дату.
    /*!
       \param day День
       \param month Месяц
       \param year Год
       \retval 0 Успех
       \sa put_Date()
     */
    virtual int DTOSHARED_CCA get_Date(int *day, int *month, int *year) = 0;

    //! Устанаваливает дату.
    /*!
       \param day День
       \param month Месяц
       \param year Год
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Date()
     */
    virtual int DTOSHARED_CCA put_Date(int day, int month, int year) = 0;

    //! Устанавливает режим ККТ.
    /*!
       Выполняет вход в режим \a Mode с паролем \a UserPassword.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Mode            <td>Режим (TED::Fptr::Mode) <td>put_Mode()
        <tr><td>UserPassword    <td>Пароль для входа        <td>put_UserPassword()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetMode() = 0;

    //! Сбрасывает режим ККТ.
    /*!
       Выполняется автоматически в SetMode()
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ResetMode() = 0;

    //! Возвращает флаг тестового режима.
    /*!
       \param value Флаг тестового режима (1 (true) - тестовый режим, 0 (false) - боевой режим)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_TestMode(int *value) = 0;

    //! Устанавливает флаг тестового режима.
    /*!
       Установка флага позволяет провести операцию в тестовом режиме.
       \param value Флаг тестового режима
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_TestMode(int value) = 0;

    //! Возвращает пароль пользователя.
    /*!
       \param bfr Буфер для пароля
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_UserPassword(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает пароль пользователя.
    /*!
       \param value Пароль
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_UserPassword(const wchar_t *value) = 0;

    //! Возвращает значение режима.
    /*!
       \param value Режим
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Mode(int *value) = 0;

    //! Устанаваливает значение режима работы.
    /*!
       \param value Режим
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_Mode(int value) = 0;

    //! Возвращает флаг проверки наличности.
    /*!
       \param value Флаг проверки наличности (1 (true) - проверять наличность, 0 (false) - не проверять наличность)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_EnableCheckSumm(int *value) = 0;

    //! Устанавливает флаг проверки наличности.
    /*!
       \param value Флаг проверки наличности
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_EnableCheckSumm(int value) = 0;

    //! Производит полную отрезку чековой ленты.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA FullCut() = 0;

    //! Производит частичную отрезку чековой ленты.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PartialCut() = 0;

    //! Отменяет текущий чек и пытается войти в заданный режим.
    /*!
       Данный метод является сервисным и представляет собой логическое сочетание нескольких
       кассовых операций (GetStatus(), SetMode(), CancelCheck() и т.д.).
       Выполняются следующие действия:
       - если чек открыт, он будет отменен;
       - выполняется запрос некоторых параметров ККТ;
       - драйвер возвращает ошибки, возникающие в работе ККТ;
       - выполняется попытка войти в режим, указанный в свойстве Mode.

       \retval -1 Ошибка
       \retval 0 Успех
       \sa put_Mode()
       \sa put_UserPassword()
     */
    virtual int DTOSHARED_CCA NewDocument() = 0;

    //! Переводит драйвер в состояние буферизации данных, добавляемых
    //! методами печати
    /*!
       \warning Не используется в текущей версии
       \retval -1 Ошибка
       \retval 0 Успех
       \sa EndDocument()
       \sa ClearOutput()
     */
    virtual int DTOSHARED_CCA BeginDocument() = 0;

    //! Печатает на ПД все забуферизованные данные.
    /*!
       \warning Не используется в текущей версии
       \retval -1 Ошибка
       \retval 0 Успех
       \sa BeginDocument()
       \sa ClearOutput()
     */
    virtual int DTOSHARED_CCA EndDocument() = 0;

    //! Очищает буфер печати.
    /*!
       \warning Не используется в текущей версии
       \retval -1 Ошибка
       \retval 0 Успех
       \sa BeginDocument()
       \sa EndDocument()
     */
    virtual int DTOSHARED_CCA ClearOutput() = 0;

    //! Печатает клише.
    /*!
       Метод печатает на чеке клише, запрограммированное в ККТ.
       ККТ самостоятельно печатает клише на всех документах, предусмотренных технической
       документацией на нее. Но при печати каких-либо дополнительных документов можно
       оформить их «в едином стиле данной ККТ», напечатав в начале документа клише, а в конце
       документа блок атрибутов чека (методом PrintFooter()).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PrintFooter()
     */
    virtual int DTOSHARED_CCA PrintHeader() = 0;

    //! Печатает строку.
    /*!
       Метод служит для печати строки символов \a Caption, выровненной в соответствии с \a Alignment с переносом \a TextWrap
       на чековой ленте/контрольной ленте/электронном журнале.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Caption             <td>Тип ШК (TED::Fptr::BarcodeType)                 <td>put_Caption()
        <tr><td>Alignment           <td>Выравнивание (TED::Fptr::Alignment)             <td>put_Alignment()
        <tr><td>TextWrap            <td>Перенос текста (TED::Fptr::TextWrap)            <td>put_TextWrap()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintString() = 0;

    //! Печатает штрихкод.
    /*!
       Печатает ШК с типом \a BarcodeType и данными \a Barcode с заданными параметрами.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>BarcodeType             <td>Тип ШК (TED::Fptr::BarcodeType)                 <td>put_BarcodeType()
        <tr><td>BarcodePrintType        <td>Способ генерации ШК (TED::Fptr::BarcodePrintType) <td>put_BarcodePrintType()
        <tr><td>Barcode                 <td>Данные ШК                                       <td>put_Barcode()
        <tr><td>BarcodePixelProportions <td>Пропорции пикселя ШК                            <td>put_BarcodePixelProportions()
        <tr><td>BarcodeProportions      <td>Пропорции ШК                                    <td>put_BarcodeProportions()
        <tr><td>BarcodeColumns          <td>Количество столбцов ШК                          <td>put_BarcodeColumns()
        <tr><td>BarcodeRows             <td>Количество строк ШК                             <td>put_BarcodeRows()
        <tr><td>BarcodePackingMode      <td>Режим упаковки ШК                               <td>put_BarcodePackingMode()
        <tr><td>BarcodeUseProportions   <td>Флаг использования пропорций ШК                 <td>put_BarcodeUseProportions()
        <tr><td>BarcodeUseRows          <td>Флаг использования кол-ва строк ШК              <td>put_BarcodeUseRows()
        <tr><td>BarcodeUseColumns       <td>Флаг использования кол-ва столбцов ШК           <td>put_BarcodeUseColumns()
        <tr><td>BarcodeUseCorrection    <td>Флаг использования коррекции ШК                 <td>put_BarcodeUseCorrection()
        <tr><td>BarcodeInvert           <td>Флаг инверсии ШК                                <td>put_BarcodeInvert()
        <tr><td>%Scale                  <td>Масштаб                                         <td>put_Scale()
        <tr><td>BarcodeVersion          <td>Версия ШК                                       <td>put_BarcodeVersion()
        <tr><td>BarcodeCorrection       <td>Коррекция ШК (TED::Fptr::BarcodeQRCorrection)   <td>put_BarcodeCorrection()
        <tr><td>BarcodeEncoding         <td>Кодировка ШК (TED::Fptr::BarcodeQREncoding)     <td>put_BarcodeEncoding()
        <tr><td>BarcodeEncodingMode     <td>Режим кодировки ШК (TED::Fptr::BarcodeQREncodingMode) <td>put_BarcodeEncodingMode()
        <tr><td>PrintBarcodeText        <td>Флаг печати данных ШК                           <td>put_PrintBarcodeText()
        <tr><td>Height                  <td>Высота ШК                                       <td>put_Height()
        <tr><td>BarcodeOverlay          <td>Флаг печати ШК поверх текста                    <td>put_BarcodeOverlay
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintBarcode() = 0;

    //! Печатает блок атрибутов чека.
    /*!
       Метод печатает на чеке блок атрибутов чека, идентичный тому, который печатается в отчетах без гашения.
       При печати каких-либо дополнительных отчетов можно оформить их «в едином стиле
       данной ККТ», напечатав в начале документа клише (методом PrintHeader()), а в конце
       документа блок атрибутов чека.

       Работает из режима регистрации (TED::Fptr::ModeReportNoClear).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PrintHeader()
     */
    virtual int DTOSHARED_CCA PrintFooter() = 0;

    //! Возвращает выравнивание.
    /*!
       \param value Выравнивание
       \retval 0 Успех
       \sa put_Alignment()
       \sa TED::Fptr::Alignment
     */
    virtual int DTOSHARED_CCA get_Alignment(int *value) = 0;

    //! Устанавливает выравнивание.
    /*!
       \param value Выравнивание
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Alignment()
       \sa TED::Fptr::Alignment
     */
    virtual int DTOSHARED_CCA put_Alignment(int value) = 0;

    //! Возвращает перенос текста.
    /*!
       \param value Перенос текста
       \retval 0 Успех
       \sa put_TextWrap()
       \sa TED::Fptr::TextWrap
     */
    virtual int DTOSHARED_CCA get_TextWrap(int *value) = 0;

    //! Устанавливает перенос.
    /*!
       \param value Перенос текста
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TextWrap()
       \sa TED::Fptr::TextWrap
     */
    virtual int DTOSHARED_CCA put_TextWrap(int value) = 0;

    //! Возвращает данные штрихкода.
    /*!
       \param bfr Буфер для данных штрихкода
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_Barcode()
     */
    virtual int DTOSHARED_CCA get_Barcode(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает данные штрихкода.
    /*!
       \param value Данные штрихкода
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Barcode()
     */
    virtual int DTOSHARED_CCA put_Barcode(const wchar_t *value) = 0;

    //! Возвращает тип штрихкода.
    /*!
       \param value Тип штрихкода
       \retval 0 Успех
       \sa put_BarcodeType()
       \sa TED::Fptr::BarcodeType
     */
    virtual int DTOSHARED_CCA get_BarcodeType(int *value) = 0;

    //! Устанавливает тип штрихкода.
    /*!
       \param value Тип штрихкода
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeType()
       \sa TED::Fptr::BarcodeType
     */
    virtual int DTOSHARED_CCA put_BarcodeType(int value) = 0;

    //! Возвращает флаг печати данных штрихкода.
    /*!
       \param value Флаг печати данных штрихкода (1 (true) - печатать, 0 (false) - не печатать)
       \retval 0 Успех
       \sa put_PrintBarcodeText()
     */
    virtual int DTOSHARED_CCA get_PrintBarcodeText(int *value) = 0;

    //! Устанавливает флаг печати данных штрихкода.
    /*!
       \param value Флаг печати данных штрихкода (1 (true) - печатать, 0 (false) - не печатать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PrintBarcodeText()
     */
    virtual int DTOSHARED_CCA put_PrintBarcodeText(int value) = 0;

    //! Возвращает ориентацию подкладного документа.
    /*!
       \param value Ориентация документа
       \retval 0 Успех
       \sa put_SlipDocOrientation()
     */
    virtual int DTOSHARED_CCA get_SlipDocOrientation(int *value) = 0;

    //! Устанавливает ориентацию подкладного документа.
    /*!
       \param value Ориентация документа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_SlipDocOrientation()
     */
    virtual int DTOSHARED_CCA put_SlipDocOrientation(int value) = 0;

    //! Возвращает масштаб.
    /*!
       \param value Масштаб
       \retval 0 Успех
       \sa put_Scale()
     */
    virtual int DTOSHARED_CCA get_Scale(double *value) = 0;

    //! Устанавливает масштаб.
    /*!
       \param value Масштаб
       \retval -1 Ошибка
       \retval 0 Успех
       \return Код результата
       \sa get_Scale()
     */
    virtual int DTOSHARED_CCA put_Scale(double value) = 0;

    //! Возвращает высоту.
    /*!
       \param value Высота
       \retval 0 Успех
       \sa put_Height()
     */
    virtual int DTOSHARED_CCA get_Height(int *value) = 0;

    //! Устанавливает высоту.
    /*!
       \param value Высота
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Height()
     */
    virtual int DTOSHARED_CCA put_Height(int value) = 0;

    //! Производит регистрацию оплаты.
    /*!
       Регистрирует оплату с суммой \a Summ и типом \a TypeClose. В свойства \a Remainder и \a Change
       записываются остаток к оплате и сдача соответственно.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма                   <td>put_Summ()
        <tr><td>TypeClose       <td>Тип оплаты              <td>put_TypeClose()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Remainder       <td>Остаток к оплате        <td>get_Remainder()
        <tr><td>Change          <td>Сдача                   <td>get_Change()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Payment() = 0;

    //! Запрашивает сумму наличный в денежном ящике.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>Summ        <td>Сумма        <td>get_Summ()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetSumm() = 0;

    //! Возвращает тип платежа.
    /*!
       \param value Тип платежа
       \retval 0 Успех
       \sa put_TypeClose()
     */
    virtual int DTOSHARED_CCA get_TypeClose(int *value) = 0;

    //! Устанавливает тип платежа.
    /*!
       \param value Тип платежа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TypeClose()
     */
    virtual int DTOSHARED_CCA put_TypeClose(int value) = 0;

    //! Возвращает сумму.
    /*!
       \param value Сумма
       \return Код результата
       \sa put_Summ()
     */
    virtual int DTOSHARED_CCA get_Summ(double *value) = 0;

    //! Устанавливает сумму.
    /*!
       \param value Сумма
       \return Код результата
       \sa get_Summ()
     */
    virtual int DTOSHARED_CCA put_Summ(double value) = 0;

    //! Открывает чек в ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CheckType       <td>Тип чека (TED::Fptr::ChequeType)    <td>put_CheckType()
        <tr><td>PrintCheck      <td>Печатать чек                        <td>put_PrintCheck()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме         <td>put_TestMode()
       </table>

       При работе с ФЗ-54-совместимыми ККТ, команда расширена дополнительными параметрами:
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>PrintCheck      <td>Печатать чек                        <td>put_PrintCheck()
       </table>

       Для печати электронного чека необходимо:
       - выставить флаг PrintCheck в значение 0 (true)
       - после выполнения операции открытия чека сразу назначить реквизит 1008 (адрес покупателя)
       \code{.cpp}
        fptr->put_CheckType(TED::Fptr::ChequeSell);
        fptr->put_PrintCheck(0);
        fptr->OpenCheck();

        fptr->put_FiscalPropertyNumber(1008);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"user@mail.ru");
        fptr->WriteFiscalProperty();

        ...
       \endcode

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA OpenCheck() = 0;

    //! Закрывает чек в ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>TypeClose       <td>Тип оплаты                      <td>put_TypeClose()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA CloseCheck() = 0;

    //! Аннулирует чек.
    /*!
       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA CancelCheck() = 0;

    //! Запрашивает значение регистра.
    /*!
       Запрашивает значение регистра \a RegisterNumber с заданными параметрами.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>RegisterNumber      <td>Номер регистра                      <td>put_RegisterNumber()
        <tr><td>CheckType           <td>Тип чека (TED::Fptr::ChequeType)    <td>put_CheckType()
        <tr><td>TypeClose           <td>Тип оплаты                          <td>put_TypeClose()
        <tr><td>OperationType       <td>Тип операции                        <td>put_OperationType()
        <tr><td>CounterDimension    <td>Размерность счетчика (TED::Fptr::CounterDimension)<td>put_CounterDimension()
        <tr><td>TaxNumber           <td>Номер налога                        <td>put_TaxNumber()
        <tr><td>DiscountNumber      <td>Номер скидки/надбавки               <td>put_DiscountNumber()
        <tr><td>CounterType         <td>Тип счетчика                        <td>put_CounterType()
        <tr><td>StepCounterType     <td>Тип счетчика шагов                  <td>put_StepCounterType()
        <tr><td>PowerSupplyType     <td>Тип источника питания               <td>put_PowerSupplyType()
        <tr><td>Department          <td>Номер секции                        <td>put_Department()
        <tr><td>Count               <td>Счетчик                             <td>put_Count()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Count               <td>Счетчик                             <td>get_Count()
        <tr><td>Date                <td>Дата                                <td>get_Date()
        <tr><td>Time                <td>Время                               <td>get_Time()
        <tr><td>SessionOpened       <td>Флаг состояния смены                <td>get_SessionOpened()
        <tr><td>Mode                <td>Режим (TED::Fptr::Mode)             <td>get_Mode()
        <tr><td>AdvancedMode        <td>Подрежим                            <td>get_AdvancedMode()
        <tr><td>CheckState          <td>Состояние чека (TED::Fptr::ChequeType) <td>get_CheckState()
        <tr><td>CheckNumber         <td>Номер чека                          <td>get_CheckNumber()
        <tr><td>DocNumber           <td>Сквозной номер документа            <td>get_DocNumber()
        <tr><td>Summ                <td>Сумма                               <td>get_Summ()
        <tr><td>Remainder           <td>Остаток к оплате                    <td>get_Remainder()
        <tr><td>Change              <td>Сдача                               <td>get_Change()
        <tr><td>Session             <td>Номер смены                         <td>get_Session()
        <tr><td>SerialNumber        <td>Серийный номер                      <td>get_SerialNumber()
        <tr><td>Model               <td>Номер модели                        <td>get_Model()
        <tr><td>VerHi               <td>Версия                              <td>get_VerHi()
        <tr><td>VerLo               <td>Подверсия                           <td>get_VerLo()
        <tr><td>RcpCharLineLength   <td>Ширина ЧЛ в символах                <td>get_RcpCharLineLength()
        <tr><td>RcpPixelLineLength  <td>Ширина ЧЛ в точках                  <td>get_RcpPixelLineLength()
        <tr><td>JrnCharLineLength   <td>Ширина КЛ в символах                <td>get_JrnCharLineLength()
        <tr><td>JrnPixelLineLength  <td>Ширина КЛ в точках                  <td>get_JrnPixelLineLength()
        <tr><td>SlipCharLineLength  <td>Ширина ПД в символах                <td>get_SlipCharLineLength()
        <tr><td>SlipPixelLineLength <td>Ширина ПД в точках                  <td>get_SlipPixelLineLength()
        <tr><td>INN                 <td>ИНН                                 <td>get_INN()
        <tr><td>MachineNumber       <td>Регистрационный номер               <td>get_MachineNumber()
        <tr><td>PowerSupplyState    <td>Состояние источника питания         <td>get_PowerSupplyState()
        <tr><td>PowerSupplyValue    <td>Напряжение источника питания        <td>get_PowerSupplyValue()
        <tr><td>DiscountInSession   <td>Сумма скидок за смену               <td>get_DiscountInSession()
        <tr><td>ChargeInSession     <td>Сумма надбавок за смену             <td>get_ChargeInSession()
        <tr><td>NetworkError        <td>Код ошибки сети                     <td>get_NetworkError()
        <tr><td>OFDError            <td>Код ошибки ОФД                      <td>get_OFDError()
        <tr><td>FNError             <td>Код ошибки ФН                       <td>get_FNError()
        <tr><td>DateEnd             <td>Дата окончания                      <td>get_DateEnd()
        <tr><td>TimeEnd             <td>Время окончания                     <td>get_TimeEnd()
        <tr><td>ENVDMode            <td>Режим ЕНВД                          <td>get_ENVDMode()
        <tr><td>ENVDEnabled         <td>Разрешение режима ЕНВД              <td>get_ENVDEnabled()
        <tr><td>TaxNumeration       <td>Режим работы с налоговыми ставками  <td>get_TaxNumeration()
       </table>
       <br>

       <table>
        <caption>Список регистров</caption>
        <tr><th rowspan=2>Номер<th colspan=11>Дополнительный параметр<th rowspan=2>Свойства
        <tr>        <th>CheckType<th>TypeClose<th>OperationType<th>CounterDimension<th>TaxNumber<th>DiscountNumber<th>CounterType<th>StepCounterType<th>PowerSupplyType<th>Department<th>Count

        <tr><td>1   <td>+<td><td><td><td><td><td><td><td><td><td><td>Сумма всех регистраций (\a Summ)

        <tr><td>2   <td>+<td><td><td><td><td><td><td><td><td><td><td>Сумма всех сторно (\a Summ)

        <tr><td>3   <td>+<td>+<td><td><td><td><td><td><td><td><td><td>Сумма всех платежей (\a Summ)

        <tr><td>4   <td><td><td><td><td><td><td><td><td><td><td><td>Сумма внесений наличных денег (\a Summ)

        <tr><td>5   <td><td><td><td><td><td><td><td><td><td><td><td>Сумма выплат наличных денег (\a Summ)

        <tr><td>6   <td>+<td><td><td><td><td><td><td><td><td><td><td>Количество всех регистраций (\a Count)

        <tr><td>7   <td>+<td><td><td><td><td><td><td><td><td><td><td>Количество всех сторно (\a Count)

        <tr><td>8   <td><td><td><td><td><td><td><td><td><td><td><td>Количество всех внесений наличных денег (\a Count)

        <tr><td>9   <td><td><td><td><td><td><td><td><td><td><td><td>Количество всех выплат наличных денег (\a Count)

        <tr><td>10  <td><td><td><td><td><td><td><td><td><td><td><td>Сумма наличности в ККТ (\a Summ)

        <tr><td>11  <td><td><td><td><td><td><td><td><td><td><td><td>Выручка (\a Summ)

        <tr><td>12  <td><td><td>+<td><td><td><td><td><td><td><td><td>Сумма сменного итога (\a Summ)

        <tr><td>13  <td><td><td>+<td>+<td><td><td><td><td><td><td><td>Необнуляемая сумма по всем записям фискальной памяти + сменный итог текущей смены (\a Summ)

        <tr><td>14  <td><td><td>+<td>+<td><td><td><td><td><td><td><td>Необнуляемая сумма после последней перерегистрации + сменный итог текущей смены (\a Summ)

        <tr><td>15  <td><td><td><td><td><td><td><td><td><td><td><td>Количество оставшихся перерегистраций (\a Count)

        <tr><td>16  <td><td><td><td><td><td><td><td><td><td><td><td>Количество оставшихся сменных записей в фискальной памяти ККТ (\a Count)

        <tr><td>17  <td><td><td><td><td><td><td><td><td><td><td><td>Текущие дата и время в ККТ (\a Date, \a Time)

        <tr><td>18  <td><td><td><td><td><td><td><td><td><td><td><td>Флаг открытости смены (\a SessionOpened)
        <br>Дата и время окончания текущей смены (\a Date, \a Time)

        <tr><td>19  <td><td><td><td><td><td><td><td><td><td><td><td>Режим работы ККТ (\a Mode)
        <br>Номер текущего чека (\a CheckNumber)
        <br>Состояние текущего чека (\a CheckState)
        <br>Сквозной номер документа (\a DocNumber)

        <tr><td>20  <td><td><td><td><td><td><td><td><td><td><td><td>Текущая сумма чека (\a Summ)
        <br>Остаток чека (\a Remainder)
        <br>Сумма сдачи (\a Change)

        <tr><td>21  <td><td><td><td><td><td><td><td><td><td><td><td>Номер смены (\a Session)

        <tr><td>22  <td><td><td><td><td><td><td><td><td><td><td><td>Заводской номер ККТ (\a SerialNumber)

        <tr><td>23  <td><td><td><td><td><td><td><td><td><td><td><td>Модель ККТ (\a Model)
        <br>Версия и подверсия ККТ (\a VerHi, \a VerLo)

        <tr><td>24  <td><td><td><td><td><td><td><td><td><td><td><td>Параметры печатающего устройства (\a RcpCharLineLength, \a RcpPixelLineLength, \a JrnCharLineLength, \a JrnPixelLineLength, \a SlipCharLineLength, \a SlipPixelLineLength)

        <tr><td>25  <td><td><td><td><td><td><td><td><td><td><td><td>Длина входного буфера ККТ (\a Count)

        <tr><td>26  <td><td><td><td><td><td><td><td><td><td><td><td>Количество оставшихся активизаций ЭКЛЗ (\a Count)

        <tr><td>27  <td><td><td><td><td><td><td><td><td><td><td><td>Параметры фискализации:
        - ИНН (\a INN);
        - РНМ (\a MachineNumber);
        - Номер смены (\a Session);
        - Дата (\a Date)

        <tr><td>28  <td><td><td><td><td><td><td><td><td><td><td><td>Параметры ЭКЛЗ:
        - номер ЭКЛЗ (\a SerialNumber);
        - номер смены активизации (\a Session);
        - Дата активизации(\a Date)

        <tr><td>29  <td><td><td><td><td><td><td><td><td><td><td><td>Фискальная станция (\a PrintPurpose)

        <tr><td>30  <td><td><td>+<td><td><td><td><td><td><td><td><td>Сумма последней записи в ФП (\a Summ)

        <tr><td>31  <td>+<td><td><td><td>+<td><td><td><td><td><td><td>Счетчик зарегистрированных налогов (\a Summ)

        <tr><td>32  <td><td><td><td><td><td>+<td><td><td><td><td><td>Суммовой счётчик зарегистрированных скидок/надбавок за смену (\a Summ)

        <tr><td>33  <td><td><td><td><td><td><td>+<td><td><td><td><td>Количество отрезов (\a Count)

        <tr><td>34  <td><td><td><td><td><td><td>+<td>+<td><td><td><td>Количество шагов двигателя промотки бумаги (\a Count)

        <tr><td>35  <td><td><td><td><td><td><td>+<td><td><td><td><td>Количество циклов нагрева печатающей головки (\a Count)

        <tr><td>36  <td><td><td><td><td><td><td><td><td><td><td><td>Версия и сборка электронного модуля (\a VerHi, \a VerLo)

        <tr><td>38  <td><td><td><td><td><td><td><td><td>+<td><td><td>Состояние источника питания (\a PowerSupplyState)
        <br>Напряжение источника питания (\a PowerSupplyValue)
        <br>Процент заряда аккумулятора (\a BatteryCharge)

        <tr><td>39  <td><td><td><td><td><td><td><td><td><td><td><td>Температура ТПМ (\a Value)

        <tr><td>40  <td>+<td><td><td><td><td><td><td><td><td><td><td>Необнуляемая сумма (\a Summ)

        <tr><td>41  <td>+<td><td><td><td><td><td><td><td><td>+<td><td>Сменный оборот налога по секциям (\a Value)
        <br>Сумма налогов по секциям (\a Summ)

        <tr><td>42  <td>+<td><td><td><td><td><td><td><td><td><td><td>Сумма скидок за смену(\a DiscountInSession)
        <br>Сумма надбавок за смену (\a ChargeInSession)

        <tr><td>43  <td><td><td><td><td><td><td><td><td><td><td><td>Код ошибки обмена ОФД:
        - Код ошибки сети (\a NetworkError)
        - Код ошибки ОФД (\a OFDError)
        - Код ошибки ФН (\a FNError)

        <tr><td>44  <td><td><td><td><td><td><td><td><td><td><td><td>Количество не отправленных документов в ФН (\a Count)

        <tr><td>45  <td><td><td><td><td><td><td><td><td><td><td><td>Дата и время самого раннего не отправленного документа в ФН (\a Date, \a Time)

        <tr><td>46  <td>+<td><td><td><td><td><td><td><td><td><td><td>Сумма аннулированных чеков (\a Summ)

        <tr><td>47  <td><td><td><td><td><td><td><td><td><td><td><td>Номер ФН (\a SerialNumber)
        <br>Состояние ФН (\a FNState)

        <tr><td>48  <td><td><td><td><td><td><td><td><td><td><td><td>Дата и время последней перерегистрации (\a Date, \a Time)
        <br>Номер ФД последней регистрации/перерегистрации (\a DocNumber>

        <tr><td>51  <td><td><td><td><td><td><td><td><td><td><td><td>Номер фискального документа последнего сформированного чека (\a DocNumber)
        <br>Тип чека (\a CheckType)
        <br>Итог (\a Summ)
        <br>Дата и время чека (\a Date, \a Time)
        <br>Фискальный признак (\a Value)

        <tr><td>52  <td><td><td><td><td><td><td><td><td><td><td><td>Номер последнего ФД (\a DocNumber)
        <br>Дата и время последнего ФД (\a Date, \a Time)
        <br>Фискальный признак последнего ФД (\a Value)

        <tr><td>53  <td><td><td><td><td><td><td><td><td><td><td><td>Номер чека в ФН за смену (\a CheckNumber)
        <br>Номер смены в ФН (\a Session)

        <tr><td>54  <td><td><td><td><td><td><td><td><td><td><td><td>Версия ФФД ККТ (\a DeviceFfdVersion)
        <br>Версия ФФД ФН (\a FNFfdVersion)
        <br>Версия ФФД (\a FfdVersion)
        <br>Дата документа ФФД (\a Date)

        <tr><td>55  <td><td><td><td><td><td><td><td><td><td><td><td>Код команды, на которой произошла ошибка (\a CommandCode)
        <br>Код ошибки (\a ErrorCode)
        <br>Данные ошибки (\a ErrorData)

        <tr><td>56  <td>+<td>+<td><td><td><td><td><td><td><td><td><td>Необнуляемая сумма по типам оплат (\a Summ)

        <tr><td>57  <td><td><td><td><td><td><td><td><td><td><td><td>Дата и время отправки последнего документа в ОФД (\a Date, \a Time)

        <tr><td>58  <td><td><td><td><td><td><td><td><td><td><td><td>Общий счетчик кол-ва расчетных документов с момента общего гашения (\a Count)
        <br>Количество ФД за смену (\a DocNumber)

        <tr><td>59  <td><td><td><td><td><td><td><td><td><td><td><td> Режим ЕНВД (\a ENVDMode)
        <br>Разрешение режима ЕНВД (\a ENVDEnabled)
        <br>Режим работы с налоговыми ставками (\a TaxNumeration)

        <tr><td>60  <td>+<td><td><td><td>+<td><td><td><td><td><td><td> Сумма налога за смену (\a Summ)

        <tr><td>61  <td><td><td><td><td>+<td><td><td><td><td><td><td> Сумма налога на чек (\a Summ)

        <tr><td>63  <td><td><td><td><td><td><td><td><td><td><td><td> Счетчик количества отчетов об открытии и закрытии смены в буфере (\a Count)

        <tr><td>65  <td>+<td><td><td><td><td><td><td><td><td><td><td> Количество закрытых чеков (\a Count)
        <br> Количества отмененных чеков (\a Value)

        <tr><td>66  <td><td><td><td><td><td><td><td><td><td><td><td> Количество байт, переданных в открытом чеке (\a Count)

        <tr><td>67  <td><td><td><td><td><td><td><td><td><td><td><td> Количество секунд с включения ККТ (\a Value)

        <tr><td>70  <td><td><td><td><td><td><td><td><td><td><td><td> Дата и время открытия предыдущей смены (\a Date, \a Time)
        <br>Дата и время закрытия предыдущей смены (\a DateEnd, \a TimeEnd)

        <tr><td><th>CheckType<th>TypeClose<th>OperationType<th>CounterDimension<th>TaxNumber<th>DiscountNumber<th>CounterType<th>StepCounterType<th>PowerSupplyType<th>Department<th>Count<td>
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetRegister() = 0;

    //! Возвращает тип чека.
    /*!
       \param value Тип чека
       \retval 0 Успех
       \sa put_CheckType()
       \sa ChequeType
     */
    virtual int DTOSHARED_CCA get_CheckType(int *value) = 0;

    //! Устанавливает тип чека.
    /*!
       \param value Тип чека
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CheckType()
       \sa ChequeType
     */
    virtual int DTOSHARED_CCA put_CheckType(int value) = 0;

    //! Возвращает состояние чека.
    /*!
       \param value Состояние чека
       \retval 0 Успех
       \sa ChequeType
     */
    virtual int DTOSHARED_CCA get_CheckState(int *value) = 0;

    //! Возвращает номер чека.
    /*!
       \param value Номер чека
       \retval 0 Успех
       \sa put_CheckNumber()
     */
    virtual int DTOSHARED_CCA get_CheckNumber(int *value) = 0;

    //! Устанавливает номер чека.
    /*!
       \param value Номер чека
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CheckNumber()
     */
    virtual int DTOSHARED_CCA put_CheckNumber(int value) = 0;

    //! Возвращает номер регистра.
    /*!
       \param value Номер регистра
       \retval 0 Успех
       \sa put_RegisterNumber()
     */
    virtual int DTOSHARED_CCA get_RegisterNumber(int *value) = 0;

    //! Устанавливает номер регистра.
    /*!
       \param value Номер регистра
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_RegisterNumber()
     */
    virtual int DTOSHARED_CCA put_RegisterNumber(int value) = 0;

    //! Возвращает номер документа.
    /*!
       \param value Номер документа
       \retval 0 Успех
       \sa put_DocNumber()
     */
    virtual int DTOSHARED_CCA get_DocNumber(int *value) = 0;

    //! Устанавливает номер документа.
    /*!
       \param value Номер документа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DocNumber()
     */
    virtual int DTOSHARED_CCA put_DocNumber(int value) = 0;

    //! Открывает смену на ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Caption         <td>Строка (будет печататься в теле документа открытия смены)<td>put_Caption()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA OpenSession() = 0;

    //! Возвращает флаг состояния смены.
    /*!
       \param value Флаг состояния смены (1 (true) - открыта, 0 (false) - закрыта)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_SessionOpened(int *value) = 0;

    //! Возвращает номер смены.
    /*!
       \param value Номер смены
       \retval 0 Успех
       \sa put_Session()
     */
    virtual int DTOSHARED_CCA get_Session(int *value) = 0;

    //! Устанавливает номер смены.
    /*!
       \param value Номер смены
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Session()
     */
    virtual int DTOSHARED_CCA put_Session(int value) = 0;

    //! Возвращает флаг наличия бумаги в принтере чеков.
    /*!
       \param value Флаг наличия бумаги (1 (true) - бумага есть, 0 (false) - бумаги нет)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_CheckPaperPresent(int *value) = 0;

    //! Возвращает флаг наличия бумаги в принтере контрольной ленты.
    /*!
       \param value Флаг наличия бумаги (1 (true) - бумага есть, 0 (false) - бумаги нет)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ControlPaperPresent(int *value) = 0;

    //! Возвращает внутренний код товара.
    /*!
       \warning Не используется в текущей версии
       \param value Код товара
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PLUNumber(int *value) = 0;

    //! Устанавливает внутренний код товара.
    /*!
       \warning Не используется в текущей версии
       \param value Код товара
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_PLUNumber(int value) = 0;

    //! Возвращает название.
    /*!
       \param bfr Буфер для идентификатора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
       \sa put_Name()
     */
    virtual int DTOSHARED_CCA get_Name(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает название.
    /*!
       \param value Название
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Name()
     */
    virtual int DTOSHARED_CCA put_Name(const wchar_t *value) = 0;

    //! Возвращает цену.
    /*!
       \param value Цена
       \retval 0 Успех
       \sa put_Price()
     */
    virtual int DTOSHARED_CCA get_Price(double *value) = 0;

    //! Устанавливает цену.
    /*!
       \param value Цена
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Price()
     */
    virtual int DTOSHARED_CCA put_Price(double value) = 0;

    //! Возвращает количество.
    /*!
       \param value Количество
       \retval 0 Успех
       \sa put_Quantity()
     */
    virtual int DTOSHARED_CCA get_Quantity(double *value) = 0;

    //! Устанавливает количество.
    /*!
       \param value Количество
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Quantity()
     */
    virtual int DTOSHARED_CCA put_Quantity(double value) = 0;

    //! Возвращает номер секции.
    /*!
       \param value Номер секции
       \retval 0 Успех
       \sa put_Department()
     */
    virtual int DTOSHARED_CCA get_Department(int *value) = 0;

    //! Устанавливает номер секции.
    /*!
       \param value Номер секции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Department()
     */
    virtual int DTOSHARED_CCA put_Department(int value) = 0;

    //! Возвращает тип скидки.
    /*!
       \param value Тип скидки
       \retval 0 Успех
       \sa put_DiscountType()
       \sa DiscountType
     */
    virtual int DTOSHARED_CCA get_DiscountType(int *value) = 0;

    //! Устанавливает тип скидки.
    /*!
       \param value Тип скидки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DiscountType()
       \sa DiscountType
     */
    virtual int DTOSHARED_CCA put_DiscountType(int value) = 0;

    //! Производит регистрацию продажи / прихода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       При работе с ФЗ-54-совместимыми ККТ, команда регистрации расширена дополнительными параметрами.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
        <tr><td>PositionQuantityType    <td>Тип печатаемого количества  <td>put_PositionQuantityType()
        <tr><td>UseOnlyTaxNumber <td>Использовать только ставку налога <td>put_UseOnlyTaxNumber()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек продажи / прихода.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Registration() = 0;

    //! Производит регистрацию аннулирования продажи.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>EnableCheckSumm <td>Проверять наличность            <td>put_EnableCheckSumm()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       Первая операция открывает чек аннулирования продажи.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Annulate() = 0;

    //! Производит регистрацию возврата продажи / прихода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>EnableCheckSumm <td>Проверять наличность            <td>put_EnableCheckSumm()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       При работе с ФЗ-54-совместимыми ККТ, команда регистрации расширена дополнительными параметрами.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber()
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
        <tr><td>PositionQuantityType    <td>Тип печатаемого количества  <td>put_PositionQuantityType()
        <tr><td>UseOnlyTaxNumber <td>Использовать только ставку налога <td>put_UseOnlyTaxNumber()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек возврата продажи / прихода.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Return() = 0;

    //! Производит сторнирование позиции.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       При работе с ФЗ-54-совместимыми ККТ, команда запрещена.

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Storno() = 0;

    //! Производит регистрацию покупки / расхода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       При работе с ФЗ-54-совместимыми ККТ, команда регистрации расширена дополнительными параметрами.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
        <tr><td>PositionQuantityType    <td>Тип печатаемого количества  <td>put_PositionQuantityType()
        <tr><td>UseOnlyTaxNumber <td>Использовать только ставку налога <td>put_UseOnlyTaxNumber()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек покупки / расхода.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Buy() = 0;

    //! Производит регистрацию возврата покупки / расхода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       При работе с ФЗ-54-совместимыми ККТ, команда регистрации расширена дополнительными параметрами.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
        <tr><td>PositionQuantityType    <td>Тип печатаемого количества  <td>put_PositionQuantityType()
        <tr><td>UseOnlyTaxNumber <td>Использовать только ставку налога <td>put_UseOnlyTaxNumber()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек возврата покупки / расхода.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA BuyReturn() = 0;

    //! Производит регистрацию аннулирования покупки.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Alignment       <td>Выравнивание наименования товара<td>put_Alignment()
        <tr><td>TextWrap        <td>Перенос наименования товара     <td>put_TextWrap()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       Первая операция открывает чек аннулирования покупки.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA BuyAnnulate() = 0;

    //! Производит регистрацию внесения наличных в кассу.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма                           <td>put_Summ()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa CashOutcome()
     */
    virtual int DTOSHARED_CCA CashIncome() = 0;

    //! Производит регистрацию выплаты наличных из кассы.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма                           <td>put_Summ()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa CashIncome()
     */
    virtual int DTOSHARED_CCA CashOutcome() = 0;

    //! Печатает указанный отчет.
    /*!
       Метод печатает указанный отчет на ККТ.

       Логика работы драйвера и тип снимаемого отчета определяются содержимым свойства \a ReportType.

       Метод Report() возвращает управление вызвавшему его клиенту только после завершения
       печати всего отчета или возникновения ошибки. Некоторые отчеты могут выполняться
       несколько минут.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReportType      <td>Тип отчета (TED::Fptr::ReportType)  <td>put_ReportType()
        <tr><td>Date            <td>Дата начала диапазона               <td>put_Date()
        <tr><td>DateEnd         <td>Дата конца диапазона                <td>put_DateEnd()
        <tr><td>Session         <td>Начальная смена диапазона           <td>put_Session()
        <tr><td>SessionEnd      <td>Конечная смена диапазона            <td>put_SessionEnd()
        <tr><td>EKLZKPKNumber   <td>Номер КПК                           <td>put_EKLZKPKNumber()
        <tr><td>DocNumber       <td>Начальный номер документа           <td>put_DocNumber()
        <tr><td>DocNumberEnd    <td>Конечный номер документа            <td>put_DocNumberEnd()
        <tr><td>ClearFlag       <td>Флаг очистки журнала                <td>put_ClearFlag()
       </table>


       <table>
        <caption>Список отчетов</caption>
        <tr><th>Номер отчета                                   <th>Описание<th>Входные свойства<th>Режим
        <tr><td>TED::Fptr::ReportCRCLear                       <td>Гашение контрольной ленты<td><td>TED::Fptr::ModeReportClear
        <tr><td>TED::Fptr::ReportZ                             <td>Суточный отчет с гашением<td><td>TED::Fptr::ModeReportClear
        <tr><td>TED::Fptr::ReportX                             <td>Суточный отчет без гашения<td><td>TED::Fptr::ModeReportNoClear
        <tr><td>TED::Fptr::ReportFiscalDatesReduced            <td>Краткий фискальный отчет по диапазону дат<td>\a Date, \a DateEnd<td>TED::Fptr::ModeFiscalMemory
        <tr><td>TED::Fptr::ReportFiscalSessionsReduced         <td>Краткий фискальный отчет по диапазону смен<td>\a Session, \a SessionEnd<td>TED::Fptr::ModeFiscalMemory
        <tr><td>TED::Fptr::ReportFiscalDatesFull               <td>Полный фискальный отчет по диапазону дат<td>\a Date, \a DateEnd<td>TED::Fptr::ModeFiscalMemory
        <tr><td>TED::Fptr::ReportFiscalSessionsFull            <td>Полный фискальный отчет по диапазону смен<td>\a Session, \a SessionEnd<td>TED::Fptr::ModeFiscalMemory
        <tr><td>TED::Fptr::ReportDepartment                    <td>Отчет по секциям<td><td>TED::Fptr::ModeReportNoClear
        <tr><td>TED::Fptr::ReportCashiers                      <td>Отчет по кассирам<td><td>TED::Fptr::ModeReportNoClear
        <tr><td>TED::Fptr::ReportHours                         <td>Отчет по часам<td><td>TED::Fptr::ModeReportNoClear
        <tr><td>TED::Fptr::ReportQuantity                      <td>Отчет по кол-ву<td><td>TED::Fptr::ModeReportNoClear
        <tr><td>TED::Fptr::ReportEKLZActivationTotal           <td>ЭКЛЗ итоги активации<td><td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZSessionTotal              <td>ЭКЛЗ итоги смены<td>\a Session<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZSessionCR                 <td>ЭКЛЗ контрольная лента смены<td>\a Session<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZKPKDoc                    <td>ЭКЛЗ документ по номеру КПК<td>\a EKLZKPKNumber<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZDatesDepartmentsReduced   <td>ЭКЛЗ по датам краткий по секциям<td>\a Date, \a DateEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZDatesDepartmentsFull      <td>ЭКЛЗ по датам полный по секциям<td>\a Date, \a DateEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZDatesSessionsTotalReduced <td>ЭКЛЗ по датам краткий по итогам смен<td>\a Date, \a DateEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZDatesSessionsTotalFull    <td>ЭКЛЗ по датам полный по итогам смен<td>\a Date, \a DateEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZSessionsDepartmentsReduced <td>ЭКЛЗ по сменам краткий по секциям<td>\a Session, \a SessionEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZSessionsDepartmentsFull   <td>ЭКЛЗ по сменам полный по секциям<td>\a Session, \a SessionEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZSessionsTotalReduced      <td>ЭКЛЗ по сменам краткий по итогам смен<td>\a Session, \a SessionEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportEKLZSessionsTotalFull         <td>ЭКЛЗ по сменам полный по итогам смен<td>\a Session, \a SessionEnd<td>TED::Fptr::ModeEKLZ
        <tr><td>TED::Fptr::ReportDocumentByNumber              <td>Печать документа из ЭЖ по номеру<td>\a DocNumber<td>TED::Fptr::ModeReportClear
        <tr><td>TED::Fptr::ReportCRPrintFull                   <td>Печать ЭЖ полностью<td>\a ClearFlag<td>TED::Fptr::ModeReportClear
        <tr><td>TED::Fptr::ReportCRPrintReduced                <td>Печать ЭЖ сокращенно<td>\a ClearFlag<td>TED::Fptr::ModeReportClear
        <tr><td>TED::Fptr::ReportService                       <td>Служебный отчет<td><td>
        <tr><td>TED::Fptr::ReportSD                            <td>Электронный отчет с SD<td><td>
        <tr><td>TED::Fptr::ReportAccountingState               <td>Состояние расчетов<td><td>TED::Fptr::ModeReportNoClear
        <tr><td>TED::Fptr::ReportTestDevice                    <td>Тестирование ККТ<td><td>
        <tr><td>TED::Fptr::ReportPrintInfo                     <td>Печать информации о ККТ<td><td>
        <tr><td>TED::Fptr::ReportOfdConnectionDiagnostic       <td>Диагностика соединения с ОФД<td><td>
        <tr><td>TED::Fptr::ReportSessionTotalCounters          <td>Счетчики итогов смены<td><td>
        <tr><td>TED::Fptr::ReportFNTotalCounters               <td>Счетчики итогов ФН<td><td>
        <tr><td>TED::Fptr::ReportNotSentDocumentsCounters      <td>Счетчики по непереданным ФД<td><td>
        <tr><td>TED::Fptr::ReportPrintDocumentsFromJournalByNumbers     <td>ЭЖ по диапазону документов<td>\a DocNumber, \a DocNumberEnd<td>
        <tr><td>TED::Fptr::ReportPrintDocumentsFromJournalBySessions    <td>ЭЖ по диапазону смен<td>\a Session, \a SessionEnd<td>
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Report() = 0;

    //! Возвращает тип отчета.
    /*!
       \param value Тип отчета
       \retval 0 Успех
       \sa put_ReportType()
       \sa ReportType
     */
    virtual int DTOSHARED_CCA get_ReportType(int *value) = 0;

    //! Устанавливает тип отчета.
    /*!
       \param value Тип отчета
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportType()
       \sa ReportType
     */
    virtual int DTOSHARED_CCA put_ReportType(int value) = 0;

    //! Возвращает флаг буферезированной печати.
    /*!
       \warning Для внутреннего использования
       \param value Флаг буферизированной печати (1 (true) - буферизировать, 0 (false) - не буферизировать)
       \retval 0 Успех
       \sa put_BufferedPrint()
     */
    virtual int DTOSHARED_CCA get_BufferedPrint(int *value) = 0;

    //! Устанавливает флаг буферизованной печати.
    /*!
       \warning Для внутреннего использования
       \param value Флаг буферизированной печати (1 (true) - буферизировать, 0 (false) - не буферизировать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BufferedPrint()
     */
    virtual int DTOSHARED_CCA put_BufferedPrint(int value) = 0;

    //! Печатает накопившийся буфер.
    /*!
       \warning Для внутреннего использования
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA FlushBuffer() = 0;

    //! Формирует и возвращает строку с информацией об устройстве.
    /*!
       \warning Для внутреннего использования
       \param bfr Буфер для строки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_InfoLine(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает модель устройства.
    /*!
       \param value Модель устройства
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Model(int *value) = 0;

    //! Возвращает флаг гашения.
    /*!
       \param value Флаг гашения (1 (true) - гасить, 0 (false) - не гасить)
       \retval 0 Успех
       \sa put_ClearFlag()
     */
    virtual int DTOSHARED_CCA get_ClearFlag(int *value) = 0;

    //! Устанавливает флаг гашения.
    /*!
       \param value Флаг гашения (1 (true) - гасить, 0 (false) - не гасить)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ClearFlag()
     */
    virtual int DTOSHARED_CCA put_ClearFlag(int value) = 0;

    //! Возвращает имя файла.
    /*!
       \param bfr Буфер для имени файла
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_FileName()
     */
    virtual int DTOSHARED_CCA get_FileName(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает имя файла.
    /*!
       \param value Имя файла
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FileName()
     */
    virtual int DTOSHARED_CCA put_FileName(const wchar_t *value) = 0;

    //! Производит печать изображения из файла.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FileName        <td>Путь к файлу                        <td>put_FileName()
        <tr><td>%Scale          <td>Масштаб                             <td>put_Scale()
        <tr><td>Alignment       <td>Выравнивание (TED::Fptr::Alignment) <td>put_Alignment()
        <tr><td>LeftMargin      <td>Отступ слева                        <td>put_LeftMargin()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintPicture() = 0;

    //! Возвращает дескриптор устройства вывода (для подключения дисплея покупателя).
    /*!
       Используется для настройки дисплея, подключенного через порт ККТ. Данное свойство нужно передать в TED::Display::IDisplay::put_ClsPtr().
       \param value Устройство вывода
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ClsPtr(void **value) = 0;

    //! Прокручивает бумагу.
    /*!
       \warning Для внутреннего использования
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Feed() = 0;

    //! Регистрирует налог.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма налога                        <td>put_Summ()
        <tr><td>TaxNumber       <td>Номер налога                        <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>Destination     <td>Назначение налога (TED::Fptr::DestinationType) <td>put_Destination()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме         <td>put_TestMode()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SummTax() = 0;

    //! Отменяет последний зарегистрированный налог.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма налога                        <td>put_Summ()
        <tr><td>TaxNumber       <td>Номер налога                        <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TestMode        <td>Выполнить в тестовом режиме         <td>put_TestMode()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA StornoTax() = 0;

    //! Устанавливает номер налога.
    /*!
       \param value Номер налога
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TaxNumber()
     */
    virtual int DTOSHARED_CCA put_TaxNumber(int value) = 0;

    //! Возвращает номер налога.
    /*!
       \param value Номер налога
       \retval 0 Успех
       \sa put_TaxNumber()
     */
    virtual int DTOSHARED_CCA get_TaxNumber(int *value) = 0;

    //! Производит фискализацию/перерегистрацию.
    /*!
       Метод производит фискализацию или перерегистрацию ККТ с заданными параметрами.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>INN             <td>ИНН                        <td>put_INN()
        <tr><td>MachineNumber   <td>Регистрационный номер      <td>put_MachineNumber()
        <tr><td>TaxPassword     <td>Пароль налогового инспектора<td>put_TaxPassword()
       </table>

       Работает из режима доступа к ФП (TED::Fptr::ModeFiscalMemory).
       \warning Перед использованием данного метода внимательно прочтите руководство налогового
       инспектора, входящее в комплект поставки ККТ.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Fiscalization() = 0;

    //! Устанавливает ИНН.
    /*!
       \param value ИНН
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_INN()
     */
    virtual int DTOSHARED_CCA put_INN(const wchar_t *value) = 0;

    //! Возвращает ИНН.
    /*!
       \param bfr Буфер для ИНН
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_INN()
     */
    virtual int DTOSHARED_CCA get_INN(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает регистрационный номер.
    /*!
       \param value Регистрационный номер
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_MachineNumber()
     */
    virtual int DTOSHARED_CCA put_MachineNumber(const wchar_t *value) = 0;

    //! Возвращает регистрационный номер.
    /*!
       \param bfr Буфер для номера
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_MachineNumber()
     */
    virtual int DTOSHARED_CCA get_MachineNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает код защиты.
    /*!
       \param value Код защиты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_License()
     */
    virtual int DTOSHARED_CCA put_License(const wchar_t *value) = 0;

    //! Возвращает код защиты.
    /*!
       \param bfr Буфер для кода
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_License()
     */
    virtual int DTOSHARED_CCA get_License(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает номер код защиты.
    /*!
       \param value Номер кода защиты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_LicenseNumber()
     */
    virtual int DTOSHARED_CCA put_LicenseNumber(int value) = 0;

    //! Возвращает номер код защиты.
    /*!
       \param value Номер кода защиты
       \retval 0 Успех
       \sa put_LicenseNumber()
     */
    virtual int DTOSHARED_CCA get_LicenseNumber(int *value) = 0;

    //! Запрашивает состояние кода защиты.
    /*!
       Проверяет состояние код защиты с номером \a LicenseNumber.
       Если код защиты не введен или введен неверно, \a ResultCode будет заполнен ошибкой EC_3859 (-3859).
       Если код защиты введен верно, \a ResultCode будет заполнен ошибкой EC_OK (0).

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>LicenseNumber       <td>Номер кода защиты   <td>put_LicenseNumber()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
       \sa put_LicenseNumber()
       \sa SetLicense()
     */
    virtual int DTOSHARED_CCA GetLicense() = 0;

    //! Вводит код защиты
    /*!
       Вводит код защиты \a License с номером \a LicenseNumber.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>LicenseNumber       <td>Номер кода защиты   <td>put_LicenseNumber()
        <tr><td>License             <td>Кода защиты         <td>put_License()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
       \sa GetLicense()
     */
    virtual int DTOSHARED_CCA SetLicense() = 0;

    //! Регистрирует скидку.
    /*!
       Метод производит регистрацию скидки (\a Summ) на весь чек или последнюю операцию (\a Destination).
       Тип значения \a Summ определяется значением \a DiscountType.
       При передачи \a DiscountNumber != 0 будет производиться регистрация скидки по номеру.
       При передачи отрицательного значение произведется регистрация надбавки.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма/процент                       <td>put_Summ()
        <tr><td>DiscountType    <td>Тип скидки (TED::Fptr::DestinationType) <td>put_DiscountType()
        <tr><td>Destination     <td>Назначение                          <td>put_Destination()
        <tr><td>DiscountNumber  <td>Номер скидки/надбавки               <td>put_DiscountNumber()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме         <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Discount() = 0;

    //! Регистрирует надбавку.
    /*!
       Метод производит регистрацию надбавки (\a Summ) на весь чек или последнюю операцию (\a Destination).
       Тип значения \a Summ определяется значением \a DiscountType.
       При передачи \a DiscountNumber != 0 будет производиться регистрация надбавки по номеру.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ            <td>Сумма/процент                       <td>put_Summ()
        <tr><td>DiscountType    <td>Тип скидки (TED::Fptr::DestinationType) <td>put_DiscountType()
        <tr><td>Destination     <td>Назначение                          <td>put_Destination()
        <tr><td>DiscountNumber  <td>Номер скидки/надбавки               <td>put_DiscountNumber()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме         <td>put_TestMode()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Charge() = 0;

    //! Устанавливает заводской номер.
    /*!
       Метод не запрашивает каких-либо предупреждений. Изменить или стереть введенный номер
       нельзя. После ввода заводского номера ККТ перестает быть демонстрационной версией и
       требует ввод лицензии (кода защиты) для выполнения лицензируемых функций

       Обычно ККТ продаются уже с введенным заводским номером, но существует
       демонстрационная (NFR) версия ККТ. Перед введением таких ККТ в эксплуатацию в них
       следует ввести заводской номер, указанный на табличке ККТ и в формуляре, входящем в ее
       комплект поставки.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>SerialNumber            <td>Серийный номер      <td>put_SerialNumber()
       </table>

       Работает из режима доступа к ФП (TED::Fptr::ModeFiscalMemory).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa put_SerialNumber()
     */
    virtual int DTOSHARED_CCA SetSerialNumber() = 0;

    //! Производит общее гашение ККТ.
    /*!
       При выполнении метод производит общее гашение ККТ. Перед выполнением метода необходимо снять суточный отчет с гашением.

       Работает из режима доступа к ФП (TED::Fptr::ModeFiscalMemory).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa Report()
     */
    virtual int DTOSHARED_CCA ResetSummary() = 0;

    //! Производит технологическое обнуление.
    /*!
       Метод производит технологическое обнуление ККТ. Технологическое обнуление включает в себя:
       - инициализацию системных таблиц начальными значениями;
       - общее гашение ККТ;
       - обнуление счетчика общих гашений.

       Метод не выдает каких-либо предупреждений и может выполняться только в
       определенном состоянии ККТ (подробнее смотрите в руководстве по сервисному
       обслуживанию для данной ККТ).

       Работает из режима выбора (TED::Fptr::ModeSelect).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA TechZero() = 0;

    //! Устанавливает номер таблицы.
    /*!
       \param value Номер кода таблицы
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Table()
     */
    virtual int DTOSHARED_CCA put_Table(int value) = 0;

    //! Возвращает номер таблицы.
    /*!
       \param value Номер кода защиты
       \retval 0 Успех
       \sa put_Table()
     */
    virtual int DTOSHARED_CCA get_Table(int *value) = 0;

    //! Устанавливает номер строки.
    /*!
       \param value Номер кода строки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Row()
     */
    virtual int DTOSHARED_CCA put_Row(int value) = 0;

    //! Возвращает номер строки.
    /*!
       \param value Номер кода защиты
       \retval 0 Успех
       \sa put_Row()
     */
    virtual int DTOSHARED_CCA get_Row(int *value) = 0;

    //! Устанавливает номер поля.
    /*!
       \param value Номер поля
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Field()
     */
    virtual int DTOSHARED_CCA put_Field(int value) = 0;

    //! Возвращает номер поля.
    /*!
       \param value Номер поля
       \retval 0 Успех
       \sa put_Field()
     */
    virtual int DTOSHARED_CCA get_Field(int *value) = 0;

    //! Устанавливает тип поля.
    /*!
       \param value Тип поля
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FieldType()
       \sa FieldType
     */
    virtual int DTOSHARED_CCA put_FieldType(int value) = 0;

    //! Возвращает тип поля.
    /*!
       \param value Тип поля
       \retval 0 Успех
       \sa put_FieldType()
     */
    virtual int DTOSHARED_CCA get_FieldType(int *value) = 0;

    //! Устанавливает настройку в ККТ.
    /*!
       Метод используется для записи данных в указанную ячейку любой системной таблицы ККТ.
       Структура таблиц описана в руководстве оператора и в протоколе работы ККТ, входящими в
       комплект поставки ККТ.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Table       <td>Номер таблицы       <td>put_Table()
        <tr><td>Row         <td>Номер ряда          <td>put_Row()
        <tr><td>Field       <td>Номер поля          <td>put_Field()
        <tr><td>FieldType   <td>Тип значения        <td>put_FieldType()
        <tr><td>Caption     <td>Значение            <td>put_Caption()
       </table>

        При записи значения типа FieldInteger передавать строку с числом ("123").
        При записи значения типа FieldString передавать строку.
        При записи значения типаFieldBuffer передавать строку с набором байтов, записанных в шестнадцатиричном виде ("FF 01 04")

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa GetTableField()
     */
    virtual int DTOSHARED_CCA SetTableField() = 0;

    //! Получает значение настройки из ККТ.
    /*!
       Метод используется для получения содержимого указанной ячейки из любой системной
       таблицы ККТ. Структура таблиц описана в руководстве оператора и в протоколе работы ККТ,
       входящими в комплект поставки ККТ.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Table       <td>Номер таблицы       <td>put_Table()
        <tr><td>Row         <td>Номер ряда          <td>put_Row()
        <tr><td>Field       <td>Номер поля          <td>put_Field()
        <tr><td>FieldType   <td>Тип значения        <td>put_FieldType()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Caption     <td>Значение            <td>get_Caption()
       </table>

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa SetTableField()
     */
    virtual int DTOSHARED_CCA GetTableField() = 0;

    //! Устанавливает команду ККТ.
    /*!
       \param value Команда
       \return -1, если произошла ошибка. Иначе 0
       \sa get_CommandBuffer()
     */
    virtual int DTOSHARED_CCA put_CommandBuffer(const wchar_t *value) = 0;

    //! Получает команду ККТ.
    /*!
       \param bfr Буфер для команды
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CommandBuffer()
     */
    virtual int DTOSHARED_CCA get_CommandBuffer(wchar_t *bfr, int bfrSize) = 0;

    //! Получает ответа ККТ.
    /*!
       \param bfr Буфер для ответа
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_AnswerBuffer(wchar_t *bfr, int bfrSize) = 0;

    //! Запускает команду на исполнение.
    /*!
       Метод выполняет произвольную низкоуровневую команду протокола обмена ККТ.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CommandBuffer       <td>Команда                 <td>put_CommandBuffer()
        <tr><td>NeedResultFlag      <td>Флаг ожидания ответа    <td>put_NeedResultFlag()
        <tr><td>TimeoutACK          <td>Таймаут ожидания ответа на прием команды    <td>put_TimeoutACK()
        <tr><td>TimeoutENQ          <td>Таймаут ожидания ответа на команду    <td>put_TimeoutENQ()
        <tr><th colspan=3>Выходные свойства
        <tr><td>AnswerBuffer        <td>Ответ                   <td>get_AnswerBuffer()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA RunCommand() = 0;

    //! Отменяет последнюю скидку/надбавку.
    /*!
       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ResetChargeDiscount() = 0;

    //! Инициализирует системные таблицы начальными значениями.
    /*!
       Метод не запрашивает каких-либо предупреждений и может выполняться только
       при первом включении после замены фискального ядра.

       Работает из режима выбора (TED::Fptr::ModeSelect).
       \warning Для корректной инициализации таблиц необходимо после вызова метода выключить и
       включить ККТ.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA InitTables() = 0;

    //! Возвращает значение флагов ЭКЛЗ.
    /*!
       \param value Флаги ЭКЛЗ
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_EKLZFlags(int *value) = 0;

    //! Возвращает номер КПК.
    /*!
       \param value Номер КПК
       \retval 0 Успех
       \sa put_EKLZKPKNumber()
     */
    virtual int DTOSHARED_CCA get_EKLZKPKNumber(int *value) = 0;

    //! Устанавливает номер КПК.
    /*!
       \param value Номер КПК
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_EKLZKPKNumber()
     */
    virtual int DTOSHARED_CCA put_EKLZKPKNumber(int value) = 0;

    //! Производит активизацию ЭКЛЗ / ФН.
    /*!
       \warning Выполняется только один раз при активизации текущей ЭКЛЗ в ККТ
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA EKLZActivate() = 0;

    //! Выполняет закрытие архива ЭКЛЗ / ФН.
    /*!
       \warning Выполняется только один раз для текущей ЭКЛЗ.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA EKLZCloseArchive() = 0;

    //! Запрашивает состояние ЭКЛЗ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>Date            <td>Дата последней записи в ЭКЛЗ    <td>get_Date()
        <tr><td>Time            <td>Время последней записи в ЭКЛЗ   <td>get_Time()
        <tr><td>Summ            <td>Сумма текущего чека             <td>get_Summ()
        <tr><td>Session         <td>Номер смены                     <td>get_Session()
        <tr><td>EKLZFlags       <td>Текущее состояние ЭКЛЗ          <td>get_EKLZFlags()
        <tr><td>EKLZKPKNumber   <td>Номер КПК                       <td>get_EKLZKPK()
        <tr><td>SerialNumber    <td>Серийный номер ЭКЛЗ             <td>get_SerialNumber()
       </table>
       <br>

       <table>
        <caption>Расшифровка EKLZFlags</caption>
        <tr><th>Биты<th>Описание
        <tr><td>0–1<td>0 - продажа<br>
        1 - покупка<br>
        2 - возврат продажи<br>
        3 - возврат покупки
        <tr><td>2<td>0 – архив закрыт<br>
        1 – архив открыт
        <tr><td>3<td>0 – ЭКЛЗ не активизирована<br>
        1 – активизирована
        <tr><td>4<td>0 – нет отчета<br>
        1 – снимается отчет
        <tr><td>5<td>0 – документ закрыт<br>
        1 – документ открыт
        <tr><td>6<td>0 – смена закрыта<br>
        1 – смена открыта
        <tr><td>7<td>0 – нет неисправимой ошибки<br>
        1 – есть ошибка.
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Date()
       \sa get_Time()
       \sa get_Summ()
       \sa get_Session()
       \sa get_EKLZFlags()
       \sa get_EKLZKPKNumber()
       \sa get_SerialNumber()
     */
    virtual int DTOSHARED_CCA EKLZGetStatus() = 0;

    //! Возвращает верхную границу диапазона дат.
    /*!
       \param day День
       \param month Месяц
       \param year Год
       \retval 0 Успех
       \sa put_DateEnd()
     */
    virtual int DTOSHARED_CCA get_DateEnd(int *day, int *month, int *year) = 0;

    //! Устанавливает верхную границу диапазона дат.
    /*!
       \param day День
       \param month Месяц
       \param year Год
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DateEnd()
     */
    virtual int DTOSHARED_CCA put_DateEnd(int day, int month, int year) = 0;

    //! Возвращает верхнюю границу диапазона смен.
    /*!
       \param value Номер смены
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_SessionEnd(int *value) = 0;

    //! Устанавливает верхнюю границу диапазона смен.
    /*!
       \param value Номер смены
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_SessionEnd(int value) = 0;

    //! Запрашивает диапазоны фискальных отчетов.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>Date        <td>Дата первой записи в ФП     <td>get_Date()
        <tr><td>DateEnd     <td>Дата последней записи в ФП  <td>get_DateEnd()
        <tr><td>Session     <td>Номер первой смены в ФП     <td>get_Session()
        <tr><td>SessionEnd  <td>Номер последней смены в ФП  <td>get_SessionEnd()
       </table>

       Работает из режима доступа к ФП (TED::Fptr::ModeFiscalMemory).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetRange() = 0;

    //! Начинает чтение данных.
    /*!
       Метод начинает чтение данных (переводит драйвер в режим отчета).

       Отчет кэшируется – все данные считываются во внутренний буфер драйвера,
       расположенный в оперативной памяти ПК. Если считать без ошибок все заданные строки не
       удалось, то возвращается ошибка и очищается кэш (те данные, которые были считаны до
       возникновения ошибки, получить методом GetRecord() не удастся).

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReportType          <td>Тип данных              <td>put_ReportType()
        <tr><td>UnitType            <td>Тип части устройства    <td>put_UnitType()
       </table>
       <br>

       <table>
        <caption id="begin_report_types">Список отчетов</caption>
        <tr><th>TED::Fptr::ReportType               <th>Описание<th>Входные свойства<th>Выходные свойства
        <tr><td>TED::Fptr::ReportRom                <td>ПО ККТ<td><td>\a Caption
        <tr><td>TED::Fptr::ReportRomUnit            <td>ПО модуля ККТ<td>\a UnitType<td>\a Caption
        <tr><td>TED::Fptr::ReportJournalData        <td>Данные ЭЖ<td><td>\a Caption
        <tr><td>TED::Fptr::ReportLastDocument       <td>Последний документ<td><td>\a Caption
            <br>\a FontDblWidth
            <br>\a ReceiptFont
            <br>\a ReceiptFontHeight
            <br>\a ReceiptLinespacing
            <br>\a ReceiptBrightness
       </table>

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
       \sa GetRecord()
       \sa EndReport()
     */
    virtual int DTOSHARED_CCA BeginReport() = 0;

    //! Считывает очередную запись.
    /*!
       Метод записывает в соответствующие свойства драйвера параметры очередной записи.
       Выходные свойства зависят от \ref begin_report_types "типа отчета".

       \retval -1 Ошибка
       \retval 0 Успех
       \sa BeginReport()
       \sa EndReport()
     */
    virtual int DTOSHARED_CCA GetRecord() = 0;

    //! Завершает чтение данных.
    /*!
       При выполнении метод завершает чтение данных, начатое BeginReport (выводит драйвер
       из режима снятия отчета), очищает буфер драйвера и освобождает выделенную для него
       память.

       \retval -1 Ошибка
       \retval 0 Успех
       \sa BeginReport()
       \sa GetRecord()
     */
    virtual int DTOSHARED_CCA EndReport() = 0;

    //! Возвращает тип части устройства.
    /*!
       \param value Тип части устройства
       \retval 0 Успех
       \sa put_UnitType()
       \sa UnitType
     */
    virtual int DTOSHARED_CCA get_UnitType(int *value) = 0;

    //! Устанавливает тип части устройства.
    /*!
       \param value Тип части устройства
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_UnitType()
       \sa UnitType
     */
    virtual int DTOSHARED_CCA put_UnitType(int value) = 0;

    //! Устанавливает номер изображения в памяти ККТ.
    /*!
       \param value Номер изображения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PictureNumber()
     */
    virtual int DTOSHARED_CCA put_PictureNumber(int value) = 0;

    //! Возвращает номер изображения в памяти ККТ.
    /*!
       \param value Номер изображения
       \retval 0 Успех
       \sa put_PictureNumber()
     */
    virtual int DTOSHARED_CCA get_PictureNumber(int *value) = 0;

    //! Устанавливает отступ от левой стороны области печати.
    /*!
       \param value Значение отступа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_LeftMargin()
     */
    virtual int DTOSHARED_CCA put_LeftMargin(int value) = 0;

    //! Возвращает отступ от левой стороны области печати.
    /*!
       \param value Значение отступа
       \retval 0 Успех
       \sa put_LeftMargin()
     */
    virtual int DTOSHARED_CCA get_LeftMargin(int *value) = 0;


    //! Добавить изображение из файла в память ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FileName        <td>Путь к файлу изображения    <td>put_FileName()
        <tr><th colspan=3>Выходные свойства
        <tr><td>PictureNumber   <td>Номер картинки в памяти     <td>get_PictureNumber()
       </table>

       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddPictureFromFile() = 0;

    //! Печатает изображение из памяти ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>PictureNumber   <td>Номер картинки в памяти     <td>put_PictureNumber()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintPictureByNumber() = 0;

    //! Возвращает размер памяти.
    /*!
       \param value Размер памяти
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Memory(int *value) = 0;

    //! Возвращает состояние изображения.
    /*!
       \param value Состояние изображения
       \retval 0 Успех
       \sa PictureState
     */
    virtual int DTOSHARED_CCA get_PictureState(int *value) = 0;

    //! Возвращает ширину.
    /*!
       \param value Ширина
       \retval 0 Успех
       \sa put_Width()
     */
    virtual int DTOSHARED_CCA get_Width(int *value) = 0;

    //! Устанавливает ширину изображения.
    /*!
       \param value Ширина изображения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Width()
     */
    virtual int DTOSHARED_CCA put_Width(int value) = 0;

    //! Запрашивает информацию о массиве изображений.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>PictureNumber   <td>Количество изображений в памяти     <td>get_PictureNumber()
        <tr><td>Memory<td>Количество свободной памяти в массиве<td>get_Memory()
        <tr><td>PictureState    <td>Состояние последнего изображения    <td>get_PictureState()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetPictureArrayStatus() = 0;

    //! Запрашивает информацию об изображении.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>PictureNumber   <td>Номер изображения       <td>put_PictureNumber()
        <tr><th colspan=3>Выходные свойства
        <tr><td>PictureState    <td>Состояние изображения   <td>get_PictureState()
        <tr><td>Height          <td>Высота изображения      <td>get_Height()
        <tr><td>Width           <td>Ширина изображения      <td>get_Width()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetPictureStatus() = 0;

    //! Удаляет последнее изображение из массива.
    /*!
       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA DeleteLastPicture() = 0;

    //! Очищает массив изображений.
    /*!
       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ClearPictureArray() = 0;

    //! Начинает процесс добавления информации.
    /*!
       \warning Не поддерживается в текущей версии
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA BeginAdd() = 0;

    //! Записывает элемент данных.
    /*!
       \warning Не поддерживается в текущей версии
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetRecord() = 0;

    //! Заканчивает добавление.
    /*!
       \warning Не поддерживается в текущей версии
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA EndAdd() = 0;

    //! Запрашивает изображение из памяти ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>PictureNumber   <td>Номер изображения       <td>put_PictureNumber()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Caption         <td>Данные изображения      <td>get_Caption()
        <tr><td>Height          <td>Высота изображения      <td>get_Height()
        <tr><td>Width           <td>Ширина изображения      <td>get_Width()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetPicture() = 0;

    //! Загружает изображение из буфера в память ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Caption         <td>Данные изображения  <td>put_Caption()
        <tr><td>Height          <td>Высота изображения  <td>put_Height()
        <tr><td>Width           <td>Ширина изображения  <td>put_Width()
        <tr><th colspan=3>Выходные свойства
        <tr><td>PictureNumber   <td>Номер изображения   <td>get_PictureNumber()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddPicture() = 0;

    //! Возвращает номер оператора.
    /*!
       \param value Номер оператора
       \retval 0 Успех
       \sa put_Operator()
     */
    virtual int DTOSHARED_CCA get_Operator(int *value) = 0;

    //! Добавляет текстовое поле для печати.
    /*!
       Подробнее см. документацию на ККТ АТОЛ (команда 87h с режимКЛ=режимЧЛ=0)

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Caption         <td>Строка                  <td>put_Caption()
        <tr><td>TestMode        <td>Тестовый режим          <td>put_TestMode()
        <tr><td>FontNegative    <td>Фон символов            <td>put_FontNegative()
        <tr><td>FontUnderline   <td>Подчеркнутый шрифт      <td>put_FontUnderline()
        <tr><td>FontBold        <td>Шрифт двойной толщины   <td>put_FontBold()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddTextField() = 0;

    //! Устанавливает флаг жирного шрифта.
    /*!
       \param value Флаг (1 (true) - жирный шрифт, 0 (false) - нет модификатора)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FontBold()
     */
    virtual int DTOSHARED_CCA put_FontBold(int value) = 0;

    //! Возвращает флаг жирного шрифта.
    /*!
       \param value Флаг (1 (true) - жирный шрифт, 0 (false) - нет модификатора)
       \retval 0 Успех
       \sa put_FontBold()
     */
    virtual int DTOSHARED_CCA get_FontBold(int *value) = 0;

    //! Устанавливает флаг наклоного шрифта.
    /*!
       \param value Флаг (1 (true) - наклонный шрифт, 0 (false) - нет модификатора)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FontItalic()
     */
    virtual int DTOSHARED_CCA put_FontItalic(int value) = 0;

    //! Возвращает флаг наклоного шрифта.
    /*!
       \param value Флаг (1 (true) - наклонный шрифт, 0 (false) - нет модификатора)
       \retval 0 Успех
       \sa put_FontItalic()
     */
    virtual int DTOSHARED_CCA get_FontItalic(int *value) = 0;

    //! Устанавливает флаг инверсного шрифта.
    /*!
       \param value Флаг (1 (true) - инверсный шрифт, 0 (false) - нет модификатора)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FontNegative()
     */
    virtual int DTOSHARED_CCA put_FontNegative(int value) = 0;

    //! Возвращает флаг инверсного шрифта.
    /*!
       \param value Флаг (1 (true) - инверсный шрифт, 0 (false) - нет модификатора)
       \retval 0 Успех
       \sa put_FontNegative()
     */
    virtual int DTOSHARED_CCA get_FontNegative(int *value) = 0;

    //! Устанавливает флаг подчеркнутого шрифта.
    /*!
       \param value Флаг (1 (true) - подчеркнутый шрифт, 0 (false) - нет модификатора)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FontUnderline()
     */
    virtual int DTOSHARED_CCA put_FontUnderline(int value) = 0;

    //! Возвращает флаг подчеркнутого шрифта.
    /*!
       \param value Флаг (1 (true) - подчеркнутый шрифт, 0 (false) - нет модификатора)
       \retval 0 Успех
       \sa put_FontUnderline()
     */
    virtual int DTOSHARED_CCA get_FontUnderline(int *value) = 0;

    //! Устанавливает флаг шрифта двойной высоты.
    /*!
       \param value Флаг (1 (true) - шрифт двойной высоты, 0 (false) - нет модификатора)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FontDblHeight()
     */
    virtual int DTOSHARED_CCA put_FontDblHeight(int value) = 0;

    //! Возвращает флаг шрифта двойной высоты.
    /*!
       \param value Флаг (1 (true) - шрифт двойной высоты, 0 (false) - нет модификатора)
       \retval 0 Успех
       \sa put_FontDblHeight()
     */
    virtual int DTOSHARED_CCA get_FontDblHeight(int *value) = 0;

    //! Устанавливает флаг шрифта двойной ширины.
    /*!
       \param value Флаг (1 (true) - шрифт двойной ширины, 0 (false) - нет модификатора)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FontDblWidth()
     */
    virtual int DTOSHARED_CCA put_FontDblWidth(int value) = 0;

    //! Возвращает флаг шрифта двойной ширины.
    /*!
       \param value Флаг  (1 (true) - шрифт двойной ширины, 0 (false) - нет модификатора)
       \retval 0 Успех
       \sa put_FontDblWidth()
     */
    virtual int DTOSHARED_CCA get_FontDblWidth(int *value) = 0;

    //! Печатает текст с форматированием.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>TestMode            <td>Тестовый режим                              <td>put_TestMode()
        <tr><td>Caption             <td>Строка                                      <td>put_Caption()
        <tr><td>Alignment           <td>Выравнивание строки (TED::Fptr::Alignment)  <td>put_Alignment()
        <tr><td>TextWrap            <td>Перенос строки (TED::Fptr::TextWrap)        <td>put_TextWrap()
        <tr><td>PrintPurpose        <td>Место печати (TED::Fptr::PrintPurpose)      <td>put_PrintPurpose()
        <tr><td>ReceiptFont         <td>Шрифт ЧЛ (TED::Fptr::Font)                  <td>put_ReceiptFont()
        <tr><td>ReceiptFontHeight   <td>Высота шрифта ЧЛ (TED::Fptr::FontHeight)    <td>put_ReceiptFontHeight()
        <tr><td>ReceiptLinespacing  <td>Межстрочный интервал на ЧЛ                  <td>put_ReceiptLinespacing()
        <tr><td>ReceiptBrightness   <td>Яркость текста на ЧЛ                        <td>put_ReceiptBrightness()
        <tr><td>JournalFont         <td>Шрифт КЛ (TED::Fptr::Font)                  <td>put_JournalFont()
        <tr><td>JournalFontHeight   <td>Высота шрифта КЛ (TED::Fptr::FontHeight)    <td>put_JournalFontHeight()
        <tr><td>JournalLinespacing  <td>Межстрочный интервал на КЛ                  <td>put_JournalLinespacing()
        <tr><td>JournalBrightness   <td>Яркость текста на КЛ                        <td>put_JournalBrightness()
        <tr><td>FontNegative        <td>Фон символов                                <td>put_FontNegative()
        <tr><td>FontUnderline       <td>Подчеркнутый шрифт                          <td>put_FontUnderline()
        <tr><td>FontBold            <td>Шрифт двойной толщины                       <td>put_FontBold()
        <tr><td>FontDblWidth        <td>Двойная ширина                              <td>put_FontDblWidth()
        <tr><td>FontDblHeight       <td>Двойная высота                              <td>put_FontDblHeight()
       </table>

       Свойство FontDblHeight работает аналогично свойству ReceiptFontHeight.
При установке FontDblHeight в 1 (true) свойство ReceiptFontHeight выставляется в TED::Fptr::FontHeightStretched.
При установке FontDblHeight в 0 (false) свойство ReceiptFontHeight выставляется в TED::Fptr::FontHeightSingle.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintFormattedText() = 0;

    //! Устанавливает место печати.
    /*!
       \param value Место печати
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PrintPurpose()
       \sa TED::Fptr::PrintPurpose
     */
    virtual int DTOSHARED_CCA put_PrintPurpose(int value) = 0;

    //! Возвращает место печати.
    /*!
       \param value Место печати
       \retval 0 Успех
       \sa put_PrintPurpose()
       \sa TED::Fptr::PrintPurpose
     */
    virtual int DTOSHARED_CCA get_PrintPurpose(int *value) = 0;

    //! Устанавливает шрифт ЧЛ.
    /*!
       \param value Шрифт
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReceiptFont()
       \sa TED::Fptr::Font
     */
    virtual int DTOSHARED_CCA put_ReceiptFont(int value) = 0;

    //! Возвращает шрифт ЧЛ.
    /*!
       \param value Шрифт
       \retval 0 Успех
       \sa put_ReceiptFont()
       \sa TED::Fptr::Font
     */
    virtual int DTOSHARED_CCA get_ReceiptFont(int *value) = 0;

    //! Устанавливает высоту шрифта ЧЛ.
    /*!
       \param value Высота шрифта
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReceiptFontHeight()
       \sa TED::Fptr::FontHeight
     */
    virtual int DTOSHARED_CCA put_ReceiptFontHeight(int value) = 0;

    //! Возвращает высоту шрифта ЧЛ.
    /*!
       \param value Высота шрифта
       \retval 0 Успех
       \sa get_ReceiptFontHeight()
       \sa TED::Fptr::FontHeight
     */
    virtual int DTOSHARED_CCA get_ReceiptFontHeight(int *value) = 0;

    //! Возвращает яркость шрифта ЧЛ.
    /*!
       \param value Яркость
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReceiptBrightness()
     */
    virtual int DTOSHARED_CCA put_ReceiptBrightness(int value) = 0;

    //! Возвращает яркость шрифта ЧЛ.
    /*!
       \param value Яркость
       \retval 0 Успех
       \sa put_ReceiptBrightness()
     */
    virtual int DTOSHARED_CCA get_ReceiptBrightness(int *value) = 0;

    //! Устанавливает межстрочный интервал ЧЛ.
    /*!
       \param value Интервал
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReceiptLinespacing()
     */
    virtual int DTOSHARED_CCA put_ReceiptLinespacing(int value) = 0;

    //! Возвращает межстрочный интервал ЧЛ.
    /*!
       \param value Интервал
       \retval 0 Успех
       \sa put_ReceiptLinespacing()
     */
    virtual int DTOSHARED_CCA get_ReceiptLinespacing(int *value) = 0;

    //! Устанавливает шрифт КЛ.
    /*!
       \param value Шрифт
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_JournalFont()
       \sa TED::Fptr::Font
     */
    virtual int DTOSHARED_CCA put_JournalFont(int value) = 0;

    //! Возвращает шрифт КЛ.
    /*!
       \param value Шрифт
       \retval 0 Успех
       \sa put_JournalFont()
       \sa TED::Fptr::Font
     */
    virtual int DTOSHARED_CCA get_JournalFont(int *value) = 0;

    //! Устанавливает высоту шрифта КЛ.
    /*!
       \param value Высота шрифта
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_JournalFontHeight()
       \sa TED::Fptr::FontHeight
     */
    virtual int DTOSHARED_CCA put_JournalFontHeight(int value) = 0;

    //! Возвращает высоту шрифта КЛ.
    /*!
       \param value Высота шрифта
       \retval 0 Успех
       \sa put_JournalFontHeight()
       \sa TED::Fptr::FontHeight
     */
    virtual int DTOSHARED_CCA get_JournalFontHeight(int *value) = 0;

    //! Устанавливает яркость шрифта КЛ.
    /*!
       \param value Яркость
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_JournalBrightness()
     */
    virtual int DTOSHARED_CCA put_JournalBrightness(int value) = 0;

    //! Возвращает яркость шрифта КЛ.
    /*!
       \param value Яркость
       \retval 0 Успех
       \sa put_JournalBrightness()
     */
    virtual int DTOSHARED_CCA get_JournalBrightness(int *value) = 0;

    //! Устанавливает межстрочный интервал КЛ.
    /*!
       \param value Интервал
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_JournalLinespacing()
     */
    virtual int DTOSHARED_CCA put_JournalLinespacing(int value) = 0;

    //! Возвращает межстрочный интервал КЛ.
    /*!
       \param value Интервал
       \retval 0 Успех
       \sa put_JournalLinespacing()
     */
    virtual int DTOSHARED_CCA get_JournalLinespacing(int *value) = 0;

    //! Устанавливает значение десятичной точки в суммах.
    /*!
       \param value Кол-во разрядов
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_SummPointPosition()
     */
    virtual int DTOSHARED_CCA put_SummPointPosition(int value) = 0;

    //! Возвращает значение десятичной точки в суммах.
    /*!
       \param value Кол-во разрядов
       \retval 0 Успех
       \sa put_SummPointPosition()
     */
    virtual int DTOSHARED_CCA get_SummPointPosition(int *value) = 0;

    //! Устанавливает способ печати ШК.
    /*!
       \param value Способ печати
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodePrintType()
       \sa TED::Fptr::BarcodePrintType
     */
    virtual int DTOSHARED_CCA put_BarcodePrintType(int value) = 0;

    //! Возвращает способ печати ШК.
    /*!
       \param value Способ печати
       \retval 0 Успех
       \sa put_BarcodePrintType()
       \sa TED::Fptr::BarcodePrintType
     */
    virtual int DTOSHARED_CCA get_BarcodePrintType(int *value) = 0;

    //! Устанавливает флаг наличия контрольных символов ШК.
    /*!
       \param value Флаг (1 (true) - в переданной в \a Barcode строке есть контрольные символы, 0 (false) - нет символов)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeControlCode()
     */
    virtual int DTOSHARED_CCA put_BarcodeControlCode(int value) = 0;

    //! Возвращает флаг наличия контрольных символов ШК.
    /*!
       \param value Флаг (1 (true) - в переданной в \a Barcode строке есть контрольные символы, 0 (false) - нет символов)
       \retval 0 Успех
       \sa put_BarcodeControlCode()
     */
    virtual int DTOSHARED_CCA get_BarcodeControlCode(int *value) = 0;

    //! Устанавливает уровень коррекции ШК.
    /*!
       \param value Уровень коррекции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeCorrection()
     */
    virtual int DTOSHARED_CCA put_BarcodeCorrection(int value) = 0;

    //! Возвращает уровень коррекции ШК.
    /*!
       \param value Уровень коррекции
       \retval 0 Успех
       \sa put_BarcodeCorrection()
     */
    virtual int DTOSHARED_CCA get_BarcodeCorrection(int *value) = 0;

    //! Устанавливает кодировку ШК.
    /*!
       \param value Кодировка
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeEncoding()
     */
    virtual int DTOSHARED_CCA put_BarcodeEncoding(int value) = 0;

    //! Возвращает кодировку ШК.
    /*!
       \param value Кодировка
       \retval 0 Успех
       \sa put_BarcodeEncoding()
     */
    virtual int DTOSHARED_CCA get_BarcodeEncoding(int *value) = 0;

    //! Устанавливает режим кодировки ШК.
    /*!
       \param value Режим кодировки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeEncodingMode()
     */
    virtual int DTOSHARED_CCA put_BarcodeEncodingMode(int value) = 0;

    //! Возвращает режим кодировки ШК.
    /*!
       \param value Режим кодировки
       \retval 0 Успех
       \sa put_BarcodeEncodingMode()
     */
    virtual int DTOSHARED_CCA get_BarcodeEncodingMode(int *value) = 0;

    //! Устанавливает кол-во строк промотки.
    /*!
       \warning Для внутреннего использования
       \param value Кол-во строк промотки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FeedValue()
     */
    virtual int DTOSHARED_CCA put_FeedValue(int value) = 0;

    //! Возвращает кол-во строк промотки.
    /*!
       \warning Для внутреннего использования
       \param value Кол-во строк промотки
       \retval 0 Успех
       \sa put_FeedValue()
     */
    virtual int DTOSHARED_CCA get_FeedValue(int *value) = 0;

    //! Устанавливает позицию десятичной точки в ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>SummPointPosition   <td>Позиция десятичной точки в суммах   <td>put_SummPointPosition()
       </table>

       Работает из режима отчетов с гашением (TED::Fptr::ModeReportClear).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetPointPosition() = 0;

    //! Возвращает ширину текущей станции в точках.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PixelLineLength(int *value) = 0;

    //! Возвращает ширину ЧЛ в точках.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_RcpPixelLineLength(int *value) = 0;

    //! Возвращает ширину КЛ в точках.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_JrnPixelLineLength(int *value) = 0;

    //! Возвращает ширину ПД в точках.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_SlipPixelLineLength(int *value) = 0;

    //! Возвращает ширину ЧЛ в символах.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_RcpCharLineLength(int *value) = 0;

    //! Возвращает ширину КЛ в символах.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_JrnCharLineLength(int *value) = 0;

    //! Возвращает ширину ПД в символах.
    /*!
       \param value Ширина
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_SlipCharLineLength(int *value) = 0;

    //! Устанавливает значение счетчика.
    /*!
       \param value Счетчик
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Count()
     */
    virtual int DTOSHARED_CCA put_Count(int value) = 0;

    //! Возвращает значение счетчика.
    /*!
       \param value Счетчик
       \return Код результата
       \retval 0 Успех
       \sa put_Count()
     */
    virtual int DTOSHARED_CCA get_Count(int *value) = 0;

    //! Устанавливает номер кассира.
    /*!
       \param value Номер
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Operator()
     */
    virtual int DTOSHARED_CCA put_Operator(int value) = 0;

    //! Возвращает номер / тип порта.
    /*!
       \param value Номер / тип порта
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_SlotNumber()
     */
    virtual int DTOSHARED_CCA put_SlotNumber(int value) = 0;

    //! Устанавливает номер / тип порта.
    /*!
       \param value Номер / тип порта
       \retval 0 Успех
       \sa put_SlotNumber()
     */
    virtual int DTOSHARED_CCA get_SlotNumber(int *value) = 0;

    //! Возвращает состояние ДЯ.
    /*!
       \param value Состояние ДЯ (1 (true) - открыт, 0 (false) - закрыт)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_DrawerOpened(int *value) = 0;

    //! Возвращает состояние крышки.
    /*!
       \param value Состояние крышки (1 (true) - открыта, 0 (false) - закрыта)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_CoverOpened(int *value) = 0;

    //! Возвращает состояние батарейки.
    /*!
       \param value Состояние батарейки (1 (true) - низкий заряд, 0 (false) - нормальный заряд)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_BatteryLow(int *value) = 0;

    //! Возвращает версию.
    /*!
       \param bfr Буфер для версии
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_VerHi(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает подверсию.
    /*!
       \param bfr Буфер для подверсии
       \param bfrSize Размер буфера
       \return Код результата
     */
    virtual int DTOSHARED_CCA get_VerLo(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает номер сборки.
    /*!
       \param bfr Буфер для номера сборки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_Build(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает кодовую страницу.
    /*!
       \param value Номер кодовой страницы
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Codepage(int *value) = 0;

    //! Возвращает значение остатка.
    /*!
       \param value Остаток
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Remainder(double *value) = 0;

    //! Возвращает значение сдачи.
    /*!
       \param value Сдача
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Change(double *value) = 0;

    //! Возвращает логический номер кассы.
    /*!
       \param value Номер кассы
       \retval 0 Успех
       \sa put_LogicalNumber()
     */
    virtual int DTOSHARED_CCA get_LogicalNumber(int *value) = 0;

    //! Возвращает тип операции.
    /*!
       \param value Тип операции
       \retval 0 Успех
       \sa put_OperationType()
     */
    virtual int DTOSHARED_CCA get_OperationType(int *value) = 0;

    //! Устанавливает тип операции.
    /*!
       \param value Тип операции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_OperationType()
     */
    virtual int DTOSHARED_CCA put_OperationType(int value) = 0;

    //! Устанавливает номер скидки.
    /*!
       \param value Номер скидки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DiscountNumber()
     */
    virtual int DTOSHARED_CCA put_DiscountNumber(int value) = 0;

    //! Возвращает номер скидки.
    /*!
       \param value Номер скидки
       \retval 0 Успех
       \sa put_DiscountNumber()
     */
    virtual int DTOSHARED_CCA get_DiscountNumber(int *value) = 0;

    //! Устанавливает тип счетчика.
    /*!
       \param value Тип счетчика
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CounterType()
     */
    virtual int DTOSHARED_CCA put_CounterType(int value) = 0;

    //! Возвращает тип счетчика.
    /*!
       \param value Тип счетчика
       \retval 0 Успех
       \sa put_CounterType()
     */
    virtual int DTOSHARED_CCA get_CounterType(int *value) = 0;

    //! Возвращает напряжение источника питания.
    /*!
       \param value Напряжение
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PowerSupplyValue(double *value) = 0;

    //! Возвращает состояние источника питания.
    /*!
       \param value Состояние
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PowerSupplyState(int *value) = 0;

    //! Устанавливает тип источника питания.
    /*!
       \param value Тип источника питания
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PowerSupplyType()
     */
    virtual int DTOSHARED_CCA put_PowerSupplyType(int value) = 0;

    //! Получает тип источника питания.
    /*!
       \param value Тип источника питания
       \retval 0 Успех
       \sa put_PowerSupplyType()
     */
    virtual int DTOSHARED_CCA get_PowerSupplyType(int *value) = 0;

    //! Устанавливает тип счетчика шагов.
    /*!
       \param value Тип счетчика шагов
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_StepCounterType()
     */
    virtual int DTOSHARED_CCA put_StepCounterType(int value) = 0;

    //! Возвращает тип счетчика шагов.
    /*!
       \param value Тип счетчика шагов
       \retval 0 Успех
       \sa put_StepCounterType()
     */
    virtual int DTOSHARED_CCA get_StepCounterType(int *value) = 0;

    //! Устанавливает область действия налога / скидки.
    /*!
       \param value Область действия
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Destination()
     */
    virtual int DTOSHARED_CCA put_Destination(int value) = 0;

    //! Возвращает область действия налога / скидки.
    /*!
       \param value Область действия
       \retval 0 Успех
       \sa put_Destination()
     */
    virtual int DTOSHARED_CCA get_Destination(int *value) = 0;

    //! Производит сторнирование платежа.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Summ        <td>Сумма платежа       <td>put_Summ()
        <tr><td>TypeClose   <td>Тип платежа         <td>put_TypeClose()
        <tr><td>TestMode    <td>Тестовый режим      <td>put_TestMode()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Remainer    <td>Остаток для оплаты  <td>put_Summ()
        <tr><td>Change      <td>Сдача               <td>put_Summ()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA StornoPayment() = 0;

    //! Устанавливает пропорции пикселя ШК.
    /*!
       \param value Пропорции пикселя
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodePixelProportions()
     */
    virtual int DTOSHARED_CCA put_BarcodePixelProportions(int value) = 0;

    //! Устанавливает пропорции пикселя ШК.
    /*!
       \param value Пропорции пикселя
       \retval 0 Успех
       \sa get_BarcodePixelProportions()
     */
    virtual int DTOSHARED_CCA get_BarcodePixelProportions(int *value) = 0;

    //! Устанавливает пропорции ШК.
    /*!
       \param value Пропорции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeProportions()
     */
    virtual int DTOSHARED_CCA put_BarcodeProportions(int value) = 0;

    //! Возвращает пропорции ШК.
    /*!
       \param value Пропорции
       \retval 0 Успех
       \sa put_BarcodeProportions()
     */
    virtual int DTOSHARED_CCA get_BarcodeProportions(int *value) = 0;

    //! Устанавливает количество колонок ШК.
    /*!
       \param value Количество колонок
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeColumns()
     */
    virtual int DTOSHARED_CCA put_BarcodeColumns(int value) = 0;

    //! Возвращает количество колонок ШК.
    /*!
       \param value Количество колонок
       \retval 0 Успех
       \sa put_BarcodeColumns()
     */
    virtual int DTOSHARED_CCA get_BarcodeColumns(int *value) = 0;

    //! Устанавливает количество строк ШК.
    /*!
       \param value Количество строк
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeRows()
     */
    virtual int DTOSHARED_CCA put_BarcodeRows(int value) = 0;

    //! Возвращает количество строк ШК.
    /*!
       \param value Количество строк
       \retval 0 Успех
       \sa put_BarcodeRows()
     */
    virtual int DTOSHARED_CCA get_BarcodeRows(int *value) = 0;

    //! Устанавливает режим упаковки ШК.
    /*!
       \param value Режим упаковки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodePackingMode()
     */
    virtual int DTOSHARED_CCA put_BarcodePackingMode(int value) = 0;

    //! Возвращает режим упаковки ШК.
    /*!
       \param value Режим упаковки
       \retval 0 Успех
       \sa put_BarcodePackingMode()
     */
    virtual int DTOSHARED_CCA get_BarcodePackingMode(int *value) = 0;

    //! Устанавливает флаг использования пропорций ШК.
    /*!
       \param value Флаг
       \retval -1 Ошибка (1 (true) - использовать, 0 (false) - не использовать)
       \retval 0 Успех
       \sa get_BarcodePixelProportions()
     */
    virtual int DTOSHARED_CCA put_BarcodeUseProportions(int value) = 0;

    //! Возвращает флаг использования пропорций ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval 0 Успех
       \sa put_BarcodeUseProportions()
     */
    virtual int DTOSHARED_CCA get_BarcodeUseProportions(int *value) = 0;

    //! Устанавливает флаг использования строк ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeUseRows()
     */
    virtual int DTOSHARED_CCA put_BarcodeUseRows(int value) = 0;

    //! Возвращает флаг использования строк ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval 0 Успех
       \sa put_BarcodeUseRows()
     */
    virtual int DTOSHARED_CCA get_BarcodeUseRows(int *value) = 0;

    //! Устанавливает флаг использования столбцов ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeUseColumns()
     */
    virtual int DTOSHARED_CCA put_BarcodeUseColumns(int value) = 0;

    //! Возвращает флаг использования столбцов ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval 0 Успех
       \sa put_BarcodeUseColumns()
     */
    virtual int DTOSHARED_CCA get_BarcodeUseColumns(int *value) = 0;

    //! Устанавливает флаг использования коррекции ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeUseCorrection()
     */
    virtual int DTOSHARED_CCA put_BarcodeUseCorrection(int value) = 0;

    //! Возвращает флаг использования коррекции ШК.
    /*!
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval 0 Успех
       \sa put_BarcodeUseCorrection()
     */
    virtual int DTOSHARED_CCA get_BarcodeUseCorrection(int *value) = 0;

    //! Устанавливает флаг использования кодовых слов ШК.
    /*!
       \warning Не используется в текущей версии
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeUseCodeWords()
     */
    virtual int DTOSHARED_CCA put_BarcodeUseCodeWords(int value) = 0;

    //! Возвращает флаг использования кодовых слов ШК.
    /*!
       \warning Не используется в текущей версии
       \param value Флаг (1 (true) - использовать, 0 (false) - не использовать)
       \retval 0 Успех
       \sa put_BarcodeUseCodeWords()
     */
    virtual int DTOSHARED_CCA get_BarcodeUseCodeWords(int *value) = 0;

    //! Устанавливает флаг инверсии ШК.
    /*!
       \param value Флаг (1 (true) - инвертировать, 0 (false) - не инвертировать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeInvert()
     */
    virtual int DTOSHARED_CCA put_BarcodeInvert(int value) = 0;

    //! Возвращает флаг инверсии ШК.
    /*!
       \param value Флаг (1 (true) - инвертировать, 0 (false) - не инвертировать)
       \retval 0 Успех
       \sa get_BarcodeInvert()
     */
    virtual int DTOSHARED_CCA get_BarcodeInvert(int *value) = 0;

    //! Устанавливает номер ШК.
    /*!
       \param value Номер ШК
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeNumber()
     */
    virtual int DTOSHARED_CCA put_BarcodeNumber(int value) = 0;

    //! Возвращает флаг номер ШК.
    /*!
       \param value Номер ШК
       \retval 0 Успех
       \sa get_BarcodeNumber()
     */
    virtual int DTOSHARED_CCA get_BarcodeNumber(int *value) = 0;

    //! Печатает ШК из памяти ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>BarcodeNumber       <td>Номер ШК     <td>put_BarcodeNumber()
        <tr><td>PrintPurpose        <td>Место печати (TED::Fptr::PrintPurpose) <td>put_PrintPurpose()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintBarcodeByNumber() = 0;

    //! Очищает массив штрихкодов в памяти ККТ.
    /*!
       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ClearBarcodeArray() = 0;

    //! Удаляет последний ШК из массива.
    /*!
       Работает из режима программирования (TED::Fptr::ModeProgramming).
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA DeleteLastBarcode() = 0;

    //! Запрашивает ШК из памяти ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>BarcodeNumber           <td>Номер ШК                                        <td>put_BarcodeNumber()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Barcode                 <td>Данные ШК                                       <td>get_Barcode()
        <tr><td>BarcodeType             <td>Тип ШК (TED::Fptr::BarcodeType)                 <td>get_BarcodeType()
        <tr><td>BarcodePixelProportions <td>Пропорции пикселя ШК                            <td>get_BarcodePixelProportions()
        <tr><td>BarcodeProportions      <td>Пропорции ШК                                    <td>get_BarcodeProportions()
        <tr><td>BarcodeColumns          <td>Количество столбцов ШК                          <td>get_BarcodeColumns()
        <tr><td>BarcodeRows             <td>Количество строк ШК                             <td>get_BarcodeRows()
        <tr><td>BarcodePackingMode      <td>Режим упаковки ШК                               <td>get_BarcodePackingMode()
        <tr><td>BarcodeUseProportions   <td>Флаг использования пропорций ШК                 <td>get_BarcodeUseProportions()
        <tr><td>BarcodeUseRows          <td>Флаг использования кол-ва строк ШК              <td>get_BarcodeUseRows()
        <tr><td>BarcodeUseColumns       <td>Флаг использования кол-ва столбцов ШК           <td>get_BarcodeUseColumns()
        <tr><td>BarcodeUseCorrection    <td>Флаг использования коррекции ШК                 <td>get_BarcodeUseCorrection()
        <tr><td>BarcodeInvert           <td>Флаг инверсии ШК                                <td>get_BarcodeInvert()
        <tr><td>%Scale                  <td>Масштаб                                         <td>get_Scale()
        <tr><td>BarcodeVersion          <td>Версия ШК                                       <td>get_BarcodeVersion()
        <tr><td>BarcodeCorrection       <td>Коррекция ШК (TED::Fptr::BarcodeQRCorrection)   <td>get_BarcodeCorrection()
        <tr><td>BarcodeEncoding         <td>Кодировка ШК (TED::Fptr::BarcodeQREncoding)     <td>get_BarcodeEncoding()
        <tr><td>BarcodeEncodingMode     <td>Режим кодировки ШК (TED::Fptr::BarcodeQREncodingMode)<td>get_BarcodeEncodingMode()
        <tr><td>PrintBarcodeText        <td>Флаг печати данных ШК                           <td>get_PrintBarcodeText()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetBarcode() = 0;

    //! Производит тест раъема.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>SlotNumber      <td>Номер разъема   <td>put_SlotNumber()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA TestConnector() = 0;

    //! Выполняет демо-печать.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>PrintPurpose    <td>Место печати (TED::Fptr::PrintPurpose)  <td>put_PrintPurpose()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA DemoPrint() = 0;

    //! Производит программное выключение ККТ.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PowerOff() = 0;

    //! Отправляет данные в порт.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CommandBuffer   <td>Данные      <td>put_CommandBuffer()
        <tr><td>SlotNumber      <td>Номер порта <td>put_SlotNumber()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA WriteData() = 0;

    //! Открывает каталог на SD.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Directory   <td>Каталог     <td>put_Directory()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA OpenDirectory() = 0;

    //! Устанавливает каталог.
    /*!
       \param value Каталог
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Directory()
     */
    virtual int DTOSHARED_CCA put_Directory(const wchar_t *value) = 0;

    //! Возвращает каталог.
    /*!
       \param bfr Буфер для каталога
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval -1 Ошибка
       \retval 0 Успех
       \sa put_Directory()
     */
    virtual int DTOSHARED_CCA get_Directory(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает размер файла.
    /*!
       \param value Размер файла
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FileSize()
     */
    virtual int DTOSHARED_CCA put_FileSize(int value) = 0;

    //! Возвращает размер файла.
    /*!
       \param value Размер файла
       \retval 0 Успех
       \sa put_FileSize()
     */
    virtual int DTOSHARED_CCA get_FileSize(int *value) = 0;

    //! Читает каталог на SD.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>FileName    <td>Имя очередного файла/каталога   <td>get_FileName()
        <tr><td>FileSize    <td>Размер очередного файла         <td>get_FileSize()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ReadDirectory() = 0;

    //! Закрывает каталог на SD.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA CloseDirectory() = 0;

    //! Устанавливает способ открытия файла.
    /*!
       \param value Способ открытия файла
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FileOpenType()
     */
    virtual int DTOSHARED_CCA put_FileOpenType(int value) = 0;

    //! Возвращает способ открытия файла.
    /*!
       \param value Способ открытия файла
       \retval 0 Успех
       \sa put_FileOpenType()
     */
    virtual int DTOSHARED_CCA get_FileOpenType(int *value) = 0;

    //! Устанавливает режим открытия файла.
    /*!
       \param value Режим открытия файла
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FileOpenMode()
     */
    virtual int DTOSHARED_CCA put_FileOpenMode(int value) = 0;

    //! Возвращает режим открытия файла.
    /*!
       \param value Режим открытия файла
       \retval 0 Успех
       \sa put_FileOpenMode()
     */
    virtual int DTOSHARED_CCA get_FileOpenMode(int *value) = 0;

    //! Открывает файл на SD.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FileName        <td>Имя файла               <td>put_FileName()
        <tr><td>FileOpenMode    <td>Режим открытия файл     <td>put_FileOpenMode()
        <tr><td>FileOpenType    <td>Способ открытия файла   <td>put_FileOpenType()
        <tr><th colspan=3>Выходные свойства
        <tr><td>FileSize        <td>Размер файла            <td>get_FileSize()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA OpenFile() = 0;

    //! Закрывает файл на SD.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA CloseFile() = 0;

    //! Удаляет файл с SD.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FileName        <td>Имя файла   <td>put_FileName()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA DeleteFileFromSD() = 0;

    //! Записывает данные в файл на SD.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FileOffset      <td>Смещение    <td>put_FileOffset()
        <tr><td>Caption         <td>Данные      <td>put_Caption()
        <tr><th colspan=3>Выходные свойства
        <tr><td>FileOffset      <td>Смещение    <td>get_FileOffset()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA WriteFileToSD() = 0;

    //! Читает данные из файла на SD.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FileOffset          <td>Смещение                    <td>put_FileOffset()
        <tr><td>FileReadSize        <td>Размер данных для чтения    <td>put_FileReadSize()
        <tr><th colspan=3>Выходные свойства
        <tr><td>FileOffset          <td>Смещение                    <td>get_FileOffset()
        <tr><td>FileReadSize        <td>Размер считанных данных     <td>get_FileOffset()
        <tr><td>Caption             <td>Данные                      <td>get_Caption()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ReadFile() = 0;

    //! Устанавливает смещение в файле.
    /*!
       \param value Смещение
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FileOffset()
     */
    virtual int DTOSHARED_CCA put_FileOffset(int value) = 0;

    //! Возвращает смещение в файле.
    /*!
       \param value Смещение
       \retval 0 Успех
       \sa put_FileOffset()
     */
    virtual int DTOSHARED_CCA get_FileOffset(int *value) = 0;

    //! Устанавливает размер данных для чтения.
    /*!
       \param value Размер данных
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FileReadSize()
     */
    virtual int DTOSHARED_CCA put_FileReadSize(int value) = 0;

    //! Устанавливает смещение в файле.
    /*!
       \param value Размер данных
       \retval 0 Успех
       \sa put_FileReadSize()
     */
    virtual int DTOSHARED_CCA get_FileReadSize(int *value) = 0;

    //! Печатает копию последнего чека/отчета.
    /*!
       В режиме регистрации (TED::Fptr::ModeRegistration) печатает копию последнего чека.
       В режиме отчетов с гашением (TED::Fptr::ModeReportClear) печатает копию последнего отчета с гашением.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrintLastCheckCopy() = 0;

    //! Устанавливает флаг отложенной печати ШК.
    /*!
       \param value Флаг (1 (true) - отложить печать, 0 (false) - печатать сразу)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeDeferredPrint()
     */
    virtual int DTOSHARED_CCA put_BarcodeDeferredPrint(int value) = 0;

    //! Возвращает флаг отложенной печати ШК.
    /*!
       \param value Флаг (1 (true) - отложить печать, 0 (false) - печатать сразу)
       \retval 0 Успех
       \sa put_BarcodeDeferredPrint()
     */
    virtual int DTOSHARED_CCA get_BarcodeDeferredPrint(int *value) = 0;

    //! Устанавливает дескриптор для получения данных от устройства ввода.
    /*!
       Используется, когда устройство ввода соединено с портом ККТ.
       При получении сообщения от УВ, будет вызываться метод данного дескриптора.
       \param value Дескриптор
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_ScannerEventHandler(AbstractScannerEventHandler *value) = 0;

    //! Возвращает дескриптор для управления портом ККТ из дУВ.
    /*!
       Используется для связи дУВ и дККТ в случае, когда устройство ввода соединено с портом ККТ.
       Полученный с помощью данного метода дескриптор необходимо передать в НАСТРОЕННЫЙ экземпляр дУВ.
       С помощью данного дескриптора дУВ будет активировать дККТ, если он не активирован,
       а также устанавливать свойство ScannerMode (put_ScannerMode()) в TED::Fptr::ScannerModeAsync.
       \param value Дескриптор
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ScannerPortHandler(void **value) = 0;

    //! Устанавливает режим опроса устройства ввода.
    /*!
       \param value Режим опроса УВ
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ScannerMode()
       \sa TED::Fptr::ScannerMode
     */
    virtual int DTOSHARED_CCA put_ScannerMode(int value) = 0;

    //! Возвращает режим опроса устройства ввода.
    /*!
       \param value Режим опроса УВ
       \retval 0 Успех
       \sa put_ScannerMode()
       \sa TED::Fptr::ScannerMode
     */
    virtual int DTOSHARED_CCA get_ScannerMode(int *value) = 0;

    //! Устанавливает режим опроса пинпада.
    /*!
       \param value Режим опроса
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PinPadMode()
       \sa TED::Fptr::PinPadMode
     */
    virtual int DTOSHARED_CCA put_PinPadMode(int value) = 0;

    //! Возвращает режим опроса пинпада.
    /*!
       \param value Режим опроса
       \retval 0 Успех
       \sa put_PinPadMode()
       \sa TED::Fptr::PinPadMode
     */
    virtual int DTOSHARED_CCA get_PinPadMode(int *value) = 0;

    //! Записывает данные в порт пинпада.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CommandBuffer   <td>Данные      <td>put_CommandBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa ReadPinPad()
     */
    virtual int DTOSHARED_CCA WritePinPad() = 0;

    //! Считывает данные из порта пинпада.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReadSize    <td>Данные      <td>put_ReadSize()
        <tr><th colspan=3>Выходные свойства
        <tr><td>AnswerBuffer    <td>Данные      <td>put_AnswerBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa WritePinPad()
     */
    virtual int DTOSHARED_CCA ReadPinPad() = 0;

    //! Возвращает дескриптор пинпада.
    /*!
       Используется для связи дПС и дККТ в случае работы с пинпадом FPrintPay-01 и АТОЛ 60Ф.
       С помощью данного дескриптора будет осуществляться управление пинпадом.

       \warning Полученный дескриптор актуален до следующей перенастройки драйвера (ApplySingleSettings() или put_DeviceSettings())!

       \param value Дескриптор пинпада
       \retval -1 Ошибка
       \retval 0 Успех
       \sa TED::PayCard::IPayCard::put_PinPadDevice()
     */
    virtual int DTOSHARED_CCA get_PinPadDevice(void **value) = 0;

    //! Включает питание пинпада.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PowerOffPinPad()
     */
    virtual int DTOSHARED_CCA PowerOnPinPad() = 0;

    //! Выключает питание пинпада.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PowerOnPinPad()
     */
    virtual int DTOSHARED_CCA PowerOffPinPad() = 0;

    //! Устанавливает режим опроса модема.
    /*!
       \param value Режим опроса
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ModemMode()
       \sa TED::Fptr::ModemMode
     */
    virtual int DTOSHARED_CCA put_ModemMode(int value) = 0;

    //! Возвращает режим опроса модема.
    /*!
       \param value Режим опроса
       \retval 0 Успех
       \sa put_ModemMode()
       \sa TED::Fptr::ModemMode
     */
    virtual int DTOSHARED_CCA get_ModemMode(int *value) = 0;

    //! Записывает данные в порт модема.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CommandBuffer   <td>Данные      <td>put_CommandBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA WriteModem() = 0;

    //! Считывает данные из порта модема.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReadSize    <td>Данные      <td>put_ReadSize()
        <tr><th colspan=3>Выходные свойства
        <tr><td>AnswerBuffer    <td>Данные      <td>put_AnswerBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ReadModem() = 0;

    //! Возвращает дескриптор модема.
    /*!
       Используется для связи дПС и дККТ в случае работы с модемом FPrintPay-01.
       С помощью данного дескриптора будет осуществляться управление модемом.

       \warning Полученный дескриптор актуален до следующей перенастройки драйвера (ApplySingleSettings() или put_DeviceSettings())!

       \param value Дескриптор модема
       \retval -1 Ошибка
       \retval 0 Успех
       \sa TED::PayCard::IPayCard::put_ModemDevice
     */
    virtual int DTOSHARED_CCA get_ModemDevice(void **value) = 0;

    //! Включает питание модема.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PowerOffModem()
     */
    virtual int DTOSHARED_CCA PowerOnModem() = 0;

    //! Выключает питание модема.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PowerOnModem()
     */
    virtual int DTOSHARED_CCA PowerOffModem() = 0;

    //! Устанавливает размер данных для чтения.
    /*!
       \param value Размер данных
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReadSize()
     */
    virtual int DTOSHARED_CCA put_ReadSize(int value) = 0;

    //! Возвращает размер данных для чтения.
    /*!
       \param value Размер данных
       \retval 0 Успех
       \sa put_ReadSize()
     */
    virtual int DTOSHARED_CCA get_ReadSize(int *value) = 0;

    //! Устанавливает логический номер кассы.
    /*!
       \param value Размер данных
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_LogicalNumber()
     */
    virtual int DTOSHARED_CCA put_LogicalNumber(int value) = 0;

    //! Устанавливает флаг ожидания ответа.
    /*!
       \param value Флаг (1 (true) - ждать ответ, 0 (false) - не ждать ответ)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_NeedResultFlag()
     */
    virtual int DTOSHARED_CCA put_NeedResultFlag(int value) = 0;

    //! Возвращает флаг ожидания ответа.
    /*!
       \param value Флаг (1 (true) - ждать ответ, 0 (false) - не ждать ответ)
       \retval 0 Успех
       \sa put_NeedResultFlag()
     */
    virtual int DTOSHARED_CCA get_NeedResultFlag(int *value) = 0;

    //! Открывает соединение с пинпадом.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>PinPadMode  <td>Режим работы с пинпадом     <td>put_PinPadMode()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa ClosePinPad()
     */
    virtual int DTOSHARED_CCA OpenPinPad() = 0;

    //! Закрывает соединение с пинпадом.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa OpenPinPad()
     */
    virtual int DTOSHARED_CCA ClosePinPad() = 0;

    //! Открывает соединение по модему.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ModemMode           <td>Режим работы с модемом      <td>put_ModemMode()
        <tr><td>ModemAddress        <td>IP-адрес для соединения     <td>put_ModemAddress()
        <tr><td>ModemPort           <td>IP-порт для соединения      <td>put_ModemPort()
        <tr><td>ModemConnectionType <td>Способ соединения по модему <td>put_ModemConnectionType()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa CloseModem()
     */
    virtual int DTOSHARED_CCA OpenModem() = 0;

    //! Закрывает соединение по модему.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa OpenModem()
     */
    virtual int DTOSHARED_CCA CloseModem() = 0;

    //! Устанавливает тип соединения по модему.
    /*!
       \param value Тип соединения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ModemConnectionType()
     */
    virtual int DTOSHARED_CCA put_ModemConnectionType(int value) = 0;

    //! Возвращает тип соединения по модему.
    /*!
       \param value Тип соединения
       \retval 0 Успех
       \sa put_ModemConnectionType()
     */
    virtual int DTOSHARED_CCA get_ModemConnectionType(int *value) = 0;

    //! Устанавливает IP-адрес для соединения по модему.
    /*!
       \param value Адрес
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ModemAddress()
     */
    virtual int DTOSHARED_CCA put_ModemAddress(const wchar_t *value) = 0;

    //! Возвращает IP-адрес для соединения по модему.
    /*!
       \param bfr Буфер для адреса
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa get_ModemAddress()
     */
    virtual int DTOSHARED_CCA get_ModemAddress(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает IP-порт для соединения по модему.
    /*!
       \param value Порт
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ModemPort()
     */
    virtual int DTOSHARED_CCA put_ModemPort(int value) = 0;

    //! Возвращает IP-порт для соединения по модему.
    /*!
       \param value Порт
       \retval 0 Успех
       \sa put_ModemPort()
     */
    virtual int DTOSHARED_CCA get_ModemPort(int *value) = 0;

    //! Запрашивает состояние модема.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>ModemStatus     <td>Состояние модема                    <td>get_ModemStatus()
        <tr><td>ModemSignal     <td>Уровень сигнала                     <td>get_ModemSignal()
        <tr><td>ModemOperator   <td>Оператор                            <td>get_ModemOperator()
        <tr><td>ModemError      <td>Последняя ошибка                    <td>get_ModemError()
        <tr><td>ReadSize        <td>Размер доступных для чтения данных  <td>get_ReadSize()
        <tr><td>WriteSize       <td>Размер данных, ожидающих отправки   <td>get_WriteSize()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetModemStatus() = 0;

    //! Запрашивает состояние пинпада.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetPinPadStatus() = 0;

    //! Возвращает размер данных для записи.
    /*!
       \param value Размер
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_WriteSize(int *value) = 0;

    //! Возвращает состояние модема.
    /*!
       \param value Состояние
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ModemStatus(int *value) = 0;

    //! Возвращает уровень сигнала модема.
    /*!
       \param value Уровень сигнала
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ModemSignal(int *value) = 0;

    //! Возвращает оператора сети модема.
    /*!
       \param bfr Буфер для оператора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ModemOperator(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает ошибку модема.
    /*!
       \param bfr Буфер для ошибки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ModemError(wchar_t *bfr, int bfrSize) = 0;

    //! Запрашивает описание устройства.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>VerHi               <td>Версия ПО           <td>get_VerHi()
        <tr><td>VerLo               <td>Подверсия ПО        <td>get_VerLo()
        <tr><td>Build               <td>Версия сборки ПО    <td>get_Build()
        <tr><td>DeviceDescription   <td>Описание устройства <td>get_DeviceDescription()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetDeviceMetrics() = 0;

    //! Возвращает описание устройства.
    /*!
       \param bfr Буфер для описания
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_DeviceDescription(wchar_t *bfr, int bfrSize) = 0;

    //! Запрашивает текущий режим.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>Mode                        <td>Режим                               <td>get_Mode()
        <tr><td>AdvancedMode                <td>Подрежим                            <td>get_AdvancedMode()
        <tr><td>OutOfPaper                  <td>Флаг отсутствия бумаги              <td>get_OutOfPaper()
        <tr><td>PrinterConnectionFailed     <td>Флаг отсутствия связи с принтером   <td>get_PrinterConnectionFailed()
        <tr><td>PrinterMechanismError       <td>Флаг ошибки печатающего устройства  <td>get_PrinterMechanismError()
        <tr><td>PrinterCutMechanismError    <td>Флаг ошибки отрезчика               <td>get_PrinterCutMechanismError()
        <tr><td>PrinterOverheatError        <td>Флаг перегрева печатающей головки   <td>get_PrinterOverheatError()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetCurrentMode() = 0;

    //! Возвращает флаг отсутствия бумаги.
    /*!
       \param value Флаг отсутствия бумаги (1 (true) - нет бумаги, 0 (false) - есть бумага)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_OutOfPaper(int *value) = 0;

    //! Возвращает флаг отсутствия связи с принтером.
    /*!
       \param value Флаг отсутствия связи с принтером (1 (true) - нет связи, 0 (false) - есть связь)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PrinterConnectionFailed(int *value) = 0;

    //! Возвращает флаг ошибки печатающего устройства.
    /*!
       \param value Флаг ошибки печатающего устройства (1 (true) - есть ошибка, 0 (false) - нет ошибки)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PrinterMechanismError(int *value) = 0;

    //! Возвращает флаг ошибки отрезчика.
    /*!
       \param value Флаг ошибки отрезчика (1 (true) - есть ошибка, 0 (false) - нет ошибки)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PrinterCutMechanismError(int *value) = 0;

    //! Возвращает флаг перегрева печатающей головки.
    /*!
       \param value Флаг перегрева печатающей головки (1 (true) - перегрев, 0 (false) - нет перегрева)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PrinterOverheatError(int *value) = 0;

    //! Запрашивает краткое состояние ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>SerialNumber        <td>Заводской номер                     <td>get_SerialNumber()
        <tr><td>Session             <td>Смена                               <td>get_Session()
        <tr><td>CheckNumber         <td>Номер чека                          <td>get_CheckNumber()
        <tr><td>Date                <td>Текущая дата                        <td>get_Date()
        <tr><td>Time                <td>Текущее время                       <td>get_Time()
        <tr><td>Operator            <td>Номер оператора                     <td>get_Operator()
        <tr><td>LogicalNumber       <td>Номер ККТ                           <td>get_LogicalNumber()
        <tr><td>SessionOpened       <td>Флаг состояния смены                <td>get_SessionOpened()
        <tr><td>Fiscal              <td>Флаг фискальности                   <td>get_Fiscal()
        <tr><td>DrawerOpened        <td>Флаг состояния денежного ящика      <td>get_DrawerOpened()
        <tr><td>CoverOpened         <td>Флаг состояния крышки               <td>get_CoverOpened()
        <tr><td>CheckPaperPresent   <td>Флаг наличия ЧЛ                     <td>get_CheckPaperPresent()
        <tr><td>ControlPaperPresent <td>Флаг наличия КЛ                     <td>get_ControlPaperPresent()
        <tr><td>Model               <td>Модель                              <td>get_Model()
        <tr><td>Mode                <td>Режим                               <td>get_Mode()
        <tr><td>AdvancedMode        <td>Подрежим                            <td>get_AdvancedMode()
        <tr><td>CheckState          <td>Состояние чека                      <td>get_CheckState()
        <tr><td>SummPointPosition   <td>Позиция десятичной точки в суммах   <td>get_SummPointPosition()
        <tr><td>SlotNumber          <td>Номер/тип порта                     <td>get_SlotNumber()
        <tr><td>Summ                <td>Сумма чека                          <td>get_Summ()
        <tr><td>FNFiscal            <td>Флаг фискализации ФН                <td>get_FNFiscal()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetCurrentStatus() = 0;

    //! Запрашивает последний сменный итог продаж.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>Summ    <td>Итог    <td>get_Summ()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetLastSummary() = 0;

    //! Возвращает подрежим.
    /*!
       \param value Подрежим
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_AdvancedMode(int *value) = 0;

    //! Устанавливает отступ снизу.
    /*!
       \param value Отступ снизу
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BottomMargin()
     */
    virtual int DTOSHARED_CCA put_BottomMargin(int value) = 0;

    //! Возвращает отступ снизу.
    /*!
       \param value Отступ снизу
       \retval 0 Успех
       \sa put_BottomMargin()
     */
    virtual int DTOSHARED_CCA get_BottomMargin(int *value) = 0;

    //! Запрашивает КПК по его номеру.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>EKLZKPKNumber   <td>Номер КПК   <td>put_EKLZKPKNumber()
        <tr><th colspan=3>Выходные свойства
        <tr><td>EKLZKPK         <td>КПК         <td>get_EKLZKPK()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA EKLZGetKPK() = 0;

    //! Возвращает КПК.
    /*!
       \param value КПК
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_EKLZKPK(int *value) = 0;

    //! Устанавливает версию ШК.
    /*!
       \param value Версия ШК
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeVersion()
     */
    virtual int DTOSHARED_CCA put_BarcodeVersion(int value) = 0;

    //! Возвращает версию ШК.
    /*!
       \param value Версия ШК
       \retval 0 Успех
       \sa put_BarcodeVersion()
     */
    virtual int DTOSHARED_CCA get_BarcodeVersion(int *value) = 0;

    //! Устанавливает пароль налогового инспектора.
    /*!
       \param value Пароль
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TaxPassword()
     */
    virtual int DTOSHARED_CCA put_TaxPassword(const wchar_t *value) = 0;

    //! Возвращает пароль налогового инспектора.
    /*!
       \param bfr Буфер для пароля
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_TaxPassword()
     */
    virtual int DTOSHARED_CCA get_TaxPassword(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает классификатор товара.
    /*!
       \param value Классификатор
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Classifier()
     */
    virtual int DTOSHARED_CCA put_Classifier(const wchar_t *value) = 0;

    //! Возвращает классификатор товара.
    /*!
       \param bfr Буфер для классификатора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_Classifier()
     */
    virtual int DTOSHARED_CCA get_Classifier(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает номер реквизита.
    /*!
       \param value Номер реквизита
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FiscalPropertyNumber()
     */
    virtual int DTOSHARED_CCA put_FiscalPropertyNumber(int value) = 0;

    //! Возвращает номер реквизита.
    /*!
       \param value Номер реквизита
       \retval 0 Успех
       \sa put_FiscalPropertyNumber()
     */
    virtual int DTOSHARED_CCA get_FiscalPropertyNumber(int *value) = 0;

    //! Устанавливает значение реквизита.
    /*!
       \param value Значение реквизита
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FiscalPropertyValue()
     */
    virtual int DTOSHARED_CCA put_FiscalPropertyValue(const wchar_t *value) = 0;

    //! Возвращает значение реквизита.
    /*!
       \param bfr Буфер для значения реквизита
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_FiscalPropertyValue()
     */
    virtual int DTOSHARED_CCA get_FiscalPropertyValue(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает тип реквизита.
    /*!
       \param value Тип реквизита
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FiscalPropertyType()
     */
    virtual int DTOSHARED_CCA put_FiscalPropertyType(int value) = 0;

    //! Возвращает тип реквизита.
    /*!
       \param value Тип реквизита
       \retval 0 Успех
       \sa put_FiscalPropertyType()
     */
    virtual int DTOSHARED_CCA get_FiscalPropertyType(int *value) = 0;

    //! Устанавливает флаг печати реквизита.
    /*!
       \param value Флаг (1 (true) - печатать, 0 (false) - не печатать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FiscalPropertyPrint()
     */
    virtual int DTOSHARED_CCA put_FiscalPropertyPrint(int value) = 0;

    //! Возвращает флаг печати реквизита.
    /*!
       \param value Флаг (1 (true) - печатать, 0 (false) - не печатать)
       \retval 0 Успех
       \sa put_FiscalPropertyPrint()
     */
    virtual int DTOSHARED_CCA get_FiscalPropertyPrint(int *value) = 0;

    //! Записывает реквизит в фискальный накопитель.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FiscalPropertyNumber    <td>Номер рекзивита         <td>put_FiscalPropertyNumber()
        <tr><td>FiscalPropertyType      <td>Тип реквизита           <td>put_FiscalPropertyType()
        <tr><td>FiscalPropertyValue     <td>Значение реквизита      <td>put_FiscalPropertyValue()
        <tr><td>FiscalPropertyPrint     <td>Флаг печати реквизита   <td>put_FiscalPropertyPrint()
        <tr><td>FiscalPropertyUser      <td>Флаг пользовательского реквизита   <td>put_FiscalPropertyUser()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA WriteFiscalProperty() = 0;

    //! Считывает реквизит из фискального накопителя.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FiscalPropertyNumber    <td>Номер рекзивита         <td>put_FiscalPropertyNumber()
        <tr><td>FiscalPropertyType      <td>Тип рекзивита           <td>put_FiscalPropertyType()
        <tr><th colspan=3>Выходные свойства
        <tr><td>FiscalPropertyValue     <td>Значение реквизита      <td>put_FiscalPropertyValue()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ReadFiscalProperty() = 0;

    //! Возвращает флаг наличия неотправленных в ОФД документов.
    /*!
       \warning Не используется в текущей версии
       \param value Флаг (1 (true) - есть неотправленные документы, 0 (false) - нет неотправленных документов)
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_HasNotSendedDocs(int *value) = 0;

    //! Выполняет команду фискального накопителя.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CommandBuffer   <td>Команда     <td>put_CommandBuffer()
        <tr><th colspan=3>Выходные свойства
        <tr><td>AnswerBuffer    <td>Ответ       <td>get_AnswerBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA RunFNCommand() = 0;

    //! Возвращает размерность счетчика.
    /*!
       \param value Размерность
       \retval 0 Успех
       \sa put_CounterDimension()
     */
    virtual int DTOSHARED_CCA get_CounterDimension(int *value) = 0;

    //! Устанавливает размерность счетчика.
    /*!
       \param value Размерность
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CounterDimension()
     */
    virtual int DTOSHARED_CCA put_CounterDimension(int value) = 0;

    //! Возвращает сумму скидок за смену.
    /*!
       \param value Сумма надбавок
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_DiscountInSession(double *value) = 0;

    //! Возвращает сумму надбавок за смену.
    /*!
       \param value Сумма надбавок
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ChargeInSession(double *value) = 0;

    //! Возвращает код ошибки сети.
    /*!
       \param value Код ошибки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_NetworkError(int *value) = 0;

    //! Возвращает код ошибки ОФД.
    /*!
       \param value Код ошибки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_OFDError(int *value) = 0;

    //! Возвращает код ошибки ФН.
    /*!
       \param value Код ошибки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_FNError(int *value) = 0;

    //! Устанавливает таймаут ожидания ответа на прием команды.
    /*!
       \param value Таймаут
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TimeoutACK()
     */
    virtual int DTOSHARED_CCA put_TimeoutACK(int value) = 0;

    //! Возвращает таймаут ожидания ответа на прием команды.
    /*!
       \param value Таймаут
       \retval 0 Успех
       \sa put_TimeoutACK()
     */
    virtual int DTOSHARED_CCA get_TimeoutACK(int *value) = 0;

    //! Устанавливает таймаут ожидания ответа на команду.
    /*!
       \param value Таймаут
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TimeoutENQ()
     */
    virtual int DTOSHARED_CCA put_TimeoutENQ(int value) = 0;

    //! Возвращает таймаут ожидания ответа на команду.
    /*!
       \param value Таймаут
       \retval 0 Успех
       \sa put_TimeoutENQ()
     */
    virtual int DTOSHARED_CCA get_TimeoutENQ(int *value) = 0;

    //! Формирует штрихкод в памяти ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>BarcodeType             <td>Тип ШК (TED::Fptr::BarcodeType)                 <td>put_BarcodeType()
        <tr><td>Barcode                 <td>Данные ШК                                       <td>put_Barcode()
        <tr><td>BarcodePixelProportions <td>Пропорции пикселя ШК                            <td>put_BarcodePixelProportions()
        <tr><td>BarcodeProportions      <td>Пропорции ШК                                    <td>put_BarcodeProportions()
        <tr><td>BarcodeColumns          <td>Количество столбцов ШК                          <td>put_BarcodeColumns()
        <tr><td>BarcodeRows             <td>Количество строк ШК                             <td>put_BarcodeRows()
        <tr><td>BarcodePackingMode      <td>Режим упаковки ШК                               <td>put_BarcodePackingMode()
        <tr><td>BarcodeUseProportions   <td>Флаг использования пропорций ШК                 <td>put_BarcodeUseProportions()
        <tr><td>BarcodeUseRows          <td>Флаг использования кол-ва строк ШК              <td>put_BarcodeUseRows()
        <tr><td>BarcodeUseColumns       <td>Флаг использования кол-ва столбцов ШК           <td>put_BarcodeUseColumns()
        <tr><td>BarcodeUseCorrection    <td>Флаг использования коррекции ШК                 <td>put_BarcodeUseCorrection()
        <tr><td>BarcodeInvert           <td>Флаг инверсии ШК                                <td>put_BarcodeInvert()
        <tr><td>%Scale                  <td>Масштаб                                         <td>put_Scale()
        <tr><td>BarcodeVersion          <td>Версия ШК                                       <td>put_BarcodeVersion()
        <tr><td>BarcodeCorrection       <td>Коррекция ШК (TED::Fptr::BarcodeQRCorrection)   <td>put_BarcodeCorrection()
        <tr><td>BarcodeEncoding         <td>Кодировка ШК (TED::Fptr::BarcodeQREncoding)     <td>put_BarcodeEncoding()
        <tr><td>BarcodeEncodingMode     <td>Режим кодировки ШК (TED::Fptr::BarcodeQREncodingMode)<td>put_BarcodeEncodingMode()
        <tr><td>PrintBarcodeText        <td>Флаг печати данных ШК                           <td>put_PrintBarcodeText()
        <tr><td>Height                  <td>Высота ШК                                       <td>put_Height()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddBarcode() = 0;

    //! Запрашивает информацию о массиве штрихкодов.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>BarcodeNumber   <td>Количество изображений в памяти         <td>get_BarcodeNumber()
        <tr><td>Memory          <td>Количество свободной памяти в массиве   <td>get_Memory()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetBarcodeArrayStatus() = 0;

    //! Производит регистрацию коррекции прихода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
        <tr><td>Classifier      <td>Классификатор товара            <td>put_Classifier()
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек коррекции прихода.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Correction() = 0;

    //! Производит регистрацию коррекции возврата прихода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
        <tr><td>Classifier      <td>Классификатор товара            <td>put_Classifier()
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек коррекции возврата прихода.

       \warning Более не актуален.

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ReturnCorrection() = 0;

    //! Производит регистрацию коррекции расхода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
        <tr><td>Classifier      <td>Классификатор товара            <td>put_Classifier()
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек коррекции расхода.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA BuyCorrection() = 0;

    //! Производит регистрацию коррекции возврата расхода.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Name            <td>Наименование товара             <td>put_Name()
        <tr><td>Price           <td>Цена товара                     <td>put_Price()
        <tr><td>Quantity        <td>Количество товара               <td>put_Quantity()
        <tr><td>Department      <td>Секция                          <td>put_Department()
        <tr><td>TestMode        <td>Выполнить в тестовом режиме     <td>put_TestMode()
        <tr><td>Classifier      <td>Классификатор товара            <td>put_Classifier()
        <tr><td>PositionSum     <td>Полная сумма позиции            <td>put_PositionSum()
        <tr><td>TaxNumber       <td>Номер налоговой ставки          <td>put_TaxNumber() (TED::Fptr::TaxNumber)
        <tr><td>TaxSum          <td>Сумма налога                    <td>put_TaxSum()
        <tr><td>TaxMode         <td>Режим работы с налогом          <td>put_TaxMode() (TED::Fptr::TaxMode)
        <tr><td>PositionType    <td>Признак предмета расчета        <td>put_PositionType()
        <tr><td>PositionPaymentType <td>Признак способа расчета     <td>put_PositionPaymentType()
        <tr><td>Summ            <td>Информационная скидка / надбавка<td>put_Summ()
        <tr><td>PrintCheck      <td>Печатать чек                    <td>put_PrintCheck()
       </table>

       Работает из режима регистрации (TED::Fptr::ModeRegistration).

       При закрытом чеке открывает чек коррекции возврата расхода.

       \warning Более не актуален.

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA BuyReturnCorrection() = 0;

    //! Устанавливает флаг "Печатать чек".
    /*!
       \param value Флаг (1 (true) - печатать, 0 (false) - не печатать)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PrintCheck()
     */
    virtual int DTOSHARED_CCA put_PrintCheck(int value) = 0;

    //! Возвращает флаг "Печатать чек".
    /*!
       \param value Флаг (1 (true) - печатать, 0 (false) - не печатать)
       \retval 0 Успех
       \sa put_PrintCheck()
     */
    virtual int DTOSHARED_CCA get_PrintCheck(int *value) = 0;

    //! Возвращает состояние ФН.
    /*!
       \param value Флаги состояния ФН
        Бит 0 – проведена настройка ФН
        Бит 1 – открыт фискальный режим
        Бит 2 – постфискальный режим
        Бит 3 – закончена передача фискальных данных в ОФД
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_FNState(int *value) = 0;

    //! Запрашивет версию ПО модуля ККТ
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>UnitType            <td>Тип модуля    <td>put_UnitType() (TED::Fptr::UnitType)
        <tr><th colspan=3>Выходные свойства
        <tr><td>VerHi               <td>Версия ПО               <td>get_VerHi()
        <tr><td>VerLo               <td>Подверсия ПО            <td>get_VerLo()
        <tr><td>Build               <td>Версия сборки ПО        <td>get_Build()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetUnitVersion() = 0;
    //! Возвращает сумму налога.
    /*!
       \param value Сумма налога
       \retval 0 Успех
       \sa put_TaxSum()
     */
    virtual int DTOSHARED_CCA get_TaxSum(double *value) = 0;

    //! Устанавливает сумму налога.
    /*!
       \param value Сумма налога
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TaxSum()
     */
    virtual int DTOSHARED_CCA put_TaxSum(double value) = 0;

    //! Возвращает режим работы налога.
    /*!
       \param value Режим работы налога
       \retval 0 Успех
       \sa put_TaxMode()
     */
    virtual int DTOSHARED_CCA get_TaxMode(int *value) = 0;

    //! Устанавливает режим работы налога.
    /*!
       \param value Режим работы налога
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TaxMode()
     */
    virtual int DTOSHARED_CCA put_TaxMode(int value) = 0;

    //! Возвращает предмет расчета.
    /*!
       \param value Предмет расчета
       \retval 0 Успех
       \sa put_PositionType()
     */
    virtual int DTOSHARED_CCA get_PositionType(int *value) = 0;

    //! Устанавливает предмет расчета.
    /*!
       \param value Предмет расчета
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PositionType()
     */
    virtual int DTOSHARED_CCA put_PositionType(int value) = 0;

    //! Возвращает способ расчета.
    /*!
       \param value Способ расчета
       \retval 0 Успех
       \sa put_PositionPaymentType()
     */
    virtual int DTOSHARED_CCA get_PositionPaymentType(int *value) = 0;

    //! Устанавливает способ расчета.
    /*!
       \param value Способ расчета
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PositionPaymentType()
     */
    virtual int DTOSHARED_CCA put_PositionPaymentType(int value) = 0;

    //! Добавляет произвольный реквизит к позиции / буферу.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>FiscalPropertyNumber    <td>Номер рекзивита         <td>put_FiscalPropertyNumber()
        <tr><td>FiscalPropertyType      <td>Тип реквизита           <td>put_FiscalPropertyType()
        <tr><td>FiscalPropertyValue     <td>Значение реквизита      <td>put_FiscalPropertyValue()
        <tr><td>FiscalPropertyPrint     <td>Флаг печати реквизита   <td>put_FiscalPropertyPrint()
        <tr><td>FiscalPropertyUser      <td>Флаг пользовательского реквизита   <td>put_FiscalPropertyUser()
       </table>

       \warning Метод меняет свое поведение после вызова \ref BeginFormFiscalProperty().
        Подробнее в описании метода \ref BeginFormFiscalProperty().

       Данный метод необходимо вызывать перед непосредственной регистрацией позиции.

       Список реквизитов автоматически очищается после удачной регистрации позиции.

       Примерный алгоритм использования данного метода:
       \code{.cpp}
        fptr->put_FiscalPropertyNumber(1197);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"кг");
        fptr->AddFiscalProperty();

        fptr->put_Name(L"Мороженое");
        ...
        fptr->Registration();
       \endcode

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddFiscalProperty() = 0;

    //! Сбрасывает список произвольных реквизитов позиции.
    /*!
       Данный метод необходимо вызывать перед непосредственной регистрацией позиции.

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ResetFiscalProperties() = 0;

    //! Возвращает версию формата фискальных данных.
    /*!
       \param value Версия
       \retval 0 Успех
       \sa FFDVersion
     */
    virtual int DTOSHARED_CCA get_FfdVersion(int *value) = 0;

    //! Возвращает версию формата фискальных данных ККТ.
    /*!
       \param value Версия
       \retval 0 Успех
       \sa FFDVersion
     */
    virtual int DTOSHARED_CCA get_DeviceFfdVersion(int *value) = 0;

    //! Возвращает версию формата фискальных данных ФН.
    /*!
       \param value Версия
       \retval 0 Успех
       \sa FFDVersion
     */
    virtual int DTOSHARED_CCA get_FNFfdVersion(int *value) = 0;

    //! Возвращает код команды.
    /*!
       \param value Код команды
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_CommandCode(int *value) = 0;

    //! Возвращает код ошибки.
    /*!
       \param value Код ошибки
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ErrorCode(int *value) = 0;

    //! Возвращает данные ошибки.
    /*!
       \param bfr Буфер для строки
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ErrorData(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает сумму позиции.
    /*!
       \param value Сумма
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_PositionSum(double value) = 0;

    //! Возвращает сумму позиции.
    /*!
       \param value Сумма
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_PositionSum(double *value) = 0;

    //! Устанавливает флаг пользовательского реквизита.
    /*!
       \param value Флаг (1 (true) - пользовательский, 0 (false) - реквизит ФН)
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_FiscalPropertyUser()
     */
    virtual int DTOSHARED_CCA put_FiscalPropertyUser(int value) = 0;

    //! Возвращает флаг печати реквизита.
    /*!
       \param value Флаг (1 (true) - пользовательский, 0 (false) - реквизит ФН)
       \retval 0 Успех
       \sa put_FiscalPropertyUser()
     */
    virtual int DTOSHARED_CCA get_FiscalPropertyUser(int *value) = 0;

    //! Устанавливает режим опроса Wi-Fi.
    /*!
       \param value Режим опроса
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_WiFiMode()
       \sa TED::Fptr::WiFiMode
     */
    virtual int DTOSHARED_CCA put_WiFiMode(int value) = 0;

    //! Возвращает режим опроса Wi-Fi.
    /*!
       \param value Режим опроса
       \retval 0 Успех
       \sa put_WiFiMode()
       \sa TED::Fptr::WiFiMode
     */
    virtual int DTOSHARED_CCA get_WiFiMode(int *value) = 0;

    //! Записывает данные в порт Wi-Fi.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CommandBuffer   <td>Данные      <td>put_CommandBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA WriteWiFi() = 0;

    //! Считывает данные из порта Wi-Fi.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReadSize    <td>Данные      <td>put_ReadSize()
        <tr><th colspan=3>Выходные свойства
        <tr><td>AnswerBuffer    <td>Данные      <td>put_AnswerBuffer()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ReadWiFi() = 0;

    //! Возвращает дескриптор WiFi.
    /*!
       Используется для связи дПС и дККТ в случае работы с Wi-Fi АТОЛ 60Ф.
       С помощью данного дескриптора будет осуществляться управление Wi-Fi.
       \param value Дескриптор Wi-Fi
       \retval -1 Ошибка
       \retval 0 Успех
       \sa TED::PayCard::IPayCard::put_WiFiDevice
     */
    virtual int DTOSHARED_CCA get_WiFiDevice(void **value) = 0;

    //! Включает питание Wi-Fi.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PowerOffWiFi()
     */
    virtual int DTOSHARED_CCA PowerOnWiFi() = 0;

    //! Выключает питание Wi-Fi.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa PowerOnWiFi()
     */
    virtual int DTOSHARED_CCA PowerOffWiFi() = 0;


    //! Устанавливает тип соединения по Wi-Fi.
    /*!
       \param value Тип соединения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_WiFiConnectionType()
     */
    virtual int DTOSHARED_CCA put_WiFiConnectionType(int value) = 0;

    //! Возвращает тип соединения по Wi-Fi.
    /*!
       \param value Тип соединения
       \retval 0 Успех
       \sa put_WiFiConnectionType()
     */
    virtual int DTOSHARED_CCA get_WiFiConnectionType(int *value) = 0;

    //! Устанавливает IP-адрес для соединения по Wi-Fi.
    /*!
       \param value Адрес
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_WiFiAddress()
     */
    virtual int DTOSHARED_CCA put_WiFiAddress(const wchar_t *value) = 0;

    //! Возвращает IP-адрес для соединения по Wi-Fi.
    /*!
       \param bfr Буфер для адреса
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa get_WiFiAddress()
     */
    virtual int DTOSHARED_CCA get_WiFiAddress(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает IP-порт для соединения по Wi-Fi.
    /*!
       \param value Порт
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_WiFiPort()
     */
    virtual int DTOSHARED_CCA put_WiFiPort(int value) = 0;

    //! Возвращает IP-порт для соединения по Wi-Fi.
    /*!
       \param value Порт
       \retval 0 Успех
       \sa put_WiFiPort()
     */
    virtual int DTOSHARED_CCA get_WiFiPort(int *value) = 0;

    //! Запрашивает состояние Wi-Fi.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>ModemStatus     <td>Состояние соединения                <td>get_WiFiStatus()
        <tr><td>ReadSize        <td>Размер доступных для чтения данных  <td>get_ReadSize()
        <tr><td>WriteSize       <td>Размер данных, ожидающих отправки   <td>get_WriteSize()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA GetWiFiStatus() = 0;

    //! Возвращает состояние Wi-Fi.
    /*!
       \param value Состояние
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_WiFiStatus(int *value) = 0;

    //! Открывает соединение по Wi-Fi.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>WiFiMode           <td>Режим работы с Wi-Fi        <td>put_WiFiMode()
        <tr><td>WiFiAddress        <td>IP-адрес для соединения     <td>put_WiFiAddress()
        <tr><td>WiFiPort           <td>IP-порт для соединения      <td>put_WiFiPort()
        <tr><td>WiFiConnectionType <td>Способ соединения по Wi-Fi  <td>put_WiFiConnectionType()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa CloseModem()
     */
    virtual int DTOSHARED_CCA OpenWiFi() = 0;

    //! Закрывает соединение по Wi-Fi.
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
       \sa OpenModem()
     */
    virtual int DTOSHARED_CCA CloseWiFi() = 0;

    //! Возвращает статус фискализации ФН.
    /*!
       \param value Статус фискализации
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_FNFiscal(int *value) = 0;

    //! Возвращает состояние режима ЕНВД.
    /*!
       \param value Состояние режима ЕНВД
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ENVDMode(int *value) = 0;

    //! Начинает формирование тела тега ФН.
    /*!
       Переводит драйвер в режим формирования составного тега ФН.
       Метод \ref AddFiscalProperty() меняет свое поведение и служит для записи тегов в память драйвера.
       После выхода из режима формирования составного тега (\ref EndFormFiscalProperty())
        можно передать полученный тег в ККТ (\ref WriteFiscalProperty()) или добавить его к позиции (\ref AddFiscalProperty()).
       В этом случае типом записываемого тега всегда должен быть TED::Fptr::FiscalPropertyTypeRaw.

       Пример формирования и записи тега 1223 (Данные агента)
       \code{.cpp}
        fptr->BeginFormFiscalProperty(); // Вход в режим формирования составного тега

        fptr->put_FiscalPropertyNumber(1005);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"ул. Свободы, 13");
        fptr->AddFiscalProperty(); // Добавление адреса оператора перевода (тег 1005)

        fptr->put_FiscalPropertyNumber(1016);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"123456789047");
        fptr->AddFiscalProperty(); // Добавление ИНН оператора перевода (1016)

        fptr->put_FiscalPropertyNumber(1026);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"Оператор 1");
        fptr->AddFiscalProperty(); // Добавление наименования оператора перевода (тег 1026)

        fptr->put_FiscalPropertyNumber(1044);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"Оплата");
        fptr->AddFiscalProperty(); // Добавление операции платежного агента (тег 1044)

        fptr->put_FiscalPropertyNumber(1073);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"+79121234567");
        fptr->AddFiscalProperty(); // Добавление телефона платежного агента (тег 1073)

        fptr->put_FiscalPropertyNumber(1073);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"+79121234590");
        fptr->AddFiscalProperty(); // Добавление еще одного телефона телефона платежного агента (тег 1073)

        fptr->put_FiscalPropertyNumber(1074);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"+79121234568");
        fptr->AddFiscalProperty(); // Добавление телефона оператора по приему платежей (тег 1074)

        fptr->put_FiscalPropertyNumber(1075);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"+79121234569");
        fptr->AddFiscalProperty(); // Добавление телефона оператора перевода (тег 1075)

        fptr->EndFormFiscalProperty(); // Выход из режима формирования составного тега

        // Получение тела тега 1223
        wchar_t tag1223[1024] = {0};
        fptr->get_FiscalPropertyValue(&tag1223[0], sizeof(tag1223) / sizeof(tag1223[0]));

        ...

        // Добавление тега позиции
        fptr->put_FiscalPropertyNumber(1223);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeRaw);
        fptr->put_FiscalPropertyValue(&tag1223[0]);
        fptr->AddFiscalProperty();
        ...
        fptr->Registration();
       \endcode


       Пример формирования и записи тега 1174 (Основание для коррекции)
       \code{.cpp}
        fptr->BeginFormFiscalProperty(); // Вход в режим формирования составного тега

        fptr->put_FiscalPropertyNumber(1177);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"Описание коррекции");
        fptr->AddFiscalProperty(); // Добавление описания коррекции (тег 1177)

        fptr->put_FiscalPropertyNumber(1178);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeUnixTime);
        fptr->put_FiscalPropertyValue(L"1494506624");
        fptr->AddFiscalProperty(); // Добавление даты документа основания для коррекции (тег 1178)

        fptr->put_FiscalPropertyNumber(1179);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeString);
        fptr->put_FiscalPropertyValue(L"123");
        fptr->AddFiscalProperty(); // Добавление номера документа основания для коррекции (тег 1179)

        fptr->EndFormFiscalProperty(); // Выход из режима формирования составного тега

        // Получение тела тега 1174
        wchar_t tag1174[1024] = {0};
        fptr->get_FiscalPropertyValue(&tag1174[0], sizeof(tag1174) / sizeof(tag1174[0]));

        ...

        // Добавление тега чека
        fptr->put_FiscalPropertyNumber(1174);
        fptr->put_FiscalPropertyType(TED::Fptr::FiscalPropertyTypeRaw);
        fptr->put_FiscalPropertyValue(&tag1174[0]);
        fptr->WriteFiscalProperty();
       \endcode

       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA BeginFormFiscalProperty() = 0;

    //! Завершает формирование тела тега ФН.
    /*!
       Подробнее см. \ref BeginFormFiscalProperty().

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>FiscalPropertyValue     <td>Тело тега       <td>get_FiscalPropertyValue()
       </table>

       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA EndFormFiscalProperty() = 0;

    //! Устанавливает уровень логирования.
    /*!
       \param value Уровень логирования
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_LogLvl()
     */
    virtual int DTOSHARED_CCA put_LogLvl(int value) = 0;

    //! Возвращает уровень логирования.
    /*!
       \param value Уровень логирования
       \retval 0 Успех
       \sa put_LogLvl()
     */
    virtual int DTOSHARED_CCA get_LogLvl(int *value) = 0;

    //! Применяет уровень логирования.
    /*!
      <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>LogLvl      <td>Уровень логирования     <td>put_LogLvl()
       </table>

       \warning Изменение уровня логирования применяется ко всем экземплярам драйвера в текущем процессе.

       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetLogLvl() = 0;

    //! Сбрасывает уровень логирования.
    /*!
       Сбрасывает уровень логирования до того, с каким был запущен драйвер
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ResetLogLvl() = 0;

    //! Устанавливает сообщение для лога.
    /*!
       \param value Сообщение
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_LogMessage(const wchar_t *value) = 0;

    //! Записывает сообщение в лог
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>LogMessage      <td>Сообщение   <td>put_LogMessage()
       </table>

       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA WriteLog() = 0;

    //! Возвращает флаг разрешения использования режима ЕНВД.
    /*!
       \param value Флаг
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ENVDEnabled(int *value) = 0;

    //! Возвращает тип нумерации налоговых ставок.
    /*!
       \warning Информационные данные. Драйвер сам определяет нумерацию налогов.

       \param value Тип нумерации налоговых ставок
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_TaxNumeration(int *value) = 0;

    //! Устанавливает конечный номер документа.
    /*!
       \param value Конечный номер документа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DocNumberEnd()
     */
    virtual int DTOSHARED_CCA put_DocNumberEnd(int value) = 0;

    //! Возвращает конечный номер документа.
    /*!
       \param value Конечный номер документа
       \retval 0 Успех
       \sa put_DocNumberEnd()
     */
    virtual int DTOSHARED_CCA get_DocNumberEnd(int *value) = 0;

    //! Устанавливает флаг печати ШК поверх текста.
    /*!
       \param value Флаг
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_BarcodeOverlay()
     */
    virtual int DTOSHARED_CCA put_BarcodeOverlay(int value) = 0;

    //! Возвращает флаг печати ШК поверх текста.
    /*!
       \param value Флаг
       \retval 0 Успех
       \sa put_BarcodeOverlay()
     */
    virtual int DTOSHARED_CCA get_BarcodeOverlay(int *value) = 0;

    //! Устанавливает тип печати кол-ва позиции.
    /*!
       \param value 0 - весовой, 1 - штучный
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PositionQuantityType()
     */
    virtual int DTOSHARED_CCA put_PositionQuantityType(int value) = 0;

    //! Возвращает флаг тип печати кол-ва позиции.
    /*!
       \param value 0 - весовой, 1 - штучный
       \retval 0 Успех
       \sa put_PositionQuantityType()
     */
    virtual int DTOSHARED_CCA get_PositionQuantityType(int *value) = 0;

    //! Продолжает прерванную печать последнего документа
    /*!
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ContinuePrint() = 0;

    //! Устанавливает дату и время в ККТ.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>Date        <td>Дата            <td>put_Date()
        <tr><td>Time        <td>Время           <td>put_Time()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA SetDateTime() = 0;

    //! Возвращает значение заряда аккумулятора в процентах.
    /*!
       \param value Процент заряда аккумулятора
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_BatteryCharge(int *value) = 0;

    //! Устанавливает флаг передачи только ставки налога.
    /*!
       Используется для отключения передачи суммы налога на позицию в ФН.

       \param value 0 - не расчитывать, 1 - расчитывать
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_UseOnlyTaxNumber()
     */
    virtual int DTOSHARED_CCA put_UseOnlyTaxNumber(int value) = 0;

   //! Возвращает флаг передачи только ставки налога.
   /*!
       Используется для отключения передачи суммы налога на позицию в ФН.

      \param value 0 - не расчитывать, 1 - расчитывать
      \retval 0 Успех
      \sa put_UseOnlyTaxNumber()
    */
    virtual int DTOSHARED_CCA get_UseOnlyTaxNumber(int *value) = 0;

    //! Записывает в ККТ текстовый атрибут чека для дальнейшей его печати.
    /*!
       Метод предназначен для модификации шаблона чека.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CheckAttributeNumber<td>Номер атрибута чека                         <td>put_CheckAttributeNumber()
        <tr><td>Caption             <td>Текст атрибута                              <td>put_Caption()
        <tr><td>Alignment           <td>Выравнивание строки (TED::Fptr::Alignment)  <td>put_Alignment()
        <tr><td>TextWrap            <td>Перенос строки (TED::Fptr::TextWrap)        <td>put_TextWrap()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddTextAttribute() = 0;

    //! Записывает в ККТ текстовый атрибут чека с форматированием для дальнейшей его печати.
    /*!
       Метод предназначен для модификации шаблона чека.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>CheckAttributeNumber<td>Номер атрибута чека                         <td>put_CheckAttributeNumber()
        <tr><td>Caption             <td>Текст атрибута                              <td>put_Caption()
        <tr><td>Alignment           <td>Выравнивание строки (TED::Fptr::Alignment)  <td>put_Alignment()
        <tr><td>TextWrap            <td>Перенос строки (TED::Fptr::TextWrap)        <td>put_TextWrap()
        <tr><td>ReceiptFont         <td>Шрифт ЧЛ (TED::Fptr::Font)                  <td>put_ReceiptFont()
        <tr><td>ReceiptLinespacing  <td>Межстрочный интервал на ЧЛ                  <td>put_ReceiptLinespacing()
        <tr><td>ReceiptBrightness   <td>Яркость текста на ЧЛ                        <td>put_ReceiptBrightness()
        <tr><td>FontDblWidth        <td>Двойная ширина                              <td>put_FontDblWidth()
        <tr><td>FontDblHeight       <td>Двойная высота                              <td>put_FontDblHeight()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA AddFormattedTextAttribute() = 0;

    //! Устанавливает номер атрибута чека.
    /*!
       \param value Номер атрибута чека.
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CheckAttributeNumber()
     */
    virtual int DTOSHARED_CCA put_CheckAttributeNumber(int value) = 0;

   //! Возвращает номер атрибута чека.
   /*!
      \param value Номер атрибута чека.
      \retval 0 Успех
      \sa put_CheckAttributeNumber()
    */
    virtual int DTOSHARED_CCA get_CheckAttributeNumber(int *value) = 0;

    //! Возвращает верхнюю границу диапазона времени.
    /*!
       \param hours Часы
       \param minutes Минуты
       \param seconds Секунды
       \retval 0 Успех
       \sa put_TimeEnd()
     */
    virtual int DTOSHARED_CCA get_TimeEnd(int *hours, int *minutes, int *seconds) = 0;

    //! Устанавливает верхнюю границу диапазона времени.
    /*!
       \param hours Часы
       \param minutes Минуты
       \param seconds Секунды
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TimeEnd()
     */
    virtual int DTOSHARED_CCA put_TimeEnd(int hours, int minutes, int seconds) = 0;

    //! Устанавливает тип системной операции
    /*!
       \param value Тип системной операции
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_SystemOperationType(int value) = 0;

    //! Устанавливает параметры системной операции
    /*!
       \param bfr Параметры системной операции
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA put_SystemOperationData(const wchar_t *bfr) = 0;

    //! Получает результат системной операции
    /*!
       \param bfr Буфер
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_SystemOperationResult(wchar_t *bfr, int size) = 0;

    //! Выполняет системную операцию
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>SystemOperationType     <td>Тип системной операции                  <td>put_SystemOperationType()
        <tr><td>SystemOperationData     <td>Параметры системной операции            <td>put_SystemOperationData()
        <tr><th colspan=3>Выходные свойства
        <tr><td>SystemOperationResult   <td>Результат системной операции            <td>get_SystemOperationResult()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ExecSystemOperation() = 0;

    virtual int DTOSHARED_CCA put_JournalDataType(int value) = 0;
    virtual int DTOSHARED_CCA get_JournalDataType(int *value) = 0;
    virtual int DTOSHARED_CCA put_JournalAttributesType(int value) = 0;
    virtual int DTOSHARED_CCA get_JournalAttributesType(int *value) = 0;
    virtual int DTOSHARED_CCA get_JournalDocumentType(int *value) = 0;
    virtual int DTOSHARED_CCA GetJournalStatus() = 0;

protected:
    IFptr()
    {
    }
    ~IFptr()
    {
    }

private:
    IFptr(const IFptr&);
    IFptr& operator=(const IFptr&);
};

} // namespace Fptr
} // namespace TED


#endif /* IFPTR_H */
