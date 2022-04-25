/*************************************************
*       Perl-Compatible Regular Expressions      *
*************************************************/

/*
Copyright (c) 2005, Google Inc.
All rights reserved.

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
*/
#ifndef PCRECPP_INTERNAL_H
#define PCRECPP_INTERNAL_H

/* When compiling a DLL for Windows, the exported symbols have to be declared
using some MS magic. I found some useful information on this web page:
http://msdn2.microsoft.com/en-us/library/y4h7bcy6(VS.80).aspx. According to the
information there, using __declspec(dllexport) without "extern" we have a
definition; with "extern" we have a declaration. The settings here override the
setting in pcre.h. We use:

  PCRECPP_EXP_DECL       for declarations
  PCRECPP_EXP_DEFN       for definitions of exported functions

*/

#ifndef PCRECPP_EXP_DECL
#ifdef _WIN32
#ifndef PCRE_STATIC
#define PCRECPP_EXP_DECL       extern __declspec(dllexport)
#define PCRECPP_EXP_DEFN       __declspec(dllexport)
#else
#define PCRECPP_EXP_DECL       extern
#define PCRECPP_EXP_DEFN
#endif
#else
#define PCRECPP_EXP_DECL         extern
#define PCRECPP_EXP_DEFN
#endif
#endif

#endif  /* PCRECPP_INTERNAL_H */

/* End of pcrecpp_internal.h */
