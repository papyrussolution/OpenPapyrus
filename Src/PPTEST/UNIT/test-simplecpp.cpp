// simplecpp - A simple and high-fidelity C/C++ preprocessor library
// Copyright (C) 2016-2023 simplecpp team
// 
#include <pp.h>
#pragma hdrstop
#include <..\slib\simplecpp\simplecpp.h>

// @sobolev #define STRINGIZE_(x) #x
// @sobolev (already defined at slib.h) #define STRINGIZE(x) STRINGIZE_(x)

static int numberOfFailedAssertions = 0;

#define ASSERT_EQUALS(expected, actual)  (assertEquals((expected), (actual), __LINE__))
#define ASSERT_THROW_EQUALS(stmt, e, \
	    expected) do { try { stmt; assertThrowFailed(__LINE__); } catch(const e& ex) { assertEquals((expected), (ex.what()), __LINE__); } } while(false)

static std::string pprint(const std::string &in)
{
	std::string ret;
	for(std::string::size_type i = 0; i < in.size(); ++i) {
		if(in[i] == '\n')
			ret += "\\n";
		ret += in[i];
	}
	return ret;
}

static int assertEquals(const std::string & rExpected, const std::string & rActual, int line)
{
	if(rExpected != rActual) {
		numberOfFailedAssertions++;
		std::cerr << "------ assertion failed ---------" << std::endl;
		std::cerr << "line " << line << std::endl;
		std::cerr << "expected:" << pprint(rExpected) << std::endl;
		std::cerr << "actual:" << pprint(rActual) << std::endl;
	}
	return (rExpected == rActual);
}

static int assertEquals(const long long &expected, const long long &actual, int line)
{
	return assertEquals(std::to_string(expected), std::to_string(actual), line);
}

static void assertThrowFailed(int line)
{
	numberOfFailedAssertions++;
	std::cerr << "------ assertion failed ---------" << std::endl;
	std::cerr << "line " << line << std::endl;
	std::cerr << "exception not thrown" << std::endl;
}

static void testcase(const std::string &name, void (*f)(), int argc, char * const * argv)
{
	if(argc == 1)
		f();
	else {
		for(int i = 1; i < argc; i++) {
			if(name == argv[i])
				f();
		}
	}
}

#define TEST_CASE(F)    (testcase(#F, F, argc, argv))

static simplecpp::TokenList makeTokenList(const char code[], std::size_t size, std::vector<std::string> &filenames, const std::string &filename = std::string(), simplecpp::OutputList * outputList = nullptr)
{
	std::istringstream istr(std::string(code, size));
	return simplecpp::TokenList(istr, filenames, filename, outputList);
}

static simplecpp::TokenList makeTokenList(const char code[], std::vector<std::string> &filenames, const std::string &filename = std::string(), simplecpp::OutputList * outputList = nullptr)
{
	return makeTokenList(code, strlen(code), filenames, filename, outputList);
}

static std::string readfile(const char code[], simplecpp::OutputList * outputList = nullptr)
{
	std::vector<std::string> files;
	return makeTokenList(code, files, std::string(), outputList).stringify();
}

static std::string readfile(const char code[], std::size_t size, simplecpp::OutputList * outputList = nullptr)
{
	std::vector<std::string> files;
	return makeTokenList(code, size, files, std::string(), outputList).stringify();
}

static std::string preprocess(const char code[], const simplecpp::DUI &dui, simplecpp::OutputList * outputList)
{
	std::vector<std::string> files;
	std::map<std::string, simplecpp::TokenList*> filedata;
	simplecpp::TokenList tokens = makeTokenList(code, files);
	tokens.removeComments();
	simplecpp::TokenList tokens2(files);
	simplecpp::preprocess(tokens2, tokens, files, filedata, dui, outputList);
	return tokens2.stringify();
}

static std::string preprocess(const char code[]) { return preprocess(code, simplecpp::DUI(), nullptr); }
static std::string preprocess(const char code[], const simplecpp::DUI &dui) { return preprocess(code, dui, nullptr); }
static std::string preprocess(const char code[], simplecpp::OutputList * outputList) { return preprocess(code, simplecpp::DUI(), outputList); }

static std::string toString(const simplecpp::OutputList &outputList)
{
	std::ostringstream ostr;
	for(const simplecpp::Output &output : outputList) {
		ostr << "file" << output.location.fileIndex << ',' << output.location.line << ',';
		switch(output.type) {
			case simplecpp::Output::Type::ERROR_: ostr << "#error,"; break;
			case simplecpp::Output::Type::WARNING: ostr << "#warning,"; break;
			case simplecpp::Output::Type::MISSING_HEADER: ostr << "missing_header,"; break;
			case simplecpp::Output::Type::INCLUDE_NESTED_TOO_DEEPLY: ostr << "include_nested_too_deeply,"; break;
			case simplecpp::Output::Type::SYNTAX_ERROR: ostr << "syntax_error,"; break;
			case simplecpp::Output::Type::PORTABILITY_BACKSLASH: ostr << "portability_backslash,"; break;
			case simplecpp::Output::Type::UNHANDLED_CHAR_ERROR: ostr << "unhandled_char_error,"; break;
			case simplecpp::Output::Type::EXPLICIT_INCLUDE_NOT_FOUND: ostr << "explicit_include_not_found,"; break;
			case simplecpp::Output::Type::FILE_NOT_FOUND: ostr << "file_not_found,"; break;
			case simplecpp::Output::Type::DUI_ERROR: ostr << "dui_error,"; break;
		}
		ostr << output.msg << '\n';
	}
	return ostr.str();
}

static std::string testConstFold(const char code[])
{
	try {
		std::vector<std::string> files;
		simplecpp::TokenList expr = makeTokenList(code, files);
		expr.constFold();
		return expr.stringify();
	} catch(std::exception &) {
		return "exception";
	}
}

#ifdef __CYGWIN__
	static void convertCygwinPath()
	{
		// absolute paths
		ASSERT_EQUALS("X:\\", simplecpp::convertCygwinToWindowsPath("/cygdrive/x")); // initial backslash
		ASSERT_EQUALS("X:\\", simplecpp::convertCygwinToWindowsPath("/cygdrive/x/"));
		ASSERT_EQUALS("X:\\dir", simplecpp::convertCygwinToWindowsPath("/cygdrive/x/dir"));
		ASSERT_EQUALS("X:\\dir\\file", simplecpp::convertCygwinToWindowsPath("/cygdrive/x/dir/file"));

		// relative paths
		ASSERT_EQUALS("file", simplecpp::convertCygwinToWindowsPath("file"));
		ASSERT_EQUALS("dir\\file", simplecpp::convertCygwinToWindowsPath("dir/file"));
		ASSERT_EQUALS("..\\dir\\file", simplecpp::convertCygwinToWindowsPath("../dir/file"));

		// incorrect Cygwin paths
		ASSERT_EQUALS("\\cygdrive", simplecpp::convertCygwinToWindowsPath("/cygdrive"));
		ASSERT_EQUALS("\\cygdrive\\", simplecpp::convertCygwinToWindowsPath("/cygdrive/"));
	}
#endif

static void assertToken(const std::string& s, bool name, bool number, bool comment, char op, int line)
{
	const std::vector<std::string> f;
	const simplecpp::Location l(f);
	const simplecpp::Token t(s, l);
	assertEquals(name, t.name, line);
	assertEquals(number, t.number, line);
	assertEquals(comment, t.comment, line);
	assertEquals(op, t.op, line);
}

#define ASSERT_TOKEN(s, na, nu, c) assertToken(s, na, nu, c, '\0', __LINE__)
#define ASSERT_TOKEN_OP(s, na, nu, c, o) assertToken(s, na, nu, c, o, __LINE__)

//int SimpleCpp_Test_Main2()
SLTEST_R(SimpleCpp)
{
	numberOfFailedAssertions = 0;
	{ // backslash()
		// <backslash><space><newline> preprocessed differently
		simplecpp::OutputList outputList;

		readfile("//123 \\\n456", &outputList);
		ASSERT_EQUALS("", toString(outputList));
		readfile("//123 \\ \n456", &outputList);
		ASSERT_EQUALS("file0,1,portability_backslash,Combination 'backslash space newline' is not portable.\n", toString(outputList));

		outputList.clear();
		readfile("#define A \\\n123", &outputList);
		ASSERT_EQUALS("", toString(outputList));
		readfile("#define A \\ \n123", &outputList);
		ASSERT_EQUALS("file0,1,portability_backslash,Combination 'backslash space newline' is not portable.\n", toString(outputList));
	}
	{ // builtin()
		ASSERT_EQUALS("\"\" 1 0", preprocess("__FILE__ __LINE__ __COUNTER__"));
		ASSERT_EQUALS("\n\n3", preprocess("\n\n__LINE__"));
		ASSERT_EQUALS("\n\n0", preprocess("\n\n__COUNTER__"));
		ASSERT_EQUALS("\n\n0 1", preprocess("\n\n__COUNTER__ __COUNTER__"));
		ASSERT_EQUALS("\n0 + 0", preprocess("#define A(c)  c+c\n" "A(__COUNTER__)\n"));
		ASSERT_EQUALS("\n0 + 0 + 1", preprocess("#define A(c)  c+c+__COUNTER__\n" "A(__COUNTER__)\n"));
	}
	{ // characterLiteral()
		ASSERT_EQUALS('A', simplecpp::characterLiteralToLL("'A'"));

		ASSERT_EQUALS('\'', simplecpp::characterLiteralToLL("'\\''"));
		ASSERT_EQUALS('\"', simplecpp::characterLiteralToLL("'\\\"'"));
		ASSERT_EQUALS('\?', simplecpp::characterLiteralToLL("'\\?'"));
		ASSERT_EQUALS('\\', simplecpp::characterLiteralToLL("'\\\\'"));
		ASSERT_EQUALS('\a', simplecpp::characterLiteralToLL("'\\a'"));
		ASSERT_EQUALS('\b', simplecpp::characterLiteralToLL("'\\b'"));
		ASSERT_EQUALS('\f', simplecpp::characterLiteralToLL("'\\f'"));
		ASSERT_EQUALS('\n', simplecpp::characterLiteralToLL("'\\n'"));
		ASSERT_EQUALS('\r', simplecpp::characterLiteralToLL("'\\r'"));
		ASSERT_EQUALS('\t', simplecpp::characterLiteralToLL("'\\t'"));
		ASSERT_EQUALS('\v', simplecpp::characterLiteralToLL("'\\v'"));

		// GCC extension for ESC character
		ASSERT_EQUALS(0x1b, simplecpp::characterLiteralToLL("'\\e'"));
		ASSERT_EQUALS(0x1b, simplecpp::characterLiteralToLL("'\\E'"));

		// more obscure GCC extensions
		ASSERT_EQUALS('(', simplecpp::characterLiteralToLL("'\\('"));
		ASSERT_EQUALS('[', simplecpp::characterLiteralToLL("'\\['"));
		ASSERT_EQUALS('{', simplecpp::characterLiteralToLL("'\\{'"));
		ASSERT_EQUALS('%', simplecpp::characterLiteralToLL("'\\%'"));

		ASSERT_EQUALS('\0',   simplecpp::characterLiteralToLL("'\\0'"));
		ASSERT_EQUALS('\1',   simplecpp::characterLiteralToLL("'\\1'"));
		ASSERT_EQUALS('\10',  simplecpp::characterLiteralToLL("'\\10'"));
		ASSERT_EQUALS('\010', simplecpp::characterLiteralToLL("'\\010'"));
		ASSERT_EQUALS('\377', simplecpp::characterLiteralToLL("'\\377'"));

		ASSERT_EQUALS('\x0',  simplecpp::characterLiteralToLL("'\\x0'"));
		ASSERT_EQUALS('\x10', simplecpp::characterLiteralToLL("'\\x10'"));
		ASSERT_EQUALS('\xff', simplecpp::characterLiteralToLL("'\\xff'"));

		ASSERT_EQUALS('\u0012',     simplecpp::characterLiteralToLL("'\\u0012'"));
		ASSERT_EQUALS('\U00000012', simplecpp::characterLiteralToLL("'\\U00000012'"));

		ASSERT_EQUALS((static_cast<unsigned int>(static_cast<unsigned char>('b')) << 8) | static_cast<unsigned char>('c'),    simplecpp::characterLiteralToLL("'bc'"));
		ASSERT_EQUALS((static_cast<unsigned int>(static_cast<unsigned char>('\x23')) << 8) | static_cast<unsigned char>('\x45'), simplecpp::characterLiteralToLL("'\\x23\\x45'"));
		ASSERT_EQUALS((static_cast<unsigned int>(static_cast<unsigned char>('\11')) << 8) | static_cast<unsigned char>('\222'), simplecpp::characterLiteralToLL("'\\11\\222'"));
		ASSERT_EQUALS((static_cast<unsigned int>(static_cast<unsigned char>('\a')) << 8) | static_cast<unsigned char>('\b'),   simplecpp::characterLiteralToLL("'\\a\\b'"));
		if(sizeof(int) <= 4)
			ASSERT_EQUALS(-1, simplecpp::characterLiteralToLL("'\\xff\\xff\\xff\\xff'"));
		else
			ASSERT_EQUALS(0xffffffff, simplecpp::characterLiteralToLL("'\\xff\\xff\\xff\\xff'"));

		ASSERT_EQUALS('A', simplecpp::characterLiteralToLL("u8'A'"));
		ASSERT_EQUALS('A', simplecpp::characterLiteralToLL("u'A'"));
		ASSERT_EQUALS('A', simplecpp::characterLiteralToLL("L'A'"));
		ASSERT_EQUALS('A', simplecpp::characterLiteralToLL("U'A'"));

		ASSERT_EQUALS(0xff, simplecpp::characterLiteralToLL("u8'\\xff'"));
		ASSERT_EQUALS(0xff, simplecpp::characterLiteralToLL("u'\\xff'"));
		ASSERT_EQUALS(0xff, simplecpp::characterLiteralToLL("L'\\xff'"));
		ASSERT_EQUALS(0xff, simplecpp::characterLiteralToLL("U'\\xff'"));

		ASSERT_EQUALS(0xfedc,     simplecpp::characterLiteralToLL("u'\\xfedc'"));
		ASSERT_EQUALS(0xfedcba98, simplecpp::characterLiteralToLL("L'\\xfedcba98'"));
		ASSERT_EQUALS(0xfedcba98, simplecpp::characterLiteralToLL("U'\\xfedcba98'"));

		ASSERT_EQUALS(0x12,       simplecpp::characterLiteralToLL("u8'\\u0012'"));
		ASSERT_EQUALS(0x1234,     simplecpp::characterLiteralToLL("u'\\u1234'"));
		ASSERT_EQUALS(0x00012345, simplecpp::characterLiteralToLL("L'\\U00012345'"));
		ASSERT_EQUALS(0x00012345, simplecpp::characterLiteralToLL("U'\\U00012345'"));

	#ifdef __GNUC__
		// BEGIN Implementation-specific results
		ASSERT_EQUALS(static_cast<int>('AB'), simplecpp::characterLiteralToLL("'AB'"));
		ASSERT_EQUALS(static_cast<int>('ABC'), simplecpp::characterLiteralToLL("'ABC'"));
		ASSERT_EQUALS(static_cast<int>('ABCD'), simplecpp::characterLiteralToLL("'ABCD'"));
		ASSERT_EQUALS('\134t', simplecpp::characterLiteralToLL("'\\134t'")); // cppcheck ticket #7452
		// END Implementation-specific results
	#endif

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("'\\9'"), std::runtime_error, "invalid escape sequence");

		// Input is manually encoded to (escaped) UTF-8 byte sequences
		// to avoid dependence on source encoding used for this file
		ASSERT_EQUALS(0xb5, simplecpp::characterLiteralToLL("U'\302\265'"));
		ASSERT_EQUALS(0x157, simplecpp::characterLiteralToLL("U'\305\227'"));
		ASSERT_EQUALS(0xff0f, simplecpp::characterLiteralToLL("U'\357\274\217'"));
		ASSERT_EQUALS(0x3042, simplecpp::characterLiteralToLL("U'\343\201\202'"));
		ASSERT_EQUALS(0x13000, simplecpp::characterLiteralToLL("U'\360\223\200\200'"));

		ASSERT_EQUALS(0xb5, simplecpp::characterLiteralToLL("L'\302\265'"));
		ASSERT_EQUALS(0x157, simplecpp::characterLiteralToLL("L'\305\227'"));
		ASSERT_EQUALS(0xff0f, simplecpp::characterLiteralToLL("L'\357\274\217'"));
		ASSERT_EQUALS(0x3042, simplecpp::characterLiteralToLL("L'\343\201\202'"));
		ASSERT_EQUALS(0x13000, simplecpp::characterLiteralToLL("L'\360\223\200\200'"));

		ASSERT_EQUALS(0xb5, simplecpp::characterLiteralToLL("u'\302\265'"));
		ASSERT_EQUALS(0x157, simplecpp::characterLiteralToLL("u'\305\227'"));
		ASSERT_EQUALS(0xff0f, simplecpp::characterLiteralToLL("u'\357\274\217'"));
		ASSERT_EQUALS(0x3042, simplecpp::characterLiteralToLL("u'\343\201\202'"));
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("u'\360\223\200\200'"), std::runtime_error, "code point too large");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("u8'\302\265'"), std::runtime_error, "code point too large");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("u8'\305\227'"), std::runtime_error, "code point too large");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("u8'\357\274\217'"), std::runtime_error, "code point too large");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("u8'\343\201\202'"), std::runtime_error, "code point too large");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("u8'\360\223\200\200'"), std::runtime_error, "code point too large");

		ASSERT_EQUALS('\x89', simplecpp::characterLiteralToLL("'\x89'"));
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\x89'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf4\x90\x80\x80'"), std::runtime_error, "code point too large");

		// following examples based on https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
		ASSERT_EQUALS(0x80, simplecpp::characterLiteralToLL("U'\xc2\x80'"));
		ASSERT_EQUALS(0x800, simplecpp::characterLiteralToLL("U'\xe0\xa0\x80'"));
		ASSERT_EQUALS(0x10000, simplecpp::characterLiteralToLL("U'\xf0\x90\x80\x80'"));

		ASSERT_EQUALS(0x7f, simplecpp::characterLiteralToLL("U'\x7f'"));
		ASSERT_EQUALS(0x7ff, simplecpp::characterLiteralToLL("U'\xdf\xbf'"));
		ASSERT_EQUALS(0xffff, simplecpp::characterLiteralToLL("U'\xef\xbf\xbf'"));

		ASSERT_EQUALS(0xd7ff, simplecpp::characterLiteralToLL("U'\xed\x9f\xbf'"));
		ASSERT_EQUALS(0xe000, simplecpp::characterLiteralToLL("U'\xee\x80\x80'"));
		ASSERT_EQUALS(0xfffd, simplecpp::characterLiteralToLL("U'\xef\xbf\xbd'"));
		ASSERT_EQUALS(0x10ffff, simplecpp::characterLiteralToLL("U'\xf4\x8f\xbf\xbf'"));

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\x80'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\x80\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\x80\x8f\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\x80\x8f\x8f\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xbf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xbf\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xbf\x8f\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xbf\x8f\x8f\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xc0'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xc0 '"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xe0\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xe0\x8f '"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf0\x8f\x8f'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf0\x8f\x8f '"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf8'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xff'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xc0\xaf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xe0\x80\xaf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf0\x80\x80\xaf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xc1\xbf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xe0\x9f\xbf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf0\x8f\xbf\xbf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xc0\x80'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xe0\x80\x80'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xf0\x80\x80\x80'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");

		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xed\xa0\x80'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
		ASSERT_THROW_EQUALS(simplecpp::characterLiteralToLL("U'\xed\xbf\xbf'"), std::runtime_error, "assumed UTF-8 encoded source, but sequence is invalid");
	}
	{ // combineOperators_floatliteral()
		ASSERT_EQUALS("1.", preprocess("1."));
		ASSERT_EQUALS("1.f", preprocess("1.f"));
		ASSERT_EQUALS(".1", preprocess(".1"));
		ASSERT_EQUALS(".1f", preprocess(".1f"));
		ASSERT_EQUALS("3.1", preprocess("3.1"));
		ASSERT_EQUALS("1E7", preprocess("1E7"));
		ASSERT_EQUALS("1E-7", preprocess("1E-7"));
		ASSERT_EQUALS("1E+7", preprocess("1E+7"));
		ASSERT_EQUALS("1.e+7", preprocess("1.e+7"));
		ASSERT_EQUALS("0x1E + 7", preprocess("0x1E+7"));
		ASSERT_EQUALS("0x1ffp10", preprocess("0x1ffp10"));
		ASSERT_EQUALS("0x0p-1", preprocess("0x0p-1"));
		ASSERT_EQUALS("0x1.p0", preprocess("0x1.p0"));
		ASSERT_EQUALS("0xf.p-1", preprocess("0xf.p-1"));
		ASSERT_EQUALS("0x1.2p3", preprocess("0x1.2p3"));
		ASSERT_EQUALS("0x1.ap3", preprocess("0x1.ap3"));
		ASSERT_EQUALS("0x1.2ap3", preprocess("0x1.2ap3"));
		ASSERT_EQUALS("0x1p+3", preprocess("0x1p+3"));
		ASSERT_EQUALS("0x1p+3f", preprocess("0x1p+3f"));
		ASSERT_EQUALS("0x1p+3L", preprocess("0x1p+3L"));
		ASSERT_EQUALS("1p + 3", preprocess("1p+3"));
		ASSERT_EQUALS("1.0_a . b", preprocess("1.0_a.b"));
		ASSERT_EQUALS("1_a . b", preprocess("1_a.b"));
	}
	{ // combineOperators_increment()
		ASSERT_EQUALS("; ++ x ;", preprocess(";++x;"));
		ASSERT_EQUALS("; x ++ ;", preprocess(";x++;"));
		ASSERT_EQUALS("1 + + 2", preprocess("1++2"));
	}
	{ // combineOperators_coloncolon()
		ASSERT_EQUALS("x ? y : :: z", preprocess("x ? y : ::z"));
	}
	{ // combineOperators_andequal()
		ASSERT_EQUALS("x &= 2 ;", preprocess("x &= 2;"));
		ASSERT_EQUALS("void f ( x & = 2 ) ;", preprocess("void f(x &= 2);"));
		ASSERT_EQUALS("f ( x &= 2 ) ;", preprocess("f(x &= 2);"));
	}
	{ // combineOperators_ellipsis()
		ASSERT_EQUALS("void f ( int , ... ) ;", preprocess("void f(int, ...);"));
		ASSERT_EQUALS("void f ( ) { switch ( x ) { case 1 ... 4 : } }", preprocess("void f() { switch(x) { case 1 ... 4: } }"));
	}
	{ // comment()
		ASSERT_EQUALS("// abc", readfile("// abc"));
		ASSERT_EQUALS("", preprocess("// abc"));
		ASSERT_EQUALS("/*\n\n*/abc", readfile("/*\n\n*/abc"));
		ASSERT_EQUALS("\n\nabc", preprocess("/*\n\n*/abc"));
		ASSERT_EQUALS("* p = a / * b / * c ;", readfile("*p=a/ *b/ *c;"));
		ASSERT_EQUALS("* p = a / * b / * c ;", preprocess("*p=a/ *b/ *c;"));
	}
	{ // comment_multiline()
		const char code[] = "#define ABC {// \\\n"
			"}\n"
			"void f() ABC\n";
		ASSERT_EQUALS("\n\nvoid f ( ) { }", preprocess(code));
	}
	{ //constFold()
		ASSERT_EQUALS("7", testConstFold("1+2*3"));
		ASSERT_EQUALS("15", testConstFold("1+2*(3+4)"));
		ASSERT_EQUALS("123", testConstFold("+123"));
		ASSERT_EQUALS("1", testConstFold("-123<1"));
		ASSERT_EQUALS("6", testConstFold("14 & 7"));
		ASSERT_EQUALS("29", testConstFold("13 ^ 16"));
		ASSERT_EQUALS("25", testConstFold("24 | 1"));
		ASSERT_EQUALS("2", testConstFold("1?2:3"));
		ASSERT_EQUALS("24", testConstFold("010+020"));
		ASSERT_EQUALS("1", testConstFold("010==8"));
		// (проблема с exception) ASSERT_EQUALS("exception", testConstFold("!1 ? 2 :"));
		// (проблема с exception) ASSERT_EQUALS("exception", testConstFold("?2:3"));
	}
#ifdef __CYGWIN__
	convertCygwinPath();
#endif
	{ //define1()
		const char code[] = "#define A 1+2\n"
			"a=A+3;";
		ASSERT_EQUALS("# define A 1 + 2\n"
			"a = A + 3 ;",
			readfile(code));
		ASSERT_EQUALS("\na = 1 + 2 + 3 ;",
			preprocess(code));
	}
	{ //define2()
		const char code[] = "#define ADD(A,B) A+B\n" "ADD(1+2,3);";
		ASSERT_EQUALS("# define ADD ( A , B ) A + B\n" "ADD ( 1 + 2 , 3 ) ;", readfile(code));
		ASSERT_EQUALS("\n1 + 2 + 3 ;", preprocess(code));
	}
	{ //define3()
		const char code[] = "#define A   123\n"
			"#define B   A\n"
			"A B";
		ASSERT_EQUALS("# define A 123\n"
			"# define B A\n"
			"A B",
			readfile(code));
		ASSERT_EQUALS("\n\n123 123", preprocess(code));
	}
	{ //define4()
		const char code[] = "#define A      123\n"
			"#define B(C)   A\n"
			"A B(1)";
		ASSERT_EQUALS("# define A 123\n"
			"# define B ( C ) A\n"
			"A B ( 1 )",
			readfile(code));
		ASSERT_EQUALS("\n\n123 123", preprocess(code));
	}
	{ //define5()
		const char code[] = "#define add(x,y) x+y\n"
			"add(add(1,2),3)";
		ASSERT_EQUALS("\n1 + 2 + 3", preprocess(code));
	}
	{ //define6()
		const char code[] = "#define A() 1\n" "A()";
		ASSERT_EQUALS("\n1", preprocess(code));
	}
	{ //define7()
		const char code[] = "#define A(X) X+1\n" "A(1 /*23*/)";
		ASSERT_EQUALS("\n1 + 1", preprocess(code));
	}
	{ //define8()   // 6.10.3.10
		const char code[] = "#define A(X) \n" "int A[10];";
		ASSERT_EQUALS("\nint A [ 10 ] ;", preprocess(code));
	}
	{ //define9()
		const char code[] = "#define AB ab.AB\n" "AB.CD\n";
		ASSERT_EQUALS("\nab . AB . CD", preprocess(code));
	}
	{ //define10()   // don't combine prefix with space in macro
		const char code[] = "#define A u8 \"a b\"\n" "A;";
		ASSERT_EQUALS("\nu8 \"a b\" ;", preprocess(code));
	}
	{ //define11() // location of expanded argument
		const char code[] = "#line 4 \"version.h\"\n"
			"#define A(x) B(x)\n"
			"#define B(x) x\n"
			"#define VER A(1)\n"
			"\n"
			"#line 10 \"cppcheck.cpp\"\n"
			"VER;";
		ASSERT_EQUALS("\n#line 10 \"cppcheck.cpp\"\n1 ;", preprocess(code));
	}
	{ //define12()
		const char code[] = "struct foo x = {\n"
			"  #define V 0\n"
			"  .x = V,\n"
			"};\n";
		ASSERT_EQUALS("struct foo x = {\n"
			"# define V 0\n"
			". x = V ,\n"
			"} ;", readfile(code));
		ASSERT_EQUALS("struct foo x = {\n"
			"\n"
			". x = 0 ,\n"
			"} ;", preprocess(code));
	}
	{ //define13()
		const char code[] = "#define M 180.\n"
			"extern void g();\n"
			"void f(double d) {\n"
			"    if (d > M) {}\n"
			"}\n";
		ASSERT_EQUALS("\nextern void g ( ) ;\n"
			"void f ( double d ) {\n"
			"if ( d > 180. ) { }\n"
			"}", preprocess(code));
	}
	{//define_invalid_1()
		const char code[] = "#define  A(\nB\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,Failed to parse #define\n", toString(outputList));
	}
	{//define_invalid_2()
		const char code[] = "#define\nhas#";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,Failed to parse #define\n", toString(outputList));
	}
	{//define_define_1()
		const char code[] = "#define A(x) (x+1)\n"
			"#define B    A(\n"
			"B(i))";
		ASSERT_EQUALS("\n\n( ( i ) + 1 )", preprocess(code));
	}
	{//define_define_2()
		const char code[] = "#define A(m)    n=m\n"
			"#define B(x)    A(x)\n"
			"B(0)";
		ASSERT_EQUALS("\n\nn = 0", preprocess(code));
	}
	{//define_define_3()
		const char code[] = "#define ABC 123\n"
			"#define A(B) A##B\n"
			"A(BC)";
		ASSERT_EQUALS("\n\n123", preprocess(code));
	}
	{//define_define_4()
		const char code[] = "#define FOO1()\n"
			"#define TEST(FOO) FOO FOO()\n"
			"TEST(FOO1)";
		ASSERT_EQUALS("\n\nFOO1", preprocess(code));
	}
	{//define_define_5()
		const char code[] = "#define X() Y\n"
			"#define Y() X\n"
			"A: X()()()\n";
		// mcpp outputs "A: X()" and gcc/clang/vc outputs "A: Y"
		ASSERT_EQUALS("\n\nA : Y", preprocess(code)); // <- match the output from gcc/clang/vc
	}
	{//define_define_6()
		const char code1[] = "#define f(a) a*g\n"
			"#define g f\n"
			"a: f(2)(9)\n";
		ASSERT_EQUALS("\n\na : 2 * f ( 9 )", preprocess(code1));

		const char code2[] = "#define f(a) a*g\n"
			"#define g(a) f(a)\n"
			"a: f(2)(9)\n";
		ASSERT_EQUALS("\n\na : 2 * 9 * g", preprocess(code2));
	}
	{//define_define_7()
		const char code[] = "#define f(x) g(x\n"
			"#define g(x) x()\n"
			"f(f))\n";
		ASSERT_EQUALS("\n\nf ( )", preprocess(code));
	}
	{//define_define_8()   // line break in nested macro call
		const char code[] = "#define A(X,Y)  ((X)*(Y))\n"
			"#define B(X,Y)  ((X)+(Y))\n"
			"B(0,A(255,x+\n"
			"y))\n";
		ASSERT_EQUALS("\n\n( ( 0 ) + ( ( ( 255 ) * ( x + y ) ) ) )", preprocess(code));
	}
	{//define_define_9()   // line break in nested macro call
		const char code[] = "#define A(X) X\n"
			"#define B(X) X\n"
			"A(\nB(dostuff(1,\n2)))\n";
		ASSERT_EQUALS("\n\ndostuff ( 1 , 2 )", preprocess(code));
	}
	{//define_define_10()
		const char code[] = "#define glue(a, b) a ## b\n"
			"#define xglue(a, b) glue(a, b)\n"
			"#define AB 1\n"
			"#define B B 2\n"
			"xglue(A, B)\n";
		ASSERT_EQUALS("\n\n\n\n1 2", preprocess(code));
	}
	{//define_define_11()
		const char code[] = "#define XY(x, y)   x ## y\n"
			"#define XY2(x, y)  XY(x, y)\n"
			"#define PORT       XY2(P, 2)\n"
			"#define ABC        XY2(PORT, DIR)\n"
			"ABC;\n";
		ASSERT_EQUALS("\n\n\n\nP2DIR ;", preprocess(code));
	}
	{//define_define_11a()
		const char code[] = "#define A_B_C              0x1\n"
			"#define A_ADDRESS          0x00001000U\n"
			"#define A                  ((uint32_t ) A_ADDRESS)\n"
			"#define CONCAT(x, y, z)    x ## _ ## y ## _ ## z\n"
			"#define TEST_MACRO         CONCAT(A, B, C)\n"
			"TEST_MACRO\n";
		ASSERT_EQUALS("\n\n\n\n\n0x1", preprocess(code));

		const char code2[] = "#define ADDER_S(a, b) a + b\n" // #374
			"#define ADDER(x) ADDER_S(x)\n"
			"#define ARGUMENTS 1, 2\n"
			"#define RUN ADDER(ARGUMENTS)\n"
			"void f() { RUN; }\n";
		ASSERT_EQUALS("\n\n\n\nvoid f ( ) { 1 + 2 ; }", preprocess(code2));
	}
	{//define_define_12() // expand result of ##
		const char code[] = "#define XY(Z)  Z\n"
			"#define X(ID)  X##ID(0)\n"
			"X(Y)\n";
		ASSERT_EQUALS("\n\n0", preprocess(code));
	}
	{//define_define_13() // issue #49 - empty macro
		const char code[] = "#define f()\n"
			"#define t(a) a\n"
			"(t(f))\n";
		ASSERT_EQUALS("\n\n( f )", preprocess(code));
	}
	{//define_define_14() // issue #58 - endless recursion
		const char code[] = "#define z f(w\n"
			"#define f()\n"
			"#define w f(z\n"
			"w\n";
		ASSERT_EQUALS("\n\n\nf ( f ( w", preprocess(code)); // Don't crash
	}
	{//define_define_15() // issue #72 without __VA_ARGS__
		const char code[] = "#define a          f\n"
			"#define foo(x,y)   a(x,y)\n"
			"#define f(x, y)    x y\n"
			"foo(1,2)";
		ASSERT_EQUALS("\n\n\n1 2", preprocess(code));
	}
	{//define_define_16() // issue #72 with __VA_ARGS__
		const char code[] = "#define ab(a, b)  a##b\n"
			"#define foo(...) ab(f, 2) (__VA_ARGS__)\n"
			"#define f2(x, y) x y\n"
			"foo(1,2)";
		ASSERT_EQUALS("\n\n\n1 2", preprocess(code));
	}
	{//define_define_17()
		const char code[] = "#define Bar(x) x\n"
			"#define Foo Bar(1)\n"
			"Bar( Foo ) ;";
		ASSERT_EQUALS("\n\n1 ;", preprocess(code));
	}
	{//define_define_18()
		const char code[] = "#define FOO(v)      BAR(v, 0)\n"
			"#define BAR(v, x)   (v)\n"
			"#define var         (p->var)\n"
			"FOO(var);";
		ASSERT_EQUALS("\n\n\n( ( p -> var ) ) ;", preprocess(code));
	}
	{//define_define_19() // #292
		const char code[] = "#define X 1,2,3\n"
			"#define Foo(A, B) A\n"
			"#define Bar Foo(X, 0)\n"
			"Bar\n";
		ASSERT_EQUALS("\n\n\n1 , 2 , 3", preprocess(code));
	}
	{//define_define_20() // #384 arg contains comma
		const char code[] = "#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)\n"
			"#define _XXXX1 _YYYY,\n"
			"#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)\n"
			"#define Z_IS_ENABLED3(ignore_this, val, ...) val\n"
			"#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)\n"
			"#define FEATURE 1\n"
			"a = IS_ENABLED(FEATURE)\n";
		ASSERT_EQUALS("\n\n\n\n\n\na = 1", preprocess(code));
	}
	{//define_define_21() // #397 DEBRACKET macro
		const char code1[] = "#define A(val) B val\n"
			"#define B(val) val\n"
			"A((2))\n";
		ASSERT_EQUALS("\n\n2", preprocess(code1));

		const char code2[] = "#define x (2)\n"
			"#define A B x\n"
			"#define B(val) val\n"
			"A\n";
		ASSERT_EQUALS("\n\n\nB ( 2 )", preprocess(code2));

		const char code3[] = "#define __GET_ARG2_DEBRACKET(ignore_this, val, ...) __DEBRACKET val\n"
			"#define __DEBRACKET(...) __VA_ARGS__\n"
			"#5 \"a.c\"\n"
			"__GET_ARG2_DEBRACKET(432 (33), (B))\n";
		ASSERT_EQUALS("\n#line 5 \"a.c\"\nB", preprocess(code3));
	}
	{//define_define_22() // #400 inner macro not expanded after hash hash
		const char code[] = "#define FOO(a) CAT(DO, STUFF)(1,2)\n"
			"#define DOSTUFF(a, b)  CAT(3, 4)\n"
			"#define CAT(a, b) a##b\n"
			"FOO(1)\n";
		ASSERT_EQUALS("\n\n\n34", preprocess(code));
	}
	{//define_define_23() // #403 crash (infinite recursion)
		const char code[] = "#define C_(x, y)       x ## y\n"
			"#define C(x, y)        C_(x, y)\n"
			"#define X(func)        C(Y, C(func, Z))\n"
			"#define die X(die)\n"
			"die(void);\n";
		ASSERT_EQUALS("\n\n\n\nYdieZ ( void ) ;", preprocess(code));
	}
	{ //define_va_args_1()
		const char code[] = "#define A(fmt...) dostuff(fmt)\n"
			"A(1,2);";
		ASSERT_EQUALS("\ndostuff ( 1 , 2 ) ;", preprocess(code));
	}
	{//define_va_args_2()
		const char code[] = "#define A(X,...) X(#__VA_ARGS__)\n"
			"A(f,123);";
		ASSERT_EQUALS("\nf ( \"123\" ) ;", preprocess(code));
	}
	{ //define_va_args_3()   // min number of arguments
		const char code[] = "#define A(x, y, z...) 1\n"
			"A(1, 2)\n";
		ASSERT_EQUALS("\n1", preprocess(code));
	}
	{ //define_va_args_4() // cppcheck trac #9754
		const char code[] = "#define A(x, y, ...) printf(x, y, __VA_ARGS__)\n"
			"A(1, 2)\n";
		ASSERT_EQUALS("\nprintf ( 1 , 2 )", preprocess(code));
	}
	{ //define_va_opt_1()
		const char code[] = "#define p1(fmt, args...) printf(fmt __VA_OPT__(,) args)\n"
			"p1(\"hello\");\n"
			"p1(\"%s\", \"hello\");\n";

		ASSERT_EQUALS("\nprintf ( \"hello\" ) ;\n"
			"printf ( \"%s\" , \"hello\" ) ;",
			preprocess(code));
	}
	{ //define_va_opt_2()
		const char code[] = "#define err(...)\\\n"
			"__VA_OPT__(\\\n"
			"printf(__VA_ARGS__);\\\n"
			")\n"
			"#define err2(something, ...) __VA_OPT__(err(__VA_ARGS__))\n"
			"err2(test)\n"
			"err2(test, \"%d\", 2)\n";

		ASSERT_EQUALS("\n\n\n\n\n\nprintf ( \"%d\" , 2 ) ;", preprocess(code));
	}
	{ //define_va_opt_3()
		// non-escaped newline without closing parenthesis
		const char code1[] = "#define err(...) __VA_OPT__(printf( __VA_ARGS__);\n"
			")\n"
			"err()";

		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code1, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'err', Missing parenthesis for __VA_OPT__(content)\n",
			toString(outputList));

		outputList.clear();

		// non-escaped newline without open parenthesis
		const char code2[] = "#define err(...) __VA_OPT__\n"
			"(something)\n"
			"err()";

		ASSERT_EQUALS("", preprocess(code2, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'err', Missing parenthesis for __VA_OPT__(content)\n",
			toString(outputList));
	}
	{ //define_va_opt_4()
		// missing parenthesis
		const char code1[] = "#define err(...) __VA_OPT__ something\n"
			"err()";

		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code1, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'err', Missing parenthesis for __VA_OPT__(content)\n",
			toString(outputList));

		outputList.clear();

		// missing open parenthesis
		const char code2[] = "#define err(...) __VA_OPT__ something)\n"
			"err()";

		ASSERT_EQUALS("", preprocess(code2, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'err', Missing parenthesis for __VA_OPT__(content)\n",
			toString(outputList));
	}
	{ //define_va_opt_5()
		// parenthesis not directly proceeding __VA_OPT__
		const char code[] = "#define err(...) __VA_OPT__ something (something)\n"
			"err()";

		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'err', Missing parenthesis for __VA_OPT__(content)\n",
			toString(outputList));
	}
	{//pragma_backslash() // multiline pragma directive
		const char code[] = "#pragma comment (longstring, \\\n"
			"\"HEADER\\\n"
			"This is a very long string that is\\\n"
			"a multi-line string.\\\n"
			"How much more do I have to say?\\\n"
			"Well, be prepared, because the\\\n"
			"story is just beginning. This is a test\\\n"
			"string for demonstration purposes. \")\n";

		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
	}
	{//define_ifdef() // UB: #ifdef as macro parameter
		const char code[] = "#define A(X) X\n"
			"A(1\n"
			"#ifdef CFG\n"
			"#endif\n"
			")\n";

		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,3,syntax_error,failed to expand 'A', it is invalid to use a preprocessor directive as macro parameter\n", toString(outputList));
	}
	{//dollar()
		ASSERT_EQUALS("$ab", readfile("$ab"));
		ASSERT_EQUALS("a$b", readfile("a$b"));
	}
	{ //error1()
		const char code[] = "#error    hello world!\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,#error,#error hello world!\n", toString(outputList));
	}
	{ //error2()
		const char code[] = "#error   it's an error\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,#error,#error it's an error\n", toString(outputList));
	}
	{ //error3()
		const char code[] = "#error \"bla bla\\\n"
			" bla bla.\"\n";
		std::vector<std::string> files;
		simplecpp::OutputList outputList;
		const simplecpp::TokenList rawtokens = makeTokenList(code, files, "test.c", &outputList);
		ASSERT_EQUALS("", toString(outputList));
	}
	{ //error4()
		// "#error x\n1"
		const char code[] = "\xFE\xFF\x00\x23\x00\x65\x00\x72\x00\x72\x00\x6f\x00\x72\x00\x20\x00\x78\x00\x0a\x00\x31";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::OutputList outputList;
		simplecpp::TokenList tokens2(files);
		const simplecpp::TokenList rawtoken = makeTokenList(code, sizeof(code), files, "test.c");
		simplecpp::preprocess(tokens2, rawtoken, files, filedata, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS("file0,1,#error,#error x\n", toString(outputList));
	}
	{ //error5()
		// "#error x\n1"
		const char code[] = "\xFF\xFE\x23\x00\x65\x00\x72\x00\x72\x00\x6f\x00\x72\x00\x20\x00\x78\x00\x0a\x00\x78\x00\x31\x00";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::OutputList outputList;
		simplecpp::TokenList tokens2(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, sizeof(code), files, "test.c");
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS("file0,1,#error,#error x\n", toString(outputList));
	}
	{//garbage()
		simplecpp::OutputList outputList;

		outputList.clear();
		ASSERT_EQUALS("", preprocess("#ifdef\n", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,Syntax error in #ifdef\n", toString(outputList));

		outputList.clear();
		ASSERT_EQUALS("", preprocess("#define TEST2() A ##\nTEST2()\n", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'TEST2', Invalid ## usage when expanding 'TEST2': Unexpected newline\n", toString(outputList));

		outputList.clear();
		ASSERT_EQUALS("", preprocess("#define CON(a,b)  a##b##\nCON(1,2)\n", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'CON', Invalid ## usage when expanding 'CON': Unexpected newline\n", toString(outputList));
	}
	{//garbage_endif()
		simplecpp::OutputList outputList;

		outputList.clear();
		ASSERT_EQUALS("", preprocess("#elif A<0\n", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,#elif without #if\n", toString(outputList));

		outputList.clear();
		ASSERT_EQUALS("", preprocess("#else\n", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,#else without #if\n", toString(outputList));

		outputList.clear();
		ASSERT_EQUALS("", preprocess("#endif\n", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,#endif without #if\n", toString(outputList));
	}
	{//hash()
		ASSERT_EQUALS("x = \"1\"", preprocess("x=#__LINE__"));

		const char code[] = "#define a(x) #x\n"
			"a(1)\n"
			"a(2+3)";
		ASSERT_EQUALS("\n"
			"\"1\"\n"
			"\"2+3\"", preprocess(code));

		ASSERT_EQUALS("\n\"\\\"abc\\\\0\\\"\"", preprocess("#define str(x) #x\nstr(\"abc\\0\")\n"));

		ASSERT_EQUALS("\n\n( \"123\" )",
			preprocess("#define A(x)  (x)\n"
			"#define B(x)  A(#x)\n"
			"B(123)"));

		ASSERT_EQUALS("\n\nprintf ( \"bar(3)\" \"\\n\" ) ;",
			preprocess("#define bar(x) x % 2\n"
			"#define foo(x) printf(#x \"\\n\")\n"
			"foo(bar(3));"));

		ASSERT_EQUALS("\n\n\n\"Y Y\"",
			preprocess("#define X(x,y)    x y\n"
			"#define STR_(x)   #x\n"
			"#define STR(x)    STR_(x)\n"
			"STR(X(Y,Y))"));
	}
	{//hashhash1()   // #4703
		const char code[] = "#define MACRO( A, B, C ) class A##B##C##Creator {};\n"
			"MACRO( B\t, U , G )";
		ASSERT_EQUALS("\nclass BUGCreator { } ;", preprocess(code));
	}
	{//hashhash2()
		const char code[] = "#define A(x) a##x\n"
			"#define B 0\n"
			"A(B)";
		ASSERT_EQUALS("\n\naB", preprocess(code));
	}
	{//hashhash3()
		const char code[] = "#define A(B) A##B\n"
			"#define a(B) A(B)\n"
			"a(A(B))";
		ASSERT_EQUALS("\n\nAAB", preprocess(code));
	}
	{//hashhash4()    // nonstandard gcc/clang extension for empty varargs
		const char * code;
		code = "#define A(x,y...)  a(x,##y)\n"
			"A(1)\n";
		ASSERT_EQUALS("\na ( 1 )", preprocess(code));
		code = "#define A(x, ...)   a(x, ## __VA_ARGS__)\n"
			"#define B(x, ...)   A(x, ## __VA_ARGS__)\n"
			"B(1);";
		ASSERT_EQUALS("\n\na ( 1 ) ;", preprocess(code));
	}
	{//hashhash4a() // #66, #130
		const char code[] = "#define GETMYID(a) ((a))+1\n"
			"#define FIGHT_FOO(c, ...) foo(c, ##__VA_ARGS__)\n"
			"#define FIGHT_BAR(c, args...) bar(c, ##args)\n"
			"FIGHT_FOO(1, GETMYID(a));\n"
			"FIGHT_BAR(1, GETMYID(b));";
		ASSERT_EQUALS("\n\n\nfoo ( 1 , ( ( a ) ) + 1 ) ;\nbar ( 1 , ( ( b ) ) + 1 ) ;", preprocess(code));
	}
	{//hashhash5()
		ASSERT_EQUALS("x1", preprocess("x##__LINE__"));
	}
	{//hashhash6()
		const char * code = "#define A(X, ...) LOG(X, ##__VA_ARGS__)\n"
			"A(1,(int)2)";
		ASSERT_EQUALS("\nLOG ( 1 , ( int ) 2 )", preprocess(code));
		code = "#define A(X, ...) LOG(X, ##__VA_ARGS__)\n"
			"#define B(X, ...) A(X, ##__VA_ARGS__)\n"
			"#define C(X, ...) B(X, ##__VA_ARGS__)\n"
			"C(1,(int)2)";
		ASSERT_EQUALS("\n\n\nLOG ( 1 , ( int ) 2 )", preprocess(code));
	}
	{//hashhash7()   // # ## #  (C standard; 6.10.3.3.p4)
		const char * code = "#define hash_hash # ## #\n"
			"x hash_hash y";
		ASSERT_EQUALS("\nx ## y", preprocess(code));
	}
	{//hashhash8()
		const char code[] = "#define a(xy)    x##y = xy\n"
			"a(123);";
		ASSERT_EQUALS("\nxy = 123 ;", preprocess(code));
	}
	{//hashhash9()
		const char * code = "#define ADD_OPERATOR(OP) void operator OP ## = (void) { x = x OP 1; }\n"
			"ADD_OPERATOR(+);\n"
			"ADD_OPERATOR(-);\n"
			"ADD_OPERATOR(*);\n"
			"ADD_OPERATOR(/);\n"
			"ADD_OPERATOR(%);\n"
			"ADD_OPERATOR(&);\n"
			"ADD_OPERATOR(|);\n"
			"ADD_OPERATOR(^);\n"
			"ADD_OPERATOR(<<);\n"
			"ADD_OPERATOR(>>);\n";
		const char expected[] = "\n"
			"void operator += ( void ) { x = x + 1 ; } ;\n"
			"void operator -= ( void ) { x = x - 1 ; } ;\n"
			"void operator *= ( void ) { x = x * 1 ; } ;\n"
			"void operator /= ( void ) { x = x / 1 ; } ;\n"
			"void operator %= ( void ) { x = x % 1 ; } ;\n"
			"void operator &= ( void ) { x = x & 1 ; } ;\n"
			"void operator |= ( void ) { x = x | 1 ; } ;\n"
			"void operator ^= ( void ) { x = x ^ 1 ; } ;\n"
			"void operator <<= ( void ) { x = x << 1 ; } ;\n"
			"void operator >>= ( void ) { x = x >> 1 ; } ;";
		ASSERT_EQUALS(expected, preprocess(code));

		simplecpp::OutputList outputList;

		code = "#define A +##x\n"
			"A";
		outputList.clear();
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'A', Invalid ## usage when expanding 'A': Combining '+' and 'x' yields an invalid token.\n", toString(outputList));

		code = "#define A 2##=\n"
			"A";
		outputList.clear();
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'A', Invalid ## usage when expanding 'A': Combining '2' and '=' yields an invalid token.\n", toString(outputList));

		code = "#define A <<##x\n"
			"A";
		outputList.clear();
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'A', Invalid ## usage when expanding 'A': Combining '<<' and 'x' yields an invalid token.\n", toString(outputList));
	}
	{//hashhash10() // #108 : #define x # #
		const char code[] = "#define x # #\n"
			"x";
		ASSERT_EQUALS("# #", preprocess(code));
	}
	{//hashhash11() // #60: #define x # # #
		const char code[] = "#define x # # #\n"
			"x";
		ASSERT_EQUALS("# # #", preprocess(code));
	}
	{//hashhash12()
		{
			const char code[] = "#define MAX_FOO 1\n"
				"#define MAX_FOO_AA 2\n"
				"\n"
				"#define M(UpperCaseName, b)                "
				"  do {                                     "
				"    int MaxValue = MAX_##UpperCaseName;    "
				"    if (b) {                               "
				"      MaxValue = MAX_##UpperCaseName##_AA; "
				"    }                                      "
				"  } while (0)"
				"\n"
				"static void f(bool b) { M(FOO, b); }\n";

			ASSERT_EQUALS("\n\n\n\nstatic void f ( bool b ) { do { int MaxValue = 1 ; if ( b ) { MaxValue = 2 ; } } while ( 0 ) ; }", preprocess(code));
		}
		{
			const char code[] = "#define MAX_FOO (1 * 1)\n"
				"#define MAX_FOO_AA (2 * 1)\n"
				"\n"
				"#define M(UpperCaseName, b)                "
				"  do {                                     "
				"    int MaxValue = MAX_##UpperCaseName;    "
				"    if (b) {                               "
				"      MaxValue = MAX_##UpperCaseName##_AA; "
				"    }                                      "
				"  } while (0)"
				"\n"
				"static void f(bool b) { M(FOO, b); }\n";

			ASSERT_EQUALS("\n\n\n\nstatic void f ( bool b ) { do { int MaxValue = ( 1 * 1 ) ; if ( b ) { MaxValue = ( 2 * 1 ) ; } } while ( 0 ) ; }", preprocess(code));
		}
	}
	{//hashhash13()
		const char code[] = "#define X(x) x##U\n"
			"X((1<<1)-1)";
		ASSERT_EQUALS("\n( 1 << 1 ) - 1U", preprocess(code));

		const char code2[] = "#define CONCAT(x, y) x##y\n"
			"CONCAT(&a, b)";
		ASSERT_EQUALS("\n& ab", preprocess(code2));
	}
	{//hashhash_string_literal()
		const char code[] =
			"#define UL(x) x##_ul\n"
			"\"ABC\"_ul;\n"
			"UL(\"ABC\");";

		ASSERT_EQUALS("\n\"ABC\" _ul ;\n\"ABC\" _ul ;", preprocess(code));
	}
	{//hashhash_string_wrapped()
		const char code[] =
			"#define CONCAT(a,b) a##b\n"
			"#define STR(x) CONCAT(x,s)\n"
			"STR(\"ABC\");";

		ASSERT_EQUALS("\n\n\"ABC\" s ;", preprocess(code));
	}
	{//hashhash_char_literal()
		const char code[] =
			"#define CH(x) x##_ch\n"
			"CH('a');";

		ASSERT_EQUALS("\n'a' _ch ;", preprocess(code));
	}
	{//hashhash_multichar_literal()
		const char code[] =
			"#define CH(x) x##_ch\n"
			"CH('abcd');";

		ASSERT_EQUALS("\n'abcd' _ch ;", preprocess(code));
	}
	{//hashhash_char_escaped()
		const char code[] =
			"#define CH(x) x##_ch\n"
			"CH('\\'');";

		ASSERT_EQUALS("\n'\\'' _ch ;", preprocess(code));
	}
	{//hashhash_string_nothing()
		const char code[] =
			"#define CONCAT(a,b) a##b\n"
			"CONCAT(\"ABC\",);";

		ASSERT_EQUALS("\n\"ABC\" ;", preprocess(code));
	}
	{//hashhash_string_char()
		const char code[] =
			"#define CONCAT(a,b) a##b\n"
			"CONCAT(\"ABC\", 'c');";

		// This works, but maybe shouldn't since the result isn't useful.
		ASSERT_EQUALS("\n\"ABC\" 'c' ;", preprocess(code));
	}
	{//hashhash_string_name()
		const char code[] =
			"#define CONCAT(a,b) a##b\n"
			"#define LIT _literal\n"
			"CONCAT(\"string\", LIT);";

		// TODO is this correct? clang fails because that's not really a valid thing but gcc seems to accept it
		// see https://gist.github.com/patrickdowling/877a25294f069bf059f3b07f9b5b7039

		ASSERT_EQUALS("\n\n\"string\" LIT ;", preprocess(code));
	}
	{//hashhashhash_int_literal()
		const char code[] =
			"#define CONCAT(a,b,c) a##b##c\n"
			"#define PASTER(a,b,c) CONCAT(a,b,c)\n"
			"PASTER(\"123\",_i,ul);";

		ASSERT_EQUALS("\n\n\"123\" _iul ;", preprocess(code));
	}
	{//hashhash_int_literal()
		const char code[] =
			"#define PASTE(a,b) a##b\n"
			"PASTE(123,_i);\n"
			"1234_i;\n";

		ASSERT_EQUALS("\n123_i ;\n1234_i ;", preprocess(code));
	}
	{//hashhash_invalid_1()
		const char code[] = "#define  f(a)  (##x)\nf(1)";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'f', Invalid ## usage when expanding 'f': Unexpected token '('\n", toString(outputList));
	}
	{//hashhash_invalid_2()
		const char code[] = "#define  f(a)  (x##)\nf(1)";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'f', Invalid ## usage when expanding 'f': Unexpected token ')'\n", toString(outputList));
	}
	{//hashhash_invalid_string_number()
		const char code[] = "#define BAD(x) x##12345\nBAD(\"ABC\")";
		simplecpp::OutputList outputList;
		preprocess(code, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'BAD', Invalid ## usage when expanding 'BAD': Combining '\"ABC\"' and '12345' yields an invalid token.\n",
			toString(outputList));
	}
	{//hashhash_invalid_missing_args()
		const char code[] = "#define BAD(x) ##x\nBAD()";
		simplecpp::OutputList outputList;
		preprocess(code, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,failed to expand 'BAD', Invalid ## usage when expanding 'BAD': Missing first argument\n", toString(outputList));
	}
	{//hashhash_null_stmt()
		const char code[] =
			"# define B(x) C ## x\n"
			"#\n"
			"# define C0 1\n"
			"\n"
			"B(0);\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("\n\n\n\n1 ;", preprocess(code, &outputList));
	}
	{//hashhash_empty_va_args()
		// #395 hash hash with an empty __VA_ARGS__ in a macro
		const char code[] =
			"#define CAT(a, ...)        a##__VA_ARGS__\n"
			"#define X(a, ...)          CAT(a)\n"
			"#define LEVEL_2            (2)\n"
			"X(LEVEL_2)\n";
		ASSERT_EQUALS("\n\n\n( 2 )", preprocess(code));
	}
	{//hashhash_universal_character()
		// C standard, 5.1.1.2, paragraph 4:
		//    If a character sequence that matches the syntax of a universal
		//    character name is produced by token concatenation (6.10.3.3),
		//    the behavior is undefined."
		const char code[] =
			"#define A(x,y) x##y\nint A(\\u01,04);";
		simplecpp::OutputList outputList;
		preprocess(code, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS(
			"file0,1,syntax_error,failed to expand 'A', Invalid ## usage when expanding 'A': Combining '\\u01' and '04' yields universal character '\\u0104'. This is undefined behavior according to C standard chapter 5.1.1.2, paragraph 4.\n",
			toString(outputList));
	}
	// c++17 __has_include
	{ //has_include_1()
		const char code[] = "#ifdef __has_include\n"
			"  #if __has_include(\"simplecpp.h\")\n"
			"    A\n"
			"  #else\n"
			"    B\n"
			"  #endif\n"
			"#endif";
		simplecpp::DUI dui;
		dui.std = "c++17";
		//ASSERT_EQUALS("\n\nA", preprocess(code, dui)); // ! Не срабатывает - какие-то проблемы с __has_include
		dui.std = "c++14";
		ASSERT_EQUALS("", preprocess(code, dui));
		ASSERT_EQUALS("", preprocess(code));
	}
	{ //has_include_2()
		const char code[] = "#if defined( __has_include)\n"
			"  #if /*comment*/ __has_include /*comment*/(\"simplecpp.h\") // comment\n"
			"    A\n"
			"  #else\n"
			"    B\n"
			"  #endif\n"
			"#endif";
		simplecpp::DUI dui;
		dui.std = "c++17";
		//ASSERT_EQUALS("\n\nA", preprocess(code, dui)); // ! Не срабатывает - какие-то проблемы с __has_include
		ASSERT_EQUALS("", preprocess(code));
	}
	{ //has_include_3()
		const char code[] = "#ifdef __has_include\n"
			"  #if __has_include(<realFileName1.cpp>)\n"
			"    A\n"
			"  #else\n"
			"    B\n"
			"  #endif\n"
			"#endif";
		simplecpp::DUI dui;
		dui.std = "c++17";
		// Test file not found...
		ASSERT_EQUALS("\n\n\n\nB", preprocess(code, dui));
		// Unless -I is set (preferably, we should differentiate -I and -isystem...)
		dui.includePaths.push_back("./testsuite");
		//ASSERT_EQUALS("\n\nA", preprocess(code, dui)); // ! Не срабатывает - какие-то проблемы с __has_include
		ASSERT_EQUALS("", preprocess(code));
	}
	{ //has_include_4()
		const char code[] = "#ifdef __has_include\n"
			"  #if __has_include(\"testsuite/realFileName1.cpp\")\n"
			"    A\n"
			"  #else\n"
			"    B\n"
			"  #endif\n"
			"#endif";
		simplecpp::DUI dui;
		dui.std = "c++17";
		//ASSERT_EQUALS("\n\nA", preprocess(code, dui)); // ! Не срабатывает - какие-то проблемы с __has_include
		ASSERT_EQUALS("", preprocess(code));
	}
	{ //has_include_5()
		const char code[] = "#if defined( __has_include)\n"
			"  #if !__has_include(<testsuite/unrealFileName2.abcdef>)\n"
			"    A\n"
			"  #else\n"
			"    B\n"
			"  #endif\n"
			"#endif";
		simplecpp::DUI dui;
		dui.std = "c++17";
		ASSERT_EQUALS("\n\nA", preprocess(code, dui));
		ASSERT_EQUALS("", preprocess(code));
	}
	{//ifdef1()
		const char code[] = "#ifdef A\n"
			"1\n"
			"#else\n"
			"2\n"
			"#endif";
		ASSERT_EQUALS("\n\n\n2", preprocess(code));
	}
	{//ifdef2()
		const char code[] = "#define A\n"
			"#ifdef A\n"
			"1\n"
			"#else\n"
			"2\n"
			"#endif";
		ASSERT_EQUALS("\n\n1", preprocess(code));
	}
	{//ifndef()
		const char code1[] = "#define A\n"
			"#ifndef A\n"
			"1\n"
			"#endif";
		ASSERT_EQUALS("", preprocess(code1));

		const char code2[] = "#ifndef A\n"
			"1\n"
			"#endif";
		ASSERT_EQUALS("\n1", preprocess(code2));
	}
	{//ifA()
		const char code[] = "#if A==1\n"
			"X\n"
			"#endif";
		ASSERT_EQUALS("", preprocess(code));

		simplecpp::DUI dui;
		dui.defines.push_back("A=1");
		ASSERT_EQUALS("\nX", preprocess(code, dui));
	}
	{//ifCharLiteral()
		const char code[] = "#if ('A'==0x41)\n"
			"123\n"
			"#endif";
		ASSERT_EQUALS("\n123", preprocess(code));
	}
	{//ifDefined()
		const char code[] = "#if defined(A)\n"
			"X\n"
			"#endif";
		simplecpp::DUI dui;
		ASSERT_EQUALS("", preprocess(code, dui));
		dui.defines.push_back("A=1");
		ASSERT_EQUALS("\nX", preprocess(code, dui));
	}
	{//ifDefinedNoPar()
		const char code[] = "#if defined A\n"
			"X\n"
			"#endif";
		simplecpp::DUI dui;
		ASSERT_EQUALS("", preprocess(code, dui));
		dui.defines.push_back("A=1");
		ASSERT_EQUALS("\nX", preprocess(code, dui));
	}
	{//ifDefinedNested()
		const char code[] = "#define FOODEF defined(FOO)\n"
			"#if FOODEF\n"
			"X\n"
			"#endif";
		simplecpp::DUI dui;
		ASSERT_EQUALS("", preprocess(code, dui));
		dui.defines.push_back("FOO=1");
		ASSERT_EQUALS("\n\nX", preprocess(code, dui));
	}
	{//ifDefinedNestedNoPar()
		const char code[] = "#define FOODEF defined FOO\n"
			"#if FOODEF\n"
			"X\n"
			"#endif";
		simplecpp::DUI dui;
		ASSERT_EQUALS("", preprocess(code, dui));
		dui.defines.push_back("FOO=1");
		ASSERT_EQUALS("\n\nX", preprocess(code, dui));
	}
	{//ifDefinedInvalid1()   // #50 - invalid unterminated defined
		const char code[] = "#if defined(A";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition\n", toString(outputList));
	}
	{//ifDefinedInvalid2()
		const char code[] = "#if defined";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition\n", toString(outputList));
	}
	{//ifDefinedHashHash()
		const char code[] = "#define ENABLE(FEATURE)  defined ENABLE_##FEATURE\n"
			"#define ENABLE_FOO 1\n"
			"#if ENABLE(FOO)\n"
			"#error FOO is enabled\n"             // <-- expected result
			"#else\n"
			"#error FOO is not enabled\n"
			"#endif\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,4,#error,#error FOO is enabled\n", toString(outputList));
	}
	{//ifDefinedHashHash2()
		// #409
		// do not crash when expanding P()  (as ## rhs is "null")
		// note: gcc outputs "defined E"
		const char code[] = "#define P(p)defined E##p\n"
			"P()\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("\n0", preprocess(code, &outputList));
	}
	{//ifLogical()
		const char code[] = "#if defined(A) || defined(B)\n"
			"X\n"
			"#endif";
		simplecpp::DUI dui;
		ASSERT_EQUALS("", preprocess(code, dui));
		dui.defines.clear();
		dui.defines.push_back("A=1");
		ASSERT_EQUALS("\nX", preprocess(code, dui));
		dui.defines.clear();
		dui.defines.push_back("B=1");
		ASSERT_EQUALS("\nX", preprocess(code, dui));
	}
	{//ifSizeof()
		const char code[] = "#if sizeof(unsigned short)==2\n"
			"X\n"
			"#else\n"
			"Y\n"
			"#endif";
		ASSERT_EQUALS("\nX", preprocess(code));
	}
	{//elif()
		const char code1[] = "#ifndef X\n"
			"1\n"
			"#elif 1<2\n"
			"2\n"
			"#else\n"
			"3\n"
			"#endif";
		ASSERT_EQUALS("\n1", preprocess(code1));

		const char code2[] = "#ifdef X\n"
			"1\n"
			"#elif 1<2\n"
			"2\n"
			"#else\n"
			"3\n"
			"#endif";
		ASSERT_EQUALS("\n\n\n2", preprocess(code2));

		const char code3[] = "#ifdef X\n"
			"1\n"
			"#elif 1>2\n"
			"2\n"
			"#else\n"
			"3\n"
			"#endif";
		ASSERT_EQUALS("\n\n\n\n\n3", preprocess(code3));
	}
	{//ifif()
		// source code from LLVM
		const char code[] = "#if defined(__has_include)\n"
			"#if __has_include(<sanitizer / coverage_interface.h>)\n"
			"#endif\n"
			"#endif\n";
		ASSERT_EQUALS("", preprocess(code));
	}
	{//ifoverflow()
		// source code from CLANG
		const char code[] = "#if 0x7FFFFFFFFFFFFFFF*2\n"
			"#endif\n"
			"#if 0xFFFFFFFFFFFFFFFF*2\n"
			"#endif\n"
			"#if 0x7FFFFFFFFFFFFFFF+1\n"
			"#endif\n"
			"#if 0xFFFFFFFFFFFFFFFF+1\n"
			"#endif\n"
			"#if 0x7FFFFFFFFFFFFFFF--1\n"
			"#endif\n"
			"#if 0xFFFFFFFFFFFFFFFF--1\n"
			"#endif\n"
			"123";
		(void)preprocess(code);
	}
	{//ifdiv0()
		const char code[] = "#if 1000/0\n"
			"#endif\n"
			"123";
		ASSERT_EQUALS("", preprocess(code));
	}
	{//ifalt()   // using "and", "or", etc
		const char * code;
		code = "#if 1 and 1\n"
			"1\n"
			"#else\n"
			"2\n"
			"#endif\n";
		ASSERT_EQUALS("\n1", preprocess(code));
		code = "#if 1 or 0\n"
			"1\n"
			"#else\n"
			"2\n"
			"#endif\n";
		ASSERT_EQUALS("\n1", preprocess(code));
	}
	{//ifexpr()
		const char code[] = "#define MACRO()  (1)\n"
			"#if ~MACRO() & 8\n"
			"1\n"
			"#endif";
		ASSERT_EQUALS("\n\n1", preprocess(code));
	}
	{//location1()
		const char * code =  "# 1 \"main.c\"\n\n\n"
			"x";
		ASSERT_EQUALS("\n#line 3 \"main.c\"\nx", preprocess(code));
	}
	{//location2()
		const char * code = "{ {\n"
			"#line 40 \"abc.y\"\n"
			"{\n"
			"}\n"
			"#line 42 \"abc.y\"\n"
			"{\n"
			"}\n"
			"} }";
		ASSERT_EQUALS("{ {\n"
			"#line 40 \"abc.y\"\n"
			"{\n"
			"}\n"
			"{\n"
			"}\n"
			"} }", preprocess(code));
	}
	{//location3()
		const char * code = "#line 1 \"x\" \n"
			"a\n"
			"#line 1 \"x\" \n"
			"b\n";
		ASSERT_EQUALS("\n#line 1 \"x\"\na b", preprocess(code));
	}
	{//location4()
		const char * code = "#line 1 \"abc\\\\def.g\" \n"
			"a\n";
		ASSERT_EQUALS("\n#line 1 \"abc\\def.g\"\na", preprocess(code));
	}
	{//location5()
		// https://sourceforge.net/p/cppcheck/discussion/general/thread/eccf020a13/
		const char * code;
		code = "#line 10 \"/a/Attribute/parser/FilterParser.y\" // lalr1.cc:377\n"
			"int x;\n";
		ASSERT_EQUALS("\n#line 10 \"/a/Attribute/parser/FilterParser.y\"\n"
			"int x ;", preprocess(code));
	}
	{//missingHeader1()
		const char code[] = "#include \"notexist.h\"\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,missing_header,Header not found: \"notexist.h\"\n", toString(outputList));
	}
	{//missingHeader2()
		const char code[] = "#include \"foo.h\"\n"; // this file exists
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		filedata["foo.h"] = nullptr;
		simplecpp::OutputList outputList;
		simplecpp::TokenList tokens2(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS("", toString(outputList));
	}
	{//missingHeader3()
		const char code[] = "#ifdef UNDEFINED\n#include \"notexist.h\"\n#endif\n"; // this file is not included
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("", toString(outputList));
	}
	{//missingHeader4()
		const char code[] = "#/**/include <>\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,No header in #include\n", toString(outputList));
	}
	{//nestedInclude()
		const char code[] = "#include \"test.h\"\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens = makeTokenList(code, files, "test.h");
		std::map<std::string, simplecpp::TokenList*> filedata;
		filedata["test.h"] = &rawtokens;

		simplecpp::OutputList outputList;
		simplecpp::TokenList tokens2(files);
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI(), &outputList);

		ASSERT_EQUALS("file0,1,include_nested_too_deeply,#include nested too deeply\n", toString(outputList));
	}
	{//systemInclude()
		const char code[] = "#include <limits.h>\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens = makeTokenList(code, files, "local/limits.h");
		std::map<std::string, simplecpp::TokenList*> filedata;
		filedata["limits.h"] = nullptr;
		filedata["local/limits.h"] = &rawtokens;
		simplecpp::OutputList outputList;
		simplecpp::TokenList tokens2(files);
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI(), &outputList);
		ASSERT_EQUALS("", toString(outputList));
	}
	{//nullDirective1()
		const char code[] = "#\n"
			"#if 1\n"
			"#define a 1\n"
			"#endif\n"
			"x = a;\n";

		ASSERT_EQUALS("\n\n\n\nx = 1 ;", preprocess(code));
	}
	{//nullDirective2()
		const char code[] = "# // comment\n"
			"#if 1\n"
			"#define a 1\n"
			"#endif\n"
			"x = a;\n";

		ASSERT_EQUALS("\n\n\n\nx = 1 ;", preprocess(code));
	}
	{//nullDirective3()
		const char code[] = "#if 1\n"
			"#define a 1\n"
			"#\n"
			"#endif\n"
			"x = a;\n";

		ASSERT_EQUALS("\n\n\n\nx = 1 ;", preprocess(code));
	}
	{//include1()
		const char code[] = "#include \"A.h\"\n";
		ASSERT_EQUALS("# include \"A.h\"", readfile(code));
	}
	{//include2()
		const char code[] = "#include <A.h>\n";
		ASSERT_EQUALS("# include <A.h>", readfile(code));
	}
	{//include3()   // #16 - crash when expanding macro from header
		const char code_c[] = "#include \"A.h\"\n"
			"glue(1,2,3,4)\n";
		const char code_h[] = "#define glue(a,b,c,d) a##b##c##d\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens_c = makeTokenList(code_c, files, "A.c");
		simplecpp::TokenList rawtokens_h = makeTokenList(code_h, files, "A.h");
		ASSERT_EQUALS(2U, files.size());
		ASSERT_EQUALS("A.c", files[0]);
		ASSERT_EQUALS("A.h", files[1]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["A.c"] = &rawtokens_c;
		filedata["A.h"] = &rawtokens_h;
		simplecpp::TokenList out(files);
		simplecpp::preprocess(out, rawtokens_c, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("\n1234", out.stringify());
	}
	{//include4()   // #27 - -include
		const char code_c[] = "X\n";
		const char code_h[] = "#define X 123\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens_c = makeTokenList(code_c, files, "27.c");
		simplecpp::TokenList rawtokens_h = makeTokenList(code_h, files, "27.h");
		ASSERT_EQUALS(2U, files.size());
		ASSERT_EQUALS("27.c", files[0]);
		ASSERT_EQUALS("27.h", files[1]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["27.c"] = &rawtokens_c;
		filedata["27.h"] = &rawtokens_h;
		simplecpp::TokenList out(files);
		simplecpp::DUI dui;
		dui.includes.push_back("27.h");
		simplecpp::preprocess(out, rawtokens_c, files, filedata, dui);
		ASSERT_EQUALS("123", out.stringify());
	}
	{//include5()    // #3 - handle #include MACRO
		const char code_c[] = "#define A \"3.h\"\n#include A\n";
		const char code_h[] = "123\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens_c = makeTokenList(code_c, files, "3.c");
		simplecpp::TokenList rawtokens_h = makeTokenList(code_h, files, "3.h");
		ASSERT_EQUALS(2U, files.size());
		ASSERT_EQUALS("3.c", files[0]);
		ASSERT_EQUALS("3.h", files[1]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["3.c"] = &rawtokens_c;
		filedata["3.h"] = &rawtokens_h;
		simplecpp::TokenList out(files);
		simplecpp::preprocess(out, rawtokens_c, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("\n#line 1 \"3.h\"\n123", out.stringify());
	}
	{//include6()   // #57 - incomplete macro  #include MACRO(,)
		const char code[] = "#define MACRO(X,Y) X##Y\n#include MACRO(,)\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens = makeTokenList(code, files, "57.c");
		ASSERT_EQUALS(1U, files.size());
		ASSERT_EQUALS("57.c", files[0]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["57.c"] = &rawtokens;
		simplecpp::TokenList out(files);
		simplecpp::preprocess(out, rawtokens, files, filedata, simplecpp::DUI());
	}
	{//include7()    // #include MACRO
		const char code_c[] = "#define HDR  <3.h>\n"
			"#include HDR\n";
		const char code_h[] = "123\n";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens_c = makeTokenList(code_c, files, "3.c");
		simplecpp::TokenList rawtokens_h = makeTokenList(code_h, files, "3.h");
		ASSERT_EQUALS(2U, files.size());
		ASSERT_EQUALS("3.c", files[0]);
		ASSERT_EQUALS("3.h", files[1]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["3.c"] = &rawtokens_c;
		filedata["3.h"] = &rawtokens_h;
		simplecpp::TokenList out(files);
		simplecpp::DUI dui;
		dui.includePaths.push_back(".");
		simplecpp::preprocess(out, rawtokens_c, files, filedata, dui);
		ASSERT_EQUALS("\n#line 1 \"3.h\"\n123", out.stringify());
	}
	{//include8()    // #include MACRO(X)
		const char code[] = "#define INCLUDE_LOCATION ../somewhere\n"
			"#define INCLUDE_FILE(F)  <INCLUDE_LOCATION/F.h>\n"
			"#include INCLUDE_FILE(header)\n";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,3,missing_header,Header not found: <../somewhere/header.h>\n", toString(outputList));
	}
	{//include9() //#include MACRO
		const char code_c[] = "#define HDR \"1.h\"\n"
			"#include HDR\n";
		const char code_h[] = "/**/ #define X 1\n" // <- comment before hash should be ignored
			"x=X;";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens_c = makeTokenList(code_c, files, "1.c");
		simplecpp::TokenList rawtokens_h = makeTokenList(code_h, files, "1.h");
		ASSERT_EQUALS(2U, files.size());
		ASSERT_EQUALS("1.c", files[0]);
		ASSERT_EQUALS("1.h", files[1]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["1.c"] = &rawtokens_c;
		filedata["1.h"] = &rawtokens_h;
		simplecpp::TokenList out(files);
		simplecpp::DUI dui;
		dui.includePaths.push_back(".");
		simplecpp::preprocess(out, rawtokens_c, files, filedata, dui);
		ASSERT_EQUALS("\n#line 2 \"1.h\"\nx = 1 ;", out.stringify());
	}
	{//multiline1()
		const char code[] = "#define A \\\n"
			"1\n"
			"A";
		ASSERT_EQUALS("\n\n1", preprocess(code));
	}
	{//multiline2()
		const char code[] = "#define A /*\\\n"
			"*/1\n"
			"A";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens = makeTokenList(code, files);
		ASSERT_EQUALS("# define A /**/ 1\n\nA", rawtokens.stringify());
		rawtokens.removeComments();
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokens2(files);
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("\n\n1", tokens2.stringify());
	}
	{//multiline3()   // #28 - macro with multiline comment
		const char code[] = "#define A /*\\\n"
			"           */ 1\n"
			"A";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens = makeTokenList(code, files);
		ASSERT_EQUALS("# define A /*           */ 1\n\nA", rawtokens.stringify());
		rawtokens.removeComments();
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokens2(files);
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("\n\n1", tokens2.stringify());
	}
	{//multiline4()   // #28 - macro with multiline comment
		const char code[] = "#define A \\\n"
			"          /*\\\n"
			"           */ 1\n"
			"A";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens = makeTokenList(code, files);
		ASSERT_EQUALS("# define A /*           */ 1\n\n\nA", rawtokens.stringify());
		rawtokens.removeComments();
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokens2(files);
		simplecpp::preprocess(tokens2, rawtokens, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("\n\n\n1", tokens2.stringify());
	}
	{//multiline5()   // column
		const char code[] = "#define A\\\n"
			"(";
		std::vector<std::string> files;
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		ASSERT_EQUALS("# define A (", rawtokens.stringify());
		ASSERT_EQUALS(11, rawtokens.cback()->location.col);
	}
	{//multiline6()   // multiline string in macro
		const char code[] = "#define string  (\"\\\n"
			"x\")\n"
			"string\n";
		std::vector<std::string> files;
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		ASSERT_EQUALS("# define string ( \"x\" )\n"
			"\n"
			"string", rawtokens.stringify());
	}
	{//static multiline7()   // multiline string in macro
		const char code[] = "#define A(X) aaa { f(\"\\\n"
			"a\"); }\n"
			"A(1)";
		std::vector<std::string> files;
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		ASSERT_EQUALS("# define A ( X ) aaa { f ( \"a\" ) ; }\n"
			"\n"
			"A ( 1 )", rawtokens.stringify());
	}
	{//multiline8()   // multiline prefix string in macro
		const char code[] = "#define A L\"a\\\n"
			" b\"\n"
			"A;";
		ASSERT_EQUALS("\n\nL\"a b\" ;", preprocess(code));
	}
	{//multiline9()   // multiline prefix string in macro
		const char code[] = "#define A u8\"a\\\n"
			" b\"\n"
			"A;";
		ASSERT_EQUALS("\n\nu8\"a b\" ;", preprocess(code));
	}
	{//multiline10() // multiline string literal
		const char code[] = "const char *ptr = \"\\\\\n"
			"\\n\";";
		ASSERT_EQUALS("const char * ptr = \"\\\\n\"\n;", preprocess(code));
	}
	{//readfile_nullbyte()
		const char code[] = "ab\0cd";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("ab cd", readfile(code, sizeof(code), &outputList));
		ASSERT_EQUALS(true, outputList.empty()); // should warning be written?
	}
	{//readfile_char()
		ASSERT_EQUALS("'c'", readfile("'c'"));
		ASSERT_EQUALS("L'c'", readfile("L'c'"));
		ASSERT_EQUALS("L 'c'", readfile("L 'c'"));
		ASSERT_EQUALS("A = 'c'", readfile("A = 'c'"));
		ASSERT_EQUALS("A = L'c'", readfile("A = L'c'"));
		ASSERT_EQUALS("A = u'c'", readfile("A = u'c'"));
		ASSERT_EQUALS("A = U'c'", readfile("A = U'c'"));
		ASSERT_EQUALS("A = u8'c'", readfile("A = u8'c'"));
		ASSERT_EQUALS("A = u8 'c'", readfile("A = u8 'c'"));

		// The character \'
		ASSERT_EQUALS("'\\''", readfile("'\\\''"));
		ASSERT_EQUALS("L'\\''", readfile("L'\\\''"));
		ASSERT_EQUALS("u'\\''", readfile("u'\\\''"));
		ASSERT_EQUALS("U'\\''", readfile("U'\\\''"));
		ASSERT_EQUALS("u8'\\''", readfile("u8'\\\''"));
	}
	{//readfile_char_error()
		simplecpp::OutputList outputList;

		readfile("A = L's", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,No pair for character (\'). Can't process file. File is either invalid or unicode, which is currently not supported.\n",
			toString(outputList));
		outputList.clear();

		readfile("A = 's\n'", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,No pair for character (\'). Can't process file. File is either invalid or unicode, which is currently not supported.\n",
			toString(outputList));
	}
	{//readfile_string()
		ASSERT_EQUALS("\"\"", readfile("\"\""));
		ASSERT_EQUALS("\"a\"", readfile("\"a\""));
		ASSERT_EQUALS("u\"a\"", readfile("u\"a\""));
		ASSERT_EQUALS("u \"a\"", readfile("u \"a\""));
		ASSERT_EQUALS("A = \"\"", readfile("A = \"\""));
		ASSERT_EQUALS("A = \"abc\'def\"", readfile("A = \"abc\'def\""));
		ASSERT_EQUALS("( \"\\\\\\\\\" )", readfile("(\"\\\\\\\\\")"));
		ASSERT_EQUALS("x = \"a  b\"\n;", readfile("x=\"a\\\n  b\";"));
		ASSERT_EQUALS("x = \"a  b\"\n;", readfile("x=\"a\\\r\n  b\";"));

		ASSERT_EQUALS("A = u8\"a\"", readfile("A = u8\"a\""));
		ASSERT_EQUALS("A = u\"a\"", readfile("A = u\"a\""));
		ASSERT_EQUALS("A = U\"a\"", readfile("A = U\"a\""));
		ASSERT_EQUALS("A = L\"a\"", readfile("A = L\"a\""));
		ASSERT_EQUALS("A = L \"a\"", readfile("A = L \"a\""));
		ASSERT_EQUALS("A = \"abc\\\\\\\\def\"", readfile("A = R\"(abc\\\\def)\""));
		ASSERT_EQUALS("A = \"abc\\\\\\\\def\"", readfile("A = R\"x(abc\\\\def)x\""));
		ASSERT_EQUALS("A = \"\"", readfile("A = R\"()\""));
		ASSERT_EQUALS("A = \"\\\\\"", readfile("A = R\"(\\)\""));
		ASSERT_EQUALS("A = \"\\\"\"", readfile("A = R\"(\")\""));
		ASSERT_EQUALS("A = \"abc\"", readfile("A = R\"\"\"(abc)\"\"\""));
		ASSERT_EQUALS("A = \"a\nb\nc\";", readfile("A = R\"foo(a\nb\nc)foo\";"));
		ASSERT_EQUALS("A = L\"abc\"", readfile("A = LR\"(abc)\""));
		ASSERT_EQUALS("A = u\"abc\"", readfile("A = uR\"(abc)\""));
		ASSERT_EQUALS("A = U\"abc\"", readfile("A = UR\"(abc)\""));
		ASSERT_EQUALS("A = u8\"abc\"", readfile("A = u8R\"(abc)\""));

		// Strings containing \"
		ASSERT_EQUALS("\"\\\"\"", readfile("\"\\\"\""));
		ASSERT_EQUALS("L\"\\\"\"", readfile("L\"\\\"\""));
		ASSERT_EQUALS("u\"\\\"\"", readfile("u\"\\\"\""));
		ASSERT_EQUALS("U\"\\\"\"", readfile("U\"\\\"\""));
		ASSERT_EQUALS("u8\"\\\"\"", readfile("u8\"\\\"\""));
		ASSERT_EQUALS("\"\\\"a\\\"\"", readfile("\"\\\"a\\\"\""));
		ASSERT_EQUALS("L\"a\\\"\\\"\"", readfile("L\"a\\\"\\\"\""));
		ASSERT_EQUALS("u\"a\\\"\\\"\"", readfile("u\"a\\\"\\\"\""));
		ASSERT_EQUALS("U\"a\\\"\\\"\"", readfile("U\"a\\\"\\\"\""));
		ASSERT_EQUALS("u8\"a\\\"\\\"\"", readfile("u8\"a\\\"\\\"\""));

		// Do not concatenate when prefix is not directly adjacent to "
		ASSERT_EQUALS("u8 \"a b\"", readfile("u8 \"a b\""));
		ASSERT_EQUALS("u8\n\"a b\"", readfile("u8\n  \"a b\""));
	}
	{//readfile_string_error()
		simplecpp::OutputList outputList;

		readfile("A = \"abs", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,No pair for character (\"). Can't process file. File is either invalid or unicode, which is currently not supported.\n",
			toString(outputList));
		outputList.clear();

		readfile("A = u8\"abs\n\"", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,No pair for character (\"). Can't process file. File is either invalid or unicode, which is currently not supported.\n",
			toString(outputList));
		outputList.clear();

		readfile("A = R\"as\n(abc)as\"", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,Invalid newline in raw string delimiter.\n", toString(outputList));
		outputList.clear();

		readfile("A = u8R\"as\n(abc)as\"", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,Invalid newline in raw string delimiter.\n", toString(outputList));
		outputList.clear();

		readfile("A = R\"as(abc)a\"", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,Raw string missing terminating delimiter.\n", toString(outputList));
		outputList.clear();

		readfile("A = LR\"as(abc)a\"", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,Raw string missing terminating delimiter.\n", toString(outputList));
		outputList.clear();

		readfile("#define A \"abs", &outputList);
		ASSERT_EQUALS("file0,1,syntax_error,No pair for character (\"). Can't process file. File is either invalid or unicode, which is currently not supported.\n",
			toString(outputList));
		outputList.clear();

		// Don't warn for a multiline define
		readfile("#define A \"abs\\\n\"", &outputList);
		ASSERT_EQUALS("", toString(outputList));
	}
	{//readfile_cpp14_number()
		ASSERT_EQUALS("A = 12345 ;", readfile("A = 12\'345;"));
	}
	{//readfile_unhandled_chars()
		simplecpp::OutputList outputList;
		readfile("// 你好世界", &outputList);
		ASSERT_EQUALS("", toString(outputList));
		readfile("s=\"你好世界\"", &outputList);
		ASSERT_EQUALS("", toString(outputList));
		readfile("int 你好世界=0;", &outputList);
		ASSERT_EQUALS("file0,1,unhandled_char_error,The code contains unhandled character(s) (character code=228). Neither unicode nor extended ascii is supported.\n",
			toString(outputList));
	}
	{//readfile_error()
		ASSERT_EQUALS("# if ! A\n"
			"# error\n"
			"# endif\n"
			"X", readfile("#if !A\n#error\n#endif\nX\n"));
	}
	{//readfile_file_not_found()
		simplecpp::OutputList outputList;
		std::vector<std::string> files;
		(void)simplecpp::TokenList("NotAFile", files, &outputList);
		ASSERT_EQUALS("file0,1,file_not_found,File is missing: NotAFile\n", toString(outputList));
	}
	{//stringify1()
		const char code_c[] = "#include \"A.h\"\n"
			"#include \"A.h\"\n";
		const char code_h[] = "1\n2";
		std::vector<std::string> files;
		simplecpp::TokenList rawtokens_c = makeTokenList(code_c, files, "A.c");
		simplecpp::TokenList rawtokens_h = makeTokenList(code_h, files, "A.h");
		ASSERT_EQUALS(2U, files.size());
		ASSERT_EQUALS("A.c", files[0]);
		ASSERT_EQUALS("A.h", files[1]);
		std::map<std::string, simplecpp::TokenList *> filedata;
		filedata["A.c"] = &rawtokens_c;
		filedata["A.h"] = &rawtokens_h;
		simplecpp::TokenList out(files);
		simplecpp::preprocess(out, rawtokens_c, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("\n#line 1 \"A.h\"\n1\n2\n#line 1 \"A.h\"\n1\n2", out.stringify());
	}
	{//tokenMacro1()
		const char code[] = "#define A 123\n"
			"A";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokenList(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		simplecpp::preprocess(tokenList, rawtokens, files, filedata, simplecpp::DUI());
		ASSERT_EQUALS("A", tokenList.cback()->macro);
	}
	{//tokenMacro2()
		const char code[] = "#define ADD(X,Y) X+Y\n"
			"ADD(1,2)";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokenList(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		simplecpp::preprocess(tokenList, rawtokens, files, filedata, simplecpp::DUI());
		const simplecpp::Token * tok = tokenList.cfront();
		ASSERT_EQUALS("1", tok->str());
		ASSERT_EQUALS("", tok->macro);
		tok = tok->next;
		ASSERT_EQUALS("+", tok->str());
		ASSERT_EQUALS("ADD", tok->macro);
		tok = tok->next;
		ASSERT_EQUALS("2", tok->str());
		ASSERT_EQUALS("", tok->macro);
	}
	{//tokenMacro3()
		const char code[] = "#define ADD(X,Y) X+Y\n"
			"#define FRED  1\n"
			"ADD(FRED,2)";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokenList(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		simplecpp::preprocess(tokenList, rawtokens, files, filedata, simplecpp::DUI());
		const simplecpp::Token * tok = tokenList.cfront();
		ASSERT_EQUALS("1", tok->str());
		ASSERT_EQUALS("FRED", tok->macro);
		tok = tok->next;
		ASSERT_EQUALS("+", tok->str());
		ASSERT_EQUALS("ADD", tok->macro);
		tok = tok->next;
		ASSERT_EQUALS("2", tok->str());
		ASSERT_EQUALS("", tok->macro);
	}
	{//tokenMacro4()
		const char code[] = "#define A B\n"
			"#define B 1\n"
			"A";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokenList(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		simplecpp::preprocess(tokenList, rawtokens, files, filedata, simplecpp::DUI());
		const simplecpp::Token * const tok = tokenList.cfront();
		ASSERT_EQUALS("1", tok->str());
		ASSERT_EQUALS("A", tok->macro);
	}
	{//tokenMacro5()
		const char code[] = "#define SET_BPF(code) (code)\n"
			"#define SET_BPF_JUMP(code) SET_BPF(D | code)\n"
			"SET_BPF_JUMP(A | B | C);";
		std::vector<std::string> files;
		std::map<std::string, simplecpp::TokenList*> filedata;
		simplecpp::TokenList tokenList(files);
		const simplecpp::TokenList rawtokens = makeTokenList(code, files);
		simplecpp::preprocess(tokenList, rawtokens, files, filedata, simplecpp::DUI());
		const simplecpp::Token * const tok = tokenList.cfront()->next;
		ASSERT_EQUALS("D", tok->str());
		ASSERT_EQUALS("SET_BPF_JUMP", tok->macro);
	}
	{//undef()
		const char code[] = "#define A\n"
			"#undef A\n"
			"#ifdef A\n"
			"123\n"
			"#endif";
		ASSERT_EQUALS("", preprocess(code));
	}
	{//userdef()
		const char code[] = "#ifdef A\n123\n#endif\n";
		simplecpp::DUI dui;
		dui.defines.push_back("A=1");
		ASSERT_EQUALS("\n123", preprocess(code, dui));
	}
	// utf/unicode
	{//utf8()
		ASSERT_EQUALS("123", readfile("\xEF\xBB\xBF 123"));
	}
	{//utf8_invalid()
		ASSERT_EQUALS("", readfile("\xEF 123"));
		ASSERT_EQUALS("", readfile("\xEF\xBB 123"));
	}
	{//unicode()
		{
			const char code[] = "\xFE\xFF\x00\x31\x00\x32";
			ASSERT_EQUALS("12", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFF\xFE\x31\x00\x32\x00";
			ASSERT_EQUALS("12", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFE\xFF\x00\x2f\x00\x2f\x00\x0a\x00\x31";
			ASSERT_EQUALS("//\n1", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFF\xFE\x2f\x00\x2f\x00\x0a\x00\x31\x00";
			ASSERT_EQUALS("//\n1", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFE\xFF\x00\x22\x00\x61\x00\x22";
			ASSERT_EQUALS("\"a\"", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFF\xFE\x22\x00\x61\x00\x22\x00";
			ASSERT_EQUALS("\"a\"", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xff\xfe\x0d\x00\x0a\x00\x2f\x00\x2f\x00\x31\x00\x0d\x00\x0a\x00";
			ASSERT_EQUALS("\n//1", readfile(code, sizeof(code)));
		}
	}
	{//unicode_invalid()
		{
			const char code[] = "\xFF";
			ASSERT_EQUALS("", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFE";
			ASSERT_EQUALS("", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFE\xFF\x31";
			ASSERT_EQUALS("", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFF\xFE\x31";
			ASSERT_EQUALS("1", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFE\xFF\x31\x32";
			ASSERT_EQUALS("", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFF\xFE\x31\x32";
			ASSERT_EQUALS("", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFE\xFF\x00\x31\x00\x32\x33";
			ASSERT_EQUALS("", readfile(code, sizeof(code)));
		}
		{
			const char code[] = "\xFF\xFE\x31\x00\x32\x00\x33";
			ASSERT_EQUALS("123", readfile(code, sizeof(code)));
		}
	}
	{//warning()
		const char code[] = "#warning MSG\n1";
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("\n1", preprocess(code, &outputList));
		ASSERT_EQUALS("file0,1,#warning,#warning MSG\n", toString(outputList));
	}
	{//simplifyPath()
		ASSERT_EQUALS("1.c", simplecpp::simplifyPath("./1.c"));
		ASSERT_EQUALS("1.c", simplecpp::simplifyPath("././1.c"));
		ASSERT_EQUALS("/1.c", simplecpp::simplifyPath("/./1.c"));
		ASSERT_EQUALS("/1.c", simplecpp::simplifyPath("/././1.c"));
		ASSERT_EQUALS("trailing_dot./1.c", simplecpp::simplifyPath("trailing_dot./1.c"));

		ASSERT_EQUALS("1.c", simplecpp::simplifyPath("a/../1.c"));
		ASSERT_EQUALS("1.c", simplecpp::simplifyPath("a/b/../../1.c"));
		ASSERT_EQUALS("a/1.c", simplecpp::simplifyPath("a/b/../1.c"));
		ASSERT_EQUALS("/a/1.c", simplecpp::simplifyPath("/a/b/../1.c"));
		ASSERT_EQUALS("/a/1.c", simplecpp::simplifyPath("/a/b/c/../../1.c"));
		ASSERT_EQUALS("/a/1.c", simplecpp::simplifyPath("/a/b/c/../.././1.c"));

		ASSERT_EQUALS("../1.c", simplecpp::simplifyPath("../1.c"));
		ASSERT_EQUALS("../1.c", simplecpp::simplifyPath("../a/../1.c"));
		ASSERT_EQUALS("/../1.c", simplecpp::simplifyPath("/../1.c"));
		ASSERT_EQUALS("/../1.c", simplecpp::simplifyPath("/../a/../1.c"));

		ASSERT_EQUALS("a/..b/1.c", simplecpp::simplifyPath("a/..b/1.c"));
		ASSERT_EQUALS("../../1.c", simplecpp::simplifyPath("../../1.c"));
		ASSERT_EQUALS("../../../1.c", simplecpp::simplifyPath("../../../1.c"));
		ASSERT_EQUALS("../../../1.c", simplecpp::simplifyPath("../../../a/../1.c"));
		ASSERT_EQUALS("../../1.c", simplecpp::simplifyPath("a/../../../1.c"));
	}
	{//simplifyPath_cppcheck()
		// tests transferred from cppcheck
		// https://github.com/danmar/cppcheck/blob/d3e79b71b5ec6e641ca3e516cfced623b27988af/test/testpath.cpp#L43
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath(".//index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath(".///index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/index.h"));
		ASSERT_EQUALS("/path/", simplecpp::simplifyPath("/path/"));
		ASSERT_EQUALS("/", simplecpp::simplifyPath("/"));
		ASSERT_EQUALS("/", simplecpp::simplifyPath("/."));
		ASSERT_EQUALS("/", simplecpp::simplifyPath("/./"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/./index.h"));
		ASSERT_EQUALS("/", simplecpp::simplifyPath("/.//"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/.//index.h"));
		ASSERT_EQUALS("../index.h", simplecpp::simplifyPath("../index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path/../index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path/../index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path//../index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path//../index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path//../index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/..//index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path/..//index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path/..//index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path//..//index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("./path//..//index.h"));
		ASSERT_EQUALS("index.h", simplecpp::simplifyPath("path//..//index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other/../index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other///././../index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other/././..///index.h"));
		ASSERT_EQUALS("/index.h", simplecpp::simplifyPath("/path/../other///././..///index.h"));
		ASSERT_EQUALS("../path/index.h", simplecpp::simplifyPath("../path/other/../index.h"));
		ASSERT_EQUALS("a/index.h", simplecpp::simplifyPath("a/../a/index.h"));
		ASSERT_EQUALS(".", simplecpp::simplifyPath("a/.."));
		ASSERT_EQUALS(".", simplecpp::simplifyPath("./a/.."));
		// В следующих 2 тестах я поменял наименование каталога src на src_NotExists поскольку у меня на машине есть такой
		// каталог - он его находит и меняет регистр src-->Src в результате тест не срабатывает.
		ASSERT_EQUALS("../../src_NotExists/test.cpp", simplecpp::simplifyPath("../../src_NotExists/test.cpp"));
		ASSERT_EQUALS("../../../src_NotExists/test.cpp", simplecpp::simplifyPath("../../../src_NotExists/test.cpp"));
		ASSERT_EQUALS("src/test.cpp", simplecpp::simplifyPath(".//src/test.cpp"));
		ASSERT_EQUALS("src/test.cpp", simplecpp::simplifyPath(".///src/test.cpp"));
		ASSERT_EQUALS("test.cpp", simplecpp::simplifyPath("./././././test.cpp"));
		ASSERT_EQUALS("src/", simplecpp::simplifyPath("src/abc/.."));
		ASSERT_EQUALS("src/", simplecpp::simplifyPath("src/abc/../"));

		// Handling of UNC paths on Windows
		ASSERT_EQUALS("//" STRINGIZE(UNCHOST) "/test.cpp", simplecpp::simplifyPath("//" STRINGIZE(UNCHOST) "/test.cpp"));
		ASSERT_EQUALS("//" STRINGIZE(UNCHOST) "/test.cpp", simplecpp::simplifyPath("///" STRINGIZE(UNCHOST) "/test.cpp"));
	}
	{//simplifyPath_New()
		ASSERT_EQUALS("", simplecpp::simplifyPath(""));
		ASSERT_EQUALS("/", simplecpp::simplifyPath("/"));
		ASSERT_EQUALS("//", simplecpp::simplifyPath("//"));
		ASSERT_EQUALS("//", simplecpp::simplifyPath("///"));
		ASSERT_EQUALS("/", simplecpp::simplifyPath("\\"));
	}
	{//preprocessSizeOf()
		simplecpp::OutputList outputList;
		ASSERT_EQUALS("", preprocess("#if 3 > sizeof", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition, missing sizeof argument\n", toString(outputList));
		outputList.clear();
		ASSERT_EQUALS("", preprocess("#if 3 > sizeof A", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition, missing sizeof argument\n", toString(outputList));
		outputList.clear();
		ASSERT_EQUALS("", preprocess("#if 3 > sizeof(int", &outputList));
		ASSERT_EQUALS("file0,1,syntax_error,failed to evaluate #if condition, invalid sizeof expression\n", toString(outputList));
	}
	{//timeDefine()
		const char code[] = "__TIME__";
		const std::string t = preprocess(code);
		// "19:09:53"
		ASSERT_EQUALS(10, t.size());
		// TODO: split string and check proper ranges instead
		ASSERT_EQUALS('"', t[0]);
		ASSERT_EQUALS(true, isdigit(t[1]) != 0);
		ASSERT_EQUALS(true, isdigit(t[2]) != 0);
		ASSERT_EQUALS(':', t[3]);
		ASSERT_EQUALS(true, isdigit(t[4]) != 0);
		ASSERT_EQUALS(true, isdigit(t[5]) != 0);
		ASSERT_EQUALS(':', t[6]);
		ASSERT_EQUALS(true, isdigit(t[7]) != 0);
		ASSERT_EQUALS(true, isdigit(t[8]) != 0);
		ASSERT_EQUALS('"', t[9]);
	}
	{//dateDefine()
		const char code[] = "__DATE__";
		const std::string dt = preprocess(code);
		// "\"Mar 11 2022\""
		ASSERT_EQUALS(13, dt.size());
		// TODO: split string and check proper ranges instead
		ASSERT_EQUALS('"', dt[0]);
		ASSERT_EQUALS(true, dt[1] >= 'A' && dt[1] <= 'Z'); // uppercase letter
		ASSERT_EQUALS(true, dt[2] >= 'a' && dt[2] <= 'z'); // lowercase letter
		ASSERT_EQUALS(true, dt[3] >= 'a' && dt[3] <= 'z'); // lowercase letter
		ASSERT_EQUALS(' ', dt[4]);
		ASSERT_EQUALS(true, isdigit(dt[5]) != 0);
		ASSERT_EQUALS(true, isdigit(dt[6]) != 0);
		ASSERT_EQUALS(' ', dt[7]);
		ASSERT_EQUALS(true, isdigit(dt[8]) != 0);
		ASSERT_EQUALS(true, isdigit(dt[9]) != 0);
		ASSERT_EQUALS(true, isdigit(dt[10]) != 0);
		ASSERT_EQUALS(true, isdigit(dt[11]) != 0);
		ASSERT_EQUALS('"', dt[12]);
	}
	{//stdcVersionDefine()
		const char code[] = "#if defined(__STDC_VERSION__)\n"
			"  __STDC_VERSION__\n"
			"#endif\n";
		simplecpp::DUI dui;
		dui.std = "c11";
		ASSERT_EQUALS("\n201112L", preprocess(code, dui));
	}
	{//cpluscplusDefine()
		const char code[] = "#if defined(__cplusplus)\n"
			"  __cplusplus\n"
			"#endif\n";
		simplecpp::DUI dui;
		dui.std = "c++11";
		ASSERT_EQUALS("\n201103L", preprocess(code, dui));
	}
	{//invalidStd()
		const char code[] = "";
		simplecpp::DUI dui;
		simplecpp::OutputList outputList;

		dui.std = "c88";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(1, outputList.size());
		ASSERT_EQUALS(simplecpp::Output::Type::DUI_ERROR, outputList.cbegin()->type);
		ASSERT_EQUALS("unknown standard specified: 'c88'", outputList.cbegin()->msg);
		outputList.clear();

		dui.std = "gnu88";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(1, outputList.size());
		ASSERT_EQUALS(simplecpp::Output::Type::DUI_ERROR, outputList.cbegin()->type);
		ASSERT_EQUALS("unknown standard specified: 'gnu88'", outputList.cbegin()->msg);
		outputList.clear();

		dui.std = "d99";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(1, outputList.size());
		ASSERT_EQUALS(simplecpp::Output::Type::DUI_ERROR, outputList.cbegin()->type);
		ASSERT_EQUALS("unknown standard specified: 'd99'", outputList.cbegin()->msg);
		outputList.clear();

		dui.std = "c++77";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(1, outputList.size());
		ASSERT_EQUALS(simplecpp::Output::Type::DUI_ERROR, outputList.cbegin()->type);
		ASSERT_EQUALS("unknown standard specified: 'c++77'", outputList.cbegin()->msg);
		outputList.clear();

		dui.std = "gnu++33";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(1, outputList.size());
		ASSERT_EQUALS(simplecpp::Output::Type::DUI_ERROR, outputList.cbegin()->type);
		ASSERT_EQUALS("unknown standard specified: 'gnu++33'", outputList.cbegin()->msg);
		outputList.clear();
	}
	{//stdEnum()
		ASSERT_EQUALS(simplecpp::cstd_t::C89, simplecpp::getCStd("c89"));
		ASSERT_EQUALS(simplecpp::cstd_t::C89, simplecpp::getCStd("c90"));
		ASSERT_EQUALS(simplecpp::cstd_t::C11, simplecpp::getCStd("iso9899:2011"));
		ASSERT_EQUALS(simplecpp::cstd_t::C23, simplecpp::getCStd("gnu23"));
		ASSERT_EQUALS(simplecpp::cstd_t::CUnknown, simplecpp::getCStd("gnu77"));
		ASSERT_EQUALS(simplecpp::cstd_t::CUnknown, simplecpp::getCStd("c++11"));

		ASSERT_EQUALS(simplecpp::cppstd_t::CPP03, simplecpp::getCppStd("c++03"));
		ASSERT_EQUALS(simplecpp::cppstd_t::CPP03, simplecpp::getCppStd("c++98"));
		ASSERT_EQUALS(simplecpp::cppstd_t::CPP17, simplecpp::getCppStd("c++1z"));
		ASSERT_EQUALS(simplecpp::cppstd_t::CPP26, simplecpp::getCppStd("gnu++26"));
		ASSERT_EQUALS(simplecpp::cppstd_t::CPPUnknown, simplecpp::getCppStd("gnu++77"));
		ASSERT_EQUALS(simplecpp::cppstd_t::CPPUnknown, simplecpp::getCppStd("c11"));
	}
	{//stdValid()
		const char code[] = "";
		simplecpp::DUI dui;
		simplecpp::OutputList outputList;

		dui.std = "c89";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(0, outputList.size());
		outputList.clear();

		dui.std = "gnu23";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(0, outputList.size());
		outputList.clear();

		dui.std = "c++03";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(0, outputList.size());
		outputList.clear();

		dui.std = "gnu++26";
		ASSERT_EQUALS("", preprocess(code, dui, &outputList));
		ASSERT_EQUALS(0, outputList.size());
		outputList.clear();
	}
	{//token()
		// name
		ASSERT_TOKEN("n", true, false, false);
		ASSERT_TOKEN("name", true, false, false);
		ASSERT_TOKEN("name_1", true, false, false);
		ASSERT_TOKEN("name2", true, false, false);
		ASSERT_TOKEN("name$", true, false, false);

		// character literal
		ASSERT_TOKEN("'n'", false, false, false);
		ASSERT_TOKEN("'\\''", false, false, false);
		ASSERT_TOKEN("'\\u0012'", false, false, false);
		ASSERT_TOKEN("'\\xff'", false, false, false);
		ASSERT_TOKEN("u8'\\u0012'", false, false, false);
		ASSERT_TOKEN("u'\\u0012'", false, false, false);
		ASSERT_TOKEN("L'\\u0012'", false, false, false);
		ASSERT_TOKEN("U'\\u0012'", false, false, false);

		// include
		ASSERT_TOKEN("<include>", false, false, false);

		// comment
		ASSERT_TOKEN("/*comment*/", false, false, true);
		ASSERT_TOKEN("// TODO", false, false, true);

		// string literal
		ASSERT_TOKEN("\"literal\"", false, false, false);

		// op
		ASSERT_TOKEN_OP("<", false, false, false, '<');
		ASSERT_TOKEN_OP(">", false, false, false, '>');
		ASSERT_TOKEN_OP("(", false, false, false, '(');
		ASSERT_TOKEN_OP(")", false, false, false, ')');

		// number
		ASSERT_TOKEN("2", false, true, false);
		ASSERT_TOKEN("22", false, true, false);
		ASSERT_TOKEN("-2", false, true, false);
		ASSERT_TOKEN("-22", false, true, false);
		ASSERT_TOKEN("+2", false, true, false);
		ASSERT_TOKEN("+22", false, true, false);
	}
	{//preprocess_files()
		{
			const char code[] = "#define A";
			std::vector<std::string> files;

			const simplecpp::TokenList tokens = makeTokenList(code, files);
			ASSERT_EQUALS(1, files.size());
			ASSERT_EQUALS("", *files.cbegin());

			simplecpp::TokenList tokens2(files);
			ASSERT_EQUALS(1, files.size());
			ASSERT_EQUALS("", *files.cbegin());

			std::map<std::string, simplecpp::TokenList*> filedata;
			simplecpp::preprocess(tokens2, tokens, files, filedata, simplecpp::DUI(), nullptr);
			ASSERT_EQUALS(1, files.size());
			ASSERT_EQUALS("", *files.cbegin());
		}
		{
			const char code[] = "#define A";
			std::vector<std::string> files;

			const simplecpp::TokenList tokens = makeTokenList(code, files, "test.cpp");
			ASSERT_EQUALS(1, files.size());
			ASSERT_EQUALS("test.cpp", *files.cbegin());

			simplecpp::TokenList tokens2(files);
			ASSERT_EQUALS(1, files.size());
			ASSERT_EQUALS("test.cpp", *files.cbegin());

			std::map<std::string, simplecpp::TokenList*> filedata;
			simplecpp::preprocess(tokens2, tokens, files, filedata, simplecpp::DUI(), nullptr);
			ASSERT_EQUALS(1, files.size());
			ASSERT_EQUALS("test.cpp", *files.cbegin());
		}
	}
	{//fuzz_crash()
		{
			const char code[] = "#define n __VA_OPT__(u\n"
				"n\n";
			(void)preprocess(code, simplecpp::DUI()); // do not crash
		}
	}
	//return (numberOfFailedAssertions > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
	if(numberOfFailedAssertions > 0)
		CurrentStatus = 0;
	{
		SString temp_buf;
		SString dump_buf;
		SFsPath ps(__FILE__); // Мы находимся в каталоге \\papyrus\src\pptest\unit
		ps.Dir.SetLastSlash().Cat("..\\..\\include");
		ps.Nam = "ppdefs";
		ps.Ext = "h";
		SString input_h_file_name;
		ps.Merge(input_h_file_name);
		simplecpp::DUI dui;
		dui.includes.push_back(input_h_file_name.cptr());
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
		dui.includePaths.push_back(temp_buf.RmvLastSlash().cptr());
		dui.removeComments = true;
		//
		// simplecpp.exe -I d:\papyrus\src\include -D _MSC_VER -D _MSC_FULL_VER -D _WIN32 -D _M_IX86 a.cpp   > a
		// 
		dui.defines.push_back("_MSC_VER");
		dui.defines.push_back("_MSC_FULL_VER");
		dui.defines.push_back("_WIN32");
		dui.defines.push_back("_M_IX86");
		SString code_buf;
		{
			simplecpp::PreprocessBlock pblk;
			pblk.Flags |= (simplecpp::PreprocessBlock::fOutputTokenList|simplecpp::PreprocessBlock::fMsgList);
			simplecpp::TokenList base_src_token_list = pblk.MakeSourceTokenList("#include <ppdefs.h>\n", "test_buf.h");
			simplecpp::Preprocess(pblk, dui, base_src_token_list);
			std::string dump = pblk.OutputTokenList.stringify();
			dump_buf.Z().CatN(dump.data(), dump.length());

			static const SIntToSymbTabEntry defined_symbols[] = {
				{ CTL_CSPANEL_CSESSCLOSE, "CTL_CSPANEL_CSESSCLOSE" },
				{ DLG_EXTGSEL, "DLG_EXTGSEL" },
				{ CTLSEL_REPLPRSN_KIND2, "CTLSEL_REPLPRSN_KIND2" },
				{ PPCFGOBJ_SUPPLDEAL, "PPCFGOBJ_SUPPLDEAL" },
				{ cmCurAmtGrpSetupCurrencyCombo, "cmCurAmtGrpSetupCurrencyCombo" }
			};
			{
				for(uint i = 0; i < SIZEOFARRAY(defined_symbols); i++) {
					const SIntToSymbTabEntry & r_item = defined_symbols[i];
					simplecpp::TokenList src_token_list = pblk.MakeSourceTokenList(r_item.P_Symb, "test_buf.h");
					const simplecpp::Token * tmp = src_token_list.cfront();
					simplecpp::TokenList result_token_list(pblk.FileList);
					if(pblk.PreprocessToken(result_token_list, &tmp)) {
						result_token_list.constFold();
						dump = result_token_list.stringify();
						dump_buf.Z().CatN(dump.data(), dump.length());
						const int value = dump_buf.ToLong();
						SLCHECK_EQ(value, r_item.Id);
					}
				}
			}
		}
	}
	return CurrentStatus;
}
