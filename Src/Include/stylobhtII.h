// STYLOBHTII.H
// 2008, 2009, 2014
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
	StyloBhtIIConfig()
	{
		THISZERO();
		Ver = 1;
		Size = sizeof(StyloBhtIIConfig);
	}
	int ToHost()
	{
		return 1;
	}
	int ToDevice()
	{
		return 1;
	}
	int Save(const char * pPath)
	{
		int ok = 0;
		/* Класс SIniFile требует адаптации под WinCe
		SString buf;
		SIniFile file(path, 1, 1);
		if(file.Valid()) {
			(buf = 0).Cat(pCfg->Size);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_SIZE, buf, 1);
			(buf = 0).Cat(pCfg->Ver);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_VER, buf, 1);
			(buf = 0).Cat(ID);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_DEVICEID, buf, 1);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_DEVICENAME, DeviceName, 1);
			(buf = 0).Cat(Flags);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_FLAGS, buf, 1);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_WEIGHTPREFIX, WeightPrefix, 1);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_USERNAME, UserName, 1);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_PASSWORD, Password, 1);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_DBSYMB, DbSymb, 1);
			InetAddr::UlongToIP(pCfg->ServerAddr, buf = 0);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_SERVERADDR, buf, 1);
			(buf = 0).Cat(pCfg->ServerPort);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_SERVERPORT, buf, 1);
			InetAddr::UlongToIP(pCfg->ServerMask, buf = 0);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_SERVERMASK, buf, 1);
			(buf = 0).Cat(pCfg->DefQtty);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_DEFQTTY, buf, 1);
			(buf = 0).Cat(pCfg->DocNo);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_DOCNO, buf, 1);
			(buf = 0).Cat(pCfg->LastExchange);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_LASTEXCH, buf, 1);
			(buf = 0).Cat(pCfg->GoodsLastExch;);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_GOODSLASTEXCH, buf, 1);
			(buf = 0).Cat(pCfg->ArticleLastExch);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_ARTICLESLASTEXCH, buf, 1);
			(buf = 0).Cat(pCfg->BillLastExch);
			file.AppendParam(CFGSECTION_MAIN, CFGINIPARAM_BILLSLASTEXCH, buf, 1);
			ok = 1;
		}
		*/
		return ok;
	}
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
	SBIIGoodsStateInfo()
	{
		THISZERO();
	}
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
	virtual size_t GetSize() {return -1;}
};

struct SBIIGoodsRec : public SBIIRec { // size = 132
	SBIIGoodsRec()
	{
		Init();
	}
	void Init()
	{
		ID = 0;
		memzero(Barcode, sizeof(Barcode));
		memzero(Serial, sizeof(Serial));
		memzero(Name, sizeof(Name));
		Pack  = 0;
		Price = 0;
		Rest  = 0;
		Cost  = 0;
	}
	virtual size_t GetSize()
	{
		return 132;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int    fldn_id = 0;
			int    fldn_code = 0;
			int    fldn_name = 0;
			int    fldn_upp = 0;
			int    fldn_price = 0;
			int    fldn_rest = 0;
			int    fldn_cost = 0;
			SString name, barcode;
			pTbl->getFieldNumber("ID",    &fldn_id);
			pTbl->getFieldNumber("CODE",  &fldn_code);
			pTbl->getFieldNumber("NAME",  &fldn_name);
			pTbl->getFieldNumber("UPP",   &fldn_upp);
			pTbl->getFieldNumber("PRICE", &fldn_price);
			pTbl->getFieldNumber("REST",  &fldn_rest);
			pTbl->getFieldNumber("COST",  &fldn_cost);

			DbfRecord dbf_rec(pTbl);

			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_id,    ID);
			dbf_rec.get(fldn_code,  barcode);
			dbf_rec.get(fldn_name,  name);
			dbf_rec.get(fldn_upp,   Pack);
			dbf_rec.get(fldn_price, Price);
			dbf_rec.get(fldn_rest,  Rest);
			dbf_rec.get(fldn_cost,  Cost);
			name.Strip().CopyTo(Name, sizeof(Name));
			barcode.Strip().CopyTo(Barcode, sizeof(Barcode));
			MEMSZERO(Serial);
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int    ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;
				memcpy(p_buf + bytes,                      &ID,     sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),      Barcode, sizeof(Barcode));
				memcpy(p_buf + (bytes += sizeof(Barcode)), Serial,  sizeof(Serial));
				memcpy(p_buf + (bytes += sizeof(Serial)),  Name,    sizeof(Name));
				memcpy(p_buf + (bytes += sizeof(Name)),    &Pack,   sizeof(Pack));
				memcpy(p_buf + (bytes += sizeof(Pack)),    &Price,  sizeof(Price));
				memcpy(p_buf + (bytes += sizeof(Price)),   &Rest,   sizeof(Rest));
				memcpy(p_buf + (bytes += sizeof(Rest)),    &Cost,   sizeof(Cost));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,     p_buf + bytes,                      sizeof(ID));
		memcpy(Barcode, p_buf + (bytes += sizeof(ID)),      sizeof(Barcode));
		memcpy(Serial,  p_buf + (bytes += sizeof(Barcode)), sizeof(Serial));
		memcpy(Name,    p_buf + (bytes += sizeof(Serial)),  sizeof(Name));
		memcpy(&Pack,   p_buf + (bytes += sizeof(Name)),    sizeof(Pack));
		memcpy(&Price,  p_buf + (bytes += sizeof(Pack)),    sizeof(Price));
		memcpy(&Rest,   p_buf + (bytes += sizeof(Price)),   sizeof(Rest));
		memcpy(&Cost,   p_buf + (bytes += sizeof(Rest)),    sizeof(Cost));
		return 1;
	}
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
	virtual size_t GetSize()
	{
		return 56;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int fldn_id = 0, fldn_accsheet = 0, fldn_name = 0;
			SString    name;
			pTbl->getFieldNumber("ID",        &fldn_id);
			pTbl->getFieldNumber("ACCSHEET",  &fldn_accsheet);
			pTbl->getFieldNumber("NAME",      &fldn_name);

			DbfRecord  dbf_rec(pTbl);

			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_id,        ID);
			dbf_rec.get(fldn_accsheet,  AccSheetID);
			dbf_rec.get(fldn_name,      name);
			name.Strip().CopyTo(Name, sizeof(Name));
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                         &ID,         sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),         &AccSheetID, sizeof(AccSheetID));
				memcpy(p_buf + (bytes += sizeof(AccSheetID)), Name,        sizeof(Name));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,         p_buf + bytes,                         sizeof(ID));
		memcpy(&AccSheetID, p_buf + (bytes += sizeof(ID)),         sizeof(AccSheetID));
		memcpy(Name,        p_buf + (bytes += sizeof(AccSheetID)), sizeof(Name));
		return 1;
	}
	int32 ID;
	int32 AccSheetID;
	char  Name[48];
};

struct SBIISampleBillRec : public SBIIRec { // size = 32
	virtual size_t GetSize()
	{
		return 32;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int fldn_id = 0, fldn_dt = 0, fldn_article = 0, fldn_opid = 0, fldn_code = 0;
			SString code;
			pTbl->getFieldNumber("ID",      &fldn_id);
			pTbl->getFieldNumber("DATE",    &fldn_dt);
			pTbl->getFieldNumber("ARTICLE", &fldn_article);
			pTbl->getFieldNumber("OPID",    &fldn_opid);
			pTbl->getFieldNumber("CODE",    &fldn_code);

			DbfRecord  dbf_rec(pTbl);
			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_id,      ID);
			dbf_rec.get(fldn_dt,      Dt);
			dbf_rec.get(fldn_article, ArticleID);
			dbf_rec.get(fldn_opid,    OpID);
			dbf_rec.get(fldn_code,    code);
			code.Strip().CopyTo(Code, sizeof(Code));
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;
				memcpy(p_buf + bytes, &ID,        sizeof(ID)); bytes += sizeof(ID);
				memcpy(p_buf + bytes, &Dt,        sizeof(Dt)); bytes += sizeof(Dt);
				memcpy(p_buf + bytes, &ArticleID, sizeof(ArticleID)); bytes += sizeof(ArticleID);
				memcpy(p_buf + bytes, &OpID,      sizeof(OpID)); bytes += sizeof(OpID);
				memcpy(p_buf + bytes, Code,       sizeof(Code)); bytes += sizeof(Code);
				assert(bytes == GetSize());
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,        p_buf + bytes, sizeof(ID)); bytes += sizeof(ID);
		memcpy(&Dt,        p_buf + bytes, sizeof(Dt)); bytes += sizeof(Dt);
		memcpy(&ArticleID, p_buf + bytes, sizeof(ArticleID)); bytes += sizeof(ArticleID);
		memcpy(&OpID,      p_buf + bytes, sizeof(OpID)); bytes += sizeof(OpID);
		memcpy(Code,       p_buf + bytes, sizeof(Code)); bytes += sizeof(Code);
		assert(bytes == GetSize());
		return 1;
	}
	int32 ID;
	LDATE Dt;
	int32 ArticleID;
	int32 OpID;
	char  Code[16];
};

struct SBIISampleBillRowRec : public SBIIRec { // size = 56
	virtual size_t GetSize()
	{
		return 56;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int fldn_billid = 0, fldn_goods = 0, fldn_serial = 0, fldn_qtty = 0, fldn_cost = 0, fldn_rbybill = 0;
			int fldn_supdeal = 0, fldn_supdealup = 0, fldn_supdeallow = 0;
			SString serial;
			pTbl->getFieldNumber("BILLID",   &fldn_billid);
			pTbl->getFieldNumber("GOODSID",  &fldn_goods);
			pTbl->getFieldNumber("SERIAL",   &fldn_serial);
			pTbl->getFieldNumber("QTTY",     &fldn_qtty);
			pTbl->getFieldNumber("COST",     &fldn_cost);
			pTbl->getFieldNumber("RBYBILL",  &fldn_rbybill);
			pTbl->getFieldNumber("SUPLDEAL", &fldn_supdeal);
			pTbl->getFieldNumber("SUPLDLOW", &fldn_supdeallow);
			pTbl->getFieldNumber("SUPLDUP",  &fldn_supdealup);

			DbfRecord dbf_rec(pTbl);
			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_billid,       BillID);
			dbf_rec.get(fldn_goods,        GoodsID);
			dbf_rec.get(fldn_serial,       serial);
			dbf_rec.get(fldn_qtty,         Qtty);
			dbf_rec.get(fldn_cost,         Cost);
			dbf_rec.get(fldn_rbybill,      RByBill);
			dbf_rec.get(fldn_supdeal,      SupplDeal);
			{
				long v = 0;
				dbf_rec.get(fldn_supdeallow, v);
				SupplDealLow = (int16)v;
				dbf_rec.get(fldn_supdealup,  v);
				SupplDealUp  = (int16)v;
			}
			serial.Strip().CopyTo(Serial, sizeof(Serial));
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;
				memcpy(p_buf + bytes, &BillID,        sizeof(BillID));       bytes += sizeof(BillID);
				memcpy(p_buf + bytes, &GoodsID,       sizeof(GoodsID));      bytes += sizeof(GoodsID);
				memcpy(p_buf + bytes, Serial,         sizeof(Serial));       bytes += sizeof(Serial);
				memcpy(p_buf + bytes, &Qtty,          sizeof(Qtty));         bytes += sizeof(Qtty);
				memcpy(p_buf + bytes, &Cost,          sizeof(Cost));         bytes += sizeof(Cost);
				memcpy(p_buf + bytes, &RByBill,       sizeof(RByBill));      bytes += sizeof(RByBill);
				memcpy(p_buf + bytes, &SupplDeal,     sizeof(SupplDeal));    bytes += sizeof(SupplDeal);
				memcpy(p_buf + bytes, &SupplDealLow,  sizeof(SupplDealLow)); bytes += sizeof(SupplDealLow);
				memcpy(p_buf + bytes, &SupplDealUp,   sizeof(SupplDealUp));  bytes += sizeof(SupplDealUp);
				assert(bytes == GetSize());
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&BillID,        p_buf + bytes, sizeof(BillID)); bytes += sizeof(BillID);
		memcpy(&GoodsID,       p_buf + bytes, sizeof(GoodsID)); bytes += sizeof(GoodsID);
		memcpy(Serial,         p_buf + bytes, sizeof(Serial)); bytes += sizeof(Serial);
		memcpy(&Qtty,          p_buf + bytes, sizeof(Qtty)); bytes += sizeof(Qtty);
		memcpy(&Cost,          p_buf + bytes, sizeof(Cost)); bytes += sizeof(Cost);
		memcpy(&RByBill,       p_buf + bytes, sizeof(RByBill)); bytes += sizeof(RByBill);
		memcpy(&SupplDeal,     p_buf + bytes, sizeof(SupplDeal)); bytes += sizeof(SupplDeal);
		memcpy(&SupplDealLow,  p_buf + bytes, sizeof(SupplDeal)); bytes += sizeof(SupplDealLow);
		memcpy(&SupplDealUp,   p_buf + bytes, sizeof(SupplDealUp)); bytes += sizeof(SupplDealUp);
		assert(bytes == GetSize());
		return 1;
	}
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
	virtual size_t GetSize()
	{
		return 24;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int fldn_id = 0, fldn_accsheet = 0, fldn_tobhtop = 0, fldn_bhtokca = 0, fldn_bhtcfma = 0, fldn_flags = 0;
			pTbl->getFieldNumber("OPID",     &fldn_id);
			pTbl->getFieldNumber("ACCSHEET", &fldn_accsheet);
			pTbl->getFieldNumber("TOBHTOP",  &fldn_tobhtop);
			pTbl->getFieldNumber("BHTOKCA",  &fldn_bhtokca);
			pTbl->getFieldNumber("BHTCFMA",  &fldn_bhtcfma);
			pTbl->getFieldNumber("FLAGS",    &fldn_flags);

			DbfRecord dbf_rec(pTbl);
			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_id,        ID);
			dbf_rec.get(fldn_accsheet,  AccSheetID);
			dbf_rec.get(fldn_tobhtop,   OnBhtOpID);
			dbf_rec.get(fldn_bhtokca,   OkCancelActions);
			dbf_rec.get(fldn_bhtcfma,   CfmActions);
			dbf_rec.get(fldn_flags,     Flags);
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                              &ID,              sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),              &AccSheetID,      sizeof(AccSheetID));
				memcpy(p_buf + (bytes += sizeof(AccSheetID)),      &OnBhtOpID,       sizeof(OnBhtOpID));
				memcpy(p_buf + (bytes += sizeof(OnBhtOpID)),       &OkCancelActions, sizeof(OkCancelActions));
				memcpy(p_buf + (bytes += sizeof(OkCancelActions)), &CfmActions,      sizeof(CfmActions));
				memcpy(p_buf + (bytes += sizeof(CfmActions)),      &Flags,           sizeof(Flags));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,              p_buf + bytes,                              sizeof(ID));
		memcpy(&AccSheetID,      p_buf + (bytes += sizeof(ID)),              sizeof(AccSheetID));
		memcpy(&OnBhtOpID,       p_buf + (bytes += sizeof(AccSheetID)),      sizeof(OnBhtOpID));
		memcpy(&OkCancelActions, p_buf + (bytes += sizeof(OnBhtOpID)),       sizeof(OkCancelActions));
		memcpy(&CfmActions,      p_buf + (bytes += sizeof(OkCancelActions)), sizeof(CfmActions));
		memcpy(&Flags,           p_buf + (bytes += sizeof(CfmActions)),      sizeof(Flags));
		return 1;
	}
	int32 ID;
	int32 AccSheetID;
	int32 OnBhtOpID;
	int32 OkCancelActions;
	int32 CfmActions;
	int32 Flags;
};

struct SBIIBillRec : public SBIIRec { // size = 56
	virtual size_t GetSize()
	{
		return 56;
	}
	virtual DbfTable * CreateDbfTbl(const char * pPath)
	{
		DbfTable * p_tbl = 0;
		if(p_tbl = new DbfTable(pPath)) {
			int    num_flds = 0;
			DBFCreateFld fld_list[32];

			fld_list[num_flds++].Init("ID",       'N', 10, 0);
			fld_list[num_flds++].Init("SAMPLEID", 'N', 10, 0);
			fld_list[num_flds++].Init("DATE",     'D',  8, 0);
			fld_list[num_flds++].Init("TIME",     'C', 14, 0);
			fld_list[num_flds++].Init("ARTICLE",  'N', 10, 0);
			fld_list[num_flds++].Init("OPID",     'N', 10, 0);
			fld_list[num_flds++].Init("CODE",     'C', 16, 0);
			fld_list[num_flds++].Init("GUID",     'C', 38, 0);
			if(!p_tbl->create(num_flds, fld_list) || !p_tbl->open())
				ZDELETE(p_tbl);
		}
		return p_tbl;
	}
	virtual int ToDbfTbl(DbfTable * pTbl)
	{
		int ok = 0;
		if(pTbl) {
			S_GUID guid;
			SString str_guid, str_tm;
			DbfRecord dbf_rec(pTbl);

			str_tm.Cat(Tm, TIMF_HMS);
			guid.Init(Uuid);
			guid.ToStr(S_GUID::fmtIDL, str_guid);
			dbf_rec.put(1, ID);
			dbf_rec.put(2, SampleBillID);
			dbf_rec.put(3, Dt);
			dbf_rec.put(4, str_tm);
			dbf_rec.put(5, ArticleID);
			dbf_rec.put(6, OpID);
			dbf_rec.put(7, Code);
			dbf_rec.put(8, (const char*)str_guid);
			ok = pTbl->appendRec(&dbf_rec);
		}
		return ok;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                             &ID,           sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),             &SampleBillID, sizeof(SampleBillID));
				memcpy(p_buf + (bytes += sizeof(SampleBillID)),   &Dt,           sizeof(Dt));
				memcpy(p_buf + (bytes += sizeof(Dt)),             &OpID,         sizeof(OpID));
				memcpy(p_buf + (bytes += sizeof(OpID)),           &Uuid,         sizeof(Uuid));
				memcpy(p_buf + (bytes += sizeof(Uuid)),           &Tm,           sizeof(Tm));
				memcpy(p_buf + (bytes += sizeof(Tm)),             &ArticleID,    sizeof(ArticleID));
				memcpy(p_buf + (bytes += sizeof(ArticleID)),      Code,          sizeof(Code));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,           p_buf + bytes,                           sizeof(ID));
		memcpy(&SampleBillID, p_buf + (bytes += sizeof(ID)),           sizeof(SampleBillID));
		memcpy(&Dt,           p_buf + (bytes += sizeof(SampleBillID)), sizeof(Dt));
		memcpy(&OpID,         p_buf + (bytes += sizeof(Dt)),           sizeof(OpID));
		memcpy(&Uuid,         p_buf + (bytes += sizeof(OpID)),         sizeof(Uuid));
		memcpy(&Tm,           p_buf + (bytes += sizeof(Uuid)),         sizeof(Tm));
		memcpy(&ArticleID,    p_buf + (bytes += sizeof(Tm)),           sizeof(ArticleID));
		memcpy(Code,          p_buf + (bytes += sizeof(ArticleID)),    sizeof(Code));
		return 1;
	}
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
	virtual size_t GetSize()
	{
		return 56;
	}
	virtual DbfTable * CreateDbfTbl(const char * pPath)
	{
		DbfTable * p_tbl = 0;
		if(p_tbl = new DbfTable(pPath)) {
			int    num_flds = 0;
			DBFCreateFld fld_list[32];

			fld_list[num_flds++].Init("BILLID",   'N', 10, 0);
			fld_list[num_flds++].Init("GOODSID",  'N', 10, 0);
			fld_list[num_flds++].Init("STORAGEP", 'N', 10, 0);
			fld_list[num_flds++].Init("NUMBER",   'N', 10, 0);
			fld_list[num_flds++].Init("SERIAL",   'C', 16, 0);
			fld_list[num_flds++].Init("EXPIRY",   'D',  8, 0);
			fld_list[num_flds++].Init("TIME",     'C', 14, 0);
			fld_list[num_flds++].Init("QTTY",     'N', 10, 3);
			fld_list[num_flds++].Init("COST",     'N', 10, 2);
			if(!p_tbl->create(num_flds, fld_list) || !p_tbl->open())
				ZDELETE(p_tbl);
		}
		return p_tbl;
	}
	virtual int ToDbfTbl(DbfTable * pTbl)
	{
		int ok = 0;
		if(pTbl) {
			SString str_tm;
			DbfRecord dbf_rec(pTbl);

			str_tm.Cat(Tm, TIMF_HMS);
			dbf_rec.put(1, BillID);
			dbf_rec.put(2, GoodsID);
			dbf_rec.put(3, StoragePlace);
			dbf_rec.put(4, Number);
			dbf_rec.put(5, Serial);
			dbf_rec.put(6, Expiry);
			dbf_rec.put(7, str_tm);
			dbf_rec.put(8, Qtty);
			dbf_rec.put(9, Cost);
			ok = pTbl->appendRec(&dbf_rec);
		}
		return ok;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                           &BillID,       sizeof(BillID));
				memcpy(p_buf + (bytes += sizeof(BillID)),       &GoodsID,      sizeof(GoodsID));
				memcpy(p_buf + (bytes += sizeof(GoodsID)),      &StoragePlace, sizeof(StoragePlace));
				memcpy(p_buf + (bytes += sizeof(StoragePlace)), &Number,       sizeof(Number));
				memcpy(p_buf + (bytes += sizeof(Number)),       Serial,        sizeof(Serial));
				memcpy(p_buf + (bytes += sizeof(Serial)),       &Tm,           sizeof(Tm));
				memcpy(p_buf + (bytes += sizeof(Tm)),           &Expiry,       sizeof(Expiry));
				memcpy(p_buf + (bytes += sizeof(Expiry)),       &Qtty,         sizeof(Qtty));
				memcpy(p_buf + (bytes += sizeof(Qtty)),         &Cost,         sizeof(Cost));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&BillID,       p_buf + bytes,                           sizeof(BillID));
		memcpy(&GoodsID,      p_buf + (bytes += sizeof(BillID)),       sizeof(GoodsID));
		memcpy(&StoragePlace, p_buf + (bytes += sizeof(GoodsID)),      sizeof(StoragePlace));
		memcpy(&Number,       p_buf + (bytes += sizeof(StoragePlace)), sizeof(Number));
		memcpy(Serial,        p_buf + (bytes += sizeof(Number)),       sizeof(Serial));
		memcpy(&Tm,           p_buf + (bytes += sizeof(Serial)),       sizeof(Tm));
		memcpy(&Expiry,       p_buf + (bytes += sizeof(Tm)),           sizeof(Expiry));
		memcpy(&Qtty,         p_buf + (bytes += sizeof(Expiry)),       sizeof(Qtty));
		memcpy(&Cost,         p_buf + (bytes += sizeof(Qtty)),         sizeof(Cost));
		return 1;
	}
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
	SBIILocCellRec() {Init();}
	void Init()
	{
		ID   = 0;
		Qtty = 0;
		memzero(Code, sizeof(Code));
		memzero(Name, sizeof(Name));
	}
	virtual size_t GetSize()
	{
		return 76;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int fldn_id = 0, fldn_code = 0, fldn_name = 0, fldn_qtty = 0;
			SString name, code;
			pTbl->getFieldNumber("ID",    &fldn_id);
			pTbl->getFieldNumber("CODE",  &fldn_code);
			pTbl->getFieldNumber("NAME",  &fldn_name);
			pTbl->getFieldNumber("QTTY",  &fldn_qtty);

			DbfRecord dbf_rec(pTbl);

			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_id,    ID);
			dbf_rec.get(fldn_code,  code);
			dbf_rec.get(fldn_name,  name);
			dbf_rec.get(fldn_qtty,  Qtty);
			name.Strip().CopyTo(Name, sizeof(Name));
			code.Strip().CopyTo(Code, sizeof(Code));
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                   &ID,   sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),   Code,  sizeof(Code));
				memcpy(p_buf + (bytes += sizeof(Code)), Name,  sizeof(Name));
				memcpy(p_buf + (bytes += sizeof(Name)), &Qtty, sizeof(Qtty));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,     p_buf + bytes,                      sizeof(ID));
		memcpy(Code,    p_buf + (bytes += sizeof(ID)),      sizeof(Code));
		memcpy(Name,    p_buf + (bytes += sizeof(Code)),    sizeof(Name));
		memcpy(&Qtty,   p_buf + (bytes += sizeof(Name)),    sizeof(Qtty));
		return 1;
	}
	int32  ID;
	char   Code[16];
	char   Name[48];
	double Qtty;
};

struct SBIILocOp : public SBIIRec { // size = 28
	virtual size_t GetSize()
	{
		return 28;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                           &BillID,      sizeof(BillID));
				memcpy(p_buf + (bytes += sizeof(BillID)),       &RByBill,     sizeof(RByBill));
				memcpy(p_buf + (bytes += sizeof(RByBill)),      &GoodsID,     sizeof(GoodsID));
				memcpy(p_buf + (bytes += sizeof(GoodsID)),      &LocCellID,   sizeof(LocCellID));
				memcpy(p_buf + (bytes += sizeof(LocCellID)),    &Op,          sizeof(Op));
				memcpy(p_buf + (bytes += sizeof(Op)),           &Qtty,        sizeof(Qtty));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;

		memcpy(&BillID,    p_buf + bytes,                        sizeof(BillID));
		memcpy(&RByBill,   p_buf + (bytes += sizeof(BillID)),    sizeof(RByBill));
		memcpy(&GoodsID,   p_buf + (bytes += sizeof(RByBill)),   sizeof(GoodsID));
		memcpy(&LocCellID, p_buf + (bytes += sizeof(GoodsID)),   sizeof(LocCellID));
		memcpy(&Op,        p_buf + (bytes += sizeof(LocCellID)), sizeof(Op));
		memcpy(&Qtty,      p_buf + (bytes += sizeof(Op)),        sizeof(Qtty));

		return 1;
	}
	int32  BillID;
	int32  RByBill;
	int32  GoodsID;
	int32  LocCellID;
	int32  Op;
	double Qtty;
};


struct SBIIBillRowWithCellsRec : public SBIIRec { // size = 96
	virtual size_t GetSize()
	{
		return 96;
	}
	virtual int FromDbfTbl(DbfTable * pTbl)
	{
		if(pTbl) {
			int fldn_billid = 0, fldn_goods = 0, fldn_serial = 0, fldn_qtty = 0, fldn_cost = 0,
				fldn_rbybill = 0, fldn_name = 0, fldn_loc = 0, fldn_expended = 0;
			SString serial, name;

			pTbl->getFieldNumber("BILLID",   &fldn_billid);
			pTbl->getFieldNumber("GOODSID",  &fldn_goods);
			pTbl->getFieldNumber("SERIAL",   &fldn_serial);
			pTbl->getFieldNumber("QTTY",     &fldn_qtty);
			pTbl->getFieldNumber("COST",     &fldn_cost);
			pTbl->getFieldNumber("RBYBILL",  &fldn_rbybill);
			pTbl->getFieldNumber("EXPENDED", &fldn_expended);
			pTbl->getFieldNumber("NAME",     &fldn_name);
			pTbl->getFieldNumber("LOCID",    &fldn_loc);

			DbfRecord dbf_rec(pTbl);
			pTbl->getRec(&dbf_rec);
			dbf_rec.get(fldn_billid,   BillID);
			dbf_rec.get(fldn_goods,    GoodsID);
			dbf_rec.get(fldn_serial,   serial);
			dbf_rec.get(fldn_qtty,     Qtty);
			dbf_rec.get(fldn_cost,     Cost);
			{
				long v = 0;
				dbf_rec.get(fldn_rbybill,  v);
				RByBill = (int16)v;
				dbf_rec.get(fldn_expended, (v = 0));
				Expended = (int16)v;
			}
			dbf_rec.get(fldn_name,     name);
			dbf_rec.get(fldn_loc,      LocID);
			serial.Strip().CopyTo(Serial, sizeof(Serial));
			name.Strip((LocID == 0) ? 0 : 1).CopyTo(Name, sizeof(Name));
		}
		return 1;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < GetSize()) {
				*pBufSize = GetSize();
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                       &BillID,   sizeof(BillID));
				memcpy(p_buf + (bytes += sizeof(BillID)),   &GoodsID,  sizeof(GoodsID));
				memcpy(p_buf + (bytes += sizeof(GoodsID)),  Serial,    sizeof(Serial));
				memcpy(p_buf + (bytes += sizeof(Serial)),   &Qtty,     sizeof(Qtty));
				memcpy(p_buf + (bytes += sizeof(Qtty)),     &Cost,     sizeof(Cost));
				memcpy(p_buf + (bytes += sizeof(Cost)),     &RByBill,  sizeof(RByBill));
				memcpy(p_buf + (bytes += sizeof(RByBill)),  &Expended, sizeof(Expended));
				memcpy(p_buf + (bytes += sizeof(Expended)),  Name,     sizeof(Name));
				memcpy(p_buf + (bytes += sizeof(Name)),     &LocID,    sizeof(LocID));
				*pBufSize = GetSize();
				ok = 1;
			}
		}
		return ok;
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&BillID,   p_buf + bytes,                       sizeof(BillID));
		memcpy(&GoodsID,  p_buf + (bytes += sizeof(BillID)),   sizeof(GoodsID));
		memcpy(Serial,    p_buf + (bytes += sizeof(GoodsID)),  sizeof(Serial));
		memcpy(&Qtty,     p_buf + (bytes += sizeof(Serial)),   sizeof(Qtty));
		memcpy(&Cost,     p_buf + (bytes += sizeof(Qtty)),     sizeof(Cost));
		memcpy(&RByBill,  p_buf + (bytes += sizeof(Cost)),     sizeof(RByBill));
		memcpy(&Expended, p_buf + (bytes += sizeof(RByBill)),  sizeof(Expended));
		memcpy(Name,      p_buf + (bytes += sizeof(Expended)), sizeof(Name));
		memcpy(&LocID,    p_buf + (bytes += sizeof(Name)),     sizeof(LocID));
		return 1;
	}

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
