/** @file
 * @brief Internal definitions for glass database replication
 */
/* Copyright 2008 Lemur Consulting Ltd
 * Copyright 2009,2014 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_GLASS_REPLICATE_INTERNAL_H
#define XAPIAN_INCLUDED_GLASS_REPLICATE_INTERNAL_H

#define CHANGES_MAGIC_STRING "GlassChanges" // Magic string used to recognise a changeset file.

// The current version of changeset files.
// 1  - initial implementation
// 2  - compressed changesets
// 3  - store (block_size / GLASS_MIN_BLOCKSIZE)
// 4  - reworked for switch from base files to version file
#define CHANGES_VERSION 4u

// Must be big enough to ensure that the start of the changeset (up to the new
// revision number) will fit in this much space.
#define REASONABLE_CHANGESET_SIZE 1024

#endif /* XAPIAN_INCLUDED_GLASS_REPLICATE_INTERNAL_H */
