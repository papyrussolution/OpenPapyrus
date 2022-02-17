// PPFTS.CPP
// Copyright (c) A.Sobolev 2022
// @codepage UTF-8
// @construction
// Полнотекстовый поиск
//
#include <pp.h>
#pragma hdrstop
#include <dbkv-lmdb.h>

//int  Test_Fts() { return 1; }
#if 1 // {

#include <xapian.h>
#include <unicode\uclean.h>
#include <unicode\brkiter.h>
//
// Xapian interface development
//
using namespace U_ICU_NAMESPACE;

int  Test_Fts()
{
	int    ok = 1;
	const char * pp_text_file_names[] = { "rustext.txt", "phrases-ru-1251.txt", "person.txt", "address.txt" };
	SString base_path;
	SString file_path;
	SString temp_buf;
	STempBuffer raw_input_buf(SKILOBYTE(512));
	SStringU input_text_u;
	SString input_text_utf8;
	UErrorCode icu_status = U_ZERO_ERROR;
	{
		//
		// Пробуем работать с базой данных LMDB
		//
		SString lmdb_path;
		PPGetPath(PPPATH_WORKSPACE, lmdb_path);
		lmdb_path.SetLastSlash().Cat("lmdb");
		createDir(lmdb_path);
		LmdbDatabase * p_db = LmdbDatabase::GetInstance(lmdb_path, 0, 0);
		if(p_db) {
			LmdbDatabase * p_db2 = LmdbDatabase::GetInstance(lmdb_path, 0, 0);
			assert(p_db2 == p_db);
			{
				uint put_err = 0;
				uint get_err = 0;
				uint get_cmp_err = 0;
				UuidArray uuidlist;
				{
					LmdbDatabase::Transaction txn(*p_db, false);
					LmdbDatabase::Table tbl(txn, "fts-supplement");
					if(tbl.IsValid()) {
						for(uint i = 0; i < 1000000; i++) {
							S_GUID uuid(SCtrGenerate_);
							temp_buf.Z().Cat(uuid);
							uuidlist.insert(&uuid);
							if(!tbl.Put(&uuid, sizeof(uuid), temp_buf.cptr(), temp_buf.Len(), 0))
								put_err++;
						}
						txn.Commit();
					}
				}
				{
					SString result_val_text;
					LmdbDatabase::Transaction txn(*p_db, true);
					LmdbDatabase::Table tbl(txn, "fts-supplement");
					if(tbl.IsValid()) {
						uuidlist.shuffle();
						for(uint i = 0; i < uuidlist.getCount(); i++) {
							S_GUID uuid = uuidlist.at(i);
							temp_buf.Z().Cat(uuid);
							SBaseBuffer val_buf;
							int r = tbl.Get(&uuid, sizeof(uuid), val_buf);
							if(r > 0) {
								assert(val_buf.P_Buf != 0);
								result_val_text.Z().CatN(val_buf.P_Buf, val_buf.Size);
								if(temp_buf != result_val_text) {
									get_cmp_err++;
								}
							}
							else {
								get_err++;
							}
						}
					}
				}
			}
		}
	}
	{
		PPGetPath(PPPATH_BIN, temp_buf);
		u_setDataDirectory(temp_buf);
		u_init(&icu_status);
	}
	Locale icu_locale("ru", "RU");
	//SLS.QueryPath("testroot", base_path);
	base_path = "\\papyrus\\src\\pptest";
	base_path.SetLastSlash().Cat("data").SetLastSlash();
	{
		PPGetFilePath(PPPATH_OUT, "fts-token-list.txt", temp_buf);
		SFile f_tokens(temp_buf, SFile::mWrite);
		Xapian::WritableDatabase db("xapian-test-db", Xapian::DB_CREATE_OR_OPEN, 0);
		static const wchar_t p_puncts[] = { L'.', L',', L';', L':', L' ', L'\t', L'\n', L'-', L'(', L')', L'[', L']', L'{', L'}', L'!', L'\"', L'+', L'/', L'=', L'?', L'\\', L'|', L'№' };
		for(uint i = 0; i < SIZEOFARRAY(pp_text_file_names); i++) {
			(file_path = base_path).Cat(pp_text_file_names[i]);
			SFile f_in(file_path, SFile::mRead);
			if(f_in.IsValid()) {
				int64 file_size = 0;
				if(f_in.CalcSize(&file_size)) {
					assert(file_size >= 0);
					if(file_size == 0) {
						; // file is empty
					}
					else if(file_size > SMEGABYTE(128)) {
						; // file too big
					} 
					else {
						size_t needed_size = static_cast<size_t>(file_size + 512); // 512 - insuring
						size_t actual_size;
						Xapian::Document doc;
						db.begin_transaction(true);
						THROW(raw_input_buf.AllocIncr(needed_size));
						if(f_in.Read(raw_input_buf, static_cast<size_t>(file_size), &actual_size)) {
							BreakIterator * p_bi = BreakIterator::createWordInstance(icu_locale, icu_status);
							if(U_SUCCESS(icu_status)) {
								f_tokens.WriteLine((temp_buf = file_path).CR()); // @debug
								//input_text_utf8.Z().CatN(raw_input_buf, actual_size).Transf(CTRANSF_OUTER_TO_UTF8);
								input_text_u.CopyFromMb(cp1251, raw_input_buf, actual_size);
								UnicodeString us(input_text_u);
								//input_text.Z().CopyFromMb(cp1251, raw_input_buf, actual_size);
								p_bi->setText(us);
								int32 prev_boundary = 0;
								int32 p = p_bi->first();
								if(p != BreakIterator::DONE) do {
									//
									auto word = us.tempSubStringBetween(prev_boundary, p);
									char buffer[16384];
									bool skip = false;
									word.trim();
									if(word.length() == 0)
										skip = true;
									else if(word.length() == 1) {
										for(uint pidx = 0; !skip && pidx < SIZEOFARRAY(p_puncts); pidx++) {
											if(word.charAt(0) == p_puncts[pidx])
												skip = true;
										}
									}
									if(!skip) {
										word.toLower();
										word.toUTF8(CheckedArrayByteSink(buffer, sizeof(buffer)));
										doc.add_term(buffer);
										f_tokens.WriteLine((temp_buf = buffer).CR());
										//words.emplace_back(QString::fromUtf8(buffer));
										//
									}
									prev_boundary = p;
									p = p_bi->next();
								} while(p != BreakIterator::DONE);
							}
						}
						Xapian::docid did = db.add_document(doc);
						db.commit_transaction();
					}
				}
			}
		}
	}
	{
		bool debug_mark = false;
		SStringU qb_u;
		std::string found_doc_text;
		Xapian::Database db("xapian-test-db", Xapian::DB_OPEN);
		Xapian::Enquire enq(db);
		Xapian::QueryParser qp;
		qp.set_database(db);
		temp_buf = "автоматически";
		qb_u.CopyFromUtf8(temp_buf);
		Xapian::Query q = qp.parse_query(temp_buf.cptr());
		std::string qdescr = q.get_description();
		//
		enq.set_query(q);
		Xapian::MSet ms = enq.get_mset(0, 10);
		for(Xapian::MSetIterator i = ms.begin(); i != ms.end(); ++i) {
			Xapian::doccount rank = i.get_rank();
			double wt = i.get_weight();
			Xapian::docid did = *i;
			found_doc_text = i.get_document().get_data();
			//cout << i.get_rank() + 1 << ": " << i.get_weight() << " docid=" << *i << " [" << i.get_document().get_data() << "]\n\n";			
		}
		debug_mark = true;
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}
#endif // } 0