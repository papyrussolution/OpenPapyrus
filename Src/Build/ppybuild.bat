rem
rem Скрипт для создания дистрибутива Papyrus
rem

rem Первый параметр - тип сборки (/manual | /demo | /test), если пустой - обычная сборка
set ASSMTYPE=%1%

set DEBUG=0
set MKDEMO=0
set TEST=0

if %MKDEMO%==0 goto type_done
	set ARG=%ASSMTYPE%/MKDEMO
	:если не передан параметр
	if /I %ARG% NEQ /MKDEMO goto type_done
		set ASSMTYPE=/MKDEMO
:type_done

set PPYROOT=\PAPYRUS
set PPYSRC=%PPYROOT%\src
set COMPILEPATH=%PPYSRC%\buildvc70

set STATUS=OK
del %PPYSRC%\build\log\OK
del %PPYSRC%\build\*detailed*.log
echo status > %PPYSRC%\build\log\FAILED

if %DEBUG% == 1 goto mkdist

rem Creating manual

; @v7.5.3 call %PPYROOT%\manwork\distrib.bat > %PPYSRC%\build\log\detailed_create_manuals.log
; @v7.5.3 if exist %PPYSRC%\build\log\createmanuals.ok (echo created manuals OK) else (set STATUS=FAILED)
:
: Deleting DOS and resource intermediate files 
call clrtemp.bat

rem Building Win32 project

cd   %COMPILEPATH%
call %COMPILEPATH%\_build.bat
cd   %PPYSRC%\build
if exist %PPYSRC%\build\log\build.ok (echo built ok) else (set STATUS=FAILED)
:mkdist
..\..\tools\mknsish.exe ..\rsrc\version\genver.dat /run mkdist.bat %PPYROOT% %ASSMTYPE% > %PPYSRC%\build\log\detailed_mkdist.log
if exist %PPYSRC%\build\log\mkdist.ok (echo mkdist ok) else (set STATUS=FAILED)

rem Gather statistics

if %DEBUG% == 1 goto done

rem set CCCC_CMD=%PPYSRC%\tools\cccc\cccc\cccc.exe
rem set CCCC_OUTDIR=%PPYROOT%\SrcPrc\metrics
rem mkdir %CCCC_OUTDIR%
rem dir /b/s %PPYSRC%\*.h %PPYSRC%\*.c %PPYSRC%\*.cpp | %CCCC_CMD% - --outdir=%CCCC_OUTDIR%
rem call mkdumpstats.bat

:done

del %PPYSRC%\build\log\FAILED
echo status > %PPYSRC%\build\log\%STATUS%
