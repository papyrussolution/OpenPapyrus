// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (c) 1997-2016, International Business Machines Corporation and others. All Rights Reserved.
//
#include <icu-internal.h>
#pragma hdrstop
/**
 * IntlTest is a base class for tests.
 */
#include "unicode/ctest.h" // for str_timeDelta
#include "unicode/ucnv.h"
#include "intltest.h"
#include "caltztst.h"
#include "itmajor.h"
#include "lstmbe.h"
#include "putilimp.h" // for uprv_getRawUTCtime()
#include "udbgutil.h"
#include "uoptions.h"
#include "number_decnum.h"
#ifdef XP_MAC_CONSOLE
	#include <console.h>
	#include "Files.h"
#endif

static char * _testDataPath = NULL;

// Static list of errors found
static UnicodeString errorList;
static void * knownList = NULL; // known issues
static bool noKnownIssues = FALSE; // if TRUE, don't emit known issues

//-----------------------------------------------------------------------------
//convenience classes to ease porting code that uses the Java
//string-concatenation operator (moved from findword test by rtg)

// [LIU] Just to get things working
UnicodeString UCharToUnicodeString(char16_t c)
{
	return UnicodeString(c);
}

// [rtg] Just to get things working
UnicodeString operator+(const UnicodeString & left, long num)
{
	char buffer[64]; // nos changed from 10 to 64
	char danger = 'p'; // guard against overrunning the buffer (rtg)
	sprintf(buffer, "%ld", num);
	assert(danger == 'p');
	return left + buffer;
}

UnicodeString operator+(const UnicodeString & left, unsigned long num)
{
	char buffer[64]; // nos changed from 10 to 64
	char danger = 'p'; // guard against overrunning the buffer (rtg)
	sprintf(buffer, "%lu", num);
	assert(danger == 'p');
	return left + buffer;
}

UnicodeString Int64ToUnicodeString(int64_t num)
{
	char buffer[64]; // nos changed from 10 to 64
	char danger = 'p'; // guard against overrunning the buffer (rtg)
#if defined(_MSC_VER)
	sprintf(buffer, "%I64d", num);
#else
	sprintf(buffer, "%lld", (long long)num);
#endif
	assert(danger == 'p');
	return buffer;
}

UnicodeString DoubleToUnicodeString(double num)
{
	char buffer[64]; // nos changed from 10 to 64
	char danger = 'p'; // guard against overrunning the buffer (rtg)
	sprintf(buffer, "%1.14e", num);
	assert(danger == 'p');
	return buffer;
}

// [LIU] Just to get things working
UnicodeString operator+(const UnicodeString & left, double num)
{
	char buffer[64]; // was 32, made it arbitrarily bigger (rtg)
	char danger = 'p'; // guard against overrunning the buffer (rtg)
	// IEEE floating point has 52 bits of mantissa, plus one assumed bit
	//  53*log(2)/log(10) = 15.95
	// so there is no need to show more than 16 digits. [alan]
	sprintf(buffer, "%.17g", num);
	assert(danger == 'p');
	return left + buffer;
}

#if 0
UnicodeString operator+(const UnicodeString & left, int64_t num) { return left + Int64ToUnicodeString(num); }
#endif

#if !UCONFIG_NO_FORMATTING

/**
 * Return a string display for this, without surrounding braces.
 */
UnicodeString _toString(const Formattable& f) 
{
	UnicodeString s;
	switch(f.getType()) {
		case Formattable::kDate:
	    {
		    UErrorCode status = U_ZERO_ERROR;
		    SimpleDateFormat fmt(status);
		    if(U_SUCCESS(status)) {
			    FieldPosition pos;
			    fmt.format(f.getDate(), s, pos);
			    s.insert(0, "Date:");
		    }
		    else {
			    s = UnicodeString("Error creating date format]");
		    }
	    }
	    break;
		case Formattable::kDouble:
		    s = UnicodeString("double:") + f.getDouble();
		    break;
		case Formattable::kLong:
		    s = UnicodeString("long:") + f.getLong();
		    break;
		case Formattable::kInt64:
		    s = UnicodeString("int64:") + Int64ToUnicodeString(f.getInt64());
		    break;
		case Formattable::kString:
		    f.getString(s);
		    s.insert(0, "String:");
		    break;
		case Formattable::kArray:
	    {
		    int32_t i, n;
		    const Formattable* array = f.getArray(n);
		    s.insert(0, UnicodeString("Array:"));
		    UnicodeString delim(", ");
		    for(i = 0; i<n; ++i) {
			    if(i > 0) {
				    s.append(delim);
			    }
			    s = s + _toString(array[i]);
		    }
	    }
	    break;
		case Formattable::kObject: {
		    const CurrencyAmount* c = dynamic_cast<const CurrencyAmount*>(f.getObject());
		    if(c != NULL) {
			    s = _toString(c->getNumber()) + " " + UnicodeString(c->getISOCurrency());
		    }
		    else {
			    s = UnicodeString("Unknown UObject");
		    }
		    break;
	    }
		default:
		    s = UnicodeString("Unknown Formattable type=") + (int32_t)f.getType();
		    break;
	}
	return s;
}

/**
 * Originally coded this as operator+, but that makes the expression
 * + char * ambiguous. - liu
 */
UnicodeString toString(const Formattable& f) 
{
	UnicodeString s((char16_t)91 /*[*/);
	s.append(_toString(f));
	s.append((char16_t)0x5d /*]*/);
	return s;
}
#endif

// useful when operator+ won't cooperate
UnicodeString toString(int32_t n) { return UnicodeString() + (long)n; }
UnicodeString toString(bool b) { return b ? UnicodeString("TRUE") : UnicodeString("FALSE"); }

UnicodeString toString(const UnicodeSet & uniset, UErrorCode & status) 
{
	UnicodeString result;
	uniset.toPattern(result, status);
	return result;
}

// stephen - cleaned up 05/05/99
UnicodeString operator+(const UnicodeString & left, char num) { return left + (long)num; }
UnicodeString operator+(const UnicodeString & left, short num) { return left + (long)num; }
UnicodeString operator+(const UnicodeString & left, int num) { return left + (long)num; }
UnicodeString operator+(const UnicodeString & left, unsigned char num) { return left + (unsigned long)num; }
UnicodeString operator+(const UnicodeString & left, unsigned short num) { return left + (unsigned long)num; }
UnicodeString operator+(const UnicodeString & left, unsigned int num) { return left + (unsigned long)num; }
UnicodeString operator+(const UnicodeString & left, float num) { return left + (double)num; }

//------------------

// Append a hex string to the target
UnicodeString &IntlTest::appendHex(uint32_t number, int32_t digits, UnicodeString & target)
{
	static const char16_t digitString[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0 }; /* "0123456789ABCDEF" */
	if(digits < 0) { // auto-digits
		digits = 2;
		uint32_t max = 0xff;
		while(number > max) {
			digits += 2;
			max = (max << 8) | 0xff;
		}
	}
	switch(digits) {
		case 8:
		    target += digitString[(number >> 28) & 0xF];
		    CXX_FALLTHROUGH;
		case 7:
		    target += digitString[(number >> 24) & 0xF];
		    CXX_FALLTHROUGH;
		case 6:
		    target += digitString[(number >> 20) & 0xF];
		    CXX_FALLTHROUGH;
		case 5:
		    target += digitString[(number >> 16) & 0xF];
		    CXX_FALLTHROUGH;
		case 4:
		    target += digitString[(number >> 12) & 0xF];
		    CXX_FALLTHROUGH;
		case 3:
		    target += digitString[(number >>  8) & 0xF];
		    CXX_FALLTHROUGH;
		case 2:
		    target += digitString[(number >>  4) & 0xF];
		    CXX_FALLTHROUGH;
		case 1:
		    target += digitString[(number >>  0) & 0xF];
		    break;
		default:
		    target += "**";
	}
	return target;
}

UnicodeString IntlTest::toHex(uint32_t number, int32_t digits) 
{
	UnicodeString result;
	appendHex(number, digits, result);
	return result;
}

static inline bool isPrintable(UChar32 c) { return c <= 0x7E && (c >= 0x20 || c == 9 || c == 0xA || c == 0xD); }

// Replace nonprintable characters with unicode escapes
UnicodeString &IntlTest::prettify(const UnicodeString & source, UnicodeString & target)
{
	int32_t i;
	target.remove();
	target += "\"";
	for(i = 0; i < source.length();) {
		UChar32 ch = source.char32At(i);
		i += U16_LENGTH(ch);
		if(!isPrintable(ch)) {
			if(ch <= 0xFFFF) {
				target += "\\u";
				appendHex(ch, 4, target);
			}
			else {
				target += "\\U";
				appendHex(ch, 8, target);
			}
		}
		else {
			target += ch;
		}
	}
	target += "\"";
	return target;
}

// Replace nonprintable characters with unicode escapes
UnicodeString IntlTest::prettify(const UnicodeString & source, bool parseBackslash)
{
	int32_t i;
	UnicodeString target;
	target.remove();
	target += "\"";
	for(i = 0; i < source.length();) {
		UChar32 ch = source.char32At(i);
		i += U16_LENGTH(ch);
		if(!isPrintable(ch)) {
			if(parseBackslash) {
				// If we are preceded by an odd number of backslashes,
				// then this character has already been backslash escaped.
				// Delete a backslash.
				int32_t backslashCount = 0;
				for(int32_t j = target.length()-1; j>=0; --j) {
					if(target.charAt(j) == (char16_t)92) {
						++backslashCount;
					}
					else {
						break;
					}
				}
				if((backslashCount % 2) == 1) {
					target.truncate(target.length() - 1);
				}
			}
			if(ch <= 0xFFFF) {
				target += "\\u";
				appendHex(ch, 4, target);
			}
			else {
				target += "\\U";
				appendHex(ch, 8, target);
			}
		}
		else {
			target += ch;
		}
	}

	target += "\"";

	return target;
}

/*  IntlTest::setICU_DATA  - if the ICU_DATA environment variable is not already
 *    set, try to deduce the directory in which ICU was built,
 *    and set ICU_DATA to "icu/source/data" in that location.
 *    The intent is to allow the tests to have a good chance
 *    of running without requiring that the user manually set
 *    ICU_DATA.  Common data isn't a problem, since it is
 *    picked up via a static (build time) reference, but the
 *    tests dynamically load some data.
 */
void IntlTest::setICU_DATA() {
	const char * original_ICU_DATA = getenv("ICU_DATA");

	if(original_ICU_DATA != NULL && *original_ICU_DATA != 0) {
		/*  If the user set ICU_DATA, don't second-guess the person. */
		return;
	}

	// U_TOPBUILDDIR is set by the makefiles on UNIXes when building cintltst and intltst
	//              to point to the top of the build hierarchy, which may or
	//              may not be the same as the source directory, depending on
	//              the configure options used.  At any rate,
	//              set the data path to the built data from this directory.
	//              The value is complete with quotes, so it can be used
	//              as-is as a string constant.

#if defined (U_TOPBUILDDIR)
	{
		static char env_string[] = U_TOPBUILDDIR "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
		u_setDataDirectory(env_string);
		return;
	}

#else
	// Use #else so we don't get compiler warnings due to the return above.

	/* On Windows, the file name obtained from __FILE__ includes a full path.
	 *             This file is "wherever\icu\source\test\cintltst\cintltst.c"
	 *             Change to    "wherever\icu\source\data"
	 */
	{
		char p[sizeof(__FILE__) + 10];
		char * pBackSlash;
		int i;

		strcpy(p, __FILE__);
		/* We want to back over three '\' chars. */
		/*   Only Windows should end up here, so looking for '\' is safe.   */
		for(i = 1; i<=3; i++) {
			pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
			if(pBackSlash != NULL) {
				*pBackSlash = 0; /* Truncate the string at the '\'   */
			}
		}

		if(pBackSlash != NULL) {
			/* We found and truncated three names from the path.
			 *  Now append "source\data" and set the environment
			 */
			strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING);
			u_setDataDirectory(p); /*  p is "ICU_DATA=wherever\icu\source\data"    */
			return;
		}
		else {
			/* __FILE__ on MSVC7 does not contain the directory */
			u_setDataDirectory(".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING);
			return;
		}
	}
#endif

	/* No location for the data dir was identifiable.
	 *   Add other fallbacks for the test data location here if the need arises
	 */
}

//--------------------------------------------------------------------------------------

static const int32_t indentLevel_offset = 3;
static const char delim = '/';

IntlTest* IntlTest::gTest = NULL;

static int32_t execCount = 0;

void it_log(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->log(message);
}

void it_logln(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->logln(message);
}

void it_logln(void)
{
	if(IntlTest::gTest)
		IntlTest::gTest->logln();
}

void it_info(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->info(message);
}

void it_infoln(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->infoln(message);
}

void it_infoln(void)
{
	if(IntlTest::gTest)
		IntlTest::gTest->infoln();
}

void it_err()
{
	if(IntlTest::gTest)
		IntlTest::gTest->err();
}

void it_err(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->err(message);
}

void it_errln(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->errln(message);
}

void it_dataerr(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->dataerr(message);
}

void it_dataerrln(UnicodeString message)
{
	if(IntlTest::gTest)
		IntlTest::gTest->dataerrln(message);
}

IntlTest::IntlTest()
{
	caller = NULL;
	testPath = NULL;
	LL_linestart = TRUE;
	errorCount = 0;
	dataErrorCount = 0;
	verbose = FALSE;
	no_time = FALSE;
	no_err_msg = FALSE;
	warn_on_missing_data = FALSE;
	quick = FALSE;
	leaks = FALSE;
	threadCount = 12;
	testoutfp = stdout;
	LL_indentlevel = indentLevel_offset;
	numProps = 0;
	strcpy(basePath, "/");
	currName[0] = 0;
}

void IntlTest::setCaller(IntlTest* callingTest)
{
	caller = callingTest;
	if(caller) {
		warn_on_missing_data = caller->warn_on_missing_data;
		verbose = caller->verbose;
		no_err_msg = caller->no_err_msg;
		quick = caller->quick;
		threadCount = caller->threadCount;
		testoutfp = caller->testoutfp;
		write_golden_data = caller->write_golden_data;
		LL_indentlevel = caller->LL_indentlevel + indentLevel_offset;
		numProps = caller->numProps;
		for(int32_t i = 0; i < numProps; i++) {
			proplines[i] = caller->proplines[i];
		}
	}
}

bool IntlTest::callTest(IntlTest& testToBeCalled, char * par)
{
	execCount--; // correct a previously assumed test-exec, as this only calls a subtest
	testToBeCalled.setCaller(this);
	strcpy(testToBeCalled.basePath, this->basePath);
	bool result = testToBeCalled.runTest(testPath, par, testToBeCalled.basePath);
	strcpy(testToBeCalled.basePath, this->basePath); // reset it.
	return result;
}

void IntlTest::setPath(char * pathVal)
{
	this->testPath = pathVal;
}

bool IntlTest::setVerbose(bool verboseVal)
{
	bool rval = this->verbose;
	this->verbose = verboseVal;
	return rval;
}

bool IntlTest::setNotime(bool no_time)
{
	bool rval = this->no_time;
	this->no_time = no_time;
	return rval;
}

bool IntlTest::setWarnOnMissingData(bool warn_on_missing_dataVal)
{
	bool rval = this->warn_on_missing_data;
	this->warn_on_missing_data = warn_on_missing_dataVal;
	return rval;
}

bool IntlTest::setWriteGoldenData(bool write_golden_data)
{
	bool rval = this->write_golden_data;
	this->write_golden_data = write_golden_data;
	return rval;
}

bool IntlTest::setNoErrMsg(bool no_err_msgVal)
{
	bool rval = this->no_err_msg;
	this->no_err_msg = no_err_msgVal;
	return rval;
}

bool IntlTest::setQuick(bool quickVal)
{
	bool rval = this->quick;
	this->quick = quickVal;
	return rval;
}

bool IntlTest::setLeaks(bool leaksVal)
{
	bool rval = this->leaks;
	this->leaks = leaksVal;
	return rval;
}

int32_t IntlTest::setThreadCount(int32_t count)
{
	int32_t rval = this->threadCount;
	this->threadCount = count;
	return rval;
}

int32_t IntlTest::getErrors(void)
{
	return errorCount;
}

int32_t IntlTest::getDataErrors(void)
{
	return dataErrorCount;
}

bool IntlTest::runTest(char * name, char * par, char * baseName)
{
	bool rval;
	char * pos = NULL;
	char * baseNameBuffer = NULL;
	if(baseName == NULL) {
		baseNameBuffer = (char *)SAlloc::M(1024);
		baseName = baseNameBuffer;
		strcpy(baseName, "/");
	}
	if(name)
		pos = strchr(name, delim); // check if name contains path (by looking for '/')
	if(pos) {
		testPath = pos+1; // store subpath for calling subtest
		*pos = 0; // split into two strings
	}
	else {
		testPath = NULL;
	}

	if(!name || (name[0] == 0) || sstreq(name, "*")) {
		rval = runTestLoop(NULL, par, baseName);
	}
	else if(sstreq(name, "LIST")) {
		this->usage();
		rval = TRUE;
	}
	else {
		rval = runTestLoop(name, par, baseName);
	}
	if(pos)
		*pos = delim; // restore original value at pos
	SAlloc::F(baseNameBuffer);
	return rval;
}

// call individual tests, to be overridden to call implementations
void IntlTest::runIndexedTest(int32_t /*index*/, bool /*exec*/, const char * & /*name*/, char * /*par*/)
{
	// to be overridden by a method like:
	/*
	   switch (index) {
	    case 0: name = "First Test"; if(exec) FirstTest( par ); break;
	    case 1: name = "Second Test"; if(exec) SecondTest( par ); break;
	    default: name = ""; break;
	   }
	 */
	this->errln("*** runIndexedTest needs to be overridden! ***");
}

bool IntlTest::runTestLoop(char * testname, char * par, char * baseName)
{
	int32_t index = 0;
	const char * name;
	bool run_this_test;
	int32_t lastErrorCount;
	bool rval = FALSE;
	bool lastTestFailed;
	if(baseName == NULL) {
		printf("ERROR: baseName can't be null.\n");
		return FALSE;
	}
	else {
		if((char *)this->basePath != baseName) {
			strcpy(this->basePath, baseName);
		}
	}
	char * saveBaseLoc = baseName+strlen(baseName);
	IntlTest* saveTest = gTest;
	gTest = this;
	do {
		this->runIndexedTest(index, FALSE, name, par);
		if(sstreq(name, "skip")) {
			run_this_test = FALSE;
		}
		else {
			if(!name || (name[0] == 0))
				break;
			if(!testname) {
				run_this_test = TRUE;
			}
			else {
				run_this_test = sstreq(name, testname);
			}
		}
		if(run_this_test) {
			lastErrorCount = errorCount;
			execCount++;
			char msg[256];
			sprintf(msg, "%s {", name);
			LL_message(msg, TRUE);
			UDate timeStart = uprv_getRawUTCtime();
			strcpy(saveBaseLoc, name);
			strcat(saveBaseLoc, "/");
			strcpy(currName, name); // set
			this->runIndexedTest(index, TRUE, name, par);
			currName[0] = 0; // reset
			UDate timeStop = uprv_getRawUTCtime();
			rval = TRUE; // at least one test has been called
			char secs[256];
			if(!no_time) {
				sprintf(secs, "%f", (timeStop-timeStart)/1000.0);
			}
			else {
				secs[0] = 0;
			}
			strcpy(saveBaseLoc, name);
			ctest_xml_testcase(baseName, name, secs, (lastErrorCount!=errorCount) ? "err" : NULL);
			saveBaseLoc[0] = 0; /* reset path */
			if(lastErrorCount == errorCount) {
				sprintf(msg, "   } OK:   %s ", name);
				if(!no_time) str_timeDelta(msg+strlen(msg), timeStop-timeStart);
				lastTestFailed = FALSE;
			}
			else {
				sprintf(msg,  "   } ERRORS (%li) in %s", (long)(errorCount-lastErrorCount), name);
				if(!no_time) 
					str_timeDelta(msg+strlen(msg), timeStop-timeStart);
				for(int i = 0; i<LL_indentlevel; i++) {
					errorList += " ";
				}
				errorList += name;
				errorList += "\n";
				lastTestFailed = TRUE;
			}
			LL_indentlevel -= 3;
			if(lastTestFailed) {
				LL_message("", TRUE);
			}
			LL_message(msg, TRUE);
			if(lastTestFailed) {
				LL_message("", TRUE);
			}
			LL_indentlevel += 3;
		}
		index++;
	} while(name);
	*saveBaseLoc = 0;
	gTest = saveTest;
	return rval;
}

/**
 * Adds given string to the log if we are in verbose mode.
 */
void IntlTest::log(const UnicodeString & message)
{
	if(verbose) {
		LL_message(message, FALSE);
	}
}

/**
 * Adds given string to the log if we are in verbose mode. Adds a new line to
 * the given message.
 */
void IntlTest::logln(const UnicodeString & message)
{
	if(verbose) {
		LL_message(message, TRUE);
	}
}

void IntlTest::logln(void)
{
	if(verbose) {
		LL_message("", TRUE);
	}
}

/**
 * Unconditionally adds given string to the log.
 */
void IntlTest::info(const UnicodeString & message)
{
	LL_message(message, FALSE);
}

/**
 * Unconditionally adds given string to the log. Adds a new line to
 * the given message.
 */
void IntlTest::infoln(const UnicodeString & message)
{
	LL_message(message, TRUE);
}

void IntlTest::infoln(void)
{
	LL_message("", TRUE);
}

int32_t IntlTest::IncErrorCount(void)
{
	errorCount++;
	if(caller) caller->IncErrorCount();
	return errorCount;
}

int32_t IntlTest::IncDataErrorCount(void)
{
	dataErrorCount++;
	if(caller) caller->IncDataErrorCount();
	return dataErrorCount;
}

void IntlTest::err()
{
	IncErrorCount();
}

void IntlTest::err(const UnicodeString & message)
{
	IncErrorCount();
	if(!no_err_msg) LL_message(message, FALSE);
}

void IntlTest::errln(const UnicodeString & message)
{
	IncErrorCount();
	if(!no_err_msg) LL_message(message, TRUE);
}

void IntlTest::dataerr(const UnicodeString & message)
{
	IncDataErrorCount();

	if(!warn_on_missing_data) {
		IncErrorCount();
	}

	if(!no_err_msg) LL_message(message, FALSE);
}

void IntlTest::dataerrln(const UnicodeString & message)
{
	int32_t errCount = IncDataErrorCount();
	UnicodeString msg;
	if(!warn_on_missing_data) {
		IncErrorCount();
		msg = message;
	}
	else {
		msg = UnicodeString("[DATA] " + message);
	}

	if(!no_err_msg) {
		if(errCount == 1) {
			LL_message(msg + " - (Are you missing data?)", TRUE); // only show this message the first time
		}
		else {
			LL_message(msg, TRUE);
		}
	}
}

void IntlTest::errcheckln(UErrorCode status, const UnicodeString & message) {
	if(status == U_FILE_ACCESS_ERROR || status == U_MISSING_RESOURCE_ERROR) {
		dataerrln(message);
	}
	else {
		errln(message);
	}
}

/* convenience functions that include sprintf formatting */
void IntlTest::log(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	/* sprintf it just to make sure that the information is valid */
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	if(verbose) {
		log(UnicodeString(buffer, (const char *)NULL));
	}
}

void IntlTest::logln(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	/* sprintf it just to make sure that the information is valid */
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	if(verbose) {
		logln(UnicodeString(buffer, (const char *)NULL));
	}
}

bool IntlTest::logKnownIssue(const char * ticket, const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	/* sprintf it just to make sure that the information is valid */
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	return logKnownIssue(ticket, UnicodeString(buffer, (const char *)NULL));
}

bool IntlTest::logKnownIssue(const char * ticket) {
	return logKnownIssue(ticket, UnicodeString());
}

bool IntlTest::logKnownIssue(const char * ticket, const UnicodeString & msg) {
	if(noKnownIssues) return FALSE;

	char fullpath[2048];
	strcpy(fullpath, basePath);
	strcat(fullpath, currName);
	UnicodeString msg2 = msg;
	bool firstForTicket = TRUE, firstForWhere = TRUE;
	knownList = udbg_knownIssue_openU(knownList, ticket, fullpath, msg2.getTerminatedBuffer(), &firstForTicket, &firstForWhere);

	msg2 = UNICODE_STRING_SIMPLE("(Known issue ") +
	    UnicodeString(ticket, -1, US_INV) + UNICODE_STRING_SIMPLE(") ") + msg;
	if(firstForTicket || firstForWhere) {
		infoln(msg2);
	}
	else {
		logln(msg2);
	}

	return TRUE;
}

/* convenience functions that include sprintf formatting */
void IntlTest::info(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	/* sprintf it just to make sure that the information is valid */
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	info(UnicodeString(buffer, (const char *)NULL));
}

void IntlTest::infoln(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	/* sprintf it just to make sure that the information is valid */
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	infoln(UnicodeString(buffer, (const char *)NULL));
}

void IntlTest::err(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	err(UnicodeString(buffer, (const char *)NULL));
}

void IntlTest::errln(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	errln(UnicodeString(buffer, (const char *)NULL));
}

void IntlTest::dataerrln(const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	dataerrln(UnicodeString(buffer, (const char *)NULL));
}

void IntlTest::errcheckln(UErrorCode status, const char * fmt, ...)
{
	char buffer[4000];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	if(status == U_FILE_ACCESS_ERROR || status == U_MISSING_RESOURCE_ERROR) {
		dataerrln(UnicodeString(buffer, (const char *)NULL));
	}
	else {
		errln(UnicodeString(buffer, (const char *)NULL));
	}
}

void IntlTest::printErrors()
{
	IntlTest::LL_message(errorList, TRUE);
}

bool IntlTest::printKnownIssues()
{
	if(knownList != NULL) {
		udbg_knownIssue_print(knownList);
		udbg_knownIssue_close(knownList);
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void IntlTest::LL_message(UnicodeString message, bool newline)
{
	// Synchronize this function.
	// All error messages generated by tests funnel through here.
	// Multithreaded tests can concurrently generate errors, requiring synchronization
	// to keep each message together.
	static UMutex messageMutex;
	Mutex lock(&messageMutex);

	// string that starts with a LineFeed character and continues
	// with spaces according to the current indentation
	static const char16_t indentUChars[] = {
		'\n',
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
		32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	};
	assert(1 + LL_indentlevel <= SIZEOFARRAYi(indentUChars));
	UnicodeString indent(FALSE, indentUChars, 1 + LL_indentlevel);

	char buffer[30000];
	int32_t length;

	// stream out the indentation string first if necessary
	length = indent.extract(1, indent.length(), buffer, sizeof(buffer));
	if(length > 0) {
		fwrite(buffer, sizeof(*buffer), length, (FILE*)testoutfp);
	}

	// replace each LineFeed by the indentation string
	message.findAndReplace(UnicodeString((char16_t)'\n'), indent);

	// stream out the message
	length = message.extract(0, message.length(), buffer, sizeof(buffer));
	if(length > 0) {
		length = length > 30000 ? 30000 : length;
		fwrite(buffer, sizeof(*buffer), length, (FILE*)testoutfp);
	}

	if(newline) {
		char newLine = '\n';
		fwrite(&newLine, sizeof(newLine), 1, (FILE*)testoutfp);
	}

	// A newline usually flushes the buffer, but
	// flush the message just in case of a core dump.
	fflush((FILE*)testoutfp);
}

/**
 * Print a usage message for this test class.
 */
void IntlTest::usage(void)
{
	bool save_verbose = setVerbose(TRUE);
	logln("Test names:");
	logln("-----------");

	int32_t index = 0;
	const char * name = NULL;
	do {
		this->runIndexedTest(index, FALSE, name);
		if(!name) break;
		logln(name);
		index++;
	} while(name && (name[0] != 0));
	setVerbose(save_verbose);
}

// memory leak reporting software will be able to take advantage of the testsuite
// being run a second time local to a specific method in order to report only actual leaks
bool IntlTest::run_phase2(char * name, char * par) // supports reporting memory leaks
{
	UnicodeString * strLeak = new UnicodeString("forced leak"); // for verifying purify filter
	strLeak->append(" for verifying purify filter");
	return this->runTest(name, par);
}

#if UCONFIG_NO_LEGACY_CONVERSION
#define TRY_CNV_1 "iso-8859-1"
#define TRY_CNV_2 "ibm-1208"
#else
#define TRY_CNV_1 "iso-8859-7"
#define TRY_CNV_2 "sjis"
#endif

#ifdef UNISTR_COUNT_FINAL_STRING_LENGTHS
U_CAPI void unistr_printLengths();
#endif

int main(int argc, char * argv[])
{
	bool syntax = FALSE;
	bool all = FALSE;
	bool verbose = FALSE;
	bool no_err_msg = FALSE;
	bool no_time = FALSE;
	bool quick = TRUE;
	bool name = FALSE;
	bool leaks = FALSE;
	bool utf8 = FALSE;
	const char * summary_file = NULL;
	bool warnOnMissingData = FALSE;
	bool writeGoldenData = FALSE;
	bool defaultDataFound = FALSE;
	int32_t threadCount = 12;
	UErrorCode errorCode = U_ZERO_ERROR;
	UConverter * cnv = NULL;
	const char * warnOrErr = "Failure";
	UDate startTime, endTime;
	int32_t diffTime;
	const char * props[IntlTest::kMaxProps];
	int32_t nProps = 0;
	U_MAIN_INIT_ARGS(argc, argv);
	startTime = uprv_getRawUTCtime();
	for(int i = 1; i < argc; ++i) {
		if(argv[i][0] == '-') {
			const char * str = argv[i] + 1;
			if(sstreq("verbose", str) || sstreq("v", str))
				verbose = TRUE;
			else if(sstreq("noerrormsg", str) || sstreq("n", str))
				no_err_msg = TRUE;
			else if(sstreq("exhaustive", str) || sstreq("e", str))
				quick = FALSE;
			else if(sstreq("all", str) || sstreq("a", str))
				all = TRUE;
			else if(sstreq("utf-8", str) || sstreq("u", str))
				utf8 = TRUE;
			else if(sstreq("noknownissues", str) || sstreq("K", str))
				noKnownIssues = TRUE;
			else if(sstreq("leaks", str) || sstreq("l", str))
				leaks = TRUE;
			else if(sstreq("notime", str) || sstreq("T", str))
				no_time = TRUE;
			else if(sstreq("goldens", str) || sstreq("G", str))
				writeGoldenData = TRUE;
			else if(strncmp("E", str, 1) == 0)
				summary_file = str+1;
			else if(sstreq("x", str)) {
				if(++i>=argc) {
					printf("* Error: '-x' option requires an argument. usage: '-x outfile.xml'.\n");
					syntax = TRUE;
				}
				if(ctest_xml_setFileName(argv[i])) { /* set the name */
					return 1; /* error */
				}
			}
			else if(sstreq("w", str)) {
				warnOnMissingData = TRUE;
				warnOrErr = "WARNING";
			}
			else if(strncmp("threads:", str, 8) == 0) {
				threadCount = atoi(str + 8);
			}
			else if(strncmp("prop:", str, 5) == 0) {
				if(nProps < IntlTest::kMaxProps) {
					props[nProps] = str + 5;
				}
				nProps++;
			}
			else {
				syntax = TRUE;
			}
		}
		else {
			name = TRUE;
		}
	}
	if(!all && !name) {
		all = TRUE;
	}
	else if(all && name) {
		syntax = TRUE;
	}
	if(syntax) {
		fprintf(stdout, "### Syntax:\n"
		    "### IntlTest [-option1 -option2 ...] [testname1 testname2 ...] \n"
		    "### \n"
		    "### Options are: verbose (v), all (a), noerrormsg (n), \n"
		    "### exhaustive (e), leaks (l), -x xmlfile.xml, prop:<property>=<value>, \n"
		    "### notime (T), \n"
		    "### threads:<threadCount>\n"
		    "###     (The default thread count is 12.),\n"
		    "### (Specify either -all (shortcut -a) or a test name). \n"
		    "### -all will run all of the tests.\n"
		    "### \n"
		    "### To get a list of the test names type: intltest LIST \n"
		    "### To run just the utility tests type: intltest utility \n"
		    "### \n"
		    "### Test names can be nested using slashes (\"testA/subtest1\") \n"
		    "### For example to list the utility tests type: intltest utility/LIST \n"
		    "### To run just the Locale test type: intltest utility/LocaleTest \n"
		    "### \n"
		    "### A parameter can be specified for a test by appending '@' and the value \n"
		    "### to the testname. \n\n");
		return 1;
	}
	if(nProps > IntlTest::kMaxProps) {
		fprintf(stdout, "### Too many properties.  Exiting.\n");
	}
	MajorTestLevel major;
	major.setVerbose(verbose);
	major.setNoErrMsg(no_err_msg);
	major.setQuick(quick);
	major.setLeaks(leaks);
	major.setThreadCount(threadCount);
	major.setWarnOnMissingData(warnOnMissingData);
	major.setWriteGoldenData(writeGoldenData);
	major.setNotime(no_time);
	for(int32_t i = 0; i < nProps; i++) {
		major.setProperty(props[i]);
	}
	fprintf(stdout, "-----------------------------------------------\n");
	fprintf(stdout, " IntlTest (C++) Test Suite for                 \n");
	fprintf(stdout, "   International Components for Unicode %s\n", U_ICU_VERSION);
	{
		const char * charsetFamily = "Unknown";
		int32_t voidSize = (int32_t)sizeof(void *);
		int32_t bits = voidSize * 8;
		if(U_CHARSET_FAMILY==U_ASCII_FAMILY) {
			charsetFamily = "ASCII";
		}
		else if(U_CHARSET_FAMILY==U_EBCDIC_FAMILY) {
			charsetFamily = "EBCDIC";
		}
		fprintf(stdout, "   Bits: %d, Byte order: %s, Chars: %s\n", bits, U_IS_BIG_ENDIAN ? "Big endian" : "Little endian", charsetFamily);
	}
	fprintf(stdout, "-----------------------------------------------\n");
	fprintf(stdout, " Options:                                       \n");
	fprintf(stdout, "   all (a)                  : %s\n", (all ?               "On" : "Off"));
	fprintf(stdout, "   Verbose (v)              : %s\n", (verbose ?           "On" : "Off"));
	fprintf(stdout, "   No error messages (n)    : %s\n", (no_err_msg ?        "On" : "Off"));
	fprintf(stdout, "   Exhaustive (e)           : %s\n", (!quick ?            "On" : "Off"));
	fprintf(stdout, "   Leaks (l)                : %s\n", (leaks ?             "On" : "Off"));
	fprintf(stdout, "   utf-8 (u)                : %s\n", (utf8 ?              "On" : "Off"));
	fprintf(stdout, "   notime (T)               : %s\n", (no_time ?           "On" : "Off"));
	fprintf(stdout, "   noknownissues (K)        : %s\n", (noKnownIssues ?     "On" : "Off"));
	fprintf(stdout, "   Warn on missing data (w) : %s\n", (warnOnMissingData ? "On" : "Off"));
	fprintf(stdout, "   Write golden data (G)    : %s\n", (writeGoldenData ?   "On" : "Off"));
	fprintf(stdout, "   Threads                  : %d\n", threadCount);
	for(int32_t i = 0; i < nProps; i++) {
		fprintf(stdout, "   Custom property (prop:)  : %s\n", props[i]);
	}
	fprintf(stdout, "-----------------------------------------------\n");
	if(utf8) {
		ucnv_setDefaultName("utf-8");
	}
	/* Check whether ICU will initialize without forcing the build data directory into
	 *  the ICU_DATA path.  Success here means either the data dll contains data, or that
	 *  this test program was run with ICU_DATA set externally.  Failure of this check
	 *  is normal when ICU data is not packaged into a shared library.
	 *
	 *  Whether or not this test succeeds, we want to cleanup and reinitialize
	 *  with a data path so that data loading from individual files can be tested.
	 */
	u_init(&errorCode);
	if(U_FAILURE(errorCode)) {
		slfprintf_stderr("#### Note:  ICU Init without build-specific setDataDirectory() failed.\n");
		defaultDataFound = FALSE;
	}
	else {
		defaultDataFound = TRUE;
	}
	u_cleanup();
	if(utf8) {
		ucnv_setDefaultName("utf-8");
	}
	errorCode = U_ZERO_ERROR;
	/* Initialize ICU */
	if(!defaultDataFound) {
		IntlTest::setICU_DATA(); // Must set data directory before u_init() is called.
	}
	u_init(&errorCode);
	if(U_FAILURE(errorCode)) {
		slfprintf_stderr("#### ERROR! %s: u_init() failed with status = \"%s\".\n"
		    "*** Check the ICU_DATA environment variable and \n"
		    "*** check that the data files are present.\n", argv[0], u_errorName(errorCode));
		if(warnOnMissingData == 0) {
			slfprintf_stderr("*** Exiting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
			u_cleanup();
			return 1;
		}
	}
	// initial check for the default converter
	errorCode = U_ZERO_ERROR;
	cnv = ucnv_open(0, &errorCode);
	if(cnv != 0) {
		// ok
		ucnv_close(cnv);
	}
	else {
		fprintf(stdout, "*** %s! The default converter [%s] cannot be opened.\n"
		    "*** Check the ICU_DATA environment variable and\n"
		    "*** check that the data files are present.\n",
		    warnOrErr, ucnv_getDefaultName());
		if(!warnOnMissingData) {
			fprintf(stdout, "*** Exiting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
			return 1;
		}
	}
	// try more data
	cnv = ucnv_open(TRY_CNV_2, &errorCode);
	if(cnv != 0) {
		// ok
		ucnv_close(cnv);
	}
	else {
		fprintf(stdout, "*** %s! The converter for " TRY_CNV_2 " cannot be opened.\n*** Check the ICU_DATA environment variable and \n"
		    "*** check that the data files are present.\n", warnOrErr);
		if(!warnOnMissingData) {
			fprintf(stdout, "*** Exiting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
			return 1;
		}
	}
	UResourceBundle * rb = ures_open(0, "en", &errorCode);
	ures_close(rb);
	if(U_FAILURE(errorCode)) {
		fprintf(stdout, "*** %s! The \"en\" locale resource bundle cannot be opened.\n"
		    "*** Check the ICU_DATA environment variable and \n" "*** check that the data files are present.\n", warnOrErr);
		if(!warnOnMissingData) {
			fprintf(stdout, "*** Exiting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
			return 1;
		}
	}
	Locale originalLocale; // Save the default locale for comparison later on.
	if(ctest_xml_init("intltest"))
		return 1;
	/* TODO: Add option to call u_cleanup and rerun tests. */
	if(all) {
		major.runTest();
		if(leaks) {
			major.run_phase2(NULL, NULL);
		}
	}
	else {
		for(int i = 1; i < argc; ++i) {
			if(argv[i][0] != '-') {
				char * name = argv[i];
				fprintf(stdout, "\n=== Handling test: %s: ===\n", name);
				char baseName[1024];
				sprintf(baseName, "/%s/", name);
				char * parameter = strchr(name, '@');
				if(parameter) {
					*parameter = 0;
					parameter += 1;
				}
				execCount = 0;
				bool res = major.runTest(name, parameter, baseName);
				if(leaks && res) {
					major.run_phase2(name, parameter);
				}
				if(!res || (execCount <= 0)) {
					fprintf(stdout, "\n---ERROR: Test doesn't exist: %s!\n", name);
				}
			}
			else if(sstreq(argv[i], "-x")) {
				i++;
			}
		}
	}
#if !UCONFIG_NO_FORMATTING
	CalendarTimeZoneTest::cleanup();
#endif
	SAlloc::F(_testDataPath);
	Locale lastDefaultLocale;
	if(originalLocale != lastDefaultLocale) {
		major.errln("FAILURE: A test changed the default locale without resetting it.");
	}
	fprintf(stdout, "\n--------------------------------------\n");
	if(major.printKnownIssues()) {
		fprintf(stdout, " To run suppressed tests, use the -K option. \n");
	}
	if(major.getErrors() == 0) {
		/* Call it twice to make sure that the defaults were reset. */
		/* Call it before the OK message to verify proper cleanup. */
		u_cleanup();
		u_cleanup();
		fprintf(stdout, "OK: All tests passed without error.\n");
		if(major.getDataErrors() != 0) {
			fprintf(stdout, "\t*WARNING* some data-loading errors were ignored by the -w option.\n");
		}
	}
	else {
		fprintf(stdout, "Errors in total: %ld.\n", (long)major.getErrors());
		major.printErrors();
		if(summary_file != NULL) {
			FILE * summf = fopen(summary_file, "w");
			if(summf != NULL) {
				char buf[10000];
				int32_t length = errorList.extract(0, errorList.length(), buf, sizeof(buf));
				fwrite(buf, sizeof(*buf), length, (FILE*)summf);
				fclose(summf);
			}
		}

		if(major.getDataErrors() != 0) {
			fprintf(stdout, "\t*Note* some errors are data-loading related. If the data used is not the \n"
			    "\tstock ICU data (i.e some have been added or removed), consider using\n"
			    "\tthe '-w' option to turn these errors into warnings.\n");
		}

		/* Call afterwards to display errors. */
		u_cleanup();
	}

#ifdef UNISTR_COUNT_FINAL_STRING_LENGTHS
	unistr_printLengths();
#endif
	fprintf(stdout, "--------------------------------------\n");
	if(execCount <= 0) {
		fprintf(stdout, "***** Not all called tests actually exist! *****\n");
	}
	if(!no_time) {
		endTime = uprv_getRawUTCtime();
		diffTime = (int32_t)(endTime - startTime);
		printf("Elapsed Time: %02d:%02d:%02d.%03d\n",
		    (int)((diffTime%U_MILLIS_PER_DAY)/U_MILLIS_PER_HOUR),
		    (int)((diffTime%U_MILLIS_PER_HOUR)/U_MILLIS_PER_MINUTE),
		    (int)((diffTime%U_MILLIS_PER_MINUTE)/U_MILLIS_PER_SECOND),
		    (int)(diffTime%U_MILLIS_PER_SECOND));
	}
	if(ctest_xml_fini())
		return 1;
	return major.getErrors();
}

const char * IntlTest::loadTestData(UErrorCode & err) 
{
	if(_testDataPath == NULL) {
		const char * directory = NULL;
		UResourceBundle * test = NULL;
		char * tdpath = NULL;
		const char * tdrelativepath;
#if defined (U_TOPBUILDDIR)
		tdrelativepath = "test" U_FILE_SEP_STRING "testdata" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
		directory = U_TOPBUILDDIR;
#else
		tdrelativepath = ".." U_FILE_SEP_STRING "test" U_FILE_SEP_STRING "testdata" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
		directory = pathToDataDirectory();
#endif
		tdpath = (char *)SAlloc::M(sizeof(char) *(( strlen(directory) * strlen(tdrelativepath)) + 100));
		if(tdpath == NULL) {
			err = U_MEMORY_ALLOCATION_ERROR;
			it_dataerrln((UnicodeString)"Could not allocate memory for _testDataPath " + u_errorName(err));
			return "";
		}
		/* u_getDataDirectory shoul return \source\data ... set the
		 * directory to ..\source\data\..\test\testdata\out\testdata
		 */
		strcpy(tdpath, directory);
		strcat(tdpath, tdrelativepath);
		strcat(tdpath, "testdata");
		test = ures_open(tdpath, "testtypes", &err);
		if(U_FAILURE(err)) {
			err = U_FILE_ACCESS_ERROR;
			it_dataerrln((UnicodeString)"Could not load testtypes.res in testdata bundle with path " + tdpath + (UnicodeString)" - " + u_errorName(err));
			return "";
		}
		ures_close(test);
		_testDataPath = tdpath;
		return _testDataPath;
	}
	return _testDataPath;
}

const char * IntlTest::getTestDataPath(UErrorCode & err) 
{
	return loadTestData(err);
}
/**
 * Returns the path to icu/source/test/testdata/
 * Note: this function is parallel with C loadSourceTestData in cintltst.c
 */
const char * IntlTest::getSourceTestData(UErrorCode & /*err*/) 
{
	const char * srcDataDir = NULL;
#ifdef U_TOPSRCDIR
	srcDataDir = U_TOPSRCDIR "/test/testdata/";
#else
	// @sobolev srcDataDir = "../../test/testdata/";
	srcDataDir = "/papyrus/src/osf/icu/icu4c/source/test/testdata/"; // @sobolev
	/* @sobolev
	FILE * f = fopen("../../test/testdata/rbbitst.txt", "r");
	if(f) {
		// We're in icu/source/test/intltest/ 
		fclose(f);
	}
	else {
		// We're in icu/source/test/intltest/Platform/(Debug|Release) 
		srcDataDir = "../../../../test/testdata/";
	}*/
#endif
	return srcDataDir;
}

char * IntlTest::getUnidataPath(char path[]) 
{
	const int kUnicodeDataTxtLength = 15; // strlen("UnicodeData.txt")
	// Look inside ICU_DATA first.
	strcpy(path, pathToDataDirectory());
	strcat(path, "unidata" U_FILE_SEP_STRING "UnicodeData.txt");
	FILE * f = fopen(path, "r");
	if(f != NULL) {
		fclose(f);
		*(strchr(path, 0) - kUnicodeDataTxtLength) = 0; // Remove the basename.
		return path;
	}

	// As a fallback, try to guess where the source data was located
	// at the time ICU was built, and look there.
#ifdef U_TOPSRCDIR
	strcpy(path, U_TOPSRCDIR U_FILE_SEP_STRING "data");
#else
	UErrorCode errorCode = U_ZERO_ERROR;
	const char * testDataPath = loadTestData(errorCode);
	if(U_FAILURE(errorCode)) {
		it_errln(UnicodeString(
			    "unable to find path to source/data/unidata/ and loadTestData() failed: ") +
		    u_errorName(errorCode));
		return NULL;
	}
	strcpy(path, testDataPath);
	strcat(path, U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".."
	    U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".."
	    U_FILE_SEP_STRING "data");
#endif
	strcat(path, U_FILE_SEP_STRING);
	strcat(path, "unidata" U_FILE_SEP_STRING "UnicodeData.txt");
	f = fopen(path, "r");
	if(f != NULL) {
		fclose(f);
		*(strchr(path, 0) - kUnicodeDataTxtLength) = 0; // Remove the basename.
		return path;
	}
	return NULL;
}

const char * IntlTest::fgDataDir = NULL;

/* returns the path to icu/source/data */
const char * IntlTest::pathToDataDirectory()
{
	if(fgDataDir != NULL) {
		return fgDataDir;
	}

	/* U_TOPSRCDIR is set by the makefiles on UNIXes when building cintltst and intltst
	   //              to point to the top of the build hierarchy, which may or
	   //              may not be the same as the source directory, depending on
	   //              the configure options used.  At any rate,
	   //              set the data path to the built data from this directory.
	   //              The value is complete with quotes, so it can be used
	   //              as-is as a string constant.
	 */
#if defined (U_TOPSRCDIR)
	{
		fgDataDir = U_TOPSRCDIR U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
	}
#else

	/* On Windows, the file name obtained from __FILE__ includes a full path.
	 *             This file is "wherever\icu\source\test\cintltst\cintltst.c"
	 *             Change to    "wherever\icu\source\data"
	 */
	{
		static char p[sizeof(__FILE__) + 10];
		char * pBackSlash;
		int i;

		strcpy(p, __FILE__);
		/* We want to back over three '\' chars. */
		/*   Only Windows should end up here, so looking for '\' is safe.   */
		for(i = 1; i<=3; i++) {
			pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
			if(pBackSlash != NULL) {
				*pBackSlash = 0; /* Truncate the string at the '\'   */
			}
		}

		if(pBackSlash != NULL) {
			/* We found and truncated three names from the path.
			 *  Now append "source\data" and set the environment
			 */
			strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING);
			fgDataDir = p;
		}
		else {
			/* __FILE__ on MSVC7 does not contain the directory */
			FILE * file = fopen(".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
			if(file) {
				fclose(file);
				fgDataDir = ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
			}
			else {
				fgDataDir =
				    ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING "data"
				    U_FILE_SEP_STRING;
			}
		}
	}
#endif

	return fgDataDir;
}

/*
 * This is a variant of cintltst/ccolltst.c:CharsToUChars().
 * It converts an invariant-character string into a UnicodeString, with
 * unescaping \u sequences.
 */
UnicodeString CharsToUnicodeString(const char * chars) {
	return UnicodeString(chars, -1, US_INV).unescape();
}

UnicodeString ctou(const char * chars) {
	return CharsToUnicodeString(chars);
}

#define RAND_M  (714025)
#define RAND_IA (1366)
#define RAND_IC (150889)

static int32_t RAND_SEED;

/**
 * Returns a uniform random value x, with 0.0 <= x < 1.0.  Use
 * with care: Does not return all possible values; returns one of
 * 714,025 values, uniformly spaced.  However, the period is
 * effectively infinite.  See: Numerical Recipes, section 7.1.
 *
 * @param seedp pointer to seed. Set *seedp to any negative value
 * to restart the sequence.
 */
float IntlTest::random(int32_t* seedp) {
	static int32_t iy, ir[98];
	static bool first = TRUE;
	int32_t j;
	if(*seedp < 0 || first) {
		first = FALSE;
		if((*seedp = (RAND_IC-(*seedp)) % RAND_M) < 0) *seedp = -(*seedp);
		for(j = 1; j<=97; ++j) {
			*seedp = (RAND_IA*(*seedp)+RAND_IC) % RAND_M;
			ir[j] = (*seedp);
		}
		*seedp = (RAND_IA*(*seedp)+RAND_IC) % RAND_M;
		iy = (*seedp);
	}
	j = (int32_t)(1 + 97.0*iy/RAND_M);
	assert(j>=1 && j<=97);
	iy = ir[j];
	*seedp = (RAND_IA*(*seedp)+RAND_IC) % RAND_M;
	ir[j] = (*seedp);
	return (float)iy/RAND_M;
}

/**
 * Convenience method using a global seed.
 */
float IntlTest::random() {
	return random(&RAND_SEED);
}

/*
 * Integer random number class implementation.
 * Similar to C++ std::minstd_rand, with the same algorithm & constants.
 */
IntlTest::icu_rand::icu_rand(uint32_t seed) {
	seed = seed % 2147483647UL;
	if(seed == 0) {
		seed = 1;
	}
	fLast = seed;
}

IntlTest::icu_rand::~icu_rand() {
}

void IntlTest::icu_rand::seed(uint32_t seed) {
	if(seed == 0) {
		seed = 1;
	}
	fLast = seed;
}

uint32_t IntlTest::icu_rand::operator()() {
	fLast = ((uint64_t)fLast * 48271UL) % 2147483647UL;
	return fLast;
}

uint32_t IntlTest::icu_rand::getSeed() {
	return (uint32_t)fLast;
}

static inline char16_t toHex(int32_t i) {
	return (char16_t)(i + (i < 10 ? 0x30 : (0x41 - 10)));
}

static UnicodeString & escape(const UnicodeString & s, UnicodeString & result) {
	for(int32_t i = 0; i<s.length(); ++i) {
		char16_t c = s[i];
		if(c <= (char16_t)0x7F) {
			result += c;
		}
		else {
			result += (char16_t)0x5c;
			result += (char16_t)0x75;
			result += toHex((c >> 12) & 0xF);
			result += toHex((c >>  8) & 0xF);
			result += toHex((c >>  4) & 0xF);
			result += toHex(c        & 0xF);
		}
	}
	return result;
}

#define VERBOSE_ASSERTIONS

bool IntlTest::assertTrue(const char * message, bool condition, bool quiet, bool possibleDataError, const char * file, int line) {
	if(file != NULL) {
		if(!condition) {
			if(possibleDataError) {
				dataerrln("%s:%d: FAIL: assertTrue() failed: %s", file, line, message);
			}
			else {
				errln("%s:%d: FAIL: assertTrue() failed: %s", file, line, message);
			}
		}
		else if(!quiet) {
			logln("%s:%d: Ok: %s", file, line, message);
		}
	}
	else {
		if(!condition) {
			if(possibleDataError) {
				dataerrln("FAIL: assertTrue() failed: %s", message);
			}
			else {
				errln("FAIL: assertTrue() failed: %s", message);
			}
		}
		else if(!quiet) {
			logln("Ok: %s", message);
		}
	}
	return condition;
}

bool IntlTest::assertFalse(const char * message, bool condition, bool quiet, bool possibleDataError) {
	if(condition) {
		if(possibleDataError) {
			dataerrln("FAIL: assertFalse() failed: %s", message);
		}
		else {
			errln("FAIL: assertFalse() failed: %s", message);
		}
	}
	else if(!quiet) {
		logln("Ok: %s", message);
	}
	return !condition;
}

bool IntlTest::assertSuccess(const char * message, UErrorCode ec, bool possibleDataError, const char * file, int line) {
	if(!file) {
		file = ""; // prevent failure if no file given
	}
	if(U_FAILURE(ec)) {
		if(possibleDataError) {
			dataerrln("FAIL: %s:%d: %s (%s)", file, line, message, u_errorName(ec));
		}
		else {
			errcheckln(ec, "FAIL: %s:%d: %s (%s)", file, line, message, u_errorName(ec));
		}
		return FALSE;
	}
	else {
		logln("OK: %s:%d: %s - (%s)", file, line, message, u_errorName(ec));
	}
	return TRUE;
}

bool IntlTest::assertEquals(const char * message,
    const UnicodeString & expected,
    const UnicodeString & actual,
    bool possibleDataError) {
	if(expected != actual) {
		if(possibleDataError) {
			dataerrln((UnicodeString)"FAIL: " + message + "; got " +
			    prettify(actual) +
			    "; expected " + prettify(expected));
		}
		else {
			errln((UnicodeString)"FAIL: " + message + "; got " +
			    prettify(actual) +
			    "; expected " + prettify(expected));
		}
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + prettify(actual));
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message, const char * expected, const char * actual) 
{
	assert(expected != nullptr);
	assert(actual != nullptr);
	if(!sstreq(expected, actual)) {
		errln((UnicodeString)"FAIL: " + message + "; got \"" + actual + "\"; expected \"" + expected + "\"");
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got \"" + actual + "\"");
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message, int32_t expected, int32_t actual) 
{
	if(expected != actual) {
		errln((UnicodeString)"FAIL: " + message + "; got " +
		    actual + "=0x" + toHex(actual) +
		    "; expected " + expected + "=0x" + toHex(expected));
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + actual + "=0x" + toHex(actual));
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message, int64_t expected, int64_t actual) 
{
	if(expected != actual) {
		errln((UnicodeString)"FAIL: " + message + "; got int64 " + Int64ToUnicodeString(actual) + "; expected " + Int64ToUnicodeString(expected));
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got int64 " + Int64ToUnicodeString(actual));
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message,
    double expected,
    double actual) {
	bool bothNaN = std::isnan(expected) && std::isnan(actual);
	if(expected != actual && !bothNaN) {
		errln((UnicodeString)"FAIL: " + message + "; got " +
		    actual +
		    "; expected " + expected);
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + actual);
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message,
    bool expected,
    bool actual) {
	if(expected != actual) {
		errln((UnicodeString)"FAIL: " + message + "; got " +
		    toString(actual) +
		    "; expected " + toString(expected));
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + toString(actual));
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message,
    UErrorCode expected,
    UErrorCode actual) {
	if(expected != actual) {
		errln((UnicodeString)"FAIL: " + message + "; got " +
		    u_errorName(actual) +
		    "; expected " + u_errorName(expected));
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + u_errorName(actual));
	}
#endif
	return TRUE;
}

bool IntlTest::assertEquals(const char * message,
    const UnicodeSet & expected,
    const UnicodeSet & actual) {
	IcuTestErrorCode status(*this, "assertEqualsUniSet");
	if(expected != actual) {
		errln((UnicodeString)"FAIL: " + message + "; got " +
		    toString(actual, status) +
		    "; expected " + toString(expected, status));
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + toString(actual, status));
	}
#endif
	return TRUE;
}

#if !UCONFIG_NO_FORMATTING
bool IntlTest::assertEquals(const char * message,
    const Formattable& expected,
    const Formattable& actual,
    bool possibleDataError) {
	if(expected != actual) {
		if(possibleDataError) {
			dataerrln((UnicodeString)"FAIL: " + message + "; got " +
			    toString(actual) +
			    "; expected " + toString(expected));
		}
		else {
			errln((UnicodeString)"FAIL: " + message + "; got " +
			    toString(actual) +
			    "; expected " + toString(expected));
		}
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + toString(actual));
	}
#endif
	return TRUE;
}

#endif

std::string vectorToString(const std::vector<std::string>& strings) {
	std::string result = "{";
	bool first = true;
	for(auto element : strings) {
		if(first) {
			first = false;
		}
		else {
			result += ", ";
		}
		result += "\"";
		result += element;
		result += "\"";
	}
	result += "}";
	return result;
}

bool IntlTest::assertEquals(const char * message, const std::vector<std::string>& expected, const std::vector<std::string>& actual) 
{
	if(expected != actual) {
		std::string expectedAsString = vectorToString(expected);
		std::string actualAsString = vectorToString(actual);
		errln((UnicodeString)"FAIL: " + message +
		    "; got " + actualAsString.c_str() +
		    "; expected " + expectedAsString.c_str());
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)"Ok: " + message + "; got " + vectorToString(actual).c_str());
	}
#endif
	return TRUE;
}

bool IntlTest::assertNotEquals(const char * message, int32_t expectedNot, int32_t actual) 
{
	if(expectedNot == actual) {
		errln((UnicodeString)("FAIL: ") + message + "; got " + actual + "=0x" + toHex(actual) + "; expected != " + expectedNot);
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)("Ok: ") + message + "; got " + actual + "=0x" + toHex(actual) + " != " + expectedNot);
	}
#endif
	return TRUE;
}

bool IntlTest::assertEqualsNear(const char * message, double expected, double actual, double delta) 
{
	if(std::isnan(delta) || std::isinf(delta)) {
		errln((UnicodeString)("FAIL: ") + message + "; nonsensical delta " + delta + " - delta may not be NaN or Inf");
		return FALSE;
	}
	bool bothNaN = std::isnan(expected) && std::isnan(actual);
	double difference = std::abs(expected - actual);
	if(expected != actual && (difference > delta || std::isnan(difference)) && !bothNaN) {
		errln((UnicodeString)("FAIL: ") + message + "; got " + actual + "; expected " + expected +
		    "; acceptable delta " + delta);
		return FALSE;
	}
#ifdef VERBOSE_ASSERTIONS
	else {
		logln((UnicodeString)("Ok: ") + message + "; got " + actual);
	}
#endif
	return TRUE;
}

static char ASSERT_BUF[256];

static const char * extractToAssertBuf(const UnicodeString & message) 
{
	UnicodeString buf;
	escape(message, buf);
	buf.extract(0, 0x7FFFFFFF, ASSERT_BUF, sizeof(ASSERT_BUF)-1, 0);
	ASSERT_BUF[sizeof(ASSERT_BUF)-1] = 0;
	return ASSERT_BUF;
}

bool IntlTest::assertTrue(const UnicodeString & message, bool condition, bool quiet, bool possibleDataError) 
{
	return assertTrue(extractToAssertBuf(message), condition, quiet, possibleDataError);
}

bool IntlTest::assertFalse(const UnicodeString & message, bool condition, bool quiet, bool possibleDataError) 
{
	return assertFalse(extractToAssertBuf(message), condition, quiet, possibleDataError);
}

bool IntlTest::assertSuccess(const UnicodeString & message, UErrorCode ec) 
{
	return assertSuccess(extractToAssertBuf(message), ec);
}

bool IntlTest::assertEquals(const UnicodeString & message, const UnicodeString & expected, const UnicodeString & actual, bool possibleDataError) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual, possibleDataError);
}

bool IntlTest::assertEquals(const UnicodeString & message, const char * expected, const char * actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, bool expected, bool actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, int32_t expected, int32_t actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, int64_t expected, int64_t actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, double expected, double actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, UErrorCode expected, UErrorCode actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, const UnicodeSet & expected, const UnicodeSet & actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertEquals(const UnicodeString & message, const std::vector<std::string>& expected, const std::vector<std::string>& actual) 
{
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

bool IntlTest::assertNotEquals(const UnicodeString & message, int32_t expectedNot, int32_t actual) 
{
	return assertNotEquals(extractToAssertBuf(message), expectedNot, actual);
}

bool IntlTest::assertEqualsNear(const UnicodeString & message, double expected, double actual, double delta) 
{
	return assertEqualsNear(extractToAssertBuf(message), expected, actual, delta);
}

#if !UCONFIG_NO_FORMATTING
bool IntlTest::assertEquals(const UnicodeString & message,
    const Formattable& expected,
    const Formattable& actual) {
	return assertEquals(extractToAssertBuf(message), expected, actual);
}

#endif

void IntlTest::setProperty(const char * propline) 
{
	if(numProps < kMaxProps) {
		proplines[numProps] = propline;
	}
	numProps++;
}

const char * IntlTest::getProperty(const char * prop) 
{
	const char * val = NULL;
	for(int32_t i = 0; i < numProps; i++) {
		int32_t plen = static_cast<int32_t>(strlen(prop));
		if((int32_t)strlen(proplines[i]) > plen + 1
		 && proplines[i][plen] == '='
		 && uprv_strncmp(proplines[i], prop, plen) == 0) {
			val = &(proplines[i][plen+1]);
			break;
		}
	}
	return val;
}

//-------------------------------------------------------------------------------
//
//    ReadAndConvertFile   Read a text data file, convert it to UChars, and
//    return the data in one big char16_t * buffer, which the caller must delete.
//
//    parameters:
//          fileName:   the name of the file, with no directory part.  The test data directory
//                      is assumed.
//          ulen        an out parameter, receives the actual length (in UChars) of the file data.
//          encoding    The file encoding.  If the file contains a BOM, that will override the encoding
//                      specified here.  The BOM, if it exists, will be stripped from the returned data.
//                      Pass NULL for the system default encoding.
//          status
//    returns:
//                      The file data, converted to char16_t.
//                      The caller must delete this when done with
//                           delete [] theBuffer;
//
//
//--------------------------------------------------------------------------------
char16_t * IntlTest::ReadAndConvertFile(const char * fileName, int &ulen, const char * encoding, UErrorCode & status) 
{
	char16_t * retPtr  = NULL;
	char * fileBuf = NULL;
	UConverter * conv     = NULL;
	FILE * f       = NULL;
	ulen = 0;
	if(U_FAILURE(status)) {
		return retPtr;
	}
	//
	//  Open the file.
	//
	f = fopen(fileName, "rb");
	if(f == 0) {
		dataerrln("Error opening test data file %s\n", fileName);
		status = U_FILE_ACCESS_ERROR;
		return NULL;
	}
	//
	//  Read it in
	//
	int fileSize;
	int amt_read;
	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	fileBuf = new char[fileSize];
	fseek(f, 0, SEEK_SET);
	amt_read = static_cast<int>(fread(fileBuf, 1, fileSize, f));
	if(amt_read != fileSize || fileSize <= 0) {
		errln("Error reading test data file.");
		goto cleanUpAndReturn;
	}
	{
		//
		// Look for a Unicode Signature (BOM) on the data just read
		//
		int32_t signatureLength;
		const char * bomEncoding;
		const char * fileBufC = fileBuf;
		bomEncoding = ucnv_detectUnicodeSignature(fileBuf, fileSize, &signatureLength, &status);
		if(bomEncoding!=NULL) {
			fileBufC  += signatureLength;
			fileSize  -= signatureLength;
			encoding = bomEncoding;
		}
		//
		// Open a converter to take the rule file to UTF-16
		//
		conv = ucnv_open(encoding, &status);
		if(U_FAILURE(status)) {
			goto cleanUpAndReturn;
		}
		//
		// Convert the rules to char16_t.
		//  Preflight first to determine required buffer size.
		//
		ulen = ucnv_toUChars(conv, NULL/*dest*/, 0/*destCapacity*/, fileBufC, fileSize, &status);
		if(status == U_BUFFER_OVERFLOW_ERROR) {
			// Buffer Overflow is expected from the preflight operation.
			status = U_ZERO_ERROR;
			retPtr = new char16_t[ulen+1];
			ucnv_toUChars(conv, retPtr/*dest*/, ulen+1, fileBufC, fileSize, &status);
		}
	}
cleanUpAndReturn:
	fclose(f);
	delete [] fileBuf;
	ucnv_close(conv);
	if(U_FAILURE(status)) {
		errln("ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
		delete []retPtr;
		retPtr = 0;
		ulen   = 0;
	}
	return retPtr;
}

#if !UCONFIG_NO_BREAK_ITERATION
bool LSTMDataIsBuilt() {
	// If we can find the LSTM data, the RBBI will use the LSTM engine.
	// So we skip the test which depending on the dictionary data.
	UErrorCode status = U_ZERO_ERROR;
	DeleteLSTMData(CreateLSTMDataForScript(USCRIPT_THAI, status));
	bool thaiDataIsBuilt = U_SUCCESS(status);
	status = U_ZERO_ERROR;
	DeleteLSTMData(CreateLSTMDataForScript(USCRIPT_MYANMAR, status));
	bool burmeseDataIsBuilt = U_SUCCESS(status);
	return thaiDataIsBuilt | burmeseDataIsBuilt;
}

bool IntlTest::skipLSTMTest() { return !LSTMDataIsBuilt(); }
bool IntlTest::skipDictionaryTest() { return LSTMDataIsBuilt(); }

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
