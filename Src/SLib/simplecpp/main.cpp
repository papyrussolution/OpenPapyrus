/*
 * simplecpp - A simple and high-fidelity C/C++ preprocessor library
 * Copyright (C) 2016-2023 simplecpp team
 */

#include <slib-internal.h>
#pragma hdrstop
#include "simplecpp.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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
	delete rawtokens;
	rawtokens = nullptr;

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
