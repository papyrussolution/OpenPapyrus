@echo on
..\..\tools\bison -d -t -v dl200.y
@pause
..\..\tools\flex dl200.l
@pause

del dl200.tab.cpp
rename dl200.tab.c dl200.tab.cpp
del lex.yy.cpp
rename lex.yy.c lex.yy.cpp

call vcvars32.bat
set  include=%include%;..\include\win32;..\include
set  lib=d:\papyrus\src\objwin\release;%lib%
set  cc="d:\msvs\vc98\bin\cl.exe"
%CC% /D "LOCAL_PPERRCODE" /Zp1 dl200.tab.cpp lex.yy.cpp dl200.cpp dl200ent.cpp dl200_op.cpp slib.lib ..\lib\libfl.lib /odl200c.exe | more
@del *.obj

:@del dl200.tab.h
:@del dl200.tab.cpp
:@del dl200.output
:@del lex.yy.cpp
