// TEST-SQLITE.CPP
// Copyright (c) A.Sobolev 2024, 2025
//
#include <pp.h>
#pragma hdrstop
#include <..\osf\SQLite\sqlite3.h>

void * __CreatePrcssrTestDbObject();
void __DestroyPrcssrTestDbObject(void * p);
int __PrcssrTestDb_Generate_TestTa01_Rec(void * p, TestTa01Tbl::Rec * pRec);
int __PrcssrTestDb_Are_TestTa01_RecsEqual(const TestTa01Tbl::Rec & rRec1, const TestTa01Tbl::Rec & rRec2);

SLTEST_R(SQLite)
{
	SString temp_buf;
	{
		static const SIntToSymbTabEntry tok_list[] = {
			{ Generator_SQL::tokCreate, "CREATE" },
			{ Generator_SQL::tokAlter, "ALTER" },
			{ Generator_SQL::tokDrop, "DROP" },
			{ Generator_SQL::tokDatabase, "DATABASE" },
			{ Generator_SQL::tokTable, "TABLE" },
			{ Generator_SQL::tokIndex, "INDEX" },
			{ Generator_SQL::tokConstr, "CONSTRAINT" },
			{ Generator_SQL::tokSequence, "SEQUENCE" },
			{ Generator_SQL::tokInsert, "INSERT" },
			{ Generator_SQL::tokUpdate, "UPDATE" },
			{ Generator_SQL::tokDelete, "DELETE" },
			{ Generator_SQL::tokSelect, "SELECT" },
			{ Generator_SQL::tokInto, "INTO" },
			{ Generator_SQL::tokWhere, "WHERE" },
			{ Generator_SQL::tokFrom, "FROM" },
			{ Generator_SQL::tokUnique, "UNIQUE" },
			{ Generator_SQL::tokOn, "ON" },
			{ Generator_SQL::tokAnd, "AND" },
			{ Generator_SQL::tokOr, "OR" },
			{ Generator_SQL::tokDesc, "DESC" },
			{ Generator_SQL::tokValues, "VALUES" },
			{ Generator_SQL::tokHintIndexAsc, "INDEX_ASC" },
			{ Generator_SQL::tokHintIndexDesc, "INDEX_DESC" },
			{ Generator_SQL::tokFor, "FOR" },
			{ Generator_SQL::tokRowId, "ROWID" },
			{ Generator_SQL::tokSet, "SET" },
			{ Generator_SQL::tokReturning, "RETURNING" },
			{ Generator_SQL::tokMax, "MAX" },      // function max
			{ Generator_SQL::tokNlsLower, "NLS_LOWER" }, // function nls_lower
			{ Generator_SQL::tokLower, "LOWER" },    // @v11.6.0 function lower
			{ Generator_SQL::tokIfNotExists, "IF NOT EXISTS" }, // @v11.9.12 if not exists
			{ Generator_SQL::tokBegin, "BEGIN" }, // @v12.3.12
			{ Generator_SQL::tokCommit, "COMMIT" }, // @v12.3.12
			{ Generator_SQL::tokRollback, "ROLLBACK" }, // @v12.3.12
			{ Generator_SQL::tokTransaction, "TRANSACTION" }, // @v12.3.12
			{ Generator_SQL::tokIndexedBy, "INDEXED BY" }, // @v12.4.0 sqlite "indexed by"
		};
		//Generator_SQL gen(sqlstSQLite, 0);
		for(uint i = 0; i < SIZEOFARRAY(tok_list); i++) {
			assert(sstreqi_ascii(Generator_SQL::GetToken(tok_list[i].Id), tok_list[i].P_Symb));
		}
	}
	bool   debug_mark = false;
	const  uint record_list_max_count = 10000;
	void * prc_testdbobj = __CreatePrcssrTestDbObject();
	DBTable * p_tbl = 0;
	DbLoginBlock dblb;
    TSCollection <TestTa01Tbl::Rec> record_list;
	SLS.QueryPath("testroot", temp_buf);
	temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("SQLite").SetLastSlash().Cat("test-sqlite-db");
    const SString db_path(temp_buf);
	dblb.SetAttr(DbLoginBlock::attrDbPath, db_path);
	SSqliteDbProvider dbp;
	{
		// —оздать базу данных
		THROW(SLCHECK_NZ(dbp.Login(&dblb, 0)));
	}
	{
		int efer = dbp.IsFileExists_("TestTa01");
        if(!efer) {
		    // —оздать таблицу
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
		// «агрузить в таблицу большой набор известных записей (из record_list)
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
		const  uint rec_list_count = record_list.getCount();
		const TestTa01Tbl::Rec * p_first_pattern_rec = record_list.at(0);
		TestTa01Tbl::Key0 k0;
		TestTa01Tbl::Rec * p_rec_buf = static_cast<TestTa01Tbl::Rec *>(p_tbl->getDataBuf());
		{
			// Ќайти каждую из записей
			MEMSZERO(k0);
			k0.Dt = p_first_pattern_rec->Dt;
			k0.Tm = p_first_pattern_rec->Tm;
			uint   srch_count = 0;
			uint   uneq_rec_count = 0;
			if(p_tbl->search(0, &k0, spGe)) do {
				const TestTa01Tbl::Rec * p_pattern_rec = record_list.at(srch_count);
				if(!__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_pattern_rec)) {
					uneq_rec_count++;
				}
				srch_count++;
			} while(p_tbl->search(0, &k0, spNext));
			SLCHECK_Z(uneq_rec_count);
		}
		{
			// «десь делаем тоже самое, что и в предыдущем блоке, но перебираем записи в обратном пор€дке
			MEMSZERO(k0);
			k0.Dt = MAXDATE;
			k0.Tm = MAXTIME;
			uint   srch_count = 0;
			uint   uneq_rec_count = 0;
			if(p_tbl->search(0, &k0, spLe)) do {
				const TestTa01Tbl::Rec * p_pattern_rec = record_list.at(rec_list_count-srch_count-1);
				if(!__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_pattern_rec)) {
					uneq_rec_count++;
				}
				srch_count++;
			} while(p_tbl->search(0, &k0, spPrev) && (p_rec_buf->Dt > p_first_pattern_rec->Dt || (p_rec_buf->Dt == p_first_pattern_rec->Dt && p_rec_buf->Tm >= p_first_pattern_rec->Tm)));
			SLCHECK_Z(uneq_rec_count);
		}
	}
	{
		// Ќайти несколько выборок записей по критери€м
	}
	{
		// ”далить записи и убедитьс€, что ее больше нет в таблице
	}
	{
		// »зменить записи и убедитьс€, что она действительно изменилась
	}
	{
		// ”далить все записи
	}
	{
		// ”далить таблицу
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	__DestroyPrcssrTestDbObject(prc_testdbobj);
	prc_testdbobj = 0;
	delete p_tbl;
	return CurrentStatus;
}