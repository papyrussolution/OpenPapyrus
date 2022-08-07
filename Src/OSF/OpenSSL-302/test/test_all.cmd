@echo off
setlocal

cd %~dp0\..
set MY_ROOT=%cd%

echo ROOT: %MY_ROOT%
echo.

for /d %%d in ( build-* ) do (
    for %%r in ( Release Debug x64\Release x64\Debug ) do (
        if exist %MY_ROOT%\%%d\%%r\openssl.exe (
            call test\test_one.cmd %%d\%%r
            echo.
        )
        if exist %MY_ROOT%\%%d\%%r\openssl-static.exe (
            call test\test_one.cmd %%d\%%r -static
            echo.
        )
    )
)

endlocal
