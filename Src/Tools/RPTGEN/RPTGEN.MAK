.AUTODEPEND

.PATH.obj = ..\OBJ

#		*Translator Definitions*
CC = bcc +RPTGEN.CFG
TASM = TASM
TLIB = tlib
TLINK = tlink
LIBPATH = \BC\LIB
INCLUDEPATH = \BC\INCLUDE;..\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 ppaldd.obj \
 report.obj \
 sprinter.obj \
 rptcreat.obj \
 rptload.obj \
 rptwrite.obj \
 rptlex.obj \
 rpty.obj \
 rpty_tab.obj \
 round.obj \
 ..\lib\dec.obj \
 {$(LIBPATH)}lexlib.lib \
 ..\lib\ppslapi.lib \
 ..\lib\pptvo.lib \
 ..\lib\pptv.lib \
 ..\lib\ppdb.lib

#		*Explicit Rules*
..\obj\rptgen.exe: rptgen.cfg $(EXE_dependencies)
  $(TLINK) /v/x/c/P-/L$(LIBPATH)/o @&&|
/o- c0l.obj+
/o+ ..\obj\ppaldd.obj+
..\obj\report.obj+
..\obj\sprinter.obj+
..\obj\rptcreat.obj+
..\obj\rptload.obj+
..\obj\rptwrite.obj+
..\obj\rptlex.obj+
..\obj\rpty.obj+
..\obj\rpty_tab.obj+
/o- ..\obj\round.obj+
..\lib\dec.obj
..\obj\rptgen
		# no map file
/o+ ..\obj\lexlib.lib+
/o- ..\lib\ppslapi.lib+
..\lib\pptvo.lib+
..\lib\pptv.lib+
..\lib\ppdb.lib+
overlay.lib+
emu.lib+
mathl.lib+
cl.lib
|


#		*Individual File Dependencies*
ppaldd.obj: rptgen.cfg ..\pplib\ppaldd.cpp 
	$(CC) -c ..\pplib\ppaldd.cpp

report.obj: rptgen.cfg ..\slib\report.cpp 
	$(CC) -c ..\slib\report.cpp

sprinter.obj: rptgen.cfg ..\slib\sprinter.cpp 
	$(CC) -c ..\slib\sprinter.cpp

rptcreat.obj: rptgen.cfg rptcreat.cpp 

rptload.obj: rptgen.cfg ..\slib\rptload.cpp 
	$(CC) -c ..\slib\rptload.cpp

rptwrite.obj: rptgen.cfg rptwrite.cpp 

rptlex.obj: rptgen.cfg rptlex.cpp 

rpty.obj: rptgen.cfg rpty.cpp 

rpty_tab.obj: rptgen.cfg rpty_tab.cpp 

round.obj: rptgen.cfg ..\slib\round.cpp 
	$(CC) -c ..\slib\round.cpp

#		*Compiler Configuration File*
rptgen.cfg: rptgen.mak
  copy &&|
-ml
-3
-v
-G
-O
-Og
-Oe
-Om
-Ov
-Ol
-Ob
-Op
-Z
-k-
-Y
-vi-
-w-par
-wpro
-wdef
-weas
-wpre
-n..\OBJ
-I$(INCLUDEPATH)
-L$(LIBPATH)
-D_REPORT_
| rptgen.cfg


