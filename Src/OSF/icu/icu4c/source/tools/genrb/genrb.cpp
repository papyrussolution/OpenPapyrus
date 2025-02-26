// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 *
 *   Copyright (C) 1998-2016, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *
 * File genrb.cpp
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   05/25/99    stephen     Creation.
 *   5/10/01     Ram         removed ustdio dependency
 *******************************************************************************
 */
#include <icu-internal.h>
#pragma hdrstop
#include "genrb.h"
#include "charstr.h"
#include "filterrb.h"
#include "reslist.h"
#include "ucmndata.h"  /* TODO: for reading the pool bundle */

U_NAMESPACE_USE

/* Protos */
void  processFile(const char * filename, const char * cp, const char * inputDir, const char * outputDir, const char * filterDir,
    const char * packageName, SRBRoot * newPoolBundle, bool omitBinaryCollation, UErrorCode & status);
static char * make_res_filename(const char * filename, const char * outputDir, const char * packageName, UErrorCode & status);

/* File suffixes */
#define RES_SUFFIX ".res"
#define COL_SUFFIX ".col"

const char * gCurrentFileName = NULL;
#ifdef XP_MAC_CONSOLE
	#include <console.h>
#endif

void ResFile::close() {
	delete[] fBytes;
	fBytes = NULL;
	delete fStrings;
	fStrings = NULL;
}

enum {
	HELP1,
	HELP2,
	VERBOSE,
	QUIET,
	VERSION,
	SOURCEDIR,
	DESTDIR,
	ENCODING,
	ICUDATADIR,
	WRITE_JAVA,
	COPYRIGHT,
	JAVA_PACKAGE,
	BUNDLE_NAME,
	WRITE_XLIFF,
	STRICT__, // @sobolev STRICT-->STRICT__ name conflict resolving
	NO_BINARY_COLLATION,
	LANGUAGE,
	NO_COLLATION_RULES,
	FORMAT_VERSION,
	WRITE_POOL_BUNDLE,
	USE_POOL_BUNDLE,
	INCLUDE_UNIHAN_COLL,
	FILTERDIR
};

UOption options[] = {
	UOPTION_HELP_H,
	UOPTION_HELP_QUESTION_MARK,
	UOPTION_VERBOSE,
	UOPTION_QUIET,
	UOPTION_VERSION,
	UOPTION_SOURCEDIR,
	UOPTION_DESTDIR,
	UOPTION_ENCODING,
	UOPTION_ICUDATADIR,
	UOPTION_WRITE_JAVA,
	UOPTION_COPYRIGHT,
	UOPTION_DEF("java-package", '\x01', UOPT_REQUIRES_ARG),
	UOPTION_BUNDLE_NAME,
	UOPTION_DEF("write-xliff", 'x', UOPT_OPTIONAL_ARG),
	UOPTION_DEF("strict",    'k', UOPT_NO_ARG),               /* 14 */
	UOPTION_DEF("noBinaryCollation", 'C', UOPT_NO_ARG),              /* 15 */
	UOPTION_DEF("language",  'l', UOPT_REQUIRES_ARG),               /* 16 */
	UOPTION_DEF("omitCollationRules", 'R', UOPT_NO_ARG),              /* 17 */
	UOPTION_DEF("formatVersion", '\x01', UOPT_REQUIRES_ARG),              /* 18 */
	UOPTION_DEF("writePoolBundle", '\x01', UOPT_OPTIONAL_ARG),              /* 19 */
	UOPTION_DEF("usePoolBundle", '\x01', UOPT_OPTIONAL_ARG),              /* 20 */
	UOPTION_DEF("includeUnihanColl", '\x01', UOPT_NO_ARG),              /* 21 */ /* temporary, don't display in
	                                                                       usage info */
	UOPTION_DEF("filterDir", '\x01', UOPT_OPTIONAL_ARG),               /* 22 */
};

static bool write_java = FALSE;
static bool write_xliff = FALSE;
static const char * outputEnc = "";

static ResFile poolBundle;

/*added by Jing*/
static const char * language = NULL;
static const char * xliffOutputFileName = NULL;
int main(int argc,
    char * argv[])
{
	UErrorCode status = U_ZERO_ERROR;
	const char * arg       = NULL;
	const char * outputDir = NULL; /* NULL = no output directory, use current */
	const char * inputDir  = NULL;
	const char * filterDir = NULL;
	const char * encoding  = "";
	int i;
	bool illegalArg = FALSE;

	U_MAIN_INIT_ARGS(argc, argv);

	options[JAVA_PACKAGE].value = "com.ibm.icu.impl.data";
	options[BUNDLE_NAME].value = "LocaleElements";
	argc = u_parseArgs(argc, argv, SIZEOFARRAYi(options), options);

	/* error handling, printing usage message */
	if(argc<0) {
		slfprintf_stderr("%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
		illegalArg = TRUE;
	}
	else if(argc<2) {
		illegalArg = TRUE;
	}
	if(options[WRITE_POOL_BUNDLE].doesOccur && options[USE_POOL_BUNDLE].doesOccur) {
		slfprintf_stderr("%s: cannot combine --writePoolBundle and --usePoolBundle\n", argv[0]);
		illegalArg = TRUE;
	}
	if(options[FORMAT_VERSION].doesOccur) {
		const char * s = options[FORMAT_VERSION].value;
		if(strlen(s) != 1 || (s[0] < '1' && '3' < s[0])) {
			slfprintf_stderr("%s: unsupported --formatVersion %s\n", argv[0], s);
			illegalArg = TRUE;
		}
		else if(s[0] == '1' && (options[WRITE_POOL_BUNDLE].doesOccur || options[USE_POOL_BUNDLE].doesOccur)) {
			slfprintf_stderr("%s: cannot combine --formatVersion 1 with --writePoolBundle or --usePoolBundle\n", argv[0]);
			illegalArg = TRUE;
		}
		else {
			setFormatVersion(s[0] - '0');
		}
	}
	if((options[JAVA_PACKAGE].doesOccur || options[BUNDLE_NAME].doesOccur) && !options[WRITE_JAVA].doesOccur) {
		slfprintf_stderr("%s error: command line argument --java-package or --bundle-name without --write-java\n", argv[0]);
		illegalArg = TRUE;
	}
	if(options[VERSION].doesOccur) {
		slfprintf_stderr("%s version %s (ICU version %s).\n%s\n", argv[0], GENRB_VERSION, U_ICU_VERSION, U_COPYRIGHT_STRING);
		if(!illegalArg) {
			return U_ZERO_ERROR;
		}
	}
	if(illegalArg || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		/*
		 * Broken into chunks because the C89 standard says the minimum
		 * required supported string length is 509 bytes.
		 */
		slfprintf_stderr("Usage: %s [OPTIONS] [FILES]\n\tReads the list of resource bundle source files and creates\n\tbinary version of resource bundles (.res files)\n", argv[0]);
		slfprintf_stderr("Options:\n"
		    "\t-h or -? or --help       this usage text\n"
		    "\t-q or --quiet            do not display warnings\n"
		    "\t-v or --verbose          print extra information when processing files\n"
		    "\t-V or --version          prints out version number and exits\n"
		    "\t-c or --copyright        include copyright notice\n");
		slfprintf_stderr("\t-e or --encoding         encoding of source files\n"
		    "\t-d or --destdir          destination directory, followed by the path, defaults to '%s'\n"
		    "\t-s or --sourcedir        source directory for files followed by path, defaults to '%s'\n"
		    "\t-i or --icudatadir       directory for locating any needed intermediate data files,\n"
		    "\t                         followed by path, defaults to '%s'\n", u_getDataDirectory(), u_getDataDirectory(), u_getDataDirectory());
		slfprintf_stderr("\t-j or --write-java       write a Java ListResourceBundle for ICU4J, followed by optional encoding\n"
		    "\t                         defaults to ASCII and \\uXXXX format.\n"
		    "\t      --java-package     For --write-java: package name for writing the ListResourceBundle,\n"
		    "\t                         defaults to com.ibm.icu.impl.data\n");
		slfprintf_stderr("\t-b or --bundle-name      For --write-java: root resource bundle name for writing the ListResourceBundle,\n"
		    "\t                         defaults to LocaleElements\n"
		    "\t-x or --write-xliff      write an XLIFF file for the resource bundle. Followed by\n"
		    "\t                         an optional output file name.\n"
		    "\t-k or --strict           use pedantic parsing of syntax\n"
		    /*added by Jing*/
		    "\t-l or --language         for XLIFF: language code compliant with BCP 47.\n");
		slfprintf_stderr("\t-C or --noBinaryCollation  do not generate binary collation image;\n"
		    "\t                           makes .res file smaller but collator instantiation much slower;\n"
		    "\t                           maintains ability to get tailoring rules\n"
		    "\t-R or --omitCollationRules do not include collation (tailoring) rules;\n"
		    "\t                           makes .res file smaller and maintains collator instantiation speed\n"
		    "\t                           but tailoring rules will not be available (they are rarely used)\n");
		slfprintf_stderr("\t      --formatVersion      write a .res file compatible with the requested formatVersion (single digit);\n"
		    "\t                           for example, --formatVersion 1\n");
		slfprintf_stderr("\t      --writePoolBundle [directory]  write a pool.res file with all of the keys of all input bundles\n"
		    "\t      --usePoolBundle [directory]  point to keys from the pool.res keys pool bundle if they are available there;\n"
		    "\t                           makes .res files smaller but dependent on the pool bundle\n"
		    "\t                           (--writePoolBundle and --usePoolBundle cannot be combined)\n");
		slfprintf_stderr("\t      --filterDir          Input directory where filter files are available.\n"
		    "\t                           For more on filter files, see ICU Data Build Tool.\n");
		return illegalArg ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
	}
	if(options[VERBOSE].doesOccur) {
		setVerbose(TRUE);
	}
	if(options[QUIET].doesOccur) {
		setShowWarning(FALSE);
	}
	if(options[STRICT__].doesOccur) {
		setStrict(TRUE);
	}
	if(options[COPYRIGHT].doesOccur) {
		setIncludeCopyright(TRUE);
	}

	if(options[SOURCEDIR].doesOccur) {
		inputDir = options[SOURCEDIR].value;
	}
	if(options[DESTDIR].doesOccur) {
		outputDir = options[DESTDIR].value;
	}
	if(options[FILTERDIR].doesOccur) {
		filterDir = options[FILTERDIR].value;
	}
	if(options[ENCODING].doesOccur) {
		encoding = options[ENCODING].value;
	}
	if(options[ICUDATADIR].doesOccur) {
		u_setDataDirectory(options[ICUDATADIR].value);
	}
	/* Initialize ICU */
	u_init(&status);
	if(U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		/* Note: u_init() will try to open ICU property data.
		 *       failures here are expected when building ICU from scratch.
		 *       ignore them.
		 */
		slfprintf_stderr("%s: can not initialize ICU.  status = %s\n",
		    argv[0], u_errorName(status));
		exit(1);
	}
	status = U_ZERO_ERROR;
	if(options[WRITE_JAVA].doesOccur) {
		write_java = TRUE;
		outputEnc = options[WRITE_JAVA].value;
	}
	if(options[WRITE_XLIFF].doesOccur) {
		write_xliff = TRUE;
		if(options[WRITE_XLIFF].value != NULL) {
			xliffOutputFileName = options[WRITE_XLIFF].value;
		}
	}
	initParser();
	/*added by Jing*/
	if(options[LANGUAGE].doesOccur) {
		language = options[LANGUAGE].value;
	}
	LocalPointer<SRBRoot> newPoolBundle;
	if(options[WRITE_POOL_BUNDLE].doesOccur) {
		newPoolBundle.adoptInsteadAndCheckErrorCode(new SRBRoot(NULL, TRUE, status), status);
		if(U_FAILURE(status)) {
			slfprintf_stderr("unable to create an empty bundle for the pool keys: %s\n", u_errorName(status));
			return status;
		}
		else {
			const char * poolResName = "pool.res";
			char * nameWithoutSuffix = static_cast<char *>(uprv_malloc(strlen(poolResName) + 1));
			if(nameWithoutSuffix == NULL) {
				slfprintf_stderr("out of memory error\n");
				return U_MEMORY_ALLOCATION_ERROR;
			}
			strcpy(nameWithoutSuffix, poolResName);
			*uprv_strrchr(nameWithoutSuffix, '.') = 0;
			newPoolBundle->fLocale = nameWithoutSuffix;
		}
	}

	if(options[USE_POOL_BUNDLE].doesOccur) {
		const char * poolResName = "pool.res";
		FileStream * poolFile;
		int32_t poolFileSize;
		int32_t indexLength;
		/*
		 * TODO: Consolidate inputDir/filename handling from main() and processFile()
		 * into a common function, and use it here as well.
		 * Try to create toolutil functions for dealing with dir/filenames and
		 * loading ICU data files without udata_open().
		 * Share code with icupkg?
		 * Also, make_res_filename() seems to be unused. Review and remove.
		 */
		CharString poolFileName;
		if(options[USE_POOL_BUNDLE].value!=NULL) {
			poolFileName.append(options[USE_POOL_BUNDLE].value, status);
		}
		else if(inputDir) {
			poolFileName.append(inputDir, status);
		}
		poolFileName.appendPathPart(poolResName, status);
		if(U_FAILURE(status)) {
			return status;
		}
		poolFile = T_FileStream_open(poolFileName.data(), "rb");
		if(poolFile == NULL) {
			slfprintf_stderr("unable to open pool bundle file %s\n", poolFileName.data());
			return 1;
		}
		poolFileSize = T_FileStream_size(poolFile);
		if(poolFileSize < 32) {
			slfprintf_stderr("the pool bundle file %s is too small\n", poolFileName.data());
			return 1;
		}
		poolBundle.fBytes = new uint8_t[(poolFileSize + 15) & ~15];
		if(poolFileSize > 0 && poolBundle.fBytes == NULL) {
			slfprintf_stderr("unable to allocate memory for the pool bundle file %s\n", poolFileName.data());
			return U_MEMORY_ALLOCATION_ERROR;
		}

		UDataSwapper * ds;
		const DataHeader * header;
		int32_t bytesRead = T_FileStream_read(poolFile, poolBundle.fBytes, poolFileSize);
		if(bytesRead != poolFileSize) {
			slfprintf_stderr("unable to read the pool bundle file %s\n", poolFileName.data());
			return 1;
		}
		/*
		 * Swap the pool bundle so that a single checked-in file can be used.
		 * The swapper functions also test that the data looks like
		 * a well-formed .res file.
		 */
		ds = udata_openSwapperForInputData(poolBundle.fBytes, bytesRead,
			U_IS_BIG_ENDIAN, U_CHARSET_FAMILY, &status);
		if(U_FAILURE(status)) {
			slfprintf_stderr("udata_openSwapperForInputData(pool bundle %s) failed: %s\n",
			    poolFileName.data(), u_errorName(status));
			return status;
		}
		ures_swap(ds, poolBundle.fBytes, bytesRead, poolBundle.fBytes, &status);
		udata_closeSwapper(ds);
		if(U_FAILURE(status)) {
			slfprintf_stderr("ures_swap(pool bundle %s) failed: %s\n",
			    poolFileName.data(), u_errorName(status));
			return status;
		}
		header = (const DataHeader*)poolBundle.fBytes;
		if(header->info.formatVersion[0] < 2) {
			slfprintf_stderr("invalid format of pool bundle file %s\n", poolFileName.data());
			return U_INVALID_FORMAT_ERROR;
		}
		const int32_t * pRoot = (const int32_t*)(
			(const char *)header + header->dataHeader.headerSize);
		poolBundle.fIndexes = pRoot + 1;
		indexLength = poolBundle.fIndexes[URES_INDEX_LENGTH] & 0xff;
		if(indexLength <= URES_INDEX_POOL_CHECKSUM) {
			slfprintf_stderr("insufficient indexes[] in pool bundle file %s\n", poolFileName.data());
			return U_INVALID_FORMAT_ERROR;
		}
		int32_t keysBottom = 1 + indexLength;
		int32_t keysTop = poolBundle.fIndexes[URES_INDEX_KEYS_TOP];
		poolBundle.fKeys = (const char *)(pRoot + keysBottom);
		poolBundle.fKeysLength = (keysTop - keysBottom) * 4;
		poolBundle.fChecksum = poolBundle.fIndexes[URES_INDEX_POOL_CHECKSUM];

		for(i = 0; i < poolBundle.fKeysLength; ++i) {
			if(poolBundle.fKeys[i] == 0) {
				++poolBundle.fKeysCount;
			}
		}

		// 16BitUnits[] begins with strings-v2.
		// The strings-v2 may optionally be terminated by what looks like
		// an explicit string length that exceeds the number of remaining 16-bit units.
		int32_t stringUnitsLength = (poolBundle.fIndexes[URES_INDEX_16BIT_TOP] - keysTop) * 2;
		if(stringUnitsLength >= 2 && getFormatVersion() >= 3) {
			poolBundle.fStrings = new PseudoListResource(NULL, status);
			if(poolBundle.fStrings == NULL) {
				slfprintf_stderr("unable to allocate memory for the pool bundle strings %s\n",
				    poolFileName.data());
				return U_MEMORY_ALLOCATION_ERROR;
			}
			// The PseudoListResource constructor call did not allocate further memory.
			assert(U_SUCCESS(status));
			const char16_t * p = (const char16_t*)(pRoot + keysTop);
			int32_t remaining = stringUnitsLength;
			do {
				int32_t first = *p;
				int8_t numCharsForLength;
				int32_t length;
				if(!U16_IS_TRAIL(first)) {
					// NUL-terminated
					numCharsForLength = 0;
					for(length = 0;
					    length < remaining && p[length] != 0;
					    ++length) {
					}
				}
				else if(first < 0xdfef) {
					numCharsForLength = 1;
					length = first & 0x3ff;
				}
				else if(first < 0xdfff && remaining >= 2) {
					numCharsForLength = 2;
					length = ((first - 0xdfef) << 16) | p[1];
				}
				else if(first == 0xdfff && remaining >= 3) {
					numCharsForLength = 3;
					length = ((int32_t)p[1] << 16) | p[2];
				}
				else {
					break; // overrun
				}
				// Check for overrun before changing remaining,
				// so that it is always accurate after the loop body.
				if((numCharsForLength + length) >= remaining ||
				    p[numCharsForLength + length] != 0) {
					break; // overrun or explicitly terminated
				}
				int32_t poolStringIndex = stringUnitsLength - remaining;
				// Maximum pool string index when suffix-sharing the last character.
				int32_t maxStringIndex = poolStringIndex + numCharsForLength + length - 1;
				if(maxStringIndex >= RES_MAX_OFFSET) {
					// pool string index overrun
					break;
				}
				p += numCharsForLength;
				remaining -= numCharsForLength;
				if(length != 0) {
					StringResource * sr =
					    new StringResource(poolStringIndex, numCharsForLength,
						p, length, status);
					if(sr == NULL) {
						slfprintf_stderr("unable to allocate memory for a pool bundle string %s\n",
						    poolFileName.data());
						return U_MEMORY_ALLOCATION_ERROR;
					}
					poolBundle.fStrings->add(sr);
					poolBundle.fStringIndexLimit = maxStringIndex + 1;
					// The StringResource constructor did not allocate further memory.
					assert(U_SUCCESS(status));
				}
				p += length + 1;
				remaining -= length + 1;
			} while(remaining > 0);
			if(poolBundle.fStrings->fCount == 0) {
				delete poolBundle.fStrings;
				poolBundle.fStrings = NULL;
			}
		}

		T_FileStream_close(poolFile);
		setUsePoolBundle(TRUE);
		if(isVerbose() && poolBundle.fStrings != NULL) {
			printf("number of shared strings: %d\n", (int)poolBundle.fStrings->fCount);
			int32_t length = poolBundle.fStringIndexLimit + 1; // incl. last NUL
			printf("16-bit units for strings: %6d = %6d bytes\n",
			    (int)length, (int)length * 2);
		}
	}

	if(!options[FORMAT_VERSION].doesOccur && getFormatVersion() == 3 &&
	    poolBundle.fStrings == NULL &&
	    !options[WRITE_POOL_BUNDLE].doesOccur) {
		// If we just default to formatVersion 3
		// but there are no pool bundle strings to share
		// and we do not write a pool bundle,
		// then write formatVersion 2 which is just as good.
		setFormatVersion(2);
	}

	if(options[INCLUDE_UNIHAN_COLL].doesOccur) {
		puts("genrb option --includeUnihanColl ignored: \n"
		    "CLDR 26/ICU 54 unihan data is small, except\n"
		    "the ucadata-unihan.icu version of the collation root data\n"
		    "is about 300kB larger than the ucadata-implicithan.icu version.");
	}

	if((argc-1)!=1) {
		printf("genrb number of files: %d\n", argc - 1);
	}
	/* generate the binary files */
	for(i = 1; i < argc; ++i) {
		status = U_ZERO_ERROR;
		arg    = getLongPathname(argv[i]);

		CharString theCurrentFileName;
		if(inputDir) {
			theCurrentFileName.append(inputDir, status);
		}
		theCurrentFileName.appendPathPart(arg, status);
		if(U_FAILURE(status)) {
			break;
		}

		gCurrentFileName = theCurrentFileName.data();
		if(isVerbose()) {
			printf("Processing file \"%s\"\n", theCurrentFileName.data());
		}
		processFile(arg, encoding, inputDir, outputDir, filterDir, NULL,
		    newPoolBundle.getAlias(),
		    options[NO_BINARY_COLLATION].doesOccur, status);
	}

	poolBundle.close();

	if(U_SUCCESS(status) && options[WRITE_POOL_BUNDLE].doesOccur) {
		const char * writePoolDir;
		if(options[WRITE_POOL_BUNDLE].value!=NULL) {
			writePoolDir = options[WRITE_POOL_BUNDLE].value;
		}
		else {
			writePoolDir = outputDir;
		}
		char outputFileName[256];
		newPoolBundle->write(writePoolDir, NULL, outputFileName, sizeof(outputFileName), status);
		if(U_FAILURE(status)) {
			slfprintf_stderr("unable to write the pool bundle: %s\n", u_errorName(status));
		}
	}

	u_cleanup();

	/* Don't return warnings as a failure */
	if(U_SUCCESS(status)) {
		return 0;
	}

	return status;
}

/* Process a file */
void processFile(const char * filename, const char * cp,
    const char * inputDir, const char * outputDir, const char * filterDir,
    const char * packageName,
    SRBRoot * newPoolBundle,
    bool omitBinaryCollation, UErrorCode & status) {
	LocalPointer<SRBRoot> data;
	LocalUCHARBUFPointer ucbuf;
	CharString openFileName;
	CharString inputDirBuf;

	char outputFileName[256];
	int32_t dirlen  = 0;

	if(U_FAILURE(status)) {
		return;
	}
	if(filename==NULL) {
		status = U_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	if(inputDir == NULL) {
		const char * filenameBegin = uprv_strrchr(filename, U_FILE_SEP_CHAR);
		if(filenameBegin != NULL) {
			/*
			 * When a filename ../../../data/root.txt is specified,
			 * we presume that the input directory is ../../../data
			 * This is very important when the resource file includes
			 * another file, like UCARules.txt or thaidict.brk.
			 */
			int32_t filenameSize = (int32_t)(filenameBegin - filename + 1);
			inputDirBuf.append(filename, filenameSize, status);

			inputDir = inputDirBuf.data();
			dirlen  = inputDirBuf.length();
		}
	}
	else {
		dirlen  = (int32_t)strlen(inputDir);

		if(inputDir[dirlen-1] != U_FILE_SEP_CHAR) {
			/*
			 * append the input dir to openFileName if the first char in
			 * filename is not file separation char and the last char input directory is  not '.'.
			 * This is to support :
			 * genrb -s. /home/icu/data
			 * genrb -s. icu/data
			 * The user cannot mix notations like
			 * genrb -s. /icu/data --- the absolute path specified. -s redundant
			 * user should use
			 * genrb -s. icu/data  --- start from CWD and look in icu/data dir
			 */
			if((filename[0] != U_FILE_SEP_CHAR) && (inputDir[dirlen-1] !='.')) {
				openFileName.append(inputDir, status);
			}
		}
		else {
			openFileName.append(inputDir, status);
		}
	}
	openFileName.appendPathPart(filename, status);

	// Test for CharString failure
	if(U_FAILURE(status)) {
		return;
	}

	ucbuf.adoptInstead(ucbuf_open(openFileName.data(), &cp, getShowWarning(), TRUE, &status));
	if(status == U_FILE_ACCESS_ERROR) {
		slfprintf_stderr("couldn't open file %s\n", openFileName.data());
		return;
	}
	if(ucbuf.isNull() || U_FAILURE(status)) {
		slfprintf_stderr("An error occurred processing file %s. Error: %s\n",
		    openFileName.data(), u_errorName(status));
		return;
	}
	/* auto detected popular encodings? */
	if(cp!=NULL && isVerbose()) {
		printf("autodetected encoding %s\n", cp);
	}
	/* Parse the data into an SRBRoot */
	data.adoptInstead(parse(ucbuf.getAlias(), inputDir, outputDir, filename,
	    !omitBinaryCollation, options[NO_COLLATION_RULES].doesOccur, &status));

	if(data.isNull() || U_FAILURE(status)) {
		slfprintf_stderr("couldn't parse the file %s. Error:%s\n", filename, u_errorName(status));
		return;
	}

	// Run filtering before writing pool bundle
	if(filterDir != nullptr) {
		CharString filterFileName(filterDir, status);
		filterFileName.appendPathPart(filename, status);
		if(U_FAILURE(status)) {
			return;
		}

		// Open the file and read it into filter
		SimpleRuleBasedPathFilter filter;
		std::ifstream f(filterFileName.data());
		if(f.fail()) {
			std::cerr << "genrb error: unable to open " << filterFileName.data() << std::endl;
			status = U_FILE_ACCESS_ERROR;
			return;
		}
		std::string currentLine;
		while(std::getline(f, currentLine)) {
			// Ignore # comments and empty lines
			if(currentLine.empty() || currentLine[0] == '#') {
				continue;
			}
			filter.addRule(currentLine, status);
			if(U_FAILURE(status)) {
				return;
			}
		}

		if(isVerbose()) {
			filter.print(std::cout);
		}

		// Apply the filter to the data
		ResKeyPath path;
		data->fRoot->applyFilter(filter, path, data.getAlias());
	}

	if(options[WRITE_POOL_BUNDLE].doesOccur) {
		data->fWritePoolBundle = newPoolBundle;
		data->compactKeys(status);
		int32_t newKeysLength;
		const char * newKeys = data->getKeyBytes(&newKeysLength);
		newPoolBundle->addKeyBytes(newKeys, newKeysLength, status);
		if(U_FAILURE(status)) {
			slfprintf_stderr("bundle_compactKeys(%s) or bundle_getKeyBytes() failed: %s\n",
			    filename, u_errorName(status));
			return;
		}
		/* count the number of just-added key strings */
		for(const char * newKeysLimit = newKeys + newKeysLength; newKeys < newKeysLimit; ++newKeys) {
			if(*newKeys == 0) {
				++newPoolBundle->fKeysCount;
			}
		}
	}

	if(options[USE_POOL_BUNDLE].doesOccur) {
		data->fUsePoolBundle = &poolBundle;
	}

	/* Determine the target rb filename */
	uprv_free(make_res_filename(filename, outputDir, packageName, status));
	if(U_FAILURE(status)) {
		slfprintf_stderr("couldn't make the res fileName for  bundle %s. Error:%s\n",
		    filename, u_errorName(status));
		return;
	}
	if(write_java== TRUE) {
		bundle_write_java(data.getAlias(), outputDir, outputEnc,
		    outputFileName, sizeof(outputFileName),
		    options[JAVA_PACKAGE].value, options[BUNDLE_NAME].value, &status);
	}
	else if(write_xliff ==TRUE) {
		bundle_write_xml(data.getAlias(), outputDir, outputEnc,
		    filename, outputFileName, sizeof(outputFileName),
		    language, xliffOutputFileName, &status);
	}
	else {
		/* Write the data to the file */
		data->write(outputDir, packageName, outputFileName, sizeof(outputFileName), status);
	}
	if(U_FAILURE(status)) {
		slfprintf_stderr("couldn't write bundle %s. Error:%s\n", outputFileName, u_errorName(status));
	}
}

/* Generate the target .res file name from the input file name */
static char * make_res_filename(const char * filename,
    const char * outputDir,
    const char * packageName,
    UErrorCode & status) {
	char * basename;
	char * dirname;
	char * resName;

	int32_t pkgLen = 0; /* length of package prefix */

	if(U_FAILURE(status)) {
		return 0;
	}

	if(packageName != NULL) {
		pkgLen = (int32_t)(1 + strlen(packageName));
	}

	/* setup */
	basename = dirname = resName = 0;

	/* determine basename, and compiled file names */
	basename = (char *)uprv_malloc(sizeof(char) * (strlen(filename) + 1));
	if(basename == 0) {
		status = U_MEMORY_ALLOCATION_ERROR;
		goto finish;
	}

	get_basename(basename, filename);

	dirname = (char *)uprv_malloc(sizeof(char) * (strlen(filename) + 1));
	if(dirname == 0) {
		status = U_MEMORY_ALLOCATION_ERROR;
		goto finish;
	}

	get_dirname(dirname, filename);

	if(outputDir == NULL) {
		/* output in same dir as .txt */
		resName = (char *)uprv_malloc(sizeof(char) * (strlen(dirname)
			+ pkgLen
			+ strlen(basename)
			+ strlen(RES_SUFFIX) + 8));
		if(resName == 0) {
			status = U_MEMORY_ALLOCATION_ERROR;
			goto finish;
		}
		strcpy(resName, dirname);
		if(packageName != NULL) {
			uprv_strcat(resName, packageName);
			uprv_strcat(resName, "_");
		}
		uprv_strcat(resName, basename);
	}
	else {
		int32_t dirlen      = (int32_t)strlen(outputDir);
		int32_t basenamelen = (int32_t)strlen(basename);
		resName = (char *)uprv_malloc(sizeof(char) * (dirlen + pkgLen + basenamelen + 8));
		if(resName == NULL) {
			status = U_MEMORY_ALLOCATION_ERROR;
			goto finish;
		}
		strcpy(resName, outputDir);
		if(outputDir[dirlen] != U_FILE_SEP_CHAR) {
			resName[dirlen] = U_FILE_SEP_CHAR;
			resName[dirlen + 1] = '\0';
		}
		if(packageName != NULL) {
			uprv_strcat(resName, packageName);
			uprv_strcat(resName, "_");
		}
		uprv_strcat(resName, basename);
	}
finish:
	uprv_free(basename);
	uprv_free(dirname);
	return resName;
}

/*
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 */
