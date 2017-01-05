@echo _build.bat
SET BUILDLOG=%PPYSRC%\BUILD\LOG\vc70build
mkdir %BUILDLOG%
set BUILD_STATUS=build.ok
set FULLFLAG=/rebuild
del %PPYSRC%\build\log\build.ok
echo status > %PPYSRC%\build\log\build.failed
del %BUILDLOG%\*.log
SET MSVS_PATH="\msvs70\common7\ide\devenv.exe"
%MSVS_PATH% papyrus.sln %FULLFLAG% "Release" /out %BUILDLOG%\client.log
if exist ..\..\ppy\bin\ppw.exe (echo ppw=OK) else (set BUILD_STATUS=build.failed)
%MSVS_PATH% papyrus.sln %FULLFLAG% "MtDllRelease" /out %BUILDLOG%\mtdll.log
if exist ..\..\ppy\bin\ppwmt.dll (echo ppwmt=OK) else (set BUILD_STATUS=build.failed)
%MSVS_PATH% papyrus.sln %FULLFLAG% "ServerRelease" /out %BUILDLOG%\jobsrv.log
if exist ..\..\ppy\bin\ppws.exe (echo ppws=OK) else (set BUILD_STATUS=build.failed)

%MSVS_PATH% EquipSolution.sln %FULLFLAG% "Release" /out %BUILDLOG%\EquipSolution.log
if exist ..\..\ppy\bin\ppdrv-pirit.dll (echo ppdrv-pirit=OK) else (set BUILD_STATUS=build.failed)

%MSVS_PATH% PPSoapModules.sln %FULLFLAG% "Release" /out %BUILDLOG%\PPSoapModules.log
if exist ..\..\ppy\bin\PPSoapUhtt.dll     (echo PPSoapUhtt=OK)     else (set BUILD_STATUS=build.failed)


del %PPYSRC%\build\log\build.failed
echo status > %PPYSRC%\build\log\%BUILD_STATUS%
