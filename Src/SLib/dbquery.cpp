// DBQUERY.CPP
// Copyright (c) Sobolev A. 1995, 1996-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

DBQuery::DBQuery()
{
	THISZERO();
	options = fDestroyTables;
}

DBQuery::~DBQuery()
{
	uint   i;
	if(w_restrict) {
		w_restrict->destroy(1);
		delete w_restrict;
	}
	/* @v12.4.4 @unused 
	if(h_restrict) {
		h_restrict->destroy(0);
		delete h_restrict;
	}*/
	delete P_Frame;
	if(tbls) {
		const bool do_destroy_tbl = LOGIC(options & fDestroyTables);
		for(i = 0; i < tblCount; i++) {
			Tbl  & r_t = tbls[i];
			// @v12.4.6 if((t->flg & DBQTF_OVRRD_DESTROY_TAG) ? !do_destroy_tbl : do_destroy_tbl)
			if(do_destroy_tbl) {
				delete r_t.P_Tbl;
			}
			r_t.tree.destroy();
			r_t.key.destroy();
			// @v12.4.5 delete r_t.keyBuf;
		}
		SAlloc::F(tbls);
	}
	if(flds) {
		i = fldCount;
		if(i) do {
			flds[--i].cell.I.destroy();
		} while(i);
		SAlloc::F(flds);
	}
	SAlloc::F(order);
}

void DBQuery::setDestroyTablesMode(int set)
{
	SETFLAG(options, DBQuery::fDestroyTables, set);
}

void DBQuery::setSearchForUpdateMode(int set)
{
	for(uint i = 0; i < tblCount; i++) {
		SETFLAG(tbls[i].flg, DBQTF_SEARCHFORUPDATE, set);
	}
}

static int FASTCALL _search_item(int count, const int * pArray, int item)
{
	for(int i = 0; i < count; i++)
		if(pArray[i] == item)
			return i;
	return -1;
}

int DBQuery::_max_hdl(DBDataCell * c, const int * pList, int _max)
{
	if(c->GetId() > 0) {
		const int tmp = _search_item(tblCount, pList, c->GetId());
		_max = smax(_max, tmp);
	}
	else if(c->GetId() == DBE_ID) {
		for(int i = 0; i < c->E.Count; i++) {
			const int h = c->E.getTblHandle(i);
			if(h > 0) {
				const int tmp = _search_item(tblCount, pList, h);
				_max = smax(_max, tmp);
			}
		}
	}
	return _max;
}

void DBQuery::arrangeTerms()
{
	int   list[32];
	assert(tblCount <= sizeof(list));
	uint  n;
	for(n = 0; n < tblCount; n++)
		list[n] = tbls[n].P_Tbl->GetHandle();
	for(n = 0; n < w_restrict->count; n++) {
		DBQ::T * t = &w_restrict->items[n];
		t->TblIdx = _max_hdl(&t->right, list, _max_hdl(&t->left, list, -1));
		if(t->flags & DBQ_OUTER && t->TblIdx >= 0)
			tbls[t->TblIdx].flg |= DBQTF_OUTER_JOIN;
	}
}
//
#define BIND                      1
#define NOTBIND                   2
//
#define TEST_ONLY                 1
#define GET_ALL                   2
//
#define INVALID_DBTREE_ITEMS_LINK 0
//
//	Return: 0 - error, 1 - bind, 2 - not bind
//
int DBQuery::makeNode(int tblN, int * pNode, int option, int * pPos)
{
	int    result = 0;
	if(*pNode == -1) {
		if(option != TEST_ONLY)
			*pPos = -1;
		result = NOTBIND;
	}
	else {
		int    left_child;
		int    right_child;
		int    lv;
		int    rv;
		int    lp;
		int    rp;
		DBTree * p_tree = &tbls[tblN].tree;
		DBTree::T & t = w_restrict->tree->P_Items[*pNode];
		if(t.link == 0) // Leaf
			if(w_restrict->items[t.term].TblIdx == tblN) {
				if(option != TEST_ONLY) {
					result = p_tree->addLeaf(t.term, t.flags, pPos) ? BIND : 0;
					*pNode = -1;
				}
				else
					result = BIND;
			}
			else if(option == GET_ALL) {
				result = p_tree->addLeaf(t.term, t.flags, pPos) ? NOTBIND : 0;
				*pNode = -1;
			}
			else
				result = NOTBIND;
		else {
			left_child  = t.left;
			right_child = t.right;
			if(t.link == _AND___) {
				lv = makeNode(tblN, &left_child,  option, &lp);
				rv = makeNode(tblN, &right_child, option, &rp);
				if(lv && rv) {
					if(lv == rv)
						if(lv == NOTBIND)
							if(option == GET_ALL)
								result = p_tree->addNode(t.link, lp, rp, pPos) ? NOTBIND : 0;
							else
								result = NOTBIND;
						else if(option != TEST_ONLY)
							result = p_tree->addNode(t.link, lp, rp, pPos) ? BIND : 0;
						else
							result = BIND;
					else if(option == GET_ALL)
						result = p_tree->addNode(t.link, lp, rp, pPos) ? BIND : 0;
					else if(option == TEST_ONLY)
						result = BIND;
					else {
						*pPos = (lv == BIND) ? lp : rp;
						result = BIND;
					}
				}
			}
			else if(t.link == _OR___) {
				lv = makeNode(tblN, &left_child,  TEST_ONLY, &lp);
				rv = makeNode(tblN, &right_child, TEST_ONLY, &rp);
				if(lv && rv) {
					if(lv == BIND || rv == BIND) {
						if(option != TEST_ONLY) {
							lv = makeNode(tblN, &left_child,  GET_ALL, &lp);
							rv = makeNode(tblN, &right_child, GET_ALL, &rp);
							if(lv && rv)
								result = p_tree->addNode(t.link, lp, rp, pPos) ? BIND : 0;
						}
						else
							result = BIND;
					}
					else
						result = NOTBIND;
				}
			}
			else
				;//CHECK(INVALID_DBTREE_ITEMS_LINK);
			if(option != TEST_ONLY) {
				t.left  = left_child;
				t.right = right_child;
				if(left_child == -1)
					*pNode = right_child;
				else if(right_child == -1)
					*pNode = left_child;
			}
		}
	}
	return result;
}

int FASTCALL DBQuery::chooseKey(int tblN)
{
	Tbl  & t = tbls[tblN];
	int    seg;
	if(t.key.P_h) { // Ключ хоть и инициализирован, но является динамическим
		if(t.key.P_h->pseg & DYN_KEY) {
			t.key.trim();
			t.key.P_h->pseg = 0;
			seg = 0;
			do {
				if(!t.tree.chooseKey(t.tree.Root, t.P_Tbl->GetHandle(), seg, &t.key, 0))
					return 0;
			} while((t.key.P_h->pseg & ~DYN_KEY) == ++seg);
		}
	}
	else {
		int    ka_c = 0; // Количество допустимых индексов
		int  * p_ka = 0; // Массив допустимых индексов
		const  BNKeyList & r_indices = t.P_Tbl->GetIndices();
		const  uint num_keys = r_indices.getNumKeys();
		if(tblN == 0) {
			p_ka = static_cast<int *>(SAlloc::M(num_keys * sizeof(int)));
			if(p_ka)
				ka_c = analyzeOrder(p_ka);
			else
				return 0;
		}
		uint   i;
		uint   tc = (t.tree.Count+1) * sizeof(uint);
		uint * p_trace = static_cast<uint *>(SAlloc::M(tc));
		uint * p_best_trace = static_cast<uint *>(SAlloc::M(tc));
		if(p_trace == 0 || p_best_trace == 0)
			return 0;
		p_best_trace[0] = 0;
		for(i = 0; i < num_keys; i++) {
			bool   use_index = true;
			if(ka_c) {
				use_index = false;
				for(int j = 0; !use_index && j < ka_c; j++)
					if(p_ka[j] == i)
						use_index = true;
			}
			if(use_index) {
				p_trace[0] = 0;
				KR     k(t.P_Tbl->GetHandle(), i);
				seg = 0;
				do {
					if(!t.tree.chooseKey(t.tree.Root, t.P_Tbl->GetHandle(), seg, &k, p_trace))
						return (k.destroy(), 0);
				} while((k.P_h->pseg & ~DYN_KEY) == ++seg);
				const int cm = compare(t.key, k);
				if(cm < 0 || (cm == 0 && t.key.P_h == 0)) {
					t.key.copy(k);
					memcpy(p_best_trace, p_trace, sizeof(int) * (p_trace[0]+1));
				}
				k.destroy();
			}
		}
		SAlloc::F(p_ka);
		SAlloc::F(p_trace);
		for(i = 1; i <= p_best_trace[0]; i++) {
			uint * f = &w_restrict->items[p_best_trace[i]].flags;
			(*f) &= ~DBQ_LOGIC;
			/* @v6.1.1 Этот флаг блокирует проверку условий, которые, как оказалось, не всегда истинны
			(*f) |=  DBQ_TRUE;
			*/
		}
		SAlloc::F(p_best_trace);
		t.P_Tbl->setIndex(r_indices[t.key.P_h->keyNum].getKeyNumber());
		/* @v12.4.5 
		t.keyBuf = new char[BTRMAXKEYLEN];
		if(t.keyBuf == 0)
			return 0;
		*/
	}
	return 1;
}
//
// Если с помощью существующих индексов можно построить заданную
// сортировку, то возвращает (>0). Иначе возвращает 0 и устанавливает
// либо флаг статуса s_tmp_table_needed, либо s_add_index_needed.
// Если OK, то возвращает количество индексов, которые можно использовать,
// а по указателю pKeyArray (если pKeyArray != 0) записывает номера
// позиций допустимых индексов (Этои индексы принадлежат первой
// таблице из указанных в запросе).
// Если сортировка вообще не задана, то возвращается 0. Отличить
// этот случай от отсутствия подходящих индексов проще всего
// по равенству нулю поля DBQuery::ordCount. Можно и по установке
// флагов s_tmp_table_needed и s_add_index_needed в поле DBQuery::status.
//
int FASTCALL DBQuery::analyzeOrder(int * pKeyArray)
{
	uint   count = 0;
	if(ordCount) {
		uint   i;
		const  uint num_keys = tbls[0].P_Tbl->GetIndices().getNumKeys();
		for(i = 0; i < ordCount; i++) {
			if(order[i].getTable() != tbls[0].P_Tbl) {
				status |= s_tmp_table_needed;
				return 0;
			}
		}
		for(i = 0; i < num_keys; i++) {
			int    ok = 1;
			const  BNKey key = tbls[0].P_Tbl->GetIndices()[i];
			for(uint j = 0; j < static_cast<uint>(key.getNumSeg()) && j < ordCount; j++) {
				if(order[j].fld != key.getFieldID(j)) {
					ok = 0;
					break;
				}
			}
			if(ok) {
				if(pKeyArray)
					pKeyArray[count] = i;
				count++;
			}
		}
		if(!count)
			status |= s_add_index_needed;
	}
	return count;
}

int FASTCALL DBQuery::addField(DBConst & r)
{
	flds = static_cast<DBQuery::Fld *>(SAlloc::R(flds, sizeof(DBQuery::Fld) * (fldCount+1)));
	if(flds) {
		flds[fldCount].cell.C = r;
		flds[fldCount].type = static_cast<const DBItem *>(&r)->stype();
		fldCount++;
		calcRecSize();
		return 1;
	}
	else
		return 0;
}

int FASTCALL DBQuery::addField(DBE & r)
{
	flds = static_cast<DBQuery::Fld *>(SAlloc::R(flds, sizeof(DBQuery::Fld) * (fldCount+1)));
	if(flds) {
		flds[fldCount].cell.E = r;
		flds[fldCount].type = static_cast<const DBItem *>(&r)->stype();
		fldCount++;
		calcRecSize();
		return 1;
	}
	else
		return 0;
}

int FASTCALL DBQuery::addField(const DBField & r)
{
	flds = static_cast<DBQuery::Fld *>(SAlloc::R(flds, sizeof(DBQuery::Fld) * (fldCount+1)));
	if(flds) {
		flds[fldCount].cell.F = r;
		flds[fldCount].type = static_cast<const DBItem *>(&r)->stype();
		fldCount++;
		calcRecSize();
		return 1;
	}
	else
		return 0;
}

DBQuery & FASTCALL select(const DBFieldList & list)
{
	DBQuery & q = *new DBQuery;
	if(&q != 0) {
		if((q.flds = static_cast<DBQuery::Fld *>(SAlloc::M(sizeof(DBQuery::Fld) * list.GetCount()))) != 0) {
			for(uint i = 0; i < list.GetCount(); i++) {
				DBQuery::Fld & fld = q.flds[i];
				fld.cell.F = list.Get(i);
				fld.type   = list.Get(i).stype();
			}
			q.fldCount = list.GetCount();
		}
		else {
			q.error = 1;
			q.fldCount = 0;
		}
		q.syntax |= DBQuery::tSelect;
	}
	return q;
}

DBQuery & FASTCALL selectbycell(int count, const DBDataCell * pList)
{
	DBQuery & q = *new DBQuery;
	if(&q != 0) {
		if((q.flds = static_cast<DBQuery::Fld *>(SAlloc::M(sizeof(DBQuery::Fld) * count))) != 0)
			for(int i = 0; i < count; i++) {
				DBQuery::Fld & fld = q.flds[i];
				fld.cell = pList[i];
				fld.type = pList[i].I.stype();
			}
		else {
			q.error = 1;
			count   = 0;
		}
		q.fldCount = count;
		q.syntax |= DBQuery::tSelect;
	}
	return q;
}

DBQuery & CDECL select(DBField first_arg, ...)
{
	va_list list = reinterpret_cast<va_list>(&first_arg);
	DBQuery & q = *new DBQuery;
	if(&q != 0) {
		int  id;
		int  i = 0;
		size_t delta = 0;
		while((id = *reinterpret_cast<const int *>(list)) != 0) {
			q.flds = static_cast<DBQuery::Fld *>(SAlloc::R(q.flds, sizeof(DBQuery::Fld) * (i+1)));
			if(q.flds == 0) {
				i = 0;
				break;
			}
			DBQuery::Fld & fld = q.flds[i];
			if(id == DBConst_ID) {
				delta = sizeof(DBConst);
				fld.cell.C = *reinterpret_cast<const DBConst *>(list);
			}
			else if(id == DBE_ID) {
				delta = sizeof(DBE);
				fld.cell.E = *reinterpret_cast<const DBE *>(list);
			}
			else if(id > 0) {
				delta = sizeof(DBField);
				fld.cell.F = *reinterpret_cast<const DBField *>(list);
			}
			else {
				;//CHECK(Invalid_DBItem);
			}
			fld.type = reinterpret_cast<const DBItem *>(list)->stype();
			list = reinterpret_cast<va_list>(PTR8(list) + delta);
			i++;
		}
		q.fldCount = i;
		q.syntax |= DBQuery::tSelect;
	}
	va_end(list);
	return q;
}

DBQuery & selectAll()
{
	DBQuery & q = *new DBQuery;
	q.syntax |= (DBQuery::tSelect | DBQuery::tAll);
	return q;
}

DBQuery & CDECL DBQuery::from(DBTable * first_arg, ...)
{
	uint   i = 0;
	DBTable * p_tbl;
	va_list  list = reinterpret_cast<va_list>(&first_arg);
	while((p_tbl = va_arg(list, DBTable*)) != 0) {
		tbls = static_cast<Tbl *>(SAlloc::R(tbls, sizeof(Tbl) * (i+1)));
		if(tbls) {
			tbls[i].P_Tbl = p_tbl;
			tbls[i].tree.init(0);
			tbls[i].key.P_b = 0;
			// @v12.4.5 tbls[i].keyBuf = 0;
			tbls[i].flg = 0;
			i++;
		}
		else {
			i = 0;
			break;
		}
	}
	tblCount = i;
	va_end(list);
	syntax |= tFrom;
	calcRecSize();
	return *this;
}

int FASTCALL DBQuery::addTable(DBTable * pTbl)
{
	int    ok = 1;
	if(pTbl) {
		uint   i = tblCount;
		tbls = static_cast<Tbl *>(SAlloc::R(tbls, sizeof(Tbl) * (i+1)));
		if(tbls != 0) {
			tbls[i].P_Tbl = pTbl;
			tbls[i].tree.init(0);
			tbls[i].key.P_b = 0;
			// @v12.4.5 tbls[i].keyBuf = 0;
			tbls[i].flg = 0;
			++tblCount;
			syntax |= tFrom;
			calcRecSize();
		}
		else {
			tblCount = 0;
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}

int DBQuery::addOrderField(const DBField & rFld)
{
	if(&rFld) {
		order = static_cast<DBField *>(SAlloc::R(order, sizeof(DBField) * (ordCount+1)));
		order[ordCount++] = rFld;
		return 1;
	}
	else
		return 0;
}

DBQuery & CDECL DBQuery::orderBy(DBField first_arg, ...)
{
	va_list  list = reinterpret_cast<va_list>(&first_arg);
	ordCount = 0;
	DBField f;
	while((f = va_arg(list, DBField)).Id != 0) {
		order = static_cast<DBField *>(SAlloc::R(order, sizeof(DBField) * (ordCount+1)));
		if(order == 0) {
			ordCount = 0;
			break;
		}
		order[ordCount++] = f;
	}
	va_end(list);
	syntax |= tOrder;
	analyzeOrder(0);
	return *this;
}

int DBQuery::checkWhereRestriction()
{
	return TRUE;
}

DBQuery & FASTCALL DBQuery::where(DBQ & q)
{
	if(&q != 0) {
		const bool is_first = !(syntax & tWhere);
		if(w_restrict) {
			w_restrict->destroy(1);
			delete w_restrict;
		}
		w_restrict = &q;
		arrangeTerms();
		for(int i = tblCount-1; i >= 0; i--) {
			if(!is_first) {
				tbls[i].tree.destroy();
				tbls[i].key.destroy();
				/* @v12.4.5 if(tbls[i].keyBuf) {
					delete tbls[i].keyBuf;
					tbls[i].keyBuf = 0;
				}*/
			}
			tbls[i].tree.init(w_restrict);
			if(!makeNode(i, &w_restrict->tree->Root, 0, &tbls[i].tree.Root)) {
				error = 1;
				break;
			}
		}
		syntax |= tWhere;
	}
	return *this;
}

int DBQuery::calcRecSize()
{
	recSize  = 0;
	if(syntax & tAll) {
		for(uint i = 0; i < tblCount;) {
			const RECORDSIZE rs = tbls[i++].P_Tbl->getRecSize();
			recSize += rs;
		}
	}
	else {
		for(uint i = 0; i < fldCount;)
			recSize += static_cast<RECORDSIZE>(stsize(flds[i++].type));
	}
	return 1;
}

int DBQuery::getFieldPosByName(const char * pFldName, uint * pPos) const
{
	for(uint i = 0; i < fldCount; i++)
		if(flds[i].cell.GetId() > 0 && sstreqi_ascii(flds[i].cell.F.getField().Name, pFldName)) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	ASSIGN_PTR(pPos, 0);
	return 0;
}

DBQuery::Tbl::Tbl() : P_Tbl(0), /*keyBuf(0)*/flg(0)
{
	memzero(KeyBuf, sizeof(KeyBuf));
}

int DBQuery::Tbl::Srch(void * pKey, int sp)
{
	int    ok = (flg & DBQTF_SEARCHFORUPDATE) ? P_Tbl->searchForUpdate(pKey, sp) : P_Tbl->search(pKey, sp);
	if(ok) {
		// @v12.4.6 {
		const DBRowId * p_rowid = P_Tbl->getCurRowIdPtr();
		if(p_rowid) {
			Pos_ = *p_rowid;
		}
		// } @v12.4.6 
	}
	else if(BtrError == BE_INVPOS)
		BtrError = BE_EOF;
	return ok;
}

int DBQuery::_search(uint n, int dir)
{
	Tbl  & r_tbl = tbls[n];
	int    outer_rec = 0;
	int    sp;
	const  bool is_first = (dir == spFirst || dir == spLast);
	int    more; // Флаг, предписывающий обновить значение ключа
	if(is_first) {
		r_tbl.flg &= ~(DBQTF_PREVDOWN | DBQTF_PREVUP);
		r_tbl.P_Tbl->ToggleStmt(true/*release*/);
		if(chooseKey(n)) {
			if(dir == spFirst)
				r_tbl.key.first();
			else
				r_tbl.key.last();
			more = 1;
			dir  = (dir == spFirst) ? spNext : spPrev;
		}
		else {
			error = 1;
			return 0;
		}
	}
	else {
		if((n+1) < tblCount) {
			if(_search(n+1, dir)) // @recursion
				return 1;
			else if(error)
				return 0;
		}
		// @todo Этот участок кода блокирует выбор второй и далее записей из таблиц, следующих
		// за неизменной записью первой таблицы.
		// Выбрать момент, когда можно протестировать и убрать этот участок. {
		else {
			if(!(options & fCorrectSearchMoreProblem)) {
				if(tblCount > 1)
					return 0;
			}
		}
		// }
		more = 0;
	}
	do {
		sp = dir;
		if(dir == spNext) {
			if(r_tbl.flg & DBQTF_PREVUP) {
				r_tbl.P_Tbl->ToggleStmt(0);
				r_tbl.flg &= ~DBQTF_PREVUP;
			}
			r_tbl.flg |= DBQTF_PREVDOWN;
		}
		else if(dir == spPrev) {
			if(r_tbl.flg & DBQTF_PREVDOWN) {
				r_tbl.P_Tbl->ToggleStmt(0);
				r_tbl.flg &= ~DBQTF_PREVDOWN;
			}
			r_tbl.flg |= DBQTF_PREVUP;
		}
		if(more) {
			r_tbl.flg &= ~ONLY_ONE_REC;
			r_tbl.key.getKey(r_tbl.KeyBuf, &sp);
			if(sp & SKIP_ONE_REC) {
				sp &= ~SKIP_ONE_REC;
			   	if(!r_tbl.Srch(r_tbl.KeyBuf, sp) && !BTRNFOUND)
					error = 1;
				sp = dir;
			}
			if(sp & ONLY_ONE_REC) {
				sp &= ~ONLY_ONE_REC;
				r_tbl.flg |= ONLY_ONE_REC;
			}
		}
		else if(r_tbl.flg & ONLY_ONE_REC) {
			r_tbl.flg &= ~ONLY_ONE_REC;
			return 0;
		}
		while(!error && r_tbl.Srch(r_tbl.KeyBuf, sp) && r_tbl.key.checkKey(r_tbl.KeyBuf)) {
			if(r_tbl.tree.checkRestriction()) {
__get_next_tbl:
				if((n+1 == tblCount || _search(n+1, (dir == spNext ? spFirst : spLast)))) { // @recursion
					if(options & fSavePositions) {
						r_tbl.P_Tbl->getPosition(&r_tbl.Pos_);
					}
					return 1;
				}
				else if(outer_rec)
					return 0;
			}
			sp = dir;
		}
		if(!btrokornfound())
			error = 1;
	} while((more = error ? 0 : ((dir == spNext) ? ++r_tbl.key : --r_tbl.key)) != 0);
	if(!error && (r_tbl.flg & DBQTF_OUTER_JOIN) && is_first) {
		outer_rec = 1;
		r_tbl.P_Tbl->clearDataBuf();
		goto __get_next_tbl;
	}
	return 0;
}

void DBQuery::fillRecord(char * pBuf, /*RECORDNUMBER*/DBRowId * pPos)
{
	RECORDSIZE p = 0;
	if(pBuf) {
		if(syntax & tAll) {
			for(uint i = 0; i < tblCount; i++) {
				const RECORDSIZE s = tbls[i].P_Tbl->GetFields().CalculateFixedRecSize();
				memcpy(pBuf+p, tbls[i].P_Tbl->getDataBufConst(), s);
				p += s;
			}
		}
		else {
			for(uint i = 0; i < fldCount; i++) {
				const TYPEID t = flds[i].type;
				flds[i].cell.getValue(t, pBuf+p);
				p += static_cast<RECORDSIZE>(stsize(t));
			}
		}
	}
	if(pPos && options & fSavePositions) {
		for(uint i = 0; i < tblCount; i++)
			pPos[i] = tbls[i].Pos_;
	}
}

#define INVALID_DIRECTION_CODE 0

int DBQuery::single_fetch(char * pBuf, /*RECORDNUMBER*/DBRowId * pPos, int dir)
{
	if(_search(0, dir)) {
		fillRecord(pBuf, pPos);
		ActualCount = 1;
	}
	else
		ActualCount = 0;
	return static_cast<int>(ActualCount);
}

int DBQuery::fetch(uint count, char * pBuf, /*RECORDNUMBER*/DBRowId * pPos, int dir)
{
	uint   rec_no = 0L;
	assert(oneof4(dir, spNext, spFirst, spPrev, spLast));
	const int next = (dir == spPrev || dir == spLast) ? spPrev : spNext;
	if(_search(0, dir)) {
		do {
			if(pBuf)
				fillRecord(pBuf + recSize * rec_no, pPos ? (pPos + tblCount * rec_no) : 0);
		} while(++rec_no < count && _search(0, next));
	}
	ActualCount = rec_no;
	if(next == spPrev && pBuf && rec_no > 1 && !(options & fFetchReverse)) {
		for(uint j = 0; j < (rec_no >> 1); j++)
			memswap(pBuf + j * recSize, pBuf + (rec_no - j - 1) * recSize, recSize);
	}
	return (rec_no < count) ? 0 : 1;
}

int DBQuery::fetch(uint count, char * pBuf, int dir) { return fetch(count, pBuf, 0, dir); }

//#define BOQ  (P_Frame->State & Frame::stTop)
//#define EOQ  (P_Frame->State & Frame::stBottom)

long DBQuery::_defaultBufSize  = 64L;
long DBQuery::_defaultBufDelta =  4L;

inline uint FASTCALL DBQuery::addr(uint p) const { return ((P_Frame->Zero+p) % P_Frame->Size); }
void * FASTCALL DBQuery::getRecord(uint r) { return (r < P_Frame->Count) ? (P_Frame->P_Buf + recSize * addr(r)) : 0; }
const void * FASTCALL DBQuery::getRecordC(uint r) const { return (r < P_Frame->Count) ? (P_Frame->P_Buf + recSize * addr(r)) : 0; }
void * DBQuery::getCurrent() { return getRecord(P_Frame->Cur); }
void * DBQuery::getBuffer() { return P_Frame->P_Buf; }

int DBQuery::setFrame(uint viewHight, uint bufSize, uint bufDelta)
{
	int    ok = 1;
	if(bufSize != UNDEF && !allocFrame(bufSize))
		ok = 0;
	else {
		if(P_Frame) {
			if(bufDelta != UNDEF)
				P_Frame->Inc = bufDelta;
			if(viewHight != UNDEF) {
				P_Frame->Height = smin(viewHight, P_Frame->Size);
				if(P_Frame->Cur != P_Frame->Top ||/*P_Frame->Top < 0 ||*/P_Frame->Top > P_Frame->Count) {
					if((P_Frame->Cur - P_Frame->Top) >= P_Frame->Height)
						P_Frame->Top = P_Frame->Cur-P_Frame->Height+1;
					else if(P_Frame->Cur < P_Frame->Height)
						P_Frame->Top = 0;
					else if((P_Frame->Count - P_Frame->Top) < P_Frame->Height)
						P_Frame->Top = (P_Frame->Count - P_Frame->Height);
				}
			}
		}
		options |= fSmartFrame;
		options &= ~fFetchReverse;
	}
	return ok;
}

int DBQuery::allocFrame(uint bufSize)
{
	int    ok = 1;
	if(!P_Frame || P_Frame->Size != bufSize) {
		if(bufSize) {
			delete P_Frame;
			P_Frame = new Frame(bufSize);
			THROW(P_Frame);
			P_Frame->P_Buf = static_cast<char *>(SAlloc::R(P_Frame->P_Buf, bufSize * recSize));
			THROW(P_Frame->P_Buf);
			P_Frame->Count = smin(P_Frame->Count, bufSize);
			if(options & fSavePositions) {
				P_Frame->P_PosBuf = static_cast<DBRowId *>(SAlloc::R(P_Frame->P_PosBuf, P_Frame->Size * tblCount * sizeof(DBRowId)));
				THROW(P_Frame->P_PosBuf);
			}
		}
		else {
			if(P_Frame) {
				ZFREE(P_Frame->P_Buf);
				ZFREE(P_Frame->P_PosBuf);
				// @v12.4.5 P_Frame->Size = 0;
			}
			options &= ~fSmartFrame;
		}
	}
	CATCH
		ZDELETE(P_Frame);
	ENDCATCH
	return ok;
}

void DBQuery::moveRec(uint rd, uint rs)
{
	/*ulong*/DBRowId * p_position = P_Frame->P_PosBuf;
	char * b = P_Frame->P_Buf;
	memmove(b + rd * recSize, b + rs * recSize, recSize);
	if(p_position) {
		uint ps = tblCount * sizeof(/*RECORDNUMBER*/DBRowId);
		memmove(p_position + rd * tblCount, p_position + rs * tblCount, ps);
	}
}

void DBQuery::moveBuf(uint dest, uint src, uint recs)
{
	if(dest > src) { // Копируем сверху-вниз
		for(int i = (recs-1); i >= 0; i--)
			moveRec(addr(dest+i), addr(src+i));
	}
	else { // Копируем снизу-вверх
		for(uint j = 0; j < recs; j++)
			moveRec(addr(dest+j), addr(src+j));
	}
}

void FASTCALL DBQuery::frameOnBottom(int undefSDelta)
{
	const uint c = P_Frame->Count;
	if(c) {
		const uint ht = smin(P_Frame->Height, c);
		P_Frame->SDelta = undefSDelta ? 0x8000 : (c - ht - P_Frame->Top);
		P_Frame->Cur    = c - 1;
		P_Frame->Top    = c - ht;
	}
	else {
		P_Frame->SDelta = 0;
		P_Frame->Cur = 0;
		P_Frame->Top = 0;
	}
}

void FASTCALL DBQuery::setCount(uint c)
{
	if(c) {
		if(P_Frame->Top >= c)
			P_Frame->Top = c-1;
		if(P_Frame->Cur >= c)
			P_Frame->Cur = c-1;
	}
	else {
		P_Frame->Top = 0;
		P_Frame->Cur = 0;
	}
	P_Frame->Count = c;
}

#define LBUFPTR(p) (P_Frame->P_Buf+addr(p)*recSize)
#define PBUFPTR(p) (P_Frame->P_Buf+(p)*recSize)
#define LPOSPTR(p) (P_Frame->P_PosBuf ? (P_Frame->P_PosBuf+addr(p)*tblCount) : 0)
//
// Состояния признака DBQuery::Frame::last
//
#define _lbottom  1
#define _lnext    2
#define _ltop     3
#define _lprev    4

int DBQuery::_fetch_next(uint count, uint p, int dir)
{
	uint i;
	for(i = 0; i < count; i++) {
		if(!single_fetch(LBUFPTR(p+i), LPOSPTR(p+i), i ? spNext : dir))
			break;
	}
	P_Frame->State = (dir == spFirst) ? Frame::stTop : Frame::stUndef;
	if(i < count) {
		P_Frame->Last = _lbottom;
		if(!error)
			P_Frame->State |= Frame::stBottom;
	}
	else
		P_Frame->Last = _lnext;
	ActualCount = i;
	return !error;
}

int DBQuery::_fetch_prev(uint count, uint p)
{
	uint  i;
	const uint s = P_Frame->Size;
	for(i = 0; i < count; i++) {
		if(!single_fetch(LBUFPTR(s+p-i), LPOSPTR(s+p-i), spPrev))
			break;
	}
	P_Frame->State = Frame::stUndef;
	if(i < count) {
		P_Frame->Last = _ltop;
		if(!error)
			P_Frame->State |= Frame::stTop;
	}
	else
		P_Frame->Last = _lprev;
	ActualCount = i;
	return !error;
}

int DBQuery::_fetch_last(uint count, uint p)
{
	uint   i;
	const  uint s = P_Frame->Size;
	for(i = 0; i < count; i++) {
		if(!single_fetch(LBUFPTR(s+p-i), LPOSPTR(s+p-i), i ? spPrev : spLast))
			break;
	}
	P_Frame->State = Frame::stBottom;
	if(i < count) {
		P_Frame->Last = _ltop;
		if(!error)
			P_Frame->State |= Frame::stTop;
	}
	else
		P_Frame->Last = _lprev;
	ActualCount = i;
	return !error;
}

#pragma warn -sig

int FASTCALL DBQuery::normalizeFrame(int dir)
{
	assert(dir == spNext || dir == spPrev);
	uint   recs;
	uint   pos;
	if(dir == spNext) {
		if(P_Frame->Last == _lprev) {
			recs = P_Frame->Count-1;
			pos = 1;
		}
		else if(P_Frame->Last == _ltop) {
			recs = P_Frame->Count-1;
			pos = 1;
		}
		else
			return 1;
		_fetch_next(recs, pos, spNext);
		if(ActualCount < recs)
			setCount(ActualCount);
	}
	else { // dir == spPrev
		if(P_Frame->Last == _lnext) {
			recs = P_Frame->Count-1;
			pos = P_Frame->Count-2;
		}
		else if(P_Frame->Last == _lbottom) {
			recs = P_Frame->Count-1;
			pos = P_Frame->Count-2;
		}
		else
			return 1;
		_fetch_prev(recs, pos);
		if(ActualCount < recs) {
			P_Frame->Zero = addr(P_Frame->Count-ActualCount);
			setCount(ActualCount);
		}
	}
	return !error;
}

int DBQuery::refresh()
{
	P_Frame->SDelta = 0;
	if(P_Frame->State & Frame::stTop) {
		_fetch_next(P_Frame->Size, 0, spFirst);
		setCount(ActualCount);
	}
	else if(P_Frame->State & Frame::stBottom) {
		const uint s = P_Frame->Size;
		_fetch_last(s, s-1);
		P_Frame->Zero = addr(s - ActualCount);
		setCount(ActualCount);
	}
	else if(P_Frame->Last == _ltop || P_Frame->Last == _lprev) {
		_fetch_prev(1, 0);
		_fetch_next(P_Frame->Count, 0, spNext);
		setCount(ActualCount);
	}
	else {
		normalizeFrame(spPrev);
		normalizeFrame(spNext);
	}
	return !error;
}

int DBQuery::top()
{
	if(P_Frame->State & Frame::stTop)
		P_Frame->SDelta = -static_cast<long>(P_Frame->Top);
	else {
		P_Frame->SDelta = 0x8000;
		_fetch_next(P_Frame->Size, 0, spFirst);
		P_Frame->Count = ActualCount;
	}
	P_Frame->Top = 0;
	P_Frame->Cur = 0;
	P_Frame->SPos = 0;
	return !error;
}

int DBQuery::bottom()
{
	if(!error) {
		if(!(P_Frame->State & Frame::stBottom)) {
			const uint s = P_Frame->Size;
			_fetch_last(s, s-1);
			P_Frame->Zero  = addr(s - ActualCount);
			P_Frame->Count = ActualCount;
		}
		frameOnBottom(!(P_Frame->State & Frame::stBottom));
		P_Frame->SPos = P_Frame->SRange;
	}
	return !error;
}

int DBQuery::step(long delta)
{
	int    result = 0;
	if(!error) {
		const uint ht = P_Frame->Height;
		if(delta == 0)
			result = 1;
		else {
			P_Frame->SDelta = 0;
			const  uint _cur = P_Frame->Cur;
			const  uint s = P_Frame->Size;
			uint   c = P_Frame->Count;
			if(delta > 0) {
				delta = smin(static_cast<uint>(delta), s);
				const int diff = (_cur+delta-c);
				if(diff >= 0) {
					if(!(P_Frame->State & Frame::stBottom)) {
						normalizeFrame(spNext);
						_fetch_next(diff+1, c = P_Frame->Count, spNext);
						const uint temp = c + ActualCount;
						if(temp > s)
							P_Frame->Zero = addr(temp % s);
						P_Frame->Count = smin(temp, s);
					}
					if(P_Frame->State & Frame::stBottom) {
						frameOnBottom(0);
						P_Frame->SPos = P_Frame->SRange;
						return error ? 0 : -1;
					}
				}
				P_Frame->Cur = smin(P_Frame->Count - 1, static_cast<uint>(P_Frame->Cur + delta));
				if(P_Frame->Cur >= (P_Frame->Top + ht)) {
					const uint temp = P_Frame->Cur - ht + 1;
					P_Frame->SDelta = temp - P_Frame->Top;
					P_Frame->Top    = temp;
				}
				//P_Frame->SPos += delta;
				P_Frame->SPos = P_Frame->SRange / 2;
			}
			else {
				delta = smin(static_cast<uint>(-delta), ht);
				if(P_Frame->Cur < (ulong)delta) {
					if(!(P_Frame->State & Frame::stTop)) {
						normalizeFrame(spPrev);
						const uint recs = delta - P_Frame->Cur;
						_fetch_prev(recs, s-1);
						P_Frame->Zero = addr(s - ActualCount % s);
						P_Frame->Top = P_Frame->Cur = 0;
						P_Frame->SDelta = -delta;
						//P_Frame->SPos -= delta;
						P_Frame->SPos = P_Frame->SRange / 2;
					}
					if(P_Frame->State & Frame::stTop) {
						if(P_Frame->Cur) {
							P_Frame->SDelta = -static_cast<long>(P_Frame->Top);
							P_Frame->Top = P_Frame->Cur = 0;
							P_Frame->SPos = 0;
						}
					}
					return !error;
				}
				P_Frame->Cur = P_Frame->Cur-delta;
				if(P_Frame->Cur < P_Frame->Top) {
					P_Frame->SDelta = P_Frame->Cur - P_Frame->Top;
					P_Frame->Top = P_Frame->Cur;
				}
				//P_Frame->SPos -= delta;
				P_Frame->SPos = P_Frame->SRange / 2;
			}
			P_Frame->SPos = smin(P_Frame->SRange, P_Frame->SPos);
			result = !error;
		}
	}
	return result;
}

#pragma warn .sig

DBQuery::Frame::Frame(uint sizeInRecs) : Size(sizeInRecs), P_Buf(0), P_PosBuf(0), Zero(0), State(stUndef), Inc(0),
	Height(0), Top(0), Cur(0), SDelta(0), SRange(0), SPos(0), Last(0), Count(0)
{
}

DBQuery::Frame::~Frame()
{
	SAlloc::F(P_Buf);
	SAlloc::F(P_PosBuf);
}

void FASTCALL DBQuery::Frame::TopByCur(int dir)
{
	if(!Count)
		Top = 0;
	else if(dir == spNext) {
		if(Cur >= Height) {
			Top = (Count < Height) ? 0 : smin(Cur, Count-Height);
		}
	}
	else
		Top = ((Cur+1) >= Height) ? (Cur+1-Height) : 0;
}

int DBQuery::search(const void * pPattern, CompFunc fcmp, int fld, uint srchMode, void * pExtraData)
{
	const  int dir = (srchMode & ~srchFlags);
	int    nxt;
	if(dir == srchFirst) {
		P_Frame->Zero = 0;
		top();
		nxt = spNext;
	}
	else if(dir == srchLast) {
		P_Frame->Zero = 0;
		bottom();
		nxt = spPrev;
	}
	else {
		step((dir == srchNext) ? 1 : -1);
		nxt = (dir == srchNext) ? spNext : spPrev;
	}
	const  int _eof = (nxt == spNext) ? Frame::stBottom : Frame::stTop;
	int    r = 0;
	uint   pos;
	uint   ofs = 0;
	for(int i = 0; i < fld; i++)
		ofs += stsize(flds[i].type);
	do {
		r = searchOnPage(pPattern, &pos, fcmp, ofs, srchMode, nxt, pExtraData);
		if(r) {
			P_Frame->Cur = pos;
			P_Frame->TopByCur(spNext);
		}
		else if(!(P_Frame->State & _eof) && !error) {
			if(nxt == spNext) {
				P_Frame->Cur = P_Frame->Count-1;
				P_Frame->TopByCur(spPrev);
			}
			else {
				P_Frame->Cur = 0;
				P_Frame->TopByCur(spNext);
			}
			step((nxt == spNext) ? P_Frame->Size : -static_cast<long>(P_Frame->Size));
			P_Frame->Cur = 0;
			P_Frame->Top = 0;
			P_Frame->SPos = P_Frame->SRange / 2;
		}
		else
			break;
	} while(!r && !error);
	return r;
}

int DBQuery::searchOnPage(const void * pPattern, uint * pPos, CompFunc fcmp, uint ofs, int srchMode, int nxt, void * pExtraData)
{
	int  r = 0;
	uint p = addr(P_Frame->Cur);
	uint count = (P_Frame->Count - P_Frame->Cur);
	uint s = MIN(count, P_Frame->Size-p);
	if(count) {
		do {
			SArray ary(P_Frame->P_Buf + p * recSize, recSize, s);
			uint _p = 0;
			if(srchMode & srchBinary) {
				r = ary.imp_bsearch(pPattern, &_p, fcmp, ofs, pExtraData);
				if((nxt == spNext && r > 0) || (nxt == spPrev && r < 0))
					//
					// При бинарном поиске больше ничего хорошего не ожидается //
					//
					return 0;
				r = BIN(r == 0);
			}
			else
				r = ary.lsearch(pPattern, &_p, fcmp, ofs, pExtraData);
			if(r)
				*pPos = (_p + p + P_Frame->Size - P_Frame->Zero) % P_Frame->Size;
			else if(s < count)
				if(p) {
					p = 0;
					s = count - s;
				}
				else
					break;
		} while(!r && s < count);
	}
	return r;
}
//
//
//
int FASTCALL deleteFrom(DBTable * pTbl, int useTa, DBQ & rQuery)
{
	return pTbl ? pTbl->deleteByQuery(useTa, rQuery) : 0;
}
//
//
//
DBUpdateSet::DBUpdateSet() : SArray(sizeof(DBUpdateSetItem), O_ARRAY|aryEachItem)
{
}

DBUpdateSet::~DBUpdateSet()
{
	freeAll();
}

uint DBUpdateSet::GetCount() const
{
	return count;
}

DBUpdateSetItem & DBUpdateSet::Get(uint pos) const
{
	return *static_cast<DBUpdateSetItem *>(SArray::at(pos));
}

/*virtual*/void FASTCALL DBUpdateSet::freeItem(void * pItem)
{
	DBUpdateSetItem * p_item = static_cast<DBUpdateSetItem *>(pItem);
	p_item->Val.I.destroy();
}

DBUpdateSet & FASTCALL set(DBField f, DBConst val) // @v11.2.11
{
	return set(f, static_cast<DBItem &>(val));
}

DBUpdateSet & FASTCALL set(DBField f, DBItem & val)
{
	DBUpdateSet * p_set = new DBUpdateSet;
	p_set->set(f, val);
	return *p_set;
}

DBUpdateSet & STDCALL DBUpdateSet::set(DBField f, DBConst val) // @v11.2.11
{
	return set(f, static_cast<DBItem &>(val));
}

DBUpdateSet & STDCALL DBUpdateSet::set(DBField f, DBItem & val)
{
	DBUpdateSetItem item;
	MEMSZERO(item);
	item.Fld = f;
	if(val.Id == DBConst_ID) {
		item.Val.C = *static_cast<DBConst *>(&val);
	}
	else if(val.Id == DBE_ID) {
		item.Val.E = *static_cast<DBE *>(&val);
		delete static_cast<DBE *>(&val);
	}
	else if(val.Id > 0) {
		item.Val.F = *static_cast<DBField *>(&val);
	}
	else {
		assert(0);
	}
	insert(&item);
	return *this;
}

int updateForCb(DBTable * pTbl, int useTA, DBQ & query, DBUpdateSet & rSet, UpdateDbTable_CbProc cbProc, void * extraPtr)
{
	int    ok = 1;
	int    ta = 0;
	DBQuery * q = 0;
	const  uint c = rSet.GetCount();
	if(c) {
		STempBuffer rec_before_buf(cbProc ? pTbl->getBufLen() : 0);
		q = & selectAll().from(pTbl, 0L).where(query);
		q->setDestroyTablesMode(0);
		q->setSearchForUpdateMode(1);
		if(useTA) {
			THROW(Btrieve::StartTransaction(1));
			ta = 1;
		}
		for(int dir = spFirst; ok && q->single_fetch(0, 0, dir); dir = spNext) {
			if(rec_before_buf.GetSize()) {
				memcpy(rec_before_buf, pTbl->getDataBufConst(), rec_before_buf.GetSize());
			}
			for(uint i = 0; i < c; i++) {
				DBUpdateSetItem & r_item = rSet.Get(i);
				if(r_item.Fld.Id == pTbl->GetHandle()) {
					void * p_data = r_item.Fld.getValuePtr();
					const BNField & r_fld = r_item.Fld.getField();
					DBConst val;
					r_item.Val.getValue(&val);
					val.convert(r_fld.T, p_data);
				}
			}
			//
			// Здесь необходимо использовать функцию без изменения текущей позиции.
			// Связано это с тем, что если записи извлекаются по индексу, значение которого
			// меняется, то произойдет сбой в запросе после чего результат может оказаться не
			// адекватным.
			//
			THROW(pTbl->updateRecNCC());
			if(cbProc)
				cbProc(pTbl, rec_before_buf, pTbl->getDataBufConst(), extraPtr);
		}
		THROW(!q->error);
		if(ta) {
			THROW(Btrieve::CommitWork());
			ta = 0;
		}
	}
	CATCH
		if(ta) {
			Btrieve::RollbackWork();
			ta = 0;
		}
		ok = 0;
	ENDCATCH
	delete q;
	delete &rSet;
	return ok;
}

int FASTCALL updateFor(DBTable * pTbl, int useTA, DBQ & query, DBUpdateSet & rSet)
{
	return updateForCb(pTbl, useTA, query, rSet, 0, 0);
}

