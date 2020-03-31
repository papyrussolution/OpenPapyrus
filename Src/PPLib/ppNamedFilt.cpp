// PPNAMEDFILT.CPP
// Copyright (c) P.Andrianov 2011, 2014, 2016, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

static const long Current_PPNamedFilt_Ver = 3; // @v10.6.7 2-->3

SLAPI PPNamedFilt::PPNamedFilt() : ID(0), Ver(Current_PPNamedFilt_Ver), ViewID(0), Flags(0) //@erik ver 0-->1 // @v10.5.3 ver 1-->2
{
	memzero(Reserve, sizeof(Reserve));
}

PPNamedFilt & FASTCALL PPNamedFilt::operator = (const PPNamedFilt & s)
{
	ID = s.ID;
	Ver = s.Ver;
	ViewID = s.ViewID;
	Flags = s.Flags;
	Name = s.Name;
	DbSymb = s.DbSymb;
	Symb = s.Symb;
	ViewSymb = s.ViewSymb;
	Param = s.Param;
	VD = s.VD;
	DestGuaList = s.DestGuaList; // @v10.5.3
	return *this;
}

int SLAPI PPNamedFilt::Write(SBuffer & rBuf, long p) // @erik const -> notConst
{
	SSerializeContext sctx;
	int    ok = 1;
	Ver = Current_PPNamedFilt_Ver; // @v10.5.3
	THROW_SL(rBuf.Write(ID));
	THROW_SL(rBuf.Write(Ver));
	THROW_SL(rBuf.Write(ViewID));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Write(Name));
	THROW_SL(rBuf.Write(DbSymb));
	THROW_SL(rBuf.Write(Symb));
	THROW_SL(rBuf.Write(ViewSymb));
	THROW_SL(rBuf.Write(Param));
	{
		//THROW_SL(VD.Serialize(+1, rBuf, &sctx));
		//int PPNamedFilt::ViewDefinition::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
		THROW_SL(sctx.Serialize(+1, &VD.L, rBuf));
		THROW_SL(VD.SStrGroup::SerializeS(+1, rBuf, &sctx));
		THROW_SL(sctx.Serialize(+1, VD.StrucSymb, rBuf)); // @v10.6.7
	}
	THROW(DestGuaList.Serialize(+1, rBuf, &sctx)); // @v10.5.3
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFilt::Read(SBuffer & rBuf, long p)
{
	int    ok = 1;
	THROW_SL(rBuf.Read(ID));
	THROW_SL(rBuf.Read(Ver));
	THROW_SL(rBuf.Read(ViewID));
	THROW_SL(rBuf.Read(Flags)); // @v8.4.2 (за счет Reserve)
	THROW_SL(rBuf.Read(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Read(Name));
	THROW_SL(rBuf.Read(DbSymb));
	THROW_SL(rBuf.Read(Symb));
	THROW_SL(rBuf.Read(ViewSymb));
	THROW_SL(rBuf.Read(Param));
	if(Ver > 0) {
		SSerializeContext sctx;
		//THROW(VD.Serialize(-1, rBuf, &sctx));
		THROW_SL(sctx.Serialize(-1, &VD.L, rBuf));
		THROW_SL(VD.SStrGroup::SerializeS(-1, rBuf, &sctx));
		// @v10.6.7 {
		if(Ver >= 3) {
			THROW_SL(sctx.Serialize(-1, VD.StrucSymb, rBuf)); 
		}
		// } @v10.6.7 
		// @v10.5.3 {
		if(Ver > 1) {
			THROW(DestGuaList.Serialize(-1, rBuf, &sctx));
		}
		// } @v10.5.3 
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFilt::Write2(xmlTextWriter * pXmlWriter)
{
	int ok = 1;
	SString temp_buf;
	SBuffer buf;
	SSerializeContext sctx;
	Ver = Current_PPNamedFilt_Ver;
	assert(pXmlWriter);
	THROW(pXmlWriter);
	{
	SXml::WNode pp_nfilt_node(pXmlWriter, "NamedFilt");
	pp_nfilt_node.PutInner("ID", temp_buf.Z().Cat(ID));
	pp_nfilt_node.PutInner("Ver", temp_buf.Z().Cat(Ver));
	pp_nfilt_node.PutInner("ViewID", temp_buf.Z().Cat(ViewID));
	pp_nfilt_node.PutInner("Flags", temp_buf.Z().Cat(Flags));
	XMLReplaceSpecSymb(temp_buf.Z().Cat(Name), "&<>\'");
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	pp_nfilt_node.PutInner("Name", temp_buf);
	XMLReplaceSpecSymb(temp_buf.Z().Cat(DbSymb), "&<>\'");
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	pp_nfilt_node.PutInner("DbSymb", temp_buf);
	XMLReplaceSpecSymb(temp_buf.Z().Cat(Symb), "&<>\'");
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	pp_nfilt_node.PutInner("Symb", temp_buf);
	XMLReplaceSpecSymb(temp_buf.Z().Cat(ViewSymb), "&<>\'");
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	pp_nfilt_node.PutInner("ViewSymb", temp_buf);
	temp_buf.Z().EncodeMime64(static_cast<const char *>(Param.GetBuf(Param.GetRdOffs())), Param.GetAvailableSize());
	pp_nfilt_node.PutInner("Param", temp_buf);
	THROW(VD.XmlWrite(pXmlWriter));
	THROW(XmlWriteGuaList(pXmlWriter));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFilt::XmlWriteGuaList(xmlTextWriter * pXmlWriter)
{
	int    ok = 1;
	SString temp_buf;
	assert(pXmlWriter);
	THROW(pXmlWriter);
	{
		temp_buf.Z();
		if(DestGuaList.IsExists()) {
			for(uint i = 0; i<DestGuaList.GetCount(); i++) {
				temp_buf.Cat(DestGuaList.Get(i));
				if(i<DestGuaList.GetCount()-1) {
					temp_buf.Comma();
				}
			}
		}
		else {
			temp_buf.Cat("undefined");
		}
		SXml::WNode gua_list_node(pXmlWriter, "DestGuaList", temp_buf);
	}
	CATCHZOK;
	return ok;
} 

int SLAPI PPNamedFilt::ReadGuaListFromStr(SString & rGuaListInStr)
{
	int     ok = 1;
	SString temp_buf;
	StringSet ss;
	if(!rGuaListInStr.IsEqiAscii("undefined")) {
		THROW(DestGuaList.InitEmpty());
		if(rGuaListInStr.NotEmpty()) {
			THROW(rGuaListInStr.Tokenize(",", ss));
			temp_buf.Z();
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				DestGuaList.Add(temp_buf.ToLong());
			}
		}
	}
	else {
		DestGuaList.Set(0);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFilt::Read2(xmlNode * pParentNode)
{
	int    ok = 1;
	SString temp_buf;
	for(xmlNode * p_node = pParentNode->children; p_node; p_node = p_node->next) {
		if(SXml::GetContentByName(p_node, "ID", temp_buf)>0) {
			ID = temp_buf.ToLong();
		}
		else if(SXml::GetContentByName(p_node, "Ver", temp_buf)) {
			Ver = temp_buf.ToLong();
		}
		else if(SXml::GetContentByName(p_node, "ViewID", temp_buf)) {
			ViewID = temp_buf.ToLong();
		}
		else if(SXml::GetContentByName(p_node, "Flags", temp_buf)) {
			Flags = temp_buf.ToLong();
		}
		else if(SXml::GetContentByName(p_node, "Name", temp_buf)) {
			Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::GetContentByName(p_node, "DbSymb", temp_buf)) {
			DbSymb = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::GetContentByName(p_node, "Symb", temp_buf)) {
			Symb = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::GetContentByName(p_node, "ViewSymb", temp_buf)) {
			ViewSymb = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::GetContentByName(p_node, "Param", temp_buf)) {
			STempBuffer bin_buf(temp_buf.Len()*3);
			size_t actual_len = 0;
			temp_buf.DecodeMime64(bin_buf, bin_buf.GetSize(), &actual_len);
			Param.Write(bin_buf, actual_len);
		}
		else if(SXml::GetContentByName(p_node, "DestGuaList", temp_buf)) {
			THROW(ReadGuaListFromStr(temp_buf));
		}
		else {
			if(SXml::IsName(p_node, "VD")) {
				THROW(VD.XmlRead(p_node));
			}
		}
	}
	CATCHZOK
	return ok;
}

SLAPI PPNamedFilt::~PPNamedFilt()
{
	Param.Z();
}

SLAPI PPNamedFiltPool::PPNamedFiltPool(const char * pDbSymb, const int readOnly) : TSCollection <PPNamedFilt>(), DbSymb(pDbSymb), Flags(0)
{
	SETFLAG(Flags, fReadOnly, readOnly);
}

const SString & SLAPI PPNamedFiltPool::GetDbSymb() const
{
	return DbSymb;
}

int SLAPI PPNamedFiltPool::IsNamedFiltSuited(const PPNamedFilt * pNFilt) const
{
	if(DbSymb.Empty() || DbSymb.CmpNC(pNFilt->DbSymb) == 0)
		return 1;
	else
		return PPSetError(PPERR_NFSTRNGFORPOOL);
}

uint SLAPI PPNamedFiltPool::GetCount() const
{
	uint   c = 0;
	if(DbSymb.NotEmpty()) {
		for(uint i = 0; i < getCount(); i++)
			if(DbSymb.CmpNC(at(i)->DbSymb) == 0)
				c++;
	}
	else
		c = getCount();
	return c;
}

int SLAPI PPNamedFiltPool::CheckUniqueNamedFilt(const PPNamedFilt * pNFilt) const
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		const PPNamedFilt * p_nfilt = at(i);
		if(p_nfilt->ID != pNFilt->ID) {
			THROW_PP_S(p_nfilt->Name.CmpNC(pNFilt->Name) != 0, PPERR_DUPNFNAME, p_nfilt->Name);
			if(pNFilt->Symb[0])
				THROW_PP_S(stricmp(p_nfilt->Symb, pNFilt->Symb) != 0, PPERR_DUPNFSYMB, p_nfilt->Symb);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFiltPool::Enum(PPID * pID, PPNamedFilt * pNFilt, int ignoreDbSymb) const
{
	for(uint i = 0; i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if((ignoreDbSymb || IsNamedFiltSuited(p_nfilt)) && (p_nfilt->ID > *pID)) {
			ASSIGN_PTR(pNFilt, *p_nfilt);
			(*pID) = p_nfilt->ID;
			return 1;
		}
	}
	return 0;
}

const PPNamedFilt * SLAPI PPNamedFiltPool::GetByID(PPID namedFiltID, int ignoreDbSymb) const
{
	const PPNamedFilt * p_filt = 0;
	for(uint i = 0; !p_filt && i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if((ignoreDbSymb || IsNamedFiltSuited(p_nfilt)) && p_nfilt->ID == namedFiltID)
			p_filt = p_nfilt;
	}
	return p_filt;
}

const  PPNamedFilt * SLAPI PPNamedFiltPool::GetBySymb(const char * pSymb, int ignoreDbSymb) const
{
	const PPNamedFilt * p_filt = 0;
	for(uint i = 0; !p_filt && i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if((ignoreDbSymb || IsNamedFiltSuited(p_nfilt)) && p_nfilt->Symb.CmpNC(pSymb) == 0)
			p_filt = p_nfilt;
	}
	if(!p_filt)
		PPSetError(PPERR_NAMEDFILTNFOUND, pSymb);
	return p_filt;
}

int SLAPI PPNamedFiltPool::PutNamedFilt(PPID * pNamedFiltID, const PPNamedFilt * pNFilt)
{
	int    ok = -1;
	PPID   max_id = 0;
	THROW_PP((Flags & fReadOnly) == 0, PPERR_NFPOOLISREADONLY);
	THROW(!pNFilt || IsNamedFiltSuited(pNFilt));
	for(uint i = 0; i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if(p_nfilt->ID == *pNamedFiltID) {
			THROW(!pNFilt || IsNamedFiltSuited(p_nfilt));
			if(pNFilt)
				*p_nfilt = *pNFilt;
			else
				atFree(i);
			ok = 1;
		}
		else if(p_nfilt->ID > max_id)
			max_id = p_nfilt->ID;
	}
	if (ok < 0 && pNFilt) {
		PPNamedFilt * p_nfilt = CreateNewItem();
		THROW_SL(p_nfilt);
		*p_nfilt = *pNFilt;
		p_nfilt->ID = max_id + 1;
		ASSIGN_PTR(pNamedFiltID, p_nfilt->ID);
	}
	CATCHZOK
	return ok;
}

SLAPI PPNamedFiltMngr::PPNamedFiltMngr() : LastLoading(ZERODATETIME)
{
	SString name;
	P_Rez = new TVRez(makeExecPathFileName("pp", "res", name), 1);
	PPGetFilePath(PPPATH_BIN, PPFILNAM_NFPOOL, FilePath);
	//@erik v10.7.5
	GetXmlPoolDir(XmlFilePath);
	XmlFilePath.SetLastSlash().Cat("namedfiltpool").Cat(".xml");
	// } @erik
}

SLAPI PPNamedFiltMngr::~PPNamedFiltMngr()
{
	delete P_Rez;
}

int SLAPI PPNamedFiltMngr::LoadResource(PPID viewID, SString & rSymb, SString & rText, long * pFlags) const
{
	int    ok = 1;
	long   flags;
	if(P_Rez) {
		THROW_PP(P_Rez->findResource((uint)viewID, PP_RCDECLVIEW), PPERR_RESFAULT);
		P_Rez->getString(rSymb, 2); /*0 - 866, 1 - w_char, 2 - 1251*/
		P_Rez->getString(rText, 2);
		flags = (long)P_Rez->getUINT();
		pFlags = &flags;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFiltMngr::GetResourceLists(StrAssocArray * pSymbList, StrAssocArray * pTextList) const
{
	int    ok = 1;
	SString text;
	SString symb;
	long flags;
	pSymbList->Z();
	pTextList->Z();
	if(P_Rez) {
		ulong pos = 0;
		for(uint rsc_id = 0; P_Rez->enumResources(PP_RCDECLVIEW, &rsc_id, &pos) > 0;) {
			if(!oneof4(rsc_id, PPVIEW_GOODSGROUP, PPVIEW_REGISTER, PPVIEW_TAG, PPVIEW_BBOARD)) { // Исключаем фиктивные PPView
				THROW(LoadResource(rsc_id, symb, text, &flags));
				THROW_SL(pSymbList->Add(rsc_id, symb));
				PPExpandString(text, CTRANSF_UTF8_TO_INNER); // @v10.1.6
				THROW_SL(pTextList->Add(rsc_id, text));
			}
		}
		pTextList->SortByText();
	}
	CATCHZOK
	return ok;
}

#define NFSTRGSIGN 'SFPP'

struct NamedFiltStrgHeader { // @persistent @size=64
	long   Signature;        // const=NFSTRGSIGN
	long   Reserve1[2];
	uint32 Count;
	char   Reserve2[48];
};

int SLAPI PPNamedFiltMngr::LoadPool(const char * pDbSymb, PPNamedFiltPool * pPool, int readOnly)
{
	int    ok = 1;
	pPool->Flags &= ~PPNamedFiltPool::fReadOnly;
	pPool->freeAll();
	if(fileExists(FilePath)) {
		SFile f;
		NamedFiltStrgHeader hdr;
		THROW_SL(f.Open(FilePath, SFile::mRead | SFile::mBinary));
		THROW_SL(f.Read(&hdr, sizeof(hdr)));
		THROW_PP(hdr.Signature == NFSTRGSIGN, PPERR_NFSTRGCORRUPTED);
		for(uint i = 0; i < hdr.Count; i++) {
			PPID   id = 0;
			PPNamedFilt nf;
			SBuffer buf;
			THROW_SL(f.Read(buf));
			THROW(nf.Read(buf, 0));
			id = nf.ID;
			THROW(pPool->PutNamedFilt(&id, &nf));
		}
	}
	else
		ok = (PPSetErrorSLib(), -1);
	LastLoading = getcurdatetime_();
	pPool->DbSymb = pDbSymb;
	CATCHZOK
	SETFLAG(pPool->Flags, PPNamedFiltPool::fReadOnly, readOnly);
	return ok;
}

int SLAPI PPNamedFiltMngr::SavePool(const PPNamedFiltPool * pPool) const
{
	int    ok = 1;
	SFile  f;
	SBuffer buf;
	NamedFiltStrgHeader hdr;
	THROW_SL(f.Open(FilePath, SFile::mWrite | SFile::mBinary));
	MEMSZERO(hdr);
	hdr.Signature = NFSTRGSIGN;
	hdr.Count = pPool->getCount();
	THROW_SL(f.Write(&hdr, sizeof(hdr)));
	for(uint i = 0; i < hdr.Count; i++) {
		buf.Z();
		PPNamedFilt * p_nfilt = pPool->at(i);
		THROW(p_nfilt->Write(buf, 0));
		THROW_SL(f.Write(buf));
	}
	CATCHZOK
	return ok;
}

int PPNamedFiltMngr::GetXmlPoolDir(SString &rXmlPoolPath)
{
	int ok = 1;
	THROW(PPGetFilePath(PPPATH_WORKSPACE, "namedfilt", rXmlPoolPath));  // получаем путь к workspace
	THROW(::createDir(rXmlPoolPath));
	CATCHZOK
		return ok;
}

int SLAPI PPNamedFiltMngr::LoadPool2(const char * pDbSymb, PPNamedFiltPool * pPool, int readOnly) //@erik v10.7.4
{
	int    ok = 1;
	pPool->Flags &= ~PPNamedFiltPool::fReadOnly;
	pPool->freeAll();
	if(!fileExists(XmlFilePath)) {
		THROW(ConvertBinToXml());
	}
	xmlDoc  * p_doc = 0;
	xmlParserCtxt * p_xml_parser = xmlNewParserCtxt();
	assert(p_xml_parser);
	THROW(p_xml_parser);
	THROW_SL(fileExists(XmlFilePath));
	THROW_LXML((p_doc = xmlCtxtReadFile(p_xml_parser, XmlFilePath, 0, XML_PARSE_NOENT)), p_xml_parser) 
	for(xmlNode * p_root = p_doc->children; p_root; p_root = p_root->next) {
		if(SXml::IsName(p_root, "NamedFiltPool")) {
			for(xmlNode * p_node = p_root->children; p_node; p_node = p_node->next) {
				if(SXml::IsName(p_node, "NamedFilt")) {
					PPID   id = 0;
					PPNamedFilt nf;
					THROW(nf.Read2(p_node));
					id = nf.ID;
					THROW(pPool->PutNamedFilt(&id, &nf));
				}
			}
		}
	}
	LastLoading = getcurdatetime_();
	pPool->DbSymb = pDbSymb;
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_xml_parser);
	SETFLAG(pPool->Flags, PPNamedFiltPool::fReadOnly, readOnly);
	return ok;
}

int SLAPI PPNamedFiltMngr::ConvertBinToXml()
{
	int ok = 1;
	PPNamedFiltPool pool(0, 0);
	THROW(LoadPool(0, &pool, 1));
	THROW(SavePool2(&pool));
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFiltMngr::SavePool2(const PPNamedFiltPool * pPool) const //@erik v10.7.4
{
	int    ok = 1;
	SString temp_buf;
	xmlTextWriter * p_xml_writer = xmlNewTextWriterFilename(XmlFilePath, 0);
	assert(p_xml_writer);
	THROW(p_xml_writer);
	xmlTextWriterSetIndent(p_xml_writer, 1);
	xmlTextWriterSetIndentString(p_xml_writer, reinterpret_cast<const xmlChar *>("\t"));
	{
		SXml::WDoc _doc(p_xml_writer, cpUTF8);
		uint count = pPool->getCount();
		SXml::WNode root_node(p_xml_writer, "NamedFiltPool");
		{
			SXml::WNode xml_job_pool_hdr(p_xml_writer, "NamedFiltStrgHeader");
			xml_job_pool_hdr.PutInner("Count", temp_buf.Z().Cat(count));
		}
		for(uint i = 0; i < count; i++) {
			PPNamedFilt * p_nfilt = pPool->at(i);
			THROW(p_nfilt->Write2(p_xml_writer));
		}
	}
	CATCHZOK
	xmlFreeTextWriter(p_xml_writer);
	return ok;
}

//
// Descr: Отвечает за диалог "Список фильтров"
//
class FiltPoolDialog : public PPListDialog {
public:
	FiltPoolDialog(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pData) : PPListDialog(DLG_FILTPOOL, CTL_FILTPOOL_LIST), P_Mngr(pMngr), P_Data(pData)
	{
		CALLPTRMEMB(P_Mngr, GetResourceLists(&CmdSymbList, &CmdTextList));
		updateList(-1);
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	PPNamedFiltMngr * P_Mngr; // @notowned
	PPNamedFiltPool * P_Data; // @notowned
	StrAssocArray CmdSymbList;
	StrAssocArray CmdTextList;
};

//
// Descr: Обновляет таблицу фильтров в диалоге "Список фильтров"
//
int FiltPoolDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	PPNamedFilt  nfilt;
	StringSet ss(SLBColumnDelim);
	for(PPID id = 0; ok && P_Data->Enum(&id, &nfilt, 0/*1 - ForAllDb*/);) {
		ss.clear();
		ss.add(temp_buf.Z().Cat(nfilt.ID));
		ss.add(nfilt.Name);
		ss.add(nfilt.Symb);
		uint    pos = 0;
		temp_buf.Z();
		if(CmdSymbList.SearchByText(nfilt.ViewSymb, 1, &pos)) {
			StrAssocArray::Item symb_item = CmdSymbList.at_WithoutParent(pos);
			uint  tpos = 0;
			if(CmdTextList.Search(symb_item.Id, &tpos))
				temp_buf = CmdTextList.at_WithoutParent(tpos).Txt;
		}
		ss.add(temp_buf);
		if(!addStringToList(id, ss.getBuf()))
			ok = 0;
	}
	return ok;
}
//
// Descr: Создает и отображает диалог "Список фильтров"
//
int SLAPI ViewFiltPool()
{
	int    ok = -1;
	PPNamedFiltMngr mngr;
	PPNamedFiltPool pool(0, 0);
	SString db_symb;
	FiltPoolDialog * dlg = 0;
	THROW_PP(CurDict->GetDbSymb(db_symb) > 0, PPERR_DBSYMBUNDEF);
	//THROW(mngr.LoadPool(db_symb, &pool, 0)); //@erik v10.7.5
	THROW(mngr.LoadPool2(db_symb, &pool, 0));//@erik v10.7.5
	THROW(CheckDialogPtrErr(&(dlg = new FiltPoolDialog(&mngr, &pool))));
	while(ExecView(dlg) == cmOK) {
		//if(mngr.SavePool(&pool)) { //@erik v10.7.5
		if(mngr.SavePool2(&pool)) {  //@erik v10.7.5
			ok = 1;
			break;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

//
// Descr: Класс, отвечающий за диалог "Создать фильтр"
//
class FiltItemDialog : public TDialog {
	DECL_DIALOG_DATA(PPNamedFilt);
public:
	FiltItemDialog(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pPool) : TDialog(DLG_FILTITEM), P_Mngr(pMngr), P_Pool(pPool)
	{
		CALLPTRMEMB(P_Mngr, GetResourceLists(&CmdSymbList, &CmdTextList));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		PPID   view_id = 0;                            // Идентификатор обьекта PPView
		RVALUEPTR(Data, pData);
		setCtrlString(CTL_FILTITEM_NAME, Data.Name); // Поле "Наименование"
		setCtrlString(CTL_FILTITEM_SYMB, Data.Symb); // Поле "Символ"
		setCtrlLong(CTL_FILTITEM_ID, Data.ID);       // Поле "ID"
		disableCtrl(CTL_FILTITEM_ID, 1);             // Идентификатор только отображаетс
		view_id = Data.ViewID; // Может быть и ноль, если это создание (а не редактирование) именованного фильтра
		//
		// Инициализировать комбобокс:
		//   отсортированный по описанию список {id PPView, описание PPView}
		//   активным элементом сделать элемент с идентификатором view_id
		//
		SetupStrAssocCombo(this, CTLSEL_FILTITEM_CMD, &CmdTextList, view_id, 0);
		AddClusterAssoc(CTL_FILTITEM_FLAGS, 0, PPNamedFilt::fDontWriteXmlDTD);
		AddClusterAssoc(CTL_FILTITEM_FLAGS, 1, PPNamedFilt::fCompressXml); // @v10.6.0
		SetClusterData(CTL_FILTITEM_FLAGS, Data.Flags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		PPID   view_id = 0; // Идентификатор обьекта PPView
		uint   sel = 0;     // Идентификатор управляющего элемента, данные из которого анализировались в момент ошибки
		uint pos;
		getCtrlString(sel = CTL_FILTITEM_NAME, Data.Name);
		THROW_PP(Data.Name.NotEmptyS(), PPERR_NFILTNAMENEEDED);
		getCtrlString(sel = CTL_FILTITEM_SYMB, Data.Symb);
		THROW_PP(stricmp(Data.Symb, "") != 0, PPERR_SYMBNEEDED);
		THROW(P_Pool->CheckUniqueNamedFilt(&Data));
		getCtrlData(sel = CTLSEL_FILTITEM_CMD, &view_id);
		THROW_PP(CmdSymbList.Search(view_id, &pos), PPERR_INVNFCMD);
		GetClusterData(CTL_FILTITEM_FLAGS, &Data.Flags);
		Data.ViewSymb = CmdSymbList.Get(pos).Txt;
		Data.ViewID = view_id;
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG // Вывести сообщение о ошибке и активировать породивший его управляющий элемент
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    ChangeBaseFilter();    // Обрабатывает изменение базового фильтра
	int    ViewMobColumnList(); //@erik 10.5.0
	StrAssocArray CmdSymbList; // Список ассоциаций для обьектов PPView {id, символ}
	StrAssocArray CmdTextList; // Список ассоциаций для обьектов PPView {id, описание} (упорядоченный по возрастанию)
	PPNamedFiltPool * P_Pool;  // @notowned
	PPNamedFiltMngr * P_Mngr;  // @notowned
};
//
// Descr: Отображает диалог редактирования именованного фильтра
//
int SLAPI EditFiltItem(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pNFiltPool, PPNamedFilt * pData)
{
	DIALOG_PROC_BODY_P2(FiltItemDialog, pMngr, pNFiltPool, pData);
}

//
// Descr: Обрабатывает события диалога "Создать фильтр"
//
IMPL_HANDLE_EVENT(FiltItemDialog)
{
	TDialog::handleEvent(event);
	uint pos;
	// Событие: выбор элемента в комбобоксе
	if(event.isCbSelected(CTLSEL_FILTITEM_CMD)) {
		PPID view_id = getCtrlLong(CTLSEL_FILTITEM_CMD);
		if(view_id && CmdTextList.Search(view_id, &pos)) {
			// Если пользователь еще не ввел название, возможно он захочет совпадающее с именем PPView
			SString name;
			getCtrlString(CTL_FILTITEM_NAME, name);
			if(!name.NotEmptyS()) {
				setCtrlString(CTL_FILTITEM_NAME, CmdTextList.Get(pos).Txt);
			}
			Data.Param.Z(); // Поменялся PPView, фильтр устарел
			enableCommand(cmCmdParam, 1);
		}
		else
			enableCommand(cmCmdParam, 0);
	}
	// Событие: нажатие кнопки "Фильтр.."
	else if(event.isCmd(cmCmdParam)) {
		ChangeBaseFilter();
	}
	else if(event.isCmd(cmOutFields)) {
		ViewMobColumnList();
	}
	// @v10.5.3 {
	else if(event.isCmd(cmGuaList)) {
		PPIDArray id_list;
		Data.DestGuaList.Get(id_list);
		ListToListData ltld(PPOBJ_GLOBALUSERACC, 0, &id_list);
		ltld.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&ltld) > 0)
			Data.DestGuaList.Set(&id_list);
	}
	// } @v10.5.3 
	clearEvent(event);
}

int FiltItemDialog::ChangeBaseFilter()
{
	int    ok = 1;
	size_t sav_offs = 0;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	PPID   view_id = getCtrlLong(CTLSEL_FILTITEM_CMD);
	if(view_id && CmdSymbList.Search(view_id)) {
		sav_offs = Data.Param.GetRdOffs();
		THROW(PPView::CreateInstance(view_id, &p_view) > 0);
		{
			if(Data.Param.GetAvailableSize()) {
				THROW(PPView::ReadFiltPtr(Data.Param, &p_filt));
			}
			SETIFZ(p_filt, p_view->CreateFilt(0));
			if((ok = p_view->EditBaseFilt(p_filt)) > 0) {
				Data.Param.Z();
				THROW(p_view->WriteFiltPtr(Data.Param, p_filt));
			}
			else
				Data.Param.SetRdOffs(sav_offs);
		}

	}
	CATCH
		Data.Param.SetRdOffs(sav_offs);
	ENDCATCH
	ZDELETE(p_filt);
	ZDELETE(p_view);
	return ok;
}
//
// Descr: Обрабатывает нажатие кнопки "Добавить.."
//
int FiltPoolDialog::addItem(long * pPos, long * pID)
{
	int ok = -1;
	PPNamedFilt  nfilt;
	nfilt.DbSymb = P_Data->GetDbSymb();
	if(EditFiltItem(P_Mngr, P_Data, &nfilt) > 0) {
		PPID   id = 0;
		THROW(P_Data->PutNamedFilt(&id, &nfilt));
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	CATCHZOKPPERR
		return ok;
}
//
// Descr: Обрабатывает редактирование именованного фильтра
//
int FiltPoolDialog::editItem(long pos, long id)
{
	int    ok = -1;
	const  PPNamedFilt * p_nfilt = P_Data->GetByID(id, 1);
	if(p_nfilt) {
		PPNamedFilt nfilt = *p_nfilt;
		// Вызываем диалог редактировани
		if(EditFiltItem(P_Mngr, P_Data, &nfilt) > 0) {
			// OK - сохраняем данные
			ok = P_Data->PutNamedFilt(&id, &nfilt);
		}
		if(!ok)
			PPError();
	}
	else
		ok = PPError();
	return ok;
}
//
// Descr: Обрабатывает удаление именованного фильтра из списка
//
int FiltPoolDialog::delItem(long pos, long id)
{
	return P_Data->PutNamedFilt(&id, 0);
}

PPNamedFilt::ViewDefinition::Entry::Entry() : TotalFunc(0)
{
}

PPNamedFilt::ViewDefinition::Entry & PPNamedFilt::ViewDefinition::Entry::Z()
{
	Zone.Z();
	FieldName.Z();
	Text.Z();
	TotalFunc = 0;
	return *this;
}

PPNamedFilt::ViewDefinition::ViewDefinition()
{
}

//@erik v10.5.0
uint PPNamedFilt::ViewDefinition::GetCount() const
{
	return L.getCount();
}

const SString & PPNamedFilt::ViewDefinition::GetStrucSymb() const
{
	return StrucSymb;
}

int PPNamedFilt::ViewDefinition::SetStrucSymb(const char * pSymb)
{
	(StrucSymb = pSymb).Strip();
	return 1;
}

int PPNamedFilt::ViewDefinition::Swap(uint p1, uint p2)
{
	return L.swap(p1, p2);
}

int PPNamedFilt::ViewDefinition::SearchEntry(const char * pZone, const char *pFieldName, uint * pPos, InnerEntry * pInnerEntry) const
{
	int    ok = 0;
	SString temp_buf;
	for(uint i = 0; !ok && i < L.getCount(); i++) {
		const InnerEntry & r_inner_entry = L.at(i);
		GetS(r_inner_entry.ZoneP, temp_buf);
		if(temp_buf.IsEqiAscii(pZone)) {
			GetS(r_inner_entry.FieldNameP, temp_buf);
			if(temp_buf.IsEqiAscii(pFieldName)) {
				ok = 1;
				ASSIGN_PTR(pPos, i);
				ASSIGN_PTR(pInnerEntry, r_inner_entry);
			}
		}
	}
	return ok;
}

int PPNamedFilt::ViewDefinition::SetEntry(const Entry & rE)
{
	int    ok = 0;
	uint   pos = 0;
	InnerEntry new_entry;
	if(SearchEntry(rE.Zone, rE.FieldName, &pos, 0)) {
		AddS(rE.Zone, &new_entry.ZoneP);
		AddS(rE.FieldName, &new_entry.FieldNameP);
		AddS(rE.Text, &new_entry.TextP);
		new_entry.TotalFunc = rE.TotalFunc;
		L.at(pos) = new_entry;
		ok = 1;
	}
	else {
		AddS(rE.Zone, &new_entry.ZoneP);
		AddS(rE.FieldName, &new_entry.FieldNameP);
		AddS(rE.Text, &new_entry.TextP);
		new_entry.TotalFunc = rE.TotalFunc;
		L.insert(&new_entry);
		ok = 2;
	}
	return ok;
}

int PPNamedFilt::ViewDefinition::GetEntry(const uint pos, Entry & rE) const
{
	rE.Z();
	int ok = 0;
	if(pos < L.getCount()) {
		const InnerEntry & r_entry = L.at(pos);
		GetS(r_entry.ZoneP, rE.Zone);
		GetS(r_entry.FieldNameP, rE.FieldName);
		GetS(r_entry.TextP, rE.Text);
		rE.TotalFunc = r_entry.TotalFunc;
		ok = 1;
	}
	return ok;
}

int PPNamedFilt::ViewDefinition::RemoveEntryByPos(uint pos)
{
	int    ok = 0;
	if(pos < GetCount() && L.atFree(pos))
		ok = 1;
	return ok;
}

int PPNamedFilt::ViewDefinition::XmlWriter(void * param)
{
	int ok = 1;
	return ok;
}

int PPNamedFilt::ViewDefinition::XmlWrite(xmlTextWriter * pXmlWriter) 
{
	int ok = 1;
	SString temp_buf;
	SBuffer buf;
	assert(pXmlWriter);
	THROW(pXmlWriter);
	{
		SXml::WNode pp_vd_node(pXmlWriter, "VD");
		for(uint i = 0; i<GetCount(); i++) {
			Entry tmp_entry;
			if(GetEntry(i, tmp_entry)) {
				SXml::WNode pp_entry_node(pXmlWriter, "Entry");
				XMLReplaceSpecSymb(temp_buf.Z().Cat(tmp_entry.Zone), "&<>\'");
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				pp_entry_node.PutInner("Zone", temp_buf);
				XMLReplaceSpecSymb(temp_buf.Z().Cat(tmp_entry.FieldName), "&<>\'");
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				pp_entry_node.PutInner("FieldName", temp_buf);
				XMLReplaceSpecSymb(temp_buf.Z().Cat(tmp_entry.Text), "&<>\'");
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				pp_entry_node.PutInner("Description", temp_buf);
				pp_entry_node.PutInner("TotalFunc", temp_buf.Z().Cat(tmp_entry.TotalFunc));
			}
		}
	} 
	CATCHZOK;
	return ok;
}

int PPNamedFilt::ViewDefinition::XmlRead(xmlNode * pParentNode) 
{
	int ok = 1;
	SString temp_buf;
	for(xmlNode * p_node = pParentNode->children; p_node; p_node = p_node->next) {
		if(SXml::IsName(p_node, "Entry")) {
			Entry tmp_entry;
			for(xmlNode * p_entry_node = p_node->children; p_entry_node; p_entry_node = p_entry_node->next) {
				if(SXml::GetContentByName(p_entry_node, "Zone", temp_buf)){
					tmp_entry.Zone = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(SXml::GetContentByName(p_entry_node, "FieldName", temp_buf)) {
					tmp_entry.FieldName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(SXml::GetContentByName(p_entry_node, "Description", temp_buf)) {
					tmp_entry.Text = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(SXml::GetContentByName(p_entry_node, "TotalFunc", temp_buf)) {
					tmp_entry.TotalFunc = temp_buf.ToLong();
				}
			}	
			SetEntry(tmp_entry);
		}
	}
	return ok;
} 

/* @v10.6.7 int PPNamedFilt::ViewDefinition::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, &L, rBuf));
	THROW_SL(SStrGroup::SerializeS(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}*/

class MobileClmnValListDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPNamedFilt::ViewDefinition);
	enum {
		dummyFirst = 1,
		brushValidStrucSymb,
		brushInvalidStrucSymb
	};
public:
	MobileClmnValListDialog() : PPListDialog(DLG_MOBCLMNN, CTL_MOBCLMNN_LIST), P_Dl600Scope(0)
	{
		Dl600Ctx.InitSpecial(DlContext::ispcExpData);
		Ptb.SetBrush(brushValidStrucSymb,   SPaintObj::bsSolid, LightenColor(GetColorRef(SClrGreen), 0.8f), 0);
		Ptb.SetBrush(brushInvalidStrucSymb, SPaintObj::bsSolid, LightenColor(GetColorRef(SClrRed), 0.8f), 0);
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		PPNamedFilt::ViewDefinition::Entry mobTypeClmn;
		StringSet ss(SLBColumnDelim);
		setCtrlString(CTL_MOBCLMNN_STRUC, Data.GetStrucSymb()); // @v10.6.7 
		if(Data.GetStrucSymb().NotEmpty()) {
			P_Dl600Scope = Dl600Ctx.GetScopeByName_Const(DlScope::kExpData, Data.GetStrucSymb());
		}

		for(uint i = 0; ok && i<Data.GetCount(); i++) {
			const PPID id = static_cast<PPID>(i+1);
			Data.GetEntry(i, mobTypeClmn);
			ss.clear();
			((ss += mobTypeClmn.Zone) += mobTypeClmn.FieldName) += mobTypeClmn.Text;
			if(!addStringToList(id, ss.getBuf()))
				ok = 0;
		}
		updateList(1, 1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		// @v10.6.7 {
		SString temp_buf;
		getCtrlString(CTL_MOBCLMNN_STRUC, temp_buf);
		Data.SetStrucSymb(temp_buf);
		// } @v10.6.7
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_MOBCLMNN_STRUC)) {
				SString temp_buf;
				getCtrlString(CTL_MOBCLMNN_STRUC, temp_buf);
				P_Dl600Scope = Dl600Ctx.GetScopeByName_Const(DlScope::kExpData, temp_buf.Strip());
				clearEvent(event);
			}
		}
		else if(event.isCmd(cmCtlColor)) {
			TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
			if(p_dc) {
				if(p_dc->H_Ctl == getCtrlHandle(CTL_MOBCLMNN_STRUC)) {
					TCanvas canv(p_dc->H_DC);
					::SetBkMode(p_dc->H_DC, TRANSPARENT);
					if(P_Dl600Scope) {
						//canv.SetTextColor(GetColorRef(SClrWhite));
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushValidStrucSymb));
						clearEvent(event);
					}
					else {
						SString temp_buf;
						getCtrlString(CTL_MOBCLMNN_STRUC, temp_buf);
						if(temp_buf.NotEmpty()) {
							//canv.SetTextColor(GetColorRef(SClrWhite));
							p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushInvalidStrucSymb));
							clearEvent(event);
						}
					}
				}
			}
		}
		else
			return;
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int moveItem(long pos, long id, int up)
	{
		int    ok = 1;
		if(up && pos > 0)
			Data.Swap(pos, pos-1);
		else if(!up && pos < (long)(Data.GetCount()-1))
			Data.Swap(pos, pos+1);
		else
			ok = -1;
		return ok;
	}
	DlContext Dl600Ctx;
	const DlScope * P_Dl600Scope;
	SPaintToolBox Ptb;
};
//
// Descr: Создает и отображает диалог "Список"
//
int FiltItemDialog::ViewMobColumnList()
{
	DIALOG_PROC_BODY(MobileClmnValListDialog, &Data.VD);
}
//
// Descr: Класс, отвечающий за диалог "добавить элемент"
//
class MobileClmnValItemDialog : public TDialog {
	DECL_DIALOG_DATA(PPNamedFilt::ViewDefinition::Entry);
	const DlScope * P_Dl600Scope;
public:
	MobileClmnValItemDialog(PPNamedFilt::ViewDefinition * pViewDef, const DlScope * pScope) : TDialog(DLG_MOBCLEDT), P_Dl600Scope(pScope)/*, P_Data(pViewDef)*/
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//PPNamedFilt::ViewDefinition::Entry entry = *pEntry;
		setCtrlString(CTL_MOBCLEDT_Z, Data.Zone); // Поле "Zone"
		setCtrlString(CTL_MOBCLEDT_FN, Data.FieldName); // Поле "FieldName"
		setCtrlString(CTL_MOBCLEDT_TXT, Data.Text); // Поле "Text"
		SetupStringCombo(this, CTLSEL_MOBCLEDT_AGGR, PPTXT_AGGRFUNCNAMELIST, Data.TotalFunc);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;     // Идентификатор управляющего элемента, данные из которого анализировались в момент ошибки
		//PPNamedFilt::ViewDefinition::Entry entry;
		getCtrlString(sel = CTL_MOBCLEDT_Z, Data.Zone);
		THROW_PP(Data.Zone.NotEmptyS(), PPERR_USERINPUT);
		getCtrlString(sel = CTL_MOBCLEDT_FN, Data.FieldName);
		THROW_PP(Data.FieldName.NotEmptyS(), PPERR_USERINPUT);
		getCtrlString(sel = CTL_MOBCLEDT_TXT, Data.Text);
		THROW_PP(Data.Text.NotEmptyS(), PPERR_USERINPUT);
		getCtrlData(CTLSEL_MOBCLEDT_AGGR, &Data.TotalFunc);
		ASSIGN_PTR(pData, Data);
		CATCH
			PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isKeyDown(kbF2)) {
			if(GetCurrId() == CTL_MOBCLEDT_Z)
				SelectZone();
			else if(GetCurrId() == CTL_MOBCLEDT_FN)
				SelectField();
			else
				return;
		}
		else if(event.isCmd(cmSelectDlZone)) {
			SelectZone();
		}
		else if(event.isCmd(cmSelectDlField)) {
			SelectField();
		}
		else
			return;
		clearEvent(event);
	}
	SString & TranslateDlZoneNameToXmlName(SString & rBuf) const
	{
		if(rBuf.IsEqiAscii("iter@def"))
			rBuf = "Iter";
		else if(rBuf.IsEqiAscii("hdr"))
			rBuf = "Head";
		return rBuf;
	}
	void SelectZone()
	{
		if(P_Dl600Scope) {
			SString temp_buf;
			SString cur_text;
			const uint ctl_id = CTL_MOBCLEDT_Z;
			const DlScopeList & r_cl = P_Dl600Scope->GetChildList();
			if(r_cl.getCount()) {
				StrAssocArray * p_list = new StrAssocArray;
				if(p_list) {
					long id = 0;
					getCtrlString(ctl_id, cur_text);
					{
						for(uint i = 0; i < r_cl.getCount(); i++) {
							const DlScope * p_scope = r_cl.at(i);
							if(p_scope) {
								TranslateDlZoneNameToXmlName(temp_buf = p_scope->GetName());
								if(temp_buf.IsEqiAscii(cur_text))
									id = static_cast<long>(p_scope->GetId());
								p_list->Add(p_scope->GetId(), temp_buf);
							}
						}
					}
					p_list->SortByText();
					if(ListBoxSelDialog(p_list, "@ttl_seldl600zone", &id, 0) > 0 && id > 0) {
						for(uint i = 0; i < r_cl.getCount(); i++) {
							const DlScope * p_scope = r_cl.at(i);
							if(p_scope->GetId() == static_cast<DLSYMBID>(id)) {
								TranslateDlZoneNameToXmlName(temp_buf = p_scope->GetName());
								setCtrlString(ctl_id, temp_buf);
								break;
							}
						}
					}
				}
			}
		}
	}
	void SelectField()
	{
		if(P_Dl600Scope) {
			SString temp_buf;
			SString cur_text;
			SString zone_text;
			const uint ctl_id = CTL_MOBCLEDT_FN;
			const DlScopeList & r_cl = P_Dl600Scope->GetChildList();
			getCtrlString(CTL_MOBCLEDT_Z, zone_text);
			if(r_cl.getCount() && zone_text.NotEmpty()) {
				getCtrlString(CTL_MOBCLEDT_FN, cur_text);
				DLSYMBID zone_id = 0;
				const DlScope * p_zone_scope = 0;
				{
					for(uint i = 0; !zone_id && i < r_cl.getCount(); i++) {
						const DlScope * p_scope = r_cl.at(i);
						if(p_scope) {
							TranslateDlZoneNameToXmlName(temp_buf = p_scope->GetName());
							if(temp_buf.IsEqiAscii(zone_text)) {
								p_zone_scope = p_scope;
								zone_id = static_cast<long>(p_scope->GetId());
							}
						}
					}
				}
				if(p_zone_scope) {
					StrAssocArray * p_list = new StrAssocArray;
					if(p_list) {
						long   id = 0;
						SdbField fld;
						{
							for(uint i = 0; i < p_zone_scope->GetCount(); i++) {
								if(p_zone_scope->GetFieldByPos(i, &fld)) {
									p_list->Add(fld.ID, fld.Name);
									if(fld.Name.IsEqiAscii(cur_text))
										id = fld.ID;
								}
							}
						}
						p_list->SortByText();
						if(ListBoxSelDialog(p_list, "@ttl_seldl600field", &id, 0) > 0 && id > 0) {
							if(p_zone_scope->GetFieldByID(id, 0, &fld)) {
								setCtrlString(CTL_MOBCLEDT_FN, fld.Name);
								getCtrlString(CTL_MOBCLEDT_TXT, temp_buf);
								if(temp_buf.Empty())
									setCtrlString(CTL_MOBCLEDT_TXT, fld.Name);
							}
						}
					}
				}
			}
		}
	}
};
//
// Descr: Отображает диалог редактирования 
//
static int SLAPI EditMobTypeClmn(PPNamedFilt::ViewDefinition * pViewDef, const DlScope * pScope, PPNamedFilt::ViewDefinition::Entry * pEntry)
{
	DIALOG_PROC_BODY_P2(MobileClmnValItemDialog, pViewDef, pScope, pEntry);
}
//
// Descr: загрузка и перезагрузка списка всех отображаемых полей
//
int MobileClmnValListDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	PPNamedFilt::ViewDefinition::Entry entry;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; ok && i < Data.GetCount(); i++) {
		PPID id = static_cast<PPID>(i + 1);
		Data.GetEntry(i, entry);
		ss.clear();
		ss.add(entry.Zone);
		ss.add(entry.FieldName);
		ss.add(entry.Text);
		const char * p_aggrfunc_sign = 0;
		switch(entry.TotalFunc) {
			case AGGRFUNC_COUNT:  p_aggrfunc_sign = "aggrfunc_count"; break;
			case AGGRFUNC_SUM:    p_aggrfunc_sign = "aggrfunc_sum"; break;
			case AGGRFUNC_AVG:    p_aggrfunc_sign = "aggrfunc_average"; break;
			case AGGRFUNC_MIN:    p_aggrfunc_sign = "aggrfunc_min"; break;
			case AGGRFUNC_MAX:    p_aggrfunc_sign = "aggrfunc_max"; break;
			case AGGRFUNC_STDDEV: p_aggrfunc_sign = "aggrfunc_stddev"; break;
		}
		if(p_aggrfunc_sign)
			PPLoadString(p_aggrfunc_sign, temp_buf);
		else
			temp_buf.Z();
		ss.add(temp_buf);
		if(!addStringToList(id, ss.getBuf()))
			ok = 0;
	}
	return ok;
}
//
// Descr: Обрабатывает нажатие кнопки "Добавить.."
//
int MobileClmnValListDialog::addItem(long * pPos, long * pID)
{
	int    ok = 0;
	PPNamedFilt::ViewDefinition::Entry entry;
	if(EditMobTypeClmn(&Data, P_Dl600Scope, &entry) > 0) {
		Data.SetEntry(entry);
		ok = 1;
	}
	return ok;
}
//
// Descr: Обрабатывает редактирование
//
int MobileClmnValListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	PPNamedFilt::ViewDefinition::Entry entry;
	if(Data.GetEntry(pos, entry)) {
		// Вызываем диалог редактировани
		if(EditMobTypeClmn(&Data, P_Dl600Scope, &entry) > 0) {
			// OK - сохраняем данные
			ok = Data.SetEntry(entry);
		}
	}
	else {
		ok = 0;
	}
	return ok;
}
//
// Descr: Обрабатывает удаление
//
int MobileClmnValListDialog::delItem(long pos, long id)
{
	return Data.RemoveEntryByPos(pos);
}
//@erik