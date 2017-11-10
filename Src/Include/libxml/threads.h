/**
 * Summary: interfaces for thread handling
 * Description: set of generic threading related routines
 *              should work with pthreads, Windows native or TLS threads
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#ifndef __XML_THREADS_H__
#define __XML_THREADS_H__

//#include <libxml/xmlversion.h>

#ifdef __cplusplus
extern "C" {
#endif
// 
// xmlMutex are a simple mutual exception locks.
// 
struct xmlMutex;
//typedef xmlMutex *xmlMutexPtr;
// 
// xmlRMutex are reentrant mutual exception locks.
// 
struct xmlRMutex;
//typedef xmlRMutex * xmlRMutexPtr;

#ifdef __cplusplus
}
#endif
//#include <libxml/globals.h>
#ifdef __cplusplus
extern "C" {
#endif
XMLPUBFUN xmlMutex * XMLCALL xmlNewMutex();
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlMutexLock(xmlMutex * tok);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlMutexUnlock(xmlMutex * tok);
XMLPUBFUN void XMLCALL xmlFreeMutex(xmlMutex * tok);
XMLPUBFUN xmlRMutex * XMLCALL xmlNewRMutex();
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlRMutexLock(xmlRMutex * tok);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlRMutexUnlock(xmlRMutex * tok);
XMLPUBFUN void XMLCALL xmlFreeRMutex(xmlRMutex * tok);
// 
// Library wide APIs.
// 
XMLPUBFUN void XMLCALL xmlInitThreads();
XMLPUBFUN void XMLCALL xmlLockLibrary();
XMLPUBFUN void XMLCALL xmlUnlockLibrary();
XMLPUBFUN int XMLCALL xmlGetThreadId();
XMLPUBFUN int XMLCALL xmlIsMainThread();
XMLPUBFUN void XMLCALL xmlCleanupThreads();
XMLPUBFUN xmlGlobalStatePtr XMLCALL xmlGetGlobalState();
#if defined(HAVE_WIN32_THREADS) && !defined(HAVE_COMPILER_TLS) && defined(LIBXML_STATIC_FOR_DLL)
	int XMLCALL xmlDllMain(void *hinstDLL, unsigned long fdwReason, void *lpvReserved);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __XML_THREADS_H__ */
