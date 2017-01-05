@call b\unset.bat
@call b\clean.bat

@set CFI=-Iinclude -I.
@set CFASM=-DUCL_USE_ASM
@set BNAME=ucl
@set BLIB=ucl.lib
@set BDLL=ucl.dll

@echo Compiling, please be patient...
