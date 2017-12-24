// _PPO.H
// Copyright (c) A.Sobolev 2009
//
struct PPObjectTag_ {   // @persistent @store(ReferenceTbl+)
	PPID   Tag;         // Const=PPOBJ_TAG
	PPID   ID;          // @id
	char   Name[30];    // @name @!refname
	char   Symb[20];    // Символ для использования в формулах
	char   Reserve[8];  // @reserve
	int16  Flags;       // OTF_XXX
	PPID   LinkObjGrp;  // Дополнительный параметр для ссылочного объекта
	PPID   TagEnumID;   // Тип ссылочного объекта
	long   TagDataType; // OTTYP_XXX
	PPID   ObjTypeID;   // Тип объекта, для которого определен тэг
	PPID   TagGroupID;  // Группа, к которой относится тэг
};

struct PPSecur_ {          // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const={PPOBJ_USR | PPOBJ_USRGRP}
	long   ID;             // @id
	char   Name[32];       // @name @!refname
	PPID   PersonID;       // (USER only) Связанная персоналия //
	long   Reserve[3];     // @reserve
	char   Password[20];   // (USER only) Пароль
	long   Flags;          // (USER only) Флаги (USRF_XXX)
	long   ParentID;       // Родительский объект (группа | конфигурация)
	LDATE  PwUpdate;       // (USER only) Дата последнего изменения пароля //
};

struct PPBarcodeStruc_ {   // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_BCODESTRUC
	long   ID;             // @id
	char   Name[48];       // @name
	char   Templ[20];      // Шаблон
	long   Flags;          // Флаги
	long   Reserve1;       // @reserve
	long   Reserve2;       // @reserve
};

struct PPSCardSeries_ {    // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_SCARDSERIES
	long   ID;             // @id
	char   Name[30];       // @name @!refname
	char   CodeTempl[18];  // Шаблон номеров карт
	long   QuotKindID;     // Вид котировки
	long   PersonKindID;   // @v5.1.8 Вид персоналии, используемый для владельцев карт (по умолчанию - PPPRK_CLIENT)
	long   PDis;           // Скидка (.01%)
	double MaxCredit;      // Максимальный кредит (для кредитных карт)
	long   Flags;          // Флаги
	LDATE  Issue;          // Дата выпуска
	LDATE  Expiry;         // Дата окончания действия //
};

struct PPUnit_ {           // @persistent @store(ReferenceTbl+)
	enum {
		SI       = 0x0001, // (S) Единица системы СИ
		Phisical = 0x0002, // (P) Физическая единица
		Trade    = 0x0004, // (T) Торговая единица (может не иметь однозначного физ. эквивалента)
		Hide     = 0x0008, // (H) Единицу не следует показывать
		IntVal   = 0x0010  // (I) Единица может быть только целочисленной
	};
	long   Tag;            // Const=PPOBJ_UNIT
	long   ID;             // @id
	char   Name[32];       // @name @!refname
	char   Abbr[16];       // 56
	PPID   BaseUnitID;     // ->Ref(PPOBJ_UNIT)
	double BaseRatio;      // 68
	char   Reserve[16];    // 84
	long   Flags;          // 88
};

struct PPNamedObjAssoc_ {  // @persistent @store(ReferenceTbl+)
	enum {
		fLocAsWarehouse = 0x0001,
		fLocAsWareplace = 0x0002
	};
	long   Tag;            // Const=PPOBJ_NAMEDOBJASSOC
	long   ID;             // @id
	char   Name[48];       // @name @!refname
	char   Symb[12];       //
	long   Reserve;        // @reserve
	long   ScndObjGrp;     //
	long   Flags;          //
	long   PrmrObjType;    // PPOBJ_GOODS ||
	long   ScndObjType;    // PPOBJ_WAREHOUSE || PPOBJ_WAREPLACE || PPOBJ_ARTICLE
};

struct PPPersonKind_ {     // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_PRSNKIND
	long   ID;             // @id
	char   Name[48];       // @name
	long   CodeRegTypeID;  // Тип регистрационного документа, используемого для поиска
	PPID   FolderRegTypeID; // @v5.0.6 Тип регистра, идентифицирующего наименование каталога с документами по персоналии
	long   Flags;          // @v5.9.10 @flags
	PPID   DefStatusID;    // @v5.9.10 ->Ref(PPOBJ_PRSNSTATUS) Статус новой персоналии по умолчанию
	char   Reserve[16];    // @reserve
};

struct PPPersonStatus_ {   // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_PRSNSTATUS
	long   ID;             // @id
	char   Name[48];       // @name
	char   Reserve1[20];   // @reserve
	long   Flags;          // PSNSTF_XXX
	long   Reserve2[2];    // @reserve
};

struct PPELinkKind_ {      // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_ELINKKIND
	PPID   ID;             // @id
	char   Name[32];       // @name @!refname
	char   Reserve1[36];   // @reserve
	long   Flags;          //
	PPID   Type;           // ELNKRT_XXX
	long   Reserve2;       // @reserve
};

struct PPCurrency_ {       // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_CURRENCY
	PPID   ID;             // @id
	char   Name[32];       // @name @!refname
	char   Reserve1[16];   // @reserve
	char   Symb[16];       //
	long   Code;           // Numeric currency code
	long   Flags;          //
	long   Reserve3[2];    //
};

struct PPCurRateType_ {    // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_CURRATETYPE
	PPID   ID;             // @id
	char   Name[32];       // @name @!refname
	char   Reserve1[36];   // @reserve
	long   Flags;          //
	long   Reserve2[2];    // @reserve
};

struct PPAmountType_ {     // @persistent @store(ReferenceTbl+)
	int    SLAPI IsTax(PPID taxID  /* GTAX_XXX */) const;
	int    SLAPI IsComplementary() const;
	enum {
		fErrOnDefault = 0x0001, // Если сумма в документе отсутствует, то
			// генерировать сообщение об ошибке
		fManual       = 0x0002, // Сумма может быть введена в ручную.
			// Если флаг установлен, то допускается ручной ввод суммы.
			// Если нет, то сумма расчитывается автоматически по содержанию
			// документа. Исключение - номинальная сумма (PPAMT_MAIN).
			// Она может быть как автоматической так и устанавливаемой в
			// ручную в зависимости от вида операции.
		fTax          = 0x0004, // Сумма налога. Если этот флаг установлен,
			// то значение суммы отражает сумму налога Tax со ставкой (TaxRate*100) %,
			// которым облагается данная операция.
		//
		// Назначение следующих трех флагов в том, чтобы можно было быстро определить итоговые
		// обобщенные суммы GenCost, GenPrice, GenDiscount по товарному документу.
		//
		fReplaceCost     = 0x0010, // Замещает сумму в ценах поступления (Cost)
		fReplacePrice    = 0x0020, // Замещает сумму в ценах реализациия (Price)
		fReplaceDiscount = 0x0040, // Замещает скидку (Discount)
			// @#{fReplaceCost^fReplacePrice^fReplaceDiscount}
		fInAmount        = 0x0080, // Входящая сумма, комплементарная сумме RefAmtTypeID
		fOutAmount       = 0x0100, // Исходящая сумма, комплементарная сумме RefAmtTypeID
			// @#{fTax^fInAmount^fOutAmount}
		fReplaces        = (fReplaceCost | fReplacePrice | fReplaceDiscount),
		fStaffAmount     = 0x0200  // Штатная сумма (используется для расчета зарплаты)
	};
	long   Tag;            // Const=PPOBJ_AMOUNTTYPE
	long   ID;             // @id
	char   Name[48];       // @name
	char   Symb[20];       // Символ для представления в формулах
	long   Flags;          // Флаги
	long   Tax;            // Налог (GTAX_XXX)
	union {
		long   TaxRate;       // Ставка налога в сотых долях процента (Flags & fTax)
		PPID   RefAmtTypeID;  // Сумма, комплементарная данной (Flags & (fInAmount | fOutAmount))
	};
};

struct PPOprType_ {        // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_OPRTYPE
	long   ID;             // @id
	char   Name[48];       // @name
	char   Pict[6];        //
	char   Reserve1[14];   //
	long   Reserve2;       //
	long   Reserve3;       //
	long   Reserve4;       //
};

struct PPOpCounter_ {      // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_OPCOUNTER
	long   ID;             // @id
	char   Name[30];       // @name @!refname
	char   CodeTemplate[18]; // Шаблон нумерации
	char   Reserve[16];    // @reserve
	PPID   ObjType;        // Тип объекта, владеющего счетчиком. Если 0, то PPOBJ_OPRKIND
	long   Flags;          // OPCNTF_XXX
	long   Counter;        //
	long   OwnerObjID;     // Объект-владелец счетчика (если 0, то может принадлежать более чем одной операции)
		// Если OwnerObjID == -1, то счетчик принадлежит конфигурации объекта ObjType
};

struct PPGdsCls_ {         // @persistent @store(ReferenceTbl+)
	static int   SLAPI IsEqByDynGenMask(long mask, const GoodsExtTbl::Rec * p1, const GoodsExtTbl::Rec * p2);
	void   SLAPI SetDynGenMask(int fld, int val);
	int    FASTCALL GetDynGenMask(int fld) const;

	enum { // @persistent
		eKind = 1,
		eGrade,
		eAdd,
		eX,
		eY,
		eZ,
		eW,    // @v5.1.9
		eAdd2  // @v5.1.9
	};
	enum { // @persistent
		fUsePropKind    = 0x0001,
		fUsePropGrade   = 0x0002,
		fUsePropAdd     = 0x0004,
		fUseDimX        = 0x0008,
		fUseDimY        = 0x0010,
		fUseDimZ        = 0x0020,
		fStdEditDlg     = 0x0100, // Используется стандартный, а не редуцированный диалог
			// редактирования товаров, относящихся к классу
		fDupCombine     = 0x0200, // Комбинации {PropKind, PropGrade, DimX, DimY, DimZ}
			// могут дублироваться //
		fDisableFreeDim = 0x0400, // Запрет на ввод размерностей не перечисленных в списке
		fUseDimW        = 0x0800, // @v5.1.9 Использует размерность W
		fUsePropAdd2    = 0x1000  // @v5.1.9 Использует свойство Add2
	};

	long   Tag;            // Const=PPOBJ_GOODSCLASS
	long   ID;             // @id
	char   Name[32];       // @name @!refname
	char   Reserve[12];    // @reserve
	PPID   DefGrpID;       // Группа товара по умолчанию. Используется в случае создания товара только по параметрам расширения.
	PPID   DefUnitID;      // Торговая единица измерения по умолчанию
	PPID   DefPhUnitID;    // Физическая единица измерения по умолчанию
	long   DefPhUPerU;     // @unused
	PPID   DefTaxGrpID;    // Налоговая группа по умолчанию
	PPID   DefGoodsTypeID; // Тип товара по умолчанию
	long   Flags;          // @flags
	uint16 EditDlgID;      // @unused
	uint16 FiltDlgID;      // @unused
	long   DynGenMask;     // @v5.0.1 Маска динамического обобщения. Флаги маски соответствуют
		// элементам перечисления (1<<(PPGdsCls::eXXX-1)). Если система видит обобщенный товар G с
		// параметрами расширения, соответствующими маске, эквивалентными некоторому товару X,
		// то считает, что X принадлежит G.
};

struct PPAssetWrOffGrp_ {  // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_ASSTWROFFGRP
	PPID   ID;             // @id
	char   Name[30];       // @name @!refname
	char   Reserve1[4];    // @reserve
	char   Code[14];       // Код нормы амортизационных отчислений
	long   Reserve2[2];    // @reserve
	long   WrOffType;      // Тип списания (AWOGT_XXX)
	long   WrOffTerm;      // Months
	long   Limit;          // Предельная остаточная стоимость (% от начальной стоимости)
	long   Flags;          //
	long   Reserve3[2];    // @reserve
};

struct PPOprKind_ {        // @persistent @store(ReferenceTbl+)
	SLAPI  PPOprKind_()
	{
		THISZERO();
	}

	long   Tag;            // Const=PPOBJ_OPRKIND
	long   ID;             // @id
	char   Name[42];       // @name @!refname
	int16  Rank;           //
	PPID   LinkOpID;       //
	PPID   AccSheet2ID;    //
	PPID   OpCounterID;    //
	long   PrnFlags;       //
	PPID   DefLocID;       //
	int16  PrnOrder;       //
	int16  SubType;        // OPSUBT_XXX
	long   Flags;          // OPKF_XXX
	PPID   OpTypeID;       //
	PPID   AccSheetID;     //
};

struct PPBillStatus_ {     // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_BILLSTATUS
	long   ID;             // @id
	char   Name[20];       // @name @!refname
	char   Symb[16];       // Символ статуса
	char   Reserve1[32];   // @reserve
	int16  Reserve2;       // @reserve
	int16  Rank;           //
	long   Flags;          // BILSF_XXX
	long   Reserve3;       // @reserve
	// @v5.8.2 (Превышение размера ReferenceTbl::Rec) long   Reserve4;    // @reserve
};

struct PPAccSheet_ {       // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_ACCSHEET
	long   ID;             // @id
	char   Name[48];       // @name
	PPID   BinArID;        // Статья для сброса остатков по закрываемым статьям
	PPID   CodeRegTypeID;  // ИД типа регистрационного документа, идентифицирующего персоналию, соответствующую статье.
	char   Reserve1[12];   // @reserve
	long   Flags;          // ACSHF_XXX
	long   Assoc;          // @#{0L, PPOBJ_PERSON, PPOBJ_LOCATION, PPOBJ_ACCOUNT} Ассоциированный объект
	long   ObjGroup;       // Подгруппа ассоциированных объектов
};

struct PPCashNode_ {       // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_CASHNODE
	long   ID;             // @id
	char   Name[8];        // @name @!refname
	char   Port[8];        // Имя порта (LPT1, COM1, ...)
	char   Reserve[8];     // @reserve
	PPID   GoodsLocAssocID;  // @v5.7.3 Именованная ассоциация товар-склад, спользуемая для печати чеков и списания //
	char   Reserve2[2];    // @reserve
	uint16 SleepTimeout;   // @v5.3.10 Таймаут бездействия (сек), после которого панель блокируется //
	PPID   CurRestBillID;  // Временный документ текущих остатков по незакрытым кассовым сессиям
	char   Accum[8];       // Аккумулятор
	//
	// Поле DownBill имеет специальное назначение.
	// При закрытии синхронной кассовой сессии количество вбитых чеков может
	// быть настолько большим, что в рамках одной транзакции не удается //
	// собрать их все в один документ (ошибка Btrieve = 2). По-этому
	// каждый чек вливается в единый документ отдельной транзакцией. Но
	// если процесс был прерван в результате ошибки, то частично заполненный
	// новый документ остается. Поле DownBill как раз и сохраняет
	// идентификатор этого документа. После устранения ошибки процесс
	// закрытия кассовой сессии можно продолжить с этим документом.
	// В нормальной ситуации это поле всегда нулевое.
	//
	PPID   DownBill;
	PPID   CashType;       // Тип ККМ (PPCMT_XXX)
	long   LogNum;         // Логический номер кассы
	int16  DrvVerMajor;    //
	int16  DrvVerMinor;    //
	PPID   CurSessID;      // ->CSession.ID Текущая кассовая сессия (для синхронных узлов)
	PPID   ExtQuotID;      // ->Ref(PPOBJ_QUOTKIND) Дополнительная котировка (используется некоторыми типами ККМ)
	long   Flags;          // CASHF_XXX      Флаги
	long   LocID;          // ->Location.ID  Склад
	LDATE  CurDate;        // Текущая операционная дата
};

struct PPLocPrinter_ {     // @persistent @store(ReferenceTbl+)
	SLAPI  PPLocPrinter_()
	{
		THISZERO();
	}
	long   Tag;            // Const=PPOBJ_LOCPRINTER
	long   ID;             // @id
	char   Name[20];       // @name @!refname
	char   Port[48];       // Имя порта вывода
	long   Flags;          //
	long   LocID;          // ->Location.ID  Склад
	long   Reserve2;       // @reserve
};

struct PPBarcodePrinter_ { // @persistent @store(ReferenceTbl+)
	SLAPI  PPBarcodePrinter_()
	{
		THISZERO();
	}
	int    SLAPI Normalyze();

	enum {
		fPortEx = 0x0001 // Запись использует расширенное поле имени порта вывода
	};
	PPID   Tag;            // Const=PPOBJ_BCODEPRINTER
	PPID   ID;             // @id
	char   Name[20];       // @name @!refname
	char   Port[18];       //
	char   LabelName[20];  //
	CommPortParams Cpp;    // Size=6
	char   Reserve1[4];    // @reserve
	long   Flags;          //
	PPID   PrinterType;    // PPBCPT_ZEBRA | PPBCPT_DATAMAX
	long   Reserve2;       // @reserve
	//
	SString PortEx;        // @persistent @store(PropertyTbl[PPOBJ_BCODEPRINTER, id, BCPPRP_PORTEX]) @anchor
};

struct PPStyloPalm_ {      // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_STYLOPALM
	PPID   ID;             // @id
	char   Name[20];       // @name @!refname
	char   Reserve1[28];   // @reserve
	PPID   LocID;          // ->Location.ID
	PPID   GoodsGrpID;     //
	PPID   OrderOpID;      // ->Ref(PPOBJ_OPRKIND)
	PPID   AgentID;        // ->Article.ID; Agent - owner of palm
	PPID   GroupID;        // ->Ref(PPOBJ_STYLOPALM) Группа, которой принадлежит устройство
		// Запись, порожденная от группы, наследует у группы следующие поля: LocID, OrderOpID,
		// Flags & (PLMF_IMPASCHECKS|PLMF_EXPCLIDEBT|PLMF_EXPSELL), PPStyloPalmPacket::P_LocList
	long   Flags;          // PLMF_XXX
	PPID   FTPAcctID;      // @v5.0.1 AHTOXA
	long   Reserve4;       //
};

struct PPTouchScreen_ {    // @persistent @store(ReferenceTbl+)
	SLAPI  PPTouchScreen_()
	{
		THISZERO();
	}
	PPID   Tag;            // Const=PPOBJ_TOUCHSCREEN
	PPID   ID;             // @id
	char   Name[20];       // @name @!refname
	PPID   TouchScreenType;  //
	PPID   AltGdsGrpID;    // Альтернативная группа, товары из которой показываются в кассовой панели по умолчанию
	long   GdsListFontHight;    // @v5.3.11 VADIM
	char   GdsListFontName[32]; // @v5.3.11 VADIM
	uint8  GdsListEntryGap;     // @v5.3.11 Дополнительное слагаемое высоты строки списка выбора товара (pixel)
		// Рекомендуемый диапазон: [0..8]
	char   Reserve[3];     // @reserve
	long   Flags;          // TSF_XXX
	long   Reserve2[2];    // @reserve
};

struct PPInternetAccount_ { // @persistent @store(ReferenceTbl+)
	enum {
		fFtpAccount = 0x00000001L
	};
	SLAPI  PPInternetAccount_()
	{
		THISZERO();
	}
	void   SLAPI Init()
	{
		ExtStr = 0;
		memzero(this, sizeof(*this)-sizeof(ExtStr));
	}
	int    SLAPI Cmp(PPInternetAccount_ * pAccount); // @v5.3.5 AHTOXA realy const
	int    SLAPI NotEmpty(); // @v5.3.5 AHTOXA
	int    SLAPI GetExtField(int fldID, char * pBuf, size_t bufLen);
	int    SLAPI GetExtField(int fldID, SString & rBuf); // @v5.3.5 AHTOXA
	int    SLAPI SetExtField(int fldID, const char * pBuf);
	int    SLAPI SetPassword(const char *, int fldID = MAEXSTR_RCVPASSWORD); // @v5.0.1 AHTOXA change
	int    SLAPI GetPassword(char * pBuf, size_t bufLen, int fldID = MAEXSTR_RCVPASSWORD); // @v5.0.1 AHTOXA change
	int    SLAPI SetMimedPassword(const char * pPassword, int fldID  = MAEXSTR_RCVPASSWORD); // @v5.3.5 AHTOXA
	int    SLAPI GetMimedPassword(char * pBuf, size_t bufLen, int fldID = MAEXSTR_RCVPASSWORD); // @v5.3.5 AHTOXA
	int    SLAPI GetSendPort();
	int    SLAPI GetRcvPort();
	//
	long   Tag;            // Const=PPOBJ_INTERNETACCOUNT
	long   ID;             //
	char   Name[32];       // @name
	uint16 SmtpAuthType;   // Тип аутентификации для соединения с SMTP сервером
	char   Reserve1[32];   // @reserve
	int16  Timeout;        // Таймаут сервера (сек)
	long   Flags;          //
	PPID   PersonID;       // ->Person.ID
	long   Reserve2;       // @reserve
	//
	SString ExtStr;        // @anchor
};

struct PPDBDiv_ {          // @persistent @store(ReferenceTbl+)
	//
	// Descr: Следующие флаги полностью дублируют макросы DBDIVF_XXX.
	//   Использовать флаги PPDBDiv::fXXX НЕЛЬЗЯ. Сделано для будущих релизов.
	//
	enum {
		fDispatch              = 0x0001, // Раздел-диспетчер
		fSCardOnly             = 0x0002, // Обмен с разделом идет только на уровне пластиковых карт
		fRcvCSessAndWrOffBills = 0x0004, // Раздел принимает кассовые сессии вместе с документами списания //
			// Если этот флаг установлен, то функция PPObjectTransmit::PutObjectToIndex пропускает документы
			// списания кассовых сессий несмотря на флаг DBDXF_SENDCSESSION в конфигурации обмена данными.
		fConsolid              = 0x0008, // Раздел, принимающий консолидирующую информацию из других
			// разделов (документы без товарных строк)
		fPassive               = 0x0010  // Пассивный раздел
	};
	long   Tag;           // Const=PPOBJ_DBDIV
	long   ID;            // Ид
	char   Name[30];      // Наименование раздела
	char   Addr[38];      // Адрес узла e-mail или что-то в этом роде
	long   Flags;         // Флаги
	long   IntrRcptOpr;   // Операция межскладского прихода
	long   OutCounter;    // Счетчик исходящих пакетов
};

struct PPGoodsType_ {      // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_GOODSTYPE
	long   ID;             //
	char   Name[30];       //
	char   Reserve1[18];   //
	PPID   WrOffGrpID;     // Группа списания основных фондов (required GTF_ASSETS)
	PPID   AmtCVat;        // Сумма НДС в ценах поступления             //
	PPID   AmtCost;        // ->Ref(PPOBJ_AMOUNTTYPE) Сумма поступления //
	PPID   AmtPrice;       // ->Ref(PPOBJ_AMOUNTTYPE) Сумма реализации  //
	PPID   AmtDscnt;       // ->Ref(PPOBJ_AMOUNTTYPE) Сумма скидки
	long   Flags;          //
	long   Reserve3[2];    //
};

struct PPGoodsStrucHeader_ { // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_GOODSSTRUC
	long   ID;             // @id
	char   Name[30];       // @name
	char   Reserve1[18];   // @reserve
	PPID   VariedPropObjType; // Тип объекта изменяющейся характиристики заголовочного товара структуры и элементов структуры.
	DateRange Period;      // Период актуальности структуры
	double CommDenom;      //
	long   Flags;          // GSF_XXX
	PPID   ParentID;       // Родительская структура
	long   Reserve3;       // @reserve
};

struct PPGoodsTax_ {       // @persistent @store(ReferenceTbl+)
	int    SLAPI ToEntry(PPGoodsTaxEntry *) const;
	int    SLAPI FromEntry(const PPGoodsTaxEntry *);
	PPID   Tag;            // Const=PPOBJ_GOODSTAX
	PPID   ID;             //
	char   Name[30];       //
	char   Symb[14];       //
	double VAT;            //
	double Excise;         //
	double SalesTax;       //
	long   Flags;          //
	long   Order;          //
	long   UnionVect;      //
};

struct PPRegisterType_ {   // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_REGISTERTYPE
	PPID   ID;             // @id
	char   Name[40];       // Наименование типа регистра
	char   Symb[20];       // Символ для ссылок из формул и т.д.
	PPID   PersonKindID;   // Вид персоналии, к которой должен относиться этот регистр. Если 0, то к любому виду.
	PPID   RegOrgKind;     // ->Ref(PPOBJ_PRSNKIND) Вид регистрирующей организации
	long   Flags;          // @flags
	PPID   CounterID;      // ->Ref(PPOBJ_OPCOUNTER)
	long   Reserve2;       // @reserve
};

struct PPQuotKind_ {       // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_QUOTKIND
	PPID   ID;             // @id
	char   Name[30];       // @name
	double Discount;       // Скидка
	char   Symb[12];       // Символ
	DateRange  Period;     // Период действия розничной котировки
	int16  BeginTm;        // Время начала действия розничной котировки
	int16  EndTm;          // Время окончания действия розничной котировки
	int16  Rank;           // Уровень приоритета котировки при показе в диалоге
		// редактирования или в тех случаях, когда все котировки не могут быть использованы
		// в виду ограничения на количество видов (например, при загрузке в StyloPalm).
		// Чем выше значение, тем выше вероятность использования этого вида котировки
		// по сравнению с другими.
	PPID   OpID;           // Вид операции, для которой определена котировка
	long   Flags;          // Флаги (QUOTKF_XXX)
	PPID   AccSheetID;     // Таблица статей, с которыми ассоциируются значения котировок.
		// Если AccSheetID == 0, то полагается, что таблица статей GetSellAccSheet() (покупатели)
	char   DaysOfWeek;     // @v5.1.7 VADIM Дни недели действия розничной котировки (0x01 - Пн, ... , 0x40 - Вс)
	char   UsingWSCard;    // @v5.1.7 VADIM Совместное использование с дисконтными картами (uwscXXX)
	enum {                 //
		uwscDefault = 0,   // - сначала котировка, затем скидка по карте
		uwscSCardNQuot,    // - сначала скидка по карте, затем котировка
		uwscOnlyQuot,      // - только котировка
		uwscOnlySCard      // - только скидка по карте
	};
	char   Reserve[2];     // @reserve
};

struct PPPsnOpKind_ {      // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_PERSONOPKIND
	PPID   ID;             // @id
	char   Name[36];       // @name // @v5.4.7 48->36
	char   Symb[14];       // @v5.4.7 Символ
	PPID   PairOp;         // @#{PairOp != ID} ->self.ID   Парная операция //
	PPID   RegTypeID;      // ->Ref(PPOBJ_REGISTERTYPE).ID Тип регистрационного документа
	short  Reserve2;       // @reserve
	short  ExValGrp;       // Группа дополнительного значения (POPKEVG_XXX)
	PPID   ExValSrc;       // Источник дополнительного значения //
		// if(ExValGrp == POPKEVG_TAG) then ExValSrc ->Ref(PPOBJ_TAG).ID
	short  PairType;       // Тип парности (POPKPT_XXX)
	long   Flags;          // Флаги (POPKF_XXX)
	PPID   LinkBillOpID;   // ->Ref(PPOBJ_OPRKIND).ID @v5.2.2
	PPID   ParentID;       // Родительская группа
};

struct PPWorldObjStatus_ { // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_CITYSTATUS
	long   ID;             // @id
	char   Name[32];       // @name
	char   Abbr[16];       // Сокращение
	char   Reserve[20];    // @reserve
	long   Flags;          // @reserve
	long   Kind;           // WORLDOBJ_XXX
	long   Code;           // Классификатор (возможно, связан с внешним классификатором). Если Code!=0, то значение уникально
};

struct PPPersonRelType_ {  // @persistent @store(ReferenceTbl+)
	enum {
		cOneToOne = 1,
		cOneToMany,
		cManyToOne,
		cManyToMany
	};
	enum {
		ssUndef = 0,
		ssPrivateToPrivate,
		ssPrivateToLegal,
		ssLegalToPrivate,
		ssLegalToLegal
	};
	enum {
		fInhAddr  = 0x0001,
		fInhRAddr = 0x0002,
		fGrouping = 0x0004, // @v5.7.1 Группирующее отношение (только для Cardinality = cManyToOne)
		fInhMainOrgAgreement = 0x0008 // Наследует соглашение с клиентами из главной организации (только для филиалов)
	};
	long   Tag;            // Const=PPOBJ_PERSONRELTYPE
	long   ID;             //
	char   Name[32];       //
	char   Reserve1[16];   //
	char   Symb[20];       //
	int16  StatusRestriction; // Ограничение по статусу отношений (PPPersonRelType::ssXXX)
	int16  Cardinality;    // Ограничение по множественности отношений (PPPersonRelType::cXXX)
	long   Flags;          // Флаги (PPPersonRelType::fXXX)
	long   Reserve2;       // @reserve // @v5.8.2 [2]-->[1] (Превышение размера ReferenceTbl::Rec)
};

struct PPSalCharge_ {      // @persistent @store(ReferenceTbl+)
	enum {
		fGroup       = 0x0001, // Группа начислений (объединяет произвольный упорядоченный набор обыкновенных начислений)
		fWrOffSingle = 0x0002  // Начисления по всем контрагентам за один период списывать одним документом (суммы складываются)
	};
	PPID   Tag;            // Const=PPOBJ_SALCHARGE
	PPID   ID;             // @id
	char   Name[30];       // @name
	char   Symb[10];       // Символ
	PPID   AmtID;          // ->Ref(PPOBJ_AMOUNTTYPE) Тип учетной суммы для этого начисления //
	PPID   CalID;          // ->Ref(PPOBJ_STAFFCAL)   Календарь, используемый для этого начисления //
	PPID   WrOffOpID;      // ->Ref(PPOBJ_OPRKIND)    Вид операции списания //
	char   Reserve2[16];   // @reserve
	long   Flags;          //
	long   Reserve3[2];    // @reserve
};

struct PPDateTimeRep_ {    // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_DATETIMEREP
	PPID   ID;             // @id
	char   Name[48];       // @name
	DateTimeRepeating Dtr; //
	long   Duration;       // Продолжительность (sec)
	char   Reserve1[4];    //
	long   Reserve2[3];    //
};

struct PPDutySched_ {      // @persistent @store(ReferenceTbl+)
	PPID   Tag;            // Const=PPOBJ_DUTYSHED
	PPID   ID;             // @id
	char   Name[20];       // @name
	char   Reserve1[48];   // @reserve
	long   Flags;          //
	PPID   ObjType;        //
	long   ObjGroup;       //
};

struct PPStaffCal_ {       // @persistent @store(ReferenceTbl+)
	enum {
		fInherited       = 0x0001, // Календарь наследует элементы от родителя. То есть,
			// такой календарь не может содержать собственных элементов.
		fUseNominalPeriod = 0x0002  // При начислении зарплаты к календарю по умолчанию
			// применяется актуальный период начисления (не номинальный)
	};
	DECL_INVARIANT_C();

	PPID   Tag;            // Const=PPOBJ_STAFFCAL
	PPID   ID;             // @id
	char   Name[30];       // @name @!refname Для порожденных календарей (LinkCalID != 0) - пусто
	char   Symb[18];       // Символ. Для порожденных календарей (LinkCalID != 0) - пусто
	char   Reserve2[8];    // @reserve
	PPID   PersonKind;     // @v5.7.7 Вид персоналий, с которыми связываются (устанавливается в родительском календаре)
	PPID   SubstCalID;     // @v5.6.10 Календарь, замещающий родительский. Используется в том случае,
		// если родительский календарь является смысловым, а StubCalID - фактическим.
		// Пример: регулярный календарь (смысловой) замещается графиком для женщин (фактический)
	PPID   LinkObjType;    // Тип связанного объекта
	long   Flags;          //
	PPID   LinkCalID;      // ИД родительского календаря //
	PPID   LinkObjID;      // ИД связанного объекта
};

struct PPGoodsInfo_ {      // @persistent @store(ReferenceTbl+)
	long   ObjType;
	long   ObjID;
	char   Name[20];
	long   LocID;
	long   TouchScreenID;
	long   LabelPrinterID;
	long   Flags;
	char   Reserve[44];
};

struct PPScale_ {          // @persistent @store(ReferenceTbl+)
	int    SLAPI IsValidBcPrefix() const
	{
		return BIN((BcPrefix >= 200 && BcPrefix <= 299) || (BcPrefix >= 20 && BcPrefix <= 29));
	}
	long   Tag;            // Const=PPOBJ_SCALE
	long   ID;             // @id
	char   Name[30];       // @name @!refname
	//
	// System params @v3.2.9 {
	//
	uint16 Get_NumTries;   // @#{0..32000}
	uint16 Get_Delay;      // @#{0..1000}
	uint16 Put_NumTries;   // @#{0..32000}
	uint16 Put_Delay;      // @#{0..1000}
	// }
	int16  BcPrefix;       // @v5.8.6 Префикс искусственного штирхкода, загружаемого на весы
	char   Reserve1[4];    //
	PPID   QuotKindID;     // ->Ref(PPOBJ_QUOTKIND) Вид котировки, используемый для ценообразования //
	//
	// Если Flags & SCALF_TCPIP, то IP-адрес устройства упаковывается в
	// поле Port в виде: Port[0].Port[1].Port[2].Port[3]
	//
	char   Port[8];        // Порт обмена
	PPID   ScaleTypeID;    // Тип устройства
	long   ProtocolVer;    // Версия протокола обмена. Зависит от типа устройства
	long   LogNum;         // Логический номер устройства. Применяется для некоторых типов устройств.
	long   Flags;          // SCALF_XXX
	long   Location;       // ->Location.ID Склад, к которому относится устройство
	long   AltGoodsGrp;    // ->Goods2.ID   Альтернативная группа товаров, загружаемая на весы
};

struct PPBhtTerminal_ {    // @persistent @store(ReferenceTbl+)
	enum {
		fDelAfterImport = 0x00000001L
	};
	PPID   Tag;            // Const=PPOBJ_BHT
	PPID   ID;             // @id
	char   Name[30];       // @name @!refname
	short  ReceiptPlace;   // Куда качать накладные  @todo > Flags
	//
	// Параметры com-порта
	//
	uint16 ComGet_NumTries; // @#{0..32000} not used
	uint16 ComGet_Delay;    // @#{0..1000}  for Win32 only
	uint16 ComPut_NumTries; // @#{0..32000} not used
	uint16 ComPut_Delay;   // @#{0..1000}  not used
	char   Reserve1[6];    // @reserve
	PPID   IntrExpndOpID;  // Операция внутреннего перемещения (PPOPT_GOODSEXPEND || PPOPT_DRAFTEXPEND)
	PPID   InventOpID;     // Inventory Operation Kind ID
	char   Port[8];        // Output port name (default "COM1")
	int16  Cbr;            // ComBaudRate (default 19200)
	uint16 BhtpTimeout;    // Bht protocol timeout, mc (default 3000)
	uint16 BhtpMaxTries;   // Bht protocol max attempts sending data (default 10)
	long   Flags;          //
	long   BhtTypeID;      // Reserved (Denso only)
	PPID   ExpendOpID;     // Expend Operation Kind ID (PPOPT_GOODSEXPEND || PPOPT_DRAFTEXPEND)
};

struct PPDraftWrOff_ {     // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_DRAFTWROFF
	long   ID;             // @id
	char   Name[48];       // @name
	PPID   PoolOpID;       // Операция формирования пула документов списания //
	PPID   DfctCompensOpID; // Операция компенсации дефицита
	PPID   DfctCompensArID; // Контрагент в документах компенсации дефицита
	long   Reserve1[2];    // @reserve
	long   Flags;          // Флаги DWOF_XXX
	long   Reserve2[2];    // @reserve
};

struct PPAdvBillKind_ {    // @persistent @store(ReferenceTbl+)
	enum {
		fSkipAccturn = 0x0001 // Для строки расширенного бух документа с таким видом не проводит бух проводку
	};
	long   Tag;            // Const=PPOBJ_ADVBILLKIND
	long   ID;             // @id
	char   Name[48];       // @name
	PPID   LinkOpID;       // Операция связанного документа //
	long   Reserve1[4];    // @reserve
	long   Flags;          // Флаги PPAdvBillKind::fXXX
	long   Reserve2[2];    // @reserve
};

struct PPGoodsBasket_ {    // @persistent @store(ReferenceTbl+)
	long   Tag;            // Const=PPOBJ_GOODSBASKET
	long   ID;             // @id
	char   Name[30];       // @name @!refname
	long   Num;            // Внутренний номер (не используется, но инициализируется)
	long   Flags;          // GBASKF_XXX
	PPID   User;           // ->Ref(PPOBJ_USR) Пользователь, создавший корзину
	PPID   SupplID;        // ->Article.ID
	char   Reserve1[34];   // @reserve
};

struct PPDraftCreateRule_ { // @persistent @store(ReferenceTbl+)
	enum PriceAlgoritm {  // агоритм образования цены реализации
		pByLastLot   = 1, // из последнего лота
		pByAvgSum    = 2, // среднее по выборке чеков
		pByAvgSumDis = 3, // среднее по выборке чеков минус скидка
		pByQuot        = 4, // по котировке
		pByCostPctVal  = 5 // из цены поступления плюс некоторый процент
	};
    enum CostAlgoritm {      // алгоритм образования цены поступлени
		cByLastLot      = 1, // из последнего лота
		cByPricePctVal  = 2, // из цены реализации минус некоторый процент
		cByQuot         = 3  // по котировке
	};
	enum {
		fExclGoodsGrp = 0x0001,
		fIsRulesGroup = 0x0002
	};
	PPID   Tag;            // Const=PPOBJ_DFCREATERULE
	PPID   ID;             // @id
	char   Name[20];       // @offse(28) @name @!refname
	PPID   OpID;           // ->Ref(PPOBJ_OPRKIND)
	PPID   ArID;           // ->Article.ID
	PPID   AgentID;        // ->Article.ID
	PPID   GoodsGrpID;     // ->Goods2.ID
	PPID   CQuot;          // ->Ref(PPOBJ_QUOTKIND)
	PPID   PQuot;          // @offse(52) ->Ref(PPOBJ_QUOTKIND)
	int16  CostAlg;        //
	int16  PriceAlg;       // @offse(56)
	float  CPctVal;        //
	float  PPctVal;        // @offse(64)
	double MaxSum;         // @offse(72)
	long   MaxPos;         //
	int16  Flags;          //
	PPID   ParentID;       //
	char   Reserve[2];     // @offse(84)
	long   Reserve2;       // @offse(88) @v5.4.6
};

