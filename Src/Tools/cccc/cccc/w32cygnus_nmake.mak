# w32cygnus_nmake.mak

## NMakefile to build the CCCC project on the Win32/CygWinB20 platform.
## See rules.mak for discussion of the meaning of the make variables
## which this file defines

# support for debugging (note that debug building is on by default)
!IF "$(DEBUG)"=="true"
CFLAGS_DEBUG=-g
LDFLAGS_DEBUG=-g
!ENDIF

PATHSEP=\\
PATH=\cygwin\bin
CCC=$(PATH)\gcc.exe
LD=$(PATH)\gcc.exe
CFLAGS=-c -I$(PCCTS_H) $(CFLAGS_DEBUG) -x c++ -DCCCC_CONF_W32VC
C_OFLAG=-o
LDFLAGS=$(LDFLAGS_DEBUG) 
LD_OFLAG=-o
OBJEXT=o
CCCC_EXE=cccc.exe

COPY=copy
RM=del


!INCLUDE rules.mak

