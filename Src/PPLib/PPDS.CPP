// PPDS.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2007, 2009, 2010, 2015, 2016, 2017, 2018, 2021, 2023, 2025
//
#pragma hdrstop
#ifdef PPDSC
	#define LOCAL_PPERRCODE
#endif
#include <pp.h>
#include <ppds.h>

PPDeclStruc::PPDeclStruc() : SArray(sizeof(PPDeclStrucItem)), Id(0), P_Outer(0), UpdateProtocol(0), P_Logger(0)
{
}

PPDeclStruc::~PPDeclStruc()
{
	InitData(idoDestroy, 0, 0);
}

uint PPDeclStruc::GetCount() const
{
	return getCount();
}

long PPDeclStruc::GetId() const
{
	return Id;
}

int PPDeclStruc::InitData(Ido op, void * extraPtr, long)
{
	int    ok = 1;
	if(op == idoAlloc)
		ok = -1;
	else if(op == idoDestroy)
		ok = -1;
	else if(op == idoExtract)
		ok = 1;
	else if(op == idoAccept)
		ok = 1;
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(int * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && (*pData) == 0)
			ok = -1;
		rBuf.Z().Cat(*pData);
	}
	else if(dir == tfdBufToData)
		*pData = rBuf.ToLong();
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferDataFlag(int * pData, long flag, Tfd dir, SString & rBuf, int skipZero)
{
	int32  val = 0;
	if(dir == tfdDataToBuf)
		val = BIN(*pData & flag);
	int    ok = TransferData(&val, dir, rBuf, skipZero);
	if(dir == tfdBufToData)
		SETFLAG(*pData, flag, val);
	return ok;
}

int PPDeclStruc::TransferDataFlag(long * pData, long flag, Tfd dir, SString & rBuf, int skipZero)
{
	return TransferDataFlag(reinterpret_cast<int *>(pData), flag, dir, rBuf, skipZero);
}


int PPDeclStruc::TransferDataFlag(int16 * pData, long flag, Tfd dir, SString & rBuf, int skipZero)
{
	int32 val = 0;
	if(dir == tfdDataToBuf)
		val = BIN(*pData & flag);
	int    ok = TransferData(&val, dir, rBuf, skipZero);
	if(dir == tfdBufToData)
		SETFLAG(*pData, flag, val);
	return ok;
}

int PPDeclStruc::TransferData(int16 * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && (*pData) == 0)
			ok = -1;
		rBuf.Z().Cat((long)*pData);
	}
	else if(dir == tfdBufToData)
		*pData = (int16)rBuf.ToLong();
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(uint16 * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && (*pData) == 0)
			ok = -1;
		rBuf.Z().Cat((long)*pData);
	}
	else if(dir == tfdBufToData)
		*pData = (uint16)rBuf.ToLong();
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(uint8 * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && (*pData) == 0)
			ok = -1;
		rBuf.Z().Cat((long)*pData);
	}
	else if(dir == tfdBufToData)
		*pData = (uint8)rBuf.ToLong();
	else
		ok = PPSetErrorInvParam();
	return ok;

}

int PPDeclStruc::TransferData(double * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && (*pData) == 0)
			ok = -1;
		rBuf.Z().Cat(*pData, MKSFMTD(0, 10, NMBF_NOTRAILZ));
	}
	else if(dir == tfdBufToData)
		*pData = rBuf.ToReal();
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(SString & rData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		rBuf.Z();
		if(skipZero && rData.IsEmpty())
			ok = -1;
		for(uint i = 0; i < rData.Len(); i++) {
			char c = rData.C(i);
			switch(c) {
				case '\n': rBuf.BSlash().CatChar('n'); break;
				case '\r': rBuf.BSlash().CatChar('r'); break;
				case '\t': rBuf.BSlash().CatChar('t'); break;
				case ';' : rBuf.BSlash().Semicol(); break;
				case '\\': rBuf.BSlash().BSlash(); break;
				default: rBuf.CatChar(c); break;
			}
		}
		rBuf.Transf(CTRANSF_INNER_TO_OUTER);
	}
	else if(dir == tfdBufToData) {
		rData.Z();
		// @v11.2.2 rBuf.ToOem();
		rBuf.Transf(CTRANSF_OUTER_TO_INNER); // @v11.2.2
		for(uint i = 0; i < rBuf.Len(); i++) {
			char c = rBuf.C(i);
			if(c == '\\') {
				switch(rBuf.C(++i)) {
					case 'n':  c = '\n';  break;
					case 'r':  c = '\r';  break;
					case 't':  c = '\t';  break;
					case ';':  c = ';';   break;
					case '\\': c = '\\';  break;
					default: --i; break;
				}
			}
			rData.CatChar(c);
		}
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(char * pData, size_t dataBufLen, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && pData[0] == 0)
			ok = -1;
		for(const char * p = pData; *p; p++) {
			switch(*p) {
				case '\n': rBuf.BSlash().CatChar('n'); break;
				case '\r': rBuf.BSlash().CatChar('r'); break;
				case '\t': rBuf.BSlash().Tab(); break;
				case ';' : rBuf.BSlash().Semicol(); break;
				case '\\': rBuf.BSlash().BSlash(); break;
				default: rBuf.CatChar(*p); break;
			}
		}
		rBuf.Transf(CTRANSF_INNER_TO_OUTER);
	}
	else if(dir == tfdBufToData) {
		rBuf.ToOem();
		for(uint i = 0; i < rBuf.Len(); i++) {
			char c = rBuf.C(i);
			if(c == '\\') {
				switch(rBuf.C(++i)) {
					case 'n':  c = '\n';  break;
					case 'r':  c = '\r';  break;
					case 't':  c = '\t';  break;
					case ';':  c = ';';   break;
					case '\\': c = '\\';  break;
					default: --i; break;
				}
			}
			*pData++ = c;
		}
		*pData = 0;
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(DateRange * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && !pData->IsZero())
			ok = -1;
		rBuf.Z().Cat(*pData);
	}
	else if(dir == tfdBufToData)
		pData->FromStr(rBuf, 0);
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(LDATE * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && !*pData)
			ok = -1;
		rBuf.Z().Cat(*pData, DATF_DMY|DATF_CENTURY);
	}
	else if(dir == tfdBufToData)
		strtodate(rBuf, DATF_DMY|DATF_CENTURY, pData);
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(LTIME * pData, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && !*pData)
			ok = -1;
		rBuf.Z().Cat(*pData, TIMF_HMS);
	}
	else if(dir == tfdBufToData)
		strtotime(rBuf, TIMF_HMS, pData);
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(long * pData, long flag, Tfd dir, SString & rBuf, int skipZero)
{
	int    ok = 1;
	if(dir == tfdDataToBuf) {
		if(skipZero && !(*pData & flag))
			ok = -1;
		rBuf.Z().CatChar((*pData & flag) ? '1' : '0');
	}
	else if(dir == tfdBufToData) {
		SETFLAG(*pData, flag, rBuf.ToLong());
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferData(PPIDArray * pList, Tfd dir, uint iter, SString & rBuf)
{
	int    ok = -1;
	if(dir == tfdDataToBuf) {
		if(pList && iter < pList->getCount()) {
			rBuf.Z().Cat(pList->at(iter));
			ok = 1;
		}
	}
	else if(dir == tfdBufToData) {
		CALLPTRMEMB(pList, add(rBuf.ToLong()));
		ok = 1;
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::TransferField(long, Tfd dir, uint *, SString & rBuf)
{
	int    ok = -1;
	if(dir == tfdDataToBuf) {
		rBuf.Z();
		ok = -1;
	}
	else if(dir == tfdBufToData)
		ok = -1;
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int PPDeclStruc::AcceptListItem(long, PPDeclStruc *, ObjTransmContext * pCtx)
{
	return -1;
}

int PPDeclStruc::CreateListItem(long, uint *, PPDeclStruc *)
{
	return -1;
}

int PPDeclStruc::WriteDataToStream(FILE * stream, void * extraPtr)
{
	return Helper_WriteDataToStream(stream, 1, 1, extraPtr);
}

static void FASTCALL WriteIndent(FILE * stream, int level)
{
	for(int l = 0; l < level; l++)
		fputc('\t', stream);
}

static void FASTCALL WriteBrace(FILE * stream, int isOpen)
{
	fputc(isOpen ? '{' : '}', stream);
	fputc('\n', stream);
}

static void FASTCALL WriteEqSign(FILE * stream)
{
	//fputc(' ', stream);
	fputc('=', stream);
	//fputc(' ', stream);
}

int PPDeclStruc::Helper_CreateListItem(long fldID, long typeID, uint * pIter, FILE * stream, int level, void * extraPtr)
{
	int    ok = -1;
	PPDeclStruc * p_decl = PPDeclStruc::CreateInstance(typeID, extraPtr, this, P_Logger);
	THROW(p_decl);
	THROW(p_decl->InitData(PPDeclStruc::idoAlloc, 0, 0));
	THROW(ok = CreateListItem(fldID, pIter, p_decl));
	if(ok > 0) {
		WriteIndent(stream, level);
		WriteBrace(stream, 1);
		THROW(p_decl->Helper_WriteDataToStream(stream, 0, level, extraPtr));
		WriteIndent(stream, level);
		WriteBrace(stream, 0);
	}
	CATCHZOK
	delete p_decl;
	return ok;
}

int PPDeclStruc::Helper_WriteDataToStream(FILE * stream, int putPrefix, int level, void * extraPtr)
{
	int    ok = 1, r;
	PPDeclStrucItem item;
	PPDeclStruc * p_decl = 0;
	SString buf;
	if(GetItem(0, &item) > 0) {
		if(putPrefix) {
			fputc('\n', stream);
			fputs("data", stream);
			fputc(' ', stream);
			fputs(item.Name, stream);
			fputc(' ', stream);
			WriteBrace(stream, 1);
		}
		for(uint i = 1; i < getCount(); i++) {
			r = 1;
			uint   j;
			buf.Z();
			GetItem(i, &item);
			if(item.Flags & DSIF_LIST) {
				WriteIndent(stream, level);
				fputs(item.Name, stream);
				WriteEqSign(stream);
				WriteBrace(stream, 1);
				if(item.Type >= FIRST_DECLSTRUC_ID) {
					for(j = 0; (r = Helper_CreateListItem(item.ID, item.Type, &j, stream, level+1, extraPtr)) > 0;)
						;
				}
				else
					for(j = 0; (r = TransferField(item.ID, tfdDataToBuf, &j, buf.Z())) > 0;) {
						WriteIndent(stream, level+1);
						if(buf.NotEmpty())
							fputs(buf, stream);
						fputc(';', stream);
						fputc('\n', stream);
					}
				WriteIndent(stream, level);
				WriteBrace(stream, 0);
			}
			else
				if(item.Type >= FIRST_DECLSTRUC_ID) {
					WriteIndent(stream, level);
					fputs(item.Name, stream);
					WriteEqSign(stream);
					fputc('\n', stream);
					r = Helper_CreateListItem(item.ID, item.Type, &(j = 0), stream, level+1, extraPtr);
					if(r < 0) {
						WriteIndent(stream, level+1);
						WriteBrace(stream, 1);
						WriteIndent(stream, level+1);
						WriteBrace(stream, 0);
					}
				}
				else if((r = TransferField(item.ID, tfdDataToBuf, &(j = 0), buf.Z())) > 0) {
					WriteIndent(stream, level);
					fputs(item.Name, stream);
					WriteEqSign(stream);
					if(buf.NotEmpty())
						fputs(buf, stream);
					fputc(';', stream);
					fputc('\n', stream);
				}
			THROW(r);
		}
		if(putPrefix)
			WriteBrace(stream, 0);
	}
	CATCHZOK
	delete p_decl;
	return ok;
}

int PPDeclStruc::SetHeader(PPID id, const char * pName, long flags)
{
	PPDeclStrucItem item;
	MEMSZERO(item);
	item.ID = id;
	STRNSCPY(item.Name, pName);
	item.Flags = flags;
	if(getCount() == 0) {
		return insert(&item) ? 1 : PPSetErrorSLib();
	}
	else {
		*static_cast<PPDeclStrucItem *>(at(0)) = item;
	}
	return 1;
}

int PPDeclStruc::AddItem(PPDeclStrucItem * pItem)
{
	if(getCount() == 0)
		if(!SetHeader(0, 0, 0))
			return 0;
	return insert(pItem) ? 1 : PPSetErrorSLib();
}

int PPDeclStruc::GetItem(uint idx, PPDeclStrucItem * pItem)
{
	if(idx < getCount()) {
		ASSIGN_PTR(pItem, *static_cast<const PPDeclStrucItem *>(at(idx)));
		return 1;
	}
	return 0;
}

void PPDeclStruc::SetItemAddedMsgString(const char * pName)
{
	SString msg;
	if(getCount()) {
		PPDeclStrucItem item;
		GetItem(0, &item);
		msg.Cat(item.Name).CatChar('-').CatChar('>');
	}
	msg.Cat(pName);
	PPSetAddedMsgString(msg);
}

int PPDeclStruc::SearchItem(const char * pName, PPDeclStrucItem * pItem)
{
	PPDeclStrucItem * p_item;
	for(uint i = 1; enumItems(&i, (void **)&p_item);) {
		if(sstreqi_ascii(p_item->Name, pName)) {
			ASSIGN_PTR(pItem, *p_item);
			return (PPErrCode = PPERR_DS_STRUCITEMEXISTS, 1);
		}
	}
	return (PPErrCode = PPERR_DS_STRUCITEMNFOUND, -1);
}

int PPDeclStruc::EnumItems(uint * pIdx, PPDeclStrucItem * pItem)
{
	PPDeclStrucItem * p_item;
	if(SArray::enumItems(pIdx, (void **)&p_item) > 0) {
		ASSIGN_PTR(pItem, *p_item);
		return 1;
	}
	else
		return 0;
}
