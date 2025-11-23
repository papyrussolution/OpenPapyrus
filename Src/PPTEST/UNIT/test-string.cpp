// TEST-STRING.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025
// @codepage UTF-8
// Тестирование функций класса SString
//
#include <pp.h>
#pragma hdrstop
#include <string>

const char * byteshift_strstr(const char * pHayStack, const char * pNeedle); // @prototype(stext.cpp)

struct SlTestFixtureSString {
public:
	SlTestFixtureSString() : P_StrList(0), MaxStrListItemLen(0)
	{
	}
	~SlTestFixtureSString()
	{
		delete P_StrList;
	}
	int    FASTCALL InitStrList(const char * pFileName)
	{
		// phrases.en
		int    ok = 1;
        if(!P_StrList) {
			MaxStrListItemLen = 0;
			THROW(P_StrList = new SStrCollection);
			{
				SString temp_buf;
				SFile f_in(pFileName, SFile::mRead);
				THROW(f_in.IsValid());
				while(f_in.ReadLine(temp_buf)) {
					char * p_new_str = newStr(temp_buf);
					THROW(p_new_str);
					THROW(P_StrList->insert(p_new_str));
					{
						const size_t len = sstrlen(p_new_str);
						SETMAX(MaxStrListItemLen, len);
					}
				}
			}
		}
		else
			ok = -1;
        CATCHZOK
        return ok;
	}
	int    FASTCALL InitRandomRealList(uint maxCount)
	{
		int    ok = 1;
		SString temp_buf;
		if(!RandomRealList.getCount()) {
			SlThreadLocalArea & r_tla = SLS.GetTLA();
			for(uint j = 0; j < maxCount; j++) {
				//double r = r_tla.Rg.GetReal();
				double r = r_tla.Rg.GetGaussian(1.0E+9);
				RandomRealList.insert(&r);
				temp_buf.Z().Cat(r, MKSFMTD(0, 32, NMBF_NOTRAILZ));
				SsRrl.add(temp_buf);
			}
		}
		else
			ok = -1;
		return ok;
	}
    SStrCollection * P_StrList;
	RealArray RandomRealList;
	StringSet SsRrl;
	size_t MaxStrListItemLen; // @v11.7.10 Для оценки производительности smemchr(ptr, 0, size)
};

class OtherImplementation_String {
public:
	// скопировал различные реализации для теста и, если тест отрабатывает, то заменяю эти реализации на sstrnlen
	static size_t OPENSSL_strnlen(const char * str, size_t maxlen)
	{
		const char * p;
		for(p = str; maxlen-- != 0 && *p != 0; ++p)
			;
		return p - str;
	}
	static size_t tftp_strnlen(const char * string, size_t maxlen)
	{
		const char * end = (const char *)smemchr(string, '\0', maxlen);
		return end ? (size_t)(end - string) : maxlen;
	}
	static int32_t u_astrnlen(const char * s1, int32_t n) // icu
	{
		int32_t len = 0;
		if(s1) {
			while(n-- && *(s1++)) {
				len++;
			}
		}
		return len;
	}
};

SLTEST_FIXTURE(SString, SlTestFixtureSString)
{
	// benchmark: benchmark=stack;sstring;revolver;atof;satof
	int    ok = 1;
	int    bm = -1;
	const  uint max_bm_phase = 10000;
	const  uint max_strlen_phase = 100000;
	uint32 line_no = 0;
	SBaseBuffer temp_buf;
	SBaseBuffer test_buf;
	temp_buf.Init();
	test_buf.Init();
	SString str;
	SString out_buf;
	SString in_buf;
	const int islr = F.InitStrList(MakeInputFilePath("phrases.en"));
    THROW(SLCHECK_NZ(islr));
	F.InitRandomRealList(1000000);
	if(pBenchmark == 0) bm = 0;
	else if(sstreqi_ascii(pBenchmark, "stack"))        bm = 1;
	else if(sstreqi_ascii(pBenchmark, "stack-xeos"))   bm = 6;
	else if(sstreqi_ascii(pBenchmark, "sstring"))      bm = 2;
	else if(sstreqi_ascii(pBenchmark, "std::string"))  bm = 5;
	else if(sstreqi_ascii(pBenchmark, "revolver"))     bm = 3;
	else if(sstreqi_ascii(pBenchmark, "revolver-tla")) bm = 4;
	else if(sstreqi_ascii(pBenchmark, "atof"))         bm = 7;
	else if(sstreqi_ascii(pBenchmark, "satof"))        bm = 8;
	else if(sstreqi_ascii(pBenchmark, "strlen"))       bm = 9;
	else if(sstreqi_ascii(pBenchmark, "sstrlen"))      bm = 10;
	else if(sstreqi_ascii(pBenchmark, "smemchr0"))     bm = 11; // @v11.7.10
	else if(sstreqi_ascii(pBenchmark, "strnlen"))      bm = 12; // @v11.7.10
	else if(sstreqi_ascii(pBenchmark, "sstrnlen"))     bm = 13; // @v11.7.10
	else SetInfo("invalid benchmark");
	if(bm == 0) {
		{
			// Тестирование функций sstrlen и sstrnlen
			SLCHECK_EQ(sstrlen(static_cast<const char *>(0)), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(static_cast<const uchar *>(0)), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(static_cast<const wchar_t *>(0)), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(static_cast<const char16_t *>(0)), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(static_cast<const ushort *>(0)), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(static_cast<const short *>(0)), static_cast<size_t>(0));

			SLCHECK_EQ(sstrlen(static_cast<const char *>("")), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const uchar *>("")), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(static_cast<const wchar_t *>(L"")), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const char16_t *>(L"")), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const ushort *>(L"")), static_cast<size_t>(0));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const short *>(L"")), static_cast<size_t>(0));

			SLCHECK_EQ(sstrlen(static_cast<const char *>("abc")), static_cast<size_t>(3U));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const uchar *>("abc")), static_cast<size_t>(3U));
			SLCHECK_EQ(sstrlen(static_cast<const wchar_t *>(L"abc")), static_cast<size_t>(3U));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const char16_t *>(L"abc")), static_cast<size_t>(3U));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const ushort *>(L"abc")), static_cast<size_t>(3U));
			SLCHECK_EQ(sstrlen(reinterpret_cast<const short *>(L"abc")), static_cast<size_t>(3U));
			{
				constexpr size_t buf_len = SMEGABYTE(20) + 37;
				constexpr size_t delta = 6;
				char * p_very_long_string = static_cast<char *>(SAlloc::M(buf_len));
				SLCHECK_NZ(p_very_long_string);
				if(p_very_long_string) {
					SObfuscateBuffer(p_very_long_string, buf_len);
					char * p = p_very_long_string;
					do {
						p = static_cast<char *>(const_cast<void *>(smemchr(p, 0, buf_len - (p - p_very_long_string))));
						if(p)
							*p++ = '\x01';
					} while(p && (p - p_very_long_string) < buf_len);
					assert(smemchr(p_very_long_string, 0, buf_len) == 0);
					p_very_long_string[buf_len-delta] = 0;

					SLCHECK_EQ(sstrlen(p_very_long_string), buf_len-delta);
					SLCHECK_EQ(sstrlen(reinterpret_cast<const uchar *>(p_very_long_string)), buf_len-delta);
					for(uint j = 0; j < SKILOBYTE(1); j++) {
						SLCHECK_EQ(sstrnlen(p_very_long_string, j), static_cast<size_t>(j));
						SLCHECK_EQ(sstrnlen(reinterpret_cast<const uchar *>(p_very_long_string), j), static_cast<size_t>(j));

						SLCHECK_EQ(OtherImplementation_String::OPENSSL_strnlen(p_very_long_string, j), static_cast<size_t>(j));
						SLCHECK_EQ(OtherImplementation_String::tftp_strnlen(p_very_long_string, j), static_cast<size_t>(j));
						SLCHECK_EQ(OtherImplementation_String::u_astrnlen(p_very_long_string, j), (int)j);
					}
					SLCHECK_EQ(sstrnlen(p_very_long_string, buf_len), buf_len-delta);
					SLCHECK_EQ(sstrnlen(reinterpret_cast<const uchar *>(p_very_long_string), buf_len), buf_len-delta);

					SLCHECK_EQ(OtherImplementation_String::OPENSSL_strnlen(p_very_long_string, buf_len), buf_len-delta);
					SLCHECK_EQ(OtherImplementation_String::tftp_strnlen(p_very_long_string, buf_len), buf_len-delta);
					SLCHECK_EQ(OtherImplementation_String::u_astrnlen(p_very_long_string, buf_len), (int)(buf_len-delta));

					ZFREE(p_very_long_string);
				}
			}
		}
		{
			// @v11.9.4 Ситуативный тест проверки преобразования double в строку
			double qtty = 0.3;
			str.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ|NMBF_OMITEPS));
			SLCHECK_EQ(str, "0.3");
			qtty = 3.0;
			str.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ|NMBF_OMITEPS));
			SLCHECK_EQ(str, "3");
		}
		{
			//
			// Тестирование функций сравнения строк
			//
			// strcmp с нулевыми указателями не работает! SLCHECK_Z(strcmp("", 0));
			SLCHECK_NZ(sstreq("", static_cast<const char *>(0)));
			SLCHECK_NZ(sstreq(static_cast<const char *>(0), ""));
			SLCHECK_Z(sstreq(static_cast<const char *>(0), "ab"));
			SLCHECK_Z(sstreq("b", static_cast<const char *>(0)));
			SLCHECK_NZ(sstreq("ab", "ab"));
			SLCHECK_NZ(sstreq("", ""));
			SLCHECK_Z(sstreq("ab", "abc"));
			SLCHECK_NZ(sstreq("король и королева", SString("король").Space().Cat("и").CatChar(' ').Cat("королева")));
			SLCHECK_Z(sstreq("король и королева", "король и королёва"));
			SLCHECK_NZ(sstreq(L"король и королева", L"король и королева"));
			SLCHECK_NZ(sstreq(L"robot", SStringU(L"robot")));
			//
			SLCHECK_Z(stricmp866(0, ""));
			SLCHECK_Z(stricmp866("", 0));
			SLCHECK_Z(stricmp1251(0, ""));
			SLCHECK_Z(stricmp1251("", 0));
			// Attention! Следующие тесты действительны до тех пор, пока выполняется соответствие INNER-кодировка == cp866, OUTER-кодировка == cp1251
			SLCHECK_Z(stricmp866(SString("Король и Королева").Transf(CTRANSF_UTF8_TO_INNER), SString("король и королева").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(stricmp866(SString("Король и КоролЁва").Transf(CTRANSF_UTF8_TO_INNER), SString("король и королёва").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(stricmp1251(SString("Король и Королева").Transf(CTRANSF_UTF8_TO_OUTER), SString("король и королева").Transf(CTRANSF_UTF8_TO_OUTER)));
			SLCHECK_Z(stricmp1251(SString("Король и Королёва").Transf(CTRANSF_UTF8_TO_OUTER), SString("король и королЁва").Transf(CTRANSF_UTF8_TO_OUTER)));
		}
		{
			SFile out(MakeOutputFilePath("SString_NumberToLat.txt"), SFile::mWrite);
			for(uint i = 0; i < 1000; i++) {
				str.Z().NumberToLat(i);
				out.WriteLine(out_buf.Printf("%u\t\t%s\n", i, str.cptr()));
			}
			{
				SLCHECK_NZ(sstreqi_ascii("u", "u"));
				SLCHECK_NZ(sstreqi_ascii("u", "U"));
				SLCHECK_NZ(sstreqi_ascii("U", "u"));
				SLCHECK_Z(sstreqi_ascii("U", "u1"));
				SLCHECK_NZ(sstreqi_ascii("string", "sTrInG"));
				SLCHECK_NZ(sstreqi_ascii("string", "string"));
				SLCHECK_Z(sstreqi_ascii("string", "string "));
			}
		}
		//
		// Тестирование функций HasPrefix() и CmpPrefix()
		//
		{
			SString prefix;
			SString text("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz");
			SLCHECK_NZ(text.HasPrefix("A"));
			SLCHECK_NZ(text.HasPrefix("ABC"));
			SLCHECK_NZ(text.HasPrefix("ABCD"));
			SLCHECK_Z(text.HasPrefix("a"));
			SLCHECK_Z(text.HasPrefix("abc"));
			SLCHECK_Z(text.HasPrefix("aBcd"));
			SLCHECK_NZ(text.HasPrefixNC("A"));
			SLCHECK_NZ(text.HasPrefixNC("ABC"));
			SLCHECK_NZ(text.HasPrefixNC("ABCD"));
			SLCHECK_NZ(text.HasPrefixNC("a"));
			SLCHECK_NZ(text.HasPrefixNC("AbC"));
			SLCHECK_NZ(text.HasPrefixNC("AbCd"));
			SLCHECK_Z(text.HasPrefixNC("QUQU"));
			SLCHECK_NZ(text.HasPrefixIAscii("A"));
			SLCHECK_NZ(text.HasPrefixIAscii("ABC"));
			SLCHECK_NZ(text.HasPrefixIAscii("ABCD"));
			SLCHECK_NZ(text.HasPrefixIAscii("a"));
			SLCHECK_NZ(text.HasPrefixIAscii("AbC"));
			SLCHECK_NZ(text.HasPrefixIAscii("AbCd"));

			(text = "На дворе трава на траве дрова").Transf(CTRANSF_UTF8_TO_INNER);
			SLCHECK_NZ(text.HasPrefix((prefix = "Н").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_NZ(text.HasPrefix((prefix = "На дворе").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_NZ(text.HasPrefix((prefix = "На дворе трава").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefix((prefix = "дрова").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefix((prefix = "НА ДВОРЕ ТРАВА").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_NZ(text.HasPrefixNC((prefix = "н").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_NZ(text.HasPrefixNC((prefix = "На дВоре").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_NZ(text.HasPrefixNC((prefix = "На дворе трАва").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_NZ(text.HasPrefixNC((prefix = "НА ДВОРЕ ТРАВА").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefixNC((prefix = "ДРОВА").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefixIAscii((prefix = "н").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefixIAscii((prefix = "На дВоре").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefixIAscii((prefix = "На дворе трАва").Transf(CTRANSF_UTF8_TO_INNER)));
			SLCHECK_Z(text.HasPrefixIAscii((prefix = "НА ДВОРЕ ТРАВА").Transf(CTRANSF_UTF8_TO_INNER)));
		}
		//
		// Тестирование функции CopyTo
		//
		{
			char   buffer[1024];
			char   preserve_buffer[sizeof(buffer)];
			SObfuscateBuffer(buffer, sizeof(buffer));
			memcpy(preserve_buffer, buffer, sizeof(preserve_buffer));
			//
			{
				const size_t _src_len = 503;
				str.Z();
				for(uint si = 0; si < _src_len; si++) {
					uint _v = SLS.GetTLA().Rg.GetUniformInt(100000);
					char _c = (_v % 26) + ((si & 1) ? 'a' : 'A');
					str.CatChar(_c);
				}
				SLCHECK_EQ(str.Len(), _src_len);
			}
			for(uint tl = 1; tl <= sizeof(buffer); tl++) {
				str.CopyTo(buffer, tl);
				if(tl > 1) {
					SLCHECK_Z(str.CmpPrefix(buffer, 0));
					SLCHECK_NZ(str.HasPrefix(buffer));
				}
				{
					const size_t real_data_len = MIN(tl, str.Len()+1);
					SLCHECK_Z(memcmp(str.cptr(), buffer, real_data_len-1));
					SLCHECK_EQ((uint8)buffer[real_data_len-1], static_cast<uint8>(0));
					SLCHECK_Z(memcmp(buffer+real_data_len, preserve_buffer+real_data_len, sizeof(buffer)-real_data_len));
				}
			}
			{
				str.CopyTo(buffer, 0);
				SLCHECK_Z(str.CmpPrefix(buffer, 0));
				SLCHECK_NZ(str.HasPrefix(buffer));
				SLCHECK_Z(memcmp(str.cptr(), buffer, str.Len()));
				SLCHECK_EQ(strlen(buffer), str.Len());
				SLCHECK_EQ((uint8)buffer[str.Len()], static_cast<uint8>(0));
				SLCHECK_Z(memcmp(buffer+str.Len()+1, preserve_buffer+str.Len()+1, sizeof(buffer)-str.Len()-1));
			}
		}
		{
			//
			// Тестирование функции Reverse()
			//
			const char * p_pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
			const size_t pattern_len = sstrlen(p_pattern);
			SString rev_str;
			for(size_t lenidx = 0; lenidx <= pattern_len; lenidx++) {
				str.Z().Cat(p_pattern+lenidx);
				const size_t org_len = str.Len();
				(rev_str = str).Reverse();
				SLCHECK_EQ(org_len, rev_str.Len());
				SLCHECK_EQ(org_len, str.Len());
				for(uint i = 0; i < org_len; i++) {
					SLCHECK_EQ(static_cast<uint8>(str.C(i)), static_cast<uint8>(rev_str.C(org_len-i-1)));
				}
			}
		}
		{
			// Тестирование функции Chomp
			SString org_buf;
			static const SStrToStrAssoc case_list[] = {
				{ "", "" },
				{ "\xD\xA", "" },
				{ "\xD", "" },
				{ "\xA", "" },
				{ "курвуазье ", "курвуазье " },
				{ "курвуазье \xD\xA", "курвуазье " },
				{ "курвуазье \xD", "курвуазье " },
				{ "курвуазье \xA", "курвуазье " }
			};
			for(uint i = 0; i < SIZEOFARRAY(case_list); i++) {
				org_buf = case_list[i].Key;
				SLCHECK_EQ(org_buf.Chomp(), case_list[i].Val);
			}
		}
		{
			//
			// Тестирование функций Search
			//
			const char * p_text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz АБРИКОС городовой";
			const char * p_temp_text = 0;
			SStringU text_u;
			SStringU temp_buf_u;
			text_u.CopyFromUtf8(p_text, sstrlen(p_text));
			size_t pos;
			SLCHECK_NZ(text_u.Search(L"ABC", 0, &pos));
			SLCHECK_EQ(pos, static_cast<size_t>(0U));
			SLCHECK_Z(text_u.Search(L"ABC", 10, &pos));
			p_temp_text = "АБРИК";
			temp_buf_u.CopyFromUtf8(p_temp_text, sstrlen(p_temp_text));
			SLCHECK_NZ(text_u.Search(temp_buf_u, 0, &pos));
			SLCHECK_EQ(pos, static_cast<size_t>(63U));
			SLCHECK_NZ(text_u.Search(temp_buf_u, 20, &pos));
			SLCHECK_EQ(pos, static_cast<size_t>(63U));
			temp_buf_u.CopyFromUtf8(p_text, sstrlen(p_text));
			SLCHECK_NZ(text_u.Search(temp_buf_u, 0, &pos));
			SLCHECK_EQ(pos, static_cast<size_t>(0U));
			p_temp_text = "городовой";
			temp_buf_u.CopyFromUtf8(p_temp_text, sstrlen(p_temp_text));
			SLCHECK_NZ(text_u.Search(temp_buf_u, 0, &pos));
			SLCHECK_EQ(pos, static_cast<size_t>(71U));
			SLCHECK_NZ(text_u.Search(temp_buf_u, 71, &pos));
			SLCHECK_EQ(pos, static_cast<size_t>(71U));
			SLCHECK_Z(text_u.Search(temp_buf_u, 1000, &pos));
			p_temp_text = "гороДовой";
			temp_buf_u.CopyFromUtf8(p_temp_text, sstrlen(p_temp_text));
			SLCHECK_Z(text_u.Search(temp_buf_u, 1, &pos));
		}
		{
			SLCHECK_EQ((str = " abc ").Strip(1), " abc");
			SLCHECK_EQ((str = " abc ").Strip(2), "abc ");
			SLCHECK_EQ((str = " abc ").Strip(0), "abc");
			SLCHECK_EQ((str = " abc ").Strip(), "abc");
			SLCHECK_EQ((str = "abc").Strip(), "abc");
			SLCHECK_EQ((str = "\"abc\"").StripQuotes(), "abc");
			SLCHECK_EQ((str = " \"abc\"" ).StripQuotes(), "abc");
			SLCHECK_EQ((str = "abc").StripQuotes(), "abc");
		}
		{
			//
			// Тестирование IsEqiAscii
			//
			SLCHECK_NZ((str = "").IsEqiAscii(0));
			SLCHECK_NZ((str = "").IsEqiAscii(""));
			SLCHECK_NZ((str = " ").IsEqiAscii(" "));
			SLCHECK_NZ((str = "abc").IsEqiAscii("abc"));
			SLCHECK_NZ((str = "abc").IsEqiAscii("aBc"));
			SLCHECK_NZ((str = "городовой").IsEqiAscii("городовой"));
			SLCHECK_Z((str = "городовой").IsEqiAscii("гороДовой"));
		}
		{
			//
			// Тестирование IsEqiUtf8 (не забываем, что кодировка исходного файла UTF8)
			//
			SLCHECK_NZ((str = "").IsEqiUtf8(0));
			SLCHECK_NZ((str = "").IsEqiUtf8(""));
			SLCHECK_NZ((str = " ").IsEqiUtf8(" "));
			SLCHECK_NZ((str = "abc").IsEqiUtf8("abc"));
			SLCHECK_NZ((str = "abc").IsEqiUtf8("aBc"));
			SLCHECK_NZ((str = "городовой").IsEqiUtf8("городОвой"));
			SLCHECK_NZ((str = "штрихкод").IsEqiUtf8("Штрихкод"));
			SLCHECK_NZ((str = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя").IsEqiUtf8("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"));
			SLCHECK_NZ((str = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ").IsEqiUtf8("абвгдеёжзийклмнопрстуфхцчшщъыьэюя"));
			SLCHECK_Z((str = "городовой").IsEqiUtf8("городОвой-"));
			SLCHECK_Z((str = "городовой").IsEqiUtf8("городoвой")); // во втором слове русская 'о' заменена на латинскую 'o'
			SLCHECK_Z((str = "").IsEqiUtf8(" "));
		}
		{
			//
			// Тестирование функции ReplaceStr
			//
			(str = "abcabc").ReplaceStr("abc", "abcd", 0);
			SLCHECK_EQ(str, "abcdabcd");
			(str = "  ab  cab  c  ").ElimDblSpaces();
			SLCHECK_EQ(str, " ab cab c ");
			(str = "$ab$$cab$c$").ReplaceStr("$", "$$", 0);
			SLCHECK_EQ(str, "$$ab$$$$cab$$c$$");
		}
		//
		// Тестирование функций EncodeMime64 и DecodeMime64
		//
		{
			SString org_bin_file_name;
			SString test_bin_file_name;
			size_t actual_size = 0;
			THROW(SLCHECK_NZ(temp_buf.Alloc(4096)));
			THROW(SLCHECK_NZ(test_buf.Alloc(8192)));
			{
				int r;
				org_bin_file_name = MakeInputFilePath("binfile.");
				SFile inf(org_bin_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd);
				SFile outf(MakeOutputFilePath("txtfile.temp"), SFile::mWrite);
				THROW(SLCHECK_NZ(inf.IsValid()));
				THROW(SLCHECK_NZ(outf.IsValid()));
				while((r = inf.Read(temp_buf.P_Buf, temp_buf.Size, &actual_size)) != 0) {
					size_t as = 0;
					str.EncodeMime64(temp_buf.P_Buf, actual_size).CR();
					str.DecodeMime64(test_buf.P_Buf, test_buf.Size, &as);
					THROW(SLCHECK_EQ(actual_size, as));
					THROW(SLCHECK_Z(memcmp(temp_buf.P_Buf, test_buf.P_Buf, as)));
					THROW(SLCHECK_NZ(outf.WriteLine(str)));
					if(r < 0)
						break;
				}
			}
			//
			{
				(in_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("txtfile.temp");
				(test_bin_file_name = GetSuiteEntry()->OutPath).SetLastSlash().Cat("binfile.temp");
				SFile inf(in_buf, SFile::mRead);
				SFile outf(test_bin_file_name, SFile::mWrite|SFile::mBinary);
				THROW(SLCHECK_NZ(inf.IsValid()));
				THROW(SLCHECK_NZ(outf.IsValid()));
				THROW(SLCHECK_NZ(temp_buf.Alloc(8192)));
				while(inf.ReadLine(str, SFile::rlfChomp)) {
					THROW(SLCHECK_NZ(str.DecodeMime64(temp_buf.P_Buf, temp_buf.Size, &actual_size)));
					THROW(SLCHECK_NZ(outf.Write(temp_buf.P_Buf, actual_size)));
				}
			}
			SLCHECK_LT(0, SFile::Compare(org_bin_file_name, test_bin_file_name, 0));
			//
			// Тестирование функций конвертации между различными кодировкам
			// Файл rustext.txt должен быть размером более 2048 байт и содержать русский
			// текст в кодировке windows-1251
			//
			{
				(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("rustext.txt");
				SFile inf(in_buf, SFile::mRead);
				(in_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("rustext.out");
				SFile outf(in_buf, SFile::mWrite);
				in_buf = 0;
				while(inf.ReadLine(str)) {
					in_buf.Cat(str);
				}
				out_buf = in_buf;
				out_buf.ToUtf8();
				out_buf.Utf8ToOem();
				out_buf.Transf(CTRANSF_INNER_TO_OUTER);
				SLCHECK_Z(in_buf.Cmp(out_buf, 0));

				out_buf = in_buf;
				out_buf.ToUtf8();
				out_buf.Utf8ToChar();
				out_buf.ToOem();
				out_buf.Transf(CTRANSF_INNER_TO_OUTER);
				SLCHECK_Z(in_buf.Cmp(out_buf, 0));

				{
					SStringU s16;
					out_buf = in_buf;
					out_buf.ToUtf8();
					s16.CopyFromUtf8Strict(out_buf, out_buf.Len());
					s16.CopyToUtf8(out_buf, 1);
					out_buf.Utf8ToChar();
					out_buf.ToOem();
					out_buf.Transf(CTRANSF_INNER_TO_OUTER);
					SLCHECK_Z(in_buf.Cmp(out_buf, 0));
				}

				outf.WriteLine(out_buf);
			}
			//
			// Тестирование функции Tokenize
			//
			{
				(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("tokenize.txt");
				SFile inf(in_buf, SFile::mRead);
				StringSet ss_line(";"), ss_result(",");
				SString delim, src_buf, result_str;
				while(inf.ReadLine(str, SFile::rlfChomp|SFile::rlfStrip)) {
					if(str.NotEmpty()) {
						ss_line.setBuf(str);
						uint p = 0;
						if(ss_line.get(&p, src_buf)) {
							if(ss_line.get(&p, delim)) {
								if(ss_line.get(&p, result_str)) {
									ss_result.Z();
									src_buf.Tokenize(delim.NotEmpty() ? (const char *)delim : 0, ss_result);
									SLCHECK_NZ(result_str == ss_result.getBuf());
								}
							}
						}
					}
				}
			}
			//
			// Тестирование распознавания и валидации email адресов и одновременное тестирование функций
			// sstreq и sstreqi_ascii (воспользуемся теми фактами, что все адреса написаны латиницей и
			// имеют разбросанную длину).
			//
			{
				SStrScan scan;
				(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("email-list.txt");
				SFile inf(in_buf, SFile::mRead);
				line_no = 0;
				SLCHECK_NZ(sstreq(static_cast<const char *>(0), static_cast<const char *>(0)));
				SLCHECK_NZ(sstreqi_ascii(static_cast<const char *>(0), static_cast<const char *>(0)));
				SLCHECK_NZ(sstreqi_ascii((const uchar *)0, (const uchar *)0));
				while(inf.ReadLine(str, SFile::rlfChomp|SFile::rlfStrip)) {
					line_no++;
					if(str.NotEmpty()) {
						scan.Set(str, 0);
						int   emsr = scan.GetEMail(out_buf.Z());
						SLCHECK_NZ(emsr);
					}
					{
						out_buf = str;
						SLCHECK_NZ(sstreq(out_buf, str));
						SLCHECK_NZ(sstreq(str, str));
						out_buf.ToUpper();
						str.ToLower();
						SLCHECK_Z(sstreq(out_buf, str));
						SLCHECK_NZ(sstreqi_ascii(str, out_buf));
						out_buf.CatChar('x');
						SLCHECK_Z(sstreqi_ascii(str, out_buf));
						str.CatChar('y');
						SLCHECK_Z(sstreqi_ascii(str, out_buf));
					}
				}
			}
			{
				//
				// Тестирование различных функций класса SStrScan
				//
				SStrScan scan;
				//
				scan.Set("", 0);
				SLCHECK_Z(scan.GetQuotedString(SFileFormat::Csv, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(0U));
				SLCHECK_NZ(str.IsEmpty());
				//
				scan.Set("abc", 0);
				SLCHECK_Z(scan.GetQuotedString(SFileFormat::Csv, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(0U));
				SLCHECK_NZ(str.IsEmpty());
				//
				scan.Set("\"abc\"0123", 0);
				SLCHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(5U));
				SLCHECK_EQ(str, "abc");
				//
				scan.Set("\"ab\"\"c\"0123", 0);
				SLCHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(7U));
				SLCHECK_EQ(str, "ab\"c");
				//
				scan.Set("\"abc\"", 0);
				SLCHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(5U));
				SLCHECK_EQ(str, "abc");
				//
				scan.Set("\"ab\"\"c\"", 0);
				SLCHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(7U));
				SLCHECK_EQ(str, "ab\"c");
				//
				scan.Set("\"ab\"\"c\"", 0);
				SLCHECK_NZ(scan.GetQuotedString(SFileFormat::Unkn, str));
				SLCHECK_EQ(scan.Offs, static_cast<size_t>(4U));
				SLCHECK_EQ(str, "ab");
			}
			{
				//
				// Тест функции IsLegalUtf8
				//
				(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("phrases-ru-1251.txt");
				SFile inf(in_buf, SFile::mRead);
				while(inf.ReadLine(str, SFile::rlfChomp|SFile::rlfStrip)) {
					//const char * p_test_string_1251 = "Захавай еще этих тупых французских булок";
					//str = p_test_string_1251;
					if(str.NotEmpty()) {
						SLCHECK_Z(str.IsLegalUtf8());
						str.Transf(CTRANSF_OUTER_TO_UTF8);
						SLCHECK_NZ(str.IsLegalUtf8());
					}
				}
			}
			{
				SLCHECK_EQ((str = "0").ToLong(), 0L);
				SLCHECK_EQ((str = "abc").ToLong(), 0L);
				SLCHECK_EQ((str = "\t+100 ").ToLong(), 100L);
				SLCHECK_EQ((str = " -197 ").ToLong(), -197L);
				SLCHECK_EQ((str = "0xbeefZ").ToLong(), 0xbeefL);
				SLCHECK_EQ((str = " -0x2BcD7a92 ").ToLong(), -0x2BCD7A92L);
				SLCHECK_EQ((str = " - 17 ").ToLong(), 0L); // Между знаком - и числом не должно быть пробелов
				SLCHECK_EQ((str = "0").ToInt64(), 0LL);
				SLCHECK_EQ((str = "abc").ToInt64(), 0LL);
				SLCHECK_EQ((str = "\t 0x1ABCDEF234567890").ToInt64(), 0x1ABCDEF234567890LL);
				SLCHECK_EQ((str = "\t\t123000012878963").ToInt64(), 123000012878963LL);
				SLCHECK_EQ((str = "-123000012878963").ToInt64(), -123000012878963LL);
			}
			{
				const char * p_atof_tab[] = {
					"0", " 0.001 ", "  -1.0", "+.1", "0.15", "-0.177777777", "+0003.2533333", "0010.0001", "1E-3", ".1e+9",
					"-1.1", "1111111.315687841464787467897878", "-333333.7646878744111165464", "1548754.02117e-17"
				};
				for(uint i = 0; i < SIZEOFARRAY(p_atof_tab); i++) {
					const char * p_text = p_atof_tab[i];
					double v_satof;
					const double v_atof = atof(p_text);
					//SLCHECK_NZ(satof(p_text, &v_satof));
					//SLCHECK_EQ(v_satof, v_atof);

					const char * p_end = 0;
					int erange = 0;
					//SLCHECK_NZ(dconvstr_scan(p_text, &p_end, &v_satof, &erange));
					v_satof = satof(p_text); // @v10.7.9 dconvstr_scan-->satof
					SLCHECK_EQ(v_satof, v_atof);
				}
				SString atof_buf;
				for(uint j = 0; j < F.RandomRealList.getCount(); j++) {
					atof_buf.Z().Cat(F.RandomRealList.at(j), MKSFMTD(0, 20, NMBF_NOTRAILZ));
					double v_satof;
					const double v_atof = atof(atof_buf);
					const char * p_end = 0;
					int erange = 0;
					//SLCHECK_NZ(dconvstr_scan(atof_buf, &p_end, &v_satof, &erange));
					v_satof = satof(atof_buf); // @v10.7.9 dconvstr_scan-->satof
					//SLCHECK_NZ(satof(atof_buf, &v_satof));
					SLCHECK_EQ(v_satof, v_atof);
				}
			}
			{
				SLCHECK_EQ(str.Z().Cat(0.1, MKSFMTD_020), "0.10");
				SLCHECK_EQ(str.Z().Cat(17.1997, MKSFMTD(0, 3, NMBF_DECCOMMA)), "17,200");
				SLCHECK_EQ(str.Z().Cat(135.1997, MKSFMTD(0, 0, 0)), "135");
				SLCHECK_EQ(str.Z().Cat(3308.04, MKSFMTD(0, 8, NMBF_OMITEPS|NMBF_NOTRAILZ)), "3308.04");
				SLCHECK_EQ(str.Z().Cat(3308.039999999999512506, MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "3308.04");
				SLCHECK_EQ(str.Z().Cat(3308.04, MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "3308.04");
				SLCHECK_EQ(str.Z().Cat(2572.92*1.00000000001, MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "2572.92");
				SLCHECK_EQ(str.Z().Cat(369.900000000000102333,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "369.9");
				SLCHECK_EQ(str.Z().Cat(369.900000000000102333,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ|NMBF_DECCOMMA)), "369,9");
				SLCHECK_EQ(str.Z().Cat(369.000000000001023334,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "369");
				SLCHECK_EQ(str.Z().Cat(369.000000000001023334,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ|NMBF_DELCOMMA)), "369");
			}
			{
				LDATETIME dtm;
				LDATETIME dtm_converted;
				dtm.d.encode(29, 2, 2016);
				dtm.t.encode(21, 17, 2, 250);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_DMY), "29/02/16");
				SLCHECK_EQ(strtodate_(str, DATF_DMY), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_DMY|DATF_CENTURY), "29/02/2016");
				SLCHECK_EQ(strtodate_(str, DATF_DMY), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_GERMANCENT), "29.02.2016");
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_GERMAN|DATF_CENTURY), "29.02.2016"); // Проверяем эквивалентность DATF_GERMANCENT==DATF_GERMAN|DATF_CENTURY
				SLCHECK_EQ(strtodate_(str, DATF_GERMAN), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_AMERICAN|DATF_CENTURY), "02/29/2016");
				SLCHECK_EQ(strtodate_(str, DATF_AMERICAN), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_ANSI|DATF_CENTURY), "2016.02.29");
				SLCHECK_EQ(strtodate_(str, DATF_ANSI), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_ITALIAN|DATF_CENTURY), "29-02-2016");
				SLCHECK_EQ(strtodate_(str, DATF_ITALIAN), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_ITALIAN), "29-02-16");
				SLCHECK_EQ(strtodate_(str, DATF_ITALIAN), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_JAPAN), "16/02/29");
				SLCHECK_EQ(strtodate_(str, DATF_JAPAN), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_ISO8601CENT), "2016-02-29");
				SLCHECK_EQ(strtodate_(str, DATF_ISO8601), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_ISO8601), "16-02-29");
				SLCHECK_EQ(strtodate_(str, DATF_ISO8601), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_AMERICAN|DATF_CENTURY|DATF_NODIV), "02292016");
				SLCHECK_EQ(strtodate_(str, DATF_AMERICAN|DATF_CENTURY|DATF_NODIV), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_DMY|DATF_NODIV), "290216");
				SLCHECK_EQ(strtodate_(str, DATF_DMY|DATF_NODIV), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_SQL), "DATE '2016-02-29'");
				SLCHECK_EQ(strtodate_(str, DATF_SQL), dtm.d);
				SLCHECK_EQ(str.Z().Cat(dtm.d, DATF_INTERNET), "Mon, 29 Feb 2016");
				// (Такой формат не распознается) SLCHECK_EQ(strtodate_(str, DATF_INTERNET), dtm.d);
				SLCHECK_EQ(str.Z().Cat(ZERODATE, DATF_DMY), "");
				SLCHECK_EQ(strtodate_(str, DATF_DMY), ZERODATE);
				SLCHECK_EQ(str.Z().Cat(ZERODATE, DATF_DMY|DATF_NOZERO), "");
				SLCHECK_EQ(strtodate_(str, DATF_DMY|DATF_NOZERO), ZERODATE);

				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_HMS), "21:17:02");
				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_HMS|TIMF_NODIV), "211702");
				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_HM), "21:17");
				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_HM|TIMF_NODIV), "2117");
				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_MS), "17:02");
				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_MS|TIMF_NODIV), "1702");
				SLCHECK_EQ(str.Z().Cat(dtm.t, TIMF_HMS|TIMF_MSEC), "21:17:02.250");

				SLCHECK_EQ(str.Z().Cat(dtm, DATF_ISO8601CENT, 0), "2016-02-29T21:17:02");
				SLCHECK_EQ(str.Z().Cat(dtm, DATF_ISO8601, 0), "16-02-29T21:17:02");
				SLCHECK_EQ(str.Z().Cat(dtm, DATF_MDY|DATF_CENTURY, TIMF_HMS|TIMF_MSEC), "02/29/2016 21:17:02.250");
				//
				strtotime("211702", TIMF_HMS|TIMF_NODIV, &dtm_converted.t);
				SLCHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 0));
				strtotime("\t21:17:02    ", TIMF_HMS, &dtm_converted.t);
				SLCHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 0));
				strtotime("      21:17:02.047   ", TIMF_HMS, &dtm_converted.t);
				SLCHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 4));
				strtotime(" 21 17 02 ", TIMF_HMS, &dtm_converted.t);
				SLCHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 0));
				strtotime(" 2117 ", TIMF_HM|TIMF_NODIV, &dtm_converted.t);
				SLCHECK_EQ(dtm_converted.t, encodetime(21, 17, 00, 0));
			}
			{
				IntRange ir;
				int    r;
				{
					r = str.Z().ToIntRange(ir, SString::torfDoubleDot);
					SLCHECK_Z(r);
				}
				{
					r = (str = "12..987").ToIntRange(ir, SString::torfDoubleDot);
					SLCHECK_NZ(r);
					SLCHECK_EQ(r, (int)str.Len());
					SLCHECK_EQ(ir.low, (int32)12);
					SLCHECK_EQ(ir.upp, (int32)987);
				}
				{
					// Здесь .. не будет распознано как разделитель границ (SString::torfHyphen)
					r = (str = "12..987").ToIntRange(ir, SString::torfHyphen);
					SLCHECK_NZ(r);
					SLCHECK_EQ(r, 2);
					SLCHECK_EQ(ir.low, (int32)12);
					SLCHECK_EQ(ir.upp, (int32)12);
				}
				{
					r = (str = " -12 : 987q").ToIntRange(ir, SString::torfAny);
					SLCHECK_NZ(r);
					SLCHECK_EQ(r, (int)(str.Len()-1));
					SLCHECK_EQ(ir.low, (int32)-12);
					SLCHECK_EQ(ir.upp, (int32)987);
				}
				{
					r = (str = " -12,,-987 z").ToIntRange(ir, SString::torfAny);
					SLCHECK_NZ(r);
					SLCHECK_EQ(r, (int)(str.Len()-2));
					SLCHECK_EQ(ir.low, (int32)-12);
					SLCHECK_EQ(ir.upp, (int32)-987);
				}
				{
					r = (str = " -12000 --987 z").ToIntRange(ir, SString::torfAny);
					SLCHECK_NZ((long)r);
					SLCHECK_EQ(r, (int)(str.Len()-2));
					SLCHECK_EQ(ir.low, (int32)-12000);
					SLCHECK_EQ(ir.upp, (int32)-987);
				}
				{
					SLCHECK_Z((str = " &12").ToIntRange(ir, SString::torfAny));
				}
				{
					r = (str = " +19 ").ToIntRange(ir, SString::torfAny);
					SLCHECK_NZ(r);
					SLCHECK_EQ(r, (int)(str.Len()-1));
					SLCHECK_EQ(ir.low, (int32)19);
					SLCHECK_EQ(ir.upp, (int32)19);
				}
				{
					r = (str = " -1234567890 ").ToIntRange(ir, SString::torfAny);
					SLCHECK_NZ(r);
					SLCHECK_EQ(r, (int)(str.Len()-1));
					SLCHECK_EQ(ir.low, (int32)-1234567890);
					SLCHECK_EQ(ir.upp, (int32)-1234567890);
				}
			}
		}
		{
			// @v11.7.6 Тестирование функций ReadQuotedString и WriteQuotedString
			SString revert_buf;
			size_t end_pos = 0;
			{
				const int result_ReadQuotedString = ReadQuotedString("abc", sstrlen("abc"), 0, &end_pos, str);
				SLCHECK_EQ(0, result_ReadQuotedString);
				SLCHECK_EQ(end_pos, static_cast<size_t>(0U));
			}
			{
				const int result_ReadQuotedString = ReadQuotedString("\"abc", sstrlen("\"abc"), 0, &end_pos, str);
				SLCHECK_LT(result_ReadQuotedString, 0);
				SLCHECK_EQ(end_pos, sstrlen("\"abc"));
			}
			{
				str.Z();
				const char * p_text = "\"abc def\"";
				{
					const int result_ReadQuotedString = ReadQuotedString(p_text, sstrlen(p_text), 0, &end_pos, str);
					SLCHECK_LT(0, result_ReadQuotedString);
					SLCHECK_EQ(str, "abc def");
					SLCHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), 0, revert_buf);
					SLCHECK_EQ(revert_buf, p_text);
				}
				{
					const int result_ReadQuotedString = ReadQuotedString(p_text, sstrlen(p_text), QSF_APPEND, &end_pos, str);
					SLCHECK_LT(0, result_ReadQuotedString);
					SLCHECK_EQ(str, "abc def""abc def");
					SLCHECK_EQ(end_pos, sstrlen(p_text));
				}
			}
			{
				const char * p_text = "\"abc \"\" \\t def\"";
				{
					ReadQuotedString(p_text, sstrlen(p_text), QSF_DBLQ, &end_pos, str);
					SLCHECK_EQ(str, "abc \" \\t def");
					SLCHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), QSF_DBLQ, revert_buf);
					SLCHECK_EQ(revert_buf, p_text);
				}
			}
			{
				const char * p_text = "\"abc \\\" \\t def\"";
				{
					ReadQuotedString(p_text, sstrlen(p_text), QSF_ESCAPE, &end_pos, str);
					SLCHECK_EQ(str, "abc \" \t def");
					SLCHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), QSF_ESCAPE, revert_buf);
					SLCHECK_EQ(revert_buf, p_text);
				}
			}
			{
				const char * p_text = "   \"abc \\\" \\t def\""; // 3 spaces in front of string
				{
					ReadQuotedString(p_text, sstrlen(p_text), QSF_SKIPUNTILQ|QSF_ESCAPE, &end_pos, str);
					SLCHECK_EQ(str, "abc \" \t def");
					SLCHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), QSF_ESCAPE, revert_buf);
					SLCHECK_EQ(revert_buf, p_text+3);
				}
			}
		}
		{
			//
			// Кроме "законных" вариантов представления символов в строки добавлены ложные цели для проверки корректной работы
			// &#, &, &#32 (без ';' в конце)
			//
			const char * p_src_xmlenc = "<ФИОИП &#Фамилия=\"&#x423;&#x421;&#x41A;&#x41E;&#x412;\" &Имя=\"&#x415;&#x412;&#x413;&#x415;&#x41D;&#x418;&#x419;\" &#32Отчество=\"&#x41D;&#x418;&#x41A;&#x41E;&#x41B;&#x410;&#x415;&#x412;&#x418;&#x427;\"/>";
			const char * p_result_xmlenc = "<ФИОИП &#Фамилия=\"УСКОВ\" &Имя=\"ЕВГЕНИЙ\" &#32Отчество=\"НИКОЛАЕВИЧ\"/>";
			(str = p_src_xmlenc).Decode_XMLENT(out_buf);
			SLCHECK_EQ(out_buf, p_result_xmlenc);
		}
		{
			STokenRecognizer tr;
			SNaturalTokenStat nts;
			SNaturalTokenArray nta;
			{
				// @v12.3.0 {
				// Прогоняем обычную текстовую строку на русском языке в 3-х кодировках
				const char * p_ru_text_utf8 = "Товары гастронома со скидкой";
				{
					(str = p_ru_text_utf8).Transf(CTRANSF_UTF8_TO_OUTER);
					tr.Run(str.ucptr(), -1, nta.Z(), &nts); 
					SLCHECK_LT(0.0f, nta.Has(SNTOK_GENERICTEXT_CP1251));
				}
				{
					(str = p_ru_text_utf8);
					tr.Run(str.ucptr(), -1, nta.Z(), &nts); 
					SLCHECK_LT(0.0f, nta.Has(SNTOK_GENERICTEXT_UTF8));
				}
				{
					(str = p_ru_text_utf8).Transf(CTRANSF_UTF8_TO_INNER);
					tr.Run(str.ucptr(), -1, nta.Z(), &nts); 
					SLCHECK_LT(0.0f, nta.Has(SNTOK_GENERICTEXT_CP866));
				}
				// } @v12.3.0 
			}
			tr.Run((const uchar *)"0123", -1, nta.Z(), &nts); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			tr.Run((const uchar *)"4610017121115", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_EAN13));
			tr.Run((const uchar *)"4610017121116", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_EQ(0.0f, nta.Has(SNTOK_EAN13)); // Инвалидная контрольная цифра
			tr.Run((const uchar *)"20352165", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_EAN8));
			tr.Run((const uchar *)"100100802804", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_RU_INN));
			tr.Run((const uchar *)"47296611", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"99057850", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"6z3417681", -1, nta.Z(), 0); 
			SLCHECK_EQ(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_EQ(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"6993417681", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_EQ(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"0034012000001472206", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_EGAISWARECODE));
			tr.Run((const uchar *)"a98P8s00W", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGLAT));
			tr.Run((const uchar *)"4FC737F1-C7A5-4376-A066-2A32D752A2FF", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_GUID));
			tr.Run((const uchar *)"mail.123@gogo-fi.com", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_EMAIL));
			// @notimplemented tr.Run((const uchar *)"123-15-67", -1, nta.Z(), 0); 
			// @notimplemented SLCHECK_LT(0.0f, nta.Has(SNTOK_PHONE));
			// @notimplemented tr.Run((const uchar *)"+7(911)123-15-67", -1, nta.Z(), 0); 
			// @notimplemented SLCHECK_LT(0.0f, nta.Has(SNTOK_PHONE));
			tr.Run((const uchar *)"354190023896443", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_IMEI));
			tr.Run((const uchar *)"192.168.0.1", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_IP4));
			tr.Run((const uchar *)"1/12/2018", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DATE));
			tr.Run((const uchar *)"20180531", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_DATE));
			// @notimplemented tr.Run((const uchar *)"11:30", -1, nta.Z(), 0); 
			// @notimplemented SLCHECK_LT(0.0f, nta.Has(SNTOK_TIME));
			// @notimplemented tr.Run((const uchar *)"11:30:46", -1, nta.Z(), 0); 
			// @notimplemented SLCHECK_LT(0.0f, nta.Has(SNTOK_TIME));
			tr.Run((const uchar *)"10.2.5", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_SOFTWAREVER));
			tr.Run((const uchar *)"#000000", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_COLORHEX));
			tr.Run((const uchar *)"#f82aB7", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_COLORHEX));

			tr.Run((const uchar *)"+100,000.00", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_DOT));
			tr.Run((const uchar *)"+100 000,00", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_COM));
			tr.Run((const uchar *)"-100'000,00", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_COM));
			tr.Run((const uchar *)"9", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_DOT));

			tr.Run((const uchar *)"00000046209443j+Q\'?P5ACZAC8bG", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"00000046209443x-8xfgOACZAYGfv", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"04606203098187o&zWeIyABr8l/nT", -1, nta.Z(), 0); 
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"04606203098187o&zWe\x81yABr8l/nT", -1, nta.Z(), 0); // !SNTOK_CHZN_CIGITEM
			SLCHECK_EQ(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"00000%46209443x-8xfgOACZAYGfv", -1, nta.Z(), 0); // !SNTOK_CHZN_CIGITEM
			SLCHECK_EQ(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			// @v11.9.0 {
			tr.Run((const uchar *)"04670190770074MmK).<EAAAAEC/d", -1, nta.Z(), 0); // SNTOK_CHZN_CIGITEM && SNTOK_CHZN_ALTCIGITEM
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_ALTCIGITEM));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"04670190770814IsYvQdxAAAAtgjI", -1, nta.Z(), 0); // SNTOK_CHZN_CIGITEM && SNTOK_CHZN_ALTCIGITEM
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_ALTCIGITEM));
			SLCHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			// } @v11.9.0 
			// @v12.4.5 {
			{
				const char * p_sscc_code_list[] = {
					"00046112345678901238",
					"00006141421356289525",
					"00006141421356289075",
					"00046100000000000014",
					"00046100000000000021",
					"00046100000000000038",
					"00046100000000000045",
					"00046100000000000052",
					"00046198765432109876",
					"00046255555555555551",
					"00046311111111111118",
					"00046499999999999991",
					"00048177777777777772",
					"00048288888888888884",
					"00053901234567890125",
					"00054433333333333338",
					"00056066666666666665",
					"00078944444444444442",
					"00079022222222222222",
					"00089057012345678908",
					"00089057123445678900",
				};
				for(uint i = 0; i < SIZEOFARRAY(p_sscc_code_list); i++) {
					const char * p_code = p_sscc_code_list[i];
					tr.Run((const uchar *)p_code, -1, nta.Z(), 0);
					SLCHECK_LT(0.0f, nta.Has(SNTOK_SSCC));
					tr.Run((const uchar *)(p_code+2), -1, nta.Z(), 0);
					SLCHECK_LT(0.0f, nta.Has(SNTOK_SSCC));
				}
			}
			// } @v12.4.5 
			{
				static const char * p_cl_rut_data_list[] = {
					"9007920-4", "21620312-7", "13621690-2", "9329827-6", "5946647-k", 
					"5946647-K"/*upper K*/, "7425273-7",  "17694763-2",  "23212441-5", 
					"21485432-5", "15459172-9", "154591729"/*no hyphen*/, "10218932-9", 
					"11316657-6", "24130358-6", "11377848-2", "18609823-4", "18004377-2",
					"180.043.77-2", "8784472-2", "12357399-4", "12391279-9", "8304218-4"
				};
				for(size_t clrutidx = 0; clrutidx < SIZEOFARRAY(p_cl_rut_data_list); clrutidx++) {
					tr.Run((const uchar *)p_cl_rut_data_list[clrutidx], -1, nta.Z(), 0); // !SNTOK_CHZN_CIGITEM
					SLCHECK_LT(0.0f, nta.Has(SNTOK_CL_RUT));
				}
			}
			{
				{
					const char * p_base64_wp_valid[] = {
						"Zg==",
						"Zm8=",
						"Zm9v",
						"Zm9vYg==",
						"Zm9vYmE=",
						"Zm9vYmFy",
						"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4=",
						"Vml2YW11cyBmZXJtZW50dW0gc2VtcGVyIHBvcnRhLg==",
						"U3VzcGVuZGlzc2UgbGVjdHVzIGxlbw==",
						"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuMPNS1Ufof9EW/M98FNw"
						"UAKrwflsqVxaxQjBQnHQmiI7Vac40t8x7pIb8gLGV6wL7sBTJiPovJ0V7y7oc0Ye"
						"rhKh0Rm4skP2z/jHwwZICgGzBvA0rH8xlhUiTvcwDCJ0kc+fh35hNt8srZQM4619"
						"FTgB66Xmp4EtVyhpQV+t02g6NzK72oZI0vnAvqhpkxLeLiMCyrI416wHm5Tkukhx"
						"QmcL2a6hNOyu0ixX/x2kSFXApEnVrJ+/IxGyfyw8kf4N2IZpW5nEP847lpfj0SZZ"
						"Fwrd1mnfnDbYohX2zRptLy2ZUn06Qo9pkG5ntvFEPo9bfZeULtjYzIl6K8gJ2uGZ"
						"HQIDAQAB",
					};
					for(uint i = 0; i < SIZEOFARRAY(p_base64_wp_valid); i++) {
						tr.Run((const uchar *)p_base64_wp_valid[i], -1, nta.Z(), 0);
						SLCHECK_LT(0.0f, nta.Has(SNTOK_BASE64_WP));
					}
				}
				{
					const char * p_base64_wp_invalid[] = {
						"12345",
						"Vml2YW11cyBmZXJtZtesting123",
						"Zg=",
						"Z===",
						"Zm=8",
						"=m9vYg==",
						"Zm9vYmFy====",
					};
					for(uint i = 0; i < SIZEOFARRAY(p_base64_wp_invalid); i++) {
						tr.Run((const uchar *)p_base64_wp_invalid[i], -1, nta.Z(), 0);
						SLCHECK_Z(nta.Has(SNTOK_BASE64_WP));
					}
				}
				{
					const char * p_base64_url_valid[] = {
						"bGFkaWVzIGFuZCBnZW50bGVtZW4sIHdlIGFyZSBmbG9hdGluZyBpbiBzcGFjZQ",
						"1234",
						"bXVtLW5ldmVyLXByb3Vk",
						"PDw_Pz8-Pg",
						"VGhpcyBpcyBhbiBlbmNvZGVkIHN0cmluZw",
					};
					for(uint i = 0; i < SIZEOFARRAY(p_base64_url_valid); i++) {
						tr.Run((const uchar *)p_base64_url_valid[i], -1, nta.Z(), 0);
						SLCHECK_LT(0.0f, nta.Has(SNTOK_BASE64_URL));
					}
				}
				{
					const char * p_base64_url_invalid[] = {
						" AA",
						"\tAA",
						"\rAA",
						"\nAA",
						"This+isa/bad+base64Url==",
						"0K3RgtC+INC30LDQutC+0LTQuNGA0L7QstCw0L3QvdCw0Y8g0YHRgtGA0L7QutCw",
					};
					for(uint i = 0; i < SIZEOFARRAY(p_base64_url_invalid); i++) {
						tr.Run((const uchar *)p_base64_url_invalid[i], -1, nta.Z(), 0);
						SLCHECK_Z(nta.Has(SNTOK_BASE64_URL));
					}
				}
				{
					const char * p_base64_url_wp_valid[] = {
						"SGVsbG8=",
						"U29mdHdhcmU=",
						"YW55IGNhcm5hbCBwbGVhc3VyZS4=",
						"SGVsbG8-",
						"SGVsbG8_",
					};
					for(uint i = 0; i < SIZEOFARRAY(p_base64_url_wp_valid); i++) {
						tr.Run((const uchar *)p_base64_url_wp_valid[i], -1, nta.Z(), 0);
						SLCHECK_LT(0.0f, nta.Has(SNTOK_BASE64_URL_WP));
					}
				}
				{
					const char * p_base64_url_wp_invalid[] = {
						"SGVsbG8===",
						"SGVsbG8@",
						"SGVsb G8=",
						"====",
					};
					for(uint i = 0; i < SIZEOFARRAY(p_base64_url_wp_invalid); i++) {
						tr.Run((const uchar *)p_base64_url_wp_invalid[i], -1, nta.Z(), 0);
						SLCHECK_Z(nta.Has(SNTOK_BASE64_URL_WP));
					}
				}
			}
			{ // @v12.4.7 
				SString line_buf;
				{ 
					// ru-inn
					/* Этот блок нужен только для того, чтобы сформировать файл валидных КПП
					{
						SString out_buf;
						SString raw_file_name(MakeInputFilePath("ru-kpp.txt"));
						SString out_file_name(MakeInputFilePath("ru-kpp-out.txt"));
						SFile f_in_raw(raw_file_name, SFile::mRead);
						SFile f_out(out_file_name, SFile::mWrite);
						if(f_in_raw.IsValid() && f_out.IsValid()) {
							while(f_in_raw.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
								tr.Run(line_buf.ucptr(), line_buf.Len(), nta.Z(), 0);
								const float p = nta.Has(SNTOK_RU_KPP);
								if(p >= 0.1f) {
									//out_buf.Z().Cat(line_buf).Tab().Cat(p, MKSFMTD(0, 2, NMBF_NOZERO)).CR();
									out_buf.Z().Cat(line_buf).CR();
									f_out.WriteLine(out_buf);
								}
							}
						}
					}
					*/
					SFile f_in(MakeInputFilePath("ru-kpp.txt"), SFile::mRead);
					if(f_in.IsValid()) {
						while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
							tr.Run(line_buf.ucptr(), line_buf.Len(), nta.Z(), 0);
							const float p = nta.Has(SNTOK_RU_KPP);
							SLCHECK_LE(0.1f, p);
						}
					}
				}
				{
					// ru-inn
					/* Этот блок нужен только для того, чтобы сформировать файл валидных ИНН
					{
						SString out_buf;
						SString raw_file_name(MakeInputFilePath("ru-inn.txt"));
						SString out_file_name(MakeInputFilePath("ru-inn-out.txt"));
						SFile f_in_raw(raw_file_name, SFile::mRead);
						SFile f_out(out_file_name, SFile::mWrite);
						if(f_in_raw.IsValid() && f_out.IsValid()) {
							while(f_in_raw.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
								tr.Run(line_buf.ucptr(), line_buf.Len(), nta.Z(), 0);
								const float p = nta.Has(SNTOK_RU_INN);
								if(p >= 0.1f) {
									//out_buf.Z().Cat(line_buf).Tab().Cat(p, MKSFMTD(0, 2, NMBF_NOZERO)).CR();
									out_buf.Z().Cat(line_buf).CR();
									f_out.WriteLine(out_buf);
								}
							}
						}
					}
					*/
					SFile f_in(MakeInputFilePath("ru-inn.txt"), SFile::mRead);
					if(f_in.IsValid()) {
						while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
							tr.Run(line_buf.ucptr(), line_buf.Len(), nta.Z(), 0);
							const float p = nta.Has(SNTOK_RU_INN);
							SLCHECK_LE(0.1f, p);
						}
					}
				}
			}
		}
	}
	else {
		uint64 total_len = 0;
		const uint scc = F.P_StrList->getCount();
		if(bm == 1) {
			for(uint phase = 0; phase < max_bm_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					char buffer[2048];
					strnzcpy(buffer, F.P_StrList->at(i), sizeof(buffer));
					total_len += strlen(buffer);
				}
			}
		}
		else if(bm == 2) {
			for(uint phase = 0; phase < max_bm_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					SString buffer;
					buffer = F.P_StrList->at(i);
					total_len += strlen(buffer.cptr());
				}
			}
		}
		else if(bm == 5) {
			for(uint phase = 0; phase < max_bm_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					std::string buffer;
					buffer = F.P_StrList->at(i);
					total_len += strlen(buffer.c_str());
				}
			}
		}
		else if(bm == 3) {
			SRevolver_SString rvl(1024);
			for(uint phase = 0; phase < max_bm_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					SString & r_buffer = rvl.Get();
					r_buffer = F.P_StrList->at(i);
					total_len += strlen(r_buffer.cptr());
				}
			}
		}
		else if(bm == 4) {
			for(uint phase = 0; phase < max_bm_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					SString & r_buffer = SLS.AcquireRvlStr();
					r_buffer = F.P_StrList->at(i);
					total_len += strlen(r_buffer.cptr());
				}
			}
		}
		else if(bm == 7) {
			SString atof_buf;
			for(uint ssp = 0; F.SsRrl.get(&ssp, atof_buf);) {
			//for(uint j = 0; j < F.RandomRealList.getCount(); j++) {
				//double r = F.RandomRealList.at(j);
				//atof_buf.Z().Cat(r, MKSFMTD(0, 32, NMBF_NOTRAILZ));
				double r2;
				r2 = atof(atof_buf);
			}
		}
		else if(bm == 8) {
			SString atof_buf;
			for(uint ssp = 0; F.SsRrl.get(&ssp, atof_buf);) {
			//for(uint j = 0; j < F.RandomRealList.getCount(); j++) {
				//double r = F.RandomRealList.at(j);
				//atof_buf.Z().Cat(r, MKSFMTD(0, 32, NMBF_NOTRAILZ));
				double r2;
				//satof(atof_buf, &r2);
				//
				const char * p_end = 0;
				int erange = 0;
				//dconvstr_scan(atof_buf, &p_end, &r2, &erange);
				r2 = satof(atof_buf); // @v10.7.9 dconvstr_scan-->satof
			}
		}
		else if(bm == 9) { // strlen
			for(uint phase = 0; phase < max_strlen_phase; phase++) {
				for(uint i = 0; i < scc; i++)
					total_len += strlen(F.P_StrList->at(i));
			}
		}
		else if(bm == 10) { // sstrlen
			for(uint phase = 0; phase < max_strlen_phase; phase++) {
				for(uint i = 0; i < scc; i++)
					total_len += sstrlen(F.P_StrList->at(i));
			}
		}
		// @v11.7.10 {
		else if(bm == 11) { // smemchr0
			for(uint phase = 0; phase < max_strlen_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					const char * p_str = F.P_StrList->at(i);
					const char * p_end = static_cast<const char *>(smemchr(p_str, 0, F.MaxStrListItemLen+1));
					assert(p_end);
					total_len += (p_end - p_str);
				}
			}
		}
		else if(bm == 12) { // strnlen
			for(uint phase = 0; phase < max_strlen_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					const char * p_str = F.P_StrList->at(i);
					total_len += strnlen(p_str, F.MaxStrListItemLen+1);
				}
			}
		}
		else if(bm == 13) { // sstrnlen
			for(uint phase = 0; phase < max_strlen_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					const char * p_str = F.P_StrList->at(i);
					total_len += sstrnlen(p_str, F.MaxStrListItemLen+1);
				}
			}
		}
		// } @v11.7.10 
	}
	CATCH
		CurrentStatus = ok = 0;
	ENDCATCH
	test_buf.Destroy();
	temp_buf.Destroy();
	return CurrentStatus;
}

SLTEST_R(SFsPath)
{
	int    ok = 1;
	SFile file(MakeInputFilePath("path.txt"), SFile::mRead | SFile::mBinary);
	if(file.IsValid()) {
		SString line_buf;
		SFile out_file(MakeOutputFilePath("path.out"), SFile::mWrite);
		SFsPath ps;
		while(file.ReadLine(line_buf, SFile::rlfChomp)) {
			SInvariantParam ip;
			ps.Split(line_buf);
			SLCHECK_NZ(ps.Invariant(&ip));
			out_file.WriteLine(ip.MsgBuf.CR());
		}
	}
	return CurrentStatus;
}
//
//
//
const void * fast_memchr(const void * haystack, int n, size_t len);
//const void * fast_memchr_sse2(const void * haystack, int n, size_t len);

struct Test_memchr_Block {
	Test_memchr_Block(const char needle, const char * pHaystackFileName) : Needle(needle), Status(0)
	{
		Status |= stError;
		SFile f_in(pHaystackFileName, SFile::mRead);
		if(f_in.IsValid()) {
			STempBuffer _buf(SMEGABYTE(1));
			size_t actual_size = 0;
			if(f_in.ReadAll(_buf, 0, &actual_size) > 0) {
				Status &= ~stError;
				Haystack.CatN(_buf.cptr(), actual_size);
				//
				const uint hs_len = Haystack.Len();
				for(uint i = 0; i < hs_len; i++) {
					PosList.add((long)i);
					if(hs_len <= 1024) {
						LenList.add((long)(i+1));
					}
					else if(i < 512/* || i >= (hs_len - 512)*/)
						LenList.add((long)(i+1));
					if(Haystack.C(i) == Needle) {
						TargetPosList.add((long)i);
					}
				}
				TargetPosList.sort();
				LenList.shuffle();
				PosList.shuffle();
			}
		}
	}
	enum {
		stError = 0x0001
	};
	bool   IsValid() const { return !(Status & stError); }

	const  char Needle;
	uint8  Reserve[3]; // @alignment
	int    Status;
	SString Haystack;
	LongArray TargetPosList; // Список позиций, в которых встрачается целевой символ
	LongArray PosList; // Список позиций, начиная с которых следует осуществлять поиск
	LongArray LenList; // Список длин отрезков, на которых следует осуществлять поиск
};

static int Test_memchr(Test_memchr_Block & rBlk, const void * (*func)(const void * pHaystack, int needle, size_t len))
{
	int    ok = 1;
	{
		const char * p_haystack = rBlk.Haystack.cptr();
		const size_t haystack_len = rBlk.Haystack.Len();
		for(uint lidx = 0; ok && lidx < rBlk.LenList.getCount(); lidx++) {
			const size_t len = (size_t)rBlk.LenList.get(lidx);
			for(uint pidx = 0; ok && pidx < rBlk.PosList.getCount(); pidx++) {
				const uint pos = (size_t)rBlk.PosList.get(pidx);
				if((pos+len) <= haystack_len) {
					const char * p_target = static_cast<const char *>(func(const_cast<char *>(p_haystack+pos), rBlk.Needle, len));
					const long pos_key = (long)pos;
					uint  pos_key_idx = 0;
					const bool tpl_search_result = rBlk.TargetPosList.bsearchGe(&pos_key, &pos_key_idx, CMPF_LONG);
					const size_t found_pos = tpl_search_result ? static_cast<ssize_t>(rBlk.TargetPosList.get(pos_key_idx)) : 0;
					if(p_target) {
						if(tpl_search_result && (p_target - p_haystack) == found_pos) {
							; // ok
						}
						else {
							ok = 0;
						}
					}
					else {
						if(!tpl_search_result || found_pos >= (pos+len)) {
							; // ok
						}
						else {
							ok = 0;
						}
					}
				}
			}
		}
	}
	return ok;
}

static int Profile_memchr(Test_memchr_Block & rBlk, const void * (*func)(const void * pHaystack, int needle, size_t len))
{
	int    ok = 1;
	{
		const char * p_haystack = rBlk.Haystack.cptr();
		const size_t haystack_len = rBlk.Haystack.Len();
		volatile char * p_target = 0;
		for(uint lidx = 0; ok && lidx < rBlk.LenList.getCount(); lidx++) {
			const size_t len = (size_t)rBlk.LenList.get(lidx);
			for(uint pidx = 0; ok && pidx < rBlk.PosList.getCount(); pidx++) {
				const uint pos = (size_t)rBlk.PosList.get(pidx);
				if((pos+len) <= haystack_len) {
					p_target = const_cast<volatile char *>(static_cast<const char *>(func(const_cast<char *>(p_haystack+pos), rBlk.Needle, len)));
				}
			}
		}
	}
	return ok;
}

SLTEST_R(memchr)
{
	//benchmark=memchr;fast_memchr;fast_memchr_sse2
	int    bm = -1;
	if(pBenchmark == 0) 
		bm = 0;
	else if(sstreqi_ascii(pBenchmark, "memchr"))        
		bm = 1;
	else if(sstreqi_ascii(pBenchmark, "fast_memchr"))   
		bm = 2;
	else if(sstreqi_ascii(pBenchmark, "fast_memchr_sse2"))      
		bm = 3;
	SString test_data_path(MakeInputFilePath("sherlock-holmes-huge.txt"));
	Test_memchr_Block blk('A', test_data_path);
	THROW(SLCHECK_NZ(blk.IsValid()));
	if(bm == 0) {
		SLCHECK_NZ(Test_memchr(blk, memchr));
		SLCHECK_NZ(Test_memchr(blk, fast_memchr));
		SLCHECK_NZ(Test_memchr(blk, /*fast_memchr_sse2*/smemchr));
	}
	else if(bm == 1) {
		Profile_memchr(blk, memchr);
	}
	else if(bm == 2) {
		Profile_memchr(blk, fast_memchr);
	}
	else if(bm == 3) {
		Profile_memchr(blk, /*fast_memchr_sse2*/smemchr);
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;	
}

static int Test_strstr(const char * pHaystack, const char * pNeedle, const size_t * pPosList, uint posListCount, const char * (* funcStrStr)(const char *, const char *))
{
	int    ok = 1;
	if(funcStrStr) {
		size_t result_pos_list[128];
		uint result_count = 0;
		const char * p = pHaystack;
		do {
			p = funcStrStr(p, pNeedle);
			if(p) {
				result_pos_list[result_count++] = p-pHaystack;
				p++;
			}
		} while(p);
		if(result_count != posListCount)
			ok = 0;
		else {
			for(uint i = 0; ok && i < result_count; i++) {
				if(result_pos_list[i] != pPosList[i])
					ok = 0;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

SLTEST_R(strstr)
{
	const char * p_haystack = "abcDEFabcababc";
	const char * p_needle = "abc";
	const size_t pos_list[] = {0, 6, 11};
	SLCHECK_NZ(Test_strstr(p_haystack, p_needle, pos_list, SIZEOFARRAY(pos_list), strstr));
	SLCHECK_NZ(Test_strstr(p_haystack, p_needle, pos_list, SIZEOFARRAY(pos_list), byteshift_strstr));
	{
		//ExtStrSrch()
		const char * p_text = "The Perfetto Tracing SDK is a C++17 library that allows userspace applications to emit trace events and add more app-specific context to a Perfetto trace."
			"When using the Tracing SDK there are two main aspects to consider:"
			"Whether you are interested only in tracing events coming from your own app or want to collect full-stack traces that overlay app trace events with system trace events like scheduler traces, syscalls or any other Perfetto data source."
			"For app-specific tracing, whether you need to trace simple types of timeline events (e.g., slices, counters) or need to define complex data sources with a custom strongly-typed schema (e.g., for dumping the state of a subsystem of your app into the trace)."
			"For Android-only instrumentation, the advice is to keep using the existing android.os.Trace (SDK) / ATrace_* (NDK) if they are sufficient for your use cases. Atrace-based instrumentation is fully supported in Perfetto. See the Data Sources -> Android System -> Atrace Instrumentation for details.";
		SLCHECK_NZ(ExtStrSrch(p_text, "n tRacing", 0));
		SLCHECK_Z(ExtStrSrch(p_text, "n traCing", essfWholeWords));
		SLCHECK_Z(ExtStrSrch(p_text, "n tRacing", essfCaseSensitive));
		SLCHECK_Z(ExtStrSrch(p_text, "n traCing", essfCaseSensitive|essfWholeWords));
		SLCHECK_NZ(ExtStrSrch(p_text, "when using", 0));
		SLCHECK_NZ(ExtStrSrch(p_text, "when using", essfWholeWords));
		SLCHECK_Z(ExtStrSrch(p_text, "hen using", essfWholeWords));
		SLCHECK_NZ(ExtStrSrch(p_text, "consider", essfWholeWords));
		SLCHECK_NZ(ExtStrSrch(p_text, "consider", essfCaseSensitive));
		SLCHECK_Z(ExtStrSrch(p_text, "xzxaxaee", 0));
	}
	return CurrentStatus;	
}

static int Make_STextEncodingStat_FilePool(const SString & rPath, SFileEntryPool & rFep)
{
	int    ok = 1;
	SDirEntry de;
	SString temp_buf;
	(temp_buf = rPath).SetLastSlash().Cat("*.*");
	for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
		if(!de.IsSelf() && !de.IsUpFolder()) {
			if(de.IsFile()) {
				THROW(rFep.Add(rPath, de, 0));
			}
			else if(de.IsFolder()) {
				de.GetNameA(rPath, temp_buf);
				THROW(Make_STextEncodingStat_FilePool(temp_buf, rFep)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

SLTEST_R(STextEncodingStat)
{
	SString test_data_path(MakeInputFilePath("uchardet"));
	SString out_file_name(MakeOutputFilePath("uchardet-out.txt"));
	SString in_file_name;
	SString temp_buf;
	SFileEntryPool fep;
	//SFileEntryPool::Entry fep_entry;
	SFsPath ps;
	{
		STextEncodingStat tes_icu(STextEncodingStat::fUseIcuCharDet);
		THROW(Make_STextEncodingStat_FilePool(test_data_path, fep));
		{
			SFile f_out(out_file_name, SFile::mWrite);
			for(uint i = 0; i < fep.GetCount(); i++) {
				if(fep.Get(i, /*fep_entry*/0, &in_file_name)) {
					//(in_file_name = fep_entry.Path).SetLastSlash().Cat(fep_entry.Name);
					SFile f_in(in_file_name, SFile::mRead);
					tes_icu.Init(STextEncodingStat::fUseIcuCharDet);
					while(f_in.ReadLine(temp_buf)) {
						tes_icu.Add(temp_buf, temp_buf.Len());
					}
					tes_icu.Finish();
					ps.Split(in_file_name);
					SLCHECK_NZ(tes_icu.CheckFlag(STextEncodingStat::fUCharDetWorked));
					SLCHECK_Z(ps.Nam.CmpNC(tes_icu.GetCpName()));
					temp_buf.Z().Cat(in_file_name).Tab().Cat(tes_icu.GetCpName()).CR();
					f_out.WriteLine(temp_buf);
				}
			}
		}
	}
	{
		STextEncodingStat tes_(STextEncodingStat::fUseUCharDet);
		THROW(Make_STextEncodingStat_FilePool(test_data_path, fep));
		{
			SFile f_out(out_file_name, SFile::mWrite);
			for(uint i = 0; i < fep.GetCount(); i++) {
				if(fep.Get(i, /*fep_entry*/0, &in_file_name)) {
					//(in_file_name = fep_entry.Path).SetLastSlash().Cat(fep_entry.Name);
					SFile f_in(in_file_name, SFile::mRead);
					tes_.Init(STextEncodingStat::fUseUCharDet);
					while(f_in.ReadLine(temp_buf)) {
						tes_.Add(temp_buf, temp_buf.Len());
					}
					tes_.Finish();
					ps.Split(in_file_name);
					SLCHECK_NZ(tes_.CheckFlag(STextEncodingStat::fUCharDetWorked));
					SLCHECK_Z(ps.Nam.CmpNC(tes_.GetCpName()));
					temp_buf.Z().Cat(in_file_name).Tab().Cat(tes_.GetCpName()).CR();
					f_out.WriteLine(temp_buf);
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(sstrchr)
{
	SLCHECK_Z(sstrchr((const char *)0, 'z'));
	SLCHECK_Z(sstrrchr((const char *)0, 'z'));
	SLCHECK_Z(sstrchr((const char *)0, '\0'));
	SLCHECK_Z(sstrrchr((const char *)0, '\0'));
	SLCHECK_Z(sstrchr((char *)0, 'z'));
	SLCHECK_Z(sstrrchr((char *)0, 'z'));
	SLCHECK_Z(sstrchr((char *)0, '\0'));
	SLCHECK_Z(sstrrchr((const wchar_t *)0, L'\0'));
	SLCHECK_Z(sstrchr((const wchar_t *)0, L'z'));
	SLCHECK_Z(sstrrchr((const wchar_t *)0, L'z'));
	SLCHECK_Z(sstrchr((const wchar_t *)0, L'\0'));
	SLCHECK_Z(sstrrchr((const wchar_t *)0, L'\0'));
	SLCHECK_Z(sstrrchr((wchar_t *)0, L'\0'));
	SLCHECK_Z(sstrchr((wchar_t *)0, L'z'));
	SLCHECK_Z(sstrrchr((wchar_t *)0, L'z'));
	SLCHECK_Z(sstrchr((wchar_t *)0, L'\0'));
	SLCHECK_Z(sstrrchr((wchar_t *)0, L'\0'));
	{
		SLCHECK_Z(sstrchr("abcd", 'z'));
		SLCHECK_Z(sstrrchr("abcd", 'z'));
		SLCHECK_Z(sstrchr(L"abcd", L'z'));
		SLCHECK_Z(sstrrchr(L"abcd", L'z'));
		{
			char   one[128];
			strcpy(one, "abcd");
			SLCHECK_EQ(sstrchr(one, 'c'), one+2);
			SLCHECK_EQ(sstrrchr(one, 'c'), one+2);
			SLCHECK_EQ(sstrchr(one, 'd'), one+3);
			SLCHECK_EQ(sstrrchr(one, 'd'), one+3);
			SLCHECK_EQ(sstrchr(one, 'a'), one);
			SLCHECK_EQ(sstrrchr(one, 'a'), one);
			SLCHECK_EQ(sstrchr(one, '\0'), one+4);
			SLCHECK_EQ(sstrrchr(one, '\0'), one+4);
			strcpy(one, "ababa");
			SLCHECK_EQ(sstrchr(one, 'b'), one+1);
			SLCHECK_EQ(sstrrchr(one, 'b'), one+3);
			strcpy(one, "");
			SLCHECK_Z(sstrchr(one, 'b'));
			SLCHECK_Z(sstrrchr(one, 'b'));
			SLCHECK_EQ(sstrchr(one, '\0'), one);
			SLCHECK_EQ(sstrrchr(one, '\0'), one);
		}
		{
			wchar_t one[128];
			wcscpy(one, L"abcd");
			SLCHECK_EQ(sstrchr(one, L'c'), one+2);
			SLCHECK_EQ(sstrrchr(one, L'c'), one+2);
			SLCHECK_EQ(sstrchr(one, L'd'), one+3);
			SLCHECK_EQ(sstrrchr(one, L'd'), one+3);
			SLCHECK_EQ(sstrchr(one, L'a'), one);
			SLCHECK_EQ(sstrrchr(one, L'a'), one);
			SLCHECK_EQ(sstrchr(one, L'\0'), one+4);
			SLCHECK_EQ(sstrrchr(one, L'\0'), one+4);
			wcscpy(one, L"ababa");
			SLCHECK_EQ(sstrchr(one, L'b'), one+1);
			SLCHECK_EQ(sstrrchr(one, L'b'), one+3);
			wcscpy(one, L"");
			SLCHECK_Z(sstrchr(one, L'b'));
			SLCHECK_Z(sstrrchr(one, L'b'));
			SLCHECK_EQ(sstrchr(one, L'\0'), one);
			SLCHECK_EQ(sstrrchr(one, L'\0'), one);
		}
	}
	{
		char buf[4096];
		char * p;
		for(size_t i = 0; i < 0x100; i++) {
			p = (char *)((ulong)(buf + 0xff) & ~0xff) + i;
			strcpy(p, "OK");
			strcpy(p+3, "BAD/WRONG");
			SLCHECK_Z(sstrchr(p, '/'));
		}
	}
	{
		STempBuffer buf(SMEGABYTE(4));
		SLCHECK_NZ(buf);
		memset(buf, 'x', buf.GetSize());
		buf[buf.GetSize()-1] = 0;
		SLCHECK_Z(sstrchr(buf, 'a'));
		SLCHECK_Z(sstrrchr(buf, 'a'));
		SLCHECK_EQ(sstrchr(buf, '\0'), buf+buf.GetSize()-1);
		SLCHECK_EQ(sstrrchr(buf, '\0'), buf+buf.GetSize()-1);
		SLCHECK_EQ(sstrchr(buf, 'x'), buf);
		SLCHECK_EQ(sstrrchr(buf, 'x'), buf+buf.GetSize()-2);
		for(size_t i = 0; i < buf.GetSize()-1; i += 1021) {
			buf[i] = 'y';
			SLCHECK_EQ(sstrchr(buf, 'y'), buf+i);
			SLCHECK_EQ(sstrrchr(buf, 'y'), buf+i);
			buf[i] = 'x';
		}
	}
	return CurrentStatus;
}

SLTEST_R(StringSet)
{
	class InnerBlock {
	public:
		static int Verify(const StringSet & rSs, const SStrCollection & rStringList, LongArray * pPosList)
		{
			int    ok = 1;
			SString temp_buf;
			char   c_buf[2048];
			uint   sspos = 0;
			for(uint i = 0; i < rStringList.getCount(); i++) {
				const uint _prev_sspos = sspos;
				THROW(rSs.get(&sspos, temp_buf));
				sspos = _prev_sspos; // @v12.3.5
				THROW(rSs.get(&sspos, c_buf, sizeof(c_buf))); // @v12.3.5
				const char * p_str = rStringList.at(i);
				THROW(temp_buf == p_str);
				THROW(sstreq(p_str, c_buf)); // @v12.3.5
				if(pPosList) {
					THROW(pPosList->at(i) == static_cast<long>(_prev_sspos));
				}
			}
			CATCHZOK
			return ok;
		}
	};
	int    ok = 1;
	{
		SStrCollection string_list;
		SString temp_buf;
		(temp_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("phrases-ru-1251.txt");
		SFile f_in(temp_buf, SFile::mRead);
		THROW(f_in.IsValid());
		while(f_in.ReadLine(temp_buf)) {
			temp_buf.Chomp();
			temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
			if(temp_buf.Len())
				THROW(string_list.insert(newStr(temp_buf)));
		}
		{
			// Сначала тестируем StringSet с пустым разделителем. В такой StringSet пустые строки вставлять нельзя!
			StringSet ss;
			LongArray pos_list;
			for(uint i = 0; i < string_list.getCount(); i++) {
				uint pos = 0;
				SLCHECK_Z(ss.IsCountGreaterThan(i)); // @v12.3.5
				THROW(ss.add(string_list.at(i), &pos));
				THROW(pos_list.add(static_cast<long>(pos)));
				SLCHECK_NZ(ss.IsCountGreaterThan(i)); // @v12.3.5
			}
			THROW(SLCHECK_EQ(string_list.getCount(), ss.getCount()));
			THROW(SLCHECK_EQ(pos_list.getCount(), ss.getCount()));
			SLCHECK_NZ(InnerBlock::Verify(ss, string_list, &pos_list));
			//
			// Теперь сортируем string_list и ss и сравниваем без позиций (они стали инвалидными)
			//
			string_list.sort(PTR_CMPFUNC(PcharNoCase));
			ss.sort();
			SLCHECK_NZ(InnerBlock::Verify(ss, string_list, 0));
		}
		{
			// Теперь тестируем StringSet с непустым разделителем. Дополнительно вставляем пустые строки в string_list
			for(uint j = 0; j < 20; j++) {
				THROW(string_list.insert(newStr("")));
			}
			string_list.shuffle();
			//
			StringSet ss("*-$");
			LongArray pos_list;
			for(uint i = 0; i < string_list.getCount(); i++) {
				uint pos = 0;
				SLCHECK_Z(ss.IsCountGreaterThan(i)); // @v12.3.5
				THROW(ss.add(string_list.at(i), &pos));
				THROW(pos_list.add(static_cast<long>(pos)));
				SLCHECK_NZ(ss.IsCountGreaterThan(i)); // @v12.3.5
			}
			THROW(SLCHECK_EQ(string_list.getCount(), ss.getCount()));
			THROW(SLCHECK_EQ(pos_list.getCount(), ss.getCount()));
			SLCHECK_NZ(InnerBlock::Verify(ss, string_list, &pos_list));
			//
			// Теперь сортируем string_list и ss и сравниваем без позиций (они стали инвалидными)
			//
			string_list.sort(PTR_CMPFUNC(PcharNoCase));
			ss.sort();
			SLCHECK_NZ(InnerBlock::Verify(ss, string_list, 0));
			{
				// Проверяем copy-constructor StringSet
				StringSet ss2(ss);
				SLCHECK_NZ(ss2.IsEq(ss));
				SLCHECK_NZ(ss2 == ss);
				SLCHECK_NZ(InnerBlock::Verify(ss2, string_list, 0));
			}
			{
				// Проверяем operator = StringSet
				StringSet ss2;
				ss2 = ss;
				SLCHECK_NZ(ss2.IsEq(ss));
				SLCHECK_NZ(ss == ss2);
				SLCHECK_NZ(InnerBlock::Verify(ss2, string_list, 0));
			}
			{
				// Проверяем StringSet::Copy
				StringSet ss2;
				ss2.copy(ss);
				SLCHECK_NZ(ss.IsEq(ss2));
				SLCHECK_NZ(ss2 == ss);
				SLCHECK_NZ(InnerBlock::Verify(ss2, string_list, 0));
			}
			{
				// Проверяем Shuffle и IsEqPermutation
				StringSet ss2;
				ss2.copy(ss);
				SLCHECK_NZ(ss.IsEq(ss2));
				SLCHECK_NZ(ss2 == ss);
				SLCHECK_NZ(InnerBlock::Verify(ss2, string_list, 0));
				// @v11.7.12 {
				ss2.shuffle();
				SLCHECK_NZ(ss2 != ss);
				SLCHECK_NZ(ss2.IsEqPermutation(ss));
				// Здесь InnerBlock::Verify не сработает
				// } @v11.7.12 
			}
		}
		{
			// Функция getByIdx
			StringSet ss;
			const uint test_set_count = 1000;
			{
				for(uint i = 0; i < test_set_count; i++)
					ss.add(temp_buf.Z().Cat(i));
			}
			{
				uint i = test_set_count;
				do {
					bool gbir = ss.getByIdx(--i, temp_buf);
					SLCHECK_NZ(gbir);
					SLCHECK_EQ(temp_buf.ToLong(), static_cast<long>(i));
				} while(i);
				{
					bool gbir = ss.getByIdx(test_set_count, temp_buf);
					SLCHECK_Z(gbir);
					SLCHECK_NZ(temp_buf.IsEmpty());
				}
				{
					bool gbir = ss.getByIdx(test_set_count+1, temp_buf);
					SLCHECK_Z(gbir);
					SLCHECK_NZ(temp_buf.IsEmpty());
				}
			}
		}
	}
	CATCHZOK
	return ok ? CurrentStatus : 0;
}

#if 0 // @v12.3.3 @obsolete {
SLTEST_R(CRegExp)
{
	int    ok = 1;
	SString file_name, temp_buf;
	uint   arg_no = 0;
	if(EnumArg(&arg_no, temp_buf))
		file_name = temp_buf;
	else
		file_name = temp_buf = "cregexp.txt";
	SFile file(MakeInputFilePath(file_name), SFile::mRead);
	if(file.IsValid()) {
		SString line_buf, re_buf, text_buf, temp_buf, out_line;
		SFsPath::ReplaceExt(file_name, "out", 1);
		SFile out_file(MakeOutputFilePath(file_name), SFile::mWrite);
		while(file.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			if(line_buf.NotEmpty()) {
				StringSet ss('\t', line_buf); // @v12.3.3 ':'-->'\t'
				uint   ssp = 0;
				ss.get(&ssp, re_buf);
				ss.get(&ssp, text_buf);
				ss.get(&ssp, temp_buf);
				const long right_count = temp_buf.ToLong();
				long   count = 0;
				SRegExp2 re;
				THROW(SLCHECK_NZ(re.Compile(re_buf, cp1251, SRegExp2::syntaxDefault, 0)));
				{
					const char * p = text_buf;
					out_line.Z().Cat(re_buf).CatDiv(':', 2).Cat(text_buf);
					SStrScan scan(text_buf);
					if(re.Find(&scan)) {
						out_line.CatDiv(':', 2);
						do {
							scan.Get(temp_buf);
							if(count)
								out_line.CatDiv(',', 2);
							out_line.CatQStr(temp_buf);
							scan.IncrLen();
							count++;
						} while(re.Find(&scan));
					}
					out_line.CatDiv(':', 2).Cat(count).CR();
					out_file.WriteLine(out_line);
					THROW(SLCHECK_EQ(count, right_count));
				}
			}
		}
	}
	CATCH
		CurrentStatus = ok = 0;
	ENDCATCH
	return CurrentStatus;
}
#endif // } 0 @v12.3.3 

SLTEST_R(SRegExp)
{
	struct Entry {
		const char * P_RegExp;
		const char * P_Text;
		uint   Count;
	};

	static const Entry entry_list[] = {
		{ "{.*} ", "{abc{efg}} ", 1 },
		{ "[ ]+", " My brather    like  me. ", 5 },
		{ "\"[^\"]*\"", "There is \"quoted string\"", 1 },
		{ "[+-]?[0-9]+", "10 barrels oil isn't too mach", 1 },
		{ "[+-]?[0-9]*([\\.][0-9]*)([Ee][+-]?[0-9]+)?", "I think, 3.1415926 - near approach to pi-number", 1 },
		{ "[+-]?[0-9]*([\\.][0-9]*)([Ee][+-]?[0-9]+)?", "Some number - -23.002", 1 },
		{ "[+-]?[0-9]*([\\.][0-9]*)([Ee][+-]?[0-9]+)?", "Another number - +5.20021e-14", 1 },
		{ "[0-9]+[ ]*\\,[ ]*[0-9]+", "Some example - 3  , 22233", 1 },
		{ "\\[[ ]*[0-9]+(\\.[0-9]*)?[ ]*\\]", "[12.33  ] [ 192.1] [    41]", 3 },
		{ "\\([ ]*[0-9]+(\\.[0-9]*)?[ ]*\\)", "(12.00 ) ( 192.168    ) ( 19  )", 3 },
		{ "\\(.*\\)", " some text ( (nested text), (nested text2)) . ( [], ())", 1 },
		{ "\\#\\@\\([^)]*\\)", "prefix text #@(abc), suffix text", 1 },
		{ "[-A-Za-z0-9!#$%&'*+/=?^_`{|}~]+(\\.[-A-Za-z0-9!#$%&'*+/=?^_`{|}~]+)*\\@([A-Za-z0-9][-A-Za-z0-9]*\\.)+[a-z]+", " abc@mail.ru xy.z@dot.com -nik?&+$a.b.c02@yandex.ru", 3 },
		{ "([+]?[0-9]+)?[ ]*([-(]*[0-9]+[-(])?[ ]*[0-9]+[- ]?[0-9]+[- ]?[0-9]+[ ]*([Ww][0-9]+)?", " +7(921)7002198 8(921)7002198 8(921)7002198 38-15-60 1-98-12", 5 },
		{ "(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{4})", "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4=", 1 }
	};

	int    ok = 1;
	SString file_name;
	SString temp_buf;
	SString out_line;
	{
		for(uint i = 0; i < SIZEOFARRAY(entry_list); i++) {
			const char * p_regexp = entry_list[i].P_RegExp;
			const char * p_text = entry_list[i].P_Text;
			const uint valid_count = entry_list[i].Count;

			uint  count = 0;
			SRegExp2 re;
			THROW(SLCHECK_NZ(re.Compile(p_regexp, cp1251, SRegExp2::syntaxDefault, 0)));
			{
				const char * p = p_text;
				//out_line.Z().Cat(p_regexp).CatDiv(':', 2).Cat(p_text);
				SStrScan scan(p_text);
				if(re.Find(&scan)) {
					out_line.CatDiv(':', 2);
					do {
						scan.Get(temp_buf);
						if(count)
							out_line.CatDiv(',', 2);
						out_line.CatQStr(temp_buf);
						scan.IncrLen();
						count++;
					} while(re.Find(&scan));
				}
				//out_line.CatDiv(':', 2).Cat(count).CR();
				//out_file.WriteLine(out_line);
				THROW(SLCHECK_EQ(count, valid_count));
			}
		}
	}
#if 0 // {
	{
		uint   arg_no = 0;
		if(EnumArg(&arg_no, temp_buf))
			file_name = temp_buf;
		else
			file_name = temp_buf = "cregexp.txt";
		SFile file(MakeInputFilePath(file_name), SFile::mRead);
		if(file.IsValid()) {
			SString line_buf;
			SString re_buf;
			SString text_buf;
			SFsPath::ReplaceExt(file_name, "out", 1);
			SFile out_file(MakeOutputFilePath(file_name), SFile::mWrite);
			while(file.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
				if(line_buf.NotEmpty()) {
					StringSet ss('\t', line_buf); // @v12.3.3 ':'-->'\t'
					uint   ssp = 0;
					ss.get(&ssp, re_buf);
					ss.get(&ssp, text_buf);
					ss.get(&ssp, temp_buf);
					const long right_count = temp_buf.ToLong();
					long   count = 0;
					SRegExp2 re;
					THROW(SLCHECK_NZ(re.Compile(re_buf, cp1251, SRegExp2::syntaxDefault, 0)));
					{
						const char * p = text_buf;
						out_line.Z().Cat(re_buf).CatDiv(':', 2).Cat(text_buf);
						SStrScan scan(text_buf);
						if(re.Find(&scan)) {
							out_line.CatDiv(':', 2);
							do {
								scan.Get(temp_buf);
								if(count)
									out_line.CatDiv(',', 2);
								out_line.CatQStr(temp_buf);
								scan.IncrLen();
								count++;
							} while(re.Find(&scan));
						}
						out_line.CatDiv(':', 2).Cat(count).CR();
						out_file.WriteLine(out_line);
						THROW(SLCHECK_EQ(count, right_count));
					}
				}
			}
		}
	}
#endif // 0 } 
	CATCH
		CurrentStatus = ok = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(slprintf)
{
	// dummy putchar
	char flat_buffer[100];
	SString sbuf;

	//benchmark=slprintf;sprintf
	int    bm = -1;
	if(pBenchmark == 0) bm = 0;
	else if(sstreqi_ascii(pBenchmark, "slprintf"))  bm = 1;
	else if(sstreqi_ascii(pBenchmark, "sprintf"))   bm = 2;
	else SetInfo("invalid benchmark");
	if(bm == 0) {
		{ // Trivial tests
			slprintf(sbuf.Z(), "\n");
			SLCHECK_EQ(sbuf, "\n");
			slprintf(sbuf.Z(), "");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%%");
			SLCHECK_EQ(sbuf, "%");
			slprintf(sbuf.Z(), "%%%s", "beef");
			SLCHECK_EQ(sbuf, "%beef");
			slprintf(sbuf.Z(), "zero ptr string=\"%s\"", 0);
			SLCHECK_EQ(sbuf, "zero ptr string=\"\"");
		}
		{ // TEST_CASE("printf", "[]" )
			SLCHECK_EQ(slprintf(sbuf.Z(), "% d", 4232), 5);
			SLCHECK_EQ(sbuf.Len(), static_cast<size_t>(5U));
			SLCHECK_EQ(sbuf.cptr()[5], 0);
			SLCHECK_EQ(sbuf, " 4232");
		}
		{ // TEST_CASE("fctprintf", "[]" )
			SLCHECK_EQ(slprintf(sbuf.Z(), "This is a test of %X", 0x12EFU), 22);
			SLCHECK_EQ(sbuf.Len(), static_cast<size_t>(22U));
			SLCHECK_EQ(sbuf, "This is a test of 12EF");
		}
		{ // TEST_CASE("snprintf", "[]" )
			slprintf(sbuf.Z(), "%d", -1000);
			SLCHECK_EQ(sbuf, "-1000");
			//slprintf(buffer, 3U, "%d", -1000);
			//SLCHECK_EQ(sbuf, "-1"));
		}
		{ // TEST_CASE("space flag", "[]" )
			slprintf(sbuf.Z(), "% d", 42);
			SLCHECK_EQ(sbuf, " 42");
			slprintf(sbuf.Z(), "% d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "% 5d", 42);
			SLCHECK_EQ(sbuf, "   42");
			slprintf(sbuf.Z(), "% 5d", -42);
			SLCHECK_EQ(sbuf, "  -42");
			slprintf(sbuf.Z(), "% 15d", 42);
			SLCHECK_EQ(sbuf, "             42");
			slprintf(sbuf.Z(), "% 15d", -42);
			SLCHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "% 15d", -42);
			SLCHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "% 15.3f", -42.987);
			SLCHECK_EQ(sbuf, "        -42.987");
			slprintf(sbuf.Z(), "% 15.3f", 42.987);
			SLCHECK_EQ(sbuf, "         42.987");
			slprintf(sbuf.Z(), "% s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "% d", 1024);
			SLCHECK_EQ(sbuf, " 1024");
			slprintf(sbuf.Z(), "% d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "% i", 1024);
			SLCHECK_EQ(sbuf, " 1024");
			slprintf(sbuf.Z(), "% i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "% u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "% u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "% o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "% o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "% x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "% x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "% X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "% X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "% c", 'x');
			SLCHECK_EQ(sbuf, "x");
		}
		{ // TEST_CASE("+ flag", "[]" )
			slprintf(sbuf.Z(), "%+d", 42);
			SLCHECK_EQ(sbuf, "+42");
			slprintf(sbuf.Z(), "%+d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%+5d", 42);
			SLCHECK_EQ(sbuf, "  +42");
			slprintf(sbuf.Z(), "%+5d", -42);
			SLCHECK_EQ(sbuf, "  -42");
			slprintf(sbuf.Z(), "%+15d", 42);
			SLCHECK_EQ(sbuf, "            +42");
			slprintf(sbuf.Z(), "%+15d", -42);
			SLCHECK_EQ(sbuf, "            -42");
			slprintf(sbuf.Z(), "%+s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%+d", 1024);
			SLCHECK_EQ(sbuf, "+1024");
			slprintf(sbuf.Z(), "%+d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%+i", 1024);
			SLCHECK_EQ(sbuf, "+1024");
			slprintf(sbuf.Z(), "%+i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%+u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%+u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%+o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%+o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%+x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%+x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%+X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%+X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%+c", 'x');
			SLCHECK_EQ(sbuf, "x");
			slprintf(sbuf.Z(), "%+.0d", 0);
			SLCHECK_EQ(sbuf, "+");
		}
		{ // TEST_CASE("0 flag", "[]" )
			slprintf(sbuf.Z(), "%0d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0ld", 42L);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%05d", 42);
			SLCHECK_EQ(sbuf, "00042");
			slprintf(sbuf.Z(), "%05d", -42);
			SLCHECK_EQ(sbuf, "-0042");
			slprintf(sbuf.Z(), "%015d", 42);
			SLCHECK_EQ(sbuf, "000000000000042");
			slprintf(sbuf.Z(), "%015d", -42);
			SLCHECK_EQ(sbuf, "-00000000000042");
			slprintf(sbuf.Z(), "%015.2f", 42.1234);
			SLCHECK_EQ(sbuf, "000000000042.12");
			slprintf(sbuf.Z(), "%015.3f", 42.9876);
			SLCHECK_EQ(sbuf, "00000000042.988");
			slprintf(sbuf.Z(), "%015.5f", -42.9876);
			SLCHECK_EQ(sbuf, "-00000042.98760");
		}
		{ // TEST_CASE("- flag", "[]" )
			slprintf(sbuf.Z(), "%-d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%-d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%-5d", 42);
			SLCHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%-5d", -42);
			SLCHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%-15d", 42);
			SLCHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%-15d", -42);
			SLCHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%-0d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%-0d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%-05d", 42);
			SLCHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%-05d", -42);
			SLCHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%-015d", 42);
			SLCHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%-015d", -42);
			SLCHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%0-d", 42);
			SLCHECK_EQ(sbuf, "42");
			slprintf(sbuf.Z(), "%0-d", -42);
			SLCHECK_EQ(sbuf, "-42");
			slprintf(sbuf.Z(), "%0-5d", 42);
			SLCHECK_EQ(sbuf, "42   ");
			slprintf(sbuf.Z(), "%0-5d", -42);
			SLCHECK_EQ(sbuf, "-42  ");
			slprintf(sbuf.Z(), "%0-15d", 42);
			SLCHECK_EQ(sbuf, "42             ");
			slprintf(sbuf.Z(), "%0-15d", -42);
			SLCHECK_EQ(sbuf, "-42            ");
			slprintf(sbuf.Z(), "%0-15.3e", -42.);
			SLCHECK_EQ(sbuf, "-4.200e+01     ");
			sprintf(flat_buffer, "%0-15.3g", -42.); // @proof
			slprintf(sbuf.Z(), "%0-15.3g", -42.);
			SLCHECK_EQ(sbuf, "-42            ");
		}
		{ // TEST_CASE("# flag", "[]" )
			slprintf(sbuf.Z(), "%#.0x", 0);
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%#.1x", 0);
			SLCHECK_EQ(sbuf, "0");
			slprintf(sbuf.Z(), "%#.0llx", (int64)0);
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%#.8x", 0x614e);
			SLCHECK_EQ(sbuf, "0x0000614e");
			slprintf(sbuf.Z(), "%#b", 6);
			SLCHECK_EQ(sbuf, "0b110");
		}
		{ // TEST_CASE("specifier", "[]" )
			slprintf(sbuf.Z(), "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%d", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%i", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%%");
			SLCHECK_EQ(sbuf, "%");
		}
		{ // TEST_CASE("width", "[]" )
			slprintf(sbuf.Z(), "%1s", "Hello testing");
			SLCHECK_EQ(sbuf, "Hello testing");
			slprintf(sbuf.Z(), "%1d", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1d", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%1i", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1i", -1024);
			SLCHECK_EQ(sbuf, "-1024");
			slprintf(sbuf.Z(), "%1u", 1024);
			SLCHECK_EQ(sbuf, "1024");
			slprintf(sbuf.Z(), "%1u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272");
			slprintf(sbuf.Z(), "%1o", 511);
			SLCHECK_EQ(sbuf, "777");
			slprintf(sbuf.Z(), "%1o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001");
			slprintf(sbuf.Z(), "%1x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd");
			slprintf(sbuf.Z(), "%1x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433");
			slprintf(sbuf.Z(), "%1X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD");
			slprintf(sbuf.Z(), "%1X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433");
			slprintf(sbuf.Z(), "%1c", 'x');
			SLCHECK_EQ(sbuf, "x");
		}
		{ // TEST_CASE("width 20", "[]" )
			slprintf(sbuf.Z(), "%20s", "Hello");
			SLCHECK_EQ(sbuf, "               Hello");
			slprintf(sbuf.Z(), "%20d", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20d", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20i", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20i", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20u", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20o", 511);
			SLCHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%20o", 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20x", 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%20x", 3989525555U);
			SLCHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%20X", 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20X", 3989525555U);
			SLCHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%20c", 'x');
			SLCHECK_EQ(sbuf, "                   x");
		}
		{ // TEST_CASE("width *20", "[]" )
			slprintf(sbuf.Z(), "%*s", 20, "Hello");
			SLCHECK_EQ(sbuf, "               Hello");
			slprintf(sbuf.Z(), "%*d", 20, 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*d", 20, -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%*i", 20, 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*i", 20, -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%*u", 20, 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%*u", 20, 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%*o", 20, 511);
			SLCHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%*o", 20, 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%*x", 20, 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%*x", 20, 3989525555U);
			SLCHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%*X", 20, 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%*X", 20, 3989525555U);
			SLCHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%*c", 20,'x');
			SLCHECK_EQ(sbuf, "                   x");
		}
		{ // TEST_CASE("width -20", "[]" )
			slprintf(sbuf.Z(), "%-20s", "Hello");
			SLCHECK_EQ(sbuf, "Hello               ");
			slprintf(sbuf.Z(), "%-20d", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20d", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%-20i", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20i", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%-20u", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%-20.4f", 1024.1234);
			SLCHECK_EQ(sbuf, "1024.1234           ");
			slprintf(sbuf.Z(), "%-20u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272          ");
			slprintf(sbuf.Z(), "%-20o", 511);
			SLCHECK_EQ(sbuf, "777                 ");
			slprintf(sbuf.Z(), "%-20o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001         ");
			slprintf(sbuf.Z(), "%-20x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd            ");
			slprintf(sbuf.Z(), "%-20x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433            ");
			slprintf(sbuf.Z(), "%-20X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD            ");
			slprintf(sbuf.Z(), "%-20X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433            ");
			slprintf(sbuf.Z(), "%-20c", 'x');
			SLCHECK_EQ(sbuf, "x                   ");
			slprintf(sbuf.Z(), "|%5d| |%-2d| |%5d|", 9, 9, 9);
			SLCHECK_EQ(sbuf, "|    9| |9 | |    9|");
			slprintf(sbuf.Z(), "|%5d| |%-2d| |%5d|", 10, 10, 10);
			SLCHECK_EQ(sbuf, "|   10| |10| |   10|");
			slprintf(sbuf.Z(), "|%5d| |%-12d| |%5d|", 9, 9, 9);
			SLCHECK_EQ(sbuf, "|    9| |9           | |    9|");
			slprintf(sbuf.Z(), "|%5d| |%-12d| |%5d|", 10, 10, 10);
			SLCHECK_EQ(sbuf, "|   10| |10          | |   10|");
		}
		{ // TEST_CASE("width 0-20", "[]" )
			slprintf(sbuf.Z(), "%0-20s", "Hello");
			SLCHECK_EQ(sbuf, "Hello               ");
			slprintf(sbuf.Z(), "%0-20d", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20d", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%0-20i", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20i", -1024);
			SLCHECK_EQ(sbuf, "-1024               ");
			slprintf(sbuf.Z(), "%0-20u", 1024);
			SLCHECK_EQ(sbuf, "1024                ");
			slprintf(sbuf.Z(), "%0-20u", 4294966272U);
			SLCHECK_EQ(sbuf, "4294966272          ");
			slprintf(sbuf.Z(), "%0-20o", 511);
			SLCHECK_EQ(sbuf, "777                 ");
			slprintf(sbuf.Z(), "%0-20o", 4294966785U);
			SLCHECK_EQ(sbuf, "37777777001         ");
			slprintf(sbuf.Z(), "%0-20x", 305441741);
			SLCHECK_EQ(sbuf, "1234abcd            ");
			slprintf(sbuf.Z(), "%0-20x", 3989525555U);
			SLCHECK_EQ(sbuf, "edcb5433            ");
			slprintf(sbuf.Z(), "%0-20X", 305441741);
			SLCHECK_EQ(sbuf, "1234ABCD            ");
			slprintf(sbuf.Z(), "%0-20X", 3989525555U);
			SLCHECK_EQ(sbuf, "EDCB5433            ");
			slprintf(sbuf.Z(), "%0-20c", 'x');
			SLCHECK_EQ(sbuf, "x                   ");
		}
		{ // TEST_CASE("padding 20", "[]" ) 
			slprintf(sbuf.Z(), "%020d", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020d", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%020i", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020i", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%020u", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%020u", 4294966272U);
			SLCHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%020o", 511);
			SLCHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%020o", 4294966785U);
			SLCHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%020x", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234abcd");
			slprintf(sbuf.Z(), "%020x", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000edcb5433");
			slprintf(sbuf.Z(), "%020X", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234ABCD");
			slprintf(sbuf.Z(), "%020X", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000EDCB5433");
		}
		{ // TEST_CASE("padding .20", "[]" )
			slprintf(sbuf.Z(), "%.20d", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20d", -1024);
			SLCHECK_EQ(sbuf, "-00000000000000001024");
			slprintf(sbuf.Z(), "%.20i", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20i", -1024);
			SLCHECK_EQ(sbuf, "-00000000000000001024");
			slprintf(sbuf.Z(), "%.20u", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%.20u", 4294966272U);
			SLCHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%.20o", 511);
			SLCHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%.20o", 4294966785U);
			SLCHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%.20x", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234abcd");
			slprintf(sbuf.Z(), "%.20x", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000edcb5433");
			slprintf(sbuf.Z(), "%.20X", 305441741);
			SLCHECK_EQ(sbuf, "0000000000001234ABCD");
			slprintf(sbuf.Z(), "%.20X", 3989525555U);
			SLCHECK_EQ(sbuf, "000000000000EDCB5433");
		}
		{ // TEST_CASE("padding #020", "[]" )
			slprintf(sbuf.Z(), "%#020d", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020d", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%#020i", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020i", -1024);
			SLCHECK_EQ(sbuf, "-0000000000000001024");
			slprintf(sbuf.Z(), "%#020u", 1024);
			SLCHECK_EQ(sbuf, "00000000000000001024");
			slprintf(sbuf.Z(), "%#020u", 4294966272U);
			SLCHECK_EQ(sbuf, "00000000004294966272");
			slprintf(sbuf.Z(), "%#020o", 511);
			SLCHECK_EQ(sbuf, "00000000000000000777");
			slprintf(sbuf.Z(), "%#020o", 4294966785U);
			SLCHECK_EQ(sbuf, "00000000037777777001");
			slprintf(sbuf.Z(), "%#020x", 305441741);
			SLCHECK_EQ(sbuf, "0x00000000001234abcd");
			slprintf(sbuf.Z(), "%#020x", 3989525555U);
			SLCHECK_EQ(sbuf, "0x0000000000edcb5433");
			slprintf(sbuf.Z(), "%#020X", 305441741);
			SLCHECK_EQ(sbuf, "0X00000000001234ABCD");
			slprintf(sbuf.Z(), "%#020X", 3989525555U);
			SLCHECK_EQ(sbuf, "0X0000000000EDCB5433");
		}
		{ // TEST_CASE("padding #20", "[]" )
			slprintf(sbuf.Z(), "%#20d", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20d", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%#20i", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20i", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%#20u", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%#20u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%#20o", 511);
			SLCHECK_EQ(sbuf, "                0777");
			slprintf(sbuf.Z(), "%#20o", 4294966785U);
			SLCHECK_EQ(sbuf, "        037777777001");
			slprintf(sbuf.Z(), "%#20x", 305441741);
			SLCHECK_EQ(sbuf, "          0x1234abcd");
			slprintf(sbuf.Z(), "%#20x", 3989525555U);
			SLCHECK_EQ(sbuf, "          0xedcb5433");
			slprintf(sbuf.Z(), "%#20X", 305441741);
			SLCHECK_EQ(sbuf, "          0X1234ABCD");
			slprintf(sbuf.Z(), "%#20X", 3989525555U);
			SLCHECK_EQ(sbuf, "          0XEDCB5433");
		}
		{ // TEST_CASE("padding 20.5", "[]" )
			slprintf(sbuf.Z(), "%20.5d", 1024);
			SLCHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5d", -1024);
			SLCHECK_EQ(sbuf, "              -01024");
			slprintf(sbuf.Z(), "%20.5i", 1024);
			SLCHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5i", -1024);
			SLCHECK_EQ(sbuf, "              -01024");
			slprintf(sbuf.Z(), "%20.5u", 1024);
			SLCHECK_EQ(sbuf, "               01024");
			slprintf(sbuf.Z(), "%20.5u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20.5o", 511);
			SLCHECK_EQ(sbuf, "               00777");
			slprintf(sbuf.Z(), "%20.5o", 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20.5x", 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%20.10x", 3989525555U);
			SLCHECK_EQ(sbuf, "          00edcb5433");
			slprintf(sbuf.Z(), "%20.5X", 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20.10X", 3989525555U);
			SLCHECK_EQ(sbuf, "          00EDCB5433");
		}
		{ // TEST_CASE("padding neg numbers", "[]" )
			// space padding
			slprintf(sbuf.Z(), "% 1d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "% 2d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "% 3d", -5);
			SLCHECK_EQ(sbuf, " -5");
			slprintf(sbuf.Z(), "% 4d", -5);
			SLCHECK_EQ(sbuf, "  -5");
			// zero padding
			slprintf(sbuf.Z(), "%01d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%02d", -5);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%03d", -5);
			SLCHECK_EQ(sbuf, "-05");
			slprintf(sbuf.Z(), "%04d", -5);
			SLCHECK_EQ(sbuf, "-005");
		}
		{ // TEST_CASE("float padding neg numbers", "[]" )
			// space padding
			slprintf(sbuf.Z(), "% 3.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "% 4.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "% 5.1f", -5.);
			SLCHECK_EQ(sbuf, " -5.0");
			slprintf(sbuf.Z(), "% 6.1g", -5.);
			SLCHECK_EQ(sbuf, "    -5");
			slprintf(sbuf.Z(), "% 6.1e", -5.);
			SLCHECK_EQ(sbuf, "-5.0e+00");
			slprintf(sbuf.Z(), "% 10.1e", -5.);
			SLCHECK_EQ(sbuf, "  -5.0e+00");
			// zero padding
			slprintf(sbuf.Z(), "%03.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "%04.1f", -5.);
			SLCHECK_EQ(sbuf, "-5.0");
			slprintf(sbuf.Z(), "%05.1f", -5.);
			SLCHECK_EQ(sbuf, "-05.0");
			// zero padding no decimal point
			slprintf(sbuf.Z(), "%01.0f", -5.);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%02.0f", -5.);
			SLCHECK_EQ(sbuf, "-5");
			slprintf(sbuf.Z(), "%03.0f", -5.);
			SLCHECK_EQ(sbuf, "-05");
			slprintf(sbuf.Z(), "%010.1e", -5.);
			SLCHECK_EQ(sbuf, "-005.0e+00");
			slprintf(sbuf.Z(), "%07.0E", -5.);
			SLCHECK_EQ(sbuf, "-05E+00");
			slprintf(sbuf.Z(), "%03.0g", -5.);
			SLCHECK_EQ(sbuf, "-05");
		}
		{ // TEST_CASE("length", "[]" )
			slprintf(sbuf.Z(), "%.0s", "Hello testing");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%20.0s", "Hello testing");
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%.s", "Hello testing");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%20.s", "Hello testing");
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.0d", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.0d", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20.d", 0);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.0i", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.i", -1024);
			SLCHECK_EQ(sbuf, "               -1024");
			slprintf(sbuf.Z(), "%20.i", 0);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.u", 1024);
			SLCHECK_EQ(sbuf, "                1024");
			slprintf(sbuf.Z(), "%20.0u", 4294966272U);
			SLCHECK_EQ(sbuf, "          4294966272");
			slprintf(sbuf.Z(), "%20.u", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.o", 511);
			SLCHECK_EQ(sbuf, "                 777");
			slprintf(sbuf.Z(), "%20.0o", 4294966785U);
			SLCHECK_EQ(sbuf, "         37777777001");
			slprintf(sbuf.Z(), "%20.o", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.x", 305441741);
			SLCHECK_EQ(sbuf, "            1234abcd");
			slprintf(sbuf.Z(), "%50.x", 305441741);
			SLCHECK_EQ(sbuf, "                                          1234abcd");
			slprintf(sbuf.Z(), "%50.x%10.u", 305441741, 12345);
			SLCHECK_EQ(sbuf, "                                          1234abcd     12345");
			slprintf(sbuf.Z(), "%20.0x", 3989525555U);
			SLCHECK_EQ(sbuf, "            edcb5433");
			slprintf(sbuf.Z(), "%20.x", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%20.X", 305441741);
			SLCHECK_EQ(sbuf, "            1234ABCD");
			slprintf(sbuf.Z(), "%20.0X", 3989525555U);
			SLCHECK_EQ(sbuf, "            EDCB5433");
			slprintf(sbuf.Z(), "%20.X", 0U);
			SLCHECK_EQ(sbuf, "                    ");
			slprintf(sbuf.Z(), "%02.0u", 0U);
			SLCHECK_EQ(sbuf, "  ");
			slprintf(sbuf.Z(), "%02.0d", 0);
			SLCHECK_EQ(sbuf, "  ");
		}
		{ // TEST_CASE("float", "[]" )
			// test special-case floats using math.h macros
			slprintf(sbuf.Z(), "%8f", fgetnan());
			SLCHECK_EQ(sbuf, "     nan");
			slprintf(sbuf.Z(), "%8f", fgetposinf());
			SLCHECK_EQ(sbuf, "     inf");
			slprintf(sbuf.Z(), "%-8f", fgetneginf());
			SLCHECK_EQ(sbuf, "-inf    ");
			slprintf(sbuf.Z(), "%+8e", fgetposinf());
			SLCHECK_EQ(sbuf, "    +inf");
			slprintf(sbuf.Z(), "%.4f", 3.1415354);
			SLCHECK_EQ(sbuf, "3.1415");
			slprintf(sbuf.Z(), "%.3f", 30343.1415354);
			SLCHECK_EQ(sbuf, "30343.142");
			slprintf(sbuf.Z(), "%.0f", 34.1415354);
			SLCHECK_EQ(sbuf, "34");
			slprintf(sbuf.Z(), "%.0f", 1.3);
			SLCHECK_EQ(sbuf, "1");
			slprintf(sbuf.Z(), "%.0f", 1.55);
			SLCHECK_EQ(sbuf, "2");
			slprintf(sbuf.Z(), "%.1f", 1.64);
			SLCHECK_EQ(sbuf, "1.6");
			slprintf(sbuf.Z(), "%.2f", 42.8952);
			SLCHECK_EQ(sbuf, "42.90");
			slprintf(sbuf.Z(), "%.9f", 42.8952);
			SLCHECK_EQ(sbuf, "42.895200000");
			slprintf(sbuf.Z(), "%.10f", 42.895223);
			SLCHECK_EQ(sbuf, "42.8952230000");
			// this testcase checks, that the precision is truncated to 9 digits.
			// a perfect working float should return the whole number
			slprintf(sbuf.Z(), "%.12f", 42.89522312345678);
			SLCHECK_EQ(sbuf, "42.895223123457"); // @sobolev "42.895223123000"-->"42.895223123457"
			// this testcase checks, that the precision is truncated AND rounded to 9 digits.
			// a perfect working float should return the whole number
			slprintf(sbuf.Z(), "%.12f", 42.89522387654321);
			SLCHECK_EQ(sbuf, "42.895223876543"); // @sobolev "42.895223877000"-->"42.895223876543"
			slprintf(sbuf.Z(), "%6.2f", 42.8952);
			SLCHECK_EQ(sbuf, " 42.90");
			slprintf(sbuf.Z(), "%+6.2f", 42.8952);
			SLCHECK_EQ(sbuf, "+42.90");
			slprintf(sbuf.Z(), "%+5.1f", 42.9252);
			SLCHECK_EQ(sbuf, "+42.9");
			sprintf(flat_buffer, "%f", 42.5); // @proof
			slprintf(sbuf.Z(), "%f", 42.5);
			SLCHECK_EQ(sbuf, "42.500000");
			slprintf(sbuf.Z(), "%.1f", 42.5);
			SLCHECK_EQ(sbuf, "42.5");
			slprintf(sbuf.Z(), "%f", 42167.0);
			SLCHECK_EQ(sbuf, "42167.000000");
			slprintf(sbuf.Z(), "%.9f", -12345.987654321);
			SLCHECK_EQ(sbuf, "-12345.987654321");
			slprintf(sbuf.Z(), "%.1f", 3.999);
			SLCHECK_EQ(sbuf, "4.0");
			slprintf(sbuf.Z(), "%.0f", 3.5);
			SLCHECK_EQ(sbuf, "4");
			sprintf(flat_buffer, "%.0f", 4.5); // @proof
			slprintf(sbuf.Z(), "%.0f", 4.5);
			SLCHECK_EQ(sbuf, "5"); // @sobolev "4"-->"5"
			slprintf(sbuf.Z(), "%.0f", 3.49);
			SLCHECK_EQ(sbuf, "3");
			slprintf(sbuf.Z(), "%.1f", 3.49);
			SLCHECK_EQ(sbuf, "3.5");
			slprintf(sbuf.Z(), "a%-5.1f", 0.5);
			SLCHECK_EQ(sbuf, "a0.5  ");
			slprintf(sbuf.Z(), "a%-5.1fend", 0.5);
			SLCHECK_EQ(sbuf, "a0.5  end");
			slprintf(sbuf.Z(), "%G", 12345.678);
			SLCHECK_EQ(sbuf, "12345.7");
			slprintf(sbuf.Z(), "%.7G", 12345.678);
			SLCHECK_EQ(sbuf, "12345.68");
			slprintf(sbuf.Z(), "%.5G", 123456789.);
			SLCHECK_EQ(sbuf, "1.2346E+08");
			sprintf(flat_buffer, "%.6G", 12345.); // @proof
			slprintf(sbuf.Z(), "%.6G", 12345.);
			SLCHECK_EQ(sbuf, flat_buffer);
			SLCHECK_EQ(sbuf, "12345"); // @sobolev "12345.0"-->"12345"
			slprintf(sbuf.Z(), "%+12.4g", 123456789.);
			SLCHECK_EQ(sbuf, "  +1.235e+08");
			slprintf(sbuf.Z(), "%.2G", 0.001234);
			SLCHECK_EQ(sbuf, "0.0012");
			slprintf(sbuf.Z(), "%+10.4G", 0.001234);
			SLCHECK_EQ(sbuf, " +0.001234");
			slprintf(sbuf.Z(), "%+012.4g", 0.00001234);
			SLCHECK_EQ(sbuf, "+001.234e-05");
			slprintf(sbuf.Z(), "%.3g", -1.2345e-308);
			SLCHECK_EQ(sbuf, "-1.23e-308");
			slprintf(sbuf.Z(), "%+.3E", 1.23e+308);
			SLCHECK_EQ(sbuf, "+1.230E+308");
			// out of range for float: should switch to exp notation if supported, else empty
			sprintf(flat_buffer, "%.1f", 1E20); // @proof
			slprintf(sbuf.Z(), "%.1f", 1E20);
			SLCHECK_EQ(sbuf, flat_buffer);
			SLCHECK_EQ(sbuf, "100000000000000000000.0"); // @sobolev "1.0e+20"-->"100000000000000000000.0"

			// brute force float
			for(float i = -100000.0f; i < 100000.0f; i += 1.0f) {
				slprintf(sbuf.Z(), "%.5f", i / 10000.0f);
				sprintf(flat_buffer, "%.5f", i / 10000.0f);
				SLCHECK_EQ(sbuf, flat_buffer);
			}
			//
			// brute force exp
			// @sobolev Пришлось модифицировать следующий тест,ограничив точность с 1e20 до 1e15 из-за того, что
			// разные реализации (даже в рамках одного компилятора разных версий) могут по-разному формировать
			// последние значащие цифры величин большой точности.
			// Для вящей убедительности я увеличил количество итераций инкрементируя величину случайным значением,
			// порядка 1e8 (1e-1 * 1e9).
			//
			/*for(float i = -1.0e20f; i < 1e20f; i += 1e15f) {
				slprintf(sbuf.Z(), "%.5f", i);
				sprintf(flat_buffer, "%.5f", i);
				SLCHECK_EQ(sbuf, flat_buffer);
			}*/
			for(float i = -1.0e15f; i < 1e15f;) {
				slprintf(sbuf.Z(), "%.5f", i);
				sprintf(flat_buffer, "%.5f", i);
				SLCHECK_EQ(sbuf, flat_buffer);
				float rn = static_cast<float>(SLS.GetTLA().Rg.GetReal()) * 1.0e9f;
				i += rn;
			}
		}
		{ // TEST_CASE("types", "[]" )
			slprintf(sbuf.Z(), "%i", 0);
			SLCHECK_EQ(sbuf, "0");
			slprintf(sbuf.Z(), "%i", 1234);
			SLCHECK_EQ(sbuf, "1234");
			slprintf(sbuf.Z(), "%i", 32767);
			SLCHECK_EQ(sbuf, "32767");
			slprintf(sbuf.Z(), "%i", -32767);
			SLCHECK_EQ(sbuf, "-32767");
			slprintf(sbuf.Z(), "%li", 30L);
			SLCHECK_EQ(sbuf, "30");
			slprintf(sbuf.Z(), "%li", -2147483647L);
			SLCHECK_EQ(sbuf, "-2147483647");
			slprintf(sbuf.Z(), "%li", 2147483647L);
			SLCHECK_EQ(sbuf, "2147483647");
			slprintf(sbuf.Z(), "%lli", 30LL);
			SLCHECK_EQ(sbuf, "30");
			slprintf(sbuf.Z(), "%lli", -9223372036854775807LL);
			SLCHECK_EQ(sbuf, "-9223372036854775807");
			slprintf(sbuf.Z(), "%lli", 9223372036854775807LL);
			SLCHECK_EQ(sbuf, "9223372036854775807");
			slprintf(sbuf.Z(), "%lu", 100000L);
			SLCHECK_EQ(sbuf, "100000");
			slprintf(sbuf.Z(), "%lu", 0xFFFFFFFFL);
			SLCHECK_EQ(sbuf, "4294967295");
			slprintf(sbuf.Z(), "%llu", 281474976710656ULL);
			SLCHECK_EQ(sbuf, "281474976710656");
			slprintf(sbuf.Z(), "%llu", 18446744073709551615ULL);
			SLCHECK_EQ(sbuf, "18446744073709551615");
			slprintf(sbuf.Z(), "%zu", 2147483647UL);
			SLCHECK_EQ(sbuf, "2147483647");
			slprintf(sbuf.Z(), "%zd", 2147483647UL);
			SLCHECK_EQ(sbuf, "2147483647");
			if(sizeof(size_t) == sizeof(long)) {
				slprintf(sbuf.Z(), "%zi", -2147483647L);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
			else {
				slprintf(sbuf.Z(), "%zi", -2147483647LL);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
			slprintf(sbuf.Z(), "%b", 60000);
			SLCHECK_EQ(sbuf, "1110101001100000");
			slprintf(sbuf.Z(), "%lb", 12345678L);
			SLCHECK_EQ(sbuf, "101111000110000101001110");
			slprintf(sbuf.Z(), "%o", 60000);
			SLCHECK_EQ(sbuf, "165140");
			slprintf(sbuf.Z(), "%lo", 12345678L);
			SLCHECK_EQ(sbuf, "57060516");
			slprintf(sbuf.Z(), "%lx", 0x12345678L);
			SLCHECK_EQ(sbuf, "12345678");
			slprintf(sbuf.Z(), "%llx", 0x1234567891234567ULL);
			SLCHECK_EQ(sbuf, "1234567891234567");
			slprintf(sbuf.Z(), "%lx", 0xabcdefabL);
			SLCHECK_EQ(sbuf, "abcdefab");
			slprintf(sbuf.Z(), "%lX", 0xabcdefabL);
			SLCHECK_EQ(sbuf, "ABCDEFAB");
			slprintf(sbuf.Z(), "%c", 'v');
			SLCHECK_EQ(sbuf, "v");
			slprintf(sbuf.Z(), "%cv", 'w');
			SLCHECK_EQ(sbuf, "wv");
			slprintf(sbuf.Z(), "%s", "A Test");
			SLCHECK_EQ(sbuf, "A Test");
			slprintf(sbuf.Z(), "%hhu", 0xFFFFUL);
			SLCHECK_EQ(sbuf, "255");
			slprintf(sbuf.Z(), "%hu", 0x123456UL);
			SLCHECK_EQ(sbuf, "13398");
			slprintf(sbuf.Z(), "%s%hhi %hu", "Test", 10000, 0xFFFFFFFF);
			SLCHECK_EQ(sbuf, "Test16 65535");
			slprintf(sbuf.Z(), "%tx", &flat_buffer[10] - &flat_buffer[0]);
			SLCHECK_EQ(sbuf, "a");
			// TBD
			if(sizeof(intmax_t) == sizeof(long)) {
				slprintf(sbuf.Z(), "%ji", -2147483647L);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
			else {
				slprintf(sbuf.Z(), "%ji", -2147483647LL);
				SLCHECK_EQ(sbuf, "-2147483647");
			}
		}
		{ // TEST_CASE("pointer", "[]" )
			slprintf(sbuf.Z(), "%p", (void *)0x1234U);
			if(sizeof(void *) == 4U) {
				SLCHECK_EQ(sbuf, "00001234");
			}
			else {
				SLCHECK_EQ(sbuf, "0000000000001234");
			}
			slprintf(sbuf.Z(), "%p", (void *)0x12345678U);
			if(sizeof(void *) == 4U) {
				SLCHECK_EQ(sbuf, "12345678");
			}
			else {
				SLCHECK_EQ(sbuf, "0000000012345678");
			}
			slprintf(sbuf.Z(), "%p-%p", (void *)0x12345678U, (void *)0x7EDCBA98U);
			if(sizeof(void *) == 4U) {
				SLCHECK_EQ(sbuf, "12345678-7EDCBA98");
			}
			else {
				SLCHECK_EQ(sbuf, "0000000012345678-000000007EDCBA98");
			}
			if(sizeof(uintptr_t) == sizeof(uint64_t)) {
				slprintf(sbuf.Z(), "%p", (void *)(uintptr_t)0xFFFFFFFFU);
				SLCHECK_EQ(sbuf, "00000000FFFFFFFF");
			}
			else {
				slprintf(sbuf.Z(), "%p", (void *)(uintptr_t)0xFFFFFFFFU);
				SLCHECK_EQ(sbuf, "FFFFFFFF");
			}
		}
		{ // TEST_CASE("unknown flag", "[]" )
			slprintf(sbuf.Z(), "%kmarco", 42, 37);
			SLCHECK_EQ(sbuf, "kmarco");
		}
		{ // TEST_CASE("string length", "[]" )
			slprintf(sbuf.Z(), "%.4s", "This is a test");
			SLCHECK_EQ(sbuf, "This");
			slprintf(sbuf.Z(), "%.4s", "test");
			SLCHECK_EQ(sbuf, "test");
			slprintf(sbuf.Z(), "%.7s", "123");
			SLCHECK_EQ(sbuf, "123");
			slprintf(sbuf.Z(), "%.7s", "");
			SLCHECK_EQ(sbuf, "");
			slprintf(sbuf.Z(), "%.4s%.2s", "123456", "abcdef");
			SLCHECK_EQ(sbuf, "1234ab");
			slprintf(sbuf.Z(), "%.4.2s", "123456");
			SLCHECK_EQ(sbuf, ".2s");
			slprintf(sbuf.Z(), "%.*s", 3, "123456");
			SLCHECK_EQ(sbuf, "123");
		}
		{ // TEST_CASE("misc", "[]" )
			slprintf(sbuf.Z(), "%u%u%ctest%d %s", 5, 3000, 'a', -20, "bit");
			SLCHECK_EQ(sbuf, "53000atest-20 bit");
			slprintf(sbuf.Z(), "%.*f", 2, 0.33333333);
			SLCHECK_EQ(sbuf, "0.33");
			slprintf(sbuf.Z(), "%.*d", -1, 1);
			SLCHECK_EQ(sbuf, "1");
			slprintf(sbuf.Z(), "%.3s", "foobar");
			SLCHECK_EQ(sbuf, "foo");
			slprintf(sbuf.Z(), "% .0d", 0);
			SLCHECK_EQ(sbuf, " ");
			slprintf(sbuf.Z(), "%10.5d", 4);
			SLCHECK_EQ(sbuf, "     00004");
			slprintf(sbuf.Z(), "%*sx", -3, "hi");
			SLCHECK_EQ(sbuf, "hi x");
			slprintf(sbuf.Z(), "%.*g", 2, 0.33333333);
			SLCHECK_EQ(sbuf, "0.33");
			slprintf(sbuf.Z(), "%.*e", 2, 0.33333333);
			SLCHECK_EQ(sbuf, "3.33e-01");
		}
#if 1 // @construction {
		{
			//
			// Попытка проверить slprintf на тестах curl (там эти тесты верифицируют собственную имплементацию ..printf)
			//
			#if (SIZEOF_CURL_OFF_T > SIZEOF_LONG)
				#define MPRNT_SUFFIX_CURL_OFF_T  LL
			#else
				#define MPRNT_SUFFIX_CURL_OFF_T  L
			#endif
			#define MPRNT_OFF_T_C_HELPER2(Val, Suffix) Val ## Suffix 
			#define MPRNT_OFF_T_C_HELPER1(Val, Suffix) MPRNT_OFF_T_C_HELPER2(Val, Suffix)
			#define MPRNT_OFF_T_C(Val)  MPRNT_OFF_T_C_HELPER1(Val, MPRNT_SUFFIX_CURL_OFF_T)
			#define BUFSZ    256
			#define USHORT_TESTS_ARRSZ 1 + 100
			#define SSHORT_TESTS_ARRSZ 1 + 100
			#define UINT_TESTS_ARRSZ   1 + 100
			#define SINT_TESTS_ARRSZ   1 + 100
			#define ULONG_TESTS_ARRSZ  1 + 100
			#define SLONG_TESTS_ARRSZ  1 + 100
			#define COFFT_TESTS_ARRSZ  1 + 100

			struct unsshort_st {
				ushort num; /* unsigned short  */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};
			struct sigshort_st {
				short num;      /* signed short    */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};
			struct unsint_st {
				uint num; /* unsigned int    */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};
			struct sigint_st {
				int num;        /* signed int      */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};
			struct unslong_st {
				ulong num; /* unsigned long   */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};
			struct siglong_st {
				long num;       /* signed long     */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};
			struct curloff_st {
				off_t num; /* curl_off_t      */
				const char * expected; // expected string
				char result[BUFSZ]; /* result string   */
			};

			static struct unsshort_st us_test[USHORT_TESTS_ARRSZ];
			static struct sigshort_st ss_test[SSHORT_TESTS_ARRSZ];
			static struct unsint_st ui_test[UINT_TESTS_ARRSZ];
			static struct sigint_st si_test[SINT_TESTS_ARRSZ];
			static struct unslong_st ul_test[ULONG_TESTS_ARRSZ];
			static struct siglong_st sl_test[SLONG_TESTS_ARRSZ];
			static struct curloff_st co_test[COFFT_TESTS_ARRSZ];
#pragma warning(push)
#pragma warning(disable: 4305)
#pragma warning(disable: 4309)
			class InnerBlock {
			public:
				static int test_unsigned_short_formatting()
				{
					int i, j;
					int num_ushort_tests = 0;
					int failed = 0;
				//#if (SIZEOF_SHORT == 1)
					if(sizeof(short) == 1) {
						i = 1; us_test[i].num = 0xFFU; us_test[i].expected = "256";
						i++; us_test[i].num = 0xF0U; us_test[i].expected = "240";
						i++; us_test[i].num = 0x0FU; us_test[i].expected = "15";
						i++; us_test[i].num = 0xE0U; us_test[i].expected = "224";
						i++; us_test[i].num = 0x0EU; us_test[i].expected = "14";
						i++; us_test[i].num = 0xC0U; us_test[i].expected = "192";
						i++; us_test[i].num = 0x0CU; us_test[i].expected = "12";
						i++; us_test[i].num = 0x01U; us_test[i].expected = "1";
						i++; us_test[i].num = 0x00U; us_test[i].expected = "0";
						num_ushort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 2)
					else if(sizeof(short) == 2) {
						i = 1; us_test[i].num = 0xFFFFU; us_test[i].expected = "65535";
						i++; us_test[i].num = 0xFF00U; us_test[i].expected = "65280";
						i++; us_test[i].num = 0x00FFU; us_test[i].expected = "255";
						i++; us_test[i].num = 0xF000U; us_test[i].expected = "61440";
						i++; us_test[i].num = 0x0F00U; us_test[i].expected = "3840";
						i++; us_test[i].num = 0x00F0U; us_test[i].expected = "240";
						i++; us_test[i].num = 0x000FU; us_test[i].expected = "15";
						i++; us_test[i].num = 0xC000U; us_test[i].expected = "49152";
						i++; us_test[i].num = 0x0C00U; us_test[i].expected = "3072";
						i++; us_test[i].num = 0x00C0U; us_test[i].expected = "192";
						i++; us_test[i].num = 0x000CU; us_test[i].expected = "12";
						i++; us_test[i].num = 0x0001U; us_test[i].expected = "1";
						i++; us_test[i].num = 0x0000U; us_test[i].expected = "0";
						num_ushort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 4)
					else if(sizeof(short) == 4) {
						i = 1; us_test[i].num = 0xFFFFFFFFU; us_test[i].expected = "4294967295";
						i++; us_test[i].num = 0xFFFF0000U; us_test[i].expected = "4294901760";
						i++; us_test[i].num = 0x0000FFFFU; us_test[i].expected = "65535";
						i++; us_test[i].num = 0xFF000000U; us_test[i].expected = "4278190080";
						i++; us_test[i].num = 0x00FF0000U; us_test[i].expected = "16711680";
						i++; us_test[i].num = 0x0000FF00U; us_test[i].expected = "65280";
						i++; us_test[i].num = 0x000000FFU; us_test[i].expected = "255";
						i++; us_test[i].num = 0xF0000000U; us_test[i].expected = "4026531840";
						i++; us_test[i].num = 0x0F000000U; us_test[i].expected = "251658240";
						i++; us_test[i].num = 0x00F00000U; us_test[i].expected = "15728640";
						i++; us_test[i].num = 0x000F0000U; us_test[i].expected = "983040";
						i++; us_test[i].num = 0x0000F000U; us_test[i].expected = "61440";
						i++; us_test[i].num = 0x00000F00U; us_test[i].expected = "3840";
						i++; us_test[i].num = 0x000000F0U; us_test[i].expected = "240";
						i++; us_test[i].num = 0x0000000FU; us_test[i].expected = "15";
						i++; us_test[i].num = 0xC0000000U; us_test[i].expected = "3221225472";
						i++; us_test[i].num = 0x0C000000U; us_test[i].expected = "201326592";
						i++; us_test[i].num = 0x00C00000U; us_test[i].expected = "12582912";
						i++; us_test[i].num = 0x000C0000U; us_test[i].expected = "786432";
						i++; us_test[i].num = 0x0000C000U; us_test[i].expected = "49152";
						i++; us_test[i].num = 0x00000C00U; us_test[i].expected = "3072";
						i++; us_test[i].num = 0x000000C0U; us_test[i].expected = "192";
						i++; us_test[i].num = 0x0000000CU; us_test[i].expected = "12";
						i++; us_test[i].num = 0x00000001U; us_test[i].expected = "1";
						i++; us_test[i].num = 0x00000000U; us_test[i].expected = "0";
						num_ushort_tests = i;
					}
				//#endif
					for(i = 1; i <= num_ushort_tests; i++) {
						for(j = 0; j < BUFSZ; j++)
							us_test[i].result[j] = 'X';
						us_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(us_test[i].result, sizeof(us_test[i].result), "%hu", us_test[i].num);
						if(memcmp(us_test[i].result, us_test[i].expected, strlen(us_test[i].expected))) {
							printf("unsigned short test #%.2d: Failed (Expected: %s Got: %s)\n", i, us_test[i].expected, us_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() unsigned short tests OK!\n");
					else
						printf("Some curl_mprintf() unsigned short tests Failed!\n");
					return failed;
				}
				static int test_signed_short_formatting()
				{
					int i, j;
					int num_sshort_tests = 0;
					int failed = 0;
				//#if (SIZEOF_SHORT == 1)
					if(sizeof(short) == 1) {
						i = 1; ss_test[i].num = 0x7F; ss_test[i].expected = "127";
						i++; ss_test[i].num = 0x70; ss_test[i].expected = "112";
						i++; ss_test[i].num = 0x07; ss_test[i].expected = "7";
						i++; ss_test[i].num = 0x50; ss_test[i].expected = "80";
						i++; ss_test[i].num = 0x05; ss_test[i].expected = "5";
						i++; ss_test[i].num = 0x01; ss_test[i].expected = "1";
						i++; ss_test[i].num = 0x00; ss_test[i].expected = "0";
						i++; ss_test[i].num = -0x7F -1; ss_test[i].expected = "-128";
						i++; ss_test[i].num = -0x70 -1; ss_test[i].expected = "-113";
						i++; ss_test[i].num = -0x07 -1; ss_test[i].expected = "-8";
						i++; ss_test[i].num = -0x50 -1; ss_test[i].expected = "-81";
						i++; ss_test[i].num = -0x05 -1; ss_test[i].expected = "-6";
						i++; ss_test[i].num =  0x00 -1; ss_test[i].expected = "-1";
						num_sshort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 2)
					else if(sizeof(short) == 2) {
						i = 1; ss_test[i].num = 0x7FFF; ss_test[i].expected = "32767";
						i++; ss_test[i].num = 0x7FFE; ss_test[i].expected = "32766";
						i++; ss_test[i].num = 0x7FFD; ss_test[i].expected = "32765";
						i++; ss_test[i].num = 0x7F00; ss_test[i].expected = "32512";
						i++; ss_test[i].num = 0x07F0; ss_test[i].expected = "2032";
						i++; ss_test[i].num = 0x007F; ss_test[i].expected = "127";
						i++; ss_test[i].num = 0x7000; ss_test[i].expected = "28672";
						i++; ss_test[i].num = 0x0700; ss_test[i].expected = "1792";
						i++; ss_test[i].num = 0x0070; ss_test[i].expected = "112";
						i++; ss_test[i].num = 0x0007; ss_test[i].expected = "7";
						i++; ss_test[i].num = 0x5000; ss_test[i].expected = "20480";
						i++; ss_test[i].num = 0x0500; ss_test[i].expected = "1280";
						i++; ss_test[i].num = 0x0050; ss_test[i].expected = "80";
						i++; ss_test[i].num = 0x0005; ss_test[i].expected = "5";
						i++; ss_test[i].num = 0x0001; ss_test[i].expected = "1";
						i++; ss_test[i].num = 0x0000; ss_test[i].expected = "0";
						i++; ss_test[i].num = -0x7FFF -1; ss_test[i].expected = "-32768";
						i++; ss_test[i].num = -0x7FFE -1; ss_test[i].expected = "-32767";
						i++; ss_test[i].num = -0x7FFD -1; ss_test[i].expected = "-32766";
						i++; ss_test[i].num = -0x7F00 -1; ss_test[i].expected = "-32513";
						i++; ss_test[i].num = -0x07F0 -1; ss_test[i].expected = "-2033";
						i++; ss_test[i].num = -0x007F -1; ss_test[i].expected = "-128";
						i++; ss_test[i].num = -0x7000 -1; ss_test[i].expected = "-28673";
						i++; ss_test[i].num = -0x0700 -1; ss_test[i].expected = "-1793";
						i++; ss_test[i].num = -0x0070 -1; ss_test[i].expected = "-113";
						i++; ss_test[i].num = -0x0007 -1; ss_test[i].expected = "-8";
						i++; ss_test[i].num = -0x5000 -1; ss_test[i].expected = "-20481";
						i++; ss_test[i].num = -0x0500 -1; ss_test[i].expected = "-1281";
						i++; ss_test[i].num = -0x0050 -1; ss_test[i].expected = "-81";
						i++; ss_test[i].num = -0x0005 -1; ss_test[i].expected = "-6";
						i++; ss_test[i].num =  0x0000 -1; ss_test[i].expected = "-1";
						num_sshort_tests = i;
					}
				//#elif (SIZEOF_SHORT == 4)
					else if(sizeof(short) == 4) {
						i = 1; ss_test[i].num = 0x7FFFFFFF; ss_test[i].expected = "2147483647";
						i++; ss_test[i].num = 0x7FFFFFFE; ss_test[i].expected = "2147483646";
						i++; ss_test[i].num = 0x7FFFFFFD; ss_test[i].expected = "2147483645";
						i++; ss_test[i].num = 0x7FFF0000; ss_test[i].expected = "2147418112";
						i++; ss_test[i].num = 0x00007FFF; ss_test[i].expected = "32767";
						i++; ss_test[i].num = 0x7F000000; ss_test[i].expected = "2130706432";
						i++; ss_test[i].num = 0x007F0000; ss_test[i].expected = "8323072";
						i++; ss_test[i].num = 0x00007F00; ss_test[i].expected = "32512";
						i++; ss_test[i].num = 0x0000007F; ss_test[i].expected = "127";
						i++; ss_test[i].num = 0x70000000; ss_test[i].expected = "1879048192";
						i++; ss_test[i].num = 0x07000000; ss_test[i].expected = "117440512";
						i++; ss_test[i].num = 0x00700000; ss_test[i].expected = "7340032";
						i++; ss_test[i].num = 0x00070000; ss_test[i].expected = "458752";
						i++; ss_test[i].num = 0x00007000; ss_test[i].expected = "28672";
						i++; ss_test[i].num = 0x00000700; ss_test[i].expected = "1792";
						i++; ss_test[i].num = 0x00000070; ss_test[i].expected = "112";
						i++; ss_test[i].num = 0x00000007; ss_test[i].expected = "7";
						i++; ss_test[i].num = 0x50000000; ss_test[i].expected = "1342177280";
						i++; ss_test[i].num = 0x05000000; ss_test[i].expected = "83886080";
						i++; ss_test[i].num = 0x00500000; ss_test[i].expected = "5242880";
						i++; ss_test[i].num = 0x00050000; ss_test[i].expected = "327680";
						i++; ss_test[i].num = 0x00005000; ss_test[i].expected = "20480";
						i++; ss_test[i].num = 0x00000500; ss_test[i].expected = "1280";
						i++; ss_test[i].num = 0x00000050; ss_test[i].expected = "80";
						i++; ss_test[i].num = 0x00000005; ss_test[i].expected = "5";
						i++; ss_test[i].num = 0x00000001; ss_test[i].expected = "1";
						i++; ss_test[i].num = 0x00000000; ss_test[i].expected = "0";
						i++; ss_test[i].num = -0x7FFFFFFF -1; ss_test[i].expected = "-2147483648";
						i++; ss_test[i].num = -0x7FFFFFFE -1; ss_test[i].expected = "-2147483647";
						i++; ss_test[i].num = -0x7FFFFFFD -1; ss_test[i].expected = "-2147483646";
						i++; ss_test[i].num = -0x7FFF0000 -1; ss_test[i].expected = "-2147418113";
						i++; ss_test[i].num = -0x00007FFF -1; ss_test[i].expected = "-32768";
						i++; ss_test[i].num = -0x7F000000 -1; ss_test[i].expected = "-2130706433";
						i++; ss_test[i].num = -0x007F0000 -1; ss_test[i].expected = "-8323073";
						i++; ss_test[i].num = -0x00007F00 -1; ss_test[i].expected = "-32513";
						i++; ss_test[i].num = -0x0000007F -1; ss_test[i].expected = "-128";
						i++; ss_test[i].num = -0x70000000 -1; ss_test[i].expected = "-1879048193";
						i++; ss_test[i].num = -0x07000000 -1; ss_test[i].expected = "-117440513";
						i++; ss_test[i].num = -0x00700000 -1; ss_test[i].expected = "-7340033";
						i++; ss_test[i].num = -0x00070000 -1; ss_test[i].expected = "-458753";
						i++; ss_test[i].num = -0x00007000 -1; ss_test[i].expected = "-28673";
						i++; ss_test[i].num = -0x00000700 -1; ss_test[i].expected = "-1793";
						i++; ss_test[i].num = -0x00000070 -1; ss_test[i].expected = "-113";
						i++; ss_test[i].num = -0x00000007 -1; ss_test[i].expected = "-8";
						i++; ss_test[i].num = -0x50000000 -1; ss_test[i].expected = "-1342177281";
						i++; ss_test[i].num = -0x05000000 -1; ss_test[i].expected = "-83886081";
						i++; ss_test[i].num = -0x00500000 -1; ss_test[i].expected = "-5242881";
						i++; ss_test[i].num = -0x00050000 -1; ss_test[i].expected = "-327681";
						i++; ss_test[i].num = -0x00005000 -1; ss_test[i].expected = "-20481";
						i++; ss_test[i].num = -0x00000500 -1; ss_test[i].expected = "-1281";
						i++; ss_test[i].num = -0x00000050 -1; ss_test[i].expected = "-81";
						i++; ss_test[i].num = -0x00000005 -1; ss_test[i].expected = "-6";
						i++; ss_test[i].num =  0x00000000 -1; ss_test[i].expected = "-1";
						num_sshort_tests = i;
					}
				//#endif
					for(i = 1; i <= num_sshort_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							ss_test[i].result[j] = 'X';
						ss_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(ss_test[i].result, sizeof(ss_test[i].result), "%hd", ss_test[i].num);
						if(memcmp(ss_test[i].result,
							ss_test[i].expected,
							strlen(ss_test[i].expected))) {
							printf("signed short test #%.2d: Failed (Expected: %s Got: %s)\n", i, ss_test[i].expected, ss_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() signed short tests OK!\n");
					else
						printf("Some curl_mprintf() signed short tests Failed!\n");
					return failed;
				}
				static int test_unsigned_int_formatting()
				{
					int i, j;
					int num_uint_tests = 0;
					int failed = 0;
				//#if (SIZEOF_INT == 2)
					if(sizeof(int) == 2) {
						i = 1; ui_test[i].num = 0xFFFFU; ui_test[i].expected = "65535";
						i++; ui_test[i].num = 0xFF00U; ui_test[i].expected = "65280";
						i++; ui_test[i].num = 0x00FFU; ui_test[i].expected = "255";
						i++; ui_test[i].num = 0xF000U; ui_test[i].expected = "61440";
						i++; ui_test[i].num = 0x0F00U; ui_test[i].expected = "3840";
						i++; ui_test[i].num = 0x00F0U; ui_test[i].expected = "240";
						i++; ui_test[i].num = 0x000FU; ui_test[i].expected = "15";
						i++; ui_test[i].num = 0xC000U; ui_test[i].expected = "49152";
						i++; ui_test[i].num = 0x0C00U; ui_test[i].expected = "3072";
						i++; ui_test[i].num = 0x00C0U; ui_test[i].expected = "192";
						i++; ui_test[i].num = 0x000CU; ui_test[i].expected = "12";
						i++; ui_test[i].num = 0x0001U; ui_test[i].expected = "1";
						i++; ui_test[i].num = 0x0000U; ui_test[i].expected = "0";
						num_uint_tests = i;
					}
				//#elif (SIZEOF_INT == 4)
					else if(sizeof(int) == 4) {
						i = 1; ui_test[i].num = 0xFFFFFFFFU; ui_test[i].expected = "4294967295";
						i++; ui_test[i].num = 0xFFFF0000U; ui_test[i].expected = "4294901760";
						i++; ui_test[i].num = 0x0000FFFFU; ui_test[i].expected = "65535";
						i++; ui_test[i].num = 0xFF000000U; ui_test[i].expected = "4278190080";
						i++; ui_test[i].num = 0x00FF0000U; ui_test[i].expected = "16711680";
						i++; ui_test[i].num = 0x0000FF00U; ui_test[i].expected = "65280";
						i++; ui_test[i].num = 0x000000FFU; ui_test[i].expected = "255";
						i++; ui_test[i].num = 0xF0000000U; ui_test[i].expected = "4026531840";
						i++; ui_test[i].num = 0x0F000000U; ui_test[i].expected = "251658240";
						i++; ui_test[i].num = 0x00F00000U; ui_test[i].expected = "15728640";
						i++; ui_test[i].num = 0x000F0000U; ui_test[i].expected = "983040";
						i++; ui_test[i].num = 0x0000F000U; ui_test[i].expected = "61440";
						i++; ui_test[i].num = 0x00000F00U; ui_test[i].expected = "3840";
						i++; ui_test[i].num = 0x000000F0U; ui_test[i].expected = "240";
						i++; ui_test[i].num = 0x0000000FU; ui_test[i].expected = "15";
						i++; ui_test[i].num = 0xC0000000U; ui_test[i].expected = "3221225472";
						i++; ui_test[i].num = 0x0C000000U; ui_test[i].expected = "201326592";
						i++; ui_test[i].num = 0x00C00000U; ui_test[i].expected = "12582912";
						i++; ui_test[i].num = 0x000C0000U; ui_test[i].expected = "786432";
						i++; ui_test[i].num = 0x0000C000U; ui_test[i].expected = "49152";
						i++; ui_test[i].num = 0x00000C00U; ui_test[i].expected = "3072";
						i++; ui_test[i].num = 0x000000C0U; ui_test[i].expected = "192";
						i++; ui_test[i].num = 0x0000000CU; ui_test[i].expected = "12";
						i++; ui_test[i].num = 0x00000001U; ui_test[i].expected = "1";
						i++; ui_test[i].num = 0x00000000U; ui_test[i].expected = "0";
						num_uint_tests = i;
					}
				//#elif (SIZEOF_INT == 8)
					else if(sizeof(int) == 8) {
						/* !checksrc! disable LONGLINE all */
						i = 1; ui_test[i].num = 0xFFFFFFFFFFFFFFFFU; ui_test[i].expected = "18446744073709551615";
						i++; ui_test[i].num = 0xFFFFFFFF00000000U; ui_test[i].expected = "18446744069414584320";
						i++; ui_test[i].num = 0x00000000FFFFFFFFU; ui_test[i].expected = "4294967295";
						i++; ui_test[i].num = 0xFFFF000000000000U; ui_test[i].expected = "18446462598732840960";
						i++; ui_test[i].num = 0x0000FFFF00000000U; ui_test[i].expected = "281470681743360";
						i++; ui_test[i].num = 0x00000000FFFF0000U; ui_test[i].expected = "4294901760";
						i++; ui_test[i].num = 0x000000000000FFFFU; ui_test[i].expected = "65535";
						i++; ui_test[i].num = 0xFF00000000000000U; ui_test[i].expected = "18374686479671623680";
						i++; ui_test[i].num = 0x00FF000000000000U; ui_test[i].expected = "71776119061217280";
						i++; ui_test[i].num = 0x0000FF0000000000U; ui_test[i].expected = "280375465082880";
						i++; ui_test[i].num = 0x000000FF00000000U; ui_test[i].expected = "1095216660480";
						i++; ui_test[i].num = 0x00000000FF000000U; ui_test[i].expected = "4278190080";
						i++; ui_test[i].num = 0x0000000000FF0000U; ui_test[i].expected = "16711680";
						i++; ui_test[i].num = 0x000000000000FF00U; ui_test[i].expected = "65280";
						i++; ui_test[i].num = 0x00000000000000FFU; ui_test[i].expected = "255";
						i++; ui_test[i].num = 0xF000000000000000U; ui_test[i].expected = "17293822569102704640";
						i++; ui_test[i].num = 0x0F00000000000000U; ui_test[i].expected = "1080863910568919040";
						i++; ui_test[i].num = 0x00F0000000000000U; ui_test[i].expected = "67553994410557440";
						i++; ui_test[i].num = 0x000F000000000000U; ui_test[i].expected = "4222124650659840";
						i++; ui_test[i].num = 0x0000F00000000000U; ui_test[i].expected = "263882790666240";
						i++; ui_test[i].num = 0x00000F0000000000U; ui_test[i].expected = "16492674416640";
						i++; ui_test[i].num = 0x000000F000000000U; ui_test[i].expected = "1030792151040";
						i++; ui_test[i].num = 0x0000000F00000000U; ui_test[i].expected = "64424509440";
						i++; ui_test[i].num = 0x00000000F0000000U; ui_test[i].expected = "4026531840";
						i++; ui_test[i].num = 0x000000000F000000U; ui_test[i].expected = "251658240";
						i++; ui_test[i].num = 0x0000000000F00000U; ui_test[i].expected = "15728640";
						i++; ui_test[i].num = 0x00000000000F0000U; ui_test[i].expected = "983040";
						i++; ui_test[i].num = 0x000000000000F000U; ui_test[i].expected = "61440";
						i++; ui_test[i].num = 0x0000000000000F00U; ui_test[i].expected = "3840";
						i++; ui_test[i].num = 0x00000000000000F0U; ui_test[i].expected = "240";
						i++; ui_test[i].num = 0x000000000000000FU; ui_test[i].expected = "15";
						i++; ui_test[i].num = 0xC000000000000000U; ui_test[i].expected = "13835058055282163712";
						i++; ui_test[i].num = 0x0C00000000000000U; ui_test[i].expected = "864691128455135232";
						i++; ui_test[i].num = 0x00C0000000000000U; ui_test[i].expected = "54043195528445952";
						i++; ui_test[i].num = 0x000C000000000000U; ui_test[i].expected = "3377699720527872";
						i++; ui_test[i].num = 0x0000C00000000000U; ui_test[i].expected = "211106232532992";
						i++; ui_test[i].num = 0x00000C0000000000U; ui_test[i].expected = "13194139533312";
						i++; ui_test[i].num = 0x000000C000000000U; ui_test[i].expected = "824633720832";
						i++; ui_test[i].num = 0x0000000C00000000U; ui_test[i].expected = "51539607552";
						i++; ui_test[i].num = 0x00000000C0000000U; ui_test[i].expected = "3221225472";
						i++; ui_test[i].num = 0x000000000C000000U; ui_test[i].expected = "201326592";
						i++; ui_test[i].num = 0x0000000000C00000U; ui_test[i].expected = "12582912";
						i++; ui_test[i].num = 0x00000000000C0000U; ui_test[i].expected = "786432";
						i++; ui_test[i].num = 0x000000000000C000U; ui_test[i].expected = "49152";
						i++; ui_test[i].num = 0x0000000000000C00U; ui_test[i].expected = "3072";
						i++; ui_test[i].num = 0x00000000000000C0U; ui_test[i].expected = "192";
						i++; ui_test[i].num = 0x000000000000000CU; ui_test[i].expected = "12";
						i++; ui_test[i].num = 0x00000001U; ui_test[i].expected = "1";
						i++; ui_test[i].num = 0x00000000U; ui_test[i].expected = "0";
						num_uint_tests = i;
				//#endif
					}
					for(i = 1; i <= num_uint_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							ui_test[i].result[j] = 'X';
						ui_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(ui_test[i].result, sizeof(ui_test[i].result), "%u", ui_test[i].num);
						if(memcmp(ui_test[i].result,
							ui_test[i].expected,
							strlen(ui_test[i].expected))) {
							printf("unsigned int test #%.2d: Failed (Expected: %s Got: %s)\n", i, ui_test[i].expected, ui_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() unsigned int tests OK!\n");
					else
						printf("Some curl_mprintf() unsigned int tests Failed!\n");
					return failed;
				}
				static int test_signed_int_formatting()
				{
					int i, j;
					int num_sint_tests = 0;
					int failed = 0;
				//#if (SIZEOF_INT == 2)
					if(sizeof(int) == 2) {
						i = 1; si_test[i].num = 0x7FFF; si_test[i].expected = "32767";
						i++; si_test[i].num = 0x7FFE; si_test[i].expected = "32766";
						i++; si_test[i].num = 0x7FFD; si_test[i].expected = "32765";
						i++; si_test[i].num = 0x7F00; si_test[i].expected = "32512";
						i++; si_test[i].num = 0x07F0; si_test[i].expected = "2032";
						i++; si_test[i].num = 0x007F; si_test[i].expected = "127";
						i++; si_test[i].num = 0x7000; si_test[i].expected = "28672";
						i++; si_test[i].num = 0x0700; si_test[i].expected = "1792";
						i++; si_test[i].num = 0x0070; si_test[i].expected = "112";
						i++; si_test[i].num = 0x0007; si_test[i].expected = "7";
						i++; si_test[i].num = 0x5000; si_test[i].expected = "20480";
						i++; si_test[i].num = 0x0500; si_test[i].expected = "1280";
						i++; si_test[i].num = 0x0050; si_test[i].expected = "80";
						i++; si_test[i].num = 0x0005; si_test[i].expected = "5";
						i++; si_test[i].num = 0x0001; si_test[i].expected = "1";
						i++; si_test[i].num = 0x0000; si_test[i].expected = "0";
						i++; si_test[i].num = -0x7FFF -1; si_test[i].expected = "-32768";
						i++; si_test[i].num = -0x7FFE -1; si_test[i].expected = "-32767";
						i++; si_test[i].num = -0x7FFD -1; si_test[i].expected = "-32766";
						i++; si_test[i].num = -0x7F00 -1; si_test[i].expected = "-32513";
						i++; si_test[i].num = -0x07F0 -1; si_test[i].expected = "-2033";
						i++; si_test[i].num = -0x007F -1; si_test[i].expected = "-128";
						i++; si_test[i].num = -0x7000 -1; si_test[i].expected = "-28673";
						i++; si_test[i].num = -0x0700 -1; si_test[i].expected = "-1793";
						i++; si_test[i].num = -0x0070 -1; si_test[i].expected = "-113";
						i++; si_test[i].num = -0x0007 -1; si_test[i].expected = "-8";
						i++; si_test[i].num = -0x5000 -1; si_test[i].expected = "-20481";
						i++; si_test[i].num = -0x0500 -1; si_test[i].expected = "-1281";
						i++; si_test[i].num = -0x0050 -1; si_test[i].expected = "-81";
						i++; si_test[i].num = -0x0005 -1; si_test[i].expected = "-6";
						i++; si_test[i].num =  0x0000 -1; si_test[i].expected = "-1";
						num_sint_tests = i;
					}
				//#elif (SIZEOF_INT == 4)
					else if(sizeof(int) == 4) {
						i = 1; si_test[i].num = 0x7FFFFFFF; si_test[i].expected = "2147483647";
						i++; si_test[i].num = 0x7FFFFFFE; si_test[i].expected = "2147483646";
						i++; si_test[i].num = 0x7FFFFFFD; si_test[i].expected = "2147483645";
						i++; si_test[i].num = 0x7FFF0000; si_test[i].expected = "2147418112";
						i++; si_test[i].num = 0x00007FFF; si_test[i].expected = "32767";
						i++; si_test[i].num = 0x7F000000; si_test[i].expected = "2130706432";
						i++; si_test[i].num = 0x007F0000; si_test[i].expected = "8323072";
						i++; si_test[i].num = 0x00007F00; si_test[i].expected = "32512";
						i++; si_test[i].num = 0x0000007F; si_test[i].expected = "127";
						i++; si_test[i].num = 0x70000000; si_test[i].expected = "1879048192";
						i++; si_test[i].num = 0x07000000; si_test[i].expected = "117440512";
						i++; si_test[i].num = 0x00700000; si_test[i].expected = "7340032";
						i++; si_test[i].num = 0x00070000; si_test[i].expected = "458752";
						i++; si_test[i].num = 0x00007000; si_test[i].expected = "28672";
						i++; si_test[i].num = 0x00000700; si_test[i].expected = "1792";
						i++; si_test[i].num = 0x00000070; si_test[i].expected = "112";
						i++; si_test[i].num = 0x00000007; si_test[i].expected = "7";
						i++; si_test[i].num = 0x50000000; si_test[i].expected = "1342177280";
						i++; si_test[i].num = 0x05000000; si_test[i].expected = "83886080";
						i++; si_test[i].num = 0x00500000; si_test[i].expected = "5242880";
						i++; si_test[i].num = 0x00050000; si_test[i].expected = "327680";
						i++; si_test[i].num = 0x00005000; si_test[i].expected = "20480";
						i++; si_test[i].num = 0x00000500; si_test[i].expected = "1280";
						i++; si_test[i].num = 0x00000050; si_test[i].expected = "80";
						i++; si_test[i].num = 0x00000005; si_test[i].expected = "5";
						i++; si_test[i].num = 0x00000001; si_test[i].expected = "1";
						i++; si_test[i].num = 0x00000000; si_test[i].expected = "0";
						i++; si_test[i].num = -0x7FFFFFFF -1; si_test[i].expected = "-2147483648";
						i++; si_test[i].num = -0x7FFFFFFE -1; si_test[i].expected = "-2147483647";
						i++; si_test[i].num = -0x7FFFFFFD -1; si_test[i].expected = "-2147483646";
						i++; si_test[i].num = -0x7FFF0000 -1; si_test[i].expected = "-2147418113";
						i++; si_test[i].num = -0x00007FFF -1; si_test[i].expected = "-32768";
						i++; si_test[i].num = -0x7F000000 -1; si_test[i].expected = "-2130706433";
						i++; si_test[i].num = -0x007F0000 -1; si_test[i].expected = "-8323073";
						i++; si_test[i].num = -0x00007F00 -1; si_test[i].expected = "-32513";
						i++; si_test[i].num = -0x0000007F -1; si_test[i].expected = "-128";
						i++; si_test[i].num = -0x70000000 -1; si_test[i].expected = "-1879048193";
						i++; si_test[i].num = -0x07000000 -1; si_test[i].expected = "-117440513";
						i++; si_test[i].num = -0x00700000 -1; si_test[i].expected = "-7340033";
						i++; si_test[i].num = -0x00070000 -1; si_test[i].expected = "-458753";
						i++; si_test[i].num = -0x00007000 -1; si_test[i].expected = "-28673";
						i++; si_test[i].num = -0x00000700 -1; si_test[i].expected = "-1793";
						i++; si_test[i].num = -0x00000070 -1; si_test[i].expected = "-113";
						i++; si_test[i].num = -0x00000007 -1; si_test[i].expected = "-8";
						i++; si_test[i].num = -0x50000000 -1; si_test[i].expected = "-1342177281";
						i++; si_test[i].num = -0x05000000 -1; si_test[i].expected = "-83886081";
						i++; si_test[i].num = -0x00500000 -1; si_test[i].expected = "-5242881";
						i++; si_test[i].num = -0x00050000 -1; si_test[i].expected = "-327681";
						i++; si_test[i].num = -0x00005000 -1; si_test[i].expected = "-20481";
						i++; si_test[i].num = -0x00000500 -1; si_test[i].expected = "-1281";
						i++; si_test[i].num = -0x00000050 -1; si_test[i].expected = "-81";
						i++; si_test[i].num = -0x00000005 -1; si_test[i].expected = "-6";
						i++; si_test[i].num =  0x00000000 -1; si_test[i].expected = "-1";
						num_sint_tests = i;
					}
				//#elif (SIZEOF_INT == 8)
					else if(sizeof(int) == 8) {
						i = 1; si_test[i].num = 0x7FFFFFFFFFFFFFFF; si_test[i].expected = "9223372036854775807";
						i++; si_test[i].num = 0x7FFFFFFFFFFFFFFE; si_test[i].expected = "9223372036854775806";
						i++; si_test[i].num = 0x7FFFFFFFFFFFFFFD; si_test[i].expected = "9223372036854775805";
						i++; si_test[i].num = 0x7FFFFFFF00000000; si_test[i].expected = "9223372032559808512";
						i++; si_test[i].num = 0x000000007FFFFFFF; si_test[i].expected = "2147483647";
						i++; si_test[i].num = 0x7FFF000000000000; si_test[i].expected = "9223090561878065152";
						i++; si_test[i].num = 0x00007FFF00000000; si_test[i].expected = "140733193388032";
						i++; si_test[i].num = 0x000000007FFF0000; si_test[i].expected = "2147418112";
						i++; si_test[i].num = 0x0000000000007FFF; si_test[i].expected = "32767";
						i++; si_test[i].num = 0x7F00000000000000; si_test[i].expected = "9151314442816847872";
						i++; si_test[i].num = 0x007F000000000000; si_test[i].expected = "35747322042253312";
						i++; si_test[i].num = 0x00007F0000000000; si_test[i].expected = "139637976727552";
						i++; si_test[i].num = 0x0000007F00000000; si_test[i].expected = "545460846592";
						i++; si_test[i].num = 0x000000007F000000; si_test[i].expected = "2130706432";
						i++; si_test[i].num = 0x00000000007F0000; si_test[i].expected = "8323072";
						i++; si_test[i].num = 0x0000000000007F00; si_test[i].expected = "32512";
						i++; si_test[i].num = 0x000000000000007F; si_test[i].expected = "127";
						i++; si_test[i].num = 0x7000000000000000; si_test[i].expected = "8070450532247928832";
						i++; si_test[i].num = 0x0700000000000000; si_test[i].expected = "504403158265495552";
						i++; si_test[i].num = 0x0070000000000000; si_test[i].expected = "31525197391593472";
						i++; si_test[i].num = 0x0007000000000000; si_test[i].expected = "1970324836974592";
						i++; si_test[i].num = 0x0000700000000000; si_test[i].expected = "123145302310912";
						i++; si_test[i].num = 0x0000070000000000; si_test[i].expected = "7696581394432";
						i++; si_test[i].num = 0x0000007000000000; si_test[i].expected = "481036337152";
						i++; si_test[i].num = 0x0000000700000000; si_test[i].expected = "30064771072";
						i++; si_test[i].num = 0x0000000070000000; si_test[i].expected = "1879048192";
						i++; si_test[i].num = 0x0000000007000000; si_test[i].expected = "117440512";
						i++; si_test[i].num = 0x0000000000700000; si_test[i].expected = "7340032";
						i++; si_test[i].num = 0x0000000000070000; si_test[i].expected = "458752";
						i++; si_test[i].num = 0x0000000000007000; si_test[i].expected = "28672";
						i++; si_test[i].num = 0x0000000000000700; si_test[i].expected = "1792";
						i++; si_test[i].num = 0x0000000000000070; si_test[i].expected = "112";
						i++; si_test[i].num = 0x0000000000000007; si_test[i].expected = "7";
						i++; si_test[i].num = 0x0000000000000001; si_test[i].expected = "1";
						i++; si_test[i].num = 0x0000000000000000; si_test[i].expected = "0";
						i++; si_test[i].num = -0x7FFFFFFFFFFFFFFF -1; si_test[i].expected = "-9223372036854775808";
						i++; si_test[i].num = -0x7FFFFFFFFFFFFFFE -1; si_test[i].expected = "-9223372036854775807";
						i++; si_test[i].num = -0x7FFFFFFFFFFFFFFD -1; si_test[i].expected = "-9223372036854775806";
						i++; si_test[i].num = -0x7FFFFFFF00000000 -1; si_test[i].expected = "-9223372032559808513";
						i++; si_test[i].num = -0x000000007FFFFFFF -1; si_test[i].expected = "-2147483648";
						i++; si_test[i].num = -0x7FFF000000000000 -1; si_test[i].expected = "-9223090561878065153";
						i++; si_test[i].num = -0x00007FFF00000000 -1; si_test[i].expected = "-140733193388033";
						i++; si_test[i].num = -0x000000007FFF0000 -1; si_test[i].expected = "-2147418113";
						i++; si_test[i].num = -0x0000000000007FFF -1; si_test[i].expected = "-32768";
						i++; si_test[i].num = -0x7F00000000000000 -1; si_test[i].expected = "-9151314442816847873";
						i++; si_test[i].num = -0x007F000000000000 -1; si_test[i].expected = "-35747322042253313";
						i++; si_test[i].num = -0x00007F0000000000 -1; si_test[i].expected = "-139637976727553";
						i++; si_test[i].num = -0x0000007F00000000 -1; si_test[i].expected = "-545460846593";
						i++; si_test[i].num = -0x000000007F000000 -1; si_test[i].expected = "-2130706433";
						i++; si_test[i].num = -0x00000000007F0000 -1; si_test[i].expected = "-8323073";
						i++; si_test[i].num = -0x0000000000007F00 -1; si_test[i].expected = "-32513";
						i++; si_test[i].num = -0x000000000000007F -1; si_test[i].expected = "-128";
						i++; si_test[i].num = -0x7000000000000000 -1; si_test[i].expected = "-8070450532247928833";
						i++; si_test[i].num = -0x0700000000000000 -1; si_test[i].expected = "-504403158265495553";
						i++; si_test[i].num = -0x0070000000000000 -1; si_test[i].expected = "-31525197391593473";
						i++; si_test[i].num = -0x0007000000000000 -1; si_test[i].expected = "-1970324836974593";
						i++; si_test[i].num = -0x0000700000000000 -1; si_test[i].expected = "-123145302310913";
						i++; si_test[i].num = -0x0000070000000000 -1; si_test[i].expected = "-7696581394433";
						i++; si_test[i].num = -0x0000007000000000 -1; si_test[i].expected = "-481036337153";
						i++; si_test[i].num = -0x0000000700000000 -1; si_test[i].expected = "-30064771073";
						i++; si_test[i].num = -0x0000000070000000 -1; si_test[i].expected = "-1879048193";
						i++; si_test[i].num = -0x0000000007000000 -1; si_test[i].expected = "-117440513";
						i++; si_test[i].num = -0x0000000000700000 -1; si_test[i].expected = "-7340033";
						i++; si_test[i].num = -0x0000000000070000 -1; si_test[i].expected = "-458753";
						i++; si_test[i].num = -0x0000000000007000 -1; si_test[i].expected = "-28673";
						i++; si_test[i].num = -0x0000000000000700 -1; si_test[i].expected = "-1793";
						i++; si_test[i].num = -0x0000000000000070 -1; si_test[i].expected = "-113";
						i++; si_test[i].num = -0x0000000000000007 -1; si_test[i].expected = "-8";
						i++; si_test[i].num =  0x0000000000000000 -1; si_test[i].expected = "-1";
						num_sint_tests = i;
				//#endif
					}
					for(i = 1; i <= num_sint_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							si_test[i].result[j] = 'X';
						si_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(si_test[i].result, sizeof(si_test[i].result), "%d", si_test[i].num);
						if(memcmp(si_test[i].result, si_test[i].expected, strlen(si_test[i].expected))) {
							printf("signed int test #%.2d: Failed (Expected: %s Got: %s)\n", i, si_test[i].expected, si_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() signed int tests OK!\n");
					else
						printf("Some curl_mprintf() signed int tests Failed!\n");
					return failed;
				}
				static int test_unsigned_long_formatting()
				{
					int i, j;
					int num_ulong_tests = 0;
					int failed = 0;
				//#if (SIZEOF_LONG == 2)
					if(sizeof(long) == 2) {
						i = 1; ul_test[i].num = 0xFFFFUL; ul_test[i].expected = "65535";
						i++; ul_test[i].num = 0xFF00UL; ul_test[i].expected = "65280";
						i++; ul_test[i].num = 0x00FFUL; ul_test[i].expected = "255";
						i++; ul_test[i].num = 0xF000UL; ul_test[i].expected = "61440";
						i++; ul_test[i].num = 0x0F00UL; ul_test[i].expected = "3840";
						i++; ul_test[i].num = 0x00F0UL; ul_test[i].expected = "240";
						i++; ul_test[i].num = 0x000FUL; ul_test[i].expected = "15";
						i++; ul_test[i].num = 0xC000UL; ul_test[i].expected = "49152";
						i++; ul_test[i].num = 0x0C00UL; ul_test[i].expected = "3072";
						i++; ul_test[i].num = 0x00C0UL; ul_test[i].expected = "192";
						i++; ul_test[i].num = 0x000CUL; ul_test[i].expected = "12";
						i++; ul_test[i].num = 0x0001UL; ul_test[i].expected = "1";
						i++; ul_test[i].num = 0x0000UL; ul_test[i].expected = "0";
						num_ulong_tests = i;
					}
				//#elif (SIZEOF_LONG == 4)
					else if(sizeof(long) == 4) {
						i = 1; ul_test[i].num = 0xFFFFFFFFUL; ul_test[i].expected = "4294967295";
						i++; ul_test[i].num = 0xFFFF0000UL; ul_test[i].expected = "4294901760";
						i++; ul_test[i].num = 0x0000FFFFUL; ul_test[i].expected = "65535";
						i++; ul_test[i].num = 0xFF000000UL; ul_test[i].expected = "4278190080";
						i++; ul_test[i].num = 0x00FF0000UL; ul_test[i].expected = "16711680";
						i++; ul_test[i].num = 0x0000FF00UL; ul_test[i].expected = "65280";
						i++; ul_test[i].num = 0x000000FFUL; ul_test[i].expected = "255";
						i++; ul_test[i].num = 0xF0000000UL; ul_test[i].expected = "4026531840";
						i++; ul_test[i].num = 0x0F000000UL; ul_test[i].expected = "251658240";
						i++; ul_test[i].num = 0x00F00000UL; ul_test[i].expected = "15728640";
						i++; ul_test[i].num = 0x000F0000UL; ul_test[i].expected = "983040";
						i++; ul_test[i].num = 0x0000F000UL; ul_test[i].expected = "61440";
						i++; ul_test[i].num = 0x00000F00UL; ul_test[i].expected = "3840";
						i++; ul_test[i].num = 0x000000F0UL; ul_test[i].expected = "240";
						i++; ul_test[i].num = 0x0000000FUL; ul_test[i].expected = "15";
						i++; ul_test[i].num = 0xC0000000UL; ul_test[i].expected = "3221225472";
						i++; ul_test[i].num = 0x0C000000UL; ul_test[i].expected = "201326592";
						i++; ul_test[i].num = 0x00C00000UL; ul_test[i].expected = "12582912";
						i++; ul_test[i].num = 0x000C0000UL; ul_test[i].expected = "786432";
						i++; ul_test[i].num = 0x0000C000UL; ul_test[i].expected = "49152";
						i++; ul_test[i].num = 0x00000C00UL; ul_test[i].expected = "3072";
						i++; ul_test[i].num = 0x000000C0UL; ul_test[i].expected = "192";
						i++; ul_test[i].num = 0x0000000CUL; ul_test[i].expected = "12";
						i++; ul_test[i].num = 0x00000001UL; ul_test[i].expected = "1";
						i++; ul_test[i].num = 0x00000000UL; ul_test[i].expected = "0";
						num_ulong_tests = i;
					}
				//#elif (SIZEOF_LONG == 8)
					else if(sizeof(long) == 8) {
						i = 1; ul_test[i].num = 0xFFFFFFFFFFFFFFFFUL; ul_test[i].expected = "18446744073709551615";
						i++; ul_test[i].num = 0xFFFFFFFF00000000UL; ul_test[i].expected = "18446744069414584320";
						i++; ul_test[i].num = 0x00000000FFFFFFFFUL; ul_test[i].expected = "4294967295";
						i++; ul_test[i].num = 0xFFFF000000000000UL; ul_test[i].expected = "18446462598732840960";
						i++; ul_test[i].num = 0x0000FFFF00000000UL; ul_test[i].expected = "281470681743360";
						i++; ul_test[i].num = 0x00000000FFFF0000UL; ul_test[i].expected = "4294901760";
						i++; ul_test[i].num = 0x000000000000FFFFUL; ul_test[i].expected = "65535";
						i++; ul_test[i].num = 0xFF00000000000000UL; ul_test[i].expected = "18374686479671623680";
						i++; ul_test[i].num = 0x00FF000000000000UL; ul_test[i].expected = "71776119061217280";
						i++; ul_test[i].num = 0x0000FF0000000000UL; ul_test[i].expected = "280375465082880";
						i++; ul_test[i].num = 0x000000FF00000000UL; ul_test[i].expected = "1095216660480";
						i++; ul_test[i].num = 0x00000000FF000000UL; ul_test[i].expected = "4278190080";
						i++; ul_test[i].num = 0x0000000000FF0000UL; ul_test[i].expected = "16711680";
						i++; ul_test[i].num = 0x000000000000FF00UL; ul_test[i].expected = "65280";
						i++; ul_test[i].num = 0x00000000000000FFUL; ul_test[i].expected = "255";
						i++; ul_test[i].num = 0xF000000000000000UL; ul_test[i].expected = "17293822569102704640";
						i++; ul_test[i].num = 0x0F00000000000000UL; ul_test[i].expected = "1080863910568919040";
						i++; ul_test[i].num = 0x00F0000000000000UL; ul_test[i].expected = "67553994410557440";
						i++; ul_test[i].num = 0x000F000000000000UL; ul_test[i].expected = "4222124650659840";
						i++; ul_test[i].num = 0x0000F00000000000UL; ul_test[i].expected = "263882790666240";
						i++; ul_test[i].num = 0x00000F0000000000UL; ul_test[i].expected = "16492674416640";
						i++; ul_test[i].num = 0x000000F000000000UL; ul_test[i].expected = "1030792151040";
						i++; ul_test[i].num = 0x0000000F00000000UL; ul_test[i].expected = "64424509440";
						i++; ul_test[i].num = 0x00000000F0000000UL; ul_test[i].expected = "4026531840";
						i++; ul_test[i].num = 0x000000000F000000UL; ul_test[i].expected = "251658240";
						i++; ul_test[i].num = 0x0000000000F00000UL; ul_test[i].expected = "15728640";
						i++; ul_test[i].num = 0x00000000000F0000UL; ul_test[i].expected = "983040";
						i++; ul_test[i].num = 0x000000000000F000UL; ul_test[i].expected = "61440";
						i++; ul_test[i].num = 0x0000000000000F00UL; ul_test[i].expected = "3840";
						i++; ul_test[i].num = 0x00000000000000F0UL; ul_test[i].expected = "240";
						i++; ul_test[i].num = 0x000000000000000FUL; ul_test[i].expected = "15";
						i++; ul_test[i].num = 0xC000000000000000UL; ul_test[i].expected = "13835058055282163712";
						i++; ul_test[i].num = 0x0C00000000000000UL; ul_test[i].expected = "864691128455135232";
						i++; ul_test[i].num = 0x00C0000000000000UL; ul_test[i].expected = "54043195528445952";
						i++; ul_test[i].num = 0x000C000000000000UL; ul_test[i].expected = "3377699720527872";
						i++; ul_test[i].num = 0x0000C00000000000UL; ul_test[i].expected = "211106232532992";
						i++; ul_test[i].num = 0x00000C0000000000UL; ul_test[i].expected = "13194139533312";
						i++; ul_test[i].num = 0x000000C000000000UL; ul_test[i].expected = "824633720832";
						i++; ul_test[i].num = 0x0000000C00000000UL; ul_test[i].expected = "51539607552";
						i++; ul_test[i].num = 0x00000000C0000000UL; ul_test[i].expected = "3221225472";
						i++; ul_test[i].num = 0x000000000C000000UL; ul_test[i].expected = "201326592";
						i++; ul_test[i].num = 0x0000000000C00000UL; ul_test[i].expected = "12582912";
						i++; ul_test[i].num = 0x00000000000C0000UL; ul_test[i].expected = "786432";
						i++; ul_test[i].num = 0x000000000000C000UL; ul_test[i].expected = "49152";
						i++; ul_test[i].num = 0x0000000000000C00UL; ul_test[i].expected = "3072";
						i++; ul_test[i].num = 0x00000000000000C0UL; ul_test[i].expected = "192";
						i++; ul_test[i].num = 0x000000000000000CUL; ul_test[i].expected = "12";
						i++; ul_test[i].num = 0x00000001UL; ul_test[i].expected = "1";
						i++; ul_test[i].num = 0x00000000UL; ul_test[i].expected = "0";
						num_ulong_tests = i;
					}
				//#endif
					for(i = 1; i <= num_ulong_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							ul_test[i].result[j] = 'X';
						ul_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(ul_test[i].result, sizeof(ul_test[i].result), "%lu", ul_test[i].num);
						if(memcmp(ul_test[i].result, ul_test[i].expected, strlen(ul_test[i].expected))) {
							printf("unsigned long test #%.2d: Failed (Expected: %s Got: %s)\n", i, ul_test[i].expected, ul_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() unsigned long tests OK!\n");
					else
						printf("Some curl_mprintf() unsigned long tests Failed!\n");
					return failed;
				}
				static int test_signed_long_formatting()
				{
					int i, j;
					int num_slong_tests = 0;
					int failed = 0;
				//#if (SIZEOF_LONG == 2)
					if(sizeof(long) == 2) {
						i = 1; sl_test[i].num = 0x7FFFL; sl_test[i].expected = "32767";
						i++; sl_test[i].num = 0x7FFEL; sl_test[i].expected = "32766";
						i++; sl_test[i].num = 0x7FFDL; sl_test[i].expected = "32765";
						i++; sl_test[i].num = 0x7F00L; sl_test[i].expected = "32512";
						i++; sl_test[i].num = 0x07F0L; sl_test[i].expected = "2032";
						i++; sl_test[i].num = 0x007FL; sl_test[i].expected = "127";
						i++; sl_test[i].num = 0x7000L; sl_test[i].expected = "28672";
						i++; sl_test[i].num = 0x0700L; sl_test[i].expected = "1792";
						i++; sl_test[i].num = 0x0070L; sl_test[i].expected = "112";
						i++; sl_test[i].num = 0x0007L; sl_test[i].expected = "7";
						i++; sl_test[i].num = 0x5000L; sl_test[i].expected = "20480";
						i++; sl_test[i].num = 0x0500L; sl_test[i].expected = "1280";
						i++; sl_test[i].num = 0x0050L; sl_test[i].expected = "80";
						i++; sl_test[i].num = 0x0005L; sl_test[i].expected = "5";
						i++; sl_test[i].num = 0x0001L; sl_test[i].expected = "1";
						i++; sl_test[i].num = 0x0000L; sl_test[i].expected = "0";
						i++; sl_test[i].num = -0x7FFFL -1L; sl_test[i].expected = "-32768";
						i++; sl_test[i].num = -0x7FFEL -1L; sl_test[i].expected = "-32767";
						i++; sl_test[i].num = -0x7FFDL -1L; sl_test[i].expected = "-32766";
						i++; sl_test[i].num = -0x7F00L -1L; sl_test[i].expected = "-32513";
						i++; sl_test[i].num = -0x07F0L -1L; sl_test[i].expected = "-2033";
						i++; sl_test[i].num = -0x007FL -1L; sl_test[i].expected = "-128";
						i++; sl_test[i].num = -0x7000L -1L; sl_test[i].expected = "-28673";
						i++; sl_test[i].num = -0x0700L -1L; sl_test[i].expected = "-1793";
						i++; sl_test[i].num = -0x0070L -1L; sl_test[i].expected = "-113";
						i++; sl_test[i].num = -0x0007L -1L; sl_test[i].expected = "-8";
						i++; sl_test[i].num = -0x5000L -1L; sl_test[i].expected = "-20481";
						i++; sl_test[i].num = -0x0500L -1L; sl_test[i].expected = "-1281";
						i++; sl_test[i].num = -0x0050L -1L; sl_test[i].expected = "-81";
						i++; sl_test[i].num = -0x0005L -1L; sl_test[i].expected = "-6";
						i++; sl_test[i].num =  0x0000L -1L; sl_test[i].expected = "-1";
						num_slong_tests = i;
					}
				//#elif (SIZEOF_LONG == 4)
					else if(sizeof(long) == 4) {
						i = 1; sl_test[i].num = 0x7FFFFFFFL; sl_test[i].expected = "2147483647";
						i++; sl_test[i].num = 0x7FFFFFFEL; sl_test[i].expected = "2147483646";
						i++; sl_test[i].num = 0x7FFFFFFDL; sl_test[i].expected = "2147483645";
						i++; sl_test[i].num = 0x7FFF0000L; sl_test[i].expected = "2147418112";
						i++; sl_test[i].num = 0x00007FFFL; sl_test[i].expected = "32767";
						i++; sl_test[i].num = 0x7F000000L; sl_test[i].expected = "2130706432";
						i++; sl_test[i].num = 0x007F0000L; sl_test[i].expected = "8323072";
						i++; sl_test[i].num = 0x00007F00L; sl_test[i].expected = "32512";
						i++; sl_test[i].num = 0x0000007FL; sl_test[i].expected = "127";
						i++; sl_test[i].num = 0x70000000L; sl_test[i].expected = "1879048192";
						i++; sl_test[i].num = 0x07000000L; sl_test[i].expected = "117440512";
						i++; sl_test[i].num = 0x00700000L; sl_test[i].expected = "7340032";
						i++; sl_test[i].num = 0x00070000L; sl_test[i].expected = "458752";
						i++; sl_test[i].num = 0x00007000L; sl_test[i].expected = "28672";
						i++; sl_test[i].num = 0x00000700L; sl_test[i].expected = "1792";
						i++; sl_test[i].num = 0x00000070L; sl_test[i].expected = "112";
						i++; sl_test[i].num = 0x00000007L; sl_test[i].expected = "7";
						i++; sl_test[i].num = 0x50000000L; sl_test[i].expected = "1342177280";
						i++; sl_test[i].num = 0x05000000L; sl_test[i].expected = "83886080";
						i++; sl_test[i].num = 0x00500000L; sl_test[i].expected = "5242880";
						i++; sl_test[i].num = 0x00050000L; sl_test[i].expected = "327680";
						i++; sl_test[i].num = 0x00005000L; sl_test[i].expected = "20480";
						i++; sl_test[i].num = 0x00000500L; sl_test[i].expected = "1280";
						i++; sl_test[i].num = 0x00000050L; sl_test[i].expected = "80";
						i++; sl_test[i].num = 0x00000005L; sl_test[i].expected = "5";
						i++; sl_test[i].num = 0x00000001L; sl_test[i].expected = "1";
						i++; sl_test[i].num = 0x00000000L; sl_test[i].expected = "0";
						i++; sl_test[i].num = -0x7FFFFFFFL -1L; sl_test[i].expected = "-2147483648";
						i++; sl_test[i].num = -0x7FFFFFFEL -1L; sl_test[i].expected = "-2147483647";
						i++; sl_test[i].num = -0x7FFFFFFDL -1L; sl_test[i].expected = "-2147483646";
						i++; sl_test[i].num = -0x7FFF0000L -1L; sl_test[i].expected = "-2147418113";
						i++; sl_test[i].num = -0x00007FFFL -1L; sl_test[i].expected = "-32768";
						i++; sl_test[i].num = -0x7F000000L -1L; sl_test[i].expected = "-2130706433";
						i++; sl_test[i].num = -0x007F0000L -1L; sl_test[i].expected = "-8323073";
						i++; sl_test[i].num = -0x00007F00L -1L; sl_test[i].expected = "-32513";
						i++; sl_test[i].num = -0x0000007FL -1L; sl_test[i].expected = "-128";
						i++; sl_test[i].num = -0x70000000L -1L; sl_test[i].expected = "-1879048193";
						i++; sl_test[i].num = -0x07000000L -1L; sl_test[i].expected = "-117440513";
						i++; sl_test[i].num = -0x00700000L -1L; sl_test[i].expected = "-7340033";
						i++; sl_test[i].num = -0x00070000L -1L; sl_test[i].expected = "-458753";
						i++; sl_test[i].num = -0x00007000L -1L; sl_test[i].expected = "-28673";
						i++; sl_test[i].num = -0x00000700L -1L; sl_test[i].expected = "-1793";
						i++; sl_test[i].num = -0x00000070L -1L; sl_test[i].expected = "-113";
						i++; sl_test[i].num = -0x00000007L -1L; sl_test[i].expected = "-8";
						i++; sl_test[i].num = -0x50000000L -1L; sl_test[i].expected = "-1342177281";
						i++; sl_test[i].num = -0x05000000L -1L; sl_test[i].expected = "-83886081";
						i++; sl_test[i].num = -0x00500000L -1L; sl_test[i].expected = "-5242881";
						i++; sl_test[i].num = -0x00050000L -1L; sl_test[i].expected = "-327681";
						i++; sl_test[i].num = -0x00005000L -1L; sl_test[i].expected = "-20481";
						i++; sl_test[i].num = -0x00000500L -1L; sl_test[i].expected = "-1281";
						i++; sl_test[i].num = -0x00000050L -1L; sl_test[i].expected = "-81";
						i++; sl_test[i].num = -0x00000005L -1L; sl_test[i].expected = "-6";
						i++; sl_test[i].num =  0x00000000L -1L; sl_test[i].expected = "-1";
						num_slong_tests = i;
					}
				//#elif (SIZEOF_LONG == 8)
					else if(sizeof(long) == 8) {
						i = 1; sl_test[i].num = 0x7FFFFFFFFFFFFFFFL; sl_test[i].expected = "9223372036854775807";
						i++; sl_test[i].num = 0x7FFFFFFFFFFFFFFEL; sl_test[i].expected = "9223372036854775806";
						i++; sl_test[i].num = 0x7FFFFFFFFFFFFFFDL; sl_test[i].expected = "9223372036854775805";
						i++; sl_test[i].num = 0x7FFFFFFF00000000L; sl_test[i].expected = "9223372032559808512";
						i++; sl_test[i].num = 0x000000007FFFFFFFL; sl_test[i].expected = "2147483647";
						i++; sl_test[i].num = 0x7FFF000000000000L; sl_test[i].expected = "9223090561878065152";
						i++; sl_test[i].num = 0x00007FFF00000000L; sl_test[i].expected = "140733193388032";
						i++; sl_test[i].num = 0x000000007FFF0000L; sl_test[i].expected = "2147418112";
						i++; sl_test[i].num = 0x0000000000007FFFL; sl_test[i].expected = "32767";
						i++; sl_test[i].num = 0x7F00000000000000L; sl_test[i].expected = "9151314442816847872";
						i++; sl_test[i].num = 0x007F000000000000L; sl_test[i].expected = "35747322042253312";
						i++; sl_test[i].num = 0x00007F0000000000L; sl_test[i].expected = "139637976727552";
						i++; sl_test[i].num = 0x0000007F00000000L; sl_test[i].expected = "545460846592";
						i++; sl_test[i].num = 0x000000007F000000L; sl_test[i].expected = "2130706432";
						i++; sl_test[i].num = 0x00000000007F0000L; sl_test[i].expected = "8323072";
						i++; sl_test[i].num = 0x0000000000007F00L; sl_test[i].expected = "32512";
						i++; sl_test[i].num = 0x000000000000007FL; sl_test[i].expected = "127";
						i++; sl_test[i].num = 0x7000000000000000L; sl_test[i].expected = "8070450532247928832";
						i++; sl_test[i].num = 0x0700000000000000L; sl_test[i].expected = "504403158265495552";
						i++; sl_test[i].num = 0x0070000000000000L; sl_test[i].expected = "31525197391593472";
						i++; sl_test[i].num = 0x0007000000000000L; sl_test[i].expected = "1970324836974592";
						i++; sl_test[i].num = 0x0000700000000000L; sl_test[i].expected = "123145302310912";
						i++; sl_test[i].num = 0x0000070000000000L; sl_test[i].expected = "7696581394432";
						i++; sl_test[i].num = 0x0000007000000000L; sl_test[i].expected = "481036337152";
						i++; sl_test[i].num = 0x0000000700000000L; sl_test[i].expected = "30064771072";
						i++; sl_test[i].num = 0x0000000070000000L; sl_test[i].expected = "1879048192";
						i++; sl_test[i].num = 0x0000000007000000L; sl_test[i].expected = "117440512";
						i++; sl_test[i].num = 0x0000000000700000L; sl_test[i].expected = "7340032";
						i++; sl_test[i].num = 0x0000000000070000L; sl_test[i].expected = "458752";
						i++; sl_test[i].num = 0x0000000000007000L; sl_test[i].expected = "28672";
						i++; sl_test[i].num = 0x0000000000000700L; sl_test[i].expected = "1792";
						i++; sl_test[i].num = 0x0000000000000070L; sl_test[i].expected = "112";
						i++; sl_test[i].num = 0x0000000000000007L; sl_test[i].expected = "7";
						i++; sl_test[i].num = 0x0000000000000001L; sl_test[i].expected = "1";
						i++; sl_test[i].num = 0x0000000000000000L; sl_test[i].expected = "0";
						i++; sl_test[i].num = -0x7FFFFFFFFFFFFFFFL -1L; sl_test[i].expected = "-9223372036854775808";
						i++; sl_test[i].num = -0x7FFFFFFFFFFFFFFEL -1L; sl_test[i].expected = "-9223372036854775807";
						i++; sl_test[i].num = -0x7FFFFFFFFFFFFFFDL -1L; sl_test[i].expected = "-9223372036854775806";
						i++; sl_test[i].num = -0x7FFFFFFF00000000L -1L; sl_test[i].expected = "-9223372032559808513";
						i++; sl_test[i].num = -0x000000007FFFFFFFL -1L; sl_test[i].expected = "-2147483648";
						i++; sl_test[i].num = -0x7FFF000000000000L -1L; sl_test[i].expected = "-9223090561878065153";
						i++; sl_test[i].num = -0x00007FFF00000000L -1L; sl_test[i].expected = "-140733193388033";
						i++; sl_test[i].num = -0x000000007FFF0000L -1L; sl_test[i].expected = "-2147418113";
						i++; sl_test[i].num = -0x0000000000007FFFL -1L; sl_test[i].expected = "-32768";
						i++; sl_test[i].num = -0x7F00000000000000L -1L; sl_test[i].expected = "-9151314442816847873";
						i++; sl_test[i].num = -0x007F000000000000L -1L; sl_test[i].expected = "-35747322042253313";
						i++; sl_test[i].num = -0x00007F0000000000L -1L; sl_test[i].expected = "-139637976727553";
						i++; sl_test[i].num = -0x0000007F00000000L -1L; sl_test[i].expected = "-545460846593";
						i++; sl_test[i].num = -0x000000007F000000L -1L; sl_test[i].expected = "-2130706433";
						i++; sl_test[i].num = -0x00000000007F0000L -1L; sl_test[i].expected = "-8323073";
						i++; sl_test[i].num = -0x0000000000007F00L -1L; sl_test[i].expected = "-32513";
						i++; sl_test[i].num = -0x000000000000007FL -1L; sl_test[i].expected = "-128";
						i++; sl_test[i].num = -0x7000000000000000L -1L; sl_test[i].expected = "-8070450532247928833";
						i++; sl_test[i].num = -0x0700000000000000L -1L; sl_test[i].expected = "-504403158265495553";
						i++; sl_test[i].num = -0x0070000000000000L -1L; sl_test[i].expected = "-31525197391593473";
						i++; sl_test[i].num = -0x0007000000000000L -1L; sl_test[i].expected = "-1970324836974593";
						i++; sl_test[i].num = -0x0000700000000000L -1L; sl_test[i].expected = "-123145302310913";
						i++; sl_test[i].num = -0x0000070000000000L -1L; sl_test[i].expected = "-7696581394433";
						i++; sl_test[i].num = -0x0000007000000000L -1L; sl_test[i].expected = "-481036337153";
						i++; sl_test[i].num = -0x0000000700000000L -1L; sl_test[i].expected = "-30064771073";
						i++; sl_test[i].num = -0x0000000070000000L -1L; sl_test[i].expected = "-1879048193";
						i++; sl_test[i].num = -0x0000000007000000L -1L; sl_test[i].expected = "-117440513";
						i++; sl_test[i].num = -0x0000000000700000L -1L; sl_test[i].expected = "-7340033";
						i++; sl_test[i].num = -0x0000000000070000L -1L; sl_test[i].expected = "-458753";
						i++; sl_test[i].num = -0x0000000000007000L -1L; sl_test[i].expected = "-28673";
						i++; sl_test[i].num = -0x0000000000000700L -1L; sl_test[i].expected = "-1793";
						i++; sl_test[i].num = -0x0000000000000070L -1L; sl_test[i].expected = "-113";
						i++; sl_test[i].num = -0x0000000000000007L -1L; sl_test[i].expected = "-8";
						i++; sl_test[i].num =  0x0000000000000000L -1L; sl_test[i].expected = "-1";
						num_slong_tests = i;
					}
				//#endif
					for(i = 1; i <= num_slong_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							sl_test[i].result[j] = 'X';
						sl_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(sl_test[i].result, sizeof(sl_test[i].result), "%ld", sl_test[i].num);
						if(memcmp(sl_test[i].result, sl_test[i].expected, strlen(sl_test[i].expected))) {
							printf("signed long test #%.2d: Failed (Expected: %s Got: %s)\n", i, sl_test[i].expected, sl_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() signed long tests OK!\n");
					else
						printf("Some curl_mprintf() signed long tests Failed!\n");
					return failed;
				}
				static int test_curl_off_t_formatting()
				{
					int i, j;
					int num_cofft_tests = 0;
					int failed = 0;
				//#if (SIZEOF_CURL_OFF_T == 2)
					if(sizeof(off_t) == 2) {
						i = 1; co_test[i].num = MPRNT_OFF_T_C(0x7FFF); co_test[i].expected = "32767";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFE); co_test[i].expected = "32766";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFD); co_test[i].expected = "32765";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7F00); co_test[i].expected = "32512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x07F0); co_test[i].expected = "2032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x007F); co_test[i].expected = "127";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7000); co_test[i].expected = "28672";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0700); co_test[i].expected = "1792";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0070); co_test[i].expected = "112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0007); co_test[i].expected = "7";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x5000); co_test[i].expected = "20480";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0500); co_test[i].expected = "1280";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0050); co_test[i].expected = "80";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0005); co_test[i].expected = "5";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0001); co_test[i].expected = "1";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000); co_test[i].expected = "0";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32768";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFE) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32767";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFD) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32766";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7F00) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x07F0) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x007F) -MPRNT_OFF_T_C(1); co_test[i].expected = "-128";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-28673";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0700) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1793";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0070) -MPRNT_OFF_T_C(1); co_test[i].expected = "-113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0007) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x5000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-20481";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0500) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1281";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0050) -MPRNT_OFF_T_C(1); co_test[i].expected = "-81";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0005) -MPRNT_OFF_T_C(1); co_test[i].expected = "-6";
						i++; co_test[i].num =  MPRNT_OFF_T_C(0x0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1";
						num_cofft_tests = i;
					}
				//#elif (SIZEOF_CURL_OFF_T == 4)
					else if(sizeof(off_t) == 4) {
						i = 1; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFF); co_test[i].expected = "2147483647";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFE); co_test[i].expected = "2147483646";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFD); co_test[i].expected = "2147483645";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFF0000); co_test[i].expected = "2147418112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007FFF); co_test[i].expected = "32767";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7F000000); co_test[i].expected = "2130706432";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x007F0000); co_test[i].expected = "8323072";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007F00); co_test[i].expected = "32512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000007F); co_test[i].expected = "127";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x70000000); co_test[i].expected = "1879048192";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x07000000); co_test[i].expected = "117440512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00700000); co_test[i].expected = "7340032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00070000); co_test[i].expected = "458752";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007000); co_test[i].expected = "28672";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000700); co_test[i].expected = "1792";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000070); co_test[i].expected = "112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000007); co_test[i].expected = "7";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x50000000); co_test[i].expected = "1342177280";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x05000000); co_test[i].expected = "83886080";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00500000); co_test[i].expected = "5242880";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00050000); co_test[i].expected = "327680";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00005000); co_test[i].expected = "20480";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000500); co_test[i].expected = "1280";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000050); co_test[i].expected = "80";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000005); co_test[i].expected = "5";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000001); co_test[i].expected = "1";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000000); co_test[i].expected = "0";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483648";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFE) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483647";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFD) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483646";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFF0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147418113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007FFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32768";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7F000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2130706433";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x007F0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8323073";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007F00) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000007F) -MPRNT_OFF_T_C(1); co_test[i].expected = "-128";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x70000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1879048193";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x07000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-117440513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00700000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-7340033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00070000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-458753";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-28673";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000700) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1793";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000070) -MPRNT_OFF_T_C(1); co_test[i].expected = "-113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000007) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x50000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1342177281";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x05000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-83886081";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00500000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-5242881";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00050000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-327681";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00005000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-20481";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000500) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1281";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000050) -MPRNT_OFF_T_C(1); co_test[i].expected = "-81";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000005) -MPRNT_OFF_T_C(1); co_test[i].expected = "-6";
						i++; co_test[i].num =  MPRNT_OFF_T_C(0x00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1";
						num_cofft_tests = i;
					}
				//#elif (SIZEOF_CURL_OFF_T == 8)
					else if(sizeof(off_t) == 8) {
						i = 1; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFF); co_test[i].expected = "9223372036854775807";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFE); co_test[i].expected = "9223372036854775806";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFD); co_test[i].expected = "9223372036854775805";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFFFFFF00000000); co_test[i].expected = "9223372032559808512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000007FFFFFFF); co_test[i].expected = "2147483647";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7FFF000000000000); co_test[i].expected = "9223090561878065152";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007FFF00000000); co_test[i].expected = "140733193388032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000007FFF0000); co_test[i].expected = "2147418112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000007FFF); co_test[i].expected = "32767";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7F00000000000000); co_test[i].expected = "9151314442816847872";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x007F000000000000); co_test[i].expected = "35747322042253312";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00007F0000000000); co_test[i].expected = "139637976727552";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000007F00000000); co_test[i].expected = "545460846592";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000007F000000); co_test[i].expected = "2130706432";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x00000000007F0000); co_test[i].expected = "8323072";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000007F00); co_test[i].expected = "32512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x000000000000007F); co_test[i].expected = "127";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x7000000000000000); co_test[i].expected = "8070450532247928832";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0700000000000000); co_test[i].expected = "504403158265495552";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0070000000000000); co_test[i].expected = "31525197391593472";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0007000000000000); co_test[i].expected = "1970324836974592";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000700000000000); co_test[i].expected = "123145302310912";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000070000000000); co_test[i].expected = "7696581394432";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000007000000000); co_test[i].expected = "481036337152";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000700000000); co_test[i].expected = "30064771072";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000070000000); co_test[i].expected = "1879048192";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000007000000); co_test[i].expected = "117440512";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000700000); co_test[i].expected = "7340032";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000070000); co_test[i].expected = "458752";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000007000); co_test[i].expected = "28672";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000700); co_test[i].expected = "1792";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000070); co_test[i].expected = "112";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000007); co_test[i].expected = "7";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000001); co_test[i].expected = "1";
						i++; co_test[i].num = MPRNT_OFF_T_C(0x0000000000000000); co_test[i].expected = "0";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372036854775808";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFE) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372036854775807";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFFFFFFFFFD) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372036854775806";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFFFFFF00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223372032559808513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000007FFFFFFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147483648";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7FFF000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9223090561878065153";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007FFF00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-140733193388033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000007FFF0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2147418113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000007FFF) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32768";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7F00000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-9151314442816847873";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x007F000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-35747322042253313";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00007F0000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-139637976727553";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000007F00000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-545460846593";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000007F000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-2130706433";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x00000000007F0000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8323073";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000007F00) -MPRNT_OFF_T_C(1); co_test[i].expected = "-32513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x000000000000007F) -MPRNT_OFF_T_C(1); co_test[i].expected = "-128";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x7000000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8070450532247928833";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0700000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-504403158265495553";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0070000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-31525197391593473";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0007000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1970324836974593";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000700000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-123145302310913";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000070000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-7696581394433";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000007000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-481036337153";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000700000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-30064771073";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000070000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1879048193";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000007000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-117440513";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000700000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-7340033";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000070000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-458753";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000007000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-28673";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000000700) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1793";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000000070) -MPRNT_OFF_T_C(1); co_test[i].expected = "-113";
						i++; co_test[i].num = -MPRNT_OFF_T_C(0x0000000000000007) -MPRNT_OFF_T_C(1); co_test[i].expected = "-8";
						i++; co_test[i].num =  MPRNT_OFF_T_C(0x0000000000000000) -MPRNT_OFF_T_C(1); co_test[i].expected = "-1";
						num_cofft_tests = i;
					}
				//#endif
					for(i = 1; i <= num_cofft_tests; i++) {
						for(j = 0; j<BUFSZ; j++)
							co_test[i].result[j] = 'X';
						co_test[i].result[BUFSZ-1] = '\0';
						slsprintf_s(co_test[i].result, sizeof(co_test[i].result), "%" "lld", co_test[i].num);
						if(memcmp(co_test[i].result, co_test[i].expected, strlen(co_test[i].expected))) {
							printf("curl_off_t test #%.2d: Failed (Expected: %s Got: %s)\n", i, co_test[i].expected, co_test[i].result);
							failed++;
						}
					}
					if(!failed)
						printf("All curl_mprintf() curl_off_t tests OK!\n");
					else
						printf("Some curl_mprintf() curl_off_t tests Failed!\n");
					return failed;
				}
				static int _string_check(int linenumber, char * buf, const char * buf2)
				{
					if(strcmp(buf, buf2)) {
						/* they shouldn't differ */
						printf("sprintf line %d failed:\nwe      '%s'\nsystem: '%s'\n", linenumber, buf, buf2);
						return 1;
					}
					return 0;
				}
				#define string_check(x, y) _string_check(__LINE__, x, y)
				static int _strlen_check(int linenumber, char * buf, size_t len)
				{
					size_t buflen = strlen(buf);
					if(len != buflen) {
						/* they shouldn't differ */
						printf("sprintf strlen:%d failed:\nwe '%zu'\nsystem: '%zu'\n", linenumber, buflen, len);
						return 1;
					}
					return 0;
				}
				#define strlen_check(x, y) _strlen_check(__LINE__, x, y)
				//
				// The output strings in this test need to have been verified with a system
				// sprintf() before used here.
				//
				static int test_string_formatting()
				{
					int errors = 0;
					char buf[256];
					slsprintf_s(buf, sizeof(buf), "%0*d%s", 2, 9, "foo");
					errors += string_check(buf, "09foo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", 5, 2, "foo");
					errors += string_check(buf, "   fo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", 2, 5, "foo");
					errors += string_check(buf, "foo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", 0, 10, "foo");
					errors += string_check(buf, "foo");
					slsprintf_s(buf, sizeof(buf), "%-10s", "foo");
					errors += string_check(buf, "foo       ");
					slsprintf_s(buf, sizeof(buf), "%10s", "foo");
					errors += string_check(buf, "       foo");
					slsprintf_s(buf, sizeof(buf), "%*.*s", -10, -10, "foo");
					errors += string_check(buf, "foo       ");
					if(!errors)
						printf("All curl_mprintf() strings tests OK!\n");
					else
						printf("Some curl_mprintf() string tests Failed!\n");
					return errors;
				}
				static int test_weird_arguments()
				{
					int errors = 0;
					char buf[256];
					int rc;
					/* MAX_PARAMETERS is 128, try exact 128! */
					rc = slsprintf_s(buf, sizeof(buf),
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 1 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 2 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 3 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 4 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 5 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 6 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 7 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 8 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 9 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 11 */
						"%d%d%d%d%d%d%d%d"           /* 8 */
						,
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 1 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 2 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 3 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 4 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 5 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 6 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 7 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 8 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 9 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 11 */
						0, 1, 2, 3, 4, 5, 6, 7);       /* 8 */
					if(rc != 128) {
						printf("curl_mprintf() returned %d and not 128!\n", rc);
						errors++;
					}
					errors += string_check(buf,
						"0123456789"          /* 10 */
						"0123456789"          /* 10 1 */
						"0123456789"          /* 10 2 */
						"0123456789"          /* 10 3 */
						"0123456789"          /* 10 4 */
						"0123456789"          /* 10 5 */
						"0123456789"          /* 10 6 */
						"0123456789"          /* 10 7 */
						"0123456789"          /* 10 8 */
						"0123456789"          /* 10 9 */
						"0123456789"          /* 10 10*/
						"0123456789"          /* 10 11 */
						"01234567"            /* 8 */
						);
					/* MAX_PARAMETERS is 128, try more! */
					buf[0] = 0;
					rc = slsprintf_s(buf, sizeof(buf),
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 1 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 2 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 3 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 4 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 5 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 6 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 7 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 8 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 9 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 10 */
						"%d%d%d%d%d%d%d%d%d%d"       /* 10 11 */
						"%d%d%d%d%d%d%d%d%d"         /* 9 */
						,
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 1 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 2 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 3 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 4 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 5 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 6 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 7 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 8 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 9 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 10 */
						0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       /* 10 11 */
						0, 1, 2, 3, 4, 5, 6, 7, 8);         /* 9 */
					/* Это тест проверяет невозможность передачи более 128 параметров.
					Я не согласен с таким правилом (оно не оговорено ни какими стандартами.
					if(rc != -1) {
						printf("curl_mprintf() returned %d and not -1!\n", rc);
						errors++;
					}
					errors += string_check(buf, "");
					*/
					/* Do not skip sanity checks with parameters! */
					buf[0] = 0;
					rc = slsprintf_s(buf, sizeof(buf), "%d, %.*1$d", 500, 1);
					if(rc != 256) {
						printf("curl_mprintf() returned %d and not 256!\n", rc);
						errors++;
					}
					errors += strlen_check(buf, 255);
					if(errors)
						printf("Some curl_mprintf() weird arguments tests failed!\n");
					return errors;
				}

				#define MAXIMIZE -1.7976931348623157081452E+308 // DBL_MAX value from Linux  !checksrc! disable PLUSNOSPACE 1 

				static int test_float_formatting()
				{
					int errors = 0;
					char buf[512]; /* larger than max float size */
					slsprintf_s(buf, sizeof(buf), "%f", 9.0);
					errors += string_check(buf, "9.000000");
					slsprintf_s(buf, sizeof(buf), "%.1f", 9.1);
					errors += string_check(buf, "9.1");
					slsprintf_s(buf, sizeof(buf), "%.2f", 9.1);
					errors += string_check(buf, "9.10");
					slsprintf_s(buf, sizeof(buf), "%.0f", 9.1);
					errors += string_check(buf, "9");
					slsprintf_s(buf, sizeof(buf), "%0f", 9.1);
					errors += string_check(buf, "9.100000");
					slsprintf_s(buf, sizeof(buf), "%10f", 9.1);
					errors += string_check(buf, "  9.100000");
					slsprintf_s(buf, sizeof(buf), "%10.3f", 9.1);
					errors += string_check(buf, "     9.100");
					slsprintf_s(buf, sizeof(buf), "%-10.3f", 9.1);
					errors += string_check(buf, "9.100     ");
					slsprintf_s(buf, sizeof(buf), "%-10.3f", 9.123456);
					errors += string_check(buf, "9.123     ");
					slsprintf_s(buf, sizeof(buf), "%.-2f", 9.1);
					errors += string_check(buf, "9.100000");
					slsprintf_s(buf, sizeof(buf), "%*f", 10, 9.1);
					errors += string_check(buf, "  9.100000");
					slsprintf_s(buf, sizeof(buf), "%*f", 3, 9.1);
					errors += string_check(buf, "9.100000");
					slsprintf_s(buf, sizeof(buf), "%*f", 6, 9.2987654);
					errors += string_check(buf, "9.298765");
					slsprintf_s(buf, sizeof(buf), "%*f", 6, 9.298765);
					errors += string_check(buf, "9.298765");
					slsprintf_s(buf, sizeof(buf), "%*f", 6, 9.29876);
					errors += string_check(buf, "9.298760");
					slsprintf_s(buf, sizeof(buf), "%.*f", 6, 9.2987654);
					errors += string_check(buf, "9.298765");
					slsprintf_s(buf, sizeof(buf), "%.*f", 5, 9.2987654);
					errors += string_check(buf, "9.29877");
					slsprintf_s(buf, sizeof(buf), "%.*f", 4, 9.2987654);
					errors += string_check(buf, "9.2988");
					slsprintf_s(buf, sizeof(buf), "%.*f", 3, 9.2987654);
					errors += string_check(buf, "9.299");
					slsprintf_s(buf, sizeof(buf), "%.*f", 2, 9.2987654);
					errors += string_check(buf, "9.30");
					slsprintf_s(buf, sizeof(buf), "%.*f", 1, 9.2987654);
					errors += string_check(buf, "9.3");
					slsprintf_s(buf, sizeof(buf), "%.*f", 0, 9.2987654);
					errors += string_check(buf, "9");
					/* very large precisions easily turn into system specific outputs so we only
					   check the output buffer length here as we know the internal limit */
					slsprintf_s(buf, sizeof(buf), "%.*f", (1<<30), 9.2987654);
					errors += strlen_check(buf, 325);
					slsprintf_s(buf, sizeof(buf), "%10000.10000f", 9.2987654);
					errors += strlen_check(buf, 325);
					slsprintf_s(buf, sizeof(buf), "%240.10000f", 123456789123456789123456789.2987654);
					errors += strlen_check(buf, 325);
					/* check negative when used signed */
					slsprintf_s(buf, sizeof(buf), "%*f", INT_MIN, 9.1);
					errors += string_check(buf, "9.100000");
					// slsprintf_s() limits a single float output to 325 bytes maximum width 
					slsprintf_s(buf, sizeof(buf), "%*f", (1<<30), 9.1);
					errors += string_check(buf, "                                                                                                                                                                                                                                                                                                                             9.100000");
					slsprintf_s(buf, sizeof(buf), "%100000f", 9.1);
					errors += string_check(buf, "                                                                                                                                                                                                                                                                                                                             9.100000");
					slsprintf_s(buf, sizeof(buf), "%f", MAXIMIZE);
					errors += strlen_check(buf, 317);
					slsprintf_s(buf, 2, "%f", MAXIMIZE);
					errors += strlen_check(buf, 1);
					slsprintf_s(buf, 3, "%f", MAXIMIZE);
					errors += strlen_check(buf, 2);
					slsprintf_s(buf, 4, "%f", MAXIMIZE);
					errors += strlen_check(buf, 3);
					slsprintf_s(buf, 5, "%f", MAXIMIZE);
					errors += strlen_check(buf, 4);
					slsprintf_s(buf, 6, "%f", MAXIMIZE);
					errors += strlen_check(buf, 5);
					if(!errors)
						printf("All float strings tests OK!\n");
					else
						printf("test_float_formatting Failed!\n");
					return errors;
				}
			};
#pragma warning(pop)
			SLCHECK_Z(InnerBlock::test_unsigned_short_formatting());
			SLCHECK_Z(InnerBlock::test_signed_short_formatting());
			SLCHECK_Z(InnerBlock::test_unsigned_int_formatting());
			SLCHECK_Z(InnerBlock::test_signed_int_formatting());
			SLCHECK_Z(InnerBlock::test_unsigned_long_formatting());
			SLCHECK_Z(InnerBlock::test_signed_long_formatting());
			// @todo Следующие 4 теста не проходят - надо разбираться
			//SLCHECK_Z(InnerBlock::test_weird_arguments()); // !
			//SLCHECK_Z(InnerBlock::test_float_formatting()); // !
			//SLCHECK_Z(InnerBlock::test_string_formatting()); // !
			//SLCHECK_Z(InnerBlock::test_curl_off_t_formatting()); // !
		}
#endif // } 0 @construction
	}
	else if(bm == 1) { // slprintf
		SString temp_buf;
		{
			for(long i = -2000000; i <= +1000000; i++) {
				slprintf(temp_buf.Z(), "this is a long number %ld", i);
			}
		}
		{
			for(int64 i = -2000000; i <= +1000000; i++) {
				slprintf(temp_buf.Z(), "this is a long long number %llx", i);
			}
		}
		{
			for(double v = -1.0e15; v < 1.0e15;) {
				slprintf(temp_buf.Z(), "this is a real number %.5f", v);
				double rn = 0.8e9;
				v += rn;
			}
		}
	}
	else if(bm == 2) { // sprintf
		char  temp_buf[1024];
		{
			for(long i = -2000000; i <= +1000000; i++) {
				_snprintf(temp_buf, sizeof(temp_buf), "this is a long number %ld", i);
			}
		}
		{
			for(int64 i = -2000000; i <= +1000000; i++) {
				_snprintf(temp_buf, sizeof(temp_buf), "this is a long long number %llx", i);
			}
		}
		{
			for(double v = -1.0e15; v < 1.0e15;) {
				_snprintf(temp_buf, sizeof(temp_buf), "this is a real number %.5f", v);
				double rn = 0.8e9;
				v += rn;
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(SStrTransform) // @v11.9.9 @construction
{
	return CurrentStatus;
}

SLTEST_R(FormatInt)
{
	SString result_buf;
	char  proof_buf[128];
	{
		for(int i = -10000000; i < 10000000; i++) {
			FormatInt(i, result_buf);
			_itoa(i, proof_buf, 10);
			SLCHECK_NZ(result_buf == proof_buf);
			assert(result_buf == proof_buf);
		}
	}
	{
		for(uint i = 0; i < 20000000; i++) {
			FormatUInt(i, result_buf);
			_ultoa(i, proof_buf, 10);
			SLCHECK_NZ(result_buf == proof_buf);
			assert(result_buf == proof_buf);
		}
	}
	{
		for(int64 i = -10000000; i < 10000000; i++) {
			FormatInt64(i, result_buf);
			_i64toa(i, proof_buf, 10);
			SLCHECK_NZ(result_buf == proof_buf);
			assert(result_buf == proof_buf);
		}
	}
	{
		for(uint64 i = 0; i < 20000000; i++) {
			FormatUInt64(i, result_buf);
			_ui64toa(i, proof_buf, 10);
			SLCHECK_NZ(result_buf == proof_buf);
			assert(result_buf == proof_buf);
		}
	}
	//benchmark=formatint;itoa;formatuint;ultoa;formatint64;i64toa;formatuint64;ui64toa
	if(sstreqi_ascii(pBenchmark, "formatint")) {
		for(int i = -10000000; i < 10000000; i++) {
			FormatInt(i, result_buf);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "itoa")) {
		for(int i = -10000000; i < 10000000; i++) {
			_itoa(i, proof_buf, 10);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "formatuint")) {
		for(uint i = 0; i < 20000000; i++) {
			FormatUInt(i, result_buf);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "ultoa")) {
		for(uint i = 0; i < 20000000; i++) {
			_ultoa(i, proof_buf, 10);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "formatint64")) {
		for(int64 i = -10000000; i < 10000000; i++) {
			FormatInt64(i, result_buf);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "i64toa")) {
		for(int64 i = -10000000; i < 10000000; i++) {
			_i64toa(i, proof_buf, 10);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "formatuint64")) {
		for(uint64 i = 0; i < 20000000; i++) {
			FormatUInt64(i, result_buf);
		}
	}
	else if(sstreqi_ascii(pBenchmark, "ui64toa")) {
		for(uint64 i = 0; i < 20000000; i++) {
			_ui64toa(i, proof_buf, 10);
		}
	}
	return CurrentStatus;
}

class SlTestFixtureAtoi {
public:
	SlTestFixtureAtoi()
	{
		{
			for(uint i = 0; i < 1000; i++) {
				uint64 v = static_cast<uint64>(i);
				SString * p_new_entry = L.CreateNewItem();
				assert(p_new_entry);
				p_new_entry->Cat(v);
			}
		}
		{
			for(uint i = 1000; i < 10000000; i++) {
				uint64 v = i * 713ULL;
				SString * p_new_entry = L.CreateNewItem();
				assert(p_new_entry);
				p_new_entry->Cat(v);
			}
		}
	}
	TSCollection <SString> L;
};

SLTEST_FIXTURE(atoi, SlTestFixtureAtoi)
{
	// benchmark=atoi;satoi;texttodec1;texttodec2
	if(isempty(pBenchmark)) {
		SStringU temp_buf_u;
		{
			const char * pp_list[] = {
				"0", "-1 ", "1", " 1000000000000001", " -70003394398439", " -3", "-17a", "    49999900000x"
			};
			for(uint i = 0; i < SIZEOFARRAY(pp_list); i++) {
				const char * p_str = pp_list[i];
				int64 a1 = _atoi64(p_str);
				int64 a2 = satoi64(p_str);
				SLCHECK_EQ(a1, a2);
				const bool ucr = temp_buf_u.CopyFromUtf8Strict(p_str, sstrlen(p_str));
				if(ucr) {
					int64 a3 = satoi64(temp_buf_u);
					SLCHECK_EQ(a1, a3);
				}
			}
		}
		{
			const char* pp_list[] = {
				"0", "-1 ", "1", " 10000001", "-394398439", "-3", " -17a", "  4999900x"
			};
			for(uint i = 0; i < SIZEOFARRAY(pp_list); i++) {
				const char* p_str = pp_list[i];
				int a1 = atoi(p_str);
				int a2 = satoi(p_str);
				SLCHECK_EQ(a1, a2);
				const bool ucr = temp_buf_u.CopyFromUtf8Strict(p_str, sstrlen(p_str));
				if(ucr) {
					int a3 = satoi(temp_buf_u);
					SLCHECK_EQ(a1, a3);
				}
			}
		}
		{
			for(uint i = 0; i < F.L.getCount(); i++) {
				const SString * p_item = F.L.at(i);
				int64 a1 = _atoi64(*p_item);
				int64 a2 = satoi64(*p_item);
				SLCHECK_EQ(a1, a2);
				{
					const bool ucr = temp_buf_u.CopyFromUtf8(*p_item);
					if(ucr) {
						int64 a3 = satoi64(temp_buf_u);
						SLCHECK_EQ(a1, a3);
					}
				}
				if(HiDWord(a1) == 0 && !(LoDWord(a1) & 0x80000000U)) {
					int i1 = atoi(*p_item);
					int i2 = satoi(*p_item);
					SLCHECK_EQ(i1, i2);
					{
						const bool ucr = temp_buf_u.CopyFromUtf8(*p_item);
						if(ucr) {
							int i3 = satoi(temp_buf_u);
							SLCHECK_EQ(i1, i3);
						}
					}
				}
			}
		}
		{
			for(uint i = 0; i < F.L.getCount(); i++) {
				const SString * p_item = F.L.at(i);
				uint64 a1 = _texttodec64(*p_item, p_item->Len());
				uint64 a2 = _texttodec64_2(*p_item, p_item->Len());
				SLCHECK_EQ(a1, a2);
			}
		}
	}
	else if(sstreqi_ascii(pBenchmark, "atoi")) {
		int64 s = 0;
		for(uint i = 0; i < F.L.getCount(); i++) {
			const SString * p_item = F.L.at(i);
			int64 a1 = _atoi64(*p_item);
			//int64 a2 = satoi64(*p_item);
			s += a1;
		}
	}
	else if(sstreqi_ascii(pBenchmark, "satoi")) {
		int64 s = 0;
		for(uint i = 0; i < F.L.getCount(); i++) {
			const SString * p_item = F.L.at(i);
			//int64 a1 = _atoi64(*p_item);
			int64 a2 = satoi64(*p_item);
			s += a2;
		}
	}
	else if(sstreqi_ascii(pBenchmark, "texttodec1")) {
		uint64 s = 0;
		for(uint i = 0; i < F.L.getCount(); i++) {
			const SString * p_item = F.L.at(i);
			uint64 a = _texttodec64(*p_item, p_item->Len());
			s += a;
		}
	}
	else if(sstreqi_ascii(pBenchmark, "texttodec2")) {
		uint64 s = 0;
		for(uint i = 0; i < F.L.getCount(); i++) {
			const SString * p_item = F.L.at(i);
			uint64 a = _texttodec64_2(*p_item, p_item->Len());
			s += a;
		}
	}
	return CurrentStatus;
}
//
// Descr: Тест классификации символов
//
SLTEST_R(charclass)
{
	{
		// char
		for(char c = -127; c <= 126; c++) { // ограничение цикла в 126 (не 127) из-за того, что в противном случае на следующей итерации произойдет зацикливание (c вернется к -127)
			if(c >= 'A' && c <= 'Z') {
				SLCHECK_NZ(isasciiupr(c));
			}
			else {
				SLCHECK_Z(isasciiupr(c));
			}
			if(c >= 'a' && c <= 'z') {
				SLCHECK_NZ(isasciilwr(c));
			}
			else {
				SLCHECK_Z(isasciilwr(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
				SLCHECK_NZ(isasciialpha(c));
			}
			else {
				SLCHECK_Z(isasciialpha(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				SLCHECK_NZ(isasciialnum(c));
			}
			else {
				SLCHECK_Z(isasciialnum(c));
			}
			if(c >= '0' && c <= '9') {
				SLCHECK_NZ(isdec(c));
			}
			else {
				SLCHECK_Z(isdec(c));
			}
			if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
				SLCHECK_NZ(ishex(c));
			}
			else {
				SLCHECK_Z(ishex(c));
			}
			if(c == '/' || c == '\\') {
				SLCHECK_NZ(isdirslash(c));
			}
			else {
				SLCHECK_Z(isdirslash(c));
			}
		}
	}
	{
		// uchar
		for(uchar c = 0; c <= 254; c++) { // ограничение 254 - не 255 во избежании зацикливания (итератор 8-битный)
			if(c >= 'A' && c <= 'Z') {
				SLCHECK_NZ(isasciiupr(c));
			}
			else {
				SLCHECK_Z(isasciiupr(c));
			}
			if(c >= 'a' && c <= 'z') {
				SLCHECK_NZ(isasciilwr(c));
			}
			else {
				SLCHECK_Z(isasciilwr(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
				SLCHECK_NZ(isasciialpha(c));
			}
			else {
				SLCHECK_Z(isasciialpha(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				SLCHECK_NZ(isasciialnum(c));
			}
			else {
				SLCHECK_Z(isasciialnum(c));
			}
			if(c >= '0' && c <= '9') {
				SLCHECK_NZ(isdec(c));
			}
			else {
				SLCHECK_Z(isdec(c));
			}
			if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
				SLCHECK_NZ(ishex(c));
			}
			else {
				SLCHECK_Z(ishex(c));
			}
			if(c == '/' || c == '\\') {
				SLCHECK_NZ(isdirslash(c));
			}
			else {
				SLCHECK_Z(isdirslash(c));
			}
		}
	}
	{
		// int
		for(int c = -1000; c <= +1000; c++) {
			if(c >= 'A' && c <= 'Z') {
				SLCHECK_NZ(isasciiupr(c));
			}
			else {
				SLCHECK_Z(isasciiupr(c));
			}
			if(c >= 'a' && c <= 'z') {
				SLCHECK_NZ(isasciilwr(c));
			}
			else {
				SLCHECK_Z(isasciilwr(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
				SLCHECK_NZ(isasciialpha(c));
			}
			else {
				SLCHECK_Z(isasciialpha(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				SLCHECK_NZ(isasciialnum(c));
			}
			else {
				SLCHECK_Z(isasciialnum(c));
			}
			if(c >= '0' && c <= '9') {
				SLCHECK_NZ(isdec(c));
			}
			else {
				SLCHECK_Z(isdec(c));
			}
			if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
				SLCHECK_NZ(ishex(c));
			}
			else {
				SLCHECK_Z(ishex(c));
			}
			if(c == '/' || c == '\\') {
				SLCHECK_NZ(isdirslash(c));
			}
			else {
				SLCHECK_Z(isdirslash(c));
			}
		}
	}
	{
		// uint
		for(uint c = 0; c <= 1000; c++) {
			if(c >= 'A' && c <= 'Z') {
				SLCHECK_NZ(isasciiupr(c));
			}
			else {
				SLCHECK_Z(isasciiupr(c));
			}
			if(c >= 'a' && c <= 'z') {
				SLCHECK_NZ(isasciilwr(c));
			}
			else {
				SLCHECK_Z(isasciilwr(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
				SLCHECK_NZ(isasciialpha(c));
			}
			else {
				SLCHECK_Z(isasciialpha(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				SLCHECK_NZ(isasciialnum(c));
			}
			else {
				SLCHECK_Z(isasciialnum(c));
			}
			if(c >= '0' && c <= '9') {
				SLCHECK_NZ(isdec(c));
			}
			else {
				SLCHECK_Z(isdec(c));
			}
			if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
				SLCHECK_NZ(ishex(c));
			}
			else {
				SLCHECK_Z(ishex(c));
			}
			if(c == '/' || c == '\\') {
				SLCHECK_NZ(isdirslash(c));
			}
			else {
				SLCHECK_Z(isdirslash(c));
			}
		}
	}
	{
		// wchar_t
		for(wchar_t c = -1000; c <= 1000; c++) {
			if(c >= 'A' && c <= 'Z') {
				SLCHECK_NZ(isasciiupr(c));
			}
			else {
				SLCHECK_Z(isasciiupr(c));
			}
			if(c >= 'a' && c <= 'z') {
				SLCHECK_NZ(isasciilwr(c));
			}
			else {
				SLCHECK_Z(isasciilwr(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
				SLCHECK_NZ(isasciialpha(c));
			}
			else {
				SLCHECK_Z(isasciialpha(c));
			}
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				SLCHECK_NZ(isasciialnum(c));
			}
			else {
				SLCHECK_Z(isasciialnum(c));
			}
			if(c >= '0' && c <= '9') {
				SLCHECK_NZ(isdec(c));
			}
			else {
				SLCHECK_Z(isdec(c));
			}
			if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
				SLCHECK_NZ(ishex(c));
			}
			else {
				SLCHECK_Z(ishex(c));
			}
			if(c == '/' || c == '\\') {
				SLCHECK_NZ(isdirslash(c));
			}
			else {
				SLCHECK_Z(isdirslash(c));
			}
		}
	}
	return CurrentStatus;
}