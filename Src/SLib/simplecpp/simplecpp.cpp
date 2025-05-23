// SIMPLE.CPP
// simplecpp - A simple and high-fidelity C/C++ preprocessor library
// Copyright (C) 2016-2023 simplecpp team
// @codepage UTF-8
// For this module compiler option /EHsc must be set (Enable C++ exceptions) and precompiled headers must be off
//
#include <slib-internal.h>
#pragma hdrstop

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
	#define SIMPLECPP_WINDOWS
	#define NOMINMAX
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstddef> // IWYU pragma: keep
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iosfwd>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#if __cplusplus >= 201103L
#ifdef SIMPLECPP_WINDOWS
#include <mutex>
#endif
#include <unordered_map>
#endif
#include <utility>
#include <vector>
#include "simplecpp.h"
/* @sobolev
#ifdef SIMPLECPP_WINDOWS
	#include <windows.h>
	#undef ERROR
#endif*/
//#if (__cplusplus < 201103L) && !defined(__APPLE__)
	//#define nullptr NULL
//#endif

static bool isHex(const std::string & s) { return s.size()>2 && (s.compare(0, 2, "0x")==0 || s.compare(0, 2, "0X")==0); }
static bool isOct(const std::string & s) { return s.size()>1 && (s[0]=='0') && (s[1] >= '0') && (s[1] < '8'); }

// TODO: added an undercore since this conflicts with a function of the same name in utils.h from Cppcheck source when building Cppcheck with MSBuild
static bool isStringLiteral_(const std::string &s) { return s.size() > 1 && (s[0]=='\"') && (*s.rbegin()=='\"'); }

// TODO: added an undercore since this conflicts with a function of the same name in utils.h from Cppcheck source when building Cppcheck with MSBuild
static bool isCharLiteral_(const std::string &s)
{
	// char literal patterns can include 'a', '\t', '\000', '\xff', 'abcd', and maybe ''
	// This only checks for the surrounding '' but doesn't parse the content.
	return s.size() > 1 && (s[0]=='\'') && (*s.rbegin()=='\'');
}

static const simplecpp::TokenString DEFINE("define");
static const simplecpp::TokenString UNDEF_("undef");
static const simplecpp::TokenString INCLUDE("include");
static const simplecpp::TokenString ERROR_("error");
static const simplecpp::TokenString WARNING("warning");
static const simplecpp::TokenString IF("if");
static const simplecpp::TokenString IFDEF("ifdef");
static const simplecpp::TokenString IFNDEF("ifndef");
static const simplecpp::TokenString DEFINED("defined");
static const simplecpp::TokenString ELSE("else");
static const simplecpp::TokenString ELIF("elif");
static const simplecpp::TokenString ENDIF("endif");
static const simplecpp::TokenString PRAGMA("pragma");
static const simplecpp::TokenString ONCE("once");
static const simplecpp::TokenString HAS_INCLUDE("__has_include");

template <class T> static std::string toString(T t)
{
	// NOLINTNEXTLINE(misc-const-correctness) - false positive
	std::ostringstream ostr;
	ostr << t;
	return ostr.str();
}

#ifdef SIMPLECPP_DEBUG_MACRO_EXPANSION
	static std::string locstring(const simplecpp::Location &loc)
	{
		std::ostringstream ostr;
		ostr << '[' << loc.file() << ':' << loc.line << ':' << loc.col << ']';
		return ostr.str();
	}
#endif

static int64 stringToLL(const std::string &s)
{
	int64 ret;
	const bool hex = isHex(s);
	const bool oct = isOct(s);
	std::istringstream istr(hex ? s.substr(2) : oct ? s.substr(1) : s);
	if(hex)
		istr >> std::hex;
	else if(oct)
		istr >> std::oct;
	istr >> ret;
	return ret;
}

static uint64 stringToULL(const std::string &s)
{
	uint64 ret;
	const bool hex = isHex(s);
	const bool oct = isOct(s);
	std::istringstream istr(hex ? s.substr(2) : oct ? s.substr(1) : s);
	if(hex)
		istr >> std::hex;
	else if(oct)
		istr >> std::oct;
	istr >> ret;
	return ret;
}

static bool endsWith(const std::string &s, const std::string &e)
{
	return (s.size() >= e.size()) && std::equal(e.rbegin(), e.rend(), s.rbegin());
}

static bool sameline(const simplecpp::Token * pTok1, const simplecpp::Token * pTok2)
{
	return (pTok1 && pTok2 && pTok1->location.sameline(pTok2->location));
}

static bool isAlternativeBinaryOp(const simplecpp::Token * tok, const std::string &alt)
{
	return (tok->name && tok->str() == alt && tok->previous && tok->next &&
	       (tok->previous->number || tok->previous->name || tok->previous->op == ')') && (tok->next->number || tok->next->name || tok->next->op == '('));
}

static bool isAlternativeUnaryOp(const simplecpp::Token * tok, const std::string &alt)
{
	return ((tok->name && tok->str() == alt) && (!tok->previous || tok->previous->op == '(') && (tok->next && (tok->next->name || tok->next->number)));
}

static std::string replaceAll(std::string s, const std::string& from, const std::string& to)
{
	for(size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size()))
		s.replace(pos, from.size(), to);
	return s;
}

const std::string simplecpp::Location::emptyFileName;

void simplecpp::Location::adjust(const std::string &str)
{
	if(strpbrk(str.c_str(), "\r\n") == nullptr) {
		col += str.size();
		return;
	}
	for(std::size_t i = 0U; i < str.size(); ++i) {
		col++;
		if(str[i] == '\n' || str[i] == '\r') {
			col = 1;
			line++;
			if(str[i] == '\r' && (i+1)<str.size() && str[i+1]=='\n')
				++i;
		}
	}
}

bool simplecpp::Token::isOneOf(const char ops[]) const { return (op != '\0') && (std::strchr(ops, op) != nullptr); }
bool simplecpp::Token::startsWithOneOf(const char c[]) const { return std::strchr(c, string[0]) != nullptr; }
bool simplecpp::Token::endsWithOneOf(const char c[]) const { return std::strchr(c, string[string.size() - 1U]) != nullptr; }

const simplecpp::Token * simplecpp::Token::previousSkipComments() const 
{
	const Token * tok = this->previous;
	while(tok && tok->comment)
		tok = tok->previous;
	return tok;
}

const simplecpp::Token * simplecpp::Token::nextSkipComments() const 
{
	const Token * tok = this->next;
	while(tok && tok->comment)
		tok = tok->next;
	return tok;
}

void simplecpp::Token::setExpandedFrom(const Token * tok, const Macro* m) 
{
	mExpandedFrom = tok->mExpandedFrom;
	mExpandedFrom.insert(m);
	if(tok->whitespaceahead)
		whitespaceahead = true;
}


void simplecpp::Token::printAll() const
{
	const Token * tok = this;
	while(tok->previous)
		tok = tok->previous;
	for(; tok; tok = tok->next) {
		if(tok->previous) {
			std::cout << (sameline(tok, tok->previous) ? ' ' : '\n');
		}
		std::cout << tok->str();
	}
	std::cout << std::endl;
}

void simplecpp::Token::printOut() const
{
	for(const Token * tok = this; tok; tok = tok->next) {
		if(tok != this) {
			std::cout << (sameline(tok, tok->previous) ? ' ' : '\n');
		}
		std::cout << tok->str();
	}
	std::cout << std::endl;
}

// cppcheck-suppress noConstructor - we call init() in the inherited to initialize the private members
class simplecpp::TokenList::Stream {
public:
	virtual ~Stream() {}
	virtual int get() = 0;
	virtual int peek() = 0;
	virtual void unget() = 0;
	virtual bool good() = 0;
	uchar readChar() 
	{
		uchar ch = static_cast<uchar>(get());
		// For UTF-16 encoded files the BOM is 0xfeff/0xfffe. If the
		// character is non-ASCII character then replace it with 0xff
		if(isUtf16) {
			const uchar ch2 = static_cast<uchar>(get());
			const int ch16 = makeUtf16Char(ch, ch2);
			ch = static_cast<uchar>(((ch16 >= 0x80) ? 0xff : ch16));
		}

		// Handling of newlines..
		if(ch == '\r') {
			ch = '\n';

			int ch2 = get();
			if(isUtf16) {
				const int c2 = get();
				ch2 = makeUtf16Char(ch2, c2);
			}

			if(ch2 != '\n')
				ungetChar();
		}

		return ch;
	}
	uchar peekChar() 
	{
		uchar ch = static_cast<uchar>(peek());
		// For UTF-16 encoded files the BOM is 0xfeff/0xfffe. If the
		// character is non-ASCII character then replace it with 0xff
		if(isUtf16) {
			(void)get();
			const uchar ch2 = static_cast<uchar>(peek());
			unget();
			const int ch16 = makeUtf16Char(ch, ch2);
			ch = static_cast<uchar>(((ch16 >= 0x80) ? 0xff : ch16));
		}
		// Handling of newlines..
		if(ch == '\r')
			ch = '\n';
		return ch;
	}
	void ungetChar() 
	{
		unget();
		if(isUtf16)
			unget();
	}
protected:
	void init() {
		// initialize since we use peek() in getAndSkipBOM()
		isUtf16 = false;
		bom = getAndSkipBOM();
		isUtf16 = (bom == 0xfeff || bom == 0xfffe);
	}

private:
	inline int makeUtf16Char(const uchar ch, const uchar ch2) const {
		return (bom == 0xfeff) ? (ch<<8 | ch2) : (ch2<<8 | ch);
	}

	unsigned short getAndSkipBOM() {
		const int ch1 = peek();

		// The UTF-16 BOM is 0xfffe or 0xfeff.
		if(ch1 >= 0xfe) {
			(void)get();
			const unsigned short byte = (static_cast<uchar>(ch1) << 8);
			if(peek() >= 0xfe)
				return byte | static_cast<uchar>(get());
			unget();
			return 0;
		}

		// Skip UTF-8 BOM 0xefbbbf
		if(ch1 == 0xef) {
			(void)get();
			if(peek() == 0xbb) {
				(void)get();
				if(peek() == 0xbf) {
					(void)get();
					return 0;
				}
				unget();
			}
			unget();
		}

		return 0;
	}

	unsigned short bom;
protected:
	bool isUtf16;
};

class StdIStream : public simplecpp::TokenList::Stream {
public:
	// cppcheck-suppress uninitDerivedMemberVar - we call Stream::init() to initialize the private members
	explicit StdIStream(std::istream &istr)
		: istr(istr) {
		assert(istr.good());
		init();
	}

	virtual int get() override {
		return istr.get();
	}

	virtual int peek() override {
		return istr.peek();
	}

	virtual void unget() override {
		istr.unget();
	}

	virtual bool good() override {
		return istr.good();
	}

private:
	std::istream &istr;
};

class StdCharBufStream : public simplecpp::TokenList::Stream {
public:
	// cppcheck-suppress uninitDerivedMemberVar - we call Stream::init() to initialize the private members
	StdCharBufStream(const uchar* str, std::size_t size)
		: str(str)
		, size(size)
		, pos(0)
		, lastStatus(0) {
		init();
	}

	virtual int get() override {
		if(pos >= size)
			return lastStatus = EOF;
		return str[pos++];
	}

	virtual int peek() override {
		if(pos >= size)
			return lastStatus = EOF;
		return str[pos];
	}

	virtual void unget() override {
		--pos;
	}

	virtual bool good() override {
		return lastStatus != EOF;
	}

private:
	const uchar * str;
	const std::size_t size;
	std::size_t pos;
	int lastStatus;
};

class FileStream : public simplecpp::TokenList::Stream {
public:
	// cppcheck-suppress uninitDerivedMemberVar - we call Stream::init() to initialize the private members
	explicit FileStream(const std::string &filename, std::vector<std::string> &files) : file(fopen(filename.c_str(), "rb")), lastCh(0), lastStatus(0) 
	{
		if(!file) {
			files.push_back(filename);
			throw simplecpp::Output(files, simplecpp::Output::FILE_NOT_FOUND, "File is missing: " + filename);
		}
		init();
	}
	~FileStream() override 
	{
		fclose(file);
		file = nullptr;
	}
	virtual int get() override 
	{
		lastStatus = lastCh = fgetc(file);
		return lastCh;
	}
	virtual int peek() override 
	{
		// keep lastCh intact
		const int ch = fgetc(file);
		unget_internal(ch);
		return ch;
	}
	virtual void unget() override { unget_internal(lastCh); }
	virtual bool good() override { return lastStatus != EOF; }
private:
	void unget_internal(int ch) 
	{
		if(isUtf16) {
			// TODO: use ungetc() as well
			// UTF-16 has subsequent unget() calls
			fseek(file, -1, SEEK_CUR);
		}
		else
			ungetc(ch, file);
	}
	FileStream(const FileStream&);
	FileStream &operator=(const FileStream&);

	FILE * file;
	int lastCh;
	int lastStatus;
};

simplecpp::TokenList::TokenList(std::vector<std::string> &filenames) : frontToken(nullptr), backToken(nullptr), files(filenames) {}

simplecpp::TokenList::TokenList(std::istream &istr, std::vector<std::string> &filenames, const std::string &filename, OutputList * outputList)
	: frontToken(nullptr), backToken(nullptr), files(filenames)
{
	StdIStream stream(istr);
	readfile(stream, filename, outputList);
}

simplecpp::TokenList::TokenList(const uchar* data, std::size_t size, std::vector<std::string> &filenames, const std::string &filename, OutputList * outputList)
	: frontToken(nullptr), backToken(nullptr), files(filenames)
{
	StdCharBufStream stream(data, size);
	readfile(stream, filename, outputList);
}

simplecpp::TokenList::TokenList(const char* data, std::size_t size, std::vector<std::string> &filenames, const std::string &filename, OutputList * outputList)
	: frontToken(nullptr), backToken(nullptr), files(filenames)
{
	StdCharBufStream stream(reinterpret_cast<const uchar*>(data), size);
	readfile(stream, filename, outputList);
}

simplecpp::TokenList::TokenList(const std::string &filename, std::vector<std::string> &filenames, OutputList * outputList)
	: frontToken(nullptr), backToken(nullptr), files(filenames)
{
	try {
		FileStream stream(filename, filenames);
		readfile(stream, filename, outputList);
	} catch(const simplecpp::Output & e) { // TODO handle extra type of errors
		outputList->push_back(e);
	}
}

simplecpp::TokenList::TokenList(const TokenList &other) : frontToken(nullptr), backToken(nullptr), files(other.files)
{
	*this = other;
}

#if __cplusplus >= 201103L
simplecpp::TokenList::TokenList(TokenList &&other) : frontToken(nullptr), backToken(nullptr), files(other.files)
{
	*this = std::move(other);
}
#endif

simplecpp::TokenList::~TokenList()
{
	clear();
}

simplecpp::TokenList & simplecpp::TokenList::operator=(const TokenList &other)
{
	if(this != &other) {
		clear();
		files = other.files;
		for(const Token * tok = other.cfront(); tok; tok = tok->next)
			push_back(new Token(*tok));
		sizeOfType = other.sizeOfType;
	}
	return *this;
}

#if __cplusplus >= 201103L
	simplecpp::TokenList &simplecpp::TokenList::operator=(TokenList &&other)
	{
		if(this != &other) {
			clear();
			frontToken = other.frontToken;
			other.frontToken = nullptr;
			backToken = other.backToken;
			other.backToken = nullptr;
			files = other.files;
			sizeOfType = std::move(other.sizeOfType);
		}
		return *this;
	}
#endif

void simplecpp::TokenList::clear()
{
	backToken = nullptr;
	while(frontToken) {
		Token * const next = frontToken->next;
		delete frontToken;
		frontToken = next;
	}
	sizeOfType.clear();
}

void simplecpp::TokenList::push_back(Token * tok)
{
	if(!frontToken)
		frontToken = tok;
	else
		backToken->next = tok;
	tok->previous = backToken;
	backToken = tok;
}

void simplecpp::TokenList::dump() const
{
	std::cout << stringify() << std::endl;
}

std::string simplecpp::TokenList::stringify() const
{
	std::ostringstream ret;
	Location loc(files);
	for(const Token * tok = cfront(); tok; tok = tok->next) {
		if(tok->location.line < loc.line || tok->location.fileIndex != loc.fileIndex) {
			ret << "\n#line " << tok->location.line << " \"" << tok->location.file() << "\"\n";
			loc = tok->location;
		}
		while(tok->location.line > loc.line) {
			ret << '\n';
			loc.line++;
		}
		if(sameline(tok->previous, tok))
			ret << ' ';
		ret << tok->str();
		loc.adjust(tok->str());
	}
	return ret.str();
}

static bool isNameChar(uchar ch) { return std::isalnum(ch) || ch == '_' || ch == '$'; }

static std::string escapeString(const std::string &str)
{
	std::ostringstream ostr;
	ostr << '\"';
	for(std::size_t i = 1U; i < str.size() - 1; ++i) {
		const char c = str[i];
		if(c == '\\' || c == '\"' || c == '\'')
			ostr << '\\';
		ostr << c;
	}
	ostr << '\"';
	return ostr.str();
}

static int MakeOutputMessage(simplecpp::OutputList * pOutputList, const std::vector<std::string> & rFiles, simplecpp::Output::Type typ, const simplecpp::Location * pLoc, const char * pMsg)
{
	int    result = 1;
	if(pOutputList) {
		simplecpp::Output err(rFiles);
		err.type = typ;
		if(pLoc)
			err.location = *pLoc;
		if(pMsg)
			err.msg = pMsg;
		pOutputList->push_back(err);
	}
	else
		result = -1;
	return result;
}

static void portabilityBackslash(simplecpp::OutputList * pOutputList, const std::vector<std::string> & files, const simplecpp::Location & location)
{
	MakeOutputMessage(pOutputList, files, simplecpp::Output::PORTABILITY_BACKSLASH, &location, "Combination 'backslash space newline' is not portable."); // @sobolev
}

static bool isStringLiteralPrefix(const std::string &str) { return oneof9(str, "u", "U", "L", "u8", "R", "uR", "UR", "LR", "u8R"); }

void simplecpp::TokenList::lineDirective(uint fileIndex, uint line, Location * location)
{
	if(fileIndex != location->fileIndex || line >= location->line) {
		location->fileIndex = fileIndex;
		location->line = line;
	}
	else if((line + 2) >= location->line) {
		location->line = line;
		while(cback()->op != '#')
			deleteToken(back());
		deleteToken(back());
	}
}

static const std::string COMMENT_END("*/");

void simplecpp::TokenList::readfile(Stream &stream, const std::string &filename, OutputList * outputList)
{
	std::stack<simplecpp::Location> loc;
	uint multiline = 0U;
	const Token * oldLastToken = nullptr;
	Location location(files);
	location.fileIndex = fileIndex(filename);
	location.line = 1U;
	location.col  = 1U;
	while(stream.good()) {
		uchar ch = stream.readChar();
		if(!stream.good())
			break;
		if(ch >= 0x80) {
			if(outputList) {
				SString msg_;
				msg_.Cat("The code contains unhandled character(s) (character code=").Cat(ch/*здесь должен быть именно код символа, а не он сам*/).
					CatChar(')').Dot().Space().Cat("Neither unicode nor extended ascii is supported.");
				MakeOutputMessage(outputList, files, simplecpp::Output::UNHANDLED_CHAR_ERROR, &location, msg_); // @sobolev
			}
			clear();
			return;
		}
		if(ch == '\n') {
			if(cback() && cback()->op == '\\') {
				if(location.col > cback()->location.col + 1U)
					portabilityBackslash(outputList, files, cback()->location);
				++multiline;
				deleteToken(back());
			}
			else {
				location.line += multiline + 1;
				multiline = 0U;
			}
			if(!multiline)
				location.col = 1;
			if(oldLastToken != cback()) {
				oldLastToken = cback();
				if(!isLastLinePreprocessor())
					continue;
				const std::string lastline(lastLine());
				if(lastline == "# file %str%") {
					const Token * strtok = cback();
					while(strtok->comment)
						strtok = strtok->previous;
					loc.push(location);
					location.fileIndex = fileIndex(strtok->str().substr(1U, strtok->str().size() - 2U));
					location.line = 1U;
				}
				else if(lastline == "# line %num%") {
					const Token * numtok = cback();
					while(numtok->comment)
						numtok = numtok->previous;
					lineDirective(location.fileIndex, std::atol(numtok->str().c_str()), &location);
				}
				else if(lastline == "# %num% %str%" || lastline == "# line %num% %str%") {
					const Token * strtok = cback();
					while(strtok->comment)
						strtok = strtok->previous;
					const Token * numtok = strtok->previous;
					while(numtok->comment)
						numtok = numtok->previous;
					lineDirective(fileIndex(replaceAll(strtok->str().substr(1U, strtok->str().size() - 2U), "\\\\", "\\")),
					    std::atol(numtok->str().c_str()), &location);
				}
				// #endfile
				else if(lastline == "# endfile" && !loc.empty()) {
					location = loc.top();
					loc.pop();
				}
			}
			continue;
		}
		if(ch <= ' ') {
			location.col++;
			continue;
		}
		TokenString currentToken;
		if(cback() && cback()->location.line == location.line && cback()->previous && cback()->previous->op == '#') {
			const Token* const llTok = lastLineTok();
			if(llTok && llTok->op == '#' && llTok->next && (llTok->next->str() == "error" || llTok->next->str() == "warning")) {
				char prev = ' ';
				while(stream.good() && (prev == '\\' || (ch != '\r' && ch != '\n'))) {
					currentToken += ch;
					prev = ch;
					ch = stream.readChar();
				}
				stream.ungetChar();
				push_back(new Token(currentToken, location));
				location.adjust(currentToken);
				continue;
			}
		}
		// number or name
		if(isNameChar(ch)) {
			const bool num = std::isdigit(ch);
			while(stream.good() && isNameChar(ch)) {
				currentToken += ch;
				ch = stream.readChar();
				if(num && ch=='\'' && isNameChar(stream.peekChar()))
					ch = stream.readChar();
			}
			stream.ungetChar();
		}
		// comment
		else if(ch == '/' && stream.peekChar() == '/') {
			while(stream.good() && ch != '\r' && ch != '\n') {
				currentToken += ch;
				ch = stream.readChar();
			}
			const std::string::size_type pos = currentToken.find_last_not_of(" \t");
			if(pos < currentToken.size() - 1U && currentToken[pos] == '\\')
				portabilityBackslash(outputList, files, location);
			if(currentToken[currentToken.size() - 1U] == '\\') {
				++multiline;
				currentToken.erase(currentToken.size() - 1U);
			}
			else {
				stream.ungetChar();
			}
		}

		// comment
		else if(ch == '/' && stream.peekChar() == '*') {
			currentToken = "/*";
			(void)stream.readChar();
			ch = stream.readChar();
			while(stream.good()) {
				currentToken += ch;
				if(currentToken.size() >= 4U && endsWith(currentToken, COMMENT_END))
					break;
				ch = stream.readChar();
			}
			// multiline..

			std::string::size_type pos = 0;
			while((pos = currentToken.find("\\\n", pos)) != std::string::npos) {
				currentToken.erase(pos, 2);
				++multiline;
			}
			if(multiline || isLastLinePreprocessor()) {
				pos = 0;
				while((pos = currentToken.find('\n', pos)) != std::string::npos) {
					currentToken.erase(pos, 1);
					++multiline;
				}
			}
		}

		// string / char literal
		else if(ch == '\"' || ch == '\'') {
			std::string prefix;
			if(cback() && cback()->name && isStringLiteralPrefix(cback()->str()) &&
			    ((cback()->location.col + cback()->str().size()) == location.col) &&
			    (cback()->location.line == location.line)) {
				prefix = cback()->str();
			}
			// C++11 raw string literal
			if(ch == '\"' && !prefix.empty() && *cback()->str().rbegin() == 'R') {
				std::string delim;
				currentToken = ch;
				prefix.resize(prefix.size() - 1);
				ch = stream.readChar();
				while(stream.good() && ch != '(' && ch != '\n') {
					delim += ch;
					ch = stream.readChar();
				}
				if(!stream.good() || ch == '\n') {
					if(outputList) {
						Output err(files);
						err.type = Output::SYNTAX_ERROR;
						err.location = location;
						err.msg = "Invalid newline in raw string delimiter.";
						outputList->push_back(err);
					}
					return;
				}
				const std::string endOfRawString(')' + delim + currentToken);
				while(stream.good() && !(endsWith(currentToken, endOfRawString) && currentToken.size() > 1))
					currentToken += stream.readChar();
				if(!endsWith(currentToken, endOfRawString)) {
					if(outputList) {
						Output err(files);
						err.type = Output::SYNTAX_ERROR;
						err.location = location;
						err.msg = "Raw string missing terminating delimiter.";
						outputList->push_back(err);
					}
					return;
				}
				currentToken.erase(currentToken.size() - endOfRawString.size(), endOfRawString.size() - 1U);
				currentToken = escapeString(currentToken);
				currentToken.insert(0, prefix);
				back()->setstr(currentToken);
				location.adjust(currentToken);
				if(currentToken.find_first_of("\r\n") == std::string::npos)
					location.col += 2 + 2 * delim.size();
				else
					location.col += 1 + delim.size();
				continue;
			}
			currentToken = readUntil(stream, location, ch, ch, outputList);
			if(currentToken.size() < 2U)
				return; // Error is reported by readUntil()
			std::string s = currentToken;
			std::string::size_type pos;
			int newlines = 0;
			while((pos = s.find_first_of("\r\n")) != std::string::npos) {
				s.erase(pos, 1);
				newlines++;
			}
			if(prefix.empty())
				push_back(new Token(s, location, std::isspace(stream.peekChar()))); // push string without newlines
			else
				back()->setstr(prefix + s);
			if(newlines > 0) {
				const Token * const llTok = lastLineTok();
				if(llTok && llTok->op == '#' && llTok->next && (llTok->next->str() == "define" || llTok->next->str() == "pragma") && llTok->next->next) {
					multiline += newlines;
					location.adjust(s);
					continue;
				}
			}
			location.adjust(currentToken);
			continue;
		}
		else {
			currentToken += ch;
		}
		if(*currentToken.begin() == '<') {
			const Token * const llTok = lastLineTok();
			if(llTok && llTok->op == '#' && llTok->next && llTok->next->str() == "include") {
				currentToken = readUntil(stream, location, '<', '>', outputList);
				if(currentToken.size() < 2U)
					return;
			}
		}
		push_back(new Token(currentToken, location, std::isspace(stream.peekChar())));
		if(multiline)
			location.col += currentToken.size();
		else
			location.adjust(currentToken);
	}
	combineOperators();
}

void simplecpp::TokenList::constFold()
{
	while(cfront()) {
		// goto last '('
		Token * tok = back();
		while(tok && tok->op != '(')
			tok = tok->previous;
		// no '(', goto first token
		if(!tok)
			tok = front();
		// Constant fold expression
		constFoldUnaryNotPosNeg(tok);
		constFoldMulDivRem(tok);
		constFoldAddSub(tok);
		constFoldShift(tok);
		constFoldComparison(tok);
		constFoldBitwise(tok);
		constFoldLogicalOp(tok);
		constFoldQuestionOp(&tok);
		// If there is no '(' we are done with the constant folding
		if(tok->op != '(')
			break;
		if(!tok->next || !tok->next->next || tok->next->next->op != ')')
			break;
		tok = tok->next;
		deleteToken(tok->previous);
		deleteToken(tok->next);
	}
}

static bool isFloatSuffix(const simplecpp::Token * tok)
{
	if(!tok || tok->str().size() != 1U)
		return false;
	const char c = std::tolower(tok->str()[0]);
	return c == 'f' || c == 'l';
}

void simplecpp::TokenList::combineOperators()
{
	std::stack<bool> executableScope;
	executableScope.push(false);
	for(Token * tok = front(); tok; tok = tok->next) {
		if(tok->op == '{') {
			if(executableScope.top()) {
				executableScope.push(true);
				continue;
			}
			const Token * prev = tok->previous;
			while(prev && prev->isOneOf(";{}()"))
				prev = prev->previous;
			executableScope.push(prev && prev->op == ')');
			continue;
		}
		if(tok->op == '}') {
			if(executableScope.size() > 1)
				executableScope.pop();
			continue;
		}

		if(tok->op == '.') {
			// ellipsis ...
			if(tok->next && tok->next->op == '.' && tok->next->location.col == (tok->location.col + 1) &&
			    tok->next->next && tok->next->next->op == '.' && tok->next->next->location.col == (tok->location.col + 2)) {
				tok->setstr("...");
				deleteToken(tok->next);
				deleteToken(tok->next);
				continue;
			}
			// float literals..
			if(tok->previous && tok->previous->number && sameline(tok->previous, tok) && tok->previous->str().find_first_of("._") == std::string::npos) {
				tok->setstr(tok->previous->str() + '.');
				deleteToken(tok->previous);
				if(sameline(tok, tok->next) && (isFloatSuffix(tok->next) || (tok->next && tok->next->startsWithOneOf("AaBbCcDdEeFfPp")))) {
					tok->setstr(tok->str() + tok->next->str());
					deleteToken(tok->next);
				}
			}
			if(tok->next && tok->next->number) {
				tok->setstr(tok->str() + tok->next->str());
				deleteToken(tok->next);
			}
		}
		// match: [0-9.]+E [+-] [0-9]+
		const char lastChar = tok->str()[tok->str().size() - 1];
		if(tok->number && !isOct(tok->str()) &&
		    ((!isHex(tok->str()) && (lastChar == 'E' || lastChar == 'e')) ||
		    (isHex(tok->str()) && (lastChar == 'P' || lastChar == 'p'))) &&
		    tok->next && tok->next->isOneOf("+-") && tok->next->next && tok->next->next->number) {
			tok->setstr(tok->str() + tok->next->op + tok->next->next->str());
			deleteToken(tok->next);
			deleteToken(tok->next);
		}

		if(tok->op == '\0' || !tok->next || tok->next->op == '\0')
			continue;
		if(!sameline(tok, tok->next))
			continue;
		if(tok->location.col + 1U != tok->next->location.col)
			continue;

		if(tok->next->op == '=' && tok->isOneOf("=!<>+-*/%&|^")) {
			if(tok->op == '&' && !executableScope.top()) {
				// don't combine &= if it is a anonymous reference parameter with default value:
				// void f(x&=2)
				int indentlevel = 0;
				const Token * start = tok;
				while(indentlevel >= 0 && start) {
					if(start->op == ')')
						++indentlevel;
					else if(start->op == '(')
						--indentlevel;
					else if(start->isOneOf(";{}"))
						break;
					start = start->previous;
				}
				if(indentlevel == -1 && start) {
					const Token * const ftok = start;
					bool isFuncDecl = ftok->name;
					while(isFuncDecl) {
						if(!start->name && start->str() != "::" && start->op != '*' && start->op != '&')
							isFuncDecl = false;
						if(!start->previous)
							break;
						if(start->previous->isOneOf(";{}:"))
							break;
						start = start->previous;
					}
					isFuncDecl &= start != ftok && start->name;
					if(isFuncDecl) {
						// TODO: we could loop through the parameters here and check if they are correct.
						continue;
					}
				}
			}
			tok->setstr(tok->str() + "=");
			deleteToken(tok->next);
		}
		else if((tok->op == '|' || tok->op == '&') && tok->op == tok->next->op) {
			tok->setstr(tok->str() + tok->next->str());
			deleteToken(tok->next);
		}
		else if(tok->op == ':' && tok->next->op == ':') {
			tok->setstr(tok->str() + tok->next->str());
			deleteToken(tok->next);
		}
		else if(tok->op == '-' && tok->next->op == '>') {
			tok->setstr(tok->str() + tok->next->str());
			deleteToken(tok->next);
		}
		else if((tok->op == '<' || tok->op == '>') && tok->op == tok->next->op) {
			tok->setstr(tok->str() + tok->next->str());
			deleteToken(tok->next);
			if(tok->next && tok->next->op == '=' && tok->next->next && tok->next->next->op != '=') {
				tok->setstr(tok->str() + tok->next->str());
				deleteToken(tok->next);
			}
		}
		else if((tok->op == '+' || tok->op == '-') && tok->op == tok->next->op) {
			if(tok->location.col + 1U != tok->next->location.col)
				continue;
			if(tok->previous && tok->previous->number)
				continue;
			if(tok->next->next && tok->next->next->number)
				continue;
			tok->setstr(tok->str() + tok->next->str());
			deleteToken(tok->next);
		}
	}
}

static const std::string COMPL("compl");
static const std::string NOT("not");
void simplecpp::TokenList::constFoldUnaryNotPosNeg(simplecpp::Token * tok)
{
	for(; tok && tok->op != ')'; tok = tok->next) {
		// "not" might be !
		if(isAlternativeUnaryOp(tok, NOT))
			tok->op = '!';
		// "compl" might be ~
		else if(isAlternativeUnaryOp(tok, COMPL))
			tok->op = '~';

		if(tok->op == '!' && tok->next && tok->next->number) {
			tok->setstr(tok->next->str() == "0" ? "1" : "0");
			deleteToken(tok->next);
		}
		else if(tok->op == '~' && tok->next && tok->next->number) {
			tok->setstr(toString(~stringToLL(tok->next->str())));
			deleteToken(tok->next);
		}
		else {
			if(tok->previous && (tok->previous->number || tok->previous->name))
				continue;
			if(!tok->next || !tok->next->number)
				continue;
			switch(tok->op) {
				case '+':
				    tok->setstr(tok->next->str());
				    deleteToken(tok->next);
				    break;
				case '-':
				    tok->setstr(tok->op + tok->next->str());
				    deleteToken(tok->next);
				    break;
			}
		}
	}
}

void simplecpp::TokenList::constFoldMulDivRem(Token * tok)
{
	for(; tok && tok->op != ')'; tok = tok->next) {
		if(!tok->previous || !tok->previous->number)
			continue;
		if(!tok->next || !tok->next->number)
			continue;
		int64 result;
		if(tok->op == '*')
			result = (stringToLL(tok->previous->str()) * stringToLL(tok->next->str()));
		else if(tok->op == '/' || tok->op == '%') {
			const int64 rhs = stringToLL(tok->next->str());
			if(rhs == 0)
				throw std::overflow_error("division/modulo by zero");
			const int64 lhs = stringToLL(tok->previous->str());
			if(rhs == -1 && lhs == std::numeric_limits<int64>::min())
				throw std::overflow_error("division overflow");
			if(tok->op == '/')
				result = (lhs / rhs);
			else
				result = (lhs % rhs);
		}
		else
			continue;
		tok = tok->previous;
		tok->setstr(toString(result));
		deleteToken(tok->next);
		deleteToken(tok->next);
	}
}

void simplecpp::TokenList::constFoldAddSub(Token * tok)
{
	for(; tok && tok->op != ')'; tok = tok->next) {
		if(!tok->previous || !tok->previous->number)
			continue;
		if(!tok->next || !tok->next->number)
			continue;
		int64 result;
		if(tok->op == '+')
			result = stringToLL(tok->previous->str()) + stringToLL(tok->next->str());
		else if(tok->op == '-')
			result = stringToLL(tok->previous->str()) - stringToLL(tok->next->str());
		else
			continue;
		tok = tok->previous;
		tok->setstr(toString(result));
		deleteToken(tok->next);
		deleteToken(tok->next);
	}
}

void simplecpp::TokenList::constFoldShift(Token * tok)
{
	for(; tok && tok->op != ')'; tok = tok->next) {
		if(!tok->previous || !tok->previous->number)
			continue;
		if(!tok->next || !tok->next->number)
			continue;
		int64 result;
		if(tok->str() == "<<")
			result = stringToLL(tok->previous->str()) << stringToLL(tok->next->str());
		else if(tok->str() == ">>")
			result = stringToLL(tok->previous->str()) >> stringToLL(tok->next->str());
		else
			continue;
		tok = tok->previous;
		tok->setstr(toString(result));
		deleteToken(tok->next);
		deleteToken(tok->next);
	}
}

static const std::string NOTEQ("not_eq");
void simplecpp::TokenList::constFoldComparison(Token * tok)
{
	for(; tok && tok->op != ')'; tok = tok->next) {
		if(isAlternativeBinaryOp(tok, NOTEQ))
			tok->setstr("!=");

		if(!tok->startsWithOneOf("<>=!"))
			continue;
		if(!tok->previous || !tok->previous->number)
			continue;
		if(!tok->next || !tok->next->number)
			continue;

		int result;
		if(tok->str() == "==")
			result = (stringToLL(tok->previous->str()) == stringToLL(tok->next->str()));
		else if(tok->str() == "!=")
			result = (stringToLL(tok->previous->str()) != stringToLL(tok->next->str()));
		else if(tok->str() == ">")
			result = (stringToLL(tok->previous->str()) > stringToLL(tok->next->str()));
		else if(tok->str() == ">=")
			result = (stringToLL(tok->previous->str()) >= stringToLL(tok->next->str()));
		else if(tok->str() == "<")
			result = (stringToLL(tok->previous->str()) < stringToLL(tok->next->str()));
		else if(tok->str() == "<=")
			result = (stringToLL(tok->previous->str()) <= stringToLL(tok->next->str()));
		else
			continue;

		tok = tok->previous;
		tok->setstr(toString(result));
		deleteToken(tok->next);
		deleteToken(tok->next);
	}
}

static const std::string BITAND("bitand");
static const std::string BITOR("bitor");
static const std::string XOR("xor");
void simplecpp::TokenList::constFoldBitwise(Token * tok)
{
	Token * const tok1 = tok;
	for(const char * op = "&^|"; *op; op++) {
		const std::string* alternativeOp;
		if(*op == '&')
			alternativeOp = &BITAND;
		else if(*op == '|')
			alternativeOp = &BITOR;
		else
			alternativeOp = &XOR;
		for(tok = tok1; tok && tok->op != ')'; tok = tok->next) {
			if(tok->op != *op && !isAlternativeBinaryOp(tok, *alternativeOp))
				continue;
			if(!tok->previous || !tok->previous->number)
				continue;
			if(!tok->next || !tok->next->number)
				continue;
			int64 result;
			if(*op == '&')
				result = (stringToLL(tok->previous->str()) & stringToLL(tok->next->str()));
			else if(*op == '^')
				result = (stringToLL(tok->previous->str()) ^ stringToLL(tok->next->str()));
			else /*if (*op == '|')*/
				result = (stringToLL(tok->previous->str()) | stringToLL(tok->next->str()));
			tok = tok->previous;
			tok->setstr(toString(result));
			deleteToken(tok->next);
			deleteToken(tok->next);
		}
	}
}

static const std::string AND("and");
static const std::string OR("or");
void simplecpp::TokenList::constFoldLogicalOp(Token * tok)
{
	for(; tok && tok->op != ')'; tok = tok->next) {
		if(tok->name) {
			if(isAlternativeBinaryOp(tok, AND))
				tok->setstr("&&");
			else if(isAlternativeBinaryOp(tok, OR))
				tok->setstr("||");
		}
		if(tok->str() != "&&" && tok->str() != "||")
			continue;
		if(!tok->previous || !tok->previous->number)
			continue;
		if(!tok->next || !tok->next->number)
			continue;

		int result;
		if(tok->str() == "||")
			result = (stringToLL(tok->previous->str()) || stringToLL(tok->next->str()));
		else /*if (tok->str() == "&&")*/
			result = (stringToLL(tok->previous->str()) && stringToLL(tok->next->str()));

		tok = tok->previous;
		tok->setstr(toString(result));
		deleteToken(tok->next);
		deleteToken(tok->next);
	}
}

void simplecpp::TokenList::constFoldQuestionOp(Token ** tok1)
{
	bool gotoTok1 = false;
	for(Token * tok = *tok1; tok && tok->op != ')'; tok = gotoTok1 ? *tok1 : tok->next) {
		gotoTok1 = false;
		if(tok->str() != "?")
			continue;
		if(!tok->previous || !tok->next || !tok->next->next) {
			throw std::runtime_error("invalid expression");
			break; // На случай, если exception не поддерживаются //
		}
		if(!tok->previous->number)
			continue;
		if(tok->next->next->op != ':')
			continue;
		Token * const condTok = tok->previous;
		Token * const trueTok = tok->next;
		Token * const falseTok = trueTok->next->next;
		if(!falseTok) {
			throw std::runtime_error("invalid expression");
			break; // На случай, если exception не поддерживаются //
		}
		else {
			if(condTok == *tok1)
				*tok1 = (condTok->str() != "0" ? trueTok : falseTok);
			deleteToken(condTok->next); // ?
			deleteToken(trueTok->next); // :
			deleteToken(condTok->str() == "0" ? trueTok : falseTok);
			deleteToken(condTok);
			gotoTok1 = true;
		}
	}
}

void simplecpp::TokenList::removeComments()
{
	Token * tok = frontToken;
	while(tok) {
		Token * const tok1 = tok;
		tok = tok->next;
		if(tok1->comment)
			deleteToken(tok1);
	}
}

void FASTCALL simplecpp::TokenList::deleteToken(Token * pTok) 
{
	if(pTok) {
		Token * const prev = pTok->previous;
		Token * const next = pTok->next;
		if(prev)
			prev->next = next;
		if(next)
			next->previous = prev;
		if(frontToken == pTok)
			frontToken = next;
		if(backToken == pTok)
			backToken = prev;
		delete pTok;
	}
}

void FASTCALL simplecpp::TokenList::takeTokens(TokenList & other) 
{
	if(other.frontToken) {
		if(!frontToken) {
			frontToken = other.frontToken;
		}
		else {
			backToken->next = other.frontToken;
			other.frontToken->previous = backToken;
		}
		backToken = other.backToken;
		other.frontToken = other.backToken = nullptr;
	}
}

std::string simplecpp::TokenList::readUntil(Stream &stream, const Location &location, const char start, const char end, OutputList * outputList)
{
	std::string ret;
	ret += start;
	bool backslash = false;
	char ch = 0;
	while(ch != end && ch != '\r' && ch != '\n' && stream.good()) {
		ch = stream.readChar();
		if(backslash && ch == '\n') {
			ch = 0;
			backslash = false;
			continue;
		}
		backslash = false;
		ret += ch;
		if(ch == '\\') {
			bool update_ch = false;
			char next = 0;
			do {
				next = stream.readChar();
				if(next == '\r' || next == '\n') {
					ret.erase(ret.size()-1U);
					backslash = (next == '\r');
					update_ch = false;
				}
				else if(next == '\\')
					update_ch = !update_ch;
				ret += next;
			} while(next == '\\');
			if(update_ch)
				ch = next;
		}
	}

	if(!stream.good() || ch != end) {
		clear();
		if(outputList) {
			Output err(files);
			err.type = Output::SYNTAX_ERROR;
			err.location = location;
			err.msg = std::string("No pair for character (") + start + "). Can't process file. File is either invalid or unicode, which is currently not supported.";
			outputList->push_back(err);
		}
		return "";
	}

	return ret;
}

std::string simplecpp::TokenList::lastLine(int maxsize) const
{
	std::string ret;
	int count = 0;
	for(const Token * tok = cback(); ; tok = tok->previous) {
		if(!sameline(tok, cback())) {
			break;
		}
		if(tok->comment)
			continue;
		if(++count > maxsize)
			return "";
		if(!ret.empty())
			ret += ' ';
		// add tokens in reverse for performance reasons
		if(tok->str()[0] == '\"')
			ret += "%rts%"; // %str%
		else if(tok->number)
			ret += "%mun%"; // %num%
		else {
			ret += tok->str();
			std::reverse(ret.end() - tok->str().length(), ret.end());
		}
	}
	std::reverse(ret.begin(), ret.end());
	return ret;
}

const simplecpp::Token* simplecpp::TokenList::lastLineTok(int maxsize) const
{
	const Token* prevTok = nullptr;
	int count = 0;
	for(const Token * tok = cback(); ; tok = tok->previous) {
		if(!sameline(tok, cback()))
			break;
		if(tok->comment)
			continue;
		if(++count > maxsize)
			return nullptr;
		prevTok = tok;
	}
	return prevTok;
}

bool simplecpp::TokenList::isLastLinePreprocessor(int maxsize) const
{
	const Token * const prevTok = lastLineTok(maxsize);
	return prevTok && prevTok->op == '#';
}

uint simplecpp::TokenList::fileIndex(const std::string &filename)
{
	for(uint i = 0; i < files.size(); ++i) {
		if(files[i] == filename)
			return i;
	}
	files.push_back(filename);
	return files.size() - 1U;
}

namespace simplecpp {

Macro::wrongNumberOfParameters::wrongNumberOfParameters(const Location &loc, const std::string &macroName) : 
	Error(loc, "Wrong number of parameters for macro \'" + macroName + "\'.") 
{
}

/*static*/std::string Macro::invalidHashHash::format(const std::string &macroName, const std::string &message) 
{
	return "Invalid ## usage when expanding \'" + macroName + "\': " + message;
}
		
Macro::invalidHashHash::invalidHashHash(const Location &loc, const std::string &macroName, const std::string &message) : Error(loc, format(macroName, message)) 
{
}

/*static*/Macro::invalidHashHash Macro::invalidHashHash::unexpectedToken(const Location &loc, const std::string &macroName, const Token * tokenA) 
{
	return invalidHashHash(loc, macroName, "Unexpected token '"+ tokenA->str()+"'");
}
		
/*static*/Macro::invalidHashHash Macro::invalidHashHash::cannotCombine(const Location &loc, const std::string &macroName, const Token * tokenA, const Token * tokenB) 
{
	return invalidHashHash(loc, macroName, "Combining '"+ tokenA->str()+ "' and '"+ tokenB->str() + "' yields an invalid token.");
}
		
/*static*/Macro::invalidHashHash Macro::invalidHashHash::unexpectedNewline(const Location &loc, const std::string &macroName) 
{
	return invalidHashHash(loc, macroName, "Unexpected newline");
}
		
/*static*/Macro::invalidHashHash Macro::invalidHashHash::universalCharacterUB(const Location &loc, const std::string &macroName, const Token* tokenA, const std::string& strAB) 
{
	return invalidHashHash(loc, macroName,
		"Combining '\\"+ tokenA->str()+ "' and '"+ strAB.substr(tokenA->str().size()) + "' yields universal character '\\" + strAB +
		"'. This is undefined behavior according to C standard chapter 5.1.1.2, paragraph 4.");
}

Macro::Macro(std::vector<std::string> &f) : nameTokDef(nullptr), valueToken(nullptr), endToken(nullptr), files(f), tokenListDefine(f), variadic(false),
	valueDefinedInCode_(false) 
{
}

Macro::Macro(const Token * tok, std::vector<std::string> &f) : nameTokDef(nullptr), files(f), tokenListDefine(f), valueDefinedInCode_(true) 
{
	if(sameline(tok->previousSkipComments(), tok))
		throw std::runtime_error("bad macro syntax");
	if(tok->op != '#')
		throw std::runtime_error("bad macro syntax");
	const Token * const hashtok = tok;
	tok = tok->next;
	if(!tok || tok->str() != DEFINE)
		throw std::runtime_error("bad macro syntax");
	tok = tok->next;
	if(!tok || !tok->name || !sameline(hashtok, tok))
		throw std::runtime_error("bad macro syntax");
	if(!parseDefine(tok))
		throw std::runtime_error("bad macro syntax");
}

Macro::Macro(const std::string &name, const std::string &value, std::vector<std::string> &f) : nameTokDef(nullptr), files(f), tokenListDefine(f), valueDefinedInCode_(false) 
{
	const std::string def(name + ' ' + value);
	StdCharBufStream stream(reinterpret_cast<const uchar*>(def.data()), def.size());
	tokenListDefine.readfile(stream);
	if(!parseDefine(tokenListDefine.cfront()))
		throw std::runtime_error("bad macro syntax. macroname=" + name + " value=" + value);
}

Macro::Macro(const Macro &other) : nameTokDef(nullptr), files(other.files), tokenListDefine(other.files), valueDefinedInCode_(other.valueDefinedInCode_) 
{
	*this = other;
}

Macro & Macro::operator = (const Macro &other) 
{
	if(this != &other) {
		files = other.files;
		valueDefinedInCode_ = other.valueDefinedInCode_;
		if(other.tokenListDefine.empty())
			parseDefine(other.nameTokDef);
		else {
			tokenListDefine = other.tokenListDefine;
			parseDefine(tokenListDefine.cfront());
		}
		usageList = other.usageList;
	}
	return *this;
}

bool Macro::functionLike() const 
{
	return nameTokDef->next && nameTokDef->next->op == '(' && sameline(nameTokDef, nameTokDef->next) &&
		nameTokDef->next->location.col == nameTokDef->location.col + nameTokDef->str().size();
}

const Token * Macro::expand(TokenList * const pOutputTokenList, const Token * pRawTok, const MacroMap & macros, std::vector<std::string> & rInputFiles) const 
{
	std::set<TokenString> expandedmacros;
#ifdef SIMPLECPP_DEBUG_MACRO_EXPANSION
	std::cout << "expand " << name() << " " << locstring(rawtok->location) << std::endl;
#endif
	TokenList output2(rInputFiles);
	if(functionLike() && pRawTok->next && pRawTok->next->op == '(') {
		// Copy macro call to a new tokenlist with no linebreaks
		const Token * const rawtok1 = pRawTok;
		TokenList rawtokens2(rInputFiles);
		rawtokens2.push_back(new Token(pRawTok->str(), rawtok1->location, pRawTok->whitespaceahead));
		pRawTok = pRawTok->next;
		rawtokens2.push_back(new Token(pRawTok->str(), rawtok1->location, pRawTok->whitespaceahead));
		pRawTok = pRawTok->next;
		int par = 1;
		while(pRawTok && par > 0) {
			if(pRawTok->op == '(')
				++par;
			else if(pRawTok->op == ')')
				--par;
			else if(pRawTok->op == '#' && !sameline(pRawTok->previous, pRawTok))
				throw Error(pRawTok->location, "it is invalid to use a preprocessor directive as macro parameter");
			rawtokens2.push_back(new Token(pRawTok->str(), rawtok1->location, pRawTok->whitespaceahead));
			pRawTok = pRawTok->next;
		}
		if(expand(&output2, rawtok1->location, rawtokens2.cfront(), macros, expandedmacros))
			pRawTok = rawtok1->next;
	}
	else {
		pRawTok = expand(&output2, pRawTok->location, pRawTok, macros, expandedmacros);
	}
	while(output2.cback() && pRawTok) {
		uint par = 0;
		Token * macro2tok = output2.back();
		while(macro2tok) {
			if(macro2tok->op == '(') {
				if(par==0)
					break;
				--par;
			}
			else if(macro2tok->op == ')')
				++par;
			macro2tok = macro2tok->previous;
		}
		if(macro2tok) { // macro2tok->op == '('
			macro2tok = macro2tok->previous;
			expandedmacros.insert(name());
		}
		else if(pRawTok->op == '(')
			macro2tok = output2.back();
		if(!macro2tok || !macro2tok->name)
			break;
		if(output2.cfront() != output2.cback() && macro2tok->str() == this->name())
			break;
		const MacroMap::const_iterator macro = macros.find(macro2tok->str());
		if(macro == macros.end() || !macro->second.functionLike())
			break;
		TokenList rawtokens2(rInputFiles);
		const Location loc(macro2tok->location);
		while(macro2tok) {
			Token * const next = macro2tok->next;
			rawtokens2.push_back(new Token(macro2tok->str(), loc));
			output2.deleteToken(macro2tok);
			macro2tok = next;
		}
		par = (rawtokens2.cfront() != rawtokens2.cback()) ? 1U : 0U;
		const Token * rawtok2 = pRawTok;
		for(; rawtok2; rawtok2 = rawtok2->next) {
			rawtokens2.push_back(new Token(rawtok2->str(), loc));
			if(rawtok2->op == '(')
				++par;
			else if(rawtok2->op == ')') {
				if(par <= 1U)
					break;
				--par;
			}
		}
		if(!rawtok2 || par != 1U)
			break;
		if(macro->second.expand(&output2, pRawTok->location, rawtokens2.cfront(), macros, expandedmacros) != nullptr)
			break;
		pRawTok = rawtok2->next;
	}
	pOutputTokenList->takeTokens(output2);
	return pRawTok;
}

Token * Macro::newMacroToken(const TokenString &str, const Location &loc, bool replaced, const Token * expandedFromToken) const 
{
	Token * tok = new Token(str, loc);
	if(replaced)
		tok->macro = nameTokDef->str();
	if(expandedFromToken)
		tok->setExpandedFrom(expandedFromToken, this);
	return tok;
}

bool Macro::parseDefine(const Token * nametoken) 
{
	nameTokDef = nametoken;
	variadic = false;
	if(!nameTokDef) {
		valueToken = endToken = nullptr;
		args.clear();
		return false;
	}
	// function like macro..
	if(functionLike()) {
		args.clear();
		const Token * argtok = nameTokDef->next->next;
		while(sameline(nametoken, argtok) && argtok->op != ')') {
			if(argtok->str() == "..." &&
				argtok->next && argtok->next->op == ')') {
				variadic = true;
				if(!argtok->previous->name)
					args.push_back("__VA_ARGS__");
				argtok = argtok->next; // goto ')'
				break;
			}
			if(argtok->op != ',')
				args.push_back(argtok->str());
			argtok = argtok->next;
		}
		if(!sameline(nametoken, argtok)) {
			endToken = argtok ? argtok->previous : argtok;
			valueToken = nullptr;
			return false;
		}
		valueToken = argtok ? argtok->next : nullptr;
	}
	else {
		args.clear();
		valueToken = nameTokDef->next;
	}
	if(!sameline(valueToken, nameTokDef))
		valueToken = nullptr;
	endToken = valueToken;
	while(sameline(endToken, nameTokDef))
		endToken = endToken->next;
	return true;
}
	
uint Macro::getArgNum(const TokenString &str) const 
{
	uint par = 0;
	while(par < args.size()) {
		if(str == args[par])
			return par;
		par++;
	}
	return ~0U;
}
	
std::vector<const Token *> Macro::getMacroParameters(const Token * nameTokInst, bool calledInDefine) const 
{
	if(!nameTokInst->next || nameTokInst->next->op != '(' || !functionLike())
		return std::vector<const Token *>();
	std::vector<const Token *> parametertokens;
	parametertokens.push_back(nameTokInst->next);
	uint par = 0U;
	for(const Token * tok = nameTokInst->next->next; calledInDefine ? sameline(tok, nameTokInst) : (tok != nullptr); tok = tok->next) {
		if(tok->op == '(')
			++par;
		else if(tok->op == ')') {
			if(par == 0U) {
				parametertokens.push_back(tok);
				break;
			}
			--par;
		}
		else if(par == 0U && tok->op == ',' && (!variadic || parametertokens.size() < args.size()))
			parametertokens.push_back(tok);
	}
	return parametertokens;
}
	
const Token * Macro::appendTokens(TokenList * tokens, const Location &rawloc, const Token * const lpar, const MacroMap &macros,
	const std::set<TokenString> &expandedmacros, const std::vector<const Token*> &parametertokens) const 
{
	if(!lpar || lpar->op != '(')
		return nullptr;
	uint par = 0;
	const Token * tok = lpar;
	while(sameline(lpar, tok)) {
		if(tok->op == '#' && sameline(tok, tok->next) && tok->next->op == '#' && sameline(tok, tok->next->next)) {
			// A##B => AB
			tok = expandHashHash(tokens, rawloc, tok, macros, expandedmacros, parametertokens, false);
		}
		else if(tok->op == '#' && sameline(tok, tok->next) && tok->next->op != '#') {
			tok = expandHash(tokens, rawloc, tok, expandedmacros, parametertokens);
		}
		else {
			if(!expandArg(tokens, tok, rawloc, macros, expandedmacros, parametertokens)) {
				tokens->push_back(new Token(*tok));
				if(tok->macro.empty() && (par > 0 || tok->str() != "("))
					tokens->back()->macro = name();
			}

			if(tok->op == '(')
				++par;
			else if(tok->op == ')') {
				--par;
				if(par == 0U)
					break;
			}
			tok = tok->next;
		}
	}
	for(Token * tok2 = tokens->front(); tok2; tok2 = tok2->next)
		tok2->location = lpar->location;
	return sameline(lpar, tok) ? tok : nullptr;
}

const Token * Macro::expand(TokenList * const output, const Location &loc, const Token * const nameTokInst, const MacroMap &macros, std::set<TokenString> expandedmacros) const 
{
	expandedmacros.insert(nameTokInst->str());
#ifdef SIMPLECPP_DEBUG_MACRO_EXPANSION
	std::cout << "  expand " << name() << " " << locstring(defineLocation()) << std::endl;
#endif
	usageList.push_back(loc);
	if(nameTokInst->str() == "__FILE__") {
		output->push_back(new Token('\"'+loc.file()+'\"', loc));
		return nameTokInst->next;
	}
	if(nameTokInst->str() == "__LINE__") {
		output->push_back(new Token(toString(loc.line), loc));
		return nameTokInst->next;
	}
	if(nameTokInst->str() == "__COUNTER__") {
		output->push_back(new Token(toString(usageList.size()-1U), loc));
		return nameTokInst->next;
	}
	const bool calledInDefine = (loc.fileIndex != nameTokInst->location.fileIndex ||
		loc.line < nameTokInst->location.line);
	std::vector<const Token*> parametertokens1(getMacroParameters(nameTokInst, calledInDefine));
	if(functionLike()) {
		// No arguments => not macro expansion
		if(nameTokInst->next && nameTokInst->next->op != '(') {
			output->push_back(new Token(nameTokInst->str(), loc));
			return nameTokInst->next;
		}
		// Parse macro-call
		if(variadic) {
			if(parametertokens1.size() < args.size()) {
				throw wrongNumberOfParameters(nameTokInst->location, name());
			}
		}
		else {
			if(parametertokens1.size() != args.size() + (args.empty() ? 2U : 1U))
				throw wrongNumberOfParameters(nameTokInst->location, name());
		}
	}
	// If macro call uses __COUNTER__ then expand that first
	TokenList tokensparams(files);
	std::vector<const Token *> parametertokens2;
	if(!parametertokens1.empty()) {
		bool counter = false;
		for(const Token * tok = parametertokens1[0]; tok != parametertokens1.back(); tok = tok->next) {
			if(tok->str() == "__COUNTER__") {
				counter = true;
				break;
			}
		}
		const MacroMap::const_iterator m = macros.find("__COUNTER__");
		if(!counter || m == macros.end())
			parametertokens2.swap(parametertokens1);
		else {
			const Macro &counterMacro = m->second;
			uint par = 0;
			for(const Token * tok = parametertokens1[0]; tok && par < parametertokens1.size(); tok = tok->next) {
				if(tok->str() == "__COUNTER__") {
					tokensparams.push_back(new Token(toString(counterMacro.usageList.size()), tok->location));
					counterMacro.usageList.push_back(tok->location);
				}
				else {
					tokensparams.push_back(new Token(*tok));
					if(tok == parametertokens1[par]) {
						parametertokens2.push_back(tokensparams.cback());
						par++;
					}
				}
			}
		}
	}
	Token * const output_end_1 = output->back();
	// expand
	for(const Token * tok = valueToken; tok != endToken;) {
		if(tok->op != '#') {
			// A##B => AB
			if(sameline(tok, tok->next) && tok->next && tok->next->op == '#' && tok->next->next && tok->next->next->op == '#') {
				if(!sameline(tok, tok->next->next->next))
					throw invalidHashHash::unexpectedNewline(tok->location, name());
				if(variadic && tok->op == ',' && tok->next->next->next->str() == args.back()) {
					Token *const comma = newMacroToken(tok->str(), loc, isReplaced(expandedmacros), tok);
					output->push_back(comma);
					tok = expandToken(output, loc, tok->next->next->next, macros, expandedmacros, parametertokens2);
					if(output->back() == comma)
						output->deleteToken(comma);
					continue;
				}
				TokenList new_output(files);
				if(!expandArg(&new_output, tok, parametertokens2))
					output->push_back(newMacroToken(tok->str(), loc, isReplaced(expandedmacros), tok));
				else if(new_output.empty()) // placemarker token
					output->push_back(newMacroToken("", loc, isReplaced(expandedmacros)));
				else
					for(const Token * tok2 = new_output.cfront(); tok2; tok2 = tok2->next)
						output->push_back(newMacroToken(tok2->str(), loc, isReplaced(expandedmacros), tok2));
				tok = tok->next;
			}
			else {
				tok = expandToken(output, loc, tok, macros, expandedmacros, parametertokens2);
			}
			continue;
		}
		int numberOfHash = 1;
		const Token * hashToken = tok->next;
		while(sameline(tok, hashToken) && hashToken->op == '#') {
			hashToken = hashToken->next;
			++numberOfHash;
		}
		if(numberOfHash == 4 && tok->next->location.col + 1 == tok->next->next->location.col) {
			// # ## #  => ##
			output->push_back(newMacroToken("##", loc, isReplaced(expandedmacros)));
			tok = hashToken;
			continue;
		}

		if(numberOfHash >= 2 && tok->location.col + 1 < tok->next->location.col) {
			output->push_back(new Token(*tok));
			tok = tok->next;
			continue;
		}

		tok = tok->next;
		if(tok == endToken) {
			output->push_back(new Token(*tok->previous));
			break;
		}
		if(tok->op == '#') {
			// A##B => AB
			tok = expandHashHash(output, loc, tok->previous, macros, expandedmacros, parametertokens2);
		}
		else {
			// #123 => "123"
			tok = expandHash(output, loc, tok->previous, expandedmacros, parametertokens2);
		}
	}
	if(!functionLike()) {
		for(Token * tok = output_end_1 ? output_end_1->next : output->front(); tok; tok = tok->next) {
			tok->macro = nameTokInst->str();
		}
	}
	if(!parametertokens1.empty())
		parametertokens1.swap(parametertokens2);
	return functionLike() ? parametertokens2.back()->next : nameTokInst->next;
}

const Token * Macro::recursiveExpandToken(TokenList * output, TokenList &temp, const Location &loc, const Token * tok,
	const MacroMap &macros, const std::set<TokenString> &expandedmacros, const std::vector<const Token*> &parametertokens) const 
{
	if(!(temp.cback() && temp.cback()->name && tok->next && tok->next->op == '(')) {
		output->takeTokens(temp);
		return tok->next;
	}

	if(!sameline(tok, tok->next)) {
		output->takeTokens(temp);
		return tok->next;
	}
	const MacroMap::const_iterator it = macros.find(temp.cback()->str());
	if(it == macros.end() || expandedmacros.find(temp.cback()->str()) != expandedmacros.end()) {
		output->takeTokens(temp);
		return tok->next;
	}
	const Macro &calledMacro = it->second;
	if(!calledMacro.functionLike()) {
		output->takeTokens(temp);
		return tok->next;
	}
	TokenList temp2(files);
	temp2.push_back(new Token(temp.cback()->str(), tok->location));
	const Token * const tok2 = appendTokens(&temp2, loc, tok->next, macros, expandedmacros, parametertokens);
	if(!tok2)
		return tok->next;
	output->takeTokens(temp);
	output->deleteToken(output->back());
	calledMacro.expand(output, loc, temp2.cfront(), macros, expandedmacros);
	return tok2->next;
}

const Token * Macro::expandToken(TokenList * output, const Location &loc, const Token * tok, const MacroMap &macros, const std::set<TokenString> &expandedmacros,
	const std::vector<const Token*> &parametertokens) const 
{
	// Not name..
	if(!tok->name) {
		output->push_back(newMacroToken(tok->str(), loc, true, tok));
		return tok->next;
	}
	// Macro parameter..
	{
		TokenList temp(files);
		if(tok->str() == "__VA_OPT__") {
			if(sameline(tok, tok->next) && tok->next->str() == "(") {
				tok = tok->next;
				int paren = 1;
				while(sameline(tok, tok->next)) {
					if(tok->next->str() == "(")
						++paren;
					else if(tok->next->str() == ")")
						--paren;
					if(paren == 0)
						return tok->next->next;
					tok = tok->next;
					if(parametertokens.size() > args.size() && parametertokens.front()->next->str() != ")")
						tok = expandToken(output, loc, tok, macros, expandedmacros, parametertokens)->previous;
				}
			}
			throw Error(tok->location, "Missing parenthesis for __VA_OPT__(content)");
		}
		if(expandArg(&temp, tok, loc, macros, expandedmacros, parametertokens)) {
			if(tok->str() == "__VA_ARGS__" && temp.empty() && output->cback() && output->cback()->str() == "," &&
				tok->nextSkipComments() && tok->nextSkipComments()->str() == ")")
				output->deleteToken(output->back());
			return recursiveExpandToken(output, temp, loc, tok, macros, expandedmacros, parametertokens);
		}
	}

	// Macro..
	const MacroMap::const_iterator it = macros.find(tok->str());
	if(it != macros.end() && expandedmacros.find(tok->str()) == expandedmacros.end()) {
		std::set<std::string> expandedmacros2(expandedmacros);
		expandedmacros2.insert(tok->str());

		const Macro &calledMacro = it->second;
		if(!calledMacro.functionLike()) {
			TokenList temp(files);
			calledMacro.expand(&temp, loc, tok, macros, expandedmacros);
			return recursiveExpandToken(output, temp, loc, tok, macros, expandedmacros2, parametertokens);
		}
		if(!sameline(tok, tok->next)) {
			output->push_back(newMacroToken(tok->str(), loc, true, tok));
			return tok->next;
		}
		TokenList tokens(files);
		tokens.push_back(new Token(*tok));
		const Token * tok2 = nullptr;
		if(tok->next->op == '(')
			tok2 = appendTokens(&tokens, loc, tok->next, macros, expandedmacros, parametertokens);
		else if(expandArg(&tokens, tok->next, loc, macros, expandedmacros, parametertokens)) {
			tokens.front()->location = loc;
			if(tokens.cfront()->next && tokens.cfront()->next->op == '(')
				tok2 = tok->next;
		}
		if(!tok2) {
			output->push_back(newMacroToken(tok->str(), loc, true, tok));
			return tok->next;
		}
		TokenList temp(files);
		calledMacro.expand(&temp, loc, tokens.cfront(), macros, expandedmacros);
		return recursiveExpandToken(output, temp, loc, tok2, macros, expandedmacros, parametertokens);
	}
	if(tok->str() == DEFINED) {
		const Token * const tok2 = tok->next;
		const Token * const tok3 = tok2 ? tok2->next : nullptr;
		const Token * const tok4 = tok3 ? tok3->next : nullptr;
		const Token * defToken = nullptr;
		const Token * lastToken = nullptr;
		if(sameline(tok, tok4) && tok2->op == '(' && tok3->name && tok4->op == ')') {
			defToken = tok3;
			lastToken = tok4;
		}
		else if(sameline(tok, tok2) && tok2->name) {
			defToken = lastToken = tok2;
		}
		if(defToken) {
			std::string macroName = defToken->str();
			if(defToken->next && defToken->next->op == '#' && defToken->next->next && defToken->next->next->op == '#' && defToken->next->next->next &&
				defToken->next->next->next->name && sameline(defToken, defToken->next->next->next)) {
				TokenList temp(files);
				if(expandArg(&temp, defToken, parametertokens))
					macroName = temp.cback()->str();
				if(expandArg(&temp, defToken->next->next->next, parametertokens))
					macroName += temp.cback() ? temp.cback()->str() : "";
				else
					macroName += defToken->next->next->next->str();
				lastToken = defToken->next->next->next;
			}
			const bool def = (macros.find(macroName) != macros.end());
			output->push_back(newMacroToken(def ? "1" : "0", loc, true));
			return lastToken->next;
		}
	}
	output->push_back(newMacroToken(tok->str(), loc, true, tok));
	return tok->next;
}

bool Macro::expandArg(TokenList * output, const Token * tok, const std::vector<const Token*> &parametertokens) const 
{
	if(!tok->name)
		return false;
	const uint argnr = getArgNum(tok->str());
	if(argnr >= args.size())
		return false;
	// empty variadic parameter
	if(variadic && argnr + 1U >= parametertokens.size())
		return true;
	for(const Token * partok = parametertokens[argnr]->next; partok != parametertokens[argnr + 1U]; partok = partok->next)
		output->push_back(new Token(*partok));
	return true;
}

bool Macro::expandArg(TokenList * output, const Token * tok, const Location &loc, const MacroMap &macros, const std::set<TokenString> &expandedmacros, const std::vector<const Token*> &parametertokens) const 
{
	if(!tok->name)
		return false;
	const uint argnr = getArgNum(tok->str());
	if(argnr >= args.size())
		return false;
	if(variadic && argnr + 1U >= parametertokens.size()) // empty variadic parameter
		return true;
	for(const Token * partok = parametertokens[argnr]->next; partok != parametertokens[argnr + 1U];) {
		const MacroMap::const_iterator it = macros.find(partok->str());
		if(it != macros.end() && !partok->isExpandedFrom(&it->second) && (partok->str() == name() || expandedmacros.find(partok->str()) == expandedmacros.end())) {
			std::set<TokenString> expandedmacros2(expandedmacros); // temporary amnesia to allow reexpansion of currently expanding macros during argument evaluation
			expandedmacros2.erase(name());
			partok = it->second.expand(output, loc, partok, macros, expandedmacros2);
		}
		else {
			output->push_back(newMacroToken(partok->str(), loc, isReplaced(expandedmacros), partok));
			output->back()->macro = partok->macro;
			partok = partok->next;
		}
	}
	if(tok->whitespaceahead && output->back())
		output->back()->whitespaceahead = true;
	return true;
}

const Token * Macro::expandHash(TokenList * output, const Location &loc, const Token * tok, const std::set<TokenString> &expandedmacros,
	const std::vector<const Token*> &parametertokens) const 
{
	TokenList tokenListHash(files);
	const MacroMap macros2; // temporarily bypass macro expansion
	tok = expandToken(&tokenListHash, loc, tok->next, macros2, expandedmacros, parametertokens);
	std::ostringstream ostr;
	ostr << '\"';
	for(const Token * hashtok = tokenListHash.cfront(), * next; hashtok; hashtok = next) {
		next = hashtok->next;
		ostr << hashtok->str();
		if(next && hashtok->whitespaceahead)
			ostr << ' ';
	}
	ostr << '\"';
	output->push_back(newMacroToken(escapeString(ostr.str()), loc, isReplaced(expandedmacros)));
	return tok;
}

const Token * Macro::expandHashHash(TokenList * output, const Location &loc, const Token * tok, const MacroMap & macros,
	const std::set<TokenString> &expandedmacros, const std::vector<const Token*> & parametertokens, bool expandResult) const 
{
	Token * A = output->back();
	if(!A)
		throw invalidHashHash(tok->location, name(), "Missing first argument");
	if(!sameline(tok, tok->next) || !sameline(tok, tok->next->next))
		throw invalidHashHash::unexpectedNewline(tok->location, name());

	const bool canBeConcatenatedWithEqual = A->isOneOf("+-*/%&|^") || A->str() == "<<" || A->str() == ">>";
	const bool canBeConcatenatedStringOrChar = isStringLiteral_(A->str()) || isCharLiteral_(A->str());
	const bool unexpectedA = (!A->name && !A->number && !A->str().empty() && !canBeConcatenatedWithEqual && !canBeConcatenatedStringOrChar);

	Token * const B = tok->next->next;
	if(!B->name && !B->number && B->op && !B->isOneOf("#="))
		throw invalidHashHash::unexpectedToken(tok->location, name(), B);

	if((canBeConcatenatedWithEqual && B->op != '=') ||
		(!canBeConcatenatedWithEqual && B->op == '='))
		throw invalidHashHash::cannotCombine(tok->location, name(), A, B);

	// Superficial check; more in-depth would in theory be possible _after_ expandArg
	if(canBeConcatenatedStringOrChar && (B->number || !B->name))
		throw invalidHashHash::cannotCombine(tok->location, name(), A, B);

	TokenList tokensB(files);
	const Token * nextTok = B->next;

	if(canBeConcatenatedStringOrChar) {
		if(unexpectedA)
			throw invalidHashHash::unexpectedToken(tok->location, name(), A);

		// It seems clearer to handle this case separately even though the code is similar-ish, but we don't want to merge here.
		// TODO The question is whether the ## or varargs may still apply, and how to provoke?
		if(expandArg(&tokensB, B, parametertokens)) {
			for(Token * b = tokensB.front(); b; b = b->next)
				b->location = loc;
		}
		else {
			tokensB.push_back(new Token(*B));
			tokensB.back()->location = loc;
		}
		output->takeTokens(tokensB);
	}
	else {
		std::string strAB;
		const bool varargs = variadic && !args.empty() && B->str() == args[args.size()-1U];
		if(expandArg(&tokensB, B, parametertokens)) {
			if(tokensB.empty())
				strAB = A->str();
			else if(varargs && A->op == ',')
				strAB = ",";
			else if(varargs && unexpectedA)
				throw invalidHashHash::unexpectedToken(tok->location, name(), A);
			else {
				strAB = A->str() + tokensB.cfront()->str();
				tokensB.deleteToken(tokensB.front());
			}
		}
		else {
			if(unexpectedA)
				throw invalidHashHash::unexpectedToken(tok->location, name(), A);
			strAB = A->str() + B->str();
		}

		// producing universal character is undefined behavior
		if(A->previous && A->previous->str() == "\\") {
			if(strAB[0] == 'u' && strAB.size() == 5)
				throw invalidHashHash::universalCharacterUB(tok->location, name(), A, strAB);
			if(strAB[0] == 'U' && strAB.size() == 9)
				throw invalidHashHash::universalCharacterUB(tok->location, name(), A, strAB);
		}

		if(varargs && tokensB.empty() && tok->previous->str() == ",")
			output->deleteToken(A);
		else if(strAB != "," && macros.find(strAB) == macros.end()) {
			A->setstr(strAB);
			for(Token * b = tokensB.front(); b; b = b->next)
				b->location = loc;
			output->takeTokens(tokensB);
		}
		else if(sameline(B, nextTok) && sameline(B, nextTok->next) && nextTok->op == '#' && nextTok->next->op == '#') {
			TokenList output2(files);
			output2.push_back(new Token(strAB, tok->location));
			nextTok = expandHashHash(&output2, loc, nextTok, macros, expandedmacros, parametertokens);
			output->deleteToken(A);
			output->takeTokens(output2);
		}
		else {
			output->deleteToken(A);
			TokenList tokens(files);
			tokens.push_back(new Token(strAB, tok->location));
			// for function like macros, push the (...)
			if(tokensB.empty() && sameline(B, B->next) && B->next->op=='(') {
				const MacroMap::const_iterator it = macros.find(strAB);
				if(it != macros.end() && expandedmacros.find(strAB) == expandedmacros.end() && it->second.functionLike()) {
					const Token * const tok2 = appendTokens(&tokens, loc, B->next, macros, expandedmacros, parametertokens);
					if(tok2)
						nextTok = tok2->next;
				}
			}
			if(expandResult)
				expandToken(output, loc, tokens.cfront(), macros, expandedmacros, parametertokens);
			else
				output->takeTokens(tokens);
			for(Token * b = tokensB.front(); b; b = b->next)
				b->location = loc;
			output->takeTokens(tokensB);
		}
	}
	return nextTok;
}

/*static*/bool Macro::isReplaced(const std::set<std::string> &expandedmacros) 
{
	// return true if size > 1
	std::set<std::string>::const_iterator it = expandedmacros.begin();
	if(it == expandedmacros.end())
		return false;
	++it;
	return (it != expandedmacros.end());
}

}

namespace simplecpp {
#ifdef __CYGWIN__
	bool startsWith(const std::string &str, const std::string &s)
	{
		return (str.size() >= s.size() && str.compare(0, s.size(), s) == 0);
	}

	std::string convertCygwinToWindowsPath(const std::string &cygwinPath)
	{
		std::string windowsPath;

		std::string::size_type pos = 0;
		if(cygwinPath.size() >= 11 && startsWith(cygwinPath, "/cygdrive/")) {
			const uchar driveLetter = cygwinPath[10];
			if(std::isalpha(driveLetter)) {
				if(cygwinPath.size() == 11) {
					windowsPath = toupper(driveLetter);
					windowsPath += ":\\"; // volume root directory
					pos = 11;
				}
				else if(cygwinPath[11] == '/') {
					windowsPath = toupper(driveLetter);
					windowsPath += ":";
					pos = 11;
				}
			}
		}
		for(; pos < cygwinPath.size(); ++pos) {
			uchar c = cygwinPath[pos];
			if(c == '/')
				c = '\\';
			windowsPath += c;
		}
		return windowsPath;
	}
#endif
}

#ifdef SIMPLECPP_WINDOWS

#if __cplusplus >= 201103L
	using MyMutex = std::mutex;
	template <class T>
	using MyLock = std::lock_guard<T>;
#else
	class MyMutex {
	public:
		MyMutex() 
		{
			InitializeCriticalSection(&m_criticalSection);
		}
		~MyMutex() 
		{
			DeleteCriticalSection(&m_criticalSection);
		}
		CRITICAL_SECTION * lock() { return &m_criticalSection; }
	private:
		CRITICAL_SECTION m_criticalSection;
	};

	template <typename T> class MyLock {
	public:
		explicit MyLock(T& m) : m_mutex(m) 
		{
			EnterCriticalSection(m_mutex.lock());
		}
		~MyLock() 
		{
			LeaveCriticalSection(m_mutex.lock());
		}
	private:
		MyLock & operator=(const MyLock&);
		MyLock(const MyLock&);
		T & m_mutex;
	};
#endif

class RealFileNameMap {
public:
	RealFileNameMap() 
	{
	}
	bool getCacheEntry(const std::string & path, std::string & returnPath) 
	{
		MyLock<MyMutex> lock(m_mutex);
		const std::map<std::string, std::string>::iterator it = m_fileMap.find(path);
		if(it != m_fileMap.end()) {
			returnPath = it->second;
			return true;
		}
		return false;
	}
	void addToCache(const std::string& path, const std::string& actualPath) 
	{
		MyLock<MyMutex> lock(m_mutex);
		m_fileMap[path] = actualPath;
	}
private:
	std::map<std::string, std::string> m_fileMap;
	MyMutex m_mutex;
};

static RealFileNameMap realFileNameMap;

static bool realFileName(const std::string &f, std::string &result)
{
	// are there alpha characters in last subpath?
	bool alpha = false;
	for(std::string::size_type pos = 1; pos <= f.size(); ++pos) {
		const uchar c = f[f.size() - pos];
		if(c == '/' || c == '\\')
			break;
		if(std::isalpha(c)) {
			alpha = true;
			break;
		}
	}
	// do not convert this path if there are no alpha characters (either pointless or cause wrong results for . and ..)
	if(!alpha)
		return false;
	// Lookup filename or foldername on file system
	if(!realFileNameMap.getCacheEntry(f, result)) {
		WIN32_FIND_DATAA FindFileData;
#ifdef __CYGWIN__
		const std::string fConverted = simplecpp::convertCygwinToWindowsPath(f);
		const HANDLE hFind = FindFirstFileExA(fConverted.c_str(), FindExInfoBasic, &FindFileData, FindExSearchNameMatch, NULL, 0);
#else
		HANDLE hFind = FindFirstFileExA(f.c_str(), FindExInfoBasic, &FindFileData, FindExSearchNameMatch, NULL, 0);
#endif
		if(INVALID_HANDLE_VALUE == hFind)
			return false;
		result = FindFileData.cFileName;
		realFileNameMap.addToCache(f, result);
		FindClose(hFind);
	}
	return true;
}

static RealFileNameMap realFilePathMap;

/** Change case in given path to match filesystem */
static std::string realFilename(const std::string &f)
{
	std::string ret;
	ret.reserve(f.size()); // this will be the final size
	if(realFilePathMap.getCacheEntry(f, ret))
		return ret;
	// Current subpath
	std::string subpath;
	for(std::string::size_type pos = 0; pos < f.size(); ++pos) {
		const uchar c = f[pos];
		// Separator.. add subpath and separator
		if(c == '/' || c == '\\') {
			// if subpath is empty just add separator
			if(subpath.empty()) {
				ret += c;
				continue;
			}
			const bool isDriveSpecification = (pos == 2 && subpath.size() == 2 && std::isalpha(subpath[0]) && subpath[1] == ':');
			// Append real filename (proper case)
			std::string f2;
			if(!isDriveSpecification && realFileName(f.substr(0, pos), f2))
				ret += f2;
			else
				ret += subpath;
			subpath.clear();
			// Append separator
			ret += c;
		}
		else {
			subpath += c;
		}
	}
	if(!subpath.empty()) {
		std::string f2;
		if(realFileName(f, f2))
			ret += f2;
		else
			ret += subpath;
	}
	realFilePathMap.addToCache(f, ret);
	return ret;
}

static bool isAbsolutePath(const std::string &path)
{
	if(path.length() >= 3 && path[0] > 0 && std::isalpha(path[0]) && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
		return true;
	return path.length() > 1U && (path[0] == '/' || path[0] == '\\');
}

#else
#define realFilename(f)  f

static bool isAbsolutePath(const std::string &path)
{
	return path.length() > 1U && path[0] == '/';
}

#endif

namespace simplecpp {
	/**
	 * perform path simplifications for . and ..
	 */
	std::string simplifyPath(std::string path)
	{
		if(path.empty())
			return path;
		std::string::size_type pos;
		// replace backslash separators
		std::replace(path.begin(), path.end(), '\\', '/');
		const bool unc(path.compare(0, 2, "//") == 0);
		// replace "//" with "/"
		pos = 0;
		while((pos = path.find("//", pos)) != std::string::npos) {
			path.erase(pos, 1);
		}
		// remove "./"
		pos = 0;
		while((pos = path.find("./", pos)) != std::string::npos) {
			if(pos == 0 || path[pos - 1U] == '/')
				path.erase(pos, 2);
			else
				pos += 2;
		}
		// remove trailing dot if path ends with "/."
		if(endsWith(path, "/."))
			path.erase(path.size()-1);

		// simplify ".."
		pos = 1; // don't simplify ".." if path starts with that
		while((pos = path.find("/..", pos)) != std::string::npos) {
			// not end of path, then string must be "/../"
			if(pos + 3 < path.size() && path[pos + 3] != '/') {
				++pos;
				continue;
			}
			// get previous subpath
			std::string::size_type pos1 = path.rfind('/', pos - 1U);
			if(pos1 == std::string::npos) {
				pos1 = 0;
			}
			else {
				pos1 += 1U;
			}
			const std::string previousSubPath = path.substr(pos1, pos - pos1);
			if(previousSubPath == "..") {
				// don't simplify
				++pos;
			}
			else {
				// remove previous subpath and ".."
				path.erase(pos1, pos - pos1 + 4);
				if(path.empty())
					path = ".";
				// update pos
				pos = (pos1 == 0) ? 1 : (pos1 - 1);
			}
		}
		// Remove trailing '/'?
		//if (path.size() > 1 && endsWith(path, "/"))
		//    path.erase(path.size()-1);
		if(unc)
			path = '/' + path;
		// cppcheck-suppress duplicateExpressionTernary - platform-dependent implementation
		return strpbrk(path.c_str(), "*?") == nullptr ? realFilename(path) : path;
	}
}

/** Evaluate sizeof(type) */
static void simplifySizeof(simplecpp::TokenList &expr, const std::map<std::string, std::size_t> &sizeOfType)
{
	for(simplecpp::Token * tok = expr.front(); tok; tok = tok->next) {
		if(tok->str() != "sizeof")
			continue;
		simplecpp::Token * tok1 = tok->next;
		if(!tok1) {
			throw std::runtime_error("missing sizeof argument");
		}
		simplecpp::Token * tok2 = tok1->next;
		if(!tok2) {
			throw std::runtime_error("missing sizeof argument");
		}
		if(tok1->op == '(') {
			tok1 = tok1->next;
			while(tok2->op != ')') {
				tok2 = tok2->next;
				if(!tok2) {
					throw std::runtime_error("invalid sizeof expression");
				}
			}
		}
		std::string type;
		for(simplecpp::Token * typeToken = tok1; typeToken != tok2; typeToken = typeToken->next) {
			if((typeToken->str() == "unsigned" || typeToken->str() == "signed") && typeToken->next->name)
				continue;
			if(typeToken->str() == "*" && type.find('*') != std::string::npos)
				continue;
			if(!type.empty())
				type += ' ';
			type += typeToken->str();
		}
		const std::map<std::string, std::size_t>::const_iterator it = sizeOfType.find(type);
		if(it != sizeOfType.end())
			tok->setstr(toString(it->second));
		else
			continue;
		tok2 = tok2->next;
		while(tok->next != tok2)
			expr.deleteToken(tok->next);
	}
}

/** Evaluate __has_include(file) */
static bool isCpp17OrLater(const simplecpp::DUI &dui)
{
	const std::string std_ver = simplecpp::getCppStdString(dui.std);
	return !std_ver.empty() && (std_ver >= "201703L");
}

static std::string openHeader(std::ifstream &f, const simplecpp::DUI &dui, const std::string &sourcefile, const std::string &header, bool systemheader);

static void simplifyHasInclude(simplecpp::TokenList &expr, const simplecpp::DUI &dui)
{
	if(!isCpp17OrLater(dui))
		return;

	for(simplecpp::Token * tok = expr.front(); tok; tok = tok->next) {
		if(tok->str() != HAS_INCLUDE)
			continue;
		simplecpp::Token * tok1 = tok->next;
		if(!tok1) {
			throw std::runtime_error("missing __has_include argument");
		}
		simplecpp::Token * tok2 = tok1->next;
		if(!tok2) {
			throw std::runtime_error("missing __has_include argument");
		}
		if(tok1->op == '(') {
			tok1 = tok1->next;
			while(tok2->op != ')') {
				tok2 = tok2->next;
				if(!tok2) {
					throw std::runtime_error("invalid __has_include expression");
				}
			}
		}

		const std::string &sourcefile = tok->location.file();
		const bool systemheader = (tok1 && tok1->op == '<');
		std::string header;
		if(systemheader) {
			simplecpp::Token * tok3 = tok1->next;
			if(!tok3) {
				throw std::runtime_error("missing __has_include closing angular bracket");
			}
			while(tok3->op != '>') {
				tok3 = tok3->next;
				if(!tok3) {
					throw std::runtime_error("invalid __has_include expression");
				}
			}

			for(simplecpp::Token * headerToken = tok1->next; headerToken != tok3; headerToken = headerToken->next)
				header += headerToken->str();
			// cppcheck-suppress selfAssignment - platform-dependent implementation
			header = realFilename(header);
		}
		else {
			header = realFilename(tok1->str().substr(1U, tok1->str().size() - 2U));
		}
		std::ifstream f;
		const std::string header2 = openHeader(f, dui, sourcefile, header, systemheader);
		tok->setstr(header2.empty() ? "0" : "1");

		tok2 = tok2->next;
		while(tok->next != tok2)
			expr.deleteToken(tok->next);
	}
}

static const char * const altopData[] = { "and", "or", "bitand", "bitor", "compl", "not", "not_eq", "xor" };
static const std::set<std::string> altop(&altopData[0], &altopData[8]);

static void simplifyName(simplecpp::TokenList &expr)
{
	for(simplecpp::Token * tok = expr.front(); tok; tok = tok->next) {
		if(tok->name) {
			if(altop.find(tok->str()) != altop.end()) {
				bool alt;
				if(tok->str() == "not" || tok->str() == "compl") {
					alt = isAlternativeUnaryOp(tok, tok->str());
				}
				else {
					alt = isAlternativeBinaryOp(tok, tok->str());
				}
				if(alt)
					continue;
			}
			tok->setstr("0");
		}
	}
}
/*
 * Reads at least minlen and at most maxlen digits (inc. prefix) in base base
 * from s starting at position pos and converts them to a
 * unsigned long long value, updating pos to point to the first
 * unused element of s.
 * Returns ULLONG_MAX if the result is not representable and
 * throws if the above requirements were not possible to satisfy.
 */
static uint64 stringToULLbounded(const std::string& s, std::size_t& pos, int base = 0, std::ptrdiff_t minlen = 1, std::size_t maxlen = std::string::npos)
{
	const std::string sub = s.substr(pos, maxlen);
	const char * const start = sub.c_str();
	char* end;
	const uint64 value = std::strtoull(start, &end, base);
	pos += end - start;
	if(end - start < minlen)
		throw std::runtime_error("expected digit");
	return value;
}
// 
// Converts character literal (including prefix, but not ud-suffix) to long long value.
// 
// Assumes ASCII-compatible single-byte encoded str for narrow literals and UTF-8 otherwise.
// 
// For target assumes
//   - execution character set encoding matching str
//   - UTF-32 execution wide-character set encoding
//   - requirements for __STDC_UTF_16__, __STDC_UTF_32__ and __STDC_ISO_10646__ satisfied
//   - char16_t is 16bit wide
//   - char32_t is 32bit wide
//   - wchar_t is 32bit wide and unsigned
//   - matching char signedness to host
//   - matching sizeof(int) to host
// 
// For host assumes
//   - ASCII-compatible execution character set
// 
// For host and target assumes
//   - CHAR_BIT == 8
//   - two's complement
// 
// Implements multi-character narrow literals according to GCC's behavior,
// except multi code unit universal character names are not supported.
// Multi-character wide literals are not supported.
// Limited support of universal character names for non-UTF-8 execution character set encodings.
// 
int64 simplecpp::characterLiteralToLL(const std::string& str)
{
	// default is wide/utf32
	bool narrow = false;
	bool utf8 = false;
	bool utf16 = false;
	std::size_t pos;
	if(!str.empty() && str[0] == '\'') {
		narrow = true;
		pos = 1;
	}
	else if(str.size() >= 2 && str[0] == 'u' && str[1] == '\'') {
		utf16 = true;
		pos = 2;
	}
	else if(str.size() >= 3 && str[0] == 'u' && str[1] == '8' && str[2] == '\'') {
		utf8 = true;
		pos = 3;
	}
	else if(str.size() >= 2 && (str[0] == 'L' || str[0] == 'U') && str[1] == '\'') {
		pos = 2;
	}
	else
		throw std::runtime_error("expected a character literal");
	uint64 multivalue = 0;
	std::size_t nbytes = 0;
	while(pos + 1 < str.size()) {
		if(str[pos] == '\'' || str[pos] == '\n')
			throw std::runtime_error("raw single quotes and newlines not allowed in character literals");
		if(nbytes >= 1 && !narrow)
			throw std::runtime_error("multiple characters only supported in narrow character literals");
		uint64 value;
		if(str[pos] == '\\') {
			pos++;
			const char escape = str[pos++];
			if(pos >= str.size())
				throw std::runtime_error("unexpected end of character literal");
			switch(escape) {
				// obscure GCC extensions
				case '%':
				case '(':
				case '[':
				case '{':
				// standard escape sequences
				case '\'':
				case '"':
				case '?':
				case '\\':
				    value = static_cast<uchar>(escape);
				    break;
				case 'a':
				    value = static_cast<uchar>('\a');
				    break;
				case 'b':
				    value = static_cast<uchar>('\b');
				    break;
				case 'f':
				    value = static_cast<uchar>('\f');
				    break;
				case 'n':
				    value = static_cast<uchar>('\n');
				    break;
				case 'r':
				    value = static_cast<uchar>('\r');
				    break;
				case 't':
				    value = static_cast<uchar>('\t');
				    break;
				case 'v':
				    value = static_cast<uchar>('\v');
				    break;
				// GCC extension for ESC character
				case 'e':
				case 'E':
				    value = static_cast<uchar>('\x1b');
				    break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				    // octal escape sequences consist of 1 to 3 digits
				    value = stringToULLbounded(str, --pos, 8, 1, 3);
				    break;
				case 'x':
				    // hexadecimal escape sequences consist of at least 1 digit
				    value = stringToULLbounded(str, pos, 16);
				    break;
				case 'u':
				case 'U': {
				    // universal character names have exactly 4 or 8 digits
				    const std::size_t ndigits = (escape == 'u' ? 4 : 8);
				    value = stringToULLbounded(str, pos, 16, ndigits, ndigits);

				    // UTF-8 encodes code points above 0x7f in multiple code units
				    // code points above 0x10ffff are not allowed
				    if(((narrow || utf8) && value > 0x7f) || (utf16 && value > 0xffff) || value > 0x10ffff)
					    throw std::runtime_error("code point too large");

				    if(value >= 0xd800 && value <= 0xdfff)
					    throw std::runtime_error("surrogate code points not allowed in universal character names");

				    break;
			    }

				default:
				    throw std::runtime_error("invalid escape sequence");
			}
		}
		else {
			value = static_cast<uchar>(str[pos++]);

			if(!narrow && value >= 0x80) {
				// Assuming this is a UTF-8 encoded code point.
				// This decoder may not completely validate the input.
				// Noncharacters are neither rejected nor replaced.

				int additional_bytes;
				if(value >= 0xf5) // higher values would result in code points above 0x10ffff
					throw std::runtime_error("assumed UTF-8 encoded source, but sequence is invalid");
				if(value >= 0xf0)
					additional_bytes = 3;
				else if(value >= 0xe0)
					additional_bytes = 2;
				else if(value >= 0xc2) // 0xc0 and 0xc1 are always overlong 2-bytes encodings
					additional_bytes = 1;
				else
					throw std::runtime_error("assumed UTF-8 encoded source, but sequence is invalid");
				value &= (1 << (6 - additional_bytes)) - 1;
				while(additional_bytes--) {
					if(pos + 1 >= str.size())
						throw std::runtime_error("assumed UTF-8 encoded source, but character literal ends unexpectedly");
					const uchar c = str[pos++];
					if(((c >> 6) != 2) // ensure c has form 0xb10xxxxxx
					    || (!value && additional_bytes == 1 && c < 0xa0) // overlong 3-bytes encoding
					    || (!value && additional_bytes == 2 && c < 0x90)) // overlong 4-bytes encoding
						throw std::runtime_error("assumed UTF-8 encoded source, but sequence is invalid");

					value = (value << 6) | (c & ((1 << 7) - 1));
				}
				if(value >= 0xd800 && value <= 0xdfff)
					throw std::runtime_error("assumed UTF-8 encoded source, but sequence is invalid");
				if((utf8 && value > 0x7f) || (utf16 && value > 0xffff) || value > 0x10ffff)
					throw std::runtime_error("code point too large");
			}
		}
		if(((narrow || utf8) && value > std::numeric_limits<uchar>::max()) || (utf16 && value >> 16) || value >> 32)
			throw std::runtime_error("numeric escape sequence too large");

		multivalue <<= CHAR_BIT;
		multivalue |= value;
		nbytes++;
	}
	if(pos + 1 != str.size() || str[pos] != '\'')
		throw std::runtime_error("missing closing quote in character literal");
	if(!nbytes)
		throw std::runtime_error("empty character literal");
	// ordinary narrow character literal's value is determined by (possibly signed) char
	if(narrow && nbytes == 1)
		return static_cast<char>(multivalue);
	// while multi-character literal's value is determined by (signed) int
	if(narrow)
		return static_cast<int>(multivalue);
	// All other cases are unsigned. Since long long is at least 64bit wide,
	// while the literals at most 32bit wide, the conversion preserves all values.
	return multivalue;
}

static void simplifyNumbers(simplecpp::TokenList &expr)
{
	for(simplecpp::Token * tok = expr.front(); tok; tok = tok->next) {
		if(tok->str().size() == 1U)
			continue;
		if(tok->str().compare(0, 2, "0x") == 0)
			tok->setstr(toString(stringToULL(tok->str())));
		else if(!tok->number && tok->str().find('\'') != std::string::npos)
			tok->setstr(toString(simplecpp::characterLiteralToLL(tok->str())));
	}
}

static void simplifyComments(simplecpp::TokenList & expr)
{
	for(simplecpp::Token * tok = expr.front(); tok;) {
		simplecpp::Token * const d = tok;
		tok = tok->next;
		if(d->comment)
			expr.deleteToken(d);
	}
}

static int64 evaluate(simplecpp::TokenList &expr, const simplecpp::DUI &dui, const std::map<std::string, std::size_t> &sizeOfType)
{
	simplifyComments(expr);
	simplifySizeof(expr, sizeOfType);
	simplifyHasInclude(expr, dui);
	simplifyName(expr);
	simplifyNumbers(expr);
	expr.constFold();
	// TODO: handle invalid expressions
	return (expr.cfront() && expr.cfront() == expr.cback() && expr.cfront()->number) ? stringToLL(expr.cfront()->str()) : 0LL;
}

static const simplecpp::Token * gotoNextLine(const simplecpp::Token * tok)
{
	const uint line = tok->location.line;
	const uint file = tok->location.fileIndex;
	while(tok && tok->location.line == line && tok->location.fileIndex == file)
		tok = tok->next;
	return tok;
}

#ifdef SIMPLECPP_WINDOWS
	class NonExistingFilesCache {
	public:
		NonExistingFilesCache() 
		{
		}
		bool contains(const std::string& path) 
		{
			MyLock<MyMutex> lock(m_mutex);
			return (m_pathSet.find(path) != m_pathSet.end());
		}
		void add(const std::string& path) 
		{
			MyLock<MyMutex> lock(m_mutex);
			m_pathSet.insert(path);
		}
		void clear() 
		{
			MyLock<MyMutex> lock(m_mutex);
			m_pathSet.clear();
		}
	private:
		std::set<std::string> m_pathSet;
		MyMutex m_mutex;
	};

	static NonExistingFilesCache nonExistingFilesCache;
#endif

static std::string openHeader(std::ifstream &f, const std::string &path)
{
	std::string simplePath = simplecpp::simplifyPath(path);
#ifdef SIMPLECPP_WINDOWS
	if(nonExistingFilesCache.contains(simplePath))
		return ""; // file is known not to exist, skip expensive file open call
#endif
	f.open(simplePath.c_str());
	if(f.is_open())
		return simplePath;
#ifdef SIMPLECPP_WINDOWS
	nonExistingFilesCache.add(simplePath);
#endif
	return "";
}

static std::string getRelativeFileName(const std::string &sourcefile, const std::string &header)
{
	if(sourcefile.find_first_of("\\/") != std::string::npos)
		return simplecpp::simplifyPath(sourcefile.substr(0, sourcefile.find_last_of("\\/") + 1U) + header);
	return simplecpp::simplifyPath(header);
}

static std::string openHeaderRelative(std::ifstream &f, const std::string &sourcefile, const std::string &header)
{
	return openHeader(f, getRelativeFileName(sourcefile, header));
}

static std::string getIncludePathFileName(const std::string &includePath, const std::string &header)
{
	std::string path = includePath;
	if(!path.empty() && path[path.size()-1U]!='/' && path[path.size()-1U]!='\\')
		path += '/';
	return path + header;
}

static std::string openHeaderIncludePath(std::ifstream &f, const simplecpp::DUI &dui, const std::string &header)
{
	for(std::list<std::string>::const_iterator it = dui.includePaths.begin(); it != dui.includePaths.end(); ++it) {
		std::string simplePath = openHeader(f, getIncludePathFileName(*it, header));
		if(!simplePath.empty())
			return simplePath;
	}
	return "";
}

static std::string openHeader(std::ifstream &f, const simplecpp::DUI &dui, const std::string &sourcefile, const std::string &header, bool systemheader)
{
	if(isAbsolutePath(header))
		return openHeader(f, header);
	std::string ret;
	if(systemheader) {
		ret = openHeaderIncludePath(f, dui, header);
		return ret;
	}
	ret = openHeaderRelative(f, sourcefile, header);
	if(ret.empty())
		return openHeaderIncludePath(f, dui, header);
	return ret;
}

static std::string getFileName(const std::map<std::string, simplecpp::TokenList *> &filedata, const std::string &sourcefile,
    const std::string &header, const simplecpp::DUI &dui, bool systemheader)
{
	if(filedata.empty()) {
		return "";
	}
	if(isAbsolutePath(header)) {
		return (filedata.find(header) != filedata.end()) ? simplecpp::simplifyPath(header) : "";
	}

	if(!systemheader) {
		const std::string relativeFilename = getRelativeFileName(sourcefile, header);
		if(filedata.find(relativeFilename) != filedata.end())
			return relativeFilename;
	}
	for(std::list<std::string>::const_iterator it = dui.includePaths.begin(); it != dui.includePaths.end(); ++it) {
		std::string s = simplecpp::simplifyPath(getIncludePathFileName(*it, header));
		if(filedata.find(s) != filedata.end())
			return s;
	}
	if(systemheader && filedata.find(header) != filedata.end())
		return header;
	return "";
}

static bool hasFile(const std::map<std::string, simplecpp::TokenList *> &filedata, const std::string &sourcefile, const std::string &header, const simplecpp::DUI &dui, bool systemheader)
{
	return !getFileName(filedata, sourcefile, header, dui, systemheader).empty();
}

std::map <std::string, simplecpp::TokenList*> simplecpp::load(const simplecpp::TokenList &rawtokens, std::vector<std::string> &filenames, const simplecpp::DUI &dui, simplecpp::OutputList * outputList)
{
#ifdef SIMPLECPP_WINDOWS
	if(dui.clearIncludeCache)
		nonExistingFilesCache.clear();
#endif
	std::map<std::string, simplecpp::TokenList*> ret;
	std::list<const Token *> filelist;
	// -include files
	for(std::list<std::string>::const_iterator it = dui.includes.begin(); it != dui.includes.end(); ++it) {
		const std::string &filename = realFilename(*it);
		if(ret.find(filename) != ret.end())
			continue;
		std::ifstream fin(filename.c_str());
		if(!fin.is_open()) {
			SString msg_;
			msg_.Cat("Can not open include file").Space().CatChar('\'').Cat(filename.c_str()).CatChar('\'').Space().Cat("that is explicitly included.");
			MakeOutputMessage(outputList, filenames, simplecpp::Output::EXPLICIT_INCLUDE_NOT_FOUND, &Location(filenames), msg_);
			continue;
		}
		fin.close();
		TokenList * tokenlist = new TokenList(filename, filenames, outputList);
		if(!tokenlist->front()) {
			delete tokenlist;
			continue;
		}
		if(dui.removeComments)
			tokenlist->removeComments();
		ret[filename] = tokenlist;
		filelist.push_back(tokenlist->front());
	}
	for(const Token * rawtok = rawtokens.cfront(); rawtok || !filelist.empty(); rawtok = rawtok ? rawtok->next : nullptr) {
		if(rawtok == nullptr) {
			rawtok = filelist.back();
			filelist.pop_back();
		}
		if(rawtok->op != '#' || sameline(rawtok->previousSkipComments(), rawtok))
			continue;
		rawtok = rawtok->nextSkipComments();
		if(!rawtok || rawtok->str() != INCLUDE)
			continue;
		const std::string &sourcefile = rawtok->location.file();
		const Token * const htok = rawtok->nextSkipComments();
		if(!sameline(rawtok, htok))
			continue;
		const bool systemheader = (htok->str()[0] == '<');
		const std::string header(realFilename(htok->str().substr(1U, htok->str().size() - 2U)));
		if(hasFile(ret, sourcefile, header, dui, systemheader))
			continue;
		std::ifstream f;
		const std::string header2 = openHeader(f, dui, sourcefile, header, systemheader);
		if(!f.is_open())
			continue;
		f.close();
		TokenList * tokens = new TokenList(header2, filenames, outputList);
		if(dui.removeComments)
			tokens->removeComments();
		ret[header2] = tokens;
		if(tokens->front())
			filelist.push_back(tokens->front());
	}
	return ret;
}

static bool preprocessToken(simplecpp::TokenList & rOutputTokenList, const simplecpp::Token ** ppTok1, simplecpp::MacroMap & rMacros,
    std::vector<std::string> & files, simplecpp::OutputList * pMsgList)
{
	const simplecpp::Token * const tok = *ppTok1;
	const simplecpp::MacroMap::const_iterator it = rMacros.find(tok->str());
	if(it != rMacros.end()) {
		simplecpp::TokenList value(files);
		try {
			*ppTok1 = it->second.expand(&value, tok, rMacros, files);
		} catch(simplecpp::Macro::Error &err) {
			if(pMsgList) {
				simplecpp::Output out(files);
				out.type = simplecpp::Output::SYNTAX_ERROR;
				out.location = err.location;
				out.msg = "failed to expand \'" + tok->str() + "\', " + err.what;
				pMsgList->push_back(out);
			}
			return false;
		}
		rOutputTokenList.takeTokens(value);
	}
	else {
		if(!tok->comment)
			rOutputTokenList.push_back(new simplecpp::Token(*tok));
		*ppTok1 = tok->next;
	}
	return true;
}

static void getLocaltime(struct tm & ltime)
{
	time_t t;
	time(&t);
#ifndef _WIN32
	// NOLINTNEXTLINE(misc-include-cleaner) - false positive
	localtime_r(&t, &ltime);
#else
	localtime_s(&ltime, &t);
#endif
}

static std::string getDateDefine(const struct tm * timep)
{
	char buf[] = "??? ?? ????";
	strftime(buf, sizeof(buf), "%b %d %Y", timep);
	return std::string("\"").append(buf).append("\"");
}

static std::string getTimeDefine(const struct tm * timep)
{
	char buf[] = "??:??:??";
	strftime(buf, sizeof(buf), "%T", timep);
	return std::string("\"").append(buf).append("\"");
}
//
//
//
simplecpp::PreprocessBlock::PreprocessBlock() : Flags(0), FileList(), OutputTokenList(FileList)
{
}
		
simplecpp::TokenList simplecpp::PreprocessBlock::MakeSourceTokenList(const char * pSrcBuf, const char * pSrcName)
{
	StdCharBufStream stream(reinterpret_cast<const uchar*>(pSrcBuf), strlen(pSrcBuf));
	return TokenList(pSrcBuf, sstrlen(pSrcBuf), FileList, pSrcName, &MsgList);
}

bool simplecpp::PreprocessBlock::PreprocessToken(TokenList & rOutputTokenList, const Token ** ppTok1)
{
	const simplecpp::Token * const tok = *ppTok1;
	const simplecpp::MacroMap::const_iterator it = MacroList.find(tok->str());
	if(it != MacroList.end()) {
		simplecpp::TokenList value(FileList);
		try {
			*ppTok1 = it->second.expand(&value, tok, MacroList, FileList);
		} catch(simplecpp::Macro::Error &err) {
			if(Flags & fMsgList) {
				simplecpp::Output out(FileList);
				out.type = simplecpp::Output::SYNTAX_ERROR;
				out.location = err.location;
				out.msg = "failed to expand \'" + tok->str() + "\', " + err.what;
				MsgList.push_back(out);
			}
			return false;
		}
		rOutputTokenList.takeTokens(value);
	}
	else {
		if(!tok->comment)
			rOutputTokenList.push_back(new simplecpp::Token(*tok));
		*ppTok1 = tok->next;
	}
	return true;
}

static void MakeSizeOfTypeMap(std::map <std::string, std::size_t> & rMap)
{
	rMap.insert(std::make_pair("char", sizeof(char)));
	rMap.insert(std::make_pair("short", sizeof(short)));
	rMap.insert(std::make_pair("short int", rMap["short"]));
	rMap.insert(std::make_pair("int", sizeof(int)));
	rMap.insert(std::make_pair("long", sizeof(long)));
	rMap.insert(std::make_pair("long int", rMap["long"]));
	rMap.insert(std::make_pair("long long", sizeof(long long)));
	rMap.insert(std::make_pair("float", sizeof(float)));
	rMap.insert(std::make_pair("double", sizeof(double)));
	rMap.insert(std::make_pair("long double", sizeof(long double)));
	rMap.insert(std::make_pair("char *", sizeof(char *)));
	rMap.insert(std::make_pair("short *", sizeof(short *)));
	rMap.insert(std::make_pair("short int *", rMap["short *"]));
	rMap.insert(std::make_pair("int *", sizeof(int *)));
	rMap.insert(std::make_pair("long *", sizeof(long *)));
	rMap.insert(std::make_pair("long int *", rMap["long *"]));
	rMap.insert(std::make_pair("long long *", sizeof(long long *)));
	rMap.insert(std::make_pair("float *", sizeof(float *)));
	rMap.insert(std::make_pair("double *", sizeof(double *)));
	rMap.insert(std::make_pair("long double *", sizeof(long double *)));
}

static void MakePrereqMacrosMap(const simplecpp::DUI & rDui, simplecpp::MacroMap & rMap)
{
	std::vector<std::string> dummy; // use a dummy vector for the macros because as this is not part of the file and would add an empty entry - e.g. /usr/include/poll.h
	for(std::list<std::string>::const_iterator it = rDui.defines.begin(); it != rDui.defines.end(); ++it) {
		const std::string & macrostr = *it;
		const std::string::size_type eq = macrostr.find('=');
		const std::string::size_type par = macrostr.find('(');
		const std::string macroname = macrostr.substr(0, std::min(eq, par));
		if(rDui.undefined.find(macroname) != rDui.undefined.end())
			continue;
		const std::string lhs(macrostr.substr(0, eq));
		const std::string rhs(eq==std::string::npos ? std::string("1") : macrostr.substr(eq+1));
		const simplecpp::Macro macro(lhs, rhs, dummy);
		rMap.insert(std::pair<simplecpp::TokenString, simplecpp::Macro>(macro.name(), macro));
	}
	rMap.insert(std::make_pair("__FILE__", simplecpp::Macro("__FILE__", "__FILE__", dummy)));
	rMap.insert(std::make_pair("__LINE__", simplecpp::Macro("__LINE__", "__LINE__", dummy)));
	rMap.insert(std::make_pair("__COUNTER__", simplecpp::Macro("__COUNTER__", "__COUNTER__", dummy)));
	struct tm ltime = {};
	getLocaltime(ltime);
	rMap.insert(std::make_pair("__DATE__", simplecpp::Macro("__DATE__", getDateDefine(&ltime), dummy)));
	rMap.insert(std::make_pair("__TIME__", simplecpp::Macro("__TIME__", getTimeDefine(&ltime), dummy)));
}

static void PostprocessMacroUsageList(const simplecpp::MacroMap & rMacros, 
	std::map<std::string, std::list<simplecpp::Location> > & rMaybeUsedMacros, std::list <simplecpp::MacroUsage> & rMacroUsageList)
{
	for(simplecpp::MacroMap::const_iterator macroIt = rMacros.begin(); macroIt != rMacros.end(); ++macroIt) {
		const simplecpp::Macro & macro = macroIt->second;
		std::list <simplecpp::Location> usage = macro.usage();
		const std::list <simplecpp::Location> & temp = rMaybeUsedMacros[macro.name()];
		usage.insert(usage.end(), temp.begin(), temp.end());
		for(std::list<simplecpp::Location>::const_iterator usageIt = usage.begin(); usageIt != usage.end(); ++usageIt) {
			simplecpp::MacroUsage mu(usageIt->files, macro.valueDefinedInCode());
			mu.macroName = macro.name();
			mu.macroLocation = macro.defineLocation();
			mu.useLocation = *usageIt;
			rMacroUsageList.push_back(mu);
		}
	}
}

int simplecpp::Preprocess(PreprocessBlock & rBlk, const DUI & rDui, const TokenList & rSrcTokenList)
{
	int    ok = 1;
#ifdef SIMPLECPP_WINDOWS
	if(rDui.clearIncludeCache)
		nonExistingFilesCache.clear();
#endif
	std::map <std::string, std::size_t> sizeOfType(rSrcTokenList.sizeOfType);
	MakeSizeOfTypeMap(sizeOfType);
	std::vector<std::string> dummy; // use a dummy vector for the macros because as this is not part of the file and would add an empty entry - e.g. /usr/include/poll.h
	const bool hasInclude = isCpp17OrLater(rDui);
	//MacroMap macros;
	MakePrereqMacrosMap(rDui, rBlk.MacroList);
	if(!rDui.std.empty()) {
		const cstd_t c_std = simplecpp::getCStd(rDui.std);
		if(c_std != CUnknown) {
			const std::string std_def = simplecpp::getCStdString(c_std);
			if(!std_def.empty())
				rBlk.MacroList.insert(std::make_pair("__STDC_VERSION__", Macro("__STDC_VERSION__", std_def, dummy)));
		}
		else {
			const cppstd_t cpp_std = simplecpp::getCppStd(rDui.std);
			if(cpp_std == CPPUnknown) {
				SString msg_;
				msg_.Cat("unknown standard specified").CatDiv(':', 2).CatChar('\'').Cat(rDui.std.c_str()).CatChar('\'');
				MakeOutputMessage(&rBlk.MsgList, rBlk.FileList, simplecpp::Output::DUI_ERROR, 0, msg_);
				rBlk.OutputTokenList.clear();
				return 0;
			}
			const std::string std_def = simplecpp::getCppStdString(cpp_std);
			if(!std_def.empty())
				rBlk.MacroList.insert(std::make_pair("__cplusplus", Macro("__cplusplus", std_def, dummy)));
		}
	}
	// True => code in current #if block should be kept
	// ElseIsTrue => code in current #if block should be dropped. the code in the #else should be kept.
	// AlwaysFalse => drop all code in #if and #else
	enum IfState { 
		True, 
		ElseIsTrue, 
		AlwaysFalse 
	};
	std::stack<int> ifstates;
	ifstates.push(True);
	std::stack<const Token *> includetokenstack;
	std::set<std::string> pragmaOnce;
	includetokenstack.push(rSrcTokenList.cfront());
	for(std::list<std::string>::const_iterator it = rDui.includes.begin(); it != rDui.includes.end(); ++it) {
		const std::map<std::string, TokenList*>::const_iterator f = rBlk.FileData.find(*it);
		if(f != rBlk.FileData.end())
			includetokenstack.push(f->second->cfront());
	}
	std::map<std::string, std::list<Location> > maybeUsedMacros;
	for(const Token * rawtok = nullptr; rawtok || !includetokenstack.empty();) {
		if(rawtok == nullptr) {
			rawtok = includetokenstack.top();
			includetokenstack.pop();
			continue;
		}
		else {
			if(rawtok->op == '#' && !sameline(rawtok->previousSkipComments(), rawtok)) {
				if(!sameline(rawtok, rawtok->next)) {
					rawtok = rawtok->next;
					continue;
				}
				rawtok = rawtok->next;
				if(!rawtok->name) {
					rawtok = gotoNextLine(rawtok);
					continue;
				}
				if(ifstates.size() <= 1U && (rawtok->str() == ELIF || rawtok->str() == ELSE || rawtok->str() == ENDIF)) {
					SString msg_;
					msg_.CatChar('#').Cat(rawtok->str().c_str()).Space().Cat("without #if");
					MakeOutputMessage(&rBlk.MsgList, rBlk.FileList, simplecpp::Output::SYNTAX_ERROR, &rawtok->location, msg_);
					rBlk.OutputTokenList.clear();
					return 0;
				}
				if(ifstates.top() == True && (rawtok->str() == ERROR_ || rawtok->str() == WARNING)) {
					if(rBlk.Flags & PreprocessBlock::fMsgList) {
						simplecpp::Output err(rawtok->location.files);
						err.type = rawtok->str() == ERROR_ ? Output::ERROR_ : Output::WARNING;
						err.location = rawtok->location;
						for(const Token * tok = rawtok->next; tok && sameline(rawtok, tok); tok = tok->next) {
							if(!err.msg.empty() && isNameChar(tok->str()[0]))
								err.msg += ' ';
							err.msg += tok->str();
						}
						err.msg = '#' + rawtok->str() + ' ' + err.msg;
						rBlk.MsgList.push_back(err);
					}
					if(rawtok->str() == ERROR_) {
						rBlk.OutputTokenList.clear();
						return 0;
					}
				}
				if(rawtok->str() == DEFINE) {
					if(ifstates.top() != True)
						continue;
					try {
						const Macro & macro = Macro(rawtok->previous, rBlk.FileList);
						if(rDui.undefined.find(macro.name()) == rDui.undefined.end()) {
							const MacroMap::iterator it = rBlk.MacroList.find(macro.name());
							if(it == rBlk.MacroList.end())
								rBlk.MacroList.insert(std::pair<TokenString, Macro>(macro.name(), macro));
							else
								it->second = macro;
						}
					} catch(const std::runtime_error &) {
						MakeOutputMessage(&rBlk.MsgList, rBlk.FileList, simplecpp::Output::SYNTAX_ERROR, &rawtok->location, "Failed to parse #define");
						rBlk.OutputTokenList.clear();
						return 0;
					}
				}
				else if(ifstates.top() == True && rawtok->str() == INCLUDE) {
					TokenList inc1(rBlk.FileList);
					for(const Token * inctok = rawtok->next; sameline(rawtok, inctok); inctok = inctok->next) {
						if(!inctok->comment)
							inc1.push_back(new Token(*inctok));
					}
					TokenList inc2(rBlk.FileList);
					if(!inc1.empty() && inc1.cfront()->name) {
						const Token * inctok = inc1.cfront();
						if(!preprocessToken(inc2, &inctok, rBlk.MacroList, rBlk.FileList, &rBlk.MsgList)) {
							rBlk.OutputTokenList.clear();
							return 0;
						}
					}
					else {
						inc2.takeTokens(inc1);
					}
					if(!inc1.empty() && !inc2.empty() && inc2.cfront()->op == '<' && inc2.cback()->op == '>') {
						TokenString hdr;
						// TODO: Sometimes spaces must be added in the string
						// Somehow preprocessToken etc must be told that the location should be source location not destination location
						for(const Token * tok = inc2.cfront(); tok; tok = tok->next) {
							hdr += tok->str();
						}
						inc2.clear();
						inc2.push_back(new Token(hdr, inc1.cfront()->location));
						inc2.front()->op = '<';
					}

					if(inc2.empty() || inc2.cfront()->str().size() <= 2U) {
						MakeOutputMessage(&rBlk.MsgList, rBlk.FileList, simplecpp::Output::SYNTAX_ERROR, &rawtok->location, "No header in #include");
						rBlk.OutputTokenList.clear();
						return 0;
					}
					const Token * const inctok = inc2.cfront();
					const bool systemheader = (inctok->str()[0] == '<');
					const std::string header(realFilename(inctok->str().substr(1U, inctok->str().size() - 2U)));
					std::string header2 = getFileName(rBlk.FileData, rawtok->location.file(), header, rDui, systemheader);
					if(header2.empty()) {
						// try to load file..
						std::ifstream f;
						header2 = openHeader(f, rDui, rawtok->location.file(), header, systemheader);
						if(f.is_open()) {
							f.close();
							TokenList * const tokens = new TokenList(header2, rBlk.FileList, &rBlk.MsgList);
							if(rDui.removeComments)
								tokens->removeComments();
							rBlk.FileData[header2] = tokens;
						}
					}
					if(header2.empty()) {
						if(rBlk.Flags & PreprocessBlock::fMsgList) {
							simplecpp::Output out(rBlk.FileList);
							out.type = Output::MISSING_HEADER;
							out.location = rawtok->location;
							out.msg = "Header not found: " + inctok->str();
							rBlk.MsgList.push_back(out);
						}
					}
					else if(includetokenstack.size() >= 400) {
						if(rBlk.Flags & PreprocessBlock::fMsgList) {
							simplecpp::Output out(rBlk.FileList);
							out.type = Output::INCLUDE_NESTED_TOO_DEEPLY;
							out.location = rawtok->location;
							out.msg = "#include nested too deeply";
							rBlk.MsgList.push_back(out);
						}
					}
					else if(pragmaOnce.find(header2) == pragmaOnce.end()) {
						includetokenstack.push(gotoNextLine(rawtok));
						const TokenList * const includetokens = rBlk.FileData.find(header2)->second;
						rawtok = includetokens ? includetokens->cfront() : nullptr;
						continue;
					}
				}
				else if(rawtok->str() == IF || rawtok->str() == IFDEF || rawtok->str() == IFNDEF || rawtok->str() == ELIF) {
					if(!sameline(rawtok, rawtok->next)) {
						if(rBlk.Flags & PreprocessBlock::fMsgList) {
							simplecpp::Output out(rBlk.FileList);
							out.type = Output::SYNTAX_ERROR;
							out.location = rawtok->location;
							out.msg = "Syntax error in #" + rawtok->str();
							rBlk.MsgList.push_back(out);
						}
						rBlk.OutputTokenList.clear();
						return 0;
					}
					bool conditionIsTrue;
					if(ifstates.top() == AlwaysFalse || (ifstates.top() == ElseIsTrue && rawtok->str() != ELIF))
						conditionIsTrue = false;
					else if(rawtok->str() == IFDEF) {
						conditionIsTrue = (rBlk.MacroList.find(rawtok->next->str()) != rBlk.MacroList.end() || (hasInclude && rawtok->next->str() == HAS_INCLUDE));
						maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
					}
					else if(rawtok->str() == IFNDEF) {
						conditionIsTrue = (rBlk.MacroList.find(rawtok->next->str()) == rBlk.MacroList.end() && !(hasInclude && rawtok->next->str() == HAS_INCLUDE));
						maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
					}
					else { /*if (rawtok->str() == IF || rawtok->str() == ELIF)*/
						TokenList expr(rBlk.FileList);
						for(const Token * tok = rawtok->next; tok && tok->location.sameline(rawtok->location); tok = tok->next) {
							if(!tok->name) {
								expr.push_back(new Token(*tok));
								continue;
							}
							if(tok->str() == DEFINED) {
								tok = tok->next;
								const bool par = (tok && tok->op == '(');
								if(par)
									tok = tok->next;
								maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
								if(tok) {
									if(rBlk.MacroList.find(tok->str()) != rBlk.MacroList.end())
										expr.push_back(new Token("1", tok->location));
									else if(hasInclude && tok->str() == HAS_INCLUDE)
										expr.push_back(new Token("1", tok->location));
									else
										expr.push_back(new Token("0", tok->location));
								}
								if(par)
									tok = tok ? tok->next : nullptr;
								if(!tok || !sameline(rawtok, tok) || (par && tok->op != ')')) {
									if(rBlk.Flags & PreprocessBlock::fMsgList) {
										Output out(rawtok->location.files);
										out.type = Output::SYNTAX_ERROR;
										out.location = rawtok->location;
										out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
										rBlk.MsgList.push_back(out);
									}
									rBlk.OutputTokenList.clear();
									return 0;
								}
								continue;
							}
							if(hasInclude && tok->str() == HAS_INCLUDE) {
								tok = tok->next;
								const bool par = (tok && tok->op == '(');
								if(par)
									tok = tok->next;
								bool closingAngularBracket = false;
								if(tok) {
									const std::string &sourcefile = rawtok->location.file();
									const bool systemheader = (tok && tok->op == '<');
									std::string header;
									if(systemheader) {
										while((tok = tok->next) && tok->op != '>')
											header += tok->str();
										// cppcheck-suppress selfAssignment - platform-dependent implementation
										header = realFilename(header);
										if(tok && tok->op == '>')
											closingAngularBracket = true;
									}
									else {
										header = realFilename(tok->str().substr(1U, tok->str().size() - 2U));
										closingAngularBracket = true;
									}
									std::ifstream f;
									const std::string header2 = openHeader(f, rDui, sourcefile, header, systemheader);
									expr.push_back(new Token(header2.empty() ? "0" : "1", tok->location));
								}
								if(par)
									tok = tok ? tok->next : nullptr;
								if(!tok || !sameline(rawtok, tok) || (par && tok->op != ')') || (!closingAngularBracket)) {
									if(rBlk.Flags & PreprocessBlock::fMsgList) {
										Output out(rawtok->location.files);
										out.type = Output::SYNTAX_ERROR;
										out.location = rawtok->location;
										out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
										rBlk.MsgList.push_back(out);
									}
									rBlk.OutputTokenList.clear();
									return 0;
								}
								continue;
							}
							maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
							const Token * tmp = tok;
							if(!preprocessToken(expr, &tmp, rBlk.MacroList, rBlk.FileList, &rBlk.MsgList)) {
								rBlk.OutputTokenList.clear();
								return 0;
							}
							if(!tmp)
								break;
							tok = tmp->previous;
						}
						try {
							if(rBlk.Flags & PreprocessBlock::fIfCondList) {
								std::string E;
								for(const simplecpp::Token * tok = expr.cfront(); tok; tok = tok->next)
									E += (E.empty() ? "" : " ") + tok->str();
								const int64 result = evaluate(expr, rDui, sizeOfType);
								conditionIsTrue = (result != 0);
								rBlk.IfCondList.push_back(IfCond(rawtok->location, E, result));
							}
							else {
								const int64 result = evaluate(expr, rDui, sizeOfType);
								conditionIsTrue = (result != 0);
							}
						} catch(const std::exception &e) {
							if(rBlk.Flags & PreprocessBlock::fMsgList) {
								Output out(rawtok->location.files);
								out.type = Output::SYNTAX_ERROR;
								out.location = rawtok->location;
								out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
								if(e.what() && *e.what())
									out.msg += std::string(", ") + e.what();
								rBlk.MsgList.push_back(out);
							}
							rBlk.OutputTokenList.clear();
							return 0;
						}
					}
					if(rawtok->str() != ELIF) {
						// push a new ifstate..
						if(ifstates.top() != True)
							ifstates.push(AlwaysFalse);
						else
							ifstates.push(conditionIsTrue ? True : ElseIsTrue);
					}
					else if(ifstates.top() == True) {
						ifstates.top() = AlwaysFalse;
					}
					else if(ifstates.top() == ElseIsTrue && conditionIsTrue) {
						ifstates.top() = True;
					}
				}
				else if(rawtok->str() == ELSE) {
					ifstates.top() = (ifstates.top() == ElseIsTrue) ? True : AlwaysFalse;
				}
				else if(rawtok->str() == ENDIF) {
					ifstates.pop();
				}
				else if(rawtok->str() == UNDEF_) {
					if(ifstates.top() == True) {
						const Token * tok = rawtok->next;
						while(sameline(rawtok, tok) && tok->comment)
							tok = tok->next;
						if(sameline(rawtok, tok))
							rBlk.MacroList.erase(tok->str());
					}
				}
				else if(ifstates.top() == True && rawtok->str() == PRAGMA && rawtok->next && rawtok->next->str() == ONCE && sameline(rawtok, rawtok->next)) {
					pragmaOnce.insert(rawtok->location.file());
				}
				rawtok = gotoNextLine(rawtok);
				continue;
			}
			else if(ifstates.top() != True) {
				// drop code
				rawtok = gotoNextLine(rawtok);
				continue;
			}
			else {
				bool hash = false; // "#"
				bool hashhash = false; // "##"
				if(rawtok->op == '#' && sameline(rawtok, rawtok->next)) {
					if(rawtok->next->op != '#') {
						hash = true;
						rawtok = rawtok->next; // skip '#'
					}
					else if(sameline(rawtok, rawtok->next->next)) {
						hashhash = true;
						rawtok = rawtok->next->next; // skip '#' '#'
					}
				}
				const Location loc(rawtok->location);
				TokenList tokens(rBlk.FileList);
				if(!preprocessToken(tokens, &rawtok, rBlk.MacroList, rBlk.FileList, &rBlk.MsgList)) {
					rBlk.OutputTokenList.clear();
					return 0;
				}
				else if(hash || hashhash) {
					std::string s;
					for(const Token * hashtok = tokens.cfront(); hashtok; hashtok = hashtok->next)
						s += hashtok->str();
					if(hash)
						rBlk.OutputTokenList.push_back(new Token('\"' + s + '\"', loc));
					else if(rBlk.OutputTokenList.back())
						rBlk.OutputTokenList.back()->setstr(rBlk.OutputTokenList.cback()->str() + s);
					else
						rBlk.OutputTokenList.push_back(new Token(s, loc));
				}
				else {
					rBlk.OutputTokenList.takeTokens(tokens);
				}
			}
		}
	}
	if(rBlk.Flags & PreprocessBlock::fMacroUsageList) {
		PostprocessMacroUsageList(rBlk.MacroList, maybeUsedMacros, rBlk.MacroUsageList);
	}
	return ok;
}
//
//
//
void simplecpp::preprocess(simplecpp::TokenList & rOutput, const simplecpp::TokenList & rawtokens, std::vector<std::string> & files,
    std::map <std::string, simplecpp::TokenList *> & filedata, const simplecpp::DUI & rDui,
    simplecpp::OutputList * outputList, std::list<simplecpp::MacroUsage> * macroUsage, std::list<simplecpp::IfCond> * ifCond)
{
#ifdef SIMPLECPP_WINDOWS
	if(rDui.clearIncludeCache)
		nonExistingFilesCache.clear();
#endif
	std::map<std::string, std::size_t> sizeOfType(rawtokens.sizeOfType);
	MakeSizeOfTypeMap(sizeOfType);
	std::vector<std::string> dummy; // use a dummy vector for the macros because as this is not part of the file and would add an empty entry - e.g. /usr/include/poll.h
	const bool hasInclude = isCpp17OrLater(rDui);
	MacroMap macros;
	MakePrereqMacrosMap(rDui, macros);
	if(!rDui.std.empty()) {
		const cstd_t c_std = simplecpp::getCStd(rDui.std);
		if(c_std != CUnknown) {
			const std::string std_def = simplecpp::getCStdString(c_std);
			if(!std_def.empty())
				macros.insert(std::make_pair("__STDC_VERSION__", Macro("__STDC_VERSION__", std_def, dummy)));
		}
		else {
			const cppstd_t cpp_std = simplecpp::getCppStd(rDui.std);
			if(cpp_std == CPPUnknown) {
				SString msg_;
				msg_.Cat("unknown standard specified").CatDiv(':', 2).CatChar('\'').Cat(rDui.std.c_str()).CatChar('\'');
				MakeOutputMessage(outputList, files, simplecpp::Output::DUI_ERROR, 0, msg_);
				rOutput.clear();
				return;
			}
			const std::string std_def = simplecpp::getCppStdString(cpp_std);
			if(!std_def.empty())
				macros.insert(std::make_pair("__cplusplus", Macro("__cplusplus", std_def, dummy)));
		}
	}
	// True => code in current #if block should be kept
	// ElseIsTrue => code in current #if block should be dropped. the code in the #else should be kept.
	// AlwaysFalse => drop all code in #if and #else
	enum IfState { 
		True, 
		ElseIsTrue, 
		AlwaysFalse 
	};
	std::stack<int> ifstates;
	ifstates.push(True);
	std::stack<const Token *> includetokenstack;
	std::set<std::string> pragmaOnce;
	includetokenstack.push(rawtokens.cfront());
	for(std::list<std::string>::const_iterator it = rDui.includes.begin(); it != rDui.includes.end(); ++it) {
		const std::map<std::string, TokenList*>::const_iterator f = filedata.find(*it);
		if(f != filedata.end())
			includetokenstack.push(f->second->cfront());
	}
	std::map<std::string, std::list<Location> > maybeUsedMacros;
	for(const Token * rawtok = nullptr; rawtok || !includetokenstack.empty();) {
		if(rawtok == nullptr) {
			rawtok = includetokenstack.top();
			includetokenstack.pop();
			continue;
		}
		if(rawtok->op == '#' && !sameline(rawtok->previousSkipComments(), rawtok)) {
			if(!sameline(rawtok, rawtok->next)) {
				rawtok = rawtok->next;
				continue;
			}
			rawtok = rawtok->next;
			if(!rawtok->name) {
				rawtok = gotoNextLine(rawtok);
				continue;
			}
			if(ifstates.size() <= 1U && (rawtok->str() == ELIF || rawtok->str() == ELSE || rawtok->str() == ENDIF)) {
				SString msg_;
				msg_.CatChar('#').Cat(rawtok->str().c_str()).Space().Cat("without #if");
				MakeOutputMessage(outputList, files, simplecpp::Output::SYNTAX_ERROR, &rawtok->location, msg_);
				rOutput.clear();
				return;
			}
			if(ifstates.top() == True && (rawtok->str() == ERROR_ || rawtok->str() == WARNING)) {
				if(outputList) {
					simplecpp::Output err(rawtok->location.files);
					err.type = rawtok->str() == ERROR_ ? Output::ERROR_ : Output::WARNING;
					err.location = rawtok->location;
					for(const Token * tok = rawtok->next; tok && sameline(rawtok, tok); tok = tok->next) {
						if(!err.msg.empty() && isNameChar(tok->str()[0]))
							err.msg += ' ';
						err.msg += tok->str();
					}
					err.msg = '#' + rawtok->str() + ' ' + err.msg;
					outputList->push_back(err);
				}
				if(rawtok->str() == ERROR_) {
					rOutput.clear();
					return;
				}
			}
			if(rawtok->str() == DEFINE) {
				if(ifstates.top() != True)
					continue;
				try {
					const Macro & macro = Macro(rawtok->previous, files);
					if(rDui.undefined.find(macro.name()) == rDui.undefined.end()) {
						const MacroMap::iterator it = macros.find(macro.name());
						if(it == macros.end())
							macros.insert(std::pair<TokenString, Macro>(macro.name(), macro));
						else
							it->second = macro;
					}
				} catch(const std::runtime_error &) {
					MakeOutputMessage(outputList, files, simplecpp::Output::SYNTAX_ERROR, &rawtok->location, "Failed to parse #define");
					rOutput.clear();
					return;
				}
			}
			else if(ifstates.top() == True && rawtok->str() == INCLUDE) {
				TokenList inc1(files);
				for(const Token * inctok = rawtok->next; sameline(rawtok, inctok); inctok = inctok->next) {
					if(!inctok->comment)
						inc1.push_back(new Token(*inctok));
				}
				TokenList inc2(files);
				if(!inc1.empty() && inc1.cfront()->name) {
					const Token * inctok = inc1.cfront();
					if(!preprocessToken(inc2, &inctok, macros, files, outputList)) {
						rOutput.clear();
						return;
					}
				}
				else {
					inc2.takeTokens(inc1);
				}

				if(!inc1.empty() && !inc2.empty() && inc2.cfront()->op == '<' && inc2.cback()->op == '>') {
					TokenString hdr;
					// TODO: Sometimes spaces must be added in the string
					// Somehow preprocessToken etc must be told that the location should be source location not destination location
					for(const Token * tok = inc2.cfront(); tok; tok = tok->next) {
						hdr += tok->str();
					}
					inc2.clear();
					inc2.push_back(new Token(hdr, inc1.cfront()->location));
					inc2.front()->op = '<';
				}

				if(inc2.empty() || inc2.cfront()->str().size() <= 2U) {
					MakeOutputMessage(outputList, files, simplecpp::Output::SYNTAX_ERROR, &rawtok->location, "No header in #include");
					rOutput.clear();
					return;
				}
				const Token * const inctok = inc2.cfront();
				const bool systemheader = (inctok->str()[0] == '<');
				const std::string header(realFilename(inctok->str().substr(1U, inctok->str().size() - 2U)));
				std::string header2 = getFileName(filedata, rawtok->location.file(), header, rDui, systemheader);
				if(header2.empty()) {
					// try to load file..
					std::ifstream f;
					header2 = openHeader(f, rDui, rawtok->location.file(), header, systemheader);
					if(f.is_open()) {
						f.close();
						TokenList * const tokens = new TokenList(header2, files, outputList);
						if(rDui.removeComments)
							tokens->removeComments();
						filedata[header2] = tokens;
					}
				}
				if(header2.empty()) {
					if(outputList) {
						simplecpp::Output out(files);
						out.type = Output::MISSING_HEADER;
						out.location = rawtok->location;
						out.msg = "Header not found: " + inctok->str();
						outputList->push_back(out);
					}
				}
				else if(includetokenstack.size() >= 400) {
					if(outputList) {
						simplecpp::Output out(files);
						out.type = Output::INCLUDE_NESTED_TOO_DEEPLY;
						out.location = rawtok->location;
						out.msg = "#include nested too deeply";
						outputList->push_back(out);
					}
				}
				else if(pragmaOnce.find(header2) == pragmaOnce.end()) {
					includetokenstack.push(gotoNextLine(rawtok));
					const TokenList * const includetokens = filedata.find(header2)->second;
					rawtok = includetokens ? includetokens->cfront() : nullptr;
					continue;
				}
			}
			else if(rawtok->str() == IF || rawtok->str() == IFDEF || rawtok->str() == IFNDEF || rawtok->str() == ELIF) {
				if(!sameline(rawtok, rawtok->next)) {
					if(outputList) {
						simplecpp::Output out(files);
						out.type = Output::SYNTAX_ERROR;
						out.location = rawtok->location;
						out.msg = "Syntax error in #" + rawtok->str();
						outputList->push_back(out);
					}
					rOutput.clear();
					return;
				}
				bool conditionIsTrue;
				if(ifstates.top() == AlwaysFalse || (ifstates.top() == ElseIsTrue && rawtok->str() != ELIF))
					conditionIsTrue = false;
				else if(rawtok->str() == IFDEF) {
					conditionIsTrue = (macros.find(rawtok->next->str()) != macros.end() || (hasInclude && rawtok->next->str() == HAS_INCLUDE));
					maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
				}
				else if(rawtok->str() == IFNDEF) {
					conditionIsTrue = (macros.find(rawtok->next->str()) == macros.end() && !(hasInclude && rawtok->next->str() == HAS_INCLUDE));
					maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
				}
				else { /*if (rawtok->str() == IF || rawtok->str() == ELIF)*/
					TokenList expr(files);
					for(const Token * tok = rawtok->next; tok && tok->location.sameline(rawtok->location); tok = tok->next) {
						if(!tok->name) {
							expr.push_back(new Token(*tok));
							continue;
						}
						if(tok->str() == DEFINED) {
							tok = tok->next;
							const bool par = (tok && tok->op == '(');
							if(par)
								tok = tok->next;
							maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
							if(tok) {
								if(macros.find(tok->str()) != macros.end())
									expr.push_back(new Token("1", tok->location));
								else if(hasInclude && tok->str() == HAS_INCLUDE)
									expr.push_back(new Token("1", tok->location));
								else
									expr.push_back(new Token("0", tok->location));
							}
							if(par)
								tok = tok ? tok->next : nullptr;
							if(!tok || !sameline(rawtok, tok) || (par && tok->op != ')')) {
								if(outputList) {
									Output out(rawtok->location.files);
									out.type = Output::SYNTAX_ERROR;
									out.location = rawtok->location;
									out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
									outputList->push_back(out);
								}
								rOutput.clear();
								return;
							}
							continue;
						}
						if(hasInclude && tok->str() == HAS_INCLUDE) {
							tok = tok->next;
							const bool par = (tok && tok->op == '(');
							if(par)
								tok = tok->next;
							bool closingAngularBracket = false;
							if(tok) {
								const std::string &sourcefile = rawtok->location.file();
								const bool systemheader = (tok && tok->op == '<');
								std::string header;
								if(systemheader) {
									while((tok = tok->next) && tok->op != '>')
										header += tok->str();
									// cppcheck-suppress selfAssignment - platform-dependent implementation
									header = realFilename(header);
									if(tok && tok->op == '>')
										closingAngularBracket = true;
								}
								else {
									header = realFilename(tok->str().substr(1U, tok->str().size() - 2U));
									closingAngularBracket = true;
								}
								std::ifstream f;
								const std::string header2 = openHeader(f, rDui, sourcefile, header, systemheader);
								expr.push_back(new Token(header2.empty() ? "0" : "1", tok->location));
							}
							if(par)
								tok = tok ? tok->next : nullptr;
							if(!tok || !sameline(rawtok, tok) || (par && tok->op != ')') || (!closingAngularBracket)) {
								if(outputList) {
									Output out(rawtok->location.files);
									out.type = Output::SYNTAX_ERROR;
									out.location = rawtok->location;
									out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
									outputList->push_back(out);
								}
								rOutput.clear();
								return;
							}
							continue;
						}
						maybeUsedMacros[rawtok->next->str()].push_back(rawtok->next->location);
						const Token * tmp = tok;
						if(!preprocessToken(expr, &tmp, macros, files, outputList)) {
							rOutput.clear();
							return;
						}
						if(!tmp)
							break;
						tok = tmp->previous;
					}
					try {
						if(ifCond) {
							std::string E;
							for(const simplecpp::Token * tok = expr.cfront(); tok; tok = tok->next)
								E += (E.empty() ? "" : " ") + tok->str();
							const int64 result = evaluate(expr, rDui, sizeOfType);
							conditionIsTrue = (result != 0);
							ifCond->push_back(IfCond(rawtok->location, E, result));
						}
						else {
							const int64 result = evaluate(expr, rDui, sizeOfType);
							conditionIsTrue = (result != 0);
						}
					} catch(const std::exception &e) {
						if(outputList) {
							Output out(rawtok->location.files);
							out.type = Output::SYNTAX_ERROR;
							out.location = rawtok->location;
							out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
							if(e.what() && *e.what())
								out.msg += std::string(", ") + e.what();
							outputList->push_back(out);
						}
						rOutput.clear();
						return;
					}
				}
				if(rawtok->str() != ELIF) {
					// push a new ifstate..
					if(ifstates.top() != True)
						ifstates.push(AlwaysFalse);
					else
						ifstates.push(conditionIsTrue ? True : ElseIsTrue);
				}
				else if(ifstates.top() == True) {
					ifstates.top() = AlwaysFalse;
				}
				else if(ifstates.top() == ElseIsTrue && conditionIsTrue) {
					ifstates.top() = True;
				}
			}
			else if(rawtok->str() == ELSE) {
				ifstates.top() = (ifstates.top() == ElseIsTrue) ? True : AlwaysFalse;
			}
			else if(rawtok->str() == ENDIF) {
				ifstates.pop();
			}
			else if(rawtok->str() == UNDEF_) {
				if(ifstates.top() == True) {
					const Token * tok = rawtok->next;
					while(sameline(rawtok, tok) && tok->comment)
						tok = tok->next;
					if(sameline(rawtok, tok))
						macros.erase(tok->str());
				}
			}
			else if(ifstates.top() == True && rawtok->str() == PRAGMA && rawtok->next && rawtok->next->str() == ONCE && sameline(rawtok, rawtok->next)) {
				pragmaOnce.insert(rawtok->location.file());
			}
			rawtok = gotoNextLine(rawtok);
			continue;
		}
		if(ifstates.top() != True) {
			// drop code
			rawtok = gotoNextLine(rawtok);
			continue;
		}
		{
			bool hash = false; // "#"
			bool hashhash = false; // "##"
			if(rawtok->op == '#' && sameline(rawtok, rawtok->next)) {
				if(rawtok->next->op != '#') {
					hash = true;
					rawtok = rawtok->next; // skip '#'
				}
				else if(sameline(rawtok, rawtok->next->next)) {
					hashhash = true;
					rawtok = rawtok->next->next; // skip '#' '#'
				}
			}
			const Location loc(rawtok->location);
			TokenList tokens(files);
			if(!preprocessToken(tokens, &rawtok, macros, files, outputList)) {
				rOutput.clear();
				return;
			}
			if(hash || hashhash) {
				std::string s;
				for(const Token * hashtok = tokens.cfront(); hashtok; hashtok = hashtok->next)
					s += hashtok->str();
				if(hash)
					rOutput.push_back(new Token('\"' + s + '\"', loc));
				else if(rOutput.back())
					rOutput.back()->setstr(rOutput.cback()->str() + s);
				else
					rOutput.push_back(new Token(s, loc));
			}
			else {
				rOutput.takeTokens(tokens);
			}
		}
	}
	if(macroUsage) {
		PostprocessMacroUsageList(macros, maybeUsedMacros, *macroUsage);
	}
}

void simplecpp::cleanup(std::map<std::string, TokenList*> &filedata)
{
	for(std::map<std::string, TokenList*>::iterator it = filedata.begin(); it != filedata.end(); ++it)
		delete it->second;
	filedata.clear();
}

simplecpp::cstd_t simplecpp::getCStd(const std::string &std)
{
	if(std == "c90" || std == "c89" || std == "iso9899:1990" || std == "iso9899:199409" || std == "gnu90" || std == "gnu89")
		return C89;
	if(std == "c99" || std == "c9x" || std == "iso9899:1999" || std == "iso9899:199x" || std == "gnu99"|| std == "gnu9x")
		return C99;
	if(std == "c11" || std == "c1x" || std == "iso9899:2011" || std == "gnu11" || std == "gnu1x")
		return C11;
	if(std == "c17" || std == "c18" || std == "iso9899:2017" || std == "iso9899:2018" || std == "gnu17"|| std == "gnu18")
		return C17;
	if(std == "c23" || std == "gnu23" || std == "c2x" || std == "gnu2x")
		return C23;
	return CUnknown;
}

std::string simplecpp::getCStdString(cstd_t std)
{
	const char * p_result = "";
	switch(std) {
		case C89: p_result = ""; break; // __STDC_VERSION__ is not set for C90 although the macro was added in the 1994 amendments
		case C99: p_result = "199901L"; break;
		case C11: p_result = "201112L"; break;
		case C17: p_result = "201710L"; break;
		case C23:
		    // supported by GCC 9+ and Clang 9+
		    // Clang 9, 10, 11, 12, 13 return "201710L"
		    // Clang 14, 15, 16, 17 return "202000L"
		    // Clang 9, 10, 11, 12, 13, 14, 15, 16, 17 do not support "c23" and "gnu23"
		    p_result = "202311L";
			break;
		//case CUnknown: p_result = ""; break;
	}
	return p_result;
}

std::string simplecpp::getCStdString(const std::string &std) { return getCStdString(getCStd(std)); }

simplecpp::cppstd_t simplecpp::getCppStd(const std::string & std)
{
	if(std == "c++98" || std == "c++03" || std == "gnu++98" || std == "gnu++03")
		return CPP03;
	if(std == "c++11" || std == "gnu++11" || std == "c++0x" || std == "gnu++0x")
		return CPP11;
	if(std == "c++14" || std == "c++1y" || std == "gnu++14" || std == "gnu++1y")
		return CPP14;
	if(std == "c++17" || std == "c++1z" || std == "gnu++17" || std == "gnu++1z")
		return CPP17;
	if(std == "c++20" || std == "c++2a" || std == "gnu++20" || std == "gnu++2a")
		return CPP20;
	if(std == "c++23" || std == "c++2b" || std == "gnu++23" || std == "gnu++2b")
		return CPP23;
	if(std == "c++26" || std == "c++2c" || std == "gnu++26" || std == "gnu++2c")
		return CPP26;
	return CPPUnknown;
}

std::string simplecpp::getCppStdString(cppstd_t std)
{
	const char * p_result = "";
	switch(std) {
		case CPP03: p_result = "199711L"; break;
		case CPP11: p_result = "201103L"; break;
		case CPP14: p_result = "201402L"; break;
		case CPP17: p_result = "201703L"; break;
		case CPP20: p_result = "202002L"; break; // GCC 10 returns "201703L" - correct in 11+
		case CPP23:
		    // supported by GCC 11+ and Clang 12+
		    // GCC 11, 12, 13 return "202100L"
		    // Clang 12, 13, 14, 15, 16 do not support "c++23" and "gnu++23" and return "202101L"
		    // Clang 17, 18 return "202302L"
		    p_result = "202302L"; 
			break;
		case CPP26: p_result = "202400L"; break; // supported by Clang 17+
		// case CPPUnknown: p_result = ""; break;
	}
	return p_result;
}

std::string simplecpp::getCppStdString(const std::string & std) { return getCppStdString(getCppStd(std)); }

void SimpleCppPreprocess(simplecpp::TokenList & rOutput, const simplecpp::TokenList & rRawTokens, std::vector <std::string> & rFiles,
	std::map <std::string, simplecpp::TokenList *> & rFileData, const simplecpp::DUI & rDui, simplecpp::OutputList * pOutputList, 
	std::list <simplecpp::MacroUsage> * pMacroUsage, std::list<simplecpp::IfCond> * pIfCond)
{
	simplecpp::preprocess(rOutput, rRawTokens, rFiles, rFileData, rDui, pOutputList, pMacroUsage, pIfCond);
}

#if (__cplusplus < 201103L) && !defined(__APPLE__)
	#undef nullptr
#endif
//
// Descr: В оригинале - main() 
//   Головная процедура для реализации исполняемого модуля C++-препроцессора. Оставлена здесь в справочных целях.
//
int SimpleCpp_Main(int argc, char ** argv) 
{
	bool error = false;
	const char * filename = nullptr;
	bool use_istream = false;
	// Settings..
	simplecpp::DUI dui;
	bool quiet = false;
	bool error_only = false;
	for(int i = 1; i < argc; i++) {
		const char * const arg = argv[i];
		if(*arg == '-') {
			bool found = false;
			const char c = arg[1];
			switch(c) {
				case 'D': { // define symbol
				    const char * const value = arg[2] ? (argv[i] + 2) : argv[++i];
				    dui.defines.push_back(value);
				    found = true;
				    break;
			    }
				case 'U': { // undefine symbol
				    const char * const value = arg[2] ? (argv[i] + 2) : argv[++i];
				    dui.undefined.insert(value);
				    found = true;
				    break;
			    }
				case 'I': { // include path
				    const char * const value = arg[2] ? (argv[i] + 2) : argv[++i];
				    dui.includePaths.push_back(value);
				    found = true;
				    break;
			    }
				case 'i':
				    if(std::strncmp(arg, "-include=", 9)==0) {
					    dui.includes.push_back(arg+9);
					    found = true;
				    }
				    else if(std::strncmp(arg, "-is", 3)==0) {
					    use_istream = true;
					    found = true;
				    }
				    break;
				case 's':
				    if(std::strncmp(arg, "-std=", 5)==0) {
					    dui.std = arg + 5;
					    found = true;
				    }
				    break;
				case 'q':
				    quiet = true;
				    found = true;
				    break;
				case 'e':
				    error_only = true;
				    found = true;
				    break;
			}
			if(!found) {
				std::cout << "error: option '" << arg << "' is unknown." << std::endl;
				error = true;
			}
		}
		else if(filename) {
			std::cout << "error: multiple filenames specified" << std::endl;
			std::exit(1);
		}
		else {
			filename = arg;
		}
	}
	if(error)
		std::exit(1);
	if(quiet && error_only) {
		std::cout << "error: -e cannot be used in conjunction with -q" << std::endl;
		std::exit(1);
	}
	if(!filename) {
		std::cout << "Syntax:" << std::endl;
		std::cout << "simplecpp [options] filename" << std::endl;
		std::cout << "  -DNAME          Define NAME." << std::endl;
		std::cout << "  -IPATH          Include path." << std::endl;
		std::cout << "  -include=FILE   Include FILE." << std::endl;
		std::cout << "  -UNAME          Undefine NAME." << std::endl;
		std::cout << "  -std=STD        Specify standard." << std::endl;
		std::cout << "  -q              Quiet mode (no output)." << std::endl;
		std::cout << "  -is             Use std::istream interface." << std::endl;
		std::cout << "  -e              Output errors only." << std::endl;
		std::exit(0);
	}
	dui.removeComments = true;
	// Perform preprocessing
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	simplecpp::TokenList * rawtokens;
	if(use_istream) {
		std::ifstream f(filename);
		if(!f.is_open()) {
			std::cout << "error: could not open file '" << filename << "'" << std::endl;
			std::exit(1);
		}
		rawtokens = new simplecpp::TokenList(f, files, filename, &outputList);
	}
	else {
		rawtokens = new simplecpp::TokenList(filename, files, &outputList);
	}
	rawtokens->removeComments();
	simplecpp::TokenList outputTokens(files);
	std::map<std::string, simplecpp::TokenList*> filedata;
	simplecpp::preprocess(outputTokens, *rawtokens, files, filedata, dui, &outputList);
	simplecpp::cleanup(filedata);
	ZDELETE(rawtokens);
	// Output
	if(!quiet) {
		if(!error_only)
			std::cout << outputTokens.stringify() << std::endl;
		for(const simplecpp::Output &output : outputList) {
			std::cerr << output.location.file() << ':' << output.location.line << ": ";
			switch(output.type) {
				case simplecpp::Output::ERROR_:
				    std::cerr << "#error: ";
				    break;
				case simplecpp::Output::WARNING:
				    std::cerr << "#warning: ";
				    break;
				case simplecpp::Output::MISSING_HEADER:
				    std::cerr << "missing header: ";
				    break;
				case simplecpp::Output::INCLUDE_NESTED_TOO_DEEPLY:
				    std::cerr << "include nested too deeply: ";
				    break;
				case simplecpp::Output::SYNTAX_ERROR:
				    std::cerr << "syntax error: ";
				    break;
				case simplecpp::Output::PORTABILITY_BACKSLASH:
				    std::cerr << "portability: ";
				    break;
				case simplecpp::Output::UNHANDLED_CHAR_ERROR:
				    std::cerr << "unhandled char error: ";
				    break;
				case simplecpp::Output::EXPLICIT_INCLUDE_NOT_FOUND:
				    std::cerr << "explicit include not found: ";
				    break;
				case simplecpp::Output::FILE_NOT_FOUND:
				    std::cerr << "file not found: ";
				    break;
				case simplecpp::Output::DUI_ERROR:
				    std::cerr << "dui error: ";
				    break;
			}
			std::cerr << output.msg << std::endl;
		}
	}
	return 0;
}
