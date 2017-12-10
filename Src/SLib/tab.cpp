// TAB.CPP
// Copyright (c) Sobolev A. 1995-2000, 2003, 2010, 2013, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#if 0 // (moved to sformat.cpp as static) {
struct i_tbl {
	int  a;
	int  b;
};

int SLAPI i_tab(const void * tbl, int req, int def)
{
	const i_tbl * t = (const i_tbl *)tbl;
	for(int i = 1; i <= t[0].a; i++)
		if(t[i].a == req)
			return t[i].b;
	return def;
}

int SLAPI ai_tab(const void * tbl, int req, int def)
{
	const i_tbl * t = (const i_tbl *)tbl;
	for(int i = 1; i <= t[0].a; i++)
		if(t[i].a & req)
			return t[i].b;
	return def;
}
#endif // } 0
//
//
//

//
//
//
#define STAB_ROW_SIGN 0x54425257

SLAPI STab::Row::Row() : Set("^\001")
{
	Sign = STAB_ROW_SIGN;
	Set.add("$."); // Zero position is invalid
}

SLAPI STab::Row::~Row()
{
	Sign = 0;
}

int SLAPI STab::Row::IsConsistent() const
{
	return BIN(Sign == STAB_ROW_SIGN);
}

void SLAPI STab::Row::Clear()
{
	PosList.clear();
	Set.clear();
	Set.add("$."); // Zero position is invalid
}

int FASTCALL STab::Row::Add(const char * pStr)
{
	int    ok = 1;
	uint   pos = 0;
	if(pStr)
		Set.add(pStr, &pos);
	PosList.add((long)pos);
	return ok;
}

int SLAPI STab::Row::Add(double realVal)
{
	SString temp_buf;
	return Add(temp_buf.CatReal(realVal));
}

int FASTCALL STab::Row::Add(long intVal)
{
	SString temp_buf;
	return Add(temp_buf.Cat(intVal));
}

uint SLAPI STab::Row::GetCount() const
{
	return PosList.getCount();
}

int SLAPI STab::Row::Get(uint pos, SString & rStr) const
{
	int    ok = 1;
	if(pos < PosList.getCount()) {
		Set.getnz(PosList.get(pos), rStr);
	}
	else {
		rStr = 0;
		ok = 0;
	}
	return ok;
}

int SLAPI STab::Row::Get(uint pos, double & rNumber) const
{
	SString temp_buf;
	int    ok = Get(pos, temp_buf);
	rNumber = temp_buf.ToReal();
	return ok;
}

int FASTCALL STab::Row::ToStr(SString & rBuf) const
{
	rBuf.Z();
	long   c = PosList.getCount();
	rBuf.Cat(c).Space();
	for(long i = 0; i < c; i++) {
		rBuf.Cat((long)PosList.get(i)).Space();
	}
	rBuf.Cat(Set.getBuf());
	return 1;
}

int FASTCALL STab::Row::FromStr(const char * pStr)
{
	int    ok = 1;
	Clear();
	size_t p = 0;
	long   c = 0;
	long   pos = 0;
	SString temp_buf;
	for(; isdigit(pStr[p]); p++)
		temp_buf.CatChar(pStr[p]);
	c = temp_buf.ToLong();
	for(long i = 0; i < c; i++) {
		THROW(pStr[p] == ' ');
		p++;
		THROW(isdigit(pStr[p]));
		temp_buf.Z();
		for(; isdigit(pStr[p]); p++)
			temp_buf.CatChar(pStr[p]);
		PosList.add(temp_buf.ToLong());
	}
	THROW(pStr[p] == ' ');
	p++;
	Set.setBuf(pStr+p, strlen(pStr+p)+1);
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

SLAPI STab::STab()
{
	LastRecId = 0;
}

STab & STab::SLAPI Clear()
{
	Data.Clear();
	return *this;
}

uint SLAPI STab::GetCount() const
{
	return Data.getCount();
}

int FASTCALL STab::AddRow(const Row & rRow)
{
	SString temp_buf;
	rRow.ToStr(temp_buf);
	return Data.Add(++LastRecId, temp_buf, 0);
}

int SLAPI STab::GetRow(uint pos, Row & rRow) const
{
	int    ok = 1;
	if(pos < Data.getCount()) {
		StrAssocArray::Item item = Data.at(pos);
		ok = rRow.FromStr(item.Txt);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI STab::Find(uint columnPos, const char * pKey, uint * pRowPos) const
{
	uint   row_pos = DEREFPTRORZ(pRowPos);
	Row    row;
	SString temp_buf;
	for(uint i = row_pos; i < Data.getCount(); i++) {
		if(GetRow(i, row) && row.Get(columnPos, temp_buf) && temp_buf.CmpNC(pKey) == 0) {
			ASSIGN_PTR(pRowPos, i);
			return 1;
		}
	}
	ASSIGN_PTR(pRowPos, Data.getCount());
	return 0;
}

int SLAPI STab::Find(uint columnPos, double key, uint * pRowPos) const
{
	uint   row_pos = DEREFPTRORZ(pRowPos);
	Row    row;
	SString temp_buf;
	for(uint i = row_pos; i < Data.getCount(); i++) {
		if(GetRow(i, row) && row.Get(columnPos, temp_buf) && temp_buf.ToReal() == key) {
			ASSIGN_PTR(pRowPos, i);
			return 1;
		}
	}
	ASSIGN_PTR(pRowPos, Data.getCount());
	return 0;
}

int SLAPI STab::Find(uint columnPos, long key, uint * pRowPos) const
{
	uint   row_pos = DEREFPTRORZ(pRowPos);
	Row    row;
	SString temp_buf;
	for(uint i = row_pos; i < Data.getCount(); i++) {
		if(GetRow(i, row) && row.Get(columnPos, temp_buf) && temp_buf.ToLong() == key) {
			ASSIGN_PTR(pRowPos, i);
			return 1;
		}
	}
	ASSIGN_PTR(pRowPos, Data.getCount());
	return 0;
}
//
//
//
SLAPI STabFile::STabFile()
{
	Flags = 0;
}

SLAPI STabFile::STabFile(const char * pFileName, int updateMode)
{
	Flags = 0;
	Open(pFileName, updateMode);
}

int SLAPI STabFile::IsValid() const
{
	return F.IsValid();
}

int SLAPI STabFile::Open(const char * pFileName, int updateMode)
{
	int    ok = 1;
	if(F.Open(pFileName, (updateMode ? SFile::mReadWrite : SFile::mRead))) {
		if(updateMode)
			Flags |= fUpdateMode;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI STabFile::Close()
{
	F.Close();
	Flags = 0;
	return 1;
}

int SLAPI STabFile::GetTabList(StringSet * pResult)
{
	int    ok = 1;
	int    start_tab = 0;
	STab::Row row;
	SStrScan scan;
	SString line_buf, temp_buf;
	THROW(IsValid());
	F.Seek(0);
	while(F.ReadLine(line_buf)) {
		line_buf.Chomp();
		scan.Set(line_buf, 0);
		if(scan.Skip().GetIdent(temp_buf)) {
			if(start_tab == 0 && temp_buf.CmpNC("BEGIN") == 0) {
				if(scan.Skip().GetIdent(temp_buf)) {
					if(pResult)
						pResult->add(temp_buf);
				}
				start_tab = -1;
			}
			else if(start_tab && temp_buf.CmpNC("END") == 0) {
				start_tab = 0;
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI STabFile::LoadTab(const char * pTabName, STab & rTab)
{
	int    ok = -1;
	int    start_tab = 0;
	STab::Row row;
	SStrScan scan;
	SString line_buf, temp_buf;
	THROW(IsValid());
	F.Seek(0);
	while(F.ReadLine(line_buf)) {
		line_buf.Chomp();
		scan.Set(line_buf, 0);
		if(scan.Skip().GetIdent(temp_buf)) {
			if(temp_buf.CmpNC("BEGIN") == 0) {
				if(scan.Skip().GetIdent(temp_buf) && temp_buf.CmpNC(pTabName) == 0)
					start_tab = 1;
				else
					start_tab = -1;
				continue;
			}
			else if(temp_buf.CmpNC("END") == 0) {
				if(start_tab == 1) {
					start_tab = 0;
					break;
				}
				else {
					start_tab = 0;
					continue;
				}
			}
		}
		if(start_tab == 1) {
			ok = 1;
			scan.Set(line_buf, 0); // restart scaning
			scan.Skip();
			row.Clear();
			while(scan[0] != 0 && !(scan[0] == '/' && scan[1] == '/')) {
				if(scan.GetQuotedString(temp_buf)) {
					THROW(row.Add(temp_buf));
				}
				else if(scan.GetHex(temp_buf)) {
					THROW(row.Add(temp_buf.ToLong()));
				}
				else if(scan.GetNumber(temp_buf)) {
					THROW(row.Add(temp_buf.ToReal()));
				}
				else {
					temp_buf.Z();
					int c = scan[0];
					while(!oneof4(c, ' ', '\t', ';', 0) && !(c == '/' && scan[1] == '/')) {
						temp_buf.CatChar(c);
						scan.Incr();
						c = scan[0];
					}
					THROW(row.Add(temp_buf));
				}
				if(scan.Skip()[0] == ';') {
					scan.Incr();
					scan.Skip();
				}
			}
			THROW(rTab.AddRow(row));
		}
	}
	if(ok < 0) {
		(temp_buf = F.GetName()).CatChar(':').Cat(pTabName);
		SLS.SetError(SLERR_TAB_NFOUND, temp_buf);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI STabFile::Helper_WriteTab(const char * pTabName, STab * pTab, SFile & rFile)
{
	int    ok = 1;
	if(pTab) {
		THROW(IsValid());
		THROW(rFile.IsValid());
		{
			SString line_buf, temp_buf;
			const uint c = pTab->GetCount();
			STab::Row row;
			THROW(rFile.WriteLine(line_buf.Cat("BEGIN").Space().Cat(pTabName).CR()));
			for(uint i = 0; i < c; i++) {
				line_buf.Z().Tab();
				THROW(pTab->GetRow(i, row));
				uint rc = row.GetCount();
				for(uint j = 0; j < rc; j++) {
					THROW(row.Get(j, temp_buf));
					if(j)
						line_buf.CatDiv(';', 2);
					double real_val = temp_buf.ToReal();
					if(real_val != 0.0 || temp_buf[0] == '0')
						if(ffrac(real_val) != 0.0)
							line_buf.CatReal(real_val);
						else
							line_buf.Cat((long)real_val);
					else
						line_buf.CatQStr(temp_buf);
				}
				THROW(rFile.WriteLine(line_buf.CR()));
			}
			THROW(rFile.WriteLine(line_buf.Z().Cat("END").CR()));
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI STabFile::WriteTab(const char * pTabName, STab * pTab)
{
	int    ok = -1;
	SFile  temp_file;
	SString temp_file_name, path;
	THROW(IsValid());
	{
		SPathStruc ps;
		ps.Split(F.GetName());
		ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, path);
		MakeTempFileName(path, "STB", "TMP", 0, temp_file_name);
		THROW(temp_file.Open(temp_file_name, SFile::mWrite));
		{
			int    start_tab = 0, found = 0;
			int    last_line_empty = 0;
			STab::Row row;
			SStrScan scan;
			SString line_buf, temp_buf;
			F.Seek(0);
			while(F.ReadLine(line_buf)) {
				line_buf.Chomp();
				scan.Set(line_buf, 0);
				if(scan.Skip().GetIdent(temp_buf)) {
					if(temp_buf.CmpNC("BEGIN") == 0) {
						if(scan.Skip().GetIdent(temp_buf) && temp_buf.CmpNC(pTabName) == 0) {
							THROW(Helper_WriteTab(pTabName, pTab, temp_file));
							found = 1;
							start_tab = 1;
							ok = 1;
						}
						else
							start_tab = -1;
					}
					else if(temp_buf.CmpNC("END") == 0) {
						if(start_tab == 1) {
							start_tab = 0;
							continue;
						}
						else
							start_tab = 0;
					}
				}
				else if(scan[0] == 0)
					last_line_empty = 1;
				if(start_tab != 1) {
					THROW(temp_file.WriteLine(line_buf.CR()));
				}
			}
			if(!found && pTab) {
				if(!last_line_empty)
					THROW(temp_file.WriteLine(0)); // new line
				THROW(Helper_WriteTab(pTabName, pTab, temp_file));
				ok = 1;
			}
			temp_file.Close();
			if(ok > 0) {
				SString org_file_name = F.GetName();
				F.Close();
				THROW(SFile::Remove(org_file_name));
				THROW(SFile::Rename(temp_file_name, org_file_name));
				THROW(F.Open(org_file_name, 1));
			}
			SFile::Remove(temp_file_name);
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

#if SLTEST_RUNNING // {

SLTEST_R(STab)
{
	int    ok = 1;

	const char * p_tab_list[] = {
		"colors",
		"AE13_data",
		"AE14_data",
		"MathConst"
	};
	const  long expected_exist_tab_flags = (1|2|4|8);
	long   exist_tab_flags = 0;

	uint   i;
	STabFile tab_file;
	STabFile out_file;
	STab tab;
	STab::Row tab_row;
	SString tab_name, temp_buf;
	StringSet tab_list;
	SString in_file_name = MakeInputFilePath("pptabtest.tab");
	SString copy_file_name;
	(copy_file_name = in_file_name).Cat("-copy");
	THROW(SLTEST_CHECK_NZ(copyFileByName(in_file_name, copy_file_name)));
	THROW(SLTEST_CHECK_NZ(tab_file.Open(in_file_name, 0)));
	THROW(SLTEST_CHECK_NZ(out_file.Open(copy_file_name, 1)));
	THROW(SLTEST_CHECK_NZ(tab_file.GetTabList(&tab_list)));
	for(i = 0; tab_list.get(&i, tab_name);) {
		uint   pos = 0;
		THROW(SLTEST_CHECK_NZ(tab_file.LoadTab(tab_name, tab.Clear())));
		for(uint j = 0; j < SIZEOFARRAY(p_tab_list); j++) {
			if(tab_name.CmpNC(p_tab_list[j]) == 0) {
				exist_tab_flags |= (1 << j);
			}
		}
		if(tab_name.CmpNC("colors") == 0) {
			THROW(SLTEST_CHECK_NZ(tab.Find(1, "Coral", &(pos = 0))));
			THROW(SLTEST_CHECK_NZ(tab.Find(1, "Darkslateblue", &(pos = 0))));
			//
			THROW(SLTEST_CHECK_NZ(tab.Find(0, (long)0x009932cc, &(pos = 0))));
			THROW(SLTEST_CHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLTEST_CHECK_NZ(tab_row.Get(1, temp_buf)));
			THROW(SLTEST_CHECK_Z(temp_buf.CmpNC("Darkorchid")));
			//
			THROW(SLTEST_CHECK_NZ(tab.Find(0, (long)0x009acd32, &(pos = 0))));
			THROW(SLTEST_CHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLTEST_CHECK_NZ(tab_row.Get(1, temp_buf)));
			THROW(SLTEST_CHECK_Z(temp_buf.CmpNC("Yellowgreen")));
			//
		}
		else if(tab_name.CmpNC("MathConst") == 0) {
			double val;
			THROW(SLTEST_CHECK_NZ(tab.Find(0, "SMathConst::Pi", &(pos = 0))));
			THROW(SLTEST_CHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLTEST_CHECK_NZ(tab_row.Get(1, val)));
			THROW(SLTEST_CHECK_EQ(val, SMathConst::Pi));
			//
			THROW(SLTEST_CHECK_NZ(tab.Find(0, "SMathConst::SqrtMin", &(pos = 0))));
			THROW(SLTEST_CHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLTEST_CHECK_NZ(tab_row.Get(1, val)));
			THROW(SLTEST_CHECK_EQ(val, SMathConst::SqrtMin));
			//
			THROW(SLTEST_CHECK_NZ(tab.Find(0, "SMathConst::SqrtMax", &(pos = 0))));
			THROW(SLTEST_CHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLTEST_CHECK_NZ(tab_row.Get(1, val)));
			THROW(SLTEST_CHECK_EQ(val, SMathConst::SqrtMax));
			//
		}
	}
	THROW(SLTEST_CHECK_EQ(exist_tab_flags, expected_exist_tab_flags));
	//
	// Переписываем все таблицы в файл out_file
	//
	for(i = 0; tab_list.get(&i, tab_name);) {
		THROW(SLTEST_CHECK_NZ(tab_file.LoadTab(tab_name, tab.Clear())));
		THROW(SLTEST_CHECK_NZ(out_file.WriteTab(tab_name, &tab)));
	}
	{
		//
		// Проверяем результирующий файл: количество записанных
		//  таблиц и операции удаления, вставки и изменения таблиц.
		//
		SString copy_file_name2;
		StringSet tab_list2;
		THROW(SLTEST_CHECK_NZ(out_file.GetTabList(&tab_list2)));
		THROW(SLTEST_CHECK_EQ(tab_list.getCount(), tab_list2.getCount()));
		for(i = 0; tab_list2.get(&i, tab_name);) {
			THROW(SLTEST_CHECK_NZ(out_file.LoadTab(tab_name, tab.Clear())));
			THROW(SLTEST_CHECK_NZ(out_file.WriteTab(tab_name, 0))); // удаляем таблицу
			THROW(SLTEST_CHECK_NZ(out_file.WriteTab(tab_name, &tab))); // вставляем таблицу
		}
		for(i = 0; tab_list2.get(&i, tab_name);) {
			THROW(SLTEST_CHECK_NZ(out_file.LoadTab(tab_name, tab.Clear())));
			THROW(SLTEST_CHECK_NZ(out_file.WriteTab(tab_name, &tab))); // изменяем таблицу
		}
		{
			//
			// Теперь переносим таблицы в третий файл out_file2 и сравниваем файлы out_file и out_file2:
			//   они должны быть эквивалентны.
			//
			(copy_file_name2 = in_file_name).Cat("-copy2");
			STabFile out_file2(copy_file_name2, 1);
			THROW(SLTEST_CHECK_NZ(out_file2.IsValid()));
			tab_list2.clear();
			THROW(SLTEST_CHECK_NZ(out_file.GetTabList(&tab_list2)));
			for(i = 0; tab_list2.get(&i, tab_name);) {
				THROW(SLTEST_CHECK_NZ(out_file.LoadTab(tab_name, tab.Clear())));
				THROW(SLTEST_CHECK_NZ(out_file2.WriteTab(tab_name, &tab)));
			}
		}
		out_file.Close();
		//THROW(SLTEST_CHECK_NZ(SFile::Compare(copy_file_name, copy_file_name2, 0)));
	}
	CATCH
		CurrentStatus = 0;
		ok = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
