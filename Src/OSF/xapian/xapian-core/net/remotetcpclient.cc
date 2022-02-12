/** @file
 *  @brief TCP/IP socket based RemoteDatabase implementation
 */
// Copyright (C) 2008,2010 Olly Betts
// @licence GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#include "remotetcpclient.h"
#include "tcpclient.h"

using namespace std;

int RemoteTcpClient::open_socket(const string & hostname, int port,
    double timeout_connect)
{
	// If TcpClient::open_socket() throws, fill in the context.
	try {
		return TcpClient::open_socket(hostname, port, timeout_connect, true);
	} catch(const Xapian::NetworkTimeoutError & e) {
		throw Xapian::NetworkTimeoutError(e.get_msg(), get_tcpcontext(hostname, port),
			  e.get_error_string());
	} catch(const Xapian::NetworkError & e) {
		throw Xapian::NetworkError(e.get_msg(), get_tcpcontext(hostname, port),
			  e.get_error_string());
	}
}

string RemoteTcpClient::get_tcpcontext(const string & hostname, int port)
{
	string result("remote:tcp(");
	result += hostname;
	result += ':';
	result += str(port);
	result += ')';
	return result;
}

RemoteTcpClient::~RemoteTcpClient()
{
	try {
		do_close();
	} catch(...) {
	}
}
