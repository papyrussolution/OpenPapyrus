set FNAME_LEX=lex.cpp
set FNAME_YCC=ppalddc.cpp
set YACC=..\..\..\tools\bison -d -t -v
set LEX=..\..\..\tools\flex

@echo on
%YACC% ppalddc.y
@pause
%LEX% ppalddc.l
@pause

del %FNAME_YCC%
rename ppalddc.tab.c %FNAME_YCC%
del %FNAME_LEX%
rename lex.yy.c %FNAME_LEX%

call vcvars32.bat
set  include=%include%;..\include
set  lib=d:\papyrus\src\objwin\release;%lib%
set  cc="d:\msvs\vc98\bin\cl.exe"
%CC% /Zp1 %FNAME_YCC% %FNAME_LEX% libfl.lib slib.lib /oppalddc.exe > err.
@del *.obj
@del ppalddc.tab.h
:@del %FNAME_YCC%
:@del ppalddc.output
:@del %FNAME_LEX%
