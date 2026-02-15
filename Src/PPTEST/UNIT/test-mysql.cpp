// TEST-MYSQL.CPP
// Copytight (c) A.Sobolev 2025, 2026
// @codepage UTF-8
// Тестирование интерфейса с MySQL/MariaSQL
//
#include <pp.h>
#pragma hdrstop

void * __CreatePrcssrTestDbObject();
void __DestroyPrcssrTestDbObject(void * p);
int __PrcssrTestDb_Generate_TestRef01_Rec(void * p, TestRef01Tbl::Rec * pRec);
int __PrcssrTestDb_Generate_TestRef02_Rec(void * p, TestRef02Tbl::Rec * pRec);
int __PrcssrTestDb_Generate_TestTa01_Rec(void * p, TestTa01Tbl::Rec * pRec);
int __PrcssrTestDb_Are_TestRef01_RecsEqual(const TestRef01Tbl::Rec & rRec1, const TestRef01Tbl::Rec & rRec2);
int __PrcssrTestDb_Are_TestRef02_RecsEqual(const TestRef02Tbl::Rec & rRec1, const TestRef02Tbl::Rec & rRec2);
int __PrcssrTestDb_Are_TestTa01_RecsEqual(const TestTa01Tbl::Rec & rRec1, const TestTa01Tbl::Rec & rRec2);

SLTEST_R(MySQL) // @v12.4.7
{
	PPIniFile ini_file;
	DbLoginBlock dblb;
	StrAssocArray some_text_list;
    TSCollection <TestTa01Tbl::Rec> record_list;
	TSCollection <TestRef01Tbl::Rec> ref01_rec_list;
	TSCollection <TestRef02Tbl::Rec> ref02_rec_list;
	SMySqlDbProvider dbp(0);
	uint   database_state = 0;
	const char * p_database_name = "test-db-01";
	int    gdbsr = 0; // Результат функции dbp.GetDatabaseState()
	SString temp_buf;
	bool   debug_mark = false;
	void * prc_testdbobj = __CreatePrcssrTestDbObject();
	DBTable * p_tbl = 0;
	DBTable * p_tbl_ref01 = 0;
	DBTable * p_tbl_ref02 = 0;
	int64  recscount_tbl = 0;
	int64  recscount_ref01 = 0;
	int64  recscount_ref02 = 0;
	// @doto Setup the dpb
	{
		PPDbEntrySet2 dbes;
		THROW(dbes.ReadFromProfile(&ini_file, false));
		THROW_SL(dbes.GetBySymb("mysqltest", &dblb));
	}
	THROW(SLCHECK_NZ(dbp.DbLogin(&dblb, DbProvider::openfMainThread)));
	gdbsr = dbp.GetDatabaseState(p_database_name, &database_state);
	THROW(gdbsr);
	{
		if(!(database_state & DbProvider::dbstNotExists)) {
			THROW(dbp.DropDatabase(p_database_name));
			gdbsr = dbp.GetDatabaseState(p_database_name, &database_state);
			THROW(gdbsr);
			SLCHECK_NZ(database_state & DbProvider::dbstNotExists);
		}
		THROW(dbp.CreateDatabase(p_database_name));
	}
	gdbsr = dbp.GetDatabaseState(p_database_name, &database_state);
	THROW(gdbsr);
	SLCHECK_Z(database_state & DbProvider::dbstNotExists);
	THROW(dbp.UseDatabase(p_database_name));
	{
		// Проверяем работу функций управления транзакциями.
		int    st_r1 = 0;
		int    st_r2 = 0;
		int    cw_r = 0;
		int    rw_r = 0;
		{
			st_r1 = dbp.StartTransaction();
			if(st_r1) {
				rw_r = dbp.RollbackWork();
			}
		}
		{
			st_r2 = dbp.StartTransaction();
			if(st_r2) {
				cw_r = dbp.CommitWork();
			}
		}
		SLCHECK_NZ(st_r1);
		SLCHECK_NZ(st_r2);
		SLCHECK_NZ(rw_r);
		SLCHECK_NZ(cw_r);
	}
	// @v12.5.3 {
	{
		// Проверяем пул соединений
  		bool  conn_pool_fault = false;
		const uint max_iter_count = 1000;
		TSVector <SMySqlDbProvider::ConnectionEntry> conn_list;
		{
			for(uint i = 0; i < max_iter_count; i++) {
				const ulong randval = SLS.GetTLA().Rg.GetUniformIntPos(1920701);
				if(randval & 0x1) { // Если randval - нечетное, то создаем соединение
					SMySqlDbProvider::ConnectionEntry * p_conn_to_get = dbp.GetConnection();
					if(!p_conn_to_get) {
						conn_pool_fault = true;
						break;
					}
					else {
						conn_list.insert(p_conn_to_get);
					}
				}
				else { // Если randval - четное, то разрушаем соединение (если есть такое)
					if(conn_list.getCount()) {
						conn_list.shuffle();
						const   uint conn_idx = conn_list.getCount()-1;
						SMySqlDbProvider::ConnectionEntry conn_to_release = conn_list.at(conn_idx);
						const int r = dbp.ReleaseConnection(conn_to_release);
						conn_list.atFree(conn_idx);
						if(r != 2) {
							conn_pool_fault = true;
							break;
						}
					}
				}
				SDelay(49);
			}
		}
		SLCHECK_Z(conn_pool_fault);
		{
			for(uint i = 0; i < conn_list.getCount(); i++) {
				SMySqlDbProvider::ConnectionEntry & r_conn_to_release = conn_list.at(i);
				const int r = dbp.ReleaseConnection(r_conn_to_release);
			}
		}
	}
	// } @v12.5.3 
	{
		const char * p_tbl_name_list[] = {
			"TestTa01",
			"TestRef01",
			"TestRef02"
		};
		for(uint passi = 0; passi < 2; passi++) { // Два прохода: на первом создаем таблицы и сразу удаляем, на втором - просто создаем заново.
			for(uint i = 0; i < SIZEOFARRAY(p_tbl_name_list); i++) {
				const char * p_tbl_name = p_tbl_name_list[i];
				int efer = dbp.IsFileExists_(p_tbl_name);
				if(!efer) {
					// Создать таблицу
					DBTable dbt;
					if(dbp.LoadTableSpec(&dbt, p_tbl_name, p_tbl_name, /*createIfNExists*/0)) {
						if(dbp.CreateDataFile(&dbt, p_tbl_name, /*SET_CRM_TEMP(crmNoReplace)*/crmNoReplace, 0)) {
							SLCHECK_NZ(dbp.IsFileExists_(p_tbl_name));
							///* @construction (не сработало с первого раза)
							if(passi == 0) {
								// На первом проходе сразу разрушаем таблицу для тестирования метода SMySqlDbProvider::DropFile
								SLCHECK_NZ(dbp.DropFile(p_tbl_name));
								SLCHECK_Z(dbp.IsFileExists_(p_tbl_name));
							}
							//*/
						}
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
	{
		p_tbl_ref02 = new DBTable("TestRef02", 0, 0, &dbp);
		const RECORDSIZE rs = p_tbl_ref02->GetFields().CalculateFixedRecSize(BNFieldList2::crsfIncludeNote);
		p_tbl_ref02->AllocateOwnBuffer(rs);
	}
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
						temp_buf = text_item.Txt;
						const size_t text_len = temp_buf.Len();
						if(text_len) {
							temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
							p_new_rec->TextField.InitPtr(text_len+1);
							void * ptr = p_new_rec->TextField.GetRawDataPtr();
							if(ptr) {
								memcpy(ptr, temp_buf.cptr(), text_len+1);
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
		{
			if(dbp.StartTransaction()) {
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
				{
					//
					// Для таблицы TestRef01 применяем BExtInsert
					//
					const bool use_bextinsert = false;
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
									assert(_row_id == p_rec->ID); // MySQL specific!
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
								assert(ret_row_id.GetI32() == p_rec->ID); // MySQL specific!
							}
							else {
								ref02_insert_fault_count++;
							}
						}
					}
				}
				const int cwr = dbp.CommitWork();
				SLCHECK_NZ(cwr);
				SLCHECK_Z(ta02_insert_fault_count);
				SLCHECK_Z(ref01_insert_fault_count);
				SLCHECK_Z(ref02_insert_fault_count);
			}
		}
	}
	//
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
			/*
				@todo @2020122 В этом блоке (и, видимо, во всех таких же) забиваются все свободные соединения из пула dbp.ConnPool
			*/ 
			// Поиск по GUID-полю (TestTa01Tbl::GuidVal)
			uint   srch_count = 0;
			uint   uneq_rec_count = 0;
			uint   nfound_count = 0;
			for(uint i = 0; i < record_list.getCount(); i++) {
				const TestTa01Tbl::Rec * p_pattern_rec = record_list.at(i);
				if(p_pattern_rec) {
					//BExtQuery q(p_tbl, 0);
					//q.selectAll().
					TestTa01Tbl::Key3 k3;
					k3.GuidVal = p_pattern_rec->GuidVal;
					if(p_tbl->search(3, &k3, spEq)) {
						if(!__PrcssrTestDb_Are_TestTa01_RecsEqual(*p_rec_buf, *p_pattern_rec)) {							
							uneq_rec_count++;
						}
						const DBRowId * p_row_id = p_tbl->getCurRowIdPtr();
						SLCHECK_NZ(p_row_id && p_row_id->GetI64() > 0);
					}
					else {
						nfound_count++;
					}
					srch_count++;
				}
			}
			SLCHECK_Z(uneq_rec_count);
			SLCHECK_Z(nfound_count);
		}
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
	}
	{
		// Изменить записи и убедиться, что она действительно изменилась
		uint   inprop_updated_count = 0;
		const  int trar = dbp.StartTransaction();
		SLCHECK_NZ(trar);
		if(trar) {
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
			const int cwr = dbp.CommitWork();
			SLCHECK_NZ(cwr);
			if(cwr) {
				SLCHECK_Z(inprop_updated_count);
			}
		}
	}
	{
		// Удалить каждую сотую запись и убедиться, что ее больше нет в таблице
		const  int trar = dbp.StartTransaction();
		SLCHECK_NZ(trar);
		if(trar) {
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
			const int cwr = dbp.CommitWork();
			SLCHECK_NZ(cwr);
			if(cwr) {
				SLCHECK_Z(deleted_found_fault_count);
				SLCHECK_Z(_found_fault_count);
			}
		}
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
