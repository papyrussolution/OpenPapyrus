// TEST-STRING.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование функций класса SString
//
#include <pp.h>
#pragma hdrstop
#include <string>

// @v10.7.9 int FASTCALL dconvstr_scan(const char * input, const char ** input_end, double * output, int * output_erange);

struct SlTestFixtureSString {
public:
	SlTestFixtureSString() : P_StrList(0)
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
			THROW(P_StrList = new SStrCollection);
			{
				SString temp_buf;
				SFile f_in(pFileName, SFile::mRead);
				THROW(f_in.IsValid());
				while(f_in.ReadLine(temp_buf)) {
					char * p_new_str = newStr(temp_buf);
					THROW(p_new_str);
					THROW(P_StrList->insert(p_new_str));
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
    THROW(SLTEST_CHECK_NZ(islr));
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
	else SetInfo("invalid benchmark");
	if(bm == 0) {
		{
			//
			// Тестирование функций сравнения строк
			//
			// strcmp с нулевыми указателями не работает! SLTEST_CHECK_Z(strcmp("", 0));
			SLTEST_CHECK_NZ(sstreq("", static_cast<const char *>(0)));
			SLTEST_CHECK_NZ(sstreq(static_cast<const char *>(0), ""));
			SLTEST_CHECK_Z(sstreq(static_cast<const char *>(0), "ab"));
			SLTEST_CHECK_Z(sstreq("b", static_cast<const char *>(0)));
			SLTEST_CHECK_NZ(sstreq("ab", "ab"));
			SLTEST_CHECK_NZ(sstreq("", ""));
			SLTEST_CHECK_Z(sstreq("ab", "abc"));
			SLTEST_CHECK_NZ(sstreq("король и королева", SString("король").Space().Cat("и").CatChar(' ').Cat("королева")));
			SLTEST_CHECK_Z(sstreq("король и королева", "король и королёва"));
			SLTEST_CHECK_NZ(sstreq(L"король и королева", L"король и королева"));
			SLTEST_CHECK_NZ(sstreq(L"robot", SStringU(L"robot")));
			//
			SLTEST_CHECK_Z(stricmp866(0, ""));
			SLTEST_CHECK_Z(stricmp866("", 0));
			SLTEST_CHECK_Z(stricmp1251(0, ""));
			SLTEST_CHECK_Z(stricmp1251("", 0));
			// Attention! Следующие тесты действительны до тех пор, пока выполняется соответствие INNER-кодировка == cp866, OUTER-кодировка == cp1251
			SLTEST_CHECK_Z(stricmp866(SString("Король и Королева").Transf(CTRANSF_UTF8_TO_INNER), SString("король и королева").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(stricmp866(SString("Король и КоролЁва").Transf(CTRANSF_UTF8_TO_INNER), SString("король и королёва").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(stricmp1251(SString("Король и Королева").Transf(CTRANSF_UTF8_TO_OUTER), SString("король и королева").Transf(CTRANSF_UTF8_TO_OUTER)));
			SLTEST_CHECK_Z(stricmp1251(SString("Король и Королёва").Transf(CTRANSF_UTF8_TO_OUTER), SString("король и королЁва").Transf(CTRANSF_UTF8_TO_OUTER)));
		}
		{
			SFile out(MakeOutputFilePath("SString_NumberToLat.txt"), SFile::mWrite);
			for(uint i = 0; i < 1000; i++) {
				str.Z().NumberToLat(i);
				out.WriteLine(out_buf.Printf("%u\t\t%s\n", i, str.cptr()));
			}
			{
				SLTEST_CHECK_NZ(sstreqi_ascii("u", "u"));
				SLTEST_CHECK_NZ(sstreqi_ascii("u", "U"));
				SLTEST_CHECK_NZ(sstreqi_ascii("U", "u"));
				SLTEST_CHECK_Z(sstreqi_ascii("U", "u1"));
				SLTEST_CHECK_NZ(sstreqi_ascii("string", "sTrInG"));
				SLTEST_CHECK_NZ(sstreqi_ascii("string", "string"));
				SLTEST_CHECK_Z(sstreqi_ascii("string", "string "));
			}
		}
		//
		// Тестирование функций HasPrefix() и CmpPrefix()
		//
		{
			SString prefix;
			SString text("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz");
			SLTEST_CHECK_NZ(text.HasPrefix("A"));
			SLTEST_CHECK_NZ(text.HasPrefix("ABC"));
			SLTEST_CHECK_NZ(text.HasPrefix("ABCD"));
			SLTEST_CHECK_Z(text.HasPrefix("a"));
			SLTEST_CHECK_Z(text.HasPrefix("abc"));
			SLTEST_CHECK_Z(text.HasPrefix("aBcd"));
			SLTEST_CHECK_NZ(text.HasPrefixNC("A"));
			SLTEST_CHECK_NZ(text.HasPrefixNC("ABC"));
			SLTEST_CHECK_NZ(text.HasPrefixNC("ABCD"));
			SLTEST_CHECK_NZ(text.HasPrefixNC("a"));
			SLTEST_CHECK_NZ(text.HasPrefixNC("AbC"));
			SLTEST_CHECK_NZ(text.HasPrefixNC("AbCd"));
			SLTEST_CHECK_Z(text.HasPrefixNC("QUQU"));
			SLTEST_CHECK_NZ(text.HasPrefixIAscii("A"));
			SLTEST_CHECK_NZ(text.HasPrefixIAscii("ABC"));
			SLTEST_CHECK_NZ(text.HasPrefixIAscii("ABCD"));
			SLTEST_CHECK_NZ(text.HasPrefixIAscii("a"));
			SLTEST_CHECK_NZ(text.HasPrefixIAscii("AbC"));
			SLTEST_CHECK_NZ(text.HasPrefixIAscii("AbCd"));

			(text = "На дворе трава на траве дрова").Transf(CTRANSF_UTF8_TO_INNER);
			SLTEST_CHECK_NZ(text.HasPrefix((prefix = "Н").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_NZ(text.HasPrefix((prefix = "На дворе").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_NZ(text.HasPrefix((prefix = "На дворе трава").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefix((prefix = "дрова").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefix((prefix = "НА ДВОРЕ ТРАВА").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_NZ(text.HasPrefixNC((prefix = "н").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_NZ(text.HasPrefixNC((prefix = "На дВоре").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_NZ(text.HasPrefixNC((prefix = "На дворе трАва").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_NZ(text.HasPrefixNC((prefix = "НА ДВОРЕ ТРАВА").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefixNC((prefix = "ДРОВА").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefixIAscii((prefix = "н").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefixIAscii((prefix = "На дВоре").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefixIAscii((prefix = "На дворе трАва").Transf(CTRANSF_UTF8_TO_INNER)));
			SLTEST_CHECK_Z(text.HasPrefixIAscii((prefix = "НА ДВОРЕ ТРАВА").Transf(CTRANSF_UTF8_TO_INNER)));
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
				SLTEST_CHECK_EQ(str.Len(), _src_len);
			}
			for(uint tl = 1; tl <= sizeof(buffer); tl++) {
				str.CopyTo(buffer, tl);
				if(tl > 1) {
					SLTEST_CHECK_Z(str.CmpPrefix(buffer, 0));
					SLTEST_CHECK_NZ(str.HasPrefix(buffer)); // @v10.7.8
				}
				{
					const size_t real_data_len = MIN(tl, str.Len()+1);
					SLTEST_CHECK_Z(memcmp(str.cptr(), buffer, real_data_len-1));
					SLTEST_CHECK_EQ((uint8)buffer[real_data_len-1], static_cast<uint8>(0));
					SLTEST_CHECK_Z(memcmp(buffer+real_data_len, preserve_buffer+real_data_len, sizeof(buffer)-real_data_len));
				}
			}
			{
				str.CopyTo(buffer, 0);
				SLTEST_CHECK_Z(str.CmpPrefix(buffer, 0));
				SLTEST_CHECK_NZ(str.HasPrefix(buffer));
				SLTEST_CHECK_Z(memcmp(str.cptr(), buffer, str.Len()));
				SLTEST_CHECK_EQ(strlen(buffer), str.Len());
				SLTEST_CHECK_EQ((uint8)buffer[str.Len()], static_cast<uint8>(0));
				SLTEST_CHECK_Z(memcmp(buffer+str.Len()+1, preserve_buffer+str.Len()+1, sizeof(buffer)-str.Len()-1));
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
				SLTEST_CHECK_EQ(org_len, rev_str.Len());
				SLTEST_CHECK_EQ(org_len, str.Len());
				for(uint i = 0; i < org_len; i++) {
					SLTEST_CHECK_EQ(static_cast<uint8>(str.C(i)), static_cast<uint8>(rev_str.C(org_len-i-1)));
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
			uint pos;
			SLTEST_CHECK_NZ(text_u.Search(L"ABC", 0, &pos));
			SLTEST_CHECK_EQ(pos, 0U);
			SLTEST_CHECK_Z(text_u.Search(L"ABC", 10, &pos));
			p_temp_text = "АБРИК";
			temp_buf_u.CopyFromUtf8(p_temp_text, sstrlen(p_temp_text));
			SLTEST_CHECK_NZ(text_u.Search(temp_buf_u, 0, &pos));
			SLTEST_CHECK_EQ(pos, 63U);
			SLTEST_CHECK_NZ(text_u.Search(temp_buf_u, 20, &pos));
			SLTEST_CHECK_EQ(pos, 63U);
			temp_buf_u.CopyFromUtf8(p_text, sstrlen(p_text));
			SLTEST_CHECK_NZ(text_u.Search(temp_buf_u, 0, &pos));
			SLTEST_CHECK_EQ(pos, 0U);
			p_temp_text = "городовой";
			temp_buf_u.CopyFromUtf8(p_temp_text, sstrlen(p_temp_text));
			SLTEST_CHECK_NZ(text_u.Search(temp_buf_u, 0, &pos));
			SLTEST_CHECK_EQ(pos, 71U);
			SLTEST_CHECK_NZ(text_u.Search(temp_buf_u, 71, &pos));
			SLTEST_CHECK_EQ(pos, 71U);
			SLTEST_CHECK_Z(text_u.Search(temp_buf_u, 1000, &pos));
			p_temp_text = "гороДовой";
			temp_buf_u.CopyFromUtf8(p_temp_text, sstrlen(p_temp_text));
			SLTEST_CHECK_Z(text_u.Search(temp_buf_u, 1, &pos));
		}
		{
			SLTEST_CHECK_EQ((str = " abc ").Strip(1), " abc");
			SLTEST_CHECK_EQ((str = " abc ").Strip(2), "abc ");
			SLTEST_CHECK_EQ((str = " abc ").Strip(0), "abc");
			SLTEST_CHECK_EQ((str = " abc ").Strip(), "abc");
			SLTEST_CHECK_EQ((str = "abc").Strip(), "abc");
			SLTEST_CHECK_EQ((str = "\"abc\"").StripQuotes(), "abc");
			SLTEST_CHECK_EQ((str = " \"abc\"" ).StripQuotes(), "abc");
			SLTEST_CHECK_EQ((str = "abc").StripQuotes(), "abc");
		}
		{
			//
			// Тестирование IsEqiAscii
			//
			SLTEST_CHECK_NZ((str = "").IsEqiAscii(0));
			SLTEST_CHECK_NZ((str = "").IsEqiAscii(""));
			SLTEST_CHECK_NZ((str = " ").IsEqiAscii(" "));
			SLTEST_CHECK_NZ((str = "abc").IsEqiAscii("abc"));
			SLTEST_CHECK_NZ((str = "abc").IsEqiAscii("aBc"));
			SLTEST_CHECK_NZ((str = "городовой").IsEqiAscii("городовой"));
			SLTEST_CHECK_Z((str = "городовой").IsEqiAscii("гороДовой"));
		}
		{
			//
			// Тестирование IsEqiUtf8 (не забываем, что кодировка исходного файла UTF8)
			//
			SLTEST_CHECK_NZ((str = "").IsEqiUtf8(0));
			SLTEST_CHECK_NZ((str = "").IsEqiUtf8(""));
			SLTEST_CHECK_NZ((str = " ").IsEqiUtf8(" "));
			SLTEST_CHECK_NZ((str = "abc").IsEqiUtf8("abc"));
			SLTEST_CHECK_NZ((str = "abc").IsEqiUtf8("aBc"));
			SLTEST_CHECK_NZ((str = "городовой").IsEqiUtf8("городОвой"));
			SLTEST_CHECK_NZ((str = "штрихкод").IsEqiUtf8("Штрихкод"));
			SLTEST_CHECK_NZ((str = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя").IsEqiUtf8("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"));
			SLTEST_CHECK_NZ((str = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ").IsEqiUtf8("абвгдеёжзийклмнопрстуфхцчшщъыьэюя"));
			SLTEST_CHECK_Z((str = "городовой").IsEqiUtf8("городОвой-"));
			SLTEST_CHECK_Z((str = "городовой").IsEqiUtf8("городoвой")); // во втором слове русская 'о' заменена на латинскую 'o'
			SLTEST_CHECK_Z((str = "").IsEqiUtf8(" "));
		}
		{
			//
			// Тестирование функции ReplaceStr
			//
			(str = "abcabc").ReplaceStr("abc", "abcd", 0);
			SLTEST_CHECK_EQ(str, "abcdabcd");
			(str = "  ab  cab  c  ").ReplaceStr("  ", " ", 0);
			SLTEST_CHECK_EQ(str, " ab cab c ");
			(str = "$ab$$cab$c$").ReplaceStr("$", "$$", 0);
			SLTEST_CHECK_EQ(str, "$$ab$$$$cab$$c$$");
		}
		//
		// Тестирование функций EncodeMime64 и DecodeMime64
		//
		{
			SString org_bin_file_name;
			SString test_bin_file_name;
			size_t actual_size = 0;
			THROW(SLTEST_CHECK_NZ(temp_buf.Alloc(4096)));
			THROW(SLTEST_CHECK_NZ(test_buf.Alloc(8192)));
			{
				int r;
				org_bin_file_name = MakeInputFilePath("binfile.");
				SFile inf(org_bin_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd);
				SFile outf(MakeOutputFilePath("txtfile.temp"), SFile::mWrite);
				THROW(SLTEST_CHECK_NZ(inf.IsValid()));
				THROW(SLTEST_CHECK_NZ(outf.IsValid()));
				while((r = inf.Read(temp_buf.P_Buf, temp_buf.Size, &actual_size)) != 0) {
					size_t as = 0;
					str.EncodeMime64(temp_buf.P_Buf, actual_size).CR();
					str.DecodeMime64(test_buf.P_Buf, test_buf.Size, &as);
					THROW(SLTEST_CHECK_EQ(actual_size, as));
					THROW(SLTEST_CHECK_Z(memcmp(temp_buf.P_Buf, test_buf.P_Buf, as)));
					THROW(SLTEST_CHECK_NZ(outf.WriteLine(str)));
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
				THROW(SLTEST_CHECK_NZ(inf.IsValid()));
				THROW(SLTEST_CHECK_NZ(outf.IsValid()));
				THROW(SLTEST_CHECK_NZ(temp_buf.Alloc(8192)));
				while(inf.ReadLine(str, SFile::rlfChomp)) {
					THROW(SLTEST_CHECK_NZ(str.DecodeMime64(temp_buf.P_Buf, temp_buf.Size, &actual_size)));
					THROW(SLTEST_CHECK_NZ(outf.Write(temp_buf.P_Buf, actual_size)));
				}
			}
			SLTEST_CHECK_LT(0, SFile::Compare(org_bin_file_name, test_bin_file_name, 0));
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
				SLTEST_CHECK_Z(in_buf.Cmp(out_buf, 0));

				out_buf = in_buf;
				out_buf.ToUtf8();
				out_buf.Utf8ToChar();
				out_buf.ToOem();
				out_buf.Transf(CTRANSF_INNER_TO_OUTER);
				SLTEST_CHECK_Z(in_buf.Cmp(out_buf, 0));

				{
					SStringU s16;
					out_buf = in_buf;
					out_buf.ToUtf8();
					s16.CopyFromUtf8Strict(out_buf, out_buf.Len());
					s16.CopyToUtf8(out_buf, 1);
					out_buf.Utf8ToChar();
					out_buf.ToOem();
					out_buf.Transf(CTRANSF_INNER_TO_OUTER);
					SLTEST_CHECK_Z(in_buf.Cmp(out_buf, 0));
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
									SLTEST_CHECK_NZ(result_str == ss_result.getBuf());
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
				SLTEST_CHECK_NZ(sstreq(static_cast<const char *>(0), static_cast<const char *>(0)));
				SLTEST_CHECK_NZ(sstreqi_ascii(static_cast<const char *>(0), static_cast<const char *>(0)));
				SLTEST_CHECK_NZ(sstreqi_ascii((const uchar *)0, (const uchar *)0));
				while(inf.ReadLine(str, SFile::rlfChomp|SFile::rlfStrip)) {
					line_no++;
					if(str.NotEmpty()) {
						scan.Set(str, 0);
						int   emsr = scan.GetEMail(out_buf.Z());
						SLTEST_CHECK_NZ(emsr);
					}
					{
						out_buf = str;
						SLTEST_CHECK_NZ(sstreq(out_buf, str));
						SLTEST_CHECK_NZ(sstreq(str, str));
						out_buf.ToUpper();
						str.ToLower();
						SLTEST_CHECK_Z(sstreq(out_buf, str));
						SLTEST_CHECK_NZ(sstreqi_ascii(str, out_buf));
						out_buf.CatChar('x');
						SLTEST_CHECK_Z(sstreqi_ascii(str, out_buf));
						str.CatChar('y');
						SLTEST_CHECK_Z(sstreqi_ascii(str, out_buf));
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
				SLTEST_CHECK_Z(scan.GetQuotedString(SFileFormat::Csv, str));
				SLTEST_CHECK_EQ(scan.Offs, 0U);
				SLTEST_CHECK_NZ(str.IsEmpty());
				//
				scan.Set("abc", 0);
				SLTEST_CHECK_Z(scan.GetQuotedString(SFileFormat::Csv, str));
				SLTEST_CHECK_EQ(scan.Offs, 0U);
				SLTEST_CHECK_NZ(str.IsEmpty());
				//
				scan.Set("\"abc\"0123", 0);
				SLTEST_CHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLTEST_CHECK_EQ(scan.Offs, 5U);
				SLTEST_CHECK_EQ(str, "abc");
				//
				scan.Set("\"ab\"\"c\"0123", 0);
				SLTEST_CHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLTEST_CHECK_EQ(scan.Offs, 7U);
				SLTEST_CHECK_EQ(str, "ab\"c");
				//
				scan.Set("\"abc\"", 0);
				SLTEST_CHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLTEST_CHECK_EQ(scan.Offs, 5U);
				SLTEST_CHECK_EQ(str, "abc");
				//
				scan.Set("\"ab\"\"c\"", 0);
				SLTEST_CHECK_NZ(scan.GetQuotedString(SFileFormat::Csv, str));
				SLTEST_CHECK_EQ(scan.Offs, 7U);
				SLTEST_CHECK_EQ(str, "ab\"c");
				//
				scan.Set("\"ab\"\"c\"", 0);
				SLTEST_CHECK_NZ(scan.GetQuotedString(SFileFormat::Unkn, str));
				SLTEST_CHECK_EQ(scan.Offs, 4U);
				SLTEST_CHECK_EQ(str, "ab");
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
						SLTEST_CHECK_Z(str.IsLegalUtf8());
						str.Transf(CTRANSF_OUTER_TO_UTF8);
						SLTEST_CHECK_NZ(str.IsLegalUtf8());
					}
				}
			}
			{
				SLTEST_CHECK_EQ((str = "0").ToLong(), 0L);
				SLTEST_CHECK_EQ((str = "abc").ToLong(), 0L);
				SLTEST_CHECK_EQ((str = "\t+100 ").ToLong(), 100L);
				SLTEST_CHECK_EQ((str = " -197 ").ToLong(), -197L);
				SLTEST_CHECK_EQ((str = "0xbeefZ").ToLong(), 0xbeefL);
				SLTEST_CHECK_EQ((str = " -0x2BcD7a92 ").ToLong(), -0x2BCD7A92L);
				SLTEST_CHECK_EQ((str = " - 17 ").ToLong(), 0L); // Между знаком - и числом не должно быть пробелов
				SLTEST_CHECK_EQ((str = "0").ToInt64(), 0LL);
				SLTEST_CHECK_EQ((str = "abc").ToInt64(), 0LL);
				SLTEST_CHECK_EQ((str = "\t 0x1ABCDEF234567890").ToInt64(), 0x1ABCDEF234567890LL);
				SLTEST_CHECK_EQ((str = "\t\t123000012878963").ToInt64(), 123000012878963LL);
				SLTEST_CHECK_EQ((str = "-123000012878963").ToInt64(), -123000012878963LL);
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
					//SLTEST_CHECK_NZ(satof(p_text, &v_satof));
					//SLTEST_CHECK_EQ(v_satof, v_atof);

					const char * p_end = 0;
					int erange = 0;
					//SLTEST_CHECK_NZ(dconvstr_scan(p_text, &p_end, &v_satof, &erange));
					v_satof = satof(p_text); // @v10.7.9 dconvstr_scan-->satof
					SLTEST_CHECK_EQ(v_satof, v_atof);
				}
				SString atof_buf;
				for(uint j = 0; j < F.RandomRealList.getCount(); j++) {
					atof_buf.Z().Cat(F.RandomRealList.at(j), MKSFMTD(0, 20, NMBF_NOTRAILZ));
					double v_satof;
					const double v_atof = atof(atof_buf);
					const char * p_end = 0;
					int erange = 0;
					//SLTEST_CHECK_NZ(dconvstr_scan(atof_buf, &p_end, &v_satof, &erange));
					v_satof = satof(atof_buf); // @v10.7.9 dconvstr_scan-->satof
					//SLTEST_CHECK_NZ(satof(atof_buf, &v_satof));
					SLTEST_CHECK_EQ(v_satof, v_atof);
				}
			}
			{
				SLTEST_CHECK_EQ(str.Z().Cat(0.1, MKSFMTD(0, 2, 0)), "0.10");
				SLTEST_CHECK_EQ(str.Z().Cat(17.1997, MKSFMTD(0, 3, NMBF_DECCOMMA)), "17,200");
				SLTEST_CHECK_EQ(str.Z().Cat(135.1997, MKSFMTD(0, 0, 0)), "135");
				SLTEST_CHECK_EQ(str.Z().Cat(3308.04, MKSFMTD(0, 8, NMBF_OMITEPS|NMBF_NOTRAILZ)), "3308.04");
				SLTEST_CHECK_EQ(str.Z().Cat(3308.039999999999512506, MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "3308.04");
				SLTEST_CHECK_EQ(str.Z().Cat(3308.04, MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "3308.04");
				SLTEST_CHECK_EQ(str.Z().Cat(2572.92*1.00000000001, MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "2572.92");
				SLTEST_CHECK_EQ(str.Z().Cat(369.900000000000102333,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "369.9");
				SLTEST_CHECK_EQ(str.Z().Cat(369.900000000000102333,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ|NMBF_DECCOMMA)), "369,9");
				SLTEST_CHECK_EQ(str.Z().Cat(369.000000000001023334,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ)), "369");
				SLTEST_CHECK_EQ(str.Z().Cat(369.000000000001023334,  MKSFMTD(0, 13, NMBF_OMITEPS|NMBF_NOTRAILZ|NMBF_DELCOMMA)), "369");
			}
			{
				LDATETIME dtm;
				LDATETIME dtm_converted;
				dtm.d.encode(29, 2, 2016);
				dtm.t.encode(21, 17, 2, 250);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_DMY), "29/02/16");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_DMY), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_DMY|DATF_CENTURY), "29/02/2016");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_DMY), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_GERMANCENT), "29.02.2016");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_GERMAN|DATF_CENTURY), "29.02.2016"); // Проверяем эквивалентность DATF_GERMANCENT==DATF_GERMAN|DATF_CENTURY
				SLTEST_CHECK_EQ(strtodate_(str, DATF_GERMAN), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_AMERICAN|DATF_CENTURY), "02/29/2016");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_AMERICAN), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_ANSI|DATF_CENTURY), "2016.02.29");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_ANSI), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_ITALIAN|DATF_CENTURY), "29-02-2016");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_ITALIAN), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_ITALIAN), "29-02-16");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_ITALIAN), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_JAPAN), "16/02/29");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_JAPAN), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_ISO8601CENT), "2016-02-29");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_ISO8601), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_ISO8601), "16-02-29");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_ISO8601), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_AMERICAN|DATF_CENTURY|DATF_NODIV), "02292016");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_AMERICAN|DATF_CENTURY|DATF_NODIV), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_DMY|DATF_NODIV), "290216");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_DMY|DATF_NODIV), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_SQL), "DATE '2016-02-29'");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_SQL), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.d, DATF_INTERNET), "Mon, 29 Feb 2016");
				// (Такой формат не распознается) SLTEST_CHECK_EQ(strtodate_(str, DATF_INTERNET), dtm.d);
				SLTEST_CHECK_EQ(str.Z().Cat(ZERODATE, DATF_DMY), "");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_DMY), ZERODATE);
				SLTEST_CHECK_EQ(str.Z().Cat(ZERODATE, DATF_DMY|DATF_NOZERO), "");
				SLTEST_CHECK_EQ(strtodate_(str, DATF_DMY|DATF_NOZERO), ZERODATE);

				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_HMS), "21:17:02");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_HMS|TIMF_NODIV), "211702");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_HM), "21:17");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_HM|TIMF_NODIV), "2117");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_MS), "17:02");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_MS|TIMF_NODIV), "1702");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm.t, TIMF_HMS|TIMF_MSEC), "21:17:02.250");

				SLTEST_CHECK_EQ(str.Z().Cat(dtm, DATF_ISO8601CENT, 0), "2016-02-29T21:17:02");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm, DATF_ISO8601, 0), "16-02-29T21:17:02");
				SLTEST_CHECK_EQ(str.Z().Cat(dtm, DATF_MDY|DATF_CENTURY, TIMF_HMS|TIMF_MSEC), "02/29/2016 21:17:02.250");
				//
				strtotime("211702", TIMF_HMS|TIMF_NODIV, &dtm_converted.t);
				SLTEST_CHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 0));
				strtotime("\t21:17:02    ", TIMF_HMS, &dtm_converted.t);
				SLTEST_CHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 0));
				strtotime("      21:17:02.047   ", TIMF_HMS, &dtm_converted.t);
				SLTEST_CHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 4));
				strtotime(" 21 17 02 ", TIMF_HMS, &dtm_converted.t);
				SLTEST_CHECK_EQ(dtm_converted.t, encodetime(21, 17, 02, 0));
				strtotime(" 2117 ", TIMF_HM|TIMF_NODIV, &dtm_converted.t);
				SLTEST_CHECK_EQ(dtm_converted.t, encodetime(21, 17, 00, 0));
			}
			{
				IntRange ir;
				int    r;
				{
					r = str.Z().ToIntRange(ir, SString::torfDoubleDot);
					SLTEST_CHECK_Z(r);
				}
				{
					r = (str = "12..987").ToIntRange(ir, SString::torfDoubleDot);
					SLTEST_CHECK_NZ(r);
					SLTEST_CHECK_EQ(r, (int)str.Len());
					SLTEST_CHECK_EQ(ir.low, (int32)12);
					SLTEST_CHECK_EQ(ir.upp, (int32)987);
				}
				{
					// Здесь .. не будет распознано как разделитель границ (SString::torfHyphen)
					r = (str = "12..987").ToIntRange(ir, SString::torfHyphen);
					SLTEST_CHECK_NZ(r);
					SLTEST_CHECK_EQ(r, 2);
					SLTEST_CHECK_EQ(ir.low, (int32)12);
					SLTEST_CHECK_EQ(ir.upp, (int32)12);
				}
				{
					r = (str = " -12 : 987q").ToIntRange(ir, SString::torfAny);
					SLTEST_CHECK_NZ(r);
					SLTEST_CHECK_EQ(r, (int)(str.Len()-1));
					SLTEST_CHECK_EQ(ir.low, (int32)-12);
					SLTEST_CHECK_EQ(ir.upp, (int32)987);
				}
				{
					r = (str = " -12,,-987 z").ToIntRange(ir, SString::torfAny);
					SLTEST_CHECK_NZ(r);
					SLTEST_CHECK_EQ(r, (int)(str.Len()-2));
					SLTEST_CHECK_EQ(ir.low, (int32)-12);
					SLTEST_CHECK_EQ(ir.upp, (int32)-987);
				}
				{
					r = (str = " -12000 --987 z").ToIntRange(ir, SString::torfAny);
					SLTEST_CHECK_NZ((long)r);
					SLTEST_CHECK_EQ(r, (int)(str.Len()-2));
					SLTEST_CHECK_EQ(ir.low, (int32)-12000);
					SLTEST_CHECK_EQ(ir.upp, (int32)-987);
				}
				{
					SLTEST_CHECK_Z((str = " &12").ToIntRange(ir, SString::torfAny));
				}
				{
					r = (str = " +19 ").ToIntRange(ir, SString::torfAny);
					SLTEST_CHECK_NZ(r);
					SLTEST_CHECK_EQ(r, (int)(str.Len()-1));
					SLTEST_CHECK_EQ(ir.low, (int32)19);
					SLTEST_CHECK_EQ(ir.upp, (int32)19);
				}
				{
					r = (str = " -1234567890 ").ToIntRange(ir, SString::torfAny);
					SLTEST_CHECK_NZ(r);
					SLTEST_CHECK_EQ(r, (int)(str.Len()-1));
					SLTEST_CHECK_EQ(ir.low, (int32)-1234567890);
					SLTEST_CHECK_EQ(ir.upp, (int32)-1234567890);
				}
			}
		}
		{
			// @v11.7.6 Тестирование функций ReadQuotedString и WriteQuotedString
			SString revert_buf;
			size_t end_pos = 0;
			{
				str.Z();
				const char * p_text = "\"abc def\"";
				{
					ReadQuotedString(p_text, sstrlen(p_text), 0, &end_pos, str);
					SLTEST_CHECK_EQ(str, "abc def");
					SLTEST_CHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), 0, revert_buf);
					SLTEST_CHECK_EQ(revert_buf, p_text);
				}
				{
					ReadQuotedString(p_text, sstrlen(p_text), QSF_APPEND, &end_pos, str);
					SLTEST_CHECK_EQ(str, "abc def""abc def");
					SLTEST_CHECK_EQ(end_pos, sstrlen(p_text));
				}
			}
			{
				const char * p_text = "\"abc \"\" \\t def\"";
				{
					ReadQuotedString(p_text, sstrlen(p_text), QSF_DBLQ, &end_pos, str);
					SLTEST_CHECK_EQ(str, "abc \" \\t def");
					SLTEST_CHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), QSF_DBLQ, revert_buf);
					SLTEST_CHECK_EQ(revert_buf, p_text);
				}
			}
			{
				const char * p_text = "\"abc \\\" \\t def\"";
				{
					ReadQuotedString(p_text, sstrlen(p_text), QSF_ESCAPE, &end_pos, str);
					SLTEST_CHECK_EQ(str, "abc \" \t def");
					SLTEST_CHECK_EQ(end_pos, sstrlen(p_text));
					WriteQuotedString(str, str.Len(), QSF_ESCAPE, revert_buf);
					SLTEST_CHECK_EQ(revert_buf, p_text);
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
				SLTEST_CHECK_NZ(sstreq(buffer1, buffer2));
				SLTEST_CHECK_Z(strcmp(buffer1, buffer2));
				{
					for(uint offs = 0; offs < 8; offs++) {
						const char * p_buf1 = buffer1+offs;
						const char * p_buf2 = buffer2+offs;
						for(int j = 0; j < 256; j++) {
							const char * p1 = (const char *)memchr(p_buf1, j, proof_len+1-offs);
							const char * p2 = (const char *)xeos_memchr(p_buf2, j, proof_len+1-offs);
							SLTEST_CHECK_NZ((!p1 && !p2) || ((p1 && p2) && (p1-p_buf1) == (p2-p_buf2)));
						}
					}
				}
			}
			SLTEST_CHECK_EQ(total_len1, total_len2);
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
			SLTEST_CHECK_EQ(out_buf, p_result_xmlenc);
		}
		{
			STokenRecognizer tr;
			SNaturalTokenStat nts;
			SNaturalTokenArray nta;
			tr.Run((const uchar *)"0123", -1, nta.Z(), &nts); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			tr.Run((const uchar *)"4610017121115", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_EAN13));
			tr.Run((const uchar *)"4610017121116", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_EQ(0.0f, nta.Has(SNTOK_EAN13)); // Инвалидная контрольная цифра
			tr.Run((const uchar *)"20352165", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_EAN8));
			tr.Run((const uchar *)"100100802804", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_RU_INN));
			// @v10.8.1 {
			tr.Run((const uchar *)"47296611", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"99057850", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"6z3417681", -1, nta.Z(), 0); 
			SLTEST_CHECK_EQ(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_EQ(0.0f, nta.Has(SNTOK_RU_OKPO));
			tr.Run((const uchar *)"6993417681", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_EQ(0.0f, nta.Has(SNTOK_RU_OKPO));
			// } @v10.8.1 
			tr.Run((const uchar *)"0034012000001472206", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_EGAISWARECODE));
			tr.Run((const uchar *)"a98P8s00W", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGLAT));
			tr.Run((const uchar *)"4FC737F1-C7A5-4376-A066-2A32D752A2FF", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_GUID));
			tr.Run((const uchar *)"mail.123@gogo-fi.com", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_EMAIL));
			// @notimplemented tr.Run((const uchar *)"123-15-67", -1, nta.Z(), 0); 
			// @notimplemented SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_PHONE));
			// @notimplemented tr.Run((const uchar *)"+7(911)123-15-67", -1, nta.Z(), 0); 
			// @notimplemented SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_PHONE));
			tr.Run((const uchar *)"354190023896443", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DIGITCODE));
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_IMEI));
			tr.Run((const uchar *)"192.168.0.1", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_IP4));
			tr.Run((const uchar *)"1/12/2018", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DATE));
			tr.Run((const uchar *)"20180531", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_DATE));
			// @notimplemented tr.Run((const uchar *)"11:30", -1, nta.Z(), 0); 
			// @notimplemented SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_TIME));
			// @notimplemented tr.Run((const uchar *)"11:30:46", -1, nta.Z(), 0); 
			// @notimplemented SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_TIME));
			tr.Run((const uchar *)"10.2.5", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_SOFTWAREVER));
			tr.Run((const uchar *)"#000000", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_COLORHEX));
			tr.Run((const uchar *)"#f82aB7", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_COLORHEX));

			tr.Run((const uchar *)"+100,000.00", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_DOT));
			tr.Run((const uchar *)"+100 000,00", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_COM));
			tr.Run((const uchar *)"-100'000,00", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_COM));
			tr.Run((const uchar *)"9", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_NUMERIC_DOT));

			tr.Run((const uchar *)"00000046209443j+Q\'?P5ACZAC8bG", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"00000046209443x-8xfgOACZAYGfv", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"04606203098187o&zWeIyABr8l/nT", -1, nta.Z(), 0); 
			SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"04606203098187o&zWe\x81yABr8l/nT", -1, nta.Z(), 0); // !SNTOK_CHZN_CIGITEM
			SLTEST_CHECK_EQ(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			tr.Run((const uchar *)"00000%46209443x-8xfgOACZAYGfv", -1, nta.Z(), 0); // !SNTOK_CHZN_CIGITEM
			SLTEST_CHECK_EQ(0.0f, nta.Has(SNTOK_CHZN_CIGITEM));
			{
				static const char * p_cl_rut_data_list[] = {
					"9007920-4", 
					"21620312-7", 
					"13621690-2", 
					"9329827-6", 
					"5946647-k", 
					"5946647-K", // upper K
					"7425273-7", 
					"17694763-2", 
					"23212441-5", 
					"21485432-5", 
					"15459172-9",
					"154591729", // no hyphen
					"10218932-9", 
					"11316657-6", 
					"24130358-6", 
					"11377848-2", 
					"18609823-4", 
					"18004377-2",
					"180.043.77-2",
					"8784472-2", 
					"12357399-4",
					"12391279-9", 
					"8304218-4"
				};
				for(size_t clrutidx = 0; clrutidx < SIZEOFARRAY(p_cl_rut_data_list); clrutidx++) {
					tr.Run((const uchar *)p_cl_rut_data_list[clrutidx], -1, nta.Z(), 0); // !SNTOK_CHZN_CIGITEM
					SLTEST_CHECK_LT(0.0f, nta.Has(SNTOK_CL_RUT));
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
			SLTEST_CHECK_NZ(ps.Invariant(&ip));
			out_file.WriteLine(ip.MsgBuf.CR());
		}
	}
	return CurrentStatus;
}

