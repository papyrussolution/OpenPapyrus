// DL600.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
//
//
DlFunc::DlFunc() : ArgNamList("/&"), ArgList(sizeof(DlFunc::Arg))
{
	TypID = 0;
	Flags = 0;
	ImplID = 0;
	ArgNamList.add("$", 0); // zero index - undefined name
}

int DlFunc::IsEqual(const DlFunc & rPat) const
{
	int    ok = 1;
	THROW(TypID == rPat.TypID);
	THROW(Flags == rPat.Flags);
	THROW(ImplID == rPat.ImplID);
	THROW(Name.Cmp(rPat.Name, 0) == 0);
	uint c = ArgList.getCount();
	THROW(c == rPat.ArgList.getCount());
	if(c) {
		do {
			--c;
			const Arg * p_arg = (const Arg *)ArgList.at(c);
			const Arg * p_pat_arg = (const Arg *)rPat.ArgList.at(c);
			THROW(p_arg->TypID == p_pat_arg->TypID);
			THROW(p_arg->Flags == p_pat_arg->Flags);
		} while(c);
	}
	CATCHZOK
	return ok;
}

uint DlFunc::GetArgCount() const
{
	return ArgList.getCount();
}

int DlFunc::GetArg(uint argN, Arg * pArg) const
{
	if(argN < ArgList.getCount()) {
		ASSIGN_PTR(pArg, *(const Arg *)ArgList.at(argN));
		return 1;
	}
	else
		return 0;
}

DLSYMBID DlFunc::GetArgType(uint argN) const
{
	return (argN < ArgList.getCount()) ? ((const Arg *)ArgList.at(argN))->TypID : 0;
}

int DlFunc::AddArg(uint typeId, const char * pName, uint argFlags)
{
	Arg arg;
	arg.TypID = typeId;
	arg.Flags = (uint16)argFlags;
	arg.NamePos = 0;
	if(pName && pName[0])
		ArgNamList.add(pName, &arg.NamePos);
	ArgList.insert(&arg);
	return 1;
}

int DlFunc::GetArgName(uint argN, SString & rArgName) const
{
	int    ok = 1;
	rArgName = 0;
	if(argN < ArgList.getCount()) {
		const Arg * p_arg = (const Arg *)ArgList.at(argN);
		ArgNamList.getnz(p_arg->NamePos, rArgName);
	}
	else
		ok = 0;
	return ok;
}

// static
int DlFunc::GetOpName(uint opID, SString & rName)
{
	static struct OpName { uint16 OpID; const char * P_Name; } OpNameList[] = {
		{dlopConstructor, "constructor" },
		{dlopDestructor,  "destructor"  },
		{dlopNew,         "@ new" },
		{dlopDelete,      "@ delete" },
		{dlopAssignment,  "@="  },
		{dlopEq,          "@==" },
		{dlopNeq,         "@!=" },
		{dlopConvert,     "@()" },
		{dlopAdd,         "@+"  },
		{dlopSub,         "@-"  },
		{dlopMul,         "@*"  },
		{dlopDiv,         "@/"  },
		{dlopMod,         "@%"  },
		{dlopInc,         "@++" },
		{dlopDec,         "@--" },
		{dlopLt,          "@<"  },
		{dlopLe,          "@<=" },
		{dlopGt,          "@>"  },
		{dlopGe,          "@>=" },
		{dlopAnd,         "@&&" },
		{dlopOr,          "@||" },
		{dlopNot,         "@!"  },
		{dlopBwAnd,       "@&"  },
		{dlopBwOr,        "@|"  },
		{dlopUnaryPlus,   "unary+" },
		{dlopUnaryMinus,  "unary-" },
		{dlopDot,         "@."  },
		{dlopQuest,       "@?"  }
	};
	uint   i = sizeof(OpNameList) / sizeof(OpName);
	do {
		--i;
		if((uint)OpNameList[i].OpID == opID) {
			const char * p_nam = OpNameList[i].P_Name;
			if(*p_nam == '@')
				(rName = "operator").Cat(p_nam+1);
			else
				rName = p_nam;
			return 1;
		}
	} while(i);
	rName = 0;
	return 0;
}

int DlFunc::GetName(uint options, SString & rName) const
{
	int    ok = 1;
	if(Name.C(0) == '?' && Name.C(1) == '?')
		ok = GetOpName(Name.C(2), rName);
	else
		rName = Name;
	return ok;
}
//
//
//
DlFuncPool::DlFuncPool() : Items(sizeof(F)), ArgList(sizeof(Arg))
{
	Arg    empty;
	ArgList.insert(MEMSZERO(empty)); // нулевой индекс - инвалидный
	NamePool.add("$", 0);
}

int DlFuncPool::Write(SBuffer & rBuf) const
{
	rBuf.Write(&Items, 0);
	rBuf.Write(&ArgList, 0);
	NamePool.Write(rBuf);
	return 1;
}

int DlFuncPool::Read(SBuffer & rBuf)
{
	rBuf.Read(&Items, 0);
	rBuf.Read(&ArgList, 0);
	NamePool.Read(rBuf);
	return 1;
}

int DlFuncPool::IsEqual(const DlFuncPool & rPat) const
{
	int    ok = 1;
	uint   c = Items.getCount();
	THROW(c == rPat.Items.getCount());
	{
		DlFunc f1, f2;
		if(c) do {
			--c;
			THROW(GetByPos(c, &f1));
			THROW(rPat.GetByPos(c, &f2));
			THROW(f1.IsEqual(f2));
		} while(c);
	}
	CATCHZOK
	return ok;
}

int DlFuncPool::SearchNamePos(uint namePos, uint * pPos) const
{
	uint   pos = 0;
	if(Items.lsearch(&namePos, &pos, PTR_CMPFUNC(uint))) {
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	else
		return 0;
}

int DlFuncPool::Add(const DlFunc * pF)
{
	F  f;
	MEMSZERO(f);
	NamePool.add(pF->Name, &f.NamePos);
	const  char * p_arg_name_list = pF->ArgNamList.getBuf();
	if(p_arg_name_list && *p_arg_name_list)
		NamePool.add(p_arg_name_list, &f.ArgNamPos);
	f.TypID = pF->TypID;
	f.Flags = pF->Flags;
	const  uint c = pF->GetArgCount();
	if(c) {
		f.ArgPos = ArgList.getCount();
		for(uint i = 0; i < c; i++) {
			DlFunc::Arg func_arg;
			pF->GetArg(i, &func_arg);
			DlFuncPool::Arg pool_arg;
			pool_arg.TypID = func_arg.TypID;
			pool_arg.NamePos = (uint16)func_arg.NamePos;
			pool_arg.Flags = func_arg.Flags;
			ArgList.insert(&pool_arg);
		}
	}
	f.ArgCount = c;
	if(f.Flags & DlFunc::fImplByID)
		f.ImplID = pF->ImplID;
	else
		f.Impl = pF->Impl;
	Items.insert(&f);
	return 1;
}

int DlFuncPool::GetByPos(uint pos, DlFunc * pF) const
{
	if(pos < Items.getCount()) {
		const F & f = *(F *)Items.at(pos);
		if(f.ArgNamPos) {
			NamePool.get(f.ArgNamPos, pF->Name);
			pF->ArgNamList.setBuf(pF->Name, pF->Name.Len()+1);
		}
		else
			pF->ArgNamList.clear();
		NamePool.get(f.NamePos, pF->Name);
		pF->TypID = f.TypID;
		pF->Flags = f.Flags;
		pF->ArgList.clear();
		for(uint i = 0; i < f.ArgCount; i++) {
			DlFunc::Arg arg;
			const DlFuncPool::Arg & pool_arg = *(const DlFuncPool::Arg *)ArgList.at(f.ArgPos+i);
			arg.NamePos = pool_arg.NamePos;
			arg.TypID = pool_arg.TypID;
			arg.Flags = pool_arg.Flags;
			pF->ArgList.insert(&arg);
		}
		if(f.Flags & DlFunc::fImplByID)
			pF->ImplID = f.ImplID;
		else
			pF->Impl = f.Impl;
		return 1;
	}
	else
		return 0;
}

int DlFuncPool::EnumByName(const char * pName, uint * pPos, DlFunc * pFunc) const
{
	uint   name_pos = 0, pos = 0;
	while(NamePool.search(pName, &name_pos, 0) && SearchNamePos(name_pos, &pos))
		if(!pPos || pos >= *pPos) {
			if(pFunc)
				GetByPos(pos, pFunc);
			ASSIGN_PTR(pPos, pos);
			return 1;
		}
		else
			name_pos++;
	return 0;
}
//
//
//
CtmExprConst & CtmExprConst::Init()
{
	TypeId = 0;
	Data = 0;
	return *this;
}

int CtmExprConst::operator !() const
{
	return (TypeId == 0);
}
//
//
//
CtmConstList::CtmConstList()
{
	Init();
	DataLen = 0;
}

CtmConstList::~CtmConstList()
{
	Destroy();
}

int CtmConstList::Test_Cmp(const CtmConstList & rPat) const
{
	if(DataLen != rPat.DataLen)
		return 0;
	if((P_Buf && !rPat.P_Buf) || (!P_Buf && rPat.P_Buf))
		return 0;
	if(DataLen && P_Buf) {
		if(memcmp(P_Buf, rPat.P_Buf, DataLen) != 0)
			return 0;
	}
	return 1;
}

int CtmConstList::Write(SBuffer * pBuf) const
{
	pBuf->Write(&DataLen, sizeof(DataLen));
	pBuf->Write(P_Buf, DataLen);
	return 1;
}

int CtmConstList::Read(SBuffer * pBuf)
{
	int    ok = 1;
	pBuf->Read(&DataLen, sizeof(DataLen));
	if(DataLen > Size) {
		uint8 * p = (uint8 *)SAlloc::R(P_Buf, DataLen);
		if(p) {
			P_Buf = (char *)p;
			Size = DataLen;
		}
		else
			ok = 0;
	}
	if(ok)
		pBuf->Read(P_Buf, DataLen);
	return ok;
}

int CtmConstList::SearchAnalog(const void * pBuf, size_t len, size_t * pPos) const
{
	size_t p = 0;
	while((p + len) <= DataLen)
		if(memcmp(P_Buf+p, pBuf, len) == 0) {
			ASSIGN_PTR(pPos, p);
			return 1;
		}
		else
			p++;
	return 0;
}

int CtmConstList::Add(CtmExprConst * pC, const void * pBuf, size_t len)
{
	int    ok = 1;
	if(len > sizeof(uint32)) {
		size_t pos = 0;
		if(SearchAnalog(pBuf, len, &pos)) {
			pC->Pos = pos;
		}
		else {
			size_t new_len = DataLen + len;
			if(new_len > Size) {
				uint8 * p = (uint8 *)SAlloc::R(P_Buf, new_len);
				if(p)
					Set(p, new_len);
				else
					ok = 0;
			}
			if(ok) {
				pC->Pos = (uint32)DataLen;
				memcpy(P_Buf+DataLen, pBuf, len);
				DataLen += len;
			}
		}
	}
	else
		memcpy(&pC->Data, pBuf, len);
	return ok;
}

const void * CtmConstList::GetPtr(const CtmExprConst * pC, size_t len) const
{
	const void * p_ret = 0;
	if(len > sizeof(uint32)) {
		if((pC->Pos + len) <= DataLen)
			p_ret = (P_Buf+pC->Pos);
	}
	else
		p_ret = &pC->Data;
	return p_ret;
}

int CtmConstList::Get(const CtmExprConst * pC, void * pBuf, size_t len) const
{
	int    ok = 1;
	if(len > sizeof(uint32))
		if((pC->Pos + len) <= DataLen)
			memcpy(pBuf, P_Buf+pC->Pos, len);
		else
			ok = 0;
	else
		memcpy(pBuf, &pC->Data, len);
	return ok;
}
//
//
//
void FASTCALL CtmExpr::Init(int kind)
{
	Kind = kind;
	Flags = 0;
	P_Next = 0;
	P_Arg = 0;
	TypID = 0;
	ToTypStdCvt = 0;
	MEMSZERO(U);
}

void FASTCALL CtmExpr::Init(const CtmExprConst & c)
{
	Init(kConst);
	U.C = c;
	TypID = c.TypeId;
}

void FASTCALL CtmExpr::Init(const CtmVar & v)
{
	Init(kVar);
	U.V = v;
}

int CtmExpr::InitUnaryOp(uint op, CtmExpr & a)
{
	Init(kOp);
	U.Op = op;
	AddArg(a);
	return 1;
}

int CtmExpr::InitBinaryOp(uint op, CtmExpr & a1, CtmExpr & a2)
{
	Init(kOp);
	U.Op = op;
	AddArg(a1);
	AddArg(a2);
	return 1;
}

// | '@' '(' T_TYPE ',' expr ')' { $$.InitRefOp($3.U.S, $5);

int CtmExpr::InitRefOp(DLSYMBID type, CtmExpr & a1)
{
	Init(kOp);
	U.Ref.Op = dlopObjRef;
	U.Ref.Typ = type;
	AddArg(a1);
	return 1;
}

int CtmExpr::InitTypeConversion(CtmExpr & a, DLSYMBID toType)
{
	Init(kOp);
	U.Cvt.Op = dlopConvert;
	U.Cvt.ToTyp = toType;
	AddArg(a);
	return 1;
}

int FASTCALL CtmExpr::InitVar(const char * pVarName)
{
	Init(kVarName);
	U.S = newStr(pVarName);
	return 1;
}

int FASTCALL CtmExpr::InitVar(const CtmVar & rVar)
{
	Init(kVar);
	U.V = rVar;
	return 1;
}

int CtmExpr::InitFuncCall(const char * pFuncName, CtmExpr & a)
{
	Init(kFuncName);
	U.S = newStr(pFuncName);
	if(a.Kind)
		AddArg(a);
	return 1;
}

void CtmExpr::Destroy()
{
	if(P_Next) {
		P_Next->Destroy();
		ZDELETE(P_Next);
	}
	if(P_Arg) {
		P_Arg->Destroy();
		ZDELETE(P_Arg);
	}
	if(oneof2(Kind, kFuncName, kVarName))
		ZDELETE(U.S);
}

void FASTCALL CtmExpr::SetType(DLSYMBID typeID)
{
	TypID = typeID;
	ToTypStdCvt = 0;
}

#define CTM_EXPR_PACK_SIGN  0x58454c44UL // "DLEX"
#define CTM_EXPR_SIGN       0xf0f0f0f0UL // Сигнатура элемента CtmExpr
#define CTM_EXPR_EMPTY_SIGN 0x0000ffffUL // Сигнатура пустого элемента CtmExpr

int CtmExpr::Pack(SBuffer * pBuf, size_t * pOffs) const
{
	int    ok = 1;
	size_t offs = pBuf->GetWrOffs();
	if(offs == 0) {
		const ulong signature = CTM_EXPR_PACK_SIGN;
		pBuf->Write(&signature, sizeof(signature));
	}
	offs = pBuf->GetWrOffs();
	{
		const ulong signature = CTM_EXPR_SIGN;
		pBuf->Write(&signature, sizeof(signature));
	}
	pBuf->Write(&Kind, sizeof(Kind));
	pBuf->Write(&Flags, sizeof(Flags));
	pBuf->Write(&TypID, sizeof(TypID));
	pBuf->Write(&ToTypStdCvt, sizeof(ToTypStdCvt));
	if(oneof2(Kind, kFuncName, kVarName)) {
		SString temp_buf = U.S;
		pBuf->Write(temp_buf);
	}
	else
		pBuf->Write(&U, sizeof(U));
	size_t next_offs = 0, arg_offs = 0;
	if(P_Next) {
		THROW(P_Next->Pack(pBuf, &next_offs)); // @recursion
	}
	else {
		const ulong signature = CTM_EXPR_EMPTY_SIGN;
		pBuf->Write(&signature, sizeof(signature));
	}
	if(P_Arg) {
		THROW(P_Arg->Pack(pBuf, &arg_offs));   // @recursion
	}
	else {
		const ulong signature = CTM_EXPR_EMPTY_SIGN;
		pBuf->Write(&signature, sizeof(signature));
	}
	pBuf->Write(&next_offs, sizeof(next_offs));
	pBuf->Write(&arg_offs, sizeof(arg_offs));
	CATCHZOK
	ASSIGN_PTR(pOffs, offs);
	return ok;
}

int CtmExpr::Unpack(SBuffer * pBuf)
{
	int    ok = 1;
	size_t offs = pBuf->GetRdOffs();
	ulong sign = 0;
	if(offs == 0) {
		pBuf->Read(&sign, sizeof(sign));
		THROW(sign == CTM_EXPR_PACK_SIGN);
	}
	{
		pBuf->Read(&sign, sizeof(sign));
		THROW(sign == CTM_EXPR_SIGN);
	}
	pBuf->Read(&Kind, sizeof(Kind));
	pBuf->Read(&Flags, sizeof(Flags));
	pBuf->Read(&TypID, sizeof(TypID));
	pBuf->Read(&ToTypStdCvt, sizeof(ToTypStdCvt));
	if(oneof2(Kind, kFuncName, kVarName)) {
		SString temp_buf;
		pBuf->Read(temp_buf);
		U.S = newStr(temp_buf);
	}
	else
		pBuf->Read(&U, sizeof(U));
	//
	// Reading P_Next
	//
	pBuf->Read(&sign, sizeof(sign));
	THROW(oneof2(sign, CTM_EXPR_SIGN, CTM_EXPR_EMPTY_SIGN));
	if(sign == CTM_EXPR_SIGN) {
		pBuf->Unread(sizeof(sign));
		P_Next = new CtmExpr;
		P_Next->Init();
		THROW(P_Next->Unpack(pBuf));
	}
	else if(sign == CTM_EXPR_EMPTY_SIGN)
		P_Next = 0;
	//
	// Reading P_Arg
	//
	pBuf->Read(&sign, sizeof(sign));
	THROW(oneof2(sign, CTM_EXPR_SIGN, CTM_EXPR_EMPTY_SIGN));
	if(sign == CTM_EXPR_SIGN) {
		pBuf->Unread(sizeof(sign));
		P_Arg = new CtmExpr;
		P_Arg->Init();
		P_Arg->Unpack(pBuf);
	}
	else if(sign == CTM_EXPR_EMPTY_SIGN)
		P_Arg = 0;
	CATCHZOK
	return ok;
}

const char * P_EmptyExprStr = "{E}";

int CtmExpr::Pack(SString & rBuf) const
{
	int    ok = 1;
	rBuf.CatChar('{');
	rBuf.Cat(Kind).Semicol().Cat(Flags).Semicol().Cat(TypID).Semicol().Cat(ToTypStdCvt).Semicol();
	{
		if(Kind == kEmpty)
			rBuf.Cat(0L);
		else if(Kind == kConst)
			rBuf.Cat(U.C.TypeId).Semicol().Cat(U.C.Pos);
		else if(Kind == kVar)
			rBuf.Cat(U.V.ScopeID).Semicol().Cat(U.V.Pos);
		else if(Kind == kFunc)
			rBuf.Cat(U.F.ScopeID).Semicol().Cat(U.F.Pos);
		else if(Kind == kOp)
			rBuf.Cat(U.Op);
		else if(oneof2(Kind, kFuncName, kVarName))
			rBuf.Cat(U.S);
		rBuf.Semicol();
	}
	if(P_Next) { THROW(P_Next->Pack(rBuf)); } else { rBuf.Cat(P_EmptyExprStr); }
	if(P_Arg)  { THROW(P_Arg->Pack(rBuf));  } else { rBuf.Cat(P_EmptyExprStr); }
	rBuf.CatChar('}');
	CATCHZOK
	return ok;
}

int CtmExpr::Unpack(SStrScan & scan)
{
	int    ok = 1;
	SString temp_buf;
	scan.Skip();
	THROW(*scan == '{');
	scan.Incr();
	THROW(scan.SearchChar(';')); Kind  = (uint16)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
	THROW(scan.SearchChar(';')); Flags = (uint16)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
	THROW(scan.SearchChar(';')); TypID = (DLSYMBID)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
	THROW(scan.SearchChar(';')); ToTypStdCvt = (DLSYMBID)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
	{
		if(Kind == kEmpty) {
			THROW(scan.SearchChar(';')); THROW(scan.Get(temp_buf).ToLong() == 0);
		}
		else if(Kind == kConst) {
			THROW(scan.SearchChar(';')); U.C.TypeId = (DLSYMBID)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
			THROW(scan.SearchChar(';')); U.C.Pos    = (uint)scan.Get(temp_buf).ToLong();
		}
		else if(Kind == kVar) {
			THROW(scan.SearchChar(';')); U.V.ScopeID = (DLSYMBID)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
			THROW(scan.SearchChar(';')); U.V.Pos     = (uint)scan.Get(temp_buf).ToLong();
		}
		else if(Kind == kFunc) {
			THROW(scan.SearchChar(';')); U.F.ScopeID = (DLSYMBID)scan.Get(temp_buf).ToLong(); scan.IncrLen(1);
			THROW(scan.SearchChar(';')); U.F.Pos     = (uint)scan.Get(temp_buf).ToLong();
		}
		else if(Kind == kOp) {
			THROW(scan.SearchChar(';')); U.Op = (uint)scan.Get(temp_buf).ToLong();
		}
		else if(oneof2(Kind, kFuncName, kVarName)) {
			THROW(scan.SearchChar(';')); U.S = newStr(scan.Get(temp_buf));
		}
		scan.IncrLen(1);
	}
	scan.Skip();
	const size_t empty_len = strlen(P_EmptyExprStr);
	if(scan.Search(P_EmptyExprStr) && scan.Len == 0) {
		scan.Offs += empty_len;
		P_Next = 0;
	}
	else {
		P_Next = new CtmExpr;
		P_Next->Init();
		THROW(P_Next->Unpack(scan));
	}
	if(scan.Search(P_EmptyExprStr) && scan.Len == 0) {
		scan.Offs += empty_len;
		P_Arg = 0;
	}
	else {
		P_Arg = new CtmExpr;
		P_Arg->Init();
		THROW(P_Arg->Unpack(scan));
	}
	THROW(*scan == '}');
	scan.Incr();
	CATCHZOK
	return ok;
}

int FASTCALL CtmExpr::IsResolved(int inDepth) const
{
	if(oneof3(Kind, kOp, kFuncName, kVarName))
		return 0;
	else if(inDepth)
		return ((!P_Next || P_Next->IsResolved(1)) && (!P_Arg || P_Arg->IsResolved(1)));
	else
		return 1;
}

int FASTCALL CtmExpr::Append(const CtmExpr & a)
{
	CtmExpr * p_last = this;
	while(p_last->P_Next)
		p_last = p_last->P_Next;
	p_last->P_Next = new CtmExpr(a);
	return p_last->P_Next ? 1 : 0;
}

int FASTCALL CtmExpr::AddArg(const CtmExpr & a)
{
	int    ok = 1;
	if(P_Arg) {
		ok = P_Arg->Append(a);
	}
	else {
		P_Arg = new CtmExpr(a);
		if(!P_Arg)
			ok = 0;
	}
	return ok;
}

uint CtmExpr::GetListCount() const
{
	return P_Next ? (1 + P_Next->GetListCount()) : 1;
}

uint CtmExpr::GetArgCount() const
{
	return P_Arg ? P_Arg->GetListCount() : 0;
}

CtmExpr * FASTCALL CtmExpr::GetArg(uint pos) const
{
	CtmExpr * p_expr = P_Arg;
	for(uint c = 0; p_expr && c < pos; c++)
		p_expr = p_expr->P_Next;
	return p_expr;
}

DLSYMBID CtmExpr::GetTypeID() const
{
	return NZOR(ToTypStdCvt, TypID);
}

int FASTCALL CtmExpr::SetImplicitCast(DLSYMBID toTypID)
{
	if(ToTypStdCvt)
		return (/*LastError = PPERR_DL6_IMPLICITCVTOVRD,*/ 0);
	ToTypStdCvt = toTypID;
	return 1;
}

int CtmExpr::SetResolvedVar(const CtmVar & rVar, DLSYMBID typeID)
{
	if(Kind == kVarName) {
		Kind = kVar;
		ZDELETE(U.S);
		U.V = rVar;
		SetType(typeID);
		return 1;
	}
	else
		return 0;
}

int FASTCALL CtmExpr::SetResolvedFunc(const CtmFunc & rFunc)
{
	if(Kind == kFuncName || Kind == kOp) {
		Kind = kFunc;
		if(Kind == kFuncName)
			ZDELETE(U.S);
		U.F = rFunc;
		return 1;
	}
	else
		return 0;
}
//
//
//
RtmStack::RtmStack()
{
	B.Init();
	P = 0;
}

RtmStack::~RtmStack()
{
	B.Destroy();
}

int FASTCALL RtmStack::Init(size_t sz)
{
	size_t s = ALIGNSIZE(sz, 10);
	B.P_Buf = (char *)SAlloc::R(B.P_Buf, s);
	if(B.P_Buf) {
		B.Size = s;
		return 1;
	}
	else {
		B.Size = 0;
		return 0;
	}
}

uint RtmStack::GetCurPos() const
{
	return P;
}

void FASTCALL RtmStack::SetCurPos(size_t p)
{
	P = p;
}

void * FASTCALL RtmStack::GetPtr(size_t p)
{
	return (B.P_Buf+p);
}

uint FASTCALL RtmStack::Alloc(size_t sz)
{
	size_t p = P;
	P += ALIGNSIZE(sz, 2);
	return p;
}
//
//
//
#ifdef DL600C // {

static DLSYMBID AdjRetType_ZSTRING(DlContext * pCtx, size_t s)
{
	STypEx t;
	t.Init();
	t.Typ = MKSTYPE(S_ZSTRING, 0);
	return pCtx->SetDeclTypeMod(pCtx->SearchSTypEx(t, 0), STypEx::modArray, s);
}

static DLSYMBID AdjRetType_PlusSS(DlContext * pCtx, const CtmExpr * pExpr)
{
	const  CtmExpr * arg0 = pExpr->GetArg(0);
	const  CtmExpr * arg1 = pExpr->GetArg(1);
	DlContext::TypeEntry te0, te1;
	pCtx->SearchTypeID(arg0->GetTypeID(), 0, &te0);
	pCtx->SearchTypeID(arg1->GetTypeID(), 0, &te1);
	size_t s0 = te0.T.GetBinSize();
	size_t s1 = te1.T.GetBinSize();
	// size_t s = (((s0 + s1 - 1) + 3) >> 2) << 2;
	size_t s = s0 + s1 - 1; // Для совместимости с ALDD не выравниваем размер
	return AdjRetType_ZSTRING(pCtx, s);
}

static DLSYMBID AdjRetType_MulSS(DlContext * pCtx, const CtmExpr * pExpr)
{
	const CtmExpr * arg0 = pExpr->GetArg(0);
	const CtmExpr * arg1 = pExpr->GetArg(1);
	DlContext::TypeEntry te0, te1;
	pCtx->SearchTypeID(arg0->GetTypeID(), 0, &te0);
	pCtx->SearchTypeID(arg1->GetTypeID(), 0, &te1);
	size_t s0 = te0.T.GetBinSize();
	size_t s1 = te1.T.GetBinSize();
	// size_t s  = (((s0 + s1) + 3) >> 2) << 2;
	size_t s = s0 + s1; // Для совместимости с ALDD не выравниваем размер
	return AdjRetType_ZSTRING(pCtx, s);
}

static DLSYMBID AdjRetType_format(DlContext * pCtx, const CtmExpr * pExpr)
{
	const CtmExpr * arg0 = pExpr->GetArg(0);
	const CtmExpr * arg1 = pExpr->GetArg(1);
	DlContext::TypeEntry te0;
	pCtx->SearchTypeID(arg0->GetTypeID(), 0, &te0);
	size_t s = 0;
	if(te0.T.IsPure()) {
		TYPEID tid = GETSTYPE(te0.T.Typ);
		if(arg1->GetKind() == CtmExpr::kConst) {
			char   data_buf[256];
			if(pCtx->GetConstData(arg1->U.C, data_buf, sizeof(data_buf))) {
				long fmt = pCtx->ParseFormat(data_buf, tid);
				s = SFMTLEN(fmt);
				if(s)
					s++;
			}
		}
		if(!s) {
			if(oneof5(tid, S_INT, S_FLOAT, S_DEC, S_DATE, S_TIME))
				s = 16;
			else if(tid == S_ZSTRING)
				s = GETSSIZE(te0.T.Typ);
		}
	}
	SETIFZ(s, 16);
	return AdjRetType_ZSTRING(pCtx, s);
}

static DLSYMBID AdjRetType_excise(DlContext * pCtx, const CtmExpr * pExpr)
{
	//
	// left, middle, wrap - длина результата равна длине первого аргумента
	//
	const CtmExpr * arg0 = pExpr->GetArg(0);
	DlContext::TypeEntry te0;
	pCtx->SearchTypeID(arg0->GetTypeID(), 0, &te0);
	return AdjRetType_ZSTRING(pCtx, GETSSIZE(te0.T.Typ));
}

static DLSYMBID AdjRetType_formatperiod(DlContext * pCtx, const CtmExpr * pExpr)
	{ return AdjRetType_ZSTRING(pCtx, 30); }
static DLSYMBID AdjRetType_printablebarcode(DlContext * pCtx, const CtmExpr * pExpr)
	{ return AdjRetType_ZSTRING(pCtx, 20); }
static DLSYMBID AdjRetType_barcodeaddcd(DlContext * pCtx, const CtmExpr * pExpr)
	{ return AdjRetType_ZSTRING(pCtx, 16); }
static DLSYMBID AdjRetType_qttytostr(DlContext * pCtx, const CtmExpr * pExpr)
	{ return AdjRetType_ZSTRING(pCtx, 32); }

static DLSYMBID AdjRetType_tostr(DlContext * pCtx, const CtmExpr * pExpr)
{
	//
	// money2str, num2str, date2str, date2wstr
	// Второй аргумент должен быть целочисленной константой. Иначе - ошибка
	//
	DLSYMBID type_id = 0;
	char   data_buf[256];
	const  CtmExpr * arg1 = pExpr->GetArg(1);
	if(arg1->GetKind() == CtmExpr::kConst) {
		if(pCtx->GetConstData(arg1->U.C, data_buf, sizeof(data_buf))) {
			DlContext::TypeEntry te1;
			pCtx->SearchTypeID(arg1->GetTypeID(), 0, &te1);
			if(te1.T.IsPure() && stisnumber(te1.T.Typ)) {
				int    bt = stbase(te1.T.Typ);
				size_t len = 32;
				if(bt == BTS_INT) {
					long base_data_long;
					sttobase(te1.T.Typ, data_buf, &base_data_long);
					len = (size_t)base_data_long;
				}
				else if(bt == BTS_REAL) {
					double base_data_real;
					sttobase(te1.T.Typ, data_buf, &base_data_real);
					len = (size_t)base_data_real;
				}
				type_id = AdjRetType_ZSTRING(pCtx, len +1); // +1 для совместимости с ALDD
			}
			else
				pCtx->SetError(PPERR_DL6_ARGMUSTBECONST, 0);
		}
	}
	else
		pCtx->SetError(PPERR_DL6_ARGMUSTBECONST, 0);
	return type_id;
}

#endif // } DL600C

int FASTCALL DlContext::BuiltinOp(const DlFunc * pF, SV_Uint32 * pApl)
{
	#ifdef DL600C
		uint   impl_id = 0;
		#define CTONLY(f) f
	#else
		char   temp_buf[128];
		#define CTONLY(f)
	#endif
	#define _ARG_INT(n)  (*(int *)S.GetPtr(pApl->Get(n)))
	#define _ARG_UINT(n) (*(uint *)S.GetPtr(pApl->Get(n)))
	#define _ARG_DBL(n)  (*(double *)S.GetPtr(pApl->Get(n)))
	#define _ARG_DT(n)   (*(LDATE *)S.GetPtr(pApl->Get(n)))
	#define _ARG_TM(n)   (*(LTIME *)S.GetPtr(pApl->Get(n)))
	#define _ARG_STR(n)  (**(SString **)S.GetPtr(pApl->Get(n)))
	#define _ARG1        (S.GetPtr(pApl->Get(1)))
	#define _ARG2        (S.GetPtr(pApl->Get(2)))
	#define _RET         (S.GetPtr(pApl->Get(0)))
	#define _RET_STR     (**(SString **)S.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)S.GetPtr(pApl->Get(0)))
	#define _RET_UINT    (*(uint *)S.GetPtr(pApl->Get(0)))
	#define _RET_DBL     (*(double *)S.GetPtr(pApl->Get(0)))
	#define _RET_DT      (*(LDATE *)S.GetPtr(pApl->Get(0)))
	#define _RET_TM      (*(LTIME *)S.GetPtr(pApl->Get(0)))

	int    ok = 1;
	if(!pF)
		goto _skip_switch;
	switch(pF->ImplID) {
_skip_switch:
			// Стандартное преобразование типов
		case 1: if(!pF) CTONLY(AddBOp(dlopConvert, 1, "void", 0));
			else {
				// @todo
				break;
			}
		case 2: if(!pF) CTONLY(AddBOp(dlopUnaryPlus, 2, "int", "int", 0));
			else { _RET_INT = _ARG_INT(1); break; }
		case 3: if(!pF) CTONLY(AddBOp(dlopUnaryPlus, 3, "double", "double", 0));
			else { _RET_DBL = _ARG_DBL(1); break; }
		case 4: if(!pF) CTONLY(AddBOp(dlopUnaryMinus, 4, "int", "int", 0));
			else { _RET_INT = -_ARG_INT(1); break; }
		case 5: if(!pF) CTONLY(AddBOp(dlopUnaryMinus, 5, "double", "double", 0));
			else { _RET_DBL = -_ARG_DBL(1); break; }
		case 6: if(!pF) CTONLY(AddBOp(dlopAdd, 6, "int", "int", "int", 0));
			else { _RET_INT = _ARG_INT(1)+_ARG_INT(2); break; }
		case 7: if(!pF) CTONLY(AddBOp(dlopAdd, 7, "double", "double", "double", 0));
			else { _RET_DBL = _ARG_DBL(1)+_ARG_DBL(2); break; }
		case 8:
			if(!pF) {
				#ifdef DL600C
				AddBOp(dlopAdd, impl_id = 8, "string", "string", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_PlusSS, 0, 0);
				#endif
			}
			else {
				(_RET_STR = _ARG_STR(1)).Cat(_ARG_STR(2));
				break;
			}
		case 9: if(!pF) CTONLY(AddBOp(dlopAdd, 9, "date", "date", "int", 0));
			else { *(LDATE *)_RET = plusdate(_ARG_DT(1), _ARG_INT(2)); break; }
		case 10: if(!pF) CTONLY(AddBOp(dlopAdd, 10, "date", "int", "date", 0));
			else { *(LDATE *)_RET = plusdate(_ARG_DT(2), _ARG_INT(1)); break; }
		case 11: if(!pF) CTONLY(AddBOp(dlopSub, 11, "int", "int", "int", 0));
			else { _RET_INT = _ARG_INT(1)-_ARG_INT(2); break; }
		case 12: if(!pF) CTONLY(AddBOp(dlopSub, 12, "double", "double", "double", 0));
			else { _RET_DBL = _ARG_DBL(1)-_ARG_DBL(2); break; }
			// дата - дни = дата
		case 13: if(!pF) CTONLY(AddBOp(dlopSub, 13, "date", "date", "int", 0));
			else { *(LDATE *)_RET = plusdate(_ARG_DT(1), -_ARG_INT(2)); break; }
			// дата - дата = дни
		case 14: if(!pF) CTONLY(AddBOp(dlopSub, 14, "int", "date", "date", 0));
			else { _RET_INT = diffdate(_ARG_DT(1), _ARG_DT(2)); break; }
		case 15: if(!pF) CTONLY(AddBOp(dlopMul, 15, "int", "int", "int", 0));
			else { _RET_INT = _ARG_INT(1) * _ARG_INT(2); break; }
		case 16: if(!pF) CTONLY(AddBOp(dlopMul, 16, "double", "double", "double", 0));
			else { _RET_DBL = _ARG_DBL(1) * _ARG_DBL(2); break; }
		case 17: // string * string = 'string' ' ' 'string'
			if(!pF) {
				#ifdef DL600C
				AddBOp(dlopMul, impl_id = 17, "string", "string", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_MulSS, 0, 0);
				#endif
			}
			else {
				(_RET_STR = _ARG_STR(1)).Space().Cat(_ARG_STR(2));
				break;
			}
		case 18: if(!pF) CTONLY(AddBOp(dlopDiv, 18, "int", "int", "int", 0));
			else {
				int    a2 = _ARG_INT(2);
				_RET_INT = a2 ? (_ARG_INT(1) / a2) : 0; // @todo zero_div_exception
				break;
			}
		case 19: if(!pF) CTONLY(AddBOp(dlopDiv, 19, "double", "double", "double", 0));
			else {
				double a2 = _ARG_DBL(2);
				_RET_DBL = a2 ? (_ARG_DBL(1) / a2) : 0; // @todo zero_div_exception
				break;
			}
			// int % int = int
		case 20: if(!pF) CTONLY(AddBOp(dlopMod, 20, "int", "int", "int", 0));
			else {
				int a2 = _ARG_INT(2);
				_RET_INT = a2 ? (_ARG_INT(1) % a2) : 0; // @todo zero_div_exception
				break;
			}
			// uint & uint = uint
		case 21: if(!pF) CTONLY(AddBOp(dlopBwAnd, 21, "uint", "uint", "uint", 0));
			else { _RET_UINT = _ARG_UINT(1) & _ARG_UINT(2); break; }
			// uint | uint = uint
		case 22: if(!pF) CTONLY(AddBOp(dlopBwOr, 22, "uint", "uint", "uint", 0));
			else { _RET_UINT = _ARG_UINT(1) | _ARG_UINT(2); break; }
			// !bool = bool
		case 25: if(!pF) CTONLY(AddBOp(dlopNot, 25, "bool", "bool", 0));
			else { _RET_INT = !_ARG_INT(1); break; }
			// (double)int
		case 26: if(!pF) CTONLY(AddBOp(dlopConvert, 26, "double", "int", 0));
			else { _RET_DBL = (double)_ARG_INT(1); break; }
			// (int)double
		case 27: if(!pF) CTONLY(AddBOp(dlopConvert, 27, "int", "double", 0));
			else { _RET_INT = (int)_ARG_DBL(1); break; }
		// cmp ops {
		/*
			dlopEq, dlopNeq, dlopLt, dlopLe, dlopGt, dlopGe
		*/
		case 30: if(!pF) CTONLY(AddBCmpOps(30, "int"));
			else { _RET_INT = _ARG_INT(1) == _ARG_INT(2); break; }
			case 31: if(pF) { _RET_INT = _ARG_INT(1) != _ARG_INT(2); break; }
			case 32: if(pF) { _RET_INT = _ARG_INT(1) <  _ARG_INT(2); break; }
			case 33: if(pF) { _RET_INT = _ARG_INT(1) <= _ARG_INT(2); break; }
			case 34: if(pF) { _RET_INT = _ARG_INT(1) >  _ARG_INT(2); break; }
			case 35: if(pF) { _RET_INT = _ARG_INT(1) >= _ARG_INT(2); break; }
		case 40: if(!pF) CTONLY(AddBCmpOps(40, "double"));
			else { _RET_INT = _ARG_DBL(1) == _ARG_DBL(2); break; }
			case 41: if(pF) { _RET_INT = _ARG_DBL(1) != _ARG_DBL(2); break; }
			case 42: if(pF) { _RET_INT = _ARG_DBL(1) <  _ARG_DBL(2); break; }
			case 43: if(pF) { _RET_INT = _ARG_DBL(1) <= _ARG_DBL(2); break; }
			case 44: if(pF) { _RET_INT = _ARG_DBL(1) >  _ARG_DBL(2); break; }
			case 45: if(pF) { _RET_INT = _ARG_DBL(1) >= _ARG_DBL(2); break; }
		case 50: if(!pF) CTONLY(AddBCmpOps(50, "date"));
			else { _RET_INT = _ARG_DT(1) == _ARG_DT(2); break; }
			case 51: if(pF) { _RET_INT = _ARG_DT(1) != _ARG_DT(2); break; }
			case 52: if(pF) { _RET_INT = _ARG_DT(1) <  _ARG_DT(2); break; }
			case 53: if(pF) { _RET_INT = _ARG_DT(1) <= _ARG_DT(2); break; }
			case 54: if(pF) { _RET_INT = _ARG_DT(1) >  _ARG_DT(2); break; }
			case 55: if(pF) { _RET_INT = _ARG_DT(1) >= _ARG_DT(2); break; }
		case 60: if(!pF) CTONLY(AddBCmpOps(60, "time"));
			else { _RET_INT = _ARG_TM(1) == _ARG_TM(2); break; }
			case 61: if(pF) { _RET_INT = _ARG_TM(1) != _ARG_TM(2); break; }
			case 62: if(pF) { _RET_INT = _ARG_TM(1) <  _ARG_TM(2); break; }
			case 63: if(pF) { _RET_INT = _ARG_TM(1) <= _ARG_TM(2); break; }
			case 64: if(pF) { _RET_INT = _ARG_TM(1) >  _ARG_TM(2); break; }
			case 65: if(pF) { _RET_INT = _ARG_TM(1) >= _ARG_TM(2); break; }
		case 70: if(!pF) CTONLY(AddBCmpOps(70, "string"));
			else { _RET_INT = _ARG_STR(1).Cmp(_ARG_STR(2), 0) == 0; break; }
			case 71: if(pF) { _RET_INT = _ARG_STR(1).Cmp(_ARG_STR(2), 0) != 0; break; }
			case 72: if(pF) { _RET_INT = _ARG_STR(1).Cmp(_ARG_STR(2), 0) <  0; break; }
			case 73: if(pF) { _RET_INT = _ARG_STR(1).Cmp(_ARG_STR(2), 0) <= 0; break; }
			case 74: if(pF) { _RET_INT = _ARG_STR(1).Cmp(_ARG_STR(2), 0) >  0; break; }
			case 75: if(pF) { _RET_INT = _ARG_STR(1).Cmp(_ARG_STR(2), 0) >= 0; break; }
		// } cmp ops
		case 82: if(!pF) CTONLY(AddBFunc("length", 82, "uint", "string", 0));
			else { _RET_UINT = _ARG_STR(1).Len(); break; }
		case 83: if(!pF) CTONLY(AddBFunc("sqrt", 83, "double", "double", 0));
			else { _RET_DBL = sqrt(_ARG_DBL(1)); break; }
		case 84: if(!pF) CTONLY(AddBFunc("round", 84, "double", "double", "int", 0));
			else { _RET_DBL = round(_ARG_DBL(1), _ARG_INT(2)); break; }
		case 85: if(!pF) CTONLY(AddBFunc("abs", 85, "double", "double", 0));
			else { _RET_DBL = fabs(_ARG_DBL(1)); break; }
		case 86:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("left", impl_id = 86, "string", "string", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_excise, 0, 0);
				#endif
			}
			else { _RET_STR = _ARG_STR(1).Sub(0, _ARG_UINT(2), _RET_STR); break; }
		case 87:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("middle", impl_id = 87, "string", "string", "uint", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_excise, 0, 0);
				#endif
			}
			else { _RET_STR = _ARG_STR(1).Sub(_ARG_UINT(2), _ARG_UINT(3), _RET_STR); break; }
		case 88:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("wrap", impl_id = 88, "string", "string", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_excise, 0, 0);
				#endif
			}
			else { (_RET_STR = _ARG_STR(1)).TrimToDiv(_ARG_UINT(2), " "); break; }
		case 89:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("format", impl_id = 89, "string", "int", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_format, 0, 0);
				#endif
			}
			else {
				long fmt = ParseFormat(_ARG_STR(2), MKSTYPE(S_INT, 4));
				_RET_STR.Z().Cat(_ARG_INT(1), fmt);
				break;
			}
		case 90:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("format", impl_id = 90, "string", "double", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_format, 0, 0);
				#endif
			}
			else {
				long fmt = ParseFormat(_ARG_STR(2), MKSTYPE(S_FLOAT, 8));
				_RET_STR.Z().Cat(_ARG_DBL(1), fmt);
				break;
			}
		case 91:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("format", impl_id = 91, "string", "string", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_format, 0, 0);
				#endif
			}
			else {
				long fmt = ParseFormat(_ARG_STR(2), MKSTYPE(S_ZSTRING, 0));
				_RET_STR.Z().Cat(_ARG_STR(1));
				break;
			}
		case 92:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("format", impl_id = 92, "string", "date", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_format, 0, 0);
				#endif
			}
			else {
				long fmt = ParseFormat(_ARG_STR(2), MKSTYPE(S_DATE, 4));
				_RET_STR.Z().Cat(*(LDATE *)_ARG1, fmt);
				break;
			}
		case 93:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("format", impl_id = 93, "string", "time", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_format, 0, 0);
				#endif
			}
			else {
				long fmt = ParseFormat(_ARG_STR(2), MKSTYPE(S_TIME, 4));
				_RET_STR.Z().Cat(*(LTIME *)_ARG1, fmt);
				break;
			}
		case 94:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("formatperiod", impl_id = 94, "string", "date", "date", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_formatperiod, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				DateRange period;
				period.Set(_ARG_DT(1), _ARG_DT(2));
				PPFormatPeriod(&period, _RET_STR);
				#endif
				break;
			}
		case 95: // Перевод числа в текстовую форму в рублях (копейки числом)
			if(!pF) {
				#ifdef DL600C
				AddBFunc("money2str", impl_id = 95, "string", "double", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_tostr, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				MoneyToStr(_ARG_DBL(1), NTTF_CURRENCY | NTTF_FIRSTCAP, _RET_STR);
				#endif
				break;
			}
		case 96: // Перевод числа в текстовую форму в рублях (копейки прописью)
			if(!pF) {
				#ifdef DL600C
				AddBFunc("money2str", impl_id = 96, "string", "double", "uint", "int", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_tostr, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				const double arg = _ARG_DBL(1);
				numbertotext(arg, NTTF_CURRENCY | NTTF_FIRSTCAP, temp_buf);
				_RET_STR = temp_buf;
				uint   kop = (uint)R0i((arg-floor(arg))*100);
				temp_buf[0] = ' ';
				sprintf(temp_buf+1, "%02u Є®ЇҐ", kop);
				_RET_STR.Cat(temp_buf);
				if(kop > 10 && kop < 20)
					_RET_STR.Cat("ҐЄ");
				else {
					kop %= 10;
					if(kop == 1)
						_RET_STR.Cat("©Є ");
					else if(oneof3(kop, 2, 3, 4))
						_RET_STR.Cat("©ЄЁ");
					else
						_RET_STR.Cat("ҐЄ");
				}
				#endif
				break;
			}
		case 97:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("num2str", impl_id = 97, "string", "double", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_tostr, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				numbertotext(_ARG_DBL(1), NTTF_FIRSTCAP, temp_buf);
				_RET_STR = temp_buf;
				#endif
				break;
			}
		case 98:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("date2str", impl_id = 98, "string", "date", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_tostr, 0, 0);
				#endif
			}
			else {
				_RET_STR.Z().Cat(_ARG_DT(1), MKSFMT(_ARG_UINT(2), DATF_DMY | ALIGN_LEFT));
				break;
			}
		case 99:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("date2wstr", impl_id = 99, "string", "date", "uint", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_tostr, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				DateToStr(_ARG_DT(1), _RET_STR);
				#endif
				break;
			}
		case 100:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("printablebarcode", impl_id = 100, "string", "string", "int", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_printablebarcode, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				CreatePrintableBarcode(_ARG_STR(1), _ARG_INT(2), temp_buf, sizeof(temp_buf));
				_RET_STR = temp_buf;
				#endif
				break;
			}
		case 101:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("qttytostr", impl_id = 101, "string", "double", "double", "long", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_qttytostr, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				QttyToStr(_ARG_DBL(1), _ARG_DBL(2), _ARG_INT(3), temp_buf);
				_RET_STR = temp_buf;
				#endif
				break;
			}
		case 102:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("barcodeaddcd", impl_id = 102, "string", "string", 0);
				AdjRetTypeProcList.Add(impl_id, (long)AdjRetType_barcodeaddcd, 0, 0);
				#endif
			}
			else {
				#ifndef DL600C
				SString temp_str = _ARG_STR(1);
				size_t len = temp_str.Len();
				PPObjGoods goods_obj;
				const PPGoodsConfig & gcfg = goods_obj.GetConfig();
				if(len > 3 && !(gcfg.Flags & GCF_BCCHKDIG) && !gcfg.IsWghtPrefix(temp_str))
					AddBarcodeCheckDigit(temp_str);
				_RET_STR = temp_str;
				#endif
				break;
			}
		case 103:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("tonumber", impl_id = 103, "double", "string", 0);
				#endif
			}
			else {
				#ifndef DL600C
				double result = 0.0;
				strtodoub(_ARG_STR(1), &result);
				_RET_DBL = result;
				#endif
				break;
			}
		// @v8.3.8 {
		case 104: // datedmy(d, m, y)
			if(!pF) {
				#ifdef DL600C
				AddBFunc("datedmy", impl_id = 104, "date", "int", "int", "int", 0);
				#endif
			}
			else {
				#ifndef DL600C
				_RET_DT = encodedate(_ARG_INT(1), _ARG_INT(2), _ARG_INT(3));
				#endif
				break;
			}
		case 105: // datezero()
			if(!pF) {
				#ifdef DL600C
				AddBFunc("datezero", impl_id = 105, "date", 0);
				#endif
			}
			else {
				#ifndef DL600C
				_RET_DT = ZERODATE;
				#endif
				break;
			}
		case 106: // timehms(h, m, s)
			if(!pF) {
				#ifdef DL600C
				AddBFunc("timehms", impl_id = 106, "time", "int", "int", "int", 0);
				#endif
			}
			else {
				#ifndef DL600C
				_RET_TM = encodetime(_ARG_INT(1), _ARG_INT(2), _ARG_INT(3), 0);
				#endif
				break;
			}
		case 107: // timezero()
			if(!pF) {
				#ifdef DL600C
				AddBFunc("timezero", impl_id = 107, "time", 0);
				#endif
			}
			else {
				#ifndef DL600C
				_RET_TM = ZEROTIME;
				#endif
				break;
			}
		// } @v8.3.8
		//
		// Диапазон 200..499 зарезервирован за специальными функциями, требующими внешней
		// обработки  {
		//
			// bool && bool = bool
		case DL6FI_AND: if(!pF) CTONLY(AddBOp(dlopAnd, DL6FI_AND, "bool", "bool", "bool", 0));
			else { _RET_INT = _ARG_INT(1) && _ARG_INT(2); break; }
			// bool || bool = bool
		case DL6FI_OR: if(!pF) CTONLY(AddBOp(dlopOr, DL6FI_OR, "bool", "bool", "bool", 0));
			else { _RET_INT = _ARG_INT(1) || _ARG_INT(2); break; }
			// expr ? expr : expr
		case DL6FI_QUEST: if(!pF) CTONLY(AddBOp(dlopQuest, DL6FI_QUEST, "void", "bool", "void", "void", 0));
			else {
				break;
			}
			// if(expr) expr
		case DL6FI_IF: if(!pF) CTONLY(AddBOp(dlopQuest, DL6FI_IF, "void", "bool", "void", 0));
			else {
				break;
			}
		case DL6FI_DOT: if(!pF) CTONLY(AddBOp(dlopDot, DL6FI_DOT, "void", "void", "void", 0));
			else {
				break;
			}
		case DL6FI_REF: if(!pF) CTONLY(AddBOp(dlopObjRef, DL6FI_REF, "void", "void", 0));
			else {
				break;
			}
#if 0 // {
		case DL6FI_GETGLOBAL:
			if(!pF) {
				#ifdef DL600C
				AddBFunc("getglobal", DL6FI_GETGLOBAL, "long", "string", 0);
				#endif
			}
			else {
				#ifndef DL600C
				double result = 0.0;
				strtodoub(_ARG_STR(1), &result);
				_RET_DBL = result;
				#endif
				break;
			}
#endif // } 0
		// }
		default:
			if(pF)
				ok = 0;
	}
	return ok;
	#undef CTONLY
}

SLAPI DlContext::DlContext(int toCompile) :
	Tab(8192, 1), ScopeStack(sizeof(DLSYMBID)), Sc(0, DlScope::kGlobal, "global", 0)
{
	LastSymbId  = 0;
	UniqCntr = 0;
	CurScopeID = 0;
	Flags = 0;
	P_M = 0;
	MEMSZERO(F_Dot);
	MEMSZERO(F_Ref); // @v7.1.10
	getExecPath(LogFileName).SetLastSlash().Cat("dl600.log");
	if(toCompile) {
		Flags |= fCompile;
#ifdef DL600C // {
		LastError = 0;
		AddType("void",     MKSTYPE(S_VOID, 0),    MANGLE_BT_VOID);
		AddType("bool",     MKSTYPE(S_LOGICAL, 4), 0);
		AddType("int",      MKSTYPE(S_INT, 4),     MANGLE_BT_INT);
		AddType("long",     MKSTYPE(S_INT, 4),     MANGLE_BT_LONG);
		AddType("uint",     MKSTYPE(S_UBINARY, 4), MANGLE_BT_UINT);
		AddType("char",     MKSTYPE(S_INT, 1),     MANGLE_BT_CHAR);
		AddType("int8",     MKSTYPE(S_INT, 1),     MANGLE_BT_SCHAR);
		AddType("int16",    MKSTYPE(S_INT, 2),     MANGLE_BT_SHRT);
		AddType("short",    MKSTYPE(S_INT, 2),     MANGLE_BT_SHRT);
		AddType("int32",    MKSTYPE(S_INT, 4),     MANGLE_BT_LONG);
		AddType("autolong", MKSTYPE(S_AUTOINC, 4), MANGLE_BT_LONG);
		AddType("int64",    MKSTYPE(S_INT, 8),     0);
		AddType("uint8",    MKSTYPE(S_UBINARY, 1), MANGLE_BT_UCHAR);
		AddType("uint16",   MKSTYPE(S_UBINARY, 2), MANGLE_BT_USHRT);
		AddType("uint32",   MKSTYPE(S_UBINARY, 4), MANGLE_BT_ULONG);
		AddType("float",    MKSTYPE(S_FLOAT, 4),   MANGLE_BT_FLOAT);
		AddType("double",   MKSTYPE(S_FLOAT, 8),   MANGLE_BT_DBL);
		AddType("money",    MKSTYPED(S_MONEY, 0, 0), 0);
		AddType("decimal",  MKSTYPED(S_DEC, 0, 0), 0);
		AddType("string",   MKSTYPE(S_ZSTRING, 0), MANGLE_BT_ZSTRING);
		AddType("lstring",  MKSTYPE(S_LSTRING, 0), 0);
		AddType("date",     MKSTYPE(S_DATE, 4));
		AddType("time",     MKSTYPE(S_TIME, 4));
		AddType("datetime", MKSTYPE(S_DATETIME, 8));
		AddType("key",      MKSTYPE(S_INT, 4), MANGLE_BT_LONG);
		AddType("variant",  T_VARIANT);
		AddType("wchar",    MKSTYPE(S_WCHAR, 2));
		AddType("wstring",  MKSTYPE(S_WZSTRING, 0), MANGLE_BT_WZSTRING); // @v8.6.2
		AddType("note",     MKSTYPE(S_NOTE, 0));
		AddType("raw",      MKSTYPE(S_RAW, 0));
		AddType("blob",     MKSTYPE(S_BLOB, 0));
		AddType("clob",     MKSTYPE(S_CLOB, 0));
		AddType("ipoint2",  MKSTYPE(S_IPOINT2, sizeof(TPoint))); // @v6.9.4
		AddType("fpoint2",  MKSTYPE(S_FPOINT2, sizeof(FPoint))); // @v6.9.4
		AddType("guid",     MKSTYPE(S_UUID_, sizeof(S_GUID))); // @v8.0.10
		BuiltinOp(0, 0);
		AddMacro("Date2WStr", "date2wstr");
		AddMacro("Date2Str",  "date2str");
		AddMacro("Money2Str", "money2str");
		AddMacro("Num2Str",   "num2str");
		AddMacro("Round",     "round");
		AddMacro("PrintableBarcode", "printablebarcode");
#endif // } DL600C
	}
}

DLSYMBID SLAPI DlContext::GetNewSymbID()
{
	SETIFZ(LastSymbId, Tab.GetMaxVal()); // @v6.1.11
	return ++LastSymbId;
}

long SLAPI DlContext::GetUniqCntr()
{
	return ++UniqCntr;
}

DLSYMBID SLAPI DlContext::Helper_CreateSymb(const char * pSymb, DLSYMBID newId, int prefix, long flags)
{
	DLSYMBID id = 0;
	SString name, temp_buf;
	name.CatChar(prefix).Cat(pSymb);
	if(flags & crsymfCatCurScope) {
		SString qualif;
		Sc.GetQualif(CurScopeID, "@", 1, qualif);
		if(qualif.NotEmpty())
			name.CatChar('@').Cat(qualif);
	}
	if(Tab.Search(name, &id, 0)) {
		if(flags & crsymfErrorOnDup) {
			SetError(PPERR_DL6_DUPSYMB, name);
			id = 0;
		}
	}
	else {
		if(newId) {
			if(Tab.GetByAssoc(newId, temp_buf)) {
				SString msg_buf;
				msg_buf.Cat(newId).Space().CatChar('(').Cat(temp_buf).Cat("-->").Cat(pSymb);
				SetError(PPERR_DL6_SYMBIDBUSY, msg_buf);
			}
			else {
				id = newId;
				Tab.Add(name, id, 0);
			}
		}
		else {
			do {
				id = GetNewSymbID();
			} while(Tab.GetByAssoc(id, temp_buf));
			Tab.Add(name, id, 0);
		}
	}
	return id;
}

DLSYMBID SLAPI DlContext::CreateSymb(const char * pSymb, int prefix, long flags)
{
	return Helper_CreateSymb(pSymb, 0, prefix, flags);
}

DLSYMBID SLAPI DlContext::CreateSymbWithId(const char * pSymb, DLSYMBID id, int prefix, long flags)
{
	return Helper_CreateSymb(pSymb, id, prefix, flags);
}

int DlContext::ResolveVar(const DlScope * pScope, int exactScope, const char * pSymb, CtmVar * pVar)
{
	int    ok = 0;
	CtmVar var;
	var.ScopeID = 0;
	var.Pos = 0;
	const DlScope * p_scope = pScope;
	while(!ok && p_scope) {
		uint   pos = 0;
		uint   prev_scope_kind = p_scope->GetKind();
		const  DlScope * p_base_scope = p_scope->GetBase();
		if(p_scope->SearchName(pSymb, &pos)) {
			var.ScopeID = p_scope->ID;
			var.Pos = pos;
			ok = 1;
		}
		else if(p_base_scope && ResolveVar(p_base_scope->GetId(), 1, pSymb, &var)) { // @recursion
			ok = 1;
		}
		else if(exactScope) {
			if(p_scope->GetKind() == DlScope::kExpData) {
				if(SearchVarInChildList(p_scope, DlScope::kExpDataHdr, pSymb, &var))
					ok = 1;
			}
			p_scope = 0;
		}
		else {
			p_scope = p_scope->GetOwner();
			if(p_scope && p_scope->GetKind() == DlScope::kExpData && prev_scope_kind == DlScope::kExpDataIter) {
				if(SearchVarInChildList(p_scope, DlScope::kExpDataHdr, pSymb, &var))
					ok = 1;
			}
		}
	}
	if(!ok)
		SetError(PPERR_DL6_SYMBNFOUND, pSymb);
	ASSIGN_PTR(pVar, var);
	return ok;
}

int DlContext::ResolveVar(DLSYMBID scopeID, int exactScope, const char * pSymb, CtmVar * pVar)
{
	return ResolveVar(GetScope(scopeID), exactScope, pSymb, pVar);
}

#ifndef DL600C // {

int SLAPI DlContext::InitSpecial(int ispc)
{
	int    ok = 1;
	SString file_name;
	if(ispc == ispcExpData)
		PPGetFilePath(PPPATH_BIN, "ppexp.bin", file_name);
	else if(ispc == ispcInterface)
		PPGetFilePath(PPPATH_BIN, "ppifc.bin", file_name);
	else
		ok = 0;
	return ok ? Init(file_name) : 0;
}

#endif // }

int SLAPI DlContext::Init(const char * pInFileName)
{
	int    ok = 1;
	if(Flags & fCompile) {
#ifdef DL600C // {
		Sc.SetupTitle(DlScope::kGlobal, "global");
		Sc.ID = GetNewSymbID();
		CurScopeID = Sc.GetId();
		THROW(InitFileNames(pInFileName));
		LastError = 0;
		THROW(RestoreUuidList());
#endif // } DL600C
	}
	else {
		THROW(InitFileNames(pInFileName));
		THROW(Read_Code());
		THROW(S.Init(64*1024));
	}
	CATCHZOK
	return ok;
}

int SLAPI DlContext::InitFileNames(const char * pInFileName)
{
	if(Flags & fCompile) {
		InFileName = pInFileName;
		SPathStruc ps;
		ps.Split(InFileName);
		ps.Ext = "h";
		ps.Merge(CDeclFileName);
		ps.Ext = "cpp";
		ps.Merge(CImplFileName);
		ps.Ext = "bin";
		ps.Merge(BinFileName);
	}
	else
		BinFileName = pInFileName;
	return 1;
}

const char * SLAPI DlContext::GetInputFileName() const
{
	return InFileName.cptr();
}

SLAPI DlContext::~DlContext()
{
	delete P_M;
}

DLSYMBID FASTCALL DlContext::SearchUuid(const S_GUID & rUuid) const
{
	long   key = 0;
	if(!UuidList.SearchVal(rUuid, &key, 0)) {
		key = 0;
		SString msg_buf;
		SetError(PPERR_UUIDNFOUND, rUuid.ToStr(S_GUID::fmtIDL, msg_buf).Quot('[', ']'));
	}
	return (DLSYMBID)key;
}

int SLAPI DlContext::GetUuidByScopeID(DLSYMBID scopeID, S_GUID * pUuid) const
{
	int    ok = 1;
	if(!UuidList.Search(scopeID, pUuid, 0)) {
		SString msg_buf;
		ok = SetError(PPERR_UUIDNFOUND, msg_buf.CatChar('[').Cat(scopeID).CatChar(']'));
	}
	return ok;
}

int SLAPI DlContext::GetInterface(const S_GUID & rIID, DLSYMBID * pScopeID, const DlScope ** ppScope) const
{
	int    ok = 0;
	DLSYMBID scope_id = SearchUuid(rIID);
	if(scope_id) {
		const DlScope * p_scope = GetScope_Const(scope_id);
		if(p_scope) {
			if(p_scope->IsKind(DlScope::kInterface)) {
				ASSIGN_PTR(pScopeID, scope_id);
				ASSIGN_PTR(ppScope, p_scope);
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI DlContext::EnumInterfacesByICls(const DlScope * pCls, uint * pI,
	DlScope::IfaceBase * pIfb, const DlScope ** ppIfaceScope) const
{
	int    ok = -1;
	const  uint ibc = pCls->GetIfaceBaseCount();
	uint   i = *pI;
	if(i < ibc) {
		DlScope::IfaceBase ifb;
		if(pCls->GetIfaceBase(i, &ifb)) {
			ASSIGN_PTR(pIfb, ifb);
			if(ppIfaceScope) {
				const DlScope * p_ifs = GetScope_Const(ifb.ID);
				*ppIfaceScope = p_ifs;
				ok = (p_ifs && p_ifs->IsKind(DlScope::kInterface)) ? 1 : 0;
			}
			else
				ok = 1;
		}
		else
			ok = 0;
		*pI = ++i;
	}
	return ok;
}

int SLAPI DlContext::RegisterICls(const DlScope * pCls, int unreg)
{
#ifdef DL600C
	EXCEPTVAR(LastError);
#else
	EXCEPTVAR(PPErrCode);
#endif
	int    ok = -1;
	if(pCls == 0) {
		ok = RegisterICls(&Sc, unreg); // @recursion
	}
	else if(pCls->IsKind(DlScope::kIClass)) {
		WinRegKey reg;
		SString key_buf, clsid_buf, name_buf;
		S_GUID clsid, libid;
		if(GetUuidByScopeID(pCls->ID, &clsid)) {
			if(unreg) {
				clsid.ToStr(S_GUID::fmtIDL, clsid_buf).Quot('{', '}');
				(key_buf = "CLSID").CatChar('\\').Cat(clsid_buf);
				reg.Delete(HKEY_CLASSES_ROOT, key_buf);

				MakeClassName(pCls, clsnfRegisterNoVersion, key_buf);
				reg.Delete(HKEY_CLASSES_ROOT, key_buf);

				MakeClassName(pCls, clsnfRegister, key_buf);
				reg.Delete(HKEY_CLASSES_ROOT, key_buf);
			}
			else {
				SString root_buf;
				//
				// HKEY_CLASSES_ROOT\\CLSID\\uuid
				//
				clsid.ToStr(S_GUID::fmtIDL, clsid_buf).Quot('{', '}');
				(root_buf = "CLSID").CatChar('\\').Cat(clsid_buf);

				reg.Open(HKEY_CLASSES_ROOT, root_buf, 0, 0);
				MakeClassName(pCls, clsnfFriendly, name_buf);
				reg.PutString(0, name_buf);

				reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("InprocServer32"), 0, 0);
				reg.PutString(0, SLS.GetExePath());
				reg.PutString("ThreadingModel", "Apartment");

				reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("ProgID"), 0, 0);
				MakeClassName(pCls, clsnfRegister, name_buf);
				reg.PutString(0, name_buf);

				if(pCls->GetVersion()) {
					reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("VersionIndependentProgID"), 0, 0);
					MakeClassName(pCls, clsnfRegisterNoVersion, name_buf);
					reg.PutString(0, name_buf);
				}

				const DlScope * p_parent = pCls->GetOwner();
				while(p_parent)
					if(p_parent->IsKind(DlScope::kLibrary)) {
						if(GetUuidByScopeID(p_parent->ID, &libid)) {
							reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("TypeLib"), 0, 0);
							libid.ToStr(S_GUID::fmtIDL, clsid_buf).Quot('{', '}');
							reg.PutString(0, clsid_buf);
						}
						break;
					}
					else
						p_parent = p_parent->GetOwner();
				//
				// HKEY_CLASSES_ROOT\\class_name (no version)
				//
				MakeClassName(pCls, clsnfRegisterNoVersion, root_buf);
				reg.Open(HKEY_CLASSES_ROOT, key_buf = root_buf, 0, 0);
				MakeClassName(pCls, clsnfFriendly, name_buf);
				reg.PutString(0, name_buf);

				reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("CLSID"), 0, 0);
				clsid.ToStr(S_GUID::fmtIDL, clsid_buf).Quot('{', '}');
				reg.PutString(0, clsid_buf);
				if(pCls->GetVersion()) {
					reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("CurVer"), 0, 0);
					MakeClassName(pCls, clsnfRegister, name_buf);
					reg.PutString(0, name_buf);
				}
				//
				// HKEY_CLASSES_ROOT\\class_name (version)
				//
				if(pCls->GetVersion()) {
					MakeClassName(pCls, clsnfRegister, root_buf);
					reg.Open(HKEY_CLASSES_ROOT, key_buf = root_buf, 0, 0);
					MakeClassName(pCls, clsnfFriendly, name_buf);
					reg.PutString(0, name_buf);

					reg.Open(HKEY_CLASSES_ROOT, (key_buf = root_buf).SetLastSlash().Cat("CLSID"), 0, 0);
					clsid.ToStr(S_GUID::fmtIDL, clsid_buf).Quot('{', '}');
					reg.PutString(0, clsid_buf);
				}
			}
			ok = 1;
		}
	}
	else {
		if(pCls->IsKind(DlScope::kLibrary))
			ok = RegisterTypeLib(pCls, unreg);
		DlScope * p_child = 0;
		for(uint i = 0; pCls->EnumChilds(&i, &p_child);) {
			int    r = RegisterICls(p_child, unreg); // @recursion
			THROW(r);
			if(r > 0)
				ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI DeleteKey(HKEY hKey, uint32 ver, const char * pKeyBuf)
{
	int    ok = -1;
	if(pKeyBuf && strlen(pKeyBuf)) {
		SString key_buf, entry;
		WinRegKey reg;
		key_buf.CopyFrom(pKeyBuf);
		reg.Open(hKey, key_buf, 0, 0);
		uint   idx = 0;
		while(idx < 2 && reg.EnumKeys(&idx, entry) > 0)
			;
		if(idx > 1)
			key_buf.CatChar('\\').Cat(LOWORD(ver)).Dot().Cat(HIWORD(ver));
		reg.Close();
		ok = reg.Delete(hKey, key_buf);
	}
	return ok;
}

int SLAPI DlContext::RegisterTypeLib(const DlScope * pCls, int unreg)
{
	int    ok = 1;
	if(unreg) {
		S_GUID libid;
		if((ok = GetUuidByScopeID(pCls->ID, &libid))) {
			int    del_classes_root = 0, del_hklm_root = 0;
			uint32 ver = pCls->GetVersion();
			SString key_buf, libid_buf;
			libid.ToStr(S_GUID::fmtIDL, libid_buf).Quot('{', '}');
			THROW(DeleteKey(HKEY_CLASSES_ROOT,  ver, (key_buf = "TypeLib").SetLastSlash().Cat(libid_buf)));
			// THROW(DeleteKey(HKEY_LOCAL_MACHINE, ver, (key_buf = "Software\\Classes\\TypeLib\\").Cat(libid_buf)));
			// ok = (UnRegisterTypeLib(libid, LOWORD(ver), HIWORD(ver), GetUserDefaultLCID(), SYS_WIN32) == S_OK) ? 1 : 0;
		}
	}
	else {
		BSTR buf = NULL;
		ITypeLib * p_tbl = 0;
		SString path = SLS.GetExePath();
		path.CopyToOleStr(&buf);
		HRESULT res = LoadTypeLibEx(buf, REGKIND_REGISTER, &p_tbl);
		ok = (res == S_OK) ? 1 : 0;
		CALLPTRMEMB(p_tbl, Release());
		if(buf)
			SysFreeString(buf);
	}
	CATCHZOK
	return ok;
}

size_t SLAPI DlContext::GetTypeBinSize(DLSYMBID typID) const
{
	TypeEntry te;
	return SearchTypeID(typID, 0, &te) ? te.T.GetBinSize() : 0;
}

int SLAPI DlContext::TypeCast(DLSYMBID srcTyp, DLSYMBID destTyp, int cvt, const void * pSrcData, void * pDestData, int * pLoss)
{
	int    ok = tcrUnable;
	int    loss = 0;
	int    cvt_done = 0;
	TypeEntry te_src, te_dest;
	uint   pos_src = 0, pos_dest = 0;
	if(srcTyp == destTyp) {
		if(cvt) {
			if(te_src.T.IsZStr(0)) {
				SString * p_dest_str = *(SString **)pDestData;
				SString * p_src_str = *(SString **)pSrcData;
				*p_dest_str = *p_src_str;
			}
			else {
				size_t sz = GetTypeBinSize(srcTyp);
				if(sz)
					memcpy(pDestData, pSrcData, sz);
			}
			cvt_done = 1;
		}
		ok = tcrEqual;
	}
	else {
		THROW(SearchTypeID(srcTyp,  &pos_src,  &te_src));
		THROW(SearchTypeID(destTyp, &pos_dest, &te_dest));
		if(te_dest.T.IsPure() && GETSTYPE(te_dest.T.Typ) == S_VOID) {
			ok = tcrCastLoss;
		}
		else if(te_src.T.IsPure() && te_dest.T.IsPure()) {
			int    base_src  = stbase(te_src.T.Typ);
			if(GETSTYPE(te_dest.T.Typ) == S_LOGICAL) {
				if(GETSTYPE(te_src.T.Typ) == S_LOGICAL) {
					ok = tcrCast;
				}
				else if(oneof5(base_src, BTS_INT, BTS_REAL, BTS_DATE, BTS_TIME, BTS_STRING)) {
					ok = tcrCastLoss;
					loss = 1;
				}
				if(ok > 0 && cvt) {
					if(base_src == BTS_STRING) {
						int32  b = (*(SString **)pSrcData)->Empty() ? 0 : 1;
						stcast(MKSTYPE(S_INT, 4), te_dest.T.Typ, &b, pDestData, 0);
					}
					else
						stcast(te_src.T.Typ, te_dest.T.Typ, pSrcData, pDestData, 0);
				}
			}
			if(ok < 0) {
				int    base_dest = stbase(te_dest.T.Typ);
				if(base_src == base_dest) {
					size_t sz_src  = stsize(te_src.T.Typ);
					size_t sz_dest = stsize(te_dest.T.Typ);
					if(base_src == BTS_STRING) {
						ok = sz_dest ? tcrCast : tcrEqual;
						if(sz_dest < sz_src)
							loss = 1;
						if(cvt) {
							SString * p_dest_str = *(SString **)pDestData;
							SString * p_src_str = *(SString **)pSrcData;
							*p_dest_str = *p_src_str;
							cvt_done = 1;
						}
					}
					else if(base_src == BTS_INT) {
						ok = tcrCast; // @todo Надо выдавать какое-то замечание если
						// смешивается signed/unsigned или меняется размер
						if(sz_dest < sz_src)
							loss = 1;
					}
					else if(sz_src == sz_dest)
						ok = tcrCast;
					else if(sz_src > sz_dest) {
						ok = tcrCastLoss;
						loss = 1;
					}
					else
						ok = tcrCastSuper;
				}
				else if(base_src == BTS_REAL && base_dest == BTS_INT) {
					ok = tcrCastLoss;
					loss = 1;
				}
				else if(base_src == BTS_INT && base_dest == BTS_REAL)
					ok = tcrCastSuper;
			}
			if(ok > 0 && cvt && !cvt_done)
				stcast(te_src.T.Typ, te_dest.T.Typ, pSrcData, pDestData, 0);
		}
		else if(te_src.T.Mod == STypEx::modLink) {
			int    base_dest = stbase(te_dest.T.Typ);
			if(base_dest == BTS_INT || base_dest == BTS_BOOL) { // @v8.3.0 (|| base_dest == BTS_BOOL)
				ok = tcrCast;
			}
			if(ok > 0 && cvt)
				stcast(MKSTYPE(S_INT, 4), te_dest.T.Typ, pSrcData, pDestData, 0);
		}
	}
	CATCH
		ok = tcrError;
	ENDCATCH
	ASSIGN_PTR(pLoss, loss);
	return ok;
}

int SLAPI DlContext::AddBCmpOps(uint implID, const char * pType)
{
	return (
		AddBOp(dlopEq,  implID+0, "bool", pType, pType, 0) &&
		AddBOp(dlopNeq, implID+1, "bool", pType, pType, 0) &&
		AddBOp(dlopLt,  implID+2, "bool", pType, pType, 0) &&
		AddBOp(dlopLe,  implID+3, "bool", pType, pType, 0) &&
		AddBOp(dlopGt,  implID+4, "bool", pType, pType, 0) &&
		AddBOp(dlopGe,  implID+5, "bool", pType, pType, 0));
}

int SLAPIV DlContext::AddBOp(int op, uint implID, const char * pRetType, ...)
{
	va_list arg_list;
	va_start(arg_list, pRetType);
	char   op_name[8];
	op_name[0] = '?';
	op_name[1] = op;
	op_name[2] = 0;
	int ok = Helper_AddBFunc(op_name, implID, pRetType, arg_list);
	va_end(arg_list);
	return ok;
}

int SLAPIV DlContext::AddBFunc(const char * pFuncName, uint implID, const char * pRetType, ...)
{
	va_list arg_list;
	va_start(arg_list, pRetType);
	int    ok = Helper_AddBFunc(pFuncName, implID, pRetType, arg_list);
	va_end(arg_list);
	return ok;
}

int SLAPI DlContext::Helper_AddBFunc(const char * pFuncName, uint implID, const char * pRetType, va_list pArgList)
{
	int    ok = 1;
	DLSYMBID symb_id = 0;
	DlFunc f;
	f.Name.CatChar('?').Cat(pFuncName);
	if(SearchSymb(pRetType, '@', &symb_id) > 0) {
		f.TypID = symb_id;
		const char * p_arg_typ = va_arg(pArgList, const char *);
		while(ok && p_arg_typ) {
			if(SearchSymb(p_arg_typ, '@', &symb_id))
				f.AddArg(symb_id, 0);
			else
				ok = 0;
			p_arg_typ = va_arg(pArgList, const char *);
		}
		f.Flags |= DlFunc::fImplByID;
		f.ImplID = implID;
		if(ok)
			ok = Sc.AddFunc(&f);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DlContext::GetFunc(const CtmFunc & rF, DlFunc * pFunc)
{
	if(rF.ScopeID) {
		const DlScope * p_scope = GetScope(rF.ScopeID);
		if(p_scope)
			return p_scope->GetFuncByPos(rF.Pos, pFunc);
	}
	return 0;
}

int SLAPI DlContext::GetField(const CtmVar & rV, SdbField * pFld)
{
	if(rV.ScopeID) {
		const DlScope * p_scope = GetScope(rV.ScopeID);
		if(p_scope)
			return p_scope->GetFieldByPos(rV.Pos, pFld);
	}
	return 0;
}

int SLAPI DlContext::GetConstData(const CtmExprConst & rC, void * pBuf, size_t bufLen) const
{
	int    ok = 1;
	TypeEntry te;
	size_t s;
	if(rC.TypeId == 0)
		ok = PPSetError(PPERR_DL6_INVCONSTDESCR);
	else if(SearchTypeID(rC.TypeId, 0, &te)) {
		if(te.T.IsZStr(&s)) {
			memcpy(pBuf, ConstList.GetPtr(&rC, s), MIN(s, bufLen));
		}
		else {
			s = te.T.GetBinSize();
			memcpy(pBuf, ConstList.GetPtr(&rC, s), MIN(s, bufLen));
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DlContext::SearchVarInChildList(const DlScope * pScope, uint childKind, const char * pSymb, CtmVar * pVar)
{
	int    ok = 0;
	uint   pos = 0;
	DlScope * p_child = 0;
	for(uint i = 0; !ok && pScope->EnumChilds(&i, &p_child);)
		if(p_child->GetKind() == childKind && p_child->SearchName(pSymb, &(pos = 0))) {
			pVar->ScopeID = p_child->ID;
			pVar->Pos = pos;
			ok = 1;
		}
	return ok;
}

int FASTCALL DlContext::GetDotFunc(CtmFunc * pF)
{
	int    ok = 1;
	if(!F_Dot.ScopeID || !F_Dot.Pos) {
		SString func_name;
		IntArray func_pos_list;
		const DlScope * p_scope = Sc.SearchByName(DlScope::kGlobal, "global", 0);
		THROW(p_scope);
		THROW(p_scope->GetFuncListByName(func_name.CatCharN('?', 2).CatChar(dlopDot), &func_pos_list));
		F_Dot.ScopeID = p_scope->ID;
		F_Dot.Pos = func_pos_list.at(0);
	}
	ASSIGN_PTR(pF, F_Dot);
	CATCHZOK
	return ok;
}

int FASTCALL DlContext::GetRefFunc(CtmFunc * pF)
{
	int    ok = 1;
	if(!F_Ref.ScopeID || !F_Ref.Pos) {
		SString func_name;
		IntArray func_pos_list;
		const DlScope * p_scope = Sc.SearchByName(DlScope::kGlobal, "global", 0);
		THROW(p_scope);
		THROW(p_scope->GetFuncListByName(func_name.CatCharN('?', 2).CatChar(dlopObjRef), &func_pos_list));
		F_Ref.ScopeID = p_scope->ID;
		F_Ref.Pos = func_pos_list.at(0);
	}
	ASSIGN_PTR(pF, F_Ref);
	CATCHZOK
	return ok;
}

int SLAPI DlContext::GetSymb(DLSYMBID id, SString & rBuf, int prefix) const
{
	int    ok = 1;
	if(Tab.GetByAssoc(id, rBuf)) {
		if(prefix)
			if(rBuf.C(0) == prefix)
				rBuf.ShiftLeft(1);
			else
				ok = SetError(PPERR_DL6_UNMATCHSYMBPREFIX, rBuf);
	}
	else {
		SString msg_buf;
		ok = SetError(PPERR_DL6_SYMBIDNFOUND, msg_buf.Cat(id));
	}
	return ok;
}

int SLAPI DlContext::SearchSymb(const char * pSymb, int prefix, DLSYMBID * pID) const
{
	int    ok = 1;
	uint   id = 0;
	SString temp_buf;
	if(prefix)
		temp_buf.CatChar(prefix);
	temp_buf.Cat(pSymb);
	if(!Tab.Search(temp_buf, &id, 0))
		ok = SetError(PPERR_DL6_SYMBNFOUND, temp_buf);
	else
		ASSIGN_PTR(pID, id);
	return ok;
}

DlScope * SLAPI DlContext::Helper_GetScope(DLSYMBID id, const DlScope * pScope, int kind) const
{
	if(pScope == 0) {
		SString msg_buf;
		SetError(PPERR_DL6_SCOPEIDNFOUND, msg_buf.Cat(id));
	}
	else if(kind && !pScope->IsKind(kind)) {
		SetError(PPERR_DL6_INVSCOPEKIND, pScope->Name);
		pScope = 0;
	}
	return (DlScope *)pScope;
}

DlScope * SLAPI DlContext::GetScope(DLSYMBID id, int kind)
	{ return Helper_GetScope(id, Sc.SearchByID(id, 0), kind); }
DlScope * SLAPI DlContext::GetCurScope()
	{ return Helper_GetScope(CurScopeID, Sc.SearchByID(CurScopeID, 0), 0); }
const DlScope * SLAPI DlContext::GetScope_Const(DLSYMBID id, int kind) const
	{ return (const DlScope *)Helper_GetScope(id, Sc.SearchByID_Const(id, 0), kind); }

const  DlScope * SLAPI DlContext::GetScopeByName_Const(uint kind, const char * pName) const
{
	DLSYMBID parent_id = 0;
	const DlScope * p_scope = Sc.SearchByName_Const(kind, pName, &parent_id);
	return p_scope;
}

int SLAPI DlContext::AddStructType(DLSYMBID symbId)
{
	int    ok = 1;
	TypeEntry entry;
	MEMSZERO(entry);
	entry.SymbID = symbId;
	entry.T.Flags |= STypEx::fStruct;
	entry.T.Link = symbId;
	TypeList.ordInsert(&entry, 0, PTR_CMPFUNC(uint));
	return ok;
}

int SLAPI DlContext::SearchTypeID(DLSYMBID id, uint * pPos, TypeEntry * pEntry) const
{
	int    ok = 1;
	uint   pos = 0;
	if(TypeList.bsearch(&id, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pEntry, TypeList.at(pos));
	}
	else {
		SString msg_buf;
		ok = SetError(PPERR_DL6_UNDEFTYPID, msg_buf.Cat(id));
	}
	return ok;
}

TYPEID SLAPI DlContext::TypeToSType(DLSYMBID id) const
{
	uint   pos = 0;
	return SearchTypeID(id, &pos, 0) ? TypeList.at(pos).T.Typ : 0;
}

DLSYMBID SLAPI DlContext::SetDeclType(DLSYMBID typeID)
{
	return SearchTypeID(typeID, 0, 0) ? typeID : 0;
}

int SLAPI DlContext::MakeClassName(const DlScope * pStruc, int fmt, SString & rBuf) const
{
	if(fmt == clsnfCPP) {
		if(pStruc->IsKind(DlScope::kIClass))
			(rBuf = "DL6ICLS").CatChar('_');
		else if(pStruc->IsKind(DlScope::kExpData))
			(rBuf = "PPALDD").CatChar('_');
		else
			rBuf.Z();
		rBuf.Cat(pStruc->GetName());
		if(pStruc->IsKind(DlScope::kDbTable))
			rBuf.Cat("Tbl");
	}
	else if(fmt == clsnfRegister) {
		(rBuf = "Papyrus").Dot().Cat(pStruc->GetName());
		uint32 ver = pStruc->GetVersion();
		if(ver) {
			rBuf.Dot().Cat(LoWord(ver)).Dot().Cat(HiWord(ver));
		}
	}
	else if(fmt == clsnfRegisterNoVersion) {
		(rBuf = "Papyrus").Dot().Cat(pStruc->GetName());
	}
	else if(fmt == clsnfFriendly) { // (clsnfRegisterNoVersion->clsnfFriendly) @v5.4.5 AHTOXA
		rBuf = pStruc->GetName();
	}
	else
		rBuf.Z();
	return 1;
}

DLSYMBID SLAPI DlContext::SearchSTypEx(const STypEx & rTyp, TypeEntry * pEntry) const
{
	uint   c = TypeList.getCount();
	if(c) do {
		--c;
		if(TypeList.at(c).T.IsEqual(rTyp)) {
			ASSIGN_PTR(pEntry, TypeList.at(c));
			return TypeList.at(c).SymbID;
		}
	} while(c);
	return 0;
}

int SLAPI DlContext::DemangleType(const char * pTypeStr, STypEx * pTyp, DLSYMBID * pID)
{
	STypEx t;
	t.Init();
	DLSYMBID symb_id = 0;
	SStrScan scan(pTypeStr);
	if(scan[1] == 'A') {
		if(scan[0] == 'Q') {
			t.Mod = STypEx::modArray;
			scan.Incr(2);
		}
		else if(scan[0] == 'P') {
			t.Mod = STypEx::modPtr;
			scan.Incr(2);
		}
		else if(scan[0] == 'A') {
			t.Mod = STypEx::modRef;
			scan.Incr(2);
		}
	}
	if(scan[0] == 'V') {
		scan.Incr();
		if(scan.Search("@@")) {
			SString type_symb;
			DLSYMBID type_id = 0;
			if(SearchSymb(scan.Get(type_symb), '@', &type_id)) {

			}
		}
	}
	else {
		uint c = TypeList.getCount();
		if(c)
			do {
				--c;
				const TypeEntry & r_te = TypeList.at(c);
				if(r_te.MangleC == scan[0]) {
					t = r_te.T;
					symb_id = r_te.SymbID;
					c = 0;
				}
			} while(c);
		if(symb_id) {
		}
	}
	return 0;
}

long SLAPI DlContext::ParseFormat(const char * pFmtStr, TYPEID tp) const
{
	int    oset = 0;
	const  char * curpos = pFmtStr;
	FormatSpec fs;
	MEMSZERO(fs);
	const  char first = pFmtStr ? pFmtStr[0] : 0;
	switch(GETARYSTYPE(tp)) {
		case S_INT:
			if(pFmtStr && !oneof3(first, '>', '|', '<'))
				fs.flags = ALIGN_RIGHT;
			break;
		case S_FLOAT:
			if(pFmtStr && !oneof3(first, '>', '|', '<'))
				fs.flags = ALIGN_RIGHT;
			fs.prec = 2;
			break;
		case S_DEC:
			if(pFmtStr && !oneof3(first, '>', '|', '<'))
				fs.flags = ALIGN_RIGHT;
			fs.prec = (short)GETSPRECD(tp);
			break;
		case S_ZSTRING:
			fs.len = (short)GETSSIZE(tp);
		case S_DATE:
		case S_TIME:
			if(pFmtStr && !oneof3(first, '>', '|', '<'))
				fs.flags = ALIGN_LEFT;
			break;
	}
	if(first) {
		switch(first) {
			case '>': fs.flags = ALIGN_RIGHT;  curpos++; break;
			case '<': fs.flags = ALIGN_LEFT;   curpos++; break;
			case '|': fs.flags = ALIGN_CENTER; curpos++; break;
		}
		if(*curpos == '*') {
			fs.flags |= COMF_FILLOVF;
			curpos++;
		}
		if(atoi(curpos))
			fs.len = atoi(curpos);
		if(oneof2(GETSTYPE(tp), S_FLOAT, S_DEC))
			if(curpos = strchr(curpos, '.'))
				fs.prec = atoi(curpos+1);
		if(curpos = strchr(pFmtStr, '@')) {
			switch(toupper(curpos[1])) {
				case 'U': fs.flags |= STRF_UPPER;    break;
				case 'L': fs.flags |= STRF_LOWER;    break;
				case 'P': fs.flags |= STRF_PASSWORD; break;
			}
		}
		if(curpos = strchr(pFmtStr, '#')) {
			switch(toupper(curpos[1])) {
				case 'A': fs.flags |= DATF_AMERICAN; break;
				case 'G': fs.flags |= DATF_GERMAN;   break;
				case 'B': fs.flags |= DATF_BRITISH;  break;
				case 'I': fs.flags |= DATF_ITALIAN;  break;
				case 'J': fs.flags |= DATF_JAPAN;    break;
				case 'F': fs.flags |= DATF_FRENCH;   break;
				case 'U': fs.flags |= DATF_USA;      break;
				case 'D': fs.flags |= DATF_DMY;      break;
				case 'W': fs.flags |= DATF_ANSI;     break;
				case 'M': fs.flags |= DATF_MDY;      break;
				case 'Y': fs.flags |= DATF_YMD;      break;
			}
			if(toupper(curpos[2]) == 'C') {
				fs.flags |= DATF_CENTURY;
				SETIFZ(fs.len, 10);
			}
			else
				SETIFZ(fs.len, 8);
		}
		if(curpos = strchr(pFmtStr, '$')) {
			for(curpos++; *curpos; curpos++)
				switch(toupper(*curpos)) {
					case 'C': fs.flags |= NMBF_TRICOMMA;  break;
					case 'A': fs.flags |= NMBF_TRIAPOSTR; break;
					case 'S': fs.flags |= NMBF_TRISPACE;  break;
					case 'Z': fs.flags |= NMBF_NOZERO;    break;
					case 'F': fs.flags |= NMBF_FORCEPOS;  break;
					case 'N': fs.flags |= NMBF_NOTRAILZ;  break;
				}
		}
		if((curpos = strchr(pFmtStr, '&')) != 0) {
			switch(curpos[1]) {
				case 'F':
				case 'f': SETIFZ(fs.len, 11); fs.flags |= TIMF_HMS | TIMF_MSEC; break;
				case 'N':
				case 'n': SETIFZ(fs.len, 8);  fs.flags |= TIMF_HMS; break;
				case 'H':
				case 'h': SETIFZ(fs.len, 5);  fs.flags |= TIMF_HM;  break;
				case 'L': SETIFZ(fs.len, 5);  fs.flags |= TIMF_MS | TIMF_MSEC; break;
				case 'l': SETIFZ(fs.len, 5);  fs.flags |= TIMF_MS; break;
				case 'S': SETIFZ(fs.len, 5);  fs.flags |= TIMF_S | TIMF_MSEC; break;
				case 's': SETIFZ(fs.len, 2);  fs.flags |= TIMF_S; break;
			}
			if(toupper(curpos[2]) == 'B')
				fs.flags |= TIMF_BLANK;
		}
	}
	return fs.prec ? MKSFMTD(fs.len, fs.prec, fs.flags) : MKSFMT(fs.len, fs.flags);
}

int SLAPI DlContext::Format_TypeEntry(const TypeEntry & rEntry, SString & rBuf)
{
	int    ok = 1;
	TypeEntry te;
	SString line, name;
	if(!Tab.GetByAssoc(rEntry.SymbID, name))
		name = "noname";
	line.CatEq(name, (long)rEntry.SymbID).CatDiv(':', 1);
	//
	STypEx t = rEntry.T;
	SString styp_buf, temp_buf;
	styp_buf.CatChar('{');
#define FORMAT_FLAG(f) if(t.Flags & STypEx::f) temp_buf.CatDivIfNotEmpty('|', 0).Cat(#f)
	FORMAT_FLAG(fFormula);
	FORMAT_FLAG(fZeroID);
	FORMAT_FLAG(fStruct);
	FORMAT_FLAG(fOf);
	FORMAT_FLAG(fStatic);
#undef FORMAT_FLAG
	if(temp_buf.Empty())
		temp_buf.Cat((long)0L);
	styp_buf.Cat(temp_buf.Quot('<', '>'));
	if(t.Mod == STypEx::modPtr)
		styp_buf.CatChar('*');
	else if(t.Mod == STypEx::modRef)
		styp_buf.CatChar('&');
	else if(t.Mod == STypEx::modLink)
		styp_buf.CatChar('$');
	else if(t.Mod == STypEx::modArray)
		styp_buf.CatChar('[').Cat(t.Dim).CatChar(']');
	if(t.Flags & STypEx::fOf) {
		styp_buf.Space().Cat("of").Space();
		if(SearchTypeID(t.Link, 0, &te))
			Format_TypeEntry(te, styp_buf); // @recursion
		else
			styp_buf.Cat("unnamed");
	}
	else if(t.Mod && t.Link)
		if(SearchTypeID(t.Link, 0, &te))
			Format_TypeEntry(te, styp_buf); // @recursion
		else
			styp_buf.Cat("unnamed");
	else {
		temp_buf = 0;
		temp_buf.Cat(GETSTYPE(t.Typ)).CatChar('(').Cat((long)GETSSIZE(t.Typ)).CatChar(')');
		styp_buf.Cat(temp_buf);
	}
	styp_buf.CatChar('}');
	rBuf.Cat(line).Cat(styp_buf);
	return ok;
}
//
//
//
#ifndef DL600C // {

int SLAPI DlContext::CreateNewDbTableSpec(const DBTable * pTbl)
{
	int    ok = 1;
	uint   i;
	SString temp_buf;
	SString tbl_name = pTbl->tableName;
	DLSYMBID scope_id = CreateSymb(tbl_name, '$', DlContext::crsymfErrorOnDup);
	DlScope * p_scope = new DlScope(scope_id, DlScope::kDbTable, tbl_name, 0);
	SdbField fld;
	for(i = 0; i < pTbl->fields.getCount(); i++) {
		const BNField & r_fld = pTbl->fields[i];
		uint   fld_id = 0;
		fld.Init();
		fld.T.Typ = r_fld.T;
		fld.Name = r_fld.Name;
		p_scope->AddField(&fld_id, &fld);
	}
	for(i = 0; i < pTbl->indexes.getNumKeys(); i++) {
		uint   ci = 0, cc = 0;
		DlScope * p_child = 0;
		const char * p_idx_name = 0;
		for(ci = 0; p_scope->EnumChilds(&ci, &p_child) > 0;) {
			if(p_child->IsKind(DlScope::kDbIndex)) {
				if(!isempty(p_idx_name)) {
					THROW_PP_S(p_child->GetName().CmpNC(p_idx_name) != 0, PPERR_DL6_DUPDBIDXNAME, p_idx_name);
				}
				++cc;
			}
		}
		if(!isempty(p_idx_name))
			temp_buf = p_idx_name;
		else
			(temp_buf = "Key").Cat(cc);
		{
			BNKey key = pTbl->indexes.getKey(i);
			DLSYMBID idx_id = GetNewSymbID();
			THROW_MEM(p_child = new DlScope(idx_id, DlScope::kDbIndex, temp_buf, 0));
			{
				const int f = key.getFlags();
				DlScope::Attr attr;
				MEMSZERO(attr);
				if(f & XIF_DUP) {
					attr.A = DlScope::sfDbiDup;
					p_child->SetAttrib(attr);
				}
				if(f & XIF_MOD) {
					attr.A = DlScope::sfDbiMod;
					p_child->SetAttrib(attr);
				}
				if(f & XIF_ALLSEGNULL) {
					attr.A = DlScope::sfDbiAllSegNull;
					p_child->SetAttrib(attr);
				}
				if(f & XIF_ANYSEGNULL) {
					attr.A = DlScope::sfDbiAnySegNull;
					p_child->SetAttrib(attr);
				}
			}
			p_scope->Add(p_child);
			for(uint j = 0; j < (uint)key.getNumSeg(); j++) {
				const BNField & r_fld = pTbl->indexes.field(i, j);
				THROW(p_child->AddDbIndexSegment(r_fld.Name, key.getFlags(j)));
			}
		}
	}
	THROW(Sc.Add(p_scope));
	CATCHZOK
	return ok;
}

#endif // } !DL600C

int SLAPI DlContext::Helper_LoadDbTableSpec(const DlScope * pScope, DBTable * pTbl, int format) const
{
	int    ok = 1;
	int    key_number = 0;
	uint   k;
	SdbField fld;
	char   s_buf[256];
	CtmExprConst c;
	DlScope * p_index = 0;
	THROW_INVARG(pScope);
	pTbl->tableID = pScope->GetId();
	STRNSCPY(pTbl->tableName, pScope->GetName());
	if(pScope->GetConst(DlScope::cdbtPageSize, &c)) {
		uint32 page_size = 0;
		GetConstData(c, &page_size, sizeof(page_size));
		if(page_size)
			pTbl->PageSize = (uint16)page_size;
	}
	if(pScope->GetConst(DlScope::cdbtFileName, &c)) {
		s_buf[0] = 0;
		GetConstData(c, s_buf, sizeof(s_buf));
		pTbl->fileName = s_buf;
	}
	for(k = 0; pScope->EnumFields(&k, &fld);) {
		//
		// Идентификаторы полям присваиваются автоматически [0..]
		//
		THROW_SL(pTbl->fields.addField(fld.Name, fld.T.Typ));
	}
	for(k = 0; pScope->EnumChilds(&k, &p_index);) {
		if(p_index->IsKind(DlScope::kDbIndex)) {
			int  idx_flags = XIF_EXT;
			BNKey key;
			for(uint j = 0; p_index->EnumFields(&j, &fld);) {
				//
				// Мы должны извлечь поле по имени, чтобы гарантированно получить
				// тот идентификатор поля, который был установлен предшествующим вызовом
				// p_tbl->fields.addField()
				//
				const BNField * p_f = &pTbl->fields.getField(fld.Name);
				THROW(p_f);
				long seg_flags = p_index->GetDbIndexSegOptions(j-1);
				THROW_SL(key.addSegment(p_f->Id, seg_flags));
			}
			if(p_index->GetAttrib(DlScope::sfDbiDup, 0))
				idx_flags |= XIF_DUP;
			if(p_index->GetAttrib(DlScope::sfDbiMod, 0))
				idx_flags |= XIF_MOD;
			if(p_index->GetAttrib(DlScope::sfDbiAllSegNull, 0))
				idx_flags |= XIF_ALLSEGNULL;
			if(p_index->GetAttrib(DlScope::sfDbiAnySegNull, 0))
				idx_flags |= XIF_ANYSEGNULL;
			key.setFlags(idx_flags);
			key.setKeyParams(key_number++, 0);
			pTbl->indexes.addKey(key);
		}
	}
	if(pScope->GetAttrib(DlScope::sfDbtVLR, 0))
		pTbl->flags |= XTF_VLR;
	if(pScope->GetAttrib(DlScope::sfDbtVAT, 0))
		pTbl->flags |= XTF_VAT;
	if(pScope->GetAttrib(DlScope::sfDbtTruncate, 0))
		pTbl->flags |= XTF_TRUNCATE;
	if(pScope->GetAttrib(DlScope::sfDbtCompress, 0))
		pTbl->flags |= XTF_COMPRESS;
	if(pScope->GetAttrib(DlScope::sfDbtBalanced, 0))
		pTbl->flags |= XTF_BALANCED;
	if(pScope->GetAttrib(DlScope::sfDbtTemporary, 0))
		pTbl->flags |= XTF_TEMP;
	if(pScope->GetAttrib(DlScope::sfDbtSystem, 0))
		pTbl->flags |= XTF_DICT;
	pTbl->indexes.setTableRef(offsetof(DBTable, indexes));
	pTbl->InitLob();
	CATCHZOK
	return ok;
}

int SLAPI DlContext::LoadDbTableSpec(DLSYMBID scopeID, DBTable * pTbl, int format) const
{
	int    ok = 1;
	const DlScope * p_scope = Sc.SearchByID_Const(scopeID, 0);
	THROW(p_scope);
	THROW(Helper_LoadDbTableSpec(p_scope, pTbl, format));
	CATCHZOK
	return ok;
}

int SLAPI DlContext::LoadDbTableSpec(const char * pName, DBTable * pTbl, int format) const
{
	int    ok = 1;
	const DlScope * p_scope = Sc.SearchByName_Const(DlScope::kDbTable, pName, 0);
	THROW(p_scope);
	THROW(Helper_LoadDbTableSpec(p_scope, pTbl, format));
	CATCHZOK
	return ok;
}

int SLAPI DlContext::DropDbTableSpec(const char * pName)
{
	int    ok = 1;
	DLSYMBID parent_id = 0;
	const DlScope * p_scope = Sc.SearchByName_Const(DlScope::kDbTable, pName, &parent_id);
	THROW(p_scope);
	DlScope * p_parent = Sc.SearchByID(parent_id, 0);
	THROW(p_parent);
	THROW(p_parent->Remove(p_scope->ID));
	CATCHZOK
	return ok;
}

int SLAPI DlContext::Helper_GetScopeList(int kind, int recursive, StrAssocArray * pList) const
{
	int    ok = 1;
	if(pList) {
		IntArray id_list;
		Sc.GetChildList(kind, recursive, &id_list);
		const uint c = id_list.getCount();
		for(uint i = 0; i < c; i++) {
			const DlScope * p_scope = Sc.SearchByID_Const(id_list.at(i), 0);
			if(p_scope)
				THROW_SL(pList->Add(p_scope->GetId(), p_scope->GetName()));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI DlContext::GetDbTableSpecList(StrAssocArray * pList) const
	{ return Helper_GetScopeList(DlScope::kDbTable, 1, pList); }
int SLAPI DlContext::GetDialogList(StrAssocArray * pList) const
	{ return Helper_GetScopeList(DlScope::kUiDialog, 1, pList); }
//
//
//
DlCtxHdr::DlCtxHdr()
{
	THISZERO();
	Signature[0] = 'D';
	Signature[1] = 'L';
	Signature[2] = '6';
	Signature[3] = 'B';
}

int DlCtxHdr::Check() const
{
	if(Signature[0] != 'D' || Signature[1] != 'L' || Signature[2] != '6' || Signature[3] != 'B')
		return 0;
	return 1;
}

int SLAPI DlContext::Read_Code()
{
	int    ok = 1;
	THROW(fileExists(BinFileName));
	{
		SFile  inf(BinFileName, SFile::mRead|SFile::mBinary);
		int    eot = 0;
		DlCtxHdr hdr;
		SBuffer buf;
		uint   symb_id;
		SString symb;
		uint32 crc;
		THROW(inf.CalcCRC(sizeof(hdr), &crc));
		THROW(inf.Read(&hdr, sizeof(hdr)));
		THROW(hdr.Check() && hdr.Crc32 == crc);
		THROW(inf.Read(buf));
		do {
			THROW(buf.Read(&symb_id, sizeof(symb_id)));
			THROW(buf.Read(symb));
			if(symb_id == 0xffffffffU && symb.Cmp("<<EOT>>", 0) == 0) {
				eot = 1;
			}
			else {
				THROW(Tab.Add(symb, symb_id, 0));
			}
		} while(!eot);
		THROW(ConstList.Read(&buf));
		THROW(buf.Read(&TypeList, 0));
		THROW(buf.Read(&UuidList, 0));
		THROW(Sc.Read(buf));
		THROW(Sc.InitInheritance(&Sc));
	}
	CATCHZOK
	return ok;
}

int DlContext::Test_ReWr_Code(const DlContext & rPattern)
{
	int    ok = 1;
	THROW(Tab.Test_Cmp(rPattern.Tab));
	THROW(ConstList.Test_Cmp(rPattern.ConstList));
	THROW(TypeList.IsEqual(rPattern.TypeList));
	THROW(UuidList.IsEqual(rPattern.UuidList));
	THROW(Sc.IsEqual(rPattern.Sc));
	CATCHZOK
	return ok;
}

int DlContext::SetError(int errCode, const char * pAddedMsg) const
{
#ifdef DL600C
	DlContext::LastError = errCode;
	if(pAddedMsg)
		DlContext::AddedMsgString = pAddedMsg;
#else
	PPSetError(errCode, pAddedMsg);
#endif
	return 0;
}

int DlContext::Error(int errCode, const char * pAddedInfo, long flags /* erfXXX */)
{
#ifdef DL600C // {
	struct MsgEntry {
		uint   Id;
		const  char * P_Msg;
	};
	MsgEntry msg_list[] = {
		{PPERR_NOMEM,                 "Недостаточно памяти"},
		{PPERR_DL6_DATASTRUCEXISTS,   "Структура данных %s уже определена"},
		{PPERR_DL6_DATASTRUCIDNFOUND, "Идентификатор структуры данных %s не найден"},
		{PPERR_DL6_UNDEFSTDTYP,       "Внутренняя ошибка: Не определен стандартный тип '%s'"},
		{PPERR_DL6_UNDEFDATASYMB,     "Неопределенный символ структуры данных '%s'"},
		{PPERR_DL6_UNDEFTYPID,        "Неопределенный идентификатор типа '%s'"},
		{PPERR_DL6_UNDEFTYPSYMB,      "Неопределенный тип '%s'"},
		{PPERR_DL6_UNMATCHSCOPE,      "Нарушена синхронизация областей видимости"},
		{PPERR_DL6_INPUTOPENFAULT,    "Не удалось открыть файл '%s'"},
		{PPERR_DL6_SCOPEIDNFOUND,     "Внутренняя ошибка: ИД области видимости '%s' не найден"},
		{PPERR_DL6_UNMATCHSYMBPREFIX, "Несоответствующий префикс символа '%s'"},
		{PPERR_DL6_SYMBIDNFOUND,      "ИД символа '%s' не найден"},
		{PPERR_DL6_SYMBNFOUND,        "Символ '%s' не найден"},
		{PPERR_DL6_MAXARRAYDIM,       "Превышена максимальная размерность массива"},
		{PPERR_DL6_DUPVARINSCOPE,     "Дублирование переменной '%s' в области видимости"},
		{PPERR_DL6_DUPSYMB,           "Дублирование символа '%s'"},
		{PPERR_DL6_INVLEFTOFDOT,      "Слева от оператора '.' должна быть структура"},
		{PPERR_DL6_INVLEFTOFREF,      "Первым параметром оператора '@()' должна быть структура"},
		{PPERR_DL6_FUNCAMBIG,         "Неоднозначность по параметрам при выборе функции '%s'"},
		{PPERR_DL6_IMPLICITCVTOVRD,   "Внутренняя ошибка: Неявное преобразование типа для терма уже сделано"},
		{PPERR_DL6_NOARGBYPOS,        "Внутренняя ошибка: Не удалось получить аргумент вызова функции по номеру '%s'"},
		{PPERR_DL6_NOFUNCBYPOS,       "Внутренняя ошибка: Не удалось получить функцию по номеру"},
		{PPERR_DL6_NOFUNCBYNAME,      "Неопределенная функция '%s'"},
		{PPERR_DL6_UNMATCHQUESTTYPES, "Несовместимые типы в операторе '?:' (%s)"},
		{PPERR_DL6_ICLSPARNOTIFACE,   "База '%s' класса не является интерфейсом" },
		{PPERR_DL6_CLASSEXISTS,       "Класс %s уже определен" },
		{PPERR_DL6_INVATTRLIB,        "Недопустимый атрибут '%s' библиотеки типов" },
		{PPERR_DL6_NOENUM,            "Попытка вставить элемент перечисления в структуру, не являющуюся перечислением" },
		{PPERR_UUIDNFOUND,            "UUID %s не найден" },
		{PPERR_DL6_INVSCOPEKIND,      "Внутренняя ошибка: область видимости DL600 '%s' принадлежит другому виду" },
		{PPERR_DL6_IMPFILENFOUND,     "Импортируемый файл '%s' не найден" },
		{PPERR_DL6_DUPTYPEDEFINSCOPE, "Дублирование typedef-определения '%s' в области видимости" },
		{PPERR_DL6_ARGMUSTBECONST,    "Аргумент должен быть константой" },
		{PPERR_DL6_DBFLDNOTINDBSCOPE, "Попытка создать поле '%s' базы данных вне области таблицы базы данных" },
		{PPERR_DL6_INVDECDIMUSAGE,    "Недопустимое использование числовой размерности '%s'" },
		{PPERR_DL6_DUPDBIDXNAME,      "Дублирование наименования индекса '%s'" },
		{PPERR_DL6_IDXSEGINVFLDNAME,  "Поле '%s' сегмента индекса не принадлежит таблице" },
		{PPERR_DL6_INVIDXSEGFLAG,     "Неизвестная опция '%s' сегмента индекса" },
		{PPERR_DL6_MISSIDXUNIQ,       "Для индекса одновременно определены опции UNIQUE и DUP" },
		{PPERR_DL6_INVDBFILEPAGE,     "Недопустимый размер страницы файла данных (%s)" },
		{PPERR_DL6_INVDBFILEOPTION,   "Неизвестная опция '%s' таблицы базы данных" },
		{PPERR_DL6_INVIDXFLAG,        "Неизвестная опция '%s' индекса базы данных" },
		{PPERR_DL6_DECMODONNONDECTYPE, "Числовая размерность %s применена не к десятичному типу данных" },
		{PPERR_DL6_DDFENTRYCRFAULT,   "Ошибка создания элемента словаря базы данных: %s" },
		{PPERR_DL6_BTRFILECRFAULT,    "Ошибка создания файла Btrieve: %s" },
		{PPERR_DL6_DBDICTOPENFAULT,   "Ошибка открытия словаря данных '%s'" },
		{PPERR_DL6_UNDEFDICTPATH,     "Не определен каталог словаря для создания таблиц базы данных" },
		{PPERR_DL6_FUNCINVSCOPE,      "Недопустимый вид области видимости для декларации функции" },
		{PPERR_DL6_INVPROPSYMB,       "Неизвестное имя свойства '%s'" },
		{PPERR_DL6_SYMBIDBUSY,        "Идентификатор символа занят: %s" },
		{PPERR_DL6_INVPROPTYPE,       "Недопустимый тип данных для свойства '%s'" },
		{PPERR_DL6_INVCONSTDESCR,     "Недопустимый дескриптор константы" }
	};
	SETIFZ(errCode, LastError);
	const char * p_msg = 0;
	for(uint i = 0; i < sizeof(msg_list) / sizeof(msg_list[0]); i++)
		if(msg_list[i].Id == errCode)
			p_msg = msg_list[i].P_Msg;
	SString msg_buf, temp_buf;
	SString added_info = NZOR(pAddedInfo, AddedMsgString);
	if(added_info.Empty())
		added_info.Space() = 0;
	if(p_msg == 0)
		p_msg = "Unknown error";
	temp_buf.Printf(p_msg, added_info.cptr());
	// @v7.8.5 msg_buf.Printf("%s(%d): error dl600: %s", (const char *)InFileName, yyline, (const char *)temp_buf);
	msg_buf.FormatFileParsingMessage(InFileName, yyline, 0).Cat("error").Space().Cat("dl600").CatDiv(':', 2).Cat(temp_buf); // @v7.8.5
	if(flags & erfLog)
		SLS.LogMessage(LogFileName, msg_buf);
	printf(msg_buf.CR().ToOem().cptr());
	if(flags & erfExit)
		exit(-1);
	return 0;
#else
	return PPError(errCode, pAddedInfo);
#endif
}
//
//
//
/*
MangledName:
	'?' BasicName Qualification '@' QualifiedTypeCode StorageClass
	'?' BasicName '@' UnqualifiedTypeCode StorageClass
BasicName:
	'?' operatorCode
	string '@'
Qualification:
	( string '@' )*
QualifiedTypeCode:
	'Q' FunctionTypeCode
	'2' DataTypeCode
UnqualifiedTypeCode:
	'Y' FunctionTypeCode
	'3' DataTypeCode
StorageClass:
	'A' (Normal Storage)
	'B' (Volatile Storage)
	'C' (Const Storage)
	'Z' (Executable Storage)
FunctionTypeCode:
	CallingConvention ReturnValueTypeCode ArgumentList
*/

