// XMLTABLE.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
// @v11.7.9 #include <..\osf\libxml\libxml.h>
#include <..\slib\libxml\libxml.h> // @v11.7.9

const int __EnableEmptyRoot = 1;
//
// @ModuleDef(XmlDbFile)
//
XmlDbFile::State::State() : P(0, 0, 0, 0)
{
	P_SplittedRecTag = 0;
	Reset();
}

XmlDbFile::State::~State()
{
	delete P_SplittedRecTag;
}

XmlDbFile::State::State(const State & rS) : P(rS.P)
{
	Copy(rS);
}

XmlDbFile::State & XmlDbFile::State::operator = (const XmlDbFile::State & rS)
{
	Copy(rS);
	return *this;
}

void XmlDbFile::State::Reset()
{
	Pos = 0;
	NumRecs = 0;
	P_CurRec = 0;
	P_LastRec = 0;
	ZDELETE(P_SplittedRecTag);
	P.Init(0, 0, 0, 0);
}

int FASTCALL XmlDbFile::State::Copy(const XmlDbFile::State & rS)
{
	Pos = rS.Pos;
	NumRecs = rS.NumRecs;
	P_CurRec = rS.P_CurRec;
	P_LastRec = rS.P_LastRec;
	P = rS.P;
	if(rS.P_SplittedRecTag) {
		if(SETIFZ(P_SplittedRecTag, new StringSet))
			*P_SplittedRecTag = *rS.P_SplittedRecTag;
	}
	else
		ZDELETE(P_SplittedRecTag);
	return 1;
}

int XmlDbFile::State::SetParam(const Param * pParam)
{
	int    ok = 1;
	ZDELETE(P_SplittedRecTag);
	if(pParam) {
		P = *pParam;
		if(P.RecTag.HasChr('/') || P.RecTag.HasChr('\\')) {
			P_SplittedRecTag = new StringSet;
			if(P_SplittedRecTag) {
				P.RecTag.Tokenize("/\\", *P_SplittedRecTag);
				P_SplittedRecTag->reverse();
			}
			else
				ok = 0;
		}
	}
	else {
		P.Init(0, 0, 0, 0);
	}
	return ok;
}

int FASTCALL XmlDbFile::State::IsRecTag(const char * pTag) const
{
	return BIN(stricmp(pTag, P.RecTag) == 0);
}

int FASTCALL XmlDbFile::State::IsRecNode(const xmlNode * pNode) const
{
	int    yes = 0;
	if(pNode) {
		if(P_SplittedRecTag) {
			SString temp_buf;
			const xmlNode * p_node = pNode;
			int   y = 1;
			for(uint p = 0; y && p_node && P_SplittedRecTag->get(&p, temp_buf); p_node = p_node->P_ParentNode) {
				if(temp_buf.CmpNC(PTRCHRC_(p_node->name)) != 0)
					y = 0;
			}
			yes = y;
		}
		else
			yes = BIN(P.RecTag.CmpNC(reinterpret_cast<const char *>(pNode->name)) == 0);
	}
	return yes;
}

const xmlNode * FASTCALL XmlDbFile::State::GetHeadRecNode(const xmlNode * pNode) const
{
	if(P_SplittedRecTag) {
		SString temp_buf;
		const xmlNode * p_node = pNode;
		const xmlNode * p_last = 0;
		for(uint p = 0; p_node && P_SplittedRecTag->get(&p, temp_buf);) {
			if(temp_buf.CmpNC(PTRCHRC_(p_node->name)) != 0) {
				p_last = 0;
				p_node = 0;
			}
			else {
				p_last = p_node;
				p_node = p_node->P_ParentNode;
			}
		}
		return p_last;
	}
	else
		return pNode;
}
//
//
//
#define XMLDBFILEPARAM_SVER 0

XmlDbFile::Param::Param(const char * pRootTag, const char * pHdrTag, const char * pRecTag, long flags)
{
	Init(pRootTag, pHdrTag, pRecTag, flags);
}

void XmlDbFile::Param::Init(const char * pRootTag, const char * pHdrTag, const char * pRecTag, long flags)
{
	Ver = XMLDBFILEPARAM_SVER;
	Flags = flags;
	RootTag = pRootTag;
	HdrTag = pHdrTag;
	RecTag = pRecTag;
	if(RootTag.Len() == 0 && !__EnableEmptyRoot)
		RootTag = PPYXML_ROOTTAG;
	if(RecTag.Len() == 0)
		RecTag = PPYXML_RECTAG;
}

int XmlDbFile::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, Ver, rBuf));
	if(dir < 0) {
		THROW_S_S(Ver == XMLDBFILEPARAM_SVER, SLERR_INVSERIALIZEVER, "XmlDbFile");
	}
	THROW(pCtx->Serialize(dir, Flags, rBuf));
	THROW(pCtx->Serialize(dir, RootTag, rBuf));
	THROW(pCtx->Serialize(dir, RecTag, rBuf));
	THROW(pCtx->Serialize(dir, HdrTag, rBuf));
	CATCHZOK
	return ok;
}

XmlDbFile::XmlDbFile() : P_Writer(0), P_Doc(0), ReadOnly(0), UseSubChild(0), P_Buffer(0)
{
}

XmlDbFile::~XmlDbFile()
{
	Close();
}

void XmlDbFile::SetEntitySpec(const char * pSpec)
{
	EntitySpec = pSpec;
}

static const xmlNode * FASTCALL _XmlNextElem(const xmlNode * pNode)
{
	if(pNode) {
		while((pNode = pNode->next) != 0) {
			if(pNode->type == XML_ELEMENT_NODE)
				return pNode;
		}
	}
	return 0;
}

const xmlNode * XmlDbFile::Helper_FindRec_(const xmlNode * pCurrent) const
{
	const xmlNode * p_target = 0;
	for(const xmlNode * p_rec = pCurrent; p_rec && !p_target; p_rec = _XmlNextElem(p_rec)) {
		if(St.IsRecNode(p_rec)) {
			p_target = p_rec;
		}
		else {
			const xmlNode * p_child = p_rec->children;
			if(p_child) {
				if(p_child->type != XML_ELEMENT_NODE)
					p_child = _XmlNextElem(p_child);
				p_target = Helper_FindRec_(p_child); // @recursion
			}
		}
	}
	return p_target;
}

const xmlNode * XmlDbFile::FindFirstRec_(const xmlNode * pRoot) const
{
	return Helper_FindRec_(pRoot);
}

const xmlNode * XmlDbFile::FindNextRec_(const xmlNode * pCurrent) const
{
	const xmlNode * p_rec = _XmlNextElem(St.GetHeadRecNode(pCurrent));
	return Helper_FindRec_(p_rec);
}

int XmlDbFile::CountRecords(const xmlNode * pRootNode, uint * pCount)
{
	int    ok = 1;
	uint   count = 0;
	const  xmlNode * p_rec = FindFirstRec_(pRootNode);
	if(p_rec) {
		St.P_CurRec = p_rec;
		do {
			count++;
		} while((p_rec = FindNextRec_(p_rec)) != 0);
	}
	St.NumRecs = count;
	ASSIGN_PTR(pCount, count);
	return ok;
}

int XmlDbFile::IsUtf8() const
{
	const Param & r_param = St.GetParam();
	return BIN(r_param.Flags & XmlDbFile::Param::fUtf8Codepage);
}

/*static void _Test_XPath(xmlDoc * pDoc, const char * pXPathExpr)
{
	//xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath)
	{
		xmlXPathObject * result = 0;
		xmlXPathContext * context = xmlXPathNewContext(pDoc);
		if(context == NULL) {
			printf("Error in xmlXPathNewContext\n");
		}
		else {
			result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(pXPathExpr), context);
			xmlXPathFreeContext(context);
			if(!result) {
				//printf("Error in xmlXPathEvalExpression\n");
			}
			if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
				xmlXPathFreeObject(result);
				//printf("No result\n");
			}
		}
	}
}*/

int XmlDbFile::Open(const char * pPath, const Param * pParam, const SdRecord * pRec, int readOnly)
{
	int    ok = 1;
	xmlTextReader * reader = 0;
	St.Pos = 0;
	ReadOnly = readOnly;
	St.SetParam(pParam);
	FileName = pPath;
	const Param & r_param = St.GetParam();
	if(!readOnly) {
        if(sstreqi_ascii(pPath, ":buffer:")) {
            THROW(P_Buffer = xmlBufferCreate());
            THROW(P_Writer = xmlNewTextWriterMemory(P_Buffer, 0));
        }
        else {
            THROW(P_Writer = xmlNewTextWriterFilename(pPath, 0));
        }
		const char * p_codepage = IsUtf8() ? "utf-8" : "windows-1251";
		xmlTextWriterSetIndent(P_Writer, 1);
		xmlTextWriterSetIndentTab(P_Writer);
		xmlTextWriterStartDocument(P_Writer, 0, p_codepage, 0);
		{
			if(pRec)
				WriteDTDS(*pRec);
			if(r_param.RootTag.NotEmpty()) {
				SString tag(r_param.RootTag);
				if(IsUtf8())
					tag.ToUtf8();
				StringSet ss('\\', tag);
				for(uint i = 0; ss.get(&i, tag);)
					xmlTextWriterStartElement(P_Writer, tag.ucptr());
			}
		}
	}
	else {
		int    r = 0;
		int    options = XML_PARSE_NOENT;
		SString pattern;
		if(r_param.Flags & XmlDbFile::Param::fUseDTD)
			options |= XML_PARSE_DTDATTR|XML_PARSE_DTDVALID;
		THROW(reader = xmlReaderForFile(pPath, NULL, options));
		//(pattern = r_param.RecTag).ReplaceChar('\\', '/');
		//(pattern = r_param.RootTag).ReplaceChar('\\', '/');
		pattern.Dot();
		xmlTextReaderPreservePattern(reader, pattern.ucptr(), 0);
		r = xmlTextReaderRead(reader);
		while(r == 1)
			r = xmlTextReaderRead(reader);
		if(!r) {
			THROW(P_Doc = xmlTextReaderCurrentDoc(reader));
			{
				//_Test_XPath(P_Doc, "/orders/order/basket/item");
			}
			xmlNode * p_root = xmlDocGetRootElement(P_Doc);
			CountRecords(p_root, 0);
		}
	}
	CATCH
		SLS.SetError(SLERR_OPENFAULT, pPath);
		ok = 0;
	ENDCATCH
	if(reader)
		xmlFreeTextReader(reader);
	return ok;
}

int XmlDbFile::Helper_CloseWriter()
{
	int    ok = 0;
	if(P_Writer) {
		if(St.GetParam().RootTag.NotEmpty()) {
			SString tag;
			StringSet ss('\\', St.GetParam().RootTag);
			for(uint i = 0; ss.get(&i, tag);)
				xmlTextWriterEndElement(P_Writer);
		}
		xmlTextWriterEndDocument(P_Writer);
		xmlFreeTextWriter(P_Writer);
		P_Writer = 0;
		ok = 1;
	}
	return ok;
}

int XmlDbFile::Close()
{
	if(!Helper_CloseWriter() && P_Doc) {
		xmlFreeDoc(P_Doc);
		P_Doc = 0;
	}
	xmlBufferFree(P_Buffer);
	P_Buffer = 0;
	St.Reset();
	PreserveSt.Reset();
	return 1;
}

int XmlDbFile::GetExportBuffer(SBuffer & rBuf)
{
    int    ok = 1;
    if(P_Buffer) {
		Helper_CloseWriter();
        int   sz = xmlBufferLength(P_Buffer);
        const void * p_content = xmlBufferContent(P_Buffer);
        if(sz > 0 && p_content)
            rBuf.Write(p_content, (size_t)sz);
    }
    else
        ok = -1;
    return ok;
}

int XmlDbFile::Pop()
{
	//if(UseSubChild == 1) { // @vmiller comment
	if(UseSubChild) { // @vmiller
		SString tag;
		//
		// Закроем тег вложенной записи
		//
		if(!ReadOnly) {
			if(St.GetParam().RootTag.NotEmpty()) {
				tag = St.GetParam().RootTag;
				if(IsUtf8())
					tag.ToUtf8();
				StringSet ss('\\', tag);
				for(uint i = 0, j = 0; ss.get(&i, tag);)
					xmlTextWriterEndElement(P_Writer);
			}
		}
		// @vmiller {
		// Восстановим номер позиции состояния из стека
		int pos = 0;
		StateStack.pop(pos);
		// Теперь возьмем состояние из коллекции по позиции pos
		St.Copy(*StateColl.at(pos));
		// } @vmiller

		// @vmiller comment {
		//St = PreserveSt;
		//PreserveSt.Reset();
		// } @vmiller comment
		//UseSubChild = 0; // @vmiller comment
		UseSubChild --; // @vmiller
		//
		// Закроем тег головной записи
		//
		if(St.GetParam().Flags & XmlDbFile::Param::fHaveSubRec) {
			if(!ReadOnly) {
				tag = St.GetParam().RecTag;
				if(IsUtf8())
					tag.ToUtf8();
				StringSet ss('\\', tag);
				for(uint i = 0; ss.get(&i, tag);)
					xmlTextWriterEndElement(P_Writer);
			}
			else {
				St.P_CurRec = FindNextRec_(St.P_CurRec);
			}
		}
	}
	return 1;
}

int XmlDbFile::Push(const Param * pParam)
{
	if(pParam /* @v7.8.2 && !St.IsRecTag(pParam->RecTag) */) {
		const xmlNode * p_last_rec = St.P_LastRec;
		// @vmiller {
		State * p_st = new State();
		p_st->Copy(St);
		// Запомним состояние в коллекцию
        StateColl.insert(p_st);
		// Занесем в стек
		int pos = StateColl.getCount() - 1;
		StateStack.push(pos);
		// } @vmiller
		//PreserveSt = St; // @vmiller comment
		St.Reset();
		St.SetParam(pParam);
		if(ReadOnly) {
			CountRecords(p_last_rec, 0);
		}
		//UseSubChild = 1; // @vmiller comment
		UseSubChild++; // @vmiller
	}
	return 1;
}

/*static*/int XmlDbFile::CheckParam(const Param & rParam)
{
	//SLibError = SLERR_XMLDB_INVRECORROOTTAG;
	//return ((__EnableEmptyRoot || St.P.RootTag.Len()) && St.P.RootTag.IsLatin() && St.P.RecTag.Len() && St.P.RecTag.IsLatin()) ? 1 : 0;
	int    ok = 1;
	uint   i;
	THROW_S(__EnableEmptyRoot || rParam.RootTag.NotEmpty(), SLERR_XMLDB_ROOTTAGEMPTY);
	THROW_S(rParam.RecTag.NotEmpty(), SLERR_XMLDB_RECTAGEMPTY);
	for(i = 0; i < rParam.RootTag.Len(); i++) {
		const uchar c = (uchar)rParam.RootTag.C(i);
		THROW_S(isalnum(c) || oneof4(c, '\\', '/', '_', '-') || IsLetter1251(c), SLERR_XMLDB_ROOTTAGINVCHR);
	}
	for(i = 0; i < rParam.RecTag.Len(); i++) {
		const uchar c = (uchar)rParam.RecTag.C(i);
		THROW_S(isalnum(c) || oneof4(c, '\\', '/', '_', '-') || IsLetter1251(c), SLERR_XMLDB_RECTAGINVCHR);
	}
	CATCHZOK
	return ok;
}

int XmlDbFile::GetRecord(const SdRecord & rRec, void * pDataBuf)
{
	int    ok = -1;
	THROW(CheckParam(St.GetParam()));
	if(P_Doc && St.Pos >= 0 && St.Pos < St.NumRecs && St.P_CurRec) {
		SdbField fld;
		StringSet fld_set;
		SString field_buf, temp_buf;
		SFormatParam fmt_param;
		for(uint fld_pos = 0; rRec.GetFieldByPos(fld_pos, &fld) > 0; fld_pos++) {
			int    r = -1;
			field_buf.Z();
			fld_set.Z();
			if(fld.Name.NotEmpty()) {
				uint   fld_count = 0; // Счетчик тегов в имени поля, разделенных слышом (\) - т.е.
					// уровень вложенности поля.
				//
				// Предварительная обработка имени поля для идентификации вложенности тегов
				// Например: "TAG-LEVEL-1/TAG-LEVEL-2/TAG-LEVEL-3"
				//
				if(fld.Name.HasChr('/') || fld.Name.HasChr('\\')) {
					const char * p = 0;
					const size_t len = fld.Name.Len();
					for(size_t start = 0; start < len;) {
						const char * p_fld_name = (fld.Name + start);
						p = sstrchr(p_fld_name, '/');
						SETIFZ(p, sstrchr(p_fld_name, '\\'));
						if(p) {
							size_t pos = (p - p_fld_name);
							temp_buf.Z().CatN(p_fld_name, pos-start);
							start = pos+1;
						}
						else {
							temp_buf.Z().Cat(p_fld_name);
							start = len;
						}
						fld_set.add(temp_buf);
						fld_count++;
					}
				}
				else {
					fld_set.add(fld.Name);
					fld_count++;
				}
				const xmlNode * p_rec = St.P_CurRec;
				for(uint fp = 0, fno = 0; r < 0 && fld_set.get(&fp, temp_buf);) {
					fno++;
					for(const xmlNode * p_fld = p_rec->children; p_fld != 0; p_fld = p_fld->next) {
						if(temp_buf.CmpNC((const char *)p_fld->name) == 0) {
							if(fno == fld_count) {
								p_fld = p_fld->children;
								if(p_fld) {
									if(p_fld->type == XML_CDATA_SECTION_NODE) {
										if(p_fld->content)
											field_buf.CopyFrom((const char *)p_fld->content);
									}
									else if(p_fld->name && stricmp((const char *)p_fld->name, PPYXML_TAGCONT) == 0) {
										if(p_fld->content)
											field_buf.CopyFrom((const char *)p_fld->content);
									}
								}
								if(St.GetParam().Flags & XmlDbFile::Param::fUtf8Codepage)
									field_buf.Utf8ToChar();
								fld.PutFieldDataToBuf(field_buf, pDataBuf, fmt_param);
								r = 1;
							}
							else
								p_rec = p_fld;
							break;
						}
					}
				}
			}
		}
		St.P_LastRec = St.P_CurRec;
		St.Pos++;
		ok = 1;
		if(!(St.GetParam().Flags & XmlDbFile::Param::fHaveSubRec)) {
			St.P_CurRec = FindNextRec_(St.P_CurRec);
		}
	}
	else
		St.P_LastRec = 0;
	CATCHZOK
	return ok;
}

int XmlDbFile::AppendRecord(const char * pRecTag, const SdRecord & rRec, const void * pDataBuf)
{
	int    ok = -1;
	THROW(CheckParam(St.GetParam()));
	THROW_S(!isempty(pRecTag), SLERR_XMLDB_INVRECORROOTTAG);
	if(P_Writer) {
		SdbField fld;
		SString line, field_name, tag;
		SFormatParam fp;
		StringSet ss("\\");
		fp.Flags |= SFormatParam::fFloatSize;
		tag = pRecTag; /*St.P.RecTag*/
		if(IsUtf8())
			tag.ToUtf8();
		ss.setBuf(tag);
		for(uint i = 0; ss.get(&i, tag);)
			xmlTextWriterStartElement(P_Writer, tag.ucptr());
		for(uint i = 0; rRec.EnumFields(&i, &fld);) {
			field_name = fld.Name;
			if(!field_name.NotEmptyS())
				field_name.Cat("fld").Cat(fld.ID); // @vmiller add --> Cat("fld")
			fld.GetFieldDataFromBuf(line, pDataBuf, fp);
			WriteField(field_name, line, 0);
		}
		if(!(St.GetParam().Flags & XmlDbFile::Param::fHaveSubRec))
			xmlTextWriterEndElement(P_Writer);
		St.Pos++;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int XmlDbFile::AppendRecord(const SdRecord & rRec, const void * pDataBuf)
{
	return AppendRecord(St.GetParam().RecTag, rRec, pDataBuf);
}

ulong XmlDbFile::GetNumRecords() const
{
	return P_Doc ? St.NumRecs : 0;
}

int XmlDbFile::WriteField(const char * pFieldName, const char * pFieldValue, int isDtd)
{
	int    ok = -1;
	if(P_Writer && pFieldName) {
		SString & r_fn_buf = SLS.AcquireRvlStr();
		//SString fn_buf(pFieldName);
		r_fn_buf = pFieldName;
		if(!r_fn_buf.IsAscii()) { // @v10.9.7
			if(IsUtf8() && !r_fn_buf.IsLegalUtf8()) // @v10.9.7 (&& !fn_buf.IsLegalUtf8())
				r_fn_buf.ToUtf8();
		}
		if(isDtd) {
			xmlTextWriterWriteDTDElement(P_Writer, r_fn_buf.ucptr(), (const xmlChar *)"(#PCDATA)");
			ok = 1;
		}
		else if(pFieldValue) {
			//SString sbuf(pFieldValue);
			SString & r_sbuf = SLS.AcquireRvlStr();
			r_sbuf = pFieldValue;
			if(!r_sbuf.IsAscii()) { // @v10.9.7
				if(IsUtf8() && !r_sbuf.IsLegalUtf8()) // @v10.9.7 (&& !sbuf.IsLegalUtf8())
					r_sbuf.ToUtf8();
			}
			XMLReplaceSpecSymb(r_sbuf, EntitySpec.NotEmpty() ? EntitySpec.cptr() : 0);
			xmlTextWriterWriteElement(P_Writer, r_fn_buf.ucptr(), r_sbuf.ucptr());
			ok = 1;
		}
	}
	return ok;
}

int XmlDbFile::WriteDTDS(const SdRecord & rRec)
{
	int    ok = -1;
	if(P_Writer) {
		const long pflags = St.GetParam().Flags;
		SString root_tag = St.GetParam().RootTag;
		SString rec_tag = St.GetParam().RecTag;
		if(IsUtf8()) {
			root_tag.ToUtf8();
			rec_tag.ToUtf8();
		}
		if(pflags & XmlDbFile::Param::fUseDTD && !UseSubChild) {
			uint   i;
			SString buf, field_name;
			SdbField fld;
			StringSet ss(",");
			for(i = 0; i < rRec.GetCount(); i++) {
				THROW(rRec.GetFieldByPos(i, &fld));
				field_name = fld.Name;
				if(!field_name.NotEmptyS())
					field_name.Cat(fld.ID);
				ss.add(field_name);
			}
			xmlTextWriterStartDTD(P_Writer, root_tag.ucptr(), 0, 0);
			if(!(pflags & XmlDbFile::Param::fSkipEntityList))
				XMLWriteSpecSymbEntities(P_Writer);
			buf.Printf("(%s+)", rec_tag.cptr());
			xmlTextWriterWriteDTDElement(P_Writer, root_tag.ucptr(), buf.ucptr());
			buf.Printf("(%s)", ss.getBuf());
			xmlTextWriterWriteDTDElement(P_Writer, rec_tag.ucptr(), buf.ucptr());
			for(i = 0; ss.get(&i, buf.Z());)
				WriteField(buf, 0, 1);
			xmlTextWriterEndDTD(P_Writer);
		}
		else if(!(pflags & XmlDbFile::Param::fSkipEntityList)) {
			xmlTextWriterStartDTD(P_Writer, root_tag.ucptr(), 0, 0);
			XMLWriteSpecSymbEntities(P_Writer);
			xmlTextWriterEndDTD(P_Writer);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

const char * XmlDbFile::GetFileName() const
{
	return (const char *)FileName;
}

#if SLTEST_RUNNING // {
/* @construction
SLTEST_R(XmlDbFile)
{
	int    ok = 1;
	CATCHZOK
	return ok;
}
*/
#endif // } SLTEST_RUNNING
