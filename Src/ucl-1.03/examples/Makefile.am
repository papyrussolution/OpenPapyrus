## Process this file with automake to create Makefile.in
#
# Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
#

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)

noinst_PROGRAMS = simple uclpack

simple_SOURCES = simple.c
uclpack_SOURCES = uclpack.c

LDADD = ../src/libucl.la
DEPENDENCIES = ../src/libucl.la

EXTRA_DIST = portab.h portab_a.h
