// DL200.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2007, 2008, 2009, 2010, 2011, 2012, 2015, 2016, 2017, 2018, 2019, 2020
//
#include <pp.h>
#pragma hdrstop
//
//
//
static int SLAPI WriteSArrayToFile(const SArray * pAry, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 i;
	const  uint16 c = (uint16)SVectorBase::GetCount(pAry);
	size_t item_size = pAry ? pAry->getItemSize() : 0;
	long   beg_pos = ftell(pStream);
	THROW_V(fwrite(&c, sizeof(c), 1, pStream) == 1, SLERR_WRITEFAULT);
	for(i = 0; i < c; i++)
		THROW_V(fwrite(pAry->at(i), item_size, 1, pStream) == 1, SLERR_WRITEFAULT);
	CATCH
		fseek(pStream, beg_pos, SEEK_SET);
		ok = 0;
	ENDCATCH
	return ok;
}

static int SLAPI ReadSArrayFromFile(SArray * pAry, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 i, c = 0;
	size_t item_size = pAry->getItemSize();
	long   beg_pos = ftell(pStream);
	pAry->freeAll();
	char * p_buf = static_cast<char *>(SAlloc::M(item_size));
	THROW(p_buf);
	THROW_V(fread(&c, sizeof(c), 1, pStream) == 1, SLERR_READFAULT);
	for(i = 0; i < c; i++) {
		SLibError = SLERR_READFAULT;
		THROW_V(fread(p_buf, item_size, 1, (FILE *)pStream) == 1, SLERR_READFAULT);
		THROW_V(pAry->insert(p_buf), SLERR_READFAULT);
	}
	CATCH
		if(beg_pos >= 0)
			fseek(pStream, beg_pos, SEEK_SET);
		ok = 0;
	ENDCATCH
	SAlloc::F(p_buf);
	return ok;
}

static int SLAPI WritePStrToFile(const char * pStr, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 s;
	if(pStr) {
		s = static_cast<uint16>(sstrlen(pStr)+1);
		THROW_V(fwrite(&s, sizeof(s), 1, pStream) == 1, SLERR_WRITEFAULT);
		THROW_V(fwrite(pStr, s, 1, pStream) == 1, SLERR_WRITEFAULT);
	}
	else {
		s = 0;
		THROW_V(fwrite(&s, sizeof(s), 1, pStream) == 1, SLERR_WRITEFAULT);
	}
	CATCHZOK
	return ok;
}

static int SLAPI ReadPStrFromFile(char ** ppStr, FILE * pStream)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint16 s;
	THROW_V(fread(&s, sizeof(s), 1, pStream) == 1, SLERR_READFAULT);
	if(s) {
		THROW_V(*ppStr = new char[s], SLERR_NOMEM);
		THROW_V(fread(*ppStr, s, 1, pStream) == 1, SLERR_READFAULT);
	}
	else
		*ppStr = 0;
	CATCHZOK
	return ok;
}

/*static int SLAPI ReadPStrFromFile(SString & rStr, FILE * pStream)
{
	int    ok = 1;
	uint16 s;
	rStr.Z();
	THROW_S(fread(&s, sizeof(s), 1, pStream) == 1, SLERR_READFAULT);
	if(s) {
		STempBuffer temp_buf(s);
		THROW_S(temp_buf.IsValid(), SLERR_NOMEM);
		THROW_S(fread(temp_buf, s, 1, pStream) == 1, SLERR_READFAULT);
		rStr = temp_buf;
	}
	CATCHZOK
	return ok;
}*/
//
//
//
#ifdef DL200C // {

int SLAPI PPSetErrorNoMem() { return PPSetError(PPERR_NOMEM); }
int SLAPI PPSetErrorSLib() { return PPSetError(PPERR_SLIB); }
int FASTCALL PPSetError(int errCode) { return PPSetError(errCode, static_cast<const char *>(0)); }
int FASTCALL PPSetError(int errCode, const char * pAddedMsg)
{
	PPErrCode = errCode;
	if(pAddedMsg)
		PPSetAddedMsgString(pAddedMsg);
	return 0;
}

#endif // } DL200C

static char * FASTCALL skipws(char * p)
{
	while(oneof3(*p, ' ', '\t', '\n'))
		p++;
	return p;
}

static int FASTCALL chkch(char ** pp, int c, uint16 * pFlags, long f)
{
	if(toupper(*(*pp = skipws(*pp))) == c) {
		*pFlags |= f;
		(*pp)++;
		return 1;
	}
	return 0;
}
//
//
//
#define CREATE_CI_(a) { p_result = new DL2_CI; p_result->Init(a); }
#define CREATE_CI(a)  DL2_CI * p_result = new DL2_CI; p_result->Init(a);
#define CREATEANDRET_CI(a) \
	DL2_CI * p_result = new DL2_CI;\
	p_result->Init(a);\
	return p_result;

static DL2_CI * SLAPI _plus2(const DL2_CI * pA1, const DL2_CI * pA2)
{
	DL2_CI * p_result = 0;
	dl2cit t1 = pA1->CiType, t2 = pA2->CiType;
	//
	// If exception occur in this function, then errcode may be only
	// PPERR_DL200_INVPARAMTYPE
	//
	PPErrCode = PPERR_DL200_INVPARAMTYPE;
	//
	if(t1 == DL2CIT_STRING && t2 == DL2CIT_STRING) {
		size_t len = pA1->Len + pA2->Len;
		char * p_temp = static_cast<char *>(SAlloc::C(1, len));
		strcat(strcpy(p_temp, pA1->GetStr()), pA2->GetStr());
		p_result = DL2_CI::MakeStr(p_temp);
		SAlloc::F(p_temp);
	}
	else if(t1 == DL2CIT_REAL) {
		if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->R + pA2->R);
		}
		else if(t2 == DL2CIT_INT)
			CREATE_CI_(pA1->R + pA2->I);
	}
	else if(t1 == DL2CIT_INT) {
		if(t2 == DL2CIT_INT) {
			CREATE_CI_(pA1->I + pA2->I);
		}
		else if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->I + pA2->R);
		}
		else if(t2 == DL2CIT_DATE) {
			LDATE dt = pA2->D;
			plusdate(&dt, (int)pA1->I, 0);
			CREATE_CI_(dt);
		}
	}
	else if(t1 == DL2CIT_DATE) {
		if(t2 == DL2CIT_INT) {
			LDATE dt = pA1->D;
			plusdate(&dt, (int)pA2->I, 0);
			CREATE_CI_(dt);
		}
	}
	return p_result;
}

static DL2_CI * SLAPI _minus2(const DL2_CI * pA1, const DL2_CI * pA2)
{
	DL2_CI * p_result = 0;
	dl2cit t1 = pA1->CiType, t2 = pA2->CiType;
	//
	// If exception occur in this function, then errcode may be only
	// PPERR_DL200_INVPARAMTYPE
	//
	PPErrCode = PPERR_DL200_INVPARAMTYPE;
	//
	if(t1 == DL2CIT_REAL) {
		if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->R - pA2->R);
		}
		else if(t2 == DL2CIT_INT)
			CREATE_CI_(pA1->R - pA2->I);
	}
	else if(t1 == DL2CIT_INT) {
		if(t2 == DL2CIT_INT) {
			CREATE_CI_(pA1->I - pA2->I);
		}
		else if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->I - pA2->R);
		}
	}
	else if(t1 == DL2CIT_DATE) {
		if(t2 == DL2CIT_DATE) {
			CREATE_CI_(diffdate(&pA1->D, &pA2->D, 0));
		}
		else if(t2 == DL2CIT_INT) {
			LDATE dt = pA1->D;
			plusdate(&dt, (int)-pA2->I, 0);
			CREATE_CI_(dt);
		}
	}
	return p_result;
}


static DL2_CI * SLAPI _mult2(const DL2_CI * pA1, const DL2_CI * pA2)
{
	DL2_CI * p_result = 0;
	dl2cit t1 = pA1->CiType, t2 = pA2->CiType;
	//
	// If exception occur in this function, then errcode may be only
	// PPERR_DL200_INVPARAMTYPE
	//
	PPErrCode = PPERR_DL200_INVPARAMTYPE;
	//
	if(t1 == DL2CIT_REAL) {
		if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->R * pA2->R);
		}
		else if(t2 == DL2CIT_INT) {
			CREATE_CI_(pA1->R * pA2->I);
		}
	}
	else if(t1 == DL2CIT_INT) {
		if(t2 == DL2CIT_INT) {
			CREATE_CI_(pA1->I * pA2->I);
		}
		else if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->I * pA2->R);
		}
	}
	return p_result;
}

static DL2_CI * SLAPI _div2(const DL2_CI * pA1, const DL2_CI * pA2)
{
	DL2_CI * p_result = 0;
	dl2cit t1 = pA1->CiType, t2 = pA2->CiType;
	//
	// If exception occur in this function, then errcode may be only
	// PPERR_DL200_INVPARAMTYPE
	//
	PPErrCode = PPERR_DL200_INVPARAMTYPE;
	//
	if(t1 == DL2CIT_REAL) {
		if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->R / pA2->R);
		}
		else if(t2 == DL2CIT_INT) {
			CREATE_CI_(pA1->R / pA2->I);
		}
	}
	else if(t1 == DL2CIT_INT) {
		if(t2 == DL2CIT_INT) {
			CREATE_CI_(pA1->I / pA2->I);
		}
		else if(t2 == DL2CIT_REAL) {
			CREATE_CI_(pA1->I / pA2->R);
		}
	}
	return p_result;
}

DL2_CI * SLAPI DL2_Formula::ResolveOp(const DL2_CI * pOp, const DL2_Formula * pArgList) const
{
	DL2_CI * p_result = 0;
	switch(pOp->CiType) {
		case DL2CIT_OP_UMINUS:
			{
				p_result = new DL2_CI;
				const DL2_CI * p_arg = pArgList->GetByN(1);
				if(p_arg->CiType == DL2CIT_REAL)
					p_result->Init(-p_arg->R);
				else if(p_arg->CiType == DL2CIT_INT)
					p_result->Init(-p_arg->I);
				else if(p_arg->CiType == DL2CIT_STRING)
					p_result->Init(-satof(p_arg->GetStr())); // @v10.7.9 atof-->satof
				else
					PPSetError(PPERR_DL200_INVPARAMTYPE);
			}
			break;
		case DL2CIT_OP_UPLUS: p_result = DL2_CI::Copy(pArgList->GetByN(1)); break;
		case DL2CIT_OP_PLUS: p_result = _plus2(pArgList->GetByN(1), pArgList->GetByN(2)); break;
		case DL2CIT_OP_MINUS: p_result = _minus2(pArgList->GetByN(1), pArgList->GetByN(2)); break;
		case DL2CIT_OP_MULT: p_result = _mult2(pArgList->GetByN(1), pArgList->GetByN(2)); break;
		case DL2CIT_OP_DIV: p_result = _div2(pArgList->GetByN(1), pArgList->GetByN(2)); break;
		default:
			PPSetError(PPERR_DL200_UNDEFOP);
	}
	return p_result;
}
//
//
//
SLAPI DL2_Entry::DL2_Entry(uint16 type) : EntryType(type), Flags(0), P_Descript(0)
{
	Name[0] = 0;
}

int SLAPI DL2_Entry::Setup(const char * pName, const char * pDescript, int isRef)
{
	STRNSCPY(Name, pName);
	P_Descript = newStr(pDescript);
	SETFLAG(Flags, fRef, isRef);
	return 1;
}

DL2_Entry & FASTCALL DL2_Entry::operator = (const DL2_Entry & s)
{
	destroy();
	EntryType = s.EntryType;
	STRNSCPY(Name, s.Name);
	P_Descript = newStr(s.P_Descript);
	return *this;
}

void SLAPI DL2_Entry::destroy()
{
	if(oneof3(EntryType, DL2ENT_DATA, DL2ENT_GROUP, DL2ENT_ROW)) { // @v9.9.5
		Name[0] = 0;
		ZDELETE(P_Descript);
	}
}

int SLAPI DL2_Entry::Write(DL2_Storage * pStrg) const
{
	int    ok = 1;
	if(oneof3(EntryType, DL2ENT_DATA, DL2ENT_GROUP, DL2ENT_ROW)) { // @v9.9.5
		uint32 l, t = (uint32)EntryType;
		FILE * f = pStrg->P_Stream;
		uint32 offs = (uint32)ftell(f);
		THROW_SL(fwrite(&t, sizeof(t), 1, f) == 1);
		THROW_SL(fwrite(Name, sizeof(Name), 1, f) == 1);
		l = (uint32)sstrlen(P_Descript);
		THROW_SL(fwrite(&l, sizeof(l), 1, f) == 1);
		if(l)
			THROW_SL(fwrite(P_Descript, (size_t)l, 1, f) == 1);
		THROW(pStrg->AddSymb(this, offs));
	}
	CATCH
		ok = 0;
		SLibError = SLERR_WRITEFAULT;
	ENDCATCH
	return ok;
}

// static
uint16 SLAPI DL2_Entry::ReadEntryType(FILE * pStream)
{
	uint32 t;
	if(fread(&t, sizeof(t), 1, pStream) == 1) {
		fseek(pStream, -((long)sizeof(t)), SEEK_CUR);
		return (uint16)t;
	}
	else
		return PPSetError(PPERR_DL200_READFAULT);
}

int SLAPI DL2_Entry::Read(FILE * pStream)
{
	int    ok = 1;
	if(oneof3(EntryType, DL2ENT_DATA, DL2ENT_GROUP, DL2ENT_ROW)) { // @v9.9.5
		uint32 t, l, dest_type = EntryType;
		destroy();
		THROW_PP(fread(&t, sizeof(t), 1, pStream) == 1, PPERR_DL200_READFAULT);
		THROW_PP(dest_type == t, PPERR_DL200_INVENTRYTYPE);
		THROW_PP(fread(Name, sizeof(Name), 1, pStream) == 1, PPERR_DL200_READFAULT);
		THROW_PP(fread(&l, sizeof(l), 1, pStream) == 1, PPERR_DL200_READFAULT);
		if(l) {
			THROW_MEM(P_Descript = new char[(size_t)l+1]);
			THROW_PP(fread(P_Descript, (size_t)l, 1, pStream) == 1, PPERR_DL200_READFAULT);
			P_Descript[(size_t)l] = 0;
		}
	}
	CATCH
		ok = 0;
		destroy();
	ENDCATCH
	return ok;
}

int SLAPI DL2_Entry::Print(FILE * pStream) const
{
	if(oneof3(EntryType, DL2ENT_DATA, DL2ENT_GROUP, DL2ENT_ROW)) { // @v9.9.5
		SString temp_buf;
		switch(EntryType) {
			case DL2ENT_ROW:   temp_buf = "ROW";   break;
			case DL2ENT_GROUP: temp_buf = "GROUP"; break;
			case DL2ENT_DATA:  temp_buf = "DATA";  break;
			default:
				(temp_buf = "DL2_ENTRY").CatChar('(').Cat(EntryType).CatChar(')'); break;
		}
		temp_buf.Space().Cat(Name);
		if(P_Descript)
			temp_buf.Space().CatParStr(P_Descript);
		fputs(temp_buf, pStream);
	}
	return 1;
}
//
SLAPI DL2_Row::DL2_Row() : DL2_Entry(DL2ENT_ROW), P_F(0)
{
}

DL2_Row & FASTCALL DL2_Row::operator = (const DL2_Row & s)
{
	DL2_Entry::operator = (s);
	if(s.P_F)
		P_F = new DL2_Formula(*s.P_F);
	return *this;
}

void SLAPI DL2_Row::destroy()
{
	DL2_Entry::destroy();
	ZDELETE(P_F);
}

int SLAPI DL2_Row::Write(DL2_Storage * pStrg) const
{
	int    ok = 1;
	FILE * f = pStrg->P_Stream;
	THROW(DL2_Entry::Write(pStrg));
	if(P_F) {
		THROW(P_F->Write(f));
	}
	else {
		uint32 z = 0;
		SLibError = SLERR_WRITEFAULT;
		THROW_SL(fwrite(&z, sizeof(z), 1, f) == 1);
	}
	CATCHZOK
	return ok;
}

int SLAPI DL2_Row::Read(FILE * pStream)
{
	int    ok = 1;
	DL2_Formula f;
	THROW(DL2_Entry::Read(pStream));
	THROW(f.Read(pStream));
	if(!f.IsEmpty()) {
		THROW_MEM(P_F = new DL2_Formula(f));
	}
	CATCH
		ok = 0;
		destroy();
	ENDCATCH
	return ok;
}

int SLAPI DL2_Row::Print(FILE * pStream) const
{
	DL2_Entry::Print(pStream);
	if(P_F) {
		fprintf(pStream, "\nExpression:\n");
		P_F->Print(pStream);
	}
	return 1;
}
//
SLAPI DL2_Group::DL2_Group() : DL2_Entry(DL2ENT_GROUP)
{
	Items.setFlag(aryEachItem, 0);
}

SLAPI DL2_Group::DL2_Group(uint16 type) : DL2_Entry(type)
{
	Items.setFlag(aryEachItem, 0);
}

uint SLAPI DL2_Group::GetCount() const
{
	return Items.getCount();
}

int SLAPI DL2_Group::AddItem(DL2_Entry * pItem)
{
	return Items.insert(pItem) ? 1 : PPSetErrorSLib();
}

int SLAPI DL2_Group::RemoveAll()
{
	for(uint i = 0; i < Items.getCount(); i++) {
		DL2_Entry * p_entry = GetItem(i);
		if(p_entry->EntryType == DL2ENT_GROUP) {
			((DL2_Group*)p_entry)->destroy();
			delete ((DL2_Group*)p_entry);
		}
		else if(p_entry->EntryType == DL2ENT_ROW) {
			((DL2_Row*)p_entry)->destroy();
			delete ((DL2_Row*)p_entry);
		}
	}
	Items.freeAll();
	return 1;
}

DL2_Entry * SLAPI DL2_Group::GetItem(uint i) const
{
	return (DL2_Entry *)Items.at(i);
}

void SLAPI DL2_Group::destroy()
{
	DL2_Entry::destroy();
	RemoveAll();
}

uint SLAPI DL2_Group::GetMaxNesting() const
{
	uint max_nesting = 0;
	for(uint i = 0; i < GetCount(); i++) {
		DL2_Entry * p_entry = GetItem(i);
		if(p_entry && oneof2(p_entry->EntryType, DL2ENT_GROUP, DL2ENT_DATA)) {
			uint nesting = ((DL2_Group *)p_entry)->GetMaxNesting()+1;
			if(nesting > max_nesting)
				max_nesting = nesting;
		}
	}
	return max_nesting;
}

size_t SLAPI DL2_Group::GetMaxDescriptionSize(uint level) const
{
	size_t max_size = 0;
	if(level == 0) {
		if(EntryType == DL2ENT_GROUP)
			max_size = P_Descript ? (sstrlen(P_Descript)+1) : sizeof(Name);
	}
	else
		for(uint i = 0; i < GetCount(); i++) {
			DL2_Entry * p_entry = GetItem(i);
			if(p_entry && p_entry->EntryType == DL2ENT_GROUP) {
				size_t size = ((DL2_Group *)p_entry)->GetMaxDescriptionSize(level-1);
				if(size > max_size)
					max_size = size;
			}
		}
	return max_size;
}

int SLAPI DL2_Group::Write(DL2_Storage * pStrg) const
{
	int    ok = 1;
	uint32 c = GetCount();
	uint   i;
	FILE * f = pStrg->P_Stream;
	THROW(DL2_Entry::Write(pStrg));
	SLibError = SLERR_WRITEFAULT;
	THROW_SL(fwrite(&c, sizeof(c), 1, f));
	for(i = 0; i < c; i++) {
		DL2_Entry * p_entry = GetItem(i);
		THROW(p_entry->Write(pStrg));
	}
	CATCHZOK
	return ok;
}

int SLAPI DL2_Group::Read(FILE * pStream)
{
	int    ok = 1;
	uint32 c, i = 0;
	THROW(DL2_Entry::Read(pStream));
	THROW_PP(fread(&c, sizeof(c), 1, pStream) == 1, PPERR_DL200_READFAULT);
	for(i = 0; i < c; i++) {
		uint16 type = DL2_Entry::ReadEntryType(pStream);
		THROW(type);
		if(type == DL2ENT_GROUP) {
			DL2_Group * p_group = new DL2_Group;
			THROW_MEM(p_group);
			THROW(p_group->Read(pStream));
			THROW(AddItem(p_group));
		}
		else if(type == DL2ENT_ROW) {
			DL2_Row * p_row = new DL2_Row;
			THROW_MEM(p_row);
			THROW(p_row->Read(pStream));
			THROW(AddItem(p_row));
		}
		else {
			CALLEXCEPT_PP(PPERR_DL200_INVENTRYTYPE);
		}
	}
	CATCH
		ok = 0;
		destroy();
	ENDCATCH
	return ok;
}

int SLAPI DL2_Group::Print(FILE * pStream) const
{
	DL2_Entry::Print(pStream);
	fprintf(pStream, "\n");
	for(uint i = 0; i < GetCount(); i++) {
		DL2_Entry * p_entry = GetItem(i);
		if(p_entry)
			p_entry->Print(pStream);
		fprintf(pStream, "\n");
	}
	return 1;
}
//
SLAPI DL2_Data::DL2_Data() : DL2_Group(DL2ENT_DATA)
{
	P_Columns = 0;
}

SLAPI DL2_Data::~DL2_Data()
{
	DestroyColumns();
}

void SLAPI DL2_Data::destroy()
{
	DL2_Group::destroy();
	DestroyColumns();
}

void SLAPI DL2_Data::DestroyColumns()
{
	if(P_Columns) {
		for(uint i = 0; i < P_Columns->getCount(); i++)
			ZDELETE(((DL2_Column *)P_Columns->at(i))->P_Title);
		ZDELETE(P_Columns);
	}
}

int SLAPI DL2_Data::Write(DL2_Storage * pStrg) const
{
	int    ok = 1;
	uint   i;
	const  uint32 c = SVectorBase::GetCount(P_Columns);
	FILE * f = pStrg->P_Stream;
	THROW(DL2_Group::Write(pStrg));
	SLibError = SLERR_WRITEFAULT;
	THROW_SL(fwrite(&c, sizeof(c), 1, f));
	for(i = 0; i < c; i++) {
		DL2_Column * p_col = (DL2_Column *)P_Columns->at(i);
		DL2_Column temp_col = *p_col;
		temp_col.P_Title = 0;
		SLibError = SLERR_WRITEFAULT;
		THROW_SL(fwrite(&temp_col, sizeof(temp_col), 1, f));
		THROW_SL(WritePStrToFile(p_col->P_Title, f));
	}
	CATCHZOK
	return ok;
}

int SLAPI DL2_Data::Read(FILE * pStream)
{
	int    ok = 1;
	uint32 c, i = 0;
	THROW(DL2_Group::Read(pStream));
	THROW_PP(fread(&c, sizeof(c), 1, pStream), PPERR_DL200_READFAULT);
	if(c) {
		THROW_MEM(P_Columns = new SArray(sizeof(DL2_Column)));
		for(i = 0; i < c; i++) {
			DL2_Column col;
			THROW_PP(fread(&col, sizeof(col), 1, pStream), PPERR_DL200_READFAULT);
			col.P_Title = 0;
			THROW_SL(ReadPStrFromFile(&col.P_Title, pStream));
			THROW_SL(P_Columns->insert(&col));
		}
	}
	CATCH
		ok = 0;
		destroy();
	ENDCATCH
	return ok;
}

int SLAPI DL2_Data::AddColumn(const DL2_Column * pColumn)
{
	if(!P_Columns) {
		P_Columns = new SArray(sizeof(DL2_Column));
		if(!P_Columns)
			return PPSetErrorNoMem();
	}
	if(!P_Columns->insert(pColumn))
		return PPSetErrorSLib();
	return 1;
}

int SLAPI DL2_Data::SearchColumnByName(const char * pName, uint * pPos, DL2_Column * pColumn) const
{
	int    ok = -1;
	uint   pos = 0;
	if(P_Columns && pName)
		for(uint i = 0; ok < 0 && i < P_Columns->getCount(); i++) {
			const DL2_Column * p_col = static_cast<const DL2_Column *>(P_Columns->at(i));
			if(p_col->P_Title && stricmp866(p_col->P_Title, pName) == 0) {
				ASSIGN_PTR(pColumn, *p_col);
				pos = i;
				ok = 1;
			}
		}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

uint SLAPI DL2_Data::GetColumnsCount() const
{
	return SVectorBase::GetCount(P_Columns);
}

const DL2_Column * SLAPI DL2_Data::GetColumn(uint pos) const
{
	return (P_Columns && pos < P_Columns->getCount()) ? static_cast<const DL2_Column *>(P_Columns->at(pos)) : 0;
}

int SLAPI DL2_Data::Print(FILE * pStream) const
{
	char   dummy[4];
	dummy[0] = 0;
	DL2_Group::Print(pStream);
	fprintf(pStream, "\n");
	if(P_Columns) {
		for(uint i = 0; i < GetColumnsCount(); i++) {
			const DL2_Column * p_col = GetColumn(i);
			fprintf(pStream, "<%u %s>\n", p_col->CiType, p_col->P_Title ? p_col->P_Title : dummy);
		}
	}
	return 1;
}
//
//
//
SLAPI DL2_GroupStack::DL2_GroupStack() : SStack(sizeof(void *), /*3,*/aryDataOwner|aryPtrContainer)
{
}
//
//
//
/*static*/DL2_CI * SLAPI DL2_CI::Copy(const DL2_CI * s)
{
	DL2_CI * n = 0;
	if(s->CiType == DL2CIT_STRING)
		n = new(s->GetStr()) DL2_CI;
	else
		n = new DL2_CI;
	if(n)
		memcpy(n, s, s->Size());
	return n;
}

/*static*/DL2_CI * SLAPI DL2_CI::MakeStr(const char * pStr)
{
	uint16 len = static_cast<uint16>(pStr ? sstrlen(pStr)+1 : 0);
	DL2_CI * p_str = new(pStr) DL2_CI;
	if(p_str) {
		p_str->CiType = DL2CIT_STRING;
		p_str->Len = len;
		if(len)
			memcpy(p_str+1, pStr, len);
	}
	else
		PPSetErrorNoMem();
	return p_str;
}

/*static*/void DL2_CI::Destroy(DL2_CI * p)
{
	delete p;
}

void * SLAPI DL2_CI::operator new(size_t sz, const char * pStr)
{
	size_t len = pStr ? sstrlen(pStr)+1 : 0;
	DL2_CI * p = (DL2_CI *)::new char[sz + len];
	if(!p)
		PPSetErrorNoMem();
	IdeaRandMem(p, len);
	return p;
}

int SLAPI DL2_CI::IsConst() const
{
	return (CiType >= DL2CIT_CONST_FIRST_ && CiType <= DL2CIT_CONST_LAST_);
}

void SLAPI DL2_CI::InitOp(uint16 op, uint16 argCount)
{
	CiType = op;
	ArgCount = argCount;
}

void SLAPI DL2_CI::Init(double v)
{
	CiType = DL2CIT_REAL;
	R = v;
}

void SLAPI DL2_CI::Init(long v)
{
	CiType = DL2CIT_INT;
	I = v;
}

void SLAPI DL2_CI::Init(LDATE v)
{
	CiType = DL2CIT_DATE;
	D = v;
}

void SLAPI DL2_CI::Init(LTIME v)
{
	CiType = DL2CIT_TIME;
	T = v;
}

void SLAPI DL2_CI::Init(const DateRange * pV)
{
	CiType = DL2CIT_PERIOD;
	P = *pV;
}

void SLAPI DL2_CI::Init(const DL2_Acc * pV)
{
	CiType = DL2CIT_ACC;
	A = *pV;
}

void SLAPI DL2_CI::Init(const DL2_Score * pV)
{
	CiType = DL2CIT_SCORE;
	Score = *pV;
}

void SLAPI DL2_CI::InitMetavar(long id)
{
	CiType = DL2CIT_METAVAR;
	MvID = id;
}

size_t SLAPI DL2_CI::Size() const
{
	return (CiType == DL2CIT_STRING) ? (sizeof(DL2_CI)+Len) : sizeof(DL2_CI);
}

int SLAPI DL2_CI::GetStr(char * pBuf, size_t bufLen) const
{
	if(Len)
		strnzcpy(pBuf, (const char *)(this+1), bufLen);
	else
		ASSIGN_PTR(pBuf, 0);
	return 1;
}

const char * SLAPI DL2_CI::GetStr() const
{
	return Len ? (const char *)(this+1) : 0;
}

int SLAPI DL2_CI::ToString(char * pBuf, size_t bufLen) const
{
	char   buf[128];
	char * p = buf;
	*p = 0;
	switch(CiType) {
		case DL2CIT_REAL:
			realfmt(R, MKSFMTD(0, 8, NMBF_NOTRAILZ), p);
			break;
		case DL2CIT_INT:
			intfmt(I, MKSFMT(0, 0), p);
			break;
		case DL2CIT_DATE:
			datefmt(&D, MKSFMT(0, DATF_DMY), p);
			break;
		case DL2CIT_TIME:
			timefmt(T, MKSFMT(0, TIMF_HMS), p);
			break;
		case DL2CIT_PERIOD:
			periodfmt(&P, p);
			break;
		case DL2CIT_STRING:
			strnzcpy(p, (const char *)(this+1), sizeof(buf)-sstrlen(buf));
			break;
		case DL2CIT_SCORE:
			{
				SString temp_buf;
				Score.PutToStr(temp_buf);
				temp_buf.CopyTo(p, sizeof(buf)-sstrlen(buf));
			}
			break;
		case DL2CIT_ACC:
			*p++ = '[';
			A.Acc.ToStr(ACCF_DEFAULT, p);
			p += sstrlen(p);
			if(A.Flags & DL2_Acc::fRest)
				*p++ = 'R';
			if(A.Flags & DL2_Acc::fInRest)
				*p++ = 'I';
			if(A.Flags & DL2_Acc::fTurnover)
				*p++ = 'T';
			if(A.Flags & DL2_Acc::fDebit)
				*p++ = 'D';
			if(A.Flags & DL2_Acc::fCredit)
				*p++ = 'C';
			if(A.Flags & DL2_Acc::fSpread)
				*p++ = 'S';
			if(A.Flags & DL2_Acc::fAco1) {
				*p++ = 'O';
				*p++ = '1';
			}
			if(A.Flags & DL2_Acc::fAco2) {
				*p++ = 'O';
				*p++ = '2';
			}
			if(A.Flags & DL2_Acc::fAco3) {
				*p++ = 'O';
				*p++ = '3';
			}
			if(A.GetCorrAco()) {
				*p++ = ':';
				A.CorrAcc.ToStr(ACCF_DEFAULT, p);
				p += sstrlen(p);
				if(A.Flags & DL2_Acc::fCorAco1) {
					*p++ = 'O';
					*p++ = '1';
				}
				if(A.Flags & DL2_Acc::fCorAco2) {
					*p++ = 'O';
					*p++ = '2';
				}
				if(A.Flags & DL2_Acc::fCorAco3) {
					*p++ = 'O';
					*p++ = '3';
				}
			}
			*p++ = ']';
			*p = 0;
			break;
		case DL2CIT_OP_UPLUS:
			*p++ = 'u';
			*p++ = '+';
			*p = 0;
			break;
		case DL2CIT_OP_UMINUS:
			*p++ = 'u';
			*p++ = '-';
			*p = 0;
			break;
		case DL2CIT_OP_PLUS:
			*p++ = '+';
			*p = 0;
			break;
		case DL2CIT_OP_MINUS:
			*p++ = '-';
			*p = 0;
			break;
		case DL2CIT_OP_MULT:
			*p++ = '*';
			*p = 0;
			break;
		case DL2CIT_OP_DIV:
			*p++ = '/';
			*p = 0;
			break;
		default:
			*p++ = '.';
			*p = 0;
			break;
	}
	strnzcpy(pBuf, buf, bufLen);
	return 1;
}
//
//
//
SLAPI DL2_ObjList::DL2_ObjList() : SCollection()
{
}

/*virtual*/void FASTCALL DL2_ObjList::freeItem(void * ptr) { delete static_cast<Item *>(ptr); }

int SLAPI DL2_ObjList::Set(PPID objType, const StringSet * pSs, int32 * pId)
{
	int    ok = 1;
	int32  id = 0;
	if(pSs && pSs->getCount()) {
		size_t added_len = pSs->getDataLen();
		for(uint i = 0; i < getCount(); i++) {
			const  Item * p_item = static_cast<const Item *>(at(i));
			size_t len = p_item->Ss.getDataLen();
			if(p_item->ObjType == objType && len == added_len && memcmp(p_item->Ss.getBuf(), pSs->getBuf(), len) == 0) {
				id = p_item->Id;
				break;
			}
		}
	}
	else {
		for(uint i = 0; i < getCount(); i++) {
			const Item * p_item = static_cast<const Item *>(at(i));
			if(p_item->ObjType == objType && p_item->Ss.getDataLen() == 0) {
				id = p_item->Id;
				break;
			}
		}
	}
	if(!id) {
		Item * p_item = new Item;
		if(pSs && pSs->getCount())
			p_item->Ss = *pSs;
		id = (int32)SLS.GetSequenceValue();
		p_item->Id = id;
		p_item->ObjType = objType;
		insert(p_item);
	}
	ASSIGN_PTR(pId, id);
	return ok;
}

/*static*/int FASTCALL DL2_ObjList::GetObjToken(PPID objType, SString & rToken)
{
	int    ok = 1;
	if(objType == PPOBJ_LOCATION)
		rToken = "loc";
	else if(objType == PPOBJ_GOODSGROUP)
		rToken = "goodsgroup";
	else {
		rToken.Z();
		ok = 0;
	}
	return ok;
}

int SLAPI DL2_ObjList::ToString(int32 id, SString & rBuf) const
{
	int    ok = -1;
	SString temp_buf;
	rBuf.Z();
	for(uint i = 0; ok < 0 && i < getCount(); i++) {
		const Item * p_item = static_cast<const Item *>(at(i));
		if(p_item->Id == id) {
			int    is_first = 1;
			int    is_list = 0;
			if(p_item->Ss.getCount() > 0) {
				GetObjToken(p_item->ObjType, temp_buf);
				rBuf.CatChar('@').Cat(temp_buf).CatChar('(');
				is_list = 1;
			}
			for(uint p = 0; p_item->Ss.get(&p, temp_buf);) {
				if(!is_first)
					rBuf.CatDiv(',', 0);
				rBuf.Cat(temp_buf);
				is_first = 0;
			}
			if(is_list)
				rBuf.CatChar(')');
			ok = 1;
		}
	}
	return ok;
}

int SLAPI DL2_ObjList::FromString(const char * pStr, PPID & rObjType, int32 * pId)
{
	int    ok = -1;
	int32  id = 0;
	PPID   obj_type = 0;
	SString temp_buf;
	StringSet obj_list;
	SStrScan scan(pStr);
	scan.Skip();
	if(scan[0] == '@') {
		scan.Incr();
		THROW_PP_S(scan.GetIdent(temp_buf), PPERR_DL200_OBJSYMBPFXEXPECTED, scan);
		if(temp_buf.IsEqiAscii("loc") || temp_buf.IsEqiAscii("loclist")) {
			obj_type = PPOBJ_LOCATION;
		}
		else if(temp_buf.IsEqiAscii("goodsgroup")) {
			obj_type = PPOBJ_GOODSGROUP;
		}
		else {
			CALLEXCEPT_PP_S(PPERR_DL200_INVOBJSYMBPFX, temp_buf);
		}
		if(obj_type) {
			scan.Skip();
			THROW_PP_S(scan[0] == '(', PPERR_DL200_LPAREXPECTED, scan);
			THROW_PP_S(scan.SearchChar(')'), PPERR_DL200_RPARABS, scan);
			scan.Get(temp_buf);
			scan.IncrLen();
			temp_buf.Strip().TrimRightChr(')').ShiftLeftChr('(').Strip();
			{
				StringSet ss(',', temp_buf);
				for(uint p = 0; ss.get(&p, temp_buf);)
					obj_list.add(temp_buf.Strip());
			}
			ok = 1;
		}
	}
	else if(rObjType != 0) {
		obj_type = rObjType;
		temp_buf = pStr;
		obj_list.add(temp_buf.Strip());
		ok = 1;
	}
	if(ok > 0) {
		obj_list.sort();
		THROW(Set(obj_type, &obj_list, &id));
	}
	CATCHZOK
	rObjType = obj_type;
	ASSIGN_PTR(pId, id);
	return ok;
}
//
//
//
void SLAPI DL2_Score::Helper_Init()
{
	DL2_Resolver * p_save_ctx = P_Ctx;
	THISZERO();
	P_Ctx = p_save_ctx;
}

void SLAPI DL2_Score::Init(DL2_Resolver * pCtx)
{
	Helper_Init();
	P_Ctx = pCtx;
}

int SLAPI DL2_Score::ScanArg(SStrScan & rScan, SString & rBuf)
{
	int    ok = -1;
	int    par = 0; // Счетчик внутренних скобок
	rBuf.Z();
	rScan.Skip();
	do {
		char c = rScan[0];
		if(c == '(')
			++par;
		else if(c == ')') {
			if(par)
				--par;
			else
				ok = 2;
		}
		else if(c == ',') {
			if(!par)
				ok = 1;
		}
		else if(c == 0)
			ok = 0;
		if(ok < 0)
			rBuf.CatChar(c);
		rScan.Incr();
	} while(ok < 0);
	return ok;
}

int SLAPI DL2_Score::ScanArgList(const char * pStr, size_t * pOffs)
{
	int    ok = 1;
	SString temp_buf;
	SStrScan scan(pStr);
	scan.Skip();
	if(scan[0] == '(') {
		StringSet arg_list;
		scan.Incr();
		int    r = 1;
		while(r == 1) {
			r = ScanArg(scan, temp_buf);
			if(r > 0)
				arg_list.add(temp_buf);
		}
		uint   arg_no = 0;
		for(uint p = 0; arg_list.get(&p, temp_buf); arg_no++) {
			if(arg_no == 0) {
				THROW_SL(strtoperiod(temp_buf.Strip(), &Period, 0));
			}
			else if(arg_no == 1) {
				if(oneof5(Kind, kBill, kPaym, kPersonEvent, kDebt, kBizScore)) {
					temp_buf.Strip().CopyTo(OpCode, sizeof(OpCode));
				}
				else if(oneof2(Kind, kCCheck, kGoodsRest)) {
					PPID   obj_type = PPOBJ_LOCATION;
					int32  list_id = 0;
					THROW(P_Ctx);
					THROW(P_Ctx->Oc.FromString(temp_buf, obj_type, &list_id));
					if(obj_type == PPOBJ_LOCATION) {
						LocListID = list_id;
					}
					else if(obj_type == PPOBJ_GOODSGROUP) {
						GoodsGrpListID = list_id;
					}
				}
			}
			else if(arg_no == 2) {
				if(oneof3(Kind, kBill, kPaym, kPersonEvent)) {
					PPID   obj_type = PPOBJ_LOCATION;
					int32  list_id = 0;
					THROW(P_Ctx);
					THROW(P_Ctx->Oc.FromString(temp_buf, obj_type, &list_id));
					if(obj_type == PPOBJ_LOCATION) {
						LocListID = list_id;
					}
					else if(obj_type == PPOBJ_GOODSGROUP) {
						GoodsGrpListID = list_id;
					}
				}
				else if(oneof2(Kind, kCCheck, kGoodsRest)) {
					PPID   obj_type = PPOBJ_GOODSGROUP;
					int32  list_id = 0;
					THROW(P_Ctx);
					THROW(P_Ctx->Oc.FromString(temp_buf, obj_type, &list_id));
					if(obj_type == PPOBJ_LOCATION) {
						LocListID = list_id;
					}
					else if(obj_type == PPOBJ_GOODSGROUP) {
						GoodsGrpListID = list_id;
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		// BizScore: ошибка разрешения списка параметров
	ENDCATCH
	ASSIGN_PTR(pOffs, scan.Offs);
	return ok;
}

int SLAPI DL2_Score::GetFromStr(const char * pStr, size_t * pOffs)
{
	int    ok = 1;
	size_t offs = 0;
	Helper_Init();
	SStrScan scan(pStr);
	scan.Skip();
	if(scan[0] == '@') {
		scan.Incr();
		SString temp_buf;
		if(scan.GetIdent(temp_buf)) {
			if(temp_buf.IsEqiAscii("bill")) {
				Kind = kBill;
			}
			else if(temp_buf.IsEqiAscii("paym")) {
				Kind = kPaym;
			}
			else if(temp_buf.IsEqiAscii("ccheck")) {
				Kind = kCCheck;
			}
			else if(temp_buf.IsEqiAscii("goodsrest")) {
				Kind = kGoodsRest;
			}
			else if(temp_buf.IsEqiAscii("personevent")) {
				Kind = kPersonEvent;
			}
			else if(temp_buf.IsEqiAscii("debt")) {
				Kind = kDebt;
			}
			else if(temp_buf.IsEqiAscii("bizscore")) {
				Kind = kBizScore;
			}
			else {
				// BizScore: Не определенный примитив '%s'
			}
			scan.Skip();
			if(scan[0] == '.') {
				scan.Incr();
				THROW(scan.GetIdent(temp_buf.Z()));
				if(temp_buf.IsEqiAscii("amount")) {
					Sub = subAmount;
				}
				else if(temp_buf.IsEqiAscii("cost")) {
					Sub = subCost;
				}
				else if(temp_buf.IsEqiAscii("price")) {
					Sub = subPrice;
				}
				else if(temp_buf.IsEqiAscii("discount")) {
					Sub = subDiscount;
				}
				else if(temp_buf.IsEqiAscii("netprice")) {
					Sub = subNetPrice;
				}
				else if(temp_buf.IsEqiAscii("margin")) {
					Sub = subMargin;
				}
				else if(temp_buf.IsEqiAscii("pctincome")) {
					Sub = subPctIncome;
				}
				else if(temp_buf.IsEqiAscii("pctmargin")) {
					Sub = subPctMargin;
				}
				else if(temp_buf.IsEqiAscii("count")) {
					Sub = subCount;
				}
				else if(temp_buf.IsEqiAscii("average")) {
					Sub = subAverage;
				}
				else {
					CALLEXCEPT(); // @error
				}
			}
			offs = scan.Offs;
			THROW(ScanArgList(scan, &offs));
		}
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pOffs, offs);
	return ok;
}

int SLAPI DL2_Score::PutToStr(SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	switch(Kind) {
		case kBill: rBuf.Cat("@bill"); break;
		case kPaym: rBuf.Cat("@paym"); break;
		case kCCheck: rBuf.Cat("@ccheck"); break;
		case kGoodsRest: rBuf.Cat("@goodsrest"); break;
		case kPersonEvent: rBuf.Cat("@personevent"); break;
		case kDebt: rBuf.Cat("@debt"); break;
		case kBizScore: rBuf.Cat("@bizscore"); break;
		case 0: rBuf.Cat("@none"); break;
		default: rBuf.CatEq("@invalid", (long)Kind); break;
	}
	if(Sub) {
		rBuf.Dot();
		switch(Sub) {
			case subAmount:         rBuf.Cat("amount"); break;
			case subCost:           rBuf.Cat("cost"); break;
			case subPrice:          rBuf.Cat("price"); break;
			case subDiscount:       rBuf.Cat("discount"); break;
			case subNetPrice:       rBuf.Cat("netprice"); break;
			case subMargin:         rBuf.Cat("margin"); break;
			case subPctIncome:      rBuf.Cat("pctincome"); break;
			case subPctMargin:      rBuf.Cat("pctmargin"); break;
			case subCount:          rBuf.Cat("count"); break;
			case subAverage:        rBuf.Cat("average"); break;
			default: rBuf.CatEq("ERROR", (long)Sub); break;
		}
	}
	rBuf.CatChar('(');
	{
		char   pb[64];
		periodfmt(&Period, pb);
		rBuf.Cat(pb);
	}
	if(oneof5(Kind, kBill, kPaym, kPersonEvent, kDebt, kBizScore)) {
		rBuf.CatDiv(',', 2).Cat(OpCode);
	}
	{
		SString list_buf;
		if(LocListID) {
			list_buf.Z();
			if(P_Ctx) {
				if(!P_Ctx->Oc.ToString(LocListID, list_buf))
					(list_buf = "@loclist").CatParStr("ERROR");
			}
			else
				(list_buf = "@loclist").CatParStr("ERROR");
			rBuf.CatDiv(',', 2).Cat(list_buf);
		}
		if(GoodsGrpListID) {
			list_buf.Z();
			if(P_Ctx) {
				if(!P_Ctx->Oc.ToString(GoodsGrpListID, list_buf))
					(list_buf = "@goodsgroup").CatParStr("ERROR");
			}
			else
				(list_buf = "@goodsgroup").CatParStr("ERROR");
			rBuf.CatDiv(',', 2).Cat(list_buf);
		}
	}
	rBuf.CatChar(')');
	return ok;
}

/*
// @loclist(...)
// @bill(period, op, [loc])
// @paym(period, op, [loc]) // op - операция, к которой привязаны оплаты
// @personevent(period, op)
// @ccheck(period, [loc])
// @goodsrest.cost(period, [loc])

const char * p_test_score[] = {
	"@bill(@, sell)",
	"@paym(1/@/@..@, rcv, warehouse02)",
	"@ccheck(1/@-1/@..31/@-1/@)",
	"@ccheck.count(1/@-1/@..31/@-1/@)",
	"@personevent(1/@/@..@, call_in)",
	"@goodsrest.count(@)",
	"@goodsrest.count(@, @loclist(store01, store02))"
};

const char * p_test_score_err[] = {
	"@abracadabr(1/@/@..@, rcv)",
	"@goodsrest.account(@, store01)"
};

*/
//
//
//
void SLAPI DL2_Acc::Init()
{
	THISZERO();
}

int SLAPI DL2_Acc::GetAco() const
{
	if(Flags & fAco1) return ACO_1;
	if(Flags & fAco2) return ACO_2;
	if(Flags & fAco3) return ACO_3;
	return 0;
}

int SLAPI DL2_Acc::GetCorrAco() const
{
	if(Flags & fCorAco1) return ACO_1;
	if(Flags & fCorAco2) return ACO_2;
	if(Flags & fCorAco3) return ACO_3;
	return 0;
}

int SLAPI DL2_Acc::GetAcc(char ** ptr, int isCorr, int substAr)
{
	int    ok = 1;
	int    i = 0, a = 0;
	char   acc_number[64];
	int    tok[3];
	size_t ap = 0;
	Acct * p_acct = isCorr ? &CorrAcc : &Acc;
	char * p_code = isCorr ? Code : CorrCode;
	char * p = *ptr;

	char   dot[2];
	dot[0] = '.';
	dot[1] = 0;

	if(isdec(*p)) {
		int aster_count = 0;
		while(oneof3(*p, '.', ' ', '*') || isdec(*p))
			acc_number[ap++] = *p++;
		acc_number[ap] = 0;
		strip(acc_number);
		char * d;
		if((d = strtok(acc_number, dot)) != 0)
			do {
				while(*d == ' ' || *d == '\t')
					d++;
				if(d[0] == '*') {
					aster_count++;
					THROW_PP_S(aster_count == 1, PPERR_DL200_DUPASTER, *ptr);
					THROW_PP_S(substAr >= 0, PPERR_DL200_ASTERNOTALLOWED, *ptr);
					tok[i++] = substAr;
				}
				else
					tok[i++] = atoi(d);
			} while(i < 3 && (d = strtok(0, dot)) != 0);
	}
	else if(*p == '~') {
		p++;
		while(isalnum(*p) || *p == '.')
			acc_number[ap++] = *p++;
		acc_number[ap] = 0;
		strnzcpy(p_code, strip(acc_number), sizeof(Code)); // @v10.3.4 @fix STRNSCPY(ptr)-->strnzcpy(ptr)
	}
	while(i < 3)
		tok[i++] = -1;
	if(chkch(&p, 'O', &Flags, 0))
		if(isCorr) {
			THROW_PP_S(chkch(&p, '1', &Flags, fCorAco1) ||
				chkch(&p, '2', &Flags, fCorAco2) || chkch(&p, '3', &Flags, fCorAco3), PPERR_DL200_INVACCORD, *ptr)
			a = BIN(Flags&fCorAco1) + BIN(Flags&fCorAco2) + BIN(Flags&fCorAco3);
		}
		else {
			THROW_PP_S(chkch(&p, '1', &Flags, fAco1) ||
				chkch(&p, '2', &Flags, fAco2) || chkch(&p, '3', &Flags, fAco3), PPERR_DL200_INVACCORD, *ptr);
			a = BIN(Flags&fAco1) + BIN(Flags&fAco2) + BIN(Flags&fAco3);
		}
	THROW_PP_S(a <= 1, PPERR_DL200_MOREACCORD, *ptr);
	if(a == 0) {
		if(tok[1] == -1)
			Flags |= (isCorr ? fCorAco1 : fAco1);
		else if(tok[2] == -1)
			Flags |= (isCorr ? fCorAco2 : fAco2);
		else
			Flags |= (isCorr ? fCorAco3 : fAco3);
	}
	p_acct->ac = (tok[0] == -1) ? 0 : tok[0];
	p_acct->sb = (tok[1] == -1) ? 0 : tok[1];
	p_acct->ar = (tok[2] == -1) ? 0 : tok[2];
	*ptr = p;
	CATCHZOK
	return ok;
}

int SLAPI DL2_Acc::GetFromStr(const char * pStr, int substAr)
{
	Init();

	int    ok = 1;
	char   s[64], dot[2], *p;
	dot[0] = '.';
	dot[1] = 0;
	p = skipws(STRNSCPY(s, pStr));
	if(*p == '[') {
		p++;
		p = skipws(p);
	}
	GetAcc(&p, 0, substAr);
	while(chkch(&p, 'D', &Flags, fDebit) || chkch(&p, 'C', &Flags, fCredit) || chkch(&p, 'R', &Flags, fRest) ||
		chkch(&p, 'I', &Flags, fInRest) || chkch(&p, 'T', &Flags, fTurnover) || chkch(&p, 'S', &Flags, fSpread))
		;
	p = skipws(p);
	if(*p == ':') {
		p++;
		GetAcc(&p, 1, substAr);
	}
	p = skipws(p);
	if(*p == ',') {
		int    minus = 0;
		char   number[32], * p_n = number;
		p++;
		if(*p == '-') {
			minus = 1;
			p++;
		}
		while(isdec(*p))
			*p_n++ = *p++;
		*p_n = 0;
		PrdOfs = PRD_DAY;
		NumPrdOfs = minus ? -atoi(number) : atoi(number);
		if(toupper(*p) == 'D')
			PrdOfs = PRD_DAY;
		else if(toupper(*p) == 'W')
			PrdOfs = PRD_WEEK;
		else if(toupper(*p) == 'M')
			PrdOfs = PRD_MONTH;
		else if(toupper(*p) == 'Q')
			PrdOfs = PRD_QUART;
		else if(toupper(*p) == 'S')
			PrdOfs = PRD_SEMIAN;
		else if(toupper(*p) == 'Y')
			PrdOfs = PRD_ANNUAL;
	}
	if(Flags & fDebit && Flags & fCredit)
		ok = 0;
	int    a = BIN(Flags&fRest) + BIN(Flags&fInRest) + BIN(Flags&fTurnover);
	if(a > 1)
		ok = 0;
	return ok;
}
//
// Implementation of DL2_Storage
//
IMPL_CMPFUNC(DL2_IndexItem, i1, i2)
{
	const DL2_Storage::IndexItem * e1 = static_cast<const DL2_Storage::IndexItem *>(i1);
	const DL2_Storage::IndexItem * e2 = static_cast<const DL2_Storage::IndexItem *>(i2);
	int    r = (int)(e1->EntryType - e2->EntryType);
	if(r == 0)
		r = stricmp866(e1->Name, e2->Name);
	return r;
}

#ifndef DL200C // {
/*static*/int SLAPI DL2_Storage::IsDL200File(const char * pFileName)
{
	int    ok = -1;
	Header hdr;
	FILE * stream = 0;
	THROW_SL(fileExists(pFileName));
	SLibError = SLERR_OPENFAULT;
	THROW_SL(stream = fopen(pFileName, "rb"));
	MEMSZERO(hdr);
	PPSetAddedMsgString(pFileName);
	THROW_PP(fread(&hdr, sizeof(hdr), 1, stream) == 1, PPERR_DL200_READFAULT);
	if(hdr.Signature == DL200_SIGNATURE)
		ok = 1;
	CATCHZOK
	SFile::ZClose(&stream);
	return ok;
}
#endif // } DL200C

SLAPI DL2_Storage::DL2_Storage()
{
	P_Stream = 0;
	RO_Mode  = 0;
	FileName[0] = 0;
	P_Index = 0;
}

SLAPI DL2_Storage::~DL2_Storage()
{
	SFile::ZClose(&P_Stream);
	delete P_Index;
}

int SLAPI DL2_Storage::Open(const char * pFileName, int readOnly)
{
	int    ok = 1;
	Close();
	if(readOnly) {
		RO_Mode = 1;
		SLibError = SLERR_FILENOTFOUND;
		PPSetAddedMsgString(pFileName);
		THROW_SL(fileExists(pFileName));
		SLibError = SLERR_OPENFAULT;
		THROW_SL(P_Stream = fopen(pFileName, "rb"));
		MEMSZERO(Head);
		THROW(ReadHeader());
		//Head.Signature must be == DL200_SIGNATURE;
		//Head.DL200_Ver must be == DL200_VERSION;
		THROW(ReadIndex());
	}
	else {
		RO_Mode = 0;
		PPSetAddedMsgString(pFileName);
		SLibError = SLERR_OPENFAULT;
		THROW_SL(P_Stream = fopen(pFileName, "w+b"));
		MEMSZERO(Head);
		Head.Signature = DL200_SIGNATURE;
		Head.DL200_Ver = DL200_VERSION;
		THROW(WriteHeader());
	}
	STRNSCPY(FileName, pFileName);
	CATCH
		ok = 0;
		Close();
	ENDCATCH
	return ok;
}

int SLAPI DL2_Storage::Close()
{
	int    ok = 1;
	if(P_Stream) {
		if(!RO_Mode)
			ok = (WriteIndex() && WriteHeader());
		SFile::ZClose(&P_Stream);
	}
	ZDELETE(P_Index);
	FileName[0] = 0;
	return ok;
}

int SLAPI DL2_Storage::GetDataEntriesList(SStrCollection * pList)
{
	IndexItem * p_item;
	pList->freeAll();
	if(P_Index)
		for(uint i = 0; P_Index->enumItems(&i, (void **)&p_item) > 0;)
			if(p_item->EntryType == DL2ENT_DATA)
				if(!pList->insert(newStr(p_item->Name)))
					return PPSetErrorSLib();
	return pList->getCount() ? 1 : -1;
}

int SLAPI DL2_Storage::AddSymb(const DL2_Entry * pEntry)
{
	return AddSymb(pEntry, 0);
}

int SLAPI DL2_Storage::AddSymb(const DL2_Entry * pEntry, uint32 offs)
{
	int    ok = 1;
	if(pEntry->Name[0] && !(pEntry->Flags & DL2_Entry::fRef)) {
		uint32 lu_offs = 0;
		uint   pos = 0;
		if(LookupSymb(pEntry->EntryType, pEntry->Name, &pos, &lu_offs) > 0) {
			if(lu_offs == 0 && offs != 0)
				((IndexItem *)P_Index->at(pos))->Offs = offs;
		}
		else {
			IndexItem idxitem;
			MEMSZERO(idxitem);
			idxitem.EntryType = pEntry->EntryType;
			STRNSCPY(idxitem.Name, pEntry->Name);
			idxitem.Offs = offs;
			if(!P_Index)
				THROW_MEM(P_Index = new SArray(sizeof(IndexItem)));
			THROW_SL(P_Index->ordInsert(&idxitem, 0, PTR_CMPFUNC(DL2_IndexItem)));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI DL2_Storage::LookupSymb(uint type, const char * pName, uint * pPos, uint32 * pOffs)
{
	int    ok = -1;
	uint   pos = 0;
	uint32 offs = 0;
	if(P_Index) {
		IndexItem idxitem;
		MEMSZERO(idxitem);
		idxitem.EntryType = type;
		STRNSCPY(idxitem.Name, pName);
		if(P_Index->bsearch(&idxitem, &pos, PTR_CMPFUNC(DL2_IndexItem))) {
			offs = ((IndexItem *)P_Index->at(pos))->Offs;
			ok = 1;
		}
	}
	if(ok <= 0) {
		PPSetAddedMsgString(pName);
		PPErrCode = PPERR_DL200_SYMBNFOUND;
	}
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pOffs, offs);
	return ok;
}

int SLAPI DL2_Storage::CheckDupSymb(uint type, const char * pName)
{
	int    ok = 1;
	if(LookupSymb(type, pName, 0, 0) > 0) {
		PPSetAddedMsgString(pName);
		PPErrCode = PPERR_DL200_DUPSYMB;
		ok = 0;
	}
	return ok;
}

int SLAPI DL2_Storage::WriteEntry(const DL2_Entry * pEntry)
{
	int    ok = 1;
	THROW_PP(P_Stream, PPERR_DL200_FILENOPENED);
	THROW_PP(RO_Mode == 0, PPERR_DL200_READONLYMODE);
	THROW(pEntry->Write(this));
	CATCHZOK
	return ok;
}

int SLAPI DL2_Storage::_ReadEntry(uint type, const char * pName, int hdrOnly, DL2_Entry * pEntry)
{
	int    ok = 1;
	uint32 offs;
	long   save_offs = ftell(P_Stream);
	THROW_PP(P_Stream, PPERR_DL200_FILENOPENED);
	THROW(LookupSymb(type, pName, 0, &offs) > 0);
	fseek(P_Stream, offs, SEEK_SET);
	if(hdrOnly) {
		THROW(pEntry->DL2_Entry::Read(P_Stream));
	}
	else {
		THROW(pEntry->Read(P_Stream));
	}
	CATCHZOK
	fseek(P_Stream, save_offs, SEEK_SET);
	return ok;
}

int SLAPI DL2_Storage::ReadEntry(uint type, const char * pName, DL2_Entry * pEntry)
{
	return _ReadEntry(type, pName, 0, pEntry);
}

int SLAPI DL2_Storage::ReadEntryHeader(uint type, const char * pName, DL2_Entry * pEntry)
{
	return _ReadEntry(type, pName, 1, pEntry);
}

int SLAPI DL2_Storage::WriteHeader()
{
	int    ok = 1;
	THROW_PP(P_Stream, PPERR_DL200_FILENOPENED);
	THROW_PP(RO_Mode == 0, PPERR_DL200_READONLYMODE);
	rewind(P_Stream);
	PPSetAddedMsgString(FileName);
	THROW_PP(fwrite(&Head, sizeof(Head), 1, P_Stream) == 1, PPERR_DL200_WRITEFAULT);
	CATCHZOK
	return ok;
}

int SLAPI DL2_Storage::ReadHeader()
{
	int    ok = 1;
	THROW_PP(P_Stream, PPERR_DL200_FILENOPENED);
	rewind(P_Stream);
	PPSetAddedMsgString(FileName);
	THROW_PP(fread(&Head, sizeof(Head), 1, P_Stream) == 1, PPERR_DL200_READFAULT);
	CATCHZOK
	return ok;
}

int SLAPI DL2_Storage::WriteIndex()
{
	int   ok = 1;
	ulong offs = ftell(P_Stream);
	THROW_PP(P_Stream, PPERR_DL200_FILENOPENED);
	THROW_PP(RO_Mode == 0, PPERR_DL200_READONLYMODE);
	THROW_SL(WriteSArrayToFile(P_Index, P_Stream));
	Head.IndexOffs = offs;
	CATCH
		ok = 0;
		Head.IndexOffs = 0;
	ENDCATCH
	return ok;
}

int SLAPI DL2_Storage::ReadIndex()
{
	int    ok = 1;
	THROW_PP(P_Stream, PPERR_DL200_FILENOPENED);
	ZDELETE(P_Index);
	THROW_MEM(P_Index = new SArray(sizeof(IndexItem)));
	fseek(P_Stream, Head.IndexOffs, SEEK_SET);
	THROW_SL(ReadSArrayFromFile(P_Index, P_Stream));
	CATCHZOK
	return ok;
}
//
// Implementation of DL2_Formula
//
typedef uint16 dl2_exprsize;

SLAPI DL2_Formula::DL2_Formula()
{
	Count = 0;
	Size = 0;
	P_Stack = 0;
}

SLAPI DL2_Formula::DL2_Formula(const DL2_Formula & s)
{
	P_Stack = 0;
	Copy(&s);
}

SLAPI DL2_Formula::~DL2_Formula()
{
	// no delete P_Stack (use destroy() insteed
}

void SLAPI DL2_Formula::destroy()
{
	Count = 0;
	Size = 0;
	ZFREE(P_Stack);
}

int SLAPI DL2_Formula::IsEmpty() const
{
	return (Count == 0);
}

int SLAPI DL2_Formula::GetCount() const
{
	return Count;
}

int SLAPI DL2_Formula::Write(FILE * pStream) const
{
	int    ok = 1;
	uint32 s = Size;
	uint32 c = Count;
	if(P_Stack) {
		THROW(fwrite(&s, sizeof(s), 1, pStream) == 1);
		THROW(fwrite(&c, sizeof(c), 1, pStream) == 1);
		if(Size)
			THROW(fwrite(P_Stack, Size, 1, pStream) == 1);
	}
	else {
		THROW(fwrite(&(s = 0), sizeof(s), 1, pStream) == 1);
		THROW(fwrite(&(c = 0), sizeof(c), 1, pStream) == 1);
	}
	CATCH
		ok = 0;
		PPSetErrorSLib();
		SLibError = SLERR_WRITEFAULT;
	ENDCATCH
	return ok;
}

int SLAPI DL2_Formula::Read(FILE * pStream)
{
	int    ok = 1;
	uint32 s, c;
	destroy();
	SLibError = SLERR_READFAULT;
	THROW_SL(fread(&s, sizeof(s), 1, pStream) == 1);
	THROW_SL(fread(&c, sizeof(c), 1, pStream) == 1);
	if(s) {
		THROW_MEM(P_Stack = (uint8 *)SAlloc::M((size_t)s));
		SLibError = SLERR_READFAULT;
		THROW_SL(fread(P_Stack, (size_t)s, 1, pStream) == 1);
	}
	else {
		s = 0;
		c = 0;
	}
	Size  = s;
	Count = c;
	SLibError = 0;
	CATCH
		ok = 0;
		destroy();
	ENDCATCH
	return ok;
}

int SLAPI DL2_Formula::Copy(const DL2_Formula * pS)
{
	destroy();
	P_Stack = (uint8 *)SAlloc::M(pS->Size);
	if(P_Stack) {
		memcpy(P_Stack, pS->P_Stack, pS->Size);
		Count = pS->Count;
		Size = pS->Size;
		return 1;
	}
	else
		return PPSetErrorNoMem();
}

int SLAPI DL2_Formula::Push(const void * pSrc, size_t srcSize)
{
	if(srcSize) {
		SETIFZ(Size, sizeof(dl2_exprsize));
		uint8 * p = (uint8 *)SAlloc::R(P_Stack, Size + srcSize);
		if(p) {
			P_Stack = p;
			dl2_exprsize * p_expr_size = (dl2_exprsize *)P_Stack;
			if(Size > sizeof(dl2_exprsize))
				memmove(P_Stack+sizeof(dl2_exprsize)+srcSize, P_Stack+sizeof(dl2_exprsize), Size-sizeof(dl2_exprsize));
			else
				*p_expr_size = 0;
			memmove(P_Stack+sizeof(dl2_exprsize), pSrc, srcSize);
			(*p_expr_size) += (dl2_exprsize)srcSize;
			Size += (uint32)srcSize;
			SETIFZ(Count, 1);
			return 1;
		}
		else {
			if(Size == sizeof(dl2_exprsize))
				Size = 0;
			return PPSetErrorNoMem();
		}
	}
	return -1;
}

int SLAPI DL2_Formula::PushItem(const DL2_CI * pDl2CiObj)
{
	return Push(pDl2CiObj, pDl2CiObj->Size());
}

int SLAPI DL2_Formula::PushExpression(const DL2_Formula * pF)
{
	if(pF && pF->Size > sizeof(dl2_exprsize))
		return Push(pF->P_Stack+sizeof(dl2_exprsize), (pF->Size - sizeof(dl2_exprsize)));
	else
		return -1;
}

int SLAPI DL2_Formula::AddExpression(const DL2_Formula * pF)
{
	int    ok = -1;
	if(pF)
		if(Size <= sizeof(dl2_exprsize))
			ok = Copy(pF);
		else if(pF->Size > sizeof(dl2_exprsize)) {
			uint8 * p = (uint8 *)SAlloc::R(P_Stack, Size + pF->Size);
			if(p) {
				P_Stack = p;
				memmove(P_Stack+Size, pF->P_Stack, pF->Size);
				Size += pF->Size;
				Count++;
				ok = 1;
			}
			else
				ok = PPSetErrorNoMem();
		}
	return ok;
}

int SLAPI DL2_Formula::AddItem(const DL2_CI * pItem)
{
	int    ok = -1;
	if(pItem) {
		if(Size <= sizeof(dl2_exprsize))
			ok = PushItem(pItem);
		else {
			size_t item_size = pItem->Size();
			uint8 * p = (uint8 *)SAlloc::R(P_Stack, Size + item_size);
			if(p) {
				P_Stack = p;
				dl2_exprsize * p_expr_size = (dl2_exprsize *)P_Stack;
				memmove(P_Stack+Size, pItem, item_size);
				Size += (uint32)item_size;
				(*p_expr_size) += (dl2_exprsize)item_size;
				ok = 1;
			}
			else
				ok = PPSetErrorNoMem();
		}
	}
	return ok;
}

const DL2_CI * SLAPI DL2_Formula::Get(size_t pos, size_t * pNextPos) const
{
	DL2_CI * p = 0;
	if(pos < Size) {
		p = (DL2_CI *)(P_Stack+pos);
		ASSIGN_PTR(pNextPos, pos+p->Size());
	}
	return p;
}

const DL2_CI * SLAPI DL2_Formula::GetByN(uint n) const
{
	size_t p = 0;
	if(P_Stack && Size > sizeof(dl2_exprsize)) {
		dl2_exprsize sz = *(dl2_exprsize *)P_Stack;
		p += sizeof(dl2_exprsize);
		for(uint i = 1; i < n; i++) {
			p += ((DL2_CI *)(P_Stack+p))->Size();
			if(p >= sz)
				return 0;
		}
		return (DL2_CI *)(P_Stack+p);
	}
	return 0;
}

DL2_CI * SLAPI DL2_Formula::Calc(int exprNo, DL2_Resolver * pRslvr) const
{
	DL2_CI * p_result = 0;
	uint   p = 0;
	for(int i = 0; i < (int)Count; i++) {
		uint t = sizeof(dl2_exprsize);
		dl2_exprsize sz = *(dl2_exprsize *)(P_Stack + p);
		if(i == exprNo) {
			size_t next_pos = 0;
			return Calc(exprNo, p + t, &next_pos, pRslvr);
		}
		else
			p += (t + sz);
	}
	p_result = new DL2_CI;
	if(p_result)
		memzero(p_result, sizeof(*p_result));
	else
		PPSetErrorNoMem();
	return p_result;
}

DL2_CI * SLAPI DL2_Formula::Calc(int exprNo, size_t pos, size_t * pNextPos, DL2_Resolver * pRslvr) const
{
	DL2_CI * p_result = 0;
	size_t next_pos;
	uint16 cit = 0;
	const  DL2_CI * p = Get(pos, &next_pos);
	THROW(p);
	cit = p->CiType;
	if(p->IsConst())
		p_result = DL2_CI::Copy(p);
	else if(oneof3(cit, DL2CIT_ACC, DL2CIT_METAVAR, DL2CIT_SCORE)) {
		p_result = pRslvr->Resolve(exprNo, p);
	}
	else if(cit >= DL2CIT_OP_FIRST_ && cit <= DL2CIT_OP_LAST_) {
		DL2_Formula params;
		for(uint i = 0; i < p->ArgCount; i++) {
			DL2_CI * p_arg;
			THROW(p_arg = Calc(exprNo, next_pos, &next_pos, pRslvr));
			THROW(params.AddItem(p_arg));
			DL2_CI::Destroy(p_arg);
		}
		p_result = ResolveOp(p, &params);
		params.destroy();
	}
	else
		CALLEXCEPT_PP(PPErrCode = PPERR_DL200_UNDEFCIT);
	THROW(p_result);
	CATCH
		DL2_CI::Destroy(p_result);
		p_result = 0;
	ENDCATCH
	ASSIGN_PTR(pNextPos, next_pos);
	return p_result;
}

int SLAPI DL2_Formula::Print(FILE * pStream) const
{
	uint32 p = 0;
	fprintf(pStream, "formula: count = %u, size = %u\n", Count, Size);
	for(uint32 i = 0; i < Count; i++) {
		uint32 t = sizeof(dl2_exprsize);
		dl2_exprsize sz = *(dl2_exprsize *)(P_Stack + p);
		while(t < sz) {
			char   buf[128];
		    DL2_CI * p_item = (DL2_CI *)(P_Stack + p + t);
		    p_item->ToString(buf, sizeof(buf));
		    fprintf(pStream, "\t%s\n", buf);
			t += (uint32)p_item->Size();
		}
		p += t;
		fprintf(pStream, "\n");
	}
	return 1;
}
//
//
//
#if SLTEST_RUNNING // {

SLTEST_R(DL200_Account)
{
	const char * strings[] = {
		"[90.11CI]",
		"[90.13CI]",
		"[91CSI]",
		"[84CIS]",
		"[99CI]",
		"[90.3DI]",
		"[90.21DI]",
		"[90.71DI]",
		"[91DSI]",
		"[99DI]",
		"[84DSI]",
		"[60CS]",
		"[70CS]",
		"[69CS]",
		"[68CS]",
		"[62.2CS]",
		"[71CS]",
		"[73CS]",
		"[76CS]",
		"[76SC]",
		"[60.2.218]",
		"[60.2.112O3]",
		"[60.3DT]",
		"[60.3TC]",
		"[90.1CT]",
		"[90.3DT]",
		"[90.4DT]",
		"[90.5DT]",
		"[90.2DT:20]",
		"[90.2DT:21]",
		"[90.2DT:23]",
		"[90.2DT:40]",
		"[90.2DT:41.1]",
		"[90.2DT:43]",
		"[90.2DT:45]",
		"[90.6DT:44]",
		"[91.1.28O3CT:73.2]",
		"[90.1CT:76]",
		"[90.1CT:51]",
		"[90.1CT:52]",
		"[91.1CT:76,-1Y]"
	};
	const char * p_test_score[] = {
		"@bill(@, sell)",
		"@paym(1/@/@..@, rcv, warehouse02)",
		"@ccheck(1/@-1/@..31/@-1/@)",
		"@ccheck.count(1/@-1/@..31/@-1/@)",
		"@personevent(1/@/@..@, call_in)",
		"@goodsrest.count(@)",
		"@goodsrest.count(@, @loclist(store01, store02))"
	};
	const char * p_test_score_err[] = {
		"@abracadabr(1/@/@..@, rcv)",
		"@goodsrest.account(@, store01)"
	};
	int    ok = 1;
	uint   i;
	char   temp_buf[256], temp_buf2[256];
	SString out_buf;
	PrcssrDL200 dl2_ctx;
	(out_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("DL200_Account.txt");
	SFile out(out_buf, SFile::mWrite);
	out_buf = 0;
	for(i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
		DL2_CI ci;
		DL2_Acc acc, acc2;
		acc.Init();
		acc.GetFromStr(strings[i]);
		ci.Init(&acc);
		ci.ToString(temp_buf, sizeof(temp_buf));
		acc2.Init();
		acc2.GetFromStr(temp_buf);
		ci.ToString(temp_buf2, sizeof(temp_buf2));
		out_buf.Z().Cat(strings[i]).Tab().Cat(temp_buf).Tab().Cat(temp_buf2).CR();
		out.WriteLine(out_buf);
		SLTEST_CHECK_Z(stricmp(temp_buf, temp_buf2));
	}
	for(i = 0; i < sizeof(p_test_score) / sizeof(p_test_score[0]); i++) {
		DL2_CI ci;
		DL2_Score sc, sc2;
		sc.Init(&dl2_ctx);
		size_t offs = 0;
		sc.GetFromStr(p_test_score[i], &offs);
		ci.Init(&sc);
		ci.ToString(temp_buf, sizeof(temp_buf));
		sc2.Init(&dl2_ctx);
		sc2.GetFromStr(temp_buf, &(offs = 0));
		ci.ToString(temp_buf2, sizeof(temp_buf2));
		out_buf.Z().Cat(strings[i]).Tab().Cat(temp_buf).Tab().Cat(temp_buf2).CR();
		out.WriteLine(out_buf);
		SLTEST_CHECK_Z(stricmp(temp_buf, temp_buf2));
	}
	/*
	CATCH
		CurrentStatus = ok = 0;
	ENDCATCH
	*/
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
