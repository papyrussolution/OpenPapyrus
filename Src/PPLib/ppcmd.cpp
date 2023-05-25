// PPCMD.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPCommand)
//
/*static*/const char * PPCommandDescr::P_FactoryPrfx = "CFF_";

PPCommandDescr::PPCommandDescr()
{
	Init();
}

void PPCommandDescr::Init()
{
	CmdID = 0;
	Flags = 0;
	MenuCm = 0;
	IconId = 0;
	ToolbarId = 0;
	ViewId = 0; // @v10.3.8 @fix
	FiltId = 0; // @v10.3.8 @fix
	FiltExtId = 0; // @v10.3.8 @fix
	memzero(&Reserve, sizeof(Reserve));
	Symb.Z();
	Text.Z();
}

SString & FASTCALL PPCommandDescr::GetFactoryFuncName(SString & rBuf) const
{
	(rBuf = P_FactoryPrfx).Cat(Symb).ToUpper();
	return rBuf;
}

int PPCommandDescr::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	THROW_SL(rBuf.Write(CmdID));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(MenuCm));
	THROW_SL(rBuf.Write(IconId));
	THROW_SL(rBuf.Write(ToolbarId));
	THROW_SL(rBuf.Write(ViewId));
	THROW_SL(rBuf.Write(FiltId));
	THROW_SL(rBuf.Write(FiltExtId));
	THROW_SL(rBuf.Write(Reserve));
	THROW_SL(rBuf.Write(Symb));
	THROW_SL(rBuf.Write(Text));
	CATCHZOK
	return ok;
}

int PPCommandDescr::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	THROW_SL(rBuf.Read(CmdID));
	THROW_SL(rBuf.Read(Flags));
	THROW_SL(rBuf.Read(MenuCm));
	THROW_SL(rBuf.Read(IconId));
	THROW_SL(rBuf.Read(ToolbarId));
	THROW_SL(rBuf.Read(ViewId));
	THROW_SL(rBuf.Read(FiltId));
	THROW_SL(rBuf.Read(FiltExtId));
	THROW_SL(rBuf.Read(Reserve));
	THROW_SL(rBuf.Read(Symb));
	THROW_SL(rBuf.Read(Text));
	CATCHZOK
	return ok;
}

int PPCommandDescr::LoadResource(long cmdDescrID)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	Init();
	if(p_rez) {
		THROW_PP(p_rez->findResource((uint)cmdDescrID, PP_RCDECLCMD), PPERR_RESFAULT);
		CmdID = cmdDescrID;
		p_rez->getString(Symb, 2);
		p_rez->getString(Text, 2);
		SLS.ExpandString(Text, CTRANSF_UTF8_TO_INNER);
		IconId    = static_cast<long>(p_rez->getUINT());
		ToolbarId = static_cast<long>(p_rez->getUINT());
		MenuCm    = static_cast<long>(p_rez->getUINT());
		Flags     = static_cast<long>(p_rez->getUINT());
		ViewId    = static_cast<long>(p_rez->getUINT());
		FiltId    = static_cast<long>(p_rez->getUINT());
		FiltExtId = static_cast<long>(p_rez->getUINT());
	}
	CATCHZOK
	return ok;
}

/*static*/int PPCommandDescr::GetResourceList(LAssocArray & rList)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	rList.clear();
	if(p_rez) {
		ulong pos = 0;
		for(uint   rsc_id = 0; p_rez->enumResources(PP_RCDECLCMD, &rsc_id, &pos) > 0;) {
			PPCommandDescr descr;
			THROW(descr.LoadResource(rsc_id));
			THROW_SL(rList.Add(descr.CmdID, descr.MenuCm, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int PPCommandDescr::GetResourceList(int loadText, StrAssocArray & rList)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	rList.Z();
	if(p_rez) {
		ulong pos = 0;
		for(uint   rsc_id = 0; p_rez->enumResources(PP_RCDECLCMD, &rsc_id, &pos) > 0;) {
			PPCommandDescr descr;
			THROW(descr.LoadResource(rsc_id));
			THROW_SL(rList.Add(descr.CmdID, loadText ? descr.Text : descr.Symb));
		}
	}
	CATCHZOK
	return ok;
}

PPCommandHandler * PPCommandDescr::CreateInstance(long cmdDescrID)
{
	PPCommandHandler * p_h = 0;
	SString ffn;
	THROW(LoadResource(cmdDescrID));
	PPSetAddedMsgString(Text);
	FN_CMD_FACTORY f = reinterpret_cast<FN_CMD_FACTORY>(GetProcAddress(SLS.GetHInst(), GetFactoryFuncName(ffn)));
	if(!f && MenuCm) {
		SString def_factory_name;
		def_factory_name = P_FactoryPrfx;
		def_factory_name.Cat("default").ToUpper();
		f = reinterpret_cast<FN_CMD_FACTORY>(GetProcAddress(SLS.GetHInst(), def_factory_name));
	}
	THROW(f);
	THROW(p_h = f(this));
	CATCH
		PPSetError(PPERR_JOBUNIMPL);
		p_h = 0;
	ENDCATCH
	return p_h;
}

int PPCommandDescr::DoCommand(PPCommand * pCmd, void * extraPtr)
{
	int    ok = -1;
	PPCommandHandler * p_h = 0;
	if(pCmd) {
		p_h = CreateInstance(pCmd->CmdID);
		size_t sav_offs = pCmd->Param.GetRdOffs();
		if(p_h) {
			SBuffer * p_param = &pCmd->Param;
			SBuffer temp_param;
			int    r = 1;
			if(pCmd->Flags & PPCommandItem::fAllowEditFilt) {
				temp_param = pCmd->Param;
				r = p_h->EditParam(&temp_param, pCmd->GetID(), extraPtr);
				p_param = &temp_param;
			}
			if(r > 0)
				ok = p_h->Run(p_param, pCmd->GetID(), extraPtr);
		}
		else
			ok = 0;
		pCmd->Param.SetRdOffs(sav_offs);
	}
	delete p_h;
	return ok;
}

int PPCommandDescr::DoCommandSimple(PPID cmdDescrID, int allowEditParam, const char * pTextFilt, void * extraPtr)
{
	int    ok = -1;
	SBuffer * p_filt_buf = 0;
	SBuffer filt_buf;
	PPCommandHandler * p_h = CreateInstance(cmdDescrID);
	THROW(p_h);
	if(pTextFilt) {
		SString temp_buf;
		temp_buf.Cat(PPBaseFilt::P_TextSignature).Space().Cat(pTextFilt);
		THROW_SL(filt_buf.Write(temp_buf.cptr(), temp_buf.Len()+1));
		p_filt_buf = &filt_buf;
	}
	{
		int    r = 1;
		if(allowEditParam) {
			SETIFZ(p_filt_buf, &filt_buf);
			r = p_h->EditParam(p_filt_buf, cmdDescrID, extraPtr);
		}
		if(r > 0)
			ok = p_h->Run(p_filt_buf, 0, extraPtr);
	}
	CATCHZOK
	delete p_h;
	return ok;
}

int PPCommandDescr::EditCommandParam(PPID cmdDescrID, long cmdID, SBuffer * pParam, void * extraPtr)
{
	PPCommandHandler * p_h = CreateInstance(cmdDescrID);
	return p_h ? p_h->EditParam(pParam, cmdID, extraPtr) : 0;
}
//
//
//
PPCommandItem::PPCommandItem(int kind) : Kind(kind), Flags(0), ID(0)
{
}

PPCommandItem::PPCommandItem(const PPCommandItem & rS) : Kind(rS.Kind), Flags(rS.Flags), ID(rS.ID), Name(rS.Name), Icon(rS.Icon)
{
}

PPCommandItem::~PPCommandItem()
{
}

PPCommandItem & FASTCALL PPCommandItem::operator = (const PPCommandItem & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPCommandItem::Copy(const PPCommandItem & s)
{
	Kind = s.Kind;
	Flags = s.Flags;
	ID = s.ID;
	Name = s.Name;
	Icon = s.Icon;
	return 1;
}

const PPCommandItem * PPCommandItem::Next(uint * pPos) const
{
	return 0;
}

int PPCommandItem::Enumerate(CmdItemIterFunc func, long parentID, void * extraPtr) const
{
	int    ok = 1;
	const PPCommandItem * p_item = 0;
	THROW(func(this, parentID, extraPtr));
	for(uint i = 0; (p_item = Next(&i)) != 0;) {
		THROW(p_item->Enumerate(func, ID, extraPtr));
	}
	CATCHZOK
	return ok;
}

PPCommandItem * PPCommandItem::Dup() const
{
	PPCommandItem * p_item = new PPCommandItem(PPCommandItem::kUndef);
	p_item->Copy(*this);
	return p_item;
}

struct _kf_block {
	_kf_block(const PPCommandItem * pItem) : Kind(pItem ? pItem->GetKind() : 0), Flags(pItem ? (pItem->Flags & ~PPCommandItem::fBkgndImageLoaded) : 0)
	{
	}
	int16  Kind;
	int16  Flags;
};

int PPCommandItem::Write_Depricated(SBuffer & rBuf, long) const
{
	int    ok = 1;
	_kf_block _kf(this);
	THROW_SL(rBuf.Write(&_kf, sizeof(_kf)));
	THROW_SL(rBuf.Write(ID));
	THROW_SL(rBuf.Write(Name));
	THROW_SL(rBuf.Write(Icon));
	CATCHZOK
	return ok;
}

int PPCommandItem::Read_Depricated(SBuffer & rBuf, long)
{
	int    ok = 1;
	_kf_block _kf(0);
	THROW_SL(rBuf.Read(&_kf, sizeof(_kf)));
	Kind = _kf.Kind;
	Flags = _kf.Flags;
	THROW_SL(rBuf.Read(ID));
	THROW_SL(rBuf.Read(Name));
	THROW_SL(rBuf.Read(Icon));
	CATCHZOK
	return ok;
}

int PPCommandItem::Write2(void * pHandler, const long rwFlag) const  //@erik v10.6.1
{
	int    ok = 1;
	SString temp_buf;
	_kf_block _kf(this);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		assert(pHandler);
		THROW(pHandler);
		xmlTextWriter * p_xml_writer = static_cast<xmlTextWriter *>(pHandler);
		THROW(p_xml_writer);
		SXml::WNode command_item_node(p_xml_writer, "CommandItem");
		command_item_node.PutInner("Kind", temp_buf.Z().Cat(_kf.Kind));
		command_item_node.PutInner("Flags", temp_buf.Z().Cat(_kf.Flags));
		command_item_node.PutInner("ID", temp_buf.Z().Cat(ID));
		XMLReplaceSpecSymb(temp_buf.Z().Cat(Name), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		command_item_node.PutInner("Name", temp_buf);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(Icon), "&<>\'");
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		command_item_node.PutInner("Icon", temp_buf);
	}
	else {
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int PPCommandItem::Read2(const void * pHandler, const long rwFlag) //@erik v10.6.1
{
	int    ok = 1;
	SString temp_buf;
	assert(pHandler);
	THROW(pHandler);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		const xmlNode * p_parent_node = static_cast<const xmlNode *>(pHandler);
		if(SXml::IsName(p_parent_node, "CommandItem")) {
			for(const xmlNode * p_node = p_parent_node->children; p_node; p_node = p_node->next) {
				if(SXml::GetContentByName(p_node, "Kind", temp_buf))
					Kind = static_cast<int16>(temp_buf.ToLong());
				else if(SXml::GetContentByName(p_node, "Flags", temp_buf))
					Flags = static_cast<int16>(temp_buf.ToLong());
				else if(SXml::GetContentByName(p_node, "ID", temp_buf))
					ID = temp_buf.ToLong();
				else if(SXml::GetContentByName(p_node, "Name", temp_buf))
					Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_node, "Icon", temp_buf))
					Icon = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
	}
	else if(rwFlag & PPCommandMngr::fRWByTxt) {

	}
	CATCHZOK;
	return ok;
}

bool PPCommandItem::IsEq(const void * pCommand) const  //@erik v10.6.1
{
	//bool   yes = true;
	if(!pCommand)
		return false;
	else {
		const PPCommandItem * p_compare_item = static_cast<const PPCommandItem *>(pCommand);
#define CMPF(f) if(f != p_compare_item->f) return false;
		CMPF(Kind);
		CMPF(Flags);
		CMPF(ID);
		CMPF(Name);
		CMPF(Icon);
#undef CMPF
	}
	return true;
}

/*virtual*/void FASTCALL PPCommandItem::SetUniqueID(long * pID)
{
	ID = (*pID)++;
}
//
//
//
PPCommand::PPCommand() : PPCommandItem(kCommand), CmdID(0)
{
	P.Z();
	memzero(Reserve, sizeof(Reserve));
}

int FASTCALL PPCommand::Copy(const PPCommand & s)
{
	PPCommandItem::Copy(s);
	CmdID = s.CmdID;
	P = s.P;
	memcpy(Reserve, s.Reserve, sizeof(Reserve));
	Param = s.Param;
	return 1;
}

PPCommandItem * PPCommand::Dup() const
{
	PPCommand * p_item = new PPCommand;
	p_item->Copy(*this);
	return p_item;
}

int PPCommand::Write_Depricated(SBuffer & rBuf, long extraParam) const
{
	int    ok = 1;
	THROW(PPCommandItem::Write_Depricated(rBuf, extraParam));
	THROW_SL(rBuf.Write(CmdID));
	//
	// @v9.0.11 Поля X и Y заменены на SPoint2S
	// SPoint2S содержит x и y в том же порядке но используются знаковые int16
	// (ранее X и Y были беззнаковыми uint16).
	// Вероятнее всего замена
	// {
	// THROW(rBuf.Write(&X, sizeof(X)));
	// THROW(rBuf.Write(&Y, sizeof(Y)));
	// }
	// на THROW(rBuf.Write(&P, sizeof(P)));
	// не повлечет каких-либо проблем
	//
	THROW_SL(rBuf.Write(&P, sizeof(P)));
	//
	THROW_SL(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Write(Param));
	CATCHZOK
	return ok;
}

int PPCommand::Read_Depricated(SBuffer & rBuf, const long extraParam)
{
	int    ok = 1;
	THROW(PPCommandItem::Read_Depricated(rBuf, extraParam));
	THROW(rBuf.Read(CmdID));
	THROW(rBuf.Read(&P, sizeof(P))); // @v9.0.11 see comment in PPCommand::Write
	THROW(rBuf.Read(Reserve, sizeof(Reserve)));
	THROW(rBuf.Read(Param));
	CATCHZOK
	return ok;
}

int PPCommand::Write2(void * pHandler, const long rwFlag) const
{
	int ok = 1;
	SString temp_buf;
	_kf_block _kf(this);
	assert(pHandler);
	THROW(pHandler);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		xmlTextWriter * p_xml_writer = static_cast<xmlTextWriter *>(pHandler);
		if(p_xml_writer) {
			SXml::WNode command_node(p_xml_writer, "Command");
			THROW(PPCommandItem::Write2(p_xml_writer, rwFlag));
			command_node.PutInner("CmdID", temp_buf.Z().Cat(CmdID));
			command_node.PutInner("X", temp_buf.Z().Cat(P.x));
			command_node.PutInner("Y", temp_buf.Z().Cat(P.y));
			temp_buf.Z().EncodeMime64(static_cast<const char *>(Param.GetBuf(Param.GetRdOffs())), Param.GetAvailableSize());
			command_node.PutInner("Param", temp_buf);
		}
	}
	else {
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int PPCommand::Read2(const void * pHandler, const long rwFlag)
{
	int    ok = 1;
	SString temp_buf;
	int16 x, y;
	assert(pHandler);
	THROW(pHandler);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		const xmlNode * p_parent_node = static_cast<const xmlNode *>(pHandler);
		if(SXml::IsName(p_parent_node, "Command")) {
			for(const xmlNode * p_node = p_parent_node->children; p_node; p_node = p_node->next) {
				if(SXml::GetContentByName(p_node, "CmdID", temp_buf)>0) {
					CmdID = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_node, "X", temp_buf)>0) {
					x = static_cast<int16>(temp_buf.ToLong());
				}
				else if(SXml::GetContentByName(p_node, "Y", temp_buf)>0) {
					y = static_cast<int16>(temp_buf.ToLong());
				}				
				else if(SXml::GetContentByName(p_node, "Param", temp_buf)>0) {
					STempBuffer bin_buf(temp_buf.Len() * 3);
					size_t actual_len = 0;
					temp_buf.DecodeMime64(bin_buf, bin_buf.GetSize(), &actual_len);
					Param.Write(bin_buf, actual_len);
				}
				else if(SXml::IsName(p_node, "CommandItem")) {
					THROW(PPCommandItem::Read2(p_node, rwFlag));
				}
			}
			P.Set(x, y);
		}
	}
	else if(rwFlag==PPCommandMngr::fRWByTxt) {

	}
	CATCHZOK
	return ok;
}

bool PPCommand::IsEq(const void * pCommand) const  //@erik v10.6.1
{
	if(!pCommand)
		return false;
	else {
		const PPCommand * p_compare_command = static_cast<const PPCommand *>(pCommand);
		if(!PPCommandItem::IsEq(pCommand)) {
			return false;
		}
		else {
			#define CMPF(f) if(f != p_compare_command->f) return false;
			CMPF(CmdID);
			CMPF(P);
			#undef CMPF
			if(!Param.IsEq(p_compare_command->Param))
				return false;
		}	
	}
	return true;
}
//
//
//
PPCommandFolder::PPCommandFolder() : PPCommandItem(kFolder)
{
}

PPCommandFolder::PPCommandFolder(int kind) : PPCommandItem(kind) // protected ctr
{
}

PPCommandFolder::PPCommandFolder(const PPCommandFolder & rS) : PPCommandItem(rS)
{
	Copy(rS);
}

PPCommandFolder & FASTCALL PPCommandFolder::operator = (const PPCommandFolder & rS)
{
	Copy(rS);
	return *this;
}

static int _GetIdList(const PPCommandItem * pItem, long parentID, void * extraPtr)
{
	if(pItem)
		static_cast<PPIDArray *>(extraPtr)->add(pItem->GetID()); // @v10.9.3 addUnique-->add
	return 1;
}

long PPCommandFolder::GetUniqueID() const
{
	PPIDArray id_list;
	id_list.add(ID); // @v10.9.3 addUnique-->add
	Enumerate(_GetIdList, 0, &id_list);
	id_list.sortAndUndup(); // @v10.9.3 sort-->sortAndUndup
	return id_list.getLast() + 1;
}

/*virtual*/void FASTCALL PPCommandFolder::SetUniqueID(long * pID)
{
	PPCommandItem::SetUniqueID(pID);
	for(uint i = 0; i < List.getCount(); i++) {
		PPCommandItem * p_item = List.at(i);
		CALLPTRMEMB(p_item, SetUniqueID(pID));
	}
}

int FASTCALL PPCommandFolder::Copy(const PPCommandFolder & s)
{
	int    ok = 1;
	PPCommandItem::Copy(s);
	List.freeAll();
	for(uint i = 0; i < s.GetCount(); i++) {
		const PPCommandItem * p_src_item = s.List.at(i);
		if(p_src_item) {
			PPCommandItem * p_item = p_src_item->Dup();
			THROW_SL(List.insert(p_item));
		}
	}
	CATCHZOK
	return ok;
}

const PPCommandItem * PPCommandFolder::Next(uint * pPos) const
{
	if(*pPos < GetCount()) {
		(*pPos)++;
		return List.at((*pPos)-1);
	}
	else
		return 0;
}

int PPCommandFolder::Write_Depricated(SBuffer & rBuf, long extraParam) const
{
	int    ok = 1;
	uint16 c = 0, i;
	THROW(PPCommandItem::Write_Depricated(rBuf, extraParam));
	c = List.getCount();
	THROW_SL(rBuf.Write(&c, sizeof(c)));
	for(i = 0; i < c; i++)
		THROW(List.at(i)->Write_Depricated(rBuf, extraParam));
	CATCHZOK
	return ok;
}

int PPCommandFolder::Read_Depricated(SBuffer & rBuf, long extraParam)
{
	int    ok = 1;
	uint16 c = 0, i;
	THROW(PPCommandItem::Read_Depricated(rBuf, extraParam));
	THROW_SL(rBuf.Read(&c, sizeof(c)));
	for(i = 0; i < c; i++) {
		PPCommandItem * ptr = 0;
		size_t offs = rBuf.GetRdOffs();
		PPCommandItem item(PPCommandItem::kUndef);
		THROW(item.Read_Depricated(rBuf, extraParam));
		rBuf.SetRdOffs(offs);
		if(item.IsKind(PPCommandItem::kCommand)) {
			ptr = new PPCommand;
			THROW(static_cast<PPCommand *>(ptr)->Read_Depricated(rBuf, extraParam));
		}
		else if(item.IsKind(PPCommandItem::kFolder)) {
			ptr = new PPCommandFolder;
			THROW(static_cast<PPCommandFolder *>(ptr)->Read_Depricated(rBuf, extraParam));
		}
		else if(item.IsKind(PPCommandItem::kGroup)) {
			ptr = new PPCommandGroup;
			THROW(static_cast<PPCommandGroup *>(ptr)->Read_Depricated(rBuf, extraParam));
		}
		else if(item.IsKind(PPCommandItem::kSeparator)) {
			ptr = new PPCommandItem(PPCommandItem::kUndef);
			THROW(static_cast<PPCommandItem *>(ptr)->Read_Depricated(rBuf, extraParam));
		}
		if(ptr) { // @v11.0.4 @fix
			THROW_SL(List.insert(ptr));
		}
	}
	CATCHZOK
	return ok;
}
//
// @erik v10.6.1 {
//
int PPCommandFolder::Write2(void * pHandler, const long rwFlag) const // @recursion
{
	int    ok = 1;
	assert(pHandler);
	THROW(pHandler);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		xmlTextWriter * p_xml_writer = static_cast<xmlTextWriter *>(pHandler);
		//
		//зависимы от зоны видимости command_folder_node.
		//  Поэтому происходит дублирование цикла forB.
		//  Если выйдем за пределы зоны видемости, будем писать в файл за пределами
		//  тега CommandFolder
		//Если  p_xml_writer определен, то пишем в него. Если нет, то мы еще в начале xml файла и 
		//  p_ci является экземпляром PPCommandGroup, в нем мы и определим далее p_xml_writer
		//
		THROW(p_xml_writer)
		SXml::WNode command_folder_node(p_xml_writer, "CommandFolder");
		//if(!(Kind == PPCommandItem::kGroup && !Flags && !ID))
		THROW(PPCommandItem::Write2(p_xml_writer, rwFlag));
		uint c = List.getCount();
		for(uint i = 0; i < c; i++) {
			const PPCommandItem * p_ci = List.at(i);
			if(p_ci) {
				THROW(p_ci->Write2(p_xml_writer, rwFlag));
			}
		}
	}
	else {
		ok = 0;
	}
	CATCHZOK
	return ok;
}

int PPCommandFolder::Read2(const void * pHandler, const long rwFlag)
{
	int    ok = 1;
	PPCommandItem * p_command_item = 0;
	assert(pHandler);
	THROW(pHandler);
	THROW(rwFlag == PPCommandMngr::fRWByXml);
	{
		const xmlNode * p_parent_node = static_cast<const xmlNode *>(pHandler);
		if(SXml::IsName(p_parent_node, "CommandFolder")) {
			for(const xmlNode * p_node = p_parent_node->children; p_node; p_node = p_node->next) {
				if(SXml::IsName(p_node, "CommandItem")) {
					p_command_item = new PPCommandItem(PPCommandItem::kUndef);
					THROW(p_command_item->Read2(p_node, rwFlag));
					if(p_command_item->IsKind(PPCommandItem::kSeparator)) {
						THROW_SL(List.insert(p_command_item));						
					}
					else {
						Kind = p_command_item->Kind;
						ID = p_command_item->GetID();
						Flags = p_command_item->Flags;
						Name = p_command_item->Name;
						Icon = p_command_item->Icon;
						delete p_command_item;
					}
					p_command_item = 0;
				}
				else {
					if(SXml::IsName(p_node, "Command")) {
						p_command_item = new PPCommand;
						THROW(static_cast<PPCommand *>(p_command_item)->Read2(p_node, rwFlag));
					}
					else if(SXml::IsName(p_node, "CommandFolder")) {
						p_command_item = new PPCommandFolder;
						THROW(static_cast<PPCommandFolder *>(p_command_item)->Read2(p_node, rwFlag));
					}
					else if(SXml::IsName(p_node, "CommandGroup")) {
						p_command_item = new PPCommandGroup;
						THROW(static_cast<PPCommandGroup *>(p_command_item)->Read2(p_node, rwFlag));
					}
					if(p_command_item) {
						THROW_SL(List.insert(p_command_item));
						p_command_item = 0; // to prevent destruction at the epilog
					}				
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

bool PPCommandFolder::IsEq(const void * pCommand) const  //@erik v10.6.1
{
	bool   yes = true;
	if(pCommand) {
		const  PPCommandFolder * p_compare_folder = static_cast<const PPCommandFolder *>(pCommand);
		if(!PPCommandItem::IsEq(p_compare_folder))
			yes = false;
		else {
			const uint c = List.getCount();
			if(c != p_compare_folder->List.getCount())
				yes = false;
			else {
				for(uint i = 0; yes && i < c; i++) {
					const PPCommandItem * p_comm_item = List.at(i);
					if(p_comm_item) {
						const PPCommandItem * p_other_comm_item = p_compare_folder->SearchByID(p_comm_item->GetID(), 0);
						if(!p_other_comm_item || !p_comm_item->IsEq(p_other_comm_item))
							yes = false;
					}
				}
			}
		}
	}
	else
		yes = false;
	return yes;
}
// } @erik

PPCommandItem * PPCommandFolder::Dup() const
{
	PPCommandFolder * p_item = new PPCommandFolder;
	p_item->Copy(*this);
	return p_item;
}

uint PPCommandFolder::GetCount() const { return List.getCount(); }
const PPCommandItem * PPCommandFolder::Get(uint pos) const { return (pos < GetCount()) ? List.at(pos) : 0; }

int PPCommandFolder::Update(uint pos, const PPCommandItem * pItem)
{
	int    ok = 1;
	const  uint c = GetCount();
	const  PPCommandGroup * p_new_grp = pItem->IsKind(PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(pItem) : 0;
	THROW(pos < c);
	for(uint i = 0; i < c; i++) {
		if(i != pos) {
			const PPCommandItem * p = List.at(i);
			if(p && p->IsKind(pItem->GetKind())) {
				const PPCommandGroup * p_grp = p->IsKind(PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(p) : 0;
				if(p_new_grp && p_grp) {
					THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0 || !p_new_grp->IsDbSymbEq(*p_grp), PPERR_DUPCMDNAME, pItem->Name);
				}
				else {
					THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0, PPERR_DUPCMDNAME, pItem->Name);
				}
			}
		}
	}
	{
		PPCommandItem * p_item = pItem->Dup();
		THROW_MEM(p_item);
		delete List.at(pos);
		List.atPut(pos, p_item);
	}
	CATCHZOK
	return ok;
}

int PPCommandFolder::Add(int pos, const PPCommandItem * pItem)
{
	int    ok = 1;
	THROW_INVARG(pItem);
	const  uint c = GetCount();
	const  PPCommandGroup * p_new_grp = pItem->IsKind(PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(pItem) : 0;
	for(uint i = 0; i < c; i++) {
		const PPCommandItem * p = List.at(i);
		if(p && p->IsKind(pItem->GetKind())) {
			const PPCommandGroup * p_grp = p->IsKind(PPCommandItem::kGroup) ? static_cast<const PPCommandGroup *>(p) : 0;
			if(p_new_grp && p_grp) {
				THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0 || !p_new_grp->IsDbSymbEq(*p_grp), PPERR_DUPCMDNAME, pItem->Name);
			}
			else {
				THROW_PP_S(pItem->Name.CmpNC(p->Name) != 0 || pItem->IsKind(PPCommandItem::kSeparator), PPERR_DUPCMDNAME, pItem->Name); //@SevaSob @v11.4.0
			}
		}
	}
	{
		PPCommandItem * p_item = pItem->Dup();
		THROW_MEM(p_item);
		if(pos < 0 || pos >= static_cast<int>(c))
			List.insert(p_item);
		else
			List.atInsert(pos, p_item);
	}
	CATCHZOK
	return ok;
}

int PPCommandFolder::AddSeparator(int pos)
{
	int    ok = 1;
	PPCommandItem * p_item = new PPCommandItem(kSeparator);
	p_item->Name.Z().CatCharN('-', 40).Transf(CTRANSF_OUTER_TO_INNER);
	if(pos < 0 || pos >= (int)GetCount())
		List.insert(p_item);
	else
		List.atInsert(pos, p_item);
	return ok;
}

int PPCommandFolder::Remove(uint pos)
{
	if(pos < GetCount()) {
		List.atFree(pos);
		return 1;
	}
	else
		return 0;
}

PPCommandFolder::CommandGroupList::CommandGroupList() : LastSurrID(0)
{
}

PPCommandFolder::CommandGroupList & PPCommandFolder::CommandGroupList::Z()
{
	L.clear();
	return *this;
}

uint PPCommandFolder::CommandGroupList::GetCount() const
{
	return L.getCount();
}

int PPCommandFolder::CommandGroupList::Get(uint idx, Entry & rEntry) const
{
	if(idx < L.getCount()) {
		const InnerEntry & r_entry = L.at(idx);
		rEntry.NativeID = r_entry.NativeID;
		rEntry.SurrID = r_entry.SurrID;
		rEntry.Uuid = r_entry.Uuid;
		GetS(r_entry.NameP, rEntry.Name);
		return 1;
	}
	else
		return 0;
}

long PPCommandFolder::CommandGroupList::GetSurrIdByUuid(const S_GUID & rUuid) const
{
	uint   idx = 0;
	return SearchByUuid(rUuid, &idx) ? L.at(idx).SurrID : 0;
}

S_GUID PPCommandFolder::CommandGroupList::GetUuidBySurrId(long surrId) const
{
	uint   idx = 0;
	return SearchBySurrID(surrId, &idx) ? L.at(idx).Uuid : ZEROGUID;
}

void PPCommandFolder::CommandGroupList::GetStrAssocList(StrAssocArray & rResult) const
{
	rResult.Z();
	SString temp_buf;
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_entry = L.at(i);
		GetS(r_entry.NameP, temp_buf);
		rResult.Add(r_entry.SurrID, temp_buf);
	}
}

int PPCommandFolder::CommandGroupList::SearchByNativeID(long id, LongArray & rIdxList) const
{
	int    ok = 0;
	rIdxList.Z();
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_entry = L.at(i);
		if(r_entry.NativeID == id) {
			rIdxList.add(static_cast<long>(i));
			ok = 1;
		}
	}
	return ok;
}

int PPCommandFolder::CommandGroupList::SearchBySurrID(long id, uint * pIdx) const
{
	int    ok = 0;
	if(id) {
		for(uint i = 0; !ok && i < L.getCount(); i++) {
			const InnerEntry & r_entry = L.at(i);
			if(r_entry.SurrID == id) {
				ASSIGN_PTR(pIdx, i);
				ok = 1;
			}
		}	
	}
	return ok;
}

int PPCommandFolder::CommandGroupList::SearchByUuid(const S_GUID & rUuid, uint * pIdx) const
{
	int    ok = 0;
	if(!!rUuid) {
		for(uint i = 0; !ok && i < L.getCount(); i++) {
			const InnerEntry & r_entry = L.at(i);
			if(r_entry.Uuid == rUuid) {
				ASSIGN_PTR(pIdx, i);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPCommandFolder::CommandGroupList::Add(long nativeId, const S_GUID & rUuid, const char * pName, long * pSurrID)
{
	int    ok = 0;
	uint   idx = 0;
	if(SearchByUuid(rUuid, &idx)) {
		const InnerEntry & r_entry = L.at(idx);
		ok = (nativeId == r_entry.NativeID) ? -1 : -2;
		ASSIGN_PTR(pSurrID, r_entry.SurrID);
	}
	else {
		InnerEntry new_entry;
		new_entry.SurrID = ++LastSurrID;
		new_entry.NativeID = nativeId;
		new_entry.Uuid = rUuid;
		AddS(pName, &new_entry.NameP);
		if(L.insert(&new_entry)) {
			ASSIGN_PTR(pSurrID, new_entry.SurrID);
			ok = 1;
		}
		else
			ok = PPSetErrorSLib();
	}
	return ok;
}

/*static*/int PPCommandFolder::GetCommandGroupList(const PPCommandGroup * pGrp, PPCommandGroupCategory kind, CommandGroupList & rResult)
{
	int    ok = 1;
	SString db_symb;
	const PPCommandItem * p_item = 0;
	PPCommandGroup grp;
	const PPCommandGroup * p_grp = 0;
	PPCommandMngr * p_mgr = 0;
	CurDict->GetDbSymb(db_symb);
	if(pGrp)
		p_grp = pGrp;
	else {
		THROW(p_mgr = GetCommandMngr(PPCommandMngr::ctrfReadOnly, kind));
		THROW(p_mgr->Load__2(&grp, db_symb, PPCommandMngr::fRWByXml));
		p_grp = &grp;
		ZDELETE(p_mgr);
	}
	for(uint i = 0; (p_item = p_grp->Next(&i)) != 0;) {
		if(p_item->IsKind(PPCommandItem::kGroup)) {
			const PPCommandGroup * p_cg = static_cast<const PPCommandGroup *>(p_item);
			if(kind == cmdgrpcDesktop) {
				if(p_cg->IsDbSymbEq(db_symb)) {
					long surr_id = 0;
					rResult.Add(p_cg->ID, p_cg->Uuid, p_cg->Name, &surr_id);
				}
			}
			else if(kind == cmdgrpcMenu) {
				long surr_id = 0;
				rResult.Add(p_cg->ID, p_cg->Uuid, p_cg->Name, &surr_id);
			}
		}
	}
	CATCHZOK
	ZDELETE(p_mgr);
	return ok;
}

struct _ParentList {
	int    OnlyFolders;
	StrAssocArray * P_List;
};

int _GetIdParentList(const PPCommandItem * pItem, long parentID, void * extraPtr)
{
	int    ok = 0;
	if(pItem) {
		int    only_folders = static_cast<_ParentList *>(extraPtr)->OnlyFolders;
		StrAssocArray * p_list = static_cast<_ParentList *>(extraPtr)->P_List;
		if(p_list) {
			if(!only_folders || pItem->IsKind(PPCommandItem::kFolder)) {
				SString cmd_buf(pItem->Name);
				if(cmd_buf.C(0) == '@') {
					SString temp_buf;
					if(PPLoadString(cmd_buf.ShiftLeft(), temp_buf) > 0)
						cmd_buf = temp_buf;
				}
				p_list->Add(pItem->GetID(), parentID, cmd_buf.Strip(), 0);
			}
			ok = 1;
		}
	}
	return ok;
}

int PPCommandFolder::GetCommandList(StrAssocArray * pList, int onlyFolders)
{
	int    ok = -1;
	if(pList) {
		long   id = ID;
		_ParentList _list;
		_list.P_List = pList;
		_list.P_List->Z();
		_list.OnlyFolders = onlyFolders;
		ID = 0;
		ok = Enumerate(_GetIdParentList, 0, &_list);
		if(!onlyFolders && _list.P_List->getCount()) { 
			_list.P_List->AtFree(0); // Первым элементом будет дескриптор меню
		}
		_list.P_List->SortByID();
		ID = id;
	}
	return ok;
}

const PPCommandItem * PPCommandFolder::SearchByName(const char * pName, const char * pDbSymb, uint * pPos) const
{
	uint   pos = 0;
	const  PPCommandItem * p_ret_item = 0;
	while(pos < List.getCount()) {
		const PPCommandItem * p_item = List.at(pos++);
		if(p_item->Name.CmpNC(pName) == 0) {
			if(!pDbSymb || (p_item->IsKind(PPCommandItem::kGroup) && static_cast<const PPCommandGroup *>(p_item)->IsDbSymbEq(pDbSymb))) {
				ASSIGN_PTR(pPos, pos);
				p_ret_item = p_item;
				break;
			}
		}
	}
	return p_ret_item;
}

const PPCommandItem * PPCommandFolder::SearchByUuid(const S_GUID & rUuid, uint * pPos) const
{
	const PPCommandItem * p_result = 0;
	for(uint i = 0; !p_result && i < List.getCount(); i++) {
		const PPCommandItem * p_item = List.at(i);
		if(p_item && p_item->IsKind(PPCommandItem::kGroup)) {
			if(static_cast<const PPCommandGroup *>(p_item)->Uuid == rUuid) {
				p_result = p_item;
				ASSIGN_PTR(pPos, i);
			}
		}
	}
	return p_result;
}

const PPCommandItem * PPCommandFolder::SearchByID(long id, uint * pPos) const
{
	uint   pos = 0;
	PPCommandItem * p_item = 0;
	if(List.lsearch(&id, &pos, CMPF_LONG, offsetof(PPCommandItem, ID))) {
		p_item = List.at(pos);
		ASSIGN_PTR(pPos, pos);
	}
	return p_item;
}

struct _Srch {
	_Srch(long parentID, long id, const PPCommandItem * pItem) : ParentID(parentID), ID(id), P_Item(pItem)
	{
	}
	long   ParentID;               // out param
	long   ID;                     // in param
	const  PPCommandItem * P_Item; // out param
};

static int _SearchByID(const PPCommandItem * pItem, long parentID, void * extraPtr)
{
	int    ok = -1;
	if(pItem) {
		_Srch * p_s = static_cast<_Srch *>(extraPtr);
		if(p_s->ID == pItem->GetID()) {
			p_s->ParentID = parentID;
			p_s->P_Item   = pItem;
			ok = 1;
		}
	}
	return ok;
}

PPCommandItem * PPCommandFolder::SearchByIDRecursive(long id, long * pParentID)
{
	_Srch _s(0, id, 0);
	if(Enumerate(_SearchByID, ID, &_s) > 0)
		ASSIGN_PTR(pParentID, _s.ParentID);
	return const_cast<PPCommandItem *>(_s.P_Item); // @badcast
}

const PPCommandItem * PPCommandFolder::SearchByIDRecursive_Const(long id, long * pParentID) const
{
	_Srch _s(0, id, 0);
	if(Enumerate(_SearchByID, ID, &_s) > 0)
		ASSIGN_PTR(pParentID, _s.ParentID);
	return _s.P_Item;
}

const PPCommandItem * PPCommandFolder::SearchByCoord(SPoint2S coord, const PPDesktop & rD, uint * pPos) const
{
	const  int _igap = rD.GetIconGap();
	const  int _isz  = rD.GetIconSize();
	const PPCommandItem * p_item = 0;
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->IsKind(PPCommandItem::kCommand)) {
			SPoint2S lu = static_cast<const PPCommand *>(p_item)->P;
			TRect sqr_coord(lu, lu + (_isz * 2));
			if(sqr_coord.contains(coord)) {
				ASSIGN_PTR(pPos, i - 1);
				break;
			}
		}
	}
	return p_item;
}

int PPCommandFolder::SearchFreeCoord(RECT r, const PPDesktop & rD, SPoint2S * pCoord) const
{
	const  int _igap = rD.GetIconGap();
	const  int _isz  = rD.GetIconSize();
	int    ok = -1;
	long   x = r.left + _igap + _isz * 2;
	long   y = r.top  + _igap + _isz * 2;
	long   mx = r.right - _igap;
	long   my = r.bottom - _igap;
	for(long dx = x; ok < 0 && dx < mx; dx += (_isz * 2 + _igap)) {
		for(long dy = y; ok < 0 && dy < my; dy += (_isz * 2 + _igap)) {
			SPoint2S c;
			c.Set(dx - _isz * 2, dy - _isz * 2);
			if(SearchByCoord(c, rD, 0) == 0) {
				ASSIGN_PTR(pCoord, c);
				ok = 1;
			}
		}
	}
	return ok;
}

const PPCommandItem * PPCommandFolder::SearchFirst(uint * pPos)
{
	POINT  c;
	long   pos = -1;
	const  PPCommandItem * p_item = 0;
	MEMSZERO(c);
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->IsKind(PPCommandItem::kCommand)) {
			const long x = static_cast<const PPCommand *>(p_item)->P.x;
			const long y = static_cast<const PPCommand *>(p_item)->P.y;
			if((c.y == 0 || y < c.y) || (c.x == 0 || y == c.y && x <= c.x)) {
				c.x = x;
				c.y = y;
				pos = static_cast<long>(i - 1);
			}
		}
	}
	ASSIGN_PTR(pPos, static_cast<uint>(pos));
	return (pos >= 0) ? Get(static_cast<uint>(pos)) : 0;
}

const PPCommandItem * PPCommandFolder::SearchNextByCoord(POINT coord, const PPDesktop & rD, Direction next, uint * pPos)
{
	const  int _igap = rD.GetIconGap();
	const  int _isz  = rD.GetIconSize();
	POINT  c;
	long   pos = -1;
	double distance = MAXLONG;
	const  PPCommandItem * p_item = 0;
	c.x = coord.x + _isz;
	c.y = coord.y + _isz;
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->IsKind(PPCommandItem::kCommand)) {
			long   x = static_cast<const PPCommand *>(p_item)->P.x;
			long   y = static_cast<const PPCommand *>(p_item)->P.y;
			if(next == nextUp && y < coord.y || next == nextDown && y > coord.y ||
				next == nextLeft && x < coord.x || next == nextRight && x > coord.x) {
				x += _isz;
				y += _isz;
				double d = sqrt((double)(x - c.x)*(x - c.x) + (y - c.y)*(y - c.y));
				if(d != 0 && d < distance) {
					distance = d;
					pos = static_cast<uint>(i - 1);
				}
			}
		}
	}
	ASSIGN_PTR(pPos, static_cast<uint>(pos));
	return (pos >= 0) ? Get(static_cast<uint>(pos)) : 0;
}

int PPCommandFolder::GetIntersectIDs(const TRect & rR, const PPDesktop & rD, PPIDArray * pAry)
{
	const  int _isz  = rD.GetIconSize();
	int    found = 0;
	const PPCommandItem * p_item = 0;
	for(uint i = 0; p_item = Next(&i);) {
		if(p_item->IsKind(PPCommandItem::kCommand)) {
			TRect ir;
			/*
			RECT ri;
			ri.top  = ((PPCommand*)p_item)->Y;
			ri.left = ((PPCommand*)p_item)->X;
			ri.bottom = ri.top  + _isz * 2;
			ri.right  = ri.left + _isz * 2;
			*/
			rD.CalcIconRect(static_cast<const PPCommand *>(p_item)->P, ir);
			//if(SIntersectRect(ir, rect)) {
			if(rR.Intersect(ir, 0)) {
				CALLPTRMEMB(pAry, add(p_item->GetID()));
				found = 1;
			}
		}
	}
	return found;
}

int PPCommandFolder::GetIntersectIDs(SPoint2S coord, const PPDesktop & rD, PPIDArray * pAry)
{
	/*
	const  int _isz  = rD.GetIconSize();
	RECT   r;
	r.top    = coord.y;
	r.left   = coord.x;
	r.bottom = r.top  + _isz * 2;
	r.right  = r.left + _isz * 2;
	*/
	TRect ir;
	rD.CalcIconRect(coord, ir);
	return GetIntersectIDs(ir, rD, pAry);
}

int PPCommandFolder::GetIconRect(long id, const PPDesktop & rD, TRect * pRect) const
{
	//const  int _isz = rD.GetIconSize();
	int    ok = -1;
	TRect  ir;
	const  PPCommandItem * p_item = SearchByID(id, 0);
	if(p_item && p_item->IsKind(PPCommandItem::kCommand)) {
        rD.CalcIconRect(static_cast<const PPCommand *>(p_item)->P, ir);
		ok = 1;
	}
	ASSIGN_PTR(pRect, ir);
	return ok;
}
//
//
//
PPCommandGroup::PPCommandGroup() : PPCommandFolder(kGroup), Type(cmdgrpcUndef)
{
}

PPCommandGroup::PPCommandGroup(PPCommandGroupCategory cmdgrpc, const char * pDbSymb, const char * pName) : 
	PPCommandFolder(kGroup), Type(cmdgrpc), Uuid(SCtrGenerate_)
{
	assert(oneof2(Type, cmdgrpcDesktop, cmdgrpcMenu));
	if(isempty(pDbSymb)) {
		if(Type == cmdgrpcDesktop && CurDict) {
			CurDict->GetDbSymb(DbSymb);
		}
	}
	else
		DbSymb = pDbSymb;
	Name = pName;
	if(Type == cmdgrpcDesktop)
		Flags |= PPCommandItem::fBkgndGradient;
}

/*void PPCommandGroup::InitDefaultDesktop(const char * pName)
{
	Name = pName;
	CurDict->GetDbSymb(DbSymb);
	Flags = PPCommandItem::fBkgndGradient;
}*/

PPCommandGroup::PPCommandGroup(const PPCommandGroup & rS)
{
	Copy(rS);
}

PPCommandGroup & FASTCALL PPCommandGroup::operator = (const PPCommandGroup & rS)
{
	Copy(rS);
	return *this;
}

const SString & PPCommandGroup::GetLogo() const
{
	return Logo_;
}

int PPCommandGroup::SetLogo(const char * pPath)
{
	int   ok = 1;
	Flags &= ~fBkgndImage;
	(Logo_ = pPath).Strip();
	if(Logo_.NotEmpty()) {
		if(fileExists(Logo_)) {
			Flags |= fBkgndImage;
			ok = 1;
		}
		else {
			Logo_ = 0;
			ok = PPSetErrorSLib();
		}
	}
	return ok;
}

void FASTCALL PPCommandGroup::SetDbSymb(const char * pDbSymb)
{
	(DbSymb = pDbSymb).Strip();
}

int FASTCALL PPCommandGroup::IsDbSymbEq(const char * pDbSymb) const
{
	return BIN(DbSymb.CmpNC(pDbSymb) == 0);
}

int FASTCALL PPCommandGroup::IsDbSymbEq(const PPCommandGroup & rGrp) const
{
	return BIN(DbSymb.CmpNC(rGrp.DbSymb) == 0);
}

void PPCommandGroup::GenerateGuid()
{
	Uuid.Generate();
}

const S_GUID & FASTCALL PPCommandGroup::GetGuid() const
{
	return Uuid;
}

int FASTCALL PPCommandGroup::Copy(const PPCommandGroup & s)
{
	DbSymb = s.DbSymb;
	Logo_  = s.Logo_;
	Uuid = s.Uuid;	
	Type = s.Type;
	return PPCommandFolder::Copy(s);
}

PPCommandItem * PPCommandGroup::Dup() const
{
	PPCommandGroup * p_item = new PPCommandGroup;
	p_item->Copy(*this);
	return p_item;
}

int PPCommandGroup::Write_Depricated(SBuffer & rBuf, long extraParam) const
{
	int    ok = 1;
	THROW(PPCommandFolder::Write_Depricated(rBuf, extraParam));
	THROW_SL(rBuf.Write(DbSymb));
	if(Flags & (fBkgndImage|fBkgndImageLoaded)) {
		SString buf, dir;
		PPGetPath(PPPATH_BIN, dir);
		PPLoadText(PPTXT_DESKIMGDIR, buf);
		dir.SetLastSlash().Cat(buf).SetLastSlash();
		ObjLinkFiles _lf;
		_lf.Init(PPOBJ_DESKTOP, dir);
		_lf.Load(ID, 0L);
		if(Logo_.NotEmpty()) {
			_lf.Replace(0, Logo_);
		}
		else {
			_lf.Remove(0);
		}
		_lf.Save(ID, 0L);
	}
	CATCHZOK
	return ok;
}

int PPCommandGroup::Read_Depricated(SBuffer & rBuf, long extraParam)
{
	int    ok = 1;
	THROW(PPCommandFolder::Read_Depricated(rBuf, extraParam));
	THROW_SL(rBuf.Read(DbSymb));
	// @erik v10.6.6 {
	if(Uuid.IsZero())
		Uuid.Generate();
	// } @erik
	if(Flags & fBkgndImage) {
		PROFILE_START
		SString buf, dir;
		PPGetPath(PPPATH_BIN, dir);
		PPLoadText(PPTXT_DESKIMGDIR, buf);
		dir.SetLastSlash().Cat(buf).SetLastSlash();
		ObjLinkFiles logo;
		logo.Init(PPOBJ_DESKTOP, dir);
		logo.Load(ID, 0L);
		logo.At(0, buf);
		SetLogo(buf);
		if(Flags & fBkgndImage)
			Flags |= fBkgndImageLoaded;
		PROFILE_END
	}
	CATCHZOK
	return ok;
}

// @erik v10.7.6 {
int PPCommandGroup::Write2(void * pHandler, const long rwFlag) const
{
	int    ok = 1;
	SString temp_buf;
	assert(pHandler);
	THROW(pHandler);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		SString db_symb = DbSymb;
		if(db_symb.NotEmpty() || Type == cmdgrpcMenu) { // @v11.0.1 (Type == cmdgrpcMenu)
			// @v11.0.1 {
			if(db_symb.IsEmpty())
				db_symb = "undefined";
			// } @v11.0.1 
			SString path, guid_str;
			xmlTextWriter * p_xml_writer = static_cast<xmlTextWriter *>(pHandler);
			THROW(Uuid.ToStr(S_GUID::fmtIDL, guid_str));
			THROW(p_xml_writer);
			SXml::WNode command_group_node(p_xml_writer, "CommandGroup");
			{
				{
					const char * p_type = 0;
					if(Type == cmdgrpcDesktop)
						p_type = "desktop";
					else if(Type == cmdgrpcMenu)
						p_type = "menu";
					else 
						p_type = "undefined";
					if(p_type)
						command_group_node.PutInner("Type", p_type);
				}
				XMLReplaceSpecSymb(temp_buf.Z().Cat(db_symb), "&<>\'");
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
				command_group_node.PutInner("DbSymb", temp_buf);
				command_group_node.PutInner("Uuid", guid_str); // @v10.9.3 Начиная с этой версии поле в xml-файле называется Uuid вместо DeskGuid
				SVerT version_ppy = DS.GetVersion();
				version_ppy.ToStr(temp_buf.Z());
				command_group_node.PutInner("PpyVersion", temp_buf);
			}
			THROW(PPCommandFolder::Write2(p_xml_writer, rwFlag));
			if(Flags & (fBkgndImage|fBkgndImageLoaded)) {
				SString buf, dir;
				PPGetPath(PPPATH_BIN, dir);
				PPLoadText(PPTXT_DESKIMGDIR, buf);
				dir.SetLastSlash().Cat(buf).SetLastSlash();
				ObjLinkFiles _lf;
				_lf.Init(PPOBJ_DESKTOP, dir);
				_lf.Load(ID, 0L);
				if(Logo_.NotEmpty()) {
					_lf.Replace(0, Logo_);
				}
				else {
					_lf.Remove(0);
				}
				_lf.Save(ID, 0L);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPCommandGroup::Read2(const void * pHandler, const long rwFlag)
{
	int    ok = 1;
	int    state = 0;
	SString temp_buf;
	assert(pHandler);
	THROW(pHandler);
	Type = cmdgrpcUndef; // @v10.9.3
	if(rwFlag == PPCommandMngr::fRWByXml) {
		const xmlNode * p_parent_node = static_cast<const xmlNode *>(pHandler);
		if(SXml::IsName(p_parent_node, "CommandGroup")) {
			for(const xmlNode * p_node = p_parent_node->children; p_node; p_node = p_node->next) {
				if(SXml::GetContentByName(p_node, "DbSymb", temp_buf) > 0) {
					if(temp_buf.NotEmpty()) {
						DbSymb = temp_buf.Transf(CTRANSF_UTF8_TO_OUTER);
					}						
				}
				// @v10.9.3 Начиная с этой версии поле в xml-файле называется Uuid вместо DeskGuid
				else if(SXml::GetContentByName(p_node, "DeskGuid", temp_buf) > 0 || SXml::GetContentByName(p_node, "Uuid", temp_buf) > 0)
					Uuid.FromStr(temp_buf);
				else if(SXml::GetContentByName(p_node, "Type", temp_buf) > 0) {
					if(temp_buf.IsEqiAscii("desktop"))
						Type = cmdgrpcDesktop;
					else if(temp_buf.IsEqiAscii("menu"))
						Type = cmdgrpcMenu;
					else if(temp_buf.IsEqiAscii("undefined"))
						Type = cmdgrpcUndef;
					else {
						const long  c = temp_buf.ToLong();
						Type = oneof2(c, cmdgrpcDesktop, cmdgrpcMenu) ? static_cast<PPCommandGroupCategory>(c) : cmdgrpcUndef;
					}
				}
				else if(SXml::IsName(p_node, "CommandFolder")) {
					THROW(PPCommandFolder::Read2(p_node, rwFlag)); // @recursion
					state = 1; // если данные считаны корректно, то можем работать дальше
				}
			}
			if(Uuid.IsZero())
				Uuid.Generate();
		}	
		if(Flags & fBkgndImage && state == 1) {
			PROFILE_START
			SString buf, dir;
			PPGetPath(PPPATH_BIN, dir);
			PPLoadText(PPTXT_DESKIMGDIR, buf);
			dir.SetLastSlash().Cat(buf).SetLastSlash();
			ObjLinkFiles logo;
			logo.Init(PPOBJ_DESKTOP, dir);
			logo.Load(ID, 0L);
			logo.At(0, buf);
			SetLogo(buf);
			Flags |= fBkgndImageLoaded;
			PROFILE_END
		}
	}
	else
		ok = 0;
	CATCHZOK
	return ok;
}

bool PPCommandGroup::IsEq(const void * pCommand) const  //@erik v10.6.1
{
	bool   yes = true;
	if(pCommand) {
		const PPCommandGroup * p_compare_group = static_cast<const PPCommandGroup *>(pCommand);
		if(BIN(DbSymb.CmpNC(p_compare_group->DbSymb)!=0)) {
			yes = false;
		}
		else {
			if(BIN(Logo_.CmpNC(p_compare_group->Logo_)!=0)) {
				yes = false;
			}
			else if(!PPCommandFolder::IsEq(p_compare_group)) {
				yes = false;
			}
		}
	}
	else
		yes = false;
	return yes;
}
// } @erik

int PPCommandGroup::LoadLogo()
{
	int    ok = -1;
	SString buf, dir;
	PPGetPath(PPPATH_BIN, dir);
	PPLoadText(PPTXT_DESKIMGDIR, buf);
	dir.SetLastSlash().Cat(buf).SetLastSlash();
	ObjLinkFiles logo;
	logo.Init(PPOBJ_DESKTOP, dir);
	logo.Load(ID, 0L);
	if(logo.At(0, Logo_.Z()))
		ok = 1;
	return ok;
}

int PPCommandGroup::StoreLogo()
{
	int    ok = 1;
	SString buf, dir;
	PPGetPath(PPPATH_BIN, dir);
	PPLoadText(PPTXT_DESKIMGDIR, buf);
	dir.SetLastSlash().Cat(buf).SetLastSlash();
	ObjLinkFiles _lf;
	_lf.Init(PPOBJ_DESKTOP, dir);
	_lf.Load(ID, 0L);
	if(Logo_.Len() == 0)
		_lf.Remove(0);
	else
		_lf.Replace(0, Logo_);
	_lf.Save(ID, 0L);
	return ok;
}

PPCommandGroup * PPCommandGroup::GetGroup(PPCommandGroupCategory kind, const S_GUID & rUuid)
{
	const PPCommandItem * p_item = SearchByUuid(rUuid, 0);
	if(p_item && p_item->IsKind(PPCommandItem::kGroup)) {
		const PPCommandGroup * p_grp = static_cast<const PPCommandGroup *>(p_item);
		return (kind == cmdgrpcUndef || p_grp->Type == kind) ? const_cast<PPCommandGroup *>(p_grp) : 0; // @badcast
	}
	else
		return 0;
}
//
//
//
#define PPCS_SIGNATURE 0x54435050L

PPCommandMngr * GetCommandMngr(uint ctrFlags, PPCommandGroupCategory kind, const char * pPath /*=0*/)
{
	PPCommandMngr * p_mgr = 0;
	SString path;
	if(isempty(pPath)) {
		uint   file_id = 0;
		if(kind == cmdgrpcDesktop)
			file_id = PPFILNAM_PPDESK_BIN;
		else if(kind == cmdgrpcMenu)
			file_id = PPFILNAM_PPCMD_BIN;
		PPGetFilePath(PPPATH_BIN, file_id/*isDesktop ? PPFILNAM_PPDESK_BIN : PPFILNAM_PPCMD_BIN*/, path);
	}
	else
		path = pPath;
	/* @v10.9.5 if(path.Empty())
		PPSetError(PPERR_UNDEFCMDFILENAME);
	else */
		p_mgr = new PPCommandMngr(path, /*readOnly*/ctrFlags, /*isDesktop*/kind);
	if(p_mgr) {
		if(!p_mgr->IsValid_()) {
			ZDELETE(p_mgr);
		}
	}
	else
		PPSetErrorNoMem();
	return p_mgr;
}

/*
int PPCommandMngr::Backup()
{
}
*/

/*static*/const SString & PPCommandMngr::InitStoragePath(int kind)
{
	assert(oneof2(kind, cmdgrpcMenu, cmdgrpcDesktop));
	SString & r_temp_path = SLS.AcquireRvlStr();
	if(/*isDesktop*/kind == cmdgrpcDesktop)
		PPCommandMngr::GetDesksDir(r_temp_path);
	else if(kind == cmdgrpcMenu)
		PPCommandMngr::GetMenuDir(r_temp_path);
	return r_temp_path;
}

PPCommandMngr::PPCommandMngr(const char * pFileName, uint ctrFlags, /*int isDesktop*/PPCommandGroupCategory kind) : 
	Status(0), CtrFlags(ctrFlags), XmlDirPath(InitStoragePath(kind))
{
	assert(oneof2(kind, cmdgrpcMenu, cmdgrpcDesktop));
	if(!(CtrFlags & ctrfSkipObsolete)) {
		if(fileExists(pFileName)) { // @v10.9.5
			const long mode = (SFile::mBinary|SFile::mNoStd|SFile::mDenyWrite) | ((CtrFlags & ctrfReadOnly) ? SFile::mRead : (SFile::mReadWrite|mDenyRead));
			//
			// Так как файл может быть заблокирован другим пользователем,
			// предпримем несколько попыток его открытия.
			// Исходим из предположения, что файл для записи открывается на малое время.
			//
			for(uint i = 0; i < 10; i++) {
				if(F_Obsolete.Open(pFileName, mode)) {
					break;
				}
				else
					SDelay(100);
			}
			if(!F_Obsolete.IsValid())
				Status |= stError;
		}
	}
	/*if(kind == cmdgrpcDesktop) {
		PPCommandMngr::GetDesksDir(XmlDirPath);
	}
	else if(kind == cmdgrpcMenu) {
		PPCommandMngr::GetMenuDir(XmlDirPath);
	}*/
}

PPCommandMngr::~PPCommandMngr()
{
}

int PPCommandMngr::IsValid_() const
{
	//return F_Obsolete.IsValid() ? 1 : PPSetErrorSLib();
	return (Status & stError) ? PPSetErrorSLib() : 1;
}

int PPCommandMngr::Load_Depricated(PPCommandGroup * pCmdGrp)
{
	int    ok = 1;
	if(!(CtrFlags & ctrfSkipObsolete)) {
		int    r = 0;
		Hdr    hdr;
		int64  fsz = 0;
		uint32 crc = 0;
		SBuffer buf;
		THROW_SL(F_Obsolete.IsValid());
		F_Obsolete.CalcSize(&fsz);
		if(fsz > 0) {
			THROW_SL(F_Obsolete.CalcCRC(sizeof(hdr), &crc));
			F_Obsolete.Seek(0, SEEK_SET);
			THROW_SL(F_Obsolete.Read(&hdr, sizeof(hdr)));
			THROW_PP_S(hdr.Signature==PPCS_SIGNATURE, PPERR_CMDFILSIGN, F_Obsolete.GetName());
			THROW_PP_S(hdr.Crc==crc, PPERR_CMDFILCRC, F_Obsolete.GetName());
			THROW_SL(F_Obsolete.Read(buf));
			THROW(pCmdGrp->Read_Depricated(buf, 0));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

//@erik v10.6.1 {
int PPCommandMngr::Save__2(const PPCommandGroup * pCmdGrp, const long rwFlag)
{
	int    ok = 1;
	SString path, guid_str;
	xmlTextWriter * p_xml_writer = 0;
	assert(pCmdGrp);
	THROW(pCmdGrp);
	if(rwFlag == PPCommandMngr::fRWByXml) {
		if(pCmdGrp->DbSymb.NotEmpty() || pCmdGrp->Type == cmdgrpcMenu) { // @v11.0.1 (pCmdGrp->Type == cmdgrpcMenu)
			if(pCmdGrp->Uuid.ToStr(S_GUID::fmtIDL, guid_str)) {
				path.Z().Cat(XmlDirPath).SetLastSlash().Cat(guid_str).DotCat("xml");
				p_xml_writer = xmlNewTextWriterFilename(path, 0);  // создание writerA
				xmlTextWriterSetIndent(p_xml_writer, 1);
				xmlTextWriterSetIndentTab(p_xml_writer);
				SXml::WDoc _doc(p_xml_writer, cpUTF8);
				THROW(pCmdGrp->Write2(p_xml_writer, rwFlag)); //@erik v10.6.6				
			}
		}
		else {
			const uint c = pCmdGrp->List.getCount();
			for(uint i = 0; i < c; i++) {
				const PPCommandItem * p_item = pCmdGrp->List.at(i);
				assert(p_item);
				THROW(p_item);
				if(p_item->IsKind(PPCommandItem::kGroup)) {
					//PPCommandGroup cg = *static_cast<const PPCommandGroup *>(p_item->Dup());
					const PPCommandGroup * p_cg = static_cast<const PPCommandGroup *>(p_item);
					if(p_cg->Uuid.ToStr(S_GUID::fmtIDL, guid_str)) {
						path.Z().Cat(XmlDirPath).SetLastSlash().Cat(guid_str).DotCat("xml");
						p_xml_writer = xmlNewTextWriterFilename(path, 0);  // создание writerA
						xmlTextWriterSetIndent(p_xml_writer, 1);
						xmlTextWriterSetIndentTab(p_xml_writer);
						SXml::WDoc _doc(p_xml_writer, cpUTF8);
						THROW(p_cg->Write2(p_xml_writer, rwFlag)); // @erik v10.6.6
					}
				}
				// @sobolev @v10.7.6 {
				else if(p_item->IsKind(PPCommandItem::kFolder)) {
					const PPCommandFolder * p_cf = static_cast<const PPCommandFolder *>(p_item);
					// @v11.0.0 SXml::WDoc _doc(p_xml_writer, cpUTF8);
					THROW(p_cf->Write2(p_xml_writer, rwFlag));
				}
				// } @sobolev @v10.7.6 
				xmlFreeTextWriter(p_xml_writer);
				p_xml_writer = 0;
			}
		}
	}
	CATCHZOK
	xmlFreeTextWriter(p_xml_writer);
	return ok;
}

int PPCommandMngr::Load__2(PPCommandGroup * pCmdGrp, const char * pDbSymb, const long rwFlag)
{
	int    ok = 1;
	xmlDoc * p_doc = 0;
	SString temp_buf, path;
	SString lock_path;
	xmlParserCtxt * p_xml_parser = 0;
	if(rwFlag == PPCommandMngr::fRWByXml) {
		path.Z().Cat(XmlDirPath).SetLastSlash().Cat("*.xml");
		SDirEntry de;
		// Конверация происходит, если папка с xml пуста
		uint count = 0;
		for(SDirec direc(path, 0); direc.Next(&de)>0;) {
			if(de.IsFile()) {
				count++;
				break; // Выяснили что есть по крайней мере один файл - можем идти дальше
			}
		}
		if(!count && !(CtrFlags & ctrfSkipObsolete)) {
			PPGetFileName(PPFILNAM_MENUDESKLOCK, temp_buf);
			lock_path = lock_path.Z().Cat(XmlDirPath).SetLastSlash().Cat(temp_buf);
			temp_buf.Z();
			const int  is_locked = BIN(fileExists(lock_path) && SFile::IsOpenedForWriting(lock_path));
			if(!is_locked) {
				SFile f(lock_path, SFile::mWrite);
				// @v10.7.6 f.Close();
				PPCommandGroup cg_from_bin;
				if(F_Obsolete.IsValid()) {
					int64  fsz = 0;
					if(F_Obsolete.CalcSize(&fsz)) {
						if(fsz > 0) {
							THROW(Load_Depricated(&cg_from_bin));
							const uint c = cg_from_bin.List.getCount();
							PPCommandGroup cg_pool;
							for(uint i = 0; i < c; i++) {
								PPCommandGroup * p_tmp_cg = 0;
								PPCommandItem * p_item = cg_from_bin.List.at(i);
								if(p_item) {
									//assert(p_item);
									//THROW(p_item);
									if(p_item->IsKind(PPCommandItem::kFolder)) { // Если так, то это меню. Значит его нужно обернуть в CommandGroup, задать GuiD и Type
										PPCommandFolder * p_menu = static_cast<PPCommandFolder *>(p_item->Dup());
										if(p_menu) {
											p_tmp_cg = new PPCommandGroup();
											p_tmp_cg->List = p_menu->List;
											p_tmp_cg->Name = p_menu->Name;
											p_tmp_cg->ID = p_menu->GetID();
											p_tmp_cg->Flags = p_menu->Flags;
											p_tmp_cg->Icon = p_menu->Icon;
											p_tmp_cg->GenerateGuid();
											p_tmp_cg->Type = cmdgrpcMenu;//PPCommandGroup::tMenu;
											p_tmp_cg->DbSymb = "undefined";
										}
									}
									else {
										p_tmp_cg = static_cast<PPCommandGroup *>(p_item);
										p_tmp_cg->Type = cmdgrpcDesktop;
									}
									if(p_tmp_cg)
										cg_pool.Add(-1, p_tmp_cg);
								}
							}
							THROW(Save__2(&cg_pool, rwFlag));
						}
					}
				}
				f.Close();
				SFile::Remove(lock_path);
			}			
		}
		//
		// Конец конвертации
		//
		{
			SString src_file_name;
			THROW(p_xml_parser = xmlNewParserCtxt()); // @v11.0.0 
			for(SDirec direc(path, 0); direc.Next(&de) > 0;) {
				if(de.IsFile()) {
					// @v11.0.0 PPCommandGroup * p_temp_command_group = new PPCommandGroup();
					PPCommandGroup temp_command_group;
					de.GetNameA(XmlDirPath, src_file_name);
					if(fileExists(src_file_name)) {
						// @v11.0.0 THROW(p_xml_parser = xmlNewParserCtxt());
						p_doc = xmlCtxtReadFile(p_xml_parser, src_file_name, 0, XML_PARSE_NOENT);
						if(p_doc) {
							xmlNode * p_root = xmlDocGetRootElement(p_doc);
							if(p_root && SXml::IsName(p_root, "CommandGroup")) {
								if(temp_command_group.Read2(p_root, rwFlag)) {
									if(isempty(pDbSymb) || temp_command_group.DbSymb.IsEqNC(pDbSymb) || temp_command_group.DbSymb.IsEqiAscii("undefined")) { // @v10.9.3
										const int addr = pCmdGrp->Add(-1, &temp_command_group);
										//p_temp_command_group = 0;
									}
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_DBINFO);
								}
							}
							xmlFreeDoc(p_doc);
							p_doc = 0;
						}
						else {
							PPSetLibXmlError(p_xml_parser);
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_DBINFO);
							// @v11.3.1 {
							{
								const SPathStruc ps(src_file_name);
								SString malformed_files_storage_path;
								(malformed_files_storage_path = XmlDirPath).SetLastSlash().Cat("malformed");
								if(::createDir(malformed_files_storage_path)) {
									SString malformed_file_name;
									(malformed_file_name = malformed_files_storage_path).SetLastSlash().Cat(ps.Nam).Dot().Cat(ps.Ext);
									long undup_serial = 0;
									while(fileExists(malformed_file_name)) {
										(malformed_file_name = malformed_files_storage_path).SetLastSlash().Cat(ps.Nam).CatChar('-').CatLongZ(++undup_serial, 4).Dot().Cat(ps.Ext);
									}
									if(SCopyFile(src_file_name, malformed_file_name, 0, FILE_SHARE_READ, 0)) {
										SFile::Remove(src_file_name);
									}
									{
										SString fmt_buf;
										SString msg_buf;
										PPLoadText(PPTXT_MALFORMEDCMDFILEMOVED, fmt_buf);
										(temp_buf = src_file_name).Space().Cat("-->").Space().Cat(malformed_file_name);
										PPLogMessage(PPFILNAM_ERR_LOG, msg_buf.Printf(fmt_buf, temp_buf.cptr()), LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_DBINFO);
									}
								}
							}
							// } @v11.3.1 
						}
					}
					// @v11.0.0 xmlFreeParserCtxt(p_xml_parser); 
					// @v11.0.0 p_xml_parser = 0;
					// @v11.0.0 delete p_temp_command_group;
				}
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
		if(fileExists(lock_path)) // @v10.9.3 @fix !fileExists-->fileExists
			SFile::Remove(lock_path);
		ok = 0;
	ENDCATCH
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_xml_parser);
	return ok;
}

int PPCommandMngr::SaveFromAllTo(const long rwFlag)
{
	int    ok = 1;
	PPCommandGroup cg_from_xml;
	PPCommandGroup cg_from_bin;
	PPCommandGroup cg_final;
	const PPCommandItem * p_item = 0;
	THROW(Load_Depricated(&cg_from_bin));
	THROW(Load__2(&cg_from_xml, 0, PPCommandMngr::fRWByXml));
	for(uint i = 0; p_item = cg_from_xml.Next(&i);) {
		if(cg_final.Add(-1, p_item) > 0) {
			p_item = 0;
		}
	}
	for(uint i = 0; (p_item = cg_from_bin.Next(&i)) != 0;) {
		if(cg_final.Add(-1, p_item)>0) {
			p_item = 0;
		}
	}
	THROW(Save__2(&cg_final, rwFlag));
	CATCHZOK;
	return ok;
}

int PPCommandMngr::ConvertDesktopTo(const long rwFlag)
{
	int    ok = 1;
	SString temp_buf;
	if(!(CtrFlags & ctrfSkipObsolete) && F_Obsolete.IsValid()) {
		int64  fsz = 0;
		if(F_Obsolete.CalcSize(&fsz)) {
			if(fsz > 0) {
				THROW(SaveFromAllTo(rwFlag));
				THROW_SL(F_Obsolete.Close());
				THROW(PPGetPath(PPPATH_BIN, temp_buf));
				{
					const SString old_name = temp_buf.SetLastSlash().Cat("ppdesk.bin");
					temp_buf.SetLastSlash().Cat("ppdesk").Cat(getcurdate_(), DATF_DMY).Cat(".bin");
					THROW_SL(F_Obsolete.Rename(old_name, temp_buf));
				}
			}
		}
	}
	CATCHZOK;
	return ok;
}

int PPCommandMngr::GetMaxEntryID(long * pMaxId)
{
	int    ok = -1;
	long   max_id = 0;
	PPCommandGroup group_list;
	if(Load__2(&group_list, 0, PPCommandMngr::fRWByXml) > 0) {
		const PPCommandItem * p_item = 0;
		for(uint i = 0; (p_item = group_list.Next(&i)) != 0;) {
			if(p_item->IsKind(PPCommandItem::kGroup)) {
				const PPCommandGroup * p_group = static_cast<const PPCommandGroup *>(p_item);
				SETMAX(max_id, p_group->GetID());
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pMaxId, max_id);
	return ok;
}

int PPCommandMngr::DeleteGroupByUuid(PPCommandGroupCategory kind, const S_GUID & rUuid)
{
	int    ok = -1; // @v10.8.9 @fix ok;-->ok=-1;
	SFile  file;
	if(!!rUuid) {
		SString path;
		if(kind == cmdgrpcDesktop)
			PPCommandMngr::GetDesksDir(path);
		else if(kind == cmdgrpcMenu)
			PPCommandMngr::GetMenuDir(path);
		if(path.NotEmpty()) {
			SString temp_buf(path);
			path.SetLastSlash().Cat(rUuid, S_GUID::fmtIDL|S_GUID::fmtLower).DotCat("xml");
			temp_buf.SetLastSlash().Cat(rUuid, S_GUID::fmtIDL|S_GUID::fmtLower).Cat("_deleted").DotCat("txt");
			ok = file.Rename(path, temp_buf);
		}
	}
	return ok;
}

int PPCommandMngr::GetDesksDir(SString & rPath)
{
	PPGetFilePath(PPPATH_WORKSPACE, "desktop", rPath);  // получаем путь к workspace
	::createDir(rPath);
	return 1;
}

int PPCommandMngr::GetMenuDir(SString & rPath)
{
	PPGetFilePath(PPPATH_WORKSPACE, "menu", rPath);  // получаем путь к workspace
	::createDir(rPath);
	return 1;
}

PPCommandHandler::PPCommandHandler(const PPCommandDescr * pDescr)
{
	if(!RVALUEPTR(D, pDescr))
		MEMSZERO(D);
}

PPCommandHandler::~PPCommandHandler()
{
}

int PPCommandHandler::EditParam(SBuffer * pParam, long, void * extraPtr)
{
	return -1;
}

int PPCommandHandler::Run(SBuffer * pParam, long, void * extraPtr)
{
	return -1;
}
//
//
//
#define ICON_COMMAND_BIAS 10000L

int EditPPViewFilt(int viewID, SBuffer * pParam, void * extraPtr)
{
	int    ok = 1;
	size_t sav_offs = 0;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	THROW_INVARG(pParam);
	sav_offs = pParam->GetRdOffs();
	THROW(PPView::CreateInstance(viewID, &p_view) > 0);
	{
		THROW(PPView::ReadFiltPtr(*pParam, &p_filt));
		SETIFZ(p_filt, p_view->CreateFilt(extraPtr));
		if((ok = p_view->EditBaseFilt(p_filt)) > 0) {
			THROW(p_view->WriteFiltPtr(pParam->Z(), p_filt));
		}
		else
			pParam->SetRdOffs(sav_offs);
	}
	CATCH
		CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
		ok = 0;
	ENDCATCH
	ZDELETE(p_filt);
	ZDELETE(p_view);
	return ok;
}

int RunPPViewCmd(int viewID, SBuffer * pParam, long menuCm, long cmdID, void * extraPtr)
{
	int    ok = 1;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	if((menuCm || cmdID) && APPL) {
		static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : menuCm;
		if(pParam)
			PPView::ReadFiltPtr(*pParam, &p_filt);
		THROW(p_view->Execute(viewID, p_filt, 1, extraPtr));
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		ok = 1;
	}
	CATCHZOK
	ZDELETE(p_filt);
	ZDELETE(p_view);
	return ok;
}

class CMD_HDL_CLS(DEFAULT) : public PPCommandHandler {
public:
	CMD_HDL_CLS(DEFAULT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(D.ViewId) {
			if(D.ViewId == PPVIEW_BILL) {
				BillFilt::FiltExtraParam p(1, static_cast<BrowseBillsType>(D.FiltExtId));
				ok = EditPPViewFilt(D.ViewId, pParam, &p);
			}
			else
				ok = EditPPViewFilt(D.ViewId, pParam, reinterpret_cast<void *>(D.FiltExtId));
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(D.ViewId) {
			if(D.ViewId == PPVIEW_BILL) {
				BillFilt::FiltExtraParam p(1, static_cast<BrowseBillsType>(D.FiltExtId));
				THROW(PPCheckDatabaseChain());
				ok = RunPPViewCmd(D.ViewId, pParam, D.MenuCm, cmdID, &p);
			}
			else
				ok = RunPPViewCmd(D.ViewId, pParam, D.MenuCm, cmdID, reinterpret_cast<void *>(D.FiltExtId));
		}
		else if(D.MenuCm && APPL) {
			static_cast<PPApp *>(APPL)->processCommand(static_cast<uint>(D.MenuCm));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(DEFAULT);
//
//
//
class CMD_HDL_CLS(ADDPERSON) : public PPCommandHandler {
public:
	CMD_HDL_CLS(ADDPERSON)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		return -1;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		if(D.MenuCm && APPL) {
			int    ok = -1, r = cmCancel;
			PPID   id = 0;
			SString name;
			PPObjPerson psn_obj;
			PPInputStringDialogParam isd_param;
			PsnSelAnalogDialog * dlg = 0;
			THROW_INVARG(pParam);
			sav_offs = pParam->GetRdOffs();
			pParam->Read(name);
			static_cast<PPApp *>(APPL)->LastCmd = D.MenuCm;
			GetObjectTitle(PPOBJ_PERSON, isd_param.Title);
			isd_param.InputTitle = isd_param.Title;
			if(InputStringDialog(&isd_param, name) > 0) {
				THROW(pParam->Z().Write(name));
				if(CheckDialogPtrErr(&(dlg = new PsnSelAnalogDialog(&psn_obj)))) {
					dlg->setSrchString(name);
					if(ExecView(dlg) == cmOK)
						dlg->getResult(&id);
					if(!id) {
						PPObjPerson::EditBlock eb;
						eb.InitStatusID = PPPRS_LEGAL;
						eb.Name = name;
						r = psn_obj.Edit_(&id, eb);
					}
					else
						r = cmOK;
				}
			}
			else
				pParam->SetRdOffs(sav_offs);
			delete dlg;
			if(r == cmOK) {
				PersonFilt filt;
				filt.PersonID = id;
				ViewPerson(&filt);
				ok = 1;
			}
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(ADDPERSON);
//
//
//
// @todo Заменить ReadPrjTaskPacket и WritePrjTaskPacket на PPObjPrjTask::SerializePacket (с соблюдением обратной совместимости)
static int ReadPrjTaskPacket(PPPrjTaskPacket * pPack, SBuffer & rBuf, long)
{
	int    ok = 1;
	SString code, descr, memo;
	THROW_INVARG(pPack);
	THROW_SL(rBuf.Read(pPack->Rec.ID));
	THROW_SL(rBuf.Read(pPack->Rec.ProjectID));
	THROW_SL(rBuf.Read(pPack->Rec.Kind));
	THROW_SL(rBuf.Read(code));
	THROW_SL(rBuf.Read(pPack->Rec.CreatorID));
	THROW_SL(rBuf.Read(pPack->Rec.GroupID));
	THROW_SL(rBuf.Read(pPack->Rec.EmployerID));
	THROW_SL(rBuf.Read(pPack->Rec.ClientID));
	THROW_SL(rBuf.Read(pPack->Rec.TemplateID));
	THROW_SL(rBuf.Read(pPack->Rec.Dt));
	THROW_SL(rBuf.Read(pPack->Rec.Tm));
	THROW_SL(rBuf.Read(pPack->Rec.StartDt));
	THROW_SL(rBuf.Read(pPack->Rec.StartTm));
	THROW_SL(rBuf.Read(pPack->Rec.EstFinishDt));
	THROW_SL(rBuf.Read(pPack->Rec.EstFinishTm));
	THROW_SL(rBuf.Read(pPack->Rec.FinishDt));
	THROW_SL(rBuf.Read(pPack->Rec.FinishTm));
	THROW_SL(rBuf.Read(pPack->Rec.Priority));
	THROW_SL(rBuf.Read(pPack->Rec.Status));
	THROW_SL(rBuf.Read(pPack->Rec.DrPrd));
	THROW_SL(rBuf.Read(pPack->Rec.DrKind));
	THROW_SL(rBuf.Read(pPack->Rec.DrDetail));
	THROW_SL(rBuf.Read(pPack->Rec.Flags));
	THROW_SL(rBuf.Read(pPack->Rec.DlvrAddrID));
	THROW_SL(rBuf.Read(pPack->Rec.LinkTaskID));
	THROW_SL(rBuf.Read(pPack->Rec.Amount));
	THROW_SL(rBuf.Read(pPack->Rec.OpenCount));
	THROW_SL(rBuf.Read(pPack->Rec.BillArID));
	THROW_SL(rBuf.Read(descr));
	THROW_SL(rBuf.Read(memo));
	code.CopyTo(pPack->Rec.Code, sizeof(pPack->Rec.Code));
	pPack->SDescr = descr;
	pPack->SMemo = memo;
	CATCHZOK
	return ok;
}

static int WritePrjTaskPacket(const PPPrjTaskPacket * pPack, SBuffer & rBuf, long)
{
	int    ok = 1;
	SString code, descr, memo;
	THROW_INVARG(pPack);
	code.CopyFrom(pPack->Rec.Code);
	descr = pPack->SDescr;
	memo = pPack->SMemo;
	THROW_SL(rBuf.Write(pPack->Rec.ID));
	THROW_SL(rBuf.Write(pPack->Rec.ProjectID));
	THROW_SL(rBuf.Write(pPack->Rec.Kind));
	THROW_SL(rBuf.Write(code));
	THROW_SL(rBuf.Write(pPack->Rec.CreatorID));
	THROW_SL(rBuf.Write(pPack->Rec.GroupID));
	THROW_SL(rBuf.Write(pPack->Rec.EmployerID));
	THROW_SL(rBuf.Write(pPack->Rec.ClientID));
	THROW_SL(rBuf.Write(pPack->Rec.TemplateID));
	THROW_SL(rBuf.Write(pPack->Rec.Dt));
	THROW_SL(rBuf.Write(pPack->Rec.Tm));
	THROW_SL(rBuf.Write(pPack->Rec.StartDt));
	THROW_SL(rBuf.Write(pPack->Rec.StartTm));
	THROW_SL(rBuf.Write(pPack->Rec.EstFinishDt));
	THROW_SL(rBuf.Write(pPack->Rec.EstFinishTm));
	THROW_SL(rBuf.Write(pPack->Rec.FinishDt));
	THROW_SL(rBuf.Write(pPack->Rec.FinishTm));
	THROW_SL(rBuf.Write(pPack->Rec.Priority));
	THROW_SL(rBuf.Write(pPack->Rec.Status));
	THROW_SL(rBuf.Write(pPack->Rec.DrPrd));
	THROW_SL(rBuf.Write(pPack->Rec.DrKind));
	THROW_SL(rBuf.Write(pPack->Rec.DrDetail));
	THROW_SL(rBuf.Write(pPack->Rec.Flags));
	THROW_SL(rBuf.Write(pPack->Rec.DlvrAddrID));
	THROW_SL(rBuf.Write(pPack->Rec.LinkTaskID));
	THROW_SL(rBuf.Write(pPack->Rec.Amount));
	THROW_SL(rBuf.Write(pPack->Rec.OpenCount));
	THROW_SL(rBuf.Write(pPack->Rec.BillArID));
	THROW_SL(rBuf.Write(descr));
	THROW_SL(rBuf.Write(memo));
	CATCHZOK
	return ok;
}

class CMD_HDL_CLS(ADDTASK) : public PPCommandHandler {
public:
	CMD_HDL_CLS(ADDTASK)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		PPPrjTaskPacket pack;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		// @v10.6.4 MEMSZERO(rec);
		if(pParam->GetAvailableSize() == 0)
			TodoObj.InitPacket(&pack, TODOKIND_TASK, 0, 0, 0, 0);
		else
			THROW(ReadPrjTaskPacket(&pack, *pParam, 0));
		getcurdatetime(&pack.Rec.Dt, &pack.Rec.Tm);
		if(TodoObj.EditDialog(&pack) > 0) {
			THROW(WritePrjTaskPacket(&pack, pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if((D.MenuCm || cmdID)&& APPL) {
			PPID   id = 0;
			SBuffer param;
			THROW_INVARG(pParam);
			THROW(TodoObj.CheckRightsModByID(&id));
			static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
			param = *pParam;
			if(EditParam(&param, cmdID, extraPtr) > 0) {
				PPPrjTaskPacket pack;
				THROW(ReadPrjTaskPacket(&pack, param, 0));
				THROW(ok = TodoObj.PutPacket(&id, &pack, 1));
			}
		}
		CATCHZOK
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		return ok;
	}
private:
	PPObjPrjTask TodoObj;
};

IMPLEMENT_CMD_HDL_FACTORY(ADDTASK);
//
//
//
class CMD_HDL_CLS(ADDPERSONEVENT) : public PPCommandHandler {
public:
	CMD_HDL_CLS(ADDPERSONEVENT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, valid_data = 0;
		size_t sav_offs = 0;
		AddPersonEventFilt filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(filt.Edit() > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr);
private:
	int    RunBySymb(SBuffer * pSymb);
	PPObjPersonEvent PsnEvObj;
};

class SelectPersonByCodeDialog : public TDialog {
public:
	struct Rec {
		Rec() : PrmrPsnID(0), ScndPsnID(0), SCardID(0)
		{
		}
		PPID   PrmrPsnID;
		PPID   ScndPsnID;
		PPID   SCardID;
		SCardTbl::Rec Sc;
	};
	SelectPersonByCodeDialog(const char * pSubTitle, PPPersonKind * pPsnKindRec, PPPersonKind * pPsnScndKindRec, const char * pInputPrompt) :
		TDialog(DLG_SELPERSONC)
	{
		setStaticText(CTL_SELPERSONC_SUBTITLE, pSubTitle);
		RVALUEPTR(PsnKindRec, pPsnKindRec);
		RVALUEPTR(PsnScndKindRec, pPsnScndKindRec);
		if(pInputPrompt)
			setLabelText(CTL_SELPERSONC_CODEINP, pInputPrompt);
		showCtrl(CTLSEL_SELPERSONC_SCARD, 0); // @v9.1.3
	}
	int    setDTS(const Rec *);
	int    getDTS(Rec *);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	Rec    Data;
	PPPersonKind PsnKindRec;
	PPPersonKind PsnScndKindRec;
	PPObjSCard ScObj;
	PPObjPerson PsnObj;
};

IMPL_HANDLE_EVENT(SelectPersonByCodeDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmCBSelected) && event.isCtlEvent(CTLSEL_SELPERSONC_PRSN)) {
		SetupCtrls();
		clearEvent(event);
	}
}

void SelectPersonByCodeDialog::SetupCtrls()
{
	SString code;
	PPID   psn_id = getCtrlLong(CTLSEL_SELPERSONC_PRSN);
	if(psn_id) {
		RegisterTbl::Rec reg_rec;
		// @v10.6.4 MEMSZERO(reg_rec);
		PsnObj.GetRegister(psn_id, PsnKindRec.CodeRegTypeID, &reg_rec);
		code = reg_rec.Num;
		showCtrl(CTLSEL_SELPERSONC_SCARD, 1); // @v9.1.3
		PPObjSCard::Filt sc_filt;
		sc_filt.OwnerID = psn_id;
		SetupPPObjCombo(this, CTLSEL_SELPERSONC_SCARD, PPOBJ_SCARD, Data.SCardID, 0, &sc_filt);
	}
	else
		showCtrl(CTLSEL_SELPERSONC_SCARD, 0); // @v9.1.3
	disableCtrl(CTL_SELPERSONC_CODEINP, psn_id);
	setCtrlString(CTL_SELPERSONC_CODEINP, code);
}

int SelectPersonByCodeDialog::setDTS(const Rec * pData)
{
	int    ok = 1;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	SetupPersonCombo(this, CTLSEL_SELPERSONC_PRSN, Data.PrmrPsnID, 0, PsnKindRec.ID, 0);
	if(PsnScndKindRec.ID)
		SetupPersonCombo(this, CTLSEL_SELPERSONC_PRSNSC, Data.ScndPsnID, 0, PsnScndKindRec.ID, 0);
	else
		disableCtrl(CTLSEL_SELPERSONC_PRSNSC, 1);
	SetupCtrls();
	return ok;
}

int SelectPersonByCodeDialog::getDTS(Rec * pData)
{
	int    ok = 1;
	PPIDArray psn_list;
	SCardTbl::Rec  sc_rec;
	MEMSZERO(pData->Sc);
	getCtrlData(CTLSEL_SELPERSONC_PRSN, &Data.PrmrPsnID);
	if(PsnScndKindRec.ID)
		getCtrlData(CTLSEL_SELPERSONC_PRSNSC, &Data.ScndPsnID);
	if(!Data.PrmrPsnID) {
		SString code;
		getCtrlString(CTL_SELPERSONC_CODEINP, code);
		if(code.Len()) {
			if(PsnKindRec.CodeRegTypeID && PsnObj.GetListByRegNumber(PsnKindRec.CodeRegTypeID, PsnKindRec.ID, code, psn_list) > 0)
				Data.PrmrPsnID = psn_list.getCount() ? psn_list.at(0) : 0;
			else {
				PersonTbl::Rec psn_rec;
				// @v10.6.4 MEMSZERO(psn_rec);
				// @v10.6.4 MEMSZERO(sc_rec);
				if(ScObj.P_Tbl->SearchCode(0, code, &sc_rec) > 0 && PsnObj.P_Tbl->IsBelongToKind(sc_rec.PersonID, PsnKindRec.ID) > 0) {
					Data.PrmrPsnID = sc_rec.PersonID;
					Data.Sc = sc_rec;
				}
			}
		}
		THROW_PP_S(Data.PrmrPsnID, PPERR_PERSONNOTFOUND, code);
	}
	else {
        getCtrlData(CTLSEL_SELPERSONC_SCARD, &Data.SCardID);
        if(Data.SCardID && ScObj.Search(Data.SCardID, &sc_rec) > 0 && sc_rec.PersonID == Data.PrmrPsnID) {
			Data.Sc = sc_rec;
        }
        else {
			Data.SCardID = 0;
        }
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOK
	return ok;
}

int CMD_HDL_CLS(ADDPERSONEVENT)::RunBySymb(SBuffer * pParam)
{
	int    ok = -1;
	SelectPersonByCodeDialog * p_dlg = 0;
	if(D.MenuCm && pParam) {
		SString title, code, symb, prompt;
		PPPsnOpKind  pop_rec;
		PPPersonKind pk_rec, scnd_pk_rec;
		PPPsnOpKindPacket pop_pack;
		PPRegisterTypePacket regtyp_pack;
		PPObjPersonKind pk_obj;
		PPObjPsnOpKind  pop_obj;
		PPObjPersonEvent pev_obj;
		PPObjPerson psn_obj;
		PPObjRegisterType obj_regt;
		PPObjSCard sc_obj;
		SelectPersonByCodeDialog::Rec psn_data;
		static_cast<PPApp *>(APPL)->LastCmd = D.MenuCm;
		// @v10.6.6 @ctr MEMSZERO(pop_rec);
		// @v10.6.6 @ctr MEMSZERO(pk_rec);
		// @v10.6.6 @ctr MEMSZERO(scnd_pk_rec);
		CALLPTRMEMB(pParam, Read(symb));
		THROW(pop_obj.SearchBySymb(symb, 0, &pop_rec) > 0);
		THROW(pop_obj.GetPacket(pop_rec.ID, &pop_pack) > 0);
		THROW(pk_obj.Search(pop_pack.PCPrmr.PersonKindID, &pk_rec) > 0);
		if(pop_pack.PCScnd.PersonKindID)
			THROW(pk_obj.Search(pop_pack.PCScnd.PersonKindID, &scnd_pk_rec) > 0);
		if(pop_pack.PCScnd.DefaultID)
			psn_data.ScndPsnID = pop_pack.PCScnd.DefaultID;
		if(pk_rec.CodeRegTypeID && obj_regt.GetPacket(pk_rec.CodeRegTypeID, &regtyp_pack) > 0)
			prompt = regtyp_pack.Rec.Name;
		else
			PPLoadText(PPTXT_SELPERSONBYSCARD, prompt);
		THROW(CheckDialogPtr(&(p_dlg = new SelectPersonByCodeDialog(pop_rec.Name, &pk_rec, &scnd_pk_rec, prompt))));
		THROW(p_dlg->setDTS(&psn_data));
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			int    r = 1;
			int    disable_op = 0;
			PPID   prmr_psn_id = 0, scnd_psn_id = 0;
			if(p_dlg->getDTS(&psn_data) > 0) {
				PPPsnEventPacket pack;
				prmr_psn_id = psn_data.PrmrPsnID;
				scnd_psn_id = psn_data.ScndPsnID ? psn_data.ScndPsnID : pop_pack.PCScnd.DefaultID;
				THROW(pev_obj.InitPacket(&pack, pop_rec.ID, prmr_psn_id));
				pack.Rec.SecondID   = scnd_psn_id;
				pack.Rec.LocationID = LConfig.Location;
				pack.Rec.PrmrSCardID = psn_data.Sc.ID;
				if(psn_obj.GetConfig().Flags & PPPersonConfig::fShowPsnImageAfterCmdAssoc) {
					uint   pos = 0;
					SString info, warn, buf, reg_buf;
					RegisterTbl::Rec reg_rec;
					PPPersonPacket psn_pack;
					// @v10.6.4 MEMSZERO(reg_rec);
					THROW(psn_obj.GetPacket(prmr_psn_id, &psn_pack, 0));
					info.Cat(psn_pack.Rec.Name);
					PPLoadString("validuntil", buf);
					if(psn_data.Sc.ID) {
						PPLoadString("card", symb);
						prompt.Z().Cat(symb).CatDiv(':', 2).Cat(psn_data.Sc.Code);
						{
							const int scst = PPObjSCard::GetSeriesType(psn_data.Sc.SeriesID);
							if(oneof2(scst, scstCredit, scstBonus)) {
								double sc_rest = 0.0;
								sc_obj.P_Tbl->GetRest(psn_data.Sc.ID, MAXDATE, &sc_rest);
								PPLoadString("rest", symb);
								prompt.Space().Cat(symb).CatDiv(':', 2).Cat(sc_rest, MKSFMTD(0, 2, 0));
							}
						}
						if(psn_data.Sc.Expiry) {
							//
							// Текст сообщения о сроке годности карты
							//
							prompt.CatDiv(';', 2).Cat(PPLoadTextS(PPTXT_SCARD_EXPIRY, symb)).Space().Cat(psn_data.Sc.Expiry);
						}
						if(psn_data.Sc.UsageTmStart || psn_data.Sc.UsageTmEnd) {
							//
							// Текст сообщения о времени действия карты
							//
							prompt.CatDiv(';', 2).Cat(PPLoadTextS(PPTXT_SCARD_TIMEPERIOD, symb)).Space();
							if(psn_data.Sc.UsageTmStart)
								prompt.Cat(psn_data.Sc.UsageTmStart);
							prompt.CatCharN('.', 2);
							if(psn_data.Sc.UsageTmEnd)
								prompt.Cat(psn_data.Sc.UsageTmEnd);
						}
						if(!sc_obj.CheckRestrictions(&psn_data.Sc, 0, getcurdatetime_())) {
							PPGetLastErrorMessage(1, warn);
							disable_op = 1;
						}
						info.CatDiv(';', 2).Cat(prompt);
					}
					else if(psn_pack.GetRegister(pk_rec.CodeRegTypeID, &pos) > 0) {
						reg_rec = psn_pack.Regs.at(pos-1);
						psn_obj.RegObj.Format(reg_rec.ID, 0, reg_buf);
						info.CatDivIfNotEmpty(';', 2).Cat(reg_buf);
						if(reg_rec.Expiry && reg_rec.Expiry < getcurdate_()) {
							//
							// Текст сообщения о том, что срок действия регистра истек
							//
							GetRegisterTypeName(reg_rec.RegTypeID, reg_buf);
							warn.Printf(PPLoadTextS(PPTXT_PSNREGEXPIRED, buf), reg_buf.cptr());
							disable_op = 1;
						}
					}
					if(!pev_obj.CheckRestrictions(&pack, psn_pack.Rec.ID, pack.Rec.PrmrSCardID, &pop_pack.PCPrmr)) {
						PPGetLastErrorMessage(1, warn);
						disable_op = 1;
					}
					if(warn.IsEmpty()) {
						//
						// Если все предыдущие проверки прошли успешно, то проверяем
						// все регистры персоналии на предмет истечения срока действия.
						//
						for(uint i = 0; i < psn_pack.Regs.getCount(); i++) {
							const RegisterTbl::Rec & r_rec = psn_pack.Regs.at(i);
							PPRegisterType rt_rec;
							if(obj_regt.Fetch(r_rec.RegTypeID, &rt_rec) > 0 && rt_rec.Flags & REGTF_WARNEXPIRY) {
								if(r_rec.Expiry && r_rec.Expiry < getcurdate_()) {
									warn.Printf(PPLoadTextS(PPTXT_PSNREGEXPIRED, buf), rt_rec.Name);
									break;
								}
							}
						}
					}
					/*
					if(warn.Empty()) {
						LTIME cur_tm;
						getcurtime(&cur_tm);
						if(!psn_data.UsageTm.Check(cur_tm)) {
							SString low_tm, upp_tm;
							PPLoadText(PPTXT_USAGETM, buf);
							low_tm.Cat(psn_data.UsageTm.low, TIMF_HM);
							upp_tm.Cat(psn_data.UsageTm.upp, TIMF_HM);
							warn.Printf(buf, (const char *)low_tm, (const char *)upp_tm);
						}
					}
					*/
					{
						PPObjTag tag_obj;
						PPObjectTag tag_rec;
						if(pop_pack.PCPrmr.RestrictTagID && tag_obj.Fetch(pop_pack.PCPrmr.RestrictTagID, &tag_rec) > 0) {
							const ObjTagItem * p_tag = psn_pack.TagL.GetItem_ForceEmpty(pop_pack.PCPrmr.RestrictTagID);
							if(p_tag) {
								p_tag->GetStr(prompt);
								info.CR().CatEq(tag_rec.Name, prompt);
							}
						}
						else {
							StrAssocArray warn_list, info_list;
							if(tag_obj.GetWarnList(&psn_pack.TagL, &warn_list, &info_list) > 0)
								warn = warn_list.Get(0).Txt;
							else if(info_list.getCount() && tag_obj.Fetch(info_list.Get(0).Id, &tag_rec) > 0)
								info.CR().CatEq(tag_rec.Name, info_list.Get(0).Txt);
						}
					}
					psn_pack.LinkFiles.Init(PPOBJ_PERSON);
					if(psn_pack.Rec.Flags & PSNF_HASIMAGES)
						psn_pack.LinkFiles.Load(psn_pack.Rec.ID, 0L);
					psn_pack.LinkFiles.At(0, buf);
					r = ViewImageInfo(buf, info, warn);
				}
				if(r > 0) {
					if(!disable_op) {
						PPID   ev_id = 0;
						if(pev_obj.PutPacket(&ev_id, &pack, 1))
							ok = 1;
					}
					else
						PPSetError(PPERR_OPERDISABLED);
					if(ok < 0)
						PPError();
				}
			}
			else
				PPError();
		}
	}
	CATCHZOK
	delete p_dlg;
	static_cast<PPApp *>(APPL)->LastCmd = 0;
	return ok;
}

int CMD_HDL_CLS(ADDPERSONEVENT)::Run(SBuffer * pParam, long cmdID, void * extraPtr)
{
	int    ok = -1;
	int    interactive_level = 2;
	PsnEventDialog * p_dlg = 0;
	if((D.MenuCm || cmdID) && APPL) {
		if(extraPtr)
			ok = RunBySymb(static_cast<SBuffer *>(extraPtr));
		else {
			PPID   op_id = 0;
			PPObjPsnOpKind pop_obj;
			PPPsnEventPacket pack;

			SBuffer sbuf;
			AddPersonEventFilt filt;
			static_cast<PPApp *>(APPL)->LastCmd = cmdID ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
			if(pParam)
				filt.Read(*pParam, 0);
			interactive_level = filt.InteractiveLevel;
			THROW(PsnEvObj.InitPacket(&pack, filt, 1));
			{
				PPID   id = 0;
				if(interactive_level < 2) {
					THROW(ok = PsnEvObj.PutPacket(&id, &pack, 1));
					if(interactive_level == 1 && filt.PrmrSCardCode) {
						ViewPersonInfoBySCard(filt.PrmrSCardCode);
					}
					ok = 1;
				}
				else {
					int    r = 0;
					int    valid_data = 0;
					LDATE  dt = ZERODATE;
					PsnEventDialog::Param param;
					PsnEventDialog::GetParam(pack.Rec.OpID, &param);
					THROW(CheckDialogPtr(&(p_dlg = new PsnEventDialog(&param, &PsnEvObj))));
					getcurdatetime(&dt, &pack.Rec.Tm);
					pack.Rec.Dt = (pack.Rec.Dt == ZERODATE) ? dt : pack.Rec.Dt;
					if(p_dlg->setDTS(&pack) > 0) {
						while(!valid_data && ExecView(p_dlg) == cmOK) {
							if(p_dlg->getDTS(&pack))
								valid_data = r = 1;
							else
								r = PPErrorZ();
						}
						if(r > 0) {
							THROW(ok = PsnEvObj.PutPacket(&id, &pack, 1));
							ok = 1;
						}
					}
					else
						PPError();
				}
			}
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
	}
	CATCH
		if(interactive_level > 1) {
			PPError();
		}
		else if(interactive_level == 1) {
			PPErrorTooltip(-1, 0);
		}
		ok = 0;
	ENDCATCH
	delete p_dlg;
	return ok;
}

IMPLEMENT_CMD_HDL_FACTORY(ADDPERSONEVENT);
//
//
//
class CashNodeFiltDialog : public TDialog {
public:
	CashNodeFiltDialog() : TDialog(DLG_CASHPANEFLT), PrevCashNodeID(0)
	{
	}
	int    setDTS(const CashNodePaneFilt * pData);
	int    getDTS(CashNodePaneFilt * pData);
private:
	DECL_HANDLE_EVENT;
	int    SetupCommands(PPID cashNodeID, long commandID);
	long   ConvertCommand(PPID cashNodeID, long cmd, int toDlgCmd);

	PPID   PrevCashNodeID;
	CashNodePaneFilt Data;
	PPObjCashNode CashNObj;
};

IMPL_HANDLE_EVENT(CashNodeFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_CASHPANEFLT_CNODE)) {
		PPID   cash_node_id = getCtrlLong(CTLSEL_CASHPANEFLT_CNODE);
		long   cmd_id = getCtrlLong(CTLSEL_CASHPANEFLT_CMD);
		SetupCommands(cash_node_id, cmd_id);
		clearEvent(event);
	}
}

int CashNodeFiltDialog::SetupCommands(PPID cashNodeID, long commandID)
{
	if(!PrevCashNodeID || PrevCashNodeID != cashNodeID) {
		PPCashNode cnrec, prev_cnrec;
		// @v10.7.9 @ctr MEMSZERO(cnrec);
		// @v10.7.9 @ctr MEMSZERO(prev_cnrec);
		if(cashNodeID && CashNObj.Search(cashNodeID, &cnrec) > 0) {
			SString commands;
			if(PrevCashNodeID)
				CashNObj.Search(PrevCashNodeID, &prev_cnrec);
			if((prev_cnrec.Flags & (CASHF_SYNC|CASHF_ASYNC)) != (cnrec.Flags & (CASHF_SYNC|CASHF_ASYNC))) {
				commandID = (PrevCashNodeID) ? 0 : commandID;
				SetupStringCombo(this, CTLSEL_CASHPANEFLT_CMD, (cnrec.Flags & CASHF_SYNC) ? PPTXT_SYNCCASHNODECOMMANDS : PPTXT_ASYNCCASHNODECOMMANDS, 0);
			}
		}
		else
			commandID = 0;
		setCtrlData(CTLSEL_CASHPANEFLT_CMD, &commandID);
		disableCtrl(CTLSEL_CASHPANEFLT_CMD, cashNodeID == 0);
	}
	PrevCashNodeID = cashNodeID;
	return 1;
}

long CashNodeFiltDialog::ConvertCommand(PPID cashNodeID, long cmd, int toDlgCmd)
{
	uint   out_cmd = 0;
	if(cmd && cashNodeID) {
		PPCashNode cn_rec;
		// @v10.7.9 @ctr MEMSZERO(cn_rec);
		if(CashNObj.Search(cashNodeID, &cn_rec) > 0) {
			static const uint async_cmds[] = { cmCSOpen, cmACSUpdate, cmCSClose, cmCSViewCheckList,  cmACSViewExcess };
			static const uint sync_cmds[] =  { cmCSOpen, cmCSClose, cmCSViewCheckList, cmSCSLock, cmSCSUnlock, cmSCSXReport, cmSCSZReportCopy, cmSCSIncasso };
			const  int  is_async = BIN(cn_rec.Flags & CASHF_ASYNC);
			const  uint count = is_async ? SIZEOFARRAY(async_cmds) : SIZEOFARRAY(sync_cmds);
			if(toDlgCmd) {
				cmd--;
				out_cmd = (cmd >= 0 && cmd < static_cast<long>(count)) ? ((is_async) ? async_cmds[cmd] : sync_cmds[cmd]) : 0;
			}
			else {
				for(uint i = 0; !out_cmd && i < count; i++)
					if(is_async && async_cmds[i] == cmd || (is_async == 0 && sync_cmds[i] == cmd))
						out_cmd = i + 1;
			}
		}
	}
	return out_cmd;
}

int CashNodeFiltDialog::setDTS(const CashNodePaneFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	long   cmd = ConvertCommand(Data.CashNodeID, Data.CommandID, 0);
	SetupPPObjCombo(this, CTLSEL_CASHPANEFLT_CNODE, PPOBJ_CASHNODE, Data.CashNodeID, 0);
	SetupCommands(Data.CashNodeID, cmd);
	return 1;
}

int CashNodeFiltDialog::getDTS(CashNodePaneFilt * pData)
{
	Data.CashNodeID = getCtrlLong(CTLSEL_CASHPANEFLT_CNODE);
	long   cmd = getCtrlLong(CTLSEL_CASHPANEFLT_CMD);
	Data.CommandID = ConvertCommand(Data.CashNodeID, cmd, 1);
	ASSIGN_PTR(pData, Data);
	return 1;
}

class CMD_HDL_CLS(CASHNODEPANEL) : public PPCommandHandler {
public:
	CMD_HDL_CLS(CASHNODEPANEL)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1, valid_data = 0;
		size_t sav_offs = 0;
		CashNodePaneFilt * p_filt = 0;
		CashNodeFiltDialog * p_dlg = 0;
		THROW_INVARG(pParam);
		THROW(CheckDialogPtr(&(p_dlg = new CashNodeFiltDialog)));
		sav_offs = pParam->GetRdOffs();
		THROW(PPView::ReadFiltPtr(*pParam, reinterpret_cast<PPBaseFilt **>(&p_filt)));
		if(!p_filt)
			THROW(PPView::CreateFiltInstance(PPFILT_CASHNODEPANE, reinterpret_cast<PPBaseFilt **>(&p_filt)) > 0);
		p_dlg->setDTS(p_filt);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(p_filt) > 0)
				ok = valid_data = 1;
			else
				PPError();
		}
		if(ok > 0) {
			THROW(PPView::WriteFiltPtr(pParam->Z(), p_filt));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		ZDELETE(p_filt);
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if((D.MenuCm || cmdID) && APPL) {
			CashNodePaneFilt * p_filt = 0;
			static_cast<PPApp *>(APPL)->LastCmd = cmdID ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
			PPView::ReadFiltPtr(*pParam, reinterpret_cast<PPBaseFilt **>(&p_filt));
			ok = ExecCSPanel(p_filt);
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CASHNODEPANEL);
//
//
//
class CMD_HDL_CLS(ADDBILL) : public PPCommandHandler {
public:
	CMD_HDL_CLS(ADDBILL)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		PPObjBill::CreateNewInteractive_Param filt;
		SSerializeContext sctx;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		filt.Serialize(-1, *pParam, &sctx);
		if(BillObj->EditCreateNewInteractiveParam(&filt) > 0) {
			pParam->Z();
			THROW(filt.Serialize(+1, *pParam, &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(APPL) {
			if(D.MenuCm || cmdID) {
				int    r = 1;
				SBuffer param;
				PPObjBill::CreateNewInteractive_Param filt;
				SSerializeContext sctx;
				static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				RVALUEPTR(param, pParam);
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						filt.Serialize(-1, param, &sctx);
						r = 1;
					}
					else
						r = -1;
				}
				else {
					filt.Serialize(-1, param, &sctx);
				}
				if(r > 0) {
					ok = BillObj->CreateNewInteractive(&filt);
				}
			}
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(ADDBILL);

int SearchDlvrAddr()
{
	int    ok = -1;
	SString title, inp_title, srch_str;
	PPInputStringDialogParam isd_param;
	PPLoadText(PPTXT_SRCHDLVRADDRTITLE, isd_param.Title);
	PPLoadText(PPTXT_SRCHDLVRADDRINPUTTITLE, isd_param.InputTitle);
	for(int valid_data = 0; !valid_data && InputStringDialog(&isd_param, srch_str) > 0;) {
		if(srch_str.Len()) {
			int r = 0;
			PPID id = 0;
			PPObjLocation loc_obj;
			if(srch_str.C(0) == '#') {
				id = srch_str.ShiftLeft().ToLong();
				if(id != 0)
					r = loc_obj.Search(id, 0);
			}
			else
				r = loc_obj.P_Tbl->SearchCode(LOCTYP_ADDRESS, srch_str, &id, 0);
			if(r > 0) {
				ok = BIN(loc_obj.Edit(&id, 0) != 0);
				valid_data = 1;
			}
		}
	}
	return ok;
}

class CMD_HDL_CLS(SEARCHDLVRADDR) : public PPCommandHandler {
public:
	CMD_HDL_CLS(SEARCHDLVRADDR)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		return -1;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				ok = SearchDlvrAddr();
			}
			p_app->LastCmd = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SEARCHDLVRADDR);

class TSessCreateFilt { // @persistent
public:
	static const char * P_PrivSign;

	TSessCreateFilt() : PrivateVer(2), GrpID(0), PrcID(0), Status(0), Reserve(0)
	{
		memcpy(PrivateSignature, P_PrivSign, sizeof(PrivateSignature));
		memzero(Reserve2, sizeof(Reserve2));
	}
	int    Write(SBuffer & rBuf, long) const
	{
		//return (rBuf.Write(GrpID) && rBuf.Write(PrcID) && rBuf.Write(Status) && rBuf.Write(Reserve)) ? 1 : PPSetErrorSLib();
		int    ok = 1;
		THROW_SL(rBuf.Write(PrivateSignature, sizeof(PrivateSignature)));
		THROW_SL(rBuf.Write(PrivateVer));
		THROW_SL(rBuf.Write(GrpID));
		THROW_SL(rBuf.Write(PrcID));
		THROW_SL(rBuf.Write(Status));
		THROW_SL(rBuf.Write(Reserve));
		THROW_SL(rBuf.Write(Reserve2, sizeof(Reserve2)));
		THROW_SL(rBuf.Write(WtmFileName));
		CATCHZOK
		return ok;
	}
	int    Read(SBuffer & rBuf, long)
	{
		int    ok = 1;
		if(rBuf.GetAvailableSize()) {
			THROW_SL(rBuf.Read(PrivateSignature, sizeof(PrivateSignature)));
			if(memcmp(PrivateSignature, P_PrivSign, sizeof(PrivateSignature)) == 0) {
				THROW_SL(rBuf.Read(PrivateVer));
				THROW_SL(rBuf.Read(GrpID));
				THROW_SL(rBuf.Read(PrcID));
				THROW_SL(rBuf.Read(Status));
				THROW_SL(rBuf.Read(Reserve));
				THROW_SL(rBuf.Read(Reserve2, sizeof(Reserve2)));
				THROW_SL(rBuf.Read(WtmFileName));
			}
			else {
				THROW_SL(rBuf.Unread(sizeof(PrivateSignature)));
				memcpy(PrivateSignature, P_PrivSign, sizeof(PrivateSignature));
				PrivateVer = 2;
				THROW_SL(rBuf.Read(GrpID));
				THROW_SL(rBuf.Read(PrcID));
				THROW_SL(rBuf.Read(Status));
				THROW_SL(rBuf.Read(Reserve));
			}
		}
		CATCHZOK
		return ok;
	}
	char  PrivateSignature[8]; // "TSESCR\x20\x20"
	int32 PrivateVer;
	PPID  GrpID;
	PPID  PrcID;
	int16 Status;
	int16 Reserve;
	uint8 Reserve2[32];
	SString WtmFileName;
};

/*static*/const char * TSessCreateFilt::P_PrivSign = "TSESCR\x20\x20";

#define GRP_CRTTSESSFLT_WTM 1

class CMD_HDL_CLS(CREATETECHSESS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(CREATETECHSESS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		TDialog * p_dlg = new TDialog(DLG_CRTTSESSFLT);
		TSessCreateFilt filt;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		THROW(CheckDialogPtr(&p_dlg));
		FileBrowseCtrlGroup::Setup(p_dlg, CTLBRW_CRTTSESSFLT_WTM, CTL_CRTTSESSFLT_WTM, GRP_CRTTSESSFLT_WTM, 0, PPTXT_FILPAT_WTM,
			FileBrowseCtrlGroup::fbcgfFile);
		SetupPPObjCombo(p_dlg, CTLSEL_CRTTSESSFLT_GRP, PPOBJ_PROCESSOR, filt.GrpID, OLW_CANINSERT|OLW_CANSELUPLEVEL, reinterpret_cast<void *>(PRCEXDF_GROUP));
		SetupPPObjCombo(p_dlg, CTLSEL_CRTTSESSFLT_PRC, PPOBJ_PROCESSOR, filt.PrcID, OLW_CANINSERT);
		p_dlg->AddClusterAssoc(CTL_CRTTSESSFLT_STATUS,    0, TSESST_PLANNED);
		p_dlg->AddClusterAssocDef(CTL_CRTTSESSFLT_STATUS, 1, TSESST_PENDING);
		p_dlg->AddClusterAssoc(CTL_CRTTSESSFLT_STATUS,    2, TSESST_INPROCESS);
		p_dlg->SetClusterData(CTL_CRTTSESSFLT_STATUS, filt.Status);
		if(!filt.WtmFileName.NotEmptyS()) {
			FileBrowseCtrlGroup * p_fbg = static_cast<FileBrowseCtrlGroup *>(p_dlg->getGroup(GRP_CRTTSESSFLT_WTM));
			if(p_fbg) {
				SString path;
				PPGetPath(PPPATH_WTM, path);
				p_fbg->setInitPath(path);
			}
		}
		p_dlg->setCtrlString(CTL_CRTTSESSFLT_WTM, filt.WtmFileName);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			p_dlg->getCtrlData(CTLSEL_CRTTSESSFLT_GRP,    &filt.GrpID);
			p_dlg->getCtrlData(CTLSEL_CRTTSESSFLT_PRC,    &filt.PrcID);
			p_dlg->GetClusterData(CTL_CRTTSESSFLT_STATUS, &filt.Status);
			p_dlg->getCtrlString(CTL_CRTTSESSFLT_WTM, filt.WtmFileName);
			if(filt.GrpID || filt.PrcID || fileExists(filt.WtmFileName))
				valid_data = ok = 1;
			else
				PPError(PPERR_PRCORGRPNOTDEF);
		}
		if(ok > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		delete p_dlg;
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				int    r = 1;
				SBuffer param;
				TSessCreateFilt filt;
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param = *pParam;
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						filt.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					filt.Read(param, 0);
				if(r > 0) {
					if(filt.WtmFileName.NotEmpty() && fileExists(filt.WtmFileName)) {
						TWhatmanObject::SelectObjRetBlock sel_blk;
						if(PPWhatmanWindow::Launch(filt.WtmFileName, 0, &sel_blk) > 0 && sel_blk.Val1 == PPOBJ_PROCESSOR && sel_blk.Val2 > 0)
							filt.PrcID = sel_blk.Val2;
						else
							r = -1;
					}
					if(r > 0) {
						PPID   id = 0;
						PPObjTSession obj_tsess;
						if(!filt.PrcID) {
							SString title;
							SString name;
							PPIDArray child_list;
							PPObjProcessor 	obj_prcssr;
							StrAssocArray prcssr_list;
							r = 0;
							// @v9.0.2 PPGetWord(PPWORD_PRC, 0, title);
							PPLoadString("processor", title); // @v9.0.2
							obj_prcssr.GetChildIDList(filt.GrpID, 0, &child_list);
							for(uint i = 0; i < child_list.getCount(); i++) {
								GetObjectName(PPOBJ_PROCESSOR, child_list.at(i), name = 0);
								prcssr_list.Add(child_list.at(i), 0, name);
							}
							for(int valid_data = 0; !valid_data && ListBoxSelDialog::Run(&prcssr_list, title, &filt.PrcID) > 0;) {
								if(filt.PrcID > 0)
									valid_data = r = 1;
								else
									PPError(PPERR_PRCNEEDED);
							}
						}
						if(r > 0)
							ok = obj_tsess.Add(&id, 0, filt.PrcID, TSESK_SESSION, filt.Status);
					}
				}
			}
			p_app->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CREATETECHSESS);

class CMD_HDL_CLS(CTBLORDCREATE) : public PPCommandHandler {
public:
	CMD_HDL_CLS(CTBLORDCREATE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		CTableOrder cto;
		CTableOrder::Param param;
		SSerializeContext ctx;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize())
			THROW(param.Serialize(-1, *pParam, &ctx));
		THROW(ok = cto.EditParam(&param));
		if(ok > 0) {
			THROW(param.Serialize(+1, pParam->Z(), &ctx));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				int    r = 1;
				SBuffer param_buf;
				CTableOrder::Param param;
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param_buf = *pParam;
				if(param_buf.GetAvailableSize()) {
					SSerializeContext ctx;
					THROW(param.Serialize(-1, param_buf, &ctx));
				}
				if(param.Flags & CTableOrder::Param::fShowTimeGraph) {
					CTableOrder::ShowTimeGraph(param.PosNodeID, 1);
				}
				else {
					CTableOrder cto;
					THROW(cto.Create(&param));
				}
			}
			p_app->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CTBLORDCREATE);

class CMD_HDL_CLS(UPDATEQUOTS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(UPDATEQUOTS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		QuotUpdFilt filt;
		THROW_INVARG(pParam);
		sav_offs = pParam->GetRdOffs();
		if(!pParam->GetAvailableSize()) {
			filt.LocList.Add(LConfig.Location);
			filt.Flags |= QuotUpdFilt::fExistOnly;
		}
		else
			filt.Read(*pParam, 0);
		if(EditQuotUpdDialog(&filt) > 0) {
			THROW(filt.Write(pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		PPApp * p_app = static_cast<PPApp *>(APPL);
		if(p_app) {
			if(D.MenuCm || cmdID) {
				int  r = 1;
				QuotUpdFilt filt;
				SBuffer param;
				p_app->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param = *pParam;
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						filt.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					filt.Read(param, 0);
				if(r > 0)
					UpdateQuots(&filt);
			}
			p_app->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
private:
};

IMPLEMENT_CMD_HDL_FACTORY(UPDATEQUOTS);

class CMD_HDL_CLS(TRANSMITMODIFICATIONS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(TRANSMITMODIFICATIONS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		ObjTransmitParam trnsm_param;
		QuotUpdFilt filt;
		THROW_INVARG(pParam);

		sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize())
			trnsm_param.Read(*pParam, 0);
		if(ObjTransmDialog(DLG_MODTRANSM, &trnsm_param, OBJTRNSMDLGF_SEARCHDTTM) > 0) {
			THROW(trnsm_param.Write(pParam->Z(), 0));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long cmdID, void * extraPtr)
	{
		int    ok = -1;
		if(APPL) {
			if(D.MenuCm || cmdID) {
				int  r = 1;
				ObjTransmitParam trnsm_param;
				SBuffer param;
				static_cast<PPApp *>(APPL)->LastCmd = (cmdID) ? (cmdID + ICON_COMMAND_BIAS) : D.MenuCm;
				THROW_INVARG(pParam);
				param = *pParam;
				if(!param.GetAvailableSize()) {
					if(EditParam(&param, cmdID, extraPtr) > 0) {
						trnsm_param.Read(param, 0);
						r = 1;
					}
					else
						r = -1;
				}
				else
					trnsm_param.Read(param, 0);
				if(r > 0)
					PPObjectTransmit::TransmitModificationsByDBDivList(&trnsm_param);
			}
			static_cast<PPApp *>(APPL)->LastCmd = 0;
		}
		CATCHZOK
		return ok;
	}
private:
};

IMPLEMENT_CMD_HDL_FACTORY(TRANSMITMODIFICATIONS);
//
//
//
// prototype @defined(filtrnsm.cpp)
int EditObjReceiveParam(ObjReceiveParam * pParam, int editOptions);

class CMD_HDL_CLS(RECEIVEPACKETS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(RECEIVEPACKETS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r;
		ObjReceiveParam param;
		param.Init();
		size_t sav_offs = pParam->GetRdOffs();
		if((r = param.Read(*pParam, 0)) != 0) {
			ok = EditObjReceiveParam(&param, 1);
			if(ok > 0) {
				param.Write(pParam->Z(), 0);
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		ObjReceiveParam param;
		if(param.Read(*pParam, 0) > 0)
			param.Flags |= ObjReceiveParam::fNonInteractive;
		else
			param.Init();
		ok = PPObjectTransmit::ReceivePackets(&param);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(RECEIVEPACKETS);
//
//
//
class CMD_HDL_CLS(SEARCHBILLBYCTX) : public PPCommandHandler {
public:
	CMD_HDL_CLS(SEARCHBILLBYCTX)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		return -1;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		if(D.MenuCm && APPL) {
			int    ok = -1;
			int    r = cmCancel;
			PPObjBill * p_bobj = BillObj;
			BillTbl::Rec bill_rec;
			SString srch_str;
			LongArray dt_list;
			StrAssocArray codes_list;
			PPInputStringDialogParam isd_param;
			THROW_INVARG(pParam);
			sav_offs = pParam->GetRdOffs();
			pParam->Read(srch_str);
			static_cast<PPApp *>(APPL)->LastCmd = D.MenuCm;
			isd_param.InputTitle = PPLoadTextS(PPTXT_BILLSRCHCTX, isd_param.Title);
			if(InputStringDialog(&isd_param, srch_str) > 0 && srch_str.Len()) {
				SString srch_str2;
				SString temp_buf;
				(srch_str2 = srch_str).Transf(CTRANSF_INNER_TO_OUTER);
				//
				// Поиск дат
				//
				const char * p_dt_pattern = "[0-3]?[0-9][/.-][0-1]?[0-9][/.-][0-9]?[0-9]?[0-9]?[0-9]";
				SRegExp2 expr(p_dt_pattern, cp1251, SRegExp2::syntaxDefault, 0);
				SStrScan scan(srch_str2);
				while(expr.Find(&scan, 0)) {
					DateRange period;
					scan.Get(temp_buf.Z());
					if(strtoperiod(temp_buf, &period, 0)) {
						if(checkdate(period.low, 1)) {
							LDATE dt_ = period.low.getactual(ZERODATE);
							dt_list.add(dt_.v);
						}
					}
					srch_str2.Excise(scan.Offs, scan.Len);
					scan.Offs = 0;
					scan.Len  = 0;
				}
				//
				// Поиск номеров документов
				//
				const char * p_code_pattern = "[^ ,][^ ,]+";
				expr.Compile(p_code_pattern, cp1251, SRegExp2::syntaxDefault, 0);
				scan.Offs = 0;
				scan.Len  = 0;
				while(expr.Find(&scan, 0)) {
					int has_digit = 0;
					scan.Get(temp_buf.Z());
					for(uint i = 0; !has_digit && i < temp_buf.Len(); i++)
						if(temp_buf.C(i) > 47 && temp_buf.C(i) < 58)
							has_digit = 1;
					if(has_digit)
						codes_list.Add(codes_list.getCount(), 0, temp_buf.ToOem());
					srch_str2.Excise(scan.Offs, scan.Len);
					scan.Offs = 0;
					scan.Len  = 0;
				}
				THROW(pParam->Z().Write(srch_str));
			}
			else
				pParam->SetRdOffs(sav_offs);
			if(dt_list.getCount()) {
				BillFilt   filt;
				uint dts_count = dt_list.getCount();
				for(uint i = 0; i < dts_count; i++) {
					LDATE dt = ZERODATE;
					DateIter di;
					dt.v = dt_list.at(i);
					di.dt  = dt;
					di.end = dt;
					while(p_bobj->P_Tbl->EnumByDate(&di, &bill_rec) > 0 && bill_rec.Dt == dt)
						if(codes_list.getCount() == 0 || codes_list.SearchByText(bill_rec.Code, 0, 0) > 0)
							filt.List.Add(bill_rec.ID);
				}
				if(filt.List.GetCount())
					ok = PPView::Execute(PPVIEW_BILL, &filt, 1, 0);
			}
			else if(codes_list.getCount()) {
				BillFilt   filt;
				for(uint i = 0; i < codes_list.getCount(); i++) {
					BillTbl::Key7 k7;
					MEMSZERO(k7);
					StrAssocArray::Item code_item = codes_list.Get(i);
					const size_t code_pattern_len = sstrlen(code_item.Txt);
					if(code_pattern_len) {
						STRNSCPY(k7.Code, code_item.Txt);
						if(p_bobj->P_Tbl->search(7, &k7, spGe) && strnicmp866(p_bobj->P_Tbl->data.Code, code_item.Txt, code_pattern_len) == 0) do {
							p_bobj->P_Tbl->copyBufTo(&bill_rec);
							filt.List.Add(bill_rec.ID);
						} while(p_bobj->P_Tbl->search(7, &k7, spNext) && strnicmp866(p_bobj->P_Tbl->data.Code, code_item.Txt, code_pattern_len) == 0);
					}
				}
				if(filt.List.GetCount())
					ok = PPView::Execute(PPVIEW_BILL, &filt, 1, 0);
			}
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		static_cast<PPApp *>(APPL)->LastCmd = 0;
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SEARCHBILLBYCTX);
//
//
//
class CMD_HDL_CLS(WRITEOFFDRAFTS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(WRITEOFFDRAFTS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrWrOffDraftFilt filt; // PPBaseFilt
		if(pParam) {
			if(!filt.Read(*pParam, 0))
				filt.Init(1, 0);
			if(Prcssr.EditParam(&filt) > 0) {
				if(filt.Write(*pParam, 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrWrOffDraftFilt filt, * p_filt = 0;
		if(pParam) {
			if(filt.Read(*pParam, 0))
				p_filt = &filt;
			else
				PPError();
		}
		if(!p_filt && Prcssr.EditParam(&filt) > 0) {
			p_filt = &filt;
		}
		if(p_filt)
			ok = Prcssr.Init(p_filt) ? Prcssr.Run() : PPErrorZ();
		return ok;
	}
private:
	PrcssrWrOffDraft Prcssr;
};

IMPLEMENT_CMD_HDL_FACTORY(WRITEOFFDRAFTS);
//
//
//
class CMD_HDL_CLS(INFOKIOSK) : public PPCommandHandler {
public:
	CMD_HDL_CLS(INFOKIOSK)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		InfoKioskPaneFilt filt;
		if(pParam) {
			if(!filt.Read(*pParam, 0))
				filt.Init(1, 0);
			if(Obj.EditInfoKioskPaneFilt(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		InfoKioskPaneFilt filt;
		InfoKioskPaneFilt * p_filt = 0;
		if(pParam && pParam->GetAvailableSize() > 0) { // @v11.5.9 @fix (&& pParam->GetAvailableSize() > 0)
			if(filt.Read(*pParam, 0))
				p_filt = &filt;
			else 
				PPError();
		}
		return ViewGoodsInfo(p_filt);
	}
private:
	PPObjGoodsInfo Obj;
};

IMPLEMENT_CMD_HDL_FACTORY(INFOKIOSK);
//
//
//
class CMD_HDL_CLS(UNIFYGOODSPRICE) : public PPCommandHandler {
public:
	CMD_HDL_CLS(UNIFYGOODSPRICE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrUnifyPrice prc_uniprice;
			PrcssrUnifyPriceFilt filt;
			if(!filt.Read(*pParam, 0))
				filt.Init(1, 0);
			if(prc_uniprice.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrUnifyPriceFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrUnifyPrice prc_uniprice;
				if(!prc_uniprice.Process(&filt))
					ok = PPErrorZ();
			}
			else
				ok = UnifyGoodsPrice();
		}
		else
			ok = UnifyGoodsPrice();
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(UNIFYGOODSPRICE);
//
//
//
int FASTCALL WriteParam(SBuffer & rBuf, const void * pParam, size_t paramSize); // @prototype(ppjob.cpp)
int FASTCALL ReadParam(SBuffer & rBuf, void * pParam, size_t paramSize);  // @prototype(ppjob.cpp)

class CMD_HDL_CLS(TESTPREDICTSALESTBL) : public PPCommandHandler {
public:
	CMD_HDL_CLS(TESTPREDICTSALESTBL)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 0;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		MEMSZERO(param);
		size_t sav_offs = pParam->GetRdOffs();
		if((r = ReadParam(*pParam, &param, sizeof(param))) != 0) {
			param.Process |= param.prcsTest;
			if((ok = prcssr.EditParam(&param)) > 0) {
				WriteParam(pParam->Z(), &param, sizeof(param));
			}
			else if(r > 0)
				pParam->SetRdOffs(sav_offs);
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrPrediction::Param param;
		PrcssrPrediction prcssr;
		if(ReadParam(*pParam, &param, sizeof(param))) {
			param.Process |= param.prcsTest;
			prcssr.Init(&param);
			ok = prcssr.Run();
		}
		else
			ok = 0;
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(TESTPREDICTSALESTBL);
//
//
//
class CMD_HDL_CLS(EXPORTGOODSRESTUHTT) : public PPCommandHandler {
public:
	CMD_HDL_CLS(EXPORTGOODSRESTUHTT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		uint   val = 0;
		PPViewGoodsRest gr_view;
		GoodsRestFilt filt;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			filt.Read(*pParam, 0);
		}
		if(gr_view.EditBaseFilt(&filt) > 0) {
			if(filt.Write(pParam->Z(), 0))
				ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize() != 0) {
			PPViewGoodsRest gr_view;
			GoodsRestFilt filt;
			THROW(filt.Read(*pParam, 0));
			THROW(gr_view.Init_(&filt));
			THROW(gr_view.ExportUhtt(1));
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTGOODSRESTUHTT);
//
//
//
class CMD_HDL_CLS(IMPORTBILLS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(IMPORTBILLS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		int    r = 1;
		uint   val = 0;
		SSerializeContext sctx;
		PPBillImpExpBaseProcessBlock blk;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(blk.SerializeParam(-1, *pParam, &sctx));
		}
		if(blk.Select(1) > 0) {
			THROW(blk.SerializeParam(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize()) {
			SSerializeContext sctx;
			PPBillImporter prcssr;
			prcssr.Init();
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
			THROW(prcssr.LoadConfig(1));
			THROW(prcssr.Run());
			ok = 1;
		}
		else {
			ok = ImportBills(0, 0, 0, 0);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(IMPORTBILLS);
//
//
//
#if 0 // @construction {
class CMD_HDL_CLS(IMPORTGOODS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(IMPORTGOODS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		SSerializeContext sctx;
		PPGoodsImporter prcssr;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
		}
		if(prcssr.Select(1) > 0) {
			THROW(blk.SerializeParam(+1, pParam->Clear(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize()) {
			SSerializeContext sctx;
			PPGoodsImporter prcssr;
			THROW(prcssr.Init())
			THROW(prcssr.SerializeParam(-1, *pParam, &sctx));
			THROW(prcssr.LoadConfig(1));
			THROW(prcssr.Run());
			ok = 1;
		}
		else {
			ok = ImportGoods(0, 0, 0, 0);
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(IMPORTGOODS);
#endif // } 0 @construction
//
//
//
class CMD_HDL_CLS(PROCESSOBJTEXT) : public PPCommandHandler {
public:
	CMD_HDL_CLS(PROCESSOBJTEXT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrObjText prc;
			PrcssrObjTextFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrObjTextFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrObjText prc;
				if(!prc.Init(&filt) || !prc.Run())
					ok = PPErrorZ();
			}
			else
				ok = DoProcessObjText(0);
		}
		else
			ok = DoProcessObjText(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PROCESSOBJTEXT);
//
//
//
class CMD_HDL_CLS(PERSONEVENTBYREADER) : public PPCommandHandler {
public:
	CMD_HDL_CLS(PERSONEVENTBYREADER)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		AddPersonEventFilt filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(filt.Edit() > 0) {
			THROW(filt.Write(pParam->Z(), 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			AddPersonEventFilt filt;
			if(filt.Read(*pParam, 0)) {
				PPObjPersonEvent pe_obj;
				pe_obj.ProcessDeviceInput(filt);
				ok = 1;
			}
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PERSONEVENTBYREADER);
//
//
//
// @vmiller
class CMD_HDL_CLS(TSESSAUTOSMS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(TSESSAUTOSMS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1, r = 1;
		uint   val = 0;
		PPViewTSession tsess_view;
		SSerializeContext sctx;
		TSessionFilt tsess_filt;
		size_t sav_offs = pParam->GetRdOffs();
		if(pParam->GetAvailableSize() == 0) {
			;
		}
		else {
			THROW(tsess_filt.Serialize(-1, *pParam, &sctx));
		}
		if(tsess_view.EditBaseFilt(&tsess_filt) > 0) {
			THROW(tsess_filt.Serialize(+1, pParam->Z(), &sctx));
			ok = 1;
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCHZOKPPERR
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam->GetAvailableSize()) {
			SSerializeContext sctx;
			PPViewTSession tsess_view;
			TSessionFilt tsess_filt;
			THROW(tsess_filt.Serialize(-1, *pParam, &sctx));
			THROW(tsess_view.Init_(&tsess_filt));
			THROW(tsess_view.SendAutoSms());
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(TSESSAUTOSMS);
//
//
//
class CMD_HDL_CLS(CREATEDRAFTBYSUPPLORDER) : public PPCommandHandler {
public:
	CMD_HDL_CLS(CREATEDRAFTBYSUPPLORDER)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t preserve_offs = 0;
		SStatFilt * p_filt = 0;
		if(pParam) {
			PPViewSStat view;
			preserve_offs = pParam->GetRdOffs();
			THROW_MEM(p_filt = static_cast<SStatFilt *>(view.CreateFilt(reinterpret_cast<void *>(1))));
			if(pParam->GetAvailableSize() != 0)
				p_filt->Read(*pParam, 0);
			if(view.EditBaseFilt(p_filt) > 0) {
				THROW(p_filt->Write(pParam->Z(), 0));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		ZDELETE(p_filt);
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			SStatFilt filt;
			if(filt.Read(*pParam, 0) > 0)
				ok = PrcssrBillAutoCreate::CreateDraftBySupplOrders(&filt);
			else
				ok = 0;
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CREATEDRAFTBYSUPPLORDER);
//
//
//
class CMD_HDL_CLS(SENDBILLS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(SENDBILLS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t preserve_offs = 0;
		BillTransmitParam * p_filt = 0;
		if(pParam) {
			preserve_offs = pParam->GetRdOffs();
			THROW_MEM(p_filt = new BillTransmitParam);
			if(pParam->GetAvailableSize() != 0)
				p_filt->Read(*pParam, 0);
			if(p_filt->Edit() > 0) {
				THROW(p_filt->Write(pParam->Z(), 0));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		ZDELETE(p_filt);
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam && pParam->GetAvailableSize()) {
			BillTransmitParam filt;
			THROW(filt.Read(*pParam, 0) > 0);
			THROW(PPObjectTransmit::TransmitBillsByDBDivList(&filt));
			ok = 1;
		}
		else {
			PPObjectTransmit::TransmitBillsByDBDivList(0);
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SENDBILLS);
//
//
// @v10.4.1 {
class CMD_HDL_CLS(SENDBILLSWITHFILT) : public PPCommandHandler {
public:
	CMD_HDL_CLS(SENDBILLSWITHFILT)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		const size_t preserve_offs = pParam ? pParam->GetRdOffs() : 0;
		if(pParam) {
			int    r = 1;
			BillFilt filt;
			ObjTransmitParam tr_param;
			filt.SetupBrowseBillsType(filt.Bbt = bbtUndef);
			if(pParam->GetAvailableSize() == 0) {
				uint   val = 0;
				if((r = SelectorDialog(DLG_BBTSEL, CTL_BBTSEL_TYPE, &val)) > 0)
					filt.SetupBrowseBillsType(filt.Bbt = static_cast<BrowseBillsType>(val));
			}
			else {
				filt.Read(*pParam, 0);
				tr_param.Read(*pParam, 0);
			}
			if(r > 0 && ObjTransmDialogExt(DLG_OBJTRANSM, PPVIEW_BILL, &tr_param, &filt) > 0) {
				THROW(filt.Write(pParam->Z(), 0));
				THROW(tr_param.Write(*pParam, 0));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = 1;
		if(pParam && pParam->GetAvailableSize()) {
			BillFilt filt;
			ObjTransmitParam tr_param;
			THROW(filt.Read(*pParam, 0));
			THROW(tr_param.Read(*pParam, 0));
			THROW(PPViewBill::TransmitByFilt(&filt, &tr_param));
		}
		else {
			THROW(PPViewBill::TransmitByFilt(0, 0));
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SENDBILLSWITHFILT);
// } @v10.4.1
//
//
class CMD_HDL_CLS(EXPORTDBTBLTRANSFER) : public PPCommandHandler {
public:
	CMD_HDL_CLS(EXPORTDBTBLTRANSFER)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		size_t preserve_offs = 0;
		if(pParam) {
			preserve_offs = pParam->GetRdOffs();
			if(pParam->GetAvailableSize() != 0)
				THROW(filt.Serialize(-1, *pParam, &sctx));
			if(PPDbTableXmlExportParam_TrfrBill::Edit(&filt) > 0) {
				THROW(filt.Serialize(+1, pParam->Z(), &sctx));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPDbTableXmlExporter_Transfer prc(filt);
            THROW(prc.Run(filt.FileName));
            ok = 1;
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTDBTBLTRANSFER);
//
//
//
class CMD_HDL_CLS(EXPORTDBTBLBILL) : public CMD_HDL_CLS(EXPORTDBTBLTRANSFER) {
public:
	CMD_HDL_CLS(EXPORTDBTBLBILL)(const PPCommandDescr * pDescr) : CMD_HDL_CLS(EXPORTDBTBLTRANSFER)(pDescr)
	{
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPDbTableXmlExportParam_TrfrBill filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			PPDbTableXmlExporter_Bill prc(filt);
            THROW(prc.Run(filt.FileName));
            ok = 1;
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTDBTBLBILL);
//
//
//
class CMD_HDL_CLS(IMPORTFIAS) : public PPCommandHandler {
public:
	CMD_HDL_CLS(IMPORTFIAS)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		FiasImporter::Param filt;
		SSerializeContext sctx;
		size_t preserve_offs = 0;
		if(pParam) {
			preserve_offs = pParam->GetRdOffs();
			if(pParam->GetAvailableSize() != 0)
				THROW(filt.Serialize(-1, *pParam, &sctx));
			if(FiasImporter::EditParam(filt) > 0) {
				THROW(filt.Serialize(+1, pParam->Z(), &sctx));
				ok = 1;
			}
			else
				pParam->SetRdOffs(preserve_offs);
		}
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(preserve_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		FiasImporter::Param filt;
		SSerializeContext sctx;
		THROW(filt.Serialize(-1, *pParam, &sctx));
		{
			FiasImporter prc;
			THROW(prc.Run(filt));
            ok = 1;
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(IMPORTFIAS);
//
//
//
class CMD_HDL_CLS(SUPPLINTERCHANGE) : public PPCommandHandler {
public:
	CMD_HDL_CLS(SUPPLINTERCHANGE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrSupplInterchange prc;
			SupplInterchangeFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			SupplInterchangeFilt filt;
			if(filt.Read(*pParam, 0)) {
				PrcssrSupplInterchange prc;
				if(!prc.Init(&filt) || !prc.Run())
					ok = PPErrorZ();
			}
			else
				ok = DoSupplInterchange(0);
		}
		else
			ok = DoSupplInterchange(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SUPPLINTERCHANGE);
//
//
//
class CMD_HDL_CLS(PROCESSOSM) : public PPCommandHandler {
public:
	CMD_HDL_CLS(PROCESSOSM)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrOsm prc(0);
			PrcssrOsmFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrOsmFilt filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrOsm prc(0);
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else
			ok = DoProcessOsm(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PROCESSOSM);
//
//
//
class CMD_HDL_CLS(PROCESSSARTRE) : public PPCommandHandler {
public:
	CMD_HDL_CLS(PROCESSSARTRE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrSartre prc(0);
			PrcssrSartreFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrSartreFilt filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrSartre prc(0);
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else
			ok = DoProcessSartre(0);
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(PROCESSSARTRE);
//
//
//
class CMD_HDL_CLS(BILLAUTOCREATE) : public PPCommandHandler {
public:
	CMD_HDL_CLS(BILLAUTOCREATE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrBillAutoCreate prc;
			PPBillAutoCreateParam filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PPBillAutoCreateParam filt;
		if(pParam && filt.Read(*pParam, 0)) {
			PrcssrBillAutoCreate prc;
			if(!prc.Init(&filt) || !prc.Run())
				ok = PPErrorZ();
		}
		else {
			PrcssrBillAutoCreate prc;
			if(prc.EditParam(&filt) > 0) {
				if(!prc.Init(&filt) || !prc.Run())
					ok = PPErrorZ();
				else
					ok = 1;
			}
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(BILLAUTOCREATE);
//
//
//
class CMD_HDL_CLS(TIMESERIESSA) : public PPCommandHandler {
public:
	CMD_HDL_CLS(TIMESERIESSA)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PrcssrTsStrategyAnalyze prc;
			PrcssrTsStrategyAnalyzeFilt filt;
			if(!filt.Read(*pParam, 0))
				prc.InitParam(&filt);
			if(prc.EditParam(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		PrcssrTsStrategyAnalyze prc;
		PrcssrTsStrategyAnalyzeFilt filt;
		if(pParam && filt.Read(*pParam, 0)) {
			ok = (prc.Init(&filt) && prc.Run()) ? 1 : PPErrorZ();
		}
		else if(prc.EditParam(&filt) > 0) {
			ok = (prc.Init(&filt) && prc.Run()) ? 1 : PPErrorZ();
		}
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(TIMESERIESSA);
//
//
//
int Helper_ExecuteNF_WithSerializedParam(SBuffer * pParam)
{
	int    ok = 1;
	PPView::ExecNfViewParam param;
	SString result_fname, dest_fname;
	// @debug {
		const PPThreadLocalArea & r_tla = DS.GetConstTLA();
		assert((&r_tla) != 0);
	// } @debug
	THROW_INVARG(pParam);
	THROW(param.Read(*pParam, 0));
	THROW(PPView::ExecuteNF(param.NfSymb, param.Dl600_Name, result_fname));
	if(param.FileName.NotEmpty()) {
		SPathStruc dest_ps(param.FileName);
		if(dest_ps.Nam.IsEmpty()) {
			SPathStruc src_ps(result_fname);
			dest_ps.Nam = src_ps.Nam;
			dest_ps.Ext = src_ps.Ext;
		}
		dest_ps.Merge(dest_fname);
		THROW(SCopyFile(result_fname, dest_fname, 0, FILE_SHARE_READ, 0));
		SFile::Remove(result_fname);
	}
	CATCHZOK
	return ok;
}

class CMD_HDL_CLS(EXPORTVIEW) : public PPCommandHandler {
public:
	CMD_HDL_CLS(EXPORTVIEW)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PPView::ExecNfViewParam filt;
			filt.Read(*pParam, 0);
			if(PPView::EditExecNfViewParam(filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		return Helper_ExecuteNF_WithSerializedParam(pParam);
	}
};

IMPLEMENT_CMD_HDL_FACTORY(EXPORTVIEW);
//
//
//
static int EditTextFileParamDialog(EditTextFileParam * pData)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_EDITTEXTPARAM);
	if(CheckDialogPtrErr(&dlg)) {
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_EDITTEXTPARAM_PAT, CTL_EDITTEXTPARAM_PATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfFile);
		dlg->setCtrlString(CTL_EDITTEXTPARAM_PATH, pData->FileName);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_EDITTEXTPARAM_PATH, pData->FileName);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class CMD_HDL_CLS(OPENTEXTFILE) : public PPCommandHandler {
public:
	CMD_HDL_CLS(OPENTEXTFILE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			EditTextFileParam filt;
			filt.Read(*pParam, 0);
			if(EditTextFileParamDialog(&filt) > 0) {
				if(filt.Write(pParam->Z(), 0)) {
					ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = 1;
		EditTextFileParam filt;
		if(!pParam || pParam->GetAvailableSize() == 0)
			PPEditTextFile(0);
		else if(filt.Read(*pParam, 0)) 
			PPEditTextFile(&filt);
		else
			PPEditTextFile(0);
		//CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(OPENTEXTFILE);
//
//
class CMD_HDL_CLS(SETSCARDBYRULE) : public PPCommandHandler { // @v10.8.8 
public:
	CMD_HDL_CLS(SETSCARDBYRULE)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			SCardChargeRule param;
			ReadParam(*pParam, &param, sizeof(param));
			ok = PPObjSCardSeries::SelectRule(&param);
			if(ok > 0)
				WriteParam(pParam->Z(), &param, sizeof(param));
		}
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = 1;
		SCardChargeRule param;
		if(pParam && ReadParam(*pParam, &param, sizeof(param))) {
			THROW(PPObjSCardSeries::SetSCardsByRule(&param));
		}
		else {
			THROW(ok = PPObjSCardSeries::SetSCardsByRule(0));
		}
		CATCHZOK
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(SETSCARDBYRULE);
