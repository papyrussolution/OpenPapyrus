// TEST-SQLITE.CPP
// Copyright (c) A.Sobolev 2024, 2025
//
#include <pp.h>
#pragma hdrstop
#include <..\osf\SQLite\sqlite3.h>

void * __CreatePrcssrTestDbObject();
void __DestroyPrcssrTestDbObject(void * p);
int __PrcssrTestDb_Generate_TestTa01_Rec(void * p, TestTa01Tbl::Rec * pRec);

SLTEST_R(SQLite)
{
	bool   debug_mark = false;
	const  uint record_list_max_count = 10;
	void * prc_testdbobj = __CreatePrcssrTestDbObject();
	DBTable * p_tbl = 0;
	SString temp_buf;
	DbLoginBlock dblb;
    TSCollection <TestTa01Tbl::Rec> record_list;
	SLS.QueryPath("testroot", temp_buf);
	temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("SQLite").SetLastSlash().Cat("test-sqlite-db");
    const SString db_path(temp_buf);
	dblb.SetAttr(DbLoginBlock::attrDbPath, db_path);
	SSqliteDbProvider dbp;
	{
		// Создать базу данных
		THROW(SLCHECK_NZ(dbp.Login(&dblb, 0)));
	}
	{
		int efer = dbp.IsFileExists_("TestTa01");
        if(!efer) {
		    // Создать таблицу
		    DBTable dbt;
		    if(dbp.LoadTableSpec(&dbt, "TestTa01", "TestTa01", /*createIfNExists*/0)) {
			    if(dbp.CreateDataFile(&dbt, "TestTa01", SET_CRM_TEMP(crmNoReplace), 0)) {
				    debug_mark = true;
			    }
		    }
        }
	}
	p_tbl = new DBTable("TestTa01", 0, 0, &dbp);
	p_tbl->AllocateOwnBuffer();
	{
		for(uint i = 0; i < record_list_max_count; i++) {
			uint   new_rec_pos = 0;
			TestTa01Tbl::Rec * p_new_rec = record_list.CreateNewItem(&new_rec_pos);
			if(p_new_rec) {
				if(__PrcssrTestDb_Generate_TestTa01_Rec(prc_testdbobj, p_new_rec)) {
					;
				}
				else {
					record_list.atFree(new_rec_pos);
				}
			}
		}
	}
	{
		// Загрузить в таблицу большой набор известных записей (из record_list)
		if(dbp.StartTransaction()) {
			for(uint i = 0; i < record_list.getCount(); i++) {
				const TestTa01Tbl::Rec * p_rec = record_list.at(i);
				if(p_rec) {
					int irr = p_tbl->insertRecBuf(p_rec);
					if(irr) {
						;
					}
					else {
						;
					}
				}
			}
			int cwr = dbp.CommitWork();
			if(!cwr) {
				;
			}
		}
	}
	{
		// Найти каждую из записей
		TestTa01Tbl::Key0 k0;
		MEMSZERO(k0);
		uint   srch_count = 0;
		if(p_tbl->search(0, &k0, spGe)) do {
			srch_count++;
		} while(p_tbl->search(0, spNext));
	}
	{
		// Найти несколько выборок записей по критериям
	}
	{
		// Удалить записи и убедиться, что ее больше нет в таблице
	}
	{
		// Изменить записи и убедиться, что она действительно изменилась
	}
	{
		// Удалить все записи
	}
	{
		// Удалить таблицу
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	__DestroyPrcssrTestDbObject(prc_testdbobj);
	prc_testdbobj = 0;
	delete p_tbl;
	return CurrentStatus;
}