// STYLOPALM.H
// Copyright (c) A.Sobolev 2005, 2008, 2019
// @codepage windows-1251
// Файл определений, общих для Papyrus, StyloConduit и StyloPalmII
//
#ifndef __STYLOPALM_H
#define __STYLOPALM_H
#if !defined(__palmos__) && defined(__PALMOS_TRAPS__)
	#define __palmos__ 1
#endif
#if !defined(__palmos__)
	#include <slib.h>
#else
	#include "slib.h"
#endif
//
// Структура данных, передаваемая на Palm и принимаемая с него.
// Передается и принимается эта структура в бинарном виде в файле PALMCFG.BIN
//
#define CFGF_DONTSHOWWEEKREST        0x00000001
#define CFGF_DONTSHOWWEEKSELLS       0x00000002
#define CFGF_LOADSELLS               0x00000004
#define CFGF_LOADDEBTS               0x00000008
#define CFGF_GOODSSELSRCHBYCODE      0x00000010
#define CFGF_COMPRESSDATA            0x00000020
#define CFGF_PALMCOMPRESSEDDATA      0x00000040
#define CFGF_AUTOARCPALMDATA         0x00000080
#define CFGF_BLOCKAPPSTOP            0x00000100
#define CFGF_RESETAFTERUNPACK        0x00000200 // перезагрузка после распаковки данных
#define CFGF_EXPIMPTODO              0x00000400 // экспорт/импорт дел
#define CFGF_DENYUPDATEPROGRAM       0x00000800 // Запрет на обновление программы при удаленном соединении
#define CFGF_UPDATEPROGRAM           0x00001000 // Выставляется перед закрытием прогаммы (если существует StyloWce.new), чтобы Today plagin смог обновить программу.
#define CFGF_RESTOREPROGRAM          0x00002000 // Восстановить приложение StyloWce.exe из архива
#define CFGF_LARGEBRWFONT            0x00004000 // Увеличенный размер шрифта в броузерах (StyloWCE.exe)
#define CFGF_DONTUSEBRWCOLORS        0x00008000 // Не использовать цвета для элементов броузра (StyloWCE.exe)
#define CFGF_LOADBRANDS              0x00010000 // Загружать бренды
#define CFGF_BILLROWSSETCURSINPACK   0x00020000 // При вводе строки документа выставлять курсор в поле упаковка
#define CFGF_SHOWBRANDOWNCOMBO       0x00040000 // Если указан флаг CFGF_LOADBRANDS, то при инициализации диалога выбора товара показывает ComboBox выбора владельца бренда
#define CFGF_NEWSIPALG               0x00080000 // Новый алгоритм управления отображения окна клавиатуры (в основном для смартфонов, так как обычный алгоритм не всегда
                                              // работает корректно, по неизвестным причинам.
#define CFGF_LOADLOCS                0x00100000 // Загружать склады

#define CLIENTF_BLOCKED              0x00000001 // Клиент заблокирован (->ARTRF_STOPBILL)
#define CLIENTF_DONTUSEMINSHIPMQTTY  0x00000002 // Для этого клиента не использовать кратность отгружаемого количества (AGTF_DONTUSEMINSHIPMQTTY)

#define PALM_MAXRECCOUNT 64000

#define SHOWGTBLCOLS_ALL        1
#define SHOWGTBLCOLS_GOODSPRICE 2
#define SHOWGTBLCOLS_GOODSREST  3
#define SHOWGTBLCOLS_GOODS      4
//
// Структура заголовка таблицы в архиве, передаваемом с хоста на Palm
//
struct PalmArcHdr { // size = 64
	int    Check() const;
	void   ToPalmRec();
	void   ToHostRec();
	int    ToBuf(void * pBuf, size_t * pBufSize);
	int    FromBuf(const void * pBuf);

	int16  Ver;         // 1..
	uint32 NumRecs;     // Количество записей в таблице (этот заголовок записью не считается)
	char   Reserve[26]; // zero (int8 -> char)
	char   Name[32];    // Имя таблицы данных
};

//
// Структура буфера команд
//
typedef long SpiiDbHandler[2];

struct SpiiCmdBuf { // @persistent
	SpiiCmdBuf();
#ifndef __palmos__
	void ToPalmRec();
	void ToHostRec();
	SString & ToStr(SString & rBuf) const;
#endif
	enum {
		cmOpen          =  1,
		cmCloseTbl      =  2,
		cmGetStat       =  3,
		cmGetPos        =  4,
		cmSetPos        =  5,
		cmAddRec        =  6,
		cmUpdRec        =  7,
		cmDelRec        =  8,
		cmGetRec        =  9,
		cmGetTbl        = 10,
		cmSetTbl        = 11,
		cmDelTbl        = 12,
		cmGetDevInfo    = 13,
		cmQuitSess      = 14,
		cmFindTbl       = 15,
		cmPurgeAllRecs  = 16,
		cmGetProgramVer = 17
	};
	int16  Cmd;
	int32  RetCode;
	SpiiDbHandler Hdl;
	uint32 BufSize;
	//void Buf[];
};
//
// Размер буфера упаковки данных для передачи на Palm
// Используется в StyloConuit и в StyloPalm.
// При изменении необходимо перекомпилировать оба приложения //
//
#define PALMARCBUFSIZE (24*1024)
#define PALMPACKRECLEN (4 * 1024)

#define MIN_SENDBUFSIZE (sizeof(SpiiCmdBuf) + 32)                       // Минимальный размер буфера для отправки данных с кпк
#define MIN_RECVBUFSIZE (sizeof(SpiiCmdBuf) + sizeof(PalmArcHdr) + 128) // Минимальный размер буфера для приема данных с кпк

struct PalmConfig { // size = 96 // @persistent
	PalmConfig();
	void   Init();
	int    CompressData() const;
	int    PalmCompressedData() const;
	int    Write(const char * pPath);
	int    Read(const char * pPath);
	void   ToHostRec();
	void   ToPalmRec();
	int    ToBuf(void * pBuf, size_t * pBufSize);
	int    FromBuf(const void * pBuf);
	void   SetupBuffers(uint32 recvBufSize, uint32 sendBufSize);

	uint32 Size;           // Размер структуры
	uint32 Ver;            // 4 // Номер версии структуры. Начинается с 1.
	// Любое изменение структуры должно влечь за собой увеличение этого номера на единицу.
	uint32 InvCode;        // Номер последней инвентаризации
	uint32 OrdCode;        // Номер последнего заказа
	uint32 Shift;          // @obsolete Сдвиг фиксированной точки в числах
	uint32 Flags;          // @flags
	LDATETIME TmLastXchg;  // Время последнего обмена между хостом и Palm'ом
	LDATETIME TmClient;    // Время формирования данных по клиентам
	LDATETIME TmGoods;     // Время формирования данных по товарам
	LDATETIME TmCliDebt;   // Время формирования данных по клиентским долгам
	LDATETIME TmCliSell;   // Время формирования данных по продажам клиентам
	uint32 GTblViewColsID; // Какие колонки показывать в диалоге выбора товара
	LDATETIME TmToDo;      // Время формирования данных по задачам
	uint16 NumSellWeeks;   // Количестов загруженных недельных продаж
	uint16 CardNo;         // Номер карты на которой будем хранить справочники
		// данную конфигурацию храним на карте с номером 0
	uint32 ActiveNetCfgID; // Ид активной конфигурации соединени
	uint32 SendBufSize;    // Размер пакета при передаче данных с КПК (касается только упакованных данных)
	uint32 RecvBufSize;    // Размер пакета при приеме данных на КПК (касается только упакованных данных)
	uint16 MaxNotSentOrd;  // Максимальное кол-во неотправленных заявок. При достижении этого числа, блокируется создание новой заявки
	uint16 Reserve2;       // @reserve
};

extern const char * P_PalmConfigFileName; // defined in StyloPalm.cpp
extern const char * P_StyloPalmCreatorID; // "SPII"
extern const char * P_PalmArcTblName; // "SpiiUcl.pdb"
extern const char * P_PalmPackedDataTblName; // "SpiiPD.pdb"
extern const char * P_PalmProgramFileName;   // "StyloWce.exe" обновление файлов поддерживают только устройства с ОС Windows Mobile
extern const char * P_PalmDllFileName;       // "TodayItem.dll" обновление файлов поддерживают только устройства с ОС Windows Mobile
//
// ID строки ресурсов версии StyloWce.exe
//
#define RESSTRING_VER 102

struct SpiiTblOpenParams {
	enum {
		omRead      = 1,
		omWrite     = 2,
		omReadWrite = 3,
		omCreate    = 4
	};
#ifndef __palmos__
	void ToPalmRec() 
	{
		Mode = htonl(Mode);
	}
#endif
	char  TblName[32];
	long  Mode;
};

struct SpiiTblStatParams {
#ifndef __palmos__
	void ToHostRec()
	{
		NumRecs       = ntohl(NumRecs);
		CurPos        = ntohl(CurPos);
		Size          = ntohl(Size);
		/*
		ModifTime.d.v = ntohl(ModifTime.d.v);
		ModifTime.t.v = ntohl(ModifTime.t.v);
		*/
	}
	void ToPalmRec()
	{
		NumRecs       = htonl(NumRecs);
		CurPos        = htonl(CurPos);
		Size          = htonl(Size);
		/*
		ModifTime.d.v = htonl(ModifTime.d.v);
		ModifTime.t.v = htonl(ModifTime.t.v);
		*/
	}
#endif
	char   Name[32];
	uint32 CreatorID;
	uint32 NumRecs;
	uint32 CurPos;
	uint32 Size;
	LDATETIME ModifTime;
};

struct SpiiDeviceInfoParams {
#ifndef __palmos__
	void ToHostRec() {ID = ntohl(ID);}
	void ToPalmRec() {ID = htonl(ID);}
#endif
	uint32 ID;
	char   Name[12];
};

struct SpiiTblDelParams {
	char TblName[32];
};

struct SpiiTblRecParams {
	SpiiTblRecParams() : RecID(0), Pos(0)
	{
	}
#ifndef __palmos__
	void ToHostRec() {/*RecID = ntohl(RecID);*/ Pos = ntohl(Pos);}
	void ToPalmRec() {/*RecID = htonl(RecID);*/ Pos= htonl(Pos);}
#endif

	uint32 RecID;
	uint32 Pos;
};

struct SpiiProgramVerParams {
#ifndef __palmos__
	void ToHostRec() {Ver = ntohl(Ver);}
	void ToPalmRec() {Ver = htonl(Ver);}
#endif
	uint32 Ver;
};

struct SpBaseStruc {
	virtual int  ToBuf(void * pBuf, size_t * pBufSize) {return -1;}
	virtual int  FromBuf(const void * pBuf)            {return -1;}
	virtual void ToPalmRec() {}
	virtual void ToHostRec() {}
};

struct Quot { // Size = 8
	int32  QuotKindID; // -> QuotKind.ID
	int32  Price;
};

struct SpGoodsStruc : public SpBaseStruc {
	SpGoodsStruc();
	~SpGoodsStruc();
	virtual void ToHostRec();
	virtual int FromBuf(const void * pBuf);
	void   Init();

	int32  ID;
	char   Code[16];
	int32  GoodsGrpID;
	int32  Pack;
	int32  Price;
	int32  Rest;
	char   Name[64];
	int32  BrandID;
	int32  BrandOwnerID;
	int32  MinOrd;
	int16  MultMinOrd;
	int16  QuotCount;
	Quot   * P_Quots;
};

struct SpBrandStruc : public SpBaseStruc {
	SpBrandStruc();
	virtual void ToHostRec();
	virtual int FromBuf(const void * pBuf);
	void  Init();

	int32 ID;
	int32 OwnerID;
	char  Name[64];
	char  OwnerName[64];
};

struct SpLocStruc : public SpBaseStruc {
	SpLocStruc(); 
	virtual void ToHostRec();
	virtual int FromBuf(const void * pBuf);
	void   Init();

	int32  ID;
	char   Name[64];
};

struct SpGoodsGrpStruc  : public SpBaseStruc {
	SpGoodsGrpStruc();
	virtual void ToHostRec();
	virtual int FromBuf(const void * pBuf);
	void   Init();
	int32  ID;
	int16  Code;
	char   Name[64];
};

#define MAXADDRLEN 64

struct Addr { // size = 68
	int32 ID;
	char  Loc[MAXADDRLEN];
};

struct SpClientStruc : public SpBaseStruc {
	SpClientStruc();
	~SpClientStruc();
	virtual void ToHostRec();
	virtual int FromBuf(const void * pBuf);
	void   Init();
	int32  ID;
	int32  QuotKindID; // -> QuotKind.ID
	int32  Debt;
	char   Code[16];
	char   Name[48];
	int16  AddrCount;
	Addr  * P_Addrs;
	int32  Flags;
};

struct SpOrdHeaderStruc : public SpBaseStruc { // size = 200
	SpOrdHeaderStruc();
	void Init();
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual void ToPalmRec();

	int32  ID;
	uint16 Date;
	char   Code[10];
	int32  ClientID;
	int32  DlvrAddrID;
	int32  QuotKindID;
	int32  Amount;
	int32  PctDis;
	char   Memo[160];
	int32  LocID;
};

struct SpOrdLineStruc : public SpBaseStruc { // size = 20
	SpOrdLineStruc();
	void Init();
	virtual int ToBuf(void * pBuf, size_t * pBufSize);
	virtual void ToPalmRec();

	int32 ID;
	int32 OrderID;
	int32 GoodsID;
	int32 Price;
	int32 Qtty;
};

struct SpQuotKindStruc : public SpBaseStruc {
	SpQuotKindStruc() 
	{
		Init();
	}
	void Init() 
	{
		ID = 0; 
		memzero(Name, sizeof(Name));
	}
	int FromBuf(const void * pBuf)
	{
		const char * p_buf = static_cast<const char *>(pBuf);
		size_t bytes = 0;
		memcpy(&ID,  p_buf,                         sizeof(ID));
		memcpy(Name, p_buf + (bytes += sizeof(ID)), sizeof(Name));
		return 1;
	}
	virtual void ToPalmRec()
	{
		ID = htonl(ID);
	}
	virtual void ToHostRec()
	{
		ID = ntohl(ID);
	}
	int32 ID;
	char  Name[30];
};

struct SpClientDebtStruc : public SpBaseStruc {
	SpClientDebtStruc() 
	{
		Init();
	}
	virtual void ToHostRec()
	{
		ClientID = ntohl(ClientID);
		Dt       = ntohs(Dt);
		Amount   = ntohl(Amount);
		Debt     = ntohl(Debt);
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = static_cast<const char *>(pBuf);
		size_t bytes = 0;
		memcpy(&ClientID, p_buf,                               sizeof(ClientID));
		memcpy(&Dt,       p_buf + (bytes += sizeof(ClientID)), sizeof(Dt));
		memcpy(Code,      p_buf + (bytes += sizeof(Dt)),       sizeof(Code));
		memcpy(&Amount,   p_buf + (bytes += sizeof(Code)),     sizeof(Amount));
		memcpy(&Debt,     p_buf + (bytes += sizeof(Amount)),   sizeof(Debt));
		return 1;
	}
	void Init()
	{
		ClientID = Amount = Debt = 0;
		Dt = 0;
		memzero(Code, sizeof(Code));
	}
	int32  ClientID;
	uint16 Dt;
	char   Code[10];
	int32  Amount;
	int32  Debt;
};

struct SalesItem {
	uint16 Date;
	int32  Qtty;
};

struct SpClientSalesStruc : public SpBaseStruc {
	SpClientSalesStruc() : P_Items(0)
	{
		Init();
	}
	~SpClientSalesStruc() 
	{
		ZDELETE(P_Items);
	}
	virtual void ToHostRec()
	{
		ClientID   = ntohl(ClientID);
		DlvrAddrID = ntohl(DlvrAddrID);
		GoodsID    = ntohl(GoodsID);
		ItemsCount = ntohs(ItemsCount);
		for(int16 i = 0; i < ItemsCount; i++)
			P_Items[i].Date = ntohs(P_Items[i].Date);
	}
	virtual int FromBuf(const void * pBuf)
	{
		int ok = 1;
		const char * p_buf = static_cast<const char *>(pBuf);
		size_t bytes = 0;
		memcpy(&ClientID,   p_buf,                                 sizeof(ClientID));
		memcpy(&DlvrAddrID, p_buf + (bytes += sizeof(ClientID)),   sizeof(DlvrAddrID));
		memcpy(&GoodsID,    p_buf + (bytes += sizeof(DlvrAddrID)), sizeof(GoodsID));
		memcpy(&ItemsCount, p_buf + (bytes += sizeof(GoodsID)),    sizeof(ItemsCount));
		ItemsCount = ntohs(ItemsCount);
		bytes += sizeof(ItemsCount);
		ZDELETE(P_Items);
		if(ItemsCount) {
			uint sales_item_size = 6;
			THROW(P_Items = new SalesItem[ItemsCount * sales_item_size]);
			for(int16 i = 0; i < ItemsCount; i++) {
				SalesItem * p_si = &P_Items[i];
				memcpy(&p_si->Date, p_buf + bytes,                      sizeof(p_si->Date));
				memcpy(&p_si->Qtty, p_buf + bytes + sizeof(p_si->Date), sizeof(p_si->Qtty));
				bytes += sales_item_size;
			}
		}
		CATCHZOK
		ItemsCount = htons(ItemsCount);
		return ok;
	}
	void Init()
	{
		ClientID = DlvrAddrID = GoodsID = 0;
		ItemsCount = 0;
		ZDELETE(P_Items);
	}
	int32  ClientID;
	int32  DlvrAddrID;
	int32  GoodsID;
	int16  ItemsCount;
	SalesItem * P_Items;
};

struct SpInvHeaderStruc : public SpBaseStruc { // size = 184
	SpInvHeaderStruc() 
	{
		Init();
	}
	void Init()
	{
		ID = ClientID = DlvrAddrID = 0;
		Date = 0;
		memzero(Code, sizeof(Code));
		memzero(Memo, sizeof(Memo));
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 184) {
				*pBufSize = 184;
				ok = -1;
			}
			else {
				char * p_buf = static_cast<char *>(pBuf);
				size_t bytes = 0;
				memcpy(p_buf,                                 &ID,         sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),         &Date,       sizeof(Date));
				memcpy(p_buf + (bytes += sizeof(Date)),       Code,        sizeof(Code));
				memcpy(p_buf + (bytes += sizeof(Code)),       &ClientID,   sizeof(ClientID));
				memcpy(p_buf + (bytes += sizeof(ClientID)),   &DlvrAddrID, sizeof(DlvrAddrID));
				memcpy(p_buf + (bytes += sizeof(DlvrAddrID)), Memo,        sizeof(Memo));
				*pBufSize = 184;
				ok = 1;
			}
		}
		return ok;
	}
	virtual void ToPalmRec()
	{
		ID         = htonl(ID);
		Date       = htons(Date);
		ClientID   = htonl(ClientID);
		DlvrAddrID = htonl(DlvrAddrID);
	}
	int32  ID;
	uint16 Date;
	char   Code[10];
	int32  ClientID;
	int32  DlvrAddrID;
	char   Memo[160];
};

struct SpInvLineStruc : public SpBaseStruc {
	SpInvLineStruc() 
	{
		Init();
	}
	void Init()	
	{
		InvID = GoodsID = Qtty = 0;
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 12) {
				*pBufSize = 12;
				ok = -1;
			}
			else {
				char * p_buf = static_cast<char *>(pBuf);
				size_t bytes = 0;
				memcpy(p_buf,                              &InvID,   sizeof(InvID));
				memcpy(p_buf + (bytes += sizeof(InvID)),   &GoodsID, sizeof(GoodsID));
				memcpy(p_buf + (bytes += sizeof(GoodsID)), &Qtty,    sizeof(Qtty));
				*pBufSize = 12;
				ok = 1;
			}
		}
		return ok;
	}
	virtual void ToPalmRec()
	{
		InvID   = htonl(InvID);
		GoodsID = htonl(GoodsID);
		Qtty    = htonl(Qtty);
	}
	int32  InvID;
	int32  GoodsID;
	int32  Qtty;
};

//
#define NETCMD_HELLO "HELLO"
#define NETCMD_SPII  "SPII"
//
// Tables names
//
#define SPTBLNAM_GOODS                 "Goods.tbl"
	#define SPTBLNAM_GOODSIDIDX         "GdsID.idx"
	#define SPTBLNAM_GOODSNAMIDX        "GdsName.idx"
	#define SPTBLNAM_GOODSGRPIDNAMIDX   "GdsGNam.idx"
	#define SPTBLNAM_GOODSGRPIDIDIDX    "GdsGID.idx"
	#define SPTBLNAM_GOODSCODEIDX       "GdsCode.idx"
#define SPTBLNAM_GOODSGRP              "GoodsGrp.tbl"
	#define SPTBLNAM_GOODSGRPIDIDX      "GroupID.idx"
	#define SPTBLNAM_GOODSGRPNAMIDX     "GroupNam.idx"
#define SPTBLNAM_CLIENT                "Client.tbl"
	#define SPTBLNAM_CLIENTIDIDX        "ClID.idx"
	#define SPTBLNAM_CLIENTNAMIDX       "ClNam.idx"
#define SPTBLNAM_QUOTKIND              "QuotKind.tbl"
#define SPTBLNAM_CLIENTDEBT            "CliDebt.tbl"
	#define SPTBLNAM_CLIENTDEBTIDDTCODE "CdIdDtC.idx"
#define SPTBLNAM_ORDHEADER             "OrdHdr.tbl"
	#define SPTBLNAM_ORDHEADERIDIDX     "OrdHID.idx"
	#define SPTBLNAM_ORDHEADERDATEIDIDX "OrdHDtID.idx"
#define SPTBLNAM_ORDLINE               "OrdLine.tbl"
	#define SPTBLNAM_ORDLINEORDGOODSIDX "OLOGoods.idx"
	#define SPTBLNAM_ORDLINEORDIDIDX    "OLOID.idx"
#define SPTBLNAM_INVHEADER             "InvHdr.tbl"
	#define SPTBLNAM_INVHEADERIDIDX     "InvHID.idx"
	#define SPTBLNAM_INVHEADERDATEIDX   "InvHDt.idx"
	#define SPTBLNAM_INVHEADERCLIDADRID "InvHClAdr.idx"
#define SPTBLNAM_INVLINE               "InvLine.tbl"
	#define SPTBLNAM_INVLINEINVGOODSIDX "ILIGoods.idx"
#define SPTBLNAM_SALES                 "CliSell.tbl"
#define SPTBLNAM_CFG                   "Config.tbl"
#define SPTBLNAM_TODO                  "ToDoDB"
#define SPTBLNAM_TODOASSOC             "ToDoSPII.dat"
#define SPTBLNAM_NETCONFIG             "NetCSPII.tbl"
	#define SPTBLNAM_NETCONFIGIDIDX     "NetCID.idx"
#define SPTBLNAM_BRAND                 "Brand.tbl"
#define SPTBLNAM_LOC                   "Location.tbl"
#endif
