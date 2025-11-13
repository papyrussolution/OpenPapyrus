// CROSSTAB.CPP
// Copyright (c) A.Sobolev 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

Crosstab::CalcSummaryBlock::CalcSummaryBlock(int dir) : Dir(dir), TotalItemPos(0), CtValPos(0), AggrPos(0), AggrFunc(0), P_ExtData(0), CellVal(0.0), Result(0.0)
{
}
//
//
//
Crosstab::Summary::Item::Item(uint32 aggrFldPosMask, const char * pTitle, uint entrySize) : AggrFldPosMask(aggrFldPosMask), Title(pTitle), List(entrySize)
{
}

Crosstab::Summary::Summary(size_t extSize, uint aggrFldCount) : ExtSize(extSize), AggrFldCount(NZOR(aggrFldCount, 1))
{
}

size_t Crosstab::Summary::GetEntrySize() const { return (ExtSize + (AggrFldCount * sizeof(double))); }
uint   Crosstab::Summary::GetCount() const { return Data.getCount(); }
int    Crosstab::Summary::IsAggrField(uint lineN, uint aggrPos) const { return BIN(aggrPos < 32 && lineN < Data.getCount() && (Data.at(lineN)->AggrFldPosMask & (1 << aggrPos))); }

int Crosstab::Summary::AddLine(uint32 aggrFldPosMask, const char * pTitle)
{
	Item * p_item = new Item(aggrFldPosMask, pTitle, (uint)GetEntrySize());
	if(p_item) {
		Data.insert(p_item);
		return 1;
	}
	else
		return 0;
}

int Crosstab::Summary::GetTitle(uint lineN, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 0;
	if(lineN < Data.getCount()) {
		rBuf = Data.at(lineN)->Title;
		ok = 1;
	}
	return ok;
}

double Crosstab::Summary::GetValue(uint lineNo, uint ctValPos, uint aggrPos)
{
	if(lineNo < Data.getCount()) {
		Item * p_item = Data.at(lineNo);
		if(p_item) {
			if(ctValPos >= p_item->List.getCount()) {
				STempBuffer temp_buf(GetEntrySize());
				memzero(temp_buf, temp_buf.GetSize());
				do {
					p_item->List.insert(temp_buf);
				} while(ctValPos >= p_item->List.getCount());
			}
			const uint8 * ptr = static_cast<const uint8 *>(p_item->List.at(ctValPos));
			if(ptr)
				return reinterpret_cast<const double *>(ptr+ExtSize)[aggrPos];
		}
	}
	return 0.0;
}

int Crosstab::Summary::SetValue(uint lineNo, uint ctValPos, uint aggrPos, double val)
{
	if(lineNo < Data.getCount()) {
		Item * p_item = Data.at(lineNo);
		if(p_item) {
			if(ctValPos >= p_item->List.getCount()) {
				STempBuffer temp_buf(GetEntrySize());
				memzero(temp_buf, temp_buf.GetSize());
				do {
					p_item->List.insert(temp_buf);
				} while(ctValPos >= p_item->List.getCount());
			}
			uint8 * ptr = static_cast<uint8 *>(p_item->List.at(ctValPos));
			if(ptr) {
				reinterpret_cast<double *>(ptr+ExtSize)[aggrPos] = val;
				return 1;
			}
		}
	}
	return 0;
}

void * Crosstab::Summary::GetExtPtr(uint lineNo, uint ctValPos)
{
	void * p_result = 0;
	if(lineNo < Data.getCount()) {
		const Item * p_item = Data.at(lineNo);
		if(p_item && ctValPos < p_item->List.getCount())
			p_result = p_item->List.at(ctValPos);
	}
	return p_result;
}

int Crosstab::Summary::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->SerializeBlock(dir, sizeof(AggrFldCount)+sizeof(ExtSize), &AggrFldCount, rBuf, 0));
	if(dir > 0) {
		uint32 c = Data.getCount();
		rBuf.Write(c);
		for(uint i = 0; i < c; i++) {
			const Item * p_item = Data.at(i);
			THROW_SL(rBuf.Write(p_item->AggrFldPosMask));
			THROW_SL(rBuf.Write(p_item->Title));
			THROW_SL(rBuf.Write(&p_item->List, SBuffer::ffAryCount32));
		}
	}
	else if(dir < 0) {
		SString title;
		uint32 c = 0;
		THROW_SL(rBuf.Read(c));
		Data.freeAll();
		for(uint i = 0; i < c; i++) {
			uint32 mask, aggr_fld_pos_mask = 0;
			THROW_SL(rBuf.Read(mask));
			THROW_SL(rBuf.Read(title));
			Item * p_item = new Item(mask, title, (uint)GetEntrySize());
			THROW_MEM(p_item);
			THROW_SL(rBuf.Read(&p_item->List, SBuffer::ffAryCount32));
			THROW_SL(Data.insert(p_item));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
Crosstab::Crosstab() : Flags(0), P_Tbl(0), P_RTbl(0), P_CtValList(0), P_TotalRows(0), P_TotalCols(0)
{
}

Crosstab::~Crosstab()
{
	delete P_CtValList;
	ZDELETE(P_TotalRows);
	ZDELETE(P_TotalCols);
	DestroyTable();
}

int    Crosstab::SetupFixFields(int initialCall) { return -1; }
DBTable * Crosstab::GetResultTable() { return P_RTbl; }
uint   Crosstab::GetAggrCount() const { return AggrFldList.GetCount(); }
uint   Crosstab::GetTotalRowsCount() const { return P_TotalRows ? P_TotalRows->GetCount() : 0; }
uint   Crosstab::GetTotalColsCount() const { return P_TotalCols ? P_TotalCols->GetCount() : 0; }

int Crosstab::SetTable(DBTable * pTbl, const DBField & crssFld)
{
	P_Tbl = pTbl;
	CrssFld = crssFld;
	Flags = 0;
	IdxFldList.Destroy();
	AggrFldList.Destroy();
	AggrFuncList.freeAll();
	AggrFldColNames.freeAll();
	AggrFldFormats.freeAll();
	FixFldList.Z();
	CrssFldList.Destroy();
	ZDELETE(P_CtValList);
	ZDELETE(P_TotalRows);
	ZDELETE(P_TotalCols);
	return 1;
}

int Crosstab::AddIdxField(const DBField & fld) { return IdxFldList.Add(fld); }
int Crosstab::AddFixField(const char * pName, TYPEID type) { return FixFldList.addField(pName, type); }
int Crosstab::AddInheritedFixField(const DBField & fld) { return InhFldList.Add(fld); }

int CDECL Crosstab::SetSortIdx(const char * pFldName, ...)
{
	va_list arg_list;
	va_start(arg_list, pFldName);
	SortIdxList.Z();
	const char * p = pFldName;
	while(p) {
		SortIdxList.add(p);
		p = va_arg(arg_list, const char *);
	}
	va_end(arg_list);
	return 1;
}

int Crosstab::AddAggrField(const DBField & fld, AggrFunc af /*=afSum*/, const char * pColName /*=0*/, long format /*=0*/, long options /*=0*/)
{
	char * p_col_name = newStr(pColName);
	AggrFldColNames.insert(p_col_name);
	AggrFldFormats.Add(format, options, 0);
	AggrFldList.Add(fld);
	AggrFuncList.insert(&af);
	return 1;
}

int  Crosstab::AddTotalRow(const DBFieldList & rAggrFldList, size_t extSize, const char * pTitle)
{
	int    ok = 1;
	uint32 mask = 0;
	for(uint i = 0; i < rAggrFldList.GetCount(); i++) {
		uint pos = 0;
		if(AggrFldList.Search(rAggrFldList.Get(i), &pos)) {
			if(pos < 32)
				mask |= (1 << pos);
		}
	}
	SETIFZ(P_TotalRows, new Summary(extSize, AggrFldList.GetCount()));
	P_TotalRows->AddLine(mask, pTitle);
	return ok;
}

int  Crosstab::AddTotalColumn(const DBField & rAggrFld, size_t extSize, const char * pTitle)
{
	int    ok = 1;
	uint32 mask = 0;
	uint   pos = 0;
	if(AggrFldList.Search(rAggrFld, &pos) && pos < 32)
		mask |= (1 << pos);
	assert(mask);
	SETIFZ(P_TotalCols, new Summary(extSize, AggrFldList.GetCount()));
	P_TotalCols->AddLine(mask, pTitle);
	return ok;
}

int Crosstab::DestroyTable()
{
	if(P_RTbl) {
		char   tbl_name[64], file_name[MAX_PATH];
		STRNSCPY(tbl_name, P_RTbl->GetTableName());
		STRNSCPY(file_name, P_RTbl->GetName());
		ZDELETE(P_RTbl);
		{
			PPSaveErrContext();
			DbProvider * p_dict = CurDict;
			p_dict->DropTable(tbl_name, 1);
			p_dict->DropFile(file_name);
			PPRestoreErrContext();
		}
		return 1;
	}
	else
		return -1;
}

int Crosstab::GetCrossValues(DBTable * pTbl, const DBField & crssFld, STypArray ** ppList)
{
	int    ok = 1;
	STypArray * p_list = 0;
	BtrDbKey temp_key_;
	const  size_t crss_fld_offs = crssFld.getField().Offs;
	BExtQuery q(pTbl, 0);
	DBFieldList flist;
	flist.Add(crssFld);
	q.select(flist);
	THROW_MEM(p_list = new STypArray(crssFld.getField().T, O_ARRAY));
	pTbl->search(0, temp_key_, spFirst);
	for(q.initIteration(false, 0, -1); q.nextIteration() > 0;) {
		const void * p_data_buf = PTR8C(pTbl->getDataBuf()) + crss_fld_offs;
		if(!p_list->search(p_data_buf, 0))
			p_list->insert(p_data_buf);
	}
	p_list->sort();
	CATCH
		ok = 0;
		ZDELETE(p_list);
	ENDCATCH
	delete (*ppList);
	*ppList = p_list;
	return ok;
}

int Crosstab::CreateTable()
{
	/*
	Структура результирующей кросс-таблицы следующая:
		autolong CTID;
		Поля, заданные в списке IdxFldList, копируемые из исходной таблицы;
		Поля, заданные в списке InhFldList, копируемые из исходной таблицы;
		Поля, заданные в списке FixFldList
		Список кросс-таб-полей (по AggrFldList.GetCount() на каждое табулируемое значение)
			Наименования имеют следующую форму: CTFxxyy, где xx - номер табулированного значения,
			yy - номер поля в списке AggrFldList;
		GetTotalColsCount() итоговых поля //
			Наименования имеют следующую форму: CTFTyy, где yy - номер поля в списке AggrFldList;
		? long CTIX if(SortIdxList.getCount() && GetTotalRowsCount())
	*/
	int    ok = 1;
	uint   i, j;
	char   fldname[32];

	DestroyTable();
	Flags &= ~(fHasExtSortField |fHasSortIdx) ;
	THROW(GetCrossValues(P_Tbl, CrssFld, &P_CtValList));

	THROW_MEM(P_RTbl = new DBTable);
	P_RTbl->AddField("CTID", MKSTYPE(S_AUTOINC, 4));
	const uint start_fld_pos = 1;
	const uint idx_fld_count = IdxFldList.GetCount();
	for(i = 0; i < idx_fld_count; i++)
		P_RTbl->AddField(IdxFldList.GetField(i));
	for(i = 0; i < InhFldList.GetCount(); i++)
		P_RTbl->AddField(InhFldList.GetField(i));
	for(i = 0; i < FixFldList.getCount(); i++)
		P_RTbl->AddField(FixFldList[i].Name, FixFldList[i].T);
	for(i = 0; i < P_CtValList->getCount(); i++) {
		for(j = 0; j < AggrFldList.GetCount(); j++) {
			sprintf(fldname, "CTF%02u%02u", i+1, j+1);
			P_RTbl->AddField(fldname, MKSTYPE(S_FLOAT, 8), UNDEF);
		}
	}
	if(P_TotalCols) {
		for(j = 0; j < P_TotalCols->GetCount(); j++) {
			sprintf(fldname, "CTFT%02u", j);
			P_RTbl->AddField(fldname, MKSTYPE(S_FLOAT, 8), UNDEF);
		}
	}
	if(SortIdxList.getCount() && GetTotalRowsCount()) {
		P_RTbl->AddField("CTIX", MKSTYPE(S_INT, 4)); // Поле, используемое как часть
			// сортирующего индекса для смещения итоговых строк в них таблицы просмотра
		Flags |= fHasExtSortField;
	}
	{
		BNKey key;
		key.addSegment(P_RTbl->GetFields().GetFieldByPosition(0U).Id, XIF_EXT);
		P_RTbl->AddKey(key);
		key.setKeyParams(0, 0);
		for(i = 0; i < IdxFldList.GetCount(); i++)
			key.addSegment(P_RTbl->GetFields().GetFieldByPosition(start_fld_pos+i).Id, XIF_EXT | XIF_MOD);
		key.setKeyParams(1, 0);
		P_RTbl->AddKey(key);
		if(SortIdxList.getCount()) {
			SString seg_name;
			if(Flags & fHasExtSortField) {
				uint   fld_pos = 0;
				const  BNField * p_fld = &P_RTbl->GetFields().GetFieldByName("CTIX", &fld_pos);
				THROW(p_fld);
				key.addSegment(p_fld->Id, XIF_EXT | XIF_MOD | XIF_DUP);
			}
			for(uint p = 0; SortIdxList.get(&p, seg_name);) {
				uint   fld_pos = 0;
				const  BNField * p_fld = &P_RTbl->GetFields().GetFieldByName(seg_name, &fld_pos);
				THROW(p_fld);
				key.addSegment(p_fld->Id, XIF_EXT | XIF_MOD | XIF_DUP);
			}
			key.setKeyParams(2, 0);
			P_RTbl->AddKey(key);
			Flags |= fHasSortIdx;
		}
	}
	CurDict->GetUniqueTableName("CT", P_RTbl);
	{
		DbProvider * p_dict = CurDict;
		SString table_name(P_RTbl->GetTableName());
		SString file_name;
		THROW_DB(p_dict->CreateTableSpec(P_RTbl));
		THROW_DB(p_dict->CreateTempFile(table_name, file_name, false));
		ZDELETE(P_RTbl);
		THROW_MEM(P_RTbl = new DBTable(table_name, file_name));
		P_RTbl->SetFlag(XTF_TEMP);
	}
	P_RTbl->AllocateOwnBuffer(-1);
	CrssFldList.Destroy();
	for(i = 0; i < P_CtValList->getCount(); i++)
		for(j = 0; j < AggrFldList.GetCount(); j++) {
			DBField fld;
			if(P_RTbl->getField(GetTabFldPos(i, j), &fld))
				CrssFldList.Add(fld);
		}
	CATCHZOK
	return ok;
}

uint Crosstab::GetFixFieldOffs() const { return (1 + IdxFldList.GetCount() + InhFldList.GetCount()); }
uint Crosstab::GetTabFldPos(uint ctValPos, uint aggrFldPos) const { return (GetFixFieldOffs() + FixFldList.getCount() + ctValPos * AggrFldList.GetCount() + aggrFldPos); }

int Crosstab::CalcSummary(int action, CalcSummaryBlock & rBlk)
{
	if(action == 0) {
		if(rBlk.AggrFunc == afSum)
			rBlk.Result += rBlk.CellVal;
		else if(rBlk.AggrFunc == afCount)
			rBlk.Result += 1.0;
		else
			rBlk.Result += rBlk.CellVal;
	}
	return 1;
}

int Crosstab::SetAggrValues(uint ctValPos)
{
	const uint aggr_count = AggrFldList.GetCount();
	for(uint j = 0; j < aggr_count; j++) {
		char   addendum[256];
		const  uint fld_pos = GetTabFldPos(ctValPos, j);
		double val, radd;
		AggrFldList.GetValue(j, addendum, 0);
		P_RTbl->getFieldValue(fld_pos, &val, 0);
		AggrFunc func = (AggrFunc)AggrFuncList.at(j);
		if(func == afSum) {
			stcast(AggrFldList.GetField(j).T, MKSTYPE(S_FLOAT, 8), addendum, &radd, 0);
			val = val+radd;
		}
		else /*if(func == afCount)*/ {
			val = val+1;
		}
		P_RTbl->setFieldValue(fld_pos, &val);
	}
	return 1;
}

int Crosstab::SetSummaryRows()
{
	int   ok = 1;
	const uint total_rows_count = GetTotalRowsCount();
	if(total_rows_count) {
		const uint ct_count = P_CtValList->getCount();
		const uint aggr_count = AggrFldList.GetCount();
		CalcSummaryBlock csb(0);
		for(csb.CtValPos = 0; csb.CtValPos < ct_count; csb.CtValPos++) {
			for(csb.AggrPos = 0; csb.AggrPos < aggr_count; csb.AggrPos++) {
				size_t sz;
				const  uint   fld_pos = GetTabFldPos(csb.CtValPos, csb.AggrPos);
				double val = 0.0;
				P_RTbl->getFieldValue(fld_pos, &val, &sz);
				csb.CellVal = val;
				for(csb.TotalItemPos = 0; csb.TotalItemPos < total_rows_count; csb.TotalItemPos++) {
					csb.Result    = P_TotalRows->GetValue(csb.TotalItemPos, csb.CtValPos, csb.AggrPos);
					csb.P_ExtData = P_TotalRows->GetExtPtr(csb.TotalItemPos, csb.CtValPos);
					csb.AggrFunc  = afSum;
					CalcSummary(0, csb);
					P_TotalRows->SetValue(csb.TotalItemPos, csb.CtValPos, csb.AggrPos, csb.Result);
				}
			}
		}
	}
	return ok;
}

int Crosstab::SetAggrSummaryValues()
{
	int    ok = 1;
	const  uint total_cols_count = P_TotalCols ? P_TotalCols->GetCount() : 0;
	if(total_cols_count) {
		CalcSummaryBlock csb(1);
		for(csb.TotalItemPos = 0; csb.TotalItemPos < total_cols_count; csb.TotalItemPos++) {
			csb.AggrPos = 0;
			for(uint i = 0; i < AggrFldList.GetCount(); i++) {
				if(P_TotalCols->IsAggrField(csb.TotalItemPos, i)) {
					csb.AggrPos = i;
					break;
				}
			}
			P_TotalCols->SetValue(csb.TotalItemPos, 0, csb.AggrPos, 0.0);
			for(csb.CtValPos = 0; csb.CtValPos < P_CtValList->getCount(); csb.CtValPos++) {
				size_t sz;
				const  uint   fld_pos = GetTabFldPos(csb.CtValPos, csb.AggrPos);
				double val = 0.0;
				P_RTbl->getFieldValue(fld_pos, &val, &sz);
				csb.AggrFunc = AggrFuncList.at(csb.AggrPos);
				csb.CellVal = val;
				csb.Result = P_TotalCols->GetValue(csb.TotalItemPos, 0, csb.AggrPos);
				csb.P_ExtData = P_TotalCols->GetExtPtr(csb.TotalItemPos, 0);
				CalcSummary(0, csb);
				P_TotalCols->SetValue(csb.TotalItemPos, 0, csb.AggrPos, csb.Result);
			}
			csb.Result = P_TotalCols->GetValue(csb.TotalItemPos, 0, csb.AggrPos);
			CalcSummary(1, csb);
			const  uint total_fld_pos = GetTabFldPos(P_CtValList->getCount(), csb.TotalItemPos);
			P_RTbl->setFieldValue(total_fld_pos, &csb.Result);
		}
	}
	else
		ok = -1;
	return ok;
}

int Crosstab::Create(int use_ta)
{
	int    ok = 1;
	uint   i;
	BtrDbKey temp_key_;
	char   buf[512];
	SString msg_buf;
	IterCounter cntr;
	BExtQuery q(P_Tbl, 0);
	DBFieldList flist;
	THROW(CreateTable());
	{
		PPInitIterCounter(cntr, P_Tbl);
		PPLoadText(PPTXT_CROSSTABMAKING, msg_buf);
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		flist.Add(IdxFldList);
		flist.Add(InhFldList);
		flist.Add(AggrFldList);
		flist.Add(CrssFld);
		q.select(flist);
		P_Tbl->search(0, temp_key_, spFirst);
		const uint start_fld_pos = 1;
		const uint idx_fld_count = IdxFldList.GetCount();
		const long zero = 0;
		uint  ext_sort_fld_pos = 0;
		if(Flags & fHasExtSortField && &P_RTbl->GetFields().GetFieldByName("CTIX", &ext_sort_fld_pos) == 0)
			ext_sort_fld_pos = 0;
		for(q.initIteration(false, 0, -1); q.nextIteration() > 0; PPWaitPercent(cntr.Increment(), msg_buf)) {
			uint   ct_val_pos = 0;
			CrssFld.getValue(buf, 0);
			if(P_CtValList->search(buf, &ct_val_pos)) {
				size_t offs = 0, sz;
				for(i = 0; i < idx_fld_count; i++) {
					IdxFldList.GetValue(i, buf+offs, &sz);
					offs += sz;
				}
				if(P_RTbl->searchForUpdate(1, buf, spEq)) {
					SetAggrValues(ct_val_pos);
					SetAggrSummaryValues();
					THROW_DB(P_RTbl->updateRec()); // @sfu
				}
				else {
					P_RTbl->clearDataBuf();
					if(ext_sort_fld_pos)
						P_RTbl->setFieldValue(ext_sort_fld_pos, &zero);
					for(i = 0; i < idx_fld_count; i++) {
						IdxFldList.GetValue(i, buf, &sz);
						P_RTbl->setFieldValue(start_fld_pos+i, buf);
					}
					for(i = 0; i < InhFldList.GetCount(); i++) {
						InhFldList.GetValue(i, buf, &sz);
						P_RTbl->setFieldValue(start_fld_pos+idx_fld_count+i, buf);
					}
					SetAggrValues(ct_val_pos);
					SetAggrSummaryValues();
					THROW_DB(P_RTbl->insertRec());
				}
			}
		}
		{
			PPWaitMsg(msg_buf);
			if(SetupFixFields(1) > 0) {
				if(P_RTbl->searchForUpdate(0, temp_key_, spFirst))
					do {
						int r;
						THROW(r = SetupFixFields(0));
						SetSummaryRows();
						if(r > 0)
							THROW_DB(P_RTbl->updateRec()); // @sfu
					} while(P_RTbl->searchForUpdate(0, temp_key_, spNext));
			}
			else if(GetTotalRowsCount()) {
				if(P_RTbl->search(0, temp_key_, spFirst))
					do {
						SetSummaryRows();
					} while(P_RTbl->search(0, temp_key_, spNext));
			}
		}
		{
			PPWaitMsg(msg_buf);
			const uint total_rows_count = P_TotalRows ? P_TotalRows->GetCount() : 0;
			CalcSummaryBlock csb(0);
			long   ext_sort_val = 0;
			SString title_buf;
			for(csb.TotalItemPos = 0; csb.TotalItemPos < total_rows_count; csb.TotalItemPos++) {
				size_t offs = 0;
				P_RTbl->clearDataBuf();
				if(ext_sort_fld_pos) {
					++ext_sort_val;
					P_RTbl->setFieldValue(ext_sort_fld_pos, &ext_sort_val);
				}
				for(i = 0; i < idx_fld_count; i++) {
					memzero(buf, sizeof(buf));
					const BNField & r_fld = IdxFldList.GetField(i);
					stmaxval(r_fld.T, buf);
					if(total_rows_count > 1 && i == (idx_fld_count-1)) {
						size_t sz = stsize(r_fld.T);
						PTR8(buf)[0] -= (uint8)(total_rows_count - csb.TotalItemPos);
					}
					P_RTbl->setFieldValue(start_fld_pos+i, buf);
				}
				for(i = 0; i < InhFldList.GetCount(); i++) {
					const uint s_type = GETSTYPE(InhFldList.GetField(i).T);
					memzero(buf, sizeof(buf));
					if(i == 0 && s_type == S_ZSTRING) {
						P_TotalRows->GetTitle(csb.TotalItemPos, title_buf);
						title_buf.CopyTo(buf, sizeof(buf));
					}
					P_RTbl->setFieldValue(start_fld_pos+idx_fld_count+i, buf);
				}
				{
					const  uint aggr_count = AggrFldList.GetCount();
					for(csb.CtValPos = 0; csb.CtValPos < P_CtValList->getCount(); csb.CtValPos++) {
						for(uint j = 0; j < aggr_count; j++) {
							const uint fld_pos = GetTabFldPos(csb.CtValPos, j);
							if(P_TotalRows->IsAggrField(csb.TotalItemPos, j)) {
								csb.AggrPos = j;
								csb.Result = P_TotalRows->GetValue(csb.TotalItemPos, csb.CtValPos, j);
								csb.P_ExtData = P_TotalRows->GetExtPtr(csb.TotalItemPos, csb.CtValPos);
								CalcSummary(1, csb);
							}
							else
								csb.Result = 0.0;
							P_TotalRows->SetValue(csb.TotalItemPos, csb.CtValPos, j, csb.Result);
							P_RTbl->setFieldValue(fld_pos, &csb.Result);
						}
					}
					SetAggrSummaryValues();
					THROW_DB(P_RTbl->insertRec());
				}
			}
		}
		tra.Commit();
	}
	CATCHZOK
	return ok;
}

DBQuery * Crosstab::CreateBrowserQuery()
{
	DBQuery * p_q = 0;
	DBFieldList fld_list;
	DBField fld;
	DBTable * p_tbl = new DBTable(P_RTbl->GetTableName(), P_RTbl->GetName());
	if(p_tbl) {
		p_tbl->AllocateOwnBuffer(-1);
		for(uint i = 0; i < p_tbl->GetFields().getCount(); i++) {
			p_tbl->getField(i, &fld);
			fld_list.Add(fld);
		}
		p_q = &select(fld_list).from(p_tbl, 0L);
		if(SortIdxList.getCount()) {
			SString seg_name;
			if(Flags & fHasExtSortField) {
				uint   fld_pos = 0;
				if(&p_tbl->GetFields().GetFieldByName("CTIX", &fld_pos) != 0) {
					p_tbl->getField(fld_pos, &fld);
					p_q->addOrderField(fld);
				}
			}
			for(uint p = 0; SortIdxList.get(&p, seg_name);) {
				uint   fld_pos = 0;
				if(&p_tbl->GetFields().GetFieldByName(seg_name, &fld_pos) != 0) {
					p_tbl->getField(fld_pos, &fld);
					p_q->addOrderField(fld);
				}
			}
		}
	}
	return p_q;
}

void Crosstab::GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
{
	rBuf.Z();
	if(pVal) {
		char   temp_buf[256];
		if(sttostr(typ, pVal, 0, temp_buf))
			rBuf = temp_buf;
	}
}

int Crosstab::Helper_SetupBrowserCtColumn(BrowserWindow * pBrw, uint ctValPos, const SString & rTitle) const
{
	int    ok = 1;
	const  uint aggr_count = AggrFldList.GetCount();
	uint   first_col_in_group = pBrw->getDef()->getCount();
	for(uint j = 0; j < aggr_count; j++) {
		const BroCrosstab * p_bct = pBrw->getDef()->GetCrosstab(j);
		const char * p_title = 0;
		long  fmt     = p_bct ? p_bct->Format : 0;
		uint  options = p_bct ? p_bct->Options : 0;
		if(aggr_count == 1)
			p_title = rTitle.cptr();
		else {
			if(j < AggrFldColNames.getCount())
				p_title = AggrFldColNames.at(j);
			if(p_title == 0 && p_bct)
				p_title = p_bct->P_Text;
			if(j < AggrFldFormats.getCount()) {
				long  added_fmt = AggrFldFormats.at(j).Key, added_opt = AggrFldFormats.at(j).Val;
				fmt     = (added_fmt) ? added_fmt : fmt;
				options = (added_opt) ? (uint)added_opt : options;
			}
		}
		int   fld_no  = GetTabFldPos(ctValPos, j);
		pBrw->insertColumn(-1, p_title, fld_no, 0 /* typ */, fmt, options);
	}
	if(aggr_count > 1) {
		BroGroup grp;
		grp.First = first_col_in_group;
		grp.Count = aggr_count;
		grp.Height = 1;
		grp.P_Text = newStr(rTitle);
		pBrw->getDef()->AddColumnGroup(&grp);
	}
	return ok;
}

int Crosstab::SetupBrowserCtColumns(BrowserWindow * pBrw) const
{
	const uint ct_val_count = P_CtValList->getCount();
	if(ct_val_count) {
		SString title_buf;
		for(uint i = 0; i < ct_val_count; i++) {
			GetTabTitle(P_CtValList->at(i), P_CtValList->getType(), title_buf.Z());
			if(title_buf.IsEmpty())
				P_CtValList->itemToString(i, 0, title_buf);
			Helper_SetupBrowserCtColumn(pBrw, i, title_buf);
		}
		if(P_TotalCols) {
			for(uint j = 0; j < P_TotalCols->GetCount(); j++) {
				const BroCrosstab * p_bct = pBrw->getDef()->GetCrosstab(j);
				P_TotalCols->GetTitle(j, title_buf);
				int   fld_no  = GetTabFldPos(ct_val_count, j);
				long  fmt     = p_bct ? p_bct->Format : 0;
				uint  options = p_bct ? p_bct->Options : 0;
				pBrw->insertColumn(-1, title_buf, fld_no, 0 /* typ */, fmt, options);
			}
		}
		pBrw->CalcRight();
		pBrw->SetupScroll();
	}
	return 1;
}

BrowserWindow * Crosstab::CreateBrowser(uint rezID, int dataOwner)
{
	BrowserWindow * p_brw = new BrowserWindow(rezID, CreateBrowserQuery(), 0);
	SetupBrowserCtColumns(p_brw);
	return p_brw;
}

int Crosstab::GetIdxFields(PPID id, DBFieldList * pFldList)
{
	int    ok = -1;
	if(pFldList) {
		DBField field;
		if(P_RTbl->search(0, &id, spEq) > 0) {
			for(uint i = 0; (i + 1 < P_RTbl->GetFields().getCount()) && i < IdxFldList.GetCount(); i++) {
				P_RTbl->getField(i + 1, &field);
				pFldList->Add(field);
			}
			ok = 1;
		}
	}
	return ok;
}

int Crosstab::GetTab(uint tabIdx, void * pTabVal)
{
	char   buf[512];
	size_t sz = stsize(P_CtValList->getType());
	stmaxval(P_CtValList->getType(), buf);
	return (pTabVal && tabIdx < P_CtValList->getCount() && memcmp(buf, P_CtValList->at(tabIdx), sz) != 0) ?
		(memcpy(pTabVal, P_CtValList->at(tabIdx), sz), 1) : -1;
}

int Crosstab::SetFixFieldValByCTID(long ctID, uint fldPos, const void * pBuf)
{
	int    ok = -1;
	const  int ci = P_RTbl->GetCurIndex();
	DBRowId db_pos;
	P_RTbl->getPosition(&db_pos);
	if(P_RTbl->searchForUpdate(0, &ctID, spEq) > 0) {
		P_RTbl->setFieldValue(GetFixFieldOffs() + fldPos, pBuf);
		P_RTbl->updateRec(); // @sfu
		ok = 1;
	}
	P_RTbl->getDirect(ci, 0, db_pos);
	return ok;
}

int Crosstab::GetFixFieldValByCTID(long ctID, uint fldPos, void * pBuf, size_t bufSize)
{
	int    ok = -1;
	const  int ci = P_RTbl->GetCurIndex();
	if(P_RTbl->search(0, &ctID, spEq) > 0) {
		double val = 0.0;
		P_RTbl->getFieldValue(GetFixFieldOffs() + fldPos, pBuf, &bufSize);
		ok = 1;
	}
	P_RTbl->setIndex(ci);
	return ok;
}

int Crosstab::GetIdxFieldVal(uint idxFldN, const void * pDataBuf, void * pBuf, size_t bufSize)
{
	int    ok = 0;
	if(!pDataBuf)
		pDataBuf = P_RTbl ? P_RTbl->getDataBuf() : 0;
	if(pDataBuf && idxFldN < IdxFldList.GetCount()) {
		long   type_id = IdxFldList.Get(idxFldN).stype();
		size_t sz = GETSSIZE(type_id), offs = sizeof(long);
		long   stype = GETSTYPE(type_id);
		for(uint i = 0; i < idxFldN; i++)
			offs += GETSSIZE(IdxFldList.Get(i).stype());
		if(pBuf)
			memcpy(pBuf, PTR8C(pDataBuf)+offs, MIN(sz, bufSize));
		ok = 1;
	}
	return ok;
}

int Crosstab::GetAggrFieldVal(uint tabIdx, uint aggrFldN, const void * pDataBuf, void * pBuf, size_t bufSize)
{
	int    ok = -1;
	if(pDataBuf && aggrFldN < GetAggrCount()) {
		uint   idx_flds_count  = IdxFldList.GetCount();
		uint   inh_flds_count  = InhFldList.GetCount();
		uint   fix_flds_count  = FixFldList.getCount();
		uint   aggr_flds_count = AggrFldList.GetCount();
		long   type_id = AggrFldList.Get(aggrFldN).stype();
		uint   i;
		size_t sz = sizeof(double); // Так как на данный момент все аггрегированные поля имеют тип double GETSSIZE(type_id);
		size_t offs = sizeof(long);
		for(i = 0; i < idx_flds_count; i++)
			offs += GETSSIZE(IdxFldList.Get(i).stype());
		for(i = 0; i < inh_flds_count; i++)
			offs += GETSSIZE(InhFldList.Get(i).stype());
		for(i = 0; i < fix_flds_count; i++)
			offs += GETSSIZE(FixFldList[i].T);
		offs += (tabIdx * aggr_flds_count + aggrFldN) * sizeof(double); // Так как на данный момент все агрегированные поля имеют тип double
		if(pBuf)
			memcpy(pBuf, PTR8C(pDataBuf)+offs, MIN(sz, bufSize));
		ok = 1;
	}
	return ok;
}

/*
int Crosstab::SetAggrFieldVal(uint tabIdx, uint aggrFldN, void * pBuf, const void * pData, size_t bufSize)
{
	int    ok = -1;
	if(pDataBuf && aggrFldN < GetAggrCount()) {
		uint idx_flds_count  = IdxFldList.GetCount();
		uint inh_flds_count  = InhFldList.GetCount();
		uint fix_flds_count  = FixFldList.getCount();
		uint aggr_flds_count = AggrFldList.GetCount();
		long   type_id = AggrFldList.Get(aggrFldN).stype();
		size_t sz = sizeof(double); // Так как на данный момент все аггрегированные поля имеют тип double GETSSIZE(type_id);
		size_t offs = sizeof(long);
		for(uint i = 0; i < idx_flds_count; i++)
			offs += GETSSIZE(IdxFldList.Get(i).stype());
		for(i = 0; i < inh_flds_count; i++)
			offs += GETSSIZE(InhFldList.Get(i).stype());
		for(i = 0; i < fix_flds_count; i++)
			offs += GETSSIZE(FixFldList[i].type);
		offs += (tabIdx * aggr_flds_count + aggrFldN) * sizeof(double); // Так как на данный момент все аггрегированные поля имеют тип double
		if(pBuf)
			memcpy(PTR8(pBuf)+offs, pData, MIN(sz, bufSize));
		ok = 1;
	}
	return ok;
}
*/

int Crosstab::Write(SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SString temp_buf;
	THROW_SL(pCtx->Serialize(+1, Flags, rBuf));
	THROW_SL(SortIdxList.Write(rBuf));
	THROW_SL(pCtx->Serialize(+1, (temp_buf = P_Tbl ? P_Tbl->GetTableName() : 0), rBuf));
	THROW_SL(pCtx->Serialize(+1, (temp_buf = P_Tbl ? P_Tbl->GetName().cptr() : 0), rBuf));
	THROW_SL(pCtx->Serialize(+1, &AggrFuncList, rBuf));
	THROW_SL(pCtx->Serialize(+1, &AggrFldColNames, rBuf));
	{
		TYPEID tid = P_CtValList ? P_CtValList->getType() : 0;
		THROW_SL(pCtx->Serialize(+1, tid, rBuf));
		if(P_CtValList) {
			THROW_SL(pCtx->Serialize(+1, P_CtValList, rBuf));
		}
	}
	{
		uint8  ind = P_RTbl ? 0 : 1;
		THROW_SL(rBuf.Write(ind));
		if(P_RTbl) {
			THROW(P_RTbl->SerializeSpec(+1, rBuf, pCtx));
		}
		int32 f = CrssFld.fld;
		THROW_SL(pCtx->Serialize(+1, f, rBuf));
		THROW(WriteDbFieldList(IdxFldList, rBuf, pCtx));
		THROW_SL(pCtx->SerializeFieldList(+1, &FixFldList, rBuf));
		THROW(WriteDbFieldList(InhFldList, rBuf, pCtx));
		THROW(WriteDbFieldList(CrssFldList, rBuf, pCtx));
		THROW(WriteDbFieldList(AggrFldList, rBuf, pCtx));
	}
	{
		uint8  ind = P_TotalRows ? 0 : 1;
		THROW_SL(rBuf.Write(ind));
		if(P_TotalRows)
			THROW(P_TotalRows->Serialize(+1, rBuf, pCtx));
		ind = P_TotalCols ? 0 : 1;
		THROW_SL(rBuf.Write(ind));
		if(P_TotalCols)
			THROW(P_TotalCols->Serialize(+1, rBuf, pCtx));
	}
	CATCHZOK
	return ok;
}

int Crosstab::Read(DBTable * pTbl, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SString temp_buf;

	P_Tbl = pTbl;
	THROW_SL(pCtx->Serialize(-1, Flags, rBuf));
	THROW_SL(SortIdxList.Read(rBuf));
	THROW_SL(pCtx->Serialize(-1, temp_buf, rBuf));
	THROW_PP_S(temp_buf.CmpNC(pTbl->GetTableName()) == 0, PPERR_CTSERIALIZE_INVTBLNAME, temp_buf);
	THROW_SL(pCtx->Serialize(-1, temp_buf, rBuf));
	THROW_PP_S(temp_buf.CmpNC(pTbl->GetName()) == 0, PPERR_CTSERIALIZE_INVFILNAME, temp_buf);
	THROW_SL(pCtx->Serialize(-1, &AggrFuncList, rBuf));
	THROW_SL(pCtx->Serialize(-1, &AggrFldColNames, rBuf));
	{
		TYPEID tid = 0;
		ZDELETE(P_CtValList);
		THROW_SL(pCtx->Serialize(-1, tid, rBuf));
		if(tid) {
			THROW_MEM(P_CtValList = new STypArray(tid, O_ARRAY));
			THROW_SL(pCtx->Serialize(-1, P_CtValList, rBuf));
		}
	}
	{
		uint8  ind = 0;
		ZDELETE(P_RTbl);
		THROW_SL(rBuf.Read(ind));
		if(ind == 0) {
			THROW_MEM(P_RTbl = new DBTable);
			THROW(P_RTbl->SerializeSpec(-1, rBuf, pCtx));
			{
				SString table_name(P_RTbl->GetTableName());
				SString file_name = P_RTbl->GetName();
				THROW_DB(CurDict->CreateTableSpec(P_RTbl));
				ZDELETE(P_RTbl);
				THROW_MEM(P_RTbl = new DBTable(table_name, file_name));
			}
		}

		int32 f = 0;
		THROW_SL(pCtx->Serialize(-1, f, rBuf));
		if(P_Tbl) {
			CrssFld.Id = P_Tbl->GetHandle();
			CrssFld.fld = f;
		}
		else {
			CrssFld.Id = 0;
			CrssFld.fld = 0;
		}
		THROW(ReadDbFieldList(P_Tbl, IdxFldList, rBuf, pCtx));
		THROW_SL(pCtx->SerializeFieldList(-1, &FixFldList, rBuf));
		THROW(ReadDbFieldList(P_Tbl, InhFldList, rBuf, pCtx));
		THROW(ReadDbFieldList(P_RTbl, CrssFldList, rBuf, pCtx));
		THROW(ReadDbFieldList(P_Tbl, AggrFldList, rBuf, pCtx));
	}
	{
		uint8  ind = P_TotalRows ? 0 : 1;
		ZDELETE(P_TotalRows);
		ZDELETE(P_TotalCols);
		THROW_SL(rBuf.Read(ind));
		if(ind == 0) {
			THROW_MEM(P_TotalRows = new Summary(0, 0));
			THROW(P_TotalRows->Serialize(-1, rBuf, pCtx));
		}
		THROW_SL(rBuf.Read(ind));
		if(ind == 0) {
			THROW_MEM(P_TotalCols = new Summary(0, 0));
			THROW(P_TotalCols->Serialize(-1, rBuf, pCtx));
		}
	}
	CATCHZOK
	return ok;
}

int Crosstab::WriteDbFieldList(const DBFieldList & rList, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	int32  c = static_cast<int32>(rList.GetCount());
	THROW_SL(pCtx->Serialize(+1, c, rBuf));
	for(int32 i = 0; i < c; i++) {
		int32  f = rList.Get(i).fld;
		THROW_SL(pCtx->Serialize(+1, f, rBuf));
	}
	CATCHZOK
	return ok;
}

int Crosstab::ReadDbFieldList(const DBTable * pTbl, DBFieldList & rList, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	int32  c = 0;
	THROW_SL(pCtx->Serialize(-1, c, rBuf));
	rList.Destroy();
	for(int32 i = 0; i < c; i++) {
		int32  f = 0;
		THROW_SL(pCtx->Serialize(-1, f, rBuf));
		if(pTbl) {
			DBField fld;
			fld.Id = pTbl->GetHandle();
			fld.fld = f;
			rList.Add(fld);
		}
	}
	CATCHZOK
	return ok;
}
