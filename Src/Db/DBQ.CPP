// DBQ.CPP
// Copyright (c) Sobolev A. 1996-2001, 2002, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>
//
// DBItem
//
void SLAPI DBItem::destroy()
{
	switch(Id) {
		case DBQ_ID: reinterpret_cast<DBQ *>(this)->destroy(); break;
		case DBConst_ID: reinterpret_cast<DBConst *>(this)->destroy(); break;
		case DBE_ID: reinterpret_cast<DBE *>(this)->destroy(); break;
	}
}

int SLAPI DBItem::typeOfItem() const
{
	switch(Id) {
		case DBQ_ID: return DBQ_ID;
		case DBConst_ID: return DBConst_ID;
		case DBE_ID: return DBE_ID;
		default:
			if(Id > 0)
				return DBField_ID;
	}
	//CHECK(Invalid_DBItem);
	return 0;
}

TYPEID SLAPI DBItem::stype() const
{
	int    bt;
	TYPEID st = 0;
	if(Id > 0)
		st = static_cast<const DBField *>(this)->getField().T;
	else if(Id == DBE_ID) {
		if(static_cast<const DBE *>(this)->Count) {
			DBE::T & t = static_cast<const DBE *>(this)->P_Terms[static_cast<const DBE *>(this)->Count-1];
			//CHECK(t.tag == DBE::T::func);
			bt = DbqFuncTab::Get(t.function)->typ;
			if(bt == BTS_STRING) {
				DBConst len;
				len.init(0L);
				const_cast<DBE *>(static_cast<const DBE *>(this))->evaluate(CALC_SIZE, &len); // @badcast
				st = MKSTYPE(S_ZSTRING, (int)len.lval);
			}
			else
				st = bt2st(bt);
		}
	}
	else if(Id == DBConst_ID) {
		bt = static_cast<const DBConst *>(this)->Tag;
		if(bt == BTS_STRING) {
			const char * p = static_cast<const DBConst *>(this)->sptr;
			st = MKSTYPE(S_ZSTRING, (p ? sstrlen(p) : 0) + 1);
		}
		else
			st = bt2st(bt);
	}
	return st;
}

int SLAPI DBItem::baseType() const
{
	int    bt = BTS_VOID;
	if(Id > 0)
		bt = stbase(static_cast<const DBField *>(this)->getField().T);
	else if(Id == DBE_ID) {
		if(static_cast<const DBE *>(this)->Count) {
			DBE::T & t = static_cast<const DBE *>(this)->P_Terms[static_cast<const DBE *>(this)->Count-1];
			//CHECK(t.tag == DBE::T::func);
			bt = DbqFuncTab::Get(t.function)->typ;
		}
	}
	else if(Id == DBConst_ID)
		bt = static_cast<const DBConst *>(this)->Tag;
	return bt;
}
//
// DBConst
//
DBConst FASTCALL dbconst(long v)
{
	DBConst c;
	c.init(v);
	return c;
}

DBConst FASTCALL dbconst(int16 v)
{
	DBConst c;
	c.init((long)v);
	return c;
}

DBConst FASTCALL dbconst(uint16 v)
{
	DBConst c;
	c.init((long)v);
	return c;
}

DBConst FASTCALL dbconst(double v)
{
	DBConst c;
	c.init(v);
	return c;
}

DBConst FASTCALL dbconst(const char * pS)
{
	DBConst c;
	c.init(pS);
	return c;
}

DBConst FASTCALL dbconst(const void * ptr)
{
	DBConst c;
	c.init(ptr);
	return c;
}

DBConst FASTCALL dbconst(LDATE v)
{
	DBConst c;
	c.init(v);
	return c;
}

DBConst FASTCALL dbconst(LTIME v)
{
	DBConst c;
	c.init(v);
	return c;
}

void FASTCALL DBConst::init(long l)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = lv;
	lval = l;
}

void FASTCALL DBConst::init(double d)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = rv;
	rval = d;
}

void FASTCALL DBConst::init(const char * s)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = sp;
	sptr = newStr(s);
}

void FASTCALL DBConst::InitForeignStr(const char * pS) // @v9.9.3
{
	Id = DBConst_ID;
	Flags = fNotOwner;
	Tag = sp;
	//sptr
	ptrval = pS;
	assert(sptr == ptrval);
}

void FASTCALL DBConst::init(LDATE d)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = dv;
	dval = d;
}

void FASTCALL DBConst::init(LTIME t)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = tv;
	tval = t;
}

void FASTCALL DBConst::init(LDATETIME t)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = dtv;
	dtval = t;
}

void FASTCALL DBConst::init(const void * ptr)
{
	Id = DBConst_ID;
	Flags = 0;
	Tag = ptrv;
	ptrval = ptr;
}

void FASTCALL DBConst::destroy()
{
	if(Tag == sp && !(Flags & fNotOwner))
		ZDELETE(sptr);
}

int FASTCALL DBConst::copy(const DBConst & src)
{
	memcpy(this, &src, sizeof(DBConst));
	return !(src.Tag == sp && !(Flags & fNotOwner) && src.sptr && (sptr = newStr(src.sptr)) == 0);
}

int FASTCALL DBConst::convert(TYPEID type, void * d) const
{
	return stcast(bt2st(Tag), type, ((Tag == sp) ? (void *)sptr : (void *)&lval), d, 0L);
}

char * FASTCALL DBConst::tostring(long fmt, char * pBuf) const
{
	return sttostr(bt2st(Tag), ((Tag == sp) ? (void *)sptr : (void *)&lval), fmt, pBuf);
}

int FASTCALL DBConst::fromfld(DBField f)
{
	const TYPEID t = f.getField().T;
	const void * v = f.getValuePtr();
	Tag = /*(DBConst::_tag)*/stbase(t);
	if(Tag == BTS_VOID)
		return 0;
	else if(Tag == BTS_STRING) {
		char   b[4096];
		sttobase(t, v, b);
		return BIN((sptr = newStr(b)) != 0);
	}
	else
		return sttobase(t, v, &lval);
}

#define INVALID_TAG_OF_DBCONST       0
#define INCOMPATIBLE_DBCONST_COMPARE 0

int FASTCALL compare(DBConst * c1, DBConst * c2)
{
	double  d1, d2;
	if(c1->Tag != c2->Tag) {
		if(btnumber(c1->Tag) && btnumber(c2->Tag)) {
			stcast(bt2st(c1->Tag), bt2st(BTS_REAL), &c1->rval, &d1, 0);
			stcast(bt2st(c2->Tag), bt2st(BTS_REAL), &c2->rval, &d2, 0);
			c1->init(d1);
			c2->init(d2);
		}
		/*
		else
			CHECK(INCOMPATIBLE_DBCONST_COMPARE);
		*/
	}
	if(c1->Tag == BTS_STRING)
		return stcomp(bt2st(c1->Tag), c1->sptr, c2->sptr);
	else
		return stcomp(bt2st(c1->Tag), &c1->lval, &c2->lval);
}
//
// DBE
//
void DBE::init()
{
	Id = DBE_ID;
	P_Terms = 0;
	Count = 0;
	Pointer = 0;
	DontDestroy = 0;
}

void DBE::destroy()
{
	if(P_Terms) {
		for(int i = 0; i < Count; i++) {
			T & t = P_Terms[i];
			if(t.tag == T::sp && t.sptr) {
				ZDELETE(t.sptr);
			}
		}
		ZFREE(P_Terms);
	}
	Count = 0;
}

int FASTCALL DBE::getTblHandle(int n)
{
	assert(n >= 0 && n < Count);
	return (P_Terms[n].tag == T::fld) ? P_Terms[n].field.Id : 0;
}

int FASTCALL DBE::evaluate(int option, DBConst * r)
{
	assert(Count > 0);
	Pointer = Count;
	pop();
	assert(P_Terms[Pointer].tag == T::func);
	return call(option, r);
}

int FASTCALL DBE::push(DBE::T * t)
{
	P_Terms = static_cast<DBE::T *>(SAlloc::R(P_Terms, sizeof(DBE::T) * (Count+1)));
	if(P_Terms == 0) {
		Count = 0;
		return 0;
	}
	else {
		memcpy(P_Terms+Count, t, sizeof(DBE::T));
		Count++;
		return 1;
	}
}
//
// «амечание: в этой функции предполагаетс€, что поле
// rval имеет максимальный среди остальных полей размер
// как в union из DBConst, так и в union из DBE::T
//
int FASTCALL DBE::push(DBItem & item)
{
	T      t;
	int    ok = 1;
	if(item.Id > 0) {
		t.tag = T::fld;
		t.field = *static_cast<const DBField *>(&item);
		ok = push(&t);
	}
	else if(item.Id == DBConst_ID) {
		const DBConst * c = static_cast<const DBConst *>(&item);
		t.tag = static_cast<DBE::T::_tag>(c->Tag);
		memcpy(&t.rval, &c->rval, sizeof(t.rval));
		ok = push(&t);
	}
	else if(item.Id == DBE_ID) {
		DBE * p_e = static_cast<DBE *>(&item);
		for(int i = 0; ok && i < p_e->Count; i++)
			ok = push(p_e->P_Terms+i);
		if(!p_e->DontDestroy) {
			SAlloc::F(p_e->P_Terms);
			p_e->P_Terms = 0;
			p_e->Count = 0;
			delete p_e;
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL DBE::push(DBFunc f)
{
	T t;
	t.tag = T::func;
	t.function = f;
	return push(&t);
}

int DBE::pop()
{
	if(Pointer) {
		Pointer--;
		return 1;
	}
	else
		return 0;
}

int FASTCALL DBE::call(int option, DBConst * r)
{
#define MAXSTATICPAR 32
	T    * t = &P_Terms[Pointer];
	DBFuncInfo * p_fi = DbqFuncTab::Get(t->function);
	//CHECK(p_fi != 0);
	int    i;
	const  int cnt = p_fi->paramCount;
	DBConst * p_par = 0;
	DBConst params[MAXSTATICPAR];
	if(cnt) {
		p_par = (cnt > MAXSTATICPAR) ? new DBConst[cnt] : params;
		for(i = cnt - 1; i >= 0; i--) {
			pop();
			DBConst & c = p_par[i];
			c.Id = DBConst_ID;
			t = &P_Terms[Pointer];
			switch(t->tag) {
				case T::lv:
					c.Flags = 0;
					c.Tag  = DBConst::lv;
					c.lval = t->lval;
					break;
				case T::rv:
					c.Flags = 0;
					c.Tag  = DBConst::rv;
					c.rval = t->rval;
					break;
				case T::sp:
					if(option == CALC_SIZE) {
						c.Flags = 0;
						c.Tag  = DBConst::lv;
						c.lval = sstrlen(t->sptr)+1;
					}
					else {
						c.Flags = 0;
						c.Tag  = DBConst::sp;
						c.sptr = newStr(t->sptr);
					}
					break;
				case T::dv:
					c.Flags = 0;
					c.Tag  = DBConst::dv;
					c.dval = t->dval;
					break;
				case T::tv:
					c.Flags = 0;
					c.Tag  = DBConst::tv;
					c.tval = t->tval;
					break;
				case T::ptrv:
					c.Flags = 0;
					c.Tag = DBConst::ptrv;
					c.ptrval = t->ptrval;
					break;
				case T::fld:
					if(!c.fromfld(t->field))
						return 0;
					if(option == CALC_SIZE)
						if(c.Tag == DBConst::sp) {
							const long len = sstrlen(c.sptr)+1;
							c.destroy();
							c.init(len);
						}
						else
							c.init(0L);
					break;
				case T::func:
					if(!call(option, &c))
						return 0;
					break;
			}
		}
	}
	p_fi->proc(option, r, params);
	if(p_par) {
		for(i = 0; i < cnt; i++)
			p_par[i].destroy();
		if(cnt > MAXSTATICPAR)
			delete p_par;
	}
	return 1;
}
//
// DBField
//
int SLAPI DBField::getIndex(BNKey * pKey, int * pKeyPos, int * pSeg)
{
	BNKeyList & key_list = _GetTable(Id)->indexes;
	uint   nk = key_list.getNumKeys();
	int    key_num = (!*pKey) ? -1 : pKey->getKeyNumber();
	uint   i = 0;
	while(i < nk && (key_list[i].getKeyNumber() <= key_num))
		i++;
	while(i < nk) {
		if(key_list[i].containsField(fld, pSeg)) {
			*pKeyPos = i;
			*pKey = key_list[i];
			return 1;
		}
		i++;
	}
	return 0;
}

void * SLAPI DBField::getValuePtr() const
{
	DBTable * tbl = _GetTable(Id);
	return tbl ? (PTR8(tbl->getDataBuf()) + tbl->fields[fld].Offs) : 0;
}

DBTable * SLAPI DBField::getTable() const { return _GetTable(Id); }
const  BNField & SLAPI DBField::getField() const { return _GetTable(Id)->fields[fld]; }

int FASTCALL DBField::getValue(void * d, size_t * pSize) const
{
	DBTable * tbl = _GetTable(Id);
	return tbl ? tbl->fields[fld].getValue(tbl->getDataBufConst(), d, pSize) : 0;
}

int SLAPI DBField::putValue(const void * pBuf) const
{
	DBTable * tbl = _GetTable(Id);
	return tbl ? tbl->fields[fld].setValue(tbl->getDataBuf(), pBuf) : 0;
}
//
//
//
SLAPI DBFieldList::DBFieldList(uint n) : Count(0), P_Flds(0)
{
	Alloc(n);
}

SLAPI DBFieldList::~DBFieldList()
{
	Destroy();
}

DBFieldList & FASTCALL DBFieldList::operator = (const DBFieldList & s)
{
	Destroy();
	Add(s);
	return *this;
}

void SLAPI DBFieldList::Destroy()
{
	Alloc(0);
}

int SLAPI DBFieldList::Search(const DBField & rFld, uint * pPos) const
{
	for(uint i = 0; i < Count; i++)
		if(P_Flds[i].Id == rFld.Id && P_Flds[i].fld == rFld.fld) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	return 0;
}

int FASTCALL DBFieldList::Alloc(uint n)
{
	int    ok = 1;
	ZFREE(P_Flds);
	Count = 0;
	if(n) {
		P_Flds = static_cast<DBField *>(SAlloc::M(n * sizeof(DBField)));
		if(P_Flds) {
			Count = n;
			memzero(P_Flds, sizeof(DBField) * Count);
		}
		else
			ok = (BtrError = BE_NOMEM, 0);
	}
	return ok;
}

int FASTCALL DBFieldList::Add(const DBField & rSrc)
{
	int    ok = 1;
	P_Flds = (DBField *)SAlloc::R(P_Flds, sizeof(DBField) * (Count + 1));
	if(P_Flds)
		P_Flds[Count++] = rSrc;
	else {
		Count = 0;
		ok = (BtrError = BE_NOMEM, 0);
	}
	return ok;
}

int FASTCALL DBFieldList::Add(const DBFieldList & s)
{
	for(uint i = 0; i < s.Count; i++)
		Add(s.P_Flds[i]);
	return 1;
}

uint   SLAPI DBFieldList::GetCount() const { return Count; }
const  DBField & FASTCALL DBFieldList::Get(uint fldN) const { return (fldN < Count) ? P_Flds[fldN] : *(DBField *)0; }
const  BNField & FASTCALL DBFieldList::GetField(uint fldN) const { return (fldN < Count) ? P_Flds[fldN].getField() : *(BNField *)0; }
int    SLAPI DBFieldList::GetValue(uint fldN, void * pBuf, size_t * pSize) const { return (fldN < Count) ? P_Flds[fldN].getValue(pBuf, pSize) : 0; }
//
// DBDataCell
//
SLAPI DBDataCell::DBDataCell()
{
	THISZERO();
}

SLAPI DBDataCell::DBDataCell(DBConst & r) : C(r)
{
}

SLAPI DBDataCell::DBDataCell(DBE & r) : E(r)
{
}

SLAPI DBDataCell::DBDataCell(DBField & r) : F(r)
{
}

int FASTCALL DBDataCell::containsTblRef(int tid) const
{
	const int _id = GetId();
	if(_id > 0)
		return (_id == tid);
	else if(_id == DBE_ID) {
		for(int i = 0; i < E.Count; i++) {
			if(E.P_Terms[i].tag == DBE::T::fld && E.P_Terms[i].field.Id == tid)
				return 1;
		}
	}
	return 0;
}

#pragma warn -rvl

int FASTCALL DBDataCell::getValue(TYPEID typ, void * val)
{
	int    _id = GetId();
	//CHECK(id > 0 || id == DBConst_ID || id == DBE_ID);
	if(_id > 0)
		return F.getValue(val, 0);
	else if(_id == DBConst_ID)
		return C.convert(typ, val);
	else if(_id == DBE_ID) {
		DBConst cnst;
		cnst.Id = DBConst_ID;
		int    r = E.evaluate(0, &cnst) ? cnst.convert(typ, val) : 0;
		cnst.destroy();
		return r;
	}
	else
		return 0;
}

#pragma warn +rvl

int FASTCALL DBDataCell::getValue(DBConst * val)
{
	int    _id = GetId();
	//CHECK(id > 0 || id == DBConst_ID || id == DBE_ID);
	if(_id > 0)
		return val->fromfld(F);
	else if(_id == DBConst_ID)
		return val->copy(C);
	else
		return E.evaluate(0, val); // Expression (id == DBE_ID)
}

int SLAPI DBDataCell::toString(SString & rBuf, long options) const
{
	rBuf.Z();
	if(GetId() > 0) {
		rBuf.Cat(F.getTable()->GetName()).Dot().Cat(F.getField().Name);
	}
	else if(GetId() == DBConst_ID) {
		char   temp[512];
		C.tostring(0, temp);
		rBuf.Cat(temp);
	}
	else
		rBuf.Cat("DBE");
	return 1;
}
//
// DBQ
//
SLAPI DBQ::DBQ(DBItem & left, int cmp, DBItem & right) : count(1), items(new T)
{
	items[0].flags = 0;
	if(cmp == _OUTER_EQ_) {
		items[0].flags |= DBQ_OUTER;
		cmp = _EQ_;
	}
	items[0].cmp = cmp;
	switch(left.typeOfItem()) {
		case DBConst_ID: items[0].left.C  = *static_cast<DBConst *>(&left);  break;
		case DBE_ID:     items[0].left.E  = *static_cast<DBE *>(&left); break;
		case DBField_ID: items[0].left.F  = *static_cast<DBField *>(&left);  break;
	}
	switch(right.typeOfItem()) {
		case DBConst_ID: items[0].right.C = *static_cast<DBConst *>(&right); break;
		case DBE_ID:     items[0].right.E = *static_cast<DBE *>(&right);     break;
		case DBField_ID: items[0].right.F = *static_cast<DBField *>(&right); break;
	}
	tree = new DBTree(this);
	tree->addLeaf(0, 0, &tree->Root);
}

SLAPI DBQ::DBQ(int logic, DBQ & left, DBQ & right)
{
	int    i, l;
	union {
		int    shift;
		int    r;
	};
	shift = left.count;
	if((items = new T[count = left.count + right.count]) != 0) {
		// @todo memcpy(items, left.items, shift * sizeof(T));
		for(i = 0; i < shift; i++)
			items[i] = left.items[i];
		// @todo memcpy(items+shift, right.items, (count-shift) * sizeof(T));
		for(i = shift; i < (int)count; i++)
			items[i] = right.items[i-shift];
		for(i = 0; i < right.tree->Count; i++)
			if(right.tree->P_Items[i].link == 0)
				right.tree->P_Items[i].term += shift;
		if((tree = new DBTree(this)) != 0) {
			tree->addTree(left.tree, left.tree->Root, &l);
			tree->addTree(right.tree, right.tree->Root, &r);
			tree->addNode(logic, l, r, &tree->Root);
		}
	}
	left.destroy(1);
	right.destroy(1);
	delete &left;
	delete &right;
}

void FASTCALL DBQ::destroy(int withTree)
{
	ZDELETE(items);
	count = 0;
	if(withTree && tree) {
		tree->destroy();
		ZDELETE(tree);
	}
}

int FASTCALL _invertComp(int cmp);

#define INVALID_IDENTIFIER 0

int SLAPI DBQ::testForKey(int itm, int tblID, int * pIsDyn)
{
	const int l = items[itm].left.GetId();
	const int r = items[itm].right.GetId();
	//
	//  люч можно составить только из ограничений типа (thisTable.fld cmp const) или (thisTable.fld cmp prevTable.fld)
	// где cmp - знак сравнени€ (==, <, >, <=, >=)
	//
	*pIsDyn = 0;
	if(l > 0 && r > 0)
		// — обеих сторон ограничени€ сто€т пол€ из текущей таблицы
		if(l == r && l == tblID)
			return 0;
		else
			// ќграничение не содержит ссылок на текущую таблицу
			if(l != tblID && r != tblID)
				return 0;
			else
				*pIsDyn = 1;
	else {
		// ¬ ограничении присутствует выражение, содержащее поле (или пол€) из текущей таблицы
		if(l == DBE_ID) {
			*pIsDyn = 1;
			if(items[itm].left.containsTblRef(tblID))
				return 0;
		}
		if(r == DBE_ID) {
			*pIsDyn = 1;
			if(items[itm].right.containsTblRef(tblID))
				return 0;
		}
	}
	// ≈сли сравнение типа _NE_, индекс не может быть использован
	if(items[itm].cmp == _NE_)
		return 0;
	return 1;
}

int SLAPI DBQ::getPotentialKey(int itm, int tblID, int segment, KR * kr)
{
	int    dyn = 0;
	if(kr->P_h == 0 || testForKey(itm, tblID, &dyn) == 0)
		return 0;
	int    l   = items[itm].left.GetId();
	int    cmp = items[itm].cmp;
	int    seg;
	int    keyPos;
	char   val[MAXKEYLEN];
	TYPEID t;
	BNKey  key;
	DBField f;
	DBDataCell * s;
	if(l > 0 && l == tblID) {
		f = items[itm].left.F;
		s = &items[itm].right;
	}
	else {
		f = items[itm].right.F;
		s = &items[itm].left;
		cmp = _invertComp(cmp);
	}
	t = f.getField().T;
	if(f.getIndex(&key, &keyPos, &seg)) {
		do {
			if(kr->P_h->keyNum == keyPos) {
				int    found = 0;
				if(seg == segment) {
					s->getValue(t, val);
					DBTable * tbl = f.getTable();
					char   dest[2 * MAXKEYLEN + sizeof(KR::I)];
					if(seg > 0) {
						if(kr->first())
							do {
								memcpy(dest, kr->P_b + kr->P_h->current, kr->itemSize(kr->P_b + kr->P_h->current));
								if(tbl->GetIndices().makeKey(keyPos, seg, cmp, val, dest)) {
									kr->remove();
									kr->add(dest);
								   	found = 1;
								}
							} while(++(*kr));
					}
					else {
						if(tbl->GetIndices().makeKey(keyPos, seg, cmp, val, dest))
							if(kr->disjunction(dest) > 0)
								found = 1;
					}
					if(found) {
						if(dyn)
							kr->P_h->pseg |= DYN_KEY;
					}
				}
				else if(seg == segment+1)
					kr->P_h->pseg = (kr->P_h->pseg & DYN_KEY) ? (seg | DYN_KEY) : seg;
				return found;
			}
		} while(f.getIndex(&key, &keyPos, &seg));
	}
	return 0;
}

int FASTCALL DBQ::checkTerm(int itm)
{
	T    * t = &items[itm];
	uint   f = (t->flags & DBQ_LOGIC);
	if(f == DBQ_TRUE)
		return 1;
	else if(f == DBQ_FALSE)
		return 0;
	else {
		DBConst l;
		DBConst r;
		if(!t->left.getValue(&l) || !t->right.getValue(&r))
			return 0;
		else {
			int    cmp = t->cmp;
			int    c = compare(&l, &r);
			l.destroy();
			r.destroy();
			if(c > 0)
				c = oneof2(cmp, _GT_, _GE_);
			else if(c < 0)
				c = oneof2(cmp, _LT_, _LE_);
			else
				c = oneof3(cmp, _EQ_, _LE_, _GE_);
			return c;
		}
	}
}
//
// DBTree
//
SLAPI DBTree::DBTree(DBQ * pOwner)
{
	init(pOwner);
}

void FASTCALL DBTree::init(DBQ * pOwner)
{
	P_Terms = pOwner;
	Count = 0;
	P_Items = 0;
	Root = -1;
}

void SLAPI DBTree::destroy()
{
	ZFREE(P_Items);
	Count = 0;
}

int FASTCALL DBTree::expand(int * pos)
{
	P_Items = (T *)SAlloc::R(P_Items, sizeof(T) * (Count + 1));
	if(P_Items) {
		*pos = Count;
		return TRUE;
	}
	Count = 0;
	return FALSE;
}

int SLAPI DBTree::addLeaf(int term, int flags, int * pPos)
{
	if(expand(pPos)) {
		T    * t = P_Items + *pPos;
		t->link = 0;
		t->term = term;
		t->flags = flags;
		Count++;
		return 1;
	}
	else
		return 0;
}

int SLAPI DBTree::addNode(int link, int left, int right, int * pPos)
{
	if(expand(pPos)) {
		T    * t = P_Items + *pPos;
		t->link = link;
		t->flags = 0;
		t->left = left;
		t->right = right;
		Count++;
		return 1;
	}
	else
		return 0;
}

int SLAPI DBTree::addTree(DBTree * pTree, int p, int * pPos)
{
	int    link, left, right;
	T    & t = pTree->P_Items[p];
	if(t.link == 0)		// Leaf
		 return addLeaf(t.term, t.flags, pPos);
	link = t.link;
	if(!addTree(pTree, t.left, &left) || !addTree(pTree, t.right, &right) || !addNode(link, left, right, pPos))
		return FALSE;
	return TRUE;
}
//
// DBTree::chooseKey возвращает:
// 0 - в случае ошибки
// 1 - произошли изменени€ в dest
// 2 - изменений в dest не произошло
//
int SLAPI DBTree::chooseKey(int n, int tblID, int seg, KR * dest, uint * pTrace)
{
	int    result = 2;
	if(n != -1 && !(P_Items[n].flags & NOKEY)) {
		int    lnk = P_Items[n].link;
		if(lnk == 0) {
			result = P_Terms->getPotentialKey(P_Items[n].term, tblID, seg, dest) ? 1 : 2;
			if(result == 1 && pTrace)
				pTrace[++(*pTrace)] = P_Items[n].term;
		}
		else {
			uint * p_ltrace = 0;
			uint * p_rtrace = 0;
			KR     l;
			l.copy(*dest);
			KR     r;
			r.copy(*dest);
			if(l.P_h == 0 || r.P_h == 0)
				return 0;
			if(pTrace) {
				uint tc = (Count+1) * sizeof(int);
				p_ltrace = (uint *)SAlloc::M(tc);
				p_rtrace = (uint *)SAlloc::M(tc);
				if(p_ltrace == 0 || p_rtrace == 0) {
					l.destroy();
					r.destroy();
					return 0;
				}
				p_ltrace[0] = p_rtrace[0] = 0;
			}
			int    t, _pseg;
			int    lr = chooseKey(P_Items[n].left, tblID, seg, &l, p_ltrace);  // @recursion
			int    rr = chooseKey(P_Items[n].right, tblID, seg, &r, p_rtrace); // @recursion
			if(!lr || !rr)
				result = 0;
			else {
				_pseg = MAX((l.P_h->pseg & ~DYN_KEY), (r.P_h->pseg & ~DYN_KEY));
				if((t = l.link(lnk, r)) == 0)
					result = 0;
				else {
					// CHECK(t != -1);	// ѕротиворечивые ограничени€ //
					if(seg == 0 || t == 1) {
						if(pTrace) {
							int    lc = p_ltrace[0];
							int    rc = p_rtrace[0];
							memcpy(pTrace + 1, p_ltrace + 1, sizeof(int) * lc);
							memcpy(pTrace + lc + 1, p_rtrace + 1, sizeof(int) * rc);
							pTrace[0] = lc + rc;
						}
						dest->copy(l);
						result = 1;
					}
				}
				//t = MAX((l.h->pseg & ~DYN_KEY), (r.h->pseg & ~DYN_KEY));
				//dest->h->pseg = t | (dest->h->pseg & DYN_KEY);
				dest->P_h->pseg = _pseg | (dest->P_h->pseg & DYN_KEY);
				l.destroy();
				r.destroy();
			}
			SAlloc::F(p_ltrace);
			SAlloc::F(p_rtrace);
		}
	}
	return result;
}

#define INVALID_DBTREE_ITEMS_LINK 0

int FASTCALL DBTree::checkRestriction(int node)
{
	int    ok = 0;
	if(Root == -1)
		ok = 1;
	else {
		T & t = P_Items[(node == -1) ? Root : node];
		assert(oneof3(t.link, 0, _OR___, _AND___));
		switch(t.link) {
			case _OR___:
				ok = (checkRestriction(t.left) || checkRestriction(t.right)); // @recursion
				break;
			case _AND___:
				ok = (checkRestriction(t.left) && checkRestriction(t.right)); // @recursion
				break;
			case 0:
				ok = P_Terms->checkTerm(t.term);
				break;
		}
	}
	return ok;
}

int SLAPI DBTree::CreateSqlExpr(Generator_SQL * pGen, int node) const
{
	int    ok = 1;
	if(Root != -1) {
		T & t = P_Items[(node == -1) ? Root : node];
		switch(t.link) {
			case _OR___:
				pGen->LPar();
				CreateSqlExpr(pGen, t.left); // @recursion
				pGen->Sp().Tok(Generator_SQL::tokOr).Sp();
				CreateSqlExpr(pGen, t.right); // @recursion
				pGen->RPar();
				break;
			case _AND___:
				pGen->LPar();
				CreateSqlExpr(pGen, t.left); // @recursion
				pGen->Sp().Tok(Generator_SQL::tokAnd).Sp();
				CreateSqlExpr(pGen, t.right); // @recursion
				pGen->RPar();
				break;
			case 0:
				P_Terms->CreateSqlExpr(pGen, t.term);
				break;
			default:
				;//CHECK(INVALID_DBTREE_ITEMS_LINK);
		}
	}
	return ok;
}

int SLAPI DBDataCell::CreateSqlExpr(Generator_SQL * pGen) const
{
	if(GetId() > 0) {
		pGen->/*Text(f.getTable()->tableName).Text(".").*/Text(F.getField().Name);
	}
	else if(GetId() == DBConst_ID) {
		char   temp[512];
		C.tostring(COMF_SQL, temp);
		pGen->Text(temp);
	}
	else if(GetId() == DBE_ID)
		pGen->Text("(DBE)");
	else
		pGen->Text("err");
	return 1;
}

int SLAPI DBQ::CreateSqlExpr(Generator_SQL * pGen, int itm) const
{
	int    ok = 1;
	T    * t = &items[itm];
	uint   f = (t->flags & DBQ_LOGIC);
	if(f == DBQ_TRUE)
		pGen->Text("1=1");
	else if(f == DBQ_FALSE)
		pGen->Text("1=0");
	else {
		t->left.CreateSqlExpr(pGen);
		//pGen->Sp();
		pGen->_Symb(t->cmp);
		//pGen->Sp();
		t->right.CreateSqlExpr(pGen);
	}
	return ok;
}
