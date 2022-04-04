/** @file
 * @brief Database factories for remote databases.
 */
/* Copyright (C) 2006,2007,2008,2010,2011,2014 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
	Database Remote::open(const string &host, uint port, uint timeout_, uint connect_timeout)
	{
		LOGCALL_STATIC(API, Database, "Remote::open", host | port | timeout_ | connect_timeout);
		RETURN(Database(new RemoteTcpClient(host, port, timeout_ * 1e-3, connect_timeout * 1e-3, false, 0)));
	}
	WritableDatabase Remote::open_writable(const string &host, uint port, uint timeout_, uint connect_timeout, int flags)
	{
		LOGCALL_STATIC(API, WritableDatabase, "Remote::open_writable", host | port | timeout_ | connect_timeout | flags);
		RETURN(WritableDatabase(new RemoteTcpClient(host, port, timeout_ * 1e-3, connect_timeout * 1e-3, true, flags)));
	}
	Database Remote::open(const string &program, const string &args, uint timeout_)
	{
		LOGCALL_STATIC(API, Database, "Remote::open", program | args | timeout_);
		RETURN(Database(new ProgClient(program, args, timeout_ * 1e-3, false, 0)));
	}
	WritableDatabase Remote::open_writable(const string &program, const string &args, uint timeout_, int flags)
	{
		LOGCALL_STATIC(API, WritableDatabase, "Remote::open_writable", program | args | timeout_ | flags);
		RETURN(WritableDatabase(new ProgClient(program, args, timeout_ * 1e-3, true, flags)));
	}
}
