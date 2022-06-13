/*-
 * Copyright (c) 2011 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef ARCHIVE_OPTIONS_PRIVATE_H_INCLUDED
#define ARCHIVE_OPTIONS_PRIVATE_H_INCLUDED

#include "archive_platform.h"
__FBSDID("$FreeBSD$");

#include "archive_private.h"

typedef int (* option_handler)(Archive * a, const char * mod, const char * opt, const char * val);

int _archive_set_option(Archive * a, const char * mod, const char * opt, const char * val, int magic, const char * fn, option_handler use_option);
int _archive_set_options(Archive * a, const char * options, int magic, const char * fn, option_handler use_option);
int _archive_set_either_option(Archive * a, const char * m, const char * o, const char * v, option_handler use_format_option, option_handler use_filter_option);

#endif
