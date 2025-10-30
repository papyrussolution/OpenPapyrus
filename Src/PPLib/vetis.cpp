// VETIS.CPP
// Copyright (c) A.Sobolev 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Модуль для взаимодействия с системой Меркурий (интерфейс ВЕТИС)
//
#include <pp.h>
#pragma hdrstop

#define FOREACHXMLCHILD(_parent_, _node_) for(const xmlNode * _node_ = (_parent_) ? (_parent_)->children : 0; _node_; _node_ = _node_->next)

// ModifyActivityLocationsOperation
// ResolveDiscrepancyOperation
// RegisterProductionOperation
// @todo MergeStockEntriesOperation

static const char * P_VetisGuid_Country_Ru   = "74a3cbb1-56fa-94f3-ab3f-e8db4940d96b"; // Россия
static const char * P_VetisGuid_Region_Ru_10 = "248d8071-06e1-425e-a1cf-d1ff4c4a14a8"; // Карелия
static const char * P_VetisGuid_Unit_Item    = "6606dc4a-d415-4e19-8acd-01c6d4a89e18"; // штука
static const char * P_VetisGuid_Unit_Kg      = "21ed96c9-337b-4a27-8761-c6e6ad3c9f5b"; // кг
static const char * P_ApiVtrf = "api.vetrf.ru";

struct VetisBusinessEntity;

static const SString & FASTCALL _xmlnst_vd(const char * pF) { return SXml::nst("vd", pF); }
static const SString & FASTCALL _xmlnst_dt(const char * pF) { return SXml::nst("dt", pF); }
static const SString & FASTCALL _xmlnst_bs_guid() { return SXml::nst("bs", "guid"); }
static const SString & FASTCALL _xmlnst_bs_uuid() { return SXml::nst("bs", "uuid"); }

struct VetisErrorEntry {
	VetisErrorEntry & Z()
	{
		Item.Z();
		Code.Z();
		Qualifier.Z();
		return *this;
	}
	bool   IsEmpty() const { return (Item.IsEmpty() && Code.IsEmpty() && Qualifier.IsEmpty()); }
	SString Item;
	SString Code;
	SString Qualifier;
};

struct VetisFault {
	VetisFault() : Type(tUndef)
	{
	}
	bool   IsEmpty() const { return (Message.IsEmpty() && !ErrList.getCount()); }
	VetisFault & FASTCALL operator = (const VetisFault & rS)
	{
		Type = rS.Type;
		Message = rS.Message;
		TSCollection_Copy(ErrList, rS.ErrList);
		return *this;
	}
	enum {
		tUndef = 0,
		tAccessDenied = 1,
		tEntityNotFound,
		tIncorrectRequest,
		tInternalService,
		tUnknownServiceId,
		tUnsupportedApplicationDataType
	};
	int    Type;
	SString Message;
	TSCollection <VetisErrorEntry> ErrList;
};

struct VetisEntityList {
	VetisEntityList() : Count(0), Total(0), Offset(0), Flags(0)
	{
	}
	VetisEntityList & Z()
	{
		Count = 0;
		Total = 0;
		Offset = 0;
		Flags = 0;
		return *this;
	}
	enum {
		fHasMore = 0x0001
	};
	int    Count;
	int64  Total;
	int    Offset;
	long   Flags;
};

struct VetisGenericEntity {
	VetisGenericEntity()
	{
	}
	VetisGenericEntity & Z()
	{
		Uuid.Z();
		return *this;
	}
	S_GUID Uuid;
};

struct VetisEnterpriseActivity : public VetisGenericEntity {
	VetisEnterpriseActivity & Z()
	{
		VetisGenericEntity::Z();
		Name.Z();
		return *this;
	}
	SString Name;
};

struct VetisGenericVersioningEntity : public VetisGenericEntity {
	VetisGenericVersioningEntity() : VetisGenericEntity(), Flags(0), Status(0), CreateDate(ZERODATETIME), UpdateDate(ZERODATETIME)
	{
	}
	VetisGenericVersioningEntity & Z()
	{
		VetisGenericEntity::Z();
		Guid.Z();
		Flags = 0;
		Status = 0;
		CreateDate.Z();
		UpdateDate.Z();
		Previous.Z();
		Next.Z();
		return *this;
	}
	S_GUID Guid;
	enum {
		fActive = 0x0001,
		fLast   = 0x0002
	};
	enum {
		verstatusCreated          = 100, // CREATED - Запись создана.
		verstatusCreatedWhenQuenchVetCert = 101, // CREATED_WHEN_QUENCH_VETCERTIFICATE - Запись создана путем гашения ВС (импорт).
		verstatusCreatedWhenQuenchVetDoc  = 102, // CREATED_WHEN_QUENCH_VETDOCUMENT - Запись создана путем гашения ВСД.
		verstatusCreatedByOperation       = 103, // CREATED_BY_OPERATION - Запись создана в результате производственной операции.
		verstatusCreatedWhenMerge = 110, // CREATED_WHEN_MERGE - Запись создана в результате объединения двух или более других.
		verstatusCreatedWhenSplit = 120, // CREATED_WHEN_SPLIT - Запись создана в результате разделения другой.
		verstatusUpdated          = 200, // UPDATED - В запись были внесены изменения.
		verstatusWithdrawn        = 201, // WITHDRAWN - Запись журнала аннулирована.
		verstatusUpdatedWhetWritingOff    = 202, // UPDATED_WHEN_WRITINGOFF - Запись продукции изменена путём списания. Необязательно, чтобы продукция была списана полностью, может быть списана и часть объёма.
		verstatusUpdatedWhenAttach        = 230, // UPDATED_WHEN_ATTACH - Запись была обновлена в результате присоединения другой.
		verstatusUpdatedWhenAttachAuto    = 231, // UPDATED_WHEN_ATTACH_AUTOMATIC - Запись была обновлена в результате присоединения другой.
		verstatusUpdatedWhenFork  = 240, // UPDATED_WHEN_FORK - Запись была обновлена в результате отделения от неё другой.
		verstatusRestoredAfterDelete      = 250, // RESTORED_AFTER_DELETE - Запись была восстановлена после удаления.
		verstatusMoved            = 300, // MOVED - Запись была перемещена в другую группу (для иерархических справочников).
		verstatusDeleted          = 400, // DELETED - Запись была удалена.
		verstatusDeletedWhenMerge = 410, // DELETED_WHEN_MERGE - Запись была удалена в результате объединения.
		verstatusDeletedWhenSplit = 420, // DELETED_WHEN_SPLIT - Запись была удалена в результате разделения.
		verstatusDeletedWhenAttach        = 430, // DELETED_WHEN_ATTACH - Запись была удалена в результате присоединения.
	};
	long   Flags;
	int    Status; // verstatusXXX
	LDATETIME CreateDate;
	LDATETIME UpdateDate;
	S_GUID Previous;
	S_GUID Next;
};

struct VetisNamedGenericVersioningEntity : public VetisGenericVersioningEntity {
	VetisNamedGenericVersioningEntity & Z()
	{
		VetisGenericVersioningEntity::Z();
		Name.Z();
		return *this;
	}
	SString Name;
};

struct VetisCountry : public VetisNamedGenericVersioningEntity {
	SString FullName;
	SString EnglishName;
	SString Code;
	SString Code3;
};

struct VetisAddressObjectView : public VetisNamedGenericVersioningEntity {
	VetisAddressObjectView()
	{
	}
	VetisAddressObjectView & Z()
	{
		VetisNamedGenericVersioningEntity::Z();
		EnglishName.Z();
		View.Z();
		RegionCode.Z();
		Type.Z();
		CountryGUID.Z();
		return *this;
	}
	SString EnglishName;
	SString View;
	SString RegionCode;
	SString Type;
	S_GUID CountryGUID;
	enum {
		fHasStreets = 0x00010000
	};
};

struct VetisAddress {
	struct VetisDistrict : public VetisAddressObjectView {
		VetisDistrict() : VetisAddressObjectView()
		{
		}
		VetisDistrict & Z()
		{
			VetisAddressObjectView::Z();
			RegionGUID.Z();
			return *this;
		}
		S_GUID RegionGUID;
	};
	struct VetisFederalDistrict {
		VetisFederalDistrict & Z()
		{
			FullName.Z();
			ShortName.Z();
			Abbreviation.Z();
			return *this;
		}
		SString FullName;
		SString ShortName;
		SString Abbreviation;
	};
	struct VetisStreet : public VetisAddressObjectView {
		VetisStreet() : VetisAddressObjectView()
		{
		}
		VetisStreet & Z()
		{
			VetisAddressObjectView::Z();
			LocalityGUID.Z();
			return *this;
		}
		S_GUID LocalityGUID;
	};
	struct VetisLocality : public VetisAddressObjectView {
		VetisLocality() : VetisAddressObjectView()
		{
		}
		VetisLocality & Z()
		{
			VetisAddressObjectView::Z();
			RegionGUID.Z();
			DistrictGUID.Z();
			CityGUID.Z();
			return *this;
		}
		S_GUID RegionGUID;
		S_GUID DistrictGUID;
		S_GUID CityGUID;
	};
	VetisAddress & Z()
	{
		Country.Z();
		FederalDistrict.Z();
		Region.Z();
		District.Z();
		Locality.Z();
		SubLocality.Z();
		Street.Z();
		House.Z();
		Building.Z();
		Room.Z();
		PostIndex.Z();
		PostBox.Z();
		AdditionalInfo.Z();
		AddressView.Z();
		EnAddressView.Z();
		return *this;
	}
	VetisCountry Country;
	VetisFederalDistrict FederalDistrict;
	VetisAddressObjectView Region;
	VetisDistrict District;
	VetisLocality Locality;
	VetisLocality SubLocality;
	VetisStreet Street;
	SString House;
	SString Building;
	SString Room;
	SString PostIndex;
	SString PostBox;
	SString AdditionalInfo;
	SString AddressView;
	SString EnAddressView;
};

struct VetisLocation {
	VetisLocation()
	{
	}
	VetisLocation & Z()
	{
		Name.Z();
		Address.Z();
		return *this;
	}
	SString Name;
	VetisAddress Address;
};

struct VetisOrganization {
	SString ID;
	SString Name;
	VetisAddress Address;
};

enum VetisDocType {
	vetisdoctypTRANSPORT = 0,
	vetisdoctypPRODUCTIVE = 1,
	vetisdoctypRETURNABLE = 2,
	vetisdoctypINCOMING = 3,
	vetisdoctypOUTGOING = 4,
	vetisdoctypSTOCK = 8, // Специальный тип для отражения остатков (не соответствует ни одному из "родных" типов ВЕТИС)
};

struct VetisDocument : public VetisGenericEntity {
	VetisDocument() : VetisGenericEntity(), EntityID(0), IssueDate(ZERODATE), DocumentType(0)
	{
	}
	VetisDocument & FASTCALL operator = (const VetisDocument & rS)
	{
		VetisGenericEntity::operator = (rS);
		EntityID = rS.EntityID;
		Name = rS.Name;
		IssueSeries = rS.IssueSeries;
		IssueNumber = rS.IssueNumber;
		DocumentForm = rS.DocumentForm;
		IssueDate = rS.IssueDate;
		DocumentType = rS.DocumentType;
		Issuer = rS.Issuer;
		For_ = rS.For_;
		Qualifier = rS.Qualifier;
		return *this;
	}
	PPID   EntityID;   // ID документа в собственной базе данных
	SString Name;
	SString IssueSeries;
	SString IssueNumber;
	SString DocumentForm;
	LDATE  IssueDate;
	int    DocumentType;
	VetisOrganization Issuer;
	SString For_;
	SString Qualifier; // ?
};

struct VetisUserAuthority {
	VetisUserAuthority() : Granted(0)
	{
	}
	SString ID;
	SString Name;
	int    Granted; // bool
};

struct VetisUser {
	VetisUser();
	~VetisUser();
	VetisUser & FASTCALL operator = (const VetisUser & rS);

	SString Login;
	SString Fio;
	LDATE  BirthDate;
	VetisDocument Identity;
	SString Snils;
	SString Phone;
	SString Email;
	SString WorkEmail;
	//
	int    UnionUser;
	VetisOrganization * P_Organization;
	VetisBusinessEntity * P_BusinessEntity;
	//
	SString Post;
	enum {
		fEnabled    = 0x0001,
		fNonExpired = 0x0002,
		fNonLocked  = 0x0004
	};
	long   Flags;
	TSCollection <VetisUserAuthority> AuthorityList;
};

struct VetisEnterpriseOfficialRegistration {
	VetisEnterpriseOfficialRegistration();
	~VetisEnterpriseOfficialRegistration();
	VetisEnterpriseOfficialRegistration & FASTCALL operator = (const VetisEnterpriseOfficialRegistration & rS);
	SString ID; // GRNType
	VetisBusinessEntity * P_BusinessEntity;
	SString Kpp;
};

struct VetisEnterpriseActivityList : public VetisEntityList {
	VetisEnterpriseActivityList & FASTCALL operator = (const VetisEnterpriseActivityList & rS)
	{
		TSCollection_Copy(this->Activity, rS.Activity);
		return *this;
	}
	VetisEnterpriseActivityList & Z()
	{
		VetisEntityList::Z();
		Activity.freeAll();
		return *this;
	}
	TSCollection <VetisEnterpriseActivity> Activity;
};

enum VetisRegisterModificationType {
	vetisrmtCREATE = 1,     // Добавление новой записи.
	vetisrmtFIND_OR_CREATE, // Поиск существующей или добавление новой записи, если искомой записи не существует.
	vetisrmtUPDATE,         // Внесение изменений в существующую запись.
	vetisrmtDELETE,         // Удаление существующей записи.
	vetisrmtMERGE,          // Объединение двух или нескольких записей.
	vetisrmtATTACH,         // Присоединение одной или нескольких записе к другой.
	vetisrmtSPLIT,          // Разделение записи на две и более.
	vetisrmtFORK            // Отделение одной и более записей от существующей.
};

struct VetisEnterprise : public VetisNamedGenericVersioningEntity {
	VetisEnterprise();
	~VetisEnterprise();
	VetisEnterprise & FASTCALL operator = (const VetisEnterprise & rS);
	VetisEnterprise & Z();

	SString EnglishName;
	int    Type;          // EnterpriseType
	PPID   NativeLocID;   // -->Location.ID Идентификатор соответствующей локации в БД
	StringSet NumberList; // EnterpriseNumberList
	VetisAddress Address;
	VetisEnterpriseActivityList ActivityList;
	VetisBusinessEntity * P_Owner;
	TSCollection <VetisEnterpriseOfficialRegistration> OfficialRegistration;
};

struct VetisBusinessEntity_activityLocation {
	StringSet GlobalID; // Список GLN
	VetisEnterprise Enterprise;
};

struct VetisIncorporationForm : public VetisGenericEntity {
	VetisIncorporationForm() : VetisGenericEntity()
	{
	}
	VetisIncorporationForm & Z()
	{
		VetisGenericEntity::Z();
		Name.Z();
		ShortName.Z();
		Code.Z();
		return *this;
	}
	SString Name;
	SString ShortName;
	SString Code;
};

struct VetisBusinessEntity : public VetisNamedGenericVersioningEntity {
	VetisBusinessEntity() : VetisNamedGenericVersioningEntity(), EntityID(0), Type(0), NativePsnID(0)
	{
	}
	VetisBusinessEntity & Z()
	{
		VetisNamedGenericVersioningEntity::Z();
		EntityID = 0;
		Type = 0;
		NativePsnID = 0;
		FullName.Z();
		Fio.Z();
		Passport.Z();
		Inn.Z();
		Kpp.Z();
		Ogrn.Z();
		IncForm.Z();
		JuridicalAddress.Z();
		ActivityLocationList.freeAll();
		return *this;
	}
	VetisBusinessEntity & FASTCALL operator = (const VetisBusinessEntity & rS)
	{
		VetisNamedGenericVersioningEntity::operator = (rS);
		EntityID = rS.EntityID;
		Type = rS.Type;
		NativePsnID = rS.NativePsnID;
		FullName = rS.FullName;
		Fio = rS.Fio;
		Passport = rS.Passport;
		Inn = rS.Inn;
		Kpp = rS.Kpp;
		Ogrn = rS.Ogrn;
		IncForm = rS.IncForm;
		JuridicalAddress = rS.JuridicalAddress;
		TSCollection_Copy(ActivityLocationList, rS.ActivityLocationList);
		return *this;
	}
	PPID   EntityID;
	int    Type;     // BusinessEntityType
	PPID   NativePsnID; // -->Person.ID Идентификатор соответствующей персоналии в БД
	SString FullName;
	SString Fio;
	SString Passport;
	SString Inn;
	SString Kpp;
	SString Ogrn;
	VetisIncorporationForm IncForm;
	VetisAddress JuridicalAddress;
	TSCollection <VetisBusinessEntity_activityLocation> ActivityLocationList;
};

struct VetisListOptions {
	VetisListOptions() : Count(0), Offset(0)
	{
	}
	void   Set(uint o, uint c)
	{
		Offset = o;
		Count = c;
	}
	uint   Count;
	uint   Offset;
};

enum VetisTransportStorageType { // @persistent (vetis - идентификация по символу, в БД Papyrus - по значению)
	vtstUNDEF        = 0, // Не определенный
	vtstFROZEN       = 1, // Замороженный
	vtstCHILLED      = 2, // Охлажденный
	vtstCOOLED       = 3, // Охлаждаемый
	vtstVENTILATED   = 4, // Вентилируемый
};

enum VetisProductType {
	vptUndef = 0,
	vptMeat  = 1, // 1 Мясо и мясопродукты.
	vptFeedStuff     = 2, // 2 Корма и кормовые добавки.
	vptAnimal        = 3, // 3 Живые животные.
	vptMedicine      = 4, // 4 Лекарственные средства.
	vptFood  = 5, // 5 Пищевые продукты.
	vptNonFood       = 6, // 6 Непищевые продукты и другое.
	vptFish  = 7, // 7 Рыба и морепродукты.
	vptDontReqPermit = 8 // 8 Продукция, не требующая разрешения.
};

enum VetisProductMarkingClass {
	vpmcUNDEFINED = 0,
	vpmcBN,            // Номер производственной партии
	vpmcSSCC,          // SSCC-код
	vpmcEAN8,
	vpmcEAN13,
	vpmcEAN128,
	vpmcBUNDLE         // Маркировка вышестоящей групповой упаковки, например, паллеты. Может использоваться для поиска группы вет.сертификатов для партий, находящихся на данной паллете.
};

struct VetisProduct : public VetisNamedGenericVersioningEntity {
	VetisProduct() : VetisNamedGenericVersioningEntity(), ProductType(vptUndef)
	{
	}
	SString Code;
	SString EnglishName;
	int    ProductType; // ProductType vptXXX
};

struct VetisSubProduct : public VetisNamedGenericVersioningEntity {
	VetisSubProduct() : VetisNamedGenericVersioningEntity()
	{
	}
	SString Code;
	SString EnglishName;
	S_GUID ProductGuid;
};

struct VetisUnit : public VetisNamedGenericVersioningEntity {
	VetisUnit() : VetisNamedGenericVersioningEntity(), Factor(0.0)
	{
	}
	SString FullName;
	S_GUID CommonUnitGuid;
	double Factor;
};

struct VetisPackingType : public VetisNamedGenericVersioningEntity {
	VetisPackingType() : VetisNamedGenericVersioningEntity()
	{
	}
	VetisPackingType & FASTCALL operator = (const VetisPackingType & rS)
	{
		VetisNamedGenericVersioningEntity::operator = (rS);
		GlobalID = rS.GlobalID;
		return *this;
	}
	VetisPackingType & Z()
	{
		VetisNamedGenericVersioningEntity::Z();
		GlobalID.Z();
		return *this;
	}
	SString GlobalID; // PackingCodeType
};

struct VetisPackaging {
	VetisPackaging() : Quantity(0), Volume(0.0)
	{
	}
	VetisPackingType PackagingType;
	int    Quantity;
	double Volume;
	VetisUnit Unit;
};

struct VetisProductMarks {
	VetisProductMarks() : Cls(0)
	{
	}
	VetisProductMarks & Z()
	{
		Cls = 0;
		Item.Z();
		return *this;
	}
	SString Item;
	int   Cls; // ProductMarkingClass
};

struct VetisPackage {
	VetisPackage() : Level(0), Quantity(0)
	{
	}
	VetisPackage & FASTCALL operator = (const VetisPackage & rS)
	{
		Level = rS.Level;
		Quantity = rS.Quantity;
		PackingType = rS.PackingType;
		TSCollection_Copy(ProductMarks, rS.ProductMarks);
		return *this;
	}
	VetisPackage & Z()
	{
		Level = 0;
		Quantity = 0;
		PackingType.Z();
		ProductMarks.freeAll();
		return *this;
	}
	int    Level; // PackageLevelType
	int    Quantity;
	VetisPackingType PackingType;
	TSCollection <VetisProductMarks> ProductMarks;
};

struct VetisProductItem : public VetisNamedGenericVersioningEntity {
	VetisProductItem() : VetisNamedGenericVersioningEntity(), EntityID(0), NativeGoodsID(0), ProductType(vptUndef)
	{
	}
	VetisProductItem & FASTCALL operator = (const VetisProductItem & rS)
	{
		VetisNamedGenericVersioningEntity::operator = (rS);
		EntityID = rS.EntityID;
		NativeGoodsID = rS.NativeGoodsID;
		ProductType = rS.ProductType;
		GlobalID = rS.GlobalID;
		Code = rS.Code;
		Gost = rS.Gost;
		Product = rS.Product;
		SubProduct = rS.SubProduct;
		Producer = rS.Producer;
		TmOwner = rS.TmOwner;
		Packaging = rS.Packaging;
		TSCollection_Copy(Producing, rS.Producing);
		return *this;
	}
	enum {
		fCorrespondsToGost = 0x00010000,
		fIsPublic  = 0x00020000
	};
	PPID   EntityID;   // ID элемента в собственной базе данных (VetisEntityTbl)
	PPID   NativeGoodsID;
	int    ProductType; // ProductType vptXXX
	SString GlobalID;
	SString Code;
	SString Gost;
	VetisProduct Product;
	VetisSubProduct SubProduct;
	VetisBusinessEntity Producer;
	VetisBusinessEntity TmOwner;
	TSCollection <VetisEnterprise> Producing;
	VetisPackaging Packaging;
};

struct VetisGoodsDate {
	VetisGoodsDate()
	{
	}
	VetisGoodsDate & Z()
	{
		FirstDate.Z();
		SecondDate.Z();
		InformalDate.Z();
		return *this;
	}
	SUniTime FirstDate;
	SUniTime SecondDate;
	SString InformalDate;
};

struct VetisProducer : public VetisEnterprise {
	VetisProducer() : Role(0)
	{
	}
	enum {
		roleUNKNOWN  = 0,
		rolePRODUCER = 1
	};
	int    Role; // EnterpriseRole
};

struct VetisBatchOrigin {
	VetisBatchOrigin & FASTCALL operator = (const VetisBatchOrigin & rS)
	{
		ProductItem = rS.ProductItem;
		Country = rS.Country;
		TSCollection_Copy(Producer, rS.Producer);
		return *this;
	}
	VetisBatchOrigin & Z()
	{
		ProductItem.Z();
		Country.Z();
		Producer.freeAll();
		return *this;
	}
	VetisProductItem ProductItem;
	VetisCountry Country;
	TSCollection <VetisProducer> Producer;
};

struct VetisBatch {
	VetisBatch() : NativeGoodsID(0), ProductType(vptUndef), Volume(0.0), AckVolume(0.0), PackingAmount(0), Flags(0), P_Owner(0)
	{
	}
	~VetisBatch()
	{
		delete P_Owner;
	}
	VetisBatch & FASTCALL operator = (const VetisBatch & rS)
	{
		NativeGoodsID = rS.NativeGoodsID;
		ProductType = rS.ProductType;
		Product = rS.Product;
		SubProduct = rS.SubProduct;
		ProductItem = rS.ProductItem;
		Volume = rS.Volume;
		AckVolume = rS.AckVolume;
		PackingAmount = rS.PackingAmount;
		Unit = rS.Unit;
		DateOfProduction = rS.DateOfProduction;
		ExpiryDate = rS.ExpiryDate;
		TSCollection_Copy(ProductMarkingList, rS.ProductMarkingList);
		Flags = rS.Flags;
		BatchIdList = rS.BatchIdList;
		Origin = rS.Origin;
		TSCollection_Copy(PackageList, rS.PackageList);
		TSCollection_Copy(PackingList, rS.PackingList);
		ZDELETE(P_Owner);
		if(rS.P_Owner) {
			P_Owner = new VetisBusinessEntity;
			*P_Owner = *rS.P_Owner;
		}
		return *this;
	}
	VetisBatch & Z()
	{
		NativeGoodsID = 0;
		ProductType = 0;
		Product.Z();
		SubProduct.Z();
		ProductItem.Z();
		Volume = 0.0;
		AckVolume = 0.0;
		PackingAmount = 0;
		Unit.Z();
		DateOfProduction.Z();
		ExpiryDate.Z();
		ProductMarkingList.freeAll();
		Flags = 0;
		BatchIdList.Z();
		Origin.Z();
		PackageList.freeAll();
		PackingList.freeAll();
		ZDELETE(P_Owner);
		return *this;
	}
	PPID   NativeGoodsID; // Ид товара, сопоставленного с товаром, обозначенным в этой структуре (далеко не всегда возможно сделать автоматом)
	int    ProductType; // ProductType vptXXX
	VetisProduct Product;
	VetisSubProduct SubProduct;
	VetisProductItem ProductItem;
	double Volume;
	double AckVolume; // Количество принятое при поступлении товара.
	long   PackingAmount;
	VetisUnit Unit;
	VetisGoodsDate DateOfProduction;
	VetisGoodsDate ExpiryDate;
	// StringSet ProductMarkingList; // Список маркировочных кодов продукции (при разборе входящих пакетов
		// вносим в этот список только коды, удовлетворяющие стандартам EAN13, EAN8, UPCA, UPCE).
	TSCollection <VetisProductMarks> ProductMarkingList;
	enum {
		fPerishable     = 0x0001,
		fLowGradeCargo  = 0x0002,
		// Следующие 2 флага уточняют условия принятия входящей партии. Если ни один из флагов не
		// установлен, то партия считается безусловно приятной.
		fPartlyAccepted = 0x0004, // Партия частично принята
		fRejected       = 0x0008  // Партия полностью отклонена
	};
	long   Flags;
	StringSet BatchIdList;
	VetisBatchOrigin Origin;
	TSCollection <VetisPackage> PackageList;
	TSCollection <VetisNamedGenericVersioningEntity> PackingList;
	VetisBusinessEntity * P_Owner;
};

struct VetisBusinessMember {
	VetisBusinessMember & FASTCALL operator = (const VetisBusinessMember & rS)
	{
		BusinessEntity = rS.BusinessEntity;
		Enterprise = rS.Enterprise;
		GlobalID = rS.GlobalID;
		return *this;
	}
	VetisBusinessMember & Z()
	{
		BusinessEntity.Z();
		Enterprise.Z();
		GlobalID.Z();
		return *this;
	}
	VetisBusinessEntity BusinessEntity;
	VetisEnterprise Enterprise;
	SString GlobalID;
};

struct VetisCertifiedBatch : public VetisBatch {
	VetisBusinessMember Producer;
};

struct VetisTransportNumber {
	VetisTransportNumber & Z()
	{
		ContainerNumber.Z();
		WagonNumber.Z();
		VehicleNumber.Z();
		TrailerNumber.Z();
		ShipName.Z();
		FlightNumber.Z();
		return *this;
	}
	bool   IsEmpty() const
	{
		return (ContainerNumber.IsEmpty() && WagonNumber.IsEmpty() && VehicleNumber.IsEmpty() &&
			TrailerNumber.IsEmpty() && ShipName.IsEmpty() && FlightNumber.IsEmpty());
	}
	SString ContainerNumber;
	SString WagonNumber;
	SString VehicleNumber;
	SString TrailerNumber;
	SString ShipName;
	SString FlightNumber;
};

struct VetisTransportInfo {
	enum { // @persistent
		ttCar     = 1,
		ttRailWay = 2,
		ttAvia    = 3,
		ttShip    = 4
	};
	VetisTransportInfo() : TransportType(0)
	{
	}
	VetisTransportInfo & Z()
	{
		TransportType = 0;
		TransportNumber.Z();
		return *this;
	}
	int    TransportType; // TransportType
	VetisTransportNumber TransportNumber;
};

class VetisShipmentRoutePoint : public VetisGenericEntity {
	VetisShipmentRoutePoint(const VetisShipmentRoutePoint & rS) {} // @disable-copy-ctr
public:
	VetisShipmentRoutePoint() : UnionShipmentRoutePoint(0), Flags(0), P_Location(0), P_Enterprise(0), P_NextTransport(0)
	{
	}
	~VetisShipmentRoutePoint()
	{
		delete P_NextTransport;
		delete P_Location;
		delete P_Enterprise;
	}
	VetisShipmentRoutePoint & FASTCALL operator = (const VetisShipmentRoutePoint & rS)
	{
		VetisGenericEntity::operator = (static_cast<const VetisGenericEntity &>(rS));
		SqnId = rS.SqnId;
		UnionShipmentRoutePoint = rS.UnionShipmentRoutePoint;
		Flags = rS.Flags;
		if(rS.P_Location && SETIFZ(P_Location, new VetisLocation)) {
			*P_Location = *rS.P_Location;
		}
		else {
			ZDELETE(P_Location);
		}
		if(rS.P_Enterprise && SETIFZ(P_Enterprise, new VetisEnterprise)) {
			*P_Enterprise = *rS.P_Enterprise;
		}
		else {
			ZDELETE(P_Enterprise);
		}
		if(rS.P_NextTransport && SETIFZ(P_NextTransport, new VetisTransportInfo)) {
			*P_NextTransport = *rS.P_NextTransport;
		}
		else {
			ZDELETE(P_NextTransport);
		}
		return *this;
	}
	VetisShipmentRoutePoint & Z()
	{
		VetisGenericEntity::Z();
		SqnId.Z();
		UnionShipmentRoutePoint = 0;
		Flags = 0;
		ZDELETE(P_Location);
		ZDELETE(P_Enterprise);
		ZDELETE(P_NextTransport);
		return *this;
	}
	SString SqnId;
	int   UnionShipmentRoutePoint;
	VetisLocation * P_Location;
	VetisEnterprise * P_Enterprise;
	//
	enum {
		fTransShipment = 0x0001
	};
	long   Flags;
	VetisTransportInfo * P_NextTransport;
};

struct VetisRegionalizationCondition : public VetisGenericVersioningEntity { // @v11.0.11
	VetisRegionalizationCondition() : Strict(false)
	{
	}
	SString RefNumber;
	SString Text;
	bool   Strict;
	uint8  Reserve[3]; // @alignment
	VetisNamedGenericVersioningEntity RelatedDisease; // AnimalDisease
};

class VetisRegionalizationRequirement { // @v11.0.11
	VetisRegionalizationRequirement(const VetisRegionalizationRequirement & rS) {} // @disable-copy-ctr
public:
	VetisRegionalizationRequirement() : Type(0)
	{
	}
	VetisRegionalizationRequirement & FASTCALL operator = (const VetisRegionalizationRequirement & rS)
	{
		AnimalDisease = rS.AnimalDisease;
		Type = rS.Type;
		TSCollection_Copy(ConditionGroup, rS.ConditionGroup);
		return *this;
	}
	VetisNamedGenericVersioningEntity AnimalDisease; // AnimalDisease
	int    Type; // 1 - перемещение разрешено, 2 - Перемещение разрешено при обязательном соблюдении условий, 3 - Перемещение запрещено
	TSCollection <VetisRegionalizationCondition> ConditionGroup;
};

class VetisRegionalizationShippingRule { // @v11.0.11
	VetisRegionalizationShippingRule(const VetisRegionalizationShippingRule & rS) {} // @disable-copy-ctr
public:
	VetisRegionalizationShippingRule() : Decision(0)
	{
	}
	VetisRegionalizationShippingRule & FASTCALL operator = (const VetisRegionalizationShippingRule & rS)
	{
		Decision = rS.Decision;
		TSCollection_Copy(CargoTypeList, rS.CargoTypeList);
		TSCollection_Copy(ReqList, rS.ReqList);
		return *this;
	}
	int    Decision; // 1 - перемещение разрешено, 2 - Перемещение разрешено при обязательном соблюдении условий, 3 - Перемещение запрещено
	TSCollection <VetisSubProduct> CargoTypeList;
	TSCollection <VetisRegionalizationRequirement> ReqList;
};

class VetisRouteSectionR13nRules { // @v11.0.11
	VetisRouteSectionR13nRules(const VetisRouteSectionR13nRules & rS) {} // @disable-copy-ctr
public:
	VetisRouteSectionR13nRules() : SqnId(0)
	{
	}
	VetisRouteSectionR13nRules & FASTCALL operator = (const VetisRouteSectionR13nRules & rS)
	{
		SqnId = rS.SqnId;
		TSCollection_Copy(RuleList, rS.RuleList);
		return *this;
	}
	VetisRouteSectionR13nRules & Z()
	{
		SqnId = 0;
		RuleList.freeAll();
		return *this;
	}
	int    SqnId;
	TSCollection <VetisRegionalizationShippingRule> RuleList;
};

struct VetisCargoReloadingPoint {
	VetisCargoReloadingPoint()
	{
	}
	VetisCargoReloadingPoint & Z()
	{
		Name.Z();
		NextTransport.Z();
		return *this;
	}
	SString Name;
	VetisTransportInfo NextTransport;
};

struct VetisCertifiedConsignment {
	VetisCertifiedConsignment() : TransportStorageType_id(0)
	{
	}
	VetisCertifiedConsignment & operator = (const VetisCertifiedConsignment & rS)
	{
		Consignor = rS.Consignor;
		Consignee = rS.Consignee;
		Broker = rS.Broker;
		TransportInfo = rS.TransportInfo;
		TransportStorageType_id = rS.TransportStorageType_id;
		TSCollection_Copy(RoutePointList, rS.RoutePointList);
		TSCollection_Copy(CargoReloadingPointList, rS.CargoReloadingPointList);
		Batch = rS.Batch;
		return *this;
	}
	VetisCertifiedConsignment & Z()
	{
		Consignor.Z();
		Consignee.Z();
		Broker.Z();
		TransportInfo.Z();
		TransportStorageType_id = 0;
		RoutePointList.freeAll();
		CargoReloadingPointList.freeAll();
		Batch.Z();
		return *this;
	}
	VetisBusinessMember Consignor;
	VetisBusinessMember Consignee;
	VetisBusinessEntity Broker;
	VetisTransportInfo TransportInfo;
	int    TransportStorageType_id; // TransportationStorageType
	TSCollection <VetisShipmentRoutePoint> RoutePointList;
	TSCollection <VetisCargoReloadingPoint> CargoReloadingPointList;
	VetisBatch Batch;
};

struct VetisPurpose : public VetisNamedGenericVersioningEntity {
	enum {
		fForSubstandard = 0x00010000
	};
};

struct VetisVeterinaryEvent {
	VetisVeterinaryEvent() : Type(0), ActualDateTime(ZERODATETIME), UnionVeterinaryEvent(0), P_Location(0)
	{
	}
	SString ID;
	SString Name;
	int   Type; // VeterinaryEventType
	LDATETIME ActualDateTime;
	int   UnionVeterinaryEvent;
	union {
		VetisLocation * P_Location;
		VetisEnterprise * P_Enterprise;
	};
	VetisOrganization Operator;
	TSCollection <VetisDocument> ReferencedDocumentList;
	SString Notes;
};

typedef VetisNamedGenericVersioningEntity VetisIndicator;
typedef VetisNamedGenericVersioningEntity VetisAnimalDisease;
typedef VetisNamedGenericVersioningEntity VetisResearchMethod;

struct VetisLaboratoryResearchEvent : public VetisVeterinaryEvent {
	VetisLaboratoryResearchEvent() : VetisVeterinaryEvent(), Union_LaboratoryResearchEvent(0), Result(0)
	{
	}
	StringSet BatchIdList;
	SString ExpertiseID;
	int   Union_LaboratoryResearchEvent;
	VetisNamedGenericVersioningEntity IndOrDis; // VetisIndicator || VetisAnimalDisease
	VetisResearchMethod Method;
	int    Result; // ResearchResult
	SString Conclusion;
};

struct VetisQuarantineEvent : public VetisVeterinaryEvent {
	SString Duration;
};

struct VetisMedicinalDrug {
	SString ID;
	SString Name;
	SString Series;
	VetisBusinessMember Producer;
};

struct VetisAnimalMedicationEvent : public VetisVeterinaryEvent {
	VetisAnimalMedicationEvent() : VetisVeterinaryEvent(), EffectiveBeforeDate(ZERODATETIME)
	{
	}
	VetisAnimalDisease Disease;
	VetisMedicinalDrug MedicinalDrug;
	LDATETIME EffectiveBeforeDate;
};

struct VetisVetDocument : public VetisDocument {
	struct VetDocumentStatusChange {
		VetDocumentStatusChange() : Status(0), ActualDateTime(ZERODATETIME)
		{
		}
		int    Status; // VetDocumentStatus
		VetisUser SpecifiedPerson;
		LDATETIME ActualDateTime;
		SString Reason;
	};
	struct ReferencedDocument : public VetisDocument {
		ReferencedDocument() : VetisDocument(), RelationshipType(0)
		{
		}
		int    RelationshipType; // ReferenceType
	};
	struct VeterinaryAuthentication {
		struct RegionalizationClause {
			struct RegionalizationCondition : public VetisGenericVersioningEntity {
				RegionalizationCondition() : VetisGenericVersioningEntity()
				{
				}
				enum {
					fStrict = 0x00010000
				};
				SString ReferenceNumber;
				SString Text;
				TSCollection <VetisAnimalDisease> RelatedDiseaseList;
			};
			RegionalizationCondition Condition;
			SString Text;
		};
		VeterinaryAuthentication() : Flags(0), CargoExpertized(0), AnimalSpentPeriod(0)
		{
		}
		VeterinaryAuthentication & FASTCALL operator = (const VeterinaryAuthentication & rS)
		{
			Purpose = rS.Purpose;
			Flags = rS.Flags;
			CargoExpertized = rS.CargoExpertized;
			AnimalSpentPeriod = rS.AnimalSpentPeriod;
			LocationProsperity = rS.LocationProsperity;
			MonthsSpent = rS.MonthsSpent;
			SpecialMarks = rS.SpecialMarks;
			TSCollection_Copy(LaboratoryResearchList, rS.LaboratoryResearchList);
			TSCollection_Copy(ImmunizationList, rS.ImmunizationList);
			TSCollection_Copy(VeterinaryEventList, rS.VeterinaryEventList);
			TSCollection_Copy(R13nClauseList, rS.R13nClauseList);
			return *this;
		}
		VeterinaryAuthentication & Z()
		{
			Purpose.Z();
			Flags = 0;
			CargoExpertized = 0;
			AnimalSpentPeriod = 0;
			LocationProsperity.Z();
			MonthsSpent.Z();
			SpecialMarks.Z();
			LaboratoryResearchList.freeAll();
			ImmunizationList.freeAll();
			VeterinaryEventList.freeAll();
			R13nClauseList.freeAll();
			return *this;
		}
		VetisPurpose Purpose;
		enum {
			fCargoInspected = 0x0001
		};
		//
		// Descr: Варианты значения поля CargoExpertized
		//
		enum {
			ceUnrecognized = 0, // Специальное значение для индикации невозможности распознать символ значения //
			ceUnkn        = 1, // UNKNOWN   Результат неизвестен.
			ceUndef       = 2, // UNDEFINED Результат невозможно определить (не нормируется).
			cePositive    = 3, // POSITIVE 	Положительный результат.
			ceNegative    = 4, // NEGATIVE 	Отрицательный результат.
			ceUnfulfilled = 5, // UNFULFILLED 	Не проводилось.
			ceVseRaw      = 6, // VSERAW 	ВСЭ подвергнуто сырьё, из которого произведена продукция.
			ceVseFull     = 7  // VSEFULL 	Продукция подвергнута ВСЭ в полном объеме. 
		};
		long   Flags;
		int    CargoExpertized; // ResearchResult
		int    AnimalSpentPeriod; // AnimalSpentPeriod
		SString LocationProsperity;
		SString MonthsSpent;
		SString SpecialMarks;
		TSCollection <VetisLaboratoryResearchEvent> LaboratoryResearchList;
		TSCollection <VetisAnimalMedicationEvent> ImmunizationList;
		TSCollection <VetisVeterinaryEvent> VeterinaryEventList;
		TSCollection <RegionalizationClause> R13nClauseList;
	};
	VetisVetDocument() : VetisDocument(), VetDForm(0), VetDType(0), VetDStatus(0), Flags(0), LastUpdateDate(ZERODATETIME),
		UnionVetDocument(0), P_CertifiedBatch(0), WayBillDate(ZERODATE), WayBillType(0),
		NativeLotID(0), NativeBillID(0), NativeBillRow(0)
	{
	}
	~VetisVetDocument()
	{
		delete P_CertifiedBatch;
	}
	VetisVetDocument & FASTCALL operator = (const VetisVetDocument & rS)
	{
		VetisDocument::operator = (rS);
		VetDForm = rS.VetDForm;
		VetDType = rS.VetDType;
		VetDStatus = rS.VetDStatus;
		Flags = rS.Flags;
		LastUpdateDate = rS.LastUpdateDate;
		WayBillType = rS.WayBillType;
		WayBillDate = rS.WayBillDate;
		WayBillNumber = rS.WayBillNumber;
		WayBillSeries = rS.WayBillSeries;
		NativeLotID = rS.NativeLotID;
		NativeBillID = rS.NativeBillID;
		NativeBillRow = rS.NativeBillRow;
		UnionVetDocument = rS.UnionVetDocument;
		ZDELETE(P_CertifiedBatch);
		if(rS.P_CertifiedBatch) {
			P_CertifiedBatch = new VetisCertifiedBatch;
			*P_CertifiedBatch = *rS.P_CertifiedBatch;
		}
		CertifiedConsignment = rS.CertifiedConsignment;
		Authentication = rS.Authentication; // @v12.0.5 @fix
		TSCollection_Copy(ReferencedDocumentList, rS.ReferencedDocumentList);
		//TSCollection_Copy(ActivityLocationList, rS.ActivityLocationList);
		return *this;
	}
	VetisVetDocument & Z()
	{
		VetisDocument::Z();
		VetDForm = 0;
		VetDType = 0;
		VetDStatus = 0;
		Flags = 0;
		LastUpdateDate.Z();
		WayBillType = 0;
		WayBillDate = ZERODATE;
		WayBillNumber.Z();
		WayBillSeries.Z();
		NativeLotID = 0;
		NativeBillID = 0;
		NativeBillRow = 0;
		UnionVetDocument = 0;
		ZDELETE(P_CertifiedBatch);
		CertifiedConsignment.Z();
		Authentication.Z();
		PrecedingVetDocuments.Z();
		ReferencedDocumentList.freeAll();
		StatusChangeList.freeAll();
		return *this;
	}
	const VetisVetDocument::ReferencedDocument * GetWayBillRef() const
	{
		const VetisVetDocument::ReferencedDocument * p_waybill_ref_doc = 0;
		static const int acceptable_as_waybill_doc_types[] = { 1, 5/*, 6*/ };
		for(uint adtidx = 0; !p_waybill_ref_doc && adtidx < SIZEOFARRAY(acceptable_as_waybill_doc_types); adtidx++) {
			for(uint rdidx = 0; !p_waybill_ref_doc && rdidx < ReferencedDocumentList.getCount(); rdidx++) {
				const VetisVetDocument::ReferencedDocument * p_rd_item = ReferencedDocumentList.at(rdidx);
				if(p_rd_item && p_rd_item->DocumentType == acceptable_as_waybill_doc_types[adtidx])
					p_waybill_ref_doc = p_rd_item;
			}
		}
		return p_waybill_ref_doc;
	}
	enum Form {
		formCERTCU1 = 0,
		formLIC1 = 1,
		formCERTCU2 = 2,
		formLIC2 = 3,
		formCERTCU3 = 4,
		formLIC3 = 5,
		formNOTE4 = 6,
		formCERT5I = 7,
		formCERT61 = 8,
		formCERT62 = 9,
		formCERT63 = 10,
		formPRODUCTIVE = 11
	};
	int    VetDForm;   // VetisVetDocument::formXXX
	int    VetDType;   // VetDocumentType
	int    VetDStatus; // VetDocumentStatus
	enum {
		fFinalized       = 0x00010000,
		fCargoInspected  = 0x00020000,
		fCargoExpertized = 0x00040000,
		fFromMainOrg     = 0x00100000,
		fToMainOrg       = 0x00200000,
		fDiscrepancy     = 0x00400000, // В комбинации со статусом vetisdocstOUTGOING_PREPARING строка представляет
			// предварительную запись для формирование излишков либо недостач по инвентаризации
		// Следующие 2 флага уточняют условия принятия входящей партии. Если ни один из флагов не
		// установлен, то партия считается безусловно приятной.
		// Проецируются на CertifiedConsignment.Batch.Flags (fPartlyAccepted fRejected)
		fPartlyAccepted  = 0x00800000, // Партия частично принята
		fRejected        = 0x01000000, // Партия полностью отклонена
		fDiscrepancyLack = 0x02000000, // В комбинации со статутом vetisdocstOUTGOING_PREPARING и флагом fDiscrepancy
			// строка представляет предварительную запись для формирования недостач по инвентаризации
		fManufIncome     = 0x04000000, // Партия выхода из производства
		fManufExpense    = 0x08000000, // Партия расхода на производство
		fInSendingQueue  = 0x10000000  // @v11.3.4 Запись находится в очереди на отправку. Это означает, что пытаться отправлять (снова) ее не следует
	};
	long   Flags;
	LDATETIME LastUpdateDate;
	//
	enum {
		waybilltTTN         = 1,
		waybilltConsignment = 2,
		waybilltCMR         = 3,
		waybilltAvia        = 4
	};
	int    WayBillType;
	LDATE  WayBillDate;
	SString WayBillNumber;
	SString WayBillSeries;
	PPID   NativeLotID;   // Ссылка на лот в БД, на который ссылается этот сертификат
	PPID   NativeBillID;  // Ссылка на документ в БД, на который ссылается этот сертификат
	int    NativeBillRow; // Ссылка на строку документа NativeBillID, на которую ссылается этот сертификат
	int    UnionVetDocument;
	VetisCertifiedBatch * P_CertifiedBatch;
	VetisCertifiedConsignment CertifiedConsignment;
	VeterinaryAuthentication Authentication;
	SString PrecedingVetDocuments;
	TSCollection <ReferencedDocument> ReferencedDocumentList;
	TSCollection <VetDocumentStatusChange> StatusChangeList;
};

struct VetisStockEntryEventList {
	VetisStockEntryEventList & FASTCALL operator = (const VetisStockEntryEventList & rS)
	{
		TSCollection_Copy(LaboratoryResearchList, rS.LaboratoryResearchList);
		TSCollection_Copy(QuarantineList, rS.QuarantineList);
		TSCollection_Copy(ImmunizationList, rS.ImmunizationList);
		TSCollection_Copy(VeterinaryEventList, rS.VeterinaryEventList);
		return *this;
	}
	VetisStockEntryEventList & Z()
	{
		LaboratoryResearchList.freeAll();
		QuarantineList.freeAll();
		ImmunizationList.freeAll();
		VeterinaryEventList.freeAll();
		return *this;
	}
	TSCollection <VetisLaboratoryResearchEvent> LaboratoryResearchList;
	TSCollection <VetisQuarantineEvent> QuarantineList;
	TSCollection <VetisAnimalMedicationEvent> ImmunizationList;
	TSCollection <VetisVeterinaryEvent> VeterinaryEventList;
};

struct VetisStockEntry : public VetisGenericVersioningEntity {
	VetisStockEntry() : VetisGenericVersioningEntity()
	{
	}
	VetisStockEntry & FASTCALL operator = (const VetisStockEntry & rS)
	{
		VetisGenericVersioningEntity::operator = (rS);
		EntryNumber = rS.EntryNumber;
		Batch = rS.Batch;
		VetEventList = rS.VetEventList;
		TSCollection_Copy(VetDocumentList, rS.VetDocumentList);
		return *this;
	}
	VetisStockEntry & Z()
	{
		VetisGenericVersioningEntity::Z();
		EntryNumber.Z();
		Batch.Z();
	}
	SString EntryNumber;
	VetisBatch Batch;
	VetisStockEntryEventList VetEventList;
	TSCollection <VetisVetDocument> VetDocumentList;
};

struct VetisStockEntrySearchPattern : public VetisStockEntry {
	VetisStockEntrySearchPattern() : VetisStockEntry(), BlankFilter(0)
	{
	}
	int    BlankFilter; // StockEntryBlankFilter
};

class VetisApplicationData {
public:
	enum {
		signNone = 0,
		signGetStockEntryList,
		signGetStockEntryChangesList,
		signGetBusinessEntity,
		signGetBusinessEntityByGuid,
		signGetAppliedUserAuthorityList,
		signGetRussianEnterpriseList,
		signGetVetDocumentList,
		signGetVetDocumentChangesList,
		signGetProductItemList,
		signGetVetDocumentByUuid,
		signProcessIncomingConsignment,
		signProcessOutgoingConsignment,
		signWithdrawVetDocument,
		signModifyEnterprise,
		signModifyActivityLocations,
		signResolveDiscrepancy,
		signModifyProducerStockListOperation,
		signGetStockEntryByUUID,
		signRegisterProduction,
		signCheckShipmentRegionalization // @v11.0.0
	};
	explicit VetisApplicationData(long sign) : Sign(sign)
	{
	}
	virtual ~VetisApplicationData()
	{
	}
	//
	const long Sign;
	SString LocalTransactionId;
	VetisUser Initiator;
	SString SessionToken;
};

class VetisListRequest : public VetisApplicationData {
public:
	VetisListRequest(long sign) : VetisApplicationData(sign)
	{
	}
	VetisListOptions ListOptions;
};

class VetisGetStockEntryListRequest : public VetisListRequest {
public:
	VetisGetStockEntryListRequest() : VetisListRequest(signGetStockEntryList)
	{
	}
	VetisStockEntrySearchPattern SearchPattern;
};

class VetisGetStockEntryChangesListRequest : public VetisListRequest {
public:
	VetisGetStockEntryChangesListRequest() : VetisListRequest(signGetStockEntryChangesList)
	{
	}
	STimeChunk Period;
};

class VetisGetStockEntryByUUIDRequest : public VetisApplicationData { // getStockEntryByUUID
public:
	VetisGetStockEntryByUUIDRequest() : VetisApplicationData(signGetStockEntryByUUID)
	{
	}
	S_GUID Uuid;
};

class VetisGetVetDocumentListRequest_Base : public VetisListRequest {
protected:
	VetisGetVetDocumentListRequest_Base(long sign) : VetisListRequest(sign), DocType(-1), DocStatus(-1)
	{
	}
public:
	int    DocType;   // VetisDocType
	int    DocStatus; // VetisDocStatus
	STimeChunk Period;
};

class VetisGetVetDocumentListRequest : public VetisGetVetDocumentListRequest_Base {
public:
	VetisGetVetDocumentListRequest() : VetisGetVetDocumentListRequest_Base(signGetVetDocumentList)
	{
	}
};

class VetisGetVetDocumentChangesListRequest : public VetisGetVetDocumentListRequest_Base {
public:
	VetisGetVetDocumentChangesListRequest() : VetisGetVetDocumentListRequest_Base(signGetVetDocumentChangesList)
	{
	}
};

class VetisGetVetDocumentByUuidRequest : public VetisApplicationData {
public:
	VetisGetVetDocumentByUuidRequest() : VetisApplicationData(signGetVetDocumentByUuid)
	{
	}
	S_GUID Uuid;
};

class VetisProcessIncomingConsignmentRequest : public VetisApplicationData {
public:
	VetisProcessIncomingConsignmentRequest() : VetisApplicationData(signProcessIncomingConsignment)
	{
	}
	VetisVetDocument Doc;
};

class VetisPrepareOutgoingConsignmentRequest : public VetisApplicationData {
public:
	VetisPrepareOutgoingConsignmentRequest() : VetisApplicationData(signProcessOutgoingConsignment), P_RegionRules(0)
	{
	}
	VetisDocumentTbl::Rec VdRec;
	VetisVetDocument OrgDoc; // Документ, в соответствии с которым товар оказался у нас
	S_GUID FromBusinessEntGuid;
	S_GUID FromEnterpiseGuid;
	S_GUID ToBusinessEntGuid;
	S_GUID ToEnterpiseGuid;
	S_GUID StockEntryGuid;
	S_GUID StockEntryUuid;
	VetisTransportInfo Transp;
	SString TranspStorageType;
	const TSCollection <VetisRouteSectionR13nRules> * P_RegionRules; // @v11.0.11 @notowned
};

class VetisWithdrawVetDocumentRequest : public VetisApplicationData {
public:
	VetisWithdrawVetDocumentRequest() : VetisApplicationData(signWithdrawVetDocument)
	{
	}
	VetisVetDocument Doc;
};

class VetisModifyEnterpriseRequest : public VetisApplicationData {
public:
	VetisModifyEnterpriseRequest(VetisRegisterModificationType mt) : VetisApplicationData(signModifyEnterprise), ModType(mt)
	{
	}
	VetisRegisterModificationType ModType;
	VetisEnterprise En;
};

class VetisModifyActivityLocationsRequest : public VetisApplicationData {
public:
	VetisModifyActivityLocationsRequest(VetisRegisterModificationType mt) : VetisApplicationData(signModifyActivityLocations), ModType(mt)
	{
	}
	VetisRegisterModificationType ModType;
	VetisBusinessEntity Be;
	VetisEnterprise En;
};

class ModifyProducerStockListOperationRequest : public VetisApplicationData {
public:
	ModifyProducerStockListOperationRequest(VetisRegisterModificationType mt) : VetisApplicationData(signModifyProducerStockListOperation), ModType(mt)
	{
	}
	VetisRegisterModificationType ModType;
	VetisProductItem Pi;
};

class CheckShipmentRegionalizationRequest : public VetisApplicationData {
public:
	CheckShipmentRegionalizationRequest() : VetisApplicationData(signCheckShipmentRegionalization)
	{
	}
	UuidArray SubGroupGuidList; // cargoType list
	TSCollection <VetisShipmentRoutePoint> RoutePointList;
};

class VetisResolveDiscrepancyRequest : public VetisApplicationData {
public:
	VetisResolveDiscrepancyRequest() : VetisApplicationData(signResolveDiscrepancy), StockEntryRest(0.0)
	{
	}
	VetisDocumentTbl::Rec VdRec;
	VetisVetDocument Doc;
	VetisProductItem Pi;
	VetisVetDocument OrgDoc; // Документ, в соответствии с которым товар оказался у нас
	S_GUID StockEntryGuid;   //
	S_GUID StockEntryUuid;   //
	double StockEntryRest;   //
};

class VetisRegisterProductionRequest : public VetisApplicationData {
public:
	VetisRegisterProductionRequest() : VetisApplicationData(signRegisterProduction)
	{
	}
	struct SourceEntry {
		VetisDocumentTbl::Rec Rec;
		VetisVetDocument Doc;
		S_GUID StockEntryGuid;
		S_GUID StockEntryUuid;
		double StockEntryRest;
	};
	VetisDocumentTbl::Rec VdRec;
	VetisVetDocument Doc;
	VetisProductItem Pi;
	TSCollection <SourceEntry> SourceList;
};

struct VetisApplicationBlock {
	VetisApplicationBlock(SVerT vetisSvcVer, const VetisApplicationData * pAppParam);
	VetisApplicationBlock();
	VetisApplicationBlock(const VetisApplicationBlock & rS);
	~VetisApplicationBlock();
	VetisApplicationBlock & FASTCALL operator = (const VetisApplicationBlock & rS);
	void   Clear();
	int    FASTCALL Copy(const VetisApplicationBlock & rS);
	const  char * GetServiceId() const
	{
		if(VetisSvcVer_ == SVerT(2, 1))
			return "mercury-g2b.service:2.1";
		else if(VetisSvcVer_ == SVerT(2, 0))
			return "mercury-g2b.service:2.0";
		else if(VetisSvcVer_ == SVerT(1, 0))
			return "mercury-g2b.service";
		else
			return "mercury-g2b.service:2.1";
	}
	enum {
		appstUndef     = -1,
		appstAccepted  = 0,
		appstInProcess = 1,
		appstCompleted = 2,
		appstRejected  = 3
	};
	enum {
		opUndef = 0,
		opGetVetDocumentList
	};
	struct  ReplyListValues {
		ReplyListValues() : Count(0), Offset(0), Total(0)
		{
		}
		long   Count;
		long   Offset;
		long   Total;
	};
	//uint   VetisSvcVer; // 1, 2
	SVerT  VetisSvcVer_; // 1.0, 2.0, 2.1
	int    ApplicationStatus;
	int64  LocalTransactionId;
	SString ServiceId;
	SString User;
	S_GUID ApplicationId;
	S_GUID IssuerId;
	S_GUID EnterpriseId;
	LDATETIME IssueDate;
	LDATETIME RcvDate;
	LDATETIME PrdcRsltDate;
	ReplyListValues ListResult;
	SString AppData; // xml-block
	TSCollection <VetisErrorEntry> ErrList;
	TSCollection <VetisFault> FaultList;
	const VetisApplicationData * P_AppParam;
	TSCollection <VetisProductItem> ProductItemList; // Ответ на запрос одного или нескольких ProductItem
	TSCollection <VetisProduct> ProductList;
	TSCollection <VetisSubProduct> SubProductList;
	TSCollection <VetisEnterprise> EntItemList;
	TSCollection <VetisBusinessEntity> BEntList;
	TSCollection <VetisVetDocument> VetDocList;
	TSCollection <VetisStockEntry> VetStockList;
	TSCollection <VetisUnit> UnitList;
	TSCollection <VetisPurpose> PurposeList;
	TSCollection <VetisCountry> CountryList;
	TSCollection <VetisAddressObjectView> RegionList;
	TSCollection <VetisRouteSectionR13nRules> R13RulesList; // @v11.0.11
};
//
//
//
VetisEnterpriseOfficialRegistration::VetisEnterpriseOfficialRegistration() : P_BusinessEntity(0)
{
}

VetisEnterpriseOfficialRegistration::~VetisEnterpriseOfficialRegistration()
{
	delete P_BusinessEntity;
}

VetisEnterpriseOfficialRegistration & FASTCALL VetisEnterpriseOfficialRegistration::operator = (const VetisEnterpriseOfficialRegistration & rS)
{
	ID = rS.ID;
	Kpp = rS.Kpp;
	ZDELETE(P_BusinessEntity);
	if(rS.P_BusinessEntity) {
		P_BusinessEntity = new VetisBusinessEntity;
		ASSIGN_PTR(P_BusinessEntity, *rS.P_BusinessEntity);
	}
	return *this;
}

VetisUser::VetisUser() : BirthDate(ZERODATE), Flags(0), UnionUser(0), P_Organization(0), P_BusinessEntity(0)
{
}

VetisUser::~VetisUser()
{
	delete P_Organization;
	delete P_BusinessEntity;
}

VetisUser & FASTCALL VetisUser::operator = (const VetisUser & rS)
{
	Login = rS.Login;
	Fio = rS.Fio;
	BirthDate = rS.BirthDate;
	Identity = rS.Identity; // !
	Snils = rS.Snils;
	Phone = rS.Phone;
	Email = rS.Email;
	WorkEmail = rS.WorkEmail;
	UnionUser = rS.UnionUser;
	Post = rS.Post;
	Flags = rS.Flags;
	TSCollection_Copy(AuthorityList, rS.AuthorityList);
	ZDELETE(P_Organization);
	ZDELETE(P_BusinessEntity);
	if(rS.P_Organization) {
		P_Organization = new VetisOrganization;
		ASSIGN_PTR(P_Organization, *rS.P_Organization);
	}
	if(rS.P_BusinessEntity) {
		P_BusinessEntity = new VetisBusinessEntity;
		ASSIGN_PTR(P_BusinessEntity, *rS.P_BusinessEntity);
	}
	return *this;
}

VetisEnterprise::VetisEnterprise() : P_Owner(0), Type(0), NativeLocID(0)
{
}

VetisEnterprise::~VetisEnterprise()
{
	delete P_Owner;
}

VetisEnterprise & FASTCALL VetisEnterprise::operator = (const VetisEnterprise & rS)
{
	VetisNamedGenericVersioningEntity::operator = (rS);
	EnglishName = rS.EnglishName;
	Type = rS.Type;
	NativeLocID = rS.NativeLocID;
	NumberList = rS.NumberList;
	Address = rS.Address;
	ActivityList = rS.ActivityList;
	ZDELETE(P_Owner);
	if(rS.P_Owner) {
		P_Owner = new VetisBusinessEntity;
		ASSIGN_PTR(P_Owner, *rS.P_Owner);
	}
	TSCollection_Copy(OfficialRegistration, rS.OfficialRegistration);
	return *this;
}

VetisEnterprise & VetisEnterprise::Z()
{
	VetisNamedGenericVersioningEntity::Z();
	EnglishName.Z();
	Type = 0;
	NativeLocID = 0;
	NumberList.Z();
	Address.Z();
	ActivityList.Z();
	ZDELETE(P_Owner);
	OfficialRegistration.freeAll();
	return *this;
}

VetisApplicationBlock::VetisApplicationBlock(SVerT vetisSvcVer, const VetisApplicationData * pAppParam) :
	VetisSvcVer_(vetisSvcVer), ApplicationStatus(appstUndef), IssueDate(ZERODATETIME), RcvDate(ZERODATETIME),
	PrdcRsltDate(ZERODATETIME), P_AppParam(pAppParam), LocalTransactionId(0)
{
}

VetisApplicationBlock::VetisApplicationBlock() :
	VetisSvcVer_(2, 1), ApplicationStatus(appstUndef), IssueDate(ZERODATETIME), RcvDate(ZERODATETIME),
	PrdcRsltDate(ZERODATETIME), P_AppParam(0), LocalTransactionId(0)
{
}

VetisApplicationBlock::VetisApplicationBlock(const VetisApplicationBlock & rS) //: P_GselReq(0)
{
	Copy(rS);
}

VetisApplicationBlock::~VetisApplicationBlock()
{
}

VetisApplicationBlock & FASTCALL VetisApplicationBlock::operator = (const VetisApplicationBlock & rS)
{
	Copy(rS);
	return *this;
}

void VetisApplicationBlock::Clear()
{
	ApplicationStatus = appstUndef;
	LocalTransactionId = 0;
	ServiceId.Z();
	User.Z();
	ApplicationId.Z();
	IssuerId.Z();
	EnterpriseId.Z();
	IssueDate.Z();
	RcvDate.Z();
	PrdcRsltDate.Z();
	ErrList.freeAll();
	FaultList.freeAll();
	ProductItemList.freeAll();
	ProductList.freeAll();
	SubProductList.freeAll();
	EntItemList.freeAll();
	BEntList.freeAll();
	VetDocList.freeAll();
	VetStockList.freeAll();
	UnitList.freeAll();
	PurposeList.freeAll();
	CountryList.freeAll();
	RegionList.freeAll();
	R13RulesList.freeAll(); // @v11.0.11
	MEMSZERO(ListResult);
	AppData.Z();
}

int FASTCALL VetisApplicationBlock::Copy(const VetisApplicationBlock & rS)
{
	int    ok = 1;
	VetisSvcVer_ = rS.VetisSvcVer_;
	ApplicationStatus = rS.ApplicationStatus;
	LocalTransactionId = rS.LocalTransactionId;
	ServiceId = rS.ServiceId;
	User = rS.User;
	ApplicationId = rS.ApplicationId;
	IssuerId = rS.IssuerId;
	EnterpriseId = rS.EnterpriseId;
	IssueDate = rS.IssueDate;
	RcvDate = rS.RcvDate;
	PrdcRsltDate = rS.PrdcRsltDate;
	TSCollection_Copy(ErrList, rS.ErrList);
	TSCollection_Copy(FaultList, rS.FaultList);
	TSCollection_Copy(ProductItemList, rS.ProductItemList);
	TSCollection_Copy(ProductList, rS.ProductList);
	TSCollection_Copy(SubProductList, rS.SubProductList);
	TSCollection_Copy(EntItemList, rS.EntItemList);
	TSCollection_Copy(BEntList, rS.BEntList);
	TSCollection_Copy(VetDocList, rS.VetDocList);
	TSCollection_Copy(VetStockList, rS.VetStockList);
	TSCollection_Copy(UnitList, rS.UnitList);
	TSCollection_Copy(PurposeList, rS.PurposeList);
	TSCollection_Copy(CountryList, rS.CountryList);
	TSCollection_Copy(RegionList, rS.RegionList);
	TSCollection_Copy(R13RulesList, rS.R13RulesList); // @v11.0.11
	ListResult = rS.ListResult;
	AppData = rS.AppData;
	P_AppParam = rS.P_AppParam;
	return ok;
}

VetisEntityCore::Entity::Entity() : ID(0), Kind(kUndef), Flags(0), Status(0), GuidRef(0), UuidRef(0)
{
}

VetisEntityCore::Entity::Entity(int kind, const VetisProductItem & rS) : ID(0), Kind(kind), Status(rS.Status), Flags(rS.Flags)
{
	assert(oneof3(kind, kProductItem, kProduct, kSubProduct));
	if(kind == kProductItem) {
		Guid = rS.Guid;
		Uuid = rS.Uuid;
		Name = rS.Name;
	}
	else if(kind == kProduct) {
		Guid = rS.Product.Guid;
		Uuid = rS.Product.Uuid;
		Name = rS.Product.Name;
	}
	else if(kind == kSubProduct) {
		Guid = rS.SubProduct.Guid;
		Uuid = rS.SubProduct.Uuid;
		Name = rS.SubProduct.Name;
	}
}

VetisEntityCore::Entity::Entity(int kind, const VetisNamedGenericVersioningEntity & rS) : ID(0), Kind(kind), Status(rS.Status), Flags(rS.Flags)
{
	Guid = rS.Guid;
	Uuid = rS.Uuid;
	Name = rS.Name;
}

VetisEntityCore::Entity::Entity(const VetisVetDocument & rS) : ID(0), Kind(kVetDocument), Status(0), Flags(rS.Flags)
{
	Uuid = rS.Uuid;
}

VetisEntityCore::Entity::Entity(const VetisStockEntry & rS) : ID(0), Kind(kStockEntry), Status(0), Flags(rS.Flags)
{
	Guid = rS.Guid;
	Uuid = rS.Uuid;
	Name = rS.EntryNumber;
}

void FASTCALL VetisEntityCore::Entity::Get(VetisNamedGenericVersioningEntity & rD) const
{
	rD.Guid = Guid;
	rD.Uuid = Uuid;
	rD.Name = Name;
}

VetisEntityCore::Entity & VetisEntityCore::Entity::Z()
{
	ID = 0;
	Kind = 0;
	Guid.Z();
	Uuid.Z();
	Flags = 0;
	Status = 0;
	GuidRef = 0;
	UuidRef = 0;
	Name.Z();
	return *this;
}

void VetisEntityCore::Entity::SetupVetDocument()
{
	Uuid.Generate();
	Kind = VetisEntityCore::kVetDocument;
}

VetisEntityCore::VetisEntityCore()
{
}

/*static*/int FASTCALL VetisEntityCore::ValidateEntityKind(int kind)
{
	return oneof10(kind, kProductItem, kProduct, kSubProduct, kEnterprise, kBusinessEntity,
		kUOM, kCountry, kRegion, kVetDocument, kStockEntry);
}

int VetisEntityCore::CollectUnresolvedEntityList(TSVector <UnresolvedEntity> & rList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SStringU temp_buf_u;
	VetisEntityTbl::Key0 k0;
	MEMSZERO(k0);
	BExtQuery q(&ET, 0);
	q.selectAll();
	for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
		VetisEntityTbl::Rec rec;
		ET.CopyBufTo(&rec);
		if(oneof5(rec.Kind, kProductItem, kProduct, kSubProduct, kEnterprise, kBusinessEntity)) {
			if(rec.GuidRef || rec.UuidRef) {
				if(!rList.lsearch(&rec.ID, 0, CMPF_LONG)) {
					if(p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, rec.ID, PPTRPROP_NAME), temp_buf_u) > 0 && temp_buf_u.Len())
						;
					else {
						UnresolvedEntity ure_entry;
						ure_entry.ID = rec.ID;
						ure_entry.Kind = rec.Kind;
						ure_entry.GuidRef = rec.GuidRef;
						ure_entry.UuidRef = rec.UuidRef;
						THROW_SL(rList.insert(&ure_entry));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::GetEntityByGuid(const S_GUID & rGuid, Entity & rE)
{
	int     ok = -1;
	long    uuid_id = 0;
	if(UrT.SearchUuid(rGuid, 0, &uuid_id) > 0) {
		VetisEntityTbl::Key1 k1;
		MEMSZERO(k1);
		k1.GuidRef = uuid_id;
		if(ET.search(1, &k1, spGe) && ET.data.GuidRef == uuid_id) {
			EntityRecToEntity(ET.data, rE);
			ok = 1;
		}
	}
	return ok;
}

int VetisEntityCore::GetEntityListByGuid(const S_GUID & rGuid, TSCollection <Entity> & rList)
{
	int     ok = -1;
	long    uuid_id = 0;
	if(UrT.SearchUuid(rGuid, 0, &uuid_id) > 0) {
		VetisEntityTbl::Key1 k1;
		MEMSZERO(k1);
		k1.GuidRef = uuid_id;
		if(ET.search(1, &k1, spGe) && ET.data.GuidRef == uuid_id) do {
			Entity * p_entity = rList.CreateNewItem();
			if(p_entity) {
				EntityRecToEntity(ET.data, *p_entity);
			}
			ok = 1;
		} while(ET.search(1, &k1, spNext) && ET.data.GuidRef == uuid_id);
	}
	return ok;
}

int VetisEntityCore::GetEntityByUuid(const S_GUID & rUuid, Entity & rE)
{
	int     ok = -1;
	long    uuid_id = 0;
	if(UrT.SearchUuid(rUuid, 0, &uuid_id) > 0) {
		VetisEntityTbl::Key2 k2;
		MEMSZERO(k2);
		k2.UuidRef = uuid_id;
		if(ET.search(2, &k2, spGe) && ET.data.UuidRef == uuid_id) {
			EntityRecToEntity(ET.data, rE);
			ok = 1;
		}
	}
	return ok;
}

int VetisEntityCore::EntityRecToEntity(const VetisEntityTbl::Rec & rRec, Entity & rE)
{
	int    ok = 1;
	rE.ID = rRec.ID;
	rE.Kind = rRec.Kind;
	rE.Flags = rRec.Flags;
	rE.Status = rRec.Status;
	rE.GuidRef = rRec.GuidRef;
	rE.UuidRef = rRec.UuidRef;
	UrT.Search(rRec.GuidRef, rE.Guid);
	UrT.Search(rRec.UuidRef, rE.Uuid);
	{
		SStringU temp_buf_u;
		PPRef->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, rRec.ID, PPTRPROP_NAME), temp_buf_u);
		if(temp_buf_u.Len()) {
			temp_buf_u.CopyToUtf8(rE.Name, 0);
			rE.Name.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else
			rE.Name.Z();
	}
	return ok;
}

int VetisEntityCore::GetEntity(PPID id, Entity & rE)
{
	rE.Z();
	VetisEntityTbl::Rec rec;
	int    ok = SearchByID(&ET, PPOBJ_VETISENTITY, id, &rec);
	if(ok > 0) {
		EntityRecToEntity(rec, rE);
	}
	return ok;
}

int VetisEntityCore::DeleteEntity(PPID id, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	VetisEntityTbl::Rec rec;
	VetisDocumentTbl::Rec doc_rec;
	struct TranspNumTabEntry {
		int    TxtIdent;
	};
	static const TranspNumTabEntry transp_num_tab[] = {
		{ txtprpTranspVNum },
		{ txtprpTranspTNum },
		{ txtprpTranspCNum },
		{ txtprpTranspWNum },
		{ txtprpTranspSNam },
		{ txtprpTranspFNum }
	};
	{
		//int   is_uuid_ref_removed = 0;
		PPTransaction tra(use_ta);
		THROW(tra);
		/*if(SearchByID_ForUpdate(&DT, PPOBJ_VETISENTITY, id, &doc_rec) > 0) {
			THROW_DB(DT.deleteRec());
		}*/
		if(SearchByID_ForUpdate(&ET, PPOBJ_VETISENTITY, id, &rec) > 0) {
			THROW_DB(ET.deleteRec());
			if(rec.GuidRef)
				THROW(UrT.Remove(rec.GuidRef, 0));
			if(rec.UuidRef)
				THROW(UrT.Remove(rec.UuidRef, 0));
			THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, PPTRPROP_NAME), 0, 0));
			if(rec.Kind == kVetDocument) {
				THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, VetisEntityCore::txtprpProductItemName), 0, 0));
				for(uint tti = 0; tti < SIZEOFARRAY(transp_num_tab); tti++) {
					const TranspNumTabEntry & r_tt_entry = transp_num_tab[tti];
					THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, r_tt_entry.TxtIdent), 0, 0));
				}
				THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, VetisEntityCore::txtprpGoodsCodeList), 0, 0));
				if(SearchByID_ForUpdate(&DT, PPOBJ_VETISENTITY, id, &doc_rec) > 0) {
					THROW_DB(DT.deleteRec());
				}
			}
			else if(oneof3(rec.Kind, kProduct, kSubProduct, kProductItem)) {
			}
			else if(oneof2(rec.Kind, kEnterprise, kBusinessEntity)) {
			}
		}
		else {
			if(SearchByID_ForUpdate(&DT, PPOBJ_VETISENTITY, id, &doc_rec) > 0) {
				THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, VetisEntityCore::txtprpProductItemName), 0, 0));
				for(uint tti = 0; tti < SIZEOFARRAY(transp_num_tab); tti++) {
					const TranspNumTabEntry & r_tt_entry = transp_num_tab[tti];
					THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, r_tt_entry.TxtIdent), 0, 0));
				}
				THROW(p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, id, VetisEntityCore::txtprpGoodsCodeList), 0, 0));
				THROW_DB(DT.deleteRec());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::SetEntity(Entity & rE, TSVector <UnresolvedEntity> * pUreList, PPID * pID, int use_ta)
{
	int    ok = 1;
	int    is_new_entity = 0;
	int    is_unresolved = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SString name_buf;
	SStringU temp_buf_u;
	VetisEntityTbl::Rec rec;
	PPID   entity_id = 0;
	assert(ValidateEntityKind(rE.Kind));
	THROW(ValidateEntityKind(rE.Kind));
	if(!rE.Guid && !rE.Uuid) {
		rE.ID = 0;
	}
	else {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(UrT.GetUuid(rE.Guid, &rec.GuidRef, 0, 0));
		THROW(UrT.GetUuid(rE.Uuid, &rec.UuidRef, 0, 0));
		rE.GuidRef = rec.GuidRef;
		rE.UuidRef = rec.UuidRef;
		rec.Flags = rE.Flags;
		rec.Kind = rE.Kind;
		rec.Status = rE.Status;
		{
			VetisEntityTbl::Key1 k1;
			k1.GuidRef = rec.GuidRef;
			k1.UuidRef = rec.UuidRef;
			if(ET.searchForUpdate(1, &k1, spEq)) {
				entity_id = ET.data.ID;
				assert(rE.ID == 0 || entity_id == rE.ID);
				rE.ID = entity_id;
				rec.ID = entity_id;
				if(memcmp(&ET.data, &rec, sizeof(rec)) != 0) {
					THROW_DB(ET.updateRecBuf(&rec)); // @sfu
				}
				else
					ok = -1;
			}
			else {
				THROW_DB(ET.insertRecBuf(&rec, 0, &entity_id));
				rE.ID = entity_id;
				is_new_entity = 1;
			}
		}
		name_buf = rE.Name;
		if(name_buf.NotEmptyS()) {
			name_buf.Trim(124);
			temp_buf_u.CopyFromUtf8(name_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8));
			int sr = p_ref->TrT.SetText(TextRefIdent(PPOBJ_VETISENTITY, entity_id, PPTRPROP_NAME), temp_buf_u, 0);
			THROW(sr);
			if(sr > 0 && ok < 0)
				ok = 1;
		}
		else if(is_new_entity)
			is_unresolved = 1;
		THROW(tra.Commit());
		if(pUreList && rE.Kind != kVetDocument) {
			if(!is_unresolved) {
				if(p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, entity_id, PPTRPROP_NAME), temp_buf_u) > 0 && temp_buf_u.Len())
					;
				else
					is_unresolved = 1;
			}
			if(is_unresolved && (is_new_entity || !pUreList->lsearch(&entity_id, 0, CMPF_LONG))) {
				UnresolvedEntity ure_entry;
				ure_entry.ID = entity_id;
				ure_entry.Kind = rE.Kind;
				ure_entry.GuidRef = rE.GuidRef;
				ure_entry.UuidRef = rE.UuidRef;
				THROW_SL(pUreList->insert(&ure_entry));
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, rE.ID);
	return ok;
}

int VetisEntityCore::SearchPerson(PPID id, VetisPersonTbl::Rec * pRec) { return SearchByID(&BT, 0, id, pRec); }
int VetisEntityCore::SearchDocument(PPID id, VetisDocumentTbl::Rec * pRec) { return SearchByID(&DT, 0, id, pRec); }

int VetisEntityCore::MatchDocument(PPID docEntityID, PPID billID, int rowN, int fromBill, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	VetisDocumentTbl::Rec rec;
	VetisDocumentTbl::Rec org_rec;
	VetisDocumentTbl::Key0 k0;
	TransferTbl::Rec trfr_rec;
	k0.EntityID = docEntityID;
	int16   row_n = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(rowN <= 0) {
			row_n = -1;
		}
		else {
			row_n = rowN;
			THROW(BillObj->trfr->SearchByBill(billID, 0, row_n, &trfr_rec) > 0);
			THROW(trfr_rec.Flags & PPTFR_RECEIPT && trfr_rec.LotID != 0);
		}
		if(DT.searchForUpdate(0, &k0, spEq)) {
			DT.CopyBufTo(&org_rec);
			DT.CopyBufTo(&rec);
			rec.LinkBillID = billID;
			rec.LinkBillRow = row_n;
			if(row_n > 0) {
				rec.LinkGoodsID = trfr_rec.GoodsID;
			}
			THROW_DB(DT.updateRecBuf(&rec));
			if(trfr_rec.LotID && !fromBill) {
				Entity entity;
				if(GetEntity(docEntityID, entity) > 0 && !!entity.Uuid) {
					ObjTagItem tag_item;
					if(tag_item.SetGuid(PPTAG_LOT_VETIS_UUID, &entity.Uuid)) {
						THROW(p_ref->Ot.PutTag(PPOBJ_LOT, trfr_rec.LotID, &tag_item, 0));
					}
				}
			}
			if(row_n < 0) {
				if(checkdate(rec.WayBillDate) && rec.WayBillNumber[0]) {
					PPIDArray additional_to_upd_list;
					VetisDocumentTbl::Key3 k3;
					MEMSZERO(k3);
					k3.WayBillDate = rec.WayBillDate;
					STRNSCPY(k3.WayBillNumber, rec.WayBillNumber);
					if(DT.search(3, &k3, spGe) && DT.data.WayBillDate == rec.WayBillDate && sstreq(rec.WayBillNumber, DT.data.WayBillNumber)) do {
						if(DT.data.EntityID != rec.EntityID && (DT.data.LinkBillID == 0 /*|| DT.data.LinkBillID == org_rec.LinkBillID*/)) {
							additional_to_upd_list.add(DT.data.EntityID);
						}
					} while(DT.search(3, &k3, spNext) && DT.data.WayBillDate == rec.WayBillDate && sstreq(rec.WayBillNumber, DT.data.WayBillNumber));
					for(uint i = 0; i < additional_to_upd_list.getCount(); i++) {
						const  PPID additional_to_upd_id = additional_to_upd_list.get(i);
						THROW(MatchDocument(additional_to_upd_id, billID, rowN, fromBill, 0)); // @recursion
					}
				}
			}
		}
		THROW(tra.Commit())
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::MatchPersonInDocument(PPID docEntityID, int side /*0 - from, 1 - to*/, PPID personID, PPID dlvrLocID, int use_ta)
{
	assert(oneof2(side, 0, 1));
	int    ok = -1;
	Reference * p_ref = PPRef;
	VetisDocumentTbl::Rec rec;
	VetisDocumentTbl::Key0 k0;
	VetisPersonTbl::Key0 vp_key0;
	k0.EntityID = docEntityID;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(DT.searchForUpdate(0, &k0, spEq)) {
			DT.CopyBufTo(&rec);
			Entity entity;
			ObjTagItem tag_item;
			int    is_psn_guid_set = 0;
			if(side == 0) { // FROM
				const  PPID base_entity_id = rec.FromEnterpriseID;
				if(rec.LinkFromPsnID != personID || rec.LinkFromDlvrLocID != dlvrLocID) {
					rec.LinkFromPsnID = personID;
					rec.LinkFromDlvrLocID = dlvrLocID;
					THROW_DB(DT.updateRecBuf(&rec));
					ok = 1;
				}
				if(GetEntity(base_entity_id, entity) > 0 && !!entity.Guid) {
					if(dlvrLocID) {
						if(tag_item.SetGuid(PPTAG_LOC_VETIS_GUID, &entity.Guid)) {
							THROW(p_ref->Ot.PutTag(PPOBJ_LOCATION, dlvrLocID, &tag_item, 0));
							ok = 1;
						}
					}
					else if(personID) {
						if(tag_item.SetGuid(PPTAG_PERSON_VETISUUID, &entity.Guid)) {
							THROW(p_ref->Ot.PutTag(PPOBJ_PERSON, personID, &tag_item, 0));
							ok = 1;
							is_psn_guid_set = 1;
						}
					}
				}
				if(!is_psn_guid_set && GetEntity(rec.FromEntityID, entity) > 0 && !!entity.Guid) {
					if(personID && tag_item.SetGuid(PPTAG_PERSON_VETISUUID, &entity.Guid)) {
						THROW(p_ref->Ot.PutTag(PPOBJ_PERSON, personID, &tag_item, 0));
						ok = 1;
					}
				}
				{
					const int idx = 5;
					VetisDocumentTbl::Key5 key;
					if(base_entity_id) {
						MEMSZERO(key);
						key.FromEnterpriseID = base_entity_id;
						if(DT.searchForUpdate(idx, &key, spGe) && DT.data.FromEnterpriseID == base_entity_id) do {
							if(DT.data.LinkFromPsnID != personID || DT.data.LinkFromDlvrLocID != dlvrLocID) {
								DT.data.LinkFromPsnID = personID;
								DT.data.LinkFromDlvrLocID = dlvrLocID;
								THROW_DB(DT.updateRec());
								ok = 1;
							}
						} while(DT.searchForUpdate(idx, &key, spNext) && DT.data.FromEnterpriseID == base_entity_id);
						{
							MEMSZERO(vp_key0);
							vp_key0.EntityID = base_entity_id;
							if(BT.searchForUpdate(0, &vp_key0, spEq) && (BT.data.LinkPersonID != personID || BT.data.LinkDlvrLocID != dlvrLocID)) {
								BT.data.LinkPersonID = personID;
								BT.data.LinkDlvrLocID = dlvrLocID;
								THROW_DB(BT.updateRec());
								ok = 1;
							}
						}
					}
				}
			}
			else { // TO
				const  PPID base_entity_id = rec.ToEnterpriseID;
				if(rec.LinkToPsnID != personID || rec.LinkToDlvrLocID != dlvrLocID) {
					rec.LinkToPsnID = personID;
					rec.LinkToDlvrLocID = dlvrLocID;
					THROW_DB(DT.updateRecBuf(&rec));
					ok = 1;
				}
				if(GetEntity(base_entity_id, entity) > 0 && !!entity.Guid) {
					if(dlvrLocID) {
						if(tag_item.SetGuid(PPTAG_LOC_VETIS_GUID, &entity.Guid)) {
							THROW(p_ref->Ot.PutTag(PPOBJ_LOCATION, dlvrLocID, &tag_item, 0));
							ok = 1;
						}
					}
					else if(personID) {
						if(tag_item.SetGuid(PPTAG_PERSON_VETISUUID, &entity.Guid)) {
							THROW(p_ref->Ot.PutTag(PPOBJ_PERSON, personID, &tag_item, 0));
							ok = 1;
							is_psn_guid_set = 1;
						}
					}
				}
				if(!is_psn_guid_set && GetEntity(rec.ToEntityID, entity) > 0 && !!entity.Guid) {
					if(personID && tag_item.SetGuid(PPTAG_PERSON_VETISUUID, &entity.Guid)) {
						THROW(p_ref->Ot.PutTag(PPOBJ_PERSON, personID, &tag_item, 0));
						ok = 1;
					}
				}
				{
					const int idx = 7;
					VetisDocumentTbl::Key7 key;
					if(base_entity_id) {
						MEMSZERO(key);
						key.ToEnterpriseID = base_entity_id;
						if(DT.searchForUpdate(idx, &key, spGe) && DT.data.ToEnterpriseID == base_entity_id) do {
							if(DT.data.LinkToPsnID != personID || DT.data.LinkToDlvrLocID != dlvrLocID) {
								DT.data.LinkToPsnID = personID;
								DT.data.LinkToDlvrLocID = dlvrLocID;
								THROW_DB(DT.updateRec());
								ok = 1;
							}
						} while(DT.searchForUpdate(idx, &key, spNext) && DT.data.ToEnterpriseID == base_entity_id);
						{
							MEMSZERO(vp_key0);
							vp_key0.EntityID = base_entity_id;
							if(BT.searchForUpdate(0, &vp_key0, spEq)) {
								if(BT.data.LinkPersonID != personID || BT.data.LinkDlvrLocID != dlvrLocID) {
									BT.data.LinkPersonID = personID;
									BT.data.LinkDlvrLocID = dlvrLocID;
									THROW_DB(BT.updateRec());
									ok = 1;
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

int VetisEntityCore::ResolveEntityByID(PPID entityID, VetisNamedGenericVersioningEntity & rD)
{
	int    ok = -1;
	if(entityID) {
		Entity sub_entity;
		if(GetEntity(entityID, sub_entity) > 0) {
			sub_entity.Get(rD);
			ok = 1;
		}
	}
	return ok;
}

int VetisEntityCore::Get(PPID id, VetisVetDocument & rItem)
{
	Reference * p_ref = PPRef;
	Entity entity;
	int    ok = GetEntity(id, entity);
	if(ok > 0) {
		SString temp_buf;
		SStringU temp_buf_u;
		VetisDocumentTbl::Rec rec;
		rItem.EntityID = id;
		rItem.Uuid = entity.Uuid;
		//VetisDocumentTbl::Key0 k0;
		//k0.EntityID = entity.ID;
		//if(DT.search(0, &k0, spEq)) {
		if(SearchDocument(entity.ID, &rec) > 0) {
			VetisCertifiedConsignment & r_crtc = rItem.CertifiedConsignment;
			Entity sub_entity;
			DT.CopyBufTo(&rec);
			rItem.EntityID = rec.EntityID;
			rItem.Flags = rec.Flags;
			rItem.VetDForm = rec.VetisDocForm;
			rItem.VetDType = rec.VetisDocType;
			rItem.VetDStatus = rec.VetisDocStatus;
			rItem.IssueDate = rec.IssueDate;
			rItem.IssueNumber = rec.IssueNumber;
			rItem.WayBillDate = rec.WayBillDate;
			rItem.WayBillNumber = rec.WayBillNumber;
			rItem.NativeBillID = rec.LinkBillID;
			rItem.NativeBillRow = rec.LinkBillRow;
			r_crtc.Batch.DateOfProduction.FirstDate.FromInt64(rec.ManufDateFrom);
			r_crtc.Batch.DateOfProduction.SecondDate.FromInt64(rec.ManufDateTo);
			r_crtc.Batch.ExpiryDate.FirstDate.FromInt64(rec.ExpiryFrom);
			r_crtc.Batch.ExpiryDate.SecondDate.FromInt64(rec.ExpiryTo);
			r_crtc.Batch.NativeGoodsID = rec.LinkGoodsID;
			r_crtc.Batch.Volume = rec.Volume;
			r_crtc.Batch.AckVolume = rec.AckVolume;
			SETFLAG(r_crtc.Batch.Flags, r_crtc.Batch.fPartlyAccepted, rec.Flags & VetisVetDocument::fPartlyAccepted);
			SETFLAG(r_crtc.Batch.Flags, r_crtc.Batch.fRejected, rec.Flags & VetisVetDocument::fRejected);
			r_crtc.Batch.PackingAmount = rec.PackingAmount;
			r_crtc.Batch.ProductType = rec.ProductType;
			r_crtc.TransportInfo.TransportType = rec.TranspType;
			r_crtc.TransportStorageType_id = rec.TranspStrgType;
			if(rec.FromEntityID) {
				GetEntity(rec.FromEntityID, sub_entity);
				sub_entity.Get(r_crtc.Consignor.BusinessEntity);
				r_crtc.Consignor.BusinessEntity.EntityID = sub_entity.ID;
			}
			ResolveEntityByID(rec.FromEnterpriseID, r_crtc.Consignor.Enterprise);
			if(rec.ToEntityID) {
				GetEntity(rec.ToEntityID, sub_entity);
				sub_entity.Get(r_crtc.Consignee.BusinessEntity);
				r_crtc.Consignee.BusinessEntity.EntityID = sub_entity.ID;
			}
			ResolveEntityByID(rec.ToEnterpriseID, r_crtc.Consignee.Enterprise);
			ResolveEntityByID(rec.ProductID, r_crtc.Batch.Product);
			ResolveEntityByID(rec.SubProductID, r_crtc.Batch.SubProduct);
			ResolveEntityByID(rec.OrgCountryID, r_crtc.Batch.Origin.Country);
			ResolveEntityByID(rec.UOMID, r_crtc.Batch.Unit);
			if(rec.ProductItemID) {
				GetEntity(rec.ProductItemID, sub_entity);
				sub_entity.Get(r_crtc.Batch.ProductItem);
			}
			else {
				p_ref->UtrC.SearchU(TextRefIdent(PPOBJ_VETISENTITY, id, VetisEntityCore::txtprpProductItemName), temp_buf_u);
				if(temp_buf_u.Len()) {
					temp_buf_u.CopyToUtf8(temp_buf, 0);
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					r_crtc.Batch.ProductItem.Name = temp_buf;
				}
			}
			{
				struct TranspNumTabEntry {
					int    TxtIdent;
					SString & R_Text;
				};
				/*non static!*/ const TranspNumTabEntry transp_num_tab[] = {
					{ txtprpTranspVNum, r_crtc.TransportInfo.TransportNumber.VehicleNumber },
					{ txtprpTranspTNum, r_crtc.TransportInfo.TransportNumber.TrailerNumber },
					{ txtprpTranspCNum, r_crtc.TransportInfo.TransportNumber.ContainerNumber },
					{ txtprpTranspWNum, r_crtc.TransportInfo.TransportNumber.WagonNumber },
					{ txtprpTranspSNam, r_crtc.TransportInfo.TransportNumber.ShipName },
					{ txtprpTranspFNum, r_crtc.TransportInfo.TransportNumber.FlightNumber }
				};
				for(uint tti = 0; tti < SIZEOFARRAY(transp_num_tab); tti++) {
					const TranspNumTabEntry & r_tt_entry = transp_num_tab[tti];
					p_ref->UtrC.SearchU(TextRefIdent(PPOBJ_VETISENTITY, id, r_tt_entry.TxtIdent), temp_buf_u);
					if(temp_buf_u.Len()) {
						temp_buf_u.CopyToUtf8(temp_buf, 0);
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						r_tt_entry.R_Text = temp_buf;
					}
				}
			}
			{
				p_ref->UtrC.SearchU(TextRefIdent(PPOBJ_VETISENTITY, id, VetisEntityCore::txtprpGoodsCodeList), temp_buf_u);
				if(temp_buf_u.Len()) {
					temp_buf_u.CopyToUtf8(temp_buf, 0);
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					StringSet ss(';', temp_buf);
					SString left_buf, right_buf;
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						temp_buf.Divide(',', left_buf, right_buf);
						VetisProductMarks * p_new_mark = r_crtc.Batch.ProductMarkingList.CreateNewItem();
						if(p_new_mark) {
							p_new_mark->Cls = left_buf.ToLong();
							p_new_mark->Item = right_buf;
						}
					}
					//r_crtc.Batch.ProductMarkingList.setBuf(temp_buf);
				}
			}
		}
		else
			ok = -2;
	}
	return ok;
}

int VetisEntityCore::Put(PPID * pID, const S_GUID & rBusEntGuid, const S_GUID & rEnterpriseGuid,
	const VetisStockEntry & rItem, TSVector <UnresolvedEntity> * pUreList, int use_ta)
{
	int    ok = 1;
	PPID   result_id = 0;
	Reference * p_ref = PPRef;
	SString temp_buf;
	//SStringU temp_buf_u;
	PPIDArray id_list;
	VetisDocumentTbl::Rec rec;
	LocationTbl::Rec loc_rec;
	Entity entity(rItem);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SetEntity(entity, pUreList, &result_id, 0));
		{
			const VetisBatch & r_bat = rItem.Batch;
			VetisBusinessEntity bent;
			rec.EntityID = entity.ID;
			rec.Flags    = rItem.Flags;
			rec.VetisDocForm = 0;
			rec.VetisDocType = vetisdoctypSTOCK;
			rec.VetisDocStatus = vetisdocstSTOCK;
			rec.IssueDate = rItem.UpdateDate.d;
			STRNSCPY(rec.IssueNumber, rItem.EntryNumber);
			rec.ManufDateFrom = r_bat.DateOfProduction.FirstDate.ToInt64();
			rec.ManufDateTo = r_bat.DateOfProduction.SecondDate.ToInt64();
			rec.ExpiryFrom = r_bat.ExpiryDate.FirstDate.ToInt64();
			rec.ExpiryTo = r_bat.ExpiryDate.SecondDate.ToInt64();
			rec.LinkGoodsID = r_bat.NativeGoodsID;
			rec.Volume = r_bat.Volume;
			rec.PackingAmount = r_bat.PackingAmount;
			if(rItem.VetDocumentList.getCount()) {
				for(uint i = 0; i < rItem.VetDocumentList.getCount(); i++) {
					const VetisVetDocument * p_vd = rItem.VetDocumentList.at(i);
					if(p_vd && !!p_vd->Uuid) {
						Entity sub_entity;
						if(GetEntityByUuid(p_vd->Uuid, sub_entity) > 0 && sub_entity.Kind == kVetDocument)
							rec.OrgDocEntityID = sub_entity.ID;
					}
				}
			}
			{
				Entity sub_entity;
				if(GetEntityByGuid(rEnterpriseGuid, sub_entity) > 0 && sub_entity.Kind == kEnterprise) {
					rec.ToEnterpriseID = sub_entity.ID;
					if(p_ref->Ot.SearchObjectsByGuid(PPOBJ_LOCATION, PPTAG_LOC_VETIS_GUID, sub_entity.Guid, &id_list) > 0) {
						assert(id_list.getCount());
						const  PPID loc_id = id_list.get(0);
						if(PsnObj.LocObj.Fetch(loc_id, &loc_rec) > 0) {
							rec.LinkToDlvrLocID = loc_id;
							rec.Flags |= VetisVetDocument::fToMainOrg;
						}
					}
				}
			}
			{
				Entity sub_entity;
				if(GetEntityByGuid(rBusEntGuid, sub_entity) > 0 && sub_entity.Kind == kBusinessEntity)
					rec.ToEntityID = sub_entity.ID;
			}
			THROW(SetEntity(Entity(kProduct, r_bat.Product), pUreList, &rec.ProductID, 0));
			THROW(SetEntity(Entity(kSubProduct, r_bat.SubProduct), pUreList, &rec.SubProductID, 0));
			THROW(SetEntity(Entity(kProductItem, r_bat.ProductItem), pUreList, &rec.ProductItemID, 0));
			THROW(SetEntity(Entity(kCountry, r_bat.Origin.Country), pUreList, &rec.OrgCountryID, 0));
			THROW(SetEntity(Entity(kUOM, r_bat.Unit), pUreList, &rec.UOMID, 0));
			rec.ProductType = static_cast<int16>(r_bat.ProductType);
			{
				VetisDocumentTbl::Key0 k0;
				k0.EntityID = entity.ID;
				if(DT.searchForUpdate(0, &k0, spEq)) {
					if(memcmp(&DT.data, &rec, sizeof(rec)) != 0) {
						THROW_DB(DT.updateRecBuf(&rec)); // @sfu
					}
					else
						ok = -1;
				}
				else {
					THROW_DB(DT.insertRecBuf(&rec));
				}
			}
			if(!rec.ProductItemID) {
				temp_buf = r_bat.ProductItem.Name;
				if(temp_buf.NotEmptyS()) {
					int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, txtprpProductItemName), temp_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
					THROW(sr);
					if(sr > 0 && ok < 0)
						ok = 1;
				}
			}
			if(r_bat.ProductMarkingList.getCount()) {
				SString code_list_buf;
				for(uint midx = 0; midx < r_bat.ProductMarkingList.getCount(); midx++) {
					const VetisProductMarks * p_mitem = r_bat.ProductMarkingList.at(midx);
					if(p_mitem) {
						temp_buf.Z().Cat(p_mitem->Cls).Comma().Cat(p_mitem->Item);
						code_list_buf.CatDivIfNotEmpty(';', 0).Cat(temp_buf);
						if(code_list_buf.Len() > 1024)
							break;
					}
				}
				if(code_list_buf.NotEmptyS()) {
					int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, txtprpGoodsCodeList), code_list_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
					THROW(sr);
					if(sr > 0 && ok < 0)
						ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pID, result_id);
	return ok;
}

int VetisEntityCore::SetOutgoingDocInQueueFlag(PPID id, int use_ta)
{
	int    ok = 1;
	{
		VetisDocumentTbl::Key0 k0;
		VetisDocumentTbl::Rec rec;
		k0.EntityID = id;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(DT.searchForUpdate(0, &k0, spEq));
		DT.CopyBufTo(&rec);
		if(!(rec.Flags & VetisVetDocument::fInSendingQueue)) {
			rec.Flags |= VetisVetDocument::fInSendingQueue;
			THROW_DB(DT.updateRecBuf(&rec)); // @sfu
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::SetOutgoingDocApplicationIdent(PPID id, const S_GUID & rAppId, int use_ta)
{
	int    ok = 1;
	{
		VetisDocumentTbl::Key0 k0;
		VetisDocumentTbl::Rec rec;
		k0.EntityID = id;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(DT.searchForUpdate(0, &k0, spEq));
		DT.CopyBufTo(&rec);
		THROW_PP_S(rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, id);
		if(rec.AppReqId != rAppId) {
			rec.AppReqId = rAppId;
			THROW_DB(DT.updateRecBuf(&rec)); // @sfu
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::Put(PPID * pID, const VetisVetDocument & rItem, long flags, TSVector <UnresolvedEntity> * pUreList, int use_ta)
{
	int    ok = 1;
	PPID   result_id = 0;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	//SStringU temp_buf_u;
	PPIDArray by_inn_psn_list;
	PPIDArray id_list;
	VetisDocumentTbl::Rec rec;
	LocationTbl::Rec loc_rec;
	PersonTbl::Rec psn_rec;
	Entity entity(rItem);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SetEntity(entity, pUreList, &result_id, 0));
		{
			const VetisCertifiedConsignment & r_crtc = rItem.CertifiedConsignment;
			const VetisBatch & r_bat = r_crtc.Batch;
			VetisBusinessEntity bent;
			rec.EntityID = entity.ID;
			rec.Flags    = rItem.Flags;
			rec.Flags &= ~(rItem.fFromMainOrg|rItem.fToMainOrg);
			rec.VetisDocForm = rItem.VetDForm;
			rec.VetisDocType = rItem.VetDType;
			rec.VetisDocStatus = rItem.VetDStatus;
			rec.IssueDate = rItem.IssueDate;
			const VetisVetDocument::ReferencedDocument * p_waybill_ref_doc = rItem.GetWayBillRef();
			if(p_waybill_ref_doc) {
				STRNSCPY(rec.IssueNumber, p_waybill_ref_doc->IssueNumber);
				rec.WayBillDate = p_waybill_ref_doc->IssueDate;
				STRNSCPY(rec.WayBillNumber, p_waybill_ref_doc->IssueNumber);
			}
			else {
				STRNSCPY(rec.IssueNumber, rItem.IssueNumber);
				rec.WayBillDate = rItem.WayBillDate;
				STRNSCPY(rec.WayBillNumber, rItem.WayBillNumber);
			}
			rec.ManufDateFrom = r_bat.DateOfProduction.FirstDate.ToInt64();
			rec.ManufDateTo = r_bat.DateOfProduction.SecondDate.ToInt64();
			rec.ExpiryFrom = r_bat.ExpiryDate.FirstDate.ToInt64();
			rec.ExpiryTo = r_bat.ExpiryDate.SecondDate.ToInt64();
			rec.LinkGoodsID = r_bat.NativeGoodsID;
			rec.Volume = r_bat.Volume;
			rec.PackingAmount = r_bat.PackingAmount;
			BillTbl::Rec bill_rec;
			if(rItem.NativeBillID && p_bobj->Search(rItem.NativeBillID, &bill_rec) > 0) {
				rec.LinkBillID = rItem.NativeBillID;
				rec.LinkBillRow = rItem.NativeBillRow;
			}
			else if(!(rItem.Flags & rItem.fFromMainOrg) && (flags & putvdfEnableClearNativeBillLink)) {
				rec.LinkBillID = 0;
				rec.LinkBillRow = 0;
			}
			{
				Entity sub_entity(kEnterprise, r_crtc.Consignor.Enterprise);
				THROW(SetEntity(sub_entity, pUreList, &rec.FromEnterpriseID, 0));
				if(sub_entity.ID && !!sub_entity.Guid) {
					if(p_ref->Ot.SearchObjectsByGuid(PPOBJ_LOCATION, PPTAG_LOC_VETIS_GUID, sub_entity.Guid, &id_list) > 0) {
						assert(id_list.getCount());
						const  PPID loc_id = id_list.get(0);
						if(PsnObj.LocObj.Fetch(loc_id, &loc_rec) > 0) {
							rec.LinkFromDlvrLocID = loc_id;
							if(loc_rec.Type == LOCTYP_ADDRESS && loc_rec.OwnerID)
								rec.LinkFromPsnID = loc_rec.OwnerID;
							else if(loc_rec.Type == LOCTYP_WAREHOUSE)
								rec.Flags |= rItem.fFromMainOrg;
						}
					}
					else if(p_ref->Ot.SearchObjectsByGuid(PPOBJ_PERSON, PPTAG_PERSON_VETISUUID, sub_entity.Guid, &id_list) > 0) {
						assert(id_list.getCount());
						rec.LinkFromPsnID = id_list.get(0);
					}
				}
			}
			{
				Entity sub_entity(kBusinessEntity, r_crtc.Consignor.BusinessEntity);
				THROW(SetEntity(sub_entity, pUreList, &rec.FromEntityID, 0));
				if(!rec.LinkFromPsnID && !(rec.Flags & rItem.fFromMainOrg) && Get(sub_entity.ID, bent) > 0 && bent.Inn.NotEmpty()) {
					by_inn_psn_list.clear();
					PsnObj.GetListByRegNumber(PPREGT_TPID, 0, bent.Inn, id_list);
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID _id = id_list.get(i);
						if(PsnObj.Fetch(_id, &psn_rec) > 0)
							by_inn_psn_list.add(_id);
					}
					if(by_inn_psn_list.getCount() == 1)
						rec.LinkFromPsnID = by_inn_psn_list.get(0);
				}
			}
			{
				Entity sub_entity(kEnterprise, r_crtc.Consignee.Enterprise);
				THROW(SetEntity(sub_entity, pUreList, &rec.ToEnterpriseID, 0));
				if(sub_entity.ID && !!sub_entity.Guid) {
					if(p_ref->Ot.SearchObjectsByGuid(PPOBJ_LOCATION, PPTAG_LOC_VETIS_GUID, sub_entity.Guid, &id_list) > 0) {
						assert(id_list.getCount());
						const  PPID loc_id = id_list.get(0);
						if(PsnObj.LocObj.Fetch(loc_id, &loc_rec) > 0) {
							rec.LinkToDlvrLocID = loc_id;
							if(loc_rec.Type == LOCTYP_ADDRESS && loc_rec.OwnerID)
								rec.LinkToPsnID = loc_rec.OwnerID;
							else if(loc_rec.Type == LOCTYP_WAREHOUSE)
								rec.Flags |= rItem.fToMainOrg;
						}
					}
					else if(p_ref->Ot.SearchObjectsByGuid(PPOBJ_PERSON, PPTAG_PERSON_VETISUUID, sub_entity.Guid, &id_list) > 0) {
						assert(id_list.getCount());
						rec.LinkToPsnID = id_list.get(0);
					}
				}
			}
			{
				Entity sub_entity(kBusinessEntity, r_crtc.Consignee.BusinessEntity);
				THROW(SetEntity(sub_entity, pUreList, &rec.ToEntityID, 0));
				if(!rec.LinkToPsnID && !(rec.Flags & rItem.fToMainOrg) && Get(sub_entity.ID, bent) > 0 && bent.Inn.NotEmpty()) {
					by_inn_psn_list.clear();
					PsnObj.GetListByRegNumber(PPREGT_TPID, 0, bent.Inn, id_list);
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID _id = id_list.get(i);
						if(PsnObj.Fetch(_id, &psn_rec) > 0)
							by_inn_psn_list.add(_id);
					}
					if(by_inn_psn_list.getCount() == 1)
						rec.LinkToPsnID = by_inn_psn_list.get(0);
				}
			}
			THROW(SetEntity(Entity(kProduct, r_bat.Product), pUreList, &rec.ProductID, 0));
			THROW(SetEntity(Entity(kSubProduct, r_bat.SubProduct), pUreList, &rec.SubProductID, 0));
			THROW(SetEntity(Entity(kProductItem, r_bat.ProductItem), pUreList, &rec.ProductItemID, 0));
			THROW(SetEntity(Entity(kCountry, r_bat.Origin.Country), pUreList, &rec.OrgCountryID, 0));
			THROW(SetEntity(Entity(kUOM, r_bat.Unit), pUreList, &rec.UOMID, 0));
			rec.ProductType    = static_cast<int16>(r_bat.ProductType);
			rec.TranspStrgType = static_cast<int16>(r_crtc.TransportStorageType_id);
			rec.TranspType     = static_cast<int16>(r_crtc.TransportInfo.TransportType);
			{
				VetisDocumentTbl::Key0 k0;
				k0.EntityID = entity.ID;
				if(DT.searchForUpdate(0, &k0, spEq)) {
					VetisDocumentTbl::Rec ex_rec;
					DT.CopyBufTo(&ex_rec);
					if(!(rItem.Flags & rItem.fFromMainOrg) && (flags & putvdfEnableClearNativeBillLink)) { // @v11.1.8
						;
					}
					else {
						if(!rec.LinkBillID && ex_rec.LinkBillID && p_bobj->Search(ex_rec.LinkBillID, &bill_rec) > 0) {
							rec.LinkBillID = ex_rec.LinkBillID;
							rec.LinkBillRow = ex_rec.LinkBillRow;
						}
					}
					if(flags & putvdfForceUpdateOuterFields) {
						SETFLAG(rec.Flags, rItem.fPartlyAccepted, rItem.Flags & rItem.fPartlyAccepted);
						SETFLAG(rec.Flags, rItem.fRejected, rItem.Flags & rItem.fRejected);
						rec.AckVolume = r_bat.AckVolume;
					}
					else {
						SETFLAGBYSAMPLE(rec.Flags, rItem.fPartlyAccepted, ex_rec.Flags);
						SETFLAGBYSAMPLE(rec.Flags, rItem.fRejected, ex_rec.Flags);
						rec.AckVolume = ex_rec.AckVolume;
					}
					if(memcmp(&DT.data, &rec, sizeof(rec)) != 0) {
						THROW_DB(DT.updateRecBuf(&rec)); // @sfu
					}
					else
						ok = -1;
				}
				else {
					THROW_DB(DT.insertRecBuf(&rec));
				}
			}
			if(!rec.ProductItemID) {
				temp_buf = r_bat.ProductItem.Name;
				if(temp_buf.NotEmptyS()) {
					int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, txtprpProductItemName), temp_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
					THROW(sr);
					if(sr > 0 && ok < 0)
						ok = 1;
				}
			}
			{
				struct TranspNumTabEntry {
					int    TxtIdent;
					const  SString & R_Text;
				};
				/*non static!*/ const TranspNumTabEntry transp_num_tab[] = {
					{ txtprpTranspVNum, r_crtc.TransportInfo.TransportNumber.VehicleNumber },
					{ txtprpTranspTNum, r_crtc.TransportInfo.TransportNumber.TrailerNumber },
					{ txtprpTranspCNum, r_crtc.TransportInfo.TransportNumber.ContainerNumber },
					{ txtprpTranspWNum, r_crtc.TransportInfo.TransportNumber.WagonNumber },
					{ txtprpTranspSNam, r_crtc.TransportInfo.TransportNumber.ShipName },
					{ txtprpTranspFNum, r_crtc.TransportInfo.TransportNumber.FlightNumber }
				};
				for(uint tti = 0; tti < SIZEOFARRAY(transp_num_tab); tti++) {
					const TranspNumTabEntry & r_tt_entry = transp_num_tab[tti];
					temp_buf = r_tt_entry.R_Text;
					if(temp_buf.NotEmptyS()) {
						int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, r_tt_entry.TxtIdent), temp_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
						THROW(sr);
						if(sr > 0 && ok < 0)
							ok = 1;
					}
				}
			}
			if(r_bat.ProductMarkingList.getCount()) {
				SString code_list_buf;
				for(uint midx = 0; midx < r_bat.ProductMarkingList.getCount(); midx++) {
					const VetisProductMarks * p_mitem = r_bat.ProductMarkingList.at(midx);
					if(p_mitem) {
						code_list_buf.CatDivIfNotEmpty(';', 0).Cat(p_mitem->Cls).Comma().Cat(p_mitem->Item);
						if(code_list_buf.Len() > 1024)
							break;
					}
				}
				if(code_list_buf.NotEmptyS()) {
					int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, txtprpGoodsCodeList), code_list_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
					THROW(sr);
					if(sr > 0 && ok < 0)
						ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pID, result_id);
	return ok;
}

int VetisEntityCore::Get(PPID id, VetisBusinessEntity & rItem)
{
	rItem.Z();
	Reference * p_ref = PPRef;
	Entity entity;
	int    ok = GetEntity(id, entity);
	if(ok > 0) {
		SString temp_buf;
		SStringU temp_buf_u;
		VetisPersonTbl::Rec rec;
		VetisPersonTbl::Key0 k0;
		rItem.EntityID = id;
		rItem.Guid = entity.Guid;
		rItem.Uuid = entity.Uuid;
		rItem.Flags = entity.Flags;
		k0.EntityID = entity.ID;
		if(BT.search(0, &k0, spEq)) {
			Entity sub_entity;
			PiT.CopyBufTo(&rec);
			rItem.Inn = rec.INN;
			rItem.Kpp = rec.KPP;
		}
		else
			ok = -2;
	}
	return ok;
}

int VetisEntityCore::Put(PPID * pID, const VetisBusinessEntity & rItem, TSVector <UnresolvedEntity> * pUreList, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SStringU temp_buf_u;
	VetisPersonTbl::Rec rec;
	Entity entity(kBusinessEntity, rItem);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SetEntity(entity, pUreList, &rec.EntityID, 0));
		{
			rec.EnterpriseType = rItem.Type;
			STRNSCPY(rec.INN, rItem.Inn);
			STRNSCPY(rec.KPP, rItem.Kpp);
			THROW(SetEntity(Entity(kCountry, rItem.JuridicalAddress.Country), pUreList, &rec.CountryID, 0));
			THROW(SetEntity(Entity(kRegion, rItem.JuridicalAddress.Region), pUreList, &rec.RegionID, 0));
			{
				VetisPersonTbl::Key0 k0;
				k0.EntityID = entity.ID;
				if(BT.searchForUpdate(0, &k0, spEq)) {
					VetisPersonTbl::Rec ex_rec;
					BT.CopyBufTo(&ex_rec);
					if(ex_rec.INN[0] && !rec.INN[0])
						STRNSCPY(rec.INN, ex_rec.INN);
					if(ex_rec.KPP[0] && !rec.KPP[0])
						STRNSCPY(rec.KPP, ex_rec.KPP);
					if(ex_rec.LinkPersonID && !rec.LinkPersonID)
						rec.LinkPersonID = ex_rec.LinkPersonID;
					if(ex_rec.LinkDlvrLocID && !rec.LinkDlvrLocID)
						rec.LinkDlvrLocID = ex_rec.LinkDlvrLocID;
					if(memcmp(&BT.data, &rec, sizeof(rec)) != 0) {
						THROW_DB(BT.updateRecBuf(&rec)); // @sfu
					}
					else
						ok = -1;
				}
				else {
					THROW_DB(BT.insertRecBuf(&rec));
				}
			}
			{
				temp_buf = rItem.JuridicalAddress.AddressView;
				if(temp_buf.NotEmptyS()) {
					int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, PPTRPROP_RAWADDR), temp_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
					THROW(sr);
					if(sr > 0 && ok < 0)
						ok = 1;
				}
			}
		}
		THROW(tra.Commit());
		ASSIGN_PTR(pID, entity.ID);
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::Get(PPID id, VetisEnterprise & rItem)
{
	rItem.Z();
	Reference * p_ref = PPRef;
	Entity entity;
	int    ok = GetEntity(id, entity);
	if(ok > 0) {
		//rItem.Type = entity.Type;
		SString temp_buf;
		SStringU temp_buf_u;
		VetisPersonTbl::Rec rec;
		VetisPersonTbl::Key0 k0;
		//rItem.EntityID = id;
		rItem.Guid = entity.Guid;
		rItem.Uuid = entity.Uuid;
		rItem.Flags = entity.Flags;
		k0.EntityID = entity.ID;
		if(BT.search(0, &k0, spEq)) {
			PiT.CopyBufTo(&rec);
			rItem.Type = rec.EnterpriseType;
			ResolveEntityByID(rec.CountryID, rItem.Address.Country);
			ResolveEntityByID(rec.RegionID, rItem.Address.Region);
			p_ref->UtrC.SearchU(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, PPTRPROP_RAWADDR), temp_buf_u);
			if(temp_buf_u.Len()) {
				temp_buf_u.CopyToUtf8(rItem.Address.AddressView, 0);
				rItem.Address.AddressView.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
		else
			ok = -2;
	}
	return ok;
}

int VetisEntityCore::Put(PPID * pID, const VetisEnterprise & rItem, TSVector <UnresolvedEntity> * pUreList, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	//SStringU temp_buf_u;
	VetisPersonTbl::Rec rec;
	Entity entity(kEnterprise, rItem);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SetEntity(entity, pUreList, &rec.EntityID, 0));
		{
			rec.EnterpriseType = rItem.Type;
			THROW(SetEntity(Entity(kCountry, rItem.Address.Country), pUreList, &rec.CountryID, 0));
			THROW(SetEntity(Entity(kRegion, rItem.Address.Region), pUreList, &rec.RegionID, 0));
			{
				VetisPersonTbl::Key0 k0;
				k0.EntityID = entity.ID;
				if(BT.searchForUpdate(0, &k0, spEq)) {
					VetisPersonTbl::Rec ex_rec;
					BT.CopyBufTo(&ex_rec);
					if(ex_rec.INN[0] && !rec.INN[0])
						STRNSCPY(rec.INN, ex_rec.INN);
					if(ex_rec.KPP[0] && !rec.KPP[0])
						STRNSCPY(rec.KPP, ex_rec.KPP);
					if(ex_rec.LinkPersonID && !rec.LinkPersonID)
						rec.LinkPersonID = ex_rec.LinkPersonID;
					if(ex_rec.LinkDlvrLocID && !rec.LinkDlvrLocID)
						rec.LinkDlvrLocID = ex_rec.LinkDlvrLocID;
					if(memcmp(&BT.data, &rec, sizeof(rec)) != 0) {
						THROW_DB(BT.updateRecBuf(&rec)); // @sfu
					}
					else
						ok = -1;
				}
				else {
					THROW_DB(BT.insertRecBuf(&rec));
				}
			}
			if((temp_buf = rItem.Address.AddressView).NotEmptyS()) {
				int sr = p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_VETISENTITY, entity.ID, PPTRPROP_RAWADDR), temp_buf.ToLower().Transf(CTRANSF_INNER_TO_UTF8), 0);
				THROW(sr);
				if(sr > 0 && ok < 0)
					ok = 1;
			}
		}
		THROW(tra.Commit());
		ASSIGN_PTR(pID, entity.ID);
	}
	CATCHZOK
	return ok;
}

int VetisEntityCore::Get(PPID id, VetisProductItem & rItem)
{
	Reference * p_ref = PPRef;
	Entity entity;
	int    ok = GetEntity(id, entity);
	if(ok > 0) {
		SString temp_buf;
		SStringU temp_buf_u;
		VetisProductTbl::Rec rec;
		VetisProductTbl::Key0 k0;
		rItem.EntityID = id;
		rItem.Guid = entity.Guid;
		rItem.Uuid = entity.Uuid;
		rItem.Flags = entity.Flags;
		k0.EntityID = entity.ID;
		if(PiT.search(0, &k0, spEq)) {
			PiT.CopyBufTo(&rec);
			rItem.NativeGoodsID = rec.LinkGoodsID;
			rItem.ProductType = rec.ProductType;
			rItem.GlobalID = rec.GTIN;
			ResolveEntityByID(rec.ProductID, rItem.Product);
			ResolveEntityByID(rec.SubProductID, rItem.SubProduct);
			if(rec.GostTRef && p_ref->TrT.SearchSelfRef(rec.GostTRef, temp_buf_u) > 0) {
				temp_buf_u.CopyToUtf8(temp_buf, 1);
				temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				rItem.Gost = temp_buf;
			}
		}
		else
			ok = -2;
	}
	return ok;
}

int VetisEntityCore::Put(PPID * pID, int kind, const VetisProductItem & rItem, TSVector <UnresolvedEntity> * pUreList, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SStringU temp_buf_u;
	VetisProductTbl::Rec rec;
	Entity entity(kind, rItem);
	assert(oneof3(kind, kProductItem, kProduct, kSubProduct));
	THROW_PP(oneof3(kind, kProductItem, kProduct, kSubProduct), PPERR_INVPARAM);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SetEntity(entity, pUreList, &rec.EntityID, 0));
		if(kind == kProductItem) {
			temp_buf = rItem.Gost;
			if(temp_buf.NotEmptyS()) {
				temp_buf_u.CopyFromUtf8(temp_buf.Trim(124).ToLower().Transf(CTRANSF_INNER_TO_UTF8));
				THROW(p_ref->TrT.GetSelfRefText(temp_buf_u, &rec.GostTRef, 0));
			}
		}
		else if(kind == kProduct) {
		}
		else if(kind == kSubProduct) {
		}
		rec.ProductType = rItem.ProductType;
		if(kind == kProductItem) {
			rec.LinkGoodsID = rItem.NativeGoodsID;
			THROW(SetEntity(Entity(kProduct, rItem), pUreList, &rec.ProductID, 0));
			THROW(SetEntity(Entity(kSubProduct, rItem), pUreList, &rec.SubProductID, 0));
			STRNSCPY(rec.GTIN, rItem.GlobalID);
			{
				VetisProductTbl::Key0 k0;
				k0.EntityID = entity.ID;
				if(PiT.searchForUpdate(0, &k0, spEq)) {
					if(memcmp(&PiT.data, &rec, sizeof(rec)) != 0) {
						THROW_DB(PiT.updateRecBuf(&rec)); // @sfu
					}
					else
						ok = -1;
				}
				else {
					THROW_DB(PiT.insertRecBuf(&rec));
				}
			}
		}
		THROW(tra.Commit());
		ASSIGN_PTR(pID, entity.ID);
	}
	CATCHZOK
	return ok;
}

/*int VetisEntityCore::Get(PPID id, VetisProduct & rItem)
{
	int    ok = -1;
	return ok;
}

int VetisEntityCore::Get(PPID id, VetisSubProduct & rItem)
{
	int    ok = -1;
	return ok;
}

int VetisEntityCore::RecToItem(const VetisProductTbl::Rec & rRec, VetisProductItem & rItem)
{
	int    ok = -1;
	return ok;
}*/

class PPVetisInterface {
public:
	enum {
		extssApplicationId = 1,
		extssServiceId,
		extssApiKey,
		extssUser,
		extssPassword,
		extssQInitiator,   // логин инициатора запросов
		extssDoctInitiator // логин ветеринарного врача
	};
	enum {
		stInited = 0x0001
	};
	struct Param : public PPExtStrContainer {
		Param(PPID mainOrgID, PPID locID, long flags);
		Param & Z();
		enum {
			fTestContour   = 0x0001,
			fSkipLocInitialization = 0x0002,
			fLogTalk       = 0x0004  // @v11.1.6 Если установлен, то пишется журнал общения с сервисами VETIS (vetis-talk.log)
		};
		long   Flags;
		PPID   MainOrgID;
		PPID   LocID;
		int    Timeout;    // Таймаут ожидания ответа на application-request
		S_GUID IssuerUUID; // GUID предприятия, генерурующей запросы
		S_GUID EntUUID; // GUID подразделения предприятия (склада)
	};

	struct OutcomingEntry { // @flat
		OutcomingEntry() : PrepEntityID(0), QueueN(0), State(stWaiting), OrgDocEntityID(0), LinkBillID(0), LinkBillRow(0), LinkGoodsID(0), P_RegRuleList(0)
		{
		}
		SString & DebugOutput(SString & rBuf) const
		{
			rBuf.Cat(PrepEntityID).Space().Cat(OrgDocEntityID).Space().
				Cat(LinkBillID).Colon().Cat(LinkBillRow).Space().Cat(LinkGoodsID).Space().
				CatChar('[').Cat(QueueN).Slash().Cat(State).Slash().Cat(AppId, S_GUID::fmtIDL).CatChar(']');
			return rBuf;
		}
		enum {
			stWaiting = 0,
			stSent    = 1,
			stReply   = 2
		};
		PPID  PrepEntityID; // Идент документа vetisdocstOUTGOING_PREPARING
		uint  QueueN;       // Номер очередности для отправки (документы, ссылающиеся на один OrgVetDocUuid отправляются отдельными порциями)
		uint  State;        // Состояние элемента
		PPID  OrgDocEntityID; // Идент оригинального сертификата
		PPID  LinkBillID;
		int   LinkBillRow;
		PPID  LinkGoodsID;
		S_GUID AppId;         // ApplicationId для получения результата запроса
		const TSCollection <VetisRouteSectionR13nRules> * P_RegRuleList; // @notowned
	};
	class OutcomingList : public TSVector <PPVetisInterface::OutcomingEntry> {
	public:
		OutcomingList() : LastQueueN(0)
		{
		}
		uint   GetReplyCandidateCount(uint queueN) const
		{
			uint   result = 0;
			for(uint i = 0; i < getCount(); i++) {
				const PPVetisInterface::OutcomingEntry & r_test_entry = at(i);
				if(r_test_entry.QueueN == queueN && !r_test_entry.AppId.IsZero())
					result++;
			}
			return result;
		}
		SString & DebugOutput(SString & rBuf) const
		{
			for(uint i = 0; i < getCount(); i++) {
				at(i).DebugOutput(rBuf).CR();
			}
			return rBuf;
		}
		int    InsertEntry(const PPVetisInterface::OutcomingEntry & rEntry)
		{
			int    ok = -1;
			if(rEntry.OrgDocEntityID) {
				uint target_qidx = 0;
				if(getCount() == 0) {
					assert(LastQueueN == 0);
					target_qidx = 1;
				}
				else {
					assert(LastQueueN > 0);
					if(!lsearch(&rEntry.PrepEntityID, 0, CMPF_LONG)) {
						uint last_qidx_with_orgdoc = 0;
						for(uint qidx = 1; qidx <= LastQueueN; qidx++) {
							for(uint i = 0; i < getCount(); i++) {
								const PPVetisInterface::OutcomingEntry & r_test_entry = at(i);
								if(r_test_entry.QueueN == qidx && r_test_entry.OrgDocEntityID == rEntry.OrgDocEntityID) {
									last_qidx_with_orgdoc = qidx;
									break;
								}
							}
						}
						target_qidx = last_qidx_with_orgdoc ? (last_qidx_with_orgdoc+1) : LastQueueN;
					}
				}
				if(target_qidx > 0) {
					PPVetisInterface::OutcomingEntry new_entry = rEntry;
					new_entry.QueueN = target_qidx;
					new_entry.State = PPVetisInterface::OutcomingEntry::stWaiting;
					SETMAX(LastQueueN, target_qidx);
					insert(&new_entry);
					ok = 1;
				}
			}
			return ok;
		}
		uint   GetLastQueue() const { return LastQueueN; }
	private:
		uint   LastQueueN;
	};

	static int FASTCALL SetupParam(Param & rP);
	static int GoodsDateToString(const SUniTime & rUt, SString & rBuf);
	static int LockProcess(int prcId, bool unlock);
	static const SString & FASTCALL EncText(const SString & rS) // @v12.4.6
	{
		SString & r_temp_buf = SLS.AcquireRvlStr();
		(r_temp_buf = rS).ReplaceChar('\x07', ' ');
		XMLReplaceSpecSymb(r_temp_buf, "&<>\'");
		return r_temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	}
	explicit PPVetisInterface(PPLogger * pLogger);
	~PPVetisInterface();
	int    Init(const Param & rP);
	int    GetStockEntryList(uint startOffset, uint count, VetisApplicationBlock & rReply);
	int    GetStockEntryChangesList(const STimeChunk & rPeriod, uint startOffset, uint count, VetisApplicationBlock & rReply);
	//
	// Операция GetVetDocumentListOperation предназначена для получения всех ветеринарно-сопроводительных документов предприятия.
	//   При этом список ВСД может быть отфильтрован по следующим критериям:
	//     Тип ВСД: входящий ВСД; исходящий ВСД; производственный ВСД; транспортный ВСД; возвратный ВСД.
	//     Статус ВСД: оформлен; погашен; аннулирован.
	//   На вход системы передаются следующие сведения:
	//     информация о пользователе - инициаторе запроса;
	//     информация о предприятии и хозяйствующем субъекте, где осуществляется поиск ВСД;
	//     параметры, по которым будет отфильтрован список ВСД.
	//   Результатом выполнения данной операции является:
	//     пользователю передаются сведения о запрашиваемых ВСД.
	//   Запрашиваться пользователем могут только те ВСД, где ветеринарное управление инициатор запроса обслуживает предприятия.
	//
	int    GetVetDocumentList(const STimeChunk & rPeriod, uint startOffset, uint count, VetisApplicationBlock & rReply);
	int    GetVetDocumentChangesList(const STimeChunk & rPeriod, uint startOffset, uint count, VetisApplicationBlock & rReply);
	int    GetVetDocumentByUuid(const S_GUID & rUuid, VetisApplicationBlock & rReply);
	int    GetStockEntryByUuid(const S_GUID & rUuid, VetisApplicationBlock & rReply);
	//
	// Операция IncomingOperation предназначена для оформления в системе Меркурий входящей партии.
	//   На вход системы, в зависимости от сценария, передаются следующие сведения:
	//     информация об электронном ВСД, по которому продукция поступила на предприятие (для сценария №1);
	//     информация о бумажном ВСД, по которому продукция поступила на предприятие (для сценария №2);
	//     фактические сведения о принимаемой партии;
	//     акт несоответствия, в случае если фактические сведения о продукции отличаются от сведений, указанных в ВСД;
	//     возвратный ВСД, в случае если на весь объем или на его часть оформляется возврат.
	//   Результатом выполнения данной операции является:
	//     оформление электронного ВСД, в случае, если продукция поступила по бумажному входящему документу (для сценария №2);
	//     гашение электронного ВСД (для сценария №1);
	//     добавление одной записи в журнал входящей продукции;
	//     возвратный ВСД (формируется в случае, если принимается не весь объем продукции);
	//     акт несоответствия (формируется в случае, если фактические сведения о продукции не совпадают с указанными в ВСД).
	//
	int    ProcessIncomingConsignment(const S_GUID & rDocUuid, VetisApplicationBlock & rReply);
	//
	// Операция PrepareOutgoingConsignmentOperation предназначена для оформления в системе Меркурий транспортной партии.
	//   На вход системы передаются следующие сведения:
	//     информация об одной или нескольких партиях продукции, из которых будет сформирована транспортная партия;
	//     сведения о получателе транспортной партии;
	//     сведения о транспортном средстве и маршруте его следования;
	//     дополнительные сведения необходимые для оформления ветеринарно-сопроводительного документа (ВСД), например,
	//     результат ветеринарно-санитарной экспертизы, сведения о ТТН, особые отметки и т.д.
	//   Результатом выполнения данной операции является:
	//     списание объема с одной или нескольких записей журнала продукции, которые были указаны в заявке;
	//     гашение производственной сертификата, если был указан весь объем по данной записи журнала вырабатываемой продукции;
	//     для каждого наименования продукции указанного в транспортной партии, система Меркурий формирует ветеринарно-сопроводительный документ (ВСД).
	//
	int    PrepareOutgoingConsignment(PPID docEntityID, const TSCollection <VetisRouteSectionR13nRules> * pRegionRules, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply);
	int    PrepareOutgoingConsignment2(OutcomingEntry & rEntry, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply);
	int    QueryOutgoingConsignmentResult(OutcomingEntry & rEntry, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply);
	int    ResolveDiscrepancy(PPID docEntityID, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply);
	int    RegisterProduction(PPID docEntityID, const PPIDArray & rExpenseDocEntityList, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply);
	int    WriteOffIncomeCert(PPID docEntityID, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply);
	int    WithdrawVetDocument(const S_GUID & rDocUuid, VetisApplicationBlock & rReply);
	int    ModifyEnterprise(VetisRegisterModificationType modType, const VetisEnterprise & rEnt, VetisApplicationBlock & rReply);
	int    ModifyActivityLocations(VetisRegisterModificationType modType, const VetisBusinessEntity & rBe, const VetisEnterprise & rEnt, VetisApplicationBlock & rReply);
	//
	// Операция RegisterProductionOperation предназначена для оформления в системе Меркурий производственной партии, как завершённой, так и незавершённой.
	//   На вход системы передаются следующие сведения:
	//     информация о сырье, из которого партия или несколько партий были произведены;
	//     информация о произведенной партии или нескольких партиях продукции;
	//     информация о хозяйствующем субъекте - собственнике сырья и выпускаемой продукции и информация о площадке, на которой продукция выпускается;
	//     идентификатор производственной операции (для незавершённого производства);
	//     номер производственной партии;
	//     флаг завершения производственной транзакции.
	//   Результатом выполнения данной операции является:
	//     списание объема с одной или нескольких записей журнала продукции, указанного в качестве сырья;
	//     добавление одной или нескольких записей в журнал вырабатываемой продукции о партии продукции, которая была произведена или присоединение к
	//       существующей записи вырабатываемой продукции, если оформляется незаверёшнное производство;
	//     для каждой записи журнала вырабатываемой продукции, которая была добавлена при выполнении операции, система Меркурий формирует
	//       ветеринарно-сопроводительный документ (ВСД) или происходит увеличение объёма выпущенной продукции в уже оформленном ветеринарном документе
	//       (для незавершённого производства).
	//
	int    RegisterProductionOperation(); // registerProductionOperationRequest
	//
	// Операция AddBussinessEntityUser предназначена для регистрации новых пользователей в системе Меркурий или
	//   привязки существующих пользователей к хозяйствующему субъекту.
	//   При выполнении операции на вход системы передаются следующие сведения:
	//     информация о пользователе - инициаторе запроса;
	//     имя пользователя или уникальный идентификатор, если существующий пользователь привязывается к ХС;
	//     данные пользователя (ФИО, паспортные данные, гражданство, адрес электронной почты), если регистрируется новый пользователь;
	//     при регистрации нового пользователя опционально могут быть переданы дополнительные данные пользователя (телефон, рабочий телефон, рабочий адрес электронной почты и т.д.), которые будут сохранены в системе "Ветис.Паспорт";
	//     при регистрации нового пользователя опционально может быть передан список прав пользователя, но назначены эти права будут после активации созданного пользователя.
	//   Результатом выполнения данной операции является:
	//     регистрация нового пользователя или привязка существующего пользователя к хозяйствующему субъекту.
	//
	int    AddBusinessEntityUser(); // addBusinessEntityUserRequest
	int    GetRussianEnterpriseList(uint offs, uint count, VetisApplicationBlock & rReply);
	int    GetBusinessEntityList(uint offs, uint count, VetisApplicationBlock & rReply);
	int    GetProductItemList(uint offs, uint count, VetisApplicationBlock & rReply);
	int    GetProductChangesList(uint offs, uint count, LDATE since, VetisApplicationBlock & rReply);
	int    GetSubProductChangesList(uint offs, uint count, LDATE since, VetisApplicationBlock & rReply);
	int    GetPurposeList(uint offs, uint count, VetisApplicationBlock & rReply);
	int    GetUnitList(uint offs, uint count, VetisApplicationBlock & rReply);
	int    GetCountryList(VetisApplicationBlock & rReply);
	int    GetRegionList(S_GUID & rCountryGuid, VetisApplicationBlock & rReply);
	int    GetLocalityList(S_GUID & rRegionGuid, VetisApplicationBlock & rReply);
	int    ModifyProducerStockListOperation(VetisRegisterModificationType modType, VetisProductItem & rPi, VetisApplicationBlock & rReply);
	int    CheckShipmentRegionalizationOperation(const UuidArray & rSubProductList, const TSCollection <VetisShipmentRoutePoint> & rRoutePointList, VetisApplicationBlock & rReply); // @v11.0.0
	enum {
		qtProductItemByGuid = 1,
		qtProductItemByUuid,
		qtProductByGuid,
		qtProductByUuid,
		qtSubProductByGuid,
		qtSubProductByUuid,
		qtBusinessEntityByGuid,
		qtBusinessEntityByUuid,
		qtEnterpriseByGuid,
		qtEnterpriseByUuid
	};
	int    GetEntityQuery2(int queryType, const char * pQueryParam, VetisApplicationBlock & rReply);
	int    ProcessUnresolvedEntityList(const TSVector <VetisEntityCore::UnresolvedEntity> & rList);
	int    SetupOutgoingEntries(PPID locID, const DateRange & rPeriod);
	int    InitOutgoingEntry(PPID docEntityID, const TSCollection <VetisRouteSectionR13nRules> * pRegRuleList, OutcomingList & rList);

	VetisEntityCore PeC;
private:
	class VetisSubmitRequestBlock : public SXmlWriter {
	public:
		VetisSubmitRequestBlock();
		~VetisSubmitRequestBlock();
		SString & GetReplyString(SString & rBuf) const;
	private:
		SXml::WDoc D;
	};
	int    PrepareApplicationBlockForReq(VetisApplicationBlock & rBlk);
	int    SubmitRequest(VetisApplicationBlock & rAppBlk, VetisApplicationBlock & rResult);
	int    ReceiveResult(const S_GUID & rAppId, OutcomingEntry * pOcEntry, VetisApplicationBlock & rResult, int once);
	int    SendSOAP(const char * pUrl, const char * pAction, const SString & rPack, SString & rReply);
	int    MakeAuthField(SString & rBuf);
	int    ParseReply(const SString & rReply, VetisApplicationBlock & rResult);
	int    ParseError(const xmlNode * pNode, VetisErrorEntry & rResult);
	int    ParseFault(const xmlNode * pParentNode, VetisFault & rResult);
	int    ParseDocument(const xmlNode * pParentNode, VetisDocument & rResult);
	int    ParseVetDocument(const xmlNode * pParentNode, VetisVetDocument * pResult);
	int    ParseCertifiedConsignment(const xmlNode * pParentNode, VetisCertifiedConsignment & rResult);
	int    ParseLocality(const xmlNode * pParentNode, VetisAddress::VetisLocality & rResult);
	int    ParseCountry(const xmlNode * pParentNode, VetisCountry & rResult);
	int    ParseRegion(const xmlNode * pParentNode, VetisAddressObjectView & rResult);
	int    ParseAddress(const xmlNode * pParentNode, VetisAddress & rResult);
	int    ParseLocation(const xmlNode * pParentNode, VetisLocation & rResult);
	int    ParseBusinessMember(const xmlNode * pParentNode, VetisBusinessMember & rResult);
	int    ParseEnterprise(const xmlNode * pParentNode, VetisEnterprise * pResult);
	int    ParseProducer(const xmlNode * pParentNode, VetisProducer & rResult);
	int    ParseBusinessEntity(const xmlNode * pParentNode, VetisBusinessEntity & rResult);
	int    ParseGenericVersioningEntity(const xmlNode * pParentNode, VetisGenericVersioningEntity & rResult);
	int    ParseNamedGenericVersioningEntity(const xmlNode * pParentNode, VetisNamedGenericVersioningEntity & rResult);
	int    ParseCertifiedBatch(const xmlNode * pParentNode, VetisCertifiedBatch & rResult);
	int    ParseBatch(const xmlNode * pParentNode, VetisBatch & rResult);
	int    ParseProduct(const xmlNode * pParentNode, VetisProduct & rResult);
	int    ParseSubProduct(const xmlNode * pParentNode, VetisSubProduct & rResult);
	int    ParseProductItem(const xmlNode * pParentNode, VetisProductItem * pResult);
	int    ParsePackingType(const xmlNode * pParentNode, VetisPackingType & rResult);
	int    ParsePackage(const xmlNode * pParentNode, VetisPackage & rResult);
	int    ParseComplexDate(const xmlNode * pParentNode, SUniTime & rResult);
	int    ParseGoodsDate(const xmlNode * pParentNode, VetisGoodsDate & rResult);
	int    ParseUnit(const xmlNode * pParentNode, VetisUnit & rResult);
	int    ParseTransportInfo(const xmlNode * pParentNode, VetisTransportInfo & rResult);
	void   ParseListResult(const xmlNode * pNode, VetisApplicationBlock::ReplyListValues & rResult);
	void   PutListAttributes(SXml::WNode & rN, long count, long offset, long total);
	void   PutListOptionsParam(xmlTextWriter * pWriter, uint offs, uint count);
	int    PutGoodsDate(xmlTextWriter * pWriter, const char * pScopeXmlTag, const char * pDtNs, const SUniTime & rUt);
	int    ParseStockEntry(const xmlNode * pParentNode, VetisStockEntry * pResult);
	int    Helper_PutOutgoingBillList(PPIDArray & rBillList, const long putBillRowFlags);
	int    SearchLastStockEntry(PPID docEntityID, VetisVetDocument & rDocEntity, PPID & rStockEntryEntityID, S_GUID & rStockEntryGUID, S_GUID & rStockEntryUUID, double & rRest);
	int    PrepareOutgoingTransportData(PPID billID, VetisPrepareOutgoingConsignmentRequest & rReq);
	int    SearchDupOutgoingConsignment(const VetisVetDocument & rRec);
	//
	// Descr: Специализированная структура для оптимизации серии вызовов PutBillRow для одного и того же документа.
	//
	struct PutBillRowBlock {
		PutBillRowBlock();
		PPID   BillID;
		PPID   PersonID;
		PPID   DlvrLocID;
		S_GUID PersonGuid;
		S_GUID DlvrLocGuid;
		PPID   ManufIncomeDocEntityID; // При обработке документа производства сначала формируется запись выхода а затем записи
			// расхода, которые получают ссылку на запись выхода (DepDocEntityID). Таким образом, можно сформировать записи для документов
			// производства, которые содержат одну и только одну исходящую позицию, подлежащую сертификации.
	};
	enum {
		pbrfDiscrepancy = 0x0001,
		pbrfManuf       = 0x0002, //
		pbrfManufInc    = 0x0004  // Если флаг установлен, то формируется запись для исходящей строки документа производства
	};
	static int MakeOutgoingBillList(PPID locID, const DateRange & rPeriod, const PPIDArray & rOpList, long flags, PPIDArray & rBillList);
	int    PutBillRow(const PPBillPacket & rBp, uint rowIdx, long flags, PutBillRowBlock & rPbrBlk, int use_ta);
	double CalcVolumeByGoodsQtty(PPID goodsID, double quantity);
	int    LogMessage(const char * pPrefix, const SString & rMsg);
	int    LogFaults(const VetisApplicationBlock & rAb, const OutcomingEntry * pOcEntry);

	long   State;
	PPID   DlvrLocToTranspTagID; // Тег сопоставляющий адрес транспортному стредству
	PPID   PersonToTranspTagID;  // @v11.6.12 Тег сопоставляющий контрагента транспортному стредству
	SString LogFileName;
	SString LastMsg;
	Param   P;
	int64   LastLocalTransactionId;
	PPLogger * P_Logger;
	PPObjGoods GObj;
};

PPVetisInterface::Param::Param(PPID mainOrgID, PPID locID, long flags) : Flags(flags), MainOrgID(mainOrgID), LocID(locID), Timeout(0)
{
}

PPVetisInterface::Param & PPVetisInterface::Param::Z()
{
	SetBuffer(0);
	IssuerUUID.Z();
	EntUUID.Z();
	return *this;
}

PPVetisInterface::PutBillRowBlock::PutBillRowBlock() : BillID(0), PersonID(0), DlvrLocID(0), ManufIncomeDocEntityID(0)
{
}

int PPVetisInterface::LogMessage(const char * pPrefix, const SString & rMsg)
{
	int    ok = 1;
	if(P.Flags & P.fLogTalk) {
		SString temp_buf;
		PPGetFilePath(PPPATH_LOG, PPFILNAM_VETISTALK_LOG, temp_buf);
		SFile  f_log(temp_buf, SFile::mAppend);
		if(f_log.IsValid()) {
			if(!isempty(pPrefix)) {
				temp_buf.Z().CatCurDateTime(DATF_YMD|DATF_CENTURY, TIMF_HMS|TIMF_MSEC).Space().Cat(pPrefix).CR();
				f_log.WriteLine(temp_buf);
			}
			f_log.WriteLine(rMsg);
			f_log.WriteLine(0);
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

/*static*/int FASTCALL PPVetisInterface::SetupParam(Param & rP)
{
	rP.Z();
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	PPAlbatrossConfig acfg;
	THROW(DS.FetchAlbatrosConfig(&acfg) > 0);
	SETFLAG(rP.Flags, rP.fTestContour, acfg.Hdr.Flags & acfg.Hdr.fVetisTestContour);
	SETIFZ(rP.MainOrgID, GetMainOrgID());
	SETIFZ(rP.LocID, LConfig.Location);
	acfg.GetExtStrData(ALBATROSEXSTR_VETISUSER, temp_buf);
	THROW_PP(temp_buf.NotEmptyS(), PPERR_VETISUSERUNDEF);
	rP.PutExtStrData(extssUser, temp_buf);
	acfg.GetPassword(ALBATROSEXSTR_VETISPASSW, temp_buf);
	THROW_PP(temp_buf.NotEmptyS(), PPERR_VETISUSERUNDEF);
	rP.PutExtStrData(extssPassword, temp_buf);
	acfg.GetExtStrData(ALBATROSEXSTR_VETISAPIKEY, temp_buf);
	THROW_PP(temp_buf.NotEmptyS(), PPERR_VETISAPIKEYUNDEF);
	rP.PutExtStrData(extssApiKey, temp_buf);
	acfg.GetExtStrData(ALBATROSEXSTR_VETISDOCTUSER, temp_buf);
	rP.PutExtStrData(extssDoctInitiator, temp_buf);
	if(acfg.Hdr.VetisTimeout > 0)
		rP.Timeout = acfg.Hdr.VetisTimeout;
	{
		ObjTagItem tag_item;
		if(p_ref->Ot.GetTag(PPOBJ_PERSON, rP.MainOrgID, PPTAG_PERSON_VETISUUID, &tag_item) > 0) {
			tag_item.GetGuid(&rP.IssuerUUID);
		}
		GetPersonName(rP.MainOrgID, temp_buf);
		THROW_PP_S(rP.IssuerUUID, PPERR_VETISBUSENTGUIDUNDEF, temp_buf);
		if(!(rP.Flags & rP.fSkipLocInitialization)) {
			if(p_ref->Ot.GetTag(PPOBJ_LOCATION, rP.LocID, PPTAG_LOC_VETIS_GUID, &tag_item) > 0) {
				tag_item.GetGuid(&rP.EntUUID);
			}
			GetLocationName(rP.LocID, temp_buf);
			THROW_PP_S(rP.EntUUID, PPERR_VETISLOCGUIDUNDEF, temp_buf);
		}
		if(p_ref->Ot.GetTag(PPOBJ_PERSON, rP.MainOrgID, PPTAG_PERSON_VETISUSER, &tag_item) > 0) {
			tag_item.GetStr(temp_buf);
			rP.PutExtStrData(extssQInitiator, temp_buf);
		}
	}
	// @v11.1.6 {
	{
		PPIniFile ini_file;
		int iv = 0;
		if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_VETIS_LOGTALK, &iv) > 0 && iv != 0)
			rP.Flags |= Param::fLogTalk;
		else
			rP.Flags &= ~Param::fLogTalk;
	}
	// } @v11.1.6 
	CATCHZOK
	return ok;
}

PPVetisInterface::PPVetisInterface(PPLogger * pLogger) : State(0), LastLocalTransactionId(0), P_Logger(pLogger), P(0, 0, 0), DlvrLocToTranspTagID(0), PersonToTranspTagID(0)
{
	PPGetFilePath(PPPATH_LOG, "vetis.log", LogFileName);
}

PPVetisInterface::~PPVetisInterface()
{
}

int PPVetisInterface::Init(const Param & rP)
{
	int    ok = 1;
	P = rP;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	DlvrLocToTranspTagID = 0;
	{
		for(SEnum en = tag_obj.Enum(0); !DlvrLocToTranspTagID && en.Next(&tag_rec) > 0;) {
			if(tag_rec.ObjTypeID == PPOBJ_LOCATION && tag_rec.TagDataType == OTTYP_OBJLINK && tag_rec.TagEnumID == PPOBJ_TRANSPORT)
				DlvrLocToTranspTagID = tag_rec.ID;
		}
	}
	// @v11.6.12 {
	PersonToTranspTagID = 0;
	{
		for(SEnum en = tag_obj.Enum(0); !PersonToTranspTagID && en.Next(&tag_rec) > 0;) {
			if(tag_rec.ObjTypeID == PPOBJ_PERSON && tag_rec.TagDataType == OTTYP_OBJLINK && tag_rec.TagEnumID == PPOBJ_TRANSPORT)
				PersonToTranspTagID = tag_rec.ID;
		}
	}
	// } @v11.6.12
	State |= stInited;
	return ok;
}

int PPVetisInterface::MakeAuthField(SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	SString pwd;
	SString login;
	SString temp_buf;
	P.GetExtStrData(extssUser, login);
	P.GetExtStrData(extssPassword, pwd);
	login.Colon().Cat(pwd).Transf(CTRANSF_INNER_TO_UTF8);
	temp_buf.Z().EncodeMime64(login.cptr(), login.Len());
	rBuf.Cat("Basic").Space().Cat(temp_buf);
	pwd.Obfuscate();
	login.Obfuscate();
	return ok;
}

int PPVetisInterface::SendSOAP(const char * pUrl, const char * pAction, const SString & rPack, SString & rReply)
{
	//static const char * P_VetisSoapUrl = "https://api.vetrf.ru/platform/services/ApplicationManagementService"; // product
	//static const char * P_VetisSoapUrl = "https://api2.vetrf.ru:8002/platform/services/ApplicationManagementService"; // test
	rReply.Z();
	int    ok = -1;
	SString temp_buf;
	ScURL  c;
	const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
	const char * p_url = pUrl ? pUrl : InetUrl::MkHttps(p_domain, "platform/services/ApplicationManagementService");
	InetUrl url(p_url);
	p_url = 0; // возможно, указывает на револьверный буфер - лучше обнулить во избежании использования //
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	{
		SFileFormat::GetMime(SFileFormat::Xml, temp_buf);
		temp_buf.CatDiv(';', 2).CatEq("charset", "utf-8");
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentLen, temp_buf.Z().Cat(rPack.Len()));
		THROW(MakeAuthField(temp_buf));
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAuthorization, temp_buf);
		if(!isempty(pAction)) {
			SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrSoapAction, temp_buf.Z().CatQStr(pAction));
		}
	}
	LogMessage("query", rPack);
	THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, rPack, &wr_stream));
	{
		SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
		if(p_ack_buf) {
			rReply.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
			LogMessage("reply", rReply);
		}
	}
	CATCHZOK
	return ok;
}

PPVetisInterface::VetisSubmitRequestBlock::VetisSubmitRequestBlock() : SXmlWriter(), D((xmlTextWriter *)*this, cpUTF8)
{
}

PPVetisInterface::VetisSubmitRequestBlock::~VetisSubmitRequestBlock()
{
}

SString & PPVetisInterface::VetisSubmitRequestBlock::GetReplyString(SString & rBuf) const
{
	const xmlBuffer * p_xb = static_cast<const xmlBuffer *>(*this);
	return rBuf.CopyFromN(reinterpret_cast<const char *>(p_xb->content), p_xb->use);
}

int PPVetisInterface::ParseError(const xmlNode * pNode, VetisErrorEntry & rResult)
{
	int    ok = -1;
	SString temp_buf;
	rResult.Z();
	if(SXml::GetContentByName(pNode, "error", temp_buf)) {
		rResult.Item = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		if(SXml::GetAttrib(pNode, "code", temp_buf))
			rResult.Code = temp_buf;
		ok = 1;
	}
	return ok;
}

int PPVetisInterface::ParseFault(const xmlNode * pParentNode, VetisFault & rResult)
{
	int    ok = 1;
	if(pParentNode) {
		SString temp_buf;
		VetisErrorEntry err;
		VetisErrorEntry single_err;
		for(const xmlNode * p_i2 = pParentNode->children; p_i2; p_i2 = p_i2->next) {
			if(SXml::GetContentByName(p_i2, "message", temp_buf))
				rResult.Message = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_i2, "errorCode", temp_buf))
				single_err.Code = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_i2, "reason", temp_buf))
				single_err.Item = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_i2, "location", temp_buf)) {
			}
			else if(ParseError(p_i2, err) > 0) {
				VetisErrorEntry * p_err = rResult.ErrList.CreateNewItem();
				THROW_SL(p_err);
				*p_err = err;
			}
		}
		if(!single_err.IsEmpty()) {
			VetisErrorEntry * p_err = rResult.ErrList.CreateNewItem();
			THROW_SL(p_err);
			*p_err = single_err;
		}
	}
	CATCHZOK
	return ok;
}

static const SIntToSymbTabEntry VetisAppStatus_SymbTab[] = {
	{ VetisApplicationBlock::appstAccepted, "Accepted" },
	{ VetisApplicationBlock::appstRejected, "Rejected" },
	{ VetisApplicationBlock::appstCompleted, "Completed" },
	{ VetisApplicationBlock::appstInProcess, "InProcess" },
	{ VetisApplicationBlock::appstInProcess, "IN_PROCESS" },
};

static const SIntToSymbTabEntry VetisVetDocFormat_SymbTab[] = {
	{ VetisVetDocument::formCERTCU1, "CERTCU1" },
	{ VetisVetDocument::formLIC1, "LIC1" },
	{ VetisVetDocument::formCERTCU2, "CERTCU2" },
	{ VetisVetDocument::formLIC2, "LIC2" },
	{ VetisVetDocument::formCERTCU3, "CERTCU3" },
	{ VetisVetDocument::formLIC3, "LIC3" },
	{ VetisVetDocument::formNOTE4, "NOTE4" },
	{ VetisVetDocument::formCERT5I, "CERT5I" },
	{ VetisVetDocument::formCERT61, "CERT61" },
	{ VetisVetDocument::formCERT62, "CERT62" },
	{ VetisVetDocument::formCERT63, "CERT63" },
	{ VetisVetDocument::formPRODUCTIVE, "PRODUCTIVE" }
};

static const SIntToSymbTabEntry VetisVetDocType_SymbTab[] = {
	{ vetisdoctypTRANSPORT, "TRANSPORT" },
	{ vetisdoctypPRODUCTIVE, "PRODUCTIVE" },
	{ vetisdoctypRETURNABLE, "RETURNABLE" },
	{ vetisdoctypINCOMING, "INCOMING" },
	{ vetisdoctypOUTGOING, "OUTGOING" },
	{ vetisdoctypSTOCK, "STOCK" }
};

static const SIntToSymbTabEntry VetisVetDocStatus_SymbTab[] = {
	{ vetisdocstCREATED, "CREATED" },
	{ vetisdocstCONFIRMED, "CONFIRMED" },
	{ vetisdocstWITHDRAWN, "WITHDRAWN" },
	{ vetisdocstUTILIZED, "UTILIZED" },
	{ vetisdocstFINALIZED, "FINALIZED" },
	{ vetisdocstOUTGOING_PREPARING, "OUTGOING_PREPARING" },
	{ vetisdocstSTOCK, "STOCK" }
};

static const SIntToSymbTabEntry VetisTranspStorageType_SymbTab[] = {
	{ vtstUNDEF, "UNDEFINED" },
	{ vtstFROZEN, "FROZEN" },
	{ vtstCHILLED, "CHILLED" },
	{ vtstCOOLED, "COOLED" },
	{ vtstVENTILATED, "VENTILATED" }
};

static const SIntToSymbTabEntry VetisProductMarkingClass_SymbTab[] = {
	{ vpmcUNDEFINED, "UNDEFINED" },
	{ vpmcBN, "BN" },
	{ vpmcSSCC, "SSCC" },
	{ vpmcEAN8, "EAN8" },
	{ vpmcEAN13, "EAN13" },
	{ vpmcEAN128, "EAN128" },
	{ vpmcBUNDLE, "BUNDLE" }
};

static const SIntToSymbTabEntry VetisRegisterModificationType_SymbTab[] = {
	{ vetisrmtCREATE, "CREATE" },
	{ vetisrmtFIND_OR_CREATE, "FIND_OR_CREATE" },
	{ vetisrmtUPDATE, "UPDATE" },
	{ vetisrmtDELETE, "DELETE" },
	{ vetisrmtMERGE, "MERGE" },
	{ vetisrmtATTACH, "ATTACH" },
	{ vetisrmtSPLIT, "SPLIT" },
	{ vetisrmtFORK, "FORK" }
};

static const SIntToSymbTabEntry VetAuthCargoExpertized_SymbTab[] = {
	{ VetisVetDocument::VeterinaryAuthentication::ceUnkn,        "UNKNOWN" },
	{ VetisVetDocument::VeterinaryAuthentication::ceUndef,       "UNDEFINED" },
	{ VetisVetDocument::VeterinaryAuthentication::cePositive,    "POSITIVE" },
	{ VetisVetDocument::VeterinaryAuthentication::ceNegative,    "NEGATIVE" },
	{ VetisVetDocument::VeterinaryAuthentication::ceUnfulfilled, "UNFULFILLED" },
	{ VetisVetDocument::VeterinaryAuthentication::ceVseRaw,      "VSERAW" },
	{ VetisVetDocument::VeterinaryAuthentication::ceVseFull,     "VSEFULL" },
};

int PPVetisInterface::ParseGenericVersioningEntity(const xmlNode * pParentNode, VetisGenericVersioningEntity & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "uuid", temp_buf))
			rResult.Uuid.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "guid", temp_buf))
			rResult.Guid.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "active", temp_buf)) {
			SETFLAG(rResult.Flags, rResult.fActive, temp_buf.IsEqiAscii("true"));
		}
		else if(SXml::GetContentByName(p_a, "last", temp_buf)) {
			SETFLAG(rResult.Flags, rResult.fLast, temp_buf.IsEqiAscii("true"));
		}
		else if(SXml::GetContentByName(p_a, "status", temp_buf))
			rResult.Status = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "createDate", temp_buf))
			strtodatetime(temp_buf, &rResult.CreateDate, DATF_ISO8601, TIMF_HMS);
		else if(SXml::GetContentByName(p_a, "updateDate", temp_buf))
			strtodatetime(temp_buf, &rResult.UpdateDate, DATF_ISO8601, TIMF_HMS);
		else if(SXml::GetContentByName(p_a, "previous", temp_buf))
			rResult.Previous.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "next", temp_buf))
			rResult.Next.FromStr(temp_buf);
	}
	return ok;
}

int PPVetisInterface::ParseNamedGenericVersioningEntity(const xmlNode * pParentNode, VetisNamedGenericVersioningEntity & rResult)
{
	int    ok = 1;
	SString temp_buf;
	ParseGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "name", temp_buf))
			rResult.Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
	}
	return ok;
}

int PPVetisInterface::ParsePackingType(const xmlNode * pParentNode, VetisPackingType & rResult)
{
	int    ok = 1;
	SString temp_buf;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "globalID", temp_buf))
			rResult.GlobalID = temp_buf;
	}
	return ok;
}

int PPVetisInterface::ParsePackage(const xmlNode * pParentNode, VetisPackage & rResult)
{
	int    ok = 1;
	SString temp_buf;
	SString attr_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "level", temp_buf))
			rResult.Level = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "packingType", temp_buf))
			ParsePackingType(p_a, rResult.PackingType);
		else if(SXml::GetContentByName(p_a, "quantity", temp_buf))
			rResult.Quantity = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "productMarks", temp_buf)) {
			if(temp_buf.NotEmptyS() && temp_buf.IsLegalUtf8()) { // @v11.2.4 (&& temp_buf.IsLegalUtf8())
				// @v11.3.4 {
				SStringU temp_buf_u;
				temp_buf_u.CopyFromUtf8(temp_buf); 
				temp_buf_u.ReplaceChar(0x2022, L'.');
				temp_buf_u.CopyToUtf8(temp_buf, 1);
				// } @v11.3.4 
				VetisProductMarks * p_new_mark = rResult.ProductMarks.CreateNewItem();
				p_new_mark->Item = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				p_new_mark->Cls = vpmcUNDEFINED;
				if(SXml::GetAttrib(p_a, "class", temp_buf))
					p_new_mark->Cls = SIntToSymbTab_GetId(VetisProductMarkingClass_SymbTab, SIZEOFARRAY(VetisProductMarkingClass_SymbTab), temp_buf);
			}
		}
	}
	return ok;
}

int PPVetisInterface::ParseLocality(const xmlNode * pParentNode, VetisAddress::VetisLocality & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "regionGUID", temp_buf))
			rResult.RegionGUID.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "districtGUID", temp_buf))
			rResult.DistrictGUID.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "cityGUID", temp_buf))
			rResult.CityGUID.FromStr(temp_buf);
	}
	return ok;
}

int PPVetisInterface::ParseCountry(const xmlNode * pParentNode, VetisCountry & rResult)
{
	int    ok = 1;
	SString temp_buf;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "fullName", temp_buf))
			rResult.FullName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "englishName", temp_buf))
			rResult.EnglishName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "code", temp_buf))
			rResult.Code = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "code3", temp_buf))
			rResult.Code3 = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
	}
	return ok;
}

int PPVetisInterface::ParseRegion(const xmlNode * pParentNode, VetisAddressObjectView & rResult)
{
	int    ok = 1;
	SString temp_buf;
	rResult.Flags &= ~rResult.fHasStreets;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "englishName", temp_buf))
			rResult.EnglishName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "view", temp_buf))
			rResult.View = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "regionCode", temp_buf))
			rResult.RegionCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "type", temp_buf))
			rResult.Type = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "countryGuid", temp_buf))
			rResult.CountryGUID.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "hasStreets", temp_buf))
			SETFLAG(rResult.Flags, rResult.fHasStreets, temp_buf.IsEqiAscii("true"));
	}
	return ok;
}

int PPVetisInterface::ParseAddress(const xmlNode * pParentNode, VetisAddress & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::IsName(p_a, "country")) {
		}
		else if(SXml::IsName(p_a, "region")) {
		}
		else if(SXml::GetContentByName(p_a, "district", temp_buf)) {
		}
		else if(SXml::GetContentByName(p_a, "locality", temp_buf)) {
			ParseLocality(p_a, rResult.Locality);
		}
		else if(SXml::GetContentByName(p_a, "subLocality", temp_buf)) {
			ParseLocality(p_a, rResult.SubLocality);
		}
		else if(SXml::GetContentByName(p_a, "street", temp_buf)) {
		}
		else if(SXml::GetContentByName(p_a, "addressView", temp_buf))
			rResult.AddressView = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
	}
	return ok;
}

int PPVetisInterface::ParseLocation(const xmlNode * pParentNode, VetisLocation & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "name", temp_buf))
			rResult.Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::IsName(p_a, "address"))
			ParseAddress(p_a, rResult.Address);
	}
	return ok;
}

int PPVetisInterface::ParseEnterprise(const xmlNode * pParentNode, VetisEnterprise * pResult)
{
	int    ok = 1;
	SString temp_buf;
	THROW_MEM(pResult);
	ParseNamedGenericVersioningEntity(pParentNode, *pResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "type", temp_buf)) {
		}
		else if(SXml::IsName(p_a, "address")) {
			ParseAddress(p_a, pResult->Address);
		}
		else if(SXml::IsName(p_a, "activityList")) {
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ParseProducer(const xmlNode * pParentNode, VetisProducer & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::IsName(p_a, "enterprise")) {
			ParseEnterprise(p_a, &rResult);
		}
		else if(SXml::GetContentByName(p_a, "role", temp_buf)) {
			if(temp_buf.IsEqiAscii("PRODUCER"))
				rResult.Role = rResult.rolePRODUCER;
			else if(temp_buf.IsEqiAscii("UNKNOWN"))
				rResult.Role = rResult.roleUNKNOWN;
		}
	}
	return ok;
}

int PPVetisInterface::ParseBusinessEntity(const xmlNode * pParentNode, VetisBusinessEntity & rResult)
{
	int    ok = 1;
	SString temp_buf;
	SString firm_name;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "fio", temp_buf)) {
			rResult.Fio = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::GetContentByName(p_a, "firmName", temp_buf))
			firm_name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "inn", temp_buf))
			rResult.Inn = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "kpp", temp_buf))
			rResult.Kpp = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "passport", temp_buf))
			rResult.Passport = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::IsName(p_a, "country")) {
		}
		else if(SXml::IsName(p_a, "region")) {
		}
		else if(SXml::IsName(p_a, "actualRegion")) {
		}
		else if(SXml::IsName(p_a, "juridicalAddress")) {
		}
		else if(SXml::IsName(p_a, "actualAddress")) {
		}
		else if(SXml::IsName(p_a, "activityLocation")) {
		}
	}
	if(rResult.Name.IsEmpty()) {
		if(firm_name.NotEmptyS())
			rResult.Name = firm_name;
		else if(rResult.Fio.NotEmpty())
			rResult.Name = rResult.Fio;
	}
	return ok;
}

int PPVetisInterface::ParseBusinessMember(const xmlNode * pParentNode, VetisBusinessMember & rResult)
{
	int    ok = 1;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::IsName(p_a, "businessEntity"))
			ParseBusinessEntity(p_a, rResult.BusinessEntity);
		else if(SXml::IsName(p_a, "enterprise"))
			ParseEnterprise(p_a, &rResult.Enterprise);
	}
	return ok;
}

int PPVetisInterface::ParseProduct(const xmlNode * pParentNode, VetisProduct & rResult)
{
	int    ok = 1;
	SString temp_buf;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "productType", temp_buf))
			rResult.ProductType = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "code", temp_buf))
			rResult.Code = temp_buf;
	}
	return ok;
}

int PPVetisInterface::ParseSubProduct(const xmlNode * pParentNode, VetisSubProduct & rResult)
{
	int    ok = 1;
	SString temp_buf;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "code", temp_buf))
			rResult.Code = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "productGuid", temp_buf))
			rResult.ProductGuid.FromStr(temp_buf);
	}
	return ok;
}

int PPVetisInterface::ParseProductItem(const xmlNode * pParentNode, VetisProductItem * pResult)
{
	int    ok = 1;
	SString temp_buf;
	THROW_MEM(pResult);
	ParseNamedGenericVersioningEntity(pParentNode, *pResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "correspondsToGost", temp_buf)) {
			SETFLAG(pResult->Flags, VetisProductItem::fCorrespondsToGost, temp_buf.IsEqiAscii("true"));
		}
		else if(SXml::GetContentByName(p_a, "productType", temp_buf))
			pResult->ProductType = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "gost", temp_buf))
			pResult->Gost = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "product", temp_buf)) {
			THROW(ParseProduct(p_a, pResult->Product));
		}
		else if(SXml::GetContentByName(p_a, "subProduct", temp_buf)) {
			THROW(ParseSubProduct(p_a, pResult->SubProduct));
		}
		else if(SXml::GetContentByName(p_a, "producer", temp_buf)) {
			THROW(ParseBusinessEntity(p_a, pResult->Producer));
		}
		else if(SXml::GetContentByName(p_a, "producing", temp_buf)) {
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::IsName(p_p, "location")) {
					ParseEnterprise(p_p, pResult->Producing.CreateNewItem());
				}
			}
		}
		else if(SXml::GetContentByName(p_a, "globalID", temp_buf)) {
			STokenRecognizer tr;
			SNaturalTokenArray nta;
			tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta.Z(), 0);
			if(nta.Has(SNTOK_EAN13) || nta.Has(SNTOK_EAN8) || nta.Has(SNTOK_UPCE) || nta.Has(SNTOK_UPCA)) {
				pResult->GlobalID = temp_buf;
			}
		}
		else if(SXml::IsName(p_a, "packaging")) { // @v11.4.8
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::IsName(p_p, "packagingType")) {
					ParsePackingType(p_p, pResult->Packaging.PackagingType);
				}
				else if(SXml::GetContentByName(p_p, "quantity", temp_buf)) {
					pResult->Packaging.Quantity = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_p, "volume", temp_buf)) {
					pResult->Packaging.Volume = temp_buf.ToReal_Plain();
				}
				else if(SXml::IsName(p_p, "unit")) {
					THROW(ParseUnit(p_p, pResult->Packaging.Unit));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ParseUnit(const xmlNode * pParentNode, VetisUnit & rResult)
{
	int    ok = 1;
	SString temp_buf;
	ParseNamedGenericVersioningEntity(pParentNode, rResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "commonUnitGuid", temp_buf)) {
			rResult.CommonUnitGuid.FromStr(temp_buf);
		}
		else if(SXml::GetContentByName(p_a, "factor", temp_buf)) {
			rResult.Factor = temp_buf.ToReal();
		}
	}
	return ok;
}

int PPVetisInterface::ParseComplexDate(const xmlNode * pParentNode, SUniTime & rResult)
{
	rResult.Z();
	int    ok = 1;
	int    year = 0;
	int    mon = 0;
	int    day = 0;
	int    hour = -1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "year", temp_buf))
			year = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "month", temp_buf))
			mon = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "day", temp_buf))
			day = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_a, "hour", temp_buf))
			hour = temp_buf.ToLong();
	}
	if(year > 0) {
		if(mon >= 1 && mon <= 12) {
			if(day >= 1 && mon <= 31) {
				if(hour >= 0 && hour <= 24) {
					LDATETIME dtm;
					dtm.d.encode(day, mon, year);
					dtm.t.encode(hour, 0, 0, 0);
					rResult.Set(dtm, SUniTime::indHr);
				}
				else
					rResult.Set(encodedate(day, mon, year));
			}
			else
				rResult.SetMonth(year, mon);
		}
		else
			rResult.SetYear(year);
	}
	return ok;
}

int PPVetisInterface::ParseGoodsDate(const xmlNode * pParentNode, VetisGoodsDate & rResult)
{
	int    ok = 1;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::IsName(p_a, "firstDate"))
			ParseComplexDate(p_a, rResult.FirstDate);
		else if(SXml::IsName(p_a, "secondDate"))
			ParseComplexDate(p_a, rResult.SecondDate);
	}
	return ok;
}

int PPVetisInterface::ParseBatch(const xmlNode * pParentNode, VetisBatch & rResult)
{
	int    ok = 1;
	SString temp_buf;
	SString attr_buf;
	STokenRecognizer tr;
	SNaturalTokenArray nta;
	rResult.Flags &= ~rResult.fPerishable;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "productType", temp_buf))
			rResult.ProductType = temp_buf.ToLong();
		else if(SXml::IsName(p_a, "product"))
			ParseProduct(p_a, rResult.Product);
		else if(SXml::IsName(p_a, "subProduct"))
			ParseSubProduct(p_a, rResult.SubProduct);
		else if(SXml::IsName(p_a, "productItem"))
			ParseProductItem(p_a, &rResult.ProductItem);
		else if(SXml::GetContentByName(p_a, "volume", temp_buf))
			rResult.Volume = temp_buf.ToReal();
		else if(SXml::GetContentByName(p_a, "packingAmount", temp_buf))
			rResult.PackingAmount = temp_buf.ToLong();
		else if(SXml::IsName(p_a, "unit"))
			ParseUnit(p_a, rResult.Unit);
		else if(SXml::IsName(p_a, "dateOfProduction"))
			ParseGoodsDate(p_a, rResult.DateOfProduction);
		else if(SXml::IsName(p_a, "expiryDate"))
			ParseGoodsDate(p_a, rResult.ExpiryDate);
		else if(SXml::GetContentByName(p_a, "perishable", temp_buf)) {
			if(temp_buf.IsEqiAscii("true"))
				rResult.Flags |= rResult.fPerishable;
		}
		else if(SXml::GetContentByName(p_a, "lowGradeCargo", temp_buf)) {
			if(temp_buf.IsEqiAscii("true"))
				rResult.Flags |= rResult.fLowGradeCargo;
		}
		else if(SXml::GetContentByName(p_a, "batchID", temp_buf)) {
			//rResult.BatchID = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			rResult.BatchIdList.add(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
		}
		else if(SXml::IsName(p_a, "countryOfOrigin"))
			ParseCountry(p_a, rResult.Origin.Country);
		else if(SXml::IsName(p_a, "origin")) { // BatchOrigin
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::IsName(p_p, "country")) {
					ParseCountry(p_p, rResult.Origin.Country);
				}
				else if(SXml::IsName(p_p, "producer")) {
					VetisProducer * p_new_p = rResult.Origin.Producer.CreateNewItem();
					THROW_SL(p_new_p);
					ParseProducer(p_p, *p_new_p);
				}
			}
		}
		else if(SXml::IsName(p_a, "packageList")) {
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::IsName(p_p, "package")) {
					VetisPackage * p_new_package = rResult.PackageList.CreateNewItem();
					THROW_SL(p_new_package);
					ParsePackage(p_p, *p_new_package);
				}
			}
		}
		else if(SXml::IsName(p_a, "packingList")) {
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::IsName(p_p, "packingForm")) {
					VetisNamedGenericVersioningEntity * p_new_pf = rResult.PackingList.CreateNewItem();
					THROW_SL(p_new_pf);
					ParseNamedGenericVersioningEntity(p_p, *p_new_pf);
				}
			}
		}
		else if(SXml::IsName(p_a, "producerList")) {
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::IsName(p_p, "producer")) {
					VetisProducer * p_new_p = rResult.Origin.Producer.CreateNewItem();
					THROW_SL(p_new_p);
					ParseProducer(p_p, *p_new_p);
				}
			}
		}
		else if(SXml::IsName(p_a, "productMarkingList")) {
			/*
				<ns2:productMarkingList>
					<ns2:productMarking>4607084550559</ns2:productMarking>
					<ns2:productMarking class="BN">X167</ns2:productMarking>
				</ns2:productMarkingList>
			*/
			for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
				if(SXml::GetContentByName(p_p, "productMarking", temp_buf)) {
					if(temp_buf.NotEmptyS()) {
						VetisProductMarks * p_new_mark = rResult.ProductMarkingList.CreateNewItem();
						p_new_mark->Item = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						p_new_mark->Cls = vpmcUNDEFINED;
						if(SXml::GetAttrib(p_a, "class", temp_buf)) {
							p_new_mark->Cls = SIntToSymbTab_GetId(VetisProductMarkingClass_SymbTab, SIZEOFARRAY(VetisProductMarkingClass_SymbTab), temp_buf);
						}
					}
					/*{
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta.Z(), 0);
						if(nta.Has(SNTOK_EAN13) || nta.Has(SNTOK_EAN8) || nta.Has(SNTOK_UPCE) || nta.Has(SNTOK_UPCA)) {
							rResult.ProductMarkingList.add(temp_buf);
						}
					}*/
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ParseTransportInfo(const xmlNode * pParentNode, VetisTransportInfo & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "transportType", temp_buf)) {
			rResult.TransportType = temp_buf.ToLong();
		}
		else if(SXml::IsName(p_a, "transportNumber")) {
			for(const xmlNode * p_tn = p_a->children; p_tn; p_tn = p_tn->next) {
				if(SXml::GetContentByName(p_tn, "vehicleNumber", temp_buf))
					rResult.TransportNumber.VehicleNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER).Strip();
				else if(SXml::GetContentByName(p_tn, "trailerNumber", temp_buf))
					rResult.TransportNumber.TrailerNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER).Strip();
				else if(SXml::GetContentByName(p_tn, "containerNumber", temp_buf))
					rResult.TransportNumber.ContainerNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER).Strip();
				else if(SXml::GetContentByName(p_tn, "wagonNumber", temp_buf))
					rResult.TransportNumber.WagonNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER).Strip();
				else if(SXml::GetContentByName(p_tn, "flightNumber", temp_buf))
					rResult.TransportNumber.FlightNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER).Strip();
				else if(SXml::GetContentByName(p_tn, "shipName", temp_buf))
					rResult.TransportNumber.ShipName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER).Strip();
			}
		}
	}
	return ok;
}

int PPVetisInterface::ParseCertifiedBatch(const xmlNode * pParentNode, VetisCertifiedBatch & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::IsName(p_a, "producer"))
			ParseBusinessMember(p_a, rResult.Producer);
		else if(SXml::IsName(p_a, "batch"))
			ParseBatch(p_a, rResult);
	}
	return ok;
}

int PPVetisInterface::ParseCertifiedConsignment(const xmlNode * pParentNode, VetisCertifiedConsignment & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		int    r = 1;
		if(SXml::IsName(p_a, "consignor"))
			r = ParseBusinessMember(p_a, rResult.Consignor);
		else if(SXml::IsName(p_a, "consignee"))
			r = ParseBusinessMember(p_a, rResult.Consignee);
		else if(SXml::IsName(p_a, "broker"))
			r = ParseBusinessEntity(p_a, rResult.Broker);
		else if(SXml::IsName(p_a, "batch"))
			r = ParseBatch(p_a, rResult.Batch);
		else if(SXml::IsName(p_a, "transportInfo"))
			r = ParseTransportInfo(p_a, rResult.TransportInfo);
		else if(SXml::GetContentByName(p_a, "transportStorageType", temp_buf))
			rResult.TransportStorageType_id = SIntToSymbTab_GetId(VetisTranspStorageType_SymbTab, SIZEOFARRAY(VetisTranspStorageType_SymbTab), temp_buf);
		else if(SXml::IsName(p_a, "cargoReloadingPointList")) {
			for(const xmlNode * p_b = p_a ? p_a->children : 0; p_b; p_b = p_b->next) {
				if(SXml::IsName(p_b, "cargoReloadingPoint")) {
					VetisCargoReloadingPoint * p_new_point = rResult.CargoReloadingPointList.CreateNewItem();
					THROW_SL(p_new_point);
					for(const xmlNode * p_c = p_b ? p_b->children : 0; p_c; p_c = p_c->next) {
						if(SXml::GetContentByName(p_c, "name", temp_buf)) {
							p_new_point->Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						}
						else if(SXml::IsName(p_c, "nextTransport")) {
							THROW(ParseTransportInfo(p_c, p_new_point->NextTransport));
							// @v11.1.11 { (может случиться, что NextTransport заявлен, но номера не определены - тогда наследуем из основной записи)
							if(p_new_point->NextTransport.TransportType == rResult.TransportInfo.TransportType) {
								if(p_new_point->NextTransport.TransportNumber.IsEmpty())
									p_new_point->NextTransport.TransportNumber = rResult.TransportInfo.TransportNumber;
							}
							// } @v11.1.11 
						}
					}
				}
			}
		}
		else if(SXml::IsName(p_a, "shipmentRoute")) {
			for(const xmlNode * p_b = p_a ? p_a->children : 0; p_b; p_b = p_b->next) {
				if(SXml::IsName(p_b, "routePoint")) {
					VetisShipmentRoutePoint * p_new_point = rResult.RoutePointList.CreateNewItem();
					if(p_new_point) {
						for(const xmlNode * p_c = p_b ? p_b->children : 0; p_c; p_c = p_c->next) {
							if(SXml::GetContentByName(p_c, "uuid", temp_buf))
								p_new_point->Uuid.FromStr(temp_buf);
							else if(SXml::GetContentByName(p_c, "sqnId", temp_buf))
								p_new_point->SqnId = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							else if(SXml::GetContentByName(p_c, "transshipment", temp_buf)) {
								SETFLAG(p_new_point->Flags, VetisShipmentRoutePoint::fTransShipment, temp_buf.IsEqiAscii("true"));
							}
							else if(SXml::GetContentByName(p_c, "nextTransport", temp_buf)) {
								if(SETIFZ(p_new_point->P_NextTransport, new VetisTransportInfo)) {
									THROW(ParseTransportInfo(p_c, *p_new_point->P_NextTransport));
									// @v11.1.12 { (может случиться, что NextTransport заявлен, но номера не определены - тогда наследуем из основной записи)
									if(p_new_point->P_NextTransport->TransportType == rResult.TransportInfo.TransportType) {
										if(p_new_point->P_NextTransport->TransportNumber.IsEmpty())
											p_new_point->P_NextTransport->TransportNumber = rResult.TransportInfo.TransportNumber;
									}
									// } @v11.1.12 
								}
							}
							else if(SXml::IsName(p_c, "location")) {
								if(SETIFZ(p_new_point->P_Location, new VetisLocation)) {
									THROW(ParseLocation(p_c, *p_new_point->P_Location));
								}
							}
							else if(SXml::IsName(p_c, "enterprise")) {
								if(SETIFZ(p_new_point->P_Enterprise, new VetisEnterprise)) {
									THROW(ParseEnterprise(p_c, p_new_point->P_Enterprise));
								}
							}
						}
					}
				}
			}
		}
		THROW(r);
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ParseDocument(const xmlNode * pParentNode, VetisDocument & rResult)
{
	int    ok = 1;
	SString temp_buf;
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "uuid", temp_buf))
			rResult.Uuid.FromStr(temp_buf);
		else if(SXml::GetContentByName(p_a, "name", temp_buf))
			rResult.Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "form", temp_buf) || SXml::GetContentByName(p_a, "vetDForm", temp_buf))
			rResult.DocumentForm = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "issueDate", temp_buf))
			rResult.IssueDate = strtodate_(temp_buf, DATF_ISO8601);
		else if(SXml::GetContentByName(p_a, "issueNumber", temp_buf))
			rResult.IssueNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "issueSeries", temp_buf))
			rResult.IssueSeries = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::IsName(p_a, "purpose")) {
		}
		else if(SXml::IsName(p_a, "confirmedBy")) {
		}
	}
	return ok;
}

int PPVetisInterface::ParseStockEntry(const xmlNode * pParentNode, VetisStockEntry * pResult)
{
	int    ok = 1;
	SString temp_buf;
	THROW_MEM(pResult);
	ParseGenericVersioningEntity(pParentNode, *pResult);
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "entryNumber", temp_buf)) { pResult->EntryNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER); }
		else if(SXml::IsName(p_a, "batch")) { THROW(ParseBatch(p_a, pResult->Batch)); }
		else if(SXml::IsName(p_a, "vetDocument")) { THROW(ParseVetDocument(p_a, pResult->VetDocumentList.CreateNewItem())); }
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ParseVetDocument(const xmlNode * pParentNode, VetisVetDocument * pResult)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	THROW_MEM(pResult);
	VetisCertifiedConsignment & r_crtc = pResult->CertifiedConsignment;
	THROW(ParseDocument(pParentNode, *pResult));
	THROW(ParseCertifiedConsignment(pParentNode, r_crtc));
	FOREACHXMLCHILD(pParentNode, p_a) {
		if(SXml::GetContentByName(p_a, "form", temp_buf) || SXml::GetContentByName(p_a, "vetDForm", temp_buf))
			pResult->VetDForm = SIntToSymbTab_GetId(VetisVetDocFormat_SymbTab, SIZEOFARRAY(VetisVetDocFormat_SymbTab), temp_buf);
		else if(SXml::GetContentByName(p_a, "type", temp_buf) || SXml::GetContentByName(p_a, "vetDType", temp_buf))
			pResult->VetDType = SIntToSymbTab_GetId(VetisVetDocType_SymbTab, SIZEOFARRAY(VetisVetDocType_SymbTab), temp_buf);
		else if(SXml::GetContentByName(p_a, "status", temp_buf) || SXml::GetContentByName(p_a, "vetDStatus", temp_buf))
			pResult->VetDStatus = SIntToSymbTab_GetId(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), temp_buf);
		else if(SXml::GetContentByName(p_a, "cargoInspected", temp_buf)) {
			SETFLAG(pResult->Flags, VetisVetDocument::fCargoInspected, temp_buf.IsEqiAscii("true"));
		}
		else if(SXml::GetContentByName(p_a, "cargoExpertized", temp_buf)) {
			pResult->Authentication.CargoExpertized = SIntToSymbTab_GetId(VetAuthCargoExpertized_SymbTab, SIZEOFARRAY(VetAuthCargoExpertized_SymbTab), temp_buf);
			SETFLAG(pResult->Flags, VetisVetDocument::fCargoExpertized, temp_buf.IsEqiAscii("true"));
		}
		else if(SXml::GetContentByName(p_a, "lastUpdateDate", temp_buf))
			pResult->LastUpdateDate.Set(strtodate_(temp_buf, DATF_ISO8601), ZEROTIME);
		else if(SXml::GetContentByName(p_a, "waybillSeries", temp_buf))
			pResult->WayBillSeries = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "waybillNumber", temp_buf))
			pResult->WayBillNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		else if(SXml::GetContentByName(p_a, "waybillDate", temp_buf))
			pResult->WayBillDate = strtodate_(temp_buf, DATF_ISO8601);
		else if(SXml::GetContentByName(p_a, "waybillType", temp_buf))
			pResult->WayBillType = temp_buf.ToLong();
		else if(SXml::IsName(p_a, "locationProsperity")) {
		}
		else if(SXml::IsName(p_a, "certifiedConsignment")) {
			ParseCertifiedConsignment(p_a, r_crtc);
		}
		else if(SXml::IsName(p_a, "certifiedBatch")) { // ver2
			THROW_SL(SETIFZ(pResult->P_CertifiedBatch, new VetisCertifiedBatch));
			ParseCertifiedBatch(p_a, *pResult->P_CertifiedBatch);
		}
		else if(SXml::IsName(p_a, "referencedDocument")) { // ver2
			VetisVetDocument::ReferencedDocument * p_rd = pResult->ReferencedDocumentList.CreateNewItem();
			if(p_rd) {
				for(const xmlNode * p_c = p_a ? p_a->children : 0; p_c; p_c = p_c->next) {
					if(SXml::GetContentByName(p_c, "issueSeries", temp_buf))           { p_rd->IssueSeries = temp_buf.Transf(CTRANSF_UTF8_TO_INNER); }
					else if(SXml::GetContentByName(p_c, "issueNumber", temp_buf))      { p_rd->IssueNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER); }
					else if(SXml::GetContentByName(p_c, "issueDate", temp_buf)) { p_rd->IssueDate = strtodate_(temp_buf, DATF_ISO8601); }
					else if(SXml::GetContentByName(p_c, "type", temp_buf))             { p_rd->DocumentType = temp_buf.ToLong(); }
					else if(SXml::GetContentByName(p_c, "relationshipType", temp_buf)) { p_rd->RelationshipType = temp_buf.ToLong(); }
				}
			}
		}
	}
	{
		PPObjBill * p_bobj = BillObj;
		PPIDArray lot_list;
		if(p_ref->Ot.SearchObjectsByGuid(PPOBJ_LOT, PPTAG_LOT_VETIS_UUID, pResult->Uuid, &lot_list) > 0) {
			ReceiptTbl::Rec lot_rec;
			TransferTbl::Rec trfr_rec;
			for(uint i = 0; i < lot_list.getCount(); i++) {
				const  PPID lot_id = lot_list.get(i);
				if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
					DateIter di;
					if(p_bobj->trfr->EnumByLot(lot_id, &di, &trfr_rec) > 0 && trfr_rec.Flags & PPTFR_RECEIPT) {
						r_crtc.Batch.NativeGoodsID = labs(trfr_rec.GoodsID);
						pResult->NativeLotID = lot_id;
						pResult->NativeBillID = trfr_rec.BillID;
						pResult->NativeBillRow = trfr_rec.RByBill;
					}
				}
			}
		}
	}
	if(!r_crtc.Batch.NativeGoodsID) {
		BarcodeTbl::Rec bc_rec;
		if(r_crtc.Batch.ProductMarkingList.getCount()) {
			PPIDArray link_goods_cadidate_list;
			for(uint midx = 0; midx < r_crtc.Batch.ProductMarkingList.getCount(); midx++) {
				const VetisProductMarks * p_mitem = r_crtc.Batch.ProductMarkingList.at(midx);
				if(p_mitem && GObj.SearchByBarcode(p_mitem->Item, &bc_rec, 0, 1) > 0)
					link_goods_cadidate_list.add(bc_rec.GoodsID);
			}
			link_goods_cadidate_list.sortAndUndup();
			if(link_goods_cadidate_list.getCount() == 1) {
				r_crtc.Batch.NativeGoodsID = link_goods_cadidate_list.get(0);
			}
		}
		if(!r_crtc.Batch.NativeGoodsID) {
			if(!!r_crtc.Batch.ProductItem.Guid) {
				VetisEntityCore::Entity ent_pi;
				if(PeC.GetEntityByGuid(r_crtc.Batch.ProductItem.Guid, ent_pi) > 0) {
					VetisProductItem pi;
					if(PeC.Get(ent_pi.ID, pi) > 0) {
						Goods2Tbl::Rec goods_rec;
						if(pi.NativeGoodsID > 0 && GObj.Fetch(pi.NativeGoodsID, &goods_rec) > 0) {
							r_crtc.Batch.NativeGoodsID = goods_rec.ID;
						}
						else if(pi.GlobalID.NotEmpty()) {
							if(GObj.SearchByBarcode(pi.GlobalID, &bc_rec, 0, 1) > 0) {
								r_crtc.Batch.NativeGoodsID = bc_rec.GoodsID;
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

void PPVetisInterface::ParseListResult(const xmlNode * pNode, VetisApplicationBlock::ReplyListValues & rResult)
{
	SString temp_buf;
	rResult.Count = SXml::GetAttrib(pNode, "count", temp_buf) ? temp_buf.ToLong() : 0;
	rResult.Offset = SXml::GetAttrib(pNode, "offset", temp_buf) ? temp_buf.ToLong() : 0;
	rResult.Total = SXml::GetAttrib(pNode, "total", temp_buf) ? temp_buf.ToLong() : 0;
}

void PPVetisInterface::PutListAttributes(SXml::WNode & rN, long count, long offset, long total)
{
	SString & r_temp_buf = SLS.AcquireRvlStr();
	rN.PutAttrib("count", r_temp_buf.Z().Cat(count));
	rN.PutAttrib("total", r_temp_buf.Z().Cat(total));
	rN.PutAttrib("offset", r_temp_buf.Z().Cat(offset));
}

int PPVetisInterface::ParseReply(const SString & rReply, VetisApplicationBlock & rResult)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	rResult.Clear();
	SString temp_buf;
	xmlNode * p_root = 0;
	THROW(p_ctx = xmlNewParserCtxt());
	THROW_LXML(p_doc = xmlCtxtReadMemory(p_ctx, rReply, rReply.Len(), 0, 0, XML_PARSE_NOENT), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "Envelope")) {
		for(const xmlNode * p_env = p_root->children; p_env; p_env = p_env->next) {
			if(SXml::IsName(p_env, "Body")) {
				for(const xmlNode * p_b = p_env->children; p_b; p_b = p_b->next) {
					if(SXml::IsName(p_b, "receiveApplicationResultResponse") || SXml::IsName(p_b, "submitApplicationResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "application")) {
								for(const xmlNode * p_a = p_i ? p_i->children : 0; p_a; p_a = p_a->next) {
									if(SXml::GetContentByName(p_a, "applicationId", temp_buf))
										rResult.ApplicationId.FromStr(temp_buf);
									else if(SXml::GetContentByName(p_a, "status", temp_buf))
										rResult.ApplicationStatus = SIntToSymbTab_GetId(VetisAppStatus_SymbTab, SIZEOFARRAY(VetisAppStatus_SymbTab), temp_buf);
									else if(SXml::GetContentByName(p_a, "serviceId", temp_buf))
										rResult.ServiceId = temp_buf;
									else if(SXml::GetContentByName(p_a, "issuerId", temp_buf))
										rResult.IssuerId.FromStr(temp_buf);
									else if(SXml::GetContentByName(p_a, "issueDate", temp_buf))
										strtodatetime(temp_buf, &rResult.IssueDate, DATF_ISO8601, TIMF_HMS);
									else if(SXml::GetContentByName(p_a, "rcvDate", temp_buf))
										strtodatetime(temp_buf, &rResult.RcvDate, DATF_ISO8601, TIMF_HMS);
									else if(SXml::GetContentByName(p_a, "prdcRsltDate", temp_buf))
										strtodatetime(temp_buf, &rResult.PrdcRsltDate, DATF_ISO8601, TIMF_HMS);
									else if(SXml::IsName(p_a, "result")) {
										for(const xmlNode * p_r = p_a->children; p_r; p_r = p_r->next) {
											if(SXml::IsName(p_r, "checkShipmentRegionalizationResponse")) { // @v11.0.11
												for(const xmlNode * p_l = p_r->children; p_l; p_l = p_l->next) {
													if(SXml::IsName(p_l, "r13nRouteSection")) {
														VetisRouteSectionR13nRules * p_r13rules = rResult.R13RulesList.CreateNewItem();
														THROW_SL(p_r13rules);
														for(const xmlNode * p_rs = p_l->children; p_rs; p_rs = p_rs->next) {
															if(SXml::GetContentByName(p_rs, "sqnId", temp_buf)) {
																p_r13rules->SqnId = temp_buf.ToLong();
															}
															else if(SXml::IsName(p_rs, "appliedR13nRule")) {
																VetisRegionalizationShippingRule * p_rule = p_r13rules->RuleList.CreateNewItem();
																for(const xmlNode * p_rn = p_rs->children; p_rn; p_rn = p_rn->next) {
																	if(SXml::IsName(p_rn, "cargoType")) {
																		VetisSubProduct * p_sp = p_rule->CargoTypeList.CreateNewItem();
																		THROW_SL(p_sp);
																		ParseSubProduct(p_rn, *p_sp);
																	}
																	else if(SXml::GetContentByName(p_rn, "decision", temp_buf)) {
																		p_rule->Decision = temp_buf.ToLong();
																	}
																	else if(SXml::IsName(p_rn, "requirement")) {
																		VetisRegionalizationRequirement * p_req = p_rule->ReqList.CreateNewItem();
																		THROW_SL(p_req);
																		for(const xmlNode * p_reqn = p_rn->children; p_reqn; p_reqn = p_reqn->next) {
																			if(SXml::IsName(p_reqn, "relatedDisease")) {
																				ParseNamedGenericVersioningEntity(p_reqn, p_req->AnimalDisease);
																			}
																			else if(SXml::GetContentByName(p_reqn, "type", temp_buf)) {
																				p_req->Type = temp_buf.ToLong();
																			}
																			else if(SXml::IsName(p_reqn, "conditionGroup")) {
																				TSCollection <VetisRegionalizationCondition> * p_cndgrp = &p_req->ConditionGroup;
																				for(const xmlNode * p_cg = p_reqn->children; p_cg; p_cg = p_cg->next) {
																					if(SXml::IsName(p_cg, "condition")) {
																						VetisRegionalizationCondition * p_condition = p_cndgrp->CreateNewItem();
																						THROW_SL(p_condition);
																						ParseGenericVersioningEntity(p_cg, *p_condition);
																						for(const xmlNode * p_cnd = p_cg->children; p_cnd; p_cnd = p_cnd->next) {
																							if(SXml::GetContentByName(p_cnd, "referenceNumber", temp_buf)) {
																								p_condition->RefNumber = temp_buf;
																							}
																							else if(p_cnd->type == XML_ELEMENT_NODE && SXml::GetContentByName(p_cnd, "text", temp_buf)) {
																								p_condition->Text = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
																							}
																							else if(SXml::GetContentByName(p_cnd, "strict", temp_buf)) {
																								if(temp_buf.IsEqiAscii("false") || temp_buf == "0")
																									p_condition->Strict = false;
																								else
																									p_condition->Strict = true;
																							}
																							else if(SXml::IsName(p_cnd, "relatedDisease")) {
																								ParseNamedGenericVersioningEntity(p_cnd, p_condition->RelatedDisease);
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
											else if(SXml::IsName(p_r, "processIncomingConsignmentResponse")) {
												for(const xmlNode * p_l = p_r->children; p_l; p_l = p_l->next) {
													if(SXml::IsName(p_l, "vetDocument")) {
														THROW(ParseVetDocument(p_l, rResult.VetDocList.CreateNewItem()));
													}
													else if(SXml::IsName(p_l, "stockEntry")) {
														THROW(ParseStockEntry(p_l, rResult.VetStockList.CreateNewItem()));
													}
												}
											}
											else if(SXml::IsName(p_r, "registerProductionOperationResponse")) {
												for(const xmlNode * p_si = p_r->children; p_si; p_si = p_si->next) {
													if(SXml::IsName(p_si, "stockEntryList")) {
														ParseListResult(p_si, rResult.ListResult);
														for(const xmlNode * p_pr = p_si->children; p_pr; p_pr = p_pr->next) {
															if(SXml::IsName(p_pr, "stockEntry")) {
																THROW(ParseStockEntry(p_pr, rResult.VetStockList.CreateNewItem()));
															}
														}
													}
													else if(SXml::IsName(p_si, "vetDocument")) {
														THROW(ParseVetDocument(p_si, rResult.VetDocList.CreateNewItem()));
													}
												}
											}
											else if(SXml::IsName(p_r, "getStockEntryByUuidResponse") || SXml::IsName(p_r, "getStockEntryByGuidResponse")) {
												for(const xmlNode * p_l = p_r->children; p_l; p_l = p_l->next) {
													if(SXml::IsName(p_l, "stockEntry")) {
														THROW(ParseStockEntry(p_l, rResult.VetStockList.CreateNewItem()));
													}
												}
											}
											else if(SXml::IsName(p_r, "getVetDocumentListResponse") || SXml::IsName(p_r, "getVetDocumentChangesListResponse")) {
												for(const xmlNode * p_l = p_r->children; p_l; p_l = p_l->next) {
													if(SXml::IsName(p_l, "vetDocumentList")) {
														ParseListResult(p_l, rResult.ListResult);
														for(const xmlNode * p_doci = p_l->children; p_doci; p_doci = p_doci->next) {
															if(SXml::IsName(p_doci, "vetDocument")) {
																THROW(ParseVetDocument(p_doci, rResult.VetDocList.CreateNewItem()));
															}
														}
													}
												}
											}
											else if(SXml::IsName(p_r, "modifyProducerStockListResponse")) {
												for(const xmlNode * p_l = p_r->children; p_l; p_l = p_l->next) {
													if(SXml::IsName(p_l, "productItemList")) {
														ParseListResult(p_l, rResult.ListResult);
														for(const xmlNode * p_li = p_l->children; p_li; p_li = p_li->next) {
															if(SXml::IsName(p_li, "productItem")) {
																THROW(ParseProductItem(p_li, rResult.ProductItemList.CreateNewItem()));
															}
														}
													}
												}
											}
											else if(SXml::IsName(p_r, "getVetDocumentByUuidResponse")) {
												for(const xmlNode * p_doci = p_r->children; p_doci; p_doci = p_doci->next) {
													if(SXml::IsName(p_doci, "vetDocument")) {
														THROW(ParseVetDocument(p_doci, rResult.VetDocList.CreateNewItem()));
													}
												}
											}
											else if(SXml::IsName(p_r, "getStockEntryListResponse") || SXml::IsName(p_r, "getStockEntryChangesListResponse") ||
												SXml::IsName(p_r, "resolveDiscrepancyResponse")) {
												for(const xmlNode * p_si = p_r->children; p_si; p_si = p_si->next) {
													if(SXml::IsName(p_si, "stockEntryList")) {
														ParseListResult(p_si, rResult.ListResult);
														for(const xmlNode * p_pr = p_si->children; p_pr; p_pr = p_pr->next) {
															if(SXml::IsName(p_pr, "stockEntry")) {
																THROW(ParseStockEntry(p_pr, rResult.VetStockList.CreateNewItem()));
															}
														}
													}
												}
											}
											else if(SXml::IsName(p_r, "prepareOutcomingConsignmentResponse") || SXml::IsName(p_r, "prepareOutgoingConsignmentResponse")) {
												for(const xmlNode * p_r2 = p_r->children; p_r2; p_r2 = p_r2->next) {
													if(SXml::IsName(p_r2, "stockEntry")) {
														THROW(ParseStockEntry(p_r2, rResult.VetStockList.CreateNewItem()));
													}
													else if(SXml::IsName(p_r2, "vetDocument")) {
														THROW(ParseVetDocument(p_r2, rResult.VetDocList.CreateNewItem()));
													}
												}
											}
											else if(SXml::IsName(p_r, "modifyEnterpriseResponse")) {
												for(const xmlNode * p_r2 = p_r->children; p_r2; p_r2 = p_r2->next) {
													if(SXml::IsName(p_r2, "enterprise")) {
														THROW(ParseEnterprise(p_r2, rResult.EntItemList.CreateNewItem()));
													}
												}
											}
										}
									}
									else if(SXml::IsName(p_a, "errors")) {
										for(const xmlNode * p_nerr = p_a->children; p_nerr; p_nerr = p_nerr->next) {
											if(SXml::GetContentByName(p_nerr, "error", temp_buf)) {
												VetisErrorEntry * p_err = rResult.ErrList.CreateNewItem();
												THROW_SL(p_err);
												p_err->Item = temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
												if(SXml::GetAttrib(p_nerr, "code", temp_buf))
													p_err->Code = temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
											}
										}
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getProductChangesListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "productList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "product")) {
										VetisProduct * p_new_item = rResult.ProductList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseProduct(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getProductByGuidResponse")) {
						for(const xmlNode * p_pr = p_b->children; p_pr; p_pr = p_pr->next) {
							if(SXml::IsName(p_pr, "product")) {
								VetisProduct * p_new_item = rResult.ProductList.CreateNewItem();
								THROW_SL(p_new_item);
								THROW(ParseProduct(p_pr, *p_new_item));
							}
						}
					}
					else if(SXml::IsName(p_b, "getSubProductChangesListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "subProductList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "subProduct")) {
										VetisSubProduct * p_new_item = rResult.SubProductList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseSubProduct(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getSubProductByGuidResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "subProduct")) {
								VetisSubProduct * p_new_item = rResult.SubProductList.CreateNewItem();
								THROW_SL(p_new_item);
								THROW(ParseSubProduct(p_i, *p_new_item));
							}
						}
					}
					else if(SXml::IsName(p_b, "getLocalityListByRegionResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "localityList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "locality")) {
										VetisAddressObjectView * p_new_item = rResult.RegionList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseRegion(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getRegionListByCountryResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "regionList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "region")) {
										VetisAddressObjectView * p_new_item = rResult.RegionList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseRegion(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getAllCountryListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "countryList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "country")) {
										VetisCountry * p_new_item = rResult.CountryList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseCountry(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getProductItemListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "productItemList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "productItem")) {
										THROW(ParseProductItem(p_pr, rResult.ProductItemList.CreateNewItem()));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getProductItemByGuidResponse") || SXml::IsName(p_b, "getProductItemByUuidResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "productItem")) {
								THROW(ParseProductItem(p_i, rResult.ProductItemList.CreateNewItem()));
							}
						}
					}
					else if(SXml::IsName(p_b, "getRussianEnterpriseListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "enterpriseList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "enterprise")) {
										THROW(ParseEnterprise(p_pr, rResult.EntItemList.CreateNewItem()));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getEnterpriseByGuidResponse") || SXml::IsName(p_b, "getEnterpriseByUuidResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "enterprise")) {
								THROW(ParseEnterprise(p_i, rResult.EntItemList.CreateNewItem()));
							}
						}
					}
					else if(SXml::IsName(p_b, "getBusinessEntityListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "businessEntityList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "businessEntity")) {
										VetisBusinessEntity * p_new_item = rResult.BEntList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseBusinessEntity(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getBusinessEntityByGuidResponse") || SXml::IsName(p_b, "getBusinessEntityByUuidResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "businessEntity")) {
								VetisBusinessEntity * p_new_item = rResult.BEntList.CreateNewItem();
								THROW_SL(p_new_item);
								THROW(ParseBusinessEntity(p_i, *p_new_item));
							}
						}
					}
					else if(SXml::IsName(p_b, "getUnitListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "unitList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "unit")) {
										VetisUnit * p_new_item = rResult.UnitList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseUnit(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "getPurposeListResponse")) {
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::IsName(p_i, "purposeList")) {
								ParseListResult(p_i, rResult.ListResult);
								for(const xmlNode * p_pr = p_i->children; p_pr; p_pr = p_pr->next) {
									if(SXml::IsName(p_pr, "purpose")) {
										VetisPurpose * p_new_item = rResult.PurposeList.CreateNewItem();
										THROW_SL(p_new_item);
										THROW(ParseNamedGenericVersioningEntity(p_pr, *p_new_item));
									}
								}
							}
						}
					}
					else if(SXml::IsName(p_b, "Fault")) {
						VetisFault single_fault;
						for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
							if(SXml::GetContentByName(p_i, "faultcode", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_i, "faultstring", temp_buf)) {
								single_fault.Message = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							}
							else if(SXml::GetContentByName(p_i, "faultactor", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_i, "detail", temp_buf)) {
								static const SIntToSymbTabEntry fault_tab[] = {
									{ VetisFault::tUndef, "fault" },
									{ VetisFault::tIncorrectRequest, "incorrectRequestFault" },
									{ VetisFault::tAccessDenied, "accessDeniedFault" },
									{ VetisFault::tEntityNotFound, "entityNotFoundFault" },
									{ VetisFault::tInternalService, "internalServiceFault" },
									{ VetisFault::tUnknownServiceId, "unknownServiceIdFault" },
									{ VetisFault::tUnsupportedApplicationDataType, "unsupportedApplicationDataTypeFault" },
								};
								for(const xmlNode * p_d = p_i->children; p_d; p_d = p_d->next) {
									for(uint f = 0; f < SIZEOFARRAY(fault_tab); f++) {
										if(SXml::GetContentByName(p_d, fault_tab[f].P_Symb, temp_buf)) {
											VetisFault * p_fault = rResult.FaultList.CreateNewItem();
											THROW_SL(p_fault);
											p_fault->Type = fault_tab[f].Id;
											THROW(ParseFault(p_d, *p_fault));
											break;
										}
									}
								}
							}
						}
						if(!single_fault.IsEmpty()) {
							VetisFault * p_fault = rResult.FaultList.CreateNewItem();
							THROW_SL(p_fault);
							*p_fault = single_fault;
						}
					}
				}
			}
		}
	}
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	CATCHZOK
	return ok;
}

static const char * GetAppSvcPath(SVerT ver, const char * pAppSvc) // @v11.9.11
{
	//"platform/services/2.0/ProductService"
	SString & r_result = SLS.AcquireRvlStr();
	(r_result = "platform/services");
	if(ver.IsLt(2, 0, 0)) {
		;
	}
	else if(ver.GetMajor() == 2) {
		if(ver.GetMinor() == 0)
			r_result.Slash().Cat("2.0");
		else if(ver.GetMinor() == 1)
			r_result.Slash().Cat("2.1");
	}
	if(!isempty(pAppSvc))
		r_result.Slash().Cat(pAppSvc);
	return r_result;
}

static const char * GetAppSvcUrl(SVerT ver, int isTestContour)
{
	const char * p_domain = (isTestContour/*P.Flags & P.fTestContour*/) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
	return InetUrl::MkHttps(p_domain, GetAppSvcPath(ver, "ApplicationManagementService")); // @v11.9.11
	/* @v11.9.11 if(mjVer == 1) {
		return InetUrl::MkHttps(p_domain, "platform/services/ApplicationManagementService");
	}
	else if(mjVer == 2) {
		if(mnVer == 0)
			return InetUrl::MkHttps(p_domain, "platform/services/2.0/ApplicationManagementService");
		else if(mnVer == 1)
			return InetUrl::MkHttps(p_domain, "platform/services/2.1/ApplicationManagementService");
	}
	return 0;*/
}

static void FASTCALL PutSingleGuidEntity(SXmlWriter & rDoc, const char * pEntNs, const char * pEnt, const char * pGuidNs, const S_GUID & rGuid)
{
	SXml::WNode n_e(rDoc, SXml::nst(pEntNs, pEnt));
	SString temp_buf;
	n_e.PutInner(SXml::nst(pGuidNs, "guid"), temp_buf.Z().Cat(rGuid, S_GUID::fmtIDL|S_GUID::fmtLower));
}

static void FASTCALL PutNonZeroUuid(SXml::WNode & rN, const char * pNs, const S_GUID & rUuid)
{
	if(!rUuid.IsZero()) {
		SString temp_buf;
		rN.PutInner(SXml::nst(pNs, "uuid"), temp_buf.Z().Cat(rUuid, S_GUID::fmtIDL|S_GUID::fmtLower));
	}
}

static void FASTCALL PutNonZeroGuid(SXml::WNode & rN, const char * pNs, const S_GUID & rUuid)
{
	if(!rUuid.IsZero()) {
		SString temp_buf;
		rN.PutInner(SXml::nst(pNs, "guid"), temp_buf.Z().Cat(rUuid, S_GUID::fmtIDL|S_GUID::fmtLower));
	}
}

static void FASTCALL PutNonZeroEntityGuid(xmlTextWriter * pWriter, const char * pEntityNs, const char * pEntity, const char * pGuidNs, const S_GUID & rUuid)
{
	SXml::WNode n_be(pWriter, SXml::nst(pEntityNs, pEntity));
	PutNonZeroGuid(n_be, pGuidNs, rUuid);
}

static void FASTCALL PutNonEmptyText(SXml::WNode & rN, const char * pNs, const char * pTag, const SString & rBuf)
{
	if(rBuf.NotEmpty()) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		r_temp_buf = rBuf;
		if(r_temp_buf.NotEmptyS()) {
			rN.PutInner(SXml::nst(pNs, pTag), r_temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
		}
	}
}

static void FASTCALL PutHeader(SXmlWriter & rDoc)
{
	SXml::WNode n_hdr(rDoc, SXml::nst("soapenv", "Header"));
}

static void FASTCALL PutInitiator(SXmlWriter & rDoc, const char * pNs, const char * pUserNs, const char * pUser)
{
	SXml::WNode n_n2(rDoc, SXml::nst(pNs, "initiator"));
	n_n2.PutInner(SXml::nst(pUserNs, "login"), pUser);
}

static void FASTCALL PutListOptions(SXmlWriter & rDoc, const char * pNs, const VetisListOptions & rLo)
{
	SString temp_buf;
	SXml::WNode n_n2(rDoc, SXml::nst(pNs, "listOptions"));
	n_n2.PutInner(SXml::nst(pNs, "count"), temp_buf.Z().Cat(NZOR(rLo.Count, 100)));
	n_n2.PutInner(SXml::nst(pNs, "offset"), temp_buf.Z().Cat(rLo.Offset));
}

static SString & FASTCALL VGuidToStr(const S_GUID & rGuid, SString & rBuf)
	{ return rGuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, rBuf); }

int PPVetisInterface::SubmitRequest(VetisApplicationBlock & rAppBlk, VetisApplicationBlock & rResult)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	THROW(PrepareApplicationBlockForReq(rAppBlk));
	THROW(rAppBlk.P_AppParam);
	{
		VetisSubmitRequestBlock srb;
		if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signCheckShipmentRegionalization) { // @v11.0.0
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib_Ns("dt",     P_ApiVtrf, "schema/cdm/dictionary/v2");
			n_env.PutAttrib_Ns("bs",     P_ApiVtrf, "schema/cdm/base");
			n_env.PutAttrib_Ns("merc",   P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2");
			n_env.PutAttrib_Ns("apldef", P_ApiVtrf, "schema/cdm/application/ws-definitions");
			n_env.PutAttrib_Ns("apl",    P_ApiVtrf, "schema/cdm/application");
			n_env.PutAttrib_Ns("vd",     P_ApiVtrf, "schema/cdm/mercury/vet-document/v2");
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const CheckShipmentRegionalizationRequest * p_req = static_cast<const CheckShipmentRegionalizationRequest *>(rAppBlk.P_AppParam);
							SXml::WNode n_req(srb, SXml::nst("merc", "checkShipmentRegionalizationRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							{
								for(uint cti = 0; cti < p_req->SubGroupGuidList.getCount(); cti++) {
									//SXml::WNode n_cargot(srb, "")
									SXml::WNode n_ct(srb, _xmlnst_dt("cargoType"));
									p_req->SubGroupGuidList.at(cti).ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
									n_ct.PutInner(_xmlnst_bs_guid(), temp_buf);
								}
								{
									long   rp_no = 0;
									SXml::WNode n_sr(srb, _xmlnst_vd("shipmentRoute"));
									for(uint ri = 0; ri < p_req->RoutePointList.getCount(); ri++) {
										const VetisShipmentRoutePoint * p_rp = p_req->RoutePointList.at(ri);
										if(p_rp) {
											SXml::WNode n_rp(srb, _xmlnst_vd("routePoint"));
											n_rp.PutInner(_xmlnst_vd("sqnId"), temp_buf.Z().Cat(++rp_no));
											if(p_rp->P_Enterprise) {
												SXml::WNode n_ent(srb, _xmlnst_vd("enterprise"));
												PutNonZeroUuid(n_ent, "bs", p_rp->P_Enterprise->Uuid);
												PutNonZeroGuid(n_ent, "bs", p_rp->P_Enterprise->Guid);
											}
											else if(p_rp->P_Location) {
												SXml::WNode n_loc(srb, _xmlnst_vd("location"));
												{
													SXml::WNode n_adr(srb, _xmlnst_dt("address"));
													{
														SXml::WNode n_country(srb, _xmlnst_dt("country"));
														if(!p_rp->P_Location->Address.Country.Guid.IsZero())
															p_rp->P_Location->Address.Country.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
														else
															temp_buf = P_VetisGuid_Country_Ru; // Россия
														n_country.PutInner(_xmlnst_bs_guid(), temp_buf);
													}
													{
														SXml::WNode n_reg(srb, _xmlnst_dt("region"));
														if(!p_rp->P_Location->Address.Region.Guid.IsZero())
															p_rp->P_Location->Address.Region.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
														else
															temp_buf = P_VetisGuid_Region_Ru_10; // Карелия
														n_reg.PutInner(_xmlnst_bs_guid(), temp_buf);
													}
												}
											}
										}
									}
								}
								/*
									cargoType
									shipmentRoute
										routePoint
											sqnId
											location
												address
													country
													region
													district
													locality
													subLocality
											enterprise
								*/
							}
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signModifyEnterprise) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib_Ns("dt",     P_ApiVtrf, "schema/cdm/dictionary/v2");
			n_env.PutAttrib_Ns("bs",     P_ApiVtrf, "schema/cdm/base");
			n_env.PutAttrib_Ns("merc",   P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2");
			n_env.PutAttrib_Ns("apldef", P_ApiVtrf, "schema/cdm/application/ws-definitions");
			n_env.PutAttrib_Ns("apl",    P_ApiVtrf, "schema/cdm/application");
			n_env.PutAttrib_Ns("vd",     P_ApiVtrf, "schema/cdm/mercury/vet-document/v2");
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						//n_app.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
						// (?) n_app.PutInner(SXml::nst("app", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisModifyEnterpriseRequest * p_req = static_cast<const VetisModifyEnterpriseRequest *>(rAppBlk.P_AppParam);
							const VetisEnterprise & r_ent = p_req->En;
							SXml::WNode n_req(srb, SXml::nst("merc", "modifyEnterpriseRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "modificationOperation"));
								SIntToSymbTab_GetSymb(VetisRegisterModificationType_SymbTab, SIZEOFARRAY(VetisRegisterModificationType_SymbTab), p_req->ModType, temp_buf);
								n_n2.PutInner(_xmlnst_vd("type"), temp_buf);
								{
									SXml::WNode n_n3(srb, _xmlnst_vd("resultingList"));
									PutListAttributes(n_n3, 1, 0, 1);
									{
										SXml::WNode n_n4(srb, _xmlnst_dt("enterprise"));
										temp_buf.Z();
										n_n4.PutInner(_xmlnst_dt("name"), EncText((temp_buf = r_ent.Name).Transf(CTRANSF_INNER_TO_UTF8)));
										n_n4.PutInner(_xmlnst_dt("type"), temp_buf.Z().Cat(1L));
										{
											SXml::WNode n_adr(srb, _xmlnst_dt("address"));
											{
												SXml::WNode n_c(srb, _xmlnst_dt("country"));
												if(!r_ent.Address.Country.Guid.IsZero())
													r_ent.Address.Country.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
												else
													temp_buf = P_VetisGuid_Country_Ru; // Россия
												n_c.PutInner(_xmlnst_bs_guid(), temp_buf);
											}
											{
												SXml::WNode n_c(srb, _xmlnst_dt("region"));
												if(!r_ent.Address.Region.Guid.IsZero())
													r_ent.Address.Region.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
												else
													temp_buf = P_VetisGuid_Region_Ru_10; // Карелия
												n_c.PutInner(_xmlnst_bs_guid(), temp_buf);
											}
											{
												SXml::WNode n_c(srb, _xmlnst_dt("locality"));
												if(!r_ent.Address.Locality.Guid.IsZero())
													r_ent.Address.Locality.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
												else
													temp_buf = "ccc34487-8fd4-4e71-b032-f4e6c82fb354"; // Петрозаводск
												n_c.PutInner(_xmlnst_bs_guid(), temp_buf);
											}
											n_adr.PutInner(_xmlnst_dt("addressView"), EncText((temp_buf = r_ent.Address.AddressView).Transf(CTRANSF_INNER_TO_UTF8)));
										}
										{
											SXml::WNode n_actl(srb, _xmlnst_dt("activityList"));
											PutListAttributes(n_actl, 1, 0, 1);
											{
												SXml::WNode n_act(srb, _xmlnst_dt("activity"));
												n_act.PutInner(_xmlnst_dt("name"), temp_buf = "Реализация пищевых продуктов"); //<dt:name>Реализация пищевых продуктов</dt:name>
											}
										}
										if(r_ent.P_Owner && !r_ent.P_Owner->Guid.IsZero()) {
											SXml::WNode n_owner(srb, _xmlnst_dt("owner"));
											r_ent.P_Owner->Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
											n_owner.PutInner(_xmlnst_bs_guid(), temp_buf);
										}
										/*if(r_ent.OfficialRegistration.getCount()) {
											const VetisEnterpriseOfficialRegistration * p_ofr = r_ent.OfficialRegistration.at(0);
											if(p_ofr) {
												SXml::WNode n_reg(srb, SXml::nst("ent", "officialRegistration"));
												if(p_ofr->P_BusinessEntity) {
													SXml::WNode n_be(srb, SXml::nst("ent", "businessEntity"));
													n_be.PutInner(SXml::nst("ent", "inn"), p_ofr->P_BusinessEntity->Inn);
												}
												n_reg.PutInner(SXml::nst("ent", "kpp"), p_ofr->Kpp);
											}
										}*/
									}
								}
								n_n2.PutInner(_xmlnst_vd("reason"), "I don't know. There is probably some reason."); //<vd:reason>Причина добавления предприятия в реестр вот такая вот.</vd:reason>
							}
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signModifyActivityLocations) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib_Ns("bs", P_ApiVtrf, "schema/cdm/base");
			n_env.PutAttrib_Ns("merc", P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2");
			n_env.PutAttrib_Ns("apldef", P_ApiVtrf, "schema/cdm/application/ws-definitions");
			n_env.PutAttrib_Ns("apl", P_ApiVtrf, "schema/cdm/application");
			n_env.PutAttrib_Ns("vd", P_ApiVtrf, "schema/cdm/mercury/vet-document/v2");
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						//n_app.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
						// (?) n_app.PutInner(SXml::nst("app", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisModifyActivityLocationsRequest * p_req = static_cast<const VetisModifyActivityLocationsRequest *>(rAppBlk.P_AppParam);
							const VetisEnterprise & r_ent = p_req->En;
							const VetisBusinessEntity & r_be = p_req->Be;
							SXml::WNode n_req(srb, SXml::nst("merc", "modifyActivityLocationsRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "modificationOperation"));
								SIntToSymbTab_GetSymb(VetisRegisterModificationType_SymbTab, SIZEOFARRAY(VetisRegisterModificationType_SymbTab), p_req->ModType, temp_buf);
								n_n2.PutInner(_xmlnst_vd("type"), temp_buf);
								{
									SXml::WNode n_be(srb, _xmlnst_vd("businessEntity"));
									PutNonZeroGuid(n_be, "bs", r_be.Guid);
								}
								{
									SXml::WNode n_el(srb, _xmlnst_vd("activityLocation"));
									//PutListAttributes(n_el, 1, 0, 1);
									{
										SXml::WNode n_en(srb, _xmlnst_vd("enterprise"));
										PutNonZeroGuid(n_en, "bs", r_ent.Guid);
									}
								}
							}
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signModifyProducerStockListOperation) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib_Ns("soapenv", "schemas.xmlsoap.org", "soap/envelope/");
			n_env.PutAttrib("xs",     InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
			n_env.PutAttrib("xsi",    InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
			n_env.PutAttrib_Ns("dt",     P_ApiVtrf, "schema/cdm/dictionary/v2");
			n_env.PutAttrib_Ns("bs",     P_ApiVtrf, "schema/cdm/base");
			n_env.PutAttrib_Ns("merc",   P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2");
			n_env.PutAttrib_Ns("apldef", P_ApiVtrf, "schema/cdm/application/ws-definitions");
			n_env.PutAttrib_Ns("app",    P_ApiVtrf, "schema/cdm/application");
			n_env.PutAttrib_Ns("vd",     P_ApiVtrf, "schema/cdm/mercury/vet-document/v2");
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("app", "application"));
						//n_app.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
						// (?) n_app.PutInner(SXml::nst("app", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("app", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("app", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("app", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("app", "data"));
						{
							const ModifyProducerStockListOperationRequest * p_req = static_cast<const ModifyProducerStockListOperationRequest *>(rAppBlk.P_AppParam);
							const VetisProductItem & r_pi = p_req->Pi;
							SXml::WNode n_req(srb, SXml::nst("merc", "modifyProducerStockListRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "modificationOperation"));
								SIntToSymbTab_GetSymb(VetisRegisterModificationType_SymbTab, SIZEOFARRAY(VetisRegisterModificationType_SymbTab), p_req->ModType, temp_buf);
								n_n2.PutInner(_xmlnst_vd("type"), temp_buf);
								{
									SXml::WNode n_n3(srb, _xmlnst_vd("resultingList"));
									PutListAttributes(n_n3, 1, 0, 1);
									{
										SXml::WNode n_item(srb, _xmlnst_dt("productItem"));
										n_item.PutInnerSkipEmpty(_xmlnst_dt("globalID"), r_pi.GlobalID);
										n_item.PutInner(_xmlnst_dt("name"), EncText((temp_buf = r_pi.Name).Transf(CTRANSF_INNER_TO_UTF8)));
										n_item.PutInnerSkipEmpty(_xmlnst_dt("code"), r_pi.Code);
										n_item.PutInner(_xmlnst_dt("productType"), temp_buf.Z().Cat(r_pi.ProductType));
										{
											SXml::WNode n_n4(srb, _xmlnst_dt("product"));
											PutNonZeroGuid(n_n4, "bs", r_pi.Product.Guid);
										}
										{
											SXml::WNode n_n4(srb, _xmlnst_dt("subProduct"));
											PutNonZeroGuid(n_n4, "bs", r_pi.SubProduct.Guid);
										}
										n_item.PutInner(_xmlnst_dt("correspondsToGost"), "false");
										{
											SXml::WNode n_n4(srb, _xmlnst_dt("producer"));
											PutNonZeroGuid(n_n4, "bs", r_pi.Producer.Guid);
										}
										if(r_pi.Producing.getCount()) {
											for(uint pidx = 0; pidx < r_pi.Producing.getCount(); pidx++) {
												const VetisEnterprise * p_ent = r_pi.Producing.at(pidx);
												if(p_ent) {
													SXml::WNode n_n4(srb, _xmlnst_dt("producing"));
													{
														SXml::WNode n_n5(srb, _xmlnst_dt("location"));
														PutNonZeroGuid(n_n5, "bs", p_ent->Guid);
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
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signWithdrawVetDocument) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("xs"),      InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
			n_env.PutAttrib(SXml::nst_xmlns("xsi"),     InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst(/*"ws"*/0, "submitApplicationRequest"));
					n_f.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst(/*"ws"*/0, "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst(/*"app"*/0, "application"));
						n_f.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
						n_app.PutInner(SXml::nst(/*"app"*/0, "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst(/*"app"*/0, "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst(/*"app"*/0, "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst(/*"app"*/0, "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst(/*"app"*/0, "data"));
						{
							VetisWithdrawVetDocumentRequest * p_req = (VetisWithdrawVetDocumentRequest *)rAppBlk.P_AppParam;
							const VetisVetDocument & r_doc = p_req->Doc;
							const VetisBatch & r_bat = r_doc.CertifiedConsignment.Batch;
							SXml::WNode n_req(srb, "withdrawVetDocumentRequest");
							n_req.PutAttrib("xmlns",  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/applications"));
							n_req.PutAttrib(SXml::nst_xmlns("bs"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
							n_req.PutInner("localTransactionId", temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							{
								SXml::WNode n_n2(srb, "initiator");
								n_n2.PutAttrib(SXml::nst_xmlns("d7p1"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/common"));
								n_n2.PutInner(SXml::nst("d7p1", "login"), rAppBlk.User);
							}
							n_req.PutInner("vetDocumentId", VGuidToStr(r_doc.Uuid, temp_buf));
							PPLoadString(PPSTR_HASHTOKEN_C, PPHSC_VETIS_FILLINGOUTERROR, temp_buf);
							n_req.PutInner("withdrawReason", temp_buf.Transf(CTRANSF_INNER_TO_UTF8)); // "Ошибка в оформлении"
							n_req.PutInner("withdrawDate", temp_buf.Z().CatCurDateTime(DATF_ISO8601CENT, 0));
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signRegisterProduction) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("apl"),    InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("apldef"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("bs"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("merc"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("vd"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, 0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisRegisterProductionRequest * p_req = static_cast<const VetisRegisterProductionRequest *>(rAppBlk.P_AppParam);
							const VetisVetDocument & r_inc_doc = p_req->Doc;
							SXml::WNode n_req(srb, SXml::nst("merc", "registerProductionOperationRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "enterprise"));
								PutNonZeroGuid(n_n2, "bs", rAppBlk.EnterpriseId);
							}
							{
								SXml::WNode n_po(srb, SXml::nst("merc", "productionOperation"));
								for(uint slidx = 0; slidx < p_req->SourceList.getCount(); slidx++) {
									const VetisRegisterProductionRequest::SourceEntry * p_se = p_req->SourceList.at(slidx);
									if(p_se) {
										SXml::WNode n_rb(srb, _xmlnst_vd("rawBatch"));
										{
											SXml::WNode n_sstke(srb, _xmlnst_vd("sourceStockEntry"));
											PutNonZeroGuid(n_sstke, "bs", p_se->StockEntryGuid);
										}
										n_rb.PutInner(_xmlnst_vd("volume"), temp_buf.Z().Cat(p_se->Rec.Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
										{
											SXml::WNode n_uom(srb, _xmlnst_vd("unit"));
											PutNonZeroUuid(n_uom, "bs", p_se->Doc.CertifiedConsignment.Batch.Unit.Uuid);
											PutNonZeroGuid(n_uom, "bs", p_se->Doc.CertifiedConsignment.Batch.Unit.Guid);
										}
										/*
											<vd:sourceStockEntry>
											  <bs:guid>83711ef5-1e79-439c-9714-e16887ba784a</bs:guid>
											</vd:sourceStockEntry>
											<vd:volume>1</vd:volume>
											<vd:unit>
											  <bs:uuid>069792f0-053d-11e1-99b4-d8d385fbc9e8</bs:uuid>
											  <bs:guid>21ed96c9-337b-4a27-8761-c6e6ad3c9f5b</bs:guid>
											</vd:unit>
										*/
									}
								}
								{
									SXml::WNode n_bat(srb, _xmlnst_vd("productiveBatch"));
									n_bat.PutInner(_xmlnst_vd("productType"), temp_buf.Z().Cat(p_req->Pi.ProductType));
									{
										SXml::WNode n_p(srb, _xmlnst_vd("product"));
										PutNonZeroGuid(n_p, "bs", p_req->Pi.Product.Guid);
									}
									{
										SXml::WNode n_p(srb, _xmlnst_vd("subProduct"));
										PutNonZeroGuid(n_p, "bs", p_req->Pi.SubProduct.Guid);
									}
									{
										SXml::WNode n_p(srb, _xmlnst_vd("productItem"));
										PutNonZeroGuid(n_p, "bs", p_req->Pi.Guid);
										PutNonEmptyText(n_p, "dt", "name", EncText(p_req->Pi.Name));
									}
									n_bat.PutInner(_xmlnst_vd("volume"), temp_buf.Z().Cat(p_req->VdRec.Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
									{
										SXml::WNode n_uom(srb, _xmlnst_vd("unit"));
										PutNonZeroGuid(n_uom, "bs", S_GUID(P_VetisGuid_Unit_Kg));
										//PutNonZeroUuid(n_uom, "bs", "");
										//PutNonZeroGuid(n_uom, "bs", "");
									}
									{
										SXml::WNode n_dop(srb, _xmlnst_vd("dateOfProduction"));
										SUniTime ut;
										ut.FromInt64(p_req->VdRec.ManufDateFrom);
										PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", ut);
										//ut.FromInt64(p_req->VdRec.ManufDateTo);
										//PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", ut);
									}
									{
										SXml::WNode n_dop(srb, _xmlnst_vd("expiryDate"));
										SUniTime ut;
										ut.FromInt64(p_req->VdRec.ExpiryFrom);
										PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", ut);
										//ut.FromInt64(p_req->VdRec.ExpiryTo);
										//PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", ut);
									}
									{
										temp_buf.Z().Cat(p_req->VdRec.LinkBillID).CatChar('-').Cat(p_req->VdRec.LinkBillRow);
										n_bat.PutInner(_xmlnst_vd("batchID"), temp_buf);
									}
									n_bat.PutInner(_xmlnst_vd("perishable"), "false");
									n_bat.PutInner(_xmlnst_vd("lowGradeCargo"), "false");
								}
							}
							{
								SXml::WNode n_wd(srb, SXml::nst("merc", "vetDocument"));
								{
									// @v11.8.4 символ результата экспертизы теперь берется из сертификата (ранее был константой)
									SXml::WNode n_auth(srb, _xmlnst_vd("authentication"));
									const char * p_ce_symb = SIntToSymbTab_GetSymbPtr(
										VetAuthCargoExpertized_SymbTab, SIZEOFARRAY(VetAuthCargoExpertized_SymbTab), r_inc_doc.Authentication.CargoExpertized);
									n_auth.PutInner(_xmlnst_vd("cargoExpertized"), p_ce_symb ? p_ce_symb : "VSERAW"); // @v11.6.2 VSEFULL-->VSERAW 
								}
							}
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signResolveDiscrepancy) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("apl"),    InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("apldef"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("bs"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("merc"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("vd"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					n_f.PutAttrib(SXml::nst_xmlns("xs"),      InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
					n_f.PutAttrib(SXml::nst_xmlns("xsi"),     InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, 0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisResolveDiscrepancyRequest * p_req = static_cast<const VetisResolveDiscrepancyRequest *>(rAppBlk.P_AppParam);
							SXml::WNode n_req(srb, SXml::nst("merc", "resolveDiscrepancyRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							/*{
								SXml::WNode n_n2(srb, SXml::nst("merc", "businessEntity"));
								PutNonZeroGuid(n_n2, "bs", rAppBlk.IssuerId);
							}*/
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "enterprise"));
								PutNonZeroGuid(n_n2, "bs", rAppBlk.EnterpriseId);
							}
							{
								LDATETIME invdt;
								invdt.Set(p_req->VdRec.WayBillDate, ZEROTIME);
								n_req.PutInner(SXml::nst("merc", "inventoryDate"), temp_buf.Z().Cat(invdt, DATF_ISO8601CENT, 0));
							}
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "responsible"));
								n_n2.PutInner(_xmlnst_vd("login"), rAppBlk.User);
							}
							const char * p_sd_ident = "test-rdr"; // @sample
							{
								SXml::WNode n_sd(srb, SXml::nst("merc", "stockDiscrepancy"));
								n_sd.PutAttrib("id", p_sd_ident);
								{
									SXml::WNode n_rl(srb, _xmlnst_vd("resultingList"));
									PutListAttributes(n_rl, 1, 0, 1);
									{
										SXml::WNode n_se(srb, _xmlnst_vd("stockEntry"));
										{
											if(p_req->VdRec.Flags & VetisVetDocument::fDiscrepancyLack) {
												const VetisVetDocument & r_org_doc = p_req->OrgDoc;
												const VetisBatch & r_bat = r_org_doc.CertifiedConsignment.Batch;
												PutNonZeroUuid(n_se, "bs", p_req->StockEntryUuid);
												//
												SXml::WNode n_bat(srb, _xmlnst_vd("batch"));
												n_bat.PutInner(_xmlnst_vd("productType"), temp_buf.Z().Cat(p_req->Pi.ProductType));
												{
													SXml::WNode n_p(srb, _xmlnst_vd("product"));
													PutNonZeroGuid(n_p, "bs", p_req->Pi.Product.Guid);
												}
												{
													SXml::WNode n_p(srb, _xmlnst_vd("subProduct"));
													PutNonZeroGuid(n_p, "bs", p_req->Pi.SubProduct.Guid);
												}
												{
													SXml::WNode n_p(srb, _xmlnst_vd("productItem"));
													PutNonZeroGuid(n_p, "bs", p_req->Pi.Guid);
													PutNonEmptyText(n_p, "dt", "name", EncText(p_req->Pi.Name));
												}
												{
													double volume = smax(p_req->StockEntryRest - p_req->VdRec.Volume, 0.0);
													n_bat.PutInner(_xmlnst_vd("volume"), temp_buf.Z().Cat(volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
													{
														SXml::WNode n_uom(srb, _xmlnst_vd("unit"));
														PutNonZeroUuid(n_uom, "bs", r_bat.Unit.Uuid);
														PutNonZeroGuid(n_uom, "bs", r_bat.Unit.Guid);
													}
												}
												if(!!r_bat.DateOfProduction.FirstDate || !!r_bat.DateOfProduction.SecondDate) {
													SXml::WNode n_dt(srb, _xmlnst_vd("dateOfProduction"));
													PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", r_bat.DateOfProduction.FirstDate);
													PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", r_bat.DateOfProduction.SecondDate);
												}
												if(!!r_bat.ExpiryDate.FirstDate || !!r_bat.ExpiryDate.SecondDate) {
													SXml::WNode n_dt(srb, _xmlnst_vd("expiryDate"));
													PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", r_bat.ExpiryDate.FirstDate);
													PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", r_bat.ExpiryDate.SecondDate);
												}
												if(r_bat.BatchIdList.getCount()) {
													for(uint bipos = 0; r_bat.BatchIdList.get(&bipos, temp_buf);) {
														n_bat.PutInner(_xmlnst_vd("batchID"), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
													}
												}
												n_bat.PutInner(_xmlnst_vd("perishable"), temp_buf.Z().Cat(STextConst::GetBool(r_bat.Flags & VetisBatch::fPerishable)));
												//
												if(r_bat.PackageList.getCount()) {
													SXml::WNode n_pl(srb, _xmlnst_vd("packageList"));
													for(uint plidx = 0; plidx < r_bat.PackageList.getCount(); plidx++) {
														const VetisPackage * p_pckg = r_bat.PackageList.at(plidx);
														if(p_pckg) {
															SXml::WNode n_pckg(srb, _xmlnst_dt("package"));
															n_pckg.PutInner(_xmlnst_dt("level"), temp_buf.Z().Cat(p_pckg->Level));
															{
																SXml::WNode n_pt(srb, _xmlnst_dt("packingType"));
																PutNonZeroUuid(n_pt, "bs", p_pckg->PackingType.Uuid);
																PutNonZeroGuid(n_pt, "bs", p_pckg->PackingType.Guid);
																PutNonEmptyText(n_pt, "dt", "name", EncText(p_pckg->PackingType.Name));
															}
															n_pckg.PutInner(_xmlnst_dt("quantity"), temp_buf.Z().Cat(p_pckg->Quantity));
															if(p_pckg->ProductMarks.getCount()) {
																for(uint markidx = 0; markidx < p_pckg->ProductMarks.getCount(); markidx++) {
																	const VetisProductMarks * p_mark = p_pckg->ProductMarks.at(markidx);
																	if(p_mark) {
																		SXml::WNode n_mark(srb, _xmlnst_dt("productMarks"));
																		SIntToSymbTab_GetSymb(VetisProductMarkingClass_SymbTab, SIZEOFARRAY(VetisProductMarkingClass_SymbTab), p_mark->Cls, temp_buf);
																		n_mark.PutAttribSkipEmpty("class", temp_buf);
																		{
																			temp_buf = p_mark->Item;
																			XMLReplaceSpecSymb(temp_buf, "<>&");
																			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
																			n_mark.SetValue(temp_buf);
																		}
																	}
																}
															}
														}
													}
												}
												{
													SXml::WNode n_owner(srb, _xmlnst_vd("owner"));
													PutNonZeroGuid(n_owner, "bs", rAppBlk.IssuerId);
												}
											}
											else {
												SXml::WNode n_bat(srb, _xmlnst_vd("batch"));
												n_bat.PutInner(_xmlnst_vd("productType"), temp_buf.Z().Cat(p_req->Pi.ProductType));
												{
													SXml::WNode n_p(srb, _xmlnst_vd("product"));
													PutNonZeroGuid(n_p, "bs", p_req->Pi.Product.Guid);
												}
												{
													SXml::WNode n_p(srb, _xmlnst_vd("subProduct"));
													PutNonZeroGuid(n_p, "bs", p_req->Pi.SubProduct.Guid);
												}
												{
													SXml::WNode n_p(srb, _xmlnst_vd("productItem"));
													PutNonZeroGuid(n_p, "bs", p_req->Pi.Guid);
												}
												n_bat.PutInner(_xmlnst_vd("volume"), temp_buf.Z().Cat(p_req->VdRec.Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
												{
													SXml::WNode n_uom(srb, _xmlnst_vd("unit"));
													PutNonZeroGuid(n_uom, "bs", S_GUID(P_VetisGuid_Unit_Kg));
												}
												{
													SXml::WNode n_dop(srb, _xmlnst_vd("dateOfProduction"));
													SUniTime ut;
													ut.FromInt64(p_req->VdRec.ManufDateFrom);
													PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", ut);
													//ut.FromInt64(p_req->VdRec.ManufDateTo);
													//PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", ut);
												}
												{
													SXml::WNode n_dop(srb, _xmlnst_vd("expiryDate"));
													SUniTime ut;
													ut.FromInt64(p_req->VdRec.ExpiryFrom);
													PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", ut);
													//ut.FromInt64(p_req->VdRec.ExpiryTo);
													//PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", ut);
												}
												//n_bat.PutInner(SXml::nst("vet", "batchID"), /*temp_buf.Z().Cat(p_req->VdRec.)*/"");
												n_bat.PutInner(_xmlnst_vd("perishable"), "true");
												/*{
													SXml::WNode n_coo(srb, _xmlnst_vd("countryOfOrigin"));
													S_GUID coo_guid;
													coo_guid.FromStr(P_VetisGuid_Country_Ru);
													PutNonZeroGuid(n_coo, "bs", coo_guid);
												}*/
												/*if(p_req->Pi.Producing.getCount()) {
													SXml::WNode n_ml(srb, _xmlnst_vd("producerList"));
													{
														SXml::WNode n_m(srb, SXml::nst("ent", "producer"));
														{
															SXml::WNode n_me(srb, SXml::nst("ent", "enterprise"));
															PutNonZeroGuid(n_me, "bs", p_req->Pi.Producing.at(0)->Guid);
														}
														n_m.PutInner(SXml::nst("ent", "role"), "PRODUCER");
													}
												}*/
												{
													SXml::WNode n_org(srb, _xmlnst_vd("origin"));
													{
														SXml::WNode n_p(srb, _xmlnst_vd("productItem"));
														PutNonZeroGuid(n_p, "bs", p_req->Pi.Guid);
													}
													{
														SXml::WNode n_p(srb, _xmlnst_vd("country"));
														PutNonZeroGuid(n_p, "bs", S_GUID(P_VetisGuid_Country_Ru));
													}
													if(p_req->Pi.Producing.getCount()) {
														SXml::WNode n_p(srb, _xmlnst_vd("producer"));
														{
															SXml::WNode n_me(srb, _xmlnst_dt("enterprise"));
															PutNonZeroGuid(n_me, "bs", p_req->Pi.Producing.at(0)->Guid);
														}
													}
													/*{
														SXml::WNode n_p(srb, _xmlnst_vd("producer"));
														{
															SXml::WNode n_me(srb, _xmlnst_dt("enterprise"));
															PutNonZeroGuid(n_me, "bs", p_req->Pi.Producer.Guid);
														}
													}*/
												}
												n_bat.PutInner(_xmlnst_vd("lowGradeCargo"), "false");
												if(0) {
													{
														SXml::WNode n_pl(srb, _xmlnst_vd("packageList"));
														{
															SXml::WNode n_pckg(srb, _xmlnst_dt("package"));
															n_pckg.PutInner(_xmlnst_dt("level"), "6"); //<dt:level>6</dt:level>
															{
																SXml::WNode n_pt(srb, _xmlnst_dt("packingType"));
																//<bs:uuid>a7363144-e6a0-4af0-832f-0d403fdeb761</bs:uuid>
																//<dt:globalID>BE</dt:globalID>
															}
															n_pckg.PutInner(_xmlnst_dt("quantity"), ""); // <dt:quantity>10</dt:quantity>
														}
													}
												}
												{
													SXml::WNode n_owner(srb, _xmlnst_vd("owner"));
													PutNonZeroGuid(n_owner, "bs", rAppBlk.IssuerId);
												}
											}
										}
									}
								}
							}
							{
								SXml::WNode n_dr(srb, SXml::nst("merc", "discrepancyReport"));
								n_dr.PutAttrib("for", p_sd_ident);
								//n_dr.PutInner(_xmlnst_vd("issueSeries"), "is_s");
								//n_dr.PutInner(_xmlnst_vd("issueNumber"), "is_n");
								//n_dr.PutInner(_xmlnst_vd("issueDate"), temp_buf.Z().Cat(p_req->VdRec.WayBillDate, DATF_ISO8601CENT));
								{
									SXml::WNode n_reason(srb, _xmlnst_vd("reason"));
									PPLoadString(PPSTR_HASHTOKEN_C, (p_req->VdRec.Flags & VetisVetDocument::fDiscrepancyLack) ? 
										PPHSC_VETIS_LOSSES : PPHSC_VETIS_UNRECORDEDBALANCE, temp_buf);
									n_reason.PutInner(_xmlnst_vd("name"), EncText(temp_buf.Transf(CTRANSF_INNER_TO_UTF8)));
								}
								//n_dr.PutInner(_xmlnst_vd("description"), "Сказал же: неучтенный остаток"); // Неучтенный остаток
							}
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signProcessOutgoingConsignment) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("bs"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("merc"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("apldef"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("apl"),    InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("vd"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, 0));
						{
							SXml::WNode n_data(srb, SXml::nst("apl", "data"));
							{
								const VetisPrepareOutgoingConsignmentRequest * p_req = static_cast<const VetisPrepareOutgoingConsignmentRequest *>(rAppBlk.P_AppParam);
								const VetisVetDocument & r_org_doc = p_req->OrgDoc;
								const VetisBatch & r_org_bat = DEREFPTROR(r_org_doc.P_CertifiedBatch, r_org_doc.CertifiedConsignment.Batch);
								SXml::WNode n_req(srb, SXml::nst("merc", "prepareOutgoingConsignmentRequest"));
								n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
								{
									SXml::WNode n_n2(srb, SXml::nst("merc", "initiator"));
									n_n2.PutInner(_xmlnst_vd("login"), rAppBlk.User);
								}
								{
									SXml::WNode n_n2(srb, SXml::nst("merc", "delivery"));
									LDATETIME dtm;
									dtm.Set(getcurdate_(), ZEROTIME);
									n_n2.PutInner(_xmlnst_vd("deliveryDate"), temp_buf.Z().Cat(dtm, DATF_ISO8601CENT, 0));
									{
										SXml::WNode n_c(srb, _xmlnst_vd("consignor"));
										PutSingleGuidEntity(srb, "dt", "businessEntity", "bs", p_req->FromBusinessEntGuid);
										PutSingleGuidEntity(srb, "dt", "enterprise", "bs", p_req->FromEnterpiseGuid);
									}
									{
										SXml::WNode n_c(srb, _xmlnst_vd("consignee"));
										PutSingleGuidEntity(srb, "dt", "businessEntity", "bs", p_req->ToBusinessEntGuid);
										PutSingleGuidEntity(srb, "dt", "enterprise", "bs", p_req->ToEnterpiseGuid);
									}
									{
										SXml::WNode n_c(srb, _xmlnst_vd("consignment"));
										n_c.PutAttrib("id",  temp_buf.Z().Cat("id").Cat(p_req->VdRec.LinkBillID));
										// @v11.4.8 @todo Здесь, кажется, надо развернутый productItem вставлять!
										n_c.PutInner(_xmlnst_vd("volume"), temp_buf.Z().Cat(p_req->VdRec.Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
										{
											SXml::WNode n_uom(srb, _xmlnst_vd("unit"));
											PutNonZeroUuid(n_uom, "bs", r_org_bat.Unit.Uuid);
											PutNonZeroGuid(n_uom, "bs", r_org_bat.Unit.Guid);
										}
										// @v11.4.3 {
										if(p_req->OrgDoc.P_CertifiedBatch && p_req->OrgDoc.P_CertifiedBatch->PackageList.getCount()) {
											/*
												<vd:packageList>
													<dt:package>
														<dt:level>4</dt:level>
														<dt:packingType>
															<bs:guid>fedf4328-053c-11e1-99b4-d8d385fbc9e8</bs:guid>
														</dt:packingType>
														<dt:quantity>10</dt:quantity>
													</dt:package>
													<dt:package>
														<dt:level>2</dt:level>
														<dt:packingType>
															<bs:guid>f0b0ec9b-8341-4e95-bc0e-80898be598cb</bs:guid>
														</dt:packingType>
														<dt:quantity>20</dt:quantity>
														<dt:productMarks class="EAN128">7456873456-147885</dt:productMarks>
														<dt:productMarks class="UNDEFINED">custom marking</dt:productMarks>
													</dt:package>
												</vd:packageList>
											*/
											SXml::WNode n_pl(srb, _xmlnst_vd("packageList"));
											for(uint plidx = 0; plidx < p_req->OrgDoc.P_CertifiedBatch->PackageList.getCount(); plidx++) {
												const VetisPackage * p_item = p_req->OrgDoc.P_CertifiedBatch->PackageList.at(plidx);
												if(p_item) {
													SXml::WNode n_pi(srb, _xmlnst_dt("package"));
													n_pi.PutInner(_xmlnst_dt("level"), temp_buf.Z().Cat(p_item->Level));
													if(!!p_item->PackingType.Guid) {
														PutSingleGuidEntity(srb, "dt", "packingType", "bs", p_item->PackingType.Guid);
													}
													if(p_item->Quantity > 0) {
														n_pi.PutInner(_xmlnst_dt("quantity"), temp_buf.Z().Cat(p_item->Quantity));
													}
													if(p_item->ProductMarks.getCount()) {
														for(uint pmidx = 0; pmidx < p_item->ProductMarks.getCount(); pmidx++) {
															const VetisProductMarks * p_pm = p_item->ProductMarks.at(pmidx);
															if(p_pm) {
																SXml::WNode n_pm(srb, _xmlnst_dt("productMarks"));
																if(SIntToSymbTab_GetSymb(VetisProductMarkingClass_SymbTab, SIZEOFARRAY(VetisProductMarkingClass_SymbTab), p_pm->Cls, temp_buf))
																	n_pm.PutAttrib("class", temp_buf);
																n_pm.SetValue(p_pm->Item);
															}
														}
													}
												}
											}
										}
										// } @v11.4.3 
										{
											SXml::WNode n_stk(srb, _xmlnst_vd("sourceStockEntry"));
											n_stk.PutInner(_xmlnst_bs_uuid(), temp_buf.Z().Cat(p_req->StockEntryUuid, S_GUID::fmtIDL|S_GUID::fmtLower));
											n_stk.PutInner(_xmlnst_bs_guid(), temp_buf.Z().Cat(p_req->StockEntryGuid, S_GUID::fmtIDL|S_GUID::fmtLower));
										}
									}
									if(p_req->Transp.TransportType) {
										SXml::WNode n_tr(srb, _xmlnst_vd("transportInfo"));
										temp_buf.Z().Cat(static_cast<long>(p_req->Transp.TransportType));
										n_tr.PutInner(_xmlnst_vd("transportType"), temp_buf);
										if(!p_req->Transp.TransportNumber.IsEmpty()) {
											SXml::WNode n_tn(srb, _xmlnst_vd("transportNumber"));
											PutNonEmptyText(n_tn, "vd", "vehicleNumber", p_req->Transp.TransportNumber.VehicleNumber);
											PutNonEmptyText(n_tn, "vd", "trailerNumber", p_req->Transp.TransportNumber.TrailerNumber);
											PutNonEmptyText(n_tn, "vd", "containerNumber", p_req->Transp.TransportNumber.ContainerNumber);
											PutNonEmptyText(n_tn, "vd", "wagonNumber", p_req->Transp.TransportNumber.WagonNumber);
											PutNonEmptyText(n_tn, "vd", "shipName", p_req->Transp.TransportNumber.ShipName);
											PutNonEmptyText(n_tn, "vd", "flightNumber", p_req->Transp.TransportNumber.FlightNumber);
										}
									}
									// @v11.5.10 {
									if(p_req->OrgDoc.CertifiedConsignment.TransportStorageType_id && SIntToSymbTab_GetSymb(
										VetisTranspStorageType_SymbTab, SIZEOFARRAY(VetisTranspStorageType_SymbTab), p_req->OrgDoc.CertifiedConsignment.TransportStorageType_id, temp_buf)) {
										n_n2.PutInner(_xmlnst_vd("transportStorageType"), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
									}
									else // } @v11.5.10
										n_n2.PutInner(_xmlnst_vd("transportStorageType"), (temp_buf = p_req->TranspStorageType).SetIfEmpty("FROZEN"));
									{
										SXml::WNode n_af(srb, _xmlnst_vd("accompanyingForms"));
										{
											SXml::WNode n_wb(srb, _xmlnst_vd("waybill"));
											//n_wb.PutInner(_xmlnst_vd("issueSeries"), 0);
											n_wb.PutInner(_xmlnst_vd("issueNumber"), (temp_buf = p_req->VdRec.WayBillNumber).Transf(CTRANSF_INNER_TO_UTF8));
											n_wb.PutInner(_xmlnst_vd("issueDate"), temp_buf.Z().Cat(p_req->VdRec.WayBillDate, DATF_ISO8601CENT));
											n_wb.PutInner(_xmlnst_vd("type"), "1");
										}
										{
											SXml::WNode n_vc(srb, _xmlnst_vd("vetCertificate"));
											n_vc.PutAttrib("for",  temp_buf.Z().Cat("id").Cat(p_req->VdRec.LinkBillID));
											{
												SXml::WNode n_auth(srb, _xmlnst_vd("authentication"));
												{
													SXml::WNode n_prp(srb, _xmlnst_vd("purpose"));
													// для хранения и реализации
													//n_prp.PutInner(_xmlnst_bs_guid(), "5b911047-e089-11e1-bcf3-b499babae7ea");
													// реализация после проведения ветеринарно-санитарной экспертизы
													// n_prp.PutInner(_xmlnst_bs_guid(), "d8fb4c77-f461-4eeb-9f4c-31897ddde769");
													// реализация в пищу людям
													S_GUID purpose_guid;
													temp_buf.Z();
													if(p_req->VdRec.LinkGoodsID && p_ref->Ot.GetTagGuid(PPOBJ_GOODS, p_req->VdRec.LinkGoodsID, PPTAG_GOODS_VETISPURPOSE, purpose_guid) > 0)
														purpose_guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
													else
														temp_buf = "0778b8cb-f49d-4ed9-88b9-5f70af00a211";
													n_prp.PutInner(_xmlnst_bs_guid(), temp_buf);
												}
												n_vc.PutInner(_xmlnst_vd("cargoInspected"), "true");
												{
													// @v11.8.5 символ результата экспертизы теперь берется из сертификата (ранее был константой)
													// @v11.8.11 SXml::WNode n_auth(srb, _xmlnst_vd("authentication"));
													const char * p_ce_symb = SIntToSymbTab_GetSymbPtr(
														VetAuthCargoExpertized_SymbTab, SIZEOFARRAY(VetAuthCargoExpertized_SymbTab), r_org_doc.Authentication.CargoExpertized);
													n_vc.PutInner(_xmlnst_vd("cargoExpertized"), p_ce_symb ? p_ce_symb : "VSERAW"); // @v11.6.6 VSEFULL-->VSERAW // @v11.8.5
												}
												PPLoadText(PPTXT_VETISLOCATIONPROSPERITYISOK, temp_buf);
												temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
												n_vc.PutInner(_xmlnst_vd("locationProsperity"), temp_buf);
												//PPLoadText(PPTXT_VETIS_SPCMARK_SRCWARENAME, temp_buf);
												//temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
												//n_vc.PutInner(_xmlnst_vd("specialMarks"), temp_buf);
												// @v11.0.11 {
												if(p_req->P_RegionRules && p_req->P_RegionRules->getCount()) {
													//SXml::WNode n_r13(srb, _xmlnst_vd("r13nClause"));
													for(uint rlidx = 0; rlidx < p_req->P_RegionRules->getCount(); rlidx++) {
														const VetisRouteSectionR13nRules * p_r = p_req->P_RegionRules->at(rlidx);
														if(p_r) {
															for(uint rl2idx = 0; rl2idx < p_r->RuleList.getCount(); rl2idx++) {
																const VetisRegionalizationShippingRule * p_r2 = p_r->RuleList.at(rl2idx);
																if(p_r2) {
																	for(uint rridx = 0; rridx < p_r2->ReqList.getCount(); rridx++) {
																		const VetisRegionalizationRequirement * p_rr = p_r2->ReqList.at(rridx);
																		if(p_rr) {
																			for(uint cgidx = 0; cgidx < p_rr->ConditionGroup.getCount(); cgidx++) {
																				const VetisRegionalizationCondition * p_rc = p_rr->ConditionGroup.at(cgidx);
																				if(p_rc && (!!p_rc->Guid/* @v11.4.1 || !!p_rc->Uuid*/)) { // @v11.4.0 (&& (!!p_rc->Guid || !!p_rc->Uuid))
																					SXml::WNode n_r13(srb, _xmlnst_vd("r13nClause"));
																					{
																						SXml::WNode n_rc(srb, _xmlnst_vd("condition"));
																						PutNonZeroGuid(n_rc, "bs", p_rc->Guid);
																						// @v11.4.1 PutNonZeroUuid(n_rc, "bs", p_rc->Uuid);
																						//n_rc.PutInner(_xmlnst_bs_uuid(), temp_buf.Z().Cat(p_rc->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
																						//(temp_buf = p_rc->Text).Transf(CTRANSF_INNER_TO_UTF8);
																						//XMLReplaceSpecSymb(temp_buf, "<>&");
																						//n_rc.PutInner(_xmlnst_dt("text"), temp_buf);
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
												// } @v11.0.11 
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
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signGetVetDocumentList) {
			assert(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0));
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("xs"),      InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
			n_env.PutAttrib(SXml::nst_xmlns("xsi"),     InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("bs"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("merc"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("apldef"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("apl"),    InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("vd"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						// @v11.9.11 n_app.PutInner(SXml::nst("apl", "serviceId"), (rAppBlk.VetisSvcVer == 2) ? "mercury-g2b.service:2.1" : "mercury-g2b.service"); // @note mercury-g2b.service:2.1 not mercury-g2b.service:2.0
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId()); // @v11.9.11 
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisGetVetDocumentListRequest * p_req = static_cast<const VetisGetVetDocumentListRequest *>(rAppBlk.P_AppParam);
							SXml::WNode n_req(srb, SXml::nst("merc", "getVetDocumentListRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							PutListOptions(srb, "bs", p_req->ListOptions);
							if(SIntToSymbTab_GetSymb(VetisVetDocType_SymbTab, SIZEOFARRAY(VetisVetDocType_SymbTab), p_req->DocType, temp_buf))
								n_req.PutInnerSkipEmpty(_xmlnst_vd("vetDocumentType"), temp_buf);
							if(SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), p_req->DocStatus, temp_buf))
								n_req.PutInnerSkipEmpty(_xmlnst_vd("vetDocumentStatus"), temp_buf);
							{
								SXml::WNode n_n2(srb, _xmlnst_vd("issueDateInterval"));
								n_n2.PutInner(SXml::nst("bs", "beginDate"), temp_buf.Z().Cat(p_req->Period.Start, DATF_ISO8601CENT, 0));
								n_n2.PutInner(SXml::nst("bs", "endDate"), temp_buf.Z().Cat(p_req->Period.Finish, DATF_ISO8601CENT, 0));
							}
							n_req.PutInner(_xmlnst_dt("enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signProcessIncomingConsignment) {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("xs"),      InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
			n_env.PutAttrib(SXml::nst_xmlns("xsi"),     InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("bs"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("merc"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("apldef"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("apl"),    InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("vd"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisProcessIncomingConsignmentRequest * p_req = static_cast<const VetisProcessIncomingConsignmentRequest *>(rAppBlk.P_AppParam);
							const VetisVetDocument & r_doc = p_req->Doc;
							const VetisBatch & r_bat = r_doc.CertifiedConsignment.Batch;
							SXml::WNode n_req(srb, SXml::nst("merc", "processIncomingConsignmentRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "delivery"));
								LDATETIME dtm;
								dtm.Set(r_doc.IssueDate, ZEROTIME);
								n_n2.PutInner(_xmlnst_vd("deliveryDate"), temp_buf.Z().Cat(dtm, DATF_ISO8601CENT, 0));
								{
									SXml::WNode n_c(srb, _xmlnst_vd("consignor"));
									PutSingleGuidEntity(srb, "dt", "businessEntity", "bs", r_doc.CertifiedConsignment.Consignor.BusinessEntity.Guid);
									PutSingleGuidEntity(srb, "dt", "enterprise", "bs", r_doc.CertifiedConsignment.Consignor.Enterprise.Guid);
								}
								{
									SXml::WNode n_c(srb, _xmlnst_vd("consignee"));
									PutSingleGuidEntity(srb, "dt", "businessEntity", "bs", r_doc.CertifiedConsignment.Consignee.BusinessEntity.Guid);
									PutSingleGuidEntity(srb, "dt", "enterprise", "bs", r_doc.CertifiedConsignment.Consignee.Enterprise.Guid);
								}
								{
									SXml::WNode n_c(srb, _xmlnst_vd("consignment"));
									n_c.PutInner(_xmlnst_vd("productType"), temp_buf.Z().Cat(r_bat.ProductType));
									{
										SXml::WNode n_p(srb, _xmlnst_vd("product"));
										n_p.PutInner(_xmlnst_bs_guid(), temp_buf.Z().Cat(r_bat.Product.Guid, S_GUID::fmtIDL|S_GUID::fmtLower));
									}
									{
										SXml::WNode n_p(srb, _xmlnst_vd("subProduct"));
										n_p.PutInner(_xmlnst_bs_guid(), temp_buf.Z().Cat(r_bat.SubProduct.Guid, S_GUID::fmtIDL|S_GUID::fmtLower));
									}
									{
										SXml::WNode n_p(srb, _xmlnst_vd("productItem"));
										PutNonZeroGuid(n_p, "bs", r_bat.ProductItem.Guid);
										temp_buf = r_bat.ProductItem.Name;
										if(temp_buf.NotEmptyS()) {
											// @v12.4.6 XMLReplaceSpecSymb(temp_buf, "<>&");
											temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
											n_p.PutInner(_xmlnst_dt("name"), EncText(temp_buf)); // @v12.4.6 EncText
										}
									}
									n_c.PutInner(_xmlnst_vd("volume"), temp_buf.Z().Cat(r_bat.Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
									{
										SXml::WNode n_uom(srb, _xmlnst_vd("unit"));
										PutNonZeroUuid(n_uom, "bs", r_bat.Unit.Uuid);
										PutNonZeroGuid(n_uom, "bs", r_bat.Unit.Guid);
									}
									if(!!r_bat.DateOfProduction.FirstDate || !!r_bat.DateOfProduction.SecondDate) {
										SXml::WNode n_dt(srb, _xmlnst_vd("dateOfProduction"));
										PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", r_bat.DateOfProduction.FirstDate);
										PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", r_bat.DateOfProduction.SecondDate);
									}
									if(!!r_bat.ExpiryDate.FirstDate || !!r_bat.ExpiryDate.SecondDate) {
										SXml::WNode n_dt(srb, _xmlnst_vd("expiryDate"));
										PutGoodsDate(srb, _xmlnst_vd("firstDate"), "dt", r_bat.ExpiryDate.FirstDate);
										PutGoodsDate(srb, _xmlnst_vd("secondDate"), "dt", r_bat.ExpiryDate.SecondDate);
									}
									if(r_bat.BatchIdList.getCount()) {
										for(uint bipos = 0; r_bat.BatchIdList.get(&bipos, temp_buf);) {
											n_c.PutInner(_xmlnst_vd("batchID"), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
										}
									}
									/* @v10.5.6 if(r_bat.BatchID.NotEmpty()) { // @v10.5.5
										//<vd:batchID>BN1529656417</vd:batchID>
										n_c.PutInner(_xmlnst_vd("batchID"), (temp_buf = r_bat.BatchID).Transf(CTRANSF_INNER_TO_UTF8));
									} */
									n_c.PutInner(_xmlnst_vd("perishable"), temp_buf.Z().Cat(STextConst::GetBool(r_bat.Flags & VetisBatch::fPerishable)));
									{
										SXml::WNode n_org(srb, _xmlnst_vd("origin"));
										{
											SXml::WNode n_country(srb, _xmlnst_vd("country"));
											PutNonZeroUuid(n_country, "bs", r_bat.Origin.Country.Uuid);
											PutNonZeroGuid(n_country, "bs", r_bat.Origin.Country.Guid);
										}
										for(uint pridx = 0; pridx < r_bat.Origin.Producer.getCount(); pridx++) {
											const VetisProducer * p_pl_item = r_bat.Origin.Producer.at(pridx);
											SXml::WNode n_producer(srb, _xmlnst_vd("producer"));
											{
												SXml::WNode n_ent(srb, _xmlnst_dt("enterprise"));
												PutNonZeroUuid(n_ent, "bs", p_pl_item->Uuid);
												PutNonZeroGuid(n_ent, "bs", p_pl_item->Guid);
												PutNonEmptyText(n_ent, "dt", "name", EncText(p_pl_item->Name));
											}
											const char * p_role = (p_pl_item->Role == VetisProducer::rolePRODUCER) ? "PRODUCER" : "UNKNOWN";
											if(p_role)
												n_producer.PutInner(_xmlnst_dt("role"), p_role);
										}
									}
									{

									}
									n_c.PutInner(_xmlnst_vd("lowGradeCargo"), temp_buf.Z().Cat(STextConst::GetBool(r_bat.Flags & VetisBatch::fLowGradeCargo)));
									if(r_bat.PackageList.getCount()) {
										SXml::WNode n_pl(srb, _xmlnst_vd("packageList"));
										for(uint plidx = 0; plidx < r_bat.PackageList.getCount(); plidx++) {
											const VetisPackage * p_pckg = r_bat.PackageList.at(plidx);
											if(p_pckg) {
												SXml::WNode n_pckg(srb, _xmlnst_dt("package"));
												n_pckg.PutInner(_xmlnst_dt("level"), temp_buf.Z().Cat(p_pckg->Level));
												{
													SXml::WNode n_pt(srb, _xmlnst_dt("packingType"));
													PutNonZeroUuid(n_pt, "bs", p_pckg->PackingType.Uuid);
													PutNonZeroGuid(n_pt, "bs", p_pckg->PackingType.Guid);
													PutNonEmptyText(n_pt, "dt", "name", EncText(p_pckg->PackingType.Name));
												}
												n_pckg.PutInner(_xmlnst_dt("quantity"), temp_buf.Z().Cat(p_pckg->Quantity));
												if(p_pckg->ProductMarks.getCount()) {
													for(uint markidx = 0; markidx < p_pckg->ProductMarks.getCount(); markidx++) {
														const VetisProductMarks * p_mark = p_pckg->ProductMarks.at(markidx);
														if(p_mark) {
															SXml::WNode n_mark(srb, _xmlnst_dt("productMarks"));
															SIntToSymbTab_GetSymb(VetisProductMarkingClass_SymbTab, SIZEOFARRAY(VetisProductMarkingClass_SymbTab), p_mark->Cls, temp_buf);
															n_mark.PutAttribSkipEmpty("class", temp_buf);
															{
																temp_buf = p_mark->Item;
																XMLReplaceSpecSymb(temp_buf, "<>&");
																temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
																n_mark.SetValue(temp_buf);
															}
														}
													}
												}
											}
										}
									}
									/*if(r_bat.PackingAmount > 0) {
										if(r_bat.PackingList.getCount()) {
											SXml::WNode n_pl(srb, _xmlnst_vd("packingList"));
											for(uint pli = 0; pli < r_bat.PackingList.getCount(); pli++) {
												const VetisNamedGenericVersioningEntity * p_pl_item = r_bat.PackingList.at(pli);
												if(p_pl_item) {
													SXml::WNode n_pf(srb, _xmlnst_dt("packingForm"));
													PutNonZeroUuid(n_pf, "bs", p_pl_item->Uuid);
													PutNonZeroGuid(n_pf, "bs", p_pl_item->Guid);
													temp_buf = p_pl_item->Name;
													PutNonEmptyText(n_pf, "dt", "name", EncText(temp_buf));
												}
											}
										}
										n_c.PutInner(_xmlnst_vd("packingAmount"), temp_buf.Z().Cat(r_bat.PackingAmount));
									}*/
								}
								if(!r_doc.CertifiedConsignment.Broker.Guid.IsZero()) {
									SXml::WNode n_broker(srb, _xmlnst_vd("broker"));
									PutNonZeroUuid(n_broker, "bs", r_doc.CertifiedConsignment.Broker.Uuid);
									PutNonZeroGuid(n_broker, "bs", r_doc.CertifiedConsignment.Broker.Guid);
								}
								{
									const VetisTransportInfo * p_trinfo = 0;
									const TSCollection <VetisCargoReloadingPoint> & r_crpl = r_doc.CertifiedConsignment.CargoReloadingPointList;
									const TSCollection <VetisShipmentRoutePoint> & r_srpl = r_doc.CertifiedConsignment.RoutePointList;
									if(r_crpl.getCount()) {
										p_trinfo = &r_crpl.at(r_crpl.getCount()-1)->NextTransport;
									}
									else {
										uint srplp = r_srpl.getCount();
										if(srplp) do {
											const VetisShipmentRoutePoint * p_rp = r_srpl.at(--srplp);
											if(p_rp && (p_rp->Flags & p_rp->fTransShipment) && p_rp->P_NextTransport) {
												p_trinfo = p_rp->P_NextTransport;
											}
										} while(!p_trinfo && srplp);
									}
									SETIFZ(p_trinfo, &r_doc.CertifiedConsignment.TransportInfo);
									{
										SXml::WNode n_tr(srb, _xmlnst_vd("transportInfo"));
										if(p_trinfo->TransportType) {
											temp_buf.Z().Cat(p_trinfo->TransportType);
											n_tr.PutInner(_xmlnst_vd("transportType"), temp_buf);
										}
										if(!p_trinfo->TransportNumber.IsEmpty()) {
											SXml::WNode n_tn(srb, _xmlnst_vd("transportNumber"));
											XMLReplaceSpecSymb((temp_buf = p_trinfo->TransportNumber.ContainerNumber), "<>&/");
											PutNonEmptyText(n_tn, "vd", "containerNumber", temp_buf);
											XMLReplaceSpecSymb((temp_buf = p_trinfo->TransportNumber.VehicleNumber), "<>&/");
											PutNonEmptyText(n_tn, "vd", "vehicleNumber", temp_buf);
											XMLReplaceSpecSymb((temp_buf = p_trinfo->TransportNumber.TrailerNumber), "<>&/");
											PutNonEmptyText(n_tn, "vd", "trailerNumber", temp_buf);
											XMLReplaceSpecSymb((temp_buf = p_trinfo->TransportNumber.WagonNumber), "<>&/");
											PutNonEmptyText(n_tn, "vd", "wagonNumber", temp_buf);
											XMLReplaceSpecSymb((temp_buf = p_trinfo->TransportNumber.ShipName), "<>&/");
											PutNonEmptyText(n_tn, "vd", "shipName", temp_buf);
											XMLReplaceSpecSymb((temp_buf = p_trinfo->TransportNumber.FlightNumber), "<>&/");
											PutNonEmptyText(n_tn, "vd", "flightNumber", temp_buf);
										}
									}
									if(r_doc.CertifiedConsignment.TransportStorageType_id) {
										if(SIntToSymbTab_GetSymb(VetisTranspStorageType_SymbTab, SIZEOFARRAY(VetisTranspStorageType_SymbTab),
											r_doc.CertifiedConsignment.TransportStorageType_id, temp_buf)) {
											n_n2.PutInner(_xmlnst_vd("transportStorageType"), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
										}
									}
								}
								{
									SXml::WNode n_af(srb, _xmlnst_vd("accompanyingForms"));
									const VetisVetDocument::ReferencedDocument * p_waybill_ref_doc = r_doc.GetWayBillRef();
									if(p_waybill_ref_doc) {
										SXml::WNode n_wb(srb, _xmlnst_vd("waybill"));
										if(p_waybill_ref_doc->IssueSeries.NotEmpty()) {
											n_wb.PutInner(_xmlnst_vd("issueSeries"), (temp_buf = p_waybill_ref_doc->IssueSeries).Transf(CTRANSF_INNER_TO_UTF8));
										}
										if(p_waybill_ref_doc->IssueNumber.NotEmpty())
											n_wb.PutInner(_xmlnst_vd("issueNumber"), (temp_buf = p_waybill_ref_doc->IssueNumber).Transf(CTRANSF_INNER_TO_UTF8));
										if(checkdate(p_waybill_ref_doc->IssueDate))
											n_wb.PutInner(_xmlnst_vd("issueDate"), temp_buf.Z().Cat(p_waybill_ref_doc->IssueDate, DATF_ISO8601CENT));
										if(p_waybill_ref_doc->DocumentType > 0) {
											n_wb.PutInner(_xmlnst_vd("type"), temp_buf.Z().Cat(p_waybill_ref_doc->DocumentType));
										}
										if(0/*r_doc.CertifiedConsignment.CargoReloadingPointList.getCount()*/) {
											SXml::WNode n_crpl(srb, _xmlnst_vd("cargoReloadingPointList"));
											for(uint crpidx = 0; crpidx < r_doc.CertifiedConsignment.CargoReloadingPointList.getCount(); crpidx++) {
												const VetisCargoReloadingPoint * p_crp = r_doc.CertifiedConsignment.CargoReloadingPointList.at(crpidx);
												if(p_crp) {
													SXml::WNode n_crp(srb, _xmlnst_vd("cargoReloadingPoint"));
													PutNonEmptyText(n_crp, "dt", "name", EncText(p_crp->Name));
													if(!p_crp->NextTransport.TransportNumber.IsEmpty()) {
														SXml::WNode n_tr(srb, _xmlnst_vd("transportInfo"));
														if(p_crp->NextTransport.TransportType) {
															temp_buf.Z().Cat(p_crp->NextTransport.TransportType);
															n_tr.PutInner(_xmlnst_vd("transportType"), temp_buf);
														}
														if(!p_crp->NextTransport.TransportNumber.IsEmpty()) {
															SXml::WNode n_tn(srb, _xmlnst_vd("transportNumber"));
															PutNonEmptyText(n_tn, "vd", "vehicleNumber", p_crp->NextTransport.TransportNumber.VehicleNumber);
															PutNonEmptyText(n_tn, "vd", "trailerNumber", p_crp->NextTransport.TransportNumber.TrailerNumber);
															PutNonEmptyText(n_tn, "vd", "wagonNumber", p_crp->NextTransport.TransportNumber.WagonNumber);
															PutNonEmptyText(n_tn, "vd", "shipName", p_crp->NextTransport.TransportNumber.ShipName);
															PutNonEmptyText(n_tn, "vd", "flightNumber", p_crp->NextTransport.TransportNumber.FlightNumber);
														}
													}
												}
											}
										}
									}
									{
										SXml::WNode n_vc(srb, _xmlnst_vd("vetCertificate"));
										n_vc.PutInner(_xmlnst_bs_uuid(), temp_buf.Z().Cat(r_doc.Uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
									}
								}
							}
							{
								SXml::WNode n_n2(srb, SXml::nst("merc", "deliveryFacts"));
								n_n2.PutInner(_xmlnst_vd("vetCertificatePresence"), "ELECTRONIC");
								{
									SXml::WNode n_c(srb, _xmlnst_vd("docInspection"));
									{
										SXml::WNode n_i(srb, _xmlnst_vd("responsible"));
										n_i.PutInner(_xmlnst_vd("login"), rAppBlk.User);
									}
									n_c.PutInner(_xmlnst_vd("result"), "CORRESPONDS");
								}
								{
									SXml::WNode n_c(srb, _xmlnst_vd("vetInspection"));
									{
										SXml::WNode n_i(srb, _xmlnst_vd("responsible"));
										n_i.PutInner(_xmlnst_vd("login"), rAppBlk.User);
									}
									n_c.PutInner(_xmlnst_vd("result"), "CORRESPONDS");
								}
								n_n2.PutInner(_xmlnst_vd("decision"), "ACCEPT_ALL");
							}
							/*
							{
								SXml::WNode n_n2(srb, "discrepancyReport");
								//<discrepancyReport xmlns:d7p1="http://api.vetrf.ru/schema/cdm/mercury/vet-document">
								n_n2.PutInner(SXml::nst("d7p1", "issueDate"), temp_buf.Z().Cat(getcurdate_(), DATF_ISO8601CENT));
								{
									SXml::WNode n_reason(srb, SXml::nst("d7p1", "reason"), temp_buf = "abc");
								}
								n_n2.PutInner(SXml::nst("d7p1", "description"), temp_buf = "abc");
							}
							*/
						}
					}
				}
			}
		}
		else if(rAppBlk.P_AppParam->Sign == VetisApplicationData::signGetVetDocumentChangesList) {
			assert(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0));
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("xs"),      InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
			n_env.PutAttrib(SXml::nst_xmlns("xsi"),     InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("bs"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("merc"),   InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("apldef"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("apl"),    InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("vd"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("apldef", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("apldef", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("apl", "application"));
						n_app.PutInner(SXml::nst("apl", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("apl", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("apl", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("apl", "data"));
						{
							const VetisGetVetDocumentChangesListRequest * p_req = static_cast<const VetisGetVetDocumentChangesListRequest *>(rAppBlk.P_AppParam);
							SXml::WNode n_req(srb, SXml::nst("merc", "getVetDocumentChangesListRequest"));
							n_req.PutInner(SXml::nst("merc", "localTransactionId"), temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
							PutInitiator(srb, "merc", "vd", rAppBlk.User);
							PutListOptions(srb, "bs", p_req->ListOptions);
							{
								SXml::WNode n_n2(srb, SXml::nst("bs", "updateDateInterval"));
								n_n2.PutInner(SXml::nst("bs", "beginDate"), temp_buf.Z().Cat(p_req->Period.Start, DATF_ISO8601CENT, 0));
								n_n2.PutInner(SXml::nst("bs", "endDate"), temp_buf.Z().Cat(p_req->Period.Finish, DATF_ISO8601CENT, 0));
							}
							n_req.PutInner(_xmlnst_dt("enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
							if(SIntToSymbTab_GetSymb(VetisVetDocType_SymbTab, SIZEOFARRAY(VetisVetDocType_SymbTab), p_req->DocType, temp_buf))
								n_req.PutInnerSkipEmpty(_xmlnst_vd("vetDocumentType"), temp_buf);
							if(SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), p_req->DocStatus, temp_buf))
								n_req.PutInnerSkipEmpty(_xmlnst_vd("vetDocumentStatus"), temp_buf);
						}
					}
				}
			}
		}
		else {
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"),      InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			n_env.PutAttrib(SXml::nst_xmlns("app"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
			n_env.PutAttrib(SXml::nst_xmlns("v2"),      InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("v21"),     InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "submitApplicationRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("ws", "apiKey"), temp_buf);
					{
						SXml::WNode n_app(srb, SXml::nst("app", "application"));
						n_app.PutInner(SXml::nst("app", "applicationId"), VGuidToStr(rAppBlk.ApplicationId, temp_buf));
						n_app.PutInner(SXml::nst("app", "serviceId"), rAppBlk.GetServiceId());
						n_app.PutInner(SXml::nst("app", "issuerId"), VGuidToStr(rAppBlk.IssuerId, temp_buf));
						n_app.PutInner(SXml::nst("app", "issueDate"), temp_buf.Z().Cat(rAppBlk.IssueDate, DATF_ISO8601CENT, /*TIMF_TIMEZONE*/0));
						SXml::WNode n_data(srb, SXml::nst("app", "data"));
						switch(rAppBlk.P_AppParam->Sign) {
							case VetisApplicationData::signGetStockEntryByUUID:
								{
									const VetisGetStockEntryByUUIDRequest * p_req = static_cast<const VetisGetStockEntryByUUIDRequest *>(rAppBlk.P_AppParam);
									SXml::WNode n_req(srb, "getStockEntryByUuidRequest");
									n_req.PutAttrib(SXml::nst_xmlns("sch"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
									else
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document"));
									n_req.PutAttrib(SXml::nst_xmlns("sh"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/shipment"));
									n_req.PutAttrib(SXml::nst_xmlns("ws"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
									n_req.PutAttrib(SXml::nst_xmlns("app"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
									n_req.PutAttrib(SXml::nst_xmlns("co"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/common"));
									n_req.PutAttrib(SXml::nst_xmlns("ent"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/cerberus/enterprise"));
									n_req.PutAttrib(SXml::nst_xmlns("pr"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/production"));
									n_req.PutAttrib(SXml::nst_xmlns("ik"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/ikar"));
									n_req.PutAttrib(SXml::nst_xmlns("bs"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
									n_req.PutAttrib(SXml::nst_xmlns("dt"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
									else
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/applications"));
									n_req.PutInner("localTransactionId", temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
									PutInitiator(srb, 0, (rAppBlk.VetisSvcVer_ >= SVerT(2, 0)) ? "vd" : "co", rAppBlk.User);
									//PutListOptions(srb, "bs", p_req->ListOptions);
									{
										SXml::WNode n_n2(srb, _xmlnst_bs_uuid(), temp_buf.Z().Cat(p_req->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
									}
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutInner(_xmlnst_dt("enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
									else
										n_req.PutInner(SXml::nst("ent", "enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
								}
								break;
							case VetisApplicationData::signGetVetDocumentByUuid:
								{
									/*
										<SOAP-ENV:Envelope 
											xmlns:dt="http://api.vetrf.ru/schema/cdm/dictionary/v2" 
											xmlns:bs="http://api.vetrf.ru/schema/cdm/base" 
											xmlns:merc="http://api.vetrf.ru/schema/cdm/mercury/g2b/applications/v2" 
											xmlns:apldef="http://api.vetrf.ru/schema/cdm/application/ws-definitions" 
											xmlns:apl="http://api.vetrf.ru/schema/cdm/application" 
											xmlns:vd="http://api.vetrf.ru/schema/cdm/mercury/vet-document/v2" 
											xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
										  <SOAP-ENV:Header/>
										  <SOAP-ENV:Body>
											<apldef:submitApplicationRequest>
											  <apldef:apiKey>apikey</apldef:apiKey>
											  <apl:application>
												<apl:serviceId>mercury-g2b.service:2.0</apl:serviceId>
												<apl:issuerId>issuerId</apl:issuerId>
												<apl:issueDate>2017-09-29T16:58:20</apl:issueDate>
												<apl:data>
												  <merc:getVetDocumentByUuidRequest>
													<merc:localTransactionId>a10003</merc:localTransactionId>
													<merc:initiator>
													  <vd:login>user_login</vd:login>
													</merc:initiator>
													<bs:uuid>4405c9e5-2fd6-47bc-959c-ed31267d8d4f</bs:uuid>
													<dt:enterpriseGuid>ac264dc6-a3eb-4b0f-a86a-9c9577209d6f</dt:enterpriseGuid>
												  </merc:getVetDocumentByUuidRequest>
												</apl:data>
											  </apl:application>
											</apldef:submitApplicationRequest>
										  </SOAP-ENV:Body>
										</SOAP-ENV:Envelope>
									*/
									const VetisGetVetDocumentByUuidRequest * p_req = static_cast<const VetisGetVetDocumentByUuidRequest *>(rAppBlk.P_AppParam);
									SXml::WNode n_req(srb, "getVetDocumentByUuidRequest");
									n_req.PutAttrib(SXml::nst_xmlns("sch"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0)) {
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
										n_req.PutAttrib(SXml::nst_xmlns("dt"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
									}
									else
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document"));
									n_req.PutAttrib(SXml::nst_xmlns("sh"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/shipment"));
									n_req.PutAttrib(SXml::nst_xmlns("ws"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
									n_req.PutAttrib(SXml::nst_xmlns("app"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
									n_req.PutAttrib(SXml::nst_xmlns("co"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/common"));
									n_req.PutAttrib(SXml::nst_xmlns("ent"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/cerberus/enterprise"));
									n_req.PutAttrib(SXml::nst_xmlns("pr"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/production"));
									n_req.PutAttrib(SXml::nst_xmlns("ik"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/ikar"));
									n_req.PutAttrib(SXml::nst_xmlns("bs"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
									else
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/applications"));
									n_req.PutInner("localTransactionId", temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
									PutInitiator(srb, 0, rAppBlk.VetisSvcVer_.IsGe(2, 0, 0) ? "vd" : "co", rAppBlk.User);
									{
										SXml::WNode n_n2(srb, _xmlnst_bs_uuid(), temp_buf.Z().Cat(p_req->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
									}
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutInner(_xmlnst_dt("enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
									else
										n_req.PutInner(SXml::nst("ent", "enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
								}
								break;
							case VetisApplicationData::signGetStockEntryList:
								{
									const VetisGetStockEntryListRequest * p_req = static_cast<const VetisGetStockEntryListRequest *>(rAppBlk.P_AppParam);
									SXml::WNode n_req(srb, "getStockEntryListRequest");
									n_req.PutAttrib(SXml::nst_xmlns("sch"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
									else
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document"));
									n_req.PutAttrib(SXml::nst_xmlns("sh"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/shipment"));
									n_req.PutAttrib(SXml::nst_xmlns("ws"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
									n_req.PutAttrib(SXml::nst_xmlns("app"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
									n_req.PutAttrib(SXml::nst_xmlns("co"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/common"));
									n_req.PutAttrib(SXml::nst_xmlns("ent"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/cerberus/enterprise"));
									n_req.PutAttrib(SXml::nst_xmlns("pr"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/production"));
									n_req.PutAttrib(SXml::nst_xmlns("ik"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/ikar"));
									n_req.PutAttrib(SXml::nst_xmlns("bs"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
									n_req.PutAttrib(SXml::nst_xmlns("dt"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
									else
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/applications"));
									n_req.PutInner("localTransactionId", temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
									PutInitiator(srb, 0, rAppBlk.VetisSvcVer_.IsGe(2, 0, 0) ? "vd" : "co", rAppBlk.User);
									PutListOptions(srb, "bs", p_req->ListOptions);
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutInner(_xmlnst_dt("enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
									else
										n_req.PutInner(SXml::nst("ent", "enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
								}
								break;
							case VetisApplicationData::signGetStockEntryChangesList:
								{
									const VetisGetStockEntryChangesListRequest * p_req = static_cast<const VetisGetStockEntryChangesListRequest *>(rAppBlk.P_AppParam);
									SXml::WNode n_req(srb, "getStockEntryChangesListRequest");
									n_req.PutAttrib(SXml::nst_xmlns("sch"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document/v2"));
									else
										n_req.PutAttrib(SXml::nst_xmlns("vd"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/vet-document"));
									n_req.PutAttrib(SXml::nst_xmlns("sh"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/shipment"));
									n_req.PutAttrib(SXml::nst_xmlns("ws"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
									n_req.PutAttrib(SXml::nst_xmlns("app"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application"));
									n_req.PutAttrib(SXml::nst_xmlns("co"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/common"));
									n_req.PutAttrib(SXml::nst_xmlns("ent"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/cerberus/enterprise"));
									n_req.PutAttrib(SXml::nst_xmlns("pr"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/argus/production"));
									n_req.PutAttrib(SXml::nst_xmlns("ik"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/ikar"));
									n_req.PutAttrib(SXml::nst_xmlns("bs"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
									n_req.PutAttrib(SXml::nst_xmlns("dt"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/g2b/applications/v2"));
									else
										n_req.PutAttrib("xmlns", InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/mercury/applications"));
									n_req.PutInner("localTransactionId", temp_buf.Z().Cat(rAppBlk.LocalTransactionId));
									PutInitiator(srb, 0, rAppBlk.VetisSvcVer_.IsGe(2, 0, 0) ? "vd" : "co", rAppBlk.User);
									PutListOptions(srb, "bs", p_req->ListOptions);
									{
										SXml::WNode n_n2(srb, SXml::nst("bs", "updateDateInterval"));
										n_n2.PutInner(SXml::nst("bs", "beginDate"), temp_buf.Z().Cat(p_req->Period.Start, DATF_ISO8601CENT, 0));
										n_n2.PutInner(SXml::nst("bs", "endDate"), temp_buf.Z().Cat(p_req->Period.Finish, DATF_ISO8601CENT, 0));
									}
									if(rAppBlk.VetisSvcVer_.IsGe(2, 0, 0))
										n_req.PutInner(_xmlnst_dt("enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
									else
										n_req.PutInner(SXml::nst("ent", "enterpriseGuid"), temp_buf.Z().Cat(rAppBlk.EnterpriseId, S_GUID::fmtIDL|S_GUID::fmtLower));
								}
								break;
							default:
								CALLEXCEPT();
								break;
						}
					}
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(rAppBlk.AppData);
		{
			const char * p_url = GetAppSvcUrl(rAppBlk.VetisSvcVer_, P.Flags & P.fTestContour);
			THROW(SendSOAP(p_url, "submitApplicationRequest", rAppBlk.AppData, temp_buf));
		}
		THROW(ParseReply(temp_buf, rResult));
		LogFaults(rResult, 0);
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::LogFaults(const VetisApplicationBlock & rAb, const OutcomingEntry * pOcEntry)
{
	int    ok = -1;
	if(rAb.FaultList.getCount() || rAb.ErrList.getCount()) {
		ok = 1;
		if(P_Logger) {
			SString temp_buf;
			SString link_bill_text;
			PPObjBill * p_bobj = BillObj;
			for(uint fi = 0; fi < rAb.FaultList.getCount(); fi++) {
				const VetisFault * p_f = rAb.FaultList.at(fi);
				if(p_f) {
					temp_buf.Z().Cat("fault").CatDiv(':', 2).Cat(p_f->Message);
					P_Logger->Log(temp_buf);
					for(uint fei = 0; fei < p_f->ErrList.getCount(); fei++) {
						const VetisErrorEntry * p_e = p_f->ErrList.at(fei);
						if(p_e) {
							temp_buf.Z().Cat("error").CatDiv(':', 0);
							if(p_e->Code.NotEmpty())
								temp_buf.Space().Cat(p_e->Code);
							if(p_e->Item.NotEmpty())
								temp_buf.Space().Cat(p_e->Item);
							P_Logger->Log(temp_buf);
						}
					}
				}
			}
			for(uint ei = 0; ei < rAb.ErrList.getCount(); ei++) {
				const VetisErrorEntry * p_e = rAb.ErrList.at(ei);
				if(p_e) {
					temp_buf.Z().Cat("error").CatDiv(':', 0);
					// @v11.5.11 {
					if(pOcEntry) {
						BillTbl::Rec bill_rec;
						if(pOcEntry->LinkBillID && p_bobj->Search(pOcEntry->LinkBillID, &bill_rec) > 0) {
							PPObjBill::MakeCodeString(&bill_rec, 0, link_bill_text);
						}
					}
					if(link_bill_text.NotEmpty())
						temp_buf.Space().Cat(link_bill_text);
					// } @v11.5.11 
					if(p_e->Code.NotEmpty())
						temp_buf.Space().Cat(p_e->Code);
					if(p_e->Item.NotEmpty())
						temp_buf.Space().Cat(p_e->Item);
					P_Logger->Log(temp_buf);
				}
			}
		}
	}
	return ok;
}

int PPVetisInterface::ReceiveResult(const S_GUID & rAppId, OutcomingEntry * pOcEntry, VetisApplicationBlock & rResult, int once)
{
	int    ok = -1;
	uint   try_count = 0;
	SString temp_buf;
	SString reply_buf;
	const clock_t start_clock = clock();
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	do {
		rResult.Clear();
		if(!once) {
			if(try_count == 0)
				SDelay(100);
			else {
				if(P.Timeout > 0) {
					const clock_t ct = clock();
					THROW((ct - start_clock) < (P.Timeout * 1000));
				}
				uint d = 500;
				if(try_count >= 20)
					d = 5000;
				else if(try_count >= 10)
					d = 2500;
				else if(try_count >= 5)
					d = 1000;
				SDelay(d);
			}
			try_count++;
		}
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/application/ws-definitions"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "receiveApplicationResultRequest"));
					P.GetExtStrData(extssApiKey, temp_buf);
					n_f.PutInner(SXml::nst("ws", "apiKey"), temp_buf);
					n_f.PutInner(SXml::nst("ws", "issuerId"), VGuidToStr(P.IssuerUUID, temp_buf));
					n_f.PutInner(SXml::nst("ws", "applicationId"), VGuidToStr(rAppId, temp_buf));
				}
			}
		}
		xmlTextWriterFlush(srb);
		reply_buf.CopyFromN(reinterpret_cast<char *>(static_cast<xmlBuffer *>(srb)->content), static_cast<xmlBuffer *>(srb)->use);
		{
			const char * p_url = GetAppSvcUrl(rResult.VetisSvcVer_, P.Flags & P.fTestContour);
			THROW(SendSOAP(p_url, "receiveApplicationResult", reply_buf, temp_buf));
		}
		THROW(ParseReply(temp_buf, rResult));
		if(rResult.ApplicationStatus == rResult.appstRejected) {
			LogFaults(rResult, pOcEntry);
		}
	} while(!once && rResult.ApplicationStatus == rResult.appstInProcess);
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetEntityQuery2(int queryType, const char * pQueryParam, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	THROW_PP_S(!isempty(pQueryParam), PPERR_INVPARAM_EXT, __FUNCTION__"/pQueryParam");
	{
		enum {
			svcUndef      = 0,
			svcProduct    = 1,
			svcEnterprise = 2
		};
		struct GetEntityQueryInnerBlock {
			GetEntityQueryInnerBlock() : Svc(svcUndef), P_SoapReq(0), p_SoapAction(0)
			{
			}
			void   Set(int s, const char * pSoapReq, const char * pSoapAction, const SString & rParamTag)
			{
				Svc = s;
				P_SoapReq = pSoapReq;
				p_SoapAction = pSoapAction;
				ParamTag = rParamTag;
			}
			int    Svc;
			const  char * P_SoapReq;
			const  char * p_SoapAction;
			SString ParamTag;
		};
		GetEntityQueryInnerBlock iblk;
		VetisSubmitRequestBlock srb;
		switch(queryType) {
			case qtProductItemByGuid: iblk.Set(svcProduct, "getProductItemByGuidRequest", "GetProductItemByGuid", _xmlnst_bs_guid()); break;
			case qtProductItemByUuid: iblk.Set(svcProduct, "getProductItemByUuidRequest", "GetProductItemByUuid", _xmlnst_bs_uuid()); break;
			case qtProductByGuid: iblk.Set(svcProduct, "getProductByGuidRequest", "GetProductByGuid", _xmlnst_bs_guid()); break;
			case qtProductByUuid: iblk.Set(svcProduct, "getProductByUuidRequest", "GetProductByUuid", _xmlnst_bs_uuid()); break;
			case qtSubProductByGuid: iblk.Set(svcProduct, "getSubProductByGuidRequest", "GetSubProductByGuid", _xmlnst_bs_guid()); break;
			case qtSubProductByUuid: iblk.Set(svcProduct, "getSubProductByUuidRequest", "GetSubProductByUuid", _xmlnst_bs_uuid()); break;
			case qtBusinessEntityByGuid: iblk.Set(svcEnterprise, "getBusinessEntityByGuidRequest", "GetBusinessEntityByGuid", _xmlnst_bs_guid()); break;
			case qtBusinessEntityByUuid: iblk.Set(svcEnterprise, "getBusinessEntityByUuidRequest", "GetBusinessEntityByUuid", _xmlnst_bs_uuid()); break;
			case qtEnterpriseByGuid: iblk.Set(svcEnterprise, "getEnterpriseByGuidRequest", "GetEnterpriseByGuid", _xmlnst_bs_guid()); break;
			case qtEnterpriseByUuid: iblk.Set(svcEnterprise, "getEnterpriseByUuidRequest", "GetEnterpriseByUuid", _xmlnst_bs_uuid()); break;
		}
		if(oneof2(iblk.Svc, svcProduct, svcEnterprise)) {
			const char * p_url = 0;
			const char * p_arg_ns = 0;
			const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
			{

				SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
				n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
				n_env.PutAttrib(SXml::nst_xmlns("ws"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
				n_env.PutAttrib(SXml::nst_xmlns("bs"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
				if(iblk.Svc == svcProduct) {
					p_url = InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "ProductService"));
					p_arg_ns = "ws";
				}
				else if(iblk.Svc == svcEnterprise) {
					p_url = InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "EnterpriseService"));
					p_arg_ns = "ws";
				}
				assert(p_url);
				PutHeader(srb);
				{
					SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
					{
						SXml::WNode n_f(srb, SXml::nst(p_arg_ns, iblk.P_SoapReq));
						n_f.PutInner(iblk.ParamTag, pQueryParam);
					}
				}
			}
			xmlTextWriterFlush(srb);
			srb.GetReplyString(reply_buf);
			THROW(SendSOAP(p_url, iblk.p_SoapAction, reply_buf, temp_buf));
			THROW(ParseReply(temp_buf, rReply));
			ok = 1;
		}
	}
    CATCHZOK
	return ok;
}

void PPVetisInterface::PutListOptionsParam(xmlTextWriter * pWriter, uint offs, uint count)
{
	SString temp_buf;
	SXml::WNode n_lo(pWriter, SXml::nst("base", "listOptions"));
	n_lo.PutInner(SXml::nst("base", "count"), temp_buf.Z().Cat(count));
	n_lo.PutInner(SXml::nst("base", "offset"), temp_buf.Z().Cat(offs));
}

/*static*/int PPVetisInterface::GoodsDateToString(const SUniTime & rUt, SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	LDATETIME dtm;
	const  uint uts = rUt.GetSignature();
	if(oneof5(uts, SUniTime::indYr, SUniTime::indMon, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) {
		if(rUt.Get(dtm)) {
			if(oneof5(uts, SUniTime::indYr, SUniTime::indMon, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) {
				rBuf.Cat(dtm.d.year());
				if(oneof4(uts, SUniTime::indMon, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) {
					rBuf.CatDiv('/', 0).Cat(dtm.d.month());
					if(oneof3(uts, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) {
						rBuf.CatDiv('/', 0).Cat(dtm.d.day());
						if(oneof2(uts, SUniTime::indHr, SUniTime::indMSec))
							rBuf.CatDiv(' ', 0).Cat(dtm.t.hour());
					}
				}
			}
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

/*static*/int PPVetisInterface::LockProcess(int funcId, bool unlock)
{
	int    ok = 1;
	if(!unlock) {
		long   mutex_id = 0;
		PPSyncItem sync_item;
		int    r = DS.GetSync().CreateMutex_(LConfig.SessionID, PPOBJ_VETISIC, funcId, &mutex_id, &sync_item);
		if(r < 0)
			ok = PPSetError(PPERR_VETISICFUNCLOCKED, sync_item.Name);
		else if(!r)
			ok = 0;
	}
	else
		ok = DS.GetSync().ReleaseMutex(PPOBJ_VETISIC, funcId);
	return ok;
}

int PPVetisInterface::PutGoodsDate(xmlTextWriter * pWriter, const char * pScopeXmlTag, const char * pDtNs, const SUniTime & rUt)
{
	int    ok = -1;
	const  uint uts = rUt.GetSignature();
	if(oneof5(uts, SUniTime::indYr, SUniTime::indMon, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) { // SUniTime::indMSec
		LDATETIME dtm;
		if(rUt.Get(dtm)) {
			SString temp_buf;
			SXml::WNode n_d2(pWriter, pScopeXmlTag);
			if(oneof5(uts, SUniTime::indYr, SUniTime::indMon, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) { // SUniTime::indMSec
				n_d2.PutInner(SXml::nst(pDtNs, "year"), temp_buf.SetInt(dtm.d.year()));
				if(oneof4(uts, SUniTime::indMon, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) { // SUniTime::indMSec
					n_d2.PutInner(SXml::nst(pDtNs, "month"), temp_buf.SetInt(dtm.d.month()));
					if(oneof3(uts, SUniTime::indDay, SUniTime::indHr, SUniTime::indMSec)) { // SUniTime::indMSec
						n_d2.PutInner(SXml::nst(pDtNs, "day"), temp_buf.SetInt(dtm.d.day()));
						if(oneof2(uts, SUniTime::indHr, SUniTime::indMSec)) // SUniTime::indMSec
							n_d2.PutInner(SXml::nst(pDtNs, "hour"), temp_buf.SetInt(dtm.t.hour()));
					}
				}
			}
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int PPVetisInterface::GetUnitList(uint offs, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("v2"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("v21"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("v2", "getUnitListRequest"));
					PutListOptionsParam(srb, offs, count);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "DictionaryService")), "GetUnitList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetCountryList(VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("ikar"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "getAllCountryListRequest"));
					PutListOptionsParam(srb, 0, 1000);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		//https://api.vetrf.ru/platform/services/2.0/IkarService
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "IkarService")), "GetAllCountryList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetRegionList(S_GUID & rCountryGuid, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("ikar"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "getRegionListByCountryRequest"));
					PutListOptionsParam(srb, 0, 1000);
					rCountryGuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
					n_f.PutInner(SXml::nst("ikar", "countryGuid"), temp_buf);
					//<ikar:countryGuid>f133f1fd-7fa2-da91-d069-24df64749742</ikar:countryGuid>
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "IkarService")), "GetRegionListByCountry", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetLocalityList(S_GUID & rRegionGuid, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("ikar"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "getLocalityListByRegionRequest"));
					PutListOptionsParam(srb, 0, 1000);
					rRegionGuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
					n_f.PutInner(SXml::nst("ikar", "regionGuid"), temp_buf);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "IkarService")), "GetLocalityListByRegion", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetPurposeList(uint offs, uint count, VetisApplicationBlock & rReply)
{
	int    ok = 1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("v2"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("v21"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("v2", "getPurposeListRequest"));
					PutListOptionsParam(srb, offs, count);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "DictionaryService")), "GetPurposeList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetProductChangesList(uint offs, uint count, LDATE since, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("v2"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("v21"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("v2", "getProductChangesListRequest"));
					PutListOptionsParam(srb, offs, count);
					{
						SXml::WNode n_udi(srb, SXml::nst("base", "updateDateInterval"));
						LDATETIME dtm_since;
						dtm_since.Set(ValidDateOr(since, encodedate(1, 1, 2015)), ZEROTIME);
						n_udi.PutInner(SXml::nst("base", "beginDate"), temp_buf.Z().Cat(dtm_since, DATF_ISO8601CENT, 0));
					}
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "ProductService")), "GetProductChangesList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetSubProductChangesList(uint offs, uint count, LDATE since, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			/*
			<soapenv:Envelope
				xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/"
				xmlns:ws="http://api.vetrf.ru/schema/cdm/registry/ws-definitions/v2"
				xmlns:bs="http://api.vetrf.ru/schema/cdm/base">
			   <soapenv:Header/>
			   <soapenv:Body>
				  <ws:getSubProductChangesListRequest>
					 <bs:listOptions>
						<bs:count>3</bs:count>
						<bs:offset>0</bs:offset>
					 </bs:listOptions>
					 <bs:updateDateInterval>
						<bs:beginDate>2016-10-20T00:00:00</bs:beginDate>
					 </bs:updateDateInterval>
				  </ws:getSubProductChangesListRequest>
			   </soapenv:Body>
			</soapenv:Envelope>
			*/
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("v2"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("v21"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("v2", "getSubProductChangesListRequest"));
					PutListOptionsParam(srb, offs, count);
					{
						SXml::WNode n_udi(srb, SXml::nst("base", "updateDateInterval"));
						LDATETIME dtm_since;
						dtm_since.Set(ValidDateOr(since, encodedate(1, 1, 2015)), ZEROTIME);
						n_udi.PutInner(SXml::nst("base", "beginDate"), temp_buf.Z().Cat(dtm_since, DATF_ISO8601CENT, 0));
					}
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "ProductService")), "GetSubProductChangesList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
		ok = 1;
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetProductItemList(uint offs, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("v2"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("v21"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("v2", "getProductItemListRequest"));
					PutListOptionsParam(srb, offs, count);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "ProductService")), "GetProductItemList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetRussianEnterpriseList(uint offs, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		/*
			<soapenv:Envelope 
				xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" 
				xmlns:ws="http://api.vetrf.ru/schema/cdm/registry/ws-definitions/v2" 
				xmlns:bs="http://api.vetrf.ru/schema/cdm/base" 
				xmlns:dt="http://api.vetrf.ru/schema/cdm/dictionary/v2">
			<soapenv:Header/>
			<soapenv:Body>
			<ws:getRussianEnterpriseListRequest>
			<bs:listOptions>
			<bs:count>3</bs:count>
			<bs:offset>0</bs:offset>
			</bs:listOptions>
			<dt:enterprise>
			<dt:name>Мясокомбинат</dt:name>
			</dt:enterprise>
			</ws:getRussianEnterpriseListRequest>
			</soapenv:Body>
			</soapenv:Envelope>
		*/
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/registry/ws-definitions/v2"));
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("dt"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/dictionary/v2"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "getRussianEnterpriseListRequest"));
					PutListOptionsParam(srb, offs, count);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, GetAppSvcPath(SVerT(2, 1), "EnterpriseService")), "GetRussianEnterpriseList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::GetBusinessEntityList(uint offs, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	SString temp_buf;
	SString reply_buf;
	rReply.Clear();
	THROW_PP(State & stInited, PPERR_VETISIFCNOTINITED);
	{
		VetisSubmitRequestBlock srb;
		{
			SXml::WNode n_env(srb, SXml::nst("soapenv", "Envelope"));
			n_env.PutAttrib(SXml::nst_xmlns("soapenv"), InetUrl::MkHttp("schemas.xmlsoap.org", "soap/envelope/"));
			n_env.PutAttrib(SXml::nst_xmlns("ws"), "http://api.vetrf.ru/schema/cdm/cerberus/business-entity/ws-definitions");
			n_env.PutAttrib(SXml::nst_xmlns("base"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/base"));
			n_env.PutAttrib(SXml::nst_xmlns("ent"),  InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/cerberus/enterprise"));
			n_env.PutAttrib(SXml::nst_xmlns("ikar"), InetUrl::MkHttp(P_ApiVtrf, "schema/cdm/ikar"));
			PutHeader(srb);
			{
				SXml::WNode n_bdy(srb, SXml::nst("soapenv", "Body"));
				{
					SXml::WNode n_f(srb, SXml::nst("ws", "getBusinessEntityListRequest"));
					PutListOptionsParam(srb, offs, count);
				}
			}
		}
		xmlTextWriterFlush(srb);
		srb.GetReplyString(reply_buf);
		const char * p_domain = (P.Flags & P.fTestContour) ? "api2.vetrf.ru:8002" : P_ApiVtrf;
		THROW(SendSOAP(InetUrl::MkHttps(p_domain, "platform/cerberus/services/EnterpriseService"), "GetBusinessEntityList", reply_buf, temp_buf));
		THROW(ParseReply(temp_buf, rReply));
	}
    CATCHZOK
	return ok;
}

int PPVetisInterface::PrepareApplicationBlockForReq(VetisApplicationBlock & rBlk)
{
	int   ok = 1;
	SString temp_buf;
	if(oneof2(rBlk.P_AppParam->Sign, VetisApplicationData::signProcessOutgoingConsignment, VetisApplicationData::signResolveDiscrepancy)) {
		P.GetExtStrData(extssDoctInitiator, temp_buf);
	}
	if(temp_buf.IsEmpty()) {
		P.GetExtStrData(extssQInitiator, temp_buf);
	}
	THROW_PP(temp_buf.NotEmpty(), PPERR_VETISINITUSERUNDEF);
	rBlk.User = temp_buf;
	rBlk.ServiceId = rBlk.GetServiceId();
	rBlk.IssuerId = P.IssuerUUID;
	rBlk.EnterpriseId = P.EntUUID;
	rBlk.IssueDate = getcurdatetime_();
	rBlk.ApplicationId.Generate();
	rBlk.LocalTransactionId = ++LastLocalTransactionId;
	CATCHZOK
	return ok;
}

int PPVetisInterface::GetStockEntryByUuid(const S_GUID & rUuid, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisGetStockEntryByUUIDRequest app_data;
	app_data.Uuid = rUuid;
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		blk.EnterpriseId = P.EntUUID;
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			rReply.VetisSvcVer_ = SVerT(2, 1);
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::GetVetDocumentByUuid(const S_GUID & rUuid, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisGetVetDocumentByUuidRequest app_data;
	app_data.Uuid = rUuid;
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			rReply.VetisSvcVer_ = SVerT(2, 1);
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::GetStockEntryChangesList(const STimeChunk & rPeriod, uint startOffset, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	const  uint max_force_try_count = 10;
	const  uint force_try_timeout = 5000;
	uint   force_try_count = 0;
	int    force_next_try = 0;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	VetisGetStockEntryChangesListRequest app_data;
	app_data.ListOptions.Set(startOffset, count);
	app_data.Period = rPeriod;
	do {
		force_next_try = 0;
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ErrList.getCount() == 1) {
				const VetisErrorEntry * p_err_entry = rReply.ErrList.at(0);
				if(p_err_entry && p_err_entry->Code.IsEqiAscii("APLM0012")) {
					if(++force_try_count < max_force_try_count) {
						if(P_Logger) {
							PPLoadText(PPTXT_VETISFORCENEXTTRY, fmt_buf);
							temp_buf.Z().Cat("GetStockEntryChangesList").CatDiv('-', 1).Cat(force_try_count+1).Slash().Cat(max_force_try_count);
							temp_buf.Space().CatEq("timeout", force_try_timeout);
							P_Logger->Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
						}
						SDelay(force_try_timeout);
						force_next_try = 1;
					}
				}
			}
			ok = 1;
		}
	} while(force_next_try);
	CATCHZOK
	return ok;
}

int PPVetisInterface::GetStockEntryList(uint startOffset, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	const  uint max_force_try_count = 10;
	const  uint force_try_timeout = 5000;
	uint   force_try_count = 0;
	int    force_next_try = 0;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	VetisGetStockEntryListRequest app_data;
	app_data.ListOptions.Set(startOffset, count);
	do {
		force_next_try = 0;
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ErrList.getCount() == 1) {
				const VetisErrorEntry * p_err_entry = rReply.ErrList.at(0);
				if(p_err_entry && p_err_entry->Code.IsEqiAscii("APLM0012")) {
					if(++force_try_count < max_force_try_count) {
						if(P_Logger) {
							PPLoadText(PPTXT_VETISFORCENEXTTRY, fmt_buf);
							temp_buf.Z().Cat("GetStockEntryList").CatDiv('-', 1).Cat(force_try_count+1).Slash().Cat(max_force_try_count);
							temp_buf.Space().CatEq("timeout", force_try_timeout);
							P_Logger->Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
						}
						SDelay(force_try_timeout);
						force_next_try = 1;
					}
				}
			}
			ok = 1;
		}
	} while(force_next_try);
	CATCHZOK
	return ok;
}

int PPVetisInterface::GetVetDocumentChangesList(const STimeChunk & rPeriod, uint startOffset, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	const  uint max_force_try_count = 7;
	const  uint force_try_timeout = 1000;
	uint   force_try_count = 0;
	int    force_next_try = 0;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	VetisGetVetDocumentChangesListRequest app_data;
	app_data.ListOptions.Set(startOffset, count);
	app_data.Period = rPeriod;
	do {
		force_next_try = 0;
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ErrList.getCount() == 1) {
				const VetisErrorEntry * p_err_entry = rReply.ErrList.at(0);
				if(p_err_entry && p_err_entry->Code.IsEqiAscii("APLM0012")) {
					if(++force_try_count < max_force_try_count) {
						if(P_Logger) {
							PPLoadText(PPTXT_VETISFORCENEXTTRY, fmt_buf);
							temp_buf.Z().Cat("GetVetDocumentChangesList").CatDiv('-', 1).Cat(force_try_count+1).Slash().Cat(max_force_try_count);
							temp_buf.Space().CatEq("timeout", force_try_timeout);
							P_Logger->Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
						}
						SDelay(force_try_timeout);
						force_next_try = 1;
					}
				}
			}
			ok = 1;
		}
	} while(force_next_try);
	CATCHZOK
	return ok;
}

int PPVetisInterface::GetVetDocumentList(const STimeChunk & rPeriod, uint startOffset, uint count, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisGetVetDocumentListRequest app_data;
	app_data.ListOptions.Set(startOffset, count);
	app_data.Period = rPeriod;
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ProcessIncomingConsignment(const S_GUID & rDocUuid, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisApplicationBlock doc_reply;
	if(GetVetDocumentByUuid(rDocUuid, doc_reply) > 0 && doc_reply.VetDocList.getCount() == 1) {
		VetisProcessIncomingConsignmentRequest app_data;
		const VetisVetDocument * p_src_doc = doc_reply.VetDocList.at(0);
		if(p_src_doc && p_src_doc->VetDStatus == vetisdocstCONFIRMED) {
			app_data.Doc = *p_src_doc;
			{
				rReply.Clear();
				VetisApplicationBlock submit_result;
				VetisApplicationBlock blk(SVerT(2, 1), &app_data);
				THROW(SubmitRequest(blk, submit_result));
				if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
					THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ModifyEnterprise(VetisRegisterModificationType modType, const VetisEnterprise & rEnt, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisModifyEnterpriseRequest app_data(modType);
	app_data.En = rEnt;
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ModifyActivityLocations(VetisRegisterModificationType modType, const VetisBusinessEntity & rBe, const VetisEnterprise & rEnt, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisModifyActivityLocationsRequest app_data(modType);
	app_data.En = rEnt;
	app_data.Be = rBe;
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ModifyProducerStockListOperation(VetisRegisterModificationType modType, VetisProductItem & rPi, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	ModifyProducerStockListOperationRequest app_data(modType);
	app_data.Pi = rPi;
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::CheckShipmentRegionalizationOperation(const UuidArray & rSubProductList, const TSCollection <VetisShipmentRoutePoint> & rRoutePointList, VetisApplicationBlock & rReply) // @v11.0.0
{
	int    ok = -1;
	CheckShipmentRegionalizationRequest app_data;
	app_data.SubGroupGuidList = rSubProductList;
	TSCollection_Copy(app_data.RoutePointList, rRoutePointList);
	{
		rReply.Clear();
		VetisApplicationBlock submit_result;
		VetisApplicationBlock blk(SVerT(2, 1), &app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::WithdrawVetDocument(const S_GUID & rDocUuid, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisApplicationBlock doc_reply;
	if(GetVetDocumentByUuid(rDocUuid, doc_reply) > 0 && doc_reply.VetDocList.getCount() == 1) {
		VetisWithdrawVetDocumentRequest app_data;
		app_data.Doc = *doc_reply.VetDocList.at(0);
		{
			rReply.Clear();
			VetisApplicationBlock submit_result;
			VetisApplicationBlock blk(SVerT(2, 1), &app_data);
			THROW(SubmitRequest(blk, submit_result));
			if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
				THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::WriteOffIncomeCert(PPID docEntityID, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply) // @construction
{
	int   ok = -1;
	SString temp_buf;
	VetisVetDocument org_doc_entity;
	VetisResolveDiscrepancyRequest app_data;
	VetisApplicationBlock blk(SVerT(2, 1), &app_data);
	{
		PPID   stock_entry_entity_id = 0;
		VetisApplicationBlock org_doc_reply;
		THROW(SearchLastStockEntry(/*app_data.VdRec.OrgDocEntityID*/docEntityID, org_doc_entity, stock_entry_entity_id, app_data.StockEntryGuid, app_data.StockEntryUuid, app_data.StockEntryRest));
		if(GetVetDocumentByUuid(org_doc_entity.Uuid, org_doc_reply) > 0 && org_doc_reply.VetDocList.getCount() == 1) {
			rReply.Clear();
			app_data.OrgDoc = *org_doc_reply.VetDocList.at(0);
			const VetisBatch & r_bat = app_data.OrgDoc.CertifiedConsignment.Batch;
			//app_data.Pi = app_data.OrgDoc.CertifiedConsignment.Batch.ProductItem;
			const S_GUID pi_guid = r_bat.ProductItem.Guid;
			if(!!pi_guid) {
				VetisApplicationBlock pi_reply;
				pi_guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
				if(GetEntityQuery2(qtProductItemByGuid, temp_buf, pi_reply) > 0 && pi_reply.ProductItemList.getCount())
					app_data.Pi = *pi_reply.ProductItemList.at(0);
			}
			if(!app_data.Pi.Guid) {
				app_data.Pi.ProductType = r_bat.ProductType;
				app_data.Pi.Product = r_bat.Product;
				app_data.Pi.SubProduct = r_bat.SubProduct;
			}
			if(app_data.Pi.Name.IsEmpty())
				app_data.Pi.Name = r_bat.ProductItem.Name;
			app_data.VdRec.Flags |= VetisVetDocument::fDiscrepancyLack;
			app_data.VdRec.AckVolume = 0.0;
			app_data.VdRec.Volume = app_data.StockEntryRest;
			app_data.VdRec.WayBillDate = getcurdate_();
		}
	}
	{
		VetisApplicationBlock submit_result;
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			rReply.VetisSvcVer_ = SVerT(2, 1);
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ApplicationStatus == VetisApplicationBlock::appstCompleted) {
				if(rReply.VetDocList.getCount() || rReply.VetStockList.getCount()) {
					PPTransaction tra(1);
					THROW(tra);
					if(rReply.VetDocList.getCount()) {
						for(uint i = 0; i < rReply.VetDocList.getCount(); i++) {
							VetisVetDocument * p_item = rReply.VetDocList.at(i);
							if(p_item) {
								p_item->NativeBillID = app_data.VdRec.LinkBillID;
								p_item->NativeBillRow = app_data.VdRec.LinkBillRow;
								p_item->CertifiedConsignment.Batch.NativeGoodsID = app_data.VdRec.LinkGoodsID;
								THROW(PeC.Put(0, *p_item, 0, pUreList, 0));
							}
						}
					}
					if(rReply.VetStockList.getCount()) {
						for(uint i = 0; i < rReply.VetStockList.getCount(); i++) {
							const VetisStockEntry * p_item = rReply.VetStockList.at(i);
							if(p_item)
								THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 0));
						}
					}
					// THROW(PeC.DeleteEntity(docEntityID, 0)); // Создав запись сертификата, необходимо удалить запись подготовки транспортного документа
					THROW(tra.Commit());
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::RegisterProduction(PPID docEntityID, const PPIDArray & rExpenseDocEntityList, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	VetisRegisterProductionRequest app_data;
	VetisApplicationBlock blk(SVerT(2, 1), &app_data);
	SUniTime min_src_expiry_from;
	SUniTime min_src_expiry_to;
	THROW(PeC.SearchDocument(docEntityID, &app_data.VdRec) > 0);
	THROW_PP_S(app_data.VdRec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, docEntityID);
	THROW(app_data.VdRec.Flags & VetisVetDocument::fManufIncome);
	THROW(app_data.VdRec.ProductItemID);
	PeC.Get(app_data.VdRec.ProductItemID, app_data.Pi);
	if(!app_data.Pi.Guid.IsZero()) {
		VetisApplicationBlock pi_reply;
		app_data.Pi.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		if(GetEntityQuery2(qtProductItemByGuid, temp_buf, pi_reply) > 0 && pi_reply.ProductItemList.getCount())
			app_data.Pi = *pi_reply.ProductItemList.at(0);
	}
	for(uint edeidx = 0; edeidx < rExpenseDocEntityList.getCount(); edeidx++) {
		const  PPID ede_id = rExpenseDocEntityList.get(edeidx);
		VetisRegisterProductionRequest::SourceEntry * p_se = app_data.SourceList.CreateNewItem();
		THROW_SL(p_se);
		THROW(PeC.SearchDocument(ede_id, &p_se->Rec) > 0);
		THROW_PP_S(p_se->Rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, ede_id);
		THROW(p_se->Rec.Flags & VetisVetDocument::fManufExpense);
		{
			PPID   stock_entry_entity_id = 0;
			VetisApplicationBlock src_doc_reply;
			VetisVetDocument src_doc_entity;
			THROW(SearchLastStockEntry(p_se->Rec.OrgDocEntityID, src_doc_entity, stock_entry_entity_id, p_se->StockEntryGuid, p_se->StockEntryUuid, p_se->StockEntryRest));
			if(GetVetDocumentByUuid(src_doc_entity.Uuid, src_doc_reply) > 0 && src_doc_reply.VetDocList.getCount() == 1) {
				rReply.Clear();
				p_se->Doc = *src_doc_reply.VetDocList.at(0);
				{
					SUniTime temp_ut;
					if(temp_ut.FromInt64(p_se->Rec.ExpiryFrom)) {
						if(!min_src_expiry_from || min_src_expiry_from.Compare(temp_ut, 0) < 0) {
							min_src_expiry_from = temp_ut;
						}
					}
					if(temp_ut.FromInt64(p_se->Rec.ExpiryTo)) {
						if(!min_src_expiry_to || min_src_expiry_to.Compare(temp_ut, 0) < 0) {
							min_src_expiry_to = temp_ut;
						}
					}
				}
				/*const S_GUID pi_guid = p_se->Doc.CertifiedConsignment.Batch.ProductItem.Guid;
				if(!pi_guid.IsZero()) {
					VetisApplicationBlock pi_reply;
					pi_guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
					if(GetEntityQuery2(qtProductItemByGuid, temp_buf, pi_reply) > 0 && pi_reply.ProductItemList.getCount())
						app_data.Pi = *pi_reply.ProductItemList.at(0);
				}*/
			}
		}
	}
	if(app_data.VdRec.ExpiryFrom == 0 && app_data.VdRec.ExpiryTo == 0) {
		app_data.VdRec.ExpiryFrom = min_src_expiry_from.ToInt64();
		app_data.VdRec.ExpiryTo = min_src_expiry_to.ToInt64();
	}
	{
		VetisApplicationBlock submit_result;
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			rReply.VetisSvcVer_ = SVerT(2, 1);
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ApplicationStatus == VetisApplicationBlock::appstCompleted) {
				if(rReply.VetDocList.getCount() || rReply.VetStockList.getCount()) {
					PPTransaction tra(1);
					THROW(tra);
					if(rReply.VetDocList.getCount()) {
						for(uint i = 0; i < rReply.VetDocList.getCount(); i++) {
							VetisVetDocument * p_item = rReply.VetDocList.at(i);
							if(p_item) {
								p_item->NativeBillID = app_data.VdRec.LinkBillID;
								p_item->NativeBillRow = app_data.VdRec.LinkBillRow;
								p_item->CertifiedConsignment.Batch.NativeGoodsID = app_data.VdRec.LinkGoodsID;
								THROW(PeC.Put(0, *p_item, 0, pUreList, 0));
								{
									TransferTbl::Rec trfr_rec;
									if(p_bobj->trfr->SearchByBill(app_data.VdRec.LinkBillID, 0, app_data.VdRec.LinkBillRow, &trfr_rec) > 0 && trfr_rec.Flags & PPTFR_RECEIPT && trfr_rec.LotID) {
										ObjTagItem tag_item;
										if(tag_item.SetGuid(PPTAG_LOT_VETIS_UUID, &p_item->Uuid)) {
											THROW(p_ref->Ot.PutTag(PPOBJ_LOT, trfr_rec.LotID, &tag_item, 0));
										}
									}
								}
							}
						}
					}
					if(rReply.VetStockList.getCount()) {
						for(uint i = 0; i < rReply.VetStockList.getCount(); i++) {
							const VetisStockEntry * p_item = rReply.VetStockList.at(i);
							if(p_item)
								THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 0));
						}
					}
					// Создав запись сертификата, необходимо удалить записи подготовки производственного документа
					THROW(PeC.DeleteEntity(docEntityID, 0));
					for(uint selidx = 0; selidx < rExpenseDocEntityList.getCount(); selidx++) {
						const  PPID src_doc_entity_id = rExpenseDocEntityList.get(selidx);
						THROW(PeC.DeleteEntity(src_doc_entity_id, 0));
					}
					THROW(tra.Commit());
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::ResolveDiscrepancy(PPID docEntityID, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply)
{
	int   ok = -1;
	SString temp_buf;
	VetisVetDocument org_doc_entity;
	VetisResolveDiscrepancyRequest app_data;
	VetisApplicationBlock blk(SVerT(2, 1), &app_data);
	THROW(PeC.SearchDocument(docEntityID, &app_data.VdRec) > 0);
	THROW_PP_S(app_data.VdRec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, docEntityID);
	THROW(app_data.VdRec.Flags & VetisVetDocument::fDiscrepancy);
	if(app_data.VdRec.Flags & VetisVetDocument::fDiscrepancyLack) {
		PPID   stock_entry_entity_id = 0;
		VetisApplicationBlock org_doc_reply;
		THROW(SearchLastStockEntry(app_data.VdRec.OrgDocEntityID, org_doc_entity, stock_entry_entity_id, app_data.StockEntryGuid, app_data.StockEntryUuid, app_data.StockEntryRest));
		if(GetVetDocumentByUuid(org_doc_entity.Uuid, org_doc_reply) > 0 && org_doc_reply.VetDocList.getCount() == 1) {
			rReply.Clear();
			app_data.OrgDoc = *org_doc_reply.VetDocList.at(0);
			//app_data.Pi = app_data.OrgDoc.CertifiedConsignment.Batch.ProductItem;
			const S_GUID pi_guid = app_data.OrgDoc.CertifiedConsignment.Batch.ProductItem.Guid;
			if(!pi_guid.IsZero()) {
				VetisApplicationBlock pi_reply;
				pi_guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
				if(GetEntityQuery2(qtProductItemByGuid, temp_buf, pi_reply) > 0 && pi_reply.ProductItemList.getCount())
					app_data.Pi = *pi_reply.ProductItemList.at(0);
			}
		}
	}
	else {
		THROW(app_data.VdRec.ProductItemID);
		PeC.Get(app_data.VdRec.ProductItemID, app_data.Pi);
		if(!app_data.Pi.Guid.IsZero()) {
			VetisApplicationBlock pi_reply;
			app_data.Pi.Guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
			if(GetEntityQuery2(qtProductItemByGuid, temp_buf, pi_reply) > 0 && pi_reply.ProductItemList.getCount())
				app_data.Pi = *pi_reply.ProductItemList.at(0);
		}
	}
	{
		VetisApplicationBlock submit_result;
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			rReply.VetisSvcVer_ = SVerT(2, 1);
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ApplicationStatus == VetisApplicationBlock::appstCompleted) {
				if(rReply.VetDocList.getCount() || rReply.VetStockList.getCount()) {
					PPTransaction tra(1);
					THROW(tra);
					if(rReply.VetDocList.getCount()) {
						for(uint i = 0; i < rReply.VetDocList.getCount(); i++) {
							VetisVetDocument * p_item = rReply.VetDocList.at(i);
							if(p_item) {
								p_item->NativeBillID = app_data.VdRec.LinkBillID;
								p_item->NativeBillRow = app_data.VdRec.LinkBillRow;
								p_item->CertifiedConsignment.Batch.NativeGoodsID = app_data.VdRec.LinkGoodsID;
								THROW(PeC.Put(0, *p_item, 0, pUreList, 0));
							}
						}
					}
					if(rReply.VetStockList.getCount()) {
						for(uint i = 0; i < rReply.VetStockList.getCount(); i++) {
							const VetisStockEntry * p_item = rReply.VetStockList.at(i);
							if(p_item)
								THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 0));
						}
					}
					THROW(PeC.DeleteEntity(docEntityID, 0)); // Создав запись сертификата, необходимо удалить запись подготовки транспортного документа
					THROW(tra.Commit());
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

static LDATE GetFinalExpiryData(const VetisGoodsDate & rD)
{
	LDATE final_expiry_date = ZERODATE;
	LDATETIME expiry_dtm = ZERODATETIME;
	if(!!rD.SecondDate && rD.SecondDate.Get(expiry_dtm) && checkdate(expiry_dtm.d))
		final_expiry_date = expiry_dtm.d;
	else if(!!rD.FirstDate && rD.FirstDate.Get(expiry_dtm) && checkdate(expiry_dtm.d))
		final_expiry_date = expiry_dtm.d;
	return final_expiry_date;
}

int PPVetisInterface::SearchDupOutgoingConsignment(const VetisVetDocument & rRec) // @construction
{
	//VetisVetDocument::fCargoExpertized
	int    ok = -1;
	VetisDocumentTbl::Key3 k3;
	VetisDocumentTbl::Rec rec;
	k3.WayBillDate = rRec.WayBillDate;
	STRNSCPY(k3.WayBillNumber, rRec.WayBillNumber);
	if(PeC.DT.search(3, &k3, spEq)) do {
		PeC.DT.CopyBufTo(&rec);
		if(rec.WayBillDate == rRec.WayBillDate && sstreq(rec.WayBillNumber, rRec.WayBillNumber)) { // @paranoic
			if(rec.VetisDocStatus == vetisdocstCONFIRMED) {
				//if(rec.FromEntityID == rRec.FromEnity)
			}
		}
	} while(PeC.DT.search(3, &k3, spNext) && PeC.DT.data.WayBillDate == rRec.WayBillDate && sstreq(PeC.DT.data.WayBillNumber, rRec.WayBillNumber));
	return ok;
}

int PPVetisInterface::InitOutgoingEntry(PPID docEntityID, const TSCollection <VetisRouteSectionR13nRules> * pRegRuleList, OutcomingList & rList)
{
	int    ok = 1;
	SString temp_buf;
	SString msg_buf;
	OutcomingEntry entry;
	VetisDocumentTbl::Rec vd_rec;
	VetisVetDocument org_doc_entity;
	THROW(PeC.SearchDocument(docEntityID, &vd_rec) > 0);
	msg_buf.Z().Cat(vd_rec.WayBillNumber).CatDiv('-', 0).Cat(vd_rec.WayBillDate).CatDiv('-', 0).Cat(vd_rec.LinkBillRow);
	THROW_PP_S(vd_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, docEntityID);
	THROW_PP_S(vd_rec.OrgDocEntityID, PPERR_VETISDOCRECDONTREFORGDOC, msg_buf);
	THROW(PeC.Get(vd_rec.OrgDocEntityID, org_doc_entity) > 0);
	{
		const LDATE final_expiry_date = GetFinalExpiryData(org_doc_entity.CertifiedConsignment.Batch.ExpiryDate);
		if(final_expiry_date) {
			temp_buf.Z().Cat(final_expiry_date, DATF_MDY).Space().Cat(msg_buf);
			THROW_PP_S(final_expiry_date > getcurdate_(), PPERR_VETISORGDOCEXPIRY, temp_buf);
		}
	}
	entry.PrepEntityID = docEntityID;
	entry.OrgDocEntityID = vd_rec.OrgDocEntityID;
	entry.LinkBillID = vd_rec.LinkBillID;
	entry.LinkBillRow = vd_rec.LinkBillRow;
	entry.LinkGoodsID = vd_rec.LinkGoodsID;
	entry.AppId = vd_rec.AppReqId;
	entry.P_RegRuleList = pRegRuleList; // @v11.0.11
	ok = rList.InsertEntry(entry);
	CATCHZOK
	return ok;
}

int PPVetisInterface::QueryOutgoingConsignmentResult(OutcomingEntry & rEntry, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	THROW(ReceiveResult(rEntry.AppId, &rEntry, rReply, 1/*once*/));
	if(rReply.ApplicationStatus == VetisApplicationBlock::appstCompleted) {
		if(rReply.VetDocList.getCount() == 1) {
			S_GUID stock_entry_uuid;
			VetisVetDocument * p_item = rReply.VetDocList.at(0);
			assert(p_item);
			PPTransaction tra(1);
			THROW(tra);
			if(p_item) {
				p_item->NativeBillID = rEntry.LinkBillID;
				p_item->NativeBillRow = rEntry.LinkBillRow;
				p_item->CertifiedConsignment.Batch.NativeGoodsID = rEntry.LinkGoodsID;
				THROW(PeC.Put(0, *p_item, 0, pUreList, 0));
				THROW(PeC.DeleteEntity(rEntry.PrepEntityID, 0)); // Создав запись сертификата, необходимо удалить запись подготовки транспортного документа
				rEntry.AppId.Z();
				rEntry.State = rEntry.stReply;
				ok = 1;
			}
			if(rReply.VetStockList.getCount()) {
				for(uint i = 0; i < rReply.VetStockList.getCount(); i++) {
					const VetisStockEntry * p_item = rReply.VetStockList.at(i);
					if(p_item) {
						// Полученная с этим ответом запись складского журнала не содержит ссылку на оригинальный сертификат.
						// Поэтому придется запросить отдельно.
						stock_entry_uuid = p_item->Uuid;
						//THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 0));
					}
				}
			}
			THROW(tra.Commit());
			if(!stock_entry_uuid.IsZero()) {
				VetisApplicationBlock local_reply;
				if(GetStockEntryByUuid(stock_entry_uuid, local_reply) > 0) {
					for(uint i = 0; i < local_reply.VetStockList.getCount(); i++) {
						const VetisStockEntry * p_item = local_reply.VetStockList.at(i);
						if(p_item) {
							THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 1/*use_ta - мы вышли из предыдущей транзакции*/));
						}
					}
				}
			}
		}
	}
	if(ok < 0 && oneof2(rReply.ApplicationStatus, VetisApplicationBlock::appstCompleted, VetisApplicationBlock::appstRejected)) {
		rEntry.AppId.Z();
		rEntry.State = rEntry.stReply;
		THROW(PeC.SetOutgoingDocApplicationIdent(rEntry.PrepEntityID, rEntry.AppId, 1));
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::SearchLastStockEntry(PPID docEntityID, VetisVetDocument & rDocEntity, PPID & rStockEntryEntityID, S_GUID & rStockEntryGUID, S_GUID & rStockEntryUUID, double & rRest)
{
	rDocEntity.Z();
	rStockEntryEntityID = 0;
	rStockEntryGUID.Z();
	rStockEntryUUID.Z();
	rRest = 0.0;
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	SString fmt_buf;
	VetisEntityCore::Entity sub_entity;
	THROW(PeC.Get(docEntityID, rDocEntity) > 0);
	{
		long   max_stock_id = 0;
		long   stock_ent_flags = 0;
		long   stock_rec_flags = 0;
		LDATE  stock_dt = ZERODATE;
		double stock_rest = 0.0;
		VetisDocumentTbl::Key9 k9;
		TSVector <VetisDocumentTbl::Rec> stock_rec_list;
		k9.OrgDocEntityID = docEntityID/*app_data.VdRec.OrgDocEntityID*/;
		if(PeC.DT.search(9, &k9, spEq)) do {
			if(PeC.DT.data.VetisDocStatus == vetisdocstSTOCK) {
				THROW_SL(stock_rec_list.insert(&PeC.DT.data));
			}
		} while(PeC.DT.search(9, &k9, spNext) && PeC.DT.data.OrgDocEntityID == docEntityID/*app_data.VdRec.OrgDocEntityID*/);
		{
			LongArray final_pos_list;
			LongArray act_pos_list;
			LongArray last_pos_list;
			int  final_pos = -1;
			uint sidx;
			for(sidx = 0; sidx < stock_rec_list.getCount(); sidx++) {
				const VetisDocumentTbl::Rec & r_item = stock_rec_list.at(sidx);
				if(r_item.Flags & VetisGenericVersioningEntity::fActive && r_item.Flags & VetisGenericVersioningEntity::fLast)
					final_pos_list.add(sidx);
				else if(r_item.Flags & VetisGenericVersioningEntity::fActive)
					act_pos_list.add(sidx);
				else if(r_item.Flags & VetisGenericVersioningEntity::fLast)
					last_pos_list.add(sidx);
			}
			if(!final_pos_list.getCount()) {
				final_pos_list.add(&last_pos_list);
				final_pos_list.add(&act_pos_list);
			}
			if(!final_pos_list.getCount()) {
				for(uint k = 0; k < stock_rec_list.getCount(); k++)
					final_pos_list.add(k);
			}
			if(final_pos_list.getCount()) {
				for(uint k = 0; k < final_pos_list.getCount(); k++) {
					const uint pos = (uint)final_pos_list.get(k);
					const VetisDocumentTbl::Rec & r_item = stock_rec_list.at(pos);
					PeC.GetEntity(r_item.EntityID, sub_entity);
					if(!max_stock_id || sub_entity.ID > max_stock_id) {
						rStockEntryGUID = sub_entity.Guid;
						rStockEntryUUID = sub_entity.Uuid;
						rStockEntryEntityID = sub_entity.ID;
						rRest = r_item.Volume;
						stock_dt = r_item.IssueDate;
						stock_rest = r_item.Volume;
						stock_ent_flags = sub_entity.Flags;
						stock_rec_flags = r_item.Flags;
						max_stock_id = sub_entity.ID;
						ok = 1;
					}
				}
			}
		}
		temp_buf.Z().Cat(rDocEntity.WayBillNumber).CatDiv('-', 0).Cat(rDocEntity.WayBillDate).CatChar('-').Cat(rDocEntity.NativeBillID);
		THROW_PP_S(!!rStockEntryGUID && !!rStockEntryUUID, PPERR_VETISHASNTSTOCKREF, temp_buf);
		if(P_Logger) {
			PPLoadText(PPTXT_VETISOUTGSTOCKENTRY, fmt_buf);
			temp_buf.Z().CatChar('#').Cat(max_stock_id).Space().Cat(rStockEntryUUID, S_GUID::fmtIDL).
				Space().Cat(stock_dt, DATF_DMY).Space().Cat(stock_rest, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			if(stock_ent_flags & VetisGenericVersioningEntity::fActive)
				temp_buf.Space().Cat("ent").CatChar('-').Cat("ACTIVE");
			if(stock_ent_flags & VetisGenericVersioningEntity::fLast)
				temp_buf.Space().Cat("ent").CatChar('-').Cat("LAST");
			if(stock_rec_flags & VetisGenericVersioningEntity::fActive)
				temp_buf.Space().Cat("rec").CatChar('-').Cat("ACTIVE");
			if(stock_rec_flags & VetisGenericVersioningEntity::fLast)
				temp_buf.Space().Cat("rec").CatChar('-').Cat("LAST");
			msg_buf.Printf(fmt_buf, temp_buf.cptr());
			P_Logger->Log(msg_buf);
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::PrepareOutgoingConsignment2(OutcomingEntry & rEntry, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisVetDocument org_doc_entity;
	VetisApplicationBlock org_doc_reply;
	VetisPrepareOutgoingConsignmentRequest app_data;
	VetisApplicationBlock submit_result;
	VetisApplicationBlock blk(SVerT(2, 1), &app_data);
	VetisEntityCore::Entity sub_entity;
	SString temp_buf;
	SString msg_buf;
	THROW(PeC.SearchDocument(rEntry.PrepEntityID, &app_data.VdRec) > 0);
	temp_buf.Z().Cat(app_data.VdRec.WayBillNumber).CatDiv('-', 0).Cat(app_data.VdRec.WayBillDate).CatDiv('-', 0).Cat(app_data.VdRec.LinkBillRow);
	THROW_PP_S(app_data.VdRec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, rEntry.PrepEntityID);
	THROW_PP_S(app_data.VdRec.OrgDocEntityID, PPERR_VETISDOCRECDONTREFORGDOC, temp_buf);
	{
		PPID   stock_entry_entity_id = 0;
		double stock_rest = 0.0;
		THROW(SearchLastStockEntry(app_data.VdRec.OrgDocEntityID, org_doc_entity, stock_entry_entity_id, app_data.StockEntryGuid, app_data.StockEntryUuid, stock_rest));
	}
	if(GetVetDocumentByUuid(org_doc_entity.Uuid, org_doc_reply) > 0 && org_doc_reply.VetDocList.getCount() == 1) {
		rReply.Clear();
		app_data.OrgDoc = *org_doc_reply.VetDocList.at(0);
		// @v11.0.1 {
		{
			//
			// Выше была проверка на срок годности, но там этот срок брался из внутренней базы данных.
			// Иногда что-то идет не так и проскакивают истекшие сроки годности, потому еще раз сверимся с 
			// сертификатом, только что полученным с сервера.
			//
			const LDATE final_expiry_date = GetFinalExpiryData(app_data.OrgDoc.CertifiedConsignment.Batch.ExpiryDate);
			if(final_expiry_date) {
				msg_buf.Z().Cat(app_data.VdRec.WayBillNumber).CatDiv('-', 0).Cat(app_data.VdRec.WayBillDate).CatDiv('-', 0).Cat(app_data.VdRec.LinkBillRow);
				temp_buf.Z().Cat(final_expiry_date, DATF_MDY).Space().Cat(msg_buf);
				THROW_PP_S(final_expiry_date > getcurdate_(), PPERR_VETISORGDOCEXPIRY, temp_buf);
			}
		}
		// } @v11.0.1
		app_data.P_RegionRules = rEntry.P_RegRuleList; // @v11.0.11
		{
			THROW_PP(app_data.VdRec.FromEntityID, PPERR_VETISFROMBUSENTUNDEF);
			PeC.GetEntity(app_data.VdRec.FromEntityID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISFROMBUSENTUNDEF);
			app_data.FromBusinessEntGuid = sub_entity.Guid;
			THROW_PP(app_data.VdRec.FromEnterpriseID, PPERR_VETISFROMENTERPRUNDEF);
			PeC.GetEntity(app_data.VdRec.FromEnterpriseID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISFROMENTERPRUNDEF);
			app_data.FromEnterpiseGuid = sub_entity.Guid;
			THROW_PP(app_data.VdRec.ToEntityID, PPERR_VETISTOBUSENTUNDEF);
			PeC.GetEntity(app_data.VdRec.ToEntityID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISTOBUSENTUNDEF);
			app_data.ToBusinessEntGuid = sub_entity.Guid;
			THROW_PP(app_data.VdRec.ToEnterpriseID, PPERR_VETISTOENTERPRUNDEF);
			PeC.GetEntity(app_data.VdRec.ToEnterpriseID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISTOENTERPRUNDEF);
			app_data.ToEnterpiseGuid = sub_entity.Guid;
		}
		PrepareOutgoingTransportData(app_data.VdRec.LinkBillID, app_data);
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			rEntry.State = rEntry.stSent;
			rEntry.AppId = submit_result.ApplicationId;
			THROW(PeC.SetOutgoingDocApplicationIdent(rEntry.PrepEntityID, rEntry.AppId, 1));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPVetisInterface::PrepareOutgoingTransportData(PPID billID, VetisPrepareOutgoingConsignmentRequest & rReq)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	Reference * p_ref = PPRef;
	SString temp_buf;
	PPID   transport_id = 0;
	PPID   dlvr_loc_id = 0;
	int    tst = -1;
	int    tst_goods = -1;
	BillTbl::Rec bill_rec;
	if(p_bobj->Fetch(billID, &bill_rec) > 0) {
		const  PPID cli_psn_id = ObjectToPerson(bill_rec.Object, 0); // @v11.7.4
		if(rReq.VdRec.LinkGoodsID) {
			ObjTagItem tag_item;
			if(p_ref->Ot.GetTag(PPOBJ_GOODS, rReq.VdRec.LinkGoodsID, PPTAG_GOODS_TRANSPVANTYPE, &tag_item) > 0) {
				PPID   en_id = 0;
				if(tag_item.GetEnumData(&en_id, 0, 0, &temp_buf) > 0)
					tst_goods = SIntToSymbTab_GetId(VetisTranspStorageType_SymbTab, SIZEOFARRAY(VetisTranspStorageType_SymbTab), temp_buf);
			}
		}
		{
			PPFreight freight;
			PPFreight * p_freight = 0;
			if(p_bobj->P_Tbl->GetFreight(billID, &freight) > 0) {
				p_freight = &freight;
				// @v12.1.11 dlvr_loc_id = freight.DlvrAddrID__;
				transport_id = freight.ShipID;
			}
			p_bobj->GetDlvrAddrID(bill_rec, p_freight, &dlvr_loc_id); // @v12.1.11 
		}
		if(!transport_id && dlvr_loc_id && DlvrLocToTranspTagID) {
			ObjTagItem tag_item;
			if(p_ref->Ot.GetTag(PPOBJ_LOCATION, dlvr_loc_id, DlvrLocToTranspTagID, &tag_item) > 0)
				transport_id = tag_item.Val.IntVal;
		}
		if(!transport_id && cli_psn_id && PersonToTranspTagID) {
			ObjTagItem tag_item;
			if(p_ref->Ot.GetTag(PPOBJ_PERSON, cli_psn_id, PersonToTranspTagID, &tag_item) > 0)
				transport_id = tag_item.Val.IntVal;
		}
		if(transport_id) {
			PPObjTransport tr_obj;
			PPTransportPacket tr_pack;
			if(tr_obj.Get(transport_id, &tr_pack) > 0) {
				long   transport_type = tr_pack.Rec.TrType;
				if(transport_type == PPTRTYP_CAR) {
					rReq.Transp.TransportType = VetisTransportInfo::ttCar;
					rReq.Transp.TransportNumber.VehicleNumber = tr_pack.Rec.Code;
					rReq.Transp.TransportNumber.TrailerNumber = tr_pack.Rec.TrailerCode;
					{
						switch(tr_pack.Rec.VanType) {
							case PPTransport::vantypFrozen: tst = vtstFROZEN; break;
							case PPTransport::vantypChilled: tst = vtstCHILLED; break;
							case PPTransport::vantypCooled: tst = vtstCOOLED; break;
							case PPTransport::vantypVentilated: tst = vtstVENTILATED; break;
							default: tst = tst_goods; break;
						}
						if(tst >= 0) {
							SIntToSymbTab_GetSymb(VetisTranspStorageType_SymbTab, SIZEOFARRAY(VetisTranspStorageType_SymbTab), tst, temp_buf);
							rReq.TranspStorageType = temp_buf;
						}
					}
				}
				else if(transport_type == PPTRTYP_SHIP) {
					rReq.Transp.TransportType = VetisTransportInfo::ttShip;
					if(tr_pack.Rec.Code[0])
						rReq.Transp.TransportNumber.ShipName = tr_pack.Rec.Code;
					else
						rReq.Transp.TransportNumber.ShipName = tr_pack.Rec.Name;
				}
			}
		}
		if(rReq.Transp.TransportNumber.IsEmpty()) {
			rReq.Transp.TransportType = VetisTransportInfo::ttCar;
			// @v11.7.5 rReq.Transp.TransportNumber.VehicleNumber = "М 040 РВ 10";
			PPLoadText(PPTXT_VETIS_FAKECARNUMBER, rReq.Transp.TransportNumber.VehicleNumber); // @v11.7.5 
			if(tst_goods >= 0) {
				SIntToSymbTab_GetSymb(VetisTranspStorageType_SymbTab, SIZEOFARRAY(VetisTranspStorageType_SymbTab), tst_goods, temp_buf);
				rReq.TranspStorageType = temp_buf;
			}
		}
	}
	return ok;
}

int PPVetisInterface::PrepareOutgoingConsignment(PPID docEntityID, const TSCollection <VetisRouteSectionR13nRules> * pRegionRules, 
	TSVector <VetisEntityCore::UnresolvedEntity> * pUreList, VetisApplicationBlock & rReply)
{
	int    ok = -1;
	VetisVetDocument org_doc_entity;
	VetisApplicationBlock org_doc_reply;
	VetisPrepareOutgoingConsignmentRequest app_data;
	VetisApplicationBlock submit_result;
	VetisApplicationBlock blk(SVerT(2, 1), &app_data);
	VetisEntityCore::Entity sub_entity;
	SString temp_buf;
	SString msg_buf;
	THROW(PeC.SearchDocument(docEntityID, &app_data.VdRec) > 0);
	msg_buf.Z().Cat(app_data.VdRec.WayBillNumber).CatDiv('-', 0).Cat(app_data.VdRec.WayBillDate).CatDiv('-', 0).Cat(app_data.VdRec.LinkBillRow);
	THROW_PP_S(app_data.VdRec.VetisDocStatus == vetisdocstOUTGOING_PREPARING, PPERR_VETISVETDOCOUTPREPEXP, docEntityID);
	THROW_PP_S(app_data.VdRec.OrgDocEntityID, PPERR_VETISDOCRECDONTREFORGDOC, msg_buf);
	THROW(PeC.Get(app_data.VdRec.OrgDocEntityID, org_doc_entity) > 0);
	{
		const LDATE final_expiry_date = GetFinalExpiryData(org_doc_entity.CertifiedConsignment.Batch.ExpiryDate);
		if(final_expiry_date) {
			temp_buf.Z().Cat(final_expiry_date, DATF_MDY).Space().Cat(msg_buf);
			THROW_PP_S(final_expiry_date > getcurdate_(), PPERR_VETISORGDOCEXPIRY, temp_buf);
		}
	}
	{
		PPID   stock_entry_entity_id = 0;
		double stock_rest = 0.0;
		THROW(SearchLastStockEntry(app_data.VdRec.OrgDocEntityID, org_doc_entity, stock_entry_entity_id, app_data.StockEntryGuid, app_data.StockEntryUuid, stock_rest));
	}
	if(GetVetDocumentByUuid(org_doc_entity.Uuid, org_doc_reply) > 0 && org_doc_reply.VetDocList.getCount() == 1) {
		rReply.Clear();
		app_data.OrgDoc = *org_doc_reply.VetDocList.at(0);
		// @v11.0.1 {
		{
			//
			// Выше была проверка на срок годности, но там этот срок брался из внутренней базы данных.
			// Иногда что-то идет не так и проскакивают истекшие сроки годности, потому еще раз сверимся с 
			// сертификатом, только что полученным с сервера.
			//
			const LDATE final_expiry_date = GetFinalExpiryData(app_data.OrgDoc.CertifiedConsignment.Batch.ExpiryDate);
			if(final_expiry_date) {
				temp_buf.Z().Cat(final_expiry_date, DATF_MDY).Space().Cat(msg_buf);
				THROW_PP_S(final_expiry_date > getcurdate_(), PPERR_VETISORGDOCEXPIRY, temp_buf);
			}
		}
		// } @v11.0.1
		{
			THROW_PP(app_data.VdRec.FromEntityID, PPERR_VETISFROMBUSENTUNDEF);
			PeC.GetEntity(app_data.VdRec.FromEntityID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISFROMBUSENTUNDEF);
			app_data.FromBusinessEntGuid = sub_entity.Guid;
			THROW_PP(app_data.VdRec.FromEnterpriseID, PPERR_VETISFROMENTERPRUNDEF);
			PeC.GetEntity(app_data.VdRec.FromEnterpriseID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISFROMENTERPRUNDEF);
			app_data.FromEnterpiseGuid = sub_entity.Guid;
			THROW_PP(app_data.VdRec.ToEntityID, PPERR_VETISTOBUSENTUNDEF);
			PeC.GetEntity(app_data.VdRec.ToEntityID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISTOBUSENTUNDEF);
			app_data.ToBusinessEntGuid = sub_entity.Guid;
			THROW_PP(app_data.VdRec.ToEnterpriseID, PPERR_VETISTOENTERPRUNDEF);
			PeC.GetEntity(app_data.VdRec.ToEnterpriseID, sub_entity);
			THROW_PP(!!sub_entity.Guid, PPERR_VETISTOENTERPRUNDEF);
			app_data.ToEnterpiseGuid = sub_entity.Guid;
		}
		PrepareOutgoingTransportData(app_data.VdRec.LinkBillID, app_data);
		// @v11.0.11 {
		if(pRegionRules && pRegionRules->getCount()) {
			app_data.P_RegionRules = pRegionRules; 
		}
		// } @v11.0.11
		THROW(SubmitRequest(blk, submit_result));
		if(submit_result.ApplicationStatus == VetisApplicationBlock::appstAccepted) {
			THROW(ReceiveResult(submit_result.ApplicationId, 0, rReply, 0/*once*/));
			if(rReply.ApplicationStatus == VetisApplicationBlock::appstCompleted) {
				if(rReply.VetDocList.getCount() == 1) {
					VetisVetDocument * p_item = rReply.VetDocList.at(0);
					S_GUID stock_entry_uuid;
					PPTransaction tra(1);
					THROW(tra);
					if(p_item) {
						p_item->NativeBillID = app_data.VdRec.LinkBillID;
						p_item->NativeBillRow = app_data.VdRec.LinkBillRow;
						p_item->CertifiedConsignment.Batch.NativeGoodsID = app_data.VdRec.LinkGoodsID;
						THROW(PeC.Put(0, *p_item, 0, pUreList, 0));
						THROW(PeC.DeleteEntity(docEntityID, 0)); // Создав запись сертификата, необходимо удалить запись подготовки транспортного документа
						ok = 1;
					}
					if(rReply.VetStockList.getCount()) {
						for(uint i = 0; i < rReply.VetStockList.getCount(); i++) {
							const VetisStockEntry * p_item = rReply.VetStockList.at(i);
							if(p_item) {
								// Полученная с этим ответом запись складского журнала не содержит ссылку на оригинальный сертификат.
								// Поэтому придется запросить отдельно.
								stock_entry_uuid = p_item->Uuid;
								//THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 0));
							}
						}
					}
					THROW(tra.Commit());
					if(!stock_entry_uuid.IsZero()) {
						VetisApplicationBlock local_reply;
						if(GetStockEntryByUuid(stock_entry_uuid, local_reply) > 0) {
							for(uint i = 0; i < local_reply.VetStockList.getCount(); i++) {
								const VetisStockEntry * p_item = local_reply.VetStockList.at(i);
								if(p_item) {
									THROW(PeC.Put(0, P.IssuerUUID, P.EntUUID, *p_item, pUreList, 1/*use_ta - мы вышли из предыдущей транзакции*/));
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

static void Debug_OutputProductItem(const VetisProductItem & rItem, SString & rBuf)
{
	rBuf.Z();
	rBuf.Cat(rItem.Uuid, S_GUID::fmtIDL).Tab().Cat(rItem.Guid, S_GUID::fmtIDL);
	rBuf.Tab().Cat(rItem.GlobalID);
	rBuf.Tab().Cat(rItem.Code);
	rBuf.Tab().Cat(rItem.Name);
	rBuf.Tab().Cat(rItem.ProductType);
	rBuf.Tab().CatHex(rItem.Flags);
	rBuf.Tab().Cat(rItem.Gost);
}

static void Debug_OutputEntItem(const VetisEnterprise & rItem, SString & rBuf)
{
	SString temp_buf;
	rBuf.Z();
	rBuf.Cat(rItem.Uuid, S_GUID::fmtIDL).Tab().Cat(rItem.Guid, S_GUID::fmtIDL);
	rBuf.Tab().Cat(rItem.Name).Tab().Cat(rItem.EnglishName);
	rBuf.Tab().Cat(rItem.Address.AddressView);
	rBuf.Tab();
	for(uint ssp = 0; rItem.NumberList.get(&ssp, temp_buf);) {
		rBuf.Cat(temp_buf).Space();
	}
}

int PPVetisInterface::ProcessUnresolvedEntityList(const TSVector <VetisEntityCore::UnresolvedEntity> & rList)
{
	int    ok = 1;
	SString temp_buf;
	for(uint i = 0; i < rList.getCount(); i++) {
		const VetisEntityCore::UnresolvedEntity & r_entry = rList.at(i);
		VetisApplicationBlock reply(SVerT(2, 1), 0);
		S_GUID guid;
		int    rr = 1;
		PPID   entity_id = 0;
		if(r_entry.Kind == VetisEntityCore::kProductItem) {
			if(r_entry.GuidRef && PeC.UrT.Search(r_entry.GuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtProductItemByGuid, VGuidToStr(guid, temp_buf), reply);
			else if(r_entry.UuidRef && PeC.UrT.Search(r_entry.UuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtProductItemByUuid, VGuidToStr(guid, temp_buf), reply);
			THROW(rr);
			if(rr > 0) {
				for(uint j = 0; j < reply.ProductItemList.getCount(); j++) {
					THROW(PeC.Put(&entity_id, r_entry.Kind, *reply.ProductItemList.at(j), 0, 1));
				}
			}
		}
		else if(r_entry.Kind == VetisEntityCore::kProduct) {
			if(r_entry.GuidRef && PeC.UrT.Search(r_entry.GuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtProductByGuid, VGuidToStr(guid, temp_buf), reply);
			else if(r_entry.UuidRef && PeC.UrT.Search(r_entry.UuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtProductByUuid, VGuidToStr(guid, temp_buf), reply);
			THROW(rr);
		}
		else if(r_entry.Kind == VetisEntityCore::kSubProduct) {
			if(r_entry.GuidRef && PeC.UrT.Search(r_entry.GuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtSubProductByGuid, VGuidToStr(guid, temp_buf), reply);
			else if(r_entry.UuidRef && PeC.UrT.Search(r_entry.UuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtSubProductByUuid, VGuidToStr(guid, temp_buf), reply);
			THROW(rr);
		}
		else if(r_entry.Kind == VetisEntityCore::kEnterprise) {
			if(r_entry.GuidRef && PeC.UrT.Search(r_entry.GuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtEnterpriseByGuid, VGuidToStr(guid, temp_buf), reply);
			else if(r_entry.UuidRef && PeC.UrT.Search(r_entry.UuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtEnterpriseByUuid, VGuidToStr(guid, temp_buf), reply);
			THROW(rr);
			if(rr > 0) {
				for(uint j = 0; j < reply.EntItemList.getCount(); j++) {
					THROW(PeC.Put(&entity_id, *reply.EntItemList.at(j), 0, 1));
				}
			}
		}
		else if(r_entry.Kind == VetisEntityCore::kBusinessEntity) {
			if(r_entry.GuidRef && PeC.UrT.Search(r_entry.GuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtBusinessEntityByGuid, VGuidToStr(guid, temp_buf), reply);
			else if(r_entry.UuidRef && PeC.UrT.Search(r_entry.UuidRef, guid) > 0 && !guid.IsZero())
				rr = GetEntityQuery2(qtBusinessEntityByUuid, VGuidToStr(guid, temp_buf), reply);
			THROW(rr);
			if(rr > 0) {
				for(uint j = 0; j < reply.BEntList.getCount(); j++) {
					THROW(PeC.Put(&entity_id, *reply.BEntList.at(j), 0, 1));
				}
			}
		}
		PPWaitPercent(i+1, rList.getCount(), 0);
	}
	CATCHZOK
	return ok;
}

struct VetisTestParam {
	VetisTestParam() : Flags(0)
	{
	}
	enum {
		fRcptProductRef   = 0x0001,
		fRcptEnterpriseRef        = 0x0002,
		fRcptBusinessEntityRef    = 0x0004,
		fRcptUnitRef      = 0x0008,
		fRcptPurposeRef   = 0x0010,
		fRcptCountryRef   = 0x0020,
		fRcptRegionRef    = 0x0040,
		fRcptLocalityRef  = 0x0080,
		fRcptStockEntryList       = 0x0100,
		fRcptProductChangesList   = 0x0200,
		fRcptSubProductChangesList = 0x0400
	};
	long    Flags;
	SString ParamText;
};

class VetisTestParamDialog : public TDialog {
	DECL_DIALOG_DATA(VetisTestParam);
public:
	VetisTestParamDialog() : TDialog(DLG_TESTVETIS)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 0, Data.fRcptProductRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 1, Data.fRcptEnterpriseRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 2, Data.fRcptBusinessEntityRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 3, Data.fRcptUnitRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 4, Data.fRcptPurposeRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 5, Data.fRcptCountryRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 6, Data.fRcptRegionRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 7, Data.fRcptLocalityRef);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 8, Data.fRcptProductChangesList);
		AddClusterAssoc(CTL_TESTVETIS_FLAGS, 9, Data.fRcptSubProductChangesList);
		SetClusterData(CTL_TESTVETIS_FLAGS, Data.Flags);
		setCtrlString(CTL_TESTVETIS_PARAM, Data.ParamText);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetClusterData(CTL_TESTVETIS_FLAGS, &Data.Flags);
		getCtrlString(CTL_TESTVETIS_PARAM, Data.ParamText);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int TestVetis()
{
	int    ok = 1;
	SString temp_buf;
	PPLogger logger;
	PPObjGoods goods_obj;
	VetisTestParam test_param;
	PPVetisInterface::Param param(0, 0, PPVetisInterface::Param::fSkipLocInitialization);
	TSVector <VetisEntityCore::UnresolvedEntity> ure_list;
	THROW(PPVetisInterface::SetupParam(param));
	if(PPDialogProcBody <VetisTestParamDialog, VetisTestParam>(&test_param) > 0) {
		PPWaitStart();
		PPVetisInterface ifc(&logger);
		THROW(ifc.Init(param));
		if(test_param.Flags & test_param.fRcptCountryRef) {
			VetisApplicationBlock reply;
			if(ifc.GetCountryList(reply) > 0) {
				PPGetFilePath(PPPATH_LOG, "vetis_country_list.log", temp_buf);
				SFile f_out(temp_buf, SFile::mWrite);
				for(uint i = 0; i < reply.CountryList.getCount(); i++) {
					const VetisCountry * p_item = reply.CountryList.at(i);
					if(p_item) {
						temp_buf.Z();
						temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
						temp_buf.Tab().Cat(p_item->Code).Tab().Cat(p_item->Code3).Tab().Cat(p_item->Name).Tab().Cat(p_item->EnglishName);
						f_out.WriteLine(temp_buf.CR());
					}
				}
			}
			// Russia GUID: 74A3CBB1-56FA-94F3-AB3F-E8DB4940D96B
		}
		if(test_param.Flags & test_param.fRcptRegionRef) {
			VetisApplicationBlock reply;
			if(ifc.GetRegionList(S_GUID(/*P_VetisGuid_Country_Ru*/test_param.ParamText), reply) > 0) {
				PPGetFilePath(PPPATH_LOG, "vetis_region_list.log", temp_buf);
				SFile f_out(temp_buf, SFile::mWrite);
				for(uint i = 0; i < reply.RegionList.getCount(); i++) {
					const VetisAddressObjectView * p_item = reply.RegionList.at(i);
					if(p_item) {
						temp_buf.Z();
						temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
						temp_buf.Tab().Cat(p_item->CountryGUID, S_GUID::fmtIDL).Tab().Cat(p_item->RegionCode).Tab().Cat(p_item->Type).Tab().
							Cat(p_item->Name).Tab().Cat(p_item->EnglishName).Tab().Cat(p_item->View);
						f_out.WriteLine(temp_buf.CR());
					}
				}
			}
			// Karelia GUID: 248D8071-06E1-425E-A1CF-D1FF4C4A14A8
		}
		if(test_param.Flags & test_param.fRcptLocalityRef) {
			// @todo
		}
		if(test_param.Flags & test_param.fRcptPurposeRef) {
			VetisApplicationBlock reply;
			ifc.GetPurposeList(0, 1000, reply);
		}
		if(test_param.Flags & test_param.fRcptUnitRef) {
			VetisApplicationBlock reply;
			uint req_count = 1000;
			PPGetFilePath(PPPATH_LOG, "vetis_unit_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			for(uint req_offs = 0; ifc.GetUnitList(req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.UnitList.getCount(); i++) {
					const VetisUnit * p_item = reply.UnitList.at(i);
					if(p_item) {
						temp_buf.Z();
						temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
						temp_buf.Tab().Cat(p_item->Name).Tab().Cat(p_item->CommonUnitGuid, S_GUID::fmtIDL).Tab().Cat(p_item->Factor, MKSFMTD(0, 6, 0));
						f_out.WriteLine(temp_buf.CR());
						{
							VetisEntityCore::Entity sub_entity(VetisEntityCore::kUOM, *p_item);
							THROW(ifc.PeC.SetEntity(sub_entity, /*pUreList*/0, 0, 0));
							//rec.UOMID = sub_entity.ID;
						}
					}
				}
				THROW(tra.Commit());
				if(reply.UnitList.getCount() < req_count)
					break;
				else
					req_offs += reply.UnitList.getCount();
			}
			// THROW(SetEntity(Entity(kUOM, r_bat.Unit), pUreList, &rec.UOMID, 0));
		}
		/*{
			THROW(ifc.PeC.CollectUnresolvedEntityList(ure_list));
			THROW(ifc.ProcessUnresolvedEntityList(ure_list));
			ure_list.clear();
		}*/
		if(test_param.Flags & test_param.fRcptStockEntryList) {
			VetisApplicationBlock reply;
			uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetStockEntryList(req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.VetStockList.getCount(); i++) {
					const VetisStockEntry * p_item = reply.VetStockList.at(i);
					if(p_item) {
						//PPID   pi_id = 0;
						//THROW(ifc.PeC.Put(&pi_id, *p_item, &ure_list, 0));
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
				if(reply.VetStockList.getCount() < req_count)
					break;
				else
					req_offs += reply.VetStockList.getCount();
			}
			//THROW(ifc.ProcessUnresolvedEntityList(ure_list));
		}
#if 0 // {
		{
			VetisApplicationBlock reply(1, 0);
			uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetVetDocumentList(req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.VetDocList.getCount(); i++) {
					const VetisVetDocument * p_item = reply.VetDocList.at(i);
					if(p_item) {
						PPID   pi_id = 0;
						THROW(ifc.PeC.Put(&pi_id, *p_item, &ure_list, 0));
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
				if(reply.VetDocList.getCount() < req_count)
					break;
				else
					req_offs += reply.VetDocList.getCount();
			}
			THROW(ifc.ProcessUnresolvedEntityList(ure_list));
		}
		{
			VetisApplicationBlock reply;
			temp_buf = "fcbef59e-218a-11e2-a69b-b499babae7ea"; // ЗАО "Митлэнд inn 7816358675
			ifc.GetEntityQuery2(ifc.qtBusinessEntityByGuid, temp_buf, reply);
		}
		{
			VetisApplicationBlock reply;
			temp_buf = "04ac940e-053d-11e1-99b4-d8d385fbc9e8"; // ЗАО "Митлэнд inn 7816358675
			ifc.GetEntityQuery2(ifc.qtBusinessEntityByUuid, temp_buf, reply);
		}
		{
			VetisApplicationBlock reply;
			temp_buf = "1c44b8ce-4e22-f1ee-0446-1844351a5838"; // ООО "Тельпас"
			ifc.GetEntityQuery2(ifc.qtEnterpriseByGuid, temp_buf, reply);
		}
		{
			VetisApplicationBlock reply;
			temp_buf = "075985c2-053d-11e1-99b4-d8d385fbc9e8"; // ООО "Тельпас"
			ifc.GetEntityQuery2(ifc.qtEnterpriseByUuid, temp_buf, reply);
		}
		{
			VetisApplicationBlock reply;
			temp_buf = "69bf7cfb-fd32-4d2b-99d6-7629dc4ad040"; // Колбаса "Любительская" вареная
			ifc.GetEntityQuery2(ifc.qtProductItemByGuid, temp_buf, reply);
		}
		{
			VetisApplicationBlock reply;
			temp_buf = "aceefd38-40c4-4276-96ad-039ed113a156"; // Колбаса "Любительская" вареная
			ifc.GetEntityQuery2(ifc.qtProductItemByUuid, temp_buf, reply);
		}
#endif // } 0
		if(test_param.Flags & test_param.fRcptBusinessEntityRef) {
			const uint max_zeroresult_tries = 3;
			uint  zeroresult_try = 0;
			PPGetFilePath(PPPATH_LOG, "vetis_bent_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			uint req_count = 1000;
			VetisApplicationBlock reply;
			for(uint req_offs = 0; ifc.GetBusinessEntityList(req_offs, req_count, reply);) {
				if(reply.BEntList.getCount() == 0) {
					if(zeroresult_try < max_zeroresult_tries)
						zeroresult_try++;
					else
						break;
				}
				else {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < reply.BEntList.getCount(); i++) {
						const VetisBusinessEntity * p_item = reply.BEntList.at(i);
						if(p_item) {
							temp_buf.Z();
							temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
							temp_buf.Tab().Cat(p_item->Name).Tab().Cat(p_item->Fio).Tab().Cat(p_item->Inn).Tab().Cat(p_item->Kpp);
							temp_buf.Tab().Cat(p_item->JuridicalAddress.AddressView);
							f_out.WriteLine(temp_buf.CR());
							PPID   pi_id = 0;
							THROW(ifc.PeC.Put(&pi_id, *p_item, &ure_list, 0));
						}
					}
					THROW(tra.Commit());
					PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
					zeroresult_try = 0;
					if(reply.BEntList.getCount() < req_count)
						break;
					else
						req_offs += reply.BEntList.getCount();
				}
			}
		}
		if(test_param.Flags & test_param.fRcptProductChangesList) {
			VetisApplicationBlock reply;
			PPGetFilePath(PPPATH_LOG, "vetis_product_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetProductChangesList(req_offs, req_count, ZERODATE, reply);) {
				for(uint i = 0; i < reply.ProductList.getCount(); i++) {
					const VetisProduct * p_item = reply.ProductList.at(i);
					if(p_item) {
						temp_buf.Z();
						temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
						temp_buf.Tab().Cat(p_item->Name).Tab().Cat(p_item->Code).Tab().Cat(p_item->ProductType);
						f_out.WriteLine(temp_buf.CR());
					}
				}
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
				if(reply.ProductList.getCount() < req_count)
					break;
				else
					req_offs += reply.ProductList.getCount();
			}
		}
		if(test_param.Flags & test_param.fRcptSubProductChangesList) {
			VetisApplicationBlock reply;
			PPGetFilePath(PPPATH_LOG, "vetis_subproduct_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetSubProductChangesList(req_offs, req_count, ZERODATE, reply);) {
				for(uint i = 0; i < reply.SubProductList.getCount(); i++) {
					const VetisSubProduct * p_item = reply.SubProductList.at(i);
					if(p_item) {
						temp_buf.Z();
						temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
						temp_buf.Tab().Cat(p_item->Name).Tab().Cat(p_item->Code).Tab().Cat(p_item->ProductGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
						f_out.WriteLine(temp_buf.CR());
					}
				}
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
				if(reply.SubProductList.getCount() < req_count)
					break;
				else
					req_offs += reply.SubProductList.getCount();
			}
		}
		if(test_param.Flags & test_param.fRcptProductRef) {
			VetisApplicationBlock reply;
			PPGetFilePath(PPPATH_LOG, "vetis_productitem_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetProductItemList(req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.ProductItemList.getCount(); i++) {
					VetisProductItem * p_item = reply.ProductItemList.at(i);
					if(p_item) {
						if(p_item->GlobalID.NotEmpty()) {
							BarcodeTbl::Rec bc_rec;
							if(goods_obj.SearchByBarcode(p_item->GlobalID, &bc_rec, 0, 1) > 0)
								p_item->NativeGoodsID = bc_rec.GoodsID;
						}
						Debug_OutputProductItem(*p_item, temp_buf);
						f_out.WriteLine(temp_buf.CR());
						PPID   pi_id = 0;
						THROW(ifc.PeC.Put(&pi_id, ifc.PeC.kProductItem, *p_item, &ure_list, 0));
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
				if(reply.ProductItemList.getCount() < req_count)
					break;
				else
					req_offs += reply.ProductItemList.getCount();
			}
		}
		if(test_param.Flags & test_param.fRcptEnterpriseRef) {
			PPGetFilePath(PPPATH_LOG, "vetis_ent_list.log", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			uint req_count = 1000;
			VetisApplicationBlock reply;
			for(uint req_offs = 0; ifc.GetRussianEnterpriseList(req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.EntItemList.getCount(); i++) {
					const VetisEnterprise * p_item = reply.EntItemList.at(i);
					if(p_item) {
						Debug_OutputEntItem(*p_item, temp_buf);
						f_out.WriteLine(temp_buf.CR());
						PPID   pi_id = 0;
						THROW(ifc.PeC.Put(&pi_id, *p_item, &ure_list, 0));
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
				if(reply.EntItemList.getCount() < req_count)
					break;
				else
					req_offs += reply.EntItemList.getCount();
			}
		}
	}
	CATCHZOKPPERR
	logger.Save(PPFILNAM_VETISINFO_LOG, 0);
	PPWaitStop();
	return ok;
}

double PPVetisInterface::CalcVolumeByGoodsQtty(PPID goodsID, double quantity)
{
	double phupu = 0.0;
	return (GObj.GetPhUPerU(goodsID, 0, &phupu) > 0 && phupu > 0.0) ? (fabs(quantity) * phupu) : 0.0;
}

int PPVetisInterface::PutBillRow(const PPBillPacket & rBp, uint rowIdx, long flags, PutBillRowBlock & rPbrBlk, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	const PPTransferItem & r_ti = rBp.ConstTI(rowIdx);
	if(r_ti.LotID) {
		SString temp_buf;
		SString msg_buf;
		SString fmt_buf;
		SString bill_text;
		S_GUID goods_guid;
		S_GUID lot_uuid; // UUID оригинального сертификата
		VetisEntityCore::Entity entity_doc;
		VetisDocumentTbl::Rec src_rec; // Запись оригинального сертификата
		VetisDocumentTbl::Key1 k1;
		const int org_vcert_needed = BIN(!(flags & (pbrfDiscrepancy|pbrfManufInc)) || rBp.OpTypeID == PPOPT_GOODSEXPEND);
		bool   skip = false;
		const double __volume = CalcVolumeByGoodsQtty(r_ti.GoodsID, r_ti.Quantity_);
		if(__volume <= 0.0)
			skip = true;
		else if(org_vcert_needed) {
			if(p_ref->Ot.GetTagStr(PPOBJ_LOT, r_ti.LotID, PPTAG_LOT_VETIS_UUID, temp_buf) > 0 && lot_uuid.FromStr(temp_buf)) {
				if(PeC.GetEntityByUuid(lot_uuid, entity_doc) > 0 && PeC.SearchDocument(entity_doc.ID, &src_rec) > 0) {
					;
				}
				else {
					temp_buf.Z();
					ReceiptTbl::Rec lot_rec;
					if(p_bobj->trfr->Rcpt.Search(r_ti.LotID, &lot_rec) > 0)
						p_bobj->MakeLotText(&lot_rec, PPObjBill::ltfGoodsName, temp_buf);
					else
						ideqvalstr(r_ti.LotID, temp_buf);
					CALLEXCEPT_PP_S(PPERR_VETISVETDOCBYLOTNFOUND, temp_buf);
				}
			}
			else
				skip = true;
		}
		if(!skip) {
			VetisDocumentTbl::Rec rec;
			{
				// @v11.1.12 BillCore::GetCode(temp_buf = rBp.Rec.Code);
				temp_buf = rBp.Rec.Code; // @v11.1.12 
				rec.IssueDate = getcurdate_();
				STRNSCPY(rec.IssueNumber, temp_buf);
				rec.WayBillDate = rBp.Rec.Dt;
				STRNSCPY(rec.WayBillNumber, temp_buf);
				rec.VetisDocStatus = vetisdocstOUTGOING_PREPARING; // ! important
				rec.LinkBillID = rBp.Rec.ID;
				rec.LinkBillRow = r_ti.RByBill;
				rec.LinkFromPsnID = P.MainOrgID;
				rec.LinkFromDlvrLocID = rBp.Rec.LocID;
				rec.LinkGoodsID = labs(r_ti.GoodsID);
				if(src_rec.EntityID) {
					rec.OrgDocEntityID = entity_doc.ID;
					rec.SubProductID = src_rec.SubProductID;
					rec.ProductID = src_rec.ProductID;
					rec.ProductItemID = src_rec.ProductItemID;
					rec.ProductType = src_rec.ProductType;
					rec.ExpiryFrom = src_rec.ExpiryFrom;
					rec.ExpiryTo = src_rec.ExpiryTo;
					rec.ManufDateFrom = src_rec.ManufDateFrom;
					rec.ManufDateTo = src_rec.ManufDateTo;
					rec.UOMID = src_rec.UOMID;
					rec.OrgCountryID = src_rec.OrgCountryID;
				}
			}
			PPTransaction tra(use_ta);
			THROW(tra);
			PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, bill_text);
			if(flags & pbrfDiscrepancy) {
				rec.Flags |= (VetisVetDocument::fDiscrepancy|VetisVetDocument::fFromMainOrg);
				if(rBp.OpTypeID == PPOPT_GOODSEXPEND) {
					assert(!lot_uuid.IsZero()); // Инициализирован выше
					assert(src_rec.EntityID != 0); // Инициализирован выше
					assert(__volume > 0.0); // Инициализирован выше
					rec.Volume = __volume; // ! Здесь указано израсходованное количество. В ветис мы должны будем передать остаток
					rec.Flags |= VetisVetDocument::fDiscrepancyLack;
					{
						MEMSZERO(k1);
						k1.LinkBillID = rBp.Rec.ID;
						k1.LinkBillRow = r_ti.RByBill;
						if(PeC.DT.searchForUpdate(1, &k1, spEq)) {
							do {
								VetisDocumentTbl::Rec ex_rec;
								PeC.DT.CopyBufTo(&ex_rec);
								if(ex_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
									rec.EntityID = ex_rec.EntityID;
									THROW_DB(PeC.DT.updateRecBuf(&rec));
									break;
								}
								if(ex_rec.VetisDocStatus == vetisdocstWITHDRAWN) {
									; // go to the next record
								}
								else {
									if(P_Logger) {
										// Строка документа уже имеет сертификат %s
										/*
										SString status_text;
										PPLoadText(PPTXT_VETISBILLROWALLREADYHASCERT, fmt_buf);
										if(!SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), src_rec.VetisDocStatus, status_text)) {
											status_text.Z().Cat(src_rec.VetisDocStatus);
										}
										(temp_buf = bill_text).CatDiv(':', 2).Cat(src_rec.EntityID).Space().Cat(src_rec.IssueDate, DATF_DMY).
											Space().Cat(src_rec.IssueNumber).Space().Cat(status_text);
										msg_buf.Printf(fmt_buf, temp_buf.cptr());
										P_Logger->Log(msg_buf);
										*/
									}
									break;
								}
							} while(PeC.DT.searchForUpdate(1, &k1, spNext) && PeC.DT.data.LinkBillID == rBp.Rec.ID && PeC.DT.data.LinkBillRow == r_ti.RByBill);
						}
						else {
							VetisEntityCore::Entity entity;
							entity.SetupVetDocument();
							THROW(PeC.SetEntity(entity, 0, &rec.EntityID, 0));
							THROW_DB(PeC.DT.insertRecBuf(&rec));
						}
					}
				}
				else if(p_ref->Ot.GetTagStr(PPOBJ_GOODS, labs(r_ti.GoodsID), PPTAG_GOODS_VETISGUID, temp_buf) > 0 && goods_guid.FromStr(temp_buf)) {
					VetisEntityCore::Entity entity_goods;
					VetisProductItem product_item;
					if(rPbrBlk.BillID != rBp.Rec.ID) {
						rPbrBlk.BillID = rBp.Rec.ID;
						rPbrBlk.PersonGuid.Z();
						rPbrBlk.DlvrLocGuid.Z();
					}
					rec.LinkToPsnID = rPbrBlk.PersonID;
					rec.LinkToDlvrLocID = rPbrBlk.DlvrLocID;
					int    is_product_resolved = 0;
					if(PeC.GetEntityByGuid(goods_guid, entity_goods) > 0 && PeC.Get(entity_goods.ID, product_item) > 0)
						is_product_resolved = 1;
					else {
						VetisApplicationBlock goods_reply;
						int rr = GetEntityQuery2(qtProductItemByGuid, VGuidToStr(goods_guid, temp_buf), goods_reply);
						if(rr > 0 && goods_reply.ProductItemList.getCount()) {
							PPID   entity_id = 0;
							THROW(PeC.Put(&entity_id, VetisEntityCore::kProductItem, *goods_reply.ProductItemList.at(0), 0, 0));
							if(PeC.GetEntityByGuid(goods_guid, entity_goods) > 0 && PeC.Get(entity_goods.ID, product_item) > 0)
								is_product_resolved = 1;
						}
					}
					if(is_product_resolved) {
						rec.ProductItemID = product_item.EntityID;
						const ObjTagItem * p_manuf_tm_tag = rBp.LTagL.GetTag(rowIdx, PPTAG_LOT_MANUFTIME);
						if(p_manuf_tm_tag) {
							LDATETIME manuf_dtm = ZERODATETIME;
							if(p_manuf_tm_tag->GetTimestamp(&manuf_dtm))
								rec.ManufDateTo = rec.ManufDateFrom = SUniTime(manuf_dtm).ToInt64();
						}
						if(checkdate(r_ti.Expiry))
							rec.ExpiryTo = rec.ExpiryFrom = SUniTime(r_ti.Expiry).ToInt64();
						assert(__volume > 0.0); // Инициализирован выше
						rec.Volume = __volume;
						{
							MEMSZERO(k1);
							k1.LinkBillID = rBp.Rec.ID;
							k1.LinkBillRow = r_ti.RByBill;
							if(PeC.DT.searchForUpdate(1, &k1, spEq)) {
								do {
									VetisDocumentTbl::Rec ex_rec;
									PeC.DT.CopyBufTo(&ex_rec);
									if(ex_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
										rec.EntityID = ex_rec.EntityID;
										THROW_DB(PeC.DT.updateRecBuf(&rec));
										break;
									}
									if(ex_rec.VetisDocStatus == vetisdocstWITHDRAWN) {
										; // go to the next record
									}
									else {
										if(P_Logger) {
											// Строка документа уже имеет сертификат %s
											/*
											SString status_text;
											PPLoadText(PPTXT_VETISBILLROWALLREADYHASCERT, fmt_buf);
											if(!SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), src_rec.VetisDocStatus, status_text)) {
												status_text.Z().Cat(src_rec.VetisDocStatus);
											}
											(temp_buf = bill_text).CatDiv(':', 2).Cat(src_rec.EntityID).Space().Cat(src_rec.IssueDate, DATF_DMY).
												Space().Cat(src_rec.IssueNumber).Space().Cat(status_text);
											msg_buf.Printf(fmt_buf, temp_buf.cptr());
											P_Logger->Log(msg_buf);
											*/
										}
										break;
									}
								} while(PeC.DT.searchForUpdate(1, &k1, spNext) && PeC.DT.data.LinkBillID == rBp.Rec.ID && PeC.DT.data.LinkBillRow == r_ti.RByBill);
							}
							else {
								VetisEntityCore::Entity entity;
								entity.SetupVetDocument();
								THROW(PeC.SetEntity(entity, 0, &rec.EntityID, 0));
								THROW_DB(PeC.DT.insertRecBuf(&rec));
							}
							ok = 1;
						}
					}
				}
			}
			else if(flags & pbrfManuf) {
				if(rPbrBlk.BillID != rBp.Rec.ID) {
					rPbrBlk.BillID = rBp.Rec.ID;
					rPbrBlk.PersonGuid.Z();
					rPbrBlk.DlvrLocGuid.Z();
					rPbrBlk.PersonID = 0;
					rPbrBlk.DlvrLocID = 0;
				}
				rec.Flags |= VetisVetDocument::fFromMainOrg;
				if(flags & pbrfManufInc) {
					if(r_ti.Quantity_ > 0.0) {
						rec.VetisDocType = vetisdoctypPRODUCTIVE;
						rec.Flags |= VetisVetDocument::fManufIncome;
						THROW(rPbrBlk.ManufIncomeDocEntityID == 0);
						if(p_ref->Ot.GetTagStr(PPOBJ_GOODS, labs(r_ti.GoodsID), PPTAG_GOODS_VETISGUID, temp_buf) > 0 && goods_guid.FromStr(temp_buf)) {
							VetisEntityCore::Entity entity_goods;
							VetisProductItem product_item;
							if(rPbrBlk.BillID != rBp.Rec.ID) {
								rPbrBlk.BillID = rBp.Rec.ID;
								rPbrBlk.PersonGuid.Z();
								rPbrBlk.DlvrLocGuid.Z();
							}
							rec.LinkToPsnID = rPbrBlk.PersonID;
							rec.LinkToDlvrLocID = rPbrBlk.DlvrLocID;
							int    is_product_resolved = 0;
							if(PeC.GetEntityByGuid(goods_guid, entity_goods) > 0 && PeC.Get(entity_goods.ID, product_item) > 0)
								is_product_resolved = 1;
							else {
								VetisApplicationBlock goods_reply;
								int rr = GetEntityQuery2(qtProductItemByGuid, VGuidToStr(goods_guid, temp_buf), goods_reply);
								if(rr > 0 && goods_reply.ProductItemList.getCount()) {
									PPID   entity_id = 0;
									THROW(PeC.Put(&entity_id, VetisEntityCore::kProductItem, *goods_reply.ProductItemList.at(0), 0, 0));
									if(PeC.GetEntityByGuid(goods_guid, entity_goods) > 0 && PeC.Get(entity_goods.ID, product_item) > 0)
										is_product_resolved = 1;
								}
							}
							if(is_product_resolved) {
								rec.ProductItemID = product_item.EntityID;
								const ObjTagItem * p_manuf_tm_tag = rBp.LTagL.GetTag(rowIdx, PPTAG_LOT_MANUFTIME);
								LDATETIME manuf_dtm = ZERODATETIME;
								if(!p_manuf_tm_tag || !p_manuf_tm_tag->GetTimestamp(&manuf_dtm))
									manuf_dtm.Set(rBp.Rec.Dt, ZEROTIME);
								rec.ManufDateTo = rec.ManufDateFrom = SUniTime(manuf_dtm).ToInt64();
								if(checkdate(r_ti.Expiry)) {
									rec.ExpiryTo = rec.ExpiryFrom = SUniTime(r_ti.Expiry).ToInt64();
								}
								assert(__volume > 0.0); // Инициализирован выше
								rec.Volume = __volume;
								{
									MEMSZERO(k1);
									k1.LinkBillID = rBp.Rec.ID;
									k1.LinkBillRow = r_ti.RByBill;
									if(PeC.DT.searchForUpdate(1, &k1, spEq)) {
										do {
											VetisDocumentTbl::Rec ex_rec;
											PeC.DT.CopyBufTo(&ex_rec);
											if(ex_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
												rec.EntityID = ex_rec.EntityID;
												THROW_DB(PeC.DT.updateRecBuf(&rec));
												rPbrBlk.ManufIncomeDocEntityID = rec.EntityID;
												break;
											}
											if(ex_rec.VetisDocStatus == vetisdocstWITHDRAWN) {
												; // go to the next record
											}
											else {
												if(P_Logger) {
													// Строка документа уже имеет сертификат %s
													/*
													SString status_text;
													PPLoadText(PPTXT_VETISBILLROWALLREADYHASCERT, fmt_buf);
													if(!SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), src_rec.VetisDocStatus, status_text)) {
														status_text.Z().Cat(src_rec.VetisDocStatus);
													}
													(temp_buf = bill_text).CatDiv(':', 2).Cat(src_rec.EntityID).Space().Cat(src_rec.IssueDate, DATF_DMY).
														Space().Cat(src_rec.IssueNumber).Space().Cat(status_text);
													msg_buf.Printf(fmt_buf, temp_buf.cptr());
													P_Logger->Log(msg_buf);
													*/
												}
												break;
											}
										} while(PeC.DT.searchForUpdate(1, &k1, spNext) && PeC.DT.data.LinkBillID == rBp.Rec.ID && PeC.DT.data.LinkBillRow == r_ti.RByBill);
									}
									else {
										VetisEntityCore::Entity entity;
										entity.SetupVetDocument();
										THROW(PeC.SetEntity(entity, 0, &rec.EntityID, 0));
										THROW_DB(PeC.DT.insertRecBuf(&rec));
										rPbrBlk.ManufIncomeDocEntityID = rec.EntityID;
									}
									ok = 1;
								}
							}
						}
					}
				}
				else {
					if(r_ti.Quantity_ < 0.0) {
						assert(!lot_uuid.IsZero()); // Инициализирован выше
						rec.VetisDocType = vetisdoctypPRODUCTIVE;
						rec.Flags |= VetisVetDocument::fManufExpense;
						THROW(rPbrBlk.ManufIncomeDocEntityID != 0);
						rec.DepDocEntityID = rPbrBlk.ManufIncomeDocEntityID;
						assert(src_rec.EntityID != 0); // Инициализирован выше
						assert(__volume > 0.0); // Инициализирован выше
						rec.Volume = __volume;
						{
							MEMSZERO(k1);
							k1.LinkBillID = rBp.Rec.ID;
							k1.LinkBillRow = r_ti.RByBill;
							if(PeC.DT.searchForUpdate(1, &k1, spEq)) {
								do {
									VetisDocumentTbl::Rec ex_rec;
									PeC.DT.CopyBufTo(&ex_rec);
									if(ex_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
										rec.EntityID = ex_rec.EntityID;
										THROW_DB(PeC.DT.updateRecBuf(&rec));
										break;
									}
									if(ex_rec.VetisDocStatus == vetisdocstWITHDRAWN) {
										; // go to the next record
									}
									else {
										if(P_Logger) {
											// Строка документа уже имеет сертификат %s
											/*
											SString status_text;
											PPLoadText(PPTXT_VETISBILLROWALLREADYHASCERT, fmt_buf);
											if(!SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), src_rec.VetisDocStatus, status_text)) {
												status_text.Z().Cat(src_rec.VetisDocStatus);
											}
											(temp_buf = bill_text).CatDiv(':', 2).Cat(src_rec.EntityID).Space().Cat(src_rec.IssueDate, DATF_DMY).
												Space().Cat(src_rec.IssueNumber).Space().Cat(status_text);
											msg_buf.Printf(fmt_buf, temp_buf.cptr());
											P_Logger->Log(msg_buf);
											*/
										}
										break;
									}
								} while(PeC.DT.searchForUpdate(1, &k1, spNext) && PeC.DT.data.LinkBillID == rBp.Rec.ID && PeC.DT.data.LinkBillRow == r_ti.RByBill);
							}
							else {
								VetisEntityCore::Entity entity;
								entity.SetupVetDocument();
								THROW(PeC.SetEntity(entity, 0, &rec.EntityID, 0));
								THROW_DB(PeC.DT.insertRecBuf(&rec));
							}
						}
					}
				}
			}
			else {
				assert(!lot_uuid.IsZero()); // Инициализирован выше
				if(rPbrBlk.BillID != rBp.Rec.ID) {
					rPbrBlk.BillID = rBp.Rec.ID;
					rPbrBlk.PersonGuid.Z();
					rPbrBlk.DlvrLocGuid.Z();
					if(IsIntrOp(rBp.Rec.OpID) == INTREXPND) {
						rPbrBlk.PersonID = P.MainOrgID;
						rPbrBlk.DlvrLocID = PPObjLocation::ObjToWarehouse(rBp.Rec.Object);
					}
					else {
						// @v12.2.1 {
						PPID   dlvr_loc_id = 0;
						const int gdar = p_bobj->GetDlvrAddrID(rBp.Rec, rBp.P_Freight, &dlvr_loc_id);
						if(gdar == 2) {
							rPbrBlk.PersonID = P.MainOrgID;
							rPbrBlk.DlvrLocID = dlvr_loc_id;
						}
						else {
							rPbrBlk.PersonID = ObjectToPerson(rBp.Rec.Object);
							rPbrBlk.DlvrLocID = dlvr_loc_id;
						}
						// } @v12.2.1 
					}
					THROW_PP_S(rPbrBlk.DlvrLocID && p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, rPbrBlk.DlvrLocID, PPTAG_LOC_VETIS_GUID, rPbrBlk.DlvrLocGuid) > 0, PPERR_VETISBILLHASNTDLVRLOCGUID, bill_text);
					THROW_PP_S(p_ref->Ot.GetTagGuid(PPOBJ_PERSON, rPbrBlk.PersonID, PPTAG_PERSON_VETISUUID, rPbrBlk.PersonGuid) > 0, PPERR_VETISBILLHASNTOBJGUID, bill_text);
				}
				if(!!rPbrBlk.PersonGuid && !!rPbrBlk.DlvrLocGuid) {
					int is_there_unresolved_entities = 0;
					VetisEntityCore::Entity entity_from_person;
					VetisEntityCore::Entity entity_from_loc;
					VetisEntityCore::Entity entity_to_person;
					VetisEntityCore::Entity entity_to_dlvrloc;
					assert(src_rec.EntityID != 0); // Инициализирован выше
					rec.LinkToPsnID = rPbrBlk.PersonID;
					rec.LinkToDlvrLocID = rPbrBlk.DlvrLocID;
					rec.OrgDocEntityID = entity_doc.ID;
					assert(__volume > 0.0); // Инициализирован выше
					rec.Volume = __volume;
					rec.Flags = VetisVetDocument::fFromMainOrg;
					if(src_rec.ToEntityID)
						rec.FromEntityID = src_rec.ToEntityID;
					else {
						S_GUID from_entity_guid;
						THROW_PP_S(p_ref->Ot.GetTagGuid(PPOBJ_PERSON, P.MainOrgID, PPTAG_PERSON_VETISUUID, from_entity_guid) > 0, PPERR_VETISBUSENTGUIDUNDEF, "");
						if(PeC.GetEntityByGuid(from_entity_guid, entity_from_person) > 0) {
							rec.FromEntityID = entity_from_person.ID;
						}
					}
					if(src_rec.ToEnterpriseID) {
						rec.FromEnterpriseID = src_rec.ToEnterpriseID;
					}
					else {
						S_GUID from_enterprise_guid;
						THROW_PP_S(rBp.Rec.LocID && p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, rBp.Rec.LocID, PPTAG_LOC_VETIS_GUID, from_enterprise_guid) > 0, PPERR_VETISLOCGUIDUNDEF, "");
						if(PeC.GetEntityByGuid(from_enterprise_guid, entity_from_loc) > 0) {
							rec.FromEnterpriseID = entity_from_loc.ID;
						}
					}
					if(PeC.GetEntityByGuid(rPbrBlk.PersonGuid, entity_to_person) > 0) {
						rec.ToEntityID = entity_to_person.ID;
					}
					else {
						long   guid_ref = 0;
						S_GUID guid;
						THROW(PeC.UrT.GetUuid(rPbrBlk.PersonGuid, &guid_ref, 0, 0));
						if(guid_ref && PeC.UrT.Search(guid_ref, guid) > 0 && !guid.IsZero()) {
							VetisApplicationBlock reply;
							int rr = GetEntityQuery2(qtBusinessEntityByGuid, VGuidToStr(guid, temp_buf), reply);
							if(rr > 0) {
								for(uint j = 0; j < reply.BEntList.getCount(); j++) {
									PPID   local_entity_id = 0;
									THROW(PeC.Put(&local_entity_id, *reply.BEntList.at(j), 0, 0));
								}
								if(PeC.GetEntityByGuid(rPbrBlk.PersonGuid, entity_to_person) > 0)
									rec.ToEntityID = entity_to_person.ID;
							}
						}
					}
					if(PeC.GetEntityByGuid(rPbrBlk.DlvrLocGuid, entity_to_dlvrloc) > 0) {
						rec.ToEnterpriseID = entity_to_dlvrloc.ID;
					}
					else {
						long   guid_ref = 0;
						S_GUID guid;
						THROW(PeC.UrT.GetUuid(rPbrBlk.DlvrLocGuid, &guid_ref, 0, 0));
						if(guid_ref && PeC.UrT.Search(guid_ref, guid) > 0 && !guid.IsZero()) {
							VetisApplicationBlock reply;
							int rr = GetEntityQuery2(qtEnterpriseByGuid, VGuidToStr(guid, temp_buf), reply);
							if(rr > 0) {
								for(uint j = 0; j < reply.EntItemList.getCount(); j++) {
									PPID   local_entity_id = 0;
									THROW(PeC.Put(&local_entity_id, *reply.EntItemList.at(j), 0, 0));
								}
								if(PeC.GetEntityByGuid(rPbrBlk.DlvrLocGuid, entity_to_dlvrloc) > 0)
									rec.ToEnterpriseID = entity_to_dlvrloc.ID;
							}
						}
					}
					{
						MEMSZERO(k1);
						k1.LinkBillID = rBp.Rec.ID;
						k1.LinkBillRow = r_ti.RByBill;
						if(PeC.DT.searchForUpdate(1, &k1, spEq)) {
							do {
								VetisDocumentTbl::Rec ex_rec;
								PeC.DT.CopyBufTo(&ex_rec);
								if(ex_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
									rec.EntityID = ex_rec.EntityID;
									THROW_DB(PeC.DT.updateRecBuf(&rec));
									break;
								}
								if(ex_rec.VetisDocStatus == vetisdocstWITHDRAWN) {
									; // go to the next record
								}
								else {
									if(P_Logger) {
										// Строка документа уже имеет сертификат %s
										SString status_text;
										PPLoadText(PPTXT_VETISBILLROWALLREADYHASCERT, fmt_buf);
										if(!SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), src_rec.VetisDocStatus, status_text)) {
											status_text.Z().Cat(src_rec.VetisDocStatus);
										}
										(temp_buf = bill_text).CatDiv(':', 2).Cat(src_rec.EntityID).Space().Cat(src_rec.IssueDate, DATF_DMY).
											Space().Cat(src_rec.IssueNumber).Space().CatEq("ID", ex_rec.EntityID).Space().Cat(status_text); // @v11.0.7 (CatEq("ID", ex_rec.EntityID).Space())
										msg_buf.Printf(fmt_buf, temp_buf.cptr());
										P_Logger->Log(msg_buf);
									}
									break;
								}
							} while(PeC.DT.searchForUpdate(1, &k1, spNext) && PeC.DT.data.LinkBillID == rBp.Rec.ID && PeC.DT.data.LinkBillRow == r_ti.RByBill);
						}
						else {
							VetisEntityCore::Entity entity;
							entity.SetupVetDocument();
							THROW(PeC.SetEntity(entity, 0, &rec.EntityID, 0));
							THROW_DB(PeC.DT.insertRecBuf(&rec));
						}
						ok = 1;
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPVetisInterface::MakeOutgoingBillList(PPID locID, const DateRange & rPeriod, const PPIDArray & rOpList, long flags, PPIDArray & rBillList)
{
	rBillList.clear();
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	for(uint i = 0; i < rOpList.getCount(); i++) {
		const  PPID op_id = rOpList.get(i);
		PPOprKind op_rec;
		BillTbl::Rec bill_rec;
		GetOpData(op_id, &op_rec);
		for(DateIter di(&rPeriod); p_bobj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
			if(!locID || bill_rec.LocID == locID) {
				int    suited = 1;
				if(!p_bobj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK))
					suited = 0;
				else if((flags & (pbrfDiscrepancy|pbrfManuf)) || bill_rec.Object) {
					rBillList.add(bill_rec.ID);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPVetisInterface::Helper_PutOutgoingBillList(PPIDArray & rBillList, const long putBillRowFlags)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	BillTbl::Rec bill_rec;
	{
		assert(rBillList.getCount());
		PPTransaction tra(1);
		THROW(tra);
		rBillList.sortAndUndup();
		for(uint k = 0; k < rBillList.getCount(); k++) {
			const  PPID bill_id = rBillList.get(k);
			if(p_bobj->Search(bill_id, &bill_rec) > 0) {
				PPBillPacket pack;
				if(p_bobj->ExtractPacket(bill_id, &pack) > 0) {
					PutBillRowBlock bbrblk;
					if(putBillRowFlags & pbrfManuf) {
						{
							// Сначала - выход
							for(uint tidx = 0; tidx < pack.GetTCount(); tidx++) {
								if(!PutBillRow(pack, tidx, putBillRowFlags|pbrfManufInc, bbrblk, 0)) {
									CALLPTRMEMB(P_Logger, LogLastError());
								}
								ok = 1;
							}
						}
						if(bbrblk.ManufIncomeDocEntityID) {
							// Теперь - расход
							for(uint tidx = 0; tidx < pack.GetTCount(); tidx++) {
								if(!PutBillRow(pack, tidx, putBillRowFlags, bbrblk, 0)) {
									CALLPTRMEMB(P_Logger, LogLastError());
								}
								ok = 1;
							}
						}
					}
					else {
						for(uint tidx = 0; tidx < pack.GetTCount(); tidx++) {
							if(!PutBillRow(pack, tidx, putBillRowFlags, bbrblk, 0)) {
								CALLPTRMEMB(P_Logger, LogLastError());
							}
							ok = 1;
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

int PPVetisInterface::SetupOutgoingEntries(PPID locID, const DateRange & rPeriod)
{
	int    ok = -1;
	bool   is_locked = false;
	PPIDArray bill_id_list;
	PPIDArray temp_bill_list; // Список идентификаторов документов продажи
	PPIDArray op_list;
	PrcssrAlcReport::Config alcr_cfg;
	PrcssrAlcReport::ReadConfig(&alcr_cfg);
	struct SetupOutgoingEntries_ControlBlock {
		long   Flags;
		PPID   SrcOpList[6];
	};
	const SetupOutgoingEntries_ControlBlock control_block_list[] = {
		{ pbrfDiscrepancy, { alcr_cfg.RcptEtcOpID, alcr_cfg.ExpndEtcOpID, 0, 0, 0, 0 } },
		{ pbrfManuf,       { alcr_cfg.E.ManufOpID, 0, 0, 0, 0, 0 } },
		{ 0,               { alcr_cfg.ExpndOpID, alcr_cfg.IntrExpndOpID, alcr_cfg.SupplRetOpID, 0, 0, 0 } } // @v12.3.4 SupplRetOpID
	};
	{
		THROW(PPVetisInterface::LockProcess(/*PPVETISICFUNC_SETUPOUTGOINGENTRIES*/PPVETISICFUNC_PROCESSOUTCOMING, false)); // @v11.3.2
		is_locked = true;
		for(uint cbidx = 0; cbidx < SIZEOFARRAY(control_block_list); cbidx++) {
			PPIDArray base_op_list;
			for(uint opidx = 0; opidx < SIZEOFARRAY(control_block_list[cbidx].SrcOpList); opidx++) {
				const  PPID op_id = control_block_list[cbidx].SrcOpList[opidx];
				base_op_list.addnz(op_id);
			}
			if(PPObjOprKind::ExpandOpList(base_op_list, op_list) > 0) {
				const long put_bill_row_flags = control_block_list[cbidx].Flags;
				if(MakeOutgoingBillList(locID, rPeriod, op_list, put_bill_row_flags, temp_bill_list) > 0) {
					const int local_ok = Helper_PutOutgoingBillList(temp_bill_list, put_bill_row_flags);
					THROW(local_ok);
					if(local_ok > 0)
						ok = 1;
				}
			}
		}
	}
	CATCHZOK
	// @v11.3.2 {
	if(is_locked)
		PPVetisInterface::LockProcess(/*PPVETISICFUNC_SETUPOUTGOINGENTRIES*/PPVETISICFUNC_PROCESSOUTCOMING, true);
	// } @v11.3.2
	return ok;
}

int VetisEntityCore::SetupEnterpriseEntry(PPID psnID, PPID locID, VetisEnterprise & rEntry)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	S_GUID issuer_uuid;
	S_GUID loc_uuid;
	rEntry.Z();
	if(psnID) {
		const LDATE now_date = getcurdate_();
		SString org_inn;
		SString org_kpp;
		SString loc_kpp;
		SString temp_buf;
		ObjTagItem tag_item;
		PPLocationPacket loc_pack;
		if(PsnObj.GetRegNumber(psnID, PPREGT_TPID, now_date, temp_buf) > 0 && temp_buf.NotEmpty())
			org_inn = temp_buf;
		if(PsnObj.GetRegNumber(psnID, PPREGT_KPP, now_date, temp_buf) > 0 && temp_buf.NotEmpty())
			org_kpp = temp_buf;
		if(PsnObj.LocObj.GetPacket(locID, &loc_pack) > 0) {
			if(loc_pack.Regs.GetRegNumber(PPREGT_KPP, now_date, temp_buf) > 0 && temp_buf.NotEmpty())
				loc_kpp = temp_buf;
			if(p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, locID, PPTAG_LOC_VETIS_GUID, loc_uuid) > 0) {
				VetisEntityCore::Entity sub_entity;
				if(GetEntityByGuid(loc_uuid, sub_entity) > 0 && sub_entity.Kind == VetisEntityCore::kEnterprise) {
					Get(sub_entity.ID, rEntry);
					rEntry.NativeLocID = locID;
				}
			}
			rEntry.Name.SetIfEmpty(loc_pack.Name);
			if(rEntry.Address.AddressView.IsEmpty())
				LocationCore::GetAddress(loc_pack, 0, rEntry.Address.AddressView);
		}
		if(p_ref->Ot.GetTagGuid(PPOBJ_PERSON, psnID, PPTAG_PERSON_VETISUUID, issuer_uuid) > 0) {
			VetisEntityCore::Entity sub_entity;
			if(GetEntityByGuid(issuer_uuid, sub_entity) > 0 && sub_entity.Kind == VetisEntityCore::kBusinessEntity) {
				rEntry.P_Owner = new VetisBusinessEntity;
				Get(sub_entity.ID, *rEntry.P_Owner);
				rEntry.P_Owner->NativePsnID = psnID;
			}
		}
		if(!rEntry.P_Owner) {
			rEntry.P_Owner = new VetisBusinessEntity;
			rEntry.P_Owner->NativePsnID = psnID;
		}
		if(rEntry.P_Owner) {
			if(!issuer_uuid.IsZero())
				rEntry.P_Owner->Guid = issuer_uuid;
			rEntry.P_Owner->Inn = org_inn;
			rEntry.P_Owner->Kpp = org_kpp;
		}
		if(loc_kpp.NotEmpty() || org_kpp.NotEmpty() || org_inn.NotEmpty()) {
			VetisEnterpriseOfficialRegistration * p_ofr = rEntry.OfficialRegistration.CreateNewItem();
			if(p_ofr) {
				p_ofr->Kpp = loc_kpp.NotEmpty() ? loc_kpp : org_kpp;
				if(org_inn.NotEmpty()) {
					p_ofr->P_BusinessEntity = new VetisBusinessEntity;
					p_ofr->P_BusinessEntity->Inn = org_inn;
					p_ofr->P_BusinessEntity->Kpp = org_kpp;
				}
			}
		}
		ok = 1;
	}
	return ok;
}

int EditVetisProductItem(VetisEntityCore & rEc, VetisProductItem & rData)
{
	class VetisPiDialog : public TDialog {
		DECL_DIALOG_DATA(VetisProductItem);
	public:
		enum {
			ctlgroupGoods = 1
		};
		VetisPiDialog(VetisEntityCore & rEc) : TDialog(DLG_VETISPI), R_Ec(rEc), ProdIdent(0), SubProdIdent(0), MainOrgID(0), WarehouseID(0)
		{
			addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_VETISPI_GOODSGRP, CTLSEL_VETISPI_GOODS));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			MainOrgID = GetMainOrgID();
			WarehouseID = LConfig.Location;
			SetupPersonCombo(this, CTLSEL_VETISPI_ORG, MainOrgID, 0, PPPRK_MAIN, 1);
			SetupLocationCombo(this, CTLSEL_VETISPI_LOC, WarehouseID, 0, 0);
			{
				GoodsCtrlGroup::Rec ggrp_rec(0, Data.NativeGoodsID, 0, GoodsCtrlGroup::enableInsertGoods);
				setGroupData(ctlgroupGoods, &ggrp_rec);
			}
			R_Ec.MakeProductList(ProdList, GuidAsscList);
			R_Ec.MakeSubProductList(SubProdList, GuidAsscList, &SprToPrList);
			ProdList.SortByText();
			SubProdList.SortByText();
			SetupStrAssocCombo(this, CTLSEL_VETISPI_PROD, ProdList, ProdIdent, 0, 0, 0);
			SetupStrAssocCombo(this, CTLSEL_VETISPI_SUBPROD, SubProdList, SubProdIdent, 0, 0, 0);
			Setup();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			Reference * p_ref = PPRef;
			PPObjGoods goods_obj;
			Goods2Tbl::Rec goods_rec;
			SString temp_buf;
			S_GUID guid;
			Data.Z();
			{
				GoodsCtrlGroup::Rec ggrp_rec;
				getGroupData(ctlgroupGoods, &ggrp_rec);
				Data.NativeGoodsID = ggrp_rec.GoodsID;
				if(goods_obj.Fetch(Data.NativeGoodsID, &goods_rec) > 0) {
					Data.Name = goods_rec.Name;
				}
			}
			{
				S_GUID prod_guid;
				S_GUID subprod_guid;
				ProdIdent = getCtrlLong(CTLSEL_VETISPI_PROD);
				SubProdIdent = getCtrlLong(CTLSEL_VETISPI_SUBPROD);
				if(ProdIdent)
					GuidAsscList.Search(ProdIdent, &prod_guid, 0);
				if(SubProdIdent)
					GuidAsscList.Search(SubProdIdent, &subprod_guid, 0);
				Data.Product.Guid = prod_guid;
				Data.SubProduct.Guid = subprod_guid;
			}
			{
				MainOrgID = getCtrlLong(CTLSEL_VETISPI_ORG);
				WarehouseID = getCtrlLong(CTLSEL_VETISPI_LOC);
				if(MainOrgID && p_ref->Ot.GetTagGuid(PPOBJ_PERSON, MainOrgID, PPTAG_PERSON_VETISUUID, guid) > 0)
					Data.Producer.Guid = guid;
				setCtrlString(CTL_VETISPI_BENTY, temp_buf);
				temp_buf.Z();
				if(WarehouseID && p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, WarehouseID, PPTAG_LOC_VETIS_GUID, guid) > 0) {
					VetisEnterprise * p_new_item = Data.Producing.CreateNewItem();
					if(p_new_item)
						p_new_item->Guid = guid;
				}
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_VETISPI_ORG))
				Setup();
			else if(event.isCbSelected(CTLSEL_VETISPI_LOC))
				Setup();
			else if(event.isCbSelected(CTLSEL_VETISPI_GOODS))
				Setup();
			else if(event.isCbSelected(CTLSEL_VETISPI_SUBPROD))
				Setup();
			else if(event.isCmd(cmCreateVetisProductItem)) {
				VetisProductItem pi;
				if(getDTS(&pi) && pi.NativeGoodsID) {
					PPLogger logger;
					SString temp_buf;
					PPVetisInterface ifc(&logger);
					VetisApplicationBlock reply;
					PPVetisInterface::Param param(MainOrgID, WarehouseID, PPVetisInterface::Param::fSkipLocInitialization);
					if(PPVetisInterface::SetupParam(param) && ifc.Init(param)) {
						if(!pi.SubProduct.Guid.IsZero()) {
							int rr = ifc.GetEntityQuery2(ifc.qtSubProductByGuid, VGuidToStr(pi.SubProduct.Guid, temp_buf), reply);
							if(reply.SubProductList.getCount() == 1) {
								pi.SubProduct = *reply.SubProductList.at(0);
								if(!pi.SubProduct.ProductGuid.IsZero()) {
									if(ifc.GetEntityQuery2(ifc.qtProductByGuid, VGuidToStr(pi.SubProduct.ProductGuid, temp_buf), reply) > 0) {
										if(reply.ProductList.getCount() == 1) {
											pi.Product = *reply.ProductList.at(0);
											pi.ProductType = pi.Product.ProductType;
										}
									}
								}
							}
						}
						//pi.ProductType = 7; // @todo Значение должно быть каким-то образом ассоциировано с товаров
						if(ifc.ModifyProducerStockListOperation(vetisrmtCREATE, pi, reply) > 0) {
							if(reply.ProductItemList.getCount() == 1) {
								const VetisProductItem * p_pi = reply.ProductItemList.at(0);
								if(p_pi && !p_pi->Guid.IsZero()) {
									ObjTagItem tag_item;
									if(tag_item.SetGuid(PPTAG_GOODS_VETISGUID, &p_pi->Guid)) {
										if(!PPRef->Ot.PutTag(PPOBJ_GOODS, pi.NativeGoodsID, &tag_item, 1)) {
										}
										else {
											Setup();
										}
									}
								}
							}
						}
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		void   Setup()
		{
			Reference * p_ref = PPRef;
			SString temp_buf;
			S_GUID guid;
			const  PPID ex_loc_id = getCtrlLong(CTLSEL_VETISENT_LOC);
			temp_buf.Z();
			MainOrgID = getCtrlLong(CTLSEL_VETISPI_ORG);
			WarehouseID = getCtrlLong(CTLSEL_VETISPI_LOC);
			SubProdIdent = getCtrlLong(CTLSEL_VETISPI_SUBPROD);
			if(MainOrgID && p_ref->Ot.GetTagGuid(PPOBJ_PERSON, MainOrgID, PPTAG_PERSON_VETISUUID, guid) > 0)
				guid.ToStr(S_GUID::fmtIDL, temp_buf);
			setCtrlString(CTL_VETISPI_BENTY, temp_buf);
			temp_buf.Z();
			if(WarehouseID && p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, WarehouseID, PPTAG_LOC_VETIS_GUID, guid) > 0)
				guid.ToStr(S_GUID::fmtIDL, temp_buf);
			setCtrlString(CTL_VETISPI_ENT, temp_buf);
			temp_buf.Z();
			{
				GoodsCtrlGroup::Rec ggrp_rec;
				getGroupData(ctlgroupGoods, &ggrp_rec);
				if(ggrp_rec.GoodsID && p_ref->Ot.GetTagGuid(PPOBJ_GOODS, ggrp_rec.GoodsID, PPTAG_GOODS_VETISGUID, guid) > 0)
					guid.ToStr(S_GUID::fmtIDL, temp_buf);
			}
			setCtrlString(CTL_VETISPI_PI, temp_buf);
			if(SubProdIdent) {
				long   pr_id = 0;
				if(SprToPrList.Search(SubProdIdent, &pr_id, 0) && pr_id) {
					if(ProdList.Search(pr_id, 0)) {
						setCtrlLong(CTLSEL_VETISPI_PROD, pr_id);
					}
				}
			}
			enableCommand(cmCreateVetisProductItem, 1/*!Data.NativeLocID && ex_loc_id && person_id*/);
		}
		VetisEntityCore & R_Ec;
		UUIDAssocArray GuidAsscList;
		StrAssocArray ProdList;
		StrAssocArray SubProdList;
		LAssocArray SprToPrList;
		long   ProdIdent;
		long   SubProdIdent;
		PPID   MainOrgID;
		PPID   WarehouseID;
	};
	DIALOG_PROC_BODY_P1(VetisPiDialog, rEc, &rData);
}

int EditVetisEnterprise(VetisEntityCore & rEc, VetisEnterprise & rData)
{
	class VetEntDialog : public TDialog {
		DECL_DIALOG_DATA(VetisEnterprise);
	public:
		VetEntDialog(VetisEntityCore & rEc) : TDialog(DLG_VETISENT), R_Ec(rEc), CountryIdent(0), RegionIdent(0), LocalityIdent(0)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			CountryIdent = 0;
			RegionIdent = 0;
			LocalityIdent = 0;
			RVALUEPTR(Data, pData);
			R_Ec.MakeCountryList(CountryList, GuidAsscList);
			{
				S_GUID g;
				if(g.FromStr(P_VetisGuid_Country_Ru) && GuidAsscList.SearchVal(g, &CountryIdent, 0)) {
					if(!CountryList.Search(CountryIdent, 0))
						CountryIdent = 0;
				}
			}
			SetupPersonCombo(this, CTLSEL_VETISENT_ORG, (Data.P_Owner ? Data.P_Owner->NativePsnID : 0), 0, PPPRK_MAIN, 1);
			SetupLocationCombo(this, CTLSEL_VETISENT_LOC, Data.NativeLocID, 0, 0);
			SetupStrAssocCombo(this, CTLSEL_VETISENT_COUNTRY, CountryList, CountryIdent, 0, 0, 0);
			if(CountryIdent) {
				R_Ec.MakeRegionList(CountryIdent, RegionList, GuidAsscList);
				SetupStrAssocCombo(this, CTLSEL_VETISENT_REGION, RegionList, RegionIdent, 0, 0, 0);
			}
			Setup();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;

			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_VETISENT_LOC) || event.isCbSelected(CTLSEL_VETISENT_ORG)) {
				UI_LOCAL_LOCK_ENTER
					const  PPID   psn_id = getCtrlLong(CTLSEL_VETISENT_ORG);
					const  PPID   loc_id = getCtrlLong(CTLSEL_VETISENT_LOC);
					R_Ec.SetupEnterpriseEntry(psn_id, loc_id, Data);
					Setup();
				UI_LOCAL_LOCK_LEAVE
				clearEvent(event);
			}
			else if(event.isCbSelected(CTLSEL_VETISENT_COUNTRY)) {
			}
			else if(event.isCbSelected(CTLSEL_VETISENT_REGION)) {
				const long preserve_region_ident = RegionIdent;
				RegionIdent = getCtrlLong(CTLSEL_VETISENT_REGION);
				if(RegionIdent != preserve_region_ident) {
					if(RegionIdent)
						R_Ec.MakeLocalityList(RegionIdent, CityList, GuidAsscList);
					else
						CityList.Z();
					SetupStrAssocCombo(this, CTLSEL_VETISENT_LOCALITY, CityList, LocalityIdent, 0, 0, 0);
				}
			}
			else if(event.isCmd(cmCreateVetisEnterprise)) {
				{
					PPLogger logger;
					PPVetisInterface ifc(&logger);
					VetisApplicationBlock reply;
					PPID   psn_id = getCtrlLong(CTLSEL_VETISENT_ORG);
					PPID   loc_id = 0;
					PPVetisInterface::Param param(psn_id, loc_id, PPVetisInterface::Param::fSkipLocInitialization);
					if(PPVetisInterface::SetupParam(param) && ifc.Init(param)) {
						//ifc.ProcessIncomingConsignment(item.Uuid, reply);
						VetisEnterprise entry;
						loc_id = getCtrlLong(CTLSEL_VETISENT_LOC);
						if(loc_id) {
							S_GUID country_guid;
							S_GUID region_guid;
							S_GUID locality_guid;
							R_Ec.SetupEnterpriseEntry(psn_id, loc_id, entry);
							CountryIdent = getCtrlLong(CTLSEL_VETISENT_COUNTRY);
							RegionIdent = getCtrlLong(CTLSEL_VETISENT_REGION);
							LocalityIdent = getCtrlLong(CTLSEL_VETISENT_LOCALITY);
							if(CountryIdent)
								GuidAsscList.Search(CountryIdent, &country_guid, 0);
							if(RegionIdent)
								GuidAsscList.Search(RegionIdent, &region_guid, 0);
							if(LocalityIdent)
								GuidAsscList.Search(LocalityIdent, &locality_guid, 0);
							entry.Address.Country.Guid = country_guid;
							entry.Address.Region.Guid = region_guid;
							entry.Address.Locality.Guid = locality_guid;
							if(ifc.ModifyEnterprise(vetisrmtCREATE, entry, reply) > 0) {
								if(reply.ApplicationStatus == reply.appstCompleted && reply.EntItemList.getCount()) {
									VetisEnterprise result_ent;
									result_ent = *reply.EntItemList.at(0);
									if(!result_ent.Guid.IsZero()) {
										ObjTagItem tag_item;
										if(tag_item.SetGuid(PPTAG_LOC_VETIS_GUID, &result_ent.Guid)) {
											if(!PPRef->Ot.PutTag(PPOBJ_LOCATION, loc_id, &tag_item, 1))
												logger.LogLastError();
											else {
												PPID   entity_id = 0;
												if(!R_Ec.Put(&entity_id, result_ent, 0, 1)) {
													logger.LogLastError();
												}
												else {
													SString temp_buf;
													{
														assert(entry.P_Owner);
														//int PPVetisInterface::ModifyActivityLocations(VetisRegisterModificationType modType, const VetisBusinessEntity & rBe, const VetisEnterprise & rEnt, VetisApplicationBlock & rReply)
														if(ifc.ModifyActivityLocations(vetisrmtCREATE, *entry.P_Owner, result_ent, reply) > 0) {
														}
													}
													result_ent.Guid.ToStr(S_GUID::fmtIDL, temp_buf);
													setCtrlString(CTL_VETISENT_ENT, temp_buf);
													enableCommand(cmCreateVetisEnterprise, 0);
												}
											}
										}
									}
								}
							}
							logger.Save(PPFILNAM_VETISINFO_LOG, 0);
						}
					}
					else
						PPError();
				}
				clearEvent(event);
			}
			else
				return;
		}
		void   Setup()
		{
			Reference * p_ref = PPRef;
			SString temp_buf;
			S_GUID guid;
			const  PPID ex_loc_id = getCtrlLong(CTLSEL_VETISENT_LOC);
			PPID   person_id = Data.P_Owner ? Data.P_Owner->NativePsnID : 0;
			//SetupPersonCombo(this, CTLSEL_VETISENT_ORG, person_id, 0, PPPRK_MAIN, 1);
			setCtrlLong(CTLSEL_VETISENT_ORG, person_id);
			temp_buf.Z();
			if(person_id && p_ref->Ot.GetTagGuid(PPOBJ_PERSON, person_id, PPTAG_PERSON_VETISUUID, guid) > 0)
				guid.ToStr(S_GUID::fmtIDL, temp_buf);
			setCtrlString(CTL_VETISENT_BENTY, temp_buf);
			temp_buf.Z();
			if(Data.NativeLocID) {
				setCtrlLong(CTLSEL_VETISENT_LOC, Data.NativeLocID);
				if(p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, Data.NativeLocID, PPTAG_LOC_VETIS_GUID, guid) > 0)
					guid.ToStr(S_GUID::fmtIDL, temp_buf);
			}
			setCtrlString(CTL_VETISENT_ENT, temp_buf);
			enableCommand(cmCreateVetisEnterprise, !Data.NativeLocID && ex_loc_id && person_id);
		}
		VetisEntityCore & R_Ec;
		UUIDAssocArray GuidAsscList;
		StrAssocArray CountryList;
		StrAssocArray RegionList;
		StrAssocArray CityList;
		long   CountryIdent;
		long   RegionIdent;
		long   LocalityIdent;
	};
	DIALOG_PROC_BODY_P1(VetEntDialog, rEc, &rData);
}

long VetisEntityCore::Helper_InitMaxGuidKey(const UUIDAssocArray & rGuidList) const
{
	long max_guid_key = 0;
	if(rGuidList.GetMaxKey(&max_guid_key, 0) < 0)
		max_guid_key = 0;
	return max_guid_key;
}

long VetisEntityCore::Helper_SetGuidToList(const S_GUID & rGuid, long * pMaxGuidKey, UUIDAssocArray & rGuidList) const
{
	uint   gpos = 0;
	long   gkey = 0;
	long   max_guid_key = DEREFPTRORZ(pMaxGuidKey);
	if(!rGuidList.SearchVal(rGuid, &gkey, &gpos)) {
		gkey = ++max_guid_key;
		rGuidList.Add(gkey, rGuid, &gpos);
	}
	ASSIGN_PTR(pMaxGuidKey, max_guid_key);
	return gkey;
}

int VetisEntityCore::MakeProductList(StrAssocArray & rList, UUIDAssocArray & rGuidList)
{
	rList.Z();
	int    ok = -1;
	SString temp_buf;
	PPVetisInterface::Param param(0, 0, PPVetisInterface::Param::fSkipLocInitialization);
	if(PPVetisInterface::SetupParam(param)) {
		PPVetisInterface ifc(0/*&logger*/);
		VetisApplicationBlock reply;
		if(ifc.Init(param) && ifc.GetProductChangesList(0, 1000, ZERODATE, reply) > 0) {
			long max_guid_key = Helper_InitMaxGuidKey(rGuidList);
			for(uint i = 0; i < reply.ProductList.getCount(); i++) {
				const VetisProduct * p_item = reply.ProductList.at(i);
				if(p_item && p_item->Flags & p_item->fActive) {
					rList.AddFast(Helper_SetGuidToList(p_item->Guid, &max_guid_key, rGuidList), p_item->Name);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int VetisEntityCore::MakeSubProductList(StrAssocArray & rList, UUIDAssocArray & rGuidList, LAssocArray * pParentProductList)
{
	rList.Z();
	CALLPTRMEMB(pParentProductList, clear());
	int    ok = -1;
	SString temp_buf;
	PPVetisInterface::Param param(0, 0, PPVetisInterface::Param::fSkipLocInitialization);
	if(PPVetisInterface::SetupParam(param)) {
		PPVetisInterface ifc(0/*&logger*/);
		if(ifc.Init(param)) {
			VetisApplicationBlock reply;
			uint req_count = 1000;
			long max_guid_key = Helper_InitMaxGuidKey(rGuidList);
			for(uint req_offs = 0; ifc.GetSubProductChangesList(req_offs, req_count, ZERODATE, reply);) {
				for(uint i = 0; i < reply.SubProductList.getCount(); i++) {
					const VetisSubProduct * p_item = reply.SubProductList.at(i);
					if(p_item && p_item->Flags & p_item->fActive) {
						long   sp_id = Helper_SetGuidToList(p_item->Guid, &max_guid_key, rGuidList);
						rList.AddFast(sp_id, p_item->Name);
						if(pParentProductList && !p_item->ProductGuid.IsZero()) {
							uint   gpos = 0;
							long   gkey = 0;
							if(rGuidList.SearchVal(p_item->ProductGuid, &gkey, &gpos)) {
								pParentProductList->AddUnique(sp_id, gkey, 0, 0);
							}
						}
						ok = 1;
					}
				}
				if(reply.SubProductList.getCount() < req_count)
					break;
				else
					req_offs += reply.SubProductList.getCount();
			}
		}
	}
	return ok;
}

int VetisEntityCore::MakeCountryList(StrAssocArray & rList, UUIDAssocArray & rGuidList)
{
	rList.Z();
	int    ok = -1;
	SString temp_buf;
	PPVetisInterface::Param param(0, 0, PPVetisInterface::Param::fSkipLocInitialization);
	if(PPVetisInterface::SetupParam(param)) {
		PPVetisInterface ifc(0/*&logger*/);
		VetisApplicationBlock reply;
		if(ifc.Init(param) && ifc.GetCountryList(reply) > 0) {
			long max_guid_key = Helper_InitMaxGuidKey(rGuidList);
			for(uint i = 0; i < reply.CountryList.getCount(); i++) {
				const VetisCountry * p_item = reply.CountryList.at(i);
				if(p_item)
					rList.AddFast(Helper_SetGuidToList(p_item->Guid, &max_guid_key, rGuidList), p_item->Name);
			}
		}
	}
	return ok;
}

int VetisEntityCore::MakeRegionList(long countryIdent, StrAssocArray & rList, UUIDAssocArray & rGuidList)
{
	rList.Z();
	int    ok = -1;
	S_GUID _guid;
	if(countryIdent && rGuidList.Search(countryIdent, &_guid, 0)) {
		PPVetisInterface::Param param(0, 0, PPVetisInterface::Param::fSkipLocInitialization);
		if(PPVetisInterface::SetupParam(param)) {
			PPVetisInterface ifc(0/*&logger*/);
			VetisApplicationBlock reply;
			if(ifc.Init(param) && ifc.GetRegionList(_guid, reply) > 0) {
				long max_guid_key = Helper_InitMaxGuidKey(rGuidList);
				for(uint i = 0; i < reply.RegionList.getCount(); i++) {
					const VetisAddressObjectView * p_item = reply.RegionList.at(i);
					if(p_item)
						rList.AddFast(Helper_SetGuidToList(p_item->Guid, &max_guid_key, rGuidList), p_item->View);
				}
			}
		}
	}
	return ok;
}

int VetisEntityCore::MakeLocalityList(long regionIdent, StrAssocArray & rList, UUIDAssocArray & rGuidList)
{
	rList.Z();
	int    ok = -1;
	S_GUID _guid;
	if(regionIdent && rGuidList.Search(regionIdent, &_guid, 0)) {
		PPVetisInterface::Param param(0, 0, PPVetisInterface::Param::fSkipLocInitialization);
		if(PPVetisInterface::SetupParam(param)) {
			PPVetisInterface ifc(0/*&logger*/);
			VetisApplicationBlock reply;
			if(ifc.Init(param) && ifc.GetLocalityList(_guid, reply) > 0) {
				long max_guid_key = Helper_InitMaxGuidKey(rGuidList);
				for(uint i = 0; i < reply.RegionList.getCount(); i++) {
					const VetisAddressObjectView * p_item = reply.RegionList.at(i);
					if(p_item)
						rList.AddFast(Helper_SetGuidToList(p_item->Guid, &max_guid_key, rGuidList), p_item->View);
				}
			}
		}
	}
	return ok;
}

int PPViewVetisDocument::ViewWarehouse()
{
	int    ok = -1;
	VetisEnterprise ve;
	PPID   loc_id = LConfig.Location;
	PPID   main_org_id = GetMainOrgID();
	EC.SetupEnterpriseEntry(main_org_id, loc_id, ve);
	EditVetisEnterprise(EC, ve);
	//DLG_VETISENT
	return ok;
}

int PPViewVetisDocument::ViewGoods()
{
	int    ok = -1;
	VetisProductItem vpi;
	PPID   loc_id = LConfig.Location;
	PPID   main_org_id = GetMainOrgID();
	//EC.SetupEnterpriseEntry(main_org_id, loc_id, ve);
	EditVetisProductItem(EC, vpi);
	//DLG_VETISENT
	return ok;
}

static int EditVetisVetDocument(VetisVetDocument & rData, PPID mainOrgID, PPID locID)
{
	class VetVDocInfoDialog : public TDialog {
	public:
		explicit VetVDocInfoDialog(VetisVetDocument & rData, PPID mainOrgID, PPID locID) : TDialog(DLG_VETVDOC), R_Data(rData), MainOrgID(mainOrgID), LocID(locID)
		{
		}
		void SetupNativeBill()
		{
			SString text_buf;
			SString temp_buf;
			setCtrlString(CTL_VETVDOC_LINKBILL, text_buf.Z());
			if(R_Data.NativeBillID) {
				BillTbl::Rec bill_rec;
				text_buf.Z();
				if(BillObj->Search(R_Data.NativeBillID, &bill_rec) > 0) {
					PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
					text_buf.CatChar('#').Cat(bill_rec.ID).Space().Cat(temp_buf).Space().CatEq("#row", R_Data.NativeBillRow);
					setCtrlString(CTL_VETVDOC_LINKBILL, text_buf);
					// @v11.1.8 {
					if(!(R_Data.Flags & VetisVetDocument::fFromMainOrg)) {
						enableCommand(cmUnmatch, 1); 
					}
					// } @v11.1.8 
				}
				else {
					text_buf.CatChar('#').Cat(bill_rec.ID).Space().Cat("not found").Space().CatEq("#row", R_Data.NativeBillRow);
					enableCommand(cmLinkedBill, 0);
					enableCommand(cmUnmatch, 0); // @v11.1.8
				}
			
			}
			else {
				enableCommand(cmLinkedBill, 0);
				enableCommand(cmUnmatch, 0); // @v11.1.8
			}
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmLinkedBill)) {
				PPObjBill * p_bobj = BillObj;
				PPID   bill_id = R_Data.NativeBillID;
				BillTbl::Rec bill_rec;
				if(bill_id && p_bobj->Fetch(bill_id, &bill_rec) > 0) {
					PPObjBill::EditParam ep;
					ep.Flags |= PPObjBill::efCascade;
					p_bobj->Edit(&bill_id, &ep);
				}
				clearEvent(event);
			}
			else if(event.isCmd(cmUnmatch)) { // @v11.1.8
				if(R_Data.NativeBillID && !(R_Data.Flags & VetisVetDocument::fFromMainOrg)) {
					R_Data.NativeBillID = 0;					
					SetupNativeBill();
				}
			}
			else if(event.isKeyDown(kbF10)) { // @debug
				if(R_Data.VetDType == vetisdoctypSTOCK) {
					PPVetisInterface::Param param(MainOrgID, LocID, 0);
					if(PPVetisInterface::SetupParam(param)) {
						PPVetisInterface ifc(0/*&logger*/);
						VetisApplicationBlock reply;
						if(ifc.Init(param) && ifc.GetStockEntryByUuid(R_Data.Uuid, reply) > 0) {
							;
						}
					}
				}
			}
		}
		VetisVetDocument & R_Data;
		const  PPID MainOrgID;
		const  PPID LocID;
	};
	int    ok = -1;
	VetVDocInfoDialog * dlg = new VetVDocInfoDialog(rData, mainOrgID, locID);
	if(CheckDialogPtrErr(&dlg)) {
		VetisCertifiedConsignment & r_crtc = rData.CertifiedConsignment;
		VetisBatch & r_bat = r_crtc.Batch;
		SString temp_buf;
		SString text_buf;
		//dlg->setCtrlLong(rData.)
		dlg->setCtrlString(CTL_VETVDOC_UUID, temp_buf.Z().Cat(rData.Uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
		dlg->setCtrlDate(CTL_VETVDOC_DT, rData.IssueDate);
		dlg->setCtrlString(CTL_VETVDOC_NUM, rData.IssueNumber);
		dlg->setCtrlDate(CTL_VETVDOC_WBDT, rData.WayBillDate);
		dlg->setCtrlString(CTL_VETVDOC_WBNUM, rData.WayBillNumber);
		SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), rData.VetDStatus, temp_buf);
		dlg->setCtrlString(CTL_VETVDOC_VDSTATUS, temp_buf);
		SIntToSymbTab_GetSymb(VetisVetDocFormat_SymbTab, SIZEOFARRAY(VetisVetDocFormat_SymbTab), rData.VetDForm, temp_buf);
		dlg->setCtrlString(CTL_VETVDOC_VDFORM, temp_buf);
		SIntToSymbTab_GetSymb(VetisVetDocType_SymbTab, SIZEOFARRAY(VetisVetDocType_SymbTab), rData.VetDType, temp_buf);
		dlg->setCtrlString(CTL_VETVDOC_VDTYPE, temp_buf);
		dlg->setCtrlReal(CTL_VETVDOC_QTTY, r_bat.Volume);
		{
			text_buf.Z();
			if(PPVetisInterface::GoodsDateToString(r_bat.ExpiryDate.FirstDate, temp_buf) > 0)
				text_buf.Cat(temp_buf);
			if(PPVetisInterface::GoodsDateToString(r_bat.ExpiryDate.SecondDate, temp_buf) > 0)
				text_buf.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			if(r_bat.ExpiryDate.InformalDate.NotEmptyS())
				text_buf.CatDivIfNotEmpty(' ', 0).CatParStr(r_bat.ExpiryDate.InformalDate);
			dlg->setCtrlString(CTL_VETVDOC_EXPIRY, text_buf);
		}
		if(rData.Flags & rData.fFromMainOrg) {
			dlg->showCtrl(CTL_VETVDOC_ACKQTTY, false);
			dlg->showCtrl(CTL_VETVDOC_ACK, false);
		}
		else {
			dlg->setCtrlReal(CTL_VETVDOC_ACKQTTY, r_bat.AckVolume);
			long   ack = CHKXORFLAGS(rData.Flags, VetisVetDocument::fPartlyAccepted, VetisVetDocument::fRejected);
			dlg->AddClusterAssocDef(CTL_VETVDOC_ACK, 0, 0);
			dlg->AddClusterAssoc(CTL_VETVDOC_ACK, 1, VetisVetDocument::fPartlyAccepted);
			dlg->AddClusterAssoc(CTL_VETVDOC_ACK, 2, VetisVetDocument::fRejected);
			dlg->SetClusterData(CTL_VETVDOC_ACK, ack);
		}
		text_buf.Z();
		if(!!r_crtc.Consignor.Enterprise.Guid) {
			temp_buf.Z().Cat(r_crtc.Consignor.Enterprise.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		if(!!r_crtc.Consignor.BusinessEntity.Guid) {
			temp_buf.Z().Cat(r_crtc.Consignor.BusinessEntity.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		dlg->setCtrlString(CTL_VETVDOC_FROMSI, text_buf);
		if(r_crtc.Consignor.Enterprise.Name.NotEmpty())
			dlg->setCtrlString(CTL_VETVDOC_FROMNAM, r_crtc.Consignor.Enterprise.Name);
		else
			dlg->setCtrlString(CTL_VETVDOC_FROMNAM, r_crtc.Consignor.BusinessEntity.Name);

		text_buf.Z();
		if(!!r_crtc.Consignee.Enterprise.Guid) {
			temp_buf.Z().Cat(r_crtc.Consignee.Enterprise.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		if(!!r_crtc.Consignee.BusinessEntity.Guid) {
			temp_buf.Z().Cat(r_crtc.Consignee.BusinessEntity.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		dlg->setCtrlString(CTL_VETVDOC_TOSI, text_buf);
		text_buf.Z();
		if(r_crtc.Consignee.Enterprise.Name.NotEmpty())
			text_buf.CatDivIfNotEmpty(';', 2).Cat(r_crtc.Consignee.Enterprise.Name);
		if(r_crtc.Consignee.BusinessEntity.Name.NotEmpty())
			text_buf.CatDivIfNotEmpty(';', 2).Cat(r_crtc.Consignee.Enterprise.Name);
		dlg->setCtrlString(CTL_VETVDOC_TONAM, text_buf);
		text_buf.Z();
		if(!!r_bat.Product.Guid) {
			temp_buf.Z().Cat(r_bat.Product.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		dlg->setCtrlString(CTL_VETVDOC_PRODSI, text_buf);
		dlg->setCtrlString(CTL_VETVDOC_PRODNAM, r_bat.Product.Name);

		text_buf.Z();
		if(!!r_bat.SubProduct.Guid) {
			temp_buf.Z().Cat(r_bat.SubProduct.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		dlg->setCtrlString(CTL_VETVDOC_SPRODSI, text_buf);
		dlg->setCtrlString(CTL_VETVDOC_SPRODNAM, r_bat.SubProduct.Name);

		text_buf.Z();
		if(!!r_bat.ProductItem.Guid) {
			temp_buf.Z().Cat(r_bat.ProductItem.Guid, S_GUID::fmtIDL|S_GUID::fmtLower);
			text_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
		}
		dlg->setCtrlString(CTL_VETVDOC_ITEMSI, text_buf);
		dlg->setCtrlString(CTL_VETVDOC_ITEMNAM, r_bat.ProductItem.Name);
		if(r_bat.NativeGoodsID) {
			GetGoodsName(r_bat.NativeGoodsID, temp_buf);
			dlg->setCtrlString(CTL_VETVDOC_ITEMLNK, temp_buf);
		}
		{
			text_buf.Z();
			if(rData.Flags & VetisGenericVersioningEntity::fActive)
				text_buf.CatDivIfNotEmpty(',', 2).Cat("ACTIVE");
			if(rData.Flags & VetisGenericVersioningEntity::fLast)
				text_buf.CatDivIfNotEmpty(',', 2).Cat("LAST");
			dlg->setStaticText(CTL_VETVDOC_ST_FLAGS, text_buf);
		}
		dlg->SetupNativeBill();
		if(ExecView(dlg) == cmOK) {
			if(!(rData.Flags & rData.fFromMainOrg)) {
				dlg->getCtrlData(CTL_VETVDOC_ACKQTTY, &r_bat.AckVolume);
				//long   ack = CHKXORFLAGS(rData.Flags, rData.fPartlyAccepted, rData.fRejected);
				long   ack = dlg->GetClusterData(CTL_VETVDOC_ACK);
				rData.Flags &= ~(VetisVetDocument::fPartlyAccepted|VetisVetDocument::fRejected);
				if(ack == VetisVetDocument::fPartlyAccepted)
					rData.Flags |= rData.fPartlyAccepted;
				else if(ack == VetisVetDocument::fRejected)
					rData.Flags |= rData.fRejected;
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(VetisDocument); VetisDocumentFilt::VetisDocumentFilt() : PPBaseFilt(PPFILT_VETISDOCUMENT, 0, 1)
{
	SetFlatChunk(offsetof(VetisDocumentFilt, ReserveStart),
		offsetof(VetisDocumentFilt, ReserveEnd) - offsetof(VetisDocumentFilt, ReserveStart));
	Init(1, 0);
}

int FASTCALL VetisDocumentFilt::GetStatusList(PPIDArray & rList) const
{
	rList.clear();
	if(VDStatusFlags) {
		static const VetisDocStatus statuses[] = {vetisdocstCREATED, vetisdocstCONFIRMED, vetisdocstWITHDRAWN, vetisdocstUTILIZED, vetisdocstFINALIZED, vetisdocstOUTGOING_PREPARING, vetisdocstSTOCK};
		for(uint i = 0; i < SIZEOFARRAY(statuses); i++)
			if(VDStatusFlags & (1<<statuses[i]))
				rList.add(statuses[i]);
		return 1;
	}
	else
		return -1;
}

PPViewVetisDocument::PPViewVetisDocument() : PPView(0, &Filt, PPVIEW_VETISDOCUMENT, 0, 0)
{
}

PPViewVetisDocument::~PPViewVetisDocument()
{
}

class VetisDocumentFiltDialog : public TDialog {
	DECL_DIALOG_DATA(VetisDocumentFilt);
public:
	VetisDocumentFiltDialog() : TDialog(DLG_VETDOCFLT)
	{
		SetupCalPeriod(CTLCAL_VETDOCFLT_PRD, CTL_VETDOCFLT_PRD);
		SetupCalPeriod(CTLCAL_VETDOCFLT_WBPRD, CTL_VETDOCFLT_WBPRD);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		SetPeriodInput(this, CTL_VETDOCFLT_PRD,   Data.Period);
		SetPeriodInput(this, CTL_VETDOCFLT_WBPRD, Data.WayBillPeriod);
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 0, (1<<vetisdocstCREATED));
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 1, (1<<vetisdocstCONFIRMED));
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 2, (1<<vetisdocstWITHDRAWN));
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 3, (1<<vetisdocstUTILIZED));
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 4, (1<<vetisdocstFINALIZED));
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 5, (1<<vetisdocstOUTGOING_PREPARING));
		AddClusterAssoc(CTL_VETDOCFLT_STATUS, 6, (1<<vetisdocstSTOCK));
		SetClusterData(CTL_VETDOCFLT_STATUS, Data.VDStatusFlags);
		SetupLocationCombo(this, CTLSEL_VETDOCFLT_WH, Data.LocID__, 0, LOCTYP_WAREHOUSE, 0);
		{
			const  PPID acs_from_id = GetSupplAccSheet();
			const  PPID acs_to_id = GetSellAccSheet();
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			if(acs_obj.Fetch(acs_from_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
				SetupPersonCombo(this, CTLSEL_VETDOCFLT_FROMP, Data.FromPersonID, 0, acs_rec.ObjGroup, 1);
			}
			if(acs_obj.Fetch(acs_to_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
				SetupPersonCombo(this, CTLSEL_VETDOCFLT_TOP, Data.ToPersonID, 0, acs_rec.ObjGroup, 1);
			}
		}
		setCtrlUInt16(CTL_VETDOCFLT_FTEXPIRY, (Data.Ft_Expiry > 0)   ? 1 : ((Data.Ft_Expiry < 0)   ? 2 : 0));
		setCtrlUInt16(CTL_VETDOCFLT_FTMATCH,  (Data.Ft_LotMatch > 0) ? 1 : ((Data.Ft_LotMatch < 0) ? 2 : 0));
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetPeriodInput(this, CTL_VETDOCFLT_PRD,   &Data.Period);
		GetPeriodInput(this, CTL_VETDOCFLT_WBPRD, &Data.WayBillPeriod);
		GetClusterData(CTL_VETDOCFLT_STATUS, &Data.VDStatusFlags);
		getCtrlData(CTLSEL_VETDOCFLT_WH, &Data.LocID__);
		getCtrlData(CTLSEL_VETDOCFLT_FROMP, &Data.FromPersonID);
		Data.FromLocID = 0;
		getCtrlData(CTLSEL_VETDOCFLT_TOP, &Data.ToPersonID);
		Data.ToLocID = 0;
		{
			const uint16 v = getCtrlUInt16(CTL_VETDOCFLT_FTEXPIRY);
			Data.Ft_Expiry = (v == 1) ? 1 : ((v == 2) ? -1 : 0);
		}
		{
			const uint16 v = getCtrlUInt16(CTL_VETDOCFLT_FTMATCH);
			Data.Ft_LotMatch = (v == 1) ? 1 : ((v == 2) ? -1 : 0);
		}
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

/*static*/int FASTCALL PPViewVetisDocument::EditInterchangeParam(VetisDocumentFilt * pFilt)
{
	class VetisInterchangeFiltDialog : public TDialog {
		DECL_DIALOG_DATA(VetisDocumentFilt);
	public:
		VetisInterchangeFiltDialog() : TDialog(DLG_VETISIC)
		{
			SetupCalPeriod(CTLCAL_VETDOCFLT_PRD, CTL_VETDOCFLT_PRD);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_VETDOCFLT_PRD, Data.Period);
			SetupPersonCombo(this, CTLSEL_VETDOCFLT_MAINORG, 0, Data.MainOrgID, PPPRK_MAIN, 1);
			SetupLocationCombo(this, CTLSEL_VETDOCFLT_WH, Data.LocID__, 0, LOCTYP_WAREHOUSE, 0);
			AddClusterAssoc(CTL_VETDOCFLT_ACTIONS, 0, VetisDocumentFilt::icacnLoadUpdated);
			AddClusterAssoc(CTL_VETDOCFLT_ACTIONS, 1, VetisDocumentFilt::icacnLoadStock);
			AddClusterAssoc(CTL_VETDOCFLT_ACTIONS, 2, VetisDocumentFilt::icacnLoadAllDocs);
			AddClusterAssoc(CTL_VETDOCFLT_ACTIONS, 3, VetisDocumentFilt::icacnPrepareOutgoing);
			AddClusterAssoc(CTL_VETDOCFLT_ACTIONS, 4, VetisDocumentFilt::icacnSendOutgoing);
			AddClusterAssoc(CTL_VETDOCFLT_ACTIONS, 5, VetisDocumentFilt::icacnRefsImport); // @v11.0.10
			SetClusterData(CTL_VETDOCFLT_ACTIONS, Data.Actions);
			// @v11.0.10 {
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 0, VetisDocumentFilt::refimpfCountry);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 1, VetisDocumentFilt::refimpfRegion);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 2, VetisDocumentFilt::refimpfUOM);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 3, VetisDocumentFilt::refimpfEnterprise);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 4, VetisDocumentFilt::refimpfLocation);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 5, VetisDocumentFilt::refimpfProductGroup);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 6, VetisDocumentFilt::refimpfProductSubGroup);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 7, VetisDocumentFilt::refimpfProductItem);
			AddClusterAssoc(CTL_VETDOCFLT_RFIMPF, 8, VetisDocumentFilt::refimpfPurpose);
			SetClusterData(CTL_VETDOCFLT_RFIMPF, Data.RefsImpFlags);
			disableCtrl(CTL_VETDOCFLT_RFIMPF, !(Data.Actions & VetisDocumentFilt::icacnRefsImport));
			// } @v11.0.10 
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetPeriodInput(this, CTL_VETDOCFLT_PRD,   &Data.Period);
			getCtrlData(CTLSEL_VETDOCFLT_MAINORG, &Data.MainOrgID);
			getCtrlData(CTLSEL_VETDOCFLT_WH, &Data.LocID__);
			GetClusterData(CTL_VETDOCFLT_ACTIONS, &Data.Actions);
			GetClusterData(CTL_VETDOCFLT_RFIMPF, &Data.RefsImpFlags); // @v11.0.10
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_VETDOCFLT_ACTIONS)) {
				GetClusterData(CTL_VETDOCFLT_ACTIONS, &Data.Actions);
				disableCtrl(CTL_VETDOCFLT_RFIMPF, !(Data.Actions & VetisDocumentFilt::icacnRefsImport));
			}
		}
	};
	if(pFilt) {
		pFilt->FiltKind = VetisDocumentFilt::fkInterchangeParam;
	}
	DIALOG_PROC_BODY(VetisInterchangeFiltDialog, pFilt);
}

static int _SetupTimeChunkByDateRange(const DateRange & rPeriod, STimeChunk & rTc)
{
	const int is_empty_period = rPeriod.IsZero();
	if(checkdate(rPeriod.low)) {
		rTc.Start.Set(rPeriod.low, ZEROTIME);
		if(checkdate(rPeriod.upp))
			rTc.Finish.Set(rPeriod.upp, MAXDAYTIME);
		else
			rTc.Finish.Set(rPeriod.low, MAXDAYTIME);
	}
	else if(checkdate(rPeriod.upp)) {
		rTc.Start.Set(rPeriod.upp, ZEROTIME);
		rTc.Finish.Set(rPeriod.upp, MAXDAYTIME);
	}
	else {
		const LDATE cd = getcurdate_();
		rTc.Start.Set(plusdate(cd, -1), ZEROTIME);
		rTc.Finish.Set(cd, MAXDAYTIME);
	}
	return is_empty_period;
}

/*static*/int FASTCALL PPViewVetisDocument::RunInterchangeProcess(VetisDocumentFilt * pFilt__)
{
	int    ok = -1;
	PPLogger logger;
	VetisDocumentFilt inner_filt;
	if(pFilt__ == 0) {
		inner_filt.FiltKind = VetisDocumentFilt::fkInterchangeParam;
		if(EditInterchangeParam(&inner_filt) > 0) {
			pFilt__ = &inner_filt;
		}
	}
	if(pFilt__ && pFilt__->FiltKind == VetisDocumentFilt::fkInterchangeParam) {
		VetisDocumentFilt filt;
		filt = *pFilt__;
		filt.Period.Actualize(ZERODATE);
		filt.WayBillPeriod.Actualize(ZERODATE);
		PPVetisInterface::Param param(filt.MainOrgID, filt.LocID__, 0);
		THROW(PPVetisInterface::SetupParam(param));
		{
			SString fmt_buf, msg_buf;
			SString temp_buf;
			PPVetisInterface ifc(&logger);
			VetisApplicationBlock reply;
			TSVector <VetisEntityCore::UnresolvedEntity> ure_list;
			const uint req_count = 1000;
			STimeChunk tc;
			PPObjGoods goods_obj;
			const int is_init_period_zero = _SetupTimeChunkByDateRange(filt.Period, tc);
			THROW(ifc.Init(param));
			PPWaitStart();
			if(filt.Actions & VetisDocumentFilt::icacnRefsImport) { // импорт справочников (просто в плоские файлы)
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfCountry) {
					if(ifc.GetCountryList(reply) > 0) {
						PPGetFilePath(PPPATH_OUT, "vetis_country_list.txt", temp_buf);
						SFile f_out(temp_buf, SFile::mWrite);
						if(f_out.IsValid()) {
							for(uint i = 0; i < reply.CountryList.getCount(); i++) {
								const VetisCountry * p_item = reply.CountryList.at(i);
								if(p_item) {
									temp_buf.Z();
									temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
									temp_buf.Tab().Cat(p_item->Code).Tab().Cat(p_item->Code3).Tab().Cat(p_item->Name).Tab().Cat(p_item->EnglishName);
									f_out.WriteLine(temp_buf.CR());
								}
							}
						}
					}
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfPurpose) {
					if(ifc.GetPurposeList(0, 1000, reply) > 0) {
						PPGetFilePath(PPPATH_OUT, "vetis_purpose_list.txt", temp_buf);
						SFile f_out(temp_buf, SFile::mWrite);
						if(f_out.IsValid()) {
							for(uint i = 0; i < reply.PurposeList.getCount(); i++) {
								const VetisPurpose * p_item = reply.PurposeList.at(i);
								if(p_item) {
									temp_buf.Z();
									temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL).Tab().Cat(p_item->Name);
									f_out.WriteLine(temp_buf.CR());
								}
							}
						}
					}
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfRegion) {
					if(ifc.GetRegionList(S_GUID(P_VetisGuid_Country_Ru), reply) > 0) {
						PPGetFilePath(PPPATH_OUT, "vetis_region_list.txt", temp_buf);
						SFile f_out(temp_buf, SFile::mWrite);
						if(f_out.IsValid()) {
							temp_buf.Z().Cat("uuid").Tab().Cat("guid");temp_buf.Tab().Cat("country_guid").Tab().Cat("code").Tab().Cat("type").Tab().Cat("name").Tab().Cat("en_name").Tab().Cat("view");
							f_out.WriteLine(temp_buf.CR());
							for(uint i = 0; i < reply.RegionList.getCount(); i++) {
								const VetisAddressObjectView * p_item = reply.RegionList.at(i);
								if(p_item) {
									temp_buf.Z();
									temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
									temp_buf.Tab().Cat(p_item->CountryGUID, S_GUID::fmtIDL).Tab().Cat(p_item->RegionCode).Tab().Cat(p_item->Type).Tab().
										Cat(p_item->Name).Tab().Cat(p_item->EnglishName).Tab().Cat(p_item->View);
									f_out.WriteLine(temp_buf.CR());
								}
							}
						}
					}
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfUOM) {
					uint req_count = 1000;
					PPGetFilePath(PPPATH_OUT, "vetis_unit_list.txt", temp_buf);
					SFile f_out(temp_buf, SFile::mWrite);
					for(uint req_offs = 0; ifc.GetUnitList(req_offs, req_count, reply);) {
						PPTransaction tra(1);
						THROW(tra);
						for(uint i = 0; i < reply.UnitList.getCount(); i++) {
							const VetisUnit * p_item = reply.UnitList.at(i);
							if(p_item) {
								temp_buf.Z();
								temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
								temp_buf.Tab().Cat(p_item->Name).Tab().Cat(p_item->CommonUnitGuid, S_GUID::fmtIDL).Tab().Cat(p_item->Factor, MKSFMTD(0, 6, 0));
								f_out.WriteLine(temp_buf.CR());
								{
									VetisEntityCore::Entity sub_entity(VetisEntityCore::kUOM, *p_item);
									THROW(ifc.PeC.SetEntity(sub_entity, /*pUreList*/0, 0, 0));
									//rec.UOMID = sub_entity.ID;
								}
							}
						}
						THROW(tra.Commit());
						if(reply.UnitList.getCount() < req_count)
							break;
						else
							req_offs += reply.UnitList.getCount();
					}
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfEnterprise) {
					const uint max_zeroresult_tries = 3;
					uint  zeroresult_try = 0;
					PPGetFilePath(PPPATH_OUT, "vetis_bent_list.txt", temp_buf);
					SFile f_out(temp_buf, SFile::mWrite);
					if(f_out.IsValid()) {
						uint req_count = 1000;
						VetisApplicationBlock reply;
						for(uint req_offs = 0; ifc.GetBusinessEntityList(req_offs, req_count, reply);) {
							if(reply.BEntList.getCount() == 0) {
								if(zeroresult_try < max_zeroresult_tries)
									zeroresult_try++;
								else
									break;
							}
							else {
								PPTransaction tra(1);
								THROW(tra);
								for(uint i = 0; i < reply.BEntList.getCount(); i++) {
									const VetisBusinessEntity * p_item = reply.BEntList.at(i);
									if(p_item) {
										temp_buf.Z();
										temp_buf.Cat(p_item->Uuid, S_GUID::fmtIDL).Tab().Cat(p_item->Guid, S_GUID::fmtIDL);
										temp_buf.Tab().Cat(p_item->Name).Tab().Cat(p_item->Fio).Tab().Cat(p_item->Inn).Tab().Cat(p_item->Kpp);
										temp_buf.Tab().Cat(p_item->JuridicalAddress.AddressView);
										f_out.WriteLine(temp_buf.CR());
										PPID   pi_id = 0;
										THROW(ifc.PeC.Put(&pi_id, *p_item, &ure_list, 0));
									}
								}
								THROW(tra.Commit());
								PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
								zeroresult_try = 0;
								if(reply.BEntList.getCount() < req_count)
									break;
								else
									req_offs += reply.BEntList.getCount();
							}
						}
					}
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfLocation) {
					PPGetFilePath(PPPATH_OUT, "vetis_bloc_list.txt", temp_buf);
					SFile f_out(temp_buf, SFile::mWrite);
					if(f_out.IsValid()) {
						uint req_count = 1000;
						for(uint req_offs = 0; ifc.GetRussianEnterpriseList(req_offs, req_count, reply);) {
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < reply.EntItemList.getCount(); i++) {
								const VetisEnterprise * p_item = reply.EntItemList.at(i);
								if(p_item) {
									Debug_OutputEntItem(*p_item, temp_buf);
									f_out.WriteLine(temp_buf.CR());
									PPID   pi_id = 0;
									THROW(ifc.PeC.Put(&pi_id, *p_item, &ure_list, 0));
								}
							}
							THROW(tra.Commit());
							PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
							if(reply.EntItemList.getCount() < req_count)
								break;
							else
								req_offs += reply.EntItemList.getCount();
						}
					}
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfProductGroup) {
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfProductSubGroup) {
				}
				if(filt.RefsImpFlags & VetisDocumentFilt::refimpfProductItem) {
					PPGetFilePath(PPPATH_OUT, "vetis_productitem_list.txt", temp_buf);
					SFile f_out(temp_buf, SFile::mWrite);
					if(f_out.IsValid()) {
						uint req_count = 1000;
						for(uint req_offs = 0; ifc.GetProductItemList(req_offs, req_count, reply);) {
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < reply.ProductItemList.getCount(); i++) {
								VetisProductItem * p_item = reply.ProductItemList.at(i);
								if(p_item) {
									if(p_item->GlobalID.NotEmpty()) {
										BarcodeTbl::Rec bc_rec;
										if(goods_obj.SearchByBarcode(p_item->GlobalID, &bc_rec, 0, 1) > 0)
											p_item->NativeGoodsID = bc_rec.GoodsID;
									}
									Debug_OutputProductItem(*p_item, temp_buf);
									f_out.WriteLine(temp_buf.CR());
									PPID   pi_id = 0;
									THROW(ifc.PeC.Put(&pi_id, ifc.PeC.kProductItem, *p_item, &ure_list, 0));
								}
							}
							THROW(tra.Commit());
							PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total);
							if(reply.ProductItemList.getCount() < req_count)
								break;
							else
								req_offs += reply.ProductItemList.getCount();
						}
					}
				}
			}
			if(filt.Actions & VetisDocumentFilt::icacnLoadAllDocs) { // полная загрузка
				PPLoadText(PPTXT_VETISGETTINGALLDOCS, msg_buf);
				PPWaitMsg(msg_buf);
				for(uint req_offs = 0; ifc.GetVetDocumentList(tc, req_offs, req_count, reply);) {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < reply.VetDocList.getCount(); i++) {
						const VetisVetDocument * p_item = reply.VetDocList.at(i);
						if(p_item) {
							PPID   pi_id = 0;
							THROW(ifc.PeC.Put(&pi_id, *p_item, 0, &ure_list, 0));
							ok = 1;
						}
					}
					THROW(tra.Commit());
					PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total, msg_buf);
					if(reply.VetDocList.getCount() < req_count)
						break;
					else
						req_offs += reply.VetDocList.getCount();
				}
			}
			else if(filt.Actions & VetisDocumentFilt::icacnLoadUpdated) { // изменения
				PPLoadText(PPTXT_VETISGETTINGUPD, fmt_buf);
				temp_buf.Z().Cat(tc.Start, DATF_DMY, TIMF_HM).Cat("..").Cat(tc.Finish, DATF_DMY, TIMF_HM);
				msg_buf.Printf(fmt_buf, temp_buf.cptr());
				PPWaitMsg(msg_buf);
				for(uint req_offs = 0; ifc.GetVetDocumentChangesList(tc, req_offs, req_count, reply);) {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < reply.VetDocList.getCount(); i++) {
						const VetisVetDocument * p_item = reply.VetDocList.at(i);
						if(p_item) {
							PPID   pi_id = 0;
							THROW(ifc.PeC.Put(&pi_id, *p_item, 0, &ure_list, 0));
							ok = 1;
						}
					}
					THROW(tra.Commit());
					PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total, msg_buf);
					if(reply.VetDocList.getCount() < req_count)
						break;
					else
						req_offs += reply.VetDocList.getCount();
				}
			}
			if(filt.Actions & VetisDocumentFilt::icacnLoadStock) { // текущие остатки
				if(is_init_period_zero) {
					PPLoadText(PPTXT_VETISGETTINGSTOCK, msg_buf);
				}
				else {
					PPLoadText(PPTXT_VETISGETTINGSTOCKUPD, fmt_buf);
					temp_buf.Z().Cat(tc.Start, DATF_DMY, TIMF_HM).Cat("..").Cat(tc.Finish, DATF_DMY, TIMF_HM);
					msg_buf.Printf(fmt_buf, temp_buf.cptr());
				}
				PPWaitMsg(msg_buf);
				for(uint req_offs = 0; is_init_period_zero ? ifc.GetStockEntryList(req_offs, req_count, reply) : ifc.GetStockEntryChangesList(tc, req_offs, req_count, reply);) {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < reply.VetStockList.getCount(); i++) {
						const VetisStockEntry * p_item = reply.VetStockList.at(i);
						if(p_item) {
							PPID   pi_id = 0;
							THROW(ifc.PeC.Put(&pi_id, param.IssuerUUID, param.EntUUID, *p_item, &ure_list, 0));
							ok = 1;
						}
					}
					THROW(tra.Commit());
					PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total, msg_buf);
					if(reply.VetStockList.getCount() < req_count)
						break;
					else
						req_offs += reply.VetStockList.getCount();
				}
			}
			if(filt.Actions & VetisDocumentFilt::icacnPrepareOutgoing) {
				DateRange period;
				if(checkdate(filt.Period.low))
					period.low = filt.Period.low;
				if(checkdate(filt.Period.upp))
					period.upp = filt.Period.upp;
				if(!checkdate(period.low)) {
					period.low = ValidDateOr(period.upp, getcurdate_());
				}
				if(!checkdate(period.upp)) {
					period.upp = getcurdate_();
				}
				if(ifc.SetupOutgoingEntries(filt.LocID__, period) > 0)
					ok = 1;
			}
			if(filt.Actions & VetisDocumentFilt::icacnSendOutgoing) {
			}
			if(ok > 0)
				THROW(ifc.ProcessUnresolvedEntityList(ure_list));
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	logger.Save(PPFILNAM_VETISINFO_LOG, 0);
	return ok;
}

PPBaseFilt * PPViewVetisDocument::CreateFilt(const void * extraPtr) const
{
	VetisDocumentFilt * p_filt = new VetisDocumentFilt;
	if(p_filt) {
		p_filt->VDStatusFlags |= (1<<vetisdocstCREATED);
		p_filt->VDStatusFlags |= (1<<vetisdocstCONFIRMED);
		p_filt->VDStatusFlags |= (1<<vetisdocstWITHDRAWN);
		p_filt->VDStatusFlags |= (1<<vetisdocstUTILIZED);
		p_filt->VDStatusFlags |= (1<<vetisdocstFINALIZED);
		p_filt->VDStatusFlags |= (1<<vetisdocstOUTGOING_PREPARING);
		//p_filt->VDStatusFlags |= (1<<vetisdocstSTOCK);
	}
	return p_filt;
}

int PPViewVetisDocument::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return PPErrorZ();
	VetisDocumentFilt * p_filt = static_cast<VetisDocumentFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(VetisDocumentFiltDialog, p_filt);
}

int PPViewVetisDocument::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW(Helper_InitBaseFilt(pBaseFilt));
	Filt.Period.Actualize(ZERODATE);
	Filt.WayBillPeriod.Actualize(ZERODATE);
	FromEntityIdList.Z();
	LocEntityID = 0; // @v11.5.8
	FromEnterpriseID = 0;
	ToEntityID = 0;
	ToEnterpriseID = 0;
	{
		S_GUID guid;
		VetisEntityCore::Entity ent;
		// @v11.5.8 {
		if(Filt.LocID__) {
			TSCollection <VetisEntityCore::Entity> ent_list;
			if(p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, Filt.LocID__, PPTAG_LOC_VETIS_GUID, guid) > 0 && EC.GetEntityListByGuid(guid, ent_list) > 0) {
				for(uint i = 0; !LocEntityID && i < ent_list.getCount(); i++) {
					const VetisEntityCore::Entity * p_ent = ent_list.at(i);
					if(p_ent && p_ent->Kind == VetisEntityCore::kEnterprise)
						LocEntityID = p_ent->ID;
				}				
			}
		}
		// } @v11.5.8 
		if(Filt.FromPersonID) {
			TSCollection <VetisEntityCore::Entity> ent_list;
			if(p_ref->Ot.GetTagGuid(PPOBJ_PERSON, Filt.FromPersonID, PPTAG_PERSON_VETISUUID, guid) > 0 && EC.GetEntityListByGuid(guid, ent_list) > 0) {
				for(uint i = 0; i < ent_list.getCount(); i++) {
					const VetisEntityCore::Entity * p_ent = ent_list.at(i);
					if(p_ent)
						FromEntityIdList.add(p_ent->ID);
				}
			}
		}
		if(Filt.ToPersonID) {
			if(p_ref->Ot.GetTagGuid(PPOBJ_PERSON, Filt.ToPersonID, PPTAG_PERSON_VETISUUID, guid) > 0 && EC.GetEntityByGuid(guid, ent) > 0)
				ToEnterpriseID = ent.ID;
			else
				ToEnterpriseID = -1;
		}
	}
	CATCHZOK
	return ok;
}

static IMPL_DBE_PROC(dbqf_vetis_clsftextfld_iisi)
{
	char   result_buf[32];
	if(option == CALC_SIZE) {
		result->init(sizeof(result_buf));
	}
	else {
		PPID   _id = params[0].lval;
		//PPID    = params[1].lval;
		SString temp_buf;
		STRNSCPY(result_buf, temp_buf);
		result->init(result_buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_entitytextfld_ip)
{
	char   result_buf[128];
	if(option == CALC_SIZE) {
		result->init(static_cast<long>(sizeof(result_buf)));
	}
	else {
		Reference * p_ref = PPRef;
		const  PPID entity_id = params[0].lval;
		VetisEntityCore * p_ec = reinterpret_cast<VetisEntityCore *>(params[1].ptrv);
		SString temp_buf;
		SStringU temp_buf_u;
		if(entity_id) {
			p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, entity_id, PPTRPROP_NAME), temp_buf_u);
		}
		if(temp_buf_u.Len()) {
			temp_buf_u.CopyToUtf8(temp_buf, 0);
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		STRNSCPY(result_buf, temp_buf);
		result->init(result_buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_businessmembtextfld_iip)
{
	char   result_buf[128];
	if(option == CALC_SIZE) {
		result->init(static_cast<long>(sizeof(result_buf)));
	}
	else {
		Reference * p_ref = PPRef;
		PPID   enterprise_id = params[0].lval;
		PPID   bent_id = params[1].lval;
		const VetisEntityCore * p_ec = static_cast<const VetisEntityCore *>(params[2].ptrval);
		SString temp_buf;
		SStringU temp_buf_u;
		// @v10.4.8 Поменял приоритет: сначала пытаемся получить имя enterprise затем entity (до этого было наоборот)
		if(enterprise_id) {
			p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, enterprise_id, PPTRPROP_NAME), temp_buf_u);
		}
		if(!temp_buf_u.Len() && bent_id) {
			p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, bent_id, PPTRPROP_NAME), temp_buf_u);
		}
		if(temp_buf_u.Len()) {
			temp_buf_u.CopyToUtf8(temp_buf, 0);
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		STRNSCPY(result_buf, temp_buf);
		result->init(result_buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_vetstockbydoc_i)
{
	double stock = -1.0; // Индицирует отсутствие связанной записи
	PPID   doc_id = params[0].lval;
	VetisEntityCore * p_ec = const_cast<VetisEntityCore *>(static_cast<const VetisEntityCore *>(params[1].ptrval)); // @badcast
	if(doc_id) {
		VetisDocumentTbl::Key9 k9;
		k9.OrgDocEntityID = doc_id;
		if(p_ec->DT.search(9, &k9, spEq)) do {
			// fLast
			const VetisDocumentTbl::Rec & r_rec = p_ec->DT.data;
			if(r_rec.VetisDocStatus == vetisdocstSTOCK) {
				if(r_rec.Flags & VetisGenericVersioningEntity::fActive && r_rec.Flags & VetisGenericVersioningEntity::fLast)
					stock = p_ec->DT.data.Volume;
				// @v10.1.12 (будем считать, что актуальная - последняя запись) break;
			}
		} while(p_ec->DT.search(9, &k9, spNext) && p_ec->DT.data.OrgDocEntityID == doc_id);
	}
	result->init(stock);
}

// @erik v10.4.11 {
static IMPL_DBE_PROC(dbqf_vetis_vet_uuid_i)
{
	char   result_buf[64];
	if(option == CALC_SIZE) {
		result->init(static_cast<long>(sizeof(result_buf)));
	}
	else {
		Reference * p_ref = PPRef;
		const  PPID entity_id = params[0].lval;
		VetisEntityCore * p_ec = const_cast<VetisEntityCore *>(static_cast<const VetisEntityCore *>(params[1].ptrval));
		SString temp_buf;
		if(p_ec) {
			VetisEntityCore::Entity entity;
			if(entity_id && p_ec->GetEntity(entity_id, entity) && entity.Kind == VetisEntityCore::kVetDocument)
				entity.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
		}
		STRNSCPY(result_buf, temp_buf);
		result->init(result_buf);
	}
}
// } @erik v10.4.11

/*void foo()
{
	VetisDocumentTbl::Key9 k9;
	k9.OrgDocEntityID = app_data.VdRec.OrgDocEntityID;
	if(PeC.DT.search(9, &k9, spEq)) do {
		if(PeC.DT.data.VetisDocStatus == vetisdocstSTOCK) {
			PeC.GetEntity(PeC.DT.data.EntityID, sub_entity);
			app_data.StockEntryGuid = sub_entity.Guid;
			app_data.StockEntryUuid = sub_entity.Uuid;
			break;
		}
	} while(PeC.DT.search(9, &k9, spNext) && PeC.DT.data.OrgDocEntityID == app_data.VdRec.OrgDocEntityID);
	temp_buf.Z().Cat(org_doc_entity.WayBillNumber).CatDiv('-', 0).Cat(org_doc_entity.WayBillDate).CatChar('-').Cat(org_doc_entity.NativeBillID);
	THROW_PP_S(!!app_data.StockEntryGuid && !!app_data.StockEntryUuid, PPERR_VETISHASNTSTOCKREF, temp_buf);
}*/

/*static*/int VetisEntityCore::GetProductItemName(PPID entityID, PPID productItemID, PPID subProductID, PPID productID, SString & rBuf)
{
	rBuf.Z();
	int    ok = -1;
	Reference * p_ref = PPRef;
	SStringU temp_buf_u;
	const struct {
		PPID   ObjType;
		PPID   ObjID;
		int16  TxtProp;
		int16  Result;
	} tptab[] = {
		{ PPOBJ_VETISENTITY, productItemID, PPTRPROP_NAME, 1 },
		{ PPOBJ_VETISENTITY, entityID, VetisEntityCore::txtprpProductItemName, 2 },
		{ PPOBJ_VETISENTITY, subProductID, PPTRPROP_NAME, 3 },
		{ PPOBJ_VETISENTITY, productID, PPTRPROP_NAME, 4 },
	};
	for(uint i = 0; i < SIZEOFARRAY(tptab); i++) {
		if(tptab[i].ObjID) {
			int   sr = (tptab[i].TxtProp == PPTRPROP_NAME) ?
				p_ref->TrT.Search(TextRefIdent(tptab[i].ObjType, tptab[i].ObjID, tptab[i].TxtProp), temp_buf_u) :
				p_ref->UtrC.SearchU(TextRefIdent(tptab[i].ObjType, tptab[i].ObjID, tptab[i].TxtProp), temp_buf_u);
			if(sr > 0) {
				if(temp_buf_u.Len()) {
					temp_buf_u.CopyToUtf8(rBuf, 0);
					rBuf.Transf(CTRANSF_UTF8_TO_INNER);
					ok = tptab[i].Result;
					break;
				}
			}
		}
	}
	return ok;
}

/*static*/int FASTCALL VetisEntityCore::CheckExpiryDate(int64 expiryFrom, int64 expiryTo)
{
	long   ok = 1;
	if(expiryFrom || expiryTo) {
		SUniTime ut;
		LDATETIME dtm;
		ut.FromInt64(NZOR(expiryTo, expiryFrom));
		if(ut.Get(dtm) && dtm.d <= getcurdate_())
			ok = 0;
	}
	return ok;
}

static IMPL_DBE_PROC(dbqf_vetis_productitemtextfld_iiiiip)
{
	char   result_buf[128];
	if(option == CALC_SIZE) {
		result->init(sizeof(result_buf));
	}
	else {
		PPID   product_item_id = params[0].lval;
		PPID   subproduct_id = params[1].lval;
		PPID   product_id = params[2].lval;
		PPID   entity_id = params[3].lval;
		PPID   native_goods_id = params[4].lval;
		SString temp_buf;
		VetisEntityCore::GetProductItemName(entity_id, product_item_id, subproduct_id, product_id, temp_buf);
		if(temp_buf.IsEmpty() && native_goods_id)
			GetGoodsName(native_goods_id, temp_buf);
		STRNSCPY(result_buf, temp_buf);
		result->init(result_buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_vetdstatus_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		SString temp_buf;
		PPGetSubStrById(PPTXT_VETIS_DOCSTATUS, params[0].lval, temp_buf);
		if(temp_buf.IsEmpty())
			SIntToSymbTab_GetSymb(VetisVetDocStatus_SymbTab, SIZEOFARRAY(VetisVetDocStatus_SymbTab), params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_vetdform_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		SIntToSymbTab_GetSymb(VetisVetDocFormat_SymbTab, SIZEOFARRAY(VetisVetDocFormat_SymbTab), params[0].lval, r_temp_buf);
		r_temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_vetdtype_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		SIntToSymbTab_GetSymb(VetisVetDocType_SymbTab, SIZEOFARRAY(VetisVetDocType_SymbTab), params[0].lval, r_temp_buf);
		r_temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_vetis_vet_checkexpiry_ii)
{
	result->init(static_cast<long>(VetisEntityCore::CheckExpiryDate(params[0].i64val, params[1].i64val)));
}

static IMPL_DBE_PROC(dbqf_vetis_vet_checklocation_iii)
{
	PPID loc_enterprise_id = static_cast<long>(params[0].lval);
	PPID from_enterprise_id = static_cast<long>(params[1].lval);
	PPID to_enterprise_id = static_cast<long>(params[2].lval);
	result->init(static_cast<long>(!loc_enterprise_id || loc_enterprise_id == from_enterprise_id || loc_enterprise_id == to_enterprise_id));
}

int PPViewVetisDocument::DynFuncEntityTextFld      = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_entitytextfld_ip, 2, BTS_INT, BTS_PTR);
int PPViewVetisDocument::DynFuncBMembTextFld       = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_businessmembtextfld_iip, 3, BTS_INT, BTS_INT, BTS_PTR);
int PPViewVetisDocument::DynFuncProductItemTextFld = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_productitemtextfld_iiiiip, 6, BTS_INT, BTS_INT, BTS_INT, BTS_INT, BTS_INT, BTS_PTR);
int PPViewVetisDocument::DynFuncVetDStatus         = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_vetdstatus_i, 1, BTS_INT);
int PPViewVetisDocument::DynFuncVetDForm           = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_vetdform_i, 1, BTS_INT);
int PPViewVetisDocument::DynFuncVetDType           = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_vetdtype_i, 1, BTS_INT);
int PPViewVetisDocument::DynFuncVetStockByDoc      = DbqFuncTab::RegisterDynR(BTS_REAL,   dbqf_vetis_vetstockbydoc_i, 2, BTS_INT, BTS_PTR);
int PPViewVetisDocument::DynFuncVetUUID            = DbqFuncTab::RegisterDynR(BTS_STRING, dbqf_vetis_vet_uuid_i, 2, BTS_INT, BTS_PTR);  // @erik v10.4.11
int PPViewVetisDocument::DynFuncCheckExpiry        = DbqFuncTab::RegisterDynR(BTS_INT,    dbqf_vetis_vet_checkexpiry_ii, 2, BTS_INT64_, BTS_INT64_);
int PPViewVetisDocument::DynFuncCheckLocation      = DbqFuncTab::RegisterDynR(BTS_INT,    dbqf_vetis_vet_checklocation_iii, 3, BTS_INT, BTS_INT, BTS_INT);  // @v11.5.8

DBQuery * PPViewVetisDocument::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_VETISDOCUMENT;
	DBQ * dbq = 0;
	DBQuery * q = 0;
	VetisDocumentTbl * t = 0;
	PPIDArray status_list;
	DBE    dbe_product_name;
	DBE    dbe_vetdform;
	DBE    dbe_vetdtype;
	DBE    dbe_vetdstatus;
	DBE    dbe_from;
	DBE    dbe_to;
	DBE    dbe_stock;
	DBE    dbe_uuid; //@erik v10.4.11
	DBE    dbe_checkexpiry;
	//DBE    dbe_checkloc; // @v11.5.8
	THROW(CheckTblPtr(t = new VetisDocumentTbl(EC.DT.GetName())));
	{
		dbe_product_name.init();
		dbe_product_name.push(t->ProductItemID);
		dbe_product_name.push(t->SubProductID);
		dbe_product_name.push(t->ProductID);
		dbe_product_name.push(t->EntityID);
		dbe_product_name.push(t->LinkGoodsID);
		{
			DBConst cp;
			cp.init(&EC.DT);
			dbe_product_name.push(cp);
		}
		dbe_product_name.push(static_cast<DBFunc>(DynFuncProductItemTextFld));
	}
	{
		dbe_vetdform.init();
		dbe_vetdform.push(t->VetisDocForm);
		dbe_vetdform.push(static_cast<DBFunc>(DynFuncVetDForm));
	}
	{
		dbe_vetdtype.init();
		dbe_vetdtype.push(t->VetisDocType);
		dbe_vetdtype.push(static_cast<DBFunc>(DynFuncVetDType));
	}
	{
		dbe_vetdstatus.init();
		dbe_vetdstatus.push(t->VetisDocStatus);
		dbe_vetdstatus.push(static_cast<DBFunc>(DynFuncVetDStatus));
	}
	//
	{
		dbe_from.init();
		dbe_from.push(t->FromEnterpriseID);
		dbe_from.push(t->FromEntityID);
		dbe_from.push(dbconst(&EC));
		dbe_from.push(static_cast<DBFunc>(DynFuncBMembTextFld));
	}
	{
		dbe_to.init();
		dbe_to.push(t->ToEnterpriseID);
		dbe_to.push(t->ToEntityID);
		dbe_to.push(dbconst(&EC));
		dbe_to.push(static_cast<DBFunc>(DynFuncBMembTextFld));
	}
	{
		dbe_stock.init();
		dbe_stock.push(t->EntityID);
		dbe_stock.push(dbconst(&EC));
		dbe_stock.push(static_cast<DBFunc>(DynFuncVetStockByDoc));
	}
	// @erik v10.4.11{
	{
		dbe_uuid.init();
		dbe_uuid.push(t->EntityID);
		dbe_uuid.push(dbconst(&EC));
		dbe_uuid.push(static_cast<DBFunc>(DynFuncVetUUID));
	}
	// } @erik v10.4.11
	dbq = ppcheckfiltid(dbq, t->OrgDocEntityID, Filt.LinkVDocID);
	dbq = & (*dbq && daterange(t->IssueDate, &Filt.Period));
	dbq = & (*dbq && daterange(t->WayBillDate, &Filt.WayBillPeriod));
	if(Filt.GetStatusList(status_list) > 0)
		dbq = & (*dbq && ppidlist(t->VetisDocStatus, &status_list));
	dbq = ppcheckfiltidlist(dbq, t->FromEntityID, &FromEntityIdList);
	dbq = ppcheckfiltid(dbq, t->ToEnterpriseID, ToEnterpriseID);
	if(Filt.Ft_Expiry) {
		dbe_checkexpiry.init();
		dbe_checkexpiry.push(t->ExpiryFrom);
		dbe_checkexpiry.push(t->ExpiryTo);
		dbe_checkexpiry.push(static_cast<DBFunc>(DynFuncCheckExpiry));
		dbq = (Filt.Ft_Expiry > 0) ? &(*dbq && dbe_checkexpiry == 0L) : &(*dbq && dbe_checkexpiry > 0L);
	}
	// @v11.5.8 {
	/* @v11.5.9 плохо работает - пока отключаем 
	if(LocEntityID) {
		dbe_checkloc.init();
		dbe_checkloc.push(dbconst(LocEntityID));
		dbe_checkloc.push(t->FromEnterpriseID);
		dbe_checkloc.push(t->ToEnterpriseID);
		dbe_checkloc.push(static_cast<DBFunc>(DynFuncCheckLocation));
		dbq = &(*dbq && dbe_checkloc > 0L);
	}*/
	// } @v11.5.8 
	q = &select(
		t->EntityID,          // #0
		t->Flags,             // #1
		t->LinkBillID,        // #2
		t->LinkBillRow,       // #3
		t->LinkGoodsID,       // #4
		t->FromEntityID,      // t->LinkFromPsnID,     //  #5
		t->FromEnterpriseID,  // t->LinkFromDlvrLocID, //  #6
		t->ToEntityID,        // t->LinkToPsnID,       //  #7
		t->ToEnterpriseID,    // t->LinkToDlvrLocID,   //  #8
		t->IssueDate,         // #9
		t->OrgDocEntityID,    // #10
		t->ExpiryFrom,        // #11 SUniTime
		t->ExpiryTo,          // #12 SUniTime
		dbe_vetdform,         // #13 11-->13
		dbe_vetdtype,         // #14 12-->14
		dbe_vetdstatus,       // #15 13-->15
		t->WayBillDate,       // #16 14-->16
		t->WayBillNumber,     // #17 15-->17
		dbe_product_name,     // #18 16-->18
		t->Volume,            // #19 17-->19
		dbe_from,             // #20 18-->20
		dbe_to,               // #21 19-->21
		t->IssueNumber,       // #22 20-->22
		dbe_stock,            // #23 21-->23
		dbe_uuid,             // #24 //@erik
		0L).from(t, 0).where(*dbq);
	CATCH
		if(q)
			ZDELETE(q);
		else
			ZDELETE(t);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int FASTCALL PPViewVetisDocument::CheckForFilt(const VetisDocumentViewItem * pItem)
{
	int    ok = 1;
	if(!Filt.Period.CheckDate(pItem->IssueDate))
		ok = 0;
	else if(!Filt.WayBillPeriod.CheckDate(pItem->WayBillDate))
		ok = 0;
	else if(FromEntityIdList.getCount() && !FromEntityIdList.lsearch(pItem->FromEntityID))
		ok = 0;
	else if(ToEntityID && pItem->ToEntityID != ToEntityID)
		ok = 0;
	/* @v11.5.9 плохо работает - пока отключаем
	else if(LocEntityID && (pItem->FromEnterpriseID != LocEntityID && pItem->ToEnterpriseID != LocEntityID)) // @v11.5.8
		ok = 0;
	*/
	else {
		const long ff = Filt.VDStatusFlags;
		const long rf = pItem->VetisDocStatus;
		if(ff) {
			int    r = 0;
			if(ff & (1<<vetisdocstCREATED) && rf == vetisdocstCREATED)
				r = 1;
			else if(ff & (1<<vetisdocstCONFIRMED) && rf == vetisdocstCONFIRMED)
				r = 1;
			else if(ff & (1<<vetisdocstWITHDRAWN) && rf == vetisdocstWITHDRAWN)
				r = 1;
			else if(ff & (1<<vetisdocstUTILIZED) && rf == vetisdocstUTILIZED)
				r = 1;
			else if(ff & (1<<vetisdocstFINALIZED) && rf == vetisdocstFINALIZED)
				r = 1;
			else if(ff & (1<<vetisdocstOUTGOING_PREPARING) && rf == vetisdocstOUTGOING_PREPARING)
				r = 1;
			else if(ff & (1<<vetisdocstSTOCK) && rf == vetisdocstSTOCK)
				r = 1;
			if(!r)
				ok = 0;
		}
		if(ok) {
			if(Filt.Ft_Expiry) {
				int r = VetisEntityCore::CheckExpiryDate(pItem->ExpiryFrom, pItem->ExpiryTo);
				if((Filt.Ft_Expiry > 0 && r) || (Filt.Ft_Expiry < 0 && !r))
					ok = 0;
			}
		}
	}
	return ok;
}

int PPViewVetisDocument::InitIteration()
{
	int    ok = 1;
	DBQ  * dbq = 0;
	VetisDocumentTbl * p_t = &EC.DT;
	int    idx = 0;
	union {
		VetisDocumentTbl::Key0 k0;
		VetisDocumentTbl::Key1 k1;
		VetisDocumentTbl::Key2 k2;
		VetisDocumentTbl::Key3 k3;
	} k, k_;
	MEMSZERO(k);
	BExtQuery::ZDelete(&P_IterQuery);
	if(checkdate(Filt.Period.low)) {
		idx = 2;
		k.k2.IssueDate = Filt.Period.low;
	}
	else if(checkdate(Filt.WayBillPeriod.low)) {
		idx = 3;
		k.k3.WayBillDate = Filt.WayBillPeriod.low;
	}
	else {
		idx = 0;
	}
	THROW_MEM(P_IterQuery = new BExtQuery(p_t, idx));
	P_IterQuery->selectAll();
	dbq = & (*dbq && daterange(p_t->IssueDate, &Filt.Period));
	dbq = & (*dbq && daterange(p_t->WayBillDate, &Filt.WayBillPeriod));
	dbq = ppcheckfiltidlist(dbq, p_t->FromEntityID, &FromEntityIdList);
	dbq = ppcheckfiltid(dbq, p_t->ToEntityID, ToEntityID);
	P_IterQuery->where(*dbq);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	CATCHZOK
	return ok;
}

int PPViewVetisDocument::NextIteration(VetisDocumentViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			if(CheckForFilt(&EC.DT.data)) {
				EC.DT.CopyBufTo(pItem);
				ok = 1;
			}
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewVetisDocument * p_view = static_cast<PPViewVetisDocument *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewVetisDocument::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const PPViewVetisDocument::BrwHdr * p_hdr = static_cast<const PPViewVetisDocument::BrwHdr *>(pData);
			if(r_col.OrgOffs == 22) { // issuenumber
				if(p_hdr->OrgDocEntityID)
					ok = pStyle->SetRightFigCircleColor(GetColorRef(SClrAqua));
			}
			else if(r_col.OrgOffs == 24) { // uuid 
				if(!VetisEntityCore::CheckExpiryDate(p_hdr->ExpiryFrom, p_hdr->ExpiryTo))
					ok = pStyle->SetFullCellColor(GetColorRef(SClrCrimson));
			}
			else if(r_col.OrgOffs == 17) { // waybillnumber
				if(p_hdr->LinkBillID)
					ok = pStyle->SetRightFigCircleColor(GetColorRef((p_hdr->LinkBillRow > 0) ? SClrGreen : SClrYellow));
			}
			if(r_col.OrgOffs == 18) { // goods
				if(p_hdr->LinkGoodsID)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightgreen));
			}
			else if(r_col.OrgOffs == 20) { // from
				if(p_hdr->Flags & VetisVetDocument::fFromMainOrg)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightblue));
				else if(p_hdr->LinkFromDlvrLocID)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightgreen));
				else if(p_hdr->LinkFromPsnID)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightcyan));
			}
			else if(r_col.OrgOffs == 21) { // to
				if(p_hdr->Flags & VetisVetDocument::fToMainOrg)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightblue));
				else if(p_hdr->LinkToDlvrLocID)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightgreen));
				else if(p_hdr->LinkToPsnID)
					ok = pStyle->SetFullCellColor(GetColorRef(SClrLightcyan));
			}
		}
	}
	return ok;
}

void PPViewVetisDocument::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & VetisDocumentFilt::fAsSelector) {
			PPID   sel_id = Filt.Sel;
			if(!sel_id && !!Filt.SelLotUuid) {
				VetisEntityCore::Entity ent;
				if(EC.GetEntityByUuid(Filt.SelLotUuid, ent) > 0)
					sel_id = ent.ID;
			}
			if(sel_id)
				pBrw->search2(&sel_id, CMPF_LONG, srchFirst, 0);
		}
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

class _VetisRegionRouteEntry {
	_VetisRegionRouteEntry(const _VetisRegionRouteEntry & rS) {} // @disable-copy-ctr
	_VetisRegionRouteEntry & FASTCALL operator = (const _VetisRegionRouteEntry & rS) { return *this; } // @disable-assignment-op
public:
	_VetisRegionRouteEntry()
	{
	}
	S_GUID RegionGuid_From;
	S_GUID RegionGuid_To;
	S_GUID LocGuid_To; // @v11.2.8 GUID предприятия-получателя (точка доставки)
	UuidArray SubProductGuidList;
	PPIDArray SrcIdList; // Список локальных идентификаторов записей подготовки расхода, которым соответствует данное отношение
	TSCollection <VetisRouteSectionR13nRules> R13RulesList;
};

class _VetisRegionRoutArray : public TSCollection <_VetisRegionRouteEntry> {
public:
	void   Add(const S_GUID & rRegFrom, const S_GUID & rRegTo, const S_GUID & rLocGuid_To, const S_GUID & rSubp, PPID srcRecID)
	{
		if(!!rRegFrom && !!rRegTo) {
			uint   found_pos = 0;
			for(uint i = 0; !found_pos && i < getCount(); i++) {
				const _VetisRegionRouteEntry * p_entry = at(i);
				assert(p_entry);
				if(p_entry) {
					// @v11.2.8 Переводим идентификацию региональных маршрутов на склад-получатель вместо регион-получатель
					if(p_entry->RegionGuid_From == rRegFrom) {
						if(p_entry->LocGuid_To == rLocGuid_To)
							found_pos = i+1;
						/*else if(p_entry->RegionGuid_To == rRegTo)
							found_pos = i+1;*/
					}
				}
			}
			if(!found_pos) {
				_VetisRegionRouteEntry * p_new_entry = CreateNewItem();
				p_new_entry->RegionGuid_From = rRegFrom;
				p_new_entry->RegionGuid_To = rRegTo;
				p_new_entry->LocGuid_To = rLocGuid_To; // @v11.2.8
				if(!!rSubp)
					p_new_entry->SubProductGuidList.insert(&rSubp);
				if(srcRecID)
					p_new_entry->SrcIdList.add(srcRecID);
			}
			else {
				assert(found_pos <= getCount());
				_VetisRegionRouteEntry * p_entry = at(found_pos-1);
				assert(p_entry->RegionGuid_From == rRegFrom);
				// @v11.2.8 assert(p_entry->RegionGuid_To == rRegTo);
				assert(p_entry->LocGuid_To == rLocGuid_To); // @v11.2.9
				if(!!rSubp && !p_entry->SubProductGuidList.lsearch(&rSubp, 0, PTR_CMPFUNC(S_GUID)))
					p_entry->SubProductGuidList.insert(&rSubp);
				if(srcRecID)
					p_entry->SrcIdList.add(srcRecID);
			}
		}
	}
};

int PPViewVetisDocument::ProcessOutcoming(PPID entityID__)
{
	int    ok = -1;
	int    use_alg2 = 0; // Если !0 то использовать новый алгоритм отправки запросов
	{
		PPIniFile ini_file;
		int    cfg_val = 0;
		if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_VETIS_OUTGOINGALG2, &cfg_val) > 0) {
			if(cfg_val == 1)
				use_alg2 = 1;
			else if(cfg_val == 100)
				use_alg2 = 100;
		}
	}
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	VetisDocumentViewItem vi;
	PPIDArray entity_id_list;
	PPIDArray manuf_id_list; // Список документов выхода из производства
	LAssocArray manuf_assoc_list;
	PPLogger logger;
	SString fmt_buf, msg_buf;
	SString addendum_msg_buf;
	SString wait_msg;
	SString temp_buf;
	VetisApplicationBlock _regionalization_ent_reply;
	_VetisRegionRoutArray regroute_list; // Список уникальных ассоциация откуда->куда. Откуда - предприятие-отправитель (чаще всего, главная организация).
	PPVetisInterface ifc(&logger);
	PPVetisInterface::Param param(0, Filt.LocID__, 0);
	PPVetisInterface::OutcomingList work_list;
	VetisApplicationBlock pi_reply;
	BillTbl::Rec link_bill_rec;
	bool is_locked = false;
	THROW(PPVetisInterface::SetupParam(param));
	THROW(ifc.Init(param));
	PPWaitStart();
	THROW(PPVetisInterface::LockProcess(PPVETISICFUNC_PROCESSOUTCOMING, false)); // @v11.3.2
	is_locked = true;
	for(InitIteration(); NextIteration(&vi) > 0;) {
		if(vi.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
			if(vi.Flags & VetisVetDocument::fInSendingQueue) { // @v11.3.4
				PPLoadText(PPTXT_VETIS_OUTCERTREJALREADYINQ, temp_buf);
				logger.Log(temp_buf);
			}
			else if(vi.Flags & VetisVetDocument::fManufExpense) {
				if(vi.DepDocEntityID)
					manuf_assoc_list.Add(vi.DepDocEntityID, vi.EntityID);
			}
			else {
				if(vi.LinkBillID && p_bobj->Search(vi.LinkBillID, &link_bill_rec) > 0) { // @v11.2.7
					entity_id_list.add(vi.EntityID);
					if(vi.Flags & VetisVetDocument::fManufIncome)
						manuf_id_list.add(vi.EntityID);
					else {
						VetisEntityCore::Entity _ent;
						S_GUID region_guid_from;
						S_GUID region_guid_to;
						S_GUID subp_guid;
						S_GUID loc_guid_to;
						long   region_id_from = 0;
						long   region_id_to = 0;
						if(p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, vi.LinkToDlvrLocID, PPTAG_LOC_VETIS_GUID, loc_guid_to) > 0) { // @v11.2.8
							if(vi.LinkFromDlvrLocID && p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, vi.LinkFromDlvrLocID, PPTAG_LOC_VETIS_REGIONGUID, region_guid_from) > 0 &&
								vi.LinkToDlvrLocID && p_ref->Ot.GetTagGuid(PPOBJ_LOCATION, vi.LinkToDlvrLocID, PPTAG_LOC_VETIS_REGIONGUID, region_guid_to) > 0) {
								if(vi.ProductItemID && ifc.PeC.GetEntity(vi.ProductItemID, _ent) > 0) {
									pi_reply.Clear();
									if(ifc.GetEntityQuery2(PPVetisInterface::qtProductItemByGuid, VGuidToStr(_ent.Guid, temp_buf), pi_reply) > 0) {
										if(pi_reply.ProductItemList.getCount()) {
											S_GUID subp_guid = pi_reply.ProductItemList.at(0)->SubProduct.Guid;
											regroute_list.Add(region_guid_from, region_guid_to, loc_guid_to, subp_guid, vi.EntityID);
										}
									}
								}
								// @v11.2.5 {
								else if(vi.SubProductID && ifc.PeC.GetEntity(vi.SubProductID, _ent) > 0) {
									regroute_list.Add(region_guid_from, region_guid_to, loc_guid_to, _ent.Guid, vi.EntityID);
								}
								// } @v11.2.5 
							}
						}
					}
				}
				else {
					PPLoadText(PPTXT_VETIS_OUTCERTREJNOLINKBILL, temp_buf);
					logger.Log(temp_buf);
				}
			}
		}
	}
	if(entity_id_list.getCount()) {
		PPObjBill * p_bobj = BillObj;
		Transfer * trfr = p_bobj->trfr;
		entity_id_list.sortAndUndup();
		TSVector <VetisEntityCore::UnresolvedEntity> ure_list;
		PPLoadText(PPTXT_VETISNOUTGDOCSFOUND, fmt_buf);
		msg_buf.Printf(fmt_buf, temp_buf.Z().Cat(entity_id_list.getCount()).cptr());
		logger.Log(msg_buf);
		{
			bool debug_mark = false;
			PPIDArray src_loc_list;
			PPIDArray dest_loc_list;
			VetisEntityCore::Entity _ent;
			VetisEnterprise _ent_ent;
			if(regroute_list.getCount()) {
				for(uint ridx = 0; ridx < regroute_list.getCount(); ridx++) {
					_VetisRegionRouteEntry * p_rrl_item = regroute_list.at(ridx);
					if(p_rrl_item) {
						TSCollection <VetisShipmentRoutePoint> srplist;
						{
							VetisShipmentRoutePoint * p_new_sp = srplist.CreateNewItem();
							p_new_sp->SqnId.Cat(1L);
							p_new_sp->P_Location = new VetisLocation;
							p_new_sp->P_Location->Address.Country.Guid.FromStr(P_VetisGuid_Country_Ru);
							p_new_sp->P_Location->Address.Region.Guid = p_rrl_item->RegionGuid_From;
						}
						{
							VetisShipmentRoutePoint * p_new_sp = srplist.CreateNewItem();
							p_new_sp->SqnId.Cat(2L);
							p_new_sp->P_Location = new VetisLocation;
							p_new_sp->P_Location->Address.Country.Guid.FromStr(P_VetisGuid_Country_Ru);
							p_new_sp->P_Location->Address.Region.Guid = p_rrl_item->RegionGuid_To;
							p_new_sp->P_Enterprise = new VetisEnterprise;
							p_new_sp->P_Enterprise->Guid = p_rrl_item->LocGuid_To;
						}
						if(ifc.CheckShipmentRegionalizationOperation(p_rrl_item->SubProductGuidList, srplist, _regionalization_ent_reply) > 0) {
							TSCollection_Copy(p_rrl_item->R13RulesList, _regionalization_ent_reply.R13RulesList);
						}
					}
				}
			}
		}
		PPLoadText(PPTXT_VETIS_OUTCERTSENDING, wait_msg);
		for(uint i = 0; i < entity_id_list.getCount(); i++) {
			const  PPID entity_id = entity_id_list.get(i);
			VetisDocumentTbl::Rec vd_rec;
			if(EC.SearchDocument(entity_id, &vd_rec) > 0 && vd_rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
				VetisApplicationBlock reply;
				assert(!(vd_rec.Flags & VetisVetDocument::fManufExpense));
				if(!(vd_rec.Flags & VetisVetDocument::fManufExpense)) {
					if(vd_rec.Flags & VetisVetDocument::fDiscrepancy) {
						int cr = ifc.ResolveDiscrepancy(entity_id, &ure_list, reply);
						if(cr > 0)
							ok = 1;
						else if(!cr)
							logger.LogLastError();
					}
					else if(vd_rec.Flags & VetisVetDocument::fManufIncome) {
						assert(manuf_id_list.lsearch(entity_id));
						LongArray expense_list;
						manuf_assoc_list.GetListByKey(entity_id, expense_list);
						int cr = ifc.RegisterProduction(entity_id, expense_list, &ure_list, reply);
						if(cr > 0)
							ok = 1;
						else if(!cr)
							logger.LogLastError();
					}
					else {
						// @v11.0.11 {
						const TSCollection <VetisRouteSectionR13nRules> * p_region_rules = 0;
						{
							for(uint ridx = 0; !p_region_rules && ridx < regroute_list.getCount(); ridx++) {
								const _VetisRegionRouteEntry * p_rrl_item = regroute_list.at(ridx);
								if(p_rrl_item && p_rrl_item->SrcIdList.lsearch(entity_id) && p_rrl_item->R13RulesList.getCount())
									p_region_rules = &p_rrl_item->R13RulesList;
							}
						}
						// } @v11.0.11
						// @v11.3.4 {
						const int sodiqfr = EC.SetOutgoingDocInQueueFlag(entity_id, 1); 
						THROW(sodiqfr);
						if(sodiqfr < 0) {
							PPLoadText(PPTXT_VETIS_OUTCERTREJALREADYINQ, temp_buf);
							logger.Log(temp_buf);
						}
						else { // } @v11.3.4 
							if(oneof2(use_alg2, 1, 100)) {
								if(!ifc.InitOutgoingEntry(entity_id, p_region_rules, work_list))
									logger.LogLastError();
							}
							else {
								{
									PPLoadText(PPTXT_VETISOUTGSENDING, fmt_buf);
									TransferTbl::Rec trfr_rec;
									addendum_msg_buf.Z();
									if(vd_rec.LinkBillID && p_bobj->Fetch(vd_rec.LinkBillID, &link_bill_rec) > 0) {
										PPObjBill::MakeCodeString(&link_bill_rec, PPObjBill::mcsAddObjName|PPObjBill::mcsAddObjName, temp_buf);
										addendum_msg_buf.Cat(temp_buf).Space().CatChar('#').Cat(vd_rec.LinkBillRow);
										if(vd_rec.LinkBillRow && trfr->SearchByBill(vd_rec.LinkBillID, 0, vd_rec.LinkBillRow, &trfr_rec) > 0 && trfr_rec.LotID) {
											addendum_msg_buf.Space().Cat(fabs(trfr_rec.Quantity), MKSFMTD(0, 3, NMBF_NOTRAILZ));
											GetGoodsName(trfr_rec.GoodsID, temp_buf);
											addendum_msg_buf.Space().Cat(temp_buf);
										}
									}
									else
										addendum_msg_buf.CatEq("BillID", vd_rec.LinkBillID).Space().CatChar('#').Cat(vd_rec.LinkBillRow);
									msg_buf.Printf(fmt_buf, addendum_msg_buf.cptr());
									logger.Log(msg_buf);
								}
								int cr = ifc.PrepareOutgoingConsignment(entity_id, p_region_rules, &ure_list, reply);
								if(cr > 0)
									ok = 1;
								else if(!cr)
									logger.LogLastError();
								PPWaitPercent(i+1, entity_id_list.getCount(), wait_msg);
							}
						}
					}
				}
			}
			else {
				msg_buf.Printf(PPLoadTextS(PPTXT_VETISOUTGDOCNFOUND, fmt_buf), temp_buf.Z().CatChar('#').Cat(entity_id).cptr());
				logger.Log(msg_buf);
			}
		}
		if(work_list.getCount()) {
			VetisApplicationBlock reply;
			SString wl_log_buf;
			work_list.DebugOutput(wl_log_buf.Z().Cat("Phase #1").CR());
			PPLogMessage(PPFILNAM_VETISOUTQUEUE_LOG, wl_log_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_USER);
			if(use_alg2 == 1) {
				for(uint qn = 1; qn <= work_list.GetLastQueue(); qn++) {
					PPLoadText(PPTXT_VETIS_OUTCERTSENDING, wait_msg);
					{
						for(uint wlidx = 0; wlidx < work_list.getCount(); wlidx++) {
							PPVetisInterface::OutcomingEntry & r_entry = work_list.at(wlidx);
							if(r_entry.QueueN == qn && r_entry.AppId.IsZero()) {
								if(!ifc.PrepareOutgoingConsignment2(r_entry, &ure_list, reply))
									logger.LogLastError();
							}
							PPWaitPercent(wlidx+1, work_list.getCount(), (temp_buf = wait_msg).Space().Cat(qn).Slash().Cat(work_list.GetLastQueue()));
						}
						work_list.DebugOutput(wl_log_buf.Z().Cat("Phase #2").CR());
						PPLogMessage(PPFILNAM_VETISOUTQUEUE_LOG, wl_log_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_USER);
					}
					long   phs3iter_no = 0;
					const  uint phone_iter_max = 5; // Макс количество холостых циклов (для предотвращения зацикливания)
					uint   phony_iter_count = 0; // Количество холостых циклов (для предотвращения зацикливания)
					uint   wlc = 0;
					uint   wlc_next = 0; // Количество оставшихся без ответа документов в конце цикла (для предотвращения зацикливания)
					PPLoadText(PPTXT_VETIS_OUTCERTREPLGETTING, wait_msg);
					for(wlc = work_list.GetReplyCandidateCount(qn); wlc > 0 && phony_iter_count < phone_iter_max; wlc = wlc_next) {
						phs3iter_no++;
						for(uint wlidx = 0; wlidx < work_list.getCount(); wlidx++) {
							PPVetisInterface::OutcomingEntry & r_entry = work_list.at(wlidx);
							//assert(!r_entry.AppId.IsZero() || r_entry.State == r_entry.stReply);
							if(r_entry.QueueN == qn && !r_entry.AppId.IsZero()) {
								SDelay(400 / wlc);
								ifc.QueryOutgoingConsignmentResult(r_entry, &ure_list, reply);
							}
							(temp_buf = wait_msg).Space().Cat(wlc).Slash().Cat(work_list.getCount());
							PPWaitPercent(wlidx+1, work_list.getCount(), temp_buf);
						}
						work_list.DebugOutput(wl_log_buf.Z().Cat("Phase #3").Space().CatChar('(').Cat(phs3iter_no).CatChar(')').CR());
						PPLogMessage(PPFILNAM_VETISOUTQUEUE_LOG, wl_log_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_USER);
						wlc_next = work_list.GetReplyCandidateCount(qn);
						if(wlc_next >= wlc) // должно быть ==, но из-за моей паранойи стоит >=
							phony_iter_count++;
						else
							phony_iter_count = 0;
					}
					if(wlc > 0 && phony_iter_count) {
						logger.Log(PPLoadTextS(PPTXT_VETIS_OUTCERTRGCYCLEEXIT, msg_buf));
					}
				}
			}
		}
		if(ok > 0)
			THROW(ifc.ProcessUnresolvedEntityList(ure_list));
	}
	else {
		PPLoadText(PPTXT_VETISNOOUTGDOCS, fmt_buf);
		msg_buf = fmt_buf;
		logger.Log(msg_buf);
	}
	CATCHZOKPPERR
	// @v11.3.2 {
	if(is_locked)
		PPVetisInterface::LockProcess(PPVETISICFUNC_PROCESSOUTCOMING, true); 
	// } @v11.3.2 
	logger.Save(PPFILNAM_VETISINFO_LOG, 0);
	PPWaitStop();
	return ok;
}

int PPViewVetisDocument::RerequestDocument(PPID entityID)
{
	int    ok = -1;
	VetisVetDocument item;
	if(EC.Get(entityID, item) > 0) {
		if(!item.Uuid.IsZero()) {
			PPVetisInterface::Param param(0, Filt.LocID__, 0);
			THROW(PPVetisInterface::SetupParam(param));
			{
				PPLogger logger;
				VetisApplicationBlock reply;
				PPVetisInterface ifc(&logger);
				THROW(ifc.Init(param));
				//int r = ifc.WriteOffIncomeCert(entityID, &ure_list, reply);
				{
					VetisApplicationBlock org_doc_reply;
					if(ifc.GetVetDocumentByUuid(item.Uuid, org_doc_reply) > 0) {
						ok = 1;
					}
				}
				//THROW(r);
				//if(r > 0) do_update = 1;
				logger.Save(PPFILNAM_VETISINFO_LOG, 0);
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewVetisDocument::Helper_ProcessIncoming(const S_GUID & rVetDocUuid, const void * pIfcParam, PPVetisInterface & rIfc, TSVector <VetisEntityCore::UnresolvedEntity> * pUreList)
{
	int    ok = -1;
	assert(pIfcParam);
	VetisApplicationBlock reply;
	rIfc.ProcessIncomingConsignment(rVetDocUuid, reply);
	if(oneof2(reply.ApplicationStatus, reply.appstAccepted, reply.appstCompleted)) {
		PPTransaction tra(1);
		THROW(tra);
		for(uint stkidx = 0; stkidx < reply.VetStockList.getCount(); stkidx++) {
			const VetisStockEntry * p_item = reply.VetStockList.at(stkidx);
			if(p_item) {
				PPID   pi_id = 0;
				const PPVetisInterface::Param * p_ifc_param = static_cast<const PPVetisInterface::Param *>(pIfcParam);
				THROW(rIfc.PeC.Put(&pi_id, p_ifc_param->IssuerUUID, p_ifc_param->EntUUID, *p_item, pUreList, 0));
				ok = 1;
			}
		}
		for(uint docidx = 0; docidx < reply.VetDocList.getCount(); docidx++) {
			const VetisVetDocument * p_item = reply.VetDocList.at(docidx);
			if(p_item) {
				PPID   pi_id = 0;
				THROW(rIfc.PeC.Put(&pi_id, *p_item, 0, pUreList, 0));
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewVetisDocument::ProcessIncoming(PPID entityID)
{
	int    ok = -1;
	int    do_update = 0;
	uint   v = 0;
	/*
		-- 0 @vetisprcssincoming_redeemsingle          "Погасить выбранный"
		-- 1 @vetisprcssincoming_redeemmatched         "Погасить все сопоставленные с документами"
		-- 2 @vetisprcssincoming_redeemall             "Безусловно погасить всю выборку"
		-- 3 @vetisprcssincoming_wroffsingle           "Списать с остатка выбранный документ"
		-- 4 @vetisprcssincoming_wroffall              "Списать с остатка всю выборку"
	*/
	if(SelectorDialog(DLG_SELVDUTLZ, STDCTL_SELECTOR_WHAT, &v) > 0) {
		TSVector <VetisEntityCore::UnresolvedEntity> ure_list;
		if(v == 0) { // погасить выбранный
			VetisVetDocument item;
			if(EC.Get(entityID, item) > 0) {
				if(!item.Uuid.IsZero()) {
					PPVetisInterface::Param param(0, Filt.LocID__, 0);
					THROW(PPVetisInterface::SetupParam(param));
					{
						PPLogger logger;
						PPVetisInterface ifc(&logger);
						THROW(ifc.Init(param));
						int r = Helper_ProcessIncoming(item.Uuid, &param, ifc, &ure_list);
						THROW(r);
						if(r > 0)
							do_update = 1;
						logger.Save(PPFILNAM_VETISINFO_LOG, 0);
					}
				}
			}
		}
		else if(v == 1) { // Погасить все сопоставленные с документами
			PPVetisInterface::Param param(0, Filt.LocID__, 0);
			THROW(PPVetisInterface::SetupParam(param));
			{
				PPLogger logger;
				VetisDocumentViewItem vi;
				PPVetisInterface ifc(&logger);
				THROW(ifc.Init(param));
				PPWaitStart();
				for(InitIteration(); NextIteration(&vi) > 0;) {
					BillTbl::Rec bill_rec;
					if(vi.VetisDocStatus == vetisdocstCONFIRMED && vi.Flags & VetisVetDocument::fToMainOrg &&
						vi.LinkBillID && BillObj->Search(vi.LinkBillID, &bill_rec) > 0) {
						VetisVetDocument item;
						if(EC.Get(vi.EntityID, item) > 0 && !!item.Uuid) {
							int r = Helper_ProcessIncoming(item.Uuid, &param, ifc, &ure_list);
							if(!r)
								logger.LogLastError();
							else if(r > 0)
								do_update = 1;
						}
					}
					PPWaitPercent(Counter);
				}
				logger.Save(PPFILNAM_VETISINFO_LOG, 0);
				PPWaitStop();
			}
		}
		else if(v == 2) { // Безусловно погасить всю выборку
			PPVetisInterface::Param param(0, Filt.LocID__, 0);
			THROW(PPVetisInterface::SetupParam(param));
			{
				PPLogger logger;
				VetisDocumentViewItem vi;
				PPVetisInterface ifc(&logger);
				THROW(ifc.Init(param));
				PPWaitStart();
				for(InitIteration(); NextIteration(&vi) > 0;) {
					if(vi.VetisDocStatus == vetisdocstCONFIRMED && vi.Flags & VetisVetDocument::fToMainOrg) {
						VetisVetDocument item;
						if(EC.Get(vi.EntityID, item) > 0 && !!item.Uuid) {
							int r = Helper_ProcessIncoming(item.Uuid, &param, ifc, &ure_list);
							if(!r)
								logger.LogLastError();
							if(r > 0)
								do_update = 1;
						}
					}
					PPWaitPercent(Counter);
				}
				logger.Save(PPFILNAM_VETISINFO_LOG, 0);
				PPWaitStop();
			}
		}
		else if(v == 3) { // Списать выбранный документ // @construction
			VetisVetDocument item;
			if(EC.Get(entityID, item) > 0) {
				if(!item.Uuid.IsZero()) {
					PPVetisInterface::Param param(0, Filt.LocID__, 0);
					THROW(PPVetisInterface::SetupParam(param));
					{
						PPLogger logger;
						VetisApplicationBlock reply;
						PPVetisInterface ifc(&logger);
						THROW(ifc.Init(param));
						int r = ifc.WriteOffIncomeCert(entityID, &ure_list, reply);
						THROW(r);
						if(r > 0)
							do_update = 1;
						logger.Save(PPFILNAM_VETISINFO_LOG, 0);
					}
				}
			}
		}
		else if(v == 4) { // Списать всю выборку документов
			if(CONFIRMCRIT(PPCFM_WROFFALLVETISCERTS)) {
				PPVetisInterface::Param param(0, Filt.LocID__, 0);
				THROW(PPVetisInterface::SetupParam(param));
				{
					PPLogger logger;
					VetisDocumentViewItem vi;
					VetisApplicationBlock reply;
					PPVetisInterface ifc(&logger);
					THROW(ifc.Init(param));
					PPWaitStart();
					for(InitIteration(); NextIteration(&vi) > 0;) {
						if(vi.VetisDocStatus == vetisdocstUTILIZED && vi.Flags & VetisVetDocument::fToMainOrg) {
							VetisVetDocument item;
							if(EC.Get(vi.EntityID, item) > 0 && !!item.Uuid) {
								int r = ifc.WriteOffIncomeCert(vi.EntityID, &ure_list, reply);
								if(!r)
									logger.LogLastError();
								else if(r > 0)
									do_update = 1;
							}
						}
						PPWaitPercent(Counter);
					}
					logger.Save(PPFILNAM_VETISINFO_LOG, 0);
					PPWaitStop();
				}
			}
		}
	}
	CATCHZOKPPERR
	if(do_update)
		ok = 1;
	return ok;
}

static int VetisDocumentUpdateSelectorDialog(uint * pAction, DateRange & rPeriod)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_SELVDOCUPD);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlUInt16(STDCTL_SELECTOR_WHAT, *pAction);
		SetPeriodInput(dlg, CTL_SELVDOCUPD_PERIOD, rPeriod);
		if(ExecView(dlg) == cmOK) {
			GetPeriodInput(dlg, CTL_SELVDOCUPD_PERIOD, &rPeriod);
			*pAction = dlg->getCtrlUInt16(STDCTL_SELECTOR_WHAT);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewVetisDocument::LoadDocuments()
{
	int    ok = -1;
	uint   v = 0;
	DateRange period;
	STimeChunk tc;
	PPLogger logger;
	PPVetisInterface::Param param(0, Filt.LocID__, 0);
	THROW(PPVetisInterface::SetupParam(param));
	period.Z();
	if(checkdate(Filt.Period.low)) {
		period.low = Filt.Period.low;
		period.upp = ValidDateOr(Filt.Period.upp, Filt.Period.low);
	}
	else if(checkdate(Filt.Period.upp))
		period.SetDate(Filt.Period.upp);
	else {
		const LDATE cd = getcurdate_();
		period.Set(plusdate(cd, -1), cd);
	}
	if(VetisDocumentUpdateSelectorDialog(&v, period) > 0) {
		SString fmt_buf, msg_buf;
		SString temp_buf;
		PPVetisInterface ifc(&logger);
		VetisApplicationBlock reply;
		TSVector <VetisEntityCore::UnresolvedEntity> ure_list;
		period.Actualize(ZERODATE);
		const int is_init_period_zero = _SetupTimeChunkByDateRange(period, tc);
		THROW(ifc.Init(param));
		PPWaitStart();
		if(v == 0) { // изменения
			PPLoadText(PPTXT_VETISGETTINGUPD, fmt_buf);
			temp_buf.Z().Cat(tc.Start, DATF_DMY, TIMF_HM).Cat("..").Cat(tc.Finish, DATF_DMY, TIMF_HM);
			msg_buf.Printf(fmt_buf, temp_buf.cptr());
			PPWaitMsg(msg_buf);
			const uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetVetDocumentChangesList(tc, req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.VetDocList.getCount(); i++) {
					const VetisVetDocument * p_item = reply.VetDocList.at(i);
					if(p_item) {
						PPID   pi_id = 0;
						THROW(ifc.PeC.Put(&pi_id, *p_item, 0, &ure_list, 0));
						ok = 1;
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total, msg_buf);
				if(reply.VetDocList.getCount() < req_count)
					break;
				else
					req_offs += reply.VetDocList.getCount();
			}
		}
		else if(v == 1) { // полная загрузка
			PPLoadText(PPTXT_VETISGETTINGALLDOCS, msg_buf);
			PPWaitMsg(msg_buf);
			const uint req_count = 1000;
			for(uint req_offs = 0; ifc.GetVetDocumentList(tc, req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.VetDocList.getCount(); i++) {
					const VetisVetDocument * p_item = reply.VetDocList.at(i);
					if(p_item) {
						PPID   pi_id = 0;
						THROW(ifc.PeC.Put(&pi_id, *p_item, 0, &ure_list, 0));
						ok = 1;
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total, msg_buf);
				if(reply.VetDocList.getCount() < req_count)
					break;
				else
					req_offs += reply.VetDocList.getCount();
			}
		}
		else if(v == 2) { // текущие остатки
			const uint req_count = 1000;
			if(is_init_period_zero) {
				PPLoadText(PPTXT_VETISGETTINGSTOCK, msg_buf);
			}
			else {
				PPLoadText(PPTXT_VETISGETTINGSTOCKUPD, fmt_buf);
				temp_buf.Z().Cat(tc.Start, DATF_DMY, TIMF_HM).Cat("..").Cat(tc.Finish, DATF_DMY, TIMF_HM);
				msg_buf.Printf(fmt_buf, temp_buf.cptr());
			}
			PPWaitMsg(msg_buf);
			for(uint req_offs = 0; is_init_period_zero ? ifc.GetStockEntryList(req_offs, req_count, reply) : ifc.GetStockEntryChangesList(tc, req_offs, req_count, reply);) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < reply.VetStockList.getCount(); i++) {
					const VetisStockEntry * p_item = reply.VetStockList.at(i);
					if(p_item) {
						PPID   pi_id = 0;
						THROW(ifc.PeC.Put(&pi_id, param.IssuerUUID, param.EntUUID, *p_item, &ure_list, 0));
						ok = 1;
					}
				}
				THROW(tra.Commit());
				PPWaitPercent(reply.ListResult.Count+reply.ListResult.Offset, reply.ListResult.Total, msg_buf);
				if(reply.VetStockList.getCount() < req_count)
					break;
				else
					req_offs += reply.VetStockList.getCount();
			}
		}
		else if(v == 3) { // актуализация текущих данных
#if 0 // @v10.4.1 {
			// @v10.4.1 {
			VetisDocumentViewItem view_item;
			LAssocArray doc_consignor_be_assoc_list;
			LAssocArray doc_consignee_be_assoc_list;
			VetisVetDocument doc_item;
			for(InitIteration(); NextIteration(&view_item) > 0;) {
				VetisEntityCore::Entity ent;
				if(EC.Get(view_item.EntityID, doc_item) > 0) {
					VetisCertifiedConsignment & r_crtc = doc_item.CertifiedConsignment;
					if(!r_crtc.Consignor.BusinessEntity.EntityID) {
						if(!r_crtc.Consignor.BusinessEntity.Guid.IsZero()) {
							if(ifc.PeC.GetEntityByGuid(r_crtc.Consignor.BusinessEntity.Guid, ent) > 0) {
								if(ent.Kind == VetisEntityCore::kBusinessEntity) {
									doc_consignor_be_assoc_list.Add(view_item.EntityID, ent.ID);
								}
							}
						}
					}
					if(!r_crtc.Consignee.BusinessEntity.EntityID) {
						if(!r_crtc.Consignee.BusinessEntity.Guid.IsZero()) {
							if(ifc.PeC.GetEntityByGuid(r_crtc.Consignee.BusinessEntity.Guid, ent) > 0) {
								if(ent.Kind == VetisEntityCore::kBusinessEntity) {
									doc_consignee_be_assoc_list.Add(view_item.EntityID, ent.ID);
								}
							}
						}
					}
					//if(r_crtc.Consignee.BusinessEntity)
					//EditVetisVetDocument(item);
				}
			}
			if(doc_consignor_be_assoc_list.getCount() || doc_consignee_be_assoc_list.getCount()) {
				PPIDArray doc_to_upd_list;
				{
					for(uint cridx = 0; cridx < doc_consignor_be_assoc_list.getCount(); cridx++) {
						doc_to_upd_list.add(doc_consignor_be_assoc_list.at(cridx).Key);
					}
					for(uint ceidx = 0; ceidx < doc_consignee_be_assoc_list.getCount(); ceidx++) {
						doc_to_upd_list.add(doc_consignee_be_assoc_list.at(ceidx).Key);
					}
					doc_to_upd_list.sortAndUndup();
				}
				PPTransaction tra(1);
				THROW(tra);
				for(uint docidx = 0; docidx < doc_to_upd_list.getCount(); docidx++) {
					PPID   doc_id = doc_to_upd_list.get(docidx);
					if(EC.Get(view_item.EntityID, doc_item) > 0) {
						PPID   consignor_be_id = 0;
						PPID   consignee_be_id = 0;
						const int rr = doc_consignor_be_assoc_list.Search(doc_id, &consignor_be_id, 0);
						const int er = doc_consignee_be_assoc_list.Search(doc_id, &consignee_be_id, 0);
						assert(rr > 0 || consignor_be_id == 0);
						assert(rr == 0 || consignor_be_id > 0);
						assert(er > 0 || consignee_be_id == 0);
						assert(er == 0 || consignee_be_id > 0);
						if(consignor_be_id && !doc_item.CertifiedConsignment.Consignor.BusinessEntity.EntityID)
							doc_item.CertifiedConsignment.Consignor.BusinessEntity.EntityID = consignor_be_id;
						if(consignee_be_id && !doc_item.CertifiedConsignment.Consignee.BusinessEntity.EntityID)
							doc_item.CertifiedConsignment.Consignee.BusinessEntity.EntityID = consignee_be_id;
						PPID   local_entity_id = 0;
						THROW(EC.Put(&local_entity_id, doc_item, /*pUreList*/0, 0));
					}
				}
				THROW(tra.Commit());
			}
			// } @v10.4.1
#endif // } 0 @v10.4.1
		}
		if(ok > 0)
			THROW(ifc.ProcessUnresolvedEntityList(ure_list));
	}
	CATCHZOKPPERR
	logger.Save(PPFILNAM_VETISINFO_LOG, 0);
	PPWaitStop();
	return ok;
}

struct PPMatchPersonBlock {
	PPMatchPersonBlock() : PersonKindID(0), PersonID(0), DlvrLocID(0)
	{
	}
	SString SrcInfo;
	SString SrcAddr;
	PPID   PersonKindID;
	PPID   PersonID;
	PPID   DlvrLocID;
};

class MatchPersonDialog : public TDialog {
	DECL_DIALOG_DATA(PPMatchPersonBlock);
public:
	MatchPersonDialog() : TDialog(DLG_MTCHPSN)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		{
			SString src_info(Data.SrcInfo);
			if(Data.SrcAddr.NotEmpty())
				src_info.CRB().Cat(Data.SrcAddr);
			setCtrlString(CTL_MTCHPSN_SRCINFO, src_info);
		}
		//SetupPPObjCombo(this, CTLSEL_MTCHPSN_PSN, PPOBJ_PERSON, Data.PersonID, OLW_CANINSERT, (void *)Data.PersonKindID);
		SetupPersonCombo(this, CTLSEL_MTCHPSN_PSN, Data.PersonID, OLW_CANINSERT, Data.PersonKindID, 0/*disableIfZeroPersonKind*/);
		SetupPPObjCombo(this, CTLSEL_MTCHPSN_LOC, PPOBJ_LOCATION, Data.DlvrLocID, OLW_CANINSERT, 0);
		selectCtrl(CTL_MTCHPSN_PSN);
		if(Data.PersonID)
			PsnObj.SetupDlvrLocCombo(this, CTLSEL_MTCHPSN_LOC, Data.PersonID, Data.DlvrLocID);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTLSEL_MTCHPSN_PSN, &Data.PersonID);
		getCtrlData(CTLSEL_MTCHPSN_LOC, &Data.DlvrLocID);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_MTCHPSN_PSN)) {
			const  PPID preserve_loc_id = Data.DlvrLocID;
			const  PPID preserve_psn_id = Data.PersonID;
			getCtrlData(CTLSEL_MTCHPSN_PSN, &Data.PersonID);
			if(preserve_psn_id != Data.PersonID) {
				if(Data.PersonID)
					PsnObj.SetupDlvrLocCombo(this, CTLSEL_MTCHPSN_LOC, Data.PersonID, Data.DlvrLocID);
			}
		}
		else
			return;
		clearEvent(event);
	}
	PPObjPerson PsnObj;
};

static int FASTCALL SetupSurveyPeriod(const VetisDocumentTbl::Rec & rRec, DateRange & rPeriod)
{
	int    ok = 1;
	PPAlbatrossConfig acfg;
	const  int delay_days = (DS.FetchAlbatrosConfig(&acfg) > 0 && acfg.Hdr.VetisCertDelay > 0) ? acfg.Hdr.VetisCertDelay : 3;
	if(checkdate(rRec.WayBillDate)) {
		rPeriod.Set(plusdate(rRec.WayBillDate, -delay_days), plusdate(rRec.WayBillDate, delay_days));
	}
	else if(checkdate(rRec.IssueDate)) {
		rPeriod.Set(plusdate(rRec.IssueDate, -delay_days-2), plusdate(rRec.IssueDate, delay_days+2));
	}
	else {
		rPeriod.SetDate(getcurdate_());
		ok = 0;
	}
	return ok;
}

int PPViewVetisDocument::ForceResolveObject(const VetisDocumentTbl::Rec & rRec, int objToMatch)
{
	int    ok = -1;
	if(oneof2(objToMatch, otmFrom, otmTo)) {
		Reference * p_ref = PPRef;
		const  int  side = (objToMatch == otmTo) ? 1 : 0;
		const  PPID enterprise_id = (side == 1) ? rRec.ToEnterpriseID : rRec.FromEnterpriseID;
		const  PPID bent_id = (side == 1) ? rRec.ToEntityID : rRec.FromEntityID;
		SStringU temp_buf_u;
		TSVector <VetisEntityCore::UnresolvedEntity> urel;
		if(enterprise_id) {
			VetisPersonTbl::Rec vp_rec;
			if(EC.SearchPerson(enterprise_id, &vp_rec) > 0) {
				VetisEntityCore::Entity ent_rec;
				if(EC.GetEntity(vp_rec.EntityID, ent_rec) > 0) {
					VetisEntityCore::UnresolvedEntity ure_entry;
					ure_entry.ID = vp_rec.EntityID;
					ure_entry.Kind = ent_rec.Kind;
					ure_entry.GuidRef = ent_rec.GuidRef;
					ure_entry.UuidRef = ent_rec.UuidRef;
					urel.insert(&ure_entry);
				}
			}
		}
		if(bent_id) {
			VetisPersonTbl::Rec vp_rec;
			if(EC.SearchPerson(bent_id, &vp_rec) > 0) {
				VetisEntityCore::Entity ent_rec;
				if(EC.GetEntity(vp_rec.EntityID, ent_rec) > 0) {
					VetisEntityCore::UnresolvedEntity ure_entry;
					ure_entry.ID = vp_rec.EntityID;
					ure_entry.Kind = ent_rec.Kind;
					ure_entry.GuidRef = ent_rec.GuidRef;
					ure_entry.UuidRef = ent_rec.UuidRef;
					urel.insert(&ure_entry);
				}
			}
		}
		if(urel.getCount()) {
			PPVetisInterface::Param param(0, Filt.LocID__, 0);
			if(PPVetisInterface::SetupParam(param)) {
				PPLogger logger;
				PPVetisInterface ifc(&logger);
				if(ifc.Init(param)) {
					VetisApplicationBlock reply;
					if(ifc.ProcessUnresolvedEntityList(urel) > 0)
						ok = 1;
				}
				logger.Save(PPFILNAM_VETISINFO_LOG, 0);
			}
		}
	}
	return ok;
}

int PPViewVetisDocument::MatchObject(const VetisDocumentTbl::Rec & rRec, int objToMatch)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	PPObjAccSheet acs_obj;
	SString temp_buf;
	PPIDArray locked_bill_list;
	if(oneof2(objToMatch, otmFrom, otmTo)) {
		const int  side = (objToMatch == otmTo) ? 1 : 0;
		PPMatchPersonBlock mb;
		const  PPID acs_id = (objToMatch == otmTo) ? GetSellAccSheet() : GetSupplAccSheet();
		PPAccSheet acs_rec;
		{
			Reference * p_ref = PPRef;
			const  PPID enterprise_id = (side == 1) ? rRec.ToEnterpriseID : rRec.FromEnterpriseID;
			const  PPID bent_id = (side == 1) ? rRec.ToEntityID : rRec.FromEntityID;
			SStringU temp_buf_u;
			if(!temp_buf_u.Len() && enterprise_id) {
				VetisPersonTbl::Rec vp_rec;
				if(EC.SearchPerson(enterprise_id, &vp_rec) > 0) {
					p_ref->UtrC.SearchU(TextRefIdent(PPOBJ_VETISENTITY, enterprise_id, PPTRPROP_RAWADDR), temp_buf_u);
					if(temp_buf_u.Len()) {
						temp_buf_u.CopyToUtf8(mb.SrcAddr, 0);
						mb.SrcAddr.Transf(CTRANSF_UTF8_TO_INNER);
					}
					temp_buf_u.Z();
					p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, enterprise_id, PPTRPROP_NAME), temp_buf_u);
				}
			}
			if(!temp_buf_u.Len() && bent_id) {
				p_ref->TrT.Search(TextRefIdent(PPOBJ_VETISENTITY, bent_id, PPTRPROP_NAME), temp_buf_u);
			}
			if(temp_buf_u.Len()) {
				temp_buf_u.CopyToUtf8(mb.SrcInfo, 0);
				mb.SrcInfo.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
		if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup)
			mb.PersonKindID = acs_rec.ObjGroup;
		else
			mb.PersonKindID = (objToMatch == otmTo) ? PPPRK_CLIENT : PPPRK_SUPPL;
		mb.PersonID = (objToMatch == otmTo) ? rRec.LinkToPsnID : rRec.LinkFromPsnID;
		mb.DlvrLocID = (objToMatch == otmTo) ? rRec.LinkToDlvrLocID : rRec.LinkFromDlvrLocID;
		int   r = PPDialogProcBody <MatchPersonDialog, PPMatchPersonBlock> (&mb);
		if(r > 0 && mb.PersonID) {
			THROW(ok = EC.MatchPersonInDocument(rRec.EntityID, side, mb.PersonID, mb.DlvrLocID, 1));
		}
	}
	else if(objToMatch == otmGoods) {
	}
	else if(objToMatch == otmLot) {
		if((rRec.LinkFromDlvrLocID || rRec.LinkFromPsnID) && (rRec.LinkToDlvrLocID || rRec.LinkToPsnID)) {
			PPObjArticle ar_obj;
			PPObjPerson psn_obj;
			int i_am_side = -1; // 0 - приход (главная организация TO), 1 - расход (главная организация FROM)
			if(rRec.Flags & VetisVetDocument::fToMainOrg)
				i_am_side = 0;
			else if(rRec.Flags & VetisVetDocument::fFromMainOrg)
				i_am_side = 1;
			if(i_am_side == 0) {
				PPID   acs_id = GetSupplAccSheet();
				PPID   ar_id = 0;
				PPID   loc_id = 0;
				const  PPID bill_id = rRec.LinkBillID;
				ReceiptTbl::Rec lot_rec;
				LAssocArray lot_to_rbb_list;
				const  long slp_flags = PPObjBill::SelectLotParam::fEnableZeroRest|PPObjBill::SelectLotParam::fShowBarcode|
					PPObjBill::SelectLotParam::fShowQtty|PPObjBill::SelectLotParam::fShowPhQtty|PPObjBill::SelectLotParam::fShowVetisTag;
				if(bill_id) {
					PPBillPacket bp;
					THROW(p_bobj->ExtractPacketWithFlags(bill_id, &bp, BPLD_LOCK) > 0);
					{
						locked_bill_list.add(bill_id);
						int    do_update_bpack = 0;
						ar_id = bp.Rec.Object;
						PPObjBill::SelectLotParam slp(0, bp.Rec.LocID, 0, slp_flags);
						for(uint i = 0; i < bp.GetTCount(); i++) {
							const PPTransferItem & r_ti = bp.ConstTI(i);
							if(r_ti.Flags & PPTFR_RECEIPT && r_ti.LotID) {
								lot_to_rbb_list.Add(r_ti.LotID, r_ti.RByBill);
								slp.AddendumLotList.add(r_ti.LotID);
								if(rRec.LinkBillRow > 0) {
									if(rRec.LinkBillRow == r_ti.RByBill)
										slp.RetLotID = r_ti.LotID;
								}
								else if(rRec.LinkGoodsID > 0) {
									if(!slp.RetLotID && rRec.LinkGoodsID == r_ti.GoodsID)
										slp.RetLotID = r_ti.LotID;
								}
							}
						}
						{
							// @v11.1.12 BillCore::GetCode(temp_buf = bp.Rec.Code);
							temp_buf = bp.Rec.Code; // @v11.1.12 
							(slp.Title = temp_buf).Space().Cat(bp.Rec.Dt, DATF_DMY).CatDiv('-', 1).Cat(rRec.Volume, MKSFMTD_030);
							if(VetisEntityCore::GetProductItemName(rRec.EntityID, rRec.ProductItemID, rRec.SubProductID, rRec.ProductID, temp_buf) > 0)
								slp.Title.CatDiv('-', 1).Cat(temp_buf);
						}
						if(slp.AddendumLotList.getCount() && p_bobj->SelectLot2(slp) > 0) {
							long   rbb = 0;
							if(lot_to_rbb_list.Search(slp.RetLotID, &rbb, 0) && rbb > 0) {
								uint tipos = 0;
								if(bp.SearchTI(rbb, &tipos)) {
									PPTransferItem & r_ti = bp.TI(tipos);
									SUniTime ut;
									LDATETIME dtm;
									LDATE expiry = ZERODATE;
									LDATETIME manuf_dtm = ZERODATETIME;
									ut.FromInt64(rRec.ExpiryFrom);
									if(!!ut && ut.Get(dtm))
										expiry = dtm.d;
									else {
										ut.FromInt64(rRec.ExpiryTo);
										if(!!ut && ut.Get(dtm))
											expiry = dtm.d;
									}
									if(checkdate(expiry) && expiry != r_ti.Expiry) {
										r_ti.Expiry = expiry;
										do_update_bpack = 1;
									}
									{
										PPObjTag tag_obj;
										PPObjectTag tag_rec;
										if(tag_obj.Fetch(PPTAG_LOT_MANUFTIME, &tag_rec) > 0) {
											ut.FromInt64(rRec.ManufDateFrom);
											if(!!ut && ut.Get(dtm))
												manuf_dtm = dtm;
											else {
												ut.FromInt64(rRec.ManufDateTo);
												if(!!ut && ut.Get(dtm))
													manuf_dtm = dtm;
											}
											if(checkdate(manuf_dtm.d) && checktime(manuf_dtm.t)) {
												const ObjTagItem * p_manuf_tm_tag = bp.LTagL.GetTag(tipos, PPTAG_LOT_MANUFTIME);
												LDATETIME org_manuf_dtm = ZERODATETIME;
												CALLPTRMEMB(p_manuf_tm_tag, GetTimestamp(&org_manuf_dtm));
												if(manuf_dtm != org_manuf_dtm) {
													ObjTagList dest_tag_list;
													ObjTagList * p_tag_list = bp.LTagL.Get(tipos);
													RVALUEPTR(dest_tag_list, p_tag_list);
													ObjTagItem tag_item;
													if(tag_item.SetTimestamp(PPTAG_LOT_MANUFTIME, manuf_dtm)) {
														dest_tag_list.PutItem(PPTAG_LOT_MANUFTIME, &tag_item);
														bp.LTagL.Set(tipos, &dest_tag_list);
														do_update_bpack = 1;
													}
												}
											}
										}
									}
									{
										PPTransaction tra(1);
										THROW(tra);
										if(do_update_bpack) {
											THROW(p_bobj->UpdatePacket(&bp, 0));
										}
										THROW(ok = EC.MatchDocument(rRec.EntityID, bill_id, rbb, 0/*fromBill*/, 0));
										THROW(tra.Commit());
									}
								}
							}
						}
						p_bobj->Unlock(bill_id);
						locked_bill_list.removeByID(bill_id);
					}
				}
				else {
					if(rRec.LinkFromPsnID)
						ar_obj.P_Tbl->PersonToArticle(rRec.LinkFromPsnID, acs_id, &ar_id);
					if(rRec.LinkFromDlvrLocID) {
						LocationTbl::Rec loc_rec;
						if(psn_obj.LocObj.Fetch(rRec.LinkFromDlvrLocID, &loc_rec) > 0) {
							loc_id = loc_rec.ID;
							if(!ar_id && loc_rec.OwnerID)
								ar_obj.P_Tbl->PersonToArticle(loc_rec.OwnerID, acs_id, &ar_id);
						}
					}
					if(ar_id) {
						LotFilt lot_filt;
						lot_filt.SupplID = ar_id;
						lot_filt.LocList.Add(Filt.LocID__);
						if(SetupSurveyPeriod(rRec, lot_filt.Period)) {
							lot_filt.Flags |= (LotFilt::fCheckOriginLotDate|LotFilt::fNoTempTable|LotFilt::fInitOrgLot);
							PPViewLot lot_view;
							if(lot_view.Init_(&lot_filt)) {
								LotViewItem lot_item;
								PPObjBill::SelectLotParam slp(0, 0, 0, slp_flags);
								PPIDArray lot_id_list;
								for(lot_view.InitIteration(); lot_view.NextIteration(&lot_item) > 0;) {
									lot_id_list.add(lot_item.OrgLotID);
								}
								lot_id_list.sortAndUndup();
								for(uint i = 0; i < lot_id_list.getCount(); i++) {
									const  PPID lot_id = lot_id_list.get(i);
									if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
										DateIter di;
										TransferTbl::Rec trfr_rec;
										if(p_bobj->trfr->EnumByLot(lot_id, &di, &trfr_rec) > 0 && trfr_rec.Flags & PPTFR_RECEIPT) {
											lot_to_rbb_list.Add(lot_id, trfr_rec.RByBill);
											slp.AddendumLotList.add(lot_id);
											if(rRec.LinkBillRow > 0) {
												if(rRec.LinkBillRow == trfr_rec.RByBill)
													slp.RetLotID = lot_id;
											}
											else if(rRec.LinkGoodsID > 0) {
												if(!slp.RetLotID && rRec.LinkGoodsID == lot_rec.GoodsID)
													slp.RetLotID = lot_id;
											}
										}
									}
								}
								{
									temp_buf = rRec.WayBillNumber[0] ? rRec.WayBillNumber : rRec.IssueNumber;
									(slp.Title = temp_buf).Space();
									if(checkdate(rRec.WayBillDate))
										slp.Title.Cat(rRec.WayBillDate, DATF_DMY);
									else if(checkdate(rRec.IssueDate))
										slp.Title.Cat(rRec.IssueDate, DATF_DMY);
									slp.Title.CatDiv('-', 1).Cat(rRec.Volume, MKSFMTD_030);
									if(VetisEntityCore::GetProductItemName(rRec.EntityID, rRec.ProductItemID, rRec.SubProductID, rRec.ProductID, temp_buf) > 0)
										slp.Title.CatDiv('-', 1).Cat(temp_buf);
								}
								if(slp.AddendumLotList.getCount() && p_bobj->SelectLot2(slp) > 0) {
									long   rbb = 0;
									if(lot_to_rbb_list.Search(slp.RetLotID, &rbb, 0) && rbb > 0) {
										if(p_bobj->trfr->Rcpt.Search(slp.RetLotID, &lot_rec) > 0 && !lot_rec.PrevLotID) {
											THROW(ok = EC.MatchDocument(rRec.EntityID, lot_rec.BillID, rbb, 0/*fromBill*/, 1));
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
	else if(objToMatch == otmBill) {
		if((rRec.LinkFromDlvrLocID || rRec.LinkFromPsnID) && (rRec.LinkToDlvrLocID || rRec.LinkToPsnID)) {
			PPObjArticle ar_obj;
			PPObjPerson psn_obj;
			int i_am_side = -1; // 0 - приход (главная организация TO), 1 - расход (главная организация FROM)
			if(rRec.Flags & VetisVetDocument::fToMainOrg)
				i_am_side = 0;
			else if(rRec.Flags & VetisVetDocument::fFromMainOrg)
				i_am_side = 1;
			if(i_am_side == 0) {
				const  PPID acs_id = GetSupplAccSheet();
				PPID   ar_id = 0;
				PPID   rcvr_ar_id = 0;
				PPID   loc_id = 0;
				PPID   bill_id = 0;
				int    select_intrrcpt = 0; // Выбирать из межскладских приходов
				if(rRec.LinkFromPsnID) {
					ar_obj.P_Tbl->PersonToArticle(rRec.LinkFromPsnID, acs_id, &ar_id);
				}
				if(rRec.LinkFromDlvrLocID) {
					LocationTbl::Rec loc_rec;
					if(psn_obj.LocObj.Fetch(rRec.LinkFromDlvrLocID, &loc_rec) > 0) {
						loc_id = loc_rec.ID;
						if(!ar_id) {
							if(loc_rec.Type == LOCTYP_WAREHOUSE) {
								select_intrrcpt = 1;
								if(rRec.LinkToDlvrLocID)
									rcvr_ar_id = PPObjLocation::WarehouseToObj(rRec.LinkToDlvrLocID);
								ar_id = PPObjLocation::WarehouseToObj(rRec.LinkFromDlvrLocID);
							}
							else if(loc_rec.OwnerID)
								ar_obj.P_Tbl->PersonToArticle(loc_rec.OwnerID, acs_id, &ar_id);
						}
					}
				}
				if(ar_id) {
					BillFilt bill_filt;
					{
						PPIDArray temp_id_list;
						DateRange sp;
						if(checkdate(rRec.WayBillDate)) {
							sp.Set(plusdate(rRec.WayBillDate, -10), plusdate(rRec.WayBillDate, +10));
						}
						else if(checkdate(rRec.IssueDate)) {
							sp.Set(plusdate(rRec.IssueDate, -10), plusdate(rRec.IssueDate, +10));
						}
						BillTbl::Rec bill_rec;
						class MatchCodeBlock {
						public:
							explicit MatchCodeBlock(const char * pWayBillNumber) : MaxMatchCodelen(0), MaxMatchID(0), WayBillNumber(pWayBillNumber)
							{
							}
							void ProcessBillCode(PPID billID, const SString & rCode)
							{
								const size_t way_bill_code_len = WayBillNumber.Len();
								if(way_bill_code_len) {
									const size_t min_code_len = MIN(rCode.Len(), way_bill_code_len);
									for(size_t cl = 1; cl <= min_code_len; cl++) {
										const char * p1 = rCode.cptr() + rCode.Len() - cl;
										const char * p2 = WayBillNumber.cptr() + way_bill_code_len - cl;
										if(strnicmp(p1, p2, cl) == 0) {
											if(cl > MaxMatchCodelen) {
												MaxMatchCodelen = cl;
												MaxMatchID = billID;
											}
										}
										else
											break;
									}
								}
							}
							PPID   MaxMatchID;
						private:
							uint   MaxMatchCodelen;
							const  SString WayBillNumber;
						};
						//uint   max_match_codelen = 0;
						//PPID   max_match_id = 0;
						//const  size_t way_bill_code_len = sstrlen(rRec.WayBillNumber);
						MatchCodeBlock mcb(rRec.WayBillNumber);
						if(select_intrrcpt) {
							PPObjOprKind op_obj;
							PPOprKind op_rec;
							PPIDArray op_list_intrexpnd;
							PPIDArray op_list_intrrcpt;
							for(SEnum en = op_obj.Enum(0); en.Next(&op_rec) > 0;) {
								const int intrt = IsIntrOp(op_rec.ID);
								if(intrt == INTREXPND)
									op_list_intrexpnd.add(op_rec.ID);
								else if(intrt == INTRRCPT)
									op_list_intrrcpt.add(op_rec.ID);
							}
							if(op_list_intrexpnd.getCount() && rcvr_ar_id) {
								op_list_intrexpnd.sortAndUndup();
								for(uint i = 0; i < op_list_intrexpnd.getCount(); i++) {
									const  PPID op_id = op_list_intrexpnd.get(i);
									for(DateIter di(&sp); p_bobj->P_Tbl->EnumByObj(rcvr_ar_id, &di, &bill_rec) > 0;) {
										if(bill_rec.OpID == op_id && (!loc_id || bill_rec.LocID == loc_id)) {
											temp_id_list.add(bill_rec.ID);
											// @v11.1.12 BillCore::GetCode(temp_buf = bill_rec.Code);
											temp_buf = bill_rec.Code; // @v11.1.12 
											mcb.ProcessBillCode(bill_rec.ID, temp_buf);
										}
									}
								}
							}
							if(op_list_intrrcpt.getCount()) {
								op_list_intrrcpt.sortAndUndup();
								for(uint i = 0; i < op_list_intrrcpt.getCount(); i++) {
									const  PPID op_id = op_list_intrrcpt.get(i);
									for(DateIter di(&sp); p_bobj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
										if(bill_rec.OpID == op_id && (!Filt.LocID__ || bill_rec.LocID == Filt.LocID__)) {
											temp_id_list.add(bill_rec.ID);
											// @v11.1.12 BillCore::GetCode(temp_buf = bill_rec.Code);
											temp_buf = bill_rec.Code; // @v11.1.12 
											mcb.ProcessBillCode(bill_rec.ID, temp_buf);
										}
									}
								}
							}
						}
						else {
							for(DateIter di(&sp); p_bobj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
								if((!Filt.LocID__ || bill_rec.LocID == Filt.LocID__) && GetOpType(bill_rec.OpID) == PPOPT_GOODSRECEIPT) {
									temp_id_list.add(bill_rec.ID);
									// @v11.1.12 BillCore::GetCode(temp_buf = bill_rec.Code);
									temp_buf = bill_rec.Code; // @v11.1.12 
									mcb.ProcessBillCode(bill_rec.ID, temp_buf);
								}
							}
						}
						bill_filt.List.Set(&temp_id_list);
						if(rRec.LinkBillID && temp_id_list.lsearch(rRec.LinkBillID))
							bill_filt.Sel = rRec.LinkBillID;
						else if(mcb.MaxMatchID)
							bill_filt.Sel = mcb.MaxMatchID;
					}
					//bill_filt.LocList.Add(Filt.LocID);
					//bill_filt.OpID  = op_id;
					//bill_filt.ObjectID = ar_id;
					bill_filt.Flags |= BillFilt::fAsSelector;
					/*if(SetupSurveyPeriod(rRec, bill_filt.Period))*/ {
						PPViewBill bill_view;
						if(bill_view.Init_(&bill_filt)) {
							{
								SString brw_title;
								temp_buf = rRec.WayBillNumber[0] ? rRec.WayBillNumber : rRec.IssueNumber;
								(brw_title = temp_buf).Space();
								if(checkdate(rRec.WayBillDate))
									brw_title.Cat(rRec.WayBillDate, DATF_DMY);
								else if(checkdate(rRec.IssueDate))
									brw_title.Cat(rRec.IssueDate, DATF_DMY);
								brw_title.CatDiv('-', 1).Cat(rRec.Volume, MKSFMTD_030);
								if(VetisEntityCore::GetProductItemName(rRec.EntityID, rRec.ProductItemID, rRec.SubProductID, rRec.ProductID, temp_buf) > 0)
									brw_title.CatDiv('-', 1).Cat(temp_buf);
								bill_view.SetOuterTitle(brw_title);
							}
							if(bill_view.Browse(0) > 0) {
								bill_id = static_cast<const BillFilt *>(bill_view.GetBaseFilt())->Sel;
								THROW(ok = EC.MatchDocument(rRec.EntityID, bill_id, 0, 0/*fromBill*/, 1));
							}
						}
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	{
		for(uint i = 0; i < locked_bill_list.getCount(); i++) {
			p_bobj->Unlock(locked_bill_list.get(i));
		}
	}
	return ok;
}

int PPViewVetisDocument::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				ok = -1;
				if(id) {
					VetisVetDocument item;
					if(EC.Get(id, item) > 0) {
						if(Filt.Flags & Filt.fAsSelector && pBrw->IsInState(sfModal)) {
							Filt.Sel = id;
							Filt.SelLotUuid = item.Uuid;
							ok = 1;
							pBrw->endModal(Filt.Sel ? cmOK : cmCancel);
						}
						else {
							if(EditVetisVetDocument(item, Filt.MainOrgID, Filt.LocID__) > 0) {
								if(!EC.Put(&id, item, EC.putvdfForceUpdateOuterFields|EC.putvdfEnableClearNativeBillLink, 0, 1)) // @v11.1.8 putvdfEnableClearNativeBillLink
									PPError();
							}
						}
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(id) {
					VetisDocumentTbl::Rec rec;
					if(EC.SearchDocument(id, &rec) > 0) {
						if(rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING && CONFIRM(PPCFM_VETISRMVPREPENTRY)) {
							int r = EC.DeleteEntity(id, 1);
							if(r > 0)
								ok = 1;
							else if(!r)
								PPError();
						}
						else if(rec.VetisDocStatus == vetisdocstCONFIRMED && rec.Flags & VetisVetDocument::fFromMainOrg) {
							VetisVetDocument item;
							if(EC.Get(rec.EntityID, item) > 0 && !!item.Uuid && CONFIRM(PPCFM_VETISWITHDRAWDOC)) {
								PPVetisInterface::Param param(0, Filt.LocID__, 0);
								if(PPVetisInterface::SetupParam(param)) {
									PPLogger logger;
									PPVetisInterface ifc(&logger);
									if(ifc.Init(param)) {
										VetisApplicationBlock reply;
										if(ifc.WithdrawVetDocument(item.Uuid, reply) > 0)
											ok = 1;
									}
									logger.Save(PPFILNAM_VETISINFO_LOG, 0);
								}
							}
						}
					}
				}
				break;
			case PPVCMD_LOTOPS:
				ok = -1;
				if(id) {
					VetisDocumentTbl::Rec rec;
					if(EC.SearchDocument(id, &rec) > 0 && rec.LinkBillID && rec.LinkBillRow > 0) {
						Transfer * trfr = BillObj->trfr;
						TransferTbl::Rec trfr_rec;
						if(trfr->SearchByBill(rec.LinkBillID, 0, rec.LinkBillRow, &trfr_rec) > 0 && trfr_rec.LotID)
							::ViewOpersByLot(trfr_rec.LotID, 0);
					}
				}
				break;
			case PPVCMD_VIEWLINKREST:
				if(id) {
					VetisDocumentTbl::Rec rec;
					if(EC.SearchDocument(id, &rec) > 0 && rec.VetisDocStatus != vetisdocstSTOCK) {
						if(rec.VetisDocStatus == vetisdocstOUTGOING_PREPARING) {
							if(rec.OrgDocEntityID) {
								VetisDocumentFilt inner_filt;
								inner_filt.VDStatusFlags = (1<<vetisdocstSTOCK); // только остатки
								inner_filt.LinkVDocID = rec.OrgDocEntityID;
								PPView::Execute(PPVIEW_VETISDOCUMENT, &inner_filt, /*PPView::exefModeless*/0, 0);
							}
						}
						else {
							VetisDocumentFilt inner_filt;
							inner_filt.LinkVDocID = id;
							PPView::Execute(PPVIEW_VETISDOCUMENT, &inner_filt, /*PPView::exefModeless*/0, 0);
						}
					}
				}
				break;
			case PPVCMD_UTILIZEDVDOC: ok = ProcessIncoming(id); break;
			case PPVCMD_SENDOUTGOING: ok = ProcessOutcoming(id); break;
			case PPVCMD_UPDATEITEMS:  ok = LoadDocuments(); break;
			case PPVCMD_VETISWAREHOUSE: ok = ViewWarehouse(); break;
			case PPVCMD_VETISPRODUCT:   ok = ViewGoods(); break;
			case PPVCMD_SETUPOUTGOING:
				ok = -1;
				{
					PPVetisInterface::Param param(0, Filt.LocID__, 0);
					if(PPVetisInterface::SetupParam(param)) {
						DateRange period;
						if(checkdate(Filt.Period.low))
							period.low = Filt.Period.low;
						if(checkdate(Filt.Period.upp))
							period.upp = Filt.Period.upp;
						if(!checkdate(period.low))
							period.low = ValidDateOr(period.upp, getcurdate_());
						if(!checkdate(period.upp)) {
							/*if(checkdate(period.low))
								period.upp = period.low;
							else*/
								period.upp = getcurdate_();
						}
						PPLogger logger;
						PPVetisInterface ifc(&logger);
						if(ifc.Init(param)) {
							if(ifc.SetupOutgoingEntries(Filt.LocID__, period) > 0)
								ok = 1;
						}
						logger.Save(PPFILNAM_VETISINFO_LOG, 0);
					}
					else
						PPError();
				}
				break;
			case PPVCMD_MATCHPERSON:
				ok = -1;
				if(id) {
					const  int ccol = pBrw->GetCurColumn();
					int    obj_to_match = 0;
					VetisDocumentTbl::Rec rec;
					uint   v = 0;
					switch(ccol) {
						case 7: obj_to_match = otmFrom; break; // from
						case 8: obj_to_match = otmTo; break; // to
						case 5:
						case 6: obj_to_match = otmBill; break; // bill
						case 9: obj_to_match = otmLot; break; // goods --> match lot
						default:
							if(SelectorDialog(DLG_SELVETMATCHOBJ, STDCTL_SELECTOR_WHAT, &v) > 0) {
								switch(v) {
									case 0: obj_to_match = otmFrom; break;
									case 1: obj_to_match = otmTo; break;
									case 2: obj_to_match = otmBill; break;
									case 3: obj_to_match = otmLot; break;
								}
							}
							break;
					}
					if(obj_to_match && EC.SearchDocument(id, &rec) > 0)
						ok = MatchObject(rec, obj_to_match);
				}
				break;
			case PPVCMD_REREQUESTDOC: // Отладочная функция
				ok = -1;
				if(id) {
					RerequestDocument(id);
				}
				break;
			case PPVCMD_SPCFUNC:
				ok = -1;
				if(id) {
					const  int ccol = pBrw->GetCurColumn();
					int    obj_to_match = 0;
					VetisDocumentTbl::Rec rec;
					switch(ccol) {
						case 7: obj_to_match = otmFrom; break; // from
						case 8: obj_to_match = otmTo; break; // to
					}
					if(obj_to_match && EC.SearchDocument(id, &rec) > 0)
						ok = ForceResolveObject(rec, obj_to_match);
				}
				break;
		}
	}
	return ok;
}

VetisDocumentTotal::VetisDocumentTotal()
{
	THISZERO();
}

int PPViewVetisDocument::CalcTotal(VetisDocumentTotal * pTotal)
{
	int    ok = 1;
	VetisDocumentViewItem item;
	memzero(pTotal, sizeof(*pTotal));
	for(InitIteration(); NextIteration(&item) > 0;) {
		pTotal->Count++;
		switch(item.VetisDocStatus) {
			case vetisdocstCREATED: pTotal->CtCreated++; break;
			case vetisdocstCONFIRMED: pTotal->CtConfirmed++; break;
			case vetisdocstWITHDRAWN: pTotal->CtWithdrawn++; break;
			case vetisdocstUTILIZED: pTotal->CtUtilized++; break;
			case vetisdocstFINALIZED: pTotal->CtFinalized++; break;
			case vetisdocstOUTGOING_PREPARING: pTotal->CtOutgoingPrep++; break;
			case vetisdocstSTOCK: pTotal->CtStock++; break;
		}
	}
	return ok;
}

void PPViewVetisDocument::ViewTotal()
{
	TDialog * dlg = 0;
	VetisDocumentTotal total;
	CalcTotal(&total);
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_VETDOCTOTAL)))) {
		dlg->setCtrlLong(CTL_VETDOCTOTAL_COUNT, total.Count);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_CRCT, total.CtCreated);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_CONFCT, total.CtConfirmed);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_WDCT, total.CtWithdrawn);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_UTCT, total.CtUtilized);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_FINCT, total.CtFinalized);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_OGCT, total.CtOutgoingPrep);
		dlg->setCtrlLong(CTL_VETDOCTOTAL_STKCT, total.CtStock);
		ExecViewAndDestroy(dlg);
	}
}
