// SXML.H
// Copyright (c) A.Sobolev 2015, 2016, 2017, 2019, 2020, 2022, 2023
// @codepage UTF-8
//
#ifndef __SXML_H
#define __SXML_H

#include <slib.h>
#include <libxml\xmlwriter.h>
#include <libxml\xmlreader.h>
#include <libxml\xmlsave.h>
//
//
//
class SXmlValidationMessageList : SStrGroup {
public:
	static void __cdecl SchemaValidityError(void * pCtx, const char * pMsg, ...);
	static void __cdecl SchemaValidityWarning(void * pCtx, const char * pMsg, ...);
	//typedef void (XMLCDECL *xmlSchemaValidityErrorFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);
	//typedef void (XMLCDECL *xmlSchemaValidityWarningFunc)(void * ctx, const char * msg, ...) LIBXML_ATTR_FORMAT(2, 3);

	SXmlValidationMessageList();
	int    AddMessage(int type, const char * pMsg);
	uint   GetMessageCount() const;
	int    GetMessageByIdx(uint idx, int * pType, SString & rMsg) const;
private:
	struct EntryInner {
		int   Type;
		uint  MsgP;
	};
	TSVector <EntryInner> L;
};

class SXml {
public:
	//
	// Descr: Формирует строку вида "ns:tag" из компонентов pNs и pT соответственно.
	// Note: Функция экспериментальная.
	// Returns: 
	//   Ссылка на результирующий буфер, полученный из локального к потоку револьверного хранилища.
	//
	static const SString & FASTCALL nst(const char * pNs, const char * pT);
	static const SString & FASTCALL nst_xmlns(const char * pT); // SXml::nst("xmlns", pT)
	static const SString & Helper_EncXmlText(const SString & rS, int ctransf, SString & rBuf);
	static const SString & Helper_EncXmlText(const char * pS, int ctransf, SString & rBuf);

	class WDoc {
	public:
		WDoc(xmlTextWriter * pWriter, SCodepage cp);
		//
		// Descr: Создает документ без пролога 
		//
		explicit WDoc(xmlTextWriter * pWriter);
		~WDoc();
		operator xmlTextWriter * () { return Lx; }
	private:
		enum {
			stStarted    = 0x0001,
			stSkipHeader = 0x0002 // Документ не содержит пролога <?xml version="1.0" encoding="UTF-8"?>
		};
		long   State;
		xmlTextWriter * Lx;
	};
    class WNode {
	public:
		static SString & FASTCALL CDATA(SString & rBuf);
        WNode(xmlTextWriter * pWriter, const char * pName);
        WNode(xmlTextWriter * pWriter, const char * pName, const SString & rValue);
        WNode(xmlTextWriter * pWriter, const char * pName, const char * pValue);
        ~WNode();
        int    PutAttrib(const char * pName, const char * pValue);
        int    PutAttribSkipEmpty(const char * pName, const char * pValue);
		//
		// Descr: Добавляет к узлу this атрибут вида: "xmlns:pNs"="pDomain/pPath"
		//
		int    PutAttrib_Ns(const char * pNs, const char * pDomain, const char * pPath);
        int    PutInner(const char * pInnerName, const char * pInnerValue);
        int    PutInnerSkipEmpty(const char * pInnerName, const char * pInnerValue);
		int    PutInnerValidDate(const char * pInnerName, LDATE dt, long fmt);
        int    SetValue(const SString & rText);
	private:
		int    Construct(xmlTextWriter * pWriter, const char * pName);

		enum {
			stStarted = 0x0001
		};
		long   State;
		xmlTextWriter * Lx;
        SString Name;
    };

	static int Validate(const char * pXsdFileName, const char * pXmlFileName, SXmlValidationMessageList * pMsgList);
    static bool FASTCALL IsName(const xmlNode * pNode, const char * pName);
    static bool FASTCALL IsContent(const xmlNode * pNode, const char * pText);
    static bool FASTCALL GetContent(const xmlNode * pNode, SString & rResult);
    //
    // Descr: Функция, совмещающая вызов
    // {
    //    if(SXml::IsName(pNode, pName))
    //       SXml::GetContent(pNode, rResult);
    // }
    // Returns:
    //    >0 - узел pNode имеет имя pName и не пустое содержание, которое помещено в rResult
    //    <0 - узел pNode имеет имя pName, но содержание пустое (rResult = 0)
    //    0  - узел pNode не имеет имя pName. В этом случае rResult не меняется.
    //
    static int  GetContentByName(const xmlNode * pNode, const char * pName, SString & rResult);
    static bool GetAttrib(const xmlNode * pNode, const char * pAttr, SString & rResult);
};
//
// Descr: (ситуативный) Класс, реализующий обертку вокруг xmlTexWriter
//
class SXmlWriter {
public:
	SXmlWriter();
	~SXmlWriter();
	operator xmlTextWriter * () const { return P_Writer; }
	operator xmlBuffer * () const { return P_XmlBuf; }
private:
	xmlBuffer * P_XmlBuf;
	xmlTextWriter * P_Writer;
};
//
//
//
class SXmlSaxParser {
protected:
	enum {
		fStartDocument = 0x0001,
		fEndDocument   = 0x0002,
		fStartElement  = 0x0004,
		fEndElement    = 0x0008,
		fCharacters    = 0x0010
	};
	explicit SXmlSaxParser(long flags);
	~SXmlSaxParser();
	int    ParseFile(const char * pFileName);
	void   SaxStop();
	virtual int StartDocument();
	virtual int EndDocument();
	virtual int StartElement(const char * pName, const char ** ppAttrList);
	virtual int EndElement(const char * pName);
	virtual int Characters(const char * pS, size_t len);

	enum {
		stError = 0x0002
	};
	int    State;
	long   Flags;
	SString TagValue;
	SString SrcFileName;
	xmlParserCtxt * P_SaxCtx;
private:
	static void Scb_StartDocument(void * ptr);
	static void Scb_EndDocument(void * ptr);
	static void Scb_StartElement(void * ptr, const xmlChar * pName, const xmlChar ** ppAttrList);
	static void Scb_EndElement(void * ptr, const xmlChar * pName);
	static void Scb_Characters(void * ptr, const uchar * pC, int len);
};
//
//
//
#endif // __SXML_H
