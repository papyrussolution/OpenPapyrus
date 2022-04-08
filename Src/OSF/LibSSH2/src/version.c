/* Copyright (C) 2009 Daniel Stenberg.  All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the
 * following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names
 * of any other contributors may be used to endorse or
 * promote products derived from this software without
 * specific prior written permission.
 */
#include "libssh2_priv.h"
#pragma hdrstop
/*
  libssh2_version() can be used like this:

  if(!libssh2_version(LIBSSH2_VERSION_NUM)) {
    fprintf (stderr, "Runtime libssh2 version too old!\n");
    exit(1);
  }
*/
LIBSSH2_API const char * libssh2_version(int req_version_num)
{
    return (req_version_num <= LIBSSH2_VERSION_NUM) ? LIBSSH2_VERSION : NULL/* this is not a suitable library! */;
}
