// PPEDI.H
// Copyright (c) A.Sobolev 2015, 2021
// @codepage UTF-8
//
#ifndef __PPEDI_H // {
#define __PPEDI_H

#include <sxml.h>
#include <ppdefs.h>

struct PPEdiMessageEntry { // @flat
	PPEdiMessageEntry();
	enum {
		statusUnkn = 0,
		statusNew  = 1
	};
	int    ID;             // Идентификатор сообщения, инициализируемый дравером конкретного провайдера (не зависимо от провайдера)
    int    EdiOp;          // Тип EDI-операции
    LDATETIME Dtm;         // Время создания/модификации сообщения
    S_GUID Uuid;           // GUID сообщения
    long   Status;         // PPEdiMessageEntry::statusXXX
    long   Flags;          // Флаги
    long   PrvFlags;       // Флаги, специфичные для конкретного провайдера
    char   Code[24];       // Код сообщения (номер документа)
    char   SenderCode[24];
    char   RcvrCode[24];
    char   Box[64];        // Если хранение сообщений дифференцировано по боксам, то здесь может быть имя бокса для сообщения
    char   SId[128];       // Символьный идентификатор (может быть именем файла)
};

class PPEdiMessageList : public TSVector <PPEdiMessageEntry> {
public:
	PPEdiMessageList();
	PPEdiMessageList & Z();
	int    SearchId(int id, uint * pPos) const;
	int    Add(const PPEdiMessageEntry & rEntry);
private:
    int    LastId;
};

class SEancomXmlSegment : public SXml {
public:
	explicit SEancomXmlSegment(const xmlNode * pNode = 0);
	int    operator !() const;

	enum {
		refON = 1, // "ON" Order number
        refIT,     // "IT" Internal customer number
        refYC1,    // "YC1" Additional party identification (EAN code)
        refABT,    // "ABT" Custom declaration number (RUS: номер ГТД в SG17)
        refIV      // "IV"
	};
	struct REF {
		REF();
		REF & Z();
        int    Type;
        SString Text;
	};
	enum {
		dtmMsg    = 137,     // Дата/время документа/сообщения //
		dtmRef    = 171,     // Ссылочная дата/время (дата документа, на который ссылается это сообщение)
		dtmSched  = 358,     // Планируемая дата/время
		dtmDlvr   = 35,      // Delivery date/time actual
		dtmRcpt   = 50,      // Goods receipt date/time
		dtmExpiry = 361,     // Дата истечения срока годности
		dtmExpiryDays = 36   // Срок годности в днях
	};
	struct DTM {
		DTM();
		DTM & Z();
		int    Type; // dtmXXX
		LDATETIME Dtm;
		int    Days;
	};
	struct MOA {
		MOA();
		MOA & Z();
		int    Type;
		char   CurrencySymb[8];
		double Value;
	};
	enum {
		nadSU = 1, // Поставщик
		nadBY,     // Покупатель
		nadDP,     // Delivery Party
		nadUD,     // Ultimate Delivery party
		nadIV      // Плательщик
	};
	struct NAD {
		NAD();
		NAD & Z();
        int    Type;
        char   GLN[20];
        char   CountryCode[8];
        char   PostalCode[20];
        SString Name;
        SString City;
        SString Address;
	};
	struct LIN {
		SString Ident; // E1082
	};

	int    GetNext(SEancomXmlSegment & rSeg);
	int    Is(const char * pName) const;
	int    IsContent(const char * pText) const;
	SString & GetContent(SString & rBuf) const;
	int    GetMsgTypeName() const;
	int    GetMsgTypeContent() const;
	int    GetMsgType(const char * pText) const;
	int    GetInt(int & rVal) const;
	int    GetReal(double & rVal) const;
	int    GetText(SString & rText) const;
	int    GetREF(REF & rR);
	int    GetDate(DTM & rD);
	int    GetMOA(MOA & rM);
	int    GetNAD(NAD & rN);
private:
	int    IsText(const char * pContent, const char * pText) const;
	int    Set(const xmlNode * pNode);

	const xmlNode * P_Node;
	const xmlNode * P_Cur;
};

#endif // } __PPEDI_H
