// PPTEST.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2010, 2012, 2015, 2016, 2017, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8 // @v11.4.1
//
#include <pp.h>
#pragma hdrstop

//#define TEST_LOCALE
//#define TEST_HASH_AND_SEARCH
//#define TEST_DBTEXTFILE
//#define TEST_EDITIMPEXPPARAM
//#define TEST_REGEXP
//#define TEST_DL600

#if 0 // @construction {
class SConsole {
public:
	SConsole() : IsAllocated(0)
	{
	}
	int Open(const char * pTitle) 
	{
		int    ok = 1;
		//dont do anything if we're already attached
		if(!IsAllocated) {
			//attach to an existing console (if we can; this is circuitous because AttachConsole wasnt added until XP)
			//remember to abstract this late bound function notion if we end up having to do this anywhere else
			bool attached = false;
			{
				HMODULE lib = LoadLibrary(_T("kernel32.dll"));
				if(lib) {
					typedef BOOL (WINAPI *_TAttachConsole)(DWORD dwProcessId);
					_TAttachConsole _AttachConsole  = (_TAttachConsole)GetProcAddress(lib, "AttachConsole");
					if(_AttachConsole) {
						if(_AttachConsole(-1))
							attached = true;
					}
					FreeLibrary(lib);
				}
			}
			//if we failed to attach, then alloc a new console
			if(!attached) {
				AllocConsole();
				IsAllocated = 1;
			}
			{
				//redirect stdio
				HANDLE h_stdio = GetStdHandle(STD_OUTPUT_HANDLE);
				int h_con = _open_osfhandle(reinterpret_cast<intptr_t>(h_stdio), _O_TEXT);
				THROW(h_con != -1);
				{
					FILE * fp = _fdopen(h_con, "w");
					if(fp) {
						*stdout = *fp;
						setvbuf(stdout, NULL, _IONBF, 0);
					}
				}
			}
			{
				//redirect stderr
				HANDLE h_stderr = GetStdHandle(STD_ERROR_HANDLE);
				int h_con = _open_osfhandle(reinterpret_cast<intptr_t>(h_stderr), _O_TEXT);
				THROW(h_con != -1);
				{
					FILE * fp = _fdopen(h_con, "w");
					if(fp) {
						*stderr = *fp;
						setvbuf(stderr, NULL, _IONBF, 0);
					}
				}
			}
			{
				//redirect stdin
				HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
				int h_con = _open_osfhandle(reinterpret_cast<intptr_t>(h_stdin), _O_TEXT);
				THROW(h_con != -1);
				{
					FILE * fp = _fdopen(h_con, "r");
					if(fp) {
						*stdin = *fp;
						setvbuf(stdin, NULL, _IONBF, 0);
					}
				}
			}
			//sprintf(buf,"%s OUTPUT", DESMUME_NAME_AND_VERSION);
			if(!isempty(pTitle))
				SetConsoleTitle(SUcSwitch(pTitle));
			{
				CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
				GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo);
				{
					COORD csize;
					csize.X = csbiInfo.dwSize.X;
					csize.Y = 800;
					SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), csize);
				}
				/*{
					SMALL_RECT srect = csbiInfo.srWindow;
					srect.Right  = srect.Left + 99;
					srect.Bottom = srect.Top + 64;
					SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &srect);
				}*/
			}
			SetConsoleCP(GetACP());
			SetConsoleOutputCP(GetACP());
			/*
			if(attached) 
				printlog("\n");
			printlog("%s\n",DESMUME_NAME_AND_VERSION);
			printlog("- compiled: %s %s\n\n",__DATE__,__TIME__);
			*/
		}
		else
			ok = -1;
		CATCHZOK
		return ok;
	}
	void Close() 
	{
		if(IsAllocated) {
			//printlog("Closing...");
			::FreeConsole(); 
			IsAllocated = 0;
		}
	}
private:
	int    IsAllocated;
};
#endif // } 0 @construction

#if 0 // {

class TestQ {
public:
	unsigned long v;
};

class TestABC {
public:
	TestABC();
	TestABC(int);
	operator double ();
	operator long ();
	operator TestQ ();
	int operator -- ();
	int foo(int a, int b);
	int fooC(int a, int b) const;
	int M;
};

class Test2 {
public:
	int R;
};

int TestABC::foo(int a, int b) { return a+b; }
int TestABC::fooC(int a, int b) const { return a+b; }
int TestABC::operator -- () { return --M; }
TestABC::operator double () { return (double)M; }
TestABC::operator long () { return (long)M; }
TestABC::operator TestQ() { TestQ d; d.v = (ulong)M; return d; }
int operator + (TestABC & rAbc, int a) { return rAbc.M + a; }
int operator - (TestABC & rAbc, int a) { return rAbc.M + a; }
int operator * (TestABC & rAbc, int a) { return rAbc.M * a; }
int operator / (TestABC & rAbc, int a) { return rAbc.M / a; }
int operator != (TestABC & rAbc, int a) { return (rAbc.M != a); }
int operator <= (TestABC & rAbc, int a) { return (rAbc.M <= a); }
int operator >= (TestABC & rAbc, int a) { return (rAbc.M >= a); }
int test_foo(TestABC & rAbc) { return 0; }
int test_foo(TestABC * pAbc) { return 0; }
int test_foo(TestABC a1, TestABC a2) { return 0; }

//?test_foo@@YAHVTestQ@@VTestABC@@VTest2@@12@Z       == int test_foo(TestQ q, TestABC a1, Test2 _2, TestABC a2, Test2 _4)
//?test_foo@@YAHVTestQ@@VTestABC@@VTest2@@1PAV3@@Z   == int test_foo(TestQ q, TestABC a1, Test2 _2, TestABC a2, Test2 * _4)
//?test_foo@@YAHVTestQ@@VTestABC@@VTest2@@PAV2@2@Z   == int test_foo(TestQ q, TestABC a1, Test2 _2, TestABC * a2, Test2 _4)
//?test_foo@@YAHAAVTestQ@@VTestABC@@VTest2@@PAV2@2@Z == int test_foo(TestQ & q, TestABC a1, Test2 _2, TestABC * a2, Test2 _4)
//?test_foo@@YA?AVTestQ@@AAV1@@Z                     == TestQ test_foo(TestQ & q)
//?test_foo@@YAAAVTestQ@@AAV1@@Z                     == TestQ & test_foo(TestQ & q)

int  test_foo(int a, int b) { return (a+b); }
int  test_foo(int * a, int b) { return (*a + b); }
void test_foo2(int * a, int b) { *a = *a+b; }

#endif // } 0

#if !SLTEST_RUNNING // {

//#include <ppsoapclient.h>

int ClassfGoods();
int TestTddo();
int GetGoodsFromService(SString & rCode, PPGoodsPacket * pPack);
int TestSTree();

int TestNoLogin()
{
	//TestSTree(); // @debug
	return -1;
}

int TestLogin()
{
	//ClassfGoods(); // @debug
	//TestTddo(); // @debug
	//Debug_GetFilesFromMessage("D:\\PAPYRUS\\PPY\\IN\\00000001(2).msg");

	/*
	PPUhttClient UhttClient;
	UhttSCardPacket pack;

	if(UhttClient.Auth()) {
		UhttClient.GetSCardByNumber("123ABC", pack);

		UhttCheckPacket check;
		check.Amount = 1234.56;
		int r = UhttClient.CreateSCardCheck("myloc3", "123ABC", check);

		UhttClient.WithdrawSCardAmount("123ABC", 10.5);

		double rest = 0;
		r = UhttClient.GetSCardRest("123ABC", 0, rest);

		UhttClient.DepositSCardAmount("123ABC", 100);
		r = UhttClient.GetSCardRest("123ABC", 0, rest);
	}

	// SCARD_1:AE;SCARD_2:AO;A;SCARD_2:C;SCARD_1:D;CO
	PPGlobalAccRights per_blk("A;:E");

	int ad = per_blk.IsAllow(PPGlobalAccRights::fAccess);
	int cd = per_blk.IsAllow(PPGlobalAccRights::fCreate);
	int ed = per_blk.IsAllow(PPGlobalAccRights::fEdit);
	int dd = per_blk.IsAllow(PPGlobalAccRights::fDelete);
	int od = per_blk.IsAllow(PPGlobalAccRights::fOperation);

	int a1 = per_blk.IsAllow(PPGlobalAccRights::fAccess, "SCARD_1");
	int c1 = per_blk.IsAllow(PPGlobalAccRights::fCreate, "SCARD_1");
	int e1 = per_blk.IsAllow(PPGlobalAccRights::fEdit, "SCARD_1");
	int d1 = per_blk.IsAllow(PPGlobalAccRights::fDelete, "SCARD_1");
	int o1 = per_blk.IsAllow(PPGlobalAccRights::fOperation, "SCARD_1");

	int a2 = per_blk.IsAllow(PPGlobalAccRights::fAccess, "SCARD_2");
	int c2 = per_blk.IsAllow(PPGlobalAccRights::fCreate, "SCARD_2");
	int e2 = per_blk.IsAllow(PPGlobalAccRights::fEdit, "SCARD_2");
	int d2 = per_blk.IsAllow(PPGlobalAccRights::fDelete, "SCARD_2");
	int o2 = per_blk.IsAllow(PPGlobalAccRights::fOperation, "SCARD_2");
	*/
	return -1;
}

#else //

// @v11.7.1 #include <locale.h>
/*
#define ABCD virtual void abc(int & r)

class A {
public:
	A();
	ABCD {
		r = a;
	}
	int a;
};
*/

static int TestSearch(int alg, int flags, const SString & rPat, const SString & rText, size_t numSucc, size_t * pSuccList)
{
	size_t num_suc_test = 0, srch_pos = 0;
	const  size_t max_suc = 256;
	size_t suc_test[max_suc];
	int    ok = 1, r = 0;
	SString msg_buf;
	SSrchPattern blk(rPat, flags, alg);
	//PROFILE_START
	for(size_t start = 0; (r = blk.Search(rText, start, rText.Len(), &srch_pos)) != 0; start = srch_pos+1) {
		if(num_suc_test < max_suc)
			suc_test[num_suc_test] = srch_pos;
		num_suc_test++;
	}
	//PROFILE_END
	if(num_suc_test == numSucc) {
		for(size_t j = 0; j < num_suc_test; j++)
			if(j < max_suc && pSuccList[j] != suc_test[j]) {
				(msg_buf = "Ошибка тестового поиска строк: не совпадают позиции").Transf(CTRANSF_UTF8_TO_INNER);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, 0);
				ok = 0;
			}
	}
	else {
		(msg_buf = "Ошибка тестового поиска строк: не совпадает количество удач").Transf(CTRANSF_UTF8_TO_INNER);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, 0);
		ok = 0;
	}
	return ok;
}

int Test_Hash_And_Search(const char * pInputFileName)
{
	int    ok = 1;
	size_t sz = 0;
	MemLeakTracer mlt;
	{
		for(int i = 10000; sz == 0; i++)
			if(IsPrime(i))
				sz = (size_t)i;
	}
	{
		SymbHashTable tab(sz);
		SFile file(pInputFileName, SFile::mRead | SFile::mBinary);
		if(file.IsValid()) {
			SString word_buf, text_buf;
			SSrchPattern blk(0);
			long   file_pos = file.Tell();
			uint   i = 1;
			uint   pos = 0;
			//
			// Вставляем строки в хэш-таблицу
			//
			// Одновременно забрасываем строки в общий текст для тестирования функции поиска.
			//
			while(ok && file.ReadLine(word_buf, SFile::rlfChomp)) {
				if(!tab.Search(word_buf, 0, &pos)) {
					text_buf.Cat(word_buf);
					if(!tab.Add(word_buf, (uint)file_pos, &pos))
						ok = 0;
				}
				else
					ok++;
				file_pos = file.Tell();
			}

			//
			// Проверяем правильно ли были вставлены строки в хэш-таблицу?
			//
			file.Seek(0, SEEK_SET);
			file_pos = file.Tell();
			while(ok && file.ReadLine(word_buf, SFile::rlfChomp)) {
				uint val = 0;
				if(tab.Search(word_buf, &val, 0)) {
					size_t srch_pos = 0;
					size_t num_suc = 0;
					const size_t max_suc = 256;
					size_t suc[max_suc];

					if(val != (uint)file_pos)
						ok = 0;
					file_pos = file.Tell();
					//
					// Проверяем функцию поиска
					//
					int srch_flags = 0;//SSrchPattern::fNoCase;
					if(srch_flags == SSrchPattern::fNoCase) {
						int r;
						memzero(suc, sizeof(suc));
						PROFILE_START_S("standard no case");
						SSrchPattern blk(word_buf, srch_flags, SSrchPattern::algDefault);
						for(size_t start = 0; (r = blk.Search(text_buf, start, text_buf.Len(), &srch_pos)) != 0; start = srch_pos+1) {
							if(num_suc < max_suc)
								suc[num_suc] = srch_pos;
							num_suc++;
						}
						PROFILE_END
					}
					else {
						//
						// Традиционный поиск для сравнения производительности
						//
						memzero(suc, sizeof(suc));
						PROFILE_START_S("standard");
						const char * p = 0;
						for(size_t start = 0; (p = strstr(text_buf+start, word_buf)) != 0; start = srch_pos+1) {
							srch_pos = (p-text_buf);
							if(num_suc < max_suc)
								suc[num_suc] = srch_pos;
							num_suc++;
						}
						PROFILE_END
					}
					PROFILE_S(TestSearch(SSrchPattern::algBmGoodSfx, srch_flags, word_buf, text_buf, num_suc, suc), "algBmGoodSfx");
					PROFILE_S(TestSearch(SSrchPattern::algBmBadChr,  srch_flags, word_buf, text_buf, num_suc, suc), "algBmBadChr");
					PROFILE_S(TestSearch(SSrchPattern::algDefault,   srch_flags, word_buf, text_buf, num_suc, suc), "algDefault");
				}
				else
					ok = 0;
			}
			{
				//
				// Проверяем функцию поиска для случайных строк. Результат должен быть отрицательным.
				//
				char * miss_strings[] = {
					"Проверяем функц",
					"if(!r || strncmp(text_buf + srch_pos, word_buf, word_buf.Len()) != ",
					"",
					"$$##!@ ",
					"Результат должен быть отрицательным."
				};
				for(i = 0; i < sizeof(miss_strings) / sizeof(char *); i++) {
					size_t srch_pos = 0;
					blk.Init(word_buf);
					int r = blk.Search(text_buf, 0, text_buf.Len(), &srch_pos);
					if(r && strncmp(text_buf + srch_pos, word_buf, word_buf.Len()) != 0)
						ok = 0;
				}
			}
			//
			// Перебираем все элементы хэш-таблицы и скидываем их в исходящий файл.
			// В результате мы должны получить файл, содержащий ровно столько же строк, что и входящий файл.
			//
			{
				SString fn(pInputFileName);
				//replaceExt(fn, "OUT", 1);
				SFsPath::ReplaceExt(fn, "out", 1);
				SFile out_file(fn, SFile::mWrite);
				if(out_file.IsValid()) {
					SymbHashTable::Iter iter;
					for(tab.InitIteration(&iter); tab.NextIteration(&iter, &i, 0, &word_buf);)
						out_file.WriteLine(word_buf.CR());
				}
				else
					ok = 0;
			}
			//
			//
			//
		}
		else
			ok = 0;
	}
	return ok;
}

#ifdef TEST_REGEXP
int Test_RegExp(const char * pInputFileName)
{
	const  char * p_regexp_list[] = {
		"[ \t]+",   // whitespaces
		"\"[^\"]*\"",      // quoted string
		"[+-]?[0-9]+",  // integer
		"[+-]?[0-9]*([\\.][0-9]*)([Ee][+-]?[0-9]+)?", // real
		"\\[[ \t]*[0-9]+(\\.[0-9]*)?[ \t]*\\]",
		"\\([ \t]*[0-9]+(\\.[0-9]*)?[ \t]*\\)",
		"[0-9]+[ \t]*\\,[ \t]*[0-9]+",
		"(\\[[0-9]+(\\.[0-9]*)?\\]) | (\\([0-9]+(\\.[0-9]*)?\\)) | ([0-9]+\\,[0-9]+)"
	};
	int    ok = 1;
	char   fn[MAX_PATH];
	SFile file(pInputFileName, SFile::mRead | SFile::mBinary);
	if(file.IsValid()) {
		SString line_buf, temp_buf;
		CRegExp re;

		STRNSCPY(fn, pInputFileName);
		replaceExt(fn, "OUT", 1);
		SFile out_file(fn, SFile::mWrite);

		for(uint r = 0; r < sizeof(p_regexp_list) / sizeof(*p_regexp_list); r++) {
			long   file_pos = file.Tell(SEEK_SET);
			uint   i = 1;
			uint   pos = 0;
			if(re.Compile(p_regexp_list[r])) {
				temp_buf = "RE: ";
				temp_buf.Cat(p_regexp_list[r]).CR();
				out_file.WriteLine(temp_buf);
				file.Seek(0, SEEK_SET);
				while(ok && file.ReadLine(line_buf, SFile::rlfChomp)) {
					out_file.WriteLine(line_buf);
					SStrScan scan(line_buf);
					if(re.Find(&scan)) {
						do {
							out_file.WriteLine(0);
							scan.Get(temp_buf);
							out_file.WriteLine(temp_buf.Quot('*', '*').CR());
							scan.Offs += scan.Len;
						} while(re.Find(&scan));
					}
					else
						out_file.WriteLine((temp_buf = "Not Found").CR());
					//out_file.WriteLine(0);
				}
				//out_file.WriteLine(0);
			}
			else {
				temp_buf.Printf("Error compiling regexp %s; code = %d\n\n\n", p_regexp_list[r], SLibError);
				out_file.WriteLine(temp_buf);
			}
		}
	}
	return ok;
}
#endif

//#define PP_TEST

#ifdef PP_TEST
int TestSPathStruc(const char * pInputFileName)
{
	int    ok = 1;
	SFile file(pInputFileName, SFile::mRead | SFile::mBinary);
	if(file.IsValid()) {
		char   fn[MAX_PATH];
		SString line_buf;
		STRNSCPY(fn, pInputFileName);
		replaceExt(fn, "OUT", 1);
		SFile out_file(fn, SFile::mWrite);
		SFsPath ps;
		while(file.ReadLine(line_buf, SFile::rlfChomp)) {
			SInvariantParam ip;
			ps.Split(line_buf);
			if(!ps.Invariant(&ip))
				ok = 0;
			out_file.WriteLine(ip.MsgBuf.CR());
		}
	}
	return ok;
}
#endif
//
//
//
int Test_Alg_SS_Z(const char * pInputFileName);
int TestTextDbFile(const char * pInDbfFile);
int Test_InterfaceCall();
int DummyProc_dirent(); // @prototype @forcelink
int DummyProc_bzip3(); // @prototype @forcelink
int DummyProc_sfxtree(); // @forcelink
int DummyProc_TulipIndicators(); // @prototype @forcelink
// @v11.2.0 {
// Для сборки _MSC_VER менее чем 2015 мы не будем поддерживать LMDB. Здесь включена пустышка для пропуска соответствующего теста
#if _MSC_VER >= 1900
	int DummyProc_LMDB();   // @prototype @forcelink
	int DummyProc_SFileStorage(); // @prototype @forcelink
#else
	#if SLTEST_RUNNING
		SLTEST_R(LMDB) { return 1; }
	#endif
#endif
// } @v11.2.0

int TestNoLogin()
{
	int    ok = -1;
	STestSuite s;
	SRng::CreateInstance((SRng::Algorithm)0, 0); // @forcelink RandomNumberGeneragtor
	DummyProc_dirent(); // @v10.9.12 @forcelink
	DummyProc_bzip3(); // @v11.7.4 @forcelink
	DummyProc_sfxtree(); // @v11.7.4 @forcelink
	DummyProc_TulipIndicators(); // @v11.4.4 @forcelink
#if _MSC_VER >= 1900
	DummyProc_LMDB(); // @v11.2.0 @forcelink
	DummyProc_SFileStorage(); // @v11.3.3 @forcelink
#endif
	s.Run("\\papyrus\\src\\pptest\\testdef.ini");
	//Test_Alg_SS_Z("c:\\papyrus\\src\\pptest\\words.");
	//Test_InterfaceCall();
#ifdef PP_TEST
	TestSPathStruc("c:\\papyrus\\src\\pptest\\path.txt");
#endif
#ifdef TEST_LOCALE
	char * p_loc = setlocale(LC_COLLATE, "rus_rus.1251");
	SString c1("аврора");
	SString c2("аВрОра");
	c1.Transf(CTRANSF_UTF8_TO_OUTER);
	c2.Transf(CTRANSF_UTF8_TO_OUTER);
	if(stricmp(c1, c2) != 0) {
		ok = 0;
	}
#endif // TEST_LOCALE
#ifdef TEST_HASH_AND_SEARCH
	Test_Hash_And_Search("c:\\papyrus\\src\\pptest\\words.");
#endif // TEST_HASH_AND_SEARCH
#ifdef TEST_DBTEXTFILE
	TestTextDbFile("c:\\papyrus\\src\\pptest\\sp_goods.dbf");
#endif
#ifdef TEST_REGEXP
	Test_RegExp("c:\\papyrus\\src\\pptest\\re.txt");
#endif
	return ok;
}

int Test_ImpExpParamDialog();
int Test_DL6_Rtm();

int TestLogin()
{
	STestSuite s;
	s.Run("\\papyrus\\src\\pptest\\testdef_login.ini");
	// Debug_GetFilesFromMessage("D:\PAPYRUS\PPY\IN\00000001(2).msg");
#ifdef TEST_EDITIMPEXPPARAM
	Test_ImpExpParamDialog();
#endif
#ifdef TEST_DL600
	Test_DL6_Rtm();
#endif
	return -1;
}

#endif // } NDEBUG

// turistti	@v5.3.1 {
//
//Descr:Генератор случайных чисел.
//  Может генерировать как равномернораспределенные числа,
//  так и распределенные по некоторому з-ну числа...
//
#ifndef VAR

#ifdef _WIN32
	#ifdef DLL
		#ifdef DLL_EXPORT
			#define VAR extern __declspec(dllexport)
		#else
			#define VAR extern __declspec(dllimport)
		#endif
	#else
		#define VAR extern
	#endif
#else
	#define VAR extern
#endif

#endif

typedef struct {
    const char * P_Name;
    uint   Max;
    uint   Min;
    size_t Size;
    void (* P_Set) (void * P_State, uint Seed);
    uint (* P_Get) (void * P_State);
    double (* P_GetDouble) (void * P_State);
} RngType;

typedef struct {
    const RngType * P_Type;
    void * P_State;
} Rng;

VAR   const RngType * Rng_Mt_19937;
VAR   uint RngDefaultSeed;
VAR   const RngType * RngDefault;

const  RngType ** RngTypesSetup();
Rng *  RngAlloc(const RngType * pT);
void   RngFree(Rng * pR);
void   RngSet(const Rng * pR, uint seed);
uint   RngMax(const Rng * pR);
uint   RngMin(const Rng * pR);
const  char * RngName(const Rng * pR);
size_t RngSize(const Rng * pR);
void * RngState(const Rng * pR);
const  RngType * RngEnvSetup();
uint   RngGet(const Rng * pR);
double RngUniform(const Rng * pR);
double RngUniformPos(const Rng * pR);
uint   RngUniformInt(const Rng * pR, uint n);

#ifdef HAVE_INLINE //{

extern inline uint RngGet (const Rng * pR);
extern inline uint RngGet (const Rng * pR) {return (pR->P_Type->P_Get) (pR->P_State);}
extern inline double RngUniform (const Rng * pR);
extern inline double RngUniform (const Rng * pR) {return (pR->P_Type->P_GetDouble) (pR->P_State);}
extern inline double RngUniformPos (const Rng * pR);
extern inline double RngUniformPos (const Rng * pR)
{
	double x;
	do {
		x = (pR->P_Type->P_GetDouble) (pR->P_State) ;
	}
	while (x == 0) ;
	return x ;
}

extern inline uint RngUniformInt (const Rng * pR, uint n);
extern inline uint RngUniformInt (const Rng * pR, uint n)
{
	uint offset = pR->P_Type->Min;
	uint range = pR->P_Type->Max - offset;
	uint scale;
	uint k;
	if(n > range || n == 0) {
		GSL_ERROR_VAL ("invalid n, either 0 or exceeds maximum value of generator", GSL_EINVAL, 0);
	}
	scale = range / n;
	do {
		k = (((pR->P_Type->P_Get) (pR->P_State)) - offset) / scale;
	}
	while (k >= n);
	return k;
}

#endif // } HAVE_INLINE

static inline uint MtGet (void * p_vstate);
static double MtGetDouble (void * p_vstate);
static void MtSet (void * p_state, uint s);

#define __N 624   //Period parameters
#define __M 397

static const ulong UPPER_MASK = 0x80000000UL; //most significant w-r bits
static const ulong LOWER_MASK = 0x7fffffffUL; //least significant r bits

typedef struct {
	unsigned long mt[__N];
	int mti;
} MtStateT;

static inline uint MtGet(void * p_vstate)
{
	MtStateT * p_state = static_cast<MtStateT *>(p_vstate);
	unsigned long k ;
	ulong * const mt = p_state->mt;
#define MAGIC_RNG(y) (((y)&0x1) ? 0x9908b0dfUL : 0)
	if(p_state->mti >= __N) {
		//generate N words at one time
		int kk = 0;
		for(kk = 0; kk < __N - __M; kk++) {
			unsigned long y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + __M] ^ (y >> 1) ^ MAGIC_RNG(y);
		}
		for(; kk < __N - 1; kk++) {
			unsigned long y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + (__M - __N)] ^ (y >> 1) ^ MAGIC_RNG(y);
		}
		{
			unsigned long y = (mt[__N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
			mt[__N - 1] = mt[__M - 1] ^ (y >> 1) ^ MAGIC_RNG(y);
		}
		p_state->mti = 0;
	}
	//Tempering
	k = mt[p_state->mti];
	k ^= (k >> 11);
	k ^= (k << 7) & 0x9d2c5680UL;
	k ^= (k << 15) & 0xefc60000UL;
	k ^= (k >> 18);
	p_state->mti++;
	return k;
}

static double MtGetDouble (void * p_vstate) { return MtGet (p_vstate) / SMathConst::MaxU32; }

static void MtSet(void * p_vstate, uint s)
{
	MtStateT * p_state = static_cast<MtStateT *>(p_vstate);
	int i;
	if(s == 0)
		s = 4357;   //the default seed is 4357
	p_state->mt[0]= s & 0xffffffffUL;
	for (i = 1; i < __N; i++) {
		p_state->mt[i] = (1812433253UL * (p_state->mt[i-1] ^ (p_state->mt[i-1] >> 30)) + i);
		p_state->mt[i] &= 0xffffffffUL;
	}
	p_state->mti = i;
}

static const RngType MtType = {
	"Mt_19937",			//name
	0xffffffffUL,		//RAND_MAX
	0,					//RAND_MIN
	sizeof (MtStateT),
	&MtSet,
	&MtGet,
	&MtGetDouble
};

const RngType * Rng_Mt_19937 = &MtType;

//MT19937 is the default generator, so define that here too
const RngType * RngDefault = &MtType;
uint RngDefaultSeed = 0;

void RngSet(const Rng * pR, uint seed) {(pR->P_Type->P_Set) (pR->P_State, seed);}

Rng * RngAlloc(const RngType * pT)
{
	Rng * pR = static_cast<Rng *>(SAlloc::M(sizeof(Rng)));
	/*if (pR == 0) {
		ERROR_VAL ("failed to allocate space for rng struct", ENOMEM, 0);
    };*/
	pR->P_State = SAlloc::M(pT->Size);
	if (pR->P_State == 0) {
		free (pR);		//exception in constructor, avoid memory leak
		/*ERROR_VAL ("failed to allocate space for rng state", ENOMEM, 0);*/
	};
	pR->P_Type = pT;
	RngSet (pR, RngDefaultSeed);		//seed the generator
	return pR;
}

#ifndef HIDE_INLINE_STATIC
uint RngGet(const Rng * pR) {return (pR->P_Type->P_Get) (pR->P_State);}
double RngUniform (const Rng * pR) {return (pR->P_Type->P_GetDouble) (pR->P_State);}
//
//Descr: Генератор равномернораспределенных
//  случайных чисел с плавающей точкой
//
double RngUniformPos (const Rng * pR)
{
	double x ;
	do {x = (pR->P_Type->P_GetDouble) (pR->P_State) ;}
	while (x == 0) ;
	return x ;
}
//
//Descr: Генератор равномернораспределенных
//  целых случайных чисел
//
//Note: to avoid integer overflow in (range+1) we work with scale =
//  range/n = (max-min)/n rather than scale=(max-min+1)/n, this reduces
//  efficiency slightly but avoids having to check for the out of range
//  value.  Note that range is typically O(2^32) so the addition of 1
//  is negligible in most usage.
//
uint RngUniformInt (const Rng * pR, uint n)
{
	uint offset = pR->P_Type->Min;
	uint range = pR->P_Type->Max - offset;
	uint scale;
	uint k;
	/*if (n > range || n == 0) {
		ERROR_VAL ("invalid n, either 0 or exceeds maximum value of generator", EINVAL, 0) ;
	}*/
	scale = range / n;
	do {k = (((pR->P_Type->P_Get) (pR->P_State)) - offset) / scale;}
	while (k >= n);
	return k;
}
#endif

uint RngMax (const Rng * pR) {return pR->P_Type->Max;}
uint RngMin (const Rng * pR) {return pR->P_Type->Min;}
const char * RngName (const Rng * pR) {return pR->P_Type->P_Name;}
size_t RngSize (const Rng * pR) {return pR->P_Type->Size;}
void * RngState (const Rng * pR) {return pR->P_State;}
void RngFree (Rng * pR)
{
	if(pR) {
		SAlloc::F(pR->P_State);
		SAlloc::F(pR);
	}
}
//
//The initial defaults are defined in the file mt.c, so we can get
//  access to the static parts of the default generator.

const RngType * RngEnvSetup()
{
	uint seed = 0;
	const char *p = getenv("RNG_TYPE");
	if (!p) {
		RngDefault = Rng_Mt_19937;
	}
	p = getenv ("RNG_SEED");
	if (p)
	{
		seed = strtoul (p, 0, 0);
		/*fprintf (stderr, "RNG_SEED=%lu\n", seed);*/
	};
	RngDefaultSeed = seed;
	return RngDefault;
}
//Варианты распределения случайных чисел
class RandNumbGen {
public:
	double Exponential(const Rng * P_r, const double mu);
	double Gaussian(const Rng * P_r, const double sigma);
};
//
//Descr: Генератор случайных чисел распределенных
//  по экспоненциональному закону
//
double RandNumbGen::Exponential(const Rng * P_r, const double mu)
{
	double u = RngUniformPos(P_r);
	return -mu * log(u);
}
//
//Descr: Генератор случайных чисел распределенных
//  по закону Гаусса
//
double RandNumbGen::Gaussian(const Rng * P_r, const double sigma)
{
	double x, y, r2;
	do {
		//choose x,y in uniform square (-1,-1) to (+1,+1)
		x = -1 + 2 * RngUniformPos(P_r);
		y = -1 + 2 * RngUniformPos(P_r);
		//see if it is in the unit circle
		r2 = x * x + y * y;
	} while(r2 > 1.0 || r2 == 0);
	//Box-Muller transform
	return sigma * y * sqrt (-2.0 * log (r2) / r2);
}

//ГЕНЕРАЦИЯ ТОВАРНЫХ ДОКУМЕНТОВ ПО ЭКСПОНЕНЦИАЛЬНОМУ З-НУ
/* @v11.7.10 @obsolete int GetRandom(int min, int max)
{
	return static_cast<int>(SLS.GetTLA().Rg.GetUniformPos() * (max - min) + min);
	//return (int)(((double)rand() / (RAND_MAX + 1)) * (max - min) + min);  //генератор псевдослучайных чисел
}*/

int GenerateGoodsBills()
{
	int    ok = -1;
	PPViewOpGrouping * p_vop = 0;
	PPViewGoodsOpAnalyze * p_vgoods = 0;
	Rng * p_rng = 0;
	const PPConfig & r_lcfg = LConfig;
	int  i = 0, j = 0, m = 0;  //счетчики в циклах
	uint k = 0;
	double cost = 0, price = 0; //сумма и цена
	double qtty = 0; //количество товара
	long int k1 = 0;  //генерируемое число для случайного выбора типа операции
	double k2 = 0;    //генерируемое число для случайного выбора номера строки
	ulong iter = 0; //кол-во итераций
	//для определения временного периода
	DateRange period;
	//для определения типа и вида операции
	PPID  op_id = 0,  id = 0;//id операции
	PPID  op_type = 0; //тип операции
	PPOprKind op_kind; //тип операции
	PPIDArray ops;  //массив типов операции по доходам и расходам
	PPIDArray allow_ops;  //массив доступных операции за временной период
	uint opscount, allow_opscount;  //кол-во элементов в массиве ооперации
	// для генератора
	const RngType * p_type = 0;
	RandNumbGen generator;
	int mu = 100;     //переменная для генератора по показательному з-ну
	//для введения кол-ва документов
	SString title, inp_title;
	SString msg_buf;
	double bills_count = 0;
	//для фильтра "Группировка операций"
	OpGroupingFilt op_flt;
	BillStatArray list;
	//для фильтра "Товарный отчет по операции"
	GoodsOpAnalyzeFilt goods_flt;
	GoodsOpAnalyzeViewItem goods_item;
	PPIDArray goods_list;
	int count = 0;    //кол-во док-тов в выборке по фильру по товарам
	// для массива контрагентов
	PPID acc_sheet_id = 0, acc_sheet2_id = 0; //таблица статей
	PPIDArray contragent_list; //масииви контрагентов
	long contragent_count = 0; //количество контрагентов
	PPObjArticle ar_obj;
	//PPLogger logger;

	PPIniFile ini_file;
	int    enbl = 0;
	THROW_PP(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ENABLEBILLGENERATOR, &enbl) && enbl, PPERR_BILLGEN_NOTALLOWED);

	//установка генератора
	RngEnvSetup();
	p_type = RngDefault;
	p_rng = RngAlloc(p_type);

	//генерация периода времени (год назад от настоящего)
	period.SetDate(r_lcfg.OperDate);
	plusperiod(&period.low, PRD_ANNUAL, -1, 0);

	//введение количества документов
	PPLoadText(PPTXT_BILL_COUNT, title);
	PPLoadText(PPTXT_INP_BILL_COUNT, inp_title);
	if(InputQttyDialog(title, inp_title, &bills_count) > 0) {
		PPWaitStart();
		//
		// фильтр "Группировка операций"
		//
		op_flt.Flags = OpGroupingFilt::fCalcAvgLn;
		op_flt.Period.upp = period.upp;
		op_flt.Period.low = period.low;

		THROW_MEM(p_vop = new PPViewOpGrouping);
		THROW(p_vop->Init_(&op_flt));
		//
		// создание BillStatArray массива
		//
		p_vop->CalcStat(&list);
		allow_opscount = list.getCount();
		//
		// массив операций расходов или доходов
		//
		while(EnumOperations(0, &op_id, &op_kind) > 0)
			if(oneof2(op_kind.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND))
				ops.add(op_id);
		opscount = ops.getCount();
		{
			//
			// Инициализация фильтра "Товарный отчет по операции"
			//
			goods_flt.OpGrpID = GoodsOpAnalyzeFilt::ogIncoming;
			goods_flt.Period.low = period.low;
			goods_flt.Period.upp = period.upp;
			THROW_MEM(p_vgoods = new PPViewGoodsOpAnalyze);
			THROW(p_vgoods->Init_(&goods_flt));
			//
			// Поиск документа по флагу доходные без возвратов
			//
			for(p_vgoods->InitIteration(PPViewGoodsOpAnalyze::OrdByIncome); p_vgoods->NextIteration(&goods_item) > 0;)
				goods_list.add(goods_item.GoodsID);
			if(goods_list.getCount()) {
				count = goods_list.getCount();
				{
					PPTransaction tra(1);
					THROW(tra);
					PPWaitStart();
					//
					// ГЕНЕРАЦИЯ ТОВАРНЫХ ДОКУМЕНТОВ
					//
					for(j = 0; j < bills_count; j++) {
						PPBillPacket bpack;
						//случ. выбор ID операции из массива доступных операций с условием
						//принадлежания этой операции к типу доход или расход
						while(op_type != PPOPT_GOODSRECEIPT && op_type != PPOPT_GOODSEXPEND){
							if(allow_opscount !=0) {
								k1 = RngUniformInt(p_rng, allow_opscount);
								op_id = list.at(k1).OpID;
								op_type = GetOpType(op_id);
							}
						}
						//
						// создание бланка документа с выбранным типом операции
						//
						THROW(bpack.CreateBlank(op_id, 0, r_lcfg.Location, 0));
						//
						// выбор таблицы статей
						//
						GetOpCommonAccSheet(op_id, &acc_sheet_id, &acc_sheet2_id);
						if(ar_obj.P_Tbl->GetListBySheet(acc_sheet_id, &contragent_list, &contragent_count) > 0) { // массив контрагентов
							bpack.Rec.Object = contragent_list.at(RngUniformInt(p_rng, contragent_count)); //запись контрагента
							contragent_list.freeAll(); // очищает массив конграгентов и освобождает память
						}
						//
						// примечание у генерируемого документа
						//
						// @v11.1.12 sprintf(bpack.Rec.Memo, "//@autogen");
						bpack.SMemo = "//@autogen"; // @v11.1.12
						//
						// кол-во строк в генерируемом документе
						//
						int billrows_count = 0;
						for(k = 0; k < list.getCount(); k++) {
							BillStatFunc element = list.at(k);
							if(element.OpID == op_id){
								billrows_count = element.AvgLines;
								k = list.getCount();
							}
						}
						//
						// заполнение созданного бланка документа товарными строками
						//
						for(i = 0; i < billrows_count; i++) {
							PPID   goods_id = 0;
							PPTransferItem ti;
							ILTI   ilti;
							int    sign = 0;
							ReceiptTbl::Rec rcpt_rec;
							GoodsRestParam rest_param;
							Transfer trnsf;
							double rest = 0;
							//
							// выбор ID товара с использованием генератора по экспон. з-ну
							// и учетом остатка товара по данному ID
							//
							while(rest <= 0){
								k2 = count + 1;
								while(k2 > count)
									k2 = generator.Exponential(p_rng, mu);
								goods_id = goods_list.at((uint)k2);
								rest_param.CalcMethod = GoodsRestParam::pcmDiff;
								rest_param.Date = r_lcfg.OperDate;
								rest_param.LocID = r_lcfg.Location;
								rest_param.GoodsID = goods_id;
								trnsf.GetRest(rest_param);
								rest = rest_param.Total.Rest;
							}
							//
							// кол-во товара
							//
							// @v11.7.10 qtty = (rest > 2) ? GetRandom(1, (int)rest) : rest;
							qtty = (rest > 2) ? SLS.GetTLA().Rg.GetUniformIntPos(static_cast<int>(rest) + 1) : rest; // @v11.7.10 
							//
							// цена реализации по последнему лоту
							//
							BillObj->trfr->Rcpt.GetCurrentGoodsPrice(goods_id, r_lcfg.Location, GPRET_INDEF, &price, &rcpt_rec);
							cost = rcpt_rec.Cost; //цена поступления по товару
							//ti.Discount = GetRandom(0, 10); //скидка по товару
							//знак документа
							sign = (op_type == PPOPT_GOODSRECEIPT) ? 1 : -1;
							//добавление товарной строки
							ilti.Setup(goods_id, sign, qtty, cost, price);
							BillObj->ConvertILTI(&ilti, &bpack, 0, 0, 0);
							THROW(bpack.InitAmounts());
						}
						//добавление документа
						THROW(BillObj->TurnPacket(&bpack, 0));
						//logger.Log("Bill was added");

						op_id = 0;
						op_type = 0;
						//сообщение ожидания
						(msg_buf = "Генерация товарных документов").Transf(CTRANSF_UTF8_TO_INNER);
						PPWaitPercent(++iter, (ulong)bills_count, msg_buf);
					}
					THROW(tra.Commit());
				}
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	ZDELETE(p_vop);
	ZDELETE(p_vgoods);
	PPWaitStop();
	if(p_rng)
		RngFree(p_rng);
	//
	// показать созданные документы
	//
	if(ok > 0) {
		BillFilt filt;
		filt.Period.low = filt.Period.upp = r_lcfg.OperDate;
		ViewGoodsBills(&filt, 1);
	}
	return ok;
}	// } turistti change @v5.3.9
//
//
//
static int _StringFuncSS(const char * pStr)
{
	SString buf;
	(buf = pStr).Cat("abcdefg-012345");
	return (!buf.IsEqiAscii("xyz"));
}

static int _StringFuncSP(const char * pStr, SStringPool * pSp)
{
	uint   spp = 0;
	SString & buf = *pSp->Alloc(&spp);
	(buf = pStr).Cat("abcdefg-012345");
	int ret = (!buf.IsEqiAscii("xyz"));
	pSp->Free(spp);
	return ret;
}

static int _StringFuncC(const char * pStr)
{
	char buf[512];
	strcat(strcpy(buf, pStr), "abcdefg-012345");
	return !sstreqi_ascii(buf, "xyz");
}

int TestSStringPerf()
{
	SString test_buf;
	PROFILE_START
	for(long i = 1; i < 1000000; i++) {
    	test_buf.Z().CatLongZ(i, 20);
		_StringFuncSS(test_buf);
    }
    PROFILE_END
	PROFILE_START
	for(long i = 1; i < 1000000; i++) {
    	test_buf.Z().CatLongZ(i, 20);
		_StringFuncC(test_buf);
    }
    PROFILE_END
	{
		SStringPool sp;
		PROFILE_START
		for(long i = 1; i < 1000000; i++) {
    		test_buf.Z().CatLongZ(i, 20);
			_StringFuncSP(test_buf, &sp);
		}
		PROFILE_END
	}
    return 1;
}
//
// Construction tests
// Ситуативные тесты, доступные через команду рабочего стола
//
//#ifndef NDEBUG // @construction {
#if 1
//
//
//
#include <..\SLib\gumbo\gumbo.h>
#include <..\SLib\gumbo\gumbo-internal.h>

static bool IsTextContainsSpacesOnly(const SString & rText)
{
	bool result = true;
	const size_t len = rText.Len();
	if(len) {
		for(uint i = 0; result && i < len; i++) {
			if(!oneof4(rText.C(i), ' ', '\t', '\xD', '\xA'))
				result = false;
		}
	}
	return result;
}

static void PreprocessText(SString & rText)
{
	rText.Strip();
	while(oneof2(rText.C(0), '\xD', '\xA'))
		rText.ShiftLeft();
	rText.Chomp();
	rText.ElimDblSpaces();
	rText.Strip();
}

enum {
	pgnfTextOnly      = 0x0001,
	pgnfSkipEmptyText = 0x0002,
	pgnfTagsOnly      = 0x0004, // Выводит только имена тегов
	pgnfSkipScript    = 0x0008,
};

static void PrintGumboNode(const GumboNode * pN, uint tabN, uint flags, SFile & rF, SString & rTempBuf)
{
	if(pN) {
		SString tag_buf;
		if(pN->type == GUMBO_NODE_DOCUMENT) {
			const GumboDocument & r_doc = pN->v.document;
			if(!(flags & pgnfTextOnly)) {
				rTempBuf.Z().Tab(tabN).CatEq("doc", r_doc.name);
				rF.WriteLine(rTempBuf.CR());
			}
			for(uint i = 0; i < r_doc.children.length; i++) {
				GumboNode * p_node = static_cast<GumboNode *>(r_doc.children.data[i]);
				PrintGumboNode(p_node, tabN+1, flags, rF, rTempBuf); // @recursion
			}
		}
		else if(pN->type == GUMBO_NODE_ELEMENT) {
			const GumboElement & r_el = pN->v.element;
			const char * p_tag = gumbo_normalized_tagname(r_el.tag);
			tag_buf.Z();
		    if(!isempty(p_tag)) {
				tag_buf.Cat(p_tag);
		    }
		    else {
			    if(r_el.original_tag.data && r_el.original_tag.length) {
				    GumboTagFromOriginalTextC(&r_el.original_tag, tag_buf);
					//tag_buf.CatN(r_el.original_tag.data, r_el.original_tag.length);
			    }
		    }
			if(!(flags & pgnfSkipScript) || !tag_buf.IsEqiAscii("script")) {
				if(!(flags & pgnfTextOnly)) {
					rTempBuf.Z().Tab(tabN).CatEq("element", tag_buf);
					rF.WriteLine(rTempBuf.CR());
				}
				//
				if(r_el.attributes.length) {
					if(!(flags & pgnfTextOnly) && !(flags & pgnfTagsOnly)) {
						rTempBuf.Z().Tab(tabN).Cat("attributes").Colon();
						rF.WriteLine(rTempBuf.CR());
						for(uint i = 0; i < r_el.attributes.length; i++) {
							GumboAttribute * p_attr = static_cast<GumboAttribute *>(r_el.attributes.data[i]);
							if(p_attr) {
								rTempBuf.Z().Tab(tabN+1).CatEq(p_attr->name, p_attr->value);
								rF.WriteLine(rTempBuf.CR());
							}
						}
					}
				}
				if(r_el.children.length) {
					if(!(flags & pgnfTextOnly) && !(flags & pgnfTagsOnly)) {
						rTempBuf.Z().Tab(tabN).Cat("children").Colon();
						rF.WriteLine(rTempBuf.CR());
					}
					for(uint i = 0; i < r_el.children.length; i++) {
						GumboNode * p_node = static_cast<GumboNode *>(r_el.children.data[i]);
						PrintGumboNode(p_node, tabN+1, flags, rF, rTempBuf); // @recursion
					}
				}
			}
		}
		else if(!(flags & pgnfTagsOnly)) {
			const GumboText & r_t = pN->v.text;
			tag_buf = r_t.text;
			if(IsTextContainsSpacesOnly(tag_buf) && !(flags & pgnfSkipEmptyText)) {
				rTempBuf.Z().Tab(tabN+1).Cat("text").CatDiv(':', 2).Cat("(spaces)");
			}
			else {
				PreprocessText(tag_buf);
				rTempBuf.Z().Tab(tabN+1).Cat("text").CatDiv(':', 2).Cat(tag_buf);
			}
			rF.WriteLine(rTempBuf.CR());
		}
	}
}

static void PrintGumboOutput(const GumboOutput * pGo, uint tabN, uint flags, SFile & rF, SString & rTempBuf)
{
	if(pGo) {
		if(pGo->errors.length) {
			rF.WriteLine(rTempBuf.Z().Cat("Errors").Colon().CR());
			for(uint i = 0; i < pGo->errors.length; i++) {
				//gumbo_error_to_string(GumboParser * parser, const GumboError* error, GumboStringBuffer* output) 
				const GumboError * p_err = static_cast<const GumboError *>(pGo->errors.data[i]);
				//rF.WriteLine(rTempBuf.Z().Tab().Cat(p_err->text).CR());
			}
		}
		PrintGumboNode(pGo->document, 0, flags, rF, rTempBuf);
	}
}

static int LoadHttpPage(const char * pUrl, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString temp_buf;
	InetUrl url;
	THROW(!isempty(pUrl));
	THROW(url.Parse(pUrl));
	{
		/*
			user-agent:
			Mozilla/5.0 (Windows NT 10.0; Win64; x64) 
			AppleWebKit/537.36 (KHTML, like Gecko) 
			Chrome/94.0.4606.81 
			Safari/537.36
		*/ 
		ScURL c;
		SBuffer in_buffer;
		SFile wr_stream(in_buffer, SFile::mWrite);
		StrStrAssocArray hdr_flds;
		SFileFormat::GetMime(SFileFormat::Html, temp_buf);
		temp_buf.CatDiv(';', 2).CatEq("charset", "utf-8");
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrUserAgent, "Mozilla/5.0");
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrCacheControl, "no-cache");
		//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAcceptLang, "ru");
		//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAccept, "application/json");
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, /*&hdr_flds*/0, &wr_stream));
		{
			SBuffer * p_in_buf = static_cast<SBuffer *>(wr_stream);
			if(p_in_buf) {
				rBuf.Z().CatN(p_in_buf->GetBufC(), p_in_buf->GetAvailableSize());
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

class HtmlParsingRule {
public:
	enum {
		tName = 1,
		tUrl,           // url
		tItem,          // item
		tDetailEntry,   // detail
		tPageEntry,     // pager
		tNextPageEntry, // nextpage
	};
	//
	// Descr: Отдельный терм фильтрации 
	//
	class Filter {
	public:
		Filter() : P_Child(0)
		{
		}
		~Filter()
		{
			ZDELETE(P_Child);
		}
		Filter & Z()
		{
			Tag.Z();
			AttrL.Z();
			TextPrefix.Z();
			ZDELETE(P_Child);
			return *this;
		}
		SString Tag;
		StrStrAssocArray AttrL;
		SString TextPrefix;
		Filter * P_Child;
	};
	class Entry {
	public:
		Entry() : Token(0)
		{
		}
		int    Token;
		SString Field;
		SString Text; // url or original filter description
		Filter F;
	};
	HtmlParsingRule()
	{
	}
	struct FilterResult {
		FilterResult() : P_Node(0)
		{
		}
		FilterResult(const FilterResult & rS) : P_Node(rS.P_Node), Text(rS.Text)
		{
		}
		FilterResult & FASTCALL operator = (const FilterResult & rS) { return Copy(rS); }
		FilterResult & FASTCALL Copy(const FilterResult & rS)
		{
			P_Node = rS.P_Node;
			Text = rS.Text;
			return *this;
		}
		const GumboNode * P_Node; // @notowned
		SString Text;
	};
	int FindFilter(const GumboNode * pN, const Filter * pF, TSCollection <FilterResult> & rResult) const
	{
		int ok = 1;
		if(pN && pF) {
			if(pN->type == GUMBO_NODE_DOCUMENT) {
				for(uint i = 0; i < pN->v.document.children.length; i++) {
					THROW(FindFilter(static_cast<const GumboNode *>(pN->v.document.children.data[i]), pF, rResult)); // @recursion
				}
			}
			else if(pN->type == GUMBO_NODE_ELEMENT) {
				const char * p_tag = gumbo_normalized_tagname(pN->v.element.tag);
				bool sat = true;
				SString result_text;
				if(pF->Tag.IsEqiAscii(p_tag)) {
					if(pF->AttrL.getCount()) {
						for(uint i = 0; sat && i < pF->AttrL.getCount(); i++) {
							SStrToStrAssoc a = pF->AttrL.at(i);
							const GumboAttribute * p_attr = gumbo_get_attribute(&pN->v.element.attributes, a.Key);
							if(p_attr) {
								if(sstreq(a.Val, "?")) {
									result_text = a.Val;
								}
								else if(sstreqi_ascii(p_attr->value, a.Val)) {
									;
								}
								else
									sat = false;
							}
							else {
								sat = false;
							}
						}
					}
				}
				else
					sat = false;
				if(sat) {
					FilterResult * p_result_item = rResult.CreateNewItem();
					THROW_SL(p_result_item);
					p_result_item->P_Node = pN;
					if(result_text)
						p_result_item->Text = result_text;
					else {
						// @todo Воткнуть в p_result->Text текстовое представление узла P_Node
					}
				}
				{
					for(uint i = 0; i < pN->v.element.children.length; i++) {
						THROW(FindFilter(static_cast<const GumboNode *>(pN->v.element.children.data[i]), pF, rResult)); // @recursion
					}
				}
			}
		}
		CATCHZOK
		return ok;
	}
	int    ParseFilter(const char * pStr, Filter & rF)
	{
		// syntax: tag-symb ['(' attr-symb ':' attr-value [';' attr-symb ':' attr-value] ')']
		// Специальный символ атрибута '%text-prefix' трактуется как текстовый префикс в теле
		// тега, предшествующий искомым данным.
		int    ok = 1;
		THROW(!isempty(pStr));
		{
			SString temp_buf;
			SString attr_buf;
			SString val_buf;
			SStrScan scan(pStr);
			scan.Skip();
			THROW(scan.GetIdentWithHyphen(rF.Tag));
			scan.Skip();
			if(scan[0] == '(') {
				bool do_go_next = false;
				do {
					do_go_next = false;
					attr_buf.Z();
					val_buf.Z();
					scan.Incr();
					scan.Skip();
					THROW(scan.GetIdentWithHyphen(attr_buf));
					scan.Skip();
					if(scan[0] == ':') {
						scan.Incr();
						scan.Skip();
						if(scan.GetQuotedString(val_buf)) {
							;
						}
						else if(scan.GetUntil(';', val_buf)) {
							val_buf.Strip();
							do_go_next = true;
						}
						else if(scan.GetUntil(')', val_buf)) {
							val_buf.Strip();
						}
						else {
							CALLEXCEPT();
						}
					}
					else if(scan[0] == ';') {
						do_go_next = true;
					}
					else if(scan[0] == ')') {
					}
					else {
						CALLEXCEPT();
					}
					rF.AttrL.Add(attr_buf, val_buf, 1);
				} while(do_go_next);
			}
			else if(scan.Get("->", temp_buf)) {
				// @todo
			}
			else {
				THROW(scan.IsEnd());
			}
		}
		CATCHZOK
		return ok;
	}
	int SetEntry(const char * pKey, const char * pValue)
	{
		int    ok = 1;
		if(sstreqi_ascii(pKey, "url")) {
			THROW(!isempty(pValue));
			{
				Entry * p_entry = L.CreateNewItem();
				THROW_SL(p_entry);
				p_entry->Token = tUrl;
				p_entry->Text = pValue;
			}
		}
		else {
			int    token = 0;
			SString field;
			if(sstreqi_ascii(pKey, "item")) {
				token = tItem;
			}
			else if(sstreqi_ascii(pKey, "detail")) {
				token = tDetailEntry;
			}
			else if(sstreqi_ascii(pKey, "pager")) {
				token = tPageEntry;
			}
			THROW(token);
			THROW(!isempty(pValue));
			{
				uint entry_pos = 0;
				Entry * p_entry = L.CreateNewItem(&entry_pos);
				THROW_SL(p_entry);
				p_entry->Token = token;
				p_entry->Field = field;
				p_entry->Text = pValue;
				if(ParseFilter(pValue, p_entry->F)) {
					; // ok
				}
				else {
					L.atFree(entry_pos);
					CALLEXCEPT();
				}
			}
		}
		CATCHZOK
		return ok;
	}
	bool   GetUrl(SString & rUrlBuf) const
	{
		rUrlBuf.Z();
		bool    ok = false;
		for(uint i = 0; i < L.getCount(); i++) {
			const Entry * p_entry = L.at(i);
			if(p_entry && p_entry->Token == tUrl) {
				rUrlBuf = p_entry->Text;
				ok = true;
			}
		}
		return ok;
	}
	SString Symb;
	TSCollection <Entry> L;
	//TSCollection <Filter> FiltL; // Список термов фильтрации контента для данного правила
};

class HtmlParsingRuleSet : public TSCollection <HtmlParsingRule> {
public:
	HtmlParsingRuleSet() : TSCollection <HtmlParsingRule>()
	{
	}
	int    ReadIni(const char * pIniFileName)
	{
		int    ok = -1;
		SString temp_buf;
		SString sect_buf;
		SString param_buf;
		SString value_buf;
		StringSet ss_sections;
		StringSet ss_entries;
		SIniFile ini_file(pIniFileName);
		THROW(ini_file.IsValid());
		ini_file.GetSections(&ss_sections);
		for(uint ssp = 0; ss_sections.get(&ssp, sect_buf);) {
			ini_file.GetEntries(sect_buf, &ss_entries, 0);
			if(ss_entries.getCount()) {
				HtmlParsingRule * p_rule = new HtmlParsingRule();
				THROW_SL(p_rule);
				bool is_there_err = false;
				for(uint ssp2 = 0; ss_entries.get(&ssp2, param_buf);) {
					if(ini_file.GetParam(sect_buf, param_buf, value_buf) > 0) {
						if(!p_rule->SetEntry(param_buf, value_buf)) {
							is_there_err = true;
							; // @todo @err
						}
					}
				}
				if(is_there_err) {
					delete p_rule;
				}
				else {
					p_rule->Symb = sect_buf;
					insert(p_rule);
					ok = 1;
				}
			}
		}
		CATCHZOK
		return ok;
	}
};

static void GumboTest()
{
	GumboOutput * p_output = 0;
	const GumboOptions go = kGumboDefaultOptions;
	//SString input_buf;
	SString temp_buf;
	SString html_buf;
	HtmlParsingRuleSet prset;

	PPGetPath(PPPATH_TESTROOT, temp_buf);
	temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("htmlparsing-rule-set.ini");
	if(prset.ReadIni(temp_buf)) {
		SString url_buf;
		SString _item_rule;
		for(uint i = 0; i < prset.getCount(); i++) {
			const HtmlParsingRule * p_rule = prset.at(i);
			if(p_rule) {
				p_rule->GetUrl(url_buf);
				//p_rule->GetToken(HtmlParsingRule::tItem, _item_rule);
				if(url_buf.NotEmptyS()) {
					if(LoadHttpPage(url_buf, html_buf)) {
						p_output = gumbo_parse_with_options(&go, html_buf, html_buf.Len());
						if(p_output && p_output->root) {
							{
								PPGetPath(PPPATH_TESTROOT, temp_buf);
								temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat(p_rule->Symb).Dot().Cat("out");
								SFile f_out(temp_buf, SFile::mWrite);
								uint pgn_flags = 0;
								PrintGumboOutput(p_output, 0, pgn_flags, f_out, temp_buf);
							}
							for(uint ei = 0; ei < p_rule->L.getCount(); ei++) {
								const HtmlParsingRule::Entry * p_entry = p_rule->L.at(ei);
								if(p_entry) {
									if(p_entry->Token == HtmlParsingRule::tItem) {
										TSCollection <HtmlParsingRule::FilterResult> frl;
										p_rule->FindFilter(p_output->document, &p_entry->F, frl);
										if(frl.getCount()) {
										}
									}
								}
							}
						}
						//
						gumbo_destroy_output(p_output);
						p_output = 0;
					}
				}
			}
		}
	}

	const char * p_addr_list[] = {
		"https://rutor.org.in/",
		"https://holdingbeauty.ru/catalog/",
		"https://holdingbeauty.ru/catalog/ukhod-za-volosami/"
	};
	for(uint i = 0; i < SIZEOFARRAY(p_addr_list); i++) {
		const char * p_url = p_addr_list[i];
		if(LoadHttpPage(p_url, html_buf)) {
			p_output = gumbo_parse_with_options(&go, html_buf, html_buf.Len());
			{
				PPGetPath(PPPATH_TESTROOT, temp_buf);
				temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("html").CatChar('-').CatLongZ(i+1, 3).Cat("-text").Dot().Cat("out");
				SFile f_out(temp_buf, SFile::mWrite);
				uint pgn_flags = pgnfTextOnly|pgnfSkipEmptyText|pgnfSkipScript;
				PrintGumboNode(p_output->document, 0, pgn_flags, f_out, temp_buf);
			}
			{
				PPGetPath(PPPATH_TESTROOT, temp_buf);
				temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("html").CatChar('-').CatLongZ(i+1, 3).Cat("-tags").Dot().Cat("out");
				SFile f_out(temp_buf, SFile::mWrite);
				uint pgn_flags = pgnfTagsOnly|pgnfSkipEmptyText;
				PrintGumboNode(p_output->document, 0, pgn_flags, f_out, temp_buf);
			}
			//
			gumbo_destroy_output(p_output);
			p_output = 0;
		}
	}
	//
	PPGetPath(PPPATH_TESTROOT, temp_buf);
	if(temp_buf.NotEmpty()) {
		temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("html5_spec.html");
		SFile f_in(temp_buf, SFile::mRead);
		if(f_in.IsValid()) {
			int64 fsz = 0;
			f_in.CalcSize(&fsz);
			if(fsz) {
				STempBuffer in_buf(static_cast<size_t>(fsz) + 64);
				size_t actual_size = 0;
				f_in.Read(in_buf, static_cast<size_t>(fsz), &actual_size);
				if(actual_size) {
					p_output = gumbo_parse_with_options(&go, in_buf, actual_size);
					if(p_output && p_output->document) {
						PPGetPath(PPPATH_TESTROOT, temp_buf);
						temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("html5_spec.out");
						SFile f_out(temp_buf, SFile::mWrite);
						uint pgn_flags = 0;
						PrintGumboNode(p_output->document, 0, pgn_flags, f_out, temp_buf);
					}
				}
			}
		}
	}
	gumbo_destroy_output(p_output);
}
//
//
//
int TestSuffixTree(); //
int  TestFann2();
int  LuaTest();
int  CollectLldFileStat();
int  ParseCpEncodingTables(const char * pPath, SUnicodeTable * pUt);
int  TestLargeVlrInputOutput();
void Test_SSystemBackup();
//void Test_MailMsg_ReadFromFile();
//void Test_MakeEmailMessage();
int  PPReadUnicodeBlockRawData(const char * pUnicodePath, const char * pCpPath, SUnicodeBlock & rBlk);
void TestCRC();
int  TestUhttClient();
int  TestReadXmlMem_EgaisAck();
int  TestGtinStruc();
int  TestConfigDatabase_StringHistory();
int  TestConfigDatabase_StringHistory_Interactive();
void TestGravity();
int  TestAddressRecognition(); 
int  TestTsDensityMap(); // @debug
int  TestUdsInterface();
int  SrpTest();
int  Test_StyloQInvitation();
// @construction finished int  Test_Launch_SCalendarPicker();
int  Test_StrAssocTree();
int  Test_Fts();
//int  Test_ReadUed(const char * pFileName);
int  Test_ListSelectionDialog();
int  ImportYYE(const char * pSrcPath);
int  ImportSpecial(const char * pPath);
int  ReformatRazoomnick(const char * pFileName);
int  SentencePieceExperiments(); // txtanlz.cpp
const char * Test_GetPPConstCharPtr_P_MagicFileTransmit() { return PPConst::P_MagicFileTransmit; }

/*static int TestWorkspacePath()
{
	SString path;
	PPGetPath(PPPATH_WORKSPACE, path);
	return BIN(path.NotEmpty());
}*/

//int Test_LCMS2(const char * pTestBedPath, const char * pOutputFileName, bool exhaustive); // @v10.9.7 (Экспериментальное внедрение тестирования библиотеки lcms2) 
int DoTest_PThr4w();

static void Test_LibPhoneNumber()
{
#if(_MSC_VER >= 1900)
	for(uint i = 0; i < 10; i++) {
		SLibPhoneNumber pn;
		int r4 = pn.Parse("(8142) 330 660", "RU");
		int r1 = pn.Parse("+7(921)700-29-87", "RU");
		int r2 = pn.Parse("8(921)700-29-87", "RU");
		int r3 = pn.Parse("+7(921)700-29-87", 0);
		assert(r1 && r2 && r3 && r4);
	}
#endif
}

//void TestPow10Tab(); // prototype(dconvstr.c)

static int TestTransferFileToFtp() // @v12.2.5 проверка отправки файла с длинным именем на ftp. Пользователи утверждают, что с этим есть проблемы.
{
	int    ok = 1;
	const  char * p_src_file_path = "D:/Papyrus/__TEMP__/ON_NSCHFDOPPRMARK_2BM-100100183632--2015091610525162883960000000_2BM-100100910817-20121218104345889223900000000_20250127_32013858-FEB4-49DC-8B6F-5D5B5CEDE240.xml";
	const  char * p_ftp_acc_symb = "Rekish-Prodgrupp";
	PPID   ftp_acc_id = 0;
	if(fileExists(p_src_file_path)) {
		PPObjInternetAccount ia_obj;
		if(ia_obj.SearchBySymb(p_ftp_acc_symb, &ftp_acc_id, 0) > 0) {
			PPInternetAccount2 ia_pack;
			if(ia_obj.Get(ftp_acc_id, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount2::fFtpAccount) {
				SString ftp_path;
				SString naked_file_name;
				{
					SFsPath ps(p_src_file_path);
					ps.Merge(SFsPath::fNam|SFsPath::fExt, naked_file_name);
					ia_pack.GetExtField(FTPAEXSTR_HOST, ftp_path);
				}
				{
					SUniformFileTransmParam param;
					SString accs_name;
					char   pwd[256];
					(param.SrcPath = p_src_file_path).Transf(CTRANSF_OUTER_TO_UTF8);
					SFsPath::NormalizePath(ftp_path, SFsPath::npfSlash|SFsPath::npfKeepCase, param.DestPath);
					param.Flags = 0;
					param.Format = SFileFormat::Unkn;
					ia_pack.GetExtField(FTPAEXSTR_USER, accs_name);
					ia_pack.GetPassword_(pwd, sizeof(pwd), FTPAEXSTR_PASSWORD);
					param.AccsName.EncodeUrl(accs_name, 0);
					param.AccsPassword.EncodeUrl(pwd, 0);
					memzero(pwd, sizeof(pwd));
					if(param.Run(0, 0))
						ok = 1;
				}
			}
		}
	}
	return ok;
}

int DoConstructionTest()
{
	int    ok = -1;
	{
		uint abc = 0x01020304U;
		//uint abc_s = sbswap32()
	}
#ifndef NDEBUG
//#if 1
	/*{
		class Foo {
		public:
			Foo() : A(1), B(-1.0)
			{
			}
			~Foo()
			{
				A = 0;
				B = 0.0;
			}
			uint   A;
			double B;
		};
		std::unique_ptr <Foo> ptr(new Foo);
		std::unique_ptr <Foo> ptr2;
		if(ptr->A != ptr->B) {
			ptr2 = std::move(ptr);
		}
		char str_buf[128];
		{
			std::string s1;
			s1 = "This is a sample string";
			strcpy(str_buf, s1.c_str());
		}
		ptr = std::move(ptr2);
	}*/
	/*{
		LARGE_INTEGER qpc1;
		LARGE_INTEGER qpc2;
		uint64 c1 = clock();
		QueryPerformanceCounter(&qpc1);
		SDelay(10000);
		uint64 c2 = clock();
		QueryPerformanceCounter(&qpc2);
		SDelay(1);
	}*/
	{ // @v10.9.7 Экпериментальное внедрение тестирования библиотеки lcms2
//#if _MSC_VER >= 1910
		//SString out_file_name;
		//SString testbed_path("/Papyrus/Src/OSF/lcms2/testbed");
		//PPGetFilePath(PPPATH_OUT, "lcms2-test.out", out_file_name);
		//Test_LCMS2(testbed_path, out_file_name, true);
//#endif
	}
#if(_MSC_VER >= 1900)
	//Test_Fts();
#endif
#if 0 // {
	{
		SString get_vs_inst_msg;
		//get_vs_installations(/*instance_callback callback,*/&get_vs_inst_msg);
		TSCollection <VisualStudioInstallationLocator::Entry> vs_entry_list;
		if(VisualStudioInstallationLocator::Locate(vs_entry_list, &get_vs_inst_msg)) {
			for(uint i = 0; i < vs_entry_list.getCount(); i++) {
				VisualStudioInstallationLocator::Entry * p_entry = vs_entry_list.at(i);
				if(p_entry) {
					
				}
			}
		}
	}
#endif // } 0
	//TestTransferFileToFtp();
	Test_ListSelectionDialog();
	//SentencePieceExperiments();
	//TestGtinStruc();
	//PPChZnPrcssr::Test();
	//GumboTest();
	//Test_SSystemBackup();
	//TestPow10Tab();
	//ImportSpecial("D:\\DEV\\RESOURCE\\DATA\\ETC");
	//Test_ReadUed("\\Papyrus\\Src\\Rsrc\\Data\\Sartre\\UED.txt");
	//SDecimal::Test();
	//ReformatRazoomnick("D:/Papyrus/Universe-HTT/DATA/Razoomnick-barcodes.csv");	
	//PPStyloQInterchange::PrepareAhed(true);
	//TestCRC();
	//PPStyloQInterchange::ExecuteIndexingRequest(true/*useCurrentSession*/);
	//ImportYYE("/DEV/Resource/Data/yeda");
	//DoTest_PThr4w();
	//TestMqc();
	/*{
		SString _cmd;
		SString _svcident;
		SString _svcident2;
		SBinaryChunk bc;
		SBinaryChunk bc2;
		const char * p_json_text = "{\"cmd\":\"GetForeignConfig\",\"foreignsvcident\":\"4RA8CYgG0PzF\\/bVebwsJ80qBa5g=\"}";
		SJson * p_json = SJson::Parse(p_json_text);
		assert(p_json);
		assert(SJson::IsObject(p_json));
		if(p_json->P_Child) {
			for(const SJson * p_cur = p_json->P_Child; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Text.IsEqiAscii("cmd")) {
					if(p_cur->P_Child)
						_cmd = p_cur->P_Child->Text;
				}
				else if(p_cur->Text.IsEqiAscii("foreignsvcident")) {
					if(p_cur->P_Child) {
						_svcident = p_cur->P_Child->Text;
						(_svcident2 = _svcident).Unescape();
						bc.FromMime64(_svcident);
						bc2.FromMime64(_svcident2);
					}
				}
			}
		}
		ZDELETE(p_json);
	}*/
	//Test_LibPhoneNumber();
	//Test_StyloQInvitation();
	//Test_StrAssocTree();
	// @construction finished Test_Launch_SCalendarPicker();
	/*{
		SSecretTagPool stp;
		stp.GeneratePrivateKey(2048);
	}*/
	//SrpTest();
	#if 0 // @construction {
	{
		SConsole con;
		if(con.Open("Test Console")) {
			fprintf(stdout, "Test console output\nDelay=10 sec");
			fflush(stdout);
			SDelay(10000);
		}
		con.Close();
	}
	#endif // } 0 @construction
	//TestUdsInterface();
	//TestTsDensityMap();
	//TestAddressRecognition();
	//TestGravity();
	//TestConfigDatabase_StringHistory();
	//TestConfigDatabase_StringHistory_Interactive();
	//TestWorkspacePath();
	//TestReadXmlMem_EgaisAck();
	//TestUhttClient();
	//Test_MailMsg_ReadFromFile();
	//LuaTest();
	//TestFann2();
	//Test_MakeEmailMessage();
	//CollectLldFileStat();
#endif
	//PPWhatmanWindow::Launch("D:/PAPYRUS/Src/PPTEST/DATA/test04.wtm");
	//PPWhatmanWindow::Edit("D:/PAPYRUS/Src/PPTEST/DATA/test04.wtm", "D:/PAPYRUS/Src/PPTEST/DATA/test02.wta");
	//TestSuffixTree();
	{
		//TSCollection <PPBarcode::Entry> bc_list;
		//PPBarcode::RecognizeImage("D:/Papyrus/Src/OSF/ZBAR/examples/barcode.png", bc_list);
		//PPBarcode::RecognizeImage("D:/Papyrus/ppy/out/040-69911566-57N00001CPQ0LN0GLBP1O9R30603031000007FB116511C6E341B4AB38FB95E24B46D.png", bc_list);
		//PPBarcode::RecognizeImage("D:/Papyrus/ppy/out/460622403878.png", bc_list);
	}
#if 0 // {
	{
		SString map_pool_file_name;
		SString map_transl_file_name;
		PPGetFilePath(PPPATH_OUT, "SCodepageMapPool.txt", map_pool_file_name);
		PPGetFilePath(PPPATH_OUT, "SCodepageMapTransl.txt", map_transl_file_name);
		//
		SUnicodeTable ut;
		ut.ParseSource("d:/Papyrus/Src/Rsrc/unicodedata");
		ParseCpEncodingTables("d:/papyrus/src/rsrc/data/cp", &ut);

		SUnicodeBlock ub;
		//ub.ReadRaw("d:/Papyrus/Src/Rsrc/unicodedata", "d:/papyrus/src/rsrc/data/cp");
		PPReadUnicodeBlockRawData("d:/Papyrus/Src/Rsrc/unicodedata", "d:/papyrus/src/rsrc/data/cp", ub);
		ub.Cpmp.Test(&ub.Ut, map_pool_file_name, map_transl_file_name);
		ub.Write("d:/papyrus/__temp__/ub.bin");
		{
			SUnicodeBlock ub2;
			ub2.Read("d:/papyrus/__temp__/ub.bin");
			ub2.Cpmp.Test(&ub2.Ut, map_pool_file_name, map_transl_file_name);
		}
	}
#endif // } 0
	//TestLargeVlrInputOutput();
	return ok;
}

#else // }{

int DoConstructionTest()
{
	int    ok = -1;
	return ok;
}

#endif // } 0 @construction
//
//
//
PPTestDbInfrastructure::PPTestDbInfrastructure()
{
}

PPTestDbInfrastructure::~PPTestDbInfrastructure()
{
}

int PPTestDbInfrastructure::SetupDatabaseObjects()
{
	// Здесь надо будет создать необходимые объекты данных.
	int    ok = 1;
	return ok;
}

int PPTestDbInfrastructure::_Case_TaxEvaluation()
{
	int    ok = -1;
	/*
		План теста:
			-- Создаем налоговую группу
			-- Создаем товарную позицию с заданной налоговой группой
	*/ 
	return ok;
}
