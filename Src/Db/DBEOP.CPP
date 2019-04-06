// DBEOP.CPP
// Copyright (c) Sobolev A. 1995-2001, 2005, 2007, 2008, 2009, 2010, 2014, 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>

enum {
	op_plus,
	op_minus,
	op_mult,
	op_div,
	op_rem,
	op_bw_and
};

struct _2op_entry {
	char op;
	char typ1;
	char typ2;
	char func;
} _2op_tab[] = {
	{ op_plus, BTS_INT, BTS_INT, dbq_add_ii },
	{ op_plus, BTS_INT, BTS_REAL, dbq_add_rr },
	{ op_plus, BTS_REAL, BTS_REAL, dbq_add_rr },
	{ op_plus, BTS_REAL, BTS_INT, dbq_add_rr },
	{ op_plus, BTS_STRING,  BTS_STRING,  dbq_add_ss },
	{ op_plus, BTS_DATE, BTS_INT, dbq_add_di },
	{ op_plus, BTS_DATE, BTS_REAL, dbq_add_di },
	{ op_minus, BTS_INT, BTS_INT, dbq_sub_ii },
	{ op_minus, BTS_INT, BTS_REAL, dbq_sub_rr },
	{ op_minus, BTS_REAL, BTS_REAL, dbq_sub_rr },
	{ op_minus, BTS_REAL, BTS_INT, dbq_sub_rr },
	{ op_minus, BTS_DATE, BTS_DATE, dbq_sub_dd },
	{ op_minus, BTS_TIME, BTS_TIME, dbq_sub_tt },
	{ op_mult,  BTS_INT, BTS_INT, dbq_mul_ii },
	{ op_mult,  BTS_INT, BTS_REAL, dbq_mul_rr },
	{ op_mult,  BTS_REAL, BTS_REAL, dbq_mul_rr },
	{ op_mult,  BTS_REAL, BTS_INT, dbq_mul_rr },
	{ op_div,  BTS_INT, BTS_INT, dbq_div_ii },
	{ op_div,  BTS_INT, BTS_REAL, dbq_div_rr },
	{ op_div,  BTS_REAL, BTS_REAL, dbq_div_rr },
	{ op_div,  BTS_REAL, BTS_INT, dbq_div_rr },
	{ op_rem,  BTS_INT, BTS_INT, dbq_rem_ii },
	{ op_bw_and, BTS_INT, BTS_INT, dbq_and }
};

#define _2op_tab_siz (sizeof(_2op_tab) / sizeof(_2op_entry))

static DBFunc FASTCALL get_2op(int op, int bt1, int bt2)
{
	for(_2op_entry * e = _2op_tab; e < _2op_tab + _2op_tab_siz; e++) {
		if(e->op == op && e->typ1 == bt1 && e->typ2 == bt2)
			return static_cast<DBFunc>(e->func);
	}
	return dbq_error;
}

static DBE & FASTCALL newDBE()
{
	DBE & e = *new DBE;
	e.init();
	return e;
}

static int FASTCALL adjust_type(DBItem & i, int baseTyp, int paramTyp)
{
	if(i.Id == DBConst_ID && paramTyp != baseTyp) {
		DBConst & c = *static_cast<DBConst *>(&i);
		if(baseTyp == BTS_REAL && paramTyp == BTS_INT) {
			c.init(static_cast<double>(c.lval));
		}
		else if(baseTyp == BTS_INT && paramTyp == BTS_REAL) {
			c.init(static_cast<long>(c.rval));
		}
		else
			return 0;
	}
	return 1;
}

static DBE & FASTCALL unary_func(DBItem & i, DBFunc f)
{
	DBE & e = newDBE();
	e.push(i);
	e.push(f);
	return e;
}

static DBE & FASTCALL binary_func(DBItem & i1, DBItem & i2, DBFunc f)
{
	DBE & e = newDBE();
	e.push(i1);
	e.push(i2);
	e.push(f);
	return e;
}

#pragma warn -aus

static DBE & FASTCALL binary_op(DBItem & i1, DBItem & i2, int op)
{
	DBFunc f = get_2op(op, i1.baseType(), i2.baseType());
	assert(f != dbq_error);
	DBFuncInfo * fi = DbqFuncTab::Get(f);
	assert(fi != 0);
	int adj_type;
	adj_type = adjust_type(i1, fi->paramTypes[0], i1.baseType());
	assert(adj_type != 0);
	adj_type = adjust_type(i2, fi->paramTypes[1], i2.baseType());
	assert(adj_type != 0);
	return binary_func(i1, i2, f);
}

#pragma warn .aus

static DBE & FASTCALL binary_op(DBItem & i, double r, int op)
{
	DBConst c;
	c.init(r);
	return binary_op(i, c, op);
}

static DBE & FASTCALL binary_op(double r, DBItem & i, int op)
{
	DBConst c;
	if(ffrac(r) == 0.0)
		c.init(static_cast<long>(r));
	else
		c.init(r);
	return binary_op(c, i, op);
}

static DBE & FASTCALL binary_op(DBItem & i, long l, int op) { DBConst c; c.init(l); return binary_op(i, c, op); }
static DBE & FASTCALL binary_op(long l, DBItem & i, int op) { DBConst c; c.init(l); return binary_op(c, i, op); }

DBE & __stdcall operator + (DBItem & i1, DBItem & i2) { return binary_op(i1, i2, op_plus);  }
DBE & __stdcall operator - (DBItem & i1, DBItem & i2) { return binary_op(i1, i2, op_minus); }
DBE & __stdcall operator * (DBItem & i1, DBItem & i2) { return binary_op(i1, i2, op_mult);  }
DBE & __stdcall operator / (DBItem & i1, DBItem & i2) { return binary_op(i1, i2, op_div);   }
DBE & __stdcall operator % (DBItem & i1, DBItem & i2) { return binary_op(i1, i2, op_rem);   }
DBE & __stdcall operator + (DBItem & i,  double  r)  { return binary_op(i,  r,  op_plus);  }
DBE & __stdcall operator - (DBItem & i,  double  r)  { return binary_op(i,  r,  op_minus); }
DBE & __stdcall operator * (DBItem & i,  double  r)  { return binary_op(i,  r,  op_mult);  }
DBE & __stdcall operator / (DBItem & i,  double  r)  { return binary_op(i,  r,  op_div);   }
DBE & __stdcall operator % (DBItem & i,  long    l)  { return binary_op(i,  l,  op_rem);   }
DBE & __stdcall operator & (DBItem & i,  long    l)  { return binary_op(i,  l,  op_bw_and); }
DBE & __stdcall operator & (DBItem & i1, DBItem & i2) { return binary_op(i1, i2, op_bw_and); }
DBE & __stdcall operator + (double  r,  DBItem & i) { return binary_op(r,  i,  op_plus);  }
DBE & __stdcall operator - (double  r,  DBItem & i) { return binary_op(r,  i,  op_minus); }
DBE & __stdcall operator * (double  r,  DBItem & i) { return binary_op(r,  i,  op_mult);  }
DBE & __stdcall operator / (double  r,  DBItem & i) { return binary_op(r,  i,  op_div);   }
DBE & __stdcall operator % (long    l,  DBItem & i) { return binary_op(l,  i,  op_rem);   }
DBE & __stdcall operator + (const char * s,  DBItem & i) { DBConst c; c.init(s); return binary_op(c, i, op_plus); }
DBE & __stdcall operator + (DBItem & i, const char * s)   { DBConst c; c.init(s); return binary_op(i, c, op_plus); }
DBE & __stdcall operator + (LDATE   d,  DBItem & i) { DBConst c; c.init(d); return binary_op(c, i, op_plus); }
DBE & __stdcall operator - (LDATE   d,  DBItem & i) { DBConst c; c.init(d); return binary_op(c, i, op_minus); }
DBE & __stdcall operator - (DBItem & i,  LDATE   d) { DBConst c; c.init(d); return binary_op(i, c, op_minus); }
DBE & __stdcall operator - (LTIME   t,  DBItem & i) { DBConst c; c.init(t); return binary_op(c, i, op_minus); }
DBE & __stdcall operator - (DBItem & i,  LTIME   t) { DBConst c; c.init(t); return binary_op(i, c, op_minus); }
DBE & __stdcall sstrlen(DBItem & i) { assert(i.baseType() == BTS_STRING); return unary_func(i, dbq_strlen); }
DBE & __stdcall ltrim(DBItem & i)  { assert(i.baseType() == BTS_STRING); return unary_func(i, dbq_ltrim); }
DBE & __stdcall rtrim(DBItem & i)  { assert(i.baseType() == BTS_STRING); return unary_func(i, dbq_rtrim); }
DBE & __stdcall ltoa(DBItem & i)   { assert(i.baseType() == BTS_INT); return unary_func(i, dbq_ltoa); }

DBE & __stdcall contains(DBItem & i, const char * s)
{
	assert(i.baseType() == BTS_STRING);
	DBE & e = newDBE();
	e.push(i);
	e.push(dbconst(s));
	e.push(static_cast<DBFunc>(dbq_contains));
	return e;
}

DBE & __stdcall enumtoa(DBItem & i, int n, char ** str_array)
{
	assert(i.baseType() == BTS_INT);
	DBConst ic;
	DBConst sc;
	ic.init(static_cast<long>(n));
	sc.init(reinterpret_cast<long>(str_array)); // @todo Использовать преобразование к DBConst::ptrval
	DBE & e = newDBE();
	e.push(i);
	e.push(ic);
	e.push(sc);
	e.push(static_cast<DBFunc>(dbq_enumtoa));
	return e;
}

DBE & __stdcall flagtoa(DBItem & i, long mask, char ** str_array)
{
	assert(i.baseType() == BTS_INT);
	DBConst ic;
	DBConst sc;
	ic.init(mask);
	sc.init(reinterpret_cast<long>(str_array)); // @todo Использовать преобразование к DBConst::ptrval
	DBE & e = newDBE();
	e.push(i);
	e.push(ic);
	e.push(sc);
	e.push(static_cast<DBFunc>(dbq_flagtoa));
	return e;
}
//
//
//
DBE & __stdcall left(DBItem & i, int n)
{
	assert(i.baseType() == BTS_STRING);
	DBConst c;
	c.init(static_cast<long>(n));
	return binary_func(i, c, dbq_left);
}

DBE & __stdcall right(DBItem & i, int n)
{
	assert(i.baseType() == BTS_STRING);
	DBConst c;
	c.init(static_cast<long>(n));
	return binary_func(i, c, dbq_right);
}

DBE & __stdcall lower(DBItem & i)  { assert(i.baseType() == BTS_STRING); return unary_func(i, dbq_lower); }
DBE & __stdcall upper(DBItem & i)  { assert(i.baseType() == BTS_STRING); return unary_func(i, dbq_upper); }
DBE & __stdcall day(DBItem & i)    { assert(i.baseType() == BTS_DATE); return unary_func(i, dbq_day); }
DBE & __stdcall month(DBItem & i)  { assert(i.baseType() == BTS_DATE); return unary_func(i, dbq_mon); }
DBE & __stdcall year(DBItem & i)   { assert(i.baseType() == BTS_DATE); return unary_func(i, dbq_year); }
DBE & __stdcall hour(DBItem & i)   { assert(i.baseType() == BTS_TIME); return unary_func(i, dbq_hour); }
DBE & __stdcall minute(DBItem & i) { assert(i.baseType() == BTS_TIME); return unary_func(i, dbq_minute); }
DBE & __stdcall millisecond(DBItem & i) { assert(i.baseType() == BTS_TIME); return unary_func(i, dbq_millisecond); }
//
//
//
static DBQ & FASTCALL _newdbq(DBItem & i1, int op, DBItem & i2)
{
	return *new DBQ(i1, op, i2);
}

static DBQ & FASTCALL _newdbq(DBItem & i, int op, double d)
{
	DBConst c;
	if(ffrac(d) == 0.0)
		c.init(static_cast<long>(d));
	else
		c.init(d);
	return *new DBQ(i, op, c);
}

static DBQ & FASTCALL _newdbq(DBItem & i, int op, long l)
{
	DBConst c;
	c.init(l);
	return *new DBQ(i, op, c);
}

static DBQ & FASTCALL _newdbq(DBItem & i, int op, LDATE d)
{
	DBConst c;
	c.init(d);
	return *new DBQ(i, op, c);
}

static DBQ & FASTCALL _newdbq(DBItem & i, int op, LTIME t)
{
	DBConst c;
	c.init(t);
	return *new DBQ(i, op, c);
}

static DBQ & FASTCALL _newdbq(DBItem & i, int op, LDATETIME t)
{
	DBConst c;
	c.init(t);
	return *new DBQ(i, op, c);
}

static DBQ & FASTCALL _newdbq(DBItem & i, int op, const char * s)
{
	DBConst c;
	c.init(s);
	return *new DBQ(i, op, c);
}

DBQ & __stdcall operator == (DBItem & i1, DBItem & i2) { return _newdbq(i1, _EQ_, i2); }
DBQ & __stdcall operator += (DBItem & i1, DBItem & i2) { return _newdbq(i1, _OUTER_EQ_, i2); }
DBQ & __stdcall operator != (DBItem & i1, DBItem & i2) { return _newdbq(i1, _NE_, i2); }
DBQ & __stdcall operator  < (DBItem & i1, DBItem & i2) { return _newdbq(i1, _LT_, i2); }
DBQ & __stdcall operator <= (DBItem & i1, DBItem & i2) { return _newdbq(i1, _LE_, i2); }
DBQ & __stdcall operator  > (DBItem & i1, DBItem & i2) { return _newdbq(i1, _GT_, i2); }
DBQ & __stdcall operator >= (DBItem & i1, DBItem & i2) { return _newdbq(i1, _GE_, i2); }
DBQ & __stdcall operator == (DBItem & i, double d) { return _newdbq(i, _EQ_, d); }
DBQ & __stdcall operator != (DBItem & i, double d) { return _newdbq(i, _NE_, d); }
DBQ & __stdcall operator  < (DBItem & i, double d) { return _newdbq(i, _LT_, d); }
DBQ & __stdcall operator <= (DBItem & i, double d) { return _newdbq(i, _LE_, d); }
DBQ & __stdcall operator  > (DBItem & i, double d) { return _newdbq(i, _GT_, d); }
DBQ & __stdcall operator >= (DBItem & i, double d) { return _newdbq(i, _GE_, d); }
DBQ & __stdcall operator == (DBItem & i, long v)   { return _newdbq(i, _EQ_, v); }
DBQ & __stdcall operator <  (DBItem & i, long v)   { return _newdbq(i, _LT_, v); }
DBQ & __stdcall operator >  (DBItem & i, long v)   { return _newdbq(i, _GT_, v); }
DBQ & __stdcall operator <= (DBItem & i, long v)   { return _newdbq(i, _LE_, v); }
DBQ & __stdcall operator >= (DBItem & i, long v)   { return _newdbq(i, _GE_, v); }
DBQ & __stdcall operator == (DBItem & i, LDATE d) { return _newdbq(i, _EQ_, d); }
DBQ & __stdcall operator != (DBItem & i, LDATE d) { return _newdbq(i, _NE_, d); }
DBQ & __stdcall operator  < (DBItem & i, LDATE d) { return _newdbq(i, _LT_, d); }
DBQ & __stdcall operator <= (DBItem & i, LDATE d) { return _newdbq(i, _LE_, d); }
DBQ & __stdcall operator  > (DBItem & i, LDATE d) { return _newdbq(i, _GT_, d); }
DBQ & __stdcall operator >= (DBItem & i, LDATE d) { return _newdbq(i, _GE_, d); }
DBQ & __stdcall operator == (DBItem & i, LTIME t) { return _newdbq(i, _EQ_, t); }
DBQ & __stdcall operator != (DBItem & i, LTIME t) { return _newdbq(i, _NE_, t); }
DBQ & __stdcall operator  < (DBItem & i, LTIME t) { return _newdbq(i, _LT_, t); }
DBQ & __stdcall operator <= (DBItem & i, LTIME t) { return _newdbq(i, _LE_, t); }
DBQ & __stdcall operator  > (DBItem & i, LTIME t) { return _newdbq(i, _GT_, t); }
DBQ & __stdcall operator >= (DBItem & i, LTIME t) { return _newdbq(i, _GE_, t); }
DBQ & __stdcall operator == (DBItem & i, LDATETIME t) { return _newdbq(i, _EQ_, t); }
DBQ & __stdcall operator != (DBItem & i, LDATETIME t) { return _newdbq(i, _NE_, t); }
DBQ & __stdcall operator  < (DBItem & i, LDATETIME t) { return _newdbq(i, _LT_, t); }
DBQ & __stdcall operator <= (DBItem & i, LDATETIME t) { return _newdbq(i, _LE_, t); }
DBQ & __stdcall operator  > (DBItem & i, LDATETIME t) { return _newdbq(i, _GT_, t); }
DBQ & __stdcall operator >= (DBItem & i, LDATETIME t) { return _newdbq(i, _GE_, t); }
DBQ & __stdcall operator == (DBItem & i, const char * s) { return _newdbq(i, _EQ_, s); }
DBQ & __stdcall operator != (DBItem & i, const char * s) { return _newdbq(i, _NE_, s); }
DBQ & __stdcall operator  < (DBItem & i, const char * s) { return _newdbq(i, _LT_, s); }
DBQ & __stdcall operator <= (DBItem & i, const char * s) { return _newdbq(i, _LE_, s); }
DBQ & __stdcall operator  > (DBItem & i, const char * s) { return _newdbq(i, _GT_, s); }
DBQ & __stdcall operator >= (DBItem & i, const char * s) { return _newdbq(i, _GE_, s); }
DBQ &  __stdcall operator && (DBQ & left, DBQ & right)
	{ return ((&left) == 0) ? right : (((&right) == 0) ? left : *new DBQ(_AND___, left, right)); }
DBQ &  __stdcall operator || (DBQ & l, DBQ & r)
	{ return (&l && &r) ? *new DBQ(_OR___,  l, r) : *reinterpret_cast<DBQ *>(0); }
DBQ & __stdcall daterange(DBItem & i, const DateRange * pDR)
	{ return pDR ? daterange(i, pDR->low, pDR->upp) : *reinterpret_cast<DBQ *>(0); }

DBQ & __stdcall daterange(DBItem & i, LDATE beg, LDATE end)
{
	if(i.baseType() == BTS_DATETIME) {
		LDATETIME _low, _upp;
		_low.Set(beg, ZEROTIME);
		_upp.Set(NZOR(end, MAXDATE), MAXDAYTIME);
		if(beg == end) {
			return (i >= _low && i <= _upp);
		}
		else {
			DBQ * dbq = 0;
			if(beg)
				dbq = & (i >= _low);
			if(end)
				dbq = & (*dbq && i <= _upp);
			return *dbq;
		}
	}
	else {
		if(beg == end)
			return beg ? (i == beg) : *reinterpret_cast<DBQ *>(0);
		else {
			DBQ * dbq = 0;
			if(beg)
				dbq = & (i >= beg);
			if(end)
				dbq = & (*dbq && i <= end);
			return *dbq;
		}
	}
}

DBQ & __stdcall timerange(DBItem & i, const TimeRange * pTR)
	{ return pTR ? timerange(i, pTR->low, pTR->upp) : *reinterpret_cast<DBQ *>(0); }

DBQ & __stdcall timerange(DBItem & i, LTIME beg, LTIME end)
{
	if(beg == end)
		return beg ? (i == beg) : *reinterpret_cast<DBQ *>(0);
	else {
		DBQ * dbq = 0;
		if(beg)
			dbq = & (i >= beg);
		if(end)
			dbq = & (*dbq && i <= end);
		return *dbq;
	}
}

DBQ & __stdcall realrange(DBItem & i, double lo, double up)
{
	if(lo == up)
		return lo ? (i == lo) : *reinterpret_cast<DBQ *>(0);
	else {
		DBQ * dbq = 0;
		if(lo)
			dbq = & (i >= lo);
		if(up)
			dbq = & (*dbq && i <= up);
		return *dbq;
	}
}

DBQ & __stdcall intrange(DBItem & i, const IntRange & rR)
{
	if(rR.low == rR.upp)
		return rR.low ? (i == static_cast<long>(rR.low)) : *reinterpret_cast<DBQ *>(0);
	else {
		DBQ * dbq = 0;
		if(rR.low)
			dbq = & (i >= static_cast<long>(rR.low));
		if(rR.upp)
			dbq = & (*dbq && i <= static_cast<long>(rR.upp));
		return *dbq;
	}
}

#undef INLINE
//
// Определения функций
//
#define RES(p)         result->init(p)
#define RRES           result->rval
#define IRES           result->lval
#define SRES           result->sptr
#define DRES           result->dval
#define TRES           result->tval
#define RPAR(no)       ((params[no].Tag == DBConst::rv) ? params[no].rval : params[no].lval)
#define IPAR(no)       static_cast<long>(params[no].lval)
#define SPAR(no)       params[no].sptr
#define DPAR(no)       params[no].dval
#define TPAR(no)       params[no].tval

#pragma warn -par

IMPL_DBE_PROC(_add_ii) { RES(IPAR(0)+IPAR(1)); }
IMPL_DBE_PROC(_sub_ii) { RES(IPAR(0)-IPAR(1)); }
IMPL_DBE_PROC(_mul_ii) { RES(IPAR(0)*IPAR(1)); }

IMPL_DBE_PROC(_div_ii)
{
	long v = IPAR(0);
	long d = IPAR(1);
	long r = d ? (v/d) : 0;
	RES(r);
}

IMPL_DBE_PROC(_rem_ii) { RES(IPAR(0)%IPAR(1)); }
IMPL_DBE_PROC(_add_rr) { RES(RPAR(0)+RPAR(1)); }
IMPL_DBE_PROC(_sub_rr) { RES(RPAR(0)-RPAR(1)); }
IMPL_DBE_PROC(_mul_rr) { RES(RPAR(0)*RPAR(1)); }
IMPL_DBE_PROC(_div_rr) { RES(fdivnz(RPAR(0), RPAR(1))); }
IMPL_DBE_PROC(_and_ii) { RES(IPAR(0) & IPAR(1)); }
IMPL_DBE_PROC(_or_ii)  { RES(IPAR(0) | IPAR(1)); }
IMPL_DBE_PROC(_xor_ii) { RES(IPAR(0) ^ IPAR(1)); }
IMPL_DBE_PROC(_not_ii) { RES(~IPAR(0)); }

IMPL_DBE_PROC(_ltoa_i)
{
	if(option == CALC_SIZE)
		RES(12L);
	else {
		char s[48];
		ltoa(IPAR(0), s, 10)[12-1] = 0;
		RES(s);
	}
}

IMPL_DBE_PROC(_enumtoa_iii)
{
	int    i;
	const char ** p = reinterpret_cast<const char **>(IPAR(2));
	if(option == CALC_SIZE) {
		size_t m = 1;
		for(i = 0; i < IPAR(1); i++) {
			if((sstrlen(p[i]+2)+1) > m)
				m = sstrlen(p[i]+2) + 1;
		}
		RES(static_cast<long>(m));
	}
	else {
		for(i = 0; i < IPAR(1); i++) {
			if(*reinterpret_cast<const int16*>(p[i]) == IPAR(0)) {
				RES(p[i]+2);
				return;
			}
		}
		RES(reinterpret_cast<char *>(&(i = 0)));
	}
}

IMPL_DBE_PROC(_flagtoa_iii)
{
	int    i;
	const char ** p = reinterpret_cast<const char **>(IPAR(2));
	if(option == CALC_SIZE) {
		uint   m = 1;
		for(i = 0; i < 2; i++)
			if((sstrlen(p[i]+2)+1) > m)
				m = sstrlen(p[i]+2) + 1;
		RES(static_cast<long>(m));
	}
	else {
		if(IPAR(0) & IPAR(1)) {
			RES(p[1]+2);
		}
		else {
			RES(p[0]+2);
		}
	}
}

IMPL_DBE_PROC(_add_ss)
{
	if(option == CALC_SIZE)
		RES(IPAR(0)+IPAR(1));
	else {
		const size_t len = sstrlen(SPAR(0))+sstrlen(SPAR(1));
		char * s = static_cast<char *>(SAlloc::M(len+1));
		strcpy(stpcpy(s, SPAR(0)), SPAR(1));
		if(len > 254)
			s[254] = 0;
		RES(s);
		SAlloc::F(s);
	}
}

IMPL_DBE_PROC(_left_si)
{
	if(option == CALC_SIZE)
		if(IPAR(1) == 0)
			RES(IPAR(0));
		else
			RES(IPAR(1)+1L);
	else {
		SPAR(0)[IPAR(1)] = 0;
		RES(SPAR(0));
	}
}

IMPL_DBE_PROC(_right_si)
{
	if(option == CALC_SIZE)
		if(IPAR(1) == 0)
			RES(IPAR(0));
		else
			RES(IPAR(1)+1L);
	else
		RES(SPAR(0)+IPAR(1));
}

IMPL_DBE_PROC(_ltrim_s)
{
	if(option == CALC_SIZE)
		RES(IPAR(0));
	else
		RES(trimleft(SPAR(0)));
}

IMPL_DBE_PROC(_rtrim_s)
{
	if(option == CALC_SIZE)
		RES(IPAR(0));
	else
		RES(trimright(SPAR(0)));
}

IMPL_DBE_PROC(_strlen_s)
{
	RES((long)sstrlen(SPAR(0)));
}

IMPL_DBE_PROC(_contains_s)
{
	RES((long)(BIN(strstr(SPAR(0), SPAR(1)))));
}

IMPL_DBE_PROC(_lower_s)
{
	if(option == CALC_SIZE)
		RES(IPAR(0));
	else
		RES(strlwr866(SPAR(0)));
}

IMPL_DBE_PROC(_upper_s)
{
	if(option == CALC_SIZE)
		RES(IPAR(0));
	else
		RES(strupr866(SPAR(0)));
}

IMPL_DBE_PROC(_day_d)  {int d,m,y; decodedate(&d, &m, &y, &DPAR(0)); RES(static_cast<long>(d));}
IMPL_DBE_PROC(_mon_d)  {int d,m,y; decodedate(&d, &m, &y, &DPAR(0)); RES(static_cast<long>(m));}
IMPL_DBE_PROC(_year_d) {int d,m,y; decodedate(&d, &m, &y, &DPAR(0)); RES(static_cast<long>(y));}
IMPL_DBE_PROC(_weekday_d) { RES(static_cast<long>(dayofweek(&DPAR(0), 1))); }
IMPL_DBE_PROC(_add_di) 
{ 
	LDATE dt = DPAR(0);
	plusdate(&dt, IPAR(1), 0); 
	RES(dt);
	//result->copy(params[0]); 
}
IMPL_DBE_PROC(_sub_dd) { RES(diffdate(&DPAR(0), &DPAR(1), 0)); }

IMPL_DBE_PROC(_count_v)
{
	switch(option) {
		case AGGR_BEGIN: RES(0L); break;
		case AGGR_NEXT: IRES++; break;
		case AGGR_END: break;
	}
}

#pragma warn +par

IMPL_DBE_PROC(_sum_i)
{
	switch(option) {
		case AGGR_BEGIN: RES(0L); break;
		case AGGR_NEXT: IRES+=IPAR(0); break;
		case AGGR_END: break;
	}
}

IMPL_DBE_PROC(_sum_r)
{
	switch(option) {
		case AGGR_BEGIN: RES(0.0); break;
		case AGGR_NEXT: RRES+=RPAR(0); break;
		case AGGR_END: break;
	}
}

IMPL_DBE_PROC(_avg_r)
{
	struct Temp { 
		double sum; 
		long   count; 
	};
	Temp * p_temp = 0;
	switch(option) {
		case AGGR_BEGIN:
			result->sptr = new char[sizeof(Temp)];
			p_temp = reinterpret_cast<Temp*>(result->sptr);
			p_temp->count = 0L;
			p_temp->sum = 0.0;
			break;
		case AGGR_NEXT:
			p_temp = reinterpret_cast<Temp*>(result->sptr);
			p_temp->count++;
			p_temp->sum += RPAR(0);
			break;
		case AGGR_END:
			p_temp = reinterpret_cast<Temp*>(result->sptr);
			double avg = p_temp->sum / p_temp->count;
			delete [] result->sptr;
			result->sptr = 0;
			RES(avg);
			break;
	}
}

IMPL_DBE_PROC(_min_v)
{
	int    bt = params[0].Tag;
	int    st = bt2st(bt);
	void * p;
	void * rp;
	switch(option) {
		case CALC_SIZE:
			if(bt == BTS_STRING)
				RES(IPAR(0));
			break;
		case AGGR_BEGIN:
			result->Id  = DBConst_ID;
			result->Flags = 0;
			result->Tag = /*(DBConst::_tag)*/bt;
			if(bt == BTS_STRING)
				rp = result->sptr = newStr(" ");
			else
				rp = &IRES;
			stmaxval(st, rp);
			break;
		case AGGR_NEXT:
			p  = ((bt == BTS_STRING) ? (void *)SPAR(0) : (void *)&IPAR(0));
			rp = ((bt == BTS_STRING) ? (void *)SRES : (void *)&IRES);
			if(stcomp(st, p, rp) < 0) {
				result->destroy();
				result->copy(params[0]);
			}
			break;
		case AGGR_END:
			break;
	}
}

IMPL_DBE_PROC(_max_v)
{
	int    bt = params[0].Tag;
	int    st = bt2st(bt);
	void * p;
	void * rp;
	switch(option) {
		case CALC_SIZE:
			if(bt == BTS_STRING)
				RES(IPAR(0));
			break;
		case AGGR_BEGIN:
			result->Id  = DBConst_ID;
			result->Flags = 0;
			result->Tag = /*(DBConst::_tag)*/bt;
			if(bt == BTS_STRING)
				rp = result->sptr = newStr(" ");
			else
				rp = &IRES;
			stminval(st, rp);
			break;
		case AGGR_NEXT:
			p  = ((bt == BTS_STRING) ? (void *)SPAR(0) : (void *)&IPAR(0));
			rp = ((bt == BTS_STRING) ? (void *)SRES : (void *)&IRES);
			if(stcomp(st, p, rp) > 0) {
				result->destroy();
				result->copy(params[0]);
			}
			break;
		case AGGR_END:
			break;
	}
}
//
// DbqFuncTab
//

//static
DbqFuncTab DbqFuncTab::Inst; // @global @threadsafe

//static
int SLAPIV DbqFuncTab::RegisterDyn(int * pFuncId, int retType, DBQProc pProc, int paramCount, ...)
{
	int    ok = 0, func_id = 0;
	ENTER_CRITICAL_SECTION
	if(*pFuncId == 0) {
		if(Inst.GetDynFuncId(&func_id) > 0) {
			va_list param_list;
			va_start(param_list, paramCount);
			if(Inst._RegisterFunc(func_id, 0, retType, pProc, paramCount, param_list))
				ok = 1;
			va_end(param_list);
		}
		ASSIGN_PTR(pFuncId, func_id);
	}
	else
		ok = 1;
	LEAVE_CRITICAL_SECTION
	return ok;
}

//static
int SLAPIV DbqFuncTab::RegisterDynAggr(int * pFuncId, int retType, DBQProc pProc, int paramCount, ...)
{
	int    ok = 0, func_id = 0;
	ENTER_CRITICAL_SECTION
	if(*pFuncId == 0) {
		if(Inst.GetDynFuncId(&func_id) > 0) {
			va_list param_list;
			va_start(param_list, paramCount);
			if(Inst._RegisterFunc(func_id, 1, retType, pProc, paramCount, param_list))
				ok = 1;
			va_end(param_list);
		}
		ASSIGN_PTR(pFuncId, func_id);
	}
	else
		ok = 1;
	LEAVE_CRITICAL_SECTION
	return ok;
}

//static
DBFuncInfo * FASTCALL DbqFuncTab::Get(int funcId)
{
	return Inst.GetFuncPtr(funcId);
}

SLAPI DbqFuncTab::DbqFuncTab() : P_Tab(0)
{
	PreRegister();
}

SLAPI DbqFuncTab::~DbqFuncTab()
{
	delete P_Tab;
}

int SLAPI DbqFuncTab::GetDynFuncId(int * pId)
{
	for(int i = FIRST_DYN_DBQFUNC_ID; i <= LAST_DYN_DBQFUNC_ID; i++)
		if(SearchFunc(i, 0) < 0) {
			ASSIGN_PTR(pId, i);
			return 1;
		}
	return 0;
}

int SLAPIV DbqFuncTab::RegisterFunc(int funcId, int isAggr, int retType, DBQProc pProc, int paramCount, ...)
{
	va_list param_list;
	va_start(param_list, paramCount);
	int ok = _RegisterFunc(funcId, isAggr, retType, pProc, paramCount, param_list);
	va_end(param_list);
	return ok;
}

int SLAPI DbqFuncTab::_RegisterFunc(int funcId, int isAggr, int retType, DBQProc pProc, int paramCount, va_list paramList)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	DBFuncInfo entry;
	MEMSZERO(entry);
	entry.func = funcId;
	entry.aggr = isAggr;
	entry.typ  = retType;
	entry.proc = pProc;
	entry.paramCount = paramCount;
	for(int i = 0; i < paramCount && i < MAX_DBQ_PARAMS; i++)
		entry.paramTypes[i] = va_arg(paramList, int);
	SETIFZ(P_Tab, new SArray(sizeof(DBFuncInfo)));
	if(!P_Tab || !P_Tab->ordInsert(&entry, 0, PTR_CMPFUNC(int)))
		ok = 0;
	LEAVE_CRITICAL_SECTION
	return ok;
}

DBFuncInfo * FASTCALL DbqFuncTab::GetFuncPtr(int funcId)
{
	if(P_Tab) {
		uint   pos = 0;
		if(P_Tab->bsearch(&funcId, &pos, CMPF_LONG))
			return static_cast<DBFuncInfo *>(P_Tab->at(pos));
	}
	return 0;
}

int SLAPI DbqFuncTab::SearchFunc(int funcId, DBFuncInfo * pInfo)
{
	if(P_Tab) {
		uint pos = 0;
		if(P_Tab->bsearch(&funcId, &pos, PTR_CMPFUNC(int))) {
			ASSIGN_PTR(pInfo, *static_cast<DBFuncInfo *>(P_Tab->at(pos)));
			return 1;
		}
	}
	return -1;
}

int SLAPI DbqFuncTab::PreRegister()
{
	RegisterFunc(dbq_ltoa,    0, BTS_STRING, _ltoa_i, 1, BTS_INT);
	RegisterFunc(dbq_enumtoa, 0, BTS_STRING, _enumtoa_iii, 3, BTS_INT, BTS_INT, BTS_INT);
	RegisterFunc(dbq_flagtoa, 0, BTS_STRING, _flagtoa_iii, 3, BTS_INT, BTS_INT, BTS_INT);
	RegisterFunc(dbq_add_ii, 0, BTS_INT, _add_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_sub_ii, 0, BTS_INT, _sub_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_mul_ii, 0, BTS_INT, _mul_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_div_ii, 0, BTS_INT, _div_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_rem_ii, 0, BTS_INT, _rem_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_add_rr, 0, BTS_REAL, _add_rr, 2, BTS_REAL, BTS_REAL);
	RegisterFunc(dbq_sub_rr, 0, BTS_REAL, _sub_rr, 2, BTS_REAL, BTS_REAL);
	RegisterFunc(dbq_mul_rr, 0, BTS_REAL, _mul_rr, 2, BTS_REAL, BTS_REAL);
	RegisterFunc(dbq_div_rr, 0, BTS_REAL, _div_rr, 2, BTS_REAL, BTS_REAL);
	RegisterFunc(dbq_and, 0, BTS_INT, _and_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_or,  0, BTS_INT, _or_ii , 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_xor, 0, BTS_INT, _xor_ii, 2, BTS_INT, BTS_INT);
	RegisterFunc(dbq_not, 0, BTS_INT, _not_ii, 1, BTS_INT);
	RegisterFunc(dbq_add_ss, 0, BTS_STRING, _add_ss,   2, BTS_STRING, BTS_STRING);
	RegisterFunc(dbq_left,   0, BTS_STRING, _left_si,  2, BTS_STRING, BTS_INT);
	RegisterFunc(dbq_right,  0, BTS_STRING, _right_si, 2, BTS_STRING, BTS_INT);
	RegisterFunc(dbq_ltrim,  0, BTS_STRING, _ltrim_s,  1, BTS_STRING);
	RegisterFunc(dbq_rtrim,  0, BTS_STRING, _rtrim_s,  1, BTS_STRING);
	RegisterFunc(dbq_strlen, 0, BTS_INT,    _strlen_s, 1, BTS_STRING);
	RegisterFunc(dbq_lower,  0, BTS_STRING, _lower_s,  1, BTS_STRING);
	RegisterFunc(dbq_upper,  0, BTS_STRING, _upper_s,  1, BTS_STRING);
	RegisterFunc(dbq_contains, 0, BTS_INT,  _contains_s, 2, BTS_STRING, BTS_STRING);
	RegisterFunc(dbq_day,     0, BTS_INT, _day_d,     1, BTS_DATE);
	RegisterFunc(dbq_mon,     0, BTS_INT, _mon_d,     1, BTS_DATE);
	RegisterFunc(dbq_year,    0, BTS_INT, _year_d,    1, BTS_DATE);
	RegisterFunc(dbq_weekday, 0, BTS_INT, _weekday_d, 1, BTS_DATE);
	RegisterFunc(dbq_add_di,  0, BTS_DATE, _add_di,   2, BTS_DATE, BTS_INT);
	RegisterFunc(dbq_sub_dd,  0, BTS_INT, _sub_dd,    2, BTS_DATE, BTS_DATE);
	//RegisterFunc(dbq_hour,    0, BTS_INT, _hour_t,   1, BTS_TIME);
	//RegisterFunc(dbq_minute,  0, BTS_INT, _minute_t, 1, BTS_TIME);
	//RegisterFunc(dbq_millisecond, 0, BTS_INT, _millisecond_t, 1, BTS_TIME);
	//RegisterFunc(dbq_sub_tt,  0, BTS_TIME, _sub_tt, 2, BTS_TIME, BTS_TIME);
	//RegisterFunc(dbq_count,   1, BTS_INT,  _count_v, 0);
	//RegisterFunc(dbq_sum_i,   1, BTS_INT,  _sum_i,   1, BTS_INT);
	//RegisterFunc(dbq_sum_r,   1, BTS_REAL, _sum_r,   1, BTS_REAL);
	//RegisterFunc(dbq_avg,     1, BTS_REAL, _avg_r,   1, BTS_REAL);
	//RegisterFunc(dbq_min,     1, BTS_VOID, _min_v,   1, BTS_VOID);
	//RegisterFunc(dbq_max,     1, BTS_VOID, _max_v,   1, BTS_VOID);
	return 1;
}
