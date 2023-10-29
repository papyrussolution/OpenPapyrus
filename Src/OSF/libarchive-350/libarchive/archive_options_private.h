//
// Copyright (c) 2011 Tim Kientzle All rights reserved.
//
#ifndef ARCHIVE_OPTIONS_PRIVATE_H_INCLUDED
#define ARCHIVE_OPTIONS_PRIVATE_H_INCLUDED

//#include "archive_platform.h"
__FBSDID("$FreeBSD$");

//#include "archive_private.h"

typedef int (* option_handler)(Archive * a, const char * mod, const char * opt, const char * val);

int _archive_set_option(Archive * a, const char * mod, const char * opt, const char * val, int magic, const char * fn, option_handler use_option);
int _archive_set_options(Archive * a, const char * options, int magic, const char * fn, option_handler use_option);
int _archive_set_either_option(Archive * a, const char * m, const char * o, const char * v, option_handler use_format_option, option_handler use_filter_option);

#endif
