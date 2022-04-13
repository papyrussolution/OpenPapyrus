/** @file
 * @brief Definitions, types, etc for use inside glass.
 */
/* Copyright (C) 2010,2014,2015,2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_GLASS_DEFS_H
#define XAPIAN_INCLUDED_GLASS_DEFS_H

#include "internaltypes.h"

#define GLASS_TABLE_EXTENSION "glass" /// Glass table extension.
#define GLASS_DEFAULT_BLOCKSIZE 8192 /// Default B-tree block size.
#define GLASS_MIN_BLOCKSIZE 2048 /// Minimum B-tree block size.
#define GLASS_MAX_BLOCKSIZE 65536 /// Maximum B-tree block size.

/** The largest docid value supported by glass.
 *
 *  The disk format supports 64-bit docids, but if Xapian::docid is narrower
 *  then it's the largest value supported by the type that matters here.
 */
#define GLASS_MAX_DOCID Xapian::docid(0xffffffffffffffff)

namespace Glass {
    enum table_type {
	POSTLIST,
	DOCDATA,
	TERMLIST,
	POSITION,
	SPELLING,
	SYNONYM,
	MAX_
    };
}

typedef uint4 glass_block_t; /// A block number in a glass Btree file.
typedef uint4 glass_revision_number_t; /// The revision number of a glass database.
typedef unsigned long long glass_tablesize_t; /// How many entries there are in a table.

#endif // XAPIAN_INCLUDED_GLASS_DEFS_H
