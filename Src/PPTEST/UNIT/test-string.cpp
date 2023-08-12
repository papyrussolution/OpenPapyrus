// TEST-STRING.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование функций класса SString
//
#include <pp.h>
#pragma hdrstop
#include <string>

const char * byteshift_strstr(const char * pHayStack, const char * pNeedle); // @prototype(stext.cpp)
// @v10.7.9 int FASTCALL dconvstr_scan(const char * input, const char ** input_end, double * output, int * output_erange);

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
//
// Аналог strnzcpy но с использованием xeos_memchr вместо memchr
//
/* @v11.0.0 static char * FASTCALL xeos_strnzcpy(char * dest, const char * src, size_t maxlen)
{
	if(dest)
		if(src)
			if(maxlen) {
				const char * p = (SLS.GetSSys().CpuCs >= SSystem::cpucsSSE2) ? (const char *)xeos_memchr32_sse2(src, 0, maxlen) : (const char *)xeos_memchr32(src, 0, maxlen);
				if(p)
					memcpy(dest, src, (size_t)(p - src)+1);
				else {
					memcpy(dest, src, maxlen-1);
					dest[maxlen-1] = 0;
				}
			}
			else
				strcpy(dest, src);
		else
			dest[0] = 0;
	return dest;
}*/

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
					SLCHECK_NZ(str.HasPrefix(buffer)); // @v10.7.8
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
			(str = "  ab  cab  c  ").ReplaceStr("  ", " ", 0);
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
									ss_result.clear();
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
				SLCHECK_EQ(str.Z().Cat(0.1, MKSFMTD(0, 2, 0)), "0.10");
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
// @v11.0.0 xeos-функции полностью исключены из проекта
#if 0 // @v10.8.1 После включения оптимизатора по скорости, похоже, xeos_xxx-функции стали все ломать
		{
			//
			// Тест функций xeos_strlen и xeos_memchr
			//
			uint64 total_len1 = 0;
			uint64 total_len2 = 0;
			char buffer1[2048];
			char buffer2[2048];
			for(uint i = 0; i < F.P_StrList->getCount(); i++) {
				size_t proof_len = strlen(F.P_StrList->at(i));
				assert(proof_len < sizeof(buffer1));
				//assert(proof_len > 16);
				strcpy(buffer1, F.P_StrList->at(i));
				total_len1 += strlen(buffer1);
				buffer2[0] = 0;
				strcpy(buffer2, F.P_StrList->at(i));
				total_len2 += xeos_strlen(buffer2);
				SLCHECK_NZ(sstreq(buffer1, buffer2));
				SLCHECK_Z(strcmp(buffer1, buffer2));
				{
					for(uint offs = 0; offs < 8; offs++) {
						const char * p_buf1 = buffer1+offs;
						const char * p_buf2 = buffer2+offs;
						for(int j = 0; j < 256; j++) {
							const char * p1 = (const char *)memchr(p_buf1, j, proof_len+1-offs);
							const char * p2 = (const char *)xeos_memchr(p_buf2, j, proof_len+1-offs);
							SLCHECK_NZ((!p1 && !p2) || ((p1 && p2) && (p1-p_buf1) == (p2-p_buf2)));
						}
					}
				}
			}
			SLCHECK_EQ(total_len1, total_len2);
		}
#endif // } @v10.8.1
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
			// @v10.8.1 {
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
			// } @v10.8.1 
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
		/* @v11.0.0
		else if(bm == 6) {
			for(uint phase = 0; phase < max_bm_phase; phase++) {
				for(uint i = 0; i < scc; i++) {
					char buffer[2048];
					xeos_strnzcpy(buffer, F.P_StrList->at(i), sizeof(buffer));
					total_len += xeos_strlen(buffer);
				}
			}
		}*/
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

SLTEST_R(SPathStruc)
{
	int    ok = 1;
	SFile file(MakeInputFilePath("path.txt"), SFile::mRead | SFile::mBinary);
	if(file.IsValid()) {
		SString line_buf;
		SFile out_file(MakeOutputFilePath("path.out"), SFile::mWrite);
		SPathStruc ps;
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
				THROW(rFep.Add(rPath, de));
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
	SPathStruc ps;
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
	{
		char   one[128];
		SLCHECK_Z(sstrchr("abcd", 'z'));
		strcpy(one, "abcd");
		SLCHECK_EQ(sstrchr(one, 'c'), one+2);
		SLCHECK_EQ(sstrchr(one, 'd'), one+3);
		SLCHECK_EQ(sstrchr(one, 'a'), one);
		SLCHECK_EQ(sstrchr(one, '\0'), one+4);
		strcpy(one, "ababa");
		SLCHECK_EQ(sstrchr(one, 'b'), one+1);
		strcpy(one, "");
		SLCHECK_Z(sstrchr(one, 'b'));
		SLCHECK_EQ(sstrchr(one, '\0'), one);
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
		SLCHECK_EQ(sstrchr(buf, '\0'), buf+buf.GetSize()-1);
		SLCHECK_EQ(sstrchr(buf, 'x'), buf);
		for(size_t i = 0; i < buf.GetSize()-1; i += 1021) {
			buf[i] = 'y';
			SLCHECK_EQ(sstrchr(buf, 'y'), buf+i);
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
			uint sspos = 0;
			for(uint i = 0; i < rStringList.getCount(); i++) {
				const uint _prev_sspos = sspos;
				THROW(rSs.get(&sspos, temp_buf));
				const char * p_str = rStringList.at(i);
				THROW(temp_buf == p_str);
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
				THROW(ss.add(string_list.at(i), &pos));
				THROW(pos_list.add(static_cast<long>(pos)));
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
				THROW(ss.add(string_list.at(i), &pos));
				THROW(pos_list.add(static_cast<long>(pos)));
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
