// BEXTQRY.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2004, 2006, 2007, 2008, 2009, 2010, 2013, 2015, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
// Интерфейс к расширенным операциям поиска Btrieve
//
#include <slib-internal.h>
#pragma hdrstop

#define EG_sign 0x4745U
#define UC_sign 0x4355U

/*static*/void FASTCALL BExtQuery::ZDelete(BExtQuery ** ppQ)
{
	if(ppQ && *ppQ) {
		delete *ppQ;
		*ppQ = 0;
	}
}

/*static*/int FASTCALL BExtQuery::SlToBeqOp(int s)
{
	switch(s) {
		case _EQ_: return opEq;
		case _GT_: return opGt;
		case _LT_: return opLt;
		case _NE_: return opNe;
		case _GE_: return opGe;
		case _LE_: return opLe;
	}
	assert(0);
	return 0;
}
	
/*static*/int FASTCALL BExtQuery::SlToBeqLink(int s)
{
	switch(s) {
		case _AND___: return linkAnd;
		case _OR___: return linkOr;
		case _END___: return linkEnd;
	}
	assert(0);
	return 0;
}

void BExtQuery::Init(DBTable * pTbl, int idx, uint aBufSize)
{
	P_Tbl = pTbl;
	Index_ = idx;
	P_Restrict = 0;
	P_Tree     = 0;
	P_QBuf     = 0;
	RecSize    = 0;
	ActCount   = 0;
	TailOfs    = 0;
	State      = 0;
	State     |= stFillBuf;
	MaxReject  = 0xffffU;
	P_Stmt = 0;
	const long dbp_capabilities = P_Tbl->GetDb() ? P_Tbl->GetDb()->GetCapability() : 0;
	if(dbp_capabilities & DbProvider::cSQL) {
		State |= stSqlProvider;
		if(dbp_capabilities & DbProvider::cDirectSelectDataMapping) {
			MaxRecs = 1;
		}
		else {
			MaxRecs = aBufSize;	
		}
	}
	else {
		MaxRecs = aBufSize;	
	}
}

BExtQuery::BExtQuery(DBTable * pTbl, int idx, uint aBufSize) : Buf(0)
{
	Init(pTbl, idx, aBufSize);
}

BExtQuery::BExtQuery(DBTable * pTbl, int idx) : Buf(0)
{
	Init(pTbl, idx, 32);
}

BExtQuery::~BExtQuery()
{
	delete P_Stmt;
	if(P_Restrict) {
		P_Restrict->destroy(1);
		delete P_Restrict;
	}
	SAlloc::F(P_QBuf);
}

int BExtQuery::CreateSqlExpr(Generator_SQL & rSg, int reverse, const char * pInitKey, int initSpMode) const
{
	int    ok = 1;
	SString temp_buf;
	const  BNKeyList & r_indices = P_Tbl->GetIndices();
	const BNKey & r_key = r_indices[Index_];
	const int ns = r_key.getNumSeg();
	//Generator_SQL sg(rSg.GetServerType()/*sqlstORA*/, 0);
	rSg.Z().Tok(Generator_SQL::tokSelect);
	if(rSg.GetServerType() == sqlstORA) {
		rSg.HintBegin().HintIndex(*P_Tbl, 0, Index_, BIN(reverse)).HintEnd();
	}
	const uint c = Fields.GetCount();
	rSg.Sp();
	if(c) {
		for(uint i = 0; i < c; i++) {
			if(i)
				rSg.Com();
			rSg.Text(Fields.GetField(i).Name);
		}
	}
	else
		rSg.Aster();
	rSg.Sp().From(P_Tbl->GetName());
	if(rSg.GetServerType() == sqlstSQLite) {
		if(rSg.GetIndexName(*P_Tbl, Index_, temp_buf)) {
			rSg.Sp().Tok(Generator_SQL::tokIndexedBy).Sp().Text(temp_buf);
		}
	}
	int _where_tok = 0;
	if(pInitKey && initSpMode != -1) {
		//
		// Oracle не "хочет" применять hint INDEX если в ограничениях не присутствует поле из этого индекса.
		//
		//BNKey  key = P_Tbl->indexes[index];
		size_t offs = 0;
		char   temp[512];
		rSg.Sp().Tok(Generator_SQL::tokWhere).Sp();
		_where_tok = 1;
		rSg.LPar();
		for(int i = 0; i < ns; i++) {
			int fldid = r_key.getFieldID(i);
			const BNField & r_fld = r_indices.field(Index_, i);
			if(i > 0) { // НЕ первый сегмент
				rSg.Tok(Generator_SQL::tokAnd).Sp();
				if(initSpMode != spEq)
					rSg.LPar();
			}
			if(r_key.getFlags(i) & XIF_ACS) {
				//
				// Для ORACLE нечувствительность к регистру символов
				// реализуется функциональным сегментом индекса nls_lower(fld).
				// Аналогичная конструкция применяется при генерации скрипта создания индекса
				// См. Generator_SQL::CreateIndex(const DBTable &, const char *, uint)
				//
				int   _func_tok = 0;
				if(rSg.GetServerType() == sqlstORA)
					_func_tok = Generator_SQL::tokNlsLower;
				else
					_func_tok = Generator_SQL::tokLower;
				rSg.Func(_func_tok, r_fld.Name);
			}
			else
				rSg.Text(r_fld.Name);

			int   cmps = _EQ_;
			if(initSpMode == spEq)
				cmps = _EQ_;
			else if(initSpMode == spLt)
				cmps = _LE_; //cmps = (i == ns-1) ? _LT_ : _LE_;
			else if(oneof2(initSpMode, spLe, spLast))
				cmps = _LE_;
			else if(initSpMode == spGt)
				cmps = _GE_; //cmps = (i == ns-1) ? _GT_ : _GE_;
			else if(oneof2(initSpMode, spGe, spFirst))
				cmps = _GE_;
			rSg._Symb(cmps);
			sttostr(r_fld.T, PTR8C(pInitKey)+offs, COMF_SQL, temp);
			rSg.Text(temp);
			if(i > 0 && initSpMode != spEq) {
				//
				// При каскадном сравнении ключа второй и последующие сегменты
				// должны удовлетворять условиям неравенства только при равенстве
				// всех предыдущих сегментов.
				//
				// Пример:
				//
				// index {X, Y, Z}
				// X > :A and (Y > :B or (X <> :A)) and (Z > :C or (X <> :A and Y <> :B))
				//
				rSg.Sp().Tok(Generator_SQL::tokOr).Sp().LPar();
				for(int j = 0; j < i; j++) {
					const BNField & r_fld2 = r_indices.field(Index_, j);
					if(j > 0)
						rSg.Tok(Generator_SQL::tokAnd).Sp();
					if(r_key.getFlags(j) & XIF_ACS) {
						int   _func_tok = 0;
						if(rSg.GetServerType() == sqlstORA)
							_func_tok = Generator_SQL::tokNlsLower;
						else
							_func_tok = Generator_SQL::tokLower;
						rSg.Func(_func_tok, r_fld2.Name);
					}
					else
						rSg.Text(r_fld2.Name);
					rSg._Symb(_NE_);
					sttostr(r_fld2.T, PTR8C(pInitKey) + r_indices.getSegOffset(Index_, j), COMF_SQL, temp);
					rSg.Text(temp);
					rSg.Sp();
				}
				rSg.RPar().RPar().Sp();
			}
			offs += stsize(r_fld.T);
			rSg.Sp();
		}
		rSg.RPar();
	}
	if(P_Restrict) {
		if(!_where_tok)
			rSg.Sp().Tok(Generator_SQL::tokWhere).Sp();
		else
			rSg.Sp().Tok(Generator_SQL::tokAnd).Sp();
		P_Tree->CreateSqlExpr(rSg, -1);
	}
	if(rSg.GetServerType() == sqlstSQLite) {
		if(reverse) { // Обратное направление поиска
			rSg.Tok(Generator_SQL::tokOrderBy);
			for(int i = 0; i < ns; i++) {
				const BNField & r_fld = r_indices.field(Index_, i);
				if(i)
					rSg.Com();
				rSg.Sp().Text(r_fld.Name).Sp().Tok(Generator_SQL::tokDesc);
			}
			rSg.Sp();
		}
	}
	return ok;
}

int BExtQuery::initIteration(bool reverse, const void * pInitKey, int initSpMode)
{
	int    ok = 1;
	State &= (stSqlProvider | 0); // Обнуляем все признаки кроме stSqlProvider
	State |= (stFirst | stFillBuf);
	for(uint i = 0; i < Fields.GetCount(); i++) {
		if(GETSTYPE(Fields.GetField(i).T) == S_NOTE)
			State |= stHasNote;
	}
	if(reverse)
		State |= stReverse;
	InitSpMode = initSpMode;
	if(pInitKey) {
		memcpy(InitKey_, pInitKey, BTRMAXKEYLEN);
	}
	else
		State |= stUndefInitKey;
	return ok;
}

int BExtQuery::fetchFirst(void * pInitKey, int initSpMode)
{
	const  int sp = (State & stReverse) ? spLast : spFirst;
	int    ok = BIN(search_first(static_cast<char *>(pInitKey), initSpMode, sp) > 0 || (BTRNFOUND && ActCount));
	if(ok > 0 && State & stFillBuf)
		fillTblBuf();
	return ok;
}

int BExtQuery::nextIteration()
{
	int    ok = 0;
	if(State & stFirstFail)
		ok = -1;
	else if(State & stFirst) {
		ok = fetchFirst((State & stUndefInitKey) ? static_cast<void *>(0) : InitKey_, InitSpMode);
		State &= ~stFirst;
		if(!ok)
			State |= stFirstFail;
		//
		// fetchFirst сама заполняет буфер данных таблицы. По этому здесь fillTblBuf не нужен
		//
	}
	else {
		if(++Cur < ActCount) {
			ok = (State & stSqlProvider) ? (P_Stmt ? P_Stmt->GetData(Cur) : 0) : 1;
		}
		else if(!(State & stEOF)) {
			Cur = 0;
			ok = BIN(_search((State & stReverse) ? spPrev : spNext, EG_sign) > 0 || (ActCount && BTRNFOUND));
		}
		if(ok > 0 && State & stFillBuf)
			fillTblBuf();
	}
	return ok;
}

void BExtQuery::resetEof()
{
	State &= ~stEOF;
}

long BExtQuery::countIterations(int reverse, const void * pInitKey, int initSpMode)
{
	long   c = 0;
	initIteration(reverse, pInitKey, initSpMode);
	State &= ~stFillBuf; // Для ускорения не будем заполнять буфер данных
	while(nextIteration() > 0)
		c++;
	State |= stFillBuf;
	return c;
}

int BExtQuery::search_first(const char * pInitKey, int initSpMode, int spMode)
{
	EXCEPTVAR(BtrError);
	ActCount = 0;
	RecSize = 0;
	Cur = 0;
	State &= ~stEOF;
	if(Index_ >= 0)
		P_Tbl->setIndex(Index_);
	int    ok = 1;
	int    sp;
	const  uint fields_count = Fields.GetCount();
	BExtTail * p_tail = 0;
	if(State & stSqlProvider) {
		const DbProvider * p_dbs = P_Tbl->GetDb();
		if(!p_dbs) {
			ok = -1; // @todo @err
		}
		else {
			const SqlServerType sql_server_type = p_dbs->GetSqlServerType();
			Generator_SQL sg(sql_server_type, 0);
			ZDELETE(P_Stmt);
			CreateSqlExpr(sg, BIN(State & stReverse), pInitKey, initSpMode);
			{
				int    r;
				uint   actual = 0;
				SSqlStmt stmt(P_Tbl->GetDb(), sg);
				THROW_V(P_Stmt = new SSqlStmt(P_Tbl->GetDb(), sg), BE_NOMEM);
				THROW(P_Stmt->Exec(0, OCI_DEFAULT));
				//
				// Распределяем буфер для хранения записи.
				//
				// Для SQL-провайдера не надо распределять буфер на все записи, но лишь только
				// на одну запись. Поскольку SSqlStmt сам содержит внутренний буфер, в котором
				// все извлеченные записи и так успешно храняться.
				//
				if(fields_count) {
					for(uint i = 0; i < fields_count; i++)
						RecSize += Fields.GetField(i).size();
				}
				else {
					//
					// Здесь нельзя использовать tbl->fields.CalculateFixedRecSize() поскольку
					// эта функция возвращает размер записи без полей переменной длины и без NOTE-полей.
					//
					for(uint i = 0; i < P_Tbl->GetFields().getCount(); i++)
						RecSize += P_Tbl->GetFields()[i].size();
				}
				THROW_V(Buf.Alloc(RecSize), BE_NOMEM);
				if(p_dbs->GetCapability() & DbProvider::cDirectSelectDataMapping) {
					assert(MaxRecs == 1); // Установлено в конструкторе BExtQuery
					if(fields_count) {
						THROW(P_Stmt->BindData(+1, 1, Fields, Buf.vcptr(), P_Tbl->getLobBlock()));
					}
					else {
						THROW(P_Stmt->BindData(+1, 1, P_Tbl->GetFields(), Buf.vcptr(), P_Tbl->getLobBlock()));
					}
					// @20251003
					{
						uint    local_actual_count = 0;
						THROW(r = P_Stmt->Fetch(1, &local_actual_count));
						if(r > 0) {
							/* @v12.4.2 это, кажись, не надо - функция Fetch уже получила данные //
							if(ActCount) {
								THROW(P_Stmt->GetData(0));
							}*/
							ActCount += local_actual_count;
							ok = 1;
						}
						else {
							BtrError = BE_EOF;
							ok = -1;
						}
					}
				}
				else {
					if(fields_count) {
						THROW(P_Stmt->BindData(+1, MaxRecs, Fields, Buf.vcptr(), P_Tbl->getLobBlock()));
					}
					else {
						THROW(P_Stmt->BindData(+1, MaxRecs, P_Tbl->GetFields(), Buf.vcptr(), P_Tbl->getLobBlock()));
					}
					THROW(r = P_Stmt->Fetch(MaxRecs, &ActCount));
					if(r > 0) {
						/* @v12.4.2 это, кажись, не надо - функция Fetch уже получила данные //
						if(ActCount) {
							THROW(P_Stmt->GetData(0));
						}*/
						ok = 1;
					}
					else {
						BtrError = BE_EOF;
						ok = -1;
					}
				}
			}
		}
	}
	else {
		const  uint fc = NZOR(fields_count, 1);
		const  uint tailLen = (fc + 1) * sizeof(BExtTailItem);
		BExtTailItem * p_item = 0;
		THROW_V(P_QBuf = static_cast<char *>(SAlloc::R(P_QBuf, sizeof(BExtHeader))), BE_NOMEM);
		memzero(P_QHead, sizeof(BExtHeader));
		P_QHead->bufLen = sizeof(BExtHeader);
		P_QHead->maxSkip = MaxReject; // 0xffffU;
		THROW(add_tree(0, P_Tree ? P_Tree->Root : 0));
		THROW_V(p_tail = static_cast<BExtTail *>(SAlloc::M(tailLen)), BE_NOMEM);
		memzero(p_tail, sizeof(BExtTail));
		p_tail->numFlds = fc;
		if(fields_count) {
			for(uint i = 0; i < fields_count; i++) {
				p_item = (reinterpret_cast<BExtTailItem *>(p_tail + 1)) + i;
				const BNField & f = Fields.GetField(i);
				p_item->fldLen = static_cast<uint16>(f.size());
				p_item->fldOfs = static_cast<uint16>(f.Offs);
				RecSize += p_item->fldLen;
			}
		}
		else {
			p_item = reinterpret_cast<BExtTailItem *>(p_tail + 1);
			p_item->fldLen = P_Tbl->GetFields().CalculateFixedRecSize();
			p_item->fldOfs = 0;
			RecSize += p_item->fldLen;
		}
		TailOfs = P_QHead->bufLen;
		THROW_V(P_QBuf = static_cast<char *>(SAlloc::R(P_QBuf, P_QHead->bufLen + tailLen)), BE_NOMEM);
		memcpy(P_QBuf + TailOfs, p_tail, tailLen);
		P_QHead->bufLen += tailLen;
		if(pInitKey) {
			memcpy(_Key_, pInitKey, BTRMAXKEYLEN);
			sp = initSpMode;
		}
		else
			sp = (initSpMode >= 0) ? spMode : UNDEF;
		if(sp != (int)UNDEF && !P_Tbl->search(_Key_, sp)) {
			if(BTRNFOUND)
				State |= stEOF;
			ok = -1;
		}
		else {
			State &= ~stPosSaved; // Не следует пытаться восстанавливать позицию
			ok = _search(((spMode == spFirst) ? spNext : spPrev), (Index_ < 0) ? EG_sign : UC_sign);
		}
	}
	CATCHZOK
	SAlloc::F(p_tail);
	return ok;
}

int FASTCALL BExtQuery::_search(int spMode, uint16 signature)
{
__again:
	int    r = 0;
	ActCount = 0;
	if(State & stSqlProvider) {
		if(P_Stmt) {
			const DbProvider * p_dbs = P_Tbl->GetDb();
			if(p_dbs->GetCapability() & DbProvider::cDirectSelectDataMapping) {
				assert(MaxRecs == 1); // Установлено в конструкторе BExtQuery
				{
					uint    local_actual_count = 0;
					r = P_Stmt->Fetch(1, &local_actual_count);
					if(r > 0) {
						/* @v12.4.2 это, кажись, не надо - функция Fetch уже получила данные //
						if(ActCount) {
							THROW(P_Stmt->GetData(0));
						}*/
						ActCount += local_actual_count;
					}
					else {
						BtrError = BE_EOF;
						r = -1;
					}
				}
			}
			else {
				r = P_Stmt->Fetch(MaxRecs, &ActCount);
				if(r > 0) {
					if(ActCount)
						if(!P_Stmt->GetData(0))
							r = 0;
				}
			}
		}
	}
	else {
		P_QHead->signature = signature;
		//
		// Распределяем память для данных. Размер буфера кратен максимальному числу
		// извлекаемых за один раз записей.
		//
		uint   s = sizeof(BExtResultHeader) + (sizeof(BExtResultItem) + RecSize) * MaxRecs;
		SETMAX(s, P_QHead->bufLen);
		Buf.Alloc(s);
		if(Buf.IsValid()) {
			reinterpret_cast<BExtTail *>(P_QBuf + TailOfs)->numRecs = MaxRecs;
			SBaseBuffer saved_buf = P_Tbl->getBuffer();
			//
			// Если до этого текущая позиция записи была сохранена, то восстанавливаем ее
			//
			if(State & stPosSaved && !P_Tbl->getDirect(Index_, 0, Position) && BtrError != BE_UBUFLEN) {
				P_Tbl->SetDBuf(saved_buf);
				r = 0;
			}
			else {
				memcpy(Buf.vptr(), P_QBuf, P_QHead->bufLen);
				P_Tbl->SetDBuf(Buf.vptr(), static_cast<RECORDSIZE>(Buf.GetSize()));
				if(Index_ < 0) {
					r = P_Tbl->stepExtended(spMode);
				}
				else {
					P_Tbl->setIndex(Index_);
					r = P_Tbl->getExtended(_Key_, spMode);
				}
				P_Tbl->SetDBuf(saved_buf);
				ActCount = *reinterpret_cast<const uint16 *>(Buf.cptr());
				State &= ~stRejectLimit;
				if(!r) {
					if(BTRNFOUND) {
						State |= stEOF;
						r = -1;
					}
					else if(BtrError == BE_REJECTLIMIT) {
						if(ActCount == 0) {
							P_QHead->signature = EG_sign;
							State &= ~stPosSaved;
							goto __again;
						}
						else {
							State |= stRejectLimit;
							r = 1;
						}
					}
				}
				const int saved_err = BtrError;
				//
				// Сохраняем текущую позицию записи
				//
				if(P_Tbl->getPosition(&Position)) {
					State |= stPosSaved;
					BtrError = saved_err;
				}
				else {
					State &= ~stPosSaved;
					r = 0;
				}
			}
		}
		else
			r = (BtrError = BE_NOMEM, 0);
	}
	return r;
}

void BExtQuery::setMaxReject(uint maxRej) { MaxReject = static_cast<uint16>(maxRej); }

BExtQuery & BExtQuery::selectAll()
{
	Fields.Destroy();
	return *this;
}

BExtQuery & BExtQuery::select(const DBFieldList & s)
{
	Fields = s;
	return *this;
}

BExtQuery & CDECL BExtQuery::select(DBField first_arg, ...)
{
	va_list list;
	DBField f;
	list = reinterpret_cast<va_list>(&first_arg);
	Fields.Destroy();
	while((f = va_arg(list, DBField)).Id != 0)
		Fields.Add(f);
	va_end(list);
	return *this;
}

int BExtQuery::addField(const DBField & rFld) { return Fields.Add(rFld); }

void FASTCALL BExtQuery::where(DBQ & q)
{
	if(&q) {
		P_Restrict = &q;
		P_Tree = P_Restrict->tree;
	}
}

static inline uint FASTCALL sizeofterm(const BExtTerm * term) { return (sizeof(BExtTerm) + term->fldLen); }

int FASTCALL _invertComp(int cmp);

int FASTCALL BExtQuery::add_term(int link, int n)
{
	int    ok = 0;
	DBQ::T & t = P_Restrict->items[n];
	const  int tbl_hdl = P_Tbl->GetHandle();
	assert(t.left.GetId() == tbl_hdl || t.right.GetId() == tbl_hdl);
	if(t.left.GetId() == tbl_hdl || t.right.GetId() == tbl_hdl) {
		int    cmp;
		char   val[1024];
		const  BNField * fld = 0;
		if(t.left.GetId() == tbl_hdl) {
			fld = &t.left.F.getField();
			cmp = t.cmp;
			t.right.getValue(fld->T, val);
		}
		else /*if(t.right.GetId() == tbl_hdl)*/ {
			fld = &t.right.F.getField();
			cmp = _invertComp(t.cmp);
			t.left.getValue(fld->T, val);
		}
		const uint16 sz = static_cast<uint16>(fld->size());
		P_QBuf = static_cast<char *>(SAlloc::R(P_QBuf, P_QHead->bufLen + sizeof(BExtTerm) + sz));
		if(P_QBuf) {
			BExtTerm * p_term = reinterpret_cast<BExtTerm *>(P_QBuf + P_QHead->bufLen);
			p_term->fldType = SLib2BtrType(GETSTYPE(fld->T));
			p_term->fldLen  = sz;
			p_term->fldOfs  = static_cast<uint16>(fld->Offs);
			p_term->cmp     = SlToBeqOp(cmp);
			p_term->link    = 0;
			memcpy(p_term+1, val, sz);
			/*
			p_term = get_term(P_QHead->numTerms-1);
			if(p_term)
				p_term->link = link;
			*/
			//BExtTerm * BExtQuery::get_term(int n)
			if(P_QHead->numTerms > 0) {
				const  uint _n = (P_QHead->numTerms-1);
				BExtHeader * p_h = reinterpret_cast<BExtHeader *>(P_QBuf);
				if(p_h && _n < p_h->numTerms) {
					p_term = reinterpret_cast<BExtTerm *>(p_h+1);
					for(uint i = 0; i < _n; i++) {
						p_term = reinterpret_cast<BExtTerm *>(PTR8(p_term) + sizeofterm(p_term));
					}
					p_term->link = SlToBeqLink(link);
				}
			}
			P_QHead->numTerms++;
			P_QHead->bufLen += (sizeof(BExtTerm) + sz);
			ok = 1;
		}
		else {
			BtrError = BE_NOMEM;
		}
	}
	return ok;
}

int FASTCALL BExtQuery::add_tree(int link, int n)
{
	int    ok = 1;
	if(P_Restrict && P_Tree) {
		const DBTree::T * t = &P_Tree->P_Items[n];
		if(t->link == 0) {
			ok = add_term(link, t->term);
		}
		else {
			ok = add_tree(link, t->left); // @recursion
			if(ok)
				ok = add_tree(t->link, t->right); // @recursion
		}
	}
	return ok;
}

const char * BExtQuery::getRecImage()
{
	char * p_ret = 0;
	if(Buf.IsValid()) {
		const  uint n = Cur;
		if(State & stSqlProvider) {
			if(P_Stmt && n < ActCount) {
				P_Stmt->GetData(n);
				p_ret = PTRCHR(Buf.vptr());
			}
		}
		else {
			if(n < *reinterpret_cast<const uint16 *>(Buf.cptr())) {
				size_t offs = sizeof(BExtResultHeader);
				if(State & stHasNote) {
					p_ret = PTRCHR(Buf.vptr(offs));
					uint i = n;
					if(i) {
						do {
							offs += reinterpret_cast<const BExtResultItem *>(p_ret)->recLen + sizeof(BExtResultItem);
							p_ret = PTRCHR(Buf.vptr(offs));
						} while(--i);
					}
				}
				else
					p_ret = PTRCHR(Buf.vptr(offs)) + (RecSize + sizeof(BExtResultItem)) * n;
			}
		}
	}
	return p_ret;
}

int FASTCALL BExtQuery::getRecPosition(DBRowId * pPos)
{
	const BExtResultItem * ri = reinterpret_cast<const BExtResultItem *>(getRecImage());
	if(ri) {
		ASSIGN_PTR(pPos, ri->position);
		return 1;
	}
	else
		return 0;
}

int BExtQuery::fillTblBuf()
{
	const char * p = getRecImage();
	char * b = P_Tbl ? static_cast<char *>(P_Tbl->getDataBuf()) : 0;
	if(p && b) {
		size_t rs = 0;
		if(!(State & stSqlProvider)) {
			if(State & stHasNote)
				rs = reinterpret_cast<const BExtResultItem *>(p)->recLen;
			p += sizeof(BExtResultItem);
		}
		const  uint c = Fields.GetCount();
		if(c) {
			for(uint i = 0; i < c; i++) {
				const  BNField & f = Fields.GetField(i);
				const  uint s = f.size();
				if(s == sizeof(uint32))
					*reinterpret_cast<uint32 *>(b + f.Offs) = *reinterpret_cast<const uint32 *>(p);
				else if(s == (sizeof(uint32) * 2)) {
					*reinterpret_cast<uint32 *>(b + f.Offs) = *reinterpret_cast<const uint32 *>(p);
					*reinterpret_cast<uint32 *>(b + f.Offs + sizeof(uint32)) = *reinterpret_cast<const uint32 *>(p + sizeof(uint32));
				}
				else
					memcpy(b + f.Offs, p, s);
				if(rs && GETSTYPE(f.T) == S_NOTE) {
					if(rs < RecSize) {
						PTR8(b+f.Offs)[s-RecSize+rs]=0;
					}
				}
				p += s;
			}
		}
		else {
			memcpy(b, p, RecSize);
			// @todo Обработать поля переменной длины
		}
		return 1;
	}
	return 0;
}
