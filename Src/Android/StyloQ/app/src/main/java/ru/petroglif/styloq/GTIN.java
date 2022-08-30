// GTIN.JAVA
// Copyright (c) A.Sobolev 2022
// Класс для разбора кодов GTIN (портирован из Papyrus)
//
package ru.petroglif.styloq;

public class GTIN {
	// A.I. Описание Количество цифр и формат
	public static final int fldOriginalText   =  0;  // Оригинальный текст, поданый для разбора
	public static final int fldSscc18         =  1;  // 00 Серийный код транспортной упаковки (SSCC-18): 18 цифр
	public static final int fldGTIN14         =  2;  // 01 непосредственно сам Глобальный номер товара (GTIN): 14 цифр
	public static final int fldContainerCt    =  3;  // 02 Количество контейнеров, содержащихся в другой упаковке (используется совместно с АI 37): 14 цифр
	public static final int fldPart           =  4;  // 10 Номер партии: 1?20 буквенно-цифровой
	public static final int fldManufDate      =  5;  // 11 Дата производства: 6 цифр в формате YYMMDD
	public static final int fldExpiryPeriod   =  6;  // 12 Срок годности: 6 цифр в формате YYMMDD
	public static final int fldPackDate       =  7;  // 13 Дата упаковки: 6 цифр в формате YYMMDD
	public static final int fldBestBeforeDate =  8;  // 15 Срок хранения … (Используется для контроля качества): 6 цифр в формате YYMMDD
	public static final int fldExpiryDate     =  9;  // 17 Дата окончания срока действия (Безопасность товара): 6 цифр в формате YYMMDD
	public static final int fldVariant        = 10;  // 20 Вариант продукта: 2 цифры
	public static final int fldSerial         = 11;  // 21 Серийный номер: 1?20 буквенно-цифровой
	public static final int fldHIBCC          = 12;  // 22 HIBCC в количестве, дате, в пакете и ссылка: От 1 до 29 буквенно-цифровой
	public static final int fldLot            = 13;  // 23x Номер лота: От 1 до 19 буквенно-цифровой
	public static final int fldAddendumId     = 14;  // 240 Дополнительная идентификация продукта: 1?30 буквенно-цифровой
	public static final int fldSerial2        = 15;  // 250 Второй серийный номер: 1?30 буквенно-цифровой
	public static final int fldQtty           = 16;  // 30 Количество каждого: 1?8 цифр
	public static final int fldWtNettKg       = 17;  // 310y Вес нетто (кг): 6 цифр
	public static final int fldLenM           = 18;  // 311y Длина изделия — первое измерение (в метрах): 6 цифр
	public static final int fldWidthM         = 19;  // 312y Ширина или диаметр изделия — 2-ое измерение (в метрах): 6 цифр
	public static final int fldThknM          = 20;  // 313y Глубина или толщина изделия — 3-е измерение (в метрах): 6 цифр
	public static final int fldAreaM2         = 21;  // 314y Площадь (в квадратных метрах): 6 цифр
	public static final int fldVolumeL        = 22;  // 315y Объем продукта (в литрах): 6 цифр
	public static final int fldVolumeM3       = 23;  // 316y Объем продукта (в кубических метрах): 6 цифр
	public static final int fldWtNettLb       = 24;  // 320y Вес нетто (в фунтах): 6 цифр
	public static final int fldLenInch        = 25;  // 321y Длина изделия — первое измерение (в дюймах): 6 цифр
	public static final int fldLenFt          = 26;  // 322y Длина изделия — первое измерение (в футах): 6 цифр
	public static final int fldLenYr          = 27;  // 323y Длина изделия — первое измерение (в ярдах): 6 цифр
	public static final int fldDiamInch       = 28;  // 324y Ширина или диаметр изделия — второе измерение (в дюймах): 6 цифр
	public static final int fldDiamFt         = 29;  // 325y Ширина или диаметр изделия — второе измерение (в футах): 6 цифр
	public static final int fldDiamYr         = 30;  // 326y Ширина или диаметр изделия — второе измерение (в ярдах): 6 цифр
	public static final int fldThknInch       = 31;  // 327y Глубина или толщина изделия — 3-е измерение (в дюймах): 6 цифр
	public static final int fldThknFt         = 32;  // 328y Глубина или толщина изделия — 3-е измерение (в футах): 6 цифр
	public static final int fldThknYr         = 33;  // 329y Глубина или толщина изделия — 3-е измерение (в ярдах): 6 цифр
	public static final int fldContainerWtBruttKg  = 34;  // 330Y Вес контейнера брутто (кг): 6 цифр
	public static final int fldContainerLenM       = 35;  // 331y Длина контейнера — первое измерение (в метрах): 6 цифр
	public static final int fldContainerDiamM      = 36;  // 332y Ширина или диаметр контейнера — 2-е измерение (в метрах): 6 цифр
	public static final int fldContainerThknM      = 37;  // 333y Глубина или толщина контейнера — 3-е измерение (в метрах): 6 цифр
	public static final int fldContainerAreaM2     = 38;  // 334y Площадь контейнера (в квадратных метрах): 6 цифр
	public static final int fldContainerVolumeL    = 39;  // 335y Объем контейнера (в литрах): 6 цифр
	public static final int fldContainerGVolumeM3  = 40;  // 336y Валовой объем контейнера (в кубических метрах): 6 цифр
	public static final int fldContainerMassLb     = 41;  // 340y Полная масса контейнера (в фунтах): 6 цифр
	public static final int fldContainerLenInch    = 42;  // 341y Длина контейнера — первое измерение (в дюймах): 6 цифр
	public static final int fldContainerLenFt      = 43;  // 342y Длина контейнера — первое измерение (в футах): 6 цифр
	public static final int fldContainerLenYr      = 44;  // 343y Длина контейнера — первое измерение (в ярдах): 6 цифр
	public static final int fldContainerDiamInch   = 45;  // 344y Ширина или диаметр контейнера — 2-ое измерение (в дюймах): 6 цифр
	public static final int fldContainerDiamFt     = 46;  // 345y Ширина или диаметр контейнера — 2-ое измерение (в футах): 6 цифр
	public static final int fldContainerDiamYr     = 47;  // 346y Ширина или диаметр контейнера — 2-ое измерение (в ярдах): 6 цифр
	public static final int fldContainerThknInch   = 48;  // 347y Глубина или толщина контейнера — 3-е измерение (в дюймах): 6 цифр
	public static final int fldContainerThknFt     = 49;  // 348y Глубина или толщина контейнера — 3-е измерение (в футах): 6 цифр
	public static final int fldContainerThknYr     = 50;  // 349y Глубина или толщина контейнера — 3-е измерение (в ярдах): 6 цифр
	public static final int fldProductAreaInch2    = 51;  // 350y Площадь продукта (квадратные дюймы): 6 цифр
	public static final int fldProductAreaFt2      = 52;  // 351y Площадь продукта (квадратных футов): 6 цифр
	public static final int fldProductAreaM2       = 53;  // 352y Площадь продукта (квадратных метров): 6 цифр
	public static final int fldContainerAreaInch2  = 54;  // 353y Площадь контейнера (квадратный дюйм): 6 цифр
	public static final int fldContainerAreaFt2    = 55;  // 354y Контейнер площади (квадратных футов): 6 цифр
	public static final int fldContainerAreaYr2    = 56;  // 355y Размер контейнера (квадратных ярдов): 6 цифр
	public static final int fldWtNettTOz           = 57;  // 356y Вес нетто (в тройских унциях ): 6 цифр
	public static final int fldVolumeL_2           = 58;  // 360y Объем продукта ( литров ): 6 цифр
	public static final int fldVolumeL_3           = 59;  // 361y Объем продукта ( литров ): 6 цифр
	public static final int fldContainerGVolumeQt  = 60;  // 362y Валовой объем контейнера (в квартах): 6 цифр
	public static final int fldContainerGVolumeGal = 61;  // 363y Валовой объем контейнера (в галлонах): 6 цифр
	public static final int fldVolumeInch3           = 62;  // 364y Объем продукции (в кубических дюймах): 6 цифр
	public static final int fldVolumeFt3             = 63;  // 365y Объем продукции (в кубических футах): 6 цифр
	public static final int fldVolumeM3_2            = 64;  // 366y Объем продукции (в кубических метрах): 6 цифр
	public static final int fldContainerGVolumeInch3 = 65;  // 367y Валовой объем контейнера (в кубических дюймах): 6 цифр
	public static final int fldContainerGVolumeFt3   = 66;  // 368y Валовой объем контейнера (в кубических футах): 6 цифр
	public static final int fldContainerGVolumeM3_2  = 67;  // 369y Валовой объем контейнера (в кубических метрах): 6 цифр
	public static final int fldCount                 = 68;  // 37 Количество единиц, содержащихся (используется совместно с AI 02): 1?8 цифр
	public static final int fldCustomerOrderNo       = 69;  // 400 Номер заказа Клиента: От 1 до 29 буквенно-цифровой
	public static final int fldShipTo                = 70;  // 410 Адресат/Грузополучатель (код EAN-13 или DUNS): 13 цифр
	public static final int fldBillTo                = 71;  // 411 Доставка счета / код места, где будет произведена оплата за продукт (код EAN-13 или DUNS): 13 цифр
	public static final int fldPurchaseFrom          = 72;  // 412 Приобретение / код места, где будет произведена покупка (код EAN-13 или DUNS): 13 цифр
	public static final int fldShitToZip             = 73;  // 420 Доставка по почтовому индексу (внутренний почтовый индекс): 1?9 алфавитно-цифровой
	public static final int fldShitToZipInt          = 74;  // 421 Доставка по почтовому индексу (международный почтовый индекс): 4?12 буквенно-цифровой
	public static final int fldCountry               = 75;  // 422 Код страны в соответствии со стандартом ISO: 3 цифр
	public static final int fldRollDimentions        = 76;  // 8001 Размер продукта для рулона (ширина, длина и диаметр): 14 цифр
	public static final int fldESN                   = 77;  // 8002 Электронный серийный номер (ESN) исключительно для мобильных телефонов: 1?20 буквенно-цифровой
	public static final int fldGRAI                  = 78;  // 8003 Идентификатор возвращаемого актива — GRAI: 14 цифр UPC / EAN и 1?16 серийный номер возвращаемых активов
	public static final int fldGIAI                  = 79;  // 8004 Второй идентификатор возвращаемого актива — GIAI: 1?30 буквенно-цифровой
	public static final int fldPrice                 = 80;  // 8005 Цена за единицу измерения: 6 цифр
	public static final int fldCouponCode1           = 81;  // 8100 Расширенный код купона: Системный номер и предложение: 6 цифр
	public static final int fldCouponCode2           = 82;  // 8101 Расширенный код купона: Система счисления, предложения и завершения предложения: 10 цифр
	public static final int fldCouponCode3           = 83;  // 8102 Расширенный код купона: количеству в системе счисления предшествовует 0: 2 цифры
	public static final int fldMutualCode            = 84;  // 90 По взаимному согласию сторон: 1?30 значный буквенно-цифровой
	public static final int fldUSPS                  = 85;  // 91 USPS услуг: 2-значный код услуги, 9-значный код клиента, 8-значный ID плюс 1 контрольная цифра упаковки
	public static final int fldInner1                = 86;  // 92 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner2                = 87;  // 93 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner3                = 88;  // 94 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner4                = 89;  // 95 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner5                = 90;  // 96 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner6                = 91;  // 97 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner7                = 92;  // 98 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldInner8                = 93;  // 99 Внутренние коды компании 1?30 буквенно-цифровой
	public static final int fldPriceRuTobacco        = 94;  // Собственный идентификатор - МРЦ сигарет (кодируется)
	public static final int fldControlRuTobacco      = 95;  // Собственный идентификатор - контрольная последовательность в конце маркировки сигарет (Россия).

	private static class FixedLengthToken {
		FixedLengthToken(int id, int fxLen)
		{
			Id = id;
			FxLen = fxLen;
		}
		int  Id;
		int  FxLen;
	};
	private static FixedLengthToken [] FixedLengthTokenList;
	private static SLib.SIntToSymbTabEntry [] GtinPrefix;
	private int [] OnlyTokenList;
	int    ChZnParseResult; // Целочисленный индикатор результата вызова ParseChZnCode()
	int    SpecialNaturalToken;  // При разборе может появиться специальный случай, отражаемый как NaturalToken (например, SNTOK_CHZN_CIGITEM)
	char   SpecialStopChars[];   // @v10.9.9 Специальные символы-разделители токенов. Все символы в этом буфере, предшествующие 0 считаются разделителями.
	SLib.LAssocVector SpecialFixedTokens; // Значение длины 1000 означает 'до конца строки' (UNTIL EOL)
	SLib.LAssocVector SpecialMinLenTokens; // @v10.9.6 Специфицированные минимальные длины токенов
	private SLib.StrAssocArray L; // Результат разбора строки GTIN {id, value}

	public GTIN()
	{
		L = null;
		OnlyTokenList = null;
		SpecialFixedTokens = null;
		SpecialMinLenTokens = null;
		SpecialNaturalToken = 0;
		SpecialStopChars = null;
		ChZnParseResult = 0;
		if(GtinPrefix == null) {
			GtinPrefix = new SLib.SIntToSymbTabEntry[] {
				new SLib.SIntToSymbTabEntry(fldSscc18,                "00"),
				new SLib.SIntToSymbTabEntry(fldGTIN14,                "01"),
				new SLib.SIntToSymbTabEntry(fldContainerCt,           "02"),
				new SLib.SIntToSymbTabEntry(fldPart,                  "10"),
				new SLib.SIntToSymbTabEntry(fldManufDate,             "11"),
				new SLib.SIntToSymbTabEntry(fldExpiryPeriod,          "12"),
				new SLib.SIntToSymbTabEntry(fldPackDate,              "13"),
				new SLib.SIntToSymbTabEntry(fldBestBeforeDate,        "15"),
				new SLib.SIntToSymbTabEntry(fldExpiryDate,            "17"),
				new SLib.SIntToSymbTabEntry(fldVariant,               "20"),
				new SLib.SIntToSymbTabEntry(fldSerial,                "21"),
				new SLib.SIntToSymbTabEntry(fldHIBCC,                 "22"),
				new SLib.SIntToSymbTabEntry(fldQtty,                  "30"),
				new SLib.SIntToSymbTabEntry(fldCount,                 "37"),
				new SLib.SIntToSymbTabEntry(fldMutualCode,            "90"),
				new SLib.SIntToSymbTabEntry(fldUSPS,                  "91"),
				new SLib.SIntToSymbTabEntry(fldInner1,                "92"),
				new SLib.SIntToSymbTabEntry(fldInner2,                "93"),
				new SLib.SIntToSymbTabEntry(fldInner3,                "94"),
				new SLib.SIntToSymbTabEntry(fldInner4,                "95"),
				new SLib.SIntToSymbTabEntry(fldInner5,                "96"),
				new SLib.SIntToSymbTabEntry(fldInner6,                "97"),
				new SLib.SIntToSymbTabEntry(fldInner7,                "98"),
				new SLib.SIntToSymbTabEntry(fldInner8,                "99"),
				new SLib.SIntToSymbTabEntry(fldLot,                   "23x"),
				new SLib.SIntToSymbTabEntry(fldAddendumId,            "240"),
				new SLib.SIntToSymbTabEntry(fldSerial2,               "250"),
				new SLib.SIntToSymbTabEntry(fldCustomerOrderNo,       "400"),
				new SLib.SIntToSymbTabEntry(fldShipTo,                "410"),
				new SLib.SIntToSymbTabEntry(fldBillTo,                "411"),
				new SLib.SIntToSymbTabEntry(fldPurchaseFrom,          "412"),
				new SLib.SIntToSymbTabEntry(fldShitToZip,             "420"),
				new SLib.SIntToSymbTabEntry(fldShitToZipInt,          "421"),
				new SLib.SIntToSymbTabEntry(fldCountry,               "422"),
				new SLib.SIntToSymbTabEntry(fldWtNettKg,              "310y"),
				new SLib.SIntToSymbTabEntry(fldLenM,                  "311y"),
				new SLib.SIntToSymbTabEntry(fldWidthM,                "312y"),
				new SLib.SIntToSymbTabEntry(fldThknM,                 "313y"),
				new SLib.SIntToSymbTabEntry(fldAreaM2,                "314y"),
				new SLib.SIntToSymbTabEntry(fldVolumeL,               "315y"),
				new SLib.SIntToSymbTabEntry(fldVolumeM3,              "316y"),
				new SLib.SIntToSymbTabEntry(fldWtNettLb,              "320y"),
				new SLib.SIntToSymbTabEntry(fldLenInch,               "321y"),
				new SLib.SIntToSymbTabEntry(fldLenFt,                 "322y"),
				new SLib.SIntToSymbTabEntry(fldLenYr,                 "323y"),
				new SLib.SIntToSymbTabEntry(fldDiamInch,              "324y"),
				new SLib.SIntToSymbTabEntry(fldDiamFt,                "325y"),
				new SLib.SIntToSymbTabEntry(fldDiamYr,                "326y"),
				new SLib.SIntToSymbTabEntry(fldThknInch,              "327y"),
				new SLib.SIntToSymbTabEntry(fldThknFt,                "328y"),
				new SLib.SIntToSymbTabEntry(fldThknYr,                "329y"),
				new SLib.SIntToSymbTabEntry(fldContainerWtBruttKg,    "330Y"),
				new SLib.SIntToSymbTabEntry(fldContainerLenM,         "331y"),
				new SLib.SIntToSymbTabEntry(fldContainerDiamM,        "332y"),
				new SLib.SIntToSymbTabEntry(fldContainerThknM,        "333y"),
				new SLib.SIntToSymbTabEntry(fldContainerAreaM2,       "334y"),
				new SLib.SIntToSymbTabEntry(fldContainerVolumeL,      "335y"),
				new SLib.SIntToSymbTabEntry(fldContainerGVolumeM3,    "336y"),
				new SLib.SIntToSymbTabEntry(fldContainerMassLb,       "340y"),
				new SLib.SIntToSymbTabEntry(fldContainerLenInch,      "341y"),
				new SLib.SIntToSymbTabEntry(fldContainerLenFt,        "342y"),
				new SLib.SIntToSymbTabEntry(fldContainerLenYr,        "343y"),
				new SLib.SIntToSymbTabEntry(fldContainerDiamInch,     "344y"),
				new SLib.SIntToSymbTabEntry(fldContainerDiamFt,       "345y"),
				new SLib.SIntToSymbTabEntry(fldContainerDiamYr,       "346y"),
				new SLib.SIntToSymbTabEntry(fldContainerThknInch,     "347y"),
				new SLib.SIntToSymbTabEntry(fldContainerThknFt,       "348y"),
				new SLib.SIntToSymbTabEntry(fldContainerThknYr,       "349y"),
				new SLib.SIntToSymbTabEntry(fldProductAreaInch2,      "350y"),
				new SLib.SIntToSymbTabEntry(fldProductAreaFt2,        "351y"),
				new SLib.SIntToSymbTabEntry(fldProductAreaM2,         "352y"),
				new SLib.SIntToSymbTabEntry(fldContainerAreaInch2,    "353y"),
				new SLib.SIntToSymbTabEntry(fldContainerAreaFt2,      "354y"),
				new SLib.SIntToSymbTabEntry(fldContainerAreaYr2,      "355y"),
				new SLib.SIntToSymbTabEntry(fldWtNettTOz,             "356y"),
				new SLib.SIntToSymbTabEntry(fldVolumeL_2,             "360y"),
				new SLib.SIntToSymbTabEntry(fldVolumeL_3,             "361y"),
				new SLib.SIntToSymbTabEntry(fldContainerGVolumeQt,    "362y"),
				new SLib.SIntToSymbTabEntry(fldContainerGVolumeGal,   "363y"),
				new SLib.SIntToSymbTabEntry(fldVolumeInch3,           "364y"),
				new SLib.SIntToSymbTabEntry(fldVolumeFt3,             "365y"),
				new SLib.SIntToSymbTabEntry(fldVolumeM3_2,            "366y"),
				new SLib.SIntToSymbTabEntry(fldContainerGVolumeInch3, "367y"),
				new SLib.SIntToSymbTabEntry(fldContainerGVolumeFt3,   "368y"),
				new SLib.SIntToSymbTabEntry(fldContainerGVolumeM3_2,  "369y"),
				new SLib.SIntToSymbTabEntry(fldRollDimentions,        "8001"),
				new SLib.SIntToSymbTabEntry(fldESN,                   "8002"),
				new SLib.SIntToSymbTabEntry(fldGRAI,                  "8003"),
				new SLib.SIntToSymbTabEntry(fldGIAI,                  "8004"),
				new SLib.SIntToSymbTabEntry(fldPrice,                 "8005"),
				new SLib.SIntToSymbTabEntry(fldCouponCode1,           "8100"),
				new SLib.SIntToSymbTabEntry(fldCouponCode2,           "8101"),
				new SLib.SIntToSymbTabEntry(fldCouponCode3,           "8102"),
			};
		}
		if(FixedLengthTokenList == null) {
			FixedLengthTokenList = new FixedLengthToken[] {
				new FixedLengthToken(fldSscc18, 18), // num
				new FixedLengthToken(fldGTIN14, 14), // num
				new FixedLengthToken(fldContainerCt, 14), // num
				//new FixedLengthToken(fldPart,                  "10"), // 1..20 alnum
				new FixedLengthToken(fldManufDate, 6), // num
				new FixedLengthToken(fldExpiryPeriod, 6), // num
				new FixedLengthToken(fldPackDate, 6), // num
				new FixedLengthToken(fldBestBeforeDate, 6), // num
				new FixedLengthToken(fldExpiryDate, 6), // num
				new FixedLengthToken(fldVariant, 2), // num
				//new FixedLengthToken(fldSerial,                "21"), // 1..20 num
				//new FixedLengthToken(fldHIBCC,                 "22"), // 1..29 alnum
				//new FixedLengthToken(fldQtty,                  "30"), // 1..8 num
				//new FixedLengthToken(fldCount,                 "37"), // 1..8 num
				//new FixedLengthToken(fldMutualCode,            "90"), // 1..30 alnum
				//new FixedLengthToken(fldUSPS,                  "91"), // 1..8 num
				//new FixedLengthToken(fldInner1,                "92"), // 1..30 alnum
				//new FixedLengthToken(fldInner2,                "93"), // 1..30 alnum
				//new FixedLengthToken(fldInner3,                "94"), // 1..30 alnum
				//new FixedLengthToken(fldInner4,                "95"), // 1..30 alnum
				//new FixedLengthToken(fldInner5,                "96"), // 1..30 alnum
				//new FixedLengthToken(fldInner6,                "97"), // 1..30 alnum
				//new FixedLengthToken(fldInner7,                "98"), // 1..30 alnum
				//new FixedLengthToken(fldInner8,                "99"), // 1..30 alnum
				//new FixedLengthToken(fldLot,                   "23x"), // 1..19 alnum
				//new FixedLengthToken(fldAddendumId,            "240"), // 1..30 alnum
				//new FixedLengthToken(fldSerial2,               "250"), // 1..30 alnum
				//new FixedLengthToken(fldCustomerOrderNo,       "400"), // 1..29 alnum
				new FixedLengthToken(fldShipTo, 13), // 13 num
				new FixedLengthToken(fldBillTo, 13), // 13 num
				new FixedLengthToken(fldPurchaseFrom, 13), // 13 num
				//new FixedLengthToken(fldShitToZip,             "420"), // 1..9 alnum
				//new FixedLengthToken(fldShitToZipInt,          "421"), // 4..12 alnum
				new FixedLengthToken(fldCountry, 3), // 3 num
				new FixedLengthToken(fldWtNettKg, 6), // 6 num
				new FixedLengthToken(fldLenM, 6), // 6 num
				new FixedLengthToken(fldWidthM, 6), // 6 num
				new FixedLengthToken(fldThknM, 6), // 6 num
				new FixedLengthToken(fldAreaM2, 6), // 6 num
				new FixedLengthToken(fldVolumeL, 6), // 6 num
				new FixedLengthToken(fldVolumeM3, 6), // 6 num
				new FixedLengthToken(fldWtNettLb, 6), // 6 num
				new FixedLengthToken(fldLenInch, 6), // 6 num
				new FixedLengthToken(fldLenFt, 6), // 6 num
				new FixedLengthToken(fldLenYr, 6), // 6 num
				new FixedLengthToken(fldDiamInch, 6), // 6 num
				new FixedLengthToken(fldDiamFt, 6), // 6 num
				new FixedLengthToken(fldDiamYr, 6), // 6 num
				new FixedLengthToken(fldThknInch, 6), // 6 num
				new FixedLengthToken(fldThknFt, 6), // 6 num
				new FixedLengthToken(fldThknYr, 6), // 6 num
				new FixedLengthToken(fldContainerWtBruttKg, 6), // 6 num
				new FixedLengthToken(fldContainerLenM, 6), // 6 num
				new FixedLengthToken(fldContainerDiamM, 6), // 6 num
				new FixedLengthToken(fldContainerThknM, 6), // 6 num
				new FixedLengthToken(fldContainerAreaM2, 6), // 6 num
				new FixedLengthToken(fldContainerVolumeL, 6), // 6 num
				new FixedLengthToken(fldContainerGVolumeM3, 6), // 6 num
				new FixedLengthToken(fldContainerMassLb, 6), // 6 num
				new FixedLengthToken(fldContainerLenInch, 6), // 6 num
				new FixedLengthToken(fldContainerLenFt, 6), // 6 num
				new FixedLengthToken(fldContainerLenYr, 6), // 6 num
				new FixedLengthToken(fldContainerDiamInch, 6), // 6 num
				new FixedLengthToken(fldContainerDiamFt, 6), // 6 num
				new FixedLengthToken(fldContainerDiamYr, 6), // 6 num
				new FixedLengthToken(fldContainerThknInch, 6), // 6 num
				new FixedLengthToken(fldContainerThknFt, 6), // 6 num
				new FixedLengthToken(fldContainerThknYr, 6), // 6 num
				new FixedLengthToken(fldProductAreaInch2, 6), // 6 num
				new FixedLengthToken(fldProductAreaFt2, 6), // 6 num
				new FixedLengthToken(fldProductAreaM2, 6), // 6 num
				new FixedLengthToken(fldContainerAreaInch2, 6), // 6 num
				new FixedLengthToken(fldContainerAreaFt2, 6), // 6 num
				new FixedLengthToken(fldContainerAreaYr2, 6), // 6 num
				new FixedLengthToken(fldWtNettTOz, 6), // 6 num
				new FixedLengthToken(fldVolumeL_2, 6), // 6 num
				new FixedLengthToken(fldVolumeL_3, 6), // 6 num
				new FixedLengthToken(fldContainerGVolumeQt, 6), // 6 num
				new FixedLengthToken(fldContainerGVolumeGal, 6), // 6 num
				new FixedLengthToken(fldVolumeInch3, 6), // 6 num
				new FixedLengthToken(fldVolumeFt3, 6), // 6 num
				new FixedLengthToken(fldVolumeM3_2, 6), // 6 num
				new FixedLengthToken(fldContainerGVolumeInch3, 6), // 6 num
				new FixedLengthToken(fldContainerGVolumeFt3, 6), // 6 num
				new FixedLengthToken(fldContainerGVolumeM3_2, 6), // 6 num
				new FixedLengthToken(fldRollDimentions, 14), // 14 num
				//new FixedLengthToken(fldESN,                   "8002"), // 1..20 alnum
				//new FixedLengthToken(fldGRAI,                  "8003"), // 14 num + 1..16 alnum
				//new FixedLengthToken(fldGIAI,                  "8004"), // 1..30 alnum
				new FixedLengthToken(fldPrice, 6), // 6 num
				new FixedLengthToken(fldCouponCode1, 6), // 6 num
				new FixedLengthToken(fldCouponCode2, 10), // 10 num
				new FixedLengthToken(fldCouponCode3, 2), // 2 num
			};
		}
	}
	void AddOnlyToken(int token)
	{
		if(SLib.SIntToSymbTab_HasId(GtinPrefix, token)) {
			if(OnlyTokenList == null) {
				OnlyTokenList = new int[1];
				OnlyTokenList[0] = token;
			}
			else {
				boolean found = false;
				for(int i = 0; !found && i < OnlyTokenList.length; i++) {
					if(OnlyTokenList[i] == token)
						found = true;
				}
				if(!found) {
					int [] temp_list = new int[OnlyTokenList.length + 1];
					for(int j = 0; j < OnlyTokenList.length; j++) {
						temp_list[j] = OnlyTokenList[j];
					}
					temp_list[temp_list.length-1] = token;
					OnlyTokenList = temp_list;
				}
			}
		}
	}
	private boolean IsTokenEnabled(int token)
	{
		boolean result = false;
		if(token > 0) {
			if(OnlyTokenList == null || OnlyTokenList.length == 0)
				result = true;
			else {
				for(int i = 0; !result && i < OnlyTokenList.length; i++)
					if(OnlyTokenList[i] == token)
						result = true;
			}
		}
		return result;
	}
	//
	// Descr: Флаги функции DetectPrefix
	//
	private static final int dpfBOL = 0x0001; // Мы находимся в начале строки (нужен для отсеивания префиксов, которые могут встречаться только в начале строки)

	public static class DetectedPrefix {
		DetectedPrefix(int id, int len)
		{
			Id = id;
			Len = len;
		}
		int    Id;
		int    Len;
	}
	private void AddToken(int tok, String value)
	{
		if(tok > 0 && SLib.GetLen(value) > 0) {
			if(L == null)
				L = new SLib.StrAssocArray();
			L.Set(tok, value);
		}
	}
	public boolean HasToken(int tok)
	{
		return (tok > 0 && L != null && L.GetCount() > 0) ? (L.Search(tok) >= 0) : false;
	}
	DetectedPrefix DetectPrefix(final String src, int flags, int currentId/*, uint * pPrefixLen*/)
	{
		int    prefix_id = -1;
		final  int src_len = SLib.GetLen(src);
		int    prefix_len = 0;
		String temp_buf;
		if(src_len >= 4) {
			temp_buf = src.substring(0, 4);
			int _id = SLib.SIntToSymbTab_GetId(GtinPrefix, temp_buf);
			if(IsTokenEnabled(_id) && _id != currentId && !HasToken(_id)) {
				prefix_len = 4;
				prefix_id = _id;
			}
		}
		if(prefix_id <= 0 && src_len >= 3) {
			temp_buf = src.substring(0, 3);
			int _id = SLib.SIntToSymbTab_GetId(GtinPrefix, temp_buf);
			if(IsTokenEnabled(_id) && _id != currentId && !HasToken(_id)) {
				prefix_len = 3;
				prefix_id = _id;
			}
		}
		if(prefix_id <= 0 && src_len >= 2) {
			temp_buf = src.substring(0, 2);
			int _id = SLib.SIntToSymbTab_GetId(GtinPrefix, temp_buf);
			if(IsTokenEnabled(_id) && _id != currentId && !HasToken(_id)) {
				if((flags & dpfBOL) != 0 || (_id != fldSscc18 && _id != fldGTIN14)) {
					prefix_len = 2;
					prefix_id = _id;
				}
			}
		}
		if(prefix_id == fldManufDate || prefix_id == fldExpiryPeriod || prefix_id == fldPackDate || prefix_id == fldBestBeforeDate || prefix_id == fldExpiryDate) {
			// 6 digits
			boolean false_prefix = false;
			if((src_len-prefix_len) >= 6) {
				for(int ci = 0; !false_prefix && ci < 6; ci++) {
					if(!SLib.isdec(src.charAt(prefix_len+ci)))
						false_prefix = true;
				}
			}
			else
				false_prefix = true;
			if(false_prefix) {
				prefix_id = -1;
				prefix_len = 0;
			}
		}
		return (prefix_id > 0) ? new DetectedPrefix(prefix_id, prefix_len) : null;
	}
	void AddSpecialStopChar(char stopChar)
	{
		if(stopChar != 0) {
			final int slot_size = 32;
			if(SpecialStopChars == null) {
				SpecialStopChars = new char[slot_size];
				for(int i = 0; i < slot_size; i++)
					SpecialStopChars[i] = 0;
			}
			{
				for(int i = 0; i < slot_size; i++) {
					if(SpecialStopChars[i] == stopChar)
						break;
					else if(SpecialStopChars[i] == 0) {
						SpecialStopChars[i] = stopChar;
						break;
					}
				}
			}
		}
	}
	boolean IsSpecialStopChar(final char chr)
	{
		if(SpecialStopChars != null && SpecialStopChars[0] != 0 && chr != 0) {
			for(int i = 0; i < SpecialStopChars.length; i++) {
				if(SpecialStopChars[i] == chr)
					return true;
				else if(SpecialStopChars[i] == 0)
					break;
			}
		}
		return false;
	}
	private static class PrefixSpec {
		PrefixSpec(int fixedLen, int minLen)
		{
			FixedLen = fixedLen;
			MinLen = minLen;
		}
		int   FixedLen;
		int   MinLen;
	}
	private PrefixSpec GetPrefixSpec(int prefixId/*,uint * pFixedLen, uint * pMinLen*/)
	{
		PrefixSpec result = null;
		if(SpecialFixedTokens != null) {
			final int sft_idx = SpecialFixedTokens.Search(prefixId/*, &spc_fx_len, 0*/);
			if(sft_idx >= 0)
				result = new PrefixSpec(SpecialFixedTokens.at(sft_idx).Value, 0);
		}
		if(result == null) {
			for(int i = 0; result == null && i < FixedLengthTokenList.length; i++) {
				final FixedLengthToken item = FixedLengthTokenList[i];
				if(item.Id == prefixId && item.FxLen > 0)
					result = new PrefixSpec(item.FxLen, 0);
			}
		}
		if(SpecialMinLenTokens != null) {
			int smlt_idx = SpecialMinLenTokens.Search(prefixId/*, &spc_min_len, 0*/);
			if(smlt_idx >= 0) {
				if(result == null)
					result = new PrefixSpec(0, SpecialMinLenTokens.at(smlt_idx).Value);
				else
					result.MinLen = SpecialMinLenTokens.at(smlt_idx).Value;
			}
		}
		return result;
	}
	private int SetupFixedLenField(final String src, int prefixLen, int fixLen, int fldId)
	{
		boolean is_error = false;
		int   result_offs = 0;
		String temp_buf = null;
		if(fixLen == 1000) {
			temp_buf = src.substring(prefixLen);//Cat(pSrc+prefixLen);
		}
		else {
			//temp_buf.CatN(pSrc+prefixLen, fixLen);
			temp_buf = src.substring(prefixLen, Math.min(prefixLen+fixLen, src.length()));
			if(SLib.GetLen(temp_buf) != fixLen) {
				is_error = false; // @error
			}
			else if(temp_buf.length() != fixLen)
				is_error = false; // @error
			//THROW(temp_buf.Len() == fixLen);
		}
		assert(temp_buf != null);
		final int result_fix_len = SLib.GetLen(temp_buf);
		if(fldId > 0) {
			//THROW(!StrAssocArray::Search(fldId));
			if(L != null && L.Search(fldId) >= 0) {
				is_error = false; // @error
			}
			else {
				AddToken(fldId, temp_buf);
			}
		}
		if(is_error)
			result_offs = 0;
		else
			result_offs = (prefixLen + result_fix_len);
		//CATCH
		//		result_offs = 0;
		//ENDCATCH
		return result_offs;
	}
	int RecognizeFieldLen(final String src, int currentPrefixID)
	{
		int    len = 0;
		int    cidx = 0;
		final int src_len = SLib.GetLen(src);
		while(cidx < src_len) {
			int next_prefix_len = 0;
			//final int next_prefix_id = DetectPrefix(src.substring(cidx), 0, currentPrefixID, &next_prefix_len/*, next_prefix_*/);
			DetectedPrefix next_pfx = DetectPrefix(src.substring(cidx), 0, currentPrefixID);
			if(next_pfx != null) {
				assert (next_pfx.Id > 0);
				break;
			}
			else {
				cidx++;
				len++;
			}
		}
		return len;
	}
	int Parse(String _Code)
	{
		int    ok = 1;
		try {
			if(SLib.GetLen(_Code) > 0) {
				String code_buf = null;
				code_buf = "" + _Code;
				assert (code_buf != _Code && code_buf.equals(_Code));
				//Z();
				code_buf.trim();
				if(SLib.GetLen(code_buf) > 0) {
					String temp_buf = null;
					//SString prefix_;
					//SString next_prefix_;
					AddToken(fldOriginalText, code_buf);
					//STokenRecognizer tr;
					//SNaturalTokenArray nta;
					STokenRecognizer tr = new STokenRecognizer();
					int tokn = 0;
					//tr.Run(code_buf.ucptr(), code_buf.Len(), nta, 0);
					STokenRecognizer.TokenArray nta = tr.Run(code_buf);
					if(nta.Has(STokenRecognizer.SNTOK_CHZN_CIGITEM) > 0.0f) {
						assert (code_buf.length() == 29);
						int offs = 0;
						{
							AddToken(fldGTIN14, code_buf.substring(offs, offs+14));
							offs += 14;
						}
						{
							AddToken(fldSerial, code_buf.substring(offs, offs+7));
							offs += 7;
						}
						{
							AddToken(fldPriceRuTobacco, code_buf.substring(offs, offs+4));
							offs += 4;
						}
						{
							AddToken(fldControlRuTobacco, code_buf.substring(offs, offs+4));
							offs += 4;
						}
						assert (offs == code_buf.length());
						SpecialNaturalToken = STokenRecognizer.SNTOK_CHZN_CIGITEM;
					}
					else if(nta.Has(STokenRecognizer.SNTOK_CHZN_CIGBLOCK) > 0.0f) {
						// 0104600818007879 21t"XzgHU 8005095000 930p2J24014518552
						//assert(oneof2(code_buf.Len(), 52, 35));
						int prefix_len = code_buf.startsWith("01") ? 2 : (code_buf.startsWith("(01)") ? 4 : 0);
						if(prefix_len > 0) {
							//code_buf.ShiftLeft(prefix_len);
							code_buf = code_buf.substring(prefix_len);
							//temp_buf.Z().CatN(code_buf, 14);
							temp_buf = code_buf.substring(0, 14);
							AddToken(fldGTIN14, temp_buf);
							//code_buf.ShiftLeft(14);
							code_buf = code_buf.substring(14);
							prefix_len = code_buf.startsWith("21") ? 2 : (code_buf.startsWith("(21)") ? 4 : 0);
							if(prefix_len > 0) {
								//code_buf.ShiftLeft(prefix_len);
								code_buf = code_buf.substring(prefix_len);
								//temp_buf.Z().CatN(code_buf, 7);
								temp_buf = code_buf.substring(0, 7);
								AddToken(fldSerial, temp_buf);
								//code_buf.ShiftLeft(7);
								code_buf = code_buf.substring(7);
								// @v11.4.9 prefix_len = code_buf.startsWith("8005") ? 4 : (code_buf.startsWith("(8005)") ? 6 : 0);
								// @v11.4.9 {
								boolean spcchr_cigblock_prefix = false;
								if(code_buf.startsWith("8005"))
									prefix_len = 4;
								else if(code_buf.startsWith("(8005)"))
									prefix_len = 6;
								else if(code_buf.startsWith("u001D8005")) {
									prefix_len = 5;
									spcchr_cigblock_prefix = true;
								}
								else
									prefix_len = 0;
								// } @v11.4.9
								if(prefix_len > 0) {
									//code_buf.ShiftLeft(prefix_len);
									code_buf = code_buf.substring(prefix_len);
									//temp_buf.Z().CatN(code_buf, 6);
									temp_buf = code_buf.substring(0, 6);
									AddToken(fldPrice, temp_buf);
									//code_buf.ShiftLeft(6);
									code_buf = code_buf.substring(6);
									if(spcchr_cigblock_prefix && code_buf.startsWith("\u001D93")) {
										code_buf = code_buf.substring(3);
										temp_buf = "" + code_buf;
										AddToken(fldControlRuTobacco, temp_buf);
										SpecialNaturalToken = STokenRecognizer.SNTOK_CHZN_CIGBLOCK;
									}
									else if(code_buf.startsWith("93")) {
										code_buf = code_buf.substring(2);
										temp_buf = "" + code_buf;
										AddToken(fldControlRuTobacco, temp_buf);
										SpecialNaturalToken = STokenRecognizer.SNTOK_CHZN_CIGBLOCK;
									}
								}
							}
						}
					}
					else {
						//const char * p = code_buf.cptr();
						int cidx = 0;
						final int code_buf_len = code_buf.length();
						int dpf = dpfBOL;
						while(cidx < code_buf_len) {
							if(IsSpecialStopChar(code_buf.charAt(cidx))) // @v10.9.9
								cidx++;
							//int prefix_len = 0;
							//int prefix_id = DetectPrefix(code_buf.substring(cidx), dpf, -1, & prefix_len/*, prefix_*/);
							DetectedPrefix _dp = DetectPrefix(code_buf.substring(cidx), dpf, -1);
							dpf = 0;
							//int fixed_len = 0;
							//int min_len = 0;
							if(_dp != null) {
								assert (_dp.Id > 0);
								PrefixSpec pfx_spec = GetPrefixSpec(_dp.Id);
								if(pfx_spec != null && pfx_spec.FixedLen > 0) {
									final int ro = SetupFixedLenField(code_buf.substring(cidx), _dp.Len, pfx_spec.FixedLen, _dp.Id);
									SLib.THROW(ro > 0, 0);
									//p += ro;
									cidx += ro;
								}
								else {
									//temp_buf.Z();
									temp_buf = "";
									//p += prefix_len;
									cidx += _dp.Len;
									//int next_prefix_id = -1;
									//int next_prefix_len = 0;
									//while(*p) {
									while(cidx < code_buf_len) {
										if(IsSpecialStopChar(code_buf.charAt(cidx))) { // @v10.9.9
											//p++; // Специальный стоп-символ пропускаем и завершаем акцепт токена
											cidx++;
											break;
										}
										//else if(min_len > 0 && temp_buf.Len() < min_len) {
										else if(pfx_spec != null && pfx_spec.MinLen > 0 && temp_buf.length() < pfx_spec.MinLen) {
											//temp_buf.CatChar(*p++);
											temp_buf = temp_buf + code_buf.charAt(cidx);
										}
										else {
											//next_prefix_id = DetectPrefix(p, dpf, prefix_id, &next_prefix_len/*, next_prefix_*/);
											DetectedPrefix _dp_next = DetectPrefix(code_buf.substring(cidx), dpf, _dp.Id);
											//if(next_prefix_id > 0) {
											if(_dp_next != null) {
												assert (_dp_next.Id > 0);
												//int next_fld_len = RecognizeFieldLen(p + next_prefix_len, prefix_id);
												int next_fld_len = RecognizeFieldLen(code_buf.substring(cidx + _dp_next.Len), _dp.Id);
												if(next_fld_len == 0) {
													//temp_buf.CatChar( * p++);
													temp_buf = temp_buf + code_buf.charAt(cidx);
													cidx++;
												}
												else
													break;
											}
											else {
												//temp_buf.CatChar( * p++);
												temp_buf = temp_buf + code_buf.charAt(cidx);
												cidx++;
											}
										}
									}
									//THROW(!StrAssocArray::Search (prefix_id));
									SLib.THROW(L.Search(_dp.Id) < 0, 0);
									AddToken(_dp.Id, temp_buf);
								}
							}
							else {
								ok = 2; // unexpected
								break;
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			ok = 0;
		}
		//CATCHZOK
		return ok;
	}
	void SetSpecialFixedToken(int token, int fixedLen)
	{
		if(fixedLen == 1000 || (fixedLen >= 1 && fixedLen <= 50) && SLib.SIntToSymbTab_HasId(GtinPrefix, token)) { // @v10.9.0 30-->50
			if(SpecialFixedTokens != null) {
				SpecialFixedTokens.Remove(token); // @v10.7.12
				SpecialFixedTokens.AddUnique(token, fixedLen);
			}
			else {
				SpecialFixedTokens = new SLib.LAssocVector();
				SpecialFixedTokens.Add(token, fixedLen);
			}

		}
	}
	public String GetToken(int tokenId) { return (L != null) ? L.GetText(tokenId) : null; }
	public int GetSpecialNaturalToken() { return SpecialNaturalToken; }
	public int GetChZnParseResult() { return ChZnParseResult; }

	public static final int pchzncfPretendEverythingIsOk = 0x0001;

	public static GTIN ParseChZnCode(String code, int flags)
	{
		GTIN result = new GTIN();
		result.ChZnParseResult = 0;
		result.AddSpecialStopChar('\u001D');
		result.AddSpecialStopChar('\u00E8');
		result.AddOnlyToken(fldGTIN14);
		result.AddOnlyToken(fldSerial);
		result.SetSpecialFixedToken(fldSerial, 13);
		result.AddOnlyToken(fldPart);
		result.AddOnlyToken(fldAddendumId);
		result.AddOnlyToken(fldUSPS); //
		result.SetSpecialFixedToken(fldUSPS, 4);
		result.AddOnlyToken(fldInner1);
		result.SetSpecialFixedToken(fldInner1, 1000/*UNTIL EOL*/);
		result.AddOnlyToken(fldInner2);
		result.AddOnlyToken(fldSscc18);
		result.AddOnlyToken(fldExpiryDate);
		result.AddOnlyToken(fldManufDate);
		result.AddOnlyToken(fldVariant);
		//result.AddOnlyToken(GtinStruc::fldPriceRuTobacco);
		//result.AddOnlyToken(GtinStruc::fldPrice);
		int   pr = 0;
		{
			String raw_buf;
			String temp_buf;
			byte [] code_bytes = code.getBytes(); // @debug
			{
				temp_buf = "" + code;
				//temp_buf.ShiftLeftChr('\xE8'); // @v10.9.9 Специальный символ. Может присутствовать в начале кода
				if(temp_buf.charAt(0) == '\u00E8')
					temp_buf = temp_buf.substring(1);
				// "]C1"
				// @v11.0.1 {
				//if(temp_buf.HasPrefixIAscii("]C1")) { // Выяснилось, что и такие служебные префиксы встречаются //
				if(temp_buf.startsWith("]C1") || temp_buf.startsWith("]c1")) { // Выяснилось, что и такие служебные префиксы встречаются //
					//temp_buf.ShiftLeft(3);
					temp_buf = temp_buf.substring(3);
					//temp_buf.ShiftLeftChr('\xE8'); // Черт его знает: на всякий случай снова проверим этого обдолбыша
					if(temp_buf.charAt(0) == '\u00E8') // Черт его знает: на всякий случай снова проверим этого обдолбыша
						temp_buf = temp_buf.substring(1);
				}
				code = temp_buf;
			}
			pr = result.Parse(code);
			if(pr != 1 && result.GetToken(fldGTIN14) != null) {
				result.SetSpecialFixedToken(fldSerial, 12);
				pr = result.Parse(code);
				if(pr != 1 && result.GetToken(fldGTIN14) != null) {
					result.SetSpecialFixedToken(fldSerial, 11);
					pr = result.Parse(code);
				}
			}
			if(pr == 1) {
				if(result.GetToken(fldGTIN14) != null && (temp_buf = result.GetToken(fldSerial)) != null) {
					if(result.GetSpecialNaturalToken() == STokenRecognizer.SNTOK_CHZN_CIGITEM)
						result.ChZnParseResult = STokenRecognizer.SNTOK_CHZN_CIGITEM;
					else if(result.GetSpecialNaturalToken() == STokenRecognizer.SNTOK_CHZN_CIGBLOCK)
						result.ChZnParseResult = STokenRecognizer.SNTOK_CHZN_CIGBLOCK;
					else if(temp_buf.length() == 13)
						result.ChZnParseResult = STokenRecognizer.SNTOK_CHZN_SIGN_SGTIN;
					else
						result.ChZnParseResult = STokenRecognizer.SNTOK_CHZN_GS1_GTIN;
				}
			}
		}
		if(result.ChZnParseResult == 0 && (flags & pchzncfPretendEverythingIsOk) != 0) {
			//STokenRecognizer tr;
			//SNaturalTokenArray nta;
			//SNaturalTokenStat nts;
			int tokn = 0;
			STokenRecognizer tr = new STokenRecognizer();
			STokenRecognizer.TokenArray nta = tr.Run(code);
			//tr.Run(reinterpret_cast<const uchar *>(pCode), sstrlen(pCode), nta, &nts);
			if(nta != null && (nta.S.Seq & STokenRecognizer.SNTOKSEQ_ASCII) != 0 && nta.S.Len >= 25)
				result.ChZnParseResult = 100000;
		}
		return result;
	}
}
