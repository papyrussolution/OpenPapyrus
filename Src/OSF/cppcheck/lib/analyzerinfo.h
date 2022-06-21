/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2021 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef analyzerinfoH
#define analyzerinfoH

#include "importproject.h"

class ErrorMessage;

/// @addtogroup Core
/// @{

/**
 * @brief Analyzer information
 *
 * Store various analysis information:
 * - checksum
 * - error messages
 * - whole program analysis data
 *
 * The information can be used for various purposes. It allows:
 * - 'make' - only analyze TUs that are changed and generate full report
 * - should be possible to add distributed analysis later
 * - multi-threaded whole program analysis
 */
class CPPCHECKLIB AnalyzerInformation {
public:
	~AnalyzerInformation();

	static void writeFilesTxt(const std::string &buildDir,
	    const std::list<std::string> &sourcefiles,
	    const std::string &userDefines,
	    const std::list<ImportProject::FileSettings> &fileSettings);

	/** Close current TU.analyzerinfo file */
	void close();
	bool analyzeFile(const std::string &buildDir,
	    const std::string &sourcefile,
	    const std::string &cfg,
	    unsigned long long checksum,
	    std::list<ErrorMessage> * errors);
	void reportErr(const ErrorMessage &msg, bool verbose);
	void setFileInfo(const std::string &check, const std::string &fileInfo);
	static std::string getAnalyzerInfoFile(const std::string &buildDir, const std::string &sourcefile, const std::string &cfg);
private:
	std::ofstream mOutputStream;
	std::string mAnalyzerInfoFile;
};

/// @}
#endif // analyzerinfoH
