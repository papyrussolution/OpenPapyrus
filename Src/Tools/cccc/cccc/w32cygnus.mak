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
CCC=cl.exe -nologo
LD=cl.exe -nologo
CFLAGS=-c -I$(PCCTS_H) $(CFLAGS_DEBUG) -GX -TP -DCCCC_CONF_W32VC
C_OFLAG=-Fo
LDFLAGS=$(LDFLAGS_DEBUG) 
LD_OFLAG=-Fe
OBJEXT=obj
CCCC_EXE=cccc.exe

COPY=copy
RM=del


!INCLUDE rules.mak

