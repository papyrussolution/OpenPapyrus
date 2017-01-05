// STYLOPALM.H
// Copyright (c) A.Sobolev 2005, 2008
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
	void   ToPalmRec()
	{
		Ver     = htons(Ver);
		NumRecs = htonl(NumRecs);
	}
	void   ToHostRec()
	{
		Ver     = ntohs(Ver);
		NumRecs = ntohl(NumRecs);
	}
	int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 64) {
				*pBufSize = 64;
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;
				memcpy(p_buf,                               &Ver,     sizeof(Ver));
				memcpy(p_buf + (bytes += sizeof(Ver)),      &NumRecs, sizeof(NumRecs));
				memcpy(p_buf + (bytes += sizeof(NumRecs)),  Reserve,  sizeof(Reserve));
				memcpy(p_buf + (bytes += sizeof(Reserve)),  Name,     sizeof(Name));
				*pBufSize = 64;
				ok = 1;
			}
		}
		return ok;
	}
	int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&Ver,     p_buf,                              sizeof(Ver));
		memcpy(&NumRecs, p_buf + (bytes += sizeof(Ver)),     sizeof(NumRecs));
		memcpy(Reserve,  p_buf + (bytes += sizeof(NumRecs)), sizeof(Reserve));
		memcpy(Name,     p_buf + (bytes += sizeof(Reserve)), sizeof(Name));
		return 1;
	}
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
	SpiiCmdBuf()
	{
		memset(this, 0, sizeof(SpiiCmdBuf));
	}
#ifndef __palmos__
	void ToPalmRec()
	{
		Cmd     = htons(Cmd);
		BufSize = htonl(BufSize);
		RetCode = htonl(RetCode);
	}
	void ToHostRec()
	{
		Cmd     = ntohs(Cmd);
		BufSize = ntohl(BufSize);
		RetCode = ntohl(RetCode);
	}
	SString & ToStr(SString & rBuf) const
	{
		rBuf = 0;
		return rBuf.Cat((long)Cmd).CatDiv(';', 0).Cat(RetCode).CatDiv(';', 0).
			Cat(Hdl[0]).CatChar('-').Cat(Hdl[1]).CatDiv(';', 0).Cat(BufSize);
	}
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

#define MIN_SENDBUFSIZE sizeof(SpiiCmdBuf) + 32                       // Минимальный размер буфера для отправки данных с кпк
#define MIN_RECVBUFSIZE sizeof(SpiiCmdBuf) + sizeof(PalmArcHdr) + 128 // Минимальный размер буфера для приема данных с кпк

struct PalmConfig { // size = 96 // @persistent
	PalmConfig();
	void   Init();
	int    CompressData() const
	{
		return BIN(Flags & CFGF_COMPRESSDATA);
	}
	int    PalmCompressedData() const
	{
		return BIN(Flags & CFGF_PALMCOMPRESSEDDATA);
	}
	int    Write(const char * pPath);
	int    Read(const char * pPath);
	void   ToHostRec()
	{
		Size    = ntohl(Size);
		Ver     = ntohl(Ver);
		InvCode = ntohl(InvCode);
		OrdCode = ntohl(OrdCode);
		Shift   = ntohl(Shift);
		Flags   = ntohl(Flags);
		TmLastXchg.d.v = (ulong)ntohl(TmLastXchg.d.v);
		TmClient.d.v   = (ulong)ntohl(TmClient.d.v);
		TmGoods.d.v    = (ulong)ntohl(TmGoods.d.v);
		TmCliDebt.d.v  = (ulong)ntohl(TmCliDebt.d.v);
		TmCliSell.d.v  = (ulong)ntohl(TmCliSell.d.v);
		TmToDo.d.v     = (ulong)ntohl(TmToDo.d.v);
		/*
		TmLastXchg.t.v = (ulong)ntohl(TmLastXchg.t.v);
		TmClient.t.v   = (ulong)ntohl(TmClient.t.v);
		TmGoods.t.v    = (ulong)ntohl(TmGoods.t.v);
		TmCliDebt.t.v  = (ulong)ntohl(TmCliDebt.t.v);
		TmCliSell.t.v  = (ulong)ntohl(TmCliSell.t.v);
		TmToDo.t.v     = (ulong)ntohl(TmToDo.t.v);
		*/
		RecvBufSize = ntohl(RecvBufSize);
		SendBufSize = ntohl(SendBufSize);
	}
	void ToPalmRec()
	{
		Size    = htonl(Size);
		Ver     = htonl(Ver);
		InvCode = htonl(InvCode);
		OrdCode = htonl(OrdCode);
		Shift   = htonl(Shift);
		Flags   = htonl(Flags);
		TmLastXchg.d.v = htonl(TmLastXchg.d.v);
		TmClient.d.v   = htonl(TmClient.d.v);
		TmGoods.d.v    = htonl(TmGoods.d.v);
		TmCliDebt.d.v  = htonl(TmCliDebt.d.v);
		TmCliSell.d.v  = htonl(TmCliSell.d.v);
		TmToDo.d.v     = htonl(TmToDo.d.v);
		/*
		TmLastXchg.t.v = htonl(TmLastXchg.t.v);
		TmClient.t.v   = htonl(TmClient.t.v);
		TmGoods.t.v    = htonl(TmGoods.t.v);
		TmCliDebt.t.v  = htonl(TmCliDebt.t.v);
		TmCliSell.t.v  = htonl(TmCliSell.t.v);
		TmToDo.t.v     = htonl(TmToDo.t.v);
		*/
		SendBufSize    = htonl(SendBufSize);
		RecvBufSize    = htonl(RecvBufSize);
	}
	int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 96) {
				*pBufSize = 96;
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;

				memcpy(p_buf + bytes,                             &Size,           sizeof(Size));
				memcpy(p_buf + (bytes += sizeof(Size)),           &Ver,            sizeof(Ver));
				memcpy(p_buf + (bytes += sizeof(Ver)),            &InvCode,        sizeof(InvCode));
				memcpy(p_buf + (bytes += sizeof(InvCode)),        &OrdCode,        sizeof(OrdCode));
				memcpy(p_buf + (bytes += sizeof(OrdCode)),        &Shift,          sizeof(Shift));
				memcpy(p_buf + (bytes += sizeof(Shift)),          &Flags,          sizeof(Flags));
				memcpy(p_buf + (bytes += sizeof(Flags)),          &TmLastXchg,     sizeof(TmLastXchg));
				memcpy(p_buf + (bytes += sizeof(TmLastXchg)),     &TmClient,       sizeof(TmClient));
				memcpy(p_buf + (bytes += sizeof(TmClient)),       &TmGoods,        sizeof(TmGoods));
				memcpy(p_buf + (bytes += sizeof(TmGoods)),        &TmCliDebt,      sizeof(TmCliDebt));
				memcpy(p_buf + (bytes += sizeof(TmCliDebt)),      &TmCliSell,      sizeof(TmCliSell));
				memcpy(p_buf + (bytes += sizeof(TmCliSell)),      &GTblViewColsID, sizeof(GTblViewColsID));
				memcpy(p_buf + (bytes += sizeof(GTblViewColsID)), &TmToDo,         sizeof(TmToDo));
				memcpy(p_buf + (bytes += sizeof(TmToDo)),         &NumSellWeeks,   sizeof(NumSellWeeks));
				memcpy(p_buf + (bytes += sizeof(NumSellWeeks)),   &CardNo,         sizeof(CardNo));
				memcpy(p_buf + (bytes += sizeof(CardNo)),         &ActiveNetCfgID, sizeof(ActiveNetCfgID));
				memcpy(p_buf + (bytes += sizeof(ActiveNetCfgID)), &SendBufSize,    sizeof(SendBufSize));
				memcpy(p_buf + (bytes += sizeof(SendBufSize)),    &RecvBufSize,    sizeof(RecvBufSize));
				memcpy(p_buf + (bytes += sizeof(RecvBufSize)),    &MaxNotSentOrd,  sizeof(MaxNotSentOrd));
				memcpy(p_buf + (bytes += sizeof(MaxNotSentOrd)),  &Reserve2,       sizeof(Reserve2));
				*pBufSize = 96;
				ok = 1;
			}
		}
		return ok;
	}
	int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&Size,           p_buf + bytes,                             sizeof(Size));
		memcpy(&Ver,            p_buf + (bytes += sizeof(Size)),           sizeof(Ver));
		memcpy(&InvCode,        p_buf + (bytes += sizeof(Ver)),            sizeof(InvCode));
		memcpy(&OrdCode,        p_buf + (bytes += sizeof(InvCode)),        sizeof(OrdCode));
		memcpy(&Shift,          p_buf + (bytes += sizeof(OrdCode)),        sizeof(Shift));
		memcpy(&Flags,          p_buf + (bytes += sizeof(Shift)),          sizeof(Flags));
		memcpy(&TmLastXchg,     p_buf + (bytes += sizeof(Flags)),          sizeof(TmLastXchg));
		memcpy(&TmClient,       p_buf + (bytes += sizeof(TmLastXchg)),     sizeof(TmClient));
		memcpy(&TmGoods,        p_buf + (bytes += sizeof(TmClient)),       sizeof(TmGoods));
		memcpy(&TmCliDebt,      p_buf + (bytes += sizeof(TmGoods)),        sizeof(TmCliDebt));
		memcpy(&TmCliSell,      p_buf + (bytes += sizeof(TmCliDebt)),      sizeof(TmCliSell));
		memcpy(&GTblViewColsID, p_buf + (bytes += sizeof(TmCliSell)),      sizeof(GTblViewColsID));
		memcpy(&TmToDo,         p_buf + (bytes += sizeof(GTblViewColsID)), sizeof(TmToDo));
		memcpy(&NumSellWeeks,   p_buf + (bytes += sizeof(TmToDo)),         sizeof(NumSellWeeks));
		memcpy(&CardNo,         p_buf + (bytes += sizeof(NumSellWeeks)),   sizeof(CardNo));
		memcpy(&ActiveNetCfgID, p_buf + (bytes += sizeof(CardNo)),         sizeof(ActiveNetCfgID));
		memcpy(&SendBufSize,    p_buf + (bytes += sizeof(ActiveNetCfgID)), sizeof(SendBufSize));
		memcpy(&RecvBufSize,    p_buf + (bytes += sizeof(SendBufSize)),    sizeof(RecvBufSize));
		memcpy(&MaxNotSentOrd,  p_buf + (bytes += sizeof(RecvBufSize)),    sizeof(MaxNotSentOrd));
		memcpy(&Reserve2,       p_buf + (bytes += sizeof(MaxNotSentOrd)),  sizeof(Reserve2));
		return 1;
	}
	void SetupBuffers(uint32 recvBufSize, uint32 sendBufSize)
	{
		RecvBufSize = (recvBufSize >= MIN_RECVBUFSIZE && recvBufSize <= PALMARCBUFSIZE) ? recvBufSize : PALMARCBUFSIZE;
		SendBufSize = (sendBufSize >= MIN_SENDBUFSIZE && sendBufSize <= PALMPACKRECLEN) ? sendBufSize : PALMPACKRECLEN;
	}
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
	void ToPalmRec() {Mode = htonl(Mode);}
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
	SpiiTblRecParams() {memset(this, 0, sizeof(SpiiTblRecParams));}
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
	int32 QuotKindID; // -> QuotKind.ID
	int32 Price;
};

struct SpGoodsStruc : public SpBaseStruc {
	SpGoodsStruc() {P_Quots = 0; Init();}
	~SpGoodsStruc() {ZDELETE(P_Quots);}
	virtual void ToHostRec()
	{
		ID           = ntohl(ID);
		GoodsGrpID   = ntohl(GoodsGrpID);
		Pack         = ntohl(Pack);
		Price        = ntohl(Price);
		Rest         = ntohl(Rest);
		BrandID      = ntohl(BrandID);
		BrandOwnerID = ntohl(BrandOwnerID);
		MinOrd       = ntohl(MinOrd);
		// MultMinOrd   = ntohs(MultMinOrd);
		QuotCount    = ntohs(QuotCount);
		for(int16 i = 0; i < QuotCount; i++) {
			P_Quots[i].QuotKindID = ntohl(P_Quots[i].QuotKindID);
			P_Quots[i].Price      = ntohl(P_Quots[i].Price);
		}
	}
	virtual int FromBuf(const void * pBuf)
	{
		int ok = 1;
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,           p_buf,                                   sizeof(ID));
		memcpy(Code,          p_buf + (bytes += sizeof(ID)),           sizeof(Code));
		memcpy(&GoodsGrpID,   p_buf + (bytes += sizeof(Code)),         sizeof(GoodsGrpID));
		memcpy(&Pack,         p_buf + (bytes += sizeof(GoodsGrpID)),   sizeof(Pack));
		memcpy(&Price,        p_buf + (bytes += sizeof(Pack)),         sizeof(Price));
		memcpy(&Rest,         p_buf + (bytes += sizeof(Price)),        sizeof(Rest));
		memcpy(Name,          p_buf + (bytes += sizeof(Rest)),         sizeof(Name));
		memcpy(&BrandID,      p_buf + (bytes += sizeof(Name)),         sizeof(BrandID));
		memcpy(&BrandOwnerID, p_buf + (bytes += sizeof(BrandID)),      sizeof(BrandOwnerID));
		memcpy(&MinOrd,       p_buf + (bytes += sizeof(BrandOwnerID)), sizeof(MinOrd));
		memcpy(&MultMinOrd,   p_buf + (bytes += sizeof(MinOrd)),       sizeof(MultMinOrd));
		memcpy(&QuotCount,    p_buf + (bytes += sizeof(MultMinOrd)),   sizeof(QuotCount));
		QuotCount = ntohs(QuotCount);
		bytes += sizeof(QuotCount);
		ZDELETE(P_Quots);
		if(QuotCount) {
			size_t quot_size = 8;
			THROW(P_Quots = new Quot[QuotCount * sizeof(Quot)]);
			for(int16 i = 0; i < QuotCount; i++) {
				Quot * p_q = &P_Quots[i];
				memcpy(&p_q->QuotKindID, p_buf + bytes,                           sizeof(p_q->QuotKindID));
				memcpy(&p_q->Price,      p_buf + bytes + sizeof(p_q->QuotKindID), sizeof(p_q->Price));
				bytes += quot_size;
			}
		}
		CATCH
			ok = 0;
		ENDCATCH
		QuotCount = htons(QuotCount);
		return ok;
	}
	void Init()
	{
		ID = GoodsGrpID = Pack = Price = Rest = BrandID = BrandOwnerID = MinOrd = 0;
		MultMinOrd = 0;
		QuotCount = 0;
		ZDELETE(P_Quots);
		memzero(Code, sizeof(Code));
		memzero(Name, sizeof(Name));
	}
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
	SpBrandStruc() {Init();}
	virtual void ToHostRec()
	{
		ID      = ntohl(ID);
		OwnerID = ntohl(OwnerID);
	}
	virtual int FromBuf(const void * pBuf)
	{
		int ok = 1;
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,       p_buf,                              sizeof(ID));
		memcpy(&OwnerID,  p_buf + (bytes += sizeof(ID)),      sizeof(OwnerID));
		memcpy(Name,      p_buf + (bytes += sizeof(OwnerID)), sizeof(Name));
		memcpy(OwnerName, p_buf + (bytes += sizeof(Name)),    sizeof(OwnerName));
		return ok;
	}
	void Init()
	{
		ID = OwnerID = 0;
		memzero(Name,      sizeof(Name));
		memzero(OwnerName, sizeof(OwnerName));
	}
	int32 ID;
	int32 OwnerID;
	char  Name[64];
	char  OwnerName[64];
};

struct SpLocStruc : public SpBaseStruc {
	SpLocStruc() {Init();}
	virtual void ToHostRec()
	{
		ID = ntohl(ID);
	}
	virtual int FromBuf(const void * pBuf)
	{
		int ok = 1;
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,       p_buf,                         sizeof(ID));
		memcpy(Name,      p_buf + (bytes += sizeof(ID)), sizeof(Name));
		return ok;
	}
	void Init()
	{
		ID = 0;
		memzero(Name, sizeof(Name));
	}
	int32 ID;
	char  Name[64];
};

struct SpGoodsGrpStruc  : public SpBaseStruc {
	SpGoodsGrpStruc()
	{
		Init();
	}
	virtual void ToHostRec()
	{
		ID = ntohl(ID);
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,   p_buf, sizeof(ID));
		memcpy(&Code, p_buf + (bytes += sizeof(ID)), sizeof(Code));
		memcpy(Name,  p_buf + (bytes += sizeof(Code)), sizeof(Name));
		return 1;
	}
	void    Init()
	{
		ID = 0;
		Code = 0;
		memzero(Name, sizeof(Name));
	}
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
	SpClientStruc() {P_Addrs = 0; Init();}
	~SpClientStruc() {ZDELETE(P_Addrs);}
	virtual void ToHostRec()
	{
		ID         = ntohl(ID);
		QuotKindID = ntohl(QuotKindID);
		Debt       = ntohl(Debt);
		AddrCount  = ntohs(AddrCount);
		for(int16 i = 0; i < AddrCount; i++)
			P_Addrs[i].ID = ntohl(P_Addrs[i].ID);
		Flags = ntohl(Flags);
	}
	virtual int FromBuf(const void * pBuf)
	{
		int ok = 1;
		const char * p_buf = (const char*)pBuf;
		size_t bytes = 0;
		memcpy(&ID,         p_buf,                                 sizeof(ID));
		memcpy(&QuotKindID, p_buf + (bytes += sizeof(ID)),         sizeof(QuotKindID));
		memcpy(&Debt,       p_buf + (bytes += sizeof(QuotKindID)), sizeof(Debt));
		memcpy(Code,        p_buf + (bytes += sizeof(Debt)),       sizeof(Code));
		memcpy(Name,        p_buf + (bytes += sizeof(Code)),       sizeof(Name));
		memcpy(&Flags,      p_buf + (bytes += sizeof(Name)),       sizeof(Flags));
		memcpy(&AddrCount,  p_buf + (bytes += sizeof(Flags)),      sizeof(AddrCount));
		AddrCount = ntohs(AddrCount);
		bytes += sizeof(AddrCount);
		ZDELETE(P_Addrs);
		if(AddrCount) {
			size_t addr_size = 68;
			THROW(P_Addrs = new Addr[AddrCount * sizeof(Addr)]);
			for(int16 i = 0; i < AddrCount; i++) {
				Addr * p_addr = &P_Addrs[i];
				memcpy(&p_addr->ID,  p_buf + bytes,                      sizeof(p_addr->ID));
				memcpy(&p_addr->Loc, p_buf + bytes + sizeof(p_addr->ID), sizeof(p_addr->Loc));
				bytes += addr_size;
			}
		}
		CATCH
			ok = 0;
		ENDCATCH
		AddrCount = htons(AddrCount);
		return ok;
	}
	void Init()
	{
		ID = QuotKindID = Debt = Flags = 0;
		memzero(Code, sizeof(Code));
		memzero(Name, sizeof(Name));
		AddrCount = 0;
		ZDELETE(P_Addrs);
	}
	int32 ID;
	int32 QuotKindID; // -> QuotKind.ID
	int32 Debt;
	char  Code[16];
	char  Name[48];
	int16 AddrCount;
	Addr  * P_Addrs;
	int32 Flags;
};

struct SpOrdHeaderStruc : public SpBaseStruc { // size = 200
	SpOrdHeaderStruc() {Init();}
	void Init()
	{
		ID = ClientID = DlvrAddrID = QuotKindID = Amount = PctDis = 0;
		LocID = 0;
		Date = 0;
		memzero(Code, sizeof(Code));
		memzero(Memo, sizeof(Memo));
	}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 200) {
				*pBufSize = 200;
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;
				memcpy(p_buf,                                 &ID,         sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),         &Date,       sizeof(Date));
				memcpy(p_buf + (bytes += sizeof(Date)),       Code,        sizeof(Code));
				memcpy(p_buf + (bytes += sizeof(Code)),       &ClientID,   sizeof(ClientID));
				memcpy(p_buf + (bytes += sizeof(ClientID)),   &DlvrAddrID, sizeof(DlvrAddrID));
				memcpy(p_buf + (bytes += sizeof(DlvrAddrID)), &QuotKindID, sizeof(QuotKindID));
				memcpy(p_buf + (bytes += sizeof(QuotKindID)), &Amount,     sizeof(Amount));
				memcpy(p_buf + (bytes += sizeof(Amount)),     &PctDis,     sizeof(PctDis));
				memcpy(p_buf + (bytes += sizeof(PctDis)),     Memo,        sizeof(Memo));
				memcpy(p_buf + (bytes += sizeof(Memo)),       &LocID,      sizeof(LocID));
				*pBufSize = 200;
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
		QuotKindID = htonl(QuotKindID);
		Amount     = htonl(Amount);
		PctDis     = htonl(PctDis);
		LocID      = htonl(LocID);
	}

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
	SpOrdLineStruc() {Init();}
	void Init() {ID = OrderID = GoodsID = Price = Qtty = 0;}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 20) {
				*pBufSize = 20;
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
				size_t bytes = 0;
				memcpy(p_buf,                              &ID,      sizeof(ID));
				memcpy(p_buf + (bytes += sizeof(ID)),      &OrderID, sizeof(OrderID));
				memcpy(p_buf + (bytes += sizeof(OrderID)), &GoodsID, sizeof(GoodsID));
				memcpy(p_buf + (bytes += sizeof(GoodsID)), &Price,   sizeof(Price));
				memcpy(p_buf + (bytes += sizeof(Price)),   &Qtty,    sizeof(Qtty));
				*pBufSize = 20;
				ok = 1;
			}
		}
		return ok;
	}
	virtual void ToPalmRec()
	{
		ID       = htonl(ID);
		OrderID  = htonl(OrderID);
		GoodsID  = htonl(GoodsID);
		Price    = htonl(Price);
		Qtty     = htonl(Qtty);
	}
	int32 ID;
	int32 OrderID;
	int32 GoodsID;
	int32 Price;
	int32 Qtty;
};

struct SpQuotKindStruc : public SpBaseStruc {
	SpQuotKindStruc() {Init();}
	void Init() {ID = 0; memzero(Name, sizeof(Name));}
	int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
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
	SpClientDebtStruc() {Init();}
	virtual void ToHostRec()
	{
		ClientID = ntohl(ClientID);
		Dt       = ntohs(Dt);
		Amount   = ntohl(Amount);
		Debt     = ntohl(Debt);
	}
	virtual int FromBuf(const void * pBuf)
	{
		const char * p_buf = (const char*)pBuf;
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
	SpClientSalesStruc() {P_Items = 0; Init();}
	~SpClientSalesStruc() {ZDELETE(P_Items);}
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
		const char * p_buf = (const char*)pBuf;
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
		CATCH
			ok = 0;
		ENDCATCH
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
	SpInvHeaderStruc() {Init();}
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
				char * p_buf = (char*)pBuf;
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
	SpInvLineStruc() {Init();}
	void Init()	{InvID = GoodsID = Qtty = 0;}
	virtual int ToBuf(void * pBuf, size_t * pBufSize)
	{
		int ok = 0;
		if(pBufSize) {
			if(*pBufSize < 12) {
				*pBufSize = 12;
				ok = -1;
			}
			else {
				char * p_buf = (char*)pBuf;
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
#	define SPTBLNAM_GOODSIDIDX         "GdsID.idx"
#	define SPTBLNAM_GOODSNAMIDX        "GdsName.idx"
#	define SPTBLNAM_GOODSGRPIDNAMIDX   "GdsGNam.idx"
#	define SPTBLNAM_GOODSGRPIDIDIDX    "GdsGID.idx"
#	define SPTBLNAM_GOODSCODEIDX       "GdsCode.idx"
#define SPTBLNAM_GOODSGRP              "GoodsGrp.tbl"
#	define SPTBLNAM_GOODSGRPIDIDX      "GroupID.idx"
#	define SPTBLNAM_GOODSGRPNAMIDX     "GroupNam.idx"
#define SPTBLNAM_CLIENT                "Client.tbl"
#	define SPTBLNAM_CLIENTIDIDX        "ClID.idx"
#	define SPTBLNAM_CLIENTNAMIDX       "ClNam.idx"
#define SPTBLNAM_QUOTKIND              "QuotKind.tbl"
#define SPTBLNAM_CLIENTDEBT            "CliDebt.tbl"
#	define SPTBLNAM_CLIENTDEBTIDDTCODE "CdIdDtC.idx"
#define SPTBLNAM_ORDHEADER             "OrdHdr.tbl"
#	define SPTBLNAM_ORDHEADERIDIDX     "OrdHID.idx"
#	define SPTBLNAM_ORDHEADERDATEIDIDX "OrdHDtID.idx"
#define SPTBLNAM_ORDLINE               "OrdLine.tbl"
#	define SPTBLNAM_ORDLINEORDGOODSIDX "OLOGoods.idx"
#	define SPTBLNAM_ORDLINEORDIDIDX    "OLOID.idx"
#define SPTBLNAM_INVHEADER             "InvHdr.tbl"
#	define SPTBLNAM_INVHEADERIDIDX     "InvHID.idx"
#	define SPTBLNAM_INVHEADERDATEIDX   "InvHDt.idx"
#	define SPTBLNAM_INVHEADERCLIDADRID "InvHClAdr.idx"
#define SPTBLNAM_INVLINE               "InvLine.tbl"
#	define SPTBLNAM_INVLINEINVGOODSIDX "ILIGoods.idx"
#define SPTBLNAM_SALES                 "CliSell.tbl"
#define SPTBLNAM_CFG                   "Config.tbl"
#define SPTBLNAM_TODO                  "ToDoDB"
#define SPTBLNAM_TODOASSOC             "ToDoSPII.dat"
#define SPTBLNAM_NETCONFIG             "NetCSPII.tbl"
#	define SPTBLNAM_NETCONFIGIDIDX     "NetCID.idx"
#define SPTBLNAM_BRAND                 "Brand.tbl"
#define SPTBLNAM_LOC                   "Location.tbl"
#endif
