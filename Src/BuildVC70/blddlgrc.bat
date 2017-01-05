rem 
rem @v6.6.9 Перенесено непосредственно в проект
rem
@echo off

set TOOLS=..\..\tools
set SRCPATH=..\rsrc\rc

rem v7.2.3 {
DEL /Q ..\..\__TEMP__\S\*.scp
rem } v7.2.3 

IF EXIST ppdlg.rc DEL /Q ppdlg.rc
FOR %%I IN (..\..\__TEMP__\S\*.rc) DO IF EXIST ppdlg.rc (COPY /B /Y ppdlg.rc+%%I ppdlg.rc) ELSE COPY /B %%I ppdlg.rc
MOVE /Y ppdlg.rc %SRCPATH%\ppdlg.rc

rem CALL RCWCVT.BAT

%TOOLS%\perl\perl %TOOLS%\rc_conv.pl < %SRCPATH%\ppdlg.rc > ppdlgw.866
%TOOLS%\rcd ppdlgw.866 %SRCPATH%\ppdlgw.rc /A /W
del ppdlgw.866
