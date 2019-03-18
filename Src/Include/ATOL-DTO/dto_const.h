/** @file */

#ifndef DTO_CONST_H
#define DTO_CONST_H

//! Разделитель пары "ключ-значение" (:)
#define DTO_PAIR_DELIMITER      0x3A
#define DTO_PAIR_DELIMITER_STR L"\x3A"
//! Разделитель между парами "ключ-значение" (;)
#define DTO_MAPPING_DELIMITER   0x3B
#define DTO_MAPPING_DELIMITER_STR L"\x3B"
//! Замена разделителя
#define DTO_PAIR_DELIMITER_REPLACE 0x24

#define DTO_DOUBLESIZECHAR      0x09
#define DTO_PICTUREINMEMORY     0x0A
#define DTO_BARCODEINMEMORY     0x0B
#define DTO_DOUBLESIZECHAR_GUI  0xBB
#define DTO_PICTUREINMEMORY_GUI 0xB6
#define DTO_BARCODEINMEMORY_GUI 0xA5

//! \name Ключи свойств
//! @{

//! \name Общие ключи
//! @{
#define S_MODEL                 L"Model"            /*!< Модель устройства    */
#define S_PROTOCOL              L"Protocol"         /*!< Протокол работы      */
#define S_PORT                  L"Port"             /*!< Порт устройства      */
#define S_BAUDRATE              L"BaudRate"         /*!< Скорость порта       */
#define S_BITS                  L"Bits"             /*!< Биты                 */
#define S_PARITY                L"Parity"           /*!< Четность             */
#define S_STOPBITS              L"StopBits"         /*!< Биты останова        */
#define S_FLOWCONTROL           L"FlowControl"      /*!< Управление потоком   */
#define S_DEVICEFILE            L"TTYSuffix"        /*!< Файл устройства      */
#define S_SEARCHDIR             L"SearchDir"        /*!< Каталог поиска
                                                         библиотек            */
#define S_IPADDRESS             L"IPAddress"        /*!< IP-адрес устройства  */
#define S_MACADDRESS            L"MACAddress"       /*!< MAC-адрес устройства */
#define S_IPPORT                L"IPPort"           /*!< Порт устройства      */
#define S_RECONNECTTIMEOUT      L"ReconnectTo"      /*!< Таймаут реконекта    */
#define S_RETRYCOUNT            L"RetryCount"       /*!< Кол-во попыток связи */
#define S_IPBINDADDRESS         L"IPBindAddress"    /*!< Свой адрес           */
#define S_IPBINDPORT            L"IPBindPort"       /*!< Свой порт            */
#define S_QUANTITYPOINT         L"QuanPoint"        /*!< Десятичная точка в
                                                            кол-вах           */
#define S_SUMMPOINT             L"SummPoint"        /*!< Десятичная точка в
                                                            суммах            */
#define S_MODEL_FZ54            L"Model54"          /*!< Модель устройства
                                                            (только ФЗ-54)    */
#define S_MODEL_1C              L"Model1C"          /*!< Модели, доступные в 1С*/
#define S_EXISTED_SERIAL_PORTS  L"ExistedSerial"    /*!< Существующие порты   */
#define S_EXISTED_TTY           L"ExistedTty"       /*!< Существующие tty     */
#define S_DEVICENAME            L"DeviceName"       /*!< Название устройства  */
#define S_BT_AUTOENABLE         L"AutoEnableBluetooth"  /*!< Автовключение BT */
#define S_BT_AUTODISABLE        L"AutoDisableBluetooth" /*!< Автовыключение BT*/
#define S_BT_CONNECTION_TYPE    L"ConnectionType"   /*!< Способ подключения BT*/
#define S_PID                   L"Pid"              /*!< ProductID USB-устройства*/
#define S_VID                   L"Vid"              /*!< VendorID USB-устройства*/
//! @}


//! \name Значения свойства S_PORT
//! @{
#define SV_PORT_NONE            L"NONE"         /*!< Отключен                 */
#define SV_PORT_COM             L"COM"          /*!< COM-порт                 */
#define SV_PORT_KBD             L"KBD"          /*!< Клавиатурный порт (WIN)  */
#define SV_PORT_POSIFLEX_USB    L"POSIFLEX_USB" /*!< POSIFLEX_USB-порт        */
#define SV_PORT_SOFT            L"SOFT"         /*!< Программный порт         */
#define SV_PORT_TCPIP           L"TCPIP"        /*!< TCP/IP порт              */
#define SV_PORT_UDPIP           L"UDPIP"        /*!< UDP/IP порт              */
#define SV_PORT_TTY             L"TTY"          /*!< TTY-устройство           */
#define SV_PORT_BLUETOOTH       L"BLUETOOTH"    /*!< Bluetooth                */
#define SV_PORT_USB             L"USB"          /*!< USB порт                 */
#define SV_PORT_FAKE            L"FAKE"         /*!< Тестовый порт            */
#define SV_PORT_PROTO           L"PROTO"        /*!< Протокол                 */
//! @}


//! \name Значения свойства S_BAUDRATE
//! @{
#define SV_BAUDRATE_1200        L"1200"         /*!< 1200 бод                 */
#define SV_BAUDRATE_2400        L"2400"         /*!< 2400 бод                 */
#define SV_BAUDRATE_4800        L"4800"         /*!< 4800 бод                 */
#define SV_BAUDRATE_9600        L"9600"         /*!< 9600 бод                 */
#define SV_BAUDRATE_14400       L"14400"        /*!< 14400 бод                */
#define SV_BAUDRATE_19200       L"19200"        /*!< 19200 бод                */
#define SV_BAUDRATE_38400       L"38400"        /*!< 38400 бод                */
#define SV_BAUDRATE_57600       L"57600"        /*!< 57600 бод                */
#define SV_BAUDRATE_115200      L"115200"       /*!< 115200 бод               */
#define SV_BAUDRATE_230400      L"230400"       /*!< 230400 бод               */
#define SV_BAUDRATE_460800      L"460800"       /*!< 460800 бод               */
#define SV_BAUDRATE_921600      L"921600"       /*!< 921600 бод               */
//! @}

//! \name Значения свойства S_BITS
//! @{
#define SV_BITS_7               L"7"            /*!< 7 бит                    */
#define SV_BITS_8               L"8"            /*!< 8 бит                    */
//! @}

//! \name Значения свойства S_PARITY
//! @{
#define SV_PARITY_NO            L"0"            /*!< Без четности             */
#define SV_PARITY_ODD           L"1"            /*!< Четный                   */
#define SV_PARITY_EVEN          L"2"            /*!< Нечетный                 */
#define SV_PARITY_MARK          L"3"            /*!< Метка                    */
#define SV_PARITY_SPACE         L"4"            /*!< Пробел                   */
//! @}

//! \name Значения свойства S_STOPBITS
//! @{
#define SV_STOPBITS_1           L"0"            /*!< 1 бит                    */
#define SV_STOPBITS_1_5         L"1"            /*!< 1.5 бита                 */
#define SV_STOPBITS_2           L"2"            /*!< 2 бита                   */
//! @}

//! \name Значения свойства S_FLOWCONTROL
//! @{
#define SV_FLOWCONTROL_OFF      L"0"            /*!< Не управлять             */
#define SV_FLOWCONTROL_RTSCTS   L"1"            /*!< RTS/CTS                  */
#define SV_FLOWCONTROL_DTRDSR   L"2"            /*!< DTR/DSR                  */
//! @}

//! \name Ключи свойств драйвера ККМ
//! @{
#define S_ACCESSPASSWORD        L"AccessPassword"   /*!< Пароль доступа       */
#define S_USERPASSWORD          L"UserPassword"     /*!< Пароль пользователя  */
#define S_OFD_PORT              L"OfdPort"          /*!< Канал обмена с ОФД   */
#define S_USE_JOURNAL           L"UseJournal"       /*!< Хранить чеки в БД    */
//! @}

//! \name Ключи свойств драйвера УВ
//! @{
#define S_DEVICETYPE            L"DeviceType"       /*!< Тип устройства       */
#define S_PFX                   L"Pfx"              /*!< Постфикс             */
#define S_SFX                   L"Sfx"              /*!< Суффикс              */
#define S_SENS                  L"Sens"             /*!< Чувствительность     */
#define S_DELETETRAILING        L"Trailing"         /*!< Символов с конца     */
#define S_DELETELEADING         L"Leading"          /*!< Символов с начала    */
#define S_ENABLEONSTART         L"EnabledOnStart"   /*!< Включать устройство
                                                            при создании      */
//! @}

//! \name Ключи свойств драйвера ДП
//! @{
#define S_LOADFONTS             L"LoadFonts"        /*!< Шрифты               */
#define S_MODE                  L"Mode"             /*!< Режим                */
#define S_FPTRPORT              L"FptrPort"         /*!< Номер порта ККМ      */
//! @}

//! \name Ключи свойств драйвера ПЧ
//! @{
#define S_WAITACK               L"WaitAck"          /*!< Время ожидания ответа*/
#define S_HEADER                L"Header"           /*!< Заголовок документа  */
#define S_FOOTER                L"Footer"           /*!< Концовка документа   */
#define S_CODEPAGE              L"CodePage"         /*!< Кодировка            */
#define S_CHARSET               L"CharSet"          /*!< Символьные наборы    */
#define S_CHARROTATION          L"CharRotation"     /*!< Повороты символов    */
#define S_PIXLINELENGTH         L"PixLineLength"    /*!< Ширина ленты в точках*/
//! @}

//! \name Ключи свойств драйвера ЭВ
//! @{
#define S_DECIMALPOINT          L"DecimalPoint"     /*!< Десятичная точка     */
#define S_LOGICALNUMBER         L"LogicalNumber"    /*!< Логический номер     */
//! @}

//! \name Ключи свойств драйвера ПС
//! @{
#define S_PATHAC                L"PathAC"           /*!< Каталог обмена с АС  */
#define S_PATHDB                L"PathDB"           /*!< Путь к локальной БД  */
#define S_TIMEOUT               L"Timeout"          /*!< Таймаут ответа от АС */
#define S_CHECKTIMEOUT          L"CheckTimeout"     /*!< Таймаут проверка
                                                            статуса           */
#define S_DEALERCODE            L"DealerCode"       /*!< Код дилера           */
#define S_KEYLOADPWD            L"KeyLoadPwd"       /*!< Пароль закрытого
                                                            ключа             */
#define S_USEPROXY              L"UseProxy"         /*!< Флаг использования
                                                            прокси            */
#define S_URL                   L"URL"              /*!< Адрес сервера        */
#define S_OPERATORCODE          L"OperatorCode"     /*!< Код оператора        */
#define S_OPERATORS             L"Operators"        /*!< Список операторов    */
#define S_POINTCODE             L"PointCode"        /*!< Код точки приема     */
#define S_PROXYSERVER           L"ProxyServer"      /*!< URL прокси-сервера   */
#define S_PROXYPORT             L"ProxyPort"        /*!< Порт прокси-сервера  */
#define S_SERIALKEYNUMBER       L"SerialKeyNumber"  /*!< Серийный номер
                                                            открытого ключа   */
#define S_OPCODECLOSEDAY        L"OpcodeCloseDay"   /*!< Код операции закрытия
                                                            смены             */
#define S_OPCODEPRINTJOURNAL    L"OpcodePrintJournal" /*!< Код операции печати
                                                            журнала           */

#define S_TERMINALID            L"TerminalID"       /*!< ID терминала         */
#define S_MODEMMODE             L"ModemMode"        /*!< Способ связи с
                                                         сервером авторизации */

#define S_SLIP                  L"Slip"             /*!< Настройки слипа      */
#define S_HIDECARD              L"HideCard"         /*!< Флаг сокрытия номера
                                                            карты             */

#define S_OPERATOR              L"Operator"
#define S_ADDREQSUM             L"AddReqSum"        /*!< Таймаут ответа от АС */
#define S_CODE                  L"Code"             /*!< Код                  */
#define S_COMMISSION            L"Comission"        /*!< Комиссия             */
#define S_COMMISSIONADDITION    L"ComissionAddition"/*!< Способ начисления
                                                            комиссии          */
#define S_COMMISSIONTYPE        L"ComissionType"    /*!< Тип комиссии         */
#define S_COMMISSIONWARECODE    L"ComissionWareCode"/*!< Идентификатор
                                                            комиссии          */
#define S_COMMISSIONROUNDING    L"ComissionRounding"/*!< Способ округления
                                                            комиссии          */
#define S_GATEWAY               L"Gateway"          /*!< Шлюз                 */
#define S_MASK1                 L"Mask1"            /*!< Маска параметра №2   */
#define S_MASK2                 L"Mask2"            /*!< Маска параметра №1   */
#define S_MAXSUM                L"MaxSum"           /*!< Максимальная сумма   */
#define S_MINSUM                L"MinSum"           /*!< Минимальная сумма    */
#define S_NAME                  L"Name"             /*!< Название             */
#define S_PARAM1                L"Param1"           /*!< Параметр №2          */
#define S_PARAM2                L"Param2"           /*!< Параметр №1          */
#define S_SUMWARECODE           L"SumWareCode"      /*!< Идентификатор
                                                            зачисленной суммы */
//! @}

//! @}

#endif // DTO_CONST_H
