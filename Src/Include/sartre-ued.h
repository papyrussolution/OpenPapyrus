// SARTRE-UED.H
// Copyright (c) A.Sobolev 2020
//
// Unified Entity Definition

//
// Свойства сущностей
//
#define UEDCAP_HARDIDENT     0x0001 // Сущность имеет твердый числовой идентификатор
#define UEDCAP_HARDASCIISYMB 0x0002 // Сущность имеет твердый символьный идентификатор, не ассоциированный с конкретным языком, в ascii-кодировке 
#define UEDCAP_HARDLINGSYMB  0x0004 // Сущность имеет твердый символьный идентификатор, ассоциированный с конкретным языком
//
// Идентификация типов сущностей
//
#define UED_META                            1   // Мета-значения (собственно, перечисление определений этого файла)
#define UED_OPERATOR1                       2   // Оператор с одним аргументом
#define UED_OPERATOR2                       3   // Оператор с двумя аргументами
#define UED_CONSTANT                        4   // Универсальные константы
#define UED_ERROR                           5   // 
#define UED_DIMENSION                       6   // Размерности
#define UED_UOM                             7   // Единицы измерения //
#define UED_UTF32CHAR                       8   // UNICODE-символы
#define UED_COLORRGB                        9   // 
#define UED_PARTICULAEELEMENTARIAE         10   // Элементарная частица
#define UED_ATOM                           11   // Атом
#define UED_PLANET                         13   // 
#define UED_BIOTAXON                       14   // 
#define UED_BIOTAXKINGDOM                  15   // 
#define UED_BIOTAXPHYLUM                   16   // 
#define UED_BIOTAXCLASS                    17   // 
#define UED_BIOTAXORDER                    18   // 
#define UED_BIOTAXFAMILY                   19   // 
#define UED_BIOTAXGENUS                    20   // 
#define UED_BIOTAXSPECIES                  21   // 
#define UED_LINGUA                         22   // Язык
#define UED_GENDER                         23   // Пол
#define UED_TIMEZONE                       24   // Временная зона
#define UED_CONTINENT                      25   // Земной континент
#define UED_REGIONEMMUNDI                  26   // Регион мира
#define UED_STATU                          27   // Country
#define UED_URBS                           28   // City
#define UED_CURRENCY                       29   // Валюта
#define UED_CURRENCYPAIR                   30   // Валютная пара
#define UED_TICKER                         31   // Биржевой тикер
#define UED_FISCALTAX                      32   // Типы фискальных налогов
#define UED_FISCALVATRATE                  33   // Значения ставок НДС
#define UED_PROGLANG                       34   // Языки программирования
#define UED_DATATYPE                       35   // Типы данных в программировании
#define UED_ABSTRACTDATASTRUCT             36   // Абстрактные структуры данных в программировании
#define UED_DATAFORMATMIME                 37   // Форматы данных MIME
#define UED_PACKAGE                        38   // Типы упаковки
#define UED_AIRPORT                        39   // Аэропорты
#define UED_SCRIPT                         40   // Скрипты
#define UED_BARCODEENCODING                41   // Стандарты кодирования штрихкодов

#define UED_MOLECULO               0x40000001 // Молекула
#define UED_POPULUS                0x40000002 // Люди. Как-бы сложно не было фиксировать отдельных людей, этот элемент по крайней мере позволит учесть селебрити.
#define UED_GLN                    0x40000003 // 
#define UED_EAN13                  0x40000004 // 
#define UED_EAN8                   0x40000005 // 
#define UED_UPC                    0x40000006 // 
#define UED_RU_INN                 0x40000007 // 
#define UED_RU_KPP                 0x40000008 // 
#define UED_INTEGER                0x40000009 // INT48
#define UED_DECIMAL                0x40000000 // 
#define UED_FIXEDPOINT8            0x40000000 // 
#define UED_FIXEDPOINT16           0x40000000 // 
#define UED_DATASIZE               0x40000000 // INT48
#define UED_IP4                    0x40000000 // 
#define UED_IP6                    0x40000000 //
                                   
#define UED_TIME                   0x80000000 //
#define UED_PLANARANGLE            0x80000000 // 2-D angle
#define UED_PLANARPOINT            0x80000000 // 2-D point {28bit; 28bit} fixed binary point 16bit
#define UED_PLANARSIZE             0x80000000 // 2-D size {28bit; 28bit} fixed binary point 16bit
#define UED_GEOLOC                 0x90000000 // {28bit; 28bit}
