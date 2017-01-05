# w32vc.mak

## Makefile to build the CCCC project on the Win32/Visual Studio platform.
## See rules.mak for discussion of the meaning of the make variables
## which this file defines

DEBUG=true
# support for debugging (note that debug building is on by default)
!IF "$(DEBUG)"=="true"
CFLAGS_DEBUG=-Zi
LDFLAGS_DEBUG=-Zi
!ENDIF

PATHSEP=\\
INC="C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\include"
CCC=cl.exe -nologo /I$(INC) 
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

