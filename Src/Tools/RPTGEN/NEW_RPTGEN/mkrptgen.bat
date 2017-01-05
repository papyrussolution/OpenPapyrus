@echo on
..\..\tools\bison -d -t -v rptyy.y
@pause
..\..\tools\flex rptlex.l
@pause

del _rpty.h
rename rptyy.tab.h _rpty.h
del rpty.cpp
rename rptyy.tab.c rpty.cpp
del rptlex.cpp
rename lex.yy.c rptlex.cpp

rem call vcvars32.bat
rem set  include=%include%;..\include\win32;..\include
rem set  lib=d:\papyrus\src\objwin\release;%lib%
rem set  cc="d:\msvs\vc98\bin\cl.exe"
rem %CC% /D "LOCAL_PPERRCODE" /Zp1 rpty.cpp rptlex.cpp Rptcreat.cpp rptwrite.cpp ..\lib\libfl.lib /orptgen.exe | more
rem @del *.obj

