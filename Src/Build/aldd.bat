@echo off
SET ALDDPATH=..\PPALDD
..\..\tools\ppalddc %ALDDPATH%\dd.ald
copy %ALDDPATH%\dd.bin     ..\obj\dd.bin
del  %ALDDPATH%\dd.bin
copy %ALDDPATH%\dd.h       ..\include\dd.h
rem del  %ALDDPATH%\dd.h
copy ldstat.cpp ..\pplib\ldstat.cpp
del  ldstat.cpp
DEL  *.TMP