@echo // Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
@echo //
@echo //   DOS 16-bit
@echo //   Digital Mars C/C++
@echo //
@echo // WARNING: UCL breaks the optimizer
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=dmc -ml
set CF=-w- %CFI%
set LF=%BLIB%

%CC% %CF% -c @b\src.rsp
@if errorlevel 1 goto error
lib %BLIB% /b /c /n /noi @b\dos16\bc.rsp
@if errorlevel 1 goto error

%CC% %CF% examples\simple.c %LF%
@if errorlevel 1 goto error
%CC% %CF% examples\uclpack.c %LF%
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
