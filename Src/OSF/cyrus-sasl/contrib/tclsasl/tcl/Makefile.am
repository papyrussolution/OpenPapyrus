# Makefile.am for the Tcl SASL library
# 
# $Id: Makefile.am,v 1.1 2002/02/05 23:28:26 rjs3 Exp $
# Copyright (c) 2000 Carnegie Mellon University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The name "Carnegie Mellon University" must not be used to
#    endorse or promote products derived from this software without
#    prior written permission. For permission or any other legal
#    details, please contact  
#      Office of Technology Transfer
#      Carnegie Mellon University
#      5000 Forbes Avenue
#      Pittsburgh, PA  15213-3890
#      (412) 268-4387, fax: (412) 268-7395
#      tech-transfer@andrew.cmu.edu
#
# 4. Redistributions of any form whatsoever must retain the following
#    acknowledgment:
#    "This product includes software developed by Computing Services
#     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
#
# CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
# AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# Library version info - here at the top, for sanity
tclsasl_version = 2:0:0

INCLUDES=-I$(top_srcdir)/include

common_sources = tclsasl.c

lib_LTLIBRARIES = libtclsasl.la

libtclsasl_la_SOURCES = $(common_sources)
libtclsasl_la_LDFLAGS = -version-info $(tclsasl_version)
libtclsasl_la_DEPENDENCIES = @LTLIBOBJS@
libtclsasl_la_LIBADD = $(LIB_TCL) ../lib/libsasl2.la $(LIB_SOCKET)

install-exec-hook: @INSTALL_TCLSASL@

install-tclsasl:
	$(mkinstalldirs) @PKG_TCL@
	echo	"package ifneeded sasl"					   \
		`echo $(VERSION) | awk -F. '{ printf "%s.%s", $$1, $$2 }'` \
		"[list load $(DESTDIR)$(libdir)/libtclsasl.so]"		   \
	    > @PKG_TCL@/pkgIndex.tcl
	sed -e 's%load .libs/libtclsasl.so%package require sasl%' \
	    < test.tcl      					     \
	    > @PKG_TCL@/test.tcl
