rem 
rem ѕервый параметр - корневой каталог PAPYRUS. например: C:\PAPYRUS
rem ¬торой параметр - обозначение версии без точек. например: 4906(512)
rem “ретий параметр - обозначение версии с точками. например: 4.9.06(512)
rem „етвертый		- дополнительный тип сборки (demo | manual | +demo | test)
rem
rem

rem @echo off

set COPYCMD=/Y
set PPYROOT=%1%
set VER=%2%
set VERDOT=%3%
set ASSMTYPE=%4%
set PPYSRC=%PPYROOT%\src
set PPYREDIST=%PPYSRC%\redist

set MKDIST_STATUS=mkdist.ok
del %PPYSRC%\build\log\mkdist.ok
echo status > %PPYSRC%\build\log\mkdist.failed

rem
rem  орневой каталог, в котором создаютс€ подкаталоги 
rem дл€ запускаемых модулей и файлов обеспечени€ //
rem
set PPYTARGET=%PPYROOT%\PPY

path=%PPYROOT%\tools;%path%
::
: -------------
::
set destpath=%PPYROOT%\distrib\v%VER%
set destpath_src=%PPYROOT%\distrib\src\v%VER%

call    %PPYROOT%\backup\backup.bat creatures
if exist %PPYROOT%\backup\ppsrc.7z ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
if exist %PPYROOT%\backup\ppmanual.7z ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
if exist %PPYROOT%\backup\pptools.7z ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )

xcopy   %PPYROOT%\tools\rptgen.exe         %PPYREDIST%
xcopy   %PPYROOT%\src\rsrc\rpt\stdrpt.ini  %PPYREDIST%
xcopy   %PPYROOT%\src\rsrc\rpt\pprpt.rpt   %PPYREDIST%
xcopy   %PPYROOT%\src\dl200\dl200c.exe     %PPYREDIST%
xcopy   %PPYROOT%\src\dl200\pattern.dl2    %PPYREDIST%

move /Y %PPYREDIST%\pprpt.rpt              %PPYREDIST%\pattern.rpt 

rem xdel    %destpath%\*.* /S /N /D
rem xdel    %destpath_src%\*.* /S /N /D
del     /S /Q %destpath%\*.* 
del     /S /Q %destpath_src%\*.*
mkdir   %destpath%
mkdir   %destpath%\OLDINST
mkdir   %destpath%\OLDINST\DISK1
mkdir   %destpath%\OLDINST\DISK2
mkdir   %destpath%\OLDINST\DISK_W32
mkdir   %destpath%\MANUAL
mkdir   %destpath%\W32
mkdir   %destpath%\W32\RPT
mkdir   %destpath%\DOC
mkdir   %destpath_src%

xcopy   %PPYROOT%\backup\ppsrc.7z    %destpath_src%\
xcopy   %PPYROOT%\backup\ppmanual.7z %destpath%\manual\
if exist %destpath_src%\ppsrc.7z ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
if exist %destpath%\manual\ppmanual.7z ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
xcopy   %PPYTARGET%\bin\pp.res     %destpath%\bin\         
xcopy   %PPYTARGET%\bin\pprpt.res  %destpath%\bin\         
xcopy   %PPYTARGET%\bin\pp.str     %destpath%\bin\         
xcopy   %PPYTARGET%\bin\ppraw.res  %destpath%\bin\         
xcopy   %PPYTARGET%\bin\ppexp.bin  %destpath%\bin\
xcopy   %PPYTARGET%\bin\ppifc.bin  %destpath%\bin\
xcopy   %PPYTARGET%\bin\ppw.exe    %destpath%\w32\
xcopy   %PPYREDIST%\pp.hlp         %destpath%\bin\
xcopy   %PPYREDIST%\ppos.exe       %destpath%\bin\         
xcopy   %PPYREDIST%\pp.ini         %destpath%\bin\
xcopy   %PPYREDIST%\mkmenu.bat     %destpath%\bin\
xcopy   %PPYSRC%\include\ppmenu.h  %destpath%\bin\
xcopy   %PPYSRC%\rsrc\rc\ppmenu.rc %destpath%\bin\
xcopy   %PPYSRC%\rsrc\crpt\*.rpt   %destpath%\w32\rpt\     
xcopy   %PPYSRC%\rsrc\crpt\*.bmp   %destpath%\w32\rpt\     
xcopy   %PPYROOT%\base\init\*.ddf  %destpath%\sys\ /S

xcopy   %PPYROOT%\src\doc\version.txt  %destpath%\doc\       
xcopy   %PPYROOT%\src\doc\cmdline.txt  %destpath%\doc\       
xcopy   %PPYROOT%\src\doc\pp_ini.txt   %destpath%\doc\       
xcopy   %PPYROOT%\src\doc\qref.txt     %destpath%\doc\       

xcopy   %PPYSRC%\rsrc\dl600\makedl6.bat %destpath%\dd\        
xcopy   %PPYSRC%\rsrc\dl600\ppexp.dl6   %destpath%\dd\        
xcopy   %PPYREDIST%\pattern.dl2         %destpath%\dd\        

rem fd  %destpath%\*.* /T12:00:00 /S
xcopy   %PPYREDIST%\rc.exe      %destpath%\bin\       
xcopy   %PPYREDIST%\rc.gid      %destpath%\bin\       
xcopy   %PPYREDIST%\rc.hlp      %destpath%\bin\       
xcopy   %PPYREDIST%\rcdll.dll   %destpath%\bin\       
xcopy   %PPYREDIST%\rptgen.exe  %destpath%\bin\       

7z a -r %destpath%\OLDINST\disk_w32\w32.7z %destpath%\w32\*.*
7z a    %destpath%\OLDINST\disk1\sys  %destpath%\sys\*.*

7z a %destpath%\OLDINST\disk1\dd   %destpath%\dd\*.*
7z a %destpath%\OLDINST\disk1\doc  %destpath%\doc\*.*
7z a %destpath%\OLDINST\disk2\bin  %destpath%\bin\*.*

del  %PPYROOT%\base\empty.7z
7z a %PPYROOT%\base\empty.7z  %PPYROOT%\base\empty\*.btr
del  %PPYROOT%\base\sample.7z
7z a %PPYROOT%\base\sample.7z %PPYROOT%\base\sample\*.btr

rem @v5.7.0 xcopy   %PPYREDIST%\install.exe %destpath%\OLDINST\disk1
rem @v5.7.0 xcopy   %PPYREDIST%\install.res %destpath%\OLDINST\disk1
rem @v5.7.0 xcopy   %PPYREDIST%\install.ini %destpath%\OLDINST\disk1
xcopy   %PPYREDIST%\7z.exe      %destpath%\OLDINST\disk1
xcopy   %PPYREDIST%\ppfiles.7z  %destpath%\OLDINST\disk1
xcopy   %PPYROOT%\base\empty.7z %destpath%\OLDINST\disk1

rem xdel    %destpath%\bin\*.* /N
del     /Q %destpath%\bin\*.*
rmdir   %destpath%\bin
rem xdel    %destpath%\w32\*.* /S /N /D
del     /Q %destpath%\w32\*.*
rmdir   %destpath%\w32
rem xdel    %destpath%\sys\*.* /N
del     /Q %destpath%\sys\*.*
rmdir   %destpath%\sys
rem xdel    %destpath%\doc\*.* /N
del     /Q %destpath%\doc\*.*
rmdir   %destpath%\doc
rem xdel    %destpath%\dd\*.*  /N
del     /Q %destpath%\dd\*.*
rmdir   %destpath%\dd

cd %PPYROOT%\SRC\BUILD

SET NSILOG=%PPYSRC%\BUILD\LOG\nsis
mkdir %NSILOG%
SET NSISEXE="%PPYROOT%\TOOLS\nsis\makensis.exe"
rem SET NSISFILE=%PPYSRC%\setup\nsis\papyrus.nsi
SET NSISFILE=papyrus.nsi
SET NSISFILE_PPWS=ppws.nsi
if /I %ASSMTYPE% == demo goto mk_demo
if /I %ASSMTYPE% == manual goto mk_manual
del %NSILOG%\*.log
%NSISEXE% /DPRODUCT_VERSION=%VERDOT% /DSRC_ROOT=%PPYROOT% /NOCD /V2 /P1 /DINSTALL_SERVER /O%NSILOG%\server.log %NSISFILE%
%NSISEXE% /DPRODUCT_VERSION=%VERDOT% /DSRC_ROOT=%PPYROOT% /NOCD /V2 /P1 /DINSTALL_CLIENT /O%NSILOG%\client.log %NSISFILE%
%NSISEXE% /DPRODUCT_VERSION=%VERDOT% /DSRC_ROOT=%PPYROOT% /NOCD /V2 /P1 /DINSTALL_UPDATE /O%NSILOG%\update.log %NSISFILE%
%NSISEXE% /DPRODUCT_VERSION=%VERDOT% /DSRC_ROOT=%PPYROOT% /NOCD /V2 /P1 /O%NSILOG%\jobsrvr.log %NSISFILE_PPWS%

copy PpyServer_*.exe %destpath% /Y
del PpyServer_*.exe
copy PpyClient_*.exe %destpath% /Y
del PpyClient_*.exe
copy PpyUpdate_*.exe %destpath% /Y
del PpyUpdate_*.exe
copy PpyJobSrvr_*.exe %destpath% /Y
del PpyJobSrvr_*.exe

if exist %destpath%\PpyServer_*.exe ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
if exist %destpath%\PpyClient_*.exe ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
if exist %destpath%\PpyUpdate_*.exe ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
if exist %destpath%\PpyJobSrvr_*.exe ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )

if /I %ASSMTYPE% ==distanddemo goto mk_demo
goto done_nsis

:mk_demo
%NSISEXE% /DPRODUCT_VERSION=%VERDOT% /DSRC_ROOT=%PPYROOT% /NOCD /V2 /P1 /DINSTALL_DEMO /O%NSILOG%\demo.log %NSISFILE%
copy PpyDemo_*.exe %destpath% /Y
del PpyDemo_*.exe
if exist %destpath%\PpyDemo_*.exe ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
goto done_nsis

:mk_manual
mkdir %destpath%
%NSISEXE% /DPRODUCT_VERSION=%VERDOT% /DSRC_ROOT=%PPYROOT% /NOCD /V2 /P1 /DINSTALL_MANUAL /O%NSILOG%\manual.log %NSISFILE%
copy PpyManual_*.exe %destpath% /Y
del PpyManual_*.exe
if exist %destpath%\PpyManual_*.exe ( echo OK ) else ( set MKDIST_STATUS=mkdist.failed )
:done_nsis
rem -----------------------------

del ..\..\base\empty.7z
del ..\..\base\sample.7z
del %PPYSRC%\build\log\mkdist.failed
echo status > %PPYSRC%\build\log\%MKDIST_STATUS%
