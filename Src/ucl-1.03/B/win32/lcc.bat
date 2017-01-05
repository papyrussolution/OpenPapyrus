@echo // Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
@echo //
@echo //   Windows 32-bit
@echo //   lcc-win32
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=lcc
set CF=-O -A %CFI% -Iacc %CFASM%
set LF=%BLIB% winmm.lib

for %%f in (src\*.c) do %CC% %CF% -c %%f
@if errorlevel 1 goto error
lcclib /out:%BLIB% @b\win32\vc.rsp
@if errorlevel 1 goto error

%CC% -c %CF% examples\simple.c
@if errorlevel 1 goto error
lc simple.obj %LF%
@if errorlevel 1 goto error
%CC% -c %CF% examples\uclpack.c
@if errorlevel 1 goto error
lc uclpack.obj %LF%
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
