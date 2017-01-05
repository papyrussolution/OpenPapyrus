@echo // Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
@echo //
@echo //   OS/2 32-bit
@echo //   Zortech C/C++
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=ztc -b -v0 -mf
set CF=-o -w- -r %CFI% -Isrc %CFASM%
set LF=%BLIB%

%CC% %CF% -c @b\src.rsp
@if errorlevel 1 goto error
zorlib %BLIB% @b\win32\bc.rsp
@if errorlevel 1 goto error

%CC% %CF% -c examples\simple.c
@if errorlevel 1 goto error
%CC% simple.obj %LF%
@if errorlevel 1 goto error
%CC% %CF% -c examples\uclpack.c
@if errorlevel 1 goto error
%CC% uclpack.obj %LF%
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
