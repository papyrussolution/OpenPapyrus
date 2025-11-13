// BNFLDLST.CPP
// Copyright (c) Sobolev A. 1995, 2003, 2004, 2007, 2008, 2009, 2010, 2013, 2015, 2016, 2017, 2019, 2021, 2022, 2024, 2025
//
#include <slib-internal.h>
#pragma hdrstop

BNField::BNField() : Id(0), Offs(0), T(0)
{
	Name[0] = 0;
}

bool BNField::IsEq(const BNField & rS, bool nameAndTypeOnly) const
{
	if(T != rS.T)
		return false;
	else if(stricmp(Name, rS.Name) != 0)
		return false;
	else if(!nameAndTypeOnly) {
		if(Id != rS.Id)
			return false;
		else if(Offs != rS.Offs)
			return false;
	}
	return true;
}

int BNField::getValue(const void * pDataBuf, void * pBuf, size_t * pSize) const
{
	if(pDataBuf && pBuf) {
		const size_t sz = stsize(T);
		memcpy(pBuf, PTR8C(pDataBuf)+Offs, sz);
		ASSIGN_PTR(pSize, sz);
		return 1;
	}
	else
		return 0;
}

int BNField::setValue(void * pDataBuf, const void * pBuf) const
{
	if(pDataBuf && pBuf) {
		memcpy(PTR8(pDataBuf)+Offs, pBuf, stsize(T));
		return 1;
	}
	else
		return 0;
}

int BNField::putValueToString(const void * pDataBuf, char * pBuf) const
{
	if(pDataBuf && pBuf) {
		const void * p_data = PTR8C(pDataBuf)+Offs;
		sttostr(T, p_data, 0, pBuf);
		return 1;
	}
	else
		return 0;
}
//
//
//
BNFieldList2::BNFieldList2() : TSVector <BNField>()
{
}

bool BNFieldList2::IsEq(const BNFieldList2 & rS, bool nameAndTypeOnly/*= false*/) const
{
	if(getCount() != rS.getCount())
		return false;
	else {
		for(uint i = 0; i < count; i++) {
			if(!at(i).IsEq(rS.at(i), nameAndTypeOnly))
				return false;
		}
	}
	return true;
}

BNFieldList2 & BNFieldList2::Z()
{
	TSVector <BNField>::clear();
	return *this;
}

int BNFieldList2::addField(const char * pName, TYPEID type, int id/*= UNDEF*/)
{
	int    ok = 1;
	BNField fld;
	STRNSCPY(fld.Name, pName);
	fld.T  = type;
	fld.Id = (id == (int)UNDEF) ? count : id;
	if(id == (int)UNDEF) {
		for(int i = count; i < 4096; i++) {
			if(!GetFieldPosition(i, 0)) {
				fld.Id = i;
				break;
			}
		}
	}
	else if(!GetFieldPosition(id, 0))
		fld.Id = id;
	else {
		SString msg_buf;
		msg_buf.Cat(id);
		SLS.SetError(SLERR_SDREC_DUPFLDID, msg_buf);
		ok = 0;
	}
	if(ok) {
		fld.Offs = count ? (at(count-1).Offs + stsize(at(count-1).T)) : 0;
		insert(&fld);
	}
	return ok;
}

int FASTCALL BNFieldList2::addField(const BNField & rS)
{
	return addField(rS.Name, rS.T, UNDEF);
}

bool BNFieldList2::GetFieldPosition(int fldId, uint * pPos) const
{
	bool   ok = false;
	for(uint i = 0; !ok && i < getCount(); i++) {
		if(at(i).Id == fldId) {
			ASSIGN_PTR(pPos, i);
			ok = true;
		}
	}
	return ok;
}

const BNField & BNFieldList2::GetFieldByPosition(uint fldIdx) const
{
	return (fldIdx < getCount()) ? at(fldIdx) : *reinterpret_cast<BNField *>(0);
}

/*const BNField & BNFieldList2::getField__(uint id, bool byPosition) const
{
	if(byPosition) {
		return (id < getCount()) ? at(id) : *reinterpret_cast<BNField *>(0);
	}
	else {
		for(uint i = 0; i < getCount(); i++) {
			if(at(i).Id == id)
				return at(i);
		}
	}
	return *reinterpret_cast<BNField *>(0);
}*/

const BNField & BNFieldList2::GetFieldByName(const char * pName, uint * pPos/*= 0*/) const
{
	if(!isempty(pName)) {
		for(uint i = 0; i < getCount(); i++) {
			if(stricmp(at(i).Name, pName) == 0) {
				ASSIGN_PTR(pPos, i);
				return at(i);
			}
		}
	}
	return *reinterpret_cast<BNField *>(0);
}

RECORDSIZE BNFieldList2::CalculateFixedRecSize() const
{
	RECORDSIZE s = 0;
	for(uint i = 0; i < count; i++) {
		const int t = GETSTYPE(at(i).T);
		if(!oneof3(t, S_NOTE, S_BLOB, S_CLOB)) // @stypeset
			s += static_cast<RECORDSIZE>(stsize(at(i).T));
	}
	return s;
}

int BNFieldList2::setFieldId(uint fldNo/* 0..(count-1)*/, int fldId)
{
	if(fldNo < count) {
		at(fldNo).Id = fldId;
		return 1;
	}
	else
		return 0;
}

bool BNFieldList2::IsEqualRecords(const void * pRec1, const void * pRec2, LongArray * pNeqFldList) const
{
	bool   eq = true;
	if(pRec1 == 0 && pRec2 == 0)
		eq = true;
	else if(pRec1 == 0 || pRec2 == 0)
		eq = false;
	else {
		for(uint i = 0; i < count; i++) {
			const BNField & r_fld = at(i);
			if(stcomp(r_fld.T, PTR8C(pRec1)+r_fld.Offs, PTR8C(pRec2)+r_fld.Offs) != 0) {
				eq = false;
				if(pNeqFldList)
					pNeqFldList->add(r_fld.Id);
				else
					break;
			}
		}
	}
	return eq;
}
