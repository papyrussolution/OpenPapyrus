// BNFLDLST.CPP
// Copyright (c) Sobolev A. 1995, 2003, 2004, 2007, 2008, 2009, 2010, 2013, 2015, 2016, 2017, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

int SLAPI BNField::IsEqual(const BNField & rS, int nameAndTypeOnly) const
{
	if(T != rS.T)
		return 0;
	else if(stricmp(Name, rS.Name) != 0)
		return 0;
	else if(!nameAndTypeOnly) {
		if(Id != rS.Id)
			return 0;
		else if(Offs != rS.Offs)
			return 0;
	}
	return 1;
}

int FASTCALL BNField::getValue(const void * pDataBuf, void * pBuf, size_t * pSize) const
{
	if(pDataBuf && pBuf) {
		const size_t sz = stsize(T);
		if(sz == sizeof(uint32))
			PTR32(pBuf)[0] = PTR32(PTR8(pDataBuf)+Offs)[0];
		else if(sz == sizeof(uint32) * 2) {
			PTR32(pBuf)[0] = PTR32(PTR8(pDataBuf)+Offs)[0];
			PTR32(pBuf)[1] = PTR32(PTR8(pDataBuf)+Offs)[1];
		}
		else if(sz == sizeof(uint16))
			PTR16(pBuf)[0] = PTR16(PTR8(pDataBuf)+Offs)[0];
		else
			memcpy(pBuf, PTR8(pDataBuf)+Offs, sz);
		ASSIGN_PTR(pSize, sz);
		return 1;
	}
	else
		return 0;
}

int SLAPI BNField::setValue(void * pDataBuf, const void * pBuf) const
{
	if(pDataBuf && pBuf) {
		memcpy(PTR8(pDataBuf)+Offs, pBuf, stsize(T));
		return 1;
	}
	else
		return 0;
}

int SLAPI BNField::putValueToString(const void * pDataBuf, char * pBuf) const
{
	if(pDataBuf && pBuf) {
		const void * p_data = PTR8(pDataBuf)+Offs;
		sttostr(T, p_data, 0, pBuf);
		return 1;
	}
	else
		return 0;
}
//
//
//
SLAPI BNFieldList::BNFieldList() : count(0), fields(0)
{
}

SLAPI BNFieldList::BNFieldList(const BNFieldList & rS) : count(0), fields(0)
{
	copy(&rS);
}

SLAPI BNFieldList::~BNFieldList()
{
	SAlloc::F(fields);
}

void SLAPI BNFieldList::reset()
{
	if(count) {
		SAlloc::F(fields);
		count = 0;
		fields = 0;
	}
}

int SLAPI BNFieldList::IsEqual(const BNFieldList & rS, int nameAndTypeOnly) const
{
	if(count != rS.count)
		return 0;
	else {
		for(uint i = 0; i < count; i++) {
			if(!fields[i].IsEqual(rS.fields[i], nameAndTypeOnly))
				return 0;
		}
	}
	return 1;
}

BNFieldList & FASTCALL BNFieldList::operator = (const BNFieldList & s)
{
	copy(&s);
	return *this;
}

int FASTCALL BNFieldList::copy(const BNFieldList * pSrc)
{
	reset();
	fields = static_cast<BNField *>(SAlloc::M(sizeof(BNField) * pSrc->count));
	if(fields == 0) {
		count = 0;
		return 0;
	}
	else {
		memcpy(fields, pSrc->fields, sizeof(BNField) * pSrc->count);
		count = pSrc->count;
		return 1;
	}
}

int FASTCALL BNFieldList::addField(const BNField & rSrc)
{
	return addField(rSrc.Name, rSrc.T, UNDEF);
}

int SLAPI BNFieldList::addField(const char * pName, TYPEID type, int id)
{
	int    ok = 1;
	BNField fld;
	STRNSCPY(fld.Name, pName);
	fld.T  = type;
	fld.Id = (id == (int)UNDEF) ? count : id;
	if(id == (int)UNDEF) {
		for(int i = count; i < 4096; i++)
			if(!getFieldPosition(i, 0)) {
				fld.Id = i;
				break;
			}
	}
	else if(!getFieldPosition(id, 0))
		fld.Id = id;
	else {
		SString msg_buf;
		msg_buf.Cat(id);
		SLS.SetError(SLERR_SDREC_DUPFLDID, msg_buf);
		ok = 0;
	}
	if(ok) {
		fld.Offs = count ? (fields[count-1].Offs + stsize(fields[count-1].T)) : 0;
		if((fields = static_cast<BNField *>(SAlloc::R(fields, sizeof(BNField) * (count+1)))) != 0) {
			memcpy(fields + count, &fld, sizeof(BNField));
			count++;
		}
		else {
			count = 0;
			ok = 0;
		}
	}
	return ok;
}

int SLAPI BNFieldList::setFieldId(uint fldNo, int fldId)
{
	if(fldNo < count) {
		fields[fldNo].Id = fldId;
		return 1;
	}
	else
		return 0;
}

const BNField & SLAPI BNFieldList::getField(uint id, int byPosition) const
{
	if(byPosition)
		return (id < count) ? fields[id] : *reinterpret_cast<BNField *>(0);
	else
		for(uint i = 0; i < count; i++)
			if(fields[i].Id == id)
				return fields[i];
	return *reinterpret_cast<BNField *>(0);
}

int SLAPI BNFieldList::getFieldPosition(int id, uint * pPos) const
{
	for(uint i = 0; i < count; i++)
		if(fields[i].Id == id) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	return 0;
}

const BNField & SLAPI BNFieldList::getField(const char * pName, uint * pPos) const
{
	for(uint i = 0; i < count; i++)
		if(stricmp(fields[i].Name, pName) == 0) {
			ASSIGN_PTR(pPos, i);
			return fields[i];
		}
	return *reinterpret_cast<BNField *>(0);
}

RECORDSIZE SLAPI BNFieldList::getRecSize() const
{
	RECORDSIZE s = 0;
	for(uint i = 0; i < count; i++) {
		const int t = GETSTYPE(fields[i].T);
		if(!oneof3(t, S_NOTE, S_BLOB, S_CLOB)) // @stypeset
			s += static_cast<RECORDSIZE>(stsize(fields[i].T));
	}
	return s;
}

int SLAPI BNFieldList::IsEqualRecords(const void * pRec1, const void * pRec2, LongArray * pNeqFldList) const
{
	int    eq = 1;
	if(pRec1 == 0 && pRec2 == 0)
		eq = 1;
	else if(pRec1 == 0 || pRec2 == 0)
		eq = 0;
	else {
		for(uint i = 0; i < count; i++) {
			const BNField & r_fld = fields[i];
			if(stcomp(r_fld.T, PTR8(pRec1)+r_fld.Offs, PTR8(pRec2)+r_fld.Offs) != 0) {
				eq = 0;
				if(pNeqFldList)
					pNeqFldList->add(r_fld.Id);
				else
					break;
			}
		}
	}
	return eq;
}
