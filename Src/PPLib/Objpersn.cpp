// OBJPERSN.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
#include <sartre.h>

#if 0 // {
	int __ReplacePersonNames()
	{
		int    ok = 1;
		PPID   psn_kind_id = PPPRK_SUPPL;
		PPObjPerson psn_obj;
		PPObjArticle ar_obj;
		long   counter = 0, part = 0;
		char   buf[128];
		SStrCollection strings;
		PPWaitStart();
		FILE * stream = fopen("suppls.txt", "r");
		if(stream == 0)
			return 0;
		while(fgets(buf, sizeof(buf), stream)) {
			strip(chomp(buf));
			strings.insert(newStr(buf));
		}
		PersonKindTbl::Key0 k0;
		MEMSZERO(k0);
		k0.KindID = psn_kind_id;
		{
			PPTransaction tra(1);
			THROW(tra);
			while(psn_obj.P_Tbl->Kind.search(0, &k0, spGt)) {
				PPPerson psn_rec;
				if(psn_obj.P_Tbl->Get(k0.PersonID, &psn_rec) > 0) {
					PPWaitMsg(psn_rec.Rec.Name);
					part = counter / strings.getCount();
					STRNSCPY(buf, strings.at(counter % strings.getCount()));
					if(part > 0) {
						for(int i = 0; i < part; i++) {
							if(i == 0)
								strcat(buf, " ");
							strcat(buf, "#");
						}
					}
					STRNSCPY(psn_rec.Rec.Name, buf);
					THROW(psn_obj.P_Tbl->Update(psn_rec.Rec.ID, &psn_rec, 0));
					THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, PPOBJ_PERSON, psn_rec.Rec.ID, (long)psn_rec.Rec.Name) != DBRPL_ERROR);
					counter++;
				}
			}
			THROW(tra.Commit());
		}
		PPWaitStop();
		CATCHZOKPPERR
		return ok;
	}
#endif // } 0

SString & FASTCALL GetMainOrgName(SString & rBuf)
{
	return (rBuf = DS.GetConstTLA().MainOrgName);
}

int FASTCALL GetMainOrgID(PPID * pID)
{
	const PPID _id = CConfig.MainOrgID;
	ASSIGN_PTR(pID, _id);
	return _id ? 1 : PPSetError(PPERR_UNDEFMAINORG);
}

PPID FASTCALL GetMainOrgID()
{
	const PPID _id = CConfig.MainOrgID;
	return _id ? _id : PPSetError(PPERR_UNDEFMAINORG);	
}

int FASTCALL GetMainCityID(PPID * pCityID)
{
	int    ok = -1;
	PPID   city_id = 0;
	const  PPID   main_org_id = GetMainOrgID();
	if(main_org_id) {
		PPObjPerson psn_obj;
		PersonTbl::Rec psn_rec;
		if(psn_obj.Fetch(main_org_id, &psn_rec) > 0) {
			psn_obj.GetCityByAddr(psn_rec.RLoc, &city_id, 0, 1);
			if(city_id)
				ok = 1;
			else {
				psn_obj.GetCityByAddr(psn_rec.MainLoc, &city_id, 0, 1);
				if(city_id)
					ok = 2;
			}
		}
	}
	ASSIGN_PTR(pCityID, city_id);
	return ok;
}

int FASTCALL GetMainEmployerID(PPID * pID)
{
	PPID   main_org_id = 0;
	ASSIGN_PTR(pID, 0);
	if(GetMainOrgID(&main_org_id)) {
		PPObjPerson psnobj;
		if(psnobj.P_Tbl->IsBelongToKind(main_org_id, PPPRK_EMPLOYER)) {
			ASSIGN_PTR(pID, main_org_id);
			return 1;
		}
	}
	return 0;
}

/*static*/int FASTCALL PPObjPerson::GetCurUserPerson(PPID * pID, SString * pBuf)
{
	int    ok = -1;
	PPObjSecur sec_obj(PPOBJ_USR, 0);
	PPSecur secur;
	ASSIGN_PTR(pID, 0);
	CALLPTRMEMB(pBuf, Z());
	if(sec_obj.Fetch(LConfig.UserID, &secur) > 0) {
		if(secur.PersonID) {
			ASSIGN_PTR(pID, secur.PersonID);
			if(pBuf) {
				PPObjPerson psn_obj(SConstructorLite);
				PersonTbl::Rec psn_rec;
				if(psn_obj.Fetch(secur.PersonID, &psn_rec) > 0) {
					*pBuf = psn_rec.Name;
					ok = 1;
				}
			}
			else
				ok = 1;
		}
	}
	else
		ok = -2;
	return ok;
}

int GetUserByPerson(PPID psnID, PPID * pUserID)
{
	int    ok = -1;
	PPID   user_id = 0;
	if(psnID) {
		PPSecur secur;
		PPObjSecur sc_obj(PPOBJ_USR, 0);
		for(SEnum en = sc_obj.P_Ref->Enum(PPOBJ_USR, 0); ok < 0 && en.Next(&secur) > 0;) {
			if(secur.PersonID == psnID) {
				user_id = secur.ID;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pUserID, user_id);
	return ok;
}
//
// PPVCard
//
SlVCard::Rec::Rec() : BirthDay(ZERODATE)
{
	//Init();
}

void SlVCard::Rec::Init()
{
	Name.Z();
	Org.Z();
	WorkAddr.Z();
	HomeAddr.Z();
	BirthDay = ZERODATE;
	WorkPhone.Z();
	HomePhone.Z();
	MobilePhone.Z();
	WorkFax.Z();
	HomeFax.Z();
	Email1.Z();
	Email2.Z();
}

SlVCard::SlVCard(const char * pFileName, int forExport) : P_Stream(0)
{
	PPLoadText(PPTXT_VCARD_PROPERTIES,    Properties);
	PPLoadText(PPTXT_VCARD_PROPATTRIBUTE, PropAttrib);
	Open(pFileName, forExport);
}

SlVCard::~SlVCard()
{
	Close();
}

int SlVCard::Open(const char * pFileName, int forExport)
{
	int    ok = -1;
	Close();
	if(!isempty(pFileName)) {
		Export = forExport;
		THROW_MEM(P_Stream = new SFile(pFileName, (Export) ? SFile::mWrite : SFile::mRead));
		THROW_SL(P_Stream->IsValid());
		if(forExport) {
			SString temp_buf;
			temp_buf.GetSubFrom(Properties, ';', propBegin);
			P_Stream->WriteLine(temp_buf.CR());
			temp_buf.GetSubFrom(Properties, ';', propVersion);
			P_Stream->WriteLine(temp_buf.CR());
		}
		ok = 1;
	}
	CATCH
		Close();
		ok = 0;
	ENDCATCH
	return ok;
}

int SlVCard::Close()
{
	if(P_Stream) {
		if(Export) {
			SString temp_buf;
			temp_buf.GetSubFrom(Properties, ';', propEnd);
			P_Stream->WriteLine(temp_buf.CR());
		}
		ZDELETE(P_Stream);
	}
	return 1;
}

int SlVCard::Put(const Rec * pData)
{
	int    ok = -1;
	if(pData && P_Stream->IsValid() && Export) {
		THROW(PutProp(propName,     &pData->Name));
		THROW(PutProp(propOrg,      &pData->Org));
		THROW(PutProp(propAddr,     &pData->WorkAddr,    paWork));
		THROW(PutProp(propAddr,     &pData->HomeAddr,    paHome));
		THROW(PutProp(propBirthDay, &pData->BirthDay));
		THROW(PutProp(propPhone,    &pData->WorkPhone,   paWork));
		THROW(PutProp(propPhone,    &pData->HomePhone,   paHome));
		THROW(PutProp(propPhone,    &pData->MobilePhone, paMobile));
		THROW(PutProp(propFax,      &pData->WorkFax,     paWork));
		THROW(PutProp(propFax,      &pData->HomeFax,     paHome));
		THROW(PutProp(propEmail,    &pData->Email1,      paInternet));
		THROW(PutProp(propEmail,    &pData->Email2,      paInternet));
	}
	CATCHZOK
	return ok;
}

int SlVCard::Get(Rec * pData)
{
	return -1;
}

int SlVCard::PutProp(Property prop, const void * pData, PropAttribute attrib)
{
	int    ok = -1;
	if(P_Stream && P_Stream->IsValid() && pData) {
		PropAttribute add_attrib;
		SString str_prop, val;
		add_attrib = paNone;
		switch(prop) {
			case propBirthDay: val.Cat(*static_cast<const LDATE *>(pData)); break;
			case propPhone:
				add_attrib = paVoice;
				val = *static_cast<const SString *>(pData);
				break;
			case propFax:
				add_attrib = paFax;
				val = *static_cast<const SString *>(pData);
				break;
			case propName:
			case propOrg:
			case propAddr:
			case propEmail:
				val = *static_cast<const SString *>(pData);
				break;
		}
		str_prop.GetSubFrom(Properties, ';', prop);
		if(str_prop.Len() && val.Strip().Len()) {
			SString temp_buf;
			if(prop != propAddr)
				val.ReplaceChar(';', ' ');
			val.ReplaceChar('\n', ' ');
			val.ReplaceStr("\xD\xA", " ", 0);
			val.ReplaceChar(':', ' ');
			temp_buf.Cat(str_prop);
			if(attrib != paNone) {
				SString str_attr;
				str_attr.GetSubFrom(PropAttrib, ';', attrib);
				if(str_attr.Len())
					temp_buf.Semicol().Cat(str_attr);
			}
			if(add_attrib != paNone) {
				SString str_attr;
				str_attr.GetSubFrom(PropAttrib, ';', add_attrib);
				if(str_attr.Len())
					temp_buf.Semicol().Cat(str_attr);
			}
			temp_buf.Colon().Cat(val).CR().Transf(CTRANSF_INNER_TO_OUTER);
			ok = P_Stream->WriteLine(temp_buf);
		}
	}
	return ok;
}

int SlVCard::GetName(SString &)
{
	return -1;
}

int SlVCard::GetBirthDate(LDATE *)
{
	return -1;
}

int SlVCard::GetPhones(SString &)
{
	return -1;
}

int SlVCard::GetEmails(SString &)
{
	return -1;
}

int SlVCard::GetAddrs(SString &)
{
	return -1;
}
//
//
//
PPObjPerson::SrchAnalogPattern::SrchAnalogPattern(const char * pNamePattern, long flags) : NamePattern(pNamePattern), Flags(flags)
{
}
//
// @obsolete
// Данный модуль - единственная точка проекта, где все еще используется TaggedStringArray
// потому мы локализовали все определения, связанные с этим классом, здесь.
//
struct TaggedString_obsolete {
	static TYPEID BufType() { return static_cast<TYPEID>(MKSTYPE(S_ZSTRING, TaggedString_obsolete::BufSize())); }
	static size_t BufSize() { return (sizeof(TaggedString_obsolete)-sizeof(long)); }
	TaggedString_obsolete() : Id(0)
	{
		PTR32(Txt)[0] = 0;
	}
	long   Id;
	char   Txt[64];
};

class TaggedStringArray_obsolete : public TSArray <TaggedString_obsolete> {
public:
	TaggedStringArray_obsolete() : TSArray <TaggedString_obsolete> ()
	{
	}
	TaggedStringArray_obsolete & FASTCALL operator = (const TaggedStringArray_obsolete & rS)
	{
		copy(rS);
		return *this;
	}
	int Add(long id, const char * pStr)
	{
		TaggedString_obsolete entry;
		entry.Id = id;
		STRNSCPY(entry.Txt, pStr);
		return insert(&entry);
	}
	// @v10.7.11 int    Search(long id, uint * pPos, int binary = 0) const;
	//
	// Descr: Ищет в массиве элемент, текст которого совпадает с параметром pTxt.
	//   поиск не чувствителен к регистру.
	//   Если в массиве содержится более одного элемента, соответствующего
	//   критерию то найден будет только самый первый.
	// Returns:
	//   !0 - элемент найден
	//   0  - элемент не найден
	//
	// @v10.7.11 int    SearchByText(const char * pTxt, uint * pPos) const;
	// @v10.7.11 int    Get(long id, char * pBuf, size_t bufLen, int binary = 0) const;
	// @v10.7.11 int    Get(long id, SString & rBuf, int binary = 0) const;
	// @v10.7.11 void   SortByID();
	// @v10.7.11 void   SortByText();
};

// @v10.7.10 const char * PersonAddImageFolder = "PersonAddImageFolder";

struct Storage_PPPersonConfig { // @persistent @store(PropertyTbl)
	size_t GetSize() const { return (sizeof(*this) + ExtStrSize); }
	PPID   Tag;               // Const=PPOBJ_CONFIG
	PPID   ID;                // Const=PPCFG_MAIN
	PPID   Prop;              // Const=PPPRP_PERSONCFG
	long   Flags;
	uint16 ExtStrSize;        // Размер "хвоста" под строки расширения. Общий размер записи, хранимой в БД
		// равен sizeof(PPPersonConfig) + ExtStrSize
	uint16 StrPosTopFolder;   // [config] PersonFolder
	PPID   TradeLicRegTypeID; // Тип регистрационного документа, используемого для торговой лицензии предприятия //
	PPID   RegStaffCalID;     // Регулярный штатный календарь
	long   StaffCalQuant;     // Квант времени в сек. для временной диаграммы анализа штатных календарей.
	SVerT  Ver;               // Версия системы, создавшей запись
	long   SendSmsSamePersonTimeout;
	TimeRange SmsProhibitedTr; // @v10.2.3
	char   Reserve[32];        // @v10.2.3 [40]-->[32]
	long   Reserve1[2];
	//char   ExtString[];
	//TSArray <PPPersonConfig::NewClientDetectionItem> NewClientDetectionList; // @vmiller
};

/*static*/int FASTCALL PPObjPerson::WriteConfig(const PPPersonConfig * pCfg, int use_ta)
{
	const  long prop_cfg_id = PPPRP_PERSONCFG;
	const  long cfg_obj_type = PPCFGOBJ_PERSON;

	int    ok = 1;
	int    is_new = 0;
	int    r;
	Reference * p_ref = PPRef;
	size_t sz = sizeof(Storage_PPPersonConfig);
	Storage_PPPersonConfig * p_cfg = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, 0, 0));
		is_new = (r > 0) ? 0 : 1;
		if(pCfg) {
			size_t ext_size = 0;
			if(pCfg->TopFolder.NotEmpty())
				ext_size = pCfg->TopFolder.Len()+1;
			if(pCfg->NewClientDetectionList.getCount()) {// @vmiller
				ext_size += sizeof(PPPersonConfig::NewClientDetectionItem) * pCfg->NewClientDetectionList.getCount(); // @vmiller переделать
			}
			if(ext_size)
				ext_size++; // Нулевая позиция - исключительная //
			sz += ext_size;
			p_cfg = static_cast<Storage_PPPersonConfig *>(SAlloc::M(sz));
			memzero(p_cfg, sz);
			p_cfg->Tag       = PPOBJ_CONFIG;
			p_cfg->ID        = PPCFG_MAIN;
			p_cfg->Prop      = prop_cfg_id;
			p_cfg->Flags     = pCfg->Flags;
			p_cfg->TradeLicRegTypeID = pCfg->TradeLicRegTypeID;
			p_cfg->RegStaffCalID     = pCfg->RegStaffCalID;
			p_cfg->StaffCalQuant     = pCfg->StaffCalQuant;
			p_cfg->Ver       = DS.GetVersion();
			p_cfg->SendSmsSamePersonTimeout = pCfg->SendSmsSamePersonTimeout;
			p_cfg->SmsProhibitedTr   = pCfg->SmsProhibitedTr; // @v10.2.3
			if(ext_size) {
				size_t pos = 0;
				char * p_buf = reinterpret_cast<char *>(p_cfg+1);
				p_buf[pos++] = 0;
				if(pCfg->TopFolder.NotEmpty()) {
					p_cfg->StrPosTopFolder = static_cast<uint16>(pos);
					strcpy(p_buf+pos, pCfg->TopFolder);
					pos += (pCfg->TopFolder.Len()+1);
				}
			}
			p_cfg->ExtStrSize = static_cast<uint16>(ext_size);
			{
				char   reg_buf[16];
				memzero(reg_buf, sizeof(reg_buf));
				WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
				reg_key.PutString(_PPConst.WrParam_PersonAddImageFolder, (pCfg->AddImageFolder.Len() == 0) ? reg_buf : pCfg->AddImageFolder);
			}
		}
		THROW(PPObject::Helper_PutConfig(prop_cfg_id, cfg_obj_type, is_new, p_cfg, sz, 0));
		{
			// @v10.7.11 {
			if(pCfg) {
				TaggedStringArray_obsolete temp_list;
				for(uint i = 0; i < pCfg->DlvrAddrExtFldList.getCount(); i++) {
					StrAssocArray::Item item = pCfg->DlvrAddrExtFldList.Get(i);
					if(item.Id && !isempty(item.Txt)) {
						temp_list.Add(item.Id, item.Txt);
					}
				}
				THROW(p_ref->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONCFGEXTFLDLIST, &temp_list, 0));
			}
			else {
				THROW(p_ref->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONCFGEXTFLDLIST, 0, 0));
			}
			// } @v10.7.11
			// @v10.7.11 THROW(p_ref->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONCFGEXTFLDLIST, (pCfg ? &pCfg->DlvrAddrExtFldList : 0), 0));
		}
		THROW(p_ref->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONDETECTIONLIST, (pCfg ? &pCfg->NewClientDetectionList : 0), 0)); // @vmiller
		THROW(tra.Commit());
	}
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}

/*static*/int FASTCALL PPObjPerson::ReadConfig(PPPersonConfig * pCfg)
{
	const  long prop_cfg_id = PPPRP_PERSONCFG;
	int    ok = -1, r;
	Reference * p_ref = PPRef;
	size_t sz = sizeof(Storage_PPPersonConfig) + 256;
	Storage_PPPersonConfig * p_cfg = static_cast<Storage_PPPersonConfig *>(SAlloc::M(sz));
	THROW_MEM(p_cfg);
	THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, p_cfg, sz));
	if(r > 0 && p_cfg->GetSize() > sz) {
		sz = p_cfg->GetSize();
		p_cfg = static_cast<Storage_PPPersonConfig *>(SAlloc::R(p_cfg, sz));
		THROW_MEM(p_cfg);
		THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, p_cfg, sz));
	}
	if(r > 0) {
		pCfg->Flags     = p_cfg->Flags;
		pCfg->TradeLicRegTypeID = p_cfg->TradeLicRegTypeID;
		pCfg->RegStaffCalID     = p_cfg->RegStaffCalID;
		pCfg->StaffCalQuant     = p_cfg->StaffCalQuant;
		//pCfg->Ver       = p_cfg->Ver; // @vmiller
		pCfg->SendSmsSamePersonTimeout = p_cfg->SendSmsSamePersonTimeout;
		pCfg->SmsProhibitedTr   = p_cfg->SmsProhibitedTr; // @v10.2.3
		if(p_cfg->StrPosTopFolder)
			pCfg->TopFolder = reinterpret_cast<const char *>(p_cfg+1) + p_cfg->StrPosTopFolder;
		else
			pCfg->TopFolder = 0;
		{
			size_t buf_size = 0;
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
			if(reg_key.GetRecSize(_PPConst.WrParam_PersonAddImageFolder, &buf_size) > 0 && buf_size > 0) {
				SString temp_buf;
				reg_key.GetString(_PPConst.WrParam_PersonAddImageFolder, temp_buf);
				pCfg->AddImageFolder = temp_buf;
			}
		}
		ok = 1;
	}
	else {
		SString symb;
		pCfg->Flags = 0;
		pCfg->TradeLicRegTypeID = 0;
		pCfg->TopFolder = 0;
		//
		// Для совместимости со старыми версиями извлекаем некоторые значения из pp.ini
		//
		PPIniFile ini_file;
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_TRADELIC, symb)) {
			PPID   rt_id = 0;
			if(PPObjRegisterType::GetByCode(symb, &rt_id) > 0)
				pCfg->TradeLicRegTypeID = rt_id;
		}
		if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_PERSONFOLDER, symb))
			pCfg->TopFolder = symb;
		pCfg->Flags |= PPPersonConfig::fSyncByName;
		ok = -1;
	}
	{
		// @v10.7.11 {
		pCfg->DlvrAddrExtFldList.Z();
		TaggedStringArray_obsolete temp_list;
		THROW(p_ref->GetPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONCFGEXTFLDLIST, &temp_list));
		for(uint i = 0; i < temp_list.getCount(); i++) {
			pCfg->DlvrAddrExtFldList.Add(temp_list.at(i).Id, temp_list.at(i).Txt);
		}
		// } @v10.7.11
		// @v10.7.11 THROW(p_ref->GetPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONCFGEXTFLDLIST, &pCfg->DlvrAddrExtFldList));
	}
	THROW(p_ref->GetPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PERSONDETECTIONLIST, &pCfg->NewClientDetectionList)); // @vmiller
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}

class ExtFieldsDialog : public PPListDialog {
	DECL_DIALOG_DATA(StrAssocArray); // @v10.7.11 TaggedStringArray-->StrAssocArray
public:
	ExtFieldsDialog() : PPListDialog(DLG_DLVREXTFLDS, CTL_LBXSEL_LIST)
	{
		if(P_Box) {
			CALLPTRMEMB(P_Box->def, SetOption(lbtFocNotify, 1));
		}
		selectCtrl(CTL_LBXSEL_LIST);
	}
	DECL_DIALOG_SETDTS()
	{
		Data.Z();
		RVALUEPTR(Data, pData);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();
	int    Edit(SStringTag * pItem);
};

//int ExtFieldsDialog::Edit(TaggedString * pItem)
int ExtFieldsDialog::Edit(SStringTag * pItem)
{
	class ExtFldCfgDialog : public TDialog {
		DECL_DIALOG_DATA(SStringTag); // @v10.7.11 TaggedString-->SStringTag
	public:
		explicit ExtFldCfgDialog(int isNew) : TDialog(DLG_EXTFLDCFG)
		{
			disableCtrl(CTL_EXTFLDCFG_ID, !isNew);
		}
		DECL_DIALOG_SETDTS()
		{
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			setCtrlData(CTL_EXTFLDCFG_ID,  &Data.Id);
			setCtrlString(CTL_EXTFLDCFG_NAME, Data);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(sel = CTL_EXTFLDCFG_ID,  &Data.Id);
			THROW_PP(Data.Id > LOCEXSTR_EXTFLDSOFFS && Data.Id <= LOCEXSTR_EXTFLDSOFFS + MAX_DLVRADDRFLDS, PPERR_INVEXTFLDIDRANGE);
			getCtrlString(sel = CTL_EXTFLDCFG_NAME, Data);
			THROW_PP(Data.Len(), PPERR_USERINPUT);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = (selectCtrl(sel), 0);
			ENDCATCH
			return ok;
		}
	};
	int    ok = -1;
	const  int is_new = BIN(Data.Search(pItem->Id, 0) <= 0);
	SString title;
	ExtFldCfgDialog * p_dlg = new ExtFldCfgDialog(is_new);
	if(CheckDialogPtrErr(&p_dlg) > 0) {
		p_dlg->setDTS(pItem);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			uint pos = 0;
			if(p_dlg->getDTS(pItem) <= 0)
				PPError();
			else if(is_new && Data.Search(pItem->Id, &pos) > 0)
				PPError(PPERR_DUPEXTID);
			else if(Data.SearchByText(*pItem, 1, &(pos = 0)) > 0 && Data.Get(pos).Id != pItem->Id)
				PPError(PPERR_DUPEXTFLD);
			else
				valid_data = ok = 1;
		}
	}
	else
		ok = PPErrorZ();
	return ok;
}

int ExtFieldsDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	SStringTag item; // @v10.7.11 TaggedString-->SStringTag
	// @v10.6.8 @ctr MEMSZERO(item);
	item.Id = 1 + LOCEXSTR_EXTFLDSOFFS;
	for(uint i = 0; i < Data.getCount(); i++)
		if(Data.Get(i).Id == item.Id)
			item.Id = Data.Get(i).Id + 1;
	if((item.Id - LOCEXSTR_EXTFLDSOFFS - 1) == MAX_DLVRADDRFLDS)
		PPError(PPERR_MAXEXTFLDACHIEVED);
	else if(Edit(&item) > 0) {
		Data.Add(item.Id, item);
		Data.SortByID();
		ok = 1;
	}
	return ok;
}

int ExtFieldsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < static_cast<long>(Data.getCount())) {
		StrAssocArray::Item li = Data.Get(pos);
		SStringTag temp_item;
		temp_item.Id = li.Id;
		static_cast<SString &>(temp_item) = li.Txt;
		ok = Edit(&temp_item);
		if(ok > 0) {
			Data.Add(temp_item.Id, temp_item, 1 /*replaceDup*/);
		}
	}
	return ok;
}

int ExtFieldsDialog::delItem(long pos, long id)
	{ return Data.AtFree(pos); }

int ExtFieldsDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < Data.getCount(); i++) {
		ss.clear();
		//TaggedString item = Data.at(i);
		StrAssocArray::Item item = Data.Get(i);
		ss.add(temp_buf.Z().Cat(item.Id));
		ss.add(item.Txt);
		if(!addStringToList(item.Id, ss.getBuf())) {
			ok = 0;
			PPError();
		}
	}
	return ok;
}

// @vmiller
class NewPersMarksDialog : public PPListDialog {
	DECL_DIALOG_DATA(TSVector <PPPersonConfig::NewClientDetectionItem>);
public:
	NewPersMarksDialog() : PPListDialog(DLG_NEWCLNT, CTL_LBXSEL_LIST)
	{
		if(P_Box) {
			CALLPTRMEMB(P_Box->def, SetOption(lbtFocNotify, 1));
		}
		selectCtrl(CTL_LBXSEL_LIST);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.clear();
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();
	int    Edit(PPPersonConfig::NewClientDetectionItem * pItem, SString & rStr);
};

class NewPersMarksFieldDialog : public TDialog {
	DECL_DIALOG_DATA(PPPersonConfig::NewClientDetectionItem);
public:
	explicit NewPersMarksFieldDialog(int isNew) : TDialog(DLG_NEWCNTCF)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		SString str, op_type;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		if(Data.Oi.Obj == PPOBJ_OPRKIND) {
			PPIDArray op_type_list;
			op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_DRAFTRECEIPT,
				PPOPT_DRAFTEXPEND, PPOPT_GENERIC, PPOPT_DRAFTQUOTREQ, 0L); // @v10.5.7 PPOPT_DRAFTQUOTREQ
			SetupOprKindCombo(this, CTLSEL_NEWCNTCF_OPKIND, Data.Oi.Id, 0, &op_type_list, 0);
		}
		else if(Data.Oi.Obj == PPOBJ_PERSONOPKIND) {
			Data.Oi.Obj = PPOBJ_PERSONOPKIND;
			SetupPPObjCombo(this, CTLSEL_NEWCNTCF_OPKIND, PPOBJ_PERSONOPKIND, Data.Oi.Id, 0, 0);
		}
		else if(Data.Oi.Obj == PPOBJ_SCARDSERIES) {
			Data.Oi.Obj = PPOBJ_SCARDSERIES;
			SetupPPObjCombo(this, CTLSEL_NEWCNTCF_OPKIND, PPOBJ_SCARDSERIES, Data.Oi.Id, 0, 0);
		}
		SetupStringCombo(this, CTLSEL_NEWCNTCF_OPTYPE, PPTXT_NEWCLNT_TRANSTYPE, Data.Oi.Obj);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_NEWCNTCF_OPTYPE, &Data.Oi.Obj);
		getCtrlData(sel = CTL_NEWCNTCF_OPKIND, &Data.Oi.Id);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_NEWCNTCF_OPTYPE)) {
			long   obj_type = getCtrlLong(CTLSEL_NEWCNTCF_OPTYPE);
			if(obj_type != Data.Oi.Obj) {
				switch(obj_type) {
					case PPOBJ_PERSONOPKIND:
						Data.Oi.Obj = obj_type;
						SetupPPObjCombo(this, CTLSEL_NEWCNTCF_OPKIND, PPOBJ_PERSONOPKIND, Data.Oi.Id = 0, 0, 0);
						break;
					case PPOBJ_OPRKIND:
						{
							Data.Oi.Obj = obj_type;
							PPIDArray op_type_list;
							op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
								PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_GENERIC, PPOPT_DRAFTQUOTREQ, 0L); // @v10.5.7 PPOPT_DRAFTQUOTREQ
							SetupOprKindCombo(this, CTLSEL_NEWCNTCF_OPKIND, Data.Oi.Id = 0, 0, &op_type_list, 0);
						}
						break;
					case PPOBJ_SCARDSERIES:
						Data.Oi.Obj = obj_type;
						SetupPPObjCombo(this, CTLSEL_NEWCNTCF_OPKIND, PPOBJ_SCARDSERIES, Data.Oi.Id = 0, 0, 0);
						break;
					default:
						break;
				}
			}
		}
		else
			return;
		clearEvent(event);

	}
};

int NewPersMarksDialog::Edit(PPPersonConfig::NewClientDetectionItem * pItem, SString & rStr)
{
	int    ok = -1;
	int    item_found = 0;
	// Через Search не сделать, потому что если в массиве и будут записи с одинаковыми ID, то
	// поиск все равно будет возвращать только самую первую найденную запись
	for(uint i = 0; (i < Data.getCount()) && !item_found; i++) {
		if((Data.at(i).Oi.Id == pItem->Oi.Id) && ((Data.at(i).Oi.Obj == pItem->Oi.Obj)))
			item_found = 1;
	}
	const int is_new = BIN(!item_found && Data.getCount());
	NewPersMarksFieldDialog * p_dlg = new NewPersMarksFieldDialog(is_new);
	if(CheckDialogPtrErr(&p_dlg) > 0) {
		p_dlg->setDTS(pItem);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			uint pos = 0;
			if(p_dlg->getDTS(pItem))
				if(is_new) {
					item_found = 0;
					for(uint i = 0; (i < Data.getCount()) && !item_found; i++) {
						if((Data.at(i).Oi.Id == pItem->Oi.Id) && ((Data.at(i).Oi.Obj == pItem->Oi.Obj)))
							item_found = 1;
					}
					if(item_found)
						PPError(PPERR_DUPEXTID);
					else
						valid_data = ok = 1;
				}
				else
					valid_data = ok = 1;
			else
				PPError();
		}
	}
	else
		ok = PPErrorZ();
	delete p_dlg;
	return ok;
}

int NewPersMarksDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPPersonConfig::NewClientDetectionItem item;
	SString op_kind_name;
	// @v10.8.12 @ctr MEMSZERO(item);
	if(Edit(&item, op_kind_name) > 0) {
		if(Data.insert(&item)) {
			assert(Data.getCount() > 0);
			ASSIGN_PTR(pPos, (Data.getCountI() - 1));
			ASSIGN_PTR(pID, item.Oi.Id);
			ok = 1;
		}
		else
			ok = PPSetErrorSLib();
	}
	return ok;
}

int NewPersMarksDialog::editItem(long pos, long id)
{
	SString str;
	return (pos >= 0 && pos < (long)Data.getCount()) ? Edit(&Data.at(pos), str) : -1;
}

int NewPersMarksDialog::delItem(long pos, long id)
{
	return Data.atFree(pos);
}

int NewPersMarksDialog::setupList()
{
	int    ok = 1;
	PPObjPsnOpKind pok_obj;
	SString title_line_buf, title_item_buf, title_id_buf, title_txt_buf, temp_buf;
	PPLoadText(PPTXT_NEWCLNT_TRANSTYPE, title_line_buf);
	for(uint i = 0; ok && i < Data.getCount(); i++) {
		StringSet ss(SLBColumnDelim);
		PPObjID item = Data.at(i).Oi;
		temp_buf.Z();
		for(uint idx = 0; PPGetSubStr(title_line_buf, idx, title_item_buf) > 0; idx++)
			if(title_item_buf.Divide(',', title_id_buf, title_txt_buf) > 0 && title_id_buf.ToLong() == item.Obj) {
				temp_buf = title_txt_buf;
				break;
			}
		if(temp_buf.IsEmpty())
			temp_buf.CatChar('#').Cat("OBJ").Space().Cat(item.Obj);
		ss.add(temp_buf);
		if(item.Obj == PPOBJ_PERSONOPKIND) {
			PPPsnOpKind pok_rec;
			// @v10.6.6 @ctr MEMSZERO(pok_rec);
			pok_obj.Fetch(item.Id, &pok_rec);
			ss.add(pok_rec.Name);
		}
		else if(item.Obj == PPOBJ_OPRKIND) {
			PPOprKind op_kind;
			GetOpName(item.Id, temp_buf.Z());
			ss.add(temp_buf);
		}
		else if(item.Obj == PPOBJ_SCARDSERIES) {
			if(item.Id)
				GetObjectName(item.Obj, item.Id, temp_buf.Z());
			else
				PPLoadString("all", temp_buf);
			ss.add(temp_buf);
		}
		else {
			temp_buf = "#unkn";
			ss.add(temp_buf);
		}
		if(!addStringToList(item.Id, ss.getBuf())) {
			ok = 0;
			PPError();
		}
	}
	return ok;
}

PPPersonConfig::NewClientDetectionItem::NewClientDetectionItem() : Flags(0)
{
}

PPPersonConfig::PPPersonConfig()
{
	Init();
}

void PPPersonConfig::Init()
{
	memzero(this, offsetof(PPPersonConfig, TopFolder));
	TopFolder.Z();
	AddImageFolder.Z();
	// @v10.7.11 DlvrAddrExtFldList.freeAll();
	DlvrAddrExtFldList.Z(); // @v10.7.11
	NewClientDetectionList.freeAll();
}

/*static*/int PPObjPerson::EditConfig()
{
	class PersonCfgDialog : public TDialog {
		DECL_DIALOG_DATA(PPPersonConfig);
		enum {
			ctlgroupImgFold    = 1,
			ctlgroupPersonFold = 2
		};
	public:
		PersonCfgDialog() : TDialog(DLG_PSNCFG)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_PSNCFG_FOLDER, CTL_PSNCFG_FOLDER, ctlgroupPersonFold, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
			FileBrowseCtrlGroup::Setup(this, CTLBRW_PSNCFG_IMGFOLDER, CTL_PSNCFG_IMGFOLDER, ctlgroupImgFold, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
			enableCommand(cmOK, CheckCfgRights(PPCFGOBJ_PERSON, PPR_MOD, 0));
		}
		DECL_DIALOG_SETDTS()
		{
			if(!RVALUEPTR(Data, pData))
				Data.Init();
			setCtrlString(CTL_PSNCFG_FOLDER,    Data.TopFolder);
			setCtrlString(CTL_PSNCFG_IMGFOLDER, Data.AddImageFolder);
			SetupPPObjCombo(this, CTLSEL_PSNCFG_TRADELIC, PPOBJ_REGISTERTYPE, Data.TradeLicRegTypeID, OLW_CANINSERT, 0);
			SetupPPObjCombo(this, CTLSEL_PSNCFG_REGCAL, PPOBJ_STAFFCAL,       Data.RegStaffCalID,     OLW_CANINSERT, 0);
			setCtrlData(CTL_PSNCFG_CALQUANT, &Data.StaffCalQuant);
			setCtrlData(CTL_PSNCFG_SMSTIMEOUT, &Data.SendSmsSamePersonTimeout);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 0, PPPersonConfig::fSyncByName);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 1, PPPersonConfig::fSyncBySrchReg);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 2, PPPersonConfig::fSyncDeclineUpdate);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 3, PPPersonConfig::fShowPsnImageAfterCmdAssoc);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 4, PPPersonConfig::fSyncMergeRegList);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 5, PPPersonConfig::fSendAttachment);
			AddClusterAssoc(CTL_PSNCFG_FLAGS, 6, PPPersonConfig::fSyncAppendAbsKinds); // @v10.3.0
			SetClusterData(CTL_PSNCFG_FLAGS, Data.Flags);
			SetTimeRangeInput(this, CTL_PSNCFG_SMSPRTR, TIMF_HM, &Data.SmsProhibitedTr); // @v10.2.3
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlString(CTL_PSNCFG_FOLDER,    Data.TopFolder);
			getCtrlData(CTLSEL_PSNCFG_TRADELIC, &Data.TradeLicRegTypeID);
			getCtrlData(CTLSEL_PSNCFG_REGCAL,   &Data.RegStaffCalID);
			getCtrlData(CTL_PSNCFG_CALQUANT,    &Data.StaffCalQuant);
			getCtrlData(CTL_PSNCFG_SMSTIMEOUT,  &Data.SendSmsSamePersonTimeout);
			GetClusterData(CTL_PSNCFG_FLAGS,    &Data.Flags);
			GetTimeRangeInput(this, CTL_PSNCFG_SMSPRTR, TIMF_HM, &Data.SmsProhibitedTr); // @v10.2.3
			getCtrlString(CTL_PSNCFG_IMGFOLDER,  Data.AddImageFolder);
			THROW_PP_S(!Data.AddImageFolder.Len() || pathValid(Data.AddImageFolder, 1), PPERR_DIRNOTEXISTS, Data.AddImageFolder);
			ASSIGN_PTR(pData, Data);
			CATCHZOK
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmExtFields)) {
				ExtFieldsDialog * p_dlg = new ExtFieldsDialog;
				if(CheckDialogPtrErr(&p_dlg) > 0) {
					p_dlg->setDTS(&Data.DlvrAddrExtFldList);
					for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
						if(p_dlg->getDTS(&Data.DlvrAddrExtFldList) > 0)
							valid_data = 1;
						else
							PPError();
					}
				}
				delete p_dlg;
				clearEvent(event);
			}
			// @vmiller {
			else if(event.isCmd(cmNewPersnMarks)) {
				NewPersMarksDialog * p_dlg = new NewPersMarksDialog;
				if(CheckDialogPtrErr(&p_dlg) > 0) {
					p_dlg->setDTS(&Data.NewClientDetectionList);
					for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
						if(p_dlg->getDTS(&Data.NewClientDetectionList) > 0)
							valid_data = 1;
						else
							PPError();
					}
				}
				delete p_dlg;
				clearEvent(event);
			}
			// } @vmiller
		}
	};
	int    ok = -1;
	PersonCfgDialog * dlg = 0;
	PPPersonConfig cfg;
	THROW(CheckCfgRights(PPCFGOBJ_PERSON, PPR_READ, 0));
	THROW(ReadConfig(&cfg));
	THROW(CheckDialogPtr(&(dlg = new PersonCfgDialog)));
	dlg->setDTS(&cfg);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_PERSON, PPR_MOD, 0));
		if(dlg->getDTS(&cfg) > 0 && WriteConfig(&cfg, 1)) {
			PPObjPerson psn_obj;
			psn_obj.DirtyConfig();
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

TLP_IMPL(PPObjPerson, PersonCore, P_Tbl);

PPObjPerson::PPObjPerson(SCtrLite sctr) : PPObject(PPOBJ_PERSON), LocObj(sctr), P_ArObj(0), P_PrcObj(0), ExtraPtr(0),
	DoObjVer_Person(CConfig.Flags2 & CCFLG2_USEHISTPERSON) // @v10.5.3
{
	TLP_OPEN(P_Tbl);
	Cfg.Flags &= ~PPPersonConfig::fValid;
}

PPObjPerson::PPObjPerson(void * extraPtr) : PPObject(PPOBJ_PERSON), LocObj(), P_ArObj(new PPObjArticle), P_PrcObj(new PPObjProcessor), ExtraPtr(extraPtr),
	DoObjVer_Person(CConfig.Flags2 & CCFLG2_USEHISTPERSON) // @v10.5.3
{
	TLP_OPEN(P_Tbl);
	Cfg.Flags &= ~PPPersonConfig::fValid;
}

PPObjPerson::~PPObjPerson()
{
	delete P_ArObj;
	delete P_PrcObj;
	TLP_CLOSE(P_Tbl);
}

int PPObjPerson::IsPacketEq(const PPPersonPacket & rS1, const PPPersonPacket & rS2, long flags)
{
	int    eq = 1;
	if(!P_Tbl->GetFields().IsEqualRecords(&rS1.Rec, &rS2.Rec))
		eq = 0;
	else if(rS1.SMemo != rS2.SMemo) // @v11.1.12
		eq = 0;
	else if(rS1.Kinds != rS2.Kinds)
		eq = 0;
	else if(!rS1.Regs.IsEq(rS2.Regs))
		eq = 0;
	else if(!LocObj.IsPacketEq(rS1.Loc, rS2.Loc, 0))
		eq = 0;
	else if(!LocObj.IsPacketEq(rS1.RLoc, rS2.RLoc, 0))
		eq = 0;
	else if(!rS1.ELA.IsEq(rS2.ELA))
		eq = 0;
	else if(!rS1.CshrInfo.IsEq(rS2.CshrInfo))
		eq = 0;
	else if(!rS1.TagL.IsEq(rS2.TagL))
		eq = 0;
	else if(rS1.LinkFiles.IsChanged(rS1.Rec.ID, 0L))
		eq = 0;
	else if(!rS1.Amounts.IsEq(rS2.Amounts))
		eq = 0;
	if(eq) {
		const LAssocArray & rl1 = rS1.GetRelList();
		const LAssocArray & rl2 = rS2.GetRelList();
		if(!(rl1 == rl2))
			eq = 0;
	}
	if(eq) {
		const uint c1 = rS1.GetDlvrLocCount();
		const uint c2 = rS2.GetDlvrLocCount();
		if(c1 != c2)
			eq = 0;
		else {
			PPLocationPacket d1, d2;
			uint   p1 = 0, p2 = 0;
			while(eq && rS1.EnumDlvrLoc(&p1, &d1) && rS2.EnumDlvrLoc(&p2, &d2)) {
				// if(!LocationCore::IsEqualRec(d1, d2))
				if(!LocObj.IsPacketEq(d1, d2, 0))
					eq = 0;
			}
			assert(p1 == p2);
		}
	}
	if(eq) {
		SString en1, en2;
		rS1.GetExtName(en1);
		rS2.GetExtName(en2);
		if(en1 != en2)
			eq = 0;
	}
	return eq;
}

int PPObjPerson::Search(PPID id, void * b) { return SearchByID(P_Tbl, Obj, id, b); }
int PPObjPerson::DeleteObj(PPID id) { return PutPacket(&id, 0, 0); }

ListBoxDef * PPObjPerson::_Selector2(ListBoxDef * pDef, long flags, void * extraPtr)
{
	const PPID kind_id = reinterpret_cast<PPID>(extraPtr);
	struct LbxDataPerson {
		long   KindID;
	} lbx_extra;
	ListBoxDef  * p_def = 0;
	StrAssocArray * p_array = 0;
	PersonTbl * t = 0;
	DBQuery   * p_q = 0;
	if(pDef) {
		size_t s = sizeof(lbx_extra);
		MEMSZERO(lbx_extra);
		pDef->GetUserData(&lbx_extra, &s);
	}
	else {
		lbx_extra.KindID = kind_id;
	}
	if(lbx_extra.KindID) {
		const int method_8912 = 0; // Новый метод в 3 с лишним раза медленнее. Потому пока его отключает.
			// Однако после перевода PersonKind в ObjAssoc придется к нему вернуться.
		SString text;
		PPID   code_reg_type_id = 0;
		/* @v8.9.12 Поддержку префиксов регистров в списках снимаем
		if(DS.CheckExtFlag(ECF_CODEPREFIXEDLIST)) {
			PPObjPersonKind pk_obj;
			PPPersonKind pk_rec;
            if(pk_obj.Fetch(lbx_extra.KindID, &pk_rec) > 0)
            	code_reg_type_id = pk_rec.CodeRegTypeID;
		}
		*/
		THROW_MEM(p_array = new StrAssocArray);
		{
			PROFILE_START
			if(method_8912) {
				THROW(GetListByKind(kind_id, 0, p_array));
			}
			else {
				PersonKindTbl * t = &P_Tbl->Kind;
				PersonKindTbl::Key0 k0;
				MEMSZERO(k0);
				k0.KindID = kind_id;
				BExtQuery q(t, 0, 128);
				q.select(t->PersonID, t->Name, 0).where(t->KindID == lbx_extra.KindID);
				for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
					const PPID psn_id = t->data.PersonID;
					if(code_reg_type_id) {
						RegisterTbl::Rec reg_rec;
						if(GetRegister(psn_id, code_reg_type_id, &reg_rec) > 0)
							(text = reg_rec.Num).Strip().CatCharN(' ', 3);
						else
							text = 0;
						text.Cat(t->data.Name);
						THROW_SL(p_array->AddFast(t->data.PersonID, text));
					}
					else {
						THROW_SL(p_array->AddFast(t->data.PersonID, t->data.Name));
					}
				}
			}
			PROFILE_END
		}
	}
	else {
		RECORDNUMBER nr = 0;
		P_Tbl->getNumRecs(&nr);
		if(nr > 200000) {
			if(pDef)
				pDef->refresh();
			else {
				THROW(CheckTblPtr(t = new PersonTbl));
				p_q = &select(t->ID, t->Name, 0L).from(t, 0L).orderBy(t->Name, 0L);
				THROW(CheckQueryPtr(p_q));
			}
		}
		else {
			PersonCore * p_tbl = P_Tbl;
			PersonTbl::Key1 k1;
			MEMSZERO(k1);
			BExtQuery q(p_tbl, 1, 128);
			q.select(p_tbl->ID, p_tbl->Name, 0);
			THROW_MEM(p_array = new StrAssocArray);
			for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
				THROW_SL(p_array->AddFast(p_tbl->data.ID, p_tbl->data.Name));
			}
		}
	}
	if(!pDef) {
		if(p_q)
			p_def = new DBQListBoxDef(*p_q, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		else if(p_array) {
			p_array->SortByText();
			// @v11.1.10 {
			if(flags & OLW_INSCONTEXTEDITEMS) {
				p_array->AddFast(ROBJID_CONTEXT, "#BYCONTEXT");
				p_array->Move(p_array->getCount()-1, 0);
			}
			// } @v11.1.10
			p_def = new StrAssocListBoxDef(p_array, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
		}
		THROW_MEM(p_def);
		p_def->SetUserData(&lbx_extra, sizeof(lbx_extra));
	}
	else {
		if(p_array) {
			p_array->SortByText();
			// @v11.1.10 {
			if(flags & OLW_INSCONTEXTEDITEMS) {
				p_array->AddFast(ROBJID_CONTEXT, "#BYCONTEXT");
				p_array->Move(p_array->getCount()-1, 0);
			}
			// } @v11.1.10
			static_cast<StrAssocListBoxDef *>(pDef)->setArray(p_array);
		}
		p_def = pDef;
	}
	CATCH
		if(p_def)
			ZDELETE(p_def);
		else if(p_q)
			delete p_q;
		else {
			delete p_array;
			delete t;
		}
	ENDCATCH
	return p_def;
}

int PPObjPerson::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
	{ return EditSpcRightFlags(DLG_RTPERSON, 0, 0, bufSize, rt, pDlg); }
const char * PPObjPerson::GetNamePtr() { return P_Tbl->data.Name; }
ListBoxDef * PPObjPerson::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr) { return _Selector2(pOrgDef, flags, extraPtr); }
// @v11.1.10 int PPObjPerson::UpdateSelector_Obsolete(ListBoxDef * pDef, long flags, void * extraPtr) { return BIN(_Selector2(pDef, extraPtr)); }
int PPObjPerson::Browse(void * extraPtr) { return ViewPerson(0); }

const PPPersonConfig & PPObjPerson::GetConfig()
{
	if(!(Cfg.Flags & PPPersonConfig::fValid))
		PPObjPerson::FetchConfig(&Cfg);
	return Cfg;
}
//
//
//
int PPObjPerson::GetStatus(PPID id, PPID * pStatusID, int * pIsPrivate)
{
	PPID   status_id = 0;
	int    prv = -1;
	PersonTbl::Rec psn_rec;
	int    r = Search(id, &psn_rec);
	if(r > 0) {
		PPObjPersonStatus ps_obj;
		PPPersonStatus strec;
		status_id = psn_rec.Status;
		if(status_id > 0 && ps_obj.Fetch(status_id, &strec) > 0)
		   prv = BIN(strec.Flags & PSNSTF_PRIVATE);
	}
	ASSIGN_PTR(pStatusID, status_id);
	ASSIGN_PTR(pIsPrivate, prv);
	return r;
}

int PPObjPerson::GetCountry(PPID id, PPID * pCountryID, PPCountryBlock * pBlk)
{
	CALLPTRMEMB(pBlk, Z());
	PersonTbl::Rec rec;
	if(id && Fetch(id, &rec) > 0) {
		if(rec.Status == PPPRS_COUNTRY) {
			if(pBlk) {
				RegisterArray reg_list;
				if(GetRegList(id, &reg_list, 0) > 0) {
					RegisterTbl::Rec reg_rec;
					if(reg_list.GetRegister(PPREGT_COUNTRYABBR, 0, &reg_rec) > 0)
						(pBlk->Abbr = reg_rec.Num).Strip();
					if(reg_list.GetRegister(PPREGT_COUNTRYCODE, 0, &reg_rec) > 0) {
						(pBlk->Code = reg_rec.Num).Strip();
						// @v9.7.8 {
						// Россия = 643
						pBlk->IsNative = 1;
						if(pBlk->Code.NotEmpty() && DS.GetConstTLA().MainOrgCountryCode != pBlk->Code)
							pBlk->IsNative = 0;
						// } @v9.7.8
					}
				}
				(pBlk->Name = rec.Name).Strip();
			}
			ASSIGN_PTR(pCountryID, id);
			return 2;
		}
		else
			return LocObj.GetCountry(rec.MainLoc, pCountryID, pBlk);
	}
	ASSIGN_PTR(pCountryID, 0);
	return -1;
}

int PPObjPerson::GetCityByAddr(PPID addrID, PPID * pCityID, SString * pCityName, int useLocCityCache)
{
	return LocObj.GetCity(addrID, pCityID, pCityName, useLocCityCache);
}

int PPObjPerson::AdjustLocationOwner(LocationTbl::Rec & rLocRec)
{
    int    ok = -1;
    if(rLocRec.OwnerID == 0 && rLocRec.ID != 0) {
		PersonTbl::Key3 k3;
		k3.MainLoc = rLocRec.ID;
		if(P_Tbl->search(3, &k3, spEq)) {
            rLocRec.OwnerID = P_Tbl->data.ID;
            if(P_Tbl->search(3, &k3, spNext) && P_Tbl->data.MainLoc == rLocRec.ID)
				ok = 2;
			else
				ok = 1;
		}
		else {
			PersonTbl::Key4 k4;
			k4.RLoc = rLocRec.ID;
			if(P_Tbl->search(4, &k4, spEq)) {
				rLocRec.OwnerID = P_Tbl->data.ID;
				if(P_Tbl->search(4, &k4, spNext) && P_Tbl->data.RLoc == rLocRec.ID)
					ok = 2;
				else
					ok = 1;
			}
		}
    }
    return ok;
}

int PPObjPerson::GetAddress(PPID id, SString & rBuf)
{
	rBuf.Z();
	PersonTbl::Rec rec;
	return (id && Search(id, &rec) > 0 && rec.MainLoc) ? LocObj.GetAddress(rec.MainLoc, 0, rBuf) : -1;
}
//
//
//
PPBank::PPBank() : ID(0)
{
	PTR32(Name)[0] = 0;
	PTR32(BIC)[0] = 0;
	PTR32(CorrAcc)[0] = 0;
	PTR32(City)[0] = 0;
	PTR32(ExtName)[0] = 0;
}

BnkAcctData::BnkAcctData(long initFlags) : InitFlags(initFlags), BnkAcctID(0), OwnerID(0)
{
	PTR32(Acct)[0] = 0;
}

int BnkAcctData::Format(const char * pTitle, char * pBuf, size_t bufLen) const
{
	SString buf, temp_buf;
	if(pTitle)
		buf.Cat(pTitle);
	if((temp_buf = Acct).NotEmptyS())
		buf.CatDiv('N', 1).Cat(temp_buf);
	if(InitFlags & BADIF_INITALLBR) {
		SString w_buf;
		if(InitFlags & BADIF_INITBNAME && (temp_buf = Bnk.Name).NotEmptyS()) {
			PPLoadString("prep_in", w_buf);
			buf.Space().Cat(w_buf).Space().Cat(temp_buf);
		}
		if(InitFlags & BADIF_INITBIC && (temp_buf = Bnk.BIC).NotEmptyS()) {
			PPLoadString("bic", w_buf);
			buf.Space().Cat(w_buf).Space().Cat(temp_buf);
		}
		if(InitFlags & BADIF_INITCACC && (temp_buf = Bnk.CorrAcc).NotEmptyS()) {
			PPLoadString("correspondentaccount_ss", w_buf);
			buf.Space().Cat(w_buf).Space().Cat(temp_buf);
		}
	}
	buf.CopyTo(pBuf, bufLen);
	return 1;
}

PersonReq::PersonReq() : Flags(0), AddrID(0), RAddrID(0), SrchRegTypeID(0)
{
	PTR32(Name)[0] = 0;
	PTR32(ExtName)[0] = 0;
	PTR32(Addr)[0] = 0;
	PTR32(RAddr)[0] = 0;
	PTR32(Phone1)[0] = 0;
	PTR32(TPID)[0] = 0;
	PTR32(KPP)[0] = 0;
	PTR32(OKONF)[0] = 0;
	PTR32(OKPO)[0] = 0;
	PTR32(SrchCode)[0] = 0;
	PTR32(Memo)[0] = 0;
}
//
//
//
int PPObjPerson::GetBnkAcctData(PPID bnkAcctID, const PPBankAccount * pBaRec, BnkAcctData * pData)
{
	int    ok = 1;
	PPBankAccount ba_rec;
	RegisterTbl::Rec ba_reg_rec;
	long   save_fl = pData->InitFlags;
	memzero(pData, sizeof(*pData));
	pData->InitFlags = save_fl;
	pData->BnkAcctID = bnkAcctID;
	if(pBaRec || (RegObj.Search(bnkAcctID, &ba_reg_rec) > 0 && ba_reg_rec.RegTypeID == PPREGT_BANKACCOUNT)) {
		if(!pBaRec) {
			ba_rec = ba_reg_rec;
			pBaRec = & ba_rec;
		}
		pData->OwnerID = pBaRec->PersonID;
		STRNSCPY(pData->Acct, pBaRec->Acct);
		pData->Bnk.ID = pBaRec->BankID;
		if(pData->InitFlags & BADIF_INITALLBR)
			GetBankData(pBaRec->BankID, &pData->Bnk);
	}
	else
		ok = -1;
	return ok;
}

int PPObjPerson::GetListByKind(PPID psnKindID, PPIDArray * pList, StrAssocArray * pNameList)
{
	CALLPTRMEMB(pList, clear());
	CALLPTRMEMB(pNameList, Z());
	int    ok = -1;
	PPIDArray temp_list;
	THROW(P_Tbl->GetListByKind(psnKindID, &temp_list));
	{
		const uint c = temp_list.getCount();
		if(c) {
			ok = 1;
			temp_list.sortAndUndup();
			if(pNameList || pList) {
				PersonTbl::Rec psn_rec;
				if(pNameList) {
					if(c <= 1000) {
						for(uint i = 0; i < c; i++) {
							const PPID id = temp_list.get(i);
							if(Fetch(id, &psn_rec) > 0) {
								if(pList) {
									THROW_SL(pList->add(id));
								}
								THROW_SL(pNameList->AddFast(id, psn_rec.Name));
							}
						}
					}
					else {
						const PPID id_min = temp_list.get(0);
						const PPID id_max = temp_list.get(c-1);
						BExtQuery qp(P_Tbl, 0);
						qp.select(P_Tbl->ID, P_Tbl->Name, 0L).where(P_Tbl->ID >= id_min && P_Tbl->ID <= id_max);
						PersonTbl::Key0 k0;
						MEMSZERO(k0);
						k0.ID = id_min;
						for(qp.initIteration(false, &k0, spGe); qp.nextIteration() > 0;) {
							const PPID id = P_Tbl->data.ID;
							if(temp_list.bsearch(id)) {
								if(pList) {
									THROW_SL(pList->add(id));
								}
								THROW_SL(pNameList->AddFast(id, P_Tbl->data.Name));
							}
						}
					}
				}
				else {
					ASSIGN_PTR(pList, temp_list);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::GetListBySubstring(const char * pSubstr, PPID kindID, StrAssocArray * pList, int /*fromBegStr*/)
{
	const SString pattern(pSubstr);
	PPIDArray list_by_kind;
	PersonTbl * t = P_Tbl;
	PersonTbl::Key1 k1;
	BExtQuery pq(t, 0, 128);
	pq.select(t->ID, t->Name, 0L);
	if(kindID) {
		GetListByKind(kindID, &list_by_kind, 0);
		list_by_kind.sortAndUndup();
	}
	MEMSZERO(k1);
	for(pq.initIteration(false, &k1, spFirst); pq.nextIteration() > 0;) {
		const PPID id = t->data.ID;
		if((!kindID || list_by_kind.bsearch(id)) && ExtStrSrch(t->data.Name, pattern, 0)) {
			pList->AddFast(id, t->data.Name);
		}
	}
	return 1;
}

int PPObjPerson::GetListByPattern(const SrchAnalogPattern * pPattern, PPIDArray * pList)
{
	SString pat_name, tbl_name;
	if(pPattern->Flags & PPObjPerson::sapfMatchWholeWord) {
		pat_name.Space().Cat(pPattern->NamePattern).Space();
	}
	else
		pat_name = pPattern->NamePattern;
	PersonTbl * t = P_Tbl;
	PersonTbl::Key1 k1;
	BExtQuery pq(t, 0, 64);
	pq.select(t->ID, t->Name, 0L);
	MEMSZERO(k1);
	for(pq.initIteration(false, &k1, spFirst); pq.nextIteration() > 0;) {
		if(pPattern->Flags & PPObjPerson::sapfMatchWholeWord) {
			tbl_name.Z().Space().Cat(t->data.Name).Space();
		}
		else
			tbl_name = t->data.Name;
		if(ExtStrSrch(tbl_name, pat_name, 0))
			pList->add(t->data.ID);
	}
	return 1;
}

int PPObjPerson::GetListByRegNumber(PPID regTypeID, PPID kindID, const char * pNumber, PPIDArray & rList)
{
	return GetListByRegNumber(regTypeID, kindID, 0, pNumber, rList);
}

int PPObjPerson::GetListByRegNumber(PPID regTypeID, PPID kindID, const char * pSerial, const char * pNumber, PPIDArray & rList)
{
	rList.clear();
	int    ok = 1;
	SString msg_buf;
	RegisterFilt reg_flt;
	reg_flt.Oid.Obj = PPOBJ_PERSON; // @v10.0.1
	reg_flt.RegTypeID = regTypeID;
	reg_flt.SerPattern = pSerial;
	reg_flt.NmbPattern = pNumber;
	int    r = RegObj.SearchByFilt(&reg_flt, 0, &rList);
	if(r == 0)
		ok = 0;
	else if(r < 0) {
		PPSetError(PPERR_PERSONBYREGNFOUND, msg_buf.Cat(pSerial).CatDivIfNotEmpty(':', 1).Cat(pNumber));
		ok = -1;
	}
	else if(kindID) {
		PPID   single_id = rList.getSingle();
		uint   c = rList.getCount();
		if(c) do {
			if(P_Tbl->IsBelongToKind(rList.at(--c), kindID) <= 0)
				rList.atFree(c);
		} while(c);
		if(!rList.getCount()) {
			if(single_id) {
				PersonTbl::Rec psn_rec;
				Fetch(single_id, &psn_rec);
				PPSetError(PPERR_PERSONBYREGNKIND, psn_rec.Name);
			}
			else
				PPSetError(PPERR_PERSONSBYREGNKIND, msg_buf.Cat(pSerial).CatDivIfNotEmpty(':', 1).Cat(pNumber));
			ok = -1;
		}
	}
	return ok;
}

int PPObjPerson::GetRegList(PPID personID, RegisterArray * pList, int useInheritence)
{
	int    ok = RegObj.P_Tbl->GetByPerson(personID, pList);
	if(useInheritence && pList) {
		PPObjPersonRelType prt_obj;
		PPPersonRelTypePacket prt_pack;
		LAssocArray rel_list;
		RegisterArray reg_list;
		LAssoc * p_rel;
		P_Tbl->GetRelList(personID, &rel_list, 0);
		for(uint i = 0; rel_list.enumItems(&i, (void **)&p_rel);) {
			if(prt_obj.Fetch(p_rel->Val, &prt_pack) > 0) {
				for(uint j = 0; j < prt_pack.InhRegTypeList.getCount(); j++) {
					const PPID reg_type_id = prt_pack.InhRegTypeList.at(j);
					reg_list.clear();
					if(pList->GetRegister(reg_type_id, 0, 0) <= 0 && RegObj.P_Tbl->GetByPerson(p_rel->Key, &reg_list) > 0) {
						for(uint k = 0; k < reg_list.getCount(); k++) {
							const RegisterTbl::Rec & r_item = reg_list.at(k);
							if(r_item.RegTypeID == reg_type_id) {
								RegisterTbl::Rec reg_rec = r_item;
								reg_rec.Flags |= PREGF_INHERITED;
								pList->insert(&reg_rec);
								ok = 1; // @v9.2.9 @fix
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int PPObjPerson::GetRegister(PPID personID, PPID regType, RegisterTbl::Rec * pRec)
{
	RegisterArray reg_list;
	return (GetRegList(personID, &reg_list, 1) > 0) ? reg_list.GetRegister(regType, 0, pRec) : -1;
}

int PPObjPerson::GetRegister(PPID personID, PPID regType, LDATE actualDate, RegisterTbl::Rec * pRec)
{
	RegisterArray reg_list;
	if(GetRegList(personID, &reg_list, 1) > 0) {
		reg_list.Sort();
		return reg_list.GetRegister(regType, actualDate, 0, pRec);
	}
	else
		return -1;
}

int PPObjPerson::GetRegNumber(PPID personID, PPID regType, SString & rBuf)
{
	RegisterArray reg_list;
	rBuf.Z();
	return (GetRegList(personID, &reg_list, 1) > 0) ? reg_list.GetRegNumber(regType, rBuf) : -1;
}

int PPObjPerson::GetRegNumber(PPID personID, PPID regType, LDATE actualDate, SString & rBuf)
{
	RegisterArray reg_list;
	rBuf.Z();
	return (GetRegList(personID, &reg_list, 1) > 0) ? reg_list.GetRegNumber(regType, actualDate, rBuf) : -1;
}

int PPObjPerson::ResolveGLN(const char * pGLN, PPID * pID)
{
	int    ok = -1;
	assert(pGLN);
	SString code(pGLN);
	THROW_INVARG(pGLN);
	if(code.NotEmptyS()) {
		const PPID reg_type_id = PPREGT_GLN;
		PPIDArray psn_list;
		THROW(GetListByRegNumber(reg_type_id, 0, code, psn_list));
		if(psn_list.getCount()) {
			ASSIGN_PTR(pID, psn_list.at(0));
			ok = (psn_list.getCount() > 1) ? 2 : 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::ResolveGLN_Article(const char * pGLN, PPID accSheetID, PPID * pArID)
{
	int    ok = -1;
	assert(pGLN);
	SString code(pGLN);
	THROW_INVARG(pGLN);
	if(code.NotEmptyS()) {
		const PPID reg_type_id = PPREGT_GLN;
		PPIDArray psn_list, ar_list;
		THROW(GetListByRegNumber(reg_type_id, 0, code, psn_list));
		if(psn_list.getCount()) {
			PPObjArticle ar_obj;
			THROW(ar_obj.GetByPersonList(accSheetID, &psn_list, &ar_list));
			if(ar_list.getCount()) {
				ASSIGN_PTR(pArID, ar_list.at(0));
				ok = (ar_list.getCount() > 1) ? 2 : 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::SearchFirstByName(const char * pName, const PPIDArray * pKindList, PPID exclID, PersonTbl::Rec * pRec)
{
	int    ok = -1;
	char   pattern[256];
	const  uint kind_count = SVectorBase::GetCount(pKindList);
	PersonTbl::Key1 k;
	strip(STRNSCPY(pattern, pName));
	MEMSZERO(k);
	STRNSCPY(k.Name, pattern);
	if(P_Tbl->search(1, &k, spEq)) {
		do {
			const PPID id = P_Tbl->data.ID;
			if(!exclID || id != exclID) {
				if(kind_count) {
					for(uint i = 0; ok < 0 && i < kind_count; i++) {
						if(P_Tbl->IsBelongToKind(id, pKindList->get(i)))
							ok = 1;
					}
				}
				else
					ok = 1;
				if(ok > 0)
					P_Tbl->copyBufTo(pRec);
			}
		} while(ok < 0 && P_Tbl->search(1, &k, spNext) && stricmp866(k.Name, pattern) == 0);
	}
	return ok;
}

int PPObjPerson::SearchMaxLike(const PPPersonPacket * p, PPID * pID, long flags, PPID regTypeID)
{
	SString reg_buf;
	PPID   id;
	if(regTypeID) {
		if(p->GetRegNumber(regTypeID, reg_buf) > 0) {
			PPIDArray list;
			if(GetListByRegNumber(regTypeID, 0, reg_buf, list) > 0)
				for(uint i = 0; i < list.getCount(); i++)
					if((id = list.at(i)) != 0 && id != p->Rec.ID)
						for(uint j = 0; j < p->Kinds.getCount(); j++)
							if(P_Tbl->IsBelongToKind(id, p->Kinds.at(j))) {
								ASSIGN_PTR(pID, id);
								return 1;
							}
		}
	}
	if(!(flags & smlRegisterOnly)) {
		PersonTbl::Rec rec;
		if(SearchFirstByName(p->Rec.Name, &p->Kinds, p->Rec.ID, &rec) > 0) {
			ASSIGN_PTR(pID, rec.ID);
			return 1;
		}
	}
	return -1;
}

int PPObjPerson::GetPersonReq(PPID id, PersonReq * pPersonReq)
{
	PPPersonPacket pack;
	PPBankAccount ba_rec;
	if(id && GetPacket(id, &pack, PGETPCKF_USEINHERITENCE) > 0) {
		SString temp_buf;
		STRNSCPY(pPersonReq->Name, pack.Rec.Name);
		if(pack.GetExtName(temp_buf) > 0)
			temp_buf.CopyTo(pPersonReq->ExtName, sizeof(pPersonReq->ExtName));
		else
			STRNSCPY(pPersonReq->ExtName, pack.Rec.Name);
		// @v11.1.12 STRNSCPY(pPersonReq->Memo, pack.Rec.Memo);
		STRNSCPY(pPersonReq->Memo, pack.SMemo); // @v11.1.12
		pPersonReq->AddrID = pack.Rec.MainLoc;
		pPersonReq->RAddrID = pack.Rec.RLoc;
		pack.GetAddress(0, temp_buf);  temp_buf.CopyTo(pPersonReq->Addr, sizeof(pPersonReq->Addr));
		pack.GetRAddress(0, temp_buf); temp_buf.CopyTo(pPersonReq->RAddr, sizeof(pPersonReq->RAddr));
		pack.GetPhones(3, temp_buf);   temp_buf.CopyTo(pPersonReq->Phone1, sizeof(pPersonReq->Phone1));
		pack.GetRegNumber(PPREGT_TPID,  temp_buf); temp_buf.CopyTo(pPersonReq->TPID,  sizeof(pPersonReq->TPID));
		pack.GetRegNumber(PPREGT_KPP,   temp_buf); temp_buf.CopyTo(pPersonReq->KPP,   sizeof(pPersonReq->KPP));
		pack.GetRegNumber(PPREGT_OKPO,  temp_buf); temp_buf.CopyTo(pPersonReq->OKPO,  sizeof(pPersonReq->OKPO));
		pack.GetRegNumber(PPREGT_OKONH, temp_buf); temp_buf.CopyTo(pPersonReq->OKONF, sizeof(pPersonReq->OKONF));
		pack.GetSrchRegNumber(&pPersonReq->SrchRegTypeID, temp_buf);
		temp_buf.CopyTo(pPersonReq->SrchCode, sizeof(pPersonReq->SrchCode));
		if(pack.GetCurrBnkAcct(&ba_rec) > 0) {
			pPersonReq->BnkAcct.InitFlags = BADIF_INITALLBR;
			GetBnkAcctData(ba_rec.ID, &ba_rec, &pPersonReq->BnkAcct);
		}
		pPersonReq->Flags = pack.Rec.Flags;
	}
	return 1;
}

int PPObjPerson::FormatRegister(PPID id, PPID regTypeID, char * buf, size_t buflen)
{
	RegisterArray regs;
	if(GetRegList(id, &regs, 0/*useInheritence*/)) {
		for(uint i = 0; i < regs.getCount(); i++) {
			if(regs.at(i).RegTypeID == regTypeID)
				return RegObj.Format(regs.at(i).ID, 0, buf, buflen);
		}
	}
	ASSIGN_PTR(buf, 0);
	return -1;
}

PPObjPerson::ResolverParam::ResolverParam() : Flags(0), KindID(0)
{
	AttrPriorityList.addzlist(attrUUID, attrGLN, attrINN, attrPhone, attrEMail, attrCommonName, 0);
}

struct ResolvePersonListEntry {
	int    Attr;
	PPIDArray CandidateList;
};

class ResolvePersonList : public TSCollection <ResolvePersonListEntry> {
public:
	ResolvePersonList()
	{
	}
	int    AddEntry(int attr, PPIDArray & rList)
	{
		int    ok = 1;
		assert(attr);
		if(rList.getCount()) {
			rList.sortAndUndup();
			ResolvePersonListEntry * p_new_entry = CreateNewItem();
			if(p_new_entry) {
				p_new_entry->Attr = attr;
				p_new_entry->CandidateList = rList;
			}
			else
				ok = 0;
		}
		else
			ok = -1;
		return ok;
	}
	uint   GetUniteList(LongArray & rResult) const
	{
		rResult.Z();
		for(uint i = 0; i < getCount(); i++) {
			ResolvePersonListEntry * p_entry = at(i);
			if(p_entry)
				rResult.add(&p_entry->CandidateList);
		}
		rResult.sortAndUndup();
		return rResult.getCount();
	}
	int    ArrangeResultList(LongArray & rResult) const
	{
		int    ok = -1;
		rResult.sortAndUndup(); // Параноидальная страховка
		const  uint _org_count = rResult.getCount();
		if(rResult.getCount() > 1) {
			RAssocArray temp_list;
			const uint _c = getCount();
			for(uint i = 0; i < rResult.getCount(); i++) {
				const PPID id = rResult.get(i);
				for(uint j = 0; j < _c; j++) {
					ResolvePersonListEntry * p_entry = at(j);
					if(p_entry && p_entry->CandidateList.lsearch(id)) {
						double tlv = 0.0;
						uint   tlp = 0;
						if(temp_list.Search(id, &tlv, &tlp)) {
							assert(tlv >= 1);
							tlv += 1.0 + (0.01 * (_c - j)); // Позиция в списке выступает в качестве весового коэффициента:
								// чем ближе к началу - тем важнее (в соответствии с приоритетом ResolverParam::AttrPriorityList)
							temp_list.at(tlp).Val = tlv;
						}
						else {
							RAssoc new_item(id, 1.0 + (0.01 * (_c - j)));
							temp_list.insert(&new_item);
						}
					}
				}
			}
			temp_list.SortByValRev();
			{
				rResult.Z();
				for(uint j = 0; j < temp_list.getCount(); j++) {
					rResult.add(temp_list.at(j).Key);
				}
				assert(rResult.getCount() == _org_count);
			}
			ok = 1;
		}
		return ok;
	}
};

int PPObjPerson::Resolve(const ResolverParam & rP, PPIDArray & rCandidateIdList, int use_ta)
{
	rCandidateIdList.Z();
	int    ok = -1;
	Reference * p_ref = PPRef;
	ResolvePersonList resolve_list;
	PPIDArray temp_list;
	PPIDArray temp_loc_list;
	SString temp_buf;
	PersonTbl::Rec psn_rec;
	LocationTbl::Rec loc_rec;
	SCardTbl::Rec sc_rec;
	PPObjSCard * p_sc_obj = 0;
	for(uint i = 0; i < rP.AttrPriorityList.getCount(); i++) {
		const int attr = rP.AttrPriorityList.get(i);
		temp_list.Z();
		temp_loc_list.Z();
		switch(attr) {
			case ResolverParam::attrUUID:
				if(!!rP.Uuid && p_ref->Ot.SearchObjectsByGuid(PPOBJ_PERSON, PPTAG_PERSON_UUID, rP.Uuid, &temp_list) > 0) {
					assert(temp_list.getCount());
					THROW(resolve_list.AddEntry(attr, temp_list));
				}
				break;
			case ResolverParam::attrGLN:
				if(rP.GLN.NotEmpty() && GetListByRegNumber(PPREGT_GLN, 0, rP.GLN, temp_list) > 0) {
					assert(temp_list.getCount());
					THROW(resolve_list.AddEntry(attr, temp_list));
				}
				break;
			case ResolverParam::attrINN:
				if(rP.INN.NotEmpty() && GetListByRegNumber(PPREGT_TPID, 0, rP.INN, temp_list) > 0) {
					assert(temp_list.getCount());
					if(rP.KPP.NotEmpty() && !(rP.Flags & rP.fIgnoreKppWhenSearching)) {
						uint j = temp_list.getCount();
						if(j) do {
							PPID   psn_id = temp_list.get(--j);
							RegisterTbl::Rec reg_rec;
							if(GetRegister(psn_id, PPREGT_KPP, getcurdate_(), &reg_rec) <= 0 || rP.KPP != reg_rec.Num) {
								temp_list.atFree(j);
							}
						} while(j);
					}
					THROW(resolve_list.AddEntry(attr, temp_list));
				}
				break;
			case ResolverParam::attrPhone:
				if(rP.Phone.NotEmpty()) {
					PPEAddr::Phone::NormalizeStr(rP.Phone, 0, temp_buf);
					PPObjIDArray obj_list;
					if(LocObj.P_Tbl->SearchPhoneObjList(temp_buf, 0, obj_list) > 0) {
						for(uint j = 0; j < obj_list.getCount(); j++) {
							const PPID item_obj_id = obj_list.at(j).Id;
							switch(obj_list.at(j).Obj) {
								case PPOBJ_PERSON:
									temp_list.addnz(item_obj_id);
									break;
								case PPOBJ_LOCATION:
									if(LocObj.Search(item_obj_id, &loc_rec) > 0 && loc_rec.OwnerID && Search(loc_rec.OwnerID, &psn_rec) > 0)
										temp_list.add(psn_rec.ID);
									break;
								case PPOBJ_SCARD:
									SETIFZ(p_sc_obj, new PPObjSCard);
									if(p_sc_obj && p_sc_obj->Search(item_obj_id, &sc_rec) > 0 && sc_rec.PersonID && Search(sc_rec.PersonID, &psn_rec) > 0)
										temp_list.add(psn_rec.ID);
									break;
							}
						}
						THROW(resolve_list.AddEntry(attr, temp_list));
					}
				}
				break;
			case ResolverParam::attrEMail:
				if(rP.EMail.NotEmpty()) {
					if(SearchEmail(rP.EMail, 0, &temp_list, &temp_loc_list) > 0) {
						for(uint j = 0; j < temp_loc_list.getCount(); j++) {
							PPID loc_id = temp_loc_list.get(j);
							if(LocObj.Search(loc_id, &loc_rec) > 0 && loc_rec.OwnerID && Search(loc_rec.OwnerID, &psn_rec) > 0)
								temp_list.add(psn_rec.ID);
						}
						THROW(resolve_list.AddEntry(attr, temp_list));
					}
				}
				break;
			case ResolverParam::attrCommonName:
				if(rP.CommonName.NotEmpty()) {
					if(P_Tbl->SearchByName(rP.CommonName, temp_list) > 0) {
						assert(temp_list.getCount());
						THROW(resolve_list.AddEntry(attr, temp_list));
					}
				}
				break;
		}
	}
	{
		LongArray ulist;
		resolve_list.GetUniteList(ulist);
		if(ulist.getCount()) {
			resolve_list.ArrangeResultList(ulist);
			// Первый элемент списка ulist - самый значимый
			rCandidateIdList.add(&ulist);
			ok = 1;
		}
		else {
			if(rP.Flags & rP.fCreateIfNFound) {
				if(rP.CommonName.NotEmpty()) {
					PPObjPersonKind pk_obj;
					PPPersonPacket pack;
					STRNSCPY(pack.Rec.Name, rP.CommonName);
					if(rP.KindID && pk_obj.Fetch(rP.KindID, 0) > 0)
						pack.Kinds.add(rP.KindID);
					else
						pack.Kinds.add(PPPRK_UNKNOWN);
					if(!!rP.Uuid) {
						ObjTagItem tag_item;
						if(tag_item.SetGuid(PPTAG_PERSON_UUID, &rP.Uuid))
							pack.TagL.PutItem(PPTAG_PERSON_UUID, &tag_item);
					}
					if(rP.INN.NotEmpty()) {
						pack.AddRegister(PPREGT_TPID, rP.INN);
					}
					if(rP.KPP.NotEmpty()) {
						pack.AddRegister(PPREGT_KPP, rP.KPP);
					}
					if(rP.GLN.NotEmpty()) {
						pack.AddRegister(PPREGT_GLN, rP.GLN);
					}
					if(rP.Phone.NotEmpty()) {
						pack.ELA.AddItem(PPELK_MOBILE, rP.Phone);
					}
					if(rP.EMail.NotEmpty()) {
						pack.ELA.AddItem(PPELK_EMAIL, rP.EMail);
					}
					{
						PPID   new_id = 0;
						THROW(PutPacket(&new_id, &pack, use_ta));
						rCandidateIdList.add(new_id);
						ok = 2;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_sc_obj;
	return ok;
}

int PPObjPerson::GetTradeLicList(PPID id, RegisterArray * pList)
{
	RegisterArray temp;
	CALLPTRMEMB(pList, freeAll());
	const  PPID trade_lic_reg_type = GetConfig().TradeLicRegTypeID;
	if(trade_lic_reg_type && GetRegList(id, &temp, 0/*useInheritence*/)) {
		if(pList) {
			for(uint i = 0; i < temp.getCount(); i++)
				if(temp.at(i).RegTypeID == trade_lic_reg_type)
					pList->insert(&temp.at(i));
		}
		return 1;
	}
	return -1;
}

int PPObjPerson::GetActualTradeLic(PPID id, LDATE dt, RegisterTbl::Rec * rec)
{
	RegisterArray list;
	if(GetTradeLicList(id, &list) > 0) {
		if(list.getCount()) {
			RegisterTbl::Rec * item = 0;
			if(dt)
				for(uint i = 0; i < list.getCount(); i++) {
					item = & list.at(i);
					if(item->Dt <= dt && (!item->Expiry || dt <= item->Expiry)) {
						if(rec)
							memcpy(rec, item, sizeof(RegisterTbl::Rec));
						return 1;
					}
				}
			else {
				if(rec)
					memcpy(rec, &list.at(0), sizeof(RegisterTbl::Rec));
				return 1;
			}
		}
	}
	memzero(rec, sizeof(*rec));
	return -1;
}

int PPObjPerson::AddToAddrBook(PPID personID, PPID userID, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   usr_id = (userID >= 0) ? userID : LConfig.UserID;
	if(p_ref->Assc.Search(PPASS_ADDRESSBOOK, usr_id, personID) > 0)
		ok = -1;
	else {
		PPID   id = 0;
		ObjAssocTbl::Rec rec;
		long   free_num = 0;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			// @v10.6.4 MEMSZERO(rec);
			rec.AsscType = PPASS_ADDRESSBOOK;
			rec.PrmrObjID = usr_id;
			rec.ScndObjID = personID;
			THROW(p_ref->Assc.SearchFreeNum(PPASS_ADDRESSBOOK, usr_id, &free_num, 0));
			rec.InnerNum = free_num;
			THROW(p_ref->Assc.Add(&id, &rec, 0));
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::RemoveFromAddrBook(PPID personID, PPID userID, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   usr_id = (userID >= 0) ? userID : LConfig.UserID;
	ObjAssocTbl::Rec rec;
	if(p_ref->Assc.Search(PPASS_ADDRESSBOOK, usr_id, personID, &rec) > 0) {
		if(!p_ref->Assc.Remove(rec.ID, use_ta))
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int PPObjPerson::EnumAddrBookItems(PPID userID, PPID * pPersonID, ObjAssocTbl::Rec * b)
{
	PPID   usr_id = (userID >= 0) ? userID : LConfig.UserID;
	return PPRef->Assc.EnumByPrmr(PPASS_ADDRESSBOOK, usr_id, pPersonID, b);
}

int PPObjPerson::GetAddrBookIDList(PPID userID, PPIDArray * pList)
{
	PPID   usr_id = (userID >= 0) ? userID : LConfig.UserID;
	return PPRef->Assc.GetListByPrmr(PPASS_ADDRESSBOOK, usr_id, pList);
}

int PPObjPerson::AddRegisterToPacket(PPPersonPacket & rPack, PPID regTypeID, const char * pNumber, int checkUnique /* = 1 */)
{
	int    ok = -1;
	char   temp_buf[128];
	RegisterTbl::Rec reg_rec;
	STRNSCPY(temp_buf, pNumber);
	if(*strip(temp_buf)) {
		int    reg_exists = 0;
		MEMSZERO(reg_rec);
		reg_rec.RegTypeID = regTypeID;
		STRNSCPY(reg_rec.Num, temp_buf);
		PPObjRegisterType obj_regt;
		PPRegisterType regt_rec;
		THROW(obj_regt.Fetch(reg_rec.RegTypeID, &regt_rec) > 0);
		if(regt_rec.Flags & REGTF_UNIQUE) {
			RegisterTbl::Rec test_rec;
			uint   pos = 0;
			while(rPack.Regs.GetRegister(reg_rec.RegTypeID, &pos, &test_rec) > 0) {
				if(sstreq(test_rec.Num, reg_rec.Num))
					reg_exists = 1;
				else
					rPack.Regs.atFree(--pos);
			}
		}
		if(!reg_exists) {
			THROW(!checkUnique || RegObj.CheckUniqueNumber(&reg_rec, &rPack.Regs, 0, 0));
			THROW_SL(rPack.Regs.insert(&reg_rec));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::AddSimple(PPID * pID, const char * pName, PPID kindID, PPID statusID, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	if(!(P_Tbl->SearchByName(pName, &id, 0) > 0 && P_Tbl->IsBelongToKind(id, kindID))) {
		PPPersonPacket pack;
		pack.Rec.Status = statusID;
		STRNSCPY(pack.Rec.Name, pName);
		pack.Kinds.add(kindID);
		THROW(PutPacket(&(id = 0), &pack, use_ta));
	}
	CATCH
		ok = 0;
		id = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

/*virtual*/int PPObjPerson::MakeReserved(long flags)
{
    int    ok = -1;
    if(flags & mrfInitializeDb) {
		long    _count = 0;
		{
			PPIDArray main_org_list;
			GetListByKind(PPPRK_MAIN, &main_org_list, 0);
			_count = main_org_list.getCount();
		}
        if(_count == 0) {
			PPID   id = 0;
			SString temp_buf;
            PPPersonPacket pack;
			PPLoadString("mainorg", temp_buf);
			STRNSCPY(pack.Rec.Name, temp_buf);
			pack.Rec.Status = PPPRS_LEGAL;
            pack.Kinds.add(PPPRK_MAIN);
            THROW(PutPacket(&id, &pack, 1));
			ok = 1;
        }
    }
    CATCHZOK
    return ok;
}

int PPObjPerson::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE)
		switch(_obj) {
			case PPOBJ_PERSONKIND:
				{
					PersonKindTbl::Key0 k;
					MEMSZERO(k);
					k.KindID = _id;
					if(P_Tbl->Kind.search(0, &k, spGe) && k.KindID == _id)
						ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
					else if(!BTROKORNFOUND)
						ok = PPSetErrorDB();
				}
				break;
			case PPOBJ_PRSNSTATUS:
				{
					BExtQuery q(P_Tbl, 0, 1);
					q.select(P_Tbl->ID, 0L).where(P_Tbl->Status == _id);
					if(q.fetchFirst() > 0)
						ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
				}
				break;
			case PPOBJ_PRSNCATEGORY:
				{
					BExtQuery q(P_Tbl, 0, 1);
					q.select(P_Tbl->ID, 0L).where(P_Tbl->CatID == _id);
					if(q.fetchFirst() > 0)
						ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
				}
				break;
			case PPOBJ_REGISTERTYPE:
				{
					RegisterFilt reg_flt;
					reg_flt.Oid.Obj = PPOBJ_PERSON; // @v10.0.1
					reg_flt.RegTypeID = _id;
					PPIDArray psn_list;
					if(RegObj.SearchByFilt(&reg_flt, 0, &psn_list) > 0 && psn_list.getCount())
						return RetRefsExistsErr(Obj, psn_list.get(0));
				}
				break;
			case PPOBJ_ELINKKIND:
				ok = ReplyPersonELinkDel(_id);
				break;
			case PPOBJ_TAG:
				ok = ReplyPersonTagDel(_id);
				break;
		}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_PERSON) {
			PPIDArray kind_list;
			THROW(P_Tbl->GetKindList(_id, &kind_list));
			for(uint i = 0; i < kind_list.getCount(); i++)
				THROW(P_Tbl->AddKind(reinterpret_cast<long>(extraPtr), kind_list.get(i), 0));
			THROW(BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, _id, extraPtr));
		}
		else if(_obj == PPOBJ_LOCATION) {
			ok = ReplyLocationReplace(_id, reinterpret_cast<long>(extraPtr));
		}
		else if(_obj == PPOBJ_PRSNCATEGORY) {
			THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->CatID == _id), set(P_Tbl->CatID, dbconst(reinterpret_cast<long>(extraPtr)))));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::ReplyPersonELinkDel(PPID eLinkID)
{
	int    ok = DBRPL_OK;
	PersonTbl::Key0 k0;
	k0.ID = 0;
	SString ela_buf;
	for(P_Tbl->search(0, &k0, spFirst); P_Tbl->search(0, &k0, spNext);) {
		PPELinkArray ela_list;
		const PPID psn_id = P_Tbl->data.ID;
		THROW(P_Tbl->GetELinks(psn_id, ela_list));
		if(ela_list.GetItem(eLinkID, ela_buf) > 0) {
			ok = RetRefsExistsErr(Obj, psn_id);
			break;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::ReplyPersonTagDel(PPID tagID)
{
	PPObjID objid;
	int    r = PPRef->Ot.SearchAnyRefToTagID(tagID, &objid);
	if(r > 0)
		return RetRefsExistsErr(objid.Obj, objid.Id);
	else
		return r ? DBRPL_OK : DBRPL_ERROR;
}

int PPObjPerson::ReplyLocationReplace(PPID dest, PPID src)
{
	int    ok = DBRPL_OK;
	PPIDArray psn_list;
	PersonTbl::Key0 k0;
	PPIDArray dlvr_loc_list;
	BExtQuery q(P_Tbl, 0);
	q.select(P_Tbl->ID, P_Tbl->MainLoc, P_Tbl->RLoc, 0);
	MEMSZERO(k0);
	for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
		PPID   psn_id = P_Tbl->data.ID;
		if(P_Tbl->data.MainLoc == dest) {
			//
			// Нельзя заменять юридический адрес персоналии
			//
			PersonTbl::Rec psn_rec;
			if(Search(psn_id, &psn_rec) <= 0)
				psn_rec.Name[0] = 0;
			CALLEXCEPT_PP_S(PPERR_CANTREPLACEMAINADDR, psn_rec.Name);
		}
		if(P_Tbl->data.RLoc == dest) {
			//
			// Нельзя заменять фактический адрес персоналии
			//
			PersonTbl::Rec psn_rec;
			if(Search(psn_id, &psn_rec) <= 0)
				psn_rec.Name[0] = 0;
			CALLEXCEPT_PP_S(PPERR_CANTREPLACERADDR, psn_rec.Name);
		}
		dlvr_loc_list.clear();
		if(GetDlvrLocList(psn_id, &dlvr_loc_list) > 0) {
			for(uint i = 0; i < dlvr_loc_list.getCount(); i++) {
				if(dlvr_loc_list.get(i) == dest)
					psn_list.addUnique(psn_id);
			}
		}
	}
	for(uint i = 0; i < psn_list.getCount(); i++) {
		PPID   psn_id = psn_list.get(i);
		PPPersonPacket pack;
		if(GetPacket(psn_id, &pack, 0) > 0) {
			int    r;
			THROW(r = pack.ReplaceDlvrLoc(dest, src));
			if(r > 0) {
				THROW(PutPacket(&psn_id, &pack, 0));
			}
		}
	}
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjPerson, PPPersonPacket);

int PPObjPerson::SerializePacket(int dir, PPPersonPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint   i;
	SString ext_name;
	PPIDArray addr_id_list;
	LAssocArray rel_list;
	SerializeSignature srzs(Obj, dir, rBuf); // @v11.1.12
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	// @v11.1.12 {
	if(srzs.V.IsGe(11, 1, 12)) {
		THROW_SL(pSCtx->Serialize(dir, pPack->SMemo, rBuf));
	}
	// } @v11.1.12
	THROW_SL(pSCtx->Serialize(dir, &pPack->Kinds, rBuf));
	if(dir > 0) {
		pPack->GetExtName(ext_name);
		PPLocationPacket addr;
		for(i = 0; pPack->EnumDlvrLoc(&i, &addr);)
			addr_id_list.add(addr.ID);
		rel_list = pPack->GetRelList();
	}
	THROW_SL(pSCtx->Serialize(dir, ext_name, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &addr_id_list, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &rel_list, rBuf));
	THROW(pPack->ELA.Serialize(dir, rBuf, pSCtx));
	THROW_SL(RegObj.P_Tbl->SerializeArrayOfRecords(dir, &pPack->Regs, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, &pPack->Amounts, rBuf));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	THROW(pPack->LinkFiles.Serialize(dir, (GetConfig().Flags & PPPersonConfig::fSendAttachment) ? 0 : 1, rBuf, pSCtx));
	if(dir < 0) {
		pPack->SetExtName(ext_name);
		for(i = 0; i < addr_id_list.getCount(); i++) {
			PPLocationPacket addr;
			addr.ID = addr_id_list.at(i);
			THROW(pPack->AddDlvrLoc(addr));
		}
		for(i = 0; i < rel_list.getCount(); i++)
			pPack->AddRelation(rel_list.at(i).Key, rel_list.at(i).Val, 0);
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPPersonPacket);
	{
		PPPersonPacket * p_pack = static_cast<PPPersonPacket *>(p->Data);
		if(stream == 0) {
			THROW(GetPacket(id, p_pack, 0) > 0);
			if(GetConfig().Flags & PPPersonConfig::fSendAttachment) {
				p_pack->LinkFiles.Init(Obj);
				p_pack->LinkFiles.Load(p_pack->Rec.ID, 0L);
			}
		}
		else {
			SBuffer buffer;
			THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0))
			THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
			{
				StaffAmtEntry * p_amt_entry;
				SInvariantParam ip;
				for(uint i = 0; p_pack->Amounts.enumItems(&i, (void **)&p_amt_entry);)
					if(!p_amt_entry->InvariantC(&ip)) {
						PPSetError(PPERR_INVARIANT_STAFFAMTENT);
						pCtx->OutputLastError();
						p_pack->Amounts.atFree(i-1);
					}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::PreventDupRegister(PPID id, PPPersonPacket * pPack)
{
	const uint rc = pPack->Regs.getCount();
	if(rc)
		for(uint i = 0; i < rc; i++)
			RegObj.PreventDup(pPack->Regs.at(i), Obj, id);
	return 1;
}

int PPObjPerson::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		SString msg_buf, fmt_buf;
		PPPersonPacket * p_pack = static_cast<PPPersonPacket *>(p->Data);
		PPLocationPacket addr;
		if(stream == 0) {
			int    is_analog = 0;
			const  PPPersonConfig & r_cfg = GetConfig();
			if(p_pack->Rec.MainLoc && p_pack->Loc.IsEmptyAddress())
				LocObj.GetPacket(p_pack->Rec.MainLoc, &p_pack->Loc);
			if(p_pack->Rec.RLoc && p_pack->RLoc.IsEmptyAddress())
				LocObj.GetPacket(p_pack->Rec.RLoc, &p_pack->RLoc);
			for(uint i = 0; p_pack->EnumDlvrLoc(&i, &addr);) {
				if(LocObj.GetPacket(addr.ID, &addr) > 0)
					p_pack->PutDlvrLoc(i-1, &addr);
				else {
					p_pack->PutDlvrLoc(i-1, 0);
					i--;
				}
			}
			if((*pID) == 0) {
				PPID   same_id = 0;
				p_pack->Rec.ID = 0;
				if(r_cfg.Flags & PPPersonConfig::fSyncByName && SearchMaxLike(p_pack, &same_id, 0, 0) > 0) {
					*pID = same_id;
					is_analog = 1;
					pCtx->OutputString(PPTXT_ACCEPTANALOG_PSN_NAME, p_pack->Rec.Name);
				}
				else if(r_cfg.Flags & PPPersonConfig::fSyncBySrchReg) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					for(uint i = 0; !is_analog && i < p_pack->Kinds.getCount(); i++)
						if(pk_obj.Fetch(p_pack->Kinds.get(i), &pk_rec) > 0 && pk_rec.CodeRegTypeID)
							if(SearchMaxLike(p_pack, &same_id, smlRegisterOnly, pk_rec.CodeRegTypeID) > 0) {
								*pID = same_id;
								is_analog = 1;
								(msg_buf = p_pack->Rec.Name).CatDiv('-', 1).Cat(pk_rec.Name);
								pCtx->OutputString(PPTXT_ACCEPTANALOG_PSN_CODE, msg_buf);
							}
				}
			}
			if(*pID) {
				if(!(r_cfg.Flags & PPPersonConfig::fSyncDeclineUpdate)) {
					PPIDArray _absent_kind_list; // @v10.3.0
					PPIDArray * p_absent_kind_list = (r_cfg.Flags & PPPersonConfig::fSyncAppendAbsKinds) ? &_absent_kind_list : 0; // @v10.3.0
					p_pack->Rec.ID = *pID;
					if(p_pack->LinkFiles.GetState() & ObjLinkFiles::stTransmissionNotSupported)
						p_pack->UpdFlags |= PPPersonPacket::ufDontChgImgFlag;
					p_pack->UpdFlags |= PPPersonPacket::ufDontRmvDlvrLoc;
					THROW(PreventDupRegister(*pID, p_pack));
					if(r_cfg.Flags & PPPersonConfig::fSyncMergeRegList) {
						RegisterArray org_reg_list;
						GetRegList(*pID, &org_reg_list, 0);
						org_reg_list.Merge(p_pack->Regs);
						p_pack->Regs = org_reg_list;
					}
					if(!P_ArObj->CheckPersonPacket(p_pack, p_absent_kind_list)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPERSON, p_pack->Rec.ID, p_pack->Rec.Name);
						THROW(*pID);
						ok = -1;
					}
					else {
						// @v10.3.0 {
						if(p_absent_kind_list && p_absent_kind_list->getCount()) {
							p_absent_kind_list->sortAndUndup();
							p_pack->Kinds.addUnique(p_absent_kind_list);
						}
						// } @v10.3.0
						ObjTagList org_tag_list;
						PPRef->Ot.GetList(Obj, *pID, &org_tag_list);
						org_tag_list.Merge(p_pack->TagL, ObjTagList::mumAdd|ObjTagList::mumUpdate);
						p_pack->TagL = org_tag_list;
						int    r = PutPacket(pID, p_pack, 1);
						if(!r) {
							pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPERSON, p_pack->Rec.ID, p_pack->Rec.Name);
							THROW(*pID);
							ok = -1;
						}
						else if(r > 0)
							ok = 102; // @ObjectUpdated
					}
				}
			}
			else {
				p_pack->Rec.ID = *pID = 0;
				THROW(PreventDupRegister(*pID, p_pack));
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTPERSON, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				else
					ok = 101; // @ObjectCreated
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		uint   i;
		PPLocationPacket addr;
		PPPersonPacket * p_pack = static_cast<PPPersonPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PRSNSTATUS, &p_pack->Rec.Status, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION,   &p_pack->Rec.MainLoc, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION,   &p_pack->Rec.RLoc, ary, replace))
		THROW(ProcessObjRefInArray(PPOBJ_PRSNCATEGORY, &p_pack->Rec.CatID, ary, replace));
		for(i = 0; p_pack->EnumDlvrLoc(&i, &addr) > 0;) {
			THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &addr.ID, ary, replace));
			if(replace)
				p_pack->PutDlvrLoc(i-1, &addr);
		}
		THROW(ProcessObjListRefInArray(PPOBJ_PERSONKIND, p_pack->Kinds, ary, replace));
		THROW(p_pack->Regs.ProcessObjRefs(ary, replace));
		for(i = 0; i < p_pack->ELA.getCount(); i++) {
			THROW(ProcessObjRefInArray(PPOBJ_ELINKKIND, &p_pack->ELA.at(i).KindID, ary, replace));
		}
		for(i = 0; i < p_pack->GetRelList().getCount(); i++) {
			// @privacy_violation Здесь обходится приватность PPPerson::RelList
			LAssoc & r_assoc = p_pack->GetRelList().at(i);
			THROW(ProcessObjRefInArray(PPOBJ_PERSON, &r_assoc.Key, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_PERSONRELTYPE, &r_assoc.Val, ary, replace));
		}
		for(i = 0; i < p_pack->Amounts.getCount(); i++) {
			StaffAmtEntry & r_entry = p_pack->Amounts.at(i);
			THROW(ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, &r_entry.AmtTypeID, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_CURRENCY,   &r_entry.CurID,     ary, replace));
		}
		THROW(p_pack->TagL.ProcessObjRefs(ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
// Объединение персоналий
//
struct ReplacePersonParam {
	ReplacePersonParam() : Flags(fRemoveDest), SrcKindID(0), SrcID(0), DestKindID(0)
	{
	}
	enum {
		fAddress     = 0x0001,
		fRemoveDest  = 0x0002, // Удалять DestIdList-объекты
		fDontConfirm = 0x0004  // Не запрашивать подтверждение на удаление DestIdList-объектов
	};
	long   Flags;
	PPID   SrcKindID;
	PPID   SrcID;
	PPID   DestKindID;
	PPIDArray DestIdList;
};

class ReplPrsnDialog : public TDialog {
	DECL_DIALOG_DATA(ReplacePersonParam);
	enum {
		ctlgroupPsnList = 1
	};
public:
	explicit ReplPrsnDialog(int addr) : TDialog(addr ? DLG_REPLADDR : DLG_REPLPRSN)
	{
		SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND1, PPOBJ_PERSONKIND, 0, 0, 0);
		SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND2, PPOBJ_PERSONKIND, 0, 0, 0);
		replyKindSelected(0);
		replyKindSelected(1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PPID   psn_id = 0;
		PPID   kind_id = Data.SrcKindID;
		PPID   addr_id = 0;
		if(Data.Flags & Data.fAddress) {
			addr_id = Data.SrcID;
			if(addr_id) {
				LocationTbl::Rec loc_rec;
				PPObjLocation loc_obj;
				if(loc_obj.Search(addr_id, &loc_rec) > 0 && loc_rec.OwnerID)
					psn_id = loc_rec.OwnerID;
			}
		}
		else {
			psn_id = Data.SrcID;
		}
		if(psn_id && !kind_id) {
			PPPerson    psn;
			if(PsnObj.P_Tbl->Get(psn_id, &psn) > 0 && psn.Kinds.getCount() > 0)
				kind_id = psn.Kinds.at(0);
		}
		SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND1, PPOBJ_PERSONKIND, kind_id, 0);
		SetupPPObjCombo(this, CTLSEL_REPLPRSN_PRSN1, PPOBJ_PERSON, psn_id, 0, reinterpret_cast<void *>(kind_id));
		if(Data.Flags & Data.fAddress) {
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_REPLPRSN_ADDR1, psn_id, addr_id);
			SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND2, PPOBJ_PERSONKIND, kind_id, 0);
			SetupPPObjCombo(this, CTLSEL_REPLPRSN_PRSN2, PPOBJ_PERSON, psn_id, 0, reinterpret_cast<void *>(kind_id));
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_REPLPRSN_ADDR2, psn_id, 0);
		}
		else {
			AddClusterAssoc(CTL_REPLPRSN_FLAGS, 0, Data.fRemoveDest);
			AddClusterAssoc(CTL_REPLPRSN_FLAGS, 1, Data.fDontConfirm);
			SetClusterData(CTL_REPLPRSN_FLAGS, Data.Flags);
		}
		SetupGroup();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		Data.SrcKindID = getCtrlLong(CTL_REPLPRSN_KIND1);
		Data.SrcID  = getCtrlLong((Data.Flags & Data.fAddress) ? CTLSEL_REPLPRSN_ADDR1 : CTLSEL_REPLPRSN_PRSN1);
		Data.DestKindID = getCtrlLong(CTL_REPLPRSN_KIND2);
		if(Data.Flags & Data.fAddress) {
			PPID   dest_id = getCtrlLong((Data.Flags & Data.fAddress) ? CTLSEL_REPLPRSN_ADDR2 : CTLSEL_REPLPRSN_PRSN2);
			Data.DestIdList.Z().addnz(dest_id);
		}
		else {
			PersonListCtrlGroup::Rec grp_rec;
			getGroupData(ctlgroupPsnList, &grp_rec);
			Data.DestIdList = grp_rec.List;
			GetClusterData(CTL_REPLPRSN_FLAGS, &Data.Flags);
		}
		THROW_PP(Data.SrcID != 0, PPERR_REPLZEROOBJ);
		for(uint i = 0; i < Data.DestIdList.getCount(); i++) {
			const PPID dest_id = Data.DestIdList.get(i);
			THROW_PP(dest_id != 0, PPERR_REPLZEROOBJ);
			THROW_PP(dest_id != Data.SrcID, PPERR_REPLSAMEOBJ);
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			selectCtrl(Data.SrcID ? ((Data.Flags & Data.fAddress) ? CTL_REPLPRSN_ADDR2 : CTL_REPLPRSN_PRSN2) : ((Data.Flags & Data.fAddress) ? CTL_REPLPRSN_ADDR1 : CTL_REPLPRSN_PRSN1));
			ok = 0;
		ENDCATCH
		return ok;
	}
	void   replyKindSelected(int i)
	{
		if(Data.Flags & Data.fAddress)
			PsnObj.SetupDlvrLocCombo(this, (i ? CTLSEL_REPLPRSN_ADDR2 : CTLSEL_REPLPRSN_ADDR1), 0, 0);
		else {
			if(i == 0) {
				Data.SrcKindID = getCtrlLong(CTLSEL_REPLPRSN_KIND1);
				SetupPPObjCombo(this, CTLSEL_REPLPRSN_PRSN1, PPOBJ_PERSON, 0, 0, reinterpret_cast<void *>(Data.SrcKindID));
			}
			else {
				Data.DestKindID = getCtrlLong(CTLSEL_REPLPRSN_KIND2);
				SetupGroup();
			}
		}
	}
	void   replyPersonSelected(int i)
	{
		if(Data.Flags & Data.fAddress) {
			PPID psn_id = getCtrlLong(i ? CTLSEL_REPLPRSN_PRSN2 : CTLSEL_REPLPRSN_PRSN1);
			PsnObj.SetupDlvrLocCombo(this, (i ? CTLSEL_REPLPRSN_ADDR2 : CTLSEL_REPLPRSN_ADDR1), psn_id, 0);
		}
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupGroup()
	{
		if(!(Data.Flags & Data.fAddress)) {
			PersonListCtrlGroup * p_grp = static_cast<PersonListCtrlGroup *>(getGroup(ctlgroupPsnList));
			if(!p_grp)
				addGroup(ctlgroupPsnList, new PersonListCtrlGroup(CTLSEL_REPLPRSN_PRSN2, CTLSEL_REPLPRSN_KIND2, cmPersonList, OLW_CANINSERT));
			PersonListCtrlGroup::Rec grp_rec(Data.DestKindID, &Data.DestIdList);
			setGroupData(ctlgroupPsnList, &grp_rec);
		}
	}
	void   Swap()
	{
		PPID   src_kind_id  = getCtrlLong(CTLSEL_REPLPRSN_KIND1);
		PPID   src_psn_id   = getCtrlLong(CTLSEL_REPLPRSN_PRSN1);
		if(Data.Flags & Data.fAddress) {
			PPID   dest_kind_id = getCtrlLong(CTLSEL_REPLPRSN_KIND2);
			PPID   dest_psn_id  = getCtrlLong(CTLSEL_REPLPRSN_PRSN2);
			SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND1, PPOBJ_PERSONKIND, dest_kind_id, 0);
			SetupPPObjCombo(this, CTLSEL_REPLPRSN_PRSN1, PPOBJ_PERSON, dest_psn_id, 0, reinterpret_cast<void *>(dest_kind_id));
			SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND2, PPOBJ_PERSONKIND, src_kind_id,  0);
			SetupPPObjCombo(this, CTLSEL_REPLPRSN_PRSN2, PPOBJ_PERSON, src_psn_id,  0, reinterpret_cast<void *>(src_kind_id));

			PPID   src_addr_id   = getCtrlLong(CTLSEL_REPLPRSN_ADDR1);
			PPID   dest_addr_id  = getCtrlLong(CTLSEL_REPLPRSN_ADDR2);
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_REPLPRSN_ADDR1, dest_psn_id, dest_addr_id);
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_REPLPRSN_ADDR2, src_psn_id, src_addr_id);
		}
		else {
			PersonListCtrlGroup::Rec grp_rec;
			getGroupData(ctlgroupPsnList, &grp_rec);
			PPIDArray dest_id_list = grp_rec.List;
			if(dest_id_list.getCount() <= 1) {
				PPID   dest_kind_id = grp_rec.PsnKindID;
				PPID   dest_psn_id = dest_id_list.getSingle();
				SetupPPObjCombo(this, CTLSEL_REPLPRSN_KIND1, PPOBJ_PERSONKIND, dest_kind_id, 0);
				SetupPPObjCombo(this, CTLSEL_REPLPRSN_PRSN1, PPOBJ_PERSON, dest_psn_id, 0, reinterpret_cast<void *>(dest_kind_id));
				grp_rec.PsnKindID = src_kind_id;
				grp_rec.List.Z().add(src_psn_id);
				setGroupData(ctlgroupPsnList, &grp_rec);
			}
		}
	}

	PPObjPerson PsnObj;
};

IMPL_HANDLE_EVENT(ReplPrsnDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_REPLPRSN_KIND1))
		replyKindSelected(0);
	else if(event.isCbSelected(CTLSEL_REPLPRSN_KIND2))
		replyKindSelected(1);
	else if(event.isCmd(cmSwap))
		Swap();
	else if(event.isCbSelected(CTLSEL_REPLPRSN_PRSN1))
		replyPersonSelected(0);
	else if(event.isCbSelected(CTLSEL_REPLPRSN_PRSN2))
		replyPersonSelected(1);
	else
		return;
	clearEvent(event);
}

/*static*/int PPObjPerson::ReplacePerson(PPID srcID, PPID srcKindID)
{
	int    ok = -1;
	PPLogger logger;
	SString msg_buf;
	SString temp_buf;
	ReplacePersonParam param;
	PPObjPerson psn_obj;
	ReplPrsnDialog * dlg = 0;
	THROW(psn_obj.CheckRights(PSNRT_UNITE, 0));
	THROW(CheckDialogPtrErr(&(dlg = new ReplPrsnDialog(0))));
	param.SrcID = srcID;
	param.SrcKindID = srcKindID;
	dlg->setDTS(&param);
	while(ExecView(dlg) == cmOK) {
		if(!dlg->getDTS(&param))
			PPError();
		else {
			uint   replace_obj_options = PPObject::use_transaction;
			if(!(param.Flags & param.fRemoveDest))
				replace_obj_options |= PPObject::not_repl_remove;
			else if(!(param.Flags & param.fDontConfirm))
				replace_obj_options |= PPObject::user_request;
			for(uint i = 0; i < param.DestIdList.getCount(); i++) {
				const PPID dest_id = param.DestIdList.get(i);
				PPLoadString("unite_persons", msg_buf.Z());
				GetPersonName(dest_id, temp_buf);
				msg_buf.CatDiv(':', 2).Cat(temp_buf).Space().Cat("-->");
				GetPersonName(param.SrcID, temp_buf);
				msg_buf.Space().Cat(temp_buf);
				logger.Log(msg_buf);
				if(!PPObject::ReplaceObj(PPOBJ_PERSON, dest_id, param.SrcID, replace_obj_options)) {
					logger.LogLastError();
				}
			}
			dlg->replyKindSelected(1);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*static*/int PPObjPerson::ReplaceDlvrAddr(PPID srcID)
{
	int    ok = -1;
	//PPID   dest_id = 0, src_id = srcID;
	ReplacePersonParam param;
	param.Flags |= param.fAddress;
	param.SrcID = srcID;
	PPObjPerson psn_obj;
	ReplPrsnDialog * dlg = 0;
	THROW(psn_obj.CheckRights(PSNRT_UNITEADDR, 0));
	THROW(CheckDialogPtrErr(&(dlg = new ReplPrsnDialog(1))));
	dlg->setDTS(&param);
	while(ExecView(dlg) == cmOK) {
		if(!dlg->getDTS(&param))
			PPError();
		else {
			for(uint i = 0; i < param.DestIdList.getCount(); i++) {
				const PPID dest_id = param.DestIdList.get(i);
				if(!PPObject::ReplaceObj(PPOBJ_LOCATION, dest_id, param.SrcID, PPObject::not_repl_remove|PPObject::user_request))
					PPError();
			}
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}


int PPObjPerson::GetExtName(PPID id, SString & rBuf)
{
	int    r = PPRef->GetPropVlrString(Obj, id, PSNPRP_EXTSTRDATA, rBuf);
	if(r > 0 && rBuf.IsEmpty())
		r = -1;
	return r;
}

int PPObjPerson::GetRelPersonList(PPID personID, PPID relTypeID, int reverse,
	PPIDArray * pList, int lastInHierarhy /*=0*/, PPIDArray * pHierarhIDList/*=0*/)
{
	int    ok = -1, first_iter = 0;
	LAssocArray rel_list;
	if(P_Tbl->GetRelList(personID, &rel_list, reverse) > 0) {
		LAssoc * p_item = 0;
		if(lastInHierarhy) {
			if(!pHierarhIDList) {
				THROW_MEM(pHierarhIDList = new PPIDArray);
				first_iter = 1;
			}
			pHierarhIDList->addUnique(personID);
		}
		for(uint i = 0; rel_list.enumItems(&i, (void **)&p_item);) {
			if(p_item->Val == relTypeID && p_item->Key) {
				if(lastInHierarhy) {
					if(!pHierarhIDList->lsearch(p_item->Key))
						if(GetRelPersonList(p_item->Key, relTypeID, reverse, pList, 1, pHierarhIDList) <= 0) {
							if(pList) {
								pList->addUnique(p_item->Key);
								ok = 1;
							}
						}
					if(first_iter)
						ok = 1;
				}
				else {
					if(pList)
						pList->addUnique(p_item->Key);
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	if(lastInHierarhy && pHierarhIDList)
		pHierarhIDList->freeByKey(personID, 0);
	if(first_iter)
		ZDELETE(pHierarhIDList);
	return ok;
}

int PPObjPerson::GetStaffAmtList(PPID id, StaffAmtList * pList)
{
	return PPRef->GetPropArray(Obj, id, SLPPRP_AMTLIST, pList);
}

int PPObjPerson::PutStaffAmtList(PPID id, const StaffAmtList * pList)
{
	return PPRef->PutPropArray(Obj, id, SLPPRP_AMTLIST, pList, 0);
}

int PPObjPerson::GetPersonListByDlvrLoc(PPID dlvrLocID, PPIDArray * pList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	LongArray temp_list;
	PropertyTbl::Key1 k1;
	MEMSZERO(k1);
	k1.ObjType = PPOBJ_PERSON;
	k1.Prop = PSNPRP_DLVRLOCLIST;
	while(p_ref->Prop.search(1, &k1, spGt) && k1.ObjType == PPOBJ_PERSON && k1.Prop == PSNPRP_DLVRLOCLIST) {
		const PPID person_id = p_ref->Prop.data.ObjID;
		temp_list.clear();
		THROW(p_ref->GetPropArrayFromRecBuf(&temp_list));
		if(temp_list.lsearch(dlvrLocID)) {
			ok = 1;
			if(pList) {
				THROW_SL(pList->addUnique(person_id));
			}
			else
				break;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::GetDlvrLocList(PPID personID, PPIDArray * pList)
{
	LongArray temp_list;
	int    r = PPRef->GetPropArray(Obj, personID, PSNPRP_DLVRLOCLIST, &temp_list);
	for(uint i = 0; i < temp_list.getCount(); i++) {
		const PPID loc_id = temp_list.get(i);
		uint pos = i+1;
		while(temp_list.lsearch(loc_id, &pos)) {
			temp_list.atFree(pos);
			pos = i+1;
		}
	}
	ASSIGN_PTR(pList, temp_list);
	return r;
}

int PPObjPerson::GetPacket(PPID id, PPPersonPacket * pPack, uint flags)
{
	int    ok = 1, r;
	Reference * p_ref = PPRef;
	uint   i;
	PPIDArray dlvr_loc_list;
	if(PPCheckGetObjPacketID(Obj, id)) {
		SString ext_str_buf;
		PPLocationPacket loc_pack;
		PropertyTbl::Rec cshr_prop;
		THROW(P_Tbl->Get(id, pPack));
		THROW(GetRegList(id, &pPack->Regs, BIN(flags & PGETPCKF_USEINHERITENCE)));
		if(pPack->Rec.MainLoc || pPack->Rec.RLoc) {
			if(pPack->Rec.MainLoc) {
				THROW(r = LocObj.GetPacket(pPack->Rec.MainLoc, &pPack->Loc));
				if(r < 0) {
					pPack->Rec.MainLoc = 0;
					pPack->Loc.destroy();
				}
			}
			if(pPack->Rec.RLoc) {
				THROW(r = LocObj.GetPacket(pPack->Rec.RLoc, &pPack->RLoc));
				if(r < 0) {
					pPack->Rec.RLoc = 0;
					pPack->RLoc.destroy();
				}
			}
		}
		THROW(GetDlvrLocList(id, &dlvr_loc_list));
		THROW(GetStaffAmtList(id, &pPack->Amounts));
		THROW(r = p_ref->GetProperty(Obj, id, PSNPRP_CASHIERINFO, &cshr_prop, sizeof(cshr_prop)));
		if(r > 0) {
			pPack->CshrInfo.Flags = CIF_CASHIER;
			STRNSCPY(pPack->CshrInfo.Password, (char *)cshr_prop.Text);
			pPack->CshrInfo.Rights = cshr_prop.Val1;
		}
		else
			MEMSZERO(pPack->CshrInfo);
		for(i = 0; i < dlvr_loc_list.getCount(); i++)
			if(LocObj.GetPacket(dlvr_loc_list.at(i), &loc_pack) > 0)
				pPack->AddDlvrLoc(loc_pack);
		THROW(P_Tbl->GetELinks(id, pPack->ELA));
		THROW(GetExtName(id, ext_str_buf));
		pPack->SetExtName(ext_str_buf);
		THROW(p_ref->Ot.GetList(Obj, id, &pPack->TagL));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjPerson::GetSingleBnkAcct(PPID personID, PPID bankID, PPID * pBnkAcctID, /*BankAccountTbl::Rec*/PPBankAccount * pRec)
{
	int    ok = -1;
	PPID   single_bacc_id = 0;
	TSVector <PPBankAccount> bacc_list;
	PPBankAccount pref_ba_rec;
	RegObj.GetBankAccountList(personID, &bacc_list);
	if(bacc_list.getCount()) {
		int    found = 0, pref_found = 0;
		int    pref_idx = -1;
		for(uint i = 0; !found && i < bacc_list.getCount(); i++) {
			PPBankAccount & r_rec = bacc_list.at(i);
			if(pBnkAcctID && *pBnkAcctID && r_rec.ID == *pBnkAcctID) {
				pref_idx = static_cast<int>(i);
				found = 1;
			}
			else if(!bankID || r_rec.BankID == bankID) {
				if(!pref_found && r_rec.Flags & /*BACCTF_PREFERRED*/PREGF_BACC_PREFERRED) {
					pref_idx = static_cast<int>(i);
					pref_found = 1;
				}
				else if(pref_idx < 0)
					pref_idx = static_cast<int>(i);
			}
		}
		if(pref_idx >= 0) {
			pref_ba_rec = bacc_list.at(pref_idx);
			single_bacc_id = pref_ba_rec.ID;
			ok = 1;
		}
	}
	ASSIGN_PTR(pRec, pref_ba_rec);
	ASSIGN_PTR(pBnkAcctID, single_bacc_id);
	return ok;
}

int PPObjPerson::GetBankData(PPID id, PPBank * pData)
{
	int    ok = 1;
	PersonTbl::Rec bnk_rec;
	if(Search(id, &bnk_rec) > 0) {
		SString temp_buf;
		RegisterArray regs;
		pData->ID = id;
		STRNSCPY(pData->Name, bnk_rec.Name);
		GetRegList(id, &regs, 1/*useInheritence*/); // @v9.2.1 useInheritence 0-->1
		regs.GetRegNumber(PPREGT_BIC, temp_buf);
		STRNSCPY(pData->BIC, temp_buf);
		regs.GetRegNumber(PPREGT_BNKCORRACC, temp_buf);
		STRNSCPY(pData->CorrAcc, temp_buf);
		LocObj.GetCity(bnk_rec.MainLoc, 0, &temp_buf.Z(), 1); // @v8.7.12 useCache=1
		STRNSCPY(pData->City, temp_buf);
		GetExtName(id, temp_buf);
		STRNSCPY(pData->ExtName, temp_buf);
	}
	else
		ok = -1;
	return ok;
}

int PPObjPerson::AddBankSimple(PPID * pID, const PPBank * pData, int use_ta)
{
	int    ok = 1, r;
	PPID   id = 0;
	PPObjWorld w_obj;
	PPPersonPacket pack;
	if(pData->Name[0]) {
		pack.Rec.Status = PPPRS_LEGAL;
		STRNSCPY(pack.Rec.Name, pData->Name);
		pack.SetExtName(pData->ExtName);
		pack.Kinds.add(PPPRK_BANK);
		if(pData->BIC[0])
			pack.AddRegister(PPREGT_BIC, pData->BIC, 0);
		if(pData->CorrAcc[0])
			pack.AddRegister(PPREGT_BNKCORRACC, pData->CorrAcc, 0);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(r = SearchMaxLike(&pack, &id, 0, PPREGT_BIC));
			if(r < 0) {
				if(pData->City[0])
					THROW(w_obj.AddSimple(&pack.Loc.CityID, WORLDOBJ_CITY, pData->City, 0, 0));
				THROW(PutPacket(&(id = 0), &pack, 0));
			}
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		id = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPObjPerson::UpdateAddress(PPID * pLocID, PPLocationPacket * pLocPack)
{
	int    ok = 1;
	if(pLocPack && !pLocPack->IsEmptyAddress()) {
		THROW(LocObj.PutPacket(pLocID, pLocPack, 0));
	}
	else if(*pLocID) {
		THROW(LocObj.PutPacket(pLocID, 0, 0));
		*pLocID = 0;
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPObjPerson::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	// @v10.9.1 Значительная переработка с целью удалить сначала статьи, связанные с персоналией id
	int    ok = -1;
	SETFLAG(options, not_addtolog, 1);
	if(!(options & user_request) || PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE) == cmYes) {
		options &= ~user_request;
		PPObjArticle ar_obj;
		{
			PPTransaction tra(BIN(options & use_transaction));
			THROW(tra);
			options &= ~use_transaction;
			{
				PPIDArray psn_id_list;
				PPIDArray ar_id_list;
				psn_id_list.add(id);
				ar_obj.GetByPersonList(0, &psn_id_list, &ar_id_list);
				if(ar_id_list.getCount()) {
					for(uint aridx = 0; aridx < ar_id_list.getCount(); aridx++) {
						const PPID ar_id = ar_id_list.get(aridx);
						// pExtraParam если !0, то предназначается для PPObjPerson, но не для иных типов объектов
						THROW(ar_obj.RemoveObjV(ar_id, pObjColl, options, 0/*pExtraParam*/));
					}
				}
			}
			THROW(PPObject::RemoveObjV(id, pObjColl, options, pExtraParam));
			THROW(tra.Commit());
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::Helper_PutSCard(PPID personID, PPPersonPacket * pPack, PPObjSCard * pScObj)
{
	int    ok = -1;
	if(personID && pPack && pScObj) {
		const PPSCardPacket * p_sc_pack = pPack->GetSCard();
		if(p_sc_pack) {
			PPSCardPacket sc_pack;
			sc_pack = *p_sc_pack;
			PPID   sc_id = sc_pack.Rec.ID;
			sc_pack.Rec.PersonID = personID;
			THROW(pScObj->PutPacket(&sc_id, &sc_pack, 0));
			sc_pack.Rec.ID = sc_id;
			pPack->SetSCard(&sc_pack, 0);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::PutPacket(PPID * pID, PPPersonPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    no_changes = 0;
	//
	// Признак наличия в БД адресов, имеющих не верного владельца.
	// В случае ошибки необходимо попытаться установить правильного владельца у 'тих адресов.
	//
	int    is_in_db_mism_owner_loc = 0;
	uint   i;
	PPID   id = DEREFPTRORZ(pID);
	const  int is_new = BIN(pPack && !id);
	const  int do_index_phones = BIN(CConfig.Flags2 & CCFLG2_INDEXEADDR);
	PPID   action = 0;
	PPID   dirty_id = 0;
	SString temp_buf;
	PPID   hid = 0; // @v10.5.3 Версионный идентификатор для сохранения в системном журнале
	SBuffer hist_buf; // @v10.5.3
	Reference * p_ref = PPRef;
	ObjVersioningCore * p_ovc = p_ref->P_OvT; // @v10.5.3
	PPPersonPacket org_pack;
	PPObjSCard * p_sc_obj = 0;
	PPLocationPacket loc_pack;
	PPIDArray dlvr_loc_list;
	PPIDArray * p_dlvr_loc_list = 0;
	const int is_rt_mod = CheckRights(PPR_MOD, 0);
	if(pPack && pPack->GetSCard())
		THROW_MEM(p_sc_obj = new PPObjSCard);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID && !is_rt_mod && p_sc_obj) {
			//
			// Специальный случай: нет прав доступа на изменение персоналии, но есть право на создание (изменение) карты,
			// прикрепленной к пакету персоналии.
			//
			THROW(Helper_PutSCard(*pID, pPack, p_sc_obj));
		}
		else {
			if(id) { // Update or Delete packet: we have to extract original packet (org_pack) in order to compare with new one and store history
				THROW(GetPacket(id, &org_pack, 0) > 0);
			}
			if(pPack) {
				if(id) { // Update packet
					{
						for(i = 0; i < pPack->Regs.getCount(); i++) {
							RegisterTbl::Rec & r_reg_rec = pPack->Regs.at(i);
							r_reg_rec.ObjType = PPOBJ_PERSON;
							r_reg_rec.ObjID = id;
						}
						if(IsPacketEq(*pPack, org_pack, 0))
							no_changes = 1;
					}
					for(i = 0; !is_in_db_mism_owner_loc && org_pack.EnumDlvrLoc(&i, &loc_pack) > 0;) {
						if(loc_pack.OwnerID != id)
							is_in_db_mism_owner_loc = 1;
					}
					if(no_changes) {
						ok = -1;
						//
						// Необходимо обработать специальный случай: проверка и, если необходимо, установка
						// идентификатора владельца адресов доставки.
						//
						for(i = 0; pPack->EnumDlvrLoc(&i, &loc_pack) > 0;) {
							if(loc_pack.OwnerID != id) {
								loc_pack.OwnerID = id;
								THROW(UpdateAddress(&loc_pack.ID, &loc_pack));
								ok = 1;
							}
						}
					}
					else {
						int    is_name_updated = 0;
						PersonTbl::Rec org_rec;
						PPIDArray losed_kinds;
						PPIDArray kind_list;
						THROW(is_rt_mod);
						// @v10.5.3 {
						if(!is_new && DoObjVer_Person) {
							if(p_ovc && p_ovc->InitSerializeContext(0)) {
								SSerializeContext & r_sctx = p_ovc->GetSCtx();
								THROW(SerializePacket(+1, &org_pack, hist_buf, &r_sctx));
							}
						}
						// } @v10.5.3
						THROW(P_Tbl->GetKindList(pPack->Rec.ID, &kind_list));
						THROW_SL(losed_kinds.addUniqueExclusive(&kind_list, &pPack->Kinds));
						THROW(Search(id, &org_rec) > 0);
						THROW(UpdateAddress(&pPack->Rec.MainLoc, &pPack->Loc));
						THROW(UpdateAddress(&pPack->Rec.RLoc, &pPack->RLoc));
						//
						// Обновление списка адресов доставки {
						//
						THROW(GetDlvrLocList(id, &dlvr_loc_list));
						{
							LAssocArray bill_dlvr_addr_list;
							int    bill_dlvr_addr_list_inited = 0;
							for(uint k = 0; k < dlvr_loc_list.getCount(); k++) {
								PPID   loc_id = dlvr_loc_list.get(k);
								int    found = 0;
								if(loc_id && LocObj.GetPacket(loc_id, &loc_pack) > 0) {
									{
										// @todo Добавить в PPPersonPacket функцию GetDlvrLocByID()
										PPLocationPacket loc_pack2;
										for(uint j = 0; !found && pPack->EnumDlvrLoc(&j, &loc_pack2) > 0;)
											if(loc_pack2.ID == loc_id)
												found = 1;
									}
									if(!found) {
										PPID   ref_bill_id = 0;
										if(pPack->UpdFlags & PPPersonPacket::ufDontRmvDlvrLoc) {
											THROW(pPack->AddDlvrLoc(loc_pack));
										}
										else {
											int    is_there_another_link = 0;
											PPIDArray link_psn_list;
											if(GetPersonListByDlvrLoc(loc_id, &link_psn_list) > 0) {
												for(uint n = 0; n < link_psn_list.getCount(); n++) {
													if(link_psn_list.get(n) != id && Search(link_psn_list.get(n), 0) > 0) {
														is_there_another_link = 1;
														break;
													}
												}
											}
											if(!is_there_another_link) {
												//
												// Если нет других персоналий, ссылающихся на удаляемый адрес, то...
												//
												// Прежде чем удалить адрес необходимо убедиться, что не существует документов,
												// которые на него ссылаются.
												//
												if(!bill_dlvr_addr_list_inited) {
													BillObj->P_Tbl->GetDlvrAddrList(&bill_dlvr_addr_list);
													bill_dlvr_addr_list_inited = 1;
												}
												if(bill_dlvr_addr_list.SearchByVal(loc_id, &ref_bill_id, 0)) {
													THROW(pPack->AddDlvrLoc(loc_pack));
												}
												else {
													THROW(LocObj.PutRecord(&loc_id, 0, 0));
												}
											}
										}
									}
								}
							}
						}
						dlvr_loc_list.freeAll();
						for(i = 0; pPack->EnumDlvrLoc(&i, &loc_pack) > 0;) {
							loc_pack.OwnerID = id;
							THROW(UpdateAddress(&loc_pack.ID, &loc_pack));
							if(loc_pack.ID)
								dlvr_loc_list.add(loc_pack.ID);
						}
						p_dlvr_loc_list = dlvr_loc_list.getCount() ? &dlvr_loc_list : 0;
						THROW(p_ref->PutPropArray(Obj, id, PSNPRP_DLVRLOCLIST, p_dlvr_loc_list, 0));
						// }
						if(pPack->UpdFlags & PPPersonPacket::ufDontChgImgFlag)
							SETFLAG(pPack->Rec.Flags, PSNF_HASIMAGES, org_rec.Flags & GF_HASIMAGES);
						if(!(pPack->UpdFlags & PPPersonPacket::ufDontChgStaffAmt))
							THROW(PutStaffAmtList(id, &pPack->Amounts));
						THROW(P_Tbl->Put(&id, pPack, 0));
						if(!sstreq(pPack->Rec.Name, org_rec.Name))
							THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, PPOBJ_PERSON, id, pPack->Rec.Name, 0));
						action = PPACN_OBJUPD;
						dirty_id = id;
					}
				}
				else { // Add packet
					THROW(CheckRights(PPR_INS));
					SETIFZ(pPack->Loc.Type, LOCTYP_ADDRESS);
					THROW(LocObj.PutRecord(&pPack->Rec.MainLoc, &pPack->Loc, 0));
					SETIFZ(pPack->RLoc.Type, LOCTYP_ADDRESS);
					THROW(LocObj.PutRecord(&pPack->Rec.RLoc, &pPack->RLoc, 0));
					THROW(P_Tbl->Put(&id, pPack, 0));
					for(i = 0; pPack->EnumDlvrLoc(&i, &loc_pack) > 0;) {
						SETIFZ(loc_pack.Type, LOCTYP_ADDRESS);
						loc_pack.OwnerID = id;
						THROW(LocObj.PutPacket(&loc_pack.ID, &loc_pack, 0));
						dlvr_loc_list.add(loc_pack.ID);
						p_dlvr_loc_list = &dlvr_loc_list;
					}
					THROW(p_ref->PutPropArray(Obj, id, PSNPRP_DLVRLOCLIST, p_dlvr_loc_list, 0));
					THROW(p_ref->PutPropArray(Obj, id, SLPPRP_AMTLIST, &pPack->Amounts, 0));
					pPack->Rec.ID = id;
					*pID = id;
					action = PPACN_OBJADD;
				}
				if(!no_changes) {
					THROW(RegObj.P_Tbl->PutByPerson(id, &pPack->Regs, 0));
					if(do_index_phones) {
						StringSet phone_list;
						pPack->ELA.GetListByType(ELNKRT_PHONE, phone_list);
						pPack->ELA.GetListByType(ELNKRT_INTERNALEXTEN, phone_list); // @v10.0.0
						PPObjID objid(Obj, id);
						// @v10.0.0 {
						if(action != PPACN_OBJADD) {
							StringSet org_phone_list;
							org_pack.ELA.GetListByType(ELNKRT_PHONE, org_phone_list);
							org_pack.ELA.GetListByType(ELNKRT_INTERNALEXTEN, org_phone_list);
							for(uint orgplp = 0; org_phone_list.get(&orgplp, temp_buf);) {
								if(!phone_list.search(temp_buf, 0, 1)) {
									THROW(LocObj.P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
								}
							}
						}
						// } @v10.0.0
						for(uint plp = 0; phone_list.get(&plp, temp_buf);) {
							THROW(LocObj.P_Tbl->IndexPhone(temp_buf, &objid, 0, 0));
						}
					}
					THROW(P_Tbl->PutELinks(id, &pPack->ELA, 0));
					if(pPack->CshrInfo.Flags & CIF_MODIFIED) {
						PropertyTbl::Rec  cshr_prop;
						strnzcpy((char *)cshr_prop.Text,  pPack->CshrInfo.Password, sizeof(cshr_prop.Text));
						cshr_prop.Val1 = pPack->CshrInfo.Rights;
						THROW(p_ref->PutProp(Obj, id, PSNPRP_CASHIERINFO, (pPack->CshrInfo.Flags & CIF_CASHIER) ? &cshr_prop : 0));
					}
					//
					pPack->GetExtName(temp_buf.Z());
					THROW(p_ref->PutPropVlrString(Obj, id, PSNPRP_EXTSTRDATA, temp_buf));
					//
					// @v9.0.4 THROW(BaObj.UpdateList(id, &pPack->BAA, 0));
					if(pPack->LinkFiles.IsChanged(id, 0L)) {
						THROW_PP(CheckRights(PSNRT_UPDIMAGE), PPERR_NRT_UPDIMAGE);
						pPack->LinkFiles.Save(id, 0L);
					}
					for(i = 0; i < pPack->Kinds.getCount(); i++) {
						const PPID kind_id = pPack->Kinds.at(i);
						THROW(SendObjMessage(DBMSG_PERSONACQUIREKIND, PPOBJ_ARTICLE,   PPOBJ_PERSON, id, reinterpret_cast<void *>(kind_id), 0));
						THROW(SendObjMessage(DBMSG_PERSONACQUIREKIND, PPOBJ_PROCESSOR, PPOBJ_PERSON, id, reinterpret_cast<void *>(kind_id), 0));
					}
					THROW(p_ref->Ot.PutList(Obj, pPack->Rec.ID, &pPack->TagL, 0));
				}
				THROW(Helper_PutSCard(id, pPack, p_sc_obj));
			}
			else if(id) { // Remove packet
				ObjLinkFiles _lf(PPOBJ_PERSON);
				THROW(CheckRights(PPR_DEL));
				_lf.Save(id, 0L);
				// @v10.5.3 (мы уже извлекли оригинальный пакет выше) THROW(Search(id, 0) > 0);
				// @v10.5.3 {
				if(DoObjVer_Person && p_ovc && p_ovc->InitSerializeContext(0)) {
					SSerializeContext & r_sctx = p_ovc->GetSCtx();
					THROW(SerializePacket(+1, &org_pack, hist_buf, &r_sctx));
				}
				// } @v10.5.3
				if(org_pack.Rec.MainLoc)
					THROW(LocObj.PutRecord(&org_pack.Rec.MainLoc, 0, 0));
				if(org_pack.Rec.RLoc)
					THROW(LocObj.PutRecord(&org_pack.Rec.RLoc, 0, 0));
				//
				// Удаляем список адресов доставки {
				//
				THROW(GetDlvrLocList(id, &dlvr_loc_list));
				for(i = 0; i < dlvr_loc_list.getCount(); i++) {
					PPID dlvr_loc_id = dlvr_loc_list.get(i);
					if(dlvr_loc_id)
						THROW(LocObj.PutRecord(&dlvr_loc_id, 0, 0));
				}
				THROW(p_ref->PutPropArray(Obj, id, PSNPRP_DLVRLOCLIST, 0, 0));
				// }
				THROW(RegObj.P_Tbl->PutByPerson(id, 0, 0));
				if(do_index_phones) {
					PPELinkArray ela;
					StringSet phone_list;
					THROW(P_Tbl->GetELinks(id, ela));
					ela.GetListByType(ELNKRT_PHONE, phone_list);
					ela.GetListByType(ELNKRT_INTERNALEXTEN, phone_list); // @v10.0.0
					PPObjID objid(Obj, id);
					for(uint plp = 0; phone_list.get(&plp, temp_buf);) {
						THROW(LocObj.P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
					}
				}
				THROW(P_Tbl->PutELinks(id, 0, 0));
				// @v10.5.3THROW(p_ref->PutPropVlrString(Obj, id, PSNPRP_EXTSTRDATA, 0));
				// @v9.0.4 THROW(BaObj.P_Tbl->RemoveList(id, 0));
				// @v10.5.3 THROW(p_ref->PutProp(Obj, id, PSNPRP_CASHIERINFO, 0));
				// @v10.5.3 THROW(p_ref->PutPropArray(Obj, id, SLPPRP_AMTLIST, 0, 0));
				THROW(P_Tbl->Put(&id, 0, 0));
				THROW(p_ref->RemoveProperty(Obj, id, 0, 0)); // @v10.5.3
				THROW(RemoveSync(id)); // @v10.5.3
				THROW(p_ref->Ot.PutList(Obj, id, 0, 0));
				action = PPACN_OBJRMV;
				dirty_id = id;
			}
			// @v10.5.3 {
			if(id && DoObjVer_Person && hist_buf.GetAvailableSize()) {
				if(p_ovc && p_ovc->InitSerializeContext(0)) {
					THROW(p_ovc->Add(&hid, PPObjID(Obj, id), &hist_buf, 0));
				}
			}
			// } @v10.5.3
		}
		if(action && id)
			DS.LogAction(action, Obj, id, hid, 0);
		THROW(tra.Commit());
	}
	if(dirty_id)
		Dirty(dirty_id); // @v9.5.10 @fix id-->dirty_id
	CATCH
		{
			int do_recover_loc_owners = BIN(BTROKORNFOUND && pPack && *pID && is_in_db_mism_owner_loc);
			if(do_recover_loc_owners) {
				//
				// Специальная процедура, корректирующая владельцев адресов персоналии в случае сбоя.
				// Необходимость такой процедуры обусловлена возможным несоответствием владельца адреса
				// при акцепте персоналии из другого раздела. В таких случаях сначала акцептируется локация,
				// а потом - персоналия, владеющая локацией. У локации при этом устанавливается идент владельца,
				// соответствующий разделу-отправителю.
				// Если персоналия принялась правильно, то владелец корректируется (см. код выше), в противном
				// случае у локации может остаться неверный владелец.
				//
				// Естественно, в следующем блоке обработка ошибок отключена.
				//
				PPTransaction tra(use_ta);
				if(!!tra) {
					for(i = 0; org_pack.EnumDlvrLoc(&i, &loc_pack) > 0;) {
						if(loc_pack.OwnerID != id) {
							loc_pack.OwnerID = id;
							UpdateAddress(&loc_pack.ID, &loc_pack);
						}
					}
					tra.Commit();
				}
			}
			if(is_new) {
				*pID = 0;
				pPack->Rec.ID = 0;
			}
		}
		ok = 0;
	ENDCATCH
	delete p_sc_obj;
	return ok;
}

int PPObjPerson::ValidatePacket(const PPPersonPacket * pPack, long flags)
{
	int    ok = 1;
	if(pPack) {
		SString temp_buf;
		THROW_PP(pPack->Rec.Status, PPERR_PERSONSTATUSNEEDED);
		THROW_PP((temp_buf = pPack->Rec.Name).NotEmptyS(), PPERR_NAMENEEDED);
		THROW_PP(pPack->Kinds.getCount(), PPERR_LASTPERSONSKIND);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
int PPObjPerson::SetupDlvrLocCombo(TDialog * dlg, uint ctlID, PPID personID, PPID locID)
{
	int    ok = 0;
	PPIDArray dlvr_loc_list;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(personID)
		GetDlvrLocList(personID, &dlvr_loc_list);
	else if(locID) {
		dlvr_loc_list.add(locID);
	}
	if(p_combo) {
		SString temp_buf;
		StrAssocArray * p_list = new StrAssocArray;
		for(uint i = 0; i < dlvr_loc_list.getCount(); i++) {
			PPID   loc_id = dlvr_loc_list.at(i);
			LocationTbl::Rec loc_rec;
			if(LocObj.Search(loc_id, &loc_rec) > 0) {
				LocObj.P_Tbl->GetAddress(loc_rec, 0, temp_buf);
				p_list->Add(loc_id, temp_buf);
			}
		}
		PPObjListWindow * p_lw = new PPObjListWindow(PPOBJ_LOCATION, p_list, 0, 0);
		if(p_lw) {
			p_combo->setListWindow(p_lw, locID);
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

int PPObjPerson::Helper_GetAddrID(const PersonTbl::Rec * pRec, PPID psnID, PPID dlvrAddrID, int option, PPID * pAddrID)
{
	PPID   addr_id = (option == PSNGETADDRO_DLVRADDR) ? dlvrAddrID : 0;
	if(addr_id <= 0) {
		PersonTbl::Rec rec;
		if(pRec == 0 && Fetch(psnID, &rec) > 0)
			pRec = &rec;
		if(pRec) {
			if(oneof2(option, PSNGETADDRO_DLVRADDR, PSNGETADDRO_REALADDR))
				addr_id = pRec->RLoc;
			if(addr_id <= 0)
				addr_id = pRec->MainLoc;
		}
	}
	ASSIGN_PTR(pAddrID, addr_id);
	return (addr_id > 0) ? 1 : -1;
}

int PPObjPerson::GetAddrID(PPID id, PPID dlvrAddrID, int option, PPID * pAddrID)
{
	return Helper_GetAddrID(0, id, dlvrAddrID, option, pAddrID);
}
//
//
//
PPObjPerson::SubstParam::SubstParam() : Sgp(sgpNone), Flags(0)
{
}

void PPObjPerson::SubstParam::Init(SubstGrpPerson sgp)
{
	Sgp = sgp;
	Flags = 0;
}

int PPObjPerson::SubstParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(Sgp)), &Sgp, 0, rBuf));
	THROW_SL(pCtx->Serialize(dir, Flags, rBuf));
	CATCHZOK
	return ok;
}

int PPObjPerson::Subst(PPID srcID, PPID dlvrLocID, SubstParam * pParam, long flags, PPID * pDestID)
{
	int    ok = -1;
	PPID   dest_id = 0;
	PPID   src_id = srcID;
	PPID   acs_id = 0;
	if(src_id & sgpArticleMask) {
		PPID   ar_id = (src_id & ~sgpArticleMask);
		if(pParam->Sgp == sgpAccSheet) {
			PPID   lnk_obj_id = 0;
			if(GetArticleSheetID(ar_id, &acs_id, &lnk_obj_id) > 0)
				dest_id = acs_id;
		}
		else {
			PPID   temp_id = ObjectToPerson(ar_id, &acs_id);
			if(temp_id)
				src_id = temp_id;
			else
				dest_id = src_id; // Результат - ИД статьи с флагом, укаызвающим на это
		}
	}
	if(dest_id == 0) {
		PersonTbl::Rec psn_rec;
		if(pParam->Sgp && pParam->Sgp != sgpBillAgent && pParam->Sgp != sgpVesselAgent) {
			if(src_id && Fetch(src_id, &psn_rec) > 0) {
				if(oneof3(pParam->Sgp, sgpCity, sgpCountry, sgpRegion)) {
					LocationTbl::Rec loc_rec;
					WorldTbl::Rec w_rec;
					PPID   loc_id = 0;
					if(pParam->Flags & SubstParam::fSubstDlvrAddr)
						loc_id = dlvrLocID;
					if(loc_id == 0)
						loc_id = (pParam->Flags & SubstParam::fSubstPersonRAddr && psn_rec.RLoc) ? psn_rec.RLoc : psn_rec.MainLoc;
					if(LocObj.Search(loc_id, &loc_rec) > 0) {
						if(pParam->Sgp == sgpCity)
							dest_id = loc_rec.CityID;
						else if(pParam->Sgp == sgpCountry) {
							if(pParam->WObj.GetCountryByChild(loc_rec.CityID, &w_rec) > 0)
								dest_id = w_rec.ID;
						}
						else { // pParam->Sgp == sgpRegion
							if(pParam->WObj.Fetch(loc_rec.CityID, &w_rec) > 0)
								dest_id = w_rec.ParentID;
						}
					}
				}
				else if(pParam->Sgp == sgpCategory)
					dest_id = psn_rec.CatID;
				else if(pParam->Sgp > sgpFirstRelation) {
					int    last_in_hierarh = BIN(flags & PSNSUBSTF_LASTRELINHIERARH);
					PPIDArray rel_list;
					if(GetRelPersonList(src_id, (long)(pParam->Sgp - sgpFirstRelation), 0, &rel_list, last_in_hierarh) > 0 && rel_list.getCount())
						dest_id = rel_list.at(0);
					else
						dest_id = src_id;
				}
				ok = 1;
			}
		}
		else
			dest_id = src_id;
	}
	ASSIGN_PTR(pDestID, dest_id);
	return ok;
}

int PPObjPerson::GetSubstObjType(long id, const SubstParam * pParam, PPObjID * pObjId) const
{
	int    ok = 1;
	PPObjID obj_id;
	if(id & sgpArticleMask)
		obj_id.Set(PPOBJ_ARTICLE, (id & ~sgpArticleMask));
	else if(oneof3(pParam->Sgp, sgpCity, sgpCountry, sgpRegion))
		obj_id.Set(PPOBJ_WORLD, id);
	else if(pParam->Sgp == sgpCategory)
		obj_id.Set(PPOBJ_PRSNCATEGORY, id);
	else if(pParam->Sgp == sgpAccSheet)
		obj_id.Set(PPOBJ_ACCSHEET, id);
	else
		obj_id.Set(PPOBJ_PERSON, id);
	ASSIGN_PTR(pObjId, obj_id);
	return 1;
}

void PPObjPerson::GetSubstText(PPID id, PPID dlvrLocID, SubstParam * pParam, SString & rBuf)
{
	rBuf.Z();
	SString temp_buf, addr_buf;
	if(id & sgpArticleMask)
		GetArticleName((id & ~sgpArticleMask), rBuf);
	else {
		if(oneof3(pParam->Sgp, sgpCity, sgpCountry, sgpRegion)) {
			WorldTbl::Rec w_rec;
			if(pParam->WObj.Fetch(id, &w_rec) > 0)
				rBuf = w_rec.Name;
		}
		else if(pParam->Sgp == sgpCategory)
			GetObjectName(PPOBJ_PRSNCATEGORY, id, rBuf);
		else if(pParam->Sgp == sgpAccSheet) {
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			if(acs_obj.Fetch(id, &acs_rec) > 0)
				rBuf = acs_rec.Name;
		}
		else {
			PersonTbl::Rec psn_rec;
			if(Fetch(id, &psn_rec) > 0)
				rBuf = psn_rec.Name;
			if(dlvrLocID) {
				LocObj.GetAddress(dlvrLocID, 0, addr_buf);
				rBuf.CatDivIfNotEmpty(';', 2).Cat(addr_buf);
			}
		}
		if(rBuf.IsEmpty())
			rBuf.CatChar('#').Cat(id);
	}
}
//
// Prototype (V_PERSON.CPP)
int GetExtRegListIds(PPID psnKindID, PPID statusID, PPIDArray * pList);
// Prototype
static int EditPersonRelList(PPPersonPacket *);
//
//
//
static PPID SelectPersonKind()
{
	PPID   id = 0;
	int    r = PPSelectObject(PPOBJ_PERSONKIND, &id, 0, 0);
	return (r > 0) ? id : (r ? -1 : 0);
}
//
//
//
class AddrListDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPPersonPacket);
public:
	AddrListDialog() : PPListDialog(DLG_ADDRLIST, CTL_ADDRLIST_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		updateList(0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    editAddr(uint pos);
};

int AddrListDialog::setupList()
{
	int    ok = 1;
	PPLocationPacket loc_pack;
	PPObjWorld w_obj;
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; ok && Data.EnumDlvrLoc(&i, &loc_pack);) {
		WorldTbl::Rec w_rec;
		ss.clear();
		ss.add(temp_buf.Z().Cat(loc_pack.ID));
		ss.add((w_obj.Fetch(loc_pack.CityID, &w_rec) > 0) ? w_rec.Name : 0);
		LocationCore::GetExField(&loc_pack, LOCEXSTR_SHORTADDR, temp_buf.Z());
		if(!temp_buf.NotEmptyS()) {
			LocationCore::GetExField(&loc_pack, LOCEXSTR_FULLADDR, temp_buf.Z());
			if(!temp_buf.NotEmptyS()) {
				LocationCore::GetExField(&loc_pack, LOCEXSTR_CONTACT, temp_buf.Z());
				if(!temp_buf.NotEmptyS()) {
					LocationCore::GetExField(&loc_pack, LOCEXSTR_EMAIL, temp_buf.Z());
				}
			}
		}
		ss.add(temp_buf);
		ss.add(loc_pack.Code);
		if(!addStringToList(i, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

int AddrListDialog::editAddr(uint pos)
{
	int    ok = -1;
	PPObjLocation loc_obj;
	PPLocationPacket loc_pack;
	if(pos == 0) {
		loc_pack.Type = LOCTYP_ADDRESS;
		if(loc_pack.ID == 0)
			loc_obj.InitCode(&loc_pack);
	}
	else if(!Data.GetDlvrLocByPos(pos-1, &loc_pack))
		ok = 0;
	if(ok && loc_obj.EditDialog(LOCTYP_ADDRESS, &loc_pack, 0) == cmOK)
		ok = (pos == 0) ? Data.AddDlvrLoc(loc_pack) : Data.PutDlvrLoc(pos-1, &loc_pack);
	return ok;
}

int AddrListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(editAddr(0) > 0) {
		ASSIGN_PTR(pPos, (long)(Data.GetDlvrLocCount()-1));
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	return ok;
}

int AddrListDialog::editItem(long pos, long /*id*/)
{
	return (editAddr((uint)pos+1) > 0) ? 1 : -1;
}

int AddrListDialog::delItem(long pos, long /*id*/)
{
	if(pos >= 0 && pos < (long)Data.GetDlvrLocCount())
		return Data.PutDlvrLoc((uint)pos, 0) ? 1 : -1;
	return -1;
}
//
//
//
int EditELink(PPELink * pLink); // @prototype(elinkdlg.cpp)

#define GRP_IBG 1

class PersonDialogBase : public TDialog {
public:
	explicit PersonDialogBase(uint dlgID) : TDialog(dlgID), DupID(0)
	{
		TView * p_ctrl = getCtrlView(CTL_PERSON_GENDER);
		if(p_ctrl) {
			AddClusterAssocDef(CTL_PERSON_GENDER, 0, GENDER_UNDEF);
			AddClusterAssocDef(CTL_PERSON_GENDER, 1, GENDER_MALE);
			AddClusterAssocDef(CTL_PERSON_GENDER, 2, GENDER_FEMALE);
		}
	}
	void   SetupPhoneOnInit(const char * pPhone)
	{
		InitPhone = pPhone;
	}
	PPID   GetDupID() const { return DupID; }
protected:
	DECL_DIALOG_DATA(PPPersonPacket);
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_PERSON_STATUS)) {
			SetupGender();
			GetDOB();
			SetupDOB();
			clearEvent(event);
		}
		else
			return;
	}
	int SetupGender()
	{
		int    ok = -1;
		TView * p_ctrl = getCtrlView(CTL_PERSON_GENDER);
		if(p_ctrl) {
			getCtrlData(CTLSEL_PERSON_STATUS, &Data.Rec.Status);
			PPObjPersonStatus ps_obj;
			PPPersonStatus ps_rec;
			if(ps_obj.Fetch(Data.Rec.Status, &ps_rec) > 0 && ps_rec.Flags & PSNSTF_PRIVATE)
				showCtrl(CTL_PERSON_GENDER, 1);
			else
				showCtrl(CTL_PERSON_GENDER, 0);
		}
		return ok;
	}
	void GetDOB()
	{
		LDATE  dob = ZERODATE;
		TView * p_ctrl = getCtrlView(CTL_PERSON_DOB);
		if(p_ctrl && p_ctrl->IsInState(sfVisible) && getCtrlData(CTL_PERSON_DOB, &dob)) {
			if(checkdate(dob)) {
				ObjTagItem dob_item;
				dob_item.Init(PPTAG_PERSON_DOB);
				dob_item.SetDate(PPTAG_PERSON_DOB, dob);
				Data.TagL.PutItem(PPTAG_PERSON_DOB, &dob_item);
			}
			else
				Data.TagL.PutItem(PPTAG_PERSON_DOB, 0);
		}
	}
	int SetupDOB()
	{
		int    ok = -1;
		getCtrlData(CTLSEL_PERSON_STATUS, &Data.Rec.Status);
		PPObjPersonStatus ps_obj;
		PPPersonStatus ps_rec;
		if(ps_obj.Fetch(Data.Rec.Status, &ps_rec) > 0 && ps_rec.Flags & PSNSTF_PRIVATE) {
			showCtrl(CTL_PERSON_DOB, 1);
			showCtrl(CTLCAL_PERSON_DOB, 1);
			const ObjTagItem * p_dob_tag = Data.TagL.GetItem(PPTAG_PERSON_DOB);
			if(p_dob_tag) {
				LDATE  dob = ZERODATE;
				p_dob_tag->GetDate(&dob);
				setCtrlDate(CTL_PERSON_DOB, dob);
				ok = 1;
			}
		}
		else {
			showCtrl(CTL_PERSON_DOB, 0);
			showCtrl(CTLCAL_PERSON_DOB, 0);
		}
		if(ok <= 0)
			setCtrlDate(CTL_PERSON_DOB, ZERODATE);
		return ok;
	}
	PPObjPerson PsnObj;
	PPID   DupID;
	SString Name_;
	SString InitPhone;
};

class PersonDialog : public PersonDialogBase {
public:
	explicit PersonDialog(int dlgID) : PersonDialogBase(dlgID), IsCashier(0)
	{
		if(!PsnObj.RegObj.CheckRights(PPR_READ))
			enableCommand(cmPersonReg, 0);
		if(!PsnObj.RegObj.CheckRights(PPR_READ))
			enableCommand(cmPersonBAcct, 0);
		CashiersPsnKindID = GetCashiersPsnKindID();
		PPObjSecur obj_secur(PPOBJ_USR, 0);
		if(!obj_secur.CheckRights(PPR_READ))
			enableCommand(cmCashierRights, 0);
		addGroup(GRP_IBG, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_PERSON_IMAGE,
			cmAddImage, cmDelImage, PsnObj.CheckRights(PSNRT_UPDIMAGE), ImageBrowseCtrlGroup::fUseExtOpenDlg));
		Ptb.SetBrush(brushHumanName,    SPaintObj::bsSolid, LightenColor(GetColorRef(SClrYellow), 0.8f), 0);
		Ptb.SetBrush(brushHumanNameFem, SPaintObj::bsSolid, LightenColor(GetColorRef(SClrRed), 0.8f), 0);
		Ptb.SetBrush(brushHumanNameMus, SPaintObj::bsSolid, LightenColor(GetColorRef(SClrBlue), 0.8f), 0);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_PERSON_KIND));
		LocationFilt loc_flt(LOCTYP_DIVISION);
		IsCashier = BIN(Data.CshrInfo.Flags & CIF_CASHIER);
		if(Data.Rec.ID) {
			PrevKindList = Data.Kinds;
			if(!PsnObj.CheckRights(PPR_MOD)) {
				PPLoadText(PPTXT_NORIGHTS_PERSON_UPD, temp_buf);
				setStaticText(CTL_PERSON_ST_INFO, temp_buf);
			}
		}
		setCtrlLong(CTL_PERSON_ID, Data.Rec.ID);
		SetupPPObjCombo(this, CTLSEL_PERSON_STATUS, PPOBJ_PRSNSTATUS, Data.Rec.Status, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_PERSON_DIV, PPOBJ_LOCATION, Data.Rec.Division, 0, &loc_flt);
		setCtrlData(CTL_PERSON_NAME, Data.Rec.Name);
		Data.GetExtName(temp_buf);
		setCtrlString(CTL_PERSON_EXTNAME, temp_buf);
		p_list->setDef(createKindListDef());
		p_list->Draw_();
		//
		AddClusterAssoc(CTL_PERSON_FLAGS, 0, PSNF_NOVATAX);
		AddClusterAssoc(CTL_PERSON_FLAGS, 1, PSNF_NONOTIFICATIONS);
		AddClusterAssoc(CTL_PERSON_FLAGS, 2, PSNF_DONTSENDCCHECK); // @v11.3.5
		SetClusterData(CTL_PERSON_FLAGS, Data.Rec.Flags);
		//
		// @v11.1.12 setCtrlData(CTL_PERSON_MEMO, Data.Rec.Memo);
		setCtrlString(CTL_PERSON_MEMO, Data.SMemo); // @v11.1.12
		SetupPPObjCombo(this, CTLSEL_PERSON_CATEGORY, PPOBJ_PRSNCATEGORY, Data.Rec.CatID, OLW_CANINSERT);
		SetupGender(); // @v10.9.0
		SetClusterData(CTL_PERSON_GENDER, PersonCore::GetGender(Data.Rec)); // @v10.9.0
		SetupCtrls();
		{
			ImageBrowseCtrlGroup::Rec rec;
			Data.LinkFiles.Init(PPOBJ_PERSON);
			if(Data.Rec.Flags & PSNF_HASIMAGES)
				Data.LinkFiles.Load(Data.Rec.ID, 0L);
			Data.LinkFiles.At(0, rec.Path);
			setGroupData(GRP_IBG, &rec);
		}
		if(Data.Rec.Status)
			selectCtrl(CTL_PERSON_NAME);
		enableCommand(cmSelectAnalog, !Data.Rec.ID);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok  = 1;
		uint   sel = 0, i;
		SString temp_buf;
		getCtrlData(sel = CTLSEL_PERSON_STATUS, &Data.Rec.Status);
		getCtrlData(sel = CTLSEL_PERSON_DIV, &Data.Rec.Division);
		getCtrlData(sel = CTL_PERSON_NAME, Data.Rec.Name);
		strip(Data.Rec.Name);
		getCtrlString(CTL_PERSON_EXTNAME, temp_buf);
		Data.SetExtName(temp_buf.Strip());
		GetClusterData(CTL_PERSON_FLAGS, &Data.Rec.Flags);
		// @v11.1.12 getCtrlData(CTL_PERSON_MEMO, Data.Rec.Memo);
		// @v11.1.12 strip(Data.Rec.Memo);
		getCtrlString(CTL_PERSON_MEMO, Data.SMemo); // @v11.1.12
		Data.SMemo.Strip(); // @v11.1.12
		getCtrlData(CTLSEL_PERSON_CATEGORY, &Data.Rec.CatID);
		// @v10.9.0 {
		if(getCtrlView(CTL_PERSON_GENDER))
			PersonCore::SetGender(Data.Rec, GetClusterData(CTL_PERSON_GENDER));
		// } @v10.9.0
		{
			ImageBrowseCtrlGroup::Rec rec;
			if(getGroupData(GRP_IBG, &rec))
				if(rec.Path.Len()) {
					THROW(Data.LinkFiles.Replace(0, rec.Path));
				}
				else
					Data.LinkFiles.Remove(0);
			SETFLAG(Data.Rec.Flags, PSNF_HASIMAGES, Data.LinkFiles.GetCount());
		}
		{
			PPIDArray new_kind_list;
			for(i = 0; i < Data.Kinds.getCount(); i++)
				if(!PrevKindList.lsearch(Data.Kinds.get(i)))
					new_kind_list.add(Data.Kinds.get(i));
			if(new_kind_list.getCount()) {
				int    is_private = 0;
				{
					PPObjPersonStatus ps_obj;
					PPPersonStatus ps_rec;
					if(Data.Rec.Status > 0 && ps_obj.Fetch(Data.Rec.Status, &ps_rec) > 0)
					   is_private = BIN(ps_rec.Flags & PSNSTF_PRIVATE);
				}
				PPIDArray reg_list;
				PPObjRegisterType obj_regt;
				for(i = 0; ok && i < new_kind_list.getCount(); i++) {
					THROW(GetExtRegListIds(new_kind_list.get(i), Data.Rec.Status, &reg_list));
					for(uint k = 0; k < reg_list.getCount(); k++) {
						const PPID reg_id = reg_list.get(k);
						PPRegisterType rt_rec;
						if(obj_regt.Fetch(reg_id, &rt_rec) > 0 && rt_rec.Flags & REGTF_INSERT) {
							if(!rt_rec.PersonKindID || Data.Kinds.lsearch(rt_rec.PersonKindID)) {
								const long xst = CheckXORFlags(rt_rec.Flags, REGTF_PRIVATE, REGTF_LEGAL);
								if(!xst || (is_private ? (xst & REGTF_PRIVATE) : (xst & REGTF_LEGAL))) {
									THROW_PP_S(Data.GetRegister(reg_id, 0) > 0, PPERR_REGISTERNEEDED, rt_rec.Name);
								}
							}
						}
					}
				}
			}
		}
		sel = CTL_PERSON_NAME;
		THROW(PsnObj.ValidatePacket(&Data, 0));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    EditDlvrLocList()
	{
		DIALOG_PROC_BODY(AddrListDialog, &Data);
	}
	void   RemoveKind()
	{
		int    r;
		uint   pos = 0;
		PPID   k;
		SmartListBox * box = static_cast<SmartListBox *>(getCtrlView(CTL_PERSON_KIND));
		if(box->getCurID(&k) && k) {
			THROW_PP(k != PPPRK_MAIN || PPMaster, PPERR_UNCHANGABLEPERSONKIND);
			THROW_PP(Data.Kinds.getCount() > 1, PPERR_LASTPERSONSKIND);
			if(Data.Kinds.lsearch(k, &pos)) {
				r = DBRPL_OK;
				if(Data.Rec.ID == 0 || (r = SendObjMessage(DBMSG_PERSONLOSEKIND, 0, Data.Rec.ID, Data.Kinds.at(pos))) == DBRPL_OK) {
					Data.Kinds.atFree(pos);
					box->setDef(createKindListDef());
					box->Draw_();
				}
				THROW(r != DBRPL_ERROR);
			}
		}
		CATCH
			PPError();
		ENDCATCH
		SetupCtrls();
	}
	void   SetupCtrls()
	{
		showCtrl(CTL_PERSON_CSHRRIGHTS,   IsCashier);
		showCtrl(CTL_PERSON_SELANALOGBUT, Data.Rec.ID == 0);
	}
	enum {
		dummyFirst = 1,
		brushHumanName,
		brushHumanNameFem,
		brushHumanNameMus,
	};
	ListBoxDef * createKindListDef();
	SString ImageFolder;
	PPID   CashiersPsnKindID;
	int    IsCashier;
	PPIDArray PrevKindList;   // Список видов, которым принадлежала персоанлия до редактирования //
	SPaintToolBox Ptb;
};
//
//
//
class ShortPersonDialog : public PersonDialogBase {
public:
	ShortPersonDialog(uint dlgId, PPID kindID, PPID scardSerID) : PersonDialogBase(dlgId),
		KindID(kindID), SCardSerID(scardSerID), Preserve_SCardSerID(scardSerID),
		SCardID(0), PhonePos(-1), CodeRegPos(-1), CodeRegTypeID(0), St(0)
	{
		SetupCalDate(CTLCAL_PERSON_DOB, CTL_PERSON_DOB);
		SetupCalDate(CTLCAL_PERSON_SCEXPIRY, CTL_PERSON_SCEXPIRY);
		showButton(cmCreateSCard, 0);
		enableCommand(cmCreateSCard, 0);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		uint   i;
		PPID   reg_type_id = 0;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		PhonePos = -1;
		CodeRegPos = -1;
		CodeRegTypeID = 0;
		SCardID = 0;
		if(Data.Rec.ID && !PsnObj.CheckRights(PPR_MOD)) {
			PPLoadText(PPTXT_NORIGHTS_PERSON_UPD, temp_buf);
			setStaticText(CTL_PERSON_ST_INFO, temp_buf);
		}
		setCtrlLong(CTL_PERSON_ID, Data.Rec.ID);
		setCtrlData(CTL_PERSON_NAME, Data.Rec.Name);
		SetupPPObjCombo(this, CTLSEL_PERSON_STATUS, PPOBJ_PRSNSTATUS, Data.Rec.Status, OLW_CANINSERT, 0);
		SetupPPObjCombo(this, CTLSEL_PERSON_CATEGORY, PPOBJ_PRSNCATEGORY, Data.Rec.CatID, OLW_CANINSERT, 0);
		// @v11.1.12 setCtrlData(CTL_PERSON_MEMO, Data.Rec.Memo);
		setCtrlString(CTL_PERSON_MEMO, Data.SMemo); // @v11.1.12
		// @v10.0.01 {
		if(InitPhone.NotEmpty()) {
			PPELink el;
			if(PPELinkArray::SetupNewPhoneEntry(InitPhone, el) > 0)
				Data.ELA.insert(&el);
		}
		// } @v10.0.01
		if(Data.ELA.GetSinglePhone(temp_buf, &i) > 0) {
			PhonePos = static_cast<int>(i);
			setCtrlString(CTL_PERSON_PHONE, temp_buf);
		}
		if(KindID) {
			if(!Data.Kinds.lsearch(KindID)) {
				if(Data.Kinds.getCount())
					KindID = Data.Kinds.get(0);
				else
					Data.Kinds.add(KindID);
			}
			PPObjPersonKind pk_obj;
			PPPersonKind pk_rec;
			if(pk_obj.Fetch(KindID, &pk_rec) > 0) {
				Data.Kinds.addUnique(KindID);
				setStaticText(CTL_PERSON_ST_KINDNAME, pk_rec.Name);
				{
					PPObjRegisterType rt_obj;
					PPRegisterType rt_rec;
					if(pk_rec.CodeRegTypeID && rt_obj.Fetch(pk_rec.CodeRegTypeID, &rt_rec) > 0) {
						CodeRegTypeID = pk_rec.CodeRegTypeID;
						setLabelText(CTL_PERSON_SRCHCODE, rt_rec.Name);
						uint   p = 0;
						if(Data.Regs.GetRegister(pk_rec.CodeRegTypeID, &p, 0) > 0) {
							temp_buf = Data.Regs.at(p-1).Num;
							CodeRegPos = static_cast<int>(p-1);
						}
						else
							temp_buf.Z();
						setCtrlString(CTL_PERSON_SRCHCODE, temp_buf);
					}
				}
			}
		}
		SetupGender();
		SetupDOB();
		SetClusterData(CTL_PERSON_GENDER, PersonCore::GetGender(Data.Rec)); // @v10.9.0
		if(SCardSerID) {
			SetupPPObjCombo(this, CTLSEL_PERSON_SCARDSER, PPOBJ_SCARDSERIES, (SCardSerID > 0) ? SCardSerID : 0, OLW_CANINSERT, 0);
			AddClusterAssoc(CTL_PERSON_SCARDAUTO, 0, stSCardAutoCreate);
			SetClusterData(CTL_PERSON_SCARDAUTO, St);
			//disableCtrls(!(St & stSCardAutoCreate), CTL_PERSON_SCEXPIRY, CTLCAL_PERSON_SCEXPIRY, CTL_PERSON_SCDIS, CTL_PERSON_SCTIME, CTL_PERSON_SCAG, 0);
		}
		SetupSCardSeries(0, 0);
		disableCtrl(CTL_PERSON_SRCHCODE, !CodeRegTypeID);
		enableCommand(cmSelectAnalog, !Data.Rec.ID);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlData(CTL_PERSON_NAME, Data.Rec.Name);
		getCtrlData(CTLSEL_PERSON_STATUS, &Data.Rec.Status);
		getCtrlData(CTLSEL_PERSON_CATEGORY, &Data.Rec.CatID);
		// @v11.1.12 getCtrlData(CTL_PERSON_MEMO, Data.Rec.Memo);
		getCtrlString(CTL_PERSON_MEMO, Data.SMemo); // @v11.1.12
		getCtrlString(CTL_PERSON_PHONE, temp_buf);
		if(PhonePos >= 0) {
			if(temp_buf.NotEmptyS()) {
				PPELink & r_el = Data.ELA.at(PhonePos);
				temp_buf.CopyTo(r_el.Addr, sizeof(r_el.Addr));
			}
			else
				Data.ELA.atFree(PhonePos);
		}
		else if(temp_buf.NotEmptyS())
			Data.ELA.AddItem(ELNKRT_PHONE, temp_buf);
		if(CodeRegTypeID) {
			getCtrlString(CTL_PERSON_SRCHCODE, temp_buf);
			if(CodeRegPos >= 0) {
				if(temp_buf.NotEmptyS()) {
					RegisterTbl::Rec & r_reg_rec = Data.Regs.at(CodeRegPos);
					temp_buf.CopyTo(r_reg_rec.Num, sizeof(r_reg_rec.Num));
				}
				else {
					Data.Regs.atFree(CodeRegPos);
					CodeRegPos = -1;
				}
			}
			else if(temp_buf.NotEmptyS()) {
				Data.AddRegister(CodeRegTypeID, temp_buf, 1);
			}
		}
		THROW(AcceptSCard(&sel));
		GetDOB();
		// @v10.9.0 {
		if(getCtrlView(CTL_PERSON_GENDER))
			PersonCore::SetGender(Data.Rec, GetClusterData(CTL_PERSON_GENDER));
		// } @v10.9.0
		sel = CTL_PERSON_NAME;
		THROW(PsnObj.ValidatePacket(&Data, 0));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    AcceptSCard(uint * pSel);
	void   SetupSCardRestInfo();
	void   SetupSCard();
	void   ShowSCardCtrls(int show);
	int    SetupSCardSeries(int fromCtrl, int dontSeekCard);

	PPID   KindID;
	PPID   CodeRegTypeID;
	PPID   SCardSerID;
	PPID   Preserve_SCardSerID; // Сохраненная серия карт для восстановления после
		// последовательности при вводе номера карты: найдена-карта -> не-найдена-карта
		// В базе данных могут быть карты с номерами разных длин. При наборе номера
		// программа может неверно идентифицировать номер карты до завершения ввода.
		// Если при очередном набранном символе карта уже не находится, то серия возвращается
		// к значению Preserve_SCardSerID
	PPID   SCardID;
	int    PhonePos;
	int    CodeRegPos;
	enum {
		stSCardAutoCreate = 0x0001,
		stSCardOnInput    = 0x0002 // Устанавливается при поиске карты по мере ввода номера пользователем
	};
	long   St;
	PPObjSCard ScObj;
	DateAddDialogParam ScExpiryPeriodParam;
};

IMPL_HANDLE_EVENT(ShortPersonDialog)
{
	SString temp_buf;
	PersonDialogBase::handleEvent(event);
	if(event.isCmd(cmSelectAnalog)) {
		if(!Data.Rec.ID) {
			PPID   analog_id = 0;
			getCtrlString(CTL_PERSON_NAME, temp_buf);
			if(PsnObj.SelectAnalog(temp_buf, &analog_id) > 0) {
				PPPersonPacket sel_pack;
				if(PsnObj.GetPacket(analog_id, &sel_pack, 0) > 0) {
					SCardSerID = getCtrlLong(CTLSEL_PERSON_SCARDSER);
					SCardID = 0;
					DupID = sel_pack.Rec.ID;
					setDTS(&sel_pack);
					SetFocus(H());
				}
			}
		}
	}
	else if(event.isCmd(cmPersonAddr)) {
		PPObjLocation loc_obj;
		LocationTbl::Rec * p_loc_rec = &Data.Loc;
		p_loc_rec->Type = LOCTYP_ADDRESS;
		loc_obj.EditDialog(LOCTYP_ADDRESS, p_loc_rec);
	}
	else if(event.isCmd(cmFullPersonDialog)) {
		PPPersonPacket fdata;
		if(getDTS(&fdata)) {
			PersonDialog * fdlg = new PersonDialog(DLG_PERSON);
			if(CheckDialogPtrErr(&fdlg)) {
				int    r = 1;
				int    valid_data = 0;
				fdlg->setDTS(&fdata);
				fdlg->ToCascade();
				while(!valid_data && (r = ExecView(fdlg)) == cmOK) {
					if((valid_data = fdlg->getDTS(&fdata)) != 0) {
						setDTS(&fdata);
					}
				}
				ZDELETE(fdlg);
			}
		}
	}
	else if(event.isCmd(cmFullSCardDialog)) {
		if(AcceptSCard(0) > 0) {
			const PPSCardPacket * p_sc_pack = Data.GetSCard();
			if(p_sc_pack) {
				PPSCardPacket sc_pack = *p_sc_pack;
				if(ScObj.EditDialog(&sc_pack, PPObjSCard::edfDisableCode) > 0) {
					setStaticText(CTL_PERSON_ST_SCARDINFO, temp_buf.Z());
					setCtrlString(CTL_PERSON_SCARD, temp_buf = sc_pack.Rec.Code);
					setCtrlLong(CTLSEL_PERSON_SCARDSER, SCardSerID = sc_pack.Rec.SeriesID);
					setCtrlDate(CTL_PERSON_SCEXPIRY, sc_pack.Rec.Expiry);
					setCtrlReal(CTL_PERSON_SCDIS, fdiv100i(sc_pack.Rec.PDis));
					SetTimeRangeInput(this, CTL_PERSON_SCTIME, TIMF_HM, &sc_pack.Rec.UsageTmStart, &sc_pack.Rec.UsageTmEnd);
					if(sc_pack.Rec.PersonID && sc_pack.Rec.PersonID != Data.Rec.ID) {
						PPLoadText(PPTXT_SCARDOTHEROWNER, temp_buf);
						setStaticText(CTL_PERSON_ST_SCARDINFO, temp_buf);
					}
					else {
						SCardID = sc_pack.Rec.ID;
						Data.SetSCard(&sc_pack, 0);
						SetupSCardRestInfo();
					}
				}
			}
		}
	}
	else if(event.isCbSelected(CTLSEL_PERSON_SCARDSER)) {
		SetupSCardSeries(1, 0);
		if(!(St & stSCardOnInput))
			Preserve_SCardSerID = SCardSerID;
	}
	else if(event.isCmd(cmCreateSCard)) {
		if(SCardID) {
			SCardID = 0;
			setCtrlString(CTL_PERSON_SCARD, temp_buf.Z());
			SetupSCardSeries(1, 1);
			disableCtrl(CTL_PERSON_SCARD, 0);
			showButton(cmCreateSCard, 0);
			enableCommand(cmCreateSCard, 0);
		}
	}
	else if(event.isClusterClk(CTL_PERSON_SCARDAUTO)) {
		int    sca = BIN(getCtrlUInt16(CTL_PERSON_SCARDAUTO));
		disableCtrl(CTL_PERSON_SCARD, sca);
		setCtrlString(CTL_PERSON_SCARD, temp_buf.Z());
	}
	else if(TVBROADCAST && TVCMD == cmChangedFocus && TVINFOVIEW->TestId(CTL_PERSON_NAME)) {
		if(!Data.Rec.ID) {
			getCtrlString(CTL_PERSON_NAME, temp_buf);
			if(temp_buf.Len() > 0 && Name_.CmpNC(temp_buf) != 0) {
				Name_ = temp_buf;
				PPID   dup_id = 0;
				PPPersonPacket sel_pack;
				if(PsnObj.CheckDuplicateName(temp_buf, &dup_id) == 2 && PsnObj.GetPacket(dup_id, &sel_pack, 0) > 0) {
					SCardSerID = getCtrlLong(CTLSEL_PERSON_SCARDSER);
					SCardID = 0;
					DupID = sel_pack.Rec.ID;
					setDTS(&sel_pack);
					SetFocus(H());
				}
			}
		}
	}
	else if(event.isCmd(cmInputUpdated)) {
		if(event.isCtlEvent(CTL_PERSON_SCARD)) {
			St |= stSCardOnInput;
			SetupSCard();
			St &= ~stSCardOnInput;
		}
		else
			return;
	}
	else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_PERSON_SCEXPIRY)) {
		if(DateAddDialog(&ScExpiryPeriodParam) > 0 && checkdate(ScExpiryPeriodParam.ResultDate)) {
			setCtrlDate(CTL_PERSON_SCEXPIRY, ScExpiryPeriodParam.ResultDate);
		}
	}
	else
		return;
	clearEvent(event);
}

int ShortPersonDialog::AcceptSCard(uint * pSel)
{
	int    ok = -1;
	uint   sel = 0;
	SString temp_buf;
	PPSCardPacket preserve_sc_pack;
	{
		const PPSCardPacket * p_sc_pack = Data.GetSCard();
		RVALUEPTR(preserve_sc_pack, p_sc_pack);
	}
	Data.SetSCard(0, 0);
	SCardSerID = getCtrlLong(CTLSEL_PERSON_SCARDSER);
	SETIFZ(SCardSerID, preserve_sc_pack.Rec.SeriesID);
	if(SCardSerID) {
		PPSCardPacket sc_pack;
		{
			#define CPYFLD(f) sc_pack.Rec.f = preserve_sc_pack.Rec.f
			CPYFLD(MaxCredit);
			CPYFLD(PeriodTerm);
			CPYFLD(PeriodCount);
			CPYFLD(Flags);
			#undef CPYFLD
		}
		GetClusterData(CTL_PERSON_SCARDAUTO, &St);
		getCtrlString(CTL_PERSON_SCARD, temp_buf);
		if(temp_buf.NotEmpty()) {
			int r = 0;
			THROW(r = ScObj.SearchCode(0, temp_buf, &sc_pack.Rec));
			if(r < 0) {
				sc_pack.Rec.ID = 0;
				sc_pack.Rec.SeriesID = SCardSerID;
				temp_buf.CopyTo(sc_pack.Rec.Code, sizeof(sc_pack.Rec.Code));
			}
			sc_pack.Rec.Expiry = getCtrlDate(CTL_PERSON_SCEXPIRY);
			// @v10.2.9 sc_pack.Rec.PDis = (long)(getCtrlReal(CTL_PERSON_SCDIS) * 100L);
			sc_pack.Rec.PDis = fmul100i(getCtrlReal(CTL_PERSON_SCDIS)); // @v10.2.9
			THROW(GetTimeRangeInput(this, sel = CTL_PERSON_SCTIME, TIMF_HM, &sc_pack.Rec.UsageTmStart, &sc_pack.Rec.UsageTmEnd));
			getCtrlData(CTLSEL_PERSON_SCAG, &sc_pack.Rec.AutoGoodsID);
			Data.SetSCard(&sc_pack, 0);
			ok = 1;
		}
		else if(St & stSCardAutoCreate) {
			sc_pack.Rec.SeriesID = SCardSerID;
			sc_pack.Rec.Expiry = getCtrlDate(CTL_PERSON_SCEXPIRY);
			// @v10.2.9 sc_pack.Rec.PDis = (long)(getCtrlReal(CTL_PERSON_SCDIS) * 100L);
			sc_pack.Rec.PDis = fmul100i(getCtrlReal(CTL_PERSON_SCDIS)); // @v10.2.9
			GetTimeRangeInput(this, CTL_PERSON_SCTIME, TIMF_HM, &sc_pack.Rec.UsageTmStart, &sc_pack.Rec.UsageTmEnd);
			THROW(GetTimeRangeInput(this, sel = CTL_PERSON_SCTIME, TIMF_HM, &sc_pack.Rec.UsageTmStart, &sc_pack.Rec.UsageTmEnd));
			getCtrlData(CTLSEL_PERSON_SCAG, &sc_pack.Rec.AutoGoodsID);
			Data.SetSCard(&sc_pack, 1);
			ok = 1;
		}
		else
			SetupSCard();
	}
	else
		SCardID = 0;
	CATCHZOK
	ASSIGN_PTR(pSel, sel);
	return ok;
}

void ShortPersonDialog::SetupSCardRestInfo()
{
	if(ScObj.IsCreditSeries(SCardSerID)) {
		double rest = 0.0;
		SString temp_buf;
		ScObj.P_Tbl->GetRest(SCardID, MAXDATE, &rest);
		PPLoadString("rest", temp_buf);
		temp_buf.CatDiv(':', 2).Cat(rest, SFMT_MONEY);
		setStaticText(CTL_PERSON_ST_SCARDINFO, temp_buf);
	}
}

void ShortPersonDialog::SetupSCard()
{
	SString temp_buf;
	setStaticText(CTL_PERSON_ST_SCARDINFO, temp_buf.Z());
	getCtrlString(CTL_PERSON_SCARD, temp_buf);
	SCardTbl::Rec sc_rec;
	if(temp_buf.NotEmptyS() && ScObj.SearchCode(0, temp_buf, &sc_rec) > 0) {
		setCtrlLong(CTLSEL_PERSON_SCARDSER, SCardSerID = sc_rec.SeriesID);
		if(sc_rec.PersonID && sc_rec.PersonID != Data.Rec.ID) {
			PPLoadText(PPTXT_SCARDOTHEROWNER, temp_buf);
			setStaticText(CTL_PERSON_ST_SCARDINFO, temp_buf);
		}
		else {
			SCardID = sc_rec.ID;
			SetupSCardRestInfo();
		}
	}
	else {
		SCardID = 0;
		setCtrlLong(CTLSEL_PERSON_SCARDSER, SCardSerID = Preserve_SCardSerID);
	}
}

void ShortPersonDialog::ShowSCardCtrls(int show)
{
	// @v10.9.0 {
	const ushort ctl_list[] = { CTL_PERSON_SCFRAME, CTL_PERSON_SCARD, CTL_PERSON_SCARDAUTO, CTL_PERSON_SCARDSER, CTLSEL_PERSON_SCARDSER,
		CTL_PERSON_ST_SCARDINFO, CTL_PERSON_SCEXPIRY, CTLCAL_PERSON_SCEXPIRY, CTL_PERSON_SCDIS, CTL_PERSON_SCAG, CTLSEL_PERSON_SCAG, CTL_PERSON_SCTIME };
	for(uint i = 0; i < SIZEOFARRAY(ctl_list); i++)
		showCtrl(ctl_list[i], show);
	// } @v10.9.0
	/* @v10.9.0 showCtrl(CTL_PERSON_SCFRAME, show);
	showCtrl(CTL_PERSON_SCARD, show);
	showCtrl(CTL_PERSON_SCARDAUTO, show);
	showCtrl(CTL_PERSON_SCARDSER, show);
	showCtrl(CTLSEL_PERSON_SCARDSER, show);
	showCtrl(CTL_PERSON_ST_SCARDINFO, show);
	showCtrl(CTL_PERSON_SCEXPIRY, show);
	showCtrl(CTLCAL_PERSON_SCEXPIRY, show);
	showCtrl(CTL_PERSON_SCDIS, show);
	showCtrl(CTL_PERSON_SCAG, show);
	showCtrl(CTLSEL_PERSON_SCAG, show);
	showCtrl(CTL_PERSON_SCTIME, show);*/
	showButton(cmFullSCardDialog, show);
	enableCommand(cmFullSCardDialog, show);
}

int ShortPersonDialog::SetupSCardSeries(int fromCtrl, int dontSeekCard)
{
	int    ok = 1;
	int    enable_auto_create = 0;
	PPID   scs_id = 0;
	if(fromCtrl) {
		scs_id = getCtrlLong(CTLSEL_PERSON_SCARDSER);
		SETIFZ(scs_id, -1);
		SCardSerID = scs_id;
	}
	else
		scs_id = SCardSerID;
	if(scs_id) {
		SCardTbl::Rec sc_rec;
		MEMSZERO(sc_rec);
		PPID   goods_grp_id = 0;
		PPObjSCardSeries scs_obj;
		PPSCardSerPacket scs_pack;
		if(scs_obj.GetPacket(scs_id, &scs_pack) > 0) {
			goods_grp_id = scs_pack.Rec.CrdGoodsGrpID;
			if(!dontSeekCard) {
				if(Data.Rec.ID) {
					PPIDArray sc_list;
					ScObj.P_Tbl->GetListByPerson(Data.Rec.ID, scs_id, &sc_list);
					for(uint i = 0; i < sc_list.getCount(); i++) {
						PPID sc_id = sc_list.get(i);
						if(ScObj.Search(sc_id, &sc_rec) > 0) {
							SCardID = sc_id;
							setCtrlData(CTL_PERSON_SCARD, sc_rec.Code);
							disableCtrl(CTL_PERSON_SCARD, 1);
							showButton(cmCreateSCard, 1);
							enableCommand(cmCreateSCard, 1);
							break;
						}
						else
							MEMSZERO(sc_rec);
					}
				}
			}
			if(goods_grp_id) {
				PPID   auto_goods_id = 0;
				if(SCardID)
					auto_goods_id = sc_rec.AutoGoodsID;
				else if(goods_grp_id) {
					PPIDArray temp_goods_list;
					GoodsIterator::GetListByGroup(goods_grp_id, &temp_goods_list);
					if(temp_goods_list.getCount() == 1)
						auto_goods_id = temp_goods_list.get(0);
				}
				SetupPPObjCombo(this, CTLSEL_PERSON_SCAG, PPOBJ_GOODS, auto_goods_id, 0, reinterpret_cast<void *>(goods_grp_id));
			}
			else
				setCtrlLong(CTLSEL_PERSON_SCAG, 0);
			if(!SCardID) {
				disableCtrl(CTL_PERSON_SCARD, 0);
				showButton(cmCreateSCard, 0);
				enableCommand(cmCreateSCard, 0);
				enable_auto_create = 1;
				if(scs_pack.Rec.Flags & SCRDSF_NEWSCINHF) {
					sc_rec.Flags |= SCRDF_INHERITED;
					ScObj.SetInheritance(&scs_pack, &sc_rec);
				}
				else if(Data.Rec.ID) {
					SCardTbl::Rec dbc_rec;
					if(ScObj.FindDiscountBorrowingCard(Data.Rec.ID, &dbc_rec) > 0)
						sc_rec.PDis = dbc_rec.PDis;
				}
			}
		}
		ShowSCardCtrls(1);
		disableCtrl(CTL_PERSON_SCARDAUTO, !enable_auto_create);
		setCtrlDate(CTL_PERSON_SCEXPIRY, sc_rec.Expiry);
		setCtrlReal(CTL_PERSON_SCDIS, fdiv100i(sc_rec.PDis));
		SetTimeRangeInput(this, CTL_PERSON_SCTIME, TIMF_HM, &sc_rec.UsageTmStart, &sc_rec.UsageTmEnd);
	}
	else
		ShowSCardCtrls(0);
	return ok;
}
//
//
//
class MainOrg2Dialog : public TDialog {
public:
	MainOrg2Dialog(int dlgID, PPPersonPacket *, PPObjPerson *pObj);
	int    setDTS();
	int    getDTS();
	int    getPsnRegs(char *corrAcc, char *bicAcc, char *innAcc, long bnkID, size_t cAcc_sz, size_t bAcc_sz, size_t iAcc_sz);
private:
	DECL_HANDLE_EVENT;
	PPObjPerson    * P_Obj;
	PPPersonPacket * P_Pack;
	int    PrefPos;
};

PPObjPerson::EditBlock::EditBlock() : InitKindID(0), InitStatusID(0), ShortDialog(0), SCardSeriesID(0), RetSCardID(0), UpdFlags(0)
{
}

void PPObjPerson::InitEditBlock(PPID kindID, EditBlock & rBlk)
{
	rBlk.InitKindID = kindID;
	rBlk.ShortDialog = 0;
	rBlk.SCardSeriesID = 0;
	rBlk.Name.Z();
	rBlk.InitPhone.Z();
	rBlk.RetSCardID = 0;
	rBlk.UpdFlags = 0;
	if(kindID) {
		PPObjPersonKind pk_obj;
		PPPersonKind2 pk_rec;
		if(pk_obj.Fetch(kindID, &pk_rec) > 0) {
			rBlk.InitStatusID = pk_rec.DefStatusID;
			if(pk_rec.Flags & PPPersonKind::fUseShortPersonDialog)
				rBlk.ShortDialog = 1;
		}
		else
			rBlk.InitKindID = 0;
	}
	//
	// @v11.3.1 Несколько более умное чем раньше автоматическое определение юридического статуста новой персоналии
	//
	if(!rBlk.InitStatusID && rBlk.InitKindID) {
		if(oneof3(rBlk.InitKindID, PPPRK_EMPL, PPPRK_AGENT, PPPRK_CAPTAIN))
			rBlk.InitStatusID = PPPRS_PRIVATE;
		else
			rBlk.InitStatusID = PPPRS_LEGAL;
	}
}

int PPObjPerson::CheckDuplicateName(const char * pName, PPID * pID)
{
	int    ok = -1;
	SString name(pName);
	SString temp_buf;
	if(name.Len()) {
		PPObjPersonKind pk_obj;
		PPPersonKind pk_rec;
		StrAssocArray items_list;
		PersonTbl::Key1 k1;
		name.CopyTo(k1.Name, sizeof(k1.Name));
		PPIDArray id_list, kind_list;
		if(P_Tbl->search(1, &k1, spEq))
			do {
				id_list.addUnique(P_Tbl->data.ID);
			} while(P_Tbl->search(1, &k1, spNext) && name.CmpNC(P_Tbl->data.Name) == 0);
		for(uint i = 0; i < id_list.getCount(); i++) {
			const PPID person_id = id_list.get(i);
			PersonTbl::Rec person_rec;
			if(Fetch(person_id, &person_rec) > 0) {
				(temp_buf = person_rec.Name).CatDiv(':', 2);
				kind_list.clear();
				P_Tbl->GetKindList(person_id, &kind_list);
				for(uint j = 0; j < kind_list.getCount(); j++) {
					if(j > 0)
						temp_buf.CatDiv(',', 2);
					const PPID kind_id = kind_list.get(j);
					if(pk_obj.Fetch(kind_id, &pk_rec) > 0)
						temp_buf.Cat(pk_rec.Name);
					else
						ideqvalstr(kind_id, temp_buf);
				}
				items_list.Add(person_id, temp_buf);
			}
		}
		if(items_list.getCount() > 0) {
			if(ListBoxSelDialog(DLG_DUPNAMES, &items_list, pID, 0) > 0)
				ok = 2;
			else
				ok = 1;
		}
	}
	return ok;
}

int PPObjPerson::SelectAnalog(const char * pName, PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	PsnSelAnalogDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new PsnSelAnalogDialog(this)))) {
		dlg->setSrchString(pName);
		if(ExecView(dlg) == cmOK) {
			dlg->getResult(&id);
			if(id > 0)
				ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPObjPerson::Edit_(PPID * pID, EditBlock & rBlk)
{
	int    ok = 1;
	int    valid_data = 0;
	int    r = cmCancel;
	int    is_new = 0;
	PPID   short_dlg_kind_id = 0;
	uint   dlg_id = 0;
	TDialog * dlg = 0;
	PPPersonPacket info;
	THROW(EditPrereq(pID, 0, &is_new));
	if(is_new) {
		PPID   kind_id = rBlk.InitKindID;
		if(kind_id <= 0) {
			kind_id = SelectPersonKind();
			if(kind_id > 0) {
				// @v10.5.11 InitEditBlock(kind_id, rBlk);
				// @v10.5.11 {
				EditBlock temp_blk;
				InitEditBlock(kind_id, temp_blk);
				rBlk.InitKindID = temp_blk.InitKindID;
				rBlk.InitStatusID = temp_blk.InitStatusID;
				rBlk.ShortDialog = temp_blk.ShortDialog;
				// } @v10.5.11
				kind_id = rBlk.InitKindID;
			}
			else
				return cmCancel;
		}
		info.Kinds.add(kind_id);
		rBlk.Name.CopyTo(info.Rec.Name, sizeof(info.Rec.Name));
		info.Rec.Status = rBlk.InitStatusID;
		if(rBlk.ShortDialog) {
			dlg_id = DLG_PERSON_S1;
			short_dlg_kind_id = kind_id;
		}
	}
	else {
		PPObjPersonKind pk_obj;
		PPPersonKind pk_rec;
		THROW(GetPacket(*pID, &info, 0) > 0);
		for(uint i = 0; i < info.Kinds.getCount(); i++) {
			if(pk_obj.Fetch(info.Kinds.get(i), &pk_rec) > 0 && pk_rec.Flags & PPPersonKind::fUseShortPersonDialog) {
				dlg_id = DLG_PERSON_S1;
				short_dlg_kind_id = info.Kinds.get(i);
				break;
			}
		}
	}
	info.UpdFlags = rBlk.UpdFlags;
	SETIFZ(dlg_id, ((LConfig.Flags & CFGFLG_STAFFMGMT) ? DLG_PERSONEXT : DLG_PERSON));
	if(short_dlg_kind_id) {
		THROW(CheckDialogPtr(&(dlg = new ShortPersonDialog(dlg_id, short_dlg_kind_id, rBlk.SCardSeriesID))));
		{
			ShortPersonDialog * p_dlg = static_cast<ShortPersonDialog *>(dlg);
			p_dlg->enableCommand(cmFullPersonDialog, 1);
			if(!is_new && !CheckRights(PPR_MOD))
				p_dlg->enableCommand(cmOK, 0);
			// @v10.0.01 {
			if(rBlk.InitPhone.NotEmpty()) {
				p_dlg->SetupPhoneOnInit(rBlk.InitPhone);
			}
			// } @v10.0.01
			p_dlg->setDTS(&info);
			while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
				const  PPID dup_id = p_dlg->GetDupID();
				const  int  rt_mod = CheckRights(PPR_MOD);
				if((valid_data = p_dlg->getDTS(&info)) != 0) {
					PPID   psn_id = info.Rec.ID;
					THROW((is_new && !dup_id) || (rt_mod || info.GetSCard()));
					if(!PutPacket(&psn_id, &info, 1))
						valid_data = PPErrorZ();
					else {
						rBlk.RetSCardID = info.GetSCard() ? info.GetSCard()->Rec.ID : 0;
						ASSIGN_PTR(pID, psn_id);
					}
				}
			}
		}
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new PersonDialog(dlg_id))));
		{
			PersonDialog * p_dlg = static_cast<PersonDialog *>(dlg);
			// @v10.0.01 {
			if(rBlk.InitPhone.NotEmpty())
				p_dlg->SetupPhoneOnInit(rBlk.InitPhone);
			// } @v10.0.01
			p_dlg->setDTS(&info);
			if(!is_new && !CheckRights(PPR_MOD))
				p_dlg->enableCommand(cmOK, 0);
			while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
				THROW(is_new || CheckRights(PPR_MOD));
				const PPID dup_id = p_dlg->GetDupID();
				if(dup_id) {
					valid_data = 1;
					ASSIGN_PTR(pID, dup_id);
				}
				else if((valid_data = p_dlg->getDTS(&info)) != 0) {
					if(!PutPacket(pID, &info, 1))
						valid_data = PPErrorZ();
				}
			}
		}
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
	return ok ? r : 0;
}

int PPObjPerson::Edit(PPID * pID, void * extraPtr)
{
	EditBlock eb;
	InitEditBlock(reinterpret_cast<PPID>(extraPtr), eb);
	return Edit_(pID, eb);
}

int PPObjPerson::ViewVersion(PPID histID)
{
	int    ok = -1;
	PersonDialog * dlg = 0;
	if(histID) {
		SBuffer buf;
		PPPersonPacket pack;
		ObjVersioningCore * p_ovc = PPRef->P_OvT;
		if(p_ovc && p_ovc->InitSerializeContext(1)) {
			SSerializeContext & r_sctx = p_ovc->GetSCtx();
			PPObjID oid;
			long   vv = 0;
			THROW(p_ovc->Search(histID, &oid, &vv, &buf) > 0);
			THROW(SerializePacket(-1, &pack, buf, &r_sctx));
			{
				THROW(CheckDialogPtr(&(dlg = new PersonDialog(DLG_PERSON))));
				dlg->setDTS(&pack);
				dlg->enableCommand(cmOK, 0);
				ExecView(dlg);
			}
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

ListBoxDef * PersonDialog::createKindListDef()
{
	StdListBoxDef * def = 0;
	ReferenceTbl::Rec rec;
	SArray * ary = new SArray(sizeof(rec.ObjName) + sizeof(PPID));
	int    r, is_cashier = 0;
	for(int i = Data.Kinds.getCount()-1; i >= 0; i--) {
		if((r = SearchObject(PPOBJ_PERSONKIND, Data.Kinds.at(i), &rec)) > 0) {
			ary->atInsert(0, &rec.ObjID);
			if(rec.ObjID == CashiersPsnKindID)
				is_cashier = 1;
		}
		else if(r < 0)
			Data.Kinds.atFree(i);
	}
	if(IsCashier && !is_cashier)
		Data.CshrInfo.Flags |= CIF_MODIFIED;
	IsCashier = is_cashier;
	SETFLAG(Data.CshrInfo.Flags, CIF_CASHIER, IsCashier);
	def = new StdListBoxDef(ary, lbtDisposeData, MKSTYPE(S_ZSTRING, sizeof(rec.ObjName)));
	return def;
}
//
//
//
PsnSelAnalogDialog::PsnSelAnalogDialog(PPObjPerson * pPsnObj) : TDialog(DLG_PSNSELANALOG), P_PsnObj(pPsnObj), Selection(0)
{
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_PSNSELANALOG_LIST));
	if(!SetupStrListBox(p_list))
		PPError();
	setupList();
}

void PsnSelAnalogDialog::setSrchString(const char * pStr)
{
	setCtrlData(CTL_PSNSELANALOG_SRCH, const_cast<char *>(pStr));
	selectCtrl(CTL_PSNSELANALOG_SRCH);
	if(pStr)
		setupList();
}

void PsnSelAnalogDialog::getResult(PPID * pID)
{
	ASSIGN_PTR(pID, Selection);
}

void PsnSelAnalogDialog::setupList()
{
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_PSNSELANALOG_LIST));
	if(p_list) {
		PPObjPerson::SrchAnalogPattern sap;
		getCtrlString(CTL_PSNSELANALOG_SRCH, sap.NamePattern);
		p_list->freeAll();
		if(sap.NamePattern.NotEmptyS()) {
			PPIDArray id_list;
			PersonTbl::Rec rec;
			P_PsnObj->GetListByPattern(&sap, &id_list);
			for(uint i = 0; i < id_list.getCount(); i++) {
				const PPID psn_id = id_list.get(i);
				if(P_PsnObj->Search(psn_id, &rec) > 0)
					p_list->addItem(psn_id, rec.Name);
			}
		}
		p_list->Draw_();
	}
}

IMPL_HANDLE_EVENT(PsnSelAnalogDialog)
{
	if(TVCOMMAND && oneof2(TVCMD, cmOK, cmLBDblClk)) {
		PPID   id = getCtrlLong(CTL_PSNSELANALOG_LIST);
		if(id) {
			Selection = id;
			if(IsInState(sfModal)) {
				endModal(cmOK);
				return; // После endModal не следует обращаться к this
			}
		}
		else
			clearEvent(event);
	}
	TDialog::handleEvent(event);
	if(TVBROADCAST && TVCMD == cmReceivedFocus) {
		if(event.isCtlEvent(CTL_PSNSELANALOG_SRCH)) {
			SetDefaultButton(STDCTL_MORE, 1);
			SetDefaultButton(STDCTL_OKBUTTON, 0);
		}
		else if(event.isCtlEvent(CTL_PSNSELANALOG_LIST)) {
			SetDefaultButton(STDCTL_MORE, 0);
			SetDefaultButton(STDCTL_OKBUTTON, 1);
		}
		else
			return;
	}
	else if(event.isCmd(cmSearch))
		setupList();
	else
		return;
	clearEvent(event);
}
//
//
//
enum AnalyzePersonNameResultFlags {
	apnrfFirstName        = 0x0001,
	apnrfPatronymic       = 0x0002,
	apnrfLastName         = 0x0004,
	apnrfGenusRerum       = 0x0008, // Форма собственности "formeo"
	apnrfNegotiumTaxonomyCategory = 0x0010, // Категория предприятия "entkind"
};

static long AnalyzePersonName(const SString & rName, long * pGenderMusComponents, long * pGenderFemComponents)
{
	long   nam_components = 0; // apnrfXXX
	long   gender_mus_components = 0;
	long   gender_fem_components = 0;
	SString name(rName);
	if(name.NotEmptyS()) {
		const SrSyntaxRuleSet * p_rs = DS.GetSrSyntaxRuleSet();
		if(p_rs) {
			SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();
			if(p_srdb) {
				TSCollection <SrSyntaxRuleSet::Result> result_list;
				SrCPropList cpl;
				//
				CONCEPTID parent_fname_cid = 0;
				CONCEPTID parent_sname_cid = 0;
				CONCEPTID parent_pname_cid = 0;
				CONCEPTID gender_cid = 0;
				CONCEPTID gender_fem_cid = 0;
				CONCEPTID gender_mas_cid = 0;
				const CONCEPTID prop_subclass = p_srdb->GetReservedConcept(p_srdb->rcSubclass);
				const CONCEPTID prop_instance = p_srdb->GetReservedConcept(p_srdb->rcInstance);
				if(prop_subclass && prop_instance) {
					p_srdb->SearchConcept("hum_fname", &parent_fname_cid);
					p_srdb->SearchConcept("hum_sname", &parent_sname_cid);
					p_srdb->SearchConcept("hum_pname", &parent_pname_cid);
					p_srdb->SearchConcept("gender", &gender_cid);
					p_srdb->SearchConcept("gender_mas", &gender_mas_cid);
					p_srdb->SearchConcept("gender_fem", &gender_fem_cid);
				}
				const int test_gender_concept = BIN(gender_cid && gender_fem_cid && gender_mas_cid);
				//
				SrSyntaxRuleTokenizer t;
				uint   idx_first = 0;
				uint   idx_count = 0;
				STokenizer::Item item_;
				name.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
				t.ProcessString("", name, &idx_first, &idx_count);
				if(p_rs->ProcessText(*p_srdb, t, idx_first, idx_count, result_list) > 0) {
					SString rule_name;
					SString temp_buf;
					for(uint residx = 0; residx < result_list.getCount(); residx++) {
						const SrSyntaxRuleSet::Result * p_result = result_list.at(residx);
						if(p_result) {
							const SrSyntaxRuleSet::Rule * p_rule = p_rs->GetRule(p_result->RuleIdx);
							p_rs->GetRuleName(p_result->RuleIdx, rule_name);
							if(rule_name == "formeo") {
								nam_components |= apnrfGenusRerum;
								break;
							}
							else if(rule_name == "entkind") {
								nam_components |= apnrfNegotiumTaxonomyCategory;
								break;
							}
							else if(rule_name == "humname") {
								SrCProp cp, cp_gender;
								for(uint j = 0; j < p_result->MatchList.getCount(); j++) {
									const SrSyntaxRuleSet::MatchEntry & r_me = p_result->MatchList.at(j);
									if(r_me.P_Rule && r_me.StackP < r_me.P_Rule->ES.getCount()) {
										const SrSyntaxRuleSet::ExprItem * p_ei = static_cast<const SrSyntaxRuleSet::ExprItem *>(r_me.P_Rule->ES.at(r_me.StackP));
										switch(p_ei ? p_ei->K : 0) {
											case SrSyntaxRuleSet::kLiteral:
											case SrSyntaxRuleSet::kConcept:
											case SrSyntaxRuleSet::kMorph:
												break;
											case SrSyntaxRuleSet::kRule:
												p_rs->GetS(p_ei->SymbP, temp_buf);
												temp_buf.Strip();
												if(temp_buf == "humname_s") {
													nam_components |= apnrfLastName;
													if(r_me.ConceptId && test_gender_concept && parent_sname_cid) {
														const CONCEPTID _c = r_me.ConceptId;
														if(p_srdb->GetConceptPropList(_c, cpl) > 0 && cpl.Get(_c, prop_instance, cp)) {
															CONCEPTID _val = 0;
															if(cp.Get(_val) && _val == parent_sname_cid) {
																if(cpl.Get(_c, gender_cid, cp_gender) && cp_gender.Get(_val)) {
																	if(_val == gender_mas_cid)
																		gender_mus_components |= apnrfLastName;
																	else if(_val == gender_fem_cid)
																		gender_fem_components |= apnrfLastName;
																}
															}
														}
													}
												}
												else if(temp_buf == "humname_f") {
													nam_components |= apnrfFirstName;
													if(r_me.ConceptId && test_gender_concept && parent_fname_cid) {
														const CONCEPTID _c = r_me.ConceptId;
														if(p_srdb->GetConceptPropList(_c, cpl) > 0 && cpl.Get(_c, prop_instance, cp)) {
															CONCEPTID _val = 0;
															if(cp.Get(_val) && _val == parent_fname_cid) {
																if(cpl.Get(_c, gender_cid, cp_gender) && cp_gender.Get(_val)) {
																	if(_val == gender_mas_cid)
																		gender_mus_components |= apnrfFirstName;
																	else if(_val == gender_fem_cid)
																		gender_fem_components |= apnrfFirstName;
																}
															}
														}
													}
												}
												else if(temp_buf == "humname_p") {
													nam_components |= apnrfPatronymic;
													if(r_me.ConceptId && test_gender_concept && parent_pname_cid) {
														const CONCEPTID _c = r_me.ConceptId;
														if(p_srdb->GetConceptPropList(_c, cpl) > 0 && cpl.Get(_c, prop_instance, cp)) {
															CONCEPTID _val = 0;
															if(cp.Get(_val) && _val == parent_pname_cid) {
																if(cpl.Get(_c, gender_cid, cp_gender) && cp_gender.Get(_val)) {
																	if(_val == gender_mas_cid)
																		gender_mus_components |= apnrfPatronymic;
																	else if(_val == gender_fem_cid)
																		gender_fem_components |= apnrfPatronymic;
																}
															}
														}
													}
												}
												break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	ASSIGN_PTR(pGenderMusComponents, gender_mus_components);
	ASSIGN_PTR(pGenderFemComponents, gender_fem_components);
	return nam_components;
}

IMPL_HANDLE_EVENT(PersonDialog)
{
	// @v10.0.01 {
	if(event.isCmd(cmExecute)) {
		if(InitPhone.NotEmpty()) {
			PPELink el;
			PPELinkArray::SetupNewPhoneEntry(InitPhone, el);
			if(EditELink(&el) > 0)
				Data.ELA.insert(&el);
		}
		// Далее управление передается базовому классу
	}
	// } @v10.0.01
	PersonDialogBase::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmCtlColor:
				{
					TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
					int   color_ident = 0;
					if(p_dc && getCtrlHandle(CTL_PERSON_NAME) == p_dc->H_Ctl) {
						SString name;
						getCtrlString(CTL_PERSON_NAME, name);
						long gender_fem_components = 0;
						long gender_mus_components = 0;
						long nam_components = AnalyzePersonName(name, &gender_mus_components, &gender_fem_components);
						if(nam_components) {
							if((nam_components & apnrfLastName) || (nam_components & (apnrfFirstName|apnrfPatronymic))) {
								color_ident = brushHumanName;
								if(gender_fem_components && !gender_mus_components)
									color_ident = brushHumanNameFem;
								else if(gender_mus_components && !gender_fem_components)
									color_ident = brushHumanNameMus;
								else
									color_ident = brushHumanName;
							}
						}
					}
					if(color_ident) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(color_ident));
					}
					else
						return;
				}
				break;
			case cmSelectAnalog:
				if(!Data.Rec.ID) {
					PPID   analog_id = 0;
					SString name;
					getCtrlString(CTL_PERSON_NAME, name);
					if(PsnObj.SelectAnalog(name, &analog_id) > 0) {
						PPPersonPacket  sel_pack;
						if(PsnObj.GetPacket(analog_id, &sel_pack, 0) > 0) {
							sel_pack.Kinds.addUnique(&Data.Kinds);
							setDTS(&sel_pack);
						}
					}
				}
				break;
			case cmPersonReg: PsnObj.RegObj.EditList(&Data, 0); break;
			case cmPersonPhones: EditELinks(Data.Rec.Name, &Data.ELA); break;
			case cmPersonBAcct: PsnObj.RegObj.EditBankAccountList(&Data); break;
			case cmPersonTags: EditObjTagValList(&Data.TagL, 0); break;
			case cmPersonAddr:
			case cmPersonRAddr: {
					PPObjLocation loc_obj;
					PPLocationPacket * p_loc_pack = (TVCMD == cmPersonAddr) ? &Data.Loc : &Data.RLoc;
					p_loc_pack->Type = LOCTYP_ADDRESS;
					if(p_loc_pack->ID == 0)
						loc_obj.InitCode(p_loc_pack);
					loc_obj.EditDialog(LOCTYP_ADDRESS, p_loc_pack, 0);
				}
				break;
			case cmPersonDlvrLocList: EditDlvrLocList(); break;
			case cmPersonRelList: EditPersonRelList(&Data); break;
			case cmaMore: break;
			case cmaInsert:
				{
					PPIDArray kind_list;
					ListToListData l2l(PPOBJ_PERSONKIND, 0, &kind_list);
					if(ListToListDialog(&l2l) > 0) {
						SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_PERSON_KIND));
						for(uint i = 0; i < kind_list.getCount(); i++) {
							PPID   kind_id = kind_list.at(i);
							if(kind_id == PPPRK_MAIN && !PPMaster)
								PPError(PPERR_UNCHANGABLEPERSONKIND, 0);
							else
								Data.Kinds.addUnique(kind_id);
						}
						if(p_box) {
							p_box->setDef(createKindListDef());
							p_box->Draw_();
						}
					}
					SetupCtrls();
				}
				break;
			case cmaDelete: RemoveKind(); break;
			case cmCashierRights:
				{
					#define CRDF_PRNREPORTS   0x0800

					class CashierRightsDialog : public TDialog {
						DECL_DIALOG_DATA(CashierInfo);
					public:
						CashierRightsDialog() : TDialog(DLG_CSHRRTS), OnlyView(0)
						{
							PPObjSecur obj_secur(PPOBJ_USR, 0);
							if(!obj_secur.CheckRights(PPR_MOD)) {
								enableCommand(cmCashierPassword, 0);
								enableCommand(cmOK, 0);
								disableCtrl(CTL_CSHRRTS_RIGHTS, 1);
								disableCtrl(CTL_CSHRRTS_RPTRIGHTS, 1);
								OnlyView = 1;
							}
						}
						~CashierRightsDialog()
						{
							enableCommand(cmCashierPassword, 1);
							enableCommand(cmOK, 1);
						}
						DECL_DIALOG_SETDTS()
						{
							RVALUEPTR(Data, pData);
							setCtrlData(CTL_CSHRRTS_RIGHTS, &Data.Rights);
							setCtrlData(CTL_CSHRRTS_RPTRIGHTS, reinterpret_cast<ushort *>(&Data.Rights) + 1);
							SetPrnRights();
							return 1;
						}
						DECL_DIALOG_GETDTS()
						{
							getCtrlData(CTL_CSHRRTS_RIGHTS, &Data.Rights);
							getCtrlData(CTL_CSHRRTS_RPTRIGHTS, reinterpret_cast<ushort *>(&Data.Rights) + 1);
							ASSIGN_PTR(pData, Data);
							return 1;
						}
					private:
						DECL_HANDLE_EVENT
						{
							TDialog::handleEvent(event);
							if(event.isCmd(cmCashierPassword))
								PasswordDialog(0, Data.Password, sizeof(Data.Password), 0, 1);
							else if(event.isClusterClk(CTL_CSHRRTS_RIGHTS))
								SetPrnRights();
							else
								return;
							clearEvent(event);
						}
						void   SetPrnRights()
						{
							if(!OnlyView) {
								ushort  rts = getCtrlUInt16(CTL_CSHRRTS_RIGHTS);
								if(!(rts & CRDF_PRNREPORTS))
									setCtrlData(CTL_CSHRRTS_RPTRIGHTS, &(rts = 0));
								disableCtrl(CTL_CSHRRTS_RPTRIGHTS, (rts & CRDF_PRNREPORTS) ? 0 : 1);
							}
						}
						int    OnlyView;
					};
					CashierRightsDialog * dlg = new CashierRightsDialog();
					if(CheckDialogPtrErr(&dlg)) {
						dlg->setDTS(&Data.CshrInfo);
						if(ExecView(dlg) == cmOK) {
							dlg->getDTS(&Data.CshrInfo);
							Data.CshrInfo.Flags |= CIF_MODIFIED;
						}
					}
					delete dlg;
					dlg = 0;
				}
				break;
			default:
				return;
		}
	}
	else if(TVBROADCAST && TVCMD == cmChangedFocus && TVINFOVIEW->TestId(CTL_PERSON_NAME)) {
		if(!Data.Rec.ID) {
			SString name;
			getCtrlString(CTL_PERSON_NAME, name);
			if(name.Len() > 0 && Name_.CmpNC(name) != 0) {
				Name_ = name;
				PPID   dup_id = 0;
				if(PsnObj.CheckDuplicateName(name, &dup_id) == 2) {
					DupID = dup_id;
					endModal(cmOK);
					return; // После endModal не следует обращаться к this
				}
			}
		}
	}
	else
		return;
	clearEvent(event);
}

MainOrg2Dialog::MainOrg2Dialog(int dlgID, PPPersonPacket * pData, PPObjPerson * pObj) : TDialog(dlgID), P_Obj(pObj), P_Pack(pData), PrefPos(-1)
{
	setDTS();
}

int MainOrg2Dialog::setDTS()
{
	int    ok = 1;
	char   buf[128], bic[128], corr[128], inn[128];
	SString temp_buf;
	PPID   bnk_id = 0;
	int    pos;
	uint   i;
	PPCommConfig cfg;
	THROW(GetCommConfig(&cfg));

	SetupPPObjCombo(this, CTLSEL_MAINORG2_STATUS, PPOBJ_PRSNSTATUS, P_Pack->Rec.Status, OLW_CANINSERT, 0);
	setCtrlData(CTL_MAINORG2_NAME, P_Pack->Rec.Name);
	P_Pack->GetExtName(temp_buf);
	setCtrlString(CTL_MAINORG2_EXTNAME, temp_buf);
	//
	// Set Address
	//
	SetupPPObjCombo(this, CTLSEL_MAINORG2_CITY, PPOBJ_WORLD, P_Pack->Loc.CityID, OLW_CANINSERT, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
	LocationCore::GetExField(&P_Pack->Loc, LOCEXSTR_SHORTADDR, temp_buf);
	setCtrlString(CTL_MAINORG2_ADDRESS, temp_buf);
	//
	// Set Bank, INN, CorrAcc and BicAcc
	//
	memzero(buf, sizeof(buf));
	{
		//
		// Не менять порядок просмотра всех расчетных счетов (должен быть от последнего к первому)
		//
		{
			int   ba_done = 0;
			i = P_Pack->Regs.getCount();
			if(i) do {
				const RegisterTbl::Rec & r_reg_rec = P_Pack->Regs.at(--i);
				if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
					PPBankAccount ba(r_reg_rec);
					if(ba.AccType == PPBAC_CURRENT) {
						memzero(buf, sizeof(buf));
						bnk_id = ba.BankID;
						STRNSCPY(buf, ba.Acct);
						pos = i;
						if(ba.Flags & /*BACCTF_PREFERRED*/PREGF_BACC_PREFERRED)
							ba_done = 1; // @exit
					}
				}
			} while(!ba_done && i);
			if(bnk_id) {
				P_Pack->Regs.at(pos).Flags |= /*BACCTF_PREFERRED*/PREGF_BACC_PREFERRED;
				PrefPos = pos;
			}
		}
	}
	getPsnRegs(corr, bic, inn, bnk_id, sizeof(corr), sizeof(bic), sizeof(inn));
	P_Pack->GetRegNumber(PPREGT_TPID, temp_buf);
	setCtrlString(CTL_MAINORG2_INN, temp_buf);
	SetupPPObjCombo(this, CTLSEL_MAINORG2_BANK, PPOBJ_PERSON, bnk_id, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_BANK));
	setCtrlData(CTL_MAINORG2_ACC, buf);
	setCtrlData(CTL_MAINORG2_CORRACC, corr);
	setCtrlData(CTL_MAINORG2_BIC, bic);
	//
	// Set Director and Accountant
	//
	SetupPPObjCombo(this, CTLSEL_MAINORG2_DIRECTOR, PPOBJ_PERSON, cfg.MainOrgDirector_, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
	SetupPPObjCombo(this, CTLSEL_MAINORG2_ACCTNT, PPOBJ_PERSON, cfg.MainOrgAccountant_, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
	//
	// Set Memo
	//
	// @v11.1.12 setCtrlData(CTL_MAINORG2_MEMO, P_Pack->Rec.Memo);
	setCtrlString(CTL_MAINORG2_MEMO, P_Pack->SMemo); // @v11.1.12
	//
	// Set Phone
	//
	P_Pack->ELA.GetItem(PPELK_WORKPHONE, temp_buf) > 0 || P_Pack->ELA.GetItem(PPELK_ALTPHONE, temp_buf) > 0 || P_Pack->ELA.GetItem(PPELK_HOMEPHONE, temp_buf) > 0;
	setCtrlString(CTL_MAINORG2_PHONE, temp_buf);
	CATCHZOKPPERR
	return ok;
}

int MainOrg2Dialog::getDTS()
{
	int    ok = 1, i, bnk_pos = -1, tel_pos = -1;
	int    c_pos = -1, i_pos = -1, b_pos = -1, find_acc = 0;
	uint   sel = 0;
	long   acc_type = 0, bnk_id = 0;
	char   buf[128], corr[128], bic[128], inn[128];
	SString temp_buf;
	PPCommConfig cfg;
	PPPersonPacket bnk_pack;
	PPObjRegister reg_obj;
	// get status
	getCtrlData(sel = CTLSEL_MAINORG2_STATUS, &P_Pack->Rec.Status);
	THROW_PP(P_Pack->Rec.Status, PPERR_PERSONSTATUSNEEDED);
	// get status

	// get Name
	getCtrlData(sel = CTL_MAINORG2_NAME, &P_Pack->Rec.Name);
	THROW_PP(*strip(P_Pack->Rec.Name), PPERR_NAMENEEDED);
	// get Name

	// get ExtName
	getCtrlString(CTL_MAINORG2_EXTNAME, temp_buf);
	P_Pack->SetExtName(temp_buf.Strip());
	// get ExtName

	// Get Address
	getCtrlData(CTLSEL_MAINORG2_CITY, &P_Pack->Loc.CityID);
	getCtrlString(sel = CTL_MAINORG2_ADDRESS, temp_buf);
	//
	// Либо адрес должен быть пустым либо должен быть указан город
	//
	THROW_PP(!temp_buf.NotEmptyS() || P_Pack->Loc.CityID, PPERR_CITYNEEDED);
	LocationCore::SetExField(&P_Pack->Loc, LOCEXSTR_SHORTADDR, temp_buf);
	// Get Address

	// Get Bank
	memzero(buf, sizeof(buf));
	getCtrlData(sel = CTLSEL_MAINORG2_BANK, &bnk_id);
	getCtrlData(CTL_MAINORG2_ACC, buf);
	THROW_PP(!(*strip(buf)) || bnk_id, PPERR_BANKNEEDED);
	sel = CTL_MAINORG2_ACC;
	THROW_PP(!bnk_id || *strip(buf), PPERR_ACCNEEDED);
	{
		// Не менять порядок просмотра всех расчетных счетов (должен быть от последнего к первому)
		int   ba_done = 0;
		i = P_Pack->Regs.getCount();
		if(i) do {
			const RegisterTbl::Rec & r_reg_rec = P_Pack->Regs.at(--i);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba(r_reg_rec);
				if(ba.AccType == PPBAC_CURRENT) {
					if(ba.BankID == bnk_id) {
						bnk_pos = i;
						find_acc = 1;
						if(sstreq(ba.Acct, buf))
							ba_done = 1; // @exit
					}
				}
			}
		} while(!ba_done && i);
		if(bnk_id) {
			if(PrefPos > -1)
				P_Pack->Regs.at(PrefPos).Flags &= ~PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/;
			if(find_acc && sstreq(P_Pack->Regs.at(bnk_pos).Num, buf))
				P_Pack->Regs.at(bnk_pos).Flags |= PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/;
			else {
				PPBankAccount bnk_rec;
				bnk_rec.BankID = bnk_id;
				STRNSCPY(bnk_rec.Acct, buf);
				bnk_rec.AccType = PPBAC_CURRENT;
				bnk_rec.Flags |= PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/;
				if(find_acc) {
					if(PPMessage(mfConf|mfYesNo, PPCFM_ADDACC) == cmNo) {
						THROW(P_Pack->Regs.atFree(bnk_pos));
						bnk_pos = -1;
					}
				}
				THROW(P_Pack->Regs.CheckDuplicateBankAccount(&bnk_rec, bnk_pos));
				THROW(P_Pack->Regs.SetBankAccount(&bnk_rec, static_cast<uint>(-1)));
			}
		}
		else if(PrefPos > -1) {
			P_Pack->Regs.atFree(PrefPos);
		}
		/*
		for(i = P_Pack->BAA.getCount(); i > 0 ; i--) {
			acc_type = P_Pack->BAA.at(i-1).AccType;
			if(acc_type == PPBAC_CURRENT) {
				if(P_Pack->BAA.at(i-1).BankID == bnk_id) {
					bnk_pos = i-1;
					find_acc = 1;
					if(sstreq(P_Pack->BAA.at(i-1).Acct, buf))
						break;
				}
			}
		}
		if(bnk_id) {
			if(PrefPos > -1)
				P_Pack->BAA.at(PrefPos).Flags &= ~BACCTF_PREFERRED;
			if(find_acc && sstreq(P_Pack->BAA.at(bnk_pos).Acct, buf))
				P_Pack->BAA.at(bnk_pos).Flags |= BACCTF_PREFERRED;
			else {
				BankAccountTbl::Rec bnk_rec;
				MEMSZERO(bnk_rec);
				bnk_rec.BankID = bnk_id;
				STRNSCPY(bnk_rec.Acct, buf);
				bnk_rec.AccType = PPBAC_CURRENT;
				bnk_rec.Flags |= BACCTF_PREFERRED;
				if(find_acc) {
					if(PPMessage(mfConf|mfYesNo, PPCFM_ADDACC) == cmNo) {
						THROW(P_Pack->BAA.atFree(bnk_pos));
						bnk_pos = -1;
					}
				}
				THROW(PPObjBnkAcct::CheckDuplicateBnkAcct(&bnk_rec, &P_Pack->BAA, bnk_pos));
				THROW(P_Pack->BAA.insert(&bnk_rec));
			}
		}
		else if(PrefPos>-1) {
			P_Pack->BAA.atFree(PrefPos);
		}
		*/
	}
	// Get Bank

	// Get INN, CorrAcc and BicAcc
	THROW(P_Obj->GetPacket(bnk_id, &bnk_pack, 0));
	for(i = 0; i < bnk_pack.Regs.getCountI(); i++) {
		if(bnk_pack.Regs.at(i).RegTypeID == PPREGT_BNKCORRACC && c_pos<0)
			c_pos = i;
		else if(bnk_pack.Regs.at(i).RegTypeID == PPREGT_BIC && b_pos<0)
			b_pos = i;
	}
	for(i = 0; i < P_Pack->Regs.getCountI(); i++)
		if(P_Pack->Regs.at(i).RegTypeID == PPREGT_TPID && i_pos < 0) {
			i_pos = i;
			break;
		}
	memzero(corr, sizeof(corr));
	memzero(bic,  sizeof(bic));
	memzero(inn,  sizeof(inn));
	getCtrlData(CTL_MAINORG2_INN, inn);
	getCtrlData(CTL_MAINORG2_CORRACC, corr);
	getCtrlData(CTL_MAINORG2_BIC, bic);
	THROW_PP((!(*strip(corr)) && !(*strip(bic))) || bnk_id, PPERR_BANKNEEDED);
	// Bank Corr
	if(*strip(corr)) {
		sel = CTL_MAINORG2_CORRACC;
		if(c_pos >= 0) {
			RegisterTbl::Rec reg_rec = bnk_pack.Regs.at(c_pos);
			STRNSCPY(reg_rec.Num, corr);
			THROW(reg_obj.CheckUniqueNumber(&reg_rec, &bnk_pack.Regs, PPOBJ_PERSON, reg_rec.ObjID));
			STRNSCPY(bnk_pack.Regs.at(c_pos).Num, corr);
		}
		else {
			THROW(bnk_pack.AddRegister(PPREGT_BNKCORRACC, corr));
		}
	}
	else if(c_pos >= 0)
		bnk_pack.Regs.atFree(c_pos);
	// Bank Bic
	if(*strip(bic)) {
		sel = CTL_MAINORG2_BIC;
		if(b_pos >= 0) {
			RegisterTbl::Rec reg_rec = bnk_pack.Regs.at(b_pos);
			STRNSCPY(reg_rec.Num, bic);
			THROW(reg_obj.CheckUniqueNumber(&reg_rec, &bnk_pack.Regs, PPOBJ_PERSON, reg_rec.ObjID));
			STRNSCPY(bnk_pack.Regs.at(b_pos).Num, bic);
		}
		else {
			THROW(bnk_pack.AddRegister(PPREGT_BIC, bic));
		}
	}
	else if(b_pos >= 0)
		bnk_pack.Regs.atFree(b_pos);
	// Organization INN
	if(*strip(inn)) {
		sel = CTL_MAINORG2_INN;
		if(i_pos >= 0) {
			RegisterTbl::Rec reg_rec = P_Pack->Regs.at(i_pos);
			STRNSCPY(reg_rec.Num, inn);
			THROW(reg_obj.CheckUniqueNumber(&reg_rec, &P_Pack->Regs, PPOBJ_PERSON, reg_rec.ObjID));
			STRNSCPY(P_Pack->Regs.at(i_pos).Num, inn);
		}
		else {
			THROW(P_Pack->AddRegister(PPREGT_TPID, inn));
		}
	}
	else if(i_pos >= 0)
		P_Pack->Regs.atFree(i_pos);
	THROW(P_Obj->PutPacket(&bnk_id, &bnk_pack, 1));
	// Get INN, CorrAcc and BicAcc

	// Get Director and Accountant
	THROW(GetCommConfig(&cfg));
	getCtrlData(CTLSEL_MAINORG2_DIRECTOR, &cfg.MainOrgDirector_);
	getCtrlData(CTLSEL_MAINORG2_ACCTNT, &cfg.MainOrgAccountant_);
	THROW(SetCommConfig(&cfg, 1));
	// Get Director and Accountant

	// Get Memo
	// @v11.1.12 getCtrlData(CTL_MAINORG2_MEMO, P_Pack->Rec.Memo);
	// @v11.1.12 strip(P_Pack->Rec.Memo);
	getCtrlString(CTL_MAINORG2_MEMO, P_Pack->SMemo); // @v11.1.12
	P_Pack->SMemo.Strip(); // @v11.1.12
	// Get Memo

	// Get Phone
	memzero(buf, sizeof(buf));
	getCtrlData(CTL_MAINORG2_PHONE, buf);
	for(i = P_Pack->ELA.getCount(); i > 0; i--) {
		const PPID kind_id = P_Pack->ELA.at(i-1).KindID;
		if(oneof2(kind_id, PPELK_HOMEPHONE, PPELK_ALTPHONE))
			tel_pos = i-1;
		if(kind_id == PPELK_WORKPHONE) {
			tel_pos = i-1;
			break;
		}
	}
	if(tel_pos > -1)
		if(*strip(buf) == 0)
			P_Pack->ELA.atFree(tel_pos);
		else
			STRNSCPY(P_Pack->ELA.at(tel_pos).Addr, buf);
	else if(*strip(buf))
		P_Pack->ELA.AddItem(PPELK_WORKPHONE, buf);
	// Get Phone
	CATCHZOKPPERRBYDLG
	return ok;
}

IMPL_HANDLE_EVENT(MainOrg2Dialog)
{
	uint   i;
	int    pos = -1;
	PPID   bnk_id;
	char   acct_buf[64], corr[48], bic[48], inn[20];
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		if(event.isCbSelected(CTLSEL_MAINORG2_BANK)) {
			PPPersonPacket bnk_pack;
			memzero(acct_buf, sizeof(acct_buf));
			getCtrlData(CTLSEL_MAINORG2_BANK, &bnk_id);
			getPsnRegs(corr, bic, inn, bnk_id, sizeof(corr), sizeof(bic), sizeof(inn));
			{
				int   ba_done = 0;
				i = P_Pack->Regs.getCount();
				if(i) do {
					const RegisterTbl::Rec & r_reg_rec = P_Pack->Regs.at(--i);
					if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
						PPBankAccount ba(r_reg_rec);
						if(ba.AccType == PPBAC_CURRENT && ba.BankID == bnk_id) {
							pos = i;
							if(ba.Flags & PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/)
								ba_done = 1; // @exit
						}
					}
				} while(!ba_done && i);
				if(pos >= 0) {
					STRNSCPY(acct_buf, P_Pack->Regs.at(pos).Num);
				}
			}
			setCtrlData(CTL_MAINORG2_ACC, acct_buf);
			setCtrlData(CTL_MAINORG2_CORRACC, corr);
			setCtrlData(CTL_MAINORG2_BIC, bic);
			clearEvent(event);
		}
}

int  MainOrg2Dialog::getPsnRegs(char *corrAcc, char *bicAcc, char *innAcc, long bnkID,
	size_t cAcc_sz, size_t bAcc_sz, size_t iAcc_sz)
{
	PPPersonPacket bnk_pack;
	int    ok = 1;
	memzero(corrAcc, cAcc_sz);
	memzero(bicAcc,  bAcc_sz);
	memzero(innAcc,  iAcc_sz);
	if(bnkID > -1) {
		SString temp_buf;
		P_Obj->GetPacket(bnkID, &bnk_pack, 0);
		bnk_pack.GetRegNumber(PPREGT_TPID, temp_buf); temp_buf.CopyTo(innAcc, iAcc_sz);
		bnk_pack.GetRegNumber(PPREGT_BNKCORRACC, temp_buf); temp_buf.CopyTo(corrAcc, cAcc_sz);
		bnk_pack.GetRegNumber(PPREGT_BIC, temp_buf); temp_buf.CopyTo(bicAcc, bAcc_sz);
	}
	else
		ok = 0;
	return ok;
}

int PPObjPerson::ExtEdit(PPID * pID, void * extraPtr)
{
	PPID extra_kind_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = 1, valid_data = 0, r = cmCancel, is_new = (*pID == 0);
	MainOrg2Dialog * mainorg2_dlg = 0;
	PPPersonPacket info;
	THROW(CheckRightsModByID(pID));
	if(is_new) {
		if(extra_kind_id <= 0) {
			extra_kind_id = SelectPersonKind();
			if(extra_kind_id <= 0)
				return cmCancel;
		}
		info.Kinds.insert(&extra_kind_id);
	}
	else
		THROW(GetPacket(*pID, &info, 0) > 0);
	THROW(CheckDialogPtr(&(mainorg2_dlg = new MainOrg2Dialog(DLG_MAINORG2, &info, this))));
	while(!valid_data && (r = ExecView(mainorg2_dlg)) == cmOK) {
		if((valid_data = mainorg2_dlg->getDTS()) != 0)
			if(!PutPacket(pID, &info, 1))
				valid_data = PPErrorZ();
	}
	CATCH
		PPError();
	ENDCATCH
	delete mainorg2_dlg;
	return ok ? r : 0;
}

int PPObjPerson::EditDlvrLocList(PPID personID)
{
	int    ok = -1;
	PPPersonPacket pack;
	AddrListDialog * dlg = 0;
	THROW(CheckRights(PPR_MOD));
	THROW(GetPacket(personID, &pack, 0) > 0);
	THROW(CheckDialogPtr(&(dlg = new AddrListDialog())));
	THROW(dlg->setDTS(&pack));
	while(ok < 0 && ExecView(dlg) == cmOK)
		if(dlg->getDTS(&pack)) {
			THROW(PutPacket(&personID, &pack, 1));
			ok = 1;
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int SelectPerson(SelPersonIdent * pData)
{
	class SelectPersonDialog : public TDialog {
		DECL_DIALOG_DATA(SelPersonIdent);
	public:
		SelectPersonDialog() : TDialog(DLG_SELPERSON)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_SELPERSON_KIND, PPOBJ_PERSONKIND, Data.KindID, 0);
			SetupPPObjCombo(this, CTLSEL_SELPERSON_PRSN, PPOBJ_PERSON, Data.PersonID, OLW_CANINSERT, reinterpret_cast<void *>(Data.KindID));
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			Data.KindID   = getCtrlLong(CTLSEL_SELPERSON_KIND);
			Data.PersonID = getCtrlLong(CTLSEL_SELPERSON_PRSN);
			if(Data.PersonID == 0)
				return PPErrorByDialog(this, CTLSEL_SELPERSON_PRSN, PPERR_PERSONNEEDED);
			else {
				ASSIGN_PTR(pData, Data);
				return 1;
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SELPERSON_KIND)) {
				PPID   kind_id = getCtrlLong(CTLSEL_SELPERSON_KIND);
				SetupPPObjCombo(this, CTLSEL_SELPERSON_PRSN, PPOBJ_PERSON, 0, 0, reinterpret_cast<void *>(kind_id));
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY(SelectPersonDialog, pData);
}
//
//
//
int PPObjPerson::EditAmountList(PPID id)
{
	int    ok = -1;
	PPPersonPacket pack;
	THROW(GetPacket(id, &pack, 0) > 0);
	THROW_PP(pack.Kinds.lsearch(PPPRK_EMPLOYER), PPERR_PERSONCANTHAVEAMTS);
	if(EditStaffAmtList(&pack.Amounts, 0, 1) > 0) {
		THROW(PutPacket(&id, &pack, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
struct PersonLink {
	enum {
		fLockPrmr     = 0x0001,
		fLockScnd     = 0x0002,
		fLockType     = 0x0004,
		fLockScndList = 0x0008,
		fSwapPerson   = 0x0010
	};
	PersonLink() : PrmrPersonID(0), LinkTypeID(0), ScndPersonKind(0), Flags(0)
	{
	}
	void Init()
	{
		PrmrPersonID = LinkTypeID = ScndPersonKind = Flags = 0;
		ScndPersonList.clear(); // @v10.6.12 freeAll()-->clear()
	}
	PPID   PrmrPersonID;
	PPID   LinkTypeID;
	PPID   ScndPersonKind;
	long   Flags;
	PPIDArray ScndPersonList;
};

static int EditPersonRel(PersonLink * pData)
{
	class PersonRelDialog : public TDialog {
		DECL_DIALOG_DATA(PersonLink);
		enum {
			ctlgroupPsnList = 1
		};
	public:
		PersonRelDialog() : TDialog(DLG_PERSONLINK)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			// @v10.2.3 SetupPPObjCombo(this, CTLSEL_PERSONLINK_PRMR,  PPOBJ_PERSON, Data.PrmrPersonID, OLW_CANINSERT, 0);
			SetupPersonCombo(this, CTLSEL_PERSONLINK_PRMR, Data.PrmrPersonID, OLW_CANINSERT, 0, 0); // @v10.2.3
			SetupPPObjCombo(this, CTLSEL_PERSONLINK_LTYPE, PPOBJ_PERSONRELTYPE, Data.LinkTypeID, OLW_CANINSERT, 0);
			disableCtrl(CTLSEL_PERSONLINK_PRMR,  Data.Flags & PersonLink::fLockPrmr);
			disableCtrl(CTLSEL_PERSONLINK_LTYPE, Data.Flags & PersonLink::fLockType);
			if(Data.Flags & PersonLink::fSwapPerson) {
				SString prmr_text, scnd_text;
				getLabelText(CTL_PERSONLINK_PRMR, prmr_text);
				getLabelText(CTL_PERSONLINK_SCND, scnd_text);
				setLabelText(CTL_PERSONLINK_PRMR, scnd_text);
				setLabelText(CTL_PERSONLINK_SCND, prmr_text);
			}
			SetupGroup();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			PPID   single_id = 0;
			getCtrlData(sel = CTLSEL_PERSONLINK_PRMR,  &Data.PrmrPersonID);
			THROW_PP(Data.PrmrPersonID, PPERR_PERSONNEEDED);
			if(Data.Flags & (PersonLink::fLockScndList|PersonLink::fLockScnd)) {
				getCtrlData(sel = CTLSEL_PERSONLINK_SCND, &single_id);
				Data.ScndPersonList.freeAll();
				Data.ScndPersonList.add(single_id);
				THROW_PP(single_id != Data.PrmrPersonID, PPERR_SAMEPERSONREL);
			}
			else {
				PersonListCtrlGroup::Rec grp_rec;
				getGroupData(ctlgroupPsnList, &grp_rec);
				single_id = grp_rec.List.getSingle();
				THROW_PP(single_id != Data.PrmrPersonID, PPERR_SAMEPERSONREL);
				grp_rec.List.freeByKey(Data.PrmrPersonID, 0);
				sel = CTLSEL_PERSONLINK_SCND;
				setGroupData(ctlgroupPsnList, &grp_rec);
				Data.ScndPersonList = grp_rec.List;
				Data.ScndPersonKind = grp_rec.PsnKindID;
			}
			THROW_PP(Data.ScndPersonList.getCount(), PPERR_PERSONNEEDED);
			getCtrlData(sel = CTLSEL_PERSONLINK_LTYPE, &Data.LinkTypeID);
			THROW_PP(Data.LinkTypeID, PPERR_RELTYPENEEDED);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			/*
			if(TVCOMMAND && TVCMD == cmCBSelected && (event.isCtlEvent(CTLSEL_PERSONLINK_PRMR) || event.isCtlEvent(CTLSEL_PERSONLINK_LTYPE))) {
				if((Data.Flags & (PersonLink::fLockScndList|PersonLink::fLockScnd)) == 0)
					SetupGroup();
			}
			*/
		}
		int SetupGroup()
		{
			if(Data.Flags & (PersonLink::fLockScndList|PersonLink::fLockScnd)) {
				// @v10.2.3 SetupPPObjCombo(this, CTLSEL_PERSONLINK_SCND, PPOBJ_PERSON, Data.ScndPersonList.getSingle(), OLW_CANINSERT, 0);
				SetupPersonCombo(this, CTLSEL_PERSONLINK_SCND, Data.ScndPersonList.getSingle(), OLW_CANINSERT, 0, 0); // @v10.2.3
				disableCtrl(CTLSEL_PERSONLINK_KIND, 1);
				disableCtrl(CTLSEL_PERSONLINK_SCND,  Data.Flags & PersonLink::fLockScnd);
				enableCommand(cmPersonList, 0);
			}
			else {
				PersonListCtrlGroup * p_grp = static_cast<PersonListCtrlGroup *>(getGroup(ctlgroupPsnList));
				if(!p_grp)
					addGroup(ctlgroupPsnList, new PersonListCtrlGroup(CTLSEL_PERSONLINK_SCND, CTLSEL_PERSONLINK_KIND, cmPersonList, OLW_CANINSERT));
				PersonListCtrlGroup::Rec grp_rec(Data.ScndPersonKind, &Data.ScndPersonList);
				setGroupData(ctlgroupPsnList, &grp_rec);
			}
			return 1;
		}
		PPObjPerson PsnObj;
	};
	DIALOG_PROC_BODY(PersonRelDialog, pData);
}

class PersonRelListDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPPersonPacket);
public:
	PersonRelListDialog() : PPListDialog(DLG_PERSONLINKLIST, CTL_PERSONLINKLIST_LIST), IsReverse(0)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_PERSONLINKLIST_PSN, Data.Rec.Name);
		updateList(-1);
		selectCtrl(CTL_PERSONLINKLIST_LIST);
		showCtrl(STDCTL_EDITBUTTON, !(Data.UpdFlags & PPPersonPacket::ufDontEditRelPsn));
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int delItem(long pos, long id);
	virtual int editItem(long pos, long id);
	void   reverseList();

	PPObjPerson PsnObj;
	int    IsReverse;
	LAssocArray Reverse;
};

void PersonRelListDialog::reverseList()
{
	if(IsReverse) {
		IsReverse = 0;
		Reverse.freeAll();
	}
	else {
		PsnObj.P_Tbl->GetRelList(Data.Rec.ID, &Reverse, 1);
		IsReverse = 1;
	}
	enableCommand(cmaInsert, !IsReverse);
	enableCommand(cmaDelete, !IsReverse);
	updateList(0);
}

IMPL_HANDLE_EVENT(PersonRelListDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmPersonLinkListReverse)) {
		reverseList();
		clearEvent(event);
	}
}

int PersonRelListDialog::setupList()
{
	int    ok = 1;
	StringSet ss(SLBColumnDelim);
	SString sub;
	const LAssocArray * p_rel_list = IsReverse ? &Reverse : &Data.GetRelList();
	LAssoc * p_item;
	for(uint i = 0; p_rel_list->enumItems(&i, (void **)&p_item);) {
		PPPersonRelType reltyp_rec;
		ss.clear();
		GetPersonName(p_item->Key, sub);
		ss.add(sub);
		if(SearchObject(PPOBJ_PERSONRELTYPE, p_item->Val, &reltyp_rec) > 0)
			sub = reltyp_rec.Name;
		else
			ideqvalstr(p_item->Val, sub);
		ss.add(sub);
		if(!addStringToList(/*p_item->Val*/i, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

int PersonRelListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	if(!IsReverse) {
		PersonLink pl_item;
		pl_item.PrmrPersonID = Data.Rec.ID;
		pl_item.Flags |= PersonLink::fLockPrmr;
		for(int valid_data = 0; !valid_data && EditPersonRel(&pl_item) > 0;) {
			uint   pos = 0;
			if(!pl_item.ScndPersonList.getSingle())
				Data.RemoveRelations(&pl_item.ScndPersonList, pl_item.LinkTypeID);
			if(Data.AddRelations(&pl_item.ScndPersonList, pl_item.LinkTypeID, &pos)) {
				ASSIGN_PTR(pPos, pos);
				updateList(-1);
				ASSIGN_PTR(pID, pl_item.ScndPersonList.at(0));
				ok = valid_data = 1;
			}
			else
				PPError();
		}
	}
	return ok;
}

int PersonRelListDialog::delItem(long pos, long id)
{
	return (!IsReverse && Data.RemoveRelationByPos(static_cast<uint>(pos))) ? 1 : -1;
}

int PersonRelListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	const  LAssocArray * p_rel_list = IsReverse ? &Reverse : &Data.GetRelList();
	if(p_rel_list && pos >= 0 && pos < (long)p_rel_list->getCount()) {
		PPID psn_id = p_rel_list->at(pos).Key;
		PPObjPerson::EditBlock edit_block;
		edit_block.UpdFlags |= PPPersonPacket::ufDontEditRelPsn;
		const int r = PsnObj.Edit_(&psn_id, edit_block);
		ok = (r == cmOK) ? 1 : (r ? -1 : 0);
	}
	return ok;
}

static int EditPersonRelList(PPPersonPacket * pData) { DIALOG_PROC_BODY(PersonRelListDialog, pData); }
//
//
//
int PPObjPerson::EditRelationList(PPID id)
{
	int    ok = -1;
	PPPersonPacket pack;
	THROW(GetPacket(id, &pack, 0) > 0);
	while(ok < 0 && EditPersonRelList(&pack) > 0)
		if(CheckRights(PPR_MOD) && PutPacket(&id, &pack, 1))
			ok = 1;
		else
			PPError();
	CATCHZOKPPERR
	return ok;
}

int PPObjPerson::AddRelationList(PPID * pPrmrID, PPIDArray * pScndList, PPID * pRelTypeID, int reverse)
{
	int    ok = -1, valid_data = 0;
	// @V10.3.0 (never used) int    edit = 0;
	PersonLink pl_item;
	PPPersonPacket pack;
	THROW(CheckRights(PPR_MOD));
	pl_item.PrmrPersonID = DEREFPTRORZ(pPrmrID);
	pl_item.LinkTypeID   = DEREFPTRORZ(pRelTypeID);
	RVALUEPTR(pl_item.ScndPersonList, pScndList);
	SETFLAG(pl_item.Flags, PersonLink::fLockPrmr, pl_item.PrmrPersonID);
	SETFLAG(pl_item.Flags, PersonLink::fLockScnd, pl_item.ScndPersonList.getCount());
	SETFLAG(pl_item.Flags, PersonLink::fSwapPerson, reverse);
	while(!valid_data && EditPersonRel(&pl_item) > 0) {
		PPIDArray prmr_list, scnd_list;
		if(reverse) {
			prmr_list = pl_item.ScndPersonList;
			scnd_list.add(pl_item.PrmrPersonID);
		}
		else {
			prmr_list.add(pl_item.PrmrPersonID);
			scnd_list = pl_item.ScndPersonList;
		}
		int ta = 0, stop = 0;
		THROW(PPStartTransaction(&ta, 1));
		for(uint i = 0; !stop && i < prmr_list.getCount(); i++) {
			PPID prmr_id = prmr_list.at(i);
			THROW(GetPacket(prmr_id, &pack, 0) > 0);
			if(prmr_list.getSingle() ==0 || scnd_list.getSingle() == 0)
				pack.RemoveRelations(&scnd_list, pl_item.LinkTypeID);
			if(pack.AddRelations(&scnd_list, pl_item.LinkTypeID, 0)) {
				if(!PutPacket(&prmr_id, &pack, 0)) {
					stop = 1;
					PPError();
				}
			}
			else {
				stop = 1;
				PPError();
			}
		}
		if(stop) {
			THROW(PPRollbackWork(&ta));
		}
		else {
			THROW(PPCommitWork(&ta));
			ASSIGN_PTR(pPrmrID,    pl_item.PrmrPersonID);
			ASSIGN_PTR(pScndList,  pl_item.ScndPersonList);
			ASSIGN_PTR(pRelTypeID, pl_item.LinkTypeID);
			ok = valid_data = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjPerson::EditRelation(PPID * pPrmrID, PPID * pScndID, PPID * pRelTypeID)
{
	int    ok = -1;
	int    valid_data = 0;
	bool   edit = false;
	PersonLink pl_item;
	PPPersonPacket pack;
	THROW(CheckRights(PPR_MOD));
	pl_item.PrmrPersonID = DEREFPTRORZ(pPrmrID);
	if(pScndID)
		pl_item.ScndPersonList.add(*pScndID);
	pl_item.LinkTypeID   = DEREFPTRORZ(pRelTypeID);
	SETFLAG(pl_item.Flags, PersonLink::fLockPrmr, pl_item.PrmrPersonID);
	pl_item.Flags |= PersonLink::fLockScndList;
	if(pl_item.PrmrPersonID && pl_item.ScndPersonList.getSingle()) {
		// @v10.3.0 (never used) long   type_id = 0;
		LAssocArray rel_list;
		THROW(GetPacket(pl_item.PrmrPersonID, &pack, 0) > 0);
		rel_list = pack.GetRelList();
		edit = rel_list.SearchPair(pl_item.ScndPersonList.getSingle(), pl_item.LinkTypeID, 0);
		if(edit)
			pack.RemoveRelation(pl_item.ScndPersonList.getSingle(), pl_item.LinkTypeID);
	}
	while(!valid_data && EditPersonRel(&pl_item) > 0) {
		if(!edit) {
			THROW(GetPacket(pl_item.PrmrPersonID, &pack, 0) > 0);
		}
		else
			pack.RemoveRelation(pl_item.ScndPersonList.getSingle(), pl_item.LinkTypeID);
		if(pack.AddRelation(pl_item.ScndPersonList.getSingle(), pl_item.LinkTypeID, 0)) {
			if(PutPacket(&pl_item.PrmrPersonID, &pack, 1)) {
				ASSIGN_PTR(pPrmrID,    pl_item.PrmrPersonID);
				ASSIGN_PTR(pScndID,    pl_item.ScndPersonList.getSingle());
				ASSIGN_PTR(pRelTypeID, pl_item.LinkTypeID);
				ok = valid_data = 1;
			}
			else
				PPError();
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjPerson::RemoveRelation(PPID prmrID, PPID scndID, PPID relTypeID)
{
	int    ok = -1;
	if(prmrID && scndID && CONFIRM(PPCFM_DELPERSONREL)) {
		PPPersonPacket pack;
		THROW(GetPacket(prmrID, &pack, 0) > 0);
		if(pack.RemoveRelation(scndID, relTypeID) > 0) {
			THROW(PutPacket(&prmrID, &pack, 1));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjPerson::SearchEmail(const char * pEmail, long flags, PPIDArray * pPsnList, PPIDArray * pLocList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPIDArray result_psn_list;
	PPIDArray result_loc_list;
	SString email(pEmail);
	email.Strip().ToLower();
	if(email.NotEmpty()) {
		SString temp_buf;
		if(pPsnList) {
			PPELinkArray ela;
			PPObjELinkKind elk_obj;
			PPELinkKind elk_rec;
			PPIDArray psn_id_list;
			PropertyTbl::Key1 k1;
			MEMSZERO(k1);
			k1.ObjType = PPOBJ_PERSON;
			k1.Prop = PSNPRP_ELINK;
			//
			// Сначала получим список персоналий, имеющих записи электронных адресов...
			//
			if(p_ref->Prop.search(1, &k1, spGe) && p_ref->Prop.data.ObjType == PPOBJ_PERSON && p_ref->Prop.data.Prop == PSNPRP_ELINK) do {
				if(p_ref->Prop.data.Val2)
					psn_id_list.addUnique(p_ref->Prop.data.ObjID);
			} while(p_ref->Prop.search(1, &k1, spNext) && p_ref->Prop.data.ObjType == PPOBJ_PERSON && p_ref->Prop.data.Prop == PSNPRP_ELINK);
			for(uint j = 0; j < psn_id_list.getCount(); j++) {
				const PPID psn_id = psn_id_list.get(j);
				PersonTbl::Rec psn_rec;
				P_Tbl->GetELinks(psn_id, ela);
				for(uint i = 0; i < ela.getCount(); i++) {
					const PPELink & r_item = ela.at(i);
					if(elk_obj.Fetch(r_item.KindID, &elk_rec) > 0 && elk_rec.Type == ELNKRT_EMAIL) {
						temp_buf = r_item.Addr;
						if(temp_buf.NotEmptyS()) {
							temp_buf.ToLower();
							if(temp_buf == email && Search(psn_id, &psn_rec) > 0) {
								result_psn_list.add(psn_id);
							}
						}
					}
				}
			}
		}
		if(pLocList) {
			LocationTbl * t = LocObj.P_Tbl;
			LocationTbl::Key2 k2;
			LocationTbl::Rec loc_rec;
            BExtQuery lq(t, 2);
            lq.select(t->ID, t->Tail, 0L).where(t->Type == LOCTYP_ADDRESS);
            MEMSZERO(k2);
            k2.Type = LOCTYP_ADDRESS;
            for(lq.initIteration(false, &k2, spGe); lq.nextIteration() > 0;) {
				if(LocationCore::GetExField(&t->data, LOCEXSTR_EMAIL, temp_buf) > 0) {
					if(temp_buf.NotEmptyS()) {
						temp_buf.ToLower();
						const PPID loc_id = t->data.ID;
						if(temp_buf == email && LocObj.Fetch(loc_id, &loc_rec) > 0) {
							result_loc_list.add(loc_id);
						}
					}
				}
            }
		}
	}
	if(result_psn_list.getCount() || result_loc_list.getCount())
		ok = 1;
	ASSIGN_PTR(pPsnList, result_psn_list);
	ASSIGN_PTR(pLocList, result_loc_list);
	return ok;
}

int PPObjPerson::IndexPhones(PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString phone, main_city_prefix, city_prefix, temp_buf;
	PropertyTbl::Key1 k1;
	MEMSZERO(k1);
	{
		PPELinkArray ela;
		PPObjELinkKind elk_obj;
		PPELinkKind elk_rec;
		PPIDArray psn_id_list;
		{
			PPID   main_city_id = 0;
			WorldTbl::Rec main_city_rec;
			if(GetMainCityID(&main_city_id) > 0 && LocObj.FetchCity(main_city_id, &main_city_rec) > 0)
				PPEAddr::Phone::NormalizeStr(main_city_rec.Phone, 0, main_city_prefix);
		}
		PPTransaction tra(use_ta);
		THROW(tra);
		k1.ObjType = PPOBJ_PERSON;
		k1.Prop = PSNPRP_ELINK;
		//
		// Сначала получим список персоналий, имеющих записи электронных адресов...
		//
		if(p_ref->Prop.search(1, &k1, spGe) && p_ref->Prop.data.ObjType == PPOBJ_PERSON && p_ref->Prop.data.Prop == PSNPRP_ELINK)
			do {
				psn_id_list.addUnique(p_ref->Prop.data.ObjID);
			} while(p_ref->Prop.search(1, &k1, spNext) && p_ref->Prop.data.ObjType == PPOBJ_PERSON && p_ref->Prop.data.Prop == PSNPRP_ELINK);
		//
		// ...а уже потом по этому списку проиндексируем телефоны.
		//
		for(uint j = 0; j < psn_id_list.getCount(); j++) {
			PersonTbl::Rec psn_rec;
			WorldTbl::Rec city_rec;
			PPObjID objid(PPOBJ_PERSON, psn_id_list.get(j));
			if(Search(objid.Id, &psn_rec) > 0) {
				PPID   city_addr_id = NZOR(psn_rec.RLoc, psn_rec.MainLoc);
				city_prefix = 0;
				if(LocObj.FetchCityByAddr(city_addr_id, &city_rec) > 0 && city_rec.Phone[0])
					PPEAddr::Phone::NormalizeStr(city_rec.Phone, 0, city_prefix);
				city_prefix.SetIfEmpty(main_city_prefix);
				P_Tbl->GetELinks(objid.Id, ela);
				for(uint i = 0; i < ela.getCount(); i++) {
					const PPELink & r_item = ela.at(i);
					if(elk_obj.Fetch(r_item.KindID, &elk_rec) > 0) {
						if(elk_rec.Type == ELNKRT_PHONE) {
							(temp_buf = r_item.Addr).Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
							PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone);
							if(phone.Len() >= 5) {
								if(city_prefix.Len()) {
									size_t sl = phone.Len() + city_prefix.Len();
									if(oneof2(sl, 10, 11))
										phone = (temp_buf = city_prefix).Cat(phone);
								}
								if(!LocObj.P_Tbl->IndexPhone(phone, &objid, 0, 0)) {
									if(pLogger)
										pLogger->LogLastError();
									else
										CALLEXCEPT();
								}
							}
						}
						else if(elk_rec.Type == ELNKRT_INTERNALEXTEN) {
							(temp_buf = r_item.Addr).Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
							PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone);
							if(phone.Len() > 1 && phone.Len() < 5) {
								if(!LocObj.P_Tbl->IndexPhone(phone, &objid, 0, 0)) {
									if(pLogger)
										pLogger->LogLastError();
									else
										CALLEXCEPT();
								}
							}
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
class PersonCache : public ObjCacheHash {
public:
	struct PersonData : public ObjCacheEntry {
		PPID   MainLocID;
		PPID   RLocID;
		PPID   CatID;
		long   Status;
		long   Flags;
	};
	PersonCache() : ObjCacheHash(PPOBJ_PERSON, sizeof(PersonData), (1024*1024), 4)
	{
		Cfg.Init();
		Cfg.Flags &= ~PPPersonConfig::fValid;
	}
	int    GetConfig(PPPersonConfig * pCfg, int enforce);
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	PPPersonConfig Cfg;
	ReadWriteLock CfgLock;
};

int PersonCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	PersonData * p_cache_rec = static_cast<PersonData *>(pEntry);
	PPObjPerson psn_obj(SConstructorLite);
	PersonTbl::Rec rec;
	if(psn_obj.Search(id, &rec) > 0) {
	   	p_cache_rec->MainLocID = rec.MainLoc;
		p_cache_rec->RLocID = rec.RLoc;
		p_cache_rec->CatID  = rec.CatID;
		p_cache_rec->Status = rec.Status;
		p_cache_rec->Flags  = rec.Flags;
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void PersonCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PersonTbl::Rec * p_data_rec = static_cast<PersonTbl::Rec *>(pDataRec);
	const PersonData * p_cache_rec = static_cast<const PersonData *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->ID       = p_cache_rec->ID;
	p_data_rec->MainLoc  = p_cache_rec->MainLocID;
	p_data_rec->RLoc     = p_cache_rec->RLocID;
	p_data_rec->CatID    = p_cache_rec->CatID;
	p_data_rec->Status   = p_cache_rec->Status;
	p_data_rec->Flags    = p_cache_rec->Flags;
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}

int PersonCache::GetConfig(PPPersonConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(CfgLock, SReadWriteLocker::Read);
		if(!(Cfg.Flags & PPPersonConfig::fValid) || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(Cfg.Flags & PPPersonConfig::fValid) || enforce) {
				PPObjPerson::ReadConfig(&Cfg);
				Cfg.Flags |= PPPersonConfig::fValid;
			}
		}
		ASSIGN_PTR(pCfg, Cfg);
	}
	return 1;
}

int FASTCALL PPObjPerson::FetchConfig(PPPersonConfig * pCfg)
{
	PersonCache * p_cache = GetDbLocalCachePtr <PersonCache> (PPOBJ_PERSON, 1);
	if(p_cache) {
		return p_cache->GetConfig(pCfg, 0);
	}
	else {
		CALLPTRMEMB(pCfg, Init());
		return 0;
	}
}

int PPObjPerson::DirtyConfig()
{
	PersonCache * p_cache = GetDbLocalCachePtr <PersonCache> (PPOBJ_PERSON, 0);
	return p_cache ? p_cache->GetConfig(0, 1) : 0;
}

IMPL_OBJ_FETCH(PPObjPerson, PersonTbl::Rec, PersonCache)
//
//
//
int STDCALL SetupPersonCombo(TDialog * dlg, uint ctlID, PPID id, uint flags, PPID personKindID, int disableIfZeroPersonKind)
{
	int    ok = 1;
	// @v10.3.0 (never used) int    create_ctl_grp = 0;
	if(disableIfZeroPersonKind)
		dlg->disableCtrl(ctlID, personKindID == 0);
	//if(personKindID) {
		PersonCtrlGroup * p_grp = 0;
		if(!(p_grp = static_cast<PersonCtrlGroup *>(dlg->getGroup(ctlID)))) {
			p_grp = new PersonCtrlGroup(ctlID, 0, personKindID);
			dlg->addGroup(ctlID, p_grp);
		}
		else
			p_grp->SetPersonKind(personKindID);
		ok = SetupPPObjCombo(dlg, ctlID, PPOBJ_PERSON, id, flags, reinterpret_cast<void *>(personKindID));
		if(ok) {
			int min_symb = personKindID ? 2 : 4;
			dlg->SetupWordSelector(ctlID, new PersonSelExtra(0, personKindID), id, min_symb, WordSel_ExtraBlock::fAlwaysSearchBySubStr); // @v10.1.6 WordSel_ExtraBlock::fAlwaysSearchBySubStr
		}
	/*}
	else
		dlg->setCtrlLong(ctlID, 0);*/
	return ok;
}

int MessagePersonBirthDay(TDialog * pDlg, PPID psnID)
{
	int    ok = -1;
	ObjTagItem tag;
	LDATE  dob;
	const  LDATE cday = getcurdate_();
	if(pDlg && psnID && PPRef->Ot.GetTag(PPOBJ_PERSON, psnID, PPTAG_PERSON_DOB, &tag) > 0 && tag.GetDate(&dob) > 0 && dob.day() == cday.day() && dob.month() == cday.month()) {
		SString name_buf;
		if(GetPersonName(psnID, name_buf) > 0) {
			SString msg_buf, fmt_buf;
			// @v10.2.4 PPLoadText(PPTXT_BIRTHDAYINFO, fmt_buf);
			// @v10.2.4 msg_buf.Printf(fmt_buf, name_buf.cptr());
			// @v10.2.4 {
			PPLoadText(PPTXT_CLIBIRTHDAY, fmt_buf);
			PPFormat(fmt_buf, &msg_buf, name_buf.cptr(), (int)(getcurdate_().year() - dob.year()));
			PPTooltipMessage(msg_buf, 0, pDlg->H(), 20000, GetColorRef(SClrPink),
				SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus/*|SMessageWindow::fLargeText*/);
			// } @v10.2.4
			ok = 1;
		}
	}
	return ok;
}
//
// @ModuleDef(PPObjPersonKind)
//
PPPersonKind2::PPPersonKind2()
{
	THISZERO();
}

PPObjPersonKind::PPObjPersonKind(void * extraPtr) : PPObjReference(PPOBJ_PERSONKIND, extraPtr)
{
}

int PPObjPersonKind::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	// @v10.3.0 (never used) int    ta = 0;
	int    is_new = 0;
	PPPersonKind psnk;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PSNKIND))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &psnk) > 0);
	}
	// @v10.6.6 @ctr else MEMSZERO(psnk);
	dlg->setCtrlLong(CTL_PSNKIND_ID, psnk.ID);
	dlg->setCtrlData(CTL_PSNKIND_NAME, psnk.Name);
	dlg->setCtrlData(CTL_PSNKIND_SYMB, psnk.Symb);
	SetupPPObjCombo(dlg, CTLSEL_PSNKIND_CODEREGT,  PPOBJ_REGISTERTYPE, psnk.CodeRegTypeID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_PSNKIND_FOLDREGT,  PPOBJ_REGISTERTYPE, psnk.FolderRegTypeID, 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_PSNKIND_DEFSTATUS, PPOBJ_PRSNSTATUS,   psnk.DefStatusID, 0, 0);
	dlg->AddClusterAssoc(CTL_PSNKIND_FLAGS, 0, PPPersonKind::fUseShortPersonDialog);
	dlg->SetClusterData(CTL_PSNKIND_FLAGS, psnk.Flags);
	dlg->disableCtrl(CTL_PSNKIND_ID, 1);
	if(ExecView(dlg) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		dlg->getCtrlData(CTL_PSNKIND_NAME, psnk.Name);
		dlg->getCtrlData(CTL_PSNKIND_SYMB, psnk.Symb);
		if(!CheckName(*pID, strip(psnk.Name), 0))
			dlg->selectCtrl(CTL_PSNKIND_NAME);
		else if(*strip(psnk.Symb) && !P_Ref->CheckUniqueSymb(Obj, psnk.ID, psnk.Symb, offsetof(PPPersonKind, Symb)))
			PPErrorByDialog(dlg, CTL_PSNKIND_SYMB);
		else {
			dlg->getCtrlData(CTL_PSNKIND_CODEREGT,  &psnk.CodeRegTypeID);
			dlg->getCtrlData(CTL_PSNKIND_FOLDREGT,  &psnk.FolderRegTypeID);
			dlg->getCtrlData(CTL_PSNKIND_DEFSTATUS, &psnk.DefStatusID);
			dlg->GetClusterData(CTL_PSNKIND_FLAGS, &psnk.Flags);
			*pID = psnk.ID;
			THROW(StoreItem(PPOBJ_PERSONKIND, *pID, &psnk, 1));
			*pID = psnk.ID;
			Dirty(*pID);
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjPersonKind::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRPERSONKIND, 0); }

int PPObjPersonKind::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_REGISTERTYPE) {
			PPPersonKind rec;
			for(PPID id = 0; ok == DBRPL_OK && EnumItems(&id, &rec) > 0;)
				if(rec.CodeRegTypeID == _id || rec.FolderRegTypeID == _id)
					ok = RetRefsExistsErr(Obj, id);
		}
	}
	return ok;
}

int PPObjPersonKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPPersonKind * p_rec = static_cast<PPPersonKind *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_REGISTERTYPE, &p_rec->CodeRegTypeID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_REGISTERTYPE, &p_rec->FolderRegTypeID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PRSNSTATUS,   &p_rec->DefStatusID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjPersonKind::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	THROW(p->Data);
	if(stream == 0) {
		PPPersonKind * p_rec = static_cast<PPPersonKind *>(p->Data);
		if(*pID || SearchByName(p_rec->Name, pID, 0) > 0) {
			p_rec->ID = *pID;
			int    r = StoreItem(Obj, *pID, p_rec, 1);
			if(r > 0)
   			    ok = ((*pID = P_Ref->data.ObjID), 102);
			else if(r < 0)
				ok = 1;
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
				THROW(*pID);
				ok = -1;
			}
		}
		else {
			p_rec->ID = *pID = 0;
			if(StoreItem(Obj, *pID, p_rec, 1))
			    ok = ((*pID = P_Ref->data.ObjID), 101);
			else {
				pCtx->OutputAcceptObjErrMsg(Obj, p_rec->ID, p_rec->Name);
				ok = -1;
			}
		}
	}
	else {
		THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}
//
//
//
class PersonKindCache : public ObjCache {
public:
	PersonKindCache() : ObjCache(PPOBJ_PERSONKIND, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		PPID   CodeRegTypeID;
		PPID   FolderRegTypeID;
		PPID   DefStatusID;
		long   Flags;
	};
};

int PersonKindCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjPersonKind pk_obj;
	PPPersonKind rec;
	if(pk_obj.Search(id, &rec) > 0) {
		p_cache_rec->CodeRegTypeID = rec.CodeRegTypeID;
		p_cache_rec->FolderRegTypeID = rec.FolderRegTypeID;
		p_cache_rec->DefStatusID = rec.DefStatusID;
		p_cache_rec->Flags = rec.Flags;

		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void PersonKindCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPPersonKind * p_data_rec = static_cast<PPPersonKind *>(pDataRec);
	if(p_data_rec) { // @v11.0.3 @fix
		const Data * p_cache_rec = static_cast<const Data *>(pEntry);
		memzero(p_data_rec, sizeof(*p_data_rec));
		p_data_rec->Tag   = PPOBJ_PERSONKIND;
		p_data_rec->ID    = p_cache_rec->ID;
		p_data_rec->CodeRegTypeID = p_cache_rec->CodeRegTypeID;
		p_data_rec->FolderRegTypeID = p_cache_rec->FolderRegTypeID;
		p_data_rec->DefStatusID = p_cache_rec->DefStatusID;
		p_data_rec->Flags = p_cache_rec->Flags;

		MultTextBlock b(this, pEntry);
		b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
		b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
	}
}
// }

int PPObjPersonKind::Fetch(PPID id, PPPersonKind * pRec)
{
	PersonKindCache * p_cache = GetDbLocalCachePtr <PersonKindCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
//
//
//
int FASTCALL GetPersonName(PPID id, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(id) {
		PPObjPerson psn_obj(SConstructorLite);
		PersonTbl::Rec rec;
		if(psn_obj.Fetch(id, &rec) > 0) {
			rBuf = rec.Name;
			ok = 1;
		}
		else
			ideqvalstr(id, rBuf);
	}
	return ok;
}
//
//
//
PPNewContragentDetectionBlock::PPNewContragentDetectionBlock() : P_PeObj(0), P_ArObj(0), P_ScObj(0), State(0)
{
	PPObjPerson::ReadConfig(&PsnCfg);
}

PPNewContragentDetectionBlock::~PPNewContragentDetectionBlock()
{
	delete P_ArObj;
	delete P_PeObj;
	delete P_ScObj;
}

int PPNewContragentDetectionBlock::InitProcess()
{
	int    ok = 1;
	if(!(State & stPrcInited)) {
		OpList.clear();
		AcsList.clear();
		PsnOpList.clear();
		ScOpList.clear();
		if(PsnCfg.NewClientDetectionList.getCount()) {
			for(uint i = 0; i < PsnCfg.NewClientDetectionList.getCount(); i++) {
				const PPPersonConfig::NewClientDetectionItem & r_item = PsnCfg.NewClientDetectionList.at(i);
				if(r_item.Oi.Obj == PPOBJ_OPRKIND) {
					if(IsGenericOp(r_item.Oi.Id) > 0)
						GetGenericOpList(r_item.Oi.Id, &OpList);
					else
						OpList.add(r_item.Oi.Id);
				}
				else if(r_item.Oi.Obj == PPOBJ_PERSONOPKIND)
					PsnOpList.add(r_item.Oi.Id);
				else if(r_item.Oi.Obj == PPOBJ_SCARDSERIES)
					ScOpList.add(r_item.Oi.Id);
			}
			OpList.sortAndUndup();
			{
				for(uint i = 0; i < OpList.getCount(); i++) {
					const PPID op_id = OpList.get(i);
					PPOprKind  op_rec;
					if(GetOpData(op_id, &op_rec) > 0 && op_rec.AccSheetID)
						AcsList.add(op_rec.AccSheetID);
				}
				AcsList.sortAndUndup();
			}
			PsnOpList.sortAndUndup();
			ScOpList.sortAndUndup();
		}
		if(!AcsList.getCount() && !PsnOpList.getCount() && !ScOpList.getCount())
			State |= stEmpty;
		else {
			if(AcsList.getCount()) {
				SETIFZ(P_ArObj, new PPObjArticle);
			}
			if(PsnOpList.getCount()) {
				SETIFZ(P_PeObj, new PPObjPersonEvent);
			}
			if(ScOpList.getCount()) {
				SETIFZ(P_ScObj, new PPObjSCard);
			}
		}
		State |= stPrcInited;
	}
	else
		ok = -1;
	return ok;
}

int PPNewContragentDetectionBlock::IsNewPerson(PPID psnID, const DateRange & rPeriod)
{
	int    yes = -1;
	if(rPeriod.low && psnID && InitProcess() && !(State & stEmpty)) {
		DateRange _period = rPeriod;
		SETIFZ(_period.upp, MAXDATE);
		uint   i;
		LAssocArray ar_op_list;
		{
			PPIDArray ar_list;
			PPOprKind op_rec;
			for(i = 0; i < AcsList.getCount(); i++) {
				const  PPID acs_id = AcsList.get(i);
				PPID   ar_id = 0;
				if(P_ArObj->P_Tbl->PersonToArticle(psnID, acs_id, &ar_id) > 0)
					ar_list.add(ar_id);
				else
					ar_list.add(0L);
			}
			assert(ar_list.getCount()  == AcsList.getCount());
			for(i = 0; i < OpList.getCount(); i++) {
				const PPID op_id = OpList.get(i);
				if(GetOpData(op_id, &op_rec) && op_rec.AccSheetID) {
					uint acs_pos = 0;
					if(AcsList.lsearch(op_rec.AccSheetID, &acs_pos))
						ar_op_list.Add(ar_list.get(acs_pos), op_id, 0);
				}
			}
			ar_op_list.Sort();
		}
		if(ar_op_list.getCount()) {
			BillCore * t = BillObj->P_Tbl;
			PPID   prev_ar_id = 0;
			for(i = 0; yes != 0 && i < ar_op_list.getCount(); i++) {
				// Object, Dt, BillNo (unique mod);                               // #3
				const PPID ar_id = ar_op_list.at(i).Key;
				if(ar_id && (!prev_ar_id || ar_id != prev_ar_id)) {
					BillTbl::Key3 k3;
					MEMSZERO(k3);
					k3.Object = ar_id;
					if(t->search(3, &k3, spGt) && t->data.Object == ar_id && t->data.Dt <= _period.upp) do {
						const PPID op_id = t->data.OpID;
						if(ar_op_list.SearchPair(ar_id, op_id, 0)) {
							yes = (t->data.Dt < _period.low) ? 0/*Не новый клиент*/ : 1/*Новый клиент*/;
							break;
						}
					} while(t->search(3, &k3, spNext) && t->data.Object == ar_id && t->data.Dt <= _period.upp);
					prev_ar_id = ar_id;
				}
			}
		}
		if(yes != 0 && PsnOpList.getCount()) {
			assert(P_PeObj);
			{
				//PersonID, Dt, OprNo (unique mod);   // #3
				PersonEventCore * t = P_PeObj->P_Tbl;
				PersonEventTbl::Key3 k3;
				MEMSZERO(k3);
				k3.PersonID = psnID;
				if(t->search(3, &k3, spGe) && t->data.PersonID == psnID && t->data.Dt <= _period.upp) do {
					if(PsnOpList.bsearch(t->data.OpID)) {
						yes = (t->data.Dt < _period.low) ? 0/*Не новый клиент*/ : 1/*Новый клиент*/;
						break;
					}
				} while(t->search(3, &k3, spNext) && t->data.PersonID == psnID && t->data.Dt <= _period.upp);
			}
		}
		if(yes != 0 && ScOpList.getCount()) {
			assert(P_ScObj);
			PPIDArray sc_list;
			PPIDArray temp_list;
			for(i = 0; i < ScOpList.getCount(); i++) {
				const PPID scs_id = ScOpList.get(i);
				P_ScObj->P_Tbl->GetListByPerson(psnID, scs_id, &sc_list);
				if(scs_id == 0)
					break;
			}
			sc_list.sortAndUndup();
			for(i = 0; i < sc_list.getCount(); i++) {
				const PPID sc_id = sc_list.get(i);
				{
					SCardOpTbl::Rec sco_rec;
					for(LDATETIME dtm = ZERODATETIME; P_ScObj->P_Tbl->EnumOpByCard(sc_id, &dtm, &sco_rec) > 0;) {
						const LDATE _dt = sco_rec.Dt;
						if(checkdate(_dt)) {
							if(_dt < _period.low) {
								yes = 0;
							}
							else if(_period.CheckDate(_dt)) {
								yes = 1;
							}
							break;
						}
					}
				}
				if(yes != 0) {
					CCheckCore * p_cc = P_ScObj->P_CcTbl;
					CCheckTbl::Key4 k4;
					MEMSZERO(k4);
					k4.SCardID = sc_id;
					if(p_cc->search(4, &k4, spGe) && p_cc->data.SCardID == sc_id) do {
						const LDATE _dt = p_cc->data.Dt;
						if(checkdate(_dt)) {
							if(_dt < _period.low) {
								yes = 0;
							}
							else if(_period.CheckDate(_dt)) {
								yes = 1;
							}
							break;
						}
					} while(p_cc->search(4, &k4, spNext) && p_cc->data.SCardID == sc_id);
				}
			}
		}
	}
	if(yes < 0)
		yes = 0;
	/*CATCH
		yes = 0;
	ENDCATCH*/
	return yes;
}

int PPNewContragentDetectionBlock::IsNewArticle(PPID arID, const DateRange & rPeriod)
{
	const PPID psn_id = ObjectToPerson(arID, 0);
	return IsNewPerson(ObjectToPerson(arID, 0), rPeriod);
}
//
// Implementation of PPALDD_PersonAttributes
//
PPALDD_CONSTRUCTOR(PersonReq)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjPerson;
	}
}

PPALDD_DESTRUCTOR(PersonReq)
{
	Destroy();
	delete static_cast<PPObjPerson *>(Extra[0].Ptr);
}

int PPALDD_PersonReq::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjPerson pobj;
		PersonTbl::Rec rec;
		if(pobj.Search(rFilt.ID, &rec) > 0) {
			PersonReq pr;
			pobj.GetPersonReq(rFilt.ID, &pr);
			H.ID = H.PersonID = rec.ID;
			H.BankID = pr.BnkAcct.Bnk.ID;
			STRNSCPY(H.Name,    rec.Name);
			STRNSCPY(H.ExtName, pr.ExtName);
			// @v11.1.12 STRNSCPY(H.Memo,    rec.Memo);
			STRNSCPY(H.Memo,    pr.Memo); // @v11.1.12
			pobj.FormatRegister(rFilt.ID, PPREGT_PASSPORT, H.Passport, sizeof(H.Passport));
			STRNSCPY(H.Address,  pr.Addr);
			STRNSCPY(H.RAddr,    pr.RAddr);
			STRNSCPY(H.Phone,    pr.Phone1);
			STRNSCPY(H.INN,      pr.TPID);
			STRNSCPY(H.KPP,      pr.KPP);
			STRNSCPY(H.OKONF,    pr.OKONF);
			STRNSCPY(H.OKPO,     pr.OKPO);
			STRNSCPY(H.SrchCode, pr.SrchCode);
			H.SrchRegTypeID = pr.SrchRegTypeID;
			STRNSCPY(H.BankCity, pr.BnkAcct.Bnk.City);
			pr.BnkAcct.Format(0, H.BankAccount, sizeof(H.BankAccount));
			H.Flags = pr.Flags;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_BankAccount
//
PPALDD_CONSTRUCTOR(BankAccount)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new PPObjRegister;
}

PPALDD_DESTRUCTOR(BankAccount)
{
	Destroy();
	delete static_cast<PPObjRegister *>(Extra[0].Ptr);
}

int PPALDD_BankAccount::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjRegister * p_obj = static_cast<PPObjRegister *>(Extra[0].Ptr);
		PPBankAccount rec;
		if(p_obj->Search(rFilt.ID, &rec) > 0) {
			H.ID       = rec.ID;
			H.PersonID = rec.PersonID;
			H.BankID   = rec.BankID;
			H.AccType  = rec.AccType;
			H.Flags    = rec.Flags;
			H.fPreferable = BIN(rec.Flags & PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/);
			STRNSCPY(H.Code, rec.Acct);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Person
//
PPALDD_CONSTRUCTOR(Person)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjPerson;
	}
}

PPALDD_DESTRUCTOR(Person)
{
	Destroy();
	delete static_cast<PPObjPerson *>(Extra[0].Ptr);
}

int PPALDD_Person::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjPerson * p_obj = static_cast<PPObjPerson *>(Extra[0].Ptr);
		PersonTbl::Rec rec;
		RegisterTbl::Rec reg_rec;
		if(p_obj->Search(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			H.ID = H.ReqID = rec.ID;
			H.LocID    = rec.MainLoc;
			H.RLocID   = rec.RLoc;
			H.Status   = rec.Status;
			H.Category = rec.CatID;
			STRNSCPY(H.Name, rec.Name);
			{
				// @v11.1.12 STRNSCPY(H.Memo, rec.Memo);
				p_obj->P_Tbl->GetItemMemo(rFilt.ID, temp_buf); // @v11.1.12
				STRNSCPY(H.Memo, temp_buf); // @v11.1.12
			}
			{
				const PPPersonConfig & r_cfg = p_obj->GetConfig();
				if(r_cfg.TradeLicRegTypeID && p_obj->GetRegister(rec.ID, r_cfg.TradeLicRegTypeID, &reg_rec) > 0)
					H.TradeLicID = reg_rec.ID;
			}
			H.Flags = rec.Flags;
			{
				PPObjWorld::GetNativeCountryName(temp_buf);
				H.fNativeLand = BIN((rec.Status == PPPRS_COUNTRY) && temp_buf.CmpNC(rec.Name) == 0);
			}
			H.fHasImage = BIN(rec.Flags & PSNF_HASIMAGES);
			H.fNoVat    = BIN(rec.Flags & PSNF_NOVATAX);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

static PPID Helper_DL600_GetRegister(PPID psnID, const char * pRegSymb, void * pExtra, int byDate, LDATE actualDate, RegisterTbl::Rec * pRegRec)
{
	PPID   reg_type_id = 0;
	PPObjPerson * p_obj = static_cast<PPObjPerson *>(pExtra);
	if(p_obj && PPObjRegisterType::GetByCode(pRegSymb, &reg_type_id) > 0) {
		RegisterTbl::Rec reg_rec;
		int r = byDate ? p_obj->GetRegister(psnID, reg_type_id, actualDate, &reg_rec) : p_obj->GetRegister(psnID, reg_type_id, &reg_rec);
		if(r > 0) {
			ASSIGN_PTR(pRegRec, reg_rec)
		}
		else
			reg_type_id = 0;
	}
	return reg_type_id;
}

void PPALDD_Person::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_LONG(n) (*static_cast<const long *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_DATE(n) (*static_cast<const LDATE *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_LONG    (*static_cast<long *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_DATE    (*static_cast<LDATE *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	enum {
		tNone = 0,
		tStr = 1,
		tInt,
		tReal,
		tDate
	} tag_type;
	SString temp_buf;
	SString tag_symb;
	tag_type = tNone;
	if(pF->Name == "?GetRegister") {
		RegisterTbl::Rec reg_rec;
		_RET_INT = Helper_DL600_GetRegister(H.ID, _ARG_STR(1), Extra[0].Ptr, 0, ZERODATE, &reg_rec) ? reg_rec.ID : 0;
	}
	else if(pF->Name == "?GetRegisterD") {
		RegisterTbl::Rec reg_rec;
		_RET_INT = Helper_DL600_GetRegister(H.ID, _ARG_STR(1), Extra[0].Ptr, 1, _ARG_DATE(2), &reg_rec) ? reg_rec.ID : 0;
	}
	if(pF->Name == "?FormatRegister") {
		_RET_STR.Z();
		RegisterTbl::Rec reg_rec;
		PPID   reg_type_id = Helper_DL600_GetRegister(H.ID, _ARG_STR(1), Extra[0].Ptr, 0, ZERODATE, &reg_rec);
		if(reg_type_id) {
			PPObjRegisterType rt_obj;
			if(rt_obj.GetFormat(reg_type_id, temp_buf) > 0 && temp_buf.NotEmptyS())
				PPObjRegister::Format(reg_rec, temp_buf, _RET_STR);
		}
	}
	if(pF->Name == "?FormatRegisterD") {
		_RET_STR.Z();
		RegisterTbl::Rec reg_rec;
		PPID   reg_type_id = Helper_DL600_GetRegister(H.ID, _ARG_STR(1), Extra[0].Ptr, 1, _ARG_DATE(2), &reg_rec);
		if(reg_type_id) {
			PPObjRegisterType rt_obj;
			if(rt_obj.GetFormat(reg_type_id, temp_buf) > 0 && temp_buf.NotEmptyS())
				PPObjRegister::Format(reg_rec, temp_buf, _RET_STR);
		}
	}
	else if(pF->Name == "?GetBankAccount") {
		_RET_INT = 0;
		PPObjPerson * p_obj = static_cast<PPObjPerson *>(Extra[0].Ptr);
		if(p_obj) {
			PPID   ba_id = 0;
			PPBankAccount ba_rec;
			if(p_obj->GetSingleBnkAcct(H.ID, _ARG_LONG(1), &ba_id, &ba_rec) > 0)
				_RET_INT = ba_id;
		}
	}
	else if(pF->Name == "?GetSingleRelation") {
		_RET_INT = 0;
		PPObjPerson * p_obj = static_cast<PPObjPerson *>(Extra[0].Ptr);
		if(p_obj) {
			PPObjPersonRelType prt_obj;
			PPID   prt_id = 0;
			if(prt_obj.SearchSymb(&prt_id, _ARG_STR(1)) > 0) {
				PPIDArray rel_list;
				if(p_obj->GetRelPersonList(H.ID, prt_id, 0, &rel_list) > 0 && rel_list.getCount()) {
					_RET_INT = rel_list.get(0);
				}
			}
		}
	}
	else if(pF->Name == "?GetSingleEmail") {
		_RET_STR.Z();
		PPObjPerson * p_obj = static_cast<PPObjPerson *>(Extra[0].Ptr);
		if(p_obj) {
			PPELinkArray ela;
			p_obj->P_Tbl->GetELinks(H.ID, ela);
			if(ela.getCount()) {
				StringSet ss;
				if(ela.GetListByType(ELNKRT_EMAIL, ss) > 0)
                    ss.get(0U, _RET_STR);
			}
		}
	}
	// @v10.4.0 {
	else if(pF->Name == "?GetExtName") {
		temp_buf.Z();
		PPObjPerson * p_obj = static_cast<PPObjPerson *>(Extra[0].Ptr);
		if(p_obj) {
			p_obj->GetExtName(H.ID, temp_buf);
			if(temp_buf.IsEmpty()) {
				PersonTbl::Rec psn_rec;
				if(p_obj->Fetch(H.ID, &psn_rec) > 0)
					temp_buf = psn_rec.Name;
			}
		}
		_RET_STR = temp_buf;
	}
	// } @v10.4.0
	else if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_PERSON, H.ID, _ARG_STR(1));
	}
	else if(pF->Name == "?GetTagStr") {
		tag_symb = _ARG_STR(1);
		tag_type = tStr;
		_RET_STR.Z();
	}
	else if(pF->Name == "?GetTagInt") {
		tag_symb = _ARG_STR(1);
		tag_type = tInt;
		_RET_LONG = 0;
	}
	else if(pF->Name == "?GetTagReal") {
		tag_symb = _ARG_STR(1);
		tag_type = tReal;
		_RET_DBL = 0.0;
	}
	else if(pF->Name == "?GetTagDate") {
		tag_symb = _ARG_STR(1);
		tag_type = tDate;
		_RET_DATE = ZERODATE;
	}
	if(tag_type) {
		PPObjTag tag_obj;
		PPID   tag_id = 0;
		if(tag_obj.SearchBySymb(tag_symb, &tag_id, 0) > 0) {
			ObjTagItem tag_item;
			if(PPRef->Ot.GetTag(PPOBJ_PERSON, H.ID, tag_id, &tag_item) > 0) {
				if(tag_type == tStr) {
					tag_item.GetStr(_RET_STR);
				}
				else if(tag_type == tInt) {
					long i_val = 0;
					tag_item.GetInt(&i_val);
					_RET_LONG = i_val;
				}
				else if(tag_type == tReal) {
					double r_val = 0.0;
					tag_item.GetReal(&r_val);
					_RET_DBL = r_val;
				}
				else if(tag_type == tDate) {
					LDATE d_val = ZERODATE;
					tag_item.GetDate(&d_val);
					_RET_DATE = d_val;
				}
			}
		}
	}
}
//
// Implementation of PPALDD_UhttPerson
//
struct UhttPersonBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttPersonBlock()
	{
		Clear();
	}
	void Clear()
	{
		Pack.destroy();
		LocPos = 0;
		PhPos = 0;
		EmlPos = 0;
		UrlPos = 0;
		State = stFetch;
	}
	PPObjPerson PObj;
	PPObjLocation LObj;
	PPObjELinkKind ElObj;
	PPPersonPacket Pack;
	uint   LocPos;  // Итератор локаций. 0 - MainLoc, 1 - RLob, 2.. - DlvrLoc
	uint   PhPos;
	uint   EmlPos;
	uint   UrlPos;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttPerson)
{
	if(Valid) {
		Extra[0].Ptr = new UhttPersonBlock;
		InitFixData(rscDefHdr, &H, sizeof(H));
		InitFixData("iter@KindList", &I_KindList, sizeof(I_KindList));
		InitFixData("iter@PhoneList", &I_PhoneList, sizeof(I_PhoneList));
		InitFixData("iter@EMailList", &I_EMailList, sizeof(I_EMailList));
		InitFixData("iter@UrlList", &I_UrlList, sizeof(I_UrlList));
		InitFixData("iter@RegisterList", &I_RegisterList, sizeof(I_RegisterList));
		InitFixData("iter@AddrList", &I_AddrList, sizeof(I_AddrList));
	}
}

PPALDD_DESTRUCTOR(UhttPerson)
{
	Destroy();
	delete static_cast<UhttPersonBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttPerson::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttPersonBlock & r_blk = *static_cast<UhttPersonBlock *>(Extra[0].Ptr);
	r_blk.Clear();
	MEMSZERO(H);
	if(r_blk.PObj.GetPacket(rFilt.ID, &r_blk.Pack, 0) > 0) {
		SString temp_buf;
		H.ID = r_blk.Pack.Rec.ID;
		H.PsnID = H.ID;
		STRNSCPY(H.Name, r_blk.Pack.Rec.Name);
		// @v11.1.12 STRNSCPY(H.Memo, r_blk.Pack.Rec.Memo);
		STRNSCPY(H.Memo, r_blk.Pack.SMemo); // @v11.1.12
		H.CategoryID = r_blk.Pack.Rec.CatID;
		H.StatusID = r_blk.Pack.Rec.Status;
		r_blk.Pack.GetSrchRegNumber(0, temp_buf);
		temp_buf.CopyTo(H.Code, sizeof(H.Code));
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttPerson::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttPersonBlock & r_blk = *static_cast<UhttPersonBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@KindList"))
		r_blk.Pack.Kinds.setPointer(0);
	else if(iterId == GetIterID("iter@PhoneList"))
		r_blk.PhPos = 0;
	else if(iterId == GetIterID("iter@EMailList"))
		r_blk.EmlPos = 0;
	else if(iterId == GetIterID("iter@UrlList"))
		r_blk.UrlPos = 0;
	else if(iterId == GetIterID("iter@RegisterList"))
		r_blk.Pack.Regs.setPointer(0);
	else if(iterId == GetIterID("iter@AddrList"))
		r_blk.LocPos = 0;
	return -1;
}

int PPALDD_UhttPerson::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	UhttPersonBlock & r_blk = *static_cast<UhttPersonBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@KindList")) {
		if(r_blk.Pack.Kinds.getPointer() < r_blk.Pack.Kinds.getCount()) {
			I_KindList.KindID = r_blk.Pack.Kinds.at(r_blk.Pack.Kinds.getPointer());
			PPObjPersonKind pk_obj;
			PPPersonKind pk_rec;
			if(pk_obj.Fetch(I_KindList.KindID, &pk_rec) > 0) {
				STRNSCPY(I_KindList.Code, pk_rec.Symb);
				STRNSCPY(I_KindList.Name, pk_rec.Name);
			}
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.Pack.Kinds.incPointer();
	}
	else if(iterId == GetIterID("iter@PhoneList")) {
		PPELinkKind el_rec;
		while(r_blk.PhPos < r_blk.Pack.ELA.getCount()) {
			const PPELink & r_link = r_blk.Pack.ELA.at(r_blk.PhPos++);
			if(r_blk.ElObj.Fetch(r_link.KindID, &el_rec) > 0 && el_rec.Type == ELNKRT_PHONE) {
				STRNSCPY(I_PhoneList.Code, r_link.Addr);
				ok = DlRtm::NextIteration(iterId);
				break;
			}
		}
	}
	else if(iterId == GetIterID("iter@EMailList")) {
		PPELinkKind el_rec;
		while(r_blk.EmlPos < r_blk.Pack.ELA.getCount()) {
			const PPELink & r_link = r_blk.Pack.ELA.at(r_blk.EmlPos++);
			if(r_blk.ElObj.Fetch(r_link.KindID, &el_rec) > 0 && el_rec.Type == ELNKRT_EMAIL) {
				STRNSCPY(I_EMailList.Code, r_link.Addr);
				ok = DlRtm::NextIteration(iterId);
				break;
			}
		}
	}
	else if(iterId == GetIterID("iter@UrlList")) {
		PPELinkKind el_rec;
		while(r_blk.UrlPos < r_blk.Pack.ELA.getCount()) {
			const PPELink & r_link = r_blk.Pack.ELA.at(r_blk.UrlPos++);
			if(r_blk.ElObj.Fetch(r_link.KindID, &el_rec) > 0 && el_rec.Type == ELNKRT_WEBADDR) {
				STRNSCPY(I_UrlList.Code, r_link.Addr);
				ok = DlRtm::NextIteration(iterId);
				break;
			}
		}
	}
	else if(iterId == GetIterID("iter@RegisterList")) {
		const uint _p = r_blk.Pack.Regs.getPointer();
		if(_p < r_blk.Pack.Regs.getCount()) {
			const RegisterTbl::Rec & r_reg_rec = r_blk.Pack.Regs.at(_p);
			I_RegisterList.RegID = r_reg_rec.ID;
			I_RegisterList.RegTypeID = r_reg_rec.RegTypeID;
			I_RegisterList.PersonID = r_reg_rec.ObjID;
			I_RegisterList.RegOrgID = r_reg_rec.RegOrgID;
			I_RegisterList.RegDt = r_reg_rec.Dt;
			I_RegisterList.RegExpiry = r_reg_rec.Expiry;
			STRNSCPY(I_RegisterList.RegSerial, r_reg_rec.Serial);
			STRNSCPY(I_RegisterList.RegNumber, r_reg_rec.Num);
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.Pack.Regs.incPointer();
	}
	else if(iterId == GetIterID("iter@AddrList")) {
		SString temp_buf;
		while(r_blk.LocPos < (2+r_blk.Pack.GetDlvrLocCount())) {
			uint temp_pos = r_blk.LocPos++;
			MEMSZERO(I_AddrList);
			if(temp_pos == 0) {
				if(r_blk.Pack.Loc.ID) {
					I_AddrList.LocID = r_blk.Pack.Loc.ID;
					I_AddrList.CityID = r_blk.Pack.Loc.CityID;
					STRNSCPY(I_AddrList.LocCode, r_blk.Pack.Loc.Code);
					STRNSCPY(I_AddrList.LocName, r_blk.Pack.Loc.Name);

					LocationCore::GetExField(&r_blk.Pack.Loc, LOCEXSTR_ZIP, temp_buf);
					STRNSCPY(I_AddrList.ZIP, temp_buf);
					LocationCore::GetExField(&r_blk.Pack.Loc, LOCEXSTR_SHORTADDR, temp_buf);
					STRNSCPY(I_AddrList.Address, temp_buf);

					I_AddrList.Longitude = r_blk.Pack.Loc.Longitude;
					I_AddrList.Latitude = r_blk.Pack.Loc.Latitude;
					I_AddrList.LocKind = 1;
					ok = DlRtm::NextIteration(iterId);
					break;
				}
			}
			else if(temp_pos == 1) {
				if(r_blk.Pack.RLoc.ID) {
					I_AddrList.LocID = r_blk.Pack.Loc.ID;
					I_AddrList.CityID = r_blk.Pack.Loc.CityID;
					STRNSCPY(I_AddrList.LocCode, r_blk.Pack.Loc.Code);
					STRNSCPY(I_AddrList.LocName, r_blk.Pack.Loc.Name);

					LocationCore::GetExField(&r_blk.Pack.Loc, LOCEXSTR_ZIP, temp_buf);
					STRNSCPY(I_AddrList.ZIP, temp_buf);
					LocationCore::GetExField(&r_blk.Pack.Loc, LOCEXSTR_SHORTADDR, temp_buf);
					STRNSCPY(I_AddrList.Address, temp_buf);

					I_AddrList.Longitude = r_blk.Pack.Loc.Longitude;
					I_AddrList.Latitude = r_blk.Pack.Loc.Latitude;
					I_AddrList.LocKind = 2;
					ok = DlRtm::NextIteration(iterId);
					break;
				}
			}
			else {
				PPLocationPacket loc_pack;
				if(r_blk.Pack.GetDlvrLocByPos(temp_pos-2, &loc_pack)) {
					I_AddrList.LocID = loc_pack.ID;
					I_AddrList.CityID = loc_pack.CityID;
					STRNSCPY(I_AddrList.LocCode, loc_pack.Code);
					STRNSCPY(I_AddrList.LocName, loc_pack.Name);

					LocationCore::GetExField(&loc_pack, LOCEXSTR_ZIP, temp_buf);
					STRNSCPY(I_AddrList.ZIP, temp_buf);
					LocationCore::GetExField(&loc_pack, LOCEXSTR_SHORTADDR, temp_buf);
					STRNSCPY(I_AddrList.Address, temp_buf);

					I_AddrList.Longitude = loc_pack.Longitude;
					I_AddrList.Latitude = loc_pack.Latitude;
					I_AddrList.LocKind = 3;
					ok = DlRtm::NextIteration(iterId);
				}
				break;
			}
		}
	}
	return ok;
}

// @Muxa {
int PPALDD_UhttPerson::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttPersonBlock & r_blk = *static_cast<UhttPersonBlock *>(Extra[0].Ptr);
	if(r_blk.State != UhttPersonBlock::stSet) {
		r_blk.Clear();
		r_blk.State = UhttPersonBlock::stSet;
	}
	if(commit == 0) {
		SString temp_buf;
		if(iterId == 0) {
			r_blk.Pack.Rec.ID = H.ID;
			temp_buf = strip(H.Name);
			STRNSCPY(r_blk.Pack.Rec.Name, temp_buf);
			r_blk.Pack.Rec.CatID = H.CategoryID;
			r_blk.Pack.Rec.Status = H.StatusID ? H.StatusID : PPPRS_PRIVATE;
		}
		else {
			if(iterId == GetIterID("iter@KindList")) {
				if(I_KindList.KindID)
					r_blk.Pack.Kinds.add(I_KindList.KindID);
				else if(I_KindList.Code[0]) {
                    PPObjPersonKind pk_obj;
                    PPID   pk_id = 0;
                    if(pk_obj.SearchBySymb(I_KindList.Code, &pk_id, 0) > 0) {
						r_blk.Pack.Kinds.add(pk_id);
                    }
				}
			}
			else if(iterId == GetIterID("iter@RegisterList")) {
				RegisterTbl::Rec reg_r;
				MEMSZERO(reg_r);
				reg_r.RegTypeID = I_RegisterList.RegTypeID;
				STRNSCPY(reg_r.Num, strip(I_RegisterList.RegNumber));
				r_blk.Pack.Regs.atInsert(r_blk.Pack.Regs.getPointer(), &reg_r);
				r_blk.Pack.Regs.incPointer();
			}
			else if(iterId == GetIterID("iter@PhoneList")) {
				temp_buf = I_PhoneList.Code;
				if(temp_buf.NotEmptyS())
					r_blk.Pack.ELA.AddItem(PPELK_WORKPHONE, temp_buf);
			}
			else if(iterId == GetIterID("iter@EMailList")) {
				temp_buf = I_EMailList.Code;
				if(temp_buf.NotEmptyS())
					r_blk.Pack.ELA.AddItem(PPELK_EMAIL, temp_buf);
			}
			else if(iterId == GetIterID("iter@AddrList")) {
				if(I_AddrList.LocKind == 1) {
					r_blk.Pack.Loc.Type = LOCTYP_ADDRESS;
					r_blk.Pack.Loc.CityID = I_AddrList.CityID;
					STRNSCPY(r_blk.Pack.Loc.Code, strip(I_AddrList.LocCode));
					if(isempty(r_blk.Pack.Loc.Code))
						r_blk.PObj.LocObj.InitCode(&r_blk.Pack.Loc);
					STRNSCPY(r_blk.Pack.Loc.Name, strip(I_AddrList.LocName));
					r_blk.Pack.Loc.Longitude = I_AddrList.Longitude;
					r_blk.Pack.Loc.Latitude = I_AddrList.Latitude;
					LocationCore::SetExField(&r_blk.Pack.Loc, LOCEXSTR_ZIP, I_AddrList.ZIP);
					LocationCore::SetExField(&r_blk.Pack.Loc, LOCEXSTR_SHORTADDR, I_AddrList.Address);
				}
				else if(I_AddrList.LocKind == 3) {
					PPLocationPacket loc_pack;
					if(I_AddrList.LocID > 0)
						r_blk.LObj.GetPacket(I_AddrList.LocID, &loc_pack);
					loc_pack.ID = I_AddrList.LocID;
					loc_pack.Type = LOCTYP_ADDRESS;
					loc_pack.CityID = I_AddrList.CityID;
					STRNSCPY(loc_pack.Code, strip(I_AddrList.LocCode));
					if(isempty(loc_pack.Code))
						r_blk.PObj.LocObj.InitCode(&loc_pack);
					STRNSCPY(loc_pack.Name, strip(I_AddrList.LocName));
					loc_pack.Longitude = I_AddrList.Longitude;
					loc_pack.Latitude = I_AddrList.Latitude;
					LocationCore::SetExField(&loc_pack, LOCEXSTR_ZIP, I_AddrList.ZIP);
					LocationCore::SetExField(&loc_pack, LOCEXSTR_SHORTADDR, I_AddrList.Address);
					r_blk.Pack.AddDlvrLoc(loc_pack);
				}
			}
		}
	}
	else {
		PPObjGlobalUserAcc  gua_obj;
		PPID  id = r_blk.Pack.Rec.ID;
		if(id > 0) {
			PPPersonPacket  tmp_pckt;
			THROW(r_blk.PObj.GetPacket(id, &tmp_pckt, 0));
			LAssocArray rel_list = tmp_pckt.GetRelList();
			for(uint i = 0, n = rel_list.getCount(); i < n; i++) {
				r_blk.Pack.AddRelation(rel_list.at(i).Key, rel_list.at(i).Val, 0);
			}
		}
		if(r_blk.Pack.Kinds.getCount() == 0) {
			r_blk.Pack.Kinds.add(PPPRK_UNKNOWN);
		}
		THROW(r_blk.PObj.PutPacket(&id, &r_blk.Pack, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
// } @Muxa

//
// Implementation of PPALDD_Employee
//
PPALDD_CONSTRUCTOR(Employee)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjStaffList;
	}
}

PPALDD_DESTRUCTOR(Employee)
{
	Destroy();
	delete static_cast<PPObjStaffList *>(Extra[0].Ptr);
}

int PPALDD_Employee::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		PPObjStaffList * p_obj = static_cast<PPObjStaffList *>(Extra[0].Ptr);
		SString tab_num;
		PersonPostArray post_list;
		MEMSZERO(H);
		H.ID = H.PersonID = rFilt.ID;
		const PPID org_id = GetMainOrgID();
		if(p_obj->GetPostByPersonList(rFilt.ID, org_id, 1, &post_list) > 0)
			H.PsnPostID = post_list.at(0).ID;
		p_obj->PsnObj.RegObj.GetTabNumber(H.PersonID, tab_num);
		tab_num.CopyTo(H.TabNum, sizeof(H.TabNum));
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}
//
// Implementation of PPALDD_Global
//
PPALDD_CONSTRUCTOR(Global)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(Global) { Destroy(); }

int PPALDD_Global::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = 1;
	if(H.ID == 0) {
		MEMSZERO(H);
		H.ID = 1;
		PersonTbl::Rec psn_rec;
		PPObjPerson psnobj;
		PersonReq   req, bnk_req;
		SString temp_buf;
		H.ID = GetMainOrgID();
		H.BaseCurID = LConfig.BaseCurID;
		H.MainOrgID = H.ID;
		GetMainOrgName(temp_buf).CopyTo(H.MainOrgName, sizeof(H.MainOrgName));
		H.ShortOrgName[0] = 0;
		psnobj.GetPersonReq(H.ID, &req);
		H.MainAddrID = req.AddrID;
		H.MainRAddrID = NZOR(req.RAddrID, req.AddrID);
		STRNSCPY(H.MainOrgExtName, req.ExtName);
		STRNSCPY(H.MainOrgMemo,    req.Memo);
		{
			PPLoadString("checkingaccount_s", temp_buf);
			req.BnkAcct.Format(temp_buf, H.MainOrgBankAcc, sizeof(H.MainOrgBankAcc));
		}
		STRNSCPY(H.MainOrgBnkName, req.BnkAcct.Bnk.ExtName[0] ? req.BnkAcct.Bnk.ExtName : req.BnkAcct.Bnk.Name);
		STRNSCPY(H.MainOrgBnkAccN, req.BnkAcct.Acct);
		H.CurDate = getcurdate_();
		PPObjPerson::GetCurUserPerson(0, &temp_buf);
		temp_buf.CopyTo(H.Cashier, sizeof(H.Cashier));
		if(psnobj.Fetch(H.ID, &psn_rec) > 0)
			H.IsPrivateEnt = BIN(psn_rec.Status == PPPRS_FREE);
		{
			DS.GetTLA().InitMainOrgData(0);
			const PPCommConfig & r_ccfg = CConfig;
			if(psnobj.Fetch(r_ccfg.MainOrgDirector_, &psn_rec) > 0)
				STRNSCPY(H.Director, psn_rec.Name);
			if(psnobj.Fetch(r_ccfg.MainOrgAccountant_, &psn_rec) > 0)
				STRNSCPY(H.Accountant, psn_rec.Name);
		}
		STRNSCPY(H.INN,  req.TPID);
		STRNSCPY(H.KPP,  req.KPP);
		STRNSCPY(H.OKPO, req.OKPO);
		PPELinkArray el;
		STRNSCPY(H.Address, req.Addr);
		STRNSCPY(H.RAddress, req.RAddr);
		psnobj.P_Tbl->GetELinks(H.ID, el);
		el.GetPhones(1, temp_buf); temp_buf.CopyTo(H.Phone, sizeof(H.Phone));
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

void PPALDD_Global::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_DATE(n) (*static_cast<const LDATE *>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?GetFixedStaffPost") {
		PPID   person_id = 0;
		int    kind = _ARG_INT(1);
		LDATE  dt = _ARG_DATE(2);
		if(oneof2(kind, 1, 2)) {
			PPObjStaffList st_obj;
			PersonPostTbl::Rec post_rec;
			PPID   staff_id = 0;
			if(kind == 1)
				staff_id = PPFIXSTF_DIRECTOR;
			else if(kind == 2)
				staff_id = PPFIXSTF_ACCOUNTANT;
			const PPID   org_id = GetMainOrgID();
			st_obj.GetFixedPostOnDate(org_id, staff_id, dt, &post_rec);
			person_id = post_rec.PersonID;
		}
		_RET_INT = person_id;
	}
	else if(pF->Name == "?GetSupplAccSheet") {
		_RET_INT = GetSupplAccSheet();
	}
	else if(pF->Name == "?GetClientAccSheet") {
		_RET_INT = GetSellAccSheet();
	}
	else if(pF->Name == "?GetAccSheet") {
		PPObjAccSheet acs_obj;
		PPID   acs_id = 0;
		acs_obj.SearchBySymb(_ARG_STR(1), &acs_id, 0);
		_RET_INT = acs_id;
	}
	else if(pF->Name == "?PersonToArticle") {
		PPID   ar_id = 0;
		PPObjArticle ar_obj;
		ar_obj.P_Tbl->PersonToArticle(_ARG_INT(1), _ARG_INT(2), &ar_id);
		_RET_INT = ar_id;
	}
	else if(pF->Name == "?GetCurrentGlobalUser") {
		_RET_INT = DS.GetConstTLA().GlobAccID;
	}
	else if(pF->Name == "?CheckCurrentGlobalUserRights") {
		PPGlobalAccRights rights_blk(_ARG_INT(1));
		_RET_INT = rights_blk.IsAllow(_ARG_INT(2), _ARG_STR(3));
	}
	else if(pF->Name == "?CheckFlag") {
		int flags = _ARG_INT(1);
		int flag = _ARG_INT(2);
		_RET_INT = (flags & flag) ? 1 : 0;
	}
	else if(pF->Name == "?AdjustBarcodeCD") {
		SString code = _ARG_STR(1);
		if(code.Len() > 3) {
			PPObjGoods goods_obj;
			const PPGoodsConfig & r_gcfg = goods_obj.GetConfig();
			if(!(r_gcfg.Flags & GCF_BCCHKDIG) && !r_gcfg.IsWghtPrefix(code))
				AddBarcodeCheckDigit(code);
		}
		_RET_STR = code;
	}
	else if(pF->Name == "?GetCounter") {
		SString code = _ARG_STR(1);
        PPObjOpCounter opc_obj;
        PPOpCounter opc_rec;
        PPID   opc_id = 0;
        if(opc_obj.SearchBySymb(code.Strip(), &opc_id, &opc_rec) > 0)
			_RET_INT = opc_rec.Counter;
        else {
			_RET_INT = 0;
        }
	}
	else if(pF->Name == "?UpdateCounter") {
		int    result = 0;
		SString code = _ARG_STR(1);
		long   value = _ARG_INT(2);
        PPObjOpCounter opc_obj;
        PPOpCounter opc_rec;
        PPID   opc_id = 0;
        if(opc_obj.SearchBySymb(code.Strip(), &opc_id, &opc_rec) > 0) {
			if(opc_obj.UpdateCounter(opc_id, value, opc_rec.Flags, 0, 1))
				result = 1;
        }
        _RET_INT = result;
	}
	else if(pF->Name == "?GetAlcoRepConfig") {
		_RET_INT = 1;
	}
	else if(pF->Name == "?GetDbUUID") {
		S_GUID dbuuid;
		DbProvider * p_dict = CurDict;
		CALLPTRMEMB(p_dict, GetDbUUID(&dbuuid));
		dbuuid.ToStr(S_GUID::fmtIDL, _RET_STR);
	}
}

/*static*/int PPObjPerson::TestSearchEmail()
{
	int    ok = 1;
	SString email_buf;
	SString msg_buf, temp_buf;
	PPLogger logger;
	PPInputStringDialogParam param;
	PPIDArray psn_list;
	PPIDArray loc_list;
	PPObjPerson psn_obj;
	StringSet ss_email;
    while(InputStringDialog(&param, email_buf) > 0) {
		if(psn_obj.SearchEmail(email_buf, 0, &psn_list, &loc_list)) {
			if(psn_list.getCount() || loc_list.getCount()) {
				uint    i;
				for(i = 0; i < psn_list.getCount(); i++) {
					const PPID psn_id = psn_list.get(i);
					PersonTbl::Rec psn_rec;
					if(psn_obj.Search(psn_id, &psn_rec) > 0) {
                        msg_buf.Z().CatEq("PersonID", psn_id).CatDiv('-', 1).CatEq("Name", psn_rec.Name);
                        PPELinkArray ela;
						psn_obj.P_Tbl->GetELinks(psn_id, ela);
						ss_email.clear();
						ela.GetListByType(PPELK_EMAIL, ss_email);
						for(uint j = 0; ss_email.get(&j, temp_buf);) {
                            msg_buf.CatDiv('-', 1).Cat(temp_buf);
						}
                        logger.Log(msg_buf);
					}
					else {
						logger.LogLastError();
					}
				}
				for(i = 0; i < loc_list.getCount(); i++) {
					const PPID loc_id = loc_list.get(i);
					LocationTbl::Rec loc_rec;
					if(psn_obj.LocObj.Search(loc_id, &loc_rec) > 0) {
                        msg_buf.Z().CatEq("LocID", loc_id);
                        LocationCore::GetExField(&loc_rec, LOCEXSTR_EMAIL, temp_buf.Z());
                        msg_buf.CatDiv('-', 1).Cat(temp_buf);
                        logger.Log(msg_buf);
					}
					else
						logger.LogLastError();
				}
			}
			else {
				(msg_buf = "Persons or Locations by email").Space().Cat(email_buf).Space().Cat("not found");
				logger.Log(msg_buf);
			}
		}
		else
			logger.LogLastError();
    }
    return ok;
}
