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
// Диапазон [1..100] Зарезервирован за мета-типами
// Если у идентификатора установлен бит 0x8000000000000000, то дескриптор сущности имеет длину 1 байт
// Если у идентификатора установлен бит 0x4000000000000000, то дескриптор сущности имеет длину 2 байта
//
#define UED_META                            1   // Мета-значения (собственно, перечисление определений этого файла)
#define UED_PREDEFINEDVALUE                 2   // Предопределенные значения: 0, 1, undefined, unknown, -infinity, +infinity, true, false
#define UED_METAERROR                       3   // Ошибки, возникающие при обработке данных SARTRE-UED
#define UED_CONSTANT                        4   // Универсальные константы (pi, e, фундаментальный физические константы, etc)
#define UED_OPERATOR                        5   // Экземпляры сущности имеют формат: { naked-id: 24bit; number-of-arguments: 8bit }. Примеры: add(2), sub(2), mul(2), inverse(1)
	// Оператор не определяет типы аргументов. Это - задача правил.
	/*
		len
		eq
		neq
		le
		ge
		lt
		gt
		lshift
		rshift
		lrot
		rrot
		bwand
		bwor
		bwnot
		bwxor
		and
		or
		not
		xor
		add
		sub
		mul
		div
		idiv
		rem
		sin
		arcsin
		cos
		arccos
		tan
		arctan
		cotan
		arccotan
		log
		exp
		concat
		nth
	*/
#define UED_FIXEDSIZEDATATYPE               6   // Экземпляры сущности имеют формат: { naked-id: 16bit; size: 16bit }. Примеры: GUID, SHA-1 hash, AES cipher block
#define UED_DIMENSION                     102   // Размерности. Примеры: длина, скорость, электрическая емкость, etc
#define UED_UOM                           103   // Единицы измерения //
#define UED_UOM_TIME                      104 // Единицы измерения: Время
#define UED_UOM_QUANTITY                  104 // Единицы измерения: Количество
#define UED_UOM_LENGTH                    104 // Единицы измерения: Длина   meter
#define UED_UOM_AREA                      104 // Единицы измерения: Площадь length^2
#define UED_UOM_VOLUME                    104 // Единицы измерения: Объем   length^3
#define UED_UOM_MASS                      104 // Единицы измерения: Масса   kilogram
#define UED_UOM_CURRENT                   104 // Единицы измерения: Сила тока ampere
#define UED_UOM_AMOUNT                    104 // Единицы измерения: Количество вещества mole
#define UED_UOM_ANGLE                     104 // Единицы измерения: Угол radian
#define UED_UOM_SOLID_ANGLE               104 // Единицы измерения: Сплошной угол steradian
#define UED_UOM_FORCE                     104 // Единицы измерения: Сила newton
#define UED_UOM_PRESSURE                  104 // Единицы измерения: Давление force / area
#define UED_UOM_CHARGE                    104 // Единицы измерения: Электрический заряд coulomb
#define UED_UOM_CAPACITANCE               104 // Единицы измерения: Электрическая емкость farad
#define UED_UOM_RESISTANCE                104 // Единицы измерения: Электрическое сопротивление ohm
#define UED_UOM_CONDUCTANCE               104 // Единицы измерения: Электрическая проводимость siemens
#define UED_UOM_INDUCTANCE                104 // Единицы измерения: Электрическая индуктивность henry
#define UED_UOM_FREQUENCY                 104 // Единицы измерения: Частота колебаний hertz
#define UED_UOM_VELOCITY                  104 // Единицы измерения: Скорость length / time
#define UED_UOM_ACCELERATION              104 // Единицы измерения: Ускорение velocity / time
#define UED_UOM_DENSITY                   104 // Единицы измерения: Плотность вещества mass / volume
#define UED_UOM_LINEAR_DENSITY            104 // Единицы измерения: Линейная плотность вещества mass / length
#define UED_UOM_VISCOSITY                 104 // Единицы измерения: Вязкость вещества force time / area
#define UED_UOM_KINEMATIC_VISCOSITY       104 // Единицы измерения: Кинематическая вязкость вещества viscosity / density
#define UED_UOM_ENERGY                    104 // Единицы измерения: Энергия (работа)
#define UED_UOM_POWER                     104 // Единицы измерения: Мощность watt
#define UED_UOM_MONEY                     104 // Единицы измерения: Денежная размерность us$
#define UED_UOM_CONCENTRATION             104 // Единицы измерения: Концентрация (относительное значение, как то, процент, промилле etc)
#define UED_UOM_TEMPERATURE               104 // Единицы измерения: Температура kelvin

#define UED_UTF32CHAR                     104   // UNICODE-символы
#define UED_MBSTRINGENCODING              106
#define UED_PARTICULAEELEMENTARIAE        105   // Элементарная частица
#define UED_ATOM                          106   // Атом
#define UED_CORPORECAELESTI               107   // Небесное тело (планеты, звезды, etc)
#define UED_BIOTAXON                      108   // Экземпляр биологической таксономии (царство, род, вид, etc)
#define UED_BIOTAXKINGDOM                 109   //
#define UED_BIOTAXPHYLUM                  110   //
#define UED_BIOTAXCLASS                   111   //
#define UED_BIOTAXORDER                   112   //
#define UED_BIOTAXFAMILY                  113   //
#define UED_BIOTAXGENUS                   114   //
#define UED_BIOTAXSPECIES                 115   //
#define UED_LINGUA                        116   // Язык
#define UED_LINGUALOCUS                   117   // Языковая локаль
#define UED_SCRIPT                        118   // Скрипт
#define UED_GENDER                        119   // Пол
#define UED_TIMEZONE                      120   // Временная зона
#define UED_TERRA_CONTINENTEM             121   // Земной континент
#define UED_TERRA_OCEANUM                 122   // Земной океан
#define UED_TERRA_MARE                    123   // Земное море
#define UED_TERRA_FLUMEN                  124   // Земная река
#define UED_TERRA_LACUS                   125   // Земное озеро
#define UED_TERRA_MONS                    126   // Земная гора
#define UED_REGIONEMMUNDI                 127   // Регион мира
#define UED_STATU                         128   // Country
#define UED_URBS                          129   // Населенный пункт
#define UED_CURRENCY                      130   // Валюта. Большинство современных и не очень валют имеют 3-значный ASCII-символ: значения идентификаторов будем формировать преобразованием этих символов в число.
#define UED_CURRENCYPAIR                  131   // Валютная пара
#define UED_STOCKTICKER                   132   // Биржевой тикер
#define UED_FISCALTAX                     133   // Типы фискальных налогов
#define UED_FISCALVATRATE                 134   // Значения ставок НДС
#define UED_PROGLANG                      135   // Языки программирования
#define UED_DATATYPE                      136   // Типы данных в программировании
#define UED_ABSTRACTDATASTRUCT            137   // Абстрактные структуры данных в программировании
#define UED_DATAFORMATMIME                138   // Форматы данных MIME
#define UED_URISCHEME                     139   // Схема данных URI (сетевой протокол обмена)
#define UED_BRAND                         140   // Бренд
#define UED_PACKAGE                       141   // Типы упаковки
#define UED_AIRPORT                       142   // Аэропорты
#define UED_BARCODEENCODING               143   // Стандарты кодирования штрихкодов
#define UED_COLORRGB                      144   //
#define UED_ICAO                          145   // ICAO-код (4 символа; значение включенное)
#define UED_IATA                          146   // IATA-код (3 символа; значение включенное)
#define UED_DAYOFWEEK                     147   // День недели
#define UED_MENSIS                        148   // Месяц

#define UED_MOLECULO                   0x4001 // Молекула
#define UED_POPULUS                    0x4002 // Люди. Как-бы сложно не было фиксировать отдельных людей, этот элемент по крайней мере позволит учесть селебрити.
#define UED_GLN                        0x4003 //
#define UED_EAN13                      0x4004 //
#define UED_EAN8                       0x4005 //
#define UED_UPC                        0x4006 //
#define UED_RU_INN                     0x4007 //
#define UED_RU_KPP                     0x4008 //
#define UED_INTEGER                    0x4009 // INT48
#define UED_DECIMAL                    0x400A //
#define UED_FIXEDPOINT8                0x400B //
#define UED_FIXEDPOINT16               0x400C //
#define UED_DATASIZE                   0x400D // INT48
#define UED_IP4                        0x400E //
#define UED_IP6                        0x400F //
#define UED_PHONENUMBER                0x4010 // Телефонный номер (только цифровое представление длиной до 14 символов - никаких +, добавочных и т.д.)
#define UED_DATE_DAY                   0x4089 // since 0001-01-01
#define UED_DATE_MON                   0x408A // since 0001-01-01
#define UED_DATE_QUART                 0x408B // since 0001-01-01
#define UED_DATE_SMYR                  0x408C // since 0001-01-01
#define UED_DATE_YR                    0x408D // since 0001-01-01
#define UED_DATE_DYR                   0x408E // since 0001-01-01
#define UED_DATE_SMCENT                0x408F // since 0001-01-01
#define UED_DATE_CENT                  0x4090 // since 0001-01-01
#define UED_DATE_MILLENNIUM            0x4091 // since 0001-01-01
#define UED_DATE_DAYBC                 0x4092 // before 0001-01-01
#define UED_DATE_MONBC                 0x4093 // before 0001-01-01
#define UED_DATE_YRBC                  0x4094 // before 0001-01-01
#define UED_DATE_DYRBC                 0x4095 // before 0001-01-01
#define UED_DATE_CENTBC                0x4096 // before 0001-01-01
#define UED_DATE_MILLENNIUMBC          0x4097 // before 0001-01-01
#define UED_DATE_MLNYRAGO              0x4098
#define UED_DATE_BLNYRAGO              0x4099
#define UED_STRING_UTF16               0x4000 // Оставшиеся (48-2) бита - длина данных в байтах
#define UED_STRING_UTF8                0x4000 // Оставшиеся (48-2) бита - длина данных в байтах

#define UED_PRIVATE                      0x80 // Если старший байт равен этому значению, то остальные байты трактуются как частные определения
#define UED_TIME_MSEC                    0x81 // milliseconds since 1601-01-01 (UTC).
#define UED_TIME_SEC                     0x82 // seconds since 1601-01-01 (UTC).
#define UED_TIME_MIN                     0x83 // minuts  since 1601-01-01 (UTC).
#define UED_TIME_HR                      0x84 // hours   since 1601-01-01 (UTC).
#define UED_TIME_TZMSEC                  0x85 // milliseconds since 1601-01-01 (UTC).
#define UED_TIME_TZCSEC                  0x86 // centiseconds since 1601-01-01 (UTC).
#define UED_TIME_TZSEC                   0x87 // seconds since 1601-01-01 (UTC).
#define UED_TIME_TZMIN                   0x88 // minuts  since 1601-01-01 (UTC).
#define UED_TIME_TZHR                    0x89 // hours   since 1601-01-01 (UTC).
#define UED_PLANARANGLE                  0x8A // 2-D angle 56bit
#define UED_PLANARPOINT                  0x8B // 2-D point {28bit; 28bit} integers multiple of 3600 (user will select unit of measure himself)
#define UED_PLANARSIZE                   0x8C // 2-D size {28bit; 28bit} integers multiple of 3600 (user will select unit of measure himself)
#define UED_GEOLOC                       0x8D // {28bit; 28bit}
#define UED_RANGEZEROTOONE               0x8E // Диапазон [0..1] 56bit
#define UED_RANGEMINUSONETOONE           0x8F // Диапазон [-1..1] 56bit

