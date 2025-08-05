// MARKETPLACE.CPP
// Copyright (c) A.Sobolev 2024, 2025
// @codepage UTF-8
// @construction 
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(MarketplaceGoodsSelection); MarketplaceGoodsSelectionFilt::MarketplaceGoodsSelectionFilt() : 
	PPBaseFilt(PPFILT_MARKETPLACEGOODSSELECTION, 0, 0)
{
	SetFlatChunk(offsetof(MarketplaceGoodsSelectionFilt, ReserveStart),
		offsetof(MarketplaceGoodsSelectionFilt, Reserve) + sizeof(Reserve) - offsetof(MarketplaceGoodsSelectionFilt, ReserveStart));
	SetBranchSString(offsetof(MarketplaceGoodsSelectionFilt, SearchPatternUtf8));
	Init(1, 0);
}

PPMarketplaceInterface::PPMarketplaceInterface(const char * pSymbol, PrcssrMarketplaceInterchange & rPrc) : 
	P_Symbol(pSymbol), State(0), R_Prc(rPrc)
{
}

/*virtual*/PPMarketplaceInterface::~PPMarketplaceInterface()
{
}

int PPMarketplaceInterface::CreateWarehouseFolder(PPID * pID, int use_ta)
{
	int    ok = 0;
	PPID   parent_id = 0; // Идентификатор папки для складов маркетплейса.
	PPObjLocation & r_loc_obj = PsnObj.LocObj;
	THROW(!isempty(GetSymbol())); // @todo @err
	{
		PPID   local_id = 0;
		LocationTbl::Rec loc_rec;
		SString name_buf;
		name_buf.Z().Cat("MP").CatChar('.').Cat(GetSymbol()).ToUpper();
		if(r_loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSEGROUP, name_buf, &local_id, &loc_rec) > 0) {
			parent_id = local_id;
			ok = -1;
		}
		else {
			PPLocationConfig loc_cfg;
			LocationTbl::Rec mp_loc_folder_rec;
			r_loc_obj.ReadConfig(&loc_cfg);
			PPLocationPacket folder_loc_pack;
			folder_loc_pack.Type = LOCTYP_WAREHOUSEGROUP;
			STRNSCPY(folder_loc_pack.Code, name_buf);
			STRNSCPY(folder_loc_pack.Name, name_buf);
			if(loc_cfg.MarketplaceWarehoustFolderID && r_loc_obj.Fetch(loc_cfg.MarketplaceWarehoustFolderID, &mp_loc_folder_rec) > 0 && mp_loc_folder_rec.Type == LOCTYP_WAREHOUSEGROUP) {
				folder_loc_pack.ParentID = loc_cfg.MarketplaceWarehoustFolderID;
			}
			THROW(r_loc_obj.PutPacket(&parent_id, &folder_loc_pack, use_ta));
			ok = 1;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, parent_id);
	return ok;
}
	
int PPMarketplaceInterface::GetMarketplacePerson(PPID * pID, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  PPID person_kind = PPPRK_MARKETPLACE;
	SString temp_buf;
	PPObjPersonKind pk_obj;
	PPPersonKind pk_rec;
	THROW(pk_obj.Fetch(person_kind, &pk_rec) > 0); // @todo @err (этот вид персоналий должен быть создан вызовом функции создания зарезервированных объектов)
	{
		PPID   acs_id = PrcssrMarketplaceInterchange::GetMarketplaceAccSheetID();
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			// Кроме прочего, нам необходимо убедиться, что существует таблица аналитических статей, ассоциированная с видом персоналий PPPRK_MARKETPLACE
			PPObjAccSheet acs_obj;
			PPAccSheet2 acs_rec;
			if(!acs_id) {
				MEMSZERO(acs_rec);
				STRNSCPY(acs_rec.Name, pk_rec.Name);
				acs_rec.Assoc = PPOBJ_PERSON;
				acs_rec.ObjGroup = person_kind;
				acs_rec.Flags = ACSHF_AUTOCREATART;
				THROW(p_ref->AddItem(PPOBJ_ACCSHEET, &acs_id, &acs_rec, 0));
			}
		}
		if(!isempty(P_Symbol)) {
			PPIDArray found_list;
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			THROW(tag_obj.Fetch(PPTAG_PERSON_MARKETPLACESYMB, &tag_rec) > 0); // @todo @err
			if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, PPTAG_PERSON_MARKETPLACESYMB, P_Symbol, &found_list) > 0) {
				for(uint i = 0; ok < 0 && i < found_list.getCount(); i++) {
					const PPID psn_id = found_list.get(i);
					PersonTbl::Rec psn_rec;
					if(PsnObj.Search(psn_id, &psn_rec) > 0) {
						if(PsnObj.P_Tbl->IsBelongsToKind(psn_id, person_kind)) {
							ASSIGN_PTR(pID, psn_id);
							ok = 1;
						}
					}
				}
			}
			if(ok < 0) {
				PPPersonPacket psn_pack;
				(temp_buf = "Marketplace").CatDiv('-', 1).Cat(P_Symbol);
				psn_pack.Rec.Status = PPPRS_LEGAL;
				STRNSCPY(psn_pack.Rec.Name, temp_buf);
				psn_pack.Kinds.add(person_kind);
				THROW(psn_pack.TagL.PutItemStr(PPTAG_PERSON_MARKETPLACESYMB, P_Symbol));
				THROW(PsnObj.PutPacket(pID, &psn_pack, 0));
				ok = 2;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface::Init(PPID guaID)
{
	int    ok = 1;
	/*
	PPObjGlobalUserAcc gua_obj;
	if(gua_obj.GetPacket(guaID, &GuaPack) > 0) {
		; // ok
	}
	else
		ok = 0;
	*/
	return ok;
}
//
//
//
PPMarketplaceInterface_Wildberries::FaultStatusResult::FaultStatusResult() : Status(0), Timestamp(ZERODATETIME)
{
}
	
PPMarketplaceInterface_Wildberries::FaultStatusResult & PPMarketplaceInterface_Wildberries::FaultStatusResult::Z()
{
	Status = 0;
	Timestamp = ZERODATETIME;
	StatusText.Z();
	Title.Z();
	Detail.Z();
	Code.Z();
	ReqId.Z();
	Origin.Z();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::FaultStatusResult::FromJson(const SJson * pJs)
{
	bool   ok = false;
	Z();
	/*
		{
			"title": "unauthorized",
			"detail": "token expired; Manage tokens at https://seller.wildberries.ru/supplier-settings/access-to-api",
			"code": "167251cb759b cc73ad3617b9d5c82732 456760352e0e-60",
			"requestId": "0918c63512ce3d0e2c95d6af649b5fab",
			"origin": "s2s-api-auth-supplies",
			"status": 401,
			"statusText": "Unauthorized",
			"timestamp": "2025-03-09T16:54:13Z"
		}
	*/
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("status")) {
				Status = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("statusText")) {
				(StatusText = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("title")) {
				(Title = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("detail")) {
				(Detail = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("code")) {
				(Code = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("requestId")) {
				(ReqId = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("origin")) {
				(Origin = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("timestamp")) {
				strtodatetime(p_cur->P_Child->Text, &Timestamp, DATF_ISO8601CENT, 0);
			}
		}
		if(Status != 0 && StatusText.NotEmpty()) {
			ok = true;
		}
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Warehouse::Warehouse() : ID(0), Flags(0), UedGeoLoc(0ULL), CargoType(0), DeliveryType(0)
{
}
		
PPMarketplaceInterface_Wildberries::Warehouse & PPMarketplaceInterface_Wildberries::Warehouse::Z()
{
	ID = 0;
	Flags = 0;
	UedGeoLoc = 0ULL;
	CargoType = 0;
	DeliveryType = 0;
	Name.Z();
	City.Z();
	Address.Z();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::Warehouse::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		SGeoPosLL geoloc;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("ID")) {
				ID = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				Name = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("city")) {
				City = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("address")) {
				Address = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("longitude")) {
				geoloc.Lon = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("latitude")) {
				geoloc.Lat = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("cargoType")) {
				CargoType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("deliveryType")) {
				DeliveryType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("workTime")) {
				;
			}
			else if(p_cur->Text.IsEqiAscii("acceptsQR")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1)
					Flags |= fAcceptsQR;
			}
			else if(p_cur->Text.IsEqiAscii("selected")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1)
					Flags |= fSelected;
			}
		}
		UedGeoLoc = UED::SetRaw_GeoLoc(geoloc);
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::ProductCategory::ProductCategory() : ID(0), ParentID(0), Flags(0)
{
}

PPMarketplaceInterface_Wildberries::ProductCategory & PPMarketplaceInterface_Wildberries::ProductCategory::Z()
{
	ID = 0;
	ParentID = 0;
	Flags = 0;
	Name.Z();
	SubjectList.clear();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::ProductCategory::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		enum {
			ocfName         = 0x0001,
			ocfSubjectName  = 0x0002,
			ocfParentName   = 0x0004,
			ocfID           = 0x0008,
			ocfSubjectID    = 0x0010,
			ocfParentID     = 0x0020,
			ocfIsVisible    = 0x0040,
		};

		uint occured_field = 0;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("name")) {
				(Name = p_cur->P_Child->Text).Unescape();
				occured_field |= ocfName;
			}
			else if(p_cur->Text.IsEqiAscii("subjectName")) {
				(Name = p_cur->P_Child->Text).Unescape();
				occured_field |= ocfSubjectName;
			}
			else if(p_cur->Text.IsEqiAscii("parentName")) {
				occured_field |= ocfParentName;
			}
			else if(p_cur->Text.IsEqiAscii("id")) {
				ID = p_cur->P_Child->Text.ToInt64();
				occured_field |= ocfID;
			}
			else if(p_cur->Text.IsEqiAscii("subjectID")) {
				ID = p_cur->P_Child->Text.ToInt64();
				occured_field |= ocfSubjectID;
			}
			else if(p_cur->Text.IsEqiAscii("parentID")) {
				ParentID = p_cur->P_Child->Text.ToInt64();
				occured_field |= ocfParentID;
			}
			else if(p_cur->Text.IsEqiAscii("isVisible")) {
				occured_field |= ocfIsVisible;
				if(SJson::IsTrue(p_cur->P_Child)) {
					Flags |= fVisible;
				}
				else {
					Flags &= ~fVisible;
				}
			}
		}
		if(ID > 0 && !Name.IsEmpty())
			ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::CategoryPool::Entry::Entry() : ID(0), ParentID(0), Flags(0), NameP(0), UrlP(0), ShardP(0), QueryP(0), SeoP(0), SnippetP(0), SearchQueryP(0), P_DestList(0)
{
}
			
PPMarketplaceInterface_Wildberries::CategoryPool::Entry::~Entry()
{
	ZDELETE(P_DestList);
}

PPMarketplaceInterface_Wildberries::CategoryPool::Entry & PPMarketplaceInterface_Wildberries::CategoryPool::Entry::Z()
{
	ID       = 0;
	ParentID = 0;
	Flags    = 0;
	NameP    = 0;
	UrlP     = 0;
	ShardP   = 0;
	QueryP   = 0;
	SeoP     = 0;
	SnippetP = 0;
	SearchQueryP = 0;
	Children.freeAll();
	ZDELETE(P_DestList);
	return *this;
}
			
PPMarketplaceInterface_Wildberries::CategoryPool::Entry & FASTCALL PPMarketplaceInterface_Wildberries::CategoryPool::Entry::operator = (const PPMarketplaceInterface_Wildberries::CategoryPool::Entry & rS)
{
	Copy(rS);
	return *this;
}
			
bool FASTCALL PPMarketplaceInterface_Wildberries::CategoryPool::Entry::Copy(const PPMarketplaceInterface_Wildberries::CategoryPool::Entry & rS)
{
	ID       = rS.ID;
	ParentID = rS.ParentID;
	Flags    = rS.Flags;
	NameP    = rS.NameP;
	UrlP     = rS.UrlP;
	ShardP   = rS.ShardP;
	QueryP   = rS.QueryP;
	SeoP     = rS.SeoP;
	SnippetP = rS.SnippetP;
	SearchQueryP = rS.SearchQueryP;
	TSCollection_Copy(Children, rS.Children);
	if(rS.P_DestList) {
		if(P_DestList) {
			*P_DestList = *rS.P_DestList;
		}
		else {
			P_DestList = new Int64Array(*rS.P_DestList);
		}
	}
	else {
		ZDELETE(P_DestList);
	}
	return true;
}
			
bool PPMarketplaceInterface_Wildberries::CategoryPool::Entry::FromJsonObj(CategoryPool & rPool, const SJson * pJs)
{
	bool   ok = true;
	if(SJson::IsObject(pJs)) {
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("id")) {
				ID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("parent")) {
				ParentID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &NameP);
			}
			else if(p_cur->Text.IsEqiAscii("url")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &UrlP);
			}
			else if(p_cur->Text.IsEqiAscii("shard")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &ShardP);
			}
			else if(p_cur->Text.IsEqiAscii("query")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &QueryP);
			}
			else if(p_cur->Text.IsEqiAscii("seo")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &SeoP);
			}
			else if(p_cur->Text.IsEqiAscii("snippet")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &SnippetP);
			}
			else if(p_cur->Text.IsEqiAscii("searchQuery")) {
				(temp_buf = p_cur->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &SearchQueryP);
			}
			else if(p_cur->Text.IsEqiAscii("isDenyLink")) {
				SETFLAG(Flags, fIsDenyLink, p_cur->P_Child->IsTrue());
			}
			else if(p_cur->Text.IsEqiAscii("dynamic")) {
				SETFLAG(Flags, fIsDynamic, p_cur->P_Child->IsTrue());
			}
			else if(p_cur->Text.IsEqiAscii("dest")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsNumber(p_js_item)) {
							SETIFZ(P_DestList, new Int64Array());
							int64 dest_item = p_js_item->Text.ToInt64();
							P_DestList->add(dest_item);
						}
					}
				}
			}
			else if(p_cur->Text.IsEqiAscii("childs")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsObject(p_js_item)) {
							Entry * p_new_child = new Entry;
							if(p_new_child && p_new_child->FromJsonObj(rPool, p_js_item)) {
								Children.insert(p_new_child);
							}
							else {
								delete p_new_child;
							}
						}
					}
				}
			}
			else {
				assert(0); // @debug
			}
		}
	}
	else
		ok = false;
	return ok;
}

PPMarketplaceInterface_Wildberries::CategoryPool::CategoryPool()
{
}

PPMarketplaceInterface_Wildberries::CategoryPool & PPMarketplaceInterface_Wildberries::CategoryPool::Z()
{
	SStrGroup::ClearS();
	L.freeAll();
	return *this;
}

const PPMarketplaceInterface_Wildberries::CategoryPool::Entry * PPMarketplaceInterface_Wildberries::CategoryPool::Helper_GetByID(const TSCollection <Entry> & rSrcList, int64 id) const
{
	const Entry * p_result = 0;
	if(id) {
		for(uint i = 0; !p_result && i < rSrcList.getCount(); i++) {
			const Entry * p_entry = rSrcList.at(i);
			if(p_entry) {
				if(p_entry->ID == id) {
					p_result = p_entry;
				}
				else if(p_entry->Children.getCount()) {
					p_result = Helper_GetByID(p_entry->Children, id); // @recursion
				}
			}
		}
	}
	return p_result;
}

const PPMarketplaceInterface_Wildberries::CategoryPool::Entry * PPMarketplaceInterface_Wildberries::CategoryPool::GetByID(int64 id) const
{
	return Helper_GetByID(L, id);
}

int PPMarketplaceInterface_Wildberries::CategoryPool::Helper_MakeShardList(const TSCollection <Entry> & rSrcList, long parentID, StrAssocArray & rList) const
{
	int    ok = 1;
	SString temp_buf;
	SString name_buf;
	for(uint i = 0; i < rSrcList.getCount(); i++) {
		const Entry * p_entry = rSrcList.at(i);
		if(p_entry) {
			assert(parentID == static_cast<long>(p_entry->ParentID));
			const bool has_children = p_entry->Children.getCount();
			bool  do_insert = false;
			GetS(p_entry->SeoP, name_buf);
			if(name_buf.IsEmpty())
				GetS(p_entry->NameP, name_buf);
			if(name_buf.NotEmpty()) {
				name_buf.Transf(CTRANSF_UTF8_TO_INNER);
				GetS(p_entry->ShardP, temp_buf);
				do_insert = (has_children || (temp_buf.NotEmpty() && !temp_buf.IsEqiAscii("blackhole")));
			}
			if(do_insert) {
				rList.Add(static_cast<long>(p_entry->ID), parentID, name_buf);
				if(has_children) {
					THROW(Helper_MakeShardList(p_entry->Children, static_cast<long>(p_entry->ID), rList)); // @recursion
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::CategoryPool::MakeShardList(StrAssocArray & rList) const
{
	return Helper_MakeShardList(L, 0, rList);
}

bool PPMarketplaceInterface_Wildberries::CategoryPool::FromJson(const SJson * pJs)
{
	bool   ok = true;
	THROW(SJson::IsArray(pJs));
	{
		for(const SJson * p_js_item = pJs->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
			Entry * p_new_entry = new Entry();
			THROW_SL(p_new_entry);
			if(p_new_entry->FromJsonObj(*this, p_js_item)) {
				L.insert(p_new_entry);
			}
			else {
				delete p_new_entry;
			}
		}		
	}
	CATCHZOK
	return ok;
}
//
//
//
PPMarketplaceInterface_Wildberries::PublicWarePool::Size::Size() :
	Wh(0), NameP(0), OrigNameP(0), Rank(0.0f), OptionId(0), Price100_Basic(0), Price100_Product(0), Price100_Logistics(0), Price100_Return(0), SaleConditions(0)
{
}

PPMarketplaceInterface_Wildberries::PublicWarePool::Size & PPMarketplaceInterface_Wildberries::PublicWarePool::Size::Z()
{
	Wh = 0;
	NameP = 0;
	OrigNameP = 0;
	Rank = 0.0f;
	OptionId = 0;
	Price100_Basic = 0;
	Price100_Product = 0;
	Price100_Logistics = 0;
	Price100_Return = 0;
	SaleConditions = 0;
	return *this;
}

bool PPMarketplaceInterface_Wildberries::PublicWarePool::Size::FromJsonObj(PublicWarePool & rPool, const SJson * pJs)
{
	bool   ok = true;
	if(SJson::IsObject(pJs)) {
		SString temp_buf;
		for(const SJson * p_f = pJs->P_Child; p_f; p_f = p_f->P_Next) {
			if(p_f->Text.IsEqiAscii("name")) {
				(temp_buf = p_f->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &NameP);
			}
			else if(p_f->Text.IsEqiAscii("origName")) {
				(temp_buf = p_f->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &OrigNameP);
			}
			else if(p_f->Text.IsEqiAscii("rank")) {
				Rank = p_f->P_Child->Text.ToFloat();
			}
			else if(p_f->Text.IsEqiAscii("optionId")) {
				OptionId = p_f->P_Child->Text.ToInt64();
			}
			else if(p_f->Text.IsEqiAscii("wh")) {
				Wh = p_f->P_Child->Text.ToLong();
			}
			else if(p_f->Text.IsEqiAscii("time1")) {
			}
			else if(p_f->Text.IsEqiAscii("time2")) {
			}
			else if(p_f->Text.IsEqiAscii("dtype")) {
			}
			else if(p_f->Text.IsEqiAscii("price")) {
				if(SJson::IsObject(p_f->P_Child)) {
					for(const SJson * p_jsn = p_f->P_Child->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
						if(p_jsn->Text.IsEqiAscii("basic")) {
							Price100_Basic = p_jsn->P_Child->Text.ToULong();
						}
						else if(p_jsn->Text.IsEqiAscii("product")) {
							Price100_Product = p_jsn->P_Child->Text.ToULong();
						}
						else if(p_jsn->Text.IsEqiAscii("logistics")) {
							Price100_Logistics = p_jsn->P_Child->Text.ToULong();
						}
						else if(p_jsn->Text.IsEqiAscii("return")) {
							Price100_Return = p_jsn->P_Child->Text.ToULong();
						}
					}
				}
			}
			else if(p_f->Text.IsEqiAscii("saleConditions")) {
				SaleConditions = p_f->P_Child->Text.ToUInt64();
			}
			else if(p_f->Text.IsEqiAscii("payload")) {
			}
		}
	}
	else
		ok = false;
	return ok;
}

PPMarketplaceInterface_Wildberries::PublicWarePool::Entry::Entry()
{
	Z();
}

PPMarketplaceInterface_Wildberries::PublicWarePool::Entry & PPMarketplaceInterface_Wildberries::PublicWarePool::Entry::Z()
{
	ID = 0;
	Wh = 0;
	BrandID = 0;
	SupplID = 0;
	SubjId = 0;
	SubjParentId = 0;
	SupplRating = 0.0f;
	SupplFlags = 0;
	PicsCount = 0;
	Rating = 0.0f;
	ReviewRating = 0.0f;
	NmReviewRating = 0.0f;
	FeedbackCount = 0;
	NmFeedbackCount = 0;
	FeedbackPointCount = 0;
	TotalStock = 0.0;
	BrandP = 0;
	NameP = 0;
	EntityP = 0;
	SupplP = 0;
	SizeL.clear();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::PublicWarePool::Entry::FromJsonObj(PublicWarePool & rPool, const SJson * pJs)
{
	bool   ok = true;
	if(SJson::IsObject(pJs)) {
		SString temp_buf;
		for(const SJson * p_f = pJs->P_Child; p_f; p_f = p_f->P_Next) {
			if(p_f->Text.IsEqiAscii("id")) {
				ID = p_f->P_Child->Text.ToInt64();
			}
			else if(p_f->Text.IsEqiAscii("__sort")) {
			}
			else if(p_f->Text.IsEqiAscii("ksort")) {
			}
			else if(p_f->Text.IsEqiAscii("time1")) {
			}
			else if(p_f->Text.IsEqiAscii("time2")) {
			}
			else if(p_f->Text.IsEqiAscii("wh")) {
				Wh = p_f->P_Child->Text.ToLong();
			}
			else if(p_f->Text.IsEqiAscii("dtype")) {
			}
			else if(p_f->Text.IsEqiAscii("dist")) {
			}
			else if(p_f->Text.IsEqiAscii("root")) {
			}
			else if(p_f->Text.IsEqiAscii("kindId")) {
			}
			else if(p_f->Text.IsEqiAscii("brand")) {
				(temp_buf = p_f->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &BrandP);
			}
			else if(p_f->Text.IsEqiAscii("brandId")) {
				BrandID = p_f->P_Child->Text.ToInt64();
			}
			else if(p_f->Text.IsEqiAscii("siteBrandId")) {
			}
			else if(p_f->Text.IsEqiAscii("colors")) {
				if(SJson::IsArray(p_f->P_Child)) {
				}
			}
			else if(p_f->Text.IsEqiAscii("subjectId")) {
				SubjId = p_f->P_Child->Text.ToLong();
			}
			else if(p_f->Text.IsEqiAscii("subjectParentId")) {
				SubjParentId = p_f->P_Child->Text.ToLong();
			}
			else if(p_f->Text.IsEqiAscii("name")) {
				(temp_buf = p_f->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &NameP);
			}
			else if(p_f->Text.IsEqiAscii("entity")) {
				(temp_buf = p_f->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &EntityP);
			}
			else if(p_f->Text.IsEqiAscii("matchId")) {
			}
			else if(p_f->Text.IsEqiAscii("supplier")) {
				(temp_buf = p_f->P_Child->Text).Unescape();
				rPool.AddS(temp_buf, &SupplP);				
			}
			else if(p_f->Text.IsEqiAscii("supplierId")) {
				SupplID = p_f->P_Child->Text.ToLong();
			}
			else if(p_f->Text.IsEqiAscii("supplierRating")) {
				SupplRating = p_f->P_Child->Text.ToFloat();
			}
			else if(p_f->Text.IsEqiAscii("supplierFlags")) {
			}
			else if(p_f->Text.IsEqiAscii("pics")) {
				PicsCount = p_f->P_Child->Text.ToULong();
			}
			else if(p_f->Text.IsEqiAscii("rating")) {
				Rating = p_f->P_Child->Text.ToFloat();
			}
			else if(p_f->Text.IsEqiAscii("reviewRating")) {
				ReviewRating = p_f->P_Child->Text.ToFloat();
			}
			else if(p_f->Text.IsEqiAscii("nmReviewRating")) {
				NmReviewRating = p_f->P_Child->Text.ToFloat();
			}
			else if(p_f->Text.IsEqiAscii("feedbacks")) {
				FeedbackCount = p_f->P_Child->Text.ToULong();
			}
			else if(p_f->Text.IsEqiAscii("nmFeedbacks")) {
				NmFeedbackCount = p_f->P_Child->Text.ToULong();
			}
			else if(p_f->Text.IsEqiAscii("panelPromoId")) {
			}
			else if(p_f->Text.IsEqiAscii("volume")) {
			}
			else if(p_f->Text.IsEqiAscii("viewFlags")) {
			}
			else if(p_f->Text.IsEqiAscii("sizes")) {
				if(SJson::IsArray(p_f->P_Child)) {
					for(const SJson * p_js_item = p_f->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						Size new_size;
						if(new_size.FromJsonObj(rPool, p_js_item)) {
							SizeL.insert(&new_size);
						}
					}
				}
			}
			else if(p_f->Text.IsEqiAscii("totalQuantity")) {
				TotalStock = p_f->P_Child->Text.ToReal_Plain();
			}
			else if(p_f->Text.IsEqiAscii("logs")) {
			}
			else if(p_f->Text.IsEqiAscii("meta")) {
				if(SJson::IsObject(p_f->P_Child)) {
					// "tokens": []
				}
			}
		}
	}
	else
		ok = false;
	return ok;
}

PPMarketplaceInterface_Wildberries::PublicWarePool::PublicWarePool() : TotalCountOnServer(0)
{
}

PPMarketplaceInterface_Wildberries::PublicWarePool & PPMarketplaceInterface_Wildberries::PublicWarePool::Z()
{
	SStrGroup::ClearS();
	L.freeAll();
	TotalCountOnServer = 0;
	return *this;
}

PPMarketplaceInterface_Wildberries::PublicWarePool::Entry * PPMarketplaceInterface_Wildberries::PublicWarePool::GetEntry(uint i)
{
	return (i < L.getCount()) ? L.at(i) : 0;
}

const PPMarketplaceInterface_Wildberries::PublicWarePool::Entry * PPMarketplaceInterface_Wildberries::PublicWarePool::GetEntryC(uint i) const
{
	return (i < L.getCount()) ? L.at(i) : 0;
}

bool PPMarketplaceInterface_Wildberries::PublicWarePool::FromJson(const SJson * pJs, bool concatenate, uint * pReadCount, uint * pTotalCount)
{
	bool   ok = true;
	uint   read_count = 0;
	uint   total_count = 0;
	if(!concatenate)
		TotalCountOnServer = 0;
	THROW(SJson::IsObject(pJs));
	{
		for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->Text.IsEqiAscii("products")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_item = p_jsn->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						Entry * p_new_entry = new Entry();
						THROW_SL(p_new_entry);
						if(p_new_entry->FromJsonObj(*this, p_js_item)) {
							L.insert(p_new_entry);
							read_count++;
						}
						else {
							delete p_new_entry;
						}
					}
				}				
			}
			else if(p_jsn->Text.IsEqiAscii("total")) {
				total_count = p_jsn->P_Child->Text.ToULong();
			}
		}		
	}
	TotalCountOnServer = total_count;
	CATCHZOK
	ASSIGN_PTR(pReadCount, read_count);
	ASSIGN_PTR(pTotalCount, total_count);
	return ok;
}
//
//
//
PPMarketplaceInterface_Wildberries::WareBase::WareBase() : ID(0)
{
}
		
PPMarketplaceInterface_Wildberries::WareBase & PPMarketplaceInterface_Wildberries::WareBase::Z()
{
	ID = 0;
	Name.Z();
	TechSize.Z();
	SupplArticle.Z();
	Barcode.Z();
	Category.Z();
	Brand.Z();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::WareBase::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("supplierArticle"))
				(SupplArticle = p_cur->P_Child->Text).Unescape();
			else if(p_cur->Text.IsEqiAscii("nmId"))
				ID = p_cur->P_Child->Text.ToInt64();
			else if(p_cur->Text.IsEqiAscii("barcode"))
				(Barcode = p_cur->P_Child->Text).Unescape();
			else if(p_cur->Text.IsEqiAscii("category"))
				(Category = p_cur->P_Child->Text).Unescape();
			else if(p_cur->Text.IsEqiAscii("subject"))
				(Name = p_cur->P_Child->Text).Unescape();
			else if(p_cur->Text.IsEqiAscii("brand"))
				(Brand = p_cur->P_Child->Text).Unescape();
			else if(p_cur->Text.IsEqiAscii("techSize"))
				(TechSize = p_cur->P_Child->Text).Unescape();
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::WareOnPromotion::WareOnPromotion() : ID(0), UedCurrency(0), InAction(false), Price(0.0), Discount(0.0), PlanPrice(0.0), PlanDiscount(0.0)
{
}

PPMarketplaceInterface_Wildberries::WareOnPromotion & PPMarketplaceInterface_Wildberries::WareOnPromotion::Z()
{
	ID = 0;
	UedCurrency = 0;
	Price = 0.0;
	Discount = 0.0;
	PlanPrice = 0.0;
	PlanDiscount = 0.0;
	InAction = false;
	return *this;
}

bool PPMarketplaceInterface_Wildberries::WareOnPromotion::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("id")) {
				ID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("inAction")) {
				if(SJson::IsTrue(p_cur->P_Child)) {
					InAction = true;
				}
			}
			else if(p_cur->Text.IsEqiAscii("price")) {
				Price = p_cur->P_Child->Text.ToReal_Plain();
			}
			else if(p_cur->Text.IsEqiAscii("currencyCode")) {
			}
			else if(p_cur->Text.IsEqiAscii("planPrice"))
				PlanPrice = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("discount"))
				Discount = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("planDiscount"))
				PlanDiscount = p_cur->P_Child->Text.ToReal_Plain();
		}
		if(ID)
			ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Promotion::Promotion() : ID(0), Type(tUndef),
	InPromoActionLeftovers(0), InPromoActionTotal(0), NotInPromoActionLeftovers(0),
	NotInPromoActionTotal(0), ParticipationPercentage(0), ExceptionProductsCount(0)
{
}
		
PPMarketplaceInterface_Wildberries::Promotion & PPMarketplaceInterface_Wildberries::Promotion::Z()
{
	ID = 0;
	Type = tUndef;
	Name.Z();
	DtmRange.Z();
	//
	Description.Z();
	Advantages.Z();
	InPromoActionLeftovers = 0;
	InPromoActionTotal = 0;
	NotInPromoActionLeftovers = 0;
	NotInPromoActionTotal = 0;
	ParticipationPercentage = 0;
	ExceptionProductsCount = 0;
	RangingList.clear();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Promotion::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		/*
			{
				"id": 618,
				"name": "Пресейл: День шопинга (автоакция)",
				"startDateTime": "2024-10-20T23:00:00Z",
				"endDateTime": "2024-10-26T20:59:59Z",
				"type": "auto"
			},
		*/ 
		/*
			{
				"id": 725,
				"name": "Чёрная пятница: товары-хиты",
				"description": "Обратите внимание — это не новая акция, а специальный инструмент. С его помощью вы сможете прямо здесь отслеживать долю участия товаров с продажами в акции «Чёрная пятница».\n\nМы рекомендуем добавлять в распродажу продукцию, которая имеет спрос у покупателей. В период высокого сезона это особенно важно для роста продаж и продвижения товаров.\n",
				"advantages": [
					"Плашка на карточке товара",
					"Баннер на сайте",
					"Поднятие в поиске",
					"Красная цена",
					"Градусник"
				],
				"startDateTime": "2024-11-14T18:00:00Z",
				"endDateTime": "2024-12-01T20:59:59Z",
				"inPromoActionLeftovers": 0,
				"inPromoActionTotal": 0,
				"notInPromoActionLeftovers": 0,
				"notInPromoActionTotal": 0,
				"participationPercentage": 0,
				"type": "regular",
				"ranging": [
					{
						"condition": "productsInPromotion",
						"participationRate": 1,
						"boost": 30
					}
				]
			},
		*/ 
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("id"))
				ID = p_cur->P_Child->Text.ToInt64();
			else if(p_cur->Text.IsEqiAscii("name"))
				(Name = p_cur->P_Child->Text).Unescape();
			else if(p_cur->Text.IsEqiAscii("startDateTime")) {
				strtodatetime(p_cur->P_Child->Text, &DtmRange.Start, DATF_ISO8601CENT, TIMF_HMS);
			}
			else if(p_cur->Text.IsEqiAscii("endDateTime")) {
				strtodatetime(p_cur->P_Child->Text, &DtmRange.Finish, DATF_ISO8601CENT, TIMF_HMS);
			}
			else if(p_cur->Text.IsEqiAscii("type")) {
				if(p_cur->P_Child->Text.IsEqiUtf8("auto"))
					Type = tAuto;
				else if(p_cur->P_Child->Text.IsEqiUtf8("regular"))
					Type = tRegular;
			}
			else if(p_cur->Text.IsEqiAscii("description")) {
				(Description = p_cur->P_Child->Text).Unescape();
			}
			else if(p_cur->Text.IsEqiAscii("advantages")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(p_js_item->IsString()) {
							(temp_buf = p_js_item->Text).Unescape();
							if(temp_buf.NotEmptyS())
								Advantages.add(temp_buf);
						}
					}
				}
			}
			else if(p_cur->Text.IsEqiAscii("inPromoActionLeftovers")) {
				InPromoActionLeftovers = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("inPromoActionTotal")) {
				InPromoActionTotal = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("notInPromoActionLeftovers")) {
				NotInPromoActionLeftovers = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("notInPromoActionTotal")) {
				NotInPromoActionTotal = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("participationPercentage")) {
				ParticipationPercentage = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("ranging")) {
				// array
			}
		}
		if(ID != 0 && Name.NotEmpty())
			ok = true;
	}
	return ok;	
}

PPMarketplaceInterface_Wildberries::Campaign::Campaign() : ID(0), Type(0), Status(0), ChangeDtm(ZERODATETIME)
{
	ReqPeriod.Z();
}

PPMarketplaceInterface_Wildberries::CampaignStatisticsUnit::CampaignStatisticsUnit() : ViewCount(0), ClickCount(0), Atbs(0), OrderCount(0), CRate(0.0), Shks(0), Ctr(0.0), Cpc(0.0), CostAmount(0.0), OrderAmount(0.0)
{
}

PPMarketplaceInterface_Wildberries::CampaignStatisticsUnit & PPMarketplaceInterface_Wildberries::CampaignStatisticsUnit::Z()
{
	ViewCount = 0;
	ClickCount = 0;
	Atbs = 0;
	OrderCount = 0;
	CRate = 0.0;
	Shks = 0;
	Ctr = 0.0;
	Cpc = 0.0;
	CostAmount = 0.0;
	OrderAmount = 0.0;
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::CampaignStatisticsUnit::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("views")) {
				ViewCount = p_cur->P_Child->Text.ToULong();
			}
			else if(p_cur->Text.IsEqiAscii("clicks")) {
				ClickCount = p_cur->P_Child->Text.ToULong();
			}
			else if(p_cur->Text.IsEqiAscii("ctr")) {
				Ctr = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("cpc")) {
				Cpc = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("sum")) {
				CostAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("atbs")) {
				Atbs = p_cur->P_Child->Text.ToULong();
			}
			else if(p_cur->Text.IsEqiAscii("orders")) {
				OrderCount = p_cur->P_Child->Text.ToULong();
			}
			else if(p_cur->Text.IsEqiAscii("cr")) {
				CRate = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("shks")) {
				Shks = p_cur->P_Child->Text.ToULong();
			}
			else if(p_cur->Text.IsEqiAscii("sum_price")) {
				OrderAmount = p_cur->P_Child->Text.ToReal();
			}
		}
		ok = true;		
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::CampaignStatistics_Ware::CampaignStatistics_Ware() :  WareId(0)
{
}
		
PPMarketplaceInterface_Wildberries::CampaignStatistics_Ware & PPMarketplaceInterface_Wildberries::CampaignStatistics_Ware::Z()
{
	WareId = 0;
	WareName.Z();
	S.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::CampaignStatistics_Ware::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(S.FromJsonObj(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("nmId")) {
				WareId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				(WareName = p_cur->P_Child->Text).Unescape();
			}
		}
		if(WareId)
			ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::CampaignStatistics_App::CampaignStatistics_App() : AppType(0)
{
}
		
PPMarketplaceInterface_Wildberries::CampaignStatistics_App & PPMarketplaceInterface_Wildberries::CampaignStatistics_App::Z()
{
	AppType = 0;
	S.Z();
	SList_Ware.freeAll();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::CampaignStatistics_App::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(S.FromJsonObj(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("appType")) {
				AppType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("nm")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsObject(p_js_item)) {
							uint new_item_pos = 0;
							CampaignStatistics_Ware * p_item = SList_Ware.CreateNewItem(&new_item_pos);
							if(p_item) {
								if(p_item->FromJsonObj(p_js_item)) {
									;
								}
								else {
									SList_Ware.atFree(new_item_pos);
								}
							}
						}
					}
				}
			}
		}
		if(AppType)
			ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::CampaignStatistics_Day::CampaignStatistics_Day() : Dt(ZERODATE)
{
}

PPMarketplaceInterface_Wildberries::CampaignStatistics_Day & PPMarketplaceInterface_Wildberries::CampaignStatistics_Day::Z()
{
	Dt.Z();
	S.Z();
	SList_App.freeAll();
	return *this;
}

bool PPMarketplaceInterface_Wildberries::CampaignStatistics_Day::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(S.FromJsonObj(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("date")) {
				LDATETIME dtm;
				strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, 0);
				Dt = dtm.d;
			}
			else if(p_cur->Text.IsEqiAscii("apps")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsObject(p_js_item)) {
							uint new_item_pos = 0;
							CampaignStatistics_App * p_item = SList_App.CreateNewItem(&new_item_pos);
							if(p_item) {
								if(p_item->FromJsonObj(p_js_item)) {
									;
								}
								else {
									SList_App.atFree(new_item_pos);
								}
							}
						}
					}
				}
			}
		}
		if(checkdate(Dt))
			ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::CampaignStatistics_Adv::CampaignStatistics_Adv() : AdvertId(0)
{
	Period.Z();
}
		
PPMarketplaceInterface_Wildberries::CampaignStatistics_Adv & PPMarketplaceInterface_Wildberries::CampaignStatistics_Adv::Z()
{
	AdvertId = 0;
	Period.Z();
	S.Z();
	SList_Day.freeAll();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::CampaignStatistics_Adv::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(S.FromJsonObj(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("advertId")) {
				AdvertId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("interval")) {
				// {"begin": "", "end": ""}
			}
			else if(p_cur->Text.IsEqiAscii("dates")) {
				// Array of strings <date> [ items <date > ]
			}
			else if(p_cur->Text.IsEqiAscii("days")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						if(SJson::IsObject(p_js_item)) {
							uint new_item_pos = 0;
							CampaignStatistics_Day * p_item = SList_Day.CreateNewItem(&new_item_pos);
							if(p_item) {
								if(p_item->FromJsonObj(p_js_item)) {
									;
								}
								else {
									SList_Day.atFree(new_item_pos);
								}
							}
						}
					}
				}
			}
		}
		if(AdvertId)
			ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Stock::Stock() : Ware(), DtmLastChange(ZERODATETIME), Qtty(0.0), QttyFull(0.0), InWayToClient(0.0), InWayFromClient(0.0),
	Price(0.0), Discount(0.0), Flags(0)
{
}
		
PPMarketplaceInterface_Wildberries::Stock & PPMarketplaceInterface_Wildberries::Stock::Z()
{
	Ware.Z();
	DtmLastChange.Z();
	WarehouseName.Z();
	Qtty = 0.0;
	QttyFull = 0.0;
	InWayToClient = 0.0;
	InWayFromClient = 0.0;
	Price = 0.0;
	Discount = 0.0;
	Flags = 0;
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Stock::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		Ware.FromJsonObj(pJs);
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("lastChangeDate")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmLastChange = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("warehouseName"))
				WarehouseName = p_cur->P_Child->Text;
			else if(p_cur->Text.IsEqiAscii("quantity"))
				Qtty = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("inWayToClient"))
				InWayToClient = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("inWayFromClient"))
				InWayFromClient = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("quantityFull"))
				QttyFull = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("brand")) {
				;
			}
			else if(p_cur->Text.IsEqiAscii("Price"))
				Price = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("Discount"))
				Discount = p_cur->P_Child->Text.ToReal_Plain();
			else if(p_cur->Text.IsEqiAscii("isSupply")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsSupply;
				}
			}
			else if(p_cur->Text.IsEqiAscii("isRealization")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsRealization;
				}
			}
			else if(p_cur->Text.IsEqiAscii("SCCode")) {
				;
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Income::Income() : Ware(), Dtm(ZERODATETIME), DtmLastChange(ZERODATETIME), DtmClose(ZERODATETIME), IncomeID(0),
	Qtty(0.0), TotalPrice(0.0)
{
}
		
PPMarketplaceInterface_Wildberries::Income & PPMarketplaceInterface_Wildberries::Income::Z()
{
	Ware.Z();
	IncomeID = 0;
	Dtm = ZERODATETIME;
	DtmLastChange = ZERODATETIME;
	DtmClose = ZERODATETIME;
	Number.Z();
	Qtty = 0.0;
	TotalPrice = 0.0;
	WarehouseName.Z();
	Status.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Income::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		LDATETIME dtm;
		Ware.FromJsonObj(pJs);
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("lastChangeDate")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmLastChange = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("date")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Dtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("dateClose")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmClose = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("incomeId")) {
				IncomeID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("number")) {
				Number = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("warehouseName")) {
				WarehouseName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("status")) {
				Status = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("quantity")) {
				Qtty = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("totalPrice")) {
				TotalPrice = p_cur->P_Child->Text.ToReal();
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::Sale::Sale() : Ware(), Dtm(ZERODATETIME), DtmLastChange(ZERODATETIME), DtmCancel(ZERODATETIME), IncomeID(0), Flags(0),
	TotalPrice(0.0), DiscountPct(0.0), Spp(0.0), PaymentSaleAmount(0.0), ForPay(0.0), FinishedPrice(0.0), PriceWithDiscount(0.0) 
{
}
		
PPMarketplaceInterface_Wildberries::Sale & PPMarketplaceInterface_Wildberries::Sale::Z()
{
	Dtm.Z();
	DtmLastChange.Z();
	Ware.Z();
	WarehouseName.Z();
	CountryName.Z();
	DistrictName.Z();
	RegionName.Z();
	IncomeID = 0;
	Flags = 0;
	TotalPrice = 0.0;
	DiscountPct = 0.0;
	Spp = 0.0;
	PaymentSaleAmount = 0.0;
	ForPay = 0.0;
	FinishedPrice = 0.0;
	PriceWithDiscount = 0.0;
	SaleId.Z();
	OrderType.Z();
	Sticker.Z();
	GNumber.Z();
	SrID.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::Sale::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		LDATETIME dtm;
		Ware.FromJsonObj(pJs);
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("lastChangeDate")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmLastChange = dtm;
				}
			}
			if(p_cur->Text.IsEqiAscii("date")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Dtm = dtm;
				}
			}
			if(p_cur->Text.IsEqiAscii("cancelDate")) {
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					DtmCancel = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("warehouseName")) {
				WarehouseName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("countryName")) {
				CountryName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("oblastOkrugName")) {
				DistrictName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("regionName")) {
				RegionName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("incomeID")) {
				IncomeID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("totalPrice")) {
				TotalPrice = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("discountPercent")) {
				DiscountPct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("spp")) {
				Spp = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("paymentSaleAmount")) { // sale only
				PaymentSaleAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("forPay")) { // sale only
				ForPay = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("finishedPrice")) {
				FinishedPrice = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("priceWithDisc")) {
				PriceWithDiscount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("orderType")) {
				OrderType = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("sticker")) {
				Sticker = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("gNumber")) {
				GNumber = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("srid")) {
				SrID = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("saleID")) {
				SaleId = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("isSupply")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsSupply;
				}
			}
			else if(p_cur->Text.IsEqiAscii("isRealization")) {
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsRealization;
				}
			}
			else if(p_cur->Text.IsEqiAscii("isCancel")) { // Order only
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsCancel;
				}
			}
		}
		ok = true;
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::SalesRepDbpEntry::SalesRepDbpEntry() : RepId(0), RepType(0), Flags(0), CrDate(ZERODATE), RrdId(0), IncomeID(0), OrderDtm(ZERODATETIME), 
	SaleDtm(ZERODATETIME), RrDtm(ZERODATETIME), ShkId(0), RId(0), Ppvz_OfficeId(0), Ppvz_SupplierId(0), DeliveryCount(0), ReturnCount(0),
	Qtty(0.0), RetailPrice(0.0), RetailAmount(0.0), SalePct(0.0), CommissionPct(0.0), RetailPriceWithDiscount(0.0), DeliveryAmount(0.0),
	ProductDiscount(0.0), Ppvz_Spp_Prc(0.0), Ppvz_Kvw_Prc_Base(0.0), Ppvz_Kvw_Prc(0.0), Sup_Rating_Prc_Up(0.0), IS_Kgvp_V2(0.0), Ppvz_Sales_Commission(0.0),
	Ppvz_For_Pay(0.0), Ppvz_Reward(0.0), AcquiringFee(0.0), AcquiringPct(0.0), Ppvz_Vw(0.0), Ppvz_Vw_Vat(0.0), Penalty(0.0), AdditionalPayment(0.0), RebillLogisticCost(0.0),
	StorageFee(0.0), Deduction(0.0), Acceptance(0.0)
{
	Period.Z();
	CurrencySymb[0] = 0;
}
		
PPMarketplaceInterface_Wildberries::SalesRepDbpEntry & PPMarketplaceInterface_Wildberries::SalesRepDbpEntry::Z()
{
	Period.Z();
	CurrencySymb[0] = 0;
	RepId = 0;
	RepType = 0;
	Flags = 0; // @v12.2.0
	CrDate.Z();
	RrdId = 0;
	IncomeID = 0;
	OrderDtm.Z();
	SaleDtm.Z();
	RrDtm.Z();
	ShkId = 0;
	RId = 0;
	Ppvz_OfficeId = 0;
	Ppvz_SupplierId = 0;
	DeliveryCount = 0;
	ReturnCount = 0;
	Qtty = 0.0;
	RetailPrice = 0.0;
	RetailAmount = 0.0;
	SalePct = 0.0;
	CommissionPct = 0.0;
	RetailPriceWithDiscount = 0.0;
	DeliveryAmount = 0.0;
	ProductDiscount = 0.0;
	Ppvz_Spp_Prc = 0.0;
	Ppvz_Kvw_Prc_Base = 0.0;
	Ppvz_Kvw_Prc = 0.0;
	Sup_Rating_Prc_Up = 0.0;
	IS_Kgvp_V2 = 0.0;
	Ppvz_Sales_Commission = 0.0;
	Ppvz_For_Pay = 0.0;
	Ppvz_Reward = 0.0;
	AcquiringFee = 0.0;
	AcquiringPct = 0.0;
	Ppvz_Vw = 0.0;
	Ppvz_Vw_Vat = 0.0;
	Penalty = 0.0;
	AdditionalPayment = 0.0;
	RebillLogisticCost = 0.0;
	StorageFee = 0.0;
	Deduction = 0.0;
	Acceptance = 0.0;
	SupplierContractCode.Z();
	SrID.Z();
	Ware.Z();
	DocTypeName.Z();
	Warehouse.Z();
	SupplOpName.Z();
	BoxTypeName.Z();
	SupplPromo.Z();
	AcquiringBank.Z();
	Ppvz_OfficeName.Z();
	Ppvz_SupplierName.Z();
	Ppvz_Inn.Z();
	Clb.Z();
	BonusTypeName.Z();
	Sticker.Z();
	Country.Z();
	RebillLogisticOrg.Z();
	Kiz.Z();
	return *this;
}
		
bool PPMarketplaceInterface_Wildberries::SalesRepDbpEntry::FromJsonObj(const SJson * pJs)
{
	Z();
	bool   ok = false;
	if(pJs && pJs->Type == SJson::tOBJECT) {
		SString temp_buf;
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("realizationreport_id")) {
				RepId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("date_from")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Period.low = dtm.d;
				}
			}
			else if(p_cur->Text.IsEqiAscii("date_to")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					Period.upp = dtm.d;
				}
			}
			else if(p_cur->Text.IsEqiAscii("create_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					CrDate = dtm.d;
				}
			}
			else if(p_cur->Text.IsEqiAscii("currency_name")) {
				STRNSCPY(CurrencySymb, p_cur->P_Child->Text);
			}
			else if(p_cur->Text.IsEqiAscii("suppliercontract_code")) {
				SupplierContractCode = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("rrd_id")) {
				RrdId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("gi_id")) {
				IncomeID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("subject_name")) {
				Ware.Name = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("nm_id")) {
				Ware.ID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("brand_name")) {
				Ware.Brand = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("sa_name")) {
				Ware.SupplArticle = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ts_name")) {
				Ware.TechSize = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("barcode")) {
				Ware.Barcode = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("doc_type_name")) {
				DocTypeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("quantity")) {
				Qtty = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("retail_price")) {
				RetailPrice = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("retail_amount")) {
				RetailAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("sale_percent")) {
				SalePct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("commission_percent")) {
				CommissionPct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("office_name")) {
				Warehouse = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("supplier_oper_name")) {
				SupplOpName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("order_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					OrderDtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("sale_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					SaleDtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("rr_dt")) {
				LDATETIME dtm;
				if(strtodatetime(p_cur->P_Child->Text, &dtm, DATF_ISO8601CENT, TIMF_HMS)) {
					RrDtm = dtm;
				}
			}
			else if(p_cur->Text.IsEqiAscii("shk_id")) {
				ShkId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("retail_price_withdisc_rub")) {
				RetailPriceWithDiscount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("delivery_amount")) {
				DeliveryCount = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("return_amount")) {
				ReturnCount = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("delivery_rub")) {
				DeliveryAmount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("gi_box_type_name")) {
				BoxTypeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("product_discount_for_report")) {
				ProductDiscount = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("supplier_promo")) {
				SupplPromo = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("rid")) {
				RId = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_spp_prc")) {
				Ppvz_Spp_Prc = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_kvw_prc_base")) {
				Ppvz_Kvw_Prc_Base = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_kvw_prc")) {
				Ppvz_Kvw_Prc = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("sup_rating_prc_up")) {
				Sup_Rating_Prc_Up = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("is_kgvp_v2")) {
				IS_Kgvp_V2 = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_sales_commission")) {
				Ppvz_Sales_Commission = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_for_pay")) {
				Ppvz_For_Pay = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_reward")) {
				Ppvz_Reward = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acquiring_fee")) {
				AcquiringFee = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acquiring_percent")) {
				AcquiringPct = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acquiring_bank")) {
				AcquiringBank = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_vw")) {
				Ppvz_Vw = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_vw_nds")) {
				Ppvz_Vw_Vat = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_office_id")) {
				Ppvz_OfficeId = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_office_name")) {
				Ppvz_OfficeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_supplier_id")) {
				Ppvz_SupplierId = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_supplier_name")) {
				Ppvz_SupplierName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("ppvz_inn")) {
				Ppvz_Inn = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("declaration_number")) {
				Clb = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("bonus_type_name")) {
				BonusTypeName = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("sticker_id")) {
				Sticker = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("site_country")) {
				Country = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("penalty")) {
				Penalty = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("additional_payment")) {
				AdditionalPayment = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("rebill_logistic_cost")) {
				RebillLogisticCost = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("rebill_logistic_org")) {
				RebillLogisticOrg = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("kiz")) {
				Kiz = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("storage_fee")) {
				StorageFee = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("deduction")) {
				Deduction = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("acceptance")) {
				Acceptance = p_cur->P_Child->Text.ToReal();
			}
			else if(p_cur->Text.IsEqiAscii("srid")) {
				SrID = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("report_type")) {
				RepType = p_cur->P_Child->Text.ToLong();
			}
			else if(p_cur->Text.IsEqiAscii("payment_processing")) { // @v12.2.0
				PaymentProcessing = p_cur->P_Child->Text;
			}
			else if(p_cur->Text.IsEqiAscii("is_legal_entity")) { // @v12.2.0
				if(SJson::GetBoolean(p_cur->P_Child) == 1) {
					Flags |= fIsLegalEntity;
				}				
			}
		}
		ok = true;
	}
	return ok;
}

/*static*/int PPMarketplaceInterface_Wildberries::ParseApiToken(const char * pToken, ApiTokenDecodeResult * pResult)
{
	int    ok = 0;
	ApiTokenDecodeResult result;
	SString temp_buf;
	SString token(pToken);
	if(token.NotEmptyS()) {
		STempBuffer bin_buf(8192);
		if(bin_buf.IsValid()) {
			size_t actual_size = 0;
			StringSet ss('.', token);
			uint   part_no = 0;
			bool   local_fault = false;
			for(uint ssp = 0; !local_fault && ss.get(&ssp, temp_buf);) {
				++part_no;
				uint _rem4 = temp_buf.Len() % 4;
				if(_rem4) {
					temp_buf.CatCharN('=', 4 - _rem4);
				}
				temp_buf.ReplaceChar('-', '+').ReplaceChar('_', '/');
				if(temp_buf.DecodeMime64(bin_buf, bin_buf.GetSize(), &actual_size)) {
					temp_buf.Z().CatN(bin_buf, actual_size);
					if(part_no == 1) {
						;
					}
					else if(part_no == 2) {
						SJson * p_js = SJson::Parse(temp_buf);
						if(SJson::IsObject(p_js)) {
							for(const SJson * p_jsn = p_js->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
								if(p_jsn->Text.IsEqiAscii("id")) {
									//rItem.Id = p_jsn->P_Child->Text.ToInt64();
									if(!result.Id.FromStr(p_jsn->P_Child->Text))
										local_fault = true;
								}
								else if(p_jsn->Text.IsEqiAscii("sid")) {
									//rItem.Name = p_jsn->P_Child->Text.Unescape();
									if(!result.SellerId.FromStr(p_jsn->P_Child->Text))
										local_fault = true;
								}
								else if(p_jsn->Text.IsEqiAscii("s")) {
									uint64 local_flags = p_jsn->P_Child->Text.ToUInt64();
									if(local_flags & (1ULL << 1))
										result.Flags |= ApiTokenDecodeResult::fAccs_Content;
									if(local_flags & (1ULL << 2))
										result.Flags |= ApiTokenDecodeResult::fAccs_Analitycs;
									if(local_flags & (1ULL << 3))
										result.Flags |= ApiTokenDecodeResult::fAccs_Prices;
									if(local_flags & (1ULL << 4))
										result.Flags |= ApiTokenDecodeResult::fAccs_Marketplace;
									if(local_flags & (1ULL << 5))
										result.Flags |= ApiTokenDecodeResult::fAccs_Statistics;
									if(local_flags & (1ULL << 6))
										result.Flags |= ApiTokenDecodeResult::fAccs_Promo;
									if(local_flags & (1ULL << 7))
										result.Flags |= ApiTokenDecodeResult::fAccs_QAndReviews;
									if(local_flags & (1ULL << 9))
										result.Flags |= ApiTokenDecodeResult::fAccs_Chat;
									if(local_flags & (1ULL << 10))
										result.Flags |= ApiTokenDecodeResult::fAccs_Deliveries;
									if(local_flags & (1ULL << 11))
										result.Flags |= ApiTokenDecodeResult::fAccs_Returns;
									if(local_flags & (1ULL << 12))
										result.Flags |= ApiTokenDecodeResult::fAccs_Documents;
									if(local_flags & (1ULL << 30))
										result.Flags |= ApiTokenDecodeResult::fReadOnly;
								}
								else if(p_jsn->Text.IsEqiAscii("exp")) {
									//rItem.Name = p_jsn->P_Child->Text.Unescape();
									uint64 exp = p_jsn->P_Child->Text.ToUInt64();
									result.ExpiryDtm.SetTimeT(exp);
								}
								else if(p_jsn->Text.IsEqiAscii("t")) {
									//rItem.Name = p_jsn->P_Child->Text.Unescape();
									if(SJson::GetBoolean(p_jsn->P_Child) == 1)
										result.Flags |= ApiTokenDecodeResult::fTest;
								}
							}								
						}
						ZDELETE(p_js);
					}
					else if(part_no == 3) {
						;
					}
				}
				else
					local_fault = true;
			}
			if(result.Id.IsZero() || result.SellerId.IsZero())
				local_fault = true;
			if(!local_fault)
				ok = 1;
		}
	}
	ASSIGN_PTR(pResult, result);
	return ok;
}
	
/*static*/SString & PPMarketplaceInterface_Wildberries::MakeSerialIdent(int64 incomeId, const WareBase & rWare, SString & rBuf)
{
	return rBuf.Z().Cat(incomeId).CatChar('-').Cat(rWare.ID);
}

/*static*/int PPMarketplaceInterface_Wildberries::LoadPublicGoodsCategoryList(CategoryPool & rResult, bool useCache) // @v12.3.8
{
	constexpr long cache_expiration_time_sec = (3 * 24 * 3600);
	const  LDATETIME now_dtm = getcurdatetime_();
	rResult.Z();
	//
	// url = 'https://static-basket-01.wbbasket.ru/vol0/data/main-menu-ru-ru-v3.json'
	// headers = {'Accept': '*/*', 'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}
	//
	int    ok = -1;
	SJson * p_js_cat = 0;
	SString temp_buf;
	SString cache_file_name;
	if(GetLocalCachePath(cache_file_name)) {
		assert(cache_file_name.NotEmpty());
		cache_file_name.SetLastSlash().Cat("public-goods-category-list.json");
	}
	else {
		cache_file_name.Z();
	}
	if(useCache && cache_file_name.NotEmpty()) {
		SFile::Stat st;
		if(SFile::GetStat(cache_file_name, 0, &st, 0)) {
			LDATETIME file_dtm;
			file_dtm.SetNs100(st.ModTm_);
			long s = diffdatetimesec(now_dtm, file_dtm);
			if(s <= cache_expiration_time_sec) {
				p_js_cat = SJson::ParseFile(cache_file_name);
				if(rResult.FromJson(p_js_cat))
					ok = 2;
			}
		}
	}
	if(ok < 0) {
		SUniformFileTransmParam param;
		//param.SrcPath = "https://static-basket-01.wbbasket.ru/vol0/data/main-menu-ru-ru-v3.json";
		param.SrcPath = InetUrl::MkHttps("static-basket-01.wbbasket.ru", "vol0/data/main-menu-ru-ru-v3.json");
		PPGetFilePath(PPPATH_OUT, "wildberries-goodscategory-list.json", param.DestPath);
		param.Flags = 0;
		param.Format = SFileFormat::Unkn;
		if(param.Run(0, 0)) {
			p_js_cat = SJson::ParseFile(param.DestPath);
			if(rResult.FromJson(p_js_cat)) {
				if(cache_file_name.NotEmpty())
					SCopyFile(param.DestPath, cache_file_name, 0, FILE_SHARE_READ, 0);
				ok = 1;
			}
		}
	}
	delete p_js_cat;
	return ok;
}

/*static*/int PPMarketplaceInterface_Wildberries::EditPublicGoodsSelectionFilt(const CategoryPool & rCatPool, MarketplaceGoodsSelectionFilt & rFilt)
{
	int    ok = -1;
	SString temp_buf;
	TDialog * dlg = new TDialog(DLG_WBGOODSLISTFILT);
	if(CheckDialogPtr(&dlg)) {
		const long cat_id = static_cast<long>(rFilt.CatID);
		StrAssocArray category_list;
		rCatPool.MakeShardList(category_list);
		SetupStrAssocTreeCombo(dlg, CTLSEL_WBGOODSLISTFILT_CAT, category_list, cat_id, 0);
		dlg->SetupWordSelector(CTLSEL_WBGOODSLISTFILT_CAT, 0, cat_id, 3, WordSel_ExtraBlock::fAlwaysSearchBySubStr);
		dlg->setCtrlString(CTL_WBGOODSLISTFILT_SRCH, (temp_buf = rFilt.SearchPatternUtf8).Transf(CTRANSF_UTF8_TO_INNER));
		dlg->AddClusterAssocDef(CTL_WBGOODSLISTFILT_ORD, 0, MarketplaceGoodsSelectionFilt::ordPopular);
		dlg->AddClusterAssoc(CTL_WBGOODSLISTFILT_ORD, 1, MarketplaceGoodsSelectionFilt::ordPriceUp);
		dlg->AddClusterAssoc(CTL_WBGOODSLISTFILT_ORD, 2, MarketplaceGoodsSelectionFilt::ordPriceDown);
		dlg->AddClusterAssoc(CTL_WBGOODSLISTFILT_ORD, 3, MarketplaceGoodsSelectionFilt::ordRate);
		dlg->AddClusterAssoc(CTL_WBGOODSLISTFILT_ORD, 4, MarketplaceGoodsSelectionFilt::ordNewly);
		dlg->AddClusterAssoc(CTL_WBGOODSLISTFILT_ORD, 5, MarketplaceGoodsSelectionFilt::ordBenefit);
		dlg->SetClusterData(CTL_WBGOODSLISTFILT_ORD, rFilt.Order);
		if(ExecView(dlg) == cmOK) {
			rFilt.CatID = dlg->getCtrlLong(CTLSEL_WBGOODSLISTFILT_CAT);
			dlg->getCtrlString(CTL_WBGOODSLISTFILT_SRCH, temp_buf);
			rFilt.SearchPatternUtf8 = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			dlg->GetClusterData(CTL_WBGOODSLISTFILT_ORD, &rFilt.Order);
			ok = 1;
		}
	}
	delete dlg;
	return ok;
}

/*
	nm - productId
	https://card.wb.ru/cards/v4/detail?appType=1&curr=rub&dest=123585633&spp=30&hide_dtype=13&ab_testing=false&lang=ru&nm=462198542
	rep: {"products":[{"id":462198542,"root":461257172,"kindId":0,"brand":"doTERRA","brandId":35290,"siteBrandId":0,"colors":[],"subjectId":2259,"subjectParentId":49,"name":"Эфирное масло Перечная мята, 15 мл","entity":"эфирные масла","matchId":0,"supplier":"AROMA13","supplierId":250019953,"supplierRating":4.5,"supplierFlags":12240,"pics":3,"rating":0,"reviewRating":0,"nmReviewRating":0,"feedbacks":0,"nmFeedbacks":0,"volume":1,"viewFlags":1318914,"isNew":true,"promotions":[63484,193792],"sizes":[{"name":"","origName":"0","rank":0,"optionId":650298819,"stocks":[{"wh":157848,"dtype":6597069766657,"dist":1770,"qty":19,"priority":48981,"time1":60,"time2":53}],"time1":60,"time2":53,"wh":157848,"dtype":6597069766657,"dist":1770,"price":{"basic":196000,"product":153900,"logistics":200,"return":0},"saleConditions":134217728,"payload":"A1TrAaw1I+pb/PylVFczX65L+gzJ4PCAEztq58sbQJ6uJtp98RgrL9Qk2BF4U/Z83iGZJJyW7Tv2vj+fyUn2tXfqaCm/Hwi0AixmxhJUdoPmRMalGVjw/7Pi+4pkamlPJGaF6h5hrQrMWwtn+CUoH3paxIlTDd7MZ8DMOQZ9xV9jKUAKGQfjwQ"}],"totalQuantity":19,"time1":60,"time2":53,"wh":157848,"dtype":6597069766657,"dist":1770}]}
*/ 

/*static*/int PPMarketplaceInterface_Wildberries::Helper_QueryPublicGoodsList(const SString & rUrlBuf, const char * pOutpFileNameSuffix, int pageNo, PublicWarePool & rResult)
{
	int    ok = -1;
	uint   local_read_count = 0; // Количество записей товаров, считанных из результата запроса
	uint   local_total_count = 0; // Общее количество товаров на сервере, соответствующих запросу
	SString temp_buf;
	SBuffer reply_buf;
	SString reply_js_string;
	SFile wr_stream(reply_buf, SFile::mWrite);
	InetUrl url(rUrlBuf);
	StrStrAssocArray hdr_flds;
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrUserAgent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0)");
	ScURL c;
	int r = c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream);
	if(r) {
		SBuffer * p_reply_buf = static_cast<SBuffer *>(wr_stream);
		if(p_reply_buf && p_reply_buf->GetAvailableSize()) {
			reply_js_string.Z().CatN(p_reply_buf->GetBufC(), p_reply_buf->GetAvailableSize());
			{
				SJson * p_js = SJson::Parse(reply_js_string);
				if(p_js) {
					rResult.FromJson(p_js, true/*concat*/, &local_read_count, &local_total_count);
					delete p_js;
				}
			}
			{
				SString out_file_name;
				(temp_buf = "wildberries-goods-list");
				if(!isempty(pOutpFileNameSuffix))
					temp_buf.CatChar('-').Cat(pOutpFileNameSuffix);
				if(pageNo > 0)
					temp_buf.CatChar('-').Cat(pageNo);
				temp_buf.DotCat("json");
				//(temp_buf = "wildberries-goods-list-search").CatChar('-').Cat(rFilt.CatID).CatChar('-').Cat(page_no).Dot().Cat("json");
				PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
				SFile f_out(out_file_name, SFile::mWrite|SFile::mBinary);
				if(f_out.IsValid()) {
					f_out.Write(reply_js_string.cptr(), reply_js_string.Len());
				}
			}
		}
	}
	if(local_read_count > 0)
		ok = 1;
	return ok;
}
	
/*static*/int PPMarketplaceInterface_Wildberries::LoadPublicGoodsList(const CategoryPool & rCatPool, const MarketplaceGoodsSelectionFilt & rFilt, PublicWarePool & rResult) // @v12.3.8
{
	/*
		url = f'https://catalog.wb.ru/catalog/{shard}/catalog?appType=1&curr=rub' \
				  f'&dest=-1257786' \
				  f'&locale=ru' \
				  f'&page={page}' \
				  f'&priceU={low_price * 100};{top_price * 100}' \
				  f'&sort=popular&spp=0' \
				  f'&{query}' \
				  f'&discount={discount}'


	https://catalog.wb.ru/catalog/head_accessories2/catalog?cat=9967&curr=rub&dest=-1257786&fkind=3&page=1&regions=80,64,38,4,115,83,33,68,70,69,30,86,75,40,1,66,48,110,31,22,71,114,111&sort=popular&spp=0&xsubject=79


    cat — 9967
    Циферки. Берутся они отсюда, соответственно можно искать по названию категории и потом использовать в запросе.
    curr — rub
    Тут, думаю, объяснять не надо. Ну а существующие на сайте курсы можно взять отсюда.
    dest — -1257786
    Берётся отсюда, в качестве параметров GPS координаты, адрес можно указать любой он на результат не влияет, но если его не указать, то координаты не сработают и выдаст стандартные. Забираем всё с xinfo
    fkind — 3
    Если данный параметр не указывать, результат на первый взгляд не поменялся, можно глубоко не изучать.
    page — 1
    Очевидно номер страницы
    regions — 80,64,38,4,115,83,33,68,70,69,30,86,75,40,1,66,48,110,31,22,71,114,111
    Опять циферки, много циферок!
    sort — popular
    Опять же, совершенно необязательный параметр, сортировка по дефолту идёт по популярным. Чтобы понять какие есть вариант, надо потыкать сайт и смотреть запросы. 
	Возможно они где-то даже указаны, искать сейчас не будем, оставим вам на домашнее задание.
    spp — 0
    Очередной параметр который в моём случае не меняет ничего.
    xsubject — 79
    Наконец-то что-то интересное. Это подкатегории товаров, которые можно получить в фильтрах по категории.

	https://catalog.wb.ru/catalog/electronic43/v4/catalog?ab_testing=false&appType=1&curr=rub&dest=123585633&hide_dtype=13&lang=ru&page=1&sort=popular&spp=30&subject=2290
	*/ 
	rResult.Z();
	int    ok = -1;
	bool   debug_mark = false; // @debug
	SString temp_buf;
	SString reply_js_string;
	SString url_buf;
	SString out_file_name;
	SBuffer reply_buf;
	if(rFilt.SearchPatternUtf8.NotEmpty()) {
		//https://search.wb.ru/exactmatch/ru/common/v14/search?ab_testing=false&appType=1&curr=rub&dest=123585633&hide_dtype=13&lang=ru&
			//page=1&query=%D1%81%D0%BC%D0%B0%D1%80%D1%82%D1%84%D0%BE%D0%BD%20samsung&
			//resultset=catalog&sort=popular&spp=30&suppressSpellcheck=false
		const  SString base_url(InetUrl::MkHttps("search.wb.ru", "exactmatch"));
		for(int page_no = 1; page_no <= 100; page_no++) {
			uint   local_read_count = 0; // Количество записей товаров, считанных из результата запроса
			uint   local_total_count = 0; // Общее количество товаров на сервере, соответствующих запросу

			(url_buf = base_url).SetLastDSlash().Cat("ru").SetLastDSlash().Cat("common").SetLastDSlash().Cat("v14").SetLastDSlash().Cat("search").CatChar('?');
			url_buf.CatEq("ab_testing", "false");
			url_buf.CatChar('&').CatEq("appType", 1);
			url_buf.CatChar('&').CatEq("curr", "rub");
			url_buf.CatChar('&').CatEq("dest", 123585633);
			url_buf.CatChar('&').CatEq("hide_dtype", 1);
			url_buf.CatChar('&').CatEq("lang", "ru");
			url_buf.CatChar('&').CatEq("page", page_no);
			url_buf.CatChar('&').CatEq("query", (temp_buf = rFilt.SearchPatternUtf8).ToUrl());
			url_buf.CatChar('&').CatEq("resultset", "catalog");
			{
				const char * p_order = 0;
				switch(rFilt.Order) {
					case MarketplaceGoodsSelectionFilt::ordPopular: p_order = "popular"; break;
					case MarketplaceGoodsSelectionFilt::ordNewly: p_order = "newly"; break;
					case MarketplaceGoodsSelectionFilt::ordPriceDown: p_order = "pricedown"; break;
					case MarketplaceGoodsSelectionFilt::ordPriceUp: p_order = "priceup"; break;
					case MarketplaceGoodsSelectionFilt::ordBenefit: p_order = "benefit"; break;
					case MarketplaceGoodsSelectionFilt::ordRate: p_order = "rate"; break;
					default: p_order = "popular"; break;
				}
				assert(!isempty(p_order));
				url_buf.CatChar('&').CatEq("sort", p_order);
			}
			url_buf.CatChar('&').CatEq("spp", 30);
			url_buf.CatChar('&').CatEq("suppressSpellcheck", "false");
			//
			temp_buf.Z().Cat("search").CatChar('(').Cat(SlHash::XX32(rFilt.SearchPatternUtf8, rFilt.SearchPatternUtf8.Len(), 9)).CatChar(')');
			const int qr = Helper_QueryPublicGoodsList(url_buf, temp_buf, page_no, rResult);
			if(qr > 0) {
				//
			}
			else {
				break;
			}
		}
	}
	else if(rFilt.SupplID) { // @construction
		//https://catalog.wb.ru/sellers/v4/catalog?ab_testid=no_reranking&appType=1&curr=rub&dest=123585633&hide_dtype=13&lang=ru&page=1&sort=popular&spp=30&supplier=44496
		const  SString base_url(InetUrl::MkHttps("catalog.wb.ru", "sellers/v4/catalog"));
		for(int page_no = 1; page_no <= 100; page_no++) {
			uint   local_read_count = 0; // Количество записей товаров, считанных из результата запроса
			uint   local_total_count = 0; // Общее количество товаров на сервере, соответствующих запросу
			(url_buf = base_url).CatChar('?');
			url_buf.CatEq("ab_testing", "no_reranking");
			url_buf.CatChar('&').CatEq("appType", 1);
			url_buf.CatChar('&').CatEq("curr", "rub");
			url_buf.CatChar('&').CatEq("dest", 123585633);
			url_buf.CatChar('&').CatEq("hide_dtype", 13);
			url_buf.CatChar('&').CatEq("lang", "ru");
			url_buf.CatChar('&').CatEq("page", page_no);
			{
				const char * p_order = 0;
				switch(rFilt.Order) {
					case MarketplaceGoodsSelectionFilt::ordPopular: p_order = "popular"; break;
					case MarketplaceGoodsSelectionFilt::ordNewly: p_order = "newly"; break;
					case MarketplaceGoodsSelectionFilt::ordPriceDown: p_order = "pricedown"; break;
					case MarketplaceGoodsSelectionFilt::ordPriceUp: p_order = "priceup"; break;
					case MarketplaceGoodsSelectionFilt::ordBenefit: p_order = "benefit"; break;
					case MarketplaceGoodsSelectionFilt::ordRate: p_order = "rate"; break;
					default: p_order = "popular"; break;
				}
				assert(!isempty(p_order));
				url_buf.CatChar('&').CatEq("sort", p_order);
			}
			url_buf.CatChar('&').CatEq("spp", 30);
			url_buf.CatChar('&').CatEq("supplier", rFilt.SupplID);
			//
			temp_buf.Z().Cat("suppl").CatChar('(').Cat(rFilt.SupplID).CatChar(')');
			const int qr = Helper_QueryPublicGoodsList(url_buf, temp_buf, page_no, rResult);
			if(qr > 0) {
				//
			}
			else {
				break;
			}
		}
	}
	else if(rFilt.BrandID) {
		//https://catalog.wb.ru/brands/v4/catalog?ab_testing=false&appType=1&brand=9282&curr=rub&dest=123585633&hide_dtype=13;14&lang=ru&page=1&sort=popular&spp=30
		const  SString base_url(InetUrl::MkHttps("catalog.wb.ru", "brands/v4/catalog"));
		for(int page_no = 1; page_no <= 100; page_no++) {
			uint   local_read_count = 0; // Количество записей товаров, считанных из результата запроса
			uint   local_total_count = 0; // Общее количество товаров на сервере, соответствующих запросу
			(url_buf = base_url).CatChar('?');
			url_buf.CatEq("ab_testing", "no_reranking");
			url_buf.CatChar('&').CatEq("appType", 1);
			url_buf.CatChar('&').CatEq("brand", rFilt.BrandID);
			url_buf.CatChar('&').CatEq("curr", "rub");
			url_buf.CatChar('&').CatEq("dest", 123585633);
			url_buf.CatChar('&').CatEq("hide_dtype", "13;14");
			url_buf.CatChar('&').CatEq("lang", "ru");
			url_buf.CatChar('&').CatEq("page", page_no);
			{
				const char * p_order = 0;
				switch(rFilt.Order) {
					case MarketplaceGoodsSelectionFilt::ordPopular: p_order = "popular"; break;
					case MarketplaceGoodsSelectionFilt::ordNewly: p_order = "newly"; break;
					case MarketplaceGoodsSelectionFilt::ordPriceDown: p_order = "pricedown"; break;
					case MarketplaceGoodsSelectionFilt::ordPriceUp: p_order = "priceup"; break;
					case MarketplaceGoodsSelectionFilt::ordBenefit: p_order = "benefit"; break;
					case MarketplaceGoodsSelectionFilt::ordRate: p_order = "rate"; break;
					default: p_order = "popular"; break;
				}
				assert(!isempty(p_order));
				url_buf.CatChar('&').CatEq("sort", p_order);
			}
			url_buf.CatChar('&').CatEq("spp", 30);
			//
			temp_buf.Z().Cat("brand").CatChar('(').Cat(rFilt.BrandID).CatChar(')');
			const int qr = Helper_QueryPublicGoodsList(url_buf, temp_buf, page_no, rResult);
			if(qr > 0) {
				//
			}
			else {
				break;
			}
		}
	}
	else if(rFilt.CatID) {
		const  SString base_url(InetUrl::MkHttps("catalog.wb.ru", "catalog"));
		const CategoryPool::Entry * p_entry = rCatPool.GetByID(rFilt.CatID);
		if(p_entry) {
			SString shard;
			SString sub_query;
			(url_buf = base_url).SetLastDSlash();
			rCatPool.GetS(p_entry->ShardP, shard);
			rCatPool.GetS(p_entry->QueryP, sub_query);
			if(shard.NotEmpty()) {
				for(int page_no = 1; page_no <= 100; page_no++) {
					uint   local_read_count = 0; // Количество записей товаров, считанных из результата запроса
					uint   local_total_count = 0; // Общее количество товаров на сервере, соответствующих запросу
					(url_buf = base_url).SetLastDSlash().Cat(shard).SetLastDSlash().Cat("v4").SetLastDSlash().Cat("catalog").CatChar('?');
					// fbrand=id
					url_buf.CatEq("ab_testing", "false");
					url_buf.CatChar('&').CatEq("appType", 1);
					url_buf.CatChar('&').CatEq("curr", "rub");
					url_buf.CatChar('&').CatEq("dest", 123585633);
					url_buf.CatChar('&').CatEq("hide_dtype", 1);
					url_buf.CatChar('&').CatEq("lang", "ru");
					url_buf.CatChar('&').CatEq("page", page_no);
					{
						const char * p_order = 0;
						switch(rFilt.Order) {
							case MarketplaceGoodsSelectionFilt::ordPopular: p_order = "popular"; break;
							case MarketplaceGoodsSelectionFilt::ordNewly: p_order = "newly"; break;
							case MarketplaceGoodsSelectionFilt::ordPriceDown: p_order = "pricedown"; break;
							case MarketplaceGoodsSelectionFilt::ordPriceUp: p_order = "priceup"; break;
							case MarketplaceGoodsSelectionFilt::ordBenefit: p_order = "benefit"; break;
							case MarketplaceGoodsSelectionFilt::ordRate: p_order = "rate"; break;
							default: p_order = "popular"; break;
						}
						assert(!isempty(p_order));
						url_buf.CatChar('&').CatEq("sort", p_order);
					}
					url_buf.CatChar('&').CatEq("spp", 30);
					//CatEq("fkind", 3)/*.CatChar('&').CatEq("regions", "80,64,38,4,115,83,33,68,70,69,30,86,75,40,1,66,48,110,31,22,71,114,111")*/;
					if(sub_query.NotEmpty()) {
						url_buf.CatChar('&').Cat(sub_query);
					}
					// next line works!
					// url_buf = "https://catalog.wb.ru/catalog/electronic43/v4/catalog?ab_testing=false&appType=1&curr=rub&dest=123585633&hide_dtype=13&lang=ru&page=1&sort=popular&spp=30&subject=2290";

					//
					temp_buf.Z().Cat("cat").CatChar('(').Cat(rFilt.CatID).CatChar(')');
					const int qr = Helper_QueryPublicGoodsList(url_buf, temp_buf, page_no, rResult);
					if(qr > 0) {
						;
					}
					else {
						break;
					}
				}
			}
		}
	}
	return ok;
}

PPMarketplaceInterface_Wildberries::PPMarketplaceInterface_Wildberries(PrcssrMarketplaceInterchange & rPrc) : 
	PPMarketplaceInterface("WILDBERRIES", rPrc), Lth(PPFILNAM_MRKTPLCWBTALK_LOG)
{
}

/*virtual*/PPMarketplaceInterface_Wildberries::~PPMarketplaceInterface_Wildberries()
{
}
	
/*virtual*/int PPMarketplaceInterface_Wildberries::Init(PPID guaID)
{
	int    ok = PPMarketplaceInterface::Init(guaID);
	if(ok > 0) {
		FetchWarehouseList(WhList);
	}
	return ok;
}

/*static*/int PPMarketplaceInterface_Wildberries::GetLocalCachePath(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString cache_path;
	PPGetPath(PPPATH_WORKSPACE, cache_path);	
	cache_path.SetLastSlash().Cat("cache").SetLastSlash().Cat("data").SetLastSlash().Cat("wildberries");
	if(SFile::CreateDir(cache_path))
		rBuf = cache_path;
	else
		ok = 0;
	return ok;
}

int PPMarketplaceInterface_Wildberries::ParseJson_WarehouseList(const SJson * pJs, TSCollection <Warehouse> & rList)
{
	rList.freeAll();
	int    ok = 0;
	if(SJson::IsArray(pJs)) {
		for(const SJson * p_js_item = pJs->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
			if(SJson::IsObject(p_js_item)) {
				uint   new_item_pos = 0;
				Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
				if(p_new_item) {
					if(p_new_item->FromJsonObj(p_js_item)) {
						ok = 1;
					}
					else {
						rList.atFree(new_item_pos);
					}
				}
			}
		}
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::FetchWarehouseList(TSCollection <Warehouse> & rList)
{
	constexpr long cache_expiration_time_sec = (3 * 24 * 3600);
	const  LDATETIME now_dtm = getcurdatetime_();
	int    ok = 1;
	bool   do_request = true;
	SString local_cache_path;
	if(GetLocalCachePath(local_cache_path)) {
		local_cache_path.SetLastSlash().Cat("warehouselist.json");
		SFile::Stat st;
		if(SFile::GetStat(local_cache_path, 0, &st, 0)) {
			LDATETIME file_dtm;
			file_dtm.SetNs100(st.ModTm_);
			long s = diffdatetimesec(now_dtm, file_dtm);
			if(s <= cache_expiration_time_sec) {
				SJson * p_js = SJson::ParseFile(local_cache_path);
				if(ParseJson_WarehouseList(p_js, rList)) {
					do_request = false;
				}
				delete p_js;
			}
		}
	}
	if(do_request) {
		ok = Helper_RequestWarehouseList(methWarehouses, rList, local_cache_path, 0/*FaultStatusResult*/);
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::Helper_RequestWarehouseList(int meth/*methWarehouses||methWarehouses2*/, TSCollection <Warehouse> & rList, 
	const char * pFileNameToStoreJson, FaultStatusResult * pError)
{
	rList.freeAll();
	int    ok = 0;
	FaultStatusResult err_status;
	SJson * p_js_reply = 0;
	assert(oneof2(meth, methWarehouses, methWarehouses2));
	if(oneof2(meth, methWarehouses, methWarehouses2)) {
		SString temp_buf;
		SString url_buf;
		StrStrAssocArray hdr_flds;
		THROW(Helper_InitRequest(meth, url_buf, hdr_flds));
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					{
						p_js_reply = SJson::Parse(reply_buf);
						if(ParseJson_WarehouseList(p_js_reply, rList)) {
							if(!isempty(pFileNameToStoreJson)) {
								SFile f_cache(pFileNameToStoreJson, SFile::mWrite);
								if(f_cache.IsValid()) {
									f_cache.Write(reply_buf.cptr(), reply_buf.Len());
								}
							}
							ok = 1;
						}
						else {
							if(err_status.FromJson(p_js_reply)) {
								;
							}
							ok = 0;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	ASSIGN_PTR(pError, err_status);
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseList(TSCollection <Warehouse> & rList, FaultStatusResult * pError)
{
	return Helper_RequestWarehouseList(methWarehouses, rList, 0, pError);
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseList2(TSCollection <Warehouse> & rList, FaultStatusResult * pError)
{
	return Helper_RequestWarehouseList(methWarehouses2, rList, 0, pError);
}

int PPMarketplaceInterface_Wildberries::RequestProductCategoryList(TSCollection <ProductCategory> & rList)
{
	rList.clear();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methContentCategoryList, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("locale", "ru");
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsObject(p_js_reply)) {
						for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("data")) {
								if(SJson::IsArray(p_cur->P_Child)) {
									for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
										if(SJson::IsObject(p_js_item)) {
											uint   new_item_pos = 0;
											ProductCategory * p_new_item = rList.CreateNewItem(&new_item_pos);
											if(p_new_item) {
												if(p_new_item->FromJsonObj(p_js_item)) {
													p_new_item->Flags |= ProductCategory::fTopLevel;
												}
												else {
													rList.atFree(new_item_pos);
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
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestProductSubjectList(TSCollection <ProductCategory> & rList)
{
	// В этой функции rList очищать нельзя - рассчитываем, что в нем перечислены группы верхнего уровня, полученные
	// вызовом RequestProductCategoryList()
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	SString reply_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methContentSubjectList, url_buf, hdr_flds));
	{
		ScURL c;
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		/*
			Query Parameters:
			----------------
			name	string
			Example: name=Socks
			Search by product name (any supported language)

			limit	integer
			Example: limit=1000
			Number of search results, maximum 1,000

			locale	string
			Example: locale=en
			Response language (ru, en, zh)

			offset	integer
			Example: offset=5000
			How many results to skip

			parentID	integer
			Example: parentID=1000
			Parent category ID
		*/ 
		const SString preserve_url_buf(url_buf);
		bool do_more = false;
		uint req_limit = 1000;
		uint req_offset = 0;
		do {
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			uint accepted_count = 0; // Количество элементов, которое вернул сервер. Это число понадобиться чтобы указать req_offset на следующей итерации.
			url_buf = preserve_url_buf;
			url_buf.CatChar('?').CatEq("locale", "ru").CatChar('&').CatEq("limit", req_limit).CatChar('&').CatEq("offset", req_offset);
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					{
						ZDELETE(p_js_reply);
						p_js_reply = SJson::Parse(reply_buf);
						if(SJson::IsObject(p_js_reply)) {
							for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
								if(p_cur->Text.IsEqiAscii("data")) {
									if(SJson::IsArray(p_cur->P_Child)) {
										for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
											accepted_count++;
											if(SJson::IsObject(p_js_item)) {
												ProductCategory * p_item = new ProductCategory;
												if(p_item) {
													if(p_item->FromJsonObj(p_js_item)) {
														if(p_item->ParentID) {
															for(uint i = 0; p_item && i < rList.getCount(); i++) {
																ProductCategory * p = rList.at(i);
																if(p && p->ID == p_item->ParentID) {
																	p->SubjectList.insert(p_item);
																	p_item = 0;
																}
															}
														}
														if(p_item) {
															// Не удалось найти родительский элемент группы (subject) - придется вставить в список как есть.
															rList.insert(p_item);
															p_item = 0;
														}
													}
													else {
														delete p_item;
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
				if(accepted_count >= req_limit) {
					req_offset += accepted_count;
					do_more = true;
				}
				else 
					do_more = false;
			}
		} while(do_more);
	}
	CATCHZOK
	delete p_js_reply;
	return ok;	
}

int PPMarketplaceInterface_Wildberries::RequestWareList()
{
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methContentCardsList, url_buf, hdr_flds));
	{
		/*
			{
				"settings": {
					"sort": {
						"ascending": false
					},
					"filter": {
						"textSearch": "",
						"allowedCategoriesOnly": true,
						"tagIDs": [],
						"objectIDs": [],
						"brands": [],
						"imtID": 0,
						"withPhoto": -1
					},
					"cursor": {
						"updatedAt": "",
						"nmID": 0,
						"limit": 11
					}
				}
			}
		*/
		SJson json_req(SJson::tOBJECT);
		/*{
			SJson * p_js_sort = new SJson(SJson::tOBJECT);
			p_js_sort->InsertBool("ascending", "true");
			json_req.Insert("sort", p_js_sort);
		}*/
		{
			SJson * p_js_filt = new SJson(SJson::tOBJECT);
			p_js_filt->InsertInt("withPhoto", -1);
			//p_js_filt->InsertString("textSearch", "");
			//p_js_filt->InsertBool("allowedCategoriesOnly", true);
			//p_js_filt->InsertInt("imtID", 0);
			json_req.Insert("filter", p_js_filt);
		}
		{
			SJson * p_js_cursor = new SJson(SJson::tOBJECT);
			//p_js_cursor->InsertString("updatedAt", "");
			//p_js_cursor->InsertInt("nmID", 0);
			p_js_cursor->InsertInt("limit", 100);
			json_req.Insert("cursor", p_js_cursor);
		}
		THROW_SL(json_req.ToStr(req_buf));
	}
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		url_buf.CatChar('?').CatEq("locale", "ru");
		Lth.Log("req", url_buf, temp_buf.Z());
		InetUrl url(url_buf);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
								*/
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestGoodsSizes(int64 wbGoodsID)
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	if(wbGoodsID > 0) {
		THROW(Helper_InitRequest(methGoodsSizes, url_buf, hdr_flds));
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			url_buf.CatChar('?').CatEq("nmID", wbGoodsID).CatChar('&').CatEq("limit", 1000).CatChar('&').CatEq("offset", 0);
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					{
						p_js_reply = SJson::Parse(reply_buf);
						if(SJson::IsArray(p_js_reply)) {
							for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
								if(SJson::IsObject(p_js_item)) {
									/*
									uint   new_item_pos = 0;
									Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
									if(p_new_item) {
										if(!p_new_item->FromJsonObj(p_js_item)) {
											rList.atFree(new_item_pos);
										}
									}
									*/
								}
							}
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestGoodsPrices()
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methGoodsPrices, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("limit", 1000).CatChar('&').CatEq("offset", 0);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Warehouse * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
								*/
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestCommission() // @construction
{
	// https://common-api.wildberries.ru/api/v1/tariffs/commission
	int    ok = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methCommission, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("locale", "ru");
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseCoeffsBox() // @v12.2.3 @construction
{
	int   ok = -1;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestWarehouseCoeffsPallet() // @v12.2.3 @construction
{
	int   ok = -1;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestReturnTariff() // @v12.2.3 @construction
{
	int   ok = -1;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestIncomes(TSCollection <Income> & rList, FaultStatusResult * pError)
{
	rList.freeAll();
	int    ok = 1;
	FaultStatusResult err_status;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methIncomes, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Income * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
					else {
						if(err_status.FromJson(p_js_reply)) {
							(temp_buf = "Error").CatDiv(':', 2).Cat(err_status.Status).Space().Cat(err_status.StatusText).Space().CatParStr(err_status.Detail);
							R_Prc.GetLogger().Log(temp_buf);
						}
						ok = 0;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	ASSIGN_PTR(pError, err_status);
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestStocks(TSCollection <Stock> & rList, FaultStatusResult * pError)
{
	rList.freeAll();
	int    ok = 1;
	FaultStatusResult err_status;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methStocks, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Stock * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
					else {
						if(err_status.FromJson(p_js_reply)) {
							(temp_buf = "Error").CatDiv(':', 2).Cat(err_status.Status).Space().Cat(err_status.StatusText).Space().CatParStr(err_status.Detail);
							R_Prc.GetLogger().Log(temp_buf);
						}
						ok = 0;
					}
				}
				{
					//int PPObjOprKind::GetEdiStockOp(PPID * pID, int use_ta)
					/*
					PPObjOprKind op_obj;
					PPID   op_id = 0;
					if(op_obj.GetEdiStockOp(&op_id, 1)) {
						
					}
					*/
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	ASSIGN_PTR(pError, err_status);
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSupplies()
{
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methSupples, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("limit", 1000L).CatChar('&').CatEq("next", 0L);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
			}
		}
	}
	CATCHZOK
	return ok;	
}

int PPMarketplaceInterface_Wildberries::RequestReturns()
{
	int    ok = 1;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	DateRange period;
	period.upp = plusdate(now_dtm.d, -30);
	period.low = plusdate(now_dtm.d, -60);
	THROW(Helper_InitRequest(methReturns, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		temp_buf.Z().CatEq("dateFrom", period.low, DATF_ISO8601CENT).CatChar('&').CatEq("dateTo", period.upp, DATF_ISO8601CENT);
		url_buf.CatChar('?').Cat(temp_buf);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
			}
		}
	}
	CATCHZOK
	return ok;	
}

int PPMarketplaceInterface_Wildberries::RequestAcceptanceReport(const DateRange & rPeriod)
{
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	if(checkdate(rPeriod.low) && checkdate(rPeriod.upp) && rPeriod.upp >= rPeriod.low) {
		THROW(Helper_InitRequest(methAcceptanceReport, url_buf, hdr_flds));
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			temp_buf.Z().CatEq("dateFrom", rPeriod.low, DATF_ISO8601CENT).CatChar('&').CatEq("dateTo", rPeriod.upp, DATF_ISO8601CENT);
			url_buf.CatChar('?').Cat(temp_buf);
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
				}
			}
		}
	}
	CATCHZOK
	return ok;	
}

int PPMarketplaceInterface_Wildberries::RequestOrders(TSCollection <Sale> & rList, FaultStatusResult * pError)
{
	rList.freeAll();
	int    ok = 1;
	FaultStatusResult err_status;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methOrders, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
					}
					else {
						if(err_status.FromJson(p_js_reply)) {
							(temp_buf = "Error").CatDiv(':', 2).Cat(err_status.Status).Space().Cat(err_status.StatusText).Space().CatParStr(err_status.Detail);
							R_Prc.GetLogger().Log(temp_buf);
						}
						ok = 0;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	ASSIGN_PTR(pError, err_status);
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSales(TSCollection <Sale> & rList, FaultStatusResult * pError)
{
	rList.freeAll();
	int    ok = 1;
	FaultStatusResult err_status;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methSales, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		url_buf.CatChar('?').CatEq("dateFrom", encodedate(1, 1, 2024), DATF_ISO8601CENT).CatChar('&').CatEq("flag", 0);
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
							}
						}
						if(rList.getCount()) {
							PPGetFilePath(PPPATH_OUT, "mpwb-sales_order_type.txt", temp_buf);
							SFile f_out(temp_buf, SFile::mWrite);
							for(uint i = 0; i < rList.getCount(); i++) {
								const Sale * p_item = rList.at(i);
								if(p_item) {
									if(p_item->OrderType.NotEmpty()) {
										f_out.WriteLine((temp_buf = p_item->OrderType).CR());
									}
								}
							}
						}
					}
					else {
						if(err_status.FromJson(p_js_reply)) {
							(temp_buf = "Error").CatDiv(':', 2).Cat(err_status.Status).Space().Cat(err_status.StatusText).Space().CatParStr(err_status.Detail);
							R_Prc.GetLogger().Log(temp_buf);
						}
						ok = 0;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	ASSIGN_PTR(pError, err_status);
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestDocumentsList()
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methDocumentsList, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
								*/
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestPromotionList(TSCollection <Promotion> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	const LDATETIME now_dtm = getcurdatetime_();
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methPromotions, url_buf, hdr_flds));
	{
		/*
			startDateTime required string <RFC3339> Example: startDateTime=2023-09-01T00:00:00Z
				Начало периода, формат YYYY-MM-DDTHH:MM:SSZ
			endDateTime required string <RFC3339> Example: endDateTime=2024-08-01T23:59:59Z
				Конец периода, формат YYYY-MM-DDTHH:MM:SSZ
			allPromo required boolean Default: false
				Показать акции:
					false — доступные для участия
					true — все акции
			limit integer <uint> [1..1000] Example: limit=10
				Количество запрашиваемых акций
			offset integer <uint> >= 0 Example: offset=0
				После какого элемента выдавать данные
		*/ 
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		{
			LDATETIME start_dtm;
			start_dtm.Set(plusdate(now_dtm.d, -60), encodetime(0, 0, 1, 0));
			temp_buf.Z().CatEq("startDateTime", start_dtm, DATF_ISO8601CENT, 0).CatChar('Z');
			url_buf.CatChar('?').Cat(temp_buf).CatChar('&');
			temp_buf.Z().CatEq("endDateTime", getcurdatetime_(), DATF_ISO8601CENT, 0).CatChar('Z');
			url_buf.Cat(temp_buf).CatChar('&');
			url_buf.CatEq("allPromo", "true");
		}
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsObject(p_js_reply)) {
						for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("data")) {
								if(SJson::IsObject(p_cur->P_Child)) {
									for(const SJson * p_cur2 = p_cur->P_Child->P_Child; p_cur2; p_cur2 = p_cur2->P_Next) {
										if(p_cur2->Text.IsEqiAscii("promotions")) {
											if(SJson::IsArray(p_cur2->P_Child)) {
												for(const SJson * p_js_item = p_cur2->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
													if(SJson::IsObject(p_js_item)) {
														uint   new_item_pos = 0;
														Promotion * p_new_item = rList.CreateNewItem(&new_item_pos);
														if(p_new_item) {
															if(!p_new_item->FromJsonObj(p_js_item)) {
																rList.atFree(new_item_pos);
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
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}
	
int PPMarketplaceInterface_Wildberries::RequestPromotionDetail(TSCollection <Promotion> & rList)
{
	int    ok = 1;
	SJson * p_js_reply = 0;
	if(rList.getCount()) {
		SString temp_buf;
		SString url_buf;
		StrStrAssocArray hdr_flds;
		THROW(Helper_InitRequest(methPromotionsDetail, url_buf, hdr_flds));
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			uint  ids_count = 0;
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			{
				url_buf.CatChar('?');
				for(uint i = 0; i < rList.getCount(); i++) {
					const Promotion * p_item = rList.at(i);
					if(p_item) {
						url_buf.CatDivConditionally('&', 0, ids_count > 0).CatEq("promotionIDs", p_item->ID);
						ids_count++;
					}
				}
			}
			if(ids_count) {
				Lth.Log("req", url_buf, temp_buf.Z());
				THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
				{
					SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
					if(p_ack_buf) {
						reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
						Lth.Log("rep", 0, reply_buf);
						{
							p_js_reply = SJson::Parse(reply_buf);
							if(SJson::IsObject(p_js_reply)) {
								for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
									if(p_cur->Text.IsEqiAscii("data")) {
										if(SJson::IsObject(p_cur->P_Child)) {
											for(const SJson * p_cur2 = p_cur->P_Child->P_Child; p_cur2; p_cur2 = p_cur2->P_Next) {
												if(p_cur2->Text.IsEqiAscii("promotions")) {
													if(SJson::IsArray(p_cur2->P_Child)) {
														for(const SJson * p_js_item = p_cur2->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
															if(SJson::IsObject(p_js_item)) {
																const SJson * p_js_id = p_js_item->FindChildByKey("id");
																if(SJson::IsNumber(p_js_id)) {
																	const int64 _id = p_js_id->Text.ToInt64();
																	uint  list_item_idx = 0;
																	if(rList.lsearch(&_id, &list_item_idx, CMPF_INT64)) {
																		Promotion * p_item = rList.at(list_item_idx);
																		assert(p_item); // @paranoic
																		if(p_item) { // @paranoic
																			p_item->FromJsonObj(p_js_item);
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
								}
							}
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js_reply;
	return ok;
}
	
int PPMarketplaceInterface_Wildberries::RequestPromotionWareList(int64 actionId, bool isInAction, TSCollection <WareOnPromotion> & rList)
{
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	if(actionId) {
		SString url_buf;
		StrStrAssocArray hdr_flds;
		THROW(Helper_InitRequest(methPromotionsGoods, url_buf, hdr_flds));
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			uint  ids_count = 0;
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			url_buf.CatChar('?').CatEq("promotionID", actionId).CatChar('&').CatEq("inAction", isInAction);
			{
				Lth.Log("req", url_buf, temp_buf.Z());
				THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
				{
					SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
					if(p_ack_buf) {
						reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
						Lth.Log("rep", 0, reply_buf);
						{
							p_js_reply = SJson::Parse(reply_buf);
							if(SJson::IsObject(p_js_reply)) {
								for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
									if(p_cur->Text.IsEqiAscii("data")) {
										if(SJson::IsObject(p_cur->P_Child)) {
											for(const SJson * p_cur2 = p_cur->P_Child->P_Child; p_cur2; p_cur2 = p_cur2->P_Next) {
												if(p_cur2->Text.IsEqiAscii("promotions")) {
													if(SJson::IsArray(p_cur2->P_Child)) {
														for(const SJson * p_js_item = p_cur2->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
															if(SJson::IsObject(p_js_item)) {
																const SJson * p_js_id = p_js_item->FindChildByKey("id");
																if(SJson::IsNumber(p_js_id)) {
																	const int64 _id = p_js_id->Text.ToInt64();
																	uint  list_item_idx = 0;
																	if(rList.lsearch(&_id, &list_item_idx, CMPF_INT64)) {
																		WareOnPromotion * p_item = rList.at(list_item_idx);
																		assert(p_item); // @paranoic
																		if(p_item) { // @paranoic
																			p_item->FromJsonObj(p_js_item);
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
								}
							}
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_js_reply;
	return ok;
}
	
int PPMarketplaceInterface_Wildberries::AddWareListToPromotion()
{
	int    ok = -1;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestPromoCampaignListStatistics(const DateRange * pCommonReqPeriod, const TSCollection <Campaign> & rList, TSCollection <CampaignStatistics_Adv> & rResult) // @v12.2.5 @construction
{
	rResult.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString req_buf;
	const LDATETIME now_dtm = getcurdatetime_();
	SString url_buf;
	StrStrAssocArray hdr_flds;
	if(!rList.getCount()) {
		ok = -1;
	}
	else {
		bool is_there_dateranges = false;
		if(pCommonReqPeriod && !pCommonReqPeriod->IsZero()) {
			is_there_dateranges = true;
		}
		else {
			for(uint i = 0; !is_there_dateranges && i < rList.getCount(); i++) {
				const Campaign * p_item = rList.at(i);
				if(p_item && !p_item->ReqPeriod.IsZero())
					is_there_dateranges = true;
			}
		}
		THROW(Helper_InitRequest(methPromoCampaignsStatistics, url_buf, hdr_flds));
		{
			SJson json_req(SJson::tARRAY);
			for(uint i = 0; i < rList.getCount(); i++) {
				const Campaign * p_item = rList.at(i);
				if(p_item && p_item->ID != 0) {
					SJson * p_js_item = new SJson(SJson::tOBJECT);
					if(p_js_item) {
						p_js_item->InsertInt64("id", p_item->ID);
						DateRange period(p_item->ReqPeriod);
						if(period.IsZero()) {
							if(pCommonReqPeriod && !pCommonReqPeriod->IsZero()) {
								period = *pCommonReqPeriod;
							}
						}
						if(!period.low) {
							period.low = p_item->ChangeDtm.d;
							if(!period.low)
								period.low = now_dtm.d;
						}
						if(!period.upp)
							period.upp = now_dtm.d;
						{
							SJson * p_js_period = new SJson(SJson::tOBJECT);
							if(p_js_period) {
								p_js_period->InsertString("begin", temp_buf.Z().Cat(period.low, DATF_ISO8601CENT));
								p_js_period->InsertString("end", temp_buf.Z().Cat(period.upp, DATF_ISO8601CENT));
								p_js_item->Insert("interval", p_js_period);
							}
						}
						json_req.InsertChild(p_js_item);
					}
				}
			}
			THROW_SL(json_req.ToStr(req_buf));
		}
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			InetUrl url(url_buf);
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			url_buf.CatChar('?').Cat("interval");
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					{
						p_js_reply = SJson::Parse(reply_buf);
						if(SJson::IsArray(p_js_reply)) {
							for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
								if(SJson::IsObject(p_cur)) {
									uint new_item_pos = 0;
									CampaignStatistics_Adv * p_new_item = rResult.CreateNewItem(&new_item_pos);
									if(p_new_item) {
										if(p_new_item->FromJsonObj(p_cur)) {
											;
										}
										else {
											rResult.atFree(new_item_pos);
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
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestPromoCampaignListDetail(TSCollection <Campaign> & rList) // @v12.2.4
{
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString req_buf;
	const LDATETIME now_dtm = getcurdatetime_();
	SString url_buf;
	StrStrAssocArray hdr_flds;
	if(!rList.getCount()) {
		ok = -1;
	}
	else {
		THROW(Helper_InitRequest(methPromoCampaignDetail, url_buf, hdr_flds));
		{
			SJson json_req(SJson::tARRAY);
			for(uint i = 0; i < rList.getCount(); i++) {
				const Campaign * p_item = rList.at(i);
				if(p_item && p_item->ID != 0) {
					json_req.InsertChild(SJson::CreateInt64(p_item->ID));
				}
			}
			THROW_SL(json_req.ToStr(req_buf));
		}
		{
			ScURL c;
			SString reply_buf;
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			InetUrl url(url_buf);
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					{
						p_js_reply = SJson::Parse(reply_buf);
						if(SJson::IsObject(p_js_reply)) {
							for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
								if(p_cur->Text.IsEqiAscii("adverts")) {
									if(SJson::IsArray(p_cur->P_Child)) {
										for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
											if(SJson::IsObject(p_js_item)) {
												int   _type = 0;
												int   _status = 0;
												int   _count = 0;
												TSVector <uint> new_item_pos_list;
												for(const SJson * p_ag_cur = p_js_item->P_Child; p_ag_cur; p_ag_cur = p_ag_cur->P_Next) {
													if(p_ag_cur->Text.IsEqiAscii("type")) {
														if(SJson::IsNumber(p_ag_cur->P_Child)) {
															_type = p_ag_cur->P_Child->Text.ToLong();
														}
													}
													else if(p_ag_cur->Text.IsEqiAscii("status")) {
														if(SJson::IsNumber(p_ag_cur->P_Child)) {
															_status = p_ag_cur->P_Child->Text.ToLong();
														}
													}
													else if(p_ag_cur->Text.IsEqiAscii("count")) {
														if(SJson::IsNumber(p_ag_cur->P_Child)) {
															_count = p_ag_cur->P_Child->Text.ToLong();
														}
													}
													else if(p_ag_cur->Text.IsEqiAscii("advert_list")) {
														if(SJson::IsArray(p_ag_cur->P_Child)) {
															for(const SJson * p_ag_item = p_ag_cur->P_Child->P_Child; p_ag_item; p_ag_item = p_ag_item->P_Next) {
																if(SJson::IsObject(p_ag_item)) {
																	int64 _id = 0;
																	LDATETIME _cdtm(ZERODATETIME);
																	for(const SJson * p_agi_cur = p_ag_item->P_Child; p_agi_cur; p_agi_cur = p_agi_cur->P_Next) {
																		if(p_agi_cur->Text.IsEqiAscii("advertId")) {
																			if(SJson::IsNumber(p_agi_cur->P_Child)) {
																				_id = p_agi_cur->P_Child->Text.ToInt64();
																			}
																		}
																		else if(p_agi_cur->Text.IsEqiAscii("changeTime")) {
																			if(SJson::IsString(p_agi_cur->P_Child)) {
																				strtodatetime(p_agi_cur->P_Child->Text, &_cdtm, DATF_ISO8601, 0);
																			}
																		}
																	}
																	if(_id) {
																		uint new_item_pos = 0;
																		Campaign * p_new_item = rList.CreateNewItem(&new_item_pos);
																		if(p_new_item) {
																			new_item_pos_list.insert(&new_item_pos);
																			p_new_item->ID = _id;
																			p_new_item->ChangeDtm = _cdtm;
																			p_new_item->Status = _status;
																			p_new_item->Type = _type;
																		}
																	}
																}
															}
														}
													}
												}
												//
												// Следующий цикл - страховка на случай, если массив элементов advert_list будет предшествовать
												// одному из элементов "type", "status". Я знаю, что это - маловероятно, но и не такое встречал.
												//
												for(uint nipidx = 0; nipidx < new_item_pos_list.getCount(); nipidx++) {
													const uint new_item_pos = new_item_pos_list.at(nipidx);
													Campaign * p_item = rList.at(new_item_pos);
													if(p_item) {
														p_item->Status = _status;
														p_item->Type = _type;
													}
												}
											}
										}
									}
								}
								else if(p_cur->Text.IsEqiAscii("all")) {
									// integer
									// Это - просто общее число элементов в ответе. Игнорируем.
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestPromoCampaignList(TSCollection <Campaign> & rList) // @v12.2.3
{
	rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	const LDATETIME now_dtm = getcurdatetime_();
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methPromoCampaignCount, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsObject(p_js_reply)) {
						for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("adverts")) {
								if(SJson::IsArray(p_cur->P_Child)) {
									for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
										if(SJson::IsObject(p_js_item)) {
											int   _type = 0;
											int   _status = 0;
											int   _count = 0;
											TSVector <uint> new_item_pos_list;
											for(const SJson * p_ag_cur = p_js_item->P_Child; p_ag_cur; p_ag_cur = p_ag_cur->P_Next) {
												if(p_ag_cur->Text.IsEqiAscii("type")) {
													if(SJson::IsNumber(p_ag_cur->P_Child)) {
														_type = p_ag_cur->P_Child->Text.ToLong();
													}
												}
												else if(p_ag_cur->Text.IsEqiAscii("status")) {
													if(SJson::IsNumber(p_ag_cur->P_Child)) {
														_status = p_ag_cur->P_Child->Text.ToLong();
													}
												}
												else if(p_ag_cur->Text.IsEqiAscii("count")) {
													if(SJson::IsNumber(p_ag_cur->P_Child)) {
														_count = p_ag_cur->P_Child->Text.ToLong();
													}
												}
												else if(p_ag_cur->Text.IsEqiAscii("advert_list")) {
													if(SJson::IsArray(p_ag_cur->P_Child)) {
														for(const SJson * p_ag_item = p_ag_cur->P_Child->P_Child; p_ag_item; p_ag_item = p_ag_item->P_Next) {
															if(SJson::IsObject(p_ag_item)) {
																int64 _id = 0;
																LDATETIME _cdtm(ZERODATETIME);
																for(const SJson * p_agi_cur = p_ag_item->P_Child; p_agi_cur; p_agi_cur = p_agi_cur->P_Next) {
																	if(p_agi_cur->Text.IsEqiAscii("advertId")) {
																		if(SJson::IsNumber(p_agi_cur->P_Child)) {
																			_id = p_agi_cur->P_Child->Text.ToInt64();
																		}
																	}
																	else if(p_agi_cur->Text.IsEqiAscii("changeTime")) {
																		if(SJson::IsString(p_agi_cur->P_Child)) {
																			strtodatetime(p_agi_cur->P_Child->Text, &_cdtm, DATF_ISO8601, 0);
																		}
																	}
																}
																if(_id) {
																	uint new_item_pos = 0;
																	Campaign * p_new_item = rList.CreateNewItem(&new_item_pos);
																	if(p_new_item) {
																		new_item_pos_list.insert(&new_item_pos);
																		p_new_item->ID = _id;
																		p_new_item->ChangeDtm = _cdtm;
																		p_new_item->Status = _status;
																		p_new_item->Type = _type;
																	}
																}
															}
														}
													}
												}
											}
											//
											// Следующий цикл - страховка на случай, если массив элементов advert_list будет предшествовать
											// одному из элементов "type", "status". Я знаю, что это - маловероятно, но и не такое встречал.
											//
											for(uint nipidx = 0; nipidx < new_item_pos_list.getCount(); nipidx++) {
												const uint new_item_pos = new_item_pos_list.at(nipidx);
												Campaign * p_item = rList.at(new_item_pos);
												if(p_item) {
													p_item->Status = _status;
													p_item->Type = _type;
												}
											}
										}
									}
								}
							}
							else if(p_cur->Text.IsEqiAscii("all")) {
								// integer
								// Это - просто общее число элементов в ответе. Игнорируем.
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestBalance()
{
	//rList.freeAll();
	int    ok = 1;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	THROW(Helper_InitRequest(methBalance, url_buf, hdr_flds));
	{
		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		Lth.Log("req", url_buf, temp_buf.Z());
		THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				Lth.Log("rep", 0, reply_buf);
				{
					p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsArray(p_js_reply)) {
						for(const SJson * p_js_item = p_js_reply->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
							if(SJson::IsObject(p_js_item)) {
								/*
								uint   new_item_pos = 0;
								Sale * p_new_item = rList.CreateNewItem(&new_item_pos);
								if(p_new_item) {
									if(!p_new_item->FromJsonObj(p_js_item)) {
										rList.atFree(new_item_pos);
									}
								}
								*/
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_reply;
	return ok;
}

int PPMarketplaceInterface_Wildberries::RequestSalesReportDetailedByPeriod(const DateRange & rPeriod, TSCollection <SalesRepDbpEntry> & rList, FaultStatusResult * pError)
{
	rList.freeAll();
	int    ok = 1;
	FaultStatusResult err_status;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString url_buf;
	StrStrAssocArray hdr_flds;
	StringSet ss_supplier_oper_name;
	/*
		Query parameters:

		dateFrom required string <RFC3339>
			Начальная дата отчета.
			Дата в формате RFC3339. Можно передать дату или дату со временем. Время можно указывать с точностью до секунд или миллисекунд.
			Время передаётся в часовом поясе Мск (UTC+3).
			Примеры:

				2019-06-20
				2019-06-20T23:59:59
				2019-06-20T00:00:00.12345
				2017-03-25T00:00:00 

		limit integer Default: 100000
			Максимальное количество строк отчета, возвращаемых методом. Не может быть более 100000.

		dateTo required string <date>
			Конечная дата отчета

		rrdid integer
			Уникальный идентификатор строки отчета. Необходим для получения отчета частями.
			Загрузку отчета нужно начинать с rrdid = 0 и при последующих вызовах API передавать в запросе значение rrd_id из последней строки, полученной в результате предыдущего вызова.
			Таким образом для загрузки одного отчета может понадобиться вызывать API до тех пор, пока количество возвращаемых строк не станет равным нулю.
	*/
	THROW(Helper_InitRequest(methSalesReportDetailedByPeriod, url_buf, hdr_flds));
	{
		DateRange period(rPeriod);
		period.Actualize(ZERODATE);
		if(!checkdate(period.low)) {
			if(checkdate(period.upp))
				period.low = period.upp;
			else
				period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
		}
		else if(!checkdate(period.upp))
			period.upp = period.low;

		ScURL c;
		SString reply_buf;
		SBuffer ack_buf;
		bool   do_more = false;
		uint   query_no = 0;
		int64  last_rrdid = 0LL;
		clock_t last_clock = 0;
		const  clock_t min_timeout_between_query = 60000;
		const  SString preserve_url_buf(url_buf);
		do {
			query_no++;
			ack_buf.Z();
			SFile  wr_stream(ack_buf, SFile::mWrite);
			uint   rep_limit = 5000;
			uint   result_count = 0;
			THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
			url_buf = preserve_url_buf;
			url_buf.CatChar('?').CatEq("dateFrom", period.low, DATF_ISO8601CENT).CatChar('&').CatEq("dateTo", period.upp, DATF_ISO8601CENT).
				CatChar('&').CatEq("limit", rep_limit).CatChar('&').CatEq("rrdid", last_rrdid);
			Lth.Log("req", url_buf, temp_buf.Z());
			{
				if(query_no > 1) {
					const clock_t cc = clock();
					if((cc - last_clock) < min_timeout_between_query) {
						SDelay(min_timeout_between_query - (cc - last_clock) + 1000); // 1000 - ensurance
					}
				}
				last_clock = clock();
			}
			THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					{
						ZDELETE(p_js_reply);
						p_js_reply = SJson::Parse(reply_buf);
						if(SJson::IsArray(p_js_reply)) {
							for(const SJson * p_js_item = p_js_reply->P_Child; ok && p_js_item; p_js_item = p_js_item->P_Next) {
								if(SJson::IsObject(p_js_item)) {
									result_count++;
									uint   new_item_pos = 0;
									SalesRepDbpEntry * p_new_item = rList.CreateNewItem(&new_item_pos);
									if(p_new_item) {
										if(p_new_item->FromJsonObj(p_js_item)) {
											SETMAX(last_rrdid, p_new_item->RrdId);
											ss_supplier_oper_name.add(p_new_item->SupplOpName);
										}
										else {
											rList.atFree(new_item_pos);
										}
									}
								}
								else {
									; // @todo @err
									ok = 0;
								}
							}
						}
						else {
							if(err_status.FromJson(p_js_reply)) {
								(temp_buf = "Error").CatDiv(':', 2).Cat(err_status.Status).Space().Cat(err_status.StatusText).Space().CatParStr(err_status.Detail);
								R_Prc.GetLogger().Log(temp_buf);
							}
							ok = 0;
						}
					}
				}
			}
			ss_supplier_oper_name.sortAndUndup();
			do_more = LOGIC(ok && result_count && result_count >= rep_limit);
		} while(do_more);
	}
	CATCHZOK
	delete p_js_reply;
	{
		ss_supplier_oper_name.sortAndUndup();
		if(ss_supplier_oper_name.getCount()) {
			PPGetFilePath(PPPATH_OUT, "mpwb-supplier_oper_name.txt", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			for(uint ssp = 0; ss_supplier_oper_name.get(&ssp, temp_buf);) {
				f_out.WriteLine(temp_buf.CR());
			}
		}
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::CreateWarehouse(PPID * pID, int64 outerId, const char * pOuterName, const char * pAddress, int use_ta) // @construction
{
	int    ok = -1;
	PPID   result_id = 0;
	PPID   psn_id = 0;
	SString temp_buf;
	PPObjLocation & r_loc_obj = PsnObj.LocObj;
	LocationTbl::Rec loc_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		const int  gmppr = GetMarketplacePerson(&psn_id, 0);
		THROW(gmppr);
		{
			PPID   local_id = 0;
			SString code_buf;
			code_buf.Z().Cat(outerId);
			if(r_loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, code_buf, &local_id, &loc_rec) > 0 && loc_rec.OwnerID == psn_id) {
				result_id = loc_rec.ID;
				ok = 1;
			}
			else {
				PPID   parent_id = 0; // Идентификатор папки для складов маркетплейса.
				PPLocationPacket loc_pack;
				THROW(!isempty(pOuterName));
				THROW(CreateWarehouseFolder(&parent_id, 0));
				loc_pack.Type = LOCTYP_WAREHOUSE;
				loc_pack.OwnerID = psn_id;
				loc_pack.ParentID = parent_id;
				STRNSCPY(loc_pack.Code, code_buf);
				temp_buf = pOuterName;
				if(temp_buf.IsLegalUtf8()) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				STRNSCPY(loc_pack.Name, temp_buf);
				temp_buf = pAddress;
				if(temp_buf.NotEmptyS()) {
					if(temp_buf.IsLegalUtf8())
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					LocationCore::SetExField(&loc_pack, LOCEXSTR_FULLADDR, temp_buf);
					loc_pack.Flags |= LOCF_MANUALADDR;
				}
				THROW(r_loc_obj.PutPacket(&result_id, &loc_pack, 0));
				R_Prc.GetLogger().LogAcceptMsg(PPOBJ_LOCATION, result_id, 0);
				ok = 2;
			}
		}
		THROW(tra.Commit());
	}
	ASSIGN_PTR(pID, result_id);
	CATCHZOK
	return ok;
}

const PPMarketplaceInterface_Wildberries::Warehouse * PPMarketplaceInterface_Wildberries::SearchWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName, bool adoptive) const
{
	const Warehouse * p_result = 0;
	SString temp_buf;
	SString key_buf(pWhName);
	key_buf.Strip();
	{
		for(uint i = 0; !p_result && i < rWhList.getCount(); i++) {
			const Warehouse * p_iter = rWhList.at(i);
			if(p_iter && p_iter->Name.IsEqiUtf8(key_buf))
				p_result = p_iter;
		}
	}
	if(!p_result && adoptive) {
		key_buf.Utf8ToLower().ReplaceStr("(", " ", 0).ReplaceStr(")", " ", 0).ElimDblSpaces().Strip();
		{
			LongArray eqpfx_list;
			LongArray hassub_list;
			for(uint i = 0; !p_result && i < rWhList.getCount(); i++) {
				const Warehouse * p_iter = rWhList.at(i);
				if(p_iter) {
					(temp_buf = p_iter->Name).Utf8ToLower().ReplaceStr("(", " ", 0).ReplaceStr(")", " ", 0).ElimDblSpaces().Strip();
					if(temp_buf.IsEq(key_buf))
						p_result = p_iter;
					else if(temp_buf.HasPrefix(key_buf)) {
						eqpfx_list.add(i);
					}
					else if(temp_buf.Search(key_buf, 0, 0, 0)) {
						hassub_list.add(i);
					}
				}
			}
			if(!p_result) {
				if(eqpfx_list.getCount() == 1) {
					p_result = rWhList.at(eqpfx_list.get(0));
				}
				else if(hassub_list.getCount() == 1) {
					p_result = rWhList.at(hassub_list.get(0));
				}
			}
		}
	}
	if(!p_result) {
		(temp_buf = pWhName).Transf(CTRANSF_UTF8_TO_INNER);
		PPSetError(PPERR_MP_WAREHOUSEIDENTFAULT, temp_buf);
	}
	return p_result;
}

int PPMarketplaceInterface_Wildberries::ResolveWarehouseByName(const TSCollection <Warehouse> & rWhList, const char * pWhName, PPID defaultID, PPID * pResultID, int use_ta)
{
	int    ok = -1;
	PPID   wh_id = 0;
	if(!isempty(pWhName)) {
		const Warehouse * p_wh = SearchWarehouseByName(WhList, pWhName, true);
		if(p_wh) {
			if(CreateWarehouse(&wh_id, p_wh->ID, p_wh->Name, p_wh->Address, use_ta) > 0) {
				assert(wh_id > 0);
				MpLocList.addUnique(wh_id);
				ok = 1;
			}
		}
	}
	if(!wh_id) {
		if(defaultID) {
			wh_id = defaultID;
			ok = 2;
		}
	}
	ASSIGN_PTR(pResultID, wh_id);
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::CreateWare(const WareBase & rWare, int use_ta) 
{
	PPID   result_id = 0;
	SString temp_buf;
	BarcodeTbl::Rec bc_rec;
	Goods2Tbl::Rec goods_rec;
	if(GObj.SearchByBarcode(rWare.Barcode, &bc_rec, &goods_rec, 0) > 0) {
		assert(goods_rec.ID > 0 && bc_rec.GoodsID > 0 && goods_rec.ID == bc_rec.GoodsID); // @paranoic
		result_id = goods_rec.ID;
	}
	else {
		SString name_buf;
		if(rWare.Name.NotEmpty())
			name_buf = rWare.Name;
		if(rWare.SupplArticle.NotEmpty()) {
			name_buf.CatDivIfNotEmpty('-', 1).Cat(rWare.SupplArticle);
		}
		if(name_buf.IsEmpty())
			name_buf = rWare.Barcode;
		if(name_buf.NotEmpty()) {
			const PPGoodsConfig & r_cfg = GObj.GetConfig();
			const PPID acs_id = PrcssrMarketplaceInterchange::GetMarketplaceAccSheetID();
			SString ar_code;
			PPID   mp_ar_id = 0;
			PPID   temp_id = 0;
			if(rWare.ID)
				ar_code.Cat(rWare.ID);
			if(name_buf.IsLegalUtf8()) {
				name_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			if(rWare.Barcode.IsEmpty() && GObj.SearchByName(name_buf, &temp_id, &goods_rec) > 0) {
				result_id = goods_rec.ID;
			}
			else {
				PPID   mp_psn_id = 0;
				PPObjBrand brand_obj;
				{
					PPTransaction tra(use_ta);
					THROW(tra);
					GetMarketplacePerson(&mp_psn_id, 0/*use_ta*/);
					if(ar_code.NotEmpty() && acs_id) {
						if(mp_psn_id) {
							ArObj.P_Tbl->PersonToArticle(mp_psn_id, acs_id, &mp_ar_id);
							if(mp_ar_id) {
								ArGoodsCodeTbl::Rec ar_code_rec;
								if(GObj.P_Tbl->SearchByArCode(mp_ar_id, ar_code, &ar_code_rec, &goods_rec) > 0) {
									result_id = goods_rec.ID;
								}
							}
						}
					}
					if(!result_id) {
						PPGoodsPacket goods_pack;
						assert(result_id == 0);
						goods_pack.Rec.Kind = PPGDSK_GOODS;
						STRNSCPY(goods_pack.Rec.Name, name_buf);
						STRNSCPY(goods_pack.Rec.Abbr, name_buf);
						if(rWare.Barcode.NotEmpty()) {
							goods_pack.AddCode(rWare.Barcode, 0, 1);
						}
						if(ar_code.NotEmpty()) {
							if(acs_id) {
								if(!mp_ar_id) {
									ArObj.P_Tbl->PersonToArticle(mp_psn_id, acs_id, &mp_ar_id);
								}
								if(mp_ar_id) {
									ArGoodsCodeTbl::Rec ar_code_rec;
									STRNSCPY(ar_code_rec.Code, ar_code);
									ar_code_rec.ArID = mp_ar_id;
									ar_code_rec.Pack = 1000; // 1.0
									ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
									goods_pack.ArCodes.insert(&ar_code_rec);
								}
							}
						}
						if(rWare.Brand.NotEmpty()) {
							temp_buf = rWare.Brand;
							if(temp_buf.IsLegalUtf8()) {
								temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							}
							PPID   brand_id = 0;
							THROW(brand_obj.AddSimple(&brand_id, temp_buf, 0, 0/*use_ta*/));
							goods_pack.Rec.BrandID = brand_id;
						}
						goods_pack.Rec.UnitID = r_cfg.DefUnitID;
						goods_pack.Rec.ParentID = r_cfg.DefGroupID;
						THROW(GObj.PutPacket(&result_id, &goods_pack, 0/*use_ta*/));
						R_Prc.GetLogger().LogAcceptMsg(PPOBJ_GOODS, result_id, 0);
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCH
		result_id = 0;
	ENDCATCH
	return result_id;
}

PPID PPMarketplaceInterface_Wildberries::CreateBuyer(const Sale * pSaleEntry, int use_ta)
{
	PPID  result_ar_id = 0;
	if(pSaleEntry) {
		const  PPID sale_op_id = R_Prc.GetSaleOpID();
		if(sale_op_id) {
			PPOprKind op_rec;
			if(GetOpData(sale_op_id, &op_rec) > 0) {
				const PPID acs_id = op_rec.AccSheetID;
				if(acs_id) {
					PPObjAccSheet acs_obj;
   					PPAccSheet acs_rec;
					PPID   ar_id = 0;
					PPID   psn_id = 0;
					if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
						const PPID  psn_kind_id = acs_rec.ObjGroup;
						if(psn_kind_id) {
							SString name;
							PPID   status_id = PPPRS_REGION;
							if(pSaleEntry->CountryName.NotEmpty()) {
								name.Cat(pSaleEntry->CountryName);
								status_id = PPPRS_COUNTRY;
							}
							if(pSaleEntry->RegionName.NotEmpty()) {
								name.CatDivIfNotEmpty('-', 1).Cat(pSaleEntry->RegionName);
								status_id = PPPRS_REGION;
							}
							if(name.NotEmpty()) {
								if(name.IsLegalUtf8()) {
									name.Transf(CTRANSF_UTF8_TO_INNER);
								}
								PPObjPerson::ResolverParam rslvp;
								rslvp.Flags = rslvp.fCreateIfNFound;
								rslvp.KindID = psn_kind_id;
								rslvp.StatusID = status_id;
								rslvp.CommonName = name;
								PPIDArray psn_candidate_list;
								PsnObj.Resolve(rslvp, psn_candidate_list, use_ta);
								if(psn_candidate_list.getCount()) {
									PPID psn_id = psn_candidate_list.get(0);
									PPID ar_id = 0;
									if(ArObj.P_Tbl->PersonToArticle(psn_id, acs_id, &ar_id)) {
										result_ar_id = ar_id;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return result_ar_id;
}

int PPMarketplaceInterface_Wildberries::InsertReceiptItem(PPBillPacket & rPack, const char * pSerial, PPID goodsID, double qtty)
{
	int    ok = 1;
	PPTransferItem ti;
	LongArray row_idx_list;
	double cost = 0.0;
	double price = 0.0;
	if(!isempty(pSerial)) {
		ReceiptTbl::Rec own_lot_rec;
		PPID   own_lot_id = 0;
		if(SearchOriginalLotForMp(pSerial, rPack.Rec.Dt, rPack.Rec.LocID, goodsID, &own_lot_id) > 0) {
			if(BillObj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
				cost = own_lot_rec.Cost;
				price = own_lot_rec.Price;
			}							
		}
	}
	ti.GoodsID = goodsID;
	ti.Cost = cost;
	ti.Price = price;
	ti.Quantity_ = fabs(qtty);
	THROW(rPack.InsertRow(&ti, &row_idx_list));
	assert(row_idx_list.getCount() == 1);
	if(!isempty(pSerial)) {
		const long new_row_idx = row_idx_list.get(0);
		rPack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, pSerial);
	}
	CATCHZOK
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::CreateReceipt(int64 incomeId, const WareBase & rWare, LDATE dt, PPID locID, PPID goodsID, double qtty, bool surrogateAutoLot, int use_ta)
{
	PPID   result_lot_id = 0;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	PPID   suppl_id = 0;
	PPID   own_lot_id = 0;
	double adj_cost = 0.0;
	double adj_price = 0.0;
	SString bill_code;
	SString serial_buf;
	PPBillPacket::SetupObjectBlock sob;
	PPBillPacket pack;
	THROW(incomeId); // @todo @err
	THROW(rcpt_op_id); // @todo @err
	THROW(checkdate(dt)); 
	bill_code.Z().Cat(incomeId);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ArObj.GetMainOrgAsSuppl(&suppl_id, 1/*processAbsense*/, 0/*use_ta*/));
		THROW(pack.CreateBlank_WithoutCode(rcpt_op_id, 0, locID, 0));
		pack.Rec.Object = suppl_id;
		pack.Rec.Dt = dt;
		STRNSCPY(pack.Rec.Code, bill_code);
		THROW(pack.SetupObject(suppl_id, sob));
		{
			THROW(InsertReceiptItem(pack, MakeSerialIdent(incomeId, rWare, serial_buf), goodsID, qtty));
			if(surrogateAutoLot)
				PPGetWord(PPWORD_AT_AUTO, 0, pack.SMemo);
			pack.InitAmounts();
			p_bobj->FillTurnList(&pack);
			if(!surrogateAutoLot)
				pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_RECEIPT, "MP-WILDBERRIES", 0);
			THROW(p_bobj->TurnPacket(&pack, 0));
			assert(pack.GetTCount() == 1);
			THROW(pack.GetTCount() == 1);
			result_lot_id = pack.ConstTI(0).LotID;
			if(own_lot_id)
				MpLotToOwnLotAssocList.Add(result_lot_id, own_lot_id);
			R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
		}
		THROW(tra.Commit());
	}
	CATCH
		result_lot_id = 0;
	ENDCATCH
	return result_lot_id;
}

int PPMarketplaceInterface_Wildberries::ImportReceipts()
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	PPID   suppl_id = 0;
	SString bill_code;
	TSCollection <PPMarketplaceInterface_Wildberries::Income> income_list;
	FaultStatusResult error_status;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	THROW_PP(rcpt_op_id, PPERR_UNDEFRECEIPTOP);
	THROW(ArObj.GetMainOrgAsSuppl(&suppl_id, 1/*processAbsense*/, 1/*use_ta*/));
	if(!RequestIncomes(income_list, &error_status)) {
	}
	else if(income_list.getCount()) {
		for(uint i = 0; i < income_list.getCount(); i++) {
			const Income * p_wb_item = income_list.at(i);
			if(p_wb_item) {
				PPID   wh_id = 0;
				BillTbl::Rec ex_bill_rec;
				ResolveWarehouseByName(WhList, p_wb_item->WarehouseName, LConfig.Location, &wh_id, 1/*use_ta*/);
				const  PPID  goods_id = CreateWare(p_wb_item->Ware, 1/*use_ta*/);
				if(goods_id) {
					bill_code.Z().Cat(p_wb_item->IncomeID);
					if(p_bobj->P_Tbl->SearchByCode(bill_code, rcpt_op_id, ZERODATE, &ex_bill_rec) > 0) {
						// Поступление на склад '%s' уже акцептировано ранее
						PPBillPacket ex_pack;
						if(p_bobj->ExtractPacket(ex_bill_rec.ID, &ex_pack) > 0) {
							uint   goods_idx = 0;
							if(ex_pack.SearchGoods(goods_id, &goods_idx)) {
								;
							}
							else {
								if(InsertReceiptItem(ex_pack, MakeSerialIdent(p_wb_item->IncomeID, p_wb_item->Ware, temp_buf), goods_id, fabs(p_wb_item->Qtty))) {
									ex_pack.InitAmounts();
									p_bobj->FillTurnList(&ex_pack);
									const int upr = p_bobj->UpdatePacket(&ex_pack, 1);
									if(upr > 0) {
										// @v12.2.6 R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, ex_pack.Rec.ID, 1/*upd*/);
										ok = 1;
									}
									else if(!upr) {
										R_Prc.GetLogger().LogLastError();
									}
								}
							}
						}
					}
					else {
						PPBillPacket pack;
						PPID   ex_bill_id = 0;
						Goods2Tbl::Rec goods_rec;
						PPBillPacket::SetupObjectBlock sob;
						const LDATE dt = ValidDateOr(p_wb_item->Dtm.d, getcurdate_());
						const PPID lot_id = CreateReceipt(p_wb_item->IncomeID, p_wb_item->Ware, dt, wh_id, goods_id, fabs(p_wb_item->Qtty), false, 1);
						if(lot_id) {
							ok = 1;
						}
					}
				}
				else {
					// @todo @err
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

bool PPMarketplaceInterface_Wildberries::MakeTargetUrl_(int meth, int * pReq/*SHttpProtocol::reqXXX*/, SString & rResult) const
{
	// ПВЗ : https://static-basket-01.wbbasket.ru/vol0/data/all-poo-fr-v11.json 
	//
	//
	static const SIntToSymbTabEntry api_list[] = {
		{ apiCommon, "common-api" },
		{ apiStatistics, "statistics-api" },
		{ apiSellerAnalytics, "seller-analytics-api" },
		{ apiAdvert, "advert-api" },
		{ apiRecommend, "recommend-api" },
		{ apiSupplies, "supplies-api" },
		{ apiDiscountsPrices, "discounts-prices-api" },
		{ apiContent, "content-api" },
		{ apiMarketplace, "marketplace-api" },
		{ apiAnalytics, "seller-analytics-api" },
		{ apiDocuments, "documents-api" },
		{ apiDpCalendar, "dp-calendar-api" }, // @v12.2.2
	};
	struct MethEntry {
		int    Meth;
		int    Api;
		int    Req;
		const char * P_UrlSuffix;
	};
	static const MethEntry meth_list[] = {
		{ methCommission, apiCommon, SHttpProtocol::reqGet, "api/v1/tariffs/commission" },
		{ methTariffBox, apiCommon, SHttpProtocol::reqGet, "api/v1/tariffs/box" }, // https://common-api.wildberries.ru/api/v1/tariffs/box
		{ methTariffPallet, apiCommon, SHttpProtocol::reqGet, "api/v1/tariffs/pallet" }, // https://common-api.wildberries.ru/api/v1/tariffs/pallet
		{ methTariffReturn, apiCommon, SHttpProtocol::reqGet, "api/v1/tariffs/return" }, // https://common-api.wildberries.ru/api/v1/tariffs/return
		{ methWarehouses, apiSupplies, SHttpProtocol::reqGet, "api/v1/warehouses" },
		{ methWarehouses2, apiMarketplace, SHttpProtocol::reqGet, "api/v3/offices" },
		{ methIncomes, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/incomes" },
		{ methStocks, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/stocks" },
		{ methOrders, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/orders" },
		{ methSales, apiStatistics, SHttpProtocol::reqGet, "api/v1/supplier/sales" },
		{ methSalesReportDetailedByPeriod, apiStatistics,  SHttpProtocol::reqGet, "api/v5/supplier/reportDetailByPeriod" },
		{ methSupples, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" },
		{ methSupply,  apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" }, // /supplies/{supplyId}
		{ methSupplyOrders, apiMarketplace, SHttpProtocol::reqGet, "api/v3/supplies" }, // /{supplyId}/orders
		{ methAcceptanceReport, apiAnalytics, SHttpProtocol::reqGet, "api/v1/analytics/acceptance-report" },
		{ methGoodsPrices, apiDiscountsPrices, SHttpProtocol::reqGet, "api/v2/list/goods/filter" },
		{ methGoodsSizes,  apiDiscountsPrices, SHttpProtocol::reqGet, "api/v2/list/goods/size/nm" }, // @v12.2.5
		{ methContentCardsList, apiContent, SHttpProtocol::reqPost, "content/v2/get/cards/list" },
		{ methContentCategoryList, apiContent, SHttpProtocol::reqGet, "content/v2/object/parent/all" }, // @v12.2.4
		{ methContentSubjectList, apiContent, SHttpProtocol::reqGet, "content/v2/object/all" }, // @v12.2.4
		{ methBalance, apiAdvert, SHttpProtocol::reqGet, "adv/v1/balance" },
		{ methDocumentsList, apiDocuments, SHttpProtocol::reqGet, "api/v1/documents/list" },
		{ methReturns, apiAnalytics, SHttpProtocol::reqGet, "api/v1/analytics/goods-return" },
		{ methPromotions, apiDpCalendar, SHttpProtocol::reqGet, "api/v1/calendar/promotions" },
		{ methPromotionsDetail, apiDpCalendar, SHttpProtocol::reqGet, "api/v1/calendar/promotions/details" },
		{ methPromotionsGoods, apiDpCalendar, SHttpProtocol::reqGet, "api/v1/calendar/promotions/nomenclatures" },
		{ methPromotionsAddGoods, apiDpCalendar, SHttpProtocol::reqPost, "api/v1/calendar/promotions/upload" },
		{ methPromoCampaignCount, apiAdvert, SHttpProtocol::reqGet, "adv/v1/promotion/count" },
		{ methPromoCampaignDetail, apiAdvert, SHttpProtocol::reqPost, "adv/v1/promotion/adverts" },
		{ methPromoCampaignsStatistics, apiAdvert, SHttpProtocol::reqPost, "adv/v2/fullstats" },
	};
	//https://content-api.wildberries.ru/content/v2/cards/upload
	//https://discounts-prices-api.wildberries.ru/api/v2/upload/task
	//https://supplies-api.wildberries.ru/api/v1/acceptance/coefficients
	//https://recommend-api.wildberries.ru/api/v1/ins
	//https://advert-api.wildberries.ru/adv/v1/save-ad
	// https://seller-analytics-api.wildberries.ru/api/v1/analytics/excise-report
	//https://statistics-api.wildberries.ru/api/v1/supplier/incomes
	// https://common-api.wildberries.ru/api/v1/tariffs/commission
	bool ok = false;
	rResult.Z();
	int  req = 0;
	SString temp_buf;
	{
		bool   local_ok = false;
		for(uint midx = 0; midx < SIZEOFARRAY(meth_list); midx++) {
			if(meth_list[midx].Meth == meth) {
				const int api = meth_list[midx].Api;
				if(SIntToSymbTab_GetSymb(api_list, SIZEOFARRAY(api_list), api, temp_buf)) {
					rResult.Cat("https").Cat("://").Cat(temp_buf).DotCat("wildberries").DotCat("ru");
					if(!isempty(meth_list[midx].P_UrlSuffix)) {
						req = meth_list[midx].Req;
						rResult.SetLastDSlash().Cat(meth_list[midx].P_UrlSuffix);
						local_ok = true;
					}
				}
				break;
			}
		}
		if(local_ok) {
			ok = true;
		}
	}
	/*switch(query) {
		case qAuthLogin: entry.Set(SHttpProtocol::reqPost, "login-api/api/v1/Auth/login"); break;
		case qAuthExtTok: entry.Set(SHttpProtocol::reqPost, "login-api/api/v1/Auth/extended-token"); break;
		case qGetWarehouses: entry.Set(SHttpProtocol::reqGet, "distribution-api/api/v1/Distributions/warehouses"); break;
		case qGetProducts: entry.Set(SHttpProtocol::reqGet, "product-api/api/v1/Products/integration"); break;
		case qGetClients: entry.Set(SHttpProtocol::reqPost, "client-api/api/v1/Clients/GetFilteredList"); break;
		case qSendSellout: entry.Set(SHttpProtocol::reqPost, "sales-api/api/v2/Sales/sellout"); break;
		case qSendSellin: entry.Set(SHttpProtocol::reqPost, "sales-api/api/v2/Sales/sellin"); break;
		case qSendRest: entry.Set(SHttpProtocol::reqPost, "sales-api/api/v1/Sales/warehousebalances"); break;
	}*/
	/*if(!isempty(entry.P_Path)) {
		rResult.SetLastDSlash().Cat(entry.P_Path);
	}*/
	ASSIGN_PTR(pReq, req);
	return ok;
}

int PPMarketplaceInterface_Wildberries::Helper_InitRequest(int meth, SString & rUrlBuf, StrStrAssocArray & rHdrFlds)
{
	int    ok = 0;
	rUrlBuf.Z();
	rHdrFlds.Z();
	if(R_Prc.GetGuaPack().Rec.ServiceIdent == PPGLS_WILDBERRIES) {
		SString token;
		if(R_Prc.GetGuaPack().GetAccessKey(token) > 0) {
			//InetUrl url(MakeTargetUrl_(qAuthLogin, &req, url_buf));
			int   req = 0;
			if(MakeTargetUrl_(meth, &req/*SHttpProtocol::reqXXX*/, rUrlBuf)) {
				SString hdr_buf;
				MakeHeaderFields(token, &rHdrFlds, hdr_buf);
				ok = 1;
			}
		}
	}
	return ok;
}

SString & PPMarketplaceInterface_Wildberries::MakeHeaderFields(const char * pToken, StrStrAssocArray * pHdrFlds, SString & rBuf) const
{
	StrStrAssocArray hdr_flds;
	SETIFZ(pHdrFlds, &hdr_flds);
	{
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrCacheControl, "no-cache");
		//SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAcceptLang, "ru");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAccept, "application/json");
	}
	if(!isempty(pToken)) {
		SString temp_buf;
		//(temp_buf = "Bearer").Space().Cat(pToken);
		temp_buf = pToken;
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAuthorization, temp_buf);
	}
	SHttpProtocol::PutHeaderFieldsIntoString(*pHdrFlds, rBuf);
	return rBuf;
}

int PPMarketplaceInterface_Wildberries::SearchOriginalLotForMp(const char * pSerial, LDATE dt, PPID locID, PPID goodsID, PPID * pResultLotID)
{
	int    ok = -1;
	PPID   target_lot_id = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	if(goodsID) {
		PPID   transf_op_id = R_Prc.GetConfig().TransferToMpOpID;
		if(transf_op_id) {
			PPObjBill * p_bobj = BillObj;
			BillCore * p_billc = p_bobj->P_Tbl;
			Transfer * p_trfr = p_bobj->trfr;
			LDATE recent_date = ZERODATE;
			BillTbl::Rec bill_rec;
			BillTbl::Key2 k2;
			k2.OpID = transf_op_id;
			k2.Dt   = dt;
			k2.BillNo = MAXLONG;
			const long max_days_for_survey = 30;
			PPID   last_suitable_lot_id_without_serial = 0;
			PPIDArray candidate_lot_list_by_serial;
			SString serial_buf(pSerial);
			if(serial_buf.NotEmpty()) {
				p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_SN, serial_buf, &candidate_lot_list_by_serial);
			}
			if(p_billc->search(2, &k2, spLe) && k2.OpID == transf_op_id) do {
				const PPID bill_id = p_billc->data.ID;
				PPTransferItem ti;
				for(int rbb = 0; p_trfr->EnumItems(bill_id, &rbb, &ti) > 0;) {
					if(ti.GoodsID == goodsID) {
						SETIFZ(last_suitable_lot_id_without_serial, ti.LotID);
						if(serial_buf.NotEmpty() && candidate_lot_list_by_serial.getCount()) {
							if(candidate_lot_list_by_serial.lsearch(ti.LotID))
								target_lot_id = ti.LotID;
						}
					}
				}
			} while(!target_lot_id && p_billc->search(2, &k2, spPrev) && k2.OpID == transf_op_id);
			ReceiptTbl::Rec target_lot_rec;
			SETIFZ(target_lot_id, last_suitable_lot_id_without_serial);
			if(target_lot_id && p_trfr->Rcpt.Search(target_lot_id, &target_lot_rec) > 0) {
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pResultLotID, target_lot_id);
	return ok;
}

PPID PPMarketplaceInterface_Wildberries::AdjustReceiptOnExpend(
	/*const Sale & rWbItem,*/const WareBase & rW, int64 incomeId, LDATE dt, PPID locID, PPID goodsID, double neededQtty, double nominalPrice, int use_ta)
{
	PPID   result_lot_id = 0;
	PPObjBill * p_bobj = BillObj;
	SString serial_buf;
	const  PPID rcpt_op_id = CConfig.ReceiptOp;
	THROW_SL(checkdate(dt)); 
	if(incomeId && rW.ID && locID) {
		MakeSerialIdent(incomeId, rW, serial_buf);
		PPIDArray lot_id_list;
		bool   lot_found = false;
		PPID   intrexp_op_id = 0;
		ReceiptTbl::Rec lot_rec;
		{
			PPObjOprKind op_obj;
			PPOprKind op_rec;
			for(SEnum en = op_obj.Enum(0); !intrexp_op_id && en.Next(&op_rec) > 0;) {
				if(IsIntrOp(op_rec.ID) == INTREXPND)
					intrexp_op_id = op_rec.ID;
			}
		}
		if(p_bobj->SearchLotsBySerialExactly(serial_buf, &lot_id_list) > 0) {
			PPID   lot_id = 0;
			PPID   lot_candidate_for_intrexpend = 0; // Лот - кандидат на передачу товара межскладом
			for(uint i = 0; !lot_id && i < lot_id_list.getCount(); i++) {
				const PPID iter_lot_id = lot_id_list.get(i);
				if(p_bobj->trfr->Rcpt.Search(iter_lot_id, &lot_rec) > 0 && lot_rec.GoodsID == goodsID) {
					if(lot_rec.LocID == locID) {
						lot_id = iter_lot_id;
					}
					else if(intrexp_op_id) {
						BillTbl::Rec bill_rec;
						if(p_bobj->Search(lot_rec.BillID, &bill_rec) > 0) {
							if(GetOpType(bill_rec.OpID) == rcpt_op_id && bill_rec.EdiOp == PPEDIOP_MRKTPLC_RECEIPT) {
								lot_candidate_for_intrexpend = iter_lot_id;
							}
							else if(!lot_candidate_for_intrexpend) {
								double down_lim = 0.0;
								double up_lim = 0.0;
								p_bobj->trfr->GetBounds(iter_lot_id, dt, -1, &down_lim, &up_lim);
								if(down_lim >= neededQtty) {
									lot_candidate_for_intrexpend = iter_lot_id;
								}
							}
						}
					}
				}
			}
			if(!lot_id) {
				// @v12.1.10 {
				if(lot_candidate_for_intrexpend) {
					assert(intrexp_op_id);
					{
						const  PPID ar_id = PPObjLocation::WarehouseToObj(locID);
						SString bill_code;
						PPBillPacket::SetupObjectBlock sob;
						PPBillPacket pack;
						bill_code.Z().Cat("TRANSF").CatChar('-').Cat(incomeId);
						THROW(p_bobj->trfr->Rcpt.Search(lot_candidate_for_intrexpend, &lot_rec) > 0 && lot_rec.GoodsID == goodsID);
						{
							PPTransaction tra(use_ta);
							THROW(tra);
							THROW(pack.CreateBlank(intrexp_op_id, 0, lot_rec.LocID, 0));
							pack.Rec.Dt = dt;
							STRNSCPY(pack.Rec.Code, bill_code);
							THROW(pack.SetupObject(ar_id, sob));
							{
								{
									PPTransferItem ti;
									LongArray row_idx_list;
									ti.GoodsID = goodsID;
									ti.LotID = lot_candidate_for_intrexpend;
									ti.Cost = lot_rec.Cost;
									ti.Price = lot_rec.Price;
									ti.Quantity_ = -fabs(neededQtty);
									THROW(pack.InsertRow(&ti, &row_idx_list));
									assert(row_idx_list.getCount() == 1);
								}
								PPGetWord(PPWORD_AT_AUTO, 0, pack.SMemo);
								pack.InitAmounts();
								p_bobj->FillTurnList(&pack);
								THROW(p_bobj->TurnPacket(&pack, 0));
								assert(pack.GetTCount() == 1);
								THROW(pack.GetTCount() == 1);
							}
							THROW(tra.Commit());
							R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
							{
								const PPTransferItem & r_ti = pack.ConstTI(0);
								TransferTbl::Rec trfr_rec;
								if(p_bobj->trfr->SearchByBill(r_ti.BillID, 1, r_ti.RByBill, &trfr_rec) > 0) {
									result_lot_id = trfr_rec.LotID;
									lot_found = true;
								}
							}
						}
					}
				}
				// } @v12.1.10 
			}
			else {
				double adj_cost = 0.0;
				{
					uint   _pos = 0;
					PPID   own_lot_id = 0;
					ReceiptTbl::Rec own_lot_rec;
					if(MpLotToOwnLotAssocList.Search(lot_id, &own_lot_id, &_pos)) {
						if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
							adj_cost = own_lot_rec.Cost;
						}
					}
					else if(SearchOriginalLotForMp(serial_buf, dt, locID, goodsID, &own_lot_id) > 0) {
						if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
							MpLotToOwnLotAssocList.Add(lot_id, own_lot_id);
							adj_cost = own_lot_rec.Cost;
						}							
					}
				}
				bool   do_update = false;
				double adj_price = lot_rec.Price;
				double adj_qtty = lot_rec.Quantity;
				LDATE  adj_date = lot_rec.Dt;
				assert(lot_rec.ID == lot_id);
				assert(lot_rec.LocID == locID);
				PPID   rcpt_bill_id = lot_rec.BillID;
				double down_lim = 0.0;
				double up_lim = 0.0;
				if(lot_rec.Dt > dt) {
					adj_date = dt;
					do_update = true;
					//
					down_lim = lot_rec.Rest; // Если дата лота превышала дату создаваемой расходной операции, то остаток, 
						// доступный для расхода равен тепкущему остатку лота (с учетом того факта, что мы переместим лот назад по времени)
				}
				else {
					p_bobj->trfr->GetBounds(lot_id, dt, -1, &down_lim, &up_lim);
				}
				if(down_lim < neededQtty) {
					adj_qtty = lot_rec.Quantity + (neededQtty - down_lim);
					do_update = true;
				}
				if(nominalPrice > 0.0 && nominalPrice != lot_rec.Price) {
					adj_price = nominalPrice;
					do_update = true;
				}
				if(adj_cost > 0.0 && adj_cost != lot_rec.Cost) {
					do_update = true;
				}
				if(do_update) {
					PPBillPacket rcpt_bill_pack;
					THROW(p_bobj->ExtractPacketWithFlags(rcpt_bill_id, &rcpt_bill_pack, BPLD_FORCESERIALS) > 0);
					{
						uint   ti_pos = 0;
						THROW(rcpt_bill_pack.SearchLot(lot_id, &ti_pos));
						if(checkdate(adj_date)) {
							rcpt_bill_pack.Rec.Dt = adj_date;
						}
						if(adj_qtty) {
							rcpt_bill_pack.TI(ti_pos).Quantity_ = adj_qtty;
						}
						if(adj_price > 0.0) {
							rcpt_bill_pack.TI(ti_pos).Price = adj_price;
						}
						if(adj_cost > 0.0) {
							rcpt_bill_pack.TI(ti_pos).Cost = adj_cost;
						}
						const int upr = p_bobj->UpdatePacket(&rcpt_bill_pack, 1/*use_ta*/);
						THROW(upr);
						if(upr > 0) {
							// @v12.2.6 R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, rcpt_bill_pack.Rec.ID, 1/*upd*/);
						}
					}
				}
				result_lot_id = lot_id;
				lot_found = true;
			}
		}
		if(!lot_found) {
			result_lot_id = CreateReceipt(incomeId, rW, dt, locID, goodsID, fabs(neededQtty), true, 1);
		}
	}
	CATCH
		result_lot_id = 0;
	ENDCATCH
	return result_lot_id;
}

int PPMarketplaceInterface_Wildberries::ImportSales()
{
	int    ok = -1;
	bool   debug_mark = false; // @debug
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	SString fmt_buf;
	SString msg_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::Sale> sale_list;
	FaultStatusResult error_status;
	const PPID order_op_id = R_Prc.GetOrderOpID();
	const PPID sale_op_id = R_Prc.GetSaleOpID();
	const PPID ret_op_id = R_Prc.GetRetOpID();
	THROW(sale_op_id); // @todo @err
	if(!RequestSales(sale_list, &error_status)) {
		;
	}
	else if(sale_list.getCount()) {
		SString bill_code;
		SString ord_bill_code;
		SString serial_buf;
		SString ord_serial_buf;
		SString org_order_ident; // Оригинальный идентификатор строки заказа
		PPIDArray mp_lot_list; // Список идентификаторов лотов на маркетплейсе, "увиденных" при обработке документов
		for(uint i = 0; i < sale_list.getCount(); i++) {
			const PPMarketplaceInterface_Wildberries::Sale * p_wb_item = sale_list.at(i);
			if(p_wb_item) {
				const bool is_return = (p_wb_item->SaleId.C(0) == 'R' && p_wb_item->FinishedPrice < 0.0);
				PPID   wh_id = 0;
				const  double seller_part_amount = p_wb_item->ForPay;
				BillTbl::Rec ord_bill_rec;
				BillTbl::Rec ex_bill_rec;
				ResolveWarehouseByName(WhList, p_wb_item->WarehouseName, LConfig.Location, &wh_id, 1/*use_ta*/);
				bill_code.Z().Cat(p_wb_item->SaleId);
				ord_bill_code.Z().Cat(p_wb_item->GNumber);
				// @v12.1.11 {
				const PPID goods_id = CreateWare(p_wb_item->Ware, 1/*use_ta*/);
				serial_buf.Z();
				if(p_wb_item->IncomeID && p_wb_item->Ware.ID)
					MakeSerialIdent(p_wb_item->IncomeID, p_wb_item->Ware, serial_buf);
				if(!goods_id) {
					// @todo @err
				}
				else if(is_return && !ret_op_id) {
					// @todo @err
				} // } @v12.1.11
				else if(p_bobj->P_Tbl->SearchByCode(bill_code, is_return ? ret_op_id : sale_op_id, ZERODATE, &ex_bill_rec) > 0) {
					// Документ уже акцептирован.
					// Увы, все равно придется его открыть - нужен список лотов, которые он использует и, возможно, надо установить какие-то суммы.
					PPBillPacket ex_pack;
					THROW(p_bobj->ExtractPacketWithFlags(ex_bill_rec.ID, &ex_pack, BPLD_FORCESERIALS) > 0);
					{
						for(uint tiidx = 0; tiidx < ex_pack.GetTCount(); tiidx++) {
							const PPID lot_id = ex_pack.ConstTI(tiidx).LotID;
							if(lot_id > 0)
								mp_lot_list.add(lot_id);
						}
					}
					if(seller_part_amount != 0.0) {
						double ex_seller_part_amount = ex_pack.Amounts.Get(PPAMT_MP_SELLERPART, 0);
						if(ex_seller_part_amount != seller_part_amount) {
							ex_pack.Amounts.Put(PPAMT_MP_SELLERPART, 0, seller_part_amount, 1, 1);
							p_bobj->FillTurnList(&ex_pack);
							const int upr = p_bobj->UpdatePacket(&ex_pack, 1);
							if(upr > 0) {
								// @v12.2.6 R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, ex_pack.Rec.ID, 1/*upd*/);
							}
							else if(!upr) {
								R_Prc.GetLogger().LogLastError();
							}
						}
					}
				}
				else if(p_bobj->P_Tbl->SearchByCode(ord_bill_code, order_op_id, ZERODATE, &ord_bill_rec) > 0) {
					PPBillPacket ord_pack;
					PPBillPacket pack;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;
					PPID   ar_id = CreateBuyer(p_wb_item, 1/*use_ta*/);
					THROW(p_bobj->ExtractPacketWithFlags(ord_bill_rec.ID, &ord_pack, BPLD_FORCESERIALS) > 0);
					if(is_return) {
						bool local_fault = false;
						PPIDArray shipm_bill_id_list;
						PPBillPacket shipm_pack;
						int   shipm_rbb = -1; // Номер строки документа отгрузки, с которой связан возврат
						PPID  src_lot_id = 0;
						//p_bobj->GetShipmByOrder(ord_bill_rec.ID, 0, shipm_bill_id_list);
						const int fsr = FindShipmentBillByOrderIdent(p_wb_item->SrID, shipm_bill_id_list);
						PPOprKind ret_op_rec;
						PPID  link_bill_id = 0;
						LongArray row_idx_list;
						THROW(GetOpData(ret_op_id, &ret_op_rec) > 0);
						if(ret_op_rec.LinkOpID) {
							if(shipm_bill_id_list.getCount() == 1) {
								link_bill_id = shipm_bill_id_list.get(0);
								if(p_bobj->ExtractPacketWithFlags(link_bill_id, &shipm_pack, BPLD_FORCESERIALS) > 0) {
									row_idx_list.Z();
									if(shipm_pack.LTagL.SearchString(p_wb_item->SrID, PPTAG_LOT_ORGORDERIDENT, 0, row_idx_list)) {
										if(row_idx_list.getCount() == 1) {
											shipm_rbb = row_idx_list.get(0);
											src_lot_id = shipm_pack.ConstTI(shipm_rbb).LotID;
										}
										else
											local_fault = true;
									}
									else
										local_fault = true;
								}
								else
									local_fault = true;
							}
							else {
								local_fault = true;
							}
						}
						if(!local_fault) {
							if(!pack.CreateBlank_WithoutCode(ret_op_id, link_bill_id, wh_id, 1)) {
								R_Prc.GetLogger().LogLastError();
							}
							else {
								pack.Rec.Dt = ValidDateOr(p_wb_item->Dtm.d, getcurdate_());
								STRNSCPY(pack.Rec.Code, bill_code);
								if(ar_id) {
									pack.SetupObject(ar_id, sob);
								}
								ReceiptTbl::Rec lot_rec;
								PPTransferItem ti;
								ti.Init(&pack.Rec);
								ti.SetupGoods(goods_id);
								if(p_bobj->trfr->Rcpt.Search(src_lot_id, &lot_rec) > 0 && ti.SetupLot(src_lot_id, &lot_rec, 0)) {
									ti.Quantity_ = 1.0;
									if(shipm_rbb >= 0) {
										const PPTransferItem & r_shipm_ti = shipm_pack.ConstTI(shipm_rbb);
										ti.Cost = r_shipm_ti.Cost;
										ti.Price = r_shipm_ti.Price;
										ti.Discount = r_shipm_ti.Discount;
									}
									else {
										ti.Cost = lot_rec.Cost;
										ti.Price = lot_rec.Price;
										ti.Discount = (ti.Price - p_wb_item->FinishedPrice);
									}
									row_idx_list.Z();
									pack.InsertRow(&ti, &row_idx_list);
									{
										const long new_row_idx = 0;//row_idx_list.get(0);
										if(p_wb_item->PriceWithDiscount < 0.0)
											pack.LTagL.SetReal(PPTAG_LOT_MP_SELLERSPRICE, new_row_idx, &p_wb_item->PriceWithDiscount);
										pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, serial_buf);
										pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, temp_buf.Z().Cat(p_wb_item->SrID));
										if(p_wb_item->Spp != 0.0) {
											pack.LTagL.SetReal(PPTAG_LOT_MP_DISCOUNTPCT, new_row_idx, &p_wb_item->Spp);
										}
										pack.InitAmounts();
										if(p_wb_item->ForPay < 0.0) {
											pack.Amounts.Put(PPAMT_MP_SELLERPART, 0, p_wb_item->ForPay, 1, 1);
										}
										p_bobj->FillTurnList(&pack);
										pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_RETURN, "MP-WILDBERRIES", 0);
										if(p_bobj->TurnPacket(&pack, 1)) {
											R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
											ok = 1;
										}
										else {
											R_Prc.GetLogger().LogLastError();
										}
									}
								}
							}
						}
					}
					else {
						PPID   ex_bill_id = 0;
						if(!pack.CreateBlank_WithoutCode(sale_op_id, 0, wh_id, 1)) {
							R_Prc.GetLogger().LogLastError();
						}
						else {
							const double sold_quantity = 1.0; // 
							pack.Rec.Dt = ValidDateOr(p_wb_item->Dtm.d, getcurdate_());
							STRNSCPY(pack.Rec.Code, bill_code);
							if(ar_id) {
								pack.SetupObject(ar_id, sob);
							}
							//
							//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
							//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
							//
							double nominal_price = 0.0;
							if(p_wb_item->PriceWithDiscount > 0.0)
								nominal_price = p_wb_item->PriceWithDiscount;
							else if(p_wb_item->TotalPrice > 0.0)
								nominal_price = p_wb_item->TotalPrice;
							else
								nominal_price = p_wb_item->FinishedPrice;
							// Предполагаем, что на маркетплейсе заказ может прийти только тогда, когда товар уже есть на складе.
							// Таким образом, дата прихода должна быть не больше даты заказа.
							const  LDATE lot_date = ValidDateOr(ord_pack.Rec.Dt, pack.Rec.Dt);
							PPID   lot_id = AdjustReceiptOnExpend(p_wb_item->Ware, p_wb_item->IncomeID, lot_date, wh_id, goods_id, sold_quantity, nominal_price, 1/*use_ta*/);
							if(lot_id) {
								mp_lot_list.add(lot_id);
								uint   ord_ti_idx = 0; // [1..]
								org_order_ident.Z(); // Оригинальный идентификатор строки заказа
								for(uint oti = 0; !ord_ti_idx && oti < ord_pack.GetTCount(); oti++) {
									const PPTransferItem & r_ord_ti = ord_pack.ConstTI(oti);
									if(labs(r_ord_ti.GoodsID) == goods_id) {
										ord_pack.LTagL.GetString(PPTAG_LOT_SN, oti, ord_serial_buf);
										ord_pack.LTagL.GetString(PPTAG_LOT_ORGORDERIDENT, oti, org_order_ident);
										if(p_wb_item->SrID.NotEmpty()) {
											if(org_order_ident.IsEqiUtf8(p_wb_item->SrID)) {
												ord_ti_idx = oti+1;
											}
										}
										else if(p_wb_item->IncomeID != 0) {
											if(ord_serial_buf.ToInt64() == p_wb_item->IncomeID) {
												ord_ti_idx = oti+1;
											}
										}
									}
								}
								if(!ord_ti_idx) {
									// @todo @err
									// Не удалось найти заказ, который закрывается документом продажи %s
									PPLoadText(PPTXT_MP_ORDERFORSALESITEMNFOUND, fmt_buf);
									(temp_buf = bill_code).CatDiv('-', 1).Cat(p_wb_item->Dtm.d, DATF_DMY);
									msg_buf.Printf(fmt_buf, temp_buf.cptr());
									R_Prc.GetLogger().Log(msg_buf);
								}
								else {
									const int isibor = p_bobj->InsertShipmentItemByOrder(&pack, &ord_pack, static_cast<int>(ord_ti_idx)-1, lot_id/*srcLotID*/, 1.0, PPObjBill::isibofErrOnCompletedOrder);
									if(isibor > 0) {
										const long new_row_idx = 0;//row_idx_list.get(0);
										ReceiptTbl::Rec lot_rec;
										if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
											PPTransferItem & r_ti = pack.TI(new_row_idx);
											r_ti.Cost = lot_rec.Cost;
											r_ti.Price = lot_rec.Price;
											r_ti.Discount = (r_ti.Price - p_wb_item->FinishedPrice);
										}
										// @v12.1.9 {
										if(p_wb_item->PriceWithDiscount > 0.0)
											pack.LTagL.SetReal(PPTAG_LOT_MP_SELLERSPRICE, new_row_idx, &p_wb_item->PriceWithDiscount);
										// } @v12.1.9 
										pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, serial_buf);
										pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, temp_buf.Z().Cat(p_wb_item->SrID));
										if(p_wb_item->Spp != 0.0) {
											pack.LTagL.SetReal(PPTAG_LOT_MP_DISCOUNTPCT, new_row_idx, &p_wb_item->Spp);
										}
										pack.InitAmounts();
										if(p_wb_item->ForPay > 0.0) {
											pack.Amounts.Put(PPAMT_MP_SELLERPART, 0, p_wb_item->ForPay, 1, 1);
										}
										p_bobj->FillTurnList(&pack);
										pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_SALE, "MP-WILDBERRIES", 0);
										if(p_bobj->TurnPacket(&pack, 1)) {
											R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
											ok = 1;
										}
										else {
											R_Prc.GetLogger().LogLastError();
										}
									}
									else if(isibor < 0) {
										debug_mark = true;
									}
									else {
										R_Prc.GetLogger().LogLastError();
									}
								}
							}
						}
					}
				}
				else {
					// Документ заказа не найден
					PPLoadText(PPTXT_MP_ORDERFORSALESITEMNFOUND, fmt_buf);
					(temp_buf = bill_code).CatDiv('-', 1).Cat(p_wb_item->Dtm.d, DATF_DMY);
					msg_buf.Printf(fmt_buf, temp_buf.cptr());
					R_Prc.GetLogger().Log(msg_buf);
				}
			}
		}
		if(mp_lot_list.getCount()) {
			mp_lot_list.sortAndUndup();
			for(uint i = 0; i < mp_lot_list.getCount(); i++) {
				const PPID lot_id = mp_lot_list.get(i);
				ReceiptTbl::Rec lot_rec;
				if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
					if(lot_rec.Cost == 0.0) {
						double adj_cost = 0.0;
						{
							uint   _pos = 0;
							PPID   own_lot_id = 0;
							ReceiptTbl::Rec own_lot_rec;
							if(MpLotToOwnLotAssocList.Search(lot_id, &own_lot_id, &_pos)) {
								if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
									adj_cost = own_lot_rec.Cost;
								}
							}
							else {
								PPID   own_lot_id = 0;
								p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_id, PPTAG_LOT_SN, serial_buf);
								if(SearchOriginalLotForMp(serial_buf, lot_rec.Dt, lot_rec.LocID, lot_rec.GoodsID, &own_lot_id) > 0) {
									if(p_bobj->trfr->Rcpt.Search(own_lot_id, &own_lot_rec) > 0) {
										MpLotToOwnLotAssocList.Add(lot_id, own_lot_id);
										adj_cost = own_lot_rec.Cost;
									}							
								}
							}
						}
						if(adj_cost > 0.0) {
							PPBillPacket rcpt_bpack;
							if(p_bobj->ExtractPacketWithFlags(lot_rec.BillID, &rcpt_bpack, BPLD_FORCESERIALS) > 0) {
								uint   tiidx = 0;
								if(rcpt_bpack.SearchLot(lot_id, &tiidx)) {
									rcpt_bpack.TI(tiidx).Cost = adj_cost;
									rcpt_bpack.InitAmounts();
									p_bobj->FillTurnList(&rcpt_bpack);
									const int upr = p_bobj->UpdatePacket(&rcpt_bpack, 1);
									THROW(upr);
									if(upr > 0) {
										// @v12.2.6 R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, rcpt_bpack.Rec.ID, 1/*upd*/);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::ImportOrders()
{
	int    ok = -1;
	bool   debug_mark = false;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	SString fmt_buf;
	SString msg_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::Sale> order_list;
	FaultStatusResult error_status;
	if(!RequestOrders(order_list, &error_status)) {
		;
	}
	else if(order_list.getCount()) {
		const PPID order_op_id = R_Prc.GetOrderOpID();
		THROW(order_op_id); // @todo @err
		{
			SString bill_code;
			LongArray seen_idx_list;
			for(uint ord_list_idx = 0; ord_list_idx < order_list.getCount(); ord_list_idx++) {
				if(!seen_idx_list.lsearch(static_cast<long>(ord_list_idx))) {
					seen_idx_list.add(static_cast<long>(ord_list_idx));
					const PPMarketplaceInterface_Wildberries::Sale * p_wb_item = order_list.at(ord_list_idx);
					if(p_wb_item) {
						PPID   wh_id = 0;
						BillTbl::Rec ex_bill_rec;
						if(p_wb_item->SrID == "20977596600611035.0.0") {
							debug_mark = true;
						}
						ResolveWarehouseByName(WhList, p_wb_item->WarehouseName, LConfig.Location, &wh_id, 1/*use_ta*/);
						bill_code.Z().Cat(p_wb_item->GNumber);
						if(p_bobj->P_Tbl->SearchByCode(bill_code, order_op_id, ZERODATE, &ex_bill_rec) > 0) {
							if(!(ex_bill_rec.Flags2 & BILLF2_DECLINED) && p_wb_item->Flags & Sale::fIsCancel) {
								PPBillPacket ex_pack;
								if(p_bobj->ExtractPacketWithFlags(ex_bill_rec.ID, &ex_pack, BPLD_FORCESERIALS) > 0) {
									ex_pack.Rec.Flags2 |= BILLF2_DECLINED;
									ex_pack.InitAmounts();
									p_bobj->FillTurnList(&ex_pack);
									if(p_bobj->UpdatePacket(&ex_pack, 1)) {
										PPObjBill::MakeCodeString(&ex_pack.Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddObjName, temp_buf);
										PPLoadText(PPTXT_ORDERCANCELLED, fmt_buf);
										msg_buf.Printf(fmt_buf, temp_buf.cptr());
										R_Prc.GetLogger().Log(msg_buf);
										ok = 1;
									}
									else {
										R_Prc.GetLogger().LogLastError();
									}

								}
							}
						}
						else {
							PPBillPacket pack;
							PPID   ex_bill_id = 0;
							Goods2Tbl::Rec goods_rec;
							PPBillPacket::SetupObjectBlock sob;
							PPID   ar_id = CreateBuyer(p_wb_item, 1/*use_ta*/);
							if(pack.CreateBlank_WithoutCode(order_op_id, 0, wh_id, 1)) {
								pack.Rec.Dt = ValidDateOr(p_wb_item->Dtm.d, getcurdate_());
								STRNSCPY(pack.Rec.Code, bill_code);
								if(ar_id) {
									PPBillPacket::SetupObjectBlock sob;
									pack.SetupObject(ar_id, sob);
								}
								//
								//pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
								//sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
								//
								PPID   goods_id = CreateWare(p_wb_item->Ware, 1/*use_ta*/);
								if(!goods_id) {
									// @todo @err
								}
								else {
									PPTransferItem ti;
									LongArray row_idx_list;
									ti.GoodsID = goods_id;
									// @v12.1.9 ti.Price = p_wb_item->FinishedPrice;
									// @v12.1.9 {
									ti.Price = p_wb_item->PriceWithDiscount;
									ti.Discount = p_wb_item->PriceWithDiscount - p_wb_item->FinishedPrice;
									// } @v12.1.9
									ti.Quantity_ = 1.0;
									if(pack.InsertRow(&ti, &row_idx_list)) {
										{
											assert(row_idx_list.getCount() == 1);
											const long new_row_idx = row_idx_list.get(0);
											pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, MakeSerialIdent(p_wb_item->IncomeID, p_wb_item->Ware, temp_buf));
											pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, p_wb_item->SrID);
										}
										{
											//
											// Если покупатель заказал несколько штук одного и того же, то каждая штука
											// будет задана отдельным документом заказа, но все эти документы будут содержать одно и то же значение GNumber
											//
											for(uint j = ord_list_idx+1; j < order_list.getCount(); j++) {
												const PPMarketplaceInterface_Wildberries::Sale * p_wb_item_inner = order_list.at(j);
												if(p_wb_item_inner && p_wb_item_inner->GNumber.IsEqiUtf8(p_wb_item->GNumber)) {
													if(!seen_idx_list.lsearch(static_cast<long>(j))) {
														seen_idx_list.add(static_cast<long>(j));
														PPTransferItem ti_inner;
														const PPID inner_goods_id = CreateWare(p_wb_item_inner->Ware, 1/*use_ta*/);
														if(!inner_goods_id) {
															// @todo @err
														}
														else {
															ti_inner.GoodsID = inner_goods_id;
															// @v12.1.9 ti_inner.Price = p_wb_item_inner->FinishedPrice;
															// @v12.1.9 {
															ti_inner.Price = p_wb_item_inner->PriceWithDiscount;
															ti_inner.Discount = p_wb_item_inner->PriceWithDiscount - p_wb_item_inner->FinishedPrice;
															// } @v12.1.9 
															ti_inner.Quantity_ = 1.0;
															row_idx_list.Z();
															if(pack.InsertRow(&ti_inner, &row_idx_list)) {
																assert(row_idx_list.getCount() == 1);
																const long new_row_idx = row_idx_list.get(0);
																pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, MakeSerialIdent(p_wb_item_inner->IncomeID, p_wb_item_inner->Ware, temp_buf));
																pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, p_wb_item_inner->SrID);
																if(p_wb_item_inner->Spp != 0.0) {
																	pack.LTagL.SetReal(PPTAG_LOT_MP_DISCOUNTPCT, new_row_idx, &p_wb_item_inner->Spp);
																}
															}
														}
													}
												}
											}
										}
										//
										if(p_wb_item->Flags & Sale::fIsCancel) {
											pack.Rec.Flags2 |= BILLF2_DECLINED;
										}
										pack.InitAmounts();
										p_bobj->FillTurnList(&pack);
										pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_ORDER, "MP-WILDBERRIES", 0);
										if(p_bobj->TurnPacket(&pack, 1)) {
											R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
											ok = 1;
										}
										else {
											R_Prc.GetLogger().LogLastError();
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
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::FindShipmentBillByOrderIdent(const char * pOrgOrdIdent, PPIDArray & rShipmBillIdList)
{
	int    ok = -1;
	rShipmBillIdList.Z();
	if(!isempty(pOrgOrdIdent)) {
		Reference * p_ref = PPRef;
		PPObjBill * p_bobj = BillObj;
		Transfer * p_trfr = p_bobj->trfr;
		PPIDArray lot_id_list;
		PPIDArray temp_list;
		p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_ORGORDERIDENT, pOrgOrdIdent, &lot_id_list);
		if(lot_id_list.getCount()) {
			PPIDArray bill_id_list;
			for(uint i = 0; i < lot_id_list.getCount(); i++) {
				const PPID ord_lot_id = lot_id_list.get(i);
				ReceiptTbl::Rec ord_lot_rec;
				if(p_trfr->Rcpt.Search(ord_lot_id, &ord_lot_rec) > 0) {
					p_bobj->GetShipmByOrder(ord_lot_rec.BillID, 0/*DateRange*/, temp_list);
					bill_id_list.add(&temp_list);
				}
			}
			if(bill_id_list.getCount()) {
				bill_id_list.sortAndUndup();
				for(uint i = 0; i < bill_id_list.getCount(); i++) {
					const PPID bill_id = bill_id_list.get(i);
					PPLotTagContainer ltc;
					p_bobj->LoadRowTagListForDraft(bill_id, ltc);
					LongArray row_idx_list;
					if(ltc.SearchString(pOrgOrdIdent, PPTAG_LOT_ORGORDERIDENT, 0, row_idx_list)) {
						rShipmBillIdList.add(bill_id);
					}
					//ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARINFB, ref_b);
					//ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_mark);					
				}
				if(rShipmBillIdList.getCount() == 1) {
					ok = 1;
				}
				else if(rShipmBillIdList.getCount() > 1) {
					ok = -2;
				}
			}
		}
	}
	return ok;
}

int PPMarketplaceInterface_Wildberries::ImportStocks() // @construction	
{
	int    ok = -1;
	PPID   stock_op_id = 0; // Вид операции фиксации остатков (драфт-документ, чаще всего это - PPOPK_EDI_STOCK)
	PPObjOprKind op_obj;
	TSCollection <Stock> stock_list;
	FaultStatusResult error_status;
	{
		PPID   op_id = 0;
		if(op_obj.GetEdiStockOp(&op_id, 1)) {
			stock_op_id = op_id;
		}
	}
	if(stock_op_id) {
		int    r = RequestStocks(stock_list, &error_status);
		if(r && stock_list.getCount()) {
			PPObjBill * p_bobj = BillObj;
			BillCore * p_billc = p_bobj->P_Tbl;
			PPIDArray wh_list;
			PPTransaction tra(1);
			THROW(tra);
			{
				//
				// Сначала соберем список складов, на которых что-то есть, и лишь потом пробежим циклом по каждому складу и для каждого сформируем отдельный документ
				//
				for(uint i = 0; i < stock_list.getCount(); i++) {
					const Stock * p_item = stock_list.at(i);
					if(p_item && p_item->Qtty > 0.0) { // Не используем QttyFull поскольку нам интересен только фактический остаток (без учета того что движется на склад/со склада)
						PPID   wh_id = 0;
						if(ResolveWarehouseByName(WhList, p_item->WarehouseName, 0/*default*/, &wh_id, 0/*use_ta*/) > 0) {
							wh_list.add(wh_id);
						}
						else
							R_Prc.GetLogger().LogLastError();
					}
				}
			}
			if(wh_list.getCount()) {
				wh_list.sortAndUndup();
				const LDATETIME now_dtm = getcurdatetime_();
				for(uint whidx = 0; whidx < wh_list.getCount(); whidx++) {
					const PPID wh_id = wh_list.get(whidx);
					PPBillPacket bpack;
					bool is_bpack_inited = false;
					for(uint i = 0; i < stock_list.getCount(); i++) {
						const Stock * p_item = stock_list.at(i);
						if(p_item && p_item->Qtty > 0.0) { // Не используем QttyFull поскольку нам интересен только фактический остаток (без учета того что движется на склад/со склада)
							PPID   local_wh_id = 0;
							if(ResolveWarehouseByName(WhList, p_item->WarehouseName, 0/*default*/, &local_wh_id, 0/*use_ta*/) > 0 && local_wh_id == wh_id) {
								PPID   goods_id = CreateWare(p_item->Ware, 0/*use_ta*/);
								if(!goods_id) {
									; // @todo что-то в log вывести
								}
								else {
									if(!is_bpack_inited) {
										{
											BillTbl::Key2 k2;
											k2.OpID = stock_op_id;
											k2.Dt   = now_dtm.d;
											k2.BillNo = 0;
											while(p_billc->search(2, &k2, spGt) && k2.OpID == stock_op_id && k2.Dt == now_dtm.d) {
												BillTbl::Rec ex_bill_rec;
												p_billc->copyBufTo(&ex_bill_rec);
												if(ex_bill_rec.LocID == wh_id) {
													THROW(p_bobj->RemovePacket(ex_bill_rec.ID, 0));
												}
											}
										}
										THROW(bpack.CreateBlank2(stock_op_id, now_dtm.d, wh_id, 0/*use_ta*/));
										bpack.SetupEdiAttributes(PPEDIOP_MRKTPLC_STOCK, "MP-WILDBERRIES", 0);
										is_bpack_inited = true;
									}
									PPTransferItem ti;
									const uint new_pos = bpack.GetTCount();
									THROW(ti.Init(&bpack.Rec, 1));
									THROW(ti.SetupGoods(goods_id, 0));
									ti.Quantity_ = p_item->Qtty;
									THROW(bpack.LoadTItem(&ti, 0, 0));
									{
										ObjTagList tag_list;
										if(p_item->InWayFromClient > 0.0)
											tag_list.PutItemReal(PPTAG_LOT_MP_STOCK_INWAYTO, p_item->InWayFromClient);
										if(p_item->InWayToClient > 0.0)
											tag_list.PutItemReal(PPTAG_LOT_MP_STOCK_INWAYFROM, p_item->InWayToClient);
										if(checkdate(p_item->DtmLastChange.d)) {
											ObjTagItem tag_item;
											if(tag_item.SetTimestamp(PPTAG_LOT_MP_STOCK_TIMESTAMP, p_item->DtmLastChange)) {
												tag_list.PutItem(PPTAG_LOT_MP_STOCK_TIMESTAMP, &tag_item);
											}
										}
										THROW(bpack.LTagL.Set(new_pos, &tag_list));
									}
								}
							}
						}
					}
					if(is_bpack_inited && bpack.GetTCount()) {
						bpack.InitAmounts();
						THROW(p_bobj->TurnPacket(&bpack, 0));						
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PPMarketplaceInterface_Wildberries::ImportFinancialTransactions()
{
	int    ok = -1;
	bool   debug_mark = false;
	Reference * p_ref = PPRef;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	SString bill_code;
	SString fmt_buf;
	SString msg_buf;
	TSCollection <PPMarketplaceInterface_Wildberries::SalesRepDbpEntry> sales_rep_dbp_list;
	FaultStatusResult error_status;
	DateRange period;
	PPID   acc_id = 0;
	PPID   acs_id = 0;
	PPID   op_id = 0;
	PPObjBill * p_bobj = BillObj;
	Transfer * p_trfr = p_bobj->trfr;
	PPObjOprKind op_obj;
	PPObjAccSheet acs_obj;
	PPObjAccount acc_obj;
	PPObjTag tag_obj;
	PPObjGlobalUserAcc gua_obj;
	{
		PPTransaction tra(1);
		THROW(tra);
		acs_id = R_Prc.Helper_GetMarketplaceOpsAccSheetID(true/*createIfNExists*/, true/*createArticles*/, 0);
		THROW(acs_id);
		{
			acc_id = R_Prc.Helper_GetMarketplaceOpsAccount(true, 0);
			THROW(acc_id);
		}
		op_obj.GetGenericAccTurnForRegisterOp(&op_id, 0/*use_ta*/);
		THROW(op_id);
		THROW(tra.Commit());
	}
	period.low.encode(1, 5, 2024);
	period.upp = now_dtm.d;
	//PPTXT_MP_FINTRA_WB_REQ               "Запрос отчета о продажах по реализации за период %s"
	PPLoadText(PPTXT_MP_FINTRA_WB_REQ, fmt_buf);
	PPFormatPeriod(&period, temp_buf);
	msg_buf.Printf(fmt_buf, temp_buf.cptr());
	R_Prc.GetLogger().Log(msg_buf);
	int r = RequestSalesReportDetailedByPeriod(period, sales_rep_dbp_list, &error_status);
	if(!r) {
		;
	}
	else if(!sales_rep_dbp_list.getCount()) {
		R_Prc.GetLogger().Log(PPLoadTextS(PPTXT_MP_FINTRA_WB_REPEMPTY, msg_buf));
	}
	else {
		// PPTXT_MPWB_NATIVEOPS
		// PUP - pick-up point (ПВЗ - пункт выдачи заказов)
		enum {
			nativeopCargo               =  1, // Возмещение издержек по перевозке/по складским операциям с товаром
			nativeopLogistics           =  2, // Логистика
			nativeopAcceptanceRecalc    =  3, // Пересчет платной приемки
			nativeopSales               =  4, // Продажа
			nativeopDeduction           =  5, // Удержание
			nativeopStorage             =  6, // Хранение
			nativeopLogisticsCorrection =  7, // @v12.2.0 Коррекция логистики 
			nativeopAcquiringCorrection =  8, // @v12.2.0 Корректировка эквайринга
			nativeopFine                =  9, // @v12.2.0 Штраф 
			nativeopReturn              = 10, // @v12.2.0 Возврат
			nativeopDamagesCompensation = 11, // @v12.2.1 Компенсация ущерба
			nativeopReimbursementAtPUP  = 12, // @v12.2.10 Возмещение за выдачу и возврат товаров на ПВЗ
			nativeopAcceptance          = 13, // @v12.3.3 Платная приемка
		};
		PPLoadTextUtf8(PPTXT_MPWB_NATIVEOPS, temp_buf);
		const StringSet ss_native_ops(';', temp_buf);
		SString id_buf;
		SString text_buf;
		PPIDArray shipm_bill_id_list;
		LongArray seen_pos_list; // список обработанных позиций массива sales_rep_dbp_list. 
		StringSet ss_order_ident;
		const char * p_empty_sr_ident_symb = "$empty-sr-id";
		{
			for(uint i = 0; i < sales_rep_dbp_list.getCount(); i++) {
				const SalesRepDbpEntry * p_entry = sales_rep_dbp_list.at(i);
				if(p_entry) {
					temp_buf.Z();
					if(p_entry->SrID.NotEmpty()) {
						temp_buf = p_entry->SrID;
					}
					else {
						temp_buf = p_empty_sr_ident_symb;
					}
					ss_order_ident.add(temp_buf);
				}
			}
			ss_order_ident.sortAndUndup();
		}
		SString sr_ident;
		for(uint ssp = 0; ss_order_ident.get(&ssp, sr_ident);) {
			const bool is_empty_sr_ident = (sr_ident == p_empty_sr_ident_symb);
			const int fsr = is_empty_sr_ident ? -1 : FindShipmentBillByOrderIdent(sr_ident, shipm_bill_id_list);
			PPID  shipm_bill_id = 0;
			PPBillPacket bpack;
			int   row_idx = -1;
			PPTransferItem * p_ti = 0;
			bool is_bpack_updated = false;
			double freight_amount = 0.0;
			if(fsr > 0) {
				assert(shipm_bill_id_list.getCount() == 1);
				shipm_bill_id = shipm_bill_id_list.get(0);
				if(p_bobj->ExtractPacketWithFlags(shipm_bill_id, &bpack, BPLD_FORCESERIALS) > 0) {
					LongArray row_list;
					bpack.LTagL.SearchString(sr_ident, PPTAG_LOT_ORGORDERIDENT, 0, row_list);
					if(row_list.getCount() == 1) {
						row_idx = row_list.get(0);
						p_ti = &bpack.TI(row_idx);
					}
				}
				else {
					;
				}
			}
			for(uint i = 0; i < sales_rep_dbp_list.getCount(); i++) {
				const SalesRepDbpEntry * p_entry = sales_rep_dbp_list.at(i);
				if(p_entry && ((is_empty_sr_ident && p_entry->SrID.IsEmpty()) || p_entry->SrID == sr_ident)) {
					int   native_op_id = 0;
					for(uint ssp = 0; !native_op_id && ss_native_ops.get(&ssp, temp_buf);) {
						if(temp_buf.Divide(',', id_buf, text_buf) > 0 && text_buf.IsEqiUtf8(p_entry->SupplOpName)) {
							native_op_id = id_buf.ToLong();
						}
					}
					if(!native_op_id) {
						// @todo Написать что-то в лог
						PPLoadText(PPTXT_MPWB_UNKN_NATIVEOP, fmt_buf);
						(temp_buf = p_entry->SupplOpName).Transf(CTRANSF_UTF8_TO_INNER);
						msg_buf.Printf(fmt_buf, temp_buf.cptr());
						R_Prc.GetLogger().Log(msg_buf);
					}
					else {
						PPID  loc_id = 0;
						ArticleTbl::Rec ar_rec;
						ResolveWarehouseByName(WhList, p_entry->Warehouse, LConfig.Location, &loc_id, 1/*use_ta*/);
						switch(native_op_id) {
							case nativeopCargo:
								// ppvz_vw, ppvz_vw_nds, rebill_logistic_cost
								// Насколько я понял из изучения материала, rebill_logistic_cost == ppvz_vw + ppvz_vw_nds
								if(bpack.Rec.ID) {
									double delivery_amount = p_entry->RebillLogisticCost;
									if(delivery_amount > 0.0) {
										freight_amount += delivery_amount;
									}
								}
								break;
							case nativeopLogistics:
								// delivery_amount, delivery_rub, 
								if(bpack.Rec.ID) {
									double delivery_amount = p_entry->DeliveryAmount;
									if(delivery_amount > 0.0) {
										freight_amount += delivery_amount;
									}
								}
								break;
							case nativeopAcceptanceRecalc:
							case nativeopAcceptance: // @v12.3.3
								// acceptance
								if(p_entry->Acceptance != 0.0) {
									bill_code.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(bill_code, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = ValidDateOr(p_entry->RrDtm.d, ValidDateOr(p_entry->CrDate, now_dtm.d));
											STRNSCPY(bpack_at.Rec.Code, bill_code);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_ACCEPTANCE, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->Acceptance);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												(bpack_at.SMemo = p_entry->SupplOpName).Transf(CTRANSF_UTF8_TO_INNER);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
							case nativeopReturn: // @v12.2.0
								if(bpack.Rec.ID) { // идентификатор документа продажи. Нам же нужен возврат по этому документу
									debug_mark = true;
									BillTbl::Rec ret_bill_rec;
									uint ret_count = 0;
									for(DateIter di; p_bobj->P_Tbl->EnumLinks(bpack.Rec.ID, &di, BLNK_RETURN, &ret_bill_rec) > 0;) {
										ret_count++;
									}
									if(ret_count) {
										if(ret_count > 1) {
											// В условиях wildberries это почти невозможно, но тем не менее, я должен пометить не нулевую вероятность
										}
										PPBillPacket ret_bpack;
										bool is_ret_bpack_updated = false;
										if(p_bobj->ExtractPacketWithFlags(ret_bill_rec.ID, &ret_bpack, BPLD_FORCESERIALS) > 0) {
											if(p_entry->CommissionPct != 0.0) {
												if(row_idx >= 0) {
													ret_bpack.LTagL.SetReal(PPTAG_LOT_MP_COMMISSIONPCT, row_idx, &p_entry->CommissionPct);
													if(p_entry->RetailPrice != 0.0) {
														double commission_amount = -fabs((p_entry->RetailPrice * p_entry->Qtty) * (p_entry->CommissionPct / 100.0));
														ret_bpack.Amounts.Put(PPAMT_MP_COMMISSION, 0, commission_amount, 1, 1);
													}
												}
											}
											if(p_entry->Ppvz_For_Pay != 0.0) {
												ret_bpack.Amounts.Put(PPAMT_MP_SELLERPART, 0, -fabs(p_entry->Ppvz_For_Pay), 1, 1);
												is_ret_bpack_updated = true;
											}
											/*if(p_entry->Ppvz_Vw != 0.0) {
												ret_bpack.Amounts.Put(PPAMT_MP_COMMISSION, 0, fabs(p_entry->Ppvz_Vw + p_entry->Ppvz_Vw_Vat), 1, 1);
												is_ret_bpack_update = true;
											}*/
											if(p_entry->AcquiringFee != 0.0) {
												ret_bpack.Amounts.Put(PPAMT_MP_ACQUIRING, 0, fabs(p_entry->AcquiringFee), 1, 1);
												is_ret_bpack_updated = true;
											}
											if(is_ret_bpack_updated) {
												// Возврат должны менять в отдельной ветке так как, это - другой (отличный от bpack.Rec.ID документ)
												p_bobj->FillTurnList(&ret_bpack);
												const int upr = p_bobj->UpdatePacket(&ret_bpack, 1);
												if(upr > 0) {
													// @v12.2.6 R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, ret_bpack.Rec.ID, 1/*upd*/);
												}
												else if(!upr) {
													R_Prc.GetLogger().LogLastError();
												}
											}
										}
									}
								}
								break;
							case nativeopSales:
								// quantity, retail_price, retail_price_withdisc_rub, retail_amount, commission_percent, ppvz_spp_prc, ppvz_kvw_prc_base, ppvz_kvw_prc, 
								// ppvz_sales_commission, ppvz_for_pay, acquiring_fee, acquiring_percent, ppvz_vw, ppvz_vw_nds	
								if(bpack.Rec.ID) {
									if(p_entry->CommissionPct != 0.0) {
										if(row_idx >= 0) {
											bpack.LTagL.SetReal(PPTAG_LOT_MP_COMMISSIONPCT, row_idx, &p_entry->CommissionPct);
											if(p_entry->RetailPrice > 0.0) {
												double commission_amount = (p_entry->RetailPrice * p_entry->Qtty) * (p_entry->CommissionPct / 100.0);
												bpack.Amounts.Put(PPAMT_MP_COMMISSION, 0, commission_amount, 1, 1);
											}
										}
									}
									if(p_entry->Ppvz_For_Pay != 0.0) {
										bpack.Amounts.Put(PPAMT_MP_SELLERPART, 0, p_entry->Ppvz_For_Pay, 1, 1);
										is_bpack_updated = true;
									}
									/*if(p_entry->Ppvz_Vw != 0.0) {
										bpack.Amounts.Put(PPAMT_MP_COMMISSION, 0, fabs(p_entry->Ppvz_Vw + p_entry->Ppvz_Vw_Vat), 1, 1);
										is_bpack_updated = true;
									}*/
									if(p_entry->AcquiringFee != 0.0) {
										bpack.Amounts.Put(PPAMT_MP_ACQUIRING, 0, fabs(p_entry->AcquiringFee), 1, 1);
										is_bpack_updated = true;
									}
								}
								break;
							case nativeopLogisticsCorrection: // @v12.2.0 Коррекция логистики 
								if(bpack.Rec.ID) {
									double delivery_amount = p_entry->DeliveryAmount;
									if(delivery_amount != 0.0) {
										freight_amount += delivery_amount;
									}
								}
								break;
							case nativeopDamagesCompensation: // @v12.2.1 Компенсация ущерба
								{
									const double amount = p_entry->Ppvz_For_Pay;
									PPID clost_op_id = R_Prc.GetCompensatedLostOp();
									if(!clost_op_id) {
										debug_mark = true;
									}
									else if(!loc_id) {
										debug_mark = true;
									}
									else if(amount == 0.0) {
										debug_mark = true;
									}
									else {
										const PPID acs_id = PrcssrMarketplaceInterchange::GetMarketplaceAccSheetID();
										SString serial_buf;
										bill_code.Z().Cat(p_entry->RrdId);
										BillTbl::Rec ex_bill_rec;
										if(p_bobj->P_Tbl->SearchByCode(bill_code, clost_op_id, ZERODATE, &ex_bill_rec) > 0) {
											;
										}
										else {
											PPTransaction tra(1);
											if(!!tra) {
												const PPID goods_id = CreateWare(p_entry->Ware, 0/*use_ta*/);
												if(goods_id) {
													serial_buf.Z();
													PPID   mp_ar_id = 0;
													PPOprKind clost_op_rec;
													GetOpData(clost_op_id, &clost_op_rec);
													if(acs_id && acs_id == clost_op_rec.AccSheetID) {
														PPID   mp_psn_id = 0;
														GetMarketplacePerson(&mp_psn_id, 0/*use_ta*/);
														if(mp_psn_id)
															ArObj.P_Tbl->PersonToArticle(mp_psn_id, acs_id, &mp_ar_id);
													}
													MakeSerialIdent(p_entry->IncomeID, p_entry->Ware, serial_buf); // nativeopDamagesCompensation
													const double qtty = fabs(p_entry->Qtty);
													const double nominal_price = 0.0;
													const LDATE  bill_date = ValidDateOr(p_entry->RrDtm.d, getcurdate_());
													// Предполагаем, что на маркетплейсе заказ может прийти только тогда, когда товар уже есть на складе.
													// Таким образом, дата прихода должна быть не больше даты заказа.
													const  LDATE lot_date = ValidDateOr(p_entry->OrderDtm.d, bill_date);
													PPID   lot_id = AdjustReceiptOnExpend(p_entry->Ware, p_entry->IncomeID, lot_date, loc_id, goods_id, qtty, nominal_price, 0/*use_ta*/);
													ReceiptTbl::Rec lot_rec;
													if(lot_id && p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
														PPBillPacket pack;
														if(!pack.CreateBlank_WithoutCode(clost_op_id, 0, loc_id, 0/*use_ta*/)) {
															R_Prc.GetLogger().LogLastError();
														}
														else {
															pack.Rec.Dt = bill_date;
															STRNSCPY(pack.Rec.Code, bill_code);
															if(mp_ar_id) {
																PPBillPacket::SetupObjectBlock sob;
																pack.SetupObject(mp_ar_id, sob);
															}
															{
																PPTransferItem ti;
																ti.Init(&pack.Rec);
																ti.GoodsID = labs(goods_id);
																ti.LotID = lot_id;
																ti.Quantity_ = -qtty;
																ti.Cost = lot_rec.Cost;
																ti.Price = lot_rec.Price;
																ti.Discount = R5(ti.Price - fabs(amount / qtty));
																LongArray row_idx_list;
																if(pack.InsertRow(&ti, &row_idx_list)) {
																	assert(row_idx_list.getCount() == 1);
																	const int new_row_idx = row_idx_list.at(0);
																	pack.LTagL.SetString(PPTAG_LOT_SN, new_row_idx, serial_buf);
																	pack.LTagL.SetString(PPTAG_LOT_ORGORDERIDENT, new_row_idx, sr_ident);
																	pack.InitAmounts();
																	if(amount != 0.0) {
																		pack.Amounts.Put(PPAMT_MP_SELLERPART, 0, amount, 1, 1);
																	}
																	p_bobj->FillTurnList(&pack);
																	pack.SetupEdiAttributes(PPEDIOP_MRKTPLC_CLOST, "MP-WILDBERRIES", 0);
																	if(p_bobj->TurnPacket(&pack, 0)) {
																		R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
																		ok = 1;
																	}
																	else {
																		R_Prc.GetLogger().LogLastError();
																	}
																}
															}
														}
													}
												}
												tra.Commit();
											}
										}
									}
								}
								break;
							case nativeopFine: // @v12.2.0
								if(p_entry->Penalty != 0.0) {
									bill_code.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(bill_code, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = ValidDateOr(p_entry->RrDtm.d, ValidDateOr(p_entry->CrDate, now_dtm.d));
											STRNSCPY(bpack_at.Rec.Code, bill_code);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_PENALTY, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->Penalty);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												if(p_entry->BonusTypeName.NotEmpty()) {
													(bpack_at.SMemo = p_entry->BonusTypeName).Transf(CTRANSF_UTF8_TO_INNER);
												}
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
							case nativeopDeduction:
								// deduction
								if(p_entry->Deduction != 0.0) {
									bill_code.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(bill_code, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = ValidDateOr(p_entry->RrDtm.d, ValidDateOr(p_entry->CrDate, now_dtm.d));
											STRNSCPY(bpack_at.Rec.Code, bill_code);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_DEDUCTION, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->Deduction);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												if(p_entry->BonusTypeName.NotEmpty()) {
													(bpack_at.SMemo = p_entry->BonusTypeName).Transf(CTRANSF_UTF8_TO_INNER);
												}
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
							case nativeopStorage:
								// storage_fee
								if(p_entry->StorageFee != 0.0) {
									bill_code.Z().Cat(p_entry->RrdId);
									BillTbl::Rec ex_bill_rec;
									if(p_bobj->P_Tbl->SearchByCode(bill_code, op_id, ZERODATE, &ex_bill_rec) > 0) {
										;
									}
									else {
										PPBillPacket bpack_at;
										{
											PPTransaction tra(1);
											THROW(tra);
											bpack_at.CreateBlank_WithoutCode(op_id, 0, loc_id, 0);
											bpack_at.Rec.Dt = ValidDateOr(p_entry->RrDtm.d, ValidDateOr(p_entry->CrDate, now_dtm.d));
											STRNSCPY(bpack_at.Rec.Code, bill_code);
											if(ArObj.P_Tbl->SearchNum(acs_id, ARTN_MRKTPLCACC_STORAGE, &ar_rec) > 0) {
												PPAccTurn at;
												bpack_at.CreateAccTurn(at);
												at.DbtID.ac = acc_id;
												at.DbtID.ar = ar_rec.ID;
												at.DbtSheet = acs_id;
												at.Amount = -fabs(p_entry->StorageFee);
												bpack_at.Turns.insert(&at);
												bpack_at.Rec.Amount = at.Amount;
												bpack_at.SetupEdiAttributes(0, "MP-WILDBERRIES", 0);
												THROW(p_bobj->TurnPacket(&bpack_at, 0));
											}
											THROW(tra.Commit());
										}
									}
								}
								break;
							case nativeopAcquiringCorrection: // @v12.2.0 Корректировка эквайринга
								{
									// @todo
									debug_mark = true;
								}
								break;
							case nativeopReimbursementAtPUP: // @v12.2.10 Возмещение за выдачу и возврат товаров на ПВЗ
								{
									//double Ppvz_Reward;           // ppvz_reward	number Возмещение за выдачу и возврат товаров на ПВЗ
									//double Ppvz_Vw;               // ppvz_vw number Вознаграждение WB без НДС
									//double Ppvz_Vw_Vat;           // ppvz_vw_nds number НДС с вознаграждения WB
									// В элементе p_entry значатся следующие суммы:
									// (Ppvz_Reward >0) & ((Ppvz_Vw+Ppvz_Vw_Vat) < 0) причем (Ppvz_Vw+Ppvz_Vw_Vat) == -Ppvz_Reward
									// @todo
									debug_mark = true;
								}								
								break;
						}
					}
				}
			}
			if(freight_amount != 0.0) {
				bpack.Amounts.Put(PPAMT_FREIGHT, 0L, freight_amount, 1/*ignoreZero*/, 1/*replace*/);
				is_bpack_updated = true;
			}
			if(is_bpack_updated) {
				//bpack.InitAmounts();
				p_bobj->FillTurnList(&bpack);
				const int upr = p_bobj->UpdatePacket(&bpack, 1);
				if(upr > 0) {
					// @v12.2.6 R_Prc.GetLogger().LogAcceptMsg(PPOBJ_BILL, bpack.Rec.ID, 1/*upd*/);
				}
				else if(!upr) {
					R_Prc.GetLogger().LogLastError();
				}
			}
		}
	}
	CATCH
		R_Prc.GetLogger().LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(MarketplaceInterchange); MarketplaceInterchangeFilt::MarketplaceInterchangeFilt() : PPBaseFilt(PPFILT_MARKETPLACEINTERCHANGE, 0, 0)
{
	SetFlatChunk(offsetof(MarketplaceInterchangeFilt, ReserveStart),
		offsetof(MarketplaceInterchangeFilt, Reserve) - offsetof(MarketplaceInterchangeFilt, ReserveStart) + sizeof(Reserve));
	Init(1, 0);
}

MarketplaceInterchangeFilt & FASTCALL MarketplaceInterchangeFilt::operator = (const MarketplaceInterchangeFilt & rS)
{
	PPBaseFilt::Copy(&rS, 0);
	return *this;
}

PPMarketplaceConfig::PPMarketplaceConfig() : Tag(PPOBJ_CONFIG), ID(PPCFG_MAIN), Prop(PPPRP_MRKTPLCCFG), OrderOpID(0), SalesOpID(0), TransferToMpOpID(0), Flags(0),
	CompensatedLostOpID(0)
{
	memzero(Reserve, sizeof(Reserve));
	memzero(Reserve2, sizeof(Reserve2));
}

PPMarketplaceConfig & PPMarketplaceConfig::Z()
{
	Tag = PPOBJ_CONFIG;
	ID = PPCFG_MAIN;
	Prop = PPPRP_MRKTPLCCFG;
	OrderOpID = 0;
	SalesOpID = 0;
	TransferToMpOpID = 0;
	CompensatedLostOpID = 0; // @v12.2.1
	Flags = 0;
	return *this;
}

bool PPMarketplaceConfig::IsEq_ForStorage(const PPMarketplaceConfig & rS) const
{
	return (OrderOpID == rS.OrderOpID && SalesOpID == rS.SalesOpID && ReturnOpID == rS.ReturnOpID && 
		TransferToMpOpID == rS.TransferToMpOpID && CompensatedLostOpID == rS.CompensatedLostOpID);
}

/*static*/int FASTCALL PrcssrMarketplaceInterchange::ReadConfig(PPMarketplaceConfig * pCfg)
{
	static_assert(sizeof(PPMarketplaceConfig) == PROPRECFIXSIZE);
	int    r = PPRef->GetPropMainConfig(PPPRP_MRKTPLCCFG, pCfg, sizeof(*pCfg));
	if(r > 0) {
		if(pCfg)
			pCfg->Flags |= PPMarketplaceConfig::fValid;
	}
	else {
		memzero(pCfg, sizeof(*pCfg));
	}
	return r;
}

/*static*/int FASTCALL PrcssrMarketplaceInterchange::WriteConfig(PPMarketplaceConfig * pCfg, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPMarketplaceConfig org_cfg;
	const  int  rcr = ReadConfig(&org_cfg);
	const  bool is_exists = (rcr > 0);
	PPTransaction tra(1);
	THROW(tra);
	if(pCfg) {
		if(pCfg->IsEq_ForStorage(org_cfg)) {
			;
		}
		else {
			PPMarketplaceConfig cfg(*pCfg);
			cfg.Flags &= ~PPMarketplaceConfig::fValid; // transient-flag
			THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_MRKTPLCCFG, &cfg, sizeof(cfg), 0));
			DS.LogAction(is_exists ? PPACN_CONFIGUPDATED : PPACN_CONFIGCREATED, PPCFGOBJ_MARKETPLACE, 0, 0, 0);
		}
	}
	else {
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_MRKTPLCCFG, 0, 0, 0));
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

/*static*/int PrcssrMarketplaceInterchange::AutoConfigure(PPMarketplaceConfig & rCfg, uint flags, int use_ta) // @v12.2.1 @construction
{
	//PPTXT_OPK_MP_ORDER                  "Заказ на маркетплейсе"                  // @v12.2.1
	//PPTXT_OPK_MP_SALE                   "Продажа на маркетплейсе"                // @v12.2.1
	//PPTXT_OPK_MP_RETURN                 "Возврат от покупателя на маркетплейсе"  // @v12.2.1
	//PPTXT_OPK_MP_COMPENSATEDLOST        "Потери на маркетплейсе с компенсацией"  // @v12.2.1
	//PPTXT_OPK_MP_TRANSFERTO             "Передача товара на маркетплейс"         // @v12.2.1
	int    ok = -1;
	SString temp_buf;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	PPID   acs_id = GetSellAccSheet();
	PPAlbatrossConfig albtr_cfg;
	const int acgr = PPAlbatrosCfgMngr::Get(&albtr_cfg);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!acs_id) {
			
		}
		THROW(acs_id); // @todo @err
		{
			{
				if(rCfg.OrderOpID > 0 && GetOpType(rCfg.OrderOpID) == PPOPT_GOODSORDER) {
					;
				}
				else {
					rCfg.OrderOpID = 0;
					if(flags & acfUseExOrderOp) {
						const PPID local_op_id = (acgr > 0) ? albtr_cfg.Hdr.OpID : 0;
						if(local_op_id && GetOpType(local_op_id) == PPOPT_GOODSORDER)
							rCfg.OrderOpID = local_op_id;
					}
					if(!rCfg.OrderOpID) {
						PPID   op_id = 0;
						PPOprKindPacket op_pack;
						PPLoadText(PPTXT_OPK_MP_ORDER, temp_buf);
						THROW(temp_buf.NotEmptyS());
						STRNSCPY(op_pack.Rec.Name, temp_buf);
						STRNSCPY(op_pack.Rec.Symb, "MP_ORDER");
						op_pack.Rec.AccSheetID = acs_id;
						op_pack.Rec.OpTypeID = PPOPT_GOODSORDER;
						op_pack.Rec.Flags |= OPKF_SELLING;
						op_pack.OpCntrPack.Init(0);
						STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, "MPO%06");
						op_pack.OpCntrPack.Head.Counter = 0;
						THROW(op_obj.PutPacket(&op_id, &op_pack, 0));					
						rCfg.OrderOpID = op_id;
					}
				}
			}
			if(rCfg.OrderOpID) {
				PPOprKind order_op_rec;
				THROW(GetOpData(rCfg.OrderOpID, &order_op_rec) > 0);
				if(rCfg.SalesOpID > 0 && GetOpType(rCfg.SalesOpID, &op_rec) == PPOPT_GOODSEXPEND && op_rec.AccSheetID == order_op_rec.AccSheetID && op_rec.Flags & OPKF_ONORDER) {
					;
				}
				else {
					rCfg.SalesOpID = 0;
					if(flags & acfUseExSaleOp) {
						PPID   result_id = 0;
						{
							for(PPID iter_op_id = 0; !result_id && EnumOperations(PPOPT_GOODSEXPEND, &iter_op_id, &op_rec) > 0;) {
								if(op_rec.Flags & OPKF_ONORDER && op_rec.AccSheetID == order_op_rec.AccSheetID) {
									result_id = op_rec.ID;
								}
							}
						}
						if(result_id)
							rCfg.SalesOpID = result_id;
					}
					if(!rCfg.SalesOpID) {
						PPID   op_id = 0;
						PPOprKindPacket op_pack;
						PPLoadText(PPTXT_OPK_MP_SALE, temp_buf);
						THROW(temp_buf.NotEmptyS());
						STRNSCPY(op_pack.Rec.Name, temp_buf);
						STRNSCPY(op_pack.Rec.Symb, "MP_SALE");
						op_pack.Rec.AccSheetID = acs_id;
						op_pack.Rec.OpTypeID = PPOPT_GOODSEXPEND;
						op_pack.Rec.Flags |= (OPKF_ONORDER|OPKF_PROFITABLE|OPKF_SELLING);
						op_pack.OpCntrPack.Init(0);
						STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, "MPS%06");
						op_pack.OpCntrPack.Head.Counter = 0;
						THROW(op_obj.PutPacket(&op_id, &op_pack, 0));
						rCfg.SalesOpID = op_id;					
					}
				}
				if(rCfg.SalesOpID) {
					PPOprKind sales_op_rec;
					THROW(GetOpData(rCfg.SalesOpID, &sales_op_rec) > 0);
					if(rCfg.ReturnOpID && GetOpType(rCfg.ReturnOpID, &op_rec) == PPOPT_GOODSRETURN && op_rec.LinkOpID == rCfg.SalesOpID) {
						;
					}
					else {
						PPID   op_id = 0;
						PPOprKindPacket op_pack;
						PPLoadText(PPTXT_OPK_MP_RETURN, temp_buf);
						THROW(temp_buf.NotEmptyS());
						STRNSCPY(op_pack.Rec.Name, temp_buf);
						STRNSCPY(op_pack.Rec.Symb, "MP_RETURN");
						op_pack.Rec.AccSheetID = acs_id;
						op_pack.Rec.OpTypeID = PPOPT_GOODSRETURN;
						op_pack.Rec.LinkOpID = rCfg.SalesOpID;
						op_pack.Rec.Flags |= OPKF_SELLING;
						op_pack.OpCntrPack.Init(0);
						STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, "MPR%06");
						op_pack.OpCntrPack.Head.Counter = 0;
						THROW(op_obj.PutPacket(&op_id, &op_pack, 0));
						rCfg.ReturnOpID = op_id;
					}
				}
			}
			if(!rCfg.TransferToMpOpID) {
				PPID wh_acs_id = Helper_GetMarketplaceWarehousesAccSheetID(true/*createIfNExists*/, 0/*use_ta*/);
				if(wh_acs_id) {
					PPID   op_id = 0;
					PPOprKindPacket op_pack;
					PPLoadText(PPTXT_OPK_MP_TRANSFERTO, temp_buf);
					THROW(temp_buf.NotEmptyS());
					STRNSCPY(op_pack.Rec.Name, temp_buf);
					STRNSCPY(op_pack.Rec.Symb, "MP_TRFRTO");
					op_pack.Rec.AccSheetID = wh_acs_id;
					op_pack.Rec.OpTypeID = PPOPT_GOODSEXPEND;
					op_pack.Rec.LinkOpID = 0;
					op_pack.Rec.Flags |= OPKF_BUYING;
					op_pack.OpCntrPack.Init(0);
					STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, "MPT%06");
					op_pack.OpCntrPack.Head.Counter = 0;
					THROW(op_obj.PutPacket(&op_id, &op_pack, 0));
					rCfg.TransferToMpOpID = op_id;					
				}
			}
			if(!rCfg.CompensatedLostOpID) {
				const PPID mp_acs_id = PrcssrMarketplaceInterchange::GetMarketplaceAccSheetID();
				PPID   op_id = 0;
				PPOprKindPacket op_pack;
				PPLoadText(PPTXT_OPK_MP_COMPENSATEDLOST, temp_buf);
				THROW(temp_buf.NotEmptyS());
				STRNSCPY(op_pack.Rec.Name, temp_buf);
				STRNSCPY(op_pack.Rec.Symb, "MP_COMPENSATEDLOST");
				op_pack.Rec.AccSheetID = mp_acs_id;
				op_pack.Rec.OpTypeID = PPOPT_GOODSEXPEND;
				op_pack.Rec.LinkOpID = 0;
				op_pack.Rec.Flags |= OPKF_PROFITABLE|OPKF_SELLING;
				op_pack.OpCntrPack.Init(0);
				STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, "MPC%06");
				op_pack.OpCntrPack.Head.Counter = 0;
				THROW(op_obj.PutPacket(&op_id, &op_pack, 0));
				rCfg.CompensatedLostOpID = op_id;				
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int PrcssrMarketplaceInterchange::EditConfig()
{
	class MpConfigDialog : public TDialog {
		DECL_DIALOG_DATA(PPMarketplaceConfig);
	public:
		MpConfigDialog() : TDialog(DLG_MPCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			PPIDArray op_type_list;
			RVALUEPTR(Data, pData);
			op_type_list.Z().add(PPOPT_GOODSORDER);
			SetupOprKindCombo(this, CTLSEL_MPCFG_ORDEROP, Data.OrderOpID, 0, &op_type_list, 0);
			op_type_list.Z().add(PPOPT_GOODSEXPEND);
			SetupOprKindCombo(this, CTLSEL_MPCFG_SALEOP, Data.SalesOpID, 0, &op_type_list, 0);
			// @v12.1.11 {
			op_type_list.Z().addzlist(PPOPT_GOODSRETURN, PPOPT_GOODSRECEIPT, 0L);
			SetupOprKindCombo(this, CTLSEL_MPCFG_RETOP, Data.ReturnOpID, 0, &op_type_list, 0);
			// } @v12.1.11 
			op_type_list.Z().add(PPOPT_GOODSEXPEND);
			SetupOprKindCombo(this, CTLSEL_MPCFG_TRANSFOP, Data.TransferToMpOpID, 0, &op_type_list, 0);
			// @v12.2.1 {
			op_type_list.Z().add(PPOPT_GOODSEXPEND);
			SetupOprKindCombo(this, CTLSEL_MPCFG_CLOSTOP, Data.CompensatedLostOpID, 0, &op_type_list, 0);
			// } @v12.2.1
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_MPCFG_ORDEROP, &Data.OrderOpID);
			getCtrlData(CTLSEL_MPCFG_SALEOP, &Data.SalesOpID);
			getCtrlData(CTLSEL_MPCFG_RETOP, &Data.ReturnOpID); // @v12.1.11
			getCtrlData(CTLSEL_MPCFG_TRANSFOP, &Data.TransferToMpOpID);
			getCtrlData(CTLSEL_MPCFG_CLOSTOP, &Data.CompensatedLostOpID); // @v12.2.1
			//
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmAutoConfigure)) {
				if(getDTS(0)) {
					PPMarketplaceConfig cfg(Data);
					if(PrcssrMarketplaceInterchange::AutoConfigure(cfg, 0, 1)) {
						setDTS(&cfg);
						if(PrcssrMarketplaceInterchange::WriteConfig(&cfg, 1)) {
							;
						}
						else {
							PPError();
						}
					}
				}
				clearEvent(event);
			}
		}
	};

	int    ok = -1;
	int    is_new = 0;
	PPMarketplaceConfig cfg;
	MpConfigDialog * dlg = new MpConfigDialog();
	THROW(CheckCfgRights(PPCFGOBJ_MARKETPLACE, PPR_READ, 0));
	THROW(is_new = ReadConfig(&cfg));
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(&cfg);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&cfg)) {
			THROW(CheckCfgRights(PPCFGOBJ_MARKETPLACE, PPR_MOD, 0));
			THROW(WriteConfig(&cfg, 1));
			ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

PrcssrMarketplaceInterchange::PrcssrMarketplaceInterchange() : P_Ifc(0), State(0)
{
}
	
PrcssrMarketplaceInterchange::~PrcssrMarketplaceInterchange()
{
	delete P_Ifc;
}

class MarketplaceInterchangeFiltDialog : public TDialog {
	DECL_DIALOG_DATA(MarketplaceInterchangeFilt);
public:
	MarketplaceInterchangeFiltDialog() : TDialog(DLG_MRKTPLCIX)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int   ok = 1;
		RVALUEPTR(Data, pData);
		SetupPPObjCombo(this, CTLSEL_MRKTPLCIX_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, 0, reinterpret_cast<void *>(PPGLS_WILDBERRIES));
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int   ok = 1;
		Data.GuaID = getCtrlLong(CTLSEL_MRKTPLCIX_GUA);
		//
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int PrcssrMarketplaceInterchange::EditParam(PPBaseFilt * pBaseFilt)
{
	MarketplaceInterchangeFilt temp_filt;
	if(!temp_filt.IsA(pBaseFilt))
		return 0;
	MarketplaceInterchangeFilt * p_filt = static_cast<MarketplaceInterchangeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(MarketplaceInterchangeFiltDialog, p_filt);
}

int PrcssrMarketplaceInterchange::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	// @nothingtodo
	return ok;
}

PPID PrcssrMarketplaceInterchange::GetOrderOpID()
{
	PPID   result_id = 0;
	if(Cfg.OrderOpID > 0) {
		result_id = Cfg.OrderOpID;
	}
	else {
		if(Cfg.OrderOpID == 0) {
			PPAlbatrossConfig albtr_cfg;
			result_id = (PPAlbatrosCfgMngr::Get(&albtr_cfg) > 0) ? albtr_cfg.Hdr.OpID : 0;
			if(result_id == 0)
				Cfg.OrderOpID = -1; // Индицирует факт того, что вид операции заказа получить не удается.
			else
				Cfg.OrderOpID = result_id;
		}
	}
	return result_id;
}

PPID PrcssrMarketplaceInterchange::GetSaleOpID()
{
	PPID   result_id = 0;
	if(Cfg.SalesOpID > 0)
		result_id = Cfg.SalesOpID;
	else {
		if(Cfg.SalesOpID == 0) {
			const PPID order_op_id = GetOrderOpID();
			PPOprKind order_op_rec;
			if(order_op_id) {
				if(GetOpData(order_op_id, &order_op_rec) > 0) {
					PPOprKind op_rec;
					for(PPID iter_op_id = 0; !result_id && EnumOperations(PPOPT_GOODSEXPEND, &iter_op_id, &op_rec) > 0;) {
						if(op_rec.Flags & OPKF_ONORDER && op_rec.AccSheetID == order_op_rec.AccSheetID) {
							result_id = op_rec.ID;
						}
					}
				}
			}
			if(result_id == 0)
				Cfg.SalesOpID = -1; // Индицирует факт того, что вид операции продажи получить не удается.
			else
				Cfg.SalesOpID = result_id;
		}
	}
	return result_id;
}

PPID PrcssrMarketplaceInterchange::GetCompensatedLostOp() // @v12.2.1
{
	PPID   result_id = 0;
	if(Cfg.CompensatedLostOpID > 0)
		result_id = Cfg.CompensatedLostOpID;
	else {
		// Пока не придумал способ автоматического выбора или генерации такого вида операции
	}
	return result_id;
}

PPID PrcssrMarketplaceInterchange::GetRetOpID() // @v12.1.11
{
	PPID   result_id = 0;
	if(Cfg.ReturnOpID > 0)
		result_id = Cfg.ReturnOpID;
	else {
		if(Cfg.ReturnOpID == 0) {
			PPID sale_op_id = GetSaleOpID();
			if(sale_op_id > 0) {
				PPOprKind op_rec;
				for(PPID iter_op_id = 0; !result_id && EnumOperations(PPOPT_GOODSRETURN, &iter_op_id, &op_rec) > 0;) {
					if(op_rec.LinkOpID == sale_op_id) {
						result_id = op_rec.ID;
					}
				}
			}
			if(result_id == 0)
				Cfg.ReturnOpID = -1; // Индицирует факт того, что вид операции возврата получить не удается.
			else
				Cfg.ReturnOpID = result_id;
		}
	}
	return result_id;
}

/*static*/PPID PrcssrMarketplaceInterchange::GetMarketplaceAccSheetID()
{
	PPID   acs_id = 0;
	Reference * p_ref = PPRef;
	PPAccSheet2 acs_rec;
	for(SEnum en = p_ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, PPOBJ_PERSON); !acs_id && en.Next(&acs_rec) > 0;) {
		if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_MARKETPLACE) {
			acs_id = acs_rec.ID;
		}
	}
	return acs_id;
}

/*static*/PPID PrcssrMarketplaceInterchange::Helper_GetMarketplaceWarehousesAccSheetID(bool createIfNExists, int use_ta)
{
	PPID   acs_id = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	PPObjLocation loc_obj;
	PPObjAccSheet acs_obj;
	PPLocationConfig loc_cfg;
	PPAccSheet2 acs_rec;
	PPObjLocation::ReadConfig(&loc_cfg);
	const PPID regular_loc_acs_id = LConfig.LocAccSheetID;
	const char * p_symb = "MP.WAREHOUSES";
	SString _name;
	PPLoadString("marketplacewarehousegroup", _name);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!loc_cfg.MarketplaceWarehoustFolderID) {
			if(createIfNExists) {
				PPID   loc_folder_id = 0;
				PPLocationPacket loc_pack;
				STRNSCPY(loc_pack.Name, _name);
				STRNSCPY(loc_pack.Code, p_symb);
				loc_pack.Type = LOCTYP_WAREHOUSEGROUP;
				THROW(loc_obj.PutPacket(&loc_folder_id, &loc_pack, 0));
				if(loc_folder_id) { // @paranoic
					loc_cfg.MarketplaceWarehoustFolderID = loc_folder_id;
					THROW(PPObjLocation::WriteConfig(&loc_cfg, 0));
				}
			}
		}
		if(loc_cfg.MarketplaceWarehoustFolderID) {
			for(SEnum en = p_ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, PPOBJ_LOCATION); !acs_id && en.Next(&acs_rec) > 0;) {
				if(acs_rec.ID != regular_loc_acs_id && acs_rec.Assoc == PPOBJ_LOCATION) {
					if(acs_rec.ObjGroup == loc_cfg.MarketplaceWarehoustFolderID) {
						acs_id = acs_rec.ID;
					}
					else {
						;
					}
				}
			}
			if(!acs_id) {
				if(createIfNExists) {
					MEMSZERO(acs_rec);
					STRNSCPY(acs_rec.Name, _name);
					STRNSCPY(acs_rec.Symb, p_symb);
					acs_rec.Assoc = PPOBJ_LOCATION;
					acs_rec.ObjGroup = loc_cfg.MarketplaceWarehoustFolderID;
					acs_rec.Flags = 0;
					THROW(p_ref->AddItem(PPOBJ_ACCSHEET, &acs_id, &acs_rec, 0));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		acs_id = 0;
	ENDCATCH
	return acs_id;
}

PPID PrcssrMarketplaceInterchange::GetMarketplaceWarehousesAccSheetID()
{
	return Helper_GetMarketplaceWarehousesAccSheetID(false/*createIfNExists*/, 0/*use_ta*/);
}

PPID PrcssrMarketplaceInterchange::Helper_GetMarketplaceOpsAccSheetID(bool createIfNExists, bool createArticles, int use_ta)
{
//PPTXT_ACCSHEET_MP_NAME               "Операции на маркетплейсах"
//PPTXT_ACCSHEET_MP_AR_NAMES           "1,Комиссионное вознаграждение;2,Стоимость эквайринга;3,Стоимость хранения;4,Стоимость приемки на склад хранения"
	const char * p_symb = "MARKETPLACE-OPS";
	PPID   acs_id = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	PPObjAccSheet acs_obj;
	PPAccSheet2 acs_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(acs_obj.SearchBySymb(p_symb, &acs_id, &acs_rec) > 0) {
			;
		}
		else if(createIfNExists) {
			MEMSZERO(acs_rec);
			PPLoadText(PPTXT_ACCSHEET_MP_NAME, temp_buf);
			STRNSCPY(acs_rec.Name, temp_buf);
			STRNSCPY(acs_rec.Symb, p_symb);
			acs_rec.Assoc = 0;
			acs_rec.ObjGroup = 0;
			acs_rec.Flags = 0;
			THROW(p_ref->AddItem(PPOBJ_ACCSHEET, &acs_id, &acs_rec, 0));		
		}
		if(acs_id && createArticles) {
			SString id_buf;
			SString nm_buf;
			PPLoadText(PPTXT_ACCSHEET_MP_AR_NAMES, temp_buf);
			StringSet ss(';', temp_buf);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				if(temp_buf.Divide(',', id_buf, nm_buf) > 0) {
					long ar_n = id_buf.ToLong();
					if(ar_n > 0 && nm_buf.NotEmptyS()) {
						ArticleTbl::Rec ar_rec;
						const int snr = ArObj.P_Tbl->SearchNum(acs_id, ar_n, &ar_rec);
						if(snr < 0) {
							PPID new_id = 0;
							ar_rec.Clear();
							ar_rec.Article = ar_n;
							ar_rec.ObjID = ar_n;
							ar_rec.AccSheetID = acs_id;
							STRNSCPY(ar_rec.Name, nm_buf);
							THROW(ArObj.P_Tbl->Add(&new_id, &ar_rec, 0));
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		acs_id = 0;
	ENDCATCH
	return acs_id;
}

PPID PrcssrMarketplaceInterchange::Helper_GetMarketplaceOpsAccount(bool createIfNExists, int use_ta)
{
	PPID   acc_id = 0;
	SString temp_buf;
	PPObjAccount acc_obj;
	PPAccount acc_rec;
	PPID acs_id = 0;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	const PPID gua_id = GetGuaPack().Rec.ID;
	THROW(tag_obj.Fetch(PPTAG_GUA_ACCOUNT, &tag_rec) > 0); // @todo @err (создание зарезервированных объектов)
	if(gua_id) {
		PPTransaction tra(use_ta);
		THROW(tra);
		acs_id = Helper_GetMarketplaceOpsAccSheetID(createIfNExists, true/*createArticles*/, 0);
		THROW(acs_id);
		{
			const ObjTagItem * p_tag_item = GetGuaPack().TagL.GetItem(PPTAG_GUA_ACCOUNT);
			int    intval = 0;
			if(p_tag_item && p_tag_item->GetInt(&intval) && acc_obj.Search(intval, &acc_rec) > 0) {
				acc_id = acc_rec.ID;
			}
			else if(createIfNExists) {
				PPAccountPacket acc_pack;
				(temp_buf = "Marketplace account").CatDiv('-', 1).Cat(GetGuaPack().Rec.Name);
				STRNSCPY(acc_pack.Rec.Name, temp_buf);
				acc_pack.Rec.Type = ACY_REGISTER;
				acc_pack.Rec.Kind = ACT_ACTIVE;
				acc_pack.Rec.Flags |= ACF_SYSNUMBER;
				acc_pack.Rec.AccSheetID = acs_id;
				THROW(acc_obj.GenerateNumber(&acc_pack.Rec));
				assert(acc_id == 0);
				THROW(acc_obj.PutPacket(&acc_id, &acc_pack, 0));
				{
					assert(acc_id);
					ObjTagItem tag_item;
					tag_item.Init(PPTAG_GUA_ACCOUNT);
					tag_item.SetInt(PPTAG_GUA_ACCOUNT, acc_id);
					THROW(PPRef->Ot.PutTag(PPOBJ_GLOBALUSERACC, GetGuaPack().Rec.ID, &tag_item, 0));
					THROW(ReloadGuaPack()); // Повторно извлекаем пакет глобальной учетной записи ибо там теперь новый тег
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		acc_id = 0;
	ENDCATCH
	return acc_id;
}

PPID PrcssrMarketplaceInterchange::GetMarketplaceOpsAccSheetID()
{
	return Helper_GetMarketplaceOpsAccSheetID(false, false, 0);
}

int PrcssrMarketplaceInterchange::ReloadGuaPack()
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	THROW(gua_obj.GetPacket(GuaPack.Rec.ID, &GuaPack) > 0);
	CATCHZOK
	return ok;
}

int PrcssrMarketplaceInterchange::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	MarketplaceInterchangeFilt temp_filt;
	State &= ~stInited;
	ZDELETE(P_Ifc);
	THROW(temp_filt.IsA(pBaseFilt));
	temp_filt = *static_cast<const MarketplaceInterchangeFilt *>(pBaseFilt);
	THROW(temp_filt.GuaID); // @todo @err
	THROW(gua_obj.GetPacket(temp_filt.GuaID, &GuaPack) > 0);
	{
		int rcr = ReadConfig(&Cfg);
		THROW(rcr);
		if(rcr < 0) {
			Cfg.OrderOpID = GetOrderOpID();
			Cfg.SalesOpID = GetSaleOpID();
		}
	}
	if(GuaPack.Rec.ServiceIdent == PPGLS_WILDBERRIES) {
		PPID   mp_psn_id = 0;
		P_Ifc = new PPMarketplaceInterface_Wildberries(*this);
		THROW_SL(P_Ifc);
		const int gmppr = P_Ifc->GetMarketplacePerson(&mp_psn_id, 1);
		THROW(P_Ifc->Init(temp_filt.GuaID));
	}
	else {
		CALLEXCEPT(); // @todo @err
	}
	State |= stInited;
	CATCHZOK
	return ok;
}

int PrcssrMarketplaceInterchange::EvaluateAverageRest(PPID goodsID, const DateRange & rPeriod, double * pResult)
{
	int   ok = 1;
	double result = 0.0;
	PPObjBill * p_bobj = BillObj;
	Transfer * p_trfr = p_bobj->trfr;
	DateRange period(rPeriod);
	PPID   folder_loc_id = 0;
	PPIDArray loc_list;
	if(P_Ifc) {
		P_Ifc->CreateWarehouseFolder(&folder_loc_id, 1);
	}
	if(folder_loc_id) {
		PPObjLocation loc_obj;
		PPIDArray init_loc_list;
		init_loc_list.add(folder_loc_id);
		loc_obj.ResolveWarehouseList(&init_loc_list, loc_list);
	}
	if(loc_list.getCount()) {
		if(!checkdate(period.low)) {
			for(uint locidx = 0; locidx < loc_list.getCount(); locidx++) {
				const PPID loc_id = loc_list.get(locidx);
				ReceiptTbl::Rec lot_rec;
				if(p_trfr->Rcpt.GetFirstLot(goodsID, loc_id, &lot_rec) > 0) {
					if(!checkdate(period.low) || period.low > lot_rec.Dt)
						period.low = lot_rec.Dt;
				}
			}
		}
		if(!checkdate(period.upp))
			period.upp = getcurdate_();
		if(checkdate(period.low) && checkdate(period.upp) && period.low <= period.upp) {
			LotArray lot_rec_list;
			for(uint locidx = 0; locidx < loc_list.getCount(); locidx++) {
				const PPID loc_id = loc_list.get(locidx);
				p_trfr->Rcpt.GetList(goodsID, loc_id, 0, period.upp, 0, &lot_rec_list);
			}
			//
			// В общем случае среднее значение не аддитивно, однако функция Transfer::EvaluateAverageRestByLot
			// гарантированно считает средний остаток за один и тот же набор дней и по этому в этом частном случае
			// мы можем складывать средние остатки по каждому лоту дабы получить общий средний остаток по товару.
			//
			for(uint i = 0; i < lot_rec_list.getCount(); i++) {
				const ReceiptTbl::Rec & r_lot_rec = lot_rec_list.at(i);
				double pv = 0.0;
				if(p_trfr->EvaluateAverageRestByLot(r_lot_rec.ID, period, &pv) > 0) {
					ok = 1;
				}
				result += pv;
			}
		}
	}
	ASSIGN_PTR(pResult, result);
	return ok;
}

int PrcssrMarketplaceInterchange::Run()
{
	int    ok = -1;
	SString temp_buf;
	THROW(State & stInited); // @todo @err
	THROW(P_Ifc); // @todo @err
	PPWait(1);
	{
		PPMarketplaceInterface_Wildberries * p_ifc_wb = static_cast<PPMarketplaceInterface_Wildberries *>(P_Ifc);
		Logger.LogString(PPTXT_MP_PROCESSING_RECEIPTS, 0);
		p_ifc_wb->ImportReceipts();
		Logger.LogString(PPTXT_MP_PROCESSING_ORDERS, 0);
		p_ifc_wb->ImportOrders();
		Logger.LogString(PPTXT_MP_PROCESSING_SALES, 0);
		p_ifc_wb->ImportSales();
		Logger.LogString(PPTXT_MP_PROCESSING_FINTRANSACTIONS, 0);
		p_ifc_wb->ImportFinancialTransactions();
		Logger.LogString(PPTXT_MP_PROCESSING_STOCK, 0);
		p_ifc_wb->ImportStocks();
	}
	CATCH
		ok = 0;
		Logger.LogLastError();
	ENDCATCH
	Logger.Save(PPGetFilePathS(PPPATH_LOG, PPFILNAM_MRKTPLC_LOG, temp_buf), 0);
	PPWait(0);
	return ok;
}

int DoMarketplaceInterchange()
{
	MarketplaceInterchangeFilt * pFilt = 0; // @stub
	int    ok = -1;
	PrcssrMarketplaceInterchange prcssr;
	if(pFilt) {
		if(prcssr.Init(pFilt) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	else {
		MarketplaceInterchangeFilt param;
		prcssr.InitParam(&param);
		if(prcssr.EditParam(&param) > 0)
			if(prcssr.Init(&param) && prcssr.Run())
				ok = 1;
			else
				ok = PPErrorZ();
	}
	return ok;
}
//
//
//
/*
Показатели продаж маркетплейса, необходимые для анализа:

% брака кабинета
% брака, шт
% Выкупа
% выполнения плана по МАРЖЕ
% компенсации брака от себеса
??? Индекс локализации, %
??? КТР (коэффициент)
xxx Контент всего, руб
xxx Контент, руб
xxx Прочие расходы, %
xxx Прочие расходы, руб
xxx Расход всего, руб
xxx Расходы на закуп товара, руб
xxx Реклама и маркетинг, %
xxx Реклама и маркетинг, руб
xxx Специалисты, руб
xxx Фотосессии, руб
Брак компенсация, руб
Внешний трафик, в т.ч. блогеры, руб
Внешний трафик, руб
Возвраты (не выкуп), шт
Всего к перечислению, руб - проверка
Всего начислено, в т.ч. комиссия, руб
Выручка к перечислению ФАКТ, руб
Выручка ПЛАН без комиссии, руб
Выручка, руб
Доплата за разгрузку, руб
Кол-во брака, шт
Комиссия проверка (ручная)
Комиссия, %
Комиссия, руб
Компенсация брака, руб
Маржа за минусом всех расх., %
Маржа за минусом всех расх., руб
Маржа, руб
Маржа, руб/ед
Маржа,%
Переменные расходы, руб
Платная приемка, руб
Премиум-пакет, руб
Продажи, шт
Продвижение, руб
Прочие + хранение + реклама, %
Прочие + хранение + реклама, руб
Реклама WB, руб
Самовыкупы, руб
Самовыкупы, шт
Себестоимость брака, руб
Себестоимость закупа товара, руб
СПП, %
СПП, руб
Ср.маржа на 1 ед.товара, руб
Ср.себес на 1 ед. товара, руб
Ср.чек на 1 ед. товара, руб
Транзит, руб
Утилизация ед, шт
Утилизация, руб
Хранение WB, руб
Хранение ФФ, руб
Хранение, %
Хранение, руб
Штрафы, %
Штрафы, руб
Логистика, руб
Логистика, руб в т.ч.:
	Повышенная логистика, руб
	Обратная логистика, руб
	Обратная логистика, %
	Ср.логистика на 1 ед. товара, руб
*/

int TestMarketplace()
{
	class TestMarketplaceDialog : public TDialog {
	public:
		TestMarketplaceDialog() : TDialog(DLG_TESTMRKTPLC)
		{
		}
	};
	int    ok = -1;
	bool   do_test = false;
	PPID   gua_id = 0;
	SString temp_buf;
	SString param_buf;
	TestMarketplaceDialog * dlg = new TestMarketplaceDialog();
	if(CheckDialogPtr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_TESTMRKTPLC_GUA, PPOBJ_GLOBALUSERACC, gua_id, 0);
		dlg->setCtrlString(CTL_TESTMRKTPLC_TEXT, param_buf);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_TESTMRKTPLC_GUA, &gua_id);
			dlg->getCtrlString(CTL_TESTMRKTPLC_TEXT, param_buf);
			do_test = true;
		}
	}
	if(do_test) {
		{
			PPMarketplaceInterface_Wildberries::CategoryPool cat_pool;
			PPMarketplaceInterface_Wildberries::PublicWarePool goods_pool;
			MarketplaceGoodsSelectionFilt filt;
			PPMarketplaceInterface_Wildberries::LoadPublicGoodsCategoryList(cat_pool, true);
			if(PPMarketplaceInterface_Wildberries::EditPublicGoodsSelectionFilt(cat_pool, filt) > 0) {
				if(filt.CatID) {
					PPMarketplaceInterface_Wildberries::LoadPublicGoodsList(cat_pool, filt, goods_pool);
				}
			}
			return 1; // !
		}
		PPID   mp_psn_id = 0;
		//PPLogger logger;
		PrcssrMarketplaceInterchange prc;
		MarketplaceInterchangeFilt filt;
		filt.GuaID = gua_id;
		if(prc.Init(&filt)) {
			PPMarketplaceInterface * p_ifc = prc.GetIfc();
			if(p_ifc) {
				PPMarketplaceInterface_Wildberries * p_ifc_wb = static_cast<PPMarketplaceInterface_Wildberries *>(p_ifc);
				const int gmppr = p_ifc_wb->GetMarketplacePerson(&mp_psn_id, 1);
				TSCollection <PPMarketplaceInterface_Wildberries::Warehouse> wh_list2;
				TSCollection <PPMarketplaceInterface_Wildberries::Stock> stock_list;
				//TSCollection <PPMarketplaceInterface_Wildberries::Sale> sale_list;
				//TSCollection <PPMarketplaceInterface_Wildberries::Sale> order_list;
				//TSCollection <PPMarketplaceInterface_Wildberries::Income> income_list;
				TSCollection <PPMarketplaceInterface_Wildberries::SalesRepDbpEntry> sales_rep_dbp_list;
				TSCollection <PPMarketplaceInterface_Wildberries::Promotion> promo_list;
				TSCollection <PPMarketplaceInterface_Wildberries::Campaign> campaign_list;
				TSCollection <PPMarketplaceInterface_Wildberries::ProductCategory> product_category_list;
				TSCollection <PPMarketplaceInterface_Wildberries::CampaignStatistics_Adv> campaign_stat_list;

				int r = 0;
				{
					r = p_ifc_wb->RequestProductCategoryList(product_category_list);
					r = p_ifc_wb->RequestProductSubjectList(product_category_list);
					if(product_category_list.getCount()) {
						PPGetFilePath(PPPATH_OUT, "wb_category_list.txt", temp_buf);
						SFile f_out(temp_buf, SFile::mWrite);
						if(f_out.IsValid()) {
							for(uint i = 0; i < product_category_list.getCount(); i++) {
								const PPMarketplaceInterface_Wildberries::ProductCategory * p_cat = product_category_list.at(i);
								if(p_cat) {
									temp_buf.Z().Cat(p_cat->ID).Tab().Cat(p_cat->Name).CR();
									f_out.WriteLine(temp_buf);
									if(p_cat->SubjectList.getCount()) {
										for(uint subjidx = 0; subjidx < p_cat->SubjectList.getCount(); subjidx++) {
											const PPMarketplaceInterface_Wildberries::ProductCategory * p_subj = p_cat->SubjectList.at(subjidx);
											if(p_subj) {
												temp_buf.Z().Tab().Cat(p_subj->ID).Tab().Cat(p_subj->Name).CR();
												f_out.WriteLine(temp_buf);
											}
										}
									}
								}
							}
						}
					}
				}
				r = p_ifc_wb->RequestGoodsPrices();
				r = p_ifc_wb->RequestGoodsSizes(245313051);
				r = p_ifc_wb->RequestWareList();
				r = p_ifc_wb->RequestPromoCampaignList(campaign_list); // @v12.2.3
				r = p_ifc_wb->RequestPromoCampaignListDetail(campaign_list); // @v12.2.4
				r = p_ifc_wb->RequestPromoCampaignListStatistics(0/*pCommonReqPeriod*/, campaign_list, campaign_stat_list);
				r = p_ifc_wb->RequestPromotionList(promo_list); // @v12.2.2
				r = p_ifc_wb->RequestPromotionDetail(promo_list); // @v12.2.2
				{
					TSCollection <PPMarketplaceInterface_Wildberries::WareOnPromotion> ware_on_promo_list;
					r = p_ifc_wb->RequestPromotionWareList(/*actionId*/805, true, ware_on_promo_list);
				}
				r = p_ifc_wb->RequestReturns();
				r = p_ifc_wb->RequestWarehouseList2(wh_list2, 0/*FaultStatusResult*/);
				r = p_ifc_wb->RequestDocumentsList();
				r = p_ifc_wb->RequestBalance();
				//
				DateRange period;
				period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
				r = p_ifc_wb->ImportReceipts();
				r = p_ifc_wb->ImportOrders();
				r = p_ifc_wb->ImportSales();
				//r = p_ifc_wb->RequestWarehouseList(wh_list);
				r = p_ifc_wb->RequestAcceptanceReport(period);
				r = p_ifc_wb->RequestSupplies();
				//r = p_ifc_wb->RequestIncomes(income_list);
				r = p_ifc_wb->RequestCommission();
				r = p_ifc_wb->RequestStocks(stock_list, 0);
				//r = p_ifc_wb->RequestOrders(order_list);
				//r = p_ifc_wb->RequestSales(sale_list);
				{
					//period.SetPredefined(PREDEFPRD_LASTMONTH, ZERODATE);
					//period.low.encode(1, 5, 2024);
					//period.upp = getcurdate_();
					//r = p_ifc_wb->RequestSalesReportDetailedByPeriod(period, sales_rep_dbp_list);
					r = p_ifc_wb->ImportFinancialTransactions();
				}
			}
		}
	}
	delete dlg;
	return ok;
}