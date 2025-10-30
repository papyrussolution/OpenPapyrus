// TEST-SQLITE.CPP
// Copyright (c) A.Sobolev 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <..\osf\SQLite\sqlite3.h>

void * __CreatePrcssrTestDbObject();
void __DestroyPrcssrTestDbObject(void * p);
int __PrcssrTestDb_Generate_TestRef01_Rec(void * p, TestRef01Tbl::Rec * pRec);
int __PrcssrTestDb_Generate_TestRef02_Rec(void * p, TestRef02Tbl::Rec * pRec);
int __PrcssrTestDb_Generate_TestTa01_Rec(void * p, TestTa01Tbl::Rec * pRec);
int __PrcssrTestDb_Are_TestRef01_RecsEqual(const TestRef01Tbl::Rec & rRec1, const TestRef01Tbl::Rec & rRec2);
int __PrcssrTestDb_Are_TestRef02_RecsEqual(const TestRef02Tbl::Rec & rRec1, const TestRef02Tbl::Rec & rRec2);
int __PrcssrTestDb_Are_TestTa01_RecsEqual(const TestTa01Tbl::Rec & rRec1, const TestTa01Tbl::Rec & rRec2);

SLTEST_R(SQLite)
{
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
			{ Generator_SQL::tokOrderBy, "ORDER BY" },      // @v12.4.0
			{ Generator_SQL::tokTemp, "TEMP" }, // @v12.4.4 tokTemp
			{ Generator_SQL::tokCollate, "COLLATE" }, // @v12.4.7 tokCollate
		};
		//Generator_SQL gen(sqlstSQLite, 0);
		for(uint i = 0; i < SIZEOFARRAY(tok_list); i++) {
			assert(sstreqi_ascii(Generator_SQL::GetToken(tok_list[i].Id), tok_list[i].P_Symb));
		}
	}
	SString temp_buf;
	SString db_file_name;
	bool   debug_mark = false;
	void * prc_testdbobj = __CreatePrcssrTestDbObject();
	DBTable * p_tbl = 0;
	DBTable * p_tbl_ref01 = 0;
	DBTable * p_tbl_ref02 = 0;
	int64  recscount_tbl = 0;
	int64  recscount_ref01 = 0;
	int64  recscount_ref02 = 0;
	DbLoginBlock dblb;
	StrAssocArray some_text_list;
    TSCollection <TestTa01Tbl::Rec> record_list;
	TSCollection <TestRef01Tbl::Rec> ref01_rec_list;
	TSCollection <TestRef02Tbl::Rec> ref02_rec_list;
	SLS.QueryPath("testroot", db_file_name);
	db_file_name.SetLastSlash().Cat("out").SetLastSlash().Cat("SQLite").SetLastSlash().Cat("test-sqlite-db"); // Без расширения!
    const SString db_path(db_file_name);
	dblb.SetAttr(DbLoginBlock::attrDbPath, db_path);
	SSqliteDbProvider dbp;
	{
		// Создать базу данных
		THROW(SLCHECK_NZ(dbp.DbLogin(&dblb, DbProvider::openfMainThread)));
	}
	{
		const char * p_tbl_name_list[] = {
			"TestTa01",
			"TestRef01",
			"TestRef02"
		};
		for(uint i = 0; i < SIZEOFARRAY(p_tbl_name_list); i++) {
			const char * p_tbl_name = p_tbl_name_list[i];
			int efer = dbp.IsFileExists_(p_tbl_name);
			if(!efer) {
				// Создать таблицу
				DBTable dbt;
				if(dbp.LoadTableSpec(&dbt, p_tbl_name, p_tbl_name, /*createIfNExists*/0)) {
					if(dbp.CreateDataFile(&dbt, p_tbl_name, /*SET_CRM_TEMP(crmNoReplace)*/crmNoReplace, 0)) {
						debug_mark = true;
					}
				}
			}
		}
	}
	p_tbl = new DBTable("TestTa01", 0, 0, &dbp);
	p_tbl->AllocateOwnBuffer(-1);
	//
	p_tbl_ref01 = new DBTable("TestRef01", 0, 0, &dbp);
	p_tbl_ref01->AllocateOwnBuffer(-1);
	p_tbl_ref02 = new DBTable("TestRef02", 0, 0, &dbp);
	p_tbl_ref02->AllocateOwnBuffer(-1);
	{
		(temp_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("phrases-ru-1251.txt");
		SFile f_in(temp_buf, SFile::mRead);
		if(f_in.IsValid()) {
			long   str_id = 0;
			while(f_in.ReadLine(temp_buf, SFile::rlfChomp|SFile::rlfStrip)) {
				if(temp_buf.Len()) {
					some_text_list.AddFast(++str_id, temp_buf);
				}
			}
		}
	}
	{
		//
		// Генерация записей таблиц TestRef01, TestRef02, TestTa01
		//
		const  uint ref01_rec_list_max_count = 8000;
		const  uint ref02_rec_list_max_count = 2500;
		const  uint record_list_max_count = 10000;
		{
			for(uint i = 0; i < ref01_rec_list_max_count; i++) {
				uint   new_rec_pos = 0;
				TestRef01Tbl::Rec * p_new_rec = ref01_rec_list.CreateNewItem(&new_rec_pos);
				if(p_new_rec) {
					if(__PrcssrTestDb_Generate_TestRef01_Rec(prc_testdbobj, p_new_rec)) {
						;
					}
					else {
						ref01_rec_list.atFree(new_rec_pos);
					}
				}
			}
		}
		{
			for(uint i = 0; i < ref02_rec_list_max_count; i++) {
				uint   new_rec_pos = 0;
				TestRef02Tbl::Rec * p_new_rec = ref02_rec_list.CreateNewItem(&new_rec_pos);
				if(p_new_rec) {
					if(__PrcssrTestDb_Generate_TestRef02_Rec(prc_testdbobj, p_new_rec)) {
						;
					}
					else {
						ref02_rec_list.atFree(new_rec_pos);
					}
				}
			}
		}
		{
			for(uint i = 0; i < record_list_max_count; i++) {
				uint   new_rec_pos = 0;
				TestTa01Tbl::Rec * p_new_rec = record_list.CreateNewItem(&new_rec_pos);
				if(p_new_rec) {
					if(__PrcssrTestDb_Generate_TestTa01_Rec(prc_testdbobj, p_new_rec)) {
						uint str_idx = (i % some_text_list.getCount());
						StrAssocArray::Item text_item = some_text_list.Get(str_idx);
						const size_t text_len = sstrlen(text_item.Txt);
						if(text_len) {
							p_new_rec->TextField.InitPtr(text_len+1);
							void * ptr = p_new_rec->TextField.GetRawDataPtr();
							if(ptr) {
								memcpy(ptr, text_item.Txt, text_len+1);
							}
						}
					}
					else {
						record_list.atFree(new_rec_pos);
					}
				}
			}
		}
	}
	{
		uint   ref01_insert_fault_count = 0;
		uint   ref01_bextins_fault_count = 0;

		uint   ref02_insert_fault_count = 0;
		uint   ta02_insert_fault_count = 0;
		// Загрузить в таблицу большой набор известных записей (из record_list)
		if(dbp.StartTransaction()) {
			{
				//
				// Для таблицы TestRef01 применяем BExtInsert
				//
				const bool use_bextinsert = true;
				TestRef01Tbl::Key0 k0;
				DBRowId ret_row_id;
				BExtInsert * p_bei = 0;
				if(use_bextinsert)
					p_bei = new BExtInsert(p_tbl_ref01, SKILOBYTE(2));
				for(uint i = 0; i < ref01_rec_list.getCount(); i++) {
					TestRef01Tbl::Rec * p_rec = ref01_rec_list.at(i);
					if(p_rec) {
						if(p_bei) {
							if(!p_bei->insert(p_rec)) {
								ref01_bextins_fault_count++;
							}
						}
						else {
							MEMSZERO(k0);
							p_tbl_ref01->CopyBufLobFrom(p_rec, sizeof(*p_rec));
							int irr = p_tbl_ref01->insertRec(0, &k0);
							const DBRowId * p_row_id = p_tbl_ref01->getCurRowIdPtr();
							if(p_row_id)
								ret_row_id = *p_row_id;
							if(irr) {
								p_rec->ID = k0.ID;
								const uint32 _row_id = ret_row_id.GetI32();
								assert(_row_id == p_rec->ID); // SQLite specific!
							}
							else {
								ref01_insert_fault_count++;
							}
						}
					}
				}
				if(p_bei) {
					if(!p_bei->flash())
						ref01_bextins_fault_count++;
					ZDELETE(p_bei);
				}
			}
			{
				TestRef02Tbl::Key0 k0;
				DBRowId ret_row_id;
				for(uint i = 0; i < ref02_rec_list.getCount(); i++) {
					TestRef02Tbl::Rec * p_rec = ref02_rec_list.at(i);
					if(p_rec) {
						MEMSZERO(k0);
						p_tbl_ref02->CopyBufLobFrom(p_rec, sizeof(*p_rec));
						int irr = p_tbl_ref02->insertRec(0, &k0);
						const DBRowId * p_row_id = p_tbl_ref02->getCurRowIdPtr();
						if(p_row_id)
							ret_row_id = *p_row_id;
						if(irr) {
							p_rec->ID = k0.ID;
							assert(ret_row_id.GetI32() == p_rec->ID); // SQLite specific!
						}
						else {
							ref02_insert_fault_count++;
						}
					}
				}
			}
			{
				TestTa01Tbl::Key0 k0;
				DBRowId ret_row_id;
				for(uint i = 0; i < record_list.getCount(); i++) {
					const TestTa01Tbl::Rec * p_rec = record_list.at(i);
					if(p_rec) {
						MEMSZERO(k0);
						p_tbl->CopyBufLobFrom(p_rec, sizeof(*p_rec));
						int irr = p_tbl->insertRec(0, &k0);
						const DBRowId * p_row_id = p_tbl->getCurRowIdPtr();
						if(p_row_id)
							ret_row_id = *p_row_id;
						if(irr) {
							;
						}
						else {
							ta02_insert_fault_count++;
						}
					}
				}
			}
			int cwr = dbp.CommitWork();
			if(!cwr) {
				;
			}
			SLCHECK_Z(ta02_insert_fault_count);
			SLCHECK_Z(ref01_insert_fault_count);
			SLCHECK_Z(ref02_insert_fault_count);
		}
	}
	LDATETIME first_pattern_dtm;
	LDATETIME last_pattern_dtm;
	first_pattern_dtm.Set(record_list.at(0)->Dt, record_list.at(0)->Tm);
	last_pattern_dtm.Set(record_list.at(record_list.getCount()-1)->Dt, record_list.at(record_list.getCount()-1)->Tm);
	{
		const  uint rec_list_count = record_list.getCount();
		//const TestTa01Tbl::Rec * p_first_pattern_rec = record_list.at(0);
		TestTa01Tbl::Key0 k0;
		TestTa01Tbl::Rec * p_rec_buf = static_cast<TestTa01Tbl::Rec *>(p_tbl->getDataBuf());
		{
			// Найти каждую из записей
			MEMSZERO(k0);
			k0.Dt = first_pattern_dtm.d;
			k0.Tm = first_pattern_dtm.t;
			uint   srch_count = 0;
			uint   uneq_rec_count = 0;
			uint   uneq_clob_rec_count = 0;
			if(p_tbl->search(0, &k0, spGe)) do {
				const TestTa01Tbl::Rec * p_pattern_rec = record_list.at(srch_count);
				const DBRowId * p_rowid = p_tbl->getCurRowIdPtr();
				const int64 rowid_i64 = p_rowid ? p_rowid->GetI64() : 0;
				if(!__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_pattern_rec)) {
					uneq_rec_count++;
				}
				{
					const size_t ls_p = p_pattern_rec->TextField.GetPtrSize();
					const char * lp_p = static_cast<const char *>(p_pattern_rec->TextField.GetRawDataPtrC());

					const size_t ls_t = p_rec_buf->TextField.GetPtrSize();
					const char * lp_t = static_cast<const char *>(p_rec_buf->TextField.GetRawDataPtrC());
					if(ls_p != ls_t || memcmp(lp_p, lp_t, ls_p) != 0) {
						uneq_clob_rec_count++;
					}
				}
				srch_count++;
			} while(srch_count < rec_list_count && p_tbl->search(0, &k0, spNext));
			SLCHECK_Z(uneq_rec_count);
		}
		{
			// Здесь делаем тоже самое, что и в предыдущем блоке, но перебираем записи в обратном порядке
			MEMSZERO(k0);
			k0.Dt = MAXDATE;
			k0.Tm = MAXTIME;
			uint   srch_count = 0;
			uint   uneq_rec_count = 0;
			uint   uneq_clob_rec_count = 0;
			if(p_tbl->search(0, &k0, spLe)) do {
				const TestTa01Tbl::Rec * p_pattern_rec = record_list.at(rec_list_count-srch_count-1);
				const DBRowId * p_rowid = p_tbl->getCurRowIdPtr();
				const int64 rowid_i64 = p_rowid ? p_rowid->GetI64() : 0;
				if(!__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_pattern_rec)) {
					uneq_rec_count++;
				}
				{
					const size_t ls_p = p_pattern_rec->TextField.GetPtrSize();
					const char * lp_p = static_cast<const char *>(p_pattern_rec->TextField.GetRawDataPtrC());

					const size_t ls_t = p_rec_buf->TextField.GetPtrSize();
					const char * lp_t = static_cast<const char *>(p_rec_buf->TextField.GetRawDataPtrC());
					if(ls_p != ls_t || memcmp(lp_p, lp_t, ls_p) != 0) {
						uneq_clob_rec_count++;
					}
				}
				srch_count++;
			} while(srch_count < rec_list_count && p_tbl->search(0, &k0, spPrev) && (p_rec_buf->Dt > first_pattern_dtm.d || (p_rec_buf->Dt == first_pattern_dtm.d && p_rec_buf->Tm >= first_pattern_dtm.t)));
			SLCHECK_Z(uneq_rec_count);
		}
		if(p_tbl_ref01) {
			//
			// Теперь ищем по текстовому индексу в таблице Ref01
			//
			for(uint i = 0; i < ref01_rec_list.getCount(); i++) {
				const TestRef01Tbl::Rec * p_ref01_patten_rec = ref01_rec_list.at(i);
				if(p_ref01_patten_rec && !isempty(p_ref01_patten_rec->S48)) {
					TestRef01Tbl::Key5 ref01_k5;
					MEMSZERO(ref01_k5);
					STRNSCPY(ref01_k5.S48, p_ref01_patten_rec->S48);
					const int sr = p_tbl_ref01->search(5, &ref01_k5, spEq);
					SLCHECK_NZ(sr);
					if(sr) {
						TestRef01Tbl::Rec * p_ref01_rec_buf = static_cast<TestRef01Tbl::Rec *>(p_tbl_ref01->getDataBuf());
						SLCHECK_Z(stricmp866(p_ref01_rec_buf->S48, p_ref01_patten_rec->S48));
					}
					else {
						;
					}
				}
			}
		}
	}
	{
		// Изменить записи и убедиться, что она действительно изменилась
		uint   inprop_updated_count = 0;
		if(dbp.StartTransaction()) {
			for(uint i = 0; i < record_list.getCount(); i++) {
				TestTa01Tbl::Rec * p_rec = record_list.at(i);
				if(p_rec) {
					S_GUID new_uuid(SCtrGenerate_);
					TestTa01Tbl::Key0 k0;
					k0.Dt = p_rec->Dt;
					k0.Tm = p_rec->Tm;
					if(p_tbl->search(&k0, spEq)) {
						TestTa01Tbl::Rec * p_rec_buf = static_cast<TestTa01Tbl::Rec *>(p_tbl->getDataBuf());
						p_rec_buf->GuidVal = new_uuid;
						const int urr = p_tbl->updateRec();
						if(urr) {
							if(p_tbl->search(&k0, spEq)) {
								if(p_rec_buf->GuidVal == new_uuid) { // ok
									p_rec->GuidVal = new_uuid;
								}
								else { // bad
									inprop_updated_count++;
									
								}
							}
						}
					}
					else {
						; // bad
					}
				}
			}
			int cwr = dbp.CommitWork();
			if(cwr) {
				SLCHECK_Z(inprop_updated_count);
			}
			else {
				;
			}
		}
	}
	{
		// Найти несколько выборок записей по критериям
		{
			// @20250916 Увы, сейчас генератор записей не создает дубликатов - придется менять генератор дабы он спонтанно 
			// создавал дубликаты

			// Поле TestTa01::I64Val проиндексировано. Многие значения имеют дубликаты (выяснено экспериментально).
			// В общем, то что нужно.
			// Ищем с списке record_list дублированные значения, затем осуществляем поиск в таблице по индексу 2
			if(false/* @construction */) {
				const uint max_test_count = 10;
				uint test_count = 0;
				int64 val_to_search = -1;
				uint local_fault_count = 0;
				for(uint i = 0; test_count < max_test_count && i < record_list.getCount(); i++) {
					TestTa01Tbl::Rec * p_rec = record_list.at(i);
					if(p_rec) {
						const int64 prim_val = p_rec->I64Val;
						LongArray pos_list;
						pos_list.add(static_cast<long>(i));
						if(prim_val > 0) {
							for(uint j = i+1; j < record_list.getCount(); j++) {
								TestTa01Tbl::Rec * p_rec2 = record_list.at(j);
								if(p_rec2 && p_rec2->I64Val == prim_val) {
									pos_list.add(static_cast<long>(j));
									val_to_search = prim_val;
								}
							}
							if(pos_list.getCount() > 1) {
								test_count++;
								TestTa01Tbl::Key2 k2;
								k2.I64Val = prim_val;
								uint local_found_rec_count = 0;
								if(p_tbl->search(2, &k2, spEq)) {
									do {
										local_found_rec_count++;
										TestTa01Tbl::Rec * p_rec_buf = static_cast<TestTa01Tbl::Rec *>(p_tbl->getDataBuf());
										bool _local_found = false;
										for(uint n = 0; !_local_found && n < pos_list.getCount(); n++) {
											const uint _pos = pos_list.get(n);
											const TestTa01Tbl::Rec * p_inner_rec = record_list.at(_pos);
											if(__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_inner_rec)) {
												_local_found = true;
											}
										}
										if(!_local_found) {
											local_fault_count++;
										}
									} while(p_tbl->search(2, &k2, spNext) && k2.I64Val == prim_val);
									if(local_fault_count != pos_list.getCount()) {
										local_fault_count++;
									}
								}
								else {
									local_fault_count++;
								}
							}
						}
					}
				}
				SLCHECK_Z(local_fault_count);
			}
		}
	}
	{
		// Удалить каждую сотую запись и убедиться, что ее больше нет в таблице
		if(dbp.StartTransaction()) {
			LongArray deleted_idx_list;
			uint    delete_fault_count = 0;
			uint    deleted_found_fault_count = 0;
			uint    _found_fault_count = 0;
			{
				for(uint i = 0; i < record_list.getCount(); i++) {
					TestTa01Tbl::Rec * p_rec = record_list.at(i);
					if(p_rec) {
						if((i % 100) == 0) {
							TestTa01Tbl::Key0 k0;
							k0.Dt = p_rec->Dt;
							k0.Tm = p_rec->Tm;
							if(p_tbl->search(&k0, spEq)) {
								if(p_tbl->deleteRec()) {
									deleted_idx_list.add(static_cast<long>(i));
								}
								else {
									delete_fault_count++;
								}
							}
						}
					}
				}
			}
			if(!delete_fault_count) {
				uint i = record_list.getCount();
				if(i) do {
					TestTa01Tbl::Rec * p_rec = record_list.at(--i);
					if(p_rec) {
						const bool must_be_deleted = deleted_idx_list.lsearch(i);
						TestTa01Tbl::Key0 k0;
						k0.Dt = p_rec->Dt;
						k0.Tm = p_rec->Tm;
						if(p_tbl->search(&k0, spEq)) {
							if(must_be_deleted) {
								deleted_found_fault_count++;
							}
						}
						else {
							if(!must_be_deleted) {
								_found_fault_count++;
							}
						}
						if(must_be_deleted) {
							record_list.atFree(i);
						}
					}
				} while(i);
			}
			else {
			}
			int cwr = dbp.CommitWork();
			if(cwr) {
				;
			}
			else {
				;
			}
		}
	}
	{
		//
		// Найти записи в таблице TestTa01 по запросу RVal1 в диапазоне [2.5..2.6]
		// 
		//BExtQuery q(p_tbl, 0);
		//q.selectAll().where(p_tbl->RVal1 >= 2.5 && p_tbl->)
		//TestTa01Tbl tbl__;
	}
	{
		DbTableStat stat;
		if(p_tbl->GetFileStat(-1, &stat) > 0)
			recscount_tbl = stat.NumRecs;
		stat.Z();
		if(p_tbl_ref01->GetFileStat(-1, &stat) > 0)
			recscount_ref01 = stat.NumRecs;
		stat.Z();
		if(p_tbl_ref02->GetFileStat(-1, &stat) > 0)
			recscount_ref02 = stat.NumRecs;
	}
	{
		//
		// Удалить записи из таблицы TestTa01 по запросу RVal1 в диапазоне [2.5..2.6]
		// 
#if 0 // @construction {
		RealRange range;
		range.Set(2.5, 2.6);
		LongArray idx_list_to_remove;
		{
			for(uint i = 0; i < record_list.getCount(); i++) {
				const TestTa01Tbl::Rec * p_rec = record_list.at(i);
				if(p_rec && range.Check(p_rec->RVal1)) {
					idx_list_to_remove.add(static_cast<long>(i));
				}
			}
			if(idx_list_to_remove.getCount()) {
				TestTa01Tbl ta_tbl; // В отличии от p_tbl здесь есть заданные дескрипторы полей
				int dfr = deleteFrom(&ta_tbl, 1/*use_ta*/, (ta_tbl.RVal1 >= range.low && ta_tbl.RVal1 <= range.upp));
				if(dfr) {
					{
						// Найти каждую из записей
						TestTa01Tbl::Key0 k0;
						MEMSZERO(k0);
						k0.Dt = first_pattern_dtm.d;
						k0.Tm = first_pattern_dtm.t;
						uint   srch_count = 0;
						uint   uneq_rec_count = 0;
						uint   uneq_clob_rec_count = 0;
						if(p_tbl->search(0, &k0, spGe)) do {
							const TestTa01Tbl::Rec * p_pattern_rec = record_list.at(srch_count);
							const DBRowId * p_rowid = p_tbl->getCurRowIdPtr();
							const int64 rowid_i64 = p_rowid ? p_rowid->GetI64() : 0;
							if(!__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_pattern_rec)) {
								uneq_rec_count++;
							}
							{
								const size_t ls_p = p_pattern_rec->TextField.GetPtrSize();
								const char * lp_p = static_cast<const char *>(p_pattern_rec->TextField.GetRawDataPtrC());

								const size_t ls_t = p_rec_buf->TextField.GetPtrSize();
								const char * lp_t = static_cast<const char *>(p_rec_buf->TextField.GetRawDataPtrC());
								if(ls_p != ls_t || memcmp(lp_p, lp_t, ls_p) != 0) {
									uneq_clob_rec_count++;
								}
							}
							srch_count++;
						} while(srch_count < rec_list_count && p_tbl->search(0, &k0, spNext));
						SLCHECK_Z(uneq_rec_count);
					}
				}
			}
		}
#endif // } 0 @construction
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
	delete p_tbl_ref01;
	delete p_tbl_ref02;
	return CurrentStatus;
}