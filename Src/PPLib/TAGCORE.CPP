// TAGCORE.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000-2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2019
// @codepage windows-1251
// @Kernel
// Теги объектов
//
#include <pp.h>
#pragma hdrstop
//
//static
SString & FASTCALL ObjTagItem::GetTypeString(int tagType, PPID enumId, SString & rBuf)
{
	rBuf.Z();
	const char * p_str_signature = 0;
	switch(tagType) {
		case OTTYP_GROUP: p_str_signature = "group"; break;
		case OTTYP_BOOL: p_str_signature = "typboolean"; break;
		case OTTYP_STRING: p_str_signature = "typstring"; break;
		case OTTYP_NUMBER: p_str_signature = "typnumeric"; break;
		case OTTYP_ENUM: p_str_signature = "typenumeration"; break;
		case OTTYP_INT: p_str_signature = "typinteger"; break;
		case OTTYP_DATE: p_str_signature = "date"; break;
		case OTTYP_IMAGE: p_str_signature = "typimage"; break;
		case OTTYP_TIMESTAMP: p_str_signature = "typtimestamp"; break;
		case OTTYP_GUID: rBuf = "GUID"; break;
		case OTTYP_OBJLINK:
			if(enumId)
				GetObjectTitle(enumId, rBuf);
			else
				p_str_signature = "object";
			break;
	}
	if(p_str_signature)
		PPLoadString(p_str_signature, rBuf);
	return rBuf;
}

SLAPI ObjTagItem::ObjTagItem()
{
	THISZERO();
}

SLAPI ObjTagItem::ObjTagItem(const ObjTagItem & rS)
{
	THISZERO();
	Copy(rS);
}

SLAPI ObjTagItem::~ObjTagItem()
{
	Destroy();
}

int FASTCALL ObjTagItem::Init(PPID tagID)
{
	Destroy();
	if(tagID) {
		PPObjTag tagobj;
		PPObjectTag tagrec;
		if(tagobj.Fetch(tagID, &tagrec) > 0) {
			TagID       = tagID;
			TagDataType = tagrec.TagDataType;
			TagEnumID   = tagrec.TagEnumID;
		}
		else
			return PPSetError(PPERR_INVTAGID);
	}
	return 1;
}

int FASTCALL ObjTagItem::GetInt(long * pVal) const
{
	int    ok = 1;
	long   val = 0;
	switch(TagDataType) {
		case OTTYP_BOOL: val = Val.IntVal; break;
		case OTTYP_STRING: strtolong(Val.PStr, &val); break;
		case OTTYP_NUMBER: val = (long)round(Val.RealVal, 0); break;
		case OTTYP_ENUM: val = Val.IntVal; break;
		case OTTYP_INT:  val = Val.IntVal; break;
		case OTTYP_OBJLINK: val = Val.IntVal; break;
		case OTTYP_DATE: val = (long)Val.DtVal.v; break;
		case OTTYP_TIMESTAMP: val = (long)Val.DtmVal.d.v; break;
		default:
			ok = -1;
			break;
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

int FASTCALL ObjTagItem::GetReal(double * pVal) const
{
	int    ok = 1;
	double val = 0.0;
	switch(TagDataType) {
		case OTTYP_BOOL: val = Val.IntVal; break;
		case OTTYP_STRING: strtodoub(Val.PStr, &val); break;
		case OTTYP_NUMBER: val = Val.RealVal; break;
		case OTTYP_ENUM: val = Val.IntVal; break;
		case OTTYP_INT:  val = Val.IntVal; break;
		case OTTYP_OBJLINK: val = Val.IntVal; break;
		case OTTYP_DATE: val = (double)Val.DtVal.v; break;
		case OTTYP_TIMESTAMP: val = (OleDate)Val.DtmVal; break;
		default:
			ok = -1;
			break;
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

int FASTCALL ObjTagItem::GetDate(LDATE * pVal) const
{
	int    ok = 1;
	LDATE  val = ZERODATE;
	switch(TagDataType) {
		case OTTYP_STRING: strtodate(Val.PStr, DATF_DMY, &val); break;
		case OTTYP_DATE: val = Val.DtVal; break;
		case OTTYP_TIMESTAMP: val = Val.DtmVal.d; break;
		default:
			ok = -1;
			break;
	}
	ASSIGN_PTR(pVal, val);
	return ok;
}

int SLAPI ObjTagItem::GetEnumData(long * pID, long * pParentID, SString * pTxt, SString * pSymb) const
{
	int    ok = 0;
	PPID   item_id = 0;
	PPID   parent_id = 0;
	ASSIGN_PTR(pTxt, 0);
	ASSIGN_PTR(pSymb, 0);
	if(TagDataType == OTTYP_ENUM) {
		item_id = Val.IntVal;
		if(pTxt || pParentID || pSymb) {
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			if(tag_obj.Fetch(TagID, &tag_rec) > 0) {
				if(tag_rec.TagEnumID && item_id) {
					if(IS_DYN_OBJTYPE(tag_rec.TagEnumID)) {
						ReferenceTbl::Rec rec;
						if(PPRef->GetItem(tag_rec.TagEnumID, item_id, &rec) > 0) {
							ASSIGN_PTR(pTxt, rec.ObjName);
							ASSIGN_PTR(pSymb, rec.Symb);
							parent_id = rec.Val2;
							ok = 1;
						}
					}
					else if(pTxt) {
						GetObjectName(tag_rec.TagEnumID, item_id, *pTxt);
						ok = 2;
					}
				}
				else {
					ASSIGN_PTR(pTxt, "zero");
				}
			}
		}
	}
	ASSIGN_PTR(pID, item_id);
	ASSIGN_PTR(pParentID, parent_id);
	return ok;
}

int FASTCALL ObjTagItem::GetStr(SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	switch(TagDataType) {
		case OTTYP_BOOL: rBuf = Val.IntVal ? "True" : "False"; break;
		case OTTYP_STRING:
		case OTTYP_GUID:
		case OTTYP_IMAGE: rBuf = Val.PStr; break;
		case OTTYP_DATE: rBuf.Cat(Val.DtVal, DATF_DMY|DATF_CENTURY); break;
		case OTTYP_TIMESTAMP: rBuf.Cat(Val.DtmVal, DATF_DMY|DATF_CENTURY); break;
		case OTTYP_NUMBER:
			if((long)Val.RealVal == Val.RealVal)
				rBuf.Cat((long)Val.RealVal);
			else
				rBuf.Cat(Val.RealVal, SFMT_MONEY);
			break;
		case OTTYP_OBJLINK:
		case OTTYP_ENUM:
			{
				PPObjTag tag_obj;
				PPObjectTag tag_rec;
				if(tag_obj.Fetch(TagID, &tag_rec) > 0) {
					SString objname;
					if(tag_rec.TagEnumID && Val.IntVal) {
						GetObjectName(tag_rec.TagEnumID, Val.IntVal, objname);
						if(!objname.NotEmptyS())
							ideqvalstr(Val.IntVal, objname);
					}
					else
						objname = "zero";
					rBuf.Cat(objname);
				}
			}
			break;
		default:
			ok = -1;
			break;
	}
	return ok;
}

int FASTCALL ObjTagItem::GetGuid(S_GUID * pGuid) const
{
	int    ok = 0;
	if(oneof2(TagDataType, OTTYP_GUID, OTTYP_STRING)) { // @v9.0.0 OTTYP_STRING
		CALLPTRMEMB(pGuid, FromStr(Val.PStr));
		ok = 1;
	}
	else {
		PPSetError(PPERR_INVOBJTAGTYPE, "GUID");
		CALLPTRMEMB(pGuid, Z());
	}
	return ok;
}

int FASTCALL ObjTagItem::GetTimestamp(LDATETIME * pDtm) const
{
	int    ok = 1;
	LDATETIME dtm = ZERODATETIME;
	switch(TagDataType) {
		case OTTYP_DATE: dtm.d = Val.DtVal; break;
		case OTTYP_TIMESTAMP: dtm = Val.DtmVal; break;
		case OTTYP_STRING: strtodatetime(Val.PStr, &dtm, DATF_ISO8601, TIMF_HMS); break;
		default: ok = -1; break;
	}
	ASSIGN_PTR(pDtm, dtm);
	return ok;
}

int SLAPI ObjTagItem::SetInt(PPID tagID, long lVal)
{
	if(Init(tagID)) {
		if(TagDataType == OTTYP_STRING) {
			char   temp_buf[64];
			ltoa(lVal, temp_buf, 10);
			Val.PStr = newStr(temp_buf);
		}
		else if(TagDataType == OTTYP_NUMBER)
			Val.RealVal = lVal;
		else if(oneof3(TagDataType, OTTYP_BOOL, OTTYP_ENUM, OTTYP_OBJLINK))
			Val.IntVal = lVal;
		else if(TagDataType == OTTYP_DATE)
			Val.DtVal.v = lVal;
		else if(TagDataType == OTTYP_TIMESTAMP) {
			Val.DtmVal.d.v = lVal;
			Val.DtmVal.t.v = 0;
		}
		return 1;
	}
	else
		return 0;
}

int SLAPI ObjTagItem::SetReal(PPID tagID, double rVal)
{
	if(Init(tagID)) {
		if(TagDataType == OTTYP_STRING) {
			char   temp_buf[64];
			realfmt(rVal, SFMT_MONEY, temp_buf);
			Val.PStr = newStr(temp_buf);
		}
		else if(TagDataType == OTTYP_NUMBER)
			Val.RealVal = rVal;
		else if(oneof3(TagDataType, OTTYP_BOOL, OTTYP_ENUM, OTTYP_OBJLINK))
			Val.IntVal = (long)rVal;
		else if(TagDataType == OTTYP_DATE)
			Val.DtVal.v = (long)rVal;
		else if(TagDataType == OTTYP_TIMESTAMP) {
			OleDate od;
			od.v = rVal;
			Val.DtmVal = od;
		}
		return 1;
	}
	else
		return 0;
}

int SLAPI ObjTagItem::SetDate(PPID tagID, LDATE dt)
{
	if(Init(tagID)) {
		if(TagDataType == OTTYP_STRING) {
			char   temp_buf[64];
			datefmt(&dt, DATF_DMY|DATF_CENTURY, temp_buf);
			Val.PStr = newStr(temp_buf);
		}
		else if(TagDataType == OTTYP_NUMBER)
			Val.RealVal = 0;
		else if(oneof3(TagDataType, OTTYP_BOOL, OTTYP_ENUM, OTTYP_OBJLINK))
			Val.IntVal = 0;
		else if(TagDataType == OTTYP_DATE)
			Val.DtVal = dt;
		else if(TagDataType == OTTYP_TIMESTAMP) {
			Val.DtmVal.d = dt;
			Val.DtmVal.t = ZEROTIME;
		}
    	return 1;
	}
	else
		return 0;
}

int SLAPI ObjTagItem::SetStr(PPID tagID, const char * pStrVal)
{
	int    ok = 1;
	THROW(Init(tagID));
	if(oneof3(TagDataType, OTTYP_STRING, OTTYP_GUID, OTTYP_IMAGE)) {
		if(!isempty(pStrVal))
			Val.PStr = newStr(pStrVal);
	}
	else if(TagDataType == OTTYP_NUMBER)
		Val.RealVal = pStrVal ? atof(pStrVal) : 0;
	else if(TagDataType == OTTYP_ENUM) {
		PPID   _id = 0;
		SString text = pStrVal;
		if(text.NotEmptyS()) {
			if(TagEnumID) {
				ReferenceTbl::Rec rec;
				int    r = PPRef->SearchName(TagEnumID, &_id, text, &rec);
				THROW(r);
				if(r < 0) {
					MEMSZERO(rec);
					text.CopyTo(rec.ObjName, sizeof(rec.ObjName));
					PPTransaction tra(-1);
					THROW(tra);
					THROW(PPRef->AddItem(TagEnumID, &_id, &rec, 0));
					THROW(tra.Commit());
				}
			}
		}
		Val.IntVal = _id;
	}
	else if(oneof2(TagDataType, OTTYP_BOOL, OTTYP_OBJLINK))
		Val.IntVal = pStrVal ? atol(pStrVal) : 0;
	else if(TagDataType == OTTYP_DATE)
		strtodate(pStrVal, DATF_DMY, &Val.DtVal);
	else if(TagDataType == OTTYP_TIMESTAMP)
		strtodatetime(pStrVal, &Val.DtmVal, DATF_DMY, TIMF_HMS);
	CATCHZOK
    return ok;
}

int SLAPI ObjTagItem::AddKeyword(PPID tagID, const char * pKeyword)
{
	int    ok = 1;
	if(TagDataType == OTTYP_STRING) {
		if(IsZeroVal()) {
			ok = SetStr(tagID, pKeyword);
		}
		else {
			SString temp_buf;
			StringSet ss(',', Val.PStr);
			for(uint p = 0; ok > 0 && ss.get(&p, temp_buf);) {
				if(temp_buf.Strip().CmpNC(pKeyword) == 0)
					ok = -1;
			}
			if(ok > 0) {
				ss.add(pKeyword);
				(temp_buf = ss.getBuf()).Strip();
				ok = SetStr(tagID, temp_buf);
			}
		}
	}
	else if(TagDataType == 0) {
		if(Init(tagID)) {
			if(TagDataType == OTTYP_STRING) {
				Val.PStr = newStr(pKeyword);
				if(Val.PStr == 0)
					ok = PPSetErrorNoMem();
			}
			else
				ok = PPSetError(PPERR_INVOBJTAGTYPE, "STRING");
		}
	}
	else
		ok = PPSetError(PPERR_INVOBJTAGTYPE, "STRING");
	return ok;
}

int SLAPI ObjTagItem::SetGuid(PPID tagID, const S_GUID_Base * pGuid)
{
	int    ok = 0;
	if(Init(tagID)) {
		if(TagDataType == OTTYP_GUID && pGuid && !pGuid->IsZero()) {
			SString temp_buf;
			pGuid->ToStr(S_GUID::fmtIDL, temp_buf);
			Val.PStr = newStr(temp_buf.cptr());
		}
		ok = 1;
	}
	return ok;
}

int SLAPI ObjTagItem::SetTimestamp(PPID tagID, LDATETIME dtm)
{
	int    ok = 1;
	THROW(Init(tagID));
	switch(TagDataType) {
		case OTTYP_DATE: Val.DtVal = dtm.d; break;
		case OTTYP_TIMESTAMP: Val.DtmVal = dtm; break;
		case OTTYP_STRING:
			{
				SString temp_buf;
				temp_buf.Cat(dtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS);
				Val.PStr = newStr(temp_buf.cptr());
			}
			break;
		default:
			ok = -1;
			break;
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjTagItem::AddReal(double rVal)
{
	int    ok = 1;
	switch(TagDataType) {
		case OTTYP_STRING:
			{
				double prev = Val.PStr ? atof(Val.PStr) : 0.0;
				ZDELETE(Val.PStr);
				char   temp_buf[64];
				realfmt(prev+rVal, SFMT_MONEY, temp_buf);
				Val.PStr = newStr(temp_buf);
			}
			break;
		case OTTYP_NUMBER:
			Val.RealVal += rVal;
			break;
		case OTTYP_INT:
			Val.IntVal += (long)rVal;
			break;
		case OTTYP_DATE:
			if(!Val.DtVal)
				Val.DtVal = getcurdate_();
			else
				Val.DtVal = plusdate(Val.DtVal, R0i(rVal));
			break;
		case OTTYP_TIMESTAMP:
			if(!Val.DtmVal.d) {
				Val.DtmVal = getcurdatetime_();
			}
			OleDate od, od_addendum;
			od = Val.DtmVal;
			od_addendum.v = rVal;
			od.v += od_addendum.v;
			Val.DtmVal = od;
			break;
		default:
			ok = -1;
			break;
	}
	return ok;
}

void SLAPI ObjTagItem::Destroy()
{
	if(oneof3(TagDataType, OTTYP_STRING, OTTYP_GUID, OTTYP_IMAGE))
		delete Val.PStr;
	THISZERO();
}

int FASTCALL ObjTagItem::Copy(const ObjTagItem & src)
{
	Destroy();
	TagID       = src.TagID;
	TagDataType = src.TagDataType;
	TagEnumID   = src.TagEnumID;
	if(oneof3(TagDataType, OTTYP_STRING, OTTYP_GUID, OTTYP_IMAGE))
		Val.PStr = newStr(src.Val.PStr);
	else
		memcpy(&Val, &src.Val, sizeof(Val));
	return 1;
}

ObjTagItem & FASTCALL ObjTagItem::operator = (const ObjTagItem & src)
{
	Copy(src);
	return *this;
}

int FASTCALL ObjTagItem::operator == (const ObjTagItem & a) const { return (Cmp(a) == 0); }
int FASTCALL ObjTagItem::operator != (const ObjTagItem & a) const { return (Cmp(a) != 0); }

int SLAPI ObjTagItem::Put(PPID objType, PPID objID, long tagByObj, ObjTagTbl::Rec * pRec) const
{
	memzero(pRec, sizeof(*pRec));
	pRec->ObjType  = objType;
	pRec->ObjID    = objID;
	pRec->TagID    = TagID;
	pRec->TagByObj = tagByObj;
	if(TagDataType == OTTYP_NUMBER)
		pRec->RealVal = Val.RealVal;
	else if(oneof2(TagDataType, OTTYP_STRING, OTTYP_GUID)) {
		if(Val.PStr)
			STRNSCPY(pRec->StrVal, Val.PStr);
	}
	else if(TagDataType == OTTYP_IMAGE) {
		SString temp_buf;
		SString result_fname;
		ObjLinkFiles _lf(PPOBJ_TAG);
		_lf.Load(pRec->TagID, pRec->ObjID);
		if(sstrlen(Val.PStr))
			_lf.Replace(0, Val.PStr);
		else
			_lf.Remove(0);
		temp_buf.Z().Cat(pRec->ObjID);
		if(_lf.SaveSingle(pRec->TagID, temp_buf, 0, &result_fname) > 0)
			result_fname.CopyTo(pRec->StrVal, sizeof(pRec->StrVal));
		else
			pRec->StrVal[0] = 0;
	}
	else if(TagDataType == OTTYP_DATE)
		pRec->IntVal = Val.DtVal.v;
	else if(TagDataType == OTTYP_TIMESTAMP) {
		OleDate od;
		od = Val.DtmVal;
		pRec->RealVal = od.v;
	}
	else
		pRec->IntVal = Val.IntVal;
	return 1;
}

int FASTCALL ObjTagItem::Cmp(const ObjTagItem & a) const
{
	int    r;
	if((r = CMPSIGN(TagID, a.TagID)) == 0 && (r = CMPSIGN(TagDataType, a.TagDataType)) == 0) {
		switch(TagDataType) {
			case OTTYP_BOOL:
			case OTTYP_INT:
			case OTTYP_OBJLINK:
			case OTTYP_ENUM:
				r = CMPSIGN(Val.IntVal, a.Val.IntVal);
				break;
			case OTTYP_NUMBER:
				r = CMPSIGN(Val.RealVal, a.Val.RealVal);
				break;
			case OTTYP_STRING:
			case OTTYP_GUID:
			case OTTYP_IMAGE:
				if(Val.PStr)
					r = a.Val.PStr ? strcmp(Val.PStr, a.Val.PStr) : 1;
				else
					r = a.Val.PStr ? -1 : 0;
				break;
			case OTTYP_DATE:
				r = CMPFUNC(LDATE, &Val.DtVal, &a.Val.DtVal);
				break;
			case OTTYP_TIMESTAMP:
				r = CMPFUNC(LDATETIME, &Val.DtmVal, &a.Val.DtmVal);
				break;
			default:
				r = CMPSIGN(Val.IntVal, a.Val.IntVal);
				break;
		}
	}
	return r;
}

int SLAPI ObjTagItem::IsZeroVal() const
{
	switch(TagDataType) {
		case OTTYP_NUMBER: return (Val.RealVal == 0.0);
		case OTTYP_STRING:
		case OTTYP_GUID:
		case OTTYP_IMAGE: return isempty(Val.PStr);
		case OTTYP_DATE: return (Val.DtVal == ZERODATE);
		case OTTYP_TIMESTAMP: return !Val.DtmVal;
		default: return (Val.IntVal == 0);
	}
}

int SLAPI ObjTagItem::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, TagID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, TagDataType, rBuf));
	THROW_SL(pSCtx->Serialize(dir, TagEnumID, rBuf));
	if(TagDataType == OTTYP_NUMBER) {
		THROW_SL(pSCtx->Serialize(dir, Val.RealVal, rBuf));
	}
	else if(TagDataType == OTTYP_DATE) {
		THROW_SL(pSCtx->Serialize(dir, Val.DtVal, rBuf));
	}
	else if(TagDataType == OTTYP_IMAGE) {
		SString temp_buf;
#if 0 // @construction @dbd_exchange {
		{
			ObjLinkFiles link_files;
			if(dir > 0) {
				ObjLinkFiles::Fns fns;
				temp_buf = Val.PStr;
				if(ObjLinkFiles::SplitInnerFileName(temp_buf, &fns)	> 0 && fns.ObjType == PPOBJ_TAG && fns.ObjID == TagID) {
					link_files.Init(PPOBJ_TAG);
					link_files.Load(TagID, fns.AddedID);
				}
			}
			THROW(link_files.Serialize(dir, 0, rBuf, pSCtx));
			if(dir < 0) {
				link_files.At(0, temp_buf);
				Val.PStr = newStr(temp_buf);
			}
		}
#else
		if(dir > 0)
			temp_buf = Val.PStr;
		THROW_SL(pSCtx->Serialize(dir, temp_buf, rBuf));
		if(dir < 0) {
			// При приеме данных не изменяем адрес изображения дабы не разрушить уже привязанное
		}
#endif // } 0 @construction
	}
	else if(oneof2(TagDataType, OTTYP_STRING, OTTYP_GUID)) {
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v10.1.0
		if(dir > 0)
			r_temp_buf = Val.PStr;
		THROW_SL(pSCtx->Serialize(dir, r_temp_buf, rBuf));
		if(dir < 0) {
			if(r_temp_buf.NotEmpty())
				Val.PStr = newStr(r_temp_buf);
		}
	}
	else if(TagDataType == OTTYP_TIMESTAMP) {
		THROW_SL(pSCtx->Serialize(dir, Val.DtmVal, rBuf));
	}
	else {
		THROW_SL(pSCtx->Serialize(dir, Val.IntVal, rBuf));
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjTagItem::IsWarnVal() const
{
	if(TagDataType == OTTYP_NUMBER)
		return (Val.RealVal <= 0.0);
	else if(oneof3(TagDataType, OTTYP_STRING, OTTYP_GUID, OTTYP_IMAGE)) {
		if(Val.PStr && Val.PStr[0])
			return (atol(Val.PStr) <= 0);
		else
			return 1;
	}
	else if(TagDataType == OTTYP_DATE)
		return (Val.DtVal == ZERODATE);
	else if(TagDataType == OTTYP_TIMESTAMP)
		return !Val.DtmVal;
	else if(TagDataType == OTTYP_BOOL)
		return (Val.IntVal == 0);
	else
		return (Val.IntVal == 0);
}
//
// @todo(eliminate) {
//
int SLAPI ObjTagItem::Write__(SBuffer & rBuf) const
{
	rBuf.Write(TagID);
	rBuf.Write(TagDataType);
	rBuf.Write(TagEnumID);
	if(TagDataType == OTTYP_NUMBER)
		rBuf.Write(Val.RealVal);
	else if(TagDataType == OTTYP_IMAGE) {
		SString temp_buf = Val.PStr;
		/*
		ObjLinkFiles link_files(PPOBJ_TAG);
		link_files.Load(TagID, rRec.ObjID);
		link_files.At(0, img_path);
		Val.PStr = newStr(img_path);
		*/
		rBuf.Write(temp_buf);
	}
	else if(oneof2(TagDataType, OTTYP_STRING, OTTYP_GUID)) {
		SString temp_buf = Val.PStr;
		rBuf.Write(temp_buf);
	}
	else if(TagDataType == OTTYP_DATE)
		rBuf.Write(Val.DtVal);
	else if(TagDataType == OTTYP_TIMESTAMP)
		rBuf.Write(Val.DtmVal);
	else
		rBuf.Write(Val.IntVal);
	return 1;
}

int SLAPI ObjTagItem::Read__(SBuffer & rBuf)
{
	rBuf.Read(TagID);
	rBuf.Read(TagDataType);
	rBuf.Read(TagEnumID);
	if(TagDataType == OTTYP_NUMBER)
		rBuf.Read(Val.RealVal);
	else if(TagDataType == OTTYP_IMAGE) {
		SString temp_buf;
		rBuf.Read(temp_buf);
		if(temp_buf.NotEmpty())
			Val.PStr = newStr(temp_buf);
	}
	else if(oneof2(TagDataType, OTTYP_STRING, OTTYP_GUID)) {
		SString temp_buf;
		rBuf.Read(temp_buf);
		if(temp_buf.NotEmpty())
			Val.PStr = newStr(temp_buf);
	}
	else if(TagDataType == OTTYP_DATE)
		rBuf.Read(Val.DtVal);
	else if(TagDataType == OTTYP_TIMESTAMP)
		rBuf.Read(Val.DtmVal);
	else
		rBuf.Read(Val.IntVal);
	return 1;
}
// } @todo(eliminate)
//
//
//
SLAPI ObjTagList::ObjTagList() : SArray(sizeof(ObjTagItem), /*8,*/aryDataOwner|aryEachItem), ObjType(0), ObjID(0)
{
}
//
// Не используем унаследованный copy-constructor так как ObjTagList::Copy сама
// все сделает правильно.
//
SLAPI ObjTagList::ObjTagList(const ObjTagList & s) : SArray(sizeof(ObjTagItem), /*8,*/aryDataOwner|aryEachItem)
{
	Copy(s);
}

SLAPI ObjTagList::~ObjTagList()
{
    freeAll();
}

ObjTagList & FASTCALL ObjTagList::operator = (const ObjTagList & s)
{
	Copy(s);
	return *this;
}

int FASTCALL ObjTagList::IsEqual(const ObjTagList & rS) const
{
	int    eq = 1;
	if(GetCount() != rS.GetCount())
		eq = 0;
	else if(GetCount() > 0) {
		LongArray eq_pos_list;
		for(uint i = 0; eq && i < GetCount(); i++) {
			const ObjTagItem * p_item = GetItemByPos(i);
			int    found = 0;
			for(uint j = 0; !found && j < rS.GetCount(); j++) {
				if(!eq_pos_list.lsearch((long)j)) {
					const ObjTagItem * p_s_item = rS.GetItemByPos(j);
					if(p_item->Cmp(*p_s_item) == 0) {
						found = 1;
						eq_pos_list.add((long)j);
					}
				}
			}
			if(!found)
				eq = 0;
		}
	}
	return eq;
}

uint SLAPI ObjTagList::GetCount() const { return count; }
//virtual
void FASTCALL ObjTagList::freeItem(void * pItem) { static_cast<ObjTagItem *>(pItem)->Destroy(); }
const ObjTagItem * FASTCALL ObjTagList::GetItemByPos(uint pos) const
	{ return (pos < count) ? static_cast<const ObjTagItem *>(at(pos)) : 0; }
const ObjTagItem * FASTCALL ObjTagList::EnumItems(uint * pos) const
	{ return (*pos < count) ? static_cast<const ObjTagItem *>(at((*pos)++)) : 0; }

const ObjTagItem * FASTCALL ObjTagList::GetItem(PPID id) const
{
	uint   pos = 0;
	return lsearch(&id, &pos, CMPF_LONG) ? static_cast<const ObjTagItem *>(at(pos)) : 0;
}

int SLAPI ObjTagList::GetItemStr(PPID tagID, SString & rBuf) const
{
    rBuf.Z();
	if(tagID) {
		const ObjTagItem * p_item = GetItem(tagID);
		return (p_item && p_item->GetStr(rBuf) && rBuf.NotEmptyS()) ? 1 : -1;
	}
	else
		return -1;
}

const ObjTagItem * FASTCALL ObjTagList::GetItem_ForceEmpty(PPID id)
{
	uint   pos = 0;
	if(!lsearch(&id, &pos, CMPF_LONG)) {
		ObjTagItem tag_item;
		if(!tag_item.Init(id) || !PutItem(id, &tag_item))
			return 0;
		else
			pos = getCount() - 1;
	}
	return (const ObjTagItem *)at(pos);
}

int SLAPI ObjTagList::PutItem(PPID id, const ObjTagItem * pi)
{
	int    ok = 1;
	uint   pos = 0;
	if(lsearch(&id, &pos, CMPF_LONG)) {
		if(pi == 0)
			atFree(pos);
		else {
			*((ObjTagItem*)at(pos)) = *pi;
		}
	}
	else if(pi) {
		ObjTagItem item2(*pi);
		ok = insert(&item2);
		if(ok) {
			item2.Val.PStr = 0; // Trick (prevent destroying string ptr by destructor)
		}
		else
			PPSetErrorSLib();
	}
	return ok;
}

int SLAPI ObjTagList::PutItemStr(PPID tagID, const char * pStr)
{
	ObjTagItem tag_item;
	return tag_item.SetStr(tagID, pStr) ? PutItem(tagID, &tag_item) : 0;
}

int SLAPI ObjTagList::PutItemStrNE(PPID tagID, const char * pStr)
{
	int    ok = -1;
    if(!isempty(pStr)) {
		ObjTagItem tag_item;
		ok = tag_item.SetStr(tagID, pStr) ? PutItem(tagID, &tag_item) : 0;
	}
	return ok;
}

int SLAPI ObjTagList::PutItemNZ(const ObjTagItem * pItem, long flags)
{
	int    ok = -1;
	if(pItem) {
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		if(tag_obj.Fetch(pItem->TagID, &tag_rec) > 0) {
			if((!(flags & pinzfInheritableOnly) || (tag_rec.Flags & OTF_INHERITABLE)) &&
				(!(flags & pinzfUnmirroredOnly) || tag_obj.IsUnmirrored(pItem->TagID))) {
				ok = PutItem(pItem->TagID, pItem);
			}
		}
	}
	return ok;
}

int FASTCALL ObjTagList::Copy(const ObjTagList & src)
{
	const ObjTagItem * p_item;
	freeAll();
	ObjType = src.ObjType;
	ObjID   = src.ObjID;
	for(uint i = 0; (p_item = src.EnumItems(&i)) != 0;) {
		ObjTagItem item2(*p_item);
		insert(&item2);
		item2.Val.PStr = 0; // Trick (prevent destroying string ptr by destructor)
	}
	return 1;
}

int SLAPI ObjTagList::Merge(const ObjTagList & src, int updateMode)
{
	int    ok = -1;
	uint   i, j;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	const ObjTagItem * p_src_item;
	LongArray add_pos_list;
	LongArray rmv_pos_list;
	for(j = 0; (p_src_item = src.EnumItems(&j)) != 0;) {
		int    nfound = 1;
		const ObjTagItem * p_item;
		for(i = 0; (p_item = EnumItems(&i)) != 0;) {
			if(p_item->TagID == p_src_item->TagID) {
				nfound = 0;
				if(updateMode & mumRemove) {
					rmv_pos_list.addUnique((long)(i-1));
				}
				else if(updateMode & mumUpdate) {
					if(tag_obj.Fetch(p_item->TagID, &tag_rec) > 0) {
						rmv_pos_list.addUnique((long)(i-1));
						add_pos_list.add((long)(j-1));
					}
					else
						rmv_pos_list.addUnique((long)(i-1));
				}
			}
		}
		if(nfound && updateMode & mumAdd)
			add_pos_list.add((long)(j-1));
	}
	if(add_pos_list.getCount() || rmv_pos_list.getCount()) {
		rmv_pos_list.sort();
		for(int k = rmv_pos_list.getCount()-1; k >= 0; k--) {
			atFree(rmv_pos_list.get(k));
		}
		for(j = 0; j < add_pos_list.getCount(); j++) {
			ObjTagItem item2(*(const ObjTagItem *)src.at(add_pos_list.get(j)));
			insert(&item2);
			item2.Val.PStr = 0; // Trick (prevent destroying string ptr by destructor)
		}
		ok = 1;
	}
	return ok;
}

void SLAPI ObjTagList::Destroy()
{
	freeAll();
}

int SLAPI ObjTagList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir < 0)
		Destroy();
	int32  c = (int32)getCount();
	THROW_SL(pSCtx->Serialize(dir, c, rBuf));
	for(int32 i = 0; i < c; i++) {
		ObjTagItem item;
		if(dir > 0) {
			const ObjTagItem * p_item = GetItemByPos(i);
			if(!RVALUEPTR(item, p_item))
				continue;
		}
		THROW(item.Serialize(dir, rBuf, pSCtx));
		if(dir < 0) {
			THROW(PutItem(item.TagID, &item));
		}
	}
	CATCHZOK
	return ok;
}
//
// @todo Эти 2 функции используются в PPPsnEventPacket::Write и PPPsnEventPacket::Read
// Следует элиминировать и эти и вызывающие функции
//
int SLAPI ObjTagList::Write__(SBuffer & rBuf) const
{
	const long c = static_cast<long>(getCount());
	rBuf.Write(c);
	for(uint i = 0; i < getCount(); i++)
		GetItemByPos(i)->Write__(rBuf);
	return 1;
}

int SLAPI ObjTagList::Read__(SBuffer & rBuf)
{
	Destroy();
	long   c = 0;
	rBuf.Read(c);
	for(uint i = 0; i < (uint)c; i++) {
		ObjTagItem item;
		item.Read__(rBuf);
		PutItem(item.TagID, &item);
	}
	return 1;
}
//
// }
//
int SLAPI ObjTagList::ProcessObjRefs(PPObjIDArray * ary, int replace)
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		ObjTagItem * p_item = static_cast<ObjTagItem *>(at(i));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_TAG, &p_item->TagID, ary, replace));
		if(IS_DYN_OBJTYPE(p_item->TagEnumID)) {
			THROW(PPObject::ProcessObjRefInArray(PPOBJ_DYNAMICOBJS, &p_item->TagEnumID, ary, replace));
		}
		if(oneof2(p_item->TagDataType, OTTYP_OBJLINK, OTTYP_ENUM)) {
			THROW(PPObject::ProcessObjRefInArray(p_item->TagEnumID, &p_item->Val.IntVal, ary, replace));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI ObjTagCore::ObjTagCore() : ObjTagTbl()
{
}

SLAPI ObjTagCore::ObjTagCore(const char * pFileName) : ObjTagTbl(pFileName)
{
}

int SLAPI ObjTagCore::GetTagRec(PPID objType, PPID objID, PPID tagID, ObjTagTbl::Rec * pRec)
{
	ObjTagTbl::Key0 k;
	k.ObjType  = objType;
	k.ObjID    = objID;
	k.TagID    = tagID;
	k.TagByObj = 0;
	if(search(0, &k, spGe) && k.ObjType == objType && k.ObjID == objID && k.TagID == tagID) {
		copyBufTo(pRec);
		return 1;
	}
	else {
		memzero(pRec, sizeof(*pRec));
		return PPDbSearchError();
	}
}

int SLAPI ObjTagCore::GetTag(PPID objType, PPID objID, PPID tagID, ObjTagItem * pItem)
{
	ObjTagTbl::Rec rec;
	int    ok = GetTagRec(objType, objID, tagID, &rec);
	if(ok > 0 && pItem) {
		if(!pItem->Get_(data))
			ok = 0;
	}
	return ok;
}

int SLAPI ObjTagCore::GetTagStr(PPID objType, PPID objID, PPID tagID, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(tagID) { // @v10.3.3
		ObjTagItem item;
		int    r = GetTag(objType, objID, tagID, &item);
		if(r > 0 && item.GetStr(rBuf) && rBuf.NotEmptyS())
			ok = 1;
		else
			ok = r;
	}
	return ok;
}

int SLAPI ObjTagCore::GetTagGuid(PPID objType, PPID objID, PPID tagID, S_GUID & rGuid)
{
	rGuid.Z();
	int    ok = -1;
	ObjTagItem item;
	int    r = GetTag(objType, objID, tagID, &item);
	if(r > 0 && item.GetGuid(&rGuid) && !!rGuid)
		ok = 1;
	else
		ok = r;
	return ok;
}

int SLAPI ObjTagCore::RemoveTag(PPID objType, PPID objID, PPID tagID, int use_ta)
{
	int    ok = -1;
	ObjTagTbl::Key0 k;
	k.ObjType  = objType;
	k.ObjID    = objID;
	k.TagID    = tagID;
	k.TagByObj = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(search(0, &k, spGt) && data.ObjType == objType && data.ObjID == objID) {
			do {
				const PPID _tag_id = data.TagID;
				if(!tagID || _tag_id == tagID) {
					THROW_DB(rereadForUpdate(0, 0));
					THROW_DB(deleteRec()); // @sfu
					DS.LogAction(PPACN_OBJTAGRMV, objType, objID, _tag_id, 0);
					ok = 1;
				}
				if(tagID && _tag_id > tagID)
					break;
			} while(search(0, &k, spNext) && data.ObjType == objType && data.ObjID == objID);
		}
		THROW_DB(BTROKORNFOUND);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjTagCore::PutListExcl(PPID objType, PPID objID, const ObjTagList * pList, const PPIDArray * pExclList, int use_ta)
{
	int    ok = 1;
	uint   pos = 0;
	const  ObjTagItem * p_item = 0;
	const  ObjTagItem * p_org_item = 0;
	const  int is_list = BIN(pList && pList->GetCount());
	const  int is_excl_list = BIN(pExclList && pExclList->getCount());
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!is_excl_list && !is_list) { // @v8.9.2 (|| !pList->GetCount()) ради ускорения
			THROW(RemoveTag(objType, objID, 0, 0));
		}
		else {
			ObjTagList org_list;
			THROW(GetList(objType, objID, &org_list));
			{
				//
				// Сначала удаляем теги, которых нет в новом списке
				//
				for(uint org_pos = 0; (p_org_item = org_list.EnumItems(&org_pos)) != 0;) {
					p_item = is_list ? pList->GetItem(p_org_item->TagID) : 0;
					if(!p_item && !(is_excl_list && pExclList->lsearch(p_org_item->TagID))) {
						THROW(RemoveTag(objType, objID, p_org_item->TagID, 0));
					}
				}
			}
			if(is_list) {
				//
				// Теперь, для каждого тега из нового списка ищем соответствующий в существующем (org_list);
				// Если тег найден и 'квивалентен, то ничего не делаем, в противном случае создаем или модифицируем тег.
				//
				for(pos = 0; (p_item = pList->EnumItems(&pos)) != 0;) {
					p_org_item = org_list.GetItem(p_item->TagID);
					if(!p_org_item || *p_org_item != *p_item) {
						if(!(is_excl_list && pExclList->lsearch(p_item->TagID))) { // @v9.0.9
							THROW(PutTag(objType, objID, (ObjTagItem*)p_item, 0));
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

int SLAPI ObjTagCore::PutList(PPID objType, PPID objID, const ObjTagList * pList, int use_ta)
{
	return PutListExcl(objType, objID, pList, 0, use_ta);
}

#if 0 // {
int SLAPI ObjTagCore::PutList(PPID objType, PPID objID, const ObjTagList * pList, int use_ta)
{
	int    ok = 1, ta = 0;
	uint   pos = 0;
	const  ObjTagItem * p_item = 0;
	THROW(PPStartTransaction(&ta, use_ta));
	THROW(RemoveTag(objType, objID, 0, 0));
	if(pList)
		for(pos = 0; (p_item = pList->EnumItems(&pos)) != 0;) {
			THROW(PutTag(objType, objID, p_item, 0));
		}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}
#endif // } 0

int SLAPI ObjTagCore::PutTag(PPID objType, PPID objID, const ObjTagItem * pItem, int use_ta)
{
	int    ok = 1;
	int    r = 1;
	long   tbo, c = 0;
	ObjTagItem  temp;
	PPObjectTag tag;
	PPObjTag tagobj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(tagobj.Fetch(pItem->TagID, &tag) > 0);
		THROW_PP(tag.TagDataType > 0 && tag.TagDataType <= OTTYP_LAST, PPERR_INVTAGDATATYPE);
		{
			const int zero = BIN(tag.Flags & OTF_NOZERO && pItem->IsZeroVal());
			for(tbo = 0, c = 0; (r = EnumTags(objType, objID, tag.ID, &tbo, &temp)) > 0; c++) {
				if(c == 0 && !zero) {
					if(*pItem != temp) {
						THROW_DB(rereadForUpdate(0, 0)); // @v8.2.0
						THROW(PutTagToRec(objType, objID, tbo, pItem));
						THROW_DB(updateRec()); // @sfu
						DS.LogAction(PPACN_OBJTAGUPD, objType, objID, pItem->TagID, 0);
					}
				}
				else {
					THROW_DB(rereadForUpdate(0, 0)); // @v8.2.0
					THROW_DB(deleteRec()); // @sfu
					DS.LogAction(PPACN_OBJTAGRMV, objType, objID, pItem->TagID, 0);
				}
			}
			THROW(r);
			if(c == 0 && !zero) {
				THROW(PutTagToRec(objType, objID, 0, pItem));
				THROW_DB(insertRec());
				DS.LogAction(PPACN_OBJTAGADD, objType, objID, pItem->TagID, 0); // @v8.0.1 PPACN_OBJTAGUPD-->PPACN_OBJTAGADD
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ObjTagCore::GetNewTagByObjCounter(PPID objType, PPID objID, PPID tagID, long * pTagByObj)
{
	int    ok = 1;
	long   tag_by_obj = 0;
	ObjTagTbl::Key0 k;
	k.ObjType = objType;
	k.ObjID = objID;
	k.TagID = tagID;
	k.TagByObj = MAXLONG;
	if(search(0, &k, spLe) && k.ObjType == objType && k.ObjID == objID && k.TagID == tagID) {
		tag_by_obj = k.TagByObj + 1;
		ok = 1;
	}
	else {
		tag_by_obj = 1;
		ok = (BTROKORNFOUND) ? 1 : PPSetErrorDB();
	}
	ASSIGN_PTR(pTagByObj, tag_by_obj);
	return ok;
}

int SLAPI ObjTagCore::PutTagToRec(PPID objType, PPID objID, long tagByObj, const ObjTagItem * pItem)
{
	int    ok = 0;
	if(tagByObj || GetNewTagByObjCounter(objType, objID, pItem->TagID, &tagByObj)) {
		pItem->Put(objType, objID, tagByObj, &data);
		ok = 1;
	}
	return ok;
}

int FASTCALL ObjTagItem::Get_(const ObjTagTbl::Rec & rRec)
{
	int    ok = 1;
	Destroy();
	PPObjectTag tag_rec;
	PPObjTag tag_obj;
	if(tag_obj.Fetch(rRec.TagID, &tag_rec) > 0) {
		TagID       = rRec.TagID;
		TagDataType = tag_rec.TagDataType;
		TagEnumID   = tag_rec.TagEnumID;
		if(TagDataType == OTTYP_NUMBER)
			Val.RealVal = rRec.RealVal;
		else if(oneof2(TagDataType, OTTYP_STRING, OTTYP_GUID))
			Val.PStr = newStr(rRec.StrVal);
		else if(TagDataType == OTTYP_IMAGE) {
			SString img_path;
			ObjLinkFiles link_files(PPOBJ_TAG);
			link_files.Load(rRec.TagID, rRec.ObjID);
			link_files.At(0, img_path);
			Val.PStr = newStr(img_path);
		}
		else if(TagDataType == OTTYP_DATE)
			Val.DtVal.v = rRec.IntVal;
		else if(TagDataType == OTTYP_TIMESTAMP) {
			OleDate od;
			od.v = rRec.RealVal;
			Val.DtmVal = od;
		}
		else
			Val.IntVal = rRec.IntVal;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI ObjTagCore::EnumTags(PPID objType, PPID objID, PPID * pTagID, long * pTagByObj, ObjTagItem * pItem)
{
	ObjTagTbl::Key0 k;
	k.ObjType  = objType;
	k.ObjID    = objID;
	k.TagID    = *pTagID;
	k.TagByObj = DEREFPTRORZ(pTagByObj);
	while(search(0, &k, spGt) && k.ObjType == objType && k.ObjID == objID) {
		if(pItem) {
			if(pItem->Get_(data)) {
				*pTagID = data.TagID;
				ASSIGN_PTR(pTagByObj, data.TagByObj);
				return 1;
			}
		}
		else
			return 1;
	}
	return PPDbSearchError();
}

int SLAPI ObjTagCore::EnumTags(PPID objType, PPID objID, PPID tagID, long * pTagByObj, ObjTagItem * pItem)
{
	PPID   _tagid = tagID;
	int    r = EnumTags(objType, objID, &_tagid, pTagByObj, pItem);
	if(_tagid != tagID)
		r = -1;
	return r;
}

int SLAPI ObjTagCore::GetList(PPID objType, PPID objID, ObjTagList * pList)
{
	int    ok = 1;
	PPID   tag_id = 0;
	long   tag_by_obj = 0;
	ObjTagItem item;
	while(ok && (ok = EnumTags(objType, objID, &tag_id, &tag_by_obj, &item)) > 0) {
		if(pList) {
			if(!pList->PutItem(tag_id, &item))
				ok = 0;
		}
	}
	return ok;
}

int SLAPI ObjTagCore::GetObjectList(PPID objType, PPID tagID, UintHashTable & rList)
{
	int    ok = -1;
	ObjTagTbl::Key1 k1;
	MEMSZERO(k1);
	k1.TagID = tagID;
	BExtQuery q(this, 1);
	q.select(this->ObjType, this->ObjID, this->TagID, 0).where(this->TagID == tagID && this->ObjType == objType);
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
		rList.Add(data.ObjID);
		ok = 1;
	}
	return ok;
}

int SLAPI ObjTagCore::SearchAnyRefToTagID(PPID tagID, PPObjID * pObjID)
{
	int    ok = 1;
	ObjTagTbl::Key1 k1;
	MEMSZERO(k1);
	k1.TagID = tagID;
	if(search(1, &k1, spGe) && k1.TagID == tagID) {
		if(pObjID) {
			pObjID->Obj = data.ObjType;
			pObjID->Id  = data.ObjID;
		}
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI ObjTagCore::SearchObjectsByStrExactly(PPID objType, PPID tagID, const char * pStr, PPIDArray * pList)
{
	PPIDArray list;
	if(!isempty(pStr)) {
		ObjTagTbl::Key3 k3;
		MEMSZERO(k3);
		k3.TagID = tagID;
 		STRNSCPY(k3.StrVal, pStr);
		if(search(3, &k3, spEq))
	 		do {
				list.add(data.ObjID);
	 		} while(search(3, &k3, spNext) && k3.TagID == tagID && stricmp866(k3.StrVal, pStr) == 0);
	}
	list.sortAndUndup();
	ASSIGN_PTR(pList, list);
	return list.getCount() ? 1 : -1;
}

int SLAPI ObjTagCore::SearchObjectsByGuid(PPID objType, PPID tagID, const S_GUID & rGuid, PPIDArray * pList)
{
	ObjTagItem tag;
	return tag.SetGuid(tagID, &rGuid) ? SearchObjectsByStrExactly(objType, tagID, tag.Val.PStr, pList) : 0;
}

int SLAPI ObjTagCore::SearchObjectsByStr(PPID objType, PPID tagID, const char * pStr, PPIDArray * pList)
{
	PPIDArray list;
	if(!isempty(pStr)) {
	 	// AHTOXA {
		int    substr_srch  = (pStr && pStr[0] == '*');
		int    sp_type      = substr_srch ? spGe : spEq;
		const  char * p_str = substr_srch ? (pStr + 1) : pStr;
		int    srch_str_not_empty = (p_str && sstrlen(p_str) > 0);
		// } AHTOXA
		ObjTagTbl::Key3 k3;
		MEMSZERO(k3);
		k3.TagID = tagID;
		if(!substr_srch) // AHTOXA
	 		STRNSCPY(k3.StrVal, pStr);
		if(search(3, &k3, sp_type))
	 		do {
	 			if(data.ObjType == objType) {
	 				// AHTOXA {
	 				if(!(substr_srch && srch_str_not_empty) || ExtStrSrch(k3.StrVal, p_str, 0) > 0)
	 					list.add(data.ObjID);
	 				// } AHTOXA
	 			}
	 		} while(search(3, &k3, spNext) && k3.TagID == tagID && (substr_srch || stricmp866(k3.StrVal, pStr) == 0)); // AHTOXA
	}
	list.sortAndUndup();
	ASSIGN_PTR(pList, list);
	return list.getCount() ? 1 : -1;
}

int SLAPI ObjTagCore::GetObjTextList(PPID objType, PPID tagID, StrAssocArray & rList)
{
	rList.Z();

	int    ok = -1;
	ObjTagTbl::Key3 k3;
	MEMSZERO(k3);
	k3.TagID = tagID;
	SString temp_buf;
	if(search(3, &k3, spGe) && data.TagID == tagID) {
		BExtQuery q(this, 3);
		q.select(this->ObjID, this->StrVal, 0).where(this->TagID == tagID);
		for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;) {
			temp_buf = data.StrVal;
			if(temp_buf.NotEmptyS()) {
				rList.AddFast(data.ObjID, temp_buf);
			}
		}
	}
	if(rList.getCount())
		ok = 1;
	return ok;
}

#if 0 // @obsolete since @v9.0.11 {
int SLAPI ObjTagCore::CreateUniqueStr(PPID objType, PPID tagID, PPID * pObjID, const char * pStr, int use_ta)
{
	int    ok = -1;
	PPID   obj_id = 0;
	const  size_t _len = sstrlen(pStr);
	if(_len) {
		ObjTagTbl::Key3 k3;
		MEMSZERO(k3);
		k3.TagID = tagID;
		STRNSCPY(k3.StrVal, pStr);
		BExtQuery q(this, 3);
		THROW_PP_S(_len <= sizeof(data.StrVal)-1, PPERR_EXCSTRLEN, pStr);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			if(search(3, &k3, spEq)) {
				do {
					if(data.ObjType == objType) {
						obj_id = data.ObjID;
						ok = 1;
					}
				} while(ok < 0 && search(3, &k3, spNext) && data.TagID == tagID && stricmp866(k3.StrVal, pStr) == 0);
			}
			if(ok < 0) {
				ObjTagTbl::Key1 k1;
				MEMSZERO(k1);
				k1.TagID = tagID;
				k1.ObjID = MAXLONG;
				obj_id = 1;
				while(search(1, &k1, spLt) && k1.TagID == tagID)
					if(data.ObjType == objType) {
						obj_id = data.ObjID+1;
						break;
					}
				{
					ObjTagTbl::Rec rec;
					MEMSZERO(rec);
					rec.ObjType = objType;
					rec.ObjID = obj_id;
					rec.TagID = tagID;
					THROW(GetNewTagByObjCounter(objType, obj_id, tagID, &rec.TagByObj));
					STRNSCPY(rec.StrVal, pStr);
					THROW_DB(insertRecBuf(&rec));
					DS.LogAction(PPACN_OBJTAGUPD, objType, obj_id, tagID, 0);
					ok = 2;
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	ASSIGN_PTR(pObjID, obj_id);
	return ok;
}
#endif // } 0 @obsolete since @v9.0.11
