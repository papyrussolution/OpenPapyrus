// TEST-FTS.CPP
// Copyright (c) A.Sobolev 2023, 2025
// @codepage UTF-8
// Модуль тестирования полнотекстового поиска на базе xapian
//
#include <pp.h>
#pragma hdrstop

class TestFtsThread_Reader : public PPThread {
public:
	TestFtsThread_Reader(const StrAssocArray & rTestList, long lockTimeout) : PPThread(PPThread::kUnknown, "test-fts-reader", 0), TestList(rTestList), LockTimeout(lockTimeout)
	{
		InitStartupSignal();
	}
private:
	virtual void Startup()
	{
		PPThread::Startup();
		SignalStartup();
	}
	virtual void Run()
	{
		SString temp_buf;
		{
			SString line_buf;
			SString name_en;
			SString name_ru;
			PPFtsInterface r(0/*pDbLoc*/, false, LockTimeout);
			assert(r);
			THROW(r);
			for(uint j = 0; j < 20; j++) {
				const uint id = SLS.GetTLA().Rg.GetUniformInt(TestList.getCount())+1;
				assert(id > 0 && id <= TestList.getCount());
				const StrAssocArray::Item item = TestList.Get(id-1);
				bool  found = false;
				line_buf = item.Txt;
				assert(line_buf.NotEmpty());
				assert(line_buf.Divide(';', name_en, name_ru) > 0);
				name_ru.Strip();
				name_en.Strip();
				TSCollection <PPFtsInterface::SearchResultEntry> search_result_list;
				int sr = r.Search(name_ru, 10, search_result_list);
				//assert(sr > 0);
				for(uint idx = 0; !found && idx < search_result_list.getCount(); idx++) {
					const PPFtsInterface::SearchResultEntry * p_se = search_result_list.at(idx);
					assert(p_se);
					if(p_se->ObjId == id)
						found = true;
				}
				//assert(found);
			}
		}
		CATCH
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
		ENDCATCH
	}
	const long LockTimeout;
	const StrAssocArray TestList;
};

class TestFtsThread_Writer : public PPThread {
public:
	TestFtsThread_Writer(const char * pFileName, long lockTimeout) : PPThread(PPThread::kUnknown, "test-fts-writer", 0), SrcFileName(pFileName), LockTimeout(lockTimeout)
	{
		InitStartupSignal();
	}
private:
	virtual void Startup()
	{
		PPThread::Startup();
		SignalStartup();
	}
	virtual void Run()
	{
		SString temp_buf;
		uint   line_no = 0;
		SFile f_in(SrcFileName, SFile::mRead);
		assert(f_in.IsValid());
		{
			SString line_buf;
			SString name_en;
			SString name_ru;
			PPFtsInterface w(0/*pDbLoc*/, true, LockTimeout);
			assert(w.IsWriter());
			THROW(w.IsWriter());
			assert(w);
			THROW(w);
			{
				PPFtsInterface::TransactionHandle tra(w);
				StringSet ss;
				line_no = 0;
				THROW(tra);
				while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
					line_no++;
					if(line_buf.NotEmpty()) {
						if(line_buf.Divide(';', name_en, name_ru) > 0) {
							name_ru.Strip().Transf(CTRANSF_OUTER_TO_UTF8);
							name_en.Strip().Transf(CTRANSF_OUTER_TO_UTF8);
							PPFtsInterface::Entity ent;
							ent.Scope = PPFtsInterface::scopeTest;
							ent.ScopeIdent = SrcFileName;
							ent.ObjType = PPOBJ_CITY;
							ent.ObjId = line_no;
							ss.Z();
							ss.add(name_en);
							ss.add(name_ru);
							THROW(tra.PutEntity(ent, ss, 0));
						}
					}
				}
				THROW(tra.Commit());
			}
		}
		CATCH
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
		ENDCATCH
	}
	const long LockTimeout;
	const SString SrcFileName;
};

SLTEST_R(PPFtsInterface)
{
	// D:\Papyrus\Src\PPTEST\DATA\city-enru-pair.csv 
	int    ok = 1;
	uint   line_no = 0;
	SString path_in = GetSuiteEntry()->InPath;
	SString src_file_name;
	SString db_loc;
	StrAssocArray test_list;
	(db_loc = GetSuiteEntry()->OutPath).SetLastSlash().Cat("fts-db-loc");
	{
		const char * p_file_name = "city-enru-pair.csv";
		(src_file_name = path_in).SetLastSlash().Cat(p_file_name); // ANSI-codepage (windows-1251)
		SFile f_in(src_file_name, SFile::mRead);
		if(f_in.IsValid()) {
			SString line_buf;
			SString name_en;
			SString name_ru;
			{
				
				PPFtsInterface w(db_loc, true, 0/*default*/);
				THROW(SLCHECK_NZ(w.IsWriter()));
				THROW(SLCHECK_NZ(w));
				{
					PPFtsInterface::TransactionHandle tra(w);
					StringSet ss;
					line_no = 0;
					THROW(SLCHECK_NZ(tra));
					while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
						line_no++;
						if(line_buf.NotEmpty()) {
							line_buf.Transf(CTRANSF_OUTER_TO_UTF8);
							if(line_buf.Divide(';', name_en, name_ru) > 0) {
								test_list.AddFast(line_no, line_buf);
								name_ru.Strip();
								name_en.Strip();
								PPFtsInterface::Entity ent;
								ent.Scope = PPFtsInterface::scopeTest;
								ent.ScopeIdent = p_file_name;
								ent.ObjType = PPOBJ_CITY;
								ent.ObjId = line_no;
								ss.Z();
								ss.add(name_en);
								ss.add(name_ru);
								THROW(SLCHECK_NZ(tra.PutEntity(ent, ss, 0)));
								if((line_no % 5000) == 0) {
									PPFtsInterface r(0/*pDbLoc*/, false, 0/*default*/);
									THROW(SLCHECK_Z(r));
								}
							}
						}
					}
					THROW(SLCHECK_NZ(tra.Commit()));
				}
			}
			// Нужно тестирование ReadWrite-блокировки с таймаутом
			{
				const uint max_thread_count = 40;
				uint   tc = 0;
				HANDLE thread_list[128];
				MEMSZERO(thread_list);
				for(uint i = 0; i < max_thread_count; i++) {
					if((i % 8) == 0) {
						TestFtsThread_Writer * p_writer = new TestFtsThread_Writer(src_file_name, 60000);
						assert(p_writer);
						p_writer->Start(1);
						thread_list[tc++] = *p_writer;
					}
					else {
						TestFtsThread_Reader * p_reader = new TestFtsThread_Reader(test_list, 60000);
						assert(p_reader);
						p_reader->Start(1);
						thread_list[tc++] = *p_reader;
					}
				}
				WaitForMultipleObjects(tc, thread_list, TRUE, INFINITE);
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
		ok = 0;
	ENDCATCH
	return ok;
}