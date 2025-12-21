// PPDEFS.H
// Copyright (c) Sobolev A. 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Файл содержит только define-определения. Используется, кроме прочего, для включения в исходные файлы ресурсов
//
#ifndef __PPDEFS_H
#define __PPDEFS_H
#ifndef __PPMENU_H
	#include <ppmenu.h>
#endif
#ifndef RC_INVOKED
	#ifndef __SLIB_H
		#include <slib.h>
	#endif
#endif
#include <ppbrow.h> // @autogen
#include <pphelp.h> // @autogen
//
// Navigation:
//   LastObject
//   LastDialog
//   LastBrowser
//   LastError
//   LastText
//   LastToolbar
//
#define WINDOWS_ID_BIAS   1200   //
#define TXTREF_PROP_BIAS 20000   // Смещение идентификаторов текстовых свойств, хранящихся в TextRefTbl
#ifdef __WIN32__
	#define KB_CTRLENTER   kbF11
#else
	#define KB_CTRLENTER   kbCtrlEnter
#endif
// @v12.2.2 #define PP_MSGLEN         1024   // Макс. длина сообщения //
// @v12.2.2 #define PP_SYMBLEN          32   // Макс. длина символа переменной

#define PP_RCDATA          501   // Тип ресурса для сырых данных
	#define DBFS_MNEXPORT             1001   // Таблица DBF для экспорта в ККМ Ел-92-Акв
	#define DBFS_ECREXPORT            1002   // Таблица DBF для экспорта в ККМ ЭКР-4110
	#define DBFS_SMUKM_GOODS_EXPORT   1003
	#define DBFS_SMUKM_BAR_EXPORT     1004
	#define DBFS_RETAILGOODSHDR       1005
	#define DBFS_RETAILGOODS          1006
	#define DBFS_TRFRANLZLIC          1007   // DBF экспорт анализа товарных операций
	#define DBFS_PETROVICHEXPORT      1008   // DBF экспорт в драйвер PETROVICH
	#define DBFS_Exp_Price_List       1016
	#define DBFS_PALM_GOODS           1017   // DBF экспорт в StyloPalm - товары
	#define DBFS_PALM_GOODSGRP        1018   // DBF экспорт в StyloPalm - группы товаров
	#define DBFS_PALM_CLIENTS         1019   // DBF экспорт в StyloPalm - клиенты
	#define DBFS_CRCS_GOODS_EXPORT    1020   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_BAR_EXPORT      1021   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CLN_BNK_EXPORT       1022   // DBF экспорт в клиент-банк
	#define DBFS_CRCS_CARDS_EXPORT    1023   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_SCALE_EXPORT    1024   // DBF экспорт в Кристалл-Кассовый сервер (весы)
	#define DBFS_CRCS_DSCNT_EXPORT    1025   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_ATLAS_GDSBILL_HDRS   1026   // DBF экспорт в ПИК "Госмонополия РСМ" (НТЦ Атлас)
	#define DBFS_ATLAS_GDSBILL_ROWS   1027   // DBF экспорт в ПИК "Госмонополия РСМ" (НТЦ Атлас)
	#define DBFS_CRCS_CSHRS_EXPORT    1028   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_PALM_CLIADDR         1029   // DBF экспорт в StyloPalm - адреса клиентов
	#define DBFS_PALM_CLIDEBT         1030   // DBF экспорт в StyloPalm - долги клиентов
	#define DBFS_PALM_QUOTKIND        1031   // DBF экспорт в StyloPalm - виды котировок
	#define DBFS_PALM_TODO            1032   // DBF экспорт/импорт в StyloPalm - задачи
	#define DBFS_PALM_CLISELL         1033   // DBF экспорт в StyloPalm - продажи по клиентам
	#define DBFS_CRCS_DSCNT_EXP49     1034   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_PPY_GOODS_IMPEXP     1035   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_BARCODES_IMPEXP  1036   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_QUOTKINDS_IMPEXP 1037   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_QUOTS_IMPEXP     1038   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_SCARDS_IMPEXP    1039   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_CSESS_IMPEXP     1040   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_CCHECKS_IMPEXP   1041   // DBF импорт/экспорт данных по кассам
	#define DBFS_PPY_CCLINES_IMPEXP   1042   // DBF импорт/экспорт данных по кассам
	#define DBFS_EGAIS_GDSBILL_HDRS   1043   // DBF экспорт в ЕГАИС
	#define DBFS_EGAIS_GDSBILL_ROWS   1044   // DBF экспорт в ЕГАИС
	#define DBFS_CRCS_GROUP_EXPORT    1045   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_GDSQTTYDSC_EXPORT  1046   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_GRPQTTYDSC_EXPORT  1047   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_SALESGGRP_EXPORT   1048   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_SALESGGRPI_EXPORT  1049   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_GOODSDIS_EXPORT    1050   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_GOODSGRPDIS_EXPORT 1051   // DBF экспорт в Кристалл-Кассовый сервер
	#define DBFS_CRCS_SIGNAL_ALL_EXPORT  1052   // Сигнальный DBF-файл для Set Retail 5
	#define CACLFUNC_DESCR            1101   // Описание встроенных функций калькулятора
	#define TAB_UNITS                 1102   // Единицы измерения (см. UNIT_XXX) //
	#define TAB_HELP                  1103   // Таблица соответствий идентификаторов тем Help'а и идентификаторов
		// окон (диалогов, таблиц и т.д.)
	//
	// Зарезервированные объекты
	//
	#define ROD_CONFIG           2   // Конфигурации
	#define ROD_USRGRP           3   // Группы пользователей
	#define ROD_USR              4   // Пользователи
	#define ROD_UNIT             5   // Единицы измерения //
	#define ROD_CITYSTATUS       6   // Статусы населенных пунктов
	#define ROD_PRSNKIND         7   // Виды персоналий
	#define ROD_PRSNSTATUS       8   // Статусы персоналий
	#define ROD_OPRTYPE          9   // Типы операций
	#define ROD_BNKACCTYPE      10   // Типы банковских счетов
	#define ROD_AMOUNTTYPE      11   // Суммы документов
	#define ROD_ACTION          12   // Системные событи
	#define ROD_LOCATION        13   // Локации
	#define ROD_REGISTERTYPE    14   // Типы регистрационных документов
	#define ROD_ELINKKIND       15   // Виды электронных адресов
	#define ROD_PERSONRELTYPE   16   // Типы персональных отношений
	#define ROD_QUOTKIND        17   // Виды котировок
	#define ROD_TAG             18   // Типы тегов
	#define ROD_OPCOUNTER       19   // Счетчики
	#define ROD_TAXSYSTEMKIND   20   // Виды налогообложения //
	#define ROD_GOODSBASKET     21   // Товарные корзины //
#define PP_RCDECLSTRUC        502   // Тип ресурса для деклараций структур экспорта/импорта данных
#define PP_RCDECLOBJ          503   // Тип ресурса для деклараций типов объектов PPObject
	//
	// PPOBJ_XXX PP_RCDECLOBJ { "Symb\0", "Descript\0" };
	//
#define PP_RCDECLJOB          504   // Тип ресурса для декларации типов задач PPJob
	//
	// PPJOB_XXX PP_RCDECLJOB { "Symb\0", "Descript\0" };
	//
#define PP_RCDECLCMD          505   // Тип ресурса для деклараций типов команд
#define PP_RCDECLRECORD       506   // Тип ресурса для деклараций записей (SdRecord)
	//
	// PPREC_XXX PP_RCDECLRECORD { "Symb\0", FieldsCount, // Далее следуют описания полей в следующем формате:
	//   FldID, "FldName\0", FldType, FldFormat, "FldDescr\0"
	//   ...
	//
#define PP_RCDECLREPORTSTUB   507   // Тип ресурса для декларации отчетов
	//
	// REPORT_XXX PP_RCDECLREPORT { "Symb\0", "DataName\0", "Descript\0" }
	//
#define PP_RCDECLVIEW         508   // Тип ресурса для декларации КАД (PPView)
	//
	// PPVIEW_XXX PP_RCDECLVIEW { "Symb\0", "Descript\0" }
	//
#define PP_RCDECLFILT         509   // Тип ресурса для декларации фильтра (PPBaseFilt)
	//
	// PPFILT_XXX PP_RCDECLFILT { "Symb\0" }
	//
#define PP_RCDECLCTRLMENU     510   // Тип ресурса для декларации меню, привязанного к элементу диалога
	//
	// PPCTRLMENU_XXX PP_RCDECLCTRLMENU { count, ["Text\0", keyCode,]* "Text\0", keyCode }
	//
#define PP_RCDECLRFILE        511   // Тип ресурса для декларации зарезервированных файлов
	//
	// PPRFILE_XXX PP_RCDECLRFILE { "Symb\0", "file_name", PPPATH_XXX(1), PPPATH_XXX(2), flags, "Descript\0" }
	// Мнемоника PPPATH_XXX(1) определяет местонахождение файла в установленном дистрибутиве
	// Мнемоника PPPATH_XXX(2) определяет исходное местонахождение файла при сборке дистрибутива (reserved)
	//
//
// Descr: Флаги зарезервировнных системой имен файлов.
// @attention: Значения флагов менять нельзя - проецируются на исходный rc-файл
//
#define PPRFILEF_TEXT      0x0001 // Текстовый файл
#define PPRFILEF_DISTRIB   0x0002 // Файл распространяется с дистрибутивом
#define PPRFILEF_UPD       0x0004 // Файл может быть изменен после установки дистрибутива

#define PP_FIRSTUSROBJ       1001L
#define PP_FIRSTUSRREF       1001L
#define PP_FIRSTUSRPROP      1001L
#define PP_FIRSTUSRLOCATION   101L
#define PP_MAXATURNTEMPLATES  100 // Макс. колич. шаблонов для операции
//
// Прикладные типы данных SType
//
#define S_ACCT          S_FIRSTUSERTYPE
#define T_BAL           MKSTYPE(S_ACCT, sizeof(int16)+sizeof(int16))
#define T_ACCT          MKSTYPE(S_ACCT, sizeof(int16)+sizeof(int16)+sizeof(int32))
#define S_ARTICLE       (S_ACCT+1)
//
// Виды активности счетов
//
#define ACT_ACTIVE           1
#define ACT_PASSIVE          2
#define ACT_AP               3   // Активно-пассивные счета
//
// Типы счетов
//
#define ACY_BAL              1   // Балансовый счет
#define ACY_OBAL             2   // Внебалансовый счет
#define ACY_AGGR             3   // Агрегирующий счет
#define ACY_REGISTER         4   // Регистр
#define ACY_PERSONAL         5   // Персональный счет (under construction)
#define ACY_ALIAS            6   // Элиас счета
#define ACY_BUDGET           7   // Бюджетный счет
//
// Порядок счета
//
#define ACO_1                1   // Синт. счет 1-го порядка
#define ACO_2                2   // Синт. счет 2-го порядка (субсчет)
#define ACO_3                3   // Терминальный счет (не обязательно субконто)
//
// Смещение поля AccTurn.RByBill для проводок покрытия валютных операций в базовой валюте
//
#define BASE_RBB_BIAS     1000
//
// Флаги форматирования счетов
//
#define ACCF_BAL        0x0001   // Без аналитической статьи
#define ACCF_DELDOT     0x0002   // Разделитель между уровнями - точка
#define ACCF_DELSPACE   0x0004   // Разделитель между уровнями - пробел
#define ACCF_DELHYP     0x0008   // Разделитель между уровнями - тире
#define ACCF_PADACC     0x0010   // Выравнивать нулями счет (до 2-х разрядов)
#define ACCF_PADSUB     0x0020   // Выравнивать нулями субсчет (до 2-х разрядов)
#define ACCF_NAMEONLY   0x0040   // Если счет идет с наименованием, то имя не выводить

#define ACCF_DEFAULT    (ACCF_DELDOT|ACCF_PADACC)
//
// Флаг форматирования аналитической статьи
//
#define ARTF_NAMEONLY   0x0040
//
// Флаги форматирования количества товара (QttyToStr)
//
#define QTTYF_NUMBER    0x0100   // Просто число
#define QTTYF_SIMPLPACK 0x0200   // "q/p"
#define QTTYF_COMPLPACK 0x0400   // "q/p+f"
#define QTTYF_FRACTION  0x0800   // 9.999 три знака после точки
#define QTTYF_MASK      (QTTYF_NUMBER|QTTYF_SIMPLPACK|QTTYF_COMPLPACK|QTTYF_FRACTION)
//
// Важные балансовые счета по умолчанию.
//
#define DEFAC_SUPPL          60   // @used{GetCommConfig()} Расчеты с поставщиками
#define DEFAC_SELL           45   // @used{GetCommConfig()} Расчеты с покупателями
#define DEFAC_CASH           50   // @used{GetCommConfig()} Касса
//
// Полезные макроопределения //
//
#define PPDEBIT               0
#define PPCREDIT              1
//
// Порядок реализации товаров
//
#define RLZORD_UNDEF          0
#define RLZORD_FIFO           1
#define RLZORD_LIFO           2
//
// Фиксированные должности
//
#define PPFIXSTF_DIRECTOR     1 // Первое лицо
#define PPFIXSTF_ACCOUNTANT   2 // Главный бухгалтер
#define PPFIXSTF_CASHIER      3 // Кассир (или лицо, выписавшее документ)
//
// Флаги статуса
//
#define PPSTATE_DATECLOSED    0x00000001L
#define PPSTATE_DISABLELOGIN  0x00000002L
//
// Свойства конфигурации системы, групп и пользователей. (Property.Prop)
//
#define PPPRP_CFG             1L  // Структура PPConfig
//
// Идентификаторы путей
//
#define PPPATH_FIRST          2L
#define PPPATH_DRIVE          2  // Дисковод
#define PPPATH_ROOT           3  // Корневой каталог, определенный в конфигурации. Может отличаться от PPPATH_SYSROOT
#define PPPATH_SYS            4  // Системный подкаталог (словарь базы данных)
#define PPPATH_DAT            5  // База данных
#define PPPATH_ARC            6  // Архив базы данных
#define PPPATH_BIN            7  // Исполняемые файлы
#define PPPATH_IN             8  // Входящие файлы
#define PPPATH_OUT            9  // Исходящие файлы
#define PPPATH_TEMP          10  // Временные файлы
#define PPPATH_LOG           11  // Log-файлы
#define PPPATH_DD            12 // Файлы описания данных
#define PPPATH_SYSROOT       13 // Корневой каталог относительно каталога BIN (BIN\..)
#define PPPATH_LOCAL         14 // Каталог локальных по отношению к пользователю компьютера данных
	// Используется системный каталог "Application Data\Papyrus"
#define PPPATH_WTM           15 // Каталог хранения ватманов и графических панелей инструментов (.wtm, .wta)
#define PPPATH_PACK          16 // Каталог хранения некоторых файлов, распространяемых с дистрибутивом
#define PPPATH_DOC           17 // Каталог хранения документации на систему
#define PPPATH_SPII          18 // Каталог обмена между мобильными устройствами и JobServer'ом
#define PPPATH_SARTREDB      19 // Путь к базе данных проекта Sartre
#define PPPATH_WORKSPACE     20 // Путь к каталогу рабочего пространства (туда перенесем все конфигруационные и прочие файлы, не относящиеся к дистрибутиву, из BIN
#define PPPATH_TESTROOT      21 // (только для отладки и тестирования) корневой каталог тестовых данных
#define PPPATH_REPORTDATA    22 // [system]PPINIPARAM_REPORTDATAPATH
#define PPPATH_UID           23 // @v11.9.3 Путь к каталогу с json-описаниями пользовательского интерфейса
#define PPPATH_SRCROOT       24 // @v11.9.4 (для верификации сборки, отладки, тестирования) корневой каталог исходных кодов системы
#define PPPATH_BUILDROOT     25 // @v12.3.2 (для верификации сборки, отладки, тестирования) корневой рабочий каталог разработки

#define PPPRP_PATHS          20L  //
#define PPPRP_PRINTER        21L  // Принтер
#define PPPRP_CFGPRIVATE     22L  // Частная конфигурация пользователя //
//
// Следующие типы свойств используются только объектом
// PPOBJ_CONFIG (для групп и пользователей не создаются и игнорируются)
//
#define PPPRP_COMMCFG              101L  // Общие настройки
#define PPPRP_DISLIST              102L  // Система оптовых скидок (может быть несколько записей)
#define PPPRP_GOODSCFG             103L  // Конфигурация справочника товаров
#define PPPRP_VATBSCFG             104L  // Конфигурация книги продаж
#define PPPRP_VATBBCFG             105L  // Конфигурация книги покупок
#define PPPRP_EXPMONARCH           106L  // Конфигурация экспорта в устройство Monarch
#define PPPRP_DBXCHGCFG            107L  // Конфигурация обмена между разделами БД
#define PPPRP_CURRENCYCFG          108L  // Конфигурация валютного учета
#define PPPRP_CURREVALCFG          109L  // Конфигурация валютной переоценки
#define PPPRP_PLISTCFG             110L  // Конфигурация прайс-листа
#define PPPRP_BILLCFG              111L  // Конфигурация документов
#define PPPRP_PREDICTCFG           112L  // Конфигурация прогноза продаж
#define PPPRP_ALBATROSCFG          113L  // Конфигурация проекта альбатрос (version 1)
#define PPPRP_INETCONNCFG          114L  // Конфигурация подключения к интернету
#define PPPRP_GRPSDSCNT            115L  // Скидки на группы для данной котировки
#define PPPRP_SMPLLEDGCFG          116L  // Конфигурация книги доходов / расходов
#define PPPRP_ALBATROSCFG2         117L  // Конфигурация проекта альбатрос (version 2)
#define PPPRP_MAILCHARRYCFG        118L  // Конфигурация посылки объектов charry по почте
#define PPPRP_EQUIPCFG             119L  // Конфигурация оборудования //
#define PPPRP_CLIBNKASSCCFG        120L  // @obsolete since @v5.7.8 Ассоциация видов операций для импорта из система клиент-банк //
#define PPPRP_STYLOPALMCFG         121L  // Конфигурация обмена данными со StyloPalm
#define PPPRP_PROJECTCFG           122L  // Конфигурация проектов
#define PPPRP_MRPTABCFG            123L  // Конфигурация MRP-таблиц
#define PPPRP_PRCCFG               124L  // Конфигурация процессоров, технологий
#define PPPRP_TSESSCFG             125L  // Конфигурация техн сессий
#define PPPRP_KEYBWKEYCFG          126L  // Конфигурация клавиатуры с ключом
#define PPPRP_SCARDCFG             127L  // Конфигурация дисконтных карт
#define PPPRP_DESKCMDASSOC         128L  // Ассоциации команд рабочего стола
#define PPPRP_PERSONCFG            129L  // Конфигурация справочника персоналий
#define PPPRP_DBMAINTAINCFG        130L  // Конфигурация обслуживания базы данных
#define PPPRP_CLIBNKASSCCFG2       131L  // Вторая версия ассоциаций видов операций для импорта из система клиент-банк //
#define PPPRP_TRANSPCFG            132L  // Конфигурация транспорта
#define PPPRP_DEBTORSTATCFG        133L  // Конфигурация статистики дебиторов
#define PPPRP_LOCATIONCFG          134L  // Конфигурация локаций
#define PPPRP_PERSONCFGEXTFLDLIST  135L  // Список дополнительных полей для адресов доставки
#define PPPRP_STOCKOPTCFG          136L  // Конфигурация оптимизации запасов
#define PPPRP_GLOBALUSERACCCFG     137L  // Конфигурация глобальных учетных записей
#define PPPRP_WORKBOOKCFG          138L  // Конфигурация рабочих книг
#define PPPRP_PERSONDETECTIONLIST  139L  // @vmiller Список конфигураций для определения новизны клиента
#define PPPRP_ALCREPORTCFG         140L  // Конфигурация алкогольной декларации
#define PPPRP_TSSTAKECFG           141L  // PPObjTimeSeries
#define PPPRP_CCDATETOMAXIDINDEX   142L  // @v11.7.4 (Не конфигурация!) Индекс соответствий {дата чека}->{максимальный ид} для быстрого поиска расширенных данных по чекам.
#define PPPRP_MRKTPLCCFG           143L  // @v12.1.5 Конфигурация маркетплейсов
//
// Layout of extra record of access rights
//
#define PPPRP_RTBIAS       2000L            // Смещение прав доступа
#define PPPRP_RTCOMM       (PPPRP_RTBIAS+0) // Общие права доступа
//
// Далее следуют права доступа по объектам (PPPRP_RTBIAS + ObjType)
//
#define PPPRP_RTLAST       3999L  // Последнее свойство, описывающее права
#define PPPRP_RT_OPLIST    4001L  // Список доступных опрераций
#define PPPRP_RT_LOCLIST   4002L  // Список доступных складов
#define PPPRP_RT_CFG       4003L  // Список прав доступа по конфигурациям
#define PPPRP_RT_ACCLIST   4004L  // Права на список счетов
#define PPPRP_RT_POSLIST   4005L  // Права на список кассовых узлов
#define PPPRP_RT_QKLIST    4006L  // Права на список видов товарных котировок

#define PPPRP_FIRSTOBJMEMO 5001L  // Первое свойство объекта, в котором хранится произвольное примечание к нему
#define PPPRP_BILLMEMO     5002L  // Произвольное примечание к документу
#define PPPRP_CHKINPMEMO   5003L  // Пул примечаний к списку персональных регистраций PPCheckInPersonArray
#define PPPRP_LASTOBJMEMO  5999L  // Максимальное свойство объекта, в котором хранится примечание
//
// Типы объектов и зарезервированные объекты
//
#define PPOBJ_FIRSTDYN   10001L  // Первый динамически выделяемый ИД объекта
#define PPOBJ_LASTDYN    20000L  // Последний динамически выделяемый ИД объекта
#define IS_DYN_OBJTYPE(o) ((o)>=PPOBJ_FIRSTDYN && (o)<PPOBJ_LASTDYN)
//
// Descr: макрос возвращает !0 если тип объекта o хранится в таблице Reference
//
#define IS_REF_OBJTYPE(o) (((o)>=1 && (o)<1000) || IS_DYN_OBJTYPE(o))
//
// Типы объектов-конфигураций
//
#define PPOBJ_FIRST_CFG_OBJ 100000L // Первый объект конфигурации
#define PPOBJ_LAST_CFG_OBJ  100999L // Последний объект конфигурации
//
// Prefix of configuration objects: PPCFGOBJ_
//
#define PPCFGOBJ_GOODS            (PPOBJ_FIRST_CFG_OBJ + 1)
#define PPCFGOBJ_BILL             (PPOBJ_FIRST_CFG_OBJ + 2)
#define PPCFGOBJ_PROJECT          (PPOBJ_FIRST_CFG_OBJ + 3)
#define PPCFGOBJ_TECHSESS         (PPOBJ_FIRST_CFG_OBJ + 4)
#define PPCFGOBJ_PREDICTSALES     (PPOBJ_FIRST_CFG_OBJ + 5)
#define PPCFGOBJ_PREDICTHOLYDAYS  (PPOBJ_FIRST_CFG_OBJ + 6)
#define PPCFGOBJ_ALBATROS         (PPOBJ_FIRST_CFG_OBJ + 7)
#define PPCFGOBJ_INETCONN         (PPOBJ_FIRST_CFG_OBJ + 8)
#define PPCFGOBJ_VATBOOKSELL      (PPOBJ_FIRST_CFG_OBJ + 9)
#define PPCFGOBJ_VATBOOKBUY       (PPOBJ_FIRST_CFG_OBJ + 10)
#define PPCFGOBJ_SIMPLELEDGER     (PPOBJ_FIRST_CFG_OBJ + 11)
#define PPCFGOBJ_PRICELIST        (PPOBJ_FIRST_CFG_OBJ + 12)
#define PPCFGOBJ_DBXCHNG          (PPOBJ_FIRST_CFG_OBJ + 13)
#define PPCFGOBJ_EQUIP            (PPOBJ_FIRST_CFG_OBJ + 14)
#define PPCFGOBJ_STYLOPALM        (PPOBJ_FIRST_CFG_OBJ + 15)
#define PPCFGOBJ_CLIBNKAS         (PPOBJ_FIRST_CFG_OBJ + 16)
#define PPCFGOBJ_JOBPOOL          (PPOBJ_FIRST_CFG_OBJ + 17)
#define PPCFGOBJ_CLIENTDEAL       (PPOBJ_FIRST_CFG_OBJ + 18)
#define PPCFGOBJ_SUPPLDEAL        (PPOBJ_FIRST_CFG_OBJ + 19)
#define PPCFGOBJ_COMMCONFIG       (PPOBJ_FIRST_CFG_OBJ + 20)
#define PPCFGOBJ_KEYBWKEYCFG      (PPOBJ_FIRST_CFG_OBJ + 21)
#define PPCFGOBJ_CSESSIMPEXPCFG   (PPOBJ_FIRST_CFG_OBJ + 22)
#define PPCFGOBJ_SCARD            (PPOBJ_FIRST_CFG_OBJ + 23)
#define PPCFGOBJ_PERSON           (PPOBJ_FIRST_CFG_OBJ + 24)
#define PPCFGOBJ_DBMAINTAIN       (PPOBJ_FIRST_CFG_OBJ + 25)
#define PPCFGOBJ_TRANSP           (PPOBJ_FIRST_CFG_OBJ + 26)
#define PPCFGOBJ_DEBTORSTAT       (PPOBJ_FIRST_CFG_OBJ + 27)
#define PPCFGOBJ_LOCATION         (PPOBJ_FIRST_CFG_OBJ + 28)
#define PPCFGOBJ_STOCKOPT         (PPOBJ_FIRST_CFG_OBJ + 29)
#define PPCFGOBJ_GLOBALUSERACC    (PPOBJ_FIRST_CFG_OBJ + 30)
#define PPCFGOBJ_WORKBOOK         (PPOBJ_FIRST_CFG_OBJ + 31)
#define PPCFGOBJ_ALCREPORT        (PPOBJ_FIRST_CFG_OBJ + 32)
#define PPCFGOBJ_TIMESERIES       (PPOBJ_FIRST_CFG_OBJ + 33)
#define PPCFGOBJ_MARKETPLACE      (PPOBJ_FIRST_CFG_OBJ + 34) // @v12.1.5
//
#define PPOBJ_CONFIG              1L  // $(PPSecur) Конфигурация системы
	#define PPCFG_MAIN           1L
#define PPOBJ_CURRENTSTATE       2L  // Текущее состояние // @v11.3.8 PPOBJ_STATUS-->PPOBJ_CURRENTSTATE Объект теперь используется для хранения специальных
	// данных, отражающих текущее состояние, используемое для инфраструктурых целей.
	#define CSTATEPRP_STQOBJBLOBLIST 1L // @v11.3.8 Список идентификаторов объектов, у которых есть BLOB'ы подлежащие отправки сервисам-медиаторам Stylo-Q
#define PPOBJ_USRGRP              3L  // $(PPSecur) Группы пользователей (struct PPSecur)
	#define PPGRP_EVERYONE       1L
#define PPOBJ_USR                 4L  // $(PPSecur) Пользователи (struct PPSecur)
	#define PPUSR_MASTER         1L
#define PPOBJ_UNIT                5L  // $ Единицы измерения (struct PPUnit)
	// @v11.9.4 Макро-определения PPUNT_XXX замещаются аналогичными SUOM_XXX. С этой целью значения SUOM_XXX изменены чтобы
	// обеспечить бинарную совместимость с существующими базами данных.
	//#define PPUNT_KILOGRAM       1L
	//#define PPUNT_METRE          2L
	//#define PPUNT_SECOND         3L // Зарезервировано, но можно не выводить
	//#define PPUNT_LITER          4L
	//#define PPUNT_ITEM          21L
#define PPOBJ_CURRATE             6L  // @unused # Просто строки
	#define PPCUR_MAIN           1L // Основной курс. Что угодно, но "основное"
#define PPOBJ_CITYSTATUS          7L  // $(PPWorldObjStatus) Статусы географических объектов
	#define PPCTS_CITY           1L
	#define PPCTS_STREET         2L
#define PPOBJ_PERSONKIND          8L  // $ Виды персоналий
	#define PPPRK_MAIN           1L // Главная организация (Владелец программы)
	#define PPPRK_FILIAT         2L // Филиал главной организации
	#define PPPRK_EMPL           3L // Персонал
	#define PPPRK_CLIENT         4L // Клиенты
	#define PPPRK_SUPPL          5L // Поставщики
	#define PPPRK_BANK           6L // Банки
	#define PPPRK_MANUF          7L // Производители
	#define PPPRK_AGENT          8L // Агенты
	#define PPPRK_HOMEOFFICE     9L // Отделы|Управления внутренних дел
	#define PPPRK_BUSADMIN      10L // Гос. службы, регистрирующие юр. лица
	#define PPPRK_EMPLOYER      11L // Работодатель
	#define PPPRK_UNKNOWN       12L // Неизвестный вид (если персоналия теряет принадлежность ко всем видам, то приобретает принадлежность к виду
		//  PPPRK_UNKNOWN. Приобретая любой вид кроме этого, персоналия теряет вид PPPRK_UNKNOWN).
	#define PPPRK_SHIPOWNER     13L // Владельцы транспортных средств
	#define PPPRK_VESSELSAGENT  14L // Транспортные брокеры
	#define PPPRK_CAPTAIN       15L // Командиры транспортных средств
	#define PPPRK_DTO           16L // Организации-операторы передачи данных
	#define PPPRK_MARKETPLACE   17L // @v12.0.12 Маркетплейсы

#define PPOBJ_PRSNSTATUS          9L  // $ Юридические статусы персоналий
	#define PPPRS_PRIVATE        1L // Физическое лицо
	#define PPPRS_FREE           2L // Частный предприниматель
	#define PPPRS_PRIVENTR       3L // Частное предприятие
	#define PPPRS_LTD            4L // ООО
	#define PPPRS_STOCK          5L // Просто АО (может открытого, может закрытого типа)
	#define PPPRS_OPENSTOCK      6L // АООТ
	#define PPPRS_CLOSESTOCK     7L // АОЗТ
	#define PPPRS_JOINT          8L // Совместное предприятие
	#define PPPRS_FILIATION      9L // Филиал
	#define PPPRS_STATE         10L // Государственная организация //
	#define PPPRS_LEGAL         11L // Юридическое лицо
	#define PPPRS_COUNTRY       12L // Государство
	#define PPPRS_REGION        13L // @v12.1.3 Регион государства
#define PPOBJ_STAFFRANK          10L  // #
	#define PPSFR_DIRECTOR       1L // Первое лицо (Директор, Ген. Директор и тд)
	#define PPSFR_ACCOUNTANT     2L // Главный бухгалтер
#define PPOBJ_OPRTYPE            11L  // $ Типы операций *
	#define PPOPT_ACCTURN        1L // Бухгалтерский документ
	#define PPOPT_GOODSRECEIPT   2L // Приход товара
	#define PPOPT_GOODSEXPEND    3L // Расход товара
	//
	// Документы, имеющие тип PPOPT_PAYMENT должны иметь ссылку на
	// документ, который они оплачивают.
	//
	#define PPOPT_PAYMENT        4L // Оплата
	//
	// Документы, имеющие тип PPOPT_GOODSRETURN должны иметь ссылку на
	// документ по которому товар был получен. Приходная это операция или
	// расходная, зависит от того, на какой документ ссылается возвратный док.
	//
	#define PPOPT_GOODSRETURN    5L // Возврат товара
	#define PPOPT_GOODSREVAL     6L // Товарная переоценка
	#define PPOPT_GOODSORDER     7L // Товарный заказ
	#define PPOPT_GOODSMODIF     8L // Модификация товаров
	#define PPOPT_INVENTORY      9L // Инвентаризация //
	//
	// Документы, имеющие тип PPOPT_CHARGE должны иметь ссылку на документ,
	// который оговаривает условия выплат. Поведение этих документов аналогично
	// поведению расширенных бух. проводок. Как правило, операции этого типа
	// должны иметь флаг OPKF_NEEDPAYMENT.
	//
	#define PPOPT_CHARGE        10L // Начисление
	#define PPOPT_GENERIC       11L // Обобщение (коллекция операций)
	#define PPOPT_GOODSACK      12L // Acknowledge receipt of goods
	#define PPOPT_POOL          13L // Пул документов
	#define PPOPT_CASHSESS      14L // Кассовая сессия //
	#define PPOPT_DRAFTRECEIPT  15L // Draft goods receiption bill
	#define PPOPT_DRAFTEXPEND   16L // Draft goods expend bill
	#define PPOPT_WAREHOUSE     17L // Документы складских операций. Строки этих документов выполняют товарные транзакции над ячейками склада
		// или над адресами ответственного хранения.
	#define PPOPT_EXTERNAL      18L // Специализированный тип операции, определяющий документ, содержащий внешний документ (обычно, файл).
	#define PPOPT_AGREEMENT     19L // Соглашение (договор) с покупателем или поставщиком
	#define PPOPT_CORRECTION    20L // Корректирующий документ
	#define PPOPT_DRAFTTRANSIT  21L // Специальная категория драфт-документов, которые
		// отражают транзакции и отчеты сторонних контрагентов, для обмена друг с другом посредством системы, но
		// без посредничества главной организации.
	#define PPOPT_DRAFTQUOTREQ  22L // Запрос котировок (запрос на возможность, сроки и стоимость поставки товаров)
#define PPOBJ_OPRKIND            12L  // $ Виды операций
	#define PPOPK_GENERICACCTURN         1L // Общая бухгалтерская проводка
	#define PPOPK_RECEIPT                2L // Прием товара от поставщика
	#define PPOPK_SELL                   3L // Продажа товара именованному клиенту
	#define PPOPK_RETAIL                 4L // Продажа в розницу неименованному клиенту
	#define PPOPK_INTREXPEND             5L // Внутренняя передача товара
	#define PPOPK_INTRRECEIPT            6L // Внутренний прием товара
//
// Следующие три операции зарезервированы только для конвертации
// из других баз данных, где возврат осуществляется без ссылки на осн. док.
// Какие-либо операции с этими видами при нормальной работе запрещены
//
	#define _PPOPK_SUPPLRET              7L // Левый возврат поставщику (без ссылки)
	#define _PPOPK_SELLRET               8L // Левый возврат от клиента (без ссылки)
	#define _PPOPK_RETAILRET             9L // Левый возврат в розницу (без ссылки)
	#define PPOPK_INVENTORY             10L // Инвентаризация //
	#define PPOPK_CASHSESS              11L // @unused Кассовая сессия (тип PPOPT_CASHSESS)
	#define PPOPK_EDI_ACTCHARGEON       12L // Постановка лотов на баланс в ЕГАИС
	#define PPOPK_EDI_RECADV            13L // Драфт-операция подтверждения отгрузки, поступившего из EDI
	#define PPOPK_EDI_STOCK             14L // Драфт-операция подтверждения остатков, учитываемых сервисом EDI
	#define PPOPK_EDI_SHOPCHARGEON      15L // Драфт-операция перемещения остатков со склада на регистр 2 ЕГАИС
	#define PPOPK_EDI_ACTCHARGEONSHOP   16L // Постановка лотов на баланс торгового зала в ЕГАИС (регистр 2)
	#define PPOPK_EDI_WRITEOFFSHOP      17L // Списание с баланса торгового зала в ЕГАИС (регистр 2)
		// Если документ содержит специальную метку, то это - автоматическое списание реализации по акцизным маркам
	#define PPOPK_EDI_ORDERRSP          18L // Драфт-операция подтверждения заказа, переденного в EDI
	#define PPOPK_EDI_WROFFWITHMARKS    19L // Драфт-операция списание с остатков ЕГАИС акцизных марок
	#define PPOPK_EDI_CHARGEONWITHMARKS 20L // Драфт-операция постановки на баланс маркированной продукции
	#define PPOPK_GENERICREGACCTURN     21L // Общая регистровая бухгалтерская проводка
	#define PPOPK_UNASSIGNED          1000L // Специальный идентификатор
	//
	// Extra records of PPOBJ_OPRKIND
	#define OPKPRP_BILLMEMO   (PP_MAXATURNTEMPLATES+1)  // @obsolete since @v3.0.2 Шаблон текста для ввода в примечание документа
	#define OPKPRP_EXTSTRDATA (PP_MAXATURNTEMPLATES+1)  //
	#define OPKPRP_EXAMTLIST  (PP_MAXATURNTEMPLATES+2)  //
	#define OPKPRP_INVENTORY  (PP_MAXATURNTEMPLATES+3)  //
	#define OPKPRP_GENLIST    (PP_MAXATURNTEMPLATES+4)  // PPIDArray obsolete since @v3.10.8
	#define OPKPRP_PAYMOPLIST (PP_MAXATURNTEMPLATES+5)  //
	#define OPKPRP_BILLPOOL   (PP_MAXATURNTEMPLATES+6)  //
	#define OPKPRP_DRAFT      (PP_MAXATURNTEMPLATES+7)  //
	#define OPKPRP_GENLIST2   (PP_MAXATURNTEMPLATES+8)  // ObjRestrictArray
	#define OPKPRP_DIFFCNTRS  (PP_MAXATURNTEMPLATES+9)  // LAssocArray
	#define OPKPRP_DEBTINVENT (PP_MAXATURNTEMPLATES+10) // PPDebtInventOpEx
#define PPOBJ_ACCSHEET           13L  // $ Таблицы аналитических статей
	#define PPACSH_UNASSIGNED 1000L // Специальный идентификатор
#define PPOBJ_BNKACCTYPE         14L  // # Типы банковских счетов *
	#define PPBAC_NOSTRO         1L // Корр. счет НОСТРО (только для банков)
	#define PPBAC_LORO           2L // Корр. счет ЛОРО (только для банков)
	#define PPBAC_CURRENT        3L // Расчетный счет
	#define PPBAC_CURRENCY       4L // Валютный расчетный счет
	#define PPBAC_LOAN           5L // Кредитный счет
	#define PPBAC_DEPOSIT        6L // Депозитный счет
	#define PPBAC_SAVINGS        7L // Сберегательный счет
#define PPOBJ_PRICETYPE          15L  // @unused Типы цен
	#define PPPRC_BUYING         1L // @unused Цена поступления //
	#define PPPRC_SELLING        2L // @unused Цена реализациии //
#define PPOBJ_AMOUNTTYPE         16L  // $ Типы сумм документов *
	//
	// Extra records of PPOBJ_AMOUNTTYPE
	#define AMTTPRP_FORMULA         1L // Строка формулы

	#define PPAMT_MAIN              1L // Номинальная сумма документа
	#define PPAMT_BUYING            2L // Станд сумма в ценах поступления //
	#define PPAMT_SELLING           3L // Станд сумма в ценах реализации
	#define PPAMT_DISCOUNT          4L // Скидка
	#define PPAMT_PAYMENT           5L // Сумма оплаты на текущий момент
	#define PPAMT_PCTDIS            6L // Скидка в процентах (если задана пользователем)
	#define PPAMT_VATAX             7L // НДС
	#define PPAMT_EXCISE            8L // Акциз
	#define PPAMT_CVAT              9L // НДС в ценах поступления //
	#define PPAMT_PVAT             10L // НДС в ценах реализации
	#define PPAMT_MANDIS           11L // Скидка суммой на весь док (заданная пользователем)
	#define PPAMT_RENTPRD          12L // Сумма выплат по ренте за период
	#define PPAMT_SALESTAX         13L // Налог с продаж
	#define PPAMT_CRATE            14L // Валютный курс относительно базовой валюты
	#define PPAMT_CEXCISE          15L // Акциз в ценах поступления //
	#define PPAMT_TRANSITCRATE     16L // Транзитный валютный курс (валюты определяются из контекста)
	#define PPAMT_FIXPAYMENT       17L // Фиксированная сумма оплаты (не зависит от привязанных оплат)
	#define PPAMT_CS_AGGRSUM       18L // Сумма, агрегированная по чековым строкам кассовой сессии
	#define PPAMT_CS_AGGRREST      19L // Сумма непроведенных излишков по чековым строкам кассовой сессии
	#define PPAMT_CSTAX            20L // Сумма налога с продаж в ценах поступления //
	#define PPAMT_ASSETEXPL        21L // Сумма вводимых в эксплуатацию основных средств
	#define PPAMT_POOLAMOUNT       22L // Сумма системного пула документов (проецируется на внутренние документы пула)
	#define PPAMT_FREIGHT          23L // Сумма фрахта (извлекается из информации по фрахту документа, однако может быть определена и в типе товара).
	#define PPAMT_CS_CASH          24L // Сумма кассовой сессии, оплаченная наличными
	#define PPAMT_CS_BANK          25L // Сумма кассовой сессии, оплаченная безналично
	#define PPAMT_CS_CSCARD        26L // Сумма кассовой сессии, оплаченная корпоративными кредитными картами
	#define PPAMT_MP_ACCEPTANCE    27L // @v12.1.4 Стоимость приемки товара на складе маркетплейса
	#define PPAMT_MP_STORAGE       28L // @v12.1.4 Стоимость хранения товара на складе маркетплейса
	#define PPAMT_MP_COMMISSION    29L // @v12.1.4 Сумма комиссионного вознаграждения маркетплейса
	#define PPAMT_MP_SELLERPART    30L // @v12.1.4 Сумма, перечисляемая маркетплейсом продавцу за проданный товар
	#define PPAMT_MP_ACQUIRING     31L // @v12.1.6 Стоимость экваринга на стороне маркетплейса, котороую маркетплейс переносит на поставщика
	// (это было ошибкой) #define PPAMT_MP_COMMISSIONPCT 32L // @v12.1.8 Процент комиссии маркетплейса с цены продажи
	// (это было ошибкой) #define PPAMT_MP_DISCOUNTPCT   33L // @v12.1.8 Процентная скидка на товар, предоставляемая маркетплейсом к цене продавца
	// Набор сумм по строкам расширения бухгалтерских документов
	#define PPAMT_ADLN_MAIN     41L // Номинальная сумма строки расширения //
#define PPOBJ_CASHNODE           17L  // $ Расчетные кассовые узлы
	#define PPPOSN_SHADOW              101L // @v12.0.12 Идентификатор теневого узла, к которому привязываются образы настоящих чеков. Такие образы формируются //
		// для специальных целей.
	//
	// Extra records of PPOBJ_CASHNODE
	#define CNPRP_EXTRA          1L  // Дополнительная запись
	#define CNPRP_IMPFILES       2L  // Obsolete since @v3.4.9 Файлы импорта (Async)
	#define CNPRP_EXPPATHS       3L  // Obsolete since @v3.4.9 Пути экспорта (Async)
	#define CNPRP_EXTSTR         4L  // Доп строки
	#define CNPRP_DIVGRPASSC     5L  // Ассоциативность группа-отдел
	#define CNPRP_CUSTDISP       6L  // Дисплей покупателя - obsolete
	#define CNPRP_TOUCHSCREEN    7L  // TouchScreen        - obsolete
	#define CNPRP_EXTDEVICES     8L  // Дополнительные устройства
	#define CNPRP_CTBLLIST       9L  // Список номеров столов
	#define CNPRP_APNCORRLIST   10L  // Список корреспондирующих кассовых узлов (Async)
#define PPOBJ_ACTION             18L  // # События, регистрируемые в системном журнале *
	// Текстовые строки, соответствующие событиям содержатся в группе [PPSTR_ACTION=112]
	// со значениями, равными приведенным ниже символам.
	// Символы и значения констант синхронизированы с проектом Stylo-Q (see \Papyrus\Src\Android\StyloQ\app\src\main\java\ru\petroglif\styloq\SLib.java)
	//
	#define PPACN_LOGIN                  1L
	#define PPACN_LOGOUT                 2L
	#define PPACN_TURNBILL               3L
	#define PPACN_UPDBILL                4L
	#define PPACN_RMVBILL                5L
	#define PPACN_PRNBILL                6L
	#define PPACN_ACCESSCASH             7L
	#define PPACN_CSESSCLOSED            8L // Закрытие кассовой сессии
	#define PPACN_EXPCASHSESS            9L // Выгрузка данных для Async Cash Sess
	#define PPACN_UPDCASHSESS           10L //
	#define PPACN_OBJADD                11L // Объект добавлен
	#define PPACN_OBJUPD                12L // Объект изменен
	#define PPACN_OBJRMV                13L // Объект удален
	#define PPACN_OBJUNIFY              14L // Объекты объединены
	#define PPACN_TRANSMOD              15L // Переданы изменения (PPOBJ_DBDIV, DestDbDivID)
	#define PPACN_GOODSQUOTUPD          16L // Изменены котировки товара
	#define PPACN_SCARDDISUPD           17L // Изменена скидка по дисконтной карте
	#define PPACN_GOODSNODISRMVD        18L // Снят признак "Без скидки" с товара
	#define PPACN_GOODSVATGRPCHD        19L // Изменена налоговая группа на товар
	#define PPACN_SCARDBINDUPD          20L // Изменена привязка карты.
		// Объект: документ или чек, Extra - ид предшествующей карты.
	#define PPACN_MAINTAINPRJTASK       21L // Осуществлена процедура обслуживания задач
		// Extra - количество измененных задач
	#define PPACN_BILLSTATUSUPD         22L // Изменен статус документа. Дополнительному полю
		// присваивается значение предшествующего статуса
	#define PPACN_CONFIGCREATED         23L // Создана конфигурация //
	#define PPACN_CONFIGUPDATED         24L // Изменена конфигурация //
	#define PPACN_OBJVIEWED             25L // Объект просмотрен
	#define PPACN_TRANSMIT              26L // Осуществлена передача данных в другой раздел
	#define PPACN_CLEARCHECK            27L // Очистка чека
	#define PPACN_RMVCHKLINE            28L // Удаление позиции из чека
	#define PPACN_RMVALLGDSALTGRP       29L // Удаление всех товаров из альт.группы
	#define PPACN_AUTOFILLALTGRP        30L // Автозаполнение альт.группы
	#define PPACN_CHNGPLUALTGRP         31L // Изменение номера PLU в альт.группе
	#define PPACN_GOODSMTXUPD           32L // Изменена котировка товарной матрицы. В качестве объекта
		// указывается пара {PPOBJ_GOODS, goods_id}, в качестве дополнительного параметра - ид склада, к
		// которому относится элемент измененной матрицы. Если доп параметр == -1, то склад не определен,
		// если доп параметр == 0, то речь идет о котировке по всем складам
	#define PPACN_CCHECKUPDATED         33L // Некоторые параметры чека изменены в ручную
	#define PPACN_OBJSYNCACCEPTED       34L // @construction Объекта акцептирован в другом разделе БД.
		// Дополнительный параметр является идентификатором раздела, в котором акцептирован объект.
		// Дата и время события соответствуют дате и времени акцепта объекта в другом разделе.
		// Из-за этого данное событие вводится не стандартной функцией
	#define PPACN_OBJSYNCCOMMIT         35L // Фиксирована транзакция приема данных из других разделов.
	#define PPACN_BILLCORRECTED         36L // Документ исправлен функцией восстановления //
	#define PPACN_EXT_BILLCORRECTED_LINKRESET 1 // Обнулена ссылка на отсутствующий связанный документ
	#define PPACN_INVENTWROFF           37L // Списание инвентаризации
	#define PPACN_INVENTROLLBACK        38L // Откат списания инвентаризации
	#define PPACN_INVENTBYTERM          39L // Загрузка инвентаризации из терминала
	#define PPACN_INVENTUNITE           40L // Объединение инвентаризаций
	#define PPACN_OPCNTRUPD             41L // Счетчик по виду операции изменен
		// Дополнительное поле получает величину, равную разнице между новым и старым значением счетчика
	#define PPACN_GOODSMTXRESTRUPD      42L // Изменено ограничение на количество элементов матрицы. В качестве объекта
		// указывается пара {PPOBJ_GOODS, goods_id}, в качестве дополнительного параметра - ид склада, к
		// которому относится элемент измененной матрицы. Если доп параметр == -1, то склад не определен,
		// если доп параметр == 0, то речь идет о котировке по всем складам
	#define PPACN_ARSTOPSET             43L // На аналитическую статью установлен флаг STOP
	#define PPACN_ARSTOPRESET           44L // С аналитической статьи снят флаг STOP
	#define PPACN_BIZSCOREUPDATED       45L // Пересчитаны бизнес-показатели
	#define PPACN_GLOBBIZSCOREUPD       46L // Обновлены глобальные бизнес-показатели
	#define PPACN_BILLCCHKPRINTED       47L // По документу отпечатан кассовый чек
		// В качестве доп значения передается ИД кассового узла, по которому отпечатан чек.
	#define PPACN_OBJEXTMEMOUPD         48L // Изменено расширенное примечание по объекту (возможно, удалено)
	#define PPACN_UPDBILLEXT            49L // Изменено расширение документа (независимо от основной части документа)
	#define PPACN_CCHECKDELETED         50L // Чек удален (регистрируется только для ручных операций удаления одного чека).
	#define PPACN_OBJTAGUPD             51L // Изменен тег объекта. Доп параметр - ID тега.
	#define PPACN_INVENTTURN            52L // Создан документ инвентаризации
	#define PPACN_RCVRFREIGHTCITY       53L // Восстановлен город во фрахте документа
	#define PPACN_UPDBILLFREIGHT        54L // Изменен фрахт документа (независимо от основной части документа)
	#define PPACN_UPDBILLOBJECT         55L // Изменен контрагент по документу. Доп параметр - ID предыдущего значения контрагента
	#define PPACN_UPDTSESSPRC           56L // Изменен процессор в техн сессии. Доп параметр - ID предыдущего значения процессора
	#define PPACN_UNMODIFBILLTRANS      57L // Часть изменений принятого документа не могут быть акцептированы.
		// Доп параметр - ID раздела-отправителя //
	#define PPACN_MTXGOODSADD           58L // Создан товар, один из членов родительской ветки которого принадлежит
		// товарной матрице (по любому складу). Это событие необходимо для обновления кэша товарной матрицы другими
		// сеансами для адекватной вставки товара в матрицу.
		// Доп параметр: ID родительской группы, принадлежащей матрице //
	#define PPACN_CCHECKTOGGLEDLVR      59L // Изменен признак завершения доставки по чеку. Доп параметр: 1 - признак установлен, 0 - признак снят
	#define PPACN_UNDOCSESSWROFF        60L // Откат списания кассовой сессии
	#define PPACN_UNDOCSESSGRPNG        61L // Откат группировки кассовой сессии
	#define PPACN_SUPPLDEALUPD          62L // Изменена контрактная цена по поставщику. Доп параметр: ИД поставщика
	#define PPACN_QUOTUPD2              63L // Изменена котировка (2) товара. Доп параметр: ИД транзакции.
	#define PPACN_QUOTRMV2              64L // Удалена котировка (2) товара. Доп параметр: ИД транзакции.
	#define PPACN_DISPOSEBILL           65L // Товарный документ был размещен по складским ячейкам
	#define PPACN_EVENTTOKEN            66L // Событийная метка с объектом типа PPOBJ_EVENTTOKEN
	#define PPACN_UNITECCHECK           67L // Объединены два чека. ИД объекта - чек, который остался, доп параметр: ИД чека, который был влит в первый.
	#define PPACN_SCARDBONUSCHARGE      68L // По бонусной карте начислен бонус. Доп параметр: сжатое значение периода начисления.
	#define PPACN_OBJTAGRMV             69L // Удален тег объекта. Доп параметр - ID тега.
	#define PPACN_PERSONEVENTREDO       70L // Повтор персонального события //
	#define PPACN_SCARDOPMOV            71L // Операция по карте перемещена на другую карту. Доп параметр - ID карты назначения //
	#define PPACN_SMSSENDED             72L // Отправлено SMS персоналии
	#define PPACN_OBJTAGADD             73L // Создан тег объекта. Доп параметр - ID тега.
	#define PPACN_SCARDTRNOVRUPD        74L // Изменено значение оборота по персональной карте
	#define PPACN_UPDBILLWLABEL         75L // Изменен признак WhiteLabel по документу
	#define PPACN_SCARDACTIVATED        76L // Персональная карта активирована
	#define PPACN_STYLOREGISTERING      77L // Зарегистрировано устройство Stylo
	#define PPACN_ATTACHMENTUPD         78L // Приклепленный к объекту файл изменен (доп параметр - номер файла)
	#define PPACN_BILLWROFF             79L // Документ списан
	#define PPACN_BILLWROFFUNDO         80L // Откат списания документа
	#define PPACN_GOODSPASSVSET         81L // Установлен признак "Пассивный" для товара
	#define PPACN_GSTRUCATTACHED        82L // К товару привязана товарная структура
	#define PPACN_GSTRUCDETACHED        83L // От товара отвязана товарная структура
	#define PPACN_GSTRUCCUPD            84L // Изменился компонентный состав товарной структуры (добавлен или удален компонент)
	#define PPACN_TSSERIESUPD           85L // Изменена временная серия
	#define PPACN_TSSTRATEGYUPD         86L // Изменена стратегия временной серии
	#define PPACN_SCARDEXPRYUPD         87L // Изменена дата истечения срока действия персональной карты
	#define PPACN_SYSMAINTENANCE        88L // Событие вызова одной из специальных функций обслуживания системы. Дополнительный параметр - ид функции
	#define PPACN_BILLUNLINK            89L // Удалена привязка документа к другому документу. Дополнительный параметр - ид документа, к которому была привязка
	#define PPACN_EVENTDETECTION        90L // Попытка обнаружения события (PPObjEvent). Следующая попытка будет опираться на время этой записи.
	#define PPACN_RECENTVERSIONLAUNCHED 91L // @v11.2.10 Событие, иниициируемое при запуске сеанса, версия которого превышает последнее значение, зафиксированное
		// таким же событием. Используется в Stylo-Q и, возможно, в иных однопользовательских приложениях. В Papyrus'е не применяется из-за того, что он многопользовательский -
		// там другие механизмы.
	#define PPACN_STYLOQSVCSELFREG      92L // @v11.2.12 Сервис Stylo-Q отправил собственные регистрационные данные медиатору. Доп параметр - ид медиатора.
		// Событие используется для правильной идентификации необходимости повторной саморегистрации после изменения собственных данных.
	#define PPACN_SCARDOWNERSET         93L // @v11.3.1 Установлен владелец персональной карты. Доп параметр - ID предыдущего владельца.
	#define PPACN_STYLOQSVCIDXQUERY     94L // @v11.3.4 Сервис Stylo-Q отправил медиатору данные для индексации. Доп параметр - ид медиатора.
		// Событие используется для правильной идентификации необходимости повторной посылки данных.
	#define PPACN_STYLOQFACETRANSMITTED 95L // @v11.3.9 Клиент Stylo-Q передал сервису свой лик. Объект: oid сервиса {PPOBJ_STYLOQBINDERY; id},
		// доп параметр - ид лика. Событие используется для правильной идентификации необходимости повторной посылки данных.
	#define PPACN_SCARDOWNERUPDATED     96L // @v11.3.10 Изменилась персоналия-владелец карты. Событие привязывается к карте (PPOBJ_SCARD). Доп параметр - ид персоналии.
	#define PPACN_LOTQCERTLINKUPDATED   97L // @v11.9.3 Изменилась ссылка лота на сертификат (ReceiptTbl::Rec::QCertID)
	#define PPACN_BILLSHIPMFLAGSET      98L // @v12.4.3 У документа установлен флаг "Отгружен"
		// !!! При вставке нового значения изменить PPACN_LAST !!!
	#define PPACN_LAST                  98L // Последнее значение (ИЗМЕНИТЬ ПРИ ВСТАВКЕ НОВОГО ЗНАЧЕНИЯ)

	#define SYSMAINTENANCEFUNC_AUTOMQSCONFIG 1 // Автоматическое обновление конфигурации обмена с брокером сообщений
	//
	// Далее следуют идентификаторы событий, которые не отображаются в системном журнале, однако
	// должны быть в том же поле идентификаторов, что и PPACN_XXX поскольку заносятся в очередь
	// извещения о событиях наравне с PPACN_XXX.
	//
	#define PPEVNT_PHNS_PRERING  1001L // PhoneService event: Channel has detected an incoming call and is waiting for ring
	#define PPEVNT_PHNS_RING     1002L // PhoneService event: Line is ringing
	#define PPEVNT_PHNS_RINGING  1003L // PhoneService event: Remote end is ringing
	#define PPEVNT_PHNC_UP       1004L // PhoneService event: Line is up
	#define PPEVNT_MQB_MESSAGE   1101L // MessageQueueBroker event: message
//
// Фантомный объект. В таблице Reference для него не существует записей.
// Могут существовать записи в таблице Property, реализующие линейный
// список групп операций по счетчику. При изменении счетчика любой из
// операций, принадлежащих одной группе, значение счетчика в остальных
// операциях группы изменяется синхронно.
//
#define PPOBJ_COUNTGRP           19L  // @unused Использовался до версии 3.0.1
	#define PPCNT_ONE            1L
#define PPOBJ_BCODESTRUC         20L  // $ Система внутренних штрих-кодов
#define PPOBJ_DBDIV              21L  // $ Разделы базы данных
	//
	// Extra records of PPOBJ_DBDIV
	#define DBDPRP_LOCLIST       1L
	#define DBDPRP_ACCLIST       2L
	#define DBDPRP_EXTSTRING     2L // @alias(DBDPRP_ACCLIST)
#define PPOBJ_GOODSTYPE          22L  // $ Типы товаров
	#define PPGT_DEFAULT         1L
//
// @v1.9.10
// Виртуальный объект. Представлен набором типов проблем.
// В принципе записи в таблице Reference для него существовать
// могут, но любой тип проблемы жестко зашит в программу.
//
#define PPOBJ_PROBLEM            23L // @unused
#define PPOBJ_GOODSSTRUC         24L // $ Структуры товаров
	//
	// Extra records of PPOBJ_GOODSSTRUC
	#define GSPRP_EXTITEMSTR     1L // Формулы для количества элементов структуры
#define PPOBJ_FORMULA            25L // # Формулы
#define PPOBJ_SCALE              26L // $ Электронные весы
	//
	// Extra record of PPOBJ_SCALE
	#define SCLPRP_EXPPATHS      1L
#define PPOBJ_REGISTERTYPE       27L // $ Типы регистрационных документов
	#define PPREGT_PASSPORT      1 // Паспорт
	#define PPREGT_REGFREE       2 // Свид частного предпринимателя //
	#define PPREGT_TPID          3 // ИНН
	#define PPREGT_OKONH         4 // ОКОНХ
	#define PPREGT_OKPO          5 // ОКПО
	#define PPREGT_BIC           6 // БИК (банковский идентификатор, бывший МФО)
	#define PPREGT_BNKCORRACC    7 // Корр сч банка
	// Вообще говоря, логичнее было бы корр счет держать среди банковких
	// счетов соответствующего банка, однако ввиду устоявшегося статуса
	// непременного атрибута любого банка, этот корр счет будем хранить
	// среди регистрационных данных.
	//
	#define PPREGT_CODE          8 // Код персоналии
	#define PPREGT_ALBATROSCLID  9 // Идентификатор клиента проекта альбатрос
	#define PPREGT_UHTTCLID     PPREGT_ALBATROSCLID // Идентификатор клиента проекта Universe-HTT
	#define PPREGT_KPP          10 // Код причины постановки на налоговый учет налогоплательщика
	#define PPREGT_OKATO        11 // Код фискального органа по ОКАТО
	#define PPREGT_CERTORGCODE  12 // Код сертифицирующей организации
	#define PPREGT_GLN          13 // GLN, ассоциированный с персоналией (не путать с кодом адреса)
	#define PPREGT_COUNTRYABBR  14 // Аналог аббревиатуры государства для персоналий со статусом PPPRS_COUNTRY
	#define PPREGT_COUNTRYCODE  15 // Аналог кода государства для персоналий со статусом PPPRS_COUNTRY
	#define PPREGT_BANKACCOUNT  16 // Банковский счет (начиная с v9.0.4 банковские счета переносятся в таблицу Register)
	#define PPREGT_DTO_ID       17 // Идентификатор оператора передачи данных (PPPRK_DTO)
	#define PPREGT_TAXSYSTEM    18 // Вид системы налогообложения //
	#define PPREGT_ENALOGID     19 // @v12.0.0 ИД участника документооборота (nalog.ru). До @v12.0.0 применялся только тег PPTAG_PERSON_ENALOGID,
		// однако выяснилось, что у организации может существовать несколько идентификаторов в зависимости от привязки к провайдеру.
		// Регистр решает проблему привязки, поскольку содержит атрибут регистрирующей организации.
		// И я в очередной раз восхищен тупостью всех тех, кто организовывал весь этот бардак.
	//
	// Extra records of PPOBJ_REGISTERTYPE
	#define REGTPRP_FORMAT       1L // Формат вывода регистра
#define PPOBJ_ELINKKIND          28L // $ Виды адресов электронной связи
//
// Зарезервированные виды электронных адресов предназначены для совместимости
// с прежними версиями программы. Они соответствуют полям старой таблицы
// [Obsolete]PersonTbl.
//
	#define PPELK_WORKPHONE      1
	#define PPELK_HOMEPHONE      2
	#define PPELK_ALTPHONE       3
	#define PPELK_FAX            4
	#define PPELK_EMAIL          5
	#define PPELK_MOBILE         6
	#define PPELK_WWW            7
#define PPOBJ_QUOTKIND           29L // $ Виды котировок
	#define PPQUOTK_BASE         1 // Базовая котировка
	#define PPQUOTK_GIFT         2 // Подарочная котировка
	#define PPQUOTK_ESTBOMVALUE  3 // @v11.7.11 Ожидаемая цена, рассчитанная по товарной структуре (комплектация)
#define PPOBJ_PERSONOPKIND       30L // $ Вид персональной операции
	//
	// Extra records of PPOBJ_PERSONOPKIND
	#define POKPRP_EXTRA         1L // Дополнительная запись
	#define POKPRP_ALLOWEDTAGS   2L // Теги
	#define POKPRP_FIRSTCLAUSE 100L // Действия над персоналиями [100..199]
	#define POKPRP_LASTCLAUSE  199L
#define PPOBJ_TAG                31L // $ Теги объектов
#define PPOBJ_GOODSTAX           32L // $ Налоговые группы товаров
	//
	// Extra records of PPOBJ_GOODSTAX
	#define GTGPRP_ENTRIES       1L
#define PPOBJ_BCODEPRINTER       33L // $ Принтеры штрихкодов
	// Extra records of PPOBJ_OPRKIND
	#define BCPPRP_PORTEX    1L // Расширенная строка порта вывода
#define PPOBJ_CURRENCY           34L // Валюты
#define PPOBJ_CURRATETYPE        35L // Типы валютных курсов
#define PPOBJ_OPCOUNTER          36L // Счетчики документов
	#define PPOPCNTR_WORKBOOK    1L // Зарезервированных счетчик для символов обьектов PPObjWorkbook_Pre813
#define PPOBJ_GOODSCLASS         37L // Отраслевые классы товаров
	//
	// Extra records of PPOBJ_GOODSCLASS
	#define GDCPRP_PROPS         1L
	#define GDCPRP_DIMX          2L
	#define GDCPRP_DIMY          3L
	#define GDCPRP_DIMZ          4L
	#define GDCPRP_DIMW          5L
	#define GDCPRP_FORMLIST     10L
#define PPOBJ_GOODSBASKET        40L // $ Корзина товаров
	#define PPGDSBSK_ACNUPD      11L // Зарезервированная корзина для форсированного обновления товаров в асинхронном кассовом модуле
	#define PPGDSBSK_ACNRMV      12L // Зарезервированная корзина для форсированного удаления товаров из асинхронного кассового модуля
#define PPOBJ_BHT                41L // $ BHT-терминалы
	//
	// Extra record of PPOBJ_BHT
	#define BHTPRP_GOODSFILT_    1L
	#define BHTPRP_PATH          2L
	#define BHTPRP_SBIICFG       3L
	#define BHTPRP_SBIICFG2      4L  //
	#define BHTPRP_GOODSFILT2    5L  //
#define PPOBJ_SCARDSERIES        42L // $ Серия персональных карт
	//
	// Extra record of PPOBJ_SCARDSERIES
	#define SCARDSERIES_RULE       1L // Правило для серии персональных карт
	#define SCARDSERIES_TRNOVRRNGS 2L // Массив градаций оборота и соотв. скидок
	#define SCARDSERIES_RULE2      3L // Правило для серии персональных карт (вторая версия)
	#define SCARDSERIES_CCHECKRULE 4L // Правило для чеков
	#define SCARDSERIES_BONUSRULE  5L // Правило расчета бонусов
	#define SCARDSERIES_QKLIST     6L // Список видов котировок, ассоциированных с серией
	#define SCARDSERIES_EXT        7L // Блок расширения данных серии
#define PPOBJ_STYLOPALM          43L // $ PDA Palm
	//
	// Extra records of PPOBJ_STYLOPALM
	#define PLMPRP_PATH          1L
	#define PLMPRP_LOCLIST       2L
	#define PLMPRP_FTPPATH       3L
	#define PLMPRP_QUOTKINDLIST  4L  //
#define PPOBJ_ASSTWROFFGRP       44L // $ Группа списания активов
#define PPOBJ_DRAFTWROFF         45L // $ Конфигурация списания драфт-документов
	//
	// Extra record of PPOBJ_DRAFTWROFF
	#define DWOPRP_ORDER         1L
#define PPOBJ_ADVBILLKIND        46L // $ Виды первичных документов (например, для авансовых отчетов)
#define PPOBJ_TRANSPMODEL        47L // # Модели транспортных средств
#define PPOBJ_INTERNETACCOUNT    48L // $ Почтовые учетные записи
	//
	// Extra record of PPOBJ_INTERNETACCOUNT
	#define MACPRP_EXTRA         1L
#define PPOBJ_PERSONRELTYPE      49L // $ Тип отношения между персоналиями
	#define PPPSNRELTYP_AFFIL        1 // Филиал-Головная организация //
	#define PPPSNRELTYP_UHTTSUBORD   2 // Подчиненная персоналия-Управляющая персоналия  @uhtt
	// @construction #define PPPSNRELTYP_AGENTOF      3 // привязка агента к контрагенту (чаще всего к поставщику)
	//
	// Extra record of PPOBJ_PERSONRELTYPE
	#define PRTPRP_INHREGLIST    1L //
#define PPOBJ_PRSNCATEGORY       50L // # Категории персоналий
#define PPOBJ_BILLSTATUS         51L // $ Статусы документов
#define PPOBJ_TOUCHSCREEN        52L // $ TouchScreen
	#define TSCPRP_GRPLIST        1L // Список групп товаров
#define PPOBJ_DUTYSCHED          53L // $ Расписания дежурств
	#define DSHPRP_LIST           1L // Список дежурств
	#define DSHPRP_POINTLIST      2L // Список точек отсчета
#define PPOBJ_DATETIMEREP        54L // $ Периодичность
#define PPOBJ_LOCPRINTER         55L // $ Принтеры, привязанные к складам
#define PPOBJ_SALCHARGE          56L // $ Схема начисления зарплаты
	#define SCPRP_FORMULA         1L // Формула начисления //
	#define SCPPRP_GRPLIST        2L // Список элементов-членов группы
#define PPOBJ_STAFFCAL           58L // $ Штатный календарь
	#define PPSTCAL_STANDARD     10L // Стандартный календарь.
	#define PPSTCAL_REGULAR      11L // @v11.3.10 Регулярный календарь
#define PPOBJ_NAMEDOBJASSOC      59L // $ Именованные ассоциации объектов
#define PPOBJ_BIZSCORE           60L // Определители бизнес-показателей
	#define BZSPRP_DESCR          1L // Описание показателя, совмещенное с формулой
#define PPOBJ_GLOBALUSERACC      61L // Учетная запись глобального пользователя (внешнего по отношению к текущей базе данных)
#define PPOBJ_GOODSVALRESTR      62L // Ограничения товарных величин
	#define GVRPROP_TEXT          1L // Формулы
	#define GVRPROP_BAR           2L // Список ограничений по статьям документа
#define PPOBJ_PALLET             63L // Складские паллеты
#define PPOBJ_DEBTDIM            64L // Размерность расчета долгов по документам
	#define DBTDIMPRP_AGENTLIST   1L // Список агентов
#define PPOBJ_EVENTTOKEN         65L // Специальный объект, используемый как уникальная метка для событий.
	#define PPEVTOK_CVTSCS9809      1L // Событие конвертации серий персональных карт v9.8.9
	#define PPEVTOK_OBJHIST9811     2L // Событие переключения техники хранения версий объектов
	#define PPEVTOK_CVTCMDGROUP     3L // Событие переключения форматов идентификации пользовательских меню и рабочих столов
	#define PPEVTOK_CVTCLIAGT11200  4L // @v11.2.0 Событие переключения техники хранения версий объектов
#define PPOBJ_SMSPRVACCOUNT      66L // Учетная запись провайдера рассылки SMS
	//
	// Extra record of PPOBJ_SMSPRVACCOUNT
	#define SMACCPRP_EXTRA		 1L
#define PPOBJ_PHONESERVICE       67L // Оборудование компьютерной телефонии
	#define PHNSVCPRP_TAIL       1L // Строковый "хвост" пакета телефонного сервиса
#define PPOBJ_GTACTION           68L // # События, регистрируемые в журнале тарфицируемых операций глобальных аккаунтов
#define PPOBJ_SENDSMS		     69L // Отправка смс
	//
	// Extra record of PPOBJ_SENDSMS
	#define SENDSMS_EXTRA		 1L
#define PPOBJ_UHTTSTORE          70L // Интернет-магазин Universe-HTT
	#define UHTTSPRP_SELECTORS   1L  // Список селекторов
#define PPOBJ_GENERICDEVICE      71L // Обобщенное устройство
	#define GENDVCPRP_EXTSTRDATA 2L  // Строка дополнительных данных
#define PPOBJ_WORKBOOK_PRE813    72L // элемент сайта
//
// Функциональные единицы, являющиеся объектами замера производительности. Все значения являются зарезервированными
// и их идентификаторы не могут быть изменены.
//
#define PPOBJ_USERPROFILE        73L // @v8.0.6
	#define USERPROFILEPPRP_FILEINFO   1L
	#define USERPROFILEPPRP_FILEINFO2  2L

	#define PPUPRF_LOGIN                        1
	#define PPUPRF_SESSION           	  	    2
	#define PPUPRF_BILLTURN_RCPT     	  	    3
	#define PPUPRF_BILLTURN_EXP      	  	    4
	#define PPUPRF_BILLTURN_IEXP     	  	    5
	#define PPUPRF_BILLTURN_MOD      	  	    6
	#define PPUPRF_BILLTURN_RVL      	  	    7
	#define PPUPRF_BILLTURN_ORD      	  	    8
	#define PPUPRF_BILLTURN_DRFT     	  	    9
	#define PPUPRF_BILLTURN_RET      	  	   10
	#define PPUPRF_BILLTURN_ETC      	  	   11
	#define PPUPRF_BILLUPD_RCPT      	  	   12
	#define PPUPRF_BILLUPD_EXP       	  	   13
	#define PPUPRF_BILLUPD_IEXP      	  	   14
	#define PPUPRF_BILLUPD_MOD       	  	   15
	#define PPUPRF_BILLUPD_RVL       	  	   16
	#define PPUPRF_BILLUPD_ORD       	  	   17
	#define PPUPRF_BILLUPD_DRFT      	  	   18
	#define PPUPRF_BILLUPD_RET       	  	   19
	#define PPUPRF_BILLUPD_ETC       	  	   20
	#define PPUPRF_BILLRMV_RCPT      	  	   21
	#define PPUPRF_BILLRMV_EXP       	  	   22
	#define PPUPRF_BILLRMV_IEXP      	  	   23
	#define PPUPRF_BILLRMV_MOD       	  	   24
	#define PPUPRF_BILLRMV_RVL       	  	   25
	#define PPUPRF_BILLRMV_ORD       	  	   26
	#define PPUPRF_BILLRMV_DRFT      	  	   27
	#define PPUPRF_BILLRMV_RET       	  	   28
	#define PPUPRF_BILLRMV_ETC       	  	   29
	#define PPUPRF_VIEW_GREST        	  	   51
	#define PPUPRF_VIEW_GREST_DT     	  	   52
	#define PPUPRF_VATFREEPERSONLIST 	  	   53
	#define PPUPRF_PSALBLDGOODS      	  	   54
	#define PPUPRF_PSALBLDGOODSTEST  	  	   55
	#define PPUPRF_GOODSPUT          	  	   56
	#define PPUPRF_CCHECKPUT         	  	   57
	#define PPUPRF_GETMTXLISTBYLOC   	  	   58
	#define PPUPRF_CSESSTXMRESTORE   	  	   59
	#define PPUPRF_PACKCACHETEXT     	  	   60
	#define PPUPRF_MTXCACHEACTUALIZE 	  	   61
	#define PPUPRF_SCARDUPDBYRULE    	  	   62
	#define PPUPRF_GETINVSTOCKREST   	  	   63
	#define PPUPRF_VIEW_GOODSOPANLZ  	  	   64
	#define PPUPRF_VIEW_TRFRANLZ     	  	   65
	#define PPUPRF_INVENTAUTOBUILD   	  	   66
	#define PPUPRF_CALCARTDEBT       	  	   67
	#define PPUPRF_VIEW_DEBT         	  	   68
	#define PPUPRF_GETOPENEDLOTS     	  	   69
	#define PPUPRF_BHTPREPBILL       	  	   70
	#define PPUPRF_BHTPREPGOODS      	  	   71
	#define PPUPRF_OSMXMLPARSETAG    	  	   72
	#define PPUPRF_BUILDGOODSFL      	  	   73
	#define PPUPRF_SRCHINGOODSFL     	  	   74
	#define PPUPRF_TIMSERWRITE       	  	   75
	#define PPUPRF_TSEVALSTAKES      	  	   76
	#define PPUPRF_STYLOQ_CMD_ORDERPREREQ 	   77 // @v11.2.12
	#define PPUPRF_STYLOQ_CMD_INDOORSVCPREREQ  78 // @v11.4.4
	#define PPUPRF_STYLOQ_CMD_ATTENDANCEPREREQ 79 // @v11.4.4
	#define PPUPRF_EGAISMARKAUTOSELECTOR       80 // @v12.2.11 Локальный вызов функции EgaisMarkAutoselector
	#define PPUPRF_EGAISMARKAUTOSELECTOR_SVR   81 // @v12.2.11 Серверный вызов функции EgaisMarkAutoselector
	#define PPUPRF_LAST                        82 // @anchor Идентификатор для отмера количества существующих значений
#define PPOBJ_EDIPROVIDER	     74L // Провайдеры EDI-обмена
	#define EDIPRVPRP_EXTSTRING       1L // Текстовые данные пакета
#define PPOBJ_USREXCLRIGHTS      75L // Исключения для пользовательских прав доступа при некоторых действиях
#define PPOBJ_STAFFLIST2         76L // Штатное расписание (2-я версия объекта - записи перемещены из
	// специализированной таблицы StaffList в Reference
#define PPOBJ_ACCOUNT2           77L // Бухгалтерские счета (балансовые) (2-я версия объекта - записи перемещены из
	// специализированной таблицы Account в Reference
#define PPOBJ_TIMESERIES         78L // Временные серии. Специализированный объект, служащий в качестве заголовков временных серий данных.
	#define TIMSERPRP_STAKEMODEL       1L
	#define TIMSERPRP_STAKEMODEL_SEL   1L
	#define TIMSERPRP_STAKEMODEL_ALL   2L
	#define TIMSERPRP_STAKEMODEL_SELTF 3L
	#define TIMSERPRP_EXTENSION       10L
	#define TIMSERPRP_DENSITYMAP      11L

#define PPOBJ_FREIGHTPACKAGETYPE 79L // Тип транспортной упаковки
#define PPOBJ_TAXSYSTEMKIND      80L // Вид системы налогообложения //
	#define TAXSYSK_GENERAL            1L // RU: ОСН — общая система налогообложения
	#define TAXSYSK_SIMPLIFIED         2L // RU: УСН — упрощенная система налогообложения (доход)
	#define TAXSYSK_PATENT             3L // RU: ПАТЕНТ — патентная система налогообложения
	#define TAXSYSK_IMPUTED            4L // RU: ЕНВД — вмененная система налогообложения или единый налог на вмененный доход
	#define TAXSYSK_SINGLEAGRICULT     5L // RU: ЕСХН — единый сельскохозяйственный налог
	#define TAXSYSK_SIMPLIFIED_PROFIT  6L // RU: УСН — упрощенная система налогообложения (доход-расход)
#define PPOBJ_TSSMODEL           81L // Параметры модели расчета стратегий на временных сериях
	#define TSSMODELPRP_EXTENSION      1L
#define PPOBJ_EVENTSUBSCRIPTION  82L // Подписка на события //
	#define EVNTSUBSCRPRP_EXTENSION    1L
#define PPOBJ_BIZSCORE2          83L // @v11.9.1 @construction 2-я версия определителей бизнес-показателей
	#define BIZSC2PRP_EXTENSION        1L // @v12.3.3 Запись подробностей о бизнес-показателе
#define PPOBJ_SWPROGRAMCATEGORY  84L // @v11.9.12 Категория исполняемой программы. Простой вспомогательный справочник для объекта PPOBJ_SWPROGRAM
#define PPOBJ_COMPUTERCATEGORY   85L // @v12.0.1 Категория компьютера. Простой вспомогательный справочник для объекта PPOBJ_COMPUTER
	#define PPCOMPCAT_DEFAULT          1L // @v12.0.1 Зарезервированная запись категории компьютеров default
#define PPOBJ_RAWMATERIAL        86L // @v12.0.10 Сырьевые материалы. Объект введен из-за необходимость рассчитывать параметры классифицированных товаров.
#define PPOBJ_SALESRESTRICTION   87L // @v12.3.7  Ограничение на продажу товара.
#define PPOBJ_DYNAMICOBJS  100L // Список динамических объектов
//
// Отраслевые справочники
//
#define PPOBJ_WOODBREED    101L // Породы древесины
#define PPOBJ_SLVP         102L // @fantom Поправки объемов бревен
	#define SLVPPRP_VALUES       1L
//
// Следующий идентификатор объекта имеет специальное назначение и
// не соответствует какому-либо реальному объекту.
//
#define PPOBJ_UNASSIGNED    1000L
//
// Следующие объекты хранятся в таблицах, отличных от Reference.
// Поэтому их коды начинаются с PP_FIRSTUSROBJ.
//
#define PPOBJ_COUNTRY       1001L  // @obsolete Государства
#define PPOBJ_REGION        1002L  // @obsolete Регионы
#define PPOBJ_CITY          1003L  // @obsolete Населенные пункты
#define PPOBJ_PERSON        1004L  // Персоналии
	//
	// Extra records of PPOBJ_PERSON
	#define PSNPRP_INFO         10L  // Дополнительная запись, зависящая от вида персоналии
	// @v11.9.1 #define PSNPRP_REGFREE      11L  // Свид частн предпринимателя Obsolete @v2.2.4
	// @v11.9.1 #define PSNPRP_REGBANK      12L  // Доп информация о банке     Obsolete @v2.2.4
	// @v11.9.1 #define PSNPRP_MEMO         13L  // Примечание                 Obsolete @v2.2.4
	#define PSNPRP_ELINK        14L  // Адреса электронной связи
	#define PSNPRP_EXTSTRDATA   15L  // Строка дополнительной информации
	#define PSNPRP_DLVRLOCLIST  16L  // Список адресов доставки (PPIDArray)
	#define PSNPRP_CASHIERINFO  17L  // Права и пароль кассира
	#define PSNPRP_ACTIVITYSTAT 18L  // @v12.2.2 Статистика клиенской активности персоналии
#define PPOBJ_ARTICLE       1006L  // Статьи аналитического учета
	//
	// Extra records of PPOBJ_ARTICLE (Account's articles)
	//
	#define ARTPRP_CLIAGT        1L  // Соглашение с клиентом об условиях торговли
	#define ARTPRP_ALIASSUBST    2L  // Подстановка алиасов счетов
	#define ARTPRP_SUPPLAGT      3L  // Соглашение с поставщиком об условиях торговли
	#define ARTPRP_SUPPLAGT_EXCH 4L  // Конфигурация обмена для данного поставщика
	#define ARTPRP_DEBTLIMLIST   5L  // Список долговых лимитов
	#define ARTPRP_DEBTLIMLIST2  6L  // @v7.1.5 Список долговых лимитов версии 7.1.5 и выше
	#define ARTPRP_SUPPLAGT2     7L  // @v8.5.0 Новый формат хранения соглашения с поставщиком
	#define ARTPRP_CLIAGT2       8L  // @v11.2.0 Новый формат соглашения с клиентом об условиях торговли
	//
	// @v12.1.3
	// Зарезервированные номера статей для специализированных случаев
	// Символы с префиксом ARTN_MRKTPLCACC_ обозначают зарезервированные номера статей расходов и доходов в расчетах с маркетплейсом.
	// Наименования этих статей перечислены в строке PPTXT_ACCSHEET_MP_AR_NAMES. Статьи создаются автоматически в модуле marketplace.cpp
	//
	#define ARTN_MRKTPLCACC_COMMISSION   1 // Комиссионное вознаграждение
	#define ARTN_MRKTPLCACC_ACQUIRING    2 // Стоимость эквайринга
	#define ARTN_MRKTPLCACC_STORAGE      3 // Стоимость хранения
	#define ARTN_MRKTPLCACC_ACCEPTANCE   4 // Стоимость приемки на склад хранения
	#define ARTN_MRKTPLCACC_CORRECTION   5 // Коррекция баланса
	#define ARTN_MRKTPLCACC_DEDUCTION    6 // Удержания //
	#define ARTN_MRKTPLCACC_PENALTY      7 // @v12.2.0 Штрафы
#define PPOBJ_ACCOUNT_PRE9004 1007L  // Бухгалтерские счета (балансовые) // @v9.0.4 Заменен на PPOBJ_ACCOUNT2
	//
	// Extra records of PPOBJ_ACCOUNT
	//
	#define ACCPRP_GENACCLIST    1L // Список группируемых счетов
#define PPOBJ_GOODSGROUP    1008L  // Группы товаров
#define PPOBJ_GOODS         1009L  // Товары
	//
	// Extra records of PPOBJ_GOODSGROUP and PPOBJ_GOODS (Goods groups and Goods)
	//
	#define GGPRP_GOODSFLT_      1L  // Фильтр товаров для альтернативной группы (вариант до v8.6.4)
	#define GDSPRP_EXTSTRDATA    2L  // Строка дополнительных данных
	#define GDSPRP_STOCKDATA     3L  // Параметры товара для заказа, транспортировки и хранения //
	#define GGPRP_EXSTRTITLES    4L  // Заголовки дополнительных полей товара.
	#define GDSPRP_PICTURE       5L  // Картинка
	#define GDSPRP_MINSTOCKBYLOC 6L  // Минимальный запас товара по складам
	#define GGPRP_GOODSFILT2     7L  // Фильтр товаров для альтернативной группы (сериализованный вариант)
#define PPOBJ_LOCATION      1010L  // Позиции @store(LocationTbl)
	#define LOCPRP_WAREHOUSELIST 1L // @v11.0.6 Список складов, ассоциированных с подразделением главной организации
	// @v12.1.2 // Зарезервированные позиции (не используются)
	// @v12.1.2 #define PPLOC_RECEIPT        1L // @unused Поступление товара из-вне
	// @v12.1.2 #define PPLOC_EXPEND         2L // @unused Расход товара
#define PPOBJ_BILL          1011L  // Документы @store(BillTbl)
	//
	// Extra records of PPOBJ_BILL
	//
	#define BILLPRP_EXTRA        1L  // Дополнительная запись
	#define BILLPRP_RENT         2L  // Условия ренты по документу
	#define BILLPRP_PAYMORDER    3L  // Платежное поручение
	#define BILLPRP_FREIGHT      4L  // Фрахт
	#define BILLPRP_ADVANCEREP   5L  // Авансовый отчет
	#define BILLPRP_LINKFILES    6L  // Привязанные файлы
	#define BILLPRP_GUID         7L  // GUID, в частности, для документов с BHT терминала
	#define BILLPRP_DRAFTTAGLIST 8L  // Список тегов, привязанных к строкам драфт-документа.
		// При списании приходного драфт-документа этот список привязывается к лотам создаваемого документа
	#define BILLPRP_AGREEMENT    9L  // Блок, хранящий данные соглашения (договора) с контрагентом. Сопоставляется с документами, имеющими тип операции PPOPT_AGREEMENT
	#define BILLPRP_VALXCL      10L  // Контейнер валидации расширенных кодов
#define PPOBJ_ACCTURN       1012L  // Бухгалтерские проводки
#define PPOBJ_QCERT         1013L  // Сертификаты качества
#define PPOBJ_PRICELIST     1014L  // Прайс-листы
	//
	// Extra record of PPOBJ_PRICELIST
	#define PLPRP_GDSGRPLIST     1L // @v4.6.11
#define PPOBJ_VATBOOK       1015L  // Книги продаж и покупок
#define PPOBJ_REGISTER      1017L  // Регистрационные документы
#define PPOBJ_PERSONEVENT   1018L  // Персональные события //
#define PPOBJ_INVENTORY     1019L  // Инвентаризация //
#define PPOBJ_STAFFLIST_PRE9003 1021L  // Штатное расписание
	#define SLPPRP_AMTLIST     1L // список сумм для штатного расписани
#define PPOBJ_LOT           1022L  // Лоты
#define PPOBJ_CSESSION      1023L  // Кассовые сессии
#define PPOBJ_ACCTREL       1024L  // !PPObject Бухгалтерские связки
#define PPOBJ_OBJASSOC      1025L  // !PPObject Ассоциации объектов
#define PPOBJ_PACKAGE       1026L  // !PPObject Товарные пакеты
#define PPOBJ_PCKGTYPE      1027L  // Types of goods packages
#define PPOBJ_TRANSPORT     1028L  // Транспортные средства
#define PPOBJ_QUOT          1029L  // !PPObject Котировки
#define PPOBJ_GOODSREST     1030L  // !PPObject Остатки товаров (синхронизируется)
#define PPOBJ_SCARD         1031L  // Персональные карты
#define PPOBJ_CCHECK        1032L  // !PPObject Кассовые чеки
#define PPOBJ_PROJECT       1033L  // Проекты  @store(ProjectTbl)
#define PPOBJ_BRAND         1034L  // Бренды   @store(Goods2Tbl)
#define PPOBJ_MRPTAB        1035L  // MRP-таблицы
#define PPOBJ_PRJTASK       1036L  // Задачи по проекту
#define PPOBJ_PROCESSOR     1037L  // Процессоры
	#define PRCPRP_EXT           1L // Структура расширения пакета процессора
	#define PRCPRP_EXT2          2L // Структура расширения пакета процессора (новая версия с сериализацией)
#define PPOBJ_TECH          1038L  // Технологии
	#define TECPRP_EXTSTR        1L  // Строка дополнительных параметров
#define PPOBJ_TSESSION      1039L  // Процессорные сессии
	//
	// Extra record of PPOBJ_TSESSION
	#define TSESPRP_WROFFORDER   1L // Порядок списания техн сессий (ID объекта - 0)
	#define TSESPRP_EXT          2L // Расширение пакета сессии (PPProcessorPacket::ExtBlock)
#define PPOBJ_DFCREATERULE  1040L // Правила для создания драфт документа
	//
	// Exra record of PPOBJ_DFCREATERULE
	#define DFCRRULPRP_CASHNN    1L
#define PPOBJ_BILLSYMB      1041L  // !PPObject Символы формул сумм документов (используется только для организации кэша)
#define PPOBJ_DBXCHG        1042L  // !PPObject Обмен данными между разделами (только для блокировки приема разными пользователями)
#define PPOBJ_WORLD         1043L  // Географические объекты
#define PPOBJ_GOODSINFO     1044L  //
#define PPOBJ_PERSONPOST    1045L  //  Должностные назначения //
	#define PSNPPPRP_AMTLIST     1L // Суммы для данного должностного назначения //
#define PPOBJ_DESKTOP       1046L  // !PPObject Рабочий стол. Нужен для сохранения логотипа рабочего стола и некоторых свойств в PropertyTbl
#define PPOBJ_SERIAL        1047L  // !PPObject Ассоциации серийных номеров с уникальными идентификаторами.
	// Используется для хранения привязки серийных номеров к записям группировка строк кассовых чеков (CGoodsLine)
	// @store(ObjTagTbl)
#define PPOBJ_TSALESBUILD   1048L  // !PPObject Построение таблицы продаж
#define PPOBJ_DL600DATA     1049L  // !PPObject Структура данных DL600
#define PPOBJ_QUOT2REL      1050L  // !PPObject Группирующие характеристики второй реализации котировок
#define PPOBJ_QUOT2         1051L  // !PPObject
#define PPOBJ_GOODSARCODE   1052L  // !PPObject
#define PPOBJ_BILLEXT       1053L  // !PPObject Используется для кэширования расширения документов
#define PPOBJ_BUDGET        1054L  // Бюджеты
#define PPOBJ_BIZSCTEMPL    1055L  // Шаблоны бизнес показателей
	#define PPPRP_BIZSCTEMPLCOLS  1L // Колонки шаблона бизнес-показателей
	#define PPPRP_BIZSCTEMPLROWS  2L // Строки шаблона бизнес-показателей
	#define PPPRP_BIZSCTEMPLCELLS 3L // Ячейки шаблона бизнес-показателей
#define PPOBJ_CAFETABLE     1056L  // Столы HORECA // Since @v8.4.1 Real Object
#define PPOBJ_RFIDDEVICE    1057L  // PPObject rfid устройства
#define PPOBJ_SPECSERIES    1058L  //
#define PPOBJ_EADDR         1059L  //
#define PPOBJ_GTA           1060L  // !PPObject Специальный тип для обозначения тарифицируемых событий глобальных учетных записей.
#define PPOBJ_CURRATEIDENT  1061L  // !PPObject @uhtt Значение курса валюты
#define PPOBJ_UHTTSCARDOP   1062L  // !PPObject @uhtt Используется для отображения информации о карте
#define PPOBJ_TAGVALUE      1063L  // !PPObject Специальный тип для идентификации значений тегов объектов
#define PPOBJ_CHKINP        1064L  // !PPObject Специальный тип для персональных регистраций
	#define PPPRP_CHKINP_MEMO     1L // Строка примечания к персональной регистрации
#define PPOBJ_COMPGOODS     1065L  // Товары с составляющими // @vmiller
#define PPOBJ_WORKBOOK      1066L  // рабочие книги (замещает PPOBJ_WORKBOOK_PRE813)
	#define WBPRP_EXTSTRDATA      1L  // Строка расширения записи PPOBJ_WORKBOOK
// @unused #define PPOBJ_KEYWORD       1067L  // Ключевые слова
#define PPOBJ_BILLFREIGHT   1068L  // !PPObject Используется для кэширования фрахтов документов
#define PPOBJ_FIAS          1069L  // Объект, управляющий данными справочника адресов России ФИАС. Не является полноправным PPObject'ом
	#define TXTREF_FIAS_FORMTEXT  (TXTREF_PROP_BIAS+1)
	#define TXTREF_FIAS_OFFCTEXT  (TXTREF_PROP_BIAS+2)
	#define TXTREF_FIAS_PLAINCODE (TXTREF_PROP_BIAS+3)
	#define FIASPRP_IMPORTSTATE_  1L  // @obsolete @v8.6.8 Состояние импорта из внешнего источника.
	#define FIASPRP_IMPORTSTATE2  2L  // Состояние импорта из внешнего источника (версия 2).
#define PPOBJ_UUIDREF      	1070L  // !PPObject Специальное значение для идентификации данных таблицы UuidRef
#define PPOBJ_EGAISREFA     1071L  // Специализированный объект, хранящий данные справок А системы ЕГАИС
#define PPOBJ_EGAISPRODUCT  1072L  // Специализированный объект, хранящий данные о товарах системы ЕГАИС
#define PPOBJ_EGAISPERSON   1073L  // Специализированный объект, хранящий данные о персоналиях системы ЕГАИС
#define PPOBJ_VETISENTITY   1074L  // Специализированный объект, хранящий данные о сущностях системы VETIS
#define PPOBJ_GEOTRACKING   1075L  // !PPObject Специализированный объект для представления гео-треков.
	// Значение фактически применяется только для идентификации запроса к JobServer'у.
#define PPOBJ_EVENT         1076L  // События
	#define PPEVENTTYPE_OBJCREATED        1L // Создан новый объект
	#define PPEVENTTYPE_OUTER             2L // Внешнее по отношению к системе событие. Например, уведомление от какого-либо сервиса
	#define PPEVENTTYPE_SPCBILLCHANGE     3L // Изменение документа каким-либо специфическим образом
	#define PPEVENTTYPE_SYSJOURNAL        4L // Событие в системном журнале по фильтру
	#define PPEVENTTYPE_LOTEXPIRATION     5L // Истечение срока годности лота
	#define PPEVENTTYPE_ORDERCREATED      6L // @v11.5.10 Stylo-Q Создан новый документ заказа
	#define PPEVENTTYPE_TSESSCREATED      7L // @v11.6.6  Stylo-Q Создана новая технологическая сессия (@todo придется поработать над текстовой трактовкой этого события - нормальные люди не поймут) //
#define PPOBJ_STYLOQBINDERY 1077L  // @v11.1.7 Подшивка объектов Stylo-Q
#define PPOBJ_VETISIC       1078L  // @v11.3.2 !PPObject Используется для блокировки функций обмена с сервисами ВЕТИС
	#define PPVETISICFUNC_SETUPOUTGOINGENTRIES 1
	#define PPVETISICFUNC_PROCESSOUTCOMING     2
#define PPOBJ_COMPUTER      1079L  // @v11.7.3  Управляемый компьютер (в рамках проекта WSCTL). Хранится в таблице gooods2 с видом записи PPGDSK_COMPUTER
	#define COMPUTERPRP_SYS       1L // @v12.0.0 Запись свойств управляемого компьютера, хранящая системные параметры (MAC-addr, IP-addr, cpu etc)
#define PPOBJ_SWPROGRAM     1080L  // @v11.9.11 Программа (в рамках проекта WSCTL). Хранится в таблице gooods2 с видом записи PPGDSK_SWPROGRAM
#define PPOBJ_PERSON_CAS    1081L  // @v12.2.2 !PPObject Используется для кэширования статистики активности клиентов
#define PPOBJ_DBCONVERT     1100L  // Специальный объект, используемый для формирования записей
	// с информацией о конвертации базы данных. Запись, идентифицирующая конвертацию имеет идентификатор
	// вида 0xMJMNRR  где MJ - мажор, MN - минор, RR - релиз
	// Например, после выполнения конвертации 6.4.06 появится запись с ИД 0x060406
#define PPOBJ_ADVEVENTQUEUE 1101L   // Специальное значение для идентификации очереди системных событий
#define PPOBJ_BIZSCORE2VAL  1102L   // @v12.3.4 @construction !PPObject значения бизнес-показателей (2-я версия).
#define PPOBJ_SELFREFTEXT   2000L   // Специальное значение для записей в TextRef, представлющих только текст с автоматически назначенным идентификатором
// LastObject
//
// @v11.3.0 Общие (и, иногда, частные) атрибуты объектов. Вводятся для управления техникой поиска данных по текстовым запросам (fulltext index).
// Attention! Чтобы минимизировать разногласия между методами идентификации многие атрибуты имеют значения, совпадающие с константами PPTRPROP_XXX
//   Если добавляете новые значения, то согласуйте их с соответствующим набором констант.
// Attention! (2) Кроме того, эти константы синхронизируются с проектом Stylo-Q (slib.java)
// @persistent
//
#define PPOBJATTR_NAME          1L // Наименование
#define PPOBJATTR_SYMB          2L // Символ
#define PPOBJATTR_LONGNAME      3L // Длинное наименование
#define PPOBJATTR_MEMO          4L
#define PPOBJATTR_RAWADDR       6L
#define PPOBJATTR_DESCR         8L
#define PPOBJATTR_CODE          9L
#define PPOBJATTR_RAWDATA_XML  10L // @v12.4.7 Исходные данные использованные для формирования объекта в xml-формате
#define PPOBJATTR_RAWDATA_JSON 11L // @v12.4.7 Исходные данные использованные для формирования объекта в json-формате
#define PPOBJATTR_ID           21L // Целочисленный уникальный идентификатор
#define PPOBJATTR_FLAGS        22L
#define PPOBJATTR_PHONE        23L
#define PPOBJATTR_EMAIL        24L
#define PPOBJATTR_GLN          25L
#define PPOBJATTR_RUINN        26L
#define PPOBJATTR_RUKPP        27L
#define PPOBJATTR_RUSNILS      28L
#define PPOBJATTR_RAWMAINADDR  29L // @v12.4.10 основной адрес персоналии
#define PPOBJATTR_RAWRADDR     30L // @v12.4.10 фактический адрес персоналии
#define PPOBJATTR_EGAISMARK    31L // @v12.4.10 марка егаис (один объект может иметь множество таких марок)
#define PPOBJATTR_CHZNMARK     32L // @v12.4.10 марка честный знак (один объект может иметь множество таких марок)
#define PPOBJATTR_UUID         33L // @v12.4.10 UUID объекта. Практически возможна ситуация, когда несколько объектов имеют один и тот же UUID,
	// а так же, один объект может иметь несколько UUID'ов
#define PPOBJATTR_GENERICREG   34L // @v12.4.10 Некий регистрационный номер. Может относится к PPOBJ_PERSON, PPOBJ_LOCATION, PPOBJ_PERSONEVENT
//
// Descr: Зарезервированные значения текстовых свойств объектов
// Attention! Чтобы минимизировать разногласия между методами идентификации константы PPOBJATTR_XXX (ppdefs.h) имеют значения, совпадающие с данным набором констант.
// Если добавляете новые значения, то согласуйте их с соответствующим набором констант.
// @v11.3.0 Переместил сюда коды PPTRPROP_XXX с целью согласования с идентификаторами семейства PPOBJATTR_XXX
// @persistent
//
#define PPTRPROP_DEFAULT        0
#define PPTRPROP_NAME           1 // Если у объекта есть и короткое и длинное наименования, то короткое заносится в PPTRPROP_NAME, а длинное - в PPTRPROP_LONGNAME
#define PPTRPROP_SYMB           2 // Единственный уникальный символьный идентификатор объекта (может быть пустым, но если нет, то обязательно уникальным)
#define PPTRPROP_LONGNAME       3
#define PPTRPROP_MEMO           4
#define PPTRPROP_COMBINE        5 // Комбинированная строка, содержащая набор текстовых свойств
#define PPTRPROP_RAWADDR        6 // Простое текстовое представление адреса
#define PPTRPROP_TIMESERIES     7 // Временная серия - специальное свойство, для которого хранится не строка, а STimeSeries.
	// Применяется только для таблицы UnxTextRefTbl (но не TextRefTbl).
	// Для оперирования этим свойством объект UnxTextRefCore имеет несколько специализированных методов. Попытка работать с этим
	// свойством как со строкой инициирует ошибку.
#define PPTRPROP_DESCR          8 // Строка описания (не путать с PPTRPROP_NAME и PPTRPROP_LONGNAME). Используется, например, в проектах и задачах.
#define PPTRPROP_CODE           9 // @v11.3.0 Отличается от SYMB тем, что в общем случае может существовать более одного кода для одного объекта
#define PPTRPROP_RAWDATA_XML   10 // @v12.4.7 Исходные данные использованные для формирования объекта в xml-формате
#define PPTRPROP_RAWDATA_JSON  11 // @v12.4.7 Исходные данные использованные для формирования объекта в json-формате

#define PPTRPROP_USER         100 // Стартовое значение, с которого можно использовать пользовательские идентификаторы текстовых свойств
//
// Зарезервированные теги объектов
//
#define PPTAG_LOT_CLB                 1L // Номер грузовой таможенной декларации (PPOBJ_LOT)
#define PPTAG_LOT_SN                  2L // Серийный номер (PPOBJ_LOT)
#define PPTAG_FLOAT_SERIAL            3L // @obsolete since @v9.0.11 (Заменено на TextRefCore SelfRefText)
	// "Плавающие" серийные номера. Такие серийные номера не привязаны к
	// конкретным объектам, но используются для того, чтreобы хранить произвольные строковые значения серий,
	// ассоциированные и целочисленными идентификаторами. Первоначальное применение - списание кассовых сессий
	// с серийными номерами, при этом таблица CGoodsLine хранить не сами номера, что было бы расточительно,
	// а ссылки на них.
#define PPTAG_PERSON_POSRIGHTS        4L // Права агента на доступ к кассовым операциям.
#define PPTAG_PERSON_DOB              5L // Дата рождения //
#define PPTAG_BILL_CREATEDTM          6L // Дата создания документа. В данный момент используется для отражения информации о начале создания заказа на КПК.
#define PPTAG_BILL_CREATEDTMEND       7L // Дата создания документа. В данный момент используется для отражения информации о окончании создания заказа на КПК.
#define PPTAG_BILL_GPSCOORD           8L // GPS координаты агента при начале создания заказа на КПК.
#define PPTAG_BILL_GPSCOORDEND        9L // GPS координаты агента при окончании создания заказа на КПК.
#define PPTAG_GUA_GOODSRIGHTS        10L // Права глобальной учетной записи на доступ/изменение товара
#define PPTAG_GUA_PERSONRIGHTS       11L // Права глобальной учетной записи на доступ/изменение персоналий
#define PPTAG_GUA_SCARDRIGHTS        12L // Права глобальной учетной записи на доступ/изменение пластиковых карт
#define PPTAG_GUA_SCARDPREFIX        13L // Префикс номеров пластиковых карт
#define PPTAG_LOT_MANUFTIME          14L // Дата/время производства товара, оприходованного лотом.
#define PPTAG_GUA_FILESRIGHTS        15L // Права глобальной учетной записи на доступ/изменение файлов
#define PPTAG_BILL_UUID              16L // UUID документа
#define PPTAG_GOODS_KEYWORDS         17L // Ключевые слова товара
#define PPTAG_BILL_KEYWORDS          18L // Ключевые слова документа
#define PPTAG_GUA_PGGTAG             19L // Символ тега приватной группы товара для данной глобальной учетной записи
#define PPTAG_WORKBOOK_KEYWORDS      20L // Ключевые слова рабочих книг
#define PPTAG_WORKBOOK_KWSYN         21L // Синонимы ключевых слов рабочих книг
#define PPTAG_WORKBOOK_KWLOC         22L // Локации, ассоциированные с ключевыми словами рабочих книг (пока - наименования городов)
#define PPTAG_CCHECK_JS_UUID         23L // UUID сессии, создавшей JUNK-чек (для того, чтобы можно было быстро восстановить чек после сбоя сеанса)
#define PPTAG_GUA_SALOCRIGHTS        24L // Права глобальной учетной записи на доступ/изменение к автономным адресам
#define PPTAG_GUA_WBRIGHTS           25L // Права глобальной учетной записи на доступ/изменение к рабочим книгам
#define PPTAG_WORKBOOK_LONGCODE      26L // Длинный уникальный код записи рабочей книги (уникальность обеспечивается программно,
	// поскольку соответствующий индекс в таблице ObjTag дублируемый
#define PPTAG_BILL_AUTOCREATION      27L // Текстовая метка специальной структуры, помечающая автоматически созданные документы.
	// Основное назначение - предотвращение дублирования документов при автоматическом формировании.
#define PPTAG_BILL_OUTERCODE         28L // Номер документа у контрагента
#define PPTAG_BILL_OUTERDATE         29L // Дата документа у контрагента
#define PPTAG_PERSON_ENALOGDEST      30L // Код назначения для отправки электронных налоговых документов (для главных организаций)
#define PPTAG_LOC_FIASGUID_ADR       31L // Идентификатор терминального адреса по справочнику ФИАС
#define PPTAG_LOC_FIASGUID_HSE       32L // Идентификатор здания по справочнику ФИАС
#define PPTAG_GUA_STYLORIGHTS        33L // Права глобальной учетной записи на доступ/изменение записей устройств Stylo
#define PPTAG_GUA_PRCRIGHTS          34L // Права глобальной учетной записи на доступ/изменение записей процессоров
#define PPTAG_GUA_TSESSRIGHTS        35L // Права глобальной учетной записи на доступ/изменение технологических сессий
#define PPTAG_TSESS_UUID             36L // UUID технологической сессии
#define PPTAG_TSESS_DESCR            37L // Описание технологической сессии
#define PPTAG_LOC_FSRARID            38L // Идентификатор склада в системе ЕГАИС
#define PPTAG_PERSON_FSRARID         39L // Идентификатор организации в системе ЕГАИС
// #define PPTAG_GOODS_FSRARCODE       40L // Код товара в системе ЕГАИС
#define PPTAG_LOT_FSRARINFA          41L // Номер справки А на алкогольную продукцию
#define PPTAG_LOT_FSRARINFB          42L // Номер справки Б на алкогольную продукцию
#define PPTAG_LOT_FSRARLOTGOODSCODE  43L // Код товара ЕГАИС привязанный к лоту. Так как один товар может соответствовать
	// нескольким кодам ЕГАИС для того, чтобы исключить рассогласования, в этот тег записывается код ЕГАИС оприходованного товара
#define PPTAG_BILL_EDIACK            44L // По каналу EDI отправлено подтверждение на импортированный документ, либо получено
	// подтверждение на экспортированный. Если в записи установлен флаг BILLF2_ACKPENDING, то это означает, что было
	// отправлено уведомление о получении импортированного документа, но ответ по каналу EDI еще был не получен.
#define PPTAG_BILL_EDIIDENT          45L // Идентификатор документа у провайдера EDI (обычно отличается и от собственных номеров/идентификаторов и от оных у контрагента).
#define PPTAG_LOC_EGAISSRVURL        46L // URL сервера ЕГАИС (это значение переопределяет URL заданный в PPAlbatrossConfig)
#define PPTAG_LOT_EGIASINFAREG       47L // Подробная информация о справке А (сжатая в одну строку)
#define PPTAG_PERSON_EGAISSRVURL     48L // URL сервера ЕГАИС (это значение переопределяет URL заданный в PPAlbatrossConfig)
#define PPTAG_BILL_EDIRECADVCONFIRM  49L // Номер отправленного по каналу EDI нашего согласия/не согласия с полученным RECADV
#define PPTAG_LOT_DIMENSIONS         50L // Размерности товара, ассоциированные с лотом (для изделий неправильной/нестандартной формы) // @v11.7.0 @fix PPTAG_LOT_DIMENTIONS-->PPTAG_LOT_DIMENSIONS
#define PPTAG_BILL_EDIREJECTACK      51L // По каналу EDI отправлено требование отменить ранее совершенную передачу документа
#define PPTAG_GOODS_OUTERPOSCODE     52L // (в ppdata.rc не добавлять) Код товара у источника данных для POS-узла
#define PPTAG_BILL_EDICHANNEL        53L // Символьное обозначение EDI-канала, по которому был доставлен документ в базу данных.
	// Этот тег может быть автоматически установлен у документов, имеющих не нулевой признак BillTbl::Rec::EdiOp
	// Ручная установка этого тега крайне не желательна.
#define PPTAG_BILL_EDIREPEALACK      54L // По каналу EDI Papyrus'ом отправлен запрос на отмену проведения документа.
	// Этот тег содержит подтверждение о получении сервером EDI такого запроса.
#define PPTAG_BILL_EDIPROBLEMTRFR    55L // Текстовый тег, перечисляющий проблемные номера товарных строк в формате: number[:descr][;number[:descr]]*
#define PPTAG_LOT_SOURCESERIAL       56L // Серийный номер исходного лота для данной строки. Применяется в документах модификации.
#define PPTAG_PERSON_UUID            57L // UUID персоналии
#define PPTAG_LOC_UUID               58L // UUID локации
#define PPTAG_BILL_FORMALREASON      59L // Формальное основание документа (перечисление)
#define PPTAG_BILL_EDIREPEALREQ      60L // Запрос на отмену проведения документа EDI
#define PPTAG_POSNODE_UUID           61L // UUID кассового узла
#define PPTAG_POSNODE_HOSTUUID       62L // UUID управляющего кассового узла (для синхронных кассовый узлов Papyrus)
#define PPTAG_BILL_STATECONTRACTID   63L // ИД государственного контракта (для счетов-фактур)
#define PPTAG_PERSON_ENALOGID        64L // ИД участника документооборота (nalog.ru)
#define PPTAG_BILL_COMPLEMENTARY     65L // Формальная текстовая ссылка на комплементарный (парный) документ.
	// Теоретически, использоваться может достаточно широко, однако непосредственным поводом для ввода тега
	// послужила необходимость при передаче в ЕГАИС акта постановки на баланс по пересортице (PPEDIOP_EGAIS_ACTCHARGEON_V2)
	// передавать ссылку на соответствующий документ потерь (PPEDIOP_EGAIS_ACTWRITEOFF_V2).
#define PPTAG_PERSON_VETISUUID       66L // UUID предприятия в системе Меркурий (интерфейс ВЕТИС) // в терминах ВЕТИС - business-entity GUID (не UUID)
#define PPTAG_GUA_LOGIN              67L // Строка авторизации для глобальной учетной записи. Необходим в том случае,
	// если эта строка не может быть помещена в наименование записи из-за нарушения требования уникальности либо максимальной длины.
#define PPTAG_GOODS_UUID             68L // UUID товара
#define PPTAG_BILL_EDIRECADVRCV      69L // Информация о RECADV полученном в ответ на отправленный DESADV
#define PPTAG_BILL_EDIDESADVSENT     70L // Информация об отправленном DESADV
#define PPTAG_BILL_EDIORDRSPSENT     71L // Информация об отправленном ORDRSP
#define PPTAG_BILL_EDIALCDESADVSENT  72L // Информация об отправленном ALCODESADV
#define PPTAG_BILL_EDIINVOICSENT     73L // Информация об отправленном INVOIC
#define PPTAG_PERSON_VETISISSUUID__unused   74L // Vetis IssuerUUID предприятия в системе Меркурий (интерфейс ВЕТИС)
#define PPTAG_PERSON_VETISUSER       75L // Vetis initiator в системе Меркурий (интерфейс ВЕТИС)
#define PPTAG_LOT_VETIS_UUID         76L // ВЕТИС-идент входящей (произведенной) партии
#define PPTAG_LOC_VETIS_GUID         77L // ВЕТИС-идент локации (склад или адрес доставки) // в терминах ВЕТИС - enterprise GUID (не UUID)
#define PPTAG_GUA_SECRET             78L // Строка секрета авторизации для глобальной учетной записи. Необходим в том случае,
	// если эта строка не может быть помещена в штатный пароль записи из-за нарушения требования максимальной длины, либо по каким-то иным причинам.
#define PPTAG_GOODS_VETISPURPOSE     79L // Цель отгрузки товара для ВЕТИС. Размещение этого признака в товаре
	// спорно, однако, на момент создания это - быстрое решение проблемы автоматической дифференциации целей отгрузки
	// в зависимости от товара.
// @postponed #define PPTAG_LOC_VETIS_ENTITYGUID  80L // GUID предприятия в системе Меркурий (интерфейс ВЕТИС) // в терминах ВЕТИС - business-entity GUID (не UUID). Имеет приоритет перед PPTAG_PERSON_VETISUUID
// @postponed #define PPTAG_LOC_VETISUSER         81L // Vetis initiator в системе Меркурий (интерфейс ВЕТИС). Имеет приоритет перед PPTAG_PERSON_VETISUSER
#define PPTAG_GOODS_TRANSPVANTYPE    82L // Тип транспортного фургона для отправки данных в Меркурий
#define PPTAG_BILL_EDIORDERSENT      83L // Информация об отправленном ORDER
#define PPTAG_BILL_EDIORDRSPRCV      84L // Информация об ORDRSP, полученном в ответ на отправленный ORDER
#define PPTAG_LOT_ORGLINEIDENT       85L // Оригинальный идентификатор строки документа. Используется в случае, если идент импортированного документа
	// не может быть представлена целым числом [1..30000]
#define PPTAG_LOT_FREIGHTPACKAGE     86L // Транспортная упаковка. Структурированная информация сжата в строку.
#define PPTAG_GOODS_VETISGUID        87L // GUID товара в справочнике ВЕТИС
#define PPTAG_GUA_GUID                  88L // GUID глобальной учетной записи
#define PPTAG_GUA_ACCESSKEY             89L // Ключ доступа к глобальному ресурсу (не путать с паролем или секретом)
#define PPTAG_LOT_LINKBILLROW           90L // Связанная строка документа
#define PPTAG_GUA_CERTSUBJCN            91L // CN субъекта сертификата электронной подписи
#define PPTAG_PERSON_CHZNCODE           92L // Код контрагента в честном знаке
#define PPTAG_LOC_CHZNCODE              93L // Код локации в честном знаке (назначение тоже что и PPTAG_PERSON_CHZNCODE но имеет приоритет при идентификации)
#define PPTAG_GUA_CHZNCODE              94L // Код своего предприятия в честном знаке (назначение тоже что и PPTAG_PERSON_CHZNCODE, PPTAG_LOC_CHZNCODE но имеет меньший приоритет при идентификации)
#define PPTAG_GUA_PROTOCOL              95L // Протокол либо версия протокола обмена с сервисом, к которому привязана запись.
#define PPTAG_GUA_OUTERWAREIDTAG        96L // Ссылка на тег товара, содержащий внешний идентификатор
#define PPTAG_GUA_SOCIALGROUPCODE       97L // Код группы в социальной сети
#define PPTAG_GUA_SOCIALPAGECODE        98L // Код страницы в социальной сети
#define PPTAG_LOC_VETIS_REGIONGUID      99L // @v11.0.10 ВЕТИС-идент региона локации (склад или адрес доставки) // в терминах ВЕТИС - region GUID (не UUID)
#define PPTAG_BILL_LONGCODE            100L // @v11.0.12 (надеюсь, временный тег) Если номер документа, поступившего из-вне длиннее sizeof(Bill:Code)-1 то полный номер сохраняется в этом теге
#define PPTAG_POSNODE_OFDVER           101L // @v11.1.9  Версия ОФД на аппарате (аппаратах), с которой работает кассовый узел
#define PPTAG_LOC_TRUNKPOINT           102L // @v11.2.9  Магистральная точка, ближайшая к адресу. Используется в логистике для планирования маршрутов.
#define PPTAG_TSESS_OUTERCODE          103L // @v11.6.6  Внешний код технологической сессии
#define PPTAG_PERSON_TRANSPORTTO       104L // @v11.6.9  Транспорт, используемый для доставки товара клиенту по-умолчанию (используется для интеграции с VETIS)
#define PPTAG_LOT_ACCEPTANCE           105L // @v11.7.0  Результат "физической" приемки документа. Структурированная информация сжата в строку.
#define PPTAG_PRC_UUID                 106L // @v11.7.2  UUID процессора
#define PPTAG_BILL_LINKCCHECKUUID      107L // @v11.8.8  UUID кассового чека, сформированного по документу
#define PPTAG_BILL_CHZNDISPOSALRSN     108L // @v11.8.12 Признак причины выбытия марки честный знак <СвВыбытияМАРК> (1,2,3,4 or empty)
#define PPTAG_PERSON_CHZN_PM_TOKEN     109L // @v11.9.11 Токен для проверки марок честный знак в разрешительном режиме (привязка к персоналии)
#define PPTAG_GUA_CHZN_PM_TOKEN        110L // @v11.9.11 Токен для проверки марок честный знак в разрешительном режиме (привязка к глобальной учетной записи)
#define PPTAG_GUA_CHZN_PM_HOST         111L // @v11.9.12 Хост для проверки марок честный знак в разрешительном режиме. Значение тега формируется автоматически
#define PPTAG_COMPUTER_GUID            112L // @v12.0.0 GUID записи управляемого компьютера
	// и содержит строку вида [url space timestamp]
#define PPTAG_PERSON_EXTDLVRLOCCODETAG 113L // @v12.0.3 Ссылка на тег локации, содержащего внешний (дополнительный) код адреса доставки. Вообще говоря, это - ситуативный
	// тег, необходимый для унификации двух механизмом импорта документов (штатный настраиваемый импорт и EDI).
#define PPTAG_LOT_CHZNINTQTTY          114L // @v12.0.7 Количество целых единиц весового товара, соответствующих строке товарного документа. (бля!!! я не уверен,
	// что правильно сформулировал эту хуйню: суть в том, что при отгрузке сыра, сметаны и прочего по весу, честный знак требует указания количества штук, ну, типа,
	// головок сыра или боченков сметаны. И вот это количество операторы должны будут как-то ввести в строку документа. Для этого и предназначен этот тег-мутант). Аминь, блядь :(((
#define PPTAG_LOT_PREFSUPPL            115L // @v12.0.7 Предпочтительный поставщик для строки заказа. Тут - сложный бизнес-процесс. Идея в том, что при обработке заказа
	// от покупателя, в случае дроп-шиппинга и т.д. менеджер должен решить у кого он будет закупать заказанный товар. Приняв решение (опустим детали того, как он будет
	// принимать это решение), менеджер установит этот тег в строку заказа и система сформирует документ прихода (или еще какой), связанный с этим поставщиком.
#define PPTAG_PERSON_MARKETPLACESYMB   116L // @v12.1.1 Зарезервированный символ конкретного маркетплейса, присваиваемый персоналии, относящейся к виду 'Маркетплейс'
	// для того, чтобы можно было взаимодействовать с api этого маркетплейса.
#define PPTAG_GUA_ACCOUNT              117L // @v12.1.3 Бухгалтерский счет, связанный с глобальной учетной записью, для учета движения средств.
	// Кейс следующий: если глобальная учетная запись ассоциирована с каким-то внешним сервисом, который открывает для нас счет, по которому
	// осуществляется движение и существует возможность вывести наши средства, то мы заводим некоторый счет (возможно, регистровый) и отображаем
	// на нем движение средств.
#define PPTAG_LOT_ORGORDERIDENT        118L // @v12.1.4 Оригинальный идентификатор заказа. Обычно импортируемый идентификатор строки заказа.
#define PPTAG_LOT_MP_COMMISSIONPCT     119L // @v12.1.8 Процент комиссии маркетплейса с цены продажи
#define PPTAG_LOT_MP_DISCOUNTPCT       120L // @v12.1.8 Процентная скидка на товар, предоставляемая маркетплейсом к цене продавца
#define PPTAG_LOT_MP_SELLERSPRICE      121L // @v12.1.9 Цена товара на маркетплейсе, выставленная продавцом. Эта цена в общем случае может отличаться от цены лота по ряду причин.
#define PPTAG_LOT_MP_STOCK_INWAYTO     122L // @v12.1.10 Тег строки документа вида PPOPK_EDI_STOCK. Количество торговых единиц товара, находящихся в пути на склад
#define PPTAG_LOT_MP_STOCK_INWAYFROM   123L // @v12.1.10 Тег строки документа вида PPOPK_EDI_STOCK. Количество торговых единиц товара, находящихся в пути со склада
#define PPTAG_LOT_MP_STOCK_TIMESTAMP   124L // @v12.1.10 Тег строки документа вида PPOPK_EDI_STOCK. Момент актуалности элемента остатков, полученных от маркетплейса.
#define PPTAG_PERSON_SPCTAXGROUP       125L // @v12.1.12 Специальная налоговая ставка персоналии при продаже товаров (инициировано необходимостью определять специальные
	// ставки НДС 5% и 7% в России с 2025года).
#define PPTAG_BILL_ORGORDERIDENT       126L // @v12.2.2 Оригинальный идентификатор документа заказа. Дополнительный код документа (не заказа), который идентифицирует
	// оригинальный заказ покупателя, в соответствии с которым документ был создан.
#define PPTAG_LOC_SPCTAXGROUP          127L // @v12.2.4 Специальная налоговая ставка склада при продаже товаров (инициировано необходимостью определять специальные
	// ставки НДС 5% и 7% в России с 2025года).
#define PPTAG_POSNODE_TEST             128L // @v12.2.9 Признак тестовой конфигурации кассового узла. Строка. Трактуется так как вздумается в целях разработки.
#define PPTAG_PERSON_KEYWORDS          129L // @v12.3.6 Ключевые слова персоналии, а также специальные метки.
#define PPTAG_GOODS_SALESRESTR         130L // @v12.3.7 Ограничение на продажу товара. Ссылается на экземпляр объекта PPOBJ_SALESRESTRICTION.
	// Необходим для напоминаний на Point-Of-Sale.
#define PPTAG_GUA_CHZNLOCPMURL         131L // @v12.3.11 Параметры сервера для локальной проверки марок честный знак. Тег композитный - для редактирования применяется специальный диалог.
#define PPTAG_PERSON_NOTCH             132L // @v12.4.11 Текстовый тег, содержащий одну или несколько текстовых меток для тонкой корректировки некоторых функций системы
	// в применении к этой персоналии. Инициирующим толчком послужила необходимость модифицировать экспорт документов в формате nalog.ru в зависимости от контрагента или провайдера.
//
// Зарезервированные идентификаторы глобальных учетных записей
//
#define PPGUAID_UHTT_CORE      1L // Universe-HTT
//
// Типы кассовых систем
// Очень специфичная штука. Все типы покамест будем резервировать и жестко привязываться в программе к ним.
// CMT - Cash Mashine Type
// Наименования типов хранятся в ресурсах строк в группе PPTXT_CMT.
// Нумерация строк совпадает с нумерацией типов аппаратов.
//
#define PPCMT_OKA500          1L // ОКА 500.1
#define PPCMT_EL92AQ          2L // Электроника-92-Аквариус-Ф (И.Николаев)
#define PPCMT_ECR930          4L // ЭКР-4110
#define PPCMT_SMUKM30         5L // IPC-POS (СуперМаг УКМ Сервис-Плюс)
#define PPCMT_SYNCSYM         6L // Эмулятор синхронного кассового аппарата
#define PPCMT_PETROVICH       7L // PETROVICH (Ладога-Ф, Электроника-92, ЭКР-3102Ф)
#define PPCMT_AMC200          8L // АМС-200Ф (Драйвер ЗАО Шанс)
#define PPCMT_CRUKM           9L // Кристалл-УКМ (ООО Кристалл-Сервис)
#define PPCMT_CRCSHSRV       10L // Кристалл-Кассовый сервер
#define PPCMT_SHTRIHFRF      11L // Штрих-ФР
#define PPCMT_ATOL           12L // Атол
#define PPCMT_PAPYRUS        14L // Papyrus
#define PPCMT_ATOLWOATOLCARD 15L // Атол без АТОЛ-CARD
#define PPCMT_SHTRIHMFRK     16L // Штрих-М-ФК-К
#define PPCMT_FRONTOL        17L // Фронтол
#define PPCMT_ATOLDRV        18L // Универсальный драйвер АТОЛ
#define PPCMT_CASHNGROUP     19L // Группа кассовых узлов
#define PPCMT_SYNCCASH		 20L // Синхронный кассовый узел // @vmiller
#define PPCMT_DISTRIB        21L // Распределенный кассовый узел. Специализированный тип кассового узла,
	// применяемый для централизованного управления хранением кассовых чеков, генерируемых разными предприятиями.
	// Кассовые сессии, привязанные к такому узлу в качестве номера кассы используют идент адреса магазина.
#define PPCMT_SETSTART       22L // SETSTART
#define PPCMT_FIRST_DYN_DVC  23L // Идентификатор первого динамически подгружаемого драйвера 
#define PPCMT_LAST_DYN_DVC  500L // Последний допустимый идентификатор динамически подгружаемого драйвера
// Все новые статически идентифицируемые устройсва должны иметь идент >PPCMT_LAST_DYN_DVC
#define PPCMT_DREAMKAS      501L // DREAMKAS
//
// Типы принтеров штрихкодов
// Наименования типов хранятся в ресурсах строк в группе PPTXT_BCPT.
// Нумерация строк совпадает с нумерацией типов аппаратов.
//
#define PPBCPT_ZEBRA         1L // Zebra
#define PPBCPT_DATAMAX       2L // Datamax Corporation
#define PPBCPT_ELTRON        3L // Zebra-Eltron (EPL2)
#define PPBCPT_WINDOWS       4L // Стандартный Windows-принтер с отчетов из CrystalReports
//
// Типы электронных весов (@v3.0.0)
// Наименования типов хранятся в ресурсах строк в группе PPTXT_SCLT.
// Нумерация строк совпадает с нумерацией типов аппаратов.
//
#define PPSCLT_CAS            1L // CAS
#define PPSCLT_MASSAK         2L // Масса-К
#define PPSCLT_MTOLEDO        3L // Mettler Toledo
#define PPSCLT_CRCSHSRV       4L // Кассовый сервер Кристалл
#define PPSCLT_WEIGHTTERM     5L // Весовой терминал ТВ
#define PPSCLT_DIGI           6L // DIGI
#define PPSCLT_BIZERBA        7L // BIZERBA
#define PPSCLT_SHTRIHPRINT    8L // Штрих-Принт
#define PPSCLT_CASCL5000J     9L // CAS CL5000J
#define PPSCLT_SCALEGROUP    10L // Группа весов
#define PPSCLT_EXPORTTOFILE  11L // Экспорт в файл
#define PPSCLT_SHTRIHCE      12L // Штрих PC200 CE
//
// Типы дисплеев покупателей (@v4.7.6)
// Наименования типов хранятся в ресурсах строк в группе PPTXT_CUSTDISP.
// Нумерация строк совпадает с нумерацией типов аппаратов.
//
/*#define PPCUSTDISP_VFD       1L // VFD-Epson compatible customer display
#define PPCUSTDISP_DPD201    2L // Штрих-DPD201
#define PPCUSTDISP_POSIFLEX	 3L	// Posiflex*/
//
// Типы TouchScreen
// Наименования типов хранятся в ресурсах строк в группе PPTXT_TOUCHSCREEN.
// Нумерация строк совпадает с нумерацией типов аппаратов.
//
#define PPTOUCHSCREEN_ELO       1L // ELO TouchScreen
//
// Типы ассоциаций
//
#define PPASS_ALTGOODSGRP        1L // Альтернативные группы товаров
#define PPASS_BILLSET            2L // Набор документов                                ***
#define PPASS_GOODSSTRUC         3L // Компоненты структуры товара
#define PPASS_GENGOODS           4L // Обобщенный товар
#define PPASS_PERSONPOST         5L // Должность персоналии (в штатном расписании)
#define PPASS_PAYMBILLPOOL       6L // Пул зачетных оплат                      ***
#define PPASS_OPBILLPOOL         7L // Операционный пул документов             ***
#define PPASS_CSESSBILLPOOL      8L // Пул документов списания кассовой сессии ***
#define PPASS_GOODSBASKET       10L // Корзина товаров
#define PPASS_GROUPARTICLE      11L // Группирующая статья аналитического учета
#define PPASS_ADDRESSBOOK       12L // Адресная книга (На каждого пользователя своя)
#define PPASS_PERSONREL         13L // Связи между персоналиями (managed by PersonCore)
#define PPASS_TSESSBILLPOOL     14L // Пул документов списания технологической сессии ***
#define PPASS_GOODS2LOC         15L // Ассоциация товар(группа товаров)-склад
#define PPASS_CSDBILLPOOL       16L // Пул документов покрытия дефицита по кассовым сессиям
#define PPASS_GOODS2SUPPL       17L // Ассоциация товар (группа товаров)-поставщик
#define PPASS_TODOBILLPOOL      18L // Пул документов по задачам
#define PPASS_PRJBILLPOOL       19L // Пул документов по проектам
#define PPASS_PRJPHASEBILLPOOL  20L // Пул документов по фазам проектов
#define PPASS_TSDBILLPOOL       21L // Пул документов покрытия дефицита по технологическим сессиям
#define PPASS_GOODS2CASHNODE    22L // Ассоциация товар(группа товаров)-кассовый узел
#define PPASS_GOODS2WAREPLACE   23L // Ассоциация товар(группа товаров)-место хранения. Допускает дублирование: с одним товаром может быть сопоставлено более одного места хранения //
#define PPASS_CHECKINPSNTSES    25L // PPCheckInPersonItem Регистрация персоналий в технологических сессиях
#define PPASS_GOODSCOMP	        26L // Ассоциация товара и его компонента
#define PPASS_GOODSSTRUC_AR     27L // @v12.0.6 Компоненты структуры товара (аналитические статьи)
#define PPASS_ORDACCOMPBILLPOOL 28L // @v12.0.11 Пул документов, сопуствующих списанию заказа в отгрузку
//
// Категории документов. Применяются в PPViewBill.
// Перечисление BrowseBillsType (pp.h) однозначно проецируется в эти константы.
// Необходимость в этих константах возникла из-за использования в описании ресурсов PPBROW.RC2
//
#define BBT_UNDEF              -1
#define BBT_GOODSBILLS          0
#define BBT_ORDERBILLS          1
#define BBT_ACCTURNBILLS        2
#define BBT_INVENTORYBILLS      3
#define BBT_POOLBILLS           4
#define BBT_CLIENTDEBT          5
#define BBT_CLIENTRPAYMENT      6
#define BBT_DRAFTBILLS          7
#define BBT_REALTYPES           8  // Товарные, бухгалтерские документы и оплаты
#define BBT_WMSBILLS            9  // Документы складских операций
#define BBT_SPC_CHARGEONMARKS  10  // Special: постановка на баланс маркированной продукции
//
// Вид анализа счета. Применяются в PPViewAccAnlz
// Перечисление AccAnlzKind (pp.h) однозначно проецируется в эти константы.
// Необходимость в этих константах возникла из-за использования в описании ресурсов PPBROW.RC2
//
#define AAKND_GENERIC           0
#define AAKND_CASHBOOK          1
#define AAKND_ACCTRNOVR         2
#define AAKND_SUPPLTRNOVR       3

#define BBT_UNDEF              -1
#define BBT_GOODSBILLS          0
#define BBT_ORDERBILLS          1
#define BBT_ACCTURNBILLS        2
#define BBT_INVENTORYBILLS      3
#define BBT_POOLBILLS           4
#define BBT_CLIENTDEBT          5
#define BBT_CLIENTRPAYMENT      6
#define BBT_DRAFTBILLS          7
#define BBT_REALTYPES           8  // Товарные, бухгалтерские документы и оплаты
//
// Отраслевые классы товаров
//
#define PPGDSCLS_STANDARD       1L // Стандартный
#define PPGDSCLS_SAWLOG       101L // Пиловочные бревна
#define PPGDSCLS_TIMBER       102L // Пиломатериалы
//
// Типы EDI-операций
// @persistent
//
#define PPEDIOP_NOP                            0 // Не EDI-операция //
#define PPEDIOP_ORDER                          1 // Заказ поставщику
#define PPEDIOP_ORDERRSP                       2 // Подтверждение заказа от поставщика
#define PPEDIOP_APERAK                         3 // Статус обработки заказа поставщиком
#define PPEDIOP_DESADV                         4 // Уведомление от отгрузки поставщиком
#define PPEDIOP_RECADV                         5 // Подтверждение получения товара от поставщика
#define PPEDIOP_DECLINEORDER                   6 // Отклонение заказа ранее высланного поставщику после получения подтверждения //
#define PPEDIOP_ALCODESADV                     7 // Дополнительный DESADV-документ с атрибутами, необходимыми для декларирования алкогольной продукции
#define PPEDIOP_SALESORDER                     8 // Заказ от покупателя
#define PPEDIOP_PARTIN                         9 //
#define PPEDIOP_INVOIC                        10 //
#define PPEDIOP_RETURNREQ                     11 //
// Следующие 4 операции не являются стандартными в наборе EDI-операций. Они здесь для необычных операций размещения подотчетного торгового оборудования на точках продаж.
#define PPEDIOP_PLACEMENTORDER                12 // @v12.4.5 Ситуативный тип для BAILMENT-операций. Ордер на размещение оборудования. 
#define PPEDIOP_REMOVALORDER                  13 // @v12.4.5 Ситуативный тип для BAILMENT-операций. Ордер на возврат оборудования. 
#define PPEDIOP_POS2POSMOVEMENTORDER          14 // @v12.4.5 Ситуативный тип для BAILMENT-операций. Ордер на перемещение оборудования между точками продаж.
#define PPEDIOP_INSTALLATIONORDER             15 // @v12.4.5 Ситуативный тип для BAILMENT-операций. Ордер на выполнение работ по установке оборудования.
//
// Типы операций обмена с системой ЕГАИС
//
#define PPEDIOP_EGAIS_WAYBILL                 21
#define PPEDIOP_EGAIS_TICKET                  22
#define PPEDIOP_EGAIS_WAYBILLACT              23
#define PPEDIOP_EGAIS_CONFIRMTICKET           24
#define PPEDIOP_EGAIS_TTNINFORMBREG           25
#define PPEDIOP_EGAIS_ACTREJECT               26
#define PPEDIOP_EGAIS_ACTUSING                27
#define PPEDIOP_EGAIS_ACTINVENTORY            28
#define PPEDIOP_EGAIS_ACTINVENTORYPARTIAL     29
#define PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG  30
#define PPEDIOP_EGAIS_QUERYAP                 31
#define PPEDIOP_EGAIS_QUERYSSP                32
#define PPEDIOP_EGAIS_QUERYSP                 33
#define PPEDIOP_EGAIS_QUERYCLIENTS            34
#define PPEDIOP_EGAIS_QUERYRESTS              35
#define PPEDIOP_EGAIS_QUERYFORMA              36
#define PPEDIOP_EGAIS_QUERYFORMB              37
#define PPEDIOP_EGAIS_REPLYSSP                38
#define PPEDIOP_EGAIS_REPLYSPIRIT             39
#define PPEDIOP_EGAIS_REPLYCLIENT             40
#define PPEDIOP_EGAIS_REPLYAP                 41
#define PPEDIOP_EGAIS_REPLYRESTS              42 // Ответ на запрос остатков
#define PPEDIOP_EGAIS_REPLYFORMA              43
#define PPEDIOP_EGAIS_REPLYFORMB              44
#define PPEDIOP_EGAIS_ACTCHARGEON             45
#define PPEDIOP_EGAIS_ACTWRITEOFF             46
#define PPEDIOP_EGAIS_REQUESTREPEALWB         47 //
#define PPEDIOP_EGAIS_CONFIRMREPEALWB         48 //
#define PPEDIOP_EGAIS_QUERYRESTSSHOP          49 //
#define PPEDIOP_EGAIS_REPLYRESTSSHOP          50 //
#define PPEDIOP_EGAIS_QUERYBARCODE            51 //
#define PPEDIOP_EGAIS_REPLYBARCODE            52 //
#define PPEDIOP_EGAIS_TRANSFERTOSHOP          53 //
#define PPEDIOP_EGAIS_ACTCHARGEONSHOP         54 // ActChargeOnShop_v2
#define PPEDIOP_EGAIS_TRANSFERFROMSHOP        55 //
#define PPEDIOP_EGAIS_ACTCHARGEON_V2          56 //
#define PPEDIOP_EGAIS_ACTWRITEOFF_V2          57 //
#define PPEDIOP_EGAIS_ACTWRITEOFFSHOP         58 //
#define PPEDIOP_EGAIS_WAYBILL_V2              59 //
#define PPEDIOP_EGAIS_TTNINFORMF2REG          60 // version 2 of PPEDIOP_EGAIS_TTNINFORMBREG
#define PPEDIOP_EGAIS_WAYBILLACT_V2           61 // version 2 of PPEDIOP_EGAIS_WAYBILLACT
#define PPEDIOP_EGAIS_NOTIFY_WBVER2           62 // Уведомление сервера ЕГАИС об использовании версии 2 протокола обмена
#define PPEDIOP_EGAIS_QUERYRESTS_V2           63 // Запрос остатков version 2
#define PPEDIOP_EGAIS_REPLYRESTS_V2           64 // Ответ на запрос остатков version 2
#define PPEDIOP_EGAIS_ACTINVENTORYINFORMF2REG 65 // version 2 of PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG
#define PPEDIOP_EGAIS_WAYBILL_V3              66 //
#define PPEDIOP_EGAIS_WAYBILLACT_V3           67 // version 3 of PPEDIOP_EGAIS_WAYBILLACT
#define PPEDIOP_EGAIS_ACTWRITEOFF_V3          68 //
#define PPEDIOP_EGAIS_NOTIFY_WBVER3           69 // Уведомление сервера ЕГАИС об использовании версии 3 протокола обмена (то же, что и PPEDIOP_EGAIS_NOTIFY_WBVER2, но с небольшим изменением внутри).
#define PPEDIOP_EGAIS_REQUESTREPEALAWO        70 //
#define PPEDIOP_EGAIS_QUERYRESENDDOC          71 //
#define PPEDIOP_EGAIS_QUERYRESTBCODE          72 // Запрос марок по справке Б
#define PPEDIOP_EGAIS_REPLYRESTBCODE          73 // Ответ на запрос марок по справке Б
#define PPEDIOP_EGAIS_ACTFIXBARCODE           74 // ActFixBarCode Акт фиксации штрихкодов на балансе организации
#define PPEDIOP_EGAIS_ACTUNFIXBARCODE         75 // ActUnFixBarCode Акт отмены фиксации штрихкодов на балансе организации
#define PPEDIOP_EGAIS_NOTIFY_WBVER4           76 // @v11.0.12 Уведомление сервера ЕГАИС об использовании версии 4 протокола обмена (то же, что и PPEDIOP_EGAIS_NOTIFY_WBVER2, но с небольшим изменением внутри).
#define PPEDIOP_EGAIS_WAYBILL_V4              77 // @v11.0.12 version 4 of PPEDIOP_EGAIS_WAYBILLACT
#define PPEDIOP_EGAIS_WAYBILLACT_V4           78 // @v11.0.12 version 4 of PPEDIOP_EGAIS_WAYBILLACT
#define PPEDIOP_EGAIS_QUERYFORMF1             79 // @v11.2.12 Новая версия PPEDIOP_EGAIS_QUERYFORMA
#define PPEDIOP_EGAIS_QUERYFORMF2             80 // @v11.2.12 Новая версия PPEDIOP_EGAIS_QUERYFORMB
#define PPEDIOP_EGAIS_REPLYFORM1              81 // @v11.3.4
#define PPEDIOP_EGAIS_REPLYFORM2              82 // @v11.3.4
//
#define PPEDIOP_MRKTPLC_RECEIPT              101 // @v12.1.6 Поступление товара на склад маркетплейса
#define PPEDIOP_MRKTPLC_ORDER                102 // @v12.1.6 Заказ от покупателя, поступивший на маркетплейс
#define PPEDIOP_MRKTPLC_SALE                 103 // @v12.1.6 Продажа покупателю маркетплейсом
#define PPEDIOP_MRKTPLC_STOCK                104 // @v12.1.10 Остатки на складе маркетплейса
#define PPEDIOP_MRKTPLC_RETURN               105 // @v12.1.11 Возврат от покупателя маркетплейсу
#define PPEDIOP_MRKTPLC_CLOST                106 // @v12.2.1  Компенсированная маркетплейсом потеря на складе маркетплейсе
//
// Descr: Статусы ответа (RECADV) на уведомление об отгрузке (DESADV)
//
#define PPEDI_RECADV_STATUS_UNDEF             0 // Не определен
#define PPEDI_RECADV_STATUS_ACCEPT            1 // Полностью принят
#define PPEDI_RECADV_STATUS_PARTACCEPT        2 // Частично принят
#define PPEDI_RECADV_STATUS_REJECT            3 // Полностью отклонен
//
// Descr: Статусы подтверждения ответа (RECADV) на уведомление об отгрузке (DESADV)
//
#define PPEDI_RECADVCONF_STATUS_UNDEF         0 // Не определен
#define PPEDI_RECADVCONF_STATUS_ACCEPT        1 // Принят
#define PPEDI_RECADVCONF_STATUS_REJECT        3 // Отклонен
//
// Descr: Состояния документов относительно EDI, отображаемые для пользователей
//
#define BEDIUS_DESADV_OUT_SENDED              1 // Документ отправлен получателю в виде DESADV
#define BEDIUS_DESADV_OUT_PROCESSED           2 // Документ, отправленный получателю в виде DESADV, принят EDI-сервером
#define BEDIUS_DESADV_OUT_RECADV_ACC          3 // На документ, отправленный получателю в виде DESADV, получен RECADV "принят"
#define BEDIUS_DESADV_OUT_RECADV_PACC         4 // На документ, отправленный получателю в виде DESADV, получен RECADV "частично принят"
#define BEDIUS_DESADV_OUT_RECADV_REJ          5 // На документ, отправленный получателю в виде DESADV, получен RECADV "отклонен"
#define BEDIUS_DESADV_OUT_RECADV_CONF_ACC   100 // В ответ на RECADV отправлено подтверждение "принято"
#define BEDIUS_DESADV_OUT_RECADV_CONF_REJ   200 // В ответ на RECADV отправлено подтверждение "отклонено"

#define BEDIUS_DESADV_IN_ACCEPTED             8 // Документ получен от отправителя в виде DESADV
#define BEDIUS_DESADV_IN_RECADV_ACC           9 // На документ, полученный от отправителя в виде DESADV, отправлен RECADV "принят"
#define BEDIUS_DESADV_IN_RECADV_PACC         10 // На документ, полученный от отправителя в виде DESADV, отправлен RECADV "частично принят"
#define BEDIUS_DESADV_IN_RECADV_REJ          11 // На документ, полученный от отправителя в виде DESADV, отправлен RECADV "отклонен"

#define BEDIUS_DESADV_OUT_RECADV_CONF_ACC_ON_ACC   (BEDIUS_DESADV_OUT_RECADV_CONF_ACC + BEDIUS_DESADV_OUT_RECADV_ACC)
#define BEDIUS_DESADV_OUT_RECADV_CONF_ACC_ON_PACC  (BEDIUS_DESADV_OUT_RECADV_CONF_ACC + BEDIUS_DESADV_OUT_RECADV_PACC)
#define BEDIUS_DESADV_OUT_RECADV_CONF_ACC_ON_REJ   (BEDIUS_DESADV_OUT_RECADV_CONF_ACC + BEDIUS_DESADV_OUT_RECADV_REJ)
#define BEDIUS_DESADV_OUT_RECADV_CONF_REJ_ON_ACC   (BEDIUS_DESADV_OUT_RECADV_CONF_REJ + BEDIUS_DESADV_OUT_RECADV_ACC)
#define BEDIUS_DESADV_OUT_RECADV_CONF_REJ_ON_PACC  (BEDIUS_DESADV_OUT_RECADV_CONF_REJ + BEDIUS_DESADV_OUT_RECADV_PACC)
#define BEDIUS_DESADV_OUT_RECADV_CONF_REJ_ON_REJ   (BEDIUS_DESADV_OUT_RECADV_CONF_REJ + BEDIUS_DESADV_OUT_RECADV_REJ)
//
// Descr: Квалификаторы субъекта-стороны электронного документооборота
//
#define EDIPARTYQ_BUYER                       1
#define EDIPARTYQ_CORPOFFICE                  2
#define EDIPARTYQ_DELIVERY                    3 // Грузополучатель
#define EDIPARTYQ_INVOICEE                    4 //
#define EDIPARTYQ_STORENUMBER                 5
#define EDIPARTYQ_SUPPLAGENT                  6
#define EDIPARTYQ_SUPPLIER                    7
#define EDIPARTYQ_WAREHOUSEKEEPER             8
#define EDIPARTYQ_CONSIGNOR                   9
#define EDIPARTYQ_CONSIGNER                   EDIPARTYQ_CONSIGNOR // @synonym
#define EDIPARTYQ_CONSIGNEE                  10
#define EDIPARTYQ_SELLER                     11
#define EDIPARTYQ_PAYEE                      12
#define EDIPARTYQ_CONSOLIDATOR               13
#define EDIPARTYQ_ISSUEROFINVOICE            14
#define EDIPARTYQ_SHIPTO                     15
#define EDIPARTYQ_BILLANDSHIPTO              16
#define EDIPARTYQ_BROKERORSALESOFFICE        17
#define EDIPARTYQ_ULTIMATECONSIGNEE          18 // @v12.4.10 Party who has been designated on the invoice or packing list as the final recipient of the stated merchandise.
#define EDIPARTYQ_ULTIMATECUSTOMER           19 // @v12.4.10 The final recipient of goods. (похоже, это - тоже, что и EDIPARTYQ_ULTIMATECONSIGNEE)
//
// Menus
//
#define MENU_MASTER                   1
#define MENU_DEFAULT        MENU_MASTER
#define MENU_SYSTEM                   4 // Конверторы, корректоры и т.д.
#define MENU_NOLOGIN                103 // Меню без авторизации в базе данных
//
// Status Defs
//
#define STATUS_MASTER               1
//
// Bitmap
//
#define IDB_CALENDAR              BM_CALENDAR
#define IDB_DATERANGE             BM_DATERANGE
#define IDB_FILEBROWSE            BM_FILEBROWSE
#define IDB_INLNCALC              BM_INLNCALC
#define IDB_MONEY01               BM_MONEY01
#define IDB_CASH01                BM_CASH01
#define IDB_BARCODEPRINTER        BM_BARCODEPRINTER
#define IDB_FILLSALESTBL          BM_FILLSALESTBL
#define IDB_BHT                   BM_BHT
#define IDB_INLNCALCL             BM_INLNCALCL
#define IDB_CASH01S               BM_CASH01S
#define IDB_QUANTITY              BM_QUANTITY
#define IDB_SCARD                 BM_SCARD
#define IDB_RETURN                BM_RETURN
#define IDB_BYPRICE               BM_BYPRICE
#define IDB_BYCASH                BM_BYCASH
#define IDB_BANKING               BM_BANKING
#define IDB_BANKINGDISABLED       BM_BANKINGDISABLED
#define IDB_GDSGRP                BM_GDSGRP
#define IDB_DEFGRP                BM_DEFGRP
#define IDB_ENTER                 BM_ENTER
#define IDB_CANCEL                BM_CANCEL
#define IDB_REDO                  BM_REDO
#define IDB_SUSCHECK              BM_SUSCHECK
#define IDB_PRINTER               BM_PRINTER
#define IDB_PRINTDEF              BM_PRINTDEF
#define IDB_TABLE                 BM_TABLE
#define IDB_ADD                   BM_ADD
#define IDB_GREEN                 BM_GREEN
#define IDB_RED                   BM_RED
#define IDB_RTABLE                BM_RTABLE
#define IDB_BYNAME                BM_BYNAME
#define IDB_CLOSE                 BM_CLOSE     // Отмена        16*16*256
#define IDB_OK                    BM_OK        // OK            16*16*256
#define IDB_PRINT_448             BM_PRINT_448 // Принтер       16*16*256
#define IDB_DEL_448               BM_DEL_448 // Удалить       16*16*256
#define IDB_INS_448               BM_INS_448 // Добавить      16*16*256
#define IDB_EDIT_448              BM_EDIT_448 // Редактировать 16*16*256
#define IDB_MAGN_448              BM_MAGN_448 // Линза         16*16*256
#define IDB_CITY                  BM_CITY // Город
#define IDB_RGB                   BM_RGB //
#define IDB_HSB                   BM_HSB //
#define IDB_PICT_STATUS           BM_PICT_STATUS // Картинка для диалога DLG_STATUS (текущее состояние)
#define IDB_KEYS                  BM_KEYS // Ключи         48*48*256
#define IDB_MAIL_SEND             BM_MAIL_SEND
#define IDB_MENU                  BM_MENU // Меню
#define IDB_BARCODE               BM_BARCODE // Штрихкод
#define IDB_MATRIX                BM_MATRIX // Товарная матрица
#define IDB_DIVISION              BM_DIVISION // Отдел
#define IDB_ARROW_UP              BM_ARROW_UP
#define IDB_ARROW_DOWN            BM_ARROW_DOWN
#define IDB_GEAR                  BM_GEAR
#define IDB_LOCK                  BM_LOCK
#define IDB_PRINTDEFMARK          BM_PRINTDEFMARK // То же, что и IDB_PRINTDEF но с галкой
#define IDB_CLOCK                 BM_CLOCK
#define IDB_GUESTS                BM_GUESTS
#define IDB_TABLE_ORDERS          BM_TABLE_ORDERS
#define IDB_CRDCARD               BM_CRDCARD
#define IDB_ARROW_LEVELUP         BM_ARROW_LEVELUP
#define IDB_SELMODIFIER           BM_SELMODIFIER
#define IDB_PASTE                 BM_PASTE
#define IDB_PHONE                 BM_PHONE
#define IDB_PHONEFORWARDED        BM_PHONEFORWARDED
#define IDB_DELIVERY              BM_DELIVERY
//
// Virtual Buttons
//
#define VB_IL_CALC                  1 // InputLine Calculator
#define VB_IL_CALCL                 2 // InputLine Calculator (Large button)
//
// Background
//
#define IDB_BG_PAPYRUS            201
//
// Dialogs and it's controls
//
#define MASTERDLG_BIAS           2000 // Смещение

#define STDCTL_OKBUTTON           101
#define STDCTL_CANCELBUTTON       102
#define STDCTL_INSBUTTON          103
#define STDCTL_DELBUTTON          104
#define STDCTL_EDITBUTTON         105
#define STDCTL_MORE               106
#define STDCTL_DOWNBUTTON         107
#define STDCTL_UPBUTTON           108
#define STDCTL_SKIPBUTTON         109
#define STDCTL_ALLBUTTON          110
#define STDCTL_PRINT              112
#define STDCTL_BACKBUTTON         113
#define STDCTL_TESTBUTTON         114
#define STDCTL_PRINTBUTTON        115
#define STDCTL_POSNODELISTBUTTON  116 // Кнопка выбора списка кассовых узлов
#define STDCTL_LOCLISTBUTTON      117 // Кнопка выбора списка складов
#define STDCTL_GOODSFILTBUTTON    118 // Кнопка расширенного фильтра товаров
#define STDCTL_TAGSBUTTON         119 // Кнопка просмотра тегов по объекту
#define STDCTL_LOTSBUTTON         120 // Кнопка просмотра лотов

#define STDCTL_LOCMENUBUTTON_01   121
#define STDCTL_LOCMENUBUTTON_02   122
#define STDCTL_LOCMENUBUTTON_03   123
#define STDCTL_LOCMENUBUTTON_04   124
#define STDCTL_LOCMENUBUTTON_05   125
#define STDCTL_LOCMENUBUTTON_06   126
#define STDCTL_LOCMENUBUTTON_07   127
#define STDCTL_LOCMENUBUTTON_08   128
#define STDCTL_LOCMENUBUTTON_09   129
#define STDCTL_LOCMENUBUTTON_10   130
#define STDCTL_LOCMENUBUTTON_11   131
#define STDCTL_LOCMENUBUTTON_12   132
#define STDCTL_LOCMENUBUTTON_13   133
#define STDCTL_LOCMENUBUTTON_14   134
#define STDCTL_LOCMENUBUTTON_15   135
#define STDCTL_LOCMENUBUTTON_16   136
#define STDCTL_LOCMENUBUTTON_17   137
#define STDCTL_LOCMENUBUTTON_18   138
#define STDCTL_LOCMENUBUTTON_19   139
#define STDCTL_LOCMENUBUTTON_20   140

#define STDCTL_LOCMENUBUTTON_FIRST 121
#define STDCTL_LOCMENUBUTTON_LAST  140

#define STDCTL_UHTT               141 //
#define STDCTL_IMGADDBUTTON       142 //
#define STDCTL_IMGDELBUTTON       143 //
#define STDCTL_IMGPSTBUTTON       144 //
#define STDCTL_SELECTOR_WHAT      145
#define STDCTL_ACTIVATEBUTTON     146
#define STDCTL_BILLSBUTTON        147 //
#define STDCTL_REGISTERSBUTTON    148 // Кнопка "Регистры" (@but_registers)
#define STDCTL_SCSERLISTBUTTON    149 // Кнопка выбора списка серий персональных карт
#define STDCTL_ARLISTBUTTON       150 // Кнопка выбора списка статей
#define STDCTL_AGENTLISTBUTTON    151 // Кнопка выбора списка агентов
#define STDCTL_AGREEMENTBUTTON    152 // Кнопка соглашения с покупателем/поставщиком
#define STDCTL_SYSINFOBUTTON      153 // Кнопка системной информации по объекту
#define STDCTL_VIEWOPTBUTTON      154 // Кнопка "Опции просмотра"
#define STDCTL_SJBUTTON           155 // Кнопка "Системные события" (для внутреннего фильтра по сис журналу и для просмотра событий)
#define STDCTL_PERSONBUTTON       156 // Кнопка "Персоналия"
#define STDCTL_LINKOBJBUTTON      157 // Кнопка "Связанный объект"
#define STDCTL_QKLISTBUTTON       158 // Кнопка выбора списка видов котировок
#define STDCTL_FILTERBUTTON       159 // Кнопка "Фильтр" (только для тех диалогов, в которых есть единственный фильтр)
#define STDCTL_VERIFYBUTTON       160 // Кнопка "Верифицировать"
#define STDCTL_LAYOUTBUTTON       161 // Кнопка "Layout"
#define STDCTL_TRANSMITBUTTON     162 // Кнопка "Передать"
#define STDCTL_DIAGNOSTICS        163 // Кнопка "Диагностика"
#define STDCTL_REPEATINGBUTTON    164 // @v11.0.4 Кнопка "Повторение"
#define STDCTL_MAPONGEOLOCBUTTON  165 // @v11.6.2 Кнопка запуска функции открытия карты с координатами (cmExecMapOnGeoLoc)
#define STDCTL_SINGLELISTBOX      166 // @v12.2.2 Список (SmartListBox)
#define STDCTL_HEADERSTATICTEXT   167 // @v12.2.5 Статический текст (TStaticText) в заголовочной части окна.
#define STDCTL_TODOBUTTON         168 // @v12.4.7 Кнопка 'Задача' cmToDo

#define cmPPAccSelected          1101 // Сообщение, посылаемое группой
	// AcctCtrlGroup диалогу-владельцу когда пользователь выбрал новый счет. (AcctCtrlGroup*)InfoPtr
#define cmPPArSelected           1102 // Сообщение, посылаемое группой
	// AcctCtrlGroup диалогу-владельцу когда пользователь выбрал новую статью. (AcctCtrlGroup*)InfoPtr
#define cmCurAmtGrpSetupCurrencyCombo 1103 // Сообщение, посылаемое группе
	// CurAmtCtrlGroup для установки ограничения выбора валюты Info param (PPIDArray *)
//
// Следующие команды используются несколькими диалогами
// Эти команды не запрещаются (enableCommand на них не действует)
// из-за того, что номера выходят за пределы 1000
//
#define cmSwap                   1111 // Обмен содержимого двух полей или комбо-боксов
#define cmSelQc                  1112 // Выбор серитфиката качества
#define cmEditMailAcc            1113 // Редактирование почтовой учетной записи
#define cmAddBySample            1114 // Создание объекта по образцу
#define cmQuot                   1115 // Просмотр котировок
#define cmSearch                 1116 // Поиск
#define cmTransmit               1117 // Передать выборку объектов
#define cmSysJournalByObj        1118 // Системные события по объекту
#define cmTest                   1119 // Кнопка отладочного тестирования //
#define cmLocList                1120 // Кнопка [Список складов]
#define cmGoodsStruc             1121 // Кнопка [Структура товара]
#define cmTransmitCharry         1122 // Кнопка [Передать Charry]
#define cmPosNodeList            1123 // Кнопка [Список кассовых узлов]
#define cmAddress                1124 // Кнопка [Адрес]
#define cmAddress2               1125 // Кнопка [Адрес]
#define cmRounding               1126 // Параметры округления //
#define cmUniteMaxLike           1127 // Объединение похожих объектов
#define cmTags                   1128 // Теги объекта
#define cmGoodsFilt              1129 // Кнопка [Фильтр] для установки сложного фильтра по товарам в отчетах
#define cmSelSCard               1130 // Выбор дисконтной карты
#define cmTimeGraph              1131 // Временная диаграмма
#define cmCreateSCard            1132 // Создать дисконтную карту
#define cmCommit                 1133 // Подтверждение некоторого действия без закрытия диалога (не путать с cmOK)
#define cmUniteObj               1134 // Объединить объект с другим (interactive)
#define cmSearchUHTT             1135 // Искать в Universe-HTT
#define cmArList                 1136 // Кнопка [Список статей]
#define cmSupplCost              1137
#define cmGoodsMatrix            1138
#define cmGoodsMatrixRestrict    1139
#define cmHeaderFields           1140
#define cmImport                 1141
#define cmSelColor               1142
#define cmTagIndFilt             1143
#define cmAddImage               1144
#define cmDelImage               1145
#define cmPasteImage             1146
#define cmActivate               1147
#define cmCCheck                 1148
#define cmSysjFilt2              1149 // Вызов внутреннего фильтра по системным событиям
#define cmEditFilt               1150
#define cmSCardSerList           1151
#define cmAgentList              1152
#define cmBills                  1153
#define cmObjSyncTab             1154
#define cmArrange                1155
#define cmAgreement              1156
#define cmSysInfo                1157
#define cmViewOptions            1158
#define cmForceUnlock            1159
#define cmSelSerial              1160
#define cmEMailList              1161
#define cmLinkObj                1162 // STDCTL_LINKOBJBUTTON @linkobject
#define cmQuotKindList           1163
#define cmSendSms                1164
#define cmVerify                 1165
#define cmFilter                 1166
#define cmExtParams              1167 // команда стала общей (ранее принадлежала только диалогу фильтра товаров)
#define cmLinkedBill             1168 // команда стала общей (ранее принадлежала только диалогу DLG_ATWROFF)
#define cmCopyToClipboard        1169
#define cmCopyToClipboardAll     1170
#define cmLayoutEntry            1171 //
#define cmPasteFromClipboardAll  1172 // @erik v10.5.2
#define cmDiagnostics            1173 // Кнопка диагностики
#define cmTsList                 1174 // Кнопка списка временных серий
#define cmPersonList             1175 // Кнопка списка персоналий
#define cmGuaList                1176 //
#define cmTotal                  1177 //
#define cmAutoFill               1178 // @v10.8.12 ранее была локальной относительно DLG_GDSLST (теперь используется и в других диалогах)
#define cmCopyToClipboardSpc     1179 // Специальное копирование в буфер обмена
#define cmConfigure              1180 //
#define cmQueryToken             1181 //
#define cmLayoutContainer        1182 //
#define cmRepeating              1183 // Вызов диалога повторений задания //
#define cmUnmatch                1184 // @v11.1.8 Отменить (разорвать) сопоставление
#define cmNow                    1185 // @v11.2.0 Кнопка [Сегодня] или [Сейчас] в календарях
#define cmLayoutList             1186 // @v11.2.1 Команда отображения иерархического списка layout'ов
#define cmPeriodResetLeft        1187 // @v11.2.4 Кнопка сброса левой границы периода в календарях
#define cmPeriodResetRight       1188 // @v11.2.4 Кнопка сброса правой границы периода в календарях
#define cmPeriodReset            1189 // @v11.2.4 Кнопка сброса периода в календарях
#define cmTree                   1190 // @v11.2.11 Кнопка для просмотра древовидной структуры объекта (пока для товарных структур)
#define cmSelectProcessors       1191 // @v11.3.4  Кнопка выбора списка процессоров
#define cmToggleAllOrOwnOnly     1192 // @v11.4.1  Переключение отображения все/только свои (пока для товарных корзин)
#define cmExecMapOnGeoLoc        1193 // @v11.6.2  Кнопка запускает функцию открытия карты с координатами, введенными в прикрепленном поле
#define cmImageChanged           1194 // @v12.0.4  [evCommand] Событие генерируется группой управляющих элементов ImageBrowseCtrlGroup в ответ на изменение изображения и передается в родительский диалог.
#define cmAutoConfigure          1195 // @v12.2.1
#define cmClientActivityConfig   1196 // @v12.2.4 Кнопка, открывающая диалог редактирования параметров конфигурации анализа клиентской активностий
#define cmRun                    1197 // @v12.3.12 Кнопка запуска основной функции (что бы это не значило)
#define cmCcExtList              1198 // @v11.8.8
#define cmCcLnExtList            1199 // @v12.4.1
#define cmToDo                   1200 // @v12.4.7 Кнопка 'Задача' в документах или еще где-нибудь. Введена ради документов.
//
#define DLG_LOGIN                   1
	#define CTL_LOGIN_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_PASSWORD           (2 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_DATE               (3 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_DB                 (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOGIN_DB              (5 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_PROTECT            (6 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_NEWPASSWORD        (7 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_LOGO               (8 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_ST_INFO            (9 + WINDOWS_ID_BIAS)
	#define CTL_LOGIN_QR                 (10 + WINDOWS_ID_BIAS)
#define DLG_CFGOPTIONS              2
#define DLG_MAINCFG                 4
	#define CTL_CFGOPTIONS_ACCESS        (1 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_MENU          (2 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_LOC           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CFGOPTIONS_LOC        (4 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_SHEET         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CFGOPTIONS_SHEET      (6 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_RLZORD        (7 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_OPTIONS       (8 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_ROUND         (9 + WINDOWS_ID_BIAS) // Кнопка "Округление..."
	#define CTL_CFGOPTIONS_RIGHTS       (10 + WINDOWS_ID_BIAS) // Кнопка "Права..."
	#define CTL_CFGOPTIONS_DBDIV        (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_CFGOPTIONS_DBDIV     (12 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_STAFF        (13 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_SECFLAGS     (14 + WINDOWS_ID_BIAS) // Флаги управления секретностью //
	#define CTL_CONFIG_MAINORG          (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_CONFIG_MAINORG       (16 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_DESK         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_CFGOPTIONS_DESK      (18 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_MENU2        (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_CFGOPTIONS_MENU2     (20 + WINDOWS_ID_BIAS)
	#define CTL_CFGOPTIONS_FEFO         (21 + WINDOWS_ID_BIAS)
#define DLG_STATUS                  3
	#define cmAeqStat                    101
	#define CTL_STATUS_USR               (1 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_LOC               (3 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_DATE              (5 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_MAINORG           (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_STATUS_MAINORG        (8 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_DBDIV             (9 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_DATAPATH         (11 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_BINPATH          (12 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_INPATH           (13 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_OUTPATH          (14 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_TEMPPATH         (15 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_ACCESSDOC        (16 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_ACCESSBIN        (17 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_ACCESSIN         (18 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_ACCESSOUT        (19 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_ACCESSTEMP       (20 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_DBSYMBOL         (21 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_USEGDSMATRIX     (22 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_IMG              (23 + WINDOWS_ID_BIAS)
	#define CTL_STATUS_LOGPATH          (24 + WINDOWS_ID_BIAS)
#define DLG_WAIT                   10
	#define CTL_WAIT_TEXT                (1 + WINDOWS_ID_BIAS)
#define DLG_COUNTRY                20 // COUNTRY
#define DLG_REGION                 21 // REGION
#define DLG_CITY                   22 // CITY
#define DLG_STREET                993 // STREET
#define DLG_CITYAREA             1137 // CITYAREA
	#define CTL_WORLD_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_ABBR              (2 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_PHONE             (3 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_HASREGIONS        (4 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_COUNTRY           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_WORLD_COUNTRY        (5 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_PARENT            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_WORLD_PARENT         (7 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_STATUS            (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_WORLD_STATUS         (9 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_CODE             (10 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_ZIP              (11 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_COORD            (12 + WINDOWS_ID_BIAS)
	#define CTL_WORLD_ID               (13 + WINDOWS_ID_BIAS)
//
// Диалоги, связанные с персоналиями
//
#define DLG_PERSONSTATUS           23
#define DLG_PERSON                 24 // @v2.2.4
#define DLG_PERSONEXT              25 // @v2.4.6
#define DLG_PERSON_BNK            598 // @v4.2.7
#define DLG_PERSON_S1            1152 // PSN_S1
	#define cmPersonReg               111 // Регистры
	#define cmPersonPhones           2002 // Электронные адреса
	#define cmPersonBAcct             112 // Список банковских счетов
	#define cmSelectAnalog            113 // @v4.2.12 Выбор аналогичной персоналии
	#define cmCashierRights           114 // @v4.9.9  Права кассира
	#define cmPersonRegFree          2004 // Регистр частного предпринимателя //
	#define cmPersonAddr             2005 // Юридический адрес
	#define cmPersonRAddr            2006 // Фактический адрес
	#define cmPersonTags             2007 // Теги персоналии
	#define cmPersonDlvrLocList      2008 // Список адресов доставки
	#define cmPersonRelList          2009 // Список отношений персоналии
	#define cmWaitImage               115 // Ожидание появления в папке (PersonConfig->ImageFolder) нового файла
	#define cmFullPersonDialog        116 // Кнопка вызова полного диалога персоналии (для сокращенных диалогов)
	#define cmFullSCardDialog         115 // Кнопка вызова полного диалога персональной карты (для сокращенных диалогов)
	#define CTL_PERSON_NAME              (2 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_KIND              (9 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_FLAGS            (18 + WINDOWS_ID_BIAS) // Флаги
	#define CTL_PERSON_MEMO             (22 + WINDOWS_ID_BIAS) // Строка примечания //
	#define CTL_PERSON_STATUS           (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSON_STATUS        (24 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_DIV              (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSON_DIV           (26 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_EXTNAME          (27 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_BNK_BIC          (28 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_BNK_CORRACC      (29 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SELANALOGBUT     (30 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_CATEGORY         (31 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSON_CATEGORY      (32 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_CSHRRIGHTS       (33 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_IMAGE            (34 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_ID               (35 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SRCHCODE         (36 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_PHONE            (37 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCARD            (38 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_DOB              (39 + WINDOWS_ID_BIAS) // Дата рождения //
	#define CTLCAL_PERSON_DOB           (40 + WINDOWS_ID_BIAS) //
	#define CTL_PERSON_ST_KINDNAME      (41 + WINDOWS_ID_BIAS) // Для сокращенного диалога отображает единственный вид, которому принадлежит персоналия //
	#define CTL_PERSON_ST_SCARDSER      (42 + WINDOWS_ID_BIAS) // Для сокращенного диалога отображает серию карт, которой должна принадлежать вводимая карта
	#define CTL_PERSON_ST_SCARDINFO     (43 + WINDOWS_ID_BIAS) //
	#define CTL_PERSON_SCARDAUTO        (44 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCARDSER         (45 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSON_SCARDSER      (46 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCEXPIRY         (47 + WINDOWS_ID_BIAS)
	#define CTLCAL_PERSON_SCEXPIRY      (48 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCDIS            (49 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCFRAME          (50 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCAG             (51 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSON_SCAG          (52 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_SCTIME           (53 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_ST_INFO          (54 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_FULLSCARDBUT     (55 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_GENDER           (56 + WINDOWS_ID_BIAS)
	#define CTL_PERSON_EMAIL            (57 + WINDOWS_ID_BIAS)
#define DLG_REG_PRIVATE            26 // Регистр. данные физ. лиц
#define DLG_REG_FREE               27 // Регистр. данные частных предпр.
#define DLG_REG_LEGAL              28 // Регистр. данные юр. лиц
#define DLG_REG_BANK               29 // Регистр. данные банков
#define DLG_REGISTER               36 // REGISTER @v2.2.4 Common register dialog
	#define CTL_REG_BIRTHDAY             (1 + WINDOWS_ID_BIAS)
	#define CTL_REG_DATE                 (2 + WINDOWS_ID_BIAS)
	#define CTL_REG_SERIALNO             (3 + WINDOWS_ID_BIAS)
	#define CTL_REG_NUMBER               (4 + WINDOWS_ID_BIAS)
	#define CTL_REG_ORGAN                (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_REG_ORGAN             (6 + WINDOWS_ID_BIAS)
	#define CTL_REG_STATUS               (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_REG_STATUS            (8 + WINDOWS_ID_BIAS)
	#define CTL_REG_REGTYP               (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_REG_REGTYP           (10 + WINDOWS_ID_BIAS)
	#define CTL_REG_PNAME               (11 + WINDOWS_ID_BIAS)
	#define CTL_REG_EXPIRY              (12 + WINDOWS_ID_BIAS)
	#define CTL_REG_SERLEN              (13 + WINDOWS_ID_BIAS)
	#define CTL_REG_NUMLEN              (14 + WINDOWS_ID_BIAS)
	#define CTLCAL_REG_DATE             (15 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_REG_EXPIRY           (16 + WINDOWS_ID_BIAS) // @alena
	#define CTL_REG_ID                  (17 + WINDOWS_ID_BIAS) // @v8.2.7
	#define CTL_REG_LINKOBJTEXT         (18 + WINDOWS_ID_BIAS) // @v8.3.6
#define DLG_PHONES_LEGAL           30 // Телефоны, факсы, E-mail
#define DLG_PHONES_PRIVATE         31
	#define CTL_PHONES_WORK              (1 + WINDOWS_ID_BIAS)
	#define CTL_PHONES_HOME              (2 + WINDOWS_ID_BIAS)
	#define CTL_PHONES_ALT               (3 + WINDOWS_ID_BIAS)
	#define CTL_PHONES_FAX               (4 + WINDOWS_ID_BIAS)
	#define CTL_PHONES_EMAIL             (5 + WINDOWS_ID_BIAS)
#define DLG_BACCT                  32 // BACCT Банковские счета
	#define CTL_BACCT_LIST               (1 + WINDOWS_ID_BIAS)
	#define CTL_BACCT_BANK               (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_BACCT_BANK            (3 + WINDOWS_ID_BIAS)
	#define CTL_BACCT_ACCTYPE            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BACCT_ACCTYPE         (5 + WINDOWS_ID_BIAS)
	#define CTL_BACCT_ACCT               (6 + WINDOWS_ID_BIAS)
	#define CTL_BACCT_OPENDATE           (7 + WINDOWS_ID_BIAS)
	#define CTL_BACCT_FLAGS              (8 + WINDOWS_ID_BIAS)
	#define CTL_BACCT_ACCTLEN            (9 + WINDOWS_ID_BIAS)
    #define CTLCAL_BACCT_OPENDATE        (10 + WINDOWS_ID_BIAS) // @alena
	#define CTL_BACCT_BIC                (11 + WINDOWS_ID_BIAS)
#define DLG_BACCLST                33 // Список банковских счетов
	#define CTL_BACCLST_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_BACCLST_NAME             (2 + WINDOWS_ID_BIAS) // StaticText имя персоналии
//                                 35 // Occupied
//                                 36 // Occupied
#define DLG_PASSWORD               39 // Ввод пароля //
	#define CTL_PASSWORD_FIRST           (1 + WINDOWS_ID_BIAS)
	#define CTL_PASSWORD_SECOND          (2 + WINDOWS_ID_BIAS)
	#define CTL_PASSWORD_IMG             (3 + WINDOWS_ID_BIAS)
#define DLG_USRGRP                 40 // Группа пользователей
	#define cmCfgPath                 101
	#define cmCfgConfig               102
		#define cmCfgRound                101
		#define cmCfgCurrency             102
	#define cmCfgRights               103
	#define cmCfgPassword             104
	#define CTL_USRGRP_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_USRGRP_ID                (9 + WINDOWS_ID_BIAS)
	#define CTL_USRGRP_PATH             (10 + WINDOWS_ID_BIAS) // Button "Пути..."
	#define CTL_USRGRP_CFG              (11 + WINDOWS_ID_BIAS) // Button "Конфигурация..."
	#define CTL_USRGRP_RIGHTS           (13 + WINDOWS_ID_BIAS) // Button "Права..."
	#define CTL_USRGRP_SYMB             (14 + WINDOWS_ID_BIAS)
#define DLG_USR                    41 // Пользователь
	#define CTL_USR_NAME                 (1 + WINDOWS_ID_BIAS)
	#define CTL_USR_GRP                  (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_USR_GRP               (5 + WINDOWS_ID_BIAS)
	#define CTL_USR_EQUIVALENT           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_USR_EQUIVALENT        (7 + WINDOWS_ID_BIAS)
	#define CTL_USR_FLAGS                (8 + WINDOWS_ID_BIAS)
	#define CTL_USR_ID                   (9 + WINDOWS_ID_BIAS) // == CTL_USRGRP_ID
	#define CTL_USR_PATH                (10 + WINDOWS_ID_BIAS) // Button "Пути..."
	#define CTL_USR_CFG                 (11 + WINDOWS_ID_BIAS) // Button "Конфигурация..."
	#define CTL_USR_PASSWORD            (12 + WINDOWS_ID_BIAS) // Button "Пароль..."
	#define CTL_USR_RIGHTS              (13 + WINDOWS_ID_BIAS) // Button "Права..."
	#define CTL_USR_PERSON              (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_USR_PERSON           (15 + WINDOWS_ID_BIAS)
	#define CTL_USR_UER                 (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_USR_UER              (17 + WINDOWS_ID_BIAS)
	#define CTL_USR_UERFLAGS            (18 + WINDOWS_ID_BIAS)
	#define CTL_USR_EXPIRY              (19 + WINDOWS_ID_BIAS)
	#define CTL_USR_SYMB                (20 + WINDOWS_ID_BIAS)
	#define CTL_USR_PDESKTOP            (21 + WINDOWS_ID_BIAS) // @v11.0.0
	#define CTLSEL_USR_PDESKTOP         (22 + WINDOWS_ID_BIAS) // @v11.0.0
#define DLG_GRPNUSR                42 // Просмотр групп и пользователей
	#define cmAddSecurGrp                111
	#define cmEditSecurGrp               112
	#define cmDelSecurGrp                113
	#define cmAddSecurUser               114
	#define cmEditSecurUser              115
	#define cmDelSecurUser               116
	#define CTL_GRPNUSR_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_GRPNUSR_GRP              (2 + WINDOWS_ID_BIAS)
	#define CTL_GRPNUSR_USR              (3 + WINDOWS_ID_BIAS)
#define DLG_PATH                   43 // Пути поиска файлов
	#define CTL_PATH_DRV                 (1 + WINDOWS_ID_BIAS)
	#define CTL_PATH_ROOT                (2 + WINDOWS_ID_BIAS)
	#define CTL_PATH_BIN                 (3 + WINDOWS_ID_BIAS)
	#define CTL_PATH_DAT                 (4 + WINDOWS_ID_BIAS)
	#define CTL_PATH_ARC                 (5 + WINDOWS_ID_BIAS)
	#define CTL_PATH_IN                  (6 + WINDOWS_ID_BIAS)
	#define CTL_PATH_OUT                 (7 + WINDOWS_ID_BIAS)
	#define CTL_PATH_TMP                 (8 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_DRV             (201 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_ROOT            (202 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_BIN             (203 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_DAT             (204 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_ARC             (205 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_IN              (206 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_OUT             (207 + WINDOWS_ID_BIAS)
	#define CTL_PATH_D_TMP             (208 + WINDOWS_ID_BIAS)
#define DLG_CONFIG                 44 // Полный аналог DLG_USRGRP
#define DLG_UNIT                   50 // Единица измерения //
	#define CTL_UNIT_NAME                (1 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_ABBR                (2 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_KIND                (3 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_FLAGS               (4 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_ID                  (9 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_BASE                (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_UNIT_BASE             (11 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_BASERATIO           (12 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_ROUNDING            (13 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_CODE                (14 + WINDOWS_ID_BIAS)
	#define CTL_UNIT_FRAGM               (15 + WINDOWS_ID_BIAS)
#define DLG_COMMOBJ                51 // Общий диалог редактирования объекта
	#define CTL_COMMOBJ_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_COMMOBJ_SYMB             (2 + WINDOWS_ID_BIAS)
	#define CTL_COMMOBJ_ID               (9 + WINDOWS_ID_BIAS)
	#define CTL_COMMOBJ_FLAGS            (10 + WINDOWS_ID_BIAS)
	#define CTL_COMMOBJ_PARENT           (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_COMMOBJ_PARENT        (12 + WINDOWS_ID_BIAS)
#define DLG_OBJVIEW                52 // Общий диалог просмотра списка объектов
	#define CTL_OBJVIEW_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_OBJVIEW_ST_MSG           (2 + WINDOWS_ID_BIAS)
#define DLG_GEO                    53 // Георафические объекты
	#define CTL_GEO_LIST                 (1 + WINDOWS_ID_BIAS)
#define DLG_ACCSHEET               54 // Таблица аналитических статей
	#define CTL_ACCSHEET_NAME            (1 + WINDOWS_ID_BIAS)
	// @v11.3.12 #define CTL_ACCSHEET_ASSOC_Removed   (2 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_GROUP           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCSHEET_GROUP        (4 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_ARTICLES        (5 + WINDOWS_ID_BIAS) // Кнопка "Статьи..."
	#define CTL_ACCSHEET_FLAGS           (6 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_BIN             (7 + WINDOWS_ID_BIAS) // @v3.4.5 Project
	#define CTLSEL_ACCSHEET_BIN          (8 + WINDOWS_ID_BIAS) // @v3.4.5 Project
	#define CTL_ACCSHEET_AGTKIND         (9 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_REGTYPE         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCSHEET_REGTYPE      (11 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_SYMB            (12 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_ID              (13 + WINDOWS_ID_BIAS)
	#define CTL_ACCSHEET_ASSCOBJ         (14 + WINDOWS_ID_BIAS) // @v11.3.12
	#define CTLSEL_ACCSHEET_ASSCOBJ      (15 + WINDOWS_ID_BIAS) // @v11.3.12
#define DLG_ARTICLE                55 // Аналитическая статья //
#define DLG_ARTICLEAUTO            57
#define DLG_ARTICLEGROUP          808 // Группирующая статья аналитического учета
#define DLG_ARTICLE_RL           1041 // Аналитическая статья с возможностью выбора связанного объекта
// @v8.2.4 #define cmCliAgt                     101
#define cmClearAgreement             102
	#define CTL_ARTICLE_NUMBER           (1 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_NAME             (2 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_ACCESS           (3 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_CLOSED           (4 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_ACC              (5 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_ACCNAME          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_ARTICLE_ACCNAME       (7 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_SHEETNAME        (8 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_ALIASSUBST       (9 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_FLAGS            (10 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_AGTISEMPTY       (11 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_LINKOBJ          (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_ARTICLE_LINKOBJ       (13 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLE_ID               (14 + WINDOWS_ID_BIAS)
#define DLG_ACCOUNT                58 // Балансовый счет
#define DLG_ACCREGISTER            59 // Бухгалтерский регистр
	#define CTL_ACCOUNT_NUMBER           (1 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_SUBNUMBER        (2 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_NAME             (3 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_ACCSHEET         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCOUNT_ACCSHEET      (5 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_TYPE             (6 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_FLAGS            (7 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_OVERDRAFT        (8 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_LIMIT            (9 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_ACCESS          (10 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_CURLIST         (11 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_OUTBAL          (12 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_CODE            (13 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_AUTONUMBER      (14 + WINDOWS_ID_BIAS)
	#define CTL_ACCOUNT_ST_ACSMSG       (15 + WINDOWS_ID_BIAS) // Текст предупреждения о наличии связанных с
	#define CTL_ACCOUNT_PARENT          (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCOUNT_PARENT       (17 + WINDOWS_ID_BIAS)
		// аналитической таблицей проводок
#define DLG_OPRTYPE                60 // Тип операции
	#define CTL_OPRTYPE_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_OPRTYPE_PICT             (2 + WINDOWS_ID_BIAS)
	#define CTL_OPRTYPE_ID               (9 + WINDOWS_ID_BIAS)
#define DLG_OPRKIND                61 // Вид операции
	#define cmResetCounter            101
	#define cmOprKindPrnOpt           102
	#define cmOprKindExAmt            103
	#define cmOprKindPaymList         104
	#define cmOprKindCounter          105
	#define cmEditOp                  106
	#define cmOprKindExt              107 // Расширение подтипа
	#define CTL_OPRKIND_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_TYPE             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_TYPE          (3 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_LIST             (4 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_COUNTER          (5 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_RESETCOUNTER     (6 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_ACCSHEET         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_ACCSHEET      (8 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_ID               (9 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_LINK            (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_LINK         (11 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_FLAGS           (12 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_CVAL            (13 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_ACCESS          (14 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_PAYMF           (15 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_PASSIVE         (16 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_RANK            (17 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_ACCSHEET2       (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_ACCSHEET2    (19 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_WROFFOP         (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_WROFFOP      (21 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_WROFFOBJ        (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_WROFFOBJ     (23 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_WROFFCOP        (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_WROFFCOP     (25 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_DRFTOPTION      (26 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_SYMB            (27 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_INITST          (28 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRKIND_INITST       (29 + WINDOWS_ID_BIAS)
	#define CTL_OPRKIND_EXPSYMB         (30 + WINDOWS_ID_BIAS)
#define DLG_AMOUNTTYPE             62 // Тип суммы документа
	#define cmDistribCost             101
	#define CTL_AMOUNTTYPE_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_SYMB          (2 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_FLAGS         (3 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_DEFAULT       (4 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_ID            (9 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_TAX           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_AMOUNTTYPE_TAX        (11 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_TAXRATE       (12 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_REPLACE       (13 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_KIND          (14 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_REFAMT        (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_AMOUNTTYPE_REFAMT     (16 + WINDOWS_ID_BIAS)
	#define CTL_AMOUNTTYPE_FORMULA       (17 + WINDOWS_ID_BIAS)
#define DLG_ATRNTMPL               63 // Шаблон бух. проводки
	#define cmAturnTmplSelAmtSymb     101
	#define CTL_ATRNTMPL_DOC             (1 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DATE            (2 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DTEXT           (3 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_CTEXT           (4 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DFIX            (5 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_CFIX            (6 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_PRIMARY         (7 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DACC           (10 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DACCNAME       (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATRNTMPL_DACCNAME    (12 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DART           (13 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_DARTNAME       (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATRNTMPL_DARTNAME    (15 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_CACC           (16 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_CACCNAME       (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATRNTMPL_CACCNAME    (18 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_CART           (19 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_CARTNAME       (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATRNTMPL_CARTNAME    (21 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_PSUBST         (22 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_FSUBST         (23 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_SFRAME         (24 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_AMOUNT         (30 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_SKIPNEG        (31 + WINDOWS_ID_BIAS)
	#define CTL_ATRNTMPL_PERIOD         (32 + WINDOWS_ID_BIAS)
	#define CTLCAL_ATRNTMPL_PERIOD      (33 + WINDOWS_ID_BIAS)
#define DLG_GGRPKIND              354 // Выбор вида группы товаров
	#define CTL_GGRPKIND_KIND            (1 + WINDOWS_ID_BIAS)
#define DLG_GOODSGROUP             70 // Группа товаров
#define DLG_GGRPFOLD              355 // Группа верхнего уровня
#define DLG_GGRPALT               356 // Альтернативная группа
	#define cmGoodsExTitles           104
	#define CTL_GOODSGROUP_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_GOODSGROUP_CODE          (2 + WINDOWS_ID_BIAS)
	#define CTL_GOODSGROUP_VATAX         (3 + WINDOWS_ID_BIAS) // НДС
	#define CTL_GOODSGROUP_EXCISE        (4 + WINDOWS_ID_BIAS) // Акциз
	#define CTL_GOODSGROUP_UNIT          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSGROUP_UNIT       (7 + WINDOWS_ID_BIAS)
	#define CTL_GOODSGROUP_PHUNIT        (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSGROUP_PHUNIT     (9 + WINDOWS_ID_BIAS)
	#define CTL_GOODSGROUP_ALT          (10 + WINDOWS_ID_BIAS)
	#define CTL_GOODSGROUP_TYPE         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSGROUP_TYPE      (13 + WINDOWS_ID_BIAS)
	#define CTL_GOODSGROUP_LIMIT        (14 + WINDOWS_ID_BIAS)
// Диалог просмотра групп товаров
#define DLG_GGVIEW                 71
	#define CTL_GGVIEW_GGRPTYPE     (1 + WINDOWS_ID_BIAS)
	#define CTL_GGVIEW_LIST         (2 + WINDOWS_ID_BIAS)
#define DLG_GOODS                  72 // Товар
	#define cmGoodsEditTax            602
	#define cmGoodsExt                131
	#define cmGoodsValueAddedData     132
	#define cmGoodsWrOffGrp           133
	#define cmEditObjAssoc            134
	#define cmArGoodsCodeList         139
	#define cmGoodsStat               140
	#define CTL_GOODS_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_GROUP              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_GROUP           (3 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_MANUF              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_MANUF           (5 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_UNIT               (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_UNIT            (7 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_PHUNIT             (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_PHUNIT          (9 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_PHUPERU           (10 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_ABBR              (11 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_BARCODE           (12 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_VATAX             (13 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_EXCISE            (14 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_TYPE              (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_TYPE           (16 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_TAX               (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_TAX            (18 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_GENERIC           (19 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_FLAGS             (20 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_FLDFLAGS          (21 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_CLS               (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_CLS            (23 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_WROFFBUTTON       (24 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_WROFFGRP          (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_WROFFGRP       (26 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_ASSETFLAGS        (27 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_BRAND             (28 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_BRAND          (29 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_ID                (30 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_OKOF              (31 + WINDOWS_ID_BIAS) // For DLG_ASSET Код ОКОФ
	#define CTL_GOODS_BCODESTRUC        (32 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_BCODESTRUC     (33 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_DYNAMIC           (34 + WINDOWS_ID_BIAS) // DLG_GGRPALT
	#define CTL_GOODS_ST_INH_TAXGRP     (35 + WINDOWS_ID_BIAS) // DLG_GGRPALT
	#define CTL_GOODS_IMAGE             (36 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_OWNARCODE         (37 + WINDOWS_ID_BIAS) // Индикатор собственного артикула
	#define CTL_GOODS_DEFPRC            (38 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_DEFPRC         (39 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_DEFGSTRUC         (40 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODS_DEFGSTRUC      (41 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_ST_INFO           (42 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_RANK              (43 + WINDOWS_ID_BIAS)
	#define CTL_GOODS_GRPFLAGS          (44 + WINDOWS_ID_BIAS)
#define DLG_LOCATION               73 // Location (Warehouse)
#define DLG_LOCATION2              75 // @v2.2.4 Location
#define DLG_ADDRESS                76 // @v2.2.4 Address (Location)
#define DLG_WAREPLACE              77 // @v2.2.4 Wareplace (Location)
#define DLG_DIVISION               79 // @v4.0.5 Division (Location)
#define DLG_WHCOLUMN             1063 // WHCOLUMN
#define DLG_WHCELL               1064 // WHCELL
	#define cmExtFields                  110
	#define cmLocRegisters               111
	#define cmLocTags                    112
	#define cmLocAction1                 113
	#define CTL_LOCATION_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_CODE            (2 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_OWNER           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCATION_OWNER        (5 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_FRAME           (6 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_ZIP             (7 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_COUNTRY         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCATION_COUNTRY      (9 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_CITY           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCATION_CITY        (11 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_ADDR           (12 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_RSPNS          (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCATION_RSPNS       (14 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_ID             (15 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_FULLADDR       (16 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_CAPACITY       (17 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_SZWD           (18 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_SZLN           (19 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_SZHT           (20 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_VOLUME         (21 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_VOLUMEVAL      (22 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_PARENT         (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCATION_PARENT      (24 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_ST_EXTTITLE    (25 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_NUMROWS        (26 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_NUMLAYERS      (27 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_COORD          (28 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_ST_ADDR        (29 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_OWNERNAME      (30 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_WHFLAGS        (31 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_DEPTH          (32 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_PHONE          (33 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_CONTACT        (34 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_EMAIL          (35 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_ACN1           (36 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_WHLIST         (37 + WINDOWS_ID_BIAS)
	#define CTL_LOCATION_PASSIVE        (38 + WINDOWS_ID_BIAS)

#define DLG_ATURN                  74 // Бух. проводка
#define DLG_REGATURN               78 // Регистровая проводка
	#define CTL_ATURN_DOC                (1 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_DATE               (2 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_DTEXT              (3 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CTEXT              (4 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_DACC              (10 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_DACCNAME          (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATURN_DACCNAME       (12 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_DART              (13 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_DARTNAME          (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATURN_DARTNAME       (15 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CACC              (16 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CACCNAME          (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATURN_CACCNAME       (18 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CART              (19 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CARTNAME          (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATURN_CARTNAME       (21 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_AMOUNT            (30 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_MEMO              (31 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CUR               (32 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATURN_CUR            (33 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_CRATE             (34 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_BASEAMT           (35 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_OP                (36 + WINDOWS_ID_BIAS)
	#define CTLCAL_ATURN_DATE           (37 + WINDOWS_ID_BIAS)
	#define CTL_ATURN_LOCATION          (38 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATURN_LOCATION       (39 + WINDOWS_ID_BIAS)
//                                 75, 76, 77, 78, 79 // Occupied
#define DLG_BALFORM                80 // Фильтр бух. баланса
	#define CTL_BALFORM_PERIOD           (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_BALFORM_PERIOD        (2 + WINDOWS_ID_BIAS)
	#define CTL_BALFORM_ACCTYPE          (3 + WINDOWS_ID_BIAS)
	#define CTL_BALFORM_OPTIONS          (4 + WINDOWS_ID_BIAS)
	#define CTL_BALFORM_SPREAD           (5 + WINDOWS_ID_BIAS)
	#define CTL_BALFORM_CUR              (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_BALFORM_CUR           (7 + WINDOWS_ID_BIAS)
	#define CTL_BALFORM_ALLCUR           (8 + WINDOWS_ID_BIAS)
// Просмотр таблиц аналитического учета (аналогичен DLG_OBJVIEW)
#define DLG_ACCSHEETVIEW           81
#define DLG_ACCANLZ                82 // Фильтр анализа счета
#define DLG_CASHBOOK               83 // Кассовая книга
#define DLG_ACCANLZ2              563 // Расширение фильтра анализа счета
	#define CTL_ACCANLZ_PERIOD           (1 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ACCGRP           (3 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_CORACCGRP        (4 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ACC             (10 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ACCNAME         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_ACCNAME      (12 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ART             (13 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ARTNAME         (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_ARTNAME      (15 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_LEAF            (16 + WINDOWS_ID_BIAS) // Лист (для кассовой книги)
	#define CTL_ACCANLZ_LABEL           (17 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_CUR             (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_CUR          (19 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ALLCUR          (20 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_TRNOVR          (21 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_CYCLE           (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_CYCLE        (23 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_NUMCYCLES       (24 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ORDER           (25 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_EXCLINNRT       (26 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ST_GRPNG        (27 + WINDOWS_ID_BIAS)
	#define CTLCAL_ACCANLZ_PERIOD       (28 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_LOC             (29 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_LOC          (30 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_AGENT           (31 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_AGENT        (32 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_SUBST           (33 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_SUBST        (34 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_RELSUBST        (35 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_RELSUBST     (36 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_DLVRLOC         (37 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCANLZ_DLVRLOC      (38 + WINDOWS_ID_BIAS)
	#define CTL_ACCANLZ_ACS             (39 + WINDOWS_ID_BIAS) // @v12.2.0
	#define CTLSEL_ACCANLZ_ACS          (40 + WINDOWS_ID_BIAS) // @v12.2.0
	#define CTL_ACCANLZ_OBJ2            (41 + WINDOWS_ID_BIAS) // @v12.2.0
	#define CTLSEL_ACCANLZ_OBJ2         (42 + WINDOWS_ID_BIAS) // @v12.2.0
#define DLG_QCERT                  90 // Сертификат качества
	#define CTL_QCERT_CODE               (1 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_BCODE              (2 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_GOODS              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QCERT_GOODS           (4 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_PRODUCEDATE        (5 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_INITDATE           (6 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_EXPIRY             (7 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_REGORGAN           (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_QCERT_REGORGAN        (9 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_MANUF             (10 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_SPRDATE           (11 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_ETC               (12 + WINDOWS_ID_BIAS)
	#define CTLCAL_QCERT_INITDATE       (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_QCERT_EXPIRY         (14 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_PASSIVE           (15 + WINDOWS_ID_BIAS)
	#define CTL_QCERT_INNERCODE         (16 + WINDOWS_ID_BIAS)
#define DLG_LOTITEM                91 // Поступление от поставщика
#define DLG_SELLITEM               92 // Продажа (клиент, розница)
#define DLG_INTRITEM               93 // Межсклад
#define DLG_REVALITEM              94 // Переоценка
#define DLG_PSELLITEM              95 // Продажа с выбором товара по цене
#define DLG_ORDSELLITEM            96 // Продажа по заказу
#define DLG_ORDLOTITEM             97 // Поступление заказа
#define DLG_SLOTITEM               98 // Поступление товара с полем поставщика
#define DLG_ACKITEM                99 // Подтверждение приемки товара
#define DLG_ASSETLOTITEM          100 // Поступление объекта основных средств
#define DLG_ASSETEXPLITEM         111 // Ввод в эксплуатацию объекта основных средств
#define DLG_SELLITEMDRAFT         694 // Продажа (клиент, розница)
#define DLG_QUOTEREQSEQ           122 // Последовательный запрос котировки у поставщика в ответ на запрос от покупателя //
#define DLG_TI_LOCTR             1135 // @unused Складская операция //
	#define cmQCert                   101
	#define cmLot                     102
	#define cmOrdLot                  104
	#define cmLotDim                  105
	#define cmSourceSerial            106
	#define cmVetisMatch              107
	#define CTL_LOT_GOODSGRP             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOT_GOODSGRP          (2 + WINDOWS_ID_BIAS)
	#define CTL_LOT_GOODS                (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOT_GOODS             (4 + WINDOWS_ID_BIAS)
	#define CTL_LOT_UNITPERPACK          (5 + WINDOWS_ID_BIAS)
	#define CTL_LOT_PACKS                (6 + WINDOWS_ID_BIAS)
	#define CTL_LOT_QUANTITY             (7 + WINDOWS_ID_BIAS)
	#define CTL_LOT_QTTY                 (7 + WINDOWS_ID_BIAS) // @alias(CTL_LOT_QUANTITY)
	#define CTL_LOT_COST                 (8 + WINDOWS_ID_BIAS)
	#define CTL_LOT_PRICE                (9 + WINDOWS_ID_BIAS)
	#define CTL_LOT_DISCOUNT            (10 + WINDOWS_ID_BIAS)
	#define CTL_LOT_QCERT               (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOT_QCERT            (12 + WINDOWS_ID_BIAS)
	#define CTL_LOT_LOT                 (20 + WINDOWS_ID_BIAS) // Кнопка "Партия..."
	#define CTL_LOT_STREST              (22 + WINDOWS_ID_BIAS) // StaticText "Остаток"
	#define CTL_LOT_OLDCOST             (23 + WINDOWS_ID_BIAS) // Для переоценки
	#define CTL_LOT_OLDPRICE            (24 + WINDOWS_ID_BIAS) // Для переоценки
	#define CTL_LOT_ORDQTTY             (25 + WINDOWS_ID_BIAS) // Заказанное количество
	#define CTL_LOT_ORDPRICE            (26 + WINDOWS_ID_BIAS) // Цена из заказа
	#define CTL_LOT_SUPPL               (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOT_SUPPL            (28 + WINDOWS_ID_BIAS)
	#define CTL_LOT_EXPIRY              (29 + WINDOWS_ID_BIAS)
	#define CTL_LOT_QUOTSIGN            (30 + WINDOWS_ID_BIAS) // Значок котировки
	#define CTL_LOT_CLB                 (31 + WINDOWS_ID_BIAS) // Грузовая таможенная деклараци
	#define CTL_LOT_CURPRICE            (32 + WINDOWS_ID_BIAS)
	#define CTL_LOT_CURSYMB             (33 + WINDOWS_ID_BIAS)
	#define CTL_LOT_INTAXGRP            (34 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOT_INTAXGRP         (35 + WINDOWS_ID_BIAS) //
	#define CTL_LOT_NOVAT               (36 + WINDOWS_ID_BIAS) // Цена поступления без НДС
	#define CTL_LOT_NOVATTAG            (37 + WINDOWS_ID_BIAS) // Ярлык "Без НДС"
	#define CTL_LOT_PHQTTY              (38 + WINDOWS_ID_BIAS) //
	#define CTL_LOT_SERIAL              (39 + WINDOWS_ID_BIAS) //
	#define CTL_LOT_IMMASSETEXPL        (40 + WINDOWS_ID_BIAS) // Флаг [Немедленный ввод в эксплуатацию]
	#define CTL_LOT_ASSETEXPL           (41 + WINDOWS_ID_BIAS) // Переключатель // [Ввод в эксплуатацию/Вывод из эксплуатации]
	#define CTL_LOT_ST_GOODSTAXGRP      (42 + WINDOWS_ID_BIAS) // Налоговая группа товара
	#define CTL_LOT_ST_VAPCT            (43 + WINDOWS_ID_BIAS) // Процент наценки
	#define CTL_LOT_ST_VATSUM           (44 + WINDOWS_ID_BIAS) // Сумма НДС по строке
	#define CTL_LOT_FIXEDMODIFCOST      (45 + WINDOWS_ID_BIAS) // Цена поступления по строке модификации фиксирована
	#define CTL_LOT_INDEPPHQTTY         (46 + WINDOWS_ID_BIAS) // Независимое от количества физическое количество
	#define CTL_LOT_ST_PHQTTY           (47 + WINDOWS_ID_BIAS) // Ярлык для полей CTL_LOT_INDEPPHQTTY и CTL_LOT_PHQTTY
	#define CTLCAL_LOT_EXPIRY           (48 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_GOODSGRP         (49 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_GOODS            (50 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_PACKS            (51 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_QUANTITY         (52 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_COST             (53 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_PRICE            (54 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_CURPRICE         (55 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOT_SERIAL           (56 + WINDOWS_ID_BIAS)
	#define CTL_LOT_BASEQUOT            (57 + WINDOWS_ID_BIAS)
	#define CTL_LOT_EXTCOST             (58 + WINDOWS_ID_BIAS)
	#define CTL_LOT_ST_PRICELIMIT       (59 + WINDOWS_ID_BIAS) // Индикация ограничения цены
	#define CTL_LOT_LOCCODE             (60 + WINDOWS_ID_BIAS) // Код складской ячейки
	#define CTL_LOT_LOC                 (61 + WINDOWS_ID_BIAS) // Наименование складской ячейки
	#define CTLSEL_LOT_LOC              (62 + WINDOWS_ID_BIAS)
	#define CTL_LOT_ST_BILLTITLE        (63 + WINDOWS_ID_BIAS)
	#define CTL_LOT_TM                  (64 + WINDOWS_ID_BIAS)
	#define CTL_LOT_LOCTROP             (65 + WINDOWS_ID_BIAS)
	#define CTL_LOT_LOTINFO             (66 + WINDOWS_ID_BIAS)
	#define CTL_LOT_ST_SERIALWARN       (67 + WINDOWS_ID_BIAS)
	#define CTL_LOT_ST_INFO             (68 + WINDOWS_ID_BIAS)
	#define CTL_LOT_ORGQTTY             (69 + WINDOWS_ID_BIAS)
	#define CTL_LOT_MANUF               (70 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOT_MANUF            (71 + WINDOWS_ID_BIAS)
	#define CTL_LOT_SOURCESERIAL        (72 + WINDOWS_ID_BIAS)
	#define CTL_LOT_VETISIND            (73 + WINDOWS_ID_BIAS)
	#define CTL_LOT_VETISMATCHBUTTON    (74 + WINDOWS_ID_BIAS)
	#define CTL_LOT_SEQQRACK            (75 + WINDOWS_ID_BIAS)
	#define CTL_LOT_PCFORWTQTY          (76 + WINDOWS_ID_BIAS) // @v12.1.4 Очень специфичное поле ввода: количество мест для весового маркируемого товара.
		// Отображается в тег строки документа PPTAG_LOT_CHZNINTQTTY

// Однотипные диалоги заполнения документов
#define DLG_BILL                  101
#define DLG_RCPTBILL              102 // Приход товара от поставщика
#define DLG_SELLBILL              103 // Продажа именованному клиенту
#define DLG_RETAILBILL            104 // Розничная продажа
#define DLG_INTRRCPT              105 // Межскладской приход
#define DLG_INTREXPD              106 // Межскладской расход
#define DLG_REVALBILL             107 // Переоценка
#define DLG_PAYMENT               108 // Оплата
#define DLG_WMSBILL               109 // @unused @v6.1.1 B_WMS Складской документ
#define DLG_SELLPBILL             112 // Продажа именованному клиенту + плательщик
#define DLG_MODIFBILL             113 // Модификация товара
#define DLG_ATEXT                 114 // Расширенная бух проводка
#define DLG_INVENTORY             115 // Инвентаризация (обслуживается отдельным классом)
#define DLG_CHARGE                116 // Начисление
#define DLG_ACKBILL               117 // Подтверждение
#define DLG_BILLPOOL              118 // Пул документов
#define DLG_ORDERBILL             119 // Документ заказа
#define DLG_C_TRANS               120 //
#define DLG_ATWROFF               121 // Бухгалтерский документ списания по счетам
	#define cmDetail                  101
	#define cmExAmountList            102
	#define cmFilteredDetail          104
	#define cmInvWriteOff             105 // Списать инвентаризацию
	#define cmInvRollback             106 // Откат списания //
	#define cmRentCondition           107 // Условия ренты
	#define cmBillExtra               108 // Дополнительная информация по документу
	#define cmPaymOrder               109 // Платежное поручение
	#define cmSelInRate               110 //
	#define cmSelOutRate              111 //
	#define cmBillTaxes               112 //
	#define cmBillFreight             114 // Фрахт
	#define cmAdvItems                115 // Строки расширения бух документа
	#define cmCalcPrevAdvRest         116 // Расчет остатка предыдущих авансов
	#define cmPrevAdvBills            117 // Просмотр предыдущих авансовых отчетов
	#define cmReckonBill              118 // Просмотр зачетного документа из зачитывающего
	#define cmLinkFiles               119 // Файлы привязанные к данному документу
	#define cmBillPayPlan             120 // План платежей по документу
	#define cmEdiAckBill              121 // Кнопка просмотра документа EDI-подтверждения от покупателя
	#define CTL_BILL_DOC                 (1 + WINDOWS_ID_BIAS)
	#define CTL_BILL_DATE                (2 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PERSON              (3 + WINDOWS_ID_BIAS) // @obsolete
	#define CTLSEL_BILL_PERSON           (4 + WINDOWS_ID_BIAS) // @obsolete
	#define CTL_BILL_OBJECT              (3 + WINDOWS_ID_BIAS) // Синоним CTL_BILL_PERSON
	#define CTLSEL_BILL_OBJECT           (4 + WINDOWS_ID_BIAS) // Синоним CTLSEL_BILL_PERSON
	#define CTL_BILL_LOCATION            (5 + WINDOWS_ID_BIAS) //
	#define CTLSEL_BILL_LOCATION         (6 + WINDOWS_ID_BIAS) //
	#define CTL_BILL_AMOUNT              (7 + WINDOWS_ID_BIAS)
	#define CTL_BILL_COST                (8 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PRICE               (9 + WINDOWS_ID_BIAS)
	#define CTL_BILL_DISCOUNT           (10 + WINDOWS_ID_BIAS)
	#define CTL_BILL_TTLDISCOUNT        (11 + WINDOWS_ID_BIAS) // ChkBox "Скидка на весь док."
	#define CTL_BILL_PAYDATE            (13 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PAYPLAN            (14 + WINDOWS_ID_BIAS) // Кнопка "План погашения..."
	#define CTL_BILL_DETAIL             (15 + WINDOWS_ID_BIAS) // Кнопка "Строки..."
	#define CTL_BILL_TURNS              (16 + WINDOWS_ID_BIAS) // Кнопка "Проводки..."
	#define CTL_BILL_DEBTSUM            (17 + WINDOWS_ID_BIAS)
	#define CTL_BILL_MEMO               (18 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PAYER              (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILL_PAYER           (20 + WINDOWS_ID_BIAS)
	#define CTL_BILL_STOCKAMT           (21 + WINDOWS_ID_BIAS) // @v2.2.5  DLG_INVENTORY
	#define CTL_BILL_SELEXSGOODS        (22 + WINDOWS_ID_BIAS) // @v2.2.11 DLG_INVENTORY
	#define CTL_BILL_RENTBUTTON         (23 + WINDOWS_ID_BIAS) // @v2.4.11 Кнопка "Рента..."
	#define CTL_BILL_PAYMORDBUTTON      (24 + WINDOWS_ID_BIAS) // @v2.9.8 Кнопка "Платежное поручение..."
	#define CTL_BILL_CUR                (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILL_CUR             (26 + WINDOWS_ID_BIAS)
	#define CTL_BILL_CRATE              (27 + WINDOWS_ID_BIAS)
	#define CTL_BILL_BASEAMT            (28 + WINDOWS_ID_BIAS)
	#define CTL_BILL_FRAME1             (29 + WINDOWS_ID_BIAS)
	#define CTL_BILL_FRAME2             (30 + WINDOWS_ID_BIAS)
	#define CTL_BILL_OUTCTAMOUNT        (31 + WINDOWS_ID_BIAS)
	#define CTL_BILL_OUTCTCUR           (32 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILL_OUTCTCUR        (33 + WINDOWS_ID_BIAS)
	#define CTL_BILL_OUTCTRATE          (34 + WINDOWS_ID_BIAS)
	#define CTL_BILL_OUTCTBASE          (35 + WINDOWS_ID_BIAS)
	#define CTL_BILL_CTRATE             (36 + WINDOWS_ID_BIAS)
	#define CTL_BILL_OBJ2               (37 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILL_OBJ2            (38 + WINDOWS_ID_BIAS)
	#define CTL_BILL_OBJ2NAME           (39 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ORDER              (40 + WINDOWS_ID_BIAS)
	#define CTL_BILL_CALB_DT            (41 + WINDOWS_ID_BIAS)
	#define CTL_BILL_CALB_PAYM          (42 + WINDOWS_ID_BIAS)
	#define CTL_BILL_TOGGLESTAX         (43 + WINDOWS_ID_BIAS)
	#define CTL_BILL_FREIGHTBUTTON      (44 + WINDOWS_ID_BIAS) // Кнопка "Фрахт"
	#define CTL_BILL_CLOSETAG           (45 + WINDOWS_ID_BIAS)
	// Added fields for advance report
	#define CTL_BILL_ADV_INREST         (46 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_RCPTXT1        (47 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_RCPDT1         (48 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_RCPAMT1        (49 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_RCPTXT2        (50 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_RCPDT2         (51 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_RCPAMT2        (52 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_TIN            (53 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_TOUT           (54 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_TREST          (55 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_NUMADDBILL     (56 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ADV_NUMADDSHEET    (57 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILL_DATE            (58 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILL_PAYDATE         (59 + WINDOWS_ID_BIAS)
	#define CTL_BILL_EXTINFO            (60 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ST_SCARD           (61 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ST_PARENTPERSON    (62 + WINDOWS_ID_BIAS)
	#define CTL_BILL_DUEDATE            (63 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILL_DUEDATE         (64 + WINDOWS_ID_BIAS)
	#define CTL_BILL_STATUS             (65 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILL_STATUS          (66 + WINDOWS_ID_BIAS)
	#define CTL_BILL_LNKFILELIST        (67 + WINDOWS_ID_BIAS)
	#define CTL_BILL_LINKFILESBUTTON    (68 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PAYPLANBUTTON      (69 + WINDOWS_ID_BIAS)
	#define CTL_BILL_BUT_LOCLIST        (70 + WINDOWS_ID_BIAS)
	#define CTLMNU_BILL_DOC             (71 + WINDOWS_ID_BIAS)
	#define CTLMNU_BILL_PAYDATE         (72 + WINDOWS_ID_BIAS)
	#define CTLMNU_BILL_MEMO            (73 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PAYMORD_IND        (74 + WINDOWS_ID_BIAS) // Индикатор существования платежного поручения //
	#define CTL_BILL_ST_LINKBILL        (75 + WINDOWS_ID_BIAS) // Связанный документ (строка описания)
	#define CTL_BILL_PERIOD             (76 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILL_PERIOD          (77 + WINDOWS_ID_BIAS)
	#define CTL_BILL_ST_DEBT            (78 + WINDOWS_ID_BIAS)
	#define CTL_BILL_DECLINE            (79 + WINDOWS_ID_BIAS)
	#define CTL_BILL_STRGLOC            (80 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILL_STRGLOC         (81 + WINDOWS_ID_BIAS)
	#define CTL_BILL_EDIACKRESP         (82 + WINDOWS_ID_BIAS)
	#define CTL_BILL_EDIACKSTATUS       (83 + WINDOWS_ID_BIAS)
	#define CTL_BILL_EXPIRY             (84 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILL_EXPIRY          (85 + WINDOWS_ID_BIAS)
	#define CTL_BILL_MAXCREDIT          (86 + WINDOWS_ID_BIAS)
	#define CTL_BILL_MAXDSCNT           (87 + WINDOWS_ID_BIAS)
	#define CTL_BILL_PAYPERIOD          (88 + WINDOWS_ID_BIAS)
	#define CTL_BILL_IND_AUTORCPT       (89 + WINDOWS_ID_BIAS)
	#define CTL_BILL_IND_WL             (90 + WINDOWS_ID_BIAS)
	#define CTL_BILL_TPLNPRD            (91 + WINDOWS_ID_BIAS) // @v12.1.12 Поле периода планирования для документа торгового плана
	#define CTLCAL_BILL_TPLNPRD         (92 + WINDOWS_ID_BIAS)
	#define CTL_BILL_TPLNLOC            (93 + WINDOWS_ID_BIAS) // @v12.1.12 Поле склада, возможно, группирующего, по которому ведется планирование и анализ.
		// Штатное поле склада документа, увы, не подходит из-за того, что существует необходимость применения группирующего склада.
	#define CTLSEL_BILL_TPLNLOC         (94 + WINDOWS_ID_BIAS)

#define DLG_REVAL                 110 // Переоценка
	#define CTL_REVAL_DOC                (1 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_DATE               (2 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_GOODSGRP           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REVAL_GOODSGRP        (4 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_GOODS              (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_REVAL_GOODS           (6 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_OLDCOST            (7 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_OLDPRICE           (8 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_OLDFRAME           (9 + WINDOWS_ID_BIAS) // StaticText with frame
	#define CTL_REVAL_NEWCOST           (10 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_NEWPRICE          (11 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_NEWFRAME          (12 + WINDOWS_ID_BIAS) // StaticText with frame
	#define CTL_REVAL_FLAGS             (13 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_LOT               (14 + WINDOWS_ID_BIAS) // Кнопка "Лот..."
	#define CTL_REVAL_SUPPL             (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_REVAL_SUPPL          (16 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_LOTTEXT           (17 + WINDOWS_ID_BIAS) // StaticText
	#define CTL_REVAL_MEMO              (18 + WINDOWS_ID_BIAS)
	#define CTL_REVAL_AMOUNT            (19 + WINDOWS_ID_BIAS)
//       112, 113, 114, 115, 116, 117, 118, 120, 122 // Occupied
#define DLG_FLTLOT                130 // Фильтр по лотам
#define DLG_FLTLOTORD             395 // Фильтр по лотам заказов
	#define CTL_FLTLOT_PERIOD            (1 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_CLOSED            (2 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_SUPPL             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTLOT_SUPPL          (4 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_GGRP              (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTLOT_GGRP           (6 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_GOODS             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTLOT_GOODS          (8 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_LOC               (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTLOT_LOC           (10 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_COST             (11 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_PRICE            (12 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_FLAGS            (13 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_EXPIRY           (14 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_OPERAT           (16 + WINDOWS_ID_BIAS)
	#define CTLCAL_FLTLOT_PERIOD        (17 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_QCEXPIRY         (18 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_SERIAL           (19 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_INTAXGRP         (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTLOT_INTAXGRP      (21 + WINDOWS_ID_BIAS)
	#define CTLCAL_FLTLOT_EXPIRY        (22 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_FLTLOT_QCEXPIRY      (23 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_FLTLOT_OPERAT        (24 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_SHOWPRICEDEV     (25 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_CDEVRESTR        (26 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_PDEVRESTR        (27 + WINDOWS_ID_BIAS)
	#define CTL_FLTLOT_EXTVIEWATTR      (28 + WINDOWS_ID_BIAS) // Выбор варианта отображения для обычных лотов (не заказов)
	#define CTL_FLTLOT_SPPLPSNCAT       (29 + WINDOWS_ID_BIAS) // @v11.4.4
	#define CTLSEL_FLTLOT_SPPLPSNCAT    (30 + WINDOWS_ID_BIAS) // @v11.4.4
	#define CTL_FLTLOT_ORDEXTVIEWATR    (31 + WINDOWS_ID_BIAS) // @v12.1.6 Выбор варианта отображения для заказов //
	#define CTL_FLTLOT_ORDVIEWFLAGS     (32 + WINDOWS_ID_BIAS) // @v12.1.6 Дополнительные флаги опций отображения //
	#define CTL_FLTLOT_AGENT            (33 + WINDOWS_ID_BIAS) // @v12.1.6
	#define CTLSEL_FLTLOT_AGENT         (34 + WINDOWS_ID_BIAS) // @v12.1.6
#define DLG_BILLFLT               131 // Фильтр реестра товарных документов
#define DLG_BILLFLTEXT            275 // (@obsolete v3.6.4) Расширение фильтра документов
#define DLG_BILLPREAMB            514 // Преамбула для ввода документа по коду операции и контрагента
	#define cmContractorTags             101 // @v11.9.6
	#define CTL_BILLFLT_PERIOD           (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_OPRKIND          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_OPRKIND       (3 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_OBJECT           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_OBJECT        (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_LOC              (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_LOC           (7 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_AMOUNT           (8 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_PAYM             (9 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_FLAGS           (11 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_LABEL           (40 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_OPRSET          (41 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_DIFFBYPRICE     (42 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_SUPPL           (43 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_SUPPL        (44 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_GGRP            (45 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_GGRP         (46 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_PAYER           (47 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_PAYER        (48 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_LOGFILE         (49 + WINDOWS_ID_BIAS) // Для диалогов проверки
	#define CTL_BILLFLT_INTRREVAL       (50 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_AGENT           (51 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_AGENT        (52 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_CUR             (53 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_CUR          (54 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_ALLCUR          (55 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_OBJ2            (56 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_OBJ2         (57 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_OBJ2NAME        (58 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLFLT_PERIOD       (59 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_INPCODE         (60 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_OBJCITY         (61 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_OBJCITY      (62 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_SUBSTGDS        (63 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_SUBSTGDS     (64 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_SUPPLAGNT       (65 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_SUPPLAGNT    (66 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_USEABCANLZ      (67 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_RESTDATE        (68 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLCAL_BILLFLT_RESTDATE     (69 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_QUOTKIND        (70 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_QUOTKIND     (71 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_PRICEDEV        (72 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_EACHLOC         (73 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_STATUS          (74 + WINDOWS_ID_BIAS) //
	#define CTLSEL_BILLFLT_STATUS       (75 + WINDOWS_ID_BIAS) //
	#define CTL_BILLFLT_ORDER           (76 + WINDOWS_ID_BIAS)
	#define CTLBRW_BILLFLT_LOGFILE      (77 + WINDOWS_ID_BIAS) // Для диалогов проверки
	#define CTLMNU_BILLFLT_RESTDATE     (78 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_SUBST           (79 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_SUBST        (80 + WINDOWS_ID_BIAS) // GoodsOpAnalyzeFilt
	#define CTL_BILLFLT_FRGHTAGNT       (83 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLFLT_FRGHTAGNT    (84 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_DUEPERIOD       (85 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLFLT_DUEPERIOD    (86 + WINDOWS_ID_BIAS)
	#define CTL_BILLFLT_MEMOTEXT        (87 + WINDOWS_ID_BIAS) // @v11.7.4
	#define CTLBUT_BILLFLT_OBJPTAGS     (88 + WINDOWS_ID_BIAS) // @v11.9.6 Кнопка фильтра по тегам персоналий контрагентов
	#define CTL_BILLFLT_MPGUA           (89 + WINDOWS_ID_BIAS) // @v12.1.12 GoodsOpAnalyzeFilt
	#define CTLSEL_BILLFLT_MPGUA        (90 + WINDOWS_ID_BIAS) // @v12.1.12 GoodsOpAnalyzeFilt
#define DLG_GTO                   133 // Фильтр товарооборота
	#define CTL_GTO_PERIOD               (1 + WINDOWS_ID_BIAS)
	#define CTL_GTO_SUPPL                (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_SUPPL             (4 + WINDOWS_ID_BIAS)
	#define CTL_GTO_GGRP                 (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_GGRP              (6 + WINDOWS_ID_BIAS)
	#define CTL_GTO_GOODS                (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_GOODS             (8 + WINDOWS_ID_BIAS)
	#define CTL_GTO_LOC                  (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_LOC              (10 + WINDOWS_ID_BIAS)
	#define CTL_GTO_LABEL               (11 + WINDOWS_ID_BIAS)
	#define CTL_GTO_OPR                 (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_OPR              (13 + WINDOWS_ID_BIAS)
	#define CTL_GTO_ORDER               (14 + WINDOWS_ID_BIAS)
	#define CTL_GTO_GENGGRP             (15 + WINDOWS_ID_BIAS)
	#define CTL_GTO_OBJECT              (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_OBJECT           (17 + WINDOWS_ID_BIAS)
	#define CTL_GTO_CYCLE               (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_CYCLE            (19 + WINDOWS_ID_BIAS)
	#define CTL_GTO_NUMCYCLES           (20 + WINDOWS_ID_BIAS)
	#define CTL_GTO_FLAGS               (21 + WINDOWS_ID_BIAS)
	#define CTLCAL_GTO_PERIOD           (22 + WINDOWS_ID_BIAS)
	#define CTL_GTO_LOTPERIOD           (23 + WINDOWS_ID_BIAS)
	#define CTL_GTO_GRPDATE             (24 + WINDOWS_ID_BIAS)
	#define CTL_GTO_AGENT               (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_AGENT            (26 + WINDOWS_ID_BIAS)
	#define CTL_GTO_EXTGOODSTYPE        (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_EXTGOODSTYPE     (28 + WINDOWS_ID_BIAS)
	#define CTL_GTO_SALDOINFO           (29 + WINDOWS_ID_BIAS)
	#define CTL_GTO_SUPPLAG             (30 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_SUPPLAG          (31 + WINDOWS_ID_BIAS)
	#define CTL_GTO_BRAND               (32 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_BRAND            (33 + WINDOWS_ID_BIAS)
	#define CTL_GTO_CITY                (34 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_CITY             (35 + WINDOWS_ID_BIAS)
	#define CTL_GTO_PSNCAT              (36 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_PSNCAT           (37 + WINDOWS_ID_BIAS)
	#define CTL_GTO_SHOWALL             (38 + WINDOWS_ID_BIAS)
	#define CTLCAL_GTO_LOTPERIOD        (39 + WINDOWS_ID_BIAS)
	#define CTL_GTO_CYCLESTAT           (40 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_CYCLESTAT        (41 + WINDOWS_ID_BIAS)
	#define CTL_GTO_PRICEKIND           (42 + WINDOWS_ID_BIAS)
	#define CTL_GTO_OLDALG              (43 + WINDOWS_ID_BIAS)
	#define CTL_GTO_ACS                 (44 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTO_ACS              (45 + WINDOWS_ID_BIAS)
	#define CTL_GTO_DUEPERIOD           (46 + WINDOWS_ID_BIAS)
	#define CTLCAL_GTO_DUEPERIOD        (47 + WINDOWS_ID_BIAS)
#define DLG_PRINT                 134 // Общий диалог параметров печати
	#define CTL_PRINT_PORT               (1 + WINDOWS_ID_BIAS) // Порт вывода
	#define CTL_PRINT_PGHT               (2 + WINDOWS_ID_BIAS) // Длина листа
	#define CTL_PRINT_LMGN               (3 + WINDOWS_ID_BIAS) // Левое поле
	#define CTL_PRINT_PRINTER            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRINT_PRINTER         (5 + WINDOWS_ID_BIAS)
	#define CTL_PRINT_NCOPIES            (6 + WINDOWS_ID_BIAS)
	#define CTL_PRINT_RPTNAME            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRINT_RPTNAME         (8 + WINDOWS_ID_BIAS)
#define DLG_GOODSREST             135 // Фильтр остатков товаров
	#define CTL_GOODSREST_DATE           (1 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_LOC            (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_LOC         (3 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_SUPPL          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_SUPPL       (5 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_GGRP           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_GGRP        (7 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_METHOD         (8 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_FLAGS          (9 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_SPLIT         (10 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_WL            (11 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_DIFFBYLOC     (12 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_BYQUOT        (13 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_QUOTK         (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_QUOTK      (15 + WINDOWS_ID_BIAS)
	#define CTLCAL_GOODSREST_DATE       (16 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_SUBST         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_SUBST      (18 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_BRAND         (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_BRAND      (20 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_DRAFTPRD      (21 + WINDOWS_ID_BIAS)
	#define CTLCAL_GOODSREST_DRAFTPRD   (22 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_BYQUOT2       (23 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_LOTTAG        (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSREST_LOTTAG     (25 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_RTLPRICES     (26 + WINDOWS_ID_BIAS)
	#define CTL_GOODSREST_ORD           (27 + WINDOWS_ID_BIAS) // @v11.4.6
	#define CTLSEL_GOODSREST_ORD        (28 + WINDOWS_ID_BIAS) // @v11.4.6
	#define CTL_GOODSREST_RRANGE        (29 + WINDOWS_ID_BIAS) // @v11.4.6 Диапазон величины остатка
#define DLG_PRNCASH               136 // Выбор формы кассового ордера
	#define CTL_PRNCASH_FORM             (1 + WINDOWS_ID_BIAS)
#define DLG_ATFLT                 137 // Фильтр описи бух. проводок
	#define CTL_ATFLT_PERIOD             (1 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_WL                 (2 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_LASTONLY           (3 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_OPRKIND            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATFLT_OPRKIND         (5 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_CUR                (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATFLT_CUR             (7 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_ALLCUR             (8 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_AMOUNT             (9 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_GRPACO             (10 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_CYCLE              (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_ATFLT_CYCLE           (12 + WINDOWS_ID_BIAS)
	#define CTL_ATFLT_NUMCYCLES          (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_ATFLT_PERIOD          (14 + WINDOWS_ID_BIAS)
#define DLG_DATERNG               138 // Общий диалог выбора диапазона дат
#define DLG_SELSESSRNG            809 // Выбор начала и конца сессии
	#define CTL_DATERNG_PERIOD           (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_DATERNG_PERIOD        (2 + WINDOWS_ID_BIAS)
#define DLG_GDSGRPRLZ             139 // Фильтр реализации по группам товаров
	#define CTL_GDSGRPRLZ_PERIOD         (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_LOC            (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_LOC         (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_FLAGS          (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_GRP            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_GRP         (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_LABEL          (7 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_LOTSPRD        (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_GDSGRPRLZ_PERIOD      (9 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_OP             (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_OP          (11 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_CCL            (12 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_OBJ            (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_OBJ         (14 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_SUPPL          (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_SUPPL       (16 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_SUPPLAG        (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_SUPPLAG     (18 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_GSUBST         (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSGRPRLZ_GSUBST      (20 + WINDOWS_ID_BIAS)
	#define CTL_GDSGRPRLZ_DIFFTAX        (21 + WINDOWS_ID_BIAS)
	#define CTLCAL_GDSGRPRLZ_LOTSPRD     (22 + WINDOWS_ID_BIAS)
#define DLG_GDSRLZ                140 // Аналогичен DLG_GTO Obsolete: use DLG_GDSMOV
#define DLG_GDSMOV                140 //
#define DLG_PRNGDSREST            142 // Опции печати остатков товаров
	#define CTL_PRNGDSREST_PRICE         (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNGDSREST_FLAGS         (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNGDSREST_SORT          (3 + WINDOWS_ID_BIAS)
#define DLG_CASHN                 143
#define DLG_CASHNS                143  // Синхронный кассовый узел
#define DLG_CASHNA                157  // Асинхронный кассовый узел
#define DLG_CASHNG                1138 // Группа кассовых узлов
#define DLG_CASHND                1193 // Распределенный кассовый узел
	#define cmDivGrpAssc                 101 //
	#define cmExtDevices                 102 // Дополнительные устройства
	#define cmImpParam                   104 // Параметры импорта
	#define cmEditCashParam              105 // Параметры кассы
	#define cmApnCorrList                106 // Список корреспондирующих кассовых узлов
	#define CTL_CASHN_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_PORT               (3 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_LOC                (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_LOC             (5 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_CURDATE            (6 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_OVRFLW             (7 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_ACCUM              (8 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_DEVICE             (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_DEVICE         (10 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_FLAGS             (11 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_LOGNUM            (12 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_IMPFILES          (13 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_EXPPATHS          (14 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_DRVVERMAJOR       (15 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_DRVVERMINOR       (16 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_CSESS             (17 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_LOGNUMS           (18 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_EXTQUOT           (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_EXTQUOT        (20 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_DISROUND          (21 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_SLEEPTIMEOUT      (22 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_EXTFLAGS          (23 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_ID                (24 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_NOA               (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_NOA            (26 + WINDOWS_ID_BIAS)
	#define CTLCAL_CASHN_CURDATE        (27 + WINDOWS_ID_BIAS) // @alena
	#define CTLBRW_CASHN_IMPFILES       (28 + WINDOWS_ID_BIAS)
	#define CTLBRW_CASHN_EXPPATHS       (29 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_IMPPARAM          (30 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_SYMB              (31 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_NOA_PRINTONLY     (32 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_TBLDGMPATH        (33 + WINDOWS_ID_BIAS)
	#define CTLBRW_CASHN_TBLDGMPATH     (34 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_GROUP             (35 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_GROUP          (36 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_GRPFLAGS          (37 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_SPECIALITY        (38 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_SPECIALITY     (39 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_ADDEDMSGSIGN      (40 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_GOODSGRP          (41 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHN_GOODSGRP       (42 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_SLIPFMTPATH       (43 + WINDOWS_ID_BIAS)
	#define CTLBRW_CASHN_SLIPFMTPATH    (44 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_PASSIVE           (45 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_ST_CRBILL         (46 + WINDOWS_ID_BIAS)
	#define CTL_CASHN_DRVVER            (47 + WINDOWS_ID_BIAS)
#define DLG_CALCPRICE             144 // Спец. калькулятор (расчет наценки)
	#define CTL_CALCPRICE_COST           (1 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_PERCENT        (2 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_PRICE          (3 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_GOODS          (4 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_EXCISE         (5 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_EXCLTAXES      (6 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_VATRATE        (7 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_VATSUM         (8 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_COSTWOVAT   (9 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_ROUND         (10 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_PREC          (11 + WINDOWS_ID_BIAS)
	#define CTL_CALCPRICE_ROUNDVAT      (12 + WINDOWS_ID_BIAS)
#define DLG_NEWDAY                145 // Начало кассовой сессии
#define DLG_NEWDAY_L             1030
	#define CTL_NEWDAY_DATE              (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_NEWDAY_DATE           (2 + WINDOWS_ID_BIAS) // @alena
#define DLG_CASHDISP_WCFM        1014
	#define CTL_CASHDISP_BEGSUM          (1 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_INCOME          (2 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_CURSUM          (3 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_CASHSUM         (4 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_DIFF            (5 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_FRAME1          (6 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_FRAME2          (7 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_CASHAMT         (8 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_BANKAMT         (9 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_RETAMT         (10 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_DSCNTAMT       (11 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_SCARDAMT       (12 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_ST_TITLE       (13 + WINDOWS_ID_BIAS)
	#define CTL_CASHDISP_ST_INFO        (14 + WINDOWS_ID_BIAS)
#define DLG_CASHCORR              147 // Корректировка сумматора кассы
	#define CTL_CASHCORR_CURVAL          (1 + WINDOWS_ID_BIAS)
	#define CTL_CASHCORR_ADD             (2 + WINDOWS_ID_BIAS)
	#define CTL_CASHCORR_SUM             (3 + WINDOWS_ID_BIAS)
#define DLG_OPKMORE               148 // Опции вида операции
#define DLG_OPKMORE_AT            396 // Опции бухгалтерской операции
#define DLG_OPKMORE_GRC           397 // Опции прихода товара
#define DLG_OPKMORE_GEX           398 // Опции расхода товара
#define DLG_OPKMORE_GRV           399 // Опции переоценки
#define DLG_OPKMORE_RET           400 // Опции товарного возврата
#define DLG_OPKMORE_PAY           401 // Опции оплаты
#define DLG_OPKMORE_ORD           555 // Опции товарного заказа
#define DLG_OPKMORE_CHG           625 // Опции начисления по ренте
#define DLG_OPKMORE_MDF           681 // Опции модификации
#define DLG_OPKMORE_DRC           924 // OPKO_DRC Опции драфт-прихода
#define DLG_OPKMORE_DEX           925 // OPKO_DEX Опции драфт-расхода
	#define CTL_OPKMORE_FLAGS            (1 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_AMOUNT           (2 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_MEMO             (5 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_OBJ2NAME         (6 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_DEFLOC           (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKMORE_DEFLOC        (8 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_SUBTYPE          (9 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_AMTFORMULA       (10 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_EXTFLAGS         (11 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_MCR              (12 + WINDOWS_ID_BIAS)
    #define CTL_OPKMORE_PAYMTYPE         (13 + WINDOWS_ID_BIAS)
#define DLG_OPKPRNOPT             155 // Опции печати документов по операции
	#define CTL_OPKMORE_PRTAMT           (3 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_PRTQC            (4 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_WHAT             (6 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_PRNFORM          (7 + WINDOWS_ID_BIAS)
	#define CTL_OPKMORE_PRTORD           (9 + WINDOWS_ID_BIAS)
#define DLG_CALCDIFF              149 // Спец. калькулятор (расчет сдачи)
#define DLG_CALCDIFF_L            898
	#define CTL_CALCDIFF_AMOUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_CALCDIFF_CASH            (2 + WINDOWS_ID_BIAS)
	#define CTL_CALCDIFF_DIFF            (3 + WINDOWS_ID_BIAS)
#define DLG_CFGROUND              150 // Параметры округлени
	#define CTL_CFGROUND_PREC            (1 + WINDOWS_ID_BIAS)
	#define CTL_CFGROUND_DIR             (2 + WINDOWS_ID_BIAS)
//
// Диалоги DLG_RECLCTRN, DLG_GOODSOPR и DLG_CHKBAMT используют
// управляющие элементы от диалога DLG_BILLFLT
//
#define DLG_RECLCTRN              151
#define DLG_GOODSOPR              152
	#define cmABCAnlzOptions   101
	#define cmCompare          102
#define DLG_CHKBAMT               156 // Проверка сумм документов
#define DLG_SYSJFILT              153 // Фильтр по системному журналу
	#define cmSysJActionList             2001
	#define CTL_SYSJFILT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_SYSJFILT_ACTION          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYSJFILT_ACTION       (3 + WINDOWS_ID_BIAS)
	#define CTL_SYSJFILT_USER            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYSJFILT_USER         (5 + WINDOWS_ID_BIAS)
	#define CTL_SYSJFILT_OBJ             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYSJFILT_OBJ          (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_SYSJFILT_PERIOD       (8 + WINDOWS_ID_BIAS)
	#define CTL_SYSJFILT_FLAGS           (9 + WINDOWS_ID_BIAS)
	#define CTL_SYSJFILT_SUBST           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYSJFILT_SUBST        (11 + WINDOWS_ID_BIAS)
	#define CTL_SYSJFILT_SUBSTDT         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYSJFILT_SUBSTDT      (13 + WINDOWS_ID_BIAS)
#define DLG_BILLINFO              154 // Системная информация о документе
	#define CTL_BILLINFO_ID              (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_ACCESS          (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_USER            (3 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_FLAGS           (4 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_AMTLIST         (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_UUID            (6 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_LINKBILLID      (7 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_RBB             (8 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_FLAGS2          (9 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_OMTPAYM         (10 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_EDIOP           (11 + WINDOWS_ID_BIAS)
	#define CTL_BILLINFO_RECADV          (12 + WINDOWS_ID_BIAS)
//                                155 // Occupied
//                                156 // Occupied
//                                157 // Occupied
#define DLG_RTCOMM                161 // Общие права доступа
	#define cmRtOpList                   2001
	#define cmRtLocList                  2002
	#define cmRtConfigList               2003
	#define cmRtAccList                  2004
	#define cmRtPosNodeList              2005
	#define cmRtQuotKindList             2006
	#define CTL_RTCOMM_PWMIN             (1 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_PWPERIOD          (2 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_DAYS              (3 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_BEGTIME           (4 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_ENDTIME           (5 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_RBILLPRD          (6 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_OBJLIST           (7 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_ACCESS            (8 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_WBILLPRD          (9 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_OPLIST           (10 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_LOCLIST          (11 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_CONFLIST         (12 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_RTDESKTOP        (13 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_ACCLIST          (14 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_ONLYGGRP         (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_RTCOMM_ONLYGGRP      (16 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_ONLYGGRPSTRIC    (17 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_OWNBILLRESTR     (18 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_PRDFORCSESS      (19 + WINDOWS_ID_BIAS)
	#define CTL_RTCOMM_DBXRCV           (20 + WINDOWS_ID_BIAS)
#define DLG_RTOBJ                 162 // Права доступа обобщенного объекта
	#define CTL_RTOBJ_FLAGS              (1 + WINDOWS_ID_BIAS)
	#define CTL_RTOBJ_SFLAGS            (11 + WINDOWS_ID_BIAS)
#define DLG_RTBILL                163 // Права доступа к документам
	#define CTL_RTBILL_FLAGS             (1 + WINDOWS_ID_BIAS)
	#define CTL_RTBILL_SFLAGS           (11 + WINDOWS_ID_BIAS)
	#define CTL_RTBILL_OPRFLAGS         (12 + WINDOWS_ID_BIAS)
#define DLG_RTOPRK                164 // Права доступа к видам операций
	#define CTL_RTOPRK_FLAGS             (1 + WINDOWS_ID_BIAS)
	#define CTL_RTOPRK_SFLAGS           (11 + WINDOWS_ID_BIAS)
#define DLG_RTSCARD               165 // Права доступа к пластиковым картам
	#define CTL_RTSCARD_FLAGS            (1 + WINDOWS_ID_BIAS)
	#define CTL_RTSCARD_SFLAGS          (11 + WINDOWS_ID_BIAS)
#define DLG_RTGOODS               166 // @v4.3.10 Права доступа к товарам
#define DLG_SPLTOFLT              200 // Фильтр оборотки по поставщикам
	#define CTL_SPLTOFLT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_SPLTOFLT_STATUS          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPLTOFLT_STATUS       (3 + WINDOWS_ID_BIAS)
	#define CTL_SPLTOFLT_CITY            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPLTOFLT_CITY         (5 + WINDOWS_ID_BIAS)
	#define CTL_SPLTOFLT_SUPPL           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPLTOFLT_SUPPL        (7 + WINDOWS_ID_BIAS)
	#define CTL_SPLTOFLT_FLAGS           (8 + WINDOWS_ID_BIAS)
	#define CTL_SPLTOFLT_LOC             (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPLTOFLT_LOC          (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_SPLTOFLT_PERIOD       (11 + WINDOWS_ID_BIAS)
#define DLG_CALC                  201 // Калькулятор
	#define cmCalcEq                  101
	#define cmCalcMem                 102
	#define cmCalcPlusMem             103
	#define cmCalcMinusMem            104
	#define CTL_CALC_INPUT               (1 + WINDOWS_ID_BIAS)
	#define CTL_CALC_RESULT              (2 + WINDOWS_ID_BIAS)
	#define CTL_CALC_MEMVIEW             (3 + WINDOWS_ID_BIAS)
	#define CTL_CALC_EQ                 (11 + WINDOWS_ID_BIAS) // Кнопка "="
	#define CTL_CALC_MEM                (12 + WINDOWS_ID_BIAS) // Кнопка "M"
	#define CTL_CALC_PLUSMEM            (13 + WINDOWS_ID_BIAS) // Кнопка "+M"
	#define CTL_CALC_MINUSMEM           (14 + WINDOWS_ID_BIAS) // Кнопка "-M"
#define DLG_SLLTOFLT              202 // Фильтр оборотки по покупателям
	#define cmEditCycles                 101
	#define cmEditExt                    102
	#define CTL_SLLTOFLT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_STATUS          (2 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_CLIENT          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_CLIENT       (7 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_EXPIRYPRD       (8 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_FLAGS           (9 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_LOC            (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_LOC         (11 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_WL             (12 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_ORDER          (13 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_CUR            (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_CUR         (15 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_ALLCUR         (16 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_ACCSHEET       (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_ACCSHEET    (18 + WINDOWS_ID_BIAS)
	#define CTLCAL_SLLTOFLT_PERIOD      (19 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_PAYMPERIOD     (20 + WINDOWS_ID_BIAS)
	#define CTLCAL_SLLTOFLT_PAYMPERIOD  (21 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_SGP            (22 + WINDOWS_ID_BIAS) // @v4.6.12 INP Подстановка контрагента
	#define CTLSEL_SLLTOFLT_SGP         (23 + WINDOWS_ID_BIAS) // @v4.6.12 SEL Подстановка контрагента
	#define CTL_SLLTOFLT_CITY           (24 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTLSEL_SLLTOFLT_CITY        (25 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTL_SLLTOFLT_COUNTRY        (26 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTLSEL_SLLTOFLT_COUNTRY     (27 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTL_SLLTOFLT_REGION         (28 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTLSEL_SLLTOFLT_REGION      (29 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTL_SLLTOFLT_CATEGORY       (30 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTLSEL_SLLTOFLT_CATEGORY    (31 + WINDOWS_ID_BIAS) // @v4.8.8
	#define CTLCAL_SLLTOFLT_EXPIRYPRD   (32 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_CYCLEBUTTON    (33 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_EXTBUTTON      (34 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_SGB            (35 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_SGB         (36 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_SGEXT          (37 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_SGEXT       (38 + WINDOWS_ID_BIAS)
	#define CTL_SLLTOFLT_OP             (39 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLLTOFLT_OP          (40 + WINDOWS_ID_BIAS)
#define DLG_NEWPRICE              203 // Установка новой цены на товар
	#define CTL_NEWPRICE_LOC             (1 + WINDOWS_ID_BIAS)
	#define CTL_NEWPRICE_GOODS           (2 + WINDOWS_ID_BIAS)
	#define CTL_NEWPRICE_PRICE           (3 + WINDOWS_ID_BIAS)
	#define CTL_NEWPRICE_LOT             (4 + WINDOWS_ID_BIAS) // Кнопка "Партия..."
	#define CTL_NEWPRICE_ALL             (5 + WINDOWS_ID_BIAS)
#define DLG_PRNGBILL              204 // Выбор формы печати товарного документа
#define DLG_PRNGBILL2             211
#define DLG_PRNGBILL3             212
#define DLG_PRNGBILLM             213
	#define cmSave                       101
	#define CTL_PRNGBILL_WHAT            (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_PRICE           (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_FDIVCOPIES      (3 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_MAINORG         (4 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_LOC             (5 + WINDOWS_ID_BIAS)

	#define CTL_PRNGBILL_NUMCOPIES       (6 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES2      (7 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES3      (8 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES4      (9 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES5      (10 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES6      (11 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES7      (12 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES8      (13 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES9      (14 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES10     (15 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES11     (16 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES12     (17 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_NUMCOPIES13     (18 + WINDOWS_ID_BIAS)

	#define CTL_PRNGBILL_SPIN            (19 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN2           (20 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN3           (21 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN4           (22 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN5           (23 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN6           (24 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN7           (25 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN8           (26 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN9           (27 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN10          (28 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN11          (29 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN12          (30 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_SPIN13          (31 + WINDOWS_ID_BIAS)

	#define CTL_PRNGBILL_ONLYPRCHNG      (40 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_EXPFMT          (41 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRNGBILL_EXPFMT       (42 + WINDOWS_ID_BIAS)
	#define CTL_PRNGBILL_FLAGS           (43 + WINDOWS_ID_BIAS) // @v11.2.0 вместо (CTL_PRNGBILL_ONLYPRCHNG и CTL_PRNGBILL_FDIVCOPIES)

#define DLG_BILLAMT               205 // Сумма документа
#define DLG_BILLAMTCUR            425 // Сумма документа с валютным курсом
#define DLG_STAFFAMT              981 // STAFFAMT Кадровая сумма (включает период действия)
	#define cmBillAmtSelRate             101
	#define CTL_BILLAMT_AMTTYPE          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLAMT_AMTTYPE       (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLAMT_AMOUNT           (3 + WINDOWS_ID_BIAS)
	#define CTL_BILLAMT_CUR              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLAMT_CUR           (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLAMT_CRATE            (6 + WINDOWS_ID_BIAS)
	#define CTL_BILLAMT_BASEAMT          (7 + WINDOWS_ID_BIAS)
	#define CTL_BILLAMT_PERIOD           (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLAMT_PERIOD        (9 + WINDOWS_ID_BIAS)
#define DLG_PLIST                 206 // Полная форма прайс-листа
#define DLG_PLIST_                207 // Урезанная форма прайс-листа
	#define cmPListLines           101
	#define CTL_PLIST_ARTICLE            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLIST_ARTICLE         (2 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_LOC                (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLIST_LOC             (4 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_DATE               (5 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_MEMO               (6 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_KIND               (7 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_QUOTK              (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLIST_QUOTK           (9 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_FLAGS              (10 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_SUBST              (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLIST_SUBST           (12 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_IGNZEROQUOT        (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_PLIST_DATE            (14 + WINDOWS_ID_BIAS) // @alena
	#define CTL_PLIST_GGRP               (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLIST_GGRP            (16 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_LINEFLAGS          (17 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_ACCSHEET           (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLIST_ACCSHEET        (19 + WINDOWS_ID_BIAS)
	#define CTL_PLIST_PRICERNG           (20 + WINDOWS_ID_BIAS)
#define DLG_PLINE                 208 // Строка прайс-листа
	#define CTL_PLINE_GGRP               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLINE_GGRP            (2 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_GOODS              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLINE_GOODS           (4 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_UPP                (5 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_PRICE              (6 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_MEMO               (7 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_INFO               (8 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_LOT                (9 + WINDOWS_ID_BIAS) // Кнопка "Партия..."
	#define CTL_PLINE_QUOTKIND           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLINE_QUOTKIND        (11 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_AP1                (12 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_AP2                (13 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_AP3                (14 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_PLABELGNAME        (15 + WINDOWS_ID_BIAS)
	#define CTL_PLINE_GOODSCODE          (16 + WINDOWS_ID_BIAS)
#define DLG_PLISTFLT              209 // Фильтр прайс-листов
	// Используются управляющие элементы CTL_PLIST_XXX
#define DLG_PLINEFLT              210 // Фильтр строк прайс-листа
	#define cmUpdatePList              12 // cmYes
	#define CTL_PLINEFLT_GGRP            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLINEFLT_GGRP         (2 + WINDOWS_ID_BIAS)
	#define CTL_PLINEFLT_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTL_PLINEFLT_UPDATE          (4 + WINDOWS_ID_BIAS) // Кнопка "Обновить"
	#define CTL_PLINEFLT_EXCLGGRP        (5 + WINDOWS_ID_BIAS)
	#define CTL_PLINEFLT_NAMESTR         (6 + WINDOWS_ID_BIAS)
//                                211 // Occupied DLG_PRNGBILL2
//                                212 // Occupied DLG_PRNGBILL3
//                                213 // Occupied DLG_PRNGBILLM
#define DLG_COMMCFG               220 // Общая конфигураци
	#define cmCommCfgMainOrg          101
	#define cmCommCfgOp               102
	#define cmCommCfgSys              103
	#define CTL_COMMCFG_SUPPLACC         (1 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_SELLACC          (2 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_PAYPERIOD        (3 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_DISLIST          (4 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_CASHACC          (5 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_INCM             (6 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_ACOP             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_COMMCFG_ACOP          (8 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_FLAGS            (9 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_MAXGBLINES       (10 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_FRRLDAYS         (11 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_IMPRESTACC       (12 + WINDOWS_ID_BIAS)
	#define CTL_COMMCFG_ASSETACC         (13 + WINDOWS_ID_BIAS)
#define DLG_CCFGSYS               221 // CCFGSYS Системные параметры общей конфигурации
	#define CTL_CCFGSYS_MAXGBLINES       (1 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_FRRLDAYS         (2 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_FLAGS            (3 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_GRESTTHR         (4 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_CSUNITPRD        (5 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_CSFWPRD          (6 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_LCRUSAGE         (7 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_FLAGS2           (8 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_ICQTTYEPS        (9 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_LCRV2            (10 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_LCRSINCE         (11 + WINDOWS_ID_BIAS)
	#define CTL_CCFGSYS_OBJHISTF         (12 + WINDOWS_ID_BIAS)
#define DLG_QCERTFLT              222 // Фильтр по сертификатам
	#define CTL_QCERTFLT_EXPIRY          (1 + WINDOWS_ID_BIAS)
	#define CTL_QCERTFLT_INITDATE        (2 + WINDOWS_ID_BIAS)
	#define CTL_QCERTFLT_ORGAN           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QCERTFLT_ORGAN        (4 + WINDOWS_ID_BIAS)
	#define CTL_QCERTFLT_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_QCERTFLT_EXPIRY       (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_QCERTFLT_INITDATE     (7 + WINDOWS_ID_BIAS)
	#define CTL_QCERTFLT_CODESTR         (8 + WINDOWS_ID_BIAS)
	#define CTL_QCERTFLT_INNERNUM        (9 + WINDOWS_ID_BIAS)
#define DLG_CNTGRP                223 // Группировка операций по счетчику
	#define CTL_CNTGRP_LIST              (1 + WINDOWS_ID_BIAS)
#define DLG_PRNERR                224 // Сообщение об ошибке печати
	#define CTL_PRNERR_TEXT              (1 + WINDOWS_ID_BIAS)
#define DLG_BARCODE               225 // BARCODE Элемент списка штрих-кодов
	#define CTL_BARCODE_GOODS            (1 + WINDOWS_ID_BIAS)
	#define CTL_BARCODE_CODE             (2 + WINDOWS_ID_BIAS)
	#define CTL_BARCODE_UPP              (3 + WINDOWS_ID_BIAS)
	#define CTL_BARCODE_LEN              (4 + WINDOWS_ID_BIAS)
	#define CTL_BARCODE_FLAGS            (5 + WINDOWS_ID_BIAS)
#define DLG_DATE                  228 // Простой ввод даты
	#define CTL_DATE_DATE                (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_DATE_DATE             (2 + WINDOWS_ID_BIAS)
#define DLG_SRCHBCODE             229 // Простой ввод штрих-кода для поиска
	#define CTL_SRCHBCODE_CODE           (1 + WINDOWS_ID_BIAS)
	#define CTL_SRCHBCODE_CHKDIG         (2 + WINDOWS_ID_BIAS)
	#define CTL_SRCHBCODE_IMG            (3 + WINDOWS_ID_BIAS)
#define DLG_BCODESTR              230 // Структура внутренних штрих-кодов
	#define CTL_BCODESTR_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_BCODESTR_TEMPL           (2 + WINDOWS_ID_BIAS)
	#define CTL_BCODESTR_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTL_BCODESTR_SPC             (4 + WINDOWS_ID_BIAS)
#define DLG_CBAL                  231 // Корректировка балансов
	#define CTL_CBAL_PERIOD              (1 + WINDOWS_ID_BIAS)
	#define CTL_CBAL_BAL                 (2 + WINDOWS_ID_BIAS)
	#define CTL_CBAL_FLAGS               (3 + WINDOWS_ID_BIAS)
	#define CTL_CBAL_LOG                 (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_CBAL_PERIOD           (5 + WINDOWS_ID_BIAS) // @alena
	#define CTLBRW_CBAL_LOG              (6 + WINDOWS_ID_BIAS)
#define DLG_CORCREST              234 // Корректировка CurRest
	#define CTL_CORCREST_LOG             (1 + WINDOWS_ID_BIAS)
	#define CTL_CORCREST_FLAGS           (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORCREST_LOG          (3 + WINDOWS_ID_BIAS)
#define DLG_VATAXLST              235 // Просмотр расчета НДС по документам
	#define CTL_VATAXLST_RATETXT         (1 + WINDOWS_ID_BIAS)
	#define CTL_VATAXLST_COSTTXT         (2 + WINDOWS_ID_BIAS)
	#define CTL_VATAXLST_PRICETXT        (3 + WINDOWS_ID_BIAS)
	#define CTL_VATAXLST_LIST            (4 + WINDOWS_ID_BIAS)
	#define CTL_VATAXLST_RATE            (5 + WINDOWS_ID_BIAS)
	#define CTL_VATAXLST_COST            (6 + WINDOWS_ID_BIAS)
	#define CTL_VATAXLST_PRICE           (7 + WINDOWS_ID_BIAS)
#define DLG_DBDIV                 236 // Раздел базы данных
	#define cmAddrList                   101
	#define CTL_DBDIV_ID                 (1 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_NAME               (2 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_ADDR               (3 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_INTRRCPT           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBDIV_INTRRCPT        (5 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_FLAGS              (6 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_LIST               (7 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_CNT                (8 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_ACCLIST            (9 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_UUID               (10 + WINDOWS_ID_BIAS)
	#define CTL_DBDIV_MQBEXCHG           (11 + WINDOWS_ID_BIAS)
#define DLG_SELFSYNC              237
	#define CTL_SELFSYNC_DBDIV           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELFSYNC_DBDIV        (2 + WINDOWS_ID_BIAS)
	#define CTL_SELFSYNC_FLAGS           (3 + WINDOWS_ID_BIAS)
#define DLG_BTRAN                 238 // Фильтр для пересылки документов
	#define CTL_BTRAN_PERIOD             (1 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_OP                 (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_BTRAN_OP              (3 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_LABEL              (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_BTRAN_PERIOD          (5 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_LIST               (6 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_TRNSMFLAGS         (7 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_TOOP               (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_BTRAN_TOOP            (9 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_AR2                (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_BTRAN_AR2             (11 + WINDOWS_ID_BIAS)
	#define CTL_BTRAN_AR                 (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_BTRAN_AR              (13 + WINDOWS_ID_BIAS)
#define DLG_REPLGOODS             239 // Объединение товаров
	#define cmLots1                   101
	#define cmLots2                   102
	#define cmExchange                103
	#define CTL_REPLGOODS_GRP1           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLGOODS_GRP1        (2 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_GOODS1         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLGOODS_GOODS1      (4 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_PANE1          (5 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_GRP2           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLGOODS_GRP2        (7 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_GOODS2         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLGOODS_GOODS2      (9 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_PANE2         (10 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_FLTF          (11 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_FLTINFO       (12 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_ST_INFO       (13 + WINDOWS_ID_BIAS)
	#define CTL_REPLGOODS_ALLDEST       (14 + WINDOWS_ID_BIAS)
#define DLG_REPLPRSN              240 // Объединение персоналий
#define DLG_REPLADDR              241 // Объединение адресов доставки
	#define CTL_REPLPRSN_KIND1           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLPRSN_KIND1        (2 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_PRSN1           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLPRSN_PRSN1        (4 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_PANE1           (5 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_KIND2           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLPRSN_KIND2        (7 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_PRSN2           (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLPRSN_PRSN2        (9 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_PANE2          (10 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_ADDR1          (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLPRSN_ADDR1       (12 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_ADDR2          (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLPRSN_ADDR2       (14 + WINDOWS_ID_BIAS)
	#define CTL_REPLPRSN_FLAGS          (15 + WINDOWS_ID_BIAS)
#define DLG_PRNBLST               242 // Форма печати реестра документов
	#define CTL_PRNBLST_AMTTYPE          (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNBLST_ORDER            (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNBLST_FLAGS            (3 + WINDOWS_ID_BIAS)
#define DLG_SR                    243 // Поиск и замена текстовых полей
#define DLG_SRGOODS               244 // Поиск и замена наименований товаров
	#define CTL_SR_SEARCH                (1 + WINDOWS_ID_BIAS)
	#define CTL_SR_REPLACE               (2 + WINDOWS_ID_BIAS)
	#define CTL_SR_REGEXP                (3 + WINDOWS_ID_BIAS)
	#define CTL_SR_CAPS                  (4 + WINDOWS_ID_BIAS)
	#define CTL_SR_WHAT                  (5 + WINDOWS_ID_BIAS)
	#define CTL_SRGOODS_BRAND            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SRGOODS_BRAND         (7 + WINDOWS_ID_BIAS)
	#define CTL_SRGOODS_GRP              (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_SRGOODS_GRP           (9 + WINDOWS_ID_BIAS)
	#define CTL_SRGOODS_FLAGS            (10 + WINDOWS_ID_BIAS)
	#define CTL_SRGOODS_DOREPLACE        (11 + WINDOWS_ID_BIAS)
	#define CTL_SRGOODS_SPCFLAGS         (12 + WINDOWS_ID_BIAS)
	#define CTL_SRGOODS_MANUF            (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_SRGOODS_MANUF         (14 + WINDOWS_ID_BIAS)
#define DLG_FOUND                 245 // Замена найденного текста
	#define CTL_FOUND_ORGTEXT            (1 + WINDOWS_ID_BIAS)
	#define CTL_FOUND_NEWTEXT            (2 + WINDOWS_ID_BIAS)
#define DLG_GOODSFLT              250 // Фильтр по товарам
	#define cmGrpList                 101
	#define cmManufFilt               102
	#define cmAdvOptions              103
	#define cmGoodsFiltVat            105
	#define cmBrandList               107
	#define cmBrandOwnerList          108
	#define CTL_GOODSFLT_GRP             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_GRP          (2 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_GRPLIST         (3 + WINDOWS_ID_BIAS) // Button cmGrpList
	#define CTL_GOODSFLT_MANUF           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_MANUF        (5 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_MNFFLT          (6 + WINDOWS_ID_BIAS) // Button cmManufFilt
	#define CTL_GOODSFLT_UNIT            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_UNIT         (8 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_UNITF           (9 + WINDOWS_ID_BIAS) // Флаги фильтрации по единице
	#define CTL_GOODSFLT_NOT            (11 + WINDOWS_ID_BIAS) // Отрицание фильтра
	#define CTL_GOODSFLT_KIND           (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_KIND        (13 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_NOKIND         (14 + WINDOWS_ID_BIAS) // Без типа товара
	#define CTL_GOODSFLT_TAXGRP         (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_TAXGRP      (16 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_PHUNIT         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_PHUNIT      (18 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_NAMESTR        (19 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_COUNTRY        (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_COUNTRY     (21 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_BCLEN          (22 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_VATFLT         (23 + WINDOWS_ID_BIAS) // Button cmGoodsFiltVat
	#define CTL_GOODSFLT_BRAND          (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_BRAND       (25 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_SHOWPASSIVE    (26 + WINDOWS_ID_BIAS)
	// @v8.3.6 Заменено на STDCTL_VIEWOPTBUTTON #define CTL_GOODSFLT_VIEWOPT        (27 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_WOTAXGRP       (28 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_FLAGS          (29 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_BROWNER        (30 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_BROWNER     (31 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_ORDER          (32 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSFLT_ORDER       (33 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_BARCODESTR     (34 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLT_NOBRAND        (35 + WINDOWS_ID_BIAS)

#define DLG_PSNFLT                251 // Фильтр по персоналиям
#define DLG_PSNEXFLT              252 // Фильтр по кадрам           @obsolete
	#define CTL_PSNFLT_KIND              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_KIND           (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_STATUS            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_STATUS         (4 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_COUNTRY           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_COUNTRY        (6 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_CITY              (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_CITY           (8 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_FLAGS             (9 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_DIV              (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_DIV           (11 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_ORG              (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_ORG           (13 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_ATTR             (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_ATTR          (15 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_EMPTY            (16 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_NAMESTR          (17 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_VATFREE          (18 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_CATEGORY         (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNFLT_CATEGORY      (20 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_NEWCLIPERIOD     (21 + WINDOWS_ID_BIAS)
	#define CTLCAL_PSNFLT_NEWCLIPERIOD  (22 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_NEWCLIONLY       (23 + WINDOWS_ID_BIAS)
	#define CTL_PSNFLT_CASDT            (24 + WINDOWS_ID_BIAS) // @v12.2.2
	#define CTLCAL_PSNFLT_CASDT         (25 + WINDOWS_ID_BIAS) // @v12.2.2
#define DLG_LST2LST               261 // Формирование одного списка из другого
	#define cmForward                 101
	#define cmAllForward              102
	#define cmBackward                103
	#define cmAllBackward             104
	#define cmSelectByTag             105 // @v11.0.3
	#define CTL_LST2LST_LST1             (1 + WINDOWS_ID_BIAS)
	#define CTL_LST2LST_LST2             (2 + WINDOWS_ID_BIAS)
	#define CTL_LST2LST_FW               (3 + WINDOWS_ID_BIAS) // Button
	#define CTL_LST2LST_ALLFW            (4 + WINDOWS_ID_BIAS) // Button
	#define CTL_LST2LST_BW               (5 + WINDOWS_ID_BIAS) // Button
	#define CTL_LST2LST_ALLBW            (6 + WINDOWS_ID_BIAS) // Button
	#define CTL_LST2LST_CT2              (7 + WINDOWS_ID_BIAS) // Количество элементов в правом списке
#define DLG_GDSLST                262 // Список товаров для добавления в группу
	#define CTL_GDSLST_GGRP              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSLST_GGRP           (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSLST_LIST              (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSLST_AFBUTTON          (4 + WINDOWS_ID_BIAS)
#define DLG_GDSCFG                263 // Опции справочника товаров
	#define CTL_GDSCFG_CODLEN            (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_WPRFX             (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_FLAGS             (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_DEFUNIT           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_DEFUNIT        (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_DEFPCKGTYP        (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_DEFPCKGTYP     (7 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_MINUNIQBCLEN      (8 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_ACGIT             (9 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_ASSETGRP          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_ASSETGRP       (11 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_GDSMATRIX         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_GDSMATRIX      (13 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_XCHG_FLAGS        (14 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_OWNACTMPL         (15 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_OWNACCNTR         (16 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_GMTXRESTR         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_GMTXRESTR      (18 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_TAREGRP           (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_TAREGRP        (20 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_CPRFX             (21 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_DEFGRP            (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_DEFGRP         (23 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_BCPGUATAG         (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_BCPGUATAG      (25 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_IGNFLDMTX         (26 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_DEFGOODS          (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCFG_DEFGOODS       (28 + WINDOWS_ID_BIAS)
	#define CTL_GDSCFG_FLAGS2            (29 + WINDOWS_ID_BIAS)
#define DLG_PRNSELLTO             264 // Опции печати ведомости должников
	#define CTL_PRNSELLTO_FLAGS          (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNSELLTO_ORDER          (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNSELLTO_DTIN           (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRNSELLTO_DTIN        (4 + WINDOWS_ID_BIAS)
#define DLG_PRNPLST               265 // Опции печати прайс-листа
	#define CTL_PRNPLST_WHAT             (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNPLST_PCTADD           (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNPLST_SORTBY           (3 + WINDOWS_ID_BIAS)
#define DLG_VATSELL               266 // Запись книги продаж
#define DLG_VATBUY                267 // Запись книги покупок
	#define cmVATBookLink             101
	#define cmVATBookSum              102
	#define CTL_VATBOOK_DT               (1 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_CODE             (2 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_INVCDT           (3 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_PAYMDT           (4 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_RCPTDT           (5 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_OBJ              (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBOOK_OBJ           (7 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_AMT              (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_VATBOOK_DT            (9 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_VATBOOK_INVCDT        (10 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_VATBOOK_PAYMDT        (11 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_VATBOOK_RCPTDT        (12 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_VATBOOK_DT2           (13 + WINDOWS_ID_BIAS) // @alena
	#define CTL_VATBOOK_LOC              (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBOOK_LOC           (15 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_CBILLDT          (16 + WINDOWS_ID_BIAS)
	#define CTLCAL_VATBOOK_CBILLDT       (17 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_CBILLCODE        (18 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_TAXOPCODE        (19 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_EXP              (20 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_FRAME            (21 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_FLAGS            (22 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_VAT0             (31 + WINDOWS_ID_BIAS) // @anchor ident
	#define CTL_VATBOOK_VAT1             (32 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_VAT2             (33 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_VAT3             (34 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_VAT4             (35 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_VAT5             (36 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_SVAT1            (41 + WINDOWS_ID_BIAS) // @anchor ident
	#define CTL_VATBOOK_SVAT2            (42 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_SVAT3            (43 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_SVAT4            (44 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_SVAT5            (45 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOK_TVAT1            (51 + WINDOWS_ID_BIAS) // @anchor ident StaticText
	#define CTL_VATBOOK_TVAT2            (52 + WINDOWS_ID_BIAS) // StaticText
	#define CTL_VATBOOK_TVAT3            (53 + WINDOWS_ID_BIAS) // StaticText
	#define CTL_VATBOOK_TVAT4            (54 + WINDOWS_ID_BIAS) // StaticText
	#define CTL_VATBOOK_TVAT5            (55 + WINDOWS_ID_BIAS) // StaticText
#define DLG_VATBFLT               268 // Фильтр по книге продаж/покупок
#define DLG_VATBWHAT              269 // Выбор книги продаж либо покупок
	#define CTL_VATBFLT_WHAT             (1 + WINDOWS_ID_BIAS)
	#define CTL_VATBWHAT_WHAT            (1 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_PERIOD           (2 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_OBJ              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBFLT_OBJ           (4 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_LINKT            (5 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_PAYMPRD          (6 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_FLAGS            (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_VATBFLT_PERIOD        (8 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_EOBJSHEET        (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBFLT_EOBJSHEET     (10 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_EXTOBJ           (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBFLT_EXTOBJ        (12 + WINDOWS_ID_BIAS)
	#define CTL_VATBFLT_LOC              (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBFLT_LOC           (14 + WINDOWS_ID_BIAS)
#define DLG_VATBCFG_BUY           270 // Конфигурация книги покупок
#define DLG_VATBCFG_SELL          271 // Конфигурация книги продаж
	#define CTL_VATBCFG_SHEET            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBCFG_SHEET         (2 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_LIST             (3 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_INCM             (4 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_FLAGS            (5 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_BUY_FLAGS        (6 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_SELL_FLAGS       (7 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_INCMPRD          (8 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_PERIOD           (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_VATBCFG_PERIOD        (10 + WINDOWS_ID_BIAS)
	#define CTL_VATBCFG_SELDATE          (11 + WINDOWS_ID_BIAS)
#define DLG_VATBABFLT             272 // Фильтр для построения книги продаж/покупок
	#define CTL_VATBABFLT_WL             (1 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_PERIOD         (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_VATBABFLT_PERIOD      (3 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_SHIPMPRD       (4 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_OBJECT         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBABFLT_OBJECT      (6 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_LOC            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBABFLT_LOC         (8 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_SHEET2         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBABFLT_SHEET2      (10 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_OBJECT2        (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBABFLT_OBJECT2     (12 + WINDOWS_ID_BIAS)
	#define CTL_VATBABFLT_FLAGS          (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_VATBABFLT_SHIPMPRD    (14 + WINDOWS_ID_BIAS) // @alena
#define DLG_BILLF                 273 // Изменение флагов по выборки документов
	#define CTL_BILLF_SET                (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLF_RESET              (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLF_STATUS             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLF_STATUS          (4 + WINDOWS_ID_BIAS)
#define DLG_LOTQCERT              274 // Сертификат по лоту
	#define CTL_LOTQCERT_QCERT           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOTQCERT_QCERT        (2 + WINDOWS_ID_BIAS)
	#define CTL_LOTQCERT_EXPIRY          (3 + WINDOWS_ID_BIAS)
	#define CTL_LOTQCERT_FRAME           (4 + WINDOWS_ID_BIAS)
	#define CTL_LOTQCERT_CLB             (5 + WINDOWS_ID_BIAS)
	#define CTL_LOTQCERT_SERIAL          (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_LOTQCERT_EXPIRY       (7 + WINDOWS_ID_BIAS)
//                                275 // Occupied
#define DLG_GDSTYP                276 // Тип товара
	#define CTL_GDSTYP_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_UNLIM             (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_COST              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_COST           (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_PRICE             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_PRICE          (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_DISCOUNT          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_DISCOUNT       (8 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_FLAGS             (9 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_ID                (10 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_CVAT              (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_CVAT           (12 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_AWOG              (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_AWOG           (14 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_PRICERESTR        (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_PRICERESTR     (16 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_SYMB              (17 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_STKTLR            (18 + WINDOWS_ID_BIAS)
	#define CTL_GDSTYP_CHZNPT            (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTYP_CHZNPT         (20 + WINDOWS_ID_BIAS)
#define DLG_QUOT                  277 // Котировка
#define DLG_QUOTNEW               291 // Новая котировка
	#define CTL_QUOT_GGRP                (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOT_GGRP             (2 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_GOODS               (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOT_GOODS            (4 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_LOC                 (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOT_LOC              (6 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_DATE                (7 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_PREVPRICE           (8 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_PRICE               (9 + WINDOWS_ID_BIAS)

	#define CTL_QUOT_KIND                (10 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_VALUE               (11 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_FLAGS               (12 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_DATETIME            (13 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_AR                  (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOT_AR               (15 + WINDOWS_ID_BIAS)
	#define CTL_QUOT_PERIOD              (16 + WINDOWS_ID_BIAS)
#define DLG_ACSPANEL              278 // Панель асинхронной кассовой сессии
#define DLG_SCSPANEL              279 // Панель синхронной кассовой сессии
#define DLG_CASHREPORTS           945 // Отчеты по кассе
#define DLG_CASHREPORTS_L        1008
#define cmSCSZReportCopy             101
#define cmSCSDrawerOpen              102 // @v11.6.9 Команда [Открыть денежный ящик]
	#define CTL_CSPANEL_NODE             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSPANEL_NODE          (2 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_GETCHECKS        (3 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_INPUT            (4 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_CSESSOPEN        (5 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_CSESSCLOSE       (6 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_XREP             (7 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_BMP              (8 + WINDOWS_ID_BIAS)
	#define CTL_CSPANEL_RESETCHZNPMSVRADDR (9 + WINDOWS_ID_BIAS) // @v12.4.1
#define DLG_LOTTOTAL              280 // Итоги выборки лотов
#define DLG_LOTTOTALE             281 // Раширенные итоги выборки лотов
	#define CTL_LOTTOTAL_FRAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_INFRAME         (2 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_DFRAME          (3 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_COUNT           (4 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_DCOUNT          (5 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_QTTY            (6 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_INCOST          (7 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_INPRICE         (8 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_REST            (9 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_COST            (10 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_PRICE           (11 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_DREST           (12 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_DCOST           (13 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_DPRICE          (14 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_OPRESTBEG       (15 + WINDOWS_ID_BIAS)
	#define CTL_LOTTOTAL_OPRESTEND       (16 + WINDOWS_ID_BIAS)
#define DLG_PRNGOODSMOV           282 // Форма отчета о движении товара
	#define CTL_PRNGOODSMOV_WHAT         (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNGOODSMOV_AVPRICE      (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNGOODSMOV_SHRTMOVF     (3 + WINDOWS_ID_BIAS)
#define DLG_EXTGSEL               283 // EXTGSEL Расширенный выбор товара
	#define CTL_EXTGSEL_QTTY             (1 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_PACKF            (2 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_GGRP             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTGSEL_GGRP          (4 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_LIST             (5 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_MTX_IND          (6 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_BRAND            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTGSEL_BRAND         (8 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_INFO             (9 + WINDOWS_ID_BIAS)
	#define CTL_EXTGSEL_MODIFMODE        (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTGSEL_MODIFMODE     (11 + WINDOWS_ID_BIAS)
#define DLG_SELMODIF              284 // Выбор типа строки модифкации товара
	#define CTL_SELMODIF_WHAT            (1 + WINDOWS_ID_BIAS)
	#define CTL_SELMODIF_FLAGS           (2 + WINDOWS_ID_BIAS)
	#define CTL_SELMODIF_ST_INFO         (3 + WINDOWS_ID_BIAS)
#define DLG_GSTRUC                285 // Структура товара
#define DLG_GSTRUCFOLDER          666 // Расширенная структура товара (структура верхнего уровня)
	#define cmGStrucExpandReduce      131 // Кнопка расширения/редукции структуры
	#define cmSelNamedGS              601 // Кнопка выбора именованной структуры
	#define cmEditStruct              132 // Кнопка редактирования структуры элемента
	#define cmGiftParam               133 // @v12.3.7 Кнопка редактирования параметров подарочной структуры 
	#define CTL_GSTRUC_GOODS             (1 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_LIST              (2 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_FLAGS             (3 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_GNAME             (4 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_NAME              (5 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_COMMDENOM         (6 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_PERIOD            (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_GSTRUC_PERIOD         (8 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_VAROBJ            (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSTRUC_VAROBJ         (10 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_INFO              (11 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_GIFTAMTRESTR      (12 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_GIFTQK            (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSTRUC_GIFTQK         (14 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_KIND              (15 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_GIFTLIMIT         (16 + WINDOWS_ID_BIAS)
	#define CTL_GSTRUC_ID                (17 + WINDOWS_ID_BIAS)
#define DLG_GSITEM                286 // Элемент структуры товара
	#define cmGSItemLots              601
	#define CTL_GSITEM_HEADER            (1 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_GGRP              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSITEM_GGRP           (3 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_GOODS             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSITEM_GOODS          (5 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_VALUE             (6 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_PCT               (7 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_PRICE             (8 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_ROUNDDOWN         (9 + WINDOWS_ID_BIAS)  // @Obsolet since @v4.7.2
	#define CTL_GSITEM_FLAGS             (9 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_UNITS             (10 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_NETTO             (11 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_SYMB              (12 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_FORMULA           (13 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_GROUPONLY         (14 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_NTBRTCOEF         (15 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_SELOBJTYPE        (16 + WINDOWS_ID_BIAS) // @v12.0.6 radio-button выбора типа объекта для элемента планирования цены
	#define CTL_GSITEM_BIZSCORE          (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSITEM_BIZSCORE       (18 + WINDOWS_ID_BIAS)
	#define CTL_GSITEM_V_EVALED          (19 + WINDOWS_ID_BIAS) // @v12.3.7 Вычисленное значение (для планирование цены)
	#define CTL_GSITEM_V_MANUAL          (20 + WINDOWS_ID_BIAS) // @v12.3.7 Ручное значение (для планирование цены)
#define DLG_MSGNCMPL              287
#define DLG_MSGZCMPL              288
#define DLG_MSGNCMPL2             383
	#define cmMsgNCmplPrint           601
	#define cmPuglSupplAssoc          101
	#define CTL_MSGNCMPL_LINE1           (1 + WINDOWS_ID_BIAS)
	#define CTL_MSGNCMPL_ACTION          (2 + WINDOWS_ID_BIAS)
	#define CTL_MSGNCMPL_LIST            (3 + WINDOWS_ID_BIAS)
	#define CTL_MSGNCMPL_COSTALG         (4 + WINDOWS_ID_BIAS)
	#define CTL_MSGNCMPL_CPCTVAL         (5 + WINDOWS_ID_BIAS)
#define DLG_FORMULA               289 // Формула
	#define CTL_FORMULA_SYMB             (1 + WINDOWS_ID_BIAS)
	#define CTL_FORMULA_EXPR             (2 + WINDOWS_ID_BIAS)
#define DLG_LOGIND                290 // = DLG_LOGIN with CTL_LOGIN_DB
//                                291 // Occupied
#define DLG_PRNCFG                292 // Конфигурация принтера
	#define CTL_PRNCFG_PRINTER           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRNCFG_PRINTER        (2 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_PGLENPT           (3 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_PGLENLS           (4 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_MRGTOP            (5 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_MRGLEFT           (6 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_FONT              (7 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_PORT              (8 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_FLAGS             (9 + WINDOWS_ID_BIAS)
	#define CTL_PRNCFG_WINPRINTER        (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRNCFG_WINPRINTER     (11 + WINDOWS_ID_BIAS)
#define DLG_CLIAGT                293 // CLIAGT Соглашение с клиентом
#define cmDebtLimList                102
#define cmMultiPrintParam            103
	#define CTL_CLIAGT_CLIENT            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_CLIENT         (2 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_DATE              (3 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_EXPIRY            (4 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_MAXCREDIT         (5 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_CURDEBT           (6 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_DSCNT             (7 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_MAXDSCNT          (8 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_PAYPERIOD         (9 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_AGENT             (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_AGENT          (11 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_QUOTKIND          (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_QUOTKIND       (13 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_CODE              (14 + WINDOWS_ID_BIAS)
	#define CTLCAL_CLIAGT_DATE           (15 + WINDOWS_ID_BIAS)
	#define CTLCAL_CLIAGT_EXPIRY         (16 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_EXTOBJECT         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_EXTOBJECT      (18 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_LOCKPRCBEFORE     (19 + WINDOWS_ID_BIAS)
	#define CTLCAL_CLIAGT_LOCKPRCBEFORE  (20 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_FLAGS             (21 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_RETLIMPRD         (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_RETLIMPRD      (23 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_RETLIM            (24 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_ADDEDLIMITVAL     (25 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_PAYMDTBASE        (26 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_PAYMDTBASE     (27 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_EDIPRV            (28 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLIAGT_EDIPRV         (29 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGT_DUEPERIOD         (30 + WINDOWS_ID_BIAS) // @v11.4.8
#define DLG_RTART                 294 // Права доступа к аналитическим статьям
	#define CTL_RTART_FLAGS              (1 + WINDOWS_ID_BIAS)
	#define CTL_RTART_SFLAGS             (2 + WINDOWS_ID_BIAS)
#define DLG_PROTECT               296 // Установка/снятие защиты БД
#define DLG_EXAMTLIST             297 // Список доп сумм документа
	#define CTL_EXAMTLIST_LIST           (1 + WINDOWS_ID_BIAS)
#define DLG_CSALESEX              298 // Фильтр излишков по кассовым сессиям
/* @obsolete v2.8.0
#define DLG_BILLTOTAL             299 // Итог по выборке документов
	#define CTL_BILLTOTAL_COUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLTOTAL_AMOUNT         (2 + WINDOWS_ID_BIAS)
*/
#define DLG_SELCASHNS             300 // Выбор синхронности кассового узла
	#define CTL_SELCASHNS_SA             (1 + WINDOWS_ID_BIAS)
	#define CTL_SELCASHNS_DEVICE         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELCASHNS_DEVICE      (3 + WINDOWS_ID_BIAS)
#define DLG_LOTINFO               301
#define cmLotBill                    101
#define cmLotOps                     102
#define cmPrevLot                    103
	#define CTL_LOTINFO_ID               (1 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_BILLNO           (2 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_QTTY             (3 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_REST             (4 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_UPP              (5 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_COST             (6 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_PRICE            (7 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_CLOSEDT          (8 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_CLOSED           (9 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_LOC              (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOTINFO_LOC           (11 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_GOODS            (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOTINFO_GOODS         (13 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_QCERT            (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOTINFO_QCERT         (15 + WINDOWS_ID_BIAS) // @obsolete since @v4.6.5
	#define CTL_LOTINFO_SUPPL            (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOTINFO_SUPPL         (17 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_INTAXGRP         (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOTINFO_INTAXGRP      (19 + WINDOWS_ID_BIAS)
	#define CTL_LOTINFO_PHREST           (20 + WINDOWS_ID_BIAS)
#define DLG_RECOVER               302
	#define CTL_RECOVER_DB               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_RECOVER_DB            (2 + WINDOWS_ID_BIAS)
	#define CTL_RECOVER_TBL              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_RECOVER_TBL           (4 + WINDOWS_ID_BIAS)
	#define CTL_RECOVER_DEST             (5 + WINDOWS_ID_BIAS)
	#define CTL_RECOVER_LOG              (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_RECOVER_DEST          (7 + WINDOWS_ID_BIAS)
	#define CTLBRW_RECOVER_LOG           (8 + WINDOWS_ID_BIAS)
#define DLG_CHKPAN                303
#define DLG_CHKPANV               304
#define DLG_CHKPAN_TS             896
#define DLG_CHKPANV_L            1026
	#define cmQuantity                101
	#define cmRetCheck                102
	#define cmByPrice                 104
	#define cmCash                    105
	#define cmBanking                 106
	#define cmChkPanSuspend           107
	#define cmToLocPrinters           108
	#define cmChkPanPrint             109
	#define cmSelTable                110
	#define cmByName                  111
	#define cmCashOper                112
	#define cmGoodsDiv                113
	#define cmChkPanF1                113
	#define cmChkPanF2                104
	#define cmMemo                    114
	#define cmSelModifier             115
	#define cmSelGoods                116
	//#define cmSelBill                 117 // @v11.8.7 Кнопка выбора документа
	#define CTL_CHKPAN_LIST              (1 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_TOTAL             (2 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GOODS             (3 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_PRICE             (4 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_QTTY              (5 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SUM               (6 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_INPUT             (7 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_INFO              (8 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_FRAME             (9 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_DISCOUNT          (10 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_STARTHINT         (11 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINTBIAS         10
	#define CTL_CHKPAN_NUMHINTS           7
	#define CTL_CHKPAN_HINT1             (11 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT1           (21 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_HINT2             (12 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT2           (22 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_HINT3             (13 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT3           (23 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_HINT4             (14 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT4           (24 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_HINT5             (15 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT5           (25 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_HINT6             (16 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT6           (26 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_HINT7             (17 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_KBHINT7           (27 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPBOX1           (28 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPBOX2           (29 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPBOX3           (30 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPBOX4           (31 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GDSLIST           (32 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPLIST           (33 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_STARTBUTTON       (34 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_NUMBUTTONS         16
	#define CTL_CHKPAN_SETQTTY		     (34 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_RETCHECK          (35 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SCARD             (36 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_BYPRICE           (37 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_ENTER             (38 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CANCEL            (39 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CASH              (40 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_BANKING           (41 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SELGDSGRP         (42 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPBYDEF          (43 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SUSPCHECK         (44 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_TOLOCPRN          (45 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_TODEFPRN          (46 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SELTABLE          (47 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CASHOPER          (48 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_DIVISION          (49 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_GRPNAME           (60 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CAFE_STATUS       (61 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_ST_TOSLEEP        (62 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CHKID             (63 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CHKDTTM           (64 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CHKNUM            (65 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_CASHNUM           (66 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SCARDCODE         (67 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_FLAGS             (68 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_ARROW_UP          (69 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_ARROW_DOWN        (70 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_LEVELUP           (71 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_INITDTM           (72 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_SELMODIFIER       (73 + WINDOWS_ID_BIAS) // Кнопка выбора модификатора товара
	#define CTL_CHKPAN_GRPBOX5           (74 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_BYNAME            (75 + WINDOWS_ID_BIAS) // Кнопка выбора товара по наименованию
	#define CTL_CHKPAN_SELGOODS          (76 + WINDOWS_ID_BIAS) // Кнопка выбора товара из списка
	#define CTL_CHKPAN_BIGHINT           (77 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_BIGHINT_KB        (78 + WINDOWS_ID_BIAS)
	#define CTL_CHKPAN_INITUSER          (79 + WINDOWS_ID_BIAS)
	//#define CTL_CHKPAN_SELBILL           (80 + WINDOWS_ID_BIAS) // @v11.8.7 Кнопка выбора документа для загрузки в кассовую панель
#define DLG_CCHECKFLT             305 // Фильтр по чекам
	#define cmCrosstab                   101
	#define CTL_CCHECKFLT_PERIOD         (1 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_CASHN          (2 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_NODE           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_NODE        (4 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_FLAGS          (5 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_GGRP           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_GGRP        (7 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_GOODS          (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_GOODS       (9 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_GRP            (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_CCHECKFLT_PERIOD      (11 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_AMTQUANT       (12 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_SCARD          (13 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_SCSER          (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_SCSER       (16 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_CASHORBANK     (17 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_AGENT          (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_AGENT       (19 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_TABLECODE      (20 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_CODERANGE      (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_GRP         (22 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_ORDER          (23 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_SUBST          (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_SUBST       (25 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_STARTSRVCP     (26 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_ALTREG         (27 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_CSHR           (28 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_CSHR        (29 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_CRUSER         (30 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKFLT_CRUSER      (31 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKFLT_FLAGS2         (32 + WINDOWS_ID_BIAS)
#define DLG_CORLOTS               306 // C_LOTS Корректировка лотов
	#define CTL_CORLOTS_LOG              (1 + WINDOWS_ID_BIAS)
	#define CTL_CORLOTS_FLAGS            (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORLOTS_LOG           (3 + WINDOWS_ID_BIAS)
	#define CTL_CORLOTS_MCOP             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CORLOTS_MCOP          (5 + WINDOWS_ID_BIAS)
	#define CTL_CORLOTS_MCOP_HINT        (6 + WINDOWS_ID_BIAS)
#define DLG_SCALE                 307 // Параметры электронных весов
	#define cmScaleSys                   2001
	#define CTL_SCALE_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_PORT               (2 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_LOGNUM             (3 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_LOC                (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALE_LOC             (5 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_GRP                (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALE_GRP             (7 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_FLAGS              (8 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_ID                 (9 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_TYPE               (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALE_TYPE            (11 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_PRTCLVER           (12 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_QUOT               (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALE_QUOT            (14 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_BCPREFIX           (15 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_PARENT             (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALE_PARENT          (17 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_ADDEDMSGSIGN       (18 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_ADDEDLINELEN       (19 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_SYSBTN             (20 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_ADDEDLNCT          (21 + WINDOWS_ID_BIAS)
	#define CTL_SCALE_FONTSIZE           (22 + WINDOWS_ID_BIAS)
#define DLG_SCALEPREP             308 // Подготовка данных для весов
#define DLG_SCALETRAN             309 // Загрузка данных в весы
	#define CTL_SCALEPREP_SCALE          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALEPREP_SCALE       (2 + WINDOWS_ID_BIAS)
	#define CTL_SCALEPREP_FLAGS          (3 + WINDOWS_ID_BIAS)
	#define CTL_SCALEPREP_READY          (4 + WINDOWS_ID_BIAS)
	#define CTL_SCALEPREP_READYCOLOR     (5 + WINDOWS_ID_BIAS)
#define DLG_PREGTYP               310 // Тип регистрационного документа
	#define CTL_PREGTYP_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_ID               (2 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_REGPKIND         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PREGTYP_REGPKIND      (4 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_ST               (5 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_FLAGS            (6 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_FORMAT           (7 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_SYMB             (8 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_PKIND            (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_PREGTYP_PKIND         (10 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_CNTRTEMPL        (11 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_CNTR             (12 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_EXPIRYPERIOD     (13 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_GGRP             (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PREGTYP_GGRP          (15 + WINDOWS_ID_BIAS)
	#define CTL_PREGTYP_GGRPALLOW        (16 + WINDOWS_ID_BIAS)
#define DLG_PSNST                 311 // Юридические статусы персоналий
	#define CTL_PSNST_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_PSNST_ID                 (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNST_TYPE               (3 + WINDOWS_ID_BIAS)
	#define CTL_PSNST_SYMB               (4 + WINDOWS_ID_BIAS)
#define DLG_ELNKK                 312 // Виды адресов электронной связи
	#define CTL_ELNKK_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_ELNKK_ID                 (2 + WINDOWS_ID_BIAS)
	#define CTL_ELNKK_TYPE               (3 + WINDOWS_ID_BIAS)
	#define CTL_ELNKK_FLAGS              (4 + WINDOWS_ID_BIAS)
	#define CTL_ELNKK_SYMB               (5 + WINDOWS_ID_BIAS)
#define DLG_ELNKSET               313 // Електронные адреса персоналии
	#define cmELnkSetAction1             101
	#define cmELnkSetAction2             102
	#define cmELnkSetAction3             103
	#define cmELnkSetAction4             104
	#define cmELnkSetAction5             105
	#define CTL_ELNKSET_KIND1            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_ELNKSET_KIND1         (2 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ADDR1            (3 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_KIND2            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_ELNKSET_KIND2         (5 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ADDR2            (6 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_KIND3            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_ELNKSET_KIND3         (8 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ADDR3            (9 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_KIND4           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_ELNKSET_KIND4        (11 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ADDR4           (12 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_KIND5           (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_ELNKSET_KIND5        (14 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ADDR5           (15 + WINDOWS_ID_BIAS)

	#define CTL_ELNKSET_ACN1            (61 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ACN2            (62 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ACN3            (63 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ACN4            (64 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_ACN5            (65 + WINDOWS_ID_BIAS)
	#define CTL_ELNKSET_INFO            (81 + WINDOWS_ID_BIAS)
#define DLG_ELNKLST               314 // Список эл адресов персоналии
	#define CTL_ELNKLST_LIST             (1 + WINDOWS_ID_BIAS)
#define DLG_REGLST                315 // Список регистров
	#define CTL_REGLST_LIST              (1 + WINDOWS_ID_BIAS)
	#define CTL_REGLST_NAME              (2 + WINDOWS_ID_BIAS)
	#define CTL_REGLST_LINKOBJTEXT       (3 + WINDOWS_ID_BIAS)
#define DLG_ELINK                 316 // Электронный адрес
	#define CTL_ELINK_KIND               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_ELINK_KIND            (2 + WINDOWS_ID_BIAS)
	#define CTL_ELINK_ADDR               (3 + WINDOWS_ID_BIAS)
	#define CTL_ELINK_INFO               (4 + WINDOWS_ID_BIAS)
#define DLG_QUOTKIND              317 // Вид котировки товара
	#define cmDscntOnGroups              101
	#define cmQuotKindRestr              102
	#define CTL_QUOTKIND_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_ID              (2 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_OP              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTKIND_OP           (4 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_DISCOUNT        (5 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_SYMB            (6 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_FLAGS           (7 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_ACCSHEET        (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTKIND_ACCSHEET     (9 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_RANK            (10 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_PERIOD          (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_QUOTKIND_PERIOD       (12 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_TIMEPERIOD      (13 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_ST_RESTR        (14 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_RNDGDIR         (15 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_RNDGPREC        (16 + WINDOWS_ID_BIAS)
	#define CTL_QUOTKIND_FORMULA         (17 + WINDOWS_ID_BIAS) // @v12.0.10
#define DLG_GQUOT                 318 // GQUOT   Котировки товара
#define DLG_GQUOTSC               682 // GQUOTSC Контрактные цены и отклонения по поставщикам
#define DLG_GQUOTPC              1112 // GQUOTPC Поправочные коэффициенты прогноза продаж
	#define cmGrpQuot                    101 // Котировки на группу
	#define CTL_GQUOT_LOCLIST            (1 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_GOODS              (2 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_CUR                (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GQUOT_CUR             (4 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_BASE               (5 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LABB               (6 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_BASEP              (7 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL1               (8 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB1               (9 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P1                (10 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL2              (11 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB2              (12 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P2                (13 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL3              (14 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB3              (15 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P3                (16 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL4              (17 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB4              (18 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P4                (19 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL5              (20 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB5              (21 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P5                (22 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL6              (23 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB6              (24 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P6                (25 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL7              (26 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB7              (27 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P7                (28 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL8              (29 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB8              (30 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P8                (31 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL9              (32 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB9              (33 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P9                (34 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_VAL10             (35 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LAB10             (36 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_P10               (37 + WINDOWS_ID_BIAS)

	#define CTL_GQUOT_GNOTE             (38 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_ARTICLE           (39 + WINDOWS_ID_BIAS)
	#define CTLSEL_GQUOT_ARTICLE        (40 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_LASTPRICES        (41 + WINDOWS_ID_BIAS) // static text
	#define CTL_GQUOT_COST              (51 + WINDOWS_ID_BIAS) // DLG_GQUOTSC
	#define CTL_GQUOT_LOWDEV            (52 + WINDOWS_ID_BIAS) // DLG_GQUOTSC
	#define CTL_GQUOT_UPPDEV            (53 + WINDOWS_ID_BIAS) // DLG_GQUOTSC
	#define CTL_GQUOT_VIEW              (54 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_EMBED_WINDOW      (55 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_GRPQUOT           (56 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_ACCSHEET          (57 + WINDOWS_ID_BIAS)
	#define CTLSEL_GQUOT_ACCSHEET       (58 + WINDOWS_ID_BIAS)
	#define CTL_GQUOT_COEFF             (59 + WINDOWS_ID_BIAS) // DLG_GQUOTPC
	#define CTL_GQUOT_ST_INFO           (60 + WINDOWS_ID_BIAS) // static text
	#define CTL_GQUOT_PRDLIST           (61 + WINDOWS_ID_BIAS)
// Управляющие элементы и класс для DLG_INVFLT те же, что и DLG_BILLFLT
#define DLG_INVFLT                319 // Фильтр по инвентаризациям
#define DLG_INVITEM               320 // Элемент инвентаризации
	#define CTL_INVITEM_GOODSGRP         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVITEM_GOODSGRP      (2 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_GOODS            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVITEM_GOODS         (4 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_UNITPERPACK      (5 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_PACKS            (6 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_QUANTITY         (7 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_STOCKREST        (8 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_DIFFREST         (9 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_PRICE           (10 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_FRAME           (11 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_WROFFPRICE      (12 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_SERIAL          (13 + WINDOWS_ID_BIAS)
	#define CTL_INVITEM_CODE            (14 + WINDOWS_ID_BIAS) // Для DLG_INVITEMQ
#define DLG_OPKINVE               321 // Опции инвентаризации
	#define CTL_OPKINVE_WRDNOP           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINVE_WRDNOP        (2 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_WRDNOBJ          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINVE_WRDNOBJ       (4 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_WRUPOP           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINVE_WRUPOP        (6 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_WRUPOBJ          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINVE_WRUPOBJ       (8 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_NOMINAL          (9 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_DEFREST         (10 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_AUTOMETHOD      (11 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_CALCPRICE       (12 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_FLAGS           (13 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_FRAME1          (14 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_FRAME2          (15 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_ACCSLMODE       (16 + WINDOWS_ID_BIAS)
	#define CTL_OPKINVE_ONWROFFST       (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINVE_ONWROFFST    (18 + WINDOWS_ID_BIAS)
#define DLG_OPRINV                322 // Вид операции "Инвентаризация"
	// Управляющие элементы те же, что и в DLG_OPRKIND
#define DLG_INVDIFFLT             323 // Фильтр по рассогласованиям инвентаризации
	#define CTL_INVDIFFLT_GRP            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVDIFFLT_GRP         (2 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_FRAME          (3 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_SIGN           (4 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_UNIT           (5 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_PCTVAL         (6 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_VAL            (7 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_WROFF          (8 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_ORD            (9 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_STRGLOC        (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVDIFFLT_STRGLOC     (11 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_SUBST          (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVDIFFLT_SUBST       (13 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_SUBSTG         (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVDIFFLT_SUBSTG      (15 + WINDOWS_ID_BIAS)
	#define CTL_INVDIFFLT_GOODS          (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_INVDIFFLT_GOODS       (17 + WINDOWS_ID_BIAS)
#define DLG_UNIPRICE              324 // Унификация цен
#define DLG_UNICOST               352 // Унификация цен поступления по поставщику
	#define CTL_UNIPRICE_OPRKIND         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_UNIPRICE_OPRKIND      (2 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_LOC             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_UNIPRICE_LOC          (4 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_PCT             (6 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_FRAME           (7 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_PREC            (8 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_ROUNDDIR        (9 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_GGRP           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_UNIPRICE_GGRP        (11 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_EXCLGGRPF      (12 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_QUOTK          (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_UNIPRICE_QUOTK       (14 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_OLDPRICE       (15 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_MODE           (16 + WINDOWS_ID_BIAS)
	#define CTL_UNIPRICE_PSRC           (17 + WINDOWS_ID_BIAS)
#define DLG_QUOTUPD               326 // QUOTUPD Массовое изменение котировок
	#define cmQuotUpdSetQuot             101 // Установка значения котировки
	#define CTL_QUOTUPD_KIND             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPD_KIND          (2 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_LOC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPD_LOC           (4 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_GGRP             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPD_GGRP          (6 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_PCT              (7 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_HOW              (8 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_NEXS             (9 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_ALLLOCS         (10 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_AR              (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPD_AR           (12 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_WARN            (13 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_WHAT            (14 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_ST_VALEXT       (15 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPD_GOODS           (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPD_GOODS        (17 + WINDOWS_ID_BIAS)
    #define CTL_QUOTUPD_TEST            (18 + WINDOWS_ID_BIAS)
#define DLG_PRNCASHBOOK           327 // Печать кассовой книги
	#define CTL_PRNCASHBOOK_WHAT         (1 + WINDOWS_ID_BIAS)
#define DLG_PSNOPK                328 // Вид персональной операции
#define cmPsnOpkPrmr                 101
#define cmPsnOpkScnd                 102
#define cmPsnOpkExtraObj             103
#define cmPsnOpkTags                 104
#define cmPsnOpkActl                 105
	#define CTL_PSNOPK_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_PAIR              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPK_PAIR           (3 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_REGTYP            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPK_REGTYP         (7 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_FLAGS             (8 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_PAIRTYPE          (9 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_SYMB              (10 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_PARENT            (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPK_PARENT         (12 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_ID                (13 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_REDOTIMEOUT       (14 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_RSC               (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPK_RSC            (16 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPK_RSCMAXTIMES       (17 + WINDOWS_ID_BIAS)
#define DLG_PSNOPKPC              329 // Ограничения по первичной персоналии
#define DLG_PSNOPKSC              384 // Ограничения по вторичной персоналии
	#define CTL_PSNOPKPC_KIND            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPKPC_KIND         (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPKPC_STATUSTYP       (3 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPKPC_DEFAULT         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPKPC_DEFAULT      (5 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPKPC_RTAG            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNOPKPC_RTAG         (7 + WINDOWS_ID_BIAS)
	#define CTL_PSNOPKPC_SCSLIST         (8 + WINDOWS_ID_BIAS)
#define DLG_ABOUT                 331 // О программе
	#define CTL_ABOUT_VERSION            (1 + WINDOWS_ID_BIAS)
	#define CTL_ABOUT_LICNAME            (2 + WINDOWS_ID_BIAS)
	#define CTL_ABOUT_LICREG             (3 + WINDOWS_ID_BIAS)
	#define CTL_ABOUT_LICCOUNT           (4 + WINDOWS_ID_BIAS)
	#define CTL_ABOUT_FREELICCOUNT       (5 + WINDOWS_ID_BIAS)
	#define CTL_ABOUT_LICADDON           (6 + WINDOWS_ID_BIAS)
	#define CTL_ABOUT_TEAM               (7 + WINDOWS_ID_BIAS)
#define DLG_PSNEVFLT              332 // Фильтр по персональным событиям
	#define cmOpList                     101
	#define CTL_PSNEVFLT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_OP              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_OP           (3 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_PRMR            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_PRMR         (5 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_SCND            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_SCND         (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_PSNEVFLT_PERIOD       (8 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_SUBST           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_SUBST        (10 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_SUBSTDT         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_SUBSTDT      (12 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_FLAGS           (13 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_EPRMRREG        (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_EPRMRREG     (15 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVFLT_EPRMRTAG        (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVFLT_EPRMRTAG     (17 + WINDOWS_ID_BIAS)
#define DLG_PSNEVNT               334 // Персональные событи
	#define cmPsnEvntRegister         101
	#define cmPsnEvntLocation         102
	#define cmVkApiPost               103
	#define cmPostPhotoMsg            104
	#define cmAddPhotoToAlbum         105
	#define cmAddGoodToMarket         106
	#define CTL_PSNEVNT_PRMR             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVNT_PRMR          (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_SCND             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVNT_SCND          (4 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_OP               (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVNT_OP            (6 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_EXTRA            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVNT_EXTRA         (8 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_DATE             (9 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_MEMO            (10 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_DIV             (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVNT_DIV          (13 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_POST            (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNEVNT_POST         (15 + WINDOWS_ID_BIAS)
	#define CTLCAL_PSNEVNT_DATE         (16 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_IMAGE           (17 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_TAGLIST         (18 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_TIME            (19 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_ST_PAIR         (20 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_PRMRSCARD       (21 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_SCNDSCARD       (22 + WINDOWS_ID_BIAS)
	#define CTL_PSNEVNT_FLAGS           (23 + WINDOWS_ID_BIAS)
	#define CTLTM_PSNEVNT_TIME          (24 + WINDOWS_ID_BIAS) // @v11.2.6
#define DLG_GRESTTOTAL            335 // Итог по остаткам товаров
	#define CTL_GRESTTOTAL_COUNT         (1 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_REST          (2 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_COST          (3 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_PRICE         (4 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_AMTLIST       (5 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_PHREST        (6 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_PCTADD        (7 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_DRAFTRCPT     (8 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_DRAFCOST      (9 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_DRAFPRICE     (10 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_CVAT          (11 + WINDOWS_ID_BIAS)
	#define CTL_GRESTTOTAL_PVAT          (12 + WINDOWS_ID_BIAS)
//                                338 // (@v3.1.12 Not) Occupied
#define DLG_DBXCHGCFG             339 // Конфигурация обмена между разделами
	#define CTL_DBXCHGCFG_LOG            (1 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_FLAGS          (2 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_ONELOC         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBXCHGCFG_ONELOC      (4 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_PCTADD         (5 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_RLZORD         (6 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_CHARRYF        (7 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_DROP           (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBXCHGCFG_DROP        (9 + WINDOWS_ID_BIAS)
	#define CTL_DBXCHGCFG_SPCSGG         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBXCHGCFG_SPCSGG      (11 + WINDOWS_ID_BIAS)
#define DLG_OBJTAG                340 // Тег объекта
#define DLG_OBJTAGGRP             341 // Группа тегов
#define DLG_OBJTAGTYP             342 // Тип тега
	#define CTL_OBJTAG_GRP               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJTAG_GRP            (2 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_NAME              (3 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_ID                (4 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_TYPE              (5 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_OBJTYP            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJTAG_OBJTYP         (7 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_SYMB              (8 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_FLAGS             (9 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_OBJGRP            (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJTAG_OBJGRP         (11 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_TAGOBJTYP         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJTAG_TAGOBJTYP      (13 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_HOTKEY            (14 + WINDOWS_ID_BIAS)
	#define CTL_OBJTAG_HOTKEY_HINT       (15 + WINDOWS_ID_BIAS)
// Диалог просмотра тегов (аналогичен DLG_OBJVIEW)
#define DLG_TAGVIEW               343
#define DLG_TAGENUMVIEW           344
	#define CTL_TAGENUMVIEW_LIST         (1 + WINDOWS_ID_BIAS)
#define DLG_TAGVLOG               345
#define DLG_TAGVNMB               346
#define DLG_TAGVSTR               347
#define DLG_TAGVLNK               348
#define DLG_TAGVDATE              349
#define DLG_TAGVGUID              1150
#define DLG_TAGVIMG               1182
#define DLG_TAGVTIMESTAMP         1204
#define DLG_TAGVLOCLNK            1461 // @v12.2.10 Диалог значения тега, ссылающегося на адрес персоналии
// Reserve                        350
	#define cmGenerate            101
	#define CTL_TAGV_OBJNAME             (1  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_TAG                 (2  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_BOOL                (3  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_STR                 (4  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_NUMBER              (5  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_LINK                (6  + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGV_LINK             (7  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_DATE                (8  + WINDOWS_ID_BIAS)
	#define CTLCAL_TAGV_DATE             (9  + WINDOWS_ID_BIAS)
	#define CTL_TAGV_IMAGE               (10 + WINDOWS_ID_BIAS)
	#define CTL_TAGV_TIME                (11 + WINDOWS_ID_BIAS)
	#define CTLTM_TAGV_TIME              (12 + WINDOWS_ID_BIAS)
	#define CTL_TAGV_TAGID               (13 + WINDOWS_ID_BIAS) // @v11.5.12
	#define CTL_TAGV_INT                 (14 + WINDOWS_ID_BIAS) // @v11.8.12
	#define CTL_TAGV_HOBJ1               (15 + WINDOWS_ID_BIAS) // @v12.2.10
	#define CTLSEL_TAGV_HOBJ1            (16 + WINDOWS_ID_BIAS) // @v12.2.10 Вспомогательный объект #1 для выбора ссылки
	#define CTL_TAGV_HOBJ2               (17 + WINDOWS_ID_BIAS) // @v12.2.10
	#define CTLSEL_TAGV_HOBJ2            (18 + WINDOWS_ID_BIAS) // @v12.2.10 Вспомогательный объект #2 для выбора ссылки
#define DLG_TAGVALVIEW            351
	#define CTL_TAGVALVIEW_LIST          (1 + WINDOWS_ID_BIAS)
	#define CTL_TAGVALVIEW_OBJ           (2 + WINDOWS_ID_BIAS)
	#define CTL_TAGVALVIEW_UPD           (3 + WINDOWS_ID_BIAS)
//                                352 // Occupied
#define DLG_MOVLOTOPS             353 // Параметры переноса операций между лотами
	#define CTL_MOVLOTOPS_MERGE          (1 + WINDOWS_ID_BIAS)
	#define CTL_MOVLOTOPS_AVG            (2 + WINDOWS_ID_BIAS)
	#define CTL_MOVLOTOPS_RMVSRC         (3 + WINDOWS_ID_BIAS)
	#define CTL_MOVLOTOPS_RMVREVAL       (4 + WINDOWS_ID_BIAS)
//                      354, 355, 356 // Occupied
#define DLG_RENT                  357 // Условия начисления ренты
	#define CTL_RENT_FROMTO              (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_RENT_FROMTO           (2 + WINDOWS_ID_BIAS)
	#define CTL_RENT_PERIOD              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_RENT_PERIOD           (4 + WINDOWS_ID_BIAS)
	#define CTL_RENT_SUMPRD              (6 + WINDOWS_ID_BIAS)
	#define CTL_RENT_ACMDATE             (8 + WINDOWS_ID_BIAS)
	#define CTL_RENT_ISPERCENT           (11 + WINDOWS_ID_BIAS)
	#define CTL_RENT_PERCENT             (12 + WINDOWS_ID_BIAS)
#define DLG_OPKVIEW               358 // Список видов операций
#define DLG_OPLINKSVIEW           447 // Список видов операций по привязке
	#define cmOpListLinks                101
	#define CTL_OPKVIEW_TYPE             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKVIEW_TYPE          (2 + WINDOWS_ID_BIAS)
	#define CTL_OPKVIEW_LIST             (3 + WINDOWS_ID_BIAS)
#define DLG_GENOPRKIND            359 // Обобщенный вид операции
	// CTL[SEL]_OPRKIND_XXX
#define DLG_TRFRANLZ              360 // Фильтр анализа товарных операций
	#define cmTaGrpngOptions             110
	// CTL[SEL]_GTO_XXX
#define DLG_STAFF                 361 // Штатная должность
	#define cmAmounts                    101
	#define CTL_STAFF_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_STAFF_ORG                (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFF_ORG             (3 + WINDOWS_ID_BIAS)
	#define CTL_STAFF_DIV                (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFF_DIV             (5 + WINDOWS_ID_BIAS)
	#define CTL_STAFF_COUNT              (6 + WINDOWS_ID_BIAS)
	#define CTL_STAFF_RANK               (9 + WINDOWS_ID_BIAS)
	#define CTL_STAFF_FIX                (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFF_FIX             (11 + WINDOWS_ID_BIAS)
	#define CTL_STAFF_CHARGE             (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFF_CHARGE          (13 + WINDOWS_ID_BIAS)
#define DLG_PERSONPOST            362 // PSNPOST Должностное назначение
	#define CTL_PERSONPOST_ORG           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONPOST_ORG        (2 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_DIV           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONPOST_DIV        (4 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_POST          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONPOST_POST       (6 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_PERSON        (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONPOST_PERSON     (8 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_DATE          (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_PERSONPOST_DATE       (10 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_FINISH        (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_PERSONPOST_FINISH     (12 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_CLOSED        (13 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_CHARGE        (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONPOST_CHARGE     (15 + WINDOWS_ID_BIAS)
	#define CTL_PERSONPOST_ID            (16 + WINDOWS_ID_BIAS)

#define DLG_STAFFLFLT             363 // Фильтр по штатному расписанию
	#define CTL_STAFFLFLT_ORG            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFLFLT_ORG         (2 + WINDOWS_ID_BIAS)
	#define CTL_STAFFLFLT_DIV            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFLFLT_DIV         (4 + WINDOWS_ID_BIAS)
#define DLG_DIVVIEW               364 // Подразделени
	#define CTL_DIVVIEW_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_DIVVIEW_ORG              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_DIVVIEW_ORG           (3 + WINDOWS_ID_BIAS)
//                                365..380 // Occupied
#define DLG_POKEXV                381 // Расширение персональной операции
	#define CTL_POKEXV_GRP               (1 + WINDOWS_ID_BIAS)
	#define CTL_POKEXV_ALLOWEDTAGS      (2 + WINDOWS_ID_BIAS)
#define DLG_MAINORG               382 // Параметры главной организации
	#define CTL_MAINORG_DEF              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG_DEF           (2 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG_DIRECTOR         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG_DIRECTOR      (4 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG_ACCTNT           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG_ACCTNT        (6 + WINDOWS_ID_BIAS)
//                                383 // Occupied
//                                384 // Occupied
#define DLG_POKACTL               385 // Список действий персональной операции
	#define CTL_POKACTL_LIST             (1 + WINDOWS_ID_BIAS)
#define DLG_POVERB                386 // Действие персональной операции
	#define CTL_POVERB_SUBJ              (1 + WINDOWS_ID_BIAS)
	#define CTL_POVERB_VERB              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_POVERB_VERB           (3 + WINDOWS_ID_BIAS)
	#define CTL_POVERB_LINK              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_POVERB_LINK           (5 + WINDOWS_ID_BIAS)
	#define CTL_POVERB_EXTEXT            (6 + WINDOWS_ID_BIAS)
	#define CTL_POVERB_TAG               (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_POVERB_TAG            (8 + WINDOWS_ID_BIAS)
	#define CTL_POVERB_CMDTXT            (9 + WINDOWS_ID_BIAS)
	#define CTL_POVERB_FLAGS             (10 + WINDOWS_ID_BIAS)
#define DLG_GSDATA                388 // Выбор структуры товара
	#define cmShowGoodsStruc             101
	#define CTL_GSDATA_NAMEDSTRUC        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSDATA_NAMEDSTRUC     (2 + WINDOWS_ID_BIAS)
#define DLG_PLISTUPD              389 // Обновление прайс-листа
	#define CTL_PLISTUPD_SEL             (1 + WINDOWS_ID_BIAS)
	#define CTL_PLISTUPD_DT              (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_PLISTUPD_DT           (3 + WINDOWS_ID_BIAS)
#define DLG_CURRCFG               390 // Текущая конфигураци
	#define CTL_CURRCFG_MAINORG          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURRCFG_MAINORG       (2 + WINDOWS_ID_BIAS)
	#define CTL_CURRCFG_LOC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURRCFG_LOC           (4 + WINDOWS_ID_BIAS)
#define DLG_CCFGOP                391 // Общая конфигурация операций
	#define CTL_CCFGOP_SUPPLSH           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_SUPPLSH        (2 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_SELLSH            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_SELLSH         (4 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_RCPTOP            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_RCPTOP         (6 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_RETAILOP          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_RETAILOP       (8 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_RETAILRET         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_RETAILRET      (10 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_INTRRCPT          (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_INTRRCPT       (12 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_ACOP              (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_ACOP           (14 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_PREPAYGOODS       (15 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_FLAGS             (16 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_DRAFTRCPT         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_DRAFTRCPT      (18 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_DYNGTYPESA        (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_DYNGTYPESA     (20 + WINDOWS_ID_BIAS)
	#define CTL_CCFGOP_PREPAYWARE        (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCFGOP_PREPAYWARE     (22 + WINDOWS_ID_BIAS)
#define DLG_TDEFICIT              392 // Приходование дефицита при приеме документов
	#define CTL_TDEFICIT_DATE            (1 + WINDOWS_ID_BIAS)
	#define CTL_TDEFICIT_PCTADD          (2 + WINDOWS_ID_BIAS)
	#define CTL_TDEFICIT_BEGDATE         (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_TDEFICIT_DATE         (4 + WINDOWS_ID_BIAS) // @alena
	#define CTL_TDEFICIT_LIST            (5 + WINDOWS_ID_BIAS)
#define DLG_MKRPTFLS              393 // Создание файлов для WIN32 отчетов
	#define CTL_MKRPTFLS_RPTNAME         (1 + WINDOWS_ID_BIAS)
	#define CTL_MKRPTFLS_RPTPATH         (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_MKRPTFLS_RPTPATH      (3 + WINDOWS_ID_BIAS)
//  395, 396, 397, 398, 399, 400, 401 // Occupied
#define DLG_GDSTAX                402 // Налоговая группа товаров
	#define cmGoodsTaxList               101
	#define CTL_GDSTAX_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_ID                (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_VAT               (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_EXCISE            (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_STAX              (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_ORDER             (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_FLAGS             (7 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_PERIOD            (8 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_OP                (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSTAX_OP             (10 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_NOLOTEXCISE       (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_GDSTAX_PERIOD         (12 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_SYMB              (13 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAX_SPCVAT            (14 + WINDOWS_ID_BIAS) // @v12.2.3
#define DLG_GTAXTEST              403
	#define CTL_GTAXTEST_AMOUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_AMTTYPE         (2 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_AT              (3 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_EXCISE          (4 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_VAT             (5 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_STAX            (6 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_SUM             (7 + WINDOWS_ID_BIAS)
	#define CTL_GTAXTEST_EXCL            (8 + WINDOWS_ID_BIAS)
#define DLG_BCPRT                 404 // Параметры принтера штрихкодов
	#define CTL_BCPRT_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_ID                 (2 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_DEVICE             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BCPRT_DEVICE          (4 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_PORT               (5 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_FORM               (6 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_BAUD               (7 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_PARITY             (8 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_DATA               (9 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_BMP                (10 + WINDOWS_ID_BIAS)
	#define CTLMNU_BCPRT_PORT            (11 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_NARROWPT           (12 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_WIDEPT             (13 + WINDOWS_ID_BIAS)
	#define CTL_BCPRT_CP                 (14 + WINDOWS_ID_BIAS)
#define DLG_BCPLABEL              405 // BCPLABEL Печать этикетки на принтере штрихкодов
#define DLG_BCPLABEL_EXT          812 // BCPLBLEX Печать этикетки на принтере штрихкодов
	#define CTL_BCPLABEL_PRINTER         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BCPLABEL_PRINTER      (2 + WINDOWS_ID_BIAS)
	#define CTL_BCPLABEL_PORT            (3 + WINDOWS_ID_BIAS)
	#define CTL_BCPLABEL_COUNT           (4 + WINDOWS_ID_BIAS)
	#define CTL_BCPLABEL_LOC             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_BCPLABEL_LOC          (6 + WINDOWS_ID_BIAS)
	#define CTL_BCPLABEL_HOW             (7 + WINDOWS_ID_BIAS)
	#define CTL_BCPLABEL_FLAGS           (8 + WINDOWS_ID_BIAS)
	#define CTL_BCPLABEL_BMP             (9 + WINDOWS_ID_BIAS)
#define DLG_SHANLZFLT             406 // Фильтр по анализу отгрузки
	#define CTL_SHANLZFLT_PERIOD         (1 + WINDOWS_ID_BIAS)
	#define CTL_SHANLZFLT_OPRKIND        (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SHANLZFLT_OPRKIND     (3 + WINDOWS_ID_BIAS)
	#define CTL_SHANLZFLT_OBJECT         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SHANLZFLT_OBJECT      (5 + WINDOWS_ID_BIAS)
	#define CTL_SHANLZFLT_LOC            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SHANLZFLT_LOC         (7 + WINDOWS_ID_BIAS)
	#define CTL_SHANLZFLT_FLAGS          (8 + WINDOWS_ID_BIAS)
	#define CTL_SHANLZFLT_LABEL          (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_SHANLZFLT_PERIOD      (10 + WINDOWS_ID_BIAS)
#define DLG_BILLEXT               407 // Дополнительная информация по документу
#define DLG_BILLEXTFLT            516 // Фильтр по доп информации документа
#define DLG_BILLEXTFLT2           522 // Фильтр по доп информации документа 2
	#define CTL_BILLEXT_PAYER            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXT_PAYER         (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_AGENT            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXT_AGENT         (4 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_INVCCODE         (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_INVCDATE         (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLEXT_INVCDATE      (7 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_PAYDATE          (8 + WINDOWS_ID_BIAS) // DLG_BILLEXTMOD
	#define CTLCAL_BILLEXT_PAYDATE       (9 + WINDOWS_ID_BIAS) // DLG_BILLEXTMOD
	#define CTL_BILLEXT_SHIPPED          (10 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_EXTPQUOT         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXT_EXTPQUOT      (12 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_PAYMBCODE        (13 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_PAYMBDATE        (14 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLEXT_PAYMBDATE     (15 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_SCARDN           (16 + WINDOWS_ID_BIAS) // @v4.4.1 Номер пластиковой карты
	#define CTL_BILLEXTFLT_STAXTGGL      (21 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_CREATOR          (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXT_CREATOR       (23 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_DUEPERIOD        (24 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLEXT_DUEPERIOD     (25 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_DCLTGGL       (26 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_OUTERCODE        (27 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_OUTERDATE        (28 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLEXT_OUTERDATE     (29 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_RECADV        (30 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_RECADVCFM     (31 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXT_AGREEMENT        (32 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXT_AGREEMENT     (33 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_CHECKPRST     (34 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_GGRP          (35 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXTFLT_GGRP       (36 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_ORDFFST       (37 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_CLICAT        (38 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXTFLT_CLICAT     (39 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXTFLT_DPORT         (40 + WINDOWS_ID_BIAS) // @v11.9.4 Порт разгрузки
	#define CTLSEL_BILLEXTFLT_DPORT      (41 + WINDOWS_ID_BIAS) // @v11.9.4 Порт разгрузки (selector)

#define DLG_OPRPOP                408 // Параметры зачета для вида операции
	#define CTL_OPRPOP_LIST              (1 + WINDOWS_ID_BIAS)
	#define CTL_OPRPOP_PERIOD            (2 + WINDOWS_ID_BIAS)
	#define CTL_OPRPOP_FLAGS             (3 + WINDOWS_ID_BIAS)
	#define CTL_OPRPOP_PSNRELTYPE        (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPRPOP_PSNRELTYPE     (5 + WINDOWS_ID_BIAS)
#define DLG_CFM_RECKONING         409 // Подтверждение на зачет платежа
#define DLG_CFM_RECKONDEBT        410 // Подтверждение на зачет долга
	#define cmRcknSelectedBill           2001
	#define CTL_CFM_RECKONING_OBJ        (1 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_DEBT       (2 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_AMT        (3 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_DTSEL      (4 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_DT         (5 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_CURSYM     (6 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_LIST       (7 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_SELBUT     (8 + WINDOWS_ID_BIAS)
	#define CTL_CFM_RECKONING_ALLBUT     (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_CFM_RECKONING_DT      (10 + WINDOWS_ID_BIAS) // @alena
	#define CTL_CFM_RECKONING_INFO       (11 + WINDOWS_ID_BIAS)
#define DLG_BILLTOTAL2            411 // Итоги по выборке документов
	#define cmCalcVAT                    101
	#define CTL_BILLTOTAL2_COUNT         (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLTOTAL2_SUM           (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLTOTAL2_AMTLIST       (3 + WINDOWS_ID_BIAS)
	#define CTL_BILLTOTAL2_DEBT          (4 + WINDOWS_ID_BIAS)
#define DLG_CURRENCY              412 // Валюта
	#define CTL_CURRENCY_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_CURRENCY_ID              (2 + WINDOWS_ID_BIAS)
	#define CTL_CURRENCY_SYMB            (3 + WINDOWS_ID_BIAS)
	#define CTL_CURRENCY_CODE            (4 + WINDOWS_ID_BIAS)
#define DLG_CURRATE               413 // Валютный курс
	#define CTL_CURRATE_DATE             (1 + WINDOWS_ID_BIAS)
	#define CTL_CURRATE_CUR              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURRATE_CUR           (3 + WINDOWS_ID_BIAS)
	#define CTL_CURRATE_RATETYPE         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURRATE_RATETYPE      (5 + WINDOWS_ID_BIAS)
	#define CTL_CURRATE_RELCUR           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURRATE_RELCUR        (7 + WINDOWS_ID_BIAS)
	#define CTL_CURRATE_RATE             (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_CURRATE_DATE          (9 + WINDOWS_ID_BIAS) // @alena
#define DLG_CRATEFLT              414 // Фильтр валютных курсов
	#define CTL_CRATEFLT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_CRATEFLT_CUR             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_CRATEFLT_CUR          (3 + WINDOWS_ID_BIAS)
	#define CTL_CRATEFLT_RATETYPE        (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CRATEFLT_RATETYPE     (5 + WINDOWS_ID_BIAS)
	#define CTL_CRATEFLT_RELCUR          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CRATEFLT_RELCUR       (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_CRATEFLT_PERIOD       (8 + WINDOWS_ID_BIAS) // @alena
#define DLG_CURCFG                415 // Конфигурация валютного учета
	#define CTL_CURCFG_RATETYPE          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURCFG_RATETYPE       (2 + WINDOWS_ID_BIAS)
	#define CTL_CURCFG_BASECUR           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CURCFG_BASECUR        (4 + WINDOWS_ID_BIAS)
	#define CTL_CURCFG_FLAGS             (5 + WINDOWS_ID_BIAS)
#define DLG_PRNLOTS               420 // Печать лотов
	#define CTL_PRNLOTS_ORDER            (1 + WINDOWS_ID_BIAS)
	#define CTL_PRNLOTS_FLAGS            (2 + WINDOWS_ID_BIAS)
#define DLG_EDITLOT               421 // Изменение лота
	#define CTL_EDITLOT_COST             (1 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_PRICE            (2 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_QUANTITY         (3 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_UNITPERPACK      (4 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_NAME             (5 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_BILLNO           (6 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_SUPPL            (7 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_LOTID            (8 + WINDOWS_ID_BIAS)
	#define CTL_EDITLOT_DELETE           (9 + WINDOWS_ID_BIAS)
#define DLG_ATURNTOTAL            423 // Итог по выборке бух проводок
	#define CTL_ATURNTOTAL_COUNT         (1 + WINDOWS_ID_BIAS)
	#define CTL_ATURNTOTAL_AMTLIST       (2 + WINDOWS_ID_BIAS)
#define DLG_AANLZTOTAL            424 // Итоги анализа счета
	#define CTL_AANLZTOTAL_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_ACC           (2 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_COUNT         (3 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_INRESTD       (4 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_INRESTC       (5 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_INREST        (6 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_OUTRESTD      (7 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_OUTRESTC      (8 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_OUTREST       (9 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_DBT           (10 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_CRD           (11 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_CUR           (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_AANLZTOTAL_CUR        (13 + WINDOWS_ID_BIAS)
	#define CTL_AANLZTOTAL_DIFF          (14 + WINDOWS_ID_BIAS)
//                                425 // Occupied (DLG_BILLAMTCUR)
#define DLG_SELCRATE              426 // Выбор валютного курса
	#define CTL_SELCRATE_LIST            (1 + WINDOWS_ID_BIAS)
	#define CTL_SELCRATE_CUR             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELCRATE_CUR          (3 + WINDOWS_ID_BIAS)
	#define CTL_SELCRATE_RATE            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELCRATE_RATE         (5 + WINDOWS_ID_BIAS)
#define DLG_SELAMTSYMB            427 // Выбор суммы или формулы
	#define CTL_SELAMTSYMB_LIST          (1 + WINDOWS_ID_BIAS)
#define DLG_BNKPAYM               428 // Дополнительная информация по документу
//                                       для печати банковского платежного поручени
	#define cmTaxMarkers                 2001
	#define CTL_BNKPAYM_DT               (1 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_BILLNO           (2 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_PAYERKIND        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKPAYM_PAYERKIND     (4 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_PAYER            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKPAYM_PAYER         (6 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_PAYERACC         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKPAYM_PAYERACC      (8 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_PAYERBNK         (9 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_RCVRKIND         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKPAYM_RCVRKIND      (11 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_RCVR             (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKPAYM_RCVR          (13 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_RCVRACC          (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKPAYM_RCVRACC       (15 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_RCVRBNK          (16 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_METHOD           (17 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_QUEUEING         (18 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_AMOUNT           (19 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_VATRATE          (20 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_VATSUM           (21 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_WHAT             (22 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_ACCEPT           (23 + WINDOWS_ID_BIAS)
	#define CTLCAL_BNKPAYM_DT            (24 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_PAYERSTATUS      (25 + WINDOWS_ID_BIAS)
	#define CTL_BNKPAYM_FPURPS           (26 + WINDOWS_ID_BIAS)
#define DLG_OPRPOOL               429 // Доп опции операции пула документов
	#define CTL_OPRPOOL_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_OPRPOOL_FLAGS            (2 + WINDOWS_ID_BIAS)
	#define CTL_OPRPOOL_COMMF            (3 + WINDOWS_ID_BIAS)
#define DLG_BPOOLADD              430 // Опции добавления документа в пул
	#define cmBillFilt                   101
	#define CTL_BPOOLADD_VERB            (1 + WINDOWS_ID_BIAS)
	#define CTL_BPOOLADD_BILLKIND        (2 + WINDOWS_ID_BIAS)
#define DLG_BPOOLRMV              431 // Опции удаления документа из пула
	#define CTL_BPOOLRMV_VERB            (1 + WINDOWS_ID_BIAS)
#define DLG_PRNPOOLBILL           432 // Опции печати пула документов
	#define CTL_PRNPOOLBILL_WHAT         (1 + WINDOWS_ID_BIAS)
#define DLG_GTANLZTOTAL           433 // Итог налогового анализа товарооборота
	#define CTL_GTANLZTOTAL_COUNT        (1 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_TOCOST       (2 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_TOPRICE      (3 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_INCOME       (4 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_EXCISE       (5 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_CVAT         (6 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_INCVAT       (7 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_VAT          (8 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_STAX         (9 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_INOUTVL      (10 + WINDOWS_ID_BIAS) // IN/OUT VAT LIST
	#define CTL_GTANLZTOTAL_RCPT         (11 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_EXP          (12 + WINDOWS_ID_BIAS)
	#define CTL_GTANLZTOTAL_REST         (13 + WINDOWS_ID_BIAS)
#define DLG_OBJTRANSM             434 // Передача объектов на другой раздел
	#define cmAddTrnsmObj        101
	#define cmDelTrnsmObj        102
	#define CTL_OBJTRANSM_DBDIVLIST      (1 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_OBJLIST        (2 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_PROTOCOL       (3 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_DT             (4 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_TM             (5 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_FLAGS          (6 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_TRNSMFLAGS     (7 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_FILT           (8 + WINDOWS_ID_BIAS)
	#define CTL_OBJTRANSM_INMASS         (9 + WINDOWS_ID_BIAS)
#define DLG_OPCNTR                435 // Счетчик операций
#define DLG_OPKCOUNTER            436 // Выбор счетчика в виде операций
	#define cmDiffByLoc                  101
	#define CTL_OPKCOUNTER_TEMPL         (1 + WINDOWS_ID_BIAS)
	#define CTL_OPKCOUNTER_COUNTER       (2 + WINDOWS_ID_BIAS)
	#define CTL_OPKCOUNTER_CNTR          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKCOUNTER_CNTR       (4 + WINDOWS_ID_BIAS)
	#define CTL_OPKCOUNTER_FLAGS         (5 + WINDOWS_ID_BIAS)
	#define CTL_OPKCOUNTER_NAME          (6 + WINDOWS_ID_BIAS)
	#define CTL_OPKCOUNTER_ID            (7 + WINDOWS_ID_BIAS)
	#define CTL_OPKCOUNTER_SYMB          (8 + WINDOWS_ID_BIAS)
#define DLG_UPDCNTR               437 // Изменение счетчика операций
	#define CTL_UPDCNTR_COUNTER          (1 + WINDOWS_ID_BIAS)
	#define CTL_UPDCNTR_FLAGS            (2 + WINDOWS_ID_BIAS)
#define DLG_REPLGTX               438 // Объединение налоговых групп
	#define CTL_REPLGTX_DEST             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLGTX_DEST          (2 + WINDOWS_ID_BIAS)
	#define CTL_REPLGTX_SRC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLGTX_SRC           (4 + WINDOWS_ID_BIAS)
#define DLG_DEBTTOTAL             439 // Итоги долговой ведомости
	#define CTL_DEBTTOTAL_COUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_DEBTTOTAL_LIST           (2 + WINDOWS_ID_BIAS)
#define DLG_CUTTING               440 // Усечение базы данных
	#define CTL_CUTTING_DATE             (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_CUTTING_DATE          (2 + WINDOWS_ID_BIAS)
#define DLG_VATBL                 441 // Элемент списка конфигурации книги продаж/покупок
	#define CTL_VATBL_OPRKIND            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBL_OPRKIND         (2 + WINDOWS_ID_BIAS)
	#define CTL_VATBL_FLAGS              (3 + WINDOWS_ID_BIAS)
	#define CTL_VATBL_EXPBYFACT          (4 + WINDOWS_ID_BIAS)
	#define CTL_VATBL_MAINAMT            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_VATBL_MAINAMT         (6 + WINDOWS_ID_BIAS)
	#define CTL_VATBL_SIGNFILT           (7 + WINDOWS_ID_BIAS) // @v11.0.3
#define DLG_VATBOOKTOTAL          442 // Итоги по книге продаж/покупок
	#define CTL_VATBOOKTOTAL_FRAME       (1 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_COUNT       (2 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_AMT         (3 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_VAT0        (10 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_VAT1        (11 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_VAT2        (12 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_VAT3        (13 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_VAT4        (14 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_VAT5        (15 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_SVAT1       (21 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_SVAT2       (22 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_SVAT3       (23 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_SVAT4       (24 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_SVAT5       (25 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_TVAT1       (31 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_TVAT2       (32 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_TVAT3       (33 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_TVAT4       (34 + WINDOWS_ID_BIAS)
	#define CTL_VATBOOKTOTAL_TVAT5       (35 + WINDOWS_ID_BIAS)
#define DLG_SELPRNCHK             443 // Выбор при печати кассового чека
	#define CTL_SELPRNCHK_WHAT           (1 + WINDOWS_ID_BIAS)
#define DLG_CREVAL                444 // Валютная переоценка
//	#define cmCRevalAddAcc               2001
//	#define cmCRevalDelAcc               2002
	#define cmCRevalUpdateCRate          2003
	#define CTL_CREVAL_ACCLIST           (1 + WINDOWS_ID_BIAS)
	#define CTL_CREVAL_CRATELIST         (2 + WINDOWS_ID_BIAS)
	#define CTL_CREVAL_DT                (3 + WINDOWS_ID_BIAS)
	#define CTL_CREVAL_CORRACC           (4 + WINDOWS_ID_BIAS)
	#define CTL_CREVAL_NEGCORRACC        (5 + WINDOWS_ID_BIAS)
	#define CTL_CREVAL_UPDCRBUTTON       (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_CREVAL_DT             (7 + WINDOWS_ID_BIAS)
	#define CTL_CREVAL_LOC               (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_CREVAL_LOC            (9 + WINDOWS_ID_BIAS)
#define DLG_GDSOPRTOTAL           445 // T_GDSOPR Итоги товарного отчета по операции
#define DLG_INOUTANLZTOTAL        562 // T_INOUTA Итоги анализа вход/выход
#define DLG_GDSOPRPLANTOTAL       926 // T_GOAPLN Итоги товарного отчета по операции с планом продаж
	#define CTL_GDSOPRTOTAL_COUNT        (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_QTTY         (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_COST         (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PRICE        (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INCOME       (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_AMTLIST      (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PHQTTY       (7 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INCOUNT      (8 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INQTTY       (9 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INCOST       (10 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INPRICE      (11 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INPHQTTY     (12 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_RSTCOST      (13 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_RSTPRICE     (14 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PLANQTTY     (15 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PLANSUM      (16 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PCT_QTTY     (17 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PCT_SUM      (18 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_DIFCOUNT     (19 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_DIFQTTY      (20 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_DIFPHQTT     (21 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_DIFCOST      (22 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_DIFPRICE     (23 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_COUNT2       (24 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_QTTY2        (25 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_COST2        (26 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PRICE2       (27 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_INCOME2      (28 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_PHQTTY2      (29 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_RSTCOST2     (30 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_RSTPR2       (31 + WINDOWS_ID_BIAS)
	#define CTL_GDSOPRTOTAL_BZSILIST     (32 + WINDOWS_ID_BIAS) // @v12.1.11 Список индикаторов

#define DLG_SAVEBALANCE           446 // Сохранение/восстановление баланса
	#define CTL_SAVEBALANCE_DATE         (1 + WINDOWS_ID_BIAS)
	#define CTL_SAVEBALANCE_FNAME        (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_SAVEBALANCE_DATE      (3 + WINDOWS_ID_BIAS) // @alena
	#define CTLBRW_SAVEBALANCE_FNAME     (4 + WINDOWS_ID_BIAS)
//                                447 // Occupied
#define DLG_OPGRPNGFLT            448 // Фильтр группировки операций
#define DLG_GOPANLZORD            449 // Изменение сортировки просмотра товарного отчета
	#define CTL_GOPANLZORD_ORDER         (1 + WINDOWS_ID_BIAS)
#define DLG_GOPANLZPRN            450 // Опции печати товарного отчета
	#define CTL_GOPANLZPRN_ORDER         (1 + WINDOWS_ID_BIAS)
	#define CTL_GOPANLZPRN_FLAGS         (2 + WINDOWS_ID_BIAS)
	#define CTL_GOPANLZPRN_SPC           (3 + WINDOWS_ID_BIAS)
#define DLG_OPCNTRVIEW            451 // Просмотр счетчиков операций
#define DLG_BILLTAX               452 // Налоги по документу
	#define cmBillTaxCalcVat1     2001
	#define cmBillTaxCalcVat2     2002
	#define cmBillTaxCalcSTax     2003
	#define CTL_BILLTAX_VAT1             (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_T_VAT1           (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_VAT2             (3 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_T_VAT2           (4 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_VAT3             (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_T_VAT3           (6 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_VAT4             (7 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_T_VAT4           (8 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_VAT5             (9 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_T_VAT5           (10 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_STAX             (11 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_T_STAX           (12 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_AMOUNT           (13 + WINDOWS_ID_BIAS)
	#define CTL_BILLTAX_BILLAMT          (14 + WINDOWS_ID_BIAS)
#define DLG_BARCODELIST           453 // Список штрихкодов по товару
	#define CTL_BARCODELIST_GOODS        (1 + WINDOWS_ID_BIAS)
	#define CTL_BARCODELIST_LIST         (2 + WINDOWS_ID_BIAS)
#define DLG_CSESSFILT             454 // Фильтр по кассовым сессиям
	#define cmCSessList                  101 // @unused
	#define CTL_CSESSFILT_PERIOD         (1 + WINDOWS_ID_BIAS)
	#define CTL_CSESSFILT_NODE           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSESSFILT_NODE        (3 + WINDOWS_ID_BIAS)
	#define CTL_CSESSFILT_CASHN          (4 + WINDOWS_ID_BIAS)
	#define CTL_CSESSFILT_FLAGS          (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_CSESSFILT_PERIOD      (6 + WINDOWS_ID_BIAS)
	#define CTL_CSESSFILT_ORDER          (7 + WINDOWS_ID_BIAS)
#define DLG_CSESSEXC              455 // Фильтр излишков по кассовым сессиям
	#define CTL_CSESSEXC_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXC_NODE            (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSESSEXC_NODE         (3 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXC_GGRP            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSESSEXC_GGRP         (5 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXC_GOODS           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSESSEXC_GOODS        (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_CSESSEXC_PERIOD       (8 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXC_FLAGS           (9 + WINDOWS_ID_BIAS)
#define DLG_CSESSEXCTOTAL         456 // Итоги излишков по кассовым сессиям
	#define CTL_CSESSEXCTOTAL_PERIOD     (1 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXCTOTAL_COUNT      (2 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXCTOTAL_QTTY       (3 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXCTOTAL_REST       (4 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXCTOTAL_ALTSUM     (5 + WINDOWS_ID_BIAS)
	#define CTL_CSESSEXCTOTAL_ALTDIF     (6 + WINDOWS_ID_BIAS)
#define DLG_CCHECKTOTAL           457 // Итоги по чекам
	#define CTL_CCHECKTOTAL_COUNT        (1 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_AMOUNT       (2 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_DISCOUNT     (3 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_QTTY         (4 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_MINSUM       (5 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_AVRGSUM      (6 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_MAXSUM       (7 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_ACASH        (8 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_ABANK        (9 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_ASCARD       (10 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKTOTAL_GUESTC       (11 + WINDOWS_ID_BIAS)
#define DLG_BALTOTAL              458 // Итоги по балансу
	#define CTL_BALTOTAL_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_INREST_D        (2 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_INREST_C        (3 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_TRNOVR_D        (4 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_TRNOVR_C        (5 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_OUTREST_D       (6 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_OUTREST_C       (7 + WINDOWS_ID_BIAS)
	#define CTL_BALTOTAL_DIFF            (8 + WINDOWS_ID_BIAS)
#define DLG_CSEXCAG               459 // Альтернативный товар для списания излишков по кассовой сессии
	#define CTL_CSEXCAG_ST_MAINGOODS     (1 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_GGRP             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSEXCAG_GGRP          (3 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_GOODS            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSEXCAG_GOODS         (5 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_FLAGS            (6 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_DFCTDT           (7 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_DFCTCOST         (8 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_DFCTPRICE        (9 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_ALGDT            (10 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_ALGCOST          (11 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_ALGPRICE         (12 + WINDOWS_ID_BIAS)
	#define CTL_CSEXCAG_SELLPRICE        (13 + WINDOWS_ID_BIAS)
#define DLG_CSESSTOTAL            460 // Итоги по кассовым сессиям
#define DLG_CSESSWRETTOTAL        883 // Итоги по кассовым сессиям с указанием суммы по чекам возврата
	#define CTL_CSESSTOTAL_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_COUNT         (2 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_AMOUNT        (3 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_WROFF         (4 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_EXC           (5 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_WROFFRCPT     (6 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_INCOME        (7 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_DISCOUNT      (8 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_BANKING       (9 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_AMTRET        (10 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_AMTWORET      (11 + WINDOWS_ID_BIAS)
	#define CTL_CSESSTOTAL_CHKCOUNT      (12 + WINDOWS_ID_BIAS)
#define DLG_RTOPLIST              461 // Список доступных операций
	#define CTL_RTOPLIST_LIST            (1 + WINDOWS_ID_BIAS)
#define DLG_RTOPLI                462 // Доступная операция //
	#define CTL_RTOPLI_OPRKIND           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_RTOPLI_OPRKIND        (2 + WINDOWS_ID_BIAS)
	#define CTL_RTOPLI_FLAGS             (3 + WINDOWS_ID_BIAS)
	#define CTL_RTOPLI_COMMRT            (4 + WINDOWS_ID_BIAS)
#define DLG_RTLOCLIST             463 // Список доступных складов
	#define CTL_RTLOCLIST_LIST           (1 + WINDOWS_ID_BIAS)
#define DLG_RTLOCLI               464 // Доступный склад
	#define CTL_RTLOCLI_LOC              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_RTLOCLI_LOC           (2 + WINDOWS_ID_BIAS)
	#define CTL_RTLOCLI_FLAGS            (3 + WINDOWS_ID_BIAS)
#define DLG_CSESSDEL              465 // Опции удаления кассовой сессии
	#define CTL_CSESSDEL_OPTIONS         (1 + WINDOWS_ID_BIAS)
#define DLG_ACCAGGR               466 // Группирующий бухгалтерский счет
	#define CTL_ACCAGGR_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGR_FLAGS            (2 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGR_LIST             (3 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGR_ACCSHEET         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCAGGR_ACCSHEET      (5 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGR_NUMBER           (6 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGR_CODE             (7 + WINDOWS_ID_BIAS)
#define DLG_ACCAGGRI              467 // Элемент группирующего счета
	#define CTL_ACCAGGRI_ACC             (1 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGRI_ACCNAME         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCAGGRI_ACCNAME      (3 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGRI_ART             (4 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGRI_ARTNAME         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ACCAGGRI_ARTNAME      (6 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGRI_FLAGS           (7 + WINDOWS_ID_BIAS)
	#define CTL_ACCAGGRI_ACCGRP          (8 + WINDOWS_ID_BIAS)
#define DLG_SELACTYP              468 // Выбор типа бухгалтерского счета
	#define CTL_SELACTYP_TYP             (1 + WINDOWS_ID_BIAS)
#define DLG_SELCSESS              469 // Выбор действия либо для одной кассовой сессии, либо для всей выборки
#define DLG_SELCSESSEXC           560 // Выбор действия либо для одной кассовой сессии, либо для всей выборки
	#define CTL_SELCSESS_SEL             (1 + WINDOWS_ID_BIAS)
	#define CTL_SELCSESS_FLAGS           (2 + WINDOWS_ID_BIAS)
#define DLG_SELPRNAT              470 // Выбор печати оборотной ведомости по статьям счета
	#define CTL_SELPRNAT_SEL             (1 + WINDOWS_ID_BIAS)
	#define CTL_SELPRNAT_EXPIRY          (2 + WINDOWS_ID_BIAS)
	#define CTL_SELPRNAT_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELPRNAT_EXPIRY       (4 + WINDOWS_ID_BIAS)
#define DLG_INVLINETOTAL          471 // Итог по инвентарным строкам
	#define CTL_INVLINETOTAL_COUNT       (1 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_SQTTY       (2 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_IQTTY       (3 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_DQTTY       (4 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_SAMT        (5 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_IAMT        (6 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_DAMT        (7 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_SURPLUS     (8 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_LACK        (9 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_DFCTQTY     (10 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_DFCTAMT     (11 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_BILLC       (12 + WINDOWS_ID_BIAS)
	#define CTL_INVLINETOTAL_GOODSC      (13 + WINDOWS_ID_BIAS)
#define DLG_SYNCFILT              472 // Фильтр по таблице синхронизации
	#define CTL_SYNCFILT_OBJ             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYNCFILT_OBJ          (2 + WINDOWS_ID_BIAS)
	#define CTL_SYNCFILT_DIV             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYNCFILT_DIV          (4 + WINDOWS_ID_BIAS)
	#define CTL_SYNCFILT_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTL_SYNCFILT_OBJIDS          (6 + WINDOWS_ID_BIAS)
	#define CTL_SYNCFILT_COMMIDS         (7 + WINDOWS_ID_BIAS)
#define DLG_MODTRANSM             473 // Фильтр передачи измененных объектов
// использует те же элементы, что и DLG_OBJTRANSM
#define DLG_SCALESYS              474 //
	#define CTL_SCALESYS_GETTRIES        (1 + WINDOWS_ID_BIAS)
	#define CTL_SCALESYS_GETDELAY        (2 + WINDOWS_ID_BIAS)
	#define CTL_SCALESYS_PUTTRIES        (3 + WINDOWS_ID_BIAS)
	#define CTL_SCALESYS_PUTDELAY        (4 + WINDOWS_ID_BIAS)
#define DLG_CVTAT2AB              475 // Параметры конвертации бух проводок в документы
	#define CTL_CVTAT2AB_OP              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTAT2AB_OP           (2 + WINDOWS_ID_BIAS)
	#define CTL_CVTAT2AB_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTL_CVTAT2AB_EXTOBJ          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTAT2AB_EXTOBJ       (5 + WINDOWS_ID_BIAS)
	#define CTL_CVTAT2AB_LOC             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTAT2AB_LOC          (7 + WINDOWS_ID_BIAS)
	#define CTL_CVTAT2AB_OBJ             (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTAT2AB_OBJ          (9 + WINDOWS_ID_BIAS)
#define DLG_TAGRPNG               476 // Опции группировки анализа товарных операций
	#define cmEditCtParam               101 // @>>DLG_TAC
	#define CTL_TAGRPNG_FRAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_GOODS            (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGRPNG_GOODS         (3 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_CNTRAGENT        (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGRPNG_CNTRAGENT     (5 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_DATE             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGRPNG_DATE          (7 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_SUBSTRADDR       (8 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_DIFFDLVRADDR     (9 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_GGRPNG           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGRPNG_GGRPNG        (11 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_CALCREST         (12 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_TURNOVER         (13 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_EXTVAL1          (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGRPNG_EXTVAL1       (15 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_EXTF1            (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_TAGRPNG_EXTF1         (17 + WINDOWS_ID_BIAS)
	#define CTL_TAGRPNG_EXTVALFLAGS      (18 + WINDOWS_ID_BIAS) // @v11.9.4 Флаги дополнительных показателей
#define DLG_FIXSTAFF              477 // Директор и гл бух организации
	#define CTL_FIXSTAFF_ORGNAME         (1 + WINDOWS_ID_BIAS)
	#define CTL_FIXSTAFF_DIRECTOR        (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_FIXSTAFF_DIRECTOR     (3 + WINDOWS_ID_BIAS)
	#define CTL_FIXSTAFF_ACCTNT          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_FIXSTAFF_ACCTNT       (5 + WINDOWS_ID_BIAS)
#define DLG_RCPTRETBILL           478 // Документ возврата поставщику (CTL_BILL_XXX)
#define DLG_SELLRETBILL           479 // Документ возврата от покупателя (CTL_BILL_XXX)

#define DLG_GDSCLS                480 // Отраслевой класс товара
	#define cmGdsClsKind                 101
	#define cmGdsClsAdd                  102
	#define cmGdsClsGrade                103
	#define cmGdsClsX                    104
	#define cmGdsClsY                    105
	#define cmGdsClsZ                    106
	#define cmGdsClsAdd2                 107
	#define cmGdsClsW                    108
	#define cmGdsClsFormList             120
	#define cmGdsClsDynGenMask           121
	#define cmGdsClsLotDim               122
	#define CTL_GDSCLS_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_ID                (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_NAMECONV          (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_ABBRCONV          (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_KIND            (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_ADD             (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_GRADE           (7 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_X               (8 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_Y               (9 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_Z               (10 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_UNIT              (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLS_UNIT           (12 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_PHUNIT            (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLS_PHUNIT         (14 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_PHUPERU           (15 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_GDSTYPE           (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLS_GDSTYPE        (17 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_TAXGRP            (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLS_TAXGRP         (19 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_FLAGS             (20 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_TAXFACTOR         (21 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_GRP               (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLS_GRP            (23 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_PACKAGE           (24 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_ADD2            (25 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_E_W               (26 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_SYMB              (27 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLS_BRUTTO            (28 + WINDOWS_ID_BIAS) // @v12.0.10 Формула для расчета брутто

#define DLG_GDSCLSPROP            481 // Свойство класса товаров
#define DLG_GDSCLSPROP2           697
	#define cmGcPropViewObj           101
	#define cmGcPropCreateObj         102
	#define cmGcPropReplaceObj        103
	#define CTL_GDSCLSPROP_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSPROP_OBJ           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLSPROP_OBJ        (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSPROP_LIST          (4 + WINDOWS_ID_BIAS)
#define DLG_GDSCLSDIM             482 // Размерность класса товаров
	#define CTL_GDSCLSDIM_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSDIM_SCALE          (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSDIM_VALS           (3 + WINDOWS_ID_BIAS)
#define DLG_SG                    483 // Общий диалог доп характеристик товара
	#define cmSgDimX                     101
	#define cmSgDimY                     102
	#define cmSgDimZ                     103
	#define cmFullGoodsDialog            104
	#define cmSgDimW                     105
	#define CTL_SG_KIND                  (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SG_KIND               (2 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_KIND                (3 + WINDOWS_ID_BIAS)
	#define CTL_SG_ADDPROP               (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SG_ADDPROP            (5 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_ADDPROP             (6 + WINDOWS_ID_BIAS)
	#define CTL_SG_GRADE                 (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SG_GRADE              (8 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_GRADE               (9 + WINDOWS_ID_BIAS)
	#define CTL_SG_DIMX                  (10 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_DIMX                (11 + WINDOWS_ID_BIAS)
	#define CTL_SG_DIMY                  (12 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_DIMY                (13 + WINDOWS_ID_BIAS)
	#define CTL_SG_DIMZ                  (14 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_DIMZ                (15 + WINDOWS_ID_BIAS)
	#define CTL_SG_VOLUME                (16 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_VOLUME              (17 + WINDOWS_ID_BIAS)
	#define CTL_SG_FULLDLGBUTTON         (18 + WINDOWS_ID_BIAS) // Button cmFullGoodsDialog
	#define CTL_SG_ADD2PROP              (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_SG_ADD2PROP           (20 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_ADD2PROP            (21 + WINDOWS_ID_BIAS)
	#define CTL_SG_DIMW                  (22 + WINDOWS_ID_BIAS)
	#define CTL_SG_L_DIMW                (23 + WINDOWS_ID_BIAS)
	#define CTL_SG_CLS                   (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_SG_CLS                (25 + WINDOWS_ID_BIAS)
	#define CTL_SG_ZEROX                 (26 + WINDOWS_ID_BIAS)
	#define CTL_SG_ZEROY                 (27 + WINDOWS_ID_BIAS)
	#define CTL_SG_ZEROZ                 (28 + WINDOWS_ID_BIAS)
	#define CTL_SG_ZEROW                 (29 + WINDOWS_ID_BIAS)

#define DLG_MSGNCMPL3             484 // CTL_MSGNCMPL_XXX
#define DLG_GOODSS                485 // @obsolete @v4.6.4 Сокращенная форма диалога товара
#define DLG_PCKG                  486 // Package
	#define cmPckgMakeCode               101
	#define CTL_PCKG_CODE                (1 + WINDOWS_ID_BIAS)
	#define CTL_PCKG_ID                  (2 + WINDOWS_ID_BIAS)
	#define CTL_PCKG_TYPE                (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PCKG_TYPE             (4 + WINDOWS_ID_BIAS)
	#define CTL_PCKG_COST                (5 + WINDOWS_ID_BIAS)
	#define CTL_PCKG_PRICE               (6 + WINDOWS_ID_BIAS)
#define DLG_PCKGTYPE              487 // Package Type
	#define CTL_PCKGTYPE_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_PCKGTYPE_TEMPL           (2 + WINDOWS_ID_BIAS)
	#define CTL_PCKGTYPE_COUNTER         (3 + WINDOWS_ID_BIAS)
	#define CTL_PCKGTYPE_GRP             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PCKGTYPE_GRP          (5 + WINDOWS_ID_BIAS)
	#define CTL_PCKGTYPE_FLAGS           (6 + WINDOWS_ID_BIAS)
	#define CTL_PCKGTYPE_ROWINP          (7 + WINDOWS_ID_BIAS)
#define DLG_TRSEL                 488 // Выбор типа транспортного средства
	#define CTL_TRSEL_WHAT               (1 + WINDOWS_ID_BIAS)
#define DLG_TR_CAR                489 // Автомобиль
#define DLG_TR_SHIP               490 // Торговое судно
	#define CTL_TRANSPORT_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_OWNER          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPORT_OWNER       (3 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_CNTRY          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPORT_CNTRY       (5 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_CAPTAIN        (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPORT_CAPTAIN     (7 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_CODE           (8 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_TRAILCODE      (9 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_MODEL          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPORT_MODEL       (11 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_CAPACITY       (12 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_VANTYP         (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPORT_VANTYP      (14 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPORT_FLAGS          (15 + WINDOWS_ID_BIAS)
#define DLG_FREIGHT               491 // Фрахт
	#define cmFreightEditDlvrLocList     101
	#define CTL_FREIGHT_TRTYP            (1 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_NAME             (2 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_SHIP             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_SHIP          (4 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_CAPTAIN          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_CAPTAIN       (6 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_AGENT            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_AGENT         (8 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_ISSLOC           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_ISSLOC        (10 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_ISSDT            (11 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_ARRIVLOC         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_ARRIVLOC      (13 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_ARRIVDT          (14 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_NMBORIGSBSL      (15 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_DLVRLOC          (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_DLVRLOC       (17 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_SHIPPED          (18 + WINDOWS_ID_BIAS)
	#define CTLCAL_FREIGHT_ISSDT         (19 + WINDOWS_ID_BIAS) // @alena
	#define CTLCAL_FREIGHT_ARRIVDT       (20 + WINDOWS_ID_BIAS) // @alena
	#define CTL_FREIGHT_COST             (21 + WINDOWS_ID_BIAS)
	#define CTL_FREIGHT_CAPTAIN2         (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_FREIGHT_CAPTAIN2      (23 + WINDOWS_ID_BIAS)
#define DLG_SELBBSMPL             492 // Выбор способа создания документа по образцу
	#define CTL_SELBBSMPL_WHAT           (1 + WINDOWS_ID_BIAS)
	#define CTL_SELBBSMPL_OP             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELBBSMPL_OP          (3 + WINDOWS_ID_BIAS)
	#define CTL_SELBBSMPL_LOC            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELBBSMPL_LOC         (5 + WINDOWS_ID_BIAS)
	#define CTL_SELBBSMPL_FLAGS          (6 + WINDOWS_ID_BIAS)
	#define CTL_SELBBSMPL_SAMECODE       (6 + WINDOWS_ID_BIAS) // @obsolete synonym of CTL_SELBBSMPL_FLAGS
	#define CTL_SELBBSMPL_QK             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELBBSMPL_QK          (8 + WINDOWS_ID_BIAS)
	#define CTL_SELBBSMPL_DT             (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELBBSMPL_DT          (10 + WINDOWS_ID_BIAS)
	#define CTL_SELBBSMPL_INFO           (11 + WINDOWS_ID_BIAS)
#define DLG_SELPCKG               493 // Выбор пакета
	#define CTL_SELPCKG_LIST             (1 + WINDOWS_ID_BIAS)
#define DLG_GOODSOPR2             494 // Расширение фильтра товарного отчета по операции
#define DLG_GDSTAXLST             495 // Список налоговых параметров по периодам и операциям
	#define CTL_GDSTAXLST_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAXLST_ID             (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSTAXLST_LIST           (3 + WINDOWS_ID_BIAS)
#define DLG_GDSTAXENTRY           496 // Элемент списка налоговой группы
// controls CTL(SEL)_GDSTAX_XXX
#define DLG_IMPGOODS              497 // Параметры импорта товаров
	#define CTL_IMPGOODS_GRP             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPGOODS_GRP          (2 + WINDOWS_ID_BIAS)
	#define CTL_IMPGOODS_OP              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPGOODS_OP           (4 + WINDOWS_ID_BIAS)
	#define CTL_IMPGOODS_SUPPL           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPGOODS_SUPPL        (6 + WINDOWS_ID_BIAS)
	#define CTL_IMPGOODS_UNIT            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPGOODS_UNIT         (8 + WINDOWS_ID_BIAS)
	#define CTL_IMPGOODS_FLAGS           (9 + WINDOWS_ID_BIAS)
	#define CTL_IMPGOODS_PHUNIT          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPGOODS_PHUNIT       (11 + WINDOWS_ID_BIAS)
#define DLG_SELQCERT              498 // Диалог выбора сертификата
	#define cmSelQCertByGoods            2001
	#define CTL_SELQCERT_SRCH            (1 + WINDOWS_ID_BIAS)
	#define CTL_SELQCERT_GOODSGRP        (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELQCERT_GOODSGRP     (3 + WINDOWS_ID_BIAS)
	#define CTL_SELQCERT_GOODS           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELQCERT_GOODS        (5 + WINDOWS_ID_BIAS)
	#define CTL_SELQCERT_LIST            (6 + WINDOWS_ID_BIAS)
#define DLG_BILLFLT_ORDER         499 // Фильтр по заказам (same as DLG_BILLFLT)
#define DLG_GBSTRUC		          500 //Корзина товаров
#define DLG_GBSTRUC_N            1038
	#define cmClearGB                 102 // Кнопка очистки корзины
	#define cmAddFromBasket           106 // Добавить из корзины
	#define CTL_GBTRUC_GOODS             (1 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_LIST              (2 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_FLAGS             (3 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_TPRICE            (4 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_GNAME             (5 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_BASKET            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBTRUC_BASKET         (7 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_TOTAL             (8 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_SUPPL             (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBTRUC_SUPPL          (10 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_LINES             (11 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_BRUTTO            (12 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_VOLUME            (13 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_ID                (14 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_PRIVATE           (15 + WINDOWS_ID_BIAS)
	#define CTL_GBTRUC_USER              (16 + WINDOWS_ID_BIAS) // @v11.4.1 read-only user-name
#define DLG_GBITEM                501   // Элемент корзины товаров
	#define cmShowBasket                 104
	#define cmPrognosis                  105
	#define CTL_GBITEM_HEADER            (1 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_GGRP              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBITEM_GGRP           (3 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_GOODS             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBITEM_GOODS          (5 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_VALUE             (6 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_PRICE             (7 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_LINESCOUNT        (8 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_AMOUNT            (9 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_BASKET            (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBITEM_BASKET         (11 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_UPPACK            (12 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_EXPIRY            (13 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_QTTYPACK          (14 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_SUPPL             (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBITEM_SUPPL          (16 + WINDOWS_ID_BIAS)
	#define CTLCAL_GBITEM_EXPIRY         (17 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_BRUTTO            (18 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_PALETTE           (19 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_VOLUME            (20 + WINDOWS_ID_BIAS)
	#define CTL_GBITEM_PRIVATE           (21 + WINDOWS_ID_BIAS)
#define DLG_GBDATA                502 // Выбор структуры товара
	#define CTL_GBDATA_NAMEDSTRUC        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GBDATA_NAMEDSTRUC     (2 + WINDOWS_ID_BIAS)
	#define CTL_GBDATA_SELPRICE          (3 + WINDOWS_ID_BIAS)
	#define CTL_GBDATA_REPLACE           (4 + WINDOWS_ID_BIAS)
	#define CTL_GBDATA_SELGOODSREST      (5 + WINDOWS_ID_BIAS)
#define DLG_SELOBSMPL             504 // Выбор способа создания документа по образцу заказа
	// Ctrls same as for DLG_SELBBSMPL
#define DLG_PLISTCFG              505 // Конфигурация прайс-листа
	#define CTL_PLISTCFG_P1Q             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLISTCFG_P1Q          (2 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_P2Q             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLISTCFG_P2Q          (4 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_P3Q             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLISTCFG_P3Q          (6 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_GGRP            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLISTCFG_GGRP         (8 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_EXCLGGRP        (9 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_FLAGS          (10 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_ORGNAME        (11 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_BUSINESS       (12 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_PUBMAIL        (13 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_EXTFLD         (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLISTCFG_EXTFLD      (15 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_EXPORTSPEC     (16 + WINDOWS_ID_BIAS)
	#define CTL_PLISTCFG_LOC			   (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_PLISTCFG_LOC		   (18 + WINDOWS_ID_BIAS)
#define DLG_FLTLOTEXT             506 // Расширение фильтра по лотам
#define DLG_CRITWARN              507 // Критическое предупреждение
	#define CTL_CRITWARN_HEAD            (1 + WINDOWS_ID_BIAS)
	#define CTL_CRITWARN_ANSWER          (2 + WINDOWS_ID_BIAS)
#define DLG_DBCHAIN               508 // Стяжка базы данных
	// Used as PasswordDialog(DLG_DBCHAIN, ...)
#define DLG_DBUNCHAIN             509 // Снятие стяжки базы данных
	#define CTL_DBUNCHAIN_PASSWORD       (1 + WINDOWS_ID_BIAS)
#define DLG_GOODSVAD              510 // @v3.5.0 Дополнительные данные по товару
	#define cmGoodsPalletList            101
	#define CTL_GOODSVAD_STORAGE         (1 + WINDOWS_ID_BIAS)  // field A
	#define CTL_GOODSVAD_STANDARD        (2 + WINDOWS_ID_BIAS)  // field B
	#define CTL_GOODSVAD_LABELNAME       (3 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_MINSTOCKLI      (4 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_BRUTTO          (5 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_SZLN            (6 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_SZWD            (7 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_SZHT            (8 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_INGRED          (9 + WINDOWS_ID_BIAS)  // field C
	#define CTL_GOODSVAD_ENERGY          (10 + WINDOWS_ID_BIAS) // field D
	#define CTL_GOODSVAD_USAGE           (11 + WINDOWS_ID_BIAS) // field E
	#define CTL_GOODSVAD_TITLE_A         (12 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_TITLE_B         (13 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_TITLE_C         (14 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_TITLE_D         (15 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_TITLE_E         (16 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_PACKAGE         (17 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_VOLUME          (18 + WINDOWS_ID_BIAS)  //
	#define CTL_GOODSVAD_VOLUMEVAL       (19 + WINDOWS_ID_BIAS)  // Флаг "Указывать объем непосредственно"
	#define CTL_GOODSVAD_EXPIRYPRD       (20 + WINDOWS_ID_BIAS)  // Срок годности товара (дней)
	#define CTL_GOODSVAD_PLTTYPE         (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSVAD_PLTTYPE      (22 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_PLTLAYER        (23 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_PLTLAYERC       (24 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_MINSHQTTY       (25 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_MULTMINSH       (26 + WINDOWS_ID_BIAS)
	#define CTL_GOODSVAD_NTBRTCOEF       (27 + WINDOWS_ID_BIAS)

#define DLG_SELQUOT               511 // Выбор котировки в диалоге строки товарного документа
	#define CTL_SELQUOT_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_SELQUOT_FLAGS            (2 + WINDOWS_ID_BIAS)
#define DLG_OBJSYNC               512 // Редактирование записи ObjSync
	#define CTL_OBJSYNC_OBJ              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJSYNC_OBJ           (2 + WINDOWS_ID_BIAS)
	#define CTL_OBJSYNC_ID               (3 + WINDOWS_ID_BIAS)
	#define CTL_OBJSYNC_COMMID           (4 + WINDOWS_ID_BIAS)
	#define CTL_OBJSYNC_DIV              (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJSYNC_DIV           (6 + WINDOWS_ID_BIAS)
	#define CTL_OBJSYNC_DT               (7 + WINDOWS_ID_BIAS)
	#define CTL_OBJSYNC_TM               (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_OBJSYNC_DT            (9 + WINDOWS_ID_BIAS) // @alena
#define DLG_BILLCFG               513 // Конфигурация документов
	#define cmBillCfgTDisMethod          101
	#define cmBillCfgValuation           102
	#define cmBillCfgExt                 103
	#define cmLotTagIndFilt              104
	#define CTL_BILLCFG_OPPRFX           (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_CLPRFX           (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_CLCODEREG        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLCFG_CLCODEREG     (4 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_FLAGS            (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_CASHNODE         (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLCFG_CASHNODE      (7 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_SNTEMPLT         (8 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_INVSNTEMPLT      (9 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_SNRCNTR          (10 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_SHOWADDFLD       (11 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_INITST           (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLCFG_INITST        (13 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_DEFICITSUBST     (14 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_FILESFOLDER      (15 + WINDOWS_ID_BIAS)
	#define CTLBRW_BILLCFG_FILESFOLD     (16 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_FLAGS2           (17 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_UNIQSNSFX        (18 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_CREGTYP          (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLCFG_CREGTYP       (20 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_MNFLTAG          (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLCFG_MNFLTAG       (22 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_LOWDEBTDATE      (23 + WINDOWS_ID_BIAS)
	#define CTLCAL_BILLCFG_LOWDEBTDATE   (24 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_WLED             (25 + WINDOWS_ID_BIAS)
	#define CTL_BILLCFG_WLEF             (26 + WINDOWS_ID_BIAS)
//                                514 // Occupied
#define DLG_GFLTADVOPT            515 // GFADVOPT Дополнительные опции фильтра по товарам
	#define CTL_GFLTADVOPT_LOTPERIOD     (1 + WINDOWS_ID_BIAS)
	#define CTL_GFLTADVOPT_SUPPL         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFLTADVOPT_SUPPL      (3 + WINDOWS_ID_BIAS)
	#define CTL_GFLTADVOPT_PLISTFNEW     (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFLTADVOPT_LOC        (5 + WINDOWS_ID_BIAS)
	#define CTL_GFLTADVOPT_LOC           (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_GFLTADVOPT_LOTPERIOD  (7 + WINDOWS_ID_BIAS)
	#define CTL_GFLTADVOPT_EXTFLT        (8 + WINDOWS_ID_BIAS)
	#define CTL_GFLTADVOPT_PLUSNREST     (9 + WINDOWS_ID_BIAS) // Товар не попадает в период поступления, но есть на остатке
	#define CTL_GFLTADVOPT_FLAGS         (10 + WINDOWS_ID_BIAS)
	#define CTL_GFLTADVOPT_SNEXP         (11 + WINDOWS_ID_BIAS) // @v11.6.2 f2SoonExpiredOnly
	#define CTL_GFLTADVOPT_AHEXPD        (12 + WINDOWS_ID_BIAS) // @v11.6.2 AheadExpiryDays
	#define CTL_GFLTADVOPT_IDRNG         (13 + WINDOWS_ID_BIAS) // @v11.8.3 IdRange
	#define CTL_GFLTADVOPT_GRPCRNG       (14 + WINDOWS_ID_BIAS) // @v11.8.3 GrpCountRange
//                                516 // Occupied (DLG_BILLEXTFLT)
#define DLG_OPRDRAFT              517 // Вид драфт-операции
	// CTL[SEL]_OPRKIND_XXX

#define DLG_PRNBLSTI              518 // Форма печати реестра межскладских документов
	// Управляющие элементы те же, что и в DLG_PRNBLSTI
#define DLG_RENTCHARGE            519
	#define CTL_RENTCHARGE_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTL_RENTCHARGE_OBJECT        (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_RENTCHARGE_OBJECT     (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_RENTCHARGE_PERIOD     (4 + WINDOWS_ID_BIAS)
#define DLG_MAINORG2              520
	#define CTL_MAINORG2_STATUS          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG2_STATUS       (2 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_NAME            (3 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_EXTNAME         (4 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_INN             (5 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_PHONE           (6 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_COUNTRY         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG2_COUNTRY      (8 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_CITY            (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG2_CITY         (10 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_ADDRESS         (11 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_BANK            (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG2_BANK         (13 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_CORRACC         (14 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_BIC             (15 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_ACC             (16 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_MEMO            (17 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_ACCTNT          (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG2_ACCTNT       (19 + WINDOWS_ID_BIAS)
	#define CTL_MAINORG2_DIRECTOR        (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAINORG2_DIRECTOR     (21 + WINDOWS_ID_BIAS)
#define DLG_MSGNCMPL4             521 // Lack for draft bill writing off
	// CTL_MSGNCMPL_XXX
//                                522 // Occupied (DLG_BILLEXTFLT2)
#define DLG_PRNPRP                523 // Выбор печати тетради частного предпринимателя 2001
	#define CTL_PRNPRP_WHAT              (1 + WINDOWS_ID_BIAS)
#define DLG_SELWROFF              524 // @v3.7.0 Списание драфт-документов
	#define CTL_SELWROFF_SEL             (1 + WINDOWS_ID_BIAS)
	#define CTL_SELWROFF_SELDATE         (2 + WINDOWS_ID_BIAS)
	#define CTL_SELWROFF_DATE            (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELWROFF_DATE         (4 + WINDOWS_ID_BIAS)
#define DLG_BHT                   525 // BHT-терминал
	#define cmBhtGoodsFilt               601
	#define CTL_BHT_NAME                 (1 + WINDOWS_ID_BIAS)
	#define CTL_BHT_ID                   (2 + WINDOWS_ID_BIAS)
	#define CTL_BHT_COMPORT              (3 + WINDOWS_ID_BIAS)
	#define CTL_BHT_BAUDRATE             (4 + WINDOWS_ID_BIAS)
	#define CTL_BHT_TIMEOUT              (5 + WINDOWS_ID_BIAS)
	#define CTL_BHT_MAXTRIES             (6 + WINDOWS_ID_BIAS)
	#define CTL_BHT_INVENTOP             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHT_INVENTOP          (8 + WINDOWS_ID_BIAS)
	#define CTL_BHT_RCPTPLACE            (9 + WINDOWS_ID_BIAS)
	#define CTL_BHT_COMREADDELAY         (10 + WINDOWS_ID_BIAS)
	#define CTL_BHT_TYPE                 (11 + WINDOWS_ID_BIAS)
	#define CTL_BHT_EXPENDOP             (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHT_EXPENDOP          (13 + WINDOWS_ID_BIAS)
	#define CTL_BHT_IMPEXPPATH           (14 + WINDOWS_ID_BIAS)
	#define CTL_BHT_INTROP               (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHT_INTROP            (16 + WINDOWS_ID_BIAS)
	#define CTL_BHT_FLAGS                (17 + WINDOWS_ID_BIAS)
	#define CTLBRW_BHT_IMPEXPPATH        (18 + WINDOWS_ID_BIAS)
	#define CTL_BHT_LOC                  (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHT_LOC               (20 + WINDOWS_ID_BIAS)
#define DLG_BHTSEND               526
	#define CTL_BHTSEND_BHT              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHTSEND_BHT           (2 + WINDOWS_ID_BIAS)
	#define CTL_BHTSEND_WHAT             (3 + WINDOWS_ID_BIAS)
	#define CTL_BHTSEND_FLAGS            (4 + WINDOWS_ID_BIAS)
	#define CTL_BHTSEND_FILENAME         (5 + WINDOWS_ID_BIAS) // used for DLG_BHTPROGUPLOAD
	#define CTLBRW_BHTSEND_FILENAME      (6 + WINDOWS_ID_BIAS) // used for DLG_BHTPROGUPLOAD
#define DLG_BHTRCV                527
	#define CTL_BHTRCV_BHT               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHTRCV_BHT            (2 + WINDOWS_ID_BIAS)
	#define CTL_BHTRCV_DINTRLOC          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BHTRCV_DINTRLOC       (4 + WINDOWS_ID_BIAS)
#define DLG_BILLFLT_DRAFT         528 // Фильтр по драфт-документам
	// CTL_BILLFLT_XXX
#define DLG_SCARDSER              529 // Серия персональных карт
	#define cmSCardRule                  101
	#define cmCCheckRule                 102
	#define cmBonusRule                  103
	#define cmRestrictions               104 // @v12.5.1
	#define CTL_SCARDSER_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_ID              (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_CODETEMPL       (3 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_DATE            (4 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_EXPIRY          (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_PDIS            (6 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_MAXCRED         (7 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_TYPE            (8 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_QUOTKIND        (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_QUOTKIND     (10 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_PSNKIND         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_PSNKIND      (12 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDSER_DATE         (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDSER_EXPIRY       (14 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_CRDGGRP         (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_CRDGGRP      (17 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_FLAGS           (18 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_BONCGRP         (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_BONCGRP      (20 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_SYMB            (21 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_BONER           (23 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_BONERVAL        (24 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_USAGETM         (25 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_PARENT          (26 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_PARENT       (27 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_SPCTRT          (28 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_SPCTRT       (29 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_RPDEST          (30 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDSER_RPDEST       (31 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_FIXBON          (32 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSER_RESTRICTIONS    (33 + WINDOWS_ID_BIAS) // @v12.5.1 cmRestrictions
#define DLG_SCARD                 530 // Персональная карта
	#define cmNotifyOptions              101
	#define cmSCardAction1               102
	#define cmAutoCreateOwner            103 // @v11.6.3
	#define CTL_SCARD_CODE               (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_ID                 (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_SERIES             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARD_SERIES          (4 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PERSON             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARD_PERSON          (6 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_DATE               (7 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_EXPIRY             (8 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PDIS               (9 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_MAXCRED            (10 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_FLAGS              (11 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PSW                (12 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARD_DATE            (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARD_EXPIRY          (14 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_USAGETM            (15 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_AUTOGOODS          (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARD_AUTOGOODS       (17 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_TYPE               (18 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_ST_INFO            (19 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PRD                (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARD_PRD             (21 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PRDCOUNT           (22 + WINDOWS_ID_BIAS)
	#define CTLSPN_SCARD_PRDCOUNT        (23 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_MEMO               (24 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_ADDRINFO           (25 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PHONEINPUT         (26 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_PHONE              (27 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_ACN1               (28 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_POOLDSER           (29 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARD_POOLDSER        (30 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_FIXBON             (31 + WINDOWS_ID_BIAS)
	#define CTL_SCARD_OUTERID            (32 + WINDOWS_ID_BIAS)
	#define CTLBUT_SCARD_AUTOCROWNER     (33 + WINDOWS_ID_BIAS) // @v11.6.3 button cmAutoCreateOwner
	#define CTL_SCARD_DOW                (34 + WINDOWS_ID_BIAS) // @v12.5.1 Дни недели, когда карта действительна
#define DLG_SCARDFLT              531 // F_SCARD Фильтр по пластиковым карточкам
	#define cmExcludeOwnerScFilt         101
	#define CTL_SCARDFLT_SERIES          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDFLT_SERIES       (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_PERSON          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDFLT_PERSON       (4 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_EMPLOYER        (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDFLT_EMPLOYER     (7 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_TRNOVRPRD       (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDFLT_TRNOVRPRD    (9 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_PDISRANGE       (10 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_TRNOVRRANGE     (11 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_SINCE           (12 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_ORD             (13 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_NUMBER          (14 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_ISSUE           (15 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDFLT_ISSUE        (16 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_EXPIRY          (17 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDFLT_EXPIRY       (18 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFLT_WOOWNER         (19 + WINDOWS_ID_BIAS)

#define DLG_SCSERIESVIEW          532 // Список серий пластиковых карт
#define DLG_ARTICLEACC            533 // Аналитическая статья, ассоциированная со счетом
	// CTL_ARTICLE_XXX
#define DLG_PALM                  534 // PDA Palm
#define DLG_PALMGROUP             657 // @v4.6.8 Группа PDA Palm (CTL_PALM_XXX)
	// @v8.6.8 #define cmPalmLocList                2001 // @todo @v4.6.12 replace with cmLocList
	#define cmGeoTracking                2002 // @v8.6.8
	#define CTL_PALM_NAME                (1 + WINDOWS_ID_BIAS)
	#define CTL_PALM_ID                  (2 + WINDOWS_ID_BIAS)
	#define CTL_PALM_LOC                 (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_LOC              (4 + WINDOWS_ID_BIAS)
	#define CTL_PALM_OP                  (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_OP               (6 + WINDOWS_ID_BIAS)
	#define CTL_PALM_PATH                (7 + WINDOWS_ID_BIAS)
	#define CTL_PALM_AGENT               (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_AGENT            (9 + WINDOWS_ID_BIAS)
	#define CTL_PALM_FLAGS               (10 + WINDOWS_ID_BIAS)
	#define CTL_PALM_GROUP               (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_GROUP            (12 + WINDOWS_ID_BIAS)
	#define CTL_PALM_FTPACCT             (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_FTPACCT          (14 + WINDOWS_ID_BIAS)
	#define CTL_PALM_FTPPATH             (15 + WINDOWS_ID_BIAS)
	#define CTL_PALM_GGRP                (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_GGRP             (17 + WINDOWS_ID_BIAS)
	#define CTLBRW_PALM_PATH             (18 + WINDOWS_ID_BIAS)
	#define CTL_PALM_SYMB                (19 + WINDOWS_ID_BIAS)
	#define CTL_PALM_MAXNOTSENTORD       (20 + WINDOWS_ID_BIAS)
	#define CTL_PALM_DVCTYPE             (21 + WINDOWS_ID_BIAS)
	#define CTL_PALM_REGINFO             (22 + WINDOWS_ID_BIAS)
	#define CTL_PALM_REGISTERED          (23 + WINDOWS_ID_BIAS)
	#define CTL_PALM_MAXSENTDAYS         (24 + WINDOWS_ID_BIAS)
	#define CTL_PALM_INHBTAGVAL          (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_INHBTAGVAL       (26 + WINDOWS_ID_BIAS)
	#define CTL_PALM_QUOTKIND            (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALM_QUOTKIND         (28 + WINDOWS_ID_BIAS)
	#define CTL_PALM_QUOTKINDOPTIONS     (29 + WINDOWS_ID_BIAS) // @v11.7.1
#define DLG_PALMPANE              535 // PDA Import/Export Panel
	#define CTL_PALMPANE_PALM            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMPANE_PALM         (2 + WINDOWS_ID_BIAS)
	#define CTL_PALMPANE_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTL_PALMPANE_OP              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMPANE_OP           (5 + WINDOWS_ID_BIAS)
	#define CTL_PALMPANE_LOC             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMPANE_LOC          (7 + WINDOWS_ID_BIAS)
#define DLG_AWOG                  536 // Группа списания (PPOBJ_ASSTWROFFGRP)
	#define CTL_AWOG_NAME                (1 + WINDOWS_ID_BIAS)
	#define CTL_AWOG_ID                  (2 + WINDOWS_ID_BIAS)
	#define CTL_AWOG_SCHEME              (3 + WINDOWS_ID_BIAS)
	#define CTL_AWOG_TERM                (4 + WINDOWS_ID_BIAS)
	#define CTL_AWOG_LIMIT               (5 + WINDOWS_ID_BIAS)
	#define CTL_AWOG_CODE                (6 + WINDOWS_ID_BIAS)
#define DLG_ASSTREVAL             537 // Переоценка основных фондов
	#define CTL_ASSTREVAL_DT             (1 + WINDOWS_ID_BIAS)
	#define CTL_ASSTREVAL_OPRKIND        (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASSTREVAL_OPRKIND     (3 + WINDOWS_ID_BIAS)
	#define CTL_ASSTREVAL_LOC            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASSTREVAL_LOC         (5 + WINDOWS_ID_BIAS)
	#define CTL_ASSTREVAL_GT             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASSTREVAL_GT          (7 + WINDOWS_ID_BIAS)
	#define CTL_ASSTREVAL_GGRP           (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASSTREVAL_GGRP        (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_ASSTREVAL_DT          (10 + WINDOWS_ID_BIAS)
#define DLG_DRAFTWROFF            538 // Тип списания драфт-документов
	#define CTL_DRAFTWROFF_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_ID            (2 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_LIST          (3 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_POOLOP        (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_DRAFTWROFF_POOLOP     (5 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_DFCTOP        (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_DRAFTWROFF_DFCTOP     (7 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_DFCTAR        (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_DRAFTWROFF_DFCTAR     (9 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_FLAGS         (10 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_SETMT         (11 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_SETMTDO       (12 + WINDOWS_ID_BIAS)
	#define CTL_DRAFTWROFF_SETMTFT       (13 + WINDOWS_ID_BIAS)
#define DLG_DWOITEM               539 // Элемент типа списания драфт-документов
	#define CTL_DWOITEM_OP               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DWOITEM_OP            (2 + WINDOWS_ID_BIAS)
	#define CTL_DWOITEM_LOC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DWOITEM_LOC           (4 + WINDOWS_ID_BIAS)
#define DLG_DWOFILT               540 // Списание драфт-документов
	#define CTL_DWOFILT_PERIOD           (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_DWOFILT_PERIOD        (2 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_DWO              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DWOFILT_DWO           (4 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_LOC              (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_DWOFILT_LOC           (6 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_FLAGS            (7 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_MRPNAME          (8 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_ST_INFO          (9 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_SETMT            (10 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_SETMTDO          (11 + WINDOWS_ID_BIAS)
	#define CTL_DWOFILT_SETMTFT          (12 + WINDOWS_ID_BIAS)
#define DLG_GOODSWOG              541 // Группа списания товара (диалог товара)
	#define CTL_GOODSWOG_WOG             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSWOG_WOG          (2 + WINDOWS_ID_BIAS)
#define DLG_CALCTAXPRICE          542 // Калькулятор расчета цены с налогами
	#define CTL_CALCTAXPRICE_GOODS       (1 + WINDOWS_ID_BIAS)
	#define CTL_CALCTAXPRICE_TG          (2 + WINDOWS_ID_BIAS)
	#define CTL_CALCTAXPRICE_DT          (3 + WINDOWS_ID_BIAS)
	#define CTL_CALCTAXPRICE_OP          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CALCTAXPRICE_OP       (5 + WINDOWS_ID_BIAS)
	#define CTL_CALCTAXPRICE_WOTAX       (6 + WINDOWS_ID_BIAS)
	#define CTL_CALCTAXPRICE_WTAX        (7 + WINDOWS_ID_BIAS)
	#define CTL_CALCTAXPRICE_CALCFLG     (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_CALCTAXPRICE_DT       (9 + WINDOWS_ID_BIAS)
#define DLG_OUTBALATURN           543 // Забалансовая бухгалтерская проводка
	// CTL_ATURN_XXX
#define DLG_PAYPLANFLT            544 // План платежей
	#define CTL_PAYPLANFLT_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_PAYPLANFLT_PERIOD     (2 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_LOC           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PAYPLANFLT_LOC        (4 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_OBJECT        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PAYPLANFLT_OBJECT     (6 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_CYCLE         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PAYPLANFLT_CYCLE      (8 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_NUMCYCLES     (9 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_LABEL         (10 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_SHEET         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PAYPLANFLT_SHEET      (12 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_FLAGS         (13 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_SGP           (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PAYPLANFLT_SGP        (15 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANFLT_AGENT         (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_PAYPLANFLT_AGENT      (17 + WINDOWS_ID_BIAS)
#define DLG_PAYPLANTOTAL          545 // Итоги плана платежей
	#define CTL_PAYPLANTOTAL_COUNT       (1 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANTOTAL_AMOUNT      (2 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANTOTAL_PAYM        (3 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANTOTAL_DEBT        (4 + WINDOWS_ID_BIAS)
#define DLG_ADVBITEM              546
#define DLG_ADVREPITEM            547
	#define cmAdvBillItemEditLink        101
	#define cmAdvBillItemRemoveLink      102
	#define CTL_ADVBITEM_DT              (1 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_CODE            (2 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_BILLKIND        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADVBITEM_BILLKIND     (4 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_ACC             (5 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_ACCNAME         (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADVBITEM_ACCNAME      (7 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_ART             (8 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_ARTNAME         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADVBITEM_ARTNAME      (10 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_AMOUNT          (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_ADVBITEM_DT           (12 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_MEMO            (13 + WINDOWS_ID_BIAS)
	#define CTL_ADVBITEM_LINKINFO        (14 + WINDOWS_ID_BIAS)
#define DLG_ADVANCEREP            548 // Авансовый отчет
	// CTL_BILL_XXX, CTL_BILL_ADV_XXX
#define DLG_CABSLOTS              549 // Восстановление отсутствующих лотов
	#define CTL_CABSLOTS_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTL_CABSLOTS_LOG             (2 + WINDOWS_ID_BIAS)
	#define CTL_CABSLOTS_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_CABSLOTS_PERIOD       (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_CABSLOTS_LOG          (5 + WINDOWS_ID_BIAS)
#define DLG_SCARDOP               550 // Операция по персональной карте
	#define cmSCardOpCheck               101
	#define CTL_SCARDOP_DT               (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_TM               (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_AMOUNT           (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDOP_DT            (4 + WINDOWS_ID_BIAS)
	#define CTLTM_SCARDOP_TM             (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_ST_REST          (6 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_CARDNO           (7 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_OWNER            (8 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_DESTCARDNO       (9 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_DESTOWNER        (10 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_ST_DESTREST      (11 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_FRZSTART         (12 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDOP_FRZSTART      (13 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_FRZEND           (14 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDOP_FRZEND        (15 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_EXPIRY           (16 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_FRZPERIOD        (17 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDOP_FRZPERIOD     (18 + WINDOWS_ID_BIAS)
	#define CTL_SCARDOP_QTTY             (19 + WINDOWS_ID_BIAS) // @v12.0.6 Количественная операция
	#define CTL_SCARDOP_ST_CTREST        (20 + WINDOWS_ID_BIAS) // @v12.0.6 Количественный остаток
#define DLG_REQALTOBJ             551 // Выбор альтернативной статьи для зачета
	#define CTL_REQALTOBJ_SHEET          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_REQALTOBJ_SHEET       (2 + WINDOWS_ID_BIAS)
	#define CTL_REQALTOBJ_CLIENT         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REQALTOBJ_CLIENT      (4 + WINDOWS_ID_BIAS)
#define DLG_SCCHRGCRD             552 // Начисление по выборке кредитных карт
	#define CTL_SCCHRGCRD_DATE           (1 + WINDOWS_ID_BIAS)
	#define CTL_SCCHRGCRD_WHAT           (2 + WINDOWS_ID_BIAS)
	#define CTL_SCCHRGCRD_AMOUNT         (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCCHRGCRD_DATE        (4 + WINDOWS_ID_BIAS)
#define DLG_DL200P                553 // @v3.9.11 DL200 Processor Params
	#define CTL_DL200P_FNAME             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DL200P_FNAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_DL200P_DATANAME          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DL200P_DATANAME       (4 + WINDOWS_ID_BIAS)
	#define CTL_DL200P_PERIOD            (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_DL200P_PERIOD         (6 + WINDOWS_ID_BIAS)
	#define CTL_DL200P_CYCLE             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_DL200P_CYCLE          (8 + WINDOWS_ID_BIAS)
	#define CTL_DL200P_NUMCYCLES     	(9 + WINDOWS_ID_BIAS)
#define DLG_DBTRANSM_IN           554 // @3.9.14 Прием данных из другого раздела
	#define CTL_DBTRANSM_IN_INFO1        (1 + WINDOWS_ID_BIAS)
	#define CTL_DBTRANSM_IN_INFO2        (2 + WINDOWS_ID_BIAS)
	#define CTL_DBTRANSM_IN_INFO3        (3 + WINDOWS_ID_BIAS)
	#define CTL_DBTRANSM_IN_OPTIONS      (4 + WINDOWS_ID_BIAS)
	#define CTL_DBTRANSM_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTL_DBTRANSM_IN_LIST         (6 + WINDOWS_ID_BIAS)
//                                555 // Occupide by DLG_OPKMORE_ORD
#define DLG_CORATURN              556 // Корректировка бухгалтерских проводок
	#define CTL_CORATURN_LOG             (1 + WINDOWS_ID_BIAS)
	#define CTL_CORATURN_FLAGS           (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORATURN_LOG          (3 + WINDOWS_ID_BIAS)
#define DLG_UNITEINVENT           557 // Опции объединения документов инвентаризации
	#define CTL_UNITEINVENT_SGOPTION     (1 + WINDOWS_ID_BIAS)
	#define CTL_UNITEINVENT_FLAGS        (2 + WINDOWS_ID_BIAS)
#define DLG_RMVDRAFT              558 // Удаление или откат списания драфт-документа
#define DLG_RMVDRAFTALL           559 // Удаление или откат списания драфт-документов
	#define CTL_RMVDRAFT_WHAT            (1 + WINDOWS_ID_BIAS)
//                                560 // Occupied (DLG_SELCSESSEXC)
#define DLG_GENOPITEM             561 // Элемент обобщенной операции
	#define CTL_GENOPITEM_OP             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GENOPITEM_OP          (2 + WINDOWS_ID_BIAS)
	#define CTL_GENOPITEM_FLAGS          (3 + WINDOWS_ID_BIAS)
//                                562 // Occupied (DLG_INOUTANLZTOTAL)
//                                563 // Occupied (DLG_ACCANLZ2)
#define DLG_GOODSTAXVIEW          564 // Список налоговых групп
#define DLG_BILLPRELUDE           565 // Диалог выбора вида операции и склада перед вводом нового документа
	#define CTL_BILLPRELUDE_OPLIST       (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLPRELUDE_LOC          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLPRELUDE_LOC       (3 + WINDOWS_ID_BIAS)
#define DLG_CHGSCARDDISC          566 // Изменение процента скидки по выборке карточек
	#define CTL_CHGSCARDDISC_VAL         (1 + WINDOWS_ID_BIAS)
#define DLG_CABSGOODS             567 // Восстановление отсутствующих товаров
	#define CTL_CABSGOODS_LOG            (1 + WINDOWS_ID_BIAS)
	#define CTL_CABSGOODS_FLAGS          (2 + WINDOWS_ID_BIAS)
	#define CTL_CABSGOODS_GGRP           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CABSGOODS_GGRP        (4 + WINDOWS_ID_BIAS)
	#define CTL_CABSGOODS_UNIT           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CABSGOODS_UNIT        (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_CABSGOODS_LOG         (7 + WINDOWS_ID_BIAS)
#define DLG_PRNGLABEL             568 // Печать товарных этикеток
	#define CTL_PRNGLABEL_COUNT          (1 + WINDOWS_ID_BIAS)
#define DLG_SYNCCMPFLT            569 // Внутренний фильтр анализа синхронизации объектов
	#define CTL_SYNCCMPFLT_ERR           (1 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPFLT_ORDER         (2 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPFLT_OBJ           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SYNCCMPFLT_OBJ        (4 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPFLT_SCORE         (5 + WINDOWS_ID_BIAS)
#define DLG_FILLSALESTBL          570 // FSALES Заполнение таблицы продаж
	#define CTL_FILLSALESTBL_BEGDT       (1 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_LASTDT      (2 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_OPNAME      (3 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_ENDDT       (4 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_UPDATE      (5 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_PROCESS     (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_FILLSALESTBL_ENDDT    (7 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_GRP         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_FILLSALESTBL_GRP      (9 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_RCALBDT     (10 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_CONT        (11 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_ST_CONT     (12 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_MAXTIME     (13 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_IMG         (14 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_GDS         (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_FILLSALESTBL_GDS      (16 + WINDOWS_ID_BIAS)
	#define CTL_FILLSALESTBL_FLAGS       (17 + WINDOWS_ID_BIAS)
#define DLG_PSALESTOTAL           571 // Итоги по статистике продаж
	#define CTL_PSALESTOTAL_COUNT        (1 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYSUM       (2 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYAVG       (3 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYTRN       (4 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYSIGM      (5 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYSTDDV     (6 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYVAR       (7 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYERAVG     (8 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_QTYERPCT     (9 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTSUM       (10 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTAVG       (11 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTTRN       (12 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTSIGM      (13 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTSTDDV     (14 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTVAR       (15 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTERAVG     (16 + WINDOWS_ID_BIAS)
	#define CTL_PSALESTOTAL_AMTERPCT     (17 + WINDOWS_ID_BIAS)

#define DLG_TESTPREDICT           572 // Тест прогноза продаж
	#define CTL_TESTPREDICT_P            (1 + WINDOWS_ID_BIAS)
	#define CTL_TESTPREDICT_Q            (2 + WINDOWS_ID_BIAS)
	#define CTL_TESTPREDICT_FLAGS        (3 + WINDOWS_ID_BIAS)
#define DLG_TRFRANLZTOTAL         573 // Итоги анализа товарных операций
#define DLG_TRFRANLZTOTAL_S       811 // Итоги анализа товарных операций с сальдо
	#define CTL_TRFRANLZTOTAL_COUNT      (1 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_QTTY       (2 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_PHQTTY     (3 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_COST       (4 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_PRICE      (5 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_DIS        (6 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_AMOUNT     (7 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_SQTTY      (8 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_SAMT       (9 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_PVAT       (10 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_EXTV1      (11 + WINDOWS_ID_BIAS)
	#define CTL_TRFRANLZTOTAL_EXTV2      (12 + WINDOWS_ID_BIAS)
#define DLG_GOODSTOTAL            574 // Итоги по выборке товаров
	#define CTL_GOODSTOTAL_COUNT         (1 + WINDOWS_ID_BIAS)
#define DLG_PERSONTOTAL           575 // Итоги по выборке персоналий
	#define CTL_PERSONTOTAL_COUNT        (1 + WINDOWS_ID_BIAS)
	#define CTL_PERSONTOTAL_CA_ND        (2 + WINDOWS_ID_BIAS) // stNoData
	#define CTL_PERSONTOTAL_CA_R         (3 + WINDOWS_ID_BIAS) // stRegularTa
	#define CTL_PERSONTOTAL_CA_D         (4 + WINDOWS_ID_BIAS) // stDelayedTa
	#define CTL_PERSONTOTAL_CA_L         (5 + WINDOWS_ID_BIAS) // stHopelesslyDelayedTa
	#define CTL_PERSONTOTAL_CA_N         (6 + WINDOWS_ID_BIAS) // stfNewClient
#define DLG_BILLEXTMOD            576 // Независимое редактирование некоторых параметров документа
#define DLG_BHTPROGUPLOAD         577 // Загрузка программы на BHT-терминал
#define DLG_TXM                   578 // Маркеры налоговых платежей для платежных поручений
	#define cmSelectPeriod               601
	#define CTL_TXM_TAXCLASS             (1 + WINDOWS_ID_BIAS)
	#define CTL_TXM_OKATO                (2 + WINDOWS_ID_BIAS)
	#define CTL_TXM_REASON               (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TXM_REASON            (4 + WINDOWS_ID_BIAS)
	#define CTL_TXM_TAXPERIOD            (5 + WINDOWS_ID_BIAS)
	#define CTL_TXM_DOCNUMBER            (6 + WINDOWS_ID_BIAS)
	#define CTL_TXM_DOCDATE              (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_TXM_DOCDATE           (8 + WINDOWS_ID_BIAS)
	#define CTL_TXM_PAYMTYPE             (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_TXM_PAYMTYPE          (10 + WINDOWS_ID_BIAS)
	#define CTL_TXM_UIN                  (11 + WINDOWS_ID_BIAS)
#define DLG_TXMPERIOD             579 // Выбор налогового периода
	#define CTL_TXMPERIOD_PERIOD         (1 + WINDOWS_ID_BIAS)
	#define CTL_TXMPERIOD_YEAR           (2 + WINDOWS_ID_BIAS)
	#define CTL_TXMPERIOD_MONTH          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TXMPERIOD_MONTH       (4 + WINDOWS_ID_BIAS)
	#define CTL_TXMPERIOD_DAY            (5 + WINDOWS_ID_BIAS)
#define DLG_ABSBILL               580 // Потерянный документ
	#define cmViewAccturns            101
	#define CTL_ABSBILL_ID               (1 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_DT               (2 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_COST             (3 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_PRICE            (4 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_DISCOUNT         (5 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_LINESCOUNT       (6 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_LOCNAME          (7 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_OP               (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_ABSBILL_OP            (9 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_OBJECT           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_ABSBILL_OBJECT        (11 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_STOP             (12 + WINDOWS_ID_BIAS)
	#define CTL_ABSBILL_ST_SRC           (13 + WINDOWS_ID_BIAS)
#define DLG_GOODSEXTITLES         581 // Наименования дополнительных полей товара
	#define CTL_GOODSEXTITLES_A          (1 + WINDOWS_ID_BIAS)
	#define CTL_GOODSEXTITLES_B          (2 + WINDOWS_ID_BIAS)
	#define CTL_GOODSEXTITLES_C          (3 + WINDOWS_ID_BIAS)
	#define CTL_GOODSEXTITLES_D          (4 + WINDOWS_ID_BIAS)
	#define CTL_GOODSEXTITLES_E          (5 + WINDOWS_ID_BIAS)
	#define CTL_GOODSEXTITLES_AI         (6 + WINDOWS_ID_BIAS)
#define DLG_ASSET                 582 // Основные средства (CTL_GOODS_XXX)
#define DLG_ASSETFLT              583 // Фильтр по основным средствам
	#define CTL_ASSETFLT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_ASSETFLT_PERIOD       (2 + WINDOWS_ID_BIAS)
	#define CTL_ASSETFLT_OPERAT          (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_ASSETFLT_OPERAT       (4 + WINDOWS_ID_BIAS)
	#define CTL_ASSETFLT_LOC             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASSETFLT_LOC          (6 + WINDOWS_ID_BIAS)
	#define CTL_ASSETFLT_GGRP            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASSETFLT_GGRP         (8 + WINDOWS_ID_BIAS)
	#define CTL_ASSETFLT_CLOSED          (9 + WINDOWS_ID_BIAS)
	#define CTL_ASSETFLT_FLAGS           (10 + WINDOWS_ID_BIAS)
	#define CTL_ASSETFLT_INEXPL          (11 + WINDOWS_ID_BIAS)
#define DLG_ASSETTOTAL            584 // Итоги по основным средствам
	#define CTL_ASSETTOTAL_COUNT         (1 + WINDOWS_ID_BIAS)
	#define CTL_ASSETTOTAL_COST          (2 + WINDOWS_ID_BIAS)
	#define CTL_ASSETTOTAL_PRICE         (3 + WINDOWS_ID_BIAS)
	#define CTL_ASSETTOTAL_DEPREC        (4 + WINDOWS_ID_BIAS)
#define DLG_GOODSBILLTOTAL        585 // Итоги по строкам товарного документа
	#define CTL_GBILLTOTAL_LINES         (1 + WINDOWS_ID_BIAS)
	#define CTL_GBILLTOTAL_GCOUNT        (2 + WINDOWS_ID_BIAS)
	#define CTL_GBILLTOTAL_BRUTTO        (3 + WINDOWS_ID_BIAS)
	#define CTL_GBILLTOTAL_VOLUME        (4 + WINDOWS_ID_BIAS)
	#define CTL_GBILLTOTAL_AMTLIST       (5 + WINDOWS_ID_BIAS)
	#define CTL_GBILLTOTAL_QTTY          (6 + WINDOWS_ID_BIAS)
	#define CTL_GBILLTOTAL_PHQTTY        (7 + WINDOWS_ID_BIAS)
#define DLG_ADDRLIST              586 // Список адресов доставки (в диалоге персоналии)
	#define CTL_ADDRLIST_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_ADDRLIST_LIST            (2 + WINDOWS_ID_BIAS)
#define DLG_UPDBLIST              587 // Изменение атрибутов выборки документов
	#define CTL_UPDBLIST_PAYER           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDBLIST_PAYER        (2 + WINDOWS_ID_BIAS)
	#define CTL_UPDBLIST_AGENT           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDBLIST_AGENT        (4 + WINDOWS_ID_BIAS)
	#define CTL_UPDBLIST_OBJECT          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDBLIST_OBJECT       (6 + WINDOWS_ID_BIAS)
	#define CTL_UPDBLIST_OBJECT2         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDBLIST_OBJECT2      (8 + WINDOWS_ID_BIAS)
	#define CTL_UPDBLIST_MODCODEST       (9 + WINDOWS_ID_BIAS)
#define DLG_CHKPAYM               588 // Выбор способа оплаты кассового чека
	#define CTL_CHKPAYM_METHOD           (1 + WINDOWS_ID_BIAS)
#define DLG_RTCSESS               589 // Права доступа к кассовым сессиям
	#define CTL_RTCSESS_FLAGS            (1 + WINDOWS_ID_BIAS)
	#define CTL_RTCSESS_SFLAGS           (2 + WINDOWS_ID_BIAS)
	#define CTL_RTCSESS_SFLAGS2          (3 + WINDOWS_ID_BIAS)
	#define CTL_RTCSESS_SFLAGS3          (4 + WINDOWS_ID_BIAS)
#define DLG_SRGOODSCFM            590 // Подтверждение замены наименования товара
#define DLG_FRGHTFLT              592 // Фильтр анализа фрахтов
	#define CTL_FRGHTFLT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_FRGHTFLT_PERIOD       (2 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_SHIPMPERIOD     (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_FRGHTFLT_SHIPMPERIOD  (4 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_ARRVLPERIOD     (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_FRGHTFLT_ARRVLPERIOD  (6 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_OP              (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_FRGHTFLT_OP           (8 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_OBJECT          (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_FRGHTFLT_OBJECT       (10 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_LOC             (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_FRGHTFLT_LOC          (12 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_SHIP            (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_FRGHTFLT_SHIP         (14 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_PORT            (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_FRGHTFLT_PORT         (16 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_FLAGS           (17 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_CARGOPAR        (18 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_CAPTAIN         (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_FRGHTFLT_CAPTAIN      (20 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTFLT_ORDER           (21 + WINDOWS_ID_BIAS)
#define DLG_FRGHTTOTAL            593 // Итоги по зафрахтованным документам
	#define CTL_FRGHTTOTAL_COUNT         (1 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTTOTAL_AMOUNT        (2 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTTOTAL_PACKCOUNT     (3 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTTOTAL_BRUTTO        (4 + WINDOWS_ID_BIAS)
	#define CTL_FRGHTTOTAL_VOLUME        (5 + WINDOWS_ID_BIAS)
#define DLG_PROJECT               594 // Проект
#define DLG_PRJPHASE              595 // Фаза проекта
	#define CTL_PRJ_NAME                 (1 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_CODE                 (2 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_DT                   (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRJ_DT                (4 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_START                (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRJ_START             (6 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_ESTFINISH            (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRJ_ESTFINISH         (8 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_FINISH               (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRJ_FINISH            (10 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_STATUS               (11 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_MNGR                 (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRJ_MNGR              (13 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_CLIENT               (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRJ_CLIENT            (15 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_DESCR                (16 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_PARENTNAME           (17 + WINDOWS_ID_BIAS)
	#define CTL_PRJ_BILLOP               (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRJ_BILLOP            (19 + WINDOWS_ID_BIAS)
#define DLG_PRJFLT                596 // Фильтр по проектам
	#define CTL_PRJFLT_PRDSTART          (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRJFLT_PRDSTART       (2 + WINDOWS_ID_BIAS)
	#define CTL_PRJFLT_PRDESTFINISH      (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRJFLT_PRDESTFINISH   (4 + WINDOWS_ID_BIAS)
	#define CTL_PRJFLT_MNGR              (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRJFLT_MNGR           (6 + WINDOWS_ID_BIAS)
	#define CTL_PRJFLT_CLIENT            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRJFLT_CLIENT         (8 + WINDOWS_ID_BIAS)
	#define CTL_PRJFLT_FLAGS             (9 + WINDOWS_ID_BIAS)
	#define CTL_PRJFLT_SORTORD           (10 + WINDOWS_ID_BIAS)
#define DLG_QUOTFLT               597 // Фильтр по котировкам
	#define CTL_QUOTFLT_QK               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_QK            (2 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_LOC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_LOC           (4 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_CLIENT           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_CLIENT        (6 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_GGRP             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_GGRP          (8 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_GOODS            (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_GOODS         (10 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_ALLQK            (11 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_FLAGS            (12 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_CUR              (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_CUR           (14 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_ALLLOC           (15 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_VALRANGE         (16 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_ABSENCE          (17 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_TYPE             (18 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_EFFDATE          (19 + WINDOWS_ID_BIAS)
	#define CTLCAL_QUOTFLT_EFFDATE       (20 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_ORDER            (21 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_SETPRD           (22 + WINDOWS_ID_BIAS)
	#define CTLCAL_QUOTFLT_SETPRD        (23 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_GGRPDIFF         (24 + WINDOWS_ID_BIAS)
	#define CTL_QUOTFLT_SGG              (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTFLT_SGG           (26 + WINDOWS_ID_BIAS)
//                                598 // Occupied (DLG_PERSON_BNK)
#define DLG_MAILACC               599 // MailAccount
	#define CTL_MAILACC_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_ID               (2 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_FROMADDR         (3 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_SENDSRV          (4 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_SENDPORT         (5 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_RCVSRV           (6 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_RCVPORT          (7 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_RCVNAME          (8 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_RCVPASSWORD      (9 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_TIMEOUT          (10 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_AUTHTYPE         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAILACC_AUTHTYPE      (12 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_FLAGS            (13 + WINDOWS_ID_BIAS)
	#define CTL_MAILACC_SYMB             (14 + WINDOWS_ID_BIAS)

#define DLG_BACKUP		          601 // Резервное копирование
	#define CTL_BACKUP_DB                (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BACKUP_DB             (2 + WINDOWS_ID_BIAS)
	#define CTL_BACKUP_DATE              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BACKUP_DATE           (4 + WINDOWS_ID_BIAS)
#define DLG_BU_PRELUDE            602 // Служебные операции с базами данных
	#define cmBuBackup                   101
	#define cmBuRestore                  102
	#define cmBuInfo                     103
	#define cmBuRemove                   104
	#define cmBuRemoveCr                 105
	#define cmBuAutoBackup               106
	#define cmRecover                    107
	#define cmProtect                    108
	#define cmCreateDB                   109
	#define cmSetupBackupCfg             110
	#define cmBuCheck                    111
	#define cmBuReleaseContinuous        112 // Освобождение файлов от continuous-режима
	#define cmDump                       113 // Дамп базы данных
	#define cmDbMonitor                  114
	#define CTL_BU_PRELUDE_DB            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BU_PRELUDE_DB         (2 + WINDOWS_ID_BIAS)
#define DLG_BU_BACKUP             603 // Резервное копирование
#define DLG_BU_RSTR               604 // Восстановление копии
	#define CTL_BU_BACKUP_SCEN           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BU_BACKUP_SCEN        (2 + WINDOWS_ID_BIAS)
	#define CTL_BU_BACKUP_LIST           (3 + WINDOWS_ID_BIAS)
// @v11.0.2 #define DLG_MAIN_CONFIG           605 // Конфигурирование баз данных
	// @v11.0.2 #define CTLSLC_CONFIG                (1 + WINDOWS_ID_BIAS)
#define DLG_MAKEDB_CONFIG         606 // Создание базы данных
	#define CTLSLC_CONFIG                (1 + WINDOWS_ID_BIAS)
	#define CTL_MAKEDB_DBNAME            (2 + WINDOWS_ID_BIAS)
	#define CTLCBX_MAKEDB_DBNAME         (3 + WINDOWS_ID_BIAS)
	#define CTL_MAKEDB_PATH              (4 + WINDOWS_ID_BIAS)
	#define CTL_MAKEDB_NAME              (5 + WINDOWS_ID_BIAS)
	#define CTL_MAKEDB_SYMBOL            (6 + WINDOWS_ID_BIAS)
#define DLG_BUCFG_SELECT          607 // Выбор сценария для Buckup
	#define CTL_BUCFG_SCNAME             (1 + WINDOWS_ID_BIAS)
#define DLG_BUCFG_EDIT            608 // Редактирование сценария //
#define DLG_BUCFG_EDIT_JOB        700 // Параметры резервного копирования (серверная задача)
	#define CTL_BUCFG_DBNAME             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BUCFG_DBNAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_BUCFG_CFGNAME            (3 + WINDOWS_ID_BIAS)
	#define CTL_BUCFG_PERIOD             (4 + WINDOWS_ID_BIAS)
	#define CTL_BUCFG_COPIES             (5 + WINDOWS_ID_BIAS)
	#define CTL_BUCFG_COMPRESS           (6 + WINDOWS_ID_BIAS)
	#define CTL_BUCFG_PATH               (7 + WINDOWS_ID_BIAS)
	#define CTLBRW_BUCFG_PATH            (8 + WINDOWS_ID_BIAS)
#define DLG_CFGBAT                614 // Конфигурация BAT файла
	#define CTL_CFGBAT_BTRVPATH          (1 + WINDOWS_ID_BIAS)
	#define CTL_CFGBAT_TYPE              (2 + WINDOWS_ID_BIAS)
	#define CTL_CFGBAT_PRN               (3 + WINDOWS_ID_BIAS)
	#define CTL_CFGBAT_DBNAME            (4 + WINDOWS_ID_BIAS)
	#define CTL_CFGBAT_USER              (5 + WINDOWS_ID_BIAS)
	#define CTL_CFGBAT_DFLT              (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_CFGBAT_BTRVPATH       (7 + WINDOWS_ID_BIAS)
#define DLG_GOODSRMVALL           615 // GRMVALL Удаление или перемещение выборки товаров
	#define cmGoodsChgClssfr             101
	#define CTL_REMOVEALL_WHAT           (1 + WINDOWS_ID_BIAS)
	#define CTL_REMOVEALL_GRP            (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_REMOVEALL_GRP         (3 + WINDOWS_ID_BIAS)
	#define CTL_REMOVEALL_FLAGS          (4 + WINDOWS_ID_BIAS)
	#define CTL_REMOVEALL_RULE           (5 + WINDOWS_ID_BIAS)
#define DLG_BSKT2BILL             616 // Опции переноса корзины в товарный документ
	#define CTL_BSKT2BILL_RULECOST       (1 + WINDOWS_ID_BIAS)
	#define CTL_BSKT2BILL_RULEPRICE      (2 + WINDOWS_ID_BIAS)
	#define CTL_BSKT2BILL_QK             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BSKT2BILL_QK          (4 + WINDOWS_ID_BIAS)
	#define CTL_BSKT2BILL_USELINK        (5 + WINDOWS_ID_BIAS)
#define DLG_PSNSELANALOG          617 // Выбор персоналии по соответствию имени
	#define CTL_PSNSELANALOG_SRCH        (1 + WINDOWS_ID_BIAS)
	#define CTL_PSNSELANALOG_LIST        (2 + WINDOWS_ID_BIAS)
#define DLG_ADDRBOOK              618 // Адресная книга
	#define CTL_ADDRBOOK_LIST            (1 + WINDOWS_ID_BIAS)
#define DLG_SELPERSON             619 // Выбор персоналии
	#define CTL_SELPERSON_KIND           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELPERSON_KIND        (2 + WINDOWS_ID_BIAS)
	#define CTL_SELPERSON_PRSN           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELPERSON_PRSN        (4 + WINDOWS_ID_BIAS)
#define DLG_MAILCHRY              620 // Параметры отправки объектов charry по email
	#define CTL_MAILCHRY_MAILACC         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAILCHRY_MAILACC      (2 + WINDOWS_ID_BIAS)
	#define CTL_MAILCHRY_DESTADDR        (3 + WINDOWS_ID_BIAS)
	#define CTL_MAILCHRY_FLAGS           (4 + WINDOWS_ID_BIAS)
#define DLG_RCVCHRY               621 // Параметры приема объектов CHARRY
	#define CTL_RCVCHRY_ACTION           (1 + WINDOWS_ID_BIAS)
	#define CTL_RCVCHRY_MAILACC          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_RCVCHRY_MAILACC       (3 + WINDOWS_ID_BIAS)
#define DLG_GBILLCMPFLT           622 // Сравнение товарных строк документов
	#define CTL_GBILLCMPFLT_LHBILL       (1 + WINDOWS_ID_BIAS)
	#define CTL_GBILLCMPFLT_RHBILL       (2 + WINDOWS_ID_BIAS)
	#define CTL_GBILLCMPFLT_FLAGS        (3 + WINDOWS_ID_BIAS)
	#define CTL_GBILLCMPFLT_ORDER        (4 + WINDOWS_ID_BIAS)
#define DLG_GBILLCMPTTL           623 // Итоги сравнения товарных строк документов
	#define CTL_GBILLCMPTTL_LHBILL       (1 + WINDOWS_ID_BIAS)
	#define CTL_GBILLCMPTTL_RHBILL       (2 + WINDOWS_ID_BIAS)
	#define CTL_GBILLCMPTTL_COUNT        (3 + WINDOWS_ID_BIAS)
	#define CTL_GBILLCMPTTL_DIFCOUNT     (4 + WINDOWS_ID_BIAS)
#define DLG_PAYMTOTAL             624 // Итоги оплат по документу
	#define CTL_PAYMTOTAL_COUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_PAYMTOTAL_AMOUNT         (2 + WINDOWS_ID_BIAS)
//                                625 // Occupied DLG_OPKMORE_CHG
#define DLG_SETQUOT               626 // Помощник установки значений котировок
	#define CTL_SETQUOT_HOW              (1 + WINDOWS_ID_BIAS)
	#define CTL_SETQUOT_VAL              (2 + WINDOWS_ID_BIAS)
	#define CTL_SETQUOT_QTTY             (3 + WINDOWS_ID_BIAS)
	#define CTL_SETQUOT_ZERO             (4 + WINDOWS_ID_BIAS)
	#define CTL_SETQUOT_ST_INFO          (5 + WINDOWS_ID_BIAS)
	#define CTL_SETQUOT_PERIOD           (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_SETQUOT_PERIOD        (7 + WINDOWS_ID_BIAS)
#define DLG_DIVGRPASSC            627 // Список соответствия 'группа-отдел' для кассового узла
	#define CTL_DIVGRPASSC_LIST          (1 + WINDOWS_ID_BIAS)
#define DLG_DIVGRPASSCITM         628 // Элемент списка ассоциаций 'группа-отдел'
	#define CTL_DIVGRPASSCITM_GRP        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DIVGRPASSCITM_GRP     (2 + WINDOWS_ID_BIAS)
	#define CTL_DIVGRPASSCITM_DIV        (3 + WINDOWS_ID_BIAS)
#define DLG_GOODSFLTVAT           629 // Фильтрация справочника товаров по ставке НДС
	#define CTL_GOODSFLTVAT_RATE         (1 + WINDOWS_ID_BIAS)
	#define CTL_GOODSFLTVAT_DATE         (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_GOODSFLTVAT_DATE      (3 + WINDOWS_ID_BIAS)
#define DLG_W_UPDQUOT             630 // Предупреждение о замене котировки
	#define CTL_W_UPDQUOT_GOODS          (1 + WINDOWS_ID_BIAS)
	#define CTL_W_UPDQUOT_OLDQUOT        (2 + WINDOWS_ID_BIAS)
	#define CTL_W_UPDQUOT_NEWQUOT        (3 + WINDOWS_ID_BIAS)
	#define CTL_W_UPDQUOT_ALLFLAGS       (4 + WINDOWS_ID_BIAS)
#define DLG_BRAND                 631 // Бренд
#define DLG_RTPERSON              632 // Права доступа к персоналиям
#define DLG_UPDGOODSFLAGS         633 // Изменение флагов выборки товаров
	#define CTL_UPDGOODSFLAGS_SET        (1 + WINDOWS_ID_BIAS)
	#define CTL_UPDGOODSFLAGS_RESET      (2 + WINDOWS_ID_BIAS)
#define DLG_HOLIDAYS              634 // Список выходных дней
	#define cmHolidaysScan               101
	#define cmHolidaysTest               102
	#define CTL_HOLIDAYS_LIST            (1 + WINDOWS_ID_BIAS)
	#define CTL_HOLIDAYS_LOC             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_HOLIDAYS_LOC          (3 + WINDOWS_ID_BIAS)
#define DLG_HOLIDAY               635 // Выходной день
	#define CTL_HOLIDAY_KIND             (1 + WINDOWS_ID_BIAS)
	#define CTL_HOLIDAY_DATE             (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_HOLIDAY_DATE          (3 + WINDOWS_ID_BIAS)
	#define CTL_HOLIDAY_DAYOFWEEK        (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_HOLIDAY_DAYOFWEEK     (5 + WINDOWS_ID_BIAS)
	#define CTL_HOLIDAY_ST_LOC           (6 + WINDOWS_ID_BIAS)
#define DLG_REPLCARD              636 // Перенос операций с одной карточки на другую //
	#define CTL_REPLCARD_SRCCARD         (1 + WINDOWS_ID_BIAS)
	#define CTL_REPLCARD_DESTCARD        (2 + WINDOWS_ID_BIAS)
#define DLG_CCHECKINFO            637 // Системная информация по чеку
	#define CTL_CCHECKINFO_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_CODE          (2 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_CASHCODE      (3 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_USERID        (4 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_SESSID        (5 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_DATE          (6 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_TIME          (7 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_AMOUNT        (8 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_DSCNT         (9 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_CARDNO        (10 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_FLAGS         (11 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_ADDPAYM       (12 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_AGENT         (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHECKINFO_AGENT      (14 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_TABLE         (15 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_FLAGS2        (16 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_LINKID        (17 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_PAYMLIST      (18 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_SESSUUID      (19 + WINDOWS_ID_BIAS)
	#define CTL_CCHECKINFO_RPRCTAID      (20 + WINDOWS_ID_BIAS) // remote-processing-transation-id
	#define CTL_CCHECKINFO_EADDR         (21 + WINDOWS_ID_BIAS) // @v11.3.6 Электронный адрес покупателя //
	#define CTL_CCHECKINFO_UUID          (22 + WINDOWS_ID_BIAS) // @v11.5.8
	#define CTL_CCHECKINFO_FSCLSIGN      (23 + WINDOWS_ID_BIAS) // @v12.2.8 Фискальный признак чека
	#define CTL_CCHECKINFO_OP            (24 + WINDOWS_ID_BIAS) // @v12.2.9 Операция чека (general|return|correction|invalid)
#define DLG_PSNRELTYPE            638 // Тип связи между персоналиями
	#define CTL_PSNRELTYPE_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELTYPE_ID            (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELTYPE_SYMB          (3 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELTYPE_FLAGS         (4 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELTYPE_CARDINAL      (5 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELTYPE_STATUSR       (6 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELTYPE_INHRGLIST     (7 + WINDOWS_ID_BIAS)
#define DLG_SYNCCMPTOTAL          639 // Итоги сравнения синхронизации объектов
	#define CTL_SYNCCMPTOTAL_NECOUNT     (1 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPTOTAL_N1COUNT     (2 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPTOTAL_N2COUNT     (3 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPTOTAL_N3COUNT     (4 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPTOTAL_N4COUNT     (5 + WINDOWS_ID_BIAS)
	#define CTL_SYNCCMPTOTAL_COUNT       (6 + WINDOWS_ID_BIAS)
#define DLG_PERSONLINK            640 // Отношение между персоналиями
	#define CTL_PERSONLINK_PRMR          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONLINK_PRMR       (2 + WINDOWS_ID_BIAS)
	#define CTL_PERSONLINK_SCND          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONLINK_SCND       (4 + WINDOWS_ID_BIAS)
	#define CTL_PERSONLINK_LTYPE         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONLINK_LTYPE      (6 + WINDOWS_ID_BIAS)
	#define CTL_PERSONLINK_KIND          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PERSONLINK_KIND       (8 + WINDOWS_ID_BIAS)
#define DLG_PERSONLINKLIST        641 // Список отношений между персоналиями
	#define cmPersonLinkListReverse      2001 // Обращение списка (показать связи других персоналий с данной)
	#define CTL_PERSONLINKLIST_PSN       (1 + WINDOWS_ID_BIAS)
	#define CTL_PERSONLINKLIST_LIST      (2 + WINDOWS_ID_BIAS)
#define DLG_SELRMVPAYM            642 // Удаление документа оплаты
	#define CTL_SELRMVPAYM_WHAT          (1 + WINDOWS_ID_BIAS)
#define DLG_SSTATORDFLT           643 // Фильтр по статистике продаж с заказом поставщику
#define DLG_CSESS                 644 // @v4.6.4 Системная информация о кассовой сессии
	#define CTL_CSESS_ID                 (1 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_NUMBER             (2 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_DATE               (3 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_TIME               (4 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_CASHNODE           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CSESS_CASHNODE        (6 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_SUPERSESSID        (7 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_INCOMPL            (8 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_TEMP               (9 + WINDOWS_ID_BIAS)
	#define CTL_CSESS_SUPERSESSTEXT      (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_CSESS_DATE            (11 + WINDOWS_ID_BIAS) // @alena
	#define CTL_CSESS_AMTLIST            (12 + WINDOWS_ID_BIAS)
#define DLG_LOCVIEW               645 // @v4.6.4 Список просмотра складов
	#define cmWareplaceList             101 // Места хранения, соответствующие складу
	#define cmLocGoodsAssoc             102 // Ассоциации "товар-склад" или "товар-место хранения"
#define DLG_RMVDGDS               646 // @v4.6.4 Сообщение о висячем товаре с предложением замены
	#define CTL_RMVDGDS_ID               (1 + WINDOWS_ID_BIAS)
	#define CTL_RMVDGDS_GOODSGRP         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_RMVDGDS_GOODSGRP      (3 + WINDOWS_ID_BIAS)
	#define CTL_RMVDGDS_GOODS            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_RMVDGDS_GOODS         (5 + WINDOWS_ID_BIAS)
#define DLG_ACCALIAS              647 // @v4.6.4 Алиас счета (CTL_ACCOUNT_XXX)
#define DLG_ALSSUBST              648 // @v4.6.4 Элемент подстановки алиаса счета
	#define CTL_ALSSUBST_ALS             (1 + WINDOWS_ID_BIAS)
	#define CTL_ALSSUBST_ALSNAME         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALSSUBST_ALSNAME      (3 + WINDOWS_ID_BIAS)
	#define CTL_ALSSUBST_ACC             (4 + WINDOWS_ID_BIAS)
	#define CTL_ALSSUBST_ACCNAME         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALSSUBST_ACCNAME      (6 + WINDOWS_ID_BIAS)
#define DLG_ARTICLE2              649 // @v4.6.4 Аналитическая статья с подстановкой алиасов (CTL_ARTICLE_XXX)
#define DLG_MRPTABFLT             650 // @v4.6.6 Фильтр по MRP-таблицам
	#define CTL_MRPTABFLT_OBJTYPE        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MRPTABFLT_OBJTYPE     (2 + WINDOWS_ID_BIAS)
	#define CTL_MRPTABFLT_LOC            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_MRPTABFLT_LOC         (4 + WINDOWS_ID_BIAS)
	#define CTL_MRPTABFLT_PERIOD         (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_MRPTABFLT_PERIOD      (6 + WINDOWS_ID_BIAS)
	#define CTL_MRPTABFLT_FLAGS          (7 + WINDOWS_ID_BIAS)
#define DLG_MRPLINEFLT            651 // @v4.6.6 Фильтр по строкам MRP-таблицы
	#define CTL_MRPLINEFLT_KIND          (1 + WINDOWS_ID_BIAS)
	#define CTL_MRPLINEFLT_FLAGS         (2 + WINDOWS_ID_BIAS)
	#define CTL_MRPLINEFLT_TERM          (3 + WINDOWS_ID_BIAS)
#define DLG_MRPTAB                652 // @v4.6.6 MRP-таблица
	#define CTL_MRPTAB_NAME              (1 + WINDOWS_ID_BIAS)
	#define CTL_MRPTAB_ID                (2 + WINDOWS_ID_BIAS)
	#define CTL_MRPTAB_LINKOBJ           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_MRPTAB_LINKOBJ        (4 + WINDOWS_ID_BIAS)
	#define CTL_MRPTAB_LINKOBJNAME       (5 + WINDOWS_ID_BIAS)
	#define CTL_MRPTAB_DATE              (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_MRPTAB_DATE           (7 + WINDOWS_ID_BIAS)
	#define CTL_MRPTAB_LOC               (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_MRPTAB_LOC            (9 + WINDOWS_ID_BIAS)
	#define CTL_MRPTAB_PARENTID          (10 + WINDOWS_ID_BIAS)
#define DLG_MRPLNTOTAL            653 // Итоги по строкам MRP-таблицы
	#define CTL_MRPLNTOTAL_COUNT         (1 + WINDOWS_ID_BIAS)
	#define CTL_MRPLNTOTAL_INDEPCNT      (2 + WINDOWS_ID_BIAS)
	#define CTL_MRPLNTOTAL_DEPCNT        (3 + WINDOWS_ID_BIAS)
	#define CTL_MRPLNTOTAL_TERMCNT       (4 + WINDOWS_ID_BIAS)
	#define CTL_MRPLNTOTAL_DFCTCNT       (5 + WINDOWS_ID_BIAS)
#define DLG_BILL2MRP              654 // Создание MRP-таблицы из документов
	#define CTL_BILL2MRP_WHAT            (1 + WINDOWS_ID_BIAS)
	#define CTL_BILL2MRP_MRPNAME         (2 + WINDOWS_ID_BIAS)
#define DLG_TODO                  655 // Задача
	#define cmTm                         101
	#define CTL_TODO_PARENTNAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_TODO_TEMPLATENAME        (2 + WINDOWS_ID_BIAS)
	#define CTL_TODO_CODE                (3 + WINDOWS_ID_BIAS)
	#define CTL_TODO_DT                  (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODO_DT               (5 + WINDOWS_ID_BIAS)
	#define CTL_TODO_START               (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODO_START            (7 + WINDOWS_ID_BIAS)
	#define CTL_TODO_ESTFINISH           (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODO_ESTFINISH        (9 + WINDOWS_ID_BIAS)
	#define CTL_TODO_FINISH              (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODO_FINISH           (11 + WINDOWS_ID_BIAS)
	#define CTL_TODO_CREATOR             (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_CREATOR          (13 + WINDOWS_ID_BIAS)
	#define CTL_TODO_CLIENT              (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_CLIENT           (15 + WINDOWS_ID_BIAS)
	#define CTL_TODO_EMPLOYER            (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_EMPLOYER         (17 + WINDOWS_ID_BIAS)
	#define CTL_TODO_PRIOR               (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_PRIOR            (19 + WINDOWS_ID_BIAS)
	#define CTL_TODO_STATUS              (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_STATUS           (21 + WINDOWS_ID_BIAS)
	#define CTL_TODO_DESCR               (22 + WINDOWS_ID_BIAS)
	#define CTL_TODO_DLVRADDR            (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_DLVRADDR         (24 + WINDOWS_ID_BIAS)
	#define CTL_TODO_TM                  (25 + WINDOWS_ID_BIAS)
	#define CTL_TODO_STARTTM             (26 + WINDOWS_ID_BIAS)
	#define CTL_TODO_ESTFINISHTM         (27 + WINDOWS_ID_BIAS)
	#define CTL_TODO_FINISHTM            (28 + WINDOWS_ID_BIAS)
	#define CTL_TODO_MEMO                (29 + WINDOWS_ID_BIAS)
	#define CTL_TODO_TEMPLATE            (30 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_TEMPLATE         (31 + WINDOWS_ID_BIAS)
	#define CTL_TODO_AMOUNT              (32 + WINDOWS_ID_BIAS)
	#define CTL_TODO_PRJ                 (33 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_PRJ              (34 + WINDOWS_ID_BIAS)
	#define CTL_TODO_PHASE               (35 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_PHASE            (36 + WINDOWS_ID_BIAS)
	#define CTL_TODO_LINKTASK            (37 + WINDOWS_ID_BIAS)
	#define CTL_TODO_BILLAR              (38 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODO_BILLAR           (39 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODO_TM               (40 + WINDOWS_ID_BIAS)
	#define CTLTM_TODO_STARTTM           (41 + WINDOWS_ID_BIAS)
	#define CTLTM_TODO_ESTFINISHTM       (42 + WINDOWS_ID_BIAS)
	#define CTLTM_TODO_FINISHTM          (43 + WINDOWS_ID_BIAS)
	#define CTL_TODO_ID                  (44 + WINDOWS_ID_BIAS)
#define DLG_TODOFILT              656 // Фильтр по задачам
	#define CTL_TODOFILT_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODOFILT_PERIOD       (2 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_START           (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODOFILT_START        (4 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_ESTFINISH       (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODOFILT_ESTFINISH    (6 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_FINISH          (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_TODOFILT_FINISH       (8 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_CREATOR         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_CREATOR      (10 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_CLIENT          (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_CLIENT       (12 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_EMPLOYER        (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_EMPLOYER     (14 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_PRIOR           (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_PRIOR        (16 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_STATUS          (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_STATUS       (18 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_KIND            (19 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_FLAGS           (20 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_CITY            (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_CITY         (22 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_ORDER           (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_ORDER        (24 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_CROSSTAB        (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_CROSSTAB     (26 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_CTPAR           (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_CTPAR        (28 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_TEMPLATE        (29 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_TEMPLATE     (30 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_SUBST           (31 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_SUBST        (32 + WINDOWS_ID_BIAS)
	#define CTL_TODOFILT_PRJ             (33 + WINDOWS_ID_BIAS)
	#define CTLSEL_TODOFILT_PRJ          (34 + WINDOWS_ID_BIAS)
//                                657 // Occupied (DLG_PALMGROUP)
#define DLG_SELNEWPALM            658 // Выбор вида новой записи для StyloPalm (устройство или группа)
	#define CTL_SELNEWPALM_WHAT          (1 + WINDOWS_ID_BIAS)
#define DLG_TODOTEMPL             659 // Шаблон задачи (CTL_TODO_XXX)
	// @v11.0.4 (moved up) #define cmRepeating              2002 // Вызов диалога повторений задания //
#define DLG_REPEATING             660 // Повторение по дням
	#define CTL_REPEATING_PRD            (1 + WINDOWS_ID_BIAS)  // Период (dayly, weekly, monthly, annualy)
	#define CTL_REPEATING_NUMPRD         (2 + WINDOWS_ID_BIAS)  // Количество периодов
	#define CTL_REPEATING_ST_NUMPRD      (3 + WINDOWS_ID_BIAS)  //
	#define CTL_REPEATING_WEEKDAYS       (4 + WINDOWS_ID_BIAS)  // checkboxes
	#define CTL_REPEATING_KIND           (5 + WINDOWS_ID_BIAS)  // Вариант периодичности
	#define CTL_REPEATING_DTLMD          (6 + WINDOWS_ID_BIAS)  // Число месяца [1..31]
	#define CTL_REPEATING_DTLWN          (7 + WINDOWS_ID_BIAS)  // Выбор номера недели (1,2,3,4,last(5))
	#define CTLSEL_REPEATING_DTLWN       (8 + WINDOWS_ID_BIAS)  // Выбор номера недели (1,2,3,4,last(5))
	#define CTL_REPEATING_DTLWD          (9 + WINDOWS_ID_BIAS)  // Выбор дня недели
	#define CTLSEL_REPEATING_DTLWD       (10 + WINDOWS_ID_BIAS) // Выбор дня недели
	#define CTL_REPEATING_DTLMON         (11 + WINDOWS_ID_BIAS) // Выбор месяца
	#define CTLSEL_REPEATING_DTLMON      (12 + WINDOWS_ID_BIAS) // Выбор месяца
	#define CTL_REPEATING_RUNTIME        (13 + WINDOWS_ID_BIAS)
	#define CTL_REPEATING_NUMSECS        (14 + WINDOWS_ID_BIAS) // Период в секундах
	#define CTL_REPEATING_DURATION       (15 + WINDOWS_ID_BIAS) // Продолжительность в секундах
	#define CTL_REPEATING_DTLAS          (16 + WINDOWS_ID_BIAS) // Дату следующего исполнения считать от даты начала/завершения предыдущего
	#define CTL_REPEATING_DTLNM          (17 + WINDOWS_ID_BIAS) // Номер месяца для ежемесячных периодов, кратных нескольким месяцам
	#define CTLTM_REPEATING_RUNTIME      (18 + WINDOWS_ID_BIAS)
#define DLG_REP_DAYLY             661 // Subdialog of DLG_REPEATING (CTL_REPEATING_XXX)
#define DLG_REP_WEEKLY            662 // Subdialog of DLG_REPEATING (CTL_REPEATING_XXX)
#define DLG_REP_MONTHLY           663 // Subdialog of DLG_REPEATING (CTL_REPEATING_XXX)
#define DLG_REP_ANN               664 // Subdialog of DLG_REPEATING (CTL_REPEATING_XXX)
#define DLG_SPIICFG               665 // Конфигурация обмена со StyloPalm
	#define CTL_SPIICFG_SELLTERM         (1 + WINDOWS_ID_BIAS)
	#define CTL_SPIICFG_SELLOP           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPIICFG_SELLOP        (3 + WINDOWS_ID_BIAS)
	#define CTL_SPIICFG_CLIINVOP         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPIICFG_CLIINVOP      (5 + WINDOWS_ID_BIAS)
	#define CTL_SPIICFG_ORDCFMTTYP       (6 + WINDOWS_ID_BIAS)
	#define CTL_SPIICFG_INHBTAG          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPIICFG_INHBTAG       (8 + WINDOWS_ID_BIAS)
//                                666 // Occupied (DLG_GSTRUCFOLDER)
#define DLG_PRC                   667 // Процессор
#define DLG_PRCGROUP              668 // Группа процессоров
	#define cmPrcExt                     102
	#define CTL_PRC_NAME                 (1 + WINDOWS_ID_BIAS)
	#define CTL_PRC_ID                   (2 + WINDOWS_ID_BIAS)
	#define CTL_PRC_PARENT               (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_PARENT            (4 + WINDOWS_ID_BIAS)
	#define CTL_PRC_LOC                  (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_LOC               (6 + WINDOWS_ID_BIAS)
	#define CTL_PRC_ACCSHEET             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_ACCSHEET          (8 + WINDOWS_ID_BIAS)
	#define CTL_PRC_ACCSHEET2            (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_ACCSHEET2         (10 + WINDOWS_ID_BIAS)
	#define CTL_PRC_WROFFOP              (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_WROFFOP           (12 + WINDOWS_ID_BIAS)
	#define CTL_PRC_WROFFAR              (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_WROFFAR           (14 + WINDOWS_ID_BIAS)
	#define CTL_PRC_SUPERSESSCONT        (15 + WINDOWS_ID_BIAS)
	#define CTL_PRC_GRP_FLAGS            (16 + WINDOWS_ID_BIAS)
	#define CTL_PRC_RESTGGRP             (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_RESTGGRP          (18 + WINDOWS_ID_BIAS)
	#define CTL_PRC_PRINTER              (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_PRINTER           (20 + WINDOWS_ID_BIAS)
	//#define CTL_PRC_PRINTNEWLINE         (21 + WINDOWS_ID_BIAS)
	#define CTL_PRC_LABELCOUNT           (22 + WINDOWS_ID_BIAS)
	#define CTL_PRC_SYMB                 (23 + WINDOWS_ID_BIAS)
	#define CTL_PRC_PANE_FLAGS           (24 + WINDOWS_ID_BIAS)
	#define CTL_PRC_FLAGS                (25 + WINDOWS_ID_BIAS)
	#define CTL_PRC_WROFFDT              (26 + WINDOWS_ID_BIAS)
	#define CTL_PRC_WROFFDT_BYSUPER      (27 + WINDOWS_ID_BIAS)
	#define CTL_PRC_ASSOC                (28 + WINDOWS_ID_BIAS)
	#define CTL_PRC_ASSOCGROUP           (29 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_ASSOCGROUP        (30 + WINDOWS_ID_BIAS)
	#define CTL_PRC_LINKOBJNAME          (32 + WINDOWS_ID_BIAS)
	#define CTL_PRC_LINKOBJ              (33 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_LINKOBJ           (34 + WINDOWS_ID_BIAS)
	#define CTL_PRC_SRVJOBSYMB           (35 + WINDOWS_ID_BIAS)
	#define CTL_PRC_CIPMAX               (36 + WINDOWS_ID_BIAS)
	#define CTL_PRC_CIPKIND              (37 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRC_CIPKIND           (38 + WINDOWS_ID_BIAS)
	#define CTL_PRC_TCBQUANT             (39 + WINDOWS_ID_BIAS)
#define DLG_PRCFILT               669 // Фильтр процессоров
	#define CTL_PRCFILT_KIND             (1 + WINDOWS_ID_BIAS)
	#define CTL_PRCFILT_PARENT           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCFILT_PARENT        (3 + WINDOWS_ID_BIAS)
	#define CTL_PRCFILT_LOC              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCFILT_LOC           (5 + WINDOWS_ID_BIAS)
#define DLG_TECH                  670 // TECH Технология //
#define DLG_TECHAUTO             1045 // TECHAUTO Автотехнология //
	#define CTL_TECH_ID                  (1 + WINDOWS_ID_BIAS)
	#define CTL_TECH_CODE                (2 + WINDOWS_ID_BIAS)
	#define CTL_TECH_PRCGRPONLY          (3 + WINDOWS_ID_BIAS)
	#define CTL_TECH_PRC                 (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_PRC              (5 + WINDOWS_ID_BIAS)
	#define CTL_TECH_GGRP                (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_GGRP             (7 + WINDOWS_ID_BIAS)
	#define CTL_TECH_GOODS               (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_GOODS            (9 + WINDOWS_ID_BIAS)
	#define CTL_TECH_GSTRUC              (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_GSTRUC           (11 + WINDOWS_ID_BIAS)
	#define CTL_TECH_SIGN                (12 + WINDOWS_ID_BIAS)
	#define CTL_TECH_CAPACITY            (13 + WINDOWS_ID_BIAS)
	#define CTL_TECH_TIMING              (14 + WINDOWS_ID_BIAS)
	#define CTL_TECH_MEMO                (15 + WINDOWS_ID_BIAS)
	#define CTL_TECH_ROUNDING            (16 + WINDOWS_ID_BIAS)
	#define CTL_TECH_FLAGS               (17 + WINDOWS_ID_BIAS)
	#define CTL_TECH_PREVGGRP            (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_PREVGGRP         (19 + WINDOWS_ID_BIAS)
	#define CTL_TECH_PREVGOODS           (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_PREVGOODS        (21 + WINDOWS_ID_BIAS)
	#define CTL_TECH_TIME                (22 + WINDOWS_ID_BIAS)
	#define CTL_TECH_TRANSCOND           (23 + WINDOWS_ID_BIAS)
	#define CTL_TECH_INITQTTY            (24 + WINDOWS_ID_BIAS) // float
	#define CTL_TECH_CAPACITYFORM        (25 + WINDOWS_ID_BIAS)
	#define CTL_TECH_PARENT              (26 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECH_PARENT           (27 + WINDOWS_ID_BIAS)
	#define CTL_TECH_ORDERN              (28 + WINDOWS_ID_BIAS)
	#define CTL_TECH_CIPMAX              (29 + WINDOWS_ID_BIAS)
#define DLG_TECHFILT              671 // Фильтр технологий
	#define CTL_TECHFILT_PRCGRPONLY      (1 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_PRC             (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECHFILT_PRC          (3 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_GGRP            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECHFILT_GGRP         (5 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_GOODS           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECHFILT_GOODS        (7 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_FLAGS           (8 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_SIGN            (9 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_KIND            (10 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_PARENT          (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_TECHFILT_PARENT       (12 + WINDOWS_ID_BIAS)
	#define CTL_TECHFILT_ORDER           (13 + WINDOWS_ID_BIAS)
#define DLG_PRJCFG                672 // Конфигурация проектов
	#define CTL_PRJCFG_PRJTEMPL          (1 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_PRJCNTR           (2 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_PHSTEMPL          (3 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_PHSCNTR           (4 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_TODOTEMPL         (5 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_TODOCNTR          (6 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_TODTTEMPL         (7 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_TODTCNTR          (8 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_NEWTASKTERM       (9 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_REJTASKTERM       (10 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_TEMPLGENTERM      (11 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_WORKHOURS         (12 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_REFRESHTIME       (13 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_FLAGS             (14 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_BILLOP            (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRJCFG_BILLOP         (16 + WINDOWS_ID_BIAS)
	#define CTL_PRJCFG_REMINDPRD         (17 + WINDOWS_ID_BIAS)
#define DLG_TSESS                 673 // Технологическая сессия //
	#define cmSelTechByGoods           101 // Выбор технологии по товару
	#define cmSelOrder                 102 // Выбор заказа
	#define cmFreeList                 103 // Показывает список доступных периодов процессора
	#define cmTSessDetail              104 // Строки сессии
	#define cmCreateGoods              105 // Создание товара (при выбранном процессоре и наличии автотехнологии)
	#define cmChkInP                   107 // Регистрация персоналий
	#define cmBrowseTSessByAr          108
	#define cmAddPaym                  109
	#define CTL_TSESS_NUMBER             (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ID                 (2 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_STATUS             (3 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PRC                (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESS_PRC             (5 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_TECH               (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESS_TECH            (7 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ST_GOODS           (8 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_OBJ                (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESS_OBJ             (10 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_OBJ2               (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESS_OBJ2            (12 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ST_EXTOBJNAME      (13 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_STDT               (14 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSESS_STDT            (15 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_STTM               (16 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_FNDT               (17 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSESS_FNDT            (18 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_FNTM               (19 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_CONTIN             (20 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PLANQTTY           (21 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PLANTIMING         (22 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ACTQTTY            (23 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ACTTIMING          (24 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_INCOMPL            (25 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_SUPERSES           (26 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESS_SUPERSES        (27 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_IDLE               (28 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_MEMO               (29 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ORDER              (30 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PLANPERIOD         (31 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSESS_PLANPERIOD      (32 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_AMOUNT             (33 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PRCGRPONLY         (34 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PLANFLAGS          (35 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_SCARD              (36 + WINDOWS_ID_BIAS)
	#define CTLTM_TSESS_STTM             (37 + WINDOWS_ID_BIAS)
	#define CTLTM_TSESS_FNTM             (38 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ST_CC              (39 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_ADDPAYMBUTT        (40 + WINDOWS_ID_BIAS)
	#define CTL_TSESS_PAYMINFO           (41 + WINDOWS_ID_BIAS)

#define DLG_TSESSLN               674 // Строка технологической сессии
	#define CTL_TSESSLN_SESSID           (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_OPRNO            (2 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_GGRP             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSLN_GGRP          (4 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_GOODS            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSLN_GOODS         (6 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_SIGN             (7 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_DT               (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSESSLN_DT            (9 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_TM               (10 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_QTTY             (11 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_SERIAL           (12 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_SESSTITLE        (13 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_PHQTTY           (14 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_PRICE            (15 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_FLAG_REST        (16 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_ST_PHQTTY        (17 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_INDEPPHQTTY      (18 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_PLANFLAGS        (19 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_DSCNT            (20 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_NETPRICE         (21 + WINDOWS_ID_BIAS)
	#define CTLMNU_TSESSLN_QTTY          (22 + WINDOWS_ID_BIAS)
	#define CTLMNU_TSESSLN_PHQTTY        (23 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLN_LOTDIMX          (24 + WINDOWS_ID_BIAS) // @v11.0.4
	#define CTL_TSESSLN_LOTDIMY          (25 + WINDOWS_ID_BIAS) // @v11.0.4
	#define CTL_TSESSLN_LOTDIMZ          (26 + WINDOWS_ID_BIAS) // @v11.0.4
	#define CTL_TSESSLN_ORGQTTY          (27 + WINDOWS_ID_BIAS) // @v11.0.7
#define DLG_TSESSFILT             675 // Фильтр по технологическим сессиям
	#define CTL_TSESSFILT_PRC            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSFILT_PRC         (2 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_TECH           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSFILT_TECH        (4 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_ST_GOODS       (5 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_AR             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSFILT_AR          (7 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_AR2            (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSFILT_AR2         (9 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_STATUS         (10 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_STPERIOD       (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSESSFILT_STPERIOD    (12 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_FNPERIOD       (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSESSFILT_FNPERIOD    (14 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_ST_AR2TEXT     (15 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_FLAGS          (16 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_IDLE           (17 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_PRCGRPONLY     (18 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_STTIME         (19 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_FNTIME         (20 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_TECBYGBUT      (21 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_ORDER          (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSFILT_ORDER       (23 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_WROFF          (24 + WINDOWS_ID_BIAS)
	#define CTL_TSESSFILT_GDSGRP         (25 + WINDOWS_ID_BIAS) // @v11.7.8
	#define CTLSEL_TSESSFILT_GDSGRP      (26 + WINDOWS_ID_BIAS) // @v11.7.8
#define DLG_RMVTSESS              676 // Удаление техн сессии
	#define CTL_RMVTSESS_WHAT            (1 + WINDOWS_ID_BIAS)
#define DLG_GSCOPY                677 // Выбор структуры товара для копирования //
	#define CTL_GSCOPY_GGRP              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSCOPY_GGRP           (2 + WINDOWS_ID_BIAS)
	#define CTL_GSCOPY_GOODS             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSCOPY_GOODS          (4 + WINDOWS_ID_BIAS)
	#define CTL_GSCOPY_GSTRUC            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSCOPY_GSTRUC         (6 + WINDOWS_ID_BIAS)
#define DLG_TSES_SUP              678 // Супесессия //
	// CTL[SEL]_TSESS_XXX
#define DLG_CAPACITY              679 // Установка величины производительности
	#define CTL_CAPACITY_VAL             (1 + WINDOWS_ID_BIAS)
	#define CTL_CAPACITY_TIMEUNIT        (2 + WINDOWS_ID_BIAS)
	#define CTL_CAPACITY_REVERSE         (3 + WINDOWS_ID_BIAS)
#define DLG_G2OA                  680 // Ассоциая товар-объект
	#define CTL_G2OA_GOODSGRP            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_G2OA_GOODSGRP         (2 + WINDOWS_ID_BIAS)
	#define CTL_G2OA_GOODS               (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_G2OA_GOODS            (4 + WINDOWS_ID_BIAS)
	#define CTL_G2OA_OBJ                 (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_G2OA_OBJ              (6 + WINDOWS_ID_BIAS)
	#define CTL_G2OA_ASSOC               (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_G2OA_ASSOC            (8 + WINDOWS_ID_BIAS)
//                                681 // Occupied(DLG_OPKMORE_MDF)
//                                682 // Occupied(DLG_GQUOTSC)
#define DLG_TSESSWROFFORD         683 // Порядок списания техн сессий
	#define CTL_TSESSWROFFORD_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_TSESWOORDI            684 // Элемент порядка списания техн сессий
	#define CTL_TSESWOORDI_PRCGRP        (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESWOORDI_PRC           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESWOORDI_PRC        (3 + WINDOWS_ID_BIAS)
#define DLG_RCVRGGRP              685 // Корректировка товарных групп
	#define CTL_RCVRGGRP_FLAGS           (1 + WINDOWS_ID_BIAS)
	#define CTL_RCVRGGRP_LOG             (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_RCVRGGRP_LOG          (3 + WINDOWS_ID_BIAS)
	#define CTL_RCVRGGRP_EGA             (4 + WINDOWS_ID_BIAS)
	#define CTL_RCVRGGRP_EGAFOLD         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_RCVRGGRP_EGAFOLD      (6 + WINDOWS_ID_BIAS)
#define DLG_INPUT                 686 // Простой диалог, содержащий текстовое поле ввода
	#define CTL_INPUT_STR                (1 + WINDOWS_ID_BIAS)
#define DLG_TSESSCFG              687 // Конфигурация технологических сессий
	#define cmTSessStatusColors          110
	#define cmTSSetAutoSmsParam          111
	#define CTL_TSESSCFG_FLAGS           (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_IDLEAS          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCFG_IDLEAS       (3 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_MINIDLE         (4 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_INITTIME        (5 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_ROUNDPERIOD     (6 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_VIEWREFRESH     (7 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_TCBQUANT        (8 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCFG_DEFTMTEC        (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCFG_DEFTMTEC     (10 + WINDOWS_ID_BIAS)
#define DLG_DEBTTOC               688 // Цикличность фильтра долговой ведомости
	#define CTL_DEBTTOC_KIND             (31 + WINDOWS_ID_BIAS) // Не должен перекрываться с DLG_SLLTOFLT
#define DLG_DEBTTOC_DELAY         689 // Внутренний диалог DLG_DEBTTOC
	#define CTL_DEBTTOC_CYCLE            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTTOC_CYCLE         (2 + WINDOWS_ID_BIAS)
	#define CTL_DEBTTOC_NUMCYCLES        (3 + WINDOWS_ID_BIAS)
#define DLG_DEBTTOC_CYCLE         690 // Внутренний диалог DLG_DEBTTOC
//	#define CTL_DEBTTOC_CYCLE            (1 + WINDOWS_ID_BIAS)
//	#define CTLSEL_DEBTTOC_CYCLE         (2 + WINDOWS_ID_BIAS)
//	#define CTL_DEBTTOC_NUMCYCLES        (3 + WINDOWS_ID_BIAS)
#define DLG_TSESANLZFILT          691 // Фильтр анализа технологических сессий
	#define CTL_TSESANLZFILT_FLAGS       (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_WHAT        (2 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_NMG         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESANLZFILT_NMG      (4 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_SUBS        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESANLZFILT_SUBS     (6 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_DIFPRC      (7 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_DIFMG       (8 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_DIFDT       (9 + WINDOWS_ID_BIAS)
	#define CTL_TSESANLZFILT_NMGOPT      (10 + WINDOWS_ID_BIAS)
#define DLG_DEBTTEXT              692 // Расширение фильтра долговой ведомости
	#define CTL_DEBTTEXT_KIND            (1 + WINDOWS_ID_BIAS)
	#define CTL_DEBTTEXT_EXPIRYTERM      (2 + WINDOWS_ID_BIAS)
	#define CTL_DEBTTEXT_MINDEBTPART     (3 + WINDOWS_ID_BIAS)
#define DLG_TAC                   693 // sub DLG_TRFRANLZ параметры кросстабуляции
	#define CTL_TAC_KIND                 (1 + WINDOWS_ID_BIAS)
	#define CTL_TAC_VALLIST              (2 + WINDOWS_ID_BIAS)
	#define CTL_TAC_SHOWVAL              (3 + WINDOWS_ID_BIAS)
//                                694 // @occupied DLG_SELLITEMDRAFT
#define DLG_TODOTOTAL             695 // Итоги по выборке задач
	#define CTL_TODOTOTAL_COUNT          (1 + WINDOWS_ID_BIAS)
#define DLG_DYNOBJSVIEW           696 // Динамические объекты
//                                697 // @occupied DLG_GDSCLSPROP2
#define DLG_GDSCLSVIEW            698 // Классы товаров
#define DLG_JOBPOOL               699 // Пул серверных заданий
	#define CTL_JOBPOOL_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_JOBPOOL_FLAGS            (2 + WINDOWS_ID_BIAS)
//                                700 // @occupied DLG_BUCFG_EDIT_JOB
#define DLG_SLVPL                 701 // Список поправок объемов бревен
	#define CTL_SLVPL_LIST               (1 + WINDOWS_ID_BIAS)
#define DLG_SLVPI                 702 // Поправка объема бревен
	#define CTL_SLVPI_BREED              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SLVPI_BREED           (2 + WINDOWS_ID_BIAS)
	#define CTL_SLVPI_DIAM               (3 + WINDOWS_ID_BIAS)
	#define CTL_SLVPI_PERMIT             (4 + WINDOWS_ID_BIAS)
	#define CTL_SLVPI_VOL                (5 + WINDOWS_ID_BIAS)
#define DLG_DELTMP                703 // Очистка временных файлов
	#define CTL_DELTMP_FILES             (1 + WINDOWS_ID_BIAS)
	#define CTL_DELTMP_INCHECK           (2 + WINDOWS_ID_BIAS)
	#define CTL_DELTMP_OUTCHECK          (3 + WINDOWS_ID_BIAS)
	#define CTL_DELTMP_INDAYS            (4 + WINDOWS_ID_BIAS)
	#define CTL_DELTMP_OUTDAYS           (5 + WINDOWS_ID_BIAS)
#define DLG_MAKENEWDB             704 // Создание новой базы данных
	#define cmPathBrowse                 101
	#define CTL_MAKENEWDB_TYPE           (1 + WINDOWS_ID_BIAS)
	#define CTL_MAKENEWDB_PATH           (2 + WINDOWS_ID_BIAS)
	#define CTL_MAKENEWDB_NAME           (3 + WINDOWS_ID_BIAS)
	#define CTL_MAKENEWDB_SYMBOL         (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_MAKENEWDB_PATH        (5 + WINDOWS_ID_BIAS)
	#define CTL_MAKENEWDB_POSHOST        (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAKENEWDB_POSHOST     (7 + WINDOWS_ID_BIAS)
#define DLG_REGISTRATION          705 // Регистрация лицензии
	#define CTL_REGISTRATION_NAME        (1 + WINDOWS_ID_BIAS)
	#define CTL_REGISTRATION_SERIAL      (2 + WINDOWS_ID_BIAS)
	#define CTL_REGISTRATION_COUNT       (3 + WINDOWS_ID_BIAS)
	#define CTL_REGISTRATION_LICFILE     (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_REGISTRAT_LICFILE     (5 + WINDOWS_ID_BIAS)
#define DLG_LICUPD                706 // Обновление лицензии
	//  CTL_REGISTRATION_XXX
#define DLG_EXTTDEBT              707 // EXTTDEBT Итог долговой ведомости
#define DLG_DEBTTOTAL01          1042 // T_DEBT01 Итог по спец форме долговой ведомости (доля просроченных долгов)
	#define CTL_EXTTDEBT_SELL            (1 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_PAYM            (2 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_DEBT            (3 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_RECKON          (4 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_RPAYM           (5 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_RDEBT           (6 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_TDEBT           (7 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_CUR             (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTTDEBT_CUR          (9 + WINDOWS_ID_BIAS)
	#define CTL_EXTTDEBT_COUNT           (10 + WINDOWS_ID_BIAS) // Количество строк
	#define CTL_EXTTDEBT_EXPDEBT         (11 + WINDOWS_ID_BIAS) // Просроченная задолженность
	#define CTL_EXTTDEBT_EXPDEBTPART     (12 + WINDOWS_ID_BIAS) // Доля просроченной задолженности
#define DLG_GBASKETN              708
	#define CTL_GBASKETN_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_GBASKETN_ID              (2 + WINDOWS_ID_BIAS)
	#define CTL_GBASKETN_DEFAULTGB       (3 + WINDOWS_ID_BIAS)
#define DLG_GOODSBASKETVIEW       709
#define DLG_PSALESFLT             710  // F_PSALES Фильтр статистика продаж по товару
	#define CTL_PSALESFLT_GOODS          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSALESFLT_GOODS       (2 + WINDOWS_ID_BIAS)
	#define CTL_PSALESFLT_PERIOD         (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_PSALESFLT_PERIOD      (4 + WINDOWS_ID_BIAS)
	#define CTL_PSALESFLT_GGROUP         (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSALESFLT_GGROUP      (7 + WINDOWS_ID_BIAS)
	#define CTL_PSALESFLT_FLAGS          (8 + WINDOWS_ID_BIAS)
	#define CTL_PSALESFLT_CYCLE          (9 + WINDOWS_ID_BIAS)
	#define CTL_PSALESFLT_WATERSHED      (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_PSALESFLT_WATERSHED   (11 + WINDOWS_ID_BIAS)
	#define CTL_PSALESFLT_LOC            (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSALESFLT_LOC         (13 + WINDOWS_ID_BIAS)
#define DLG_PROGNOSISFLT          711
	#define CTL_PROGNOSISFLT_PRDPRGN     (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_PROGNOSISFLT_PRDPRGN  (2 + WINDOWS_ID_BIAS)
	#define CTL_PROGNOSISFLT_GID         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PROGNOSISFLT_GID      (4 + WINDOWS_ID_BIAS)
#define DLG_PREDICTCFG            712
	#define CTL_PREDICTCFG_OPRS          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PREDICTCFG_OPRS       (2 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_DTBEGSELL     (3 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_P             (4 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_Q             (5 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_METHOD        (6 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_TRUST         (7 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_OPRPCH        (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_PREDICTCFG_OPRPCH     (9 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_IMPOSINGD     (10 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_FLAGS         (11 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_DTENDCALC     (12 + WINDOWS_ID_BIAS)
	#define CTLCAL_PREDICTCFG_DTBEGSELL  (13 + WINDOWS_ID_BIAS) // @alena
	#define CTL_PREDICTCFG_USEPCKG       (14 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_ROUNDPCKG     (15 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_DINSTOCK      (16 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_ST_CONT       (17 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_PMIN          (18 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_CKOEFF        (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_PREDICTCFG_CKOEFF     (20 + WINDOWS_ID_BIAS)
	#define CTL_PREDICTCFG_FIXARCODE     (21 + WINDOWS_ID_BIAS)
#define DLG_VIEWPRGN              713
	#define CTL_VIEWPRGN_PROGNOSIS       (1 + WINDOWS_ID_BIAS)
	#define CTL_VIEWPRGN_PRDPRGN         (2 + WINDOWS_ID_BIAS)
	#define CTL_VIEWPRGN_GOODS           (3 + WINDOWS_ID_BIAS)
#define DLG_ARTICLEFLT            714
	#define CTL_ARTICLEFLT_ACCSID        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_ARTICLEFLT_ACCSID     (2 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLEFLT_ORDER         (3 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLEFLT_FTCLOSED      (4 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLEFLT_FTSTOP        (5 + WINDOWS_ID_BIAS)
	#define CTL_ARTICLEFLT_FLAGS         (6 + WINDOWS_ID_BIAS)
#define DLG_EXPIRYREG             716
	#define CTL_EXPIRYREG_LIST           (1 + WINDOWS_ID_BIAS)
	#define CTL_EXPIRYREG_PSNTEXT        (2 + WINDOWS_ID_BIAS)
#define DLG_GRWPRGNFLT            717
	#define CTL_GRWPRGNFLT_PRGNPRD       (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_GRWPRGNFLT_PRGNPRD    (2 + WINDOWS_ID_BIAS)
	#define CTL_GRWPRGNFLT_DATE          (3 + WINDOWS_ID_BIAS)
	#define CTL_GRWPRGNFLT_GRPID         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GRWPRGNFLT_GRPID      (5 + WINDOWS_ID_BIAS)
	#define CTL_GRWPRGNFLT_SUPPL         (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_GRWPRGNFLT_SUPPL      (7 + WINDOWS_ID_BIAS)
	#define CTL_GRWPRGNFLT_FLAGS         (8 + WINDOWS_ID_BIAS)
	#define CTL_GRWPRGNFLT_FILLDATE      (9 + WINDOWS_ID_BIAS)
	#define CTL_GRWPRGNFLT_LOC           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_GRWPRGNFLT_LOC        (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_GRWPRGNFLT_DATE       (12 + WINDOWS_ID_BIAS) // @alena
	#define CTL_GRWPRGNFLT_EXHTERM       (13 + WINDOWS_ID_BIAS)
#define DLG_GBPRINT               718
	#define CTL_GBPRINT_REPORT           (1 + WINDOWS_ID_BIAS)
	#define CTL_GBPRINT_FLAGS            (2 + WINDOWS_ID_BIAS)
#define DLG_AUSERSLST             719 // AUSRSLST Список активных сеансов в базе данных
	#define cmUpdate               101
	#define CTL_AUSERSLST_LIST           (1 + WINDOWS_ID_BIAS)
	#define CTL_AUSERSLST_FLAGS          (2 + WINDOWS_ID_BIAS)
	#define CTL_AUSERSLST_USERSCOUNT     (3 + WINDOWS_ID_BIAS)
	#define CTL_AUSERSLST_UNIQMACS       (4 + WINDOWS_ID_BIAS)
#define DLG_ALBTRCFG2             720
	#define cmVetisConfig          101
	#define cmMqcConfig            102
	#define CTL_ALBTRCFG_MSRVADDR        (1 + WINDOWS_ID_BIAS) // not used since @v4.2.9
	#define CTL_ALBTRCFG_LOGIN           (2 + WINDOWS_ID_BIAS) // not used since @v4.2.9
	#define CTL_ALBTRCFG_OPKINDID        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_OPKINDID     (4 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_PASSWORD        (5 + WINDOWS_ID_BIAS) // not used since @v4.2.9
	#define CTL_ALBTRCFG_MAILADDR        (6 + WINDOWS_ID_BIAS) // not used since @v4.2.9
	#define CTL_ALBTRCFG_MAILACC         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_MAILACC      (8 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_UHTTURN         (9 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_UHTTURLPFX      (10 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_UHTTACCOUNT     (11 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_UHTTPASSW       (12 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_SMSACC	         (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_SMSACC       (14 + WINDOWS_ID_BIAS)

	#define CTL_ALBTRCFG_EDIORD          (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_EDIORD       (16 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_EDIORDSP        (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_EDIORDSP     (18 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_DESADV          (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_DESADV       (20 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_RCVTAG          (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_RCVTAG       (22 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_TTNTAG          (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_TTNTAG       (24 + WINDOWS_ID_BIAS)

	#define CTL_ALBTRCFG_EGAISURL        (25 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_EGRCPTOP        (26 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_EGRCPTOP     (27 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_FLAGS           (28 + WINDOWS_ID_BIAS)
	#define CTL_ALBTRCFG_EGRETOP         (29 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALBTRCFG_EGRETOP      (30 + WINDOWS_ID_BIAS)
#define DLG_CLINFO                721
	#define cmChangeINN               110
	#define CTL_CLINFO_CLNAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLBASETPID        (2 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLORDERTPID       (3 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLCOUNTRY         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLINFO_CLCOUNTRY      (5 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLCITY            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CLINFO_CLCITY         (7 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLADDR            (8 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLPHONE           (9 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_CLMAIL            (10 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_TEXT1             (11 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_TEXT2             (12 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_TEXT3             (13 + WINDOWS_ID_BIAS)
	#define CTL_CLINFO_TEXT4             (14 + WINDOWS_ID_BIAS)
#define DLG_SSTATFLT              722
	#define CTL_SSTATFLT_GGROUP          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SSTATFLT_GGROUP       (2 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_PERIOD          (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SSTATFLT_PERIOD       (4 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_ORDER           (6 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_LOC             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SSTATFLT_LOC          (8 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_SUBST           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SSTATFLT_SUBST        (10 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_CYCLE           (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_SSTATFLT_CYCLE        (12 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_NUMCYCLES       (13 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_SUPPL           (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_SSTATFLT_SUPPL        (15 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_DATE            (16 + WINDOWS_ID_BIAS)
	#define CTLCAL_SSTATFLT_DATE         (17 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_RESTDATE        (18 + WINDOWS_ID_BIAS)
	#define CTLCAL_SSTATFLT_RESTDATE     (19 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_ORDP            (20 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_DLVRP           (21 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_SORTORDER       (22 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_FLAGS2          (23 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_DEFINSSTOCK     (24 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_UPRESTR         (25 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_ARCODEUSAGE     (26 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_COUNT           (27 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_QTTYAVG         (28 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_AMTAVG          (29 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_QTTYVAR         (30 + WINDOWS_ID_BIAS)

	#define CTL_SSTATFLT_OVERRCFG        (31 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_METHOD          (32 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_USEPCKG         (33 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_ROUNDPCKG       (34 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_TRUST           (35 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_P               (36 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_CFLAGS          (37 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_PMIN            (38 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_MTX_IND         (39 + WINDOWS_ID_BIAS)
	#define CTL_SSTATFLT_PDC             (40 + WINDOWS_ID_BIAS)

#define DLG_ICONNCFG              723
	#define CTL_ICONNCFG_AGENT           (1 + WINDOWS_ID_BIAS)
	#define CTL_ICONNCFG_MAXTRIES        (2 + WINDOWS_ID_BIAS)
	#define CTL_ICONNCFG_PROXYHOST       (3 + WINDOWS_ID_BIAS)
	#define CTL_ICONNCFG_PROXYPORT       (4 + WINDOWS_ID_BIAS)
	#define CTL_ICONNCFG_ACCESSTYPE      (5 + WINDOWS_ID_BIAS)
	#define CTL_ICONNCFG_URLDIR          (6 + WINDOWS_ID_BIAS)
#define DLG_TLST2TLST             724
#define DLG_UICFG                 725
	#define cmSelBrowserFont             101
	#define cmSelListFont                102
	#define cmUiCfgBITF                  103 // @v11.5.11 Опции отображения строк документа
	#define CTL_UICFG_SELBCLRSHM         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_UICFG_SELBCLRSHM      (2 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_FLAGS              (3 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_LISTELEMCOUNT      (4 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_WNDVIEWKIND        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_UICFG_WNDVIEWKIND     (6 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_ST_TABLEFONT       (7 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_ST_LISTFONT        (8 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_SPCINPDRV          (9 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_MULTBILLPRINT      (10 + WINDOWS_ID_BIAS)
	#define CTL_UICFG_FLAGS2             (11 + WINDOWS_ID_BIAS) // @v12.4.9
#define DLG_SMPLLEDG              727
	#define CTL_SMPLLEDG_DT               (1 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_CODE             (2 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_OBJ              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SMPLLEDG_OBJ           (4 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_INCEXPTXT        (5 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_INCEXPVTXT       (6 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_INCEXPAMT        (7 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_INCEXPVAMT       (8 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_INCEXPSEL        (9 + WINDOWS_ID_BIAS)
	#define CTL_SMPLLEDG_FLAGS            (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_SMPLLEDG_DT            (11 + WINDOWS_ID_BIAS) // @alena
#define DLG_VATBCFG_SMPLLEDG      728
#define DLG_SLEDGTOTAL            729
	#define CTL_SLEDGTOTAL_COUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_SLEDGTOTAL_AMT            (2 + WINDOWS_ID_BIAS)
	#define CTL_SLEDGTOTAL_AMTV           (3 + WINDOWS_ID_BIAS)
	#define CTL_SLEDGTOTAL_EXP            (4 + WINDOWS_ID_BIAS)
	#define CTL_SLEDGTOTAL_EXPV           (5 + WINDOWS_ID_BIAS)
#define DLG_QUOTUPDA              730
	#define CTL_QUOTUPDA_KIND             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPDA_KIND          (2 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_LOC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPDA_LOC           (4 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_QUOT             (5 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_OP               (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPDA_OP            (7 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_FORMULA          (8 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_ACTION           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPDA_ACTION        (10 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_TOKEN            (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_QUOTUPDA_TOKEN         (12 + WINDOWS_ID_BIAS)
	#define CTL_QUOTUPDA_FLAGS            (13 + WINDOWS_ID_BIAS)
#define DLG_ABCANLZ               731
	#define CTL_ABCANLZ_GROUPBY           (1 + WINDOWS_ID_BIAS)
	#define CTL_ABCANLZ_FRSUM             (2 + WINDOWS_ID_BIAS)
	#define CTL_ABCANLZ_GRP1              (3 + WINDOWS_ID_BIAS)
	#define CTL_ABCANLZ_GRP2              (4 + WINDOWS_ID_BIAS)
	#define CTL_ABCANLZ_GRP3              (5 + WINDOWS_ID_BIAS)
	#define CTL_ABCANLZ_GRP4              (6 + WINDOWS_ID_BIAS)
	#define CTL_ABCANLZ_GRP5              (7 + WINDOWS_ID_BIAS)
#define DLG_ABCANLZPRN            732
	#define CTL_ABCANLZPRN_WHAT           (1 + WINDOWS_ID_BIAS)
#define DLG_GRPSEL                733
	#define CTL_GRPSEL_GROUP              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GRPSEL_GROUP           (2 + WINDOWS_ID_BIAS)
	#define CTL_GRPSEL_TXT                (3 + WINDOWS_ID_BIAS)
#define DLG_REGIONSEL             734 // REGNSEL Выбор опций для импорта банков (не удачное название для диалога)
	#define CTL_REGIONSEL_REGION          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_REGIONSEL_REGION       (2 + WINDOWS_ID_BIAS)
	#define CTL_REGIONSEL_CITY            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REGIONSEL_CITY         (4 + WINDOWS_ID_BIAS)
	#define CTL_REGIONSEL_FLAGS           (5 + WINDOWS_ID_BIAS)
#define DLG_DISCOUNT              735
	#define CTL_DISCOUNT_VALUE            (1 + WINDOWS_ID_BIAS)
#define DLG_GBDATAPRICE           736
// used DLG_GBDATA controls
#define DLG_EDITRHTS              737
	#define CTL_EDITRHTS_GRPUSRLIST       (1 + WINDOWS_ID_BIAS)
	#define CTL_EDITRHTS_EDITWHAT         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_EDITRHTS_EDITWHAT      (3 + WINDOWS_ID_BIAS)
	#define CTL_EDITRHTS_MSG              (4 + WINDOWS_ID_BIAS)
#define DLG_RHTSPRD               738
	#define CTL_RHTSPRD_RBILL             (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_RHTSPRD_RBILL          (2 + WINDOWS_ID_BIAS)
	#define CTL_RHTSPRD_WBILL             (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_RHTSPRD_WBILL          (4 + WINDOWS_ID_BIAS)
#define DLG_BUCHECK               739
	#define CTL_BUCHECK_LASTMODIF         (1 + WINDOWS_ID_BIAS)
	#define CTL_BUCHECK_SIZE              (2 + WINDOWS_ID_BIAS)
	#define CTL_BUCHECK_DIR               (3 + WINDOWS_ID_BIAS)
#define DLG_OPKINDFLT             740
	#define CTL_OPKINDFLT_TYPE            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINDFLT_TYPE         (2 + WINDOWS_ID_BIAS)
	#define CTL_OPKINDFLT_ACCSHT          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKINDFLT_ACCSHT       (4 + WINDOWS_ID_BIAS)
	#define CTL_OPKINDFLT_FLAGS           (5 + WINDOWS_ID_BIAS)
	#define CTL_OPKINDFLT_SORTORD         (6 + WINDOWS_ID_BIAS)
#define DLG_GRESTFLTA             741
	#define CTL_GRESTFLTA_AGENT           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GRESTFLTA_AGENT        (2 + WINDOWS_ID_BIAS)
	#define CTL_GRESTFLTA_DEFICDT         (3 + WINDOWS_ID_BIAS)
	#define CTL_GRESTFLTA_CALCDEFIC       (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_GRESTFLTA_DEFICDT      (5 + WINDOWS_ID_BIAS)
	#define CTL_GRESTFLTA_PRGNDAYS        (6 + WINDOWS_ID_BIAS)
	#define CTL_GRESTFLTA_UHTTEXPF        (7 + WINDOWS_ID_BIAS)
	#define CTL_GRESTFLTA_UHTTLOC         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_GRESTFLTA_UHTTLOC      (9 + WINDOWS_ID_BIAS)
	#define CTL_GRESTFLTA_EVF             (10 + WINDOWS_ID_BIAS)
#define DLG_GFEXTOPT              742 // GFEXTOPT
	#define CTL_GFEXTOPT_KIND             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFEXTOPT_KIND          (2 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_KIND           (3 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_ADDPROP          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFEXTOPT_ADDPROP       (5 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_ADDPROP        (6 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_GRADE            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFEXTOPT_GRADE         (8 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_GRADE          (9 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_DIMX             (10 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_DIMX           (11 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_DIMY             (12 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_DIMY           (13 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_DIMZ             (14 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_DIMZ           (15 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_GDSCLS           (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFEXTOPT_GDSCLS        (17 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_ADD2PROP         (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_GFEXTOPT_ADD2PROP      (19 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_ADD2PROP       (20 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_DIMW             (21 + WINDOWS_ID_BIAS)
	#define CTL_GFEXTOPT_L_DIMW           (22 + WINDOWS_ID_BIAS)

#define DLG_FLTAFINV                743
	#define CTL_FLTAFINV_GGRP             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTAFINV_GGRP          (2 + WINDOWS_ID_BIAS)
	#define CTL_FLTAFINV_METHOD           (3 + WINDOWS_ID_BIAS)
	#define CTL_FLTAFINV_FLAGS            (4 + WINDOWS_ID_BIAS)
	#define CTL_FLTAFINV_ST_INFO          (5 + WINDOWS_ID_BIAS)
#define DLG_DIFFCNTR                744
	#define cmEditLocCounter              101
	#define CTL_DIFFCNTR_LOCLIST          (1 + WINDOWS_ID_BIAS)
	#define CTL_DIFFCNTR_VAL              (2 + WINDOWS_ID_BIAS)
#define DLG_GDSMOVT                 745
	#define CTL_GDSMOVT_INRESTQTY         (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_INRESTPHQTY       (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_INRESTCOST        (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_INRESTPRICE       (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_OUTRESTQTY        (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_OUTRESTPHQTY      (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_OUTRESTCOST       (7 + WINDOWS_ID_BIAS)
	#define CTL_GDSMOVT_OUTRESTPRICE      (8 + WINDOWS_ID_BIAS)

// 801.. VADIM
#define DLG_SAVEDATA			  801
	#define CTL_SAVEDATA_REPERR          (1 + WINDOWS_ID_BIAS)
	#define CTL_SAVEDATA_SVDT            (2 + WINDOWS_ID_BIAS)
	#define CTL_SAVEDATA_SVDTPATH        (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_SAVEDATA_SVDTPATH     (4 + WINDOWS_ID_BIAS)
	#define CTL_SAVEDATA_EDREP           (5 + WINDOWS_ID_BIAS)
	#define CTL_SAVEDATA_EDREPPATH       (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_SAVEDATA_EDREPPTH     (7 + WINDOWS_ID_BIAS)
#define DLG_INLNCALC			  802
#define DLG_INLNCALC_L            895
	#define CTL_INLNCALC_0               (1 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_1               (2 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_2               (3 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_3               (4 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_4               (5 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_5               (6 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_6               (7 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_7               (8 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_8               (9 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_9               (10 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_DOT             (11 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_INVERSE         (12 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_CLEAR_ENTER     (13 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_CLEAR           (14 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_PLUS            (15 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_MINUS           (16 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_MULT            (17 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_DIVD            (18 + WINDOWS_ID_BIAS)
	#define CTL_INLNCALC_EQV             (19 + WINDOWS_ID_BIAS)
#define DLG_PFLTADVOPT            803
	#define CTL_PFLTADVOPT_REG           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PFLTADVOPT_REG        (2 + WINDOWS_ID_BIAS)
	#define CTL_PFLTADVOPT_SERIAL        (3 + WINDOWS_ID_BIAS)
	#define CTL_PFLTADVOPT_NUMBER        (4 + WINDOWS_ID_BIAS)
	#define CTL_PFLTADVOPT_REGPRD        (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_PFLTADVOPT_REGPRD     (6 + WINDOWS_ID_BIAS)
	#define CTL_PFLTADVOPT_EXPIRYPRD     (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_PFLTADVOPT_EXPIRYPRD  (8 + WINDOWS_ID_BIAS)
#define DLG_SELQUOT2              804
#define DLG_SELQUOT3              835 // Выбор вида котировки и параметров округления для расценки документа
	#define CTL_SELQUOT2_KIND            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELQUOT2_KIND         (2 + WINDOWS_ID_BIAS)
	#define CTL_SELQUOT2_ROUND           (3 + WINDOWS_ID_BIAS)
	#define CTL_SELQUOT2_PREC            (4 + WINDOWS_ID_BIAS)
	#define CTL_SELQUOT2_ROUNDVAT        (5 + WINDOWS_ID_BIAS)
	#define CTL_SELQUOT2_FLAGS_          (6 + WINDOWS_ID_BIAS)
	#define CTL_SELQUOT2_ACTION          (7 + WINDOWS_ID_BIAS)
#define DLG_RCVRRES               805
	#define CTL_RCVRRES_LIST             (1 + WINDOWS_ID_BIAS)
#define DLG_SCARDNUM              806
#define DLG_SCARDCHK              818
	#define CTL_SCARDNUM_SCARDNUM        (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDNUM_CHKDT           (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_SCARDNUM_CHKDT        (3 + WINDOWS_ID_BIAS)
	#define CTL_SCARDNUM_CHKNUM          (4 + WINDOWS_ID_BIAS)
	#define CTL_SCARDNUM_CASHNUM         (5 + WINDOWS_ID_BIAS)
#define DLG_SCARDINFO             807
	#define CTL_SCARDINFO_CODE           (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDINFO_NAME           (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARDINFO_PSW            (3 + WINDOWS_ID_BIAS)
	#define CTL_SCARDINFO_OP             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDINFO_OP          (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDINFO_ST_INFO        (6 + WINDOWS_ID_BIAS)
//                                808 // Occupied DLG_ARTICLEGROUP
//                                809 // Occupied DLG_SELSESSRNG
#define DLG_GDSSALDO              810
	#define CTL_GDSSALDO_GGRP            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSSALDO_GGRP         (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSSALDO_GOODS           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSSALDO_GOODS        (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSSALDO_CNTRAGNT        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSSALDO_CNTRAGNT     (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSSALDO_PERIOD          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSSALDO_PERIOD       (8 + WINDOWS_ID_BIAS)
	#define CTL_GDSSALDO_LASTCALC        (9 + WINDOWS_ID_BIAS)
	#define CTL_GDSSALDO_HOW             (10 + WINDOWS_ID_BIAS)
//                                811 // Occupied DLG_TRFRANLZTOTAL_S
//                                812 // Occupied DLG_BCPLABEL_EXT
#define DLG_GDSASSCINFO           813
	#define cmEditPLU                    101
	#define CTL_GDSASSCINFO_ID           (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSASSCINFO_LIST         (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSASSCINFO_GOODS        (3 + WINDOWS_ID_BIAS)
#define DLG_GDSFVOPT              814 // F_GVOPT Опции просмотра фильтра по товарам
	#define CTL_GDSFVOPT_FLAGS     (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSFVOPT_ACS       (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSFVOPT_ACS    (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSFVOPT_AR        (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSFVOPT_AR     (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSFVOPT_OWNCODES  (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSFVOPT_FASSOC    (7 + WINDOWS_ID_BIAS) // Флаг предписывающий показывать ассоциации товар-склад
	#define CTL_GDSFVOPT_ASSOC     (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSFVOPT_ASSOC  (9 + WINDOWS_ID_BIAS)
#define DLG_WARRANT               815 // Доверенность
	// CTL_BILL_XXX
#define DLG_WARRITEM              816
	#define CTL_WARRITEM_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_WARRITEM_UNIT            (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_WARRITEM_UNIT         (3 + WINDOWS_ID_BIAS)
	#define CTL_WARRITEM_QTTY            (4 + WINDOWS_ID_BIAS)
#define DLG_SCALE_EXPPATHS        817
	#define CTL_SCALE_EXPPATHS           (1 + WINDOWS_ID_BIAS)
//                                818 // Occupied DLG_SCARDCHK

#define DLG_CREATEDB              819 // CREATEDB
	#define CTL_CREATEDB_ENTRYNAME       (1 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_NAME            (2 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_DICT            (3 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_DATA            (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_CREATEDB_DICT         (5 + WINDOWS_ID_BIAS)
	#define CTLBRW_CREATEDB_DATA         (6 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_ADMINNAME       (7 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_ADMINPWD        (8 + WINDOWS_ID_BIAS)
	#define CTL_USEDTI                   (9 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_DBNAME          (10 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_DBUSER          (11 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_DBUSERPW        (12 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_SRVTYPE         (13 + WINDOWS_ID_BIAS)
	#define CTL_CREATEDB_SRVURL          (14 + WINDOWS_ID_BIAS)
#define DLG_DBLIST				  820
	#define CTL_DBLIST_DB					(1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBLIST_DB					(2 + WINDOWS_ID_BIAS)
#define DLG_EQUIPCFG              821
	#define CTL_EQCFG_PSNKNDCSHRS        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_PSNKNDCSHRS     (2 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_DEFCASHNODE        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_DEFCASHNODE     (4 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_SCALE              (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_SCALE           (6 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_FLAGS              (7 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_OPDTHISLOC         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_OPDTHISLOC      (9 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_OPDOTHRLOC         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_OPDOTHRLOC      (11 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_TEMPSESSOP         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_TEMPSESSOP      (13 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_QUOT               (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_QUOT            (15 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_AGENTCODELEN       (16 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_AGENTPREFIX        (17 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_FTPACCT            (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_FTPACCT         (19 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_SALESGRP           (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_SALESGRP        (21 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_DFCTCOSTRNG        (22 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_WROFFACCOP         (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_WROFFACCOP      (24 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_RNGLIMGOODSBHT     (25 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_FLAGS2             (26 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_RNGLIMPRICEBHT     (27 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_SUSPCPFX           (28 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_LOOKBKPRCPRD       (29 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_PHNSVC             (30 + WINDOWS_ID_BIAS)
	#define CTLSEL_EQCFG_PHNSVC          (31 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_CPIMPOP            (32 + WINDOWS_ID_BIAS) // @v11.8.6 input вид операции документов для импорта в кассовую панель
	#define CTLSEL_EQCFG_CPIMPOP         (33 + WINDOWS_ID_BIAS) // @v11.8.6 combo вид операции документов для импорта в кассовую панель
	#define CTL_EQCFG_CPIMPTAG           (34 + WINDOWS_ID_BIAS) // @v11.8.6 Тег документа, наличие которого свидетельствует о том, что документ может быть импортирован в кассовую панель
	#define CTLSEL_EQCFG_CPIMPTAG        (35 + WINDOWS_ID_BIAS)
	#define CTL_EQCFG_LOOKBKBILPRD       (36 + WINDOWS_ID_BIAS) // @v11.8.9 LookBackBillPeriod
	#define CTL_EQCFG_POSPAYMFLAGS       (37 + WINDOWS_ID_BIAS) // @v12.1.10 Часть флагов, отвечающих за расчет в кассовой панели
/* @v11.1.6
#define DLG_CBFLDS                822 //PLUS Fields copied from LST2LST
	#define CTL_CBFLDS_TYPE             (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_CBFLDS_TYPE          (8 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_SIZE             (9 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_DEC              (10 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_NAME             (11 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_FILENAME         (12 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_CODEPAGE         (13 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_FMT              (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_CBFLDS_FMT           (15 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_FLDDIV           (16 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_BANKCODE         (17 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_SECTNAME         (18 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_IMPEXP           (19 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_DEFVAL           (20 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_PARVALDIV        (21 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_HEADER           (22 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_END              (23 + WINDOWS_ID_BIAS)
	#define CTL_CBFLDS_PAYERBYSIGN      (24 + WINDOWS_ID_BIAS)
*/
#define DLG_CBASCFG               823
	#define CTL_CBASCFG_LIST            (1 + WINDOWS_ID_BIAS)
#define DLG_CBASITM               824 // CB_ASITM Элемент ассоциации импорта из систем клиент-банк
	#define CTL_CBASITM_ACC             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CBASITM_ACC          (2 + WINDOWS_ID_BIAS)
	#define CTL_CBASITM_SIGN            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CBASITM_SIGN         (4 + WINDOWS_ID_BIAS)
	#define CTL_CBASITM_OPRKIND         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CBASITM_OPRKIND      (6 + WINDOWS_ID_BIAS)
	#define CTL_CBASITM_ADDEDTAG        (7 + WINDOWS_ID_BIAS)
#define DLG_ASC_RESOLVE           825
	#define CTL_ASCRES_TXT              (1 + WINDOWS_ID_BIAS)
	#define CTL_ASCRES_OPRKIND          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASCRES_OPRKIND       (3 + WINDOWS_ID_BIAS)
	#define CTL_ASCRES_PERSON           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_ASCRES_PERSON        (5 + WINDOWS_ID_BIAS)
#define DLG_SELOGPRN              826
	#define CTL_SELOGPRN_SEL             (1 + WINDOWS_ID_BIAS)
#define DLG_CUSTDISP              827 // Дисплей покупателя //
	#define CTL_CUSTDISP_DEVICE         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CUSTDISP_DEVICE      (2 + WINDOWS_ID_BIAS)
	#define CTL_CUSTDISP_PORT           (3 + WINDOWS_ID_BIAS)
	#define CTL_CUSTDISP_TIMEOUT        (4 + WINDOWS_ID_BIAS)
	#define CTL_CUSTDISP_USB			(5 + WINDOWS_ID_BIAS)
#define DLG_DFRULE                828
	#define CTL_DFRULE_NAME             (1  + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_CASHNN           (2  + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_OPKIND           (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_OPKIND        (4  + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_OBJECT           (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_OBJECT        (6  + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_AGENT            (7  + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_AGENT         (8  + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_GGRP             (9  + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_GGRP          (10 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_PRICEALG         (11 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_COSTALG          (12 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_PQUOT            (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_PQUOT         (14 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_CQUOT            (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_CQUOT         (16 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_CPCTVAL          (17 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_EXCLGGRP         (18 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_MAXSUM           (19 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_MAXPOS           (20 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_RULEGRP          (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_RULEGRP       (22 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_PPCTVAL          (23 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_SCARDSER         (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULE_SCARDSER      (25 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_WOSCARD          (26 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_PAYTYPE          (27 + WINDOWS_ID_BIAS)
	#define CTL_DFRULE_RETSALTGGL       (28 + WINDOWS_ID_BIAS)
#define DLG_DFRULESEL             829 // DFRULSEL
	#define CTL_DFRULESEL_RULE         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULESEL_RULE      (2 + WINDOWS_ID_BIAS)
	#define CTL_DFRULESEL_FLAGS        (3 + WINDOWS_ID_BIAS)
	#define CTL_DFRULESEL_RULEGRP      (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULESEL_RULEGRP   (5 + WINDOWS_ID_BIAS)
	#define CTL_DFRULESEL_PERIOD       (6 + WINDOWS_ID_BIAS)
	#define CTLCAL_DFRULESEL_PERIOD    (7 + WINDOWS_ID_BIAS)
	#define CTL_DFRULESEL_NODE         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_DFRULESEL_NODE      (9 + WINDOWS_ID_BIAS)
	#define CTL_DFRULESEL_FLAGS2       (10 + WINDOWS_ID_BIAS)
#define DLG_DFRULEADDW            830
	#define CTL_DFRULEADDW_WHAT        (1 + WINDOWS_ID_BIAS)
#define DLG_SUPPLAGT              831
#define DLG_DEFSUPPLAGT           832
	#define cmExchangeCfg              110
	#define cmOrdCalendar              111
	#define CTL_SUPPLAGT_SUPPLIER      (1 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_DATE          (2 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_EXPIRY        (3 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_PAYPERIOD     (4 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_DELIVERY      (5 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_MAXRETURN     (6 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_AGENT         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLAGT_AGENT      (8 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_OPRKIND       (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLAGT_OPRKIND    (10 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_QUOTCOST      (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLAGT_QUOTCOST   (12 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_QUOTUP        (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLAGT_QUOTUP     (14 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_QUOTDOWN      (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLAGT_QUOTDOWN   (16 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_MNGRREL       (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLAGT_MNGRREL    (18 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_INVPACTION    (19 + WINDOWS_ID_BIAS)
	#define CTLCAL_SUPPLAGT_DATE       (20 + WINDOWS_ID_BIAS)
	#define CTLCAL_SUPPLAGT_EXPIRY     (21 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_FLAGS         (22 + WINDOWS_ID_BIAS)
	#define CTLCAL_SUPPLAGT_ORDPRD     (23 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_ORDPRD        (24 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_FLAGS2        (25 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLAGT_ORDPL         (26 + WINDOWS_ID_BIAS)
#define DLG_GPLABEL               833
	#define CTL_GPLABEL_WHAT           (1 + WINDOWS_ID_BIAS)
#define DLG_AUTONEWP              834 // Изменение цен в товарном документе
	#define CTL_AUTONEWP_LIST          (1 + WINDOWS_ID_BIAS)
//                                835 // Occupied by DLG_SELQUOT3
#define DLG_EDITPLU               836
	#define CTL_EDITPLU_PLU            (1 + WINDOWS_ID_BIAS)
#define DLG_LBXSEL                837
#define DLG_LBXSELT               838
	#define cmCreateAll                101
	#define cmSubstGoods               102
	#define CTL_LBXSEL_LIST            (1 + WINDOWS_ID_BIAS)
	#define CTL_LBXSEL_UPBTN           (2 + WINDOWS_ID_BIAS)
	#define CTL_LBXSEL_DOWNBTN         (3 + WINDOWS_ID_BIAS)
	#define CTL_LBXSEL_CREATEBTN       (4 + WINDOWS_ID_BIAS)
	#define CTL_LBXSEL_PERIOD          (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_LBXSEL_PERIOD       (6 + WINDOWS_ID_BIAS)
	#define CTL_LBXSEL_CREATEALLBTN    (7 + WINDOWS_ID_BIAS)
#define DLG_DLSFLT                839 // Фильтр по статистике загрузки устройств
	#define CTL_DLSFLT_DVCTYPE         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DLSFLT_DVCTYPE      (2 + WINDOWS_ID_BIAS)
	#define CTL_DLSFLT_DVCNAME         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DLSFLT_DVCNAME      (4 + WINDOWS_ID_BIAS)
	#define CTL_DLSFLT_PERIOD          (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_DLSFLT_PERIOD       (6 + WINDOWS_ID_BIAS)
	#define CTL_DLSFLT_GGRP            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_DLSFLT_GGRP         (8 + WINDOWS_ID_BIAS)
	#define CTL_DLSFLT_GOODS           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_DLSFLT_GOODS        (10 + WINDOWS_ID_BIAS)
#define DLG_CYCLES                840
	#define CTL_CYCLES_LENGTH          (1 + WINDOWS_ID_BIAS)
	#define CTL_CYCLES_NUM             (2 + WINDOWS_ID_BIAS)
#define DLG_SCARDRULE             841
	#define CTL_SCARDRULE_TRNOVRRNG    (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDRULE_PRD          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDRULE_PRD       (3 + WINDOWS_ID_BIAS)
	#define CTL_SCARDRULE_CNCLPRD      (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDRULE_CNCLPRD   (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDRULE_CNCLPRDC     (6 + WINDOWS_ID_BIAS)
#define DLG_TRNVRRNG             842
	#define CTL_TRNVRRNG_RANGE        (1 + WINDOWS_ID_BIAS)
	#define CTL_TRNVRRNG_DIS          (2 + WINDOWS_ID_BIAS)
	#define CTL_TRNVRRNG_SERIES       (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRNVRRNG_SERIES    (4 + WINDOWS_ID_BIAS)
	#define CTL_TRNVRRNG_METHOD       (5 + WINDOWS_ID_BIAS)
	#define CTL_TRNVRRNG_ZEROTRNOVR   (6 + WINDOWS_ID_BIAS)
#define DLG_QUOTLIST              843 // Список видов котировок
	#define CTL_QUOTLIST_LIST           (1 + WINDOWS_ID_BIAS)
#define DLG_CBXSEL               844
	#define CTL_CBXSEL_COMBO          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CBXSEL_COMBO       (2 + WINDOWS_ID_BIAS)
	#define CTL_CBXSEL_NAME           (3 + WINDOWS_ID_BIAS)
#define DLG_PANLZFLT             845
	#define CTL_PANLZFLT_PERIOD       (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_PANLZFLT_PERIOD    (2 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_SUPPL        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PANLZFLT_SUPPL     (4 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_GOODSGRP     (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PANLZFLT_GOODSGRP  (6 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_COSTALG      (7 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_FLAGS        (8 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_LOC          (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_PANLZFLT_LOC       (10 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_BASE         (11 + WINDOWS_ID_BIAS)
	#define CTL_PANLZFLT_BASELOC      (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_PANLZFLT_BASELOC   (13 + WINDOWS_ID_BIAS)
#define DLG_SELCHECK             846
#define DLG_SELCHECK_L          1025
	#define cmViewCheck               101
	#define cmSplitCheck              102
	#define cmNewCheck                103
	#define cmUniteChecks             104
	#define cmUnfinishedChecks        105
	#define CTL_SELCHECK_CODE         (1 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_DATE         (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELCHECK_DATE      (3 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_LIST         (4 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_ITEMLIST     (5 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_FORMAT       (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELCHECK_FORMAT    (7 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_FLTDATE      (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELCHECK_FLTDATE   (9 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_MEMO         (10 + WINDOWS_ID_BIAS)
	#define CTL_SELCHECK_UNFC         (11 + WINDOWS_ID_BIAS) // check-box for selection unfinished checks
	#define CTL_SELCHECK_OP           (12 + WINDOWS_ID_BIAS) // @v12.2.8 Выбор операции, которую надо осуществить по выбранному чеку (возврат/коррекция)
#define DLG_SSAUTODIS            847
	#define CTL_SSAUTODIS_SSER        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SSAUTODIS_SSER     (2 + WINDOWS_ID_BIAS)
	#define CTL_SSAUTODIS_PRD         (3 + WINDOWS_ID_BIAS)
	#define CTL_SSAUTODIS_ST_INFO     (4 + WINDOWS_ID_BIAS) // @v11.3.5
	#define CTL_SSAUTODIS_AP          (5 + WINDOWS_ID_BIAS) // @v11.3.5 arbitrary period
	#define CTLCAL_SSAUTODIS_AP       (6 + WINDOWS_ID_BIAS) // @v11.3.5 arbitrary period
#define DLG_RTQUOTK              848
#define DLG_PSNKIND              849
	#define CTL_PSNKIND_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_PSNKIND_NAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNKIND_CODEREGT      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNKIND_CODEREGT   (4 + WINDOWS_ID_BIAS)
	#define CTL_PSNKIND_FOLDREGT      (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNKIND_FOLDREGT   (6 + WINDOWS_ID_BIAS)
	#define CTL_PSNKIND_DEFSTATUS     (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNKIND_DEFSTATUS  (8 + WINDOWS_ID_BIAS)
	#define CTL_PSNKIND_SYMB          (9 + WINDOWS_ID_BIAS)
	#define CTL_PSNKIND_FLAGS         (10 + WINDOWS_ID_BIAS)
#define DLG_JOBITEM              850
	#define cmSchedule                101
	#define cmJobParam                102
	#define cmEmailParam              103
	#define CTL_JOBITEM_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_NAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_CMD           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_JOBITEM_CMD        (4 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_DBSYMB        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_JOBITEM_DBSYMB     (6 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_NEXTJOB       (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_JOBITEM_NEXTJOB    (8 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_FLAGS         (9 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_SYMB          (10 + WINDOWS_ID_BIAS)
	#define CTL_JOBITEM_SCHDLBEFORE   (11 + WINDOWS_ID_BIAS)
	#define CTLTM_JOBITEM_SCHDLBEFORE (12 + WINDOWS_ID_BIAS)
#define DLG_MSGNCMPL5            851 // Lack for draft bill writing off
	// CTL_MSGNCMPL_XXX
#define DLG_GSTRUCPRN            852
	#define CTL_GSTRUCPRN_WHAT        (1 + WINDOWS_ID_BIAS)
#define DLG_BILLSTATUS           853 // Статус документа
	#define CTL_BILLSTATUS_NAME       (1 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_ID         (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_RANK       (3 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_FLAGS      (4 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_SYMB       (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_CHECKFLDS  (6 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_OPCNTR     (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLSTATUS_OPCNTR  (8 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_OP         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLSTATUS_OP      (10 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_CHECKF2    (11 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_COLOR      (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLSTATUS_COLOR   (13 + WINDOWS_ID_BIAS)
	#define CTL_BILLSTATUS_SELCOLOR   (14 + WINDOWS_ID_BIAS)
#define DLG_TSA2                 854 // Анализ технологических сессий
	#define CTL_TSA_PRCGROUPONLY      (1 + WINDOWS_ID_BIAS)
	#define CTL_TSA_PRC               (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSA_PRC            (3 + WINDOWS_ID_BIAS)
	#define CTL_TSA_TECH              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSA_TECH           (5 + WINDOWS_ID_BIAS)
	#define CTL_TSA_ST_GOODS          (6 + WINDOWS_ID_BIAS)
	#define CTL_TSA_AR                (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSA_AR             (8 + WINDOWS_ID_BIAS)
	#define CTL_TSA_AR2               (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSA_AR2            (10 + WINDOWS_ID_BIAS)
	#define CTL_TSA_FNPERIOD          (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_TSA_FNPERIOD       (12 + WINDOWS_ID_BIAS)
	#define CTL_TSA_FLAGS             (13 + WINDOWS_ID_BIAS)
#define DLG_BUPARAM              855
	#define CTL_BUPARAM_PATH          (1 + WINDOWS_ID_BIAS)
	#define CTL_BUPARAM_MAXCOPIES     (2 + WINDOWS_ID_BIAS)
	#define CTL_BUPARAM_FLAGS         (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_BUPARAM_PATH       (4 + WINDOWS_ID_BIAS)
	#define CTL_BUPARAM_DBSYMB        (5 + WINDOWS_ID_BIAS)
#define DLG_CASHNPAR              856
	#define CTL_CASHNPAR_NODE          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHNPAR_NODE       (2 + WINDOWS_ID_BIAS)
	#define CTL_CASHNPAR_FLAGS         (3 + WINDOWS_ID_BIAS)
#define DLG_CSHRRTS               857
	#define cmCashierPassword         101
	#define CTL_CSHRRTS_RIGHTS        (1 + WINDOWS_ID_BIAS)
	#define CTL_CSHRRTS_RPTRIGHTS     (2 + WINDOWS_ID_BIAS)
#define DLG_SELBILLSTATUS        858 // Выбор статуса документа
	#define CTL_SELBILLSTATUS_BILL     (1 + WINDOWS_ID_BIAS)
	#define CTL_SELBILLSTATUS_STQUO    (2 + WINDOWS_ID_BIAS)
	#define CTL_SELBILLSTATUS_LIST     (3 + WINDOWS_ID_BIAS)
#define DLG_SCARDFL              859
	#define CTL_SCARDFL_SET           (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDFL_RESET         (2 + WINDOWS_ID_BIAS)
#define DLG_DBMAINTAIN           860
	#define CTL_DBMAINTAIN_TABLES      (1 + WINDOWS_ID_BIAS)
	#define CTL_DBMAINTAIN_DLSDAYS     (2 + WINDOWS_ID_BIAS)
	#define CTL_DBMAINTAIN_MRPDAYS     (3 + WINDOWS_ID_BIAS)
	#define CTL_DBMAINTAIN_SJDAYS      (4 + WINDOWS_ID_BIAS)
	#define CTL_DBMAINTAIN_XBILDAYS    (5 + WINDOWS_ID_BIAS)
#define DLG_PRCPAN               861 // Панель ввода сессий по процессору
	#define CTL_PRCPAN_INPUT           (1 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_PRCNAME         (2 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_CURSESS         (3 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_MAINGOODS       (4 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_GOODSNAME       (5 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_SERIAL          (6 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_PACK            (7 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_QTTYPACK        (8 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_QTTY            (9 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_SIGN            (10 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_INFO            (11 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_TOTALQTTY       (12 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_TOTALPACK       (13 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_ST_EXPIRY       (14 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_ST_PRICE        (15 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_ST_SUM          (16 + WINDOWS_ID_BIAS)

	#define CTL_PRCPAN_STARTHINT         (31 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINTBIAS        20
	#define CTL_PRCPAN_NUMHINTS           7
	#define CTL_PRCPAN_HINT1             (31 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT1           (51 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_HINT2             (32 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT2           (52 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_HINT3             (33 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT3           (53 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_HINT4             (34 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT4           (54 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_HINT5             (35 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT5           (55 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_HINT6             (36 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT6           (56 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_HINT7             (37 + WINDOWS_ID_BIAS)
	#define CTL_PRCPAN_KBHINT7           (57 + WINDOWS_ID_BIAS)
#define DLG_TSESSIDLE            862 // Перевод процессора в режим простоя //
	// CTL_TSESS_XXX
#define DLG_GDSCLSFORM           863 // Формула класса товаров
	#define CTL_GDSCLSFORM_NAME        (1 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSFORM_FORM        (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSFORM_PAR1        (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSFORM_PAR2        (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSFORM_PAR3        (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSFORM_FLAGS       (6 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSFORM_SYMB        (7 + WINDOWS_ID_BIAS) // @v12.0.11 Символ формулы для ссылок
#define DLG_GDSCLSCALC           864 // Расчет по формуле класса товаров
	#define CTL_GDSCLSCALC_FORM        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GDSCLSCALC_FORM     (2 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSCALC_PAR1        (3 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSCALC_PAR2        (4 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSCALC_PAR3        (5 + WINDOWS_ID_BIAS)
	#define CTL_GDSCLSCALC_VAL         (6 + WINDOWS_ID_BIAS)
#define DLG_GDSCLSFORMLIST       865 // Список формул по классу товаров
	#define CTL_GDSCLSFORMLIST_LIST    (1 + WINDOWS_ID_BIAS)
#define DLG_RTCFGLIST            866
	#define CTL_RTCFGLIST_LIST         (1 + WINDOWS_ID_BIAS)
#define DLG_SELCNODE             867
	#define CTL_SELCNODE_CNODE         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELCNODE_CNODE      (2 + WINDOWS_ID_BIAS)
	#define CTL_SELCNODE_PERIOD        (3 + WINDOWS_ID_BIAS)
#define DLG_TSES_PLAN            868 // Производственный план
	// CTL_TSESS_XXX
#define DLG_TSESSPLANLN          869 // Строка производственного плана
	// CTL_TSESSLN_XXX
#define DLG_TSESSPLANFILT        870 // Фильтр производственных планов
	// CTL_TSESSFILT_XXX
#define DLG_GCDGM                871 // GC_DGM Маска динамического обобщения класса товаров
	#define CTL_GCDGM_MASK             (1 + WINDOWS_ID_BIAS)
	#define CTL_GCDGM_GDSCLS           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_GCDGM_GDSCLS        (3 + WINDOWS_ID_BIAS)
#define DLG_FTPACCT              872
	#define CTL_FTPACCT_HOST           (1 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_PORT           (2 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_USER           (3 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_PWD            (4 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_PROXY          (5 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_PROXYPORT      (6 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_PROXYUSER      (7 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_PROXYPWD       (8 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_TIMEOUT        (9 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_NAME           (10 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_ID             (11 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_FLAGS          (12 + WINDOWS_ID_BIAS)
	#define CTL_FTPACCT_SYMB           (13 + WINDOWS_ID_BIAS)
#define DLG_PRCFREELIST          873 // Список периодов доступности процессора
	#define CTL_PRCFREELIST_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_TAGFLT               874
	#define cmDeleteAll                101
	#define CTL_TAGFLT_LIST            (1 + WINDOWS_ID_BIAS)
	#define CTL_TAGFLT_FLAGS           (2 + WINDOWS_ID_BIAS)
	#define CTL_TAGFLT_INFO            (3 + WINDOWS_ID_BIAS)
#define DLG_SELTAG               875
	#define cmSelEnum                  101
	#define CTL_SELTAG_TAG             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELTAG_TAG          (2 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_RESTRICT        (3 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_TAGTYPE         (4 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_EMPTY           (5 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_COLOR           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELTAG_COLOR        (7 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_SELCOLOR        (8 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_OPTION          (9 + WINDOWS_ID_BIAS)
	#define CTL_SELTAG_INFO            (10 + WINDOWS_ID_BIAS)
#define DLG_OPGRPNGFLTA          876
	#define CTL_OPGRPNGFLTA_PRD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_OPGRPNGFLTA_PRD     (2 + WINDOWS_ID_BIAS)
#define DLG_REPORTFLT			 877  // Фильтр отчетов
	#define CTL_REPORTFLT_PERIOD	  (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_REPORTFLT_PERIOD   (2 + WINDOWS_ID_BIAS)
	#define CTL_REPORTFLT_STDNAME	  (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPORTFLT_STDNAME  (4 + WINDOWS_ID_BIAS)
	#define CTL_REPORTFLT_STRUC       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPORTFLT_STRUC    (6 + WINDOWS_ID_BIAS)
	#define CTL_REPORTFLT_TYPE		  (7 + WINDOWS_ID_BIAS)
	#define CTL_REPORTFLT_ORDER       (8 + WINDOWS_ID_BIAS)
#define DLG_REPORT               878 // Изменение свойств отчета
	#define CTL_REPORT_DESCR          (1 + WINDOWS_ID_BIAS)
	#define CTL_REPORT_PATH           (2 + WINDOWS_ID_BIAS)
	#define CTL_REPORT_TYPE           (3 + WINDOWS_ID_BIAS)
	#define CTL_REPORT_MODIFDATE      (4 + WINDOWS_ID_BIAS)
	#define CTL_REPORT_STDNAME        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPORT_STDNAME     (6 + WINDOWS_ID_BIAS)
	#define CTL_REPORT_STRUCNAME      (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPORT_STRUCNAME   (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_REPORT_MODIFDATE   (9 + WINDOWS_ID_BIAS)
#define DLG_PRCBUSYFLT           879 // Фильтр занятости процессоров
	#define CTL_PRCBUSYFLT_PRCGRP      (1 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_PRC         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCBUSYFLT_PRC      (3 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_STDT        (4 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_STTM        (5 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_FNDT        (6 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_FNTM        (7 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_FLAGS       (8 + WINDOWS_ID_BIAS)
	#define CTL_PRCBUSYFLT_MINDUR      (9 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRCBUSYFLT_STDT     (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_PRCBUSYFLT_FNDT     (11 + WINDOWS_ID_BIAS)
	#define CTLTM_PRCBUSYFLT_STTM      (12 + WINDOWS_ID_BIAS)
	#define CTLTM_PRCBUSYFLT_FNTM      (13 + WINDOWS_ID_BIAS)
#define DLG_OPKDINVE             880 // Расширение вида операции инвентаризации долгов
	#define CTL_OPKDINVE_WRDNOP        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKDINVE_WRDNOP     (2 + WINDOWS_ID_BIAS)
	#define CTL_OPKDINVE_WD_GRP        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKDINVE_WD_GRP     (4 + WINDOWS_ID_BIAS)
	#define CTL_OPKDINVE_WD_GOODS      (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKDINVE_WD_GOODS   (6 + WINDOWS_ID_BIAS)
	#define CTL_OPKDINVE_WRUPOP        (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKDINVE_WRUPOP     (8 + WINDOWS_ID_BIAS)
	#define CTL_OPKDINVE_WU_GRP        (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKDINVE_WU_GRP     (10 + WINDOWS_ID_BIAS)
	#define CTL_OPKDINVE_WU_GOODS      (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_OPKDINVE_WU_GOODS   (12 + WINDOWS_ID_BIAS)
#define DLG_BILL_DEBTINVENT      881 // Документ долговой инвентаризации
	// CTL_BILL_XXX
#define DLG_DINVITEM             882 // Строка документа долговой инвентаризации
	#define CTL_DINVITEM_AR            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DINVITEM_AR         (2 + WINDOWS_ID_BIAS)
	#define CTL_DINVITEM_AMOUNT        (3 + WINDOWS_ID_BIAS)
	#define CTL_DINVITEM_MEMO          (4 + WINDOWS_ID_BIAS)
//                               883 // Occupied by DLG_CSESSTOTALWRET
#define DLG_PRCALTGROUP          884 // Альтернативная группа процессоров
	// CTL_PRC_XXX
#define DLG_ADVBILLKIND          885 // Вид документа строки расширения бух проводки
	#define CTL_ADVBILLKIND_NAME       (1 + WINDOWS_ID_BIAS)
	#define CTL_ADVBILLKIND_ID         (2 + WINDOWS_ID_BIAS)
	#define CTL_ADVBILLKIND_LNKOP      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADVBILLKIND_LNKOP   (4 + WINDOWS_ID_BIAS)
	#define CTL_ADVBILLKIND_FLAGS      (5 + WINDOWS_ID_BIAS)
#define DLG_SELECTOR             886 // Универсальный селектор. Состоит из блока радио-кнопок и кнопок
	// [ОК] и [Отмена]. Радио-кнопки заполняются из программы.
#define DLG_SELLINKBILLADD       887 // Установка связанного документа
#define DLG_SELLINKBILLRMV       888 // Удаление связанного документа
#define DLG_RPTMAIL 	         889 // Отправка отчета по e-mail
	#define CTL_RPTMAIL_ACCNT	       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_RPTMAIL_ACCNT	   (2 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_SUPPMAIL	   (3 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_ORG			   (4 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_LIC			   (5 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_USER		   (6 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_DB			   (7 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_RPT			   (8 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_DATASTRUC	   (9 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_DATE		   (10 + WINDOWS_ID_BIAS)
	#define CTL_RPTMAIL_TIME		   (11 + WINDOWS_ID_BIAS)
#define DLG_TSESSLNTOTAL         890 // Итоги по строкам техн сессий
	#define CTL_TSESSLNTOTAL_COUNT     (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLNTOTAL_QTTY      (2 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLNTOTAL_AMOUNT    (3 + WINDOWS_ID_BIAS)
	#define CTL_TSESSLNTOTAL_DSCNT     (4 + WINDOWS_ID_BIAS)
#define DLG_LINKFILES            891 // Файлы привязанные к документу
	#define cmLink                     101
	#define cmWaitFile                 102
	#define CTL_LINKFILES_LIST         (1 + WINDOWS_ID_BIAS)
#define DLG_SCARDRMVALL          892
	// controls from DLG_GOODSRMVALL
#define DLG_ASSETMODIFLOTITEM    893 // Строка модификации основого средства
	// CTL_LOT_XXX
#define DLG_TSESSTOTAL           894 // Итоги по технологическим сессиям
	#define CTL_TSESSTOTAL_COUNT       (1 + WINDOWS_ID_BIAS)
	#define CTL_TSESSTOTAL_DURATION    (2 + WINDOWS_ID_BIAS)
//                               895 // Occupied DLG_INLNCALC_L
//                               896 // Occupied DLG_CHKPAN_TS
#define DLG_TCHSCR               897 // Параметры TouchScreen
	#define cmSelFont                  101
	#define cmSelGrpList                  102
	#define CTL_TCHSCR_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_TCHSCR_ID              (2 + WINDOWS_ID_BIAS)
	#define CTL_TCHSCR_TYPE            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TCHSCR_TYPE         (4 + WINDOWS_ID_BIAS)
	#define CTL_TCHSCR_GDSGRP          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_TCHSCR_GDSGRP       (6 + WINDOWS_ID_BIAS)
	#define CTL_TCHSCR_GOODSLISTGAP    (7 + WINDOWS_ID_BIAS)
	#define CTL_TCHSCR_FLAGS           (8 + WINDOWS_ID_BIAS)
//                               898 // Occupied DLG_CALCDIFF_L
#define DLG_CMDITEM              899 // CMDITEM
	#define cmCmdParam                 101
	#define cmSelectIcon               102
	#define cmClearFilt                103
	#define cmSelBkgnd                 104
	#define CTL_CMDITEM_ID             (1 + WINDOWS_ID_BIAS)
	#define CTL_CMDITEM_NAME           (2 + WINDOWS_ID_BIAS)
	#define CTL_CMDITEM_CMD            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CMDITEM_CMD         (4 + WINDOWS_ID_BIAS)
	#define CTL_CMDITEM_ICON           (5 + WINDOWS_ID_BIAS)
	#define CTL_CMDITEM_USEDEFICON     (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_CMDITEM_ICON        (7 + WINDOWS_ID_BIAS)
	#define CTL_CMDITEM_CLEARFILT      (8 + WINDOWS_ID_BIAS)
	#define CTL_CMDITEM_FLAGS          (9 + WINDOWS_ID_BIAS)
#define DLG_MENULIST              900
	#define cmEditMenu                 101
	#define cmEditAsscCmdCommonList    102
	#define cmEditAsscCmdList          103
	#define CTL_MENULIST_LIST          (1 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_EDMBTN        (2 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_EDASSCBTN     (3 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_EDASSCCBTN    (4 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_IMAGE         (5 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_ADDIMG        (6 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_DELIMG        (7 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_BKGND         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_MENULIST_BKGND      (9 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_SELBKGND      (10 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_FLAGS         (11 + WINDOWS_ID_BIAS)
	#define CTL_MENULIST_GRPFLAGS      (12 + WINDOWS_ID_BIAS)
#define DLG_ADDCMD                901
	#define CTL_ADDCMD_WHAT            (1 + WINDOWS_ID_BIAS)
	#define CTL_ADDCMD_PARENT          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADDCMD_PARENT       (3 + WINDOWS_ID_BIAS)
#define DLG_FLDCORR               902 // FLDCORR Сопоставление поля импорта/экспорта с полем структуры SdRecord
	#define CTL_FLDCORR_INNERFLD       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLDCORR_INNERFLD    (2 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_OUTERFLD       (3 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_OUTERTYPE      (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLDCORR_OUTERTYPE   (5 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_OUTERSZ        (6 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_OUTERPRC       (7 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_USEFORMULA     (8 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_FORMULA        (9 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_OUTERFORMULA   (10 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_INFO           (11 + WINDOWS_ID_BIAS)
	#define CTL_FLDCORR_USEOUTERF      (12 + WINDOWS_ID_BIAS)
#define DLG_FLDCORRLIST           903 // FLDCORRL Список сопоставления полей импорта/экспорта
	#define CTL_FLDCORRLIST_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_TXTDBPARAM            904 // TXTDBPAR Формат текстового файла данных
	#define CTL_TXTDBPARAM_FILENAME    (1 + WINDOWS_ID_BIAS)
	#define CTL_TXTDBPARAM_ORIENT      (2 + WINDOWS_ID_BIAS)
	#define CTL_TXTDBPARAM_FLAGS       (3 + WINDOWS_ID_BIAS)
	#define CTL_TXTDBPARAM_DIV         (4 + WINDOWS_ID_BIAS)
	#define CTL_TXTDBPARAM_HDRCOUNT    (5 + WINDOWS_ID_BIAS)
	#define CTLBRW_TXTDBPARAM_FNAME    (6 + WINDOWS_ID_BIAS)
	#define CTL_TXTDBPARAM_FOOTER      (7 + WINDOWS_ID_BIAS)
	#define CTL_TXTDBPARAM_ENCODING    (8 + WINDOWS_ID_BIAS)
#define DLG_IMPEXP                905 // IMPEXP Базовый диалог параметров экспорта/импорта
	#define IMPEXP_CTL_BIAS            (50 + WINDOWS_ID_BIAS)
//  Если какие-либо диалоги наследуют поведение этого диалога, то
//  коды их управляющих элементов должны начинаться с IMPEXP_CTL_BIAS+1
//
	#define cmImpExpTxtDbParam         101
	#define cmImpExpFldCorr            102
	#define CTL_IMPEXP_BUT_TXTDB       (1 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_BUT_FLDCORR     (2 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_FORMAT          (3 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_DIR             (4 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_FILENAME        (5 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_FLAGS           (6 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_NAME            (8 + WINDOWS_ID_BIAS)
	#define CTLBRW_IMPEXP_FILENAME     (9 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_FTPACC          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPEXP_FTPACC       (11 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXP_OUTERFMTVER     (12 + WINDOWS_ID_BIAS) // @v11.6.5
//
#define DLG_PAYPLAN               906 // B_PAYPLN План платежей по документу
	#define cmPayPlanCalc              101
	#define CTL_PAYPLAN_LIST           (1 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLAN_BUT_CALC       (2 + WINDOWS_ID_BIAS)
#define DLG_PAYPLANITEM           907 // B_PPITEM Элемент плана платежей по документу
	#define CTL_PAYPLANITEM_DT         (1 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANITEM_AMOUNT     (2 + WINDOWS_ID_BIAS)
	#define CTL_PAYPLANITEM_INTEREST   (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_PAYPLANITEM_DT      (4 + WINDOWS_ID_BIAS) // @alena
#define DLG_PRNACCTURN            908 // PRNAT Выбор печатной формы по бухгалтерской операции
	#define CTL_PRNACCTURN_FORM        (1 + WINDOWS_ID_BIAS)
#define DLG_TSESANLZFILT3         909 // F_TSESA3 Фильтр анализа технологических сессий
	// CTL_TSESANLZFILT_XXX
#define DLG_SYSJFILT2             910
// controls from DLG_SYSJFILT
#define DLG_RTTSES                911 // RT_TSES Права доступа к технологическим сессиям
	// CTL_RTOBJ_XXX
#define DLG_IMPEXPCLIBNK          912 // CB_IMEX Параметры импорта/экспорта для систем клиент-банк
	#define CTL_IMPEXPCLIBNK_BNKCODE   (2 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPCLIBNK_PMCODES   (3 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPCLIBNK_FLAGS     (4 + IMPEXP_CTL_BIAS)
#define DLG_DUTYSCHED              913 // DTYSCH График дежурств
	#define cmDutySchedAddCp           101 // Добавить точку отсчета
	#define cmDutySchedRmvCp           102 // Удалить точку отсчета
	#define cmDutySchedStaffCal        103 // Просмотр штатных календарей (если связанный объект PPOBJ_PERSON)
	#define cmFillStaffCal             104 // Заполнение штатного календаря (если связанный объект PPOBJ_PERSON)
	#define CTL_DUTYSCHED_ID           (1 + WINDOWS_ID_BIAS)
	#define CTL_DUTYSCHED_NAME         (2 + WINDOWS_ID_BIAS)
	#define CTL_DUTYSCHED_OBJTYPE      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DUTYSCHED_OBJTYPE   (4 + WINDOWS_ID_BIAS) // reserve (CTL_DUTYSCHED_OBJTYPE - radiobutton)
	#define CTL_DUTYSCHED_OBJGRP       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_DUTYSCHED_OBJGRP    (6 + WINDOWS_ID_BIAS)
	#define CTL_DUTYSCHED_LIST         (7 + WINDOWS_ID_BIAS)
	#define CTL_DUTYSCHED_CPLIST       (8 + WINDOWS_ID_BIAS)
#define DLG_DUTYSCHEDITEM          914 // DTYSCHI Элемент графика дежурств
	#define CTL_DUTYSCHEDITEM_OBJ      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DUTYSCHEDITEM_OBJ   (2 + WINDOWS_ID_BIAS)
	#define CTL_DUTYSCHEDITEM_REP      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DUTYSCHEDITEM_REP   (4 + WINDOWS_ID_BIAS)
#define DLG_DTR                    915 // DTR Периодичность (PPOBJ_DATETIMEREP)
	#define cmDtrRepeating             101
	#define CTL_DTR_ID                 (1 + WINDOWS_ID_BIAS)
	#define CTL_DTR_NAME               (2 + WINDOWS_ID_BIAS)
#define DLG_LOCPRN                 916 // Параметры локального принтера
	#define CTL_LOCPRN_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_LOCPRN_SYMB            (2 + WINDOWS_ID_BIAS)
	#define CTL_LOCPRN_ID              (3 + WINDOWS_ID_BIAS)
	#define CTL_LOCPRN_PORT            (4 + WINDOWS_ID_BIAS)
	#define CTL_LOCPRN_LOCATION        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCPRN_LOCATION     (6 + WINDOWS_ID_BIAS)
	#define CTLMNU_LOCPRN_PORT         (7 + WINDOWS_ID_BIAS)
	#define CTL_LOCPRN_FLAGS           (8 + WINDOWS_ID_BIAS)
#define DLG_SELLOCPRN              917 // Выбор принтера для печати чека
	#define CTL_SELLOCPRN_PRNTYPE      (1 + WINDOWS_ID_BIAS)
#define DLG_TOOLING                918 // TOOLING Параметры перенастройки процессора
	#define cmToolingTransGcMask       110
	// CTL_TECH_XXX
#define DLG_SPOILLIST              919 // SPPOILLST Список наименований, связанных с забракованной серией
	#define CTL_SPOILLIST_SERIAL       (1 + WINDOWS_ID_BIAS)
	#define CTL_SPOILLIST_LIST         (2 + WINDOWS_ID_BIAS)
	#define CTL_SPOILLIST_TEXT         (3 + WINDOWS_ID_BIAS)
#define DLG_SELSUSCHECK            920 // SELSUSCH Выбор отложеноого чека
#define DLG_SELSUSCHECK_L          933
// CTL_SELCHECK_XXX
#define DLG_CHNGTASKS              921
	#define CTL_CHNGTASKS_FLAGS        (1 + WINDOWS_ID_BIAS)
#define DLG_RTPRJTASK              922
	#define CTL_RTPRJTASK_FLAGS        (1 + WINDOWS_ID_BIAS)
	#define CTL_RTPRJTASK_SFLAGS       (2 + WINDOWS_ID_BIAS)
#define DLG_QTTY                   923
	#define CTL_QTTY_QTTY              (1 + WINDOWS_ID_BIAS)
//                                 924 // Occupied by DLG_OPKMORE_DRC
//                                 925 // Occupied by DLG_OPKMORE_DEX
//                                 926 // Occupied by DLG_GDSOPRPLANTOTAL
#define DLG_SELSERIAL              927 // SELSER Выбор серийного номера
	#define CTL_SELSERIAL_LIST         (1 + WINDOWS_ID_BIAS)
	#define CTL_SELSERIAL_GOODSNAME    (2 + WINDOWS_ID_BIAS)
#define DLG_CRTFILE                928
	#define CTL_CRTFILE_TYPE           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CRTFILE_TYPE        (2 + WINDOWS_ID_BIAS)
	#define CTL_CRTFILE_PATH           (3 + WINDOWS_ID_BIAS)
	#define CTL_CRTFILE_BROWSE         (4 + WINDOWS_ID_BIAS)
#define DLG_TDISCALC               929 // TDISCALC
	#define CTL_TDISCALC_METHOD        (1 + WINDOWS_ID_BIAS)
	#define CTL_TDISCALC_ST_INFO       (2 + WINDOWS_ID_BIAS)
	#define CTL_TDISCALC_PREC          (3 + WINDOWS_ID_BIAS)
	#define CTL_TDISCALC_DTSWITCH      (4 + WINDOWS_ID_BIAS)
	#define CTL_TDISCALC_DT3918        (5 + WINDOWS_ID_BIAS)
	#define CTL_TDISCALC_DT4005        (6 + WINDOWS_ID_BIAS)
	#define CTL_TDISCALC_DT4108        (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_TDISCALC_DTSWITCH   (8 + WINDOWS_ID_BIAS) // @alena
#define DLG_SELWORLD               930
	#define CTL_SELWORLD_TYPE          (1 + WINDOWS_ID_BIAS)
#define DLG_CONTINENT              931
//
// CTL_WORLD_...
//
#define DLG_OBJVIEWT               932
//
// CTL_OBJVIEW_...
//
//                                 933 // Occupied by DLG_SELSUSCHECK_L
#define DLG_SELDRAFTBYCHK          934
	#define CTL_SELDRAFTBYCHK_SEL      (1 + WINDOWS_ID_BIAS)
	#define CTL_SELDRAFTBYCHK_OP       (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELDRAFTBYCHK_OP    (3 + WINDOWS_ID_BIAS)
#define DLG_IEBILLSEL              935
	#define CTL_IEBILLSEL_BILL         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEBILLSEL_BILL      (2 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_BROW         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEBILLSEL_BROW      (4 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_OP           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEBILLSEL_OP        (6 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_LOC          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEBILLSEL_LOC       (8 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_SIGN         (9 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_FLAGS        (10 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_PERIOD       (11 + WINDOWS_ID_BIAS)
	#define CTLCAL_IEBILLSEL_PERIOD    (12 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_TECH         (13 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_MAILACC      (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEBILLSEL_MAILACC   (15 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_MAILADR      (16 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_MAILSUBJ     (17 + WINDOWS_ID_BIAS)
	#define CTL_IEBILLSEL_GUA          (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEBILLSEL_GUA       (19 + WINDOWS_ID_BIAS)
#define DLG_GMQUOT                 936
//
// CTL_GQUOT_...
//
#define DLG_PSNEVTTOTAL            937
	#define CTL_PSNEVTTOTAL_COUNT      (1 + WINDOWS_ID_BIAS)
#define DLG_INFKIOSK               938
#define DLG_INFKIOSK_TS            942
// cmXXX - as at DLG_CHKPAN
	#define CTL_INFKIOSK_GOODS         (1 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_PRICE         (2 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_STATUS        (3 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_INPUT         (4 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_INFO          (5 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_CODE          (6 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_LOTS          (7 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_IMAGE         (8 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_DSCNTPRICE    (9 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_ADDINF1       (10 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_ADDINF2       (11 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_ADDINF3       (12 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_ADDINF4       (13 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_ADDINF5       (14 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPBOX1       (15 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPBOX2       (16 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GDSLIST       (17 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPLIST       (18 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_STARTBUTTON   (19 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_NUMBUTTONS     7
	#define CTL_INFKIOSK_SCARD         (19 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_BYPRICE       (20 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_ENTER         (21 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_SELGDSGRP     (22 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPBYDEF      (23 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_PRINTLBL      (24 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_BYNAME        (25 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPNAME       (26 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPBOX3       (27 + WINDOWS_ID_BIAS)
	#define CTL_INFKIOSK_GRPBOX4       (28 + WINDOWS_ID_BIAS)
#define DLG_EGDSINFO               939
	#define CTL_EGDSINFO_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_EGDSINFO_TOUCHSC       (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_EGDSINFO_TOUCHSC    (3 + WINDOWS_ID_BIAS)
	#define CTL_EGDSINFO_LOC           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_EGDSINFO_LOC        (5 + WINDOWS_ID_BIAS)
	#define CTL_EGDSINFO_PRINTER       (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_EGDSINFO_PRINTER    (7 + WINDOWS_ID_BIAS)
	#define CTL_EGDSINFO_FLAGS         (8 + WINDOWS_ID_BIAS)
#define DLG_RCVRCSESS             940 // Корректировка кассовых сессий
	#define CTL_RCVRCSESS_WHAT         (1 + WINDOWS_ID_BIAS)
#define DLG_PSNRELFLT             941
	#define CTL_PSNRELFLT_RELTYPE      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNRELFLT_RELTYPE   (2 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELFLT_PSNFLTTO     (3 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELFLT_SORTORD      (4 + WINDOWS_ID_BIAS)
	#define CTL_PSNRELFLT_ADDEDSEL     (5 + WINDOWS_ID_BIAS)
//                                942 // Occupied by DLG_INFKIOSK_TS
#define DLG_LOGS                  943
	#define cmSendByEmail              101
	#define CTL_LOGS_LIST              (1 + WINDOWS_ID_BIAS)
#define DLG_MAILACCVIEW           944
	#define cmExport            101
// CTL_OBJVIEW_...
//                                945 // Occupied by DLG_CASHREPORTS
#define DLG_CFGKBDWKEY            946
	#define CTL_CFGKBDWKEY1            (1 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY2            (2 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY3            (3 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY4            (4 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY5            (5 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY6            (6 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY7            (7 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY8            (8 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY9            (9 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY10           (10 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY11           (11 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY12           (12 + WINDOWS_ID_BIAS)
	#define CTL_CFGKBDWKEY13           (13 + WINDOWS_ID_BIAS)
#define DLG_XMLDBPARAM            947
	#define CTL_XMLDBPARAM_ROOTTAG     (1 + WINDOWS_ID_BIAS)
	#define CTL_XMLDBPARAM_RECTAG      (2 + WINDOWS_ID_BIAS)
	#define CTL_XMLDBPARAM_FLAGS       (3 + WINDOWS_ID_BIAS)
	#define CTL_XMLDBPARAM_HDRTAG      (4 + WINDOWS_ID_BIAS)
#define DLG_RCVRTRFR              948 // R_TRFR
	#define CTL_RCVRTRFR_WHAT          (1 + WINDOWS_ID_BIAS)
	#define CTL_RCVRTRFR_VAL           (2 + WINDOWS_ID_BIAS)
#define DLG_TGSTRUC               949
	#define CTL_TGSTRUC_LINES          (1 + WINDOWS_ID_BIAS)
	#define CTL_TGSTRUC_GOODS          (2 + WINDOWS_ID_BIAS)
#define DLG_SALCHARGE             950
#define DLG_SALCHGRP              951
	#define cmSalChargeSelSymb         101
	#define CTL_SALCHARGE_ID           (1 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_NAME         (2 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_STAFCAL      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALCHARGE_STAFCAL   (4 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_SYMB         (5 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_FORMULA      (6 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_LIST         (7 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_WROFFOP      (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALCHARGE_WROFFOP   (9 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_AMTTYPE      (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALCHARGE_AMTTYPE   (11 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_FLAGS        (12 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_ENUM         (13 + WINDOWS_ID_BIAS)
	#define CTL_SALCHARGE_ENUMEXT      (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALCHARGE_ENUMEXT   (15 + WINDOWS_ID_BIAS)
//#define DLG_SALCHGRPV             952
#define DLG_SALCHARGEVIEW         952 // SALCVIEW Список просмотра видов начислений зарплаты
// CTL_OBJVIEW_...
#define DLG_R_TFR_A               953 // Обработка битых строк Transfer
	#define CTL_R_TFR_A_LIST           (1 + WINDOWS_ID_BIAS)
#define DLG_STAFFCAL              954
	#define CTL_STAFFCAL_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_NAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_LIST          (3 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_SYMB          (4 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_LINKOBJ       (5 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_PARENTNAME    (6 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_PARENT        (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCAL_PARENT     (8 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_FLAGS         (9 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_SUBST         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCAL_SUBST      (11 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_COLOR         (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCAL_COLOR      (13 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCAL_SELCOLOR      (14 + WINDOWS_ID_BIAS)
#define DLG_STAFFCALD             955
	#define CTL_STAFFCALD_KIND         (1 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_DATE         (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_STAFFCALD_DATE      (3 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_DAYOFW       (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALD_DAYOFW    (5 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_WORKTIME     (6 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_FLAGS        (7 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_TMSTART      (8 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_TMEND        (9 + WINDOWS_ID_BIAS)
	#define CTLTM_STAFFCALD_TMSTART    (10 + WINDOWS_ID_BIAS)
	#define CTLTM_STAFFCALD_TMEND      (11 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_ST_CAL       (12 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALD_ST_OBJ       (13 + WINDOWS_ID_BIAS)
#define DLG_CHKPANSLEEP           956 // CHKPANSL Вывод чековой панели из режима блокировки
	#define CTL_CHKPANSLEEP_CODE       (1 + WINDOWS_ID_BIAS)
#define DLG_DEBTEXF               957
	#define CTL_DEBTEXF_PAYER          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTEXF_PAYER       (2 + WINDOWS_ID_BIAS)
	#define CTL_DEBTEXF_AGENT          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTEXF_AGENT       (4 + WINDOWS_ID_BIAS)
	#define CTL_DEBTEXF_EOBJSHEET      (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTEXF_EOBJSHEET   (6 + WINDOWS_ID_BIAS)
	#define CTL_DEBTEXF_EXTOBJ         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTEXF_EXTOBJ      (8 + WINDOWS_ID_BIAS)
	#define CTL_DEBTEXF_DBTDIM         (9 + WINDOWS_ID_BIAS)
#define DLG_INCASSO               958
#define DLG_INCASSO_L            1009
	#define CTL_INCASSO_SUM            (1 + WINDOWS_ID_BIAS)
	#define CTL_INCASSO_TYPE           (2 + WINDOWS_ID_BIAS)
#define DLG_TESTCHNG              959
	#define CTL_TESTCHNG_STRING        (1 + WINDOWS_ID_BIAS)
	#define CTL_TESTCHNG_LIST          (2 + WINDOWS_ID_BIAS)
#define DLG_SUPLEXCHCFG           960
	#define CTL_SUPLEXCHCFG_LASTDT     (1 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_GGRP       (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXCHCFG_GGRP    (3 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_TECH       (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXCHCFG_TECH    (5 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_CLICODE    (6 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_OP         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXCHCFG_OP      (8 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_IP         (9 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_PORT       (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_SUPLEXCHCFG_LASTDT  (11 + WINDOWS_ID_BIAS) // @alena
	#define CTL_SUPLEXCHCFG_QUOT       (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXCHCFG_QUOT    (13 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_VER        (14 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_PRVDR      (15 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_TADDR      (16 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_TNAME      (17 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_TPW        (18 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXCHCFG_STYLO      (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXCHCFG_STYLO   (20 + WINDOWS_ID_BIAS)
#define DLG_SCARDCFG              961 // CF_SCARD Конфигурация дисконтных карт
	#define CTL_SCARDCFG_PSNKND        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_PSNKND     (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_DEFPSN        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_DEFPSN     (4 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_FLAGS         (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_CHARGEGG      (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_CHARGEGG   (7 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_CHARGEG       (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_CHARGEG    (9 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_CHRGAMT       (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_CHRGAMT    (11 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_DEFSER        (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_DEFSER     (13 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_DEFCSER       (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_SCARDCFG_DEFCSER    (15 + WINDOWS_ID_BIAS)
	#define CTL_SCARDCFG_WARNEXPB      (16 + WINDOWS_ID_BIAS)

#define DLG_YESNO                 962
	#define CTL_YESNO_YESNO            (1 + WINDOWS_ID_BIAS)
#define DLG_RENMDUPLSC            963
	#define CTL_RENMDUPLSC_FLAGS       (1 + WINDOWS_ID_BIAS)
#define DLG_GBDATAINV             964 // Выбор корзины-приемника для инвентаризации
// CTL_GBDATA_...
#define DLG_BSKTTOINV             965 // Перенос корзины в документ инвентаризации
	#define CTL_BSKTTOINV_SGOPTION     (1 + WINDOWS_ID_BIAS)
	#define CTL_BSKTTOINV_RULEPRICE    (2 + WINDOWS_ID_BIAS)
#define DLG_SCARDORD              966
	#define CTL_SCARDORD_ORD           (1 + WINDOWS_ID_BIAS)
#define DLG_ABSACCT               967
	#define CTL_ABSACCT_LOGFNAME       (1 + WINDOWS_ID_BIAS)
	#define CTL_ABSACCT_FLAGS          (2 + WINDOWS_ID_BIAS)
	#define CTL_ABSACCT_CUR            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ABSACCT_CUR         (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_ABSACCT_LOGFNAME    (5 + WINDOWS_ID_BIAS)
#define DLG_STAFFCALVIEW          968 // SCALVIEW Список штатных календарей
	#define CTL_STAFFCALVIEW_LIST      (1 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALVIEW_ST_TTL    (2 + WINDOWS_ID_BIAS)
#define DLG_TESTSTAFFCAL          969 // __STCAL Тест расчета дней по штатным календарям
	#define CTL_TESTSTAFFCAL_POST      (1 + WINDOWS_ID_BIAS)
	#define CTL_TESTSTAFFCAL_CSYM      (2 + WINDOWS_ID_BIAS)
	#define CTL_TESTSTAFFCAL_PCSYM     (3 + WINDOWS_ID_BIAS)
	#define CTL_TESTSTAFFCAL_PERIOD    (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_TESTSTAFFCAL_PERIOD (5 + WINDOWS_ID_BIAS)
	#define CTL_TESTSTAFFCAL_DAYS      (6 + WINDOWS_ID_BIAS)
	#define CTL_TESTSTAFFCAL_HOURS     (7 + WINDOWS_ID_BIAS)
	#define CTL_TESTSTAFFCAL_ST_MSG    (8 + WINDOWS_ID_BIAS)
#define DLG_WAITCMD               970 // Диалог ожидания команды
	#define CTL_WAITCMD_INPUT          (1 + WINDOWS_ID_BIAS)
	#define CTL_WAITCMD_STATUS         (2 + WINDOWS_ID_BIAS)
#define DLG_DESKCMDA              971 // Диалог редактирования ассоциаций команд рабочего стола
	#define CTL_DESKCMDA_DESKTOP       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DESKCMDA_DESKTOP    (2 + WINDOWS_ID_BIAS)
	#define CTL_DESKCMDA_LIST          (3 + WINDOWS_ID_BIAS)
#define DLG_DESKCMDAI             972 // Диалог редактирования ассоциации команды рабочего стола
	#define CTL_DESKCMDAI_COMMAND      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DESKCMDAI_COMMAND   (2 + WINDOWS_ID_BIAS)
	#define CTL_DESKCMDAI_CODE         (3 + WINDOWS_ID_BIAS)
	#define CTL_DESKCMDAI_FLAGS        (4 + WINDOWS_ID_BIAS)
	#define CTL_DESKCMDAI_DVCSERIAL    (5 + WINDOWS_ID_BIAS)
	#define CTL_DESKCMDAI_PARAM        (6 + WINDOWS_ID_BIAS)
#define DLG_SELPERSONC            973  // SELPRSNC
	#define CTL_SELPERSONC_PRSN        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELPERSONC_PRSN     (2 + WINDOWS_ID_BIAS)
	#define CTL_SELPERSONC_CODEINP     (3 + WINDOWS_ID_BIAS)
	#define CTL_SELPERSONC_SUBTITLE    (4 + WINDOWS_ID_BIAS)
	#define CTL_SELPERSONC_PRSNSC      (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELPERSONC_PRSNSC   (6 + WINDOWS_ID_BIAS)
	#define CTL_SELPERSONC_SCARD       (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELPERSONC_SCARD    (8 + WINDOWS_ID_BIAS)
#define DLG_RLOSTPSN              974
	#define cmResolveCreator           101
	#define cmResolveEmployer          102
	#define cmResolveClient            103
	#define cmViewTasksByCreator       104
	#define cmViewTasksByEmployer      105
	#define cmViewTasksByClient        106
	#define CTL_RLOSTPSN_LIST          (1 + WINDOWS_ID_BIAS)
#define DLG_VIEWTASKS             975
	#define CTL_VIEWTASKS_LIST         (1 + WINDOWS_ID_BIAS)
#define DLG_SELBASKET             976 // GBSEL
#define DLG_PSNCFG                977 // CF_PSN
	#define cmNewPersnMarks            101
	#define CTL_PSNCFG_FOLDER          (1  + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_TRADELIC        (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNCFG_TRADELIC     (3  + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_FLAGS           (4  + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_REGCAL          (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_PSNCFG_REGCAL       (6  + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_IMGFOLDER       (7  + WINDOWS_ID_BIAS)
	#define CTLBRW_PSNCFG_IMGFOLDER    (8  + WINDOWS_ID_BIAS)
	#define CTLBRW_PSNCFG_FOLDER       (9  + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_CALQUANT        (10 + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_SMSTIMEOUT      (11 + WINDOWS_ID_BIAS)
	#define CTL_PSNCFG_SMSPRTR         (12 + WINDOWS_ID_BIAS)
#define DLG_REPLOBJ               978
	#define CTL_REPLOBJ_SRC            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLOBJ_SRC         (2 + WINDOWS_ID_BIAS)
	#define CTL_REPLOBJ_DEST           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_REPLOBJ_DEST        (4 + WINDOWS_ID_BIAS)
#define DLG_ADVCBXSEL             979
	// CTL_CBXSEL_...
#define DLG_PPOSTFLT              980 // F_PPOST
	#define CTL_PPOSTFLT_ORG          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PPOSTFLT_ORG       (2 + WINDOWS_ID_BIAS)
	#define CTL_PPOSTFLT_DIV          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PPOSTFLT_DIV       (4 + WINDOWS_ID_BIAS)
	#define CTL_PPOSTFLT_STAFF        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PPOSTFLT_STAFF     (6 + WINDOWS_ID_BIAS)
	#define CTL_PPOSTFLT_PERIOD       (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_PPOSTFLT_PERIOD    (8 + WINDOWS_ID_BIAS)
	#define CTL_PPOSTFLT_OPENED       (9 + WINDOWS_ID_BIAS)
	#define CTL_PPOSTFLT_FNPERIOD     (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_PPOSTFLT_FNPERIOD  (11 + WINDOWS_ID_BIAS)
//                                981 // @occupied DLG_STAFFAMT
#define DLG_STAFFAMTLIST          982 // STAMTLST Список кадровых сумм
	#define CTL_STAFFAMTLIST_LIST     (1 + WINDOWS_ID_BIAS)
	#define CTL_STAFFAMTLIST_ST_INFO  (2 + WINDOWS_ID_BIAS)
#define DLG_RTSTAFF               983 // RT_STAFF Права доступа к штатному расписанию и штатным назначениям
	#define CTL_RTSTAFF_FLAGS         (1 + WINDOWS_ID_BIAS)
	#define CTL_RTSTAFF_SFLAGS        (2 + WINDOWS_ID_BIAS)
#define DLG_SALARYFLT             984 // F_SALARY Фильтр анализа начислений зарплаты
	#define CTL_SALARYFLT_ORG          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARYFLT_ORG       (2 + WINDOWS_ID_BIAS)
	#define CTL_SALARYFLT_DIV          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARYFLT_DIV       (4 + WINDOWS_ID_BIAS)
	#define CTL_SALARYFLT_STAFF        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARYFLT_STAFF     (6 + WINDOWS_ID_BIAS)
	#define CTL_SALARYFLT_PERIOD       (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_SALARYFLT_PERIOD    (8 + WINDOWS_ID_BIAS)
	#define CTL_SALARYFLT_CHARGE       (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARYFLT_CHARGE    (10 + WINDOWS_ID_BIAS)
	#define CTL_SALARYFLT_POST         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARYFLT_POST      (12 + WINDOWS_ID_BIAS)
#define DLG_PP_SALARY             985 // PP_SLR Параметры процессора начисления зарплаты
	#define CTL_PP_SALARY_ORG          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_PP_SALARY_ORG       (2 + WINDOWS_ID_BIAS)
	#define CTL_PP_SALARY_DIV          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_PP_SALARY_DIV       (4 + WINDOWS_ID_BIAS)
	#define CTL_PP_SALARY_STAFF        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_PP_SALARY_STAFF     (6 + WINDOWS_ID_BIAS)
	#define CTL_PP_SALARY_PERIOD       (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_PP_SALARY_PERIOD    (8 + WINDOWS_ID_BIAS)
	#define CTL_PP_SALARY_FLAGS        (9 + WINDOWS_ID_BIAS)
#define DLG_SALARY                986 // SALARY Запись в книге начисления зарплаты
	#define CTL_SALARY_ORG             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARY_ORG          (2 + WINDOWS_ID_BIAS)
	#define CTL_SALARY_DIV             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARY_DIV          (4 + WINDOWS_ID_BIAS)
	#define CTL_SALARY_STAFF           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARY_STAFF        (6 + WINDOWS_ID_BIAS)
	#define CTL_SALARY_PERIOD          (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_SALARY_PERIOD       (8 + WINDOWS_ID_BIAS)
	#define CTL_SALARY_CHARGE          (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARY_CHARGE       (10 + WINDOWS_ID_BIAS)
	#define CTL_SALARY_AMOUNT          (11 + WINDOWS_ID_BIAS)
	#define CTL_SALARY_POST            (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_SALARY_POST         (13 + WINDOWS_ID_BIAS)
#define DLG_SUBSTGL               987
// ..CTL_LBXSEL_...
#define DLG_FILLSCDT              988 // FILLSCDT Заполнение штатных календарей по графику дежурств
	#define CTL_FILLSCDT_CAL           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FILLSCDT_CAL        (2 + WINDOWS_ID_BIAS)
	#define CTL_FILLSCDT_PERIOD        (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_FILLSCDT_PERIOD     (4 + WINDOWS_ID_BIAS)
#define DLG_SUPLEXPFLT            989 // Фильтр экспорта данных поставщику
	#define CTL_SUPLEXPFLT_SUPPL       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXPFLT_SUPPL    (2 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_PRD         (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SUPLEXPFLT_PRD      (4 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_GGRP        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXPFLT_GGRP     (6 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_OP          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXPFLT_OP       (8 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_FLAGS       (9 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_LOC         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPLEXPFLT_LOC      (11 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_DISCOUNT1   (12 + WINDOWS_ID_BIAS)
	#define CTL_SUPLEXPFLT_DISCOUNT2   (13 + WINDOWS_ID_BIAS)
#define DLG_EXTDEV                990 // Диалог выбора дополнительного оборудования для кассового узла
	#define cmCustDisp                  101
	#define cmBnkTerminal				102
	#define CTL_EXTDEV_TCHSCREEN        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_TCHSCREEN     (2 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_CASHNODE         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_CASHNODE      (4 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_PAPYRUS          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_PAPYRUS       (6 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_SCALE            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_SCALE         (8 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_PRINTER          (9 + WINDOWS_ID_BIAS)
	#define CTLMNU_EXTDEV_PRINTER       (10 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_LOCTCHSCR        (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_LOCTCHSCR     (12 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_PHNSVC           (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_PHNSVC        (14 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_DRAWERPORT       (15 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_DRAWERCMD        (16 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_KBELLPORT        (17 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_KBELLCMD         (18 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_RPTPRNPORT       (19 + WINDOWS_ID_BIAS)
	#define CTLMNU_EXTDEV_RPTPRNPORT    (20 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_EGAISMODE        (21 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_MANUFSERIAL      (22 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_EXTNODEASALT     (23 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_ALTREG           (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_ALTREG        (25 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_HOSTICURL        (26 + WINDOWS_ID_BIAS)
	#define CTLBRW_EXTDEV_HOSTICURL     (27 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_DRVVER           (28 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_CHKEGMUNIQ       (29 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_CHZNPM           (30 + WINDOWS_ID_BIAS)
	#define CTL_EXTDEV_CHZNGUA          (31 + WINDOWS_ID_BIAS)
	#define CTLSEL_EXTDEV_CHZNGUA       (32 + WINDOWS_ID_BIAS)

#define DLG_STAFFCALFLT           991 // F_STCAL Фильтр штатных календарей
	#define cmStaffCalFiltObjList       101
	#define cmStaffCalFiltCalList       102
	#define CTL_STAFFCALFLT_PERIOD      (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_STAFFCALFLT_PERIOD   (2 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_OT          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALFLT_OT       (4 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_OBJ         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALFLT_OBJ      (6 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_OBJLIST     (7 + WINDOWS_ID_BIAS)  // button
	#define CTL_STAFFCALFLT_CAL         (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALFLT_CAL      (9 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_CALLIST     (10 + WINDOWS_ID_BIAS) // button
	#define CTL_STAFFCALFLT_ORDER       (11 + WINDOWS_ID_BIAS) // button
	#define CTL_STAFFCALFLT_PK          (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALFLT_PK       (13 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_GRP         (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALFLT_GRP      (15 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_PROJC       (16 + WINDOWS_ID_BIAS)
	#define CTLSEL_STAFFCALFLT_PROJC    (17 + WINDOWS_ID_BIAS)
	#define CTL_STAFFCALFLT_FLAGS       (18 + WINDOWS_ID_BIAS)
#define DLG_SELSESNO              992
#define DLG_SELSESNO_L           1029
	#define CTL_SELSESNO_NUMBER          (1 + WINDOWS_ID_BIAS)
	#define CTL_SELSESNO_DATE            (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELSESNO_DATE         (3 + WINDOWS_ID_BIAS)
	#define CTL_SELSESNO_EXTCASHNODE     (4 + WINDOWS_ID_BIAS)
//                                993 // @occupied DLG_STREET
#define DLG_WOBJSTATUS            994 // WOBJSTAT
	#define CTL_WOBJSTATUS_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_WOBJSTATUS_NAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_WOBJSTATUS_ABBR          (3 + WINDOWS_ID_BIAS)
	#define CTL_WOBJSTATUS_KIND          (4 + WINDOWS_ID_BIAS)
	#define CTL_WOBJSTATUS_CODE          (5 + WINDOWS_ID_BIAS)
#define DLG_IMPKLADR              995 // IMPKLADR
	#define CTL_IMPKLADR_PARENT          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPKLADR_PARENT       (2 + WINDOWS_ID_BIAS)
	#define CTL_IMPKLADR_FLAGS           (3 + WINDOWS_ID_BIAS)
#define DLG_WORLDVIEW             996 // V_WORLD
#define DLG_RCVRGOODS             997 // R_GOODS Корректировка товарных групп
	#define CTL_RCVRGOODS_FLAGS          (1 + WINDOWS_ID_BIAS)
	#define CTL_RCVRGOODS_LOG            (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_RCVRGOODS_LOG         (3 + WINDOWS_ID_BIAS)
#define DLG_COLORS                998
	#define CTL_COLORS_RED          (1 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_GREEN        (2 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_BLUE         (3 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_RUP          (4 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_RDOWN        (5 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_GUP          (6 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_GDOWN        (7 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_BUP          (8 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_BDOWN        (9 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_HUE          (10 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_HUP          (11 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_HDOWN        (12 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_SAT          (13 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_SUP          (14 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_SDOWN        (15 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_VAL          (16 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_VUP          (17 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_VDOWN        (18 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_RGBRECT      (19 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_HSBRECT      (20 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_NEWCOLOR     (21 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_OLDCOLOR     (22 + WINDOWS_ID_BIAS)
	#define CTL_COLORS_BRIGHTRECT   (23 + WINDOWS_ID_BIAS)
#define DLG_TSESSCLRS             999
	#define cmChooseColor             110
	#define CTL_TSESSCLRS_PLANNED     (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCLRS_PLANNED  (2 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_PLANNEDNEW  (3 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_PENDING     (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCLRS_PENDING  (5 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_PENDINGNEW  (6 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_INPROGR     (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCLRS_INPROGR  (8 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_INPROGRNEW  (9 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_CLOSED      (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCLRS_CLOSED   (11 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_CLOSEDNEW   (12 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_CANCEL      (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSESSCLRS_CANCEL   (14 + WINDOWS_ID_BIAS)
	#define CTL_TSESSCLRS_CANCELNEW   (15 + WINDOWS_ID_BIAS)
#define DLG_TEST                 1000
	#define CTL_TEST_DATE                (1 + WINDOWS_ID_BIAS)
	#define CTL_TEST_PREV                (2 + WINDOWS_ID_BIAS)
	#define CTL_TEST_CHARGE              (3 + WINDOWS_ID_BIAS)
#define DLG_ARGCODELIST          1001 // ARGDSCL Список кодов товаров, ассоциированных со статьями
	#define CTL_ARGCODELIST_GOODS        (1 + WINDOWS_ID_BIAS)
	#define CTL_ARGCODELIST_LIST         (2 + WINDOWS_ID_BIAS)
#define DLG_ARGOODSCODE          1002 // ARGDSC  Код товара, ассоциированный со статьей
	#define CTL_ARGOODSCODE_GOODS        (1 + WINDOWS_ID_BIAS)
	#define CTL_ARGOODSCODE_CODE         (2 + WINDOWS_ID_BIAS)
	#define CTL_ARGOODSCODE_UPP          (3 + WINDOWS_ID_BIAS)
	#define CTL_ARGOODSCODE_LEN          (4 + WINDOWS_ID_BIAS)
	#define CTL_ARGOODSCODE_ACS          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ARGOODSCODE_ACS       (6 + WINDOWS_ID_BIAS)
	#define CTL_ARGOODSCODE_AR           (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_ARGOODSCODE_AR        (8 + WINDOWS_ID_BIAS)
#define DLG_LAUNCHAPP            1003
	#define CTL_LAUNCHAPP_APP                (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_LAUNCHAPP_APP             (2 + WINDOWS_ID_BIAS)
	#define CTL_LAUNCHAPP_PARAMS             (3 + WINDOWS_ID_BIAS)
	#define CTL_LAUNCHAPP_FLAGS              (4 + WINDOWS_ID_BIAS)
	#define CTL_LAUNCHAPP_SERVER		 (5 + WINDOWS_ID_BIAS)
	#define CTL_LAUNCHAPP_USER		 (6 + WINDOWS_ID_BIAS)
	#define CTL_LAUNCHAPP_PWD		 (7 + WINDOWS_ID_BIAS)
#define DLG_C_LINKBILL           1004 // CC_LBILL Проверка связанных документов
	#define CTL_C_LINKBILL_LOG           (1 + WINDOWS_ID_BIAS)
	#define CTL_C_LINKBILL_FLAGS         (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_C_LINKBILL_LOG        (3 + WINDOWS_ID_BIAS)
#define DLG_AMTTVIEW             1005 // AMTTVIEW Список типов сумм
	#define CTL_AMTTVIEW_LIST            (1 + WINDOWS_ID_BIAS)
#define DLG_LIKEOBJFLT           1006
	#define CTL_LIKEOBJFLT_OBJ           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_LIKEOBJFLT_OBJ        (2 + WINDOWS_ID_BIAS)
	#define CTL_LIKEOBJFLT_RATEPCT       (3 + WINDOWS_ID_BIAS)
#define DLG_REFOBJVIEW           1007
	#define CTL_REFOBJVIEW_LIST          (1 + WINDOWS_ID_BIAS)
//                               1008 // Occupied by DLG_CASHREPORTS_L
//                               1009 // Occupied by DLG_INCASSO_L
#define DLG_NOBJASSC             1010 // NOBJASSC
	#define CTL_NOBJASSC_NAME            (1 + WINDOWS_ID_BIAS)
	#define CTL_NOBJASSC_SYMB            (2 + WINDOWS_ID_BIAS)
	#define CTL_NOBJASSC_ID              (3 + WINDOWS_ID_BIAS)
	#define CTL_NOBJASSC_PRMR            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_NOBJASSC_PRMR         (5 + WINDOWS_ID_BIAS)
	#define CTL_NOBJASSC_SCND            (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_NOBJASSC_SCND         (7 + WINDOWS_ID_BIAS)
	#define CTL_NOBJASSC_LOCGRP          (8 + WINDOWS_ID_BIAS)
	#define CTL_NOBJASSC_SCNDGRP         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_NOBJASSC_SCNDGRP      (10 + WINDOWS_ID_BIAS)
#define DLG_NOBJASSCVIEW         1011 // NOBJASCV
	#define CTL_NOBJASSCVIEW_LIST        (1 + WINDOWS_ID_BIAS)
#define DLG_IMAGEINFO            1012
	#define CTL_IMAGEINFO_IMAGE          (1 + WINDOWS_ID_BIAS)
	#define CTL_IMAGEINFO_INFO           (2 + WINDOWS_ID_BIAS)
	#define CTL_IMAGEINFO_WARN           (3 + WINDOWS_ID_BIAS)
	#define CTL_IMAGEINFO_PIC_WARN       (4 + WINDOWS_ID_BIAS)
#define DLG_TOOLTIP              1013
	#define CTL_TOOLTIP_TEXT             (1 + WINDOWS_ID_BIAS)
	#define CTL_TOOLTIP_IMAGE            (2 + WINDOWS_ID_BIAS)
//                               1014 // Occupied by DLG_CASHDISP_WCFM
#define DLG_CASHPANEFLT          1015
	#define CTL_CASHPANEFLT_CNODE        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHPANEFLT_CNODE     (2 + WINDOWS_ID_BIAS)
	#define CTL_CASHPANEFLT_CMD          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHPANEFLT_CMD       (4 + WINDOWS_ID_BIAS)
#define DLG_ADDBILLFLT           1016
	#define CTL_ADDBILLFLT_BBT           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADDBILLFLT_BBT        (2 + WINDOWS_ID_BIAS)
	#define CTL_ADDBILLFLT_OP            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADDBILLFLT_OP         (4 + WINDOWS_ID_BIAS)
	#define CTL_ADDBILLFLT_LOC           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADDBILLFLT_LOC        (6 + WINDOWS_ID_BIAS)
	#define CTL_ADDBILLFLT_RESERVED      (7 + WINDOWS_ID_BIAS) // radio-button with reserved operations
	#define CTL_ADDBILLFLT_FLAGS         (8 + WINDOWS_ID_BIAS) //
#define DLG_CLIBNKFMTLIST        1017
	#define cmCliBnkFmtListAssoc         101
	#define CTL_CLIBNKFMTLIST_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_RTACCLIST            1018
	#define CTL_RTACCLIST_LIST           (1 + WINDOWS_ID_BIAS)
#define DLG_RTACCLI              1019
	#define CTL_RTACCLI_ACC              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_RTACCLI_ACC           (2 + WINDOWS_ID_BIAS)
	#define CTL_RTACCLI_FLAGS            (3 + WINDOWS_ID_BIAS)
#define DLGW_LOGIN               1020
#define DLG_BILLCFGVAL           1021 // CF_BVAL
	// CTL_SELQUOT2_XXX
#define DLG_CSESSCRDRAFT         1022 // CSCRDRFT
	// CTL_DFRULESEL_XXX
// CTL_LOGIN_...
#define DLG_GOODSOPRE            1023
	#define CTL_GOODSOPRE_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_GOODSOPRE_PERIOD       (2 + WINDOWS_ID_BIAS)
	#define CTL_GOODSOPRE_VALLIST         (3 + WINDOWS_ID_BIAS)
	#define CTL_GOODSOPRE_FLAGS           (4 + WINDOWS_ID_BIAS)
	#define CTLMNU_GOODSOPRE_PERIOD       (5 + WINDOWS_ID_BIAS)
	#define CTL_GOODSOPRE_PCTDIFF         (6 + WINDOWS_ID_BIAS)
	#define CTL_GOODSOPRE_RESTDT          (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_GOODSOPRE_RESTDT       (8 + WINDOWS_ID_BIAS)
	#define CTLMNU_GOODSOPRE_RESTDT       (9 + WINDOWS_ID_BIAS)
#define DLG_SELTRPLANCMP         1024 // SELTPCMP
	#define CTL_SELTRPLANCMP_SEL          (1 + WINDOWS_ID_BIAS)
//                               1025 // Occupied by DLG_SELCHECK_L
//                               1026 // Occupied by DLG_CHKPANV_L
#define DLG_QUOTTOTAL            1027 // T_QUOT
	#define CTL_QUOTTOTAL_COUNT           (1 + WINDOWS_ID_BIAS)
#define DLG_IEINV                1028 // IEINV Параметры импорта инвентаризации
	#define CTL_IEINV_CFG                 (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEINV_CFG              (2 + WINDOWS_ID_BIAS)
	#define CTL_IEINV_OP                  (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEINV_OP               (4 + WINDOWS_ID_BIAS)
	#define CTL_IEINV_LOC                 (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEINV_LOC              (6 + WINDOWS_ID_BIAS)
	#define CTL_IEINV_DATE                (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_IEINV_DATE             (8 + WINDOWS_ID_BIAS)
//                               1029 // Occupied by DLG_SELSESNO_L
//                               1030 // Occupied by DLG_NEWDAY_L
#define DLG_GGRPTOTAL            1031 // T_GGRP Итоги по выборке товарных групп
	#define CTL_GGRPTOTAL_COUNT           (1 + WINDOWS_ID_BIAS)
	#define CTL_GGRPTOTAL_MAXLEVEL        (2 + WINDOWS_ID_BIAS)
	#define CTL_GGRPTOTAL_ALTCOUNT        (3 + WINDOWS_ID_BIAS)
	#define CTL_GGRPTOTAL_FOLDCOUNT       (4 + WINDOWS_ID_BIAS)
	#define CTL_GGRPTOTAL_GRPCOUNT        (5 + WINDOWS_ID_BIAS)

#define DLG_SCALEFLT             1032
	#define CTL_SCALEFLT_TYPE             (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALEFLT_TYPE          (2  + WINDOWS_ID_BIAS)
	#define CTL_SCALEFLT_LOC              (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALEFLT_LOC           (4  + WINDOWS_ID_BIAS)
	#define CTL_SCALEFLT_ALTGRP           (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALEFLT_ALTGRP        (6  + WINDOWS_ID_BIAS)
	#define CTL_SCALEFLT_PROT             (7  + WINDOWS_ID_BIAS)
	#define CTL_SCALEFLT_GROUP            (8  + WINDOWS_ID_BIAS)
	#define CTLSEL_SCALEFLT_GROUP         (9  + WINDOWS_ID_BIAS)
	#define CTL_SCALEFLT_FLAGS            (10 + WINDOWS_ID_BIAS)
#define DLG_CASHNFLT             1033
	#define CTL_CASHNFLT_TYPE             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHNFLT_TYPE          (2 + WINDOWS_ID_BIAS)
	#define CTL_CASHNFLT_LOC              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CASHNFLT_LOC           (4 + WINDOWS_ID_BIAS)
	#define CTL_CASHNFLT_SYNCTYPE         (5 + WINDOWS_ID_BIAS)
#define DLG_SELBYBCODE           1034
	#define CTL_SELBYBCODE_LIST           (1 + WINDOWS_ID_BIAS)
#define DLG_GMRQUOT              1035
#define DLG_SCARDTOTAL           1036 // T_SCARD
	#define CTL_SCARDTOTAL_COUNT          (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDTOTAL_TRNOVR         (2 + WINDOWS_ID_BIAS)
#define DLG_CNROUND              1037 // CNROUND
	#define CTL_CNROUND_DISDIR            (1 + WINDOWS_ID_BIAS)
	#define CTL_CNROUND_DISPREC           (2 + WINDOWS_ID_BIAS)
	#define CTL_CNROUND_AMTDIR            (3 + WINDOWS_ID_BIAS)
	#define CTL_CNROUND_AMTPREC           (4 + WINDOWS_ID_BIAS)
	#define CTL_IGNPANNYFROMBCARD         (5 + WINDOWS_ID_BIAS)
//                               1038 // Occupied by DLG_GBSTRUC_N
#define DLG_LOGSMON              1039
	#define CTL_LOGSMON_LIST              (1 + WINDOWS_ID_BIAS)
	#define	CTL_EDITLOGSLIST              (2 + WINDOWS_ID_BIAS)
	#define	cmEditLogsList                (2 + WINDOWS_ID_BIAS)
#define DLG_SELECT_LOGS          1040
	#define CTL_LLIST_LOGS                (1 + WINDOWS_ID_BIAS)
	#define CTL_RLIST_LOGS                (2 + WINDOWS_ID_BIAS)
	#define CTL_ADD_LOG                   (3 + WINDOWS_ID_BIAS)
	#define CTL_REM_LOG                   (4 + WINDOWS_ID_BIAS)
	#define CTL_ADD_ALL_LOGS              (5 + WINDOWS_ID_BIAS)
	#define CTL_REM_ALL_LOGS              (6 + WINDOWS_ID_BIAS)
//                               1041 // Occupied by DLG_ARTICLE_RL
//                               1042 // Occupied by DLG_DEBTTOTAL01
#define DLG_SELWHTYP             1043 // SELWHTYP Выбор типа склада (обыкновенный/группа)
	#define CTL_SELWHTYP_WHAT             (1 + WINDOWS_ID_BIAS)
#define DLG_GOODSCORR            1044
	#define CTL_GOODSCORR_MINQTTY         (1 + WINDOWS_ID_BIAS)
	#define	CTL_GOODSCORR_SORT            (2 + WINDOWS_ID_BIAS)
	#define CTL_GOODSCORR_SUBST           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSCORR_SUBST        (4 + WINDOWS_ID_BIAS)
	#define CTL_GOODSCORR_EXCLGG          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GOODSCORR_EXCLGG       (6 + WINDOWS_ID_BIAS)
//                               1045 // Occupied by DLG_TECHAUTO
#define DLG_IMPSPOIL             1046 // IMPSPOIL Импорт бракованных серий
	#define	CTL_IMPSPOIL_FLAGS            (1 + WINDOWS_ID_BIAS)
#define DLG_DEBTRATE             1047 // DEBTRATE Параметры PrcssrDebtRate
	#define CTL_DEBTRATE_ACS              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTRATE_ACS           (2 + WINDOWS_ID_BIAS)
	#define CTL_DEBTRATE_GANDICAP         (3 + WINDOWS_ID_BIAS)
	#define CTL_DEBTRATE_FLAGS            (4 + WINDOWS_ID_BIAS)
#define DLG_BIZSCORE             1048 // BIZSCORE Запись объекта PPOBJ_BIZSCORE
	#define CTL_BIZSCORE_ID               (1 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_NAME             (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_SYMB             (3 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_USER             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCORE_USER          (5 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_DESCR            (6 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_FORMULA          (7 + WINDOWS_ID_BIAS)
	#define CTLMNU_BIZSCORE_FORMULA       (8 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_TESTLINE         (9 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_BOUNDS           (10 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCORE_REVTEXT          (11 + WINDOWS_ID_BIAS)
#define DLG_BIZSCOREFILT         1049 // F_BIZSC Фильтр по бизнес-показателям
	#define CTL_BIZSCOREFILT_USER         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCOREFILT_USER      (2 + WINDOWS_ID_BIAS)
#define DLG_TRANSPCFG            1050 // CF_TRANS Конфигурация транспорта
	#define CTL_TRANSPCFG_NAMETEMPL       (1 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPCFG_OWNERK          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPCFG_OWNERK       (3 + WINDOWS_ID_BIAS)
	#define CTL_TRANSPCFG_CAPTK           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_TRANSPCFG_CAPTK        (5 + WINDOWS_ID_BIAS)
#define DLG_BIZSCVFILT           1051 // F_BIZSCV Фильтр по значениям бизнес-показателей
	#define CTL_BIZSCVFILT_USER           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCVFILT_USER        (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCVFILT_BSC            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCVFILT_BSC         (4 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCVFILT_DT             (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZSCVFILT_DT          (6 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCVFILT_TM             (7 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCVFILT_PERIOD         (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZSCVFILT_PERIOD      (9 + WINDOWS_ID_BIAS)
#define DLG_BIZSCPRC             1052 // BIZSCPRC Параметры расчета бизнес-показателей
	#define CTL_BIZSCPRC_PERIOD           (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZSCPRC_PERIOD        (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCPRC_FLAGS            (3 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCPRC_FTPACC           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCPRC_FTPACC        (5 + WINDOWS_ID_BIAS)
#define DLG_BUSPARAMS            1053
	#define CTL_BUSPARAMS_TEXT            (1 + WINDOWS_ID_BIAS)
#define DLG_GLOBUSERACC          1054 // GLOBUACC Запись объекта PPOBJ_GLOBALUSERACC
	#define cmGlobalServiceSetting        101
	#define cmGuaExtList                  102 // @v11.9.11
	#define CTL_GLOBUSERACC_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_NAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_SYMB          (3 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_PASSW         (4 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_DBUUID        (5 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_LOCALID       (6 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_PSN           (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GLOBUSERACC_PSN        (8 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_SVC           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_GLOBUSERACC_SVC        (10 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_FLAGS         (11 + WINDOWS_ID_BIAS)
	#define CTL_GS_SETTING                (13 + WINDOWS_ID_BIAS)
	#define CTL_GLOBUSERACC_LOGO          (14 + WINDOWS_ID_BIAS) // @v12.2.0
#define DLG_NEWACCT              1055 //
	#define cmPassword                    110
	#define CTL_NEWACCT_USER              (1 + WINDOWS_ID_BIAS)
#define DLG_DEBTSTATFILT         1056 //
	#define CTL_DEBTSTATFILT_ACS          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DEBTSTATFILT_ACS       (2 + WINDOWS_ID_BIAS)
	#define CTL_DEBTSTATFILT_ORDER        (3 + WINDOWS_ID_BIAS)
	#define CTL_DEBTSTATFILT_INCL         (4 + WINDOWS_ID_BIAS)
#define DLG_MEMO                 1057 // Диалог ввода примечани
// CTL_INPUT...
#define DLG_MEMOLIST             1058 // Список примечаний
// CTL_LBXSEL...
#define DLG_DSTATCFG             1059 // CF_DSTAT Конфигурация статистики дебиторов
	#define CTL_DSTATCFG_SIGMFACTOR       (1 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_FINRATE          (2 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_LIMITTERM        (3 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_RPREC            (4 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_RDIR             (5 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_PAYMBAR          (6 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_DELAYBAR         (7 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_EXPIRYWT         (8 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_FLAGS            (9 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_RELTYPE          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_DSTATCFG_RELTYPE       (11 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_LIMITFACTOR      (12 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_PERIOD           (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_DSTATCFG_PERIOD        (14 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_LASTTIME         (15 + WINDOWS_ID_BIAS)
	#define CTL_DSTATCFG_ADLIMITTERM      (16 + WINDOWS_ID_BIAS)
#define DLG_SBIICFG              1060 // CF_SBII Конфигурация для программы StyloBhtII
	#define CTL_SBIICFG_DEVICENAME        (1 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_USER              (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SBIICFG_USER           (3 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_SERVADDR          (4 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_SERVMASK          (5 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_SERVPORT          (6 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_WEIGHTPREFX       (7 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_FLAGS             (8 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_DEFQTTY           (9 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_DOCPRD            (10 + WINDOWS_ID_BIAS)
	#define CTLCAL_SBIICFG_DOCPRD         (11 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_OPLIST            (12 + WINDOWS_ID_BIAS)
	#define CTL_SBIICFG_PRINTER           (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_SBIICFG_PRINTER        (14 + WINDOWS_ID_BIAS)
#define DLG_SBIIOPLI             1061
	// CTL_LBXSEL_...
#define DLG_SBIIRESTR            1062
	#define CTL_SBIIRESTR_ERRLIST         (1 + WINDOWS_ID_BIAS)
	#define CTL_SBIIRESTR_ACTION          (2 + WINDOWS_ID_BIAS)
	#define CTL_SBIIRESTR_BHTOP           (3 + WINDOWS_ID_BIAS)
	#define CTL_SBIIRESTR_HOSTOP          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SBIIRESTR_HOSTOP       (5 + WINDOWS_ID_BIAS)
	#define CTL_SBIIRESTR_FLAGS           (6 + WINDOWS_ID_BIAS)
//                               1063 // Occupied by DLG_WHCOLUMN
//                               1064 // Occupied by DLG_WHCELL
#define DLG_SELWZTYP             1065 // SELWZTYP Выбор типа складской зоны (зона/колонна/ячейка)
	#define cmGenWhCellSample             101
	#define CTL_SELWZTYP_WHAT             (1 + WINDOWS_ID_BIAS)
	#define CTL_SELWZTYP_ST_GENCELLC      (2 + WINDOWS_ID_BIAS)
#define DLG_LOCTRANSF            1066 // LOCTRAN Складская операция //
	#define CTL_LOCTRANSF_BILLTITLE       (1 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_TM              (2 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_LOCCODE         (3 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_LOC             (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRANSF_LOC          (5 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_OP              (6 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_GGRP            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRANSF_GGRP         (8 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_GOODS           (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRANSF_GOODS        (10 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_LOTINFO         (11 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_SERIAL          (12 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_QTTY            (13 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_PLTTYPE         (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRANSF_PLTTYPE      (15 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_PLTC            (16 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_PLT             (17 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_PCKG            (18 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_PCKGC           (19 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_BILLQTTY        (20 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRANSF_PSN             (21 + WINDOWS_ID_BIAS) // Персоналия, которой принадлежит склад CTLSEL_LOCTRANSF_LOC (domain==LOCTRFRDOMAIN_BAILMENT)
	#define CTLSEL_LOCTRANSF_PSN          (22 + WINDOWS_ID_BIAS)
#define DLG_LOCTRFILT            1067 // F_LOCTR Фильтр по складским операциям
	#define CTL_LOCTRFILT_LOC             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRFILT_LOC          (2 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRFILT_GGRP            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRFILT_GGRP         (4 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRFILT_GOODS           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCTRFILT_GOODS        (6 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRFILT_FLAGS           (7 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRFILT_MODE            (8 + WINDOWS_ID_BIAS)
	#define CTL_LOCTRFILT_DOMAIN          (9 + WINDOWS_ID_BIAS) // @v12.4.1
#define DLG_LOCCFG               1068 // CF_LOC Конфигурация локаций
	#define CTL_LOCCFG_WHZONECOD          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCCFG_WHZONECOD       (2 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_WHCOLCOD           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCCFG_WHCOLCOD        (4 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_WHCELLCOD          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCCFG_WHCELLCOD       (6 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_FLAGS              (7 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_DIVCOD             (8 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_DEFPALTYPE         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCCFG_DEFPALTYPE      (10 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_ADDRCODETEMPL      (12 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_STRIDXTAG          (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_LOCCFG_STRIDXTAG       (14 + WINDOWS_ID_BIAS)
	#define CTL_LOCCFG_MPWHFLD            (15 + WINDOWS_ID_BIAS) // @v12.2.1 MarketplaceWarehoustFolderID
	#define CTLSEL_LOCCFG_MPWHFLD         (16 + WINDOWS_ID_BIAS) // @v12.2.1 MarketplaceWarehoustFolderID
#define DLG_PALMFLT              1069 // F_PALM Фильтр КПК
	#define CTL_PALMFLT_LOC               (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMFLT_LOC            (2  + WINDOWS_ID_BIAS)
	#define CTL_PALMFLT_GOODSGRP          (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMFLT_GOODSGRP       (4  + WINDOWS_ID_BIAS)
	#define CTL_PALMFLT_GROUP             (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMFLT_GROUP          (6  + WINDOWS_ID_BIAS)
	#define CTL_PALMFLT_AGENT             (7  + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMFLT_AGENT          (8  + WINDOWS_ID_BIAS)
	#define CTL_PALMFLT_FTPACC            (9  + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMFLT_FTPACC         (10 + WINDOWS_ID_BIAS)
	#define CTL_PALMFLT_ORDOP             (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_PALMFLT_ORDOP          (12 + WINDOWS_ID_BIAS)
	#define CTL_PALMFLT_TYPE              (13 + WINDOWS_ID_BIAS)
#define DLG_OPRWMS               1070 // OPKWMS Вид складской операции
	// Управляющие элементы те же, что и в DLG_OPRKIND
#define DLG_SELNEWPSNOPK         1071
	#define CTL_SELNEWPSNOPK_WHAT     (1 + WINDOWS_ID_BIAS)
#define DLG_EDDIVSUM             1072 // Редактирование типов сумм и календарей штатного подразделени
	#define CTL_EDDIVSUM_DIV          (1 + WINDOWS_ID_BIAS)
	#define CTL_EDDIVSUM_SUM          (2 + WINDOWS_ID_BIAS)
#define DLG_VIEWSUMDIV           1073
	#define CTL_VIEWSUMDIV_SUM        (1 + WINDOWS_ID_BIAS)
	#define CTL_VIEWSUMDIV_DIV        (2 + WINDOWS_ID_BIAS)
#define DLG_SELNEWSTAFFAMT       1074
	#define CTL_SELNEWSTAFFAMT_WHAT   (1 + WINDOWS_ID_BIAS)
#define DLG_PRNSAL               1075
	#define CTL_PRNSAL_WHAT           (1 + WINDOWS_ID_BIAS)
#define DLG_LOCPRNVIEW           1076
// CTL_OBJVIEW_...
#define DLG_IMPEXPGOODS          1077
	#define CTL_IMPEXPGOODS_SUBCODE    (1  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_SUPPL      (2  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_SUPPL   (3  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_UNIT       (4  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_UNIT    (5  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_PHUNI      (6  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_PHUNI   (7  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_GRP        (8  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_GRP     (9  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_OP         (10 + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_OP      (11 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_LOC        (12 + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_LOC     (13 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_FLAGS      (14 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_MXACT      (15 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPGOODS_ACS        (16 + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPGOODS_ACS     (17 + IMPEXP_CTL_BIAS)
#define DLG_PSNOPKINDVIEW           1078
// DLG_OBJVIEW_...
#define DLG_FLTADDPSNEV             1079
	#define CTL_FLTADDPSNEV_DATE       (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_FLTADDPSNEV_DATE    (2 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_OP         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTADDPSNEV_OP      (4 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_PRMR       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTADDPSNEV_PRMR    (6 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_SCND       (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTADDPSNEV_SCND    (8 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_TAGS       (9 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_PSCARD     (10 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_SSCARD     (11 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_INTAL      (12 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_DVC        (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTADDPSNEV_DVC     (14 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_RDCYCLE    (15 + WINDOWS_ID_BIAS)
	#define CTL_FLTADDPSNEV_RDTIME     (16 + WINDOWS_ID_BIAS)
#define DLG_DUPNAMES                1080
// DLG_LBXSEL_...
#define DLG_VCALPAR                 1081
	#define CTL_VCALPAR_CREATOR        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_VCALPAR_CREATOR     (2 + WINDOWS_ID_BIAS)
	#define CTL_VCALPAR_CLIENT         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_VCALPAR_CLIENT      (4 + WINDOWS_ID_BIAS)
	#define CTL_VCALPAR_EMPL           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_VCALPAR_EMPL        (6 + WINDOWS_ID_BIAS)
	#define CTL_VCALPAR_FILE           (7 + WINDOWS_ID_BIAS)
	#define CTLBRW_VCALPAR_FILE        (8 + WINDOWS_ID_BIAS)
#define DLG_PRINT2                  1082 // PRINT2
	#define cmPrntCfg            101 // @erik v10.4.10
	#define cmPrintExportParam   102 // @v12.3.9 Команда выбора специфических параметров экспорта
	#define CTL_PRINT2_ACTION          (1 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_REPORT          (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRINT2_REPORT       (3 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_ST_FILENAME     (4 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_ST_FILEDTTM     (5 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_ST_DATANAME     (6 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_NUMCOPIES       (7 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_MAKEDATA        (8 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_MAKEDATAPATH    (9 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_PRINTER         (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRINT2_PRINTER      (11 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_DOMAIL          (12 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_DUPLEX          (13 + WINDOWS_ID_BIAS)
	#define CTL_PRINT2_EXPFMT          (14 + WINDOWS_ID_BIAS) // @v12.3.9
	#define CTLSEL_PRINT2_EXPFMT       (15 + WINDOWS_ID_BIAS) // @v12.3.9
#define DLG_DBDUMP                  1083 // dbdump
	#define CTL_DBDUMP_DB              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBDUMP_DB           (2 + WINDOWS_ID_BIAS)
	#define CTL_DBDUMP_TBL             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBDUMP_TBL          (4 + WINDOWS_ID_BIAS)
	#define CTL_DBDUMP_ACTION          (5 + WINDOWS_ID_BIAS)
	#define CTL_DBDUMP_FILE            (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_DBDUMP_FILE         (7 + WINDOWS_ID_BIAS)
	#define CTL_DBDUMP_SPCOB           (8 + WINDOWS_ID_BIAS)
#define DLG_IMPEXPVIEW              1084 // ieview
#define DLG_IMPEXPBILH              1085 // IE_BILH Параметры импорта/экспорта заголовков документов
	#define CTL_IMPEXPBILH_FLAGS       (IMPEXP_CTL_BIAS+1)
	#define CTL_IMPEXPBILH_IMPOP       (IMPEXP_CTL_BIAS+2)
	#define CTLSEL_IMPEXPBILH_IMPOP    (IMPEXP_CTL_BIAS+3)
	#define CTL_IMPEXPBILH_SRCHCODE1   (IMPEXP_CTL_BIAS+4)
	#define CTL_IMPEXPBILH_SRCHCODE2   (IMPEXP_CTL_BIAS+5)
	#define CTL_IMPEXPBILH_PDFMT       (IMPEXP_CTL_BIAS+6)
	#define CTLSEL_IMPEXPBILH_PDFMT    (IMPEXP_CTL_BIAS+7)
	#define CTL_IMPEXPBILH_REGTAG      (IMPEXP_CTL_BIAS+8) // @v11.5.6
	#define CTLSEL_IMPEXPBILH_REGTAG   (IMPEXP_CTL_BIAS+9) // @v11.5.6
	#define CTL_IMPEXPBILH_OUTRFMTV    (IMPEXP_CTL_BIAS+10) // @v11.6.5
#define DLG_IMPEXPCFGS              1086
	#define CTL_IMPEXPCFGS_CFGS        (1 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXPCFGS_CFGNAME     (2 + WINDOWS_ID_BIAS)
	#define CTL_IMPEXPCFGS_LIST        (3 + WINDOWS_ID_BIAS)
#define DLG_SELNEWSEC               1087 // SELSEC Выбор нового объекта для создания //
	#define CTL_SELNEWSEC_SEL          (1 + WINDOWS_ID_BIAS)
#define DLG_FLTTRANSP               1088
	#define CTL_FLTTRANSP_MODEL        (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTTRANSP_MODEL     (2  + WINDOWS_ID_BIAS)
	#define CTL_FLTTRANSP_OWNER        (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTTRANSP_OWNER     (4  + WINDOWS_ID_BIAS)
	#define CTL_FLTTRANSP_CAPTAIN      (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTTRANSP_CAPTAIN   (6  + WINDOWS_ID_BIAS)
	#define CTL_FLTTRANSP_CNTRY        (7  + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTTRANSP_CNTRY     (8  + WINDOWS_ID_BIAS)
	#define CTL_FLTTRANSP_CODE         (9  + WINDOWS_ID_BIAS)
	#define CTL_FLTTRANSP_TRAILCODE    (10 + WINDOWS_ID_BIAS)
	#define CTL_FLTTRANSP_TRTYPE       (11 + WINDOWS_ID_BIAS)
#define DLG_REGCOMSVR               1089
	#define CTL_REGCOMSVR_ACTION       (1 + WINDOWS_ID_BIAS)
	#define CTL_REGCOMSVR_DL200INST    (2 + WINDOWS_ID_BIAS) // @obsolete
	#define CTL_REGCOMSVR_FLAGS        (2 + WINDOWS_ID_BIAS)
	#define CTL_REGCOMSVR_INFO         (3 + WINDOWS_ID_BIAS)
#define DLG_QCERTTOTAL              1090 // Итоги по выборке сертификатов
	#define CTL_QCERTTOTAL_COUNT       (1 + WINDOWS_ID_BIAS)
#define DLG_SSTATORDFLTEXT          1091 // Расширение фильтра по статистике продаж с заказом поставщику
#define DLG_CLIAGTRND               1092 // CLIAGT_R Параметры округления цен в документах
	#define CTL_CLIAGTRND_FLAGS        (1 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGTRND_ROUND        (2 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGTRND_PREC         (3 + WINDOWS_ID_BIAS)
	#define CTL_CLIAGTRND_ROUNDVAT     (4 + WINDOWS_ID_BIAS)
#define DLG_UNITVIEW                1093 // Диалог просмотра списка единиц измерения товара
#define DLG_BBTSEL                  1094
	#define CTL_BBTSEL_TYPE            (1 + WINDOWS_ID_BIAS)
#define DLG_COSTDISTR               1095 // COSTDIST
	#define CTL_COSTDISTR_ALG          (1 + WINDOWS_ID_BIAS)
	#define CTL_COSTDISTR_FLAGS        (2 + WINDOWS_ID_BIAS)
#define DLG_OPENFILE                1096
	#define cmBrowseFile               110
	#define cmBrowseDir                111
	#define CTL_OPENFILE_PATH          (1 + WINDOWS_ID_BIAS)
#define DLG_XLSDBPARAM              1097
	#define CTL_XLSDBPARAM_FLAGS       (1 + WINDOWS_ID_BIAS)
	#define CTL_XLSDBPARAM_HDRCOUNT    (2 + WINDOWS_ID_BIAS)
	#define CTL_XLSDBPARAM_SHEETNUM    (3 + WINDOWS_ID_BIAS)
	#define CTL_XLSDBPARAM_SHEETNAME   (4 + WINDOWS_ID_BIAS)
	#define CTL_XLSDBPARAM_ORIENT      (5 + WINDOWS_ID_BIAS)
	#define CTL_XLSDBPARAM_ENDSTR      (6 + WINDOWS_ID_BIAS)
	#define CTL_XLSDBPARAM_COLCOUNT    (7 + WINDOWS_ID_BIAS)
#define DLG_FLTAMTTYPE              1098
	// CTL_AMOUNTTYPE_...
#define DLG_FLTREGT                 1099
	#define CTL_FLTREGT_REGPKIND       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTREGT_REGPKIND    (2 + WINDOWS_ID_BIAS)
	#define CTL_FLTREGT_PKIND          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTREGT_PKIND       (4 + WINDOWS_ID_BIAS)
	#define CTL_FLTREGT_ST             (5 + WINDOWS_ID_BIAS)
#define DLG_GVR                     1100 // GVR Ограничения товарных величин
	#define cmShipmControl             101
	#define CTL_GVR_NAME               (1 + WINDOWS_ID_BIAS)
	#define CTL_GVR_SYMB               (2 + WINDOWS_ID_BIAS)
	#define CTL_GVR_ID                 (3 + WINDOWS_ID_BIAS)
	#define CTL_GVR_LOWBFORM           (4 + WINDOWS_ID_BIAS)
	#define CTL_GVR_UPPBFORM           (5 + WINDOWS_ID_BIAS)
	#define CTL_GVR_BARLIST            (6 + WINDOWS_ID_BIAS)
#define DLG_SPLITSUSCHK              1101
#define DLG_SPLITSUSCHK_L            1102
	#define CTL_SPLITSUSCHK_AGENT      (1 + WINDOWS_ID_BIAS)
	#define CTL_SPLITSUSCHK_AMOUNT     (2 + WINDOWS_ID_BIAS)
	#define CTL_SPLITSUSCHK_TABLE      (3 + WINDOWS_ID_BIAS)
	#define CTL_SPLITSUSCHK_LIST1      (4 + WINDOWS_ID_BIAS)
	#define CTL_SPLITSUSCHK_LIST2      (5 + WINDOWS_ID_BIAS)
#define DLG_BILLEXPFILT              1103
	#define CTL_BILLEXPFILT_CFG        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXPFILT_CFG     (2 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXPFILT_RCFG       (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BILLEXPFILT_RCFG    (4 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXPFILT_BILLTYP	   (5 + WINDOWS_ID_BIAS)
	#define CTL_BILLEXPFILT_FLAGS	   (6 + WINDOWS_ID_BIAS)
#define DLG_SCARDVIEW                1104 // CARDVIEW
	#define cmCheckOpSwitch            101
	#define cmSelectByOwner            102
	#define cmSCardMovCrd              103
	#define cmEditPerson               104
	#define cmCharge                   105
	#define CTL_SCARDVIEW_LIST         (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_INPUT        (2 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_IMAGE        (3 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_SALDO        (4 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_OWNER        (5 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_CARD         (6 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_SWITCHBTN    (7 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_SCINFO       (8 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_OWNERINFO    (9 + WINDOWS_ID_BIAS)
	#define CTL_SCARDVIEW_MOVBTN       (10 + WINDOWS_ID_BIAS)
#define DLG_REP_APRD                 1105 // Subdialog of DLG_REPEATING (CTL_REPEATING_XXX)
#define DLG_TSESSSIMPLE              1106 // CTL_TSESS_...
#define DLG_CRTTSESSFLT              1107
	#define CTL_CRTTSESSFLT_GRP        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CRTTSESSFLT_GRP     (2 + WINDOWS_ID_BIAS)
	#define CTL_CRTTSESSFLT_PRC        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CRTTSESSFLT_PRC     (4 + WINDOWS_ID_BIAS)
	#define CTL_CRTTSESSFLT_STATUS     (5 + WINDOWS_ID_BIAS)
	#define CTL_CRTTSESSFLT_WTM        (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_CRTTSESSFLT_WTM     (7 + WINDOWS_ID_BIAS)
#define DLG_CORTRFRREV               1108 // c_trfrr
	#define CTL_CORTRFRREV_PERIOD      (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_CORTRFRREV_PERIOD   (2 + WINDOWS_ID_BIAS)
	#define CTL_CORTRFRREV_FLAGS       (3 + WINDOWS_ID_BIAS)
	#define CTL_CORTRFRREV_LOG         (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORTRFRREV_LOG      (5 + WINDOWS_ID_BIAS)
#define DLG_DLVREXTFLDS              1109 // CTL_LBXSEL_...
#define DLG_ADDEXTFLD                1110
	#define CTL_ADDEXTFLD_FLD          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADDEXTFLD_FLD       (2 + WINDOWS_ID_BIAS)
	#define CTL_ADDEXTFLD_VAL          (3 + WINDOWS_ID_BIAS)
#define DLG_EXTFLDCFG                1111
	#define CTL_EXTFLDCFG_ID           (1 + WINDOWS_ID_BIAS)
	#define CTL_EXTFLDCFG_NAME         (2 + WINDOWS_ID_BIAS)
									 // 1112 @occupied
#define DLG_ACCBUDGET                1113
// CTL_ACCOUNT_...
#define DLG_BUDGFLT                  1114
	#define CTL_BUDGFLT_PERIOD         (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_BUDGFLT_PERIOD      (2 + WINDOWS_ID_BIAS)
	#define CTL_BUDGFLT_BUDGET         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BUDGFLT_BUDGET      (4 + WINDOWS_ID_BIAS)
#define DLG_BUDGET                   1115
	#define cmScenaries                101
	#define CTL_BUDGET_ID              (1 + WINDOWS_ID_BIAS)
	#define CTL_BUDGET_NAME            (2 + WINDOWS_ID_BIAS)
	#define CTL_BUDGET_CODE            (3 + WINDOWS_ID_BIAS)
	#define CTL_BUDGET_PERIOD          (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_BUDGET_PERIOD       (5 + WINDOWS_ID_BIAS)
	#define CTL_BUDGET_CYCLE           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_BUDGET_CYCLE        (7 + WINDOWS_ID_BIAS)
#define DLG_BUDGITEM                 1116
	#define CTL_BUDGITEM_DT            (1  + WINDOWS_ID_BIAS)
	#define CTLCAL_BUDGITEM_DT         (2  + WINDOWS_ID_BIAS)
	#define CTL_BUDGITEM_BUDGET        (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_BUDGITEM_BUDGET     (4  + WINDOWS_ID_BIAS)
	#define CTL_BUDGITEM_ACCT          (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_BUDGITEM_ACCT       (6  + WINDOWS_ID_BIAS)
	#define CTL_BUDGITEM_KIND          (7  + WINDOWS_ID_BIAS)
	#define CTL_BUDGITEM_AMOUNT        (8  + WINDOWS_ID_BIAS)
	#define CTL_BUDGITEM_MEMO          (9  + WINDOWS_ID_BIAS)
	#define CTL_BUDGITEM_LIST          (10 + WINDOWS_ID_BIAS)
#define DLG_SUPPLEOPS                1117
	#define CTL_SUPPLEOPS_EXP          (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_EXP	   (2  + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_RCPT         (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_RCPT	   (4  + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_SUPRET       (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_SUPRET	   (6  + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_RET          (7  + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_RET	   (8  + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_LOSS         (9  + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_LOSS	   (10 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_INV          (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_INV	   (12 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_MOVIN        (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_MOVIN	   (14 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_MOVOUT       (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_MOVOUT	   (16 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_DBTDIM       (17 + WINDOWS_ID_BIAS) // Список долговых размерностей
	#define CTL_SUPPLEOPS_UNIT         (18 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_UNIT      (19 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_CLICTAG      (20 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_CLICTAG   (21 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_LOCCTAG      (22 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_LOCCTAG   (23 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_BACKTAG      (24 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_BACKTAG   (25 + WINDOWS_ID_BIAS)
	#define CTL_SUPPLEOPS_LOC          (26 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPPLEOPS_LOC       (27 + WINDOWS_ID_BIAS)
#define DLG_BUDGTOTAL                1118
	#define CTL_BUDGTOTAL_PLAN         (1 + WINDOWS_ID_BIAS)
	#define CTL_BUDGTOTAL_FACT         (2 + WINDOWS_ID_BIAS)
	#define CTL_BUDGTOTAL_DIFF         (3 + WINDOWS_ID_BIAS)
	#define CTL_BUDGTOTAL_DEFICITTXT   (4 + WINDOWS_ID_BIAS)
#define DLG_WTMTOOLPARAM             1119 // WTMTOOLP
	#define CTL_WTMTOOLPARAM_NAME      (1 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOLPARAM_SYMB      (2 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOLPARAM_PICSIZE   (3 + WINDOWS_ID_BIAS)
#define DLG_WTMTOOL                  1120 // WTMTOOL
	#define CTL_WTMTOOL_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_SYMB           (2 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_CMD            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_WTMTOOL_CMD         (4 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_FIGPATH        (5 + WINDOWS_ID_BIAS)
	#define CTLBRW_WTMTOOL_FIGPATH     (6 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_FIGSIZE        (7 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_PICPATH        (8 + WINDOWS_ID_BIAS)
	#define CTLBRW_WTMTOOL_PICPATH     (9 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_PICSIZE        (10 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_WTMOBJ         (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_WTMTOOL_WTMOBJ      (12 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_FLAGS          (13 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_BKGOPTIONS     (14 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_KIND           (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_WTMTOOL_KIND        (16 + WINDOWS_ID_BIAS)
	#define CTL_WTMTOOL_CLOT           (17 + WINDOWS_ID_BIAS) // @v11.2.2 complex layout type
	#define CTLSEL_WTMTOOL_CLOT        (18 + WINDOWS_ID_BIAS) // @v11.2.2 complex layout type
#define DLG_MOVARCODE                1121 // MOVARCOD
	#define CTL_MOVARCODE_DESTAR       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MOVARCODE_DESTAR    (2 + WINDOWS_ID_BIAS)
	#define CTL_MOVARCODE_SRCAR        (3 + WINDOWS_ID_BIAS)
	#define CTL_MOVARCODE_GGRP         (4 + WINDOWS_ID_BIAS)
	#define CTL_MOVARCODE_ST_WARN      (5 + WINDOWS_ID_BIAS)
	#define CTL_MOVARCODE_FLAGS        (6 + WINDOWS_ID_BIAS) // @v11.7.5
#define DLG_BUDGITEMS                1122
// CTL_BUDGITEM_...
#define DLG_BUDGICYCLEPAR            1123
	#define CTL_BUDGICYCLEPAR_AMT      (1 + WINDOWS_ID_BIAS)
	#define CTL_BUDGICYCLEPAR_MEMO     (2 + WINDOWS_ID_BIAS)
#define DLG_GTANLZTOTAL_PIL          1124 // Итог налогового анализа товарооборота (тетрадь частн предпринимателя по приходам)
// CTL_GTANLZTOTAL_...
#define DLG_WOTEXT                   1125 // WhatmanObjectText
	#define CTL_WOTEXT_TEXT            (1 + WINDOWS_ID_BIAS)
#define DLG_WOCAFETBL                1126 // WhatmanObjectCafeTable
	#define CTL_WOCAFETBL_N            (1 + WINDOWS_ID_BIAS)
#define DLG_BIZSCT                   1127
	#define cmEditCell                 101
	#define CTL_BIZSCT_ID              (1 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCT_SYMB            (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCT_NAME            (3 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCT_LIST            (4 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCT_ROWS            (5 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCT_CELLTEXT        (6 + WINDOWS_ID_BIAS)
#define DLG_BIZSCTI                  1128
	#define CTL_BIZSCTI_CODE           (1 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTI_NAME           (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTI_PERIOD         (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZSCTI_PERIOD      (4 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTI_TYPE           (5 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTI_FLAGS          (6 + WINDOWS_ID_BIAS)
#define DLG_BIZSCTR                  1129
	#define CTL_BIZSCTR_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTR_CODE          (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTR_NAME          (3 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTR_BIZSC         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCTR_BIZSC      (5 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTR_FORMULA       (6 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTR_TESTLINE      (7 + WINDOWS_ID_BIAS)
#define DLG_FLTBIZSCVT               1130
	#define CTL_FLTBIZSCVT_TEMPL         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTBIZSCVT_TEMPL      (2 + WINDOWS_ID_BIAS)
#define DLG_BIZSCTC                  1131
	#define CTL_BIZSCTC_COLID         (1 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTC_ROWID         (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTC_BIZSC         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSCTC_BIZSC      (4 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTC_FORMULA       (5 + WINDOWS_ID_BIAS)
	#define CTL_BIZSCTC_TESTLINE      (6 + WINDOWS_ID_BIAS)
#define DLG_CHKOPJFILT               1132
	#define cmChkOpJActionList        101
	#define CTL_CHKOPJFILT_PERIOD     (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_CHKOPJFILT_PERIOD  (2 + WINDOWS_ID_BIAS)
	#define CTL_CHKOPJFILT_ACTION     (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKOPJFILT_ACTION  (4 + WINDOWS_ID_BIAS)
	#define CTL_CHKOPJFILT_USER       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKOPJFILT_USER    (6 + WINDOWS_ID_BIAS)
	#define CTL_CHKOPJFILT_AGENT      (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKOPJFILT_AGENT   (8 + WINDOWS_ID_BIAS)
#define DLG_WTMPARAM                 1133
	#define CTL_WTMPARAM_NAME         (1 + WINDOWS_ID_BIAS)
	#define CTL_WTMPARAM_SYMB         (2 + WINDOWS_ID_BIAS)
	#define CTL_WTMPARAM_UNIT         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_WTMPARAM_UNIT      (4 + WINDOWS_ID_BIAS)
	#define CTL_WTMPARAM_UNITFACTOR   (5 + WINDOWS_ID_BIAS)
	#define CTL_WTMPARAM_SCALE        (6 + WINDOWS_ID_BIAS)
	#define CTL_WTMPARAM_FLAGS        (7 + WINDOWS_ID_BIAS)
#define DLG_RTPOSAGENT               1134 // RT_POSA
	#define CTL_RTPOSAGENT_FLAGS      (1 + WINDOWS_ID_BIAS)
//                                   1135 // @occupied DLG_TI_LOCTR
#define DLG_PALLET                   1136 // PALLET
	#define CTL_PALLET_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_PALLET_SYMB           (2 + WINDOWS_ID_BIAS)
	#define CTL_PALLET_ID             (3 + WINDOWS_ID_BIAS)
	#define CTL_PALLET_WIDTH          (4 + WINDOWS_ID_BIAS)
	#define CTL_PALLET_LENGTH         (5 + WINDOWS_ID_BIAS)
	#define CTL_PALLET_MAXLOAD        (6 + WINDOWS_ID_BIAS)
//                                   1137 // @occupied DLG_CITYAREA
//                                   1138 // @occupied DLG_CASHNG
#define DLG_WTMTOOLBKG               1139 // WTMTBKG
	// CTL_WTMTOOL_XXX
#define DLG_COMPLDIN                 1140
	#define cmInputQtty               101
	#define CTL_COMPLDIN_ELEMENTS     (1 + WINDOWS_ID_BIAS)
	#define CTL_COMPLDIN_ALTLIST      (2 + WINDOWS_ID_BIAS)
	#define CTL_COMPLDIN_TOTAL        (3 + WINDOWS_ID_BIAS)
	#define BTN_COMPLDIN_INPUTQTTY    (4 + WINDOWS_ID_BIAS)
#define DLG_TSESSTM                  1141 // TSESSTM Технологическая сессия для повременного процессора //
#define DLG_WOPRC                    1142 // WO_PRC
	#define CTL_WOPRC_PRC             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_WOPRC_PRC          (2 + WINDOWS_ID_BIAS)
#define DLG_CTBLORD                  1143 // CTBLORD
	#define CTL_CTBLORD_TBL           (1 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_CCN           (2 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_CCDT          (3 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_CCTM          (4 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_STDT          (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_CTBLORD_STDT       (6 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_STTM          (7 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_FNDT          (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_CTBLORD_FNDT       (9 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_FNTM          (10 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_CONT          (11 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_SCARD         (12 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_PREPAY        (13 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_POSNODE       (14 + WINDOWS_ID_BIAS)
	#define CTLSEL_CTBLORD_POSNODE    (15 + WINDOWS_ID_BIAS)
	#define CTLTM_CTBLORD_STTM        (16 + WINDOWS_ID_BIAS)
	#define CTLTM_CTBLORD_FNTM        (17 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_ST_SCREST     (18 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_FLAGS         (19 + WINDOWS_ID_BIAS)
	#define CTL_CTBLORD_MEMO          (20 + WINDOWS_ID_BIAS)
#define DLG_CTBLORDPAR               1144 // CTBLORDPAR
	// CTL_CTBLORD_XXX
#define DLG_WHCELLSAMPLE             1145 // WHCELLSA
	// CTL_LOCATION_XXX
#define DLG_TMPICKR                  1146 // Диалог для выбора времени
	#define cmCurTime                101
#define DLG_ORDERCHECKS              1147 // Редкатирование заказов столиков
#define DLG_ORDERCHECKS_L            1148
// CTL_SELCHECK_...
#define DLG_CPPAYM                   1149 // CPPAYM Диалог приема оплаты в кассовой панели // @v12.0.6 removed (replaced with DLG_CPPAYM2)
	#define CTL_CPPAYM_KIND           (1 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_BNKAMT         (2 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CSHAMT         (3 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_AMOUNT         (4 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CASH           (5 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_DIFF           (6 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CRDCARD        (7 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CRDCARDOWNER   (8 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CRDCARDREST    (9 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CRDCARDAMT     (10 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_USEBONUS       (11 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_IMG_CASH       (12 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_IMG_BANK       (13 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_IMG_SCARD      (14 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CCRDLIST       (15 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_ALTCASHREG     (16 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_EADDR          (17 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_EADDRINF       (18 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_PAPERLESS      (19 + WINDOWS_ID_BIAS)
	#define CTLFRAME_CPPAYM_PAPERLESS (20 + WINDOWS_ID_BIAS)
	#define CTL_CPPAYM_CASHLESSBPEQ   (21 + WINDOWS_ID_BIAS) // @v12.0.6 checkbox
// DLG_TAGVGUID                      1150
#define DLG_QUOTKINDRESTR            1151 // Ограничения применимости вида котировки
	#define CTL_QKRESTR_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_QKRESTR_PERIOD     (2 + WINDOWS_ID_BIAS)
	#define CTL_QKRESTR_TIMERANGE     (3 + WINDOWS_ID_BIAS)
	#define CTL_QKRESTR_WEEKDAYS      (4 + WINDOWS_ID_BIAS)
	#define CTL_QKRESTR_AMTRANGE      (5 + WINDOWS_ID_BIAS)
// DLG_PERSON_S1                     1152 //
#define DLG_STOCKOPTCFG              1153 // CF_STOPT
#define DLG_STOCKOPTFLT              1154 // F_STOPT
	#define CTL_STOCKOPTCFG_RATE      (1 + WINDOWS_ID_BIAS)
	#define CTL_STOCKOPTCFG_EXPSAFC   (2 + WINDOWS_ID_BIAS)
	#define CTL_STOCKOPTCFG_MAXCOST   (3 + WINDOWS_ID_BIAS)
	#define CTL_STOCKOPTCFG_MAXSALES  (4 + WINDOWS_ID_BIAS)
	#define CTL_STOCKOPTCFG_MAXITEMS  (5 + WINDOWS_ID_BIAS)

	#define CTL_STOCKOPTCFG_MODE      (6 + WINDOWS_ID_BIAS)
	#define CTL_STOCKOPTCFG_ORDER     (7 + WINDOWS_ID_BIAS)
#define DLG_STOCKOPTITEM             1155 // STOPTITM
	#define CTL_SOI_GOODSGRP          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SOI_GOODSGRP       (2 + WINDOWS_ID_BIAS)
	#define CTL_SOI_GOODS             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SOI_GOODS          (4 + WINDOWS_ID_BIAS)
	#define CTL_SOI_COST              (5 + WINDOWS_ID_BIAS)
	#define CTL_SOI_PRICE             (6 + WINDOWS_ID_BIAS)
	#define CTL_SOI_DEMAND            (7 + WINDOWS_ID_BIAS)
	#define CTL_SOI_EXPIRYPERIOD      (8 + WINDOWS_ID_BIAS)
	#define CTL_SOI_MINREST           (9 + WINDOWS_ID_BIAS)
	#define CTL_SOI_INREST            (10 + WINDOWS_ID_BIAS)
	#define CTL_SOI_PCKG              (11 + WINDOWS_ID_BIAS)
#define DLG_RFIDDEV                  1156
	#define CTL_RFIDDEV_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_NUMBER        (2 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_NAME          (3 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_SYMB          (4 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_PORT          (5 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_CBR           (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_RFIDDEV_CBR        (7 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_STOPBITS      (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_RFIDDEV_STOPBITS   (9 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_DATABITS      (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_RFIDDEV_DATABITS   (11 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_PARITY        (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_RFIDDEV_PARITY     (13 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_GETTRIES      (14 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_PUTTRIES      (15 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_GETTIMEOUT    (16 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_ST_INFO       (17 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_RELECOUNT     (18 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_GGRP          (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_RFIDDEV_GGRP       (20 + WINDOWS_ID_BIAS)
	#define CTL_RFIDDEV_GOODS         (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_RFIDDEV_GOODS      (22 + WINDOWS_ID_BIAS)
#define DLG_INFOKIOSKFLT             1158
	#define CTL_INFOKIOSKFLT_NODE     (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_INFOKIOSKFLT_NODE  (2 + WINDOWS_ID_BIAS)
	#define CTL_INFOKIOSKFLT_GRP      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_INFOKIOSKFLT_GRP   (4 + WINDOWS_ID_BIAS)
#define DLG_WTMTOOLUI                1159 // WTMTUI
	// CTL_WTMTOOL_XXX
#define DLG_SSTATTOTAL               1160
	#define CTL_SSTATTOTAL_LNCOUNT    (1 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_COUNT      (2 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_QTTY       (3 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_AMOUNT     (4 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_ORDCOUNT   (5 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_ORDQTTY    (6 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_ORDAMOUNT  (7 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_UNCRTCNT   (8 + WINDOWS_ID_BIAS)
	#define CTL_SSTATTOTAL_ORDCOST    (9 + WINDOWS_ID_BIAS)
#define DLG_CROSST                      1161 // параметры кросстабуляции
	#define CTL_CROSST_KIND           (1 + WINDOWS_ID_BIAS)
	#define CTL_CROSST_VALLIST        (2 + WINDOWS_ID_BIAS)
	#define CTL_CROSST_SHOWVAL        (3 + WINDOWS_ID_BIAS)
#define DLG_ADDRSTRUC                1162 // ADDRSTR
	#define CTL_ADDRSTRUC_SRC         (1 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_ZIP         (2 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_COUNTRY     (3 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_CITY        (4 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_LOCALAREA   (5 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_STREET      (6 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_HOUSE       (7 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_HOUSEADD    (8 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_APART       (9 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_FLOOR       (10 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_POSTBOX     (11 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_ADDENDUM    (12 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_FIASID      (13 + WINDOWS_ID_BIAS)
	#define CTL_ADDRSTRUC_FIASHSEID   (14 + WINDOWS_ID_BIAS)
#define DLG_SENDMAIL                 1163
	#define CTL_SENDMAIL_ACCNT        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SENDMAIL_ACCNT     (2 + WINDOWS_ID_BIAS)
	#define CTL_SENDMAIL_ADDR         (3 + WINDOWS_ID_BIAS)
	#define CTL_SENDMAIL_SUBJ         (4 + WINDOWS_ID_BIAS)
	#define CTL_SENDMAIL_TEXT         (5 + WINDOWS_ID_BIAS)
	#define CTL_SENDMAIL_ATTACHLIST   (6 + WINDOWS_ID_BIAS)
	#define CTL_SENDMAIL_DELAY        (7 + WINDOWS_ID_BIAS)
#define DLG_EMAILADDRS               1164
	#define CTL_EMAILADDRS_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_DEBTDIM                  1165
	#define CTL_DEBTDIM_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_DEBTDIM_SYMB          (2 + WINDOWS_ID_BIAS)
	#define CTL_DEBTDIM_NAME          (3 + WINDOWS_ID_BIAS)
	#define CTL_DEBTDIM_AGENTS        (4 + WINDOWS_ID_BIAS)
#define DLG_DEBTLIMLIST              1166
	#define CTL_DEBTLIMLIST_LIST      (1 + WINDOWS_ID_BIAS)
#define DLG_DBTLIMITEM               1167
	#define CTL_DBTLIMITEM_DIM        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBTLIMITEM_DIM     (2 + WINDOWS_ID_BIAS)
	#define CTL_DBTLIMITEM_LIMIT      (3 + WINDOWS_ID_BIAS)
	#define CTL_DBTLIMITEM_FLAGS      (4 + WINDOWS_ID_BIAS)
	#define CTL_DBTLIMITEM_LOCKPRCB   (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_DBTLIMITEM_LOCKPRCB (6 + WINDOWS_ID_BIAS)
#define DLG_CCHKDLVR                 1168
	#define cmSelAddrByPhone          101
	#define CTL_CCHKDLVR_MEMO         (1 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_FLAGS        (2 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_ADDRID       (3 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_CITY         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHKDLVR_CITY      (5 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_ADDR         (6 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_PHONE        (7 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_DT           (8 + WINDOWS_ID_BIAS)
	#define CTLCAL_CCHKDLVR_DT        (9 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_TM           (10 + WINDOWS_ID_BIAS)
	#define CTLTM_CCHKDLVR_TM         (11 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_CONTACT      (12 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_ST_ADDRID    (13 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_ST_ADDRMOD   (14 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_SCARD        (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCHKDLVR_SCARD     (16 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_ST_SIP       (17 + WINDOWS_ID_BIAS)
	#define CTL_CCHKDLVR_LPHTOCRD     (18 + WINDOWS_ID_BIAS)
#define DLG_FLTSCARDCHNG             1169
	#define cmSCardFilt               101
	#define CTL_FLTSCARDCHNG_FLAGS    (1 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_DTEND    (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_FLTSCARDCHNG_DTEND (3 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_SER      (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTSCARDCHNG_SER   (5 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_AG       (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTSCARDCHNG_AG    (7 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_FS       (8 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_FR       (9 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_PRD      (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTSCARDCHNG_PRD   (11 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_PRDC     (12 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_DSCNT    (13 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_ZDSCNT   (14 + WINDOWS_ID_BIAS)
	#define CTL_FLTSCARDCHNG_ZEXPIRY  (15 + WINDOWS_ID_BIAS)
#define DLG_TOGGLECCDLVR             1170
	#define CTL_TOGGLECCDLVR_INFO     (1 + WINDOWS_ID_BIAS)
	#define CTL_TOGGLECCDLVR_DT       (2 + WINDOWS_ID_BIAS)
	#define CTLCAL_TOGGLECCDLVR_DT    (3 + WINDOWS_ID_BIAS)
	#define CTL_TOGGLECCDLVR_TM       (4 + WINDOWS_ID_BIAS)
	#define CTLTM_TOGGLECCDLVR_TM     (5 + WINDOWS_ID_BIAS)
	#define CTL_TOGGLECCDLVR_FLAG     (6 + WINDOWS_ID_BIAS)
#define DLG_POSPAYMBNK               1171 // POSPBNK
	#define CTL_POSPAYMBNK_ST_TEXT    (1 + WINDOWS_ID_BIAS)
	#define CTL_POSPAYMBNK_AMOUNT     (2 + WINDOWS_ID_BIAS)
	#define CTL_POSPAYMBNK_EADDR          (17 + WINDOWS_ID_BIAS)
	#define CTL_POSPAYMBNK_EADDRINF       (18 + WINDOWS_ID_BIAS)
	#define CTL_POSPAYMBNK_PAPERLESS      (19 + WINDOWS_ID_BIAS)
	#define CTLFRAME_POSPAYMBNK_PAPERLESS (20 + WINDOWS_ID_BIAS)
	#define CTL_POSPAYMBNK_CLBPEQ         (21 + WINDOWS_ID_BIAS) // @12.0.6 checkbox cashless-bypass-equipment

#define DLG_CHKPANROWOP              1172
	#define CTL_CHKPANROWOP_VERB      (1 + WINDOWS_ID_BIAS)
	#define CTL_CHKPANROWOP_QUEUE     (2 + WINDOWS_ID_BIAS)
	#define CTLSPIN_CHKPANROWOP_QUEUE (3 + WINDOWS_ID_BIAS)
#define DLG_FILTPOOL                 1173 // Пул именованных фильтров
	#define CTL_FILTPOOL_LIST             (1 + WINDOWS_ID_BIAS)
	//#define CTL_FILTPOOL_FLAGS            (2 + WINDOWS_ID_BIAS)
#define DLG_FILTITEM              1174
	#define cmFiltParam                102
	#define cmOutFields                103
	#define CTL_FILTITEM_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_FILTITEM_NAME          (2 + WINDOWS_ID_BIAS)
	#define CTL_FILTITEM_CMD           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_FILTITEM_CMD        (4 + WINDOWS_ID_BIAS)
	#define CTL_FILTITEM_SYMB          (5 + WINDOWS_ID_BIAS)
	#define CTL_FILTITEM_FLAGS         (6 + WINDOWS_ID_BIAS)
#define DLG_CPSELMOD              1175 // CPSELMOD
	#define CTL_CPSELMOD_ELEMENTS      (1 + WINDOWS_ID_BIAS)
#define DLG_JOB_EXPVIEW           1176 // J_EXPV
	#define CTL_JOB_EXPVIEW_FILT       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_JOB_EXPVIEW_FILT    (2 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPVIEW_DL6STRUC   (3 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPVIEW_OUT        (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_JOB_EXPVIEW_OUT     (5 + WINDOWS_ID_BIAS)
#define DLG_BRANDVIEW             1177 // V_BRAND
#define DLG_UHTTSGOODS            1178 // US_GOODS
	#define CTL_UHTTSGOODS_SEL         (1 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSGOODS_TEXT        (2 + WINDOWS_ID_BIAS)
#define DLG_JOB_EXPOBJHTM         1179 // J_EOBJHT
	#define CTL_JOB_EXPOBJHTM_OUT      (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_JOB_EXPOBJHTM_OUT   (2 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_GFLAGS   (3 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_GASETS   (4 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_GRSETS   (5 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_BFLAGS   (6 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_SFLAGS   (7 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_PK       (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_JOB_EXPOBJHTM_PK    (9 + WINDOWS_ID_BIAS)
	#define CTL_JOB_EXPOBJHTM_PSETS    (10 + WINDOWS_ID_BIAS)
#define	DLG_SMSACC               1180
	#define cmSmsSys               101
	#define	CTL_SMSACC_NAME            (1 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_ID              (2 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_HOST            (3 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_PORT            (4 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_SYSTEMID        (5 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_PASSW           (6 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_FROM            (7 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_SYSTYPE         (8 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_FLAGS           (9 + WINDOWS_ID_BIAS)
	#define CTL_SMSACC_SYSBTN          (11 + WINDOWS_ID_BIAS)
	#define	CTL_SMSACC_SYMB            (12 + WINDOWS_ID_BIAS)
#define DLG_SPCSERFLT             1181
	#define CTL_SPCSERFLT_PERIOD       (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_SPCSERFLT_PERIOD    (2 + WINDOWS_ID_BIAS)
	#define CTL_SPCSERFLT_GGRP         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPCSERFLT_GGRP      (4 + WINDOWS_ID_BIAS)
	#define CTL_SPCSERFLT_GOODS        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPCSERFLT_GOODS     (6 + WINDOWS_ID_BIAS)
	#define CTL_SPCSERFLT_KIND         (7 + WINDOWS_ID_BIAS)
// DLG_TAGVIMG                    1182 occupied
#define DLG_PHNSVC                1183
	#define CTL_PHNSVC_ID              (1 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_NAME            (2 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_SYMB            (3 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_TYPE            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PHNSVC_TYPE         (5 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_FLAGS           (6 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_ADDR            (7 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_PORT            (8 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_USERNAME        (9 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_PASSWORD        (10 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_LOCALCHNLSYMB   (11 + WINDOWS_ID_BIAS)
	#define CTL_PHNSVC_SCANCHNL        (12 + WINDOWS_ID_BIAS)
#define DLG_PHNSVCVIEW            1184
#define DLG_IMPEXPPHONE           1185
	#define CTL_IMPEXPPHONE_DEFPFX     (1  + IMPEXP_CTL_BIAS)
#define DLG_IEPHONE               1186
	#define CTL_IEPHONE_CFG            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEPHONE_CFG         (2 + WINDOWS_ID_BIAS)
	#define CTL_IEPHONE_DEFCITY        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEPHONE_DEFCITY     (4 + WINDOWS_ID_BIAS)
#define DLG_SPCSER                1187
	#define CTL_SPCSER_DT              (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_SPCSER_DT           (2 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_ALLOWDT         (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_SPCSER_ALLOWDT      (4 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_GOODSNAME       (5 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_MANUFNAME       (6 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_BARCODE         (7 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_SERIAL          (8 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_GGRP            (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPCSER_GGRP         (10 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_GOODS           (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPCSER_GOODS        (12 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_MANUF           (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_SPCSER_MANUF        (14 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_FLAGS           (15 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_INFO            (16 + WINDOWS_ID_BIAS)
	#define CTL_SPCSER_INFOIDENT       (17 + WINDOWS_ID_BIAS)
#define DLG_SELADDRBYPH           1188
	#define CTL_SELADDRBYPH_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_PHNSVCSTATE           1189
	#define CTL_PHNSVCSTATE_LIST       (1 + WINDOWS_ID_BIAS)
#define DLG_GOODSORD              1190
	#define CTL_GOODSORD_ORDER         (1 + WINDOWS_ID_BIAS)
#define DLG_CVTQCERT              1191
	#define CTL_CVTQCERT_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_CVTQCERT_PERIOD     (2 + WINDOWS_ID_BIAS)
	#define CTL_CVTQCERT_GGRP          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTQCERT_GGRP       (4 + WINDOWS_ID_BIAS)
	#define CTL_CVTQCERT_PSNKIND       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTQCERT_PSNKIND    (6 + WINDOWS_ID_BIAS)
	#define CTL_CVTQCERT_LOTTAG        (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTQCERT_LOTTAG     (8 + WINDOWS_ID_BIAS)
#define DLG_CASHNSEXT             1192
	#define CTL_CASHNSEXT_SCF_DAYS     (1 + WINDOWS_ID_BIAS)
	#define CTLSPIN_CASHNSEXT_SCF_DAYS (2 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_SCF_DLVRT    (3 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_BONUSMAX     (4 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_SLEEPTIMEOUT (5 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_CTBLLIST     (6 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_FLAGS        (7 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_NOTSF        (8 + WINDOWS_ID_BIAS)
	#define CTL_CASHNSEXT_ZEROBONUS    (9 + WINDOWS_ID_BIAS) // @v11.4.9
	#define CTL_CASHNSEXT_ALDPAYMT     (10 + WINDOWS_ID_BIAS) // @v12.3.8
// DLG_CASHND                     1193 occupied
#define DLG_GTAJFILT              1194
	#define CTL_GTAJFILT_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_GTAJFILT_PERIOD     (2 + WINDOWS_ID_BIAS)
	#define CTL_GTAJFILT_ACTION        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTAJFILT_ACTION     (4 + WINDOWS_ID_BIAS)
	#define CTL_GTAJFILT_USER          (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTAJFILT_USER       (6 + WINDOWS_ID_BIAS)
	#define CTL_GTAJFILT_OBJ           (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GTAJFILT_OBJ        (8 + WINDOWS_ID_BIAS)
	#define CTL_GTAJFILT_FLAGS         (9 + WINDOWS_ID_BIAS)
#define DLG_SCBONUSRULE           1195 //
	// CTL_TRNVRRNG_XXX
#define DLG_BNKTERM				  1196
	#define CTL_BNKTERM_TYPE			(1 + WINDOWS_ID_BIAS)
	#define CTL_BNKTERM_LOGNUM			(2 + WINDOWS_ID_BIAS)
	#define CTLSEL_BNKTERM_TYPE			(3 + WINDOWS_ID_BIAS)
	#define CTL_BNKTERM_PORT			(4 + WINDOWS_ID_BIAS)
	#define CTL_BNKTERM_PATH			(5 + WINDOWS_ID_BIAS)
	#define CTLBRW_BNKTERM_FILENAME		(6 + WINDOWS_ID_BIAS)
	#define CTL_BNKTERM_PINPAD			(7 + WINDOWS_ID_BIAS)
	#define CTL_BNKTERM_FLAGS           (8 + WINDOWS_ID_BIAS) // @v12.0.6
#define DLG_SENDSMS				  1197
	#define CTL_SENDSMS_TEXT			(1 + WINDOWS_ID_BIAS)
	#define CTL_SENDSMS_SYMBCOUNT		(2 + WINDOWS_ID_BIAS)
	#define CTL_SENDSMS_SMSCOUNT		(3 + WINDOWS_ID_BIAS)
	#define CTL_SENDSMS_PREVTEXT		(4 + WINDOWS_ID_BIAS)
	#define CTL_SENDSMS_INSERT			(5 + WINDOWS_ID_BIAS)
	#define CTL_SENDSMS_SYMBCOUNT2		(6 + WINDOWS_ID_BIAS)
	#define CTL_SENDSMS_SMSCOUNT2		(7 + WINDOWS_ID_BIAS)
#define DLG_IMPEXPSCARD           1198
	#define CTL_IMPEXPSCARD_SERSYMB     (1 + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPSCARD_REGCODE     (2 + IMPEXP_CTL_BIAS)
#define DLG_SCSQKLIST             1199
	#define CTL_SCSQKLIST_LIST          (1 + IMPEXP_CTL_BIAS)
	#define CTL_SCSQKLIST_FLAGS         (2 + IMPEXP_CTL_BIAS)
#define DLG_TAGVALUPD             1200
	// CTL_TAGVALVIEW_XXX
#define DLG_GUACFG                1201
	#define CTL_GUACFG_SCARDSER         (1 + IMPEXP_CTL_BIAS)
	#define CTLSEL_GUACFG_SCARDSER      (2 + IMPEXP_CTL_BIAS)
	#define CTL_GUACFG_OWNERKIND        (3 + IMPEXP_CTL_BIAS)
	#define CTLSEL_GUACFG_OWNERKIND     (4 + IMPEXP_CTL_BIAS)
	#define CTL_GUACFG_FLAGS            (5 + IMPEXP_CTL_BIAS)
#define DLG_SCOPFLT               1202
	#define CTL_SCOPFLT_PERIOD          (1 + IMPEXP_CTL_BIAS)
	#define CTLCAL_SCOPFLT_PERIOD       (2 + IMPEXP_CTL_BIAS)
	#define CTL_SCOPFLT_SCSER           (3 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SCOPFLT_SCSER        (4 + IMPEXP_CTL_BIAS)
	#define CTL_SCOPFLT_SC              (5 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SCOPFLT_SC           (6 + IMPEXP_CTL_BIAS)
	#define CTL_SCOPFLT_AMTR            (7 + IMPEXP_CTL_BIAS)
	#define CTL_SCOPFLT_ORD             (8 + IMPEXP_CTL_BIAS)
#define DLG_IMPEXPLOT             1203
	#define CTL_IMPEXPLOT_AR            (1 + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPLOT_AR         (2 + IMPEXP_CTL_BIAS)
// DLG_TAGVTIMESTAMP              1204 occupied
#define DLG_PSALESTEST            1205
	#define CTL_PSALTST_PERIOD          (1 + IMPEXP_CTL_BIAS)
	#define CTLCAL_PSALTST_PERIOD       (2 + IMPEXP_CTL_BIAS)
	#define CTL_PSALTST_GOODSSELKIND    (3 + IMPEXP_CTL_BIAS)
	#define CTL_PSALTST_GGROUP          (4 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PSALTST_GGROUP       (5 + IMPEXP_CTL_BIAS)
	#define CTL_PSALTST_GOODS           (6 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PSALTST_GOODS        (7 + IMPEXP_CTL_BIAS)
#define DLG_SELFBUILD             1206
	#define CTL_SELFBUILD_FLAGS         (1 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_VERSION       (2 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_VERSFX        (3 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_CMPLRPATH     (4 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_IMPATH        (5 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_TIMEOUT       (6 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_ST_INFO       (7 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_CONFIG        (8 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SELFBUILD_CONFIG     (9 + IMPEXP_CTL_BIAS)
	#define CTL_SELFBUILD_SCFG          (10 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SELFBUILD_SCFG       (11 + IMPEXP_CTL_BIAS)
#define DLG_GOODSEXPFILT          1207
	#define CTL_GOODSEXPFILT_CFG        (1 + IMPEXP_CTL_BIAS)
	#define CTLSEL_GOODSEXPFILT_CFG     (2 + IMPEXP_CTL_BIAS)
	#define CTL_GOODSEXPFILT_LOC        (3 + IMPEXP_CTL_BIAS)
	#define CTLSEL_GOODSEXPFILT_LOC     (4 + IMPEXP_CTL_BIAS)
#define DLG_SIGNBILL			  1208
	#define CTL_SIGNBILL_SIGNER			(1 + IMPEXP_CTL_BIAS)
	#define CTL_SIGNBILL_SIGNFILE		(2 + IMPEXP_CTL_BIAS)
	#define cmSignBill					101
#define DLG_JOB_OBJLSTHTM         1209
	#define CTL_JOB_OBJLSTHTM_OUT      (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_JOB_OBJLSTHTM_OUT   (2 + WINDOWS_ID_BIAS)
#define DLG_RUNIE_BILL_IMP        1210
	// CTL_IEBILLSEL_XXX
#define DLG_RUNIE_BILL_EXP        1211
	// CTL_IEBILLSEL_XXX
#define DLG_UHTTSTORE             1212
	#define CTL_UHTTSTORE_ID           (1 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_NAME         (2 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_SYMB         (3 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_PSN          (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_UHTTSTORE_PSN       (5 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_LOC          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_UHTTSTORE_LOC       (7 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_USEPKG       (8 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_SHOWREST     (9 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_FLAGS        (10 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_URST         (11 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_SLIST        (12 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTORE_KIND         (13 + WINDOWS_ID_BIAS)
#define DLG_IEPERSON              1213
	#define CTL_IEPERSON_CFG           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEPERSON_CFG        (2 + WINDOWS_ID_BIAS)
	#define CTL_IEPERSON_CAT           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEPERSON_CAT        (4 + WINDOWS_ID_BIAS)
#define DLG_IMPEXPPERSON          1214
	#define CTL_IMPEXPPSN_KIND         (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPPSN_KIND      (2  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPPSN_SREG         (3  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPPSN_SREG      (4  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPPSN_CITY         (5  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPPSN_CITY      (6  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPPSN_FLAGS        (7  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPPSN_CAT          (8  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPPSN_CAT       (9  + IMPEXP_CTL_BIAS)
#define DLG_VATBL_SIMPLE          1215 // Элемент списка конфигурации книги доходов и расходов
	// CTL_VATBL_XXX
#define DLG_SELINVCORR            1216 // Выбор режима коррекции инвентаризации
	#define CTL_SELINVCORR_FLAGS       (1  + WINDOWS_ID_BIAS)
#define DLG_DISPTYPE              1217
	#define CTL_DISPTYPE_WHAT          (1 + WINDOWS_ID_BIAS)
#define DLG_GRESTUHTTEXP          1218 // Выбор опций экспорта остатков товаров в Universe-HTT
	#define CTL_GRESTUHTTEXP_F         (1  + IMPEXP_CTL_BIAS)
	#define CTL_GRESTUHTTEXP_FLAGS     (2  + IMPEXP_CTL_BIAS)
	#define CTL_GRESTUHTTEXP_CH        (3  + IMPEXP_CTL_BIAS) //@erik v10.7.13
#define DLG_UHTTGRLIST            1219
	#define CTL_UHTTGRLIST_LIST        (1  + WINDOWS_ID_BIAS)
	#define CTL_UHTTGRLIST_ST_GOODS    (2  + WINDOWS_ID_BIAS)
#define DLG_CHKINPLIST            1220
	#define cmChangeStatusForAll       101 // @v11.0.6
	#define CTL_CHKINPLIST_LIST        (1  + WINDOWS_ID_BIAS)
	#define CTL_CHKINPLIST_ST_HEADER   (2  + WINDOWS_ID_BIAS)
	#define CTL_CHKINPLIST_ST_TOTAL    (3  + WINDOWS_ID_BIAS)
	#define CTL_CHKINPLIST_CNT         (4  + WINDOWS_ID_BIAS)
	#define CTLSPN_CHKINPLIST_CNT      (5  + WINDOWS_ID_BIAS)
#define DLG_CHKINP                1221
	#define CTL_CHKINP_ST_INFO         (1  + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_PERSON          (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKINP_PERSON       (3  + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_STATUS          (4  + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_REGDT           (5  + WINDOWS_ID_BIAS)
	#define CTLCAL_CHKINP_REGDT        (6  + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_REGTM           (7  + WINDOWS_ID_BIAS)
	#define CTLTM_CHKINP_REGTM         (8  + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_CIDT            (9  + WINDOWS_ID_BIAS)
	#define CTLCAL_CHKINP_CIDT         (10 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_CITM            (11 + WINDOWS_ID_BIAS)
	#define CTLTM_CHKINP_CITM          (12 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_REGCOUNT        (13 + WINDOWS_ID_BIAS)
	#define CTLSPN_CHKINP_REGCOUNT     (14 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_CICOUNT         (15 + WINDOWS_ID_BIAS)
	#define CTLSPN_CHKINP_CICOUNT      (16 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_ID              (17 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_SCARD           (18 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_PRICE           (19 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_AMOUNT          (20 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_ST_CCINFO       (21 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_ST_PSNMEMO      (22 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_MEMO            (23 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_ANONYM          (24 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_PLACE           (25 + WINDOWS_ID_BIAS)
	#define CTL_CHKINP_ST_PINCODE      (26 + WINDOWS_ID_BIAS)
#define DLG_NETPROXI              1222
	#define CTL_NETPROXI_PROT          (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_NETPROXI_PROT       (2  + WINDOWS_ID_BIAS)
	#define CTL_NETPROXI_KIND          (3  + WINDOWS_ID_BIAS)
	#define CTL_NETPROXI_HOST          (4  + WINDOWS_ID_BIAS)
	#define CTL_NETPROXI_PORT          (5  + WINDOWS_ID_BIAS)
	#define CTL_NETPROXI_USER          (6  + WINDOWS_ID_BIAS)
	#define CTL_NETPROXI_PW            (7  + WINDOWS_ID_BIAS)
	#define CTL_NETPROXI_FLAGS         (8  + WINDOWS_ID_BIAS)
#define DLG_DBDIVFLT              1223
	#define CTL_DBDIVFLT_LOCLIST       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_DBDIVFLT_LOCLIST    (2 + WINDOWS_ID_BIAS)
#define DLG_ADEVICE               1224
	#define CTL_ADEVICE_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_ADEVICE_SYMB           (2 + WINDOWS_ID_BIAS)
	#define CTL_ADEVICE_ID             (3 + WINDOWS_ID_BIAS)
	#define CTL_ADEVICE_FLAGS          (4 + WINDOWS_ID_BIAS)
	#define CTL_ADEVICE_CLS            (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ADEVICE_CLS         (6 + WINDOWS_ID_BIAS)
	#define CTL_ADEVICE_DVCSYMB        (7 + WINDOWS_ID_BIAS)
	#define CTL_ADEVICE_INITSTR        (8 + WINDOWS_ID_BIAS)
#define DLG_SELPRNTSES            1225 // Выбор варианта печати списка технологических сессий
	#define CTL_SELPRNTSES_WHAT        (1 + WINDOWS_ID_BIAS)
#define DLG_OPENEDFILE            1226
	#define CTL_OPENEDFILE_RECENT      (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_OPENEDFILE_RECENT   (2 + WINDOWS_ID_BIAS)
	#define CTL_OPENEDFILE_RESERV      (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_OPENEDFILE_RESERV   (4 + WINDOWS_ID_BIAS)
	#define CTL_OPENEDFILE_SELECT      (5 + WINDOWS_ID_BIAS)
	#define CTLBRW_OPENEDFILE_SELECT   (6 + WINDOWS_ID_BIAS)
	#define CTL_OPENEDFILE_ST_INFO     (7 + WINDOWS_ID_BIAS)
#define DLG_SETQUOTPC             1227
	// CTL_SETQUOT_XXX
#define DLG_FLTCOMPGDS            1228
	#define CTL_FLTCOMPGDS_SWCAT       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTCOMPGDS_SWCAT    (2 + WINDOWS_ID_BIAS)
	#define CTL_FLTCOMPGDS_SWTYPE      (3 + WINDOWS_ID_BIAS)
	#define CTL_FLTCOMPGDS_NAMESTR     (4 + WINDOWS_ID_BIAS)
#define DLG_COMPGDSLST            1229
	#define CTL_COMPGDSLST_LIST        (1 + WINDOWS_ID_BIAS)
#define DLG_GVRBAR                1230
	#define CTL_GVRBAR_ACS             (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GVRBAR_ACS          (2 + WINDOWS_ID_BIAS)
	#define CTL_GVRBAR_AR              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GVRBAR_AR           (4 + WINDOWS_ID_BIAS)
	#define CTL_GVRBAR_OPTIONS         (5 + WINDOWS_ID_BIAS)
#define DLG_OBJTEXTFILT           1231
	#define CTL_OBJTEXTFILT_OBJ        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJTEXTFILT_OBJ     (2 + WINDOWS_ID_BIAS)
	#define CTL_OBJTEXTFILT_FLD        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_OBJTEXTFILT_FLD     (4 + WINDOWS_ID_BIAS)
	#define CTL_OBJTEXTFILT_FLAGS      (5 + WINDOWS_ID_BIAS)
	#define CTL_OBJTEXTFILT_RULEF      (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_OBJTEXTFILT_RULEF   (7 + WINDOWS_ID_BIAS)
#define DLG_SUPRWARE              1232
	#define CTL_SUPRWARE_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_SUPRWARE_CODE          (2 + WINDOWS_ID_BIAS)
	#define CTL_SUPRWARE_ID            (3 + WINDOWS_ID_BIAS)
	#define CTL_SUPRWARE_CAT           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SUPRWARE_CAT        (5 + WINDOWS_ID_BIAS)
	#define CTL_SUPRWARE_TYPE          (6 + WINDOWS_ID_BIAS)
#define DLG_UHTTSTSELITEM         1233
	#define CTL_UHTTSTSELITEM_ATTR     (1 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTSELITEM_EXT      (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_UHTTSTSELITEM_EXT   (3 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTSELITEM_GCCLSF   (4 + WINDOWS_ID_BIAS)
	#define CTL_UHTTSTSELITEM_TITLE    (5 + WINDOWS_ID_BIAS)
#define DLG_WORKBOOK              1234
	#define CTL_WORKBOOK_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_CODE          (2 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_ID            (3 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_TYPE          (4 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_PARENT        (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_WORKBOOK_PARENT     (6 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_FILE          (7 + WINDOWS_ID_BIAS)
	#define CTLBRW_WORKBOOK_FILE       (8 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_EDIT_FILE     (9 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_FLAGS         (10 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_CSS           (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_WORKBOOK_CSS        (12 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_LINK          (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_WORKBOOK_LINK       (14 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_RANK          (15 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_KWC           (16 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_KWD           (17 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_IMAGE         (18 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_KEYWORDS      (19 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_DT            (20 + WINDOWS_ID_BIAS)
	#define CTLCAL_WORKBOOK_DT         (21 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_TM            (22 + WINDOWS_ID_BIAS)
	#define CTLTM_WORKBOOK_TM          (23 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_DESCR         (24 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_KWFLAGS       (25 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_KWSYN         (26 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_KWLOC         (27 + WINDOWS_ID_BIAS)
	#define CTL_WORKBOOK_VER           (28 + WINDOWS_ID_BIAS)
#define DLG_CPPAYM2               1235 // CPPAYM2 Диалог приема оплаты в кассовой панели
	// CTL_CPPAYM_XXX
#define DLG_RTPSNEV               1236
#define DLG_OBJSYNCTAB            1237
	#define CTL_OBJSYNCTAB_LIST        (1 + WINDOWS_ID_BIAS)
	#define CTL_OBJSYNCTAB_ST_INFO     (2 + WINDOWS_ID_BIAS)
#define DLG_PEVNTBYDVCINFO        1238
	#define CTL_PEVNTBYDVCINFO_TEXT    (1 + WINDOWS_ID_BIAS)
	#define CTL_PEVNTBYDVCINFO_DVC     (2 + WINDOWS_ID_BIAS)
#define DLG_FLTUSRPROF            1239
	#define CTL_FLTUSRPROF_DB          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTUSRPROF_DB       (2 + WINDOWS_ID_BIAS)
	#define CTL_FLTUSRPROF_FUNC        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_FLTUSRPROF_FUNC     (4 + WINDOWS_ID_BIAS)
	#define CTL_FLTUSRPROF_PRD         (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_FLTUSRPROF_PRD      (6 + WINDOWS_ID_BIAS)
	#define CTL_FLTUSRPROF_TIMEPRD     (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_FLTUSRPROF_TIMEPRD  (8 + WINDOWS_ID_BIAS)
#define DLG_DBDIVADDRLIST         1240
	#define CTL_DBDIVADDRLIST_LIST     (1 + WINDOWS_ID_BIAS)
#define DLG_SELGUESTCOUNT         1241
	#define cmSelGuestCount01          101
	#define cmSelGuestCount02          102
	#define cmSelGuestCount03          103
	#define cmSelGuestCount04          104
	#define cmSelGuestCount05          105
	#define cmSelGuestCount06          106
	#define cmSelGuestCount07          107
	#define cmSelGuestCount08          108
	#define cmSelGuestCount09          109
	#define cmSelGuestCount10          110
	#define cmSelGuestCount11          111
	#define cmSelGuestCount12          112
	#define cmSelGuestCount13          113
	#define cmSelGuestCount14          114
	#define cmSelGuestCount15          115
	#define CTL_SELGUESTCOUNT_01       (1 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_02       (2 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_03       (3 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_04       (4 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_05       (5 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_06       (6 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_07       (7 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_08       (8 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_09       (9 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_10       (10 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_11       (11 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_12       (12 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_13       (13 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_14       (14 + WINDOWS_ID_BIAS)
	#define CTL_SELGUESTCOUNT_15       (15 + WINDOWS_ID_BIAS)
#define DLG_SMSSYS                1242
	#define CTL_SMSSYS_RESPTIMEOUT     (1 + WINDOWS_ID_BIAS)
	#define CTL_SMSSYS_RSNDTIMEOUT     (2 + WINDOWS_ID_BIAS)
	#define CTL_SMSSYS_RSNDNUM         (3 + WINDOWS_ID_BIAS)
	#define CTL_SMSSYS_RECONNTIMEOUT   (4 + WINDOWS_ID_BIAS)
	#define CTL_SMSSYS_RECONNNUM       (5 + WINDOWS_ID_BIAS)
#define DLG_TSASMS                1243
	#define CTL_TSASMS_TDDOPATH        (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_TSASMS_FILENAME     (2 + WINDOWS_ID_BIAS)
	#define CTL_TSASMS_DAYS            (3 + WINDOWS_ID_BIAS)
	#define CTL_TSASMS_STTM            (4 + WINDOWS_ID_BIAS)
	#define CTL_TSASMS_FNTM            (5 + WINDOWS_ID_BIAS)
#define DLG_JTSASMSFILT           1244
	#define cmSelTechByGoods           101
	#define CTL_JTSASMSFILT_PRC        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_JTSASMSFILT_PRC     (2 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_TECH       (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_JTSASMSFILT_TECH    (4 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_ST_GOODS   (5 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_TECBYG     (6 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_AR         (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_JTSASMSFILT_AR      (8 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_STPRD      (9 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_STTIME     (10 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_FNPRD      (11 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_FNTIME     (12 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_STATUS     (13 + WINDOWS_ID_BIAS)
	#define CTL_JTSASMSFILT_FLAGS      (14 + WINDOWS_ID_BIAS)
	#define CTLCAL_JTSASMSFILT_STPERIOD (15 + WINDOWS_ID_BIAS)
	#define CTLCAL_JTSASMSFILT_FNPERIOD (16 + WINDOWS_ID_BIAS)
#define DLG_IMPWBOOK              1245
	#define CTL_IMPWBOOK_WILDCARD      (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_IMPWBOOK_WILDCARD   (2 + WINDOWS_ID_BIAS)
	#define CTL_IMPWBOOK_RMVPREFIX     (3 + WINDOWS_ID_BIAS)
	#define CTL_IMPWBOOK_RMVSUFFIX     (4 + WINDOWS_ID_BIAS)
	#define CTL_IMPWBOOK_FLAGS         (5 + WINDOWS_ID_BIAS)
	#define CTL_IMPWBOOK_ROOT          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_IMPWBOOK_ROOT       (7 + WINDOWS_ID_BIAS)
#define DLG_IEWORKBOOK            1246
	#define CTL_IEWORKBOOK_CFG         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEWORKBOOK_CFG      (2 + WINDOWS_ID_BIAS)
#define DLG_KEYWORD               1247
	// CTL_WORKBOOK_XXX
#define DLG_WBPRELUDE             1248
	#define CTL_WBPRELUDE_TYPE         (1 + WINDOWS_ID_BIAS)
	#define CTL_WBPRELUDE_FILE         (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_WBPRELUDE_FILE      (3 + WINDOWS_ID_BIAS)
#define DLG_JOBFILT               1249
	#define CTL_JOBFILT_CMD            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_JOBFILT_CMD         (2 + WINDOWS_ID_BIAS)
	#define CTL_JOBFILT_FLAGS          (3 + WINDOWS_ID_BIAS)
#define DLG_SCISEARCH             1250
	#define CTL_SCISEARCH_PATTERN      (1 + WINDOWS_ID_BIAS)
	#define CTL_SCISEARCH_REPLACE      (2 + WINDOWS_ID_BIAS)
	#define CTL_SCISEARCH_RF           (3 + WINDOWS_ID_BIAS)
	#define CTL_SCISEARCH_FLAGS        (4 + WINDOWS_ID_BIAS)
#define DLG_SELWBLINK             1251
	#define CTL_SELWBLINK_TYPE         (1 + WINDOWS_ID_BIAS)
	#define CTL_SELWBLINK_ITEM         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELWBLINK_ITEM      (3 + WINDOWS_ID_BIAS)
	#define CTL_SELWBLINK_IMGITEM      (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELWBLINK_IMGITEM   (5 + WINDOWS_ID_BIAS)
	#define CTL_SELWBLINK_INFO         (6 + WINDOWS_ID_BIAS)
#define DLG_PRCEXT                1252 // Расширение параметров процессора
	#define CTL_PRCEXT_CHKINTIME       (1 + WINDOWS_ID_BIAS)
	#define CTLTM_PRCEXT_CHKINTIME     (2 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_CHKOUTTIME      (3 + WINDOWS_ID_BIAS)
	#define CTLTM_PRCEXT_CHKOUTTIME    (4 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_CHKINROUND      (5 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_CHKOUTROUND     (6 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_PLUSONE         (7 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_INITSTATUS      (8 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_PLACELIST       (9 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_GUA             (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCEXT_GUA          (11 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_CIPCTO          (12 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_CIPCTONEG       (13 + WINDOWS_ID_BIAS)
	#define CTL_PRCEXT_CIPLTO          (14 + WINDOWS_ID_BIAS)
#define DLG_WBATTACHF             1253
	#define CTL_WBATTACHF_FT           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_WBATTACHF_FT        (2 + WINDOWS_ID_BIAS)
	#define CTL_WBATTACHF_FILE         (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_WBATTACHF_FILE      (4 + WINDOWS_ID_BIAS)
#define DLG_WBCFG                 1254
	#define CTL_WBCFG_CODETEMPL        (1 + WINDOWS_ID_BIAS)
	#define CTL_WBCFG_CODECNTR         (2 + WINDOWS_ID_BIAS)
	#define CTL_WBCFG_DEFIMGFOLD       (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_WBCFG_DEFIMGFOLD    (4 + WINDOWS_ID_BIAS)
	#define CTL_WBCFG_UHTTXEVTOK       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_WBCFG_UHTTXEVTOK    (6 + WINDOWS_ID_BIAS)
#define DLG_SELHTMLTAG            1255
	#define CTL_SELHTMLTAG_TAGTEXT     (1 + WINDOWS_ID_BIAS)
#define DLG_SHIPMCTRL             1256
	#define CTL_SHIPMCTRL_SHIPMOP      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SHIPMCTRL_SHIPMOP   (2 + WINDOWS_ID_BIAS)
	#define CTL_SHIPMCTRL_RETOP        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SHIPMCTRL_RETOP     (4 + WINDOWS_ID_BIAS)
	#define CTL_SHIPMCTRL_DURATION     (5 + WINDOWS_ID_BIAS)
	#define CTL_SHIPMCTRL_UPDEV        (6 + WINDOWS_ID_BIAS)
	#define CTL_SHIPMCTRL_DNDEV        (7 + WINDOWS_ID_BIAS)
	#define CTL_SHIPMCTRL_SHLIMOP      (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_SHIPMCTRL_SHLIMOP   (9 + WINDOWS_ID_BIAS)
#define DLG_DYNOBJ                1257 // Диалог редактирования динамического объекта
#define DLG_DYNOBJHDR             1258 // Диалог редактирования определителя динамических объектов
#define DLG_NEWCLNT               1259 // Диалог редактирования конфигурации анализа клиентской активности
	#define CTL_NEWCLNT_LIST           (1 + WINDOWS_ID_BIAS)
	#define CTL_NEWCLNT_CRITCADD       (2 + WINDOWS_ID_BIAS) // @v12.2.2 CriticalCliActivityDelayDays
	#define CTL_NEWCLNT_CRITCADS       (3 + WINDOWS_ID_BIAS) // @v12.2.2 CriticalCliActivityDelaySigm
	#define CTL_NEWCLNT_HLCADD         (4 + WINDOWS_ID_BIAS) // @v12.2.2 HopelessCliActivityDelayDays
	#define CTL_NEWCLNT_HLCADS         (5 + WINDOWS_ID_BIAS) // @v12.2.2 HopelessCliActivityDelaySigm
	#define CTL_NEWCLNT_PSNKIND        (6 + WINDOWS_ID_BIAS) // @v12.2.9
	#define CTLSEL_NEWCLNT_PSNKIND     (7 + WINDOWS_ID_BIAS) // @v12.2.9
#define DLG_NEWCNTCF              1260
	#define CTL_NEWCNTCF_OPTYPE        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_NEWCNTCF_OPTYPE     (2 + WINDOWS_ID_BIAS)
	#define CTL_NEWCNTCF_OPKIND        (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_NEWCNTCF_OPKIND     (4 + WINDOWS_ID_BIAS)
#define DLG_TAGENUMTREEVIEW       1261
#define DLG_IMPEXPCSC             1262
	#define CTL_IMPEXPCSC_CSESSTAG     (1  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPCSC_CCHECKTAG    (2  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPCSC_CCLINETAG    (3  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPCSC_CCPAYMTAG    (4  + IMPEXP_CTL_BIAS)
#define DLG_BILLFLTVOPT           1263
	#define CTL_BILLFLTVOPT_FLAGS      (1  + IMPEXP_CTL_BIAS)
#define DLG_UHTTEXPGOODS          1264
	#define cmCountUhttRefs            101
	#define CTL_UHTTEXPGOODS_FLAGS     (1  + IMPEXP_CTL_BIAS)
	#define CTL_UHTTEXPGOODS_INFO      (2  + IMPEXP_CTL_BIAS)
	#define CTL_UHTTEXPGOODS_CO        (3  + IMPEXP_CTL_BIAS)
	#define CTL_UHTTEXPGOODS_COT       (4  + IMPEXP_CTL_BIAS)
	#define CTLSEL_UHTTEXPGOODS_COT    (5  + IMPEXP_CTL_BIAS)
	#define CTL_UHTTEXPGOODS_CH        (6  + IMPEXP_CTL_BIAS) //@erik
	#define CTL_UHTTEXPGOODS_GS        (7  + IMPEXP_CTL_BIAS)
	#define CTL_UHTTEXPGOODS_RPO       (8  + IMPEXP_CTL_BIAS) // Опции экспорта остатков и цен
	#define CTL_UHTTEXPGOODS_GUA       (9  + IMPEXP_CTL_BIAS)
	#define CTLSEL_UHTTEXPGOODS_GUA    (10 + IMPEXP_CTL_BIAS)
	#define CTL_UHTTEXPGOODS_DEXT      (11 + IMPEXP_CTL_BIAS) // Дополнительное поле товара для экспорта описания //
#define DLG_GUART                 1265 // Права доступа глобальной учетной записи
	#define CTL_GUART_FLAGS            (1 + WINDOWS_ID_BIAS)
#define DLG_TEX2HTM               1266
	#define CTL_TEX2HTM_INFILE         (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_TEX2HTM_INFILE      (2 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_INPICPATH      (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_TEX2HTM_INPICPATH   (4 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_OUTPATH        (5 + WINDOWS_ID_BIAS)
	#define CTLBRW_TEX2HTM_OUTPATH     (6 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_PICPATH        (7 + WINDOWS_ID_BIAS)
	#define CTLBRW_TEX2HTM_PICPATH     (8 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_FLAGS          (9 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_PARWB          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_TEX2HTM_PARWB       (11 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_PARPICWB       (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_TEX2HTM_PARPICWB    (13 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_WBREFTMPL      (14 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_WBPICREFTMPL   (15 + WINDOWS_ID_BIAS)
	#define CTL_TEX2HTM_ANCHORPFX      (16 + WINDOWS_ID_BIAS)
#define DLG_BIZPRCRT              1267
	#define CTL_BIZPRCRT_PRIMITIV      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZPRCRT_PRIMITIV   (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_MODIF         (3 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_PERIOD        (4 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_LOC           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZPRCRT_LOC        (6 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_GOODSGRP      (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZPRCRT_GOODSGRP   (8 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_OPSYMB        (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZPRCRT_OPSYMB     (10 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_PRDCMP        (11 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_RESULT        (12 + WINDOWS_ID_BIAS)
	#define CTL_BIZPRCRT_PRMTVNAME     (13 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZPRCRT_PERIOD     (14 + WINDOWS_ID_BIAS)
#define DLG_BIGTXTEDIT            1268
	#define CTL_BIGTXTEDIT_TEXT        (1 + WINDOWS_ID_BIAS)
	#define CTL_BIGTXTEDIT_TITLE       (2 + WINDOWS_ID_BIAS)
#define DLG_VER2HTM               1269
	#define CTL_VER2HTM_INFILE         (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_VER2HTM_INFILE      (2 + WINDOWS_ID_BIAS)
	#define CTL_VER2HTM_PICPATH        (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_VER2HTM_PICPATH     (4 + WINDOWS_ID_BIAS)
	#define CTL_VER2HTM_OUTPATH        (5 + WINDOWS_ID_BIAS)
	#define CTLBRW_VER2HTM_OUTPATH     (6 + WINDOWS_ID_BIAS)
	#define CTL_VER2HTM_FLAGS          (7 + WINDOWS_ID_BIAS)
	#define CTL_VER2HTM_PARWB          (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_VER2HTM_PARWB       (9 + WINDOWS_ID_BIAS)
	#define CTL_VER2HTM_ANCHORPFX      (10 + WINDOWS_ID_BIAS)
#define DLG_ALCREPCFG             1270
	#define cmCCheckFilt               101
	#define CTL_ALCREPCFG_ALCGRP       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_ALCGRP    (2 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_BEERGRP      (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_BEERGRP   (4 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_CATTAG       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_CATTAG    (6 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_CATDIM       (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_CATDIM    (8 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_VOLDIM       (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_VOLDIM    (10 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_LICREG       (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_LICREG    (12 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_KPPREG       (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_KPPREG    (14 + WINDOWS_ID_BIAS)

	#define CTL_ALCREPCFG_OP_RCP       (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_RCP    (16 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_RCE       (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_RCE    (18 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_RCR       (19 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_RCR    (20 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_EXP       (21 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_EXP    (22 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_EXE       (23 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_EXE    (24 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_EXR       (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_EXR    (26 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_INT       (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_INT    (28 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_MNFTAG       (29 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_MNFTAG    (30 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_PRFDIM       (31 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_PRFDIM    (32 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_ALCCLS       (33 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_ALCCLS    (34 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_MITAG        (35 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_MITAG     (36 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_IMPPSNK      (37 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_IMPPSNK   (38 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_FLAGS        (39 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_WOSW         (40 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_OP_INV       (41 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_INV    (42 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_RSAT         (43 + WINDOWS_ID_BIAS) // RtlSaleAllwTime
	#define CTL_ALCREPCFG_OP_MFG       (44 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCREPCFG_OP_MFG    (45 + WINDOWS_ID_BIAS)
	#define CTL_ALCREPCFG_SPLAGT       (46 + WINDOWS_ID_BIAS) // @v11.0.8
	#define CTLSEL_ALCREPCFG_SPLAGT    (47 + WINDOWS_ID_BIAS) // @v11.0.8
	#define CTL_ALCREPCFG_WOSW_NMR     (48 + WINDOWS_ID_BIAS) // @v11.7.12

#define DLG_INPUTNUM              1271
	#define CTL_INPUTNUM_NUM           (1 + WINDOWS_ID_BIAS)
#define DLG_SORDPE                1272
	#define CTL_SORDPE_GOODSGRP        (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_SORDPE_GOODSGRP     (2 + WINDOWS_ID_BIAS)
	#define CTL_SORDPE_MNGR            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SORDPE_MNGR         (4 + WINDOWS_ID_BIAS)
	#define CTL_SORDPE_LOC             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_SORDPE_LOC          (6 + WINDOWS_ID_BIAS)
	//#define CTL_SORDPE_DLVRTERM        (7 + WINDOWS_ID_BIAS)
	#define CTL_SORDPE_ORDPRD          (8 + WINDOWS_ID_BIAS)
	#define CTL_SORDPE_SUPPL           (9 + WINDOWS_ID_BIAS)
	#define CTL_SORDPE_DUEPRD          (10 + WINDOWS_ID_BIAS)
// @v11.1.8 (unused) #define DLG_SORDPL                1273
	// @v11.1.8 (unused) #define CTL_SORDPL_LIST            (1 + WINDOWS_ID_BIAS)
#define DLG_SRCHBILL              1274
	#define CTL_SRCHBILL_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_SRCHBILL_PERIOD     (2 + WINDOWS_ID_BIAS)
	#define CTL_SRCHBILL_CODE          (3 + WINDOWS_ID_BIAS)
	#define CTL_SRCHBILL_MODE          (4 + WINDOWS_ID_BIAS)
#define DLG_EDIPRV                1275
	#define CTL_EDIPRV_NAME             (1 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_SYMB             (2 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_ID               (3 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_FLAGS            (4 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_SUPPOPS          (5 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_MODULE           (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_EDIPRV_MODULE        (7 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_ADDR1            (8 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_PORT1            (9 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_ADDR2            (10 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_PORT2            (11 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_GUA              (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_EDIPRV_GUA           (13 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_FORMAT           (14 + WINDOWS_ID_BIAS)
	#define CTL_EDIPRV_SUBIN            (15 + WINDOWS_ID_BIAS) // @v11.9.4
	#define CTL_EDIPRV_SUBOUT           (16 + WINDOWS_ID_BIAS) // @v11.9.4
	#define CTL_EDIPRV_DTO              (17 + WINDOWS_ID_BIAS) // @v11.9.12 Персоналия-оператор передачи данных
	#define CTLSEL_EDIPRV_DTO           (18 + WINDOWS_ID_BIAS) // @v11.9.12
#define DLG_BILLCFGEXT            1276 // Диалог дополнительных параметров конфигурации документов
	// CTL_BILLCFG_XXX
#define DLG_IMPEXPQUOT            1277
	#define CTL_IMPEXPQUOT_QK          (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPQUOT_QK       (2  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPQUOT_CURR        (3  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPQUOT_CURR     (4  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPQUOT_AR          (5  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPQUOT_AR       (6  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPQUOT_LOC         (7  + IMPEXP_CTL_BIAS)
	#define CTLSEL_IMPEXPQUOT_LOC      (8  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPQUOT_FLAGS       (9  + IMPEXP_CTL_BIAS)
	#define CTL_IMPEXPQUOT_QKCLS       (10 + IMPEXP_CTL_BIAS)
#define DLG_AEQSTAT               1278
	#define CTL_AEQSTAT_LIVING         (1  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_GETCOUNT       (2  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_GETDECLCOUNT   (3  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_PUTCOUNT       (4  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_MAXQUEUE       (5  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_STATEINFO      (6  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_SJPRCCOUNT     (7  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_SJMSGCOUNT     (8  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_PHNPRCCOUNT    (9  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_PHNMSGCOUNT    (10  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_MQBPRCCOUNT    (11  + IMPEXP_CTL_BIAS)
	#define CTL_AEQSTAT_MQBMSGCOUNT    (12  + IMPEXP_CTL_BIAS)

#define DLG_USRER                 1279
#define DLG_GDSOPRTOTALCMP        1280 // T_GDSOPR Итоги товарного отчета по операции со сравнением
#define DLG_SCARDOPFRZ            1281 // Операция приостановки действия персональной карте
#define DLG_DBTEXP                // no number
	#define CTL_DBTEXP_PERIOD          (1  + IMPEXP_CTL_BIAS)
	#define CTLCAL_DBTEXP_PERIOD       (2  + IMPEXP_CTL_BIAS)
	#define CTL_DBTEXP_PATH            (3  + IMPEXP_CTL_BIAS)
	#define CTLBRW_DBTEXP_PATH         (4  + IMPEXP_CTL_BIAS)
	#define CTL_DBTEXP_FLAGS           (5  + IMPEXP_CTL_BIAS)
	#define CTL_DBTEXP_REFDBDIV        (6  + IMPEXP_CTL_BIAS)
	#define CTLSEL_DBTEXP_REFDBDIV     (7  + IMPEXP_CTL_BIAS)
#define DLG_DBTEXPTRFR            1282
#define DLG_GEOTRACKMODE          1283 // Режим отслеживания гео-позиции
	#define CTL_GEOTRACKMODE_FLAGS     (1  + IMPEXP_CTL_BIAS)
	#define CTL_GEOTRACKMODE_CYCLE     (2  + IMPEXP_CTL_BIAS)
#define DLG_DBTEXPBILL            1284
#define DLG_FIASIMP               1285
	#define CTL_FIASIMP_PATH           (1  + IMPEXP_CTL_BIAS)
	#define CTLBRW_FIASIMP_PATH        (2  + IMPEXP_CTL_BIAS)
	#define CTL_FIASIMP_FLAGS          (3  + IMPEXP_CTL_BIAS)
#define DLG_TESTFIAS              1286
	#define CTL_TESTFIAS_INPUT         (1  + IMPEXP_CTL_BIAS)
	#define CTL_TESTFIAS_INFO          (2  + IMPEXP_CTL_BIAS)
#define DLG_GEOTRFILT             1287
	#define CTL_GEOTRFILT_PERIOD       (1  + IMPEXP_CTL_BIAS)
	#define CTLCAL_GEOTRFILT_PERIOD    (2  + IMPEXP_CTL_BIAS)
	#define CTL_GEOTRFILT_OBJT         (3  + IMPEXP_CTL_BIAS)
	#define CTLSEL_GEOTRFILT_OBJT      (4  + IMPEXP_CTL_BIAS)
	#define CTL_GEOTRFILT_OBJ          (5  + IMPEXP_CTL_BIAS)
	#define CTLSEL_GEOTRFILT_OBJ       (6  + IMPEXP_CTL_BIAS)
	#define CTL_GEOTRFILT_EXTOBJT      (7  + IMPEXP_CTL_BIAS)
	#define CTLSEL_GEOTRFILT_EXTOBJT   (8  + IMPEXP_CTL_BIAS)
	#define CTL_GEOTRFILT_EXTOBJ       (9  + IMPEXP_CTL_BIAS)
	#define CTLSEL_GEOTRFILT_EXTOBJ    (10 + IMPEXP_CTL_BIAS)
#define DLG_GEOTRTOTAL            1288
	#define CTL_GEOTRTOTAL_COUNT       (1  + IMPEXP_CTL_BIAS)
	#define CTL_GEOTRTOTAL_OBJCOUNT    (2  + IMPEXP_CTL_BIAS)
#define DLG_PRCPLACE              1289
	#define CTL_PRCPLACE_PLACES        (1  + IMPEXP_CTL_BIAS)
	#define CTL_PRCPLACE_GOODS         (2  + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCPLACE_GOODS      (3  + IMPEXP_CTL_BIAS)
	#define CTL_PRCPLACE_DESCR         (4  + IMPEXP_CTL_BIAS)
	#define CTL_PRCPLACE_ST_INFO       (5  + IMPEXP_CTL_BIAS)
	#define CTL_PRCPLACE_GGRP          (6  + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCPLACE_GGRP       (7  + IMPEXP_CTL_BIAS)
#define DLG_TSESSEXT              1290
	#define cmCopyPrcPlaceList         101
    #define CTL_TSESSEXT_DESCR         (1  + IMPEXP_CTL_BIAS)
	#define CTL_TSESSEXT_IMAGE         (2  + IMPEXP_CTL_BIAS)
	#define CTL_TSESSEXT_DETAIL        (3  + IMPEXP_CTL_BIAS)
	#define CTL_TSESSEXT_PLACELIST     (4 + WINDOWS_ID_BIAS)
#define DLG_TSESMNT               1291
	#define CTL_TSESMNT_PERIOD         (1  + IMPEXP_CTL_BIAS)
	#define CTLCAL_TSESMNT_PERIOD      (2  + IMPEXP_CTL_BIAS)
	#define CTL_TSESMNT_ACTION         (3  + IMPEXP_CTL_BIAS)
#define DLG_REPLGOODSALC          1292
	// CTL_REPLGOODS_XXX
#define DLG_EGAISQ                1293
	#define cmLotFilt                  101
	#define CTL_EGAISQ_WHAT            (1  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISQ_QADD            (2  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISQ_INFO            (3  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISQ_LOC             (4  + IMPEXP_CTL_BIAS)
	#define CTLSEL_EGAISQ_LOC          (5  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISQ_FLAGS           (6  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISQ_ACTLZVAR        (7  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISQ_MAINORG         (8  + IMPEXP_CTL_BIAS)
	#define CTLSEL_EGAISQ_MAINORG      (10  + IMPEXP_CTL_BIAS)
#define DLG_LOTEXTCODE            1294
	#define CTL_LOTEXTCODE_INFO        (1  + IMPEXP_CTL_BIAS)
	#define CTL_LOTEXTCODE_CODE        (2  + IMPEXP_CTL_BIAS)
#define DLG_TESTEGAIS             1295
	#define CTL_TESTEGAIS_FLAGS        (1  + IMPEXP_CTL_BIAS)
	#define CTL_TESTEGAIS_LOC          (2  + IMPEXP_CTL_BIAS)
	#define CTLSEL_TESTEGAIS_LOC       (3  + IMPEXP_CTL_BIAS)
#define DLG_EGAISREGA             1296
	#define CTL_EGAISREGA_QTTY         (1  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISREGA_MANUFDATE    (2  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISREGA_TTNDATE      (3  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISREGA_TTNCODE      (4  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISREGA_EGAISN       (5  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISREGA_EGAISD       (6  + IMPEXP_CTL_BIAS)
#define DLG_LOTEXTCTOTAL          1297
	#define CTL_LOTEXTCTOTAL_COUNT     (1  + IMPEXP_CTL_BIAS)
#define DLG_GDSCLSLD              1298
	#define CTL_GDSCLSLD_UNIT          (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_GDSCLSLD_UNIT       (2  + IMPEXP_CTL_BIAS)
	#define CTL_GDSCLSLD_NUMDIM        (3  + IMPEXP_CTL_BIAS)
	#define CTL_GDSCLSLD_QTTYFORM      (4  + IMPEXP_CTL_BIAS)
#define DLG_RTPOSLIST             1299
	#define CTL_RTPOSLIST_LIST         (1  + IMPEXP_CTL_BIAS)
#define DLG_RTPOSLI               1300
	#define CTL_RTPOSLI_ITEM           (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_RTPOSLI_ITEM        (2  + IMPEXP_CTL_BIAS)
#define DLG_RTQKLIST              1301
	#define CTL_RTQKLIST_LIST         (1  + IMPEXP_CTL_BIAS)
#define DLG_RTQKLI                1302
	#define CTL_RTQKLI_ITEM           (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_RTQKLI_ITEM        (2  + IMPEXP_CTL_BIAS)
#define DLG_TAGLOTDIM             1303
	#define CTL_TAGLOTDIM_X           (1  + IMPEXP_CTL_BIAS)
	#define CTL_TAGLOTDIM_Y           (2  + IMPEXP_CTL_BIAS)
	#define CTL_TAGLOTDIM_Z           (3  + IMPEXP_CTL_BIAS)
#define DLG_EDITCCAMT             1304
	#define CTL_EDITCCAMT_AMT         (1  + IMPEXP_CTL_BIAS)
	#define CTLCAL_EDITCCAMT_AMT      (2  + IMPEXP_CTL_BIAS)
	#define CTL_EDITCCAMT_INFO        (3  + IMPEXP_CTL_BIAS)
#define DLG_EGAISMARK             1305
	#define CTL_EGAISMARK_INPUT       (1  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISMARK_INFO        (2  + IMPEXP_CTL_BIAS)
	#define CTL_EGAISMARK_AGI         (3  + IMPEXP_CTL_BIAS)
#define DLG_PUGLSPLL              1306 // Списки поставщиков, ассоциированных с дефицитом списания драфт-документов
	#define CTL_PUGLSPLL_LIST         (1  + IMPEXP_CTL_BIAS)
	#define CTL_PUGLSPLL_ST_INFO      (2  + IMPEXP_CTL_BIAS)
#define DLG_PUGLSPLI              1307 // Элемент списка поставщиков, ассоциированных с дефицитом списания драфт-документов
	#define CTL_PUGLSPLI_SUPPL        (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_PUGLSPLI_SUPPL     (2  + IMPEXP_CTL_BIAS)
	#define CTL_PUGLSPLI_QTTY         (3  + IMPEXP_CTL_BIAS)
	#define CTL_PUGLSPLI_UNIT         (4  + IMPEXP_CTL_BIAS) // Переключатель терминов, в которых указывается QTTY (проценты/абсолют)
	#define CTL_PUGLSPLI_ST_INFO      (5  + IMPEXP_CTL_BIAS)
#define DLG_SUPPLIX               1308 // Параметры обмена данными с поставщиком
	#define CTL_SUPPLIX_SUPPL         (1  + IMPEXP_CTL_BIAS)
	#define CTLSEL_SUPPLIX_SUPPL      (2  + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_EXPPRD        (3  + IMPEXP_CTL_BIAS)
	#define CTLCAL_SUPPLIX_EXPPRD     (4  + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_IMPPRD        (5  + IMPEXP_CTL_BIAS)
	#define CTLCAL_SUPPLIX_IMPPRD     (6  + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_LOC           (7  + IMPEXP_CTL_BIAS)
	#define CTLSEL_SUPPLIX_LOC        (8  + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_ACTIONS       (9  + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_FLAGS         (10 + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_SPCDIS1       (11 + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_SPCDIS2       (12 + IMPEXP_CTL_BIAS)
	#define CTL_SUPPLIX_ADDPARAM      (13 + IMPEXP_CTL_BIAS)
#define DLG_MAILTO                1309
	#define CTL_MAILTO_ACCNT          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MAILTO_ACCNT       (2 + WINDOWS_ID_BIAS)
	#define CTL_MAILTO_ADDR           (3 + WINDOWS_ID_BIAS)
	#define CTL_MAILTO_SUBJ           (4 + WINDOWS_ID_BIAS)
#define DLG_SELEGAISCHRGON        1310 // Выбор варианта постановки лотов на баланс в ЕГАИС
	#define CTL_SELEGAISCHRGON_WHAT   (1 + WINDOWS_ID_BIAS)
#define DLG_UPDFREIGHT            1311
	#define CTL_UPDFREIGHT_TR         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDFREIGHT_TR      (2 + WINDOWS_ID_BIAS)
	#define CTL_UPDFREIGHT_ISSDT      (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_UPDFREIGHT_ISSDT   (4 + WINDOWS_ID_BIAS)
	#define CTL_UPDFREIGHT_ARRDT      (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_UPDFREIGHT_ARRDT   (6 + WINDOWS_ID_BIAS)
	#define CTL_UPDFREIGHT_SHPF       (7 + WINDOWS_ID_BIAS)
	#define CTL_UPDFREIGHT_CAPT       (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDFREIGHT_CAPT    (9 + WINDOWS_ID_BIAS)
	#define CTL_UPDFREIGHT_FLAGS      (10 + WINDOWS_ID_BIAS) // @v11.2.10
#define DLG_CVTCIPHER             1312
	#define CTL_CVTCIPHER_DB          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CVTCIPHER_DB       (2 + WINDOWS_ID_BIAS)
	#define CTL_CVTCIPHER_PASSWORD    (3 + WINDOWS_ID_BIAS)
	#define CTL_CVTCIPHER_INISRC      (4 + WINDOWS_ID_BIAS)
	#define CTLBRW_CVTCIPHER_INISRC   (5 + WINDOWS_ID_BIAS)
	#define CTL_CVTCIPHER_INIDEST     (6 + WINDOWS_ID_BIAS)
	#define CTLBRW_CVTCIPHER_INIDEST  (7 + WINDOWS_ID_BIAS)
#define DLG_CCBYBILL              1313
	#define CTL_CCBYBILL_POSNODE      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CCBYBILL_POSNODE   (2 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_PAYMTYPE     (3 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_DIVISION     (4 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_ST_INFO      (5 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_CASH         (6 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_DIFF         (7 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_FLAGS        (8 + WINDOWS_ID_BIAS)
	#define CTL_CCBYBILL_EADDR        (9 + WINDOWS_ID_BIAS) // @v11.3.7
	#define CTL_CCBYBILL_EADDRINF     (10 + WINDOWS_ID_BIAS) // @v11.3.7
	#define CTL_CCBYBILL_PAPERLESS    (11 + WINDOWS_ID_BIAS) // @v11.3.7
	#define CTLFRAME_CCBYBILL_PAPERLESS (12 + WINDOWS_ID_BIAS) // @v11.3.7

#define DLG_PRCROSM               1314
	#define CTL_PRCROSM_PATH          (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_PRCROSM_PATH       (2 + WINDOWS_ID_BIAS)
	#define CTL_PRCROSM_FLAGS         (3 + WINDOWS_ID_BIAS)
#define DLG_IEGOODS               1315
	#define CTL_IEGOODS_CFG           (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEGOODS_CFG        (2 + WINDOWS_ID_BIAS)
	#define CTL_IEGOODS_LOC           (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_IEGOODS_LOC        (4 + WINDOWS_ID_BIAS)
#define DLG_VERIFMPHN             1316
	#define CTL_VERIFMPHN_PHONE       (1 + WINDOWS_ID_BIAS)
	#define CTL_VERIFMPHN_CCODE       (2 + WINDOWS_ID_BIAS)
	#define CTL_VERIFMPHN_ST_INFO     (3 + WINDOWS_ID_BIAS)
#define DLG_SCNTFOPT              1317
	#define CTL_SCNTFOPT_FLAGS        (1 + WINDOWS_ID_BIAS)
#define DLG_APNCORLIST            1318
	#define CTL_APNCORLIST_LIST       (1 + WINDOWS_ID_BIAS)
	#define CTL_APNCORLIST_INFO       (2 + WINDOWS_ID_BIAS)
#define DLG_APNCORITEM            1319
	#define CTL_APNCORITEM_N          (1 + WINDOWS_ID_BIAS)
	#define CTL_APNCORITEM_NAME       (2 + WINDOWS_ID_BIAS)
	#define CTL_APNCORITEM_UUID       (3 + WINDOWS_ID_BIAS)
#define DLG_POSNQUERY             1320
	#define CTL_POSNQUERY_N           (1 + WINDOWS_ID_BIAS)
	#define CTL_POSNQUERY_Q           (2 + WINDOWS_ID_BIAS)
	#define CTL_POSNQUERY_PERIOD      (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_POSNQUERY_PERIOD   (4 + WINDOWS_ID_BIAS)
	#define CTL_POSNQUERY_SESSL       (5 + WINDOWS_ID_BIAS)
	#define CTL_POSNQUERY_INFO        (6 + WINDOWS_ID_BIAS)
#define DLG_IMAGEINFO2            1321
#define DLG_WOBARCODE             1322 // WhatmanObjectBarcode
	#define CTL_WOBARCODE_CODE        (1 + WINDOWS_ID_BIAS)
	#define CTL_WOBARCODE_STD         (2 + WINDOWS_ID_BIAS)
	#define CTL_WOBARCODE_ANGLE       (3 + WINDOWS_ID_BIAS)
#define DLG_INVITEMQ              1323 // Быстрый ввод строки инвентаризации по коду с количесвом
	// CTL_INVITEM_XXX
#define DLG_PRCRSARTR             1324
	#define CTL_PRCRSARTR_SRCPATH     (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_PRCRSARTR_SRCPATH  (2 + WINDOWS_ID_BIAS)
	#define CTL_PRCRSARTR_FLAGS       (3 + WINDOWS_ID_BIAS)
	#define CTL_PRCRSARTR_UEDFLAGS    (4 + WINDOWS_ID_BIAS)
#define DLG_PRCBAUTOC             1325 // PPBillAutoCreateParam
	#define CTL_PRCBAUTOC_WHAT        (1 + WINDOWS_ID_BIAS)
	#define CTL_PRCBAUTOC_RULE        (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCBAUTOC_RULE     (3 + WINDOWS_ID_BIAS)
	#define CTL_PRCBAUTOC_RULEGRP     (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCBAUTOC_RULEGRP  (5 + WINDOWS_ID_BIAS)
	#define CTL_PRCBAUTOC_OP          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PRCBAUTOC_OP       (7 + WINDOWS_ID_BIAS)
#define DLG_XMLVLD                1326
	#define CTL_XMLVLD_XSDFILE        (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_XMLVLD_XSDFILE     (2 + WINDOWS_ID_BIAS)
	#define CTL_XMLVLD_XMLFILE        (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_XMLVLD_XMLFILE     (4 + WINDOWS_ID_BIAS)
#define DLG_SELIACC               1327
	#define CTL_SELIACC_WHAT          (1 + WINDOWS_ID_BIAS)
#define DLG_SELLKBKPRICE          1328
	#define CTL_SELLKBKPRICE_LIST     (1 + WINDOWS_ID_BIAS)
	#define CTL_SELLKBKPRICE_INFO     (2 + WINDOWS_ID_BIAS)
#define DLG_ARMASSUPD             1329
	#define CTL_ARMASSUPD_WHAT        (1 + WINDOWS_ID_BIAS)
	#define CTL_ARMASSUPD_FLAGS       (2 + WINDOWS_ID_BIAS)
	#define CTL_ARMASSUPD_PAYPERIOD   (3 + WINDOWS_ID_BIAS)
	#define CTL_ARMASSUPD_DELIVERY    (4 + WINDOWS_ID_BIAS)
	#define CTL_ARMASSUPD_ST_WARN     (5 + WINDOWS_ID_BIAS)
#define DLG_DATEADD               1330
	#define CTL_DATEADD_SRC           (1 + WINDOWS_ID_BIAS)
	#define CTL_DATEADD_PRD           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_DATEADD_PRD        (3 + WINDOWS_ID_BIAS)
	#define CTL_DATEADD_PRDCOUNT      (4 + WINDOWS_ID_BIAS)
	#define CTL_DATEADD_RESULT        (5 + WINDOWS_ID_BIAS)
#define DLG_VETISCFG              1331
	#define CTL_VETISCFG_USER         (1 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_PASSW        (2 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_APIKEY       (3 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_TIMEOUT      (4 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_DOCTUSER     (5 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_DOCTPASSW    (6 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_DOCCRTDELAY  (7 + WINDOWS_ID_BIAS)
	#define CTL_VETISCFG_FLAGS        (8 + WINDOWS_ID_BIAS)
#define DLG_PHNCPANE              1332
	#define cmNewContact              101
	#define cmAttachContact           102
	#define cmPhnCPaneAction          103
	#define cmEditContact             104
	#define cmMinimizeWindow          105
	#define CTL_PHNCPANE_PHN          (1 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_NAME         (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_PHNCPANE_NAME      (3 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_ST_INFO      (4 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_INFOLIST     (5 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_INFSEL       (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_PHNCPANE_INFSEL    (7 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_LISTMODE     (8 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_AUTOCLOSE    (9 + WINDOWS_ID_BIAS)
	#define CTL_PHNCPANE_ACTION       (10 + WINDOWS_ID_BIAS)
#define DLG_EQCFGEXT              1333
	// CTL_EQUIPCFG_XXX
#define DLG_LOTXCLIST             1334
	#define CTL_LOTXCLIST_LIST        (1 + WINDOWS_ID_BIAS)
	#define CTL_LOTXCLIST_INFO        (2 + WINDOWS_ID_BIAS)
#define DLG_GSFILT                1335
	#define cmPGoodsFilt              101
	#define cmSGoodsFilt              102
	#define CTL_GSFILT_PGOODS         (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSFILT_PGOODS      (2 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_PGGRP          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSFILT_PGGRP       (4 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_SGOODS         (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSFILT_SGOODS      (6 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_SGGRP          (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_GSFILT_SGGRP       (8 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_FLAGS          (9 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_ORDER          (10 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_HSFLAGS        (11 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_HRFLAGS        (12 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_ISFLAGS        (13 + WINDOWS_ID_BIAS)
	#define CTL_GSFILT_IRFLAGS        (14 + WINDOWS_ID_BIAS)
#define DLG_SELOBJBYPHN           1336
	#define CTL_SELOBJBYPHN_WHAT      (1 + WINDOWS_ID_BIAS)
	#define CTL_SELOBJBYPHN_INFO      (2 + WINDOWS_ID_BIAS)
	#define CTL_SELOBJBYPHN_EXT       (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELOBJBYPHN_EXT    (4 + WINDOWS_ID_BIAS)
	#define CTL_SELOBJBYPHN_SRCH      (5 + WINDOWS_ID_BIAS)
#define DLG_CORCC                 1337
	#define CTL_CORCC_LOG             (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORCC_LOG          (2 + WINDOWS_ID_BIAS)
	#define CTL_CORCC_FLAGS           (3 + WINDOWS_ID_BIAS)
	#define CTL_CORCC_SESSID          (4 + WINDOWS_ID_BIAS)
#define DLG_SELACNBYPHN           1338
	#define CTL_SELACNBYPHN_WHAT      (1 + WINDOWS_ID_BIAS)
	#define CTL_SELACNBYPHN_INFO      (2 + WINDOWS_ID_BIAS)
	#define CTL_SELACNBYPHN_EXT       (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SELACNBYPHN_EXT    (4 + WINDOWS_ID_BIAS)
	#define CTL_SELACNBYPHN_PERSON    (5 + WINDOWS_ID_BIAS)
	#define CTL_SELACNBYPHN_SCARD     (6 + WINDOWS_ID_BIAS)
	#define CTL_SELACNBYPHN_LOC       (7 + WINDOWS_ID_BIAS)
#define DLG_CORGSTRUC             1339
	#define CTL_CORGSTRUC_LOG         (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORGSTRUC_LOG      (2 + WINDOWS_ID_BIAS)
	#define CTL_CORGSTRUC_FLAGS       (3 + WINDOWS_ID_BIAS)
#define DLG_NGSVIEW               1340
#define DLG_VETDOCFLT             1341
	#define CTL_VETDOCFLT_PRD         (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_VETDOCFLT_PRD      (2 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_WBPRD       (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_VETDOCFLT_WBPRD    (4 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_STATUS      (5 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_WH          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETDOCFLT_WH       (7 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_MAINORG     (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETDOCFLT_MAINORG  (9 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_ACTIONS     (10 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_FROMP       (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETDOCFLT_FROMP    (12 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_FROML       (13 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETDOCFLT_FROML    (14 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_TOP         (15 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETDOCFLT_TOP      (16 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_TOL         (17 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETDOCFLT_TOL      (18 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_FTEXPIRY    (19 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_FTMATCH     (20 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCFLT_RFIMPF      (21 + WINDOWS_ID_BIAS)
#define DLG_VETVDOC               1342
	#define CTL_VETVDOC_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_UUID          (2 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_DT            (3 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_NUM           (4 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_WBDT          (5 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_WBNUM         (6 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_FROMSI        (7 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_FROMNAM       (8 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_FROMLNK       (9 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_TOSI          (10 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_TONAM         (11 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_TOLNK         (12 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_PRODSI        (13 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_PRODNAM       (14 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_SPRODSI       (15 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_SPRODNAM      (16 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_ITEMSI        (17 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_ITEMNAM       (18 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_ITEMLNK       (19 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_VDFORM        (20 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_VDSTATUS      (21 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_VDTYPE        (22 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_QTTY          (23 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_LINKBILL      (24 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_ST_FLAGS      (25 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_ACKQTTY       (26 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_ACK           (27 + WINDOWS_ID_BIAS)
	#define CTL_VETVDOC_EXPIRY        (28 + WINDOWS_ID_BIAS)

#define DLG_SELVDOCUPD            1343
	#define CTL_SELVDOCUPD_PERIOD      (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_SELVDOCUPD_PERIOD   (2 + WINDOWS_ID_BIAS)
#define DLG_MTCHPSN               1344
	#define CTL_MTCHPSN_SRCINFO       (1 + WINDOWS_ID_BIAS)
	#define CTL_MTCHPSN_PSN           (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_MTCHPSN_PSN        (3 + WINDOWS_ID_BIAS)
	#define CTL_MTCHPSN_LOC           (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_MTCHPSN_LOC        (5 + WINDOWS_ID_BIAS)
#define DLG_SELVDUTLZ             1345
#define DLG_VETISIC               1346
	// CTL_VETDOCFLT_XXX
#define DLG_SELVETMATCHOBJ        1347
#define DLG_AGTBILL               1348
	// CTL_BILL_XXX
#define DLG_VETDOCTOTAL           1349
	#define CTL_VETDOCTOTAL_COUNT     (1 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_CRCT      (2 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_CONFCT    (3 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_WDCT      (4 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_UTCT      (5 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_FINCT     (6 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_OGCT      (7 + WINDOWS_ID_BIAS)
	#define CTL_VETDOCTOTAL_STKCT     (8 + WINDOWS_ID_BIAS)
#define DLG_TSVIEW                1350
#define DLG_JOB_SNRSCO            1351
	#define CTL_JOB_SNRSCO_DT         (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_JOB_SNRSCO_DT      (2 + WINDOWS_ID_BIAS)
	#define CTL_JOB_SNRSCO_TM         (3 + WINDOWS_ID_BIAS)
#define DLG_TIMSER                1352
	#define cmTimSerStat              101
	#define CTL_TIMSER_NAME           (1 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_SYMB           (2 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_ID             (9 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_FLAGS          (10 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_STAT           (11 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_BUYMARG        (12 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_SELLMARG       (13 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_PREC           (14 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_QUANT          (15 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_OPTMD          (16 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_OPTMDS         (17 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_AVGSPRD        (18 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_TGTQUANT       (19 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_MANMARG        (20 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_TYPE           (21 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_FXSTAKE        (22 + WINDOWS_ID_BIAS)
	#define CTL_TIMSER_SBIDT          (23 + WINDOWS_ID_BIAS) // strategy build init date
	#define CTLCAL_TIMSER_SBIDT       (24 + WINDOWS_ID_BIAS) // strategy build init date
	#define CTL_TIMSER_TSSMODEL       (25 + WINDOWS_ID_BIAS)
	#define CTLSEL_TIMSER_TSSMODEL    (26 + WINDOWS_ID_BIAS)
#define DLG_TSSA                  1353 //
	#define CTL_TSSA_TS               (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_TSSA_TS            (2 + WINDOWS_ID_BIAS)
	#define CTL_TSSA_FLAGS            (3 + WINDOWS_ID_BIAS)
	#define CTL_TSSA_CLOSEMODE        (4 + WINDOWS_ID_BIAS)
	#define CTL_TSSA_SBTDT            (5 + WINDOWS_ID_BIAS) // strategy build till date
	#define CTLCAL_TSSA_SBTDT         (6 + WINDOWS_ID_BIAS) // strategy build till date
	#define CTL_TSSA_SBTDT2           (7 + WINDOWS_ID_BIAS) // strategy build till date (phase2)
	#define CTLCAL_TSSA_SBTDT2        (8 + WINDOWS_ID_BIAS) // strategy build till date (phase2)
#define DLG_LOTXCCKLIST           1354
	#define CTL_LOTXCCKLIST_LIST      (1 + WINDOWS_ID_BIAS)
	#define CTL_LOTXCCKLIST_INFO      (2 + WINDOWS_ID_BIAS)
	#define CTL_LOTXCCKLIST_FLAGS     (3 + WINDOWS_ID_BIAS)
#define DLG_TIMSERCFG             1355
	#define CTL_TIMSERCFG_FLAGS       (1 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_MAXSTAKE    (2 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_ALIMPART    (3 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_ALIMABS     (4 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_MINPOTENT   (5 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_LIST        (6 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_TSFLSHTMR   (7 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_RVRMLQ      (8 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_RVRMAS      (9 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_TERMTMADJ   (10 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_LOCDEVPT    (11 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_LDMTFACTOR  (12 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_MTMAXERR    (13 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFG_TESTCOUNT   (14 + WINDOWS_ID_BIAS)
#define DLG_TIMSERCFGITEM         1356
	#define CTL_TIMSERCFGITEM_FLAGS   (1 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERCFGITEM_TS      (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_TIMSERCFGITEM_TS   (3 + WINDOWS_ID_BIAS)
#define DLG_ATTACHBILL            1357
	#define CTL_ATTACHBILL_SEL        (1 + WINDOWS_ID_BIAS)
	#define CTL_ATTACHBILL_INFO       (2 + WINDOWS_ID_BIAS)
#define DLG_FPACKTYPE             1358
	#define CTL_FPACKTYPE_NAME        (1 + WINDOWS_ID_BIAS)
	#define CTL_FPACKTYPE_SYMB        (2 + WINDOWS_ID_BIAS)
	#define CTL_FPACKTYPE_ID          (3 + WINDOWS_ID_BIAS)
	#define CTL_FPACKTYPE_FLAGS       (10 + WINDOWS_ID_BIAS)
#define DLG_FPACKAGE              1359
	#define CTL_FPACKAGE_FPT          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_FPACKAGE_FPT       (2 + WINDOWS_ID_BIAS)
	#define CTL_FPACKAGE_QTTY         (3 + WINDOWS_ID_BIAS)
#define DLG_BRANDFILT             1360
	#define CTL_BRANDFILT_OWNER       (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BRANDFILT_OWNER    (2 + WINDOWS_ID_BIAS)
	#define CTL_BRANDFILT_NAMESTR     (3 + WINDOWS_ID_BIAS)
	#define CTL_BRANDFILT_FLAGS       (4 + WINDOWS_ID_BIAS)
	#define CTL_BRANDFILT_ORDER       (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_BRANDFILT_ORDER    (6 + WINDOWS_ID_BIAS)
#define DLG_UPDPLIST              1361
	#define CTL_UPDPLIST_WHAT         (1 + WINDOWS_ID_BIAS)
	#define CTL_UPDPLIST_EXTOBJ       (2 + WINDOWS_ID_BIAS)
	#define CTLSEL_UPDPLIST_EXTOBJ    (3 + WINDOWS_ID_BIAS)
	#define CTL_UPDPLIST_YES          (4 + WINDOWS_ID_BIAS)
#define DLG_WOLAYOUT              1362 // WhatmanObjectLayout
	#define CTL_WOLAYOUT_SYMB         (1 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYOUT_FLAGS        (2 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYOUT_LAY          (3 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYOUT_ADJ          (4 + WINDOWS_ID_BIAS)
#define DLG_LAYOENTRY             1363 // Параметры члена 2-d раскладки
	#define CTL_LAYOENTRY_SIZE        (1 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_MARG        (2 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_PARENT      (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_LAYOENTRY_PARENT   (5 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_GROWF       (6 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_SHRNKF      (7 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_ASPR        (8 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_GRAVITY     (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_LAYOENTRY_GRAVITY  (10 + WINDOWS_ID_BIAS)
	#define CTL_LAYOENTRY_ALGNSLF     (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_LAYOENTRY_ALGNSLF  (12 + WINDOWS_ID_BIAS)
#define DLG_VETISENT              1364
	#define cmCreateVetisEnterprise   101
	#define CTL_VETISENT_LOC          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISENT_LOC       (2 + WINDOWS_ID_BIAS)
	#define CTL_VETISENT_ORG          (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISENT_ORG       (4 + WINDOWS_ID_BIAS)
	#define CTL_VETISENT_BENTY        (5 + WINDOWS_ID_BIAS)
	#define CTL_VETISENT_ENT          (6 + WINDOWS_ID_BIAS)
	#define CTL_VETISENT_COUNTRY      (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISENT_COUNTRY   (8 + WINDOWS_ID_BIAS)
	#define CTL_VETISENT_REGION       (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISENT_REGION    (10 + WINDOWS_ID_BIAS)
	#define CTL_VETISENT_LOCALITY     (11 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISENT_LOCALITY  (12 + WINDOWS_ID_BIAS)
#define DLG_MOBCLMNN              1365
	#define CTL_MOBCLMNN_LIST         (1 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLMNN_OBJ          (2 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLMNN_STRUC        (3 + WINDOWS_ID_BIAS)
#define DLG_MOBCLEDT              1366
	#define cmSelectDlZone            101
	#define cmSelectDlField           102
	#define cmFormat                  103
	#define CTL_MOBCLEDT_Z            (1 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLEDT_FN           (2 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLEDT_TXT          (3 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLEDT_AGGR         (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_MOBCLEDT_AGGR      (5 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLEDT_TYPE         (6 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLEDT_FMT          (7 + WINDOWS_ID_BIAS)
	#define CTL_MOBCLEDT_FMTBUT       (8 + WINDOWS_ID_BIAS)
#define DLG_TESTVETIS             1367
	#define CTL_TESTVETIS_INFO        (1 + WINDOWS_ID_BIAS)
	#define CTL_TESTVETIS_FLAGS       (2 + WINDOWS_ID_BIAS)
	#define CTL_TESTVETIS_PARAM       (3 + WINDOWS_ID_BIAS)
#define DLG_UNIFINDOBJ            1368
	#define CTL_UNIFINDOBJ_WFLAGS     (1 + WINDOWS_ID_BIAS)
	#define CTL_UNIFINDOBJ_FLAGS      (2 + WINDOWS_ID_BIAS)
	#define CTL_UNIFINDOBJ_MPATTERN   (3 + WINDOWS_ID_BIAS)
	#define CTL_UNIFINDOBJ_PERIOD     (4 + WINDOWS_ID_BIAS)
	#define CTLCAL_UNIFINDOBJ_PERIOD  (5 + WINDOWS_ID_BIAS)
	#define CTL_UNIFINDOBJ_INFO       (6 + WINDOWS_ID_BIAS)
#define DLG_VETISPI               1369
	#define cmCreateVetisProductItem   101
	#define CTL_VETISPI_GOODSGRP      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISPI_GOODSGRP   (2 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_GOODS         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISPI_GOODS      (4 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_LOC           (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISPI_LOC        (6 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_PI            (7 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_PROD          (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISPI_PROD       (9 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_SUBPROD       (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISPI_SUBPROD    (11 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_ORG           (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_VETISPI_ORG        (13 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_BENTY         (14 + WINDOWS_ID_BIAS)
	#define CTL_VETISPI_ENT           (15 + WINDOWS_ID_BIAS)
#define DLG_MQCCFG                1370
	#define cmMqcGetCommonConfig      101
	#define CTL_MQCCFG_HOST           (1 + WINDOWS_ID_BIAS)
	#define CTL_MQCCFG_USER           (2 + WINDOWS_ID_BIAS)
	#define CTL_MQCCFG_SECRET         (3 + WINDOWS_ID_BIAS)
	#define CTL_MQCCFG_DATADOMAIN     (4 + WINDOWS_ID_BIAS)
	#define CTL_MQCCFG_VIRTHOST       (5 + WINDOWS_ID_BIAS)
#define DLG_RMVTIMSER             1371
	#define CTL_RMVTIMSER_WHAT        (1 + WINDOWS_ID_BIAS)
	#define CTL_RMVTIMSER_WARN        (2 + WINDOWS_ID_BIAS)
#define DLG_TESTMQC               1372
	#define CTL_TESTMQC_CMD           (1 + WINDOWS_ID_BIAS)
	#define CTL_TESTMQC_INFO          (2 + WINDOWS_ID_BIAS)
	#define CTL_TESTMQC_RESULT        (3 + WINDOWS_ID_BIAS)
	#define CTL_TESTMQC_TIMING        (4 + WINDOWS_ID_BIAS)
#define DLG_QUOTEREQFLT           1373
    #define CTL_QUOTEREQFLT_PERIOD    (1 + WINDOWS_ID_BIAS)
    #define CTLCAL_QUOTEREQFLT_PERIOD (2 + WINDOWS_ID_BIAS)
    #define CTL_QUOTEREQFLT_OP        (3 + WINDOWS_ID_BIAS)
    #define CTLSEL_QUOTEREQFLT_OP     (4 + WINDOWS_ID_BIAS)
    #define CTL_QUOTEREQFLT_AR        (5 + WINDOWS_ID_BIAS)
    #define CTLSEL_QUOTEREQFLT_AR     (6 + WINDOWS_ID_BIAS)
    #define CTL_QUOTEREQFLT_FLAGS     (7 + WINDOWS_ID_BIAS)
#define DLG_QUOT_RB               1374 // @erik v10.5.8
    #define CTL_QUOTROLLBACK_DATE     (1 + WINDOWS_ID_BIAS)
    #define CTL_QUOTROLLBACK_TIME     (2 + WINDOWS_ID_BIAS)
    #define CTLCAL_QUOTROLLBACK_DATE  (3 + WINDOWS_ID_BIAS)
    #define CTLTM_QUOTROLLBACK_TIME   (4 + WINDOWS_ID_BIAS)
#define DLG_DIAGPOSNODE           1375
	#define CTL_DIAGPOSNODE_RESULT    (1 + WINDOWS_ID_BIAS)
#define DLG_CHZNIX                1376
	#define CTL_CHZNIX_GUA            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHZNIX_GUA         (2 + WINDOWS_ID_BIAS)
	#define CTL_CHZNIX_WHAT           (3 + WINDOWS_ID_BIAS)
	#define CTL_CHZNIX_PARAM          (4 + WINDOWS_ID_BIAS)
	#define CTL_CHZNIX_HINT           (5 + WINDOWS_ID_BIAS)
	#define CTL_CHZNIX_SUPPL          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHZNIX_SUPPL       (7 + WINDOWS_ID_BIAS)
#define DLG_TIMSERTEST            1377
	#define CTL_TIMSERTEST_WHAT       (1 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERTEST_DIST       (2 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERTEST_TS         (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_TIMSERTEST_TS      (4 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERTEST_PARAM      (5 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERTEST_INFO       (6 + WINDOWS_ID_BIAS)
	#define CTL_TIMSERTEST_STRTILL    (7 + WINDOWS_ID_BIAS)
	#define CTLCAL_TIMSERTEST_STRTILL (8 + WINDOWS_ID_BIAS)
#define DLG_CHZNMARK              1378
	#define CTL_CHZNMARK_INPUT        (1  + IMPEXP_CTL_BIAS)
	#define CTL_CHZNMARK_INFO         (2  + IMPEXP_CTL_BIAS)
#define DLG_DFCRRULEVIEW          1379
#define DLG_TSSMODEL              1380
	#define CTL_TSSMODEL_ID           (1  + IMPEXP_CTL_BIAS)
	#define CTL_TSSMODEL_NAME         (2  + IMPEXP_CTL_BIAS)
	#define CTL_TSSMODEL_SYMB         (3  + IMPEXP_CTL_BIAS)
	#define CTL_TSSMODEL_FLAGS        (4  + IMPEXP_CTL_BIAS)
	#define CTL_TSSMODEL_OPTCRIT      (5  + IMPEXP_CTL_BIAS) // OptTargetCriterion
	#define CTL_TSSMODEL_MFSL         (6  + IMPEXP_CTL_BIAS) // MainFrameSizeList
	#define CTL_TSSMODEL_IFSL         (7  + IMPEXP_CTL_BIAS) // InputFrameSizeList
	#define CTL_TSSMODEL_SBL          (8  + IMPEXP_CTL_BIAS) // StakeBoundList
	#define CTL_TSSMODEL_MDQL         (9  + IMPEXP_CTL_BIAS) // MaxDuckQuantList
	#define CTL_TSSMODEL_TQL          (10  + IMPEXP_CTL_BIAS) // TargetQuantList
	#define CTL_TSSMODEL_DATASINCE    (11  + IMPEXP_CTL_BIAS) // UseDataSince
	#define CTLCAL_TSSMODEL_DATASINCE (12  + IMPEXP_CTL_BIAS) // UseDataSince
	#define CTL_TSSMODEL_BSUBSCHNK    (13  + IMPEXP_CTL_BIAS) // BestSubsetOptChunk
	#define CTL_TSSMODEL_BSUBSDIM     (14  + IMPEXP_CTL_BIAS) // BestSubsetDimention
	#define CTL_TSSMODEL_BSUBSPHIT    (15  + IMPEXP_CTL_BIAS) // BestSubsetMaxPhonyIters
	#define CTL_TSSMODEL_MFRC         (16  + IMPEXP_CTL_BIAS) // MainFrameRangeCount
	#define CTL_TSSMODEL_OPTRSTEP     (17  + IMPEXP_CTL_BIAS) // OptRangeStep
	//#define CTL_TSSMODEL_OPTRSTEPMP   (18  + IMPEXP_CTL_BIAS) // OptRangeStepMkPart
	#define CTL_TSSMODEL_OPTRMLIM     (19  + IMPEXP_CTL_BIAS) // OptRangeMultiLimit
	#define CTL_TSSMODEL_OPTRMEXTP    (20  + IMPEXP_CTL_BIAS) // OptRangeMaxExtProbe
	#define CTL_TSSMODEL_CHAOSFACTOR  (21  + IMPEXP_CTL_BIAS) // ChaosFactor
	#define CTL_TSSMODEL_MFERRLIM     (22  + IMPEXP_CTL_BIAS) // InitMainTrendErrLimit
	#define CTL_TSSMODEL_IFERRLIM     (23  + IMPEXP_CTL_BIAS) // InitTrendErrLimit
	#define CTL_TSSMODEL_MINWINRATE   (24  + IMPEXP_CTL_BIAS) // MinWinRate
	#define CTL_TSSMODEL_OMINWINRATE  (25  + IMPEXP_CTL_BIAS) // OverallWinRateLimit
	#define CTL_TSSMODEL_OPTRSTEPCT   (26  + IMPEXP_CTL_BIAS) // OptRangeStepCount
	#define CTL_TSSMODEL_TRTOPTRSTEP  (27  + IMPEXP_CTL_BIAS) //
	#define CTL_TSSMODEL_TRTIFERRLIM  (28  + IMPEXP_CTL_BIAS) // InitTrendErrLimit
	#define CTL_TSSMODEL_SPPS         (29  + IMPEXP_CTL_BIAS) //
	#define CTL_TSSMODEL_CQAMATCH     (30  + IMPEXP_CTL_BIAS) //
	#define CTL_TSSMODEL_MINSIMIL     (31  + IMPEXP_CTL_BIAS) //
	#define CTL_TSSMODEL_ADOPTSLTS    (32  + IMPEXP_CTL_BIAS) //
	#define CTL_TSSMODEL_MAXSTCTVAR   (33  + IMPEXP_CTL_BIAS) //
#define DLG_TSSMODELVIEW          1381

#define DLG_TSOELGRAPH            1382
	#define CTL_TSOELGRAPH_FILE      (1  + IMPEXP_CTL_BIAS) //
	#define CTLSEL_TSOELGRAPH_FILE   (2  + IMPEXP_CTL_BIAS) //
	#define CTL_TSOELGRAPH_FUNC      (3  + IMPEXP_CTL_BIAS) //
	#define CTL_TSOELGRAPH_GRPPT     (4  + IMPEXP_CTL_BIAS) //
	#define CTL_TSOELGRAPH_SEGM      (5  + IMPEXP_CTL_BIAS) //
#define DLG_RTDYNOBJ              1383
	#define CTL_RTDYNOBJ_FLAGS       (1  + IMPEXP_CTL_BIAS) //
	#define CTL_RTDYNOBJ_SFLAGS      (11 + IMPEXP_CTL_BIAS) //
#define DLG_BRANDTOTAL            1384 // Итоги по выборке брендов
	#define CTL_BRANDTOTAL_COUNT     (1 + WINDOWS_ID_BIAS)
#define DLG_VK_SENDMSG            1385
	#define cmSendVkMsg               101
	#define CTL_VK_MSG                (1 + IMPEXP_CTL_BIAS)
	#define CTL_VK_FRIENDS_LIST       (2 + IMPEXP_CTL_BIAS)
#define DLG_VK_POST               1386
    #define cmSendVkPost              101
    #define CTL_VK_POST_TEXT          (1 + IMPEXP_CTL_BIAS)//
	#define CTL_VKPOST_IMAGE          (2 + IMPEXP_CTL_BIAS)
#define DLG_VKPOSTGOOD               1387
	#define CTL_VK_GOODS_NAME         (1 + IMPEXP_CTL_BIAS)//
	#define CTL_VK_GOODS_CAT          (2 + IMPEXP_CTL_BIAS)//
	#define CTL_VK_GOODS_URL          (3 + IMPEXP_CTL_BIAS)//
	#define CTL_VK_GOODS_DESCR        (4 + IMPEXP_CTL_BIAS)//
#define DLG_EDITTEXTPARAM         1388
	#define CTL_EDITTEXTPARAM_PATH    (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_EDITTEXTPARAM_PAT  (2 + WINDOWS_ID_BIAS)
#define DLG_VKGUACFG              1389
	#define CTL_VKGUACFG_LOGIN_REQ    (1 + IMPEXP_CTL_BIAS)//
	#define CTL_VKGUACFG_LOGIN_URL    (2 + IMPEXP_CTL_BIAS)//
	#define CTL_VKGUACFG_GROUP_ID     (3 + IMPEXP_CTL_BIAS)//
#define DLG_EVNTSUBSCR            1390
	#define cmUserList                101
	// cmFilter, cmGuaList
	#define CTL_EVNTSUBSCR_ID         (1 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_NAME       (2 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_SYMB       (3 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_TYPE       (4 + IMPEXP_CTL_BIAS)
	#define CTLSEL_EVNTSUBSCR_TYPE    (5 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_FLAGS      (6 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_OBJ        (7 + IMPEXP_CTL_BIAS)
	#define CTLSEL_EVNTSUBSCR_OBJ     (8 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_MSG        (9 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_MINDI      (10 + IMPEXP_CTL_BIAS) // MinDetectionInterval
	#define CTL_EVNTSUBSCR_COLOR      (11 + IMPEXP_CTL_BIAS)
	#define CTLSEL_EVNTSUBSCR_COLOR   (12 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTSUBSCR_SELCOLOR   (13 + IMPEXP_CTL_BIAS)
#define DLG_SU_UDS                1391 // Parameters for set up UDS integration
	#define CTL_SUUDS_LOGIN           (1 + IMPEXP_CTL_BIAS)
	#define CTL_SUUDS_APIKEY          (2 + IMPEXP_CTL_BIAS)
	#define CTL_SUUDS_SCARDSER        (3 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SUUDS_SCARDSER     (4 + IMPEXP_CTL_BIAS)
	#define CTL_SUUDS_HINT            (5 + IMPEXP_CTL_BIAS)
	#define CTL_SUUDS_INFO            (6 + IMPEXP_CTL_BIAS)
	#define CTL_SUUDS_GUA             (7 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SUUDS_GUA          (8 + IMPEXP_CTL_BIAS)
	#define CTL_SUUDS_LOGO            (9 + IMPEXP_CTL_BIAS)
#define DLG_EVNTFILT              1392
	#define CTL_EVNTFILT_PERIOD       (1 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTFILT_TYPE         (2 + IMPEXP_CTL_BIAS)
	#define CTLSEL_EVNTFILT_TYPE      (3 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTFILT_SUBSCR       (4 + IMPEXP_CTL_BIAS)
	#define CTLSEL_EVNTFILT_SUBSCR    (5 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTFILT_STATUS       (6 + IMPEXP_CTL_BIAS)
	#define CTL_EVNTFILT_USER         (7 + IMPEXP_CTL_BIAS)
	#define CTLSEL_EVNTFILT_USER      (8 + IMPEXP_CTL_BIAS)
#define DLG_SELEVNTSTATUS         1393
	#define CTL_SELEVNTSTATUS_EV      (1 + IMPEXP_CTL_BIAS)
	#define CTL_SELEVNTSTATUS_ST      (2 + IMPEXP_CTL_BIAS)
#define DLG_CMDGROUP              1394
	#define CTL_CMDGROUP_NAME         (1 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_UUID         (2 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_ID           (3 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_CMDLIST      (4 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_IMAGE        (5 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_BGCOLOR      (6 + IMPEXP_CTL_BIAS)
	#define CTLSEL_CMDGROUP_BGCOLOR   (7 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_FLAGS        (8 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_SELBGCOLOR   (9 + IMPEXP_CTL_BIAS)
	#define CTL_CMDGROUP_ST_INFO      (10 + IMPEXP_CTL_BIAS)
#define DLG_SELNEWCMDGROUP        1395
	#define CTL_SELNEWCMDGROUP_SEL    (1 + WINDOWS_ID_BIAS)
#define DLG_SU_VK                 1396 // Parameters for set up VK integration (по образу DLG_VKGUACFG)
	#define CTL_SUVK_LOGIN            (1 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_APIKEY           (2 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_URL              (3 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_GROUPIDENT       (4 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_HINT             (5 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_INFO             (6 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_GUA              (7 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SUVK_GUA           (8 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_LOGO             (9 + IMPEXP_CTL_BIAS)
	#define CTL_SUVK_PAGEIDENT        (10 + IMPEXP_CTL_BIAS)
#define DLG_WOLAYC                1397 // WhatmanObjectContaier
	#define CTL_WOLAYC_IDENT          (1 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYC_FLAGS          (2 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYC_LAY            (3 + WINDOWS_ID_BIAS)
	//#define CTL_WOLAYC_ADJ            (4 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYC_MARG           (5 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYC_JUSTC          (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_WOLAYC_JUSTC       (7 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYC_ALGNC          (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_WOLAYC_ALGNC       (9 + WINDOWS_ID_BIAS)
	#define CTL_WOLAYC_ALGNI          (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_WOLAYC_ALGNI       (11 + WINDOWS_ID_BIAS)
#define DLG_CHZNPARAM             1398
	#define CTL_CHZNPARAM_GUA            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHZNPARAM_GUA         (2 + WINDOWS_ID_BIAS)
	#define CTL_CHZNPARAM_LOC            (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHZNPARAM_LOC         (4 + WINDOWS_ID_BIAS)
	#define CTL_CHZNPARAM_LOGO           (5 + WINDOWS_ID_BIAS)
#define DLG_SELDBSMPL             1399 // Выбор способа создания документа по образцу драфт-документа
	// Ctrls same as for DLG_SELBBSMPL
#define DLG_ALCODECL              1400 // Фильтр алкогольной декларации
	#define CTL_ALCODECL_PERIOD          (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_ALCODECL_PERIOD       (2 + WINDOWS_ID_BIAS)
	#define CTL_ALCODECL_LOC             (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCODECL_LOC          (4 + WINDOWS_ID_BIAS)
	#define CTL_ALCODECL_ACODLIST        (5 + WINDOWS_ID_BIAS)
	#define CTL_ALCODECL_PRODGRP         (6 + WINDOWS_ID_BIAS) // пиво или алкоголь
	#define CTL_ALCODECL_VIEW            (7 + WINDOWS_ID_BIAS) // вариант отображения //
	#define CTL_ALCODECL_IMAGE           (8 + WINDOWS_ID_BIAS)
	#define CTL_ALCODECL_MAINORG         (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_ALCODECL_MAINORG      (10 + WINDOWS_ID_BIAS)
	#define CTL_ALCODECL_CORRNO          (11 + WINDOWS_ID_BIAS)
#define DLG_CHKINPSTFORALL        1401
	#define CTL_CHKINPSTFORALL_ST            (1  + WINDOWS_ID_BIAS)
#define DLG_STQINV                1402
	#define CTL_STQINV_QR                    (1  + WINDOWS_ID_BIAS)
	#define CTL_STQINV_INFO                  (2  + WINDOWS_ID_BIAS)
	#define CTL_STQINV_HINT                  (3  + WINDOWS_ID_BIAS)
#define DLG_STQCLIMATCH           1403
	#define CTL_STQCLIMATCH_OT               (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQCLIMATCH_OT            (2  + WINDOWS_ID_BIAS)
	#define CTL_STQCLIMATCH_OG               (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQCLIMATCH_OG            (4  + WINDOWS_ID_BIAS)
	#define CTL_STQCLIMATCH_OBJ              (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQCLIMATCH_OBJ           (6  + WINDOWS_ID_BIAS)
	#define CTL_STQCLIMATCH_INFO             (7  + WINDOWS_ID_BIAS)
	#define CTL_STQCLIMATCH_AUTO             (8  + WINDOWS_ID_BIAS)
	#define CTL_STQCLIMATCH_CFLAGS           (9  + WINDOWS_ID_BIAS)
#define DLG_STQCMD                1404
	#define CTL_STQCMD_UUID                  (1  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_NAME                  (2  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_BASECMD               (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQCMD_BASECMD            (4  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_OTR                   (5  + WINDOWS_ID_BIAS) // ObjTypeRestriction
	#define CTLSEL_STQCMD_OTR                (6  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_OGR                   (7  + WINDOWS_ID_BIAS) // ObjGroupRestriction
	#define CTLSEL_STQCMD_OGR                (8  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_IMAGE                 (9  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_DESCR                 (10 + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_VCMD                  (11  + WINDOWS_ID_BIAS) // Команда вывода отчета
	#define CTLSEL_STQCMD_VCMD               (12  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_REXPRTM               (13  + WINDOWS_ID_BIAS) // Время истечения срока действия результата
	#define CTL_STQCMD_OIR                   (14  + WINDOWS_ID_BIAS) // ObjIdRestriction
	#define CTLSEL_STQCMD_OIR                (15  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_FLAGS                 (16  + WINDOWS_ID_BIAS)
	#define CTL_STQCMD_NOTIFYF               (17  + WINDOWS_ID_BIAS) // @v11.5.9 Флаги уведомлений
#define DLG_STQFACE               1405
	#define CTL_STQFACE_LANG                 (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQFACE_LANG              (2  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_VRF                  (3  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_CN                   (4  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_NAME                 (5  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_PATRONYM             (6  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_SURNAME              (7  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_DOB                  (8  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_PHONE                (9  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_COUNTRY              (10  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_ZIP                  (11  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_CITY                 (12  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_STREET               (13  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_ADDR                 (14  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_GLN                  (15  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_RUINN                (16  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_RUKPP                (17  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_RUSNILS              (18  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_GEOLOC               (19  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_EXPIRYP              (20  + WINDOWS_ID_BIAS)
	#define CTL_STQFACE_IMAGE                (21  + WINDOWS_ID_BIAS)
#define DLG_STQFACELIST           1406
	#define CTL_STQFACELIST_LIST             (1  + WINDOWS_ID_BIAS)
	#define CTL_STQFACELIST_INFO             (2  + WINDOWS_ID_BIAS)
#define DLG_STQCFG                1407
	#define CTL_STQCFG_URL                   (1  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_MQBAUTH               (2  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_MQBSECR               (3  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_LOCLURL               (4  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_LOCLMQBAUTH           (5  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_LOCLMQBSECR           (6  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_FEATURES              (7  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_EXPIRYP               (8  + WINDOWS_ID_BIAS)
	#define CTL_STQCFG_ROLE                  (9  + WINDOWS_ID_BIAS) // @v11.2.8
	#define CTL_STQCFG_FLAGS                 (10  + WINDOWS_ID_BIAS) // @v11.2.10
#define DLG_FLEXLAYOUTLIST        1408 // Иерархический список layout'ов
	#define CTL_FLEXLAYOUTLIST_LIST          (1  + WINDOWS_ID_BIAS)
#define DLG_SYSJTOTAL             1409
	#define CTL_SYSJTOTAL_COUNT              (1  + WINDOWS_ID_BIAS)
#define DLG_GSTRUCTREE            1410 // @v11.2.11 Отображение дерева компонентов структуры
	#define CTL_GSTRUCTREE_LIST              (1  + WINDOWS_ID_BIAS)
#define DLG_STQATTCPARAM          1411 // @v11.3.3
	#define CTL_STQATTCPARAM_PRCLIST         (1  + WINDOWS_ID_BIAS)
	#define CTL_STQATTCPARAM_PRCTTL          (2  + WINDOWS_ID_BIAS)
	#define CTL_STQATTCPARAM_MAXSCHD         (3  + WINDOWS_ID_BIAS)
	#define CTL_STQATTCPARAM_QK              (4  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQATTCPARAM_QK           (5  + WINDOWS_ID_BIAS)
	#define CTL_STQATTCPARAM_LOC             (6  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQATTCPARAM_LOC          (7  + WINDOWS_ID_BIAS)
	#define CTL_STQATTCPARAM_TSD             (8  + WINDOWS_ID_BIAS) // TimeSheetDiscreteness
#define DLG_STQCLITEST            1412 // @v11.3.3
	#define CTL_STQCLITEST_SVC               (1  + WINDOWS_ID_BIAS)
	#define CTL_STQCLITEST_FUNC              (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQCLITEST_FUNC           (3  + WINDOWS_ID_BIAS)
	#define CTL_STQCLITEST_INFO              (5  + WINDOWS_ID_BIAS)
	#define CTL_STQCLITEST_RESULT            (4  + WINDOWS_ID_BIAS)
	#define CTL_STQCLITEST_INPUT             (6  + WINDOWS_ID_BIAS)
#define DLG_STQIDXCPARAM          1413 // @v11.3.4
	#define cmPrcList                        101
	#define CTL_STQIDXCPARAM_GGRP            (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQIDXCPARAM_GGRP         (2  + WINDOWS_ID_BIAS)
	#define CTL_STQIDXCPARAM_PRCL            (3  + WINDOWS_ID_BIAS)
	#define CTL_STQIDXCPARAM_FLAGS           (5  + WINDOWS_ID_BIAS)
	#define CTL_STQIDXCPARAM_PSNK            (6  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQIDXCPARAM_PSNK         (7  + WINDOWS_ID_BIAS)
#define DLG_FMTREAL               1414
	#define CTL_FMTREAL_PREC                 (1  + WINDOWS_ID_BIAS)
	#define CTL_FMTREAL_FLAGS                (5  + WINDOWS_ID_BIAS)
#define DLG_FMTDATE               1415
	#define CTL_FMTDATE_F                    (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTDATE_F                 (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTDATE_FLAGS             (5  + WINDOWS_ID_BIAS)
#define DLG_FMTTIME               1416
	#define CTL_FMTTIME_F                    (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTTIME_F                 (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTTIME_FLAGS             (3  + WINDOWS_ID_BIAS)
#define DLG_FMTTS                 1417 // timestamp
	#define CTL_FMTTS_DF                     (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTTS_DF                  (2  + WINDOWS_ID_BIAS)
	#define CTL_FMTTS_TF                     (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTTS_TF                  (4  + WINDOWS_ID_BIAS)
	#define CTLSEL_FMTTS_FLAGS               (5  + WINDOWS_ID_BIAS)
#define DLG_STQGINFPARAM          1418 // @v11.4.5
	#define CTL_STQGINFPARAM_LOC             (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQGINFPARAM_LOC          (2  + WINDOWS_ID_BIAS)
	#define CTL_STQGINFPARAM_FLAGS           (3  + WINDOWS_ID_BIAS)
	#define CTL_STQGINFPARAM_POSN            (4  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQGINFPARAM_POSN         (5  + WINDOWS_ID_BIAS)
	// Для следующих 4 диалогов поле идентификаторов управляющих элементов общее (XXX_STQINLPARAM_XXX)
#define DLG_STQINLOPARAM          1419 // @v11.4.6 // orders
#define DLG_STQINLCPARAM          1420 // @v11.4.6 // cchecks
#define DLG_STQINLSPARAM          1421 // @v11.4.6 // tsession
#define DLG_STQINLTPARAM          1422 // @v11.4.6 // todo
	#define cmStatusList                     101 // Команда редактирования списка статусов, которые может устанавливать клиент
	#define CTL_STQINLPARAM_LBD              (1  + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_LOC              (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQINLPARAM_LOC           (3  + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_OP               (4  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQINLPARAM_OP            (5  + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_AGNT             (6  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQINLPARAM_AGNT          (7  + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_CLI              (8  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQINLPARAM_CLI           (9  + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_POS              (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_STQINLPARAM_POS           (11 + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_PRC              (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_STQINLPARAM_PRC           (13 + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_FLAGS            (14 + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_ACTIONS          (15 + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_PERIOD           (16 + WINDOWS_ID_BIAS)
	#define CTLCAL_STQINLPARAM_PERIOD        (17 + WINDOWS_ID_BIAS)
	#define CTL_STQINLPARAM_ACCEPT           (18 + WINDOWS_ID_BIAS) // @v11.7.0
#define DLG_STQSTATUSL            1423
	#define CTL_STQSTATUSL_LIST              (1  + WINDOWS_ID_BIAS)
#define DLG_STQSTATUSE            1424
	#define CTL_STQSTATUSE_NAME              (1  + WINDOWS_ID_BIAS)
	#define CTL_STQSTATUSE_STATUS            (2  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQSTATUSE_STATUS         (3  + WINDOWS_ID_BIAS)
	#define CTL_STQSTATUSE_FLAGS             (4  + WINDOWS_ID_BIAS)
	#define CTL_STQSTATUSE_RSRVST            (5  + WINDOWS_ID_BIAS)
#define DLG_STQDOCPARAM           1425 // @v11.5.0
	#define CTL_STQDOCPARAM_PALM            (1  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQDOCPARAM_PALM         (2  + WINDOWS_ID_BIAS)
	#define CTL_STQDOCPARAM_OP              (3  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQDOCPARAM_OP           (4  + WINDOWS_ID_BIAS)
	#define CTL_STQDOCPARAM_LOC             (5  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQDOCPARAM_LOC          (6  + WINDOWS_ID_BIAS)
	#define CTL_STQDOCPARAM_QK              (7  + WINDOWS_ID_BIAS)
	#define CTLSEL_STQDOCPARAM_QK           (8  + WINDOWS_ID_BIAS)
	#define CTL_STQDOCPARAM_FLAGS           (9  + WINDOWS_ID_BIAS)
#define DLG_UICFG_BITF            1426 // @v11.5.11
	#define CTL_UICFG_BITF_FLAGS            (1  + WINDOWS_ID_BIAS)
#define DLG_GRESTBYLOCLIST        1427 // @v11.6.2 Список остаток товара на разных складах
	#define CTL_GRESTBYLOCLIST_GOODS        (1  + WINDOWS_ID_BIAS)
	#define CTL_GRESTBYLOCLIST_LIST         (2  + WINDOWS_ID_BIAS)
	#define CTL_GRESTBYLOCLIST_TOTAL        (3  + WINDOWS_ID_BIAS)
#define DLG_STQPEVPARAM           1428 // @v11.6.2 Параметры команды создания персональной операции
	#define cmPersonEventPacket             101
	#define CTL_STQPEVPARAM_MAXDIST         (1  + WINDOWS_ID_BIAS)
	#define CTLBUT_STQPEVPARAM_PACK         (2  + WINDOWS_ID_BIAS)
#define DLG_TECHFOLDER            1429 // @v11.6.3 TECH Технология верхнего уровня (папка)
#define DLG_TIACCEPTNC            1430 // @v11.7.0 Результататы приемки строки товарного документа
	#define CTL_TIACCEPTNC_QTTY             (1  + WINDOWS_ID_BIAS)
	#define CTL_TIACCEPTNC_COST             (2  + WINDOWS_ID_BIAS)
	#define CTL_TIACCEPTNC_PRICE            (3  + WINDOWS_ID_BIAS)
	#define CTL_TIACCEPTNC_AQTTY            (4  + WINDOWS_ID_BIAS)
	#define CTL_TIACCEPTNC_ACOST            (5  + WINDOWS_ID_BIAS)
	#define CTL_TIACCEPTNC_APRICE           (6  + WINDOWS_ID_BIAS)
#define DLG_CCPRESCR              1431 // @v11.7.12 Параметры рецепта, по которому продается лекарство в аптеке через кассовый узел
	#define CTL_CCPRESCR_DT                 (1  + WINDOWS_ID_BIAS)
	#define CTLCAL_CCPRESCR_DT              (2  + WINDOWS_ID_BIAS)
	#define CTL_CCPRESCR_SERIAL             (3  + WINDOWS_ID_BIAS)
	#define CTL_CCPRESCR_NUMBER             (4  + WINDOWS_ID_BIAS)
#define DLG_STQBNDRYFLT           1432 // @v11.8.0
	#define CTL_STQBNDRYFLT_KFLAGS          (1  + WINDOWS_ID_BIAS)
#define DLG_IMPEXPCC              1433 // Параметры импорта/экспорта кассовых чеков
	#define CTL_IMPEXPCC_FLAGS              (IMPEXP_CTL_BIAS+1)
	#define CTL_IMPEXPCC_PDFMT              (IMPEXP_CTL_BIAS+2)
	#define CTLSEL_IMPEXPCC_PDFMT           (IMPEXP_CTL_BIAS+3)
	#define CTL_IMPEXPCC_POSNODE            (IMPEXP_CTL_BIAS+4)
	#define CTLSEL_IMPEXPCC_POSNODE         (IMPEXP_CTL_BIAS+5)
#define DLG_RUNIE_CC_IMP          1434 //
	#define CTL_RUNIECC_CFG                 (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_RUNIECC_CFG              (2 + WINDOWS_ID_BIAS)
#define DLG_CCEXTLIST             1435 // @v11.8.8 Список полей расширения кассового чека
	#define CTL_CCEXTLIST_LIST              (1 + WINDOWS_ID_BIAS)
#define DLG_TAGVINT               1436 // @v11.8.12
#define DLG_BIZSC2                1437 // BIZSCORE2 Запись объекта PPOBJ_BIZSCORE2
	#define CTL_BIZSC2_ID                  (1 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_NAME                (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_SYMB                (3 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_PARENT              (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2_PARENT           (5 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_LINKOBJ             (6 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2_LINKOBJ          (7 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_LINKEXT             (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2_LINKEXT          (9 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_AGENTPSNK           (12 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2_AGENTPSNK        (13 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_DATATYPE            (16 + WINDOWS_ID_BIAS) // radio-cluster
	#define CTL_BIZSC2_ENUM                (17 + WINDOWS_ID_BIAS) //
	#define CTLSEL_BIZSC2_ENUM             (18 + WINDOWS_ID_BIAS) // Перечисление для типа данных OTTYP_ENUM or OTTYP_OBJLINK
	#define CTL_BIZSC2_ENUMEXT             (19 + WINDOWS_ID_BIAS) //
	#define CTLSEL_BIZSC2_ENUMEXT          (20 + WINDOWS_ID_BIAS) //
	#define CTL_BIZSC2_AGGRFTM             (21 + WINDOWS_ID_BIAS) // Агрегирующая функция по времени
	#define CTLSEL_BIZSC2_AGGRFTM          (22 + WINDOWS_ID_BIAS) //
	#define CTL_BIZSC2_AGGRFHR             (23 + WINDOWS_ID_BIAS) // Агрегирующая функция по иерархии
	#define CTLSEL_BIZSC2_AGGRFHR          (24 + WINDOWS_ID_BIAS) // 
	#define CTL_BIZSC2_AGGRFAG             (25 + WINDOWS_ID_BIAS) // Агрегирующая функция по участникам (агентам)
	#define CTLSEL_BIZSC2_AGGRFAG          (26 + WINDOWS_ID_BIAS) //
	#define CTL_BIZSC2_TIMECYCLE           (27 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2_TIMECYCLE        (28 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_DESCR               (29 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_FORMULA             (30 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2_CLS                 (31 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2_CLS              (32 + WINDOWS_ID_BIAS)

#define DLG_SWPROGRAM             1438
	#define CTL_SWPROGRAM_ID               (1 + WINDOWS_ID_BIAS)
	#define CTL_SWPROGRAM_NAME             (2 + WINDOWS_ID_BIAS)
	#define CTL_SWPROGRAM_CAT              (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_SWPROGRAM_CAT           (4 + WINDOWS_ID_BIAS)
	#define CTL_SWPROGRAM_EXEFN            (5 + WINDOWS_ID_BIAS)
	#define CTL_SWPROGRAM_IMAGE            (6 + WINDOWS_ID_BIAS)
#define DLG_COMPUTER              1439
	#define CTL_COMPUTER_ID                (1 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_NAME              (2 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_CODE              (3 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_IMAGE             (6 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_UUID              (7 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_MACADR            (8 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_IPADR             (9 + WINDOWS_ID_BIAS)
	#define CTL_COMPUTER_CAT               (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_COMPUTER_CAT            (11 + WINDOWS_ID_BIAS)
#define DLG_COMPCAT               1440 // @v12.0.1 PPOBJ_COMPUTERCATEGORY
	#define CTL_COMPCAT_ID                 (1 + WINDOWS_ID_BIAS)
	#define CTL_COMPCAT_NAME               (2 + WINDOWS_ID_BIAS)
	#define CTL_COMPCAT_SYMB               (3 + WINDOWS_ID_BIAS)
	#define CTL_COMPCAT_CNTEMPL            (4 + WINDOWS_ID_BIAS)
#define DLG_CHKCHZNMARK           1442 // @v12.0.4 Тестовый диалог проверки марки честный знак
	#define cmCheckOffline                 101 // @v12.3.12
	#define CTL_CHKCHZNMARK_INPUT          (1 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_PIC            (2 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_INFO           (3 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_GUA            (4 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKCHZNMARK_GUA         (5 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_FLAGS          (6 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_LOGO           (7 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_GOODS          (8 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKCHZNMARK_GOODS       (9 + WINDOWS_ID_BIAS)
	#define CTL_CHKCHZNMARK_GGRP           (10 + WINDOWS_ID_BIAS)
	#define CTLSEL_CHKCHZNMARK_GGRP        (11 + WINDOWS_ID_BIAS)
#define DLG_IMPSWPGM              1443 // @v12.0.4 Диалог импорт списка исполняемых программ
	#define CTL_IMPSWPGM_FILE              (1 + WINDOWS_ID_BIAS)
	#define CTLBRW_IMPSWPGM_FILE           (2 + WINDOWS_ID_BIAS)
	#define CTL_IMPSWPGM_IMGDIR            (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_IMPSWPGM_IMGDIR         (4 + WINDOWS_ID_BIAS)
#define DLG_GSPPITEM              1444 // @v12.0.6 Элемент структуры товара (для структуры планирования цены)
#define DLG_SELOPREFSUPL          1445 // @v12.0.8 Диалог выбора предпоичтительного поставщика для дозаказа в строках документа заказа
	#define CTL_SELOPREFSUPL_LIST          (1 + WINDOWS_ID_BIAS)
#define DLG_RAWMAT                1446 // @v12.0.10 PPOBJ_RAWMATERIAL
	#define CTL_RAWMAT_ID                  (1 + WINDOWS_ID_BIAS)
	#define CTL_RAWMAT_NAME                (2 + WINDOWS_ID_BIAS)
	#define CTL_RAWMAT_SYMB                (3 + WINDOWS_ID_BIAS)
	#define CTL_RAWMAT_DENS                (4 + WINDOWS_ID_BIAS)
#define DLG_TESTMRKTPLC           1447 // @v12.1.0 Параметры тестирования интерфейсов с маркетплейсами
	#define CTL_TESTMRKTPLC_GUA            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_TESTMRKTPLC_GUA         (2 + WINDOWS_ID_BIAS)
	#define CTL_TESTMRKTPLC_TEXT           (3 + WINDOWS_ID_BIAS)
#define DLG_MRKTPLCIX             1448 // @v12.1.3 Параметры обмена с маркетплейсом
	#define CTL_MRKTPLCIX_GUA              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MRKTPLCIX_GUA           (2 + WINDOWS_ID_BIAS)
#define DLG_MPCFG                 1449 // @v12.1.5 Конфигурация работы с маркетплейсами
	#define CTL_MPCFG_ORDEROP              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_MPCFG_ORDEROP           (2 + WINDOWS_ID_BIAS)
	#define CTL_MPCFG_SALEOP               (3 + WINDOWS_ID_BIAS)
	#define CTLSEL_MPCFG_SALEOP            (4 + WINDOWS_ID_BIAS)
	#define CTL_MPCFG_TRANSFOP             (5 + WINDOWS_ID_BIAS)
	#define CTLSEL_MPCFG_TRANSFOP          (6 + WINDOWS_ID_BIAS)
	#define CTL_MPCFG_RETOP                (7 + WINDOWS_ID_BIAS) // @v12.1.11
	#define CTLSEL_MPCFG_RETOP             (8 + WINDOWS_ID_BIAS) // @v12.1.11
	#define CTL_MPCFG_CLOSTOP              (9 + WINDOWS_ID_BIAS) // @v12.2.1
	#define CTLSEL_MPCFG_CLOSTOP           (10 + WINDOWS_ID_BIAS) // @v12.2.1
#define DLG_FLTORDLOTEXT          1450 // @v12.1.6 Расширение фильтра лотов заказов
#define DLG_GDSOPRMPTOTAL         1451 // @v12.1.11 Итоги товарного отчета по операции в варианте маркетплейса
#define DLG_SELBITMSPCGEN         1452 // @v12.1.12 Выбор специальной генерации для товарных строк документа
	#define CTL_SELBITMSPCGEN_WHAT       (1 + WINDOWS_ID_BIAS)
	#define CTL_SELBITMSPCGEN_INFO       (2 + WINDOWS_ID_BIAS)
#define DLG_TRDPLANBILL           1453 // @v12.1.12 Документ торгового плана
#define DLG_SU_WB                 1454 // @v12.2.0 Parameters for set up Wildberries integration
	#define CTL_SUWB_APIKEY              (1 + IMPEXP_CTL_BIAS)
	#define CTL_SUWB_GUA                 (2 + IMPEXP_CTL_BIAS)
	#define CTLSEL_SUWB_GUA              (3 + IMPEXP_CTL_BIAS)
	#define CTL_SUWB_LOGO                (4 + IMPEXP_CTL_BIAS)
	#define CTL_SUWB_HINT                (5 + IMPEXP_CTL_BIAS)
	#define CTL_SUWB_INFO                (6 + IMPEXP_CTL_BIAS)
#define DLG_PRCRAPTRU             1455 // @v12.2.1 Параметры процессора PrcssrAptekaRu
	#define CTL_PRCRAPTRU_GUA            (1 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCRAPTRU_GUA         (2 + IMPEXP_CTL_BIAS)
	#define CTL_PRCRAPTRU_RCPOP          (3 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCRAPTRU_RCPOP       (4 + IMPEXP_CTL_BIAS)
	#define CTL_PRCRAPTRU_RCPTAG         (5 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCRAPTRU_RCPTAG      (6 + IMPEXP_CTL_BIAS)
	#define CTL_PRCRAPTRU_PERIOD         (7 + IMPEXP_CTL_BIAS)
	#define CTLCAL_PRCRAPTRU_PERIOD      (8 + IMPEXP_CTL_BIAS)
	#define CTL_PRCRAPTRU_FLAGS          (9 + IMPEXP_CTL_BIAS)
	#define CTL_PRCRAPTRU_LOC            (10 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCRAPTRU_LOC         (11 + IMPEXP_CTL_BIAS)
#define DLG_PRCRCLIACST           1456 // @v12.2.2 Параметры процессора PrcssrClientActivityStatistics
	#define CTL_PRCRCLIACST_PRD          (1 + IMPEXP_CTL_BIAS)
	#define CTLCAL_PRCRCLIACST_PRD       (2 + IMPEXP_CTL_BIAS)
	#define CTL_PRCRCLIACST_PK           (3 + IMPEXP_CTL_BIAS)
	#define CTLSEL_PRCRCLIACST_PK        (4 + IMPEXP_CTL_BIAS)
#define DLG_PERSONTOTAL_CA        1457 // @v12.2.2 Итоги по выборке персоналий со статистикой по текущему состоянию клиентской активности
#define DLG_CORHUPTAGS            1458 // @v12.2.5 Корректировка висячих тегов
	#define CTL_CORHUPTAGS_LOG           (1 + WINDOWS_ID_BIAS)
	#define CTL_CORHUPTAGS_FLAGS         (2 + WINDOWS_ID_BIAS)
	#define CTLBRW_CORHUPTAGS_LOG        (3 + WINDOWS_ID_BIAS)
#define DLG_CLIACTFILT            1459 // Фильтр по персоналиям (специализация - анализ клиентской активности). Управляющие элементы: CTL_PSNFLT_XXX
#define DLG_PRCRSRCC              1460 // @v12.2.10 Параметры процессинга собственных исходных кодов проекта
	#define CTL_PRCRSRCC_FLAGS           (1 + WINDOWS_ID_BIAS)
// #define DLG_TAGVLOCLNK            1461 // occupied @v12.2.10 Диалог значения тега, ссылающегося на адрес персоналии
#define DLG_STQTESTPARAM          1462 // @v12.3.1
	#define CTL_STQTESTPARAM_REPLY       (1  + WINDOWS_ID_BIAS)
#define DLG_BIZSC2VFILT           1463 // @v12.3.4 Фильтр по значениям бизнес-показателей 2
	#define CTL_BIZSC2VFILT_BSC          (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2VFILT_BSC       (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2VFILT_PERIOD       (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZSC2VFILT_PERIOD    (4 + WINDOWS_ID_BIAS)
#define DLG_BIZSC2V               1464 // @v12.3.4 Диалог редактирования значения BizScore2
	#define CTL_BIZSC2V_BSC              (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2V_BSC           (2 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_DT               (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_BIZSC2V_DT            (4 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_TM               (5 + WINDOWS_ID_BIAS)
	#define CTLTM_BIZSC2V_TM             (6 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_AGENT            (7 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2V_AGENT         (8 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_LINKOBJ          (9 + WINDOWS_ID_BIAS)
	#define CTLSEL_BIZSC2V_LINKOBJ       (10 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_VAL_R            (21 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_VAL_I            (22 + WINDOWS_ID_BIAS)
	#define CTL_BIZSC2V_VAL_T            (23 + WINDOWS_ID_BIAS)
#define DLG_GSGIFT                1465 // Параметры подарка в товарной структуре
	// Управляющие элементы CTL_GSTRUC_XXX
#define DLG_UNITECFILT            1466 // @v12.3.7 Фильтр unit-анализа
	#define CTL_UNITECFILT_PERIOD        (1 + WINDOWS_ID_BIAS)
	#define CTLCAL_UNITECFILT_PERIOD     (2 + WINDOWS_ID_BIAS)
	#define CTL_UNITECFILT_FLAGS         (3 + WINDOWS_ID_BIAS)
#define DLG_SALESRESTR            1467 // @v12.3.7 PPSalesRestriction
	#define CTL_SALESRESTR_NAME          (1 + WINDOWS_ID_BIAS)
	#define CTL_SALESRESTR_SYMB          (2 + WINDOWS_ID_BIAS)
	#define CTL_SALESRESTR_ID            (3 + WINDOWS_ID_BIAS)
	#define CTL_SALESRESTR_AGE           (4 + WINDOWS_ID_BIAS)
	#define CTL_SALESRESTR_PERIOD        (5 + WINDOWS_ID_BIAS)
	#define CTLCAL_SALESRESTR_PERIOD     (6 + WINDOWS_ID_BIAS)
#define DLG_UNITECIGP             1468 // @v12.3.7 PPViewUnitEc::InitialGoodsParam
	#define CTL_UNITECIGP_RCPTQTY        (1 + WINDOWS_ID_BIAS)
	#define CTL_UNITECIGP_EWDEMAND       (2 + WINDOWS_ID_BIAS)
#define DLG_CR2SRGWPARAM          1469 // @v12.3.7 Параметры запуска шлюза кристалл-setretail 
	#define CTL_CR2SRGWPARAM_ACTIONS     (1 + WINDOWS_ID_BIAS)
	#define CTL_CR2SRGWPARAM_INFO        (2 + WINDOWS_ID_BIAS)
#define DLG_WBGOODSLISTFILT       1470 // @v12.3.8 @experimental Параметры получения списка товаров Wildberries
	#define CTL_WBGOODSLISTFILT_CAT      (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_WBGOODSLISTFILT_CAT   (2 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTFILT_SRCH     (3 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTFILT_ORD      (4 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTFILT_VK       (5 + WINDOWS_ID_BIAS)
#define DLG_POSNQUERYSETR         1471 // @v12.3.9 Диалог запроса к кассовому модулю SetRetail
	// CTL_POSNQUERY_XXX
#define DLG_WBGOODSLISTTOTAL      1472 // @v12.3.9 Итоги выборки товаров Wildberries
	#define CTL_WBGOODSLISTTOTAL_SCOUNT  (1 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTTOTAL_COUNT   (2 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTTOTAL_BCOUNT  (3 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTTOTAL_VCOUNT  (4 + WINDOWS_ID_BIAS)
#define DLG_WBGOODSLISTDETAIL     1473 // @v12.3.9 Детализация товара Wildberries
	#define CTL_WBGOODSLISTDETAIL_ID            (1 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_WH            (2 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_BRANDID       (3 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_BRAND         (4 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_SUPPLID       (5 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_SUPPL         (6 + WINDOWS_ID_BIAS) 
	#define CTL_WBGOODSLISTDETAIL_SUBJID        (7 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_SUBJPARID     (8 + WINDOWS_ID_BIAS) 
	#define CTL_WBGOODSLISTDETAIL_SUPPLRATING   (9 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_PICSCOUNT     (10 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_RATING        (11 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_REVIERATING   (12 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_NMREVIERATING (13 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_FEEDBKCOUNT   (14 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_NMFEEDBKCOUNT (15 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_FEEDBKPTCOUNT (16 + WINDOWS_ID_BIAS)   
	#define CTL_WBGOODSLISTDETAIL_TOTALSTOCK    (17 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_NAME          (18 + WINDOWS_ID_BIAS)
	#define CTL_WBGOODSLISTDETAIL_ENT           (19 + WINDOWS_ID_BIAS) 
#define DLG_CRREXPPARAM_PDF       1474
#define DLG_CRREXPPARAM_RTF       1475
#define DLG_CRREXPPARAM_HTML      1476
#define DLG_CRREXPPARAM_EXCEL     1477
#define DLG_CRREXPPARAM_WINWORD   1478
#define DLG_CRREXPPARAM_CSV       1479
	#define CTL_CRREXPPARAM_INFO                (1 + WINDOWS_ID_BIAS)
	#define CTL_CRREXPPARAM_DEST                (2 + WINDOWS_ID_BIAS)
	#define CTL_CRREXPPARAM_FILE                (3 + WINDOWS_ID_BIAS)
	#define CTLBRW_CRREXPPARAM_FILE             (4 + WINDOWS_ID_BIAS)
	#define CTL_CRREXPPARAM_PGRANGE             (5 + WINDOWS_ID_BIAS)
	#define CTL_CRREXPPARAM_CSVFS               (6 + WINDOWS_ID_BIAS)
	#define CTL_CRREXPPARAM_FLAGS               (7 + WINDOWS_ID_BIAS) // Набор флагов, зависящий от формата данных
	#define CTL_CRREXPPARAM_XLSCONSTCOLWD       (8 + WINDOWS_ID_BIAS)
#define DLG_CHZNLOCPMURL          1480 // @v12.3.11 Диалог редактирования url локального сервера проверки марок честный знак
	#define CTL_CHZNLOCPMURL_ADDR               (1 + WINDOWS_ID_BIAS)
	#define CTL_CHZNLOCPMURL_USER               (2 + WINDOWS_ID_BIAS)
	#define CTL_CHZNLOCPMURL_PW                 (3 + WINDOWS_ID_BIAS)
#define DLG_TESTGLBSVCACSY        1481 // @v12.3.12 Диалог для отображения результатов тестирования доступности глобальных сервисов. TestGlobalServiceAccessibility  
	#define CTL_TESTGLBSVCACSY_LIST             (1 + WINDOWS_ID_BIAS)
	#define CTL_TESTGLBSVCACSY_INFO             (2 + WINDOWS_ID_BIAS)
#define DLG_CHZNLOCPMPANE         1482 // @v12.3.12 Диалог диагностики локального сервера проверки марок чзн
	// cmConfigure
	#define CTL_CHZNLOCPMPANE_URL               (1 + WINDOWS_ID_BIAS)
	#define CTL_CHZNLOCPMPANE_STATE             (2 + WINDOWS_ID_BIAS)
#define DLG_LOCTRANSF_BAILMENT    1483 // LOCTRAN Складская операция - перемещение активов под чужим управлением (domain==LOCTRFRDOMAIN_BAILMENT) //
	// CTL_LOCTRANSF_XXX
#define DLG_OPKMORE_WH            1484 // @v12.4.1 Опции складской операции
	// CTL_OPKMORE_XXX
#define DLG_SELWHOBSMPL           1485 // @v12.4.2 Выбор способа создания документа по образцу документа заказа размещения основных фондов
	// Ctrls same as for DLG_SELBBSMPL
#define DLG_TESTSVRPRINT          1586 // @v12.4.7 Параметры теста серверной печати
	#define CTL_TESTSVRPRINT_PRINTER            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_TESTSVRPRINT_PRINTER         (2 + WINDOWS_ID_BIAS)
#define DLG_CORRECT12304          1587 // @v12.4.10  @construction
	#define CTL_CORRECT12304_POSNODE            (1 + WINDOWS_ID_BIAS)
	#define CTLSEL_CORRECT12304_POSNODE         (2 + WINDOWS_ID_BIAS)
	#define CTL_CORRECT12304_PERIOD             (3 + WINDOWS_ID_BIAS)
	#define CTLCAL_CORRECT12304_PERIOD          (4 + WINDOWS_ID_BIAS)
	#define CTL_CORRECT12304_FLAGS              (5 + WINDOWS_ID_BIAS)
#define DLG_SCARDSERRESTR         1588 // @v12.5.1 Ограничения для серии персональных карт
	#define CTL_SCARDSERRESTR_USAGETM           (1 + WINDOWS_ID_BIAS)
	#define CTL_SCARDSERRESTR_DOW               (2 + WINDOWS_ID_BIAS)
//
// Win32 Dialog Identifiers
//
#define DLGW_PRINT               4090
#define DLGW_PRINTR              4091
	#define CM_WPRINT_PRNSETUP            10
	#define CTL_WPRINT_PRINT             101
	#define CTL_WPRINT_EXPORT            102
	#define CTL_WPRINT_PREVIEW           103
	#define CTL_WPRINT_EXPORTXML         104
	#define CTL_WPRINT_REPORT            110
	#define CTL_WPRINT_NUMCOPIES         111
	#define CTL_WPRINT_SPIN              112
	#define CTL_WPRINT_RPTFILNAM         113
	#define CTL_WPRINT_RPTFILDATTIM      114
/* defined in TVDEFS.H
#define DLGW_MSGBOX              4092
	#define CTL_WMSGBOX_TEXT             200
	#define CTL_WMSGBOX_FIRSTBUTTON      201
	#define CTL_WMSGBOX_LASTBUTTON       207
*/
#define DLGW_SEARCHSTR           4094
	#define CTL_WSEARCHSTR_STR           201
#define DLGW_CUSTOMIZETOOLBAR    4103
	#define CTL_CUSTOMIZETOOLBAR_LIST    201
	#define CTL_CUSTOMIZETOOLBAR_UP      202
	#define CTL_CUSTOMIZETOOLBAR_DOWN    203
#define DLGW_CALENDAR            4104
#define DLGW_CALENDAR_L          4111
	#define CTL_CALENDAR_TODAY          1021
#define DLGW_PERIODCALENDAR      4105
	#define CTL_CALENDAR_DAYS           1021
	#define CTL_CALENDAR_MONTHS         1022
	#define CTL_CALENDAR_QUARTALS       1033
	#define CTL_CALENDAR_YEARS          1034
	#define CTL_CALENDAR_LEFTRESET      1035
	#define CTL_CALENDAR_RESET          1036
	#define CTL_CALENDAR_RIGHTRESET     1037
	#define CTL_CALENDAR_PERIODEDIT     1038
	#define CTL_CALENDAR_WEEKS          1039
	#define CTL_CALENDAR_FASTPRD        1040
	#define CTLSEL_CALENDAR_FASTPRD     1041
#define DLGW_GRAPH               4106
// 4107 occupied by DLGW_TREELISTBOX defined in tvdefs.h
// 4108 occupied by DLGW_OWNDRAWLBX defined in tvdefs.h
// 4109 occupied by DLGW_MSGBOX_L defined in tvdefs.h
// 4110 occupied by DLGW_LBX_L defined in tvdefs.h
// 4111 occupied by DLGW_CALENDAR_L
// 4112 occupied by DLGW_LOGGER
// 4113 occupied by DLGW_LBXFLAT
// 4114 occupied by DLGW_TREELBXNM defined in tvdefs.h
#define DLGW_SELVERSION          4115
	#define CTL_SELVERSION_LIST         1021
#define DLGW_EMPTYDIALOGWINDOW   4116 // @v12.2.4
//
// LastDialog
//
// Icons
//
#define ICON_MAIN_P1              102
#define ICON_MAIN_P2              101
#define ICON_MAIN_SPHERE          103
#define ICON_CAUTION              104
#define ICON_EXCLAM               105
#define ICON_GOODSBILLS           106
#define ICON_ACCBILLS             107
#define ICON_ACCTURNLIST          108
#define ICON_CURREVAL             109
#define ICON_GOODSORDER           110
#define ICON_INVENTORY            111
#define ICON_POOLBILLLIST         112
#define ICON_DRAFTBILLLIST        113
#define ICON_VATBOOK              114
#define ICON_UNIFYGOODSPRICE      115
#define ICON_UPDATEQUOTS          116
#define ICON_AUTOCHARGE           117
#define ICON_WROFFDRAFTS          118
#define ICON_REVALASSETS          119
#define ICON_PROJECT              120
#define ICON_PRJTASK              121
#define ICON_MAINTAINPRJTASK      122
#define ICON_ACCANALYZE           123
#define ICON_CASHBOOK             124
#define ICON_BALANCE              125
#define ICON_VIEWASSET            126
#define ICON_PRICELIST            127
#define ICON_VIEWLOTS             128
#define ICON_VIEWORDLOTS          129
#define ICON_TRFRANLZ             130
#define ICON_SHIPMANLZ            131
#define ICON_PRICEANLZ            132
#define ICON_GOODSREST            133
#define ICON_SELLTRNOVR           134
#define ICON_DEBTSUPPLTRNOVR      135
#define ICON_SUPPLTRNOVR          136
#define ICON_PAYMPLAN             137
#define ICON_FREIGHT              138
#define ICON_GOODSTURNOVER        139
#define ICON_GOODSGRPRLZ          140
#define ICON_GOODSMOV             141
#define ICON_GOODSOPRGROUPING     142
#define ICON_GOODSOPER            143
#define ICON_GETPROGNOSIS         144
#define ICON_PREDICTSALES         145
#define ICON_SSTAT                146
#define ICON_SSTATSUPPLORD        147
#define ICON_PROCESSDL200         148
#define ICON_DEFAULT              149
#define ICON_NEWTASK              150
#define ICON_PERSONS              151
#define ICON_WORLD                152
#define ICON_BILLS                153
#define ICON_DIVISION             154
#define ICON_CASHMCHN             155
#define ICON_CSESS                156
#define ICON_EXIT                 157
#define ICON_ADDTASK              158
#define ICON_ADDPERSON            159
#define ICON_SYSJ                 160
#define ICON_PERSONEVENT          161
#define ICON_GOODS                162
#define ICON_GOODSGROUP           163
#define ICON_STAFFCAL             164
#define ICON_SCALE                165
#define ICON_PERSONCFG            166
#define ICON_EGYPT01              167
#define ICON_VIEWSTAFFCAL         168
#define ICON_TRANSPORTCAR         169
#define ICON_TRANSPORTSHIP        170
#define ICON_STYLOPALMIMPEXP      171
#define ICON_TIMEGRID             172
#define ICON_USERSANDGROUPS       173
#define ICON_CFGBYUSERSANDGROUPS  174
#define ICON_GEAR                 175
#define ICON_WAREHOUSE            176
#define ICON_TAGS_PERSON          177
#define ICON_PRCFREE              178
#define ICON_VIEWCCHECK           179
#define ICON_DBXCHG_RCV           180
#define ICON_DBXCHG_QUEUE         181
#define ICON_ACCOUNT              182
#define ICON_ACCOUNT_BAL          183
#define ICON_ACCOUNT_OUTBAL       184
#define ICON_ACCOUNT_REG          185
#define ICON_ACCOUNT_ALIAS        186
#define ICON_IMPORTBILLS          187
#define ICON_QCERT                188
#define ICON_PERSONEVENT_ADD      189
#define ICON_ARTICLE              190
#define ICON_ADDBILL              191
#define ICON_IMPORTCLIBNK         192
#define ICON_BILLCFG              193
#define ICON_SCARD                194
#define ICON_JOBPOOL              195
#define ICON_QUOTATION            196
#define ICON_BIZSCORE             197 // Значения бизнес-показателей
#define ICON_BASKET               198 // Товарные корзины
#define ICON_ALTGRP               199
#define ICON_FOLDERGRP            200
#define ICON_TEMPALTGRP           201
#define ICON_DYNAMICALTGRP        202
#define ICON_GGROUP               203
#define ICON_SMALLCALENDAR        204
#define ICON_SMALLAMOUNT          205
#define ICON_SMALLALL             206
#define ICON_SMALLEMPLOYER        207
#define ICON_SMALLSTAFF           208
#define ICON_SMALLSTAFFPOST       209
#define ICON_SMALLPERSON          210
#define ICON_DOSALCHARGE          211 // Начисление зарплаты
#define ICON_ANLZSALCHARGE        212 // Анализ начисления зарплаты
#define ICON_BASKET_SMALL         213 // Корзина для отображения в строке состояния //
#define ICON_LABELPRINTER         214
#define ICON_LABELPRINTERFONT     215
#define ICON_TASKREMINDER         216 // Напоминает, что имеются невыполненные задачи
#define ICON_TSESS                217 // Технологические сессии
#define ICON_PERSONRELLIST        218 // Таблица персональных отношений
#define ICON_INFOKIOSK            219
#define ICON_CTBLORD              220
#define ICON_WHZONE               221
#define ICON_WHCOLUMN             222
#define ICON_WHCELL               223
#define ICON_WH                   224
#define ICON_PALLET_16            225
#define ICON_PALLET_32            226
#define ICON_DIVISION_16          227
#define ICON_LOCTRANSF            228
#define ICON_UI_TREELISTBOX       229
#define ICON_UI_LISTBOX           230
#define ICON_UI_GROUPBOX          231
#define ICON_UI_BUTTON            232
#define ICON_UI_INPUT             233
#define ICON_UI_RADIOCLUSTER      234
#define ICON_UI_CHECKCLUSTER      235
#define ICON_UI_STATICTEXT        236
#define ICON_UI_COMBOBOX          237
#define ICON_UI_DIALOG            238
#define ICON_PASTE                239
#define ICON_OPENTEXTFILE         240
#define ICON_WORKBOOK             241
#define ICON_TAGS                 242
#define ICON_TEST                 243
#define ICON_USER16               244
#define ICON_USERGROUP16          245
#define ICON_GEAR16               246
#define ICON_STARYELLOW16         247
#define ICON_PHONE                248
//
// Message groups
//
#define PPMSG_ERROR                 1
#define PPSTR_ERROR                 PPMSG_ERROR // synonym of PPMSG_ERROR
#define PPSTR_DBLIB                 4
#define PPSTR_DBENGINE              6
#define PPSTR_SLIBERR               7
#define PPMSG_WARNING             100
#define PPMSG_FATAL               101
#define PPMSG_CONFIRMATION        102
#define PPSTR_CONFIRMATION        PPMSG_CONFIRMATION // @v11.1.5
#define PPMSG_INFORMATION         103
#define PPSTR_INFORMATION         PPMSG_INFORMATION // @v11.1.5
//
// Another strings classes
//
#define PPSTR_OBJNAMES            110 // Наименования объектов
#define PPSTR_TEXT                111 // Просто строки
#define PPSTR_ACTION              112 // Наименования событий в SysJ
//#define PPSTR_CMT                 113 // Типы кассовых аппратов
#define PPSTR_SYMB                114 // Символьные переменные
// @v7.0.0 #define PPSTR_POVERB              115 // @obsolete(@v6.3.1) Действия персональных операций
//#define PPSTR_BCPT                116 // Типы принтеров штрихкодов
//#define PPSTR_SCLT                117 // Типы электронных весов
#define PPSTR_ABDVCCMD            118 // @v7.1.12 Команды абстрактных устройств
#define PPSTR_GTA                 119 // @v7.3.8 Наименования тарифицируемых событий глобальных аккаунтов
#define PPSTR_IMPEXPCMD           120 // Команды импорта/экспорта через dll
#define PPSTR_USRPROFILEFUNCNAM   121 // Наименования функций для профайлера
#define PPSTR_CURLERR             122 // Текстовые описания ошибок libcurl
#define PPSTR_HASHTOKEN           123 // Специальный мета-символы, используемые системой для обработки данных и команд.
#define PPSTR_HASHTOKEN_C         124 // Специальный мета-символы, используемые системой для обработки данных и команд (case seinsitive!).
#define PPSTR_HASHTOKEN_AGENTPLUS 125 // @v11.5.11 Специальный мета-символы, используемые системой для экспорта данных Agent+ (в частности, VladimirskiyStandard).
#define PPSTR_OBJATTR             126 // @v12.4.10 Наименования общих атрибутов объектов данных
#define PPSTR_CRYSTAL_REPORT      342 // Ошибки Crystal Report

//
// PPSTR_TEXT   Просто текст
//
#define PPTXT_NOTUNEANYOPS                176 // Не настроено ни одной операции по
	#define PPNOTUNE_ORDER    0                //   товарным заказам
	#define PPNOTUNE_INVENT   1                //   инвентаризации
	#define PPNOTUNE_DRAFT    2                //   драфт-документам
#define PPTXT_PERSONATTRIBUTE             177 // Атрибут персоналии
	#define PPPSNATTR_UNDEF          0        //   не определен
	#define PPPSNATTR_PHONEADDR      1        //   телефон/адрес
	#define PPPSNATTR_BNKACCT        2        //   банковский счет
	#define PPPSNATTR_REGISTER       3        //   регистр
	#define PPPSNATTR_TAG            4        //   тег
	#define PPPSNATTR_ALLADDR        5        //   Адреса персоналий
	#define PPPSNATTR_HANGEDADDR     6        //   Зависшие адреса
	#define PPPSNATTR_DLVRADDR       7        //   Адреса доставки
	#define PPPSNATTR_DUPDLVRADDR    8        //   Дублированные адреса доставки (по тексту адреса)
	#define PPPSNATTR_STANDALONEADDR 9        //   Автономные адреса
	#define PPPSNATTR_EMAIL          10       //   Email адрес
#define PPTXT_MAILFILEDS                  186 // Строки полей почтовых сообщений
	#define PPMAILFLD_FROM              0 // From:
	#define PPMAILFLD_TO                1 // To:
	#define PPMAILFLD_SUBJ              2 // Subject:
	#define PPMAILFLD_CONTENTTYPE       3 // Content-Type:
	#define PPMAILFLD_CONTENTDISP       4 // Content-Disposition:
	#define PPMAILFLD_CONTENTCOD        5 // Content-Transfer-Encoding:
	#define PPMAILFLD_MIMEVER           6 // MIME-Version:
	#define PPMAILFLD_BOUNDARY          7 // boundary=
	#define PPMAILFLD_CHARSET           8 // charset=
	#define PPMAILFLD_ATTACHMENT        9 // attachment;
	#define PPMAILFLD_FILENAME         10 // filename=
	#define PPMAILFLD_MESSAGEID        11 // Message-ID:
	#define PPMAILFLD_CONTENTID        12 // Content-ID:
	#define PPMAILFLD_INLINE           13 // inline;
	#define PPMAILFLD_CREATIONDATE     14 // creation-date
	#define PPMAILFLD_MODIFICATIONDATE 15 // modification-date
	#define PPMAILFLD_READDATE         16 // read-date
	#define PPMAILFLD_SIZE             17 // size
	#define PPMAILFLD_DATE             18 // Date:
#define PPTXT_CURRLISTTAGNAMES            199 // Имена тегов в файле списка валют
	#define PPCURRLTAGNAM_CONTENT       0
	#define PPCURRLTAGNAM_CURRLIST      1
	#define PPCURRLTAGNAM_ITEM          2
	#define PPCURRLTAGNAM_ID            3
	#define PPCURRLTAGNAM_NAME          4
	#define PPCURRLTAGNAM_DIGITCODE     5
	#define PPCURRLTAGNAM_MNEMONICCODE  6
#define PPTXT_XMLFILNAMES                 200 // Имена xml-файлов с данными
	#define PPXMLFILNAM_CURRLIST        0
	#define PPXMLFILNAM_BANKLIST        1
#define PPTXT_INETCONNAGENTS              201 // Имена приложений для доступа в интернет
	#define PPICONNAGENT_IEXPLORER      0
	#define PPICONNAGENT_NETSCAPE       1
	#define PPICONNAGENT_OPERA          2
#define PPTXT_GOODS_ASSC_NAME             205 // Наименование типа ассоциации товара
	#define PPGDSASSC_ALTGOODSGRP       0  // Альтернативная группа товаров
	#define PPGDSASSC_GOODSSTRUC        1  // Структура товара
	#define PPGDSASSC_GENGOODS          2  // Обобщенный товар
#define PPTXT_CLIENTBANKPARAMS            206 // Параметры экспорта в систему клиент-банк
	#define PPCLBNKPAR_FILENAME         0  // Имя файла экспорта
	#define PPCLBNKPAR_CODEPAGE         1  // Кодовая страница
	#define PPCLBNKPAR_FORMAT           2  // Формат экспорта
	#define PPCLBNKPAR_BANKCODE         3  // БИК банка (для проверки банка назначения документа)
	#define PPCLBNKPAR_IMPEXP           4  // Назначение записи
	#define PPCLBNKPAR_HDR              5  // заголовок файла
	#define PPCLBNKPAR_END              6  // концовка файла
	#define PPCLBNKPAR_PARVALDIV        7  // разделитель поля и его значения //
	#define PPCLBNKPAR_FLAGS            8  // флаги
	#define PPCLBNKPAR_PAYMMETHODTRANSL 9  // Трансляция поля "вид платежа" из papyrus в client bank
#define PPTXT_OBJACCEPTED                 220 // Сообщения о приеме объекта в базу данных
	#define PPOBJACCP_OBJCREATED         0 // Объект создан
	#define PPOBJACCP_OBJUPDATED         1 // Объект изменен
#define PPTXT_BRWCOLORSSCHEMASNAMES       221 // имена и идентификаторы цветовых схем броузеров
	#define PPBRWCOLORSSCHEMASNAMES_DEFAULTSHEMA 0
	#define PPBRWCOLORSSCHEMASNAMES_SCHEMA1      1
	#define PPBRWCOLORSCSHEMASNAMES_SCHEMA2      2
#define PPTXT_ATOL_CMDSTRINGS             226 // Командные строки для АТОЛ
	#define PPATOLCS_INITFILE            0
	#define PPATOLCS_DELETEALL           1
	#define PPATOLCS_REPLACEQTTY         2
	#define PPATOLCS_REPLACEQTTYWOSALE   3
	#define PPATOLCS_TRANSBYDTTM         4
	#define PPATOLCS_DELETEALLSCHEMES    5
	#define PPATOLCS_ADDINNERSCHEMES     6
	#define PPATOLCS_DELETEALLDSCNTS     7
	#define PPATOLCS_ADDSCARDDSCNTS      8
#define PPTXT_ATOLCARD_TBL_NAMES          228 // Наименования таблиц в ATOL-CARD
	#define PPACTN_CARDS                 0
	#define PPACTN_CATEGORIES            1
	#define PPACTN_CARDS_N_CATS          2
#define PPTXT_CLIENTSERVER_WORDS		  229
	#define CS_STATUS_BAD                0
	#define CS_STATUS_OK                 1
	#define CS_STATUS_NO                 2
	#define CS_STATUS_BYE                3
	#define CS_COMMAND_LOGIN             4
	#define CS_COMMAND_CAPABILITY        5
	#define CS_COMMAND_PROCESSOR         6
	#define CS_COMMAND_VIEW              7
	#define CS_COMMAND_BYE               3
	#define CS_RESPONSE_OK               1
	#define CS_RESPONSE_PERCENT          8
	#define CS_RESPONSE_MSG              9
	#define CS_RESPONSE_CAPABILITY       5
	#define CS_COMMAND_BREAK            10
#define PPTXT_DATATYPES_RUSSIAN			  235
	#define DATATYPES_RUSSIAN_STRING     1
	#define DATATYPES_RUSSIAN_NUMBER     2
	#define DATATYPES_RUSSIAN_DATE       3
#define PPTXT_FILETYPES					  236
	#define FILETYPES_DBF                1
	#define FILETYPES_CSV                2
	#define FILETYPES_TXT                3
#define PPTXT_SIGN                        237
	#define SIGN_LT                      1
	#define SIGN_GT                      2
	#define SIGN_NO_MATTER               3
#define PPTXT_CUSTDISP_WORDS              277 // Слова и короткие фразы для дисплея покупателя //
	#define PPCDY_WELCOME                0     // Добро пожаловать
	#define PPCDY_CASH_NOWORK            1     // Касса не работает
	#define PPCDY_DISCOUNT               2     // Скидка
	#define PPCDY_TOTAL                  3     // Итого
	#define PPCDY_CASH                   4     // Наличными
	#define PPCDY_CHANGE                 5     // Сдача
	#define PPCDY_PRESENT                6     // Подарок
#define PPTXT_TODOCTPARAMS                336 // Список показателей кросстаба для задач
	#define PPTDCTPAR_UNCOMPLTASKS      1
	#define PPTDCTPAR_COMPLTASKRATIO    2
	#define PPTDCTPAR_WROFBILLPRCT      3
#define PPTXT_DBMAINTAINLOG               414
	#define DBMAINTAINLOG_START                    0 // Обслуживание базы данных
	#define DBMAINTAINLOG_END                      1 // Обслуживание базы данных закончена
	#define DBMAINTAINLOG_STARTSYSJ                2 // Обслуживание системного журнала
	#define DBMAINTAINLOG_ENDSYSJ                  3 // Обслуживание системного журнала закончено
	#define DBMAINTAINLOG_STARTXBILL               4 // Обслуживание истории документов
	#define DBMAINTAINLOG_ENDXBILL                 5 // Обслуживание истории документов закончено
	#define DBMAINTAINLOG_STARTDLS                 6 // Обслуживание истории загрузки оборудовани
	#define DBMAINTAINLOG_ENDDLS                   7 // Обслуживание истории загрузки оборудования закончено
	#define DBMAINTAINLOG_STARTMRP                 8 // Обслуживание MRP таблиц
	#define DBMAINTAINLOG_ENDMRP                   9 // Обслуживание MRP таблиц закончено
	#define DBMAINTAINLOG_STARTRMVTEMPALTGGRP     10 // Удаление временных альтернативных товарных групп
	#define DBMAINTAINLOG_ENDRMVTEMPALTGGRP       11 // Удаление временных альтернативных товарных групп закончено
#define PPTXT_CHKPAN_INFO                 418 // Информационное сообщение в кассовой панели
	#define PPCHKPAN_RETCHECK                      0 // Возврат
	#define PPCHKPAN_WAITINGONSCARD                1 // Введите номер карты
	#define PPCHKPAN_SELBYPRICE                    2 // Выбор товара по цене
	#define PPCHKPAN_SELGROUP                      3 // Выбор группы
	#define PPCHKPAN_SELLING                       4 // Продажа
	#define PPCHKPAN_SESSISCLOSED                  5 // Кассовая сессия закрыта
	#define PPCHKPAN_CTBLORDER                     6 // Заказ стола
	#define PPCHKPAN_CTBLORDERCANCELED             7 // Отмененный заказ стола
	#define PPCHKPAN_CORRECTION                    8 // @v12.2.9 Чек коррекции
#define PPTXT_LOGSNAMES                   446 // Файлы логов и их названия
	#define PPLOGNAM_ERROR              0
	#define PPLOGNAM_ERRMSG             1
	#define PPLOGNAM_INFO               2
	#define PPLOGNAM_BACKUP             3
	#define PPLOGNAM_PSALES             4
	#define PPLOGNAM_SCALE              5
	#define PPLOGNAM_DBXRECV            6
	#define PPLOGNAM_BALANCE            7
	#define PPLOGNAM_LOTERR             8
	#define PPLOGNAM_LOTSUPPL           9
	#define PPLOGNAM_CCHECKERR         10
	#define PPLOGNAM_REPAIR            11
	#define PPLOGNAM_ACCTURN           12
	#define PPLOGNAM_ABSLOTS           13
	#define PPLOGNAM_ABSGOODS          14
	#define PPLOGNAM_ACS               15
	#define PPLOGNAM_PAYMLINK          16
	#define PPLOGNAM_PROFILE           17
	#define PPLOGNAM_SHTRIH            18
	#define PPLOGNAM_VERHIST           19
	#define PPLOGNAM_DEBUG             20
	#define PPLOGNAM_CBADRECK          21
	#define PPLOGNAM_CCHECK            22
#define PPTXT_FONTFACE                    455 // Наименования начертаний шрифтов
	#define PPFONT_MSSANSSERIF          0
	#define PPFONT_IMPACT               1
	#define PPFONT_TIMESNEWROMAN        2
	#define PPFONT_ARIAL                3
	#define PPFONT_ARIALCYR             4
#define PPTXT_DL200_NAMEVARS              565 // Описание переменных для извлечения некоторой информации DL200
	#define PPDL200_NAMEVAR_MAINORGNAME 0
	#define PPDL200_NAMEVAR_MAINORGADDR 1
	#define PPDL200_NAMEVAR_MAINORGINN  2
	#define PPDL200_NAMEVAR_MAINORGOKPO 3
	#define PPDL200_NAMEVAR_DIRECTOR    4
	#define PPDL200_NAMEVAR_ACCOUNTANT  5
	#define PPDL200_NAMEVAR_DBNAME      6
	#define PPDL200_NAMEVAR_DBSYMB      7
#define PPTXT_FILE_OR_PATH_SELECTION      579 // Выбор файла/пути
	#define PPFOPS_FILE                 0
	#define PPFOPS_PATH                 1
#define PPTXT_BILLUNCOMPATREASON          619 // Причины несовместимости документов
	#define BILLUNCOMPATREASON_OBJ              0
	#define BILLUNCOMPATREASON_OBJ2             1
	#define BILLUNCOMPATREASON_OP               2
	#define BILLUNCOMPATREASON_LOC              3
	#define BILLUNCOMPATREASON_CUR              4
	#define BILLUNCOMPATREASON_LINK             5
#define PPTXT_SETRETAIL_PARAM             626 // Параметры в Сет-Ретейл для импорта в Папирус
	#define SETR_PARAM_SEPARATEPATH      0
	#define SETR_PARAM_REPORTS           1
	#define SETR_PARAM_REPGANG           2
	#define SETR_PARAM_REPCHEQUEHEAD     3
	#define SETR_PARAM_REPCHEQUEPOS      4
	#define SETR_PARAM_REPCHEQUEDISC     5
	#define SETR_PARAM_SEPARATE          6
#define PPTXT_SERVERTAGS                  627 // Теги при ответе сервера на TCP/IP запрос
	#define SERVERTAG_ID                 0
	#define SERVERTAG_PARENTID           1
	#define SERVERTAG_TXT                2
#define PPTXT_GOODSPARAMS                 628
	#define PPGOODSPAR_SUBCODE           0
	#define PPGOODSPAR_SUPPL             1
	#define PPGOODSPAR_DEFUNIT           2
	#define PPGOODSPAR_PHUNIT            3
	#define PPGOODSPAR_DEFPARENT         4
	#define PPGOODSPAR_RCPTOP            5
	#define PPGOODSPAR_FLAGS             6
	#define PPGOODSPAR_LOC               7
	#define PPGOODSPAR_MATRIXACTION      8
	#define PPGOODSPAR_ACCSHEET          9
#define PPTXT_UHTTTAGS_GOODSPRICE         629
	#define UHTTTAG_GOODSPRICE_ID        0
	#define UHTTTAG_GOODSPRICE_SELLERID  1
	#define UHTTTAG_GOODSPRICE_BUYERID   2
	#define UHTTTAG_GOODSPRICE_CURID     3
	#define UHTTTAG_GOODSPRICE_DT        4
	#define UHTTTAG_GOODSPRICE_TM        5
	#define UHTTTAG_GOODSPRICE_VAL       6
	#define UHTTTAG_GOODSPRICE_MEMO      7
#define PPTXT_UHTTTAGS_GOODSCODE          630
	#define UHTTTAG_GOODSCODE_ID         0
	#define UHTTTAG_GOODSCODE_OWNERID    1
	#define UHTTTAG_GOODSCODE_CODE       2
#define PPTXT_SHTRIHMFRK_CMDSTRINGS             631 // Командные строки для Штрих-М-ФР-К
	#define PPSHTRIHMFRKCS_INITFILE            0
	#define PPSHTRIHMFRKCS_CLEAR               1
	#define PPSHTRIHMFRKCS_REPLACEQTTY         2
	#define PPSHTRIHMFRKCS_ADDQTTY             3
#define PPTXT_FASTPRD                           632 // Наименование элементов для быстрого выбора периода
	#define PPFASTPRD_TODAY                    0
	#define PPFASTPRD_YESTERDAY                1
	#define PPFASTPRD_LAST7DAYS                2
	#define PPFASTPRD_LASTWEEK                 3
	#define PPFASTPRD_LASTWORKWEEK             4
	#define PPFASTPRD_THISMONTH                5
	#define PPFASTPRD_LASTMONTH                6
#define PPTXT_IMPEXPPARAM_BILH                  633 // Дополнительные параметры импорта/экспорта заголовков документов
	#define IMPEXPPARAM_BILH_IMPOPSYMB      0
	#define IMPEXPPARAM_BILH_FLAGS          1
	#define IMPEXPPARAM_BILH_SRCHCODE1      2
	#define IMPEXPPARAM_BILH_SRCHCODE2      3
	#define IMPEXPPARAM_BILH_PREDEFFMT      4
	#define IMPEXPPARAM_BILH_FIXTAG         5 // @v11.5.6
	#define IMPEXPPARAM_BILH_OUTERFORMATVER 6 // @v11.6.5
#define PPTXT_BALTIKA_DOCTYPES                  636
	#define BALTIKA_DOCTYPES_EXPEND      0
	#define BALTIKA_DOCTYPES_RECEIPT     1
	#define BALTIKA_DOCTYPES_SUPPLRETURN 2
	#define BALTIKA_DOCTYPES_RETURN      3
	#define BALTIKA_DOCTYPES_LOSSES      4
	#define BALTIKA_DOCTYPES_INVENTORY   5
	#define BALTIKA_DOCTYPES_MOVINGFROM  6
	#define BALTIKA_DOCTYPES_MOVINGTO    7
#define PPTXT_SCARDPARAMS                   644 // Параметры импорта/экспорта дисконтных карт
	#define PPSCARDPAR_DEFSERIESSYMB     0
	#define PPSCARDPAR_OWNERREGTYPESYMB  1
#define PPTXT_LOTPARAMS                     645 // Параметры импорта/экспорта лотов
	#define PPLOTPAR_FLAGS               0
	#define PPLOTPAR_UHTTGOODSCODEAR     1
#define PPTXT_SMS_FIELDS					650	// Наименования полей для подставновки в смс-сообщение
	#define SMS_FIELDS_PERSNAME			0	// Имя адресата
	#define SMS_FIELDS_EXTPERSNAME      1   // Расширенное им
	#define SMS_FIELDS_MAINORGNAME		2	// Название главной организации
	#define SMS_FIELDS_MAINORGPHONE		3	// Телефон главной организации
	#define SMS_FIELDS_MAINORGADDR		4	// Адрес главной организации
#define PPTXT_FUNCIDS                       653
	#define PPTXT_FUNCID_BILLTURNPACK   1
#define PPTXT_UFP_AVG_IDS                   654
	#define PPTXT_UFP_AVG_FUNC                1
	#define PPTXT_UFP_AVG_DB_FUNC             2
	#define PPTXT_UFP_AVG_PPYVER_FUNC         3
	#define PPTXT_UFP_AVG_DB_PPYVER_FUNC      4
	#define PPTXT_UFP_AVG_FUNC_VER            5
	#define PPTXT_UFP_AVG_DB_FUNC_VER         6
	#define PPTXT_UFP_AVG_PPYVER_FUNC_VER     7
	#define PPTXT_UFP_AVG_DB_PPYVER_FUNC_VER  8
#define PPTXT_CSESSCPARAMS                  656
	#define CSESSCPARAMS_FLAGS          0
	#define CSESSCPARAMS_CSESSTAG       1
	#define CSESSCPARAMS_CCHECKTAG      2
	#define CSESSCPARAMS_CCLINETAG      3
	#define CSESSCPARAMS_CCPAYMTAG      4
#define PPTXT_BIZSCORE_PRIMITIVES           657
	#define BIZSCORE_PRIMITIVES_METAVAR       0
	#define BIZSCORE_PRIMITIVES_BILL          1
	#define BIZSCORE_PRIMITIVES_PAYM          2
	#define BIZSCORE_PRIMITIVES_CCHECK        3
	#define BIZSCORE_PRIMITIVES_GOODSREST     4
	#define BIZSCORE_PRIMITIVES_PERSONEVENT   5
	#define BIZSCORE_PRIMITIVES_DEBT          6
	#define BIZSCORE_PRIMITIVES_BIZSCORE      7
#define PPTXT_QUOTPARAMS                    658
	#define PPQUOTPAR_QUOTKIND                0
	#define PPQUOTPAR_CURRENCY                1
	#define PPQUOTPAR_ARTICLE                 2
	#define PPQUOTPAR_LOC                     3
	#define PPQUOTPAR_FLAGS                   4
	#define PPQUOTPAR_QUOTCAT                 5 // @v11.4.2
#define PPTXT_IMPEXPPARAM_CC                659 // @v11.8.5 Дополнительные параметры импорта/экспорта кассовых чеков
	#define IMPEXPPARAM_CC_PREDEFFMT          0 // Значение такое же, как и для параметров импорта/экспорта документов!
	#define IMPEXPPARAM_CC_FLAGS              1 //
	#define IMPEXPPARAM_CC_POSNODE            2 //
// LastText
//
// Intrinsic calculator functions
//
#define PPCFUNC_PAPER_ROLLOUTERLAYER 1001
#define PPCFUNC_PAPER_ROLLUSAGEPART  1002
//
// Installation (__SERG__)
//
#define PPINSTPATH_FIRST            2L
#define PPINSTPATH_DRIVE            (PPPATH_FIRST+0)  // Дисковод
#define PPINSTPATH_ROOT             (PPPATH_FIRST+1)  // Корневой каталог

#define DLG_INSTMODE              500
	#define CTL_INSTMODE_OLD_VERSION     (1 + WINDOWS_ID_BIAS)
	#define CTL_INSTMODE_NEW_VERSION     (2 + WINDOWS_ID_BIAS)
	#define CTL_INSTMODE_MODE            (3 + WINDOWS_ID_BIAS)
#define DLG_INSTPATH              501 // Пути поиска файлов
	#define CTL_INSTPATH_DRV             (1 + WINDOWS_ID_BIAS) // при инсталляции
	#define CTL_INSTPATH_ROOT            (2 + WINDOWS_ID_BIAS)
	#define CTL_INSTPATH_NETFLAG         (3 + WINDOWS_ID_BIAS)

// force svn line

#endif /* __PPDEFS_H */
