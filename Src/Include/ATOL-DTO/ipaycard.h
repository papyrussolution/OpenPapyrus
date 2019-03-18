/** @file */

#ifndef PAYCARD_IPAYCARD_H
#define PAYCARD_IPAYCARD_H

#include "idtobase.h"

namespace TED
{

//! Драйвер платежных систем (дПС)
namespace PayCard
{

//! Версия интерфейса драйвера платежных систем
#define DTO_IPAYCARD_VER1         3

//! Идентификаторы платежных систем
/*!
   Константы для put_DeviceSettings() и put_DeviceSingleSettings()
 */
enum PaySystemID
{
    PaySystemInvalid = -1,
    //! INPAS SmartSale
    PaySystemINPAS = 0,
    //! ОСМП: приём платежей
    PaySystemOSMP,
    //! CyberPlat: приём платежей
    PaySystemCyberPlat,
    //! e-port: приём платежей
    PaySystemEPort,
    //! Сбербанк
    PaySystemSberbank,
    //! Эрлайн
    PaySystemRLine,
    //! ARCOM ARCUS v.2
    PaySystemArcomARCUS2,
    //! Газпромбанк
    PaySystemGazprombank,
    //! INPAS SmartSale FPrintPay
    PaySystemINPASFPrintPay,
    //! ARCOM ARCUS v.2 FPrintPay
    PaySystemArcomARCUS2FPrintPay,
    //! Сберабанк FPrintPay
    PaySystemSberbankFPrintPay,
    //! Сберабанк (АТОЛ)
    PaySystemSberbankATOL,
    //! INPAS SmartSale (АТОЛ)
    PaySystemINPASATOL,
    //! ARCOM ARCUS v.2 (АТОЛ)
    PaySystemArcomARCUS2ATOL,
    //! ICMP
    PaySystemICMP,
    //! UCS
    PaySystemUCS,
};

//! Тип операции
enum OperationType
{
    OperationTypeInvalid = -1,
    //! Оплата
    OperationTypeSub = 0,
    //! Возврат
    OperationTypeAdd,
    //! Отмена оплаты
    OperationTypeCancelSub,
    //! Отмена возврата
    OperationTypeCancelAdd,
    //! Активация БЕЗ суммы
    OperationTypeAct,
    //! Активация С суммой
    OperationTypeActSum,
    //! Бонусы и скидка
    OperationTypeBonusDisc,
    //! Внесение предоплаты
    OperationTypePrePay,
    //! Просмотр баланса
    OperationTypeBalance,
    //! Оплата услуг
    OperationTypeServicePayment,
    //! Оплата услуг картой
    OperationTypeCardServicePayment
};

//! Тип служебной операции
enum ServiceOperationType
{
    ServiceOperationTypeInvalid = -1,
    //! Обновление авторизационного идентификатора
    ServiceOperationTypeAuthTokenUpdate = 0,
    //! Загрузка настроек
    ServiceOperationTypeDownloadSettings,
    //! Загрузка ПО
    ServiceOperationTypeDownloadSoftware,
    //! Загрузка настроек и ПО
    ServiceOperationTypeDownloadAll,
    //! Вызов меню кассира
    ServiceOperationTypeOperatorMenu,
    //! Вызов меню администратора
    ServiceOperationTypeAdminMenu
};

//! Тип отчета
enum ReportType
{
    ReportTypeInvalid = -1,
    //! Закрытие смены
    ReportTypeCloseSession = 0,
    //! Журнал операций
    ReportTypeJournalSession,
    //! Итоги операций
    ReportTypeTotalSession,
    //! Краткий отчет
    ReportTypeBrief,
    //! Полный отчет
    ReportTypeFull,
    //! Запрос чека по номеру
    ReportTypeQueryCheque
};

//! Тип транзакции
enum TransType
{
    TransTypeInvalid = -1,
    //! Продажа
    TransTypeSale = 0,
    //! Подготовка к авторизации
    TransTypePreAuth,
    //! Прием наличных
    TransTypeCashAdvance,
    //! Не используется
    TransTypeUnUsed1,
    //! Возврат
    TransTypeRefund,
    //! Не используется
    TransTypeUnUsed2,
    //! Продажа офлайн
    TransTypeOffLineSale,
    //! Не используется
    TransTypeUnUsed3,
    //! Не используется
    TransTypeUnUsed4,
    //! Аннулирование
    TransTypeVoid,
    //! Отмена продажи
    TransTypeReversalSale,
    //! Отмена возврата
    TransTypeReversalRefund,
    //! Просмотр баланса
    TransTypeBalance,
    //! Оплата услуг
    TransTypeServicePayment
};

//! Тип авторизации
enum AuthorizationType
{
    AuthorizationTypeInvalid = -1,
    //! Ридером по подписи
    AuthorizationTypeReaderSign = 0,
    //! Вручную по подписи
    AuthorizationTypeManualSign,
    //! Ридером по PIN
    AuthorizationTypeReaderPIN,
    //! Вручную по PIN
    AuthorizationTypeManualPIN,
    //! Чиповая карта
    AuthorizationTypeChip
};

//! Дополнительные параметры
/*!
   Константы для put_DeviceSettings() и put_DeviceSingleSettings()
 */
enum AddParam
{
    AddParamInvalid = -1,
    //! Нет
    AddParamNone = 0,
    //! Номер счета
    AddParamAccount,
    //! Номер телефона
    AddParamPhone,
    //! Логин
    AddParamLogin,
    //! Код
    AddParamCode
};

//! Тип комиссии
/*!
   Константы для put_DeviceSettings() и put_DeviceSingleSettings()
 */
enum CommissionType
{
    CommissionTypeInvalid = -1,
    //! Сумма
    CommissionTypeSum = 0,
    //! Процент
    CommissionTypePercent
};

//! Способ начисления комиссии
/*!
   Константы для put_DeviceSettings() и put_DeviceSingleSettings()
 */
enum CommissionAddition
{
    CommissionAdditionInvalid = -1,
    //! Включена в сумму
    CommissionAdditionIncluded = 0,
    //! Добавляется к сумме
    CommissionAdditionExcluded,
    //! Запрос у оператора
    CommissionAdditionAsk
};

//! Способ связи с сервером авторизации
/*!
   Константы для put_DeviceSettings() и put_DeviceSingleSettings()
 */
enum ModemMode
{
    //! Средствами ПК/смартфона/планшета
    ModemModeTcp = 0,
    //! Средствами ККТ
    ModemModeEcr = 1
};

class IPayCard;

}
}

//! Создает экземпляр драйвера платежных систем.
/*!
   При несовпадении версии ver и актуальной будет возвращаться 0, а в лог записываться соответствующее сообщение.
   \param ver Версия драйвера (DTO_IPAYCARD_VER1)
   \return Указатель на экземпляр драйвера платежных систем
   \retval 0 Ошибка
 */
extern "C" DTOSHARED_EXPORT TED::PayCard::IPayCard * DTOSHARED_CCA CreatePayCardInterface(int ver);

//! Разрушает экземпляр драйвера платежных систем.
/*!
   \param iface Экземпляр драйвера платежных систем
 */
extern "C" DTOSHARED_EXPORT void DTOSHARED_CCA ReleasePayCardInterface(TED::PayCard::IPayCard **iface);

//! Тип функции CreatePayCardInterface().
typedef TED::PayCard::IPayCard*(*CreatePayCardInterfacePtr)(int ver);
//! Тип функции ReleasePayCardInterface().
typedef void (*ReleasePayCardInterfacePtr)(TED::PayCard::IPayCard **iface);



namespace TED
{

namespace PayCard
{

//! Интерфейс драйвера платежных систем
class IPayCard : public IDTOBase
{
public:
    friend DTOSHARED_EXPORT void DTOSHARED_CCA::ReleasePayCardInterface(IPayCard **iface);
    typedef ReleasePayCardInterfacePtr ReleaseFunction;

    //! Возвращает указатель на функцию удаления интерфейса.
    /*!
       \return Указатель на функцию удаления интерфейса
     */
    virtual ReleaseFunction DTOSHARED_CCA get_ReleaseFunction() = 0;

    //! Сбрасывает состояние драйвера.
    /*!
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Reset() = 0;

    //! Выполняет сервисную операцию.
    /*!
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ServiceOperationType    <td>Тип сервисной операции (TED::PayCard::ServiceOperationType) <td>put_ServiceOperationType()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Execute() = 0;

    //! Выполняет подготовку к авторизации.
    /*!
       Метод выполняет подготовку к авторизации. Проверяет корректность входных параметров.
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>AuthorizationType       <td>Тип авторизации (TED::PayCard::AuthorizationType)   <td>put_AuthorizationType()
        <tr><td>OperationType           <td>Тип операции (TED::PayCard::OperationType)          <td>put_OperationType()
        <tr><td>Sum                     <td>Сумма операции      <td>put_Sum()
        <tr><td>Currency                <td>Код валюты          <td>put_Currency()
        <tr><td>ReferenceNumber         <td>Ссылочный номер     <td>put_ReferenceNumber()
        <tr><td>AuthCode                <td>Код авторизации     <td>get_AuthCode()
        <tr><td>CharLineLength          <td>Ширина слипа        <td>put_CharLineLength()
        <tr><td>Account                 <td>Номер счета         <td>put_Account()
        <tr><td>PhoneNumber             <td>Номер телефона      <td>put_PhoneNumber()
        <tr><td>DataTrack2              <td>Вторая дорожка карты<td>put_DataTrack2()
        <tr><td>ECRReceiptNumber        <td>Номер чека          <td>put_ECRReceiptNumber()
        <tr><th colspan=3>Выходные свойства
        <tr><td>Mask                    <td>Маска ожидаемого параметра  <td>get_Mask()
       </table>

       <table>
        <caption>Поддерживаемые типы операций</caption>
        <tr><th rowspan=2>Протокол  <th rowspan=2>Название  <th colspan=11>OperationType
        <tr>                                                <td>0<td>1<td>2<td>3<td>4<td>5<td>6<td>7<td>8<td>9<td>10
        <tr><td>0       <td>INPAS SmartSale                 <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>2       <td>CyberPlat: приём платежей       <td> <td> <td> <td> <td> <td> <td> <td> <td> <td>+<td>
        <tr><td>4       <td>Сбербанк                        <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>6       <td>Arcom ARCUS v.2                 <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>7       <td>Газпромбанк                     <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>8       <td>INPAS SmartSale (FPrintPay)     <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>9       <td>Arcom ARCUS v.2 (FPrintPay)     <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>10      <td>Сбербанк (FPrintPay)            <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>11      <td>Сбербанк (АТОЛ)                 <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>12      <td>INPAS SmartSale (АТОЛ)          <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
        <tr><td>13      <td>Arcom ARCUS v.2 (АТОЛ)          <td>+<td>+<td>+<td>+<td> <td> <td> <td> <td> <td> <td>
       </table>
       Метод может заполнить свойство ResultCode (помимо ошибок) кодами:
       - EC_PAYCARD_NEED_PHONE_NUMBER (необходим номер телефона)
       - EC_PAYCARD_NEED_ACCOUNT (необходим номер счета)
       - EC_PAYCARD_NEED_REFERENCE_NUMBER (необходим РРН)
       - EC_PAYCARD_NEED_AUTH_CODE (необходим код авторизации)

       Получение данных результов говорит о том, что незаполнены соответствующие обязательные параметры авторизации.
       Также возвращается маска для ввода в свойстве \a Mask.
       \warning Необходимые входные параметры сильно зависят от типа используемого АС, способа авторизации и т.д.
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA PrepareAuthorization() = 0;

    //! Выполняет авторизацию.
    /*!
       Выполняет авторизацию с параметрами, заданными для PrepareAuthorization().
       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>ResponceCode        <td>Маска ожидаемого параметра  <td>get_ResponseCode()
        <tr><td>Text                <td>Слип                        <td>get_Text()
        <tr><td>ReferenceNumber     <td>Ссылочный номер             <td>get_ReferenceNumber()
        <tr><td>AuthCode            <td>Код авторизации             <td>get_AuthCode()
        <tr><td>CardType            <td>Тип карты                   <td>get_CardType()
        <tr><td>CardNumber          <td>Номер карты                 <td>get_CardNumber()
        <tr><td>CardExpDate         <td>Срок дейтсвия карты         <td>get_CardExpDate()
        <tr><td>TransDate           <td>Дата транзакции (ДД.ММ.ГГГГ)<td>get_TransDate()
        <tr><td>TransTime           <td>Время транзакции (ЧЧ:ММ:СС) <td>get_TransTime()
        <tr><td>TransID             <td>Идентификатор транзакции    <td>get_TransID()
        <tr><td>TerminalID          <td>Идентификатор терминала     <td>get_TerminalID()
        <tr><td>SlipNumber          <td>Номер слипа                 <td>get_SlipNumber()
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA Authorization() = 0;

    //! Начинает формирование отчета.
    /*!
       Данный метод относится к группе блочных методов, т.е. используется только в сочетании с другими методами.
       Для формирования отчета необходимо передать накопленные за банковскую смену данные о финансовых операциях.
       Для этого служат следующие методы:
       - BeginReport() – начать отчет;
       - AddToReport() – добавить данные в отчет;
       - EndReport() – завершить отчет и сформировать результат.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReportType          <td>Тип отчета (TED::PayCard::ReportType)   <td>put_ReportType()
        <tr><td>CharLineLength      <td>Ширина слипа                            <td>put_CharLineLength()
       </table>

       <table>
        <caption>Поддерживаемые отчеты</caption>
        <tr><th rowspan=2>Протокол  <th rowspan=2>Название                                  <th colspan=11>ReportType
        <tr>                                                                                <td>0<td>1<td>2<td>3<td>4<td>5
        <tr><td>TED::PayCard::PaySystemINPAS                <td>INPAS SmartSale             <td>+<td> <td>+<td> <td> <td>
        <tr><td>TED::PayCard::PaySystemCyberPlat            <td>CyberPlat: приём платежей   <td> <td> <td> <td> <td> <td>
        <tr><td>TED::PayCard::PaySystemSberbank             <td>Сбербанк                    <td>+<td> <td> <td> <td> <td>
        <tr><td>TED::PayCard::PaySystemArcomARCUS2          <td>Arcom ARCUS v.2             <td>+<td>+<td>+<td> <td> <td>
        <tr><td>TED::PayCard::PaySystemGazprombank          <td>Газпромбанк                 <td>+<td>+<td> <td>+<td>+<td>
        <tr><td>TED::PayCard::PaySystemINPASFPrintPay       <td>INPAS SmartSale (FPrintPay) <td>+<td> <td>+<td> <td> <td>
        <tr><td>TED::PayCard::PaySystemArcomARCUS2FPrintPay <td>Arcom ARCUS v.2 (FPrintPay) <td>+<td>+<td>+<td> <td> <td>
        <tr><td>TED::PayCard::PaySystemSberbankFPrintPay    <td>Сбербанк (FPrintPay)        <td>+<td>+<td> <td> <td> <td>
        <tr><td>TED::PayCard::PaySystemSberbankATOL         <td>Сбербанк (АТОЛ)             <td>+<td>+<td> <td> <td> <td>
        <tr><td>TED::PayCard::PaySystemINPASATOL            <td>INPAS SmartSale (АТОЛ)      <td>+<td> <td>+<td> <td> <td>
        <tr><td>TED::PayCard::PaySystemArcomARCUS2ATOL      <td>Arcom ARCUS v.2 (АТОЛ)      <td>+<td>+<td>+<td> <td> <td>
       </table>
       \retval -1 Ошибка
       \retval 0 Успех
       \sa put_ReportType()
       \sa put_CharLineLength()
       \sa AddToReport()
       \sa EndReport()
     */
    virtual int DTOSHARED_CCA BeginReport() = 0;

    //! Добавляет данные в отчет.
    /*!
       Данный метод относится к группе блочных методов, т.е. используется только в сочетании с другими методами.
       Для формирования отчета необходимо передать накопленные за банковскую смену данные о финансовых операциях.
       Для этого служат следующие методы:
       - BeginReport() – начать отчет;
       - AddToReport() – добавить данные в отчет;
       - EndReport() – завершить отчет и сформировать результат.

       Если в процессе добавления данных в отчет, т.е. до вызова метода EndReport(),
       необходимо по какой-либо причине прервать данный процесс,
       то можно вызвать метод ResetState().

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Входные свойства
        <tr><td>ReportAuthCode          <td>Код авторизации             <td>put_ReportAuthCode()
        <tr><td>ReportCardExpDate       <td>Срок действия карты         <td>put_ReportCardExpDate()
        <tr><td>ReportCardNumber        <td>Номер карты                 <td>put_ReportCardNumber()
        <tr><td>ReportMsgNumber         <td>Номер сообщений             <td>put_ReportMsgNumber()
        <tr><td>ReportOperationType     <td>Тип операции                <td>put_ReportOperationType()
        <tr><td>ReportReferenceNumber   <td>Ссылочный номер             <td>put_ReportReferenceNumber()
        <tr><td>ReportResponseCode      <td>Код ответа                  <td>put_ReportResponseCode()
        <tr><td>ReportSlipNumber        <td>Номер слипа                 <td>put_ReportSlipNumber()
        <tr><td>ReportSum               <td>Сумма операции              <td>put_ReportSum()
        <tr><td>ReportTerminalID        <td>Идентификатор терминала     <td>put_ReportTerminalID()
        <tr><td>ReportTransDate         <td>Дата транзакции (ДД.ММ.ГГГГ)<td>put_ReportTransDate()
        <tr><td>ReportTransTime         <td>Время транзакции (ЧЧ:ММ:СС) <td>put_ReportTransTime()
        <tr><td>ReportTransType         <td>Тип транзакции              <td>put_ReportTransType()
       </table>

       \warning В текущей версии работает только для \a ReportType = TED::PayCard::ReportTypeTotalSession.
       В остальных случаях требуется сразу вызывать EndReport().
       \retval -1 Ошибка
       \retval 0 Успех
       \sa BeginReport()
       \sa EndReport()
     */
    virtual int DTOSHARED_CCA AddToReport() = 0;

    //! Завершает формирование отчета.
    /*!
       Данный метод относится к группе блочных методов, т.е. используется только в сочетании с другими методами.
       Для формирования отчета необходимо передать накопленные за банковскую смену данные о финансовых операциях.
       Для этого служат следующие методы:
       - BeginReport() – начать отчет;
       - AddToReport() – добавить данные в отчет;
       - EndReport() – завершить отчет и сформировать результат.

       <table>
        <caption>Свойства</caption>
        <tr><th>Название<th>Описание<th>Доступ
        <tr><th colspan=3>Выходные свойства
        <tr><td>ResponseCode    <td>Код ответа      <td>get_ResponseCode()
        <tr><td>Text            <td>Слип            <td>get_Text()
       </table>

       \retval -1 Ошибка
       \retval 0 Успех
       \sa BeginReport()
       \sa AddToReport()
     */
    virtual int DTOSHARED_CCA EndReport() = 0;

    //! Прерывает блочную операцию.
    /*!
       Позволяет прервать начатую блочную операцию (BeginReport(), AddToReport(), EndReport()).
       \warning Метод всегда завершается успешно (\a ResultCode = 0), независимо от того, начата какая-либо блочная операция, или нет.
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA ResetState() = 0;

    //! Возвращает номер точки оплаты.
    /*!
       \param value Номер точки оплаты
       \sa put_TerminalID()
     */
    virtual int DTOSHARED_CCA get_TerminalNumber(int *value) = 0;

    //! Устанавливает номер точки оплаты.
    /*!
       \param value Номер точки оплаты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_TerminalNumber()
     */
    virtual int DTOSHARED_CCA put_TerminalNumber(int value) = 0;

    //! Возвращает тип операции.
    /*!
       \param value Тип операции
       \retval 0 Успех
       \sa put_OperationType()
       \sa OperationType
     */
    virtual int DTOSHARED_CCA get_OperationType(int *value) = 0;

    //! Устанавливает тип операции.
    /*!
       \param value Тип операции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_OperationType()
       \sa OperationType
     */
    virtual int DTOSHARED_CCA put_OperationType(int value) = 0;

    //! Возвращает тип сервисной операции.
    /*!
       \param value Тип сервисной операции
       \retval 0 Успех
       \sa put_ServiceOperationType()
       \sa ServiceOperationType
     */
    virtual int DTOSHARED_CCA get_ServiceOperationType(int *value) = 0;

    //! Устанавливает тип сервисной операции.
    /*!
       \param value Тип сервисной операции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ServiceOperationType()
       \sa ServiceOperationType
     */
    virtual int DTOSHARED_CCA put_ServiceOperationType(int value) = 0;

    //! Возвращает тип авторизации.
    /*!
       \param value Тип авторизации
       \retval 0 Успех
       \sa put_AuthorizationType()
       \sa AuthorizationType
     */
    virtual int DTOSHARED_CCA get_AuthorizationType(int *value) = 0;

    //! Устанавливает тип авторизации.
    /*!
       \param value Тип авторизации
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_AuthorizationType()
       \sa AuthorizationType
     */
    virtual int DTOSHARED_CCA put_AuthorizationType(int value) = 0;

    //! Возвращает сумму.
    /*!
       \param value Сумма
       \retval 0 Успех
       \sa put_Sum()
     */
    virtual int DTOSHARED_CCA get_Sum(double *value) = 0;

    //! Устанавливает сумму.
    /*!
       \param value Сумма
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Sum()
     */
    virtual int DTOSHARED_CCA put_Sum(double value) = 0;

    //! Возвращает название.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для номера названия
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_Name()
     */
    virtual int DTOSHARED_CCA get_Name(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает название.
    /*!
       \warning Не используется в текущей версии
       \param value Название
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Name()
     */
    virtual int DTOSHARED_CCA put_Name(const wchar_t *value) = 0;

    //! Возвращает номер телефона.
    /*!
       \param bfr Буфер для номера телефона
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_PhoneNumber()
     */
    virtual int DTOSHARED_CCA get_PhoneNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает номер телефона.
    /*!
       \param value Номер телефона
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_PhoneNumber()
     */
    virtual int DTOSHARED_CCA put_PhoneNumber(const wchar_t *value) = 0;

    //! Возвращает номер карты.
    /*!
       \param bfr Буфер для номера карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CardNumber()
     */
    virtual int DTOSHARED_CCA get_CardNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает номер карты.
    /*!
       \param value Номер телефона
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CardNumber()
     */
    virtual int DTOSHARED_CCA put_CardNumber(const wchar_t *value) = 0;

    //! Возвращает тип карты.
    /*!
       \param bfr Буфер для типа карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_CardType(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает срок действия карты.
    /*!
       \param bfr Буфер для срока действия карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CardExpDate()
     */
    virtual int DTOSHARED_CCA get_CardExpDate(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает срок действия карты.
    /*!
       \param value Срок действия карты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CardExpDate()
     */
    virtual int DTOSHARED_CCA put_CardExpDate(const wchar_t *value) = 0;

    //! Возвращает владельца карты.
    /*!
       \param bfr Буфер для владельца карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CardHolderName()
     */
    virtual int DTOSHARED_CCA get_CardHolderName(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает владельца карты.
    /*!
       \param value Номер телефона
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CardHolderName()
     */
    virtual int DTOSHARED_CCA put_CardHolderName(const wchar_t *value) = 0;

    //! Возвращает дорожки карты.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для дорожек карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_DataTracks()
     */
    virtual int DTOSHARED_CCA get_DataTracks(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает дорожки карты.
    /*!
       \warning Не используется в текущей версии
       \param value Дорожки карты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DataTracks()
     */
    virtual int DTOSHARED_CCA put_DataTracks(const wchar_t *value) = 0;

    //! Возвращает вторую дорожку карты.
    /*!
       \param bfr Буфер для второй дорожки карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_DataTrack2()
     */
    virtual int DTOSHARED_CCA get_DataTrack2(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает вторую дорожку карты.
    /*!
       \param value Вторая дорожка карты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_DataTrack2()
     */
    virtual int DTOSHARED_CCA put_DataTrack2(const wchar_t *value) = 0;

    //! Возвращает ссылочный номер.
    /*!
       \param bfr Буфер для ссылочного номера
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReferenceNumber()
     */
    virtual int DTOSHARED_CCA get_ReferenceNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает ссылочный номер.
    /*!
       \param value Ссылочный номер
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReferenceNumber()
     */
    virtual int DTOSHARED_CCA put_ReferenceNumber(const wchar_t *value) = 0;

    //! Возвращает ширину строки.
    /*!
       \param value Ширина строки
       \retval 0 Успех
       \sa put_CharLineLength()
     */
    virtual int DTOSHARED_CCA get_CharLineLength(int *value) = 0;

    //! Устанавливает ширину строки.
    /*!
       \param value Ширина строки
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CharLineLength()
     */
    virtual int DTOSHARED_CCA put_CharLineLength(int value) = 0;

    //! Возвращает номер смены ККТ.
    /*!
       \warning Не используется в текущей версии
       \param value Номер смены ККТ
       \retval 0 Успех
       \sa put_ECRSessionNumber()
     */
    virtual int DTOSHARED_CCA get_ECRSessionNumber(int *value) = 0;

    //! Устанавливает номер смены ККТ.
    /*!
       \warning Не используется в текущей версии
       \param value Номер смены ККТ
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ECRSessionNumber()
     */
    virtual int DTOSHARED_CCA put_ECRSessionNumber(int value) = 0;

    //! Возвращает номер чека ККТ.
    /*!
       \param value Номер чека ККТ
       \retval 0 Успех
       \sa put_ECRReceiptNumber()
     */
    virtual int DTOSHARED_CCA get_ECRReceiptNumber(int *value) = 0;

    //! Устанавливает номер чека ККТ.
    /*!
       \param value Номер чека ККТ
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ECRReceiptNumber()
     */
    virtual int DTOSHARED_CCA put_ECRReceiptNumber(int value) = 0;

    //! Возвращает код валюты.
    /*!
       \param bfr Буфер для кода валюты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_Currency()
     */
    virtual int DTOSHARED_CCA get_Currency(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает код валюты.
    /*!
       \param value Код валюты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Currency()
     */
    virtual int DTOSHARED_CCA put_Currency(const wchar_t *value) = 0;

    //! Возвращает код ответа платежной системы.
    /*!
       \param value Код ответа
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_ResponseCode(int *value) = 0;

    //! Возвращает тип транзакции.
    /*!
       \warning Не используется в текущей версии
       \param value Тип транзакции
       \retval 0 Успех
       \sa TransType
     */
    virtual int DTOSHARED_CCA get_TransType(int *value) = 0;

    //! Возвращает дату транзакции.
    /*!
       \param bfr Буфер для даты транзакции
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_TransDate(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает время транзации.
    /*!
       \param bfr Буфер для времени транзакции
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_TransTime(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает идентификатор транзации.
    /*!
       \param bfr Буфер для идентификатора транзакции
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_TransID(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает код авторизации.
    /*!
       \param bfr Буфер для кода авторизации
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_AuthCode()
     */
    virtual int DTOSHARED_CCA get_AuthCode(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает код авторизации.
    /*!
       \param value Код авторизации
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_AuthCode()
     */
    virtual int DTOSHARED_CCA put_AuthCode(const wchar_t *value) = 0;

    //! Возвращает идентификатор терминала.
    /*!
       \param bfr Буфер для идентификатора терминала
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_TerminalID(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает номер сообщения.
    /*!
       \warning Не используется в текущей версии
       \param value Номер сообщения
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_MsgNumber(int *value) = 0;

    //! Возвращает тип сообщения.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для типа сообщения
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_MessageType(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает номер точки обслуживания.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для номера точки обслуживания
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_MerchNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает категорию точки обслуживания.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для категории точки обслуживания
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_MerchCategoryCode(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает английское название точки обслуживания.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для английского названия точки обслуживания
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_MerchEngName(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает номер слипа.
    /*!
       \warning Не используется в текущей версии
       \param value Номер слипа
       \retval 0 Успех
       \sa put_SlipNumber()
     */
    virtual int DTOSHARED_CCA get_SlipNumber(int *value) = 0;

    //! Устанавливает номер слипа.
    /*!
       \warning Не используется в текущей версии
       \param value Номер слипа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_SlipNumber()
     */
    virtual int DTOSHARED_CCA put_SlipNumber(int value) = 0;

    //! Возвращает текст слипа.
    /*!
       \param bfr Буфер для текста слипа
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_Text(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает скидку.
    /*!
       \warning Не используется в текущей версии
       \param value Скидка
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Discount(double *value) = 0;

    //! Возвращает значение бонуса.
    /*!
       \warning Не используется в текущей версии
       \param value Бонус
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Bonus(double *value) = 0;

    //! Возвращает код авторизации (для отчетов).
    /*!
       \param bfr Буфер для кода авторизации
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportAuthCode()
     */
    virtual int DTOSHARED_CCA get_ReportAuthCode(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает код авторизации (для отчетов).
    /*!
       \param value Код авторизации
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportAuthCode()
     */
    virtual int DTOSHARED_CCA put_ReportAuthCode(const wchar_t *value) = 0;

    //! Возвращает тип операции (для отчетов).
    /*!
       \param value Тип операции
       \retval 0 Успех
       \sa put_ReportOperationType()
     */
    virtual int DTOSHARED_CCA get_ReportOperationType(int *value) = 0;

    //! Устанавливает тип операции (для отчетов).
    /*!
       \param value Тип операции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportOperationType()
     */
    virtual int DTOSHARED_CCA put_ReportOperationType(int value) = 0;

    //! Возвращает тип транзакции (для отчетов).
    /*!
       \param value Тип транзакции
       \retval 0 Успех
       \sa put_ReportTransType()
       \sa TransType
     */
    virtual int DTOSHARED_CCA get_ReportTransType(int *value) = 0;

    //! Устанавливает тип транзакции (для отчетов).
    /*!
       \param value Тип транзакции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportTransType()
       \sa TransType
     */
    virtual int DTOSHARED_CCA put_ReportTransType(int value) = 0;

    //! Возвращает сумму (для отчетов).
    /*!
       \param value Сумма
       \retval 0 Успех
       \sa put_ReportSum()
     */
    virtual int DTOSHARED_CCA get_ReportSum(double *value) = 0;

    //! Устанавливает сумму (для отчетов).
    /*!
       \param value Сумма
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportSum()
     */
    virtual int DTOSHARED_CCA put_ReportSum(double value) = 0;

    //! Возвращает номер карты (для отчетов).
    /*!
       \param bfr Буфер для номера карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportCardNumber()
     */
    virtual int DTOSHARED_CCA get_ReportCardNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает номер карты (для отчетов).
    /*!
       \param value Номер карты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportCardNumber()
     */
    virtual int DTOSHARED_CCA put_ReportCardNumber(const wchar_t *value) = 0;

    //! Возвращает срок действия карты (для отчетов).
    /*!
       \param bfr Буфер для срока действия карты
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportCardExpDate()
     */
    virtual int DTOSHARED_CCA get_ReportCardExpDate(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает срок действия карты (для отчетов).
    /*!
       \param value Срок действия карты
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportCardExpDate()
     */
    virtual int DTOSHARED_CCA put_ReportCardExpDate(const wchar_t *value) = 0;

    //! Возвращает номер слипа (для отчетов).
    /*!
       \param value Номер слипа
       \retval 0 Успех
       \sa put_ReportSlipNumber()
     */
    virtual int DTOSHARED_CCA get_ReportSlipNumber(int *value) = 0;

    //! Устанавливает номер слипа (для отчетов).
    /*!
       \param value Номер телефона
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportSlipNumber()
     */
    virtual int DTOSHARED_CCA put_ReportSlipNumber(int value) = 0;

    //! Возвращает дату транзакции (для отчетов).
    /*!
       \param bfr Буфер для даты транзакции
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportTransDate()
     */
    virtual int DTOSHARED_CCA get_ReportTransDate(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает дату транзакции (для отчетов).
    /*!
       \param value Дата транзакции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportTransDate()
     */
    virtual int DTOSHARED_CCA put_ReportTransDate(const wchar_t *value) = 0;

    //! Возвращает время транзакции (для отчетов).
    /*!
       \param bfr Буфер для времени транзакции
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportTransTime()
     */
    virtual int DTOSHARED_CCA get_ReportTransTime(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает время транзакции (для отчетов).
    /*!
       \param value Время транзакции
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportTransTime()
     */
    virtual int DTOSHARED_CCA put_ReportTransTime(const wchar_t *value) = 0;

    //! Возвращает номер сообщения (для отчетов).
    /*!
       \param value Номер сообщения
       \retval 0 Успех
       \sa put_ReportSum()
     */
    virtual int DTOSHARED_CCA get_ReportMsgNumber(int *value) = 0;

    //! Устанавливает номер сообщения (для отчетов).
    /*!
       \param value Номер сообщения
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportMsgNumber()
     */
    virtual int DTOSHARED_CCA put_ReportMsgNumber(int value) = 0;

    //! Возвращает идентификатор терминала (для отчетов).
    /*!
       \param bfr Буфер для идентификатора терминала
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportTerminalID()
     */
    virtual int DTOSHARED_CCA get_ReportTerminalID(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает идентификатор терминала (для отчетов).
    /*!
       \param value Идентификатор терминала
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportTerminalID()
     */
    virtual int DTOSHARED_CCA put_ReportTerminalID(const wchar_t *value) = 0;

    //! Возвращает ссылочный номер (для отчетов).
    /*!
       \param bfr Буфер для ссылочного номера
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportReferenceNumber()
     */
    virtual int DTOSHARED_CCA get_ReportReferenceNumber(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает ссылочный номер (для отчетов).
    /*!
       \param value Ссылочный номер
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportReferenceNumber()
     */
    virtual int DTOSHARED_CCA put_ReportReferenceNumber(const wchar_t *value) = 0;

    //! Возвращает код ответа (для отчетов).
    /*!
       \param bfr Буфер для кода ответа
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_ReportResponseCode()
     */
    virtual int DTOSHARED_CCA get_ReportResponseCode(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает код ответа (для отчетов).
    /*!
       \param value Код ответа
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_ReportResponseCode()
     */
    virtual int DTOSHARED_CCA put_ReportResponseCode(const wchar_t *value) = 0;

    //! Возвращает тип отчета.
    /*!
       \param value Номер сообщения
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

    //! Возвращает штрихкод.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для штрихкода
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_Barcode()
     */
    virtual int DTOSHARED_CCA get_Barcode(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает штрихкод.
    /*!
       \warning Не используется в текущей версии
       \param value Тип отчета
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Barcode()
     */
    virtual int DTOSHARED_CCA put_Barcode(const wchar_t *value) = 0;

    //! Возвращает имя кассира.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для имени
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Cashier(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает имя кассира.
    /*!
       \warning Не используется в текущей версии
       \param value Имя кассира
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Cashier()
     */
    virtual int DTOSHARED_CCA put_Cashier(const wchar_t *value) = 0;

    //! Возвращает номер счета.
    /*!
       \param bfr Буфер для номера счета
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Account(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает номер счета.
    /*!
       \param value Номер счета
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_Account()
     */
    virtual int DTOSHARED_CCA put_Account(const wchar_t *value) = 0;

    //! Возвращает маску для ввода.
    /*!
       \param bfr Буфер для маски
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
     */
    virtual int DTOSHARED_CCA get_Mask(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает значение комиссии.
    /*!
       \param value Комиссия
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_Commission(double *value) = 0;
    virtual int DTOSHARED_CCA get_ServiceOperator(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает идентификатор зачисленной суммы.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для идентификатора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_SumWareCode(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает идентификатор комиссии.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для идентификатора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CommissionWareCode(wchar_t *bfr, int bfrSize) = 0;


    //! Устанавливает номер оператора.
    /*!
       \param value Номер оператора
       \retval -1 Ошибка
       \retval 0 Успех
       \sa get_CurrentOperator()
     */
    virtual int DTOSHARED_CCA put_CurrentOperator(int value) = 0;

    //! Возвращает номер оператора.
    /*!
       \param value Номер оператора
       \retval 0 Успех
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperator(int *value) = 0;

    //! Возвращает название текущего оператора.
    /*!
       \warning Не используется в текущей версии
       \param bfr Буфер для оператора
       \param bfrSize Размер буфера
       \retval 0 Успех
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorName(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает код текущего оператора.
    /*!
       \param bfr Буфер для кода
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \retval 0 Успех
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorCode(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает минимальную сумму для текущего оператора.
    /*!
       \param value Минимальная сумма
       \retval 0 Успех
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorMinSum(double *value) = 0;

    //! Возвращает максимальную сумму для текущего оператора.
    /*!
       \param value Максимальная сумма
       \retval 0 Успех
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorMaxSum(double *value) = 0;

    //! Возвращает тип комиссии для текущего оператора.
    /*!
       \param value Тип комиссии
       \retval 0 Успех
       \sa put_CurrentOperator()
       \sa CommissionType
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorCommissionType(int *value) = 0;

    //! Возвращает способ начисления комиссии для текущего оператора.
    /*!
       \param value Способ начисления комиссии
       \retval 0 Успех
       \sa put_CurrentOperator()
       \sa CommissionAddition
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorCommissionAddition(int *value) = 0;

    //! Возвращает значение комиссии для текущего оператора.
    /*!
       \param value Название оператора
       \retval 0 Успех
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorCommission(double *value) = 0;
    virtual int DTOSHARED_CCA get_CurrentOperatorAddReqSum(double *value) = 0;

    //! Возвращает тип первого параметра для текущего оператора.
    /*!
       \param value Тип параметра
       \retval 0 Успех
       \sa put_CurrentOperator()
       \sa get_CurrentOperatorMask1()
       \sa AddParam
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorParam1(int *value) = 0;

    //! Возвращает маску для ввода первого параметра для текущего оператора.
    /*!
       \param bfr Буфер для маски
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
       \sa get_CurrentOperatorParam1()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorMask1(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает тип второго параметра для текущего оператора.
    /*!
       \param value Тип параметра
       \retval 0 Успех
       \sa put_CurrentOperator()
       \sa get_CurrentOperatorMask2()
       \sa AddParam
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorParam2(int *value) = 0;

    //! Возвращает маску для ввода второго параметра для текущего оператора.
    /*!
       \param bfr Буфер для маски
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
       \sa get_CurrentOperatorParam2()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorMask2(wchar_t *bfr, int bfrSize) = 0;
    virtual int DTOSHARED_CCA get_CurrentOperatorCommissionRounding(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает шлюз для текущего оператора.
    /*!
       \param bfr Буфер для шлюза
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorGateway(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает идентификатор зачисленной суммы для текущего оператора.
    /*!
       \param bfr Буфер для идентификатора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorSumWareCode(wchar_t *bfr, int bfrSize) = 0;

    //! Возвращает идентификатор комиссии для текущего оператора.
    /*!
       \param bfr Буфер для идентификатора
       \param bfrSize Размер буфера
       \return Требуемый размер буфера
       \sa put_CurrentOperator()
     */
    virtual int DTOSHARED_CCA get_CurrentOperatorCommissionWareCode(wchar_t *bfr, int bfrSize) = 0;

    //! Устанавливает дескриптор пинпада.
    /*!
       Используется для связи дПС и дККТ в случае работы с пинпадом FPrintPay-01.
       С помощью данного дескриптора будет осуществляться управление пинпадом.

       \warning Дескриптор актуален до следующей перенастройки драйвера ККТ (ApplySingleSettings() или put_DeviceSettings())!

       \param value Дескриптор пинпада
       \retval -1 Ошибка
       \retval 0 Успех
       \sa TED::Fptr::IFptr::get_PinPadDevice
     */
    virtual int DTOSHARED_CCA put_PinPadDevice(void *value) = 0;

    //! Устанавливает дескриптор модема.
    /*!
       Используется для связи дПС и дККТ в случае работы с модемом FPrintPay-01.
       С помощью данного дескриптора будет осуществляться управление модемом.

       \warning Дескриптор актуален до следующей перенастройки драйвера ККТ (ApplySingleSettings() или put_DeviceSettings())!

       \param value Дескриптор модема
       \retval -1 Ошибка
       \retval 0 Успех
       \sa TED::Fptr::IFptr::get_ModemDevice
     */
    virtual int DTOSHARED_CCA put_ModemDevice(void *value) = 0;

protected:
    IPayCard()
    {
    }

    virtual ~IPayCard()
    {
    }

private:
    IPayCard(const IPayCard&);
    IPayCard& operator=(const IPayCard&);
};

}

}

#endif
