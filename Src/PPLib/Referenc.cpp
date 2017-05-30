// REFERENC.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

//static
int SLAPI Reference::Helper_EncodeOtherPw(const char * pEncPw, const char * pPw, size_t pwBufSize, SString & rResult)
{
	rResult = 0;
	int    ok = 1;
	const  size_t buf_quant = 256;
	assert(buf_quant >= pwBufSize);
	char   temp_pw[buf_quant], temp_str[buf_quant*3+8];
	STRNSCPY(temp_pw, pPw);
	IdeaEncrypt(pEncPw, temp_pw, pwBufSize);
	size_t i = 0, p = 0;
	for(; i < pwBufSize; i++) {
		sprintf(temp_str+p, "%03u", (uint8)temp_pw[i]);
		p += 3;
	}
	temp_str[p] = 0;
	rResult = temp_str;
	return ok;
}

//static
int SLAPI Reference::Helper_DecodeOtherPw(const char * pEncPw, const char * pPw, size_t pwBufSize, SString & rResult)
{
	rResult = 0;

	int    ok = 1;
	const  size_t buf_quant = 256;
	assert(buf_quant >= pwBufSize);
	char   temp_pw[buf_quant], temp_str[buf_quant*3+8];
	STRNSCPY(temp_str, pPw);
	if(strlen(temp_str) == (pwBufSize*3)) {
		for(size_t i = 0, p = 0; i < pwBufSize; i++) {
			char   nmb[16];
			nmb[0] = temp_str[p];
			nmb[1] = temp_str[p+1];
			nmb[2] = temp_str[p+2];
			nmb[3] = 0;
			temp_pw[i] = atoi(nmb);
			p += 3;
		}
		IdeaDecrypt(pEncPw, temp_pw, pwBufSize);
	}
	else {
		temp_pw[0] = 0;
		ok = 0;
	}
	rResult = temp_pw;
	IdeaRandMem(temp_pw, sizeof(temp_pw));
	return ok;
}

//static
int SLAPI Reference::Helper_Encrypt_(int cryptMethod, const char * pEncPw, const char * pText, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	if(cryptMethod == crymRef2) {
		SString temp_buf;
		char   pw_buf[128], pw_buf2[128];
		const  size_t bin_pw_size = (bufLen >= 34) ? 24 : (bufLen * 3 / 4 - 3);
		size_t pw_len = sstrlen(pText);
		IdeaRandMem(pw_buf2, sizeof(pw_buf2));
		if(pText)
			memcpy(pw_buf2, pText, pw_len+1);
		else
			pw_buf2[0] = 0;
		IdeaRandMem(pw_buf, sizeof(pw_buf));
		IdeaEncrypt(/*0*/pEncPw, pw_buf2, bin_pw_size);
		temp_buf.EncodeMime64(pw_buf2, bin_pw_size).CopyTo(pBuf, bufLen);
		if(temp_buf.Len() > (bufLen-1))
			ok = 0;
	}
	else if(cryptMethod == crymDefault) {
		IdeaRandMem(pBuf, bufLen);
		strnzcpy(pBuf, pText, bufLen);
		IdeaEncrypt(/*0*/pEncPw, pBuf, bufLen);
	}
	else
		ok = 0;
	return ok;
}

//static
int SLAPI Reference::Helper_Decrypt_(int cryptMethod, const char * pEncPw, const char * pBuf, size_t bufLen, SString & rText)
{
	int    ok = 1;
	char   pw_buf[128];
	rText = 0;
	if(cryptMethod == crymRef2) {
		SString temp_buf;
		size_t bin_pw_size = 0;
		temp_buf = pBuf;
		if(temp_buf.DecodeMime64(pw_buf, sizeof(pw_buf), &bin_pw_size) > 0) {
			IdeaDecrypt(/*0*/pEncPw, pw_buf, bin_pw_size);
			rText = pw_buf;
		}
		else
			ok = 0;
	}
	else if(cryptMethod == crymDefault) {
		memcpy(pw_buf, pBuf, bufLen);
		IdeaDecrypt(/*0*/pEncPw, pw_buf, bufLen);
		rText = pw_buf;
	}
	else
		ok = 0;
	return ok;
}

//static
int SLAPI Reference::Encrypt(int cryptMethod, const char * pText, char * pBuf, size_t bufLen)
{
	return Reference::Helper_Encrypt_(cryptMethod, 0, pText, pBuf, bufLen);
}

//static
int SLAPI Reference::Decrypt(int cryptMethod, const char * pBuf, size_t bufLen, SString & rText)
{
	return Reference::Helper_Decrypt_(cryptMethod, 0, pBuf, bufLen, rText);
}

//static
int SLAPI Reference::GetPassword(const PPSecur * pSecur, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	if(pSecur) {
		SString temp_buf;
		Reference::Decrypt(Reference::crymRef2, pSecur->Password, sizeof(pSecur->Password), temp_buf);
		temp_buf.CopyTo(pBuf, bufLen);
		(temp_buf = 0).CatCharN(' ', 96);
	}
	else
		ok = -1;
	return ok;
}

//static
int SLAPI Reference::VerifySecur(PPSecur2 * pSecur, int set)
{
	int    ok = 1;
	CRC32  c;
	size_t offs = offsetof(PPSecur2, Crc);
	uint32 crc = 0;
	{
		offs = offsetof(PPSecur2, Crc);
		crc = c.Calc(0, ((const uint8 *)pSecur)+8, offs-8);
		offs += sizeof(uint32);
		crc = c.Calc(crc, PTR8(pSecur) + offs, sizeof(PPSecur2) - offs);
	}
	if(set || pSecur->Crc != crc) {
		size_t len = strlen(pSecur->Name);
		if(len < sizeof(pSecur->Name))
			memzero(PTR8(pSecur->Name)+len, sizeof(pSecur->Name)-len);
		len = strlen(pSecur->Symb);
		if(len < sizeof(pSecur->Symb))
			memzero(PTR8(pSecur->Symb)+len, sizeof(pSecur->Symb)-len);
		{
			offs = offsetof(PPSecur2, Crc);
			crc = c.Calc(0, ((const uint8 *)pSecur)+8, offs-8);
			offs += sizeof(uint32);
			crc = c.Calc(crc, PTR8(pSecur) + offs, sizeof(PPSecur2) - offs);
		}
	}
	if(set) {
		pSecur->Crc = crc;
	}
	else if(pSecur->Crc != crc) {
		//
		// Исправление ошибки: вычисляем crc с учетом первых 8 байт записи
		//
		offs = offsetof(PPSecur2, Crc);
		uint32 crc2 = c.Calc(0, ((const uint8 *)pSecur), offs);
		offs += sizeof(uint32);
		crc2 = c.Calc(crc2, PTR8(pSecur) + offs, sizeof(PPSecur2) - offs);
		if(pSecur->Crc != crc2) {
			PPSetError(PPERR_INVPPSECURCRC);
			ok = 0;
		}
	}
	return ok;
}

//static
int SLAPI Reference::GetExField(const PPConfigPrivate * pRec, int fldId, SString & rBuf)
{
	int    ok = -1;
	rBuf = 0;
	if(fldId == PCFGEXSTR_DESKTOPNAME) {
		SString temp_buf = pRec->Tail;
		ok = PPGetExtStrData(fldId, temp_buf, rBuf);
	}
	return ok;
}

//static
int SLAPI Reference::SetExField(PPConfigPrivate * pRec, int fldId, const char * pBuf)
{
	int    ok = -1;
	if(fldId == PCFGEXSTR_DESKTOPNAME) {
		SString temp_buf = pRec->Tail;
		ok = PPPutExtStrData(fldId, temp_buf, pBuf);
		temp_buf.CopyTo(pRec->Tail, sizeof(pRec->Tail));
	}
	return ok;
}

SLAPI Reference::Reference() : ReferenceTbl()
{
}

SLAPI Reference::~Reference()
{
}

int SLAPI Reference::AllocDynamicObj(PPID * pDynObjType, const char * pName, long flags, int use_ta)
{
	int    ok = 1, r;
	ReferenceTbl::Rec rec;
	PPID   id = *pDynObjType;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if((r = _GetFreeID(PPOBJ_DYNAMICOBJS, &id, PPOBJ_FIRSTDYN)) > 0) {
			MEMSZERO(rec);
			if(pName)
				STRNSCPY(rec.ObjName, pName);
			rec.Val1 = flags;
			THROW(AddItem(PPOBJ_DYNAMICOBJS, &id, &rec, 0));
			*pDynObjType = id;
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::FreeDynamicObj(PPID dynObjType, int use_ta)
{
	int    ok = 1, r;
	PPID   id = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(GetItem(PPOBJ_DYNAMICOBJS, dynObjType) > 0);
		THROW_DB(deleteRec());
		THROW(RemoveProp(PPOBJ_DYNAMICOBJS, dynObjType, 0, 0));
		for(id = 0; (r = EnumItems(dynObjType, &id)) > 0;) {
			THROW_DB(deleteRec());
			THROW(RemoveProp(dynObjType, id, 0, 0));
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::_GetFreeID(PPID objType, PPID * pID, PPID firstID)
{
	int    ok, r2;
	if(*pID) {
		ok = -_Search(objType, *pID, spEq, 0);
	}
	else {
		long   inc = 0;
		PPID   potential_key = 0;
		ObjSyncTbl::Rec objsync_rec;
		THROW(ok = _Search(objType+1, 0, spLt, 0));
		if(ok > 0 && data.ObjType == objType)
			potential_key = MAX(firstID, data.ObjID+1);
		else
			potential_key = firstID;
		//
		// Проверяем не является ли новый идент дубликатом удаленного до этого
		// (и имеющего общий синхронизирующий идентификатор). Если "да", то
		// сдвигаем значение на единицу до тех пор, пока не найдем подходящее значение.
		//
		while((r2 = DS.GetTLA().P_ObjSync->SearchPrivate(objType, potential_key+inc, 0, &objsync_rec)) > 0) {
			//
			// Мы должны одновременно прочитать запись (блокировка страницы в транзакции) и удостовериться //
			// в том, что наш ключ не перехвачен другим пользователем
			//
			do {
				++inc;
			} while(_Search(objType, potential_key+inc, spEq, 0) > 0);
		}
		THROW(r2);
		if(inc > 0) {
			potential_key += inc;
			ok = 2;
			if(CConfig.Flags & CCFLG_DEBUG) {
				SString msg_buf, fmt_buf, obj_title;
				GetObjectTitle(objType, obj_title);
				msg_buf.Printf(PPLoadTextS(PPTXT_LOG_ADDOBJREC_JUMPED_ID, fmt_buf), obj_title.cptr());
				PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
		ASSIGN_PTR(pID, potential_key);
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::GetFreeID(PPID obj, PPID * id)
{
	return _GetFreeID(obj, id, PP_FIRSTUSRREF);
}

int SLAPI Reference::AddItem(PPID obj, PPID * pID, const void * b, int use_ta)
{
	int    ok = 1, r;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = GetFreeID(obj, pID));
		THROW_PP(r > 0, PPERR_REFISBUSY);
		copyBufFrom(b);
		data.ObjType = obj;
		data.ObjID   = *pID;
		THROW_DB(insertRec());
		DS.LogAction(PPACN_OBJADD, obj, data.ObjID, 0, 0);
		THROW(tra.Commit());
	}
	CATCH
		PPObject::SetLastErrObj(obj, *pID);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI Reference::UpdateItem(PPID obj, PPID id, const void * b, int logAction /*=1*/, int use_ta)
{
	int    ok = 1, r = 1, try_count = 5;
	ReferenceTbl::Rec prev_rec, new_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		do {
			ReferenceTbl::Key0 k;
			k.ObjType = obj;
			k.ObjID   = id;
			if(search(0, &k, spEq)) { // @v8.1.4 searchForUpdate-->search
				copyBufTo(&prev_rec);
				new_rec = *(ReferenceTbl::Rec *)b;
				new_rec.ObjType = obj;
				new_rec.ObjID   = id;
				// @v8.3.6 if(memcmp(&prev_rec, &new_rec, sizeof(new_rec)) != 0) {
				if(!fields.IsEqualRecords(&prev_rec, &new_rec)) { // @v8.3.6
					// @v8.1.4 {
					DBRowId _dbpos;
					THROW_DB(getPosition(&_dbpos));
					THROW_DB(getDirectForUpdate(0, &k, _dbpos));
					// } @v8.1.4
					r = updateRecBuf(&new_rec); // @sfu
					if(!r) {
						THROW_DB(BtrError == BE_CONFLICT && try_count > 0);
						//
						// Если встречаем ошибку "Конфликт блокировок на уровне записи", то
						// повторяем попытку чтения-изменения try_count раз.
						//
						unlock(0); // @v9.0.4
						SDelay(10);
						--try_count;
					}
					if(logAction)
						DS.LogAction(PPACN_OBJUPD, obj, id, 0, 0);
				}
				else
					ok = -1;
			}
			else if(BTRNFOUND)
				ok = (PPSetObjError(PPERR_OBJNFOUND, obj, id), 0);
			else
				ok = PPSetErrorDB();
		} while(r == 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::_Search(PPID obj, PPID id, int spMode, void * b)
{
	int    ok = 1;
	ReferenceTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	if(search(0, &k, spMode))
		copyBufTo(b);
	else if(BTRNFOUND)
		ok = (PPSetObjError(PPERR_OBJNFOUND, obj, id), -1);
	else
		ok = PPSetErrorDB();
	return ok;
}

int SLAPI Reference::GetItem(PPID obj, PPID id, void * b)
{
	return _Search(obj, id, spEq, b);
}

int SLAPI Reference::EnumItems(PPID obj, PPID * pID, void * b)
{
	int    r = _Search(obj, *pID, spGt, b);
	return (r > 0) ? ((data.ObjType == obj) ? (*pID = data.ObjID, 1) : -1) : r;
}

int SLAPI Reference::InitEnum(PPID objType, int flags, long * pHandle)
{
	BExtQuery * q = new BExtQuery(this, 0);
	// @todo (требуется доработка BExtQuery) q->setMaxReject(8);
	if(flags & (eoIdName|eoIdSymb)) {
		q->select(this->ObjID, 0L);
		if(flags & eoIdName)
			q->addField(this->ObjName);
		if(flags & eoIdSymb)
			q->addField(this->Symb);
	}
	else
		q->selectAll();
	q->where(this->ObjType == objType);
	ReferenceTbl::Key0 k0;
	k0.ObjType = objType;
	k0.ObjID   = 0;
	q->initIteration(0, &k0, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

int SLAPI Reference::InitEnumByIdxVal(PPID objType, int valN, long val, long * pHandle)
{
	int    ok = 1;
	assert(oneof2(valN, 1, 2));
	if(oneof2(valN, 1, 2)) {
		int    idx = 0;
		union {
			ReferenceTbl::Key2 k2;
			ReferenceTbl::Key3 k3;
		} k;
		DBQ * dbq = &(this->ObjType == objType);
		if(valN == 1) {
			idx = 2;
			k.k2.ObjType = objType;
			k.k2.Val1 = val;
			dbq = & (*dbq && this->Val1 == val);
		}
		else if(valN == 2) {
			idx = 3;
			k.k3.ObjType = objType;
			k.k3.Val2 = val;
			dbq = & (*dbq && this->Val2 == val);
		}
		BExtQuery * q = new BExtQuery(this, idx);
		q->selectAll().where(*dbq);
		q->initIteration(0, &k, spGe);
		ok = EnumList.RegisterIterHandler(q, pHandle);
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int SLAPI Reference::NextEnum(long enumHandle, void * pRec)
{
	return (EnumList.NextIter(enumHandle) > 0) ? (copyBufTo(pRec), 1) : -1;
}

int SLAPI Reference::DestroyIter(long enumHandle)
{
	return EnumList.DestroyIterHandler(enumHandle);
}

SEnumImp * SLAPI Reference::Enum(PPID objType, int options)
{
	long   h = -1;
	return InitEnum(objType, options, &h) ? new PPTblEnum <Reference>(this, h) : 0;
}

SEnumImp * SLAPI Reference::EnumByIdxVal(PPID objType, int valN, long val)
{
	long   h = -1;
	return InitEnumByIdxVal(objType, valN, val, &h) ? new PPTblEnum<Reference>(this, h) : 0;
}

int SLAPI Reference::LoadItems(PPID objType, SArray * pList)
{
	int    ok = -1;
	ReferenceTbl::Rec rec;
	for(SEnum en = Enum(objType, 0); en.Next(&rec);)
		ok = (pList && !pList->insert(&rec)) ? PPSetError(PPERR_NOMEM) : 1;
	return ok;
}

int SLAPI Reference::SearchName(PPID obj, PPID * pID, const char * pName, void * pRec)
{
	ReferenceTbl::Key1 k1;
	MEMSZERO(k1);
	k1.ObjType = obj;
	STRNSCPY(k1.ObjName, pName);
	int    ok = SearchByKey(this, 1, &k1, pRec);
	if(ok > 0) {
		ASSIGN_PTR(pID, data.ObjID);
	}
	else if(ok < 0) {
		if(obj == PPOBJ_GLOBALUSERACC) {
			PPSetError(PPERR_GLOBALUSERNAMENFOUND, pName);
		}
	}
	return ok;
}

int SLAPI Reference::SearchSymb(PPID objType, PPID * pID, const char * pSymb, size_t offs)
{
	int    ok = -1;
	ASSIGN_PTR(pID, 0);
	if(!isempty(pSymb)) {
		long   h = -1;
		ReferenceTbl::Rec rec;
		for(InitEnum(objType, 0, &h); ok < 0 && NextEnum(h, &rec) > 0;) {
			if(stricmp866(pSymb, ((char*)&rec) + offs) == 0) {
				ASSIGN_PTR(pID, rec.ObjID);
				ok =  1;
			}
		}
		DestroyIter(h);
	}
	if(ok < 0) {
		SString msg_buf;
		PPSetError(PPERR_OBJBYSYMBNFOUND, GetObjectTitle(objType, msg_buf).CatDiv(':', 1).Cat(pSymb));
	}
	return ok;
}

int SLAPI Reference::CheckUniqueSymb(PPID objType, PPID id, const char * pSymb, size_t offs)
{
	int    ok = 1;
	if(pSymb && pSymb[0] != 0) {
		long   h = -1;
		ReferenceTbl::Rec rec;
		for(InitEnum(objType, 0, &h); ok && NextEnum(h, &rec) > 0;)
			if(stricmp866(pSymb, ((char*)&rec) + offs) == 0 && rec.ObjID != id)
				ok = (PPSetObjError(PPERR_DUPSYMB, objType, rec.ObjID), 0);
		DestroyIter(h);
	}
	return ok;
}

int SLAPI Reference::RemoveItem(PPID obj, PPID id, int use_ta)
{
	int    ok = 1, r;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		r = SendObjMessage(DBMSG_OBJDELETE, 0, obj, id);
		THROW(r != DBRPL_ERROR);
		THROW_PP(r != DBRPL_CANCEL, PPERR_USERBREAK);
		THROW_DB(deleteFrom(this, 0, (this->ObjType == obj && this->ObjID == id)));
		THROW(RemoveProp(obj, id, 0, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::RemoveProp(PPID obj, PPID id, PPID prop, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(prop == 0) {
			THROW_DB(deleteFrom(&Prop, 0, (Prop.ObjType == obj && Prop.ObjID == id))); // @v6.1.6
		}
		else {
			THROW_DB(deleteFrom(&Prop, 0, (Prop.ObjType == obj && Prop.ObjID == id && Prop.Prop == prop))); // @v6.1.6
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::ReadPropBuf(void * b, size_t s, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size = 0;
	SBuffer temp_buf;
	RECORDSIZE fix_rec_size = 0;
	Prop.getRecSize(&fix_rec_size);
	temp_buf.Write(&Prop.data, fix_rec_size);
	Prop.readLobData(Prop.VT, temp_buf);
	actual_size = temp_buf.GetAvailableSize();
	temp_buf.Read(b, s);
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

int SLAPI Reference::PreparePropBuf(PPID obj, PPID id, PPID prop, const void * b, uint s)
{
	int    ok = 1;
	RECORDSIZE fix_rec_size = 0;
	Prop.getRecSize(&fix_rec_size);
	if(b) {
		Prop.copyBufFrom(b, fix_rec_size);
		if(s)
			THROW(Prop.writeLobData(Prop.VT, PTR8(b)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0));
	}
	else {
		THROW(Prop.setLobSize(Prop.VT, (s > fix_rec_size) ? (s-fix_rec_size) : 0));
	}
	Prop.data.ObjType = obj;
	Prop.data.ObjID   = id;
	Prop.data.Prop    = prop;
	CATCHZOK
	return ok;
}

int SLAPI Reference::PutProp(PPID obj, PPID id, PPID prop, const void * b, size_t s, int use_ta)
{
	int    ok = 1;
	PropertyTbl::Key0 k;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(b) {
			k.ObjType = obj;
			k.ObjID   = id;
			k.Prop    = prop;
			if(Prop.searchForUpdate(0, &k, spEq)) {
				THROW(PreparePropBuf(obj, id, prop, b, s));
				THROW_DB(Prop.updateRec()); // @sfu
			}
			else if(BTROKORNFOUND) {
				THROW(PreparePropBuf(obj, id, prop, b, s));
				THROW_DB(Prop.insertRec());
			}
			else {
				CALLEXCEPT_PP(PPERR_DBENGINE);
			}
		}
		else {
			// @v8.2.9 {
			k.ObjType = obj;
			k.ObjID   = id;
			k.Prop    = prop;
			if(Prop.search(0, &k, spEq)) {
				do {
					THROW_DB(Prop.rereadForUpdate(0, &k));
					THROW_DB(Prop.deleteRec());
				} while(Prop.search(0, &k, spNext) && Prop.data.ObjType == obj && Prop.data.ObjID == id && Prop.data.Prop == prop);
			}
			// } @v8.2.9
			// @v8.2.9 THROW_DB(deleteFrom(&Prop, 0, (Prop.ObjType == obj && Prop.ObjID == id && Prop.Prop == prop)));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::_SearchProp(PPID obj, PPID id, PPID prop, int spMode, void * b, size_t s)
{
	int    ok = 1;
	PropertyTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	k.Prop    = prop;
	if(Prop.search(0, &k, spMode))
		ReadPropBuf(b, s, 0);
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI Reference::GetProp(PPID obj, PPID id, PPID prop, void * b, size_t s)
{
	return _SearchProp(obj, id, prop, spEq, b, s);
}

int SLAPI Reference::GetPropActualSize(PPID obj, PPID id, PPID prop, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size = 0;
	PropertyTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	k.Prop    = prop;
	if(Prop.search(0, &k, spEq)) {
		ReadPropBuf(0, 0, &actual_size);
	}
	else
		ok = PPDbSearchError();
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

int SLAPI Reference::EnumProps(PPID obj, PPID id, PPID * prop, void * b, uint s)
{
	int    sp = (*prop == 0) ? spGe : spNext;
	int    r = _SearchProp(obj, id, *prop, sp, b, s);
	if(r > 0) {
		r = (Prop.data.ObjType == obj && Prop.data.ObjID == id) ? 1 : -1;
		*prop = Prop.data.Prop;
	}
	return r;
}

struct PropVlrString {
	PPID   ObjType;
	PPID   ObjID;
	PPID   PropID;
	long   Size;
	//char   Text[];
};

int SLAPI Reference::GetPropVlrString(PPID obj, PPID id, PPID prop, SString & rBuf)
{
	rBuf = 0;
	int    ok = 1;
	PropVlrString * pm = 0;
	PropertyTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	k.Prop    = prop;
	if(Prop.search(0, &k, spEq)) {
		size_t actual_size = 0, test_actual_size = 0;
		RECORDSIZE fix_size = 0;
		Prop.getRecSize(&fix_size);
		Prop.getLobSize(Prop.VT, &actual_size);
		actual_size += fix_size;
		THROW_MEM(pm = (PropVlrString*)SAlloc::M(actual_size + 32)); // +32 - страховка
		ReadPropBuf(pm, actual_size, &test_actual_size);
		assert(actual_size == test_actual_size);
		(rBuf = (const char *)(pm + 1)).Strip();
	}
	else
		ok = PPDbSearchError();
	CATCHZOK
	SAlloc::F(pm);
	return ok;
}

int SLAPI Reference::PutPropVlrString(PPID obj, PPID id, PPID prop, const char * b, int use_ta)
{
	int    ok = 1;
	PropVlrString * pm = 0;
	uint   s = 0;
	if(!isempty(b)) {
		uint sz = strlen(b) + 1;
		s = MAX(sizeof(PropVlrString) + sz, PROPRECFIXSIZE);
		THROW_MEM(pm = (PropVlrString*)SAlloc::M(s));
		memzero(pm, s);
		strcpy((char*)(pm + 1), b);
		pm->Size = sz;
	}
	THROW(PutProp(obj, id, prop, pm, s, use_ta));
	CATCHZOK
	SAlloc::F(pm);
	return ok;
}

int SLAPI Reference::PutPropSBuffer(PPID obj, PPID id, PPID prop, const SBuffer & rBuf, int use_ta)
{
	int    ok = 1;
	PropVlrString * pm = 0;
	uint   s = 0;
	uint   sz = rBuf.GetAvailableSize();
	if(sz) {
		s = MAX(sizeof(PropVlrString) + sz, PROPRECFIXSIZE);
		THROW_MEM(pm = (PropVlrString*)SAlloc::M(s));
		memzero(pm, s);
		THROW_SL(rBuf.ReadStatic((void *)(pm + 1), sz));
		pm->Size = sz;
	}
	THROW(PutProp(obj, id, prop, pm, s, use_ta));
	CATCHZOK
	SAlloc::F(pm);
	return ok;
}

int FASTCALL Reference::GetPropSBuffer_Current(SBuffer & rBuf)
{
	int    ok = 1;
	size_t actual_size = 0, test_actual_size = 0;
	RECORDSIZE fix_size = 0;
	PropVlrString * pm = 0;
	Prop.getRecSize(&fix_size);
	Prop.getLobSize(Prop.VT, &actual_size);
	actual_size += fix_size;
	THROW_MEM(pm = (PropVlrString*)SAlloc::M(actual_size + 32)); // +32 - страховка
	ReadPropBuf(pm, actual_size, &test_actual_size);
	assert(actual_size == test_actual_size);
	// @v9.1.11 if(actual_size == test_actual_size && actual_size == (pm->Size+sizeof(*pm))) {
	if(actual_size == test_actual_size && actual_size == MAX((pm->Size+sizeof(*pm)), PROPRECFIXSIZE)) { // @v9.1.11
		THROW_SL(rBuf.Write((const void *)(pm + 1), pm->Size));
	}
	else {
		SString added_msg_buf;
		added_msg_buf.Cat("Oid, prop, size").CatDiv(':', 2).CatChar('[').Cat(pm->ObjType).CatDiv(',', 2).Cat(pm->ObjID).
			CatDiv(',', 2).Cat(pm->PropID).CatDiv(',', 2).Cat(pm->Size).CatChar(']');
		PPSetError(PPERR_READPROPBUFSIZE, added_msg_buf);
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
		ok = -1;
	}
	CATCHZOK
	SAlloc::F(pm);
	return ok;
}

int SLAPI Reference::GetPropSBuffer(PPID obj, PPID id, PPID prop, SBuffer & rBuf)
{
	int    ok = 1;
	PropertyTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	k.Prop    = prop;
	if(Prop.search(0, &k, spEq)) {
		THROW(ok = GetPropSBuffer_Current(rBuf));
	}
	else
		ok = PPDbSearchError();
	CATCHZOK
	return ok;
}

int SLAPI Reference::GetPropArrayFromRecBuf(SArray * pAry)
{
	int    ok = 1;
	PropPPIDArray * p_rec = 0;
	if(pAry) {
		const uint item_size = pAry->getItemSize();
		size_t actual_size = 0, test_actual_size = 0;
		RECORDSIZE fix_size = 0;
		Prop.getRecSize(&fix_size);
		Prop.getLobSize(Prop.VT, &actual_size);
		actual_size += fix_size;

		THROW_MEM(p_rec = (PropPPIDArray *)SAlloc::C(1, actual_size + 32)); // +32 - страховка
		ReadPropBuf(p_rec, actual_size, &test_actual_size);
		assert(actual_size == test_actual_size);
		for(int i = 0; i < p_rec->Count; i++) {
			size_t offs = i * item_size;
			if((sizeof(*p_rec) + offs + item_size) <= actual_size) {
				THROW_SL(pAry->insert(PTR8(p_rec+1) + offs));
			}
			else {
				// @err
			}
		}
	}
	CATCH
		ok = 0;
		pAry->freeAll();
	ENDCATCH
	SAlloc::F(p_rec);
	return ok;
}

int SLAPI Reference::GetPropArray(PPID obj, PPID id, PPID prop, SArray * pAry)
{
	int    ok = 1;
	PropertyTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	k.Prop    = prop;
	if(Prop.search(0, &k, spEq)) {
		THROW(GetPropArrayFromRecBuf(pAry));
	}
	else
		ok = PPDbSearchError();
	CATCH
		ok = 0;
		pAry->freeAll();
	ENDCATCH
	return ok;
}

int SLAPI Reference::PutPropArray(PPID obj, PPID id, PPID prop, const SArray * ary, int use_ta)
{
	int    ok = 1;
	PropPPIDArray * p_rec = 0;
	uint   i;
	uint   count = ary ? ary->getCount() : 0;
	size_t sz = 0;
	if(count > 0) {
		const  uint minCount = (PROPRECFIXSIZE - sizeof(PropPPIDArray)) / ary->getItemSize();
		sz = sizeof(PropPPIDArray) + (MAX(count, minCount) * ary->getItemSize());
		SETMAX(sz, PROPRECFIXSIZE);
		THROW_MEM(p_rec = (PropPPIDArray*)SAlloc::C(1, sz));
		p_rec->Count = count;
		for(i = 0; i < count; i++) {
			size_t offs = i*ary->getItemSize();
			memcpy((int8*)(p_rec+1) + offs, ary->at(i), ary->getItemSize());
		}
	}
	THROW(PutProp(obj, id, prop, p_rec, sz, use_ta));
	CATCHZOK
	SAlloc::F(p_rec);
	return ok;
}

int SLAPI Reference::GetConfig(PPID obj, PPID id, PPID cfgID, void * b, uint s)
{
	int    r = GetProp(obj, id, cfgID, b, s);
	if(r < 0 && oneof2(obj, PPOBJ_USRGRP, PPOBJ_USR))
		if(GetItem(obj, id) > 0) {
			PPID   prev_level_id = (obj == PPOBJ_USRGRP) ? PPOBJ_CONFIG : PPOBJ_USRGRP;
			r = GetConfig(prev_level_id, ((PPSecur*)&data)->ParentID, cfgID, b, s); // @recursion
		}
		else {
			PPSetAddedMsgObjName(obj, id);
			r = PPSetError(PPERR_CFGOBJRDFAULT);
		}
	return r;
}

int SLAPI Reference::SetConfig(PPID obj, PPID id, PPID cfgID, void * b, uint s)
{
	return PutProp(obj, id, cfgID, b, s);
}

int SLAPI Reference::LoadSecur(PPID obj, PPID id, PPSecurPacket * sp)
{
	int    ok = 1;
	THROW(GetItem(obj, id, &sp->Secur) > 0);
	THROW(Reference::VerifySecur(&sp->Secur, 0));
	THROW(DS.FetchConfig(obj, id, &sp->Config));
	THROW(sp->Rights.Get(obj, id));
	THROW(sp->Paths.Get(obj, id));
	CATCHZOK
	return ok;
}

int SLAPI Reference::EditSecur(PPID obj, PPID id, PPSecurPacket * sp, int isNew)
{
	int   ok = 1;
	int   is_user = (obj == PPOBJ_USR);
	{
		PPTransaction tra(1);
		THROW(tra);
		if(is_user) {
			if(isNew)
				sp->Secur.PwUpdate = getcurdate_();
			else {
				THROW_DB(GetItem(obj, id) > 0);
				if(memcmp(sp->Secur.Password, ((PPSecur*)&data)->Password, sizeof(sp->Secur.Password)))
					sp->Secur.PwUpdate = getcurdate_();
			}
		}
		THROW(Reference::VerifySecur(&sp->Secur, 1));
		THROW(isNew ? AddItem(obj, &id, &sp->Secur, 0) : UpdateItem(obj, id, &sp->Secur, 1, 0));
		THROW(sp->Paths.Put(obj, id));
		if(is_user && sp->Secur.Flags & USRF_INHCFG) {
			THROW(RemoveProp(obj, id, PPPRP_CFG, 0));
		}
		else {
			THROW(SetConfig(obj, id, PPPRP_CFG, &sp->Config, sizeof(sp->Config)));
		}
		if(is_user && sp->Secur.Flags & USRF_INHRIGHTS) {
			THROW(sp->Rights.Remove(obj, id));
		}
		else {
			THROW(sp->Rights.Put(obj, id));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Reference::RemoveSecur(PPID obj, PPID id, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(obj == PPOBJ_USR) {
			THROW(RemoveItem(obj, id, 0));
			DS.LogAction(PPACN_OBJRMV, obj, id, 0, 0);
		}
		else if(obj == PPOBJ_USRGRP) {
			PPIDArray usr_list;
			PPSecur usr_rec;
			for(SEnum en = EnumByIdxVal(PPOBJ_USR, 1, id); en.Next(&usr_rec) > 0;) {
				if(usr_rec.ParentID == id) // @paranoic
					usr_list.add(usr_rec.ID);
			}
			usr_list.sortAndUndup(); // @paranoic
			for(uint i = 0; i < usr_list.getCount(); i++) {
				const PPID usr_id = usr_list.get(i);
				THROW(RemoveSecur(PPOBJ_USR, usr_id, 0)); // @recursion for users
			}
			THROW(RemoveItem(obj, id, 0));
			DS.LogAction(PPACN_OBJRMV, obj, id, 0, 0);
		}
		else if(obj == PPOBJ_CONFIG) {
			CALLEXCEPT_PP(PPERR_NORIGHTS);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// Права доступа
//
struct _PPRights {         // @persistent @store(PropertyTbl)
	PPID   SecurObj;       //
	PPID   SecurID;        //
	PPID   RightsID;       // Const = (PPPRP_BIAS + 0)
	ushort WeekDays;       // Допустимые дни работы (биты: 0 - вскр, 1 - пнд ...)
	LTIME  TimeBeg;        // Допустимое начало рабочей сессии (0..23:59:59)
	LTIME  TimeEnd;        // Допустимый конец рабочей сессии
	uchar  PwMinLen;       // Минимальная длина пароля
	uchar  PwPeriod;       // Продолжительность действия пароля (дней)
	long   CFlags;         // PPAccessRestriction::cfXXX Общие флаги прав доступу
	//LDATE  LowRBillDate;   // Дата, до которой R-доступ к документам запрещен
	//LDATE  UppRBillDate;   // Дата, после которой R-доступ к документам запрещен
	DateRange RBillPeriod;
	short  AccessLevel;    // Уровень доступа
	char   Reserve2[6];    // @reserve
	ushort ORTailSize;     // Размер хвоста с правами доступа по объектам
	//LDATE  LowWBillDate;   // Дата, до которой W-доступ к документам запрещен
	//LDATE  UppWBillDate;   // Дата, после которой W-доступ к документам запрещен
	DateRange WBillPeriod;
	ulong  ChkSumOpList;   // Контрольная сумма списка доступных операций
	ulong  ChkSumLocList;  // Контрольная сумма списка доступных складов
	ulong  ChkSumCfgList;  // Контрольная сумма списка доступных конфигураций
	ulong  ChkSumAccList;  // Контрольная сумма списка доступных счетов
	uint8  RtDesktop;      // Права доступа к рабочим столам (на изменение, на создание)
		// Если RtDesktop & 0x80, то флаги перенесены в поле CFlags
	uint8  Reserve;        // @reserve
	long   OnlyGoodsGrpID; // Единственная товарная группа, с которой может оперировать пользователь.
	ushort Flags;          // Общие флаги
	ushort OprFlags;       // Операционные флаги
	ulong  CheckSum;       //
	// ... (ObjRights: size = ORTailSize - sizeof(_PPRights)) //
};

SLAPI PPRights::PPRights()
{
	P_Rt = 0;
	P_OpList = 0;
	P_LocList = 0;
	P_CfgList = 0;
	P_AccList = 0;
	P_PosList = 0; // @v8.9.1
	P_QkList = 0; // @v8.9.1
}

SLAPI PPRights::~PPRights()
{
	Empty();
}

size_t SLAPI PPRights::Size() const
{
	return P_Rt ? (sizeof(_PPRights) + P_Rt->ORTailSize) : 0;
}

void SLAPI PPRights::Empty()
{
	ZFREE(P_Rt);
	ZDELETE(P_OpList);
	ZDELETE(P_LocList);
	ZDELETE(P_CfgList);
	ZDELETE(P_AccList);
	ZDELETE(P_PosList); // @v8.9.1
	ZDELETE(P_QkList); // @v8.9.1
}

int SLAPI PPRights::IsEmpty() const
{
	return (P_Rt == 0);
}

int SLAPI PPRights::IsInherited() const
{
	return BIN(P_Rt && P_Rt->OprFlags & PPORF_INHERITED);
}

int SLAPI PPRights::Merge(const PPRights & rS, long flags)
{
	int    ok = 1;
	//
	// Функция Merge сливает права доступа this с rS таким образом, что
	// this получает максимум прав, заданных одновременно и в this и в rS
	//
	CALLPTRMEMB(P_OpList, Merge(rS.P_OpList, ObjRestrictArray::moEmptyListIsFull));
	CALLPTRMEMB(P_LocList, Merge(rS.P_LocList, ObjRestrictArray::moEmptyListIsFull));
	//
	// Пустой список доступных конфигураций означет отсутствие доступа
	//
	if(P_CfgList) {
		P_CfgList->Merge(rS.P_CfgList, 0);
	}
	else if(rS.P_CfgList) {
		P_CfgList = new ObjRestrictArray(*rS.P_CfgList);
	}
	{
		P_Rt->WeekDays |= rS.P_Rt->WeekDays;
		if(rS.P_Rt->TimeBeg < P_Rt->TimeBeg)
			P_Rt->TimeBeg = rS.P_Rt->TimeBeg;
		if(rS.P_Rt->TimeEnd > P_Rt->TimeEnd)
			P_Rt->TimeEnd = rS.P_Rt->TimeEnd;
		SETMAX(P_Rt->PwPeriod, rS.P_Rt->PwPeriod);
		P_Rt->CFlags |= rS.P_Rt->CFlags;
		/*
		SETMIN(P_Rt->LowRBillDate, rS.P_Rt->LowRBillDate);
		SETMAX(P_Rt->UppRBillDate, rS.P_Rt->UppRBillDate);
		SETMIN(P_Rt->LowWBillDate, rS.P_Rt->LowWBillDate);
		SETMAX(P_Rt->UppWBillDate, rS.P_Rt->UppWBillDate);
		*/
		SETMIN(P_Rt->RBillPeriod.low, rS.P_Rt->RBillPeriod.low);
		SETMAX(P_Rt->RBillPeriod.upp, rS.P_Rt->RBillPeriod.upp);
		SETMIN(P_Rt->WBillPeriod.low, rS.P_Rt->WBillPeriod.low);
		SETMAX(P_Rt->WBillPeriod.upp, rS.P_Rt->WBillPeriod.upp);
		P_Rt->Flags |= rS.P_Rt->Flags;
		P_Rt->OprFlags |= rS.P_Rt->OprFlags;
		//
		const _PPRights * p_s = rS.P_Rt;
		for(uint s = 0; s < rS.P_Rt->ORTailSize;) {
			const ObjRights * p_so = (ObjRights*)(PTR8(p_s + 1) + s);
			if(!(p_so->OprFlags & PPORF_INHERITED)) {
				int   _found = 0;
				for(uint t = 0; !_found && t < P_Rt->ORTailSize;) {
					ObjRights * p_o = (ObjRights*)(PTR8(P_Rt + 1) + t);
					if(p_o->ObjType == p_so->ObjType) {
						p_o->Flags |= p_so->Flags;
						p_o->OprFlags |= p_so->OprFlags;
						_found = 1;
					}
					t += p_o->Size;
				}
				if(!_found) {
					THROW(SetObjRights(p_so->ObjType, p_so, 1));
				}
			}
			s += p_so->Size;
		}
	}
	CATCHZOK
	return ok;
}

static int FASTCALL AssignObjRestrictArray(ObjRestrictArray ** ppDest, const ObjRestrictArray * pSrc)
{
	int    ok = 1;
	ZDELETE(*ppDest);
	if(pSrc)
		*ppDest = new ObjRestrictArray(*pSrc);
	return ok;
}

PPRights & FASTCALL PPRights::operator = (const PPRights & src)
{
	Resize(src.Size());
	if(P_Rt)
		memmove(P_Rt, src.P_Rt, src.Size());
	AssignObjRestrictArray(&P_OpList, src.P_OpList);
	AssignObjRestrictArray(&P_LocList, src.P_LocList);
	AssignObjRestrictArray(&P_CfgList, src.P_CfgList);
	AssignObjRestrictArray(&P_AccList, src.P_AccList);
	AssignObjRestrictArray(&P_PosList, src.P_PosList);
	AssignObjRestrictArray(&P_QkList, src.P_QkList);
	return *this;
}

struct ObjRights_Pre855 { // @persistent
	PPID   ObjType;
	ushort Size;
	ushort Flags;
	ushort OprFlags;
};

int SLAPI PPRights::ReadRights(PPID securType, PPID securID, int ignoreCheckSum)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    r = 0;
	size_t sz = 2048; // @v8.9.1 1024-->2048
	THROW(Resize(sz));
	THROW(r = p_ref->GetConfig(securType, securID, PPPRP_RTCOMM, P_Rt, sz));
	if(r > 0) {
		if(Size() > sz) {
			THROW(Resize(sz = Size()));
			THROW(p_ref->GetConfig(securType, securID, PPPRP_RTCOMM, P_Rt, sz) > 0);
		}
		if(!ignoreCheckSum)
			THROW_PP(CheckSum() == P_Rt->CheckSum, PPERR_INVRTCHKSUM);
		{
			//
			// Процедура конвертации прав доступа по объектам в формат v8.5.5 (увеличился фиксированный размер ObjRights)
			//
			int    do_convert = 0;
			uint   s = 0;
			while(!do_convert && s < P_Rt->ORTailSize) {
				const ObjRights * o = (const ObjRights *)(PTR8(P_Rt + 1) + s);
				if(o->Size == sizeof(ObjRights_Pre855))
					do_convert = 1;
				s += o->Size;
			}
			if(do_convert) {
				uint8  temp_buffer[128];
				_PPRights * p_temp_r = (_PPRights *)SAlloc::M(Size());
				THROW_MEM(p_temp_r);
				memcpy(p_temp_r, P_Rt, Size());
				THROW(Resize(sizeof(_PPRights)));
				for(s = 0; s < p_temp_r->ORTailSize;) {
					const ObjRights * o = (const ObjRights*)(PTR8(p_temp_r + 1) + s);
					ObjRights * p_temp = (ObjRights *)temp_buffer;
					if(o->Size == sizeof(ObjRights_Pre855)) {
						const ObjRights_Pre855 * p_temp_pre855 = (const ObjRights_Pre855 *)o;
						p_temp->ObjType = p_temp_pre855->ObjType;
						p_temp->Size = sizeof(ObjRights);
						p_temp->Flags = p_temp_pre855->Flags;
						p_temp->OprFlags = (uint32)p_temp_pre855->OprFlags;
					}
					else {
						assert(o->Size <= sizeof(temp_buffer));
						memcpy(temp_buffer, o, o->Size);
					}
					if(!(p_temp->OprFlags & PPORF_DEFAULT))
						THROW(SetObjRights(p_temp->ObjType, p_temp, 0));
					s += o->Size;
				}
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPRights::Get(PPID securType, PPID securID, int ignoreCheckSum)
{
	int    ok = 1, r;
	size_t sz = 1024;
	ulong  chksum = 0;
	ObjRestrictArray temp_orlist;

	THROW(r = ReadRights(securType, securID, ignoreCheckSum));
	if(r > 0) {
		if(P_Rt->SecurObj < securType)
			P_Rt->OprFlags |= PPORF_INHERITED;
		if(oneof2(P_Rt->SecurObj, PPOBJ_USRGRP, PPOBJ_USR)) {
			//
			// Необходимо унаследовать от более высоких уровней иерархии
			// права по объектам, которые не определены на заданном уровне
			// (Рекурсия)
			//
			PPRights temp;
			PPID   prevType = (P_Rt->SecurObj == PPOBJ_USRGRP) ? PPOBJ_CONFIG : PPOBJ_USRGRP;
			PPID   prevID   = ((PPSecur*)&PPRef->data)->ParentID;
			uint   s = 0;
			THROW(temp.Get(prevType, prevID, ignoreCheckSum)); // @recursion
			while(s < temp.P_Rt->ORTailSize) {
				const ObjRights * o = (const ObjRights*)(PTR8(temp.P_Rt + 1) + s);
				if(!(o->OprFlags & PPORF_DEFAULT))
					THROW(SetObjRights(o->ObjType, o, 0));
				s += o->Size;
			}
		}
		THROW(Resize(Size()));
		{
			class OraReader {
			public:
				PPID   ObjType;
				PPID   ObjID;
				int    IgnoreCheckSum;

				OraReader(PPID objType, PPID objID, int ignoreCheckSum)
				{
					ObjType = objType;
					ObjID = objID;
					IgnoreCheckSum = ignoreCheckSum;
				}
				int Read(PPID propID, ulong * pCheckSum, ObjRestrictArray ** ppList)
				{
					int    ok = 1;
					ObjRestrictArray temp_orlist;
					THROW(PPRef->GetPropArray(ObjType, ObjID, propID, &temp_orlist));
					if(!IgnoreCheckSum && pCheckSum && *pCheckSum) {
						ulong  chksum = _checksum__((char*)temp_orlist.dataPtr(), temp_orlist.getCount() * temp_orlist.getItemSize());
						THROW_PP(chksum == *pCheckSum, PPERR_INVRTCHKSUM);
					}
					if(temp_orlist.getCount()) {
						ZDELETE(*ppList);
						THROW_MEM(*ppList = new ObjRestrictArray(temp_orlist));
					}
					CATCHZOK
					return ok;
				}
			};
			OraReader orar(P_Rt->SecurObj, P_Rt->SecurID, ignoreCheckSum);
			THROW(orar.Read(PPPRP_RT_OPLIST,  &P_Rt->ChkSumOpList, &P_OpList));
			THROW(orar.Read(PPPRP_RT_LOCLIST, &P_Rt->ChkSumLocList, &P_LocList));
			THROW(orar.Read(PPPRP_RT_CFG,     &P_Rt->ChkSumCfgList, &P_CfgList));
			THROW(orar.Read(PPPRP_RT_ACCLIST, &P_Rt->ChkSumAccList, &P_AccList));
			THROW(orar.Read(PPPRP_RT_POSLIST, 0, &P_PosList));
			THROW(orar.Read(PPPRP_RT_QKLIST,  0, &P_QkList));
		}
	}
	else {
		THROW(Resize(sizeof(_PPRights)));
		memzero(P_Rt, sizeof(_PPRights));
		P_Rt->SecurObj = securType;
		P_Rt->SecurID  = securID;
		P_Rt->RightsID = PPPRP_RTCOMM;
		P_Rt->WeekDays = 0x007f;
		P_Rt->Flags    = PPRights::GetDefaultFlags();
		P_Rt->OprFlags = (0xffff & ~PPORF_INHERITED); // @v8.3.3 PPRights::GetDefaultOprFlags()-->(0xffff & ~PPORF_INHERITED)
		P_Rt->RtDesktop = 0;
		P_OpList  = 0;
		P_LocList = 0;
		P_CfgList = 0;
		P_AccList = 0;
		P_PosList = 0; // @v8.9.1
		P_QkList = 0; // @v8.9.1
	}
	CATCHZOK
	return ok;
}

int SLAPI PPRights::Remove(PPID securType, PPID securID)
{
	return PPRef->RemoveProp(securType, securID, PPPRP_RTCOMM, 0);
}

int SLAPI PPRights::Put(PPID securType, PPID securID)
{
	int    ok = 1;
	if(!IsEmpty()) {
		Reference * p_ref = PPRef;
		P_Rt->OprFlags &= ~(PPORF_DEFAULT | PPORF_INHERITED);
		P_Rt->SecurObj  = securType;
		P_Rt->SecurID   = securID;
		P_Rt->RightsID  = PPPRP_RTCOMM;
		memzero(P_Rt->Reserve2, sizeof(P_Rt->Reserve2));
		//
		// Удаляем все права по объектам с признаком "по умолчанию"
		// и у оставшихся сбрасываем признак "наследованный"
		//
		for(uint s = 0; s < P_Rt->ORTailSize;) {
			ObjRights * o = (ObjRights *)(PTR8(P_Rt + 1) + s);
			if(o->OprFlags & PPORF_DEFAULT) {
				THROW(SetObjRights(o->ObjType, 0));
			}
			else {
				// @v8.3.3 o->OprFlags &= ~PPORF_INHERITED;
				s += o->Size;
			}
		}
		P_Rt->CheckSum = CheckSum();
		if(P_OpList)
			P_Rt->ChkSumOpList = _checksum__((char*)P_OpList->dataPtr(), P_OpList->getCount() * P_OpList->getItemSize());
		else
			P_Rt->ChkSumOpList = _checksum__(0, 0);
		if(P_LocList)
			P_Rt->ChkSumLocList = _checksum__((char*)P_LocList->dataPtr(), P_LocList->getCount() * P_LocList->getItemSize());
		else
			P_Rt->ChkSumLocList = _checksum__(0, 0);
		if(P_CfgList)
			P_Rt->ChkSumCfgList = _checksum__((char*)P_CfgList->dataPtr(), P_CfgList->getCount() * P_CfgList->getItemSize());
		else
			P_Rt->ChkSumCfgList = _checksum__(0, 0);
		if(P_AccList)
			P_Rt->ChkSumAccList = _checksum__((char*)P_AccList->dataPtr(), P_AccList->getCount() * P_AccList->getItemSize());
		else
			P_Rt->ChkSumAccList = _checksum__(0, 0);
		THROW(p_ref->SetConfig(securType, securID, PPPRP_RTCOMM, P_Rt, Size()));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_OPLIST, P_OpList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_LOCLIST, P_LocList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_CFG, P_CfgList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_ACCLIST, P_AccList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_POSLIST, P_PosList, 0)); // @v8.9.1
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_QKLIST,  P_QkList, 0));  // @v8.9.1
	}
	CATCHZOK
	return ok;
}

const ObjRights * SLAPI PPRights::GetConstObjRights(PPID objType, ObjRights * pDef) const
{
	const ObjRights * p_result = 0;
	if(P_Rt) {
		for(uint s = 0; !p_result && s < P_Rt->ORTailSize;) {
			const ObjRights * o = (ObjRights*)(PTR8(P_Rt + 1) + s);
			if(o->ObjType == objType)
				p_result = o;
			else
				s += o->Size;
		}
		if(!p_result && pDef) {
			pDef->ObjType = objType;
			pDef->Size = sizeof(ObjRights);
			pDef->Flags = PPRights::GetDefaultFlags();
			pDef->OprFlags = PPRights::GetDefaultOprFlags();
			p_result = pDef;
		}
	}
	return p_result;
}

ObjRights * SLAPI PPRights::GetObjRights(PPID objType, int use_default) const
{
	ObjRights * p_result = 0;
	if(P_Rt) {
		for(uint s = 0; !p_result && s < P_Rt->ORTailSize;) {
			ObjRights * o = (ObjRights *)(PTR8(P_Rt + 1) + s);
			const uint osz = o->Size;
			if(o->ObjType == objType) {
				// @v8.9.8 p_result = new (osz) ObjRights;
				p_result = ObjRights::Create(objType, osz); // @v8.9.8
				memcpy(p_result, o, osz);
			}
			else
				s += osz;
		}
		if(!p_result && use_default) {
			// @v8.9.8 p_result = new ObjRights;
			p_result = ObjRights::Create(objType, 0); // @v8.9.8
			/* @v8.9.8
			p_result->ObjType = objType;
			p_result->Size = sizeof(ObjRights);
			p_result->Flags = PPRights::GetDefaultFlags();
			p_result->OprFlags = PPRights::GetDefaultOprFlags();
			*/
		}
	}
	return p_result;
}

int SLAPI PPRights::Resize(uint sz)
{
	int    ok = 1;
	if(P_Rt && sz == 0)
		ZFREE(P_Rt);
	else {
		size_t prev_size = Size();
		sz = (sz < sizeof(_PPRights)) ? sizeof(_PPRights) : sz;
		P_Rt = (_PPRights*)SAlloc::R(P_Rt, sz);
		if(P_Rt) {
			if(sz > prev_size)
				memzero(PTR8(P_Rt) + prev_size, sz - prev_size);
			P_Rt->ORTailSize = sz - sizeof(_PPRights);
		}
		else
			ok = PPSetErrorNoMem();
	}
	return ok;
}

int SLAPI PPRights::SetObjRights(PPID objType, const ObjRights * rt, int replace)
{
	int    ok = 1;
	if(P_Rt) {
		uint8 * cp = PTR8(P_Rt + 1);
		size_t hs = sizeof(_PPRights);
		size_t ts = P_Rt->ORTailSize;
		size_t s  = 0;
		int    found = 0;
		while(s < ts && !found) {
			ObjRights * o = (ObjRights*)(cp + s);
			size_t os = o->Size;
			if(o->ObjType == 0 || os == 0) {
				//
				// Такая ситуация встречаться не должна, но на всякий
				// случай обработать ее стоит. В этом случае будем
				// считать, что достигли конца структуры и не нашли
				// права для заданного объекта.
				//
				ts = s;
				P_Rt->ORTailSize = (ushort)s;
				break;
			}
			else if(o->ObjType == objType) {
				if(rt == 0 || rt->Size == 0) {
					//
					// Remove item and resize buffer
					//
					memmove(cp + s, cp + s + os, ts - s - os);
					THROW(Resize(hs + ts - os));
				}
				else if(replace || rt->Size != os) {
					if(rt->Size == os) {
						//
						// Simply copy new data
						//
						memmove(o, rt, rt->Size);
					}
					else {
						//
						// Resize buffer and copy new data
						//
						THROW(Resize(hs + ts + rt->Size - os));
						cp = PTR8(P_Rt + 1);
						memmove(cp + s + rt->Size, cp + s + os, ts - s - os);
						memmove(cp + s, rt, rt->Size);
					}
				}
				found = 1;
			}
			else
				s += os;
		}
		if(!found && rt && rt->Size) {
			THROW(Resize(hs + ts + rt->Size));
			cp = PTR8(P_Rt + 1);
			memmove(cp + s, rt, rt->Size);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPRights::SetAccessRestriction(PPAccessRestriction * accsr)
{
	if(accsr && P_Rt) {
		P_Rt->WeekDays    = accsr->WeekDays;
		P_Rt->TimeBeg     = accsr->TimeBeg;
		P_Rt->TimeEnd     = accsr->TimeEnd;
		P_Rt->PwMinLen    = (uint8)accsr->PwMinLen;
		P_Rt->PwPeriod    = (uint8)accsr->PwPeriod;
		//accsr->GetRBillPeriod(&period);
		//P_Rt->LowRBillDate = period.low;
		//P_Rt->UppRBillDate = period.upp;
		accsr->GetRBillPeriod(&P_Rt->RBillPeriod);
		//accsr->GetWBillPeriod(&period);
		//P_Rt->LowWBillDate = period.low;
		//P_Rt->UppWBillDate = period.upp;
		accsr->GetWBillPeriod(&P_Rt->WBillPeriod);
		P_Rt->AccessLevel = accsr->AccessLevel;
		P_Rt->OnlyGoodsGrpID = accsr->OnlyGoodsGrpID;
		P_Rt->CFlags = accsr->CFlags;
		P_Rt->RtDesktop = 0x80;
		memzero(P_Rt->Reserve2, sizeof(P_Rt->Reserve2));
		P_Rt->Reserve = 0;
		return 1;
	}
	return 0;
}

ulong SLAPI PPRights::CheckSum()
{
	ulong  cs = 0;
	if(P_Rt) {
		ulong  save_chksum      = P_Rt->CheckSum;
		ulong  save_opl_chksum  = P_Rt->ChkSumOpList;
		ulong  sum   = P_Rt->CheckSum;
		ulong  save_locl_chksum = P_Rt->ChkSumLocList;
		ulong  save_cfgl_chksum = P_Rt->ChkSumCfgList;
		ulong  save_accl_chksum = P_Rt->ChkSumAccList;
		P_Rt->CheckSum      = 0x45654765UL;
		P_Rt->ChkSumOpList  = 0;
		P_Rt->ChkSumLocList = 0;
		P_Rt->ChkSumCfgList = 0;
		P_Rt->ChkSumAccList = 0;
		cs = _checksum__((char*)P_Rt, Size());
		P_Rt->ChkSumLocList = save_locl_chksum;
		P_Rt->ChkSumOpList  = save_opl_chksum;
		P_Rt->ChkSumCfgList = save_cfgl_chksum;
		P_Rt->ChkSumAccList = save_accl_chksum;
		P_Rt->CheckSum      = save_chksum;
	}
	return cs;
}

PPAccessRestriction & SLAPI PPRights::GetAccessRestriction(PPAccessRestriction & rAcsr) const
{
	if(P_Rt) {
		rAcsr.WeekDays    = P_Rt->WeekDays;
		rAcsr.TimeBeg     = P_Rt->TimeBeg;
		rAcsr.TimeEnd     = P_Rt->TimeEnd;
		rAcsr.PwMinLen    = P_Rt->PwMinLen;
		rAcsr.PwPeriod    = P_Rt->PwPeriod;
		rAcsr.SetBillPeriod(&P_Rt->RBillPeriod, PPAccessRestriction::pparR);
		rAcsr.SetBillPeriod(&P_Rt->WBillPeriod, PPAccessRestriction::pparW);
		rAcsr.AccessLevel  = P_Rt->AccessLevel;
		rAcsr.OnlyGoodsGrpID = P_Rt->OnlyGoodsGrpID;
		rAcsr.CFlags = P_Rt->CFlags;
		if(!(P_Rt->RtDesktop & 0x80)) {
			SETFLAG(rAcsr.CFlags, PPAccessRestriction::cfDesktopCr, P_Rt->RtDesktop & PPR_INS);
			SETFLAG(rAcsr.CFlags, PPAccessRestriction::cfDesktopMod, P_Rt->RtDesktop & PPR_MOD);
		}
	}
	else {
		MEMSZERO(rAcsr);
		rAcsr.WeekDays = 0x007f;
	}
	return rAcsr;
}

int SLAPI PPRights::CheckBillDate(LDATE dt, int forRead) const
{
	int    ok = 1;
	if(!IsEmpty()) {
		DateRange  bill_period;
		PPAccessRestriction accsr;
		GetAccessRestriction(accsr);
		if(forRead) {
			accsr.GetRBillPeriod(&bill_period);
			ok = bill_period.CheckDate(dt) ? 1 : PPSetError(PPERR_BILLDATERANGE);
		}
		else {
			accsr.GetWBillPeriod(&bill_period);
			if(!AdjustBillPeriod(bill_period, 1))
				ok = 0;
			else if(dt < bill_period.low || (bill_period.upp && dt > bill_period.upp))
				ok = PPSetError(PPERR_BILLDATERANGE);
		}
	}
	return ok;
}

//int SLAPI PPRights::AdjustBillPeriod(LDATE * pBeg, LDATE * pEnd) const
int SLAPI PPRights::AdjustBillPeriod(DateRange & rPeriod, int checkOnly) const
{
	int    ok = 1;
	if(!IsEmpty() && !PPMaster) {
		DateRange r_bill_period;
		DateRange in_period;
		in_period = rPeriod;
		in_period.Actualize(ZERODATE); // @v8.7.7
		LDATE b = in_period.low;
		LDATE e = in_period.upp;
		PPAccessRestriction accsr;
		GetAccessRestriction(accsr).GetRBillPeriod(&r_bill_period);
		r_bill_period.Actualize(ZERODATE); // @v8.7.7
		if(r_bill_period.low)
			b = MAX(b, r_bill_period.low);
		if(r_bill_period.upp)
			if(e)
				e = MIN(e, r_bill_period.upp);
			else
				e = r_bill_period.upp;
		ok = (b <= e || e == 0) ? 1 : PPSetError(PPERR_NORTPERIOD);
		if(!checkOnly)
			rPeriod.Set(b, e);
	}
	return ok;
}

int SLAPI PPRights::AdjustCSessPeriod(DateRange & rPeriod, int checkOnly) const
{
	int    ok = 1;
	if(!IsEmpty() && !PPMaster) {
		PPAccessRestriction accsr;
		GetAccessRestriction(accsr);
		if(accsr.CFlags & accsr.cfApplyBillPeriodsToCSess) {
			DateRange r_bill_period;
			DateRange in_period;
			in_period = rPeriod;
			in_period.Actualize(ZERODATE);
			LDATE b = in_period.low;
			LDATE e = in_period.upp;
			accsr.GetRBillPeriod(&r_bill_period);
			r_bill_period.Actualize(ZERODATE);
			if(r_bill_period.low)
				b = MAX(b, r_bill_period.low);
			if(r_bill_period.upp)
				if(e)
					e = MIN(e, r_bill_period.upp);
				else
					e = r_bill_period.upp;
			ok = (b <= e || e == 0) ? 1 : PPSetError(PPERR_NORTPERIOD);
			if(!checkOnly)
				rPeriod.Set(b, e);
		}
	}
	return ok;
}

int SLAPI PPRights::IsOpRights() const
{
	return BIN(P_OpList && P_OpList->getCount());
}

int SLAPI PPRights::IsLocRights() const
{
	return BIN(P_LocList && P_LocList->getCount());
}

int SLAPI PPRights::MaskOpRightsByOps(PPIDArray * pOpList, PPIDArray * pResultOpList) const
{
	if(IsOpRights()) {
		ObjRestrictItem * p_item;
		for(uint i = 0; P_OpList->enumItems(&i, (void**)&p_item);) {
			if(!pOpList || pOpList->lsearch(p_item->ObjID))
				if(!pResultOpList->add(p_item->ObjID))
					return 0;
		}
		return 1;
	}
	if(pOpList)
		pResultOpList->copy(*pOpList);
	else {
		for(PPID op_id; EnumOperations(0, &op_id) > 0;)
			if(!pResultOpList->add(op_id))
				return 1;
	}
	return -1;
}

int SLAPI PPRights::MaskOpRightsByTypes(PPIDArray * pOpTypeList, PPIDArray * pResultOpList) const
{
	if(IsOpRights()) {
		ObjRestrictItem * p_item;
		for(uint i = 0; P_OpList->enumItems(&i, (void**)&p_item);) {
			PPID op_type_id = GetOpType(p_item->ObjID);
			if(pOpTypeList->lsearch(op_type_id))
				if(!pResultOpList->add(p_item->ObjID))
					return 0;
		}
		return 1;
	}
	else {
		SArray op_rec_list(sizeof(PPOprKind));
		if(PPRef->LoadItems(PPOBJ_OPRKIND, &op_rec_list)) {
			PPOprKind * p_op_rec;
			for(uint i = 0; op_rec_list.enumItems(&i, (void **)&p_op_rec);)
				if(pOpTypeList->lsearch(p_op_rec->OpTypeID))
					if(!pResultOpList->add(p_op_rec->ID))
						return 0;
		}
		else
			return 0;
		return -1;
	}
}

int SLAPI PPRights::ExtentOpRights()
{
	if(IsOpRights()) {
		ObjRestrictArray temp_list;
		ObjRestrictItem * p_item;
		PPIDArray gen_op_list;
		for(uint i = 0; P_OpList->enumItems(&i, (void**)&p_item);) {
			if(IsGenericOp(p_item->ObjID) > 0) {
				gen_op_list.clear();
				GetGenericOpList(p_item->ObjID, &gen_op_list);
				for(uint j = 0; j < gen_op_list.getCount(); j++)
					temp_list.UpdateItemByID(gen_op_list.at(j), p_item->Flags);
			}
			temp_list.UpdateItemByID(p_item->ObjID, p_item->Flags);
		}
		P_OpList->copy(temp_list);
	}
	return 1;
}

int SLAPI PPRights::CheckOpID(PPID opID, long rtflags) const
{
	int    ok = 1;
	if(IsOpRights()) {
		uint   pos = 0;
		if(!P_OpList->SearchItemByID(opID, &pos)) {
			SString added_msg;
			GetOpName(opID, added_msg);
			ok = PPSetError(PPERR_OPNOTACCESSIBLE, added_msg);
		}
		else if(rtflags) {
			const ObjRestrictItem & r_item = P_OpList->at(pos);
			if(r_item.Flags & 0x80000000) {
				if((r_item.Flags & rtflags) != rtflags) {
					SString added_msg, temp_buf;
					GetOpName(opID, added_msg);
					added_msg.CatDiv('-', 1);
					if(rtflags & PPR_READ && !(r_item.Flags & PPR_READ)) {
						PPLoadString("rt_view", temp_buf);
						added_msg.CatBrackStr(temp_buf);
					}
					if(rtflags & PPR_INS && !(r_item.Flags & PPR_INS)) {
						PPLoadString("rt_create", temp_buf);
						added_msg.CatBrackStr(temp_buf);
					}
					if(rtflags & PPR_MOD && !(r_item.Flags & PPR_MOD)) {
						PPLoadString("rt_modif", temp_buf);
						added_msg.CatBrackStr(temp_buf);
					}
					if(rtflags & PPR_DEL && !(r_item.Flags & PPR_DEL)) {
						PPLoadString("rt_delete", temp_buf);
						added_msg.CatBrackStr(temp_buf);
					}
					ok = PPSetError(PPERR_ISNTPRVLGFOROP, added_msg);
				}
			}
		}
	}
	return ok;
}

int SLAPI PPRights::CheckLocID(PPID locID, long) const
{
	return (!(P_LocList && P_LocList->getCount()) || P_LocList->SearchItemByID(locID, 0)) ?
		1 : PPSetError(PPERR_LOCNOTACCESSIBLE);
}

int SLAPI PPRights::CheckPosNodeID(PPID id, long flags) const
{
	return (!(P_PosList && P_PosList->getCount()) || P_PosList->SearchItemByID(id, 0)) ?
		1 : PPSetError(PPERR_POSNODENOTACCESSIBLE);
}

int SLAPI PPRights::CheckQuotKindID(PPID id, long flags) const
{
	return (!(P_QkList && P_QkList->getCount()) || P_QkList->SearchItemByID(id, 0)) ?
		1 : PPSetError(PPERR_QUOTKINDNOTACCESSIBLE);
}

int SLAPI PPRights::CheckAccID(PPID accID, long rt) const
{
	int    ok = 1;
	if(P_AccList && P_AccList->getCount()) {
		uint pos = 0;
		if(P_AccList->SearchItemByID(accID, &pos) || P_AccList->SearchItemByID(0, &pos))
			ok = ((P_AccList->at(pos).Flags & rt) == rt) ? 1 : PPSetError(PPERR_ACCNOTACCESSIBLE);
		else
			ok = PPSetError(PPERR_ACCNOTACCESSIBLE);
	}
	return ok;
}

int SLAPI PPRights::CheckDesktopID(long deskID, long rt) const
{
	int    ok = 1;
	PPAccessRestriction accsr;
	if((GetAccessRestriction(accsr).CFlags & rt) != rt)
		ok = PPSetError(PPERR_NORIGHTS);
	return ok;
}

// static
ushort SLAPI PPRights::GetDefaultFlags()
{
	return (0xffff & ~PPR_ADM);
}

// static
long SLAPI PPRights::GetDefaultOprFlags()
{
	return 0xffffffff; // @v8.3.3 (0xffff & ~PPORF_INHERITED)-->0xffff // @v8.9.3 0xffff-->0xffffffff
}

int SLAPI GetCommConfig(PPCommConfig * pCfg)
{
	int    ok = 1, r = 0;
	memzero(pCfg, sizeof(*pCfg));
	THROW(r = PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_COMMCFG, pCfg, sizeof(PPCommConfig)));
	if(r <= 0) {
		memzero(pCfg, sizeof(*pCfg));
		pCfg->Tag          = PPOBJ_CONFIG;
		pCfg->ID           = PPCFG_MAIN;
		pCfg->Prop         = PPPRP_COMMCFG;
		pCfg->SupplAcct.ac = DEFAC_SUPPL;
		pCfg->SellAcct.ac  = DEFAC_SELL;
		pCfg->CashAcct.ac  = DEFAC_CASH;
	}
	if(pCfg->MainOrgID == 0) {
		PPObjPerson psn_obj;
		PersonTbl::Rec psn_rec;
		if((r = psn_obj.P_Tbl->SearchMainOrg(&psn_rec)) > 0)
			pCfg->MainOrgID = psn_rec.ID;
	}
	CATCHZOK
	return ok;
}

int SLAPI SetCommConfig(PPCommConfig * pCfg, int use_ta)
{
	return PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_COMMCFG, pCfg, sizeof(*pCfg), use_ta);
}
//
// PPSecurPacket
//
SLAPI PPSecurPacket::PPSecurPacket(PPID obj, PPID id)
{
	MEMSZERO(Secur);
	// @v9.4.9 (constructor) MEMSZERO(Config);
	Secur.Tag = obj;
	Secur.ID  = id;
}

PPSecurPacket & FASTCALL PPSecurPacket::operator = (const PPSecurPacket & src)
{
	Secur  = src.Secur;
	Config = src.Config;
	Paths  = src.Paths;
	Rights = src.Rights;
	return *this;
}
//
// PPPrinterCfg
//
static const char * RpUseDuplexPrinting = "UseDuplexPrinting";

int SLAPI PPGetPrinterCfg(PPID obj, PPID id, PPPrinterCfg * pCfg)
{
	int    ok = 1;
	int    use_duplex_printing = 0;
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1); // @v9.2.0 readonly 0-->1
		uint32 val = 0;
		if(reg_key.GetDWord(RpUseDuplexPrinting, &val))
			use_duplex_printing = val ? 1 : 0;
	}
	if(obj == 0 && id == 0) {
		obj = PPOBJ_USR;
		id  = LConfig.User;
	}
	if(PPRef->GetConfig(obj, id, PPPRP_PRINTER, pCfg, sizeof(*pCfg)) > 0) {
		ok = 1;
	}
	else {
		memzero(pCfg, sizeof(*pCfg));
		ok = -1;
	}
	if(pCfg) {
		SETFLAG(pCfg->Flags, PPPrinterCfg::fUseDuplexPrinting, use_duplex_printing);
		SETIFZ(pCfg->PrnCmdSet, SPCMDSET_EPSON);
	}
	return ok;
}

int SLAPI PPSetPrinterCfg(PPID obj, PPID id, PPPrinterCfg * pCfg)
{
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
		uint32 val = (pCfg->Flags & PPPrinterCfg::fUseDuplexPrinting) ? 1 : 0;
		reg_key.PutDWord(RpUseDuplexPrinting, val);
	}
	if(obj == 0 && id == 0) {
		obj = PPOBJ_USR;
		id  = LConfig.User;
	}
	return PPRef->SetConfig(obj, id, PPPRP_PRINTER, pCfg, sizeof(*pCfg));
}
//
//
//

#if 0 // {
//static
int FASTCALL UuidRefCore::TextToUuid(const char * pText, S_GUID & rUuid)
{
	int    ok = 1;
    if(isempty(pText)) {
		rUuid.SetZero();
		ok = -1;
    }
    else if(!rUuid.FromStr(pText))
		ok = 0;
	return ok;
}

//static
int FASTCALL UuidRefCore::UuidToText(const S_GUID & rUuid, SString & rText)
{
	int    ok = 1;
	if(rUuid.IsZero()) {
		rText = 0;
		ok = -1;
	}
	else {
		rUuid.ToStr(S_GUID::fmtPlain, rText);
	}
	return ok;
}
#endif // } 0

SLAPI UuidRefCore::UuidRefCore() : UuidRefTbl()
{
	P_Hash = 0;
}

SLAPI UuidRefCore::~UuidRefCore()
{
	delete P_Hash;
}

int SLAPI UuidRefCore::InitCache()
{
	int    ok = -1;
    if(!P_Hash) {
		P_Hash = new GuidHashTable(8*1024*1024);
		ok = P_Hash ? 1 : PPSetErrorNoMem();
    }
    return ok;
}

int SLAPI UuidRefCore::Search(long id, S_GUID & rUuid)
{
	UuidRefTbl::Rec rec;
    int    ok = SearchByID(this, PPOBJ_UUIDREF, id, &rec);
    //TextToUuid(((ok > 0) ? rec.UUID : 0), rUuid);
    rUuid = *(S_GUID *)rec.UUID;
	return ok;
}

int SLAPI UuidRefCore::SearchUuid(const S_GUID & rUuid, int useCache, long * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(useCache && P_Hash) {
		uint   hval = 0;
		uint   hpos = 0;
		if(P_Hash->Search(rUuid, &hval, &hpos)) {
			id = (long)hval;
			ok = 1;
		}
	}
	if(ok < 0) {
		UuidRefTbl::Rec rec;
		UuidRefTbl::Key1 k1;
		MEMSZERO(k1);
		//SString temp_buf;
		//UuidToText(rUuid, temp_buf);
		//STRNSCPY(k1.UUID, temp_buf);
		memcpy(k1.UUID, &rUuid, sizeof(S_GUID));
		THROW(ok = SearchByKey(this, 1, &k1, &rec));
		if(ok > 0) {
			id = rec.ID;
			if(useCache) {
				THROW(InitCache());
                THROW_SL(P_Hash->Add(rUuid, (uint)id, 0));
			}
		}
	}
	CATCHZOK
    ASSIGN_PTR(pID, id);
    return ok;
}

int SLAPI UuidRefCore::GetUuid(const S_GUID & rUuid, long * pID, int options, int use_ta)
{
	//
	// Если option & sgoOptimistic то не предпринимаем попыток предварительного поиска записи (считая, что
	// с высокой вероятностью такого GUID в таблице нет. Если он все-таки есть, то реагируем
	// на ошибку BE_DUP попыткой найти эту запись.
	// Цель опции - снизить затраты времени на поиск (поиск по GUID очень медленный)
	//
	int    ok = 1;
	if(rUuid.IsZero()) {
		ASSIGN_PTR(pID, 0);
		ok = 2;
	}
	else {
		if(options & sgoOptimistic) {
			ok = -1;
		}
		else {
			THROW(ok = SearchUuid(rUuid, BIN(options & sgoHash), pID));
		}
		if(ok < 0) {
			int    r = 0;
			PPTransaction tra(use_ta);
			THROW(tra);
			{
				//SString temp_buf;
				UuidRefTbl::Rec rec;
				MEMSZERO(rec);

				//UuidToText(rUuid, temp_buf);
                //STRNSCPY(rec.UUID, temp_buf);
                memcpy(rec.UUID, &rUuid, sizeof(S_GUID));

                r = insertRecBuf(&rec, 0, pID);
				if(!r) {
					THROW_DB((options & sgoOptimistic) && BtrError == BE_DUP);
					THROW(SearchUuid(rUuid, BIN(options & sgoHash), pID) > 0);
				}
			}
			THROW(tra.Commit());
			if(r && options & sgoHash) {
				THROW(InitCache());
				THROW_SL(P_Hash->Add(rUuid, (uint)*pID, 0));
			}
		}
	}
    CATCHZOK
	return ok;
}

int SLAPI UuidRefCore::PutChunk(TSArray <S_GUID> & rChunk, uint maxCount, int use_ta)
{
	int    ok = 1;
	const  uint cc = rChunk.getCount();
	if(cc > maxCount) {
        PPTransaction tra(use_ta);
        THROW(tra);
        {
        	BExtInsert bei(this);
        	for(uint i = 0; i < cc; i++) {
				UuidRefTbl::Rec rec;
				MEMSZERO(rec);
				memcpy(rec.UUID, &rChunk.at(i), sizeof(S_GUID));
				THROW_DB(bei.insert(&rec));
        	}
        	THROW_DB(bei.flash());
        }
        THROW(tra.Commit());
        ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI UuidRefCore::RemoveUuid(S_GUID & rUuid, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	THROW(SearchUuid(rUuid, 0, &id) > 0);
	THROW(RemoveByID(this, id, use_ta));
	CATCHZOK
	return ok;
}

int SLAPI UuidRefCore::Remove(long id, int use_ta)
{
	return RemoveByID(this, id, use_ta);
}
//
//
//
TextRefIdent::TextRefIdent()
{
	THISZERO();
}

TextRefIdent::TextRefIdent(PPID objType, PPID objID, int16 prop)
{
	O.Obj = objType;
	O.Id = objID;
	P = prop;
	L = 0;
}

int TextRefIdent::operator !() const
{
	return BIN(O.Obj == 0 && O.Id == 0);
}
//
//
//
class SelfTextRefCache : public ObjCache {
public:
	SelfTextRefCache();
	int     SLAPI FetchText(const char * pText, PPID * pID);
	void    FASTCALL SetTable(TextRefCore * pT);
private:
	virtual int  SLAPI FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
	{
		return -1;
	}
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
	}
	ReadWriteLock TcRwl;
	SymbHashTable TextCache;
	TextRefCore * P_T; // @notowned
};

SelfTextRefCache::SelfTextRefCache() : ObjCache(PPOBJ_SELFREFTEXT, sizeof(ObjCacheEntry)), TextCache(4 * 1024 * 1024)
{
	P_T = 0;
}

void FASTCALL SelfTextRefCache::SetTable(TextRefCore * pT)
{
	P_T = pT;
}

int SLAPI SelfTextRefCache::FetchText(const char * pText, PPID * pID)
{
	int    ok = -1;
	long   _id = 0;
	SString pattern;
	pattern = pText;
	if(pattern.Len()) {
		pattern.ToLower1251();
		uint   hval = 0;
		uint   hpos = 0;
		TcRwl.ReadLock();
		if(TextCache.Search(pattern, &hval, &hpos)) {
			_id = (long)hval;
			ok = 1;
		}
		else {
			if(P_T) {
				TcRwl.Unlock();
				TcRwl.WriteLock();
				if(TextCache.Search(pattern, &hval, &hpos)) { // Повторная попытка после получения блокировки
					_id = (long)hval;
					ok = 1;
				}
				else {
					SString utf_buf = pattern;
					SStringU pattern_u;
					pattern_u.CopyFromUtf8(utf_buf.ToUtf8());
					ok = P_T->SearchSelfRefText(pattern_u, &_id);
					if(ok > 0 && _id) {
						if(!TextCache.Add(pattern, (uint)_id))
							ok = PPSetErrorSLib();
					}
				}
			}
			else
				ok = -2;
		}
		TcRwl.Unlock();
	}
	ASSIGN_PTR(pID, _id);
	return ok;
}

int SLAPI TextRefCore::FetchSelfRefText(const char * pText, PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(!isempty(pText)) {
		SelfTextRefCache * p_cache = GetDbLocalCachePtr <SelfTextRefCache> (PPOBJ_SELFREFTEXT);
		if(p_cache) {
			p_cache->SetTable(this);
			ok = p_cache->FetchText(pText, &id);
			p_cache->SetTable(0);
		}
		else {
			SString utf_buf;
			SStringU pattern_u;
			pattern_u.CopyFromUtf8((utf_buf = pText).ToLower1251().ToUtf8());
			ok = SearchSelfRefText(pattern_u, &id);
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}
//
//
//
SLAPI TextRefCore::TextRefCore() : TextRefTbl()
{
}

int SLAPI TextRefCore::GetLastObjId(PPID objType, int prop, PPID * pID)
{
    int    ok = 1;
    PPID   id = 0;
    TextRefTbl::Key1 k1;
    MEMSZERO(k1);
    k1.ObjType = (int16)objType;
    k1.Prop = (int16)prop;
    k1.ObjID = MAXLONG;
    if(search(1, &k1, spLe) && data.ObjType == objType && data.Prop == (int16)prop) {
		id = data.ObjID;
		ok = 1;
    }
    else {
    	id = 0;
		ok = 2;
    }
	ASSIGN_PTR(pID, id);
    return ok;
}

int SLAPI TextRefCore::Search(const TextRefIdent & rI, SStringU & rBuf)
{
	int    ok = 1;
	TextRefTbl::Key1 k1;
	MEMSZERO(k1);
	k1.ObjType = (int16)rI.O.Obj;
	k1.Prop = rI.P;
	k1.ObjID = rI.O.Id;
	k1.Lang = rI.L;
	if(search(1, &k1, spEq)) {
        rBuf = data.Text;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI TextRefCore::SearchSelfRef(long id, SStringU & rBuf)
{
    TextRefIdent ident(PPOBJ_SELFREFTEXT, id, PPTRPROP_DEFAULT);
    return Search(ident, rBuf);
}

int SLAPI TextRefCore::SearchText(const TextRefIdent & rI, const wchar_t * pText, TextRefIdent * pResult)
{
	int    ok = -1;
	const size_t len = sstrlen(pText);
	if(len) {
		TextRefTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = (int16)rI.O.Obj;
		k0.Prop = rI.P;
		STRNSCPY(k0.Text, pText);
		if(search(0, &k0, spGe) && data.ObjType == rI.O.Obj && data.Prop == rI.P && wcsicmp(pText, data.Text) == 0) do {
			if(rI.L < 0 || data.Lang == rI.L) {
				if(pResult) {
					pResult->O.Set(data.ObjType, data.ObjID);
					pResult->P = data.Prop;
					pResult->L = data.Lang;
				}
				ok = 1;
			}
		} while(ok < 0 && search(0, &k0, spNext) && data.ObjType == rI.O.Obj && data.Prop == rI.P && wcsicmp(pText, data.Text) == 0);
		if(ok < 0)
			ok = PPDbSearchError();
	}
	return ok;
}

int SLAPI TextRefCore::SearchTextByPrefix(const TextRefIdent & rI, const wchar_t * pPrefix, TSArray <TextRefIdent> * pList)
{
	int    ok = -1;
	const size_t len = sstrlen(pPrefix);
	if(len) {
		TextRefTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = (int16)rI.O.Obj;
		k0.Prop = rI.P;
		STRNSCPY(k0.Text, pPrefix);
		if(search(0, &k0, spGe) && data.ObjType == rI.O.Obj && data.Prop == rI.P && wcsnicmp(pPrefix, data.Text, len) == 0) do {
			if(rI.L < 0 || data.Lang == rI.L) {
				ok = 1;
				if(pList) {
					TextRefIdent item;
					item.O.Set(data.ObjType, data.ObjID);
					item.P = data.Prop;
					item.L = data.Lang;
					pList->insert(&item);
				}
				else
					break;
			}
		} while(search(0, &k0, spNext) && data.ObjType == rI.O.Obj && data.Prop == rI.P && wcsnicmp(pPrefix, data.Text, len) == 0);
		if(ok < 0)
			ok = PPDbSearchError();
	}
	return ok;
}

int SLAPI TextRefCore::SearchSelfRefTextByPrefix(const wchar_t * pPrefix, TSArray <TextRefIdent> * pList)
{
	TextRefIdent ident(PPOBJ_SELFREFTEXT, 0, PPTRPROP_DEFAULT);
	return SearchTextByPrefix(ident, pPrefix, pList);
}

int SLAPI TextRefCore::SearchSelfRefText(const wchar_t * pText, PPID * pID)
{
	TextRefIdent ident(PPOBJ_SELFREFTEXT, 0, PPTRPROP_DEFAULT);
	TextRefIdent result;
	int    ok = SearchText(ident, pText, &result);
	if(ok > 0) {
		ASSIGN_PTR(pID, result.O.Id);
	}
	return ok;
}

int SLAPI TextRefCore::SetText(const TextRefIdent & rI, const wchar_t * pText, int use_ta)
{
    int    ok = 1;
    SStringU _t;
    THROW_INVARG(rI.L >= 0);
    THROW_INVARG(rI.P >= 0);
    THROW_INVARG(rI.O.Obj > 0);
    THROW_INVARG(rI.O.Id > 0);
    {
    	PPTransaction tra(use_ta);
    	THROW(tra);
		if(Search(rI, _t) > 0) {
			if(isempty(pText)) {
				THROW_DB(rereadForUpdate(1, 0));
				THROW_DB(deleteRec()); // @sfu
			}
			else if(_t.IsEqual(pText)) {
				ok = -1;
			}
			else {
				THROW_DB(rereadForUpdate(1, 0));
				STRNSCPY(data.Text, pText);
				THROW_DB(updateRec()); // @sfu
			}
		}
		else if(isempty(pText)) {
			ok = -2;
		}
		else {
			TextRefTbl::Rec rec;
			MEMSZERO(rec);
			rec.ObjType = (int16)rI.O.Obj;
			rec.ObjID = rI.O.Id;
			rec.Prop = rI.P;
			rec.Lang = rI.L;
			STRNSCPY(rec.Text, pText);
			THROW_DB(insertRecBuf(&rec));
		}
		THROW(tra.Commit());
    }
    CATCHZOK
    return ok;
}

int SLAPI TextRefCore::GetSelfRefText(const wchar_t * pText, PPID * pID, int use_ta)
{
	int    ok = -1;
	PPID   id = 0;
	if(SearchSelfRefText(pText, &id) > 0) {
		ok = 1;
	}
	else {
		PPID   last_id = 0;
		THROW(GetLastObjId(PPOBJ_SELFREFTEXT, PPTRPROP_DEFAULT, &last_id));
		{
			id = last_id + 1;
			TextRefIdent ident(PPOBJ_SELFREFTEXT, id, PPTRPROP_DEFAULT);
			THROW(SetText(ident, pText, use_ta));
			ok = 2;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}
//
//
//
SLAPI UnxTextRefCore::UnxTextRefCore() : UnxTextRefTbl()
{
}

int FASTCALL UnxTextRefCore::PostprocessRead(SStringU & rBuf)
{
	int    ok = 1;
	SBuffer temp_buf;
	readLobData(VT, temp_buf);
	const size_t actual_size = temp_buf.GetAvailableSize();
	rBuf.CopyFromUtf8((const char *)temp_buf.GetBuf(0), actual_size);
	return ok;
}

int SLAPI UnxTextRefCore::Search(const TextRefIdent & rI, SStringU & rBuf)
{
	//
	// Так как текст в этой таблице по определению не индексируемый, то
	// с версии 9.0.0 в записях хранится не "сырой" unicode, а utf8 (ради экономии пространства и производительности).
	// При этом интерфейсные функции по-прежнему оперируют форматом unicode (для совместимости с TextRefCore).
	//
	rBuf = 0;

	int    ok = 1;
	UnxTextRefTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = (int16)rI.O.Obj;
	k0.Prop = rI.P;
	k0.ObjID = rI.O.Id;
	k0.Lang = rI.L;
	if(search(0, &k0, spEq))
		PostprocessRead(rBuf);
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI UnxTextRefCore::GetText(const TextRefIdent & rI, SString & rBuf)
{
	rBuf = 0;
	SStringU temp_buf;
	int    ok = Search(rI, temp_buf);
	if(ok > 0) {
        temp_buf.CopyToUtf8(rBuf, 1);
	}
	return ok;
}

int SLAPI UnxTextRefCore::SetText(const TextRefIdent & rI, const char * pText, int use_ta)
{
	int    ok = -1;
	const  size_t src_len = sstrlen(pText);
	if(src_len) {
		SStringU temp_buf;
		temp_buf.CopyFromUtf8(pText, src_len);
		ok = SetText(rI, temp_buf, use_ta);
	}
	else
		ok = SetText(rI, (const wchar_t *)0, use_ta);
	return ok;
}

int SLAPI UnxTextRefCore::SetText(const TextRefIdent & rI, const wchar_t * pText, int use_ta)
{
	//
	// Так как текст в этой таблице по определению не индексируемый, то
	// с версии 9.0.0 в записях хранится не "сырой" unicode, а utf8 (ради экономии пространства и производительности).
	// При этом интерфейсные функции по-прежнему оперируют форматом unicode (для совместимости с TextRefCore).
	//

	// @v9.0.0 const  size_t tl = sstrlen(pText) * sizeof(wchar_t);
    int    ok = 1;
	SString utf_buf;
    SStringU _t;
    THROW_INVARG(rI.L >= 0);
    THROW_INVARG(rI.P >= 0);
    THROW_INVARG(rI.O.Obj > 0);
    THROW_INVARG(rI.O.Id > 0);
    {
    	_t = pText;
        _t.CopyToUtf8(utf_buf, 1);
    }
	const  size_t tl = utf_buf.Len(); // @v9.0.0
    {
    	PPTransaction tra(use_ta);
    	THROW(tra);
		if(Search(rI, _t) > 0) {
			if(tl == 0) {
				THROW_DB(rereadForUpdate(0, 0)); // @v9.0.5 @fix index 1-->0
				THROW_DB(deleteRec()); // @sfu
			}
			else if(_t.IsEqual(pText)) {
				ok = -1;
			}
			else {
				THROW_DB(rereadForUpdate(0, 0)); // @v9.0.5 @fix index 1-->0
				{
					assert(tl); // Ранее мы проверили длину текста на 0
					THROW(writeLobData(VT, (const char *)utf_buf, tl));
					data.Size = (long)tl;
				}
				THROW_DB(updateRec()); // @sfu
			}
		}
		else if(tl == 0) {
			ok = -2;
		}
		else {
			MEMSZERO(data);
			data.ObjType = (int16)rI.O.Obj;
			data.ObjID = rI.O.Id;
			data.Prop = rI.P;
			data.Lang = rI.L;
			{
				assert(tl); // Ранее мы проверили длину текста на 0
				THROW(writeLobData(VT, (const char *)utf_buf, tl));
				data.Size = (long)tl;
			}
			THROW_DB(insertRec());
		}
		THROW(tra.Commit());
    }
    CATCHZOK
    return ok;
}

UnxTextRefCore::_Enum::_Enum(UnxTextRefCore * pT, long h)
{
	P_T = pT;
	H = h;
}

UnxTextRefCore::_Enum::~_Enum()
{
	CALLPTRMEMB(P_T, EnumList.DestroyIterHandler(H));
}

int UnxTextRefCore::_Enum::Next(void * pRec)
{
	int    ok = 0;
	if(P_T && P_T->EnumList.NextIter(H) > 0) {
		TextRefEnumItem * p_result = (TextRefEnumItem *)pRec;
		p_result->O.Set(P_T->data.ObjType, P_T->data.ObjID);
		p_result->P = P_T->data.Prop;
		p_result->L = P_T->data.Lang;
		if(P_T->GetText(*p_result, p_result->S) > 0)
			ok = 1;
	}
	return ok;
}

int SLAPI UnxTextRefCore::InitEnum(PPID objType, int prop, long * pHandle)
{
	BExtQuery * q = new BExtQuery(this, 0);
	q->select(this->ObjType, this->ObjID, this->Prop, this->Lang, this->Size, 0);
	if(prop)
		q->where(this->ObjType == objType && this->Prop == (long)prop);
	else
		q->where(this->ObjType == objType);
	UnxTextRefTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = (int16)objType;
	q->initIteration(0, &k0, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

SEnumImp * SLAPI UnxTextRefCore::Enum(PPID objType, int prop)
{
	long   h = -1;
	return InitEnum(objType, prop, &h) ? new _Enum(this, h) : 0;
}
//
//
//
#if SLTEST_RUNNING // {

static int DummyProc(ReferenceTbl::Rec * pRec)
{
	static double X = 0.0;
	X += pRec->ObjName[0] ? 1.0 : 2.0;
	return 1;
}

SLTEST_R(Reference_EnumItems)
{
	int    ok = 1;
	long   hdl_enum = -1;
	SArray item_list1(sizeof(ReferenceTbl::Rec)); // Элементы, полученные методом EnumItems
	SArray item_list2(sizeof(ReferenceTbl::Rec)); // Элементы, полученные методом NextEnum
	// ReferenceTbl
	SRng * p_rng = 0;
	PPIDArray obj_type_list;
	Reference * p_tbl = PPRef;
	ReferenceTbl::Key0 k0;
	MEMSZERO(k0);
	if(p_tbl->search(0, &k0, spFirst))
		do {
			obj_type_list.addUnique(p_tbl->data.ObjType);
		} while(p_tbl->search(0, &k0, spNext));
	//
	// Перестраиваем массив типов объектов в случайном порядке
	//
	{
		PPIDArray temp_list;
		p_rng = SRng::CreateInstance(SRng::algMT, 0);
		uint c = obj_type_list.getCount();
		for(uint i = 0; i < c; i++)
			for(PPID id = 0; id == 0;) {
				uint   r = (uint)p_rng->GetUniformInt(c);
				if(r < c) {
					id = obj_type_list.get(r);
					if(id) {
						temp_list.add(id);
						obj_type_list.at(r) = 0;
					}
				}
			}
		THROW(SLTEST_CHECK_EQ((long)temp_list.getCount(), (long)obj_type_list.getCount()));
		obj_type_list = temp_list;
	}
	if(!pBenchmark || sstreqi_ascii(pBenchmark, "EnumItems")) {
		//
		// Перебираем все записи всех объектов методом Reference::EnumItems
		//
		for(uint i = 0; i < obj_type_list.getCount(); i++) {
			ReferenceTbl::Rec rec;
			for(PPID id = 0; p_tbl->EnumItems(obj_type_list.get(i), &id, &rec) > 0;) {
				THROW(SLTEST_CHECK_NZ(item_list1.insert(&rec)));
			}
		}
	}
	if(!pBenchmark || sstreqi_ascii(pBenchmark, "NextEnum")) {
		//
		// Перебираем все записи всех объектов методами Reference::InitEnum, Reference::NextEnum
		//
		for(uint i = 0; i < obj_type_list.getCount(); i++) {
			ReferenceTbl::Rec rec;
			THROW(SLTEST_CHECK_NZ(p_tbl->InitEnum(obj_type_list.get(i), 0, &hdl_enum)));
			for(PPID id = 0; p_tbl->NextEnum(hdl_enum, &rec) > 0;) {
				THROW(SLTEST_CHECK_NZ(item_list2.insert(&rec)));
			}
			p_tbl->DestroyIter(hdl_enum);
			hdl_enum = -1;
		}
	}
	if(!pBenchmark) {
		THROW(SLTEST_CHECK_NZ(item_list1.IsEqual(item_list2)));
	}
	CATCHZOK
	delete p_rng;
	if(hdl_enum >= 0)
		p_tbl->DestroyIter(hdl_enum);
	return ok;
}

#endif // } SLTEST_RUNNING

