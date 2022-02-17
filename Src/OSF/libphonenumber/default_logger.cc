// Copyright (C) 2011 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Philippe Liard
//
#include <libphonenumber-internal.h>
#pragma hdrstop

namespace i18n {
	namespace phonenumbers {
		void StdoutLogger::WriteMessage(const string& msg) 
		{
			std::cout << " " << msg;
		}
		void StdoutLogger::WriteLevel() 
		{
			int verbosity_level = level();
			if(verbosity_level <= 0) {
				verbosity_level = LOG_FATAL;
			}
			std::cout << "[";
			// Handle verbose logs first.
			if(verbosity_level > LOG_DEBUG) {
				std::cout << "VLOG" << (verbosity_level - LOG_DEBUG);
			}
			else {
				switch(verbosity_level) {
					case LOG_FATAL:   std::cout << "FATAL"; break;
		#ifdef ERROR  // In case ERROR is defined by MSVC (i.e not set to LOG_ERROR).
					case ERROR:
		#endif
					case LOG_ERROR:   std::cout << "ERROR"; break;
					case LOG_WARNING: std::cout << "WARNING"; break;
					case LOG_INFO:    std::cout << "INFO"; break;
					case LOG_DEBUG:   std::cout << "DEBUG"; break;
				}
			}
			std::cout << "]";
		}
	}
}
