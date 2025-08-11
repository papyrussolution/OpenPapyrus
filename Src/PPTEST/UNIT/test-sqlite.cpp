// TEST-SQLITE.CPP
// Copyright (c) A.Sobolev 2024
//
#include <pp.h>
#pragma hdrstop

SLTEST_R(SQLite)
{
	bool debug_mark = false;
	SString temp_buf;
	DbLoginBlock dblb;
	SLS.QueryPath("testroot", temp_buf);
	temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("SQLite").SetLastSlash().Cat("test-sqlite-db");
	dblb.SetAttr(DbLoginBlock::attrDbPath, temp_buf);
	SSqliteDbProvider dbp;
	{
		// ������� ���� ������
		THROW(SLCHECK_NZ(dbp.Login(&dblb, 0)));
	}
	{
		int efer = dbp.IsFileExists_("TestTa01");

		// ������� �������
		DBTable dbt;
		if(dbp.LoadTableSpec(&dbt, "TestTa01", "TestTa01", /*createIfNExists*/0)) {
			if(dbp.CreateDataFile(&dbt, "TestTa01", SET_CRM_TEMP(crmNoReplace), 0)) {
				debug_mark = true;
			}
		}
		//
		// �������� ���������� ������� �������
	}
	{
		// ��������� � ������� ������� ����� ��������� ������� (�� csv-�����)
	}
	{
		// ����� ������ �� �������
	}
	{
		// ����� ��������� ������� ������� �� ���������
	}
	{
		// ������� ������ � ���������, ��� �� ������ ��� � �������
	}
	{
		// �������� ������ � ���������, ��� ��� ������������� ����������
	}
	{
		// ������� ��� ������
	}
	{
		// ������� �������
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}