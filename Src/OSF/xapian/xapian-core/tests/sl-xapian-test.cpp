// SL-XAPIAN-TEST.CPP
//
#include <xapian-internal.h>
#pragma hdrstop
#include <unicode\uclean.h>
#include <unicode\brkiter.h>

using namespace U_ICU_NAMESPACE;

static void Tests()
{
	int    ok = 1;
	const char * pp_text_file_names[] = { "rustext.txt", "phrases-ru-1251.txt", "person.txt", "address.txt" };
	SString base_path;
	SString file_path;
	STempBuffer raw_input_buf(SKILOBYTE(512));
	SStringU input_text_u;
	SString input_text_utf8;
	UErrorCode icu_status = U_ZERO_ERROR;
	u_init(&icu_status);
	Locale icu_locale("ru", "RU");
	//SLS.QueryPath("testroot", base_path);
	base_path = "\\papyrus\\src\\pptest";
	base_path.SetLastSlash().Cat("data").SetLastSlash();
	Xapian::WritableDatabase db("xapian-test-db", Xapian::DB_CREATE_OR_OPEN, 0);
	static const wchar_t p_puncts[] = { L'.', L',', L';', L':', L' ', L'\t', L'\n', L'-', L'(', L')', L'[', L']', L'{', L'}' };
	for(uint i = 0; i < SIZEOFARRAY(pp_text_file_names); i++) {
		Xapian::Document doc;
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
					size_t needed_size = file_size + 512; // 512 - insuring
					size_t actual_size;
					THROW(raw_input_buf.AllocIncr(needed_size));
					if(f_in.Read(raw_input_buf, static_cast<size_t>(file_size), &actual_size)) {
						BreakIterator * p_bi = BreakIterator::createWordInstance(icu_locale, icu_status);
						if(U_SUCCESS(icu_status)) {
							//input_text_utf8.Z().CatN(raw_input_buf, actual_size).Transf(CTRANSF_OUTER_TO_UTF8);
							input_text_u.CopyFromMb(cp1251, raw_input_buf, actual_size);
							
							UnicodeString us(input_text_u);
							//input_text.Z().CopyFromMb(cp1251, raw_input_buf, actual_size);
							p_bi->setText(us);
							int32 prev_boundary = 0;
							int32 p = p_bi->first();
							if(p != BreakIterator::DONE) do {
								//
								const auto word = us.tempSubStringBetween(prev_boundary, p);
								char buffer[16384];
								bool skip = false;
								if(word.length() == 0)
									skip = true;
								else if(word.length() == 1) {
									for(uint pidx = 0; !skip && pidx < SIZEOFARRAY(p_puncts); pidx++) {
										if(word.charAt(0) == p_puncts[pidx])
											skip = true;
									}
								}
								if(!skip) {
									word.toUTF8(CheckedArrayByteSink(buffer, sizeof(buffer)));
									doc.add_term(buffer);
									//words.emplace_back(QString::fromUtf8(buffer));
									//
								}
								prev_boundary = p;
								p = p_bi->next();
							} while(p != BreakIterator::DONE);
						}
					}
				}
			}
			Xapian::docid did = db.add_document(doc);
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
}

int main(int argc, const char * ppArgv[])
{
	int    result = 0;
	SLS.Init("xapian-test", 0);
	Tests();
	return result;
}