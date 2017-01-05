rem
rem Скрипт для создания дистрибутива сервера задач
rem

set DEBUG=1

set PPYROOT=c:\papyrus
set PPYSRC=%PPYROOT%\src
set PPYREDIST=%PPYSRC%\REDIST

set PPWSDIR=%PPYSRC%\BuildVC70
set PPWSSLN=%PPWSDIR%\papyrus.sln

set VCIDE="D:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.exe"

set TARGET="ServerRelease"
:set TARGET="ServerDebug"


set PRODUCT_VERSION=1_56
set SETUPDIR=..\Setup\NSIS
set PPWSNSI=%SETUPDIR%\ppws.nsi 
set DESTPATH=..\..\DISTRIB\PPWS\VER_%PRODUCT_VERSION%

if %DEBUG% == 1 goto mkdist

:
: Building PPWS
:
mkdir %PPYSRC%\BUILD\LOG
SET BUILDLOG=%PPYSRC%\BUILD\LOG\ppws.log
del %BUILDLOG%
%VCIDE% %PPWSSLN% /rebuild %TARGET% /out %BUILDLOG%

:mkdist
xcopy c:\ppy\bin\ppws.exe %PPYREDIST% /Y
..\..\TOOLS\NSIS\makensis.exe /DPRODUCT_VERSION=%PRODUCT_VERSION% %PPWSNSI%

md %DESTPATH%

copy %SETUPDIR%\PpyJobSrvr_*.exe %DESTPATH% /Y
del %SETUPDIR%\PpyJobSrvr_*.exe
