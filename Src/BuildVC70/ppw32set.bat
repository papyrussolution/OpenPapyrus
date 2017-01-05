set PPYROOT=\papyrus
set PPYSRC=%PPYROOT%\src
set PPYTARGET=%PPYROOT%\PPY

rem xcopy %PPYSRC%\obj\dd.bin     %PPYTARGET%\bin /Y
rem xcopy %PPYSRC%\obj\ppexp.bin  %PPYTARGET%\bin /Y
rem xcopy %PPYSRC%\obj\ppifc.bin  %PPYTARGET%\bin /Y
rem xcopy %PPYSRC%\obj\pp.res     %PPYTARGET%\bin /Y
rem xcopy %PPYSRC%\obj\ppraw.res  %PPYTARGET%\bin /Y
rem xcopy %PPYSRC%\obj\pprpt.res  %PPYTARGET%\bin /Y
rem xcopy %PPYSRC%\obj\pp.str     %PPYTARGET%\bin /Y
rem tdstrip %PPYSRC%\obj\pp.exe
rem xcopy %PPYSRC%\obj\pp.exe %PPYTARGET%\bin /Y

xcopy %PPYTARGET%\bin\dd.bin     ..\redist /Y
xcopy %PPYTARGET%\bin\pp.res     ..\redist /Y
xcopy %PPYTARGET%\bin\ppraw.res  ..\redist /Y
xcopy %PPYTARGET%\bin\pprpt.res  ..\redist /Y
xcopy %PPYTARGET%\bin\pp.str     ..\redist /Y
xcopy %PPYTARGET%\bin\ppw.exe    ..\redist /Y
xcopy %PPYSRC%\ppaldd\dd.ald  ..\redist /Y
