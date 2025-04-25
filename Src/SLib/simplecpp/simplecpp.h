/* -*- C++ -*-
 * simplecpp - A simple and high-fidelity C/C++ preprocessor library
 * Copyright (C) 2016-2023 simplecpp team
 */

#ifndef simplecppH
#define simplecppH

#include <cctype>
#include <cstring>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#if (__cplusplus < 201103L) && !defined(__APPLE__)
	#define nullptr NULL
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
// suppress warnings about "conversion from 'type1' to 'type2', possible loss of data"
#  pragma warning(disable : 4267)
#  pragma warning(disable : 4244)
#endif

namespace simplecpp {
	/** C code standard */
	enum cstd_t { 
		CUnknown = -1, 
		C89, 
		C99, 
		C11, 
		C17, 
		C23 
	};
	/** C++ code standard */
	enum cppstd_t { 
		CPPUnknown = -1, 
		CPP03, 
		CPP11, 
		CPP14, 
		CPP17, 
		CPP20, 
		CPP23, 
		CPP26 
	};

	typedef std::string TokenString;
	class Macro;

	/**
	 * Location in source code
	 */
	class Location {
	public:
		explicit Location(const std::vector<std::string> &f) : files(f), fileIndex(0), line(1U), col(0U) 
		{
		}
		Location(const Location &loc) : files(loc.files), fileIndex(loc.fileIndex), line(loc.line), col(loc.col) 
		{
		}
		Location &operator=(const Location &other) 
		{
			if(this != &other) {
				fileIndex = other.fileIndex;
				line = other.line;
				col  = other.col;
			}
			return *this;
		}
		/** increment this location by string */
		void adjust(const std::string &str);
		bool operator<(const Location &rhs) const 
		{
			if(fileIndex != rhs.fileIndex)
				return fileIndex < rhs.fileIndex;
			if(line != rhs.line)
				return line < rhs.line;
			return col < rhs.col;
		}
		bool sameline(const Location &other) const { return fileIndex == other.fileIndex && line == other.line; }
		const std::string& file() const { return fileIndex < files.size() ? files[fileIndex] : emptyFileName; }

		const std::vector<std::string> &files;
		uint fileIndex;
		uint line;
		uint col;
	private:
		static const std::string emptyFileName;
	};
	/**
	 * token class.
	 * @todo don't use std::string representation - for both memory and performance reasons
	 */
	class Token {
	public:
		Token(const TokenString &s, const Location &loc, bool wsahead = false) : whitespaceahead(wsahead), location(loc), previous(nullptr), next(nullptr), string(s) 
		{
			flags();
		}
		Token(const Token &tok) : macro(tok.macro), op(tok.op), comment(tok.comment), name(tok.name), number(tok.number), 
			whitespaceahead(tok.whitespaceahead), location(tok.location), previous(nullptr), next(nullptr), string(tok.string), mExpandedFrom(tok.mExpandedFrom) 
		{
		}
		void flags() 
		{
			name = (std::isalpha(static_cast<uchar>(string[0])) || string[0] == '_' || string[0] == '$')
				&& (smemchr(string.c_str(), '\'', string.size()) == nullptr);
			comment = string.size() > 1U && string[0] == '/' && (string[1] == '/' || string[1] == '*');
			number = isNumberLike(string);
			op = (string.size() == 1U && !name && !comment && !number) ? string[0] : '\0';
		}
		const TokenString & str() const { return string; }
		void setstr(const std::string &s) 
		{
			string = s;
			flags();
		}
		bool isOneOf(const char ops[]) const;
		bool startsWithOneOf(const char c[]) const;
		bool endsWithOneOf(const char c[]) const;
		static bool isNumberLike(const std::string& str) { return isdec(str[0]) || (str.size() > 1U && (str[0] == '-' || str[0] == '+') && isdec(str[1])); }

		TokenString macro;
		Location location;
		Token * previous;
		Token * next;
		char   op;
		bool   comment;
		bool   name;
		bool   number;
		bool   whitespaceahead;
		uint8  Reserve[3]; // @alignment

		const Token * previousSkipComments() const;
		const Token * nextSkipComments() const;
		void setExpandedFrom(const Token * tok, const Macro* m);
		bool isExpandedFrom(const Macro* m) const { return mExpandedFrom.find(m) != mExpandedFrom.end(); }
		void printAll() const;
		void printOut() const;
	private:
		TokenString string;
		std::set<const Macro*> mExpandedFrom;
		// Not implemented - prevent assignment
		Token & operator = (const Token &tok);
	};

	/** Output from preprocessor */
	struct Output {
		enum Type {
			ERROR_, /* #error */
			WARNING, /* #warning */
			MISSING_HEADER,
			INCLUDE_NESTED_TOO_DEEPLY,
			SYNTAX_ERROR,
			PORTABILITY_BACKSLASH,
			UNHANDLED_CHAR_ERROR,
			EXPLICIT_INCLUDE_NOT_FOUND,
			FILE_NOT_FOUND,
			DUI_ERROR
		} type;

		explicit Output(const std::vector<std::string> &files) : type(ERROR_), location(files) 
		{
		}
		explicit Output(const std::vector<std::string>& files, Type type, const std::string& msg) : type(type), location(files), msg(msg) 
		{
		}
		Location location;
		std::string msg;
	};

	typedef std::list<Output> OutputList;

	/** List of tokens. */
	class TokenList {
	public:
		class Stream;

		explicit TokenList(std::vector<std::string> &filenames);
		/** generates a token list from the given std::istream parameter */
		TokenList(std::istream &istr, std::vector<std::string> &filenames, const std::string &filename = std::string(), OutputList * outputList = nullptr);
		/** generates a token list from the given buffer */
		TokenList(const uchar * data, std::size_t size, std::vector<std::string> &filenames, const std::string &filename = std::string(), OutputList * outputList = nullptr);
		/** generates a token list from the given buffer */
		TokenList(const char* data, std::size_t size, std::vector<std::string> &filenames, const std::string &filename = std::string(), OutputList * outputList = nullptr);
		/** generates a token list from the given filename parameter */
		TokenList(const std::string &filename, std::vector<std::string> &filenames, OutputList * outputList = nullptr);
		TokenList(const TokenList &other);
	#if __cplusplus >= 201103L
		TokenList(TokenList &&other);
	#endif
		~TokenList();
		TokenList & operator = (const TokenList & other);
	#if __cplusplus >= 201103L
		TokenList &operator=(TokenList &&other);
	#endif

		void clear();
		bool empty() const { return !frontToken; }
		void push_back(Token * tok);
		void dump() const;
		std::string stringify() const;
		void readfile(Stream &stream, const std::string &filename = std::string(), OutputList * outputList = nullptr);
		void constFold();
		void removeComments();
		Token * front() { return frontToken; }
		const Token * cfront() const { return frontToken; }
		Token * back() { return backToken; }
		const Token * cback() const { return backToken; }
		void FASTCALL deleteToken(Token * pTok);
		void FASTCALL takeTokens(TokenList & other);
		/** sizeof(T) */
		std::map<std::string, std::size_t> sizeOfType;
		const std::vector<std::string>& getFiles() const { return files; }
	private:
		void combineOperators();
		void constFoldUnaryNotPosNeg(Token * tok);
		void constFoldMulDivRem(Token * tok);
		void constFoldAddSub(Token * tok);
		void constFoldShift(Token * tok);
		void constFoldComparison(Token * tok);
		void constFoldBitwise(Token * tok);
		void constFoldLogicalOp(Token * tok);
		void constFoldQuestionOp(Token ** tok1);
		std::string readUntil(Stream &stream, const Location &location, char start, char end, OutputList * outputList);
		void lineDirective(uint fileIndex, uint line, Location * location);
		std::string lastLine(int maxsize = 1000) const;
		const Token* lastLineTok(int maxsize = 1000) const;
		bool isLastLinePreprocessor(int maxsize = 1000) const;
		uint fileIndex(const std::string &filename);

		Token * frontToken;
		Token * backToken;
		std::vector<std::string> & files;
	};

	/** Tracking how macros are used */
	struct MacroUsage {
		explicit MacroUsage(const std::vector<std::string> &f, bool macroValueKnown_) : macroLocation(f), useLocation(f), macroValueKnown(macroValueKnown_) 
		{
		}
		std::string macroName;
		Location macroLocation;
		Location useLocation;
		bool macroValueKnown;
	};

	/** Tracking #if/#elif expressions */
	struct IfCond {
		explicit IfCond(const Location& location, const std::string &E, int64 result) : location(location), E(E), result(result) 
		{
		}
		Location location; // location of #if/#elif
		std::string E; // preprocessed condition
		int64 result; // condition result
	};
	/**
	 * Command line preprocessor settings.
	 * On the command line these are configured by -D, -U, -I, --include, -std
	 */
	struct DUI {
		DUI() : clearIncludeCache(false), removeComments(false) 
		{
		}
		std::list<std::string> defines;
		std::set<std::string> undefined;
		std::list<std::string> includePaths;
		std::list<std::string> includes;
		std::string std;
		bool clearIncludeCache;
		bool removeComments; /** remove comment tokens from included files */
	};

	int64 characterLiteralToLL(const std::string& str);
	std::map<std::string, TokenList*> load(const TokenList &rawtokens, std::vector<std::string> &filenames, const DUI &dui, OutputList * outputList = nullptr);

	/**
	 * Preprocess
	 * @todo simplify interface
	 * @param output TokenList that receives the preprocessing output
	 * @param rawtokens Raw tokenlist for top sourcefile
	 * @param files internal data of simplecpp
	 * @param filedata output from simplecpp::load()
	 * @param dui defines, undefs, and include paths
	 * @param outputList output: list that will receive output messages
	 * @param macroUsage output: macro usage
	 * @param ifCond output: #if/#elif expressions
	 */
	void preprocess(TokenList & rOutput, const TokenList & rawtokens, std::vector <std::string> & files,
		std::map<std::string, TokenList*> & filedata, const DUI & dui, OutputList * outputList = nullptr, std::list<MacroUsage> * macroUsage = nullptr, std::list<IfCond> * ifCond = nullptr);

	struct PreprocessBlock {
		PreprocessBlock();
		TokenList MakeSourceTokenList(const char * pSrcBuf, const char * pSrcName);

		enum {
			fOutputTokenList = 0x0001,
			fMsgList         = 0x0002,
			fMacroUsageList  = 0x0004,
			fIfCondList      = 0x0008,
		};
		uint   Flags;
		std::vector <std::string> FileList;
		std::map <std::string, TokenList *> FileData;
		TokenList  OutputTokenList;
		OutputList MsgList;
		std::list <MacroUsage> MacroUsageList;
		std::list <IfCond> IfCondList;
	};
	void   ImplementPreprocess(PreprocessBlock & rBlk, const DUI & rDui, const TokenList & rSrcTokenList);
	/**
	 * Deallocate data
	 */
	void cleanup(std::map<std::string, TokenList*> &filedata);
	/** Simplify path */
	std::string simplifyPath(std::string path);
	/** Convert Cygwin path to Windows path */
	std::string convertCygwinToWindowsPath(const std::string &cygwinPath);
	/** Returns the C version a given standard */
	cstd_t getCStd(const std::string &std);
	/** Returns the C++ version a given standard */
	cppstd_t getCppStd(const std::string &std);
	/** Returns the __STDC_VERSION__ value for a given standard */
	std::string getCStdString(const std::string &std);
	std::string getCStdString(cstd_t std);
	/** Returns the __cplusplus value for a given standard */
	std::string getCppStdString(const std::string &std);
	std::string getCppStdString(cppstd_t std);
}

void SimpleCppPreprocess(simplecpp::TokenList & rOutput, const simplecpp::TokenList & rRawTokens, std::vector <std::string> & rFiles,
	std::map <std::string, simplecpp::TokenList *> & rFileData, 
	const simplecpp::DUI & rDui, simplecpp::OutputList * pOutputList = nullptr, 
	std::list <simplecpp::MacroUsage> * pMacroUsage = nullptr, std::list<simplecpp::IfCond> * pIfCond = nullptr);

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif
#if (__cplusplus < 201103L) && !defined(__APPLE__)
	#undef nullptr
#endif
#endif
