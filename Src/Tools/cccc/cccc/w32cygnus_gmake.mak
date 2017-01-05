# w32cygnus_gmake.mak
## GNUMakefile to build the CCCC project on the Win32/CygWinB20 platform.
## See rules.mak for discussion of the meaning of the make variables
## which this file defines
# support for debugging 

#if "$(DEBUG)"=="true"
#CFLAGS_DEBUG=-g
#LDFLAGS_DEBUG=-g
#endif

PATHSEP=/
CYGWIN=/cygnus/cygwin-b20/H-i586-cygwin32
EGCS=$(CYGWIN)/lib/gcc-lib/i586-cygwin32/egcs-2.91.57
PATH=$(CYGWIN)/bin;$(EGCS)
export PATH

CCC=/usr/bin/g++
LD=/usr/bin/g++
CFLAGS=-c -I../pccts/h -I$(CYGWIN)/../include/g++ -I$(CYGWIN)/i586-cygwin32/include -I$(EGCS)/include  $(CFLAGS_DEBUG) -x c++ -DCCCC_CONF_W32VC
C_OFLAG=-o
LDFLAGS=-L$(EGCS)/../lib $(LDFLAGS_DEBUG) 
LD_OFLAG=-o
OBJEXT=o
CCCC_EXE=cccc.exe

COPY=copy
RM=del


include rules.mak

