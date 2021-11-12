# Makefile.am -- automake input for cyrus-sasl tests
# Simo Sorce
#
################################################################
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
################################################################

AM_CPPFLAGS=-I$(top_srcdir)/include -DPLUGINDIR='"${top_srcdir}/plugins/.libs"'

COMMON_LDADD = ../lib/libsasl2.la $(GSSAPIBASE_LIBS) $(GSSAPI_LIBS) $(LIB_SOCKET)

t_gssapi_cli_SOURCES = \
	t_common.c \
	t_gssapi_cli.c

t_gssapi_cli_LDADD = $(COMMON_LDADD)

t_gssapi_srv_SOURCES = \
	t_common.c \
	t_gssapi_srv.c

t_gssapi_srv_LDADD = $(COMMON_LDADD)

check_PROGRAMS = \
	t_gssapi_cli \
	t_gssapi_srv \
	$(NULL)

noinst_PROGRAMS = $(check_PROGRAMS)

EXTRA_DIST = \
	runtests.py \
	$(NULL)

all: $(check_PROGRAMS)

check:
if MACOSX
# skip Mac OSX for now
else
	$(srcdir)/runtests.py $(CHECKARGS)
endif
