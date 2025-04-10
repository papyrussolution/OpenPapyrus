// TAB.CPP
// Copyright (c) Sobolev A. 1995-2000, 2003, 2010, 2013, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
//
#include <slib-internal.h>
#pragma hdrstop

bool STDCALL SIntToSymbTab_GetSymb(const SIntToSymbTabEntry * pTab, size_t tabSize, int id, SString & rSymb)
{
	rSymb.Z();
	bool   ok = false;
	for(uint i = 0; !ok && i < tabSize; i++) {
		if(pTab[i].Id == id) {
			rSymb = pTab[i].P_Symb;
			ok = true;
		}
	}
	return ok;
}

const char * STDCALL SIntToSymbTab_GetSymbPtr(const SIntToSymbTabEntry * pTab, size_t tabSize, int id)
{
	const char * ptr = 0;
	for(uint i = 0; i < tabSize; i++) {
		if(pTab[i].Id == id) {
			ptr = pTab[i].P_Symb;
			break;
		}
	}
	return ptr;
}

bool STDCALL SIntToSymbTab_HasId(const SIntToSymbTabEntry * pTab, size_t tabSize, int id)
{
	bool   yes = 0;
	for(uint i = 0; !yes && i < tabSize; i++) {
		if(pTab[i].Id == id)
			yes = 1;
	}
	return yes;
}

bool STDCALL SIntToSymbTab_HasSymb(const SIntToSymbTabEntry * pTab, size_t tabSize, const char * pSymb, int * pId)
{
	if(!isempty(pSymb)) {
		for(uint i = 0; i < tabSize; i++) {
			if(sstreqi_ascii(pSymb, pTab[i].P_Symb)) {
				ASSIGN_PTR(pId, pTab[i].Id);
				return true;
			}
		}
	}
	ASSIGN_PTR(pId, 0);
	return false;
}

int STDCALL SIntToSymbTab_GetId(const SIntToSymbTabEntry * pTab, size_t tabSize, const char * pSymb)
{
	if(!isempty(pSymb)) {
		for(uint i = 0; i < tabSize; i++) {
			if(sstreqi_ascii(pSymb, pTab[i].P_Symb))
				return pTab[i].Id;
		}
	}
	return 0;
}

#if 0 // (moved to sformat.cpp as static) {
struct i_tbl {
	int  a;
	int  b;
};

int i_tab(const void * tbl, int req, int def)
{
	const i_tbl * t = (const i_tbl *)tbl;
	for(int i = 1; i <= t[0].a; i++)
		if(t[i].a == req)
			return t[i].b;
	return def;
}

int ai_tab(const void * tbl, int req, int def)
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
#define STAB_ROW_SIGN 0x54425257U

STab::Row::Row() : Set("^\001"), Sign(STAB_ROW_SIGN)
{
	Set.add("$."); // Zero position is invalid
}

STab::Row::~Row()
{
	Sign = 0;
}

bool STab::Row::IsConsistent() const { return (Sign == STAB_ROW_SIGN); }

void STab::Row::Clear()
{
	PosList.clear();
	Set.Z().add("$."); // Zero position is invalid
}

int FASTCALL STab::Row::Add(const char * pStr)
{
	int    ok = 1;
	uint   pos = 0;
	if(pStr)
		Set.add(pStr, &pos);
	PosList.add(static_cast<long>(pos));
	return ok;
}

int STab::Row::Add(double realVal)
{
	SString temp_buf;
	return Add(temp_buf.CatReal(realVal));
}

int FASTCALL STab::Row::Add(long intVal)
{
	SString temp_buf;
	return Add(temp_buf.Cat(intVal));
}

uint STab::Row::GetCount() const { return PosList.getCount(); }

int STab::Row::Get(uint pos, SString & rStr) const
{
	int    ok = 1;
	if(pos < PosList.getCount()) {
		Set.getnz(PosList.get(pos), rStr);
	}
	else {
		rStr.Z();
		ok = 0;
	}
	return ok;
}

int STab::Row::Get(uint pos, double & rNumber) const
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
	for(; isdec(pStr[p]); p++)
		temp_buf.CatChar(pStr[p]);
	c = temp_buf.ToLong();
	for(long i = 0; i < c; i++) {
		THROW(pStr[p] == ' ');
		p++;
		THROW(isdec(pStr[p]));
		temp_buf.Z();
		for(; isdec(pStr[p]); p++)
			temp_buf.CatChar(pStr[p]);
		PosList.add(temp_buf.ToLong());
	}
	THROW(pStr[p] == ' ');
	p++;
	Set.setBuf(pStr+p, sstrlen(pStr+p)+1);
	CATCHZOK
	return ok;
}

STab::STab() : LastRecId(0)
{
}

STab & STab::Z()
{
	Data.Z();
	return *this;
}

uint STab::GetCount() const { return Data.getCount(); }

int FASTCALL STab::AddRow(const Row & rRow)
{
	SString temp_buf;
	rRow.ToStr(temp_buf);
	return Data.Add(++LastRecId, temp_buf, 0);
}

int STab::GetRow(uint pos, Row & rRow) const
{
	int    ok = 1;
	if(pos < Data.getCount()) {
		StrAssocArray::Item item = Data.Get(pos);
		ok = rRow.FromStr(item.Txt);
	}
	else
		ok = 0;
	return ok;
}

int STab::Find(uint columnPos, const char * pKey, uint * pRowPos) const
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

int STab::Find(uint columnPos, double key, uint * pRowPos) const
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

int STab::Find(uint columnPos, long key, uint * pRowPos) const
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
STabFile::STabFile() : Flags(0)
{
}

STabFile::STabFile(const char * pFileName, int updateMode) : Flags(0)
{
	Open(pFileName, updateMode);
}

bool STabFile::IsValid() const { return F.IsValid(); }

int STabFile::Open(const char * pFileName, int updateMode)
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

int STabFile::Close()
{
	F.Close();
	Flags = 0;
	return 1;
}

int STabFile::GetTabList(StringSet * pResult)
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
			if(start_tab == 0 && temp_buf.IsEqiAscii("BEGIN")) {
				if(scan.Skip().GetIdent(temp_buf)) {
					if(pResult)
						pResult->add(temp_buf);
				}
				start_tab = -1;
			}
			else if(start_tab && temp_buf.IsEqiAscii("END")) {
				start_tab = 0;
			}
		}
	}
	CATCHZOK
	return ok;
}

int STabFile::LoadTab(const char * pTabName, STab & rTab)
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
			if(temp_buf.IsEqiAscii("BEGIN")) {
				if(scan.Skip().GetIdent(temp_buf) && temp_buf.CmpNC(pTabName) == 0)
					start_tab = 1;
				else
					start_tab = -1;
				continue;
			}
			else if(temp_buf.IsEqiAscii("END")) {
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
		(temp_buf = F.GetName()).Colon().Cat(pTabName);
		SLS.SetError(SLERR_TAB_NFOUND, temp_buf);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int STabFile::Helper_WriteTab(const char * pTabName, const STab * pTab, SFile & rFile)
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
	CATCHZOK
	return ok;
}

int STabFile::WriteTab(const char * pTabName, const STab * pTab)
{
	int    ok = -1;
	SFile  temp_file;
	SString temp_file_name, path;
	THROW(IsValid());
	{
		SFsPath ps(F.GetName());
		ps.Merge(0, SFsPath::fNam|SFsPath::fExt, path);
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
					if(temp_buf.IsEqiAscii("BEGIN")) {
						if(scan.Skip().GetIdent(temp_buf) && temp_buf.CmpNC(pTabName) == 0) {
							THROW(Helper_WriteTab(pTabName, pTab, temp_file));
							found = 1;
							start_tab = 1;
							ok = 1;
						}
						else
							start_tab = -1;
					}
					else if(temp_buf.IsEqiAscii("END")) {
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
					THROW(temp_file.WriteBlancLine()); // new line
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
	CATCHZOK
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
	SString in_file_name(MakeInputFilePath("pptabtest.tab"));
	SString copy_file_name;
	(copy_file_name = in_file_name).Cat("-copy");
	THROW(SLCHECK_NZ(copyFileByName(in_file_name, copy_file_name)));
	THROW(SLCHECK_NZ(tab_file.Open(in_file_name, 0)));
	THROW(SLCHECK_NZ(out_file.Open(copy_file_name, 1)));
	THROW(SLCHECK_NZ(tab_file.GetTabList(&tab_list)));
	for(i = 0; tab_list.get(&i, tab_name);) {
		uint   pos = 0;
		THROW(SLCHECK_NZ(tab_file.LoadTab(tab_name, tab.Z())));
		for(uint j = 0; j < SIZEOFARRAY(p_tab_list); j++) {
			if(tab_name.CmpNC(p_tab_list[j]) == 0) {
				exist_tab_flags |= (1 << j);
			}
		}
		if(tab_name.IsEqiAscii("colors")) {
			THROW(SLCHECK_NZ(tab.Find(1, "Coral", &(pos = 0))));
			THROW(SLCHECK_NZ(tab.Find(1, "Darkslateblue", &(pos = 0))));
			//
			THROW(SLCHECK_NZ(tab.Find(0, 0x009932ccL, &(pos = 0))));
			THROW(SLCHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLCHECK_NZ(tab_row.Get(1, temp_buf)));
			THROW(SLCHECK_NZ(temp_buf.IsEqiAscii("Darkorchid")));
			//
			THROW(SLCHECK_NZ(tab.Find(0, 0x009acd32L, &(pos = 0))));
			THROW(SLCHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLCHECK_NZ(tab_row.Get(1, temp_buf)));
			THROW(SLCHECK_NZ(temp_buf.IsEqiAscii("Yellowgreen")));
			//
		}
		else if(tab_name.IsEqiAscii("MathConst")) {
			double val;
			THROW(SLCHECK_NZ(tab.Find(0, "SMathConst::Pi", &(pos = 0))));
			THROW(SLCHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLCHECK_NZ(tab_row.Get(1, val)));
			THROW(SLCHECK_EQ(val, SMathConst::Pi));
			//
			THROW(SLCHECK_NZ(tab.Find(0, "SMathConst::SqrtMin", &(pos = 0))));
			THROW(SLCHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLCHECK_NZ(tab_row.Get(1, val)));
			THROW(SLCHECK_EQ(val, SMathConst::SqrtMin));
			//
			THROW(SLCHECK_NZ(tab.Find(0, "SMathConst::SqrtMax", &(pos = 0))));
			THROW(SLCHECK_NZ(tab.GetRow(pos, tab_row)));
			THROW(SLCHECK_NZ(tab_row.Get(1, val)));
			THROW(SLCHECK_EQ(val, SMathConst::SqrtMax));
			//
		}
	}
	THROW(SLCHECK_EQ(exist_tab_flags, expected_exist_tab_flags));
	//
	// ������������ ��� ������� � ���� out_file
	//
	for(i = 0; tab_list.get(&i, tab_name);) {
		THROW(SLCHECK_NZ(tab_file.LoadTab(tab_name, tab.Z())));
		THROW(SLCHECK_NZ(out_file.WriteTab(tab_name, &tab)));
	}
	{
		//
		// ��������� �������������� ����: ���������� ����������
		//  ������ � �������� ��������, ������� � ��������� ������.
		//
		SString copy_file_name2;
		StringSet tab_list2;
		THROW(SLCHECK_NZ(out_file.GetTabList(&tab_list2)));
		THROW(SLCHECK_EQ(tab_list.getCount(), tab_list2.getCount()));
		for(i = 0; tab_list2.get(&i, tab_name);) {
			THROW(SLCHECK_NZ(out_file.LoadTab(tab_name, tab.Z())));
			THROW(SLCHECK_NZ(out_file.WriteTab(tab_name, 0))); // ������� �������
			THROW(SLCHECK_NZ(out_file.WriteTab(tab_name, &tab))); // ��������� �������
		}
		for(i = 0; tab_list2.get(&i, tab_name);) {
			THROW(SLCHECK_NZ(out_file.LoadTab(tab_name, tab.Z())));
			THROW(SLCHECK_NZ(out_file.WriteTab(tab_name, &tab))); // �������� �������
		}
		{
			//
			// ������ ��������� ������� � ������ ���� out_file2 � ���������� ����� out_file � out_file2:
			//   ��� ������ ���� ������������.
			//
			(copy_file_name2 = in_file_name).Cat("-copy2");
			STabFile out_file2(copy_file_name2, 1);
			THROW(SLCHECK_NZ(out_file2.IsValid()));
			tab_list2.Z();
			THROW(SLCHECK_NZ(out_file.GetTabList(&tab_list2)));
			for(i = 0; tab_list2.get(&i, tab_name);) {
				THROW(SLCHECK_NZ(out_file.LoadTab(tab_name, tab.Z())));
				THROW(SLCHECK_NZ(out_file2.WriteTab(tab_name, &tab)));
			}
		}
		out_file.Close();
		//THROW(SLCHECK_NZ(SFile::Compare(copy_file_name, copy_file_name2, 0)));
	}
	CATCH
		CurrentStatus = 0;
		ok = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
