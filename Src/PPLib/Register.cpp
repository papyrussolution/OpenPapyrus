// REGISTER.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2014, 2015, 2016, 2019, 2020, 2022, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

PPBankAccount::PPBankAccount()
{
    THISZERO();
    ObjType = PPOBJ_PERSON;
    RegTypeID = PPREGT_BANKACCOUNT;
}

PPBankAccount::PPBankAccount(const RegisterTbl::Rec & rS)
{
	*this = rS;
}

PPBankAccount & FASTCALL PPBankAccount::operator = (const RegisterTbl::Rec & rS)
{
	ID = rS.ID;
	ObjType = rS.ObjType;
	assert(ObjType == PPOBJ_PERSON);
	PersonID = rS.ObjID;
	UnusedZero = 0;
    assert(rS.RegTypeID == PPREGT_BANKACCOUNT);
	RegTypeID = PPREGT_BANKACCOUNT;
	OpenDate = rS.Dt;
	BankID = rS.RegOrgID;
	memzero(UnusedZ2, sizeof(UnusedZ2));
	STRNSCPY(Acct, rS.Num);
	Expiry = rS.Expiry;
	UniqCntr = rS.UniqCntr;
	Flags = rS.Flags;
	AccType = rS.ExtID;
	return *this;
}

void FASTCALL PPBankAccount::GetRegisterRec(RegisterTbl::Rec & rRec) const
{
    MEMSZERO(rRec);
    rRec.ID = ID;
    rRec.ObjType = PPOBJ_PERSON;
    rRec.ObjID = PersonID;
    rRec.RegTypeID = PPREGT_BANKACCOUNT;
    rRec.Dt = OpenDate;
    rRec.RegOrgID = BankID;
    STRNSCPY(rRec.Num, Acct);
    rRec.Expiry = Expiry;
    rRec.UniqCntr = UniqCntr;
    rRec.Flags = Flags;
	rRec.ExtID = AccType;
}

/* @v12.0.0 (replaced with !RegisterCore::IsEqualRec() && r1.ID != r2.ID)
int FASTCALL operator != (const RegisterTbl::Rec & r1, const RegisterTbl::Rec & r2)
{
	if(r1.ID != r2.ID) return 1;
	if(r1.ObjType != r2.ObjType || r1.ObjID != r2.ObjID) return 1;
	if(r1.PsnEventID != r2.PsnEventID) return 1;
	if(r1.RegTypeID != r2.RegTypeID) return 1;
	if(r1.Dt != r2.Dt) return 1;
	if(r1.RegOrgID != r2.RegOrgID) return 1;
	if(r1.Expiry != r2.Expiry) return 1;
	if(r1.Flags != r2.Flags) return 1;
	if(r1.ExtID != r2.ExtID) return 1; // @v10.6.0
	if(!sstreq(r1.Serial, r2.Serial)) return 1;
	if(!sstreq(r1.Num, r2.Num)) return 1;
	return 0;
}*/

/*static*/int FASTCALL RegisterCore::IsEqualRec(const RegisterTbl::Rec & rRec1, const RegisterTbl::Rec & rRec2)
{
	#define ISEQ(f) (rRec1.f == rRec2.f)
	if(ISEQ(ObjType) && ISEQ(ObjID) && ISEQ(PsnEventID) && ISEQ(RegTypeID) && ISEQ(Dt) && ISEQ(RegOrgID) &&
		ISEQ(Expiry) && ISEQ(Flags) && ISEQ(ExtID) && sstreq(rRec1.Serial, rRec2.Serial) && sstreq(rRec1.Num, rRec2.Num))
		return 1;
	else
		return 0;
	#undef ISEQ
}
//
// RegisterArray
//
RegisterArray::RegisterArray() : SVector(sizeof(RegisterTbl::Rec))
{
}

RegisterArray::RegisterArray(const RegisterArray & s) : SVector(s)
{
}

IMPL_CMPFUNC(RegisterTbl_Rec_TDE, i1, i2) 
	{ RET_CMPCASCADE3(static_cast<const RegisterTbl::Rec *>(i1), static_cast<const RegisterTbl::Rec *>(i2), RegTypeID, Dt, Expiry); }

void RegisterArray::Sort()
{
	sort(PTR_CMPFUNC(RegisterTbl_Rec_TDE));
}

int FASTCALL RegisterArray::IsEq(const RegisterArray & rS) const
{
	int    eq = 1;
	const uint c = getCount();
	const uint c2 = rS.getCount();
	if(c != c2)
		eq = 0;
	else if(c) {
		//
		// Перестановка элементов считается отличием (хотя, сейчас она и не важна)
		//
		for(uint i = 0; eq && i < c; i++) {
			const RegisterTbl::Rec & r_rec = at(i);
			const RegisterTbl::Rec & r_rec2 = rS.at(i);
			if(!RegisterCore::IsEqualRec(r_rec, r_rec2))
				eq = 0;
		}
	}
	return eq;
}

int FASTCALL RegisterArray::HasEqual(const RegisterTbl::Rec & rRec) const
{
	const uint c = getCount();
	for(uint i = 0; i < c; i++)
		if(RegisterCore::IsEqualRec(at(i), rRec))
			return (i+1);
	return 0;
}

RegisterTbl::Rec & FASTCALL RegisterArray::at(uint pos) const
{
	return *static_cast<RegisterTbl::Rec *>(SVector::at(pos));
}

int RegisterArray::GetRegister(PPID regTyp, uint * pPos, RegisterTbl::Rec * pRec) const
{
	return GetRegister(regTyp, ZERODATE, pPos, pRec);
}

struct _RegCandidItem { // @flat
	uint   P;
    long   StartDist;
    long   ExpiryDist;
};

int RegisterArray::SelectRegister(PPID regTyp, LDATE dt, uint * pPos, RegisterTbl::Rec * pRec) const
{
	int    ok = srrNothing;
	int    optimal_pos = -1;
	TSVector <_RegCandidItem> candid_list;
	for(uint i = 0; ok == srrNothing && i < getCount(); i++) {
		const RegisterTbl::Rec & r_reg = at(i);
		if(r_reg.RegTypeID == regTyp) {
			if(!dt) {
				optimal_pos = static_cast<int>(i);
				ok = srrEmptyDateCrit;
			}
			else if((!r_reg.Expiry || r_reg.Expiry >= dt) && (!r_reg.Dt || r_reg.Dt <= dt)) {
				_RegCandidItem rci;
				rci.P = i;
				rci.StartDist = r_reg.Dt ? diffdate(dt, r_reg.Dt) : -1;
				rci.ExpiryDist = r_reg.Expiry ? diffdate(r_reg.Expiry, dt) : -1;
				candid_list.insert(&rci);
			}
		}
	}
	if(ok == srrNothing) {
        if(candid_list.getCount()) {
			if(candid_list.getCount() == 1) {
				optimal_pos = static_cast<int>(candid_list.at(0).P);
				ok = srrSingle;
			}
			else {
				long   rci_min_start_dist = MAXLONG;
				int    rci_min_start_dist_pos = -1;
				long   rci_max_expiry_dist = -1;
				int    rci_max_expiry_dist_pos = -1;
				for(uint j = 0; j < candid_list.getCount(); j++) {
					const _RegCandidItem & r_rci = candid_list.at(j);
					if(r_rci.StartDist >= 0 && r_rci.StartDist < rci_min_start_dist) {
						rci_min_start_dist = r_rci.StartDist;
						rci_min_start_dist_pos = r_rci.P;
					}
					if(r_rci.ExpiryDist >= 0 && r_rci.ExpiryDist > rci_max_expiry_dist) {
						rci_max_expiry_dist = r_rci.ExpiryDist;
						rci_max_expiry_dist_pos = r_rci.P;
					}
				}
				if(rci_min_start_dist_pos >= 0) {
					optimal_pos = rci_min_start_dist_pos;
					ok = srrMinStartDist;
				}
				else if(rci_max_expiry_dist_pos >= 0) {
					optimal_pos = rci_max_expiry_dist_pos;
					ok = srrMaxExpiryDist;
				}
				else {
					optimal_pos = static_cast<int>(candid_list.at(0).P);
					ok = srrAmbig;
				}
			}
        }
	}
	if(ok > 0) {
		assert(optimal_pos >= 0 && optimal_pos < static_cast<int>(getCount()));
		ASSIGN_PTR(pPos, static_cast<uint>(optimal_pos+1));
		ASSIGN_PTR(pRec, at(optimal_pos));
	}
	else {
		memzero(pRec, sizeof(*pRec));
	}
	return ok;
}

int RegisterArray::SelectRegNumber(PPID regTyp, LDATE dt, SString & rBuf) const
{
	uint   pos = 0;
	int    r = SelectRegister(regTyp, dt, &pos, 0);
	if(r > 0) {
		const RegisterTbl::Rec & r_reg = at(pos-1);
        rBuf = r_reg.Num;
	}
	else
		rBuf.Z();
	return r;
}

int RegisterArray::GetRegister(PPID regTyp, LDATE dt, uint * pPos, RegisterTbl::Rec * pRec) const
{
	for(uint i = DEREFPTRORZ(pPos); i < getCount(); i++) {
		const RegisterTbl::Rec & r_reg = at(i);
		if(r_reg.RegTypeID == regTyp && (!dt || !r_reg.Expiry || r_reg.Expiry >= dt)) {
			ASSIGN_PTR(pRec, r_reg);
			ASSIGN_PTR(pPos, i+1);
			return 1;
		}
	}
	return -1;
}

int RegisterArray::GetListByType(PPID regTyp, LDATE dt, RegisterArray * pList) const
{
	int    ok = -1;
	CALLPTRMEMB(pList, clear());
	for(uint i = 0; i < getCount(); i++) {
		const RegisterTbl::Rec & r_reg = at(i);
		if(r_reg.RegTypeID == regTyp && (!dt || !r_reg.Expiry || r_reg.Expiry >= dt)) {
			ok = 1;
			if(pList) {
				THROW_SL(pList->insert(&r_reg));
			}
			else
				break;
		}
	}
	CATCHZOK
	return ok;
}

int RegisterArray::GetBankAccountList(TSVector <PPBankAccount> * pList) const
{
	const  LDATE dt = ZERODATE;
	int    ok = -1;
	CALLPTRMEMB(pList, clear());
	for(uint i = 0; i < getCount(); i++) {
		const RegisterTbl::Rec & r_reg = at(i);
		if(r_reg.RegTypeID == PPREGT_BANKACCOUNT && (!dt || !r_reg.Expiry || r_reg.Expiry >= dt)) {
			ok = 1;
			if(pList) {
                PPBankAccount new_item;
				new_item = r_reg;
				THROW_SL(pList->insert(&new_item));
			}
			else
				break;
		}
	}
	CATCHZOK
	return ok;
}

int RegisterArray::CheckDuplicateBankAccount(const PPBankAccount * pRec, long pos) const
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		const RegisterTbl::Rec & r_reg_rec = at(i);
		if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
			PPBankAccount ba = r_reg_rec;
			if(stricmp(pRec->Acct, ba.Acct) == 0 && pRec->BankID == ba.BankID && (uint)pos != i) {
				ok = PPSetError(PPERR_DUPLBNKACCT, ba.Acct);
				break;
			}
		}
	}
	return ok;
}

int RegisterArray::SetBankAccount(const PPBankAccount * pRec, uint pos)
{
	int    ok = -1;
    RegisterTbl::Rec reg_rec;
    CALLPTRMEMB(pRec, GetRegisterRec(reg_rec));
    if(pos < getCount()) {
        RegisterTbl::Rec & r_ex_rec = at(pos);
		if(pRec) {
			assert(r_ex_rec.RegTypeID == reg_rec.RegTypeID);
			if(r_ex_rec.RegTypeID == reg_rec.RegTypeID) {
				at(pos) = reg_rec;
				ok = 1;
			}
		}
		else {
			assert(r_ex_rec.RegTypeID == PPREGT_BANKACCOUNT);
			if(r_ex_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				atFree(pos);
				ok = 1;
			}
		}
    }
    else {
		assert(pRec);
		if(pRec) {
			THROW_SL(insert(&reg_rec));
			pos = getCount()-1;
			ok = 1;
		}
    }
    if(ok > 0 && pRec && pRec->Flags & PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/) {
		for(uint i = 0; i < getCount(); i++) {
			RegisterTbl::Rec & r_rec = at(i);
            if(i != pos && r_rec.RegTypeID == PPREGT_BANKACCOUNT && r_rec.Flags & PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/) {
				r_rec.Flags &= ~PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/;
            }
		}
    }
    CATCHZOK
	return ok;
}

int RegisterArray::GetListByPeriod(PPID regTypeID, const DateRange & rPeriod, RegisterArray * pList) const
{
	int    ok = -1;
	if(pList)
		pList->clear();
	const LDATE _l = rPeriod.low;
	const LDATE _u = rPeriod.upp;
	for(uint i = 0; i < getCount(); i++) {
		const RegisterTbl::Rec & r_reg = at(i);
		if(r_reg.RegTypeID == regTypeID && (!r_reg.Expiry || (r_reg.Expiry >= _l)) && (!r_reg.Dt || !_u || r_reg.Dt <= _u)) {
			CALLPTRMEMB(pList, insert(&r_reg));
			ok = 1;
		}
	}
	return ok;
}

int RegisterArray::GetRegNumber(PPID regTyp, SString & rBuf) const
{
	return GetRegNumber(regTyp, ZERODATE, rBuf);
}

int RegisterArray::GetRegNumber(PPID regTyp, LDATE dt, SString & rBuf) const
{
	rBuf.Z();
	int    ok = -1;
	uint   p = 0;
	if(GetRegister(regTyp, dt, &p, 0) > 0) {
		const RegisterTbl::Rec & r_reg = at(p-1);
		rBuf = r_reg.Num;
		ok = 1;
	}
	return ok;
}

int RegisterArray::Merge(const RegisterArray & rS)
{
	int    ok = -1;
	PPObjRegisterType rt_obj;
	PPRegisterType rt_rec;
	LongArray add_pos_list;
	LongArray rmv_pos_list;
	uint   j;
	for(j = 0; j < rS.getCount(); j++) {
		const RegisterTbl::Rec & r_rec = rS.at(j);
		int    nfound = 1;
		for(uint i = 0; i < getCount(); i++) {
			const RegisterTbl::Rec & r_this_rec = at(i);
			if(r_rec.RegTypeID == r_this_rec.RegTypeID) {
				nfound = 0;
				if(rt_obj.Fetch(r_rec.RegTypeID, &rt_rec) > 0) {
					if(rt_rec.Flags & REGTF_UNIQUE) {
						rmv_pos_list.addUnique(static_cast<long>(i));
						add_pos_list.add((long)j);
					}
					else if(stricmp(r_rec.Serial, r_this_rec.Serial) != 0 || stricmp(r_rec.Num, r_this_rec.Num) != 0) {
						add_pos_list.add((long)j);
					}
				}
				else
					rmv_pos_list.addUnique(static_cast<long>(i));
			}
		}
		if(nfound)
			add_pos_list.add((long)j);
	}
	if(add_pos_list.getCount() || rmv_pos_list.getCount()) {
		rmv_pos_list.sort();
		for(int k = rmv_pos_list.getCount()-1; k >= 0; k--) {
			atFree(rmv_pos_list.get(k));
		}
		for(j = 0; j < add_pos_list.getCount(); j++) {
			insert(&rS.at(add_pos_list.get(j)));
		}
		ok = 1;
	}
	return ok;
}

int RegisterArray::ProcessObjRefs(PPObjIDArray * ary, int replace)
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		RegisterTbl::Rec & r_reg_rec = at(i);
		if(replace)
			r_reg_rec.ID = 0;
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_REGISTERTYPE, &r_reg_rec.RegTypeID, ary, replace));
		THROW(PPObject::ProcessObjRefInArray(PPOBJ_PERSON, &r_reg_rec.RegOrgID, ary, replace));
		if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
			THROW(PPObject::ProcessObjRefInArray(PPOBJ_BNKACCTYPE, &r_reg_rec.ExtID, ary, replace));
		}
		else if(r_reg_rec.RegTypeID == PPREGT_TAXSYSTEM) {
			THROW(PPObject::ProcessObjRefInArray(PPOBJ_TAXSYSTEMKIND, &r_reg_rec.ExtID, ary, replace));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
RegisterCore::RegisterCore() : RegisterTbl()
{
}

int RegisterCore::Search(PPID id, RegisterTbl::Rec * b) { return SearchByID(this, PPOBJ_REGISTER, id, b); }

int RegisterCore::SearchByNumber(PPID * pID, PPID regTypeID, const char * pSerial, const char * pNumber, RegisterTbl::Rec * pBuf)
{
	RegisterTbl::Key3 k;
	MEMSZERO(k);
	k.RegTypeID = regTypeID;
	STRNSCPY(k.Serial, pSerial);
	STRNSCPY(k.Num, pNumber);
	k.UniqCntr = 0;
	int    ok = SearchByKey(this, 3, &k, pBuf);
	ASSIGN_PTR(pID, ((ok > 0) ? data.ID : 0));
	return ok;
}

int RegisterCore::GetUniqCntr(RegisterTbl::Rec * pRec, int forceDup)
{
	pRec->UniqCntr = 0;
	if(forceDup || PPObjRegisterType::IsDupRegType(pRec->RegTypeID))
		pRec->Flags |= PREGF_DUPNUMBER;
	if(pRec->Flags & PREGF_DUPNUMBER) {
		RegisterTbl::Key3 k3;
		MEMSZERO(k3);
		k3.RegTypeID = pRec->RegTypeID;
		memcpy(k3.Serial, pRec->Serial, sizeof(k3.Serial));
		memcpy(k3.Num, pRec->Num, sizeof(k3.Num));
		k3.UniqCntr = MAXLONG;
		if(search(3, &k3, spLe) && k3.RegTypeID == pRec->RegTypeID && sstreq(k3.Serial, pRec->Serial) && sstreq(k3.Num, pRec->Num))
			pRec->UniqCntr = k3.UniqCntr+1;
	}
	return 1;
}

int RegisterCore::Add(PPID * pID, RegisterTbl::Rec * pRec, int use_ta)
{
	GetUniqCntr(pRec, 0 /* !forceDup */);
	return AddByID(this, pID, pRec, use_ta);
}

int RegisterCore::Add_ForceDup(PPID * pID, RegisterTbl::Rec * pRec, int use_ta)
{
	GetUniqCntr(pRec, 1 /* forceDup */);
	return AddByID(this, pID, pRec, use_ta);
}

int RegisterCore::Update(PPID id, RegisterTbl::Rec * pRec, int use_ta)
{
	GetUniqCntr(pRec, 0 /* !forceDup */);
	return UpdateByID(this, PPOBJ_REGISTER, id, pRec, use_ta);
}

int RegisterCore::Remove(PPID id, int use_ta)
{
	return RemoveByID(this, id, use_ta);
}

int RegisterCore::SetByPerson(PPID personID, PPID regTypeID, const RegisterTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pRec) {
			RegisterTbl::Rec temp_rec;
			if(pRec->ID) {
				THROW(Search(pRec->ID, &temp_rec) > 0);
				temp_rec = *pRec;
				temp_rec.ObjType = PPOBJ_PERSON;
				temp_rec.ObjID = personID;
				THROW(Update(temp_rec.ID, &temp_rec, 0));
			}
			else {
				PPID   id = 0;
				if(pRec->RegTypeID) {
					THROW(SetByPerson(personID, pRec->RegTypeID, 0, 0));
				}
				temp_rec = *pRec;
				temp_rec.ObjID = PPOBJ_PERSON;
				temp_rec.ObjID = personID;
				THROW(Add(&id, &temp_rec, 0));
			}
		}
		else {
			THROW(deleteFrom(this, 0, *ppcheckfiltid(&(this->ObjType == PPOBJ_PERSON && this->ObjID == personID), this->RegTypeID, regTypeID)));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL RegisterCore::GetText(const RegisterTbl::Rec & rRec, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString temp_buf;
	PPID   obj_id = 0;
	if(rRec.ObjType && rRec.ObjID) {
		GetObjectTitle(rRec.ObjType, temp_buf);
		obj_id = rRec.ObjID;
	}
	else if(rRec.PsnEventID) {
		GetObjectTitle(PPOBJ_PERSONEVENT, temp_buf);
		obj_id = rRec.PsnEventID;
	}
	if(obj_id)
		rBuf.Cat(temp_buf).CatParStr(obj_id).Space();
	GetRegisterTypeName(rRec.RegTypeID, temp_buf);
	rBuf.Cat(temp_buf);
	if(rRec.Serial[0]) {
		rBuf.CatDiv(';', 2).Cat(rRec.Serial).CatDiv(':', 0);
		if(rRec.Num)
			rBuf.Cat(rRec.Num);
	}
	else if(rRec.Num[0])
		rBuf.CatDiv(';', 2).Cat(rRec.Num);
	if(rRec.Dt)
		rBuf.CatDiv(';', 2).Cat(rRec.Dt);
	return ok;
}

int RegisterCore::_Get(PPID objType, PPID id, RegisterArray * pAry)
{
	int    idx, _count = 0;
	PROFILE_START
	union {
		RegisterTbl::Key1 k1;
		RegisterTbl::Key2 k2;
	} k;
	uint   eq_count = 0;
	SString reg_text, obj_name, fmt_buf, msg_buf;
	MEMSZERO(k);
	if(objType == PPOBJ_PERSONEVENT) {
		idx = 2;
		k.k2.PsnEventID = id;
	}
	else if(oneof2(objType, PPOBJ_PERSON, PPOBJ_LOCATION)) {
		idx = 1;
		k.k1.ObjType = objType;
		k.k1.ObjID = id;
	}
	else
		return PPSetErrorInvParam();
	if(search(idx, &k, spEq)) do {
		if(objType == PPOBJ_PERSONEVENT) {
			if(data.PsnEventID != id)
				break;
		}
		else if(oneof2(objType, PPOBJ_PERSON, PPOBJ_LOCATION)) {
			if(data.ObjType != objType || data.ObjID != id)
				break;
		}
		if(!pAry)
			return 1;
		else if(pAry->HasEqual(data)) {
			int    alarm = 0;
			++eq_count;
			if(id) {
				if(eq_count == 10) {
					obj_name.Z();
					int    msg_code = 0;
					switch(objType) {
						case PPOBJ_PERSONEVENT:
							ideqvalstr(id, obj_name);
							msg_code = PPTXT_TOOMANYEQREG_PSNEV;
							break;
						case PPOBJ_PERSON:
							GetPersonName(id, obj_name);
							msg_code = PPTXT_TOOMANYEQREG_PSN;
							break;
						case PPOBJ_LOCATION:
							GetLocationName(id, obj_name);
							msg_code = PPTXT_TOOMANYEQREG_LOC;
							break;
					}
					if(msg_code)
						PPLoadText(msg_code, fmt_buf);
					RegisterCore::GetText(data, reg_text);
					msg_buf.Printf(fmt_buf, obj_name.cptr(), reg_text.cptr());
					alarm = 1;
				}
			}
			else if(eq_count == 1000) {
				RegisterCore::GetText(data, reg_text);
				msg_buf.Printf(PPLoadTextS(PPTXT_TOOMANYEQREG_DB, fmt_buf), reg_text.cptr());
				alarm = 1;
			}
			if(alarm) {
				PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
		else if(!pAry->insert(&data))
			return PPSetErrorSLib();
		else
			_count++;
	} while(search(idx, &k, spNext));
	PROFILE_END
	return _count ? 1 : -1;
}

int RegisterCore::_Put(PPID objType, PPID objID, RegisterArray * pAry, int use_ta)
{
	int    ok = 1;
	uint   i;
	PPID   reg_id;
	PPIDArray update_list, remove_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			int    idx;
			union {
				RegisterTbl::Key1 k1;
				RegisterTbl::Key2 k2;
			} k;
			DBQ  * dbq = 0;
			MEMSZERO(k);
			if(objType == PPOBJ_PERSONEVENT) {
				idx = 2;
				k.k2.PsnEventID = objID;
				dbq = & (this->PsnEventID == objID);
			}
			else if(oneof2(objType, PPOBJ_PERSON, PPOBJ_LOCATION)) {
				idx = 1;
				k.k1.ObjType = objType;
				k.k1.ObjID = objID;
				dbq = & (this->ObjType == objType && this->ObjID == objID);
			}
			else
				CALLEXCEPT_PP(PPERR_INVPARAM);
			assert(oneof2(idx, 1, 2));
			{
				if(search(idx, &k, spEq)) do {
					if(objType == PPOBJ_PERSONEVENT) {
						if(data.PsnEventID != objID)
							break;
					}
					else if(oneof2(objType, PPOBJ_PERSON, PPOBJ_LOCATION)) {
						if(data.ObjType != objType || data.ObjID != objID)
							break;
					}
					reg_id = data.ID;
					if(!pAry || !pAry->lsearch(&reg_id, &(i = 0), CMPF_LONG)) {
						THROW(remove_list.add(reg_id));
					}
					else {
						const RegisterTbl::Rec & r_new_rec = pAry->at(i);
						if(!RegisterCore::IsEqualRec(data, r_new_rec) || data.ID != r_new_rec.ID) { // @v12.0.0 
						// @v12.0.0 if(data != r_new_rec) {
							THROW(update_list.add(reg_id));
						}
					}
				} while(search(idx, &k, spNext));
			}
		}
		for(i = 0; i < remove_list.getCount(); i++) {
			THROW(Remove(remove_list.at(i), 0));
		}
		if(pAry) {
			RegisterTbl::Rec * p_rec;
			for(i = 0; pAry->enumItems(&i, (void **)&p_rec);) {
				if(!(p_rec->Flags & PREGF_INHERITED)) {
					if((reg_id = p_rec->ID) != 0) {
						if(update_list.lsearch(reg_id))
							THROW(Update(reg_id, p_rec, 0));
					}
					else {
						if(objType == PPOBJ_PERSONEVENT) {
							p_rec->ObjType = 0;
							p_rec->PsnEventID = objID;
						}
						else if(oneof2(objType, PPOBJ_PERSON, PPOBJ_LOCATION)) {
							p_rec->ObjType = objType;
							p_rec->ObjID = objID;
						}
						THROW(Add(&p_rec->ID, p_rec, 0));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int RegisterCore::PutByEvent(PPID eventID, RegisterArray * pAry, int use_ta)
	{ return _Put(PPOBJ_PERSONEVENT, eventID, pAry, use_ta); }
int RegisterCore::PutByPerson(PPID personID, RegisterArray * ary, int use_ta)
	{ return _Put(PPOBJ_PERSON, personID, ary, use_ta); }
int RegisterCore::PutByLocation(PPID locID, RegisterArray * pList, int use_ta)
	{ return _Put(PPOBJ_LOCATION, locID, pList, use_ta); }

int RegisterCore::GetByEvent(PPID eventID, RegisterArray * pList)
	{ return _Get(PPOBJ_PERSONEVENT, eventID, pList); }
int RegisterCore::GetByPerson(PPID personID, RegisterArray * pList)
	{ return _Get(PPOBJ_PERSON, personID, pList); }
int RegisterCore::GetByLocation(PPID locID, RegisterArray * pList)
	{ return _Get(PPOBJ_LOCATION, locID, pList); }
//
//
//
IMPLEMENT_PPFILT_FACTORY(Register); RegisterFilt::RegisterFilt() : PPBaseFilt(PPFILT_REGISTER, 0, 0)
{
	SetFlatChunk(offsetof(RegisterFilt, ReserveStart),
		offsetof(RegisterFilt, SerPattern) - offsetof(RegisterFilt, ReserveStart));
	SetBranchSString(offsetof(RegisterFilt, SerPattern));
	SetBranchSString(offsetof(RegisterFilt, NmbPattern));
	Init(1, 0);
}

bool RegisterFilt::IsEmpty() const
{
	return !(Oid.Id || ExclPersonID || ExclLocID || RegTypeID || SerPattern.NotEmpty() || NmbPattern.NotEmpty() || !RegPeriod.IsZero() || !ExpiryPeriod.IsZero());
}

/*static*/int FASTCALL RegisterCore::CheckRecForFilt(const RegisterTbl::Rec & rRec, const RegisterFilt * pFilt)
{
	int    ok = 1;
	if(pFilt) {
		if(pFilt->RegTypeID && rRec.RegTypeID != pFilt->RegTypeID)
			ok = 0;
		else if(pFilt->Oid.Obj != rRec.ObjType && (pFilt->Oid.Obj || rRec.ObjType != PPOBJ_PERSON))
			ok = 0;
		else if(pFilt->Oid.IsFullyDefined() && !pFilt->Oid.IsEq(rRec.ObjType, rRec.ObjID))
			ok = 0;
		else if(pFilt->ExclPersonID && rRec.ObjType == PPOBJ_PERSON && rRec.ObjID == pFilt->ExclPersonID)
			ok = 0;
		else if(pFilt->ExclLocID && rRec.ObjType == PPOBJ_LOCATION && rRec.ObjID == pFilt->ExclLocID)
			ok = 0;
		else if(!pFilt->ExpiryPeriod.CheckDate(rRec.Expiry))
			ok = 0;
		else if(!pFilt->RegPeriod.CheckDate(rRec.Dt))
			ok = 0;
		else if(pFilt->SerPattern.NotEmpty() && pFilt->SerPattern.CmpNC(rRec.Serial) != 0)
			ok = 0;
		else if(pFilt->NmbPattern.NotEmpty() && pFilt->NmbPattern.CmpNC(rRec.Num) != 0)
			ok = 0;
	}
	return ok;
}

int RegisterCore::SearchByObj(PPObjID oid, PPID regTypeID, RegisterTbl::Rec * pRec)
{
	int    ok = -1;
	RegisterTbl::Key1 k1;
	k1.ObjType = oid.Obj;
	k1.ObjID = oid.Id;
	if(search(1, &k1, spGe) && oid.IsEq(data.ObjType, data.ObjID)) do {
		if(data.RegTypeID == regTypeID) {
			CopyBufTo(pRec);
			ok = 1;
		}
	} while(ok < 0 && search(1, &k1, spNext) && oid.IsEq(data.ObjType, data.ObjID));
	return ok;
}

int RegisterCore::SearchByFilt(const RegisterFilt * pFilt, PPIDArray * pResList, PPIDArray * pObjList)
{
	int    c = 0, sp;
	DBQ  * dbq = 0;
	union {
		RegisterTbl::Key0 k0;
		RegisterTbl::Key1 k1;
		RegisterTbl::Key3 k3;
		RegisterTbl::Key4 k4;
	} k;
	int    idx = 0;
	MEMSZERO(k);
	if(pFilt->Oid.IsFullyDefined()) {
        RegisterArray list;
		_Get(pFilt->Oid.Obj, pFilt->Oid.Id, &list);
		for(uint i = 0; i < list.getCount(); i++) {
			const RegisterTbl::Rec & r_reg_rec = list.at(i);
			if(CheckRecForFilt(r_reg_rec, pFilt)) {
				CALLPTRMEMB(pResList, add(r_reg_rec.ID));
				CALLPTRMEMB(pObjList, add(r_reg_rec.ObjID));
				c++;
			}
		}
	}
	else if(pFilt->RegTypeID && pFilt->NmbPattern.NotEmpty()) {
		// RegTypeID, Serial, Num, UniqCntr (unique mod); // #3
		if(pFilt->SerPattern.NotEmpty()) {
			idx = 3;
			k.k3.RegTypeID = pFilt->RegTypeID;
			pFilt->SerPattern.CopyTo(k.k3.Serial, sizeof(k.k3.Serial));
			pFilt->NmbPattern.CopyTo(k.k3.Num, sizeof(k.k3.Num));
			if(search(idx, &k, spGe) && data.RegTypeID == pFilt->RegTypeID && pFilt->SerPattern == data.Serial && pFilt->NmbPattern == data.Num) do {
				if(CheckRecForFilt(data, pFilt)) {
					CALLPTRMEMB(pResList, add(data.ID));
					CALLPTRMEMB(pObjList, add(data.ObjID));
					c++;
				}
			} while(search(idx, &k, spNext) && data.RegTypeID == pFilt->RegTypeID && pFilt->SerPattern == data.Serial && pFilt->NmbPattern == data.Num);
		}
		else {
			idx = 4;
			k.k4.RegTypeID = pFilt->RegTypeID;
			pFilt->NmbPattern.CopyTo(k.k4.Num, sizeof(k.k4.Num));
			if(search(idx, &k, spGe) && data.RegTypeID == pFilt->RegTypeID && pFilt->NmbPattern == data.Num) do {
				if(CheckRecForFilt(data, pFilt)) {
					CALLPTRMEMB(pResList, add(data.ID));
					CALLPTRMEMB(pObjList, add(data.ObjID));
					c++;
				}
			} while(search(idx, &k, spNext) && data.RegTypeID == pFilt->RegTypeID && pFilt->NmbPattern == data.Num);
		}
	}
	else {
		BExtQuery q(this, pFilt->RegTypeID ? (pFilt->SerPattern.NotEmpty() ? 3 : 4) : 0);
		if(pFilt->RegTypeID) {
			dbq = & (*dbq && this->RegTypeID == pFilt->RegTypeID);
			if(pFilt->SerPattern.NotEmpty()) {
				k.k3.RegTypeID = pFilt->RegTypeID;
				pFilt->SerPattern.CopyTo(k.k3.Serial, sizeof(k.k3.Serial));
				dbq = & (*dbq && this->Serial == pFilt->SerPattern.cptr());
				if(pFilt->NmbPattern.NotEmpty()) {
					pFilt->NmbPattern.CopyTo(k.k3.Num, sizeof(k.k3.Num));
					dbq = & (*dbq && this->Num == pFilt->NmbPattern.cptr());
				}
			}
			else {
				k.k4.RegTypeID = pFilt->RegTypeID;
				if(pFilt->NmbPattern.NotEmpty()) {
					pFilt->NmbPattern.CopyTo(k.k4.Num, sizeof(k.k4.Num));
					dbq = & (*dbq && this->Num == pFilt->NmbPattern.cptr());
				}
			}
			sp = spGe;
		}
		else {
			sp = spFirst;
		}
		dbq = & (*dbq && daterange(this->Dt, &pFilt->RegPeriod) && daterange(this->Expiry, &pFilt->ExpiryPeriod));
		q.selectAll().where(*dbq);
		for(q.initIteration(false, &k, sp); q.nextIteration() > 0;) {
			if(CheckRecForFilt(data, pFilt)) {
				CALLPTRMEMB(pResList, add(data.ID));
				CALLPTRMEMB(pObjList, add(data.ObjID));
				c++;
			}
		}
	}
	return (c > 0) ? 1 : -1;
}
