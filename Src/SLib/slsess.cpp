// SLSESS.CPP
// Copyright (c) A.Sobolev 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <ShlObj_core.h> // for the express-test of SCOMOBJRELEASE()
// htmlhelp.h {
#define HH_INITIALIZE            0x001C  // Initializes the help system.
#define HH_UNINITIALIZE          0x001D  // Uninitializes the help system.
// } htmlhelp.h
//
// @v11.7.4 const SlConstParam _SlConst_Removed;
//
//
// 
/* Эксперименты с выравниванием структур
#pragma pack(show)

#include <sl-packing-set08.h>
#pragma pack(show)

static void test_alignment() { struct A { byte X; uint32 Y; }; static_assert(sizeof(A) == 8); }

#include <sl-packing-reset.h>
#include <sl-packing-set01.h>
#pragma pack(push, 16)
#pragma pack(show)

static void test_alignment2() { struct A { byte X; uint32 Y; }; static_assert(sizeof(A) == 8); }

#pragma pack(pop)
#pragma pack(push, 1)
#pragma pack(show)

static void test_alignment3() { struct A { byte X; uint32 Y; }; static_assert(sizeof(A) == 5); }

#pragma pack(pop)
#pragma pack(push)
#pragma pack(show)

static void test_alignment4() { struct A { byte X; uint32 Y; }; static_assert(sizeof(A) == 5); }

#pragma pack(pop)
#pragma pack(show)

static void foo()
{
	#pragma pack(push, 8)
	struct A {
		byte X;
		uint32 Y;
	};
	const size_t s8 = sizeof(A);
	#pragma pack(pop)
	#pragma pack(push, 1)
	const size_t s8_2 = sizeof(A);
	static_assert(s8 == s8_2);
	#pragma pack(pop)
}
*/
//
//
//
SlExtraProcBlock::SlExtraProcBlock()
{
	Reset();
}

void SlExtraProcBlock::Reset()
{
	F_LoadString = 0;
	F_ExpandString = 0;
	F_CallHelp = 0;
	F_CallCalc = 0;
	F_CallCalendar = 0;
	F_GetGlobalSecureConfig = 0;
	F_GetDefaultEncrKey = 0;
	F_QueryPath = 0;
}

void SlExtraProcBlock::Set(const SlExtraProcBlock * pS)
{
	if(pS) {
		F_LoadString = pS->F_LoadString;
		F_ExpandString = pS->F_ExpandString;
		F_CallHelp = pS->F_CallHelp;
		F_CallCalc = pS->F_CallCalc;
		F_CallCalendar = pS->F_CallCalendar;
		F_GetGlobalSecureConfig = pS->F_GetGlobalSecureConfig;
		F_GetDefaultEncrKey = pS->F_GetDefaultEncrKey;
		F_QueryPath = pS->F_QueryPath;
	}
	else
		Reset();
}
//
//
//
SlThreadLocalArea::SlThreadLocalArea() : Prf(1), Id(0), LastErr(0), LastOsErr(0), LastSockErr(0), LastCurlErr(0),
	BinDateFmt_(SlConst::DefaultBinDateFormat), TxtDateFmt_(SlConst::DefaultTxtDateFormat), CurrentCp(cpUndef), UiFlags(0), UiLanguageId(-1),
	SAry_OrgFCMP(0), SAry_PtrContainer(0), SAry_SortExtraData(0), FontDc(0), P_Rez(0), RvlSStA(1024), RvlSStW(1024)
{
	const LDATETIME now_time = getcurdatetime_();
	{
		DefaultYear_  = now_time.d.year();
		DefaultMonth_ = now_time.d.month();
	}
	Rg.Set(now_time.d.v ^ now_time.t.v);
	NextDialogLuPos.Set(-1, -1);
}

SlThreadLocalArea::~SlThreadLocalArea()
{
	Destroy();
}

void SlThreadLocalArea::Destroy()
{
	ZDELETE(P_Rez);
	::DeleteDC(FontDc);
	// @v11.0.0 (see comments in SlSession::ReleaseThread()) RemoveTempFiles(true);
}

TVRez * SlThreadLocalArea::GetRez()
{
	if(!P_Rez) {
		SString name;
		makeExecPathFileName("pp", "res", name);
		if(fileExists(name))
			P_Rez = new TVRez(name, 1);
	}
	return P_Rez;
}

void SlThreadLocalArea::SetNextDialogLuPos(int left, int top)
{
	NextDialogLuPos.Set(left, top);
}

SPoint2S SlThreadLocalArea::GetNextDialogLuPos()
{
	SPoint2S result = NextDialogLuPos;
	NextDialogLuPos.Set(-1, -1);
	return result;
}

HDC SlThreadLocalArea::GetFontDC()
{
	if(!FontDc) {
		FontDc = ::CreateCompatibleDC(0);
		if(FontDc && !::SetGraphicsMode(FontDc, GM_ADVANCED)) {
			::DeleteDC(FontDc);
			FontDc = 0;
		}
	}
	return FontDc;
}

int SlThreadLocalArea::RegisterTempFileName(const char * pFileName)
{
	return isempty(pFileName) ? -1 : TempFileList.add(pFileName);
}

void SlThreadLocalArea::RemoveTempFiles(bool dontStoreFailedItems)
{
	SString file_name;
	StringSet temp_list;
	for(uint i = 0; TempFileList.get(&i, file_name);) {
		if(!SFile::Remove(file_name) && !dontStoreFailedItems) {
			temp_list.add(file_name);
		}
	}
	if(dontStoreFailedItems)
		TempFileList.Z();
	else
		TempFileList = temp_list;
}

SlSession::SlSession() : SSys(1), Id(1), TlsIdx(-1), StopFlag(0), P_StopEvnt(0), DragndropObjIdx(0), GlobSymbList(512, 0),
	WsaInitCounter(0), HelpCookie(0), UiLanguageId(0), SessUuid(SCtrGenerate()) /* Генерируем абсолютно уникальный id сессии */
{
	assert((void *)&TlsIdx == (void *)this); // TlsIdx - @firstmember
#if(USE_ASMLIB > 0)
	{
		//
		// Перед началом исполнения программы сделаем вызовы функций из библиотеки ASMLIB для того,
		// чтобы они сразу инициализировали внутренние таблицы, зависящие от процессора.
		// Таким образом, мы избежим риска конфликтов при многопоточном исполнении.
		//
		const  size_t S = 128;
		char   temp_buf1[S], temp_buf2[S];
		A_memset(temp_buf1, 0, S);
		A_memset(temp_buf2, 0, S);
		A_memmove(temp_buf1, temp_buf2, S);
		A_memcpy(temp_buf2, temp_buf1, S);
		A_memset(temp_buf2, '1', S/4);
		temp_buf1[0] = 0;
	}
#endif
	ExtraProcBlk.Reset();
	TlsIdx = TlsAlloc();
	InitThread();
}

SlSession::~SlSession()
{
	if(ExtraProcBlk.F_CallHelp && HelpCookie) {
		ExtraProcBlk.F_CallHelp(0, HH_UNINITIALIZE, HelpCookie);
	}
	GlobObjList.Destroy(); // @v11.3.2 instead of automatic destruction
	ReleaseThread();
	TlsFree(TlsIdx);
	delete P_StopEvnt;
	for(int i = 0; i < WsaInitCounter; i++)
		WSACleanup();
#ifndef __GENERIC_MAIN_CONDUIT__
	ShutdownGdiplus();
#endif
}

int SlSession::CheckStopFlag() const
{
	return BIN(StopFlag);
}

int SlSession::Stop()
{
	int    ok = -1;
	if(!StopFlag) {
		ENTER_CRITICAL_SECTION
		if(!StopFlag) {
			StopFlag = 1;
			CALLPTRMEMB(P_StopEvnt, Signal());
			ok = 1;
		}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

int SlSession::ResetStopState()
{
	int    ok = -1;
	if(StopFlag) {
		ENTER_CRITICAL_SECTION
		if(StopFlag) {
			StopFlag = 0;
			if(P_StopEvnt) {
				P_StopEvnt->Reset();
				ZDELETE(P_StopEvnt);
			}
			ok = 1;
		}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

bool FASTCALL IsMadeOfEightDigitsFast(const uint8 * pS);

static void InitTest()
{
#ifndef NDEBUG // {
	{
		//
		// @paranoic (Эта проверка нужна мне для успокоения, ибо меня преследует фобия, что такое равенство не выполняется)
		//
		char   temp_buf[32];
		STATIC_ASSERT((void *)temp_buf == (void *)&temp_buf);
	}
	{
		//#pragma pack(show)
		//
		// Проверяем работоспособность *& для присваивания указателя по ссылке.
		//
		struct LocalData {
			LocalData(int a, uint8 c, double b) : A(a), C(c), B(b)
			{
			}
			int    A;
			uint8  C; // @v11.7.0
			double B;
		};

		class LocalBlock {
		public:
			static void Func1(LocalData *& prData)
			{
				prData = new LocalData(1, 15, 10.0);
			}
		};
		LocalData * p_data = 0;
		LocalBlock::Func1(p_data);
		assert(p_data != 0 && p_data->A == 1 && p_data->B == 10.0 && p_data->C == 15);
		ZDELETE(p_data);
		assert(p_data == 0);
		//
		// @v11.7.0
		// Кроме того, проверяем, чтобы код компилировался с выравниванием по 1 байту
		// 
		STATIC_ASSERT(offsetof(LocalData, A) == 0 && offsetof(LocalData, C) == 4 && offsetof(LocalData, B) == 5);
		STATIC_ASSERT(sizeof(LocalData) == 13);
	}
	{
		//
		// Проверка компилятора не предмет однозначного равенства результатов сравнения 0 или 1.
		//
		int    ix;
		double rx;
		void * p_x = 0;
		ix = 0;
		assert((ix == 0) == 1);
		assert((ix != 0) == 0);
		assert((ix > 0) == 0);
		assert((ix <= 0) == 1);
		ix = 93281;
		assert((ix == 93281) == 1);
		assert((ix != 93281) == 0);
		rx = 0.0;
		assert((rx == 0) == 1);
		assert((rx != 0) == 0);
		rx = 17.5;
		assert((rx == 17.5) == 1);
		assert((rx != 17.5) == 0);
		p_x = 0;
		assert((p_x == 0) == 1);
		assert((p_x != 0) == 0);
		p_x = &rx;
		assert((p_x == &rx) == 1);
		assert((p_x != &rx) == 0);
		// @v10.9.3 {
		STATIC_ASSERT(BIN(17) == 1); // must be 1
		STATIC_ASSERT(BIN(0) == 0);
		#if(_MSC_VER > 1400) // @v10.9.10 Выясняется, что BIN нельзя применять к floating point. Например, Visual Studio 7.1 ошибается в такой конструкции.
			STATIC_ASSERT(BIN(0.0) == 0);
			STATIC_ASSERT(BIN(0.0f) == 0); // @v10.9.10
		#endif
		STATIC_ASSERT(!0 == 1);
		STATIC_ASSERT(!17 == 0);
		STATIC_ASSERT(LOGIC(!0.000001) == false);
		STATIC_ASSERT(LOGIC(!0.00000) == true);
		// } @v10.9.3 
	}
	{
		// @paranoic (Защита от классической шутки)
		STATIC_ASSERT(TRUE == 1);
		STATIC_ASSERT(FALSE == 0);
		STATIC_ASSERT(true == 1);
		STATIC_ASSERT(false == 0);
		STATIC_ASSERT(GENDER_MALE == 1);
		STATIC_ASSERT(GENDER_FEMALE == 2);
		STATIC_ASSERT(GENDER_QUESTIONING == 3);
		STATIC_ASSERT(AGGRFUNC_COUNT == 1);
		STATIC_ASSERT(AGGRFUNC_SUM == 2);
		STATIC_ASSERT(AGGRFUNC_AVG == 3);
		STATIC_ASSERT(AGGRFUNC_MIN == 4);
		STATIC_ASSERT(AGGRFUNC_MAX == 5);
		STATIC_ASSERT(AGGRFUNC_STDDEV == 6);
		STATIC_ASSERT(SlConst::SecsPerDay == (24 * 60 * 60)); // @v11.4.7 То же кто-то может пошутить :)
	}
	{
		void * ptr = SAlloc::M(0);
		assert(ptr != 0);
		SAlloc::F(ptr);
		//
		ptr = SAlloc::M(4);
		assert(ptr != 0);
		SAlloc::F(ptr);
	}
	{
		//
		// Тестирование макроса SETIFZ
		//
		int    a = 1;
		SETIFZQ(a, 2);
		assert(a == 1);
		a = 0;
		SETIFZQ(a, 2);
		assert(a == 2);
		{
			void * ptr = 0;
			if(SETIFZ(ptr, SAlloc::M(128))) {
				assert(ptr != 0);
			}
			else {
				assert(ptr == 0);
			}
			ZFREE(ptr);
			//
			const char * p_abc = "abc";
			ptr = (void *)p_abc;
			if(SETIFZ(ptr, SAlloc::M(128))) { // Memory hasn't been allocated (ptr != 0)
				assert(ptr == p_abc);
			}
			else {
				assert(ptr == 0);
			}
			ptr = 0;
			p_abc = 0;
			if(SETIFZ(ptr, (void *)p_abc)) {
				assert(0);
			}
			else {
				assert(ptr == p_abc);
			}
		}
	}
	{
		//
		// Удостоверяемся в том, что SIZEOFARRAY работает правильно (тоже фобия)
		//
		struct TestStruc {
			const char * P_S;
			int16  I16;
		};
		TestStruc test_array[] = {
			{ "Abc", 1 },
			{ "Ab2", 2 },
			{ "Ab3", 3 },
			{ "Ab4", 4 },
			{ "Ab5", 5 }
		};
		STATIC_ASSERT(SIZEOFARRAY(test_array) == 5);
		STATIC_ASSERT(sizeofarray(test_array) == 5); // @v11.7.5
	}
	STATIC_ASSERT(sizeof(bool) == 1);
	STATIC_ASSERT(sizeof(char) == 1);
	STATIC_ASSERT(sizeof(int) == 4);
	STATIC_ASSERT(sizeof(uint) == 4);
	STATIC_ASSERT(sizeof(short) >= 2); // @v10.2.3
	STATIC_ASSERT(sizeof(long)  >= 4); // @v10.2.3
	STATIC_ASSERT(sizeof(int) >= sizeof(short)); // @v10.2.3
	STATIC_ASSERT(sizeof(long) >= sizeof(int)); // @v10.2.3
	STATIC_ASSERT(sizeof(int8) == 1);
	STATIC_ASSERT(sizeof(uint8) == 1);
	STATIC_ASSERT(sizeof(int16) == 2);
	STATIC_ASSERT(sizeof(uint16) == 2);
	STATIC_ASSERT(sizeof(int32) == 4);
	STATIC_ASSERT(sizeof(uint32) == 4);
	STATIC_ASSERT(sizeof(int64) == 8);
	STATIC_ASSERT(sizeof(uint64) == 8);
	STATIC_ASSERT(sizeof(float) == 4); // @v10.7.11
	STATIC_ASSERT(sizeof(double) == 8); // @v10.7.11
	STATIC_ASSERT(sizeof(S_GUID) == 16);
	STATIC_ASSERT(sizeof(S_GUID) == sizeof(S_GUID_Base));
	STATIC_ASSERT(sizeof(SColorBase) == 4); // @v10.2.4
	STATIC_ASSERT(sizeof(SColor) == sizeof(SColorBase)); // @v10.2.4
	STATIC_ASSERT(sizeof(IntRange) == 8);
	STATIC_ASSERT(sizeof(RealRange) == 16);
	STATIC_ASSERT(sizeof(DateRange) == 8); // @v10.2.4
	STATIC_ASSERT(sizeof(TimeRange) == 8); // @v10.2.4
#ifdef _M_X64
	STATIC_ASSERT(sizeof(SBaseBuffer) == 16);
#else
	STATIC_ASSERT(sizeof(SBaseBuffer) == 8);
#endif
	STATIC_ASSERT(sizeof(DateRepeating) == 8);
	STATIC_ASSERT(sizeof(DateTimeRepeating) == 12);
	STATIC_ASSERT(sizeof(WorkDate) == 2); // @v11.7.0
	STATIC_ASSERT(sizeof(SUnicodeBlock::StrgHeader) == 32); // @v11.7.0
	STATIC_ASSERT(sizeof(LMatrix2D) == 48); // @v11.7.0
	//
	STATIC_ASSERT(sizeof(TYPEID) == 4);
	STATIC_ASSERT(sizeof(STypEx) == 16);
	STATIC_ASSERT(sizeof(CommPortParams) == 6);
	// 
	// @v11.2.0 {
	STATIC_ASSERT(sizeof(SPoint2I) == sizeof(POINT));
	STATIC_ASSERT(offsetof(SPoint2I, x) == offsetof(POINT, x));
	STATIC_ASSERT(offsetof(SPoint2I, y) == offsetof(POINT, y));
	{
		uint ff = 0;
		{
			SETFLAG(ff, 0x08000000, true);
			assert((ff & 0x08000000));
			SETFLAG(ff, 0x08000000, false);
			assert(!(ff & 0x08000000));
		}
	}
	// } @v11.2.0 
	STATIC_ASSERT(sizeof(SPoint2S) == sizeof(4)); // @v11.7.0
	STATIC_ASSERT(sizeof(MACAddr) == 6); // @v11.7.0
	STATIC_ASSERT(sizeof(KeyDownCommand) == 4); // @v11.7.0
	STATIC_ASSERT(sizeof(SUiLayout::Result) == (24+sizeof(void *))); // @v11.7.0
	// @v11.4.8 {
	{
		// Убеждаемся в том, что memset(mem, 0xff, size) заполнит весь отрезок битовыми единицами
		// Сомнения существуют из-за того, что аргумент функции int а передаем только один байт (0xff).
		uint8 chunk[379];
		memset(chunk, 0xff, sizeof(chunk));
		for(uint i = 0; i < sizeof(chunk); i++)
			assert(chunk[i] == static_cast<uint8>(0xff));
	}
	// } @v11.4.8 
	STATIC_ASSERT(MAX(3.1, 8.5) == 8.5);
	assert(smax(3.1, 8.5) == 8.5);
	assert(smax(3.1f, 8.5f) == 8.5f);
	STATIC_ASSERT(MIN(1.5, -7.3) == -7.3);
	assert(smin(1.5, -7.3) == -7.3);
	assert(smin(1.5f, -7.3f) == -7.3f);
	assert(smin(1, 2) == 1);
	assert(smax(-5, 5) == 5);
	assert(smax(-5U, 5U) == -5U);
	assert(smin(-5U, 5U) == 5U);
	assert(smax(0.00001, 0.0000101) == 0.0000101);
	assert(smax(0.00001f, 0.0000101f) == 0.0000101f);
	assert(smin('a', 'A') == 'A');
	assert(smin('A', 'a') == 'A');
	assert(smax('z', 'Z') == 'z');
	assert(smax('Z', 'z') == 'z');
	assert(smax(100L, 100L) == smin(100L, 100L));
	STATIC_ASSERT(MIN(1.00175120103, 1.00175120103) == 1.00175120103);
	STATIC_ASSERT(MAX(1.00175120103, 1.00175120103) == 1.00175120103);
	{
		const long test_dword = 0x1234befa;
		STATIC_ASSERT(MakeLong(LoWord(test_dword), HiWord(test_dword)) == test_dword);
	}
	{
		//
		// Проверка макроса SETIFZ для даты
		//
		const LDATE cdt = getcurdate_();
		LDATE dt = ZERODATE;
		SETIFZ(dt, cdt);
		assert(dt == cdt);
		dt = encodedate(7, 11, 2017);
		SETIFZ(dt, cdt);
		assert(dt != cdt);
	}
	{
		//
		// Проверка макроса SETIFZ для LDATETIME
		//
		const LDATETIME cdtm = getcurdatetime_();
		LDATETIME dtm = ZERODATETIME;
		SETIFZ(dtm, cdtm);
		assert(dtm == cdtm);
		dtm.d = encodedate(7, 11, 2017);
		dtm.t = encodetime(12, 25, 58, 9);
		SETIFZ(dtm, cdtm);
		assert(dtm != cdtm);
	}
	// @v10.7.9 {
	{
		{
			SString temp_buf;
			StringSet ss(';', "1;2");
			ss.get(0U, temp_buf);
			assert(temp_buf == "1");
		}
		{
			SString temp_buf;
			StringSet(';', "1;2").get(0U, temp_buf);
			assert(temp_buf == "1");
		}
	}
	// } @v10.7.9 
	// @v10.9.3 {
	assert(ismemzero(&ZEROGUID, sizeof(ZEROGUID))); 
	assert(isasciialpha('A') != 0);
	assert(isasciialpha('z') != 0);
	assert(isasciialpha('U') != 0);
	assert(isasciialpha('d') != 0);
	assert(isasciialpha('Z'+1) == 0);
	assert(isasciialpha('A'-1) == 0);
	assert(isasciialpha('z'+1) == 0);
	assert(isasciialpha('a'-1) == 0);
	// } @v10.9.3 
	// assert(sizeof(SUiLayout::Result) == 24); // @v11.0.0
	{
		//bool FASTCALL IsMadeOfEightDigitsFast(const uint8 * pS);
		assert(IsMadeOfEightDigitsFast(PTR8C("00000000")));
		assert(!IsMadeOfEightDigitsFast(PTR8C("00000z00")));
		/*{
			uint8   random_buf[1024];
			for(uint i = 0; i < SIZEOFARRAY(random_buf); i++) {
			
			}
			for(uint offs = 0; offs < 800; offs++) {
			
			}
		}*/
	}
	{
		//void * ptr = SAlloc::M(0);
		//assert(ptr == 0);
#if(_MSC_VER >= 1900)
		_aligned_free(0); // Рассчитываем, что никаких последствий не будет
#endif
		SAlloc::F(0); // Рассчитываем, что никаких последствий не будет
	}
	// @v11.2.3 {
	{
		//
		// Проверка работоспособности передачи временного строкового объекта в качестве параметра в функцию
		// с целью убедиться, что указатель целый и невредимый будет доставлен в функцию (здесь вопрос
		// в порядке разрушения объекта).
		//
		class InnerBlock {
		public:
			static void StringFunc(const char * pInput, SString & rOutput)
			{
				rOutput.Z().Cat(pInput).Space().CatChar('A');
			}
		};
		SString result("B");
		InnerBlock::StringFunc(SString("00000000").Space().Cat("00000z00").Space().Cat(7L), result);
		// Temporary obj SString("00000000") has been destroyed here and func InnerBlock::StringFunc can operate it safely
		assert(result == "00000000 00000z00 7 A");
	}
	// } @v11.2.3
	// @v11.0.0 {
#if CXX_OS_WINDOWS != 0
	{
		//
		// Следующие проверки вызова некоторых очищающих функций WIN API нужны для того, чтобы убедиться,
		// что при обращении к ним с явно неопределенными аргументами ничего ужасного не случается.
		// Это важно так как я стараюсь элиминировать лишние проверки аргументов перед вызовами функций
		// (размер исходного кода морщим, знаете-ли).
		//
		// (Этот тест не работает на Win10, но работает на Win2012 server) assert(::CloseHandle(INVALID_HANDLE_VALUE) == 0);
		assert(::DestroyWindow(0) == 0);
		assert(::GlobalFree(0) == 0);
		assert(::LocalFree(0) == 0);
		assert(::RegCloseKey(0) != ERROR_SUCCESS);
		assert(::FreeLibrary(0) == 0);
		assert(::ReleaseDC(0, 0) == 0);
		assert(::DeleteDC(0) == 0);
		assert(::DeleteObject(0) == 0);
	}
#endif
	// } @v11.0.0 
	// @v11.3.8 {
	{
		//
		// Экспресс-тест json
		//
		SString js_buf;
		SJson js_in(SJson::tOBJECT);
		js_in.InsertString("string-key", "string-val");
		js_in.InsertBool("bool-key1", true);
		js_in.InsertBool("bool-key2", false);
		js_in.ToStr(js_buf);
		//
		{
			SJson * p_js_out = SJson::Parse(js_buf);
			assert(p_js_out);
			assert(p_js_out->IsObject());
			for(const SJson * p_node = p_js_out->P_Child; p_node; p_node = p_node->P_Next) {
				if(p_node->Text.IsEqiAscii("String-Key")) {
					assert(p_node->P_Child->IsString());
					assert(p_node->P_Child->Text.IsEqiAscii("string-val")); 
				}
				else if(p_node->Text.IsEqiAscii("bool-key1")) {
					assert(p_node->P_Child->IsTrue());
				}
				else if(p_node->Text.IsEqiAscii("bool-keY2")) {
					assert(p_node->P_Child->IsFalse());
				}
			}
			delete p_js_out;
		}
	}
	// } @v11.3.8
	{
		// Экспресс-тест функций setlowbits64 и setlowbits32
		{
			for(uint i = 1; i <= 32; i++) {
				uint32 v = setlowbits32(i);
				for(uint j = 0; j < 32; j++) {
					if(j < i)
						assert((v & (1 << j)) == (1 << j));
					else 
						assert((v & (1 << j)) == 0);
				}
			}
		}
		{
			for(uint i = 1; i <= 64; i++) {
				uint64 v = setlowbits64(i);
				for(uint j = 0; j < 64; j++) {
					if(j < i)
						assert((v & (1ULL << j)) == (1ULL << j));
					else 
						assert((v & (1ULL << j)) == 0);
				}
			}
		}
	}
	// @v11.7.4 {
	{
		// Экспресс-тест макроса SCOMOBJRELEASE
		IMalloc * p_malloc = 0;
		SCOMOBJRELEASE(p_malloc);
		assert(p_malloc == 0);
		SHGetMalloc(&p_malloc);
		assert(p_malloc != 0);
		SCOMOBJRELEASE(p_malloc);
		assert(p_malloc == 0);
	}
	// } @v11.7.4
	// @v11.8.1 {
	{
		union {
			uint8 RawData[256];
			struct {
				int8   I8;
				uint8  U8;
				int16  I16;
				uint16 U16;
				int    I;
				uint   U;
				long   L;
				ulong  UL;
				int64  II;
				uint64 UII;
			} TD;
		} U;
		SObfuscateBuffer(&U, sizeof(U));
		assert(SBits::Cpop((uint8)FFFF(U.TD.I8)) == 8);
		assert(SBits::Cpop(FFFF(U.TD.U8)) == 8);
		assert(SBits::Cpop((uint16)FFFF(U.TD.I16)) == 16);
		assert(SBits::Cpop(FFFF(U.TD.U16)) == 16);
		assert(SBits::Cpop((uint)FFFF(U.TD.I)) == 32);
		assert(SBits::Cpop(FFFF(U.TD.U)) == 32);
		assert(SBits::Cpop((ulong)FFFF(U.TD.L)) == 32);
		assert(SBits::Cpop(FFFF(U.TD.UL)) == 32);
		assert(SBits::Cpop((uint64)FFFF(U.TD.II)) == 64);
		assert(SBits::Cpop(FFFF(U.TD.UII)) == 64);
	}
	// } @v11.8.1
	{
		assert(log10i_floor(UINT64_MAX) == 19);
		assert(log10i_floor(UINT32_MAX) == 9);
		assert(log10i_floor(1ULL) == 0);
		assert(log10i_floor(1U) == 0);
		assert(log10i_floor(1000U) == 3);
		assert(log10i_floor(1000ULL) == 3);
		assert(log10i_floor(998U) == 2);
		assert(log10i_floor(998ULL) == 2);
	}
	{
		// Верификация константы, использующая один из вариантов представления (pthreads42 && bdb)
		STATIC_ASSERT(SlConst::Epoch1600_1970_Offs_100Ns == (((uint64_t)27111902UL << 32) + (uint64_t)3577643008UL));
		STATIC_ASSERT(SlConst::Epoch1600_1970_Offs_100Ns == SlConst::Epoch1600_1970_Offs_Mks * 10);
		STATIC_ASSERT(SlConst::Epoch1600_1970_Offs_100Ns == SlConst::Epoch1600_1970_Offs_s * 10000000);
	}
#endif // } NDEBUG
}

void SlSession::Init(const char * pAppName, HINSTANCE hInst)
{
	/*
	{
		//
		// Перед началом исполнения программы сделаем вызовы функций из библиотеки ASMLIB для того,
		// чтобы они сразу инициализировали внутренние таблицы, зависящие от процессора.
		// Таким образом, мы избежим риска конфликтов при многопоточном исполнении.
		//
		const  size_t S = 128;
		char   temp_buf1[S], temp_buf2[S];
		A_memset(temp_buf1, 0, S);
		A_memset(temp_buf2, 0, S);
		A_memmove(temp_buf1, temp_buf2, S);
		A_memcpy(temp_buf2, temp_buf1, S);
		A_memset(temp_buf2, '1', S/4);
		A_strlen(temp_buf2);
		A_strcpy(temp_buf1, temp_buf2);
		A_strcmp(temp_buf1, temp_buf2);
		A_stricmp(temp_buf1, temp_buf2);
		A_strstr(temp_buf1, "11");
	}
	*/
	H_Inst = NZOR(hInst, GetModuleHandle(0));
	// @v10.3.9 char   exe_path[MAX_PATH];
	// @v10.3.9 GetModuleFileName(H_Inst, exe_path, sizeof(exe_path)); // @unicodeproblem
	// @v10.3.9 ExePath = exe_path;
	SSystem::SGetModuleFileName(H_Inst, ExePath); // @v10.3.9
	AppName = pAppName;
	if(AppName.NotEmpty()) {
		SString n;
		P_StopEvnt = new Evnt(GetStopEventName(n), Evnt::modeCreate);
	}
	RegisterBIST();
	SFileFormat::Register();
	InitTest();
}

void SlSession::SetAppName(const char * pAppName)
{
	AppName = pAppName;
	if(!P_StopEvnt && AppName.NotEmpty()) {
		SString n;
		P_StopEvnt = new Evnt(GetStopEventName(n), Evnt::modeCreate);
	}
}

int SlSession::InitWSA()
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	WSADATA wsa_data;
	if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		ok = SetError(SLERR_SOCK_WINSOCK);
	else
		WsaInitCounter++;
	LEAVE_CRITICAL_SECTION
	return ok;
}

const void * SlSession::InitThread()
{
	SlThreadLocalArea * p_tla = new SlThreadLocalArea;
	TlsSetValue(TlsIdx, p_tla);
	p_tla->Id = GetCurrentThreadId();
	return static_cast<const void *>(p_tla);
}

void SlSession::ReleaseThread()
{
	SlThreadLocalArea * p_tla = static_cast<SlThreadLocalArea *>(TlsGetValue(TlsIdx));
	if(p_tla) {
		// @v11.0.0 {
		// Эта функция вызывалась внутри деструктора SlThreadLocalArea. Однако, если какого-то 
		// временного файла не существует, то внутренний вызов SFile::Remove инициирует ошибку
		// которая уже не может быть правильно обработана из-за полу-разрушенного экземпляра SlThreadLocalArea.
		// В результате сеанс аварийно завершается.
		//
		p_tla->RemoveTempFiles(true); 
		// } @v11.0.0 
		TlsSetValue(TlsIdx, 0);
		delete p_tla;
	}
}

/* (inlined) void * FASTCALL SGetTls(const long idx)
{
#ifdef NDEBUG
	return TlsGetValue(idx);
#else
	void * p = TlsGetValue(idx);
	if(p)
		return p;
	else {
		assert(0);
		return 0;
	}
#endif
}*/

// (inlined) SlThreadLocalArea & SlSession::GetTLA() { return *(SlThreadLocalArea *)SGetTls(TlsIdx); }
// (inlined) const SlThreadLocalArea & SlSession::GetConstTLA() const { return *(SlThreadLocalArea *)SGetTls(TlsIdx); }

bool SlSession::SetError(int errCode, const char * pAddedMsg)
{
	const int sock_err = (errCode == SLERR_SOCK_WINSOCK) ? WSAGetLastError() : 0;
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// @1 Если глобальный объект SLS разрушается раньше иных глобальных объектов,
		// которые могут вызвать SlSession::SetError, то при завершении процесса может возникнуть исключение
		// обращения к нулевому адресу. Во избежании этого проверяем &r_tla на 0.
		//
		r_tla.LastErr = errCode;
		r_tla.AddedMsgString = pAddedMsg;
		r_tla.LastSockErr = sock_err;
	}
	return false;
}

bool SlSession::SetError(int errCode, int addedMsgVal)
{
	const int sock_err = (errCode == SLERR_SOCK_WINSOCK) ? WSAGetLastError() : 0;
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// @1 Если глобальный объект SLS разрушается раньше иных глобальных объектов,
		// которые могут вызвать SlSession::SetError, то при завершении процесса может возникнуть исключение
		// обращения к нулевому адресу. Во избежании этого проверяем &r_tla на 0.
		//
		r_tla.LastErr = errCode;
		r_tla.AddedMsgString.Z().Cat(addedMsgVal);
		r_tla.LastSockErr = sock_err;
	}
	return false;
}

bool FASTCALL SlSession::SetErrorErrno(const char * pAddedMsg)
{
	return SetError(errno+SLERR_ERRNO_OFFSET, pAddedMsg);
}

bool FASTCALL SlSession::SetError(int errCode)
{
	const int sock_err = (errCode == SLERR_SOCK_WINSOCK) ? WSAGetLastError() : 0;
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// see @1 above
		//
		r_tla.LastErr = errCode;
		r_tla.AddedMsgString = 0;
		r_tla.LastSockErr = sock_err;
	}
	return false;
}

bool FASTCALL SlSession::SetOsError(const char * pAddedMsg)
{
	const int last_err = ::GetLastError();
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla) {
		//
		// see @1 above
		//
		r_tla.LastErr = SLERR_WINDOWS;
		r_tla.LastOsErr = last_err;
		r_tla.AddedMsgString = pAddedMsg;
	}
	return false;
}

int    SlSession::GetOsError() const { return GetConstTLA().LastOsErr; }
const  SString & SlSession::GetAddedMsgString() const { return GetConstTLA().AddedMsgString; }
const  SString & SlSession::GetExePath() const { return ExePath; }
const  SString & SlSession::GetAppName() const { return AppName; }
void   FASTCALL SlSession::SetAddedMsgString(const char * pStr) { GetTLA().AddedMsgString = pStr; }

void SlSession::SetUiLanguageId(int languageId, int currentThreadOnly)
{
	if(currentThreadOnly) {
		GetTLA().UiLanguageId = languageId;
	}
	else {
		ENTER_CRITICAL_SECTION
		UiLanguageId = languageId;
		LEAVE_CRITICAL_SECTION
	}
}

int  SlSession::GetUiLanguageId() const
{
	int    lid = GetConstTLA().UiLanguageId;
	return (lid < 0) ? UiLanguageId : lid;
}

SString & SlSession::GetStopEventName(SString & rBuf) const
{
	SString temp_buf;
	SessUuid.ToStr(S_GUID::fmtIDL, temp_buf);
	return rBuf.Z().Cat(AppName).CatChar('_').Cat("Stop").CatChar('_').Cat(temp_buf);
}

int    SlSession::RegisterTempFileName(const char * pFileName) { return GetTLA().RegisterTempFileName(pFileName); }
void   SlSession::RemoveTempFiles() { GetTLA().RemoveTempFiles(false); }
void   SlSession::SetLogPath(const char * pPath) { GetTLA().LogPath = pPath; }
SString & SlSession::GetLogPath(SString & rPath) const { return (rPath = GetConstTLA().LogPath); }
//
//
//
struct GlobalObjectEntry {
	void   FASTCALL operator = (SClassWrapper & rCls)
	{
		VT = *reinterpret_cast<void **>(&rCls);
	}
	int    Create()
	{
		if(VT) {
			uint8  stub[32];
			SClassWrapper * p_cls = reinterpret_cast<SClassWrapper *>(stub);
			*reinterpret_cast<void **>(p_cls) = VT;
			Ptr = p_cls->Create();
		}
		return (Ptr != 0);
	}
	void   Destroy()
	{
		if(VT && Ptr) {
			uint8  stub[32];
			SClassWrapper * p_cls = reinterpret_cast<SClassWrapper *>(stub);
			*reinterpret_cast<void **>(p_cls) = VT;
			p_cls->Destroy(Ptr);
		}
		Ptr = 0;
	}
	void * VT;
	void * Ptr;
};

SlSession::GlobalObjectArray::GlobalObjectArray() : SVector(sizeof(GlobalObjectEntry))
{
	//
	// Дабы не использовать нулевой индекс вставляем фиктивный первый элемент.
	//
	TSClassWrapper <int> zero_cls;
	GlobalObjectEntry zero_entry;
	zero_entry.VT = *reinterpret_cast<void **>(&zero_cls);
	zero_entry.Ptr = 0;
	insert(&zero_entry);
}

void SlSession::GlobalObjectArray::Destroy()
{
	Cs.Enter();
	for(uint i = 1; i < count; i++) {
		GlobalObjectEntry * p_entry = static_cast<GlobalObjectEntry *>(at(i));
		CALLPTRMEMB(p_entry, Destroy());
	}
	freeAll();
	Cs.Leave();
}

SlSession::GlobalObjectArray::~GlobalObjectArray()
{
	Destroy();
}

uint SlSession::GlobalObjectArray::CreateObject(SClassWrapper & rCls)
{
	uint   new_idx = 0;
	assert(count > 0);
	Cs.Enter();
	for(uint i = 1; !new_idx && i < count; i++) {
		GlobalObjectEntry * p_entry = static_cast<GlobalObjectEntry *>(at(i));
		if(p_entry->Ptr == 0) {
			*p_entry = rCls;
			THROW_S(p_entry->Create(), SLERR_NOMEM);
			new_idx = i;
		}
	}
	if(!new_idx) {
		GlobalObjectEntry new_entry;
		new_entry = rCls;
		THROW_S(new_entry.Create(), SLERR_NOMEM);
		THROW(insert(&new_entry));
		new_idx = getCount()-1;
	}
	CATCH
		new_idx = 0;
	ENDCATCH
	Cs.Leave();
	return new_idx;
}

int SlSession::GlobalObjectArray::DestroyObject(uint idx)
{
	int    ok = 1;
	Cs.Enter();
	if(idx && idx < count)
		static_cast<GlobalObjectEntry *>(at(idx))->Destroy();
	Cs.Leave();
	return ok;
}

void * FASTCALL SlSession::GlobalObjectArray::GetObject(uint idx)
{
	void * ptr = 0;
	Cs.Enter();
	if(idx && idx < getCount())
		ptr = static_cast<GlobalObjectEntry *>(at(idx))->Ptr;
	Cs.Leave();
	if(!ptr) {
		SString temp_buf;
		SLS.SetError(SLERR_GLOBOBJIDXNFOUNT, temp_buf.Cat(idx));
	}
	return ptr;
}

uint   SlSession::CreateGlobalObject(SClassWrapper & rCls) { return GlobObjList.CreateObject(rCls); }
int    SlSession::DestroyGlobalObject(uint idx) { return GlobObjList.DestroyObject(idx); }
void * FASTCALL SlSession::GetGlobalObject(uint idx) { return GlobObjList.GetObject(idx); }
int64  SlSession::GetSequenceValue() { return SeqValue.Incr(); }
uint64 SlSession::GetProfileTime() { return GetTLA().Prf.GetAbsTimeMicroseconds(); }

long SlSession::GetGlobalSymbol(const char * pSymb, long ident, SString * pRetSymb) // @cs
{
	long   _i = 0;
	// (здесь нельзя использовать макрос из-за зацикливания при трассировке блокировок) ENTER_CRITICAL_SECTION
	{
		static SCriticalSection::Data __csd(1);
		SCriticalSection __cs(__csd);
		uint   val = 0;
		if(pSymb) {
			if(GlobSymbList.Search(pSymb, &val, 0)) {
				_i = static_cast<long>(val);
				assert(ident <= 0 || _i == ident);
				if(ident > 0 && _i != ident) {
					_i = 0;
				}
			}
			else if(ident >= 0) {
				val = (uint)NZOR(ident, /*LastGlobSymbId*/SeqValue.Incr()); // @v9.8.1 LastGlobSymbId-->SeqValue
				if(GlobSymbList.Add(pSymb, val, 0)) {
					_i = static_cast<long>(val);
				}
			}
			else
				_i = -1;
		}
		else if(ident > 0) {
			SString temp_buf;
			SString * p_ret_symb = NZOR(pRetSymb, &temp_buf);
			if(GlobSymbList.GetByAssoc(ident, *p_ret_symb)) {
				_i = ident;
			}
		}
	}
	//LEAVE_CRITICAL_SECTION
	return _i;
}

long  SlSession::SetUiFlag(long f, int set)
{
    SlThreadLocalArea & r_tla = GetTLA();
    const long prev_ui_flags = r_tla.UiFlags;
    SETFLAG(r_tla.UiFlags, f, set);
    return prev_ui_flags;
}

int   FASTCALL SlSession::CheckUiFlag(long f) const { return BIN((GetConstTLA().UiFlags & f) == f); }

struct DdoEntry {
	DdoEntry() : Type(0), P_Obj(0)
	{
	}
	int    Type;
	void * P_Obj;
};

int SlSession::SetupDragndropObj(int ddoType, void * pObj)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	if(DragndropObjIdx) {
		DestroyGlobalObject(DragndropObjIdx);
		DragndropObjIdx = 0;
	}
	if(ddoType) {
		TSClassWrapper <DdoEntry> ptr_cls;
		DragndropObjIdx = CreateGlobalObject(ptr_cls);
        DdoEntry * p_item = DragndropObjIdx ? static_cast<DdoEntry *>(SLS.GetGlobalObject(DragndropObjIdx)) : 0;
		if(p_item) {
			 p_item->Type = ddoType;
			 p_item->P_Obj = pObj;
		}
		else
			ok = 0;
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}

int SlSession::IsThereDragndropObj(void ** ppObj)
{
	int    type = 0;
	ENTER_CRITICAL_SECTION
	if(DragndropObjIdx) {
		DdoEntry * p_item = static_cast<DdoEntry *>(SLS.GetGlobalObject(DragndropObjIdx));
		if(p_item) {
			ASSIGN_PTR(ppObj, p_item->P_Obj);
			type = p_item->Type;
		}
	}
	LEAVE_CRITICAL_SECTION
	return type;
}

SGlobalSecureConfig::SGlobalSecureConfig() : Flags(0)
{
}

bool SGlobalSecureConfig::IsEmpty() const { return (!Flags && CaFile.IsEmpty() && CaPath.IsEmpty()); }

const SGlobalSecureConfig & SlSession::GetGlobalSecureConfig()
{
	SlThreadLocalArea & r_tla = GetTLA();
	SGlobalSecureConfig & r_cfg = r_tla.Gsc;
	if(r_cfg.IsEmpty()) {
		if(ExtraProcBlk.F_GetGlobalSecureConfig) {
			ExtraProcBlk.F_GetGlobalSecureConfig(&r_cfg);
		}
		if(r_cfg.IsEmpty()) {
			SString temp_buf;
			GetExecPath(temp_buf).SetLastSlash();
			r_cfg.CaPath = temp_buf;
			r_cfg.CaFile = temp_buf.Cat("cacerts.pem");
		}
	}
	return r_cfg;
}
//
//
//
int SlSession::LogMessage(const char * pFileName, const char * pStr, ulong maxFileSize)
{
	int    ok = 1;
	long   current_size = 0;
	FILE * f = 0;
	SString file_name(pFileName);
	{
		SFsPath ps;
		if(file_name.NotEmptyS())
			ps.Split(file_name);
		ps.Nam.SetIfEmpty("slib");
		ps.Ext.SetIfEmpty("log");
		if(ps.Drv.IsEmpty() && ps.Dir.IsEmpty()) {
			GetLogPath(file_name);
			if(!file_name.NotEmptyS()) {
				GetExecPath(file_name);
			}
			file_name.SetLastSlash().Cat(ps.Nam);
			if(ps.Ext.C(0) != '.')
				file_name.Dot();
			file_name.Cat(ps.Ext);
		}
		else
			ps.Merge(file_name);
	}
	f = fopen(file_name, "r");
	if(f) {
		fseek(f, 0, SEEK_END);
		current_size = ftell(f);
		fclose(f);
		f = 0;
	}
	else {
		f = fopen(file_name, "w");
		if(f)
			fclose(f);
		else
			ok = 0;
	}
	if(ok) {
		SString msg_buf;
		(msg_buf = pStr).CR();
		ENTER_CRITICAL_SECTION
			int    counter;
			if(maxFileSize && (current_size + msg_buf.Len()) > maxFileSize*1024) {
				counter = 0;
				SString ext;
				SString b(file_name);
				do {
					SFsPath::ReplaceExt(b, ext.Z().CatLongZ(++counter, 3), 1);
				} while(fileExists(b));
				copyFileByName(file_name, b);
				SFile::Remove(file_name);
			}
			counter = 30;
			do {
				f = fopen(file_name, "a+");
				if(!f) {
					if(CheckStopFlag()) {
						ok = 0;
						break;
					}
					Sleep(10);
				}
			} while(!f && --counter);
			if(f) {
				fputs(msg_buf, f);
				fclose(f);
			}
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}

int SlSession::LoadString_(const char * pSignature, SString & rBuf) const
{
	LoadStringFunc f_ls = ExtraProcBlk.F_LoadString;
	return f_ls ? f_ls(pSignature, rBuf) : 0;
}

int SlSession::ExpandString(SString & rBuf, int ctransf) const
{
	ExpandStringFunc f_es = ExtraProcBlk.F_ExpandString;
	return f_es ? f_es(rBuf, ctransf) : 0;
}

int SlSession::QueryPath(const char * pSignature, SString & rBuf) const
{
    QueryPathFunc f_qp = ExtraProcBlk.F_QueryPath;
    return f_qp ? f_qp(pSignature, rBuf) : (rBuf.Z(), 0);
}

void SlSession::GetExtraProcBlock(SlExtraProcBlock * pBlk) const
{
	ASSIGN_PTR(pBlk, ExtraProcBlk);
}

void SlSession::SetExtraProcBlock(const SlExtraProcBlock * pBlk)
{
	ENTER_CRITICAL_SECTION
	ExtraProcBlk.Set(pBlk);
	LEAVE_CRITICAL_SECTION
}

void SlSession::LockPush(int lockType, const char * pSrcFileName, uint srcLineNo)
{
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla)
		r_tla.LckStk.Push(lockType, pSrcFileName, srcLineNo);
}

void SlSession::LockPop()
{
	SlThreadLocalArea & r_tla = GetTLA();
	if(&r_tla)
		r_tla.LckStk.Pop();
}

SString  & SlSession::AcquireRvlStr() { return GetTLA().RvlSStA.Get(); }
SStringU & SlSession::AcquireRvlStrU() { return GetTLA().RvlSStW.Get(); }

void SlSession::SaturateRvlStrPool(uint minSize) // @debug
{
	GetTLA().RvlSStA.Saturate(minSize);
}

void SlSession::SaturateRvlStrUPool(uint minSize) // @debug
{
	GetTLA().RvlSStW.Saturate(minSize);
}

int SlSession::CallHelp(void * hWnd, uint cmd, uint ctx)
{
	return ExtraProcBlk.F_CallHelp ? ExtraProcBlk.F_CallHelp(hWnd, cmd, ctx) : 0;
}

int SlSession::SubstString(const char * pSrcStr, int ansiCoding, SString & rBuf)
{
	int    ok = -1;
	if(pSrcStr && pSrcStr[0] == '@' && !sstrchr(pSrcStr, ' ')) {
		SString _text(pSrcStr);
		if(LoadString_(_text.ShiftLeft(1), rBuf) > 0) {
			if(ansiCoding)
				rBuf.Transf(CTRANSF_INNER_TO_OUTER);
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL SlSession::SetCodepage(SCodepage cp)
{
	int    ok = 1;
	SlThreadLocalArea & r_tla = GetTLA();
	r_tla.CurrentCp = cp;
	SString cp_text;
	if(r_tla.CurrentCp.ToStr(SCodepageIdent::fmtCLibLocale, cp_text)) {
		setlocale(LC_CTYPE, cp_text);
		setlocale(LC_COLLATE, cp_text);
	}
	return ok;
}

SCodepage SlSession::GetCodepage() const
{
	const SlThreadLocalArea & r_tla = GetConstTLA();
	return r_tla.CurrentCp;
}
//
//
//
// @v10.5.6 uint32 FASTCALL RSHash(const void * pData, size_t len); // @prototype
void __LinkFile_HASHFUNC()
{
	SlHash::RS("", 0);
}

#pragma warning(disable:4073)
#pragma init_seg(lib)
SlSession SLS; // @global
DbSession DBS; // @global
