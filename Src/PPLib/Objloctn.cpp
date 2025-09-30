// OBJLOCTN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage Windows-1251
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

class FiasAddrCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry {
		long   RecUuID;
		long   IdUuRef;
		long   ParentUuRef;
		long   PrevRecUuRef;
		long   NextRecUuRef;
		long   NameTRef;
		long   OfcNameTRef;
		long   SnTRef;
		long   Flags;
		int16  LevelStatus;
		int16  CenterStatus;
		int16  ActionStatus;
		int16  KladrCurStatus;
		long   PostalCode;
		long   KladrCodeTRef;
		LDATE  UpdateDt;
		LDATE  StartDt;
		LDATE  EndDt;
	};
	FiasAddrCache();
	~FiasAddrCache();
	int    GetAddrObjListByText(const char * pText, PPIDArray & rList);
	void   FASTCALL SetTable(PPFiasReference * pT);
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	struct TextEntry {
		TextEntry(PPID textRefID, const PPIDArray & rList) : TextRefID(textRefID), AddrList(rList)
		{
		}
        PPID   TextRefID;
        PPIDArray AddrList;
	};
	TSCollection <TextEntry> TextList;
	ReadWriteLock TxlLock;
	PPFiasReference * P_T; // @notowned
};

FiasAddrCache::FiasAddrCache() : ObjCacheHash(PPOBJ_FIAS, sizeof(Data), SMEGABYTE(16), 8), P_T(0)
{
}

FiasAddrCache::~FiasAddrCache()
{
}

void FASTCALL FiasAddrCache::SetTable(PPFiasReference * pT) { P_T = pT; }

int FiasAddrCache::GetAddrObjListByText(const char * pText, PPIDArray & rList)
{
	int    ok = -1;
	rList.clear();
	if(P_T) {
		PPID   text_ref_id = 0;
		uint   pos = 0;
		if(PPRef->TrT.FetchSelfRefText(pText, &text_ref_id) > 0) {
			SRWLOCKER(TxlLock, SReadWriteLocker::Read);
			if(TextList.lsearch(&text_ref_id, &(pos = 0), CMPF_LONG)) {
				rList = TextList.at(pos)->AddrList;
				ok = 1;
			}
			else {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				if(TextList.lsearch(&text_ref_id, &(pos = 0), CMPF_LONG)) { // ��������� ������� ����� ����������
					rList = TextList.at(pos)->AddrList;
					ok = 1;
				}
				else {
					TextRefIdent tri(PPOBJ_SELFREFTEXT, text_ref_id, PPTRPROP_DEFAULT);
					TSVector <TextRefIdent> text_ref_list;
					text_ref_list.insert(&tri);
					ok = P_T->SearchObjByTextRefList(text_ref_list, rList);
					if(ok > 0) {
						TextEntry * p_new_entry = new TextEntry(text_ref_id, rList);
						if(p_new_entry) {
							if(!TextList.insert(p_new_entry))
								ok = PPSetErrorSLib();
						}
						else
							ok = PPSetErrorNoMem();
					}
				}
			}
		}
	}
	return ok;
}

int FiasAddrCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	FiasAddrObjTbl::Rec data_rec;
	if(id && P_T && P_T->SearchObjByID(id, &data_rec, 0) > 0) {
		#define CPY(f) p_cache_rec->f = data_rec.f
		CPY(RecUuID);
		CPY(IdUuRef);
		CPY(ParentUuRef);
		CPY(PrevRecUuRef);
		CPY(NextRecUuRef);
		CPY(NameTRef);
		CPY(OfcNameTRef);
		CPY(SnTRef);
		CPY(Flags);
		CPY(LevelStatus);
		CPY(CenterStatus);
		CPY(ActionStatus);
		CPY(KladrCurStatus);
		CPY(PostalCode);
		CPY(KladrCodeTRef);
		CPY(UpdateDt);
		CPY(StartDt);
		CPY(EndDt);
		#undef CPY
	}
	else
		ok = -1;
	return ok;
}

void FiasAddrCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	FiasAddrObjTbl::Rec * p_data_rec = static_cast<FiasAddrObjTbl::Rec *>(pDataRec);
	if(p_data_rec) {
		const Data * p_cache_rec = static_cast<const Data *>(pEntry);
		memzero(p_data_rec, sizeof(*p_data_rec));
		#define CPY(f) p_data_rec->f = p_cache_rec->f
		CPY(RecUuID);
		CPY(IdUuRef);
		CPY(ParentUuRef);
		CPY(PrevRecUuRef);
		CPY(NextRecUuRef);
		CPY(NameTRef);
		CPY(OfcNameTRef);
		CPY(SnTRef);
		CPY(Flags);
		CPY(LevelStatus);
		CPY(CenterStatus);
		CPY(ActionStatus);
		CPY(KladrCurStatus);
		CPY(PostalCode);
		CPY(KladrCodeTRef);
		CPY(UpdateDt);
		CPY(StartDt);
		CPY(EndDt);
		#undef CPY
	}
}
//
//
//
TLP_IMPL(PPObjLocation, LocationCore, P_Tbl);
//
//
//
PPCountryBlock::PPCountryBlock() : IsNative(0)
{
}

PPCountryBlock & PPCountryBlock::Z()
{
	IsNative = 0;
	Name.Z();
	Code.Z();
	Abbr.Z();
	return *this;
}
//
//
//
PPLocationPacket::PPLocationPacket() : ObjTagContainerHelper(TagL, PPOBJ_LOCATION, PPTAG_LOC_UUID)
{
	memzero(static_cast<LocationTbl::Rec *>(this), sizeof(LocationTbl::Rec));
	TagL.Oid.Obj = PPOBJ_LOCATION;
}

PPLocationPacket::PPLocationPacket(const PPLocationPacket & rS) : ObjTagContainerHelper(rS)
{
	Copy(rS);
}

int FASTCALL PPLocationPacket::Copy(const PPLocationPacket & rS)
{
	*static_cast<LocationTbl::Rec *>(this) = *static_cast<const LocationTbl::Rec *>(&rS);
	Regs = rS.Regs;
	TagL = rS.TagL;
	WarehouseList = rS.WarehouseList; // @v11.0.6
	return 1;
}

void PPLocationPacket::destroy()
{
	memzero(static_cast<LocationTbl::Rec *>(this), sizeof(LocationTbl::Rec));
	Regs.freeAll();
	TagL.Z();
	TagL.Oid.Obj = PPOBJ_LOCATION;
	WarehouseList.Z();
}

PPLocationPacket & FASTCALL PPLocationPacket::operator = (const LocationTbl::Rec & rS)
{
	*static_cast<LocationTbl::Rec *>(this) = rS;
	return *this;
}

PPLocationPacket & FASTCALL PPLocationPacket::operator = (const PPLocationPacket & rS)
{
	Copy(rS);
	return *this;
}

int PPLocationPacket::IsEmptyAddress() const
{
	return LocationCore::IsEmptyAddressRec(*this);
}
//
//
//
PPLocationConfig::PPLocationConfig() : WhZoneCoding(0), WhColCoding(0), WhRowCoding(0), WhCellCoding(0), WhCodingDiv(0), Reserve(0),
	Flags(0), DefPalletID(0), StoreIdxTagID(0), MarketplaceWarehoustFolderID(0)
{
	AddrCodeTempl[0] = 0;
}

PPLocationConfig & PPLocationConfig::Z()
{
	WhZoneCoding = 0;
	WhColCoding = 0;
	WhRowCoding = 0;
	WhCellCoding = 0;
	WhCodingDiv = 0;
	Reserve = 0;
	Flags = 0;
	DefPalletID = 0;
	StoreIdxTagID = 0;
	MarketplaceWarehoustFolderID = 0;
	AddrCodeTempl[0] = 0;
	return *this;
}

struct Storage_PPLocationConfig {  // @persistent @store(PropertyTbl)
	Storage_PPLocationConfig()
	{
		THISZERO();
	}
	PPID   Tag;            // Const=PPOBJ_CONFIG
	PPID   ID;             // Const=PPCFG_MAIN
	PPID   Prop;           // Const=PPPRP_LOCATIONCFG
	int16  WhZoneCoding;
	int16  WhColCoding;
	int16  WhRowCoding;
	int16  WhCellCoding;
	int16  WhCodingDiv;    // ����������� ����� ��������� �������
	int16  Reserve;        // @alignment
	long   Flags;
	PPID   DefPalletID;    // ��� ������� �� ���������
	char   AddrCodeTempl[32]; // @v7.3.8
	PPID   StoreIdxTagID;  // @v8.9.11
	PPID   MarketplaceWarehoustFolderID; // @v12.2.1
	char   Reserve2[4];    // @v8.9.11 [12]-->[8] // @v12.2.1 [8]-->[4]
	long   Reserve3[2];
};

/*static*/int FASTCALL PPObjLocation::ReadConfig(PPLocationConfig * pCfg)
{
	const  long prop_cfg_id = PPPRP_LOCATIONCFG;
	int    ok = -1;
	int    r;
	Storage_PPLocationConfig cfg;
	pCfg->Z();
	THROW(r = PPRef->GetPropMainConfig(prop_cfg_id, &cfg, sizeof(cfg)));
	if(r > 0) {
		pCfg->WhZoneCoding = cfg.WhZoneCoding;
		pCfg->WhColCoding = cfg.WhColCoding;
		pCfg->WhRowCoding = cfg.WhRowCoding;
		pCfg->WhCellCoding = cfg.WhCellCoding;
		pCfg->WhCodingDiv = cfg.WhCodingDiv;
		pCfg->Flags = cfg.Flags;
		pCfg->Flags |= PPLocationConfig::fValid;
		pCfg->DefPalletID = cfg.DefPalletID;
		pCfg->StoreIdxTagID = cfg.StoreIdxTagID;
		pCfg->MarketplaceWarehoustFolderID = cfg.MarketplaceWarehoustFolderID; // @v12.2.1
		STRNSCPY(pCfg->AddrCodeTempl, cfg.AddrCodeTempl);
		ok = 1;
	}
	else {
		ok = -1;
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPObjLocation::WriteConfig(const PPLocationConfig * pCfg, int use_ta)
{
	const  long prop_cfg_id = PPPRP_LOCATIONCFG;
	const  long cfg_obj_type = PPCFGOBJ_LOCATION;
	int    ok = -1, r;
	int    is_new = 0;
	Storage_PPLocationConfig cfg;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = PPRef->GetPropMainConfig(prop_cfg_id, &cfg, sizeof(cfg)));
		is_new = (r > 0) ? 0 : 1;
		cfg.WhZoneCoding = pCfg->WhZoneCoding;
		cfg.WhColCoding = pCfg->WhColCoding;
		cfg.WhRowCoding = pCfg->WhRowCoding;
		cfg.WhCellCoding = pCfg->WhCellCoding;
		cfg.WhCodingDiv = pCfg->WhCodingDiv;
		cfg.Flags       = pCfg->Flags;
		cfg.Flags &= ~PPLocationConfig::fValid;
		cfg.DefPalletID = pCfg->DefPalletID;
		cfg.StoreIdxTagID = pCfg->StoreIdxTagID;
		cfg.MarketplaceWarehoustFolderID = pCfg->MarketplaceWarehoustFolderID; // @v12.2.1
		STRNSCPY(cfg.AddrCodeTempl, pCfg->AddrCodeTempl);
		THROW(PPObject::Helper_PutConfig(prop_cfg_id, cfg_obj_type, is_new, &cfg, sizeof(cfg), 0));
		THROW(tra.Commit());
	}
	DirtyConfig();
	CATCHZOK
	return ok;
}

/*static*/int PPObjLocation::EditConfig()
{
	int    ok = -1;
	PPLocationConfig cfg;
	PPLocationConfig org_cfg;
	TDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_PERSON, PPR_READ, 0));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_LOCCFG))));
	ReadConfig(&cfg);
	org_cfg = cfg;
	SetupStringCombo(dlg, CTLSEL_LOCCFG_WHZONECOD, PPTXT_WHCODINGITEM, cfg.WhZoneCoding);
	SetupStringCombo(dlg, CTLSEL_LOCCFG_WHCOLCOD,  PPTXT_WHCODINGITEM, cfg.WhColCoding);
	SetupStringCombo(dlg, CTLSEL_LOCCFG_WHCELLCOD, PPTXT_WHCODINGITEM, cfg.WhCellCoding);
	dlg->AddClusterAssocDef(CTL_LOCCFG_DIVCOD,  0, 0);
	dlg->AddClusterAssoc(CTL_LOCCFG_DIVCOD,  1, 1);
	dlg->AddClusterAssoc(CTL_LOCCFG_DIVCOD,  2, 2);
	dlg->SetClusterData(CTL_LOCCFG_DIVCOD, cfg.WhCodingDiv);
	SetupPPObjCombo(dlg, CTLSEL_LOCCFG_DEFPALTYPE, PPOBJ_PALLET, cfg.DefPalletID, 0, 0);
	dlg->setCtrlData(CTL_LOCCFG_ADDRCODETEMPL, cfg.AddrCodeTempl);
	{
		ObjTagFilt tag_filt(PPOBJ_LOCATION);
		SetupObjTagCombo(dlg, CTLSEL_LOCCFG_STRIDXTAG, cfg.StoreIdxTagID, OLW_CANINSERT, &tag_filt);
	}
	dlg->AddClusterAssoc(CTL_LOCCFG_FLAGS, 0, PPLocationConfig::fUseFias);
	dlg->SetClusterData(CTL_LOCCFG_FLAGS, cfg.Flags);
	{
		LocationFilt loc_filt;
		loc_filt.LocType = LOCTYP_WAREHOUSEGROUP;
		SetupLocationCombo(dlg, CTLSEL_LOCCFG_MPWHFLD, cfg.MarketplaceWarehoustFolderID, OLW_CANSELUPLEVEL/*|OLW_CANINSERT*/, &loc_filt); // @v12.2.1 @todo OLW_CANINSERT �������� � ������� - ���� ������!
	}
	while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_LOCCFG_WHZONECOD, &cfg.WhZoneCoding);
		dlg->getCtrlData(CTLSEL_LOCCFG_WHCOLCOD,  &cfg.WhColCoding);
		dlg->getCtrlData(CTLSEL_LOCCFG_WHCELLCOD, &cfg.WhCellCoding);
		dlg->GetClusterData(CTL_LOCCFG_DIVCOD, &cfg.WhCodingDiv);
		dlg->getCtrlData(CTLSEL_LOCCFG_DEFPALTYPE, &cfg.DefPalletID);
		dlg->getCtrlData(CTL_LOCCFG_ADDRCODETEMPL, cfg.AddrCodeTempl);
		dlg->getCtrlData(CTLSEL_LOCCFG_STRIDXTAG, &cfg.StoreIdxTagID);
		dlg->GetClusterData(CTL_LOCCFG_FLAGS, &cfg.Flags);
		dlg->getCtrlData(CTLSEL_LOCCFG_MPWHFLD, &cfg.MarketplaceWarehoustFolderID); // @v12.2.1
		ok = 1;
		if(memcmp(&cfg, &org_cfg, sizeof(cfg)) != 0) {
			if(!WriteConfig(&cfg, 1))
				ok = PPErrorZ();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
// Helper functions
//
int STDCALL SetupLocationCombo(TDialog * dlg, uint ctl, PPID id, uint flags, PPID locType, PPID owner)
{
	LocationFilt flt(locType, owner);
	return SetupPPObjCombo(dlg, ctl, PPOBJ_LOCATION, id, flags, &flt);
}

int STDCALL SetupLocationCombo(TDialog * dlg, uint ctl, PPID id, uint flags, const LocationFilt * pFilt)
{
	LocationFilt flt; //(locType, owner);
	if(!RVALUEPTR(flt, pFilt))
		flt.LocType = LOCTYP_WAREHOUSE;
	return SetupPPObjCombo(dlg, ctl, PPOBJ_LOCATION, id, flags, &flt);
}

/*static*/int PPObjLocation::SelectWarehouse(PPID /*owner*/, PPID /*level*/)
{
	PPID   id = LConfig.Location;
	int    r = PPSelectObject(PPOBJ_LOCATION, &id, PPTXT_SELECTLOCATION, 0);
	if(r > 0 && !DS.SetLocation(id))
		r = PPErrorZ();
	return r;
}

void PPObjLocation::InitInstance(SCtrLite sctr, void * extraPtr)
{
	Sctr = sctr;
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	P_CurrFilt = 0;
	P_WObj = 0;
	P_RegObj = 0;
	ExtraPtr = extraPtr;
	if(ExtraPtr) {
		P_CurrFilt = new LocationFilt(*static_cast<const LocationFilt *>(ExtraPtr));
		ExtraPtr = P_CurrFilt; // @v12.2.1
	}
	IsCityCacheInited = 0;
	if(Sctr != SConstructorLite)
		P_RegObj = new PPObjRegister;
}

PPObjLocation::PPObjLocation(void * extraPtr) : PPObject(PPOBJ_LOCATION)
{
	InitInstance(SConstructorDef, extraPtr);
}

PPObjLocation::PPObjLocation(SCtrLite sctr) : PPObject(PPOBJ_LOCATION)
{
	InitInstance(sctr, 0);
}

PPObjLocation::~PPObjLocation()
{
	delete P_WObj;
	delete P_RegObj;
	delete P_CurrFilt;
	TLP_CLOSE(P_Tbl);
}

SString & PPObjLocation::MakeCodeString(const LocationTbl::Rec * pRec, int options, SString & rBuf)
{
	rBuf.Z();
	if(pRec) {
		SString temp_buf;
		LocationTbl::Rec rec = *pRec;
		rBuf.Cat(rec.Code);
		if(oneof3(rec.Type, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
			PPLocationConfig cfg;
			PPObjLocation::FetchConfig(&cfg);
			while(rec.ParentID && Fetch(rec.ParentID, &rec) > 0 && rec.Type != LOCTYP_WAREHOUSEGROUP) {
				if(rec.Code[0] && (rec.Type != LOCTYP_WAREHOUSE || options & mcsWhCodePrefix)) {
					temp_buf.Z().Cat(rec.Code);
					if(cfg.WhCodingDiv == 0)
						temp_buf.Dot();
					else if(cfg.WhCodingDiv == 1)
						temp_buf.CatChar('-');
					temp_buf.Cat(rBuf);
					rBuf = temp_buf;
				}
			}
		}
		else if(oneof2(rec.Type, LOCTYP_ADDRESS, LOCTYP_WAREHOUSE)) {
			if((!options || options & mcsCode) && rec.Code[0])
				rBuf.CatDivIfNotEmpty('-', 1).Cat(rec.Code);
			if((!options || options & mcsName) && rec.Name[0])
				rBuf.CatDivIfNotEmpty('-', 1).Cat(rec.Name);
			if(!options || options & mcsContact) {
				LocationCore::GetExField(&rec, LOCEXSTR_CONTACT, temp_buf);
				if(temp_buf.NotEmptyS())
					rBuf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(options & mcsShortAddr) {
				LocationCore::GetExField(&rec, LOCEXSTR_SHORTADDR, temp_buf);
				if(temp_buf.NotEmptyS())
					rBuf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			else if(!options || options & mcsAddr) {
				LocationCore::GetAddress(rec, 0, temp_buf);
				if(temp_buf.NotEmptyS())
					rBuf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(!options || options & mcsPhone) {
				LocationCore::GetExField(&rec, LOCEXSTR_PHONE, temp_buf);
				if(temp_buf.NotEmptyS())
					rBuf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(!options || options & mcsEmail) {
				LocationCore::GetExField(&rec, LOCEXSTR_EMAIL, temp_buf);
				if(temp_buf.NotEmptyS())
					rBuf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(rBuf.IsEmpty())
				ideqvalstr(rec.ID, rBuf);
		}
	}
	return rBuf;
}

int PPObjLocation::Search(PPID id, void * b) { return P_Tbl->Search(id, (LocationTbl::Rec *)b); }

/*virtual*/int PPObjLocation::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam) // @v11.0.4
{
	// @v11.0.4 ������������ ����������� � ����� ������� ������� ������, ��������� � �������� id
	int    ok = -1;
	const  bool _user_request = LOGIC(options & user_request);
	SETFLAG(options, not_addtolog, 1);
	if(!_user_request || PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE) == cmYes) {
		options &= ~user_request;
		PPObjArticle ar_obj;
		{
			PPTransaction tra(BIN(options & use_transaction));
			THROW(tra);
			options &= ~use_transaction;
			{
				PPIDArray loc_id_list;
				PPIDArray ar_id_list;
				loc_id_list.add(id);
				ar_obj.GetByLocationList(0, &loc_id_list, &ar_id_list);
				if(ar_id_list.getCount()) {
					for(uint aridx = 0; aridx < ar_id_list.getCount(); aridx++) {
						const  PPID ar_id = ar_id_list.get(aridx);
						// pExtraParam ���� !0, �� ��������������� ��� PPObjLocation, �� �� ��� ���� ����� ��������
						THROW(ar_obj.RemoveObjV(ar_id, pObjColl, options, 0/*pExtraParam*/));
					}
				}
			}
			THROW(PPObject::RemoveObjV(id, pObjColl, options, pExtraParam));
			THROW(tra.Commit());
			ok = 1;
		}
	}
	CATCH
		// @v12.2.1 {
		if(_user_request)
			PPError();
		// } @v12.2.1 
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjLocation::GetParentWarehouse(PPID locID, PPID * pWarehouseID)
{
	int    ok = -1;
	LocationTbl::Rec rec;
	ASSIGN_PTR(pWarehouseID, 0);
	if(Fetch(locID, &rec) > 0) {
		if(rec.Type == LOCTYP_WAREHOUSE) {
			ASSIGN_PTR(pWarehouseID, rec.ID);
			ok = 1;
		}
		else if(oneof3(rec.Type, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
			while(rec.ParentID && Fetch(rec.ParentID, &rec) > 0) {
				if(rec.Type == LOCTYP_WAREHOUSE) {
					ASSIGN_PTR(pWarehouseID, rec.ID);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPObjLocation::ReqAutoName(PPID id)
{
	int    ok = 0;
	LocationTbl::Rec rec;
	PPID   temp_id = id;
	PPIDArray cycle_list;
	while(!ok && temp_id && Fetch(temp_id, &rec) > 0) {
		if(cycle_list.addUnique(temp_id) < 0) {
			// ������������� ����������� ������
			break;
		}
		else if(oneof5(rec.Type, LOCTYP_WAREHOUSE, LOCTYP_WAREPLACE, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL))
			if(rec.Flags & LOCF_WHAUTONAME)
				ok = (id == temp_id) ? 1 : -1;
			else
				temp_id = rec.ParentID;
		else
			temp_id = 0;
	}
	return ok;
}

int PPObjLocation::GetAddress(PPID locID, uint flags, SString & rBuf)
{
	rBuf.Z();
	LocationTbl::Rec loc_rec;
	return (!locID || Fetch(locID, &loc_rec) > 0) ? LocationCore::GetAddress(loc_rec, flags, rBuf) : 0;
}

int PPObjLocation::GetCountry(const LocationTbl::Rec * pLocRec, PPID * pCountryID, PPCountryBlock * pBlk)
{
	int    ok = -1;
	CALLPTRMEMB(pBlk, Z());
	ASSIGN_PTR(pCountryID, 0);
	if(pLocRec && pLocRec->CityID) {
		WorldTbl::Rec w_rec;
		THROW_MEM(SETIFZ(P_WObj, new PPObjWorld));
		if(P_WObj->GetCountryByChild(pLocRec->CityID, &w_rec) > 0) {
			if(pBlk) {
				pBlk->Name = w_rec.Name;
				pBlk->Abbr = w_rec.Abbr;
				pBlk->Code = w_rec.Code;
				pBlk->IsNative = 1;
				if(pBlk->Code.NotEmpty() && DS.GetConstTLA().MainOrgCountryCode != pBlk->Code)
					pBlk->IsNative = 0;
			}
			ASSIGN_PTR(pCountryID, w_rec.ID);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::GetCountry(PPID id, PPID * pCountryID, PPCountryBlock * pBlk)
{
	LocationTbl::Rec loc_rec;
	const LocationTbl::Rec * p_loc_rec = 0;
	if(id && Search(id, &loc_rec) > 0)
		p_loc_rec = &loc_rec;
	return GetCountry(p_loc_rec, pCountryID, pBlk);
}

int PPObjLocation::InitCityCache()
{
	int    ok = 1;
	IsCityCacheInited = 0;
	CityCache.freeAll();
	BExtQuery q(P_Tbl, 0, 128);
	q.select(P_Tbl->ID, P_Tbl->CityID, 0L).where(P_Tbl->CityID > 0L);
	LocationTbl::Key0 k0;
	MEMSZERO(k0);
	for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;)
		THROW_SL(CityCache.Add(P_Tbl->data.ID, P_Tbl->data.CityID, 0));
	CityCache.Sort();
	IsCityCacheInited = 1;
	CATCH
		ok = 0;
		CityCache.freeAll();
	ENDCATCH
	return ok;
}

int PPObjLocation::GetCityID(PPID locID, PPID * pCityID, int useCache)
{
	int    use_search = 1;
	PPID   city_id = 0;
	LocationTbl::Rec loc_rec;
	if(useCache) {
		if(Fetch(locID, &loc_rec) > 0)
			city_id = loc_rec.CityID;
		use_search = 0;
	}
	if(use_search) {
		if(Search(locID, &loc_rec) > 0)
			city_id = loc_rec.CityID;
	}
	ASSIGN_PTR(pCityID, city_id);
	return city_id ? 1 : -1;
}

int PPObjLocation::GetCity(PPID id, PPID * pCityID, SString * pCityName, int useCache)
{
	int    ok = -1;
	PPID   city_id = 0;
	if(id && GetCityID(id, &city_id, useCache) > 0) {
		if(pCityName) {
			SETIFZ(P_WObj, new PPObjWorld);
			WorldTbl::Rec w_rec;
			if(P_WObj->Fetch(city_id, &w_rec) > 0) {
				*pCityName = w_rec.Name;
			    ok = 1;
			}
		}
		else
			ok = 1;
	}
	if(ok <= 0)
		ASSIGN_PTR(pCityName, 0);
	ASSIGN_PTR(pCityID, city_id);
	return ok;
}

int PPObjLocation::GetCityByName(const char * pName, PPID * pCityID)
{
	int    ok = -1;
	ASSIGN_PTR(pCityID, 0);
	if(pName) {
		SETIFZ(P_WObj, new PPObjWorld);
		SVector w_list(sizeof(WorldTbl::Rec));
		P_WObj->GetListByName(WORLDOBJ_CITY, pName, &w_list);
		if(w_list.getCount() == 1) {
			ASSIGN_PTR(pCityID, static_cast<const WorldTbl::Rec *>(w_list.at(0))->ID);
			ok = 1;
		}
		else if(w_list.getCount()) {
			ASSIGN_PTR(pCityID, static_cast<const WorldTbl::Rec *>(w_list.at(0))->ID);
			ok = 2;
		}
	}
	return ok;
}

int PPObjLocation::FetchCityByAddr(PPID locID, WorldTbl::Rec * pRec)
{
	int    ok = -1;
	PPID   city_id = 0;
	if(locID && GetCityID(locID, &city_id, 1) > 0) {
		SETIFZ(P_WObj, new PPObjWorld);
		if(P_WObj) {
			if(P_WObj->Fetch(city_id, pRec) > 0)
				ok = 1;
		}
		else
			ok = PPSetErrorNoMem();
	}
	return ok;
}

int PPObjLocation::FetchCity(PPID cityID, WorldTbl::Rec * pRec)
{
	SETIFZ(P_WObj, new PPObjWorld);
	return P_WObj ? P_WObj->Fetch(cityID, pRec) : PPSetErrorNoMem();
}

const char * PPObjLocation::GetNamePtr()
{
	const LocationTbl::Rec & r_rec = P_Tbl->data;
	if(r_rec.Type == LOCTYP_ADDRESS)
		LocationCore::GetAddress(r_rec, 0, NameBuf);
	else if(r_rec.Type == LOCTYP_WAREPLACE) {
		LocationTbl::Rec par_rec;
		NameBuf.Z();
		if(r_rec.ParentID && Fetch(r_rec.ParentID, &par_rec) > 0)
			NameBuf.Cat(par_rec.Name).Slash();
		NameBuf.Cat(r_rec.Name);
	}
	else
		NameBuf = r_rec.Name;
	return NameBuf.cptr();
}

int PPObjLocation::DeleteObj(PPID id)
{
	return PutRecord(&id, 0, 0);
}

bool PPObjLocation::Validate(LocationTbl::Rec * pRec, int /*chkRefs*/)
{
	bool   ok = true;
	if(pRec) {
		PPID   id = 0;
		long   t = pRec->Type;
		LocTypeDescr ltd;
		THROW(P_Tbl->GetTypeDescription(t, &ltd));
		if(pRec->Name[0] == 0) {
			THROW_PP(t == LOCTYP_WHCELLAUTOGEN ||
				(t == LOCTYP_ADDRESS && (pRec->CityID || !LocationCore::IsEmptyAddressRec(*pRec))), PPERR_NAMENEEDED); // @v11.3.10 @fix IsEmptyAddressRec-->!IsEmptyAddressRec
		}
		THROW_PP(pRec->OwnerID || t != LOCTYP_DIVISION, PPERR_DIVOWNERNEEDED);
		THROW_PP(pRec->Code[0] || !(ltd.Flags & LocTypeDescr::fCodeRequired), PPERR_LOCREQCODE);
		if(pRec->ParentID) {
			LocationTbl::Rec par_rec;
			THROW(Fetch(pRec->ParentID, &par_rec) > 0);
			THROW_PP(par_rec.Type == ltd.ParentLocType, PPERR_INVLOCPARENTTYPE);
		}
		else {
			THROW_PP(!(ltd.Flags & LocTypeDescr::fParentRequired), PPERR_LOCREQPARENT);
		}
		{
			PPIDArray bycode_list;
			SString msg_buf;
			SString psn_name;
			LocationTbl::Rec cod_rec;
			if(P_Tbl->GetListByCode(t, pRec->Code, &bycode_list) > 0) {
				for(uint i = 0; i < bycode_list.getCount(); i++) {
					const  PPID cid = bycode_list.get(i);
					if(cid != pRec->ID && Search(cid, &cod_rec) > 0) {
						msg_buf.Z().CatEq("id", cod_rec.ID).CatDiv(':', 2).Cat(cod_rec.Name);
						if(cod_rec.OwnerID) {
							GetPersonName(cod_rec.OwnerID, psn_name);
							msg_buf.CatDiv(';', 2).CatEq("owner", psn_name);
						}
						THROW_PP_S(oneof3(t, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL), PPERR_DUPLOCSYMB, msg_buf);
						THROW_PP_S(cod_rec.Type != t || cod_rec.ParentID != pRec->ParentID, PPERR_DUPLOCSYMB, msg_buf);
					}
					//THROW(SearchCode(t, pRec->Code, &id, 0) < 0 || id == pRec->ID);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::AddListItem(StrAssocArray * pList, const LocationTbl::Rec * pLocRec, long zeroParentId, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	PPIDArray local_recur_trace;
	if(pList->Search(pLocRec->ID))
		ok = -1;
	else {
		PPID   par_id = pLocRec->ParentID;
		LocationTbl::Rec par_rec;
		SString text;
		if(par_id && Fetch(par_id, &par_rec) > 0) {
			int    r;
			SETIFZ(pRecurTrace, &local_recur_trace);
			THROW(r = pRecurTrace->addUnique(par_id));
			//THROW_PP_S(r > 0, PPERR_LOCATIONRECUR, par_rec.Name);
			if(r > 0) {
				THROW(AddListItem(pList, &par_rec, zeroParentId, pRecurTrace)); // @recursion
			}
			else {
				PPSetError(PPERR_LOCATIONRECUR, par_rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
				par_id = 0;
			}
		}
		else
			par_id = zeroParentId;
		text = pLocRec->Name;
		// @v12.2.10 {
		if(pLocRec->Type == LOCTYP_ADDRESS) {
			SString addr_buf;
			LocationCore::GetAddress(*pLocRec, 0, addr_buf);
			if(addr_buf.NotEmpty()) {
				text.CatDiv(':', 1).Cat(addr_buf);
			}
		}
		// } @v12.2.10 
		THROW_SL(pList->AddFast(pLocRec->ID, par_id, text));
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::MakeListByType(PPID locType, PPID parentID, long zeroParentId, int flags, StrAssocArray * pList)
{
	int    ok = 1;
	LocationTbl::Rec loc_rec, child_rec;
	PPIDArray recur_trace;
	PPIDArray col_list;
	for(SEnum en = P_Tbl->Enum(locType, parentID, flags); en.Next(&loc_rec) > 0;) {
		PPID   par_wh_id = 0;
		if(GetParentWarehouse(loc_rec.ID, &par_wh_id) <= 0 || ObjRts.CheckLocID(par_wh_id, 0)) {
			recur_trace.clear();
			THROW(AddListItem(pList, &loc_rec, zeroParentId, &recur_trace));
			if(locType == LOCTYP_WHZONE) {
				//THROW(MakeListByType(LOCTYP_WHCOLUMN, loc_rec.ID, zeroParentId, flags, pList)); // @recursion
				for(SEnum en2 = P_Tbl->Enum(LOCTYP_WHCOLUMN, loc_rec.ID, flags); en2.Next(&child_rec) > 0;) {
					THROW_SL(pList->Add(child_rec.ID, child_rec.ParentID, child_rec.Name, 0 /* don't replace dup */));
					col_list.add(child_rec.ID);
				}
			}
			else if(locType == LOCTYP_WHCOLUMN) {
				//THROW(MakeListByType(LOCTYP_WHCELL, loc_rec.ID, zeroParentId, flags, pList)); // @recursion
				for(SEnum en2 = P_Tbl->Enum(LOCTYP_WHCELL, loc_rec.ID, flags); en2.Next(&child_rec) > 0;) {
					THROW_SL(pList->Add(child_rec.ID, loc_rec.ID, child_rec.Name, 0 /* don't replace dup */));
				}
			}
		}
	}
	if(locType == LOCTYP_WHZONE) {
		col_list.sort();
		for(SEnum en3 = P_Tbl->Enum(LOCTYP_WHCELL, 0, LocationCore::eoIgnoreParent); en3.Next(&child_rec) > 0;) {
			if(child_rec.ParentID && col_list.bsearch(child_rec.ParentID)) {
				THROW_SL(pList->Add(child_rec.ID, child_rec.ParentID, child_rec.Name, 0 /* don't replace dup */));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::GetList(const LocationFilt * pLocFilt, long zeroParentId, StrAssocArray & rList)
{
	rList.Z();
	int    ok = 1;
	SString temp_buf;
	int    f = 0;
	const  LocationFilt * p_filt = 0; // NZOR(pLocFilt, &CurrFilt);
	if(pLocFilt)
		p_filt = pLocFilt;
	else {
		if(!P_CurrFilt) {
			THROW_MEM(P_CurrFilt = new LocationFilt);
			P_CurrFilt->LocType = LOCTYP_WAREHOUSE;
		}
		p_filt = P_CurrFilt;
	}
	const  PPID parent_id = (p_filt->Parent || !p_filt->Owner) ? p_filt->Parent : p_filt->Owner;
	if(oneof2(p_filt->LocType, LOCTYP_WAREHOUSE, LOCTYP_WAREHOUSEGROUP) || (p_filt->LocType == LOCTYP_WHZONE && !parent_id))
		f |= LocationCore::eoIgnoreParent;
	else if(p_filt->Owner)
		f |= LocationCore::eoParentAsOwner;
	if(p_filt->LocType == LOCTYP_ADDRESS && p_filt->GetExField(LocationFilt::exfPhone, temp_buf) > 0 && temp_buf.NotEmptyS()) {
		uint i;
		SString phone_buf;
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
		PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf);
		LongArray _pl;
		P_Tbl->SearchPhoneIndex(phone_buf, 0, _pl);
		for(i = 0; i < _pl.getCount(); i++) {
			EAddrTbl::Rec ea_rec;
			LocationTbl::Rec loc_rec;
			if(P_Tbl->GetEAddr(_pl.get(i), &ea_rec) > 0) {
				if(ea_rec.LinkObjType == PPOBJ_LOCATION && Search(ea_rec.LinkObjID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
					THROW_SL(rList.Add(loc_rec.ID, 0, loc_rec.Name, 1));
				}
			}
		}
	}
	else {
		THROW(MakeListByType(p_filt->LocType, parent_id, zeroParentId, f, &rList));
		if(oneof2(p_filt->LocType, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
			THROW(MakeListByType(LOCTYP_WHZONE, parent_id, zeroParentId, f, &rList));
		}
		else if(oneof2(p_filt->LocType, LOCTYP_WAREHOUSE, LOCTYP_WAREHOUSEGROUP)) {
			//
			// ��������� ��� ������ ������� (���������� ���� ��� ���������� �� ������, ������� �� ����� �������� ���������.
			//
			THROW(MakeListByType(LOCTYP_WAREHOUSEGROUP, parent_id, zeroParentId, f, &rList));
			{
				const uint elc = p_filt->ExtLocList.GetCount();
				if(elc) {
					LocationTbl::Rec loc_rec;
					for(uint i = 0; i < elc; i++) {
						const  PPID ext_loc_id = p_filt->ExtLocList.Get(i);
						if(Fetch(ext_loc_id, &loc_rec) > 0) {
							if(loc_rec.Type == LOCTYP_ADDRESS) {
								if(loc_rec.OwnerID) {
									GetPersonName(loc_rec.OwnerID, temp_buf);
									temp_buf.CatDiv('-', 1);
									if(loc_rec.Code[0])
										temp_buf.Cat(loc_rec.Code);
									else
										ideqvalstr(ext_loc_id, temp_buf);
								}
								else {
									(temp_buf = "Address").CatDiv(':', 1);
									if(loc_rec.Code[0])
										temp_buf.Cat(loc_rec.Code);
									else
										ideqvalstr(ext_loc_id, temp_buf);
								}
								rList.Add(ext_loc_id, 0, temp_buf);
							}
						}
					}
				}
			}
		}
	}
	if(zeroParentId) {
		LocationTbl::Rec loc_rec;
		loc_rec.ID = zeroParentId;
		PPLoadText(PPTXT_ALLWAREHOUSES, temp_buf.Z());
		temp_buf.CopyTo(loc_rec.Name, sizeof(loc_rec.Name));
		PPIDArray recur_trace;
		THROW(AddListItem(&rList, &loc_rec, 0, &recur_trace));
	}
	rList.SortByText();
	// @debug {
#if 0 // {
	{
		SString line_buf;
		for(uint i = 0; i < rList.getCount(); i++) {
			line_buf.Z();
			StrAssocArray::Item item = rList.Get(i);
			if(item.ParentId) {
				uint pos = 0;
				if(rList.Search(item.ParentId, &pos) > 0) {
					StrAssocArray::Item par_item = rList.Get(pos);
					line_buf.Cat(par_item.Txt).Semicol();
				}
			}
			line_buf.Cat(item.Txt).Semicol().Cat(item.Id).Semicol().Cat(item.ParentId);
			PPLogMessage(PPFILNAM_DEBUG_LOG, line_buf, 0);
		}
	}
#endif // } 0
	// } @debug
	CATCHZOK
	return ok;
}

StrAssocArray * PPObjLocation::MakeList_(const LocationFilt * pLocFilt, long zeroParentId)
{
	StrAssocArray * p_list = new StrAssocArray;
	if(p_list) {
		if(!GetList(pLocFilt, zeroParentId, *p_list))
			ZDELETE(p_list);
	}
	return p_list;
}

StrAssocArray * PPObjLocation::MakeStrAssocList(void * extraPtr) { return MakeList_(0, 0); }

int PPObjLocation::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->IsValid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(pDef);
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			for(uint i = 0; i < list.getCount(); i++) {
				PPID   id = list.at(i);
				long   img_id = ICON_WH;
				LocationTbl::Rec rec;
				if(Fetch(id, &rec) > 0) {
					switch(rec.Type) {
						case LOCTYP_WAREHOUSE: img_id = ICON_WH; break;
						case LOCTYP_WAREHOUSEGROUP: img_id = ICON_FOLDERGRP; break;
						case LOCTYP_WHZONE: img_id = ICON_WHZONE; break;
						case LOCTYP_WHCOLUMN: img_id = ICON_WHCOLUMN; break;
						case LOCTYP_WHCELL: img_id = ICON_WHCELL; break;
						case LOCTYP_DIVISION: img_id = ICON_DIVISION_16; break;
					}
				}
				p_def->AddImageAssoc(id, img_id);
			}
		}
	}
	return 1;
}

/*virtual*/ListBoxDef * PPObjLocation::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * p_def = PPObject::Selector(pOrgDef, flags, extraPtr);
	AssignImages(p_def);
	return p_def;
}

/*virtual*//*int PPObjLocation::UpdateSelector_Obsolete(ListBoxDef * pDef, long flags, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, flags, extraPtr);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}*/

int PPObjLocation::GenerateWhCells(PPID whColumnID, const LocationTbl::Rec * pSampleRec, int use_ta)
{
	int    ok = 1;
	LocationTbl::Rec column_rec;
	PPTransaction tra(use_ta);
	THROW(tra);
	THROW(Search(whColumnID, &column_rec) > 0);
	THROW_PP(column_rec.Type == LOCTYP_WHCOLUMN, PPERR_GENWHCOLLPARNCOL);
	THROW_PP(column_rec.NumRows > 0 && column_rec.NumLayers > 0, PPERR_GENWHCELLUNDEFCOLDIM);
	for(int row = 1; row <= column_rec.NumRows; row++) {
		const int depth = (column_rec.Depth > 0) ? column_rec.Depth : 1;
		for(int c = 1; c <= depth; c++) {
			for(int layer = 1; layer <= column_rec.NumLayers; layer++) {
				PPID   id = 0;
				LocationTbl::Rec cell_rec;
				cell_rec.Type = LOCTYP_WHCELL;
				cell_rec.ParentID = whColumnID;
				cell_rec.NumRows = static_cast<int16>(row);
				cell_rec.NumLayers = static_cast<int16>(layer);
				cell_rec.Depth = c;
				if(pSampleRec) {
					cell_rec.MassCapacity = pSampleRec->MassCapacity;
					cell_rec.X = pSampleRec->X;
					cell_rec.Y = pSampleRec->Y;
					cell_rec.Z = pSampleRec->Z;
					cell_rec.DisposeRestr = pSampleRec->DisposeRestr;
					cell_rec.DisposeRestrCount = pSampleRec->DisposeRestrCount;
				}
				InitCode(&cell_rec);
				THROW(PutRecord(&id, &cell_rec, 0));
			}
		}
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

/*
int PPObjLocation::ResolveWarehouseByCode(const char * pCode, PPID accSheetID, PPID * pArID)
{
	int    ok = -1;
	PPID   ar_id = 0;
	SString code(pCode);
	if(code.NotEmptyS()) {
		PPIDArray loc_list;
		LocationTbl::Rec loc_rec;
		P_Tbl->GetListByCode(LOCTYP_WAREHOUSE, code, &loc_list);
		if(loc_list.getCount()) {
			if(Fetch(loc_list.at(0), &loc_rec)) {
				PPObjArticle ar_obj;
				if(ar_obj.GetByPerson(accSheetID, loc_rec.OwnerID, &ar_id) > 0)
					ok = 1;
			}
		}
	}
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}
*/

int PPObjLocation::GetListByRegNumber(PPID regTypeID, PPID locTyp, const char * pSerial, const char * pNumber, PPIDArray & rList)
{
	rList.clear();
	int    ok = 1;
	SString msg_buf;
	RegisterFilt reg_flt;
	reg_flt.Oid.Obj = PPOBJ_LOCATION;
	reg_flt.RegTypeID = regTypeID;
	reg_flt.SerPattern = pSerial;
	reg_flt.NmbPattern = pNumber;
	THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister));
	int    r = P_RegObj->SearchByFilt(&reg_flt, 0, &rList);
	if(!r)
		ok = 0;
	else if(r < 0) {
		PPSetError(PPERR_LOCBYREGNFOUND, msg_buf.Cat(pSerial).CatDivIfNotEmpty(':', 1).Cat(pNumber));
		ok = -1;
	}
	else if(locTyp) {
		LocationTbl::Rec loc_rec;
		PPID   single_id = rList.getSingle();
		uint   c = rList.getCount();
		if(c) do {
			const  PPID loc_id = rList.get(--c);
			if(Fetch(loc_id, &loc_rec) <= 0 || loc_rec.Type != locTyp)
				rList.atFree(c);
		} while(c);
		if(!rList.getCount()) {
			if(single_id) {
				Fetch(single_id, &loc_rec);
				PPSetError(PPERR_LOCBYREGNTYPE, loc_rec.Name);
			}
			else
				PPSetError(PPERR_LOCSBYREGNTYPE, msg_buf.Cat(pSerial).CatDivIfNotEmpty(':', 1).Cat(pNumber));
			ok = -1;
		}
	}
	CATCHZOK
	return ok;
}

PPID PPObjLocation::GetTrunkPointByDlvrAddr(PPID dlvrAddrID)
{
	PPID   trunk_point_id = 0;
	LocationTbl::Rec loc_rec;
	if(dlvrAddrID && Fetch(dlvrAddrID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
		ObjTagItem tag_item;
		if(PPRef->Ot.GetTag(PPOBJ_LOCATION, dlvrAddrID, PPTAG_LOC_TRUNKPOINT, &tag_item) > 0) {
			PPID   _tag_dest_point_id = 0;
			if(tag_item.GetInt(&_tag_dest_point_id) && _tag_dest_point_id) {
				PPObjWorld w_obj;
				WorldTbl::Rec w_rec;
				if(w_obj.Search(_tag_dest_point_id, &w_rec) > 0 && oneof3(w_rec.Kind, WORLDOBJ_CITY, WORLDOBJ_CITYAREA, WORLDOBJ_STREET))
					trunk_point_id = _tag_dest_point_id;		
			}
		}
	}
	return trunk_point_id;
}

int PPObjLocation::ResolveGLN(PPID locTyp, const char * pGLN, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	SString code(pGLN);
	assert(pGLN);
	THROW_INVARG(pGLN);
	if(code.NotEmptyS()) {
		const  PPID reg_type_id = PPREGT_GLN;
		THROW(GetListByRegNumber(reg_type_id, locTyp, 0, code, rList));
		ok = rList.getCount() ? ((rList.getCount() > 1) ? 2 : 1) : -1;
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::ResolveWarehouse(PPID locID, PPIDArray & rDestList, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	LocationTbl::Rec loc_rec;
	PPIDArray local_recur_trace;
	SETIFZ(pRecurTrace, &local_recur_trace);
	if(Fetch(locID, &loc_rec) > 0) {
		if(pRecurTrace->addUnique(locID) < 0) {
			CALLEXCEPT_PP_S(PPERR_LOCATIONRECUR, loc_rec.Name);
		}
		if(loc_rec.Type == LOCTYP_WAREHOUSE) {
			if(ObjRts.CheckLocID(locID, 0))
				THROW_SL(rDestList.addUnique(locID));
		}
		else if(loc_rec.Type == LOCTYP_WAREHOUSEGROUP) {
			{
				for(SEnum en = P_Tbl->Enum(LOCTYP_WAREHOUSE, locID, 0); en.Next(&loc_rec) > 0;)
					THROW(ResolveWarehouse(loc_rec.ID, rDestList, pRecurTrace)); // @recursion
			}
			{
				for(SEnum en = P_Tbl->Enum(LOCTYP_WAREHOUSEGROUP, locID, 0); en.Next(&loc_rec) > 0;)
					THROW(ResolveWarehouse(loc_rec.ID, rDestList, pRecurTrace)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::ResolveWarehouseList(const PPIDArray * pList, PPIDArray & rDestList)
{
	int    ok = 1;
	PPIDArray temp_list;
	if(pList) {
		for(uint i = 0; i < pList->getCount(); i++)
			THROW(ResolveWarehouse(pList->get(i), temp_list, 0));
	}
	else
		ok = -1;
	CATCHZOK
	rDestList = temp_list;
	return ok;
}

int PPObjLocation::ResolveWhCellList(const PPIDArray * pList, long options, PPIDArray & rDestList)
{
	int    ok = 1;
	PPIDArray temp_list, wh_list;
	if(!pList) {
		GetWarehouseList(&wh_list, 0);
		pList = &wh_list;
	}
	assert(pList); // @paranoic
	const int use_cache = 1;
	for(uint i = 0; i < pList->getCount(); i++)
		THROW(ResolveWhCell(pList->get(i), temp_list, 0, use_cache));
	CATCHZOK
	rDestList = temp_list;
	return ok;
}

int PPObjLocation::IsMemberOfGroup(PPID locID, PPID grpID)
{
	int    ok = -1, r;
	PPIDArray recur_trace;
	while(locID && ok < 0) {
		LocationTbl::Rec loc_rec;
		THROW(r = Fetch(locID, &loc_rec));
		if(r > 0) {
			THROW_SL(r = recur_trace.addUnique(locID));
			THROW_PP_S(r > 0, PPERR_LOCATIONRECUR, loc_rec.Name);
			if(loc_rec.ParentID == grpID)
				ok = 1;
			else
				locID = loc_rec.ParentID;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::BelongTo(PPID locID, PPID parentID, SString * pPathText)
{
	int    yes = 0;
	ASSIGN_PTR(pPathText, 0);
	if(parentID) {
		LocationTbl::Rec loc_rec;
		if(locID == parentID) {
			if(pPathText) {
				if(Fetch(locID, &loc_rec) > 0)
					*pPathText = loc_rec.Name;
			}
			yes = 1;
		}
		else {
			PPIDArray recur_trace;
			while(!yes && Fetch(locID, &loc_rec) > 0 && loc_rec.ParentID) {
				if(pPathText) {
					if(pPathText->NotEmpty())
						pPathText->Slash();
					pPathText->Cat(loc_rec.Name);
				}
				if(loc_rec.ParentID == parentID)
					yes = 1;
				else {
					locID = loc_rec.ParentID;
					if(recur_trace.lsearch(locID))
						break; // �� �����������
					else
						recur_trace.add(locID);
				}
			}
		}
	}
	if(!yes)
		ASSIGN_PTR(pPathText, 0);
	return yes;
}

int PPObjLocation::SearchName(PPID locTyp, PPID parentID, const char * pName, PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	SString name_buf(pName);
	if(name_buf.NotEmptyS()) {
		LocationTbl::Key2 k2;
		MEMSZERO(k2);
		k2.Type = static_cast<int16>(locTyp);
		BExtQuery q(P_Tbl, 2);
		q.select(P_Tbl->ID, P_Tbl->ParentID, P_Tbl->Name, 0L).where(P_Tbl->Type == locTyp);
		for(q.initIteration(false, &k2, spGe); ok < 0 && q.nextIteration() > 0;) {
			if(name_buf.CmpNC(P_Tbl->data.Name) == 0) {
				if(IsMemberOfGroup(P_Tbl->data.ID, parentID) > 0) {
					id = P_Tbl->data.ID;
					ok = 1;
				}
			}
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

class LocationView : public ObjViewDialog {
public:
	LocationView(PPObjLocation * pObj, void * extraPtr) : ObjViewDialog(DLG_LOCVIEW, pObj, extraPtr), IsWareplaceView(0), ParentID(0)
	{
		SString fmt_buf, par_name, title_;
		if(extraPtr) {
			const LocationFilt * p_filt = static_cast<const LocationFilt *>(extraPtr);
			if(p_filt->LocType == LOCTYP_WAREPLACE) {
				enableCommand(cmWareplaceList, 0);
				PPLoadText(PPTXT_TITLE_WAREPLACEVIEW, fmt_buf);
				GetLocationName(p_filt->Parent, par_name);
				setTitle(title_.Printf(fmt_buf, par_name.cptr()));
				IsWareplaceView = 1;
				ParentID = p_filt->Parent;
			}
			else if(p_filt->LocType == LOCTYP_WAREHOUSE) {
				PPLoadString("warehouse_pl", title_);
				setTitle(title_);
			}
		}
		else {
			PPLoadString("warehouse_pl", title_);
			setTitle(title_);
		}
	}
private:
	DECL_HANDLE_EVENT
	{
		if(event.isCmd(cmaInsert)) { // Override ObjViewDialog::handeEvent on cmaInsert
			if(Rt & PPR_INS && P_List) {
				PPID   id = 0;
				const  PPID cur_id = getCurrID();
				LocationFilt flt;
				if(ExtraPtr)
					flt = *(LocationFilt*)ExtraPtr;
				if(cur_id)
					flt.CurrID = cur_id;
				if(P_Obj->Edit(&id, &flt) == cmOK)
					updateList(id);
			}
		}
		else {
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmWareplaceList))
				ViewWareplaces();
			else if(event.isCmd(cmLocGoodsAssoc)) {
				if(IsWareplaceView) {
					LocationFilt loc_filt;
					loc_filt.LocType = LOCTYP_WAREPLACE;
					loc_filt.Parent = ParentID;
					ViewGoodsToLocAssoc(0, PPASS_GOODS2WAREPLACE, &loc_filt, goafModal | goafFreeExtraAsPtr);
				}
				else {
					ViewGoodsToLocAssoc(0, PPASS_GOODS2LOC, 0, goafModal);
				}
			}
			else if(event.isCmd(cmTransmitCharry)) {
				PPIDArray id_list;
				PPObjLocation & r_obj = *static_cast<PPObjLocation *>(P_Obj);
				StrAssocArray * p_list = r_obj.MakeList_(0);
				if(p_list) {
					for(uint i = 0; i < p_list->getCount(); i++) {
						id_list.addUnique(p_list->Get(i).Id);
					}
					if(!SendCharryObject(PPDS_CRRLOCATION, id_list)) {
						PPError();
					}
				}
				else
					PPError();
				delete p_list;
			}
			else if(event.isKeyDown(kbCtrlR))
				Restore();
			else
				return;
		}
		clearEvent(event);
	}
	virtual int  transmit(PPID);
	int    Restore();
	int    ViewWareplaces();

	int    IsWareplaceView;
	PPID   ParentID;
};

int LocationView::ViewWareplaces()
{
	int    ok = -1;
	PPID   id = getCurrID();
	if(id) {
		LocationTbl::Rec loc_rec;
		if(P_Obj->Search(id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE) {
			LocationFilt filt(LOCTYP_WAREPLACE, 0, id);
			//
			// ����� ���������� ������� ��������� �� P_Obj ��������� PPObjLocation
			// ��-�� ����, ��� ������� ������������ ������, �������� ������ �
			// ���������� �������, � �� ���������� ��� ���������� ������ LocationView.
			//
			PPObjLocation loc_obj(&filt);
			ok = BIN(CheckExecAndDestroyDialog(new LocationView(&loc_obj, &filt), 1, 1) != 0);
		}
	}
	return ok;
}

int LocationView::transmit(PPID)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		PPID   id = 0;
		PPObjLocation loc_obj;
		PPIDArray wh_list;
		const PPIDArray & rary = param.DestDBDivList.Get();
		loc_obj.GetWarehouseList(&wh_list, 0);
		PPObjIDArray objid_ary;
		PPWaitStart();
		THROW(objid_ary.Add(PPOBJ_LOCATION, wh_list));
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int LocationView::Restore()
{
	int    ok = 1;
	long   art_no = 0;
	ArticleTbl::Rec rec;
	PPObjArticle ar_obj;
	PPObjLocation loc_obj;
	PPLogger logger;
	const  PPID loc_acs_id = LConfig.LocAccSheetID;
	while(ar_obj.P_Tbl->EnumBySheet(loc_acs_id, &art_no, &rec) > 0) {
		int    r = loc_obj.Search(rec.ObjID, 0);
		if(r < 0) {
			LocationTbl::Rec loc_rec;
			loc_rec.ID   = rec.ObjID;
			loc_rec.Type = LOCTYP_WAREHOUSE;
			STRNSCPY(loc_rec.Name, rec.Name);
			r = 1;
			PPID   temp_id = rec.ObjID;
			if(loc_obj.P_Tbl->Add(&temp_id, &loc_rec, 0)) {
				if(loc_rec.Type == LOCTYP_WAREHOUSE)
					if(SendObjMessage(DBMSG_WAREHOUSEADDED, PPOBJ_ARTICLE, PPOBJ_LOCATION, temp_id) != DBRPL_OK)
						r = 0;
			}
			if(r)
				logger.LogString(PPTXT_LOCRECRESTORED, loc_rec.Name);
		}
		if(!r) {
			logger.LogLastError();
			ok = 0;
		}
	}
	updateList(-1);
	return ok;
}

int PPObjLocation::Browse(void * extraPtr)
{
	int    ok = 0;
	if(CheckRights(PPR_READ)) {
		LocationView * dlg = new LocationView(this, extraPtr);
		if(CheckDialogPtrErr(&dlg)) {
			ExecViewAndDestroy(dlg);
			ok = 1;
		}
	}
	else
		PPError();
	return ok;
}
//
//
//
class LocationExtFieldsDialog : public PPListDialog {
	DECL_DIALOG_DATA(LocationTbl::Rec);
public:
	LocationExtFieldsDialog() : PPListDialog(DLG_DLVREXTFLDS, CTL_LBXSEL_LIST)
	{
		if(SmartListBox::IsValidS(P_Box))
			P_Box->P_Def->SetOption(lbtFocNotify, 1);
		PPPersonConfig psn_cfg;
		PPObjPerson::ReadConfig(&psn_cfg);
		FieldNames = psn_cfg.DlvrAddrExtFldList;
		showCtrl(STDCTL_INSBUTTON, false);
	}
	DECL_DIALOG_SETDTS()
	{
		SString temp_buf;
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		Fields.Z();
		for(int id = 1; id <= MAX_DLVRADDRFLDS; id++) {
			if(LocationCore::GetExField(&Data, id + LOCEXSTR_EXTFLDSOFFS, temp_buf.Z()) > 0)
				Fields.Add(id + LOCEXSTR_EXTFLDSOFFS, 0, temp_buf);
		}
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		for(uint id = 1; id <= MAX_DLVRADDRFLDS; id++)
			LocationCore::SetExField(&Data, id + LOCEXSTR_EXTFLDSOFFS, 0);
		for(uint i = 0; i < Fields.getCount(); i++) {
			StrAssocArray::Item item = Fields.Get(i);
			LocationCore::SetExField(&Data, item.Id, item.Txt);
		}
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();
	int    Edit(SStringTag *);

	StrAssocArray FieldNames;
	StrAssocArray Fields;
};

int LocationExtFieldsDialog::Edit(SStringTag * pData)
{
	class AddExtFldDialog : public TDialog {
		DECL_DIALOG_DATA(SStringTag);
	public:
		explicit AddExtFldDialog(const StrAssocArray * pFieldNames) : TDialog(DLG_ADDEXTFLD)
		{
			RVALUEPTR(FieldNames, pFieldNames);
			disableCtrl(CTLSEL_ADDEXTFLD_FLD, true);
		}
		DECL_DIALOG_SETDTS()
		{
			if(!RVALUEPTR(Data, pData)) {
				Data.Id = 0;
				Data.Z();
			}
			SetupStrAssocCombo(this, CTLSEL_ADDEXTFLD_FLD, FieldNames, Data.Id, 0);
			setCtrlString(CTL_ADDEXTFLD_VAL, Data);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_ADDEXTFLD_FLD, &Data.Id);
			getCtrlString(CTL_ADDEXTFLD_VAL, Data);
			THROW_PP(Data.Id, PPERR_USERINPUT);
			ASSIGN_PTR(pData, Data);
			CATCHZOK
			return ok;
		}
	private:
		StrAssocArray FieldNames;
	};
	int    ok = -1;
	SString prev_txt;
	SStringTag data;
	StrAssocArray field_names;
	AddExtFldDialog * p_dlg = 0;
	RVALUEPTR(data, pData);
	prev_txt = data;
	field_names = FieldNames;
	for(uint i = 0; i < Fields.getCount(); i++) {
		uint pos = 0;
		long id = Fields.Get(i).Id;
		if(id != data.Id && field_names.Search(id, &pos) > 0)
			field_names.AtFree(pos);
	}
	if(CheckDialogPtrErr(&(p_dlg = new AddExtFldDialog(&field_names))) > 0) {
		p_dlg->setDTS(&data);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(!p_dlg->getDTS(&data))
				PPError();
			else if(sstrlen(Data.Tail) + data.Len() - prev_txt.Len() < sizeof(Data.Tail)) {
				LocationCore::SetExField(&Data, data.Id, data);
				ASSIGN_PTR(pData, data);
				ok = valid_data = 1;
			}
			else
				PPError(PPERR_MAXTAILLENACHIEVED);
		}
	}
	delete p_dlg;
	return ok;

}

int LocationExtFieldsDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < static_cast<long>(FieldNames.getCount())) {
		SString buf;
		SStringTag item;
		item.Id = id;
		Fields.GetText(id, buf);
		static_cast<SString &>(item) = buf;
		if(Edit(&item) > 0) {
			Fields.Add(item.Id, item);
			ok = 1;
		}
	}
	return ok;
}

int LocationExtFieldsDialog::delItem(long pos, long id)
{
	LocationCore::SetExField(&Data, id, 0);
	return Fields.Remove(id);
}

int LocationExtFieldsDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < FieldNames.getCount(); i++) {
		StrAssocArray::Item item = FieldNames.Get(i);
		ss.Z();
		temp_buf.Z().Cat(item.Id);
		ss.add(temp_buf);
		temp_buf = item.Txt;
		if(temp_buf.IsEmpty())
			temp_buf.CatEq("ID", item.Id);
		ss.add(temp_buf);
		Fields.GetText(item.Id, temp_buf);
		ss.add(temp_buf);
		if(!addStringToList(item.Id, ss.getBuf())) {
			ok = 0;
			PPError();
		}
	}
	return ok;
}

int EditDlvrAddrExtFields(LocationTbl::Rec * pData) { DIALOG_PROC_BODYERR(LocationExtFieldsDialog, pData); }

class LocationDialog : public TDialog {
public:
	LocationDialog(uint rezID, long locTyp, long edflags) : TDialog(rezID), EdFlags(edflags), LocTyp(locTyp)
	{
		LocationCore::GetTypeDescription(LocTyp, &Ltd);
		setTitle(Ltd.Name);
		SetupStrListBox(getCtrlView(CTL_LOCATION_WHLIST)); // @v11.0.6
		enableCommand(cmExtFields, BIN(locTyp == LOCTYP_ADDRESS));
		SetupGeoLocButton(this, CTL_LOCATION_COORD, cmExecMapOnGeoLoc); // @v11.6.2 @construction
	}
	int    setDTS(const PPLocationPacket * pData);
	int    getDTS(PPLocationPacket * pData);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	int    CopyFullAddr();
	int    GetGeoCoord();
	int    SetGeoCoord();
	int    ReqAutoName();
	void   UpdateWarehouseList(long pos, int byPos /*= 1*/);

	long   LocTyp;
	long   EdFlags;
	PPObjLocation LocObj;
	LocTypeDescr Ltd;
	PPLocationPacket Data;
};

int LocationDialog::SetGeoCoord()
{
	SString temp_buf;
	SGeoPosLL pos(Data.Latitude, Data.Longitude);
	pos.ToStr(temp_buf);
	return setCtrlString(CTL_LOCATION_COORD, pos.ToStr(temp_buf));
}

int LocationDialog::GetGeoCoord()
{
	int    ok = -1;
	uint   sel;
	SString temp_buf;
	if(getCtrlString(sel = CTL_LOCATION_COORD, temp_buf)) {
		SGeoPosLL pos;
		if(pos.FromStr(temp_buf)) {
			Data.Latitude = pos.Lat;
			Data.Longitude = pos.Lon;
			ok = pos.IsZero() ? -1 : 1;
		}
		else
			ok = PPErrorByDialog(this, sel, PPERR_SLIB);
	}
	return ok;
}

void LocationDialog::UpdateWarehouseList(long pos, int byPos /*= 1*/)
{
	SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_LOCATION_WHLIST));
	if(p_box) {
		SString temp_buf;
		const long sav_pos = p_box->P_Def ? p_box->P_Def->_curItem() : 0;
		p_box->freeAll();
		for(uint i = 0; i < Data.WarehouseList.GetCount(); i++) {
			const  PPID loc_id = Data.WarehouseList.Get(i);
			LocationTbl::Rec loc_rec;
			if(LocObj.Fetch(loc_id, &loc_rec) > 0)
				temp_buf = loc_rec.Name;
			else
				ideqvalstr(loc_id, temp_buf.Z());
			p_box->addItem(loc_id, temp_buf);
		}
		p_box->Draw_();
		if(byPos)
		   	p_box->focusItem((pos < 0) ? sav_pos : pos);
		else
			p_box->Search_(&pos, 0, srchFirst|lbSrchByID);
	}
}

IMPL_HANDLE_EVENT(LocationDialog)
{
	TDialog::handleEvent(event);
	SString temp_buf;
	if(event.isClusterClk(CTL_LOCATION_FLAGS) && LocTyp == LOCTYP_ADDRESS)
		SetupCtrls();
	else if(event.isClusterClk(CTL_LOCATION_VOLUMEVAL)) {
		GetClusterData(CTL_LOCATION_VOLUMEVAL, &Data.Flags);
		if(Data.Flags & LOCF_VOLUMEVAL) {
			Data.X = getCtrlLong(CTL_LOCATION_SZWD);
			Data.Y = getCtrlLong(CTL_LOCATION_SZLN);
			Data.Z = getCtrlLong(CTL_LOCATION_SZHT);
			setCtrlReal(CTL_LOCATION_VOLUME, ((double)Data.X * (double)Data.Y * (double)Data.Z) / 1.E9);
		}
		else {
			double volume = getCtrlReal(CTL_LOCATION_VOLUME);
			setCtrlLong(CTL_LOCATION_SZWD, Data.X = 100L);
			setCtrlLong(CTL_LOCATION_SZLN, Data.Y = 100L);
			setCtrlLong(CTL_LOCATION_SZHT, Data.Z = R0i(volume * 100000.0));
		}
		disableCtrls(Data.Flags & LOCF_VOLUMEVAL, CTL_LOCATION_SZWD, CTL_LOCATION_SZLN, CTL_LOCATION_SZHT, 0);
	}
	else if(event.isCmd(cmaInsert)) {
		SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_LOCATION_WHLIST));
		if(p_box) {
			PPIDArray loc_id_list;
			Data.WarehouseList.Get(loc_id_list);
			ListToListData ltld(PPOBJ_LOCATION, 0, &loc_id_list);
			ltld.Flags |= ListToListData::fIsTreeList;
			ltld.TitleStrID = 0; // PPTXT_XXX;
			if(ListToListDialog(&ltld) > 0) {
				loc_id_list.sortAndUndup();
				Data.WarehouseList.Set(&loc_id_list);
				UpdateWarehouseList(-1, 1);
			}
		}
	}
	else if(event.isCmd(cmaDelete)) {
		SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_LOCATION_WHLIST));
		if(SmartListBox::IsValidS(p_box)) {
			long   i = 0;
			p_box->getCurID(&i);
			if(i) {
				uint pos = 0;
				if(Data.WarehouseList.Search(i, &pos)) {
					Data.WarehouseList.RemoveByIdx(pos);
					UpdateWarehouseList(pos, 1);
				}
			}
		}
	}
	else if(event.isCmd(cmInputUpdatedByBtn)) {
		if(event.isCtlEvent(CTL_LOCATION_ZIP) || event.isCtlEvent(CTL_LOCATION_ADDR))
			SetupCtrls();
		else if(event.isCtlEvent(CTL_LOCATION_CODE)) {
			if(oneof3(LocTyp, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
				if(ReqAutoName()) {
					Data.ParentID = getCtrlLong(CTLSEL_LOCATION_PARENT);
					getCtrlData(CTL_LOCATION_CODE, Data.Code);
					LocObj.MakeCodeString(&Data, 0, temp_buf);
					setCtrlString(CTL_LOCATION_NAME, temp_buf);
				}
			}
		}
		else
			return;
	}
	else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_LOCATION_PHONE)) {
		SetupPhoneButton(this, CTL_LOCATION_PHONE, cmLocAction1);
	}
	else if(event.isCmd(cmLocAction1)) {
		const  PPID def_phn_svc_id = DS.GetConstTLA().DefPhnSvcID;
		if(def_phn_svc_id) {
			getCtrlString(CTL_LOCATION_PHONE, temp_buf);
			if(temp_buf.Len() >= 5) {
				SString phone_buf;
				temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
				if(PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf).Len() >= 5)
					PPObjPhoneService::PhoneTo(phone_buf);
			}
		}
	}
	else if(event.isCmd(cmAddress)) {
		const int  save_loc_typ = Data.Type;
		//
		// ����� ��������������� ������ ���������� ��������� � ��������
		// ���� ParentID � ������ ��������� ������ �������������� �����
		// ������ ��������� ������������ ����� ������.
		//
		const  PPID save_par_id = Data.ParentID;
		getDTS(0);
		Data.ParentID = 0;
		if(LocObj.EditDialog(LOCTYP_ADDRESS, &Data, 0) == cmOK) {
			Data.ParentID = save_par_id;
			setDTS(0);
			Data.Type = static_cast<int16>(save_loc_typ);
		}
		else
			Data.ParentID = save_par_id;
	}
	else if(event.isCmd(cmLocRegisters)) {
		PPObjRegister reg_obj;
		reg_obj.EditList(&Data);
	}
	else if(event.isCmd(cmLocTags)) {
		Data.TagL.Oid.Id = Data.ID;
		EditObjTagValList(&Data.TagL, 0);
	}
	else if(event.isCmd(cmTest)) {
		if(LocTyp == LOCTYP_ADDRESS) {
			PPLocationPacket addr;
			getDTS(&addr);
			LocationCore::GetAddress(addr, 0, temp_buf);
			LocObj.EditAddrStruc(temp_buf);
		}
	}
	else if(event.isCmd(cmaMore)) {
		if(Data.OwnerID) {
			PPObjPerson psn_obj;
			PPID   psn_id = Data.OwnerID;
			if(psn_obj.Edit(&psn_id, 0) > 0) {
				GetPersonName(Data.OwnerID, temp_buf);
				setCtrlString(CTL_LOCATION_OWNERNAME, temp_buf);
			}
		}
	}
	else if(event.isCmd(cmExtFields))
		EditDlvrAddrExtFields(&Data);
	else if(event.isCmd(cmExecMapOnGeoLoc)) { // @v11.6.2
		if(GetGeoCoord() > 0) {
			//https://www.google.com/maps?q=61.7697198,34.2983906
			SString cmd_line;
			(temp_buf = "maps").CatChar('?').CatChar('q').Eq().Cat(Data.Latitude, MKSFMTD(0, 7, NMBF_NOTRAILZ)).Comma().Cat(Data.Longitude, MKSFMTD(0, 7, NMBF_NOTRAILZ));
			cmd_line = InetUrl::MkHttps("www.google.com", temp_buf);
			::ShellExecute(0, _T("open"), SUcSwitch(cmd_line), NULL, NULL, SW_SHOW);
		}
	}
	else if(event.isCbSelected(CTLSEL_LOCATION_CITY))
		SetupCtrls();
	else if(event.isCbSelected(CTLSEL_LOCATION_PARENT)) {
		PPID prev_par_id = Data.ParentID;
		PPID par_id = getCtrlLong(CTLSEL_LOCATION_PARENT);
		if(par_id) {
			LocationTbl::Rec loc_rec;
			for(PPID loc_id = par_id; loc_id;)
				if(LocObj.Fetch(loc_id, &loc_rec) > 0) {
					if(loc_id == Data.ID) {
						PPError(PPERR_LOCATIONRECUR, loc_rec.Name);
						setCtrlLong(CTLSEL_LOCATION_PARENT, prev_par_id);
						break;
					}
					else
						loc_id = loc_rec.ParentID;
				}
				else
					loc_id = 0;
		}
	}
	else if(event.isKeyDown(kbF2))
		CopyFullAddr();
	else
		return;
	clearEvent(event);
}

int LocationDialog::ReqAutoName()
{
	if(Data.ID)
		return LocObj.ReqAutoName(Data.ID);
	else if(Data.ParentID)
		return LocObj.ReqAutoName(Data.ParentID) ? -1 : 0;
	else
		return 0;
}

int LocationDialog::setDTS(const PPLocationPacket * pData)
{
	int    ok = 1;
	RVALUEPTR(Data, pData);
	ushort v = 0;
	SString temp_buf;
	setCtrlData(CTL_LOCATION_NAME, Data.Name);
	setCtrlData(CTL_LOCATION_CODE, Data.Code);
	setCtrlData(CTL_LOCATION_ID, &Data.ID);
	SetupPPObjCombo(this, CTLSEL_LOCATION_OWNER, PPOBJ_PERSON, Data.OwnerID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPPRK_EMPLOYER));
	SetupPPObjCombo(this, CTLSEL_LOCATION_RSPNS, PPOBJ_PERSON, Data.RspnsPersonID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPPRK_EMPL));
	SetupPPObjCombo(this, CTLSEL_LOCATION_CITY,  PPOBJ_WORLD, Data.CityID, OLW_CANINSERT|OLW_CANSELUPLEVEL|OLW_WORDSELECTOR, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
	LocationCore::GetExField(&Data, LOCEXSTR_ZIP, temp_buf);
	setCtrlString(CTL_LOCATION_ZIP, temp_buf);
	LocationCore::GetExField(&Data, LOCEXSTR_SHORTADDR, temp_buf);
	setCtrlString(CTL_LOCATION_ADDR, temp_buf);
	if(LocTyp == LOCTYP_ADDRESS) {
		AddClusterAssoc(CTL_LOCATION_FLAGS, 0, LOCF_MANUALADDR);
		SetClusterData(CTL_LOCATION_FLAGS, Data.Flags);
		// @v11.9.9 {
		AddClusterAssoc(CTL_LOCATION_PASSIVE, 0, LOCF_PASSIVE);
		SetClusterData(CTL_LOCATION_PASSIVE, Data.Flags);
		// } @v11.9.9 
		LocationCore::GetExField(&Data, LOCEXSTR_FULLADDR, temp_buf);
		setCtrlString(CTL_LOCATION_FULLADDR, temp_buf);
		SetGeoCoord();
		LocationCore::GetExField(&Data, LOCEXSTR_PHONE, temp_buf);
		setCtrlString(CTL_LOCATION_PHONE, temp_buf);
		SetupPhoneButton(this, CTL_LOCATION_PHONE, cmLocAction1);
		LocationCore::GetExField(&Data, LOCEXSTR_CONTACT, temp_buf);
		setCtrlString(CTL_LOCATION_CONTACT, temp_buf);
		LocationCore::GetExField(&Data, LOCEXSTR_EMAIL, temp_buf);
		setCtrlString(CTL_LOCATION_EMAIL, temp_buf);
		if(Data.OwnerID) {
			GetPersonName(Data.OwnerID, temp_buf);
			setCtrlString(CTL_LOCATION_OWNERNAME, temp_buf);
		}
		setCtrlReadOnly(CTL_LOCATION_OWNERNAME, true);
	}
	else if(oneof4(LocTyp, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL, LOCTYP_WHCELLAUTOGEN)) {
		PPID   par_wh_id = 0;
		if(LocObj.GetParentWarehouse(NZOR(Data.ID, Data.ParentID), &par_wh_id) > 0) {
			GetLocationName(par_wh_id, temp_buf);
			setStaticText(CTL_LOCATION_ST_EXTTITLE, temp_buf);
		}
		LocationFilt loc_filt(Ltd.ParentLocType, 0, par_wh_id);
		SetupPPObjCombo(this, CTLSEL_LOCATION_PARENT, PPOBJ_LOCATION, Data.ParentID, OLW_LOADDEFONOPEN|OLW_CANSELUPLEVEL, &loc_filt);
		setCtrlData(CTL_LOCATION_NUMROWS,   &Data.NumRows);
		setCtrlData(CTL_LOCATION_NUMLAYERS, &Data.NumLayers);
		setCtrlData(CTL_LOCATION_DEPTH,     &Data.Depth);
		setCtrlReal(CTL_LOCATION_CAPACITY,  fdiv1000i(Data.MassCapacity));
		setCtrlLong(CTL_LOCATION_SZWD, Data.X);
		setCtrlLong(CTL_LOCATION_SZLN, Data.Y);
		setCtrlLong(CTL_LOCATION_SZHT, Data.Z);
		setCtrlReal(CTL_LOCATION_VOLUME, ((double)Data.X * (double)Data.Y * (double)Data.Z) / 1.E9);
		AddClusterAssoc(CTL_LOCATION_VOLUMEVAL, 0, LOCF_VOLUMEVAL);
		SetClusterData(CTL_LOCATION_VOLUMEVAL, Data.Flags);
		disableCtrls(Data.Flags & LOCF_VOLUMEVAL, CTL_LOCATION_SZWD, CTL_LOCATION_SZLN, CTL_LOCATION_SZHT, 0);
		selectCtrl(CTL_LOCATION_CODE);
		{
			int    ran = ReqAutoName();
			AddClusterAssoc(CTL_LOCATION_WHFLAGS, 0, LOCF_WHAUTONAME);
			long   flags = Data.Flags;
			SETFLAG(flags, LOCF_WHAUTONAME, ran);
			SetClusterData(CTL_LOCATION_WHFLAGS, flags);
			DisableClusterItem(CTL_LOCATION_WHFLAGS, 0, ran < 0);
		}
	}
	else {
		LocationFilt loc_filt(LOCTYP_WAREHOUSEGROUP);
		SetupPPObjCombo(this, CTLSEL_LOCATION_PARENT, PPOBJ_LOCATION, Data.ParentID, OLW_LOADDEFONOPEN|OLW_CANSELUPLEVEL, &loc_filt);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 0, LOCF_VATFREE);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 1, LOCF_COMPARABLE);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 2, LOCF_ADJINTRPRICE);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 3, LOCF_WHAUTONAME);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 4, LOCF_WHCODEPREFIX);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 5, LOCF_SEQCOLCODE);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 6, LOCF_DISPOSEBILLS);
		AddClusterAssoc(CTL_LOCATION_FLAGS, 7, LOCF_PASSIVE); // @v11.9.9
		SetClusterData(CTL_LOCATION_FLAGS, Data.Flags);
	}
	UpdateWarehouseList(-1, 1);
	SetupCtrls();
	if(getCtrlView(CTL_LOCATION_ST_ADDR)) {
		LocationCore::GetAddress(Data, 0, temp_buf);
		setStaticText(CTL_LOCATION_ST_ADDR, temp_buf);
	}
	disableCtrl(CTL_LOCATION_ID, true);
	return ok;
}

int LocationDialog::getDTS(PPLocationPacket * pData)
{
	int    ok = 1;
	ushort v = 0;
	SString temp_buf;
	getCtrlData(CTL_LOCATION_NAME,     Data.Name);
	getCtrlData(CTL_LOCATION_CODE,     Data.Code);
	getCtrlData(CTLSEL_LOCATION_OWNER, &Data.OwnerID);
	getCtrlData(CTLSEL_LOCATION_RSPNS, &Data.RspnsPersonID);
	getCtrlData(CTLSEL_LOCATION_CITY,  &Data.CityID);
	if(LocTyp == LOCTYP_ADDRESS) {
		GetClusterData(CTL_LOCATION_FLAGS, &Data.Flags);
		GetClusterData(CTL_LOCATION_PASSIVE, &Data.Flags); // @v11.9.9
		getCtrlString(CTL_LOCATION_ZIP, temp_buf);
		LocationCore::SetExField(&Data, LOCEXSTR_ZIP, temp_buf);
		getCtrlString(CTL_LOCATION_ADDR, temp_buf);
		LocationCore::SetExField(&Data, LOCEXSTR_SHORTADDR, temp_buf);
		if(Data.Flags & LOCF_MANUALADDR)
			getCtrlString(CTL_LOCATION_FULLADDR, temp_buf);
		else
			temp_buf.Z();
		LocationCore::SetExField(&Data, LOCEXSTR_FULLADDR, temp_buf);
		getCtrlString(CTL_LOCATION_PHONE, temp_buf);
		LocationCore::SetExField(&Data, LOCEXSTR_PHONE, temp_buf);
		getCtrlString(CTL_LOCATION_CONTACT, temp_buf);
		LocationCore::SetExField(&Data, LOCEXSTR_CONTACT, temp_buf);
		getCtrlString(CTL_LOCATION_EMAIL, temp_buf);
		LocationCore::SetExField(&Data, LOCEXSTR_EMAIL, temp_buf);
		THROW(GetGeoCoord());
	}
	else if(oneof4(LocTyp, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL, LOCTYP_WHCELLAUTOGEN)) {
		getCtrlData(CTLSEL_LOCATION_PARENT, &Data.ParentID);
		getCtrlData(CTL_LOCATION_NUMROWS,   &Data.NumRows);
		getCtrlData(CTL_LOCATION_NUMLAYERS, &Data.NumLayers);
		getCtrlData(CTL_LOCATION_DEPTH,     &Data.Depth);
		Data.MassCapacity = (long)(getCtrlReal(CTL_LOCATION_CAPACITY) * 1000.0);
		GetClusterData(CTL_LOCATION_VOLUMEVAL, &Data.Flags);
		if(Data.Flags & LOCF_VOLUMEVAL) {
			double volume = getCtrlReal(CTL_LOCATION_VOLUME);
			Data.X = 100L;
			Data.Y = 100L;
			Data.Z = R0i(volume * 100000.0);
		}
		else {
			Data.X = getCtrlLong(CTL_LOCATION_SZWD);
			Data.Y = getCtrlLong(CTL_LOCATION_SZLN);
			Data.Z = getCtrlLong(CTL_LOCATION_SZHT);
		}
		{
			long   flags = Data.Flags;
			GetClusterData(CTL_LOCATION_WHFLAGS, &flags);
			if(ReqAutoName() >= 0) {
				SETFLAGBYSAMPLE(Data.Flags, LOCF_WHAUTONAME, flags);
			}
		}
	}
	else {
		getCtrlData(CTLSEL_LOCATION_PARENT, &Data.ParentID);
		GetClusterData(CTL_LOCATION_FLAGS, &Data.Flags);
	}
	strip(Data.Name);
	strip(Data.Code);
	ASSIGN_PTR(pData, Data);
	CATCHZOK
	return ok;
}

void LocationDialog::SetupCtrls()
{
	if(LocTyp == LOCTYP_ADDRESS) {
		long   flags = 0;
		GetClusterData(CTL_LOCATION_FLAGS, &flags);
		disableCtrl(CTL_LOCATION_FULLADDR, !(flags & LOCF_MANUALADDR));
		if(!(flags & LOCF_MANUALADDR)) {
			SString buf;
			setCtrlString(CTL_LOCATION_FULLADDR, buf);
			CopyFullAddr();
		}
	}
}

int LocationDialog::CopyFullAddr()
{
	SString addr_buf;
	PPLocationPacket addr;
	getDTS(&addr);
	addr.Flags &= ~LOCF_MANUALADDR;
	LocationCore::GetAddress(addr, 0, addr_buf);
	setCtrlString(CTL_LOCATION_FULLADDR, addr_buf);
	return 1;
}

int PPObjLocation::EditDialog(PPID locTyp, LocationTbl::Rec * pData)
{
    PPLocationPacket temp_pack;
	temp_pack = *pData;
    int    ok = EditDialog(locTyp, &temp_pack, edfMainRecOnly);
    if(ok > 0) {
    	ASSIGN_PTR(pData, temp_pack);
    }
    return ok;
}

int PPObjLocation::EditDialog(PPID locTyp, PPLocationPacket * pData, long flags)
{
	int    ok = cmCancel;
	uint   dlg_id;
	LocationDialog * dlg = 0;
	switch(locTyp) {
		case LOCTYP_WAREHOUSEGROUP:
		case LOCTYP_WAREHOUSE: dlg_id = DLG_LOCATION2; break;
		case LOCTYP_WAREPLACE: dlg_id = DLG_WAREPLACE; break;
		case LOCTYP_WHCOLUMN:  dlg_id = DLG_WHCOLUMN;  break;
		case LOCTYP_WHCELL:    dlg_id = DLG_WHCELL;    break;
		case LOCTYP_WHCELLAUTOGEN: dlg_id = DLG_WHCELLSAMPLE; break;
		case LOCTYP_ADDRESS:   dlg_id = DLG_ADDRESS;   break;
		case LOCTYP_DIVISION:  dlg_id = DLG_DIVISION;  break;
		default: return (PPError(PPERR_INVPARAM, 0), 0);
	}
	if(CheckDialogPtrErr(&(dlg = new LocationDialog(dlg_id, locTyp, flags)))) {
		pData->Type = static_cast<int16>(locTyp);
		if(locTyp == LOCTYP_DIVISION) {
			SETIFZ(pData->OwnerID, GetMainOrgID());
		}
		else if(locTyp == LOCTYP_WAREHOUSE && pData->OwnerID == 0)
			dlg->disableCtrl(CTLSEL_LOCATION_OWNER, true);
		dlg->setDTS(pData);
		for(int valid_data = 0; !valid_data && (ok = ExecView(dlg)) == cmOK;)
			if(dlg->getDTS(pData) && Validate(pData, 0))
				valid_data = 1;
			else
			   	PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjLocation::IsPacketEq(const PPLocationPacket & rS1, const PPLocationPacket & rS2, long flags)
{
	int    eq = 1;
	if(!LocationCore::IsEqualRec(rS1, rS2))
		eq = 0;
	else if(!rS1.Regs.IsEq(rS2.Regs))
		eq = 0;
	else if(!rS1.TagL.IsEq(rS2.TagL))
		eq = 0;
	else if(!rS1.WarehouseList.IsEq(rS2.WarehouseList)) // @v11.0.6
		eq = 0;
	return eq;
}

int PPObjLocation::GetPacket(PPID id, PPLocationPacket * pPack)
{
	int    ok = 1;
	pPack->destroy();
	if(PPCheckGetObjPacketID(Obj, id)) {
		Reference * p_ref = PPRef;
		int    sr = Search(id, pPack);
		THROW(sr);
		if(sr > 0) {
			THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister));
			THROW(P_RegObj->P_Tbl->GetByLocation(id, &pPack->Regs));
			THROW(p_ref->Ot.GetList(Obj, id, &pPack->TagL));
			// @v11.0.6 {
			if(pPack->Type == LOCTYP_DIVISION) {
				PPIDArray wh_list;
				THROW(p_ref->GetPropArray(Obj, id, LOCPRP_WAREHOUSELIST, &wh_list));
				pPack->WarehouseList.Set(wh_list.getCount() ? &wh_list : 0);
			}
			// } @v11.0.6 
		}
		else
			ok = -1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjLocation::PutPacket(PPID * pID, PPLocationPacket * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	const  bool do_index_phones = LOGIC(CConfig.Flags2 & CCFLG2_INDEXEADDR);
	SString temp_buf;
	THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister()));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			PPLocationPacket org_pack;
			THROW(GetPacket(*pID, &org_pack) > 0);
			if(pPack) {
				if(!IsPacketEq(org_pack, *pPack, 0)) {
					//
					// ��������� ������� ������� ��� ������������� ������ (�� @v5.6.6), ��-��
					// ������� � ������������ ����������� ������ ������ � ����� 0.
					//
					if(org_pack.Type != 0 || !org_pack.IsEmptyAddress()) {
						THROW_PP_S(!org_pack.Type || org_pack.Type == pPack->Type, PPERR_LOCTYPECHNG, org_pack.Name);
					}
					if(pPack->Type == LOCTYP_ADDRESS && pPack->IsEmptyAddress()) {
						if(do_index_phones) {
							LocationCore::GetExField(&org_pack, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
						}
						THROW(p_ref->Ot.PutList(Obj, *pID, 0, 0));
						THROW(P_RegObj->P_Tbl->PutByLocation(*pID, 0, 0));
						THROW(RemoveByID(P_Tbl, *pID, 0));
						DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
						*pID = 0;
					}
					else {
						if(!sstreq(org_pack.Name, pPack->Name)) {
							THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, Obj, *pID, pPack->Name, 0) == DBRPL_OK);
						}
						if(do_index_phones) {
							LocationCore::GetExField(pPack, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 0, 0));
						}
						THROW(UpdateByID(P_Tbl, Obj, *pID, pPack, 0));
						THROW(p_ref->Ot.PutList(Obj, *pID, &pPack->TagL, 0));
						THROW(P_RegObj->P_Tbl->PutByLocation(*pID, &pPack->Regs, 0));
						// @v11.0.6 {
						if(pPack->Type == LOCTYP_DIVISION) {
							THROW(p_ref->PutPropArray(Obj, *pID, LOCPRP_WAREHOUSELIST, pPack->WarehouseList.GetP(), 0));
						}
						// } @v11.0.6 
						DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
					}
				}
				else
					ok = -1;
			}
			else {
				if(do_index_phones) {
					LocationCore::GetExField(&org_pack, LOCEXSTR_PHONE, temp_buf);
					PPObjID objid(PPOBJ_LOCATION, *pID);
					THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
				}
				THROW(p_ref->Ot.PutList(Obj, *pID, 0, 0));
				THROW(P_RegObj->P_Tbl->PutByLocation(*pID, 0, 0));
				THROW(p_ref->PutPropArray(Obj, *pID, LOCPRP_WAREHOUSELIST, 0, 0)); // @v11.0.6 
				THROW(RemoveByID(P_Tbl, *pID, 0));
				THROW(RemoveSync(*pID));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0)
				Dirty(*pID);
		}
		else if(pPack) {
			if(!oneof2(pPack->Type, LOCTYP_ADDRESS, 0) || !pPack->IsEmptyAddress()) {
				PPID   temp_id = 0;
				//
				// ������� LocationCore::Add ��� �������� ��������� ���������� ��������� (IndexPhone)
				//
				THROW(P_Tbl->Add(&temp_id, pPack, 0));
				THROW(p_ref->Ot.PutList(Obj, temp_id, &pPack->TagL, 0));
				THROW(P_RegObj->P_Tbl->PutByLocation(temp_id, &pPack->Regs, 0));
				// @v11.0.6 {
				if(pPack->Type == LOCTYP_DIVISION) {
					THROW(p_ref->PutPropArray(Obj, temp_id, LOCPRP_WAREHOUSELIST, pPack->WarehouseList.GetP(), 0));
				}
				// } @v11.0.6 
				if(pPack->Type == LOCTYP_WAREHOUSE)
					THROW(SendObjMessage(DBMSG_WAREHOUSEADDED, PPOBJ_ARTICLE, Obj, temp_id) == DBRPL_OK);
				DS.LogAction(PPACN_OBJADD, Obj, temp_id, 0, 0);
				*pID = temp_id;
			}
			else
				ok = -1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::PutRecord(PPID * pID, LocationTbl::Rec * pPack, int use_ta)
{
	int    ok = 1;
	const  int do_index_phones = BIN(CConfig.Flags2 & CCFLG2_INDEXEADDR);
	SString temp_buf;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			LocationTbl::Rec org_rec;
			THROW(Search(*pID, &org_rec) > 0);
			if(pPack) {
				if(!LocationCore::IsEqualRec(org_rec, *pPack)) {
					//
					// ��������� ������� ������� ��� ������������� ������ (�� @v5.6.6), ��-��
					// ������� � ������������ ����������� ������ ������ � ����� 0.
					//
					if(org_rec.Type != 0 || !LocationCore::IsEmptyAddressRec(org_rec)) {
						THROW_PP_S(!org_rec.Type || org_rec.Type == pPack->Type, PPERR_LOCTYPECHNG, org_rec.Name);
					}
					if(pPack->Type == LOCTYP_ADDRESS && LocationCore::IsEmptyAddressRec(*pPack)) {
						if(do_index_phones) {
							LocationCore::GetExField(&org_rec, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
						}
						THROW(RemoveByID(P_Tbl, *pID, 0));
						DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
						*pID = 0;
					}
					else {
						if(!sstreq(org_rec.Name, pPack->Name))
							THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, Obj, *pID, pPack->Name, 0) == DBRPL_OK);
						if(do_index_phones) {
							LocationCore::GetExField(pPack, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 0, 0));
						}
						pPack->Counter = org_rec.Counter;
						THROW(UpdateByID(P_Tbl, Obj, *pID, pPack, 0));
						DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
					}
				}
				else
					ok = -1;
			}
			else {
				// @v12.2.1 {
				if(org_rec.Type == LOCTYP_WAREHOUSEGROUP) {
					PPLocationConfig loc_cfg;
					ReadConfig(&loc_cfg);
					if(loc_cfg.MarketplaceWarehoustFolderID == *pID) {
						PPSetObjError(PPERR_REFSEXISTS, PPCFGOBJ_LOCATION, 0);
						CALLEXCEPT();
					}
				}
				// } @v12.2.1 
				if(do_index_phones) {
					LocationCore::GetExField(&org_rec, LOCEXSTR_PHONE, temp_buf);
					PPObjID objid(PPOBJ_LOCATION, *pID);
					THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
				}
				THROW(RemoveByID(P_Tbl, *pID, 0));
				THROW(RemoveSync(*pID));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0)
				Dirty(*pID);
		}
		else if(pPack) {
			if(!oneof2(pPack->Type, LOCTYP_ADDRESS, 0) || !LocationCore::IsEmptyAddressRec(*pPack)) {
				PPID   temp_id = 0;
				//
				// ������� LocationCore::Add ��� �������� ��������� ���������� ��������� (IndexPhone)
				//
				THROW(P_Tbl->Add(&temp_id, pPack, 0));
				if(pPack->Type == LOCTYP_WAREHOUSE)
					THROW(SendObjMessage(DBMSG_WAREHOUSEADDED, PPOBJ_ARTICLE, Obj, temp_id) == DBRPL_OK);
				DS.LogAction(PPACN_OBJADD, Obj, temp_id, 0, 0);
				*pID = temp_id;
			}
			else
				ok = -1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::PutGuid(PPID id, const S_GUID * pUuid, int use_ta)
{
	const  PPID tag_id = PPTAG_LOC_UUID;
	const  int  abs_err_msg_id = PPERR_LOCATIONTAGUUIDABS;
	int    ok = 1;
	ObjTagItem tag;
	LocationTbl::Rec _rec;
	PPObjTag tagobj;
	PPObjectTag tag_rec;
	THROW_PP(tagobj.Fetch(tag_id, &tag_rec) > 0, abs_err_msg_id);
	if(!S_GUID::IsEmpty(pUuid)) {
		THROW(Search(id, &_rec) > 0);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(tag.SetGuid(tag_id, pUuid));
		THROW(PPRef->Ot.PutTag(Obj, id, &tag, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::InitCode(LocationTbl::Rec * pRec)
{
	int    ok = -1;
	PPLocationConfig cfg;
	if(pRec && PPObjLocation::FetchConfig(&cfg) > 0) {
		SString code;
		if(pRec->Type == LOCTYP_ADDRESS) {
			if(cfg.AddrCodeTempl[0]) {
				if(P_Tbl->GetCodeByTemplate(cfg.AddrCodeTempl, code) > 0) {
					code.CopyTo(pRec->Code, sizeof(pRec->Code));
				}
			}
		}
		else {
			int    coding = 0;
			if(pRec->Type == LOCTYP_WHZONE)
				coding = cfg.WhZoneCoding;
			else if(pRec->Type == LOCTYP_WHCOLUMN)
				coding = cfg.WhColCoding;
			else if(pRec->Type == LOCTYP_WHCELL)
				coding = cfg.WhCellCoding;
			if(coding) {
				PPIDArray bycode_list;
				long   wh_flags = 0;
				PPID   parent_wh_id = 0;
				{
					LocationTbl::Rec parent_rec;
					if(pRec->ParentID && GetParentWarehouse(pRec->ParentID, &parent_wh_id) > 0 && Fetch(parent_wh_id, &parent_rec) > 0)
						wh_flags = parent_rec.Flags;
					else
						parent_wh_id = 0;
				}
				for(long i = 1; ok < 0 && i < 10000; i++) {
					code.Z();
					switch(coding) {
						case whzcAlpha:
							code.NumberToLat(static_cast<uint>(i));
							break;
						case whzcDigit1:
							THROW(i <= 9);
							code.CatLongZ(i, 1);
							break;
						case whzcDigit2:
							THROW(i <= 99);
							code.CatLongZ(i, 2);
							break;
						case whzcDigit3:
							THROW(i <= 999);
							code.CatLongZ(i, 3);
							break;
						case whzcDigit4:
							THROW(i <= 9999);
							code.CatLongZ(i, 4);
							break;
					}
					if(code.NotEmpty()) {
						LocationTbl::Rec cod_rec;
						bycode_list.clear();
						ok = 1;
						if(P_Tbl->GetListByCode(pRec->Type, code, &bycode_list) > 0)
							for(uint j = 0; ok > 0 && j < bycode_list.getCount(); j++) {
								if(Fetch(bycode_list.get(j), &cod_rec) > 0) {
									if(pRec->ParentID) {
										if(wh_flags & LOCF_SEQCOLCODE && cod_rec.Type == LOCTYP_WHCOLUMN && pRec->Type == LOCTYP_WHCOLUMN) {
											PPID   _wh_id = 0;
											GetParentWarehouse(cod_rec.ID, &_wh_id);
											if(_wh_id == parent_wh_id)
												ok = -1;
										}
										else if(cod_rec.ParentID == pRec->ParentID)
											ok = -1;
									}
									else
										ok = -1;
								}
							}
						if(ok > 0) {
							long   mcsf = 0;
							if(wh_flags & LOCF_WHCODEPREFIX)
								mcsf |= mcsWhCodePrefix;
							code.CopyTo(pRec->Code, sizeof(pRec->Code));
							MakeCodeString(pRec, mcsf, code);
							code.CopyTo(pRec->Name, sizeof(pRec->Name));
						}
					}
					else
						break;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::GetRegister(PPID locID, PPID regType, LDATE actualDate, bool inheritFromOwner, RegisterTbl::Rec * pRec)
{
	int    ok = -1;
	RegisterArray reg_list;
	THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister()));
	if(P_RegObj->P_Tbl->GetByLocation(locID, &reg_list) > 0) {
		if(reg_list.GetRegister(regType, actualDate, 0, pRec) > 0) {
			ok = 1;
		}
	}
	if(ok < 0 && inheritFromOwner) {
		LocationTbl::Rec loc_rec;
        if(Search(locID, &loc_rec) > 0 && loc_rec.OwnerID) {
			PPObjPerson psn_obj;
			int r = psn_obj.GetRegister(loc_rec.OwnerID, regType, actualDate, pRec);
			if(r > 0)
				ok = 2;
        }
	}
	CATCHZOK
	return ok;
}

PPObjLocation::CreateWhLocParam::CreateWhLocParam() : Type(0)
{
}

/*struct CreateWhLocParam {
	CreateWhLocParam() : Type(0)
	{
	}
	PPID   Type;
	LocationTbl::Rec CellSample;
};*/

int PPObjLocation::EditCreateWhLocParam(CreateWhLocParam * pParam)
{
	class CreateWhLocParamDialog : public TDialog {
		DECL_DIALOG_DATA(CreateWhLocParam);
	public:
		CreateWhLocParamDialog() : TDialog(DLG_SELWZTYP)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_SELWZTYP_WHAT, 0, LOCTYP_WHZONE);
			AddClusterAssoc(CTL_SELWZTYP_WHAT, 1, LOCTYP_WHCOLUMN);
			AddClusterAssoc(CTL_SELWZTYP_WHAT, 2, LOCTYP_WHCELL);
			AddClusterAssoc(CTL_SELWZTYP_WHAT, 3, LOCTYP_WHCELLAUTOGEN);
			SetClusterData(CTL_SELWZTYP_WHAT, Data.Type);
			enableCommand(cmGenWhCellSample, Data.Type == LOCTYP_WHCELLAUTOGEN);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			GetClusterData(CTL_SELWZTYP_WHAT, &Data.Type);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmGenWhCellSample)) {
				GetClusterData(CTL_SELWZTYP_WHAT, &Data.Type);
				if(Data.Type == LOCTYP_WHCELLAUTOGEN) {
					LocObj.EditDialog(LOCTYP_WHCELLAUTOGEN, &Data.CellSample);
				}
			}
			else if(event.isClusterClk(CTL_SELWZTYP_WHAT)) {
				GetClusterData(CTL_SELWZTYP_WHAT, &Data.Type);
				enableCommand(cmGenWhCellSample, Data.Type == LOCTYP_WHCELLAUTOGEN);
			}
			else
				return;
			clearEvent(event);
		}
		PPObjLocation LocObj;
	};
	DIALOG_PROC_BODY(CreateWhLocParamDialog, pParam);
}

int PPObjLocation::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	PPID   loc_typ = 0;
	PPLocationPacket pack;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
		loc_typ = pack.Type;
	}
	else {
		uint   t = 0;
		LocationTbl::Rec cur_rec;
		if(extraPtr) {
			LocationFilt flt = *static_cast<const LocationFilt *>(extraPtr);
			loc_typ = NZOR(flt.LocType, LOCTYP_WAREHOUSE);
			pack.Type = static_cast<int16>(loc_typ);
			pack.OwnerID  = flt.Owner;
			pack.ParentID = flt.Parent;
			if(flt.CurrID == 0 || Fetch(flt.CurrID, &cur_rec) <= 0)
				cur_rec.ID = 0;
		}
		else
			loc_typ = pack.Type = LOCTYP_WAREHOUSE;
		if(loc_typ == LOCTYP_WAREHOUSE) {
			if(SelectorDialog(DLG_SELWHTYP, CTL_SELWHTYP_WHAT, &t, 0) > 0) {
				if(t == 1)
					loc_typ = pack.Type = LOCTYP_WAREHOUSEGROUP;
				if(cur_rec.ID && cur_rec.Type == LOCTYP_WAREHOUSEGROUP)
					pack.ParentID = cur_rec.ID;
			}
			else
				loc_typ = 0;
		}
		else if(loc_typ == LOCTYP_WHZONE) {
			CreateWhLocParam cwlp;
			cwlp.CellSample.ParentID = cur_rec.ID;
			if(cur_rec.ID && cur_rec.Type == LOCTYP_WHCELL) {
				loc_typ = pack.Type = LOCTYP_WHCELL;
				pack.ParentID = cur_rec.ParentID;
			}
			else if(EditCreateWhLocParam(&cwlp) > 0) {
				if(cwlp.Type == LOCTYP_WHZONE) {
					loc_typ = pack.Type = LOCTYP_WHZONE;
					if(cur_rec.ID && cur_rec.Type == LOCTYP_WAREHOUSE)
						pack.ParentID = cur_rec.ID;
				}
				else if(cwlp.Type == LOCTYP_WHCOLUMN) {
					loc_typ = pack.Type = LOCTYP_WHCOLUMN;
					if(cur_rec.ID)
						if(cur_rec.Type == LOCTYP_WHZONE)
							pack.ParentID = cur_rec.ID;
						else if(cur_rec.Type == LOCTYP_WHCOLUMN)
							pack.ParentID = cur_rec.ParentID;
				}
				else if(cwlp.Type == LOCTYP_WHCELL) {
					loc_typ = pack.Type = LOCTYP_WHCELL;
					if(cur_rec.ID && cur_rec.Type == LOCTYP_WHCOLUMN)
						pack.ParentID = cur_rec.ID;
				}
				else if(cwlp.Type == LOCTYP_WHCELLAUTOGEN) {
					THROW(GenerateWhCells(cwlp.CellSample.ParentID, &cwlp.CellSample, 1));
					ok = cmOK;
					loc_typ = 0;
				}
			}
			else
				loc_typ = 0;
		}
	}
	if(loc_typ) {
		if(*pID == 0)
			InitCode(&pack);
		while(ok != cmOK && EditDialog(loc_typ, &pack, 0) == cmOK) {
			ok = PutPacket(pID, &pack, 1) ? cmOK : PPErrorZ();
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjLocation::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PPID   ref_id = 0;
		if(_obj == PPOBJ_PERSON) {
			if(P_Tbl->SearchRef(LOCTYP_WAREHOUSE, PPOBJ_PERSON, _id, &ref_id) > 0 ||
				P_Tbl->SearchRef(LOCTYP_DIVISION, PPOBJ_PERSON, _id, &ref_id) > 0) {
				THROW(RetRefsExistsErr(Obj, ref_id));
			}
		}
		else if(_obj == PPOBJ_WORLD) {
			if(P_Tbl->SearchRef(0, PPOBJ_WORLD, _id, &ref_id) > 0)
				THROW(RetRefsExistsErr(Obj, ref_id));
		}
		else if(_obj == PPOBJ_LOCATION) {
			if(P_Tbl->SearchRef(0, PPOBJ_LOCATION, _id, &ref_id) > 0)
				THROW(RetRefsExistsErr(Obj, ref_id));
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == Obj) {
			THROW(BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, _id, extraPtr));
		}
		else if(_obj == PPOBJ_PERSON) {
			const long types[] = { LOCTYP_WAREHOUSE, LOCTYP_DIVISION, LOCTYP_ADDRESS };
			for(uint i = 0; i < SIZEOFARRAY(types); i++) {
				THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->Type == types[i] && P_Tbl->OwnerID == _id),
					set(P_Tbl->OwnerID, dbconst(reinterpret_cast<long>(extraPtr)))));
			}
		}
		else if(_obj == PPOBJ_WORLD) {
			if(_id) {
				THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->CityID == _id), set(P_Tbl->CityID, dbconst(reinterpret_cast<long>(extraPtr)))));
			}
		}
	}
	else if(msg == DBMSG_PERSONLOSEKIND) {
		if(_id == PPPRK_EMPLOYER) {
			LocationTbl::Key3 k;
			k.Type = LOCTYP_DIVISION;
			k.OwnerID = _obj;
			k.Counter = 0;
			if(P_Tbl->search(3, &k, spGt) && k.Type == LOCTYP_DIVISION && k.OwnerID == _obj)
				THROW(RetRefsExistsErr(Obj, P_Tbl->data.ID));
		}
	}
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjLocation, PPLocationPacket);

int PPObjLocation::SerializePacket(int dir, PPLocationPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister()));
	THROW_SL(P_Tbl->SerializeRecord(dir, pPack, rBuf, pSCtx));
	THROW_SL(P_RegObj->P_Tbl->SerializeArrayOfRecords(dir, &pPack->Regs, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjLocation::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPLocationPacket);
	{
		PPLocationPacket * p_pack = static_cast<PPLocationPacket *>(p->Data);
		if(stream == 0) {
			THROW(GetPacket(id, p_pack) > 0);
			if(p_pack->Type == LOCTYP_ADDRESS)
				p->Priority = PPObjectTransmit::DependedPriority;
		}
		else {
			SBuffer buffer;
			THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0));
			THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1, r = 1;
	if(p && p->Data) {
		PPLocationPacket * p_pack = static_cast<PPLocationPacket *>(p->Data);
		Reference * p_ref = PPRef;
		if(stream == 0) {
			PPID   same_id = 0;
			LocationTbl::Rec same_rec;
			if(*pID == 0) {
				p_pack->ID = 0;
				if(P_Tbl->SearchMaxLike(p_pack, 0, &same_id, &same_rec) > 0) {
					*pID = same_id;
					p_pack->ID = same_id;
					p_pack->Counter = same_rec.Counter;
					// (�� ��������� ������) r = PutPacket(pID, p_rec, 1);
				}
				else {
					r = PutPacket(pID, p_pack, 1);
				}
			}
			else if(Search(*pID, &same_rec) > 0) {
				p_pack->ID = *pID;
				p_pack->Counter = same_rec.Counter;
				if(p_pack->OwnerID != same_rec.OwnerID && p_pack->OwnerID && same_rec.OwnerID) {
					//
					// ������������ ����������� ��������: ��������� ������� ����� �������� ��
					// ������ �������� ��������� - ���������� �����������.
					//
					PPObjPerson psn_obj;
					PersonTbl::Rec native_psn_rec, foreign_psn_rec;
					if(psn_obj.Search(same_rec.OwnerID, &native_psn_rec) > 0) {
						if(psn_obj.Search(p_pack->OwnerID, &foreign_psn_rec) > 0) {
							PPIDArray native_dlvr_loc_list;
							psn_obj.GetDlvrLocList(native_psn_rec.ID, &native_dlvr_loc_list);
							if(native_dlvr_loc_list.lsearch(*pID)) {
								// ����� ���� ����� ����� ������� �������� "������" ��������� - ���������� ��������� ������������� ���������
								p_pack->OwnerID = same_rec.OwnerID;
							}
						}
						else {
							// "�����" �������� �� ������ - ���������� ��������� �������������
							p_pack->OwnerID = same_rec.OwnerID;
						}
					}
				}
				// @v11.1.11 {
				if(p_pack->Type == LOCTYP_WAREHOUSE) {
					//
					// ����������� ������: ������ ������ ��� �����-�������������� ������ - ��� ������������� �������� ��� ������� �������
					//
					ObjTagList ex_tag_list;
					p_ref->Ot.GetList(Obj, *pID, &ex_tag_list);
					const ObjTagItem * p_vetis_uuid_tag = ex_tag_list.GetItem(PPTAG_LOC_VETIS_GUID);
					p_pack->TagL.PutItem(PPTAG_LOC_VETIS_GUID, p_vetis_uuid_tag);
				}
				// } @v11.1.11 
				r = PutPacket(pID, p_pack, 1);
			}
			else {
				p_pack->ID = 0;
				r = PutPacket(pID, p_pack, 1);
			}
			if(!r) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTLOCATION, p_pack->ID, p_pack->Name);
				ok = -1;
			}
		}
		else {
			SBuffer buffer;
			THROW_SL(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjLocation::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPLocationPacket * p_pack = static_cast<PPLocationPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->ParentID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_WORLD,    &p_pack->CityID,   ary, replace));
		{
			int    dont_process_ref = 0;
			PPObjID oi;
			if(p_pack->Type == LOCTYP_ADDRESS && pCtx->GetPrevRestoredObj(&oi)) {
				if(oi.Obj == PPOBJ_PERSON)
					dont_process_ref = 1;
				else if(!replace) {
					pCtx->ForceRestore(PPObjID(PPOBJ_PERSON, p_pack->OwnerID));
				}
			}
			if(dont_process_ref) {
				THROW(ProcessObjRefInArray_NoPreprocess(PPOBJ_PERSON, &p_pack->OwnerID, ary, replace));
			}
			else {
				THROW(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->OwnerID, ary, replace));
			}
		}
		THROW(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->RspnsPersonID, ary, replace));
		THROW(p_pack->Regs.ProcessObjRefs(ary, replace));
		THROW(p_pack->TagL.ProcessObjRefs(ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjLocation::MakeReserved(long flags)
{
	int    ok = 1;
	LocationTbl::Key2 k2;
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_RESFAULT);
	MEMSZERO(k2);
	k2.Type = LOCTYP_WAREHOUSE;
	if(!(P_Tbl->search(2, &k2, spGe) && P_Tbl->data.Type == LOCTYP_WAREHOUSE)) {
		SString name;
		uint   num_recs;
		THROW_DB(BTROKORNFOUND);
		THROW_PP(p_rez->findResource(ROD_LOCATION, PP_RCDATA), PPERR_RESFAULT);
		THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
		for(uint i = 0; i < num_recs; i++) {
			PPID   id = 0;
			LocationTbl::Rec rec;
			p_rez->getUINT();
			p_rez->getString(name, 2);
			PPExpandString(name, CTRANSF_UTF8_TO_INNER);
			name.CopyTo(rec.Name, sizeof(rec.Name));
			rec.Type = LOCTYP_WAREHOUSE;
			THROW(P_Tbl->Add(&id, &rec, 1));
		}
	}
	CATCHZOK
	return ok;
}
//
// DivisionCtrlGroup
//
DivisionCtrlGroup::Rec::Rec(PPID orgID, PPID divID, PPID staffID, PPID postID) :
	OrgID(orgID), DivID(divID), StaffID(staffID), PostID(postID)
{
}

DivisionCtrlGroup::DivisionCtrlGroup(uint _ctlsel_org, uint _ctlsel_div, uint _ctlsel_staff, uint _ctlsel_post) :
	CtrlGroup(), DivF(LOCTYP_DIVISION), CtlselOrg(_ctlsel_org), CtlselDiv(_ctlsel_div), CtlselStaff(_ctlsel_staff),
	CtlselPost(_ctlsel_post), flags(0)
{
}

int DivisionCtrlGroup::setData(TDialog * dlg, void * data)
{
	Data = *static_cast<Rec *>(data);
	if(Data.OrgID == 0 && Data.DivID) {
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		Data.OrgID = (loc_obj.Search(Data.DivID, &loc_rec) > 0) ? loc_rec.OwnerID : 0;
	}
	DivF.LocType = LOCTYP_DIVISION;
	DivF.Owner   = Data.OrgID;
	DivF.Parent  = 0;
	SetupPPObjCombo(dlg, CtlselOrg, PPOBJ_PERSON,   Data.OrgID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPLOYER));
	SetupPPObjCombo(dlg, CtlselDiv, PPOBJ_LOCATION, Data.DivID, OLW_CANINSERT, &DivF);
	if(CtlselStaff)
		SetupStaffListCombo(dlg, CtlselStaff, Data.StaffID, OLW_CANINSERT, Data.OrgID, Data.DivID);
	if(CtlselPost)
		PPObjStaffList::SetupPostCombo(dlg, CtlselPost, Data.PostID, 0, Data.OrgID, Data.DivID, Data.StaffID);
	return 1;
}

int DivisionCtrlGroup::getData(TDialog * dlg, void * data)
{
	static_cast<Rec *>(data)->OrgID = dlg->getCtrlLong(CtlselOrg);
	static_cast<Rec *>(data)->DivID = dlg->getCtrlLong(CtlselDiv);
	static_cast<Rec *>(data)->StaffID = dlg->getCtrlLong(CtlselStaff);
	static_cast<Rec *>(data)->PostID = dlg->getCtrlLong(CtlselPost);
	return 1;
}

void DivisionCtrlGroup::handleEvent(TDialog * dlg, TEvent & event)
{
	if(event.isCbSelected(CtlselOrg)) {
		Data.OrgID = dlg->getCtrlLong(CtlselOrg);
		Data.DivID = 0;
		Data.StaffID = 0;
		DivF.LocType = LOCTYP_DIVISION;
		DivF.Owner   = Data.OrgID;
		DivF.Parent  = 0;
		SetupPPObjCombo(dlg, CtlselDiv, PPOBJ_LOCATION, Data.DivID, 0, &DivF);
		if(CtlselStaff) {
			SetupStaffListCombo(dlg, CtlselStaff, Data.StaffID, OLW_CANINSERT, Data.OrgID, Data.DivID);
			TView::messageCommand(dlg, cmCBSelected, dlg->getCtrlView(CtlselStaff));
		}
	}
	else if(event.isCbSelected(CtlselDiv)) {
		Data.DivID = dlg->getCtrlLong(CtlselDiv);
		Data.StaffID = 0;
		if(CtlselStaff) {
			SetupStaffListCombo(dlg, CtlselStaff, Data.StaffID, OLW_CANINSERT, Data.OrgID, Data.DivID);
			TView::messageCommand(dlg, cmCBSelected, dlg->getCtrlView(CtlselStaff));
		}
	}
	else if(event.isCbSelected(CtlselStaff)) {
		Data.StaffID = dlg->getCtrlLong(CtlselStaff);
		if(CtlselPost) {
			Data.OrgID = dlg->getCtrlLong(CtlselOrg);
			Data.DivID = dlg->getCtrlLong(CtlselDiv);
			Data.PostID = 0;
			PPObjStaffList::SetupPostCombo(dlg, CtlselPost, Data.PostID, 0, Data.OrgID, Data.DivID, Data.StaffID);
			TView::messageCommand(dlg, cmCBSelected, dlg->getCtrlView(CtlselPost));
		}
	}
	else
		return;
	dlg->clearEvent(event);
}
//
//
//
class DivisionView : public PPListDialog {
public:
	DivisionView() : PPListDialog(DLG_DIVVIEW, CTL_DIVVIEW_LIST), CurOrgID(0)
	{
		GetMainEmployerID(&CurOrgID);
		SetupPPObjCombo(this, CTLSEL_DIVVIEW_ORG, PPOBJ_PERSON, CurOrgID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPLOYER));
		updateList(-1);
	}
private:
	virtual int  setupList();
	virtual int  addItem(long * pos, long * id);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	DECL_HANDLE_EVENT;
	void    viewStaffList();

	PPID   CurOrgID;
	PPObjLocation LocObj;
};

IMPL_HANDLE_EVENT(DivisionView)
{
	PPListDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_DIVVIEW_ORG))
		updateList(0);
	else if(event.isCmd(cmaMore))
		viewStaffList();
	else
		return;
	clearEvent(event);
}

int DivisionView::setupList()
{
	int    ok = 1;
	getCtrlData(CTLSEL_DIVVIEW_ORG, &CurOrgID);
	LocationTbl::Rec loc_rec;
	SEnum en = LocObj.P_Tbl->Enum(LOCTYP_DIVISION, CurOrgID, LocationCore::eoParentAsOwner);
	while(en.Next(&loc_rec) > 0) {
		if(!addStringToList(loc_rec.ID, loc_rec.Name))
			ok = 0;
	}
	return ok;

}

int DivisionView::addItem(long *, long * pID)
{
	int    ok = -1;
	PPID   obj_id = 0;
	LocationFilt flt(LOCTYP_DIVISION, CurOrgID);
	int    r = LocObj.Edit(&obj_id, &flt);
	if(r == cmOK) {
		ASSIGN_PTR(pID, obj_id);
		ok = 1;
	}
	else if(!r)
		ok = 0;
	return ok;
}

int DivisionView::editItem(long, long id)
{
	int    r = id ? LocObj.Edit(&id, 0) : -1;
	if(r == cmOK)
		return 1;
	else if(!r)
		return 0;
	else
		return -1;
}

int DivisionView::delItem(long, long id) { return (id && LocObj.RemoveObjV(id, 0, PPObject::rmv_default, 0)) ? 1 : -1; }

void DivisionView::viewStaffList()
{
	PPID   div_id = 0;
	if(getCurItem(0, &div_id) && div_id) {
		PPObjStaffList slobj;
		PPObjStaffList::Filt flt;
		flt.OrgID = CurOrgID;
		flt.DivID = div_id;
		slobj.Browse(&flt);
	}
}
//
//
//
/*static*/int PPObjLocation::ViewWarehouse()
{
	PPObjLocation locobj(0);
	locobj.Browse(0);
	return 1;
}

/*static*/int PPObjLocation::ViewDivision()
{
	DivisionView * dlg = new DivisionView();
	return CheckDialogPtrErr(&dlg) ? ExecViewAndDestroy(dlg) : 0;
}
//
//
//
class LocationCache : public ObjCacheHash {
public:
	LocationCache() : ObjCacheHash(PPOBJ_LOCATION, sizeof(LocationData), 1024*1024, 4),
		WhObjList(sizeof(WHObjEntry)), FullEaList(BIN(CConfig.Flags2 & CCFLG2_INDEXEADDR)), IsWhObjTabInited(0)
	{
		LoadWarehouseTab();
		// @v12.2.1 @ctr MEMSZERO(Cfg);
	}
	PPID   GetSingleWarehouse();
	uint   GetWarehouseList(PPIDArray * pList, bool * pHasRestrictions);
	int    CheckWarehouseFlags(PPID locID, long f);
	PPID   FASTCALL ObjToWarehouse(PPID arID, int ignoreRights);
	PPID   FASTCALL WarehouseToObj(PPID locID, int ignoreRights);
	int    GetConfig(PPLocationConfig * pCfg, int enforce);
	int    GetCellList(PPID locID, PPIDArray * pList); // @sync_w
	int    DirtyCellList(PPID locID); // @sync_w
	int    ReleaseFullEaList(const StrAssocArray * pList);
	const  StrAssocArray * GetFullEaList();
	virtual void FASTCALL Dirty(PPID); // @sync_w
private:
	struct WHObjEntry { // @flat
		PPID   LocID;
		PPID   ObjID;
		long   Flags;
	};
	struct WHCellEntry {
		PPID   LocID;
		PPIDArray List;
	};
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	int    LoadWarehouseTab();
	int    AddWarehouseEntry(const LocationTbl::Rec * pRec, PPID accSheetID);
	//
	// Descr: ������ ������ ����������� ������� �� ��������� EAddrCore
	//   ��� �������� ������ �� ���������
	//
	class FealArray : public StrAssocArray {
	public:
		explicit FealArray(int use) : StrAssocArray(), Use(use), Inited(0)
		{
		}
		void   FASTCALL Dirty(PPID cardID)
		{
			DirtyTable.Add((uint32)labs(cardID));
		}
		int    Use;
		int    Inited;
		UintHashTable DirtyTable;
	};

	FealArray FullEaList;
	ReadWriteLock FealLock;
	int    IsWhObjTabInited;
	SVector WhObjList;
	TSCollection <WHCellEntry> WhCellList;
	PPLocationConfig Cfg;
	ReadWriteLock CfgLock;
	ReadWriteLock WhclLock; // ���������� ��� WhCellList.
public:
	struct LocationData : public ObjCacheEntry {
		PPID   ParentID; // ������������ ������� //
		int16  Type;     //
		int16  Access;   // 0 - ������������ �� ����� ���� ������� � ����� ������; 1 - ������������ ����� ����� ������� � ����� ������.
		PPID   OwnerID;
		PPID   CityID;
		PPID   RspnsPersonID;
		PPID   ArID;
		int16  Flags;
		int16  NumRows;
		int16  NumLayers;
		int16  Depth;
		double Latitude;  // @v11.6.1
		double Longitude; // @v11.6.1
		long   Counter;
	};
};

const StrAssocArray * LocationCache::GetFullEaList()
{
	int    err = 0;
	const  StrAssocArray * p_result = 0;
	if(FullEaList.Use) {
		{
			SRWLOCKER(FealLock, SReadWriteLocker::Read);
			if(!FullEaList.Inited || FullEaList.DirtyTable.GetCount()) {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				if(!FullEaList.Inited || FullEaList.DirtyTable.GetCount()) {
					PPObjLocation loc_obj(SConstructorLite);
					EAddrTbl::Rec ea_rec;
					SString phone_buf;
					if(!FullEaList.Inited) {
						if(!loc_obj.P_Tbl->GetFullEaList(FullEaList))
							err = 1;
					}
					else {
						for(ulong id = 0; !err && FullEaList.DirtyTable.Enum(&id);) {
							if(loc_obj.P_Tbl->GetEAddr(id, &ea_rec) > 0) {
								phone_buf = 0;
								reinterpret_cast<const PPEAddr *>(ea_rec.Addr)->GetPhone(phone_buf);
								if(phone_buf.NotEmpty()) {
									if(!FullEaList.Add(ea_rec.ID, phone_buf, 1)) {
										PPSetErrorSLib();
										err = 1;
									}
								}
							}
						}
					}
					if(!err) {
						FullEaList.DirtyTable.Clear();
						FullEaList.Inited = 1;
					}
				}
			}
		}
		if(!err) {
			#if SLTRACELOCKSTACK
			SLS.LockPush(SLockStack::ltRW_R, __FILE__, __LINE__);
			#endif
			FealLock.ReadLock_();
			p_result = &FullEaList;
		}
	}
	return p_result;
}

int LocationCache::ReleaseFullEaList(const StrAssocArray * pList)
{
	if(pList && pList == &FullEaList) {
		FealLock.Unlock_();
		#if SLTRACELOCKSTACK
		SLS.LockPop();
		#endif
	}
	return 1;
}

int LocationCache::GetCellList(PPID locID, PPIDArray * pList)
{
	int    ok = 0;
	uint   pos = 0;
	if(locID) {
		SRWLOCKER(WhclLock, SReadWriteLocker::Read);
		if(WhCellList.lsearch(&locID, &pos, CMPF_LONG)) {
			ASSIGN_PTR(pList, WhCellList.at(pos)->List);
			ok = 1;
		}
		else {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			//
			// ��������� ������� ������ ����� ��������� ����������
			//
			if(WhCellList.lsearch(&locID, &(pos = 0), CMPF_LONG)) {
				ASSIGN_PTR(pList, WhCellList.at(pos)->List);
				ok = 1;
			}
			else {
				WHCellEntry * p_entry = new WHCellEntry;
				if(p_entry) {
					PPObjLocation loc_obj(SConstructorLite);
					loc_obj.ResolveWhCell(locID, p_entry->List, 0, 0);
					p_entry->LocID = locID;
					WhCellList.insert(p_entry);
					ASSIGN_PTR(pList, p_entry->List);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPObjLocation::GetDirtyCellParentsList(PPID locID, PPIDArray & rDestList)
{
	int    ok = 1;
	PPID   wh_id = 0;
	LocationTbl::Rec zone_rec, col_rec;
	GetParentWarehouse(locID, &wh_id);
	if(wh_id) {
		rDestList.add(wh_id);
		SEnum en = P_Tbl->Enum(LOCTYP_WHZONE, wh_id, 0);
		while(en.Next(&zone_rec) > 0) {
			if(zone_rec.Type == LOCTYP_WHZONE) {
				rDestList.add(zone_rec.ID);
				SEnum en2 = P_Tbl->Enum(LOCTYP_WHCOLUMN, zone_rec.ID, 0);
				while(en2.Next(&col_rec) > 0) {
					if(col_rec.Type == LOCTYP_WHCOLUMN)
						rDestList.add(col_rec.ID);
				}
			}
		}
	}
	return ok;
}

int LocationCache::DirtyCellList(PPID locID)
{
	{
		SRWLOCKER(WhclLock, SReadWriteLocker::Write);
		{
			PPObjLocation loc_obj(SConstructorLite);
			PPIDArray par_list;
			loc_obj.GetDirtyCellParentsList(locID, par_list);
			for(uint i = 0; i < par_list.getCount(); i++) {
				const  PPID _id = par_list.get(i);
				uint pos = 0;
				if(WhCellList.lsearch(&_id, &pos, CMPF_LONG))
					WhCellList.atFree(pos);
			}
		}
	}
	return 1;
}

int LocationCache::GetConfig(PPLocationConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(CfgLock, SReadWriteLocker::Read);
		if(!(Cfg.Flags & PPLocationConfig::fValid) || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(Cfg.Flags & PPLocationConfig::fValid) || enforce) {
				PPObjLocation::ReadConfig(&Cfg);
				Cfg.Flags |= PPLocationConfig::fValid;
			}
		}
		ASSIGN_PTR(pCfg, Cfg);
	}
	return 1;
}

void FASTCALL LocationCache::Dirty(PPID locID)
{
	PPObjLocation loc_obj(SConstructorLite);
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		{
			LocationTbl::Rec loc_rec;
			uint pos = 0;
			if(loc_obj.Search(locID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE) {
				const  PPID acs_id = LConfig.LocAccSheetID;
				AddWarehouseEntry(&loc_rec, acs_id);
			}
			else if(WhObjList.lsearch(&locID, &pos, CMPF_LONG))
				WhObjList.atFree(pos);
			Helper_Dirty(locID);
		}
	}
}

int LocationCache::AddWarehouseEntry(const LocationTbl::Rec * pRec, PPID accSheetID)
{
	int    ok = -1;
	WHObjEntry entry;
	MEMSZERO(entry);
	entry.LocID = pRec->ID;
	entry.Flags = pRec->Flags;
	uint pos = 0;
	if(WhObjList.lsearch(&entry.LocID, &pos, CMPF_LONG))
		WhObjList.atFree(pos);
	{
		const int is_enabled = ObjRts.CheckLocID(entry.LocID, 0);
		if(!is_enabled) {
			entry.Flags |= LOCF_INTERNAL_DISABLED;
		}
		PPObjAccTurn * p_atobj = BillObj->atobj;
		SETIFZ(accSheetID, LConfig.LocAccSheetID); 
		ArticleTbl::Rec ar_rec;
		if(p_atobj->P_Tbl->Art.SearchObjRef(accSheetID, entry.LocID, &ar_rec) > 0)
			entry.ObjID = ar_rec.ID;
		ok = WhObjList.insert(&entry) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

int LocationCache::LoadWarehouseTab()
{
	int    ok = 1;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(!IsWhObjTabInited) {
			PPObjLocation lobj(SConstructorLite);
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!IsWhObjTabInited) {
				long   c = 0;
				LocationTbl::Rec loc_rec;
				PPObjAccTurn * p_atobj = BillObj->atobj;
				const  PPID acs_id = LConfig.LocAccSheetID;
				WhObjList.clear();
				for(SEnum en = lobj.P_Tbl->Enum(LOCTYP_WAREHOUSE, 0, LocationCore::eoIgnoreParent); en.Next(&loc_rec) > 0;) {
					AddWarehouseEntry(&loc_rec, acs_id);
				}
				if(ok)
					IsWhObjTabInited = 1;
			}
		}
	}
	return ok;
}

PPID LocationCache::GetSingleWarehouse()
{
	PPID   id = 0;
	LoadWarehouseTab();
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		//id = (WhObjList.getCount() == 1) ? ((WHObjEntry *)WhObjList.at(0))->LocID : 0;
		for(uint i = 0; i < WhObjList.getCount(); i++) {
			const WHObjEntry * p_entry = static_cast<const WHObjEntry *>(WhObjList.at(i));
			if(!(p_entry->Flags & LOCF_INTERNAL_DISABLED)) {
				if(!id)
					id = p_entry->LocID;
				else {
					id = 0;
					break;
				}
			}
		}
	}
	return id;
}

uint LocationCache::GetWarehouseList(PPIDArray * pList, bool * pHasRestrictions)
{
	uint   c = 0;
	bool   has_resrictions = false;
	LoadWarehouseTab();
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		WHObjEntry * p_entry;
		for(uint i = 0; WhObjList.enumItems(&i, (void **)&p_entry);) {
			if(!(p_entry->Flags & LOCF_INTERNAL_DISABLED)) {
				CALLPTRMEMB(pList, add(p_entry->LocID));
				c++;
			}
			else
				has_resrictions = true;
		}
	}
	CALLPTRMEMB(pList, sortAndUndup());
	ASSIGN_PTR(pHasRestrictions, has_resrictions);
	return c;
}

int LocationCache::CheckWarehouseFlags(PPID locID, long f)
{
	int    ok = 0;
	LoadWarehouseTab();
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		WHObjEntry * p_entry;
		for(uint i = 0; !ok && WhObjList.enumItems(&i, (void **)&p_entry);)
			if(p_entry->LocID == locID && p_entry->Flags & f)
				ok = 1;
	}
	return ok;
}

PPID FASTCALL LocationCache::ObjToWarehouse(PPID arID, int ignoreRights)
{
	PPID   id = 0;
	if(arID) {
		LoadWarehouseTab();
		{
			SRWLOCKER(RwL, SReadWriteLocker::Read);
			WHObjEntry * p_entry;
			for(uint i = 0; WhObjList.enumItems(&i, (void **)&p_entry);) {
				if(p_entry->ObjID == arID && (ignoreRights || !(p_entry->Flags & LOCF_INTERNAL_DISABLED))) {
					if(p_entry->LocID == 0) {
						PPSetAddedMsgObjName(PPOBJ_ARTICLE, arID);
						PPSetError(PPERR_INVAR2LOCASSOC);
					}
					id = p_entry->LocID;
					break;
				}
			}
			if(id == 0)
				PPSetError(PPERR_LOCNFOUND);
		}
	}
	return id;
}

PPID FASTCALL LocationCache::WarehouseToObj(PPID locID, int ignoreRights)
{
	PPID   id = 0;
	if(locID) {
		LoadWarehouseTab();
		{
			WHObjEntry * p_entry;
			SRWLOCKER(RwL, SReadWriteLocker::Read);
			for(uint i = 0; WhObjList.enumItems(&i, (void **)&p_entry);) {
				if(p_entry->LocID == locID && (ignoreRights || !(p_entry->Flags & LOCF_INTERNAL_DISABLED))) {
					if(p_entry->ObjID == 0) {
						PPSetAddedMsgObjName(PPOBJ_LOCATION, locID);
						PPErrCode = PPERR_INVLOC2ARASSOC;
					}
					id = p_entry->ObjID;
					break;
				}
			}
			if(id == 0)
				PPSetError(PPERR_LOCNFOUND);
		}
	}
	return id;
}

int LocationCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	LocationData * p_cache_rec = static_cast<LocationData *>(pEntry);
	PPObjLocation loc_obj(SConstructorLite);
	LocationTbl::Rec rec;
	if(loc_obj.Search(id, &rec) > 0) {
		p_cache_rec->Access   = BIN(rec.Type != LOCTYP_WAREHOUSE || ObjRts.CheckLocID(rec.ID, 0));
		p_cache_rec->ParentID = rec.ParentID;
	   	p_cache_rec->Type     = (int16)rec.Type;
		p_cache_rec->OwnerID  = rec.OwnerID;
		p_cache_rec->CityID   = rec.CityID;
		p_cache_rec->RspnsPersonID = rec.RspnsPersonID;
		p_cache_rec->Flags   = (int16)rec.Flags;
		p_cache_rec->NumRows   = rec.NumRows;
		p_cache_rec->NumLayers = rec.NumLayers;
		p_cache_rec->Depth     = rec.Depth;
		p_cache_rec->Latitude  = rec.Latitude;  // @v11.6.1
		p_cache_rec->Longitude = rec.Longitude; // @v11.6.1
		p_cache_rec->Counter   = rec.Counter;
		if(rec.Type == LOCTYP_WAREHOUSE) {
			ArticleCore & r_arc = BillObj->atobj->P_Tbl->Art;
			if(r_arc.SearchObjRef(LConfig.LocAccSheetID, id) > 0)
				p_cache_rec->ArID = r_arc.data.ID;
		}
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Code);
		b.Add(rec.Tail);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void LocationCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	LocationTbl::Rec * p_data_rec = static_cast<LocationTbl::Rec *>(pDataRec);
	const LocationData * p_cache_rec = static_cast<const LocationData *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->ID       = p_cache_rec->ID;
	p_data_rec->ParentID = p_cache_rec->ParentID;
	p_data_rec->Type     = p_cache_rec->Type;
	p_data_rec->OwnerID  = p_cache_rec->OwnerID;
	p_data_rec->CityID   = p_cache_rec->CityID;
	p_data_rec->RspnsPersonID = p_cache_rec->RspnsPersonID;
	p_data_rec->Flags    = p_cache_rec->Flags;
	p_data_rec->NumRows   = p_cache_rec->NumRows;
	p_data_rec->NumLayers = p_cache_rec->NumLayers;
	p_data_rec->Depth     = p_cache_rec->Depth;
	p_data_rec->Latitude  = p_cache_rec->Latitude;  // @v11.6.1
	p_data_rec->Longitude = p_cache_rec->Longitude; // @v11.6.1
	p_data_rec->Counter   = p_cache_rec->Counter;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Code, sizeof(p_data_rec->Code));
	b.Get(p_data_rec->Tail, sizeof(p_data_rec->Tail));
}

IMPL_OBJ_FETCH(PPObjLocation, LocationTbl::Rec, LocationCache);
IMPL_OBJ_DIRTY(PPObjLocation, LocationCache);

const StrAssocArray * PPObjLocation::GetFullEaList()
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->GetFullEaList() : 0;
}

void PPObjLocation::ReleaseFullEaList(const StrAssocArray * pList)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	CALLPTRMEMB(p_cache, ReleaseFullEaList(pList));
}

int PPObjLocation::Helper_GetEaListBySubstring(const char * pSubstr, void * pList, long flags)
{
	int    ok = 1, r = 0;
	const  size_t substr_len = sstrlen(pSubstr);
	PPIDArray * p_list = 0;
	StrAssocArray * p_str_list = 0;
	if(flags & clsfStrList)
		p_str_list = static_cast<StrAssocArray *>(pList);
	else
		p_list = static_cast<PPIDArray *>(pList);
	if(substr_len) {
		const StrAssocArray * p_full_list = GetFullEaList();
		if(p_full_list) {
			const uint c = p_full_list->getCount();
			for(uint i = 0; ok && i < c; i++) {
				StrAssocArray::Item item = p_full_list->at_WithoutParent(i);
				if(flags & clsfFromBeg)
					r = BIN(strncmp(item.Txt, pSubstr, substr_len) == 0);
				else
					r = ExtStrSrch(item.Txt, pSubstr, 0);
				if(r > 0) {
					if(p_list) {
						if(!p_list->addUnique(item.Id)) {
							//
							// ����� THROW �� ������� ��-�� ����, ��� ����� ����� ���������� �����
							// ���������� ������ ������� ReleaseFullList
							//
							ok = PPSetErrorSLib();
						}
					}
					else if(p_str_list) {
						p_str_list->Add(item.Id, item.Txt);
					}
				}
			}
			ReleaseFullEaList(p_full_list);
			p_full_list = 0;
		}
	}
	//CATCHZOK
	return ok;
}

int PPObjLocation::GetEaListBySubstring(const char * pSubstr, StrAssocArray * pList, int fromBegStr)
{
	long   flags = clsfStrList;
	if(fromBegStr)
		flags |= clsfFromBeg;
	int    ok = Helper_GetEaListBySubstring(pSubstr, pList, flags);
	CALLPTRMEMB(pList, SortByText());
	return ok;
}

int PPObjLocation::ResolveWhCell(PPID locID, PPIDArray & rDestList, PPIDArray * pRecurTrace, int useCache)
{
	int    ok = 1, done = 0;
	if(useCache) {
		LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION, 1);
		if(p_cache) {
			ok = p_cache->GetCellList(locID, &rDestList);
			done = 1;
		}
	}
	if(!done) {
		LocationTbl::Rec loc_rec;
		PPIDArray local_recur_trace;
		PPIDArray col_list;
		SETIFZ(pRecurTrace, &local_recur_trace);
		if(Fetch(locID, &loc_rec) > 0) {
			if(pRecurTrace->addUnique(locID) < 0) {
				CALLEXCEPT_PP_S(PPERR_LOCATIONRECUR, loc_rec.Name);
			}
			if(loc_rec.Type == LOCTYP_WHCELL) {
				THROW_SL(rDestList.add(locID));
			}
			else if(loc_rec.Type == LOCTYP_WHCOLUMN) {
				for(SEnum en = P_Tbl->Enum(LOCTYP_WHCELL, locID, 0); en.Next(&loc_rec) > 0;)
					rDestList.add(loc_rec.ID);
			}
			else if(loc_rec.Type == LOCTYP_WHZONE) {
				for(SEnum en = P_Tbl->Enum(LOCTYP_WHCOLUMN, locID, 0); en.Next(&loc_rec) > 0;)
					col_list.add(loc_rec.ID);
			}
			else if(loc_rec.Type == LOCTYP_WAREHOUSE) {
				PPIDArray zone_list;
				{
					for(SEnum en = P_Tbl->Enum(LOCTYP_WHZONE, locID, 0); en.Next(&loc_rec) > 0;)
						zone_list.add(loc_rec.ID);
				}
				{
					for(uint i = 0; i < zone_list.getCount(); i++) {
						for(SEnum en = P_Tbl->Enum(LOCTYP_WHCOLUMN, zone_list.get(i), 0); en.Next(&loc_rec) > 0;)
							col_list.add(loc_rec.ID);
					}
				}
			}
			else if(loc_rec.Type == LOCTYP_WAREHOUSEGROUP) {
				PPIDArray par_list;
				{
					for(SEnum en = P_Tbl->Enum(LOCTYP_WAREHOUSE, locID, 0); en.Next(&loc_rec) > 0;)
						par_list.addUnique(loc_rec.ID);
				}
				{
					for(SEnum en = P_Tbl->Enum(LOCTYP_WAREHOUSEGROUP, locID, 0); en.Next(&loc_rec) > 0;)
						par_list.addUnique(loc_rec.ID);
				}
				{
					for(uint i = 0; i < par_list.getCount(); i++) {
						THROW(ResolveWhCell(par_list.get(i), rDestList, pRecurTrace, 0)); // @recursion
					}
				}
			}
			if(col_list.getCount()) {
				col_list.sort();
				for(SEnum en = P_Tbl->Enum(LOCTYP_WHCELL, 0, LocationCore::eoIgnoreParent); en.Next(&loc_rec) > 0;) {
					if(loc_rec.ParentID && col_list.bsearch(loc_rec.ParentID)) {
						THROW_SL(rDestList.add(loc_rec.ID));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPObjLocation::FetchConfig(PPLocationConfig * pCfg)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION, 1);
	if(p_cache) {
		return p_cache->GetConfig(pCfg, 0);
	}
	else {
		CALLPTRMEMB(pCfg, Z());
		return 0;
	}
}

/*static*/int PPObjLocation::DirtyConfig()
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION, 0);
	return p_cache ? p_cache->GetConfig(0, 1) : 0;
}

PPID PPObjLocation::GetSingleWarehouse()
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (Obj);
	return p_cache ? p_cache->GetSingleWarehouse() : 0;
}

uint PPObjLocation::GetWarehouseList(PPIDArray * pList, bool * pHasRestrictions)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (Obj);
	return p_cache ? p_cache->GetWarehouseList(pList, pHasRestrictions) : 0;
}

/*static*/int FASTCALL PPObjLocation::CheckWarehouseFlags(PPID locID, long f)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->CheckWarehouseFlags(locID, f) : 0;
}

/*static*/PPID FASTCALL PPObjLocation::Implement_ObjToWarehouse_Direct(PPID arID, int ignoreRights, PPID * pAcsID)
{
	if(arID) { 
		PPID   acs_id = 0;
		PPID   lnk_obj_id = 0;
		const  PPID loc_acs_id = LConfig.LocAccSheetID;
		if(GetArticleSheetID(arID, &acs_id, &lnk_obj_id) > 0 && acs_id == loc_acs_id) {
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_LOCATION) {
				ASSIGN_PTR(pAcsID, acs_id);
				const int is_enabled = ignoreRights ? 1 : ObjRts.CheckLocID(lnk_obj_id, 0);
				return is_enabled ? lnk_obj_id : 0;
			}
		}
	}
	ASSIGN_PTR(pAcsID, 0);
	return 0;
}

/*static*/PPID FASTCALL PPObjLocation::ObjToWarehouse_Direct(PPID arID, PPID * pAcsID) { return Implement_ObjToWarehouse_Direct(arID, 0, pAcsID); }
/*static*/PPID FASTCALL PPObjLocation::ObjToWarehouse_IgnoreRights_Direct(PPID arID, PPID * pAcsID) { return Implement_ObjToWarehouse_Direct(arID, 1, pAcsID); }

/*static*/PPID FASTCALL PPObjLocation::WarehouseToObj_Direct(PPID locID)
{
	PPObjArticle ar_obj;
	const  PPID loc_acs_id = LConfig.LocAccSheetID;
	PPID   ar_id = 0;
	return (ar_obj.P_Tbl->LocationToArticle(locID, loc_acs_id, &ar_id) > 0) ? ar_id : 0;
}

/*static*/PPID FASTCALL PPObjLocation::ObjToWarehouse(PPID arID)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	PPID   result = p_cache ? p_cache->ObjToWarehouse(arID, 0) : 0;
	PPID   verif_result = 0;
	if(CConfig.Flags2 & CCFLG2_VERIFYARTOLOCMETHS) {
		PPID   acs_id = 0;
		verif_result = ObjToWarehouse_Direct(arID, &acs_id);
		if(verif_result != result) {
			SString msg_buf;
			msg_buf.Cat("Result of the function").Space().Cat("ObjToWarehouse(PPID)").Space().Cat("isn't relevant");
			msg_buf.Space().CatChar('[').Cat(arID).Cat("->").Cat(result).Slash().Cat(verif_result).CatChar(']');
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
	}
	return result;
}

/*static*/PPID FASTCALL PPObjLocation::ObjToWarehouse_IgnoreRights(PPID arID)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	PPID   result = p_cache ? p_cache->ObjToWarehouse(arID, 1) : 0;
	PPID   verif_result = 0;
	if(CConfig.Flags2 & CCFLG2_VERIFYARTOLOCMETHS) {
		PPID   acs_id = 0;
		verif_result = ObjToWarehouse_IgnoreRights_Direct(arID, &acs_id);
		if(verif_result != result) {
			SString msg_buf;
			msg_buf.Cat("Result of the function").Space().Cat("ObjToWarehouse_IgnoreRights(PPID)").Space().Cat("isn't relevant");
			msg_buf.Space().CatChar('[').Cat(arID).Cat("->").Cat(result).Slash().Cat(verif_result).CatChar(']');
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
	}
	return result;
}

/*static*/PPID FASTCALL PPObjLocation::WarehouseToObj(PPID locID)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	PPID   result = p_cache ? p_cache->WarehouseToObj(locID, 0) : 0;
	PPID   verif_result = 0;
	if(CConfig.Flags2 & CCFLG2_VERIFYARTOLOCMETHS) {
		verif_result = WarehouseToObj_Direct(locID);
		if(verif_result != result) {
			SString msg_buf;
			msg_buf.Cat("Result of the function").Space().Cat("WarehouseToObj(PPID)").Space().Cat("isn't relevant");
			msg_buf.Space().CatChar('[').Cat(locID).Cat("->").Cat(result).Slash().Cat(verif_result).CatChar(']');
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
	}
	return result;
}
//
// Implementation of PPALDD_Location
//
PPALDD_CONSTRUCTOR(Location)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjLocation;
	}
}

PPALDD_DESTRUCTOR(Location)
{
	Destroy();
	delete static_cast<PPObjLocation *>(Extra[0].Ptr);
	Extra[0].Ptr = 0;
}

int PPALDD_Location::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjLocation * p_locobj = static_cast<PPObjLocation *>(Extra[0].Ptr);
		LocationTbl::Rec rec;
		if(p_locobj->Search(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			H.ID = rec.ID;
			H.ParentID = rec.ParentID;
			H.OwnerID  = rec.OwnerID;
			H.Type     = rec.Type;
			H.CityID = rec.CityID;
			STRNSCPY(H.Code, rec.Code);
			LocationCore::GetExField(&rec, LOCEXSTR_ZIP, temp_buf);
			temp_buf.CopyTo(H.ZIP, sizeof(H.ZIP));
			LocationCore::GetExField(&rec, LOCEXSTR_SHORTADDR, temp_buf.Z());
			temp_buf.CopyTo(H.Text, sizeof(H.Text));
			LocationCore::GetExField(&rec, LOCEXSTR_PHONE, temp_buf);
			temp_buf.CopyTo(H.Phone, sizeof(H.Phone));
			LocationCore::GetExField(&rec, LOCEXSTR_CONTACT, temp_buf);
			temp_buf.CopyTo(H.Contact, sizeof(H.Contact));
			STRNSCPY(H.Name, rec.Name);
			LocationCore::GetAddress(rec, 0, temp_buf);
			temp_buf.CopyTo(H.Addr, sizeof(H.Addr));
			// @Muxa {
			H.Latitude = rec.Latitude;
			H.Longitude = rec.Longitude;
			// } @Muxa
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_Location::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_DATE(n) (*static_cast<const LDATE *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?GetAddedString") {
		_RET_STR.Z();
		PPObjLocation * p_obj = static_cast<PPObjLocation *>(Extra[0].Ptr);
		LocationTbl::Rec loc_rec;
		if(p_obj->Search(H.ID, &loc_rec) > 0) {
			LocationCore::GetExField(&loc_rec, _ARG_INT(1), _RET_STR);
		}
	}
	else if(pF->Name == "?GetLongAddr") {
		_RET_STR.Z();
		PPObjLocation * p_obj = static_cast<PPObjLocation *>(Extra[0].Ptr);
		LocationTbl::Rec loc_rec;
		if(p_obj->Search(H.ID, &loc_rec) > 0) {
			LocationCore::GetAddress(loc_rec, 0, _RET_STR);
		}
	}
	else if(pF->Name == "?GetRegister") {
		_RET_INT = 0;
		PPID   reg_type_id = 0;
		if(PPObjRegisterType::GetByCode(_ARG_STR(1), &reg_type_id) > 0) {
			const bool inherit = LOGIC(_ARG_INT(2));
			PPObjLocation * p_obj = static_cast<PPObjLocation *>(Extra[0].Ptr);
			RegisterTbl::Rec reg_rec;
			if(p_obj && p_obj->GetRegister(H.ID, reg_type_id, ZERODATE, inherit, &reg_rec) > 0)
				_RET_INT = reg_rec.ID;
		}
	}
	else if(pF->Name == "?GetRegisterD") {
		_RET_INT = 0;
		PPID   reg_type_id = 0;
		if(PPObjRegisterType::GetByCode(_ARG_STR(1), &reg_type_id) > 0) {
			const bool inherit = LOGIC(_ARG_INT(3));
			PPObjLocation * p_obj = static_cast<PPObjLocation *>(Extra[0].Ptr);
			RegisterTbl::Rec reg_rec;
			if(p_obj && p_obj->GetRegister(H.ID, reg_type_id, _ARG_DATE(2), inherit, &reg_rec) > 0)
				_RET_INT = reg_rec.ID;
		}
	}
	else if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_LOCATION, H.ID, _ARG_STR(1));
	}
}
//
// Implementation of PPALDD_Warehouse
//
PPALDD_CONSTRUCTOR(Warehouse)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjLocation;
	}
}

PPALDD_DESTRUCTOR(Warehouse)
{
	Destroy();
	delete static_cast<PPObjLocation *>(Extra[0].Ptr);
	Extra[0].Ptr = 0;
}

int PPALDD_Warehouse::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjLocation * locobj = static_cast<PPObjLocation *>(Extra[0].Ptr);
		LocationTbl::Rec rec;
		if(locobj->Search(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			H.ID = rec.ID;
			H.LocID = rec.ID;
			H.ParentID = rec.ParentID;
			H.RspnsPersonID = rec.RspnsPersonID;
			H.CityID = rec.CityID;
			H.Flags  = rec.Flags;
			H.fVatFree = BIN(rec.Flags & LOCF_VATFREE);
			H.fComparable = BIN(rec.Flags & LOCF_COMPARABLE);
			STRNSCPY(H.Code, rec.Code);
			STRNSCPY(H.Name, rec.Name);
			LocationCore::GetExField(&rec, LOCEXSTR_ZIP, temp_buf);
			temp_buf.CopyTo(H.ZIP, sizeof(H.ZIP));
			LocationCore::GetAddress(rec, 0, temp_buf);
			temp_buf.CopyTo(H.Addr, sizeof(H.Addr));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_WhCell
//
PPALDD_CONSTRUCTOR(WhCell)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjLocation;
	}
}

PPALDD_DESTRUCTOR(WhCell)
{
	Destroy();
	delete static_cast<PPObjLocation *>(Extra[0].Ptr);
	Extra[0].Ptr = 0;
}

int PPALDD_WhCell::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjLocation * locobj = static_cast<PPObjLocation *>(Extra[0].Ptr);
		LocationTbl::Rec rec;
		if(locobj->Search(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			H.ID = rec.ID;
			H.ParentID = rec.ParentID;
			H.Flags  = rec.Flags;
			H.Layer  = rec.NumLayers;
			H.MassCapacity = rec.MassCapacity;
			H.X = rec.X;
			H.Y = rec.Y;
			H.Z = rec.Z;
			H.DisposeRestr = rec.DisposeRestr;
			H.DisposeRestrCount = rec.DisposeRestrCount;
			STRNSCPY(H.Code, rec.Code);
			STRNSCPY(H.Name, rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// PPLocAddrStruc
//
struct AddrItemDescr {
	enum {
		fOptDot    = 0x0001,
		fSfx       = 0x0002,
		fOptSfx    = 0x0004,
		fNextDigit = 0x0008
	};
	int16  Id;
	const  char * P_Descr;
	int16  Flags;
	int16  WorldObj;
	int16  MainId;
};

struct FiasAbbrEntry {
	uint16     Level;
	const char * P_Text;
	const char * P_Abbr;
	uint16     Key;
	int16      MainId;
};

static const FiasAbbrEntry _FiasAbbrList[] = {
	{  1, "���������� �������", "����", 102, 0 },
	{  1, "����", "����", 104, 0 },
	{  1, "�������", "���", 105, 0 },
	{  1, "�����", "�����", 107, 0 },
	{  1, "����������", "����", 106, 0 },
	{  1, "���������� �����", "��", 101, 0 },
	{  2, "���������� �����", "��", 201, 0 },
	{  3, "���������� �����", "��", 305, 0 },
	{  3, "���������", "�", 306, 0 },
	{  3, "����", "�", 302, 0 },
	{  4, "�������", "�������", 410, 0 },
	{  4, "������ �������", "��", 405, 0 },
	{  4, "��������� �������", "��", 404, 0 },
	{  4, "������", "������", 416, 0 },
	{  4, "�������� �������������", "�/�", 407, 0 },
	{  4, "�������� �������.�����������", "�/��", 415, 0 },
	{  4, "�������� �����", "�/�", 409, 0 },
	{  4, "�������� ���������", "�/�", 414, 0 },
	{  4, "���������", "�/�", 406, 0 },
	{  3, "�����", "�-�", 301, 43 },
	{  5, "�����", "�-�", 503, 43 },
	{  6, "���", "���", 601, 0 },
	{  6, "����������", "����������", 645, 0 },
	{  6, "�������", "�������", 603, 0 },
	{  6, "�������(��)", "�����", 604, 0 },
	{  1, "�����", "�", 103, 1 },
	{  4, "�����", "�", 401, 1 },
	{  6, "�����", "�", 605, 1 },
	{  6, "������ �������", "��", 607, 0 },
	{  6, "����� ����", "�������", 647, 0 },
	{  6, "����� �����", "��������", 646, 0 },
	{  6, "������", "������", 614, 0 },
	{  6, "������", "������", 644, 0 },
	{  6, "��������� �������", "��", 616, 0 },
	{  6, "��������", "�", 617, 0 },
	{  6, "������", "������", 648, 0 },
	{  6, "������������� �����", "�/�", 622, 0 },
	{  4, "������� ���������� ����", "���", 402, 3 },
	{  6, "������� ���������� ����", "���", 624, 3 },
	{  6, "������", "������", 643, 0 },
	{  4, "������� �������", "��", 403, 0 },
	{  6, "������� �������", "��", 629, 0 },
	{  6, "�������", "��-��", 633, 0 },
	{  6, "����", "�", 634, 0 },
	{  7, "���", "���", 732, 0 },
	{  7, "�����", "�����", 781, 0 },
	{  7, "�����", "�����", 776, 0 },
	{  7, "�����", "�����", 782, 0 },
	{  7, "���", "���", 768, 0 },
	{  7, "�����", "�����", 703, 0 },
	{  7, "�������(��)", "�����", 734, 0 },
	{  7, "�����", "�����", 786, 0 },
	{  7, "�������-������������ ����������", "���", 763, 0 },
	{  7, "������ ���", "���", 787, 0 },
	{  7, "�����", "�����", 706, 0 },
	{  7, "����", "����", 777, 0 },
	{  7, "��������", "�", 744, 0 },
	{  7, "����", "����", 785, 0 },
	{  7, "���������", "���������", 780, 0 },
	{  7, "����", "����", 770, 0 },
	{  7, "���", "���", 784, 0 },
	{  7, "����������", "�/�", 788, 0 },
	{  7, "������������� �����", "�/�", 750, 0 },
	{  7, "�������", "�������", 715, 0 },
	{  7, "��������", "��-��", 717, 0 },
	{  7, "����������", "����������", 752, 0 },
	{  7, "������", "������", 783, 0 },
	{  7, "������", "������", 720, 0 },
	{  7, "�������", "�������", 774, 0 },
	{  7, "��������", "��������", 721, 0 },
	{  7, "������", "������", 766, 0 },
	{  7, "�������", "�������", 775, 0 },
	{  7, "�������", "�������", 722, 0 },
	{  7, "����", "����", 771, 0 },
	{  7, "���", "���", 723, 0 },
	{  7, "�����", "�����", 724, 0 },
	{  7, "�������", "�������", 779, 0 },
	{  7, "�����", "�����", 727, 0 },
	{  7, "�����", "���", 728, 0 },
	{  7, "�������", "��-�", 730, 0 },
	{  7, "����������", "�/�", 789, 0 },
	{  7, "��������", "��������", 778, 0 },
	{  8, "���", "���", 801, 0 },
	{ 90, "�������-������������ ����������", "���", 9004, 0 },
	{ 90, "������ �������������� �����������", "���", 9010, 0 },
	{ 90, "���������", "���������", 9011, 0 },
	{ 90, "�������������� �����������", "�/�", 9009, 0 },
	{  6, "������������ ����", "��������", 627, 0 },
	{ 90, "������������ ����", "��������", 9003, 0 },
	{ 90, "���", "���", 9007, 0 },
	{ 90, "������� ������������", "���", 9002, 0 },
	{  3, "����������", "���",  303, 0 },
	{  4, "����������", "���",  412, 0 },
	{  5, "����������", "���",  502, 0 },
	{  6, "����������", "���",  637, 0 },
	{  7, "����������", "���",  726, 0 },
	{ 90, "����������", "���", 9005, 0 },
	{ 90, "���������� ���������", "�/�", 9008, 0 },
	{  7, "����������� ����", "�/�",  772, 49 },
	{ 91, "����������� ����", "�/�", 9172, 49 },
	{ 91, "���", "���", 9132, 0 },
	{  7, "�����", "�����",  701, 53 },
	{ 91, "�����", "�����", 9101, 53 },
	{  6, "�����", "�����", 640, 0 },
	{  7, "�����", "�����", 760, 0 },
	{ 91, "�����", "�����", 9160, 0 },
	{  6, "���", "���",  602, 0 },
	{  7, "���", "���",  733, 0 },
	{ 91, "���", "���", 9133, 0 },
	{  7, "�����", "�����",  773, 0 },
	{ 91, "�����", "�����", 9173, 0 },
	{  7, "�������", "�-�",  702, 55 },
	{ 91, "�������", "�-�", 9102, 55 },
	{ 91, "�����", "�����", 9176, 0 },
	{ 91, "���", "���", 9168, 0 },
	{ 91, "�����", "�����", 9103, 0 },
	{ 91, "�������(��)", "�����", 9134, 0 },
	{  6, "�������", "�������",  636, 0 },
	{  7, "�������", "�������",  735, 0 },
	{ 91, "�������", "�������", 9135, 0 },
	{ 91, "�������-������������ ����������", "���", 9163, 0 },
	{  6, "�������", "�",  606, 6 },
	{  7, "�������", "�",  736, 6 },
	{ 91, "�������", "�", 9136, 6 },
	{  7, "������", "���",  704, 0 },
	{ 91, "������", "���", 9104, 0 },
	{  6, "��������������� �����", "�/�_�����", 608, 0 },
	{  6, "��������������� �������", "�/�_������", 609, 0 },
	{  6, "��������������� ��������� (��������) �����", "�/�_��", 610, 0 },
	{  6, "��������������� ���������", "�/�_�����", 638, 0 },
	{  6, "��������������� ����", "�/�_����", 611, 0 },
	{  6, "��������������� �������", "�/�_���", 612, 0 },
	{  6, "��������������� �������", "�/�_��", 613, 0 },
	{  7, "��������������� �����", "�/�_�����", 737, 0 },
	{  7, "��������������� �������", "�/�_������", 738, 0 },
	{  7, "��������������� ��������� (��������) �����", "�/�_��", 739, 0 },
	{  7, "��������������� ���������", "�/�_�����", 759, 0 },
	{  7, "��������������� ����", "�/�_����", 740, 0 },
	{  7, "��������������� �������", "�/�_���", 741, 0 },
	{  7, "��������������� �������", "�/�_��", 742, 0 },
	{  7, "���������������� �����", "��", 705, 0 },
	{ 91, "��������������� �����", "�/�_�����", 9137, 0 },
	{ 91, "��������������� �������", "�/�_������", 9138, 0 },
	{ 91, "��������������� ��������� (��������) �����", "�/�_��", 9139, 0 },
	{ 91, "��������������� ���������", "�/�_�����", 9159, 0 },
	{ 91, "��������������� ����", "�/�_����", 9140, 0 },
	{ 91, "��������������� �������", "�/�_���", 9141, 0 },
	{ 91, "��������������� �������", "�/�_��", 9142, 0 },
	{ 91, "���������������� �����", "��", 9105, 0 },
	{ 91, "�����", "�����", 9106, 0 },
	{ 91, "����", "����", 9177, 0 },
	{  6, "�������", "�������", 615, 0 },
	{  7, "�������", "�������", 743, 0 },
	{ 91, "�������", "�������", 9143, 0 },
	{  7, "�����", "�����", 762, 0 },
	{ 91, "�����", "�����", 9162, 0 },
	{  6, "�������", "��-�", 639, 0 },
	{  7, "�������", "��-�", 707, 0 },
	{ 91, "�������", "��-�", 9107, 0 },
	{  7, "��������", "��", 708, 0 },
	{ 91, "��������", "��", 9108, 0 },
	{  7, "������", "������", 709, 0 },
	{ 91, "������", "������", 9109, 0 },
	{  7, "����", "����", 767, 0 },
	{ 91, "����", "����", 9167, 0 },
	{  7, "�����", "�����", 710, 0 },
	{ 91, "�����", "�����", 9110, 0 },
	{  6, "����������", "���", 642, 0 },
	{  7, "����������", "���", 765, 0 },
	{ 91, "����������", "���", 9165, 0 },
	{ 91, "��������", "�", 9144, 0 },
	{  6, "����������", "���", 618, 0 },
	{  7, "����������", "���", 745, 0 },
	{ 90, "����������", "���", 9006, 0 },
	{ 91, "����������", "���", 9145, 0 },
	{ 91, "����", "����", 9170, 0 },
	{  7, "����������", "���", 711 , 51 },
	{ 91, "����������", "���", 9111, 51 },
	{  6, "���������� �����", "��", 619, 0 },
	{  7, "���������� �����", "��", 746, 0 },
	{ 91, "���������� �����", "��", 9146, 0 },
	{  6, "������", "������",  620, 0 },
	{  7, "������", "������",  712, 0 },
	{ 91, "������", "������", 9112, 0 },
	{  4, "�������", "�",  417, 3 },
	{  6, "�������", "�",  621, 3 },
	{  7, "�������", "�",  748, 3 },
	{ 91, "�������", "�", 9148, 3 },
	{  4, "�������� ���������", "�/�", 411, 0 },
	{  6, "�������� ���������", "�/�", 626, 0 },
	{  7, "�������� ���������", "�/�", 749, 0 },
	{ 91, "�������� ���������", "�/�", 9149, 0 },
	{ 91, "������������� �����", "�/�", 9150, 0 },
	{  6, "������� �(���) �������(�)", "�/��", 623, 0 },
	{  7, "������� �(���) �������(�)", "�/��", 751, 0 },
	{ 91, "������� �(���) �������(�)", "�/��", 9151, 0 },
	{  7, "����", "����", 713, 0 },
	{ 91, "����", "����", 9113, 0 },
	{  7, "��������", "���", 714,  24 },
	{ 91, "��������", "���", 9114, 24 },
	{ 91, "�������", "�������", 9115, 0 },
	{  7, "�������", "��",  716, 22 },
	{ 91, "�������", "��", 9116, 22 },
	{  7, "���������", "�����", 747, 0 },
	{ 91, "���������", "�����", 9147, 0 },
	{ 91, "��������", "��-��", 9117, 0 },
	{ 91, "����������", "����������", 9152, 0 },
	{  6, "�������", "�������", 625, 0 },
	{  7, "�������", "�������", 753, 0 },
	{ 91, "�������", "�������", 9153, 0 },
	{  7, "��������", "��-��", 719, 18 },
	{ 91, "��������", "��-��", 9119, 18 },
	{  7, "������", "������",  718, 0 },
	{ 91, "������", "������", 9118, 0 },
	{ 91, "������", "������", 9120, 0 },
	{ 91, "�������", "�������", 9174, 0 },
	{ 91, "��������", "��������", 9121, 0 },
	{ 91, "������", "������", 9166, 0 },
	{ 91, "�������", "�������", 9175, 0 },
	{ 91, "�������", "�������", 9122, 0 },
	{  6, "�������", "���", 628, 0 },
	{  7, "�������", "���", 754, 0 },
	{ 91, "�������", "���", 9154, 0 },
	{ 91, "����", "����", 9171, 0 },
	{  6, "����", "�",  630, 9 },
	{  7, "����", "�",  755, 9 },
	{ 91, "����", "�", 9155, 9 },
	{ 91, "���", "���", 9123, 0 },
	{ 91, "�����", "�����", 9124, 0 },
	{  6, "�������", "��", 631, 0 },
	{  7, "�������", "��", 756, 0 },
	{ 91, "�������", "��", 9156, 0 },
	{  6, "������� �������������� ������������", "���", 641, 0 },
	{  7, "������� �������������� ������������", "���", 764, 0 },
	{ 91, "������� �������������� ������������", "���", 9164, 0 },
	{  7, "�����", "�����", 761, 0 },
	{ 91, "�����", "�����", 9161, 0 },
	{  6, "�������", "��",  632, 13 },
	{  7, "�������", "��",  757, 13 },
	{ 91, "�������", "��", 9157, 13 },
	{  7, "��������", "���",  725, 59 },
	{ 91, "��������", "���", 9125, 59 },
	{ 91, "����������", "���", 9126, 0 },
	{ 91, "�����", "�����", 9127, 0 },
	{ 91, "�����", "���", 9128, 0 },
	{  7, "�����", "��",  729, 15 },
	{ 91, "�����", "��", 9129, 15 },
	{ 91, "�������", "��-�", 9130, 0 },
	{  7, "�����", "�����", 769, 0 },
	{ 91, "�����", "�����", 9169, 0 },
	{  6, "�����", "�",  635, 11 },
	{  7, "�����", "�",  758, 11 },
	{ 91, "�����", "�", 9158, 11 },
	{  7, "�����", "�",  731, 29 },
	{ 91, "�����", "�", 9131, 29 }
};

static const AddrItemDescr Aidl[] = {
	{  1, "�����",    0, PPLocAddrStruc::tCity, 0 },
	{  2, "�",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 1 },
	{  3, "�������",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tCity, 0 },
	{  4, "���",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 3 },
	{  5, "�",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 3 },
	{  6, "�������",  0, PPLocAddrStruc::tCity, 0 },
	{  7, "���",      AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 6 },
	{  8, "�",        AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 6 },
	{  9, "����",     0, PPLocAddrStruc::tCity, 0 },
	{ 10, "�",        AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 9 },
	{ 11, "�����",    0, PPLocAddrStruc::tCity, 0 },
	{ 12, "�",        AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 11 },
	{ 13, "�������",  0, PPLocAddrStruc::tCity, 0 },
	{ 14, "��",       AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 13 },
	{ 15, "�����",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 16, "��",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 15 },
	{ 17, "�",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 15 },
	{ 18, "��������", AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 19, "��",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 18 },
	{ 20, "��-�",     AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 18 },
	{ 21, "�",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 18 },
	{ 22, "�������",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 23, "��",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 22 },
	{ 24, "��������", AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 25, "���",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 24 },
	{ 26, "������",   AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 27, "��-�",     AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 26 },
	{ 28, "�-�",      AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 26 },
	{ 29, "�����",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 30, "�",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 29 },
	{ 31, "���",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 29 },
	{ 32, "���",      AddrItemDescr::fNextDigit, PPLocAddrStruc::tHouse, 0 },
	{ 33, "�",        AddrItemDescr::fNextDigit|AddrItemDescr::fOptDot, PPLocAddrStruc::tHouse, 32 },
	{ 34, "������",   0, PPLocAddrStruc::tHouseAddendum, 0 },
	{ 35, "���",      AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 34 },
	{ 36, "����",     AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 34 },
	{ 37, "��������", AddrItemDescr::fNextDigit, PPLocAddrStruc::tApart, 0 },
	{ 38, "��",       AddrItemDescr::fNextDigit|AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 37 },
	{ 39, "����",     0, PPLocAddrStruc::tApart, 0 },
	{ 40, "��",       AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 39 },
	{ 41, "�������",  0, PPLocAddrStruc::tApart, 0 },
	{ 42, "�",        AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 41 },
	{ 43, "�����",    AddrItemDescr::fSfx, PPLocAddrStruc::tLocalArea, 0 },
	{ 44, "�-�",      AddrItemDescr::fSfx, PPLocAddrStruc::tLocalArea, 43 },
	{ 45, "�",        AddrItemDescr::fSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tLocalArea, 43 },
	{ 46, "��������", AddrItemDescr::fNextDigit, PPLocAddrStruc::tHouse, 0 },
	{ 47, "���",      AddrItemDescr::fNextDigit|AddrItemDescr::fOptDot, PPLocAddrStruc::tHouse, 46 },
	{ 48, "����",     AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 41 },
	{ 49, "�/�",      AddrItemDescr::fNextDigit, PPLocAddrStruc::tPostBox,  0 },
	{ 50, "��",       AddrItemDescr::fNextDigit, PPLocAddrStruc::tPostBox, 49 },
	{ 51, "����������", AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 52, "���",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 51 },
	{ 53, "�����",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 54, "��",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 53 },
	{ 55, "�������",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 56, "���",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 55 },
	{ 57, "�������",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tLocalArea, 0 },
	{ 58, "���",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tLocalArea, 57 },
	{ 59, "��������", 0, PPLocAddrStruc::tHouseAddendum, 59 },
	{ 60, "���",      AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 59 },
	{ 61, "������",   0, PPLocAddrStruc::tHouseAddendum, 61 },
	{ 62, "���",      AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 61 },
	{ 63, "���������", 0, PPLocAddrStruc::tApart, 63 },
	{ 64, "���",      AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 63 },
	{ 65, "�-��",     AddrItemDescr::fSfx, PPLocAddrStruc::tLocalArea, 43 },
	{ 66, "��-��",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 18 },
	{ 67, "�����",    AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 61 },
	{ 68, "�-�",      AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 55 },
	{ 69, "�-��",     AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 29 },
	{ 70, "���",      AddrItemDescr::fOptSfx, PPLocAddrStruc::tCity, 3 },
	{ 71, "�",        AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 55 },
	{ 72, "����",     AddrItemDescr::fOptSfx, PPLocAddrStruc::tFloor, 0 }, // @v11.5.9
};

PPLocAddrStruc::AddrTok::AddrTok() : T(0), P(0), Flags(0), P_SplitL(0)
{
}

PPLocAddrStruc::AddrTok::~AddrTok()
{
	delete P_SplitL;
}

PPLocAddrStruc::AddrTok & PPLocAddrStruc::AddrTok::Z()
{
	T = 0;
	P = 0;
	Flags = 0;
	S = 0;
	ZDELETE(P_SplitL);
	Dl.clear();
	return *this;
}

PPLocAddrStruc::DescrSelector::DescrSelector() : T(0), Kt(0)
{
}

void PPLocAddrStruc::DescrSelector::Init()
{
	T = 0;
	Kt = 0;
	Kind = 0;
}

PPLocAddrStruc::DetectBlock & PPLocAddrStruc::DetectBlock::Init(int entityType, uint tokPos, const char * pOrgText, PPID fiasUpperID)
{
	T = entityType;
	P = tokPos;
	OrgText = pOrgText;
	RecognizedText = 0;
	FiasCandidList.clear();
	FiasUpperID = fiasUpperID;
	return *this;
}

PPLocAddrStruc_MatchEntry::PPLocAddrStruc_MatchEntry(uint p1, uint p2, int reverse) : P1(p1), P2(p2), Reverse(reverse)
{
}

PPLocAddrStruc_MatchEntry::PPLocAddrStruc_MatchEntry(const PPLocAddrStruc_MatchEntry & rS) :
	P1(rS.P1), P2(rS.P2), Reverse(rS.Reverse), CityStreetList(rS.CityStreetList)
{
}

void PPLocAddrStruc::Helper_Construct()
{
	State = 0;
	P_Fr = 0;
	Style = 0;
	Flags = 0;
	CountryID = 0;
	CityID = 0;
	StreetID = 0;
	FiasTerminalID = 0;
	FiasCityID = 0;
	FiasStreetID = 0;
	FiasHouseID = 0;
	ReZip = 0;
	ReNum_Hyp_Num = 0;
	ReNum_Sl_Num = 0;
	ReNumA_Hyp_Num = 0;
	ReNumA_Sl_Num = 0;
	P_AmbigMatchEntry = 0;
	P_AmbigMatchList = 0;

	SStrScan scan;
	Scan.RegisterRe("^[0-9][0-9][0-9][0-9][0-9][0-9]", &ReZip);
	Scan.RegisterRe("^[0-9]+[ \t]*\\-[ \t]*[0-9]+", &ReNum_Hyp_Num);
	Scan.RegisterRe("^[0-9]+[ \t]*\\-[ \t]*[0-9]+[ \t]*\\-[ \t]*[0-9]+", &ReNum_Hyp_Num_Hyp_Num);
	Scan.RegisterRe("^[0-9]+[ \t]*\\/[ \t]*[0-9]+", &ReNum_Sl_Num);
	Scan.RegisterRe("^[0-9]+[�-��-�A-Za-z][ \t]*\\-[ \t]*[0-9]+", &ReNumA_Hyp_Num);
	Scan.RegisterRe("^[0-9]+[�-��-�A-Za-z][ \t]*\\/[ \t]*[0-9]+", &ReNumA_Sl_Num);
}

PPLocAddrStruc::PPLocAddrStruc(const char * pText, PPFiasReference * pFr) : StrAssocArray()
{
	Helper_Construct();
	P_Fr = pFr;
	Recognize(pText);
}

PPLocAddrStruc::PPLocAddrStruc(ConditionalConstructWithFias ccwf) : StrAssocArray()
{
	Helper_Construct();
	PPLocationConfig loc_cfg;
	PPObjLocation::FetchConfig(&loc_cfg);
	if(loc_cfg.Flags & PPLocationConfig::fUseFias) {
		P_Fr = new PPFiasReference;
		State |= stOwnFiasRef;
	}
}

PPLocAddrStruc::~PPLocAddrStruc()
{
	ZDELETE(P_AmbigMatchEntry);
	ZDELETE(P_AmbigMatchList);
	if(State & stOwnFiasRef)
		ZDELETE(P_Fr);
}

int PPLocAddrStruc::HasAmbiguity() const { return P_AmbigMatchEntry ? 1 : (P_AmbigMatchList ? 2 : 0); }
const TSCollection <PPLocAddrStruc_MatchEntry> * PPLocAddrStruc::GetAmbiguityMatchList() const { return P_AmbigMatchList; }
const PPLocAddrStruc_MatchEntry * PPLocAddrStruc::GetAmbiguityMatchEntry() const { return P_AmbigMatchEntry; }

int PPLocAddrStruc::MatchEntryToStr(const PPLocAddrStruc_MatchEntry * pEntry, SString & rBuf)
{
	int    ok = 1;
    SString temp_buf;
    if(pEntry) {
		if(pEntry->P1 < TokList.getCount())
			rBuf.Cat(TokList.at(pEntry->P1)->S);
		else
			rBuf.CatChar('[').Cat(pEntry->P1).CatChar(']');
		rBuf.CatDiv(';', 2);
		if(pEntry->P2 < TokList.getCount())
			rBuf.Cat(TokList.at(pEntry->P2)->S);
		else
			rBuf.CatChar('[').Cat(pEntry->P2).CatChar(']');
		if(pEntry->Reverse)
			rBuf.Space().Cat("(reverse)");
		rBuf.CatDiv(':', 2);
		for(uint i = 0; i < pEntry->CityStreetList.getCount(); i++) {
			const LAssoc & r_assc = pEntry->CityStreetList.at(i);
			if(i)
				rBuf.Space();
            rBuf.CatChar('{');
			FiasAddrObjTbl::Rec rec;
            if(P_Fr && P_Fr->SearchObjByID(r_assc.Key, &rec, 1 /*use_cache*/) > 0 && P_Fr->GetText(rec.NameTRef, temp_buf) > 0)
				rBuf.CatChar('#').Cat(r_assc.Key).Space().Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
            else
				rBuf.CatChar('#').Cat(r_assc.Key);
			rBuf.CatDiv(';', 2);
            if(P_Fr && P_Fr->SearchObjByID(r_assc.Val, &rec, 1 /*use_cache*/) > 0 && P_Fr->GetText(rec.NameTRef, temp_buf) > 0)
				rBuf.CatChar('#').Cat(r_assc.Val).Space().Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
            else
				rBuf.CatChar('#').Cat(r_assc.Val);
            rBuf.CatChar('}');
		}
    }
    return ok;
}

int PPLocAddrStruc::DetectStreetName(DetectBlock & rDb)
{
	int    ok = 0;
	rDb.FiasCandidList.clear();
	SString temp_buf;
	if(P_Fr) {
		PPIDArray fao_list;
        if(P_Fr->SearchObjByText(rDb.OrgText, PPFiasReference::stfAnsiInput, rDb.FiasUpperID, fao_list) > 0) {
			for(uint i = 0; i < fao_list.getCount(); i++) {
                const  PPID fao_id = fao_list.get(i);
                FiasAddrObjTbl::Rec fao_rec;
                if(P_Fr->SearchObjByID(fao_id, &fao_rec, 1 /*use_cache*/) > 0) {
					if(fao_rec.LevelStatus == 7) {
						rDb.FiasCandidList.add(fao_id);
					}
                }
			}
        }
        if(rDb.FiasCandidList.getCount() == 1)
			ok = 2;
        else if(rDb.FiasCandidList.getCount() > 1)
			ok = 3;
	}
	return ok;
}

int PPLocAddrStruc::DetectCityName(DetectBlock & rDb)
{
	int    ok = 0;
	rDb.FiasCandidList.clear();
	SString temp_buf;
	if(P_Fr) {
		PPIDArray fao_list;
        if(P_Fr->SearchObjByText(rDb.OrgText, PPFiasReference::stfAnsiInput, 0, fao_list) > 0) {
			for(uint i = 0; i < fao_list.getCount(); i++) {
                const  PPID fao_id = fao_list.get(i);
                FiasAddrObjTbl::Rec fao_rec;
                if(P_Fr->SearchObjByID(fao_id, &fao_rec, 1 /*use_cache*/) > 0) {
					if(fao_rec.LevelStatus < 7) {
						rDb.FiasCandidList.add(fao_id);
					}
                }
			}
        }
        if(rDb.FiasCandidList.getCount() == 1)
			ok = 2;
        else if(rDb.FiasCandidList.getCount() > 1)
			ok = 3;
	}
	if(!ok) {
		if(BillObj) { // �������� �� ������, ���� ����� �� ����������� � ���� ������
			PPID   city_id = 0;
			(temp_buf = rDb.OrgText).Strip().Transf(CTRANSF_OUTER_TO_INNER);
			PPObjLocation loc_obj;
			int    r = loc_obj.GetCityByName(temp_buf, &city_id);
			if(r > 0) {
				rDb.RecognizedText = rDb.OrgText;
				ok = 1;
			}
		}
		if(!ok) {
			struct CityAbbr {
				const char * P_Abbr;
				const char * P_Name;
			};
			static const CityAbbr city_abbr[] = {
				{ "���", "�����-���������" },
				{ "���������", "�����-���������" },
				{ "���", "������" },
				{ "���", "������������" },
				{ "���", "������������" }
			};
			(temp_buf = rDb.OrgText).Strip();
			for(uint i = 0; !ok && i < SIZEOFARRAY(city_abbr); i++) {
				if(rDb.OrgText.CmpNC(city_abbr[i].P_Abbr) == 0) {
					rDb.RecognizedText = city_abbr[i].P_Name;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPLocAddrStruc::GetTok(AddrTok & rTok)
{
	int    ok = 1;
	SString temp_buf;
	Scan.Skip();
	if(Scan[0] == 0) {
		rTok.T = rTok.tFinish;
		ok = 0;
	}
	else if(Scan.GetRe(ReZip, temp_buf)) {
		rTok.T = rTok.tZip;
		rTok.S = temp_buf;
	}
	else if(Scan.GetRe(ReNum_Hyp_Num_Hyp_Num, temp_buf)) {
		rTok.T = rTok.tNum_Hyp_Num_Hyp_Num;
		rTok.S = temp_buf;
	}
	else if(Scan.GetRe(ReNum_Hyp_Num, temp_buf) || Scan.GetRe(ReNumA_Hyp_Num, temp_buf)) {
		rTok.T = rTok.tNum_Hyp_Num;
		rTok.S = temp_buf;
	}
	else if(Scan.GetRe(ReNum_Sl_Num, temp_buf) || Scan.GetRe(ReNumA_Sl_Num, temp_buf)) {
		rTok.T = rTok.tNum_Sl_Num;
		rTok.S = temp_buf;
	}
	else {
		temp_buf.Z();
		char c = Scan[0];
		const char * p_div = ",;()[]{}";
		if(sstrchr(p_div, c)) {
			temp_buf.CatChar(c);
			rTok.T = rTok.tDiv;
			rTok.S = temp_buf;
			Scan.Incr();
		}
		else if(isdec(c)) {
			do {
				temp_buf.CatChar(c);
				Scan.Incr();
				c = Scan[0];
			} while(isdec(c));
			//
			// �������� ������ ���������� ������������ � ����������� �������.
			// ��� - ������ ����� ���� �������.
			//
			uint i = 0;
			while(Scan[i] == ' ')
				i++;
			if(Scan[i] == '-') {
				i++;
				while(Scan[i] == ' ')
					i++;
			}
			static const char * p_ordinal_sfx[] = { "�", "�", "�", "�", "��", "��", "��", "��", "��", "��", "�", "�" };
			int    ordinal = 0;
			const  size_t preserve_offs = Scan.Offs;
			Scan.Incr(i);
			for(uint j = 0; !ordinal && j < SIZEOFARRAY(p_ordinal_sfx); j++) {
				if(Scan.Is(p_ordinal_sfx[j])) {
					ordinal = j+1;
					i += (uint)sstrlen(p_ordinal_sfx[j]);
				}
			}
			Scan.Offs = preserve_offs;
			if(ordinal) {
				temp_buf.CatChar('-').Cat(p_ordinal_sfx[ordinal-1]).Space();
				while(Scan[i] == ' ')
					i++;
				Scan.Incr(i);
				c = Scan[0];
				while(!oneof8(c, 0, ' ', '\t', ',', ';', '.', '(', ')') && !isdec(c)) {
					temp_buf.CatChar(c);
					Scan.Incr();
					c = Scan[0];
				}
				if(c == '.')
					Scan.Incr();
				rTok.T = rTok.tText;
				rTok.S = temp_buf;
				if(c == '.')
					rTok.Flags |= rTok.fDot;
			}
			else {
				//
				// ������ �����
				//
				rTok.T = rTok.tNumber;
				rTok.S = temp_buf;
			}
		}
		else {
			while(!oneof8(c, 0, ' ', '\t', ',', ';', '.', '(', ')') && !isdec(c)) {
				temp_buf.CatChar(c);
				Scan.Incr();
				c = Scan[0];
			}
			if(c == '.')
				Scan.Incr();
			if(temp_buf.NotEmptyS()) {
				LongArray dl;
				SString term;
				(term = temp_buf).Transf(CTRANSF_OUTER_TO_INNER).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
				for(uint i = 0; i < SIZEOFARRAY(Aidl); i++) {
					if(term.CmpNC(Aidl[i].P_Descr) == 0) {
						dl.add(i);
					}
				}
				if(dl.getCount()) {
					rTok.T = rTok.tDescr;
					rTok.Dl = dl;
					rTok.S = temp_buf;
				}
				else {
					rTok.T = rTok.tText;
					rTok.S = temp_buf;
					if(c == '.')
						rTok.Flags |= rTok.fDot;
				}
			}
		}
	}
	return ok;
}

int PPLocAddrStruc::ProcessDescr(const AddrItemDescr & rDescr, DescrSelector & rSel)
{
	int    ok = 0;
	rSel.Init();
	SString kind_buf;
	if(rDescr.MainId == 0) {
		rSel.Kind = rDescr.P_Descr;
	}
	else {
		for(uint i = 0; i < SIZEOFARRAY(Aidl); i++)
			if(Aidl[i].Id == rDescr.MainId) {
				rSel.Kind = Aidl[i].P_Descr;
				break;
			}
	}
	switch(rDescr.WorldObj) {
		case tCity:
			if(!Search(tCity)) {
				rSel.T = tCity;
				rSel.Kt = tCityKind;
			}
			break;
		case tStreet:
			if(!Search(tStreet)) {
				rSel.T = tStreet;
				rSel.Kt = tStreetKind;
			}
			break;
		case tLocalArea:
			if(!Search(tLocalArea)) {
				rSel.T = tLocalArea;
				rSel.Kt = tLocalAreaKind;
			}
			break;
		case tHouse:
			if(!Search(tHouse)) {
				rSel.T = tHouse;
				rSel.Kt = tHouseKind;
			}
			break;
		case tHouseAddendum:
			if(!Search(tHouseAddendum)) {
				rSel.T = tHouseAddendum;
				rSel.Kt = tHouseAddendumKind;
			}
			break;
		case tApart:
			if(!Search(tApart)) {
				rSel.T = tApart;
				rSel.Kt = tApartKind;
			}
			break;
		case tFloor:
			if(!Search(tFloor)) {
				rSel.T = tFloor;
			}
			break;
		case tPostBox:
			if(!Search(tPostBox)) {
				rSel.T = tPostBox;
			}
			break;
	}
	if(rSel.T)
		ok = 1;
	return ok;
}

int PPLocAddrStruc::GetFiasAddrObjKind(PPID adrObjID, SString & rKind)
{
	rKind.Z();
	int    ok = -1;
	if(P_Fr) {
		FiasAddrObjTbl::Rec rec;
		if(P_Fr->SearchObjByID(adrObjID, &rec, 1 /*use_cache*/) > 0 && rec.SnTRef) {
			SString temp_buf;
			if(P_Fr->GetText(rec.SnTRef, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				for(uint i = 0; i < SIZEOFARRAY(_FiasAbbrList); i++) {
                    const FiasAbbrEntry & r_entry = _FiasAbbrList[i];
					if(temp_buf.CmpNC(r_entry.P_Abbr) == 0) {
						if(r_entry.MainId) {
							for(uint j = 0; j < SIZEOFARRAY(Aidl); j++) {
								if(Aidl[j].Id == r_entry.MainId) {
									rKind = Aidl[j].P_Descr;
									ok = 1;
									break;
								}
							}
						}
						else {
							rKind = r_entry.P_Abbr;
							ok = 2;
						}
						break;
					}
				}
			}
		}
	}
	return ok;
}

int PPLocAddrStruc::Recognize(const char * pText)
{
	const char * p_enforcefias_prefix = "enforcefias";

	int    ok = -1;
	int    do_enforce_fias = 0;
	Style = 0;
	Flags = 0;
	CountryID = 0;
	CityID = 0;
	StreetID = 0;
	FiasTerminalID = 0;
	FiasCityID = 0;
	FiasStreetID = 0;
	FiasHouseID = 0;
	ZDELETE(P_AmbigMatchEntry);
	ZDELETE(P_AmbigMatchList);
	TokList.freeAll();
	Z();
	if(pText) {
		PPIDArray fao_list;
		SString text(pText);
		SString temp_buf, temp_buf2, temp_buf3;
		text.Strip();
		if(text.HasPrefixIAscii(p_enforcefias_prefix)) {
            text.ShiftLeft(sstrlen(p_enforcefias_prefix)).Strip();
            do_enforce_fias = 1;
		}
		if(text.C(0) == '\"') {
			text.ShiftLeft().TrimRightChr('\"');
		}
		text.ReplaceStr("\"\"", "\"", 0);
		Scan.Set(text, 0);
		{
			AddrTok * p_tok = new AddrTok;
			while(GetTok(*p_tok)) {
				TokList.insert(p_tok);
				p_tok = new AddrTok;
			}
			delete p_tok;
		}
		{
			uint c = TokList.getCount(); // not constant!
			for(uint i = 0; i < c; i++) {
				AddrTok * p_tok = TokList.at(i);
				AddrTok * p_next_tok = (i < (c-1)) ? TokList.at(i+1) : 0;
				AddrTok * p_prev_tok = (i > 0) ? TokList.at(i-1) : 0;
				if(p_tok->T == AddrTok::tText) {
					if(p_prev_tok) {
						int   do_merge = 0;
						int   merge_type = AddrTok::tText;
						int   prev_ann_number = 0; // ���� !0 �� ���������� ����� - ������� ����� (����������� � ��������� ����)
						temp_buf = p_prev_tok->S;
						temp_buf2.Z();
						if(p_prev_tok->T == AddrTok::tNumber) {
							const long v = temp_buf.ToLong();
							const ushort an_list[] = { 10, 20, 25, 30, 40, 50, 60, 75, 80, 100, 125, 150, 200 };
							for(uint j = 0; !prev_ann_number && j < SIZEOFARRAY(an_list); j++)
								if(v == (long)an_list[j])
									prev_ann_number = 1;
						}
						if(oneof2(p_prev_tok->T, AddrTok::tText, AddrTok::tDescr) || prev_ann_number) {
							if(!p_next_tok || oneof3(p_next_tok->T, AddrTok::tText, AddrTok::tDiv, AddrTok::tDescr)) {
								if(p_prev_tok->Flags & AddrTok::fDot)
									temp_buf.Dot();
								if(prev_ann_number) {
									(temp_buf2 = p_tok->S).ToLower1251();
									if(temp_buf2 == "���" || temp_buf2 == "�����") {
										temp_buf.Space().Cat(p_tok->S);
										do_merge = 1;
									}
								}
								if(!do_merge) {
									temp_buf.Space().Cat(p_tok->S);
									(temp_buf2 = temp_buf).ToLower1251();
									if(temp_buf2 == "�. ������" || temp_buf2 == "� ������") {
										temp_buf2 = "����� ������";
										do_merge = 2;
									}
									else if(temp_buf2 == "�. ��������" || temp_buf2 == "� ��������" || temp_buf2 == "��. ��������") {
										temp_buf2 = "���������� ��������";
										do_merge = 2;
									}
									else if(temp_buf2 == "�������� ������") { // @v11.3.1
										temp_buf2 = "�������� ������";
										do_merge = 2;
									}
									else if(temp_buf2 == "���� ��������") { // @v11.3.1
										temp_buf2 = "���� ��������";
										do_merge = 2;
									}
									fao_list.clear();
									if(P_Fr && P_Fr->SearchObjByText(temp_buf, PPFiasReference::stfAnsiInput, 0, fao_list) > 0)
										do_merge = 1;
								}
							}
						}
						if(do_merge) {
							if(p_prev_tok->Flags & AddrTok::fDot)
								p_prev_tok->Flags &= ~AddrTok::fDot;
							p_prev_tok->T = merge_type;
							p_prev_tok->S = (do_merge == 2) ? temp_buf2 : temp_buf;
							if(p_tok->Flags & AddrTok::fDot)
								p_prev_tok->Flags |= AddrTok::fDot;
							TokList.atFree(i--);
							c = TokList.getCount();
						}
					}
				}
			}
		}
		{
			const uint c = TokList.getCount();
			DescrSelector ds, prev_ds;
			int    last_t = 0;
			uint   i;
			for(i = 0; i < c; i++) {
				AddrTok * p_tok = TokList.at(i);
				if(!(p_tok->Flags & AddrTok::fUsed)) {
					AddrTok * p_next_tok = (i < (c-1)) ? TokList.at(i+1) : 0;
					AddrTok * p_prev_tok = (i > 0) ? TokList.at(i-1) : 0;
					if(p_prev_tok && p_prev_tok->Flags & AddrTok::fUsed)
						p_prev_tok = 0;
					if(p_next_tok && p_next_tok->Flags & AddrTok::fUsed)
						p_next_tok = 0;
					ds.Init();
					prev_ds.Init();
					switch(p_tok->T) {
						case AddrTok::tDiv:
							break;
						case AddrTok::tText:
							{
								uint   wo_idx = 0;
								if(p_prev_tok) {
									if(p_prev_tok->T == AddrTok::tDescr) {
										if(p_prev_tok->Dl.getCount() == 1) {
											wo_idx = p_prev_tok->Dl.get(0);
											const AddrItemDescr & r_aid = Aidl[wo_idx];
											ProcessDescr(r_aid, ds);
										}
										else {
											for(uint j = 0; j < p_prev_tok->Dl.getCount(); j++) {
												wo_idx = p_prev_tok->Dl.get(j);
												const AddrItemDescr & r_aid = Aidl[wo_idx];
												prev_ds = ds;
												if(ProcessDescr(r_aid, ds)) {
													if(prev_ds.T && ds.T > prev_ds.T)
														ds = prev_ds;
												}
											}
										}
										if(ds.T) {
											Add(last_t = ds.T, p_tok->S);
											if(ds.Kt && ds.Kind.NotEmpty()) {
												Add(ds.Kt, ds.Kind);
											}
											p_tok->Flags |= AddrTok::fUsed;
											p_prev_tok->Flags |= AddrTok::fUsed;
										}
									}
								}
								if(p_next_tok) {
									if(!ds.T && p_next_tok->T == AddrTok::tDescr) {
										for(uint j = 0; j < p_next_tok->Dl.getCount(); j++) {
											wo_idx = p_next_tok->Dl.get(j);
											const AddrItemDescr & r_aid = Aidl[wo_idx];
											if(r_aid.Flags & (AddrItemDescr::fSfx|AddrItemDescr::fOptSfx)) {
												prev_ds = ds;
												if(ProcessDescr(r_aid, ds)) {
													if(prev_ds.T && ds.T > prev_ds.T)
														ds = prev_ds;
												}
											}
										}
										if(ds.T) {
											Add(last_t = ds.T, p_tok->S);
											if(ds.Kt && ds.Kind.NotEmpty()) {
												Add(ds.Kt, ds.Kind);
											}
											p_tok->Flags |= AddrTok::fUsed;
											p_next_tok->Flags |= AddrTok::fUsed;
										}
									}
								}
							}
							break;
						case AddrTok::tDescr:
							/*
							if(p_next_tok && p_next_tok->T == AddrTok::tDescr) {
								const AddrTok * p_next_tok = (i < (c-1)) ? TokList.at(i+1) : 0;
								uint   wo_idx = 0;
								for(uint j = 0; j < p_tok->Dl.getCount(); j++) {
									wo_idx = p_tok->Dl.get(j);
									AddrItemDescr & r_aid = Aidl[wo_idx];
									if(!(r_aid.Flags & AddrItemDescr::fSfx)) {
										if(ProcessDescr(r_aid, ds) && ds.T == tStreet) {

										}
									}
								}
							}
							*/
							break;
						case AddrTok::tZip:
							if(p_tok->S.NotEmpty()) {
								Add(tZip, p_tok->S);
								p_tok->Flags |= AddrTok::fUsed;
							}
							break;
						case AddrTok::tNumber:
							{
								char   alpha = 0;
								if(p_next_tok && oneof2(p_next_tok->T, AddrTok::tText, AddrTok::tDescr) && p_next_tok->S.Len() == 1)
									alpha = p_next_tok->S.C(0);
								if(p_prev_tok) {
									if(p_prev_tok->T == AddrTok::tDescr) {
										for(uint j = 0; j < p_prev_tok->Dl.getCount(); j++) {
											const AddrItemDescr & r_aid = Aidl[p_prev_tok->Dl.get(j)];
											int r = ProcessDescr(r_aid, ds);
											if(oneof5(ds.T, tHouse, tHouseAddendum, tPostBox, tApart, tFloor)) { // @v11.5.10 tFloor
												temp_buf = p_tok->S;
												if(ds.T == tHouse && alpha) {
													temp_buf.CatChar(alpha);
													p_next_tok->Flags |= AddrTok::fUsed;
												}
												Add(last_t = ds.T, temp_buf);
												if(ds.Kt && ds.Kind.NotEmpty()) {
													Add(ds.Kt, ds.Kind);
												}
												p_tok->Flags |= AddrTok::fUsed;
												p_prev_tok->Flags |= AddrTok::fUsed;
											}
										}
									}
								}
								if(!(p_tok->Flags & AddrTok::fUsed)) {
									if(last_t == tStreet) {
										temp_buf = p_tok->S;
										if(alpha) {
											temp_buf.CatChar(alpha);
											p_next_tok->Flags |= AddrTok::fUsed;
										}
										Add(last_t = tHouse, temp_buf);
										p_tok->Flags |= AddrTok::fUsed;
									}
								}
							}
							break;
						case AddrTok::tNum_Hyp_Num:
							//if(last_t == tStreet) {
								p_tok->S.Divide('-', temp_buf.Z(), temp_buf2.Z());
								if(temp_buf.NotEmptyS()) {
									Add(last_t = tHouse, temp_buf);
									Add(tHouseKind, "���");
								}
								if(temp_buf2.NotEmptyS()) {
									Add(last_t = tApart, temp_buf2);
									Add(tApartKind, "��������");
								}
								p_tok->Flags |= AddrTok::fUsed;
							//}
							break;
						case AddrTok::tNum_Hyp_Num_Hyp_Num:
							//if(last_t == tStreet) {
								p_tok->S.Divide('-', temp_buf.Z(), temp_buf2.Z());
								if(temp_buf.NotEmptyS()) {
									Add(last_t = tHouse, temp_buf);
									Add(tHouseKind, "���");
								}
								if(temp_buf2.NotEmptyS()) {
									temp_buf2.Divide('-', temp_buf.Z(), temp_buf3.Z());
									if(temp_buf.NotEmptyS()) {
										Add(last_t = tHouseAddendum, temp_buf);
										Add(tHouseAddendumKind, "������");
									}
									if(temp_buf3.NotEmptyS()) {
										Add(last_t = tApart, temp_buf3);
										Add(tApartKind, "��������");
									}
								}
								p_tok->Flags |= AddrTok::fUsed;
							//}
							break;
						case AddrTok::tNum_Sl_Num:
							if(last_t == tStreet) {
								p_tok->S.Divide('/', temp_buf.Z(), temp_buf2.Z());
								temp_buf.Strip().Slash().Cat(temp_buf2.Strip());
								Add(last_t = tHouse, temp_buf);
								Add(tHouseKind, "���");
								p_tok->Flags |= AddrTok::fUsed;
							}
							break;
						case AddrTok::tNum_Al:
							break;
						case AddrTok::tNum_Hyp_Al:
							break;
						case AddrTok::tNum_Sl_Al:
							break;
					}
				}
			}
			{
				SString addendum;
				int    is_there_city = Search(tCity);
				int    is_there_street = Search(tStreet);
				if(P_Fr) {
					TSCollection <DetectBlock> dtb_list;
					PPIDArray fias_city_list;
					if(!is_there_city) {
						for(i = 0; i < c; i++) {
							AddrTok * p_tok = TokList.at(i);
							int cr = 0;
							if(!(p_tok->Flags & AddrTok::fUsed) && p_tok->T == AddrTok::tText) {
								DetectBlock * p_dtb = new DetectBlock;
								THROW_MEM(p_dtb);
								int cr = DetectCityName(p_dtb->Init(tCity, i, p_tok->S));
								if(cr) {
									THROW_SL(dtb_list.insert(p_dtb));
								}
								else
                                    delete p_dtb;
							}
						}
						{
							//
							// ����� ����������������� ����������� ������
							//
							LongArray single_fias_list;
							for(i = 0; i < dtb_list.getCount(); i++) {
								const DetectBlock * p1 = dtb_list.at(i);
								PPID  single_fias_id = 0;
								if(p1->T == tCity) {
									int   ambiguity = 0;
									for(uint cp = 0; !ambiguity && cp < p1->FiasCandidList.getCount(); cp++) {
										PPID cobj = p1->FiasCandidList.at(cp);
										if(!single_fias_id)
											single_fias_id = cobj;
										else {
											single_fias_id = 0;
											ambiguity = 1;
										}
									}
								}
								single_fias_list.add(single_fias_id);
							}
							assert(single_fias_list.getCount() == dtb_list.getCount());
							{
								uint  single_pos = 0;
								PPID  single_fias_id = 0;
								int   ambiguity = 0;
								for(i = 0; !ambiguity && i < single_fias_list.getCount(); i++) {
									if(single_fias_list.get(i)) {
										if(!single_fias_id) {
											single_fias_id = single_fias_list.get(i);
											single_pos = dtb_list.at(i)->P;
										}
										else {
											single_fias_id = 0;
											ambiguity = 1;
										}
									}
								}
								if(single_fias_id) {
									FiasCityID = single_fias_id;
									fias_city_list.add(FiasCityID);
									AddrTok * p_tok_city = TokList.at(single_pos);
									Add(tCity, p_tok_city->S);
									if(GetFiasAddrObjKind(single_fias_id, temp_buf) > 0)
										Add(tCityKind, temp_buf);
									p_tok_city->Flags |= AddrTok::fUsed;
									is_there_city = 1;
								}
							}
						}
					}
					else if(GetText(tCity, temp_buf) > 0) {
						FiasAddrObjTbl::Rec fao_rec;
						fao_list.clear();
						if(P_Fr->SearchObjByText(temp_buf, PPFiasReference::stfAnsiInput, 0, fao_list) > 0) {
							uint _p = fao_list.getCount();
							if(_p) do {
								const  PPID fao_id = fao_list.get(--_p);
								if(P_Fr->SearchObjByID(fao_id, &fao_rec, 1 /*use_cache*/) > 0 && fao_rec.LevelStatus >= 7) {
									fao_list.atFree(_p);
								}
							} while(_p);
							fias_city_list = fao_list;
							if(fao_list.getCount() == 1) {
								FiasCityID = fao_list.get(0);
							}
							else if(GetText(tCityKind, temp_buf2) > 0 && temp_buf2.NotEmpty()) {
								for(i = 0; !FiasCityID && i < fao_list.getCount(); i++) {
									const  PPID fao_id = fao_list.get(i);
									if(GetFiasAddrObjKind(fao_id, temp_buf) > 0 && temp_buf.CmpNC(temp_buf2) == 0) {
										FiasCityID = fao_id;
									}
								}
							}
						}
					}
					if(!is_there_street) {
						for(i = 0; i < c; i++) {
							AddrTok * p_tok = TokList.at(i);
							int cr = 0;
							if(!(p_tok->Flags & AddrTok::fUsed) && p_tok->T == AddrTok::tText) {
								if(fias_city_list.getCount() == 0)
									fias_city_list.add(0L);
								for(uint ci = 0; ci < fias_city_list.getCount(); ci++) {
									DetectBlock * p_dtb = new DetectBlock;
									THROW_MEM(p_dtb);
									const  PPID fias_city_id = fias_city_list.get(ci);
									int cr = DetectStreetName(p_dtb->Init(tStreet, i, p_tok->S, fias_city_id));
									if(cr) {
										dtb_list.insert(p_dtb);
									}
									else {
										delete p_dtb;
									}
								}
							}
						}
					}
					else if(GetText(tStreet, temp_buf) > 0) {
						PPIDArray local_fao_list;
						FiasAddrObjTbl::Rec fao_rec;
						fao_list.clear();
						for(uint ci = 0; ci < fias_city_list.getCount(); ci++) {
							const  PPID fias_city_id = fias_city_list.get(ci);
							local_fao_list.clear();
							if(P_Fr->SearchObjByText(temp_buf, PPFiasReference::stfAnsiInput, fias_city_id, local_fao_list) > 0) {
								uint _p = local_fao_list.getCount();
								if(_p) do {
									const  PPID fao_id = local_fao_list.get(--_p);
									if(P_Fr->SearchObjByID(fao_id, &fao_rec, 1 /*use_cache*/) > 0 && fao_rec.LevelStatus != 7) {
										local_fao_list.atFree(_p);
									}
								} while(_p);
								fao_list.add(&local_fao_list);
							}
						}
						fao_list.sortAndUndup();
						if(fao_list.getCount() == 1) {
							FiasStreetID = fao_list.get(0);
						}
						else if(GetText(tStreetKind, temp_buf2) > 0 && temp_buf2.NotEmpty()) {
							for(i = 0; !FiasStreetID && i < fao_list.getCount(); i++) {
								const  PPID fao_id = fao_list.get(i);
								if(GetFiasAddrObjKind(fao_id, temp_buf) > 0 && temp_buf.CmpNC(temp_buf2) == 0)
									FiasStreetID = fao_id;
							}
						}
						if(FiasStreetID) {
                            if(P_Fr->SearchObjByID(FiasStreetID, &fao_rec, 1) > 0)
                                FiasCityID = fao_rec.ParentUuRef;
                            else
								FiasStreetID = 0;
						}
					}
					{
						TSCollection <PPLocAddrStruc_MatchEntry> _match_list;
						for(i = 0; i < dtb_list.getCount(); i++) {
							const DetectBlock * p1 = dtb_list.at(i);
							for(uint j = i+1; j < dtb_list.getCount(); j++) {
								const DetectBlock * p2 = dtb_list.at(j);
								PPLocAddrStruc_MatchEntry * p_mentry = 0;
								PPLocAddrStruc_MatchEntry * p_mentry_reverse = 0;
								if(p1->T == tStreet && p2->T == tCity) {
									for(uint sp = 0; sp < p1->FiasCandidList.getCount(); sp++) {
										for(uint cp = 0; cp < p2->FiasCandidList.getCount(); cp++) {
                                            const  PPID sobj = p1->FiasCandidList.at(sp);
                                            const  PPID cobj = p2->FiasCandidList.at(cp);
                                            if(P_Fr->Match(sobj, cobj, -1) > 0) {
												if(!p_mentry_reverse) {
													THROW_MEM(p_mentry_reverse = new PPLocAddrStruc_MatchEntry(p1->P, p2->P, 1));
													p_mentry_reverse->P1 = p1->P;
													p_mentry_reverse->P2 = p2->P;
												}
												else {
													assert(p_mentry_reverse->Reverse != 0);
													assert(p_mentry_reverse->P1 == p1->P);
													assert(p_mentry_reverse->P2 == p2->P);
												}
												p_mentry_reverse->CityStreetList.Add(cobj, sobj, 0);
                                            }
										}
									}
								}
								else if(p1->T == tCity && p2->T == tStreet) {
									for(uint sp = 0; sp < p2->FiasCandidList.getCount(); sp++) {
										for(uint cp = 0; cp < p1->FiasCandidList.getCount(); cp++) {
                                            const  PPID sobj = p2->FiasCandidList.at(sp);
                                            const  PPID cobj = p1->FiasCandidList.at(cp);
                                            if(P_Fr->Match(sobj, cobj, -1) > 0) {
												if(!p_mentry) {
													THROW_MEM(p_mentry = new PPLocAddrStruc_MatchEntry(p1->P, p2->P, 0));
													p_mentry->P1 = p1->P;
													p_mentry->P2 = p2->P;
												}
												else {
													assert(p_mentry->Reverse == 0);
													assert(p_mentry->P1 == p1->P);
													assert(p_mentry->P2 == p2->P);
												}
												p_mentry->CityStreetList.Add(cobj, sobj, 0);
                                            }
										}
									}
								}
								if(p_mentry) {
									_match_list.insert(p_mentry);
									p_mentry = 0;
								}
								if(p_mentry_reverse) {
									_match_list.insert(p_mentry_reverse);
									p_mentry_reverse = 0;
								}
							}
						}
						if(_match_list.getCount() == 1) {
							const PPLocAddrStruc_MatchEntry * p_mentry = _match_list.at(0);
							assert(p_mentry);
							assert(p_mentry->CityStreetList.getCount());
							if(p_mentry) {
                                if(p_mentry->CityStreetList.getCount() == 1) {
									if(!is_there_city) {
										AddrTok * p_tok_city = TokList.at(p_mentry->Reverse ? p_mentry->P2 : p_mentry->P1);
										Add(tCity, p_tok_city->S);
										if(GetFiasAddrObjKind(p_mentry->CityStreetList.at(0).Key, temp_buf) > 0)
											Add(tCityKind, temp_buf);
										p_tok_city->Flags |= AddrTok::fUsed;
										FiasCityID = p_mentry->CityStreetList.at(0).Key;
										is_there_city = 1;
									}
									if(!is_there_street) {
										AddrTok * p_tok_street = TokList.at(p_mentry->Reverse ? p_mentry->P1 : p_mentry->P2);
										Add(tStreet, p_tok_street->S);
										if(GetFiasAddrObjKind(p_mentry->CityStreetList.at(0).Val, temp_buf) > 0) {
											Add(tStreetKind, temp_buf);
										}
										p_tok_street->Flags |= AddrTok::fUsed;
										FiasStreetID = p_mentry->CityStreetList.at(0).Val;
										is_there_street = 1;
									}
                                }
                                else if(p_mentry->CityStreetList.getCount() > 1) {
									// �� ������������� � ���������� ������/�����
                                    THROW_MEM(P_AmbigMatchEntry = new PPLocAddrStruc_MatchEntry(*p_mentry));
                                }
							}
						}
						else if(_match_list.getCount() > 1) {
							// ��������������� � ���������� ������/�����
							THROW_MEM(P_AmbigMatchList = new TSCollection <PPLocAddrStruc_MatchEntry>);
							THROW_SL(TSCollection_Copy(*P_AmbigMatchList, _match_list));
						}
					}
					if(FiasStreetID) {
						temp_buf.Z();
						if(GetText(tHouse, temp_buf2) > 0) {
                            (temp_buf = temp_buf2).ToLower1251();
                            temp_buf.Colon();
							if(GetText(tHouseAddendum, temp_buf2) > 0) {
								temp_buf.Cat(temp_buf2);
							}
							temp_buf.Colon();
							P_Fr->IdentifyHouse(FiasStreetID, temp_buf, &FiasHouseID);
						}
					}
				}
				else {
					if(!is_there_city) {
						//
						// ���� ����� �� ���������, �� �������� �� ���������������� ��������� �������
						// ������� ������������ ������
						//
						DetectBlock dtb;
						for(i = 0; i < c; i++) {
							AddrTok * p_tok = TokList.at(i);
							int cr = 0;
							if(!(p_tok->Flags & AddrTok::fUsed) && p_tok->T == AddrTok::tText && (cr = DetectCityName(dtb.Init(tCity, i, p_tok->S))) != 0) {
								Add(tCity, dtb.RecognizedText);
								p_tok->Flags |= AddrTok::fUsed;
								is_there_city = 1;
							}
						}
					}
					if(!is_there_street) {
						//
						// ���� ����� �� ����������, �� ������ ��������� ����� ����������� ��� ������������ �����
						//
						for(i = 0; i < c; i++) {
							AddrTok * p_tok = TokList.at(i);
							if(!(p_tok->Flags & AddrTok::fUsed) && p_tok->T == AddrTok::tText) {
								Add(tStreet, p_tok->S);
								p_tok->Flags |= AddrTok::fUsed;
								is_there_street = 1;
							}
						}
					}
				}
				last_t = 0;
				for(i = 0; i < c; i++) {
					const AddrTok * p_tok = TokList.at(i);
					if(!(p_tok->Flags & AddrTok::fUsed)) {
						if(oneof6(p_tok->T, AddrTok::tText, AddrTok::tNumber, AddrTok::tNum_Hyp_Num, AddrTok::tNum_Sl_Num, AddrTok::tZip, AddrTok::tDescr)) {
							if(last_t == p_tok->T && p_tok->T == AddrTok::tText)
								addendum.Space();
							else
								addendum.CatDivIfNotEmpty(';', 2);
							addendum.Cat(p_tok->S);
						}
						last_t = p_tok->T;
					}
					if(addendum.NotEmptyS()) {
						Add(tAddendum, addendum);
					}
				}
			}
		}
		if(FiasCityID)
			Add(tFiasCityID, temp_buf.Z().Cat(FiasCityID));
		else if(do_enforce_fias) {

		}
		if(FiasStreetID)
			Add(tFiasStreetID, temp_buf.Z().Cat(FiasStreetID));
		else if(do_enforce_fias) {
			if(FiasCityID) {
				if(P_Fr->GetRandomAddress(0, FiasCityID, &FiasStreetID, &FiasHouseID) > 0) {
					Add(tFiasStreetID, temp_buf.Z().Cat(FiasStreetID));
					ok = 102;
				}
			}
		}
		if(FiasHouseID)
			Add(tFiasHouseID, temp_buf.Z().Cat(FiasHouseID));
		else if(do_enforce_fias) {
			if(FiasStreetID) {
				int rhr = P_Fr->GetRandomHouse(0, FiasStreetID, &FiasHouseID);
				if(rhr > 0) {
					Add(tFiasHouseID, temp_buf.Z().Cat(FiasHouseID));
					ok = 103;
				}
				else if(rhr < 0 && FiasCityID) {
					if(P_Fr->GetRandomAddress(0, FiasCityID, &FiasStreetID, &FiasHouseID) > 0) {
						Add(tFiasStreetID, temp_buf.Z().Cat(FiasStreetID));
						Add(tFiasHouseID, temp_buf.Z().Cat(FiasHouseID));
						ok = 102;
					}
				}
			}
		}
		if(FiasTerminalID)
			Add(tFiasTerminalID, temp_buf.Z().Cat(FiasTerminalID));
	}
	CATCHZOK
	return ok;
}

int PPLocAddrStruc::Output(SString & rBuf)
{
	rBuf.Z();
	SString temp_buf, temp_buf2;
	if(GetText(tCountry, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2).Cat(temp_buf);
	}
	if(GetText(tZip, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2).Cat(temp_buf);
	}
	if(GetText(tLocalArea, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2);
		if(GetText(tLocalAreaKind, temp_buf2))
			rBuf.Cat(temp_buf2).Space();
		rBuf.Cat(temp_buf);
	}
	if(GetText(tCity, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2);
		if(GetText(tCityKind, temp_buf2))
			rBuf.Cat(temp_buf2).Space();
		rBuf.Cat(temp_buf);
	}
	if(GetText(tStreet, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2);
		if(GetText(tStreetKind, temp_buf2))
			rBuf.Cat(temp_buf2).Space();
		rBuf.Cat(temp_buf);
	}
	if(GetText(tHouse, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2);
		if(GetText(tHouseKind, temp_buf2))
			rBuf.Cat(temp_buf2).Space();
		else
			rBuf.Cat("house").Space();
		rBuf.Cat(temp_buf);
	}
	if(GetText(tHouseAddendum, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2);
		if(GetText(tHouseAddendumKind, temp_buf2))
			rBuf.Cat(temp_buf2).Space();
		else
			rBuf.Cat("houseaddendum").Space();
		rBuf.Cat(temp_buf);
	}
	if(GetText(tApart, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2);
		if(GetText(tApartKind, temp_buf2))
			rBuf.Cat(temp_buf2).Space();
		else
			rBuf.Cat("apart").Space();
		rBuf.Cat(temp_buf);
	}
	if(GetText(tPostBox, temp_buf)) {
		rBuf.CatDivIfNotEmpty(',', 2).Cat("PostBox").Space().Cat(temp_buf);
	}
	if(GetText(tAddendum, temp_buf)) {
		rBuf.Space().CatChar('(').Cat(temp_buf).CatChar(')');
	}
	return 1;
}

void PPLocAddrStruc::OutputTokList(const TSCollection <AddrTok> & rList, SString & rBuf)
{
	for(uint i = 0; i < rList.getCount(); i++) {
		const AddrTok * p_tok = rList.at(i);
		switch(p_tok->T) {
			case AddrTok::tFinish: rBuf.Cat("Finish"); break;
			case AddrTok::tDiv:    rBuf.Cat("Div"); break;
			case AddrTok::tText:   rBuf.Cat("Text"); break;
			case AddrTok::tDescr:  rBuf.Cat("Descr"); break;
			case AddrTok::tZip:    rBuf.Cat("ZIP"); break;
			case AddrTok::tNumber: rBuf.Cat("Number"); break;
			case AddrTok::tNum_Hyp_Num: rBuf.Cat("Num-Num"); break;
			case AddrTok::tNum_Sl_Num:  rBuf.Cat("Num/Num"); break;
			case AddrTok::tNum_Al:      rBuf.Cat("NumAlph"); break;
			case AddrTok::tNum_Hyp_Al:  rBuf.Cat("Num-Alph"); break;
			case AddrTok::tNum_Sl_Al:   rBuf.Cat("Num/Alph"); break;
			default: rBuf.Cat("???"); break;
		}
		rBuf.CatChar('(').Cat(p_tok->S).CatChar(')');
		if(p_tok->Flags & AddrTok::fDot)
			rBuf.Dot();
		if(p_tok->T == AddrTok::tDescr) {
			rBuf.Space();
			if(p_tok->Dl.getCount()) {
				rBuf.CatChar('[');
				for(uint j = 0; j < p_tok->Dl.getCount(); j++) {
					const uint idx = p_tok->Dl.get(j);
					const AddrItemDescr & r_aid = Aidl[idx];
					if(r_aid.MainId) {
						for(uint k = 0; k < SIZEOFARRAY(Aidl); k++) {
							if(Aidl[k].Id == r_aid.MainId)
								rBuf.Cat(Aidl[k].P_Descr);
						}
					}
					else
						rBuf.Cat(r_aid.P_Descr);
					if((j+1) < p_tok->Dl.getCount())
						rBuf.Semicol();
				}
				rBuf.CatChar(']');
			}
		}
	}
}

int PPLocAddrStruc::Recognize(const char * pText, TSCollection <AddrTok> & rTokList) // @debug
{
	Style = 0;
	Flags = 0;
	CountryID = 0;
	CityID = 0;
	StreetID = 0;
	Z();
	if(pText) {
		SString kind_text;
		Scan.Set(pText, 0);
		{
			AddrTok * p_tok = new AddrTok;
			while(GetTok(*p_tok)) {
				rTokList.insert(p_tok);
				p_tok = new AddrTok;
			}
			delete p_tok;
		}
	}
	return 1;
}

// D:\PAPYRUS\Src\PPTEST\DATA\address.csv

int TestAddressRecognition()
{
	PPFiasReference * p_fr = 0;
	SString path;
	SString file_name;
	PPGetPath(PPPATH_TESTROOT, path);
	(file_name = path).SetLastSlash().Cat("data").SetLastSlash().Cat("address.txt");
	SFile in_file(file_name, SFile::mRead);
	(file_name = path).SetLastSlash().Cat("data").SetLastSlash().Cat("address.out");
	SFile out_file(file_name, SFile::mWrite);
	if(in_file.IsValid()) {
		if(DS.GetConstTLA().IsAuth()) {
			p_fr = new PPFiasReference;
		}
		PPLocAddrStruc las(0, p_fr);
		SString line_buf, out_buf, temp_buf;
		while(in_file.ReadLine(line_buf, SFile::rlfChomp)) {
			out_buf.Z().Cat(line_buf).Cat("-->");
			/*
			TSCollection <AddrTok> tok_list;
			las.Recognize(line_buf.Chomp(), tok_list);
			las.OutputTokList(tok_list, out_buf);
			*/
			las.Recognize(line_buf);
			las.Output(temp_buf);
			if(las.GetAmbiguityMatchEntry()) {
				temp_buf.CR();
				las.MatchEntryToStr(las.GetAmbiguityMatchEntry(), temp_buf);
				temp_buf.CR();
			}
			else if(las.GetAmbiguityMatchList()) {
				temp_buf.CR();
				const  TSCollection <PPLocAddrStruc_MatchEntry> * p_aml = las.GetAmbiguityMatchList();
				for(uint i = 0; i < p_aml->getCount(); i++) {
					las.MatchEntryToStr(las.GetAmbiguityMatchEntry(), temp_buf);
					temp_buf.CR();
				}
			}
			out_buf.Cat(temp_buf);
			//
			out_file.WriteLine(out_buf.CR());
		}
	}
	delete p_fr;
	return 1;
}

class AddrStrucDialog : public TDialog {
	DECL_DIALOG_DATA(SString);
public:
	AddrStrucDialog() : TDialog(DLG_ADDRSTRUC), Las(0, &Fr)
	{
		// @v11.3.1 ������� TWindow::getLabelText ������ ���� �������������� ������� ����� � ������ text.Z() �������������
		SString text;
		getLabelText(CTL_ADDRSTRUC_CITY, text);
		OrgLabels.Add(Las.tCityKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_LOCALAREA, text);
		OrgLabels.Add(Las.tLocalAreaKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_STREET, text);
		OrgLabels.Add(Las.tStreetKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_HOUSE, text);
		OrgLabels.Add(Las.tHouseKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_HOUSEADD, text);
		OrgLabels.Add(Las.tHouseAddendumKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_APART, text);
		OrgLabels.Add(Las.tApartKind, text.Transf(CTRANSF_INNER_TO_OUTER));
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Z();
		setCtrlString(CTL_ADDRSTRUC_SRC, Data);
		Setup();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_ADDRSTRUC_SRC)) {
				SString preserve_buf = Data;
				getCtrlString(CTL_ADDRSTRUC_SRC, Data);
				if(Data.CmpNC(preserve_buf) != 0)
					Setup();
			}
			else
				return;
		}
	}
	void   Setup()
	{
		SString text, kind_text;
		(text = Data).Transf(CTRANSF_INNER_TO_OUTER);
		Las.Recognize(text);

		Las.GetText(Las.tCountry, text);
		setCtrlString(CTL_ADDRSTRUC_COUNTRY, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tCity, text);
		setCtrlString(CTL_ADDRSTRUC_CITY, text.Transf(CTRANSF_OUTER_TO_INNER));
		if(Las.GetText(Las.tCityKind, text) || OrgLabels.GetText(Las.tCityKind, text))
			setLabelText(CTL_ADDRSTRUC_CITY, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tLocalArea, text);
		setCtrlString(CTL_ADDRSTRUC_LOCALAREA, text.Transf(CTRANSF_OUTER_TO_INNER));
		if(Las.GetText(Las.tLocalAreaKind, text) || OrgLabels.GetText(Las.tLocalAreaKind, text))
			setLabelText(CTL_ADDRSTRUC_LOCALAREA, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tZip, text);
		setCtrlString(CTL_ADDRSTRUC_ZIP, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tStreet, text);
		setCtrlString(CTL_ADDRSTRUC_STREET, text.Transf(CTRANSF_OUTER_TO_INNER));
		if(Las.GetText(Las.tStreetKind, text) || OrgLabels.GetText(Las.tStreetKind, text))
			setLabelText(CTL_ADDRSTRUC_STREET, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tHouse, text);
		setCtrlString(CTL_ADDRSTRUC_HOUSE, text.Transf(CTRANSF_OUTER_TO_INNER));
		if(Las.GetText(Las.tHouseKind, text) || OrgLabels.GetText(Las.tHouseKind, text))
			setLabelText(CTL_ADDRSTRUC_HOUSE, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tHouseAddendum, text);
		setCtrlString(CTL_ADDRSTRUC_HOUSEADD, text.Transf(CTRANSF_OUTER_TO_INNER));
		if(Las.GetText(Las.tHouseAddendumKind, text) || OrgLabels.GetText(Las.tHouseAddendumKind, text))
			setLabelText(CTL_ADDRSTRUC_HOUSEADD, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tApart, text);
		setCtrlString(CTL_ADDRSTRUC_APART, text.Transf(CTRANSF_OUTER_TO_INNER));
		if(Las.GetText(Las.tApartKind, text) || OrgLabels.GetText(Las.tApartKind, text))
			setLabelText(CTL_ADDRSTRUC_APART, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tFloor, text);
		setCtrlString(CTL_ADDRSTRUC_FLOOR, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tPostBox, text);
		setCtrlString(CTL_ADDRSTRUC_POSTBOX, text.Transf(CTRANSF_OUTER_TO_INNER));

		Las.GetText(Las.tAddendum, text);
		setCtrlString(CTL_ADDRSTRUC_ADDENDUM, text.Transf(CTRANSF_OUTER_TO_INNER));
		{
			text.Z();
			kind_text.Z();
			FiasAddrObjTbl::Rec fias_rec;
			if(Las.FiasStreetID) {
				if(Fr.SearchObjByID(Las.FiasStreetID, &fias_rec, 1 /*use_cache*/) > 0) {
					S_GUID uuid;
					if(Fr.FT.UrT.Search(fias_rec.IdUuRef, uuid) > 0) {
						uuid.ToStr(S_GUID::fmtIDL, text);
						kind_text = "street UUID";
					}
				}
			}
			else if(Las.FiasCityID) {
				if(Fr.SearchObjByID(Las.FiasCityID, &fias_rec, 1 /*use_cache*/) > 0) {
					S_GUID uuid;
					if(Fr.FT.UrT.Search(fias_rec.IdUuRef, uuid) > 0) {
						uuid.ToStr(S_GUID::fmtIDL, text);
						kind_text = "city UUID";
					}
				}
			}
			setCtrlString(CTL_ADDRSTRUC_FIASID, text);
			if(kind_text.NotEmptyS())
				setLabelText(CTL_ADDRSTRUC_FIASID, kind_text.Transf(CTRANSF_OUTER_TO_INNER));
			//
			text.Z();
			if(Las.FiasHouseID) {
                FiasHouseObjTbl::Rec house_rec;
                if(Fr.SearchHouseByID(Las.FiasHouseID, &house_rec) > 0) {
					S_GUID uuid;
					if(Fr.FT.UrT.Search(house_rec.IdUuRef, uuid) > 0)
						uuid.ToStr(S_GUID::fmtIDL, text);
                }
			}
			setCtrlString(CTL_ADDRSTRUC_FIASHSEID, text);
		}
	}
	StrAssocArray OrgLabels;
	PPFiasReference Fr;
	PPLocAddrStruc Las;
};

int PPObjLocation::EditAddrStruc(SString & rAddr)
{
	return PPDialogProcBody <AddrStrucDialog, SString> (&rAddr);
}

int PPObjLocation::IndexPhones(PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	SString phone, main_city_prefix, city_prefix, temp_buf;
	LocationTbl::Key2 k2;
	MEMSZERO(k2);
	{
		PPID   main_city_id = 0;
		WorldTbl::Rec main_city_rec;
		if(GetMainCityID(&main_city_id) > 0 && FetchCity(main_city_id, &main_city_rec) > 0)
			PPEAddr::Phone::NormalizeStr(main_city_rec.Phone, 0, main_city_prefix);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		k2.Type = LOCTYP_ADDRESS;
		if(P_Tbl->search(2, &k2, spGe) && P_Tbl->data.Type == LOCTYP_ADDRESS) do {
			LocationCore::GetExField(&P_Tbl->data, LOCEXSTR_PHONE, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
			PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone);
			if(phone.Len() >= 5) {
				PPID   city_id = P_Tbl->data.CityID;
				city_prefix = 0;
				WorldTbl::Rec city_rec;
				if(FetchCity(city_id, &city_rec) > 0 && city_rec.Phone[0])
					PPEAddr::Phone::NormalizeStr(city_rec.Phone, 0, city_prefix);
				city_prefix.SetIfEmpty(main_city_prefix);
				PPObjID objid(PPOBJ_LOCATION, P_Tbl->data.ID);
				if(city_prefix.Len()) {
					size_t sl = phone.Len() + city_prefix.Len();
					if(oneof2(sl, 10, 11))
						phone = (temp_buf = city_prefix).Cat(phone);
				}
				if(!P_Tbl->IndexPhone(phone, &objid, 0, 0)) {
					if(pLogger)
						pLogger->LogLastError();
					else
						CALLEXCEPT();
				}
			}
		} while(P_Tbl->search(2, &k2, spNext) && P_Tbl->data.Type == LOCTYP_ADDRESS);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_UhttLocation
//
struct UhttLocationBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttLocationBlock() : State(stFetch)
	{
	}
	UhttLocationBlock & Z()
	{
		Rec.Clear();
		State = stFetch;
		return *this;
	}
	PPObjLocation LObj;
	LocationTbl::Rec Rec;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttLocation)
{
	if(Valid) {
		MEMSZERO(H);
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new UhttLocationBlock;
	}
}

PPALDD_DESTRUCTOR(UhttLocation)
{
	Destroy();
	delete static_cast<UhttLocationBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttLocation::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttLocationBlock & r_blk = *static_cast<UhttLocationBlock *>(Extra[0].Ptr);
	r_blk.Z();
	MEMSZERO(H);
	if(r_blk.LObj.Search(rFilt.ID, &r_blk.Rec) > 0) {
		SString temp_buf;
		H.ID        = r_blk.Rec.ID;
		H.ParentID  = r_blk.Rec.ParentID;
		H.OwnerID   = r_blk.Rec.OwnerID;
		H.Type      = r_blk.Rec.Type;
		H.CityID    = r_blk.Rec.CityID;
		H.Flags     = r_blk.Rec.Flags;
		H.Latitude  = r_blk.Rec.Latitude;
		H.Longitude = r_blk.Rec.Longitude;
		STRNSCPY(H.Code, r_blk.Rec.Code);
		LocationCore::GetExField(&r_blk.Rec, LOCEXSTR_ZIP, temp_buf);
		temp_buf.CopyTo(H.ZIP, sizeof(H.ZIP));
		LocationCore::GetExField(&r_blk.Rec, LOCEXSTR_PHONE, temp_buf);
		temp_buf.CopyTo(H.Phone, sizeof(H.Phone));
		LocationCore::GetExField(&r_blk.Rec, LOCEXSTR_CONTACT, temp_buf);
		temp_buf.CopyTo(H.Contact, sizeof(H.Contact));
		STRNSCPY(H.Name, r_blk.Rec.Name);
		LocationCore::GetExField(&r_blk.Rec, LOCEXSTR_SHORTADDR, temp_buf);
		//LocationCore::GetAddress(&rec, 0, temp_buf);
		temp_buf.CopyTo(H.Address, sizeof(H.Address));
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttLocation::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttLocationBlock & r_blk = *static_cast<UhttLocationBlock *>(Extra[0].Ptr);
	if(r_blk.State != UhttLocationBlock::stSet) {
		r_blk.Z();
		r_blk.State = UhttLocationBlock::stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			r_blk.Rec.ID        = H.ID;
			r_blk.Rec.ParentID  = H.ParentID;
			r_blk.Rec.OwnerID   = H.OwnerID;
			r_blk.Rec.Type      = LOCTYP_ADDRESS;  // H.Type;
			r_blk.Rec.CityID    = H.CityID;
			r_blk.Rec.Flags     = H.Flags;
			r_blk.Rec.Latitude  = H.Latitude;
			r_blk.Rec.Longitude = H.Longitude;
			STRNSCPY(r_blk.Rec.Code, strip(H.Code));
			if(isempty(r_blk.Rec.Code) || sstreq(r_blk.Rec.Code, "0"))
				r_blk.LObj.InitCode(&r_blk.Rec);
			STRNSCPY(r_blk.Rec.Name, strip(H.Name));
			LocationCore::SetExField(&r_blk.Rec, LOCEXSTR_ZIP, H.ZIP);
			LocationCore::SetExField(&r_blk.Rec, LOCEXSTR_PHONE, H.Phone);
			LocationCore::SetExField(&r_blk.Rec, LOCEXSTR_EMAIL, H.EMail);
			LocationCore::SetExField(&r_blk.Rec, LOCEXSTR_CONTACT, H.Contact);
			if(H.Address[0] && !sstreq(H.Address, "0")) {
				if(r_blk.Rec.Flags & LOCF_MANUALADDR)
					LocationCore::SetExField(&r_blk.Rec, LOCEXSTR_FULLADDR, H.Address);
				else
					LocationCore::SetExField(&r_blk.Rec, LOCEXSTR_SHORTADDR, H.Address);
			}
		}
	}
	else {
		PPID  id = r_blk.Rec.ID;
		// @v8.3.2 {
		/* �������� ���� �� ���� ����������� ������������ ����� ���������� ������������
		if(r_blk.Rec.Flags & LOCF_STANDALONE && r_blk.Rec.Type == LOCTYP_ADDRESS) {
			PPGlobalAccRights rb(PPTAG_GUA_SALOCRIGHTS);
			THROW((id == 0) ? rb.IsAllow(PPGlobalAccRights::fCreate) : rb.IsAllow(PPGlobalAccRights::fEdit));
		}
		*/
		// } @v8.3.2
		THROW(r_blk.LObj.PutRecord(&id, &r_blk.Rec, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Z();
	return ok;
}
//
// Implementation of PPALDD_UhttStore
//
struct UhttStoreBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttStoreBlock() : TagPos(0), State(stFetch)
	{
	}
	UhttStoreBlock & Z()
	{
		Pack.destroy();
		TagPos = 0;
		State = stFetch;
		return *this;
	}
	PPObjUhttStore UhttStoreObj;
	PPObjTag TagObj;
	PPUhttStorePacket Pack;
	uint   TagPos;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttStore)
{
	if(Valid) {
		Extra[0].Ptr = new UhttStoreBlock;
		InitFixData(rscDefHdr, &H, sizeof(H));
		InitFixData("iter@TagList", &I_TagList, sizeof(I_TagList));
	}
}

PPALDD_DESTRUCTOR(UhttStore)
{
	Destroy();
	delete static_cast<UhttStoreBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttStore::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttStoreBlock & r_blk = *static_cast<UhttStoreBlock *>(Extra[0].Ptr);
	r_blk.Z();
	MEMSZERO(H);
	if(r_blk.UhttStoreObj.GetPacket(rFilt.ID, &r_blk.Pack) > 0) {
		H.ID        = r_blk.Pack.Rec.ID;
		H.OwnerID   = r_blk.Pack.Rec.PersonID;
		H.LocID     = r_blk.Pack.Rec.LocID;
		H.Kind      = r_blk.Pack.Rec.Kind;
		H.Flags     = r_blk.Pack.Rec.Flags;
		H.UpRestShowThreshold = r_blk.Pack.Rec.UpRestShowThreshold;
		STRNSCPY(H.Name, r_blk.Pack.Rec.Name);
		STRNSCPY(H.Symb, r_blk.Pack.Rec.Symb);
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttStore::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttStoreBlock & r_blk = *static_cast<UhttStoreBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@TagList"))
		r_blk.TagPos = 0;
	return 1;
}

int PPALDD_UhttStore::NextIteration(long iterId)
{
	int     ok = -1;
	SString temp_buf;
	IterProlog(iterId, 0);
	UhttStoreBlock & r_blk = *static_cast<UhttStoreBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@TagList")) {
		if(r_blk.TagPos < r_blk.Pack.TagL.GetCount()) {
			MEMSZERO(I_TagList);
			const ObjTagItem * p_item = r_blk.Pack.TagL.GetItemByPos(r_blk.TagPos);
			I_TagList.TagTypeID = p_item->TagDataType;
			{
				PPObjectTag tag_rec;
				if(r_blk.TagObj.Fetch(p_item->TagID, &tag_rec) > 0)
					STRNSCPY(I_TagList.TagSymb, tag_rec.Symb);
			}
			switch(p_item->TagDataType) {
				case OTTYP_STRING:
				case OTTYP_GUID:
					p_item->GetStr(temp_buf.Z());
					STRNSCPY(I_TagList.StrVal, temp_buf);
					break;
				case OTTYP_NUMBER: p_item->GetReal(&I_TagList.RealVal); break;
				case OTTYP_BOOL:
				case OTTYP_INT: p_item->GetInt(&I_TagList.IntVal); break;
				case OTTYP_DATE: p_item->GetDate(&I_TagList.DateVal); break;
			}
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.TagPos++;
	}
	return ok;
}

int PPALDD_UhttStore::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttStoreBlock & r_blk = *static_cast<UhttStoreBlock *>(Extra[0].Ptr);
	if(r_blk.State != UhttStoreBlock::stSet) {
		r_blk.Z();
		r_blk.State = UhttStoreBlock::stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			r_blk.Pack.Rec.ID       = H.ID;
			r_blk.Pack.Rec.PersonID = H.OwnerID;
			r_blk.Pack.Rec.LocID    = H.LocID;
			r_blk.Pack.Rec.Flags    = H.Flags;
			r_blk.Pack.Rec.Kind     = H.Kind;
			r_blk.Pack.Rec.UpRestShowThreshold = H.UpRestShowThreshold;
			STRNSCPY(r_blk.Pack.Rec.Name, strip(H.Name));
			STRNSCPY(r_blk.Pack.Rec.Symb, strip(H.Symb));
		}
		else if(iterId == GetIterID("iter@TagList")) {
			PPID   id = 0;
			ObjTagItem item;
			PPObjectTag tag_rec;
			if(r_blk.TagObj.SearchBySymb(I_TagList.TagSymb, &id, &tag_rec) > 0) {
				item.TagID = tag_rec.ID;
				switch(I_TagList.TagTypeID) {
					case OTTYP_STRING:
					case OTTYP_GUID: THROW(item.SetStr(I_TagList.TagTypeID, I_TagList.StrVal)); break;
					case OTTYP_NUMBER: THROW(item.SetReal(I_TagList.TagTypeID, I_TagList.RealVal)); break;
					case OTTYP_INT: THROW(item.SetInt(I_TagList.TagTypeID, I_TagList.IntVal)); break;
					case OTTYP_DATE: THROW(item.SetDate(I_TagList.TagTypeID, I_TagList.DateVal)); break;
				}
				THROW(r_blk.Pack.TagL.PutItem(tag_rec.ID, &item));
			}
		}
	}
	else {
		PPID  id = r_blk.Pack.Rec.ID;
		THROW(r_blk.UhttStoreObj.PutPacket(&id, &r_blk.Pack, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Z();
	return ok;
}
//
//
//
/*static*/int PPFiasReference::IdentifyShortDescription(const char * pText, int * pLevel, SString * pFullText)
{
	int    ok = 0;
	int    level = DEREFPTRORZ(pLevel);
	ASSIGN_PTR(pFullText, 0);
	SString temp_buf;
	(temp_buf = pText).Transf(CTRANSF_INNER_TO_OUTER);
	if(temp_buf.NotEmptyS()) {
		for(uint i = 0; !ok && i < SIZEOFARRAY(_FiasAbbrList); i++) {
			const FiasAbbrEntry & r_entry = _FiasAbbrList[i];
			if(temp_buf.CmpNC(r_entry.P_Abbr) == 0 && (!level || level == r_entry.Level)) {
                level = r_entry.Level;
                if(pFullText) {
					(temp_buf = r_entry.P_Text).Transf(CTRANSF_OUTER_TO_INNER);
					*pFullText = temp_buf;
                }
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pLevel, level);
	return ok;
}

PPFiasReference::PPFiasReference()
{
}

PPFiasReference::~PPFiasReference()
{
}

int PPFiasReference::SearchObjByID(PPID id, FiasAddrObjTbl::Rec * pRec, int useCache)
{
	int    ok = -1;
	int    do_straight_search = 1;
	if(useCache) {
		FiasAddrCache * p_cache = GetDbLocalCachePtr <FiasAddrCache> (PPOBJ_FIAS);
		if(p_cache) {
			p_cache->SetTable(this);
			ok = p_cache->Get(id, pRec);
			p_cache->SetTable(0);
			do_straight_search = 0;
		}
	}
	if(do_straight_search)
		ok = FT.SearchAddrByID(id, pRec);
	return ok;
}

int PPFiasReference::SearchObjByUUID(const S_GUID & rUuid, FiasAddrObjTbl::Rec * pRec)
{
	return FT.SearchAddrByUUID(rUuid, pRec);
}

int PPFiasReference::SearchHouseByID(PPID id, FiasHouseObjTbl::Rec * pRec)
{
	return FT.SearchHouse(id, pRec);
}

int PPFiasReference::SearchHouseByUUID(const S_GUID & rUuid, FiasHouseObjTbl::Rec * pRec)
{
	return FT.SearchHouseByUUID(rUuid, pRec);
}

int PPFiasReference::GetText(PPID textRef, SString & rBuf)
{
	int    ok = 0;
	if(textRef) {
		SStringU temp_buf_u;
		if(PPRef->TrT.SearchSelfRef(textRef, temp_buf_u) > 0) {
			temp_buf_u.CopyToUtf8(rBuf, 1);
			rBuf.Transf(CTRANSF_UTF8_TO_INNER);
			ok = 1;
		}
	}
	return ok;
}

int PPFiasReference::Helper_GetHierarchy(PPID id, long flags, FiasHouseObjTbl::Rec * pHseRec, TSArray <FiasAddrObjTbl::Rec> & rList, long * pZip)
{
	rList.clear();
	int    ok = -1;
	FiasAddrObjTbl::Rec adr_rec;
	FiasHouseObjTbl::Rec hse_rec;
	long   zip = 0;
	PPID   adr_id = id;
	if(SearchObjByID(adr_id, &adr_rec, 1 /*use_cache*/) > 0) {
		zip = adr_rec.PostalCode;
		rList.insert(&adr_rec);
		adr_id = adr_rec.ParentUuRef;
		ok = 1;
	}
	else if(flags & matfTryHouse && FT.SearchHouse(adr_id, &hse_rec) > 0) {
		zip = hse_rec.PostalCode;
		adr_id = hse_rec.ParentUuRef;
		ASSIGN_PTR(pHseRec, hse_rec);
		ok = 2;
	}
	else
		adr_id = 0;
	for(; SearchObjByID(adr_id, &adr_rec, 1 /*use_cache*/) > 0; adr_id = adr_rec.ParentUuRef) {
		rList.insert(&adr_rec);
	}
	ASSIGN_PTR(pZip, zip);
	return ok;
}

uint PPFiasReference::IsObjInHierarchy(PPID objID, const TSArray <FiasAddrObjTbl::Rec> & rList) const
{
	uint   result = 0;
	for(uint p = 0; !result && p < rList.getCount(); p++) {
		if(rList.at(p).IdUuRef == objID)
			result = p+1;
	}
	return result;
}

int PPFiasReference::Match(PPID obj1ID, PPID obj2ID, int vect)
{
	//
	// Returns:
	//   0 - error
	//  -1 - not match
	//   1 - obj1ID belong to obj2ID
	//   2 - obj2ID belong to obj1ID
	//   3 - obj1ID equal obj2ID
	//
    int    ok = -1;
    if(obj1ID == obj2ID) {
		ok = (obj1ID == 0) ? -1 : 3;
    }
	else {
		TSArray <FiasAddrObjTbl::Rec> list1;
		TSArray <FiasAddrObjTbl::Rec> list2;
		FiasHouseObjTbl::Rec hse1_rec;
		FiasHouseObjTbl::Rec hse2_rec;
		long   zip1 = 0;
		long   zip2 = 0;
		int    hr1 = 0;
		int    hr2 = 0;
		if(vect == 0) {
			hr1 = Helper_GetHierarchy(obj1ID, matfTryHouse, &hse1_rec, list1, &zip1);
			hr2 = Helper_GetHierarchy(obj2ID, matfTryHouse, &hse2_rec, list2, &zip2);
			if(hr1 > 0 && hr2 > 0) {
				if(hr1 == 2) { // obj1ID - house
					if(hr2 == 2) { // obj2ID - house
						ok = -1; // ��� �� ������������� ���� �� ����� ��������������� ����-�������
					}
					else {
						if(IsObjInHierarchy(obj2ID, list1))
							ok = 1; // obj1ID belong to obj2ID
					}
				}
				else if(hr2 == 2) {
					if(IsObjInHierarchy(obj1ID, list2))
						ok = 2; // obj2ID belong to obj1ID
				}
				else if(IsObjInHierarchy(obj2ID, list1))
					ok = 1; // obj1ID belong to obj2ID
				else if(IsObjInHierarchy(obj1ID, list2))
					ok = 2; // obj2ID belong to obj1ID
			}
		}
		else if(vect < 0) { // obj1ID �������� �� ������ ���� obj2ID
			hr1 = Helper_GetHierarchy(obj1ID, matfTryHouse, &hse1_rec, list1, &zip1);
			if(hr1 > 0 && IsObjInHierarchy(obj2ID, list1))
				ok = 1; // obj1ID belong to obj2ID
		}
		else { // if(vect > 0) { // obj1ID �������� �� ������ ���� obj2ID
			hr2 = Helper_GetHierarchy(obj2ID, matfTryHouse, &hse2_rec, list2, &zip2);
			if(hr2 > 0 && IsObjInHierarchy(obj1ID, list2))
				ok = 2; // obj2ID belong to obj1ID
		}
	}
    return ok;
}

int PPFiasReference::IdentifyHouse(PPID terminalObjID, const char * pHouseCode, PPID * pHouseID)
{
	int    ok = -1;
	PPID   hse_id = 0;
	SString temp_buf;
	FiasHouseObjTbl & r_t = FT.HseT;
	TSArray <FiasHouseObjTbl::Rec> hse_list;
    FiasHouseObjTbl::Key1 k1;
    k1.ParentUuRef = terminalObjID;
    BExtQuery q(&r_t, 1);
    q.selectAll().where(r_t.ParentUuRef == terminalObjID);
    for(q.initIteration(false, &k1, spEq); q.nextIteration() > 0;) {
        hse_list.insert(&r_t.data);
    }
    for(uint i = 0; ok < 0 && i < hse_list.getCount(); i++) {
		const FiasHouseObjTbl::Rec & r_item = hse_list.at(i);
		if(GetText(r_item.NumTRef, temp_buf.Z()) > 0) {
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			if(temp_buf == pHouseCode) {
				hse_id = r_item.IdUuRef;
				ok = 1;
			}
		}
    }
    ASSIGN_PTR(pHouseID, hse_id);
	return ok;
}

int PPFiasReference::GetRandomHouse(long extValue, PPID terminalObjID, PPID * pHouseID)
{
	int    ok = -1;
	PPID   hse_id = 0;
	SString temp_buf;
	FiasHouseObjTbl & r_t = FT.HseT;
	TSArray <FiasHouseObjTbl::Rec> hse_list;
    FiasHouseObjTbl::Key1 k1;
    k1.ParentUuRef = terminalObjID;
    BExtQuery q(&r_t, 1);
    q.selectAll().where(r_t.ParentUuRef == terminalObjID);
    for(q.initIteration(false, &k1, spEq); q.nextIteration() > 0;) {
        hse_list.insert(&r_t.data);
    }
    const uint _hc = hse_list.getCount();
	if(_hc) {
		ulong _pos = (extValue > 0) ? (ulong)(extValue % _hc) : (SLS.GetTLA().Rg.Get() % _hc);
        hse_id = hse_list.at(_pos).IdUuRef;
        ok = 1;
	}
    ASSIGN_PTR(pHouseID, hse_id);
    return ok;
}

int PPFiasReference::GetRandomAddress(long extValue, PPID cityID, PPID * pStreetID, PPID * pHouseID)
{
	int    ok = -1;
	PPID   street_id = DEREFPTRORZ(pStreetID);
	PPID   house_id = 0;
    if(cityID) {
        if(!street_id) {
			PPIDArray child_list;
            THROW(FT.GetChildList(cityID, 7, child_list));
            const uint _sc = child_list.getCount();
            if(_sc) {
            	SlThreadLocalArea & r_stla = SLS.GetTLA();
				for(uint t = 0; ok < 0 && t < MIN(20, _sc); t++) {
					ulong _pos = (extValue > 0) ? static_cast<ulong>(extValue % _sc) : (r_stla.Rg.Get() % _sc);
					if(GetRandomHouse(extValue, child_list.get(_pos), &house_id) > 0) {
						ok = 1;
					}
				}
            }
        }
        else {
			if(GetRandomHouse(extValue, street_id, &house_id) > 0) {
				ok = 1;
			}
        }
    }
    CATCHZOK
    ASSIGN_PTR(pStreetID, street_id);
    ASSIGN_PTR(pHouseID, house_id);
    return ok;
}

int PPFiasReference::MakeAddressText(PPID terminalID, long flags, SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	SString name, sn;
	FiasHouseObjTbl::Rec hse_rec;
	TSArray <FiasAddrObjTbl::Rec> adr_list;
	long   zip = 0;
	int    hr = Helper_GetHierarchy(terminalID, flags, &hse_rec, adr_list, &zip);
	if(hr > 0) {
		if(flags & matfZipPrefix) {
            rBuf.CatLongZ(zip, 6).Space();
		}
		uint   p = adr_list.getCount();
		if(p) do {
			const FiasAddrObjTbl::Rec & r_rec = adr_list.at(--p);
			GetText(r_rec.NameTRef, name);
			GetText(r_rec.SnTRef, sn);
			if(sn.Len())
				rBuf.Cat(sn).CatChar('[').Cat(r_rec.LevelStatus).CatChar(']').Space();
			if(name.Len())
				rBuf.Cat(name);
			rBuf.Space();
			ok = 1;
		} while(p);
		if(hr == 2) {
			GetText(hse_rec.NumTRef, name = 0);
			if(name.NotEmptyS())
				rBuf.Cat(name);
			ok = 2;
		}
		rBuf.Strip();
	}
	return ok;
}

int PPFiasReference::SearchObjByTextRefList(const TSVector <TextRefIdent> & rTRefList, PPIDArray & rList)
{
	int    ok = -1;
	const int use_bextq = 1;
	for(uint i = 0; i < rTRefList.getCount(); i++) {
		const TextRefIdent & r_i = rTRefList.at(i);
		FiasAddrObjTbl & r_t = FT.AdrT;
		FiasAddrObjTbl::Key3 k3;
		MEMSZERO(k3);
		k3.NameTRef = r_i.O.Id;
		if(use_bextq) {
			BExtQuery q(&r_t, 3);
			q.select(r_t.NameTRef, r_t.IdUuRef, 0).where(r_t.NameTRef == r_i.O.Id);
			for(q.initIteration(false, &k3, spGe); q.nextIteration() > 0;) {
				FiasAddrObjTbl::Rec rec;
				r_t.CopyBufTo(&rec);
				rList.add(rec.IdUuRef);
				ok = 1;
			}
		}
		else {
			if(r_t.search(3, &k3, spGe) && r_t.data.NameTRef == r_i.O.Id) do {
				FiasAddrObjTbl::Rec rec;
				r_t.CopyBufTo(&rec);
				rList.add(rec.IdUuRef);
				ok = 1;
			} while(r_t.search(3, &k3, spNext) && r_t.data.NameTRef == r_i.O.Id);
		}
	}
	rList.sortAndUndup();
	return ok;
}

int PPFiasReference::SearchObjByText(const char * pText, long flags, PPID upperID, PPIDArray & rList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    do_post_process = 0;
    SStringU pattern;
    SString temp_buf;
    SString pattern_mb(pText);
    if(!(flags & stfAnsiInput))
		pattern_mb.Transf(CTRANSF_INNER_TO_OUTER);
    pattern_mb.ToLower1251();
    pattern.CopyFromUtf8((temp_buf = pattern_mb).ToUtf8());
	TSVector <TextRefIdent> text_ref_list;
	int   sr = 0;
	if(flags & stfPrefix)
		sr = p_ref->TrT.SearchSelfRefTextByPrefix(pattern, &text_ref_list);
	else {
		if(!(flags & stfDontUseCache)) {
			FiasAddrCache * p_cache = GetDbLocalCachePtr <FiasAddrCache> (PPOBJ_FIAS);
			if(p_cache) {
				PPIDArray temp_list;
				p_cache->SetTable(this);
				ok = p_cache->GetAddrObjListByText(pattern_mb, temp_list);
				if(ok > 0 && temp_list.getCount()) {
					rList.add(&temp_list);
					do_post_process = 1;
				}
				p_cache->SetTable(0);
			}
		}
		else {
			PPID   _id = 0;
			sr = p_ref->TrT.SearchSelfRefText(pattern, &_id);
			if(sr > 0) {
				TextRefIdent tri(PPOBJ_SELFREFTEXT, _id, PPTRPROP_DEFAULT);
				text_ref_list.insert(&tri);
			}
		}
	}
	if(sr > 0) {
		SearchObjByTextRefList(text_ref_list, rList);
		do_post_process = 1;
	}
	if(do_post_process) {
		if(upperID) {
			PPIDArray result_list;
			uint _p = rList.getCount();
			for(uint i = 0; i < _p; i++) {
				const  PPID _id = rList.get(i);
				if(Match(_id, upperID, -1) == 1) // 1 - obj1ID belong to obj2ID
					result_list.add(_id);
			}
			rList = result_list;
			if(!rList.getCount())
				ok = -1;
		}
	}
	return ok;
}

class TestFiasProcessor {
public:
	TestFiasProcessor()
	{
	}
	~TestFiasProcessor()
	{
	}
	int Run()
	{
		int    ok = 1;
        TestDialog * dlg = new TestDialog(this);
        if(CheckDialogPtrErr(&dlg)) {
			FiasAddressCtrlGroup::Rec fr;
            dlg->setGroupData(1, &fr);
			ExecViewAndDestroy(dlg);
        }
        else
			ok = 0;
        return ok;
	}
private:
    class TestDialog : public TDialog {
	public:
		explicit TestDialog(TestFiasProcessor * pPrcssr) : TDialog(DLG_TESTFIAS), P(pPrcssr)
		{
			addGroup(1,  new FiasAddressCtrlGroup(CTL_TESTFIAS_INPUT, CTL_TESTFIAS_INFO));
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
#if 0 // {
			if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_TESTFIAS_INPUT)) {
				SString temp_buf;
				getCtrlString(CTL_TESTFIAS_INPUT, temp_buf);
				if(temp_buf != Input && temp_buf.Len() >= 4) {
					PPIDArray list;
                    int r = P->F.SearchObjByText(temp_buf, PPFiasReference::stfPrefix, 0, list);
					if(r > 0) {
                        Info.Z().Cat(list.getCount()).Space().Cat("objects");
                    }
                    else if(r < 0)
						Info.Z().Cat("Nothing found");
					else
						Info.Z().Cat("Error");
					setCtrlString(CTL_TESTFIAS_INFO, Info);
				}
				Input = temp_buf;
				clearEvent(event);
			}
#endif // } 0
		}
		TestFiasProcessor * P;
	    SString Input;
	    SString Info;
    };

	PPFiasReference F;
};

int TestFias()
{
	TestFiasProcessor prcssr;
	return prcssr.Run();
}
