// SXML.CPP
// Copyright (c) A.Sobolev, 2002, 2007, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// Вспомогательные механизмы для работы с XML
//
#include <slib-internal.h>
#pragma hdrstop
// @v11.7.0 #include <snet.h>
#include <sxml.h>
#include <libxml/xmlschemastypes.h>

struct SpcSymbEntry {
	char   chr;
	int8   Amp;
	const  char * str;
};

//     ,   .   /   \
// "\x2c\x2e\x2f\x5c"

static const SpcSymbEntry SpcSymbTab[] = {
	{ '\x26', 1, "amp"    },  // & @anchor Обязательно на первом месте (замещающие строки содержат &)
	{ '\x21', 0, "exclam" },  // !
	{ '\x23', 0, "sharp"  },  // #
	{ '\x25', 0, "pct"    },  // %
	{ '\x27', 0, "apos"   },  // '
	{ '\x2c', 0, "comma"  },  // , +-@v7.6.4 @Muxa
	{ '\x2e', 0, "dot"    },  // . +-@v7.6.4 @Muxa
	{ '\x2f', 0, "fwsl"   },  // / +-@v7.6.4 @Muxa
	{ '\x3c', 1, "lt"     },  // <
	{ '\x3d', 0, "eq"     },  // =
	{ '\x3e', 0, "gt"     },  // >
	{ '\x3f', 0, "ques"   },  // ?
	{ '\x5b', 0, "lsq"    },  // [
	{ '\x5c', 0, "bksl"   },  // \ +-@v7.6.4 @Muxa
	{ '\x5d', 0, "rsq"    },  // ]
	// { '\x59', 0, "#59"    },  // ; @v10.8.10
};

void FASTCALL XMLReplaceSpecSymb(SString & rBuf, const char * pProcessSymb)
{
	// @v10.9.7 SString temp_buf = rBuf;
	SString & r_temp_buf = SLS.AcquireRvlStr(); // @v10.9.7
	r_temp_buf = rBuf; // @v10.9.7
	const char * p_include = 0;
	const char * p_exclude = 0;
	if(pProcessSymb) {
		if(pProcessSymb[0] == '-')
			p_exclude = pProcessSymb+1;
		else
			p_include = pProcessSymb;
	}
	for(size_t i = 0; i < SIZEOFARRAY(SpcSymbTab); i++) {
		const char rc = SpcSymbTab[i].chr;
		if((!p_exclude || sstrchr(p_exclude, rc) == 0) && (!p_include || sstrchr(p_include, rc) != 0)) {
			char   pattern[32];
			char   replacer[32];
			pattern[0] = rc;
			pattern[1] = 0;
			{
				char * c = replacer;
				*c++ = '&';
				c = stpcpy(c, SpcSymbTab[i].str);
				*c++ = ';';
				*c = 0;
			}
			r_temp_buf.ReplaceStr(pattern, replacer, 0);
		}
	}
	rBuf = r_temp_buf;
}

int XMLWriteSpecSymbEntities(FILE * pStream)
{
	int    ok = 1;
	if(pStream) {
		SString temp_buf;
		for(size_t i = 0; i < SIZEOFARRAY(SpcSymbTab); i++) {
			temp_buf.Z();
			if(SpcSymbTab[i].Amp)
				temp_buf.CatChar('#').Cat(0x26).Semicol(); 
			temp_buf.CatChar('#').Cat(SpcSymbTab[i].chr).Semicol();
			fprintf(pStream, "<!ENTITY %s \"&%s\">\n", SpcSymbTab[i].str, temp_buf.cptr());
		}
	}
	else
		ok = 0;
	return ok;
}

int XMLWriteSpecSymbEntities(void * pWriter)
{
	int    ok = 1;
	SString subst;
	if(pWriter) {
		for(size_t i = 0; i < SIZEOFARRAY(SpcSymbTab); i++) {
			subst.Z().CatChar('&');
			if(SpcSymbTab[i].Amp)
				subst.CatChar('#').Cat(0x26).Semicol(); 
			subst.CatChar('#').Cat(SpcSymbTab[i].chr).Semicol();
			xmlTextWriterWriteDTDEntity(static_cast<xmlTextWriter *>(pWriter), 0, reinterpret_cast<const xmlChar *>(SpcSymbTab[i].str), 0, 0, 0, subst.ucptr());
		}
	}
	else
		ok = 0;
	return ok;
}
//
//
//
/*static*/const SString & FASTCALL SXml::nst(const char * pNs, const char * pT)
{
	assert(/*!isempty(pNs) &&*/ !isempty(pT));
	SString & r_buf = SLS.AcquireRvlStr();
	return isempty(pNs) ? r_buf.Cat(pT) : r_buf.Cat(pNs).Colon().Cat(pT);
}

/*static*/const SString & FASTCALL SXml::nst_xmlns(const char * pT) // SXml::nst("xmlns", pT)
{
	assert(/*!isempty(pNs) &&*/ !isempty(pT));
	SString & r_buf = SLS.AcquireRvlStr();
	return r_buf.Cat("xmlns").Colon().Cat(pT);
}

SXml::WDoc::WDoc(xmlTextWriter * pWriter, SCodepage cp) : State(0), Lx(pWriter)
{
	if(Lx) {
		SString temp_buf;
		if(cp < 0)
			cp = cpUTF8;
		SCodepageIdent cpi(cp);
		SCodepageIdent temp_cp = cp;
		temp_cp.ToStr(SCodepageIdent::fmtXML, temp_buf.Z());
		xmlTextWriterStartDocument(Lx, 0, temp_buf, 0);
		State |= stStarted;
	}
}

SXml::WDoc::WDoc(xmlTextWriter * pWriter) : State(0), Lx(pWriter)
{
	if(Lx) {
		State |= (stStarted|stSkipHeader);
	}
}

SXml::WDoc::~WDoc()
{
	if(State & stStarted) {
		if(Lx)
			xmlTextWriterEndDocument(Lx);
		Lx = 0;
		State &= ~stStarted;
	}
}

/*static*/SString & FASTCALL SXml::WNode::CDATA(SString & rBuf)
{
	SString temp_buf;
	temp_buf.Cat("<![CDATA[").Cat(rBuf).Cat("]]>");
	rBuf = temp_buf;
	return rBuf;
}

SXml::WNode::WNode(xmlTextWriter * pWriter, const char * pName)
{
	Construct(pWriter, pName);
}

SXml::WNode::WNode(xmlTextWriter * pWriter, const char * pName, const SString & rValue)
{
	if(Construct(pWriter, pName))
		xmlTextWriterWriteString(Lx, rValue.ucptr());
}

SXml::WNode::WNode(xmlTextWriter * pWriter, const char * pName, const char * pValue)
{
	if(Construct(pWriter, pName))
		xmlTextWriterWriteString(Lx, reinterpret_cast<const xmlChar *>(pValue));
}

SXml::WNode::~WNode()
{
	if(Lx && State & stStarted)
		xmlTextWriterEndElement(Lx);
}

int SXml::WNode::PutAttribSkipEmpty(const char * pName, const char * pValue)
{
	int    ok = 1;
	if(State & stStarted && Lx && !isempty(pValue)) {
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.12
		r_temp_buf = pValue;
		if(r_temp_buf.NotEmptyS()) {
			xmlTextWriterStartAttribute(Lx, reinterpret_cast<const xmlChar *>(pName));
			xmlTextWriterWriteString(Lx, reinterpret_cast<const xmlChar *>(pValue));
			xmlTextWriterEndAttribute(Lx);
		}
	}
	else
		ok = 0;
	return ok;
}

int SXml::WNode::PutInnerValidDate(const char * pInnerName, LDATE dt, long fmt)
{
	int    ok = -1;
	if(checkdate(dt)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		ok = PutInner(pInnerName, r_temp_buf.Cat(dt, fmt));
	}
	return ok;
}

int SXml::WNode::PutInner(const char * pInnerName, const char * pInnerValue)
{
	int    ok = 1;
	if(State & stStarted && Lx) {
		WNode inner(Lx, pInnerName, isempty(pInnerValue) ? "" : pInnerValue); // @v11.5.11 0-->""
	}
	else
		ok = 0;
	return ok;
}

int SXml::WNode::PutInnerSkipEmpty(const char * pInnerName, const char * pInnerValue)
{
	int    ok = 1;
	if(State & stStarted && Lx && !isempty(pInnerValue)) {
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.12
		r_temp_buf = pInnerValue;
		if(r_temp_buf.NotEmptyS()) {
			WNode inner(Lx, pInnerName, pInnerValue);
		}
	}
	else
		ok = 0;
	return ok;
}

int SXml::WNode::SetValue(const SString & rText)
{
	int    ok = 0;
	if(State & stStarted && Lx) {
		xmlTextWriterWriteString(Lx, rText.ucptr());
		ok = 1;
	}
	return ok;
}

int SXml::WNode::Construct(xmlTextWriter * pWriter, const char * pName)
{
	int   ok = 0;
	State = 0;
	Lx = pWriter;
	Name = pName;
	if(Lx && Name.NotEmpty()) {
		xmlTextWriterStartElement(Lx, Name.ucptr());
		State |= stStarted;
		ok = 1;
	}
	return ok;
}

/*static*/bool FASTCALL SXml::IsName(const xmlNode * pNode, const char * pName)
	{ return (pNode && sstreqi_ascii(reinterpret_cast<const char *>(pNode->name), pName)); }
/*static*/bool FASTCALL SXml::IsContent(const xmlNode * pNode, const char * pText)
	{ return (pNode && sstreqi_ascii(reinterpret_cast<const char *>(pNode->content), pText)); }

/*static*/bool FASTCALL SXml::GetContent(const xmlNode * pNode, SString & rResult)
{
	bool   ok = false;
	rResult.Z();
	if(pNode) {
		if(pNode->content) {
			rResult.Set(pNode->content);
			ok = true;
		}
		else if(pNode->type == XML_ELEMENT_NODE) {
			const xmlNode * p_children = pNode->children;
			if(p_children && p_children->type == XML_TEXT_NODE && p_children->content) {
				rResult.Set(p_children->content);
				ok = true;
			}
		}
	}
	return ok;
}

/*static*/int SXml::GetContentByName(const xmlNode * pNode, const char * pName, SString & rResult)
{
	int    ok = 0;
	if(IsName(pNode, pName)) {
		GetContent(pNode, rResult);
		ok = rResult.NotEmpty() ? 1 : -1;
	}
	return ok;
}

/*static*/bool SXml::GetAttrib(const xmlNode * pNode, const char * pAttr, SString & rResult)
{
	bool   ok = false;
	rResult.Z();
    if(pNode) {
		for(const xmlAttr * p_attr = pNode->properties; p_attr; p_attr = p_attr->next) {
			if(sstreqi_ascii(reinterpret_cast<const char *>(p_attr->name), pAttr)) {
				const xmlNode * p_children = p_attr->children;
				if(p_children && p_children->type == XML_TEXT_NODE)
					rResult.Set(p_children->content);
				ok = true;
			}
		}
    }
    return ok;
}

SXmlWriter::SXmlWriter()
{
	P_XmlBuf = xmlBufferCreate();
	P_Writer = xmlNewTextWriterMemory(P_XmlBuf, 0);
	xmlTextWriterSetIndent(P_Writer, 1);
}

SXmlWriter::~SXmlWriter()
{
	xmlFreeTextWriter(P_Writer);
	xmlBufferFree(P_XmlBuf);
}

/*static*/void __cdecl SXmlValidationMessageList::SchemaValidityError(void * pCtx, const char * pMsg, ...)
{
	SXmlValidationMessageList * p_this = static_cast<SXmlValidationMessageList *>(pCtx);
	if(p_this) {
		SString text;
		va_list argptr;
		va_start(argptr, pMsg);
		text.VPrintf(pMsg, argptr);
		p_this->AddMessage(1, text);
	}
}

/*static*/void __cdecl SXmlValidationMessageList::SchemaValidityWarning(void * pCtx, const char * pMsg, ...)
{
	SXmlValidationMessageList * p_this = static_cast<SXmlValidationMessageList *>(pCtx);
	if(p_this) {
		SString text;
		va_list argptr;
		va_start(argptr, pMsg);
		text.VPrintf(pMsg, argptr);
		p_this->AddMessage(2, text);
	}
}

SXmlValidationMessageList::SXmlValidationMessageList()
{
}

int SXmlValidationMessageList::AddMessage(int type, const char * pMsg)
{
	int    ok = 1;
	if(pMsg) {
		EntryInner entry;
		entry.Type = type;
		AddS(pMsg, &entry.MsgP);
		L.insert(&entry);
	}
	return ok;
}

uint SXmlValidationMessageList::GetMessageCount() const
{
	return L.getCount();
}

int SXmlValidationMessageList::GetMessageByIdx(uint idx, int * pType, SString & rMsg) const
{
	rMsg.Z();
	int    ok = 1;
	if(idx < L.getCount()) {
		const EntryInner & r_entry = L.at(idx);
		ASSIGN_PTR(pType, r_entry.Type);
		GetS(r_entry.MsgP, rMsg);
	}
	else
		ok = 0;
	return ok;
}

/*static*/int SXml::Validate(const char * pXsdFileName, const char * pXmlFileName, SXmlValidationMessageList * pMsgList)
{
	int    ok = 1;
	xmlDoc * doc = 0;
	xmlSchema * schema = 0;
	//char * XMLFileName = "test.xml";
	//char * XSDFileName = "test.xsd";
	xmlLineNumbersDefault(1);
	xmlSchemaParserCtxt * p_sp_ctxt = xmlSchemaNewParserCtxt(pXsdFileName);
	if(pMsgList) {
		xmlSchemaSetParserErrors(p_sp_ctxt, SXmlValidationMessageList::SchemaValidityError, SXmlValidationMessageList::SchemaValidityWarning, pMsgList);
	}
	schema = xmlSchemaParse(p_sp_ctxt);
	xmlSchemaFreeParserCtxt(p_sp_ctxt);
	//xmlSchemaDump(stdout, schema); //To print schema dump
	doc = xmlReadFile(pXmlFileName, NULL, 0);
	if(doc == NULL) {
		SXmlValidationMessageList::SchemaValidityError(pMsgList, "Could not parse %s\n", pXmlFileName);
		ok = 0;
	}
	else {
		xmlSchemaValidCtxt * p_sv_ctxt = 0;
		int ret;
		p_sv_ctxt = xmlSchemaNewValidCtxt(schema);
		//xmlSchemaSetValidErrors(p_sv_ctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);
		if(pMsgList) {
			//xmlSchemaSetParserErrors(p_sp_ctxt, SXmlValidationMessageList::SchemaValidityError, SXmlValidationMessageList::SchemaValidityWarning, pMsgList);
			xmlSchemaSetValidErrors(p_sv_ctxt, SXmlValidationMessageList::SchemaValidityError, SXmlValidationMessageList::SchemaValidityWarning, pMsgList);
		}
		else 
			xmlSchemaSetValidErrors(p_sv_ctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);
		ret = xmlSchemaValidateDoc(p_sv_ctxt, doc);
		if(!ret) {
			SXmlValidationMessageList::SchemaValidityWarning(pMsgList, "%s validates\n", pXmlFileName);
			ok = 1;
		}
		else if(ret > 0) {
			SXmlValidationMessageList::SchemaValidityError(pMsgList, "%s fails to validate\n", pXmlFileName);
			ok = 0;
		}
		else {
			SXmlValidationMessageList::SchemaValidityError(pMsgList, "%s validation generated an internal error\n", pXmlFileName);
			ok = 0;
		}
		xmlSchemaFreeValidCtxt(p_sv_ctxt);
		xmlFreeDoc(doc);
	}
	// free the resource
	xmlSchemaFree(schema);
	xmlSchemaCleanupTypes();
	xmlCleanupParser();
	//xmlMemoryDump();
	return ok;
}
//
//
//
SXmlSaxParser::SXmlSaxParser(long flags) : State(0), Flags(flags), P_SaxCtx(0)
{
}

SXmlSaxParser::~SXmlSaxParser()
{
}

extern "C" xmlParserCtxt * xmlCreateURLParserCtxt(const char * filename, int options);
void FASTCALL xmlDetectSAX2(xmlParserCtxt * ctxt); // @prototype

int SXmlSaxParser::ParseFile(const char * pFileName)
{
	int    ok = 1;
	THROW(fileExists(pFileName));
	{
		xmlSAXHandler saxh;
		// @v10.7.9 @ctr MEMSZERO(saxh);
		if(Flags & fStartDocument)
			saxh.startDocument = Scb_StartDocument;
		if(Flags & fEndDocument)
			saxh.endDocument = Scb_EndDocument;
		if(Flags & fStartElement)
			saxh.startElement = Scb_StartElement;
		if(Flags & fEndElement)
			saxh.endElement = Scb_EndElement;
		if(Flags & fCharacters)
			saxh.characters = Scb_Characters;

		xmlFreeParserCtxt(P_SaxCtx);
		THROW(P_SaxCtx = xmlCreateURLParserCtxt(pFileName, 0));
		if(P_SaxCtx->sax != reinterpret_cast<xmlSAXHandler *>(&xmlDefaultSAXHandler))
			SAlloc::F(P_SaxCtx->sax);
		P_SaxCtx->sax = &saxh;
		xmlDetectSAX2(P_SaxCtx);
		P_SaxCtx->userData = this;
		SrcFileName = pFileName;
		xmlParseDocument(P_SaxCtx);
	}
	CATCHZOK
	if(P_SaxCtx) {
		P_SaxCtx->sax = 0;
		xmlFreeParserCtxt(P_SaxCtx);
	}
	P_SaxCtx = 0;
	return ok;
}

void SXmlSaxParser::SaxStop()
{
	xmlStopParser(P_SaxCtx);
}

/*virtual*/int SXmlSaxParser::StartDocument()
{
	TagValue.Z();
	return 1;
}

/*virtual*/int SXmlSaxParser::EndDocument()
{
	return 1;
}
 
/*virtual*/int SXmlSaxParser::StartElement(const char * pName, const char ** ppAttrList)
{
	TagValue.Z();
	return 1;
}
 
/*virtual*/int SXmlSaxParser::EndElement(const char * pName)
{
	return 1;
}
 
/*virtual*/int SXmlSaxParser::Characters(const char * pS, size_t len)
{
	//
	// Одна строка может быть передана несколькими вызовами. По этому StartElement обнуляет
	// буфер RdB.TagValue, а здесь каждый вызов дополняет существующую строку входящими символами
	//
	TagValue.CatN(pS, len);
	return 1;
}

void SXmlSaxParser::Scb_StartDocument(void * ptr) { CALLTYPEPTRMEMB(SXmlSaxParser, ptr, StartDocument()); }
void SXmlSaxParser::Scb_EndDocument(void * ptr) { CALLTYPEPTRMEMB(SXmlSaxParser, ptr, EndDocument()); }
void SXmlSaxParser::Scb_StartElement(void * ptr, const xmlChar * pName, const xmlChar ** ppAttrList) 
	{ CALLTYPEPTRMEMB(SXmlSaxParser, ptr, StartElement(reinterpret_cast<const char *>(pName), reinterpret_cast<const char **>(ppAttrList))); }
void SXmlSaxParser::Scb_EndElement(void * ptr, const xmlChar * pName) 
	{ CALLTYPEPTRMEMB(SXmlSaxParser, ptr, EndElement(reinterpret_cast<const char *>(pName))); }
void SXmlSaxParser::Scb_Characters(void * ptr, const uchar * pC, int len) 
	{ CALLTYPEPTRMEMB(SXmlSaxParser, ptr, Characters(reinterpret_cast<const char *>(pC), len)); }
//
//
//
#if 0 // @construction {

class XmlList;

struct XmlEntry {
	enum XET {
		tPcdata = 1,
		tList
	};
	XmlEntry(const char * pName = 0, XET type = XmlEntry::tPcdata, XmlList * pList = 0);
	~XmlEntry();

	XET    Type;
	char * P_Name;
	XmlList * P_List;
};

class XmlList : public SArray {
public:
	XmlList() : SArray(sizeof(XmlEntry), 8, aryDataOwner|aryEachItem)
	{
	}
	int    AddItem(const char *, XmlEntry::XET, XmlList * = 0);
	const  XmlEntry * GetItem(uint) const;
	const  XmlEntry * Search(const char * pName, uint *);
protected:
	virtual void FASTCALL freeItem(void * pItem)
	{
		delete static_cast<XmlEntry *>(pItem);
	}
};

XmlEntry::XmlEntry(const char * pName, XET type, XmlList * pList)
{
	Type = type;
	P_Name = newStr(pName);
	P_List = pList;
}

XmlEntry::~XmlEntry()
{
	delete P_Name;
	delete P_List;
}
//
//
//
int XmlList::AddItem(const char * pName, XmlEntry::XET type, XmlList * pList)
{
	XmlEntry entry(pName, type, pList);
	insert(&entry);
	entry.P_Name = 0;
	entry.P_List = 0;
	return 1;
}

const XmlEntry * XmlList::GetItem(uint p) const
{
	return (p < getCount()) ? static_cast<const XmlEntry *>(at(p)) : 0;
}

class XmlWriter {
public:
	XmlWriter(const char *);
	~XmlWriter();
	int    Open(const char *);
	int    Close();
	int    PutLine(const char *, int newLine = 0);
	int    PutHeader(int encoding = 0);
	int    PutDtdDocType(const char *);
	int    PutDtdEntity(const char * pName, const char * pVal);
	int    PutDtdElementHdr(const char *);
	int    PutDtdElementFtr(const char *);
	int    PutDtdElement(const char *, int dtdElemType);
	int    PutTag(const char *, int closed);
	int    PutString(const char * pTag, const char * pVal);
	int    PutInt(const char * pTag, long);
	int    PutReal(const char * pTag, double);
	int    PutDate(const char * pTag, LDATE);
	int    PutDtdList(const XmlList *);
private:
	FILE * Stream;
};

enum XmlStrId {
	xmlsHeader = 0
};

const char * XmlStrings[] = { "<?xml version=\"1.0\" encoding=\"Windows-1251\"?>", };

XmlWriter::XmlWriter(const char * pFileName)
{
	Stream = 0;
	if(pFileName)
		Open(pFileName);
}

XmlWriter::~XmlWriter()
{
	Close();
}

int XmlWriter::Open(const char * pFileName)
{
	int    ok = -1;
	Close();
	if(pFileName) {
		Stream = fopen(pFileName, "wt");
		if(Stream == 0)
			ok = (SLibError = SLERR_OPENFAULT, 0);
		else
			ok = 1;
	}
	return ok;
}

int XmlWriter::Close()
{
	SFile::ZClose(&Stream);
	return 1;
}

int XmlWriter::PutLine(const char * pStr, int newLine)
{
	int    ok = 1;
	if(!Stream)
		return 0;
	if(pStr)
		ok = (fputs(pStr, Stream) >= 0) ? 1 : (SLibError = SLERR_WRITEFAULT, 0);
	if(ok && newLine)
		ok = (fputc('\n', Stream) >= 0) ? 1 : (SLibError = SLERR_WRITEFAULT, 0);
	return ok;
}

int XmlWriter::PutHeader(int /*encoding*/)
{
	return PutLine(XmlStrings[xmlsHeader], 1);
}

int XmlWriter::PutDtdDocType(const char * pName)
{
	char buf[1024];
	sprintf(buf, "<!DOCTYPE %s [", pName);
	return PutLine(buf, 1);
}

int XmlWriter::PutDtdEntity(const char * pName, const char * pVal)
{
	return (PutLine("<!ENTITY ", 0) && PutLine(pName) && PutLine(" ") && PutLine(pVal) && PutLine(">", 1));
}

int XmlWriter::PutDtdElementHdr(const char * pName)
{
	return (PutLine("<!ELEMENT ") && PutLine(pName));
}

int XmlWriter::PutDtdList(const XmlList * pList)
{
	int    ok = 1;
	for(uint i = 0; i < pList->getCount(); i++) {
		const XmlEntry * p = pList->GetItem(i);
		if(p->Type == XmlEntry::tPcdata) {
			PutDtdElementHdr(p->P_Name);
			PutLine(" ");
			PutLine("(#PCDATA)>", 1);
		}
		else if(p->Type == XmlEntry::tList && p->P_List) {
			PutDtdElementHdr(p->P_Name);
			PutLine(" (", 1);
			for(uint j = 0; j < p->P_List->getCount(); j++) {
				const XmlEntry * p_li = p->P_List->GetItem(j);
				PutLine("\t");
				PutLine(p_li->P_Name);
				if(j < (p->P_List->getCount()-1))
					PutLine(",", 1);
			}
			PutLine("\n)>", 1);
			PutDtdList(p->P_List);
		}
	}
	return ok;
}
//
//
//
class SXml__ {
public:
	struct Status {
		uint8  VerMajor;
		uint8  VerMinor;
		uint32 Flags;
		SString Encoding;
	};
	struct DtdChildren {
		DtdChildren()
		{
			THISZERO();
		}
		~DtdChildren()
		{
			if(oneof2(Kind, 1, 2))
				delete P_Inner;
			delete P_Next;
		}

		int16  Kind; // 0 - name, 1 - choice, 2 - seq
		int16  Mult; // 0x01 - single, '?', '*', '+'
		union {
			uint32 SymbID;
			SXml::DtdChildren * P_Inner;
		};
		SXml::DtdChildren * P_Next;
	};
	struct DtdElement {
		DtdElement()
		{
			THISZERO();
		}
		~DtdElement()
		{
			delete P_Children;
		}
		uint32 SymbID;
		int32  Kind;    // 0 - EMPTY, 1 - ANY, 2 - mixed, 3 - children
		SXml::DtdChildren * P_Children;
	};
	struct DtdAttList {

	};
	struct Dtd {
		uint32  NameID;
		StringSet EntitySymbList;
		StringSet EntityValList;
		TSCollection <DtdElement> ElementList;
	};

	class TagArray;

	struct Tag {
		Tag() : SymbID(0), Kind(0), ValID(0), P_Parent(0)
		{
		}
		~Tag()
		{
			destroy();
		}
		Tag & operator = (const Tag & rS)
		{
			destroy();
			SymbID = rS.SymbID;
			Kind = rS.Kind;
			AttrList = rS.AttrList;
			if(Kind == 2) {
				if(rS.P_Inner) {
					P_Inner = new TagArray;
					*P_Inner = *rS.P_Inner;
				}
			}
			else
				ValID = rS.ValID;
			P_Parent = rS.P_Parent;
			return *this;
		}
		void destroy()
		{
			if(Kind == 2)
				ZDELETE(P_Inner);
			AttrList.freeAll();
		}
		friend class SXml;
	private:
		uint32 SymbID;
		int32  Kind; // 0 - undef, 1 - value, 2 - inner tag list
		LAssocArray AttrList;
		union {
			uint32 ValID; // 0 - empty value
			TagArray * P_Inner;
		};
		SXml::Tag * P_Parent; // @notowned
	};
	class TagArray : public TSArray <SXml::Tag> {
	public:
		TagArray() : TSArray <SXml::Tag> (aryDataOwner|aryEachItem)
		{
		}
		~TagArray()
		{
			freeAll();
		}
	private:
		virtual void FASTCALL freeItem(void * pItem)
		{
			((SXml::Tag *)pItem)->destroy();
		}
	};
	struct Doc {
		SXml::Status S;
		SXml::Dtd D;
		SXml::Tag Root;
	};

	SXml();
	~SXml();

	int    GetSymbID(const char * pSymb, uint32 * pID);
	int    GetSymb(uint32 id, SString & rSymb);
	int    SearchSymb(const char * pSymb, uint32 * pID);

	int    GetValID(const char * pVal, uint32 * pID);
	int    GetVal(uint32 id, SString & rVal);
	int    SearchVal(const char * pVal, uint32 * pID);

	DtdElement  * CreateDtdElement(const char * pName);
	DtdChildren * CreateDtdChildren(const char * pName, int mult);
	DtdChildren * CreateDtdChildren(int choiceOrSeq, DtdChildren * pInner, int mult);
	int    AddDtdChild(DtdChildren * pC, const char * pName);
	int    AddDtdChild(DtdChildren * pC, DtdChildren * pNext);

	int    CreateTag(Tag * pTag, const char * pSymb);
	int    AddAttr(Tag * pTag, const char * pAttr, const char * pVal);
	int    SetValue(Tag * pTag, const char * pVal);
	int    AddTag(Tag * pOuterTag, Tag * pInnerTag);

	int    GetAttr(const Tag * pTag, const char * pAttr, SString & rVal) const;
	int    GetValue(const Tag * pTag, SString & rVal) const;
	const TagArray * GetInnerList_Const(const Tag * pTag) const;
	TagArray * GetInnerList(Tag * pTag);

	int    Parse(const char * pSrc, Doc * pDoc);
private:
	int    SetPredefinedTokens();
	SymbHashTable SymbHash;
	StrAssocArray ValList;
	uint32 IdCounter;
};

SXml::SXml() : SymbHash(8192, 1)
{
	IdCounter = 0;
}

SXml::~SXml()
{
}

int SXml::GetSymbID(const char * pSymb, uint32 * pID)
{
	int    ok = 1;
	uint   val = 0;
	if(!SymbHash.Search(pSymb, &val)) {
		val = ++IdCounter;
		if(SymbHash.Add(pSymb, val, 0))
			ok = 2;
		else
			ok = 0;
	}
	ASSIGN_PTR(pID, val);
	return ok;
}

int SXml::GetSymb(uint32 id, SString & rSymb)
{
	return SymbHash.GetByAssoc(id, rSymb);
}

int SXml::SearchSymb(const char * pSymb, uint32 * pID)
{
	int    ok = 0;
	uint   val = 0;
	if(SymbHash.Search(pSymb, &val))
		ok = 1;
	ASSIGN_PTR(pID, val);
	return ok;
}

int SXml::GetValID(const char * pVal, uint32 * pID)
{
	int    ok = 0;
	uint   pos = 0;
	uint32 id = 0;
	if(ValList.SearchByText(pVal, 0, &pos)) {
		id = ValList.at(pos).Id;
		ok = 1;
	}
	else {
		id = ++IdCounter;
		ok = ValList.Add(id, pVal, 1) ? 2 : 0;
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SXml::GetVal(uint32 id, SString & rVal)
{
	int    ok = 0;
	uint   pos = 0;
	if(ValList.Search(id, &pos)) {
		rVal = ValList.at(pos).Txt;
		ok = 1;
	}
	else
		rVal.Z();
	return ok;
}

int SXml::SearchVal(const char * pVal, uint32 * pID)
{
	int    ok = 0;
	uint   pos = 0;
	uint32 id = 0;
	if(ValList.SearchByText(pVal, 0, &pos)) {
		id = ValList.at(pos).Id;
		ok = 1;
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

#endif // } 0 @construction
//
//
//
#if 0 // @construction { (Затянувшаяся разработка - очень вероятно, что на всегда)
//
// SOAP
//
#define SOAPIF_RESOLVED 0x0001
#define SOAPIF_STRUC    0x0002
#define SOAPIF_ARRAY    0x0004

struct SoapPacketItem {
	SoapPacketItem();
	SoapPacketItem & Clear();

	SString Name;
	SString Ref;
	SString TypeName;
	TYPEID  Typ;
	long    Flags;     // SOAPIF_XXX
	SString Value;
	union {
		SoapPacketStruc * P_Struc;
		SoapPacketArray * P_Array;
	};
};

class SoapPacketArray : private TSCollection <SoapPacketStruc> {
public:
	SoapPacketArray(SoapPacket & rR);
	uint   GetCount() const;
	SoapPacketStruc * FASTCALL Get(uint pos);
	int    CreateItem(uint * pPos);
	int    CreateItemRef(const char * pRef, const char * pType, uint * pPos);
	SoapPacketStruc * SearchItemRef(const char * pRef, const char * pType);
private:
	SoapPacket & R_R;
};

class SoapPacketStruc : private SArray {
public:
	friend class SoapPacket;
	friend class SoapPacketArray;

	SoapPacketStruc(SoapPacket & rR);
	SoapPacketStruc & Clear();
	int    Add(const char * pFldName, const char * pType, const char * pValue);
	SoapPacketArray * AddArray(const char * pFldName, const char * pType);
	SoapPacketStruc * AddStruc(const char * pFldName, const char * pType);
	int    AddRef(const char * pFldName, const char * pRef);
	int    AddResolvedRef(const char * pRef, const char * pType, const char * pValue);
	SoapPacketStruc * SearchItemRef(const char * pRef, const char * pType);
	uint   GetCount() const;
	int    Get(const char * pName, SoapPacketItem & rItem) const;
	int    Get(uint pos, SoapPacketItem & rItem) const;
private:
	struct InnerItem {
		uint   NameP;
		uint   TypeP;
		uint   ValueP; // Индекс позиции значения. Если Flags & SOAPIF_STRUC, то это - индекс в
			// ArrList[0], если Flags & SOAPIF_ARRAY, то - индекс массива в ArrList.
		uint   RefP;
		uint   Dim;
		TYPEID Typ;
		long   Flags;   // SOAPIF_XXX
	};

	TSCollection <SoapPacketArray> ArrList; // Список внутренних массивов. Элемент с индексом 0 представляет
		// внутренние одиночные структуры.

	int    Helper_Get(const InnerItem * pItem, SoapPacketItem & rItem) const;
	int    ResolveRefs(const SoapPacketStruc & rResolveList);

	uint   RefP;
	uint   TypeP;  // Позиция наименования типа записи SOAP (attr arrayType)
	SoapPacket & R_R;
};

class SoapPacket {
public:
	enum {
		stFault = 0x0001
	};
	SoapPacket();
	SoapPacket & Clear();
	//
	// Descr: Разбирает результат вызова Soap-запроса.
	// Returns:
	//   1 - запрос успешно разобран
	//   0 - error разбора
	//
	int    Parse(const char * pMethodName, void * pXmlDoc, const char * pDebugFileName = 0); // really (xmlDoc *)
	int    Write(const char * pUrn, const char * pMethodName, void ** ppXmlDoc, const char * pDebugFileName); // really (xmlDoc **)
	int    HasFault() const
	{
		return BIN(State & stFault);
	}
	const  SString & GetFaultText() const
	{
		return FaultText;
	}
	SoapPacketStruc & GetBody()
	{
		return B;
	}
	int    SetString(const char * pStr, uint * pP);
	int    GetStringPos(const char * pStr, uint * pP) const;
	int    GetString(uint pos, SString & rBuf) const;
private:
	int    ParseType(SString & rTypeBuf, uint * pArraySize) const;

	long   State;
	SString FaultCode;
	SString FaultText;
	SoapPacketStruc B;
	SymbHashTable T;
	uint   SymbCounter;
};

SoapPacketItem::SoapPacketItem()
{
	Typ = 0;
	Flags = 0;
	P_Struc = 0;
}

SoapPacketItem & SoapPacketItem::Clear()
{
	Name = 0;
	Ref = 0;
	TypeName = 0;
	Typ = 0;
	Flags = 0;
	Value = 0;
	P_Struc = 0;
	return *this;
}
//
//
//
SoapPacketArray::SoapPacketArray(SoapPacket & rR) : R_R(rR)
{
}

uint SoapPacketArray::GetCount() const
{
	return getCount();
}

SoapPacketStruc * FASTCALL SoapPacketArray::Get(uint pos)
{
	return (pos < getCount()) ? at(pos) : 0;
}

int SoapPacketArray::CreateItem(uint * pPos)
{
	int    ok = 1;
	SoapPacketStruc * p_new_item = new SoapPacketStruc(R_R);
	THROW_S(p_new_item, SLERR_NOMEM);
	THROW(insert(p_new_item));
	ASSIGN_PTR(pPos, getCount()-1);
	CATCHZOK
	return ok;
}

int SoapPacketArray::CreateItemRef(const char * pRef, const char * pType, uint * pPos)
{
	int    ok = 1;
	SoapPacketStruc * p_new_item = new SoapPacketStruc(R_R);
	THROW_S(p_new_item, SLERR_NOMEM);
	THROW(R_R.SetString(pRef, &p_new_item->RefP));
	THROW(R_R.SetString(pType, &p_new_item->TypeP));
	THROW(insert(p_new_item));
	ASSIGN_PTR(pPos, getCount()-1);
	CATCHZOK
	return ok;
}

SoapPacketStruc * SoapPacketArray::SearchItemRef(const char * pRef, const char * pType)
{
	SoapPacketStruc * p_ret = 0;
	uint   ref_val = 0, ref_pos = 0;
	uint   type_val = 0, type_pos = 0;
	THROW_S_S(R_R.GetStringPos(pRef, &ref_pos) > 0, SLERR_SOAPR_UNDEFREF, pRef);
	if(!isempty(pType)) {
		THROW_S_S(R_R.GetStringPos(pType, &type_pos) > 0, SLERR_SOAPR_UNDEFTYPE, pType);
	}
	for(uint i = 0; !p_ret && i < getCount(); i++) {
		SoapPacketStruc * p_struc = at(i);
		if(p_struc && p_struc->RefP == ref_pos && (!type_pos || p_struc->TypeP == type_pos))
			p_ret = p_struc;
	}
	if(!p_ret) {
		SString msg_buf;
		msg_buf = pRef;
		if(!isempty(pType))
			msg_buf.CatDiv(':', 0).Cat(pType);
		CALLEXCEPT_S_S(SLERR_SOAPR_ITEMREFNFOUND, msg_buf);
	}
	CATCH
		p_ret = 0;
	ENDCATCH
	return p_ret;
}
//
//
//
SoapPacketStruc::SoapPacketStruc(SoapPacket & rR) : SArray(sizeof(SoapPacketStruc::InnerItem)), R_R(rR)
{
	RefP = 0;
	TypeP = 0;
	SoapPacketArray * p_struc_list = new SoapPacketArray(R_R);
	if(p_struc_list)
		ArrList.insert(p_struc_list);
}

SoapPacketStruc & SoapPacketStruc::Clear()
{
	ArrList.freeAll();
	freeAll();
	RefP = 0;
	TypeP = 0;
	SoapPacketArray * p_struc_list = new SoapPacketArray(R_R);
	if(p_struc_list)
		ArrList.insert(p_struc_list);
	return *this;
}

uint SoapPacketStruc::GetCount() const
{
	return getCount();
}

SoapPacketStruc * SoapPacketStruc::SearchItemRef(const char * pRef, const char * pType)
{
	SoapPacketStruc * p_ret = 0;
	SoapPacketItem item;
	for(uint i = 0; !p_ret && i < getCount(); i++) {
		if(Get(i, item)) {
			if(item.Flags & SOAPIF_ARRAY) {
				if(item.P_Array)
					p_ret = item.P_Array->SearchItemRef(pRef, pType);
			}
			else if(item.Flags & SOAPIF_STRUC) {
				if(item.P_Struc)
					p_ret = item.P_Struc->SearchItemRef(pRef, pType); // @recursion
			}
		}
	}
	return p_ret;
}

int SoapPacketStruc::Helper_Get(const SoapPacketStruc::InnerItem * pItem, SoapPacketItem & rItem) const
{
	rItem.Value = 0;
	R_R.GetString(pItem->NameP, rItem.Name);
	R_R.GetString(pItem->RefP, rItem.Ref);
	R_R.GetString(pItem->TypeP, rItem.TypeName);
	rItem.Typ = pItem->Typ;
	rItem.Flags = pItem->Flags;
	assert((rItem.Flags & (SOAPIF_STRUC|SOAPIF_ARRAY)) != (SOAPIF_STRUC|SOAPIF_ARRAY));
	if(rItem.Flags & SOAPIF_STRUC) {
		assert(pItem->ValueP < ArrList.getCount());
		rItem.P_Struc = ArrList.at(0)->Get(pItem->ValueP);
	}
	else if(rItem.Flags & SOAPIF_ARRAY) {
		assert(pItem->ValueP > 0 && pItem->ValueP < ArrList.getCount());
		rItem.P_Array = ArrList.at(pItem->ValueP);
	}
	else {
		R_R.GetString(pItem->ValueP, rItem.Value);
	}
	return 1;
}

int SoapPacketStruc::Get(uint pos, SoapPacketItem & rItem) const
{
	int    ok = 1;
	rItem.Clear();
	if(pos < getCount())
		Helper_Get(static_cast<InnerItem *>(at(pos)), rItem);
	else {
		SString msg_buf;
		msg_buf.Cat(pos);
		SLS.SetError(SLERR_SOAPR_INVITEMPOS, msg_buf);
	}
	return ok;
}

int SoapPacketStruc::Get(const char * pName, SoapPacketItem & rItem) const
{
	int    ok = 0;
	uint   pos = 0;
	rItem.Clear();
	if(R_R.GetStringPos(pName, &pos) > 0) {
		for(uint i = 0; !ok && i < getCount(); i++) {
			const InnerItem * p_item = static_cast<const InnerItem *>(at(i));
			if(p_item->NameP == pos) {
				Helper_Get(p_item, rItem);
				ok = 1;
			}
		}
	}
	if(!ok) {
		SLS.SetError(SLERR_SOAPR_ITEMNAMENFOUND, pName);
	}
	return ok;
}

int SoapPacketStruc::Add(const char * pFldName, const char * pType, const char * pValue)
{
	int    ok = 1;
	InnerItem new_item;
	MEMSZERO(new_item);
	THROW(R_R.SetString(pFldName, &new_item.NameP));
	THROW(R_R.SetString(pType, &new_item.TypeP));
	THROW(R_R.SetString(pValue, &new_item.ValueP));
	new_item.Flags |= SOAPIF_RESOLVED;
	THROW(insert(&new_item));
	CATCHZOK
	return ok;
}

SoapPacketStruc * SoapPacketStruc::AddStruc(const char * pFldName, const char * pType)
{
	assert(ArrList.getCount() >= 1);

	SoapPacketStruc * p_inner = 0;
	InnerItem new_item;
	MEMSZERO(new_item);
	THROW(R_R.SetString(pFldName, &new_item.NameP));
	THROW(R_R.SetString(pType, &new_item.TypeP));
	new_item.Flags |= SOAPIF_STRUC;
	{
		uint new_struc_pos = 0;
		THROW(ArrList.at(0)->CreateItem(&new_struc_pos));
		p_inner = ArrList.at(0)->Get(new_struc_pos);
		p_inner->TypeP = new_item.TypeP;
		new_item.ValueP = new_struc_pos;
	}
	THROW(insert(&new_item));
	CATCH
		p_inner = 0;
	ENDCATCH
	return p_inner;
}

SoapPacketArray * SoapPacketStruc::AddArray(const char * pFldName, const char * pType)
{
	assert(ArrList.getCount() >= 1);

	SoapPacketArray * p_new_array = 0;
	InnerItem new_item;
	MEMSZERO(new_item);
	THROW(R_R.SetString(pFldName, &new_item.NameP));
	THROW(R_R.SetString(pType, &new_item.TypeP));
	new_item.Flags |= SOAPIF_ARRAY;
	{
		uint new_struc_pos = 0;
		p_new_array = new SoapPacketArray(R_R);
		ArrList.insert(p_new_array);
		new_item.ValueP = (ArrList.getCount()-1);
	}
	THROW(insert(&new_item));
	CATCH
		p_new_array = 0;
	ENDCATCH
	return p_new_array;
}

int SoapPacketStruc::AddResolvedRef(const char * pRef, const char * pType, const char * pValue)
{
	int    ok = 1;
	InnerItem new_item;
	MEMSZERO(new_item);
	THROW(R_R.SetString(pRef, &new_item.RefP));
	THROW(R_R.SetString(pType, &new_item.TypeP));
	THROW(R_R.SetString(pValue, &new_item.ValueP));
	THROW(insert(&new_item));
	CATCHZOK
	return ok;
}

int SoapPacketStruc::AddRef(const char * pFldName, const char * pRef)
{
	int    ok = 1;
	InnerItem new_item;
	MEMSZERO(new_item);
	THROW(R_R.SetString(pFldName, &new_item.NameP));
	THROW(R_R.SetString(pRef, &new_item.RefP));
	THROW(insert(&new_item));
	CATCHZOK
	return ok;
}

SoapPacket::SoapPacket() : T(8192, 0), B(*this)
{
	SymbCounter = 0;
	State = 0;
}

SoapPacket & SoapPacket::Clear()
{
	State = 0;
	FaultCode = 0;
	FaultText = 0;
	B.Clear();
	return *this;
}

int SoapPacket::SetString(const char * pStr, uint * pP)
{
	int    ok = 1;
	uint   val = 0, pos = 0;
	if(!isempty(pStr)) {
		if(T.Search(pStr, &val, pP)) {
			ok = 2;
		}
		else {
			THROW(T.Add(pStr, ++SymbCounter, pP));
		}
	}
	else {
		ASSIGN_PTR(pP, 0);
	}
	CATCHZOK
	return ok;
}

int SoapPacket::GetStringPos(const char * pStr, uint * pP) const
{
	int    ok = 0;
	uint   val = 0;
	ASSIGN_PTR(pP, 0);
	if(isempty(pStr))
		ok = -1;
	else if(T.Search(pStr, &val, pP))
		ok = 1;
	return ok;
}

int SoapPacket::GetString(uint pos, SString & rBuf) const
{
	return T.Get(pos, rBuf);
}

int SoapPacket::ParseType(SString & rTypeBuf, uint * pArraySize) const
{
	int    ok = 1;
	uint   array_size = 0;
	if(rTypeBuf.NotEmpty()) {
		SString temp_buf, right_buf;
		if(rTypeBuf.Divide(':', temp_buf, right_buf) > 0)
			rTypeBuf = right_buf;
		size_t par_pos = 0;
		const char * p_par = rTypeBuf.SearchChar('[', &par_pos);
		if(p_par) {
			array_size = (uint)(temp_buf = (rTypeBuf+par_pos+1)).ToLong();
			rTypeBuf.Trim(par_pos);
		}
	}
	else
		ok = -1;
	ASSIGN_PTR(pArraySize, array_size);
	return ok;
}

int SoapPacket::Parse(const char * pMethodName, void * pDoc, const char * pDebugFileName)
{
	int    ok = 1;
#if 1 // @construction {
	SString temp_buf, array_type_buf, type_buf, response_text, return_text, multi_ref_text, fault_text, href_buf;
	SString left_buf, right_buf, id_buf, item_id_buf, item_type_buf, content_buf;
	uint    array_size = 0;
	// Envelope / Body / getGoodsByNameResponse / getGoodsByNameReturn[], multiRef
	xmlDoc * p_doc = (xmlDoc *)pDoc;
	xmlNode * p_child = p_doc->children;
	if(pDebugFileName) {
		SFile debug_file(pDebugFileName, SFile::mWrite);
		if(debug_file.IsValid())
			xmlDocFormatDump(debug_file, p_doc, 1);
	}
	while(p_child && strcmp((const char *)p_child->name, "Envelope") != 0) {
		p_child = p_child->next;
	}
	if(p_child) {
		p_child = p_child->children;
		while(p_child && strcmp((const char *)p_child->name, "Body") != 0) {
			p_child = p_child->next;
		}
		if(p_child) {
			int    response_detected = 0;
			SoapPacketStruc resolve_items(*this); // Сюда временно сбрасываются разрешенные ссылки, которые потом разносятся по структурам
			(response_text = pMethodName).Cat("Response");
			(return_text = pMethodName).Cat("Return");
			fault_text = "Fault";
			multi_ref_text = "multiRef";
			xmlNode * p_body_child = p_child;
			temp_buf = "multiRef";
			for(p_child = p_body_child->children; p_child; p_child = p_child->next) {
				id_buf = 0;
				type_buf = 0;
				href_buf = 0;
				content_buf = 0;
				array_type_buf = 0;
				array_size = 0;
				if(response_text == (const char *)p_child->name && !response_detected) {
					response_detected = 1;
					enum {
						retUndef = 0,
						retRef,
						retArray,
						retItem
					};
					int    ret_type = retUndef;
					uint   item_pos = 0;
					xmlNode * p_array_item = 0;
					for(xmlNode * p_ret_child = p_child->children; ret_type == retUndef && p_ret_child; p_ret_child = p_ret_child->next) {
						if(return_text == (const char *)p_ret_child->name) {
							for(xmlAttr * p_prop = p_ret_child->properties; p_prop; p_prop = p_prop->next) {
								if(p_prop->type == XML_ATTRIBUTE_NODE) {
									if(strcmp((const char *)p_prop->name, "type") == 0) {
										type_buf = (const char *)p_prop->children->content;
									}
									else if(strcmp((const char *)p_prop->name, "arrayType") == 0) {
										array_type_buf = (const char *)p_prop->children->content;
									}
									else if(strcmp((const char *)p_prop->name, "href") == 0) {
										(href_buf = (const char *)p_prop->children->content).ShiftLeftChr('#');
										ret_type = retRef;
									}
								}
							}
							ParseType(array_type_buf, &array_size);
							ParseType(type_buf, 0);
							if(type_buf == "Array") {
								ret_type = retArray;
								p_array_item = p_ret_child->children;
							}
							else if(ret_type != retRef && type_buf.NotEmpty()) {
								ret_type = retItem;
								content_buf = p_ret_child->children ? (const char *)p_ret_child->children->content : 0;
							}
						}
					}
					if(ret_type == retArray) {
						assert(p_array_item != 0);
						// SoapPacket
						SoapPacketArray * p_soap_array = B.AddArray(return_text, array_type_buf);
						THROW(p_soap_array);
						for(; p_array_item; p_array_item = p_array_item->next) {
							int    array_item_ret_type = retUndef;
							for(xmlAttr * p_prop = p_array_item->properties; p_prop; p_prop = p_prop->next) {
								if(p_prop->type == XML_ATTRIBUTE_NODE) {
									if(strcmp((const char *)p_prop->name, "href") == 0) {
										if(p_prop->children) {
											(href_buf = (const char *)p_prop->children->content).ShiftLeftChr('#');
											array_item_ret_type = retRef;
										}
									}
									else if(strcmp((const char *)p_prop->name, "type") == 0) {
										if(p_prop->children) {
											type_buf = (const char *)p_prop->children->content;
											ParseType(type_buf, 0);
											array_item_ret_type = retItem;
										}
									}
								}
							}
							if(array_item_ret_type == retRef) {
								THROW(p_soap_array->CreateItemRef(href_buf, array_type_buf, &(item_pos = 0)));
							}
							else if(array_item_ret_type == retItem) {
								SoapPacketStruc * p_new_item = 0;
								THROW(p_soap_array->CreateItem(&(item_pos = 0)));
								THROW(p_new_item = p_soap_array->Get(item_pos));
								temp_buf = (const char *)p_array_item->content;
								THROW(p_new_item->Add((const char *)p_array_item->name, type_buf, temp_buf));
							}
						}
					}
					else if(ret_type == retRef) {
						THROW(B.AddRef(return_text, href_buf));
					}
					else if(ret_type == retItem) {
						THROW(B.Add(return_text, type_buf, content_buf));
					}
				}
				else if(multi_ref_text == (const char *)p_child->name) {
					xmlAttr * p_prop = 0;
					for(p_prop = p_child->properties; p_prop; p_prop = p_prop->next) {
						if(p_prop->type == XML_ATTRIBUTE_NODE && strcmp((const char *)p_prop->name, "id") == 0) {
							(id_buf = (const char *)p_prop->children->content).ShiftLeftChr('#');
						}
						else if(p_prop->type == XML_ATTRIBUTE_NODE && strcmp((const char *)p_prop->name, "type") == 0) {
							array_type_buf = (const char *)p_prop->children->content;
							ParseType(array_type_buf, &array_size);
						}
					}
					if(id_buf.NotEmpty()) {
						SoapPacketStruc * p_item = B.SearchItemRef(id_buf, array_type_buf);
						if(p_item) {
							for(xmlNode * p_item_child = p_child->children; p_item_child; p_item_child = p_item_child->next) {
								item_id_buf = 0;
								item_type_buf = 0;
								for(p_prop = p_item_child->properties; p_prop; p_prop = p_prop->next) {
									if(p_prop->type == XML_ATTRIBUTE_NODE && strcmp((const char *)p_prop->name, "href") == 0) {
										(item_id_buf = (const char *)p_prop->children->content).ShiftLeftChr('#');
									}
									else if(p_prop->type == XML_ATTRIBUTE_NODE && strcmp((const char *)p_prop->name, "type") == 0) {
										item_type_buf = (const char *)p_prop->children->content;
										if(item_type_buf.Divide(':', left_buf, right_buf) > 0)
											item_type_buf = right_buf;
									}
								}
								if((item_id_buf.NotEmpty() || item_type_buf.NotEmpty()) && !(item_id_buf.NotEmpty() && item_type_buf.NotEmpty())) {
									if(item_id_buf.NotEmpty()) {
										THROW(p_item->AddRef((const char *)p_item_child->name, item_id_buf));
									}
									else { // item_type_buf.NotEmpty();
										assert(item_type_buf.NotEmpty());
										temp_buf = (const char *)(p_item_child->children ? p_item_child->children->content : p_item_child->content);
										THROW(p_item->Add((const char *)p_item_child->name, item_type_buf, temp_buf));
									}
								}
								else {
									temp_buf.Z().Cat(item_id_buf).CatDiv(':', 1).Cat(item_type_buf);
									CALLEXCEPT_S_S(SLERR_SOAPR_ITEMREFTYPECONFL, temp_buf);
								}
							}
						}
						else {
							temp_buf = (const char *)(p_child->children ? p_child->children->content : p_child->content);
							THROW(resolve_items.AddResolvedRef(id_buf, array_type_buf, temp_buf));
						}
					}
				}
				else if(fault_text == (const char *)p_child->name) {
					/*
						<?xml version="1.0" encoding="UTF-8"?>
						<soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  						<soapenv:Body>
    						<soapenv:Fault>
      						<faultcode>soapenv:Server.userException</faultcode>
      						<faultstring>java.rmi.RemoteException: Неверное имя пользователя или пароль</faultstring>
      						<detail>
        						<ns1:hostname xmlns:ns1="http://xml.apache.org/axis/">UHTTSERVER-01</ns1:hostname>
      						</detail>
    						</soapenv:Fault>
  						</soapenv:Body>
						</soapenv:Envelope>
					*/
					State |= stFault;
					for(xmlNode * p_item_child = p_child->children; p_item_child; p_item_child = p_item_child->next) {
						if(sstreqi_ascii((const char *)p_item_child->name, "faultcode")) {
							FaultCode = (const char *)(p_item_child->children ? p_item_child->children->content : p_item_child->content);
							FaultCode.Transf(CTRANSF_UTF8_TO_INNER);
						}
						else if(sstreqi_ascii((const char *)p_item_child->name, "faultstring")) {
							FaultText = (const char *)(p_item_child->children ? p_item_child->children->content : p_item_child->content);
							FaultText.Transf(CTRANSF_UTF8_TO_INNER);
						}
					}
				}
			}
			THROW(B.ResolveRefs(resolve_items));
		}
	}
	CATCHZOK
#endif // } @construction
	return ok;
}

int SoapPacketStruc::ResolveRefs(const SoapPacketStruc & rResolveList)
{
	int    ok = 1;
	SString struc_ref_buf, array_type_buf, item_name_buf, temp_buf;
	R_R.GetString(RefP, struc_ref_buf);
	R_R.GetString(TypeP, array_type_buf);
	for(uint i = 0; i < getCount(); i++) {
		SoapPacketStruc::InnerItem * p_item = static_cast<SoapPacketStruc::InnerItem *>(at(i));
		if(p_item->Flags & SOAPIF_ARRAY) {
			assert(p_item->ValueP > 0 && p_item->ValueP < ArrList.getCount());
			SoapPacketArray * p_array = ArrList.at(p_item->ValueP);
			if(p_array) {
				for(uint k = 0; k < p_array->GetCount(); k++) {
					SoapPacketStruc * p_struc = p_array->Get(k);
					if(p_struc) {
						THROW(p_struc->ResolveRefs(rResolveList)); // @recursion
					}
				}
			}
		}
		else if(p_item->Flags & SOAPIF_STRUC) {
			assert(p_item->ValueP < ArrList.getCount());
			SoapPacketStruc * p_struc = ArrList.at(0)->Get(p_item->ValueP);
			if(p_struc) {
				THROW(p_struc->ResolveRefs(rResolveList)); // @recursion
			}
		}
		else if(!(p_item->Flags & SOAPIF_RESOLVED)) {
			int    resolved = 0;
			R_R.GetString(p_item->RefP, item_name_buf);
			temp_buf.Z().Cat(array_type_buf).CatDiv(':', 1).Cat(struc_ref_buf).CatDiv(':', 1).Cat(item_name_buf);
			THROW_S_S(p_item->RefP, SLERR_SOPAR_UNRESITEMHASNTREF, temp_buf);
			for(uint k = 0; !resolved && k < rResolveList.getCount(); k++) {
				SoapPacketStruc::InnerItem * p_ritem = static_cast<SoapPacketStruc::InnerItem *>(rResolveList.at(k));
				if(p_ritem->RefP == p_item->RefP) {
					p_item->TypeP = p_ritem->TypeP;
					p_item->ValueP = p_ritem->ValueP;
					p_item->Flags |= SOAPIF_RESOLVED;
					resolved = 1;
				}
			}
			THROW_S_S(resolved, SLERR_SOAPR_UNRESOLVEDITEM, temp_buf);
		}
	}
	CATCHZOK
	return ok;
}

int SoapPacket::Write(const char * pUrn, const char * pMethodName, void ** ppXmlDoc, const char * pDebugFileName)
{
	/*
		<?xml version="1.0" encoding="UTF-8"?>

		<soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
		<soapenv:Body>
			<ns1:setImageByID soapenv:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xmlns:ns1="urn:http.service.uhtt.ru">
				<token xsi:type="xsd:string">AuthToken</token>
				<objectType xsi:type="xsd:string">GOODS</objectType>
				<objectID href="#id0"/>
				<image href="#id1"/>
			</ns1:setImageByID>
			<multiRef id="id1" soapenc:root="0" soapenv:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xsi:type="ns2:Document" xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:ns2="http://bean.universehtt.petroglif.ru">
				<contentType xsi:type="soapenc:string">image/jpg</contentType>
				<data xsi:type="soapenc:string">BASE64 DATA</data>
				<encoding xsi:type="soapenc:string">BASE64</encoding>
				<name xsi:type="soapenc:string">picture.jpg</name>
				<size href="#id2"/>
			</multiRef>
			<multiRef id="id0" soapenc:root="0" soapenv:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xsi:type="xsd:long" xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/">10023</multiRef>
			<multiRef id="id2" soapenc:root="0" soapenv:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xsi:type="xsd:long" xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/">0</multiRef>
		</soapenv:Body>
		</soapenv:Envelope>
	*/
	int    ok = 1;
#if 1 // {
	const char * p_nss_env = "soapenv";
	const char * p_nss_inner = "ns1";
	const char * p_nss_xsi = "xsi";
	const char * p_nss_xsd = "xsd";
	xmlDoc * p_doc = 0;
	xmlNode * p_env_node = 0;
	xmlNode * p_body_node = 0;
	xmlNs * p_ns = 0; // xmlNewNs
	SString temp_buf, type_buf;
	THROW(p_doc = xmlNewDoc(0));
	p_doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
	THROW(p_ns = xmlNewNs(0, (const xmlChar *)"http://schemas.xmlsoap.org/soap/envelope/", (const xmlChar *)p_nss_env));
	THROW(p_env_node = xmlNewDocNode(p_doc, p_ns, (const xmlChar *)"Envelope", 0));
	xmlNewProp(p_env_node, (const xmlChar *)"xmlns:soapenv", (const xmlChar *)"http://schemas.xmlsoap.org/soap/envelope/");
	xmlNewProp(p_env_node, (const xmlChar *)"xmlns:xsd",     (const xmlChar *)"http://www.w3.org/2001/XMLSchema");
	xmlNewProp(p_env_node, (const xmlChar *)"xmlns:xsi",     (const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance");
	xmlDocSetRootElement(p_doc, p_env_node);
	THROW(p_body_node = xmlNewDocNode(p_doc, p_ns, (const xmlChar *)"Body", 0));
	xmlAddChild(p_env_node, p_body_node);
	if(pMethodName) {
		xmlNode * p_method_node = 0;
		xmlNs * p_ns_inner = xmlNewNs(0, 0, (const xmlChar *)p_nss_inner);
		THROW(p_ns_inner);
		THROW(p_method_node = xmlNewDocNode(p_doc, p_ns_inner, (const xmlChar *)pMethodName, 0));
		(temp_buf = p_nss_env).Colon().Cat("encodingStyle");
		xmlNewProp(p_method_node, temp_buf.ucptr(), (const xmlChar *)"http://schemas.xmlsoap.org/soap/envelope/");
		if(pUrn) {
			(temp_buf = "xmlns").Colon().Cat(p_nss_inner);
			xmlNewProp(p_method_node, temp_buf.ucptr(), (const xmlChar *)pUrn);
		}
		xmlAddChild(p_body_node, p_method_node);
		{
			// SoapPacketStruc
			const uint c = B.GetCount();
			SoapPacketItem spi;
			for(uint i = 0; i < c; i++) {
				if(B.Get(i, spi)) {
					if(spi.Flags & SOAPIF_ARRAY) {
						if(spi.P_Array) {
						}
					}
					else if(spi.Flags & SOAPIF_STRUC) {
						if(spi.P_Struc) {
						}
					}
					else {
						// <token xsi:type="xsd:string">AuthToken</token>
						xmlNode * p_node = xmlNewDocNode(p_doc, 0, spi.Name.ucptr(), spi.Value.ucptr());
						THROW(p_node);
						temp_buf.Z().Cat(p_nss_xsi).Colon().Cat("type");
						type_buf.Z().Cat(p_nss_xsd).Colon().Cat(spi.TypeName);
						xmlNewProp(p_node, temp_buf.ucptr(), type_buf.ucptr());
						xmlAddChild(p_method_node, p_node);
					}
				}
			}
		}
	}
	if(pDebugFileName) {
		SFile debug_file(pDebugFileName, SFile::mWrite);
		if(debug_file.IsValid())
			xmlDocFormatDump(debug_file, p_doc, 1);
	}
 	CATCH
		ok = 0;
		ZFREE(p_doc);
	ENDCATCH
	ASSIGN_PTR(ppXmlDoc, p_doc);
#endif // } 0
	return ok;
}
#endif // } @construction
