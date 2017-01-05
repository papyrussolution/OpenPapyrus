# w32bcc55.mak

## Makefile to build the CCCC project on the Borland C++ Command Line
## Compiler.
## (tested with a free copy which was shipped on a magazine CD, May 2001)
## See rules.mak for discussion of the meaning of the make variables
## which this file defines

# support for debugging (note that debug building is on by default)
!IF "$(DEBUG)"=="true"
# It would be nice to turn the CodeGuard feature on but this 
# appears to depend on a library called CG32.lib which doesn't
# appear to be present in my free command line version.
CFLAGS_DEBUG=-v 
LDFLAGS_DEBUG=-v 
!ENDIF

PATHSEP=\\
CCC=bcc32.exe 
#LD=ilink32.exe -L"$(BCDIR)\lib" C0D32.OBJ CW32.LIB IMPORT32.LIB
LD=bcc32.exe -v  -L"$(BCDIR)\lib"
CFLAGS= \
	$(CFLAGS_DEBUG) -c -P -D_NO_VCL -DCCCC_CONF_W32BC -I$(PCCTS_H) \
	-w-aus -w-par -w-hid -w-inl \
	-I"$(BCDIR)\include" -tWC  
C_OFLAG=-o
LDFLAGS=$(LDFLAGS_DEBUG)  
LD_OFLAG=-o
OBJEXT=obj
CCCC_EXE=cccc.exe

COPY=copy
RM=del

!INCLUDE rules.mak

