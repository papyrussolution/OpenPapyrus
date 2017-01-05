@echo // Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
@echo //
@echo //   DOS 32-bit
@echo //   Borland C/C++ + Borland PowerPack 1.0 (DPMI32)
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=bcc32 -WX
set CF=-O2 -w -w-aus %CFI% -Iacc %CFASM%
set LF=%BLIB%

%CC% %CF% -Isrc -c @b\src.rsp
@if errorlevel 1 goto error
tlib %BLIB% @b\win32\bc.rsp
@if errorlevel 1 goto error

%CC% %CF% -ls -Iexamples examples\simple.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -ls -Iexamples examples\uclpack.c %LF%
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
