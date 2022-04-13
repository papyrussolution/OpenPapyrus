/** @file
 *  @brief TCP/IP replication client class.
 */
/* Copyright (C) 2008,2010,2011,2015 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_REPLICATETCPCLIENT_H
#define XAPIAN_INCLUDED_REPLICATETCPCLIENT_H

/// TCP/IP replication client class.
#ifdef __WIN32__
class XAPIAN_VISIBILITY_DEFAULT ReplicateTcpClient : private WinsockInitializer {
#else
class XAPIAN_VISIBILITY_DEFAULT ReplicateTcpClient {
#endif
	void operator =(const ReplicateTcpClient &); /// Don't allow assignment.
	ReplicateTcpClient(const ReplicateTcpClient &); /// Don't allow copying.
	int socket; /// The socket fd.
	OwnedRemoteConnection remconn; /// Write-only connection to the server.
	/** Attempt to open a TCP/IP socket connection to a replication server.
	 *
	 *  Connect to replication server running on port @a port of host @a hostname.
	 *  Give up trying to connect after @a timeout_connect seconds.
	 *
	 *  Note: this method is called early on during class construction before
	 *  any member variables or even the base class have been initialised.
	 *  To help avoid accidentally trying to use member variables or call other
	 *  methods which do, this method has been deliberately made "static".
	 */
	XAPIAN_VISIBILITY_INTERNAL static int open_socket(const std::string & hostname, int port, double timeout_connect);
public:
	/** Constructor.
	 *
	 *  Connect to replication server running on port @a port of host @a hostname.
	 *  Give up trying to connect after @a timeout_connect seconds.
	 *
	 *  @param timeout_connect	 Timeout for trying to connect (in seconds).
	 *  @param socket_timeout	 Socket timeout (in seconds); 0 for no timeout.
	 */
	ReplicateTcpClient(const std::string & hostname, int port, double timeout_connect, double socket_timeout);
	void update_from_master(const std::string & path, const std::string & remotedb, Xapian::ReplicationInfo & info, double reader_close_time, bool force_copy);
	~ReplicateTcpClient();
};

#endif // XAPIAN_INCLUDED_REPLICATETCPCLIENT_H
