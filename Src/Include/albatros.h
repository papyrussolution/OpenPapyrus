// ALBATROS.H
// Copyright (c) A.Starodub 2002, 2003, 2006, 2007
//
#ifndef __ALBATROS_H
#define __ALBATROS_H

#include <slib.h>

#define ALBATROS_MAXTAGVALSIZE 256
#define ALBATROS_MAXTAGSIZE    64
#define ALBATROS_MAXTAGNAMESSIZE 2048

struct AlbatrosOrderHeader {
	LDATE OrderDate;
	char  OrderCode[16]; // !!!
	long  ClientID;
	char  ClientName[48];
	char  ClientINN[48];
	char  ClientCity[48];
	char  ClientAddr[64];
	char  ClientPhone[48];
	char  ClientMail[48];
	char  ClientBankAcc[128];
	double OrderAmount;      // Total Amount = Sum(Items[].Amount)
	double PctDis;           // Discount in percent
};

struct AlbatrosOrderItem {
	long   GoodsID;
	char   GoodsName[64];
	char   GoodsCode[24];
	double UnitsPerPack;     // Емкость упаковки
	double Qtty;
	double Price;
	double Discount;         // = 0
	double Amount;           // = Qtty * (Price - Discount)
};

struct AlbatrosOrder {
	void SLAPI Init();
	AlbatrosOrderHeader Head;
	TSArray <AlbatrosOrderItem> Items;
};

class AlbatrosTagParser : XTagParser {
public:
	SLAPI  AlbatrosTagParser();
	SLAPI ~AlbatrosTagParser();
	int    SLAPI ProcessNext(AlbatrosOrder * pOrder, const char * pPath);     // @sobolev
	int    SLAPI ResolveClientID(PPID inID, PPID opID, AlbatrosOrderHeader * pHead, PPID * pOutID, int use_ta);
	int    SLAPI ResolveArticleByPerson(PPID psnID, PPID opID, PPID * pOutID, int use_ta);
	int    SLAPI ConfirmClientAdd(PPPersonPacket * pPack, const char * pClientINNInOrder, int add);
protected:
	virtual int SLAPI ProcessTag(const char * pTag, long);
private:
	int SLAPI LoadPersonPacket(AlbatrosOrderHeader * pHead, PPID albClID, PPPersonPacket * pPacket, int update, int use_ta);
	int SLAPI SaveTagVal(const char * pTag);
	AlbatrosOrder * P_Order;
	AlbatrosOrderItem OrderItem;
	char * P_TagValBuf;
	SString TagNamesStr;
	int    SymbNum;
	PPObjPerson   PsnObj; // @sobolev
	PPObjRegister RegObj; // @sobolev
	PPObjArticle  ArObj;
};

#endif
