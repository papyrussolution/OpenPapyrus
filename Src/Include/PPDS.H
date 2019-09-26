// PPDS.H
// Copyright (c) A.Sobolev 2001-2002, 2007, 2009, 2011, 2015, 2016
// @codepage windows-1251
//
#define FIRST_DECLSTRUC_ID 1001L

#define DSIF_REQUIRED 0x0001L // Обязательное поле
#define DSIF_LIST     0x0002L // Список значений

struct PPDeclStrucItem {
	int32  ID;
	char   Name[36];
	int32  Flags;
	int32  Type;
	char   Reserve[16];
};

class PPDeclStruc : SArray {
public:
	enum Ido {    // InitData op
		idoAlloc = 1,
		idoDestroy,
		idoExtract,
		idoAccept
	};
	enum Tfd {    // Transfer field dir
		tfdBufToData = 0,
		tfdDataToBuf
	};
	static PPDeclStruc * SLAPI CreateInstance(long typeID, void * extraPtr, const PPDeclStruc * pOuter, PPLogger * pLogger);

	SLAPI  PPDeclStruc();
	virtual SLAPI ~PPDeclStruc();
	uint   SLAPI GetCount() const;
	long   SLAPI GetId() const;
	int    SLAPI SetHeader(PPID, const char * pName, long flags);
	int    SLAPI AddItem(PPDeclStrucItem *);
	int    SLAPI GetItem(uint idx, PPDeclStrucItem *);
	int    SLAPI EnumItems(uint * pIdx, PPDeclStrucItem *);
	int    SLAPI SearchItem(const char * pName, PPDeclStrucItem *);
	int    SLAPI WriteDataToStream(FILE *, void * extraPtr);
	void   SLAPI SetItemAddedMsgString(const char * pItemName);
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd, uint *, SString & rBuf);
	//
	// Descr: Принимает очередной элемент списка fldID, считанный из потока
	//
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	//
	// Descr: Создает очередной элемент списка fldID для записи в поток
	//
	virtual int SLAPI CreateListItem(long fldID, uint * pIter, PPDeclStruc *);

	enum {
		updNone = 0,
		updForce = 1
	};
	int    UpdateProtocol;
	LongArray AcceptedFields;
protected:
	int    SLAPI TransferData(int32 *,  Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(int16 *,  Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(uint16 *, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(uint8  *, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(double *, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(char *, size_t, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(SString & rData, Tfd dir, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(DateRange * pData, Tfd dir, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(LDATE *, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(LTIME *, Tfd dir, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(long *, long fl, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferData(PPIDArray *, Tfd, uint, SString & rBuf);
	int    SLAPI TransferDataFlag(int16 *, long flag, Tfd, SString & rBuf, int skipZero = 0);
	int    SLAPI TransferDataFlag(int32 *, long flag, Tfd, SString & rBuf, int skipZero = 0);

	long   Id; // Идентификатор типа структуры
	const  PPDeclStruc * P_Outer;
	PPLogger * P_Logger; // @notowned @v6.5.0
	SString TempBuf; // Временный буфер для использования порожденными классами (ради ускорения)
private:
	int    SLAPI LoadFromResource(long);
	int    SLAPI Helper_WriteDataToStream(FILE *, int putPrefix, int level, void * extraPtr);
	int    SLAPI Helper_CreateListItem(long, long, uint *, FILE *, int level, void * extraPtr);
};

enum PPDeclStrucTerm {
	ppdsUnknown = -1,
	ppdsData = 1,
	ppdsDeclare,
	ppdsEqual,
	ppdsLeftBrace,
	ppdsRightBrace,
	ppdsLeftBracket,
	ppdsRightBracket,
	ppdsAsterisk,
	ppdsColon
};

class PPDeclStrucProcessor {
public:
	SLAPI  PPDeclStrucProcessor();
	SLAPI ~PPDeclStrucProcessor();
	int    SLAPI ParseDefinition(const char * pFileName, const char * pOutRcFileName, const char * pOutChFileName);
	int    SLAPI InitDataParsing(const char * pFileName);
	int    SLAPI NextDecl(PPDeclStruc ** ppDecl, int dontAccept = 0);
	int    SLAPI ReleaseDataParsing();
	int    SLAPI MakeErrMsgString(char * pBuf, size_t bufLen);
private:
	struct NameTabItem {
		int32  ID;
		char   Name[36];
	};
	struct P {
		long   Pos;
		long   Lin;
	};
	int    SLAPI GetTerm(int * pTerm = 0);
	PPDeclStruc * SLAPI GetData(long typeID, void * extraPtr, const PPDeclStruc * pOuter, int dontAccept);
	int    SLAPI GetChar(int skipWS = 0);
	int    SLAPI UngetChar();
	int    SLAPI GetWord();
	int    SLAPI SavePos(P *);
	int    SLAPI RestorePos(const P *);
	int    SLAPI GetValue(SString &);
	int    SLAPI LoadNameTabFromResource();
	int    SLAPI AddTypeName(long typeID, const char * pName);
	int    SLAPI GetTypeByName(const char *, long *);
	int    SLAPI GetNameByType(long, char *, size_t bufLen);
	int    SLAPI GetNewTypeID(long * pTypeID);
	int    SLAPI WriteDeclToFiles(FILE * pRcStream, FILE * pChStream, PPDeclStruc *);

	SString WordBuf;
	char * P_FileName;
	FILE * InStream;
	P      ChrP;
	long   Line;
	SArray * P_NameTab;
#ifndef PPDSC
	ObjTransmContext Ctx;
	PPLogger Logger;
#endif
};

/*
	Формат представления декларации данных в ресурсах:

	Resource Type: PP_RCDECLSTRUC
	Resource ID:   PPDS_XXX

	Declaration name: "Some_Name\0"
	Items count:      uint
	{
		Item ID:          uint
		Item Name:        "Some_Item_Name\0"
		Item Flags:       uint
		Item Type ID:     uint
	} Items Count Times
*/
//
// Декларации прикладных классов, соответствующих структурам данных
//
class PPDS_CrrAddress : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrAddress();
	LocationTbl::Rec Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	SString CountryName;
	SString CityName;
	SString Address;
	PPObjWorld WObj;
};

class PPDS_CrrBnkAcct : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrBnkAcct() : PPDeclStruc() {}
	// @v9.0.4 BankAccountTbl::Rec Data;
	PPBankAccount Data; // @v9.0.4
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	BnkAcctData IntrData;
};

class PPDS_ELinkAddr : public PPDeclStruc {
public:
	SLAPI  PPDS_ELinkAddr() : PPDeclStruc() {}
	PPELink Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	PPID   KindID;
	char   KindName[48];
	char   Addr[128];    // @v6.3.10 [48]-->[128]
};

class PPDS_CrrPerson : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrPerson() : PPDeclStruc(), OrigID(0) {}
	PPPersonPacket Data;
	PPID   OrigID; // При приеме структуры из файла это поле аполняется из поля "ID"
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);
};

class PPDS_Barcode : public PPDeclStruc {
public:
	SLAPI  PPDS_Barcode() : PPDeclStruc() {}
	BarcodeTbl::Rec Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
};

class PPDS_CrrGoods : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrGoods() : PPDeclStruc() {}
	PPGoodsPacket Data;
	PPID   OrigID; // При приеме структуры из файла это поле заполняется из поля "ID"
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);
	int    SLAPI ExtractOuterData();
	int    SLAPI AcceptOuterData(int use_ta);

	char   UnitName[48];
	char   PhUnitName[48];
	double PhUPerU;
	char   GroupName[128];   // @v6.3.10 [64]-->[128]
	char   ManufName[128];   // @v6.3.10 [48]-->[128]
	char   ManufCountry[48];

	PPObjGoods GObj;
};

class PPDS_CrrQCert : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrQCert() : PPDeclStruc() {}
	QualityCertTbl::Rec Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjQCert QcObj;
	char   RegOrgName[128]; // @v6.3.10 [48]-->[128]
};

class PPDS_CrrBillItem : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrBillItem();

	PPTransferItem Data;
	char   CLB[64];
private:
	//
	// ARG(dataParam IN): Указатель на структуру PPTransferItem
	//
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);
};
/*
	Для корректного приема документа должны быть выполнены следующие условия:
	- В настроечных параметрах должна быть указана операция прихода товара от поставщика
	- Операция прихода товара от поставщика должна иметь таблицу объектов, ссылающуюся на
  	  персоналии стандартного вида PPPRK_SUPPL (Поставщики)
	- Таблица объектов операции прихода товара от поставщика должна иметь
	  установленный признак "Создавать статью при создании объекта"

	- Покупатель, указанный в документе должен иметь тот же ИНН, что и текущая //
	  установленная главная организация в принимающей базе данных
*/
class PPDS_CrrBill : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrBill();

	PPBillPacket Data;
	PPPersonPacket Seller;
	PPPersonPacket Buyer;
	PPID   SellerArID;
	LocationTbl::Rec DlvrAddr;
	PPDBXchgConfig DbxCfg;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);
	int    SLAPI IdentifySuppl(PPID * pArID, int use_ta);

	PPObjPerson PsnObj;
	PPObjArticle ArObj;
	PPObjGoods GObj;
};

class PPDS_CrrAmountType : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrAmountType() : PPDeclStruc() {}
	PPAmountTypePacket Pack;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjAmountType AmtObj;
};

class PPDS_CrrSalCharge : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrSalCharge() : PPDeclStruc() {}
	PPSalChargePacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjSalCharge Obj;
};

class PPDS_CrrSalChargeGroup : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrSalChargeGroup() : PPDeclStruc() {}
	PPSalChargePacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	PPObjSalCharge Obj;
};

class PPDS_CrrStaffCalEntry : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrStaffCalEntry() : PPDeclStruc() {}
	StaffCalendarTbl::Rec Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	LTIME  Duration;
	PPObjStaffCal Obj;
};

class PPDS_CrrStaffCal : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrStaffCal() : PPDeclStruc() {}
	PPStaffCalPacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData);
	PPObjStaffCal Obj;
};

class PPDS_CrrDbDiv : public PPDeclStruc {
public:
	SLAPI  PPDS_CrrDbDiv() : PPDeclStruc() {}
	DBDivPack Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjDBDiv Obj;
	PPObjLocation LocObj;
};

// @v5.7.1 AHTOXA {
class PPDS_CrrBarcodeStruc : public PPDeclStruc {
public:
	SLAPI PPDS_CrrBarcodeStruc() : PPDeclStruc() {}
	PPBarcodeStruc Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjBarCodeStruc Obj;
};

class PPDS_CrrGoodsType : public PPDeclStruc {
public:
	SLAPI PPDS_CrrGoodsType() : PPDeclStruc() {}
	PPGoodsType Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	int SLAPI TransferAmtType(PPID * pAmttID, Tfd dir, SString & rBuf);

	PPObjAmountType AmtTypeObj;
	PPObjAssetWrOffGrp AsstWrOffObj;
	PPObjGoodsType  Obj;
};

class PPDS_CrrFormula : public PPDeclStruc {
public:
	struct Formula {
		PPID ID;
		SString Name;
		SString Expr;
	};

	SLAPI PPDS_CrrFormula() : PPDeclStruc() {}
	Formula Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjFormula Obj;
};

class PPDS_CrrScale : public PPDeclStruc {
public:
	SLAPI PPDS_CrrScale() : PPDeclStruc() {}
	PPScalePacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	char StrPort[16];
	PPObjGoodsGroup GGObj;
	PPObjLocation   LocObj;
	PPObjQuotKind   QKObj;
	PPObjScale      Obj;
};

class PPDS_CrrRegisterType : public PPDeclStruc {
public:
	PPDS_CrrRegisterType() : PPDeclStruc() {}
	PPRegisterType Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjPersonKind   PsnKObj;
	PPObjOpCounter    OpCounterObj;
	PPObjRegisterType Obj;
};

class PPDS_CrrQuotKind : public PPDeclStruc {
public:
	SLAPI PPDS_CrrQuotKind() : PPDeclStruc() {}
	PPQuotKind Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjAccSheet ACCSObj;
	PPObjOprKind  OpKObj;
	PPObjQuotKind Obj;
};

class PPDS_CrrPersonKind : public PPDeclStruc {
public:
	PPDS_CrrPersonKind() : PPDeclStruc() {}
	PPPersonKind Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjRegisterType ObjRegT;
	PPObjPersonKind   Obj;
};

class PPDS_CrrCurrency : public PPDeclStruc {
public:
	SLAPI PPDS_CrrCurrency() : PPDeclStruc() {}
	PPCurrency Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjCurrency Obj;
};

class PPDS_CrrCurRateType : public PPDeclStruc {
public:
	PPDS_CrrCurRateType() : PPDeclStruc() {}
	PPCurRateType Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjCurRateType Obj;
};

class PPDS_CrrAssetWrOffGrp : public PPDeclStruc {
public:
	PPDS_CrrAssetWrOffGrp() : PPDeclStruc() {}
	PPAssetWrOffGrp Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjAssetWrOffGrp Obj;
};

class PPDS_CrrMailAccount : public PPDeclStruc {
public:
	PPDS_CrrMailAccount() : PPDeclStruc() {}
	PPInternetAccount Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	int SLAPI TransferMailField(int mailFldID, Tfd dir, SString & rBuf);

	PPObjPerson PsnObj;
	PPObjInternetAccount Obj;
};

class PPDS_CrrPersonRelType : public PPDeclStruc {
public:
	PPDS_CrrPersonRelType() : PPDeclStruc() {}
	PPPersonRelTypePacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjRegisterType  ObjRegT;
	PPObjPersonRelType Obj;
};

class PPDS_CrrObjTag : public PPDeclStruc {
public:
	PPDS_CrrObjTag() : PPDeclStruc() {}
	PPObjTagPacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjTag Obj;
};

class PPDS_CrrDraftWrOffEntry : public PPDeclStruc {
public:
	PPDS_CrrDraftWrOffEntry() : PPDeclStruc() {}
	PPDraftWrOffEntry Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjLocation LocObj;
	PPObjOprKind  OpKObj;
};

class PPDS_CrrDraftWrOff : public PPDeclStruc {
public:
	PPDS_CrrDraftWrOff() : PPDeclStruc() {}
	PPDraftWrOffPacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjArticle    ArObj;
	PPObjOprKind    OpKObj;
	PPObjDraftWrOff Obj;
};
// } @v5.7.1 AHTOXA

// @v6.2.x AHTOXA {
class PPDS_CrrLocation : public PPDeclStruc {
public:
	PPDS_CrrLocation() : PPDeclStruc() {}
	LocationTbl::Rec Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjLocation Obj;
};

class PPDS_CrrAccSheet : public PPDeclStruc {
public:
	PPDS_CrrAccSheet() : PPDeclStruc() {}
	PPAccSheet Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjAccSheet Obj;
};

class PPDS_CrrAccount : public PPDeclStruc {
public:
	PPDS_CrrAccount() : PPDeclStruc() {}
	PPAccountPacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjAccount Obj;
};

class PPDS_CrrAcctEntry : public PPDeclStruc {
public:
	PPDS_CrrAcctEntry() : PPDeclStruc() {}
	AcctID Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjAccount Obj;
};

class PPDS_CrrArticle : public PPDeclStruc {
public:
	PPDS_CrrArticle() : PPDeclStruc() {}
	ArticleTbl::Rec Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjArticle  Obj;
	PPObjAccSheet AccSheetObj;
};

class PPDS_CrrAccturnTempl : public PPDeclStruc {
public:
	PPDS_CrrAccturnTempl() : PPDeclStruc() {}
	PPAccTurnTempl  Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);
};

class PPDS_CrrOprKindEntry : public PPDeclStruc {
public:
	PPDS_CrrOprKindEntry() : PPDeclStruc() {}
	PPOprKind       Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPOprKindPacket Pack;
	PPObjOprKind    Obj;
};

class PPDS_CrrInvOpExEntry : public PPDeclStruc {
public:
	PPDS_CrrInvOpExEntry() : PPDeclStruc() {}
	PPInventoryOpEx Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPOprKindPacket Pack;
	PPObjOprKind    Obj;
};

class PPDS_CrrReckonOpExEntry : public PPDeclStruc {
public:
	PPDS_CrrReckonOpExEntry() : PPDeclStruc() {}
	PPReckonOpEx Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPOprKindPacket Pack;
	PPObjOprKind    Obj;
};

class PPDS_CrrDraftOpExEntry : public PPDeclStruc {
public:
	PPDS_CrrDraftOpExEntry() : PPDeclStruc() {}
	PPDraftOpEx Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPOprKindPacket Pack;
	PPObjOprKind    Obj;
};

class PPDS_CrrBillPoolOpExEntry : public PPDeclStruc {
public:
	PPDS_CrrBillPoolOpExEntry() : PPDeclStruc() {}
	PPBillPoolOpEx Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPOprKindPacket Pack;
	PPObjOprKind    Obj;
};

class PPDS_CrrOprKind : public PPDeclStruc {
public:
	PPDS_CrrOprKind() : PPDeclStruc() {}
	PPOprKindPacket Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPObjOprKind    Obj;
};
// } @v6.2.x AHTOXA

class PPDS_CrrBillStatus : public PPDeclStruc {
public:
	PPDS_CrrBillStatus() : PPDeclStruc() {}

	PPBillStatus Data;
private:
	virtual int SLAPI InitData(Ido op, void * dataPtr, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	PPObjBillStatus Obj;
	PPObjOprKind    OpKObj;
	PPObjOpCounter  ObjCntr;
};
