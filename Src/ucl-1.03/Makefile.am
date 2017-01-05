## Process this file with automake to create Makefile.in
#
# Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
#

AUTOMAKE_OPTIONS = 1.8.5 foreign

SUBDIRS = include src examples

dist-hook:
	-rm -rf "$(distdir)/acconfig/"
	cp -pR $(srcdir)/B/ $(srcdir)/acconfig/ $(srcdir)/asm/ $(srcdir)/upx/ "$(distdir)/"
	$(mkinstalldirs) "$(distdir)/acc"
	cp -pR $(srcdir)/acc/ACC* $(srcdir)/acc/acc* "$(distdir)/acc/"
	-rm -f `find "./$(distdir)/" -type l -print`

@MAINT@-include $(top_srcdir)/Makefile.maint
