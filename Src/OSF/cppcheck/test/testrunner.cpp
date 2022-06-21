/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <cppcheck-test-internal.h>
#pragma hdrstop
#include "options.h"

int main(int argc, char * argv[])
{
	// MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef NDEBUG
	try {
#endif
	Preprocessor::macroChar = '$'; // While macroChar is char(1) per default outside test suite, we require it to be
	                               // a human-readable character here.

	options args(argc, argv);

	if(args.help()) {
		TestFixture::printHelp();
		return EXIT_SUCCESS;
	}
	const std::size_t failedTestsCount = TestFixture::runTests(args);
	return (failedTestsCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
#ifdef NDEBUG
}

catch(const InternalError& e) {
	std::cout << e.errorMessage << std::endl;
} catch(const std::exception& error) {
	std::cout << error.what() << std::endl;
} catch(...) {
	std::cout << "Unknown exception" << std::endl;
}
return EXIT_FAILURE;
#endif
}
