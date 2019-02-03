// STYLOBHTII.H
// 2008, 2009, 2014, 2019
//
#ifndef __STYLOBHTII_H
#define __STYLOBHTII_H

#include <slib.h>
#include <dbf.h>

#define CFGSECTION_MAIN              "Main"

#define CFGINIPARAM_SIZE             "Size"
#define CFGINIPARAM_VER              "Version"
#define CFGINIPARAM_DEVICEID         "DeviceID"
#define CFGINIPARAM_DEVICENAME       "DeviceName"
#define CFGINIPARAM_FLAGS            "Flags"
#define CFGINIPARAM_WEIGHTPREFIX     "WeightPrefix"
#define CFGINIPARAM_USERNAME         "UserName"
#define CFGINIPARAM_PASSWORD         "Password"
#define CFGINIPARAM_DBSYMB           "DbSymb"
#define CFGINIPARAM_SERVERADDR       "ServerAddr"
#define CFGINIPARAM_SERVERPORT       "ServerPort"
#define CFGINIPARAM_SERVERMASK       "ServerMask"
#define CFGINIPARAM_DEFQTTY          "DefQuantity"
#define CFGINIPARAM_DOCNO            "DocNo"
#define CFGINIPARAM_LASTEXCH         "LastExchange"
#define CFGINIPARAM_GOODSLASTEXCH    "GoodsLastExchange"
#define CFGINIPARAM_ARTICLESLASTEXCH "ArticlesLastExchange"
#define CFGINIPARAM_BILLLSASTEXCH    "BillsLastExchange"

struct StyloBhtIIConfig { // @persistent size=872
	StyloBhtIIConfig();
	int ToHost();
	int ToDevice();
	int Save(const char * pPath);

	enum {
		fExitByEsc          = 0x00000001L,
		fUseWiFi            = 0x00000002L,
		fUseDefQtty         = 0x00000004L,
		fCheckQtty          = 0x00000008L,
		fCheckPrice         = 0x00000010L,
		fCheckExpiry        = 0x00000020L,
		fAllowUnknownGoods  = 0x00000040L,
		fInputOnlyQtty      = 0x00000080L,
		fInputBillRowNumber = 0x00000100L,
		fAdd1Qtty           = 0x00000200L,
		fAdoptSearching     = 0x00000400L
	};
	//
	// Ошибки при акцепте строки документа
	//
	enum {
		opebrowSerialNotFound = 0x00000001, // Серийный номер не найден
		opebrowGoodsNotFound  = 0x00000002, // Товар не найден (штрихкод)
		opebrowLotOfQtty      = 0x00000004, // Введенное кол-во товара > кол-ва товара из документа образца
		opebrowInvPrice       = 0x00000008  // Цена товара отличается от цены в документе образца больше разрешенных пределов
	};
	//
	// Ошибки при акцепте документа
	//
	enum {
		opebillSerialAbsence  = 0x00010000, // Отсутствует один из серийных номеров документа
		opebillGoodsAbsence   = 0x00020000, // Отсутствует один из товаров(штрихкодов) документа
		opebillLowQtty        = 0x00040000  // Введенное кол-во товара в документе < кол-ва товара из документа образца
	};
	//
	// Виды операций на терминале
	//
	enum {
		oprkExpend    = 1, // Расход
		oprkReceipt   = 2, // Приход
		oprkTransfer  = 3, // Передача
		oprkInventory = 4  // Инвентаризаци
	};
	//
	// Флаги операций
	//
	enum {
		foprCostAsPrice = 0x0001L,
		foprUseDueDate  = 0x0002L   // @v8.3.3
	};
	uint32 Size;           // Размер структуры
	uint32 Ver;            // 1 // Номер версии структуры. Начинается с 1.
	int32  DeviceID;
	char   DeviceName[32];
	int32  Flags;
	char   WeightPrefix[32];
	char   UserName[32];
	char   Password[128];
	char   DbSymb[64];
	uint32 ServerAddr;
	long   ServerPort;
	uint32 ServerMask;
	double DefQtty;
	int32  DocNo;
	LDATETIME LastExchange;
	LDATETIME GoodsLastExch;
	LDATETIME ArticleLastExch;
	LDATETIME BillLastExch;
	int32  RngLimWgtGoods;     // Предел изменения для весового товара
	int32  RngLimPrice;        // Предел изменения цены
	char   QttyWeightPrefix[32]; // Префикс весового счетного товара
	int16  SockDelayRd;        // @v8.4.2 (2)
	int16  ReserveSdr;         // @v8.4.2 (2) @alignment
	int32  LocID;              // @v8.4.2 Склад, указанный в записи терминала
	char   Reserve[464];       // @reserve
};

#define MAXRECS_IN_PACKBUF 128 // @v8.4.5 500-->128

struct SBhtIICmdBuf { // @persistent
	//
	// При добавлении команды, добавить ее текстовое выражение в TXT_TCPCOMMANDS на терминале
	//
	enum { // @persistent
		cmCheckConnection      = 1,
		cmGetConfig            = 2,
		cmGetGoods             = 3,
		cmGetArticles          = 4,
		cmGetBills             = 5,
		cmGetBillRows          = 6,
		cmGetOpRestrictions    = 7,
		cmFindGoods            = 8,
		cmFindArticle          = 9,
		cmSetConfig            = 10,
		cmSetBills             = 11,
		cmSetBillRows          = 12,
		cmLogout               = 13,
		cmPrintBarcode         = 14,
		cmPrepareBills         = 15,
		cmFindLocCell          = 16,
		cmAcceptLocOp          = 17,
		cmCellListByGoods      = 18,
		cmGoodsListByCell      = 19,
		cmGetBillRowsWithCells = 20,

		cmSearchGoodsByCode    = 21, // @v8.4.2 Возвращает ИД товара по заданному коду
		cmGetGoodsInfo         = 22, // @v8.4.2 Возвращает информацию о товаре по ИД
		cmGetGoodsRecord       = 23, // @v8.4.2 Возвращает запись товара по ИД
		cmTestConnection       = 24, // @v8.4.2 Команда тестирования обмена между ТСД и сервером
			// (не путать с cmCheckConnection, используемой для регулярной проверки соединения)

		cmNextTableChunkBias   = 1000 // Специальное смещение для команд, возвращающих таблицы - при таком смещении
			// запрашивается следующая порция записей, начиная с параметра.
	};
	//
	// При добавлении команды, добавить ее текстовое выражение в TXT_TCPCOMMANDS на терминале
	//
	int16  Cmd;
	int16  Reserve;
	int32  RetCode;
	uint32 BufSize;
};
//
// Descr: Информация о текущем состоянии товара
//
struct SBIIGoodsStateInfo { // @persistent (сетевой обмен с ТСД)
	SBIIGoodsStateInfo();
	enum {
		fMatrix = 0x0001
	};
	int32  ID;
	char   Barcode[16];
	char   Serial[16];
	char   Name[128];
	long   Flags;
	LDATE  LastLotDate;
	LDATE  LastLotExpiry;
	double Pack;
	double Cost;
	double Price;
	double Rest;
	uint8  Reserve[64];
};

struct SBIIRec {
	virtual int FromBuf(const void * pBuf) {return -1;}
	virtual int ToBuf(void * pBuf, size_t * pBufSize) {return -1;}
	virtual DbfTable * CreateDbfTbl(const char * pPath) {return 0;}
	virtual int FromDbfTbl(DbfTable * pTbl) {return -1;}
	virtual int ToDbfTbl(DbfTable * pTbl) {return -1;}
	virtual size_t GetSize() const { return -1; }
};

struct SBIIGoodsRec : public SBIIRec { // size = 132
	SBIIGoodsRec();
	void Init();
	virtual size_t GetSize() const { return 132; }
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  ID;          // 4
	char   Barcode[16]; // 20
	char   Serial[16];  // 36
	char   Name[64];    // 100
	double Pack;        // 108
	double Price;       // 116
	double Rest;        // 124
	double Cost;        // 132
};

struct SBIIArticleRec : public SBIIRec { // size = 56
	virtual size_t GetSize() const { return 56; }
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32 ID;
	int32 AccSheetID;
	char  Name[48];
};

struct SBIISampleBillRec : public SBIIRec { // size = 32
	virtual size_t GetSize() const { return 32; }
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32 ID;
	LDATE Dt;
	int32 ArticleID;
	int32 OpID;
	char  Code[16];
};

struct SBIISampleBillRowRec : public SBIIRec { // size = 56
	virtual size_t GetSize() const { return 56; }
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  BillID;
	int32  GoodsID;
	char   Serial[16];
	double Qtty;
	double Cost;
	int32  RByBill;
	double SupplDeal;
	int16  SupplDealLow;
	int16  SupplDealUp;
};

struct SBIIOpRestrRec : public SBIIRec { // size = 24
	virtual size_t GetSize() const { return 24; }
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32 ID;
	int32 AccSheetID;
	int32 OnBhtOpID;
	int32 OkCancelActions;
	int32 CfmActions;
	int32 Flags;
};

struct SBIIBillRec : public SBIIRec { // size = 56
	virtual size_t GetSize() const { return 56; }
	virtual DbfTable * CreateDbfTbl(const char * pPath);
	virtual int ToDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  ID;
	int32  SampleBillID;
	LDATE  Dt;
	int32  OpID;
	GUID   Uuid;
	LTIME  Tm;
	int32  ArticleID;
	char   Code[16];
	double DefQtty;
};

struct SBIIBillRowRec : public SBIIRec { // size = 56
	virtual size_t GetSize() const { return 56; }
	virtual DbfTable * CreateDbfTbl(const char * pPath);
	virtual int ToDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  BillID;
	int32  GoodsID;
	int32  StoragePlace;
	int32  Number;
	char   Serial[16];
	LTIME  Tm;
	LDATE  Expiry;
	double Qtty;
	double Cost;
};

struct SBIILocCellRec : public SBIIRec { // size = 76
	virtual size_t GetSize() const { return 76; }
	SBIILocCellRec();
	void Init();
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  ID;
	char   Code[16];
	char   Name[48];
	double Qtty;
};

struct SBIILocOp : public SBIIRec { // size = 28
	virtual size_t GetSize() const { return 28; }
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  BillID;
	int32  RByBill;
	int32  GoodsID;
	int32  LocCellID;
	int32  Op;
	double Qtty;
};

struct SBIIBillRowWithCellsRec : public SBIIRec { // size = 96
	virtual size_t GetSize() const { return 96; }
	virtual int FromDbfTbl(DbfTable * pTbl);
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual int FromBuf(const void * pBuf);

	int32  BillID;
	int32  GoodsID;
	char   Serial[16];
	double Qtty;
	double Cost;
	int16  RByBill;
	int16  Expended;
	char   Name[48];
	int32  LocID;
};

#endif
