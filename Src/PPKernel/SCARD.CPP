// SCARD.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2006, 2007, 2009, 2011, 2012, 2013, 2014, 2015, 2016
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
//
//
SCardCore::OpBlock::OpBlock()
{
	Init(0);
}

SCardCore::OpBlock & SCardCore::OpBlock::Init(SCardOpTbl::Rec * pRec)
{
	THISZERO();
	if(pRec) {
		SCardID = pRec->SCardID;
		Dtm.Set(pRec->Dt, pRec->Tm);
		LinkOi.Set(pRec->LinkObjType, pRec->LinkObjID);
		Amount = pRec->Amount;
		PrevRest = pRec->Rest - pRec->Amount;
		if(pRec->Flags & SCARDOPF_FREEZING) {
			Flags |= fFreezing;
			FreezingPeriod.Set(pRec->FreezingStart, pRec->FreezingEnd);
		}
	}
	return *this;
}

//static
int SCardCore::OpBlock::CheckFreezingPeriod(const DateRange & rPeriod, LDATE expiry)
{
	int    ok = 1;
	const  long pl = rPeriod.GetLength();
	if(pl < 0 || pl > (365*8+2) || (expiry && rPeriod.low > expiry)) {
        SString temp_buf;
		PPSetError(PPERR_INVSCOPFRZPRD, temp_buf.Cat(rPeriod));
		ok = 0;
	}
	return ok;
}

int SCardCore::OpBlock::CheckFreezingPeriod(LDATE expiry) const
{
	return CheckFreezingPeriod(FreezingPeriod, expiry);
}
//
//
//
SLAPI SCardCore::SCardCore() : SCardTbl()
{
}

int SLAPI SCardCore::Search(PPID id, SCardTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_SCARD, id, pRec);
}

int SLAPI SCardCore::SearchCode(PPID seriesID, const char * pCode, SCardTbl::Rec * pRec)
{
	int    ok = -1, idx = 1;
	union {
		SCardTbl::Key1 k1;
		SCardTbl::Key2 k2;
	} k;
	MEMSZERO(k);
	if(seriesID == 0 || DS.GetTLA().Cc.Flags & CCFLG_THROUGHSCARDUNIQ) {
		STRNSCPY(k.k1.Code, pCode);
		idx = 1;
		if(search(idx, &k, spGe) && stricmp866(data.Code, pCode) == 0) {
			copyBufTo(pRec);
			ok = 1;
		}
		else
			ok = (BTROKORNFOUND) ? (PPSetError(PPERR_SCARDNOTFOUND, pCode), -1) : PPSetErrorDB();
	}
	else {
		k.k2.SeriesID = seriesID;
		STRNSCPY(k.k2.Code, pCode);
		idx = 2;
		if(search(idx, &k, spEq)) {
			copyBufTo(pRec);
			ok = 1;
		}
		else
			ok = (BTROKORNFOUND) ? (PPSetError(PPERR_SCARDNOTFOUND, pCode), -1) : PPSetErrorDB();
	}
	return ok;
}

int SLAPI SCardCore::GetListByCode(const char * pCode, PPIDArray * pList)
{
	CALLPTRMEMB(pList, clear());
	int    ok = -1;
	uint   c = 0;
	SCardTbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.Code, pCode);
	if(search(1, &k1, spGe) && stricmp866(data.Code, pCode) == 0) do {
		CALLPTRMEMB(pList, add(data.ID));
		c++;
	} while(search(1, &k1, spNext) && stricmp866(data.Code, pCode) == 0);
	ok = BTROKORNFOUND ? (c ? (int)c : -1) : PPSetErrorDB();
	return ok;
}

int SLAPI SCardCore::GetListByPerson(PPID personID, PPID seriesID, PPIDArray * pList)
{
	int    ok = -1;
	if(personID) {
		SCardTbl::Key3 k3;
		MEMSZERO(k3);
		k3.PersonID = personID;
		if(search(3, &k3, spEq)) {
			do {
				if(!seriesID || data.SeriesID == seriesID) {
					CALLPTRMEMB(pList, addUnique(data.ID));
					ok = 1;
				}
			} while(search(3, &k3, spNext) && data.PersonID == personID);
		}
	}
	return ok;
}

int SLAPI SCardCore::GetListByLoc(PPID locID, PPID seriesID, PPIDArray * pList)
{
	int    ok = -1;
	if(locID) {
		SCardTbl::Key4 k4;
		MEMSZERO(k4);
		k4.LocID = locID;
		if(search(4, &k4, spEq)) {
			do {
				if(!seriesID || data.SeriesID == seriesID) {
					CALLPTRMEMB(pList, addUnique(data.ID));
					ok = 1;
				}
			} while(search(4, &k4, spNext) && data.LocID == locID);
		}
	}
	return ok;
}

int SLAPI SCardCore::UpdateDiscount(PPID id, double discount, int use_ta)
{
	int    ok = 1;
	if(id) {
		long   new_dis = (long)(R6(discount) * 100L);
		SCardTbl::Rec rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_SCARD, id, &rec) > 0 && rec.PDis != new_dis) {
			rec.PDis = new_dis;
			rec.Flags &= ~SCRDF_INHERITED;
			THROW_DB(updateRecBuf(&rec)); // @sfu
			DS.LogAction(PPACN_OBJUPD, PPOBJ_SCARD, id, 0, 0);
			DS.LogAction(PPACN_SCARDDISUPD, PPOBJ_SCARD, id, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::UpdateRest(PPID id, double rest, int use_ta)
{
	int    ok = 1;
	if(id) {
		SCardTbl::Rec rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID(this, PPOBJ_SCARD, id, &rec) > 0 && rec.Rest != rest) { // @v8.6.4 SearchByID_ForUpdate-->SearchByID
			rec.Rest = rest;
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(updateRecBuf(&rec)); // @sfu
			DS.LogAction(PPACN_OBJUPD, PPOBJ_SCARD, id, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::UpdateExpiryDelta(PPID id, long delta, int use_ta)
{
	int    ok = 1;
	if(id) {
		SCardTbl::Rec rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID(this, PPOBJ_SCARD, id, &rec) > 0) {
			if(checkdate(rec.Expiry, 0)) {
                rec.Expiry = plusdate(rec.Expiry, delta);
                THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(updateRecBuf(&rec)); // @sfu
				DS.LogAction(PPACN_OBJUPD, PPOBJ_SCARD, id, 0, 0);
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::Update(PPID id, SCardCore::Rec * pRec, int useTa)
{
	int    ok = 1;
	if(pRec && id) {
		int    activated = 0;
		SCardTbl::Rec org_rec;
		PPTransaction tra(useTa);
		THROW(tra);
		// @v8.1.8 THROW(UpdateByID(this, PPOBJ_SCARD, id, pRec, 0));
		// @v8.1.8 {
		{
			const long _act_f = SCRDF_NEEDACTIVATION|SCRDF_CLOSED;
			THROW(SearchByID_ForUpdate(this, PPOBJ_SCARD, id, &org_rec) > 0);
			if(((org_rec.Flags & _act_f) == _act_f) && !(pRec->Flags & _act_f))
				activated = 1;
			THROW_DB(updateRecBuf(pRec)); // @sfu
			if(activated)
				DS.LogAction(PPACN_SCARDACTIVATED, PPOBJ_SCARD, id, 0, 0);
		}
		// } @v8.1.8
		DS.LogAction(PPACN_OBJUPD, PPOBJ_SCARD, id, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::AddTurnover(PPID id, double add, int use_ta)
{
	int    ok = 1;
	if(id && add != 0.0) {
		SCardTbl::Rec rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_SCARD, id, &rec) > 0) {
			rec.Turnover += add;
			THROW_DB(updateRecBuf(&rec)); // @sfu
			// @v8.1.4 DS.LogAction(PPACN_SCARDTRNOVRUPD, PPOBJ_SCARD, id, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

#if 0 // @construction {

int SLAPI SCardCore::AddTurnover_Test(PPID id, double add, int use_ta)
{
	int    ok = 1;
	if(id)
		ok = updateFor(this, use_ta, (this->ID == id), set(this->Turnover, (this->Turnover+add))) ? 1 : PPSetErrorDB();
	else
		ok = -1;
	return ok;
}

#endif // } @construction

// static
int SLAPI SCardCore::GetCodeRange(const char * pTemplate, SString & rLow, SString & rUpp)
{
	int    ok = -1;
	rLow = 0;
	rUpp = 0;
	if(!isempty(pTemplate)) {
		int    add_check_dig = 0;
		double low = 0.0, upp = 0.0;
		char   pfx[32], t[32];
		char   templ[128];
		char * c = pfx;
		char * x;

		STRNSCPY(templ, pTemplate);
		uchar * p = (uchar *)strip(templ);
		memzero(pfx, sizeof(pfx));
		{
			size_t len = strlen((char *)p);
			if(p[len-1] == '^') {
				p[len-1] = 0;
				add_check_dig = 1;
			}
		}
		while(ok < 0 && *p) {
			if(isalnum(*p))
				*c++ = *p++;
			else if(*p == '%') {
				size_t r_len = 0;
				for(++p, x = t; isdigit(*p);)
					*x++ = *p++;
				*x = 0;
				r_len = atoi(t);
				if(*p == '[') {
					for(++p, x = t; *p && *p != ']';)
						*x++ = *p++;
					*x = 0;
					strtorrng(t, &low, &upp);
				}
				if(low <= 0)
					low = 1;
				if(upp <= 0 || upp > (fpow10i(r_len) - 1))
					upp = fpow10i(r_len) - 1;

				rLow.Printf("%s%0*ld", pfx, (int)r_len, (long)low);
				rUpp.Printf("%s%0*ld", pfx, (int)r_len, (long)upp);
				ok = 1;
				break;
			}
			else
				p++;
		}
	}
	return ok;
}

int SLAPI SCardCore::MakeCodeByTemplate(PPID seriesID, const char * pPattern, SString & rCode)
{
	rCode = 0;

	int    ok = -1;
	//
	int    add_check_dig = 0; // Признак необходимости добавить контрольную цифру
		// 0 - не добавлять
		// 1 - контрольная цифра рассчитывается по правилам для штрихкодов
		// 2 - контрольная цифра рассчитывается по алгоритму Луна
	//
	int    check_dig_len = -1;
	double low = 0.0, upp = 0.0;
	char   pfx[32], t[32], pttrn[32];
	char   templ[64];
	char * c = pfx;
	char * x, * p = 0;
	THROW_PP(pPattern, PPERR_INVPARAM);
	assert(pPattern);
	STRNSCPY(templ, pPattern);
	p = strip(templ);
	memzero(pfx, sizeof(pfx));
	{
		size_t len = strlen(p);
		if(oneof2(p[len-1], '^', '!')) {
			p[len-1] = 0;
			if(p[len-1] == '^')
				add_check_dig = 1;
			else // p[len-1] == '!'
				add_check_dig = 2;
			if(len > 2 && p[len-2] == ')') {
				uint i = len-3;
				check_dig_len = 0;
				while(i && p[i] != '(') {
					check_dig_len++;
					i--;
				}
				if(p[i] != '(') {
					//
					// Ошибка в шаблоне: закрывающая скобка без соответствующей открывающей
					//
					check_dig_len = -1;
				}
			}
		}
	}
	while(ok < 0 && *p) {
		if(isalnum(*p))
			*c++ = *p++;
		else if(*p == '%') {
			size_t r_len = 0;
			for(++p, x = t; isdigit(*p);)
				*x++ = *p++;
			*x = 0;
			r_len = atoi(t);
			if(*p == '[') {
				for(++p, x = t; *p && *p != ']';)
					*x++ = *p++;
				*x = 0;
				strtorrng(t, &low, &upp);
			}
			if(low <= 0)
				low = 1;
			if(upp <= 0 || upp > (fpow10i(r_len) - 1))
				upp = fpow10i(r_len) - 1;
			for(long n = (long)low; ok < 0 && n <= (long)upp; n++) {
				int    r;
				memzero(pttrn, sizeof(pttrn));
				sprintf(pttrn, "%s%0*ld", pfx, (int)r_len, n);
				if(add_check_dig) {
					char * p_cb = pttrn;
					if(check_dig_len > 0) {
						int    pl = (int)strlen(pttrn);
						if(check_dig_len <= pl)
							p_cb = pttrn+pl-check_dig_len;
					}
					if(add_check_dig == 1)
						AddBarcodeCheckDigit(p_cb);
					else if(add_check_dig == 2) {
						size_t cb_len = strlen(p_cb);
						int cd = SCalcCheckDigit(SCHKDIGALG_LUHN, p_cb, cb_len);
						if(isdec(cd)) {
							p_cb[cb_len++] = cd;
							p_cb[cb_len] = 0;
						}
					}
				}
				THROW(r = SearchCode(seriesID, pttrn, 0));
				if(r < 0) {
					rCode = pttrn;
					ok = 1;
				}
			}
		}
		else if(*p != '(' && *p != ')') // Скобки - служебные. Их пропускаем.
			p++;
	}
	CATCHZOK
	return ok;
}

struct SCardCodeTemplate {
	SCardCodeTemplate()
	{
		Reset();
	}
	void   Reset()
	{
		Prefix = 0;
		RLen = 0;
		R = 0;
		AddCheckDig = 0;
		CheckDigOffs = 0;
	}
	int    Parse(const char * pPattern)
	{
		int    ok = 1;
		SString templ = pPattern;
		SString temp_buf;
		uint    check_dig_start_pos = 0;
		double low = 0.0, upp = 0.0;
		Reset();
		if(templ.NotEmptyS()) {
			size_t len = templ.Len();
			const char last_c = templ.Last();
			if(last_c == '^' || last_c == '!') {
				if(last_c == '^')
					AddCheckDig = 1;
				else // last_c == '!'
					AddCheckDig = 2;
				templ.TrimRight();
				if(templ.Last() == ')' && templ.Len() > 2) {
					uint i = templ.Len()-1;
					while(i && templ[i] != '(') {
						i--;
					}
					if(templ[i] == '(') {
						check_dig_start_pos = i+1;
					}
					else {
						//
						// Ошибка в шаблоне: закрывающая скобка без соответствующей открывающей
						//
						check_dig_start_pos = 0;
					}
				}
			}
			{
				temp_buf = 0;
				for(uint i = 0; i < templ.Len(); i++) {
					const char c = templ.C(i);
					if(isalnum(c)) {
						temp_buf.CatChar(c);
						if(i < check_dig_start_pos)
							CheckDigOffs++;
					}
					else if(c == '%') {
						Prefix = temp_buf;
						temp_buf = 0;
						for(++i; isdigit(templ.C(i)); i++)
							temp_buf.CatChar(templ.C(i));
						RLen = (uint)temp_buf.ToLong();
						if(templ.C(i) == '[') {
							temp_buf = 0;
							for(++i; i < templ.Len() && templ.C(i) != ']'; i++)
								temp_buf.CatChar(templ.C(i));
							strtorrng(temp_buf, &low, &upp);
						}
						if(low <= 0.0)
							low = 1.0;
						if(upp <= 0.0 || upp > (fpow10i(RLen) - 1))
							upp = fpow10i(RLen) - 1;
						R.Set((int)low, (int)upp);
						break;
					}
				}
			}
		}
		else
			ok = -1;
		return ok;
	}
	int    Generate(long c, SString & rBuf) const
	{
		int    ok = -1;
		rBuf = 0;
		if(RLen && c >= R.low && c <= R.upp) {
			(rBuf = Prefix).CatLongZ(c, RLen);
			/*
			if(AddCheckDig == 1) {
				AddBarcodeCheckDigit(p_cb);
			}
			else if(AddCheckDig == 2) {
				size_t cb_len = strlen(p_cb);
				int cd = SCalcCheckDigit(SCHKDIGALG_LUHN, p_cb, cb_len);
				if(isdec(cd)) {
					p_cb[cb_len++] = cd;
					p_cb[cb_len] = 0;
				}
			}
			*/
			int    cd_alg = 0;
			if(AddCheckDig == 1) {
				cd_alg = SCHKDIGALG_BARCODE;
			}
			else if(AddCheckDig == 2) {
				cd_alg = SCHKDIGALG_LUHN;
			}
			if(cd_alg) {
				char   cdig = 0;
				if(CheckDigOffs > 0 && CheckDigOffs < rBuf.Len()) {
					cdig = SCalcCheckDigit(cd_alg, rBuf + CheckDigOffs, rBuf.Len()-CheckDigOffs);
					//cdig = CalcBarcodeCheckDigit(rBuf + CheckDigOffs);
				}
				else {
					cdig = SCalcCheckDigit(cd_alg, rBuf, rBuf.Len());
					//cdig = CalcBarcodeCheckDigit(rBuf);
				}
				if(isdec(cdig))
					rBuf.CatChar(cdig);
			}
			ok = 1;
		}
		return ok;
	}

	SString Prefix;
	uint   RLen;
	int    AddCheckDig;
	uint   CheckDigOffs;
	IntRange R;
};

int SLAPI SCardCore::AutoFill(const PPSCardSeries * pSerRec, const char * pPattern, int use_ta)
{
	int    ok = -1, r;
	SCardCodeTemplate templ;
	SString code;
	THROW(templ.Parse(NZOR(pPattern, pSerRec->CodeTempl)));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(long n = templ.R.low; templ.Generate(n, code) > 0; n++) {
			PPWaitMsg(code);
			THROW(PPCheckUserBreak());
			THROW(r = SearchCode(pSerRec->ID, code, 0));
			if(r < 0) {
				PPID   id = 0;
				SCardTbl::Rec rec;
				MEMSZERO(rec);
				rec.SeriesID = pSerRec->ID;
				code.CopyTo(rec.Code, sizeof(rec.Code));
				rec.PDis      = pSerRec->PDis;
				rec.MaxCredit = pSerRec->MaxCredit;
				rec.Dt        = pSerRec->Issue;
				rec.Expiry    = pSerRec->Expiry;
				rec.Flags    |= SCRDF_INHERITED;
				THROW(AddObjRecByID(this, PPOBJ_SCARD, &id, &rec, 0));
				DS.LogAction(PPACN_OBJADD, PPOBJ_SCARD, id, 0, 0);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

#if 0 // @obsolete {

int SLAPI SCardCore::AutoFill(const PPSCardSeries * pSerRec, const char * pPattern, int use_ta)
{
	int    ok = -1;
	int    add_check_dig = 0;
	int    check_dig_len = -1;
	double low = 0.0, upp = 0.0;
	char   pfx[32], t[32], pttrn[32];
	char   templ[48];
	char * c = pfx;
	char * x, * p = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPattern)
			STRNSCPY(templ, pPattern);
		else
			STRNSCPY(templ, pSerRec->CodeTempl);
		p = strip(templ);
		memzero(pfx, sizeof(pfx));
		{
			size_t len = strlen(p);
			if(p[len-1] == '^') {
				p[len-1] = 0;
				add_check_dig = 1;
				if(len > 2 && p[len-2] == ')') {
					uint i = len-3;
					check_dig_len = 0;
					SString temp_buf;
					while(i && p[i] != '(') {
						temp_buf.CatChar(p[i--]);
					}
					if(p[i] == '(') {
						temp_buf.Reverse();
						const char * tp = temp_buf;
						while(*tp) {
							if(isalnum(*tp))
								*c++ = *tp++;
							else if(*p == '%') {
							}
						}
					}
					else {
						//
						// Ошибка в шаблоне: закрывающая скобка без соответствующей открывающей
						//
						check_dig_len = -1;
					}
				}
			}
		}
		while(ok < 0 && *p) {
			if(isalnum(*p))
				*c++ = *p++;
			else if(*p == '%') {
				size_t r_len = 0;
				for(++p, x = t; isdigit(*p);)
					*x++ = *p++;
				*x = 0;
				r_len = atoi(t);
				if(*p == '[') {
					for(++p, x = t; *p && *p != ']';)
						*x++ = *p++;
					*x = 0;
					strtorrng(t, &low, &upp);
				}
				if(low <= 0)
					low = 1;
				if(upp <= 0 || upp > (fpow10i(r_len) - 1))
					upp = fpow10i(r_len) - 1;
				for(long n = (long)low; n <= (long)upp; n++) {
					int    r;
					memzero(pttrn, sizeof(pttrn));
					sprintf(pttrn, "%s%0*ld", pfx, (int)r_len, n);
					if(add_check_dig) {
						char * p_cb = pttrn;
						if(check_dig_len > 0) {
							int    pl = (int)strlen(pttrn);
							if(check_dig_len <= pl)
								p_cb = pttrn+pl-check_dig_len;
						}
						AddBarcodeCheckDigit(p_cb);
					}
					PPWaitMsg(pttrn);
					THROW(PPCheckUserBreak());
					THROW(r = SearchCode(pSerRec->ID, pttrn, 0));
					if(r < 0) {
						PPID   id = 0;
						SCardTbl::Rec rec;
						MEMSZERO(rec);
						rec.SeriesID = pSerRec->ID;
						STRNSCPY(rec.Code, pttrn);
						rec.PDis      = pSerRec->PDis;
						rec.MaxCredit = pSerRec->MaxCredit;
						rec.Dt        = pSerRec->Issue;
						rec.Expiry    = pSerRec->Expiry;
						rec.Flags    |= SCRDF_INHERITED;
						THROW(AddObjRecByID(this, PPOBJ_SCARD, &id, &rec, 0));
						DS.LogAction(PPACN_OBJADD, PPOBJ_SCARD, id, 0, 0);
						ok = 1;
					}
				}
			}
			else
				p++;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

#endif // } 0 @obsolete

int SLAPI SCardCore::SearchOp(PPID cardID, LDATE dt, LTIME tm, SCardOpTbl::Rec * pRec)
{
	SCardOpTbl::Key0 k0;
	k0.SCardID = cardID;
	k0.Dt = dt;
	k0.Tm = tm;
	return SearchByKey(&ScOp, 0, &k0, pRec);
}

int SLAPI SCardCore::SearchOpByLinkObj(PPID objType, PPID objID, SCardOpTbl::Rec * pRec)
{
	SCardOpTbl::Key1 k1;
	MEMSZERO(k1);
	k1.LinkObjType = PPOBJ_CCHECK;
	k1.LinkObjID = objID;
	if(ScOp.search(1, &k1, spGe) && ScOp.data.LinkObjType == objType && ScOp.data.LinkObjID == objID) {
		ScOp.copyBufTo(pRec);
		return 1;
	}
	else
		return PPDbSearchError();
}

int SLAPI SCardCore::SearchOpByCheck(PPID checkID, SCardOpTbl::Rec * pRec)
{
	return SearchOpByLinkObj(PPOBJ_CCHECK, checkID, pRec);
}

int SLAPI SCardCore::GetLastOpByCard(PPID cardID, LDATE dt, SCardOpTbl::Rec * pRec)
{
	SCardOpTbl::Key0 k0;
	k0.SCardID = cardID;
	k0.Dt = dt ? plusdate(dt, -1) : MAXDATE;
	k0.Tm = MAXTIME;
	if(ScOp.search(0, &k0, spLe) && k0.SCardID == cardID) {
		ScOp.copyBufTo(pRec);
		return 1;
	}
	else
		return PPDbSearchError();
}

int SLAPI SCardCore::EnumOpByCard(PPID cardID, LDATETIME * pDT, SCardOpTbl::Rec * pRec)
{
	SCardOpTbl::Key0 k0;
	k0.SCardID = cardID;
	k0.Dt = pDT->d;
	k0.Tm = pDT->t;
	if(ScOp.search(0, &k0, spGt) && k0.SCardID == cardID) {
		pDT->Set(k0.Dt, k0.Tm);
		ScOp.copyBufTo(pRec);
		return 1;
	}
	else
		return PPDbSearchError();
}

int SLAPI SCardCore::GetOpByLinkObj(PPObjID oid, TSArray <OpBlock> & rList)
{
	int    ok = -1;
	rList.clear();
	if(oid.Obj && oid.Id) {
		SCardOpTbl::Key1 k1;
		MEMSZERO(k1);
		k1.LinkObjType = oid.Obj;
		k1.LinkObjID = oid.Id;
		if(ScOp.search(1, &k1, spGe) && ScOp.data.LinkObjType == oid.Obj && ScOp.data.LinkObjID == oid.Id) {
			do {
				SCardOpTbl::Rec op_rec;
				ScOp.copyBufTo(&op_rec);
				OpBlock blk;
				blk.Init(&op_rec);
				blk.Flags |= OpBlock::fEdit;
				THROW_SL(rList.insert(&blk));
			} while(ScOp.search(1, &k1, spNext) && ScOp.data.LinkObjType == oid.Obj && ScOp.data.LinkObjID == oid.Id);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::GetFreezingOpList(PPID cardID, TSArray <OpBlock> & rList)
{
	int    ok = -1;
	rList.clear();
	SCardOpTbl::Key0 k0;
	MEMSZERO(k0);
	k0.SCardID = cardID;
	if(ScOp.search(0, &k0, spGe) && ScOp.data.SCardID == cardID) {
        BExtQuery q(&ScOp, 0);
		q.selectAll().where(ScOp.SCardID == cardID && ScOp.Flags > 0L);
		for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
			if(ScOp.data.Flags & SCARDOPF_FREEZING) {
				OpBlock blk;
				blk.Init(&ScOp.data);
				blk.Flags |= OpBlock::fEdit;
				THROW_SL(rList.insert(&blk));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::GetOp(PPID scardID, const LDATETIME & rDtm, OpBlock * pBlk)
{
	int    ok = -1;
	SCardOpTbl::Rec op_rec;
	if(SearchOp(scardID, rDtm.d, rDtm.t, &op_rec) > 0) {
		ok = 1;
		if(pBlk) {
			pBlk->Init(&op_rec);
			pBlk->Flags |= OpBlock::fEdit;
			if(op_rec.DestSCardID) {
				LDATETIME dtm;
				dtm = rDtm;
				if(op_rec.Amount < 0.0) {
					dtm.addhs(10);
					if(SearchOp(op_rec.DestSCardID, dtm.d, dtm.t, &op_rec) > 0) {
						pBlk->DestSCardID = op_rec.SCardID;
						pBlk->PrevDestRest = op_rec.Rest - op_rec.Amount;
					}
				}
				else if(op_rec.Amount > 0.0) {
					//
					// Поиск спаренной операции был начат с обратного конца (с начисления).
					// Инициализируем блок так, чтобы основные параметры соответствовали расходной операции.
					//
					dtm.addhs(-10);
					if(SearchOp(op_rec.DestSCardID, dtm.d, dtm.t, &op_rec) > 0) {
						pBlk->SCardID = op_rec.SCardID;
						pBlk->Dtm = dtm;
						pBlk->DestSCardID = scardID;
						pBlk->Amount = op_rec.Amount;
						pBlk->PrevDestRest = pBlk->PrevRest;
						pBlk->PrevRest = op_rec.Rest - op_rec.Amount;
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI SCardCore::PutOpByBill(PPID billID, PPID scardID, LDATE dt, double amount, int use_ta)
{
	int    ok = 1;
	if(billID) {
		LDATETIME dtm;
		SCardOpTbl::Rec rec;
		SCardOpTbl::Key1 k1;
		MEMSZERO(k1);
		k1.LinkObjType = PPOBJ_BILL;
		k1.LinkObjID = billID;
		dtm.SetZero();
		PPTransaction tra(use_ta);
		THROW(tra);
		if(ScOp.search(1, &k1, spGe) && ScOp.data.LinkObjType == PPOBJ_BILL && ScOp.data.LinkObjID == billID) {
			ScOp.copyBufTo(&rec);
			dtm.Set(rec.Dt, rec.Tm);
			if(rec.SCardID == scardID && rec.Amount == amount) {
				ok = 2;
			}
			else if(amount == 0.0 || rec.SCardID != scardID) {
				THROW(RemoveOp(rec.SCardID, rec.Dt, rec.Tm, 0));
			}
		}
		else {
			MEMSZERO(rec);
			rec.LinkObjType = PPOBJ_BILL;
			rec.LinkObjID = billID;
			rec.Dt = dt;
			rec.UserID = LConfig.User;
		}
		rec.SCardID = scardID;
		rec.Amount = amount;
		if(ok > 0 && amount != 0.0 && rec.SCardID != 0) {
			THROW(PutOpRec(&rec, 0));
			ok = 2;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::PutOpBlk(const OpBlock & rBlk, int use_ta)
{
	int    ok = 1;
	TSArray <OpBlock> ex_link_op_list;
	PPTransaction tra(use_ta);
	THROW(tra);
	/* @construction
	{
		THROW(GetOpByLinkObj(rBlk.LinkOi, ex_link_op_list));
		for(uint i = 0; i < ex_link_op_list.getCount(); i++) {

		}
	}
	*/
	if(!(rBlk.Flags & rBlk.fEdit)) {
		SCardOpTbl::Rec rec;
		MEMSZERO(rec);
		rec.SCardID = rBlk.SCardID;
		rec.Dt = rBlk.Dtm.d;
		rec.Tm = rBlk.Dtm.t;
		rec.LinkObjType = rBlk.LinkOi.Obj;
		rec.LinkObjID   = rBlk.LinkOi.Id;
		if(rBlk.DestSCardID) {
			rec.Amount = -fabs(rBlk.Amount);
			rec.DestSCardID = rBlk.DestSCardID;
		}
		else
			rec.Amount = rBlk.Amount;
		rec.UserID = LConfig.User;
		if(rBlk.Flags & rBlk.fFreezing) {
			rec.Flags |= SCARDOPF_FREEZING;
			THROW(rBlk.CheckFreezingPeriod(ZERODATE));
			rec.FreezingStart = rBlk.FreezingPeriod.low;
			rec.FreezingEnd = rBlk.FreezingPeriod.upp;
		}
		THROW(PutOpRec(&rec, 0));
		if(rBlk.DestSCardID) {
			MEMSZERO(rec);
			rec.SCardID = rBlk.DestSCardID;
			LDATETIME dtm = rBlk.Dtm;
			dtm.addhs(10);
			rec.Dt = dtm.d;
			rec.Tm = dtm.t;
			rec.LinkObjType = rBlk.LinkOi.Obj;
			rec.LinkObjID   = rBlk.LinkOi.Id;
			rec.Amount = fabs(rBlk.Amount);
			rec.UserID = LConfig.User;
			rec.DestSCardID = rBlk.SCardID;
			THROW(PutOpRec(&rec, 0));
			DS.LogAction(PPACN_SCARDOPMOV, PPOBJ_SCARD, rBlk.SCardID, rBlk.DestSCardID, 0); // @v8.0.0
		}
	}
	else {
		SCardOpTbl::Rec rec;
		MEMSZERO(rec);
		rec.SCardID = rBlk.SCardID;
		rec.Dt = rBlk.Dtm.d;
		rec.Tm = rBlk.Dtm.t;
		rec.LinkObjType = rBlk.LinkOi.Obj;
		rec.LinkObjID   = rBlk.LinkOi.Id;
		rec.Amount = rBlk.Amount;
		rec.UserID = LConfig.User;
		if(rBlk.Flags & rBlk.fFreezing) {
			rec.Flags |= SCARDOPF_FREEZING;
			THROW(rBlk.CheckFreezingPeriod(ZERODATE));
			rec.FreezingStart = rBlk.FreezingPeriod.low;
			rec.FreezingEnd = rBlk.FreezingPeriod.upp;
		}
		THROW(PutOpRec(&rec, 0));
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::PutOpRec(const SCardOpTbl::Rec * pRec, int use_ta)
{
	int    ok = 1, new_op = -1, r = -1;
	double rest;
	long   frz_delta = 0;
	SCardOpTbl::Rec exs_rec, rec = *pRec;
	SCardOpTbl::Key0 k0;
	SCardTbl::Rec sc_rec;
	THROW_PP(rec.SCardID, PPERR_SCARDIDNEEDEDFOROP);
	THROW(Search(rec.SCardID, &sc_rec)); // @v8.6.4
	if(rec.Dt == 0 || rec.Tm == 0)
		new_op = 1;
	if(rec.Dt == 0)
		rec.Dt = getcurdate_();
	if(rec.Tm == 0) {
		rec.Tm = getcurtime_();
		// @v7.7.12 {
		if(rec.Tm == 0 && (rec.LinkObjType && rec.LinkObjID)) {
			//
			// Особый случай: нулевое время. Если мы оставим его таким и к операции привязан
			// объект, то запись в индексе #1 {LinkObjType, LinkObjID, SCardID, Dt, Tm} таблицы SCardOp
			// не сформируется (индекс имеет свойство anysegnull).
			//
			rec.Tm = encodetime(0, 0, 0, 1);
		}
		// } @v7.7.12
	}
	if(rec.Flags & SCARDOPF_FREEZING) {
		rec.Amount = 0.0;
		DateRange frz_prd;
		frz_prd.Set(rec.FreezingStart, rec.FreezingEnd);
		THROW(OpBlock::CheckFreezingPeriod(frz_prd, sc_rec.Expiry));
		frz_delta = (frz_prd.GetLength()+1);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(new_op > 0 || (r = SearchOp(rec.SCardID, rec.Dt, rec.Tm, &exs_rec)) < 0) {
			THROW_DB(ScOp.insertRecBuf(&rec));
		}
		else {
			THROW(r);
            THROW_PP((exs_rec.Flags & SCARDOPF_FREEZING) == (rec.Flags & SCARDOPF_FREEZING), PPERR_ILLUPDSCARDOPSTATUSFRZ);
            if(exs_rec.Flags & SCARDOPF_FREEZING) {
				DateRange frz_prd;
				frz_prd.Set(exs_rec.FreezingStart, exs_rec.FreezingEnd);
				if(OpBlock::CheckFreezingPeriod(frz_prd, ZERODATE)) {
                    frz_delta -= (frz_prd.GetLength()+1);
				}
            }
			THROW_DB(ScOp.updateRecBuf(&rec));
		}
		if(rec.Flags & SCARDOPF_FREEZING) {
			if(frz_delta)
				THROW(UpdateExpiryDelta(rec.SCardID, frz_delta, 0));
		}
		else {
			k0.SCardID = rec.SCardID;
			k0.Dt = rec.Dt;
			k0.Tm = rec.Tm;
			if(ScOp.search(0, &k0, spLt) && k0.SCardID == rec.SCardID) {
				THROW(RecalcForwardRests(rec.SCardID, k0.Dt, k0.Tm, &rest, 0));
			}
			else {
				THROW(RecalcRests(rec.SCardID, &rest, 0));
			}
			THROW(UpdateRest(rec.SCardID, rest, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::RemoveOp(PPID cardID, LDATE dt, LTIME tm, int use_ta)
{
	int    ok = 1;
	double rest = 0.0;
	SCardOpTbl::Rec org_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchOp(cardID, dt, tm, &org_rec) > 0) {
			THROW_DB(ScOp.deleteRec());
			if(org_rec.Flags & SCARDOPF_FREEZING) {
				DateRange frz_prd;
				frz_prd.Set(org_rec.FreezingStart, org_rec.FreezingEnd);
				if(OpBlock::CheckFreezingPeriod(frz_prd, ZERODATE)) {
					long   delta = -(frz_prd.GetLength()+1);
					THROW(UpdateExpiryDelta(cardID, delta, 0));
				}
			}
			else {
				THROW(RecalcForwardRests(cardID, dt, tm, &rest, 0));
				THROW(UpdateRest(cardID, rest, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::RemoveOpAll(PPID cardID, int use_ta)
{
	int    ok = 1;
	TSArray <LDATETIME> list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			//
			// Что бы можно было использовать примитивную функцию RemoveOp и не "ломать"
			// позиции по индексу, сначала соберем все операции в список list а уже
			// потом по этому списку все удалим.
			// Важно: операции в список складываются в обратном хронологическому порядке
			// для того, чтобы избежать пересчета операций, которые все равно удаляются.
			//
			SCardOpTbl::Key0 k0;
			k0.SCardID = cardID;
			k0.Dt = MAXLONG;
			k0.Tm = MAXLONG;
			if(ScOp.search(0, &k0, spLast) && ScOp.data.SCardID == cardID) do {
				LDATETIME dtm;
				dtm.Set(ScOp.data.Dt, ScOp.data.Tm);
				list.insert(&dtm);
			} while(ScOp.search(0, &k0, spPrev) && ScOp.data.SCardID == cardID);
		}
		for(uint i = 0; i < list.getCount(); i++) {
			const LDATETIME dtm = list.at(i);
			THROW(RemoveOp(cardID, dtm.d, dtm.t, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::RemoveOpByCheck(PPID checkID, int use_ta)
{
	int    ok = 1;
	double rest = 0.0;
	SCardOpTbl::Rec scop_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		while(SearchOpByCheck(checkID, &scop_rec) > 0) {
			THROW_DB(ScOp.deleteRec());
			THROW(RecalcForwardRests(scop_rec.SCardID, scop_rec.Dt, scop_rec.Tm, &rest, 0));
			THROW(UpdateRest(scop_rec.SCardID, rest, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::RecalcForwardRests(PPID cardID, LDATE dt, LTIME tm, double * pRest, int use_ta)
{
	int    ok = 1;
	double rest = 0.0;
	SCardOpTbl::Key0 k0;
	{
		int    last_op_found = 0;
		PPTransaction tra(use_ta);
		THROW(tra);
		k0.SCardID = cardID;
		k0.Dt = dt;
		k0.Tm = tm;
		/* @v8.6.11
		if(ScOp.search(0, &k0, spLe) && k0.SCardID == cardID)
			rest = ScOp.data.Rest;
		else {
			k0.SCardID = cardID;
			k0.Dt = ZERODATE;
			k0.Tm = ZEROTIME;
			rest = 0.0;
		}
		*/
		// @v8.6.11 {
		if(ScOp.search(0, &k0, spLe) && ScOp.data.SCardID == cardID) do {
			if(!(ScOp.data.Flags & SCARDOPF_FREEZING)) {
				rest = ScOp.data.Rest;
				last_op_found = 1;
				break;
			}
		} while(ScOp.search(0, &k0, spPrev) && ScOp.data.SCardID == cardID);
		if(!last_op_found) {
			k0.SCardID = cardID;
			k0.Dt = ZERODATE;
			k0.Tm = ZEROTIME;
			rest = 0.0;
		}
		// } @v8.6.11
		while(ScOp.searchForUpdate(0, &k0, spGt) && k0.SCardID == cardID) {
			if(!(ScOp.data.Flags & SCARDOPF_FREEZING))
				rest += ScOp.data.Amount;
			ScOp.data.Rest = rest;
			THROW_DB(ScOp.updateRec()); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pRest, rest);
	return ok;
}

int SLAPI SCardCore::RecalcRests(PPID cardID, double * pRest, int use_ta)
{
	return RecalcForwardRests(cardID, ZERODATE, ZEROTIME, pRest, use_ta);
}

int SLAPI SCardCore::RecalcRestsBySeries(PPID serID, int use_ta)
{
	int    ok = 1;
	SCardTbl::Key2 k2;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k2);
		k2.SeriesID = serID;
		while(search(2, &k2, spGt) && k2.SeriesID == serID) {
			double rest = 0.0;
			THROW(RecalcRests(data.ID, &rest, 0));
			THROW(UpdateRest(data.ID, rest, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardCore::GetRest(PPID id, LDATE dt, double * pRest)
{
	int    ok = 1;
	double rest = 0.0;
	SCardOpTbl::Key0 k0;
	k0.SCardID = id;
	k0.Dt = NZOR(dt, MAXDATE);
	k0.Tm = MAXTIME;
	// @v8.6.8 double rest = (ScOp.search(0, &k0, spLe) && k0.SCardID == id) ? ScOp.data.Rest : 0.0;
	// @v8.6.8 {
	if(ScOp.search(0, &k0, spLe) && ScOp.data.SCardID == id) do {
		if(!(ScOp.data.Flags & SCARDOPF_FREEZING)) {
			rest = ScOp.data.Rest;
			break;
		}
	} while(ScOp.search(0, &k0, spPrev) && ScOp.data.SCardID == id);
	// } @v8.6.8
	ASSIGN_PTR(pRest, rest);
	return ok;
}

struct ScCodeByLenEntry {
    uint   Len;
    StrAssocArray Codes;
};

int SLAPI SCardCore::GetPrefixRanges(PPID seriesID, uint maxLen, TSCollection <PrefixRange> & rRanges)
{
	rRanges.freeAll();
	int    ok = 1;
	SString temp_buf;
	SString lo, up;
	SCardTbl::Key2 k2;

	TSCollection <ScCodeByLenEntry> list;

	DBQ  * dbq = 0;
	MEMSZERO(k2);
	k2.SeriesID = seriesID;
	BExtQuery q(this, 2);
	dbq = ppcheckfiltid(dbq, this->SeriesID, seriesID);
	q.select(this->ID, this->Code, 0L).where(*dbq);
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		const PPID sc_id = data.ID;
		temp_buf = data.Code;
        const uint len = temp_buf.Len();
        if(len) {
        	int    _found = 0;
            for(uint i = 0; !_found && i < list.getCount(); i++) {
				ScCodeByLenEntry * p_entry = list.at(i);
				if(p_entry && p_entry->Len == len) {
                    THROW_SL(p_entry->Codes.AddFast(sc_id, temp_buf));
					_found = 1;
				}
            }
            if(!_found) {
				ScCodeByLenEntry * p_new_entry = list.CreateNewItem(0);
				THROW_SL(p_new_entry);
				p_new_entry->Len = len;
				THROW_SL(p_new_entry->Codes.AddFast(sc_id, temp_buf));
            }
        }
	}
	{
		for(uint i = 0; i < list.getCount(); i++) {
			ScCodeByLenEntry * p_entry = list.at(i);
			if(p_entry) {
				const uint ec = p_entry->Codes.getCount();
				if(ec) {
					p_entry->Codes.SortByText();
					PrefixRange * p_new_range = rRanges.CreateNewItem(0);
					THROW_SL(p_new_range);
					{
						p_new_range->Len = p_entry->Len;
						lo = p_entry->Codes.at(0).Txt;
						assert(lo.Len() == p_entry->Len);
						up = p_entry->Codes.at(ec-1).Txt;
						assert(up.Len() == p_entry->Len);
						p_new_range->Low = lo.Trim(maxLen);
						p_new_range->Upp = up.Trim(maxLen);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
