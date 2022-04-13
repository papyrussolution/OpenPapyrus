/** @file
 * @brief Utility functions for replication implementations
 */
/* Copyright (C) 2010 Richard Boulton
 * Copyright (C) 2010 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop
//#include "replicate_utils.h"
/** Create a new changeset file, and return an open fd for writing to it.
 *
 *  Creates the changeset directory, if required.
 *
 *  If there is already a changeset file of the given name, it is truncated by
 *  this.
 *
 *  @param changeset_dir The directory for the changesets.
 *  @param filename The name of the changeset file.
 *  @param changes_name A string which will be set to the path of the changeset
 *  file.
 *
 *  @return The open file descriptor.
 *
 *  @exception Xapian::DatabaseError if the changeset couldn't be opened.
 */
int create_changeset_file(const std::string & changeset_dir, const std::string & filename, std::string & changes_name);
/** Write some changes from a buffer, and then drop them from the buffer.
 *
 *  @param changes_fd The file descriptor to write to (-1 to skip writing).
 *  @param buf The buffer holding the changes.
 *  @param bytes The number of bytes to write and drop.
 */
void write_and_clear_changes(int changes_fd, std::string & buf, size_t bytes);
//
using namespace std;

int create_changeset_file(const string & changeset_dir, const string & filename, string & changes_name)
{
	changes_name = changeset_dir;
	changes_name += '/';
	changes_name += filename;
	int changes_fd = posixy_open(changes_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
	if(changes_fd < 0) {
		string message("Couldn't open changeset to write: ");
		message += changes_name;
		throw Xapian::DatabaseError(message, errno);
	}
	return changes_fd;
}

void write_and_clear_changes(int changes_fd, string & buf, size_t bytes)
{
	if(changes_fd != -1) {
		io_write(changes_fd, buf.data(), bytes);
	}
	buf.erase(0, bytes);
}
