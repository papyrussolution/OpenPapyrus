// SOSSL.CPP
// Copyright (c) A.Sobolev 2017, 2020, 2022
//
#include <slib-internal.h>
#pragma hdrstop
#include <slib-ossl.h>

typedef int  (*SSL_LIBRARY_INIT_PROC)();
typedef void (*SSL_LOAD_ERROR_STRINGS_PROC)();

/* linker
..\OSF\OPENSSL\lib\libeay32.lib
..\OSF\OPENSSL\lib\ssleay32.lib
*/

int SlSession::InitSSL()
{
	int    ok = -1;
	if(SslInitCounter == 0) {
		ENTER_CRITICAL_SECTION
#ifdef USE_OPENSSL_STATIC
		{
			if(SSL_library_init()) {
				SSL_load_error_strings();
				SslInitCounter.Incr();
				ok = 1;
			}
			else
				ok = 0;
		}
#else
		{
			SDynLibrary ssl_lib("ssleay32.dll");
			if(ssl_lib.IsValid()) {
				SSL_LIBRARY_INIT_PROC ssl_init_proc = reinterpret_cast<SSL_LIBRARY_INIT_PROC>(ssl_lib.GetProcAddr("SSL_library_init"));
				SSL_LOAD_ERROR_STRINGS_PROC ssl_les_proc = reinterpret_cast<SSL_LOAD_ERROR_STRINGS_PROC>(ssl_lib.GetProcAddr("SSL_load_error_strings"));
				if(ssl_init_proc && ssl_les_proc && ssl_init_proc()) {
					ssl_les_proc();
					SslInitCounter.Incr();
					ok = 1;
				}
				else
					ok = 0;
			}
			else
				ok = 0;
		}
#endif // USE_OPENSSL_STATIC
		LEAVE_CRITICAL_SECTION
	}
	return ok;
}