// PPEDI.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2018
//
#include <pp.h>
#pragma hdrstop

class PPEdiProcessor {
public:
	struct Packet {
		Packet(int docType);
		~Packet();
		int    DocType;
		long   Flags;
		void * P_Data;
	};
	struct DocumentInfo {
		SLAPI  DocumentInfo();
		enum {
			statusUnkn = 0,
			statusNew  = 1
		};
		int    ID;          // Идентификатор сообщения, инициализируемый дравером конкретного провайдера (не зависимо от провайдера)
		int    EdiOp;       // Тип EDI-операции
		LDATETIME Time;     // Время создания/модификации сообщения
		S_GUID Uuid;        // GUID сообщения
		long   Status;      // DocumentInfo::statusXXX
		long   Flags;       // Флаги
		long   PrvFlags;    // Флаги, специфичные для конкретного провайдера
		SString Code;       // Код сообщения (номер документа)
		SString SenderCode; // Код отправителя
		SString RcvrCode;   // Код получателя  
		SString Box;        // Если хранение сообщений дифференцировано по боксам, то здесь может быть имя бокса для сообщения
		SString SId;        // Символьный идентификатор (может быть именем файла)
	};
	class DocumentInfoList : private SStrGroup {
	public:
		SLAPI  DocumentInfoList();
		uint   SLAPI GetCount() const;
		int    SLAPI GetByIdx(uint idx, DocumentInfo & rItem) const;
		int    SLAPI Add(const DocumentInfo & rItem, uint * pIdx);
	private:
		struct Entry {
			int    ID;
			int    EdiOp;
			LDATETIME Dtm;
			S_GUID Uuid;
			long   Status;
			long   Flags;
			long   PrvFlags;
			uint   CodeP;
			uint   SenderCodeP;
			uint   RcvrCodeP;
			uint   BoxP;
			uint   SIdP;
		};
		TSVector <Entry> L;
	};
	class ProviderImplementation {
	public:
		SLAPI  ProviderImplementation(const PPEdiProviderPacket & rEpp) : Epp(rEpp)
		{
		}
		virtual SLAPI ~ProviderImplementation()
		{
		}
		virtual int    SLAPI  GetDocumentList(DocumentInfoList & rList)
		{
			return -1;
		}
		virtual int    SLAPI  ReceiveDocument(const DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
		{
			return -1;
		}
		virtual int    SLAPI  SendDocument(DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
		{
			return -1;
		}
	protected:
		PPEdiProviderPacket Epp;
	};
	SLAPI  PPEdiProcessor();
	SLAPI ~PPEdiProcessor();
	int    SLAPI SendBills(const PPBillExportFilt & rP);
};

class EdiProviderImplementation_Kontur : public PPEdiProcessor::ProviderImplementation {
public:
	SLAPI  EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp);
	virtual SLAPI ~EdiProviderImplementation_Kontur();
	virtual int    SLAPI  GetDocumentList(PPEdiProcessor::DocumentInfoList & rList);
	virtual int    SLAPI  ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack);
	virtual int    SLAPI  SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack);
};

PPEdiProcessor::Packet::Packet(int docType) : DocType(docType), Flags(0), P_Data(0)
{
}

PPEdiProcessor::Packet::~Packet()
{
}

SLAPI PPEdiProcessor::PPEdiProcessor()
{
}

SLAPI PPEdiProcessor::~PPEdiProcessor()
{
}

int SLAPI PPEdiProcessor::SendBills(const PPBillExportFilt & rP)
{
	int    ok = -1;
	return ok;
}
//
//
//
SLAPI EdiProviderImplementation_Kontur::EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp) : PPEdiProcessor::ProviderImplementation(rEpp)
{
}

SLAPI EdiProviderImplementation_Kontur::~EdiProviderImplementation_Kontur()
{
}

int SLAPI EdiProviderImplementation_Kontur::GetDocumentList(PPEdiProcessor::DocumentInfoList & rList)
{
	int    ok = -1;
	SString temp_buf;
	ScURL  curl;
	Epp.GetExtStrData(Epp.extssAddr, temp_buf);
	if(!temp_buf.NotEmptyS()) {
		Epp.GetExtStrData(Epp.extssAddr2, temp_buf);
	}
	THROW(temp_buf.NotEmptyS());

	CATCHZOK
	return ok;
}

int SLAPI EdiProviderImplementation_Kontur::ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	return -1;
}

int SLAPI EdiProviderImplementation_Kontur::SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	return -1;
}
