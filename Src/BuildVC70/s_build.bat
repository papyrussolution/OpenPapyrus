del s_build.txt
del papyrus.tmp.sln
copy papyrus.sln papyrus.tmp.sln

@SET VSINSTALLDIR=C:\msvs70\Common7\IDE
@SET VCINSTALLDIR=C:\msvs70
@SET FrameworkDir=C:\WINDOWS\Microsoft.NET\Framework
@SET FrameworkVersion=v1.1.4322
@SET FrameworkSDKDir=C:\MSVS70\SDK\v1.1
@if "%VCINSTALLDIR%"=="" set VCINSTALLDIR=%VSINSTALLDIR%
@set DevEnvDir=%VSINSTALLDIR%
@set MSVCDir=%VCINSTALLDIR%\VC7
@set PATH=%DevEnvDir%;%MSVCDir%\BIN;%VCINSTALLDIR%\Common7\Tools;%VCINSTALLDIR%\Common7\Tools\bin\prerelease;%VCINSTALLDIR%\Common7\Tools\bin;%FrameworkSDKDir%\bin;%FrameworkDir%\%FrameworkVersion%;%PATH%;
@set INCLUDE=%MSVCDir%\ATLMFC\INCLUDE;%MSVCDir%\INCLUDE;%MSVCDir%\PlatformSDK\include\prerelease;%MSVCDir%\PlatformSDK\include;%FrameworkSDKDir%\include;%INCLUDE%
@set LIB=%MSVCDir%\ATLMFC\LIB;%MSVCDir%\LIB;%MSVCDir%\PlatformSDK\lib\prerelease;%MSVCDir%\PlatformSDK\lib;%FrameworkSDKDir%\lib;%LIB%

%DevEnvDir%\devenv.exe papyrus.tmp.sln %1 %2 /out s_build.txt
