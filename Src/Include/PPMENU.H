// PPMENU.H
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Константы команд меню проекта Papyrus
//
#ifndef __PPMENU_H
#define __PPMENU_H

#ifndef cmQuit
	#define cmQuit                 1
#endif
#define cmCountry                      101 // Государства
#define cmAnotherPerson                106 // Obsolete
#define cmPerson                       106 //
#define cmNewConfig                    107 //
#define cmEditConfig                   108 // Главная конфигурация //
#define cmGroups_n_Users               109 // Группы и пользователи
#define cmUnit                         110 // Единицы измерения //
#define cmCityStatus                   111 // Статусы населенных пунктов
#define cmPersonKind                   112 // Виды персоналий
#define cmPersonStatus                 113 // Юридические статусы персоналий
#define cmBnkAccType                   114 // Типы банковских счетов
#define cmAccSheet                     115 // Таблицы аналитического учета
#define cmAccArticle                   116 // Статьи аналитического учета
#define cmAccPlan                      117 // План счетов
#define cmGoodsGroup                   118 // Группы товаров
#define cmGoods                        119 // Товары
#define cmLocation                     120 // Склады
#define cmAccTurnList                  121 // Опись бух. проводок
#define cmAccTurn                      122 // Общая бух. проводка
#define cmAccAnalyze                   123 // Анализ счета
#define cmBalance                      124 // Балансы (оборотная ведомость)
#define cmAmountType                   125 // Типы сумм документов
#define cmOprType                      126 // Типы операций
#define cmOprKind                      127 // Виды операций
#define cmScale                        128 // Весы
#define cmRegisterType                 129 // Тип регистрационных документов
#define cmELinkKind                    130 // Виды электронных адресов
#define cmQuotKind                     131 // Виды котировок товаров
#define cmObjTag                       132 // Теги объектов
#define cmSelectLocation               135 // Выбор позиции
#define cmGoodsBillList                136 // Просмотр товарных документов
#define cmViewLots                     137 // Просмотр лотов
#define cmViewStatus                   138 // Просмотр текущего статуса
#define cmViewGoodsCard                139 // Карточка товара (Obsolete from @v2.5.2 instead use cmTrfrAnalyze)
#define cmGoodsTurnover                140 // Отчет о товарообороте
#define cmGoodsRest                    142 // Остатки товаров
#define cmCorrectTrfrRvrs              143 // Корректировка зеркальных проводок
#define cmConvert1                     144 //
#define cmConvert2                     145 //
#define cmConvert3                     146 //
#define cmCorrectBillPaym              147 // Корректировка сумм оплат по документу
#define cmSupplTrnovr                  148 // Обороты по поставщикам
#define cmSellTrnovr                   149 // Обороты по покупателям
#define cmGoodsGrpRlz                  150 // (obsolete since @v2.9.11) Реализация по группам товаров
#define cmGoodsTaxAnalyze              150 // Налоговый анализ товарооборота
#define cmGoodsMov                     151 // Движение товаров
#define cmGoodsOprGrouping             152 // Группировка товарных операций
#define cmCorrectAccTurnRest           153 // Корректировка остатков бух. проводок
#define cmCorrectLot                   154 // Корректировка лотов
#define cmCashNode                     157 // PPOBJ_CASHNODE
#define cmRecalcBillTurns              158 // Пересчет проводок по документам
#define cmGoodsOper                    159 // Товарная ведомость по операции
#define cmGoodsOrder                   160 // Заказы
#define cmAction                       161 // PPOBJ_ACTION
#define cmViewSysJ                     162 // Просмотр системного журнала
#define cmCalc                         164 // Калькулятор
#define cmUnifyGoodsPrice              165 // Унификация цен товаров
#define cmPriceList                    166 // Прайс-лист
#define cmCommConfig                   167 // Общие настройки
#define cmViewQCert                    168 // Просмотр сертификатов
#define cmViewCntGrpList               169 // Группировка операций по счетчику
#define cmCashBook                     170 // Кассовая книга
#if 0 // {
#define cmExportGoodsRest       171 // Экспорт товарных остатков со штрих-кодами
#define cmOpenCashDay           172 // Открыть кассовую сессию
#define cmCloseCashDay          156 // Закрыть кассовую сессию
#define cmCashNodePanel         173 // Панель операций с кассовым узлом
#define cmOperateCashDay        174 // Операции в кассовой сессии
	#define cmCashBillList          155 // Документы на кассовом узле
	#define cmUpdateCashDay         175 // Обновление кассовой сессии
#define cmACSViewExcess         180
#endif // 0 }
#define cmCashNodePanel                171
//
// Команды кассовых сессий
// (CS - CashSession, ACS - Async Cash Session, SCS - Sync Cash Session)
//
#define cmCSOpen                       172
#define cmCSClose                      173
#define cmACSUpdate                    174
#define cmCSViewCheckList              175
#define cmACSViewExcess                176 // @obsolete use cmViewCSessDeficit
#define cmViewCSessDeficit             176 // == cmACSViewExcess
#define cmSCSLock                      177
#define cmSCSUnlock                    178
#define cmSCSXReport                   179 // X-отчет (SyncCashSess)
#define cmACSGetChecks                 180 // Получить чеки (Async Cash Session)
#define cmCashBillList                 175 // Obsolete: use cmCSViewCheckList
#define cmCloseCashDay                 173 // Obsolete: use cmCSClose
#define cmBarCodeStruc                 181 // Структура внутренних штрих-кодов
#define cmCorrectBalance               182 // Корректировка балансов
#define cmCorrectCurRest               183 // Корректировка таблицы CurRest
#define cmRemoveEmptyAcctRels          184 // Удаление неиспользуемых AcctRel
#define cmDBDiv                        185 // Справочник разделов БД
#define cmSelfSync                     186 // Синхронизация БД для разбиения на два раздела
#define cmSendBills                    187 // Создать пакет для посылки документов
#define cmReceivePackets               188 // Принять входящие пакеты
#define cmReplacePerson                189 // Объединить персоналии
#define cmReplaceGoods                 190 // Объединить товары
#define cmGoodsCfg                     191 // Конфигурация справочника товаров
#define cmCorrectLotsCloseTags         192 // Корректировка признаков закрытия лотов
#define cmVATBook                      193 // Книги продаж/покупок
#define cmVATBookSellCfg               194 // Конфигурация книг продаж
#define cmVATBookBuyCfg                195 // Конфигурация книг покупок
#define cmExpMonarchCfg                196 // Конфигурация экспорта в устройство Monarch
#define cmCorrectLotSuppl              197 // Корректировка поставщиков по лотам
#define cmCreateReservedObjs           198 // Создание зарезервированных объектов
#define cmGoodsType                    199 // Типы товаров
#define cmConvert197                   200 // Конвертация прав доступа и путей @v1.9.7
#define cmPrinterCfg                   202 // Конфигурация принтера по умолчанию
#define cmTransmitQCert                220 // Временная команда @v1.9.10
#define cmCorrectZeroQCertRefs         221 // Обнуление висячих ссылок на сертификаты в ReceiptTbl
#define cmResetOpCounters              222 // Сброс счетчиков всех операций
#define cmCheckBillAmounts             223 // Проверка сумм по документам
#define cmFormula                      224 // Формулы
#define cmConvert200                   225 // Конвертация @v2.0.0
#define cmDefaultCliAgt                226 // Соглашение с клиентами по умолчанию
#define cmAccturnBillList              227 // Просмотр бухгалтерских документов
#define cmCheckPane                    228 // Панель ввода чековых операций
#define cmViewCCheck                   229 // Броузер чеков
#define cmConvert2110                  230 // Конвертация @v2.1.10
#define cmScalePrepareData             231 // Подготовка данных для весов
#define cmScaleTransmitData            232 // Передача данных на весы
#define cmConvert221                   233 // Конвертация @v2.2.1 (BarcodeTbl)
#define cmConvert224                   234 // Конвертация @v2.2.4 (Location, Person)
#define cmInventory                    235 // Инвентаризаци
#define cmConvert229                   236 // Конвертация @v2.2.9 (Receipt - Expiry)
#define cmUpdateQuots                  237 // Массовое изменение котировок
#define cmPersonOpKind                 238 // Виды персональных операций @v2.3.4
#define cmAbout                        239 // О программе...
#define cmPersonEvent                  240 // Персональные события //
#define cmDebtSupplTrnovr              244 // Долги по поставщикам
#define cmDBXchgCfg                    245 // Конфигурация обмена данными между разделами
#define cmRecover_v242                 246 // Корректировка проблемы @v2.4.2
#define cmDivision                     247 // Подразделени
#define cmAutoCharge                   248 // Автоматическое начисление ренты
#define cmTrfrAnalyze                  249 // Анализ товарных операций
#define cmStaffList                    250 // Штатное расписание
#define cmConvert253                   251 // Конвертация @v2.5.3 (VATBook - 2 indexes added)
#define cmStaff                        252 // Кадры
#define cmNamedGoodsStruc              253 // Именованные структуры товаров
#define cmMakeCRptFiles                254 //
#define cmViewOrdLots                  255 //
#define cmConvert270                   256 //
#define cmGoodsTax                     257 // Товарные налоговые группы
#define cmCorrectAbsenceLots           258 // Восстановление отсутствующих (из-за сбоя) лотов
#define cmBarcodePrinter               259 // Принтеры штрихкодов
#define cmShipmAnalyze                 260 // Анализ отгрузки
#define cmCurrency                     261 // Валюты
#define cmCurRateType                  262 // Типы валютных курсов
#define cmViewCurRates                 263 // Валютные курсы
#define cmConvert300                   265 // @v3.0.0 Конвертация //
#define cmUpdatePassword               266 // Изменение собственного пароля //
#define cmPoolBillList                 267 // Пулы документов
#define cmCorrectAccrelArRefs          268 // Корректировка несоответствий
	// между номерами аналитических статей и номерами этих же статей в таблице AcctRel. Эта проблема характерна
	// для баз данных, эксплуатировавшихся версиями ниже 2.9.11. В этих версиях отсутствовала процедура
	// восстановления данного соответствия при ручном изменении номера аналитической статьи.
	//
#define cmTransmitGoods                269 // Передача товаров (с обновлением)
#define cmShowTree                     270 // Деревянное окно
#define cmOpCounter                    271 // Счетчики операций
#define cmConvert301                   272 // Конвертация //
#define cmReplaceGoodsTaxGrp           273 // Объединение налоговых групп
#define cmDatabaseCutting              274 // Усечение базы данных
#define cmCurReval                     275 // Валютная переоценка
#define cmSaveBalance                  276 // Сохранить баланс
#define cmRestoreBalance               277 // Восстановить баланс
#define cmShowToolbar                  278 // Тулбарчик
#define cmViewCSess                    279 // Кассовые сессии
#define cmConvert329                   280 // Конвертация (sj.btr)
#define cmViewObjSync                  281 // Просмотр таблицы синхронизации объектов
#define cmTransmitModifications        282 // Передача изменений
#define cmGoodsClass                   283 // Отраслевые классы товаров
#define cmCorrectLotTaxGrp             284 // Корректировка налоговых групп в порожденных лотах
#define cmWoodBreed                    285 // Породы древесины (отраслевой справочник)
#define cmPckgType                     286 // Типы товарных пакетов
#define cmTransportCar                 287 // Автотранспорт
#define cmTransportShip                288 // Торговые суда
#define cmCorrectPckgCloseTags         291 // Корректировка признаков закрытия пакетов
#define cmGoodsBasket                  292 // Корзина товаров
#define cmPriceListCfg                 293 // Конфигурация прайс-листа
#define cmImportGoodsGrp               301 // Импорт справочника групп товаров
#define cmImportGoods                  302 // Импорт справочника товаров
#define cmImportPerson                 303 // Импорт справочника персоналий (старая версия)
#define cmImportQuot                   304 // Импорт котировок (старая версия)
#define cmChainDatabase                305 // Стяжка базы данных
#define cmConvert3512                  306 // Конвертация (Register)
#define cmBillCfg                      307 // Конфигурация документов
#define cmDraftBillList                308 // Драфт-документы
#define cmDeleteTmpFiles               309 // Удаление временных файлов
#define cmMainOrg2                     310 // Форма "Главная организация"
#define cmCreateNewFiles               311 // Создание Базы данных
#define cmBht                          312 // Терминалы сбора данных (BHT)
#define cmBhtConfig                    313 // Конфигурация обмена данными с BHT
#define cmBhtTransmit                  314 // Передача данных в BHT
#define cmBhtReceive                   315 // Прием данных из BHT
#define cmBhtTransmitProgram           316 // @TEMP Загрузка программы на BHT
#define cmUpdateLicense                318 // Обновление лицензии на использование Papyrus'a
#define cmRegistration                 319 // Регистраци
#define cmSCardSeries                  320 // Серии пластиковых карт
#define cmSCard                        321 // Пластиковые карты
#define cmConvert372                   322 // Конвертация кассовых чеков
#define cmStyloPalmExport              323 // Создание данных для загрузки в StyloPalm
#define cmStyloPalmImpExp              323 // Импорт/Экспорт для StyloPalm
#define cmStyloPalm                    324 // Palm
#define cmDraftWrOff                   325 // Типы списания драфт-документов
#define cmWriteOffDrafts               326 // Списание драфт-документов по типу
#define cmConvert380                   327 // Конвертация прайс-листа
#define cmAssetWrOffGrp                328 // Группа списания активов
#define cmRevalAssets                  329 // Списание активов
#define cmPredictSales                 330 // Продажи товаров
#define cmFillPredictSalesTbl          331 // Заполнение таблицы продаж
#define cmPredictCfg                   332 // Конфигурация прогноза продаж
#define cmGetPrognosis                 333 // Вычисление прогноза продаж по некоторому фильтру
#define cmPaymPlan                     334 // План платежей
#define cmAdvBillKind                  335 // Виды отчетных документов
#define cmActiveUsersList              336 // Список активных на данный момент ползователей
#define cmImportOrders                 337 // Прием заказов Albatros // @obsolete
#define cmAlbatrosCfg                  338 // Конфигурация проекта Альбатрос
#define cmProcessDL200                 339 // Обработка внешнего отчета DL200
#define cmAccounts                     340 // Бухгалтерские счета (любые по фильтру)
#define cmAccOutbal                    341 // Список забалансовых счетов
#define cmAccRegister                  342 // Список бухгалтерских регистров
#define cmAccAggr                      343 // Список агрегирующих счетов
#define cmConvert400                   344 // Конвертация таблицы Account
#define cmCorrectAccturn               345 // Проверка и восстановление бухгалтерских проводок
#define cmReceiveCharry                346 // Прием объектов по технологии Charry
#define cmConvert31102                 347 // Конвертация таблиц VATBook, ObjTag (меняются только индексы)
#define cmCorrectAbsenceGoods          348 // Восстановление отсутствующих (из-за сбоя) товаров
#define cmAnalyzeObjSyncCmp            349 // Анализ синхронизации объектов
#define cmConvert31110                 350 // Конвертация таблицы CCheck (меняются только индексы)
#define cmRecoverAbsenceBills          351 // Восстановление потерянных шапок документов
#define cmViewAsset                    352 // Отчет по основным средствам
#define cmSStat                        353 // Статистика по продажам
#define cmCalcGoodsSaldo               354 // Расчет сальдо по товарам
#define cmImportCurrList               356 // Загрузка списка валют из интернета
#define cmInetConnCfg                  357 // Конфигурация соединения с интернетом
#define cmUserInterfaceCfg             358 // Настройки интерфейса
#define cmFreight                      359 // Информация о фрахте по документам
#define cmProject                      360 // Проекты
#define cmViewQuots                    361 // Котировки товаров
#define cmSimpleLedgerCfg              362 // Настройки книги доходов/расходов
#define cmMailAccount                  363 // Учетные записи электронной почты
#define cmAddressBook                  364 // Адресная книга
#define cmMailCharryFiles              365 // Передача файлов charry по электронной почте
#define cmImportBanks                  366 // Импорт банков
#define cmConvert4402                  367 // Конвертация таблицы VATBook
#define cmConvert4405                  369 // Конвертация таблиц Goods2, PriceLine, QualityCert
#define cmBrand                        370 // Торговые марки
#define cmAsset                        371 // Справочник основных средств
#define cmFastGrp_n_Users              372 // Быстрое редактирование групп и пользователей (Оч. удобно)
#define cmEditHolidays                 373 // Редактирование списка выходных для прогнозирования //
#define cmPersonRelType                374 // Типы отношений между персоналиями
#define cmImportSpecSeries             375 // Импорт специальных серий
#define cmRemoveBadReckons             376 // Удаление висячих и неправильных зачитывающих оплат
#define cmEquipCfg                     377 // Конфигурация оборудовани
#define cmCliBnkFmtCfg			       378 // Настройка форматов импорта/экспорта в клиент-банк
#define cmCliBnkAsCfg                  379 // Настройка ассоциаций импорта из клиент-банка
#define cmCliBnkImport                 380 // Импорт банковской выписки из клиент-банка
#define cmSStatSupplOrder              381 // Статистика продаж с данными для заказа поставщику
#define cmPersonCategory               382 // Категории персоналий
#define cmAccAlias                     383 // Алиасы счетов
#define cmMrpTab                       384 // MRP-таблицы
#define cmPrjTask                      385 // Задачи
#define cmStyloPalmCfg                 386 // Конфигурация обмена со StyloPalm
#define cmProcessor                    387 // Процессоры
#define cmTech                         388 // Технологии
#define cmProjectCfg                   389 // Конфигурация проектов
#define cmTSession                     390 // Технологические сессии
#define cmConvert4707                  391 // Конвертация таблицы Receipt
#define cmTSessAnlz                    392 // Анализ технологических сессий
#define cmGoods2LocAssoc               393 // Список ассоциаций товар-склад
#define cmDraftCreateRules             394 // Правила создания драфтов
#define cmImportCountry                395 // Импорт гостударств
#define cmImportSCard                  396 // Импорт дисконтных карт (старый вариант до v7.3.12)
#define cmDefaultSupplAgt              397 // Соглашение с поставщиками по умолчанию
#define cmImportSuppl                  398 // Импорт поставщиков
#define cmTSessWrOffOrder              399 // Порядок списания технологических сессий
#define cmCorrectGoodsGroups           400 // Корректировка товарных групп
#define cmViewDLStat                   401 // Журнал загрузки оборудовани
#define cmConvert4805                  402 // Конвертация таблицы DlsObj
#define cmTSessionCfg                  403 // Конфигурация технологических сессий
#define cmSetSCardsByRule              404 // Установка скидки на карты по правилам
#define cmGoods2SupplAssoc             405 // Ассоциация товар (товарная группа) - поставщик
#define cmGoodsRestTest                406 // Тест по остаткам товаров
#define cmMaintainPrjTask              407 // Обслуживание задач
#define cmPriceAnlz                    408 // Сравнительный анализ цен
#define cmDynamicObjs                  409 // Список динамических объектов
#define cmJobPool                      410 // Серверные задачи
#define cmCheckGStrucRecursion         411 // Проверка нерекурсивности товарных структур
#define cmBillStatus                   412 // Статусы документов
#define cmDBMaintain                   413 // Обслуживание базы данных
#define cmPrcPane                      414 // Панель ввода технологических сессий по процессору
#define cmManufPlan                    415 // Производственные планы
#define cmFTPAcct                      416 // FTP аккаунты
#define cmReport                       417 // Настройка отчетов
#define cmPrcBusy                      418 // Занятость процессоров
#define cmPaymWOLinkBill               419 // Поиск оплат без связанного документа
#define cmRemoveZeroLinkCCLines        420 // Удаление висячих строк чеков
#define cmTouchScreen                  421 // TouchScreen
#define cmEditDesktops                 422 // Редактирование рабочих столов
#define cmSelDesktop                   423 // Выбор рабочего стола
#define cmEditMenus                    424 // Редактирование меню
#define cmEditMenusFromFile            425 // Редактирование меню из файла
#define cmDateTimeRep                  426 // Периодичность
#define cmDutySched                    427 // График дежурств
#define cmLocPrinter                   428 // Локальные принтеры
#define cmCfgImpSpecSeries             429 // Конфигурация импорта специальных серий
#define cmEditWorld                    430 // Редактирование географических объектов
#define cmBillExpImpParams             431 // Редактирование форматов экспорта/импорта документов
#define cmBRowExpImpParams             432 // Редактирование форматов экспорта/импорта строк документов
#define cmImportBills                  433 // Импорт документов
#define cmViewGoodsInfo                434 // Информационный киоск
#define cmEditGoodsInfo                435 // Редактирование информационных киосков
#define cmAddPerson                    436 // Добавить персоналию
#define cmAddTask                      437 // Добавить задачу
#define cmPrjTask_ByStatus             438 // Показать непросмотренные задачи (вызывается по двойному щелчку мыши из окна статуса)
#define cmLoadFonts                    439 // Загрузка шрифтов на принтер штрихкодов
#define cmCashSessExpImpParams         440 // Редактирование форматов экспорта/импорта кассовых сессий
#define cmGenerateGoodsBills           441 // Генерация товарных документов
#define cmPersonRel                    442 // Редактирование персональных отношений
#define cmViewLogs                     443 // Просмотр логов
#define cmMailAcct                     444 // Редактирование почтовых и FTP-аккаунтов
#define cmImportEmailAccts             445 // Импорт почтовых аккаунтов
#define cmKeyboardWKeyCfg              446 // Конфигурация клавиатуры с ключом
#define cmViewGoodsStruc               447 // Товарные структуры
#define cmCorrectTransfer              448 // Восстановление записей transfer
#define cmSalCharge                    449 // Начисления зарплаты
// @v5.5.1 #define cmSalChargeGrp      450 // Группы начислений зарплаты
#define cmStaffCalendar                451 // Календари
#define cmSCSIncasso                   452 // Инкассация (SyncCashSess)
#define cmSupplGoodsImport             453 // Импорт товаров от поставщика
#define cmSupplInterchange             454 // Экспорт данных для поставщика // @v9.2.1 cmExportDataForSuppl-->cmSupplInterchange
#define cmExportDataForSuppl           cmSupplInterchange // @obsolete
#define cmSCardCfg                     455 // Конфигурация дисконтных карт
#define cmRecoverAbsenceAccounts       456 // Восстановление отсутствующих счетов
#define cmAddPersonEvent               457 // Добавить персональное событие
#define cmRecoverAbsencePrjTaskPerson  458 // Восстановление отсутствующих персоналий в задачах
#define cmPersonCfg                    459 // Конфигурация персоналий
#define cmReplaceUnit                  460 // Объединение единиц измерени
#define cmReplaceWorld                 461 // Объединение географических объектов
#define cmRegisterComServer            462 // Регистрация COM-сервера
#define cmUnregisterComServer          463 // Дерегистрация COM-сервера
#define cmDBMaintainCfg                464 // Конфигурация обслуживания базы данных
#define cmViewSalary                   465 // Просмотр начислений зарплаты
#define cmChargeSalary                 466 // Начисление зарплаты
#define cmGoods2CashNodeAssoc          467 // Ассоциация товар (товарная группа) - кассовый узел
#define cmViewStaffCal                 468 // Анализ штатных календарей
#define cmImportKLADR                  469 // Импорт классификатора адресов (КЛАДР)
#define cmGoods2WareplaceAssoc         470 // Ассоциация товар (товарная группа) - места хранения //
#define cmViewObjSyncQueue             471 // Просмотр очереди синхронизации объектов
#define cmPriceListExpImpParams        472 // Конфигурация форматов экспорта/импорта прайс-листов
#define cmCreateObjLikeness            473 // Создание таблицы сравнения объектов
#define cmObjLikeness                  474 // Сравнение объектов
#define cmNamedObjAssoc                475 // Именованные ассоциации объеков
#define cmAddBill                      476 // Добавить документ
#define cmExpImpParamsInventory        477 // Конфигурация импорта инвентаризации
#define cmImportInventory              478 // Импорт инвентаризации
#define cmEditPaintCloths              479 // Редактирование полотен
#define cmEditWhatman                  479 //
#define cmViewLogsMon		           480 // Монитор журналов
#define cmDebtRate                     481 // Процессинг должников
#define cmBizScore                     482 // Бизнес-показатели
#define cmTransportCfg                 483 // Конфигурация транспорта
#define cmBizScoreCalc                 484 // Расчет бизнес-показателей
#define cmBizScoreVal                  485 // Просмотр бизнес-показателей
#define cmShowBizScoreOnDesktop        486 // Показать бизнес-показатели на рабочем столе
#define cmGlobalUserAcc                487 // Глобальные учетные записи
#define cmCreateGlobalUserAcc          488 // Регистрация пользователя в глобальной базе данных
#define cmDebtorStat                   489 // Статистика по дебиторам
#define cmDebtorStatCfg                490 // Конфигурация статистики дебиторов
#define cmUpdatePpIni                  491 // Изменение параметров файла bin\pp.ini посредством транспортного файла pp_upd.ini
#define cmGotoSite                     492 // @v6.0.7  Перейти на сайт фирмы Петроглиф
#define cmGenerateCliBnkImport         493 // @v6.0.9 Генерация платежей для импорта из клиент-банк
#define cmLocTransf                    494 // @v6.1.0 Складские операции
#define cmLocationCfg                  495 // @v6.1.0 Конфигурация локаций
#define cmWmsBillList                  496 // @v6.1.1 Документы складских операций
#define cmViewBBoard                   497 // @v6.1.x Информация о запущенных задачах на Job сервере
#define cmSumByDivision                498 // @v6.1.x Быстрое редактирование типов сумм и календарей в привязке к штатным подразделениям.
#define cmDivisionBySum                499 // @v6.1.x Быстрый просмотр штатных подразделений в привязке к типам сумм и календарям.
#define cmLogin                        500 // @v6.1.10 Вход в систему
#define cmDBMaintenance                501 // @v6.1.10 Обслуживание базы данных
#define cmDBExit                       502 // @v6.1.10 Выход из системы
#define cmGoodsExpImpParams            503 // @v6.1.12 Конфигурация импорта/экспорта товаров
#define cmImportGoodsOld               504 // @v6.1.12 Импорт товаров (по устаревшей технологии)
#define cmImportTodoFromVCal           505 // @v6.2.x  Импорт задач из vCalendar/iCalendar
#define cmVerifyAllCRpt                506 // @v6.2.8  Верификация всех отчетов
//#define cmEditImpExpCfgs               507 // @v6.3.0  Конфигурации импорта/экспорта
#define cmImpExpListConfig             507 // @v6.3.0  Списочные конфигурации импорта/экспорта
#define cmTransport                    508 // @v6.3.x  Транспортные средства (PPView)
#define cmLotTag                       509 // @v6.3.x  Теги лотов
#define cmPrivateBasket                510 // @v6.4.0  Показать приватную корзину
#define cmReplaceLikeWorld             511 // @v6.3.x  Объединение похожих географических объектов
#define cmSendBillsByFilt              512 // @v6.4.2  Передать документы с фильтрацией
#define cmSearchDlvrAddr               514 // @v6.4.8  Поиск адреса доставки по коду или ид.
#define cmGoodsValRestr                515 // @v6.4.10 Ограничения товарных величин PPOBJ_GOODSVALRESTR
#define cmPrjTask_ByReminder           516 // @v6.4.15 Показать невыполненые задачи за определенный промежуток дней (указывается в настроках) (вызывается по двойному щелчку мыши из окна статуса)
#define cmCreateTechSess               517 // @v6.5.1  Создание технологической сессии
#define cmConstructionTest             518 // @v6.5.2  Специальная команда для отладки разрабатываемых модулей
#define cmSCardPane                    519 // @v6.5.4  Панель информации о дисконтных картах
#define cmCreateDraftBySupplOrder      520 // @v6.5.6  Создание драфт документов на основе заказа поставщику
#define cmAccBudget                    521 // @v6.5.14 Бюджетные счета
#define cmBudgeting                    522 // @v6.5.14 Бюджетирование
#define cmViewBizScTempl               523 // @v6.6.8  Шаблоны бизнес-показателей
#define cmBizScValByTempl              524 // @v6.6.8  Просмотр бизнес-показателей по шаблону
#define cmRecalcLotCurRest             525 // @v6.6.9  Пересчет остатков по лотам
#define cmViewCheckOpJ                 526 // @v6.6.10 Журнал чековых операций
#define cmPallet                       527 // @v6.7.0  Справочник паллет
#define cmCTblOrdCreate                528 // @v6.7.7  Создание заказа стола в ресторане
#define cmReplaceDynamicObj            529 // @v6.7.12 Объединение динамических объектов
#define cmBillTag                      530 // @v6.8.4  Теги документов
#define cmCTblOrdTimeGraph             531 // @v6.8.5  Временная диаграмма заказов столов
#define cmSearchBillByCtx              532 // @v6.8.7  Поиск документа по контексту
#define cmSetupPreferredBarcodeTags    533 // @v6.8.8  Автоматически устанавливает признаки предпочтительных штрихкодов
#define cmStockOpt                     534 // @v6.8.11
#define cmStockOptCfg                  535 // @v6.8.11
#define cmRFIDDevice                   536 // @v6.8.x  RFID-устройства
#define cmReplacePersonCat             537 // @v6.9.0  Объединение категорий персоналий
#define cmViewDialog                   538 // @v6.9.9  Диалоги
#define cmDebtDim                      539 // @v6.x.x
#define cmGoodsTag                     540 // @v7.0.x  Теги товаров
#define cmFiltPool                     541 // @paul pentaho фильтры
#define cmConvertQuot720               542 // @v7.1.12 Конвертация котировок
#define cmEventToken                   543 // @v7.2.1 Событийные метки
#define cmGlobalUserAccTag             544 // @v7.2.2 Теги глобальных учетных записей
#define cmReplaceBrand                 545 // @v7.2.4 Объединение брэндов
#define cmSmsAccount                   546 // Учетные записи смс-клиентов
#define cmSpecSeries                   547 // @v7.3.0 Специальные серии
#define cmPhoneService                 548 // @v7.3.1 Справочник телефонных сервисов
#define cmImportPhoneList              549 // @v7.3.3 Импорт телефонного справочника
#define cmEditSmsAcc                   550 // @v7.3.3
#define cmIndexPhones                  551 // @v7.3.3 Индексация телефонов из персоналий и из адресов.
#define cmCvtQCert                     552 // @v7.x.x Конвертация сертификатов качества в теги лотов (для лэнда)
#define cmGtaJournal                   553 // @v7.3.9 Просмотр журнала тарифицируемых операций глобальных аккаунтов
#define cmSCardOp                      554 // @v7.3.9 Просмотр операций по дисконтным картам
#define cmImportSCard2                 555 // Импорт дисконтных карт
#define cmGlobalAccCfg                 556 // @v7.4.7 Конфигурация глобальных учетных записей
#define cmCorrectTrfrIntrUnite         557 // @v7.4.10 Корректировка ошибочных объединений документов внутренней передачи
#define cmTestPredictSalesTbl          558 // @v7.5.1 Тестирование таблицы продаж
#define cmUhttStore                    559 // @v7.6.1 Интернет магазины Universe-HTT
#define cmUhttStoreTag                 560 // @v7.6.1 Теги интернет магазинов Universe-HTT
#define cmConvertBillUUID7601          561 // @v7.6.1 Конвертация атрибутов документов UUID
#define cmImportPerson2                562 // @v7.6.1 Импорт справочника персоналий
#define cmExportGoodsRestUhtt          563 // @v7.6.3 "кспорт цен и товарных остатков в Universe-HTT
#define cmLotPacking                   564 // @v7.6.4 Упаковка лотов
#define cmCorrectCCheckLine01          565 // @v7.6.6 Корректировка проблемы строк чеков 01
#define cmTextBrowserTest              566 // @v7.6.x Текстовый редактор тест
#define cmOpenTextFile                 567 // Открыть текстовый файл для редактирования //
#define cmEditIniFiles                 567 // @obsolete @v7.6.x Редактирование ini файлов
#define cmViewNewVersionList           568 // @v7.6.x Просмотр новых версий Papyrus'a
#define cmGenericDevice                569 // @v7.8.0 Обобщенные устройства
#define cmIndexTextObjects             570 // @v7.8.0 Индексация текстовых объектов // @construction
#define cmImportSR25                   571 // Импорт базы данных SR25
#define cmImportSuprWare               572 // Импорт товаров с их состовляющими
#define cmSuprWare                     573 // Товары и их состав
#define cmProcessObjText               574 // Обработка текстовых объектов
#define cmPersonEventByReader          575 // @v7.9.6 Создание персональных событий по чтению с устройсва ввода
#define cmWorkbook                     576 // Контент
#define cmTestReconnect                577 // @v8.0.6 Тест реконнекта серверной сессии
#define cmUserProfile                  578 // @v8.x.x Профайлер по пользователям и функциям
#define cmTSessAutoSms                 579 // Рассылка смс на основе технологических сессий
#define cmImportWorkbook               580 // @v8.1.2 Импорт рабочих книг
#define cmConvertWorkbook813           581 // @v8.1.3 Конвертация рабочих книг
#define cmWorkbookCfg                  582 // @v8.1.7 Конфигурация рабочих книг
#define cmConvertLatexToHtml           583 // @v8.1.12 Команда конвертации документации (только для внутреннего использования)
#define cmRecoverLostUnifiedLinks      584 // @v8.2.8 Корректировка потерянных в результате объединения объектов, ссылок в т'гах
#define cmGenerateKeywordSeq           585 // @v8.2.10 Генерация последовательностей ключевых слов (тест)
#define cmTestUhttGetLocByPhone        586 // @v8.3.2 Тест SOAP-функции получения списка адресов по номеру телефона // @v10.5.12 @unused
#define cmTestUhttGetQuotByLoc         587 // @v8.3.6 Тест SOAP-функции получения списка котировок по коду локации  // @v10.5.12 @unused
#define cmLocationTag                  588 // @v8.3.6 Теги локаций
#define cmCorrectGoodsGroupsNIA        589 // @v8.3.6 Корректировка товарных групп (не интерактивная)
#define cmConvertVersionToHtml         590 // @v8.3.12 Команда конвертации информации о версиях (только для внутреннего использования)
#define cmCafeTable                    591 // @v8.4.1 Справочник столов в ресторанном зале
#define cmAlcReportCfg                 592 // @v8.4.4 Конфигурация алкогольной декларации
#define cmObjTagAllObj                 593 // @v8.5.3 Типы тегов по всем объектам данных
#define cmEdiProvider                  594 // @v8.5.5 EDI-провайдеры
#define cmImportQuot2                  595 // @v8.5.11 Импорт котировок
#define cmImportFias                   596 // @v8.6.2 Импорт справочника ФИАС
#define cmExportDbTblTransfer          597 // @v8.6.7 Экспорт в XML данных таблицы Transfer
#define cmExportDbTblBill              598 // @v8.6.8 Экспорт в XML данных таблицы Bill
#define cmTestFias                     599 // @v8.6.10 Тестирование справочника ФИАС
#define cmViewGeoTracking              600 // @v8.6.11 Просмотр событий гео-трекинга
#define cmImportTSessUHTT              601 // @v8.7.10 Universe-HTT импорт технологических сессий
#define cmTestCURL                     602 // @v8.8.0 Тестирование интерфейса с библиотекой cURL
#define cmTestEGAIS                    603 // @v8.8.0 Тестирование интерфейса с сервером ЕГАИС
#define cmQueryEGAIS                   604 // @v8.8.1 Интерактивный запрос к серверу ЕГАИС
#define cmTestSearchEmail              605 // @v8.8.9 Тестирование функции поиска email адресов в персоналиях и локациях
#define cmImportPosRefs                606 // @v9.0.8 Импорт POS-справочников
#define cmBuildLocalDL600              607 // @v9.1.7 Компиляция локальных структур DL600
#define cmWorkbookInterchange          608 // @v9.3.10 Синхронизация рабочих книг с серверо Universe-HTT
#define cmProcessOsm                   609 // @v9.5.9 Процессинг данных OpenStreetMap
#define cmPosNodeQuery                 610 // @v9.6.5 Интерактивный запрос к кассовому узлу
#define cmTestLogWindow                611 // @v9.7.0 Тестирование окна вывода сообщений
#define cmProcessSartre                612 // @v9.7.8 Импорт данных проекта Sartre
#define cmBillAutoCreate               613 // @v9.7.10 Автоматическое создание документов (попытка объедининить разрозненные процедуры)
#define cmValidateXml                  614 // @v9.8.3 Проверка соответствия xml-файла xsd-схеме
#define cmTestTddo2                    615 // @v9.8.3 Тестирование технологии TDDO-II
#define cmTestVetis                    616 // @v9.8.9 Тестирование интерфейса ВЕТИС
#define cmViewPhnSvcMonitor            617 // @v9.9.10 Монитор состояний телефонного сервера
#define cmDumpData                     618 // @v10.0.05 Dump/Undump данных
#define cmVetisDocument                619 // @v10.0.12 Документы ВЕТИС
#define cmVetisInterchange             620 // @v10.1.2 Обмен данными с ВЕТИС
#define cmTimeSeries                   621 // @v10.2.3 @experimental Временные серии
#define cmTimeSeriesSA                 622 // @v10.2.4 @experimental обучение нейронной сети по временным сериям
#define cmCorrectZeroDebtDimRefs       623 // @v10.2.8 Обнуление висячих ссылок на долговые размерности
#define cmTimeSeriesConfig             624 // @v10.3.1
#define cmFreightPackageType           625 // @v10.4.1 Типы транспортной упаковки
#define cmExportView                   626 // @v10.5.3 Экспорт данных по именованному фильтру
#define cmTestMqc                      627 // @v10.5.7 Тест клиента брокера сообщений
#define cmQuoteReqAnalyze              628 // @v10.5.7 Анализ котировочных запросов
#define cmRollbackQuots                629 // @v10.5.8 Анализ котировочных запросов @erik
#define cmOpenPhonePane                630 // @v10.5.8 Открыть (восстановить размеры) телефонную панель
#define cmPhonePane                    631 // @v10.5.9 Вызов телефонной панели. Создана для идентификации источника вызовов PPView. Дальнешее использование не определено.
#define cmTaxSystemKind                632 // @v10.6.0 Виды систем налогообложения //
#define cmUniFindObj                   633 // @v10.6.1 @construction Унифицированный поиск объектов
#define cmQueryCHZN                    634 // @v10.6.5 Интерактивный запрос к серверу чесный знак
#define cmConvert10703                 635 // @10.7.3 Конвертация рабочих столов в xml @erik
#define cmTssModel                     636 // @v10.7.4 Модели построения стратегий на временных сериях
#define cmGotoVK                       637 // @v10.7.7  Перейти на сайт VK.com в форму авторизации  //@erikTEMP v10.7.7
#define cmCorrectWorldObjNIA           638 // @v10.8.0 Корректировка географических объектов (не интерактивная)
#define cmEvents                       639 // @v10.8.9 События
#define cmEventSubscription            640 // @v10.8.9 Подписка на события //
#define cmSetupGlbSvcUDS               641 // @v10.9.0 Настройка интеграции с UDS Game
#define cmEventsForCurrentUser         642 // @v10.9.1 События (актуальные для текущего пользователя)
#define cmUserMenu                     643 // @v10.9.3 Просмотр пользовательских меню и рабочих столов
#define cmSetupGlbSvcVK                644 // @v10.9.5 Настройка интеграции с VK
#define cmSCSPrintCheckCopy            645 // @v10.9.6 Печать копии чека в кассовой панели

#endif /* __PPMENU_H */
