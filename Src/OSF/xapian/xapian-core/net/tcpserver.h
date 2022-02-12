// TCPSERVER.H
// Copyright (C) 2007,2008 Olly Betts
// @licence GNU GPL
// @brief Generic TCP/IP socket based server base class.
//
#ifndef XAPIAN_INCLUDED_TCPSERVER_H
#define XAPIAN_INCLUDED_TCPSERVER_H

#ifdef __WIN32__
	#include "remoteconnection.h"
	#define SOCKET_INITIALIZER_MIXIN : private WinsockInitializer
#else
	#define SOCKET_INITIALIZER_MIXIN
#endif
#if defined __CYGWIN__ || defined __WIN32__
	#include "safewindows.h" // Only for HANDLE!
#endif
#include <xapian/visibility.h>
//#include <string>

/** TCP/IP socket based server for RemoteDatabase.
 *
 *  This class implements the server used by xapian-tcpsrv.
 */
class XAPIAN_VISIBILITY_DEFAULT XapianTcpServer SOCKET_INITIALIZER_MIXIN {
	/// Don't allow assignment.
	void operator=(const XapianTcpServer &);
	/// Don't allow copying.
	XapianTcpServer(const XapianTcpServer &);
#if defined __CYGWIN__ || defined __WIN32__
	HANDLE mutex = NULL; /// Mutex to stop two TcpServers running on the same port.
#endif
	int listen_socket; /** The socket we're listening on. */
	/** Create a listening socket ready to accept connections.
	 *
	 *  @param host	hostname or address to listen on or an empty string to
	 *			accept connections on any interface.
	 *  @param port	TCP port to listen on.
	 *  @param tcp_nodelay	If true, enable TCP_NODELAY option.
	 */
	XAPIAN_VISIBILITY_INTERNAL static int get_listening_socket(const std::string & host, int port, bool tcp_nodelay
#if defined __CYGWIN__ || defined __WIN32__
	    , HANDLE &mutex
#endif
	    );

protected:
	bool verbose; /** Should we produce output when connections are made or lost? */
	/** Accept a connection and return the file descriptor for it. */
	XAPIAN_VISIBILITY_INTERNAL int accept_connection();
public:
	/** Construct a XapianTcpServer and start listening for connections.
	 *
	 *  @param host	The hostname or address for the interface to listen on
	 *			(or "" to listen on all interfaces).
	 *  @param port	The TCP port number to listen on.
	 *  @param tcp_nodelay	If true, enable TCP_NODELAY option.
	 *	@param verbose	Should we produce output when connections are
	 *			made or lost?
	 */
	XapianTcpServer(const std::string &host, int port, bool tcp_nodelay, bool verbose);
	/** Destructor. */
	virtual ~XapianTcpServer();
	/** Accept connections and service requests indefinitely.
	 *
	 *  This method runs the XapianTcpServer as a daemon which accepts a connection
	 *  and forks itself (or creates a new thread under Windows) to serve the
	 *  request while continuing to listen for more connections.
	 */
	void run();
	/** Accept a single connection, service requests on it, then stop.  */
	void run_once();
	/// Handle a single connection on an already connected socket.
	virtual void handle_one_connection(int socket) = 0;
};

#endif  // XAPIAN_INCLUDED_TCPSERVER_H
