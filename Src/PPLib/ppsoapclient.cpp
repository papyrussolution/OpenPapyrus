// PPSOAPCLIENT.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

BOOL Implement_SoapModule_DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved, const char * pProductName)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name = pProductName;
				SLS.Init(product_name, (HINSTANCE)hModule);
			}
			break;
		case DLL_THREAD_ATTACH:
			SLS.InitThread();
			break;
		case DLL_THREAD_DETACH:
			SLS.ReleaseThread();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

//static
SCollection PPSoapResultPtrBase::ResultPtrList;

int FASTCALL PPSoapDestroyResultPtr(void * p)
{
	int    ok = 0;
	uint c = PPSoapResultPtrBase::ResultPtrList.getCount();
	if(c) do {
		PPSoapResultPtrBase * p_item = (PPSoapResultPtrBase *)PPSoapResultPtrBase::ResultPtrList.at(--c);
		if(p_item && *p_item == p) {
			p_item->Destroy();
			PPSoapResultPtrBase::ResultPtrList.atFree(c);
			ok = 1;
		}
	} while(!ok && c);
	return ok;
}

PPSoapClientSession::PPSoapClientSession()
{
	Url[0] = 0;
	ErrCode = 0;
	ErrMsg[0] = 0;
}

void PPSoapClientSession::Setup(const char * pUrl)
{
	STRNSCPY(Url, pUrl);
	ErrCode = 0;
	ErrMsg[0] = 0;
	User.Z();
	Password.Z();
}

void PPSoapClientSession::Setup(const char * pUrl, const char * pUser, const char * pPassword)
{
	Setup(pUrl);
	User = pUser;
	Password = pPassword;
}

const char * PPSoapClientSession::GetUrl() const
{
	return Url[0] ? Url : 0;
}

const char * PPSoapClientSession::GetUser() const
{
	return User;
}

const char * PPSoapClientSession::GetPassword() const
{
	return Password;
}

void FASTCALL PPSoapClientSession::SetMsg(const char * pUtf8Text)
{
	SString temp_buf;
	(temp_buf = pUtf8Text).Transf(CTRANSF_UTF8_TO_INNER);
	temp_buf.ReplaceChar('\xA', ' '); // @v8.7.4
	temp_buf.CopyTo(ErrMsg, sizeof(ErrMsg));
}

const  char * PPSoapClientSession::GetMsg() const
{
	return ErrMsg;
}
//
//
//
void ** FASTCALL PPSoapCreateArray(uint count, int & rArrayCount)
{
	void ** pp_list = count ? (void **)SAlloc::C(count, sizeof(void *)) : 0;
	rArrayCount = (int)count;
	return pp_list;
}

InParamString::InParamString(const char * pStr) : STempBuffer(0)
{
	size_t slen = sstrlen(pStr);
	size_t len = slen ? slen*2 : 32;
	if(Alloc(len)) {
		if(pStr)
			strnzcpy((char *)*this, pStr, GetSize());
		else
			((char *)*this)[0] = 0;
	}
}

char * FASTCALL GetDynamicParamString(const char * pSrc, TSCollection <InParamString> & rPool)
{
	InParamString * p_new_item = new InParamString(pSrc);
	if(p_new_item) {
		if(!rPool.insert(p_new_item)) {
			ZDELETE(p_new_item);
		}
	}
	return p_new_item ? (char *)*p_new_item : 0;
}

char * FASTCALL GetDynamicParamString(long ival, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	return GetDynamicParamString(temp_buf.Cat(ival), rPool);
}

char * FASTCALL GetDynamicParamString(int ival, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	return GetDynamicParamString(temp_buf.Cat((long)ival), rPool);
}

char * FASTCALL GetDynamicParamString_(double rval, long fmt, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	temp_buf.Cat(rval, fmt);
	//temp_buf.ReplaceChar('.', ',');
	return GetDynamicParamString(temp_buf, rPool);
}

char * FASTCALL GetDynamicParamString(LDATE dval, long fmt, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	if(checkdate(dval, 0))
		temp_buf.Cat(dval, fmt);
	else
		temp_buf.Z();
	return GetDynamicParamString(temp_buf, rPool);
}

char * FASTCALL GetDynamicParamString(LDATETIME dtval, long dfmt, long tfmt, TSCollection <InParamString> & rPool)
{
	SString temp_buf;
	if(checkdate(dtval.d, 0))
		temp_buf.Cat(dtval, dfmt, tfmt);
	else
		temp_buf.Z();
	return GetDynamicParamString(temp_buf, rPool);
}
//
//
//
UhttStatus::UhttStatus()
{
	Code = 0;
	Index = -1;
	Id = -1;
}
//
//
//
UhttDate & UhttDate::Clear()
{
	Date.Z();
	return *this;
}

UhttDate & FASTCALL UhttDate::operator = (const char * pStr)
{
	Date = pStr;
	return *this;
}

UhttDate & FASTCALL UhttDate::operator = (LDATE dt)
{
	Date.Z().Cat(dt, DATF_DMY|DATF_CENTURY);
	return *this;
}

UhttDate::operator LDATE () const
{
	LDATE dt = ZERODATE;
	strtodate(Date, DATF_DMY, &dt);
	return dt;
}
//
// Descr: Дата/время, передаваемые одной строкой в формате ISO-8601 (yyyy-mm-dd Thh:mm:ss)
//
UhttTimestamp & UhttTimestamp::Clear()
{
	T.Z();
	return *this;
}

UhttTimestamp & FASTCALL UhttTimestamp::operator = (LDATE dt)
{
	LDATETIME temp;
	temp.Set(dt, ZEROTIME);
	T.Z().Cat(temp, DATF_ISO8601|DATF_CENTURY, 0);
	return *this;
}

UhttTimestamp & FASTCALL UhttTimestamp::operator = (LTIME tm)
{
	LDATETIME temp;
	temp.Set(encodedate(1, 1, 1970), tm);
	T.Z().Cat(temp, DATF_ISO8601|DATF_CENTURY, 0);
	return *this;
}

UhttTimestamp & FASTCALL UhttTimestamp::operator = (const LDATETIME & rDtm)
{
	T.Z().Cat(rDtm, DATF_ISO8601|DATF_CENTURY, 0);
	return *this;
}

UhttTimestamp::operator LDATE () const
{
	LDATETIME temp;
	temp.SetZero();
	if(T.NotEmpty())
		strtodatetime(T, &temp, DATF_ISO8601, TIMF_HMS);
	return temp.d;
}

UhttTimestamp::operator LTIME () const
{
	LDATETIME temp;
	temp.SetZero();
	if(T.NotEmpty())
		strtodatetime(T, &temp, DATF_ISO8601, TIMF_HMS);
	return temp.t;
}

UhttTimestamp::operator LDATETIME () const
{
	LDATETIME temp;
	temp.SetZero();
	if(T.NotEmpty())
		strtodatetime(T, &temp, DATF_ISO8601, TIMF_HMS);
	return temp;
}
//
//
//
UhttDateTime & UhttDateTime::Clear()
{
	Date.Z();
	Time.Z();
	return *this;
}

UhttDateTime & FASTCALL UhttDateTime::operator = (LDATE dt)
{
	Date.Z().Cat(dt, DATF_DMY|DATF_CENTURY);
	return *this;
}

UhttDateTime & FASTCALL UhttDateTime::operator = (LTIME tm)
{
	Time.Z().Cat(tm, TIMF_HMS);
	return *this;
}

UhttDateTime & FASTCALL UhttDateTime::operator = (const LDATETIME & rDtm)
{
	Date.Z().Cat(rDtm.d, DATF_DMY|DATF_CENTURY);
	Time.Z().Cat(rDtm.t, TIMF_HMS);
	return *this;
}

UhttDateTime::operator LDATE () const
{
	LDATE dt = ZERODATE;
	if(Date.NotEmpty())
		strtodate(Date, DATF_DMY, &dt);
	return dt;
}

UhttDateTime::operator LTIME () const
{
	LTIME tm = ZEROTIME;
	if(Time.NotEmpty())
		strtotime(Time, TIMF_HMS, &tm);
	return tm;
}

UhttDateTime::operator LDATETIME () const
{
	LDATETIME dtm;
	dtm.d = (LDATE)*this;
	dtm.t = (LTIME)*this;
	return dtm;
}
//
//
//
UhttDatePeriod & UhttDatePeriod::Clear()
{
	Low.Z();
	Upp.Z();
	return *this;
}

UhttDatePeriod & FASTCALL UhttDatePeriod::operator = (const DateRange & rRng)
{
	Low.Z().Cat(rRng.low, DATF_DMY|DATF_CENTURY);
	Upp.Z().Cat(rRng.upp, DATF_DMY|DATF_CENTURY);
	return *this;
}
//
//
//
UhttTagItem::UhttTagItem()
{
}

UhttTagItem::UhttTagItem(const char * pSymb, const SString & rText)
{
    SetSymb(pSymb);
    SetValue(rText);
}

void FASTCALL UhttTagItem::SetSymb(const char * pText)
{
	(Symb = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttTagItem::SetValue(const char * pText)
{
	(Value = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttBrandPacket::UhttBrandPacket() : ID(0), OwnerID(0), Flags(0)
{
	Name[0] = 0;
}
//
//
//
UhttStyloDevicePacket::UhttStyloDevicePacket() : ID(0), ParentID(0), Flags(0), DeviceVer(0)
{
}

void FASTCALL UhttStyloDevicePacket::SetName(const char * pText)
{
	(Name = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttStyloDevicePacket::SetSymb(const char * pText)
{
	(Symb = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttGoodsPacket::UhttGoodsPacket() : ID(0), BrandID(0), ManufID(0), Package(0.0)
{
	Name[0] = 0;
	SingleBarcode[0] = 0;
}

UhttGoodsPacket & FASTCALL UhttGoodsPacket::operator = (const UhttGoodsPacket & rS)
{
	return Copy(rS);
}

UhttGoodsPacket & FASTCALL UhttGoodsPacket::Copy(const UhttGoodsPacket & rS)
{
	ID = rS.ID;
	BrandID = rS.BrandID;
	ManufID = rS.ManufID;
	STRNSCPY(Name, rS.Name);
	STRNSCPY(SingleBarcode, rS.SingleBarcode);
	Package = rS.Package;
	ExtA = rS.ExtA;
	ExtB = rS.ExtB;
	ExtC = rS.ExtC;
	ExtD = rS.ExtD;
	ExtE = rS.ExtE;
	BarcodeList = rS.BarcodeList;
	TSCollection_Copy(TagList, rS.TagList);
	return *this;
}

void FASTCALL UhttGoodsPacket::SetName(const char * pName)
{
	SString temp_buf = pName;
	temp_buf.Strip();
	temp_buf.ReplaceChar('\x07', ' ');
	temp_buf.ReplaceStrR("  ", " ", 0);
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8).CopyTo(Name, sizeof(Name));
}

void FASTCALL UhttGoodsPacket::SetExt(int extFldId, const char * pText)
{
	if(oneof5(extFldId, GDSEXSTR_A, GDSEXSTR_B, GDSEXSTR_C, GDSEXSTR_D, GDSEXSTR_E)) {
		SString temp_buf = pText;
		if(temp_buf.NotEmptyS()) {
			temp_buf.ReplaceChar('\x07', ' ');
			temp_buf.ReplaceStrR("  ", " ", 0);
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			switch(extFldId) {
				case GDSEXSTR_A: ExtA = temp_buf; break;
				case GDSEXSTR_B: ExtB = temp_buf; break;
				case GDSEXSTR_C: ExtC = temp_buf; break;
				case GDSEXSTR_D: ExtD = temp_buf; break;
				case GDSEXSTR_E: ExtE = temp_buf; break;
			}
		}
	}
}
//
// Descr: Представление 'лемента сопоставления приватного идентификатора объекта с идентификатором
//   соответствующего объекта в Universe-HTT по коду.
//
UhttCodeRefItem::UhttCodeRefItem() : PrivateID(0), UhttID(0), InnerPos(0)
{
	Code[0] = 0;
}

UhttCodeRefItem & FASTCALL UhttCodeRefItem::Set(int privateID, const char * pCode)
{
	PrivateID = privateID;
	UhttID = 0;
	InnerPos = 0;
	STRNSCPY(Code, pCode);
	return *this;
}
//
//
//
UhttDocumentPacket::UhttDocumentPacket() : UhttObjID(0), Size(0)
{
}

int UhttDocumentPacket::SetFile(const char * pFileName)
{
	int    ok = 1;
	Encoding.Z();
	ContentType.Z();
	ContentMime.Z();

	SFile f(pFileName, SFile::mRead|SFile::mBinary);
	THROW(f.IsValid());
	{
		SFileFormat ff;
		ff.Identify(pFileName);
		if(!SFileFormat::GetMime((int)ff, ContentType))
			ContentType = "application/octet-stream";
	}
	Encoding = "base64";
	{
		STempBuffer tbuf(8192*3); // Кратность размера буфера 3 КРИТИЧНА!
		SString temp_str;
		THROW(f.CalcSize(&Size));
		for(int64 rest_size = Size; rest_size > 0;) {
			size_t temp_sz = (size_t)MIN(rest_size, tbuf.GetSize());
			if(temp_sz) {
				THROW(f.ReadV(tbuf, temp_sz));
				ContentMime.Cat(temp_str.EncodeMime64(tbuf, temp_sz));
			}
			rest_size -= temp_sz;
		}
		{
			SPathStruc ps;
			ps.Split(pFileName);
			Name = ps.Nam;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
UhttSpecSeriesPacket::UhttSpecSeriesPacket() : ID(0), GoodsID(0), ManufID(0), ManufCountryID(0), LabID(0), InfoKind(0), Flags(0)
{
}

void FASTCALL UhttSpecSeriesPacket::SetSerial(const char * pText)
{
	(Serial = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttSpecSeriesPacket::SetGoodsName(const char * pText)
{
	(GoodsName = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttSpecSeriesPacket::SetManufName(const char * pText)
{
	(ManufName = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttSpecSeriesPacket::SetInfoIdent(const char * pText)
{
	(InfoIdent = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttSpecSeriesPacket::SetAllowNumber(const char * pText)
{
	(AllowNumber = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttSpecSeriesPacket::SetLetterType(const char * pText)
{
	(LetterType = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttLocationPacket::UhttLocationPacket() : ID(0), ParentID(0), Type(0), CityID(0), OwnerPersonID(0), Latitude(0.0), Longitude(0.0), Flags(0)
{
}

void FASTCALL UhttLocationPacket::SetName(const char * pName)
{
	(Name = pName).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttLocationPacket::SetCode(const char * pCode)
{
	(Code = pCode).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttLocationPacket::SetPostalCode(const char * pPostalCode)
{
	(PostalCode = pPostalCode).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttLocationPacket::SetAddress(const char * pAddress)
{
	(Address = pAddress).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttLocationPacket::SetPhone(const char * pPhone)
{
	(Phone = pPhone).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttLocationPacket::SetEMail(const char * pEMail)
{
	(EMail = pEMail).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttLocationPacket::SetContact(const char * pContact)
{
	(Contact = pContact).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

SString & FASTCALL UhttLocationPacket::GetCode(SString & rBuf) const
{
	rBuf = Code;
	if(!rBuf.NotEmptyS())
		rBuf.Z().CatChar('#').Cat("UHTT").CatLongZ(ID, 6);
	return rBuf;
}
//
//
//
UhttSCardPacket::UhttSCardPacket()
{
	ID = 0;
	SeriesID = 0;
	OwnerID = 0;
	PDis = 0.0;
	Overdraft = 0.0;
	Debit = 0.0;
	Credit = 0.0;
	Rest = 0.0;
	Flags = 0;
}

void FASTCALL UhttSCardPacket::SetCode(const char * pCode)
{
	(Code = pCode).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttCheckPacket::UhttCheckPacket()
{
	PosNodeID = 0;
	LocID = 0;
	SCardID = 0;
	Amount = 0;
	Discount = 0;
	Flags = 0;
}
//
//
//
UhttQuotFilter::UhttQuotFilter()
{
	GoodsID = 0;
	GroupID = 0;
	BrandID = 0;
	SellerID = 0;
	LocationID = 0;
	BuyerID = 0;
	AndFlags = 0;
	NotFlags = 0;
}
//
//
//
UhttQuotPacket::UhttQuotPacket()
{
	GoodsID = 0;
	SellerID = 0;
	LocID = 0;
	BuyerID = 0;
	CurrID = 0;
	MinQtty = 0.0;
	Value = 0.0;
	Flags = 0;
}
//
//
//
UhttPersonPacket::UhttPersonPacket()
{
	ID = 0;
	StatusID = 0;
	CategoryID = 0;
}

UhttPersonPacket & UhttPersonPacket::operator = (const UhttPersonPacket & rS)
{
#define FLD(f) f = rS.f
	FLD(ID);
	FLD(StatusID);
	FLD(CategoryID);
	FLD(Name);
	FLD(INN);
	FLD(PhoneList);
	FLD(EMailList);
	FLD(UrlList);
#undef FLD
	TSCollection_Copy(KindList, rS.KindList);
	TSCollection_Copy(RegList, rS.RegList);
	TSCollection_Copy(AddrList, rS.AddrList);
	return *this;
}

SString & UhttPersonPacket::GetUhttContragentCode(SString & rBuf) const
{
	rBuf.Z().CatChar('#').Cat("UHTT").CatLongZ(ID, 6);
	return rBuf;
}

UhttPersonPacket::AddressP::AddressP() : UhttLocationPacket(), Kind(0)
{
}
//
//
//
UhttBillPacket::UhttBillPacket()
{
	ID = 0;
	LocID = 0;
	ArID = 0;
	DlvrLocID = 0;
	CurrID = 0;
	AgentID = 0;
	StatusID = 0;
	Flags = 0;
	Uuid.SetZero();
}
//
//
//
UhttBillFilter::UhttBillFilter() : LocID(0), ArID(0), CurrID(0), AgentID(0), Count(0), Last(0)
{
	Since = ZERODATETIME;
}

int FASTCALL UhttBillFilter::SetDate(LDATE dt)
{
	Dt.Z().Cat(dt, DATF_DMY|DATF_CENTURY);
	return 1;
}

void FASTCALL UhttBillFilter::SetOpSymb(const char * pOpSymb)
{
	(OpSymb = pOpSymb).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttGoodsRestListItem::UhttGoodsRestListItem() : GoodsID(0), LocID(0), Rest(0.0), Price(0.0)
{
}
//
//
//
UhttDCFileVersionInfo::UhttDCFileVersionInfo() : ID(0), Revision(0), Ts(0), Size(0), Flags(0)
{
}
//
//
//
UhttWorkbookItemPacket::UhttWorkbookItemPacket()
{
	ID = 0;
	Type = 0;
	ParentID = 0;
	LinkID = 0;
	CssID = 0;
	Rank = 0;
	Flags = 0;
	KeywordCount = 0;
	KeywordDilute = 0;
}

UhttWorkbookItemPacket & FASTCALL UhttWorkbookItemPacket::operator = (const UhttWorkbookItemPacket & rS)
{
	return Copy(rS);
}

UhttWorkbookItemPacket & FASTCALL UhttWorkbookItemPacket::Copy(const UhttWorkbookItemPacket & rS)
{
	ID = rS.ID;
	Type = rS.Type;
	ParentID = rS.ParentID;
	LinkID = rS.LinkID;
	CssID = rS.CssID;
	Rank = rS.Rank;
	Flags = rS.Flags;
	KeywordCount = rS.KeywordCount;
	KeywordDilute = rS.KeywordDilute;
	Name = rS.Name;
	Symb = rS.Symb;
	Dtm = rS.Dtm;
	ModifDtm = rS.ModifDtm; // @v9.3.8
	ContentModifDtm = rS.ContentModifDtm; // @v9.3.7
	Version = rS.Version;
	Descr = rS.Descr;
	TSCollection_Copy(TagList, rS.TagList);
	return *this;
}

void   FASTCALL UhttWorkbookItemPacket::SetName(const char * pText)
{
	(Name = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void   FASTCALL UhttWorkbookItemPacket::SetSymb(const char * pText)
{
	(Symb = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void   FASTCALL UhttWorkbookItemPacket::SetVersion(const char * pText)
{
	(Version = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void   FASTCALL UhttWorkbookItemPacket::SetDescr(const char * pText)
{
	(Descr = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttPrcPlaceDescription::UhttPrcPlaceDescription() : GoodsID(0)
{
}

void FASTCALL UhttPrcPlaceDescription::SetRange(const char * pText)
{
	(Range = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttPrcPlaceDescription::SetDescr(const char * pText)
{
	(Descr = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttProcessorPacket::UhttProcessorPacket()
{
	ID = 0;
	ParentID = 0;
	Kind = 0;
	Flags = 0;
	LocID = 0;
	LinkObjType = 0;
	LinkObjID = 0;
	CipPersonKindID = 0;
	CipMax = 0;
}

UhttProcessorPacket & FASTCALL UhttProcessorPacket::operator = (const UhttProcessorPacket & rS)
{
	return Copy(rS);
}

UhttProcessorPacket & FASTCALL UhttProcessorPacket::Copy(const UhttProcessorPacket & rS)
{
	ID = rS.ID;
	ParentID = rS.ParentID;
	Kind = rS.Kind;
	Flags = rS.Flags;
	LocID = rS.LocID;
	LinkObjType = rS.LinkObjType;
	LinkObjID = rS.LinkObjID;
	CipPersonKindID = rS.CipPersonKindID;
	CipMax = rS.CipMax;
	TSCollection_Copy(Places, rS.Places);
	return *this;
}

void FASTCALL UhttProcessorPacket::SetName(const char * pName)
{
	(Name = pName).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttProcessorPacket::SetSymb(const char * pSymb)
{
	(Symb = pSymb).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttTSessLine::UhttTSessLine()
{
    TSessID = 0;
    OprNo = 0;
    GoodsID = 0;
    LotID = 0;
    UserID = 0;
    Sign = 0;
    Flags = 0;
    Qtty = 0.0;
    WtQtty = 0.0;
    Price = 0.0;
    Discount = 0.0;
}
//
//
//
UhttCipPacket::UhttCipPacket()
{
	ID = 0;
	Kind = 0;
	PrmrID = 0;
	PersonID = 0;
	Num = 0;
	RegCount = 0;
	CiCount = 0;
	Flags = 0;
	Amount = 0.0;
	CCheckID = 0;
	SCardID = 0;
}

void FASTCALL UhttCipPacket::SetMemo(const char * pText)
{
	(Memo = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttCipPacket::SetPlaceCode(const char * pText)
{
	(PlaceCode = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
UhttTSessionPacket::UhttTSessionPacket() : ID(0), Num(0), PrcID(0), TechID(0), ParentID(0), Status(0), Flags(0)
{
}

UhttTSessionPacket & FASTCALL UhttTSessionPacket::operator = (const UhttTSessionPacket & rS)
{
	return Copy(rS);
}

UhttTSessionPacket & FASTCALL UhttTSessionPacket::Copy(const UhttTSessionPacket & rS)
{
	ID = rS.ID;
	Num = rS.Num;
	PrcID = rS.PrcID;
	TechID = rS.TechID;
	ParentID = rS.ParentID;
	Status = rS.Status;
	Flags = rS.Flags;
	StTime = rS.StTime;
	FinTime = rS.FinTime;
	Memo = rS.Memo;
	Detail = rS.Detail;
	TSCollection_Copy(Lines, rS.Lines);
	TSCollection_Copy(Cips, rS.Cips);
	TSCollection_Copy(Places, rS.Places);
	TSCollection_Copy(TagList, rS.TagList);
	return *this;
}

void FASTCALL UhttTSessionPacket::SetMemo(const char * pText)
{
	(Memo = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}

void FASTCALL UhttTSessionPacket::SetDetail(const char * pText)
{
	(Detail = pText).Strip().Transf(CTRANSF_INNER_TO_UTF8);
}
//
//
//
iSalesUOM::iSalesUOM() : Code(0), Width(0.0), Height(0.0), Length(0.0), Netto(0.0), Brutto(0.0)
{
}

int iSalesUOM::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Code, rBuf));
	THROW(pSCtx->Serialize(dir, Width, rBuf));
	THROW(pSCtx->Serialize(dir, Height, rBuf));
	THROW(pSCtx->Serialize(dir, Length, rBuf));
	THROW(pSCtx->Serialize(dir, Netto, rBuf));
	THROW(pSCtx->Serialize(dir, Brutto, rBuf));
	THROW(pSCtx->Serialize(dir, Barcode, rBuf));
	CATCHZOK
	return ok;
}

iSalesUOMCvt::iSalesUOMCvt() : Rate(0.0)
{
}

int iSalesUOMCvt::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, UomFrom, rBuf));
	THROW(pSCtx->Serialize(dir, UomTo, rBuf));
	THROW(pSCtx->Serialize(dir, Rate, rBuf));
	CATCHZOK
	return ok;
}
//
//
//
iSalesGoodsPacket::iSalesGoodsPacket() : TypeOfProduct(0), VatRate(0.0), UnitCode(0), Valid(0)
{
}

iSalesGoodsPacket::iSalesGoodsPacket(const iSalesGoodsPacket & rS)
{
	Copy(rS);
}

iSalesGoodsPacket & FASTCALL iSalesGoodsPacket::Copy(const iSalesGoodsPacket & rS)
{
	TypeOfProduct = rS.TypeOfProduct;
	OuterCode = rS.OuterCode;
	NativeCode = rS.NativeCode;
	Name = rS.Name;
	Abbr = rS.Abbr;
	VatRate = rS.VatRate;
	UnitCode = rS.UnitCode;
	Valid = rS.Valid;
	CountryName = rS.CountryName;
	CLB = rS.CLB;
	ErrMsg = rS.ErrMsg;
	TSCollection_Copy(UomList, rS.UomList);
	TSCollection_Copy(CvtList, rS.CvtList);
	return *this;
}

iSalesGoodsPacket & FASTCALL iSalesGoodsPacket::operator = (const iSalesGoodsPacket & rS)
{
	return Copy(rS);
}

int iSalesGoodsPacket::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
    int    ok = 1;
    THROW(pSCtx->Serialize(dir, TypeOfProduct, rBuf));
    THROW(pSCtx->Serialize(dir, OuterCode, rBuf));
    THROW(pSCtx->Serialize(dir, NativeCode, rBuf));
    THROW(pSCtx->Serialize(dir, Name, rBuf));
    THROW(pSCtx->Serialize(dir, Abbr, rBuf));
    THROW(pSCtx->Serialize(dir, VatRate, rBuf));
    THROW(pSCtx->Serialize(dir, UnitCode, rBuf));
    THROW(pSCtx->Serialize(dir, Valid, rBuf));
    THROW(pSCtx->Serialize(dir, CountryName, rBuf));
    THROW(pSCtx->Serialize(dir, CLB, rBuf));
    THROW(pSCtx->Serialize(dir, ErrMsg, rBuf));
    THROW(TSCollection_Serialize(UomList, dir, rBuf, pSCtx));
    THROW(TSCollection_Serialize(CvtList, dir, rBuf, pSCtx));
    CATCHZOK
    return ok;
}

double iSalesGoodsPacket::RecalcUnits(long uomFrom, long uomTo, double qtty) const
{
    double result = qtty;
	if(uomFrom && uomTo) {
		for(uint i = 0; i < CvtList.getCount(); i++) {
			const iSalesUOMCvt * p_cvt = CvtList.at(i);
			if(p_cvt) {
				if(p_cvt->UomFrom.ToLong() == uomFrom && p_cvt->UomTo.ToLong() == uomTo) {
					result = qtty * p_cvt->Rate;
					break;
				}
				else if(p_cvt->UomFrom.ToLong() == uomTo && p_cvt->UomTo.ToLong() == uomFrom) {
					result = fdivnz(qtty, p_cvt->Rate);
					break;
				}
			}
		}
	}
    return result;
}
//
//
//
iSalesRoutePacket::iSalesRoutePacket() : TypeOfRoute(0), Valid(0)
{
}

iSalesRoutePacket::iSalesRoutePacket(const iSalesRoutePacket & rS)
{
	Copy(rS);
}

iSalesRoutePacket & FASTCALL iSalesRoutePacket::Copy(const iSalesRoutePacket & rS)
{
	TypeOfRoute = rS.TypeOfRoute;
	Valid = rS.Valid;
	Ident = rS.Ident;
	Code = rS.Code;
	NativeAgentCode = rS.NativeAgentCode;
	ErrMsg = rS.ErrMsg;
	TSCollection_Copy(VisitList, rS.VisitList);
	return *this;
}

iSalesRoutePacket & FASTCALL iSalesRoutePacket::operator = (const iSalesRoutePacket & rS)
{
	return Copy(rS);
}

iSalesBillItem::iSalesBillItem() : LineN(0), UnitCode(0), Qtty(0.0)
{
}

iSalesBillAmountEntry::iSalesBillAmountEntry()
{
	THISZERO();
}

iSalesBillPacket::iSalesBillPacket()
{
	NativeID = 0;
	DocType = 0;
	ExtDocType = 0;
	Status = 0;
	Dtm.SetZero();
	IncDtm.SetZero();
	ExtDtm.SetZero();
	CreationDtm.SetZero();
	LastUpdDtm.SetZero();
	DueDate = ZERODATE;
}
//
//
//
SapEfesCallHeader::SapEfesCallHeader() : P_SalesOrg(0), P_Wareh(0), SessionID(0)
{
}

SapEfesOrder::SapEfesOrder() : Date(ZERODATETIME), DueDate(ZERODATE), Amount(0.0)
{
}

SapEfesBillPacket::SapEfesBillPacket() : NativeID(0), Flags(0), DocType(0), Date(ZERODATE), DueDate(ZERODATE)
{
}

SapEfesDebtReportEntry::SapEfesDebtReportEntry() : NativeArID(0), Debt(0.0), CreditLimit(0.0), PayPeriod(0), DebtDelayDays(0)
{
}

SapEfesDebtDetailReportEntry::SapEfesDebtDetailReportEntry() : NativeArID(0), BillDate(ZERODATE), PaymDate(ZERODATE), Amount(0.0), Debt(0.0)
{
}
