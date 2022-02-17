// REFERENC.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

/*static*/int Reference::Helper_EncodeOtherPw(const char * pEncPw, const char * pPw, size_t pwBufSize, SString & rResult)
{
	rResult.Z();
	int    ok = 1;
	const  size_t buf_quant = 256;
	assert(buf_quant >= pwBufSize);
	char   temp_pw[buf_quant], temp_str[buf_quant*3+8];
	STRNSCPY(temp_pw, pPw);
	IdeaEncrypt(pEncPw, temp_pw, pwBufSize);
	size_t p = 0;
	for(size_t i = 0; i < pwBufSize; i++) {
		sprintf(temp_str+p, "%03u", static_cast<uint8>(temp_pw[i]));
		p += 3;
	}
	temp_str[p] = 0;
	rResult = temp_str;
	return ok;
}

/*static*/int Reference::Helper_DecodeOtherPw(const char * pEncPw, const char * pPw, size_t pwBufSize, SString & rResult)
{
	rResult.Z();
	int    ok = 1;
	const  size_t buf_quant = 256;
	assert(buf_quant >= pwBufSize);
	char   temp_pw[buf_quant], temp_str[buf_quant*3+8];
	STRNSCPY(temp_str, pPw);
	const size_t sl = sstrlen(temp_str);
	if(sl != (pwBufSize*3) && (pwBufSize == 64 && sl == (20*3))) { // @v9.8.12 Специальный случай для обратной совместимости
		pwBufSize = 20;
	}
	if((sl % 3) == 0) {
		for(size_t i = 0, p = 0; i < pwBufSize; i++) {
			char   nmb[16];
			nmb[0] = temp_str[p];
			nmb[1] = temp_str[p+1];
			nmb[2] = temp_str[p+2];
			nmb[3] = 0;
			temp_pw[i] = satoi(nmb);
			p += 3;
		}
		IdeaDecrypt(pEncPw, temp_pw, pwBufSize);
	}
	else {
		temp_pw[0] = 0;
		ok = 0;
	}
	rResult = temp_pw;
	// @v11.1.1 IdeaRandMem(temp_pw, sizeof(temp_pw));
	SObfuscateBuffer(temp_pw, sizeof(temp_pw)); // @v11.1.1 
	return ok;
}

/*static*/int Reference::Helper_Encrypt_(int cryptMethod, const char * pEncPw, const char * pText, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	if(cryptMethod == crymRef2) {
		SString temp_buf;
		char   pw_buf[128], pw_buf2[128];
		const  size_t bin_pw_size = (bufLen >= 34) ? 24 : (bufLen * 3 / 4 - 3);
		size_t pw_len = sstrlen(pText);
		// @v11.1.1 IdeaRandMem(pw_buf2, sizeof(pw_buf2));
		SObfuscateBuffer(pw_buf2, sizeof(pw_buf2)); // @v11.1.1 
		if(pText)
			memcpy(pw_buf2, pText, pw_len+1);
		else
			PTR32(pw_buf2)[0] = 0;
		// @v11.1.1 IdeaRandMem(pw_buf, sizeof(pw_buf));
		SObfuscateBuffer(pw_buf, sizeof(pw_buf)); // @v11.1.1 
		IdeaEncrypt(/*0*/pEncPw, pw_buf2, bin_pw_size);
		temp_buf.EncodeMime64(pw_buf2, bin_pw_size).CopyTo(pBuf, bufLen);
		if(temp_buf.Len() > (bufLen-1))
			ok = 0;
	}
	else if(cryptMethod == crymDefault) {
		// @v11.1.1 IdeaRandMem(pBuf, bufLen);
		SObfuscateBuffer(pBuf, bufLen); // @v11.1.1 
		strnzcpy(pBuf, pText, bufLen);
		IdeaEncrypt(/*0*/pEncPw, pBuf, bufLen);
	}
	else
		ok = 0;
	return ok;
}

/*static*/int Reference::Helper_Decrypt_(int cryptMethod, const char * pEncPw, const char * pBuf, size_t bufLen, SString & rText)
{
	int    ok = 1;
	char   pw_buf[128];
	rText.Z();
	if(cryptMethod == crymRef2) {
		size_t bin_pw_size = 0;
		SString temp_buf(pBuf);
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

/*static*/int Reference::Encrypt(int cryptMethod, const char * pText, char * pBuf, size_t bufLen)
	{ return Reference::Helper_Encrypt_(cryptMethod, 0, pText, pBuf, bufLen); }
/*static*/int Reference::Decrypt(int cryptMethod, const char * pBuf, size_t bufLen, SString & rText)
	{ return Reference::Helper_Decrypt_(cryptMethod, 0, pBuf, bufLen, rText); }

/*static*/int Reference::GetPassword(const PPSecur * pSecur, char * pBuf, size_t bufLen)
{
	int    ok = 1;
	if(pSecur) {
		SString temp_buf;
		Reference::Decrypt(Reference::crymRef2, pSecur->Password, sizeof(pSecur->Password), temp_buf);
		temp_buf.CopyTo(pBuf, bufLen);
		temp_buf.Z().CatCharN(' ', 96);
	}
	else
		ok = -1;
	return ok;
}

/*static*/int Reference::VerifySecur(PPSecur2 * pSecur, int set)
{
	int    ok = 1;
	SCRC32 c;
	size_t offs = offsetof(PPSecur2, Crc);
	uint32 crc = 0;
	{
		offs = offsetof(PPSecur2, Crc);
		crc = c.Calc(0, PTR8C(pSecur)+8, offs-8);
		offs += sizeof(uint32);
		crc = c.Calc(crc, PTR8(pSecur) + offs, sizeof(PPSecur2) - offs);
	}
	if(set || pSecur->Crc != crc) {
		size_t len = sstrlen(pSecur->Name);
		if(len < sizeof(pSecur->Name))
			memzero(PTR8(pSecur->Name)+len, sizeof(pSecur->Name)-len);
		len = sstrlen(pSecur->Symb);
		if(len < sizeof(pSecur->Symb))
			memzero(PTR8(pSecur->Symb)+len, sizeof(pSecur->Symb)-len);
		{
			offs = offsetof(PPSecur2, Crc);
			crc = c.Calc(0, PTR8C(pSecur)+8, offs-8);
			offs += sizeof(uint32);
			crc = c.Calc(crc, PTR8C(pSecur) + offs, sizeof(PPSecur2) - offs);
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
		uint32 crc2 = c.Calc(0, PTR8C(pSecur), offs);
		offs += sizeof(uint32);
		crc2 = c.Calc(crc2, PTR8C(pSecur) + offs, sizeof(PPSecur2) - offs);
		if(pSecur->Crc != crc2) {
			PPSetError(PPERR_INVPPSECURCRC);
			ok = 0;
		}
	}
	return ok;
}

/*static*/int Reference::GetExField(const PPConfigPrivate * pRec, int fldId, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(fldId == PCFGEXSTR_DESKTOPNAME) {
		SString temp_buf(pRec->Tail);
		ok = PPGetExtStrData(fldId, temp_buf, rBuf);
	}
	return ok;
}

/*static*/int Reference::SetExField(PPConfigPrivate * pRec, int fldId, const char * pBuf)
{
	int    ok = -1;
	if(fldId == PCFGEXSTR_DESKTOPNAME) {
		SString temp_buf(pRec->Tail);
		ok = PPPutExtStrData(fldId, temp_buf, pBuf);
		temp_buf.CopyTo(pRec->Tail, sizeof(pRec->Tail));
	}
	return ok;
}

Reference::Reference() : ReferenceTbl(), P_OvT(new ObjVersioningCore)
{
}

Reference::~Reference()
{
	delete P_OvT;
}

int Reference::AllocDynamicObj(PPID * pDynObjType, const char * pName, long flags, int use_ta)
{
	assert(pDynObjType != 0); // @v10.6.8
	int    ok = 1, r;
	PPID   id = *pDynObjType;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if((r = _GetFreeID(PPOBJ_DYNAMICOBJS, &id, PPOBJ_FIRSTDYN)) > 0) {
			ReferenceTbl::Rec rec;
			// @v10.6.4 MEMSZERO(rec);
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

int Reference::FreeDynamicObj(PPID dynObjType, int use_ta)
{
	int    ok = 1, r;
	PPID   id = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(GetItem(PPOBJ_DYNAMICOBJS, dynObjType) > 0);
		THROW_DB(deleteRec());
		THROW(RemoveProperty(PPOBJ_DYNAMICOBJS, dynObjType, 0, 0));
		for(id = 0; (r = EnumItems(dynObjType, &id)) > 0;) {
			THROW_DB(deleteRec());
			THROW(RemoveProperty(dynObjType, id, 0, 0));
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int Reference::_GetFreeID(PPID objType, PPID * pID, PPID firstID)
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

int Reference::GetFreeID(PPID obj, PPID * id)
{
	return _GetFreeID(obj, id, PP_FIRSTUSRREF);
}

int Reference::AddItem(PPID obj, PPID * pID, const void * b, int use_ta)
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

int Reference::UpdateItem(PPID obj, PPID id, const void * b, int logAction /*=1*/, int use_ta)
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
			if(search(0, &k, spEq)) {
				copyBufTo(&prev_rec);
				new_rec = *static_cast<const ReferenceTbl::Rec *>(b);
				new_rec.ObjType = obj;
				new_rec.ObjID   = id;
				if(!DBTable::GetFields().IsEqualRecords(&prev_rec, &new_rec)) {
					DBRowId _dbpos;
					THROW_DB(getPosition(&_dbpos));
					THROW_DB(getDirectForUpdate(0, &k, _dbpos));
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

int Reference::_Search(PPID obj, PPID id, int spMode, void * b)
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

int Reference::GetItem(PPID obj, PPID id, void * b)
{
	return _Search(obj, id, spEq, b);
}

int Reference::EnumItems(PPID obj, PPID * pID, void * b)
{
	int    r = _Search(obj, *pID, spGt, b);
	return (r > 0) ? ((data.ObjType == obj) ? (*pID = data.ObjID, 1) : -1) : r;
}

int Reference::InitEnum(PPID objType, int flags, long * pHandle)
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
	q->initIteration(false, &k0, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

int Reference::InitEnumByIdxVal(PPID objType, int valN, long val, long * pHandle)
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
		q->initIteration(false, &k, spGe);
		ok = EnumList.RegisterIterHandler(q, pHandle);
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int Reference::NextEnum(long enumHandle, void * pRec)
{
	return (EnumList.NextIter(enumHandle) > 0) ? (copyBufTo(pRec), 1) : -1;
}

int Reference::DestroyIter(long enumHandle)
{
	return EnumList.DestroyIterHandler(enumHandle);
}

SEnum::Imp * Reference::Enum(PPID objType, int options)
{
	long   h = -1;
	return InitEnum(objType, options, &h) ? new PPTblEnum <Reference>(this, h) : 0;
}

SEnum::Imp * Reference::EnumByIdxVal(PPID objType, int valN, long val)
{
	long   h = -1;
	return InitEnumByIdxVal(objType, valN, val, &h) ? new PPTblEnum<Reference>(this, h) : 0;
}

int Reference::LoadItems(PPID objType, SVector & rList) // @v10.6.8 SArray-->SVector
{
	int    ok = -1;
	ReferenceTbl::Rec rec;
	for(SEnum en = Enum(objType, 0); ok && en.Next(&rec);)
		ok = rList.insert(&rec) ? 1 : PPSetError(PPERR_SLIB);
	return ok;
}

int Reference::SearchName(PPID obj, PPID * pID, const char * pName, void * pRec)
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

int Reference::SearchSymb(PPID objType, PPID * pID, const char * pSymb, size_t offs)
{
	int    ok = -1;
	ASSIGN_PTR(pID, 0);
	if(!isempty(pSymb)) {
		long   h = -1;
		ReferenceTbl::Rec rec;
		for(InitEnum(objType, 0, &h); ok < 0 && NextEnum(h, &rec) > 0;) {
			if(stricmp866(pSymb, reinterpret_cast<const char *>(&rec) + offs) == 0) {
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

int Reference::CheckUniqueSymb(PPID objType, PPID id, const char * pSymb, size_t offs)
{
	int    ok = 1;
	if(!isempty(pSymb)) {
		long   h = -1;
		ReferenceTbl::Rec rec;
		for(InitEnum(objType, 0, &h); ok && NextEnum(h, &rec) > 0;)
			if(stricmp866(pSymb, reinterpret_cast<const char *>(&rec) + offs) == 0 && rec.ObjID != id)
				ok = (PPSetObjError(PPERR_DUPSYMB, objType, rec.ObjID), 0);
		DestroyIter(h);
	}
	return ok;
}

int Reference::RemoveItem(PPID obj, PPID id, int use_ta)
{
	int    ok = 1, r;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		r = SendObjMessage(DBMSG_OBJDELETE, 0, obj, id);
		THROW(r != DBRPL_ERROR);
		THROW_PP(r != DBRPL_CANCEL, PPERR_USERBREAK);
		THROW_DB(deleteFrom(this, 0, (this->ObjType == obj && this->ObjID == id)));
		THROW(RemoveProperty(obj, id, 0, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int Reference::RemoveProperty(PPID obj, PPID id, PPID prop, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(prop == 0) {
			THROW_DB(deleteFrom(&Prop, 0, (Prop.ObjType == obj && Prop.ObjID == id)));
		}
		else {
			THROW_DB(deleteFrom(&Prop, 0, (Prop.ObjType == obj && Prop.ObjID == id && Prop.Prop == prop)));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int Reference::ReadPropBuf(void * b, size_t s, size_t * pActualSize)
{
	int    ok = 1;
	size_t actual_size = 0;
	SBuffer temp_buf;
	const  RECORDSIZE fix_rec_size = Prop.getRecSize();
	temp_buf.Write(&Prop.data, fix_rec_size);
	Prop.readLobData(Prop.VT, temp_buf);
	Prop.destroyLobData(Prop.VT); // @v10.2.11 @fix
	actual_size = temp_buf.GetAvailableSize();
	temp_buf.Read(b, s);
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

int Reference::PreparePropBuf(PPID obj, PPID id, PPID prop, const void * b, uint s)
{
	int    ok = 1;
	RECORDSIZE fix_rec_size = Prop.getRecSize();
	if(b) {
		Prop.copyBufFrom(b, fix_rec_size);
		if(s)
			THROW(Prop.writeLobData(Prop.VT, PTR8C(b)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0));
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

int Reference::PutProp(PPID obj, PPID id, PPID prop, const void * b, size_t s, int use_ta)
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
			k.ObjType = obj;
			k.ObjID   = id;
			k.Prop    = prop;
			if(Prop.search(0, &k, spEq)) {
				do {
					THROW_DB(Prop.rereadForUpdate(0, &k));
					THROW_DB(Prop.deleteRec());
				} while(Prop.search(0, &k, spNext) && Prop.data.ObjType == obj && Prop.data.ObjID == id && Prop.data.Prop == prop);
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int Reference::_SearchProp(PPID obj, PPID id, PPID prop, int spMode, void * b, size_t s)
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

int Reference::GetProperty(PPID obj, PPID id, PPID prop, void * b, size_t s)
	{ return _SearchProp(obj, id, prop, spEq, b, s); }
int Reference::GetPropMainConfig(PPID prop, void * b, size_t s)
	{ return _SearchProp(PPOBJ_CONFIG, PPCFG_MAIN, prop, spEq, b, s); }

int Reference::GetPropActualSize(PPID obj, PPID id, PPID prop, size_t * pActualSize)
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

int Reference::EnumProperties(PPID obj, PPID id, PPID * prop, void * b, uint s)
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

int Reference::GetPropVlrString(PPID obj, PPID id, PPID prop, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	uint8  pm_fixed_buf[1024]; // @v10.0.01 (обход динамического распределения памяти)
	PropVlrString * pm = 0;
	int    is_pm_allocated = 0;
	PropertyTbl::Key0 k;
	k.ObjType = obj;
	k.ObjID   = id;
	k.Prop    = prop;
	if(Prop.search(0, &k, spEq)) {
		size_t actual_size = 0, test_actual_size = 0;
		RECORDSIZE fix_size = Prop.getRecSize();
		Prop.getLobSize(Prop.VT, &actual_size);
		actual_size += fix_size;
		// +32 - страховка
		if((actual_size+32) <= sizeof(pm_fixed_buf)) {
			pm = reinterpret_cast<PropVlrString *>(pm_fixed_buf);
		}
		else {
			THROW_MEM(pm = static_cast<PropVlrString *>(SAlloc::M(actual_size+32)));
			is_pm_allocated = 1;
		}
		ReadPropBuf(pm, actual_size, &test_actual_size);
		assert(actual_size == test_actual_size);
		(rBuf = reinterpret_cast<const char *>(pm + 1)).Strip();
	}
	else
		ok = PPDbSearchError();
	CATCHZOK
	if(is_pm_allocated) {
		SAlloc::F(pm);
	}
	return ok;
}

int Reference::PutPropVlrString(PPID obj, PPID id, PPID prop, const char * b, int use_ta)
{
	int    ok = 1;
	PropVlrString * pm = 0;
	uint   s = 0;
	if(!isempty(b)) {
		const uint sz = sstrlen(b) + 1;
		s = MAX(sizeof(PropVlrString) + sz, PROPRECFIXSIZE);
		THROW_MEM(pm = static_cast<PropVlrString *>(SAlloc::M(s)));
		memzero(pm, s);
		strcpy(reinterpret_cast<char *>(pm + 1), b);
		pm->Size = sz;
	}
	THROW(PutProp(obj, id, prop, pm, s, use_ta));
	CATCHZOK
	SAlloc::F(pm);
	return ok;
}

int Reference::PutPropSBuffer(PPID obj, PPID id, PPID prop, const SBuffer & rBuf, int use_ta)
{
	int    ok = 1;
	PropVlrString * pm = 0;
	uint   s = 0;
	uint   sz = rBuf.GetAvailableSize();
	if(sz) {
		s = MAX(sizeof(PropVlrString) + sz, PROPRECFIXSIZE);
		THROW_MEM(pm = static_cast<PropVlrString *>(SAlloc::M(s)));
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
	PropVlrString * pm = 0;
	RECORDSIZE fix_size = Prop.getRecSize();
	Prop.getLobSize(Prop.VT, &actual_size);
	actual_size += fix_size;
	THROW_MEM(pm = static_cast<PropVlrString *>(SAlloc::M(actual_size + 32))); // +32 - страховка
	ReadPropBuf(pm, actual_size, &test_actual_size);
	assert(actual_size == test_actual_size);
	if(actual_size == test_actual_size && actual_size == MAX((pm->Size+sizeof(*pm)), PROPRECFIXSIZE)) {
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

int Reference::GetPropSBuffer(PPID obj, PPID id, PPID prop, SBuffer & rBuf)
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

int Reference::GetPropArrayFromRecBuf(SVectorBase * pAry)
{
	int    ok = 1;
	PropPPIDArray * p_rec = 0;
	if(pAry) {
		const uint item_size = pAry->getItemSize();
		size_t actual_size = 0, test_actual_size = 0;
		RECORDSIZE fix_size = Prop.getRecSize();
		Prop.getLobSize(Prop.VT, &actual_size);
		actual_size += fix_size;
		THROW_MEM(p_rec = static_cast<PropPPIDArray *>(SAlloc::C(1, actual_size + 32))); // +32 - страховка
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
	CATCHZOK
	SAlloc::F(p_rec);
	return ok;
}

int Reference::GetPropArray(PPID obj, PPID id, PPID prop, SVectorBase * pAry)
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
	CATCHZOK
	return ok;
}

int Reference::PutPropArray(PPID obj, PPID id, PPID prop, const SVectorBase * ary, int use_ta)
{
	int    ok = 1;
	PropPPIDArray * p_rec = 0;
	uint   i;
	const  uint count = SVectorBase::GetCount(ary);
	size_t sz = 0;
	if(count > 0) {
		const  uint minCount = (PROPRECFIXSIZE - sizeof(PropPPIDArray)) / ary->getItemSize();
		sz = sizeof(PropPPIDArray) + (MAX(count, minCount) * ary->getItemSize());
		SETMAX(sz, PROPRECFIXSIZE);
		THROW_MEM(p_rec = static_cast<PropPPIDArray *>(SAlloc::C(1, sz)));
		p_rec->Count = count;
		for(i = 0; i < count; i++) {
			size_t offs = i*ary->getItemSize();
			memcpy(PTR8(p_rec+1) + offs, ary->at(i), ary->getItemSize());
		}
	}
	THROW(PutProp(obj, id, prop, p_rec, sz, use_ta));
	CATCHZOK
	SAlloc::F(p_rec);
	return ok;
}

int Reference::GetConfig(PPID obj, PPID id, PPID cfgID, void * b, uint s)
{
	Reference2Tbl::Rec rec;
	int    r = GetProperty(obj, id, cfgID, b, s);
	if(r < 0 && oneof2(obj, PPOBJ_USRGRP, PPOBJ_USR)) {
		if(GetItem(obj, id, &rec) > 0) {
			const PPID prev_level_id = (obj == PPOBJ_USRGRP) ? PPOBJ_CONFIG : PPOBJ_USRGRP;
			r = GetConfig(prev_level_id, reinterpret_cast<const PPSecur *>(&rec)->ParentID, cfgID, b, s); // @recursion
		}
		else {
			PPSetAddedMsgObjName(obj, id);
			r = PPSetError(PPERR_CFGOBJRDFAULT);
		}
	}
	return r;
}

int Reference::SetConfig(PPID obj, PPID id, PPID cfgID, void * b, uint s)
{
	return PutProp(obj, id, cfgID, b, s);
}

int Reference::LoadSecur(PPID obj, PPID id, PPSecurPacket * pPack)
{
	int    ok = 1;
	THROW(GetItem(obj, id, &pPack->Secur) > 0);
	THROW(Reference::VerifySecur(&pPack->Secur, 0));
	THROW(DS.FetchConfig(obj, id, &pPack->Config));
	THROW(pPack->Rights.Get(obj, id, 0/*ignoreCheckSum*/));
	THROW(pPack->Paths.Get(obj, id));
	// @v11.0.0 {
	pPack->PrivateDesktopUUID.Z();
	if(obj == PPOBJ_USR) {
		PPConfigPrivate cfgp_rec;
		if(GetConfig(PPOBJ_USR, id, PPPRP_CFGPRIVATE, &cfgp_rec, sizeof(cfgp_rec)) > 0)
			pPack->PrivateDesktopUUID = cfgp_rec.DesktopUuid;
	}
	// } @v11.0.0 
	CATCHZOK
	return ok;
}

int Reference::EditSecur(PPID obj, PPID id, PPSecurPacket * pPack, int isNew, int use_ta)
{
	int   ok = 1;
	const int is_user = (obj == PPOBJ_USR);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(is_user) {
			if(isNew)
				pPack->Secur.PwUpdate = getcurdate_();
			else {
				THROW_DB(GetItem(obj, id) > 0);
				if(memcmp(pPack->Secur.Password, reinterpret_cast<PPSecur *>(&data)->Password, sizeof(pPack->Secur.Password)))
					pPack->Secur.PwUpdate = getcurdate_();
			}
		}
		THROW(Reference::VerifySecur(&pPack->Secur, 1));
		THROW(isNew ? AddItem(obj, &id, &pPack->Secur, 0) : UpdateItem(obj, id, &pPack->Secur, 1, 0));
		THROW(pPack->Paths.Put(obj, id));
		if(is_user && pPack->Secur.Flags & USRF_INHCFG) {
			THROW(RemoveProperty(obj, id, PPPRP_CFG, 0));
		}
		else {
			THROW(SetConfig(obj, id, PPPRP_CFG, &pPack->Config, sizeof(pPack->Config)));
		}
		if(is_user && pPack->Secur.Flags & USRF_INHRIGHTS) {
			THROW(pPack->Rights.Remove(obj, id));
		}
		else {
			THROW(pPack->Rights.Put(obj, id));
		}
		THROW(tra.Commit());
		SETIFZ(pPack->Secur.ID, id); // @v9.8.4
	}
	CATCHZOK
	return ok;
}

int Reference::RemoveSecur(PPID obj, PPID id, int use_ta)
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
	DateRange RBillPeriod; // Период R-доступа к документам
	short  AccessLevel;    // Уровень доступа
	char   Reserve2[6];    // @reserve
	ushort ORTailSize;     // Размер хвоста с правами доступа по объектам
	DateRange WBillPeriod; // Период W-доступа к документам
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

PPRights::PPRights() : P_Rt(0), P_OpList(0), P_LocList(0), P_CfgList(0), P_AccList(0), P_PosList(0), P_QkList(0)
{
}

PPRights::~PPRights()
{
	Empty();
}

int PPRights::SerializeArrayPtr(int dir, ObjRestrictArray ** ppA, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	assert(ppA);
	int    ok = 1;
	uint8  ind = 0;
	if(dir > 0) {
		ind = *ppA ? 0 : 1;
		THROW_SL(pSCtx->Serialize(dir, ind, rBuf));
		if(ind == 0) {
			THROW_SL(pSCtx->Serialize(dir, *ppA, rBuf));
		}
	}
	else if(dir < 0) {
		THROW(pSCtx->Serialize(dir, ind, rBuf));
		if(ind == 0) {
			if(!*ppA) {
				THROW_SL(*ppA = new ObjRestrictArray);
			}
			else
				(*ppA)->clear();
			THROW_SL(pSCtx->Serialize(dir, *ppA, rBuf));
		}
		else {
			if(*ppA) {
				delete *ppA;
				*ppA = 0;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPRights::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->SerializeBlock(dir, Size(), P_Rt, rBuf, 1));
	THROW(SerializeArrayPtr(dir, &P_OpList, rBuf, pSCtx));
	THROW(SerializeArrayPtr(dir, &P_LocList, rBuf, pSCtx));
	THROW(SerializeArrayPtr(dir, &P_CfgList, rBuf, pSCtx));
	THROW(SerializeArrayPtr(dir, &P_AccList, rBuf, pSCtx));
	THROW(SerializeArrayPtr(dir, &P_PosList, rBuf, pSCtx));
	THROW(SerializeArrayPtr(dir, &P_QkList, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

size_t PPRights::Size() const { return P_Rt ? (sizeof(_PPRights) + P_Rt->ORTailSize) : 0; }
bool   PPRights::IsEmpty() const { return (P_Rt == 0); }
int    PPRights::IsInherited() const { return BIN(P_Rt && P_Rt->OprFlags & PPORF_INHERITED); }

void PPRights::Empty()
{
	ZFREE(P_Rt);
	ZDELETE(P_OpList);
	ZDELETE(P_LocList);
	ZDELETE(P_CfgList);
	ZDELETE(P_AccList);
	ZDELETE(P_PosList);
	ZDELETE(P_QkList);
}

int PPRights::Merge(const PPRights & rS, long flags)
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
		SETMIN(P_Rt->RBillPeriod.low, rS.P_Rt->RBillPeriod.low);
		SETMAX(P_Rt->RBillPeriod.upp, rS.P_Rt->RBillPeriod.upp);
		SETMIN(P_Rt->WBillPeriod.low, rS.P_Rt->WBillPeriod.low);
		SETMAX(P_Rt->WBillPeriod.upp, rS.P_Rt->WBillPeriod.upp);
		P_Rt->Flags |= rS.P_Rt->Flags;
		P_Rt->OprFlags |= rS.P_Rt->OprFlags;
		//
		const _PPRights * p_s = rS.P_Rt;
		for(uint s = 0; s < rS.P_Rt->ORTailSize;) {
			const ObjRights * p_so = reinterpret_cast<const ObjRights *>(PTR8C(p_s + 1) + s);
			if(!(p_so->OprFlags & PPORF_INHERITED)) {
				int   _found = 0;
				for(uint t = 0; !_found && t < P_Rt->ORTailSize;) {
					ObjRights * p_o = reinterpret_cast<ObjRights *>(PTR8(P_Rt + 1) + t);
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

int PPRights::ReadRights(PPID securType, PPID securID, int ignoreCheckSum)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    r = 0;
	size_t sz = 2048;
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
				const ObjRights * o = reinterpret_cast<const ObjRights *>(PTR8(P_Rt + 1) + s);
				if(o->Size == sizeof(ObjRights_Pre855))
					do_convert = 1;
				s += o->Size;
			}
			if(do_convert) {
				uint8  temp_buffer[128];
				_PPRights * p_temp_r = static_cast<_PPRights *>(SAlloc::M(Size()));
				THROW_MEM(p_temp_r);
				memcpy(p_temp_r, P_Rt, Size());
				THROW(Resize(sizeof(_PPRights)));
				for(s = 0; s < p_temp_r->ORTailSize;) {
					const ObjRights * o = reinterpret_cast<const ObjRights *>(PTR8(p_temp_r + 1) + s);
					ObjRights * p_temp = reinterpret_cast<ObjRights *>(temp_buffer);
					if(o->Size == sizeof(ObjRights_Pre855)) {
						const ObjRights_Pre855 * p_temp_pre855 = reinterpret_cast<const ObjRights_Pre855 *>(o);
						p_temp->ObjType = p_temp_pre855->ObjType;
						p_temp->Size = sizeof(ObjRights);
						p_temp->Flags = p_temp_pre855->Flags;
						p_temp->OprFlags = static_cast<uint32>(p_temp_pre855->OprFlags);
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

int PPRights::Get(PPID securType, PPID securID, int ignoreCheckSum)
{
	int    ok = 1, r;
	Reference * p_ref = PPRef;
	// @v10.3.0 (never used) size_t sz = 1024;
	ulong  chksum = 0;
	ObjRestrictArray temp_orlist;
	THROW(r = ReadRights(securType, securID, ignoreCheckSum));
	if(r > 0) {
		if(P_Rt->SecurObj < securType)
			P_Rt->OprFlags |= PPORF_INHERITED;
		if(oneof2(P_Rt->SecurObj, PPOBJ_USRGRP, PPOBJ_USR)) {
			//
			// Необходимо унаследовать от более высоких уровней иерархии
			// права по объектам, которые не определены на заданном уровне (Рекурсия)
			//
			Reference2Tbl::Rec this_secur_rec;
			if(p_ref->GetItem(securType, securID, &this_secur_rec) > 0) { // @v10.6.7
				PPRights temp;
				PPID   prev_type = (P_Rt->SecurObj == PPOBJ_USRGRP) ? PPOBJ_CONFIG : PPOBJ_USRGRP;
				// @v10.6.7 PPID   prev_id   = reinterpret_cast<const PPSecur *>(&p_ref->data)->ParentID;
				PPID   prev_id = reinterpret_cast<const PPSecur *>(&this_secur_rec)->ParentID; // @v10.6.7 
				THROW(temp.Get(prev_type, prev_id, ignoreCheckSum)); // @recursion
				for(uint s = 0; s < temp.P_Rt->ORTailSize;) {
					const ObjRights * o = reinterpret_cast<const ObjRights *>(PTR8(temp.P_Rt + 1) + s);
					if(!(o->OprFlags & PPORF_DEFAULT)) {
						THROW(SetObjRights(o->ObjType, o, 0));
					}
					s += o->Size;
				}
			}
		}
		THROW(Resize(Size()));
		{
			class OraReader {
			public:
				OraReader(PPID objType, PPID objID, int ignoreCheckSum) : ObjType(objType), ObjID(objID), IgnoreCheckSum(ignoreCheckSum)
				{
				}
				int Read(PPID propID, ulong * pCheckSum, ObjRestrictArray ** ppList)
				{
					int    ok = 1;
					ObjRestrictArray temp_orlist;
					THROW(PPRef->GetPropArray(ObjType, ObjID, propID, &temp_orlist));
					if(!IgnoreCheckSum && pCheckSum && *pCheckSum) {
						ulong  chksum = _checksum__(static_cast<const char *>(temp_orlist.dataPtr()), temp_orlist.getCount() * temp_orlist.getItemSize());
						THROW_PP(chksum == *pCheckSum, PPERR_INVRTCHKSUM);
					}
					if(temp_orlist.getCount()) {
						ZDELETE(*ppList);
						THROW_MEM(*ppList = new ObjRestrictArray(temp_orlist));
					}
					CATCHZOK
					return ok;
				}
				PPID   ObjType;
				PPID   ObjID;
				int    IgnoreCheckSum;
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
		P_PosList = 0;
		P_QkList = 0;
	}
	CATCHZOK
	return ok;
}

int PPRights::Remove(PPID securType, PPID securID)
{
	return PPRef->RemoveProperty(securType, securID, PPPRP_RTCOMM, 0);
}

int PPRights::Put(PPID securType, PPID securID)
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
			P_Rt->ChkSumOpList = _checksum__((char *)P_OpList->dataPtr(), P_OpList->getCount() * P_OpList->getItemSize());
		else
			P_Rt->ChkSumOpList = _checksum__(0, 0);
		if(P_LocList)
			P_Rt->ChkSumLocList = _checksum__((char *)P_LocList->dataPtr(), P_LocList->getCount() * P_LocList->getItemSize());
		else
			P_Rt->ChkSumLocList = _checksum__(0, 0);
		if(P_CfgList)
			P_Rt->ChkSumCfgList = _checksum__((char *)P_CfgList->dataPtr(), P_CfgList->getCount() * P_CfgList->getItemSize());
		else
			P_Rt->ChkSumCfgList = _checksum__(0, 0);
		if(P_AccList)
			P_Rt->ChkSumAccList = _checksum__((char *)P_AccList->dataPtr(), P_AccList->getCount() * P_AccList->getItemSize());
		else
			P_Rt->ChkSumAccList = _checksum__(0, 0);
		THROW(p_ref->SetConfig(securType, securID, PPPRP_RTCOMM, P_Rt, Size()));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_OPLIST, P_OpList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_LOCLIST, P_LocList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_CFG, P_CfgList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_ACCLIST, P_AccList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_POSLIST, P_PosList, 0));
		THROW(p_ref->PutPropArray(securType, securID, PPPRP_RT_QKLIST,  P_QkList, 0));
	}
	CATCHZOK
	return ok;
}

const ObjRights * PPRights::GetConstObjRights(PPID objType, ObjRights * pDef) const
{
	const ObjRights * p_result = 0;
	if(P_Rt) {
		for(uint s = 0; !p_result && s < P_Rt->ORTailSize;) {
			const ObjRights * o = reinterpret_cast<const ObjRights *>(PTR8(P_Rt + 1) + s);
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

ObjRights * PPRights::GetObjRights(PPID objType, int use_default) const
{
	ObjRights * p_result = 0;
	if(P_Rt) {
		for(uint s = 0; !p_result && s < P_Rt->ORTailSize;) {
			ObjRights * o = reinterpret_cast<ObjRights *>(PTR8(P_Rt + 1) + s);
			const uint osz = o->Size;
			if(o->ObjType == objType) {
				p_result = ObjRights::Create(objType, osz);
				memcpy(p_result, o, osz);
			}
			else
				s += osz;
		}
		if(!p_result && use_default) {
			p_result = ObjRights::Create(objType, 0);
		}
	}
	return p_result;
}

int PPRights::Resize(uint sz)
{
	int    ok = 1;
	if(P_Rt && sz == 0)
		ZFREE(P_Rt);
	else {
		size_t prev_size = Size();
		sz = (sz < sizeof(_PPRights)) ? sizeof(_PPRights) : sz;
		P_Rt = static_cast<_PPRights *>(SAlloc::R(P_Rt, sz));
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

int PPRights::SetObjRights(PPID objType, const ObjRights * rt, int replace)
{
	int    ok = 1;
	if(P_Rt) {
		uint8 * cp = PTR8(P_Rt + 1);
		size_t hs = sizeof(_PPRights);
		size_t ts = P_Rt->ORTailSize;
		size_t s  = 0;
		int    found = 0;
		while(s < ts && !found) {
			ObjRights * o = reinterpret_cast<ObjRights *>(cp + s);
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

int PPRights::SetAccessRestriction(const PPAccessRestriction * accsr)
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

ulong PPRights::CheckSum()
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
		cs = _checksum__(reinterpret_cast<const char *>(P_Rt), Size());
		P_Rt->ChkSumLocList = save_locl_chksum;
		P_Rt->ChkSumOpList  = save_opl_chksum;
		P_Rt->ChkSumCfgList = save_cfgl_chksum;
		P_Rt->ChkSumAccList = save_accl_chksum;
		P_Rt->CheckSum      = save_chksum;
	}
	return cs;
}

PPAccessRestriction & PPRights::GetAccessRestriction(PPAccessRestriction & rAcsr) const
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

int PPRights::CheckBillDate(LDATE dt, int forRead) const
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

int PPRights::AdjustBillPeriod(DateRange & rPeriod, int checkOnly) const
{
	int    ok = 1;
	if(!IsEmpty() && !PPMaster) {
		DateRange r_bill_period;
		DateRange in_period;
		in_period = rPeriod;
		in_period.Actualize(ZERODATE);
		LDATE b = in_period.low;
		LDATE e = in_period.upp;
		PPAccessRestriction accsr;
		GetAccessRestriction(accsr).GetRBillPeriod(&r_bill_period);
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
	return ok;
}

int PPRights::AdjustCSessPeriod(DateRange & rPeriod, int checkOnly) const
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

int PPRights::IsOpRights() const { return BIN(P_OpList && P_OpList->getCount()); }
int PPRights::IsLocRights() const { return BIN(P_LocList && P_LocList->getCount()); }

int PPRights::MaskOpRightsByOps(const PPIDArray * pOpList, PPIDArray * pResultOpList) const
{
	if(IsOpRights()) {
		ObjRestrictItem * p_item;
		for(uint i = 0; P_OpList->enumItems(&i, (void **)&p_item);) {
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

int PPRights::MaskOpRightsByTypes(const PPIDArray * pOpTypeList, PPIDArray * pResultOpList) const
{
	if(IsOpRights()) {
		ObjRestrictItem * p_item;
		for(uint i = 0; P_OpList->enumItems(&i, (void **)&p_item);) {
			PPID op_type_id = GetOpType(p_item->ObjID);
			if(pOpTypeList->lsearch(op_type_id))
				if(!pResultOpList->add(p_item->ObjID))
					return 0;
		}
		return 1;
	}
	else {
		SVector op_rec_list(sizeof(PPOprKind)); // @v10.6.8 SArray-->SVector
		if(PPRef->LoadItems(PPOBJ_OPRKIND, op_rec_list)) {
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

int PPRights::ExtentOpRights()
{
	if(IsOpRights()) {
		ObjRestrictArray temp_list;
		ObjRestrictItem * p_item;
		PPIDArray gen_op_list;
		for(uint i = 0; P_OpList->enumItems(&i, (void **)&p_item);) {
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

int PPRights::CheckOpID(PPID opID, long rtflags) const
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
					// @v11.0.0 {
					static const SIntToSymbTabEntry ftos_list[] = { 
						{ PPR_READ, "rt_view" }, { PPR_INS,  "rt_create" }, { PPR_MOD,  "rt_modif" }, { PPR_DEL,  "rt_delete" },
					};
					for(uint i = 0; i < SIZEOFARRAY(ftos_list); i++) {
						const long _f = ftos_list[i].Id;
						if(rtflags & _f && !(r_item.Flags & _f))
							added_msg.CatBrackStr(PPLoadStringS(ftos_list[i].P_Symb, temp_buf));
					}
					// } @v11.0.0 
					/* @v11.0.0 if(rtflags & PPR_READ && !(r_item.Flags & PPR_READ)) {
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
					}*/
					ok = PPSetError(PPERR_ISNTPRVLGFOROP, added_msg);
				}
			}
			else
				ok = -1;
		}
	}
	else
		ok = -1;
	return ok;
}

int PPRights::CheckLocID(PPID locID, long) const
	{ return (!SVectorBase::GetCount(P_LocList) || P_LocList->SearchItemByID(locID, 0)) ? 1 : PPSetError(PPERR_LOCNOTACCESSIBLE); }
int PPRights::CheckPosNodeID(PPID id, long flags) const
	{ return (!SVectorBase::GetCount(P_PosList) || P_PosList->SearchItemByID(id, 0)) ? 1 : PPSetError(PPERR_POSNODENOTACCESSIBLE); }
int PPRights::CheckQuotKindID(PPID id, long flags) const
	{ return (!SVectorBase::GetCount(P_QkList) || P_QkList->SearchItemByID(id, 0)) ? 1 : PPSetError(PPERR_QUOTKINDNOTACCESSIBLE); }

int PPRights::CheckAccID(PPID accID, long rt) const
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

int PPRights::CheckDesktopID(long deskID, long rt) const
{
	int    ok = 1;
	PPAccessRestriction accsr;
	if((GetAccessRestriction(accsr).CFlags & rt) != rt)
		ok = PPSetError(PPERR_NORIGHTS);
	return ok;
}

/*static*/ushort PPRights::GetDefaultFlags()
{
	return (0xffff & ~PPR_ADM);
}

/*static*/long PPRights::GetDefaultOprFlags()
{
	return 0xffffffff;
}

int FASTCALL GetCommConfig(PPCommConfig * pCfg)
{
	int    ok = 1, r = 0;
	memzero(pCfg, sizeof(*pCfg));
	THROW(r = PPRef->GetPropMainConfig(PPPRP_COMMCFG, pCfg, sizeof(PPCommConfig)));
	if(r <= 0) {
		memzero(pCfg, sizeof(*pCfg));
		pCfg->Tag  = PPOBJ_CONFIG;
		pCfg->ID   = PPCFG_MAIN;
		pCfg->Prop = PPPRP_COMMCFG;
		pCfg->SupplAcct.ac = DEFAC_SUPPL;
		pCfg->SellAcct.ac  = DEFAC_SELL;
		pCfg->CashAcct.ac  = DEFAC_CASH;
	}
	// @v9.7.0 { Теперь вместо кода товара хранится его идентификатора.
	// Для прозрачной обратной совместимости, сформируем значение идентификатора из кода, если таковой задан.
	if(!pCfg->PrepayInvoiceGoodsID && pCfg->PrepayInvoiceGoodsCode_obsolete[0]) {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(goods_obj.SearchByBarcode(pCfg->PrepayInvoiceGoodsCode_obsolete, 0, &goods_rec) > 0) {
			pCfg->PrepayInvoiceGoodsID = goods_rec.ID;
		}
	}
	// } @v9.7.0
	if(pCfg->MainOrgID == 0) {
		PPObjPerson psn_obj;
		PersonTbl::Rec psn_rec;
		if(psn_obj.P_Tbl->SearchMainOrg(&psn_rec) > 0)
			pCfg->MainOrgID = psn_rec.ID;
	}
	CATCHZOK
	return ok;
}

int FASTCALL SetCommConfig(const PPCommConfig * pCfg, int use_ta)
{
	return PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_COMMCFG, pCfg, sizeof(*pCfg), use_ta);
}
//
// PPSecurPacket
//
PPSecur2::PPSecur2()
{
	THISZERO();
}

PPSecurPacket::PPSecurPacket()
{
	// @v10.9.3 @ctr MEMSZERO(Secur);
}

PPSecurPacket & FASTCALL PPSecurPacket::operator = (const PPSecurPacket & rS)
{
	Secur  = rS.Secur;
	Config = rS.Config;
	Paths  = rS.Paths;
	Rights = rS.Rights;
	PrivateDesktopUUID = rS.PrivateDesktopUUID; // @v11.0.0
	return *this;
}
//
// PPPrinterCfg
//
//static const char * RpUseDuplexPrinting = "UseDuplexPrinting";
//static const char * RpStoreLastSelectedPrinter = "StoreLastSelectedPrinter"; // @v10.7.10
//static const char * RpLastSelectedPrinter = "LastSelectedPrinter"; // @v10.7.10
		//WrParam_UseDuplexPrinting("UseDuplexPrinting"),
		//WrParam_StoreLastSelectedPrinter("StoreLastSelectedPrinter"),
		//WrParam_LastSelectedPrinter("LastSelectedPrinter")

int PPGetPrinterCfg(PPID obj, PPID id, PPPrinterCfg * pCfg)
{
	int    ok = 1;
	int    use_duplex_printing = 0;
	int    store_last_selected_printer = 0;
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1); // @v9.2.0 readonly 0-->1
		uint32 val = 0;
		use_duplex_printing = BIN(reg_key.GetDWord(_PPConst.WrParam_UseDuplexPrinting, &val) && val);
		store_last_selected_printer = BIN(reg_key.GetDWord(_PPConst.WrParam_StoreLastSelectedPrinter, &val) && val);
	}
	if(obj == 0 && id == 0) {
		obj = PPOBJ_USR;
		id  = LConfig.UserID;
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

int PPSetPrinterCfg(PPID obj, PPID id, PPPrinterCfg * pCfg)
{
	{
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
		reg_key.PutDWord(_PPConst.WrParam_UseDuplexPrinting, BIN(pCfg->Flags & PPPrinterCfg::fUseDuplexPrinting));
		reg_key.PutDWord(_PPConst.WrParam_StoreLastSelectedPrinter, BIN(pCfg->Flags & PPPrinterCfg::fStoreLastSelPrn)); // @v10.7.10 
	}
	if(obj == 0 && id == 0) {
		obj = PPOBJ_USR;
		id  = LConfig.UserID;
	}
	return PPRef->SetConfig(obj, id, PPPRP_PRINTER, pCfg, sizeof(*pCfg));
}
//
//
//

#if 0 // {
/*static*/int FASTCALL UuidRefCore::TextToUuid(const char * pText, S_GUID & rUuid)
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

/*static*/int FASTCALL UuidRefCore::UuidToText(const S_GUID & rUuid, SString & rText)
{
	int    ok = 1;
	if(rUuid.IsZero()) {
		rText.Z();
		ok = -1;
	}
	else {
		rUuid.ToStr(S_GUID::fmtPlain, rText);
	}
	return ok;
}
#endif // } 0

UuidRefCore::UuidRefCore() : UuidRefTbl(), P_Hash(0)
{
}

UuidRefCore::~UuidRefCore()
{
	delete P_Hash;
}

int UuidRefCore::InitCache()
{
	int    ok = -1;
    if(!P_Hash) {
		P_Hash = new GuidHashTable(8*1024*1024);
		ok = P_Hash ? 1 : PPSetErrorNoMem();
    }
    return ok;
}

int UuidRefCore::Search(long id, S_GUID & rUuid)
{
	UuidRefTbl::Rec rec;
    int    ok = SearchByID(this, PPOBJ_UUIDREF, id, &rec);
    rUuid = *reinterpret_cast<const S_GUID *>(rec.UUID);
	return ok;
}

int UuidRefCore::SearchUuid(const S_GUID & rUuid, int useCache, long * pID)
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

int UuidRefCore::GetUuid(const S_GUID & rUuid, long * pID, int options, int use_ta)
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
				UuidRefTbl::Rec rec;
				// @v10.6.4 MEMSZERO(rec);
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

int UuidRefCore::PutChunk(const TSVector <S_GUID> & rChunk, uint maxCount, int use_ta) // @v9.8.4 TSArray-->TSVector
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
				// @v10.6.4 MEMSZERO(rec);
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

int UuidRefCore::RemoveUuid(const S_GUID & rUuid, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	THROW(SearchUuid(rUuid, 0, &id) > 0);
	THROW(RemoveByID(this, id, use_ta));
	CATCHZOK
	return ok;
}

int UuidRefCore::Remove(long id, int use_ta)
{
	return RemoveByID(this, id, use_ta);
}
//
//
//
/*void TagIdentToObjProp(long tagID, int16 * pO, int16 * pP) // @v10.0.04 @construction
{
	*pO = ((tagID >> 17) & 0x7000);
	*pP = ((tagID & 0x7fff)) & 0x7000);
}

long ObjPropToTagIdent(int16 o, int16 p) // @v10.0.04 @construction
{
	return MakeLong((p & ~0x7000), (o & ~0x7000);
}*/
//
TextRefIdent::TextRefIdent() : P(0), L(0)
{
}

TextRefIdent::TextRefIdent(PPID objType, PPID objID, int16 prop) : O(objType, objID), P(prop), L(0)
{
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
	int     FetchText(const char * pText, PPID * pID);
	void    FASTCALL SetTable(TextRefCore * pT);
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
	{
		return -1;
	}
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
	}
	ReadWriteLock TcRwl;
	SymbHashTable TextCache;
	TextRefCore * P_T; // @notowned
};

SelfTextRefCache::SelfTextRefCache() : ObjCache(PPOBJ_SELFREFTEXT, sizeof(ObjCacheEntry)), TextCache(4 * 1024 * 1024), P_T(0)
{
}

void FASTCALL SelfTextRefCache::SetTable(TextRefCore * pT)
{
	P_T = pT;
}

int SelfTextRefCache::FetchText(const char * pText, PPID * pID)
{
	int    ok = -1;
	long   _id = 0;
	SString pattern;
	pattern = pText;
	if(pattern.Len()) {
		pattern.ToLower1251();
		uint   hval = 0;
		uint   hpos = 0;
		{
			SRWLOCKER(TcRwl, SReadWriteLocker::Read);
			if(TextCache.Search(pattern, &hval, &hpos)) {
				_id = (long)hval;
				ok = 1;
			}
			else {
				if(P_T) {
					SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
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
		}
	}
	ASSIGN_PTR(pID, _id);
	return ok;
}

int TextRefCore::FetchSelfRefText(const char * pText, PPID * pID)
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
TextRefCore::TextRefCore() : TextRefTbl()
{
}

int TextRefCore::GetLastObjId(PPID objType, int prop, PPID * pID)
{
    int    ok = 1;
    PPID   id = 0;
    TextRefTbl::Key1 k1;
    MEMSZERO(k1);
    k1.ObjType = static_cast<int16>(objType);
    k1.Prop = static_cast<int16>(prop);
    k1.ObjID = MAXLONG;
    if(search(1, &k1, spLe) && data.ObjType == objType && data.Prop == static_cast<int16>(prop)) {
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

int TextRefCore::Search(const TextRefIdent & rI, SStringU & rBuf)
{
	int    ok = 1;
	TextRefTbl::Key1 k1;
	MEMSZERO(k1);
	k1.ObjType = static_cast<int16>(rI.O.Obj);
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

int TextRefCore::SearchSelfRef(long id, SStringU & rBuf)
{
    TextRefIdent ident(PPOBJ_SELFREFTEXT, id, PPTRPROP_DEFAULT);
    return Search(ident, rBuf);
}

int TextRefCore::SearchText(const TextRefIdent & rI, const wchar_t * pText, TextRefIdent * pResult)
{
	int    ok = -1;
	const size_t len = sstrlen(pText);
	if(len) {
		TextRefTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = static_cast<int16>(rI.O.Obj);
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

int TextRefCore::SearchTextByPrefix(const TextRefIdent & rI, const wchar_t * pPrefix, TSVector <TextRefIdent> * pList) // @v9.8.4 TSArray-->TSVector
{
	int    ok = -1;
	const size_t len = sstrlen(pPrefix);
	if(len) {
		TextRefTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = static_cast<int16>(rI.O.Obj);
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

int TextRefCore::SearchSelfRefTextByPrefix(const wchar_t * pPrefix, TSVector <TextRefIdent> * pList) // @v9.8.4 TSArray-->TSVector
{
	TextRefIdent ident(PPOBJ_SELFREFTEXT, 0, PPTRPROP_DEFAULT);
	return SearchTextByPrefix(ident, pPrefix, pList);
}

int TextRefCore::SearchSelfRefText(const wchar_t * pText, PPID * pID)
{
	TextRefIdent ident(PPOBJ_SELFREFTEXT, 0, PPTRPROP_DEFAULT);
	TextRefIdent result;
	int    ok = SearchText(ident, pText, &result);
	if(ok > 0) {
		ASSIGN_PTR(pID, result.O.Id);
	}
	return ok;
}

int TextRefCore::SetText(const TextRefIdent & rI, const wchar_t * pText, int use_ta)
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
			else if(_t.IsEq(pText)) {
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
			// @v10.6.4 MEMSZERO(rec);
			rec.ObjType = static_cast<int16>(rI.O.Obj);
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

int TextRefCore::GetSelfRefText(const wchar_t * pText, PPID * pID, int use_ta)
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
UnxTextRefCore::UnxTextRefCore() : UnxTextRefTbl()
{
}

int FASTCALL UnxTextRefCore::PostprocessRead(SStringU & rBuf)
{
	int    ok = 1;
	SBuffer temp_buf;
	readLobData(VT, temp_buf);
	destroyLobData(VT); // @v10.2.11 @fix
	const size_t actual_size = temp_buf.GetAvailableSize();
	rBuf.CopyFromUtf8(static_cast<const char *>(temp_buf.GetBuf(0)), actual_size);
	return ok;
}

int UnxTextRefCore::Search(const TextRefIdent & rI, SStringU & rBuf)
{
	//
	// Так как текст в этой таблице по определению не индексируемый, то
	// с версии 9.0.0 в записях хранится не "сырой" unicode, а utf8 (ради экономии пространства и производительности).
	// При этом интерфейсные функции по-прежнему оперируют форматом unicode (для совместимости с TextRefCore).
	//
	rBuf.Z();

	int    ok = 1;
	if(rI.P == PPTRPROP_TIMESERIES) {
		SString temp_buf;
		rI.O.ToStr(temp_buf).CatDiv(';', 2).Cat(rI.P);
		ok = PPSetError(PPERR_INVTXREFPROPVALUE, temp_buf);
	}
	else {
		UnxTextRefTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = static_cast<int16>(rI.O.Obj);
		k0.Prop = rI.P;
		k0.ObjID = rI.O.Id;
		k0.Lang = rI.L;
		if(search(0, &k0, spEq))
			PostprocessRead(rBuf);
		else
			ok = PPDbSearchError();
	}
	return ok;
}

int UnxTextRefCore::Search(const TextRefIdent & rI, STimeSeries & rTs)
{
	rTs.Z();
	int    ok = 1;
	const  int16 prop = NZOR(rI.P, PPTRPROP_TIMESERIES);
	if(prop != PPTRPROP_TIMESERIES) {
		SString temp_buf;
		rI.O.ToStr(temp_buf).CatDiv(';', 2).Cat(prop);
		ok = PPSetError(PPERR_INVTXREFPROPVALUE, temp_buf);
	}
	else {
		UnxTextRefTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = static_cast<int16>(rI.O.Obj);
		k0.Prop = prop;
		k0.ObjID = rI.O.Id;
		k0.Lang = rI.L;
		if(search(0, &k0, spEq)) {
			SSerializeContext sctx;
			SBuffer temp_buf;
			readLobData(VT, temp_buf);
			destroyLobData(VT); // @v10.2.11 @fix
			const size_t actual_size = temp_buf.GetAvailableSize();
			const size_t cs_size = SSerializeContext::GetCompressPrefix(0);
			if(actual_size > cs_size && SSerializeContext::IsCompressPrefix(temp_buf.GetBuf(temp_buf.GetRdOffs()))) {
				SCompressor compr(SCompressor::tZLib);
				SBuffer dbuf;
				THROW_SL(compr.DecompressBlock(temp_buf.GetBuf(temp_buf.GetRdOffs()+cs_size), actual_size-cs_size, dbuf));
				THROW_SL(rTs.Serialize(-1, dbuf, &sctx));
			}
			else {
				THROW_SL(rTs.Serialize(-1, temp_buf, &sctx));
			}
		}
		else
			ok = PPDbSearchError();
	}
	CATCHZOK
	return ok;
}

int UnxTextRefCore::GetText(const TextRefIdent & rI, SString & rBuf)
{
	rBuf.Z();
	SStringU temp_buf_u;
	int    ok = Search(rI, temp_buf_u);
	if(ok > 0) {
        temp_buf_u.CopyToUtf8(rBuf, 1);
	}
	return ok;
}

int UnxTextRefCore::SetText(const TextRefIdent & rI, const char * pText, int use_ta)
{
	int    ok = -1;
	const  size_t src_len = sstrlen(pText);
	if(src_len) {
		SStringU & r_temp_buf = SLS.AcquireRvlStrU(); // @v10.5.6 @revolver
		r_temp_buf.CopyFromUtf8(pText, src_len);
		ok = SetText(rI, r_temp_buf, use_ta);
	}
	else
		ok = SetText(rI, static_cast<const wchar_t *>(0), use_ta);
	return ok;
}

int UnxTextRefCore::SetText(const TextRefIdent & rI, const wchar_t * pText, int use_ta)
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
			else if(_t.IsEq(pText)) {
				ok = -1;
			}
			else {
				THROW_DB(rereadForUpdate(0, 0)); // @v9.0.5 @fix index 1-->0
				{
					assert(tl); // Ранее мы проверили длину текста на 0
					THROW(writeLobData(VT, utf_buf.cptr(), tl));
					data.Size = static_cast<long>(tl);
				}
				THROW_DB(updateRec()); // @sfu
				destroyLobData(VT); // @v10.2.11 @fix
			}
		}
		else if(tl == 0) {
			ok = -2;
		}
		else {
			MEMSZERO(data);
			data.ObjType = static_cast<int16>(rI.O.Obj);
			data.ObjID = rI.O.Id;
			data.Prop = rI.P;
			data.Lang = rI.L;
			{
				assert(tl); // Ранее мы проверили длину текста на 0
				THROW(writeLobData(VT, utf_buf.cptr(), tl));
				data.Size = static_cast<long>(tl);
			}
			THROW_DB(insertRec());
			destroyLobData(VT); // @v10.2.11 @fix
		}
		THROW(tra.Commit());
    }
    CATCHZOK
    return ok;
}

int UnxTextRefCore::SetTimeSeries(const TextRefIdent & rI, STimeSeries * pTs, int use_ta)
{
    int    ok = 1;
    SBuffer sbuf;
    SBuffer cbuf;
	STimeSeries ex_ts;
    SSerializeContext sctx;
    THROW_INVARG(rI.L >= 0);
    THROW_INVARG(rI.P >= 0);
    THROW_INVARG(rI.O.Obj > 0);
    THROW_INVARG(rI.O.Id > 0);
    if(pTs) {
        SCompressor compr(SCompressor::tZLib);
		THROW_SL(pTs->Serialize(+1, sbuf, &sctx));
		if(sbuf.GetAvailableSize() > 128) {
			uint8 cs[32];
			const size_t cs_size = SSerializeContext::GetCompressPrefix(cs);
			THROW_SL(cbuf.Write(cs, cs_size));
			THROW_SL(compr.CompressBlock(sbuf.GetBuf(0), sbuf.GetAvailableSize(), cbuf, 0, 0));
		}
		else {
			cbuf = sbuf; // @v10.3.1 @fix
		}
    }
	const  size_t tl = cbuf.GetAvailableSize();
    {
    	PPTransaction tra(use_ta);
    	THROW(tra);
		int    sr = Search(rI, ex_ts);
		THROW(sr);
		if(sr > 0) {
			if(tl == 0) {
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(deleteRec()); // @sfu
			}
			/*else if(_t.IsEq(pText)) {
				ok = -1;
			}*/
			else {
				THROW_DB(rereadForUpdate(0, 0));
				{
					assert(tl); // Ранее мы проверили длину текста на 0
					THROW(writeLobData(VT, cbuf.GetBuf(0), tl));
					data.Size = static_cast<long>(tl);
				}
				THROW_DB(updateRec()); // @sfu
				destroyLobData(VT); // @v10.2.11 @fix
			}
		}
		else if(tl == 0) {
			ok = -2;
		}
		else {
			MEMSZERO(data);
			data.ObjType = static_cast<int16>(rI.O.Obj);
			data.ObjID = rI.O.Id;
			data.Prop = rI.P;
			data.Lang = rI.L;
			{
				assert(tl); // Ранее мы проверили длину текста на 0
				THROW(writeLobData(VT, cbuf.GetBuf(0), tl));
				data.Size = (long)tl;
			}
			THROW_DB(insertRec());
			destroyLobData(VT); // @v10.2.11 @fix
		}
		THROW(tra.Commit());
    }
    CATCHZOK
    return ok;
}

UnxTextRefCore::_Enum::_Enum(UnxTextRefCore * pT, long h) : P_T(pT), H(h)
{
}

UnxTextRefCore::_Enum::~_Enum()
{
	CALLPTRMEMB(P_T, EnumList.DestroyIterHandler(H));
}

int UnxTextRefCore::_Enum::Next(void * pRec)
{
	int    ok = 0;
	if(P_T && P_T->EnumList.NextIter(H) > 0) {
		TextRefEnumItem * p_result = static_cast<TextRefEnumItem *>(pRec);
		p_result->O.Set(P_T->data.ObjType, P_T->data.ObjID);
		p_result->P = P_T->data.Prop;
		p_result->L = P_T->data.Lang;
		if(P_T->GetText(*p_result, p_result->S) > 0)
			ok = 1;
	}
	return ok;
}

int UnxTextRefCore::InitEnum(PPID objType, int prop, long * pHandle)
{
	BExtQuery * q = new BExtQuery(this, 0);
	q->select(this->ObjType, this->ObjID, this->Prop, this->Lang, this->Size, 0);
	if(prop)
		q->where(this->ObjType == objType && this->Prop == static_cast<long>(prop));
	else
		q->where(this->ObjType == objType);
	UnxTextRefTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = static_cast<int16>(objType);
	q->initIteration(false, &k0, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

SEnum::Imp * UnxTextRefCore::Enum(PPID objType, int prop)
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
		THROW(SLTEST_CHECK_NZ(item_list1.IsEq(item_list2)));
	}
	CATCH
		ok = CurrentStatus = 0;
	ENDCATCH
	delete p_rng;
	if(hdl_enum >= 0)
		p_tbl->DestroyIter(hdl_enum);
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
