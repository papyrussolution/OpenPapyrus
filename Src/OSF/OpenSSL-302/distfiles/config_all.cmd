setlocal

set OPENSSL_VER=3.0.2
set OPENSSL_VER_SED=3\.0\.2
set OPENSSL_BASE=openssl-%OPENSSL_VER%
set OPENSSL_BASE_SED=openssl-%OPENSSL_VER_SED%
set OPENSSL_DIR=..\%OPENSSL_BASE%
set OPENSSL_DIR_SED=\.\.\\\\openssl-%OPENSSL_VER_SED%

set ZLIB_DIR=..\zlib

set _GEN_LIST_INCL=^
  include\crypto\bn_conf.h ^
  include\crypto\dso_conf.h ^
  include\openssl\asn1.h ^
  include\openssl\asn1t.h ^
  include\openssl\bio.h ^
  include\openssl\cmp.h ^
  include\openssl\cms.h ^
  include\openssl\conf.h ^
  include\openssl\configuration.h ^
  include\openssl\crmf.h ^
  include\openssl\crypto.h ^
  include\openssl\ct.h ^
  include\openssl\err.h ^
  include\openssl\ess.h ^
  include\openssl\fipskey.h ^
  include\openssl\lhash.h ^
  include\openssl\ocsp.h ^
  include\openssl\opensslv.h ^
  include\openssl\pkcs12.h ^
  include\openssl\pkcs7.h ^
  include\openssl\safestack.h ^
  include\openssl\srp.h ^
  include\openssl\ssl.h ^
  include\openssl\ui.h ^
  include\openssl\x509.h ^
  include\openssl\x509_vfy.h ^
  include\openssl\x509v3.h

set _GEN_LIST_PROV_INCL=^
  providers\common\include\prov\der_digests.h ^
  providers\common\include\prov\der_dsa.h ^
  providers\common\include\prov\der_ec.h ^
  providers\common\include\prov\der_ecx.h ^
  providers\common\include\prov\der_rsa.h ^
  providers\common\include\prov\der_sm2.h ^
  providers\common\include\prov\der_wrap.h

set _GEN_LIST_PROV_CSRC=^
  providers\common\der\der_digests_gen.c ^
  providers\common\der\der_dsa_gen.c ^
  providers\common\der\der_ec_gen.c ^
  providers\common\der\der_ecx_gen.c ^
  providers\common\der\der_rsa_gen.c ^
  providers\common\der\der_sm2_gen.c ^
  providers\common\der\der_wrap_gen.c

set _GEN_LIST=^
  %_GEN_LIST_INCL% ^
  %_GEN_LIST_PROV_INCL% ^
  %_GEN_LIST_PROV_CSRC% ^
  apps\progs.c apps\progs.h ^
  apps\CA.pl apps\tsget.pl tools\c_rehash.pl util\wrap.pl

mkdir dll64
mkdir lib64
mkdir dll32
mkdir lib32
mkdir dllarm64
mkdir libarm64
mkdir dllarm32
mkdir libarm32

pushd dll64
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\x64\Release\libz-static.lib VC-WIN64A-masm no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd lib64
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\x64\Release\libz-static.lib VC-WIN64A-masm no-shared no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd dll32
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles(x86)%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\Release\libz-static.lib VC-WIN32 no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd lib32
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles(x86)%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\Release\libz-static.lib VC-WIN32 no-shared no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd dllarm64
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\ARM64\Release\libz-static.lib VC-WIN64-ARM no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd libarm64
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\ARM64\Release\libz-static.lib VC-WIN64-ARM no-shared no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd dllarm32
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\ARM\Release\libz-static.lib VC-WIN32-ARM no-dynamic-engine zlib
call :genfile
call :clndir
popd

pushd libarm32
perl %OPENSSL_DIR%\Configure --prefix="%ProgramFiles%\OpenSSL-3" --with-zlib-include=%ZLIB_DIR% --with-zlib-lib=%ZLIB_DIR%\build\ARM\Release\libz-static.lib VC-WIN32-ARM no-shared no-dynamic-engine zlib
call :genfile
call :clndir
popd

goto :end

:genfile
for %%f in ( %_GEN_LIST_INCL% ) do (
  perl -I. -Mconfigdata %OPENSSL_DIR%\util\dofile.pl -omakefile %OPENSSL_DIR%\%%f.in > %%f
)
for %%f in ( %_GEN_LIST_PROV_INCL% %_GEN_LIST_PROV_CSRC% ) do (
  perl -I. -I%OPENSSL_DIR%\providers\common\der -Mconfigdata -Moids_to_c %OPENSSL_DIR%\util\dofile.pl -omakefile %OPENSSL_DIR%\%%f.in > %%f
)
perl %OPENSSL_DIR%\apps\progs.pl -C apps\openssl > apps\progs.c
perl %OPENSSL_DIR%\apps\progs.pl -H apps\openssl > apps\progs.h
perl -I. -Mconfigdata %OPENSSL_DIR%\util\dofile.pl -omakefile %OPENSSL_DIR%\apps\CA.pl.in > apps\CA.pl
perl -I. -Mconfigdata %OPENSSL_DIR%\util\dofile.pl -omakefile %OPENSSL_DIR%\apps\tsget.in > apps\tsget.pl
perl -I. -Mconfigdata %OPENSSL_DIR%\util\dofile.pl -omakefile %OPENSSL_DIR%\tools\c_rehash.in > tools\c_rehash.pl
perl -I. -Mconfigdata %OPENSSL_DIR%\util\dofile.pl -omakefile %OPENSSL_DIR%\util\wrap.pl.in > util\wrap.pl
ren configdata.pm configdata.pm.org
@rem Redirection must be at front for "^^" to work. Strange.
>configdata.pm sed -e "s/%OPENSSL_DIR_SED%/\./g" -e "s/\(['\"]\)[A-Za-z]:[^^'\"]*\/%OPENSSL_BASE_SED%\(['\"\/]\)/\1\.\2/" configdata.pm.org
dos2unix %_GEN_LIST%
exit /b

:clndir
@echo off
call :clndir0
@echo on
exit /b

:clndir0
for /d %%d in ( * ) do (
    pushd %%d
    call :clndir0
    popd
    rmdir %%d 2>nul
)
exit /b

:end
endlocal
