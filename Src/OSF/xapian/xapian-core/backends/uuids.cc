/** @file
 * @brief Class for handling UUIDs
 */
/* Copyright (C) 2008 Lemur Consulting Ltd
 * Copyright (C) 2013,2015,2016,2017,2018 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

#ifdef USE_PROC_FOR_UUID
//#include "safesysstat.h"
#elif defined HAVE_UUID_UUID_H
#include <exception>
#include <uuid/uuid.h>
#elif defined HAVE_UUID_H
// UUID API on FreeBSD, NetBSD, OpenBSD and AIX.
#include <arpa/inet.h> // For htonl() and htons().
#include <exception>
#include <uuid.h>
#elif defined USE_WIN32_UUID_API
#include "safewindows.h"
#include <rpc.h>
#ifdef __WIN32__
#include "safewinsock2.h" // For htonl() and htons().
#else
// Cygwin:
#include <arpa/inet.h> // For htonl() and htons().
#endif
#endif

using namespace std;

/// Bit-mask to determine where to put hyphens in the string representation.
static constexpr uint UUID_GAP_MASK = 0x2a8;

void Uuid::generate()
{
#ifdef USE_PROC_FOR_UUID
	char buf[STRING_SIZE];
	int fd = open("/proc/sys/kernel/random/uuid", O_RDONLY);
	if(UNLIKELY(fd == -1)) {
		throw Xapian::DatabaseCreateError("Opening UUID generator failed", errno);
	}
	bool failed = (read(fd, buf, STRING_SIZE) != STRING_SIZE);
	close(fd);
	if(failed) {
		throw Xapian::DatabaseCreateError("Generating UUID failed");
	}
	parse(buf);
#elif defined HAVE_UUID_UUID_H
	uuid_t uu;
	uuid_generate(uu);
	memcpy(uuid_data, &uu, BINARY_SIZE);
#elif defined HAVE_UUID_H
	uuid_t uu;
	uint32_t status;
	uuid_create(&uu, &status);
	if(status != uuid_s_ok) {
		// Can only be uuid_s_no_memory it seems.
		throw std::bad_alloc();
	}
	uu.time_low = htonl(uu.time_low);
	uu.time_mid = htons(uu.time_mid);
	uu.time_hi_and_version = htons(uu.time_hi_and_version);
	memcpy(uuid_data, &uu, BINARY_SIZE);
#elif defined USE_WIN32_UUID_API
	UUID uuid;
	if(UNLIKELY(UuidCreate(&uuid) != RPC_S_OK)) {
		// Throw a DatabaseCreateError, since we can't make a UUID.  The
		// windows API documentation is a bit unclear about the situations in
		// which this can happen.
		throw Xapian::DatabaseCreateError("Cannot create UUID");
	}
	uuid.Data1 = htonl(uuid.Data1);
	uuid.Data2 = htons(uuid.Data2);
	uuid.Data3 = htons(uuid.Data3);
	memcpy(uuid_data, &uuid, BINARY_SIZE);
#else
#error Do not know how to generate UUIDs
#endif
}

void Uuid::parse(const char * in)
{
	for(uint i = 0; i != BINARY_SIZE; ++i) {
		uuid_data[i] = hex_digit(in[0]) << 4 | hex_digit(in[1]);
		in += ((UUID_GAP_MASK >> i) & 1) | 2;
	}
}

string Uuid::to_string() const
{
	string result;
	result.reserve(STRING_SIZE);
	for(uint i = 0; i != BINARY_SIZE; ++i) {
		uchar ch = uuid_data[i];
		result += SlConst::P_HxDigL[ch >> 4];
		result += SlConst::P_HxDigL[ch & 0x0f];
		if((UUID_GAP_MASK >> i) & 1)
			result += '-';
	}
	return result;
}
