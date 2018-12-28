// DBQUERY.CPP
// Copyright (c) Sobolev A. 1995, 1996-2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2016, 2017, 2018
// @codepage UTF-8
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>

SLAPI DBQuery::DBQuery()
{
	THISZERO();
	options = destroy_tables;
}

SLAPI DBQuery::~DBQuery()
{
	uint   i;
	if(w_restrict) {
		w_restrict->destroy(1);
		delete w_restrict;
	}
	if(h_restrict) {
		h_restrict->destroy(0);
		delete h_restrict;
	}
	delete P_Frame;
	if(tbls) {
		const  int o = BIN(options & destroy_tables);
		for(i = 0; i < tblCount; i++) {
			Tbl  * t = tbls+i;
			if((t->flg & DBQTF_OVRRD_DESTROY_TAG) ? !o : o)
				delete t->tbl;
			t->tree.destroy();
			t->key.destroy();
			delete t->keyBuf;
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

void SLAPI DBQuery::setDestroyTablesMode(int set)
{
	SETFLAG(options, DBQuery::destroy_tables, set);
}

void SLAPI DBQuery::setSearchForUpdateMode(int set)
{
	for(uint i = 0; i < tblCount; i++) {
		SETFLAG(tbls[i].flg, DBQTF_SEARCHFORUPDATE, set);
	}
}

static int FASTCALL _search_item(int count, int * array, int item)
{
	for(int i = 0; i < count; i++)
		if(array[i] == item)
			return i;
	return -1;
}

int SLAPI DBQuery::_max_hdl(DBDataCell * c, int * list, int _max)
{
	if(c->GetId() > 0) {
		const int tmp = _search_item(tblCount, list, c->GetId());
		_max = MAX(_max, tmp);
	}
	else if(c->GetId() == DBE_ID) {
		for(int i = 0; i < c->E.Count; i++) {
			const int h = c->E.getTblHandle(i);
			if(h > 0) {
				const int tmp = _search_item(tblCount, list, h);
				_max = MAX(_max, tmp);
			}
		}
	}
	return _max;
}

int SLAPI DBQuery::arrangeTerms()
{
	int   list[32];
	uint  n;
	for(n = 0; n < tblCount; n++)
		list[n] = tbls[n].tbl->GetHandle();
	for(n = 0; n < w_restrict->count; n++) {
		DBQ::T * t = &w_restrict->items[n];
		t->tblIdx = _max_hdl(&t->right, list, _max_hdl(&t->left, list, -1));
		if(t->flags & DBQ_OUTER && t->tblIdx >= 0)
			tbls[t->tblIdx].flg |= DBQTF_OUTER_JOIN;
	}
	return 1;
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
int SLAPI DBQuery::makeNode(int tblN, int * pNode, int option, int * pPos)
{
	int    result = 0;
	if(*pNode == -1) {
		if(option != TEST_ONLY)
			*pPos = -1;
		result = NOTBIND;
	}
	else {
		int    left_child, right_child;
		int    lv, rv, lp, rp;
		DBTree * p_tree = &tbls[tblN].tree;
		DBTree::T & t = w_restrict->tree->P_Items[*pNode];
		if(t.link == 0) // Leaf
			if(w_restrict->items[t.term].tblIdx == tblN)
				if(option != TEST_ONLY) {
					result = p_tree->addLeaf(t.term, t.flags, pPos) ? BIND : 0;
					*pNode = -1;
				}
				else
					result = BIND;
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
				if(lv && rv)
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
			else if(t.link == _OR___) {
				lv = makeNode(tblN, &left_child,  TEST_ONLY, &lp);
				rv = makeNode(tblN, &right_child, TEST_ONLY, &rp);
				if(lv && rv)
					if(lv == BIND || rv == BIND)
						if(option != TEST_ONLY) {
							lv = makeNode(tblN, &left_child,  GET_ALL, &lp);
							rv = makeNode(tblN, &right_child, GET_ALL, &rp);
							if(lv && rv)
								result = p_tree->addNode(t.link, lp, rp, pPos) ? BIND : 0;
						}
						else
							result = BIND;
					else
						result = NOTBIND;
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
	if(t.key.P_h) { //  люч хоть и инициализирован, но €вл€етс€ динамическим
		if(t.key.P_h->pseg & DYN_KEY) {
			t.key.trim();
			t.key.P_h->pseg = 0;
			seg = 0;
			do {
				if(!t.tree.chooseKey(t.tree.Root, t.tbl->GetHandle(), seg, &t.key, 0))
					return 0;
			} while((t.key.P_h->pseg & ~DYN_KEY) == ++seg);
		}
	}
	else {
		int    ka_c = 0; //  оличество допустимых индексов
		int  * p_ka = 0;   // ћассив допустимых индексов
		uint   num_keys = t.tbl->GetIndices().getNumKeys();
		if(tblN == 0)
			if((p_ka = (int *)SAlloc::M(num_keys * sizeof(int))) != 0)
				ka_c = analyzeOrder(p_ka);
			else
				return 0;
		uint   i;
		uint   tc = (t.tree.Count+1) * sizeof(uint);
		uint * p_trace      = (uint *)SAlloc::M(tc);
		uint * p_best_trace = (uint *)SAlloc::M(tc);
		if(p_trace == 0 || p_best_trace == 0)
			return 0;
		p_best_trace[0] = 0;
		for(i = 0; i < num_keys; i++) {
			int    j, use_index = 1;
			if(ka_c)
				for(use_index = j = 0; !use_index && j < ka_c; j++)
					if(p_ka[j] == i)
						use_index = 1;
			if(use_index) {
				p_trace[0] = 0;
				KR k(t.tbl->GetHandle(), i);
				seg = 0;
				do
					if(!t.tree.chooseKey(t.tree.Root, t.tbl->GetHandle(), seg, &k, p_trace))
						return (k.destroy(), 0);
				while((k.P_h->pseg & ~DYN_KEY) == ++seg);
				int    cm = compare(t.key, k);
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
			/* @v6.1.1 Ётот флаг блокирует проверку условий, которые, как оказалось, не всегда истинны
			(*f) |=  DBQ_TRUE;
			*/
		}
		SAlloc::F(p_best_trace);
		t.tbl->setIndex(t.tbl->GetIndices()[t.key.P_h->keyNum].getKeyNumber());
		if((t.keyBuf = new char[MAXKEYLEN]) == 0)
			return 0;
	}
	return 1;
}
//
// ≈сли с помощью существующих индексов можно построить заданную
// сортировку, то возвращает (>0). »наче возвращает 0 и устанавливает
// либо флаг статуса s_tmp_table_needed, либо s_add_index_needed.
// ≈сли OK, то возвращает количество индексов, которые можно использовать,
// а по указателю pKeyArray (если pKeyArray != 0) записывает номера
// позиций допустимых индексов (Ётои индексы принадлежат первой
// таблице из указанных в запросе).
// ≈сли сортировка вообще не задана, то возвращаетс€ 0. ќтличить
// этот случай от отсутстви€ подход€щих индексов проще всего
// по равенству нулю пол€ DBQuery::ordCount. ћожно и по установке
// флагов s_tmp_table_needed и s_add_index_needed в поле DBQuery::status.
//
int FASTCALL DBQuery::analyzeOrder(int * pKeyArray)
{
	uint   count = 0;
	if(ordCount) {
		uint   i;
		const  uint num_keys = tbls[0].tbl->GetIndices().getNumKeys();
		for(i = 0; i < ordCount; i++)
			if(order[i].getTable() != tbls[0].tbl)
				return ((status |= s_tmp_table_needed), 0);
		for(i = 0; i < num_keys; i++) {
			int    ok = 1;
			const  BNKey key = tbls[0].tbl->GetIndices()[i];
			for(uint j = 0; j < (uint)key.getNumSeg() && j < ordCount; j++)
				if(order[j].fld != key.getFieldID(j)) {
					ok = 0;
					break;
				}
			if(ok) {
				if(pKeyArray)
					pKeyArray[count] = i;
				count++;
			}
		}
		if(count == 0)
			status |= s_add_index_needed;
	}
	return count;
}

int FASTCALL DBQuery::addField(DBConst & r)
{
	flds = (DBQuery::Fld*)SAlloc::R(flds, sizeof(DBQuery::Fld) * (fldCount+1));
	if(flds) {
		flds[fldCount].cell.C = r;
		flds[fldCount].type = ((DBItem *)&r)->stype();
		fldCount++;
		calcRecSize();
		return 1;
	}
	else
		return 0;
}

int FASTCALL DBQuery::addField(DBE & r)
{
	flds = (DBQuery::Fld*)SAlloc::R(flds, sizeof(DBQuery::Fld) * (fldCount+1));
	if(flds) {
		flds[fldCount].cell.E = r;
		flds[fldCount].type = ((DBItem *)&r)->stype();
		fldCount++;
		calcRecSize();
		return 1;
	}
	else
		return 0;
}

int FASTCALL DBQuery::addField(DBField & r)
{
	flds = (DBQuery::Fld*)SAlloc::R(flds, sizeof(DBQuery::Fld) * (fldCount+1));
	if(flds) {
		flds[fldCount].cell.F = r;
		flds[fldCount].type = ((DBItem *)&r)->stype();
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
		if((q.flds = (DBQuery::Fld*)SAlloc::M(sizeof(DBQuery::Fld) * list.GetCount())) != 0) {
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
		q.syntax |= DBQuery::t_select;
	}
	return q;
}

DBQuery & FASTCALL selectbycell(int count, DBDataCell * pList)
{
	DBQuery & q = *new DBQuery;
	if(&q != 0) {
		if((q.flds = (DBQuery::Fld*)SAlloc::M(sizeof(DBQuery::Fld) * count)) != 0)
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
		q.syntax |= DBQuery::t_select;
	}
	return q;
}

DBQuery & SLAPIV select(DBField first_arg, ...)
{
	va_list list = (va_list)&first_arg;
	DBQuery & q = *new DBQuery;
	if(&q != 0) {
		int  id;
		int  i = 0;
		size_t delta = 0;
		while((id = *(int *)list) != 0) {
			q.flds = (DBQuery::Fld*)SAlloc::R(q.flds, sizeof(DBQuery::Fld) * (i+1));
			if(q.flds == 0) {
				i = 0;
				break;
			}
			DBQuery::Fld & fld = q.flds[i];
			if(id == DBConst_ID) {
				delta = sizeof(DBConst);
				fld.cell.C = *(DBConst *)list;
			}
			else if(id == DBE_ID) {
				delta = sizeof(DBE);
				fld.cell.E = *(DBE *)list;
			}
			else if(id > 0) {
				delta = sizeof(DBField);
				fld.cell.F = *(DBField *)list;
			}
			else {
				;//CHECK(Invalid_DBItem);
			}
			fld.type = ((DBItem *)list)->stype();
			list = ((char *)list) + delta;
			i++;
		}
		q.fldCount = i;
		q.syntax |= DBQuery::t_select;
	}
	va_end(list);
	return q;
}

DBQuery & SLAPI selectAll()
{
	DBQuery & q = *new DBQuery;
	q.syntax |= (DBQuery::t_select | DBQuery::t_all);
	return q;
}

DBQuery & SLAPIV DBQuery::from(DBTable * first_arg, ...)
{
	register int i = 0;
	va_list  list;
	DBTable * p_tbl;
	list = (va_list)&first_arg;
	while((p_tbl = va_arg(list, DBTable*)) != 0)
		if((tbls = (Tbl*)SAlloc::R(tbls, sizeof(Tbl) * (i+1))) != 0) {
			tbls[i].tbl    = p_tbl;
			tbls[i].tree.init(0);
			tbls[i].key.P_b  = 0;
			tbls[i].keyBuf = 0;
			tbls[i].flg    = 0;
			i++;
		}
		else {
			i = 0;
			break;
		}
	tblCount = i;
	va_end(list);
	syntax |= t_from;
	calcRecSize();
	return *this;
}

int FASTCALL DBQuery::addTable(DBTable * pTbl)
{
	int    ok = 1;
	if(pTbl) {
		uint   i = tblCount;
		tbls = (Tbl *)SAlloc::R(tbls, sizeof(Tbl) * (i+1));
		if(tbls != 0) {
			tbls[i].tbl    = pTbl;
			tbls[i].tree.init(0);
			tbls[i].key.P_b  = 0;
			tbls[i].keyBuf = 0;
			tbls[i].flg    = 0;
			++tblCount;
			syntax |= t_from;
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

int SLAPI DBQuery::addOrderField(const DBField & rFld)
{
	if(&rFld) {
		order = (DBField *)SAlloc::R(order, sizeof(DBField) * (ordCount+1));
		order[ordCount++] = rFld;
		return 1;
	}
	else
		return 0;
}

DBQuery & SLAPIV DBQuery::orderBy(DBField first_arg, ...)
{
	va_list  list;
	list = (va_list)&first_arg;
	ordCount = 0;
	DBField f;
	while((f = va_arg(list, DBField)).Id != 0) {
		order = (DBField *)SAlloc::R(order, sizeof(DBField) * (ordCount+1));
		if(order == 0) {
			ordCount = 0;
			break;
		}
		order[ordCount++] = f;
	}
	va_end(list);
	syntax |= t_order;
	analyzeOrder(0);
	return *this;
}

int SLAPI DBQuery::checkWhereRestriction()
{
	return TRUE;
}

DBQuery & FASTCALL DBQuery::where(DBQ & q)
{
	if(&q != 0) {
		int isFirst = !(syntax & t_where);
		if(w_restrict) {
			w_restrict->destroy(1);
			delete w_restrict;
		}
		w_restrict = &q;
		if(!arrangeTerms()) {
			error = 1;
			syntax |= t_where;
			return *this;
		}
		for(int i = tblCount-1; i >= 0; i--) {
			if(!isFirst) {
				tbls[i].tree.destroy();
				tbls[i].key.destroy();
				if(tbls[i].keyBuf) {
					delete tbls[i].keyBuf;
					tbls[i].keyBuf = 0;
				}
			}
			tbls[i].tree.init(w_restrict);
			if(!makeNode(i, &w_restrict->tree->Root, 0, &tbls[i].tree.Root)) {
				error = 1;
				break;
			}
		}
		syntax |= t_where;
	}
	return *this;
}

int SLAPI DBQuery::calcRecSize()
{
	recSize  = 0;
	if(syntax & t_all) {
		for(uint i = 0; i < tblCount;) {
			const RECORDSIZE rs = tbls[i++].tbl->getRecSize();
			recSize += rs;
		}
	}
	else {
		for(uint i = 0; i < fldCount;)
			recSize += (RECORDSIZE)stsize(flds[i++].type);
	}
	return 1;
}

int SLAPI DBQuery::getFieldPosByName(const char * pFldName, uint * pPos) const
{
	for(uint i = 0; i < fldCount; i++)
		if(flds[i].cell.GetId() > 0 && sstreqi_ascii(flds[i].cell.F.getField().Name, pFldName)) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	ASSIGN_PTR(pPos, 0);
	return 0;
}

DBQuery::Tbl::Tbl() : tbl(0), keyBuf(0), flg(0)
{
	// @v9.8.11 (ctr has done it) Pos.SetZero();
}

int FASTCALL DBQuery::Tbl::Srch(void * pKey, int sp)
{
#if 0 // {
	//if(flg & DBQTF_LASTSRCHFAILED && (sp == spNext || sp == spPrev))
	//	return (BtrError = BE_KEYNFOUND, 0);
	if(tbl->search(pKey, sp)) {
		//flg &= ~DBQTF_LASTSRCHFAILED;
		return 1;
	}
	else {
		//flg |= DBQTF_LASTSRCHFAILED;
		return 0;
	}
#endif // } 0
	int    ok = (flg & DBQTF_SEARCHFORUPDATE) ? tbl->searchForUpdate(pKey, sp) : tbl->search(pKey, sp);
	if(!ok && BtrError == BE_INVPOS)
		BtrError = BE_EOF;
	return ok;
}

int FASTCALL DBQuery::_search(uint n, int dir)
{
	Tbl  & t = tbls[n];
	int    outer_rec = 0;
	int    sp;
	int    _first = (dir == spFirst || dir == spLast);
	int    more; // ‘лаг, предписывающий обновить значение ключа
	if(_first) {
		t.flg &= ~(DBQTF_PREVDOWN | DBQTF_PREVUP);
		t.tbl->ToggleStmt(1);
		if(chooseKey(n)) {
			if(dir == spFirst)
				t.key.first();
			else
				t.key.last();
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
		// @todo Ётот участок кода блокирует выбор второй и далее записей из таблиц, следующих
		// за неизменной записью первой таблицы.
		// ¬ыбрать момент, когда можно протестировать и убрать этот участок. {
		else {
			if(!(options & correct_search_more_problem)) {
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
			if(t.flg & DBQTF_PREVUP) {
				t.tbl->ToggleStmt(0);
				t.flg &= ~DBQTF_PREVUP;
			}
			t.flg |= DBQTF_PREVDOWN;
		}
		else if(dir == spPrev) {
			if(t.flg & DBQTF_PREVDOWN) {
				t.tbl->ToggleStmt(0);
				t.flg &= ~DBQTF_PREVDOWN;
			}
			t.flg |= DBQTF_PREVUP;
		}
		if(more) {
			t.flg &= ~ONLY_ONE_REC;
			t.key.getKey(t.keyBuf, &sp);
			if(sp & SKIP_ONE_REC) {
				sp &= ~SKIP_ONE_REC;
			   	if(!t.Srch(t.keyBuf, sp) && !BTRNFOUND)
					error = 1;
				sp = dir;
			}
			if(sp & ONLY_ONE_REC) {
				sp &= ~ONLY_ONE_REC;
				t.flg |= ONLY_ONE_REC;
			}
		}
		else if(t.flg & ONLY_ONE_REC) {
			t.flg &= ~ONLY_ONE_REC;
			return 0;
		}
		while(!error && t.Srch(t.keyBuf, sp) && t.key.checkKey(t.keyBuf)) {
			if(t.tree.checkRestriction()) {
__get_next_tbl:
				if((n+1 == tblCount || _search(n+1, (dir == spNext ? spFirst : spLast)))) { // @recursion
					if(options & save_positions)
						t.tbl->getPosition(&t.Pos);
					return 1;
				}
				else if(outer_rec)
					return 0;
			}
			sp = dir;
		}
		if(!btrokornfound())
			error = 1;
	} while((more = error ? 0 : ((dir == spNext) ? ++t.key : --t.key)) != 0);
	if(!error && (t.flg & DBQTF_OUTER_JOIN) && _first) {
		outer_rec = 1;
		t.tbl->clearDataBuf();
		goto __get_next_tbl;
	}
	return 0;
}

void FASTCALL DBQuery::fillRecord(char * buf, RECORDNUMBER * pPos)
{
	uint   i;
	RECORDSIZE p = 0;
	if(buf) {
		if(syntax & t_all) {
			for(i = 0; i < tblCount; i++) {
				RECORDSIZE s = tbls[i].tbl->GetFields().getRecSize();
				memcpy(buf+p, tbls[i].tbl->getDataBuf(), s);
				p += s;
			}
		}
		else {
			for(i = 0; i < fldCount; i++) {
				const TYPEID t = flds[i].type;
				flds[i].cell.getValue(t, buf+p);
				p += (RECORDSIZE)stsize(t);
			}
		}
	}
	if(pPos && options & save_positions)
		for(i = 0; i < tblCount; i++)
			pPos[i] = tbls[i].Pos;
}

#define INVALID_DIRECTION_CODE 0

int SLAPI DBQuery::single_fetch(char * buf, RECORDNUMBER * pPos, int dir)
{
	if(_search(0, dir)) {
		fillRecord(buf, pPos);
		actCount = 1;
	}
	else
		actCount = 0;
	return (int)actCount;
}

int SLAPI DBQuery::fetch(long count, char * pBuf, RECORDNUMBER * pPos, int dir)
{
	long   i = 0L;
	int    next;
	assert(oneof4(dir, spNext, spFirst, spPrev, spLast));
	if(dir == spNext || dir == spFirst)
		next = spNext;
	else if(dir == spPrev || dir == spLast)
		next = spPrev;
	if(_search(0, dir))
		do {
			if(pBuf)
				fillRecord(pBuf+recSize*(size_t)i, pPos ? (pPos+tblCount*(size_t)i) : 0);
		} while(++i < count && _search(0, next));
	actCount = i;
	if(next == spPrev && pBuf && i > 1 && !(options & fetch_reverse))
		for(int j = 0; j < (i >> 1); j++)
			memswap(pBuf + j * recSize, pBuf + ((size_t)i - j - 1) * recSize, recSize);
	return (i < count) ? 0 : 1;
}

int SLAPI DBQuery::fetch(long count, char * pBuf, int dir)
{
	return fetch(count, pBuf, 0, dir);
}

#define BOQ  (P_Frame->state & Frame::Top)
#define EOQ  (P_Frame->state & Frame::Bottom)

long DBQuery::_defaultBufSize  = 64L;
long DBQuery::_defaultBufDelta =  4L;

inline uint FASTCALL DBQuery::addr(uint p) const
{
	return ((P_Frame->zero+p) % P_Frame->size);
}

void * FASTCALL DBQuery::getRecord(uint r)
{
	return (r < P_Frame->count) ? (P_Frame->buf + recSize * addr(r)) : 0;
}

void * SLAPI DBQuery::getCurrent()
{
	return getRecord(P_Frame->cur);
}

void * SLAPI DBQuery::getBuffer()
{
	return P_Frame->buf;
}

int SLAPI DBQuery::setFrame(uint viewHight, uint bufSize, uint bufDelta)
{
	if(bufSize != UNDEF && !allocFrame(bufSize))
		return 0;
	if(bufDelta != UNDEF)
		P_Frame->inc = bufDelta;
	if(viewHight != UNDEF) {
		P_Frame->hight = MIN(viewHight, P_Frame->size);
		if(P_Frame->cur != P_Frame->top || /*P_Frame->top < 0 ||*/ P_Frame->top > P_Frame->count) {
			if((P_Frame->cur-P_Frame->top) >= P_Frame->hight)
				P_Frame->top = P_Frame->cur-P_Frame->hight+1;
			else if(P_Frame->cur < P_Frame->hight)
				P_Frame->top = 0;
			else if((P_Frame->count-P_Frame->top) < P_Frame->hight)
				P_Frame->top = P_Frame->count-P_Frame->hight;
		}
	}
	options |= smart_frame;
	options &= ~fetch_reverse;
	return 1;
}

int SLAPI DBQuery::allocFrame(uint bufSize)
{
	if(P_Frame && P_Frame->size == bufSize)
		return 1;
	if(bufSize) {
		if(P_Frame == 0) {
			if((P_Frame = new Frame) != 0) {
				P_Frame->buf = 0;
				P_Frame->posBuf = 0;
				P_Frame->count = 0;
				P_Frame->zero = 0;
				P_Frame->cur = 0;
				P_Frame->top = 0;
			}
			else
				return 0;
		}
		P_Frame->buf = (char *)SAlloc::R(P_Frame->buf, bufSize * recSize);
		if(!P_Frame->buf) {
			delete P_Frame;
			return 0;
		}
		P_Frame->state = Frame::Undef;
		P_Frame->size = bufSize;
		P_Frame->count = MIN(P_Frame->count, bufSize);
		if(options & save_positions) {
			P_Frame->posBuf = (ulong *)SAlloc::R(P_Frame->posBuf, P_Frame->size * tblCount * sizeof(RECORDNUMBER));
			if(!P_Frame->posBuf) {
				options &= ~save_positions;
				return 0;
			}
		}
	}
	else {
		if(P_Frame) {
			ZFREE(P_Frame->buf);
			ZFREE(P_Frame->posBuf);
			P_Frame->size = 0;
		}
		options &= ~smart_frame;
	}
	return 1;
}

void SLAPI DBQuery::moveRec(uint rd, uint rs)
{
	ulong * pp = P_Frame->posBuf;
	char * b = P_Frame->buf;
	memmove(b + rd * recSize, b + rs * recSize, recSize);
	if(pp) {
		uint ps = tblCount*sizeof(RECORDNUMBER);
		memmove(pp + rd * tblCount, pp + rs * tblCount, ps);
	}
}

void SLAPI DBQuery::moveBuf(uint dest, uint src, uint recs)
{
	if(dest > src) //  опируем сверху-вниз
		for(int i = (recs-1); i >= 0; i--)
			moveRec(addr(dest+i), addr(src+i));
	else //  опируем снизу-вверх
		for(uint j = 0; j < recs; j++)
			moveRec(addr(dest+j), addr(src+j));
}

void FASTCALL DBQuery::frameOnBottom(int undefSDelta)
{
	uint c  = P_Frame->count;
	if(c) {
		uint ht = MIN(P_Frame->hight, c);
		P_Frame->sdelta = undefSDelta ? 0x8000 : (c - ht - P_Frame->top);
		P_Frame->cur    = c - 1;
		P_Frame->top    = c - ht;
	}
	else {
		P_Frame->sdelta = 0;
		P_Frame->cur = P_Frame->top = 0;
	}
}

void FASTCALL DBQuery::setCount(uint c)
{
	if(c) {
		if(P_Frame->top >= c)
			P_Frame->top = c-1;
		if(P_Frame->cur >= c)
			P_Frame->cur = c-1;
	}
	else
		P_Frame->top = P_Frame->cur = 0;
	P_Frame->count = c;
}

#define LBUFPTR(p) (P_Frame->buf+addr(p)*recSize)
#define PBUFPTR(p) (P_Frame->buf+(p)*recSize)
#define LPOSPTR(p) (P_Frame->posBuf ? (P_Frame->posBuf+addr(p)*tblCount) : 0)
//
// —осто€ни€ признака DBQuery::Frame::last
//
#define _lbottom  1
#define _lnext    2
#define _ltop     3
#define _lprev    4

int SLAPI DBQuery::_fetch_next(uint count, uint p, int dir)
{
	uint i;
	for(i = 0; i < count; i++)
		if(!single_fetch(LBUFPTR(p+i), LPOSPTR(p+i), i ? spNext : dir))
			break;
	P_Frame->state = (dir == spFirst) ? Frame::Top : Frame::Undef;
	if(i < count) {
		P_Frame->last = _lbottom;
		if(!error)
			P_Frame->state |= Frame::Bottom;
	}
	else
		P_Frame->last = _lnext;
	actCount = i;
	return !error;
}

int SLAPI DBQuery::_fetch_prev(uint count, uint p)
{
	uint i, s = P_Frame->size;
	for(i = 0; i < count; i++)
		if(!single_fetch(LBUFPTR(s+p-i), LPOSPTR(s+p-i), spPrev))
			break;
	P_Frame->state = Frame::Undef;
	if(i < count) {
		P_Frame->last = _ltop;
		if(!error)
			P_Frame->state |= Frame::Top;
	}
	else
		P_Frame->last = _lprev;
	actCount = i;
	return !error;
}

int SLAPI DBQuery::_fetch_last(uint count, uint p)
{
	uint   i;
	const  uint s = P_Frame->size;
	for(i = 0; i < count; i++)
		if(!single_fetch(LBUFPTR(s+p-i), LPOSPTR(s+p-i), i ? spPrev : spLast))
			break;
	P_Frame->state = Frame::Bottom;
	if(i < count) {
		P_Frame->last = _ltop;
		if(!error)
			P_Frame->state |= Frame::Top;
	}
	else
		P_Frame->last = _lprev;
	actCount = i;
	return !error;
}

#pragma warn -sig

int FASTCALL DBQuery::normalizeFrame(int dir)
{
	assert(dir == spNext || dir == spPrev);
	uint   recs, pos;
	if(dir == spNext) {
		if(P_Frame->last == _lprev) {
			recs = P_Frame->count-1;
			pos = 1;
		}
		else if(P_Frame->last == _ltop) {
			recs = P_Frame->count-1;
			pos = 1;
		}
		else
			return 1;
		_fetch_next(recs, pos, spNext);
		if(actCount < recs)
			setCount(actCount);
	}
	else { // dir == spPrev
		if(P_Frame->last == _lnext) {
			recs = P_Frame->count-1;
			pos = P_Frame->count-2;
		}
		else if(P_Frame->last == _lbottom) {
			recs = P_Frame->count-1;
			pos = P_Frame->count-2;
		}
		else
			return 1;
		_fetch_prev(recs, pos);
		if(actCount < recs) {
			P_Frame->zero = addr(P_Frame->count-actCount);
			setCount(actCount);
		}
	}
	return !error;
}

int SLAPI DBQuery::refresh()
{
	P_Frame->sdelta = 0;
	if(BOQ) {
		_fetch_next(P_Frame->size, 0, spFirst);
		setCount(actCount);
	}
	else if(EOQ) {
		uint s = P_Frame->size;
		_fetch_last(s, s-1);
		P_Frame->zero  = addr(s - actCount);
		setCount(actCount);
	}
	else if(P_Frame->last == _ltop || P_Frame->last == _lprev) {
		_fetch_prev(1, 0);
		_fetch_next(P_Frame->count, 0, spNext);
		setCount(actCount);
	}
	else {
		normalizeFrame(spPrev);
		normalizeFrame(spNext);
	}
	return !error;
}

int SLAPI DBQuery::top()
{
	if(BOQ)
		P_Frame->sdelta = -((long)P_Frame->top);
	else {
		P_Frame->sdelta = 0x8000;
		_fetch_next(P_Frame->size, 0, spFirst);
		P_Frame->count = actCount;
	}
	P_Frame->top = P_Frame->cur = 0;
	P_Frame->spos = 0;
	return !error;
}

int SLAPI DBQuery::bottom()
{
	if(!error) {
		if(!EOQ) {
			uint s = P_Frame->size;
			_fetch_last(s, s-1);
			P_Frame->zero  = addr(s - actCount);
			P_Frame->count = actCount;
		}
		frameOnBottom(!EOQ);
		P_Frame->spos = P_Frame->srange;
	}
	return !error;
}

int FASTCALL DBQuery::step(long delta)
{
	if(error)
		return 0;
	int  diff;
	uint ht = P_Frame->hight;
	if(delta == 0)
		return 1;
	P_Frame->sdelta = 0;
	uint temp;
	uint _cur = P_Frame->cur;
	uint c = P_Frame->count;
	uint s = P_Frame->size;
	if(delta > 0) {
		delta = MIN((ulong)delta, s);
		diff = (_cur+delta-c);
		if(diff >= 0) {
			if(!EOQ) {
				normalizeFrame(spNext);
				_fetch_next(diff+1, c = P_Frame->count, spNext);
				if((temp = c + actCount) > s)
					P_Frame->zero = addr(temp % s);
				P_Frame->count = MIN(temp, s);
			}
			if(EOQ) {
				frameOnBottom(0);
				P_Frame->spos = P_Frame->srange;
				return error ? 0 : -1;
			}
		}
		P_Frame->cur = MIN(P_Frame->count - 1, P_Frame->cur + delta);
		if(P_Frame->cur >= (P_Frame->top + ht)) {
			temp = P_Frame->cur - ht + 1;
			P_Frame->sdelta = temp - P_Frame->top;
			P_Frame->top    = temp;
		}
		//P_Frame->spos += delta;
		P_Frame->spos = P_Frame->srange / 2;
	}
	else {
		delta = MIN((ulong)-delta, ht);
		if(P_Frame->cur < (ulong)delta) {
			if(!BOQ) {
				normalizeFrame(spPrev);
				uint recs = delta - P_Frame->cur;
				_fetch_prev(recs, s-1);
				P_Frame->zero = addr(s - actCount % s);
				P_Frame->top = P_Frame->cur = 0;
				P_Frame->sdelta = -delta;
				//P_Frame->spos -= delta;
				P_Frame->spos = P_Frame->srange / 2;
			}
			if(BOQ) {
				if(P_Frame->cur) {
					P_Frame->sdelta = -(long)P_Frame->top;
					P_Frame->top = P_Frame->cur = 0;
					P_Frame->spos = 0;
				}
			}
			return !error;
		}
		P_Frame->cur = P_Frame->cur-delta;
		if(P_Frame->cur < P_Frame->top) {
			P_Frame->sdelta = P_Frame->cur - P_Frame->top;
			P_Frame->top = P_Frame->cur;
		}
		//P_Frame->spos -= delta;
		P_Frame->spos = P_Frame->srange / 2;
	}
	P_Frame->spos = MIN(P_Frame->srange, P_Frame->spos);
	return !error;
}

#pragma warn .sig

SLAPI DBQuery::Frame::~Frame()
{
	SAlloc::F(buf);
	SAlloc::F(posBuf);
}

void FASTCALL DBQuery::Frame::topByCur(int dir)
{
	if(count == 0)
		top = 0;
	else if(dir == spNext) {
		if(cur >= hight) {
			top = (count < hight) ? 0 : MIN(cur, count-hight);
		}
	}
	else
		top = ((cur+1) >= hight) ? (cur+1-hight) : 0;
}

int SLAPI DBQuery::search(const void * pPattern, CompFunc fcmp, int fld, uint srchMode, void * pExtraData)
{
	int    dir = (srchMode & ~srchFlags);
	int    nxt;
	int    _eof;
	if(dir == srchFirst) {
		P_Frame->zero = 0;
		top();
		nxt = spNext;
	}
	else if(dir == srchLast) {
		P_Frame->zero = 0;
		bottom();
		nxt = spPrev;
	}
	else {
		step((dir == srchNext) ? 1 : -1);
		nxt = (dir == srchNext) ? spNext : spPrev;
	}
	_eof = (nxt == spNext) ? Frame::Bottom : Frame::Top;
	int    r = 0;
	uint   pos;
	uint   ofs = 0;
	for(int i = 0; i < fld; i++)
		ofs += stsize(flds[i].type);
	do {
		if((r = searchOnPage(pPattern, &pos, fcmp, ofs, srchMode, nxt, pExtraData)) != 0) {
			P_Frame->cur = pos;
			P_Frame->topByCur(spNext);
		}
		else if(!(P_Frame->state & _eof) && !error) {
			if(nxt == spNext) {
				P_Frame->cur = P_Frame->count-1;
				P_Frame->topByCur(spPrev);
			}
			else {
				P_Frame->cur = 0;
				P_Frame->topByCur(spNext);
			}
			step((nxt == spNext) ? P_Frame->size : -(long)P_Frame->size);
			P_Frame->cur = 0;
			P_Frame->top = 0;
			P_Frame->spos = P_Frame->srange / 2;
		}
		else
			break;
	} while(!r && !error);
	return r;
}

int SLAPI DBQuery::searchOnPage(const void * pPattern, uint * pPos, CompFunc fcmp,
	uint ofs, int srchMode, int nxt, void * pExtraData)
{
	int  r = 0;
	uint p = addr(P_Frame->cur);
	uint count = (P_Frame->count - P_Frame->cur);
	uint s = MIN(count, P_Frame->size-p);
	if(count) {
		do {
			SArray ary(P_Frame->buf + p * recSize, recSize, s);
			uint _p = 0;
			if(srchMode & srchBinary) {
				r = ary.imp_bsearch(pPattern, &_p, fcmp, ofs, pExtraData);
				if((nxt == spNext && r > 0) || (nxt == spPrev && r < 0))
					//
					// ѕри бинарном поиске больше ничего хорошего не ожидаетс€ //
					//
					return 0;
				r = BIN(r == 0);
			}
			else
				r = ary.lsearch(pPattern, &_p, fcmp, ofs, pExtraData);
			if(r)
				*pPos = (_p + p + P_Frame->size - P_Frame->zero) % P_Frame->size;
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
int FASTCALL deleteFrom(DBTable * pTbl, int useTa, DBQ & query)
{
	return pTbl ? pTbl->deleteByQuery(useTa, query) : 0;
}
//
//
//
DBUpdateSet::DBUpdateSet() : SArray(sizeof(DBUpdateSetItem), /*8,*/ O_ARRAY|aryEachItem)
{
}

DBUpdateSet::~DBUpdateSet()
{
	freeAll();
}

uint SLAPI DBUpdateSet::GetCount() const
{
	return count;
}

DBUpdateSetItem & DBUpdateSet::Get(uint pos) const
{
	return *(DBUpdateSetItem *)SArray::at(pos);
}

//virtual
void FASTCALL DBUpdateSet::freeItem(void * pItem)
{
	DBUpdateSetItem * p_item = (DBUpdateSetItem *)pItem;
	p_item->Val.I.destroy();
}

DBUpdateSet & FASTCALL set(DBField f, DBItem & val)
{
	DBUpdateSet * p_set = new DBUpdateSet;
	p_set->set(f, val);
	return *p_set;
}

DBUpdateSet & FASTCALL DBUpdateSet::set(DBField f, DBItem & val)
{
	DBUpdateSetItem item;
	MEMSZERO(item);
	item.Fld = f;
	// DBItem
	if(val.Id == DBConst_ID) {
		item.Val.C = *(DBConst *)&val;
	}
	else if(val.Id == DBE_ID) {
		item.Val.E = *(DBE *)&val;
		delete (DBE *)&val;
	}
	else if(val.Id > 0) {
		item.Val.F = *(DBField *)&val;
	}
	else {
		assert(0);
	}
	insert(&item);
	return *this;
}

int updateForCb(DBTable * pTbl, int useTA, DBQ & query, DBUpdateSet & rSet, UpdateDbTable_CbProc cbProc, long extraParam)
{
	int    ok = 1, ta = 0;
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
				memcpy((char *)rec_before_buf, pTbl->getDataBuf(), rec_before_buf.GetSize());
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
			// «десь необходимо использовать функцию без изменени€ текущей позиции.
			// —в€зано это с тем, что если записи извлекаютс€ по индексу, значение которого
			// мен€етс€, то произойдет сбой в запросе после чего результат может оказатьс€ не
			// адекватным.
			//
			THROW(pTbl->updateRecNCC());
			if(cbProc)
				cbProc(pTbl, (char *)rec_before_buf, pTbl->getDataBuf(), extraParam);
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

