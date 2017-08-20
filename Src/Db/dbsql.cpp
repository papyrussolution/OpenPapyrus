// DBSQL.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2013, 2015, 2016, 2017
//
#include <db.h>
#pragma hdrstop
#include <pp.h> // @test
//
// @example: SELECT employees_seq.nextval FROM DUAL;
//
/*
	Параметры для создания базы данных:

	name          descr       sample
	-----------   ----------- ---------------
	ORACLE_BASE               c:\oracle
	ORACLE_HOME               c:\oracle\ora92
	DB_NAME                   next02
	SID                       next02

//
// Системная таблица Oracle
//
table OraAllTables {
	string OWNER[31];
	string TABLE_NAME[31];
	string TABLESPACE_NAME[31];
	string CLUSTER_NAME[31];
	string IOT_NAME[31];
	int32  PCT_FREE;
	int32  PCT_USED;
	int32  INI_TRANS;
	int32  MAX_TRANS;
	int32  INITIAL_EXTENT;
	int32  NEXT_EXTENT;
	int32  MIN_EXTENTS;
	int32  MAX_EXTENTS;
	int32  PCT_INCREASE;
	int32  FREELISTS;
	int32  FREELIST_GROUPS;
	string LOGGING[4];
	string BACKED_UP[2];
	int32  NUM_ROWS;
	int32  BLOCKS;
	int32  EMPTY_BLOCKS;
	int32  AVG_SPACE;
	int32  CHAIN_CNT;
	int32  AVG_ROW_LEN;
	int32  AVG_SPACE_FREELIST_BLOCKS;
	int32  NUM_FREELIST_BLOCKS;
	string DEGREE[11];
	string INSTANCES[11];
	string CACHE[6];
	string TABLE_LOCK[9];
	int32  SAMPLE_SIZE;
	date   LAST_ANALYZED; // datetime
	string PARTITIONED[4];
	string IOT_TYPE[13];
	string TEMPORARY[2];
	string SECONDARY[2];
	string NESTED[4];
	string BUFFER_POOL[8];
	string ROW_MOVEMENT[9];
	string GLOBAL_STATS[4];
	string USER_STATS[4];
	string DURATION[16];
	string SKIP_CORRUPT[9];
	string MONITORING[4];
	string CLUSTER_OWNER[31];
	string DEPENDENCIES[9];
file:
	"all_tables";
	system;
}

table OraAllIndexes {
	string GENERATED[2];
	string SECONDARY[2];
	string BUFFER_POOL[8];
	string USER_STATS[4];
	string DURATION[16];
	int32  PCT_DIRECT_ACCESS;
	string ITYP_OWNER[31];
	string ITYP_NAME[31];
	string PARAMETERS[1001];
	string GLOBAL_STATS[4];
	string DOMIDX_STATUS[13];
	string DOMIDX_OPSTATUS[7];
	string FUNCIDX_STATUS[9];
	string JOIN_INDEX[4];
	string OWNER[31];
	string INDEX_NAME[31];
	string INDEX_TYPE[28];
	string TABLE_OWNER[31];
	string TABLE_NAME[31];
	string TABLE_TYPE[6];
	string UNIQUENESS[10];
	string COMPRESSION[9];
	int32  PREFIX_LENGTH;
	string TABLESPACE_NAME[31];
	int32  INI_TRANS;
	int32  MAX_TRANS;
	int32  INITIAL_EXTENT;
	int32  NEXT_EXTENT;
	int32  MIN_EXTENTS;
	int32  MAX_EXTENTS;
	int32  PCT_INCREASE;
	int32  PCT_THRESHOLD;
	int32  INCLUDE_COLUMN;
	int32  FREELISTS;
	int32  FREELIST_GROUPS;
	int32  PCT_FREE;
	string LOGGING[4];
	int32  BLEVEL;
	int32  LEAF_BLOCKS;
	int32  DISTINCT_KEYS;
	int32  AVG_LEAF_BLOCKS_PER_KEY;
	int32  AVG_DATA_BLOCKS_PER_KEY;
	int32  CLUSTERING_FACTOR;
	string STATUS[9];
	int32  NUM_ROWS;
	int32  SAMPLE_SIZE;
	date   LAST_ANALYZED; // datetime
	string DEGREE[41];
	string INSTANCES[41];
	string PARTITIONED[4];
	string TEMPORARY[2];
file:
	"all_indexes";
	system;
}
*/

#ifndef NDEBUG
	#define DEBUG_LOG(msg) SLS.LogMessage("dbora.log", msg, 0)
#else
	#define DEBUG_LOG(msg)
#endif

SOraDbProvider::OH::operator uint32 () const 
{ 
	return (uint32)H; 
}

//static
SOraDbProvider::OH FASTCALL SOraDbProvider::StmtHandle(const SSqlStmt & rS)
{
	OH h;
	h.T = OCI_HTYPE_STMT;
	h.H = (void *)rS.H;
	return h;
}

SSqlStmt::Bind::Bind()
{
	THISZERO();
}

IMPL_INVARIANT_C(SSqlStmt::Bind)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P((Flags & ~(fSubst|fCalcOnly)) == 0, pInvP);
	S_ASSERT_P(Dim > 0 && Dim <= 1024, pInvP);
	S_ASSERT_P(SubstSize >= Dim*ItemSize, pInvP);
	S_ASSERT_P(SubstOffs >= 4, pInvP);
	S_ASSERT_P(ItemSize >= NtvSize, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

SSqlStmt::BindArray::BindArray(uint dim) : TSArray <SSqlStmt::Bind>()
{
	Dim = dim;
}

SSqlStmt::SSqlStmt(DbProvider * pDb, const char * pText) : Descr(SdRecord::fAllowDupName)
{
	Flags = 0;
	H = 0;
	BS.Init();
	IndSubstPlus = 0;
	IndSubstMinus = 0;
	FslSubst = 0;
	InitBinding();
	if(pDb) {
		P_Db = pDb;
		SetText(pText);
	}
	else
		Flags |= fError;
}

SSqlStmt::~SSqlStmt()
{
	CALLPTRMEMB(P_Db, DestroyStmt(this));
	BS.Destroy();
}

int SSqlStmt::SetText(const char * pText)
{
	int    ok = 1;
	if(pText && P_Db) {
		if(!P_Db->CreateStmt(this, pText, 0)) {
			Flags |= fError;
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}

int SSqlStmt::IsValid() const
{
	return (Flags & fError) ? 0 : 1;
}

int SSqlStmt::SetupBindingSubstBuffer(int dir, uint count)
{
	int    ok = 1;
	size_t size = 4; // Первые 4 байта буфера зарезервированы.
	uint   col_count = 0;
	for(uint i = 0; i < BL.getCount(); i++) {
		Bind & r_bind = BL.at(i);
		if((dir < 0 && r_bind.Pos < 0) || (dir > 0 && r_bind.Pos > 0)) {
			r_bind.Flags |= Bind::fCalcOnly;
			THROW(P_Db->ProcessBinding(0, count, this, &r_bind));
			r_bind.Flags &= ~Bind::fCalcOnly;
			size += r_bind.SubstSize;
			col_count++;
		}
	}
	if(dir < 0) {
		size += col_count * count * sizeof(uint16); // Резервируем пространство для значений индикаторов
	}
	else if(dir > 0) {
		size += col_count * count * sizeof(uint16); // Резервируем пространство для значений индикаторов
		size += col_count * count * sizeof(uint16); // Резервируем пространство для значений длин выборок (FSL)
	}
	BS.Alloc(size);
	CATCHZOK
	return ok;
}

size_t SSqlStmt::Helper_AllocBindSubst(uint32 itemCount, uint32 itemSize, int calcOnly)
{
	size_t pos = 0;
	size_t s = ALIGNSIZE(itemCount * itemSize, 2);
	if(calcOnly) {
		pos = s;
	}
	else {
		size_t new_size = (TopBindSubst + s);
		assert(new_size <= BS.Size);
		if(new_size <= BS.Size) {
			memzero(BS.P_Buf+TopBindSubst, s);
			pos = TopBindSubst;
			TopBindSubst += s;
		}
	}
	return pos;
}

int SSqlStmt::AllocIndSubst(uint32 itemCount, Bind * pBind)
{
	size_t pos = Helper_AllocBindSubst(itemCount, sizeof(uint16), BIN(pBind->Flags & SSqlStmt::Bind::fCalcOnly));
	pBind->IndPos = pos;
	return BIN(pos);
}

int SSqlStmt::AllocFslSubst(uint32 itemCount, Bind * pBind)
{
	size_t pos = Helper_AllocBindSubst(itemCount, sizeof(uint16), BIN(pBind->Flags & SSqlStmt::Bind::fCalcOnly));
	pBind->FslPos = pos;
	return BIN(pos);
}

int SSqlStmt::AllocBindSubst(uint32 itemCount, uint32 itemSize, Bind * pBind)
{
	int    ok = 0;
	const  int calc_only = BIN(pBind->Flags & SSqlStmt::Bind::fCalcOnly);
	size_t pos = Helper_AllocBindSubst(itemCount, itemSize, calc_only);
	if(pos)
		if(calc_only) {
			pBind->SubstSize = pos;
			ok = -1;
		}
		else {
			pBind->Flags |= Bind::fSubst;
			pBind->SubstSize = itemCount * itemSize;
			pBind->SubstOffs = pos;
			pBind->ItemSize = itemSize;
			pBind->Dim = itemCount;
			ok = 1;
		}
	return ok;
}

void * FASTCALL SSqlStmt::GetBindOuterPtr(const Bind * pBind, uint rowN) const
{
	assert(rowN < pBind->Dim);
	return pBind->SubstOffs ? (BS.P_Buf + pBind->SubstOffs + rowN * pBind->ItemSize) : pBind->P_Data;
}

void * FASTCALL SSqlStmt::GetIndPtr(const Bind * pBind, uint rowN) const
{
	if(pBind && pBind->IndPos) {
		return &(PTR16(BS.P_Buf + pBind->IndPos)[rowN]);
	}
	else
		return 0;
}


size_t FASTCALL SSqlStmt::GetBindOuterSize(const Bind * pBind) const
{
	return pBind->SubstOffs ? pBind->SubstSize : stsize(pBind->Typ);
}

int SSqlStmt::Exec(uint count, int mode)
{
	return IsValid() ? P_Db->Exec(*this, count, mode) : 0;
}

int SSqlStmt::Describe()
{
	return P_Db->Describe(*this, Descr);
}

int SSqlStmt::InitBinding()
{
	BL.freeAll();
	BL.Dim = 1;
	BS.Destroy();
	BS.Alloc(1024*16);   // @todo Указатель должен быть неперемещаемым. В дальнейшем надо
		// придумать более умную схему распределения памяти под binding
	TopBindSubst = 4; // Нулевое смещение резервируется как неиспользование буфера
	IndSubstPlus = 0;
	IndSubstMinus = 0;
	FslSubst = 0;
	return 1;
}

uint SSqlStmt::GetBindingCount(int dir) const
{
	uint   cnt = 0;
	for(uint i = 0; i < BL.getCount(); i++) {
		const Bind & r_bind = BL.at(i);
		if(dir > 0 && r_bind.Pos > 0)
			++cnt;
		else if(dir < 0 && r_bind.Pos < 0)
			++cnt;
	}
	return cnt;
}

int SSqlStmt::BindItem(int pos, uint count, TYPEID typ, void * pDataBuf)
{
	int    ok = 1;
	BL.Dim = count;
	assert(count > 0 && count <= 1024);
	Bind b;
	b.Pos = pos;
	b.Typ = typ;
	b.P_Data = pDataBuf;
	uint   lp = 0;
	if(BL.lsearch(&b.Pos, &lp, PTR_CMPFUNC(int16)))
		BL.atFree(lp);
	BL.insert(&b);
	return ok;
}

int SSqlStmt::BindRowId(int pos, uint count, DBRowId * pDataBuf)
{
	return BindItem(pos, count, T_ROWID, pDataBuf);
}

int SSqlStmt::BindData(int dir, uint count, const BNFieldList & rFldList, const void * pDataBuf, DBLobBlock * pLob)
{
	int    ok = 1;
	const  uint fld_count = rFldList.getCount();
	BL.Dim = count;
	BL.P_Lob = pLob;
	assert(count > 0 && count <= 1024);
	for(uint i = 0; i < fld_count; i++) {
		const BNField & r_fld = rFldList.getField(i);
		BindItem((dir < 0) ? -(int16)(i+1) : +(int16)(i+1), count, r_fld.T, PTR8(pDataBuf) + r_fld.Offs);
	}
	THROW(P_Db->Binding(*this, dir));
	CATCHZOK
	return ok;
}

int SSqlStmt::BindData(int dir, uint count, const DBFieldList & rFldList, const void * pDataBuf, DBLobBlock * pLob)
{
	//
	// Принципиальное отличие этой функции от SSqlStmt::BindData(int, uint, const BNFieldList &, const void *)
	// в том, что здесь смещение рассчитывается, а не извлекается из BNField
	//
	int    ok = 1;
	const  uint fld_count = rFldList.GetCount();
	size_t offs = 0;
	BL.Dim = count;
	BL.P_Lob = pLob;
	assert(count > 0 && count <= 1024);
	for(uint i = 0; i < fld_count; i++) {
		const BNField & r_fld = rFldList.GetField(i);
		BindItem((dir < 0) ? -(int16)(i+1) : +(int16)(i+1), count, r_fld.T, PTR8(pDataBuf) + offs);
		offs += r_fld.size();
	}
	THROW(P_Db->Binding(*this, dir));
	CATCHZOK
	return ok;
}

int SSqlStmt::BindKey(const BNKeyList & rKeyList, int idxN, const void * pDataBuf)
{
	int    ok = 1;
	const  uint fld_count = rKeyList.getKey(idxN).getNumSeg();
	BL.Dim = 1;
	for(uint i = 0; i < fld_count; i++) {
		const BNField & r_fld = rKeyList.field(idxN, i);
		Bind b;
		b.Pos = -(int16)(i+1);
		b.Typ = r_fld.T;
		b.P_Data = PTR8(pDataBuf) + r_fld.Offs;
		uint   lp = 0;
		if(BL.lsearch(&b.Pos, &lp, PTR_CMPFUNC(int16)))
			BL.atFree(lp);
		BL.insert(&b);
	}
	THROW(P_Db->Binding(*this, -1));
	CATCHZOK
	return ok;
}

int SSqlStmt::SetData(uint recNo)
{
	int    ok = 1;
	for(uint i = 0; i < BL.getCount(); i++) {
		SSqlStmt::Bind & r_bind = BL.at(i);
		if(r_bind.Pos < 0)
			THROW(P_Db->ProcessBinding(-1, recNo, this, &r_bind));
	}
	CATCHZOK
	return ok;
}

int SSqlStmt::SetDataDML(uint recNo)
{
	int    ok = 1;
	for(uint i = 0; i < BL.getCount(); i++) {
		SSqlStmt::Bind & r_bind = BL.at(i);
		if(r_bind.Pos < 0)
			THROW(P_Db->ProcessBinding(-2, recNo, this, &r_bind));
	}
	CATCHZOK
	return ok;
}

int SSqlStmt::GetOutData(uint recNo)
{
	int    ok = 1;
	for(uint i = 0; i < BL.getCount(); i++) {
		SSqlStmt::Bind & r_bind = BL.at(i);
		if(r_bind.Pos < 0) {
			THROW(P_Db->ProcessBinding(+1, recNo, this, &r_bind));
		}
	}
	CATCHZOK
	return ok;
}

int SSqlStmt::GetData(uint recNo)
{
	int    ok = 1;
	for(uint i = 0; i < BL.getCount(); i++) {
		SSqlStmt::Bind & r_bind = BL.at(i);
		if(r_bind.Pos > 0) {
			THROW(P_Db->ProcessBinding(+1, recNo, this, &r_bind));
		}
	}
	CATCHZOK
	return ok;
}

int SSqlStmt::Fetch(uint count, uint * pActualCount)
{
	int    ok = 1, r;
	THROW(r = P_Db->Fetch(*this, count, pActualCount));
	if(r > 0) {
		THROW(GetData(0));
		ok = 1;
	}
	else {
		BtrError = BE_EOF;
		ok = -1;
	}
	CATCHZOK
	return ok;
}
//
//
//
int SOraDbProvider::CreateStmt(SSqlStmt * pS, const char * pText, long flags)
{
	int    ok = 1;
	OH h = OhAlloc(OCI_HTYPE_STMT);
	pS->H = h;
	THROW(ProcessError(OCIStmtPrepare(h, Err, (const OraText *)pText, strlen(pText), OCI_NTV_SYNTAX, OCI_DEFAULT)));
	CATCHZOK
	return ok;
}

int SOraDbProvider::DestroyStmt(SSqlStmt * pS)
{
	int    ok = 1;
	const  uint dim = pS->BL.Dim;
	for(uint i = 0; i < pS->BL.getCount(); i++) {
		if(!ProcessBinding(1000, dim, pS, &pS->BL.at(i)))
			ok = 0;
	}
	OH h;
	h.H = (void *)pS->H;
	h.T = OCI_HTYPE_STMT;
	OhFree(h);
	pS->H = 0;
	return ok;
}

//static char hexdig[] = "0123456789ABCDEF";

uint16 FASTCALL byte2hex(uint8 b)
{
	union H {
		uint8  c[2];
		uint16 w;
	} h;
	h.c[0] = b / 16;
	h.c[1] = b % 16;
	return h.w;
}

uint8 FASTCALL hex2byte(uint16 w)
{
	union H {
		uint8  c[2];
		uint16 w;
	} h;
	h.w = w;
	return ((h.c[0] * 16) + h.c[1]);
}

int SOraDbProvider::ProcessBinding_SimpleType(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType)
{
	if(action == 0) {
		pBind->NtvTyp = ntvType;
		if(count > 1)
			pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
	}
	else if(pBind->Dim > 1) {
		if(action < 0)
			memcpy(pStmt->GetBindOuterPtr(pBind, count), pBind->P_Data, pBind->NtvSize);
		else if(action == 1)
			memcpy(pBind->P_Data, pStmt->GetBindOuterPtr(pBind, count), pBind->NtvSize);
	}
	return 1;
}

int SOraDbProvider::ProcessBinding_AllocDescr(uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType, int descrType)
{
	int    ok = 1;
	pBind->NtvTyp = ntvType;
	pBind->NtvSize = sizeof(void *);
	if(pStmt->AllocBindSubst(count, sizeof(OD), pBind) > 0) {
		OD d;
		for(uint i = 0; i < count; i++) {
			d = OdAlloc(descrType);
			memcpy(pStmt->GetBindOuterPtr(pBind, i), &d, sizeof(d));
		}
	}
	else
		ok = 0;
	return ok;
}

int SOraDbProvider::ProcessBinding_FreeDescr(uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind)
{
	for(uint i = 0; i < count; i++)
		OdFree(*(OD *)pStmt->GetBindOuterPtr(pBind, i));
	return 1;
}
//
// ARG(action IN):
//   0 - инициализация структуры SSqlStmt::Bind
//   1 - извлечение данных из внешнего источника во внутренние буферы
//  -1 - перенос данных из внутренних буферов во внещний источник
//  1000 - разрушение специфичных для SQL-сервера элементов структуры SSqlStmt::Bind
//
int SOraDbProvider::ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind)
{
	int    ok = 1;
	size_t sz = stsize(pBind->Typ);
	uint16 out_typ = 0;
	pBind->NtvSize = (uint16)sz;
	if(action == 0)
		pBind->Dim = count;
	const int t = GETSTYPE(pBind->Typ);
	switch(t) {
		case S_CHAR:
			ProcessBinding_SimpleType(action, count, pStmt, pBind, SQLT_CHR);
			break;
		case S_INT:
		case S_AUTOINC:
			ProcessBinding_SimpleType(action, count, pStmt, pBind, SQLT_INT);
			break;
		case S_UINT:
			ProcessBinding_SimpleType(action, count, pStmt, pBind, SQLT_UIN);
			break;
		case S_FLOAT:
			ProcessBinding_SimpleType(action, count, pStmt, pBind, SQLT_FLT);
			break;
		case S_DATE:
			if(action == 0) {
				pBind->NtvTyp = SQLT_ODT;
				pBind->NtvSize = sizeof(OCIDate);
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				OCIDate * p_ocidt = (OCIDate *)pStmt->GetBindOuterPtr(pBind, count);
				LDATE * p_dt = (LDATE *)pBind->P_Data;
				memzero(p_ocidt, sizeof(*p_ocidt));
				p_ocidt->Y = p_dt->year();
				p_ocidt->M = p_dt->month();
				p_ocidt->D = p_dt->day();
			}
			else if(action == 1) {
				OCIDate * p_ocidt = (OCIDate *)pStmt->GetBindOuterPtr(pBind, count);
				*(LDATE *)pBind->P_Data = encodedate(p_ocidt->D, p_ocidt->M, p_ocidt->Y);
			}
			break;
		case S_TIME:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, SQLT_TIMESTAMP, OCI_DTYPE_TIMESTAMP);
			else if(action < 0)
				SetTime(*(OD *)pStmt->GetBindOuterPtr(pBind, count), *(LTIME *)pBind->P_Data);
			else if(action == 1)
				*(LTIME *)pBind->P_Data = GetTime(*(OD *)pStmt->GetBindOuterPtr(pBind, count));
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_DATETIME:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, SQLT_TIMESTAMP, OCI_DTYPE_TIMESTAMP);
			else if(action < 0)
				SetDateTime(*(OD *)pStmt->GetBindOuterPtr(pBind, count), *(LDATETIME *)pBind->P_Data);
			else if(action == 1)
				*(LDATETIME *)pBind->P_Data = GetDateTime(*(OD *)pStmt->GetBindOuterPtr(pBind, count));
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_BLOB:
		case S_CLOB:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, (t == S_BLOB) ? SQLT_BLOB : SQLT_CLOB, OCI_DTYPE_LOB);
			else if(action < 0) {
				OD ocilob = *(OD *)pStmt->GetBindOuterPtr(pBind, count);
				DBLobBlock * p_lob = pStmt->GetBindingLob();
				size_t lob_sz = 0;
				uint32 lob_loc = 0;
				if(p_lob) {
					p_lob->GetSize(labs(pBind->Pos)-1, &lob_sz);
					p_lob->GetLocator(labs(pBind->Pos)-1, &lob_loc);
					SETIFZ(lob_loc, (uint32)OdAlloc(OCI_DTYPE_LOB).H);
					ProcessError(OCILobAssign(Env, Err, (const OCILobLocator *)lob_loc, (OCILobLocator **)&ocilob.H));
				}
				LobWrite(ocilob, pBind->Typ, (SLob *)pBind->P_Data, lob_sz);
			}
			else if(action == 1) {
				OD ocilob = *(OD *)pStmt->GetBindOuterPtr(pBind, count);
				DBLobBlock * p_lob = pStmt->GetBindingLob();
				size_t lob_sz = 0;
				uint32 lob_loc = 0;
				LobRead(ocilob, pBind->Typ, (SLob *)pBind->P_Data, &lob_sz);
				if(p_lob) {
					SETIFZ(lob_loc, (uint32)OdAlloc(OCI_DTYPE_LOB).H);
					ProcessError(OCILobAssign(Env, Err, ocilob, (OCILobLocator **)&lob_loc));
					p_lob->SetSize(labs(pBind->Pos)-1, lob_sz);
					p_lob->SetLocator(labs(pBind->Pos)-1, lob_loc);
				}
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		case S_DEC:
		case S_MONEY:
			if(action == 0) {
				pBind->NtvTyp = SQLT_FLT;
				pBind->NtvSize = sizeof(double);
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else {
				const int16 dec_len = (int16)GETSSIZED(pBind->Typ);
				const int16 dec_prc = (int16)GETSPRECD(pBind->Typ);
				if(action < 0)
					*(double *)pStmt->GetBindOuterPtr(pBind, count) = dectobin((char *)pBind->P_Data, dec_len, dec_prc);
				else if(action == 1)
					dectodec(*(double *)pStmt->GetBindOuterPtr(pBind, count), (char *)pBind->P_Data, dec_len, dec_prc);
			}
			break;
		case S_NOTE:
		case S_ZSTRING:
			if(action == 0) {
				pBind->NtvTyp = SQLT_AVC;
				pBind->NtvSize = (uint16)sz;
				pStmt->AllocBindSubst(count, pBind->NtvSize, pBind);
			}
			else if(action < 0) {
				char * p_outer = (char *)pStmt->GetBindOuterPtr(pBind, count);
				//
				// 1. Необходимо защититься от ситуации, когда в конце буфера отсутствует '\0'
				// 2. Необходимо конвертировать OEM кодировку (используется в btrieve данных и
				//    в проекте в целом) в CHAR кодировку, которая используется для хранения строк
				//    в SQL-базах.
				// 3. Пустая строка для критериев запроса извлечения данных должна быть представлена единственным пробелом.
				//
				if(PTR8(pBind->P_Data)[0] == 0) {
					if(action == -1) {
						p_outer[0] = ' ';
						p_outer[1] = 0;
					}
					else
						p_outer[0] = 0;
				}
				else {
					//
					// Особый случай: все элементы заполнены символами 255.
					// Это - максимальное значение, используемое в сравнениях.
					// Его не следует преобразовывать функцией SOemToChar
					//
					int    is_max = 0;
					if(PTR8(pBind->P_Data)[0] == 255) {
						is_max = 1;
						for(uint k = 1; k < (sz-1); k++)
							if(PTR8(pBind->P_Data)[k] != 255) {
								is_max = 0;
								break;
							}
					}
					strnzcpy(p_outer, (char *)pBind->P_Data, sz);
					if(!is_max)
						SOemToChar(p_outer);
				}
				/*
				const size_t len = strlen(p_outer);
				if(len < sz-1) {
					memset(p_outer + len, ' ', sz - len);
					p_outer[sz-1] = 0;
				}
				*/
			}
			else if(action == 1) {
				int16 * p_ind = (int16 *)pStmt->GetIndPtr(pBind, count);
				if(p_ind && *p_ind == -1) {
					*(char *)pBind->P_Data = 0;
				}
				else {
					CharToOem((char *)pStmt->GetBindOuterPtr(pBind, count), (char *)pBind->P_Data); // @unicodeproblem 
					trimright((char *)pBind->P_Data);
				}
			}
			break;
		case S_RAW:
			if(action == 0) {
				pBind->NtvTyp = SQLT_BIN;
				pBind->NtvSize = (uint16)sz;
				pStmt->AllocBindSubst(count, (sz * 2), pBind);
			}
			else if(action < 0) {
				uint16 * p_outer = (uint16 *)pStmt->GetBindOuterPtr(pBind, count);
				memcpy(p_outer, pBind->P_Data, sz);
				/*
				for(uint i = 0; i < sz; i++)
					p_outer[i] = byte2hex(PTR8(pBind->P_Data)[i]);
				*/
			}
			else if(action == 1) {
				uint16 * p_outer = (uint16 *)pStmt->GetBindOuterPtr(pBind, count);
				memcpy(pBind->P_Data, p_outer, sz);
				/*
				for(uint i = 0; i < sz; i++)
					PTR8(pBind->P_Data)[i] = hex2byte(p_outer[i]);
				*/
			}
			break;
		case S_ROWID:
			if(action == 0)
				ProcessBinding_AllocDescr(count, pStmt, pBind, SQLT_RDD, OCI_DTYPE_ROWID);
			else if(action < 0) {
				//assert(0);
			}
			else if(action == 1) {
				OD ocirid = *(OD *)pStmt->GetBindOuterPtr(pBind, count);
				uint16 len = sizeof(DBRowId);
				OCIRowidToChar(ocirid, (OraText *)pBind->P_Data, &len, Err);
			}
			else if(action == 1000)
				ProcessBinding_FreeDescr(count, pStmt, pBind);
			break;
		//case S_LSTRING:
		//case S_WCHAR:
		//case S_LOGICAL:
		//case S_NUMERIC:
	}
	//
	// Распределяем пространство для переменных индикаторов и FSL.
	// При сигнале SSqlStmt::Bind::fCalcOnly ничего не делаем
	// поскольку функция SSqlStmt::SetupBindingSubstBuffer должна была
	// предусмотреть необходимость в пространстве для специальных значений.
	//
	if(action == 0 && !(pBind->Flags & SSqlStmt::Bind::fCalcOnly)) {
		pStmt->AllocIndSubst(count, pBind);
		if(pBind->Pos > 0)
			pStmt->AllocFslSubst(count, pBind);
	}
	return ok;
}

int SOraDbProvider::Binding(SSqlStmt & rS, int dir)
{
	int    ok = 1;
	const  uint row_count = rS.BL.Dim;
	const  uint col_count = rS.BL.getCount();
	OH     h_stmt = StmtHandle(rS);
	assert(row_count > 0 && row_count <= 1024);
	THROW(rS.SetupBindingSubstBuffer(dir, row_count));
	if(dir == -1) {
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos < 0) {
				OCIBind * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					uint16 * p_ind = r_bind.IndPos ? (uint16 *)(rS.BS.P_Buf + r_bind.IndPos) : 0;
					THROW(ProcessError(OCIBindByPos(h_stmt, &p_bd, Err, -r_bind.Pos,
						p_data, r_bind.NtvSize, r_bind.NtvTyp,
						p_ind, 0/*alenp*/, 0/*rcodep*/, 0/*maxarr_len*/, 0/*curelep*/, OCI_DEFAULT)));
					r_bind.H = (uint32)p_bd;
				}
				if(row_count > 1) {
					THROW(ProcessError(OCIBindArrayOfStruct(p_bd, Err, r_bind.ItemSize, sizeof(uint16), 0, 0)));
				}
			}
		}
	}
	else if(dir == +1) {
		for(uint i = 0; i < col_count; i++) {
			SSqlStmt::Bind & r_bind = rS.BL.at(i);
			if(r_bind.Pos > 0) {
				OCIDefine * p_bd = 0;
				THROW(ProcessBinding(0, row_count, &rS, &r_bind));
				{
					void * p_data = rS.GetBindOuterPtr(&r_bind, 0);
					uint16 * p_ind = r_bind.IndPos ? (uint16 *)(rS.BS.P_Buf + r_bind.IndPos) : 0;
					uint16 * p_fsl = r_bind.FslPos ? (uint16 *)(rS.BS.P_Buf + r_bind.FslPos) : 0;
					THROW(ProcessError(OCIDefineByPos(h_stmt, &p_bd, Err, r_bind.Pos,
						p_data, r_bind.NtvSize, r_bind.NtvTyp,
						p_ind, p_fsl, 0, OCI_DEFAULT)));
					r_bind.H = (uint32)p_bd;
				}
				if(row_count > 1) {
					THROW(ProcessError(OCIDefineArrayOfStruct(p_bd, Err, r_bind.ItemSize, sizeof(uint16), sizeof(uint16), 0)));
				}
			}
		}
	}
	CATCH
		ok = 0;
		DEBUG_LOG(LastErrMsg);
	ENDCATCH
	return ok;
}

int SOraDbProvider::Exec(SSqlStmt & rS, uint count, int mode)
{
	int    ok = ProcessError(OCIStmtExecute(Srvc, StmtHandle(rS), Err, count, 0, 0, 0, NZOR(mode, OCI_DEFAULT)));
#ifndef NDEBUG // {
	if(!ok) {
		SString log_buf;
		log_buf.Cat("EXEC").CatDiv(':', 2).Cat(ok ? "OK" : LastErrMsg);
		DEBUG_LOG(log_buf);
	}
#endif // } !NDEBUG
	return ok;
}

int SOraDbProvider::Describe(SSqlStmt & rS, SdRecord & rRec)
{
	int    ok = 1;
	uint32 argcount;
	rRec.Clear();
	SdbField fld;
	THROW(OhAttrGet(StmtHandle(rS), OCI_ATTR_PARAM_COUNT, &argcount, 0));
	for(uint32 i = 1; i < argcount; i++) {
		char * p_name = 0;
		uint32 name_len;
		uint16 type, width;
		uint16 prec;
		int8   scale;
		OCIParam * pd;
		THROW(ProcessError(OCIParamGet(StmtHandle(rS), OCI_HTYPE_STMT, Err, (void **)&pd, i)));
		OCIAttrGet(pd, OCI_DTYPE_PARAM, &type,  0,  OCI_ATTR_DATA_TYPE, Err);
		OCIAttrGet(pd, OCI_DTYPE_PARAM, &width, 0,  OCI_ATTR_DATA_SIZE, Err);
		OCIAttrGet(pd, OCI_DTYPE_PARAM, &p_name, &name_len, OCI_ATTR_DISP_NAME /*OCI_ATTR_NAME*/, Err);
		OCIAttrGet(pd, OCI_DTYPE_PARAM, &prec,  0,  OCI_ATTR_PRECISION, Err);
		OCIAttrGet(pd, OCI_DTYPE_PARAM, &scale, 0,  OCI_ATTR_SCALE, Err);
		{
			fld.Init();
			fld.Name = p_name;
			TYPEID t = 0;
			switch(type) {
				case SQLT_INT: t = MKSTYPE(S_INT, 4); break;
				case SQLT_UIN: t = MKSTYPE(S_UINT, 4); break;
				case SQLT_NUM:
					if(scale <= 0)
						t = MKSTYPE(S_INT, 4);
					else
						t = MKSTYPE(S_FLOAT, 8);
					break;
				case SQLT_FLT: t = MKSTYPE(S_FLOAT, 8); break;
				case SQLT_STR:
				case SQLT_AVC: t = MKSTYPE(S_ZSTRING, width); break;
				case SQLT_CHR: t = MKSTYPE(S_ZSTRING, width+1); break;
				case SQLT_BIN: t = MKSTYPE(S_RAW, width); break;
				case SQLT_DAT:
				case SQLT_DATE: t = MKSTYPE(S_DATE, 4); break;
				case SQLT_TIME: t = MKSTYPE(S_TIME, 4); break;
				case SQLT_TIMESTAMP: t = MKSTYPE(S_DATETIME, 8); break;
				case SQLT_RID: // rowid
				case SQLT_RDD: // rowid descriptor
					t = MKSTYPE(S_ZSTRING, 32);
					break;
				case SQLT_BLOB: t = MKSTYPE(S_BLOB, 0); break;
				case SQLT_CLOB: t = MKSTYPE(S_CLOB, 0); break;
				default:
					//ii.type = STRING_V;
					//AddColumn(SQLT_STR, ii.width ? ii.width + 1 : longsize);
					break;
			}
			if(t) {
				fld.T.Typ = t;
				uint   fld_id = i;
				THROW(rRec.AddField(&fld_id, &fld));
			}
			else {
				; // @error
			}
		}
	}
	CATCHZOK
	return ok;
}

int SOraDbProvider::StartTransaction()
{
	int    ok = 1;
	OH     h_t = OhAlloc(OCI_HTYPE_TRANS);
	THROW(OhAttrSet(Srvc, OCI_ATTR_TRANS, h_t, 0));
	{
		XID    tx_id;
		char   temp_buf[64];
		tx_id.FormatID = 7100;
		ulong rid = SLS.GetTLA().Rg.Get();
		ultoa(rid, temp_buf, 10);
		tx_id.GtrIdLen = strlen(temp_buf);
		long   i = 0;
		while(i < tx_id.GtrIdLen)
			tx_id.Data[i++] = temp_buf[i]-'0';
		tx_id.BQualLen = 1;
		tx_id.Data[i++] = 1;
		THROW(OhAttrSet(h_t, OCI_ATTR_XID, &tx_id, sizeof(tx_id)));
	}
	THROW(ProcessError(OCITransStart(Srvc, Err, 15, 0)));
	CATCHZOK
	return ok;
}

int SOraDbProvider::CommitWork()
{
	return ProcessError(OCITransCommit(Srvc, Err, OCI_DEFAULT));
}

int SOraDbProvider::RollbackWork()
{
	return ProcessError(OCITransRollback(Srvc, Err, OCI_DEFAULT));
}

int SOraDbProvider::Fetch(SSqlStmt & rS, uint count, uint * pActualCount)
{
	int    ok = 0;
	uint   actual = 0;
	if(rS.Flags & SSqlStmt::fNoMoreData)
		ok = -1;
	else {
		int    err = OCIStmtFetch2(StmtHandle(rS), Err, count, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
		if(oneof2(err, OCI_SUCCESS, OCI_SUCCESS_WITH_INFO)) {
			actual = count;
			rS.Flags &= ~SSqlStmt::fNoMoreData;
			ok = 1;
		}
		else if(err == OCI_NO_DATA) {
			OhAttrGet(StmtHandle(rS), OCI_ATTR_ROWS_FETCHED, (void *)&actual, 0);
			rS.Flags |= SSqlStmt::fNoMoreData;
			ok = actual ? 1 : -1;
		}
		else {
			ok = ProcessError(err);
		}
	}
	ASSIGN_PTR(pActualCount, actual);
	return ok;
}

int SLAPI SOraDbProvider::PostProcessAfterUndump(DBTable * pTbl)
{
	int    ok = -1;
	if(pTbl->State & DBTable::sHasAutoinc) {
		const  uint fld_count = pTbl->fields.getCount();
		SString seq_name;
		for(uint i = 0; i < fld_count; i++) {
			const BNField & r_fld = pTbl->fields.getField(i);
			if(GETSTYPE(r_fld.T) == S_AUTOINC) {
				long   seq = 0;
				long   max_val = 0;
				uint   actual = 0;
				Generator_SQL sg(sqlstORA, 0);
				sg.GetSequenceNameOnField(*pTbl, i, seq_name);
				{
					sg.Reset().Tok(Generator_SQL::tokSelect).Sp().Text(seq_name).Dot().Text("nextval").Sp().From("DUAL");
					SSqlStmt stmt(this, (const SString &)sg);
					THROW(stmt.Exec(0, OCI_DEFAULT));
					THROW(stmt.BindItem(+1, 1, T_INT32, &seq));
					THROW(Binding(stmt, +1));
					if(Fetch(stmt, 1, &actual) && actual)
						THROW(stmt.GetData(0));
				}
				{
					// select max(ID) from tbl
					sg.Reset().Tok(Generator_SQL::tokSelect).Sp().
						Func(Generator_SQL::tokMax, r_fld.Name).Sp().From(pTbl->fileName);
					SSqlStmt stmt(this, (const SString &)sg);
					THROW(stmt.Exec(0, OCI_DEFAULT));
					THROW(stmt.BindItem(+1, 1, T_INT32, &max_val));
					THROW(Binding(stmt, +1));
					if(Fetch(stmt, 1, &actual) && actual)
						THROW(stmt.GetData(0));
				}
				if(max_val > seq) {
					{
						sg.Reset().Tok(Generator_SQL::tokDrop).Sp().Tok(Generator_SQL::tokSequence).Sp().Text(seq_name);
						SSqlStmt stmt(this, (const SString &)sg);
						THROW(stmt.Exec(1, OCI_DEFAULT));
					}
					//
					//
					//
					{
						sg.Reset().CreateSequenceOnField(*pTbl, pTbl->fileName, i, max_val+1);
						SSqlStmt stmt(this, (const SString &)sg);
						THROW(stmt.Exec(1, OCI_DEFAULT));
					}
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SOraDbProvider::GetAutolongVal(const DBTable & rTbl, uint fldN, long * pVal)
{
	int    ok = 1;
	long   val = 0;
	uint   actual = 0;
	SString seq_name;
	Generator_SQL sg(sqlstORA, 0); // Так как эта функция используется внутри других вызовов SOraDbProvider,
		// здесь мы создадим локальный Generator_SQL (не будем пользоваться SqlGen).
	// SELECT secXXX.nextval FROM DUAL;
	sg.GetSequenceNameOnField(rTbl, fldN, seq_name);
	seq_name.Dot().Cat("nextval");
	sg.Reset().Tok(Generator_SQL::tokSelect).Sp().Text(seq_name).Sp().From("DUAL");
	SSqlStmt stmt(this, (const SString &)sg);
	THROW(stmt.Exec(0, OCI_DEFAULT));
	THROW(stmt.BindItem(+1, 1, T_INT32, &val));
	THROW(Binding(stmt, +1));
	if(Fetch(stmt, 1, &actual) && actual)
		THROW(stmt.GetData(0));
	CATCHZOK
	ASSIGN_PTR(pVal, val);
	return ok;
}

int SOraDbProvider::GetDirect(DBTable & rTbl, const DBRowId & rPos, int forUpdate)
{
	// select * from table where rowid = 'xxx'
	int    ok = 1;
	SString temp_buf;
	THROW(rPos.IsLong());
	rPos.ToStr(temp_buf);
	SqlGen.Reset().Tok(Generator_SQL::tokSelect).Sp().Aster().Sp().From(rTbl.fileName).Sp().
		Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
	if(forUpdate)
		SqlGen.Tok(Generator_SQL::tokFor).Sp().Tok(Generator_SQL::tokUpdate);
	{
		int    r;
		uint   actual = 0;
		SSqlStmt stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(0, OCI_DEFAULT));
		THROW(stmt.BindData(+1, 1, rTbl.fields, rTbl.getDataBuf(), rTbl.getLobBlock()));
		THROW(r = Fetch(stmt, 1, &actual));
		if(r > 0) {
			THROW(stmt.GetData(0));
			ok = 1;
		}
		else {
			BtrError = BE_EOF;
			ok = -1;
		}
	}
	CATCHZOK
	return ok;
}

SOraDbProvider::SOraDbProvider(const char * pDataPath) :
	DbProvider(DbDictionary::CreateInstance(0, 0), DbProvider::cSQL|DbProvider::cDbDependTa), SqlGen(sqlstORA, 0)
{
	DataPath = pDataPath;
	DBS.GetDbPathID(DataPath, &DbPathID);
	Ocif::Load();
	Flags = 0;
	Env.T = OCI_HTYPE_ENV;
	OCIEnvCreate(&Env.Env, OCI_OBJECT, 0, 0, 0, 0, 0, 0);
	Err = OhAlloc(OCI_HTYPE_ERROR);
	Svr = OhAlloc(OCI_HTYPE_SERVER);
	{
		CrsfcNameList.Clear();
		CrsfcNameList.Add(/*csrfpa*/ 2, "OSQL");
		CrsfcNameList.Add(/*csrfex*/ 4, "OEXEC");
		CrsfcNameList.Add(/*csrfbi*/ 6, "OBIND");
		CrsfcNameList.Add(/*csrfdb*/ 8, "ODFINN");
		CrsfcNameList.Add(/*csrfdi*/10, "ODSRBN");
		CrsfcNameList.Add(/*csrffe*/12, "OFETCH");
		CrsfcNameList.Add(/*csrfop*/14, "OOPEN");
		CrsfcNameList.Add(/*csrfcl*/16, "OCLOSE");
		CrsfcNameList.Add(/*csrfds*/22, "ODSC");
		CrsfcNameList.Add(/*csrfnm*/24, "ONAME");
		CrsfcNameList.Add(/*csrfp3*/26, "OSQL3");
		CrsfcNameList.Add(/*csrfbr*/28, "OBNDRV");
		CrsfcNameList.Add(/*csrfbx*/30, "OBNDRN");
		CrsfcNameList.Add(/*csrfso*/34, "OOPT");
		CrsfcNameList.Add(/*csrfre*/36, "ORESUM");
		CrsfcNameList.Add(/*csrfbn*/50, "OBINDN");
		CrsfcNameList.Add(/*csrfca*/52, "OCANCEL");
		CrsfcNameList.Add(/*csrfsd*/54, "OSQLD");
		CrsfcNameList.Add(/*csrfef*/56, "OEXFEN");
		CrsfcNameList.Add(/*csrfln*/58, "OFLNG");
		CrsfcNameList.Add(/*csrfdp*/60, "ODSCSP");
		CrsfcNameList.Add(/*csrfba*/62, "OBNDRA");
		CrsfcNameList.Add(/*csrfbp*/63, "OBINDPS");
		CrsfcNameList.Add(/*csrfdp*/64, "ODEFINPS");
		CrsfcNameList.Add(/*csrfgp*/65, "OGETPI");
		CrsfcNameList.Add(/*csrfsp*/66, "OSETPI");
	}
}

SOraDbProvider::~SOraDbProvider()
{
	OhFree(Svr);
	OhFree(Err);
	// OCITerminate(OCI_DEFAULT);
}

void SOraDbProvider::Helper_SetErr(int errCode, const char * pErrStr)
{
	LastErrCode = errCode;
	LastErrMsg = pErrStr;
}

int FASTCALL SOraDbProvider::ProcessError(int status)
{
	int    ok = 0;
	switch(status) {
		case OCI_SUCCESS: ok = 1; break;
		case OCI_SUCCESS_WITH_INFO: Helper_SetErr(status, "OCI_SUCCESS_WITH_INFO"); break;
		case OCI_NEED_DATA: Helper_SetErr(status, "OCI_NEED_DATA"); break;
		case OCI_NO_DATA: Helper_SetErr(status, "OCI_NO_DATA"); break;
		case OCI_INVALID_HANDLE: Helper_SetErr(status, "OCI_INVALID_HANDLE"); break;
		case OCI_STILL_EXECUTING: Helper_SetErr(status, "OCI_STILL_EXECUTE"); break;
		case OCI_CONTINUE: Helper_SetErr(status, "OCI_CONTINUE"); break;
		case OCI_ERROR:
			{
				OraText msg_buf[1024];
				OCIErrorGet(Err, 1, 0, &LastErrCode, msg_buf, sizeof(msg_buf), OCI_HTYPE_ERROR);
				LastErrMsg = (char *)msg_buf;
				DBS.SetError(BE_ORA_TEXT, SCharToOem((char *)msg_buf));
			}
			break;
		default:
			ok = 1;
			break;
	}
	return ok;
}

SOraDbProvider::OH FASTCALL SOraDbProvider::OhAlloc(int type)
{
	void * p_h = 0;
	OH     o;
	if(ProcessError(OCIHandleAlloc(Env, &p_h, type, 0, 0))) {
		o.H = p_h;
		o.T = type;
	}
	return o;
}

int FASTCALL SOraDbProvider::OhFree(OH & rO)
{
	if(!rO || !ProcessError(OCIHandleFree(rO.H, rO.T)))
		return 0;
	else {
		rO.H = 0;
		rO.T = 0;
		return 1;
	}
}

SOraDbProvider::OD FASTCALL SOraDbProvider::OdAlloc(int type)
{
	void * p_h = 0;
	OD     o;
	if(ProcessError(OCIDescriptorAlloc(Env, &p_h, type, 0, 0))) {
		o.H = p_h;
		o.T = type;
	}
	return o;
}

int FASTCALL SOraDbProvider::OdFree(OD & rO)
{
	if(!rO || !ProcessError(OCIDescriptorFree(rO.H, rO.T)))
		return 0;
	else {
		rO.H = 0;
		rO.T = 0;
		return 1;
	}
}

int SOraDbProvider::OhAttrSet(OH o, uint attr, void * pData, size_t size)
{
	return ProcessError(OCIAttrSet(o.H, o.T, pData, size, attr, Err));
}

int SOraDbProvider::OdAttrSet(OD o, uint attr, void * pData, size_t size)
{
	return ProcessError(OCIAttrSet(o.H, o.T, pData, size, attr, Err));
}

int SOraDbProvider::OhAttrGet(OH o, uint attr, void * pData, size_t * pSize)
{
	return ProcessError(OCIAttrGet(o.H, o.T, pData, (uint32 *)pSize, attr, Err));
}

int SOraDbProvider::OhAttrGet(OH o, uint attr, SString & rBuf)
{
	char   temp_buf[1024];
	size_t sz = 1023;
	temp_buf[0] = 0;
	if(OhAttrGet(o, attr, temp_buf, &sz)) {
		rBuf = temp_buf;
		return 1;
	}
	else {
		rBuf.Z();
		return 0;
	}
}

LDATE FASTCALL SOraDbProvider::GetDate(OCIDateTime * pOciDtm)
{
	int16  y;
	uint8  m, d;
	return ProcessError(OCIDateTimeGetDate(Env, Err, pOciDtm, &y, &m, &d)) ? encodedate(d, m, y) : ZERODATE;
}
//
// Descr: Величина, на которую надо умножить сотые доли секунды, чтобы получить OCI fractional second value
//
#define OCI_TIME_FRACT 10000000

LTIME FASTCALL SOraDbProvider::GetTime(OCIDateTime * pOciDtm)
{
	uint8  h, m, s;
	uint32 fs;
	if(ProcessError(OCIDateTimeGetTime(Env, Err, pOciDtm, &h, &m, &s, &fs))) {
		return encodetime(h, m, s, fs / OCI_TIME_FRACT);
	}
	else
		return ZEROTIME;
}

LDATETIME FASTCALL SOraDbProvider::GetDateTime(OCIDateTime * pOciDtm)
{
	LDATETIME dtm;
	dtm.d = GetDate(pOciDtm);
	dtm.t = GetTime(pOciDtm);
	return dtm;
}

int SOraDbProvider::SetDate(OCIDateTime * pOciDtm, LDATE dt)
{
	return ProcessError(OCIDateTimeConstruct(Env, Err, pOciDtm, dt.year(), dt.month(), dt.day(), 0, 0, 0, 0, 0, 0));
}

int SOraDbProvider::SetTime(OCIDateTime * pOciDtm, LTIME tm)
{
	return ProcessError(OCIDateTimeConstruct(Env, Err, pOciDtm, 2000, 1, 1, tm.hour(), tm.minut(), tm.sec(),
		tm.hs() * OCI_TIME_FRACT, 0, 0));
}

int SOraDbProvider::SetDateTime(OCIDateTime * pOciDtm, LDATETIME dtm)
{
	return ProcessError(OCIDateTimeConstruct(Env, Err, pOciDtm, dtm.d.year(), dtm.d.month(), dtm.d.day(),
		dtm.t.hour(), dtm.t.minut(), dtm.t.sec(), dtm.t.hs() * OCI_TIME_FRACT, 0, 0));
}

int SOraDbProvider::RowidToStr(OD rowid, SString & rBuf)
{
	rBuf.Z();
	OraText buf[64];
	uint16  buf_len = sizeof(buf);
	int     ok = ProcessError(OCIRowidToChar(rowid, buf, &buf_len, Err));
	rBuf = (const char *)buf;
	return ok;
}

size_t FASTCALL SOraDbProvider::RawGetSize(const RAW & r)
{
	uint32 sz = 0;
	return ProcessError(OCIRawAllocSize(Env, Err, r.P, &sz)) ? sz : 0;
}

int SOraDbProvider::RawResize(RAW & r, size_t newSize)
{
	return ProcessError(OCIRawResize(Env, Err, (uint16)newSize, &r.P));
}

int SOraDbProvider::RawCopyFrom(RAW & r, const void * pBuf, size_t dataSize)
{
	return ProcessError(OCIRawAssignBytes(Env, Err, (const uint8 *)pBuf, dataSize, &r.P));
}

void * FASTCALL SOraDbProvider::RawGetPtr(const RAW & r)
{
	return OCIRawPtr(Env, r.P);
}

int SOraDbProvider::LobRead(OD & rLob, TYPEID typ, SLob * pBuf, size_t * pActualSize)
{
	//
	// @todo Учесть особенность CLOB - параметр sz в функцию OCILobRead передается и
	//   возвращается в терминах символов (не байт)
	//
	int    ok = 1;
	const  uint32 raw_buf_size = GETSSIZE(typ);
	uint32 rd_size = 0; // Количество прочитанных байт
	uint32 offs = 1;    // Смещение для функции OCILobRead начинается с 1 (не с 0).
	uint32 chunk_size = 0;
	void * ptr = 0;
	SBuffer large_buf;
	STempBuffer temp_buf(0);
	pBuf->Init(0); // Инициализируем неструктурированный буфер
	//
	// Руководство по OCI рекомендует обрабатывать LOB-данные отрезками кратными chunk_size,
	// значение которого специфично для конкретного LOB.
	//
	THROW(ProcessError(OCILobGetChunkSize(Srvc, Err, rLob, &chunk_size)));
	//
	for(int status = OCI_NEED_DATA; status == OCI_NEED_DATA;) {
		uint32 sz = 0;
		uint32 max_buf_size = 0;
		if((rd_size + chunk_size) > raw_buf_size) {
			if(temp_buf.GetSize() < chunk_size)
				temp_buf.Alloc(chunk_size);
			if(ptr == pBuf->GetRawDataPtr()) {
				THROW(large_buf.Write(ptr, rd_size));
			}
			ptr = temp_buf;
			sz = chunk_size;
			max_buf_size = chunk_size;
		}
		else {
			ptr = PTR8(pBuf->GetRawDataPtr()) + rd_size;
			sz = chunk_size;
			max_buf_size = raw_buf_size - rd_size;
		}
		assert(ptr != 0);
		assert(sz != 0);
		status = OCILobRead(Srvc, Err, rLob, &sz, offs, ptr, max_buf_size, 0, 0, 0, SQLCS_IMPLICIT);
		if(oneof2(status, OCI_SUCCESS, OCI_NEED_DATA)) {
			rd_size += sz;
			offs += sz;
			if(ptr != pBuf->GetRawDataPtr()) {
				THROW(large_buf.Write(ptr, sz));
			}
		}
		else {
			THROW(ProcessError(status));
		}
	}
	if(ptr != pBuf->GetRawDataPtr()) {
		uint32 sz = large_buf.GetAvailableSize();
		THROW(pBuf->InitPtr(sz));
		if(sz) {
			THROW(ptr = pBuf->GetRawDataPtr());
			THROW(large_buf.Read(ptr, sz));
		}
	}
	CATCHZOK
	ASSIGN_PTR(pActualSize, rd_size);
	return ok;
}

int SOraDbProvider::LobWrite(OD & rLob, TYPEID typ, SLob * pBuf, size_t dataLen)
{
	//
	// @todo Учесть особенность CLOB - параметр sz в функцию OCILobRead передается и
	//   возвращается в терминах символов (не байт)
	//
	int    ok = 1;
	uint32 offs = 1; // Смещение для функции OCILobRead начинается с 1 (не с 0).
	uint32 sz = dataLen;
	void * ptr = pBuf ? pBuf->GetRawDataPtr() : 0;
	if(!dataLen || !ptr) {
		sz = 0xffffffff;
		THROW(OdAttrSet(rLob, OCI_ATTR_LOBEMPTY, 0, 0));
	}
	else {
		THROW(ProcessError(OCILobWrite(Srvc, Err, rLob, &sz, offs, ptr, dataLen, OCI_ONE_PIECE, 0, 0, 0, SQLCS_IMPLICIT)));
	}
	CATCHZOK
	return ok;
}
//
//
//
/*virtual*/ int SLAPI SOraDbProvider::Login(const DbLoginBlock * pBlk, long options)
{
	/*
void ConnectBase::MakeTNSString (std::string& str, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid)
{
    str = std::string("(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(Host=") + host + ")(Port=" + port + ")))"
        "(CONNECT_DATA=(" + (serviceInsteadOfSid ? "SERVICE_NAME" : "SID") + "=" + sid +")))";
}

    (DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(Host=") + host + ")(Port=" + port + ")))(CONNECT_DATA=(" + (serviceInsteadOfSid ? "SERVICE_NAME" : "SID") + "=" + sid +")))";

	*/
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	char   temp_buf[512];
	SString tns;
	SString attr;
	pBlk->GetAttr(DbLoginBlock::attrDbName, attr);
	{
		SString host, port, sid;
		StringSet ss(':', attr);
		uint ssp = 0;
		if(ss.get(&ssp, host)) {
			if(ss.get(&ssp, port)) {
				if(ss.get(&ssp, sid)) {
				}
				else {
					sid = port;
					(port = 0).Cat(1521);
				}
			}
			else {
				sid = host;
				host = "localhost";
				(port = 0).Cat(1521);
			}
		}
		tns.CatChar('(').
			Cat("DESCRIPTION").Eq().CatChar('(').
				Cat("ADDRESS_LIST").Eq().CatChar('(').
					Cat("ADDRESS").Eq().CatChar('(').
						CatEq("PROTOCOL", "TCP").
					CatChar(')').CatChar('(').
						CatEq("HOST", host).
					CatChar(')').CatChar('(').
						CatEq("PORT", port).
			CatCharN(')', 3).CatChar('(').
				Cat("CONNECT_DATA").Eq().CatChar('(').
					CatEq("SID", sid).
		CatCharN(')', 3);
	}
	size_t len = tns.Len();
	tns.CopyTo(temp_buf, sizeof(temp_buf));
	THROW(ProcessError(OCIServerAttach(Svr, Err, (text *)temp_buf, len, OCI_DEFAULT)));
	//
	Srvc = OhAlloc(OCI_HTYPE_SVCCTX);
	THROW(OhAttrSet(Srvc, OCI_ATTR_SERVER, Svr.H, 0));
	{
		//
		// Устанавливаем символ базы данных как INTERNAL_NAME.
		// Этот атрибут необходим для правильной работы транзакций.
		//
		pBlk->GetAttr(DbLoginBlock::attrDbSymb, attr);
		if(attr.Len())
			THROW(OhAttrSet(Svr, OCI_ATTR_INTERNAL_NAME, (void *)attr.cptr(), attr.Len())); // @badcast
	}
	Sess = OhAlloc(OCI_HTYPE_SESSION);
	//
	pBlk->GetAttr(DbLoginBlock::attrUserName, attr);
	attr.CopyTo(temp_buf, sizeof(temp_buf));
	THROW(OhAttrSet(Sess, OCI_ATTR_USERNAME, temp_buf, strlen(temp_buf)));
	//
	pBlk->GetAttr(DbLoginBlock::attrPassword, attr);
	attr.CopyTo(temp_buf, sizeof(temp_buf));
	THROW(OhAttrSet(Sess, OCI_ATTR_PASSWORD, temp_buf, strlen(temp_buf)));
	//
	THROW(ProcessError(OCISessionBegin(Srvc, Err, Sess, OCI_CRED_RDBMS, OCI_DEFAULT)));
	THROW(OhAttrSet(Srvc, OCI_ATTR_SESSION, Sess.H, 0));
	THROW(Common_Login(pBlk));
	CATCHZOK
	return ok;
}

/*virtual*/ int SLAPI SOraDbProvider::Logout()
{
	int    ok = -1;
	if(State & stLoggedIn) {
		ok = ProcessError(OCISessionEnd(Srvc, Err, Sess, OCI_DEFAULT));
	}
	return Common_Logout();
}

SString & SLAPI SOraDbProvider::MakeFileName_(const char * pTblName, SString & rBuf)
{
	if(rBuf.Empty())
		rBuf = pTblName;
	return rBuf;
}

int SLAPI SOraDbProvider::GetFileStat(const char * pFileName, long reqItems, DbTableStat * pStat)
{
	int    ok = -1;
	uint   actual = 0;
	//
	// SELECT OWNER, TABLESPACE_NAME, NUM_ROWS, TEMPORARY FROM ALL_TABLES WHERE TABLE_NAME = 'pFileName'
	//
	BNFieldList fld_list;
	struct OraTblEntry {
		char   Owner[32];
		char   TableSpace[32];
		long   NumRows;
		char   Temp[8];
	} rec_buf;
	char   name[64];
	STRNSCPY(name, pFileName);
	strupr(name);
	fld_list.addField("OWNER", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("TABLESPACE_NAME", MKSTYPE(S_ZSTRING, 32));
	fld_list.addField("NUM_ROWS", T_INT32);
	fld_list.addField("TEMPORARY", MKSTYPE(S_ZSTRING, 8));
	SqlGen.Reset().Select(&fld_list).From("ALL_TABLES").Sp().Tok(Generator_SQL::tokWhere).Sp().
		Eq("TABLE_NAME", name);
	SSqlStmt stmt(this, (const SString &)SqlGen);
	THROW(stmt.Exec(0, OCI_DEFAULT));
	THROW(stmt.BindData(+1, 1, fld_list, &rec_buf, 0));
	THROW(Binding(stmt, +1));
	if(Fetch(stmt, 1, &actual) && actual) {
		THROW(stmt.GetData(0));
		ok = 1;
		if(pStat) {
			pStat->NumRecs = rec_buf.NumRows;
			SETFLAG(pStat->Flags, XTF_TEMP, rec_buf.Temp[0] == 'Y');
			pStat->OwnerName = rec_buf.Owner;
			pStat->SpaceName = rec_buf.TableSpace; // DbTableStat
			if(reqItems & DbTableStat::iFldList) {
			}
			if(reqItems & DbTableStat::iIdxList) {
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SOraDbProvider::IsFileExists_(const char * pFileName)
{
	return BIN(GetFileStat(pFileName, 0, 0) > 0);
}

SString & SLAPI SOraDbProvider::GetTempFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath)
{
	if(pStart == 0) {
		SString temp_buf;
		for(long i = 1; i < 1000000; i++) {
			(temp_buf = "TMP").CatLongZ(i, 10);
			if(!IsFileExists_(temp_buf))
				rFileNameBuf = temp_buf;
		}
	}
	else {
		(*pStart)++;
		(rFileNameBuf = "TMP").CatLongZ(*pStart, 10);
	}
	return rFileNameBuf;
}

int SLAPI SOraDbProvider::CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode)
{
	int    ok = 1;
	THROW(SqlGen.Reset().CreateTable(*pTbl, 0));
	{
		SSqlStmt stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	uint j;
	for(j = 0; j < pTbl->indexes.getNumKeys(); j++) {
		THROW(SqlGen.Reset().CreateIndex(*pTbl, pFileName, j));
		{
			SSqlStmt stmt(this, (const SString &)SqlGen);
			THROW(stmt.Exec(1, OCI_DEFAULT));
		}
	}
	for(j = 0; j < pTbl->fields.getCount(); j++) {
		TYPEID _t = pTbl->fields[j].T;
		if(GETSTYPE(_t) == S_AUTOINC) {
			THROW(SqlGen.Reset().CreateSequenceOnField(*pTbl, pFileName, j, 0));
			{
				SSqlStmt stmt(this, (const SString &)SqlGen);
				THROW(stmt.Exec(1, OCI_DEFAULT));
			}
		}
	}
	if(createMode < 0 && IS_CRM_TEMP(createMode)) {
		//
		// Регистрируем имя временного файла в драйвере БД для последующего удаления //
		//
		AddTempFileName(pFileName);
	}
	CATCHZOK
	return ok;
}

int SLAPI SOraDbProvider::DropFile(const char * pFileName)
{
	int    ok = 1;
	if(IsFileExists_(pFileName) > 0) {
		SqlGen.Reset().Tok(Generator_SQL::tokDrop).Sp().Tok(Generator_SQL::tokTable).Sp().Text(pFileName);
			//Sp().Text("PURGE");
		SSqlStmt stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
		{
			//
			// select * from all_sequences where lower(substr(sequence_name, 0, 3)) = 'seq'
			//
			SString seq_prefix, q_pfx;
			Generator_SQL::PrefixName(pFileName, Generator_SQL::pfxSequence, seq_prefix, 0);
			seq_prefix.CatChar('_').ToUpper();

			char   seq_name[128];
			uint   actual = 0;
			StrAssocArray seq_list;
			BNFieldList fld_list;
			(q_pfx = "upper(substr(sequence_name,0,").Cat(seq_prefix.Len()).CatCharN(')', 2);
			fld_list.addField("sequence_name", MKSTYPE(S_ZSTRING, 64));
			SqlGen.Reset().Select(&fld_list).From("all_sequences").Sp().Tok(Generator_SQL::tokWhere).Sp().
				Eq(q_pfx, seq_prefix);

			SSqlStmt stmt(this, (const SString &)SqlGen);
			THROW(stmt.Exec(0, OCI_DEFAULT));
			THROW(stmt.BindData(+1, 1, fld_list, seq_name, 0));
			THROW(Binding(stmt, +1));
			while(Fetch(stmt, 1, &actual) && actual) {
				THROW(stmt.GetData(0));
				seq_list.Add(seq_list.getCount()+1, seq_name);
			}
			for(uint i = 0; i < seq_list.getCount(); i++) {
				SqlGen.Reset().Tok(Generator_SQL::tokDrop).Sp().Tok(Generator_SQL::tokSequence).Sp().Text(seq_list.at(i).Txt);
				SSqlStmt stmt(this, (const SString &)SqlGen);
				THROW(stmt.Exec(1, OCI_DEFAULT));
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI SOraDbProvider::GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat)
{
	SString file_name;
	return GetFileStat(MakeFileName_(pTbl->tableName, file_name), reqItems, pStat);
}

int SLAPI SOraDbProvider::Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword)
{
	pTbl->fileName = NZOR(pFileName, pTbl->tableName);
	pTbl->OpenedFileName = pTbl->fileName;
	pTbl->FixRecSize = pTbl->fields.getRecSize();
	return 1;
}

int SLAPI SOraDbProvider::Implement_Close(DBTable * pTbl)
{
	return 1;
}

int SOraDbProvider::Helper_Fetch(DBTable * pTbl, DBTable::SelectStmt * pStmt, uint * pActual)
{
	int    ok = 1;
	uint   actual = 0;
	THROW(ok = Fetch(*pStmt, 1, &actual));
	if(ok > 0) {
		THROW(pStmt->GetData(0));
		if(pStmt->Sf & DBTable::sfForUpdate) {
			SString temp_buf;
			OD     rowid = OdAlloc(OCI_DTYPE_ROWID);
			THROW(OhAttrGet(StmtHandle(*pStmt), OCI_ATTR_ROWID, (OCIRowid *)rowid, 0));
			RowidToStr(rowid, temp_buf);
			pTbl->getCurRowIdPtr()->FromStr(temp_buf);
			OdFree(rowid);
		}
		BtrError = 0;
		ok = 1;
	}
	else {
		BtrError = BE_EOF;
		ok = -1;
	}
	CATCHZOK
	ASSIGN_PTR(pActual, actual);
	return ok;
}

int SLAPI SOraDbProvider::Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf)
{
	// BNKeyList BNFieldList BNKey Generator_SQL
	//
	// select /*+ index_asc(tbl_name index_name) */ * from
	//
	int    ok = 1;
	//
	// Если can_continue == 1, то допускается последующий запрос spNext или spPrev
	// Соответственно, stmt сохраняется в pTbl.
	//
	int    can_continue = 0;
	int    new_stmt = 0;
	uint   actual = 0;
	LongArray seg_map; // Карта номеров сегментов индекса, которые должны быть привязаны
	DBTable::SelectStmt * p_stmt = 0;
	THROW(idx < (int)pTbl->indexes.getNumKeys());
	if(oneof2(srchMode, spNext, spPrev)) {
		p_stmt = pTbl->GetStmt();
		if(p_stmt) {
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0) {
				int    r = 1;
				pTbl->copyBufToKey(idx, pKey);
				if(oneof5(p_stmt->Sp, spGt, spGe, spLt, spLe, spEq)) {
					int kc = pTbl->indexes.compareKey(p_stmt->Idx, pKey, p_stmt->Key);
					if(kc == 0) {
						if(oneof2(p_stmt->Sp, spGt, spLt))
							r = 0;
					}
					else if(kc < 0) {
						if(oneof3(p_stmt->Sp, spGt, spGe, spEq))
							r = 0;
					}
					else if(kc > 0) {
						if(oneof3(p_stmt->Sp, spLt, spLe, spEq))
							r = 0;
					}
				}
				if(r)
					can_continue = 1;
				else {
					BtrError = BE_EOF;
					ok = 0;
				}
			}
			else
				ok = 0;
		}
		else {
			BtrError = BE_EOF;
			ok = 0;
		}
	}
	else {
		//
		// Для того, чтобы hint'ы работали, необхоидмо и в hint'е и в
		// префиксах списков полей указывать либо алиас, либо наименование таблицы,
		// но не смешивать.
		// Например конструкция //
		// SELECT/*+INDEX_DESC(Reference2 idxReference2Key0)*/ t.*, t.ROWID FROM Reference2 t WHERE ObjType<=6 AND (ObjID<0 OR (ObjType<>6 ))
		// работать будет не по hint'у поскольку в хинте указано наименование таблицы, а в списке
		// полей - алиас.
		//

		//
		// Алиас нужен в том случае, если кроме списка полей необходимо достать rowid
		// (for update и так возвращает rowid, то есть явно его указывать в этом случае не надо).
		//
		const char * p_alias = (sf & DBTable::sfForUpdate) ? 0 : "t";
		SString temp_buf;
		uint8  temp_key[1024];
		void * p_key_data = pKey;
		BNKey  key = pTbl->indexes[idx];
		const  int ns = key.getNumSeg();

		SqlGen.Reset().Tok(Generator_SQL::tokSelect);
		if(!(sf & DBTable::sfDirect)) {
			SqlGen.HintBegin().
			HintIndex(*pTbl, p_alias, idx, BIN(oneof3(srchMode, spLt, spLe, spLast))).
			HintEnd();
		}
		SqlGen.Sp();
		if(!(sf & DBTable::sfForUpdate))
			SqlGen.Text(p_alias).Dot().Aster().Com().Text(p_alias).Dot().Tok(Generator_SQL::tokRowId);
		else
			SqlGen.Aster();
		SqlGen.Sp().From(pTbl->fileName, p_alias);
		if(sf & DBTable::sfDirect) {
			DBRowId * p_rowid = (DBRowId *)pKey;
			THROW(p_rowid && p_rowid->IsLong());
			p_rowid->ToStr(temp_buf);
			SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
		}
		else {
			if(oneof2(srchMode, spFirst, spLast)) {
				memzero(temp_key, sizeof(temp_key));
				pTbl->indexes.setBound(idx, 0, BIN(srchMode == spLast), temp_key);
				p_key_data = temp_key;
			}
			SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp();
			for(int i = 0; i < ns; i++) {
				int fldid = key.getFieldID(i);
				const BNField fld = pTbl->indexes.field(idx, i);
				if(i > 0) { // НЕ первый сегмент
					SqlGen.Tok(Generator_SQL::tokAnd).Sp();
					if(srchMode != spEq)
						SqlGen.LPar();
				}

				if(key.getFlags(i) & XIF_ACS) {
					//
					// Для ORACLE нечувствительность к регистру символов
					// реализуется функциональным сегментом индекса nls_lower(fld).
					// Аналогичная конструкция применяется при генерации скрипта создания индекса
					// См. Generator_SQL::CreateIndex(const DBTable &, const char *, uint)
					//
					SqlGen.Func(Generator_SQL::tokNlsLower, fld.Name);
				}
				else
					SqlGen.Text(fld.Name);
				int   cmps = _EQ_;
				if(srchMode == spEq)
					cmps = _EQ_;
				else if(srchMode == spLt)
					cmps = (i == ns-1) ? _LT_ : _LE_;
				else if(oneof2(srchMode, spLe, spLast))
					cmps = _LE_;
				else if(srchMode == spGt) {
					cmps = (i == ns-1) ? _GT_ : _GE_;
				}
				else if(oneof2(srchMode, spGe, spFirst))
					cmps = _GE_;
				SqlGen._Symb(cmps);
				SqlGen.Param(temp_buf.NumberToLat(i));
				seg_map.add(i);

				if(i > 0 && srchMode != spEq) {
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
					SqlGen.Sp().Tok(Generator_SQL::tokOr).Sp().LPar();
					for(int j = 0; j < i; j++) {
						const BNField fld2 = pTbl->indexes.field(idx, j);
						if(j > 0)
							SqlGen.Tok(Generator_SQL::tokAnd).Sp();
						if(key.getFlags(j) & XIF_ACS)
							SqlGen.Func(Generator_SQL::tokNlsLower, fld2.Name);
						else
							SqlGen.Text(fld2.Name);
						SqlGen._Symb(_NE_);
						SqlGen.Param(temp_buf.NumberToLat(j));
						seg_map.add(j);
						SqlGen.Sp();
					}
					SqlGen.RPar().RPar().Sp();
				}
				SqlGen.Sp();
			}
			can_continue = 1;
		}
		if(sf & DBTable::sfForUpdate)
			SqlGen.Tok(Generator_SQL::tokFor).Sp().Tok(Generator_SQL::tokUpdate);
		{
			THROW(p_stmt = new DBTable::SelectStmt(this, (const SString &)SqlGen, idx, srchMode, sf));
			new_stmt = 1;
			THROW(p_stmt->IsValid());
			{
				size_t key_len = 0;
				p_stmt->BL.Dim = 1;
				for(uint i = 0; i < seg_map.getCount(); i++) {
					const int  seg = seg_map.get(i);
					const BNField & r_fld = pTbl->indexes.field(idx, seg);
					const size_t seg_offs = pTbl->indexes.getSegOffset(idx, seg);
					key_len += stsize(r_fld.T);
					if(key.getFlags(seg) & XIF_ACS) {
						strlwr866((char *)(PTR8(p_key_data) + seg_offs));
					}
					SSqlStmt::Bind b;
					b.Pos = -(int16)(i+1);
					b.Typ = r_fld.T;
					b.P_Data = PTR8(p_key_data) + seg_offs;
					uint   lp = 0;
					if(p_stmt->BL.lsearch(&b.Pos, &lp, PTR_CMPFUNC(int16)))
						p_stmt->BL.atFree(lp);
					p_stmt->BL.insert(&b);
				}
				memcpy(p_stmt->Key, pKey, MIN(sizeof(p_stmt->Key), key_len));
				THROW(Binding(*p_stmt, -1));
				THROW(p_stmt->SetData(0));
			}
			THROW(p_stmt->Exec(0, OCI_DEFAULT));
			if(!(sf & DBTable::sfForUpdate)) {
				int rowid_pos = pTbl->fields.getCount()+1;
				THROW(p_stmt->BindRowId(rowid_pos, 1, pTbl->getCurRowIdPtr()));
			}
			THROW(p_stmt->BindData(+1, 1, pTbl->fields, pTbl->getDataBuf(), pTbl->getLobBlock()));
			THROW(ok = Helper_Fetch(pTbl, p_stmt, &actual));
			if(ok > 0)
				pTbl->copyBufToKey(idx, pKey);
			else {
				ok = 0;
				can_continue = 0;
			}
		}
	}
	CATCH
		ok = 0;
		can_continue = 0;
	ENDCATCH
	if(can_continue) {
		pTbl->SetStmt(p_stmt);
	}
	else {
		pTbl->SetStmt(0);
		if(new_stmt)
			delete p_stmt;
	}
	return ok;
}

int SLAPI SOraDbProvider::Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData)
{
	int    ok = 1;
	int    subst_no = 0;
	uint   i;
	int    do_process_lob = 0;
	int    map_ret_key = 0;
	BNKey  key;
	uint   ns = 0;

	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf, let_buf;
	SSqlStmt  stmt(this, 0);
	if(pData)
		pTbl->copyBufFrom(pData);
	if(pTbl->State & DBTable::sHasLob) {
		int    r = 0;
		THROW(r = pTbl->StoreAndTrimLob());
		if(r > 0)
			do_process_lob = 1;
	}

	SqlGen.Reset().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(pTbl->fileName).Sp();
	SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();

	stmt.BL.Dim = 1;
	stmt.BL.P_Lob = pTbl->getLobBlock();
	for(i = 0; i < fld_count; i++) {
		if(i)
			SqlGen.Com();
		SqlGen.Param(temp_buf.NumberToLat(subst_no++));
		const BNField & r_fld = pTbl->fields.getField(i);
		if(GETSTYPE(r_fld.T) == S_AUTOINC) {
			long val = 0;
			size_t val_sz = 0;
			r_fld.getValue(pTbl->getDataBuf(), &val, &val_sz);
			assert(val_sz == sizeof(val));
			if(val == 0) {
				THROW(GetAutolongVal(*pTbl, i, &val));
				r_fld.setValue(pTbl->getDataBuf(), &val);
			}
		}
		stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pTbl->getDataBuf()) + r_fld.Offs);
	}
	SqlGen.RPar();
	SqlGen.Sp().Tok(Generator_SQL::tokReturning).Sp().Tok(Generator_SQL::tokRowId);
	//
	// temp_buf будет содержать список переменных, в которые должны заносится возвращаемые значения //
	//
	let_buf.NumberToLat(subst_no++);
	temp_buf.Z().CatChar(':').Cat(let_buf);
	stmt.BindRowId(-subst_no, 1, pTbl->getCurRowIdPtr());
	if(pKeyBuf && idx >= 0 && idx < (int)pTbl->indexes.getNumKeys()) {
		map_ret_key = 1;
		key = pTbl->indexes[idx];
		ns = (uint)key.getNumSeg();
		for(i = 0; i < ns; i++) {
			const BNField & r_fld = pTbl->indexes.field(idx, i);
			SqlGen.Com().Text(r_fld.Name);
			let_buf.NumberToLat(subst_no++);
			temp_buf.CatDiv(',', 0).CatChar(':').Cat(let_buf);
			stmt.BindItem(-subst_no, 1, r_fld.T, PTR8(pKeyBuf)+pTbl->indexes.getSegOffset(idx, i));
		}
	}
	SqlGen.Sp().Tok(Generator_SQL::tokInto).Sp().Text(temp_buf);
	{
		THROW(stmt.SetText((SString &)SqlGen));
		THROW(Binding(stmt, -1));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, OCI_DEFAULT));
		THROW(stmt.GetOutData(0));
		if(do_process_lob) {
			//
			// Если в записи были не пустые значения LOB-полей, то придется перечитать
			// вставленную запись и изменить значения LOB-полей.
			//
			// @todo Надо обновлять только LOB-поля, а не всю запись.
			//
			DBRowId row_id = *pTbl->getCurRowIdPtr();
			THROW(Implement_Search(pTbl, -1, &row_id, spEq, DBTable::sfDirect | DBTable::sfForUpdate));
			THROW(pTbl->RestoreLob());
			THROW(Implement_UpdateRec(pTbl, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SOraDbProvider::Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc)
{
	int    ok = 1;
	uint   i;
	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf;
	if(pDataBuf)
		pTbl->copyBufFrom(pDataBuf);
	SqlGen.Reset().Tok(Generator_SQL::tokUpdate).Sp().Text(pTbl->fileName).Sp().
		Tok(Generator_SQL::tokSet).Sp();
	for(i = 0; i < fld_count; i++) {
		if(i)
			SqlGen.Com();
		SqlGen.Text(pTbl->fields[i].Name)._Symb(_EQ_).Param(temp_buf.NumberToLat(i));
	}
	THROW(pTbl->getCurRowIdPtr()->IsLong());
	pTbl->getCurRowIdPtr()->ToStr(temp_buf);
	SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
	{
		SSqlStmt   stmt(this, (const SString &)SqlGen);
		THROW(stmt.IsValid());
		THROW(stmt.BindData(-1, 1, pTbl->fields, pTbl->getDataBuf(), pTbl->getLobBlock()));
		THROW(stmt.SetDataDML(0));
		THROW(stmt.Exec(1, /*OCI_COMMIT_ON_SUCCESS*/OCI_DEFAULT)); // @debug(OCI_COMMIT_ON_SUCCESS)
	}
	CATCHZOK
	return ok;
}

int SLAPI SOraDbProvider::Implement_DeleteRec(DBTable * pTbl)
{
	int    ok = 1;
	const  uint fld_count = pTbl->fields.getCount();
	SString temp_buf;
	SqlGen.Reset().Tok(Generator_SQL::tokDelete).Sp().From(pTbl->fileName, 0).Sp();
	THROW(pTbl->getCurRowIdPtr()->IsLong());
	pTbl->getCurRowIdPtr()->ToStr(temp_buf);
	SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp().Tok(Generator_SQL::tokRowId)._Symb(_EQ_).QText(temp_buf);
	{
		SSqlStmt   stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	CATCHZOK
	return ok;
}

int SLAPI SOraDbProvider::Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ)
{
	int    ok = 1, ta = 0;
	if(useTa) {
		THROW(StartTransaction());
		ta = 1;
	}
	SqlGen.Reset().Tok(Generator_SQL::tokDelete).Sp().From(pTbl->fileName, 0);
	if(&rQ && rQ.tree) {
		SqlGen.Sp().Tok(Generator_SQL::tokWhere).Sp();
		rQ.tree->CreateSqlExpr(&SqlGen, -1);
	}
	{
		SSqlStmt stmt(this, (const SString &)SqlGen);
		THROW(stmt.Exec(1, OCI_DEFAULT));
	}
	if(ta) {
		THROW(CommitWork());
		ta = 0;
	}
	CATCH
		if(ta) {
			RollbackWork();
			ta = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI SOraDbProvider::Implement_BExtInsert(BExtInsert * pBei)
{
	int    ok = -1;
	//
	// Чтобы не затирать содержимое внутреннего буфера таблицы pBei->P_Tbl распределяем
	// временный буфер rec_buf.
	//
	SBaseBuffer rec_buf;
	rec_buf.Init();
	const  uint num_recs = pBei->GetCount();
	if(num_recs) {
		uint   i;
		DBTable * p_tbl = pBei->getTable();
		const  uint fld_count = p_tbl->fields.getCount();
		SString temp_buf;
		SqlGen.Reset().Tok(Generator_SQL::tokInsert).Sp().Tok(Generator_SQL::tokInto).Sp().Text(p_tbl->fileName).Sp();
		SqlGen.Tok(Generator_SQL::tokValues).Sp().LPar();
		for(i = 0; i < fld_count; i++) {
			if(i)
				SqlGen.Com();
			SqlGen.Param(temp_buf.NumberToLat(i));
		}
		SqlGen.RPar();
		{
			SSqlStmt stmt(this, (const SString &)SqlGen);
			THROW(stmt.IsValid());
			THROW(rec_buf.Alloc(p_tbl->getBufLen()));
			THROW(stmt.BindData(-1, num_recs, p_tbl->fields, rec_buf.P_Buf, p_tbl->getLobBlock()));
			for(i = 0; i < num_recs; i++) {
				SBaseBuffer b = pBei->Get(i);
				assert(b.Size <= rec_buf.Size);
				memcpy(rec_buf.P_Buf, b.P_Buf, b.Size);
				if(p_tbl->State & DBTable::sHasAutoinc) {
					for(uint j = 0; j < fld_count; j++) {
						const BNField & r_fld = p_tbl->fields[j];
						if(GETSTYPE(r_fld.T) == S_AUTOINC) {
							long val = 0;
							size_t val_sz = 0;
							r_fld.getValue(rec_buf.P_Buf, &val, &val_sz);
							assert(val_sz == sizeof(val));
							if(val == 0) {
								THROW(GetAutolongVal(*p_tbl, j, &val));
								r_fld.setValue(rec_buf.P_Buf, &val);
							}
						}
					}
				}
				THROW(stmt.SetDataDML(i));
			}
			THROW(stmt.Exec(num_recs, OCI_DEFAULT));
			ok = 1;
		}
	}
	CATCHZOK
	rec_buf.Destroy();
	return ok;
}

int SLAPI SOraDbProvider::ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection)
{
	return 0;
}
