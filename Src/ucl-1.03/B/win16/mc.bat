@echo // Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
@echo //
@echo //   Windows 16-bit
@echo //   Microsoft C/C++ (using QuickWin)
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=cl -nologo -AL -G2 -Mq
set CF=-O -Gf -W3 %CFI%
set LF=/seg:256 /stack:8096 /nod:llibce /map

%CC% %CF% -c src\*.c
@if errorlevel 1 goto error
lib /nologo %BLIB% @b\dos16\bc.rsp;
@if errorlevel 1 goto error

%CC% %CF% -c examples\simple.c
@if errorlevel 1 goto error
link %LF% simple.obj,,,llibcewq.lib libw.lib %BLIB%;
@if errorlevel 1 goto error
%CC% %CF% -c examples\uclpack.c
@if errorlevel 1 goto error
link %LF% uclpack.obj,,,llibcewq.lib libw.lib %BLIB%;
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
