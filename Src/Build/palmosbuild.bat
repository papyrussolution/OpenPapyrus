rem
rem —крипт дл€ создани€ дистрибутива PalmOs
rem

set DEBUG=0

set PPYROOT=c:\papyrus
set PPYSRC=%PPYROOT%\src
set PALMOS=%PPYSRC%\PalmOs

set SPIIDIR=%PALMOS%\SPII
set SPIIMCP=%SPIIDIR%\StyloPalm.mcp
set SPIICND=%SPIIDIR%\StyloConduit
set SPIISLN=%SPIICND%\StyloConduit.sln

set SSBLDIR=%PALMOS%\StyloSymb
set SSBLMCP=%SSBLDIR%\stylosymb.mcp
set SSBLCND=%SSBLDIR%\StyloSymbConduit
set SSBLSLN=%SSBLCND%\StyloSymbConduit.sln

set CWARIDE="C:\Program Files\Metrowerks\CodeWarrior\Bin"
set VCIDE="d:\MSVS70\Common7\IDE\devenv.exe"

set PRODUCT_VERSION=1_66
set SETUP_DIR=..\Setup\NSIS
set PALMOS_NSI=%SETUP_DIR%\palmos.nsi 
set DESTPATH=..\..\DISTRIB\PalmOS\ver%PRODUCT_VERSION%


if %DEBUG% == 0 goto mkdist

:
:”далю результаты предыдущих компил€ций
:

del /F /Q %SPIIDIR%\redist\*.*
del /F /Q %SSBLDIR%\redist\*.*
del /F /Q %SPIIDIR%\release\*.*
del /F /Q %SSBLDIR%\release\*.*

@pause
:
: Building StyloPalm && StyloSymbol
:

chdir /D %CWARIDE%
call CmdIDE %SPIIMCP% /t release /r /b /c /q /s /v y
call CmdIDE %SSBLMCP% /t release /r /b /c /q /s /v y
chdir /D %PPYSRC%\build

@pause
:
: Building StyloConduit && StyloSymbolConduit
:
mkdir %PPYSRC%\BUILD\LOG
set BUILDLOG=%PPYSRC%\BUILD\LOG\SPII.log
del %BUILDLOG%
%VCIDE% %SPIISLN% /rebuild "Release" /out %BUILDLOG%
set BUILDLOG=%PPYSRC%\BUILD\LOG\SSBL.log
del %BUILDLOG%
%VCIDE% %SSBLSLN% /rebuild "Release" /out %BUILDLOG%

@pause

:mkdist
..\..\TOOLS\NSIS\makensis.exe /DPRODUCT_VERSION=%PRODUCT_VERSION% %PALMOS_NSI%

@pause

md %DESTPATH%

copy %SETUP_DIR%\PalmOS*.exe %DESTPATH%
del %SETUP_DIR%\PALMOS*.exe
