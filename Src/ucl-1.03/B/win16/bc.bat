@echo // Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
@echo //
@echo //   Windows 16-bit
@echo //   Borland C/C++
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=bcc -ml -2 -tW
set CF=-O1 -d -w %CFI% -Iacc
set LF=%BLIB%

%CC% %CF% -Isrc -c @b\src.rsp
@if errorlevel 1 goto error
tlib %BLIB% @b\dos16\bc.rsp
@if errorlevel 1 goto error

%CC% %CF% -Iexamples examples\simple.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -Iexamples examples\uclpack.c %LF%
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
