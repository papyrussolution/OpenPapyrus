// PPEXPR.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
// @Kernel
//
// Трансляция формул
//
#include <pp.h>
#pragma hdrstop

void SLAPI ExprEvalContext::FC::Init()
{
	ArgList.clear();
	StrArg = 0;
	RetType = MKSTYPE(S_FLOAT, 8);
	RetReal = 0.0;
	RetStr = 0;
}

SLAPI ExprEvalContext::ExprEvalContext()
{
	ImplementFlags = 0;
	P_Next = 0;
}

SLAPI ExprEvalContext::~ExprEvalContext()
{
}

int SLAPI ExprEvalContext::SetInnerContext(ExprEvalContext * pInner)
{
	int    ok = -1;
	if(pInner) {
		int    done = 0;
		ExprEvalContext * p_last = this;
		if(p_last != pInner) {
			while(!done && p_last && p_last->P_Next) {
				p_last = p_last->P_Next;
				if(p_last == pInner)
					done = 1;
			}
		}
		else
			done = 1;
		assert(p_last != 0 && p_last->P_Next == 0);
		if(!done) {
			p_last->P_Next = pInner;
			ok = 1;
		}
	}
	else {
		P_Next = 0;
		ok = 2;
	}
	return ok;
}

int SLAPI ExprEvalContext::SelfScanArgList() const
{
	return BIN(ImplementFlags & fSelfScanArgList);
}

int SLAPI ExprEvalContext::IsFunc(const char * pSymb, int * pFuncId)
{
	return 0;
}

int SLAPI ExprEvalContext::ResolveFunc(int funcId, FC & rFc)
{
	return 0;
}

//static
int SLAPI PPExprParser::CalcExpression(const char * pStr, double * pResult, const PPCalcFuncList * pCFL, ExprEvalContext * pCtx)
{
	PPExprParser parser(pStr, pCFL, pCtx);
	return parser.CalcExpr(pResult);
}
//
// BillContext
//
SLAPI BillSymbCache::BillSymbCache() : ObjCache(PPOBJ_BILLSYMB, sizeof(Data))
{
	sDELIM = 0x0000005CL;
	FormSet.setDelim((char*)&sDELIM);
}

int SLAPI DirtyBillSymbCache(const char * pSymb)
{
	if(pSymb) {
		BillSymbCache * p_cache = GetDbLocalCachePtr <BillSymbCache> (PPOBJ_BILLSYMB);
		if(p_cache) {
			p_cache->DirtySymb(pSymb);
			return 1;
		}
	}
	return -1;
}

int SLAPI BillSymbCache::Add(int F, PPID id, const char * symb, const char * form)
{
	int    ok = 1;
	uint   pos;
	Data   entry;
	MEMSZERO(entry);
	entry.ID = id;
	entry.F = F;
	THROW_SL(SymbSet.add(symb, &(pos = 0)));
	THROW_SL(FormSet.add(form, &(pos = 0)));
	THROW_SL(P_Ary->insert(&entry));
	CATCHZOK
	return ok;
}

int SLAPI BillSymbCache::Search(const char * pSymb, PPID * pID, SString & rBuf) const
{
	int    ok = 0;
	SString s, f;
	rBuf = 0;
	for(uint n = 0, pos = 0; !ok && SymbSet.get(&pos, s); n++)
		if(stricmp866(s, pSymb) == 0) {
			Data * p_entry = (Data *)SearchByPos(n, 0);
			if(p_entry->F & 0x0001) {
				*pID = p_entry->ID;
				ok = 1;
			}
			else if(p_entry->F & 0x0002) {
				uint j = 0;
				for(pos = 0; FormSet.get(&pos, f) && j < n; j++);
				rBuf = f;
				*pID = p_entry->ID;
				ok = 2;
			}
			else if(p_entry->F & 0x0008) {
				*pID = p_entry->ID;
				ok = 3;
			}
		}
	return ok;
}

int SLAPI BillSymbCache::DirtySymb(const char * pSymb)
{
	int    ok = 0;
	SString s;
	RwL.WriteLock(); // {
	for(uint n = 0, pos = 0; !ok && SymbSet.get(&pos, s); n++) {
		if(stricmp866(s, pSymb) == 0) {
			Data * p_entry = (Data *)SearchByPos(n, 0);
			if(p_entry->F)
				p_entry->F = 0;
		}
	}
	RwL.Unlock(); // }
	return ok;
}

int SLAPI BillSymbCache::Fetch(const char * pSymb, PPID * pID, SString & rBuf)
{
	int    ok = 0;
	int    r;
	Data   entry;
	SString s, f;
	PPErrCode = 0;
	rBuf = 0;
	RwL.ReadLock();
	StatData.Count.Incr();
	ok = Search(pSymb, pID, rBuf);
	if(ok)
		StatData.Hits.Incr();
	else
		StatData.Misses.Incr();
	if(!ok) {
		RwL.Unlock();
		RwL.WriteLock();
		PPID   id = 0;
		PPObjAmountType at_obj;
		PPObjFormula    f_obj;
		if((r = at_obj.SearchSymb(&id, pSymb)) > 0) {
			f = 0;
			s = pSymb;
			if(Add(0x0001, id, s.Strip(), f.Strip())) {
				*pID = id;
				ok = 1;
			}
			else
				ok = 0;
		}
		else if(r && (r = f_obj.SearchByName(pSymb, &id)) > 0) {
			if(f_obj.Get(id, s, f) > 0) {
				if(Add(0x0002, id, s.Strip(), f.Strip())) {
					*pID = id;
					rBuf = f;
					ok = 2;
				}
				else
					ok = 0;
			}
		}
		else {
			PPSymbTranslator st;
			if(!st)
				ok = 0;
			else {
				size_t next_pos = 0;
				id = st.Translate(pSymb, &next_pos);
				if(id == PPSYM_ADVLNAMT) {
					f = 0;
					s = pSymb;
					if(Add(0x0008, PPAMT_ADLN_MAIN, s.Strip(), f.Strip())) {
						*pID = PPAMT_ADLN_MAIN;
						ok = 3;
					}
					else
						ok = 0;
   				}
				if(!r)
					ok = 0;
			}
		}
	}
	RwL.Unlock();
	if(!ok && !PPErrCode)
		PPSetError(PPERR_ILLVAR, pSymb);
	return ok;
}
//
//
//
#define CURR_BILL      0
#define LINK_BILL      1
#define RCKN_BILL      2

#define ARTSYMB_QTTY   1
#define ARTSYMB_PHQTTY 2

SLAPI BillContext::BillContext(const PPBillPacket * p, PPID curID, uint advLineIdx) : ExprEvalContext()
{
	P_Pack = p;
	CurID = curID;
	LinkID = P_Pack ? P_Pack->Rec.LinkBillID : 0;
	ReckonID = P_Pack ? P_Pack->PaymBillID : 0;
	AdvLineIdx = advLineIdx;
}

int SLAPI BillContext::IsFunc(const char * pSymb, int * pFuncId)
{
	// amountbyvat(x)
	int    func_id = 0;
	SString temp_buf;
	SStrScan scan(pSymb);
	if(scan.Skip().GetIdent(temp_buf)) {
		temp_buf.ToLower();
		if(temp_buf == "amountbyvat")
			func_id = funcAmountByVat;
		else if(temp_buf == "amountbygvat")
			func_id = funcAmountByGVat;
		else if(temp_buf == "amountbytvat")
			func_id = funcAmountByTVat;
		else if(temp_buf == "costbyvat")
			func_id = funcCostByVat;
		else if(temp_buf == "costvat")
			func_id = funcCostVat;
		else if(temp_buf == "pricebyvat")
			func_id = funcPriceByVat;
		else if(temp_buf == "pricevat")
			func_id = funcPriceVat;
		else
			PPSetError(PPERR_UNDEFGOODSFUNC, pSymb);
	}
	ASSIGN_PTR(pFuncId, func_id);
	return BIN(func_id);
}

int SLAPI BillContext::ResolveFunc(int funcId, FC & rFc)
{
	int    ok = 1;
	double result = 0.0;
	double arg = rFc.ArgList.getCount() ? rFc.ArgList.at(0) : 0.0;
	BillVatEntry * p_vat_entry = 0;
	switch(funcId) {
		case funcAmountByVat:
			CalcTotalBlock();
			p_vat_entry = Bt.VatList.GetByRate(arg);
			if(p_vat_entry)
				result = p_vat_entry->AmountByVat;
			break;
		case funcCostByVat:
			CalcTotalBlock();
			p_vat_entry = Bt.CostVatList.GetByRate(arg);
			if(p_vat_entry)
				result = p_vat_entry->AmountByVat;
			break;
		case funcCostVat:
			CalcTotalBlock();
			p_vat_entry = Bt.CostVatList.GetByRate(arg);
			if(p_vat_entry)
				result = p_vat_entry->VatSum;
			break;
		case funcPriceByVat:
			CalcTotalBlock();
			p_vat_entry = Bt.PriceVatList.GetByRate(arg);
			if(p_vat_entry)
				result = p_vat_entry->AmountByVat;
			break;
		case funcPriceVat:
			CalcTotalBlock();
			p_vat_entry = Bt.PriceVatList.GetByRate(arg);
			if(p_vat_entry)
				result = p_vat_entry->VatSum;
			break;
		case funcAmountByGVat:
			//
			// Для этого расчета нельзя воспользоваться общим блоком this->Bt
			// из-за специального флага BTC_CALCNOMINALGVAT.
			//
			{
				BillTotalData local_bt;
				((PPBillPacket *)P_Pack)->CalcTotal(&local_bt, BTC_CALCSALESTAXES|BTC_CALCNOMINALGVAT); // @attention cast const-->nonconst
				BillVatEntry * p_vat_entry = local_bt.VatList.GetByRate(arg);
				if(p_vat_entry)
					result = p_vat_entry->AmountByVat;
			}
			break;
		case funcAmountByTVat:
			{
				PPObjID objid;
				objid = arg;
				double arg2 = (rFc.ArgList.getCount() > 1) ? rFc.ArgList.at(1) : 0.0;
				THROW_PP(objid.Obj == PPOBJ_GOODSTYPE, PPERR_OBJEXPECTED_GOODSTYPE);
				BillTotalData local_bt;
				((PPBillPacket *)P_Pack)->CalcTotal(&local_bt, objid.Id, BTC_CALCSALESTAXES); // @attention cast const-->nonconst
				BillVatEntry * p_vat_entry = local_bt.VatList.GetByRate(arg2);
				if(p_vat_entry)
					result = p_vat_entry->AmountByVat;
			}
			break;
	}
	CATCHZOK
	rFc.RetType = T_DOUBLE;
	rFc.RetReal = result;
	return ok;
}

int SLAPI BillContext::CalcTotalBlock()
{
	int    ok = -1;
	if(!(Bt.Flags & BillTotalData::fInitialized) && P_Pack) {
		((PPBillPacket *)P_Pack)->CalcTotal(&Bt, BTC_CALCSALESTAXES); // @attention cast const-->nonconst
		ok = 1;
	}
	return ok;
}

int SLAPI BillContext::ProcessLink(PPID linkBillID, PPID amtID, int artefactSymb, PPID addCurID, int errOnDefault, double * pResult)
{
	int    ok = 1;
	double result = 0.0;
	if(linkBillID) {
		PPObjBill * p_bobj = BillObj;
		if(artefactSymb) {
			BillTotalData btd;
			PPBillPacket link_pack;
			THROW(p_bobj->ExtractPacket(linkBillID, &link_pack) > 0); // @v9.4.3 @fix(>0)
			link_pack.CalcTotal(&btd, 0);
			if(artefactSymb == ARTSYMB_QTTY)
				result = btd.UnitsCount;
			else if(artefactSymb == ARTSYMB_PHQTTY)
				result = btd.PhUnitsCount;
		}
		else {
			THROW_PP((p_bobj->P_Tbl->GetAmount(linkBillID, amtID, CurID, &result) > 0 || !errOnDefault), PPERR_AMTNOTPRESENT);
			if(addCurID >= 0) {
				double add_amt = 0.0;
				THROW_PP((p_bobj->P_Tbl->GetAmount(linkBillID, amtID, addCurID, &add_amt) > 0 || !errOnDefault), PPERR_AMTNOTPRESENT);
				result += add_amt;
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pResult, result);
	return ok;
}

int SLAPI BillContext::Resolve(const char * b, double * pResult)
{
	int    ok = 1, r, sel = CURR_BILL;
	int    err_on_default = 0;
	int    artefact_symb = 0; // ahtoxa
	PPID   id = 0;
	double val = 0.0;
	char   sel_buf[64];
	SString form;
	char * p_delim = (char *)strchr(b, '.');
	*pResult = 0;
	if(p_delim) {
		long   sLINK[2], sRCKN[2];
		sLINK[0] = 0x4B4E494CL; // "LINK"
		sLINK[1] = 0;
		sRCKN[0] = 0x4E4B4352L; // "RCKN"
		sRCKN[1] = 0;
		*p_delim = 0;
		STRNSCPY(sel_buf, b);
		if(stricmp(b, (char*)&sLINK) == 0)
			sel = LINK_BILL;
		else if(stricmp(b, (char*)&sRCKN) == 0)
			sel = RCKN_BILL;
		else {
			CALLEXCEPT_PP_S(PPERR_ILLVAR, b);
		}
		b   = p_delim+1;
		*p_delim = '.';
	}
	if(sstreqi_ascii(b, "QTTY")) {
		artefact_symb = ARTSYMB_QTTY;
		r = 1;
	}
	else if(sstreqi_ascii(b, "PHQTTY")) {
		artefact_symb = ARTSYMB_PHQTTY;
		r = 1;
	}
	else {
		BillSymbCache * p_cache = GetDbLocalCachePtr <BillSymbCache> (PPOBJ_BILLSYMB);
		THROW((r = p_cache->Fetch(b, &id, form)) > 0);
	}
	if(r == 1) {
		PPID   add_cur_id = -1;
		double add_amt = 0.0;
		const  PPID base_cur_id = LConfig.BaseCurID;
		if(CurID == 0) {
			if(base_cur_id)
				add_cur_id = base_cur_id;
		}
		else if(CurID == base_cur_id)
			add_cur_id = 0L;
		if(sel == CURR_BILL) {
			// ahtoxa {
			if(artefact_symb) {
				CalcTotalBlock();
				if(artefact_symb == ARTSYMB_QTTY)
					val = Bt.UnitsCount;
				else if(artefact_symb == ARTSYMB_PHQTTY)
					val = Bt.PhUnitsCount;
			}
			// } ahtoxa
			else {
				THROW_PP(((r = P_Pack->Amounts.Get(id, CurID, &val)) > 0 || !err_on_default), PPERR_AMTNOTPRESENT);
				if(r < 0 && id == PPAMT_MAIN && CurID == P_Pack->Rec.CurID)
					val = P_Pack->GetAmount();
				if(add_cur_id >= 0) {
					THROW_PP(((r = P_Pack->Amounts.Get(id, add_cur_id, &add_amt)) > 0 || !err_on_default), PPERR_AMTNOTPRESENT);
					if(r < 0 && id == PPAMT_MAIN && add_cur_id == P_Pack->Rec.CurID)
						add_amt = P_Pack->GetAmount();
					val += add_amt;
				}
			}
		}
		else if(sel == LINK_BILL) {
			THROW_PP(LinkID, PPERR_ILLVAR);
			THROW(ProcessLink(LinkID, id, artefact_symb, add_cur_id, err_on_default, &val));
		}
		else if(sel == RCKN_BILL) {
			if(ReckonID) {
				THROW(ProcessLink(ReckonID, id, artefact_symb, add_cur_id, err_on_default, &val));
			}
			else
				val = 0.0;
		}
	}
	else if(r == 3) {
		PPAdvBillItem & r_adv_line = P_Pack->AdvList.Get(AdvLineIdx);
		if((&r_adv_line) != 0) {
			if(id == PPAMT_ADLN_MAIN)
				val = r_adv_line.Amount;
		}
	}
	else
		THROW(PPExprParser::CalcExpression(form, &val, 0, this));
	*pResult = val;
	CATCH
		*pResult = 0;
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
PPExprParser::State::State()
{
	OidObjType = 0;
	OidObjTypeExt = 0;
}

PPExprParser::State & FASTCALL PPExprParser::State::operator = (const PPExprParser::State & rS)
{
	OidObjType = rS.OidObjType;
	OidObjTypeExt = rS.OidObjTypeExt;
	return *this;
}

SLAPI PPExprParser::PPExprParser(const char * pStr, const PPCalcFuncList * pCFL, ExprEvalContext * pContext)
{
	Token   = 0;
	Number  = 0.0;
	P_S     = newStr(pStr);
	Pos     = 0;
	P_Ctx   = pContext;
	P_CFL   = pCFL;
	Next();
}

SLAPI PPExprParser::~PPExprParser()
{
	delete P_S;
}

int SLAPI PPExprParser::Next()
{
	return P_S[Pos] ? (Cur = (int)P_S[Pos++]) : (Cur = -1);
}

// sieve[1.; 20000. ? 2.5; 40000 ? 4]
// sieve[1.; @pp:alc_price_limit]

int SLAPI PPExprParser::GetSieve()
{
	int    ok = 1;
	SString temp_buf, sub;
	Sieve.Setup();
	SieveVal.freeAll();
	while(oneof2(Cur, ' ', '\t'))
		Next();
	THROW_PP_S(Cur == '[', PPERR_INVEXPR, P_S);
	while(Next() != ']') {
		THROW_PP_S(Cur != -1, PPERR_INVEXPR, P_S);
		temp_buf.CatChar(Cur);
	}
	if(ok) {
		Next(); // Забрать символ ']' из потока
		SString sub, left, right;
		StringSet ss(';', temp_buf.Strip());
		long   id = 0;
		int    was_default = 0;
		for(uint pos = 0; ss.get(&pos, sub);) {
			if(sub.Strip()[0] == '@') {
				//
				// Загрузка таблицы из файла
				//
				sub.ShiftLeft(1);
				if(sub.Divide(':', left, right) <= 0) {
					right = left;
					left = "pp";
				}
				SString file_name;
				PPGetFilePath(PPPATH_DD, left, file_name);
				SPathStruc::ReplaceExt(file_name, "tab", 0);
				STab tab;
				STab::Row row;
				STabFile f(file_name, 0);
				THROW_SL(f.IsValid());
				THROW_SL(f.LoadTab(right.Strip(), tab) > 0);
				for(uint i = 0; i < tab.GetCount(); i++) {
					double bin = 0.0;
					double val = 0.0;
					tab.GetRow(i, row);
					if(row.Get(0, bin)) {
						++id;
						if(row.Get(1, val)) {
							Sieve.AddBin(id, bin);
							SieveVal.Add(id, val, 0);
						}
						else {
							THROW_PP(was_default == 0, PPERR_SIEVEFUNC_DUPDEF);
							SieveVal.Add(-MAXLONG, bin, 0);
							was_default = 1;
						}
					}
				}
			}
			else {
				++id;
				if(sub.Divide('?', left, right) > 0) {
					Sieve.AddBin(id, left.ToReal());
					SieveVal.Add(id, right.ToReal(), 0);
				}
				else {
					THROW_PP(was_default == 0, PPERR_SIEVEFUNC_DUPDEF);
					SieveVal.Add(-MAXLONG, left.ToReal(), 0);
					was_default = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

void SLAPI PPExprParser::Skip()
{
	while(oneof4(Cur, ' ', '\t', '\r', '\n'))
		Next();
}

int SLAPI PPExprParser::ResolveContextName(const char * pSymb, double * pResult)
{
	int    ok = 0;
	for(ExprEvalContext * p_ctx = P_Ctx; !ok && p_ctx; p_ctx = p_ctx->P_Next) {
		if(p_ctx->Resolve(pSymb, pResult))
			ok = 1;
	}
	return ok;
}

int SLAPI PPExprParser::GetToken()
{
	char   buf[256];
	char * p = buf;
	Token = -1;
	if(Cur > 0) {
		size_t offs = 0;
		Skip();
		if(Cur == '[') {
			do {
				if(!oneof4(Cur, ' ', '\t', '\r', '\n'))
					*p++ = Cur;
				Next();
			} while(Cur != 0 && Cur != ']');
			if(Cur == ']') {
				*p++ = ']';
				Next();
			}
			*p = 0;
			Func = 0;
			Name = buf;
			Token = tokName;
		}
		else if(isalpha(Cur) || oneof3(Cur, '_', '@', '?')) {
			do {
				*p++ = Cur;
				Next();
			} while(isalnum(Cur) || oneof2(Cur, '_', '.'));
			*p = 0;
			Func = 0;
			if(sstreqi_ascii(buf, "abs"))        { Func = funcAbs; }
			else if(sstreqi_ascii(buf, "sqrt"))  { Func = funcSqrt; }
			else if(sstreqi_ascii(buf, "sin"))   { Func = funcSin; }
			else if(sstreqi_ascii(buf, "cos"))   { Func = funcCos; }
			else if(sstreqi_ascii(buf, "tan"))   { Func = funcTan; }
			else if(sstreqi_ascii(buf, "round")) { Func = funcRound; }
			else if(sstreqi_ascii(buf, "roundup")) { Func = funcRoundUp; }
			else if(sstreqi_ascii(buf, "rounddown")) { Func = funcRoundDown; }
			else if(sstreqi_ascii(buf, "rounddn")) { Func = funcRoundDown; }
			else if(sstreqi_ascii(buf, "oid"))   { Func = funcOid; }
			else if(sstreqi_ascii(buf, "pow"))   { Func = funcPow; }
			else if(sstreqi_ascii(buf, "min"))   { Func = funcMin; }
			else if(sstreqi_ascii(buf, "minz"))  { Func = funcMinZ; }
			else if(sstreqi_ascii(buf, "max"))   { Func = funcMax; }
			else if(sstreqi_ascii(buf, "?"))     { Func = funcQuest; }
			else if(sstreqi_ascii(buf, "eq"))    { Func = funcEq; }
			else if(sstreqi_ascii(buf, "neq"))   { Func = funcNEq; }
			else if(sstreqi_ascii(buf, "sieve")) {
				Func = funcSieve;
				THROW(GetSieve());
			}
			else if(!P_Ctx || !P_Ctx->IsFunc(buf, &Func)) {
				Func = 0;
				Name = buf;
				Token = tokName;
			}
			if(Func)
				Token = tokFunc;
		}
		else if(isdec(Cur) || Cur == '.') {
			int    dot = 0;
			do {
				if(Cur == '.')
					dot = 1;
				*p++ = Cur;
				Next();
			} while(isdec(Cur) || ((Cur == '.') && !dot));
			*p = 0;
			Number = atof(buf);
			Token  = tokNumber;
		}
		else if(Cur == '<') {
			if(Next() == '=') {
				Token = tokLe;
				Next();
			}
			else
				Token = '<';
		}
		else if(Cur == '>') {
			if(Next() == '=') {
				Token = tokGe;
				Next();
			}
			else
				Token = '>';
		}
		else if(Cur == '!') {
			if(Next() == '=') {
				Token = tokNe;
				Next();
			}
		}
		else if(strchr("*)(%/-+=,", Cur)) {
			Token = Cur;
			Next();
		}
	}
	CATCH
		Token = 0;
	ENDCATCH
	return Token;
}

int FASTCALL PPExprParser::GetFactor(double * pResult)
{
	int    ok = 1;
	int    uminus = 0;
	THROW(GetToken());
	//
	// Цикл для корректной обработки любого количества
	// подряд идущих плюсов и минусов
	//
	while(oneof2(Token, '-', '+')) {
		if(Token == '-')
			uminus++;
		THROW(GetToken());
	}
	uminus = uminus % 2;
	switch(Token) {
		case '(':
			THROW(CalcExpr(pResult));
			THROW_PP_S(Token == ')', PPERR_INVEXPR, P_S);
			break;
		case tokNumber:
			*pResult = Number;
			break;
		case tokName:
			{
				uint16 func_id = 0;
				if(P_CFL && (func_id = P_CFL->SearchFuncByName(Name)) != 0) {
					StringSet params;
					char   result[64];
					size_t next_pos;
					THROW(P_CFL->ReadParams(func_id, P_S+Pos-1, &next_pos, &params));
					Pos += next_pos;
					Cur = P_S[Pos-1];
					THROW(P_CFL->CalcFunc(func_id, &params, result, sizeof(result)));
					*pResult = atof(result);
				}
				else {
					//THROW_PP(P_Ctx, PPERR_ILLVAR);
					//THROW(P_Ctx->Resolve(Name, pResult));
					THROW_PP_S(ResolveContextName(Name, pResult), PPERR_ILLVAR, Name);
				}
			}
			break;
		case tokFunc:
			{
				ExprEvalContext::FC fc;
				fc.Init();
				int    func = Func;
				size_t arg_list_offs = 0;
				double first_arg = 0.0;
				ReadArgString(fc.StrArg, &arg_list_offs);
				if(func == funcOid || (func >= EXRP_EVAL_FIRST_FUNC && P_Ctx && P_Ctx->SelfScanArgList())) {
					Pos += arg_list_offs+1;
					Cur = P_S[Pos-1];
				}
				else {
					THROW(GetToken());
					THROW_PP_S(Token == '(', PPERR_INVEXPR, P_S);
					Skip();
					if(Cur != ')') {
						do {
							double arg = 0.0;
							const int arg_n = fc.ArgList.getCount();
							THROW(CalcExpr(&arg));
							if(arg_n == 0)
								first_arg = arg;
							fc.ArgList.insert(&arg);
						} while(Token == ',');
					}
					else {
						THROW(GetToken());
					}
					THROW_PP_S(Token == ')', PPERR_INVEXPR, P_S);
				}
				const uint arg_count = fc.ArgList.getCount();
				switch(func) {
					case funcAbs:   *pResult = fabs(first_arg); break;
					case funcSqrt:  *pResult = sqrt(first_arg); break;
					case funcSin:   *pResult = sin(first_arg);  break;
					case funcCos:   *pResult = cos(first_arg);  break;
					case funcTan:   *pResult = tan(first_arg);  break;
					case funcPow:   *pResult = pow(first_arg, (arg_count > 1) ? fc.ArgList.at(1) : 1.0); break;
					case funcRound:
						*pResult = round(first_arg, (arg_count > 1) ? (int)fc.ArgList.at(1) : 0);
						break;
					// @v8.1.9 {
					case funcRoundUp:
						*pResult = round(first_arg, (arg_count > 1) ? fc.ArgList.at(1) : 0.01, +1);
						break;
					case funcRoundDown:
						*pResult = round(first_arg, (arg_count > 1) ? fc.ArgList.at(1) : 0.01, -1);
						break;
					// } @v8.1.9
					case funcEq:
						if(arg_count == 2)
							*pResult = BIN(first_arg == fc.ArgList.at(1));
						else
							*pResult = 0.0;
						break;
					case funcNEq:
						if(arg_count == 2)
							*pResult = BIN(first_arg != fc.ArgList.at(1));
						else
							*pResult = 1.0;
						break;
					case funcMin:
						if(arg_count) {
							double t = SMathConst::Max;
							for(uint i = 0; i < arg_count; i++) {
								double v = fc.ArgList.at(i);
								SETMIN(t, v);
							}
							if(t == SMathConst::Max)
								*pResult = 0.0;
							*pResult = t;
						}
						break;
					case funcMinZ:
						if(arg_count) {
							double t = SMathConst::Max;
							for(uint i = 0; i < arg_count; i++) {
								double v = fc.ArgList.at(i);
								if(v != 0.0) {
									SETMIN(t, v);
								}
							}
							if(t == SMathConst::Max)
								*pResult = 0.0;
							*pResult = t;
						}
						break;
					case funcMax:
						if(arg_count) {
							double t = -SMathConst::Max;
							for(uint i = 0; i < arg_count; i++) {
								double v = fc.ArgList.at(i);
								SETMAX(t, v);
							}
							*pResult = t;
						}
						break;
					case funcQuest:
						if(first_arg != 0.0)
							*pResult = (arg_count > 1) ? fc.ArgList.at(1) : 1.0;
						else
							*pResult = (arg_count > 2) ? fc.ArgList.at(2) : 0.0;
						break;
					case funcOid:
						{
							PPObjID oid;
							SString obj_type_symb;
							SString obj_id_symb;
							uint   pos = 0;
							oid.Set(0, 0);
							fc.StrArg.Strip();
							THROW_PP_S(fc.StrArg[0] == '(', PPERR_INVEXPR, P_S);
							fc.StrArg.ShiftLeft().Strip();
							THROW_PP_S(fc.StrArg.Last() == ')', PPERR_INVEXPR, P_S);
							fc.StrArg.TrimRight().Strip();
							{
								StringSet ss(',', fc.StrArg);
								THROW_PP(ss.get(&pos, obj_type_symb) && ss.get(&pos, obj_id_symb), PPERR_OIDFUNC_ARGCOUNT);
								THROW(PPObject::Identify(obj_type_symb.Strip(), obj_id_symb.Strip(), &oid) > 0);
								*pResult = (double)oid;
							}
						}
						break;
					case funcSieve:
						{
							long   bin_id = 0;
							*pResult = Sieve.GetBinByVal(first_arg, &bin_id) ? SieveVal.Get(bin_id) : 0.0;
						}
						break;
					default:
						CALLPTRMEMB(P_Ctx, ResolveFunc(func, fc));
						*pResult = fc.RetReal;
				}
			}
			break;
		default:
			CALLEXCEPT_PP_S(PPERR_INVEXPR, P_S);
	}
	THROW(GetToken());
	if(uminus)
		*pResult = -*pResult;
	CATCHZOK
	return ok;
}

int SLAPI PPExprParser::ReadArgString(SString & rBuf, size_t * pOffs) const
{
	int    ok = 1;
	rBuf = 0;
	const size_t pos = Pos ? (Pos-1) : Pos; // Pos является указателем на символ, следующий за PPExprParser::Cur
	const char * p = &P_S[pos];
	while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	if(*p == '(') {
		uint   par_count = 0;
		rBuf.CatChar('(');
		p++;
		do {
			if(*p == '(')
				++par_count;
			else if(*p == ')') {
				if(par_count)
					--par_count;
				else
					break;
			}
			rBuf.CatChar(*p++);
		} while(*p != 0);
		if(*p != ')')
			ok = 0;
		else
			rBuf.CatChar(')');
		ASSIGN_PTR(pOffs, (p - &P_S[pos]));
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL PPExprParser::GetTerm(double * pResult)
{
	int    ok = 1;
	THROW(GetFactor(pResult));
	while(Token == '*' || Token == '/' || Token == '%') {
		double mult = 0.0;
		int    op   = Token;
		THROW(GetFactor(&mult));
		switch(op) {
			case '*':
				if(*pResult != 0.0)
					*pResult *= mult;
				break;
			case '/':
				*pResult = fdivnz(*pResult, mult);
				break;
			case '%':
				if(mult != 0.0)
					*pResult = (long)*pResult % (long)mult;
				else
					*pResult = 0.0;
				break;
		}
	}
	CATCHZOK
	return ok;
}

static double FASTCALL quest(int condition)
{
	return condition ? 1.0 : 0.0;
}

int FASTCALL PPExprParser::CalcExpr(double * pResult)
{
	int    ok = 1;
	double result = 0.0;
	//
	// Сохраняем текущее состояние, критичное к рекурсивному вызову CalcExpr {
	//
	State save_state;
	save_state = St;
	// }
	THROW(GetTerm(&result));
	while(oneof8(Token, '+', '-', '<', '>', '=', tokLe, tokGe, tokNe)) {
		double add = 0.0;
		if(Token == '+' || Token == '-') {
			int    sign = (Token == '+') ? 1 : -1;
			THROW(GetTerm(&add));
			if(sign > 0)
				result += add;
			else
				result -= add;
		}
		else {
			const int tok = Token;
			//
			// Операции -, + имеют больший приоритет, чем <>=. Поэтому вызываем
			// CalcExpr вместо { GetToken(); GetTerm(&add); }
			//
			THROW(CalcExpr(&add)); // @recursion
			switch(tok) {
				case '=':   result = quest(result == add); break;
				case tokNe: result = quest(result != add); break;
				case '<':   result = quest(result <  add); break;
				case tokLe: result = quest(result <= add); break;
				case '>':   result = quest(result >  add); break;
				case tokGe: result = quest(result >= add); break;
			}
		}
	}
	CATCH
		ok = 0;
		result = 0.0;
	ENDCATCH
	St = save_state; // Восстанавливаем текущее состояние
	ASSIGN_PTR(pResult, result);
	return ok;
}
//
// GoodsContext
//
SLAPI GoodsContext::Param::Param()
{
	THISZERO();
}

SLAPI GoodsContext::GoodsContext(GdsClsCalcExprContext * pCtx) : ExprEvalContext()
{
	Init();
	P_ClsPack = pCtx->P_GcPack;
	P_Pack = pCtx->P_GPack;
	P_Gs = pCtx->P_GsPack;
	P_BillPack = pCtx->P_BillPack;
	P_Ti = pCtx->P_Ti;
	P.Par1 = pCtx->Par1;
	P.Par2 = pCtx->Par2;
	P.Par3 = pCtx->Par3;
	P.TSessID = pCtx->TSessID;
	P.GoodsID = pCtx->GoodsID;
	P.PrevGoodsID = pCtx->PrevGoodsID;
	if(P.TSessID)
		P_TSesObj = new PPObjTSession;
}

SLAPI GoodsContext::GoodsContext(Param & rParam)
{
	Init();
	P = rParam;
}

SLAPI GoodsContext::GoodsContext(const PPTransferItem * pTi, const PPBillPacket * pBillPack) : ExprEvalContext()
{
	Init();
	P_Ti = pTi;
	P_BillPack = pBillPack;
	P.GoodsID = P_Ti ? P_Ti->GoodsID : 0;
}

void SLAPI GoodsContext::Init()
{
	P_ClsPack = 0;
	P_Pack = 0;
	P_Gs = 0;
	P_BillPack = 0;
	P_Ti = 0;
	MEMSZERO(P);
	P_TSesObj = 0;
}

SLAPI GoodsContext::~GoodsContext()
{
	delete P_TSesObj;
}

const GoodsContext::Param & GoodsContext::GetParam() const
{
	return P;
}

int SLAPI GoodsContext::SetParam(const GoodsContext::Param & rP)
{
    P = rP;
    return 1;
}

int SLAPI GoodsContext::AllocGoodsStruc()
{
	int    ok = -1;
	if(P_Gs == 0) {
		if(P.TSessID) {
			TSessionTbl::Rec tses_rec;
			if(P_TSesObj->Search(P.TSessID, &tses_rec) > 0 && tses_rec.TechID) {
				TechTbl::Rec tec_rec;
				if(P_TSesObj->GetTech(tses_rec.TechID, &tec_rec, 1) > 0 && tec_rec.GStrucID) {
					if(GObj.GSObj.Get(tec_rec.GStrucID, &ProperGs) > 0) {
						P_Gs = &ProperGs;
						ok = 1;
					}
				}
			}
		}
		else if(P_Pack) {
			if(P_Pack->GS.Rec.ID) {
				P_Gs = &P_Pack->GS;
				ok = 1;
			}
		}
		else if(P.GoodsID) {
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(P.GoodsID, &goods_rec) > 0 && goods_rec.StrucID && GObj.GSObj.Get(goods_rec.StrucID, &ProperGs) > 0) {
				P_Gs = &ProperGs;
				ok = 1;
			}
		}
	}
	else
		ok = 1;
	return ok;
}

int SLAPI GoodsContext::IsFunc(const char * pSymb, int * pFuncId)
{
	int    func_id = 0;
	SString temp_buf;
	SStrScan scan(pSymb);
	if(scan.Skip().GetIdent(temp_buf)) {
		if(temp_buf.CmpNC("vat") == 0)
			func_id = funcVat;
		else if(temp_buf.CmpNC("vatplus") == 0)
			func_id = funcVatPlus;
		else if(temp_buf.CmpNC("vatminus") == 0)
			func_id = funcVatMinus;
		else if(temp_buf.CmpNC("getquot") == 0)
			func_id = funcGetQuot;
		else if(temp_buf.CmpNC("getquotcp") == 0)
			func_id = funcGetQuotCP;
		else {
			PPSetError(PPERR_UNDEFGOODSFUNC, pSymb);
		}
	}
	ASSIGN_PTR(pFuncId, func_id);
	return BIN(func_id);
}

int SLAPI GoodsContext::ResolveFunc(int funcId, FC & rFc)
{
	double result = 0.0;
	double arg = rFc.ArgList.getCount() ? rFc.ArgList.at(0) : 0.0;
	Goods2Tbl::Rec goods_rec;
	if(funcId == funcVat) {
		int with_vat = (rFc.ArgList.getCount() > 1) ? (int)rFc.ArgList.at(1) : 0;
		if(GObj.Fetch(P.GoodsID, &goods_rec) > 0) {
			GObj.CalcCostVat(0, goods_rec.TaxGrpID, ZERODATE, 1.0, arg, &result, !with_vat, 0);
		}
	}
	else if(funcId == funcVatPlus) {
		if(GObj.Fetch(P.GoodsID, &goods_rec) > 0) {
			GObj.CalcCostVat(0, goods_rec.TaxGrpID, ZERODATE, 1.0, arg, &result, 1, 0);
			result += arg;
		}
		else
			result = arg;
	}
	else if(funcId == funcVatMinus) {
		if(GObj.Fetch(P.GoodsID, &goods_rec) > 0) {
			GObj.CalcCostVat(0, goods_rec.TaxGrpID, ZERODATE, 1.0, arg, &result, 0, 0);
			result = arg - result;
		}
		else
			result = arg;
	}
	else if(oneof2(funcId, funcGetQuot, funcGetQuotCP)) {
		PPID   qk_id = 0;
		PPID   cur_id = 0;
		PPID   ar_id = 0;
		PPID   loc_id = 0;
		int    loc_arg_defined = 0;
		uint   cn = 0;
		double cost  = P_Ti ? P_Ti->Cost  : 0.0;
		double price = P_Ti ? P_Ti->Price : 0.0;
		for(uint i = 0; i < rFc.ArgList.getCount(); i++) {
			const double arg = rFc.ArgList.at(i);
			PPObjID oid;
			oid = arg;
			switch(oid.Obj) {
				case PPOBJ_QUOTKIND:
					if(!qk_id)
						qk_id = oid.Id;
					break;
				case PPOBJ_CURRENCY:
					if(!cur_id)
						cur_id = oid.Id;
					break;
				case PPOBJ_LOCATION:
					if(!loc_id)
						loc_id = oid.Id;
					loc_arg_defined = 1;
					break;
				case PPOBJ_ARTICLE:
					if(!ar_id)
						ar_id = oid.Id;
					break;
				default:
					if(funcId == funcGetQuotCP) {
						if(cn == 0)
							cost = arg;
						else if(cn == 1)
							price = arg;
					}
					cn++;
					break;
			}
		}
		if(qk_id) {
			if(!loc_arg_defined) {
				SETIFZ(loc_id, P_BillPack ? P_BillPack->Rec.LocID : P.LocID);
			}
			SETIFZ(cur_id, P_BillPack ? P_BillPack->Rec.CurID : P.CurID);
			SETIFZ(ar_id, P_BillPack ? P_BillPack->Rec.Object : P.ArID);
			const QuotIdent qi(QIDATE(getcurdate_()), loc_id, qk_id, cur_id, ar_id);
			GObj.GetQuotExt(P.GoodsID, qi, cost, price, &result, 0);
		}
	}
	rFc.RetType = T_DOUBLE;
	rFc.RetReal = result;
	return 1;
}

int SLAPI GoodsContext::IsRef(const char * pStr, size_t * pPos, SString & rRef)
{
	size_t pos = *pPos;
	SString temp_buf;
	if(isalpha(pStr[pos]) || pStr[pos] == '_' || pStr[pos] == '@')
		do {
			temp_buf.CatChar(pStr[pos++]);
		} while(isalnum(pStr[pos]) || pStr[pos] == '_');
	if(pStr[pos] == '.') {
		pos++;
		ASSIGN_PTR(pPos, pos);
		rRef = temp_buf;
		return 1;
	}
	else
		return 0;
}

int SLAPI GoodsContext::ResolveRefQtty(PPID goodsID, int cbItemRef, int isPhQtty, double * pVal)
{
	int    ok = 1;
	double v = 0.0;
	if(goodsID) {
		if(P.TSessID) {
			SETIFZ(P_TSesObj, new PPObjTSession);
			TSessGoodsTotal tsgt;
			MEMSZERO(tsgt);
			THROW(P_TSesObj->P_Tbl->CalcGoodsTotal(P.TSessID, goodsID, &tsgt));
			v = isPhQtty ? tsgt.PhQtty : tsgt.Qtty;
		}
		else if(cbItemRef >= 0 && P_Gs && P_Gs->P_Cb) {
			const ComplItem * p_ci = 0;
			if(cbItemRef == 0)
				p_ci = &P_Gs->P_Cb->Head;
			else if(cbItemRef <= (int)P_Gs->P_Cb->getCount())
				p_ci = &P_Gs->P_Cb->at(cbItemRef-1);
			if(p_ci) {
				if(isPhQtty) {
					// @todo
				}
				else {
					v = fabs(p_ci->NeedQty);
				}
			}
		}
		else if(P_BillPack) {
			//PPBillPacket
            for(uint i = 0; i < P_BillPack->GetTCount(); i++) {
				const PPTransferItem & r_ti = P_BillPack->ConstTI(i);
				if(labs(r_ti.GoodsID) == labs(goodsID)) {
					if(isPhQtty) {
						// @todo
					}
					else {
						v += fabs(r_ti.Qtty());
					}
				}
            }
		}
		else {
			v = P.Qtty;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pVal, v);
	return ok;
}

int SLAPI GoodsContext::ResolveGoodsAttr(const SString & rSymb, PPID goodsID, double * pVal)
{
	int    ok = 1;
	int    ref_dim = 0;
	Goods2Tbl::Rec goods_rec;
	double v = 0.0;
	if(rSymb.CmpNC("X") == 0) ref_dim = PPGdsCls::eX;
	else if(rSymb.CmpNC("Y") == 0) ref_dim = PPGdsCls::eY;
	else if(rSymb.CmpNC("Z") == 0) ref_dim = PPGdsCls::eZ;
	else if(rSymb.CmpNC("W") == 0) ref_dim = PPGdsCls::eW;
	else if(rSymb.CmpNC("KIND") == 0) ref_dim = PPGdsCls::eKind;
	else if(rSymb.CmpNC("GRADE") == 0) ref_dim = PPGdsCls::eGrade;
	else if(rSymb.CmpNC("ADDPROP2") == 0) ref_dim = PPGdsCls::eAdd;
	else if(rSymb.CmpNC("ADDPROP") == 0) ref_dim = PPGdsCls::eAdd2;
	else if(rSymb.CmpNC("PHUPERU") == 0) {
		if(GObj.Fetch(goodsID, &goods_rec) > 0)
			v = goods_rec.PhUPerU;
		else if(P_Pack)
			v = P_Pack->Rec.PhUPerU;
	}
	else if(rSymb.CmpNC("TYPE") == 0) {
		PPID   goods_type = (GObj.Fetch(goodsID, &goods_rec) > 0) ? goods_rec.GoodsTypeID : 0;
		PPObjID oid;
		v = (double)oid.Set(PPOBJ_GOODSTYPE, goods_type);
	}
	else if(rSymb.CmpNC("ID") == 0)
		v = goodsID;
	else
		ok = 0;
	if(ref_dim) {
		PPObjID oid;
		if(goodsID) {
			PPGdsClsPacket gc_pack;
			if(GObj.FetchCls(goodsID, &goods_rec, &gc_pack) > 0) {
				GoodsExtTbl::Rec ge_rec;
				if(GObj.P_Tbl->GetExt(goodsID, &ge_rec) > 0) {
					switch(ref_dim) {
						case PPGdsCls::eKind : v = (double)oid.Set(gc_pack.PropKind.ItemsListID, ge_rec.KindID); break;
						case PPGdsCls::eGrade: v = (double)oid.Set(gc_pack.PropGrade.ItemsListID, ge_rec.GradeID); break;
						case PPGdsCls::eAdd  : v = (double)oid.Set(gc_pack.PropAdd.ItemsListID, ge_rec.AddObjID); break;
						case PPGdsCls::eAdd2 : v = (double)oid.Set(gc_pack.PropAdd2.ItemsListID, ge_rec.AddObj2ID); break;
						default:
							gc_pack.GetExtDim(&ge_rec, ref_dim, &v);
							break;
					}
				}
			}
		}
		else if(P_ClsPack && P_Pack) {
			switch(ref_dim) {
				case PPGdsCls::eKind : v = (double)oid.Set(P_ClsPack->PropKind.ItemsListID, P_Pack->ExtRec.KindID); break;
				case PPGdsCls::eGrade: v = (double)oid.Set(P_ClsPack->PropGrade.ItemsListID, P_Pack->ExtRec.GradeID); break;
				case PPGdsCls::eAdd  : v = (double)oid.Set(P_ClsPack->PropAdd.ItemsListID, P_Pack->ExtRec.AddObjID); break;
				case PPGdsCls::eAdd2 : v = (double)oid.Set(P_ClsPack->PropAdd2.ItemsListID, P_Pack->ExtRec.AddObj2ID); break;
				default:
					P_ClsPack->GetExtDim(&P_Pack->ExtRec, ref_dim, &v);
					break;
			}
		}
	}
	ASSIGN_PTR(pVal, v);
	return ok;
}

int SLAPI GoodsContext::Resolve(const char * pSymb, double * pVal)
{
	int    ok = 1;
	int    prev_goods_prefix = 0;
	double v = 0.0;
	SString ref;
	if(!ResolveGoodsAttr(ref = pSymb, P_Pack ? 0 : P.GoodsID, &v)) {
		size_t pos = 0;
		if(IsRef(pSymb, &pos, ref = 0)) {
			enum {
				spcrefQuot = 1,
				spcrefLotTag,
				spcrefGoodsTag
			};
			int    spcref = 0;
			int    ref_dim = 0;
			PPID   ref_goods_id = 0;
			int    ref_gscb_item = -1;
			if(ref.CmpNC("MAIN") == 0) {
				THROW(AllocGoodsStruc());
				THROW_PP_S(P_Gs, PPERR_INVEXPRCTX_GS, pSymb);
				ref_goods_id = P_Gs->GoodsID;
				if(P_Gs->P_Cb)
					ref_gscb_item = 0;
			}
			else if(ref.CmpNC("MAINCOM") == 0) {
				const PPGoodsStrucItem * p_gsi = 0;
				THROW(AllocGoodsStruc());
				THROW_PP_S(P_Gs, PPERR_INVEXPRCTX_GS, pSymb);
				THROW(p_gsi = P_Gs->GetMainItem(0));
				ref_goods_id = p_gsi->GoodsID;
				if(P_Gs->P_Cb) {
					for(uint i = 0; ref_gscb_item < 0 && i < P_Gs->P_Cb->getCount(); i++) {
						const ComplItem & r_ci = P_Gs->P_Cb->at(i);
						if(r_ci.GsiFlags & GSIF_MAINITEM)
							ref_gscb_item = i+1;
					}
				}
			}
			else if(ref.CmpNC("PREV") == 0) {
				ref_goods_id = P.PrevGoodsID;
			}
			else if(ref.CmpNC("QUOT") == 0) {
				spcref = spcrefQuot;
			}
			else if(ref.CmpNC("LOTTAG") == 0) {
				spcref = spcrefLotTag;
			}
			else if(ref.CmpNC("TAG") == 0) {
				spcref = spcrefGoodsTag;
			}
			else {
				THROW(AllocGoodsStruc());
				if(P_Gs) {
					uint   gs_pos = 0;
					BarcodeTbl::Rec bc_rec;
					//
					// Сначала выясняем, не является ли ссылка кодом элемента структуры
					//
					if(P_Gs->SearchSymb(ref, &gs_pos) > 0) {
						ref_goods_id = P_Gs->Items.at(gs_pos).GoodsID;
						if(P_Gs->P_Cb) {
							for(uint i = 0; ref_gscb_item < 0 && i < P_Gs->P_Cb->getCount(); i++) {
								const ComplItem & r_ci = P_Gs->P_Cb->at(i);
								if(r_ci.SrcGsPos == (gs_pos+1))
									ref_gscb_item = i+1;
							}
						}
					}
					//
					// Ссылка может являться кодом товара
					//
					else if(GObj.P_Tbl->SearchBarcode(ref, &bc_rec) > 0) {
						ref_goods_id = bc_rec.GoodsID;
					}
					else {
						SString ref2;
						//
						// Возможно, ссылка является кодом группы
						//
						ref2.CatChar('@').Cat(ref);
						if(GObj.P_Tbl->SearchBarcode(ref2, &bc_rec) > 0) {
							ref_goods_id = bc_rec.GoodsID;
						}
						//
						// Ничего не помогает: ссылка не может быть разрешена
						//
						else {
							CALLEXCEPT_PP_S(PPERR_INVEXPRREF, ref);
						}
					}
				}
				else {
					CALLEXCEPT_PP_S(PPERR_INVEXPRREF, ref);
					//THROW_PP_S(P_Gs, PPERR_INVEXPRCTX_GS, pSymb);
				}
			}
			ref = pSymb+pos;
			if(spcref == spcrefQuot) {
				PPObjQuotKind qk_obj;
				PPID   qk_id = 0;
				THROW(qk_obj.SearchSymb(&qk_id, ref) > 0);
				{
					const double cost  = P_Ti ? P_Ti->Cost  : 0.0;
					const double price = P_Ti ? P_Ti->Price : 0.0;
					const PPID   loc_id = P_BillPack ? P_BillPack->Rec.LocID : P.LocID;
					const PPID   cur_id = P_BillPack ? P_BillPack->Rec.CurID : P.CurID;
					const PPID   ar_id  = P_BillPack ? P_BillPack->Rec.Object : P.ArID;
					const LDATE  _dt = P_BillPack ? P_BillPack->Rec.Dt : getcurdate_();
					const QuotIdent qi(QIDATE(_dt), loc_id, qk_id, cur_id, ar_id);
					GObj.GetQuotExt(P.GoodsID, qi, cost, price, &v, 0);
				}
			}
			else if(spcref == spcrefLotTag) {
				PPObjTag tag_obj;
				PPID   tag_id = 0;
				THROW(tag_obj.SearchBySymb(ref, &tag_id, 0) > 0);
				if(P_Ti) {
					const ObjTagItem * p_tag_item = 0;
					if(P_BillPack) {
						int    row_pos = (int)P_BillPack->GetTPointer();
						ObjTagList * p_tag_list = P_BillPack->LTagL.Get(row_pos);
						if(p_tag_list)
							p_tag_item = p_tag_list->GetItem(tag_id);
					}
					else if(P_Ti->LotID) {
						ObjTagList tag_list;
						BillObj->GetTagListByLot(P_Ti->LotID, 0, &tag_list);
						p_tag_item = tag_list.GetItem(tag_id);
					}
					if(p_tag_item)
						p_tag_item->GetReal(&v);
				}
			}
			else if(spcref == spcrefGoodsTag) {
				PPObjTag tag_obj;
				PPID   tag_id = 0;
				THROW(tag_obj.SearchBySymb(ref, &tag_id, 0) > 0);
				{
					ObjTagItem tag_item;
					if(PPRef->Ot.GetTag(PPOBJ_GOODS, P.GoodsID, tag_id, &tag_item) > 0)
						tag_item.GetReal(&v);
				}
			}
			else if(ref.CmpNC("QTTY") == 0 || ref.CmpNC("QTY") == 0) {
				THROW(ResolveRefQtty(ref_goods_id, ref_gscb_item, 0, &v));
			}
			else if(ref.CmpNC("PHQTTY") == 0 || ref.CmpNC("PHQTY") == 0) {
				THROW(ResolveRefQtty(ref_goods_id, ref_gscb_item, 1, &v));
			}
			else if(ref.CmpNC("QTTYCOM") == 0) {
				THROW(AllocGoodsStruc());
				THROW_PP_S(P_Gs, PPERR_INVEXPRCTX_GS, pSymb);
				THROW(P_Gs->GetItemQtty(ref_goods_id, 1, &v));
			}
			else {
				THROW_PP_S(ResolveGoodsAttr(ref, ref_goods_id, &v), PPERR_INVEXPRVAR, ref)
			}
		}
		else {
			ref = pSymb;
			if(ref.CmpNC("pi") == 0)
				v = SMathConst::Pi;
			else if(ref.CmpNC("a1") == 0)
				v = P.Par1;
			else if(ref.CmpNC("a2") == 0)
				v = P.Par2;
			else if(ref.CmpNC("a3") == 0)
				v = P.Par3;
			else {
				int    r = 1;
				if(ref.CmpNC("cost") == 0) {
					if(P_Ti)
						v = P_Ti->Cost;
					else
						r = 0; // cost может быть суммой документа
				}
				else if(ref.CmpNC("price") == 0) {
					if(P_Ti)
						v = P_Ti->Price;
					else
						r = 0; // price может быть суммой документа
				}
				else if(ref.CmpNC("discount") == 0) {
					if(P_Ti)
						v = P_Ti->Discount;
					else
						r = 0; // discount может быть суммой документа
				}
				else if(ref.CmpNC("QTTY") == 0 || ref.CmpNC("QTY") == 0)
					v = P_Ti ? P_Ti->Quantity_ : P.Qtty; // @v8.8.6 0.0-->P.Qtty
				else if(ref.CmpNC("PHQTTY") == 0 || ref.CmpNC("PHQTY") == 0) {
					if(P_Ti) {
						if(P_Ti->Flags & PPTFR_INDEPPHQTTY)
							v = P_Ti->WtQtty;
						else {
							PPID   ph_unit_id = 0;
							double phuperu = 0.0;
							GObj.GetPhUPerU(P_Ti->GoodsID, &ph_unit_id, &phuperu);
							v = P_Ti->Quantity_ * phuperu;
						}
					}
					else
						v = 0.0;
				}
				// @v8.9.12 {
				else if(ref.CmpNC("LOTDIMX") == 0) {
                    v = P.LotDim.X;
				}
				else if(ref.CmpNC("LOTDIMY") == 0) {
					v = P.LotDim.Y;
				}
				else if(ref.CmpNC("LOTDIMZ") == 0) {
					v = P.LotDim.Z;
				}
				// } @v8.9.12
				else
					r = 0;
				if(!r) {
					SString form;
					PPID   id = 0;
					BillSymbCache * p_cache = GetDbLocalCachePtr <BillSymbCache> (PPOBJ_BILLSYMB);
					THROW((r = p_cache->Fetch(ref, &id, form)) > 0);
					THROW_PP_S(r == 2, PPERR_INVEXPRREF, pSymb);
					THROW(PPExprParser::CalcExpression(form, &v, 0, this));
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pVal, v);
	return ok;
}
//
//
//
int SLAPI PPCalcExpression(const char * pStr, double * pResult, const PPCalcFuncList * pCFL)
{
	return PPExprParser::CalcExpression(pStr, pResult, pCFL, 0);
}

int SLAPI PPCalcExpression(const char * pStr, double * pResult, PPBillPacket * pPack, PPID curID, uint advItemIdx)
{
	int    ok = 1;
	BillContext * p_ctx = 0;
	PPOprKind op_rec;
	if(pPack && GetOpType(pPack->Rec.OpID, &op_rec) == PPOPT_ACCTURN && op_rec.SubType == OPSUBT_ACCWROFF) {
		DL2_Resolver dl200r;
		DateRange period;
		p_ctx = new BillContext(pPack, curID, advItemIdx);
		dl200r.SetPeriod(period.Set(pPack->Rec.PeriodLow, pPack->Rec.PeriodUpp));
		dl200r.SetCurArticle(advItemIdx);
		DL200_Context ctx_dl200(&dl200r, p_ctx);
		ok = PPExprParser::CalcExpression(pStr, pResult, 0, &ctx_dl200);
	}
	else {
		p_ctx = pPack ? new BillContext(pPack, curID, advItemIdx) : (BillContext*)0;
		ok = PPExprParser::CalcExpression(pStr, pResult, 0, p_ctx);
	}
	delete p_ctx;
	return ok;
}
//
//
//
SLAPI GdsClsCalcExprContext::GdsClsCalcExprContext(PPGdsClsPacket * pGcPack, PPGoodsPacket * pGPack)
{
	THISZERO();
	P_GcPack = pGcPack;
	P_GPack = pGPack;
}

SLAPI GdsClsCalcExprContext::GdsClsCalcExprContext(const PPGoodsStruc * pGs, PPID sessID)
{
	THISZERO();
	P_GsPack = pGs;
	TSessID = sessID;
}

SLAPI GdsClsCalcExprContext::GdsClsCalcExprContext(const PPGoodsStruc * pGs, const PPBillPacket * pBillPack)
{
	THISZERO();
	P_GsPack = pGs;
	P_BillPack = pBillPack;
}

SLAPI GdsClsCalcExprContext::GdsClsCalcExprContext(PPID goodsID, PPID prevGoodsID)
{
	THISZERO();
	GoodsID = goodsID;
	PrevGoodsID = prevGoodsID;
}

SLAPI GdsClsCalcExprContext::GdsClsCalcExprContext(const PPTransferItem * pTi, const PPBillPacket * pBillPack)
{
	THISZERO();
	P_Ti = pTi;
	P_BillPack = pBillPack;
	GoodsID = P_Ti ? P_Ti->GoodsID : 0;
}

int SLAPI PPCalcExpression(const char * pStr, double * pResult, GdsClsCalcExprContext * pCtx)
{
	GoodsContext * p_ctx = (pCtx && (pCtx->P_GPack || pCtx->P_GsPack ||
		(pCtx->GoodsID && (pCtx->PrevGoodsID || pCtx->P_Ti)))) ? new GoodsContext(pCtx) : (GoodsContext*)0;
	int    ok = PPExprParser::CalcExpression(pStr, pResult, 0, p_ctx);
	delete p_ctx;
	return ok;
}

#if SLTEST_RUNNING // {

SLTEST_R(PPExprParser)
{
	const double tolerance = 1.0e-14;

	SEnum en = EnumTab(GetTestName());
	THROW(en);
	{
		SString left, right;
		SString msg_buf;
		STab::Row row;
		while(en.Next(&row)) {
			double v_left = 0.0;
			double v_right = 0.0;
			row.Get(0, left = 0);
			row.Get(1, right = 0);
			if(left.NotEmptyS() || right.NotEmptyS()) {
				SLTEST_CHECK_NZ(PPExprParser::CalcExpression(left, &v_left, 0, 0));
				SLTEST_CHECK_NZ(PPExprParser::CalcExpression(right, &v_right, 0, 0));
				SLTEST_CHECK_EQ_TOL(v_left, v_right, tolerance);
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING

