/** @file
 * @brief TCP/IP replication server class.
 */
// Copyright (C) 2008,2010,2011 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#include "replicatetcpserver.h"

using namespace std;

ReplicateTcpServer::ReplicateTcpServer(const string & host, int port, const string & path_) : XapianTcpServer(host, port, false, false), path(path_)
{
}

ReplicateTcpServer::~ReplicateTcpServer() 
{
}

void ReplicateTcpServer::handle_one_connection(int socket)
{
	RemoteConnection client(socket, -1);
	try {
		// Read start_revision from the client.
		string start_revision;
		if(client.get_message(start_revision, 0.0) != 'R') {
			throw Xapian::NetworkError("Bad replication client message");
		}
		// Read dbname from the client.
		string dbname;
		if(client.get_message(dbname, 0.0) != 'D') {
			throw Xapian::NetworkError("Bad replication client message (2)");
		}
		if(dbname.find("..") != string::npos) {
			throw Xapian::NetworkError("dbname contained '..'");
		}
		string dbpath(path);
		dbpath += '/';
		dbpath += dbname;
		Xapian::DatabaseMaster master(dbpath);
		master.write_changesets_to_fd(socket, start_revision, NULL);
	} catch(...) {
		// Ignore exceptions.
	}
}
