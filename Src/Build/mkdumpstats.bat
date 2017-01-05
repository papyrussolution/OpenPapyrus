set DUMPSTATSDIR=%PPYSRC%\build\dumpstats
set GNUTOOLSBIN=..\..\tools\usr\local\wbin
set PATH=%PATH%;%GNUTOOLSBIN%
set PPW_STATFILE=%DUMPSTATSDIR%\ppw.exe.tbl
set PPWS_STATFILE=%DUMPSTATSDIR%\ppws.exe.tbl
set TABFILE=%DUMPSTATSDIR%\tabfile
set EOLFILE=%DUMPSTATSDIR%\eolfile
set VLNFILE=%DUMPSTATSDIR%\vlnfile
set STRIPCMD=tr -delete \n

xdel %DUMPSTATSDIR%\*
mkdir %DUMPSTATSDIR%

dumpbin /headers %PPYROOT%\ppy\bin\ppw.exe > %DUMPSTATSDIR%\ppw.exe.headump.tmp
dumpbin /headers %PPYROOT%\ppy\bin\ppws.exe > %DUMPSTATSDIR%\ppws.exe.headump.tmp

date /t | tr -d \r\n >> %PPW_STATFILE%
echo - | tr -d \r\n >> %PPW_STATFILE%
time /t | tr -d \r\n >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 size.of.code %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 size.of.initialized.data %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 size.of.uninitialized.data %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 entry.point %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 section.alignment %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 file.alignment %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
agrep -V0 size.of.image %DUMPSTATSDIR%\ppw.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPW_STATFILE%
cat %TABFILE% >> %PPW_STATFILE%
cat %VLNFILE% >> %PPW_STATFILE%
cat %EOLFILE% >> %PPW_STATFILE%

date /t | tr -d \r\n >> %PPWS_STATFILE%
echo - | tr -d \r\n >> %PPWS_STATFILE%
time /t | tr -d \r\n >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 size.of.code %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 size.of.initialized.data %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 size.of.uninitialized.data %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 entry.point %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 section.alignment %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 file.alignment %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
agrep -V0 size.of.image %DUMPSTATSDIR%\ppws.exe.headump.tmp | tr -d \r\n[:lower:][:blank:] >> %PPWS_STATFILE%
cat %TABFILE% >> %PPWS_STATFILE%
cat %VLNFILE% >> %PPWS_STATFILE%
cat %EOLFILE% >> %PPWS_STATFILE%
