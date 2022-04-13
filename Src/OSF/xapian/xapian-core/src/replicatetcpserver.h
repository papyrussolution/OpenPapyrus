/** @file
 * @brief TCP/IP replication server class.
 */
/* Copyright (C) 2008,2011 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_REPLICATETCPSERVER_H
#define XAPIAN_INCLUDED_REPLICATETCPSERVER_H

class XAPIAN_VISIBILITY_DEFAULT ReplicateTcpServer : public XapianTcpServer {
	std::string path; /// The path to pass to DatabaseMaster.
public:
	/** Construct a ReplicateTcpServer and start listening for connections.
	 *
	 *  @param host	The hostname or address for the interface to listen on
	 *			(or "" to listen on all interfaces).
	 *  @param port	The TCP port number to listen on.
	 *  @param path_	The path to the parent directory of the databases.
	 */
	ReplicateTcpServer(const std::string & host, int port, const std::string & path_);
	~ReplicateTcpServer();
	/** Handle a single connection on an already connected socket.
	 *
	 *  This method may be called by multiple threads.
	 */
	void handle_one_connection(int socket);
};

#endif // XAPIAN_INCLUDED_REPLICATETCPSERVER_H
