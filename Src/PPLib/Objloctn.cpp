// OBJLOCTN.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
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
	SLAPI  FiasAddrCache();
	SLAPI ~FiasAddrCache();
	int    SLAPI GetAddrObjListByText(const char * pText, PPIDArray & rList);
	void   FASTCALL SetTable(PPFiasReference * pT);
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long extraData);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

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

SLAPI FiasAddrCache::FiasAddrCache() : ObjCacheHash(PPOBJ_FIAS, sizeof(Data), 16*1024*1024, 8), P_T(0)
{
}

SLAPI FiasAddrCache::~FiasAddrCache()
{
}

void FASTCALL FiasAddrCache::SetTable(PPFiasReference * pT)
{
	P_T = pT;
}

int SLAPI FiasAddrCache::GetAddrObjListByText(const char * pText, PPIDArray & rList)
{
	int    ok = -1;
	rList.clear();
	if(P_T) {
		PPID   text_ref_id = 0;
		uint   pos = 0;
		if(PPRef->TrT.FetchSelfRefText(pText, &text_ref_id) > 0) {
			//TxlLock.ReadLock();
			SRWLOCKER(TxlLock, SReadWriteLocker::Read);
			if(TextList.lsearch(&text_ref_id, &(pos = 0), CMPF_LONG)) {
				rList = TextList.at(pos)->AddrList;
				ok = 1;
			}
			else {
				//TxlLock.Unlock();
				//TxlLock.WriteLock();
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				if(TextList.lsearch(&text_ref_id, &(pos = 0), CMPF_LONG)) { // Повторная попытка после блокировки
					rList = TextList.at(pos)->AddrList;
					ok = 1;
				}
				else {
					TextRefIdent tri(PPOBJ_SELFREFTEXT, text_ref_id, PPTRPROP_DEFAULT);
					TSVector <TextRefIdent> text_ref_list; // @v9.8.4 TSArray-->TSVector
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
			//TxlLock.Unlock();
		}
	}
	return ok;
}

int SLAPI FiasAddrCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
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

void SLAPI FiasAddrCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	FiasAddrObjTbl::Rec * p_data_rec = (FiasAddrObjTbl::Rec *)pDataRec;
	if(p_data_rec) {
		const Data * p_cache_rec = (const Data *)pEntry;
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
PPCountryBlock & PPCountryBlock::Clear()
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
SLAPI PPLocationPacket::PPLocationPacket()
{
	memzero((LocationTbl::Rec *)this, sizeof(LocationTbl::Rec));
	TagL.ObjType = PPOBJ_LOCATION;
	//MEMSZERO(Rec);
}

SLAPI PPLocationPacket::PPLocationPacket(const PPLocationPacket & rS)
{
	Copy(rS);
}

int FASTCALL PPLocationPacket::Copy(const PPLocationPacket & rS)
{
	*(LocationTbl::Rec *)this = *(LocationTbl::Rec *)&rS;
	Regs = rS.Regs;
	TagL = rS.TagL;
	return 1;
}

void SLAPI PPLocationPacket::destroy()
{
	memzero((LocationTbl::Rec *)this, sizeof(LocationTbl::Rec));
	//MEMSZERO(Rec);
	Regs.freeAll();
	TagL.Destroy();
	TagL.ObjType = PPOBJ_LOCATION;
}

PPLocationPacket & FASTCALL PPLocationPacket::operator = (const LocationTbl::Rec & rS)
{
	*((LocationTbl::Rec *)this) = rS;
	return *this;
}

PPLocationPacket & FASTCALL PPLocationPacket::operator = (const PPLocationPacket & rS)
{
	Copy(rS);
	return *this;
}

int SLAPI PPLocationPacket::IsEmptyAddress() const
{
	return LocationCore::IsEmptyAddressRec(*this);
}
//
//
//
struct Storage_PPLocationConfig {  // @persistent @store(PropertyTbl)
	PPID   Tag;            // Const=PPOBJ_CONFIG
	PPID   ID;             // Const=PPCFG_MAIN
	PPID   Prop;           // Const=PPPRP_LOCATIONCFG
	int16  WhZoneCoding;
	int16  WhColCoding;
	int16  WhRowCoding;
	int16  WhCellCoding;
	int16  WhCodingDiv;    // Разделитель кодов складских позиций
	int16  Reserve;        // @alignment
	long   Flags;
	PPID   DefPalletID;    // Тип паллета по умолчанию
	char   AddrCodeTempl[32]; // @v7.3.8
	PPID   StoreIdxTagID;  // @v8.9.11
	char   Reserve2[8];    // @v8.9.11 [12]-->[8]
	long   Reserve3[2];
};

//static
int FASTCALL PPObjLocation::ReadConfig(PPLocationConfig * pCfg)
{
	const  long prop_cfg_id = PPPRP_LOCATIONCFG;
	int    ok = -1, r;
	Storage_PPLocationConfig cfg;
	MEMSZERO(cfg);
	memzero(pCfg, sizeof(*pCfg));
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
		STRNSCPY(pCfg->AddrCodeTempl, cfg.AddrCodeTempl);
		ok = 1;
	}
	else {
		ok = -1;
	}
	CATCHZOK
	return ok;
}

// static
int FASTCALL PPObjLocation::WriteConfig(const PPLocationConfig * pCfg, int use_ta)
{
	const  long prop_cfg_id = PPPRP_LOCATIONCFG;
	const  long cfg_obj_type = PPCFGOBJ_LOCATION;
	int    ok = -1, r;
	int    is_new = 0;
	Storage_PPLocationConfig cfg;
	MEMSZERO(cfg);
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
		STRNSCPY(cfg.AddrCodeTempl, pCfg->AddrCodeTempl);
		THROW(PPObject::Helper_PutConfig(prop_cfg_id, cfg_obj_type, is_new, &cfg, sizeof(cfg), 0));
		THROW(tra.Commit());
	}
	DirtyConfig();
	CATCHZOK
	return ok;
}

//static
int SLAPI PPObjLocation::EditConfig()
{
	int    ok = -1;
	PPLocationConfig cfg, org_cfg;
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
		ObjTagFilt tag_filt;
		tag_filt.ObjTypeID = PPOBJ_LOCATION;
		SetupObjTagCombo(dlg, CTLSEL_LOCCFG_STRIDXTAG, cfg.StoreIdxTagID, OLW_CANINSERT, &tag_filt);
	}
	dlg->AddClusterAssoc(CTL_LOCCFG_FLAGS, 0, PPLocationConfig::fUseFias);
	dlg->SetClusterData(CTL_LOCCFG_FLAGS, cfg.Flags);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTLSEL_LOCCFG_WHZONECOD, &cfg.WhZoneCoding);
		dlg->getCtrlData(CTLSEL_LOCCFG_WHCOLCOD,  &cfg.WhColCoding);
		dlg->getCtrlData(CTLSEL_LOCCFG_WHCELLCOD, &cfg.WhCellCoding);
		dlg->GetClusterData(CTL_LOCCFG_DIVCOD, &cfg.WhCodingDiv);
		dlg->getCtrlData(CTLSEL_LOCCFG_DEFPALTYPE, &cfg.DefPalletID);
		dlg->getCtrlData(CTL_LOCCFG_ADDRCODETEMPL, cfg.AddrCodeTempl);
		dlg->getCtrlData(CTLSEL_LOCCFG_STRIDXTAG, &cfg.StoreIdxTagID);
		dlg->GetClusterData(CTL_LOCCFG_FLAGS, &cfg.Flags);
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
int FASTCALL SetupLocationCombo(TDialog * dlg, uint ctl, PPID id, uint flags, PPID locType, PPID owner)
{
	LocationFilt flt(locType, owner);
	return SetupPPObjCombo(dlg, ctl, PPOBJ_LOCATION, id, flags, &flt);
}

int FASTCALL SetupLocationCombo(TDialog * dlg, uint ctl, PPID id, uint flags, const LocationFilt * pFilt)
{
	LocationFilt flt; //(locType, owner);
	if(!RVALUEPTR(flt, pFilt))
		flt.LocType = LOCTYP_WAREHOUSE;
	return SetupPPObjCombo(dlg, ctl, PPOBJ_LOCATION, id, flags, &flt);
}

// static
int SLAPI PPObjLocation::SelectWarehouse(PPID /*owner*/, PPID /*level*/)
{
	PPID   id = LConfig.Location;
	int    r = PPSelectObject(PPOBJ_LOCATION, &id, PPTXT_SELECTLOCATION, 0);
	if(r > 0 && !DS.SetLocation(id))
		r = PPErrorZ();
	return r;
}

void SLAPI PPObjLocation::InitInstance(SCtrLite sctr, void * extraPtr)
{
	Sctr = sctr;

	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	P_CurrFilt = 0;
	P_WObj = 0;
	P_RegObj = 0;
	ExtraPtr = extraPtr;
	if(ExtraPtr)
		P_CurrFilt = new LocationFilt(*(LocationFilt*)ExtraPtr);
	IsCityCacheInited = 0;
	if(Sctr != SConstructorLite)
		P_RegObj = new PPObjRegister;
}

SLAPI PPObjLocation::PPObjLocation(void * extraPtr) : PPObject(PPOBJ_LOCATION)
{
	InitInstance(SConstructorDef, extraPtr);
}

SLAPI PPObjLocation::PPObjLocation(SCtrLite sctr) : PPObject(PPOBJ_LOCATION)
{
	InitInstance(sctr, 0);
}

SLAPI PPObjLocation::~PPObjLocation()
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
						temp_buf.CatChar('.');
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
			if(rBuf.Empty()) {
				ideqvalstr(rec.ID, rBuf);
			}
		}
	}
	return rBuf;
}

int SLAPI PPObjLocation::Search(PPID id, void * b)
{
	return P_Tbl->Search(id, (LocationTbl::Rec *)b);
}

int SLAPI PPObjLocation::GetParentWarehouse(PPID locID, PPID * pWarehouseID)
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

int SLAPI PPObjLocation::ReqAutoName(PPID id)
{
	int    ok = 0;
	LocationTbl::Rec rec;
	PPID   temp_id = id;
	PPIDArray cycle_list;
	while(!ok && temp_id && Fetch(temp_id, &rec) > 0) {
		if(cycle_list.addUnique(temp_id) < 0) {
			// Зафиксирована циклическая ссылка
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

int SLAPI PPObjLocation::GetAddress(PPID locID, uint flags, SString & rBuf)
{
	rBuf.Z();
	LocationTbl::Rec loc_rec;
	return (!locID || Fetch(locID, &loc_rec) > 0) ? LocationCore::GetAddress(loc_rec, flags, rBuf) : 0;
}

int SLAPI PPObjLocation::GetCountry(const LocationTbl::Rec * pLocRec, PPID * pCountryID, PPCountryBlock * pBlk)
{
	int    ok = -1;
	CALLPTRMEMB(pBlk, Clear());
	ASSIGN_PTR(pCountryID, 0);
	if(pLocRec && pLocRec->CityID) {
		WorldTbl::Rec w_rec;
		THROW_MEM(SETIFZ(P_WObj, new PPObjWorld));
		if(P_WObj->GetCountryByChild(pLocRec->CityID, &w_rec) > 0) {
			if(pBlk) {
				pBlk->Name = w_rec.Name;
				pBlk->Abbr = w_rec.Abbr;
				pBlk->Code = w_rec.Code;
				// @v9.7.8 {
				pBlk->IsNative = 1;
				if(pBlk->Code.NotEmpty() && DS.GetConstTLA().MainOrgCountryCode != pBlk->Code)
					pBlk->IsNative = 0;
				// } @v9.7.8
			}
			ASSIGN_PTR(pCountryID, w_rec.ID);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::GetCountry(PPID id, PPID * pCountryID, PPCountryBlock * pBlk)
{
	LocationTbl::Rec loc_rec;
	const LocationTbl::Rec * p_loc_rec = 0;
	if(id && Search(id, &loc_rec) > 0)
		p_loc_rec = &loc_rec;
	return GetCountry(p_loc_rec, pCountryID, pBlk);
}

int SLAPI PPObjLocation::InitCityCache()
{
	int    ok = 1;
	IsCityCacheInited = 0;
	CityCache.freeAll();
	BExtQuery q(P_Tbl, 0, 128);
	q.select(P_Tbl->ID, P_Tbl->CityID, 0L).where(P_Tbl->CityID > 0L);
	LocationTbl::Key0 k0;
	MEMSZERO(k0);
	for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;)
		THROW_SL(CityCache.Add(P_Tbl->data.ID, P_Tbl->data.CityID, 0));
	CityCache.Sort();
	IsCityCacheInited = 1;
	CATCH
		ok = 0;
		CityCache.freeAll();
	ENDCATCH
	return ok;
}

int SLAPI PPObjLocation::GetCityID(PPID locID, PPID * pCityID, int useCache)
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

int SLAPI PPObjLocation::GetCity(PPID id, PPID * pCityID, SString * pCityName, int useCache)
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

int SLAPI PPObjLocation::GetCityByName(const char * pName, PPID * pCityID)
{
	int    ok = -1;
	ASSIGN_PTR(pCityID, 0);
	if(pName) {
		SETIFZ(P_WObj, new PPObjWorld);
		SArray w_list(sizeof(WorldTbl::Rec));
		P_WObj->GetListByName(WORLDOBJ_CITY, pName, &w_list);
		if(w_list.getCount() == 1) {
			ASSIGN_PTR(pCityID, ((WorldTbl::Rec *)w_list.at(0))->ID);
			ok = 1;
		}
		else if(w_list.getCount()) {
			ASSIGN_PTR(pCityID, ((WorldTbl::Rec *)w_list.at(0))->ID);
			ok = 2;
		}
	}
	return ok;
}

int SLAPI PPObjLocation::FetchCityByAddr(PPID locID, WorldTbl::Rec * pRec)
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

int SLAPI PPObjLocation::FetchCity(PPID cityID, WorldTbl::Rec * pRec)
{
	SETIFZ(P_WObj, new PPObjWorld);
	return P_WObj ? P_WObj->Fetch(cityID, pRec) : PPSetErrorNoMem();
}

const char * SLAPI PPObjLocation::GetNamePtr()
{
	const LocationTbl::Rec & r_rec = P_Tbl->data;
	if(r_rec.Type == LOCTYP_ADDRESS)
		LocationCore::GetAddress(r_rec, 0, NameBuf);
	else if(r_rec.Type == LOCTYP_WAREPLACE) {
		LocationTbl::Rec par_rec;
		NameBuf.Z();
		if(r_rec.ParentID && Fetch(r_rec.ParentID, &par_rec) > 0)
			NameBuf.Cat(par_rec.Name).CatChar('/');
		NameBuf.Cat(r_rec.Name);
	}
	else
		NameBuf = r_rec.Name;
	return NameBuf.cptr();
}

int SLAPI PPObjLocation::DeleteObj(PPID id)
{
	return PutRecord(&id, 0, 0);
}

int SLAPI PPObjLocation::Validate(LocationTbl::Rec * pRec, int /*chkRefs*/)
{
	int    ok = 1;
	if(pRec) {
		PPID   id = 0;
		long   t = pRec->Type;
		LocTypeDescr ltd;
		THROW(P_Tbl->GetTypeDescription(t, &ltd));
		if(pRec->Name[0] == 0) {
			THROW_PP(t == LOCTYP_WHCELLAUTOGEN ||
				(t == LOCTYP_ADDRESS && (pRec->CityID || LocationCore::IsEmptyAddressRec(*pRec))), PPERR_NAMENEEDED);
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
					const PPID cid = bycode_list.get(i);
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

int PPObjLocation::AddListItem(StrAssocArray * pList, LocationTbl::Rec * pLocRec, long zeroParentId, PPIDArray * pRecurTrace)
{
	int    ok = 1, r;
	PPIDArray local_recur_trace;
	if(pList->Search(pLocRec->ID))
		ok = -1;
	else {
		PPID   par_id = pLocRec->ParentID;
		LocationTbl::Rec par_rec;
		if(par_id && Fetch(par_id, &par_rec) > 0) {
			SETIFZ(pRecurTrace, &local_recur_trace);
			THROW(r = pRecurTrace->addUnique(par_id));
			//THROW_PP_S(r > 0, PPERR_LOCATIONRECUR, par_rec.Name);
			if(r > 0) {
				THROW(AddListItem(pList, &par_rec, zeroParentId, pRecurTrace)); // @recursion
			}
			else {
				PPSetError(PPERR_LOCATIONRECUR, par_rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
				par_id = 0;
			}
		}
		else
			par_id = zeroParentId;
		THROW_SL(pList->AddFast(pLocRec->ID, par_id, pLocRec->Name));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::MakeListByType(PPID locType, PPID parentID, long zeroParentId, int flags, StrAssocArray * pList)
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

StrAssocArray * PPObjLocation::MakeList_(const LocationFilt * pLocFilt, long zeroParentId)
{
	StrAssocArray * p_list = new StrAssocArray;
	LocationTbl::Rec loc_rec;
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
	const PPID parent_id = (p_filt->Parent || !p_filt->Owner) ? p_filt->Parent : p_filt->Owner;
	if(oneof2(p_filt->LocType, LOCTYP_WAREHOUSE, LOCTYP_WAREHOUSEGROUP) || (p_filt->LocType == LOCTYP_WHZONE && !parent_id))
		f |= LocationCore::eoIgnoreParent;
	else if(p_filt->Owner)
		f |= LocationCore::eoParentAsOwner;
	if(p_filt->LocType == LOCTYP_ADDRESS && p_filt->GetExField(LocationFilt::exfPhone, temp_buf) > 0 && temp_buf.NotEmptyS()) {
		uint i;
		SString phone_buf;
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
		PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf);
		LongArray _pl;
		P_Tbl->SearchPhoneIndex(phone_buf, 0, _pl);
		for(i = 0; i < _pl.getCount(); i++) {
			EAddrTbl::Rec ea_rec;
			LocationTbl::Rec loc_rec;
			if(P_Tbl->GetEAddr(_pl.get(i), &ea_rec) > 0) {
				if(ea_rec.LinkObjType == PPOBJ_LOCATION && Search(ea_rec.LinkObjID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
					THROW_SL(p_list->Add(loc_rec.ID, 0, loc_rec.Name, 1));
				}
			}
		}
	}
	else {
		THROW(MakeListByType(p_filt->LocType, parent_id, zeroParentId, f, p_list));
		if(oneof2(p_filt->LocType, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
			THROW(MakeListByType(LOCTYP_WHZONE, parent_id, zeroParentId, f, p_list));
		}
		else if(oneof2(p_filt->LocType, LOCTYP_WAREHOUSE, LOCTYP_WAREHOUSEGROUP)) {
			//
			// Добавляем все группы складов (предыдущий цикл мог пропустить
			// те группы, которые не имеют дочерних элементов.
			//
			THROW(MakeListByType(LOCTYP_WAREHOUSEGROUP, parent_id, zeroParentId, f, p_list));
			{
				const uint elc = p_filt->ExtLocList.GetCount();
				if(elc) {
					for(uint i = 0; i < elc; i++) {
						const PPID ext_loc_id = p_filt->ExtLocList.Get(i);
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
								p_list->Add(ext_loc_id, 0, temp_buf);
							}
						}
					}
				}
			}
		}
	}
	if(zeroParentId) {
		MEMSZERO(loc_rec);
		loc_rec.ID = zeroParentId;
		PPLoadText(PPTXT_ALLWAREHOUSES, temp_buf.Z());
		temp_buf.CopyTo(loc_rec.Name, sizeof(loc_rec.Name));
		PPIDArray recur_trace;
		THROW(AddListItem(p_list, &loc_rec, 0, &recur_trace));
	}
	p_list->SortByText();
	// @debug {
#if 0 // {
	{
		SString line_buf;
		for(uint i = 0; i < p_list->getCount(); i++) {
			line_buf.Z();
			StrAssocArray::Item item = p_list->at(i);
			if(item.ParentId) {
				uint pos = 0;
				if(p_list->Search(item.ParentId, &pos) > 0) {
					StrAssocArray::Item par_item = p_list->at(pos);
					line_buf.Cat(par_item.Txt).Semicol();
				}
			}
			line_buf.Cat(item.Txt).Semicol().Cat(item.Id).Semicol().Cat(item.ParentId);
			PPLogMessage(PPFILNAM_DEBUG_LOG, line_buf, 0);
		}
	}
#endif // } 0
	// } @debug
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

StrAssocArray * SLAPI PPObjLocation::MakeStrAssocList(void * extraPtr)
{
	return MakeList_(0, 0);
}

int SLAPI PPObjLocation::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->valid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = (StdTreeListBoxDef*)pDef;
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			for(uint i = 0; i < list.getCount(); i++) {
				PPID   id = list.at(i);
				long   img_id = ICON_WH;
				LocationTbl::Rec rec;
				if(Fetch(id, &rec) > 0) {
					if(rec.Type == LOCTYP_WAREHOUSE)
						img_id = ICON_WH;
					else if(rec.Type == LOCTYP_WAREHOUSEGROUP)
						img_id = ICON_FOLDERGRP;
					else if(rec.Type == LOCTYP_WHZONE)
						img_id = ICON_WHZONE;
					else if(rec.Type == LOCTYP_WHCOLUMN)
						img_id = ICON_WHCOLUMN;
					else if(rec.Type == LOCTYP_WHCELL)
						img_id = ICON_WHCELL;
					else if(rec.Type == LOCTYP_DIVISION)
						img_id = ICON_DIVISION_16;
				}
				p_def->AddImageAssoc(id, img_id);
			}
		}
	}
	return 1;
}

// virtual
ListBoxDef * SLAPI PPObjLocation::Selector(void * extraPtr)
{
	ListBoxDef * p_def = PPObject::Selector(extraPtr);
	AssignImages(p_def);
	return p_def;
}

// virtual
int SLAPI PPObjLocation::UpdateSelector(ListBoxDef * pDef, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, extraPtr);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}

int SLAPI PPObjLocation::GenerateWhCells(PPID whColumnID, const LocationTbl::Rec * pSampleRec, int use_ta)
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
				MEMSZERO(cell_rec);
				cell_rec.Type = LOCTYP_WHCELL;
				cell_rec.ParentID = whColumnID;
				cell_rec.NumRows = (int16)row;
				cell_rec.NumLayers = (int16)layer;
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
int SLAPI PPObjLocation::ResolveWarehouseByCode(const char * pCode, PPID accSheetID, PPID * pArID)
{
	int    ok = -1;
	PPID   ar_id = 0;
	SString code = pCode;
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

int SLAPI PPObjLocation::GetListByRegNumber(PPID regTypeID, PPID locTyp, const char * pSerial, const char * pNumber, PPIDArray & rList)
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
	if(r == 0)
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
			const PPID loc_id = rList.get(--c);
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

int SLAPI PPObjLocation::ResolveGLN(PPID locTyp, const char * pGLN, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	SString code = pGLN;
	assert(pGLN);
	THROW_INVARG(pGLN);
	if(code.NotEmptyS()) {
		const PPID reg_type_id = PPREGT_GLN;
		THROW(GetListByRegNumber(reg_type_id, locTyp, 0, code, rList));
		ok = rList.getCount() ? ((rList.getCount() > 1) ? 2 : 1) : -1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::ResolveWarehouse(PPID locID, PPIDArray & rDestList, PPIDArray * pRecurTrace)
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

int SLAPI PPObjLocation::ResolveWarehouseList(const PPIDArray * pList, PPIDArray & rDestList)
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

int SLAPI PPObjLocation::ResolveWhCellList(const PPIDArray * pList, long options, PPIDArray & rDestList)
{
	int    ok = 1;
	PPIDArray temp_list, wh_list;
	if(!pList) {
		GetWarehouseList(&wh_list);
		pList = &wh_list;
	}
	if(pList) {
		const int use_cache = 1;
		for(uint i = 0; i < pList->getCount(); i++)
			THROW(ResolveWhCell(pList->get(i), temp_list, 0, use_cache));
	}
	else {
		ok = -1;
	}
	CATCHZOK
	rDestList = temp_list;
	return ok;
}

int SLAPI PPObjLocation::IsMemberOfGroup(PPID locID, PPID grpID)
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

int SLAPI PPObjLocation::BelongTo(PPID locID, PPID parentID, SString * pPathText)
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
						pPathText->CatChar('/');
					pPathText->Cat(loc_rec.Name);
				}
				if(loc_rec.ParentID == parentID)
					yes = 1;
				else {
					locID = loc_rec.ParentID;
					if(recur_trace.lsearch(locID))
						break; // Мы зациклились
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

int SLAPI PPObjLocation::SearchName(PPID locTyp, PPID parentID, const char * pName, PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	SString name_buf = pName;
	if(name_buf.NotEmptyS()) {
		LocationTbl::Key2 k2;
		MEMSZERO(k2);
		k2.Type = (int16)locTyp;
		BExtQuery q(P_Tbl, 2);
		q.select(P_Tbl->ID, P_Tbl->ParentID, P_Tbl->Name, 0L).where(P_Tbl->Type == locTyp);
		for(q.initIteration(0, &k2, spGe); ok < 0 && q.nextIteration() > 0;) {
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
			const LocationFilt * p_filt = (const LocationFilt *)extraPtr;
			if(p_filt->LocType == LOCTYP_WAREPLACE) {
				enableCommand(cmWareplaceList, 0);
				PPLoadText(PPTXT_TITLE_WAREPLACEVIEW, fmt_buf);
				GetLocationName(p_filt->Parent, par_name);
				setTitle(title_.Printf(fmt_buf, par_name.cptr()));
				IsWareplaceView = 1;
				ParentID = p_filt->Parent;
			}
			else if(p_filt->LocType == LOCTYP_WAREHOUSE) {
				// @v9.2.5 PPLoadText(PPTXT_TITLE_WAREHOUSEVIEW, title_);
				PPLoadString("warehouse_pl", title_); // @v9.2.5
				setTitle(title_);
			}
		}
		else {
			// @v9.2.5 PPLoadText(PPTXT_TITLE_WAREHOUSEVIEW, title_);
			PPLoadString("warehouse_pl", title_); // @v9.2.5
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
				PPObjLocation & r_obj = *(PPObjLocation *)P_Obj;
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
			// Здесь необходимо создать отдельный от P_Obj экземпляр PPObjLocation
			// из-за того, что списком используется фильтр, заданный именно в
			// экземпляре объекта, а не переданный доп параметром классу LocationView.
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
		loc_obj.GetWarehouseList(&wh_list);
		PPObjIDArray objid_ary;
		PPWait(1);
		THROW(objid_ary.Add(PPOBJ_LOCATION, wh_list));
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
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
	MEMSZERO(rec);
	const  PPID loc_acs_id = LConfig.LocAccSheetID;
	while(ar_obj.P_Tbl->EnumBySheet(loc_acs_id, &art_no, &rec) > 0) {
		int    r = loc_obj.Search(rec.ObjID, 0);
		if(r < 0) {
			LocationTbl::Rec loc_rec;
			MEMSZERO(loc_rec);
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
		if(r == 0) {
			logger.LogLastError();
			ok = 0;
		}
	}
	updateList(-1);
	return ok;
}

int SLAPI PPObjLocation::Browse(void * extraPtr)
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
public:
	LocationExtFieldsDialog() : PPListDialog(DLG_DLVREXTFLDS, CTL_LBXSEL_LIST)
	{
		if(P_Box)
			CALLPTRMEMB(P_Box->def, SetOption(lbtFocNotify, 1));
		PPPersonConfig psn_cfg;
		PPObjPerson::ReadConfig(&psn_cfg);
		FieldNames.copy(psn_cfg.DlvrAddrExtFldList);
		showCtrl(STDCTL_INSBUTTON, 0);
	}
	int    setDTS(const LocationTbl::Rec *);
	int    getDTS(LocationTbl::Rec *);
private:
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();
	int    Edit(TaggedString *);

	TaggedStringArray FieldNames;
	StrAssocArray     Fields;
	LocationTbl::Rec  Data;
};

static int SLAPI SetupTaggedStringCombo(TDialog * dlg, uint ctlID, const TaggedStringArray * pStrings, long initID, uint /*flags*/)
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = (ComboBox*)dlg->getCtrlView(ctlID)) != 0) {
		uint   options = (lbtDisposeData|lbtDblClkNotify);
		uint   idx = 0;
		TaggedString * p_s;
		THROW(p_lw = CreateListWindow(TaggedString::BufSize(), options));
		for(idx = 0; pStrings->enumItems(&idx, (void **)&p_s);)
			p_lw->listBox()->addItem(p_s->Id, p_s->Txt);
		p_cb->setListWindow(p_lw, initID);
	}
	CATCHZOK
	return ok;
}

int LocationExtFieldsDialog::Edit(TaggedString * pData)
{
	class AddExtFldDialog : public TDialog {
	public:
		AddExtFldDialog(TaggedStringArray * pFieldNames) : TDialog(DLG_ADDEXTFLD)
		{
			if(pFieldNames)
				FieldNames.copy(*pFieldNames);
			disableCtrl(CTLSEL_ADDEXTFLD_FLD, 1);
		}
		int    setDTS(const TaggedString * pData)
		{
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			SetupTaggedStringCombo(this, CTLSEL_ADDEXTFLD_FLD, &FieldNames, Data.Id, 0);
			setCtrlData(CTL_ADDEXTFLD_VAL, Data.Txt);
			return 1;
		}
		int    getDTS(TaggedString * pData)
		{
			int    ok = 1;
			getCtrlData(CTLSEL_ADDEXTFLD_FLD, &Data.Id);
			getCtrlData(CTL_ADDEXTFLD_VAL,     Data.Txt);
			THROW_PP(Data.Id, PPERR_USERINPUT);
			ASSIGN_PTR(pData, Data);
			CATCHZOK
			return ok;
		}
	private:
		TaggedStringArray  FieldNames;
		TaggedString   Data;
	};
	int    ok = -1;
	SString prev_txt;
	TaggedString data;
	TaggedStringArray field_names;
	AddExtFldDialog * p_dlg = 0;
	if(!RVALUEPTR(data, pData))
		MEMSZERO(data);
	prev_txt = data.Txt;
	field_names = FieldNames;
	for(uint i = 0; i < Fields.getCount(); i++) {
		uint pos = 0;
		long id = Fields.Get(i).Id;
		if(id != data.Id && field_names.Search(id, &pos) > 0)
			field_names.atFree(pos);
	}
	if(CheckDialogPtrErr(&(p_dlg = new AddExtFldDialog(&field_names))) > 0) {
		p_dlg->setDTS(&data);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(!p_dlg->getDTS(&data))
				PPError();
			else if(sstrlen(Data.Tail) + sstrlen(data.Txt) - prev_txt.Len() < sizeof(Data.Tail)){
				LocationCore::SetExField(&Data, data.Id, data.Txt);
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
	if(pos >= 0 && pos < (long)FieldNames.getCount()) {
		SString      buf;
		TaggedString item;
		item.Id = id;
		Fields.GetText(id, buf);
		buf.CopyTo(item.Txt, sizeof(item.Txt));
		if(Edit(&item) > 0) {
			Fields.Add(item.Id, item.Txt);
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
		TaggedString item = FieldNames.at(i);
		ss.clear();
		temp_buf.Z().Cat(item.Id);
		ss.add(temp_buf, 0);
		temp_buf = item.Txt;
		if(temp_buf.Empty())
			temp_buf.CatEq("ID", item.Id);
		ss.add(temp_buf, 0);
		Fields.GetText(item.Id, temp_buf);
		ss.add(temp_buf, 0);
		if(!addStringToList(item.Id, ss.getBuf())) {
			ok = 0;
			PPError();
		}
	}
	return ok;
}

int LocationExtFieldsDialog::setDTS(const LocationTbl::Rec * pData)
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

int LocationExtFieldsDialog::getDTS(LocationTbl::Rec * pData)
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

int SLAPI EditDlvrAddrExtFields(LocationTbl::Rec * pData) { DIALOG_PROC_BODYERR(LocationExtFieldsDialog, pData); }

class LocationDialog : public TDialog {
public:
	LocationDialog(uint rezID, long locTyp, long edflags) : TDialog(rezID), EdFlags(edflags), LocTyp(locTyp)
	{
		LocationCore::GetTypeDescription(LocTyp, &Ltd);
		setTitle(Ltd.Name);
		enableCommand(cmExtFields, BIN(locTyp == LOCTYP_ADDRESS));
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
	SString temp_buf, nmb;
	if(getCtrlString(sel = CTL_LOCATION_COORD, temp_buf)) {
		SGeoPosLL pos;
		if(pos.FromStr(temp_buf)) {
			Data.Latitude = pos.Lat;
			Data.Longitude = pos.Lon;
			ok = 1;
		}
		else
			ok = PPErrorByDialog(this, sel, PPERR_SLIB);
	}
	return ok;
}

IMPL_HANDLE_EVENT(LocationDialog)
{
	TDialog::handleEvent(event);
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
	else if(event.isCmd(cmInputUpdatedByBtn)) {
		if(event.isCtlEvent(CTL_LOCATION_ZIP) || event.isCtlEvent(CTL_LOCATION_ADDR))
			SetupCtrls();
		else if(event.isCtlEvent(CTL_LOCATION_CODE)) {
			if(oneof3(LocTyp, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
				if(ReqAutoName()) {
					Data.ParentID = getCtrlLong(CTLSEL_LOCATION_PARENT);
					getCtrlData(CTL_LOCATION_CODE, Data.Code);
					SString name_buf;
					LocObj.MakeCodeString(&Data, 0, name_buf);
					setCtrlString(CTL_LOCATION_NAME, name_buf);
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
		const PPID def_phn_svc_id = DS.GetConstTLA().DefPhnSvcID;
		if(def_phn_svc_id) {
			SString temp_buf;
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
		// Перед редактированием адреса необходимо сохранить и очистить
		// поле ParentID в записи поскольку диалог редактирования очень
		// строго проверяет корректность полей записи.
		//
		const PPID save_par_id = Data.ParentID;
		getDTS(0);
		Data.ParentID = 0;
		if(LocObj.EditDialog(LOCTYP_ADDRESS, &Data, 0) == cmOK) {
			Data.ParentID = save_par_id;
			setDTS(0);
			Data.Type = (int16)save_loc_typ;
		}
		else
			Data.ParentID = save_par_id;
	}
	else if(event.isCmd(cmLocRegisters)) {
		PPObjRegister reg_obj;
		reg_obj.EditList(&Data);
	}
	else if(event.isCmd(cmLocTags)) {
		Data.TagL.ObjID = Data.ID; // @v9.9.7
		EditObjTagValList(&Data.TagL, 0);
	}
	else if(event.isCmd(cmTest)) {
		if(LocTyp == LOCTYP_ADDRESS) {
			SString addr_buf;
			PPLocationPacket addr;
			getDTS(&addr);
			LocationCore::GetAddress(addr, 0, addr_buf);
			LocObj.EditAddrStruc(addr_buf);
		}
	}
	else if(event.isCmd(cmaMore)) {
		if(Data.OwnerID) {
			PPObjPerson psn_obj;
			PPID   psn_id = Data.OwnerID;
			if(psn_obj.Edit(&psn_id, 0) > 0) {
				SString temp_buf;
				GetPersonName(Data.OwnerID, temp_buf);
				setCtrlString(CTL_LOCATION_OWNERNAME, temp_buf);
			}
		}
	}
	else if(event.isCmd(cmExtFields))
		EditDlvrAddrExtFields(&Data);
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
	SetupPPObjCombo(this, CTLSEL_LOCATION_OWNER, PPOBJ_PERSON, Data.OwnerID, OLW_LOADDEFONOPEN, (void *)PPPRK_EMPLOYER);
	SetupPPObjCombo(this, CTLSEL_LOCATION_RSPNS, PPOBJ_PERSON, Data.RspnsPersonID, OLW_LOADDEFONOPEN, (void *)PPPRK_EMPL);
	SetupPPObjCombo(this, CTLSEL_LOCATION_CITY,  PPOBJ_WORLD, Data.CityID, OLW_LOADDEFONOPEN|OLW_CANINSERT|OLW_CANSELUPLEVEL,
		PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
	LocationCore::GetExField(&Data, LOCEXSTR_ZIP, temp_buf);
	setCtrlString(CTL_LOCATION_ZIP, temp_buf);
	LocationCore::GetExField(&Data, LOCEXSTR_SHORTADDR, temp_buf);
	setCtrlString(CTL_LOCATION_ADDR, temp_buf);
	if(LocTyp == LOCTYP_ADDRESS) {
		AddClusterAssoc(CTL_LOCATION_FLAGS, 0, LOCF_MANUALADDR);
		SetClusterData(CTL_LOCATION_FLAGS, Data.Flags);
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
		setCtrlReadOnly(CTL_LOCATION_OWNERNAME, 1);
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
		setCtrlReal(CTL_LOCATION_CAPACITY, Data.MassCapacity / 1000.0);
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
		SetClusterData(CTL_LOCATION_FLAGS, Data.Flags);
	}
	SetupCtrls();
	if(getCtrlView(CTL_LOCATION_ST_ADDR)) {
		LocationCore::GetAddress(Data, 0, temp_buf);
		setStaticText(CTL_LOCATION_ST_ADDR, temp_buf);
	}
	disableCtrl(CTL_LOCATION_ID, 1);
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

int SLAPI PPObjLocation::EditDialog(PPID locTyp, LocationTbl::Rec * pData)
{
    PPLocationPacket temp_pack;
    temp_pack = *pData;
    int    ok = EditDialog(locTyp, &temp_pack, edfMainRecOnly);
    if(ok > 0) {
    	ASSIGN_PTR(pData, temp_pack);
    }
    return ok;
}

int SLAPI PPObjLocation::EditDialog(PPID locTyp, PPLocationPacket * pData, long flags)
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
		pData->Type = (int16)locTyp;
		if(locTyp == LOCTYP_DIVISION) {
			if(pData->OwnerID == 0)
				GetMainOrgID(&pData->OwnerID);
		}
		else if(locTyp == LOCTYP_WAREHOUSE && pData->OwnerID == 0)
			dlg->disableCtrl(CTLSEL_LOCATION_OWNER, 1);
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

int SLAPI PPObjLocation::IsPacketEq(const PPLocationPacket & rS1, const PPLocationPacket & rS2, long flags)
{
	int    eq = 1;
	if(!LocationCore::IsEqualRec(rS1, rS2))
		eq = 0;
	else if(!rS1.Regs.IsEqual(rS2.Regs))
		eq = 0;
	else if(!rS1.TagL.IsEqual(rS2.TagL))
		eq = 0;
	return eq;
}

int SLAPI PPObjLocation::GetPacket(PPID id, PPLocationPacket * pPack)
{
	int    ok = 1;
	pPack->destroy();
	if(id) {
		int    sr = Search(id, pPack);
		THROW(sr);
		if(sr > 0) {
			THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister));
			THROW(P_RegObj->P_Tbl->GetByLocation(id, &pPack->Regs));
			THROW(PPRef->Ot.GetList(Obj, id, &pPack->TagL));
		}
		else
			ok = -1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::PutPacket(PPID * pID, PPLocationPacket * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	const  int do_index_phones = BIN(CConfig.Flags2 & CCFLG2_INDEXEADDR);
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
					// Следующее условие введено для корректировки ошибки (до @v5.6.6), из-за
					// которой с персоналиями сохранялись пустые адреса с типом 0.
					//
					if(org_pack.Type != 0 || !org_pack.IsEmptyAddress()) {
						THROW_PP_S(!org_pack.Type || org_pack.Type == pPack->Type, PPERR_LOCTYPECHNG, org_pack.Name);
					}
					if(pPack->Type == LOCTYP_ADDRESS && pPack->IsEmptyAddress()) {
						if(do_index_phones) {
							LocationCore::GetExField(&org_pack, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid;
							objid.Set(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
						}
						THROW(p_ref->Ot.PutList(Obj, *pID, 0, 0));
						THROW(P_RegObj->P_Tbl->PutByLocation(*pID, 0, 0));
						THROW(RemoveByID(P_Tbl, *pID, 0));
						DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
						*pID = 0;
					}
					else {
						if(strcmp(org_pack.Name, pPack->Name) != 0)
							THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, Obj, *pID, pPack->Name, 0) == DBRPL_OK);
						if(do_index_phones) {
							LocationCore::GetExField(pPack, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid;
							objid.Set(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 0, 0));
						}
						THROW(UpdateByID(P_Tbl, Obj, *pID, pPack, 0));
						THROW(p_ref->Ot.PutList(Obj, *pID, &pPack->TagL, 0));
						THROW(P_RegObj->P_Tbl->PutByLocation(*pID, &pPack->Regs, 0));
						DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
					}
				}
				else
					ok = -1;
			}
			else {
				if(do_index_phones) {
					LocationCore::GetExField(&org_pack, LOCEXSTR_PHONE, temp_buf);
					PPObjID objid;
					objid.Set(PPOBJ_LOCATION, *pID);
					THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
				}
				THROW(p_ref->Ot.PutList(Obj, *pID, 0, 0));
				THROW(P_RegObj->P_Tbl->PutByLocation(*pID, 0, 0));
				THROW(RemoveByID(P_Tbl, *pID, 0));
				THROW(RemoveSync(*pID));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0)
				THROW(Dirty(*pID));
		}
		else if(pPack) {
			if(!oneof2(pPack->Type, LOCTYP_ADDRESS, 0) || !pPack->IsEmptyAddress()) {
				PPID   temp_id = 0;
				//
				// Функция LocationCore::Add уже содержит поддержку индексации телефонов (IndexPhone)
				//
				THROW(P_Tbl->Add(&temp_id, pPack, 0));
				THROW(p_ref->Ot.PutList(Obj, temp_id, &pPack->TagL, 0));
				THROW(P_RegObj->P_Tbl->PutByLocation(temp_id, &pPack->Regs, 0));
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

int SLAPI PPObjLocation::PutRecord(PPID * pID, LocationTbl::Rec * pPack, int use_ta)
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
					// Следующее условие введено для корректировки ошибки (до @v5.6.6), из-за
					// которой с персоналиями сохранялись пустые адреса с типом 0.
					//
					if(org_rec.Type != 0 || !LocationCore::IsEmptyAddressRec(org_rec)) {
						THROW_PP_S(!org_rec.Type || org_rec.Type == pPack->Type, PPERR_LOCTYPECHNG, org_rec.Name);
					}
					if(pPack->Type == LOCTYP_ADDRESS && LocationCore::IsEmptyAddressRec(*pPack)) {
						if(do_index_phones) {
							LocationCore::GetExField(&org_rec, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid;
							objid.Set(PPOBJ_LOCATION, *pID);
							THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
						}
						THROW(RemoveByID(P_Tbl, *pID, 0));
						DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
						*pID = 0;
					}
					else {
						if(strcmp(org_rec.Name, pPack->Name) != 0)
							THROW(SendObjMessage(DBMSG_OBJNAMEUPDATE, PPOBJ_ARTICLE, Obj, *pID, pPack->Name, 0) == DBRPL_OK);
						if(do_index_phones) {
							LocationCore::GetExField(pPack, LOCEXSTR_PHONE, temp_buf);
							PPObjID objid;
							objid.Set(PPOBJ_LOCATION, *pID);
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
				if(do_index_phones) {
					LocationCore::GetExField(&org_rec, LOCEXSTR_PHONE, temp_buf);
					PPObjID objid;
					objid.Set(PPOBJ_LOCATION, *pID);
					THROW(P_Tbl->IndexPhone(temp_buf, &objid, 1, 0));
				}
				THROW(RemoveByID(P_Tbl, *pID, 0));
				THROW(RemoveSync(*pID));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
			if(ok > 0)
				THROW(Dirty(*pID));
		}
		else if(pPack) {
			if(!oneof2(pPack->Type, LOCTYP_ADDRESS, 0) || !LocationCore::IsEmptyAddressRec(*pPack)) {
				PPID   temp_id = 0;
				//
				// Функция LocationCore::Add уже содрежит поддрежку индексации телефонов (IndexPhone)
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

int SLAPI PPObjLocation::InitCode(LocationTbl::Rec * pRec)
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
					code = 0;
					switch(coding) {
						case whzcAlpha:
							code.NumberToLat((uint)i);
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

int SLAPI PPObjLocation::GetRegister(PPID locID, PPID regType, LDATE actualDate, int inheritFromOwner, RegisterTbl::Rec * pRec)
{
	int    ok = -1;
	RegisterArray reg_list;
	THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister()));
	if(P_RegObj->P_Tbl->GetByLocation(locID, &reg_list) > 0) {
		if(reg_list.GetRegister(regType, actualDate, 0, pRec) > 0) { // @v9.9.12 @fix !0 --> >0
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

struct CreateWhLocParam {
	PPID   Type;
	LocationTbl::Rec CellSample;
};

int SLAPI PPObjLocation::EditCreateWhLocParam(CreateWhLocParam * pParam)
{
	class CreateWhLocParamDialog : public TDialog {
	public:
		CreateWhLocParamDialog() : TDialog(DLG_SELWZTYP)
		{
		}
		int    setDTS(CreateWhLocParam * pData)
		{
			Data = *pData;
			AddClusterAssocDef(CTL_SELWZTYP_WHAT, 0, LOCTYP_WHZONE);
			AddClusterAssoc(CTL_SELWZTYP_WHAT, 1, LOCTYP_WHCOLUMN);
			AddClusterAssoc(CTL_SELWZTYP_WHAT, 2, LOCTYP_WHCELL);
			AddClusterAssoc(CTL_SELWZTYP_WHAT, 3, LOCTYP_WHCELLAUTOGEN);
			SetClusterData(CTL_SELWZTYP_WHAT, Data.Type);
			enableCommand(cmGenWhCellSample, Data.Type == LOCTYP_WHCELLAUTOGEN);
			return 1;
		}
		int    getDTS(CreateWhLocParam * pData)
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
		CreateWhLocParam Data;
		PPObjLocation LocObj;
	};
	DIALOG_PROC_BODY(CreateWhLocParamDialog, pParam);
}

int SLAPI PPObjLocation::Edit(PPID * pID, void * extraPtr)
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
		MEMSZERO(cur_rec);
		if(extraPtr) {
			LocationFilt flt = *(LocationFilt*)extraPtr;
			loc_typ = NZOR(flt.LocType, LOCTYP_WAREHOUSE);
			pack.Type = (int16)loc_typ;
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
			MEMSZERO(cwlp);
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

int SLAPI PPObjLocation::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
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
					set(P_Tbl->OwnerID, dbconst((long)extraPtr))));
			}
		}
		else if(_obj == PPOBJ_WORLD) {
			if(_id) {
				THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->CityID == _id), set(P_Tbl->CityID, dbconst((long)extraPtr))));
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

int SLAPI PPObjLocation::SerializePacket(int dir, PPLocationPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_MEM(SETIFZ(P_RegObj, new PPObjRegister()));
	THROW_SL(P_Tbl->SerializeRecord(dir, pPack, rBuf, pSCtx));
	THROW_SL(P_RegObj->P_Tbl->SerializeArrayOfRecords(dir, &pPack->Regs, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPLocationPacket);
	{
		PPLocationPacket * p_pack = (PPLocationPacket *)p->Data;
		if(stream == 0) {
			THROW(GetPacket(id, p_pack) > 0);
			if(p_pack->Type == LOCTYP_ADDRESS)
				p->Priority = PPObjectTransmit::DependedPriority;
		}
		else {
			SBuffer buffer;
			THROW_SL(buffer.ReadFromFile((FILE*)stream, 0));
			THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1, r = 1;
	if(p && p->Data) {
		PPLocationPacket * p_pack = (PPLocationPacket *)p->Data;
		if(stream == 0) {
			PPID   same_id = 0;
			LocationTbl::Rec same_rec;
			if(*pID == 0) {
				p_pack->ID = 0;
				if(P_Tbl->SearchMaxLike(p_pack, 0, &same_id, &same_rec) > 0) {
					*pID = same_id;
					p_pack->ID = same_id;
					p_pack->Counter = same_rec.Counter;
					// (Не обновляем запись) r = PutPacket(pID, p_rec, 1);
				}
				else {
					r = PutPacket(pID, p_pack, 1);
				}
			}
			else if(Search(*pID, &same_rec) > 0) {
				p_pack->ID = *pID;
				p_pack->Counter = same_rec.Counter;
				// @v9.6.2 {
				if(p_pack->OwnerID != same_rec.OwnerID && p_pack->OwnerID && same_rec.OwnerID) {
					//
					// Потенциально конфликтная ситуация: пришедшая локация имеет отличное от
					// нашего значение владельца - необходимо разобраться.
					//
					PPObjPerson psn_obj;
					PersonTbl::Rec native_psn_rec, foreign_psn_rec;
					if(psn_obj.Search(same_rec.OwnerID, &native_psn_rec) > 0) {
						if(psn_obj.Search(p_pack->OwnerID, &foreign_psn_rec) > 0) {
							PPIDArray native_dlvr_loc_list;
							psn_obj.GetDlvrLocList(native_psn_rec.ID, &native_dlvr_loc_list);
							if(native_dlvr_loc_list.lsearch(*pID)) {
								// Нашли этот адрес среди адресов доставки "своего" владельца - безусловно оставляем существующего владельца
								p_pack->OwnerID = same_rec.OwnerID;
							}
						}
						else {
							// "Чужой" владелец не найден - безусловно оставляем существующего
							p_pack->OwnerID = same_rec.OwnerID;
						}
					}
				}
				// } @v9.6.2
				r = PutPacket(pID, p_pack, 1);
			}
			else {
				p_pack->ID = 0;
				r = PutPacket(pID, p_pack, 1);
			}
			if(r == 0) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTLOCATION, p_pack->ID, p_pack->Name);
				ok = -1;
			}
		}
		else {
			SBuffer buffer;
			THROW_SL(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjLocation::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPLocationPacket * p_pack = (PPLocationPacket *)p->Data;
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->ParentID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_WORLD,    &p_pack->CityID,   ary, replace));
		{
			int    dont_process_ref = 0;
			PPObjID oi;
			if(p_pack->Type == LOCTYP_ADDRESS && pCtx->GetPrevRestoredObj(&oi)) {
				if(oi.Obj == PPOBJ_PERSON)
					dont_process_ref = 1;
				else if(!replace) {
					PPObjID oi2;
					pCtx->ForceRestore(oi2.Set(PPOBJ_PERSON, p_pack->OwnerID));
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

int SLAPI PPObjLocation::MakeReserved(long flags)
{
	int    ok = 1;
	uint   num_recs, i;
	LocationTbl::Rec rec;
	TVRez * p_rez = P_SlRez;
	if(p_rez) {
		LocationTbl::Key2 k2;
		MEMSZERO(k2);
		k2.Type = LOCTYP_WAREHOUSE;
		if(!(P_Tbl->search(2, &k2, spGe) && P_Tbl->data.Type == LOCTYP_WAREHOUSE)) {
			SString name;
			THROW_DB(BTROKORNFOUND);
			THROW_PP(p_rez->findResource(ROD_LOCATION, PP_RCDATA), PPERR_RESFAULT);
			THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
			for(i = 0; i < num_recs; i++) {
				PPID   id = 0;
				p_rez->getUINT();
				p_rez->getString(name, 2);
				PPExpandString(name, CTRANSF_UTF8_TO_INNER); // @v9.2.1
				MEMSZERO(rec);
				rec.ID = 0;
				name.CopyTo(rec.Name, sizeof(rec.Name));
				rec.Type = LOCTYP_WAREHOUSE;
				THROW(P_Tbl->Add(&id, &rec, 1));
			}
		}
	}
	else
		ok = 0;
	CATCHZOK
	return ok;
}
//
// DivisionCtrlGroup
//
SLAPI DivisionCtrlGroup::Rec::Rec(PPID orgID, PPID divID, PPID staffID, PPID postID) :
	OrgID(orgID), DivID(divID), StaffID(staffID), PostID(postID)
{
}

DivisionCtrlGroup::DivisionCtrlGroup(uint _ctlsel_org, uint _ctlsel_div, uint _ctlsel_staff, uint _ctlsel_post) :
	CtrlGroup(), DivF(LOCTYP_DIVISION), CtlselOrg(_ctlsel_org), CtlselDiv(_ctlsel_div), CtlselStaff(_ctlsel_staff),
	CtlselPost(_ctlsel_post), flags(0)
{
	MEMSZERO(Data);
}

int DivisionCtrlGroup::setData(TDialog * dlg, void * data)
{
	Data = *(Rec *)data;
	if(Data.OrgID == 0 && Data.DivID) {
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		Data.OrgID = (loc_obj.Search(Data.DivID, &loc_rec) > 0) ? loc_rec.OwnerID : 0;
	}
	DivF.LocType = LOCTYP_DIVISION;
	DivF.Owner   = Data.OrgID;
	DivF.Parent  = 0;
	SetupPPObjCombo(dlg, CtlselOrg, PPOBJ_PERSON,   Data.OrgID, OLW_CANINSERT, (void *)PPPRK_EMPLOYER);
	SetupPPObjCombo(dlg, CtlselDiv, PPOBJ_LOCATION, Data.DivID, OLW_CANINSERT, &DivF);
	if(CtlselStaff)
		SetupStaffListCombo(dlg, CtlselStaff, Data.StaffID, OLW_CANINSERT, Data.OrgID, Data.DivID);
	if(CtlselPost)
		PPObjStaffList::SetupPostCombo(dlg, CtlselPost, Data.PostID, 0, Data.OrgID, Data.DivID, Data.StaffID);
	return 1;
}

int DivisionCtrlGroup::getData(TDialog * dlg, void * data)
{
	((Rec*)data)->OrgID = dlg->getCtrlLong(CtlselOrg);
	((Rec*)data)->DivID = dlg->getCtrlLong(CtlselDiv);
	((Rec*)data)->StaffID = dlg->getCtrlLong(CtlselStaff);
	((Rec*)data)->PostID = dlg->getCtrlLong(CtlselPost);
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
		SetupPPObjCombo(this, CTLSEL_DIVVIEW_ORG, PPOBJ_PERSON, CurOrgID, OLW_CANINSERT, (void *)PPPRK_EMPLOYER);
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
	else if(r == 0)
		ok = 0;
	return ok;
}

int DivisionView::editItem(long, long id)
{
	int    r = id ? LocObj.Edit(&id, 0) : -1;
	if(r == cmOK)
		return 1;
	else if(r == 0)
		return 0;
	else
		return -1;
}

int DivisionView::delItem(long, long id)
{
	return (id && LocObj.RemoveObjV(id, 0, PPObject::rmv_default, 0)) ? 1 : -1;
}

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

// static
int SLAPI PPObjLocation::ViewWarehouse()
{
	PPObjLocation locobj(0);
	locobj.Browse(0);
	return 1;
}

// static
int SLAPI PPObjLocation::ViewDivision()
{
	DivisionView * dlg = new DivisionView();
	return CheckDialogPtrErr(&dlg) ? ExecViewAndDestroy(dlg) : 0;
}
//
//
//
class LocationCache : public ObjCacheHash {
public:
	SLAPI LocationCache() : ObjCacheHash(PPOBJ_LOCATION, sizeof(LocationData), 1024*1024, 4),
		WhObjList(sizeof(WHObjEntry)), FullEaList(BIN(CConfig.Flags2 & CCFLG2_INDEXEADDR)), IsWhObjTabInited(0)
	{
		LoadWarehouseTab();
		MEMSZERO(Cfg);
	}
	PPID   SLAPI GetSingleWarehouse();
	uint   SLAPI GetWarehouseList(PPIDArray * pList);
	int    SLAPI CheckWarehouseFlags(PPID locID, long f);
	PPID   FASTCALL ObjToWarehouse(PPID arID, int ignoreRights);
	PPID   FASTCALL WarehouseToObj(PPID locID, int ignoreRights);
	int    SLAPI GetConfig(PPLocationConfig * pCfg, int enforce);
	int    SLAPI GetCellList(PPID locID, PPIDArray * pList); // @sync_w
	int    SLAPI DirtyCellList(PPID locID); // @sync_w

	int    SLAPI ReleaseFullEaList(const StrAssocArray * pList);
	const  StrAssocArray * SLAPI GetFullEaList();

	virtual int  FASTCALL Dirty(PPID); // @sync_w
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
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
	int    SLAPI LoadWarehouseTab();
	int    SLAPI AddWarehouseEntry(const LocationTbl::Rec * pRec);
	//
	// Descr: Полный список электронных адресов из хранилища EAddrCore
	//   для быстрого поиска по подстроке
	//
	class FealArray : public StrAssocArray {
	public:
		FealArray(int use) : StrAssocArray(), Use(use), Inited(0)
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
	SVector WhObjList; // @v10.0.05 SArray-->SVector
	TSCollection <WHCellEntry> WhCellList;
	PPLocationConfig Cfg;
	//
	ReadWriteLock CfgLock;
	ReadWriteLock WhclLock; // Блокировка для WhCellList.
public:
	struct LocationData : public ObjCacheEntry {
		PPID   ParentID; // Родительская локация //
		int16  Type;     //
		int16  Access;   // 0 - пользователь не имеет прав доступа к этому складу;
			// 1 - пользователь имеет права доступа к этому складу.
		PPID   OwnerID;
		PPID   CityID;
		PPID   RspnsPersonID;
		PPID   ArID;
		int16  Flags;
		int16  NumRows;
		int16  NumLayers;
		int16  Depth;
		long   Counter;
	};
};

const StrAssocArray * SLAPI LocationCache::GetFullEaList()
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
								((PPEAddr *)ea_rec.Addr)->GetPhone(phone_buf);
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

int SLAPI LocationCache::GetCellList(PPID locID, PPIDArray * pList)
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
			// Повторная попытка поиска после установки блокировки
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

int SLAPI PPObjLocation::GetDirtyCellParentsList(PPID locID, PPIDArray & rDestList)
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

int SLAPI LocationCache::DirtyCellList(PPID locID)
{
	{
		SRWLOCKER(WhclLock, SReadWriteLocker::Write);
		{
			PPObjLocation loc_obj(SConstructorLite);
			PPIDArray par_list;
			loc_obj.GetDirtyCellParentsList(locID, par_list);
			for(uint i = 0; i < par_list.getCount(); i++) {
				const PPID _id = par_list.get(i);
				uint pos = 0;
				if(WhCellList.lsearch(&_id, &pos, CMPF_LONG))
					WhCellList.atFree(pos);
			}
		}
	}
	return 1;
}

int SLAPI LocationCache::GetConfig(PPLocationConfig * pCfg, int enforce)
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

int FASTCALL LocationCache::Dirty(PPID locID)
{
	PPObjLocation loc_obj(SConstructorLite);
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		{
			LocationTbl::Rec loc_rec;
			uint pos = 0;
			if(loc_obj.Search(locID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE) {
				AddWarehouseEntry(&loc_rec);
			}
			else if(WhObjList.lsearch(&locID, &pos, CMPF_LONG))
				WhObjList.atFree(pos);
			Helper_Dirty(locID);
		}
	}
	return 1;
}

int SLAPI LocationCache::AddWarehouseEntry(const LocationTbl::Rec * pRec)
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
		// @v10.0.05 {
		if(!is_enabled)
			entry.Flags |= LOCF_INTERNAL_DISABLED;
		// } @v10.0.05 
		// @v10.0.05 if(ObjRts.CheckLocID(entry.LocID, 0)) {
			PPObjAccTurn * p_atobj = BillObj->atobj;
			if(p_atobj->P_Tbl->Art.SearchObjRef(LConfig.LocAccSheetID, entry.LocID) > 0)
				entry.ObjID = p_atobj->P_Tbl->Art.data.ID;
			ok = WhObjList.insert(&entry) ? 1 : PPSetErrorSLib();
		// @v10.0.05 }
	}
	return ok;
}

int SLAPI LocationCache::LoadWarehouseTab()
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
				WhObjList.clear();
				SEnum en = lobj.P_Tbl->Enum(LOCTYP_WAREHOUSE, 0, LocationCore::eoIgnoreParent);
				while(ok && en.Next(&loc_rec) > 0) {
					if(!AddWarehouseEntry(&loc_rec))
						ok = 0;
				}
				if(ok)
					IsWhObjTabInited = 1;
			}
		}
	}
	return ok;
}

PPID SLAPI LocationCache::GetSingleWarehouse()
{
	PPID   id = 0;
	LoadWarehouseTab();
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		//id = (WhObjList.getCount() == 1) ? ((WHObjEntry *)WhObjList.at(0))->LocID : 0;
		for(uint i = 0; i < WhObjList.getCount(); i++) {
			const WHObjEntry * p_entry = (const WHObjEntry *)WhObjList.at(i);
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

uint SLAPI LocationCache::GetWarehouseList(PPIDArray * pList)
{
	uint   c = 0;
	LoadWarehouseTab();
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		WHObjEntry * p_entry;
		for(uint i = 0; WhObjList.enumItems(&i, (void **)&p_entry);) {
			if(!(p_entry->Flags & LOCF_INTERNAL_DISABLED)) {
				CALLPTRMEMB(pList, add(p_entry->LocID));
				c++;
			}
		}
	}
	CALLPTRMEMB(pList, sortAndUndup());
	return c;
}

int SLAPI LocationCache::CheckWarehouseFlags(PPID locID, long f)
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

int SLAPI LocationCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	LocationData * p_cache_rec = (LocationData *)pEntry;
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

void SLAPI LocationCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	LocationTbl::Rec * p_data_rec = (LocationTbl::Rec *)pDataRec;
	const LocationData * p_cache_rec = (const LocationData *)pEntry;
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
	p_data_rec->Counter   = p_cache_rec->Counter;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Code, sizeof(p_data_rec->Code));
	b.Get(p_data_rec->Tail, sizeof(p_data_rec->Tail));
}

IMPL_OBJ_FETCH(PPObjLocation, LocationTbl::Rec, LocationCache);
IMPL_OBJ_DIRTY(PPObjLocation, LocationCache);

const StrAssocArray * SLAPI PPObjLocation::GetFullEaList()
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->GetFullEaList() : 0;
}

void SLAPI PPObjLocation::ReleaseFullEaList(const StrAssocArray * pList)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	CALLPTRMEMB(p_cache, ReleaseFullEaList(pList));
}

int SLAPI PPObjLocation::Helper_GetEaListBySubstring(const char * pSubstr, void * pList, long flags)
{
	int    ok = 1, r = 0;
	const  size_t substr_len = sstrlen(pSubstr);
	PPIDArray * p_list = 0;
	StrAssocArray * p_str_list = 0;
	if(flags & clsfStrList)
		p_str_list = (StrAssocArray *)pList;
	else
		p_list = (PPIDArray *)pList;
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
							// Здесь THROW не годится из-за того, что сразу после завершения цикла
							// необходимо быстро сделать ReleaseFullList
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

int SLAPI PPObjLocation::GetEaListBySubstring(const char * pSubstr, StrAssocArray * pList, int fromBegStr)
{
	long   flags = clsfStrList;
	if(fromBegStr)
		flags |= clsfFromBeg;
	int    ok = Helper_GetEaListBySubstring(pSubstr, pList, flags);
	CALLPTRMEMB(pList, SortByText());
	return ok;
}

int SLAPI PPObjLocation::ResolveWhCell(PPID locID, PPIDArray & rDestList, PPIDArray * pRecurTrace, int useCache)
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

// static
int FASTCALL PPObjLocation::FetchConfig(PPLocationConfig * pCfg)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION, 1);
	if(p_cache) {
		return p_cache->GetConfig(pCfg, 0);
	}
	else {
		memzero(pCfg, sizeof(*pCfg));
		return 0;
	}
}

// static
int SLAPI PPObjLocation::DirtyConfig()
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION, 0);
	return p_cache ? p_cache->GetConfig(0, 1) : 0;
}

PPID SLAPI PPObjLocation::GetSingleWarehouse()
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (Obj);
	return p_cache ? p_cache->GetSingleWarehouse() : 0;
}

uint SLAPI PPObjLocation::GetWarehouseList(PPIDArray * pList)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (Obj);
	return p_cache ? p_cache->GetWarehouseList(pList) : 0;
}

// static
int FASTCALL PPObjLocation::CheckWarehouseFlags(PPID locID, long f)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->CheckWarehouseFlags(locID, f) : 0;
}

// static
PPID FASTCALL PPObjLocation::ObjToWarehouse(PPID arID)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->ObjToWarehouse(arID, 0) : 0;
}

// static
PPID FASTCALL PPObjLocation::ObjToWarehouse_IgnoreRights(PPID arID)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->ObjToWarehouse(arID, 1) : 0;
}

// static
PPID FASTCALL PPObjLocation::WarehouseToObj(PPID locID)
{
	LocationCache * p_cache = GetDbLocalCachePtr <LocationCache> (PPOBJ_LOCATION);
	return p_cache ? p_cache->WarehouseToObj(locID, 0) : 0;
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
	delete (PPObjLocation*) Extra[0].Ptr;
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
		PPObjLocation * p_locobj = (PPObjLocation*)Extra[0].Ptr;
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
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _ARG_DATE(n) (*(LDATE *)rS.GetPtr(pApl->Get(n)))
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_STR     (**(SString **)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))
	if(pF->Name == "?GetAddedString") {
		_RET_STR.Z();
		PPObjLocation * p_obj = (PPObjLocation*)Extra[0].Ptr;
		LocationTbl::Rec loc_rec;
		if(p_obj->Search(H.ID, &loc_rec) > 0) {
			LocationCore::GetExField(&loc_rec, _ARG_INT(1), _RET_STR);
		}
	}
	else if(pF->Name == "?GetLongAddr") {
		_RET_STR.Z();
		PPObjLocation * p_obj = (PPObjLocation*)Extra[0].Ptr;
		LocationTbl::Rec loc_rec;
		if(p_obj->Search(H.ID, &loc_rec) > 0) {
			LocationCore::GetAddress(loc_rec, 0, _RET_STR);
		}
	}
	else if(pF->Name == "?GetRegister") {
		_RET_INT = 0;
		PPID   reg_type_id = 0;
		if(PPObjRegisterType::GetByCode(_ARG_STR(1), &reg_type_id) > 0) {
			const int inherit = _ARG_INT(2);
			PPObjLocation * p_obj = (PPObjLocation*)Extra[0].Ptr;
			RegisterTbl::Rec reg_rec;
			if(p_obj && p_obj->GetRegister(H.ID, reg_type_id, ZERODATE, inherit, &reg_rec) > 0)
				_RET_INT = reg_rec.ID;
		}
	}
	else if(pF->Name == "?GetRegisterD") {
		_RET_INT = 0;
		PPID   reg_type_id = 0;
		if(PPObjRegisterType::GetByCode(_ARG_STR(1), &reg_type_id) > 0) {
			const int inherit = _ARG_INT(3);
			PPObjLocation * p_obj = (PPObjLocation*)Extra[0].Ptr;
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
	delete (PPObjLocation*) Extra[0].Ptr;
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
		PPObjLocation * locobj = (PPObjLocation*)Extra[0].Ptr;
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
	delete (PPObjLocation*) Extra[0].Ptr;
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
		PPObjLocation * locobj = (PPObjLocation *)Extra[0].Ptr;
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

const FiasAbbrEntry _FiasAbbrList[] = {
	{  1, "Автономная область", "Аобл", 102, 0 },
	{  1, "Край", "край", 104, 0 },
	{  1, "Область", "обл", 105, 0 },
	{  1, "Округ", "округ", 107, 0 },
	{  1, "Республика", "Респ", 106, 0 },
	{  1, "Автономный округ", "АО", 101, 0 },
	{  2, "Автономный округ", "АО", 201, 0 },
	{  3, "Автономный округ", "АО", 305, 0 },
	{  3, "Поселение", "п", 306, 0 },
	{  3, "Улус", "у", 302, 0 },
	{  4, "Волость", "волость", 410, 0 },
	{  4, "Дачный поселок", "дп", 405, 0 },
	{  4, "Курортный поселок", "кп", 404, 0 },
	{  4, "Массив", "массив", 416, 0 },
	{  4, "Сельская администрация", "с/а", 407, 0 },
	{  4, "Сельское муницип.образование", "с/мо", 415, 0 },
	{  4, "Сельский округ", "с/о", 409, 0 },
	{  4, "Сельское поселение", "с/п", 414, 0 },
	{  4, "Сельсовет", "с/с", 406, 0 },
	{  3, "Район", "р-н", 301, 43 },
	{  5, "Район", "р-н", 503, 43 },
	{  6, "Аал", "аал", 601, 0 },
	{  6, "Автодорога", "автодорога", 645, 0 },
	{  6, "Волость", "волость", 603, 0 },
	{  6, "Выселки(ок)", "высел", 604, 0 },
	{  1, "Город", "г", 103, 1 },
	{  4, "Город", "г", 401, 1 },
	{  6, "Город", "г", 605, 1 },
	{  6, "Дачный поселок", "дп", 607, 0 },
	{  6, "Жилая зона", "жилзона", 647, 0 },
	{  6, "Жилой район", "жилрайон", 646, 0 },
	{  6, "Заимка", "заимка", 614, 0 },
	{  6, "Кордон", "кордон", 644, 0 },
	{  6, "Курортный поселок", "кп", 616, 0 },
	{  6, "Местечко", "м", 617, 0 },
	{  6, "Массив", "массив", 648, 0 },
	{  6, "Планировочный район", "п/р", 622, 0 },
	{  4, "Поселок городского типа", "пгт", 402, 3 },
	{  6, "Поселок городского типа", "пгт", 624, 3 },
	{  6, "Погост", "погост", 643, 0 },
	{  4, "Рабочий поселок", "рп", 403, 0 },
	{  6, "Рабочий поселок", "рп", 629, 0 },
	{  6, "Станица", "ст-ца", 633, 0 },
	{  6, "Улус", "у", 634, 0 },
	{  7, "Аал", "аал", 732, 0 },
	{  7, "Балка", "балка", 781, 0 },
	{  7, "Бугор", "бугор", 776, 0 },
	{  7, "Бухта", "бухта", 782, 0 },
	{  7, "Вал", "вал", 768, 0 },
	{  7, "Въезд", "въезд", 703, 0 },
	{  7, "Выселки(ок)", "высел", 734, 0 },
	{  7, "Горка", "горка", 786, 0 },
	{  7, "Гаражно-строительный кооператив", "гск", 763, 0 },
	{  7, "Дачное нек", "днп", 787, 0 },
	{  7, "Заезд", "заезд", 706, 0 },
	{  7, "Зона", "зона", 777, 0 },
	{  7, "Местечко", "м", 744, 0 },
	{  7, "Маяк", "маяк", 785, 0 },
	{  7, "Местность", "местность", 780, 0 },
	{  7, "Мост", "мост", 770, 0 },
	{  7, "Мыс", "мыс", 784, 0 },
	{  7, "Некоммерче", "н/п", 788, 0 },
	{  7, "Планировочный район", "п/р", 750, 0 },
	{  7, "Переезд", "переезд", 715, 0 },
	{  7, "Площадка", "пл-ка", 717, 0 },
	{  7, "Полустанок", "полустанок", 752, 0 },
	{  7, "Причал", "причал", 783, 0 },
	{  7, "Просек", "просек", 720, 0 },
	{  7, "Просека", "просека", 774, 0 },
	{  7, "Проселок", "проселок", 721, 0 },
	{  7, "Проток", "проток", 766, 0 },
	{  7, "Протока", "протока", 775, 0 },
	{  7, "Проулок", "проулок", 722, 0 },
	{  7, "Ряды", "ряды", 771, 0 },
	{  7, "Сад", "сад", 723, 0 },
	{  7, "Сквер", "сквер", 724, 0 },
	{  7, "тоннель", "тоннель", 779, 0 },
	{  7, "Тракт", "тракт", 727, 0 },
	{  7, "Тупик", "туп", 728, 0 },
	{  7, "Участок", "уч-к", 730, 0 },
	{  7, "Фермерское", "ф/х", 789, 0 },
	{  7, "эстакада", "эстакада", 778, 0 },
	{  8, "Дом", "ДОМ", 801, 0 },
	{ 90, "Гаражно-строительный кооператив", "гск", 9004, 0 },
	{ 90, "Дачное некоммерческое партнерство", "днп", 9010, 0 },
	{ 90, "Местность", "местность", 9011, 0 },
	{ 90, "Некоммерческое партнерство", "н/п", 9009, 0 },
	{  6, "Промышленная зона", "промзона", 627, 0 },
	{ 90, "Промышленная зона", "промзона", 9003, 0 },
	{ 90, "Сад", "сад", 9007, 0 },
	{ 90, "Садовое товарищество", "снт", 9002, 0 },
	{  3, "Территория", "тер",  303, 0 },
	{  4, "Территория", "тер",  412, 0 },
	{  5, "Территория", "тер",  502, 0 },
	{  6, "Территория", "тер",  637, 0 },
	{  7, "Территория", "тер",  726, 0 },
	{ 90, "Территория", "тер", 9005, 0 },
	{ 90, "Фермерское хозяйство", "ф/х", 9008, 0 },
	{  7, "Абонентский ящик", "а/я",  772, 49 },
	{ 91, "Абонентский ящик", "а/я", 9172, 49 },
	{ 91, "Аал", "аал", 9132, 0 },
	{  7, "Аллея", "аллея",  701, 53 },
	{ 91, "Аллея", "аллея", 9101, 53 },
	{  6, "Арбан", "арбан", 640, 0 },
	{  7, "Арбан", "арбан", 760, 0 },
	{ 91, "Арбан", "арбан", 9160, 0 },
	{  6, "Аул", "аул",  602, 0 },
	{  7, "Аул", "аул",  733, 0 },
	{ 91, "Аул", "аул", 9133, 0 },
	{  7, "Берег", "берег",  773, 0 },
	{ 91, "Берег", "берег", 9173, 0 },
	{  7, "Бульвар", "б-р",  702, 55 },
	{ 91, "Бульвар", "б-р", 9102, 55 },
	{ 91, "Бугор", "бугор", 9176, 0 },
	{ 91, "Вал", "вал", 9168, 0 },
	{ 91, "Въезд", "въезд", 9103, 0 },
	{ 91, "Выселки(ок)", "высел", 9134, 0 },
	{  6, "Городок", "городок",  636, 0 },
	{  7, "Городок", "городок",  735, 0 },
	{ 91, "Городок", "городок", 9135, 0 },
	{ 91, "Гаражно-строительный кооператив", "гск", 9163, 0 },
	{  6, "Деревня", "д",  606, 6 },
	{  7, "Деревня", "д",  736, 6 },
	{ 91, "Деревня", "д", 9136, 6 },
	{  7, "Дорога", "дор",  704, 0 },
	{ 91, "Дорога", "дор", 9104, 0 },
	{  6, "Железнодорожная будка", "ж/д_будка", 608, 0 },
	{  6, "Железнодорожная казарма", "ж/д_казарм", 609, 0 },
	{  6, "Железнодорожная остановка (обгонный) пункт", "ж/д_оп", 610, 0 },
	{  6, "Железнодорожная платформа", "ж/д_платф", 638, 0 },
	{  6, "Железнодорожный пост", "ж/д_пост", 611, 0 },
	{  6, "Железнодорожный разъезд", "ж/д_рзд", 612, 0 },
	{  6, "Железнодорожная станция", "ж/д_ст", 613, 0 },
	{  7, "Железнодорожная будка", "ж/д_будка", 737, 0 },
	{  7, "Железнодорожная казарма", "ж/д_казарм", 738, 0 },
	{  7, "Железнодорожная остановка (обгонный) пункт", "ж/д_оп", 739, 0 },
	{  7, "Железнодорожная платформа", "ж/д_платф", 759, 0 },
	{  7, "Железнодорожный пост", "ж/д_пост", 740, 0 },
	{  7, "Железнодорожный разъезд", "ж/д_рзд", 741, 0 },
	{  7, "Железнодорожная станция", "ж/д_ст", 742, 0 },
	{  7, "Животноводческая точка", "жт", 705, 0 },
	{ 91, "Железнодорожная будка", "ж/д_будка", 9137, 0 },
	{ 91, "Железнодорожная казарма", "ж/д_казарм", 9138, 0 },
	{ 91, "Железнодорожная остановкв (обгонный) пункт", "ж/д_оп", 9139, 0 },
	{ 91, "Железнодорожная платформа", "ж/д_платф", 9159, 0 },
	{ 91, "Железнодорожный пост", "ж/д_пост", 9140, 0 },
	{ 91, "Железнодорожный разъезд", "ж/д_рзд", 9141, 0 },
	{ 91, "Железнодорожная станция", "ж/д_ст", 9142, 0 },
	{ 91, "Животноводческая точка", "жт", 9105, 0 },
	{ 91, "Заезд", "заезд", 9106, 0 },
	{ 91, "Зона", "зона", 9177, 0 },
	{  6, "Казарма", "казарма", 615, 0 },
	{  7, "Казарма", "казарма", 743, 0 },
	{ 91, "Казарма", "казарма", 9143, 0 },
	{  7, "Канал", "канал", 762, 0 },
	{ 91, "Канал", "канал", 9162, 0 },
	{  6, "Квартал", "кв-л", 639, 0 },
	{  7, "Квартал", "кв-л", 707, 0 },
	{ 91, "Квартал", "кв-л", 9107, 0 },
	{  7, "Километр", "км", 708, 0 },
	{ 91, "Километр", "км", 9108, 0 },
	{  7, "Кольцо", "кольцо", 709, 0 },
	{ 91, "Кольцо", "кольцо", 9109, 0 },
	{  7, "Коса", "коса", 767, 0 },
	{ 91, "Коса", "коса", 9167, 0 },
	{  7, "Линия", "линия", 710, 0 },
	{ 91, "Линия", "линия", 9110, 0 },
	{  6, "Леспромхоз", "лпх", 642, 0 },
	{  7, "Леспромхоз", "лпх", 765, 0 },
	{ 91, "Леспромхоз", "лпх", 9165, 0 },
	{ 91, "Местечко", "м", 9144, 0 },
	{  6, "Микрорайон", "мкр", 618, 0 },
	{  7, "Микрорайон", "мкр", 745, 0 },
	{ 90, "Микрорайон", "мкр", 9006, 0 },
	{ 91, "Микрорайон", "мкр", 9145, 0 },
	{ 91, "Мост", "мост", 9170, 0 },
	{  7, "Набережная", "наб", 711 , 51 },
	{ 91, "Набережная", "наб", 9111, 51 },
	{  6, "Населенный пункт", "нп", 619, 0 },
	{  7, "Населенный пункт", "нп", 746, 0 },
	{ 91, "Населенный пункт", "нп", 9146, 0 },
	{  6, "Остров", "остров",  620, 0 },
	{  7, "Остров", "остров",  712, 0 },
	{ 91, "Остров", "остров", 9112, 0 },
	{  4, "Поселок", "п",  417, 3 },
	{  6, "Поселок", "п",  621, 3 },
	{  7, "Поселок", "п",  748, 3 },
	{ 91, "Поселок", "п", 9148, 3 },
	{  4, "Почтовое отделение", "п/о", 411, 0 },
	{  6, "Почтовое отделение", "п/о", 626, 0 },
	{  7, "Почтовое отделение", "п/о", 749, 0 },
	{ 91, "Почтовое отделение", "п/о", 9149, 0 },
	{ 91, "Планировочный район", "п/р", 9150, 0 },
	{  6, "Поселок и(при) станция(и)", "п/ст", 623, 0 },
	{  7, "Поселок и(при) станция(и)", "п/ст", 751, 0 },
	{ 91, "Поселок и(при) станция(и)", "п/ст", 9151, 0 },
	{  7, "Парк", "парк", 713, 0 },
	{ 91, "Парк", "парк", 9113, 0 },
	{  7, "Переулок", "пер", 714,  24 },
	{ 91, "Переулок", "пер", 9114, 24 },
	{ 91, "Переезд", "переезд", 9115, 0 },
	{  7, "Площадь", "пл",  716, 22 },
	{ 91, "Площадь", "пл", 9116, 22 },
	{  7, "Платформа", "платф", 747, 0 },
	{ 91, "Платформа", "платф", 9147, 0 },
	{ 91, "Площадка", "пл-ка", 9117, 0 },
	{ 91, "Полустанок", "полустанок", 9152, 0 },
	{  6, "Починок", "починок", 625, 0 },
	{  7, "Починок", "починок", 753, 0 },
	{ 91, "Починок", "починок", 9153, 0 },
	{  7, "Проспект", "пр-кт", 719, 18 },
	{ 91, "Проспект", "пр-кт", 9119, 18 },
	{  7, "Проезд", "проезд",  718, 0 },
	{ 91, "Проезд", "проезд", 9118, 0 },
	{ 91, "Просек", "просек", 9120, 0 },
	{ 91, "Просека", "просека", 9174, 0 },
	{ 91, "Проселок", "проселок", 9121, 0 },
	{ 91, "Проток", "проток", 9166, 0 },
	{ 91, "Протока", "протока", 9175, 0 },
	{ 91, "Проулок", "проулок", 9122, 0 },
	{  6, "Разъезд", "рзд", 628, 0 },
	{  7, "Разъезд", "рзд", 754, 0 },
	{ 91, "Разъезд", "рзд", 9154, 0 },
	{ 91, "Ряды", "ряды", 9171, 0 },
	{  6, "Село", "с",  630, 9 },
	{  7, "Село", "с",  755, 9 },
	{ 91, "Село", "с", 9155, 9 },
	{ 91, "Сад", "сад", 9123, 0 },
	{ 91, "Сквер", "сквер", 9124, 0 },
	{  6, "Слобода", "сл", 631, 0 },
	{  7, "Слобода", "сл", 756, 0 },
	{ 91, "Слобода", "сл", 9156, 0 },
	{  6, "Садовое некоммерческое товарищество", "снт", 641, 0 },
	{  7, "Садовое некоммерческое товарищество", "снт", 764, 0 },
	{ 91, "Садовое некоммерческое товарищество", "снт", 9164, 0 },
	{  7, "Спуск", "спуск", 761, 0 },
	{ 91, "Спуск", "спуск", 9161, 0 },
	{  6, "Станция", "ст",  632, 13 },
	{  7, "Станция", "ст",  757, 13 },
	{ 91, "Станция", "ст", 9157, 13 },
	{  7, "Строение", "стр",  725, 59 },
	{ 91, "Строение", "стр", 9125, 59 },
	{ 91, "Территория", "тер", 9126, 0 },
	{ 91, "Тракт", "тракт", 9127, 0 },
	{ 91, "Тупик", "туп", 9128, 0 },
	{  7, "Улица", "ул",  729, 15 },
	{ 91, "Улица", "ул", 9129, 15 },
	{ 91, "Участок", "уч-к", 9130, 0 },
	{  7, "Ферма", "ферма", 769, 0 },
	{ 91, "Ферма", "ферма", 9169, 0 },
	{  6, "Хутор", "х",  635, 11 },
	{  7, "Хутор", "х",  758, 11 },
	{ 91, "Хутор", "х", 9158, 11 },
	{  7, "Шоссе", "ш",  731, 29 },
	{ 91, "Шоссе", "ш", 9131, 29 }
};

static AddrItemDescr Aidl[] = {
	{  1, "город",    0, PPLocAddrStruc::tCity, 0 },
	{  2, "г",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 1 },
	{  3, "поселок",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tCity, 0 },
	{  4, "пос",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 3 },
	{  5, "п",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 3 },
	{  6, "деревня",  0, PPLocAddrStruc::tCity, 0 },
	{  7, "дер",      AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 6 },
	{  8, "д",        AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 6 },
	{  9, "село",     0, PPLocAddrStruc::tCity, 0 },
	{ 10, "с",        AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 9 },
	{ 11, "хутор",    0, PPLocAddrStruc::tCity, 0 },
	{ 12, "х",        AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 11 },
	{ 13, "станция",  0, PPLocAddrStruc::tCity, 0 },
	{ 14, "ст",       AddrItemDescr::fOptDot, PPLocAddrStruc::tCity, 13 },
	{ 15, "улица",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 16, "ул",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 15 },
	{ 17, "у",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 15 },
	{ 18, "проспект", AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 19, "пр",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 18 },
	{ 20, "пр-т",     AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 18 },
	{ 21, "п",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 18 },
	{ 22, "площадь",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 23, "пл",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 22 },
	{ 24, "переулок", AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 25, "пер",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 24 },
	{ 26, "проезд",   AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 27, "пр-д",     AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 26 },
	{ 28, "п-д",      AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 26 },
	{ 29, "шоссе",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 30, "ш",        AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 29 },
	{ 31, "шос",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 29 },
	{ 32, "дом",      AddrItemDescr::fNextDigit, PPLocAddrStruc::tHouse, 0 },
	{ 33, "д",        AddrItemDescr::fNextDigit|AddrItemDescr::fOptDot, PPLocAddrStruc::tHouse, 32 },
	{ 34, "корпус",   0, PPLocAddrStruc::tHouseAddendum, 0 },
	{ 35, "кор",      AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 34 },
	{ 36, "корп",     AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 34 },
	{ 37, "квартира", AddrItemDescr::fNextDigit, PPLocAddrStruc::tApart, 0 },
	{ 38, "кв",       AddrItemDescr::fNextDigit|AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 37 },
	{ 39, "офис",     0, PPLocAddrStruc::tApart, 0 },
	{ 40, "оф",       AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 39 },
	{ 41, "комната",  0, PPLocAddrStruc::tApart, 0 },
	{ 42, "к",        AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 41 },
	{ 43, "район",    AddrItemDescr::fSfx, PPLocAddrStruc::tLocalArea, 0 },
	{ 44, "р-н",      AddrItemDescr::fSfx, PPLocAddrStruc::tLocalArea, 43 },
	{ 45, "р",        AddrItemDescr::fSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tLocalArea, 43 },
	{ 46, "строение", AddrItemDescr::fNextDigit, PPLocAddrStruc::tHouse, 0 },
	{ 47, "стр",      AddrItemDescr::fNextDigit|AddrItemDescr::fOptDot, PPLocAddrStruc::tHouse, 46 },
	{ 48, "комн",     AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 41 },
	{ 49, "а/я",      AddrItemDescr::fNextDigit, PPLocAddrStruc::tPostBox,  0 },
	{ 50, "ая",       AddrItemDescr::fNextDigit, PPLocAddrStruc::tPostBox, 49 },
	{ 51, "набережная", AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 52, "наб",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 51 },
	{ 53, "аллея",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 54, "ал",       AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 53 },
	{ 55, "бульвар",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 0 },
	{ 56, "бул",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 55 },
	{ 57, "область",  AddrItemDescr::fOptSfx, PPLocAddrStruc::tLocalArea, 0 },
	{ 58, "обл",      AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tLocalArea, 57 },
	{ 59, "строение", 0, PPLocAddrStruc::tHouseAddendum, 59 },
	{ 60, "стр",      AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 59 },
	{ 61, "литера",   0, PPLocAddrStruc::tHouseAddendum, 61 },
	{ 62, "лит",      AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 61 },
	{ 63, "помещение", 0, PPLocAddrStruc::tApart, 63 },
	{ 64, "пом",      AddrItemDescr::fOptDot, PPLocAddrStruc::tApart, 63 },
	{ 65, "р-он",     AddrItemDescr::fSfx, PPLocAddrStruc::tLocalArea, 43 },
	{ 66, "пр-кт",    AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 18 },
	{ 67, "литер",    AddrItemDescr::fOptDot, PPLocAddrStruc::tHouseAddendum, 61 },
	{ 68, "б-р",      AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 55 },
	{ 69, "ш-се",     AddrItemDescr::fOptSfx|AddrItemDescr::fOptDot, PPLocAddrStruc::tStreet, 29 },
	{ 70, "пгт",      AddrItemDescr::fOptSfx, PPLocAddrStruc::tCity, 3 },
	{ 71, "б",        AddrItemDescr::fOptSfx, PPLocAddrStruc::tStreet, 55 },
};

PPLocAddrStruc::AddrTok::AddrTok() : T(0), P(0), Flags(0), P_SplitL(0)
{
}

PPLocAddrStruc::AddrTok::~AddrTok()
{
	delete P_SplitL;
}

PPLocAddrStruc::AddrTok & PPLocAddrStruc::AddrTok::Reset()
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

SLAPI PPLocAddrStruc_MatchEntry::PPLocAddrStruc_MatchEntry(uint p1, uint p2, int reverse) : P1(p1), P2(p2), Reverse(reverse)
{
}

SLAPI PPLocAddrStruc_MatchEntry::PPLocAddrStruc_MatchEntry(const PPLocAddrStruc_MatchEntry & rS) : 
	P1(rS.P1), P2(rS.P2), Reverse(rS.Reverse), CityStreetList(rS.CityStreetList)
{
}

int SLAPI PPLocAddrStruc::Helper_Construct()
{
	int    ok = 1;
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
	Scan.RegisterRe("^[0-9]+[А-Яа-яA-Za-z][ \t]*\\-[ \t]*[0-9]+", &ReNumA_Hyp_Num);
	Scan.RegisterRe("^[0-9]+[А-Яа-яA-Za-z][ \t]*\\/[ \t]*[0-9]+", &ReNumA_Sl_Num);
	return ok;
}

SLAPI PPLocAddrStruc::PPLocAddrStruc(const char * pText, PPFiasReference * pFr) : StrAssocArray()
{
	Helper_Construct();
	P_Fr = pFr;
	Recognize(pText);
}

SLAPI PPLocAddrStruc::PPLocAddrStruc(ConditionalConstructWithFias ccwf) : StrAssocArray()
{
	Helper_Construct();

	PPLocationConfig loc_cfg;
	PPObjLocation::FetchConfig(&loc_cfg);
	if(loc_cfg.Flags & PPLocationConfig::fUseFias) {
		P_Fr = new PPFiasReference;
		State |= stOwnFiasRef;
	}
}

SLAPI PPLocAddrStruc::~PPLocAddrStruc()
{
	ZDELETE(P_AmbigMatchEntry);
	ZDELETE(P_AmbigMatchList);
	if(State & stOwnFiasRef)
		ZDELETE(P_Fr);
}

int SLAPI PPLocAddrStruc::HasAmbiguity() const
{
	if(P_AmbigMatchEntry)
		return 1;
	else if(P_AmbigMatchList)
		return 2;
	else
		return 0;
}

const TSCollection <PPLocAddrStruc_MatchEntry> * SLAPI PPLocAddrStruc::GetAmbiguityMatchList() const
{
	return P_AmbigMatchList;
}

const PPLocAddrStruc_MatchEntry * SLAPI PPLocAddrStruc::GetAmbiguityMatchEntry() const
{
	return P_AmbigMatchEntry;
}

int SLAPI PPLocAddrStruc::MatchEntryToStr(const PPLocAddrStruc_MatchEntry * pEntry, SString & rBuf)
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

int SLAPI PPLocAddrStruc::DetectStreetName(DetectBlock & rDb)
{
	int    ok = 0;
	rDb.FiasCandidList.clear();
	SString temp_buf;
	if(P_Fr) {
		PPIDArray fao_list;
        if(P_Fr->SearchObjByText(rDb.OrgText, PPFiasReference::stfAnsiInput, rDb.FiasUpperID, fao_list) > 0) {
			for(uint i = 0; i < fao_list.getCount(); i++) {
                const PPID fao_id = fao_list.get(i);
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

int SLAPI PPLocAddrStruc::DetectCityName(DetectBlock & rDb)
{
	int    ok = 0;
	rDb.FiasCandidList.clear();
	SString temp_buf;
	if(P_Fr) {
		PPIDArray fao_list;
        if(P_Fr->SearchObjByText(rDb.OrgText, PPFiasReference::stfAnsiInput, 0, fao_list) > 0) {
			for(uint i = 0; i < fao_list.getCount(); i++) {
                const PPID fao_id = fao_list.get(i);
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
		if(BillObj) { // Проверка на случай, если сеанс не авторизован в базе данных
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
				{ "спб", "Санкт-Петербург" },
				{ "петербург", "Санкт-Петербург" },
				{ "мск", "Москва" },
				{ "пск", "Петрозаводск" },
				{ "птз", "Петрозаводск" }
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

int SLAPI PPLocAddrStruc::GetTok(AddrTok & rTok)
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
		if(strchr(p_div, c)) {
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
			} while(isdec((uchar)c));
			//
			// Пытаемся засечь порядковое числительное с последующим текстом.
			// Это - обычно улица либо поселок.
			//
			uint i = 0;
			while(Scan[i] == ' ')
				i++;
			if(Scan[i] == '-') {
				i++;
				while(Scan[i] == ' ')
					i++;
			}
			static const char * p_ordinal_sfx[] = { "я", "Я", "й", "Й", "ый", "ЫЙ", "ая", "АЯ", "го", "ГО", "е", "Е" };
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
				// Просто число
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

int SLAPI PPLocAddrStruc::ProcessDescr(const AddrItemDescr & rDescr, DescrSelector & rSel)
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

int SLAPI PPLocAddrStruc::GetFiasAddrObjKind(PPID adrObjID, SString & rKind)
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

int SLAPI PPLocAddrStruc::Recognize(const char * pText)
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
		SString text = pText;
		SString temp_buf, temp_buf2, temp_buf3;
		text.Strip();
		if(text.CmpPrefix(p_enforcefias_prefix, 1) == 0) {
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
			//
			//
			//
			uint c = TokList.getCount();
			for(uint i = 0; i < c; i++) {
				AddrTok * p_tok = TokList.at(i);
				AddrTok * p_next_tok = (i < (c-1)) ? TokList.at(i+1) : 0;
				AddrTok * p_prev_tok = (i > 0) ? TokList.at(i-1) : 0;
				if(p_tok->T == AddrTok::tText) {
					if(p_prev_tok) {
						int   do_merge = 0;
						int   merge_type = AddrTok::tText;
						int   prev_ann_number = 0; // Если !0 то предыдущий токен - круглое число (встречается в названиях улиц)
						temp_buf = p_prev_tok->S;
						temp_buf2 = 0;
						if(p_prev_tok->T == AddrTok::tNumber) {
							const long v = temp_buf.ToLong();
							const ushort an_list[] = { 10, 20, 25, 30, 40, 50, 60, 75, 80, 100, 125, 150, 200 };
							for(uint j = 0; !prev_ann_number && j < SIZEOFARRAY(an_list); j++)
								if(v == (long)an_list[j])
									prev_ann_number = 1;
						}

						/*
						if(p_prev_tok->T == AddrTok::tNumber) {
							if(p_tok->S.Len() == 1) {
								(temp_buf = p_tok->S).ToLower1251();
								const uchar _c = temp_buf.C(0);
								if(oneof5(_c, 'а', 'б', 'в', 'г', 'д'))
									do_merge = 1;
							}
						}
						else*/ if(oneof2(p_prev_tok->T, AddrTok::tText, AddrTok::tDescr) || prev_ann_number) {
							if(!p_next_tok || oneof3(p_next_tok->T, AddrTok::tText, AddrTok::tDiv, AddrTok::tDescr)) {
								if(p_prev_tok->Flags & AddrTok::fDot)
									temp_buf.Dot();
								if(prev_ann_number) {
									(temp_buf2 = p_tok->S).ToLower1251();
									if(temp_buf2 == "лет" || temp_buf2 == "летия") {
										temp_buf.Space().Cat(p_tok->S);
										do_merge = 1;
									}
								}
								if(!do_merge) {
									temp_buf.Space().Cat(p_tok->S);
									(temp_buf2 = temp_buf).ToLower1251();
									if(temp_buf2 == "к. маркса" || temp_buf2 == "к маркса") {
										temp_buf2 = "Карла Маркса";
										do_merge = 2;
									}
									else if(temp_buf2 == "а. невского" || temp_buf2 == "а невского" || temp_buf2 == "ал. невского") {
										temp_buf2 = "Александра Невского";
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
				/* @v7.8.5 const*/ AddrTok * p_tok = TokList.at(i);
				if(!(p_tok->Flags & AddrTok::fUsed)) {
					/* @v7.8.5 const*/ AddrTok * p_next_tok = (i < (c-1)) ? TokList.at(i+1) : 0;
					/* @v7.8.5 const*/ AddrTok * p_prev_tok = (i > 0) ? TokList.at(i-1) : 0;
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
											AddrItemDescr & r_aid = Aidl[wo_idx];
											ProcessDescr(r_aid, ds);
										}
										else {
											for(uint j = 0; j < p_prev_tok->Dl.getCount(); j++) {
												wo_idx = p_prev_tok->Dl.get(j);
												AddrItemDescr & r_aid = Aidl[wo_idx];
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
											AddrItemDescr & r_aid = Aidl[wo_idx];
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
											AddrItemDescr & r_aid = Aidl[p_prev_tok->Dl.get(j)];
											int r = ProcessDescr(r_aid, ds);
											if(oneof4(ds.T, tHouse, tHouseAddendum, tPostBox, tApart)) {
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
									Add(tHouseKind, "дом");
								}
								if(temp_buf2.NotEmptyS()) {
									Add(last_t = tApart, temp_buf2);
									Add(tApartKind, "квартира");
								}
								p_tok->Flags |= AddrTok::fUsed;
							//}
							break;
						case AddrTok::tNum_Hyp_Num_Hyp_Num:
							//if(last_t == tStreet) {
								p_tok->S.Divide('-', temp_buf.Z(), temp_buf2.Z());
								if(temp_buf.NotEmptyS()) {
									Add(last_t = tHouse, temp_buf);
									Add(tHouseKind, "дом");
								}
								if(temp_buf2.NotEmptyS()) {
									temp_buf2.Divide('-', temp_buf.Z(), temp_buf3.Z());
									if(temp_buf.NotEmptyS()) {
										Add(last_t = tHouseAddendum, temp_buf);
										Add(tHouseAddendumKind, "корпус");
									}
									if(temp_buf3.NotEmptyS()) {
										Add(last_t = tApart, temp_buf3);
										Add(tApartKind, "квартира");
									}
								}
								p_tok->Flags |= AddrTok::fUsed;
							//}
							break;
						case AddrTok::tNum_Sl_Num:
							if(last_t == tStreet) {
								p_tok->S.Divide('/', temp_buf.Z(), temp_buf2.Z());
								temp_buf.Strip().CatChar('/').Cat(temp_buf2.Strip());
								Add(last_t = tHouse, temp_buf);
								Add(tHouseKind, "дом");
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
							// Поиск непротиворечивого населенного пункта
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
								const PPID fao_id = fao_list.get(--_p);
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
									const PPID fao_id = fao_list.get(i);
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
									const PPID fias_city_id = fias_city_list.get(ci);
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
							const PPID fias_city_id = fias_city_list.get(ci);
							local_fao_list.clear();
							if(P_Fr->SearchObjByText(temp_buf, PPFiasReference::stfAnsiInput, fias_city_id, local_fao_list) > 0) {
								uint _p = local_fao_list.getCount();
								if(_p) do {
									const PPID fao_id = local_fao_list.get(--_p);
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
								const PPID fao_id = fao_list.get(i);
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
                                            const PPID sobj = p1->FiasCandidList.at(sp);
                                            const PPID cobj = p2->FiasCandidList.at(cp);
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
                                            const PPID sobj = p2->FiasCandidList.at(sp);
                                            const PPID cobj = p1->FiasCandidList.at(cp);
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
									// Не однозначность в разрешении города/улицы
                                    THROW_MEM(P_AmbigMatchEntry = new PPLocAddrStruc_MatchEntry(*p_mentry));
                                }
							}
						}
						else if(_match_list.getCount() > 1) {
							// Неоднозначность в разрешении города/улицы
							THROW_MEM(P_AmbigMatchList = new TSCollection <PPLocAddrStruc_MatchEntry>);
							THROW_SL(TSCollection_Copy(*P_AmbigMatchList, _match_list));
						}
					}
					if(FiasStreetID) {
						temp_buf.Z();
						if(GetText(tHouse, temp_buf2) > 0) {
                            (temp_buf = temp_buf2).ToLower1251();
                            temp_buf.CatChar(':');
							if(GetText(tHouseAddendum, temp_buf2) > 0) {
								temp_buf.Cat(temp_buf2);
							}
							temp_buf.CatChar(':');
							P_Fr->IdentifyHouse(FiasStreetID, temp_buf, &FiasHouseID);
						}
					}
				}
				else {
					if(!is_there_city) {
						//
						// Если город не определен, то пытаемся из неиспользованных текстовых токенов
						// извлечь наименование города
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
						// Если улица не определена, то первый текстовый токен засчитываем как наименование улицы
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

int SLAPI PPLocAddrStruc::Output(SString & rBuf)
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

int SLAPI PPLocAddrStruc::OutputTokList(const TSCollection <AddrTok> & rList, SString & rBuf)
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
					uint idx = p_tok->Dl.get(j);
					AddrItemDescr & r_aid = Aidl[idx];
					if(r_aid.MainId) {
						for(uint k = 0; k < SIZEOFARRAY(Aidl); k++) {
							if(Aidl[k].Id == r_aid.MainId) {
								rBuf.Cat(Aidl[k].P_Descr);
							}
						}
					}
					else {
						rBuf.Cat(r_aid.P_Descr);
					}
					if((j+1) < p_tok->Dl.getCount())
						rBuf.Semicol();
				}
				rBuf.CatChar(']');
			}
		}
	}
	return 1;
}

int SLAPI PPLocAddrStruc::Recognize(const char * pText, TSCollection <AddrTok> & rTokList) // @debug
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
	(file_name = path).SetLastSlash().Cat("data").SetLastSlash().Cat("address-land.csv");
	SFile in_file(file_name, SFile::mRead);
	(file_name = path).SetLastSlash().Cat("data").SetLastSlash().Cat("address-land.out");
	SFile out_file(file_name, SFile::mWrite);
	if(in_file.IsValid()) {
		if(DS.GetConstTLA().IsAuth()) {
			p_fr = new PPFiasReference;
		}
		PPLocAddrStruc las(0, p_fr);
		SString line_buf, out_buf, temp_buf;
		while(in_file.ReadLine(line_buf)) {
			out_buf.Z().Cat(line_buf).Cat("-->");
			/*
			TSCollection <AddrTok> tok_list;
			las.Recognize(line_buf.Chomp(), tok_list);
			las.OutputTokList(tok_list, out_buf);
			*/
			las.Recognize(line_buf.Chomp());
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
public:
	AddrStrucDialog() : TDialog(DLG_ADDRSTRUC), Las(0, &Fr)
	{
		SString text;
		getLabelText(CTL_ADDRSTRUC_CITY, text.Z());
		OrgLabels.Add(Las.tCityKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_LOCALAREA, text.Z());
		OrgLabels.Add(Las.tLocalAreaKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_STREET, text.Z());
		OrgLabels.Add(Las.tStreetKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_HOUSE, text.Z());
		OrgLabels.Add(Las.tHouseKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_HOUSEADD, text.Z());
		OrgLabels.Add(Las.tHouseAddendumKind, text.Transf(CTRANSF_INNER_TO_OUTER));
		getLabelText(CTL_ADDRSTRUC_APART, text.Z());
		OrgLabels.Add(Las.tApartKind, text.Transf(CTRANSF_INNER_TO_OUTER));
	}
	int    setDTS(const SString * pStr)
	{
		if(!RVALUEPTR(SrcLine, pStr))
			SrcLine = 0;
		setCtrlString(CTL_ADDRSTRUC_SRC, SrcLine);
		Setup();
		return 1;
	}
	int    getDTS(SString * pData)
	{
		ASSIGN_PTR(pData, SrcLine);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_ADDRSTRUC_SRC)) {
				SString preserve_buf = SrcLine;
				getCtrlString(CTL_ADDRSTRUC_SRC, SrcLine);
				if(SrcLine.CmpNC(preserve_buf) != 0)
					Setup();
			}
			else
				return;
		}
	}
	void   Setup()
	{
		SString text, kind_text;
		(text = SrcLine).Transf(CTRANSF_INNER_TO_OUTER);
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
	SString SrcLine;
	StrAssocArray OrgLabels;
	PPFiasReference Fr;
	PPLocAddrStruc Las;
};

int SLAPI PPObjLocation::EditAddrStruc(SString & rAddr)
{
	return PPDialogProcBody <AddrStrucDialog, SString> (&rAddr);
}

int SLAPI PPObjLocation::IndexPhones(PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	SString phone, main_city_prefix, city_prefix, temp_buf;
	PPObjID objid;
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
				objid.Set(PPOBJ_LOCATION, P_Tbl->data.ID);
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
	UhttLocationBlock()
	{
		Clear();
	}
	void Clear()
	{
		MEMSZERO(Rec);
		State = stFetch;
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
	delete (UhttLocationBlock *)Extra[0].Ptr;
}

int PPALDD_UhttLocation::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttLocationBlock & r_blk = *(UhttLocationBlock *)Extra[0].Ptr;
	r_blk.Clear();
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
	UhttLocationBlock & r_blk = *(UhttLocationBlock *)Extra[0].Ptr;
	if(r_blk.State != UhttLocationBlock::stSet) {
		r_blk.Clear();
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
		/* Проверка прав не дает возможности сформировать заказ анонимному пользователю
		if(r_blk.Rec.Flags & LOCF_STANDALONE && r_blk.Rec.Type == LOCTYP_ADDRESS) {
			PPGlobalAccRights rb(PPTAG_GUA_SALOCRIGHTS);
			THROW((id == 0) ? rb.IsAllow(PPGlobalAccRights::fCreate) : rb.IsAllow(PPGlobalAccRights::fEdit));
		}
		*/
		// } @v8.3.2
		THROW(r_blk.LObj.PutRecord(&id, &r_blk.Rec, 1));
		Extra[4].Ptr = (void *)id;
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
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
	UhttStoreBlock()
	{
		Clear();
	}
	void Clear()
	{
		Pack.destroy();
		TagPos = 0;
		State = stFetch;
	}
	PPObjUhttStore     UhttStoreObj;
	PPObjTag           TagObj;
	PPUhttStorePacket  Pack;
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
	delete (UhttStoreBlock *)Extra[0].Ptr;
}

int PPALDD_UhttStore::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttStoreBlock & r_blk = *(UhttStoreBlock *)Extra[0].Ptr;
	r_blk.Clear();
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
	UhttStoreBlock & r_blk = *(UhttStoreBlock *)Extra[0].Ptr;
	if(iterId == GetIterID("iter@TagList"))
		r_blk.TagPos = 0;
	return 1;
}

int PPALDD_UhttStore::NextIteration(long iterId)
{
	int     ok = -1;
	SString temp_buf;
	IterProlog(iterId, 0);
	UhttStoreBlock & r_blk = *(UhttStoreBlock *)Extra[0].Ptr;
	if(iterId == GetIterID("iter@TagList")) {
		if(r_blk.TagPos < r_blk.Pack.TagL.GetCount()) {
			MEMSZERO(I_TagList);
			const ObjTagItem * p_item = r_blk.Pack.TagL.GetItemByPos(r_blk.TagPos);
			I_TagList.TagTypeID = p_item->TagDataType;
			{
				PPObjectTag rec;
				if(r_blk.TagObj.Fetch(p_item->TagID, &rec) > 0)
					STRNSCPY(I_TagList.TagSymb, rec.Symb);
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
	UhttStoreBlock & r_blk = *(UhttStoreBlock *)Extra[0].Ptr;
	if(r_blk.State != UhttStoreBlock::stSet) {
		r_blk.Clear();
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
			PPObjectTag rec;
			if(r_blk.TagObj.SearchBySymb(I_TagList.TagSymb, &id, &rec) > 0) {
				item.TagID = rec.ID;
				switch(I_TagList.TagTypeID) {
					case OTTYP_STRING:
					case OTTYP_GUID: THROW(item.SetStr(I_TagList.TagTypeID, I_TagList.StrVal)); break;
					case OTTYP_NUMBER: THROW(item.SetReal(I_TagList.TagTypeID, I_TagList.RealVal)); break;
					case OTTYP_INT: THROW(item.SetInt(I_TagList.TagTypeID, I_TagList.IntVal)); break;
					case OTTYP_DATE: THROW(item.SetDate(I_TagList.TagTypeID, I_TagList.DateVal)); break;
				}
				THROW(r_blk.Pack.TagL.PutItem(rec.ID, &item));
			}
		}
	}
	else {
		PPID  id = r_blk.Pack.Rec.ID;
		THROW(r_blk.UhttStoreObj.PutPacket(&id, &r_blk.Pack, 1));
		Extra[4].Ptr = (void *)id;
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
//
//
//
//static
int PPFiasReference::IdentifyShortDescription(const char * pText, int * pLevel, SString * pFullText)
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

SLAPI PPFiasReference::PPFiasReference()
{
}

SLAPI PPFiasReference::~PPFiasReference()
{
}

int SLAPI PPFiasReference::SearchObjByID(PPID id, FiasAddrObjTbl::Rec * pRec, int useCache)
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

int SLAPI PPFiasReference::SearchObjByUUID(const S_GUID & rUuid, FiasAddrObjTbl::Rec * pRec)
{
	return FT.SearchAddrByUUID(rUuid, pRec);
}

int SLAPI PPFiasReference::SearchHouseByID(PPID id, FiasHouseObjTbl::Rec * pRec)
{
	return FT.SearchHouse(id, pRec);
}

int SLAPI PPFiasReference::SearchHouseByUUID(const S_GUID & rUuid, FiasHouseObjTbl::Rec * pRec)
{
	return FT.SearchHouseByUUID(rUuid, pRec);
}

int SLAPI PPFiasReference::GetText(PPID textRef, SString & rBuf)
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

int SLAPI PPFiasReference::Helper_GetHierarchy(PPID id, long flags, FiasHouseObjTbl::Rec * pHseRec, TSArray <FiasAddrObjTbl::Rec> & rList, long * pZip)
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

uint SLAPI PPFiasReference::IsObjInHierarchy(PPID objID, const TSArray <FiasAddrObjTbl::Rec> & rList) const
{
	uint   result = 0;
	for(uint p = 0; !result && p < rList.getCount(); p++) {
		if(rList.at(p).IdUuRef == objID)
			result = p+1;
	}
	return result;
}

int SLAPI PPFiasReference::Match(PPID obj1ID, PPID obj2ID, int vect)
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
						ok = -1; // два не эквивалентных дома не могут соответствовать один-другому
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
		else if(vect < 0) { // obj1ID заведомо на уровне ниже obj2ID
			hr1 = Helper_GetHierarchy(obj1ID, matfTryHouse, &hse1_rec, list1, &zip1);
			if(hr1 > 0 && IsObjInHierarchy(obj2ID, list1))
				ok = 1; // obj1ID belong to obj2ID
		}
		else { // if(vect > 0) { // obj1ID заведомо на уровне выше obj2ID
			hr2 = Helper_GetHierarchy(obj2ID, matfTryHouse, &hse2_rec, list2, &zip2);
			if(hr2 > 0 && IsObjInHierarchy(obj1ID, list2))
				ok = 2; // obj2ID belong to obj1ID
		}
	}
    return ok;
}

int SLAPI PPFiasReference::IdentifyHouse(PPID terminalObjID, const char * pHouseCode, PPID * pHouseID)
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
    for(q.initIteration(0, &k1, spEq); q.nextIteration() > 0;) {
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

int SLAPI PPFiasReference::GetRandomHouse(long extValue, PPID terminalObjID, PPID * pHouseID)
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
    for(q.initIteration(0, &k1, spEq); q.nextIteration() > 0;) {
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

int SLAPI PPFiasReference::GetRandomAddress(long extValue, PPID cityID, PPID * pStreetID, PPID * pHouseID)
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
					ulong _pos = (extValue > 0) ? (ulong)(extValue % _sc) : (r_stla.Rg.Get() % _sc);
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

int SLAPI PPFiasReference::MakeAddressText(PPID terminalID, long flags, SString & rBuf)
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

int SLAPI PPFiasReference::SearchObjByTextRefList(const TSVector <TextRefIdent> & rTRefList, PPIDArray & rList) // @v9.8.4 TSArray-->TSVector
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
			for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;) {
				FiasAddrObjTbl::Rec rec;
				r_t.copyBufTo(&rec);
				rList.add(rec.IdUuRef);
				ok = 1;
			}
		}
		else {
			if(r_t.search(3, &k3, spGe) && r_t.data.NameTRef == r_i.O.Id) do {
				FiasAddrObjTbl::Rec rec;
				r_t.copyBufTo(&rec);
				rList.add(rec.IdUuRef);
				ok = 1;
			} while(r_t.search(3, &k3, spNext) && r_t.data.NameTRef == r_i.O.Id);
		}
	}
	rList.sortAndUndup();
	return ok;
}

int SLAPI PPFiasReference::SearchObjByText(const char * pText, long flags, PPID upperID, PPIDArray & rList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    do_post_process = 0;
    SStringU pattern;
    SString temp_buf;
    SString pattern_mb = pText;
    if(!(flags & stfAnsiInput))
		pattern_mb.Transf(CTRANSF_INNER_TO_OUTER);
    pattern_mb.ToLower1251();
    pattern.CopyFromUtf8((temp_buf = pattern_mb).ToUtf8());
	TSVector <TextRefIdent> text_ref_list; // @v9.8.4 TSArray-->TSVector
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
				const PPID _id = rList.get(i);
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
		TestDialog(TestFiasProcessor * pPrcssr) : TDialog(DLG_TESTFIAS), P(pPrcssr)
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

int SLAPI TestFias()
{
	TestFiasProcessor prcssr;
	return prcssr.Run();
}
