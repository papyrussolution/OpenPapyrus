/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
/**
 *
 * @mainpage Cppcheck
 * @version 2.7
 *
 * @section overview_sec Overview
 * Cppcheck is a simple tool for static analysis of C/C++ code.
 *
 * When you write a checker you have access to:
 *  - %Token list - the tokenized code
 *  - Syntax tree - Syntax tree of each expression
 *  - %SymbolDatabase - Information about all types/variables/functions/etc
 *    in the current translation unit
 *  - Library - Configuration of functions/types
 *  - Value flow analysis - Data flow analysis that determine possible values for each token
 *
 * Use --debug-normal on the command line to see debug output for the token list
 * and the syntax tree. If both --debug-normal and --verbose is used, the symbol
 * database is also written.
 *
 * The checks are written in C++.
 *
 * @section detailed_overview_sec Detailed overview
 * This happens when you execute cppcheck from the command line:
 * -# CppCheckExecutor::check this function executes the Cppcheck
 * -# CmdLineParser::parseFromArgs parse command line arguments
 *   - The Settings class is used to maintain settings
 *   - Use FileLister and command line arguments to get files to check
 * -# ThreadExecutor create more instances of CppCheck if needed
 * -# CppCheck::check is called for each file. It checks a single file
 * -# Preprocess the file (through Preprocessor)
 *   - Comments are removed
 *   - Macros are expanded
 * -# Tokenize the file (see Tokenizer)
 * -# Run the runChecks of all check classes.
 * -# Simplify the tokenlist (Tokenizer::simplifyTokenList2)
 * -# Run the runSimplifiedChecks of all check classes
 *
 * When errors are found, they are reported back to the CppCheckExecutor through the ErrorLogger interface.
 */
#include <cppcheck-internal.h>
#pragma hdrstop
#include "cppcheckexecutor.h"

#ifdef _WIN32
//#include <windows.h>
static char exename[1024] = {0};
#endif
#if defined(__APPLE__)
	#include <mach-o/dyld.h>
	static char exename[1024] = {0};
#endif
/**
 * Main function of cppcheck
 *
 * @param argc Passed to CppCheck::parseFromArgs()
 * @param argv Passed to CppCheck::parseFromArgs()
 * @return What CppCheckExecutor::check() returns.
 */
int main(int argc, char* argv[])
{
	// MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
	CppCheckExecutor exec;
#ifdef _WIN32
	GetModuleFileNameA(nullptr, exename, sizeof(exename)/sizeof(exename[0])-1);
	argv[0] = exename;
#endif
#if defined(__APPLE__)
	uint32_t size = sizeof(exename);
	_NSGetExecutablePath(exename, &size);
	argv[0] = exename;
#endif
// *INDENT-OFF*
#ifdef NDEBUG
    try {
#endif
        return exec.check(argc, argv);
#ifdef NDEBUG
    } catch (const InternalError& e) {
        std::cout << e.errorMessage << std::endl;
    } catch (const std::exception& error) {
        std::cout << error.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception" << std::endl;
    }
    return EXIT_FAILURE;
#endif
// *INDENT-ON*
}
