// LOCATION.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2018
// @codepage UTF-8
// @Kernel
//
// Implementation of class LocationCore
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_PPFILT_FACTORY(Location); SLAPI LocationFilt::LocationFilt(PPID locType, PPID ownerID, PPID parentID) : PPBaseFilt(PPFILT_LOCATION, 0, 0)
{
	SetFlatChunk(offsetof(LocationFilt, ReserveStart),
		offsetof(LocationFilt, Reserve)-offsetof(LocationFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(LocationFilt, ExtLocList));
	SetBranchSString(offsetof(LocationFilt, ExtString)); // @v8.3.2
	Init(1, 0);

	LocType = locType;
	Owner = ownerID;
	Parent = parentID;
}

LocationFilt & FASTCALL LocationFilt::operator = (const LocationFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

int SLAPI LocationFilt::GetExField(int fldId, SString & rBuf) const
{
	rBuf.Z();
	return (fldId == exfPhone) ? PPGetExtStrData(fldId, ExtString, rBuf) : -1;
}

int SLAPI LocationFilt::SetExField(int fldId, const char * pBuf)
{
	return (fldId == exfPhone) ? PPPutExtStrData(fldId, ExtString, pBuf) : -1;
}
//
//
// static
int FASTCALL LocationCore::IsEmptyAddressRec(const LocationTbl::Rec & rRec)
{
	int    yes = 1;
	if(rRec.CityID)
		yes = 0;
	else if(rRec.Latitude != 0.0 && rRec.Longitude != 0.0)
		yes = 0;
	else {
		SString temp_buf;
		if((temp_buf = rRec.Name).NotEmptyS())
			yes = 0;
		else if((temp_buf = rRec.Code).NotEmptyS())
			yes = 0;
		else {
			GetExField(&rRec, LOCEXSTR_ZIP, temp_buf);
			if(temp_buf.NotEmptyS())
				yes = 0;
			else {
				GetExField(&rRec, LOCEXSTR_SHORTADDR, temp_buf);
				if(temp_buf.NotEmptyS())
					yes = 0;
				else {
					GetExField(&rRec, LOCEXSTR_FULLADDR, temp_buf);
					if(temp_buf.NotEmptyS())
						yes = 0;
					else {
						GetExField(&rRec, LOCEXSTR_PHONE, temp_buf);
						if(temp_buf.NotEmptyS())
							yes = 0;
					}
				}
			}
		}
	}
	return yes;
}

//static
int FASTCALL LocationCore::Helper_IsEqExField(const LocationTbl::Rec & rRec1, const LocationTbl::Rec & rRec2, int fldId, SString & rBuf1, SString & rBuf2)
{
	GetExField(&rRec1, fldId, rBuf1);
	GetExField(&rRec2, fldId, rBuf2);
	return (rBuf1 == rBuf2);
}

// static
int FASTCALL LocationCore::IsEqualRec(const LocationTbl::Rec & rRec1, const LocationTbl::Rec & rRec2)
{
	#define CMP_FLD(f) if(rRec1.f != rRec2.f) return 0;
	CMP_FLD(ParentID);
	CMP_FLD(Type);
	CMP_FLD(Flags);
	CMP_FLD(OwnerID);
	CMP_FLD(CityID);
	CMP_FLD(RspnsPersonID);
	CMP_FLD(Latitude);
	CMP_FLD(Longitude);
	CMP_FLD(NumRows);
	CMP_FLD(NumLayers);
	CMP_FLD(MassCapacity);
	CMP_FLD(X);
	CMP_FLD(Y);
	CMP_FLD(Z);
	if(!sstreq(rRec1.Name, rRec2.Name))
		return 0;
	if(!sstreq(rRec1.Code, rRec2.Code))
		return 0;
	{
		SString buf1, buf2;
		if(!Helper_IsEqExField(rRec1, rRec2, LOCEXSTR_ZIP, buf1, buf2))
			return 0;
		else if(!Helper_IsEqExField(rRec1, rRec2, LOCEXSTR_SHORTADDR, buf1, buf2))
			return 0;
		else if(!Helper_IsEqExField(rRec1, rRec2, LOCEXSTR_FULLADDR, buf1, buf2))
			return 0;
		else if(!Helper_IsEqExField(rRec1, rRec2, LOCEXSTR_PHONE, buf1, buf2))
			return 0;
		else if(!Helper_IsEqExField(rRec1, rRec2, LOCEXSTR_EMAIL, buf1, buf2))
			return 0;
		else if(!Helper_IsEqExField(rRec1, rRec2, LOCEXSTR_CONTACT, buf1, buf2))
			return 0;
		for(int id = 1; id <= MAX_DLVRADDRFLDS; id++) {
			if(!Helper_IsEqExField(rRec1, rRec2, id + LOCEXSTR_EXTFLDSOFFS, buf1, buf2))
				return 0;
		}
	}
	#undef CMP_FLD
	return 1;
}

//static
int FASTCALL LocationCore::GetExField(const LocationTbl::Rec * pRec, int fldId, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(pRec && pRec->Tail[0]) {
		if(oneof6(fldId, LOCEXSTR_ZIP, LOCEXSTR_SHORTADDR, LOCEXSTR_FULLADDR, LOCEXSTR_EMAIL, LOCEXSTR_PHONE, LOCEXSTR_CONTACT) || fldId > LOCEXSTR_EXTFLDSOFFS) {
			const SString temp_buf = pRec->Tail;
			ok = PPGetExtStrData(fldId, temp_buf, rBuf);
		}
	}
	return ok;
}

//static
int FASTCALL LocationCore::SetExField(LocationTbl::Rec * pRec, int fldId, const char * pBuf)
{
	int    ok = -1;
	if(oneof6(fldId, LOCEXSTR_ZIP, LOCEXSTR_SHORTADDR, LOCEXSTR_FULLADDR, LOCEXSTR_EMAIL, LOCEXSTR_PHONE, LOCEXSTR_CONTACT) || fldId > LOCEXSTR_EXTFLDSOFFS) {
		SString temp_buf = pRec->Tail;
		ok = PPPutExtStrData(fldId, temp_buf, pBuf);
		temp_buf.CopyTo(pRec->Tail, sizeof(pRec->Tail));
	}
	return ok;
}

SLAPI LocationCore::LocationCore() : LocationTbl()
{
	P_Eac = (CConfig.Flags2 & CCFLG2_INDEXEADDR) ? new EAddrCore : 0;
}

SLAPI LocationCore::~LocationCore()
{
	ZDELETE(P_Eac);
}

int SLAPI LocationCore::Search(PPID id, LocationTbl::Rec * b)
{
	return SearchByID(this, PPOBJ_LOCATION, id, b);
}

int SLAPI LocationCore::GetListByCode(PPID locTyp, const char * pCode, PPIDArray * pIdList)
{
	int    ok = -1;
	char   code[32];
	strip(STRNSCPY(code, pCode));
	if(code[0]) {
		LocationTbl::Key5 k5;
		BExtQuery q(this, 5, 16);
		q.select(this->ID, this->Code, this->Type, 0).where(this->Code == code && this->Type == locTyp);
		MEMSZERO(k5);
		STRNSCPY(k5.Code, pCode);
		k5.Counter = -MAXLONG;
		for(q.initIteration(0, &k5, spGt); q.nextIteration() > 0;)
			if(data.Type == locTyp && stricmp(data.Code, pCode) == 0) {
				CALLPTRMEMB(pIdList, addUnique(data.ID));
				ok = 1;
			}
	}
	return ok;
}

int SLAPI LocationCore::SearchCode(PPID locTyp, const char * pCode, PPID * pID, LocationTbl::Rec * pRec)
{
	int    ok = -1;
	char   code[32];
	strip(STRNSCPY(code, pCode));
	if(code[0]) {
		LocationTbl::Key5 k5;
		BExtQuery q(this, 5, 16);
		q.selectAll().where(this->Code == code && this->Type == locTyp);
		MEMSZERO(k5);
		STRNSCPY(k5.Code, pCode);
		k5.Counter = -MAXLONG;
		for(q.initIteration(0, &k5, spGt); ok < 0 && q.nextIteration() > 0;)
			if(data.Type == locTyp && stricmp(data.Code, pCode) == 0) {
				PPSetObjError(PPERR_DUPSYMB, PPOBJ_LOCATION, data.ID);
				ASSIGN_PTR(pID, data.ID);
				copyBufTo(pRec);
				ok = 1;
			}
	}
	return ok;
}

static SString & FormatCode(const char * pPrfx, const char * pSfx, int len, long n, SString & rBuf)
{
	return rBuf.Z().Cat(pPrfx).CatLongZ(n, len).Cat(pSfx);
}

int SLAPI LocationCore::Helper_GetCodeByTempl(const char * pPrfx, const char * pSfx, int len,
	long low, long upp, int addChkDig, SString & rCode)
{
	int    ok = 0;
	SString buffer;
	SString bound_buf, val_str;
	const  size_t code_len = (pPrfx ? sstrlen(pPrfx) : 0) + (pSfx ? sstrlen(pSfx) : 0) + len;
	long   prev_val = (low > 0) ? (low-1) : 0;
	long   using_val = 0;
	LocationTbl::Key5 k5, k_low, k_upp;
	FormatCode(pPrfx, pSfx, (int)len, low, bound_buf);
	if(addChkDig)
		bound_buf.CatChar('0');
	STRNSCPY(k_low.Code, bound_buf);
	FormatCode(pPrfx, pSfx, (int)len, upp, bound_buf);
	if(addChkDig)
		bound_buf.CatChar('9');
	STRNSCPY(k_upp.Code, bound_buf);
	BExtQuery q(this, 5, 128);
	q.select(this->Code, 0).where(this->Code >= k_low.Code && this->Code <= k_upp.Code);
	k5 = k_low;
	for(q.initIteration(0, &k5, spGe); !ok && q.nextIteration() > 0;) {
		char   temp_buf[128];
		STRNSCPY(temp_buf, data.Code);
		if(sstrlen(temp_buf) == (code_len+BIN(addChkDig))) {
			val_str = temp_buf+sstrlen(pPrfx);
			val_str.Trim(len);
			long   cur_val = val_str.ToLong();
			if(prev_val && cur_val > (prev_val+1)) {
				using_val = prev_val+1;
				ok = 1;
			}
			prev_val = cur_val;
		}
	}
	if(!ok && prev_val < upp) {
		using_val = prev_val+1;
		ok = 1;
	}
	if(ok && using_val) {
		FormatCode(pPrfx, pSfx, (int)len, using_val, buffer);
		if(addChkDig)
			AddBarcodeCheckDigit(buffer);
		if(SearchCode(LOCTYP_ADDRESS, buffer, 0) > 0)
			ok = 0;
		else
			ok = 1;
	}
	rCode = buffer;
	return ok;
}

int SLAPI LocationCore::GetCodeByTemplate(const char * pTempl, SString & rBuf)
{
	rBuf.Z();

	int    ok = -1;
	size_t x_len = 0, r_len = 0;
	double low = 0, upp = 0;
	char   pfx[32], t[32];
	char * c = pfx;
	char * p_dup_templ = newStr(pTempl);
	int    to_add_chkdig = 0;
	SString temp_buf;
	strip(p_dup_templ);
	size_t templ_len = sstrlen(p_dup_templ);
	if(p_dup_templ[templ_len-1] == '^') {
		p_dup_templ[--templ_len] = 0;
		to_add_chkdig = 1;
	}
	char * x, * p = strip(p_dup_templ);
	if(*p) {
		memzero(pfx, sizeof(pfx));
		while(*p) {
			if(isdec(*p))
				*c++ = *p++;
			else if(*p == '%') {
				SString sfx, code;
				x_len = sstrlen(pfx);
				for(++p, x = t; isdec(*p);)
					*x++ = *p++;
				*x = 0;
				r_len = atoi(t);
				if(*p == '[') {
					for(++p, x = t; *p && *p != ']';)
						*x++ = *p++;
					if(*p == ']')
						p++;
					*x = 0;
					strtorrng(t, &low, &upp);
				}
				for(; *p != 0; p++)
					if(*p != '\\')
						sfx.CatChar(*p);
				if(low <= 0)
					low = 1;
				if(upp <= 0 || upp > (fpow10i(r_len) - 1))
					upp = fpow10i(r_len) - 1;
				if(Helper_GetCodeByTempl(pfx, sfx, (int)r_len, (long)low, (long)upp, to_add_chkdig, code)) {
					(rBuf = code).Trim(16);
					ok = 1;
				}
				break;
			}
			else
				break;
		}
	}
	if(ok < 0) {
		rBuf.Z().CatCharN('0', x_len + r_len);
		ok = 0;
	}
	delete p_dup_templ;
	return ok;
}

int SLAPI LocationCore::InitEnum(PPID locTyp, PPID parentID, int flags, long * pHandle)
{
	DBQ  * dbq = &(this->Type == locTyp);
	int    idx = 2;
	union {
		LocationTbl::Key2 k2;
		LocationTbl::Key3 k3;
	} k;
	MEMSZERO(k);
	k.k2.Type = (int16)locTyp;
	if(!(flags & eoIgnoreParent)) {
		if(flags & eoParentAsOwner) {
			idx = 3;
			k.k3.OwnerID = parentID;
			dbq = & (*dbq && this->OwnerID == parentID);
		}
		else {
			k.k2.ParentID = parentID;
			dbq = & (*dbq && this->ParentID == parentID);
		}
	}
	BExtQuery * q = new BExtQuery(this, idx);
	q->selectAll();
	q->where(*dbq);
	q->initIteration(0, &k, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

SEnumImp * SLAPI LocationCore::Enum(PPID locTyp, PPID parentID, int options)
{
	long   h = -1;
	return InitEnum(locTyp, parentID, options, &h) ? new PPTblEnum <LocationCore>(this, h) : 0;
}

int SLAPI LocationCore::Add(PPID * pID, LocationTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		long   c = MAXLONG;
		THROW(tra);
		if(search(1, &c, spLt))
			c++;
		else
			c = 1;
		pRec->Counter = c;
		THROW(AddObjRecByID(this, PPOBJ_LOCATION, pID, pRec, 0));
		if(CConfig.Flags2 & CCFLG2_INDEXEADDR) {
			SString temp_buf;
			GetExField(pRec, LOCEXSTR_PHONE, temp_buf);
			PPObjID objid;
			objid.Set(PPOBJ_LOCATION, *pID);
			THROW(IndexPhone(temp_buf, &objid, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI LocationCore::SearchMaxLike(const LocationTbl::Rec * pRec, long flags, PPID * pID, LocationTbl::Rec * pLikeRec)
{
	int    ok = -1;
	PPIDArray bycode_list;
	if(GetListByCode(pRec->Type, pRec->Code, &bycode_list) > 0) {
		for(uint i = 0; ok < 0 && i < bycode_list.getCount(); i++) {
			const PPID cid = bycode_list.get(i);
			if(cid != pRec->ID) {
				LocationTbl::Rec cod_rec;
				if(Search(cid, &cod_rec) > 0) {
					if(oneof2(pRec->Type, LOCTYP_WAREHOUSE, LOCTYP_WAREHOUSEGROUP)) {
						ASSIGN_PTR(pID, cid);
						ASSIGN_PTR(pLikeRec, cod_rec);
						ok = 1;
					}
					else if(oneof4(pRec->Type, LOCTYP_DIVISION, LOCTYP_WHZONE, LOCTYP_WHCOLUMN, LOCTYP_WHCELL)) {
						if(pRec->ParentID == cod_rec.ParentID) {
							ASSIGN_PTR(pID, cid);
							ASSIGN_PTR(pLikeRec, cod_rec);
							ok = 1;
						}
					}
				}
			}
		}
	}
	return ok;
}

//static
int SLAPI LocationCore::GetTypeDescription(int locType, LocTypeDescr * pDescr)
{
	int    ok = 1;
	LocTypeDescr d;
	d.LocType = locType;
	d.ParentLocType = 0;
	d.Flags = 0;
	d.Name = 0;
	switch(locType) {
		case LOCTYP_WAREHOUSE: d.ParentLocType = LOCTYP_WAREHOUSEGROUP; break;
		case LOCTYP_DIVISION: d.ParentLocType = LOCTYP_WAREHOUSE; break;
		case LOCTYP_WAREHOUSEGROUP: d.ParentLocType = LOCTYP_WAREHOUSEGROUP; break;
		case LOCTYP_ADDRESS: d.ParentLocType = 0; break;
		case LOCTYP_WHZONE:
			d.ParentLocType = LOCTYP_WAREHOUSE;
			d.Flags |= (LocTypeDescr::fParentRequired|LocTypeDescr::fCodeRequired);
			break;
		case LOCTYP_WHCOLUMN:
			d.ParentLocType = LOCTYP_WHZONE;
			d.Flags |= (LocTypeDescr::fParentRequired|LocTypeDescr::fCodeRequired);
			break;
		case LOCTYP_WHCELL:
			d.ParentLocType = LOCTYP_WHCOLUMN;
			d.Flags |= (LocTypeDescr::fParentRequired|LocTypeDescr::fCodeRequired);
			break;
		case LOCTYP_WHCELLAUTOGEN:
			d.ParentLocType = LOCTYP_WHCOLUMN;
			d.Flags |= (LocTypeDescr::fParentRequired);
			break;
		default:
			ok = PPSetError(PPERR_INVLOCTYP);
			break;
	}
	if(ok) {
		SString line_buf, item_buf, id_buf, txt_buf;
		PPLoadText(PPTXT_LOCTYPE, line_buf);
		for(int idx = 0; PPGetSubStr(line_buf, idx, item_buf) > 0; idx++)
			if(item_buf.Divide(',', id_buf, txt_buf) > 0 && id_buf.ToLong() == locType) {
				d.Name = txt_buf;
				break;
			}
	}
	ASSIGN_PTR(pDescr, d);
	return 1;
}

int SLAPI LocationCore::SearchPersonRef(PPID locType, PPID personID, PPID * pID)
{
	int    ok = -1;
	LocationTbl::Key3 k;
	k.Type    = (int16)locType;
	k.OwnerID = personID;
	k.Counter = 0;
	if(search(3, &k, spGe) && k.OwnerID == personID) {
		ASSIGN_PTR(pID, data.ID);
		ok = 1;
	}
	return ok;
}

int SLAPI LocationCore::SearchRef(PPID locType, PPID objType, PPID objID, PPID * pID)
{
	int    ok = -1;
	if(objID) {
		if(objType == PPOBJ_WORLD) {
			BExtQuery q(this, 0, 1);
   	    	q.select(ID, 0L);
			if(locType == 0)
				q.where(CityID == objID);
			else
				q.where(Type == locType && CityID == objID);
			if(q.fetchFirst() > 0) {
				ASSIGN_PTR(pID, data.ID);
				ok = 1;
			}
		}
		else if(objType == PPOBJ_PERSON) {
			if(locType == 0) {
				if(SearchPersonRef(LOCTYP_WAREHOUSE, objID, pID) > 0)
					ok = 1;
				else if(SearchPersonRef(LOCTYP_ADDRESS, objID, pID) > 0)
					ok = 1;
				else if(SearchPersonRef(LOCTYP_DIVISION, objID, pID) > 0)
					ok = 1;
			}
			else if(SearchPersonRef(locType, objID, pID) > 0)
				ok = 1;
		}
		else if(objType == PPOBJ_LOCATION) {
			BExtQuery q(this, 0, 1);
			q.select(ID, 0L).where(this->ParentID == objID);
			if(q.fetchFirst() > 0) {
				ASSIGN_PTR(pID, data.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

// static
int FASTCALL LocationCore::GetAddress(const LocationTbl::Rec & rRec, uint, SString & rBuf)
{
	rBuf.Z();
	if(rRec.Flags & LOCF_MANUALADDR)
		LocationCore::GetExField(&rRec, LOCEXSTR_FULLADDR, rBuf);
	else {
		SString temp_buf;
		LocationCore::GetExField(&rRec, LOCEXSTR_ZIP, temp_buf);
		rBuf.Cat(temp_buf.Strip());
		if(rRec.CityID) {
			PPObjWorld w_obj;
			WorldTbl::Rec w_rec;
			if(w_obj.Fetch(rRec.CityID, &w_rec) > 0)
				rBuf.CatDivIfNotEmpty(' ', 0).Cat(strip(w_rec.Name));
		}
		LocationCore::GetExField(&rRec, LOCEXSTR_SHORTADDR, temp_buf);
		if(temp_buf.NotEmptyS())
			rBuf.CatDivIfNotEmpty(',', 2).Cat(temp_buf);
	}
	return 1;
}

int SLAPI LocationCore::GetAddress_(PPID id, uint f, SString & rBuf)
{
	rBuf.Z();
	return (!id || Search(id) > 0) ? LocationCore::GetAddress(data, f, rBuf) : 0;
}

int SLAPI LocationCore::IndexPhone(const char * pPhone, const PPObjID * pObjId, int doRemove, int use_ta)
{
	int    ok = 1, r = 1;
	PPID   addr_id = 0;
	if(P_Eac) {
		PPTransaction tra(use_ta);
		PPEAddr addr;
		THROW(tra);
		THROW(addr.SetPhone(pPhone));
		THROW(P_Eac->Search(addr, pObjId, &addr_id));
		if(doRemove) {
			if(addr_id)
				THROW(r = P_Eac->Put(&addr_id, 0, pObjId, 0));
		}
		else {
			THROW(r = P_Eac->Put(&addr_id, &addr, pObjId, 0));
		}
		if(r < 0)
			ok = -1;
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI LocationCore::SearchPhoneIndex(const char * pPhone, long options, LongArray & rResultList)
	{ return P_Eac ? P_Eac->SearchPhone(pPhone, options, rResultList) : -1; }

int SLAPI LocationCore::SearchPhoneObjList(const char * pPhone, long options, PPObjIDArray & rList)
{ 
	rList.clear();
	int    ok = -1;
	if(P_Eac) {
		LongArray temp_list;
		if(P_Eac->SearchPhone(pPhone, options, temp_list) > 0 && temp_list.getCount()) {
			for(uint i = 0; i < temp_list.getCount(); i++) {
				EAddrTbl::Rec ea_rec;
				if(GetEAddr(temp_list.get(i), &ea_rec) > 0) {
					rList.Add(ea_rec.LinkObjType, ea_rec.LinkObjID);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI LocationCore::GetEAddr(PPID id, EAddrTbl::Rec * pRec)
	{ return P_Eac ? P_Eac->Search(id, pRec) : -1; }
int SLAPI LocationCore::SearchEAddrByLink(PPID objType, PPID objID, PPIDArray & rResultList)
	{ return P_Eac ? P_Eac->SearchLink(objType, objID, rResultList) : -1; }
int SLAPI LocationCore::SearchEAddrMaxLikePhone(const char * pPhonePattern, long options, LongArray & rResult)
	{ return P_Eac ? P_Eac->SearchMaxLikePhone(pPhonePattern, options, rResult) : -1; }

int SLAPI LocationCore::DumpEaList(const char * pFileName)
{
	int    ok = -1;
    if(P_Eac) {
		SString temp_buf;
		if(pFileName)
			temp_buf = pFileName;
		else {
			PPGetFilePath(PPPATH_OUT, "ealist.txt", temp_buf);
		}
		SFile f_out(temp_buf, SFile::mWrite);
		THROW_SL(f_out.IsValid());
		{
			SString line_buf;
			EAddrTbl::Key1 k1;
			BExtQuery q(P_Eac, 1);
			SString phone_buf;
			q.select(P_Eac->ID, P_Eac->LinkObjType, P_Eac->LinkObjID, P_Eac->Addr, 0L);
			MEMSZERO(k1);
			for(q.initIteration(0, &k1, spFirst); q.nextIteration() > 0;) {
				PPObjID oid;
				oid.Set(P_Eac->data.LinkObjType, P_Eac->data.LinkObjID);
				((PPEAddr *)P_Eac->data.Addr)->GetPhone(phone_buf);
				;
				line_buf.Z().Cat(P_Eac->data.ID).Tab().Cat(phone_buf).Tab().Cat(oid.ToStr(temp_buf));
				GetObjectName(oid.Obj, oid.Id, temp_buf.Z());
				line_buf.Tab().Cat(temp_buf);
				f_out.WriteLine(line_buf.CR());
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI LocationCore::GetFullEaList(StrAssocArray & rList)
{
	int    ok = 1;
	rList.Z();
    if(P_Eac) {
		EAddrTbl::Key1 k1;
		BExtQuery q(P_Eac, 1);
		SString phone_buf;
		q.select(P_Eac->ID, P_Eac->LinkObjType, P_Eac->LinkObjID, P_Eac->Addr, 0L);
		MEMSZERO(k1);
		for(q.initIteration(0, &k1, spFirst); q.nextIteration() > 0;) {
			((PPEAddr *)P_Eac->data.Addr)->GetPhone(phone_buf);
			if(phone_buf.NotEmpty())
				THROW_SL(rList.AddFast(P_Eac->data.ID, phone_buf));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
IMPL_INVARIANT_C(PPEAddr)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(oneof6(Type, tUndef, tIpV4, tIpV4Range, tIpV6, tIpV6Range, tPhone), pInvP);
	if(Type == tPhone) {
		S_ASSERT_P(U.P.InvariantC(0), pInvP);
	}
	S_INVARIANT_EPILOG(pInvP);
}

#define EADDR_PHONE_MAXCHAR (sizeof(PPEAddr::Phone) * 2)

IMPL_INVARIANT_C(PPEAddr::Phone)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(sizeof(A) == 12, pInvP);
	for(uint i = 0; i < sizeof(A); i++) {
		uint8 p = A[i];
		uint8 d1 = (p & 0xf0) >> 4;
		uint8 d2 = (p & 0x0f);
		char c1 = (d1 >= 0 && d1 <= 9) ? (d1 + '0') : ((d1 == 0x0A) ? '+' : ((d1 == 0x0B) ? 't' : (d1 == 0x0C ? 'x' : 0x0F)));
		char c2 = (d2 >= 0 && d2 <= 9) ? (d2 + '0') : ((d2 == 0x0A) ? '+' : ((d2 == 0x0B) ? 't' : (d2 == 0x0C ? 'x' : 0x0F)));
		S_ASSERT_P(c1 != 0x0F && c2 != 0x0F, pInvP);
	}
	S_INVARIANT_EPILOG(pInvP);
}

SString & FASTCALL PPEAddr::Phone::ToStr(long fmt, SString & rBuf) const
{
	assert(EADDR_PHONE_MAXCHAR % 2 == 0);

	rBuf.Z();
	int    zero = 1;
	for(size_t i = 0; i < EADDR_PHONE_MAXCHAR; i+=2) {
		uint8 p = A[i/2];
		uint8 d1 = (p & 0xf0) >> 4;
		uint8 d2 = (p & 0x0f);
		char c1 = (d1 >= 0 && d1 <= 9) ? (d1 + '0') : ((d1 == 0x0A) ? '+' : ((d1 == 0x0B) ? 't' : (d1 == 0x0C ? 'x' : 0x0F)));
		char c2 = (d2 >= 0 && d2 <= 9) ? (d2 + '0') : ((d2 == 0x0A) ? '+' : ((d2 == 0x0B) ? 't' : (d2 == 0x0C ? 'x' : 0x0F)));
		assert(c1 != 0x0F && c2 != 0x0F);
		if(c1 != '0' || !zero) {
			rBuf.CatChar(c1);
			zero = 0;
		}
		if(c2 != '0' || !zero) {
			rBuf.CatChar(c2);
			zero = 0;
		}
	}
	return rBuf;
}

static uint SLAPI Helper_GeneratePhoneCheckNumber(const char * pOrgPhone, const char * pAddedumCode, uint checkResult)
{
	// Несколько вариантов простого делителя с количеством знаков, равным разрядности требуемого значения (5)
	static const uint32 prime_div_variants[] = { 98387, 86017, 87359, 79823, 75337 };
	const  uint addendum = 54167; // Фиксированное слагаемое для контрольной суммы, если она меньше 10000
    uint   result = 0;
	SString line_buf;
	PPEAddr::Phone::NormalizeStr(pOrgPhone, 0, line_buf);
    line_buf.Cat(pAddedumCode);
    size_t p = 0;
    while(line_buf.StrChr(' ', &p)) {
        line_buf.Excise(p, 1);
    }
    while(line_buf.StrChr('\t', &p)) {
        line_buf.Excise(p, 1);
    }
    {
		CRC32  c;
		uint32 crc = c.Calc(0, line_buf.ucptr(), line_buf.Len());
		if(crc < 10000)
			crc += addendum;
		if(checkResult) {
			for(uint i = 0; !result && i < SIZEOFARRAY(prime_div_variants); i++) {
				if(checkResult == (crc % prime_div_variants[i]))
                    result = checkResult;
			}
		}
		else {
			const uint pdv_pos = SLS.GetTLA().Rg.GetUniformInt(SIZEOFARRAY(prime_div_variants));
			assert(pdv_pos < SIZEOFARRAY(prime_div_variants));
			if(pdv_pos < SIZEOFARRAY(prime_div_variants)) {
				result = (crc % prime_div_variants[pdv_pos]);
			}
		}
    }
    return result;
}

//static
uint FASTCALL PPEAddr::Phone::GenerateCheckNumber(const char * pOrgPhone, const char * pAddedumCode)
{
	return Helper_GeneratePhoneCheckNumber(pOrgPhone, pAddedumCode, 0);
}

//static
int SLAPI PPEAddr::Phone::VerifyCheckNumber(const char * pOrgPhone, const char * pAddedumCode, uint checkedNumber)
{
	uint r = Helper_GeneratePhoneCheckNumber(pOrgPhone, pAddedumCode, checkedNumber);
	return BIN(r == checkedNumber);
}

//static
SString & FASTCALL PPEAddr::Phone::NormalizeStr(const char * pOrgPhone, long flags, SString & rResult)
{
	static StringSet ext_prefix_list;
	SString temp_buf;
	if(ext_prefix_list.getDataLen() == 0) {
		ext_prefix_list.add("exten");
		ext_prefix_list.add("ext");
		ext_prefix_list.add("x");
		ext_prefix_list.add("доб"); // this source file has UTF-8 codepage (see header of file above)
	}
	rResult.Z();
	for(const uchar * p = (const uchar *)pOrgPhone; *p != 0; p++) {
		uint  ext_prefix_width = 0; // Если !0 то надо обработать добавочный номер после (ext_prefix_width) символов префикса
		const uchar c = *p;
		if(isdec(c)) {
			if(flags & nsfPlus && c == '8' && rResult.Len() == 0)
				rResult.CatChar('+').CatChar('7');
			else
				rResult.CatChar(c);
		}
		else if(oneof2(c, 't', 'T'))
			rResult.CatChar('t');
		else if(c == '+' && rResult.Len() == 0) {
			if(!(flags & nsfPlus)) {
				if(p[1] == '7') { // Специальный случай - заменяем лидирующий +7 на 8 (Россия) поскольку Asterisk принимает коды именно так
					rResult.CatChar('8');
					p++;
				}
				else
					rResult.CatChar(c);
			}
			else
				rResult.CatChar(c);
		}
		else {
			if(oneof2(c, ',', '#') && (oneof4(p[1], '.', ':', ' ', '\t') || isdec(p[1])))
				ext_prefix_width = isdec(p[1]) ? 1 : 2;
			else {
				for(uint ssp = 0; ext_prefix_list.get(&ssp, temp_buf);) {
					size_t pl = temp_buf.Len();
					if(strncmp(temp_buf, (const char *)p, pl) == 0 && (oneof4(p[pl+1], '.', ':', ' ', '\t') || isdec(p[pl+1]))) {
						ext_prefix_width = isdec(p[pl+1]) ? pl : (pl+1);
						break;
					}
				}
			}
			if(!ext_prefix_width && !oneof9(c, ' ', '-', '(', ')', 't', 'T', 'w', 'W', '+')) {
				//
				// Как только встречаем символ, отличный от перечисленных - выходим: далее может следовать текст
				// комментария к телефону.
				//
				break;
			}
		}
		//
		if(ext_prefix_width) {
			p += ext_prefix_width;
			while(oneof2(*p, ' ', '\t'))
				p++;
			if(isdec(*p)) {
				rResult.CatChar('x');
				do {
					rResult.CatChar(*p++);
				} while(isdec(*p));
			}
			break; // Не зависимо от того, был добавочный номер правильный или нет, выходим. Добавочный номер - может быть только в конце.
		}
	}
	return rResult;
}

int FASTCALL PPEAddr::Phone::FromStr(const char * pStr)
{
	assert(EADDR_PHONE_MAXCHAR % 2 == 0);

	int    ok = 1;
	SString & r_src_str = SLS.AcquireRvlStr(); // @v9.9.11
	(r_src_str = pStr).Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower(); // @v9.9.11
	SString & r_temp_buf = SLS.AcquireRvlStr();
	NormalizeStr(r_src_str, 0, r_temp_buf);
	size_t diff = r_temp_buf.Strip().Len();
	diff = (EADDR_PHONE_MAXCHAR > diff) ? (EADDR_PHONE_MAXCHAR - diff) : 0;
	if(diff)
		r_temp_buf.PadLeft(diff, '0');
	if(r_temp_buf.Len() == EADDR_PHONE_MAXCHAR) {
		//assert(r_temp_buf.Len() == EADDR_PHONE_MAXCHAR);
		for(size_t i = 0; i < EADDR_PHONE_MAXCHAR; i+=2) {
			const char c1 = r_temp_buf.C(i);
			const char c2 = r_temp_buf.C(i+1);
			const uint8 d1 = isdec(c1) ? (c1 - '0') : ((c1 == '+') ? 0x0A : ((c1 == 't') ? 0x0B : ((c1 == 'x') ? 0x0C: 0x0F)));
			const uint8 d2 = isdec(c2) ? (c2 - '0') : ((c2 == '+') ? 0x0A : ((c2 == 't') ? 0x0B : ((c2 == 'x') ? 0x0C: 0x0F)));
			assert(d1 != 0x0F && d2 != 0x0F);
			A[i/2] = (d1 << 4) | d2;
		}
	}
	else {
		ok = PPSetError(PPERR_PHNNBMEXCMAXLEN, r_temp_buf);
	}
	return ok;
}
//
//
//
int FASTCALL PPEAddr::Init(int type)
{
	int    ok = 1;
	if(oneof6(type, tUndef, tIpV4, tIpV4Range, tIpV6, tIpV6Range, tPhone)) {
		Type = type;
		Reserve = 0;
		Flags = 0;
		MEMSZERO(U);
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPEAddr::Set(uint32 ip4)
{
	int    ok = 1;
	if(Init(tIpV4))
		U.I4.A = ip4;
	else
		ok = 0;
	return ok;
}

int PPEAddr::Set(uint32 ip4Low, uint32 ip4Upp)
{
	int    ok = 1;
	if(Init(tIpV4Range)) {
		U.I4R.Low = ip4Low;
		U.I4R.Upp = ip4Upp;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL PPEAddr::SetPhone(const char * pPhone)
{
	return Init(tPhone) ? U.P.FromStr(pPhone) : 0;
}

int FASTCALL PPEAddr::GetPhone(SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	if(Type == tPhone)
		U.P.ToStr(0, rBuf);
	else
		ok = 0;
	return ok;
}

int FASTCALL PPEAddr::operator == (const PPEAddr & rS) const
{
	int    eq = 0;
	if(rS.Type == Type) {
		switch(Type) {
			case tUndef:
				break;
			case tIpV4:
				if(U.I4.A == rS.U.I4.A)
					eq = 1;
				break;
			case tIpV4Range:
				if(U.I4R.Low == rS.U.I4R.Low && U.I4R.Upp == rS.U.I4R.Upp)
					eq = 1;
				break;
			case tIpV6:
				if(memcmp(&U.I6, &rS.U.I6, sizeof(U.I6)) == 0)
					eq = 1;
				break;
			case tIpV6Range:
				if(memcmp(&U.I6R, &rS.U.I6R, sizeof(U.I6R)) == 0)
					eq = 1;
				break;
			case tPhone:
				if(memcmp(&U.P, &rS.U.P, sizeof(U.P)) == 0)
					eq = 1;
				break;
		}
	}
	return eq;
}

int PPEAddr::IsEmpty() const
{
	int    yes = 0;
	if(Type == tUndef)
		yes = 1;
	else if(Type == tIpV4) {
		if(U.I4.A == 0)
			yes = 1;
	}
	else if(Type == tIpV4Range) {
		if(U.I4R.Low == 0 && U.I4R.Upp == 0)
			yes = 1;
	}
	else if(Type == tIpV6) {
		if(ismemzero(U.I6.A, sizeof(U.I6.A)))
			yes = 1;
	}
	else if(Type == tIpV6Range) {
		if(ismemzero(U.I6R.Low, sizeof(U.I6R.Low)) && ismemzero(U.I6R.Upp, sizeof(U.I6R.Upp)))
			yes = 1;
	}
	else if(Type == tPhone) {
		if(ismemzero(U.P.A, sizeof(U.P.A)))
			yes = 1;
	}
	else
		yes = 1;
	return yes;
}
//
//
//
SLAPI PPEAddrArray::PPEAddrArray() : TSVector <PPEAddr> () // @v9.9.3 TSArray-->TSVector
{
}

int FASTCALL PPEAddrArray::AddPhone(const char * pPhone)
{
	int    ok = 1;
	if(!isempty(pPhone)) {
		PPEAddr pattern;
		pattern.SetPhone(pPhone);
		insert(&pattern);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPEAddrArray::SearchPhone(const char * pPhone, long options, LongArray * pPosList) const
{
	int    ok = 0;
	if(getCount()) {
		PPEAddr pattern;
		pattern.SetPhone(pPhone);
		for(uint i = 0; i < getCount(); i++) {
			const PPEAddr & r_item = at(i);
			if(r_item == pattern) {
				CALLPTRMEMB(pPosList, add((long)i));
				ok = 1;
			}
		}
	}
	return ok;
}
//
//
//
SLAPI EAddrCore::EAddrCore() : EAddrTbl()
{
}

int SLAPI EAddrCore::Search(PPID id, EAddrTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_EADDR, id, pRec);
}

int SLAPI EAddrCore::Put(PPID * pID, PPEAddr * pAddr, const PPObjID * pObjId, int use_ta)
{
	int    ok = 1, r = 0;
	{
		EAddrTbl::Rec rec, prev_rec;
		MEMSZERO(rec);
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(Search(*pID, &prev_rec) > 0);
			if(pAddr && !pAddr->IsEmpty()) {
				rec.ID = *pID;
				memcpy(rec.Addr, pAddr, sizeof(rec.Addr));
				if(pObjId) {
					rec.LinkObjType = pObjId->Obj;
					rec.LinkObjID = pObjId->Id;
				}
				THROW(r = UpdateByID_Cmp(this, PPOBJ_EADDR, *pID, &rec, 0));
				if(r < 0)
					ok = -1;
			}
			else {
				THROW(RemoveByID(this, *pID, 0));
			}
		}
		else {
			if(pAddr && !pAddr->IsEmpty()) {
				memcpy(rec.Addr, pAddr, sizeof(rec.Addr));
				if(pObjId) {
					rec.LinkObjType = pObjId->Obj;
					rec.LinkObjID = pObjId->Id;
				}
				THROW(AddObjRecByID(this, PPOBJ_EADDR, pID, &rec, 0));
			}
			else
				ok = -1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/* @construction
int SLAPI EAddrCore::SearchIpV4(uint32 ip, long options, LongArray & rResult)
{
	PPEAddr addr;
	THROW(addr.Set(ip));
}
*/

int SLAPI EAddrCore::Search(const PPEAddr & rAddr, const PPObjID * pObjId, PPID * pID)
{
	int    ok = 0;
	EAddrTbl::Key1 k1;
	MEMSZERO(k1);
	memcpy(k1.Addr, &rAddr, sizeof(rAddr));
	if(pObjId) {
		k1.LinkObjType = pObjId->Obj;
		k1.LinkObjID = pObjId->Id;
	}
	if(search(1, &k1, spEq)) {
		ASSIGN_PTR(pID, data.ID);
		ok = 1;
	}
	else {
		ASSIGN_PTR(pID, 0);
		ok = PPDbSearchError();
	}
	return ok;
}

int SLAPI EAddrCore::SearchLink(PPID objType, PPID objID, LongArray & rResult)
{
	rResult.clear();

	int    ok = -1;
    EAddrTbl::Key2 k2;
    MEMSZERO(k2);
    k2.LinkObjType = objType;
    k2.LinkObjID = objID;
    if(search(2, &k2, spGe) && data.LinkObjType == objType && data.LinkObjID == objID) do {
		THROW_SL(rResult.add(data.ID));
		ok = 1;
    } while(search(2, &k2, spNext) && data.LinkObjType == objType && data.LinkObjID == objID);
    CATCHZOK
    return ok;
}

int SLAPI EAddrCore::SearchPhone(const char * pPhone, long options, LongArray & rResult)
{
	rResult.clear();

	int    ok = -1;
	PPEAddr addr;
	EAddrTbl::Key1 k1;
	THROW(addr.SetPhone(pPhone));
	MEMSZERO(k1);
	memcpy(k1.Addr, &addr, sizeof(addr));
	if(search(1, &k1, spGe) && *(PPEAddr *)data.Addr == addr) do {
		THROW_SL(rResult.add(data.ID));
		ok = 1;
	} while(search(1, &k1, spNext) && *(PPEAddr *)data.Addr == addr);
	CATCHZOK
	return ok;
}

int SLAPI EAddrCore::SearchMaxLikePhone(const char * pPhonePattern, long options, LongArray & rResult)
{
	int    ok = -1;
	EAddrTbl::Key1 k1;
	BExtQuery q(this, 1);
	SString phone_buf;
	rResult.clear();
	q.select(ID, LinkObjType, LinkObjID, Addr, 0L);
	MEMSZERO(k1);
	for(q.initIteration(0, &k1, spFirst); q.nextIteration() > 0;) {
		phone_buf = 0;
		((PPEAddr *)data.Addr)->GetPhone(phone_buf);
		//if(phone_buf.CmpPrefix(pPhonePattern, 1) == 0) {
		if(phone_buf.Search(pPhonePattern, 0, 1, 0)) {
			THROW_SL(rResult.add(data.ID));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
