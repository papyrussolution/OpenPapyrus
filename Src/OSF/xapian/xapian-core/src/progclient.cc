/** @file
 * @brief implementation of NetClient which spawns a program.
 */
// Copyright 1999,2000,2001 BrightStation PLC
// Copyright 2002 Ananova Ltd
// Copyright 2003,2004,2005,2006,2007,2010,2011,2014,2019 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#ifndef __WIN32__
	#include "safesyssocket.h"
	#include <sys/wait.h>
#endif

using namespace std;

#ifndef __WIN32__
/** Split a string into a vector of strings, using a given separator
 *  character (default space)
 */
static void split_words(const string &text, vector <string> &words, char ws = ' ')
{
	size_t i = 0;
	if(i < text.length() && text[0] == ws) {
		i = text.find_first_not_of(ws, i);
	}
	while(i < text.length()) {
		size_t j = text.find_first_of(ws, i);
		words.push_back(text.substr(i, j - i));
		i = text.find_first_not_of(ws, j);
	}
}

#endif

ProgClient::ProgClient(const string &progname, const string &args, double timeout_, bool writable, int flags) : 
	RemoteDatabase(run_program(progname, args, child), timeout_, get_progcontext(progname, args), writable, flags)
{
	LOGCALL_CTOR(DB, "ProgClient", progname | args | timeout_ | writable | flags);
}

string ProgClient::get_progcontext(const string &progname, const string &args)
{
	LOGCALL_STATIC(DB, string, "ProgClient::get_progcontext", progname | args);
	RETURN("remote:prog(" + progname + " " + args + ")");
}

int ProgClient::run_program(const string & progname, const string &args,
#ifndef __WIN32__
    pid_t& child
#else
    HANDLE& child
#endif
    )
{
	LOGCALL_STATIC(DB, int, "ProgClient::run_program", progname | args | Literal("[&child]"));
#if defined HAVE_SOCKETPAIR && defined HAVE_FORK
	/* socketpair() returns two sockets.  We keep sv[0] and give
	 * sv[1] to the child process.
	 */
	int sv[2];
	if(socketpair(PF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0, sv) < 0) {
		throw Xapian::NetworkError(string("socketpair failed"), get_progcontext(progname, args), errno);
	}
	child = fork();
	if(child < 0) {
		throw Xapian::NetworkError(string("fork failed"), get_progcontext(progname, args), errno);
	}
	if(child != 0) {
		// parent
		// close the child's end of the socket
		::close(sv[1]);
		RETURN(sv[0]);
	}
	/* child process:
	 *   set up file descriptors and exec program
	 */
#if defined F_SETFD && defined FD_CLOEXEC
	// Clear close-on-exec flag, if we set it when we called socketpair().
	// Clearing it here means there's no window where another thread in the
	// parent process could fork()+exec() and end up with this fd still
	// open (assuming close-on-exec is supported).
	//
	// We can't use a preprocessor check on the *value* of SOCK_CLOEXEC as
	// on Linux SOCK_CLOEXEC is an enum, with '#define SOCK_CLOEXEC
	// SOCK_CLOEXEC' to allow '#ifdef SOCK_CLOEXEC' to work.
	if(SOCK_CLOEXEC != 0)
		(void)fcntl(sv[1], F_SETFD, 0);
#endif

	// replace stdin and stdout with the socket
	// FIXME: check return values from dup2.
	if(sv[1] != 0) {
		dup2(sv[1], 0);
	}
	if(sv[1] != 1) {
		dup2(sv[1], 1);
	}
	// close unnecessary file descriptors
	closefrom(2);
	// Redirect stderr to /dev/null
	int stderrfd = open("/dev/null", O_WRONLY);
	if(stderrfd == -1) {
		throw Xapian::NetworkError(string("Redirecting stderr to /dev/null failed"), get_progcontext(progname, args), errno);
	}
	if(stderrfd != 2) {
		// Not sure why it wouldn't be 2, but handle the situation anyway.
		dup2(stderrfd, 2);
		::close(stderrfd);
	}
	vector <string> argvec;
	split_words(args, argvec);
	// We never explicitly free this memory, but that's OK as we're about
	// to either execvp() or _exit().
	const char ** new_argv = new const char *[argvec.size() + 2];
	new_argv[0] = progname.c_str();
	for(vector <string>::size_type i = 0; i < argvec.size(); ++i) {
		new_argv[i + 1] = argvec[i].c_str();
	}
	new_argv[argvec.size() + 1] = 0;
	execvp(progname.c_str(), const_cast<char * const *>(new_argv));
	// if we get here, then execvp failed.
	/* throwing an exception is a bad idea, since we're
	 * not the original process. */
	_exit(-1);
#ifdef __xlC__
	// Avoid "missing return statement" warning.
	return 0;
#endif
#elif defined __WIN32__
	static uint pipecount = 0;
	char pipename[256];
	sprintf(pipename, "\\\\.\\pipe\\xapian-remote-%lx-%lx-%x", static_cast<ulong>(GetCurrentProcessId()), static_cast<ulong>(GetCurrentThreadId()), pipecount++);
	// Create a pipe so we can read stdout from the child process.
	HANDLE hPipe = ::CreateNamedPipeA(pipename, PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED, 0, 1, 4096, 4096, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if(hPipe == INVALID_HANDLE_VALUE) {
		throw Xapian::NetworkError("CreateNamedPipe failed", get_progcontext(progname, args), -int(GetLastError()));
	}
	HANDLE hClient = ::CreateFileA(pipename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if(hClient == INVALID_HANDLE_VALUE) {
		throw Xapian::NetworkError("CreateFile failed", get_progcontext(progname, args), -int(GetLastError()));
	}
	if(!ConnectNamedPipe(hPipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED) {
		throw Xapian::NetworkError("ConnectNamedPipe failed", get_progcontext(progname, args), -int(GetLastError()));
	}
	// Set the appropriate handles to be inherited by the child process.
	SetHandleInformation(hClient, HANDLE_FLAG_INHERIT, 1);
	// Create the child process.
	PROCESS_INFORMATION procinfo;
	memzero(&procinfo, sizeof(PROCESS_INFORMATION));
	STARTUPINFOA startupinfo;
	memzero(&startupinfo, sizeof(STARTUPINFO));
	startupinfo.cb = sizeof(STARTUPINFO);
	startupinfo.hStdError = hClient;
	startupinfo.hStdOutput = hClient;
	startupinfo.hStdInput = hClient;
	startupinfo.dwFlags |= STARTF_USESTDHANDLES;

	// For some reason Windows wants a modifiable command line!
	string cmdline = progname + ' ' + args;
	BOOL ok = ::CreateProcessA(0, &cmdline[0], 0, 0, TRUE, 0, 0, 0, &startupinfo, &procinfo);
	if(!ok) {
		throw Xapian::NetworkError("CreateProcess failed", get_progcontext(progname, args), -int(GetLastError()));
	}
	CloseHandle(hClient);
	CloseHandle(procinfo.hThread);
	child = procinfo.hProcess;
	RETURN(_open_osfhandle(intptr_t(hPipe), O_RDWR|O_BINARY));
#endif
}

ProgClient::~ProgClient()
{
	try {
		// Close the socket and reap the child.
		do_close();
	} catch(...) {
	}
#ifndef __WIN32__
	waitpid(child, 0, 0);
#else
	WaitForSingleObject(child, INFINITE);
#endif
}

#ifdef DISABLE_GPL_LIBXAPIAN
#error GPL source we cannot relicense included in libxapian
#endif
