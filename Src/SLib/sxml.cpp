// SXML.CPP
// Copyright (c) A.Sobolev, 2002, 2007, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026
// @codepage UTF-8
// Вспомогательные механизмы для работы с XML
//
#include <slib-internal.h>
#pragma hdrstop
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
	{ '\x2c', 0, "comma"  },  // ,
	{ '\x2e', 0, "dot"    },  // .
	{ '\x2f', 0, "fwsl"   },  // /
	{ '\x3c', 1, "lt"     },  // <
	{ '\x3d', 0, "eq"     },  // =
	{ '\x3e', 0, "gt"     },  // >
	{ '\x3f', 0, "ques"   },  // ?
	{ '\x5b', 0, "lsq"    },  // [
	{ '\x5c', 0, "bksl"   },  // backslash (\)
	{ '\x5d', 0, "rsq"    },  // ]
	// { '\x59', 0, "#59"    },
};

void FASTCALL XMLReplaceSpecSymb(SString & rBuf, const char * pProcessSymb)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	r_temp_buf = rBuf;
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

/*static*/const SString & SXml::Helper_EncXmlText(const SString & rS, int ctransf, SString & rBuf)
{
	(rBuf = rS).ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(rBuf, "&<>\'");
	return ctransf ? rBuf.Transf(ctransf) : rBuf;
}

/*static*/const SString & SXml::Helper_EncXmlText(const char * pS, int ctransf, SString & rBuf)
{
	(rBuf = pS).ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(rBuf, "&<>\'");
	return ctransf ? rBuf.Transf(ctransf) : rBuf;
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
		SString & r_temp_buf = SLS.AcquireRvlStr();
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

int SXml::WNode::PutInnerReal(const char * pInnerName, double value, long fmt) // @v12.5.2
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	return PutInner(pInnerName, r_temp_buf.Cat(value, fmt));
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
		SString & r_temp_buf = SLS.AcquireRvlStr();
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
			if(p_children) {
				if(p_children->type == XML_TEXT_NODE && p_children->content) {
					rResult.Set(p_children->content);
					ok = true;
				}
				// @v12.2.1 {
				else if(p_children->type == XML_CDATA_SECTION_NODE && p_children->content) {
					rResult.Set(p_children->content);
					ok = true;					
				}
				// } @v12.2.1 
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

/*extern "C"*/xmlParserCtxt * xmlCreateURLParserCtxt(const char * filename, int options);
void FASTCALL xmlDetectSAX2(xmlParserCtxt * ctxt); // @prototype

int SXmlSaxParser::ParseFile(const char * pFileName)
{
	int    ok = 1;
	THROW(fileExists(pFileName));
	{
		xmlSAXHandler saxh;
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
	if(ValList.SearchByText(pVal, &pos)) {
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
	if(ValList.SearchByText(pVal, &pos)) {
		id = ValList.at(pos).Id;
		ok = 1;
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

#endif // } 0 @construction
